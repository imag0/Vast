#include "ppocr_recognizer.h"

#include <onnxruntime_c_api.h>

/* Android's no-SDK build supplies these symbols at runtime from libc/libdl. */
extern void *dlopen(const char *filename, int flags);
extern void *dlsym(void *handle, const char *symbol);
extern int dlclose(void *handle);
extern const char *dlerror(void);

struct PpocrTimespec {
    long tv_sec;
    long tv_nsec;
};
extern int clock_gettime(int clock_id, struct PpocrTimespec *value);

#define PPOCR_RTLD_NOW 2
#define PPOCR_RTLD_LOCAL 0
#define PPOCR_CLOCK_MONOTONIC 1
#define PPOCR_MAX_MODEL_BYTES (128u * 1024u * 1024u)
#define PPOCR_MAX_DICTIONARY_BYTES (4u * 1024u * 1024u)
#define PPOCR_ERROR_BYTES 256u

typedef struct {
    OcrU32 offset;
    OcrU32 length;
} PpocrToken;

typedef struct {
    OcrU32 low;
    OcrU32 high;
    float weight;
} PpocrResizeMap;

struct PpocrRecognizer {
    OcrU8 *model_bytes;
    OcrSize model_size;

    char *token_bytes;
    OcrSize token_bytes_size;
    PpocrToken *tokens;
    OcrU32 token_count;

    void *ort_library;
    const OrtApi *ort;
    OrtEnv *environment;
    OrtSession *session;
    OrtMemoryInfo *memory_info;
    char *input_name;
    char *output_name;

    float *input_scratch;
    OcrSize input_capacity;
    PpocrResizeMap *x_maps;
    OcrU32 x_map_capacity;

    volatile int busy;
    char last_error[PPOCR_ERROR_BYTES];
};

typedef const OrtApiBase *(ORT_API_CALL *PpocrGetApiBaseFn)(void);

static OcrSize ppocr_strlen(const char *text) {
    OcrSize n = 0;
    if (!text) return 0;
    while (text[n]) ++n;
    return n;
}

static void ppocr_error_clear(PpocrRecognizer *recognizer) {
    if (recognizer) recognizer->last_error[0] = 0;
}

static void ppocr_error_set(PpocrRecognizer *recognizer,
                            const char *context, const char *detail) {
    OcrSize used = 0;
    const char *parts[3];
    OcrU32 i;
    if (!recognizer) return;
    parts[0] = context ? context : "PP-OCR failure";
    parts[1] = detail && detail[0] ? ": " : "";
    parts[2] = detail && detail[0] ? detail : "";
    for (i = 0; i < 3u; ++i) {
        OcrSize j = 0;
        while (parts[i][j] && used + 1u < PPOCR_ERROR_BYTES)
            recognizer->last_error[used++] = parts[i][j++];
    }
    recognizer->last_error[used] = 0;
}

static OcrU64 ppocr_now_us(void) {
    struct PpocrTimespec value;
    if (clock_gettime(PPOCR_CLOCK_MONOTONIC, &value) != 0) return 0;
    if (value.tv_sec < 0 || value.tv_nsec < 0) return 0;
    return (OcrU64)value.tv_sec * 1000000u + (OcrU64)value.tv_nsec / 1000u;
}

static OcrU64 ppocr_elapsed_us(OcrU64 start, OcrU64 end) {
    return start && end >= start ? end - start : 0;
}

static int ppocr_try_lock(volatile int *lock) {
#if defined(__aarch64__)
    int previous;
    int store_failed;
    __asm__ volatile(
        "0: ldaxr %w0, [%2]\n"
        "cbnz %w0, 1f\n"
        "stxr %w1, %w3, [%2]\n"
        "cbnz %w1, 0b\n"
        "1:\n"
        : "=&r"(previous), "=&r"(store_failed)
        : "r"(lock), "r"(1)
        : "memory");
    return previous != 0;
#else
    return __atomic_exchange_n(lock, 1, __ATOMIC_ACQUIRE) != 0;
#endif
}

static void ppocr_unlock(volatile int *lock) {
#if defined(__aarch64__)
    __asm__ volatile("stlr wzr, [%0]" : : "r"(lock) : "memory");
#else
    __atomic_store_n(lock, 0, __ATOMIC_RELEASE);
#endif
}

static int ppocr_finite_probability(float value) {
    union {
        float f;
        OcrU32 u;
    } bits;
    bits.f = value;
    return (bits.u & 0x7f800000u) != 0x7f800000u &&
           value >= -0.000001f && value <= 1.000001f;
}

