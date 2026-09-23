#ifndef VAST_OCR_MANAGER_H
#define VAST_OCR_MANAGER_H

#include "ocr_core.h"
#include "ppocr_recognizer.h"

#define VAST_OCR_MODEL_ID "PP-OCRv5_mobile_rec"
#define VAST_OCR_MODEL_ASSET \
    "ocr/ppocrv5_mobile_rec/inference.onnx"
#define VAST_OCR_DICTIONARY_ASSET \
    "ocr/ppocrv5_mobile_rec/ppocrv5_dict.txt"

#define OCR_MANAGER_SEARCH_LABEL_BYTES 192u
#define OCR_MANAGER_ERROR_BYTES 256u
#define OCR_MANAGER_MAX_DIRTY_REGIONS 32u

typedef struct OcrManager OcrManager;

/*
 * All input geometry is borrowed only for the duration of the call.  The
 * manager validates and deep-copies it before returning; it never retains a
 * pointer into Vast's mutable Stroke array.
 */
typedef struct {
    OcrI32 runtime_index;
    const OcrPoint *points;
    OcrU32 point_count;
    float base_width;
    OcrI64 completed_ms; /* <= 0 when unavailable (old persisted strokes). */
    OcrU32 active;
} OcrManagerStroke;

typedef struct {
    OcrI32 runtime_index;
    float dx;
    float dy;
} OcrManagerTranslation;

typedef struct {
    OcrI32 source_runtime_index;
    OcrI32 destination_runtime_index;
} OcrManagerDuplicate;

/* A test/embedding hook.  It is called only on the OCR worker. */
typedef int (*OcrManagerRecognizeFn)(
    void *context, const OcrGrayImage *image,
    char *utf8, OcrU32 utf8_capacity, OcrU32 *utf8_length,
    float *confidence, PpocrTiming *timing);

typedef struct {
    void *asset_manager; /* Android AAssetManager*, optional for mock use. */
    void *java_vm;       /* Android JavaVM*, optional on host. */
    const char *model_asset_path;      /* NULL selects the bundled default. */
    const char *dictionary_asset_path; /* NULL selects the bundled default. */
    const OcrU8 *model_hash; /* 32 bytes; NULL selects the bundled hash. */
    const OcrConfig *config; /* Copied; NULL selects ocr_config_default(). */
    int initially_enabled;
    OcrManagerRecognizeFn recognize; /* NULL selects PP-OCR/ORT. */
    void *recognize_context;
} OcrManagerCreateInfo;

typedef enum {
    OCR_MANAGER_DISABLED = 0,
    OCR_MANAGER_LOADING_CACHE = 1,
    OCR_MANAGER_READY = 2,
    OCR_MANAGER_INDEXING = 3,
    OCR_MANAGER_REBUILDING = 4,
    OCR_MANAGER_MODEL_FAILED = 5
} OcrManagerState;

typedef struct {
    OcrU64 handle; /* Stable only within the currently open project. */
    OcrBounds bounds;
    float confidence;
    char label[OCR_MANAGER_SEARCH_LABEL_BYTES];
} OcrManagerSearchHit;

typedef struct {
    OcrU64 segmentation_us;
    OcrU64 rasterization_us;
    PpocrTiming recognition;
    OcrU64 index_update_us;
    OcrU64 total_us;
    OcrU32 candidate_regions;
    OcrU32 recognized_regions;
    OcrU32 cache_hits;
    OcrU32 discarded_stale_jobs;
} OcrManagerTiming;

typedef struct {
    OcrManagerState state;
    OcrSidecarStatus sidecar_status;
    int enabled;
    int project_open;
    int initial_scan_finished;
    int worker_started;
    int session_ready;
    OcrU32 stroke_count;
    OcrU32 cached_record_count; /* Includes inactive and low-confidence rows. */
    OcrU32 active_record_count;
    OcrU32 dirty_region_count;
    OcrU64 project_generation;
    OcrU64 document_generation;
    OcrU64 job_generation;
    OcrManagerTiming last_timing;
    char last_error[OCR_MANAGER_ERROR_BYTES];
} OcrManagerStatus;

