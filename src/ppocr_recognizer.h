#ifndef VAST_PPOCR_RECOGNIZER_H
#define VAST_PPOCR_RECOGNIZER_H

#include "ocr_core.h"

/*
 * Offline PP-OCRv5 recognition wrapper.
 *
 * The recognizer copies the model and dictionary bytes supplied at init, then
 * creates its single ONNX Runtime session lazily on the first recognition.
 * It never opens assets or files and does not contain a network/telemetry path.
 *
 * One PpocrRecognizer must not be used concurrently.  A concurrent call fails
 * with PPOCR_ERR_BUSY instead of racing session creation or scratch buffers.
 */

#define PPOCR_DICTIONARY_ENTRIES 18383u
#define PPOCR_CLASS_COUNT 18385u
#define PPOCR_INPUT_HEIGHT 48u
#define PPOCR_BASE_INPUT_WIDTH 320u
#define PPOCR_MAX_INPUT_WIDTH 3200u
#define PPOCR_MAX_TEXT_BYTES OCR_MAX_TEXT_BYTES

enum {
    PPOCR_OK = 0,
    PPOCR_ERR_INVALID = -1,
    PPOCR_ERR_NOMEM = -2,
    PPOCR_ERR_LIMIT = -3,
    PPOCR_ERR_RUNTIME = -10,
    PPOCR_ERR_DICTIONARY = -11,
    PPOCR_ERR_MODEL = -12,
    PPOCR_ERR_SHAPE = -13,
    PPOCR_ERR_BUSY = -14
};

typedef enum {
    PPOCR_PIXELS_GRAY8 = 1,
    PPOCR_PIXELS_BGR8 = 3
} PpocrPixelFormat;

typedef struct {
    const OcrU8 *pixels;
    OcrU32 width;
    OcrU32 height;
    OcrU32 stride;
    PpocrPixelFormat format;
} PpocrImage;

typedef struct {
    OcrU64 session_setup_us; /* Nonzero only when this call creates the session. */
    OcrU64 preprocess_us;
    OcrU64 inference_us;
    OcrU64 decode_us;
    OcrU64 total_us;
    OcrU32 input_width;
    OcrU32 output_timesteps;
} PpocrTiming;

typedef struct {
    char *text;              /* Owned UTF-8, NUL terminated. */
    OcrU32 text_bytes;       /* Excludes the terminating NUL. */
    float confidence;        /* Mean probability of retained CTC symbols. */
    PpocrTiming timing;
} PpocrResult;

typedef struct PpocrRecognizer PpocrRecognizer;

/* Copies model_bytes and dictionary_bytes.  No ORT library/session is loaded. */
int ppocr_recognizer_init(PpocrRecognizer **out_recognizer,
                          const void *model_bytes, OcrSize model_size,
                          const void *dictionary_bytes,
                          OcrSize dictionary_size);

/*
 * Recognizes one grayscale or interleaved BGR crop.  The returned text belongs
 * to out_result and must be released with ppocr_result_release().  out_result
 * is reset before use and is safe to release after either success or failure.
 */
int ppocr_recognizer_recognize(PpocrRecognizer *recognizer,
                               const PpocrImage *image,
                               PpocrResult *out_result);

void ppocr_result_release(PpocrResult *result);
void ppocr_recognizer_destroy(PpocrRecognizer *recognizer);

/* Borrowed diagnostic string, valid until the next call or destruction. */
const char *ppocr_recognizer_last_error(const PpocrRecognizer *recognizer);
int ppocr_recognizer_session_ready(const PpocrRecognizer *recognizer);

#endif