static int ppocr_utf8_valid(const OcrU8 *bytes, OcrSize length) {
    OcrSize i = 0;
    while (i < length) {
        OcrU32 c = bytes[i++];
        OcrU32 value;
        OcrU32 need;
        OcrU32 minimum;
        if (c == 0u) return 0;
        if (c < 0x80u) continue;
        if (c >= 0xc2u && c <= 0xdfu) {
            value = c & 0x1fu;
            need = 1u;
            minimum = 0x80u;
        } else if (c >= 0xe0u && c <= 0xefu) {
            value = c & 0x0fu;
            need = 2u;
            minimum = 0x800u;
        } else if (c >= 0xf0u && c <= 0xf4u) {
            value = c & 0x07u;
            need = 3u;
            minimum = 0x10000u;
        } else {
            return 0;
        }
        if ((OcrSize)need > length - i) return 0;
        while (need--) {
            OcrU32 next = bytes[i++];
            if ((next & 0xc0u) != 0x80u) return 0;
            value = (value << 6) | (next & 0x3fu);
        }
        if (value < minimum || value > 0x10ffffu ||
            (value >= 0xd800u && value <= 0xdfffu))
            return 0;
    }
    return 1;
}

static int ppocr_parse_dictionary(PpocrRecognizer *recognizer,
                                  const OcrU8 *bytes, OcrSize size) {
    OcrSize position = 0;
    OcrSize output_position = 0;
    OcrU32 line_count = 0;
    if (!recognizer || !bytes || !size ||
        size > (OcrSize)PPOCR_MAX_DICTIONARY_BYTES)
        return PPOCR_ERR_DICTIONARY;
    if (size + 1u < size) return PPOCR_ERR_LIMIT;
    recognizer->tokens = (PpocrToken *)calloc(
        PPOCR_DICTIONARY_ENTRIES + 1u, sizeof(PpocrToken));
    recognizer->token_bytes = (char *)malloc(size + 1u);
    if (!recognizer->tokens || !recognizer->token_bytes)
        return PPOCR_ERR_NOMEM;

    while (position < size) {
        OcrSize start = position;
        OcrSize end;
        OcrSize length;
        while (position < size && bytes[position] != (OcrU8)'\n') ++position;
        end = position;
        if (position < size) ++position;
        if (end > start && bytes[end - 1u] == (OcrU8)'\r') --end;
        length = end - start;
        if (!length) return PPOCR_ERR_DICTIONARY;
        if (line_count >= PPOCR_DICTIONARY_ENTRIES ||
            length > (OcrSize)0xffffffffu ||
            !ppocr_utf8_valid(bytes + start, length) ||
            (length == 1u && bytes[start] == (OcrU8)' '))
            return PPOCR_ERR_DICTIONARY;
        recognizer->tokens[line_count].offset = (OcrU32)output_position;
        recognizer->tokens[line_count].length = (OcrU32)length;
        memcpy(recognizer->token_bytes + output_position, bytes + start, length);
        output_position += length;
        ++line_count;
    }
    if (line_count != PPOCR_DICTIONARY_ENTRIES)
        return PPOCR_ERR_DICTIONARY;
    if (recognizer->tokens[0].length != 3u ||
        (OcrU8)recognizer->token_bytes[0] != 0xe3u ||
        (OcrU8)recognizer->token_bytes[1] != 0x80u ||
        (OcrU8)recognizer->token_bytes[2] != 0x80u)
        return PPOCR_ERR_DICTIONARY;

    recognizer->tokens[line_count].offset = (OcrU32)output_position;
    recognizer->tokens[line_count].length = 1u;
    recognizer->token_bytes[output_position++] = ' ';
    recognizer->token_bytes_size = output_position;
    recognizer->token_count = line_count + 1u;
    return PPOCR_OK;
}

static char *ppocr_copy_cstring(const char *source) {
    OcrSize length = ppocr_strlen(source);
    char *copy;
    if (!source || length + 1u < length) return 0;
    copy = (char *)malloc(length + 1u);
    if (!copy) return 0;
    memcpy(copy, source, length + 1u);
    return copy;
}

static int ppocr_ort_ok(PpocrRecognizer *recognizer, OrtStatus *status,
                        const char *context) {
    const char *message;
    if (!status) return 1;
    message = recognizer->ort && recognizer->ort->GetErrorMessage
                  ? recognizer->ort->GetErrorMessage(status)
                  : "ONNX Runtime error";
    ppocr_error_set(recognizer, context, message);
    if (recognizer->ort && recognizer->ort->ReleaseStatus)
        recognizer->ort->ReleaseStatus(status);
    return 0;
}

static void ppocr_release_runtime(PpocrRecognizer *recognizer) {
    if (!recognizer) return;
    if (recognizer->ort) {
        if (recognizer->session)
            recognizer->ort->ReleaseSession(recognizer->session);
        if (recognizer->memory_info)
            recognizer->ort->ReleaseMemoryInfo(recognizer->memory_info);
        if (recognizer->environment)
            recognizer->ort->ReleaseEnv(recognizer->environment);
    }
    recognizer->session = 0;
    recognizer->memory_info = 0;
    recognizer->environment = 0;
    if (recognizer->input_name) free(recognizer->input_name);
    if (recognizer->output_name) free(recognizer->output_name);
    recognizer->input_name = 0;
    recognizer->output_name = 0;
    recognizer->ort = 0;
    if (recognizer->ort_library) dlclose(recognizer->ort_library);
    recognizer->ort_library = 0;
}

