#include <stdio.h>

/* Include the implementation so this host test can exercise its pure helpers. */
#include "src/ppocr_recognizer.c"

static int failures;

static void check(int condition, const char *message) {
    if (!condition) {
        fprintf(stderr, "FAIL: %s\n", message);
        ++failures;
    }
}

static float abs_float(float value) {
    return value < 0.0f ? -value : value;
}

static int bytes_equal(const char *left, const OcrU8 *right, OcrU32 count) {
    OcrU32 i;
    for (i = 0; i < count; ++i)
        if ((OcrU8)left[i] != right[i]) return 0;
    return 1;
}

static OcrU8 *load_file(const char *path, OcrSize *out_size) {
    FILE *file = fopen(path, "rb");
    long length;
    OcrU8 *bytes;
    if (!file) return 0;
    if (fseek(file, 0, SEEK_END) != 0 || (length = ftell(file)) <= 0 ||
        fseek(file, 0, SEEK_SET) != 0) {
        fclose(file);
        return 0;
    }
    bytes = (OcrU8 *)malloc((OcrSize)length);
    if (!bytes || fread(bytes, 1u, (OcrSize)length, file) !=
                      (OcrSize)length) {
        if (bytes) free(bytes);
        fclose(file);
        return 0;
    }
    fclose(file);
    *out_size = (OcrSize)length;
    return bytes;
}

static OcrU8 *make_dictionary(OcrSize *out_size) {
    const OcrU8 first[] = {0xe3u, 0x80u, 0x80u}; /* U+3000 */
    const OcrU8 second[] = {0xe4u, 0xb8u, 0xadu}; /* U+4E2D */
    OcrSize size = sizeof(first) + 1u + sizeof(second) + 1u + 2u +
                   (OcrSize)(PPOCR_DICTIONARY_ENTRIES - 3u) * 2u;
    OcrU8 *dictionary = (OcrU8 *)malloc(size);
    OcrSize position = 0;
    OcrU32 i;
    if (!dictionary) return 0;
    memcpy(dictionary + position, first, sizeof(first));
    position += sizeof(first);
    dictionary[position++] = '\n';
    memcpy(dictionary + position, second, sizeof(second));
    position += sizeof(second);
    dictionary[position++] = '\n';
    dictionary[position++] = 'B';
    dictionary[position++] = '\n';
    for (i = 3u; i < PPOCR_DICTIONARY_ENTRIES; ++i) {
        dictionary[position++] = 'x';
        dictionary[position++] = '\n';
    }
    *out_size = position;
    return dictionary;
}

static void test_dictionary_and_decode(void) {
    PpocrRecognizer *recognizer =
        (PpocrRecognizer *)calloc(1u, sizeof(PpocrRecognizer));
    PpocrResult result;
    OcrSize dictionary_size = 0;
    OcrU8 *dictionary = make_dictionary(&dictionary_size);
    float *scores;
    const OcrU32 timesteps = 7u;
    const OcrU8 expected[] = {
        0xe4u, 0xb8u, 0xadu, 0xe4u, 0xb8u, 0xadu, ' ', 'B'};
    OcrU32 t;

    check(recognizer != 0 && dictionary != 0, "test allocations");
    if (!recognizer || !dictionary) goto done;
    check(ppocr_parse_dictionary(recognizer, dictionary, dictionary_size) ==
              PPOCR_OK,
          "parse exactly 18,383 UTF-8 dictionary lines");
    check(recognizer->token_count == PPOCR_DICTIONARY_ENTRIES + 1u,
          "append exactly one ASCII-space token");
    check(recognizer->tokens[PPOCR_DICTIONARY_ENTRIES].length == 1u &&
              recognizer->token_bytes[
                  recognizer->tokens[PPOCR_DICTIONARY_ENTRIES].offset] == ' ',
          "ASCII space is final dictionary token");

    scores = (float *)calloc((OcrSize)timesteps * PPOCR_CLASS_COUNT,
                             sizeof(float));
    check(scores != 0, "score allocation");
    if (!scores) goto done;
    for (t = 0; t < timesteps; ++t)
        scores[(OcrSize)t * PPOCR_CLASS_COUNT] = 1.0f;
    scores[(OcrSize)1u * PPOCR_CLASS_COUNT] = 0.2f;
    scores[(OcrSize)1u * PPOCR_CLASS_COUNT + 2u] = 0.8f;
    scores[(OcrSize)2u * PPOCR_CLASS_COUNT] = 0.1f;
    scores[(OcrSize)2u * PPOCR_CLASS_COUNT + 2u] = 0.9f;
    scores[(OcrSize)4u * PPOCR_CLASS_COUNT] = 0.4f;
    scores[(OcrSize)4u * PPOCR_CLASS_COUNT + 2u] = 0.6f;
    scores[(OcrSize)5u * PPOCR_CLASS_COUNT] = 0.25f;
    scores[(OcrSize)5u * PPOCR_CLASS_COUNT + 18384u] = 0.75f;
    scores[(OcrSize)6u * PPOCR_CLASS_COUNT] = 0.3f;
    scores[(OcrSize)6u * PPOCR_CLASS_COUNT + 3u] = 0.7f;

    memset(&result, 0, sizeof(result));
    check(ppocr_decode(recognizer, scores, timesteps, &result) == PPOCR_OK,
          "decode normalized Softmax output");
    check(result.text_bytes == sizeof(expected) &&
              bytes_equal(result.text, expected, sizeof(expected)),
          "CTC blank/raw-dedup/UTF-8/ASCII-space mapping");
    check(abs_float(result.confidence - 0.7125f) < 0.00001f,
          "mean confidence uses retained timesteps only");
    ppocr_result_release(&result);
    free(scores);
done:
    if (dictionary) free(dictionary);
    if (recognizer) ppocr_recognizer_destroy(recognizer);
}

