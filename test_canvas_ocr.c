#define VAST_OCR 1
#define main vast_v30_regression_not_run
#include "test_v30.c"
#undef main

struct OcrManager { int alive; };

typedef struct {
    int create_calls, shutdown_calls, open_calls, close_calls;
    int initial_batches, initial_strokes, initial_finished;
    int completed_calls, active_calls, translate_calls, duplicate_calls;
    int duplicate_stroke_calls, duplicate_geometry_valid;
    int checkpoint_calls, enabled_calls, rebuild_calls, search_calls, lookup_calls;
    int interaction_active_calls, interaction_release_calls;
    int last_active, last_enabled, event_sequence, completed_sequence, duplicate_sequence;
    OcrManagerTranslation translations[16];
    OcrU32 translation_count;
} MockOcr;

static struct OcrManager MOCK_MANAGER;
static MockOcr MOCK;

int ocr_manager_create(OcrManager **out_manager,
                       const OcrManagerCreateInfo *info) {
    (void)info;
    MOCK.create_calls++;
    MOCK_MANAGER.alive = 1;
    *out_manager = &MOCK_MANAGER;
    return OCR_OK;
}

void ocr_manager_shutdown(OcrManager *manager) {
    if (manager) manager->alive = 0;
    MOCK.shutdown_calls++;
}

int ocr_manager_open_project(OcrManager *manager, OcrU64 project_key,
                             const char *sidecar_path) {
    (void)manager; (void)project_key; (void)sidecar_path;
    MOCK.open_calls++;
    return OCR_OK;
}

int ocr_manager_close_project(OcrManager *manager, int checkpoint) {
    (void)manager; (void)checkpoint;
    MOCK.close_calls++;
    return OCR_OK;
}

int ocr_manager_clone_project_cache(OcrManager *manager,
                                    OcrU64 destination_project_key,
                                    const char *destination_sidecar_path) {
    (void)manager; (void)destination_project_key; (void)destination_sidecar_path;
    return OCR_OK;
}

int ocr_manager_add_initial_strokes(OcrManager *manager,
                                    const OcrManagerStroke *strokes,
                                    OcrU32 stroke_count) {
    (void)manager; (void)strokes;
    MOCK.initial_batches++;
    MOCK.initial_strokes += (int)stroke_count;
    return OCR_OK;
}

int ocr_manager_finish_initial_scan(OcrManager *manager) {
    (void)manager;
    MOCK.initial_finished++;
    return OCR_OK;
}

int ocr_manager_add_completed_stroke(OcrManager *manager,
                                     const OcrManagerStroke *stroke) {
    (void)manager; (void)stroke;
    MOCK.completed_calls++;
    MOCK.completed_sequence = ++MOCK.event_sequence;
    return OCR_OK;
}

int ocr_manager_set_strokes_active(OcrManager *manager,
                                   const OcrI32 *runtime_indices,
                                   OcrU32 count, int active) {
    (void)manager; (void)runtime_indices; (void)count;
    MOCK.active_calls++;
    MOCK.last_active = active;
    return OCR_OK;
}

int ocr_manager_translate_strokes(OcrManager *manager,
                                  const OcrManagerTranslation *translations,
                                  OcrU32 count) {
    OcrU32 i;
    (void)manager;
    MOCK.translate_calls++;
    MOCK.translation_count = count < 16u ? count : 16u;
    for (i = 0; i < MOCK.translation_count; ++i)
        MOCK.translations[i] = translations[i];
    return 1;
}

int ocr_manager_duplicate_mappings(OcrManager *manager,
                                   const OcrManagerDuplicate *mappings,
                                   OcrU32 count) {
    (void)manager; (void)mappings;
    MOCK.duplicate_calls += (int)count;
    MOCK.duplicate_sequence = ++MOCK.event_sequence;
    return (int)count;
}

int ocr_manager_duplicate_strokes(OcrManager *manager,
                                  const OcrManagerStroke *destinations,
                                  const OcrManagerDuplicate *mappings,
                                  OcrU32 count) {
    OcrU32 i;
    (void)manager;
    MOCK.duplicate_stroke_calls++;
    MOCK.duplicate_geometry_valid = destinations && mappings && count > 0u;
    for (i = 0; i < count && MOCK.duplicate_geometry_valid; ++i) {
        if (destinations[i].runtime_index !=
                mappings[i].destination_runtime_index ||
            !destinations[i].points || destinations[i].point_count == 0u ||
            !destinations[i].active)
            MOCK.duplicate_geometry_valid = 0;
    }
    MOCK.duplicate_calls += (int)count;
    MOCK.duplicate_sequence = ++MOCK.event_sequence;
    return (int)count;
}