static int ppocr_validate_io_type(PpocrRecognizer *recognizer, int input) {
    OrtTypeInfo *type_info = 0;
    const OrtTensorTypeAndShapeInfo *tensor_info = 0;
    ONNXTensorElementDataType element_type;
    int64_t dimensions[4];
    size_t dimension_count = 0;
    OrtStatus *status;
    int result = PPOCR_ERR_SHAPE;

    status = input
                 ? recognizer->ort->SessionGetInputTypeInfo(
                       recognizer->session, 0u, &type_info)
                 : recognizer->ort->SessionGetOutputTypeInfo(
                       recognizer->session, 0u, &type_info);
    if (!ppocr_ort_ok(recognizer, status,
                      input ? "read model input type" :
                              "read model output type"))
        return PPOCR_ERR_MODEL;
    if (!ppocr_ort_ok(recognizer,
                      recognizer->ort->CastTypeInfoToTensorInfo(
                          type_info, &tensor_info),
                      input ? "read model input tensor" :
                              "read model output tensor"))
        goto done;
    if (!tensor_info) {
        ppocr_error_set(recognizer,
                        input ? "model input is not a tensor" :
                                "model output is not a tensor", 0);
        goto done;
    }
    if (!ppocr_ort_ok(recognizer,
                      recognizer->ort->GetTensorElementType(
                          tensor_info, &element_type),
                      "read tensor element type"))
        goto done;
    if (element_type != ONNX_TENSOR_ELEMENT_DATA_TYPE_FLOAT) {
        ppocr_error_set(recognizer, "model tensor is not float32", 0);
        goto done;
    }
    if (!ppocr_ort_ok(recognizer,
                      recognizer->ort->GetDimensionsCount(
                          tensor_info, &dimension_count),
                      "read tensor rank"))
        goto done;
    if (dimension_count != (input ? 4u : 3u)) {
        ppocr_error_set(recognizer,
                        input ? "model input rank is not 4" :
                                "model output rank is not 3", 0);
        goto done;
    }
    if (!ppocr_ort_ok(recognizer,
                      recognizer->ort->GetDimensions(
                          tensor_info, dimensions, dimension_count),
                      "read tensor dimensions"))
        goto done;
    if (dimensions[0] > 0 && dimensions[0] != 1) {
        ppocr_error_set(recognizer, "model batch dimension rejects batch 1", 0);
        goto done;
    }
    if (input) {
        if ((dimensions[1] > 0 && dimensions[1] != 3) ||
            (dimensions[2] > 0 &&
             dimensions[2] != (int64_t)PPOCR_INPUT_HEIGHT) ||
            dimensions[3] > 0) {
            ppocr_error_set(recognizer,
                            "model input is not dynamic [1,3,48,W]", 0);
            goto done;
        }
    } else {
        if (dimensions[2] > 0 &&
            dimensions[2] != (int64_t)PPOCR_CLASS_COUNT) {
            ppocr_error_set(recognizer,
                            "model output class count is not 18385", 0);
            goto done;
        }
    }
    result = PPOCR_OK;
done:
    if (type_info) recognizer->ort->ReleaseTypeInfo(type_info);
    return result;
}

