#if !defined(_WIN32) && !defined(_POSIX_C_SOURCE)
#define _POSIX_C_SOURCE 200809L
#endif

#include "ocr_manager.h"

#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#if defined(_WIN32)
#include <windows.h>
static void test_sleep_ms(unsigned milliseconds) { Sleep(milliseconds); }
#else
static void test_sleep_ms(unsigned milliseconds) {
    struct timespec delay;
    delay.tv_sec = (time_t)(milliseconds / 1000u);
    delay.tv_nsec = (long)(milliseconds % 1000u) * 1000000L;
    (void)nanosleep(&delay, 0);
}
#endif

/* The manager mock path must not load or call the real recognizer. */
struct PpocrRecognizer { int unused; };
int ppocr_recognizer_init(PpocrRecognizer **out, const void *model,
                          OcrSize model_size, const void *dictionary,
                          OcrSize dictionary_size) {
    (void)out; (void)model; (void)model_size;
    (void)dictionary; (void)dictionary_size;
    return PPOCR_ERR_MODEL;
}
int ppocr_recognizer_recognize(PpocrRecognizer *recognizer,
                               const PpocrImage *image,
                               PpocrResult *result) {
    (void)recognizer; (void)image; (void)result;
    return PPOCR_ERR_MODEL;
}
void ppocr_result_release(PpocrResult *result) { (void)result; }
void ppocr_recognizer_destroy(PpocrRecognizer *recognizer) {
    (void)recognizer;
}
const char *ppocr_recognizer_last_error(const PpocrRecognizer *recognizer) {
    (void)recognizer;
    return "test recognizer stub must not be called";
}
int ppocr_recognizer_session_ready(const PpocrRecognizer *recognizer) {
    (void)recognizer;
    return 0;
}

static int failures;
static volatile int recognition_calls;
static volatile int invalid_raster;
static volatile int callback_on_main_thread;
static volatile int callback_started;
static volatile unsigned callback_delay_ms;
static pthread_t main_thread;

#if defined(OCR_TEST_WRAP_SIDECAR)
static volatile int checkpoint_block;
static volatile int checkpoint_save_started;
static volatile int checkpoint_save_release;

OcrSidecarStatus __real_ocr_sidecar_save_atomic(
    const char *path, const OcrIndex *index, OcrU32 index_version,
    const OcrU8 model_hash[OCR_MODEL_HASH_BYTES]);

OcrSidecarStatus __wrap_ocr_sidecar_save_atomic(
    const char *path, const OcrIndex *index, OcrU32 index_version,
    const OcrU8 model_hash[OCR_MODEL_HASH_BYTES]) {
    if (checkpoint_block) {
        checkpoint_save_started = 1;
        while (!checkpoint_save_release) test_sleep_ms(1u);
    }
    return __real_ocr_sidecar_save_atomic(path, index, index_version,
                                          model_hash);
}
#endif

#define CHECK(expression) do {                                             \
    if (!(expression)) {                                                   \
        fprintf(stderr, "FAIL line %d: %s\n", __LINE__, #expression);     \
        ++failures;                                                        \
    }                                                                      \
} while (0)

static int mock_recognize(void *context, const OcrGrayImage *image,
                          char *utf8, OcrU32 capacity, OcrU32 *length,
                          float *confidence, PpocrTiming *timing) {
    static const char text[] =
        "Engine \xe5\x8f\x91\xe5\x8a\xa8\xe6\x9c\xba";
    OcrU32 bytes = (OcrU32)strlen(text);
    (void)context;
    if (pthread_equal(pthread_self(), main_thread))
        callback_on_main_thread = 1;
    if (!image || image->height != 48u || image->width > 1600u ||
        image->stride < image->width) invalid_raster = 1;
    if (!utf8 || capacity <= bytes || !length || !confidence || !timing)
        return OCR_ERR_INVALID;
    memcpy(utf8, text, bytes);
    *length = bytes;
    *confidence = 0.91f;
    memset(timing, 0, sizeof(*timing));
    timing->total_us = 100u;
    timing->input_width = image->width < 320u ? 320u : image->width;
    callback_started = 1;
    ++recognition_calls;
    if (callback_delay_ms) test_sleep_ms(callback_delay_ms);
    return OCR_OK;
}