static void test_preprocess(void) {
    PpocrRecognizer *recognizer =
        (PpocrRecognizer *)calloc(1u, sizeof(PpocrRecognizer));
    const OcrU8 pixels[6] = {0u, 128u, 255u, 255u, 0u, 128u};
    PpocrImage image;
    OcrU32 input_width = 0;
    OcrSize green_plane;
    OcrSize red_plane;
    if (!recognizer) {
        check(0, "preprocess recognizer allocation");
        return;
    }
    image.pixels = pixels;
    image.width = 2u;
    image.height = 1u;
    image.stride = 6u;
    image.format = PPOCR_PIXELS_BGR8;
    check(ppocr_prepare_input(recognizer, &image, &input_width) == PPOCR_OK,
          "preprocess BGR crop");
    check(input_width == PPOCR_BASE_INPUT_WIDTH,
          "short crop uses base width 320");
    green_plane = (OcrSize)PPOCR_INPUT_HEIGHT * input_width;
    red_plane = green_plane * 2u;
    check(abs_float(recognizer->input_scratch[0] - -1.0f) < 0.00001f,
          "B channel normalized at first pixel");
    check(abs_float(recognizer->input_scratch[green_plane] -
                    (128.0f / 127.5f - 1.0f)) < 0.00001f,
          "G channel retained in BGR CHW order");
    check(abs_float(recognizer->input_scratch[red_plane] - 1.0f) <
              0.00001f,
          "R channel retained in BGR CHW order");
    check(abs_float(recognizer->input_scratch[95u] - 1.0f) < 0.00001f,
          "bilinear resize reaches final source B value");
    check(recognizer->input_scratch[96u] == 0.0f &&
              recognizer->input_scratch[green_plane + 96u] == 0.0f &&
              recognizer->input_scratch[red_plane + 96u] == 0.0f,
          "right padding is normalized zero");
    ppocr_recognizer_destroy(recognizer);
}

static void test_dynamic_width_and_gray(void) {
    PpocrRecognizer *recognizer =
        (PpocrRecognizer *)calloc(1u, sizeof(PpocrRecognizer));
    OcrU8 *pixels = (OcrU8 *)malloc(1010u);
    PpocrImage image;
    OcrU32 input_width = 0;
    OcrU32 i;
    if (!recognizer || !pixels) {
        check(0, "dynamic-width allocations");
        if (recognizer) ppocr_recognizer_destroy(recognizer);
        if (pixels) free(pixels);
        return;
    }
    for (i = 0; i < 1010u; ++i) pixels[i] = 255u;
    image.pixels = pixels;
    image.width = 101u;
    image.height = 10u;
    image.stride = 101u;
    image.format = PPOCR_PIXELS_GRAY8;
    check(ppocr_prepare_input(recognizer, &image, &input_width) == PPOCR_OK,
          "preprocess grayscale crop");
    check(input_width == 484u,
          "dynamic width follows floor(48*aspect), with resized width capped");
    check(abs_float(recognizer->input_scratch[0] - 1.0f) < 0.00001f &&
              abs_float(recognizer->input_scratch[
                            (OcrSize)PPOCR_INPUT_HEIGHT * input_width] -
                        1.0f) < 0.00001f &&
              abs_float(recognizer->input_scratch[
                            (OcrSize)2u * PPOCR_INPUT_HEIGHT * input_width] -
                        1.0f) < 0.00001f,
          "grayscale expands identically into B, G and R planes");
    free(pixels);
    ppocr_recognizer_destroy(recognizer);
}

static void test_packaged_assets_and_lazy_init(void) {
    const char *model_path =
        "assets/ocr/ppocrv5_mobile_rec/inference.onnx";
    const char *dictionary_path =
        "assets/ocr/ppocrv5_mobile_rec/ppocrv5_dict.txt";
    OcrSize model_size = 0;
    OcrSize dictionary_size = 0;
    OcrU8 *model = load_file(model_path, &model_size);
    OcrU8 *dictionary = load_file(dictionary_path, &dictionary_size);
    PpocrRecognizer *recognizer = 0;
    OcrU8 retained_first_byte = 0;
    check(model != 0 && dictionary != 0, "load packaged OCR assets");
    if (!model || !dictionary) goto done;
    retained_first_byte = model[0];
    check(ppocr_recognizer_init(&recognizer, model, model_size,
                                dictionary, dictionary_size) == PPOCR_OK,
          "initialize from packaged model/dictionary bytes");
    check(recognizer != 0 && !ppocr_recognizer_session_ready(recognizer),
          "initialization remains lazy and does not load ORT");
    if (recognizer) {
        model[0] ^= 0xffu;
        check(recognizer->model_bytes != model &&
                  recognizer->model_bytes[0] == retained_first_byte,
              "recognizer owns a copy of manager-supplied model bytes");
        check(recognizer->token_count == PPOCR_DICTIONARY_ENTRIES + 1u,
              "packaged official dictionary has required class mapping");
    }
done:
    if (recognizer) ppocr_recognizer_destroy(recognizer);
    if (model) free(model);
    if (dictionary) free(dictionary);
}

int main(void) {
    test_dictionary_and_decode();
    test_preprocess();
    test_dynamic_width_and_gray();
    test_packaged_assets_and_lazy_init();
    if (failures) {
        fprintf(stderr, "%d PP-OCR recognizer test(s) failed\n", failures);
        return 1;
    }
    printf("PP-OCR recognizer preprocessing/decoder tests passed\n");
    return 0;
}