static int ppocr_create_session(PpocrRecognizer *recognizer) {
    OrtSessionOptions *options = 0;
    OrtAllocator *allocator = 0;
    char *ort_input_name = 0;
    char *ort_output_name = 0;
    size_t input_count = 0;
    size_t output_count = 0;
    const OrtApiBase *api_base;
    PpocrGetApiBaseFn get_api_base;
    void *symbol;
    int result = PPOCR_ERR_RUNTIME;

    if (recognizer->session) return PPOCR_OK;
    recognizer->ort_library = dlopen("libonnxruntime.so",
                                    PPOCR_RTLD_NOW | PPOCR_RTLD_LOCAL);
    if (!recognizer->ort_library) {
        ppocr_error_set(recognizer, "load libonnxruntime.so", dlerror());
        goto fail;
    }
    (void)dlerror();
    symbol = dlsym(recognizer->ort_library, "OrtGetApiBase");
    if (!symbol) {
        ppocr_error_set(recognizer, "resolve OrtGetApiBase", dlerror());
        goto fail;
    }
    if (sizeof(get_api_base) != sizeof(symbol)) {
        ppocr_error_set(recognizer,
                        "OrtGetApiBase function-pointer size mismatch", 0);
        goto fail;
    }
    __builtin_memcpy(&get_api_base, &symbol, sizeof(get_api_base));
    api_base = get_api_base ? get_api_base() : 0;
    if (!api_base || !api_base->GetApi) {
        ppocr_error_set(recognizer, "read ONNX Runtime API base", 0);
        goto fail;
    }
    recognizer->ort = api_base->GetApi(ORT_API_VERSION);
    if (!recognizer->ort) {
        ppocr_error_set(recognizer,
                        "ONNX Runtime does not support C API version 28", 0);
        goto fail;
    }
    if (!ppocr_ort_ok(recognizer,
                      recognizer->ort->CreateEnv(ORT_LOGGING_LEVEL_ERROR,
                                                  "vast-ppocr",
                                                  &recognizer->environment),
                      "create ONNX Runtime environment"))
        goto fail;
    if (!ppocr_ort_ok(recognizer,
                      recognizer->ort->DisableTelemetryEvents(
                          recognizer->environment),
                      "disable ONNX Runtime telemetry"))
        goto fail;
    if (!ppocr_ort_ok(recognizer,
                      recognizer->ort->CreateSessionOptions(&options),
                      "create ONNX Runtime session options"))
        goto fail;
    if (!ppocr_ort_ok(recognizer,
                      recognizer->ort->SetIntraOpNumThreads(options, 1),
                      "set intra-op thread count") ||
        !ppocr_ort_ok(recognizer,
                      recognizer->ort->SetInterOpNumThreads(options, 1),
                      "set inter-op thread count") ||
        !ppocr_ort_ok(recognizer,
                      recognizer->ort->SetSessionExecutionMode(
                          options, ORT_SEQUENTIAL),
                      "set sequential execution") ||
        !ppocr_ort_ok(recognizer,
                      recognizer->ort->DisableMemPattern(options),
                      "disable memory pattern") ||
        !ppocr_ort_ok(recognizer,
                      recognizer->ort->DisableCpuMemArena(options),
                      "disable CPU memory arena") ||
        !ppocr_ort_ok(recognizer,
                      recognizer->ort->SetSessionGraphOptimizationLevel(
                          options, ORT_ENABLE_BASIC),
                      "set graph optimization level") ||
        !ppocr_ort_ok(recognizer,
                      recognizer->ort->SetSessionLogSeverityLevel(
                          options, ORT_LOGGING_LEVEL_ERROR),
                      "set session log severity"))
        goto fail;
    if (!ppocr_ort_ok(recognizer,
                      recognizer->ort->CreateSessionFromArray(
                          recognizer->environment,
                          recognizer->model_bytes,
                          (size_t)recognizer->model_size,
                          options, &recognizer->session),
                      "load PP-OCRv5 ONNX model")) {
        result = PPOCR_ERR_MODEL;
        goto fail;
    }
    recognizer->ort->ReleaseSessionOptions(options);
    options = 0;

    if (!ppocr_ort_ok(recognizer,
                      recognizer->ort->SessionGetInputCount(
                          recognizer->session, &input_count),
                      "read model input count") ||
        !ppocr_ort_ok(recognizer,
                      recognizer->ort->SessionGetOutputCount(
                          recognizer->session, &output_count),
                      "read model output count")) {
        result = PPOCR_ERR_MODEL;
        goto fail;
    }
    if (input_count != 1u || output_count != 1u) {
        ppocr_error_set(recognizer,
                        "model must have exactly one input and one output", 0);
        result = PPOCR_ERR_SHAPE;
        goto fail;
    }
    if (!ppocr_ort_ok(recognizer,
                      recognizer->ort->GetAllocatorWithDefaultOptions(
                          &allocator),
                      "get ONNX Runtime allocator") ||
        !ppocr_ort_ok(recognizer,
                      recognizer->ort->SessionGetInputName(
                          recognizer->session, 0u, allocator,
                          &ort_input_name),
                      "read model input name") ||
        !ppocr_ort_ok(recognizer,
                      recognizer->ort->SessionGetOutputName(
                          recognizer->session, 0u, allocator,
                          &ort_output_name),
                      "read model output name")) {
        result = PPOCR_ERR_MODEL;
        goto fail;
    }
    recognizer->input_name = ppocr_copy_cstring(ort_input_name);
    recognizer->output_name = ppocr_copy_cstring(ort_output_name);
    allocator->Free(allocator, ort_input_name);
    allocator->Free(allocator, ort_output_name);
    ort_input_name = 0;
    ort_output_name = 0;
    if (!recognizer->input_name || !recognizer->output_name) {
        ppocr_error_set(recognizer, "copy model I/O names", "out of memory");
        result = PPOCR_ERR_NOMEM;
        goto fail;
    }
    result = ppocr_validate_io_type(recognizer, 1);
    if (result != PPOCR_OK) goto fail;
    result = ppocr_validate_io_type(recognizer, 0);
    if (result != PPOCR_OK) goto fail;
    if (!ppocr_ort_ok(recognizer,
                      recognizer->ort->CreateCpuMemoryInfo(
                          OrtArenaAllocator, OrtMemTypeDefault,
                          &recognizer->memory_info),
                      "create CPU tensor memory info")) {
        result = PPOCR_ERR_RUNTIME;
        goto fail;
    }
    return PPOCR_OK;

fail:
    if (ort_input_name && allocator) allocator->Free(allocator, ort_input_name);
    if (ort_output_name && allocator) allocator->Free(allocator, ort_output_name);
    if (options && recognizer->ort)
        recognizer->ort->ReleaseSessionOptions(options);
    ppocr_release_runtime(recognizer);
    return result;
}