static int wait_for_active(OcrManager *manager, OcrU32 expected,
                           OcrManagerStatus *out) {
    unsigned attempt;
    OcrManagerStatus status;
    for (attempt = 0; attempt < 500u; ++attempt) {
        ocr_manager_get_status(manager, &status);
        if (status.state == OCR_MANAGER_READY &&
            status.active_record_count == expected) {
            if (out) *out = status;
            return 1;
        }
        test_sleep_ms(10u);
    }
    ocr_manager_get_status(manager, &status);
    fprintf(stderr,
            "timeout state=%d active=%u cached=%u dirty=%u error=%s\n",
            (int)status.state, status.active_record_count,
            status.cached_record_count, status.dirty_region_count,
            status.last_error);
    if (out) *out = status;
    return 0;
}

static OcrManagerStroke stroke(OcrI32 index, OcrPoint *points,
                               OcrU32 count, OcrI64 completed_ms) {
    OcrManagerStroke value;
    memset(&value, 0, sizeof(value));
    value.runtime_index = index;
    value.points = points;
    value.point_count = count;
    value.base_width = 2.0f;
    value.completed_ms = completed_ms;
    value.active = 1u;
    return value;
}

static void test_lifecycle_and_sidecars(void) {
    const char *sidecar = "build/ocr-manager-test.vastocr";
    const char *clone_sidecar = "build/ocr-manager-clone.vastocr";
    OcrManager *manager = 0;
    OcrManagerCreateInfo create_info;
    OcrConfig config;
    OcrManagerStroke initial[2], reload[4], destinations[2];
    OcrManagerSearchHit hits[8];
    OcrManagerTranslation translations[2];
    OcrManagerDuplicate duplicates[2];
    OcrManagerStatus status;
    OcrI32 original_ids[2] = {0, 1};
    OcrPoint first[3] = {{0,0,1},{5,10,1},{10,0,1}};
    OcrPoint second[3] = {{13,0,1},{18,10,1},{23,0,1}};
    OcrPoint moved_first[3] = {{100,50,1},{105,60,1},{110,50,1}};
    OcrPoint moved_second[3] = {{113,50,1},{118,60,1},{123,50,1}};
    /* Deliberately close to the source: registering these one at a time would
       invalidate the source before its metadata could be cloned. */
    OcrPoint copy_first[3] = {{128,50,1},{133,60,1},{138,50,1}};
    OcrPoint copy_second[3] = {{141,50,1},{146,60,1},{151,50,1}};
    OcrPoint stored_copy_first[3] = {{1000,0,1},{1005,10,1},{1010,0,1}};
    OcrPoint stored_copy_second[3] = {{1013,0,1},{1018,10,1},{1023,0,1}};
    int calls_before_reload;

    (void)remove(sidecar);
    (void)remove(clone_sidecar);
    memset(&create_info, 0, sizeof(create_info));
    ocr_config_default(&config);
    config.idle_delay_ms = 20;
    create_info.config = &config;
    create_info.initially_enabled = 1;
    create_info.recognize = mock_recognize;
    CHECK(ocr_manager_create(&manager, &create_info) == OCR_OK);
    if (!manager) return;
    CHECK(ocr_manager_open_project(manager, 101u, sidecar) == OCR_OK);
    initial[0] = stroke(0, first, 3u, 1000);
    initial[1] = stroke(1, second, 3u, 1010);
    CHECK(ocr_manager_add_initial_strokes(manager, initial, 2u) == OCR_OK);

    /* Prove the manager did not retain the caller's mutable point arrays. */
    first[0].x = first[1].x = first[2].x = 10000.0f;
    second[0].x = second[1].x = second[2].x = 11000.0f;
    CHECK(ocr_manager_finish_initial_scan(manager) == OCR_OK);
    CHECK(wait_for_active(manager, 1u, &status));
    CHECK(recognition_calls == 1);
    CHECK(!callback_on_main_thread);
    CHECK(!invalid_raster);
    CHECK(ocr_manager_search(manager, "engine", hits, 8u) == 1);
    CHECK(ocr_manager_search(manager,
          "\xe5\x8f\x91\xe5\x8a\xa8", hits, 8u) == 1);
    CHECK(hits[0].bounds.maxx < 100.0f);

    translations[0] = (OcrManagerTranslation){0, 100.0f, 50.0f};
    translations[1] = (OcrManagerTranslation){1, 100.0f, 50.0f};
    CHECK(ocr_manager_translate_strokes(manager, translations, 2u) == 1);
    CHECK(recognition_calls == 1);
    CHECK(ocr_manager_search(manager, "engine", hits, 8u) == 1);
    CHECK(hits[0].bounds.minx >= 100.0f);

    destinations[0] = stroke(2, copy_first, 3u, 2000);
    destinations[1] = stroke(3, copy_second, 3u, 2010);
    duplicates[0] = (OcrManagerDuplicate){0, 2};
    duplicates[1] = (OcrManagerDuplicate){1, 3};
    CHECK(ocr_manager_duplicate_strokes(
              manager, destinations, duplicates, 2u) == 1);
    CHECK(wait_for_active(manager, 2u, &status));
    CHECK(recognition_calls == 1);
    CHECK(ocr_manager_search(manager, "engine", hits, 8u) == 2);

    /* Move the complete cloned record away without inference so the remaining
       lifecycle undo/redo assertions do not intentionally re-segment the two
       spatially adjacent records as one line. */
    translations[0] = (OcrManagerTranslation){2, 872.0f, -50.0f};
    translations[1] = (OcrManagerTranslation){3, 872.0f, -50.0f};
    CHECK(ocr_manager_translate_strokes(manager, translations, 2u) == 1);
    CHECK(recognition_calls == 1);

    CHECK(ocr_manager_set_strokes_active(manager, original_ids, 2u, 0) ==
          OCR_OK);
    CHECK(ocr_manager_search(manager, "engine", hits, 8u) == 1);
    CHECK(ocr_manager_set_strokes_active(manager, original_ids, 2u, 1) ==
          OCR_OK);
    CHECK(wait_for_active(manager, 2u, &status));
    CHECK(recognition_calls == 1);
    CHECK(ocr_manager_clone_project_cache(manager, 102u,
                                          clone_sidecar) == OCR_OK);
    CHECK(ocr_manager_close_project(manager, 1) == OCR_OK);

    calls_before_reload = recognition_calls;
    reload[0] = stroke(0, moved_first, 3u, 0);
    reload[1] = stroke(1, moved_second, 3u, 0);
    reload[2] = stroke(2, stored_copy_first, 3u, 0);
    reload[3] = stroke(3, stored_copy_second, 3u, 0);
    CHECK(ocr_manager_open_project(manager, 101u, sidecar) == OCR_OK);
    CHECK(ocr_manager_add_initial_strokes(manager, reload, 4u) == OCR_OK);
    CHECK(ocr_manager_finish_initial_scan(manager) == OCR_OK);
    CHECK(wait_for_active(manager, 2u, &status));
    CHECK(recognition_calls == calls_before_reload);
    CHECK(ocr_manager_close_project(manager, 0) == OCR_OK);

    CHECK(ocr_manager_open_project(manager, 102u, clone_sidecar) == OCR_OK);
    CHECK(ocr_manager_add_initial_strokes(manager, reload, 4u) == OCR_OK);
    CHECK(ocr_manager_finish_initial_scan(manager) == OCR_OK);
    CHECK(wait_for_active(manager, 2u, &status));
    CHECK(recognition_calls == calls_before_reload);
    CHECK(ocr_manager_drain_events(manager) != 0u);
    ocr_manager_shutdown(manager);
    (void)remove(sidecar);
    (void)remove(clone_sidecar);
    printf("manager lifecycle: recognition_calls=%d cache_reload_calls=0\n",
           calls_before_reload);
}