int ocr_manager_search(OcrManager *manager, const char *query,
                       OcrManagerSearchHit *out_hits, OcrU32 capacity) {
    (void)manager;
    MOCK.search_calls++;
    if (!query || !query[0] || !capacity) return 0;
    memset(out_hits, 0, sizeof(*out_hits));
    out_hits[0].handle = 99u;
    out_hits[0].bounds = (OcrBounds){100.0f, 200.0f, 300.0f, 260.0f};
    out_hits[0].confidence = .92f;
    copy_text_local(out_hits[0].label,
                    (int)sizeof(out_hits[0].label), "engine pressure");
    return 1;
}

int ocr_manager_lookup(OcrManager *manager, OcrU64 handle,
                       OcrManagerSearchHit *out_hit) {
    (void)manager;
    MOCK.lookup_calls++;
    if (handle != 99u || !out_hit) return OCR_ERR_INVALID;
    out_hit->bounds = (OcrBounds){100.0f, 200.0f, 300.0f, 260.0f};
    return OCR_OK;
}

int ocr_manager_set_enabled(OcrManager *manager, int enabled) {
    (void)manager;
    MOCK.enabled_calls++;
    MOCK.last_enabled = enabled;
    return OCR_OK;
}

int ocr_manager_rebuild(OcrManager *manager) {
    (void)manager;
    MOCK.rebuild_calls++;
    return OCR_OK;
}

void ocr_manager_get_status(OcrManager *manager, OcrManagerStatus *out_status) {
    (void)manager;
    memset(out_status, 0, sizeof(*out_status));
    out_status->state = OCR_MANAGER_READY;
    out_status->enabled = 1;
    out_status->active_record_count = 1;
}

void ocr_manager_get_last_timing(OcrManager *manager,
                                 OcrManagerTiming *out_timing) {
    (void)manager;
    memset(out_timing, 0, sizeof(*out_timing));
}

int ocr_manager_checkpoint(OcrManager *manager) {
    (void)manager;
    MOCK.checkpoint_calls++;
    return OCR_OK;
}

void ocr_manager_set_interaction_active(OcrManager *manager, int active,
                                        OcrI64 defer_ms) {
    (void)manager; (void)defer_ms;
    if (active) MOCK.interaction_active_calls++;
    else MOCK.interaction_release_calls++;
}

int ocr_manager_event_fd(const OcrManager *manager) {
    (void)manager;
    return -1;
}

OcrU32 ocr_manager_drain_events(OcrManager *manager) {
    (void)manager;
    return 0;
}

static int translation_for(int runtime_index, float *dx, float *dy) {
    OcrU32 i;
    for (i = 0; i < MOCK.translation_count; ++i) {
        if (MOCK.translations[i].runtime_index == runtime_index) {
            *dx = MOCK.translations[i].dx;
            *dy = MOCK.translations[i].dy;
            return 1;
        }
    }
    return 0;
}