static void ppocr_resize_map(PpocrResizeMap *map, OcrU32 destination,
                             OcrU32 source_size, OcrU32 destination_size) {
    double coordinate;
    OcrU32 low;
    if (source_size <= 1u) {
        map->low = 0;
        map->high = 0;
        map->weight = 0.0f;
        return;
    }
    coordinate = (((double)destination + 0.5) * (double)source_size /
                  (double)destination_size) - 0.5;
    if (coordinate <= 0.0) {
        map->low = 0;
        map->high = 0;
        map->weight = 0.0f;
    } else if (coordinate >= (double)(source_size - 1u)) {
        map->low = source_size - 1u;
        map->high = source_size - 1u;
        map->weight = 0.0f;
    } else {
        low = (OcrU32)coordinate;
        map->low = low;
        map->high = low + 1u;
        map->weight = (float)(coordinate - (double)low);
    }
}

static float ppocr_read_channel(const PpocrImage *image, OcrU32 x, OcrU32 y,
                                OcrU32 channel) {
    OcrSize offset = (OcrSize)y * image->stride;
    if (image->format == PPOCR_PIXELS_GRAY8)
        return (float)image->pixels[offset + x];
    return (float)image->pixels[offset + (OcrSize)x * 3u + channel];
}

static int ppocr_prepare_input(PpocrRecognizer *recognizer,
                               const PpocrImage *image,
                               OcrU32 *out_input_width) {
    OcrU64 scaled_numerator;
    OcrU64 floor_width;
    OcrU64 ceil_width;
    OcrU32 input_width;
    OcrU32 resized_width;
    OcrSize element_count;
    OcrSize byte_count;
    OcrSize row_bytes;
    OcrSize last_row_offset;
    PpocrResizeMap y_maps[PPOCR_INPUT_HEIGHT];
    OcrU32 x;
    OcrU32 y;
    OcrU32 channel;

    if (!recognizer || !image || !image->pixels || !out_input_width ||
        !image->width || !image->height ||
        (image->format != PPOCR_PIXELS_GRAY8 &&
         image->format != PPOCR_PIXELS_BGR8))
        return PPOCR_ERR_INVALID;
    if ((OcrSize)image->width > ~(OcrSize)0 /
                                      (OcrSize)image->format)
        return PPOCR_ERR_LIMIT;
    row_bytes = (OcrSize)image->width * (OcrSize)image->format;
    if ((OcrSize)image->stride < row_bytes) return PPOCR_ERR_INVALID;
    if ((OcrSize)(image->height - 1u) >
        (~(OcrSize)0 - row_bytes) / (OcrSize)image->stride)
        return PPOCR_ERR_LIMIT;
    last_row_offset = (OcrSize)(image->height - 1u) * image->stride;
    if (last_row_offset + row_bytes < last_row_offset)
        return PPOCR_ERR_LIMIT;

    scaled_numerator = (OcrU64)PPOCR_INPUT_HEIGHT * image->width;
    floor_width = scaled_numerator / image->height;
    ceil_width = (scaled_numerator + image->height - 1u) / image->height;
    if (floor_width < PPOCR_BASE_INPUT_WIDTH)
        floor_width = PPOCR_BASE_INPUT_WIDTH;
    if (floor_width > PPOCR_MAX_INPUT_WIDTH)
        floor_width = PPOCR_MAX_INPUT_WIDTH;
    input_width = (OcrU32)floor_width;
    if (ceil_width > input_width) ceil_width = input_width;
    if (!ceil_width) ceil_width = 1u;
    resized_width = (OcrU32)ceil_width;

    element_count = (OcrSize)3u * PPOCR_INPUT_HEIGHT * input_width;
    if (element_count > ~(OcrSize)0 / sizeof(float))
        return PPOCR_ERR_LIMIT;
    byte_count = element_count * sizeof(float);
    if (recognizer->input_capacity < element_count) {
        float *new_scratch = (float *)realloc(recognizer->input_scratch,
                                               byte_count);
        if (!new_scratch) return PPOCR_ERR_NOMEM;
        recognizer->input_scratch = new_scratch;
        recognizer->input_capacity = element_count;
    }
    if (recognizer->x_map_capacity < resized_width) {
        PpocrResizeMap *new_maps;
        new_maps = (PpocrResizeMap *)realloc(
            recognizer->x_maps,
            (OcrSize)resized_width * sizeof(PpocrResizeMap));
        if (!new_maps) return PPOCR_ERR_NOMEM;
        recognizer->x_maps = new_maps;
        recognizer->x_map_capacity = resized_width;
    }
    memset(recognizer->input_scratch, 0, byte_count);
    for (x = 0; x < resized_width; ++x)
        ppocr_resize_map(recognizer->x_maps + x, x, image->width,
                         resized_width);
    for (y = 0; y < PPOCR_INPUT_HEIGHT; ++y)
        ppocr_resize_map(y_maps + y, y, image->height,
                         PPOCR_INPUT_HEIGHT);

    for (y = 0; y < PPOCR_INPUT_HEIGHT; ++y) {
        const PpocrResizeMap *ym = y_maps + y;
        float wy = ym->weight;
        for (x = 0; x < resized_width; ++x) {
            const PpocrResizeMap *xm = recognizer->x_maps + x;
            float wx = xm->weight;
            for (channel = 0; channel < 3u; ++channel) {
                float p00 = ppocr_read_channel(image, xm->low, ym->low,
                                               channel);
                float p01 = ppocr_read_channel(image, xm->high, ym->low,
                                               channel);
                float p10 = ppocr_read_channel(image, xm->low, ym->high,
                                               channel);
                float p11 = ppocr_read_channel(image, xm->high, ym->high,
                                               channel);
                float top = p00 + (p01 - p00) * wx;
                float bottom = p10 + (p11 - p10) * wx;
                float pixel = top + (bottom - top) * wy;
                OcrU32 resized_u8 = (OcrU32)(pixel + 0.5f);
                OcrSize destination =
                    ((OcrSize)channel * PPOCR_INPUT_HEIGHT + y) *
                        input_width +
                    x;
                if (resized_u8 > 255u) resized_u8 = 255u;
                recognizer->input_scratch[destination] =
                    (float)resized_u8 * (1.0f / 127.5f) - 1.0f;
            }
        }
    }
    *out_input_width = input_width;
    return PPOCR_OK;
}