static void test_wide_line_chunking_and_defer(void) {
    OcrManager *manager = 0;
    OcrManagerCreateInfo create_info;
    OcrConfig config;
    OcrManagerStroke input;
    OcrManagerStatus status;
    OcrPoint long_line[5] = {
        {0,0,1},{1000,10,1},{2000,0,1},{3000,10,1},{4000,0,1}
    };
    int calls_at_start = recognition_calls;
    memset(&create_info, 0, sizeof(create_info));
    ocr_config_default(&config);
    config.idle_delay_ms = 20;
    create_info.config = &config;
    create_info.initially_enabled = 1;
    create_info.recognize = mock_recognize;
    CHECK(ocr_manager_create(&manager, &create_info) == OCR_OK);
    if (!manager) return;
    CHECK(ocr_manager_open_project(manager, 201u, 0) == OCR_OK);
    input = stroke(0, long_line, 5u, 1);
    CHECK(ocr_manager_add_initial_strokes(manager, &input, 1u) == OCR_OK);
    ocr_manager_set_interaction_active(manager, 1, 0);
    CHECK(ocr_manager_finish_initial_scan(manager) == OCR_OK);
    test_sleep_ms(80u);
    CHECK(recognition_calls == calls_at_start);
    ocr_manager_set_interaction_active(manager, 0, 20);
    CHECK(wait_for_active(manager, 13u, &status));
    CHECK(recognition_calls - calls_at_start == 13);
    CHECK(status.last_timing.candidate_regions == 13u);
    CHECK(!invalid_raster);
    printf("manager wide line: chunks=%d max_width_ok=%d deferred=%d\n",
           recognition_calls - calls_at_start, !invalid_raster, 1);
    ocr_manager_shutdown(manager);
}