int main(void) {
    ANativeActivityCallbacks callbacks;
    ANativeActivity activity;
    ObjRef results[6];
    float dx, dy, cx, cy;
    int before_completed, before_active, before_translate, before_duplicate;
    int first, second, duplicate_index;

    memset(&callbacks, 0, sizeof(callbacks));
    memset(&activity, 0, sizeof(activity));
    memset(&MOCK, 0, sizeof(MOCK));
    activity.callbacks = &callbacks;
    activity.internalDataPath = "./testdata_canvas_ocr";
    activity.sdkVersion = 36;
    activity.vm = (JavaVM *)(size_t)1u;
    activity.assetManager = (AAssetManager *)(size_t)1u;
    ANativeActivity_onCreate(&activity, 0, 0);
    G.screenW = 1200;
    G.screenH = 800;

    check(MOCK.create_calls == 1 && MOCK.open_calls == 1 && G.ocr != 0,
          "OCR manager opens with the native activity");
    while (ocr_initial_scan_step()) {}

    first = start_stroke(10, 20, .5f);
    add_stroke_point(40, 20, .5f);
    end_stroke();
    second = start_stroke(70, 20, .5f);
    add_stroke_point(100, 20, .5f);
    end_stroke();
    check(MOCK.completed_calls == 0, "pen-up defers OCR submission until idle");
    ocr_pending_flush();
    check(MOCK.completed_calls == 2,
          "completed ink is submitted without retaining canvas pointers");

    MOCK.initial_batches = MOCK.initial_strokes = MOCK.initial_finished = 0;
    ocr_begin_initial_scan();
    before_translate = MOCK.translate_calls;
    ocr_move_begin();
    move_objref((ObjRef){SEL_STROKES, first}, 1.0f, 1.0f);
    ocr_move_commit();
    check(MOCK.translate_calls == before_translate + 1,
          "mutation during initial indexing updates seen ink without a rescan");
    while (ocr_initial_scan_step()) {}
    check(MOCK.initial_batches == 1 && MOCK.initial_strokes == 2 &&
          MOCK.initial_finished == 1,
          "existing ink is scanned incrementally through the manager");

    G.noteN = 1;
    memset(&G.notes[0], 0, sizeof(G.notes[0]));
    G.notes[0].active = 1;
    G.notes[0].w = 200;
    G.notes[0].h = 80;
    copy_text_local(G.notes[0].text, 96, "typed pressure note");
    copy_text_local(G.searchQuery, 64, "pressure");
    check(search_collect(results, 6) == 2 &&
          results[0].type == SEL_NOTE && results[1].type == SEL_HANDWRITING,
          "existing Search merges typed and handwriting providers");

    search_jump(results[1]);
    cx = 200.0f * G.scale + G.offX;
    cy = 230.0f * G.scale + G.offY;
    check(fabsf(cx - G.screenW * .5f) < .01f &&
          fabsf(cy - G.screenH * .5f) < .01f &&
          G.ocrHighlightAlpha > .99f && MOCK.lookup_calls == 1,
          "handwriting result fits world bounds and starts transient highlight");

    before_translate = MOCK.translate_calls;
    ocr_move_begin();
    move_objref((ObjRef){SEL_STROKES, first}, 10.0f, 5.0f);
    move_objref((ObjRef){SEL_STROKES, second}, 2.0f, 3.0f);
    move_objref((ObjRef){SEL_STROKES, first}, -1.0f, 4.0f);
    ocr_move_commit();
    check(MOCK.translate_calls == before_translate + 1 &&
          MOCK.translation_count == 2 &&
          translation_for(first, &dx, &dy) && fabsf(dx - 9.0f) < .001f &&
          fabsf(dy - 9.0f) < .001f,
          "movement coalesces exact per-stroke world deltas");
    check(translation_for(second, &dx, &dy) && fabsf(dx - 2.0f) < .001f &&
          fabsf(dy - 3.0f) < .001f,
          "independently moved strokes keep independent deltas");

    before_completed = MOCK.completed_calls;
    MOCK.event_sequence = MOCK.completed_sequence = MOCK.duplicate_sequence = 0;
    selection_set_one(SEL_STROKES, first);
    selection_duplicate();
    duplicate_index = G.strokeN - 1;
    check(MOCK.completed_calls == before_completed &&
          MOCK.duplicate_calls == 1 &&
          MOCK.duplicate_stroke_calls == 1 &&
          MOCK.duplicate_geometry_valid && MOCK.duplicate_sequence > 0,
          "duplicate geometry and reusable metadata are registered atomically");

    before_completed = MOCK.completed_calls;
    before_active = MOCK.active_calls;
    before_translate = MOCK.translate_calls;
    before_duplicate = MOCK.duplicate_calls;
    selection_set_one(SEL_STROKES, duplicate_index);
    G.color = 0x123456u;
    selection_apply_color();
    G.scale *= 1.7f;
    G.offX += 400.0f;
    G.offY -= 300.0f;
    theme_preset(2);
    check(MOCK.completed_calls == before_completed &&
          MOCK.active_calls == before_active &&
          MOCK.translate_calls == before_translate &&
          MOCK.duplicate_calls == before_duplicate,
          "recolor zoom pan and theme changes do not invalidate OCR");

    selection_set_one(SEL_STROKES, duplicate_index);
    selection_delete();
    check(MOCK.active_calls == before_active + 1 && !MOCK.last_active,
          "selection deletion removes handwriting from the live index");

    before_active = MOCK.active_calls;
    undo_action();
    redo_action();
    check(MOCK.active_calls == before_active + 2 && MOCK.last_active,
          "undo and redo mirror current stroke activity into OCR");

    G.ocrEnabled = 1;
    handle_ui(UI_OCR_TOGGLE, 1);
    handle_ui(UI_OCR_REBUILD, 2);
    check(MOCK.enabled_calls == 1 && !MOCK.last_enabled &&
          MOCK.rebuild_calls == 1,
          "Settings toggles and rebuild action reach the OCR manager");

    ocr_set_interaction(1);
    ocr_set_interaction(0);
    check(MOCK.interaction_active_calls == 1 &&
          MOCK.interaction_release_calls == 1,
          "canvas interaction defers and later releases background OCR");

    if (callbacks.onDestroy) callbacks.onDestroy(&activity);
    check(MOCK.shutdown_calls == 1 && MOCK.close_calls >= 1,
          "OCR worker is checkpointed and released before canvas teardown");

    if (fail) return 1;
    printf("Canvas OCR integration tests passed\n");
    return 0;
}