static int ppocr_decode(PpocrRecognizer *recognizer, const float *probabilities,
                        OcrU32 timesteps, PpocrResult *result) {
    char *text;
    OcrU32 text_length = 0;
    OcrU32 previous = 0xffffffffu;
    OcrU32 retained = 0;
    double confidence_sum = 0.0;
    OcrU32 timestep;
    if (!recognizer || !probabilities || !timesteps || !result ||
        recognizer->token_count != PPOCR_DICTIONARY_ENTRIES + 1u)
        return PPOCR_ERR_INVALID;
    text = (char *)malloc((OcrSize)PPOCR_MAX_TEXT_BYTES + 1u);
    if (!text) return PPOCR_ERR_NOMEM;
    for (timestep = 0; timestep < timesteps; ++timestep) {
        const float *row = probabilities +
                           (OcrSize)timestep * PPOCR_CLASS_COUNT;
        OcrU32 class_id = 0;
        OcrU32 candidate;
        float maximum = row[0];
        double probability_sum = 0.0;
        if (!ppocr_finite_probability(maximum)) {
            ppocr_error_set(recognizer,
                            "model output is not finite Softmax probability",
                            0);
            free(text);
            return PPOCR_ERR_SHAPE;
        }
        probability_sum += maximum;
        for (candidate = 1; candidate < PPOCR_CLASS_COUNT; ++candidate) {
            float value = row[candidate];
            if (!ppocr_finite_probability(value)) {
                ppocr_error_set(
                    recognizer,
                    "model output is not finite Softmax probability", 0);
                free(text);
                return PPOCR_ERR_SHAPE;
            }
            probability_sum += value;
            if (value > maximum) {
                maximum = value;
                class_id = candidate;
            }
        }
        if (probability_sum < 0.98 || probability_sum > 1.02) {
            ppocr_error_set(recognizer,
                            "model output is not normalized Softmax", 0);
            free(text);
            return PPOCR_ERR_SHAPE;
        }
        if (class_id != previous && class_id != 0u) {
            OcrU32 token_index = class_id - 1u;
            const PpocrToken *token;
            if (token_index >= recognizer->token_count) {
                ppocr_error_set(recognizer,
                                "model output class exceeds dictionary", 0);
                free(text);
                return PPOCR_ERR_SHAPE;
            }
            token = recognizer->tokens + token_index;
            if (token->length > PPOCR_MAX_TEXT_BYTES - text_length) {
                ppocr_error_set(recognizer,
                                "decoded text exceeds safety limit", 0);
                free(text);
                return PPOCR_ERR_LIMIT;
            }
            memcpy(text + text_length,
                   recognizer->token_bytes + token->offset,
                   token->length);
            text_length += token->length;
            confidence_sum += maximum;
            ++retained;
        }
        previous = class_id;
    }
    text[text_length] = 0;
    result->text = text;
    result->text_bytes = text_length;
    result->confidence = retained
                             ? (float)(confidence_sum / (double)retained)
                             : 0.0f;
    return PPOCR_OK;
}