static void test_stale_generation_discard(void) {
    OcrManager *manager = 0;
    OcrManagerCreateInfo create_info;
    OcrConfig config;
    OcrManagerStroke input;
    OcrManagerStatus status;
    OcrManagerSearchHit hit;
    OcrPoint original[3] = {{0,0,1},{5,10,1},{10,0,1}};
    OcrPoint edited[3] = {{0,0,1},{7,12,1},{20,0,1}};
    int calls_at_start = recognition_calls;
    unsigned attempt;
    memset(&create_info, 0, sizeof(create_info));
    ocr_config_default(&config);
    config.idle_delay_ms = 10;
    create_info.config = &config;
    create_info.initially_enabled = 1;
    create_info.recognize = mock_recognize;
    callback_started = 0;
    callback_delay_ms = 100u;
    CHECK(ocr_manager_create(&manager, &create_info) == OCR_OK);
    if (!manager) return;
    CHECK(ocr_manager_open_project(manager, 301u, 0) == OCR_OK);
    input = stroke(0, original, 3u, 1);
    CHECK(ocr_manager_add_initial_strokes(manager, &input, 1u) == OCR_OK);
    CHECK(ocr_manager_finish_initial_scan(manager) == OCR_OK);
    for (attempt = 0; attempt < 200u && !callback_started; ++attempt)
        test_sleep_ms(2u);
    CHECK(callback_started);
    input = stroke(0, edited, 3u, 2);
    CHECK(ocr_manager_add_completed_stroke(manager, &input) == OCR_OK);
    CHECK(wait_for_active(manager, 1u, &status));
    callback_delay_ms = 0u;
    CHECK(recognition_calls - calls_at_start >= 2);
    CHECK(status.last_timing.discarded_stale_jobs >= 1u);
    CHECK(ocr_manager_search(manager, "engine", &hit, 1u) == 1);
    CHECK(hit.bounds.maxx >= 20.0f);
    printf("manager stale job: calls=%d discarded=%u\n",
           recognition_calls - calls_at_start,
           status.last_timing.discarded_stale_jobs);
    ocr_manager_shutdown(manager);
}