enum {
    OCR_MANAGER_EVENT_INDEX = 1u << 0,
    OCR_MANAGER_EVENT_STATUS = 1u << 1,
    OCR_MANAGER_EVENT_CHECKPOINT = 1u << 2
};

int ocr_manager_create(OcrManager **out_manager,
                       const OcrManagerCreateInfo *info);
void ocr_manager_shutdown(OcrManager *manager);

/*
 * open_project begins a batched initial scan and schedules sidecar loading.
 * Each add_initial_strokes batch is a current canvas snapshot for indices not
 * submitted yet.  Mutation APIs are valid during the scan; already-submitted
 * strokes are updated in place and never overwritten by a later stale batch.
 */
int ocr_manager_open_project(OcrManager *manager, OcrU64 project_key,
                             const char *sidecar_path);
/* checkpoint == 0 never waits for an already snapshotted worker write. */
int ocr_manager_close_project(OcrManager *manager, int checkpoint);
/* Explicit project duplication helper; does not switch the open project. */
int ocr_manager_clone_project_cache(OcrManager *manager,
                                    OcrU64 destination_project_key,
                                    const char *destination_sidecar_path);
int ocr_manager_add_initial_strokes(OcrManager *manager,
                                    const OcrManagerStroke *strokes,
                                    OcrU32 stroke_count);
int ocr_manager_finish_initial_scan(OcrManager *manager);

int ocr_manager_add_completed_stroke(OcrManager *manager,
                                     const OcrManagerStroke *stroke);
int ocr_manager_set_strokes_active(OcrManager *manager,
                                   const OcrI32 *runtime_indices,
                                   OcrU32 count, int active);

/*
 * Returns 1 when every affected cached region was moved intact and therefore
 * only its bounds were translated, 0 when local reconciliation was queued,
 * and a negative OCR_ERR_* value on invalid input.
 */
int ocr_manager_translate_strokes(OcrManager *manager,
                                  const OcrManagerTranslation *translations,
                                  OcrU32 count);

/*
 * Clone metadata only for source regions completely covered by the mapping.
 * Destination strokes must already have been added.  The nonnegative return
 * value is the number of records cloned; incomplete destinations are queued
 * for ordinary local reconciliation.
 */
int ocr_manager_duplicate_mappings(OcrManager *manager,
                                   const OcrManagerDuplicate *mappings,
                                   OcrU32 count);

/*
 * Atomic duplication path for canvas integration.  Destination geometry is
 * deep-copied and complete source-region metadata is cloned under one lock,
 * so registering the new strokes cannot invalidate the source first.  The
 * arrays are aligned: destinations[i].runtime_index must equal
 * mappings[i].destination_runtime_index.
 */
int ocr_manager_duplicate_strokes(OcrManager *manager,
                                  const OcrManagerStroke *destinations,
                                  const OcrManagerDuplicate *mappings,
                                  OcrU32 count);

int ocr_manager_search(OcrManager *manager, const char *query,
                       OcrManagerSearchHit *out_hits, OcrU32 capacity);
int ocr_manager_lookup(OcrManager *manager, OcrU64 handle,
                       OcrManagerSearchHit *out_hit);

int ocr_manager_set_enabled(OcrManager *manager, int enabled);
int ocr_manager_rebuild(OcrManager *manager);
void ocr_manager_get_status(OcrManager *manager, OcrManagerStatus *out_status);
void ocr_manager_get_last_timing(OcrManager *manager,
                                 OcrManagerTiming *out_timing);

/* Requests an asynchronous atomic sidecar checkpoint on the OCR worker. */
int ocr_manager_checkpoint(OcrManager *manager);

/*
 * active != 0 defers OCR indefinitely.  On release, defer_ms is added to the
 * normal writing-idle deadline.  Calls only signal the condition variable;
 * there is no polling and no wake lock.
 */
void ocr_manager_set_interaction_active(OcrManager *manager, int active,
                                        OcrI64 defer_ms);

/* Add this nonblocking fd to Vast's looper, then call drain_events(). */
int ocr_manager_event_fd(const OcrManager *manager);
OcrU32 ocr_manager_drain_events(OcrManager *manager);

#endif