int ppocr_recognizer_init(PpocrRecognizer **out_recognizer,
                          const void *model_bytes, OcrSize model_size,
                          const void *dictionary_bytes,
                          OcrSize dictionary_size) {
    PpocrRecognizer *recognizer;
    int result;
    if (!out_recognizer) return PPOCR_ERR_INVALID;
    *out_recognizer = 0;
    if (!model_bytes || !model_size ||
        model_size > (OcrSize)PPOCR_MAX_MODEL_BYTES ||
        !dictionary_bytes || !dictionary_size)
        return PPOCR_ERR_INVALID;
    recognizer = (PpocrRecognizer *)calloc(1u, sizeof(PpocrRecognizer));
    if (!recognizer) return PPOCR_ERR_NOMEM;
    recognizer->model_bytes = (OcrU8 *)malloc(model_size);
    if (!recognizer->model_bytes) {
        ppocr_recognizer_destroy(recognizer);
        return PPOCR_ERR_NOMEM;
    }
    memcpy(recognizer->model_bytes, model_bytes, model_size);
    recognizer->model_size = model_size;
    result = ppocr_parse_dictionary(
        recognizer, (const OcrU8 *)dictionary_bytes, dictionary_size);
    if (result != PPOCR_OK) {
        ppocr_recognizer_destroy(recognizer);
        return result;
    }
    *out_recognizer = recognizer;
    return PPOCR_OK;
}

void ppocr_result_release(PpocrResult *result) {
    if (!result) return;
    if (result->text) free(result->text);
    memset(result, 0, sizeof(*result));
}