static void test_mutations_during_initial_scan(void) {
    OcrManager *manager = 0;
    OcrManagerCreateInfo create_info;
    OcrConfig config;
    OcrManagerStroke input, later[2];
    OcrManagerStatus status;
    OcrManagerSearchHit hits[4];
    OcrManagerTranslation translation;
    OcrI32 id;
    OcrPoint stale[3] = {{0,0,1},{5,10,1},{10,0,1}};
    OcrPoint edited[3] = {{20,0,1},{25,10,1},{30,0,1}};
    OcrPoint unseen_current[3] = {{110,0,1},{115,10,1},{120,0,1}};
    OcrPoint added[3] = {{300,0,1},{305,10,1},{310,0,1}};
    int calls_at_start = recognition_calls;
    int found_edited = 0;
    int i;

    memset(&create_info, 0, sizeof(create_info));
    ocr_config_default(&config);
    config.idle_delay_ms = 10;
    create_info.config = &config;
    create_info.initially_enabled = 1;
    create_info.recognize = mock_recognize;
    CHECK(ocr_manager_create(&manager, &create_info) == OCR_OK);
    if (!manager) return;
    CHECK(ocr_manager_open_project(manager, 401u, 0) == OCR_OK);
    ocr_manager_get_status(manager, &status);
    CHECK(status.state == OCR_MANAGER_INDEXING);

    input = stroke(0, stale, 3u, 1000);
    CHECK(ocr_manager_add_initial_strokes(manager, &input, 1u) == OCR_OK);

    /* A live edit of an already-scanned index is authoritative. */
    input = stroke(0, edited, 3u, 1100);
    CHECK(ocr_manager_add_completed_stroke(manager, &input) == OCR_OK);
    id = 0;
    CHECK(ocr_manager_set_strokes_active(manager, &id, 1u, 0) == OCR_OK);
    CHECK(ocr_manager_set_strokes_active(manager, &id, 1u, 1) == OCR_OK);
    translation = (OcrManagerTranslation){0, 10.0f, 0.0f};
    CHECK(ocr_manager_translate_strokes(manager, &translation, 1u) == 0);

    /* An unseen index is later supplied as a current per-batch snapshot. */
    id = 1;
    CHECK(ocr_manager_set_strokes_active(manager, &id, 1u, 0) == OCR_OK);
    translation = (OcrManagerTranslation){1, 10.0f, 0.0f};
    CHECK(ocr_manager_translate_strokes(manager, &translation, 1u) == 0);
    later[0] = stroke(0, stale, 3u, 1000); /* stale; must be ignored */
    later[1] = stroke(1, unseen_current, 3u, 2000);
    later[1].active = 0u;
    CHECK(ocr_manager_add_initial_strokes(manager, later, 2u) == OCR_OK);

    input = stroke(2, added, 3u, 8000);
    CHECK(ocr_manager_add_completed_stroke(manager, &input) == OCR_OK);
    CHECK(ocr_manager_finish_initial_scan(manager) == OCR_OK);
    CHECK(wait_for_active(manager, 2u, &status));
    CHECK(recognition_calls - calls_at_start == 2);
    CHECK(ocr_manager_search(manager, "engine", hits, 4u) == 2);
    for (i = 0; i < 2; ++i) {
        CHECK(hits[i].bounds.minx >= 30.0f);
        if (hits[i].bounds.minx < 100.0f) found_edited = 1;
    }
    CHECK(found_edited);
    printf("manager initial mutations: records=2 calls=%d\n",
           recognition_calls - calls_at_start);
    ocr_manager_shutdown(manager);
}

static void test_duplicate_region_reallocation(void) {
    enum { SOURCE_COUNT = 32 };
    OcrManager *manager = 0;
    OcrManagerCreateInfo create_info;
    OcrConfig config;
    OcrManagerStroke sources[SOURCE_COUNT];
    OcrManagerStroke destinations[SOURCE_COUNT];
    OcrManagerDuplicate mappings[SOURCE_COUNT];
    OcrManagerSearchHit hits[SOURCE_COUNT * 2];
    OcrManagerStatus status;
    OcrPoint source_points[SOURCE_COUNT][3];
    OcrPoint destination_points[SOURCE_COUNT][3];
    int calls_at_start = recognition_calls;
    int calls_before_clone;
    int i;

    memset(&create_info, 0, sizeof(create_info));
    ocr_config_default(&config);
    config.idle_delay_ms = 10;
    create_info.config = &config;
    create_info.initially_enabled = 1;
    create_info.recognize = mock_recognize;
    CHECK(ocr_manager_create(&manager, &create_info) == OCR_OK);
    if (!manager) return;
    CHECK(ocr_manager_open_project(manager, 501u, 0) == OCR_OK);
    for (i = 0; i < SOURCE_COUNT; ++i) {
        float x = (float)i * 1000.0f;
        source_points[i][0] = (OcrPoint){x, 0, 1};
        source_points[i][1] = (OcrPoint){x + 5, 10, 1};
        source_points[i][2] = (OcrPoint){x + 10, 0, 1};
        sources[i] = stroke(i, source_points[i], 3u,
                            (OcrI64)i * 5000 + 1);
    }
    CHECK(ocr_manager_add_initial_strokes(
              manager, sources, SOURCE_COUNT) == OCR_OK);
    CHECK(ocr_manager_finish_initial_scan(manager) == OCR_OK);
    CHECK(wait_for_active(manager, SOURCE_COUNT, &status));
    CHECK(recognition_calls - calls_at_start == SOURCE_COUNT);

    for (i = 0; i < SOURCE_COUNT; ++i) {
        int j;
        for (j = 0; j < 3; ++j) {
            destination_points[i][j] = source_points[i][j];
            destination_points[i][j].x += 28.0f;
        }
        destinations[i] = stroke(SOURCE_COUNT + i,
                                 destination_points[i], 3u,
                                 (OcrI64)i * 5000 + 2);
        mappings[i] = (OcrManagerDuplicate){i, SOURCE_COUNT + i};
    }
    calls_before_clone = recognition_calls;
    /* Exactly 32 source regions fill the initial region allocation.  The
       first clone grows it, exercising the former retained-pointer UAF. */
    CHECK(ocr_manager_duplicate_strokes(
              manager, destinations, mappings, SOURCE_COUNT) == SOURCE_COUNT);
    CHECK(wait_for_active(manager, SOURCE_COUNT * 2u, &status));
    CHECK(recognition_calls == calls_before_clone);
    CHECK(ocr_manager_search(manager, "engine", hits,
                             SOURCE_COUNT * 2u) == SOURCE_COUNT * 2);
    printf("manager duplicate realloc: cloned=%d inference_calls=0\n",
           SOURCE_COUNT);
    ocr_manager_shutdown(manager);
}

