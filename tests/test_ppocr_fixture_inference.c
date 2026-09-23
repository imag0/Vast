#include "ppocr_recognizer.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#endif

typedef struct {
    const char *name;
    const char *path;
    const char *required_ascii[3];
    const char *required_utf8;
} FixtureCase;

static const FixtureCase fixture_cases[] = {
    {"zh_engine", "tests/fixtures/ocr/zh_engine.pgm", {0, 0, 0},
     "发动机"},
    {"zh_school", "tests/fixtures/ocr/zh_school.pgm", {0, 0, 0},
     "学校"},
    {"en_terms", "tests/fixtures/ocr/en_terms.pgm",
     {"engine", "pressure", "navigation"}, 0},
    {"mixed", "tests/fixtures/ocr/mixed.pgm", {"engine", 0, 0},
     "发动机"},
};

static unsigned char *read_file(const char *path, size_t *out_size) {
    FILE *file = fopen(path, "rb");
    long length;
    unsigned char *bytes;
    if (!file) return 0;
    if (fseek(file, 0, SEEK_END) != 0 || (length = ftell(file)) <= 0 ||
        fseek(file, 0, SEEK_SET) != 0) {
        fclose(file);
        return 0;
    }
    bytes = (unsigned char *)malloc((size_t)length);
    if (!bytes || fread(bytes, 1u, (size_t)length, file) != (size_t)length) {
        free(bytes);
        fclose(file);
        return 0;
    }
    fclose(file);
    *out_size = (size_t)length;
    return bytes;
}

static int pgm_token(FILE *file, char *token, size_t capacity) {
    int ch;
    size_t used = 0;
    do {
        ch = fgetc(file);
        if (ch == '#') {
            do ch = fgetc(file); while (ch != '\n' && ch != EOF);
        }
    } while (ch != EOF && ch <= ' ');
    if (ch == EOF) return 0;
    while (ch > ' ') {
        if (used + 1u >= capacity) return 0;
        token[used++] = (char)ch;
        ch = fgetc(file);
    }
    token[used] = 0;
    return used != 0;
}

static unsigned char *load_pgm(const char *path, unsigned int *out_width,
                               unsigned int *out_height) {
    FILE *file = fopen(path, "rb");
    char token[32];
    unsigned long width;
    unsigned long height;
    unsigned long maximum;
    size_t pixels_size;
    unsigned char *pixels;
    if (!file) return 0;
    if (!pgm_token(file, token, sizeof(token)) || strcmp(token, "P5") != 0 ||
        !pgm_token(file, token, sizeof(token))) {
        fclose(file);
        return 0;
    }
    width = strtoul(token, 0, 10);
    if (!pgm_token(file, token, sizeof(token))) {
        fclose(file);
        return 0;
    }
    height = strtoul(token, 0, 10);
    if (!pgm_token(file, token, sizeof(token))) {
        fclose(file);
        return 0;
    }
    maximum = strtoul(token, 0, 10);
    if (!width || !height || width > 100000u || height > 100000u ||
        maximum != 255u || width > (size_t)-1 / height) {
        fclose(file);
        return 0;
    }
    pixels_size = (size_t)width * (size_t)height;
    pixels = (unsigned char *)malloc(pixels_size);
    if (!pixels || fread(pixels, 1u, pixels_size, file) != pixels_size) {
        free(pixels);
        fclose(file);
        return 0;
    }
    fclose(file);
    *out_width = (unsigned int)width;
    *out_height = (unsigned int)height;
    return pixels;
}

static int ascii_lower(int ch) {
    return ch >= 'A' && ch <= 'Z' ? ch + ('a' - 'A') : ch;
}

static int contains_ascii_casefold(const char *text, const char *needle) {
    size_t i;
    size_t j;
    if (!text || !needle || !needle[0]) return 0;
    for (i = 0; text[i]; ++i) {
        for (j = 0; needle[j] && text[i + j] &&
                    ascii_lower((unsigned char)text[i + j]) ==
                        ascii_lower((unsigned char)needle[j]);
             ++j) {
        }
        if (!needle[j]) return 1;
    }
    return 0;
}