int ppocr_recognizer_recognize(PpocrRecognizer *recognizer,
                               const PpocrImage *image,
                               PpocrResult *out_result) {
    OrtValue *input_tensor = 0;
    OrtValue *output_tensor = 0;
    OrtTensorTypeAndShapeInfo *output_info = 0;
    const OrtValue *input_values[1];
    const char *input_names[1];
    const char *output_names[1];
    int64_t input_shape[4];
    int64_t output_shape[3];
    size_t output_rank = 0;
    size_t output_elements = 0;
    ONNXTensorElementDataType output_type;
    float *output_data = 0;
    void *output_buffer = 0;
    OcrU32 input_width = 0;
    OcrU64 total_start;
    OcrU64 phase_start;
    OcrU64 phase_end;
    OcrSize input_elements;
    int is_tensor = 0;
    int result = PPOCR_ERR_RUNTIME;

    if (!recognizer || !image || !out_result) return PPOCR_ERR_INVALID;
    memset(out_result, 0, sizeof(*out_result));
    if (ppocr_try_lock(&recognizer->busy)) {
        return PPOCR_ERR_BUSY;
    }
    ppocr_error_clear(recognizer);
    total_start = ppocr_now_us();

    if (!recognizer->session) {
        phase_start = ppocr_now_us();
        result = ppocr_create_session(recognizer);
        phase_end = ppocr_now_us();
        out_result->timing.session_setup_us =
            ppocr_elapsed_us(phase_start, phase_end);
        if (result != PPOCR_OK) goto done;
    }

    phase_start = ppocr_now_us();
    result = ppocr_prepare_input(recognizer, image, &input_width);
    phase_end = ppocr_now_us();
    out_result->timing.preprocess_us =
        ppocr_elapsed_us(phase_start, phase_end);
    out_result->timing.input_width = input_width;
    if (result != PPOCR_OK) {
        ppocr_error_set(recognizer, "preprocess recognition crop",
                        result == PPOCR_ERR_NOMEM ? "out of memory" :
                        result == PPOCR_ERR_LIMIT ? "size limit exceeded" :
                                                    "invalid image");
        goto done;
    }
    result = PPOCR_ERR_RUNTIME;

    input_elements = (OcrSize)3u * PPOCR_INPUT_HEIGHT * input_width;
    input_shape[0] = 1;
    input_shape[1] = 3;
    input_shape[2] = PPOCR_INPUT_HEIGHT;
    input_shape[3] = input_width;
    if (!ppocr_ort_ok(recognizer,
                      recognizer->ort->CreateTensorWithDataAsOrtValue(
                          recognizer->memory_info,
                          recognizer->input_scratch,
                          (size_t)(input_elements * sizeof(float)),
                          input_shape, 4u,
                          ONNX_TENSOR_ELEMENT_DATA_TYPE_FLOAT,
                          &input_tensor),
                      "create model input tensor"))
        goto done;
    if (!ppocr_ort_ok(recognizer,
                      recognizer->ort->IsTensor(input_tensor, &is_tensor),
                      "validate model input tensor") || !is_tensor) {
        if (!is_tensor)
            ppocr_error_set(recognizer, "model input value is not a tensor", 0);
        result = PPOCR_ERR_SHAPE;
        goto done;
    }

    input_values[0] = input_tensor;
    input_names[0] = recognizer->input_name;
    output_names[0] = recognizer->output_name;
    phase_start = ppocr_now_us();
    if (!ppocr_ort_ok(recognizer,
                      recognizer->ort->Run(recognizer->session, 0,
                                           input_names, input_values, 1u,
                                           output_names, 1u,
                                           &output_tensor),
                      "run PP-OCRv5 inference")) {
        phase_end = ppocr_now_us();
        out_result->timing.inference_us =
            ppocr_elapsed_us(phase_start, phase_end);
        goto done;
    }
    phase_end = ppocr_now_us();
    out_result->timing.inference_us = ppocr_elapsed_us(phase_start, phase_end);

    if (!output_tensor ||
        !ppocr_ort_ok(recognizer,
                      recognizer->ort->IsTensor(output_tensor, &is_tensor),
                      "validate model output tensor") || !is_tensor) {
        if (!output_tensor || !is_tensor)
            ppocr_error_set(recognizer, "model output is not a tensor", 0);
        result = PPOCR_ERR_SHAPE;
        goto done;
    }
    if (!ppocr_ort_ok(recognizer,
                      recognizer->ort->GetTensorTypeAndShape(
                          output_tensor, &output_info),
                      "read model output shape") ||
        !ppocr_ort_ok(recognizer,
                      recognizer->ort->GetTensorElementType(
                          output_info, &output_type),
                      "read model output type") ||
        !ppocr_ort_ok(recognizer,
                      recognizer->ort->GetDimensionsCount(
                          output_info, &output_rank),
                      "read model output rank"))
        goto done;
    if (output_type != ONNX_TENSOR_ELEMENT_DATA_TYPE_FLOAT ||
        output_rank != 3u) {
        ppocr_error_set(recognizer,
                        "model output must be a rank-3 float32 tensor", 0);
        result = PPOCR_ERR_SHAPE;
        goto done;
    }
    if (!ppocr_ort_ok(recognizer,
                      recognizer->ort->GetDimensions(
                          output_info, output_shape, 3u),
                      "read concrete model output dimensions"))
        goto done;
    if (output_shape[0] != 1 || output_shape[1] <= 0 ||
        output_shape[1] > (int64_t)input_width ||
        output_shape[2] != (int64_t)PPOCR_CLASS_COUNT) {
        ppocr_error_set(recognizer,
                        "model output is not [1,T,18385]", 0);
        result = PPOCR_ERR_SHAPE;
        goto done;
    }
    if (!ppocr_ort_ok(recognizer,
                      recognizer->ort->GetTensorShapeElementCount(
                          output_info, &output_elements),
                      "read model output element count"))
        goto done;
    if (output_elements !=
        (size_t)output_shape[1] * (size_t)PPOCR_CLASS_COUNT) {
        ppocr_error_set(recognizer,
                        "model output element count is inconsistent", 0);
        result = PPOCR_ERR_SHAPE;
        goto done;
    }
    out_result->timing.output_timesteps = (OcrU32)output_shape[1];
    if (!ppocr_ort_ok(recognizer,
                      recognizer->ort->GetTensorMutableData(
                          output_tensor, &output_buffer),
                      "read model output data") || !output_buffer) {
        if (!output_buffer)
            ppocr_error_set(recognizer, "model returned no output data", 0);
        goto done;
    }
    output_data = (float *)output_buffer;

    phase_start = ppocr_now_us();
    result = ppocr_decode(recognizer, output_data,
                          (OcrU32)output_shape[1], out_result);
    phase_end = ppocr_now_us();
    out_result->timing.decode_us = ppocr_elapsed_us(phase_start, phase_end);

done:
    if (output_info)
        recognizer->ort->ReleaseTensorTypeAndShapeInfo(output_info);
    if (output_tensor) recognizer->ort->ReleaseValue(output_tensor);
    if (input_tensor) recognizer->ort->ReleaseValue(input_tensor);
    phase_end = ppocr_now_us();
    out_result->timing.total_us = ppocr_elapsed_us(total_start, phase_end);
    if (result != PPOCR_OK && out_result->text)
        ppocr_result_release(out_result);
    ppocr_unlock(&recognizer->busy);
    return result;
}

void ppocr_recognizer_destroy(PpocrRecognizer *recognizer) {
    if (!recognizer) return;
    ppocr_release_runtime(recognizer);
    if (recognizer->model_bytes) free(recognizer->model_bytes);
    if (recognizer->tokens) free(recognizer->tokens);
    if (recognizer->token_bytes) free(recognizer->token_bytes);
    if (recognizer->input_scratch) free(recognizer->input_scratch);
    if (recognizer->x_maps) free(recognizer->x_maps);
    memset(recognizer, 0, sizeof(*recognizer));
    free(recognizer);
}

const char *ppocr_recognizer_last_error(const PpocrRecognizer *recognizer) {
    static const char empty[] = "";
    return recognizer ? recognizer->last_error : empty;
}

int ppocr_recognizer_session_ready(const PpocrRecognizer *recognizer) {
    return recognizer && recognizer->session != 0;
}