#if defined(OCR_TEST_WRAP_SIDECAR)
typedef struct {
    OcrManager *manager;
    volatile int done;
    int result;
} CloseThreadContext;

static void *close_without_checkpoint(void *opaque) {
    CloseThreadContext *context = (CloseThreadContext *)opaque;
    context->result = ocr_manager_close_project(context->manager, 0);
    context->done = 1;
    return 0;
}

static void test_nonblocking_project_close(void) {
    const char *sidecar = "build/ocr-manager-close-test.vastocr";
    OcrManager *manager = 0;
    OcrManagerCreateInfo create_info;
    OcrConfig config;
    OcrManagerStroke input;
    OcrPoint points[3] = {{0,0,1},{5,10,1},{10,0,1}};
    CloseThreadContext close_context;
    pthread_t close_thread;
    unsigned attempt;

    (void)remove(sidecar);
    memset(&create_info, 0, sizeof(create_info));
    memset(&close_context, 0, sizeof(close_context));
    ocr_config_default(&config);
    config.idle_delay_ms = 10;
    create_info.config = &config;
    create_info.initially_enabled = 1;
    create_info.recognize = mock_recognize;
    checkpoint_block = 1;
    checkpoint_save_started = 0;
    checkpoint_save_release = 0;
    CHECK(ocr_manager_create(&manager, &create_info) == OCR_OK);
    if (!manager) return;
    CHECK(ocr_manager_open_project(manager, 601u, sidecar) == OCR_OK);
    input = stroke(0, points, 3u, 1);
    CHECK(ocr_manager_add_initial_strokes(manager, &input, 1u) == OCR_OK);
    CHECK(ocr_manager_finish_initial_scan(manager) == OCR_OK);
    for (attempt = 0; attempt < 1000u && !checkpoint_save_started; ++attempt)
        test_sleep_ms(2u);
    CHECK(checkpoint_save_started);

    close_context.manager = manager;
    CHECK(pthread_create(&close_thread, 0, close_without_checkpoint,
                         &close_context) == 0);
    for (attempt = 0; attempt < 100u && !close_context.done; ++attempt)
        test_sleep_ms(2u);
    CHECK(close_context.done); /* Must not wait for the blocked worker write. */
    checkpoint_save_release = 1;
    (void)pthread_join(close_thread, 0);
    CHECK(close_context.result == OCR_OK);
    checkpoint_block = 0;
    ocr_manager_shutdown(manager);
    (void)remove(sidecar);
    printf("manager close without checkpoint: nonblocking=1\n");
}
#endif

int main(void) {
    main_thread = pthread_self();
    test_lifecycle_and_sidecars();
    test_wide_line_chunking_and_defer();
    test_stale_generation_discard();
    test_mutations_during_initial_scan();
    test_duplicate_region_reallocation();
#if defined(OCR_TEST_WRAP_SIDECAR)
    test_nonblocking_project_close();
#endif
    if (failures) {
        fprintf(stderr, "ocr_manager tests: %d failure(s)\n", failures);
        return 1;
    }
    printf("ocr_manager tests: PASS\n");
    return 0;
}