int main(void) {
    const char *model_path =
        "assets/ocr/ppocrv5_mobile_rec/inference.onnx";
    const char *dictionary_path =
        "assets/ocr/ppocrv5_mobile_rec/ppocrv5_dict.txt";
    unsigned char *model = 0;
    unsigned char *dictionary = 0;
    size_t model_size = 0;
    size_t dictionary_size = 0;
    PpocrRecognizer *recognizer = 0;
    size_t case_index;
    int failures = 0;
    int status;
#ifdef _WIN32
    SetConsoleOutputCP(CP_UTF8);
#endif
    model = read_file(model_path, &model_size);
    dictionary = read_file(dictionary_path, &dictionary_size);
    if (!model || !dictionary) {
        fprintf(stderr, "FAIL could not load model/dictionary assets\n");
        failures = 1;
        goto done;
    }
    status = ppocr_recognizer_init(&recognizer, model, model_size,
                                    dictionary, dictionary_size);
    free(model);
    model = 0;
    free(dictionary);
    dictionary = 0;
    if (status != PPOCR_OK) {
        fprintf(stderr, "FAIL ppocr_recognizer_init status=%d\n", status);
        failures = 1;
        goto done;
    }
    printf("runtime=onnxruntime-1.28.0-host-x64 model_bytes=%zu "
           "dictionary_bytes=%zu session_ready_before=%d\n",
           model_size, dictionary_size,
           ppocr_recognizer_session_ready(recognizer));

    for (case_index = 0;
         case_index < sizeof(fixture_cases) / sizeof(fixture_cases[0]);
         ++case_index) {
        const FixtureCase *fixture = fixture_cases + case_index;
        unsigned int width = 0;
        unsigned int height = 0;
        unsigned char *pixels = load_pgm(fixture->path, &width, &height);
        PpocrImage image;
        PpocrResult result;
        size_t term;
        int matched = 1;
        if (!pixels) {
            fprintf(stderr, "FAIL case=%s could not load %s\n",
                    fixture->name, fixture->path);
            ++failures;
            continue;
        }
        image.pixels = pixels;
        image.width = width;
        image.height = height;
        image.stride = width;
        image.format = PPOCR_PIXELS_GRAY8;
        status = ppocr_recognizer_recognize(recognizer, &image, &result);
        free(pixels);
        if (status != PPOCR_OK) {
            fprintf(stderr, "FAIL case=%s status=%d error=%s\n",
                    fixture->name, status,
                    ppocr_recognizer_last_error(recognizer));
            ++failures;
            continue;
        }
        for (term = 0; term < 3u && fixture->required_ascii[term]; ++term)
            if (!contains_ascii_casefold(result.text,
                                         fixture->required_ascii[term]))
                matched = 0;
        if (fixture->required_utf8 &&
            !strstr(result.text, fixture->required_utf8))
            matched = 0;
        printf("case=%s recognized=\"%s\" confidence=%.9f "
               "raster=%ux%u input=1x3x48x%u timesteps=%u "
               "timing_us{session=%llu,preprocess=%llu,inference=%llu,"
               "decode=%llu,total=%llu} terms=%s\n",
               fixture->name, result.text, result.confidence, width, height,
               result.timing.input_width, result.timing.output_timesteps,
               (unsigned long long)result.timing.session_setup_us,
               (unsigned long long)result.timing.preprocess_us,
               (unsigned long long)result.timing.inference_us,
               (unsigned long long)result.timing.decode_us,
               (unsigned long long)result.timing.total_us,
               matched ? "PASS" : "FAIL");
        if (!matched) ++failures;
        ppocr_result_release(&result);
    }
    printf("session_ready_after=%d fixture_failures=%d\n",
           ppocr_recognizer_session_ready(recognizer), failures);
done:
    if (model) free(model);
    if (dictionary) free(dictionary);
    if (recognizer) ppocr_recognizer_destroy(recognizer);
    return failures ? 1 : 0;
}
