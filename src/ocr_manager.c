#if !defined(__ANDROID__) && !defined(_WIN32) && !defined(_POSIX_C_SOURCE)
#define _POSIX_C_SOURCE 200809L
#endif

#include "ocr_manager.h"
#include "ocr_unicode_android.h"

/*
 * Vast intentionally builds its Android native library without an NDK
 * sysroot.  Keep the Android ABI declarations local and opaque.  These sizes
 * are the public bionic LP64 ABI (minSdk 26), not private implementation
 * structs.
 */
#if defined(__ANDROID__)
#include "jni.h"
typedef long OcrThread;
typedef struct { OcrI32 opaque[10]; } OcrMutex;
typedef struct { OcrI32 opaque[12]; } OcrCond;
typedef struct { long tv_sec; long tv_nsec; } OcrTimespec;
extern int pthread_create(OcrThread *, const void *, void *(*)(void *), void *);
extern int pthread_join(OcrThread, void **);
extern int pthread_mutex_init(OcrMutex *, const void *);
extern int pthread_mutex_destroy(OcrMutex *);
extern int pthread_mutex_lock(OcrMutex *);
extern int pthread_mutex_unlock(OcrMutex *);
extern int pthread_cond_init(OcrCond *, const void *);
extern int pthread_cond_destroy(OcrCond *);
extern int pthread_cond_wait(OcrCond *, OcrMutex *);
extern int pthread_cond_timedwait(OcrCond *, OcrMutex *, const OcrTimespec *);
extern int pthread_cond_signal(OcrCond *);
extern int pthread_cond_broadcast(OcrCond *);
extern int clock_gettime(int, OcrTimespec *);
extern int setpriority(int, int, int);
extern int eventfd(unsigned int, int);
extern long read(int, void *, OcrSize);
extern long write(int, const void *, OcrSize);
extern int close(int);
typedef struct AAssetManager AAssetManager;
typedef struct AAsset AAsset;
extern AAsset *AAssetManager_open(AAssetManager *, const char *, int);
extern long AAsset_getLength(AAsset *);
extern int AAsset_read(AAsset *, void *, OcrSize);
extern void AAsset_close(AAsset *);
#define OCR_CLOCK_REALTIME 0
#define OCR_CLOCK_MONOTONIC 1
#define OCR_EVENT_FLAGS (2048 | 524288) /* EFD_NONBLOCK | EFD_CLOEXEC */
#else
#include <pthread.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <stdio.h>
#include <math.h>
#if defined(_WIN32)
#include <io.h>
#include <fcntl.h>
#else
#include <unistd.h>
#include <sys/eventfd.h>
#include <sys/resource.h>
#endif
typedef pthread_t OcrThread;
typedef pthread_mutex_t OcrMutex;
typedef pthread_cond_t OcrCond;
typedef struct timespec OcrTimespec;
#define OCR_CLOCK_REALTIME CLOCK_REALTIME
#define OCR_CLOCK_MONOTONIC CLOCK_MONOTONIC
#endif

#if defined(__ANDROID__)
extern void *malloc(OcrSize);
extern void *calloc(OcrSize, OcrSize);
extern void *realloc(void *, OcrSize);
extern void free(void *);
extern void *memcpy(void *, const void *, OcrSize);
extern void *memset(void *, int, OcrSize);
extern int memcmp(const void *, const void *, OcrSize);
extern OcrSize strlen(const char *);
extern int remove(const char *);
#endif

#define MANAGER_MAX_STROKES 100000u
#define MANAGER_MAX_STROKE_POINTS 1000000u
#define MANAGER_MAX_LOCAL_STROKES 65536u
#define MANAGER_PATH_BYTES 1024u
#define MANAGER_ASSET_PATH_BYTES 192u
#define MANAGER_AASSET_MODE_BUFFER 3
#define MANAGER_JOB_NONE 0
#define MANAGER_JOB_SIDECAR 1
#define MANAGER_JOB_RECOGNIZE 2
#define MANAGER_JOB_CHECKPOINT 3

typedef struct {
    OcrI32 runtime_index;
    OcrPoint *points;
    OcrU32 point_count;
    float base_width;
    OcrBounds bounds;
    OcrI64 completed_ms;
    OcrU32 active;
    OcrU32 initial_authoritative;
} ManagerStroke;

typedef struct {
    OcrU64 occurrence_id;
    OcrI32 *members;
    OcrU32 member_count;
    OcrBounds bounds;
} ManagerRegion;

typedef struct {
    OcrBounds bounds;
} ManagerDirty;

typedef struct {
    OcrStrokeView *strokes;
    OcrU32 stroke_count;
    OcrU64 *touched;
    OcrU32 touched_count;
    OcrBounds dirty_bounds;
    int full;
    OcrU64 project_generation;
    OcrU64 document_generation;
    OcrU64 job_generation;
} ManagerWork;

typedef struct {
    OcrU8 source_hash[OCR_HASH_BYTES];
    OcrSourceRef *sources;
    OcrU32 source_count;
    OcrI32 *members;
    OcrU32 member_count;
    OcrBounds bounds;
    OcrU64 reuse_id;
    OcrU64 clone_id;
    char *text;
    OcrU32 text_len;
    char *normalized;
    OcrU32 normalized_len;
    float confidence;
    int recognized;
} ManagerResult;

typedef struct {
    OcrU32 *members; /* Indices into ManagerWork.strokes. */
    OcrU32 member_count;
    OcrBounds bounds;
    OcrU32 ordinal;
    OcrU32 piece_count;
} ManagerPiece;

typedef struct {
    ManagerPiece *items;
    OcrU32 count;
} ManagerPieceList;

struct OcrManager {
    OcrMutex mutex;
    OcrCond cond;
    OcrThread worker;
    int worker_started;
    int stopping;
    int running_job;

    int event_read_fd;
    int event_write_fd;
    OcrU32 pending_events;

    void *asset_manager;
    void *java_vm;
    char model_asset[MANAGER_ASSET_PATH_BYTES];
    char dictionary_asset[MANAGER_ASSET_PATH_BYTES];
    OcrU8 model_hash[OCR_MODEL_HASH_BYTES];
    OcrConfig config;
    OcrManagerRecognizeFn recognize;
    void *recognize_context;
    PpocrRecognizer *recognizer;
    int session_ready;

    int enabled;
    int project_open;
    int initial_scan_finished;
    int interaction_active;
    OcrI64 defer_until_ms;
    OcrI64 recognize_not_before_ms;
    OcrManagerState state;
    OcrSidecarStatus sidecar_status;
    char last_error[OCR_MANAGER_ERROR_BYTES];
    OcrManagerTiming last_timing;

    OcrU64 project_generation;
    OcrU64 document_generation;
    OcrU64 job_generation;
    OcrU64 project_key;
    char sidecar_path[MANAGER_PATH_BYTES];
    int sidecar_load_pending;
    int sidecar_loaded;
    int checkpoint_pending;
    int checkpoint_running;
    int rebuilding;

    ManagerStroke *strokes;
    OcrU32 stroke_count;
    OcrU32 stroke_capacity;
    ManagerRegion *regions;
    OcrU32 region_count;
    OcrU32 region_capacity;
    ManagerDirty dirty[OCR_MANAGER_MAX_DIRTY_REGIONS];
    OcrU32 dirty_count;
    int dirty_all;
    OcrIndex index;
};

static const OcrU8 manager_default_model_hash[OCR_MODEL_HASH_BYTES] = {
    0xdc,0x7d,0xe8,0xee,0x31,0xd9,0x24,0x67,
    0x83,0xcf,0x34,0x6f,0x3b,0x20,0x7b,0x4c,
    0xb8,0x71,0xe1,0x18,0x24,0xb1,0xcf,0xab,
    0x2f,0xf2,0x0a,0xa1,0xf1,0xf8,0xe6,0x7b
};

static int manager_finite(float value) {
    OcrU32 bits = 0;
    memcpy(&bits, &value, 4u);
    return (bits & 0x7f800000u) != 0x7f800000u;
}

static float manager_absf(float value) { return value < 0.0f ? -value : value; }
static float manager_minf(float a, float b) { return a < b ? a : b; }
static float manager_maxf(float a, float b) { return a > b ? a : b; }

static int manager_bounds_valid(const OcrBounds *bounds) {
    return bounds && manager_finite(bounds->minx) &&
           manager_finite(bounds->miny) && manager_finite(bounds->maxx) &&
           manager_finite(bounds->maxy) && bounds->minx <= bounds->maxx &&
           bounds->miny <= bounds->maxy && bounds->minx >= -1.0e12f &&
           bounds->miny >= -1.0e12f && bounds->maxx <= 1.0e12f &&
           bounds->maxy <= 1.0e12f;
}

static int manager_bounds_intersect(const OcrBounds *a, const OcrBounds *b) {
    return !(a->maxx < b->minx || b->maxx < a->minx ||
             a->maxy < b->miny || b->maxy < a->miny);
}

static OcrBounds manager_bounds_union(OcrBounds a, OcrBounds b) {
    OcrBounds out;
    out.minx = manager_minf(a.minx, b.minx);
    out.miny = manager_minf(a.miny, b.miny);
    out.maxx = manager_maxf(a.maxx, b.maxx);
    out.maxy = manager_maxf(a.maxy, b.maxy);
    return out;
}

static int manager_spatially_compatible(const OcrConfig *cfg,
                                        const OcrBounds *a,
                                        const OcrBounds *b) {
    float ha = manager_maxf(a->maxy - a->miny, 0.5f);
    float hb = manager_maxf(b->maxy - b->miny, 0.5f);
    float wa = manager_maxf(a->maxx - a->minx, 0.5f);
    float wb = manager_maxf(b->maxx - b->minx, 0.5f);
    float maxh = manager_maxf(ha, hb), minh = manager_minf(ha, hb);
    float hgap = 0.0f, vgap = 0.0f, overlap, baseline;
    float small_limit = maxh * 0.34f;
    if (a->maxx < b->minx) hgap = b->minx - a->maxx;
    else if (b->maxx < a->minx) hgap = a->minx - b->maxx;
    if (a->maxy < b->miny) vgap = b->miny - a->maxy;
    else if (b->maxy < a->miny) vgap = a->miny - b->maxy;
    overlap = manager_minf(a->maxy, b->maxy) -
              manager_maxf(a->miny, b->miny);
    baseline = manager_absf(a->maxy - b->maxy);
    if (minh >= small_limit && maxh / minh > cfg->height_ratio_limit)
        return 0;
    if (hgap > cfg->horizontal_gap_factor * maxh + 0.20f * (wa + wb))
        return 0;
    if (vgap > cfg->vertical_gap_factor * maxh) return 0;
    if (overlap < 0.05f * minh && baseline > cfg->baseline_factor * maxh &&
        minh >= small_limit) return 0;
    return 1;
}

static OcrI64 manager_now_ms(void) {
    OcrTimespec value;
    if (clock_gettime(OCR_CLOCK_MONOTONIC, &value) != 0 ||
        value.tv_sec < 0 || value.tv_nsec < 0) return 0;
    return (OcrI64)value.tv_sec * 1000 + value.tv_nsec / 1000000;
}

static OcrU64 manager_now_us(void) {
    OcrTimespec value;
    if (clock_gettime(OCR_CLOCK_MONOTONIC, &value) != 0 ||
        value.tv_sec < 0 || value.tv_nsec < 0) return 0;
    return (OcrU64)value.tv_sec * 1000000u + (OcrU64)value.tv_nsec / 1000u;
}

static OcrU64 manager_elapsed_us(OcrU64 start, OcrU64 end) {
    return start && end >= start ? end - start : 0;
}

static void manager_copy_string(char *destination, OcrU32 capacity,
                                const char *source) {
    OcrU32 i = 0;
    if (!destination || !capacity) return;
    if (source) while (source[i] && i + 1u < capacity) {
        destination[i] = source[i];
        ++i;
    }
    destination[i] = 0;
}

static void manager_set_error_locked(OcrManager *manager, const char *error) {
    manager_copy_string(manager->last_error, OCR_MANAGER_ERROR_BYTES,
                        error ? error : "");
}

static int manager_create_event(OcrManager *manager) {
#if defined(_WIN32)
    int fds[2];
    if (_pipe(fds, 64, _O_BINARY | _O_NOINHERIT) != 0) return 0;
    manager->event_read_fd = fds[0];
    manager->event_write_fd = fds[1];
    return 1;
#else
    int fd = eventfd(0u,
#if defined(__ANDROID__)
                     OCR_EVENT_FLAGS
#else
                     EFD_NONBLOCK | EFD_CLOEXEC
#endif
    );
    if (fd < 0) return 0;
    manager->event_read_fd = manager->event_write_fd = fd;
    return 1;
#endif
}

static void manager_close_event(OcrManager *manager) {
    if (!manager) return;
#if defined(_WIN32)
    if (manager->event_read_fd >= 0) _close(manager->event_read_fd);
    if (manager->event_write_fd >= 0) _close(manager->event_write_fd);
#else
    if (manager->event_read_fd >= 0) close(manager->event_read_fd);
#endif
    manager->event_read_fd = manager->event_write_fd = -1;
}

static void manager_notify_locked(OcrManager *manager, OcrU32 events) {
    if (!events) return;
    if (!manager->pending_events && manager->event_write_fd >= 0) {
#if defined(_WIN32)
        unsigned char byte = 1u;
        (void)_write(manager->event_write_fd, &byte, 1u);
#else
        OcrU64 one = 1u;
        (void)write(manager->event_write_fd, &one, sizeof(one));
#endif
    }
    manager->pending_events |= events;
}

static void manager_wait_until_locked(OcrManager *manager, OcrI64 deadline_ms) {
    OcrTimespec realtime;
    OcrI64 now = manager_now_ms(), remaining = deadline_ms - now;
    if (remaining <= 0) return;
    if (clock_gettime(OCR_CLOCK_REALTIME, &realtime) != 0) {
        (void)pthread_cond_wait(&manager->cond, &manager->mutex);
        return;
    }
    realtime.tv_sec += (long)(remaining / 1000);
    realtime.tv_nsec += (long)((remaining % 1000) * 1000000);
    if (realtime.tv_nsec >= 1000000000L) {
        realtime.tv_sec += 1;
        realtime.tv_nsec -= 1000000000L;
    }
    (void)pthread_cond_timedwait(&manager->cond, &manager->mutex, &realtime);
}

static void manager_free_stroke(ManagerStroke *stroke) {
    if (!stroke) return;
    if (stroke->points) free(stroke->points);
    memset(stroke, 0, sizeof(*stroke));
}

static void manager_free_strokes(OcrManager *manager) {
    OcrU32 i;
    for (i = 0; i < manager->stroke_count; ++i)
        manager_free_stroke(manager->strokes + i);
    if (manager->strokes) free(manager->strokes);
    manager->strokes = 0;
    manager->stroke_count = manager->stroke_capacity = 0;
}

static void manager_free_region(ManagerRegion *region) {
    if (!region) return;
    if (region->members) free(region->members);
    memset(region, 0, sizeof(*region));
}

static void manager_free_regions(OcrManager *manager) {
    OcrU32 i;
    for (i = 0; i < manager->region_count; ++i)
        manager_free_region(manager->regions + i);
    if (manager->regions) free(manager->regions);
    manager->regions = 0;
    manager->region_count = manager->region_capacity = 0;
}

static OcrI32 manager_find_stroke_slot(const OcrManager *manager,
                                       OcrI32 runtime_index) {
    OcrU32 i;
    for (i = 0; i < manager->stroke_count; ++i)
        if (manager->strokes[i].runtime_index == runtime_index)
            return (OcrI32)i;
    return -1;
}

static int manager_copy_input_stroke(const OcrManagerStroke *input,
                                     ManagerStroke *out) {
    OcrStrokeView view;
    OcrSize bytes;
    int rc;
    if (!input || !out || input->runtime_index < 0 || !input->points ||
        !input->point_count || input->point_count > MANAGER_MAX_STROKE_POINTS ||
        !manager_finite(input->base_width) || input->base_width < 0.0f)
        return OCR_ERR_INVALID;
    bytes = (OcrSize)input->point_count * sizeof(OcrPoint);
    if (bytes / sizeof(OcrPoint) != input->point_count) return OCR_ERR_LIMIT;
    memset(out, 0, sizeof(*out));
    out->points = (OcrPoint *)malloc(bytes);
    if (!out->points) return OCR_ERR_NOMEM;
    memcpy(out->points, input->points, bytes);
    out->runtime_index = input->runtime_index;
    out->point_count = input->point_count;
    out->base_width = input->base_width;
    out->completed_ms = input->completed_ms;
    out->active = input->active ? 1u : 0u;
    memset(&view, 0, sizeof(view));
    view.points = out->points;
    view.point_count = out->point_count;
    rc = ocr_stroke_bounds(&view, &out->bounds);
    if (rc != OCR_OK) {
        manager_free_stroke(out);
        return rc;
    }
    return OCR_OK;
}

static int manager_reserve_strokes(OcrManager *manager, OcrU32 needed) {
    ManagerStroke *items;
    OcrU32 capacity;
    if (needed <= manager->stroke_capacity) return OCR_OK;
    if (needed > MANAGER_MAX_STROKES) return OCR_ERR_LIMIT;
    capacity = manager->stroke_capacity ? manager->stroke_capacity * 2u : 64u;
    if (capacity < needed) capacity = needed;
    if (capacity > MANAGER_MAX_STROKES) capacity = MANAGER_MAX_STROKES;
    items = (ManagerStroke *)realloc(
        manager->strokes, (OcrSize)capacity * sizeof(ManagerStroke));
    if (!items) return OCR_ERR_NOMEM;
    manager->strokes = items;
    manager->stroke_capacity = capacity;
    return OCR_OK;
}

static int manager_insert_stroke_locked(OcrManager *manager,
                                        ManagerStroke *stroke,
                                        OcrBounds *old_bounds,
                                        int *replaced) {
    OcrI32 slot = manager_find_stroke_slot(manager, stroke->runtime_index);
    if (replaced) *replaced = 0;
    if (slot >= 0) {
        ManagerStroke old = manager->strokes[(OcrU32)slot];
        if (old_bounds) *old_bounds = old.bounds;
        manager->strokes[(OcrU32)slot] = *stroke;
        memset(stroke, 0, sizeof(*stroke));
        if (old.points) free(old.points);
        if (replaced) *replaced = 1;
        return OCR_OK;
    }
    if (manager_reserve_strokes(manager, manager->stroke_count + 1u) != OCR_OK)
        return OCR_ERR_NOMEM;
    manager->strokes[manager->stroke_count++] = *stroke;
    memset(stroke, 0, sizeof(*stroke));
    return OCR_OK;
}

static int manager_region_has_member(const ManagerRegion *region,
                                     OcrI32 runtime_index) {
    OcrU32 i;
    for (i = 0; i < region->member_count; ++i)
        if (region->members[i] == runtime_index) return 1;
    return 0;
}

static OcrI32 manager_find_region_slot(const OcrManager *manager,
                                       OcrU64 occurrence_id) {
    OcrU32 i;
    for (i = 0; i < manager->region_count; ++i)
        if (manager->regions[i].occurrence_id == occurrence_id)
            return (OcrI32)i;
    return -1;
}

static void manager_remove_region_slot_locked(OcrManager *manager,
                                              OcrU32 slot) {
    if (slot >= manager->region_count) return;
    manager_free_region(manager->regions + slot);
    if (slot + 1u < manager->region_count)
        manager->regions[slot] = manager->regions[manager->region_count - 1u];
    --manager->region_count;
}

static void manager_remove_region_id_locked(OcrManager *manager,
                                            OcrU64 occurrence_id) {
    OcrI32 slot = manager_find_region_slot(manager, occurrence_id);
    if (slot >= 0) manager_remove_region_slot_locked(manager, (OcrU32)slot);
}

static int manager_set_region_locked(OcrManager *manager, OcrU64 occurrence_id,
                                     const OcrI32 *members,
                                     OcrU32 member_count, OcrBounds bounds) {
    OcrI32 *member_copy = 0;
    OcrI32 slot;
    ManagerRegion *region;
    if (!occurrence_id || (!members && member_count) ||
        member_count > OCR_MAX_SOURCES_PER_RECORD ||
        !manager_bounds_valid(&bounds)) return OCR_ERR_INVALID;
    if (member_count) {
        member_copy = (OcrI32 *)malloc((OcrSize)member_count * sizeof(OcrI32));
        if (!member_copy) return OCR_ERR_NOMEM;
        memcpy(member_copy, members, (OcrSize)member_count * sizeof(OcrI32));
    }
    slot = manager_find_region_slot(manager, occurrence_id);
    if (slot < 0) {
        if (manager->region_count == manager->region_capacity) {
            OcrU32 capacity = manager->region_capacity
                                  ? manager->region_capacity * 2u : 32u;
            ManagerRegion *items;
            if (capacity > OCR_MAX_RECORDS) capacity = OCR_MAX_RECORDS;
            if (capacity <= manager->region_count) {
                if (member_copy) free(member_copy);
                return OCR_ERR_LIMIT;
            }
            items = (ManagerRegion *)realloc(
                manager->regions, (OcrSize)capacity * sizeof(ManagerRegion));
            if (!items) {
                if (member_copy) free(member_copy);
                return OCR_ERR_NOMEM;
            }
            manager->regions = items;
            manager->region_capacity = capacity;
        }
        slot = (OcrI32)manager->region_count++;
        memset(manager->regions + (OcrU32)slot, 0, sizeof(ManagerRegion));
    }
    region = manager->regions + (OcrU32)slot;
    if (region->members) free(region->members);
    region->occurrence_id = occurrence_id;
    region->members = member_copy;
    region->member_count = member_count;
    region->bounds = bounds;
    return OCR_OK;
}

static int manager_u64_contains(const OcrU64 *items, OcrU32 count,
                                OcrU64 value) {
    OcrU32 i;
    for (i = 0; i < count; ++i) if (items[i] == value) return 1;
    return 0;
}

static int manager_clone_index(const OcrIndex *source, OcrU64 project_key,
                               OcrIndex *out) {
    OcrU32 i;
    int rc;
    ocr_index_init(out, project_key);
    for (i = 0; i < source->count; ++i) {
        rc = ocr_index_put(out, source->records + i);
        if (rc != OCR_OK) {
            ocr_index_free(out);
            return rc;
        }
    }
    return OCR_OK;
}

static void manager_deactivate_occurrence_locked(OcrManager *manager,
                                                 OcrU64 occurrence_id) {
    (void)ocr_index_set_active(&manager->index, occurrence_id, 0);
}

static int manager_invalidate_bounds_locked(OcrManager *manager,
                                            OcrBounds bounds) {
    OcrU32 i;
    int changed = 0;
    for (i = 0; i < manager->region_count; ++i) {
        ManagerRegion *region = manager->regions + i;
        if (manager_bounds_intersect(&region->bounds, &bounds) ||
            manager_spatially_compatible(&manager->config,
                                         &region->bounds, &bounds)) {
            const OcrIndexRecord *record =
                ocr_index_find(&manager->index, region->occurrence_id);
            if (record && (record->flags & OCR_RECORD_ACTIVE)) {
                manager_deactivate_occurrence_locked(manager,
                                                     region->occurrence_id);
                changed = 1;
            }
        }
    }
    return changed;
}

static void manager_queue_dirty_locked(OcrManager *manager, OcrBounds bounds,
                                       int full) {
    OcrU32 i, j;
    if (full) {
        manager->dirty_all = 1;
        manager->dirty_count = 0;
    } else if (!manager->dirty_all && manager_bounds_valid(&bounds)) {
        for (i = 0; i < manager->dirty_count; ++i) {
            if (manager_bounds_intersect(&manager->dirty[i].bounds, &bounds) ||
                manager_spatially_compatible(&manager->config,
                                             &manager->dirty[i].bounds,
                                             &bounds)) {
                manager->dirty[i].bounds =
                    manager_bounds_union(manager->dirty[i].bounds, bounds);
                for (j = 0; j < manager->dirty_count; ++j) {
                    if (j != i && manager_bounds_intersect(
                                      &manager->dirty[i].bounds,
                                      &manager->dirty[j].bounds)) {
                        manager->dirty[i].bounds = manager_bounds_union(
                            manager->dirty[i].bounds,
                            manager->dirty[j].bounds);
                        manager->dirty[j] =
                            manager->dirty[manager->dirty_count - 1u];
                        --manager->dirty_count;
                        if (j < i) --i;
                        --j;
                    }
                }
                goto queued;
            }
        }
        if (manager->dirty_count < OCR_MANAGER_MAX_DIRTY_REGIONS) {
            manager->dirty[manager->dirty_count++].bounds = bounds;
        } else {
            OcrBounds merged = bounds;
            for (i = 0; i < manager->dirty_count; ++i)
                merged = manager_bounds_union(merged,
                                              manager->dirty[i].bounds);
            manager->dirty_count = 1u;
            manager->dirty[0].bounds = merged;
        }
    }
queued:
    ++manager->job_generation;
    manager->recognize_not_before_ms =
        manager_now_ms() + manager->config.idle_delay_ms;
    if (manager->enabled && manager->project_open &&
        manager->initial_scan_finished && manager->state != OCR_MANAGER_MODEL_FAILED)
        manager->state = manager->rebuilding ? OCR_MANAGER_REBUILDING
                                             : OCR_MANAGER_INDEXING;
    (void)pthread_cond_signal(&manager->cond);
}

static int manager_strokes_compatible(const OcrManager *manager,
                                      const ManagerStroke *a,
                                      const ManagerStroke *b) {
    OcrI64 delta;
    if (!manager_spatially_compatible(&manager->config, &a->bounds,
                                      &b->bounds)) return 0;
    if (a->completed_ms <= 0 || b->completed_ms <= 0) return 1;
    delta = a->completed_ms - b->completed_ms;
    if (delta < 0) delta = -delta;
    return delta <= manager->config.temporal_gap_ms;
}

static void manager_work_free(ManagerWork *work) {
    OcrU32 i;
    if (!work) return;
    if (work->strokes) {
        for (i = 0; i < work->stroke_count; ++i)
            if (work->strokes[i].points)
                free((void *)work->strokes[i].points);
        free(work->strokes);
    }
    if (work->touched) free(work->touched);
    memset(work, 0, sizeof(*work));
}

static int manager_build_work_locked(OcrManager *manager, int full,
                                     OcrBounds dirty_bounds,
                                     ManagerWork *work) {
    OcrU8 *selected = 0, *region_selected = 0;
    OcrU32 i, j, count = 0, touched_count = 0;
    int changed = 1;
    memset(work, 0, sizeof(*work));
    work->full = full;
    work->dirty_bounds = dirty_bounds;
    work->project_generation = manager->project_generation;
    work->document_generation = manager->document_generation;
    work->job_generation = manager->job_generation;
    if (manager->stroke_count) {
        selected = (OcrU8 *)calloc(manager->stroke_count, 1u);
        if (!selected) return OCR_ERR_NOMEM;
    }
    if (manager->region_count) {
        region_selected = (OcrU8 *)calloc(manager->region_count, 1u);
        if (!region_selected) { free(selected); return OCR_ERR_NOMEM; }
    }
    if (full) {
        for (i = 0; i < manager->stroke_count; ++i) selected[i] = 1u;
        for (i = 0; i < manager->region_count; ++i) region_selected[i] = 1u;
    } else {
        for (i = 0; i < manager->stroke_count; ++i) {
            const ManagerStroke *stroke = manager->strokes + i;
            if (manager_bounds_intersect(&stroke->bounds, &dirty_bounds) ||
                manager_spatially_compatible(&manager->config,
                                             &stroke->bounds, &dirty_bounds))
                selected[i] = 1u;
        }
        while (changed) {
            changed = 0;
            for (i = 0; i < manager->region_count; ++i) {
                ManagerRegion *region = manager->regions + i;
                int touches = region_selected[i] ||
                    manager_bounds_intersect(&region->bounds, &dirty_bounds) ||
                    manager_spatially_compatible(&manager->config,
                                                 &region->bounds,
                                                 &dirty_bounds);
                if (!touches) {
                    for (j = 0; j < region->member_count && !touches; ++j) {
                        OcrI32 slot = manager_find_stroke_slot(
                            manager, region->members[j]);
                        if (slot >= 0 && selected[(OcrU32)slot]) touches = 1;
                    }
                }
                if (touches && !region_selected[i]) {
                    region_selected[i] = 1u;
                    changed = 1;
                }
                if (touches) for (j = 0; j < region->member_count; ++j) {
                    OcrI32 slot = manager_find_stroke_slot(
                        manager, region->members[j]);
                    if (slot >= 0 && !selected[(OcrU32)slot]) {
                        selected[(OcrU32)slot] = 1u;
                        changed = 1;
                    }
                }
            }
            for (i = 0; i < manager->stroke_count; ++i) {
                if (selected[i] || !manager->strokes[i].active) continue;
                for (j = 0; j < manager->stroke_count; ++j) {
                    if (selected[j] && manager_strokes_compatible(
                                           manager, manager->strokes + i,
                                           manager->strokes + j)) {
                        selected[i] = 1u;
                        changed = 1;
                        break;
                    }
                }
            }
        }
    }
    for (i = 0; i < manager->stroke_count; ++i)
        if (selected[i] && manager->strokes[i].active) ++count;
    for (i = 0; i < manager->region_count; ++i)
        if (region_selected[i]) ++touched_count;
    if (count > MANAGER_MAX_LOCAL_STROKES) {
        free(selected); free(region_selected);
        return OCR_ERR_LIMIT;
    }
    if (count) {
        OcrU32 at = 0;
        work->strokes = (OcrStrokeView *)calloc(count, sizeof(OcrStrokeView));
        if (!work->strokes) {
            free(selected); free(region_selected);
            return OCR_ERR_NOMEM;
        }
        for (i = 0; i < manager->stroke_count; ++i) if (selected[i] &&
            manager->strokes[i].active) {
            ManagerStroke *source = manager->strokes + i;
            OcrSize bytes = (OcrSize)source->point_count * sizeof(OcrPoint);
            OcrPoint *points = (OcrPoint *)malloc(bytes);
            if (!points) {
                work->stroke_count = at;
                manager_work_free(work);
                free(selected); free(region_selected);
                return OCR_ERR_NOMEM;
            }
            memcpy(points, source->points, bytes);
            work->strokes[at].points = points;
            work->strokes[at].point_count = source->point_count;
            work->strokes[at].base_width = source->base_width;
            work->strokes[at].bounds = source->bounds;
            work->strokes[at].runtime_index = source->runtime_index;
            work->strokes[at].completed_ms = source->completed_ms;
            work->strokes[at].active = 1u;
            ++at;
        }
        work->stroke_count = count;
    }
    if (touched_count) {
        OcrU32 at = 0;
        work->touched = (OcrU64 *)malloc(
            (OcrSize)touched_count * sizeof(OcrU64));
        if (!work->touched) {
            manager_work_free(work);
            free(selected); free(region_selected);
            return OCR_ERR_NOMEM;
        }
        for (i = 0; i < manager->region_count; ++i)
            if (region_selected[i])
                work->touched[at++] = manager->regions[i].occurrence_id;
        work->touched_count = touched_count;
    }
    for (i = 0; i < work->touched_count; ++i)
        manager_deactivate_occurrence_locked(manager, work->touched[i]);
    if (work->touched_count)
        manager_notify_locked(manager, OCR_MANAGER_EVENT_INDEX);
    free(selected);
    free(region_selected);
    return OCR_OK;
}

static int manager_take_work_locked(OcrManager *manager, ManagerWork *work) {
    int full;
    OcrBounds bounds = {0, 0, 0, 0};
    int rc;
    if (!manager->dirty_all && !manager->dirty_count) return OCR_ERR_NOT_FOUND;
    full = manager->dirty_all;
    if (full) {
        manager->dirty_all = 0;
    } else {
        bounds = manager->dirty[0].bounds;
        manager->dirty[0] = manager->dirty[manager->dirty_count - 1u];
        --manager->dirty_count;
    }
    rc = manager_build_work_locked(manager, full, bounds, work);
    if (rc != OCR_OK) {
        manager_queue_dirty_locked(manager, bounds, full);
        return rc;
    }
    manager->running_job = 1;
    manager->state = manager->rebuilding ? OCR_MANAGER_REBUILDING
                                         : OCR_MANAGER_INDEXING;
    manager_notify_locked(manager, OCR_MANAGER_EVENT_STATUS);
    return OCR_OK;
}

static int manager_work_current_locked(const OcrManager *manager,
                                       const ManagerWork *work) {
    return manager->project_open &&
           manager->project_generation == work->project_generation &&
           manager->document_generation == work->document_generation &&
           manager->job_generation == work->job_generation;
}

static void manager_requeue_work_locked(OcrManager *manager,
                                        const ManagerWork *work) {
    if (manager->project_open &&
        manager->project_generation == work->project_generation)
        manager_queue_dirty_locked(manager, work->dirty_bounds, work->full);
}

static void manager_piece_list_free(ManagerPieceList *list) {
    OcrU32 i;
    if (!list) return;
    for (i = 0; i < list->count; ++i)
        if (list->items[i].members) free(list->items[i].members);
    if (list->items) free(list->items);
    memset(list, 0, sizeof(*list));
}

static int manager_piece_append(ManagerPieceList *list,
                                const OcrU32 *members, OcrU32 member_count,
                                OcrBounds bounds) {
    ManagerPiece *items;
    OcrU32 *copy;
    if (!member_count || !members || list->count >= 256u)
        return OCR_ERR_LIMIT;
    copy = (OcrU32 *)malloc((OcrSize)member_count * sizeof(OcrU32));
    if (!copy) return OCR_ERR_NOMEM;
    memcpy(copy, members, (OcrSize)member_count * sizeof(OcrU32));
    items = (ManagerPiece *)realloc(
        list->items, (OcrSize)(list->count + 1u) * sizeof(ManagerPiece));
    if (!items) { free(copy); return OCR_ERR_NOMEM; }
    list->items = items;
    memset(items + list->count, 0, sizeof(ManagerPiece));
    items[list->count].members = copy;
    items[list->count].member_count = member_count;
    items[list->count].bounds = bounds;
    ++list->count;
    return OCR_OK;
}

/*
 * Split over-wide lines into deterministic 20%-overlap world-space windows.
 * Rasterization clips vector segments at the window edges below, so even one
 * long cursive stroke is never squeezed below PP-OCR's 48px height contract.
 */
static int manager_plan_pieces(const OcrConfig *config,
                               const OcrStrokeView *strokes,
                               const OcrSegment *segment,
                               ManagerPieceList *out) {
    float height, available_height, available_width, max_aspect;
    float window_width, advance, start, end;
    OcrU32 *members;
    OcrU32 i, count;
    int rc;
    memset(out, 0, sizeof(*out));
    height = manager_maxf(segment->bounds.maxy - segment->bounds.miny, 0.5f);
    available_height = (float)config->raster_height -
                       (float)(config->raster_padding * 2u);
    available_width = (float)config->raster_max_width -
                      (float)(config->raster_padding * 2u);
    if (available_height < 2.0f || available_width < 2.0f)
        return OCR_ERR_INVALID;
    max_aspect = available_width / available_height;
    if ((segment->bounds.maxx - segment->bounds.minx) / height <= max_aspect) {
        rc = manager_piece_append(out, segment->members,
                                  segment->member_count, segment->bounds);
        if (rc == OCR_OK) {
            out->items[0].ordinal = 0u;
            out->items[0].piece_count = 1u;
        }
        return rc;
    }
    window_width = max_aspect * height;
    advance = window_width * 0.80f;
    if (!(window_width > 0.0f) || !(advance > 0.0f)) return OCR_ERR_LIMIT;
    members = (OcrU32 *)malloc(
        (OcrSize)segment->member_count * sizeof(OcrU32));
    if (!members) return OCR_ERR_NOMEM;
    start = segment->bounds.minx;
    while (start < segment->bounds.maxx && out->count < 256u) {
        OcrBounds piece_bounds = segment->bounds;
        end = start + window_width;
        if (end > segment->bounds.maxx) end = segment->bounds.maxx;
        count = 0;
        for (i = 0; i < segment->member_count; ++i) {
            OcrU32 member = segment->members[i];
            OcrBounds bounds;
            if (ocr_stroke_bounds(strokes + member, &bounds) == OCR_OK &&
                bounds.maxx >= start && bounds.minx <= end)
                members[count++] = member;
        }
        if (count) {
            piece_bounds.minx = start;
            piece_bounds.maxx = end;
            rc = manager_piece_append(out, members, count, piece_bounds);
            if (rc != OCR_OK) { free(members); manager_piece_list_free(out); return rc; }
        }
        if (end >= segment->bounds.maxx) break;
        start += advance;
    }
    free(members);
    if (!out->count) return OCR_ERR_INVALID;
    for (i = 0; i < out->count; ++i) {
        out->items[i].ordinal = i;
        out->items[i].piece_count = out->count;
    }
    return OCR_OK;
}

static int manager_clip_x(const OcrPoint *a, const OcrPoint *b,
                          float minimum, float maximum,
                          OcrPoint *out_a, OcrPoint *out_b) {
    float dx = b->x - a->x, t0 = 0.0f, t1 = 1.0f;
    float enter, exit, swap;
    if (manager_absf(dx) < 1.0e-12f) {
        if (a->x < minimum || a->x > maximum) return 0;
    } else {
        enter = (minimum - a->x) / dx;
        exit = (maximum - a->x) / dx;
        if (enter > exit) { swap = enter; enter = exit; exit = swap; }
        if (enter > t0) t0 = enter;
        if (exit < t1) t1 = exit;
        if (t0 > t1 || t1 < 0.0f || t0 > 1.0f) return 0;
        if (t0 < 0.0f) t0 = 0.0f;
        if (t1 > 1.0f) t1 = 1.0f;
    }
    out_a->x = a->x + dx * t0;
    out_a->y = a->y + (b->y - a->y) * t0;
    out_a->p = a->p + (b->p - a->p) * t0;
    out_b->x = a->x + dx * t1;
    out_b->y = a->y + (b->y - a->y) * t1;
    out_b->p = a->p + (b->p - a->p) * t1;
    return 1;
}

static int manager_rasterize_piece(const OcrConfig *config,
                                   const OcrStrokeView *strokes,
                                   OcrU32 stroke_count,
                                   const ManagerPiece *piece,
                                   OcrGrayImage *out) {
    OcrSegment segment;
    OcrStrokeView *clipped = 0;
    OcrPoint *points = 0;
    OcrU32 *members = 0;
    OcrU64 maximum_segments = 0;
    OcrU32 i, j, at = 0;
    int rc;
    if (piece->piece_count == 1u) {
        segment.members = piece->members;
        segment.member_count = piece->member_count;
        segment.bounds = piece->bounds;
        segment.latest_completed_ms = 0;
        return ocr_rasterize_segment(config, strokes, stroke_count,
                                     &segment, out);
    }
    for (i = 0; i < piece->member_count; ++i) {
        const OcrStrokeView *stroke = strokes + piece->members[i];
        maximum_segments += stroke->point_count > 1u
                                ? stroke->point_count - 1u : 1u;
    }
    if (!maximum_segments || maximum_segments > 2000000u)
        return OCR_ERR_LIMIT;
    clipped = (OcrStrokeView *)calloc((OcrSize)maximum_segments,
                                      sizeof(OcrStrokeView));
    points = (OcrPoint *)malloc((OcrSize)maximum_segments * 2u *
                                sizeof(OcrPoint));
    members = (OcrU32 *)malloc((OcrSize)maximum_segments * sizeof(OcrU32));
    if (!clipped || !points || !members) { rc = OCR_ERR_NOMEM; goto done; }
    for (i = 0; i < piece->member_count; ++i) {
        const OcrStrokeView *source = strokes + piece->members[i];
        if (source->point_count == 1u) {
            if (source->points[0].x >= piece->bounds.minx &&
                source->points[0].x <= piece->bounds.maxx) {
                points[at * 2u] = points[at * 2u + 1u] = source->points[0];
                clipped[at] = *source;
                clipped[at].points = points + at * 2u;
                clipped[at].point_count = 1u;
                members[at] = at;
                ++at;
            }
            continue;
        }
        for (j = 1; j < source->point_count; ++j) {
            OcrPoint a, b;
            if (!manager_clip_x(source->points + j - 1u,
                                source->points + j,
                                piece->bounds.minx, piece->bounds.maxx,
                                &a, &b)) continue;
            points[at * 2u] = a;
            points[at * 2u + 1u] = b;
            clipped[at] = *source;
            clipped[at].points = points + at * 2u;
            clipped[at].point_count = 2u;
            members[at] = at;
            ++at;
        }
    }
    if (!at) { rc = OCR_ERR_INVALID; goto done; }
    segment.members = members;
    segment.member_count = at;
    segment.bounds = piece->bounds;
    segment.latest_completed_ms = 0;
    rc = ocr_rasterize_segment(config, clipped, at, &segment, out);
done:
    if (clipped) free(clipped);
    if (points) free(points);
    if (members) free(members);
    return rc;
}

static void manager_hash_chunk(OcrU8 hash[OCR_HASH_BYTES],
                               OcrU32 ordinal, OcrU32 count) {
    OcrSha256 sha;
    OcrU8 suffix[12];
    OcrU32 marker = 0x4b4e4843u; /* CHNK in little endian. */
    suffix[0] = (OcrU8)marker; suffix[1] = (OcrU8)(marker >> 8);
    suffix[2] = (OcrU8)(marker >> 16); suffix[3] = (OcrU8)(marker >> 24);
    suffix[4] = (OcrU8)ordinal; suffix[5] = (OcrU8)(ordinal >> 8);
    suffix[6] = (OcrU8)(ordinal >> 16); suffix[7] = (OcrU8)(ordinal >> 24);
    suffix[8] = (OcrU8)count; suffix[9] = (OcrU8)(count >> 8);
    suffix[10] = (OcrU8)(count >> 16); suffix[11] = (OcrU8)(count >> 24);
    ocr_sha256_init(&sha);
    ocr_sha256_update(&sha, hash, OCR_HASH_BYTES);
    ocr_sha256_update(&sha, suffix, sizeof(suffix));
    ocr_sha256_final(&sha, hash);
}

static void manager_results_free(ManagerResult *results, OcrU32 count) {
    OcrU32 i;
    if (!results) return;
    for (i = 0; i < count; ++i) {
        if (results[i].sources) free(results[i].sources);
        if (results[i].members) free(results[i].members);
        if (results[i].text) free(results[i].text);
        if (results[i].normalized) free(results[i].normalized);
    }
    free(results);
}

static int manager_result_identity(const OcrManager *manager,
                                   const ManagerWork *work,
                                   const ManagerPiece *piece,
                                   ManagerResult *result) {
    OcrBounds ignored;
    OcrU32 i;
    int rc;
    memset(result, 0, sizeof(*result));
    result->sources = (OcrSourceRef *)malloc(
        (OcrSize)piece->member_count * sizeof(OcrSourceRef));
    result->members = (OcrI32 *)malloc(
        (OcrSize)piece->member_count * sizeof(OcrI32));
    if (!result->sources || !result->members) return OCR_ERR_NOMEM;
    rc = ocr_region_identity(&manager->config, work->strokes, piece->members,
                             piece->member_count, result->source_hash,
                             result->sources, piece->member_count, &ignored);
    if (rc != OCR_OK) return rc;
    if (piece->piece_count > 1u)
        manager_hash_chunk(result->source_hash, piece->ordinal,
                           piece->piece_count);
    for (i = 0; i < piece->member_count; ++i)
        result->members[i] =
            work->strokes[piece->members[i]].runtime_index;
    result->source_count = piece->member_count;
    result->member_count = piece->member_count;
    result->bounds = piece->bounds;
    return OCR_OK;
}

static double manager_bounds_distance2(const OcrBounds *a,
                                       const OcrBounds *b) {
    double ax = ((double)a->minx + a->maxx) * 0.5;
    double ay = ((double)a->miny + a->maxy) * 0.5;
    double bx = ((double)b->minx + b->maxx) * 0.5;
    double by = ((double)b->miny + b->maxy) * 0.5;
    double dx = ax - bx, dy = ay - by;
    return dx * dx + dy * dy;
}

/* Select inactive cache first; an active identical record is a clone source. */
static void manager_select_cache_locked(OcrManager *manager,
                                        ManagerResult *result,
                                        const OcrU64 *used, OcrU32 used_count) {
    OcrU32 i;
    OcrI32 best_inactive = -1, best_active = -1;
    double inactive_distance = 0.0, active_distance = 0.0;
    for (i = 0; i < manager->index.count; ++i) {
        const OcrIndexRecord *record = manager->index.records + i;
        double distance;
        if (memcmp(record->source_hash, result->source_hash,
                   OCR_HASH_BYTES) != 0 ||
            manager_u64_contains(used, used_count, record->occurrence_id))
            continue;
        distance = manager_bounds_distance2(&record->bounds, &result->bounds);
        if (!(record->flags & OCR_RECORD_ACTIVE)) {
            if (best_inactive < 0 || distance < inactive_distance) {
                best_inactive = (OcrI32)i;
                inactive_distance = distance;
            }
        } else if (best_active < 0 || distance < active_distance) {
            best_active = (OcrI32)i;
            active_distance = distance;
        }
    }
    if (best_inactive >= 0)
        result->reuse_id =
            manager->index.records[(OcrU32)best_inactive].occurrence_id;
    else if (best_active >= 0)
        result->clone_id =
            manager->index.records[(OcrU32)best_active].occurrence_id;
}

#if defined(__ANDROID__)
static int manager_read_asset(OcrManager *manager, const char *path,
                              OcrU8 **out_bytes, OcrSize *out_size,
                              OcrSize maximum) {
    AAsset *asset;
    OcrU8 *bytes;
    long length, at = 0;
    if (!manager->asset_manager || !path || !out_bytes || !out_size)
        return OCR_ERR_INVALID;
    asset = AAssetManager_open((AAssetManager *)manager->asset_manager, path,
                               MANAGER_AASSET_MODE_BUFFER);
    if (!asset) return OCR_ERR_IO;
    length = AAsset_getLength(asset);
    if (length <= 0 || (OcrSize)length > maximum) {
        AAsset_close(asset);
        return OCR_ERR_LIMIT;
    }
    bytes = (OcrU8 *)malloc((OcrSize)length);
    if (!bytes) { AAsset_close(asset); return OCR_ERR_NOMEM; }
    while (at < length) {
        int got = AAsset_read(asset, bytes + at, (OcrSize)(length - at));
        if (got <= 0) { free(bytes); AAsset_close(asset); return OCR_ERR_IO; }
        at += got;
    }
    AAsset_close(asset);
    *out_bytes = bytes;
    *out_size = (OcrSize)length;
    return OCR_OK;
}
#endif

static int manager_ensure_recognizer(OcrManager *manager,
                                     char error[OCR_MANAGER_ERROR_BYTES]) {
    OcrU8 *model = 0, *dictionary = 0;
    OcrSize model_size = 0, dictionary_size = 0;
    if (manager->recognize || manager->recognizer) return OCR_OK;
#if defined(__ANDROID__)
    int rc;
    rc = manager_read_asset(manager, manager->model_asset, &model,
                            &model_size, 128u * 1024u * 1024u);
    if (rc != OCR_OK) {
        manager_copy_string(error, OCR_MANAGER_ERROR_BYTES,
                            "Unable to read bundled OCR model asset");
        return rc;
    }
    rc = manager_read_asset(manager, manager->dictionary_asset, &dictionary,
                            &dictionary_size, 4u * 1024u * 1024u);
    if (rc != OCR_OK) {
        free(model);
        manager_copy_string(error, OCR_MANAGER_ERROR_BYTES,
                            "Unable to read bundled OCR dictionary asset");
        return rc;
    }
    rc = ppocr_recognizer_init(&manager->recognizer, model, model_size,
                               dictionary, dictionary_size);
    free(model);
    free(dictionary);
    if (rc != PPOCR_OK) {
        manager_copy_string(error, OCR_MANAGER_ERROR_BYTES,
                            "Unable to initialize bundled PP-OCR resources");
        return rc;
    }
    return OCR_OK;
#else
    (void)model; (void)dictionary; (void)model_size; (void)dictionary_size;
    manager_copy_string(error, OCR_MANAGER_ERROR_BYTES,
                        "PP-OCR assets require Android AAssetManager");
    return PPOCR_ERR_MODEL;
#endif
}

static int manager_run_recognition(OcrManager *manager,
                                   const OcrGrayImage *image,
                                   ManagerResult *result,
                                   PpocrTiming *timing,
                                   char error[OCR_MANAGER_ERROR_BYTES]) {
    char *text;
    OcrU32 length = 0;
    float confidence = 0.0f;
    int rc;
    text = (char *)malloc(PPOCR_MAX_TEXT_BYTES + 1u);
    if (!text) return OCR_ERR_NOMEM;
    memset(timing, 0, sizeof(*timing));
    if (manager->recognize) {
        rc = manager->recognize(manager->recognize_context, image, text,
                                PPOCR_MAX_TEXT_BYTES + 1u, &length,
                                &confidence, timing);
        if (rc != OCR_OK && rc != PPOCR_OK) {
            free(text);
            manager_copy_string(error, OCR_MANAGER_ERROR_BYTES,
                                "OCR callback failed");
            return rc;
        }
    } else {
        PpocrImage input;
        PpocrResult output;
        rc = manager_ensure_recognizer(manager, error);
        if (rc != OCR_OK && rc != PPOCR_OK) { free(text); return rc; }
        memset(&input, 0, sizeof(input));
        memset(&output, 0, sizeof(output));
        input.pixels = image->pixels;
        input.width = image->width;
        input.height = image->height;
        input.stride = image->stride;
        input.format = PPOCR_PIXELS_GRAY8;
        rc = ppocr_recognizer_recognize(manager->recognizer, &input, &output);
        if (rc != PPOCR_OK) {
            const char *detail = ppocr_recognizer_last_error(manager->recognizer);
            free(text);
            manager_copy_string(error, OCR_MANAGER_ERROR_BYTES,
                                detail && detail[0] ? detail
                                    : "PP-OCR recognition failed");
            ppocr_result_release(&output);
            return rc;
        }
        if (output.text_bytes > PPOCR_MAX_TEXT_BYTES) {
            free(text); ppocr_result_release(&output); return OCR_ERR_LIMIT;
        }
        length = output.text_bytes;
        if (length) memcpy(text, output.text, length);
        confidence = output.confidence;
        *timing = output.timing;
        ppocr_result_release(&output);
    }
    if (length > PPOCR_MAX_TEXT_BYTES || !ocr_utf8_valid(text, length) ||
        !manager_finite(confidence) || confidence < 0.0f || confidence > 1.0f) {
        free(text);
        manager_copy_string(error, OCR_MANAGER_ERROR_BYTES,
                            "OCR returned invalid text or confidence");
        return OCR_ERR_INVALID;
    }
    text[length] = 0;
    result->text = text;
    result->text_len = length;
    result->confidence = confidence;
    result->recognized = 1;
    return OCR_OK;
}

static int manager_normalize_result(void *jni_env, ManagerResult *result) {
    int rc = ocr_unicode_normalize_android(jni_env, result->text,
                                           result->text_len,
                                           &result->normalized,
                                           &result->normalized_len);
    if (rc != OCR_OK && jni_env)
        rc = ocr_normalize_basic(result->text, result->text_len,
                                 &result->normalized,
                                 &result->normalized_len);
    return rc;
}

static int manager_should_defer_locked(OcrManager *manager) {
    OcrI64 now = manager_now_ms();
    return manager->interaction_active || now < manager->defer_until_ms ||
           now < manager->recognize_not_before_ms;
}

static void manager_add_ppocr_timing(PpocrTiming *total,
                                     const PpocrTiming *part) {
    total->session_setup_us += part->session_setup_us;
    total->preprocess_us += part->preprocess_us;
    total->inference_us += part->inference_us;
    total->decode_us += part->decode_us;
    total->total_us += part->total_us;
    if (part->input_width > total->input_width)
        total->input_width = part->input_width;
    total->output_timesteps += part->output_timesteps;
}

static int manager_append_result(ManagerResult **items, OcrU32 *count,
                                 ManagerResult *value) {
    ManagerResult *next = (ManagerResult *)realloc(
        *items, (OcrSize)(*count + 1u) * sizeof(ManagerResult));
    if (!next) return OCR_ERR_NOMEM;
    *items = next;
    next[*count] = *value;
    memset(value, 0, sizeof(*value));
    ++*count;
    return OCR_OK;
}

static int manager_commit_work_locked(OcrManager *manager,
                                      const ManagerWork *work,
                                      ManagerResult *results,
                                      OcrU32 result_count,
                                      OcrManagerTiming *timing) {
    OcrU64 update_start = manager_now_us();
    OcrU32 i;
    int rc = OCR_OK;
    timing->discarded_stale_jobs +=
        manager->last_timing.discarded_stale_jobs;
    for (i = 0; i < work->touched_count; ++i)
        manager_remove_region_id_locked(manager, work->touched[i]);
    for (i = 0; i < result_count; ++i) {
        ManagerResult *result = results + i;
        OcrIndexRecord view;
        const OcrIndexRecord *source = 0;
        OcrU64 occurrence_id;
        memset(&view, 0, sizeof(view));
        if (result->reuse_id)
            source = ocr_index_find(&manager->index, result->reuse_id);
        else if (result->clone_id)
            source = ocr_index_find(&manager->index, result->clone_id);
        if (source) {
            view = *source;
            occurrence_id = result->reuse_id
                                ? result->reuse_id
                                : manager->index.next_occurrence_id;
            view.occurrence_id = occurrence_id;
            memcpy(view.source_hash, result->source_hash, OCR_HASH_BYTES);
            view.bounds = result->bounds;
            view.flags |= OCR_RECORD_ACTIVE;
            view.sources = result->sources;
            view.source_count = result->source_count;
        } else if (result->recognized) {
            occurrence_id = manager->index.next_occurrence_id;
            view.occurrence_id = occurrence_id;
            memcpy(view.source_hash, result->source_hash, OCR_HASH_BYTES);
            view.bounds = result->bounds;
            view.confidence = result->confidence;
            view.flags = OCR_RECORD_ACTIVE;
            view.sources = result->sources;
            view.source_count = result->source_count;
            view.original_text = result->text;
            view.original_len = result->text_len;
            view.normalized_text = result->normalized;
            view.normalized_len = result->normalized_len;
        } else {
            continue;
        }
        rc = ocr_index_put(&manager->index, &view);
        if (rc != OCR_OK) break;
        rc = manager_set_region_locked(manager, occurrence_id,
                                       result->members, result->member_count,
                                       result->bounds);
        if (rc != OCR_OK) {
            (void)ocr_index_set_active(&manager->index, occurrence_id, 0);
            break;
        }
    }
    timing->index_update_us =
        manager_elapsed_us(update_start, manager_now_us());
    if (rc == OCR_OK) {
        manager->checkpoint_pending = manager->sidecar_path[0] ? 1 : 0;
        manager->rebuilding = 0;
        manager_set_error_locked(manager, "");
    } else {
        manager_set_error_locked(manager, "Unable to update OCR index");
        manager->state = OCR_MANAGER_MODEL_FAILED;
    }
    manager->last_timing = *timing;
    manager->running_job = 0;
    if (rc == OCR_OK) {
        if (manager->dirty_all || manager->dirty_count)
            manager->state = manager->rebuilding ? OCR_MANAGER_REBUILDING
                                                 : OCR_MANAGER_INDEXING;
        else
            manager->state = manager->enabled ? OCR_MANAGER_READY
                                              : OCR_MANAGER_DISABLED;
    }
    manager_notify_locked(manager, OCR_MANAGER_EVENT_INDEX |
                                   OCR_MANAGER_EVENT_STATUS);
    (void)pthread_cond_broadcast(&manager->cond);
    return rc;
}

static int manager_process_work(OcrManager *manager, ManagerWork *work,
                                void *jni_env,
                                char error[OCR_MANAGER_ERROR_BYTES]) {
    OcrSegmentList segments;
    ManagerResult *results = 0;
    OcrU64 *used_ids = 0;
    OcrU32 result_count = 0, used_count = 0;
    OcrManagerTiming timing;
    OcrU64 total_start = manager_now_us(), stage_start;
    OcrU32 i, j;
    int rc, stale = 0;
    memset(&segments, 0, sizeof(segments));
    memset(&timing, 0, sizeof(timing));
    if (!work->stroke_count) {
        pthread_mutex_lock(&manager->mutex);
        if (!manager_work_current_locked(manager, work) ||
            manager_should_defer_locked(manager)) {
            stale = 1;
            rc = OCR_ERR_NOT_FOUND;
        } else {
            timing.total_us = manager_elapsed_us(total_start,
                                                 manager_now_us());
            rc = manager_commit_work_locked(manager, work, 0, 0, &timing);
        }
        pthread_mutex_unlock(&manager->mutex);
        goto done;
    }
    stage_start = manager_now_us();
    rc = ocr_segment_world(&manager->config, work->strokes,
                           work->stroke_count, &segments);
    timing.segmentation_us =
        manager_elapsed_us(stage_start, manager_now_us());
    if (rc != OCR_OK) goto done;
    for (i = 0; i < segments.count; ++i) {
        ManagerPieceList pieces;
        rc = manager_plan_pieces(&manager->config, work->strokes,
                                 segments.items + i, &pieces);
        if (rc != OCR_OK) goto done;
        for (j = 0; j < pieces.count; ++j) {
            ManagerResult result;
            OcrGrayImage image;
            PpocrTiming recognition_timing;
            OcrU64 raster_start;
            memset(&result, 0, sizeof(result));
            memset(&image, 0, sizeof(image));
            rc = manager_result_identity(manager, work, pieces.items + j,
                                         &result);
            if (rc != OCR_OK) {
                if (result.sources) free(result.sources);
                if (result.members) free(result.members);
                manager_piece_list_free(&pieces);
                goto done;
            }
            pthread_mutex_lock(&manager->mutex);
            if (!manager_work_current_locked(manager, work) ||
                manager_should_defer_locked(manager)) {
                stale = 1;
            } else {
                manager_select_cache_locked(manager, &result,
                                            used_ids, used_count);
            }
            pthread_mutex_unlock(&manager->mutex);
            if (stale) {
                if (result.sources) free(result.sources);
                if (result.members) free(result.members);
                manager_piece_list_free(&pieces);
                rc = OCR_ERR_NOT_FOUND;
                goto done;
            }
            if (result.reuse_id || result.clone_id) {
                OcrU64 selected_id = result.reuse_id
                                         ? result.reuse_id : result.clone_id;
                OcrU64 *next_used = (OcrU64 *)realloc(
                    used_ids, (OcrSize)(used_count + 1u) * sizeof(OcrU64));
                if (!next_used) {
                    if (result.sources) free(result.sources);
                    if (result.members) free(result.members);
                    manager_piece_list_free(&pieces);
                    rc = OCR_ERR_NOMEM;
                    goto done;
                }
                used_ids = next_used;
                used_ids[used_count++] = selected_id;
                ++timing.cache_hits;
            } else {
                raster_start = manager_now_us();
                rc = manager_rasterize_piece(&manager->config, work->strokes,
                                             work->stroke_count,
                                             pieces.items + j, &image);
                timing.rasterization_us +=
                    manager_elapsed_us(raster_start, manager_now_us());
                if (rc == OCR_OK)
                    rc = manager_run_recognition(manager, &image, &result,
                                                 &recognition_timing, error);
                ocr_gray_image_free(&image);
                if (rc == OCR_OK)
                    rc = manager_normalize_result(jni_env, &result);
                if (rc != OCR_OK) {
                    if (result.sources) free(result.sources);
                    if (result.members) free(result.members);
                    if (result.text) free(result.text);
                    if (result.normalized) free(result.normalized);
                    manager_piece_list_free(&pieces);
                    goto done;
                }
                manager_add_ppocr_timing(&timing.recognition,
                                         &recognition_timing);
                ++timing.recognized_regions;
            }
            ++timing.candidate_regions;
            rc = manager_append_result(&results, &result_count, &result);
            if (rc != OCR_OK) {
                if (result.sources) free(result.sources);
                if (result.members) free(result.members);
                if (result.text) free(result.text);
                if (result.normalized) free(result.normalized);
                manager_piece_list_free(&pieces);
                goto done;
            }
        }
        manager_piece_list_free(&pieces);
    }
    pthread_mutex_lock(&manager->mutex);
    if (!manager_work_current_locked(manager, work) ||
        manager_should_defer_locked(manager)) {
        stale = 1;
        rc = OCR_ERR_NOT_FOUND;
    } else {
        timing.total_us = manager_elapsed_us(total_start, manager_now_us());
        rc = manager_commit_work_locked(manager, work, results,
                                        result_count, &timing);
        manager->session_ready = manager->recognizer
            ? ppocr_recognizer_session_ready(manager->recognizer) :
              (manager->recognize ? 1 : 0);
    }
    pthread_mutex_unlock(&manager->mutex);
done:
    if (stale) {
        pthread_mutex_lock(&manager->mutex);
        ++manager->last_timing.discarded_stale_jobs;
        manager->running_job = 0;
        manager_requeue_work_locked(manager, work);
        manager_notify_locked(manager, OCR_MANAGER_EVENT_STATUS);
        (void)pthread_cond_broadcast(&manager->cond);
        pthread_mutex_unlock(&manager->mutex);
    } else if (rc != OCR_OK) {
        pthread_mutex_lock(&manager->mutex);
        if (!manager_work_current_locked(manager, work)) {
            manager->running_job = 0;
            if (manager->project_open &&
                manager->project_generation == work->project_generation) {
                ++manager->last_timing.discarded_stale_jobs;
                manager_requeue_work_locked(manager, work);
            }
            manager_notify_locked(manager, OCR_MANAGER_EVENT_STATUS);
            (void)pthread_cond_broadcast(&manager->cond);
        } else {
            manager->running_job = 0;
            manager->state = OCR_MANAGER_MODEL_FAILED;
            manager_set_error_locked(manager,
                error[0] ? error : "OCR background job failed");
            manager_notify_locked(manager, OCR_MANAGER_EVENT_STATUS);
            (void)pthread_cond_broadcast(&manager->cond);
        }
        pthread_mutex_unlock(&manager->mutex);
    }
    if (used_ids) free(used_ids);
    manager_results_free(results, result_count);
    ocr_segment_list_free(&segments);
    return rc;
}

static void manager_load_sidecar(OcrManager *manager,
                                 OcrU64 project_generation,
                                 OcrU64 project_key,
                                 const char *path) {
    OcrIndex loaded;
    OcrSidecarStatus status;
    OcrU32 i;
    ocr_index_init(&loaded, project_key);
    if (path && path[0])
        status = ocr_sidecar_load(path, manager->config.index_version,
                                  manager->model_hash, project_key, &loaded);
    else
        status = OCR_SIDECAR_MISSING;
    pthread_mutex_lock(&manager->mutex);
    if (manager->project_open &&
        manager->project_generation == project_generation &&
        manager->project_key == project_key &&
        manager->sidecar_load_pending) {
        ocr_index_free(&manager->index);
        if (status == OCR_SIDECAR_OK) {
            manager->index = loaded;
            memset(&loaded, 0, sizeof(loaded));
            for (i = 0; i < manager->index.count; ++i)
                manager->index.records[i].flags &= ~OCR_RECORD_ACTIVE;
        } else {
            ocr_index_init(&manager->index, project_key);
        }
        manager_free_regions(manager);
        manager->sidecar_status = status;
        manager->sidecar_load_pending = 0;
        manager->sidecar_loaded = 1;
        if (manager->enabled && manager->initial_scan_finished) {
            manager_queue_dirty_locked(manager, (OcrBounds){0,0,0,0}, 1);
        } else {
            manager->state = manager->enabled ? OCR_MANAGER_INDEXING
                                              : OCR_MANAGER_DISABLED;
        }
        manager_notify_locked(manager, OCR_MANAGER_EVENT_INDEX |
                                       OCR_MANAGER_EVENT_STATUS);
    }
    pthread_mutex_unlock(&manager->mutex);
    ocr_index_free(&loaded);
}

static void manager_checkpoint_worker(OcrManager *manager) {
    OcrIndex snapshot;
    OcrU64 project_generation;
    char path[MANAGER_PATH_BYTES];
    int cloned = 0;
    OcrSidecarStatus status = OCR_SIDECAR_IO_ERROR;
    memset(&snapshot, 0, sizeof(snapshot));
    pthread_mutex_lock(&manager->mutex);
    if (manager->project_open && manager->sidecar_path[0] &&
        manager->checkpoint_pending) {
        project_generation = manager->project_generation;
        manager_copy_string(path, MANAGER_PATH_BYTES, manager->sidecar_path);
        if (manager_clone_index(&manager->index, manager->project_key,
                                &snapshot) == OCR_OK) {
            manager->checkpoint_pending = 0;
            manager->checkpoint_running = 1;
            cloned = 1;
        } else {
            manager->checkpoint_pending = 0;
            manager_set_error_locked(manager,
                                     "Unable to snapshot OCR sidecar");
            manager_notify_locked(manager, OCR_MANAGER_EVENT_CHECKPOINT |
                                           OCR_MANAGER_EVENT_STATUS);
        }
    }
    pthread_mutex_unlock(&manager->mutex);
    if (!cloned) return;
    status = ocr_sidecar_save_atomic(path, &snapshot,
                                     manager->config.index_version,
                                     manager->model_hash);
    ocr_index_free(&snapshot);
    pthread_mutex_lock(&manager->mutex);
    manager->checkpoint_running = 0;
    (void)pthread_cond_broadcast(&manager->cond);
    if (manager->project_open &&
        manager->project_generation == project_generation) {
        manager->sidecar_status = status;
        if (status != OCR_SIDECAR_OK)
            manager_set_error_locked(manager,
                                     "Unable to checkpoint OCR sidecar");
        manager_notify_locked(manager, OCR_MANAGER_EVENT_CHECKPOINT |
                                       OCR_MANAGER_EVENT_STATUS);
    }
    pthread_mutex_unlock(&manager->mutex);
}

static void *manager_worker_main(void *opaque) {
    OcrManager *manager = (OcrManager *)opaque;
    void *jni_env = 0;
    int attached = 0;
#if defined(__ANDROID__)
    JavaVM *vm = (JavaVM *)manager->java_vm;
    (void)setpriority(0, 0, 10);
    if (vm) {
        if ((*vm)->GetEnv(vm, &jni_env, JNI_VERSION_1_6) != JNI_OK) {
            if ((*vm)->AttachCurrentThread(vm, &jni_env, 0) == JNI_OK)
                attached = 1;
            else
                jni_env = 0;
        }
    }
#elif !defined(_WIN32)
    (void)setpriority(PRIO_PROCESS, 0, 10);
#endif
    for (;;) {
        int action = MANAGER_JOB_NONE;
        OcrU64 sidecar_generation = 0, sidecar_key = 0;
        char sidecar_path[MANAGER_PATH_BYTES];
        ManagerWork work;
        char error[OCR_MANAGER_ERROR_BYTES];
        memset(&work, 0, sizeof(work));
        sidecar_path[0] = 0;
        error[0] = 0;
        pthread_mutex_lock(&manager->mutex);
        for (;;) {
            OcrI64 now, deadline;
            if (manager->stopping) break;
            if (manager->project_open && manager->sidecar_load_pending) {
                action = MANAGER_JOB_SIDECAR;
                sidecar_generation = manager->project_generation;
                sidecar_key = manager->project_key;
                manager_copy_string(sidecar_path, MANAGER_PATH_BYTES,
                                    manager->sidecar_path);
                break;
            }
            if (manager->project_open && manager->checkpoint_pending &&
                !manager->interaction_active) {
                now = manager_now_ms();
                if (now >= manager->defer_until_ms) {
                    action = MANAGER_JOB_CHECKPOINT;
                    break;
                }
            }
            if (manager->project_open && manager->enabled &&
                manager->initial_scan_finished &&
                (manager->dirty_all || manager->dirty_count) &&
                manager->state != OCR_MANAGER_MODEL_FAILED) {
                if (manager->interaction_active) {
                    (void)pthread_cond_wait(&manager->cond, &manager->mutex);
                    continue;
                }
                now = manager_now_ms();
                deadline = manager->recognize_not_before_ms;
                if (manager->defer_until_ms > deadline)
                    deadline = manager->defer_until_ms;
                if (now < deadline) {
                    manager_wait_until_locked(manager, deadline);
                    continue;
                }
                if (manager_take_work_locked(manager, &work) == OCR_OK)
                    action = MANAGER_JOB_RECOGNIZE;
                break;
            }
            (void)pthread_cond_wait(&manager->cond, &manager->mutex);
        }
        if (manager->stopping) {
            pthread_mutex_unlock(&manager->mutex);
            break;
        }
        pthread_mutex_unlock(&manager->mutex);
        if (action == MANAGER_JOB_SIDECAR)
            manager_load_sidecar(manager, sidecar_generation, sidecar_key,
                                 sidecar_path);
        else if (action == MANAGER_JOB_CHECKPOINT)
            manager_checkpoint_worker(manager);
        else if (action == MANAGER_JOB_RECOGNIZE) {
            (void)manager_process_work(manager, &work, jni_env, error);
            manager_work_free(&work);
        }
    }
#if defined(__ANDROID__)
    if (attached && manager->java_vm) {
        JavaVM *vm = (JavaVM *)manager->java_vm;
        (void)(*vm)->DetachCurrentThread(vm);
    }
#else
    (void)attached;
#endif
    return 0;
}

static int manager_start_worker_locked(OcrManager *manager) {
    int rc;
    if (manager->worker_started) return OCR_OK;
    rc = pthread_create(&manager->worker, 0, manager_worker_main, manager);
    if (rc != 0) {
        manager->state = OCR_MANAGER_MODEL_FAILED;
        manager_set_error_locked(manager, "Unable to start OCR worker");
        manager_notify_locked(manager, OCR_MANAGER_EVENT_STATUS);
        return OCR_ERR_IO;
    }
    manager->worker_started = 1;
    return OCR_OK;
}

static int manager_path_valid(const char *path, OcrU32 capacity) {
    OcrU32 i = 0;
    if (!path) return 1;
    while (path[i]) {
        if (i + 1u >= capacity) return 0;
        ++i;
    }
    return 1;
}

int ocr_manager_create(OcrManager **out_manager,
                       const OcrManagerCreateInfo *info) {
    OcrManager *manager;
    int mutex_ready = 0, cond_ready = 0;
    if (!out_manager) return OCR_ERR_INVALID;
    *out_manager = 0;
    manager = (OcrManager *)calloc(1u, sizeof(OcrManager));
    if (!manager) return OCR_ERR_NOMEM;
    manager->event_read_fd = manager->event_write_fd = -1;
    if (pthread_mutex_init(&manager->mutex, 0) != 0) goto fail;
    mutex_ready = 1;
    if (pthread_cond_init(&manager->cond, 0) != 0) goto fail;
    cond_ready = 1;
    if (!manager_create_event(manager)) goto fail;
    if (info && info->config) manager->config = *info->config;
    else ocr_config_default(&manager->config);
    if (!manager->config.index_version || !manager->config.identity_version ||
        manager->config.idle_delay_ms < 0 ||
        !manager_finite(manager->config.search_min_confidence) ||
        manager->config.search_min_confidence < 0.0f ||
        manager->config.search_min_confidence > 1.0f ||
        manager->config.raster_height < 8u ||
        manager->config.raster_max_width < manager->config.raster_height ||
        manager->config.raster_padding * 2u + 2u >=
            manager->config.raster_height) goto fail;
    manager->asset_manager = info ? info->asset_manager : 0;
    manager->java_vm = info ? info->java_vm : 0;
    manager->recognize = info ? info->recognize : 0;
    manager->recognize_context = info ? info->recognize_context : 0;
    manager_copy_string(manager->model_asset, MANAGER_ASSET_PATH_BYTES,
        info && info->model_asset_path ? info->model_asset_path
                                      : VAST_OCR_MODEL_ASSET);
    manager_copy_string(manager->dictionary_asset, MANAGER_ASSET_PATH_BYTES,
        info && info->dictionary_asset_path ? info->dictionary_asset_path
                                           : VAST_OCR_DICTIONARY_ASSET);
    if (info && info->model_hash)
        memcpy(manager->model_hash, info->model_hash, OCR_MODEL_HASH_BYTES);
    else
        memcpy(manager->model_hash, manager_default_model_hash,
               OCR_MODEL_HASH_BYTES);
    manager->enabled = info ? (info->initially_enabled != 0) : 1;
    manager->state = manager->enabled ? OCR_MANAGER_READY
                                      : OCR_MANAGER_DISABLED;
    manager->sidecar_status = OCR_SIDECAR_MISSING;
    manager->project_generation = 1u;
    manager->document_generation = 1u;
    manager->job_generation = 1u;
    ocr_index_init(&manager->index, 0u);
    *out_manager = manager;
    return OCR_OK;
fail:
    manager_close_event(manager);
    if (cond_ready) (void)pthread_cond_destroy(&manager->cond);
    if (mutex_ready) (void)pthread_mutex_destroy(&manager->mutex);
    free(manager);
    return OCR_ERR_INVALID;
}

int ocr_manager_close_project(OcrManager *manager, int checkpoint) {
    OcrSidecarStatus saved = OCR_SIDECAR_OK;
    if (!manager) return OCR_ERR_INVALID;
    pthread_mutex_lock(&manager->mutex);
    if (!manager->project_open) {
        pthread_mutex_unlock(&manager->mutex);
        return OCR_OK;
    }
    /* A non-checkpointing project switch may proceed while the worker writes
       its already deep-copied snapshot.  Shutdown/explicit durable close still
       waits before taking and saving the final in-memory index. */
    while (checkpoint && manager->checkpoint_running)
        (void)pthread_cond_wait(&manager->cond, &manager->mutex);
    if (checkpoint && manager->sidecar_path[0]) {
        saved = ocr_sidecar_save_atomic(manager->sidecar_path,
                                        &manager->index,
                                        manager->config.index_version,
                                        manager->model_hash);
    }
    manager->project_open = 0;
    ++manager->project_generation;
    ++manager->document_generation;
    ++manager->job_generation;
    manager->sidecar_load_pending = 0;
    manager->checkpoint_pending = 0;
    manager->dirty_count = 0;
    manager->dirty_all = 0;
    manager->initial_scan_finished = 0;
    manager->running_job = 0;
    manager_free_strokes(manager);
    manager_free_regions(manager);
    ocr_index_free(&manager->index);
    ocr_index_init(&manager->index, 0u);
    manager->project_key = 0u;
    manager->sidecar_path[0] = 0;
    manager->state = manager->enabled ? OCR_MANAGER_READY
                                      : OCR_MANAGER_DISABLED;
    manager_notify_locked(manager, OCR_MANAGER_EVENT_INDEX |
                                   OCR_MANAGER_EVENT_STATUS |
                                   (checkpoint ? OCR_MANAGER_EVENT_CHECKPOINT : 0u));
    (void)pthread_cond_broadcast(&manager->cond);
    pthread_mutex_unlock(&manager->mutex);
    return saved == OCR_SIDECAR_OK ? OCR_OK : OCR_ERR_IO;
}

void ocr_manager_shutdown(OcrManager *manager) {
    if (!manager) return;
    (void)ocr_manager_close_project(manager, 1);
    pthread_mutex_lock(&manager->mutex);
    manager->stopping = 1;
    (void)pthread_cond_broadcast(&manager->cond);
    pthread_mutex_unlock(&manager->mutex);
    if (manager->worker_started) (void)pthread_join(manager->worker, 0);
    if (manager->recognizer) ppocr_recognizer_destroy(manager->recognizer);
    manager_free_strokes(manager);
    manager_free_regions(manager);
    ocr_index_free(&manager->index);
    manager_close_event(manager);
    (void)pthread_cond_destroy(&manager->cond);
    (void)pthread_mutex_destroy(&manager->mutex);
    free(manager);
}

int ocr_manager_open_project(OcrManager *manager, OcrU64 project_key,
                             const char *sidecar_path) {
    int rc;
    if (!manager || !project_key ||
        !manager_path_valid(sidecar_path, MANAGER_PATH_BYTES))
        return OCR_ERR_INVALID;
    (void)ocr_manager_close_project(manager, 1);
    pthread_mutex_lock(&manager->mutex);
    manager->project_open = 1;
    manager->project_key = project_key;
    ++manager->project_generation;
    ++manager->document_generation;
    ++manager->job_generation;
    manager_copy_string(manager->sidecar_path, MANAGER_PATH_BYTES,
                        sidecar_path ? sidecar_path : "");
    ocr_index_free(&manager->index);
    ocr_index_init(&manager->index, project_key);
    manager->initial_scan_finished = 0;
    manager->sidecar_loaded = manager->sidecar_path[0] ? 0 : 1;
    manager->sidecar_load_pending = manager->sidecar_path[0] ? 1 : 0;
    manager->checkpoint_pending = 0;
    manager->sidecar_status = OCR_SIDECAR_MISSING;
    manager->state = manager->enabled
        ? (manager->sidecar_load_pending ? OCR_MANAGER_LOADING_CACHE
                                         : OCR_MANAGER_INDEXING)
        : OCR_MANAGER_DISABLED;
    manager_set_error_locked(manager, "");
    rc = manager->sidecar_load_pending
             ? manager_start_worker_locked(manager) : OCR_OK;
    (void)pthread_cond_signal(&manager->cond);
    manager_notify_locked(manager, OCR_MANAGER_EVENT_STATUS |
                                   OCR_MANAGER_EVENT_INDEX);
    pthread_mutex_unlock(&manager->mutex);
    return rc;
}

int ocr_manager_clone_project_cache(OcrManager *manager,
                                    OcrU64 destination_project_key,
                                    const char *destination_sidecar_path) {
    OcrIndex snapshot;
    OcrSidecarStatus status;
    int rc;
    if (!manager || !destination_project_key ||
        !destination_sidecar_path || !destination_sidecar_path[0] ||
        !manager_path_valid(destination_sidecar_path, MANAGER_PATH_BYTES))
        return OCR_ERR_INVALID;
    memset(&snapshot, 0, sizeof(snapshot));
    pthread_mutex_lock(&manager->mutex);
    if (!manager->project_open) {
        pthread_mutex_unlock(&manager->mutex);
        return OCR_ERR_INVALID;
    }
    rc = manager_clone_index(&manager->index, destination_project_key,
                             &snapshot);
    pthread_mutex_unlock(&manager->mutex);
    if (rc != OCR_OK) return rc;
    status = ocr_sidecar_save_atomic(destination_sidecar_path, &snapshot,
                                     manager->config.index_version,
                                     manager->model_hash);
    ocr_index_free(&snapshot);
    return status == OCR_SIDECAR_OK ? OCR_OK : OCR_ERR_IO;
}

int ocr_manager_add_initial_strokes(OcrManager *manager,
                                    const OcrManagerStroke *strokes,
                                    OcrU32 stroke_count) {
    ManagerStroke *copies = 0;
    OcrU32 i;
    int rc = OCR_OK;
    if (!manager || (!strokes && stroke_count) ||
        stroke_count > MANAGER_MAX_STROKES) return OCR_ERR_INVALID;
    if (stroke_count) {
        copies = (ManagerStroke *)calloc(stroke_count, sizeof(ManagerStroke));
        if (!copies) return OCR_ERR_NOMEM;
        for (i = 0; i < stroke_count; ++i) {
            rc = manager_copy_input_stroke(strokes + i, copies + i);
            if (rc != OCR_OK) goto done;
        }
    }
    pthread_mutex_lock(&manager->mutex);
    if (!manager->project_open || manager->initial_scan_finished) {
        rc = OCR_ERR_INVALID;
    } else if (manager_reserve_strokes(
                   manager, manager->stroke_count + stroke_count) != OCR_OK) {
        rc = OCR_ERR_NOMEM;
    } else {
        for (i = 0; i < stroke_count; ++i) {
            OcrI32 existing = manager_find_stroke_slot(
                manager, copies[i].runtime_index);
            if (existing >= 0 &&
                manager->strokes[(OcrU32)existing].initial_authoritative)
                continue;
            rc = manager_insert_stroke_locked(manager, copies + i, 0, 0);
            if (rc != OCR_OK) break;
        }
        if (rc == OCR_OK && stroke_count) ++manager->document_generation;
    }
    pthread_mutex_unlock(&manager->mutex);
done:
    if (copies) {
        for (i = 0; i < stroke_count; ++i) manager_free_stroke(copies + i);
        free(copies);
    }
    return rc;
}

int ocr_manager_finish_initial_scan(OcrManager *manager) {
    int rc;
    if (!manager) return OCR_ERR_INVALID;
    pthread_mutex_lock(&manager->mutex);
    if (!manager->project_open) {
        pthread_mutex_unlock(&manager->mutex);
        return OCR_ERR_INVALID;
    }
    if (manager->initial_scan_finished) {
        pthread_mutex_unlock(&manager->mutex);
        return OCR_OK;
    }
    manager->initial_scan_finished = 1;
    {
        OcrU32 i;
        for (i = 0; i < manager->stroke_count; ++i)
            manager->strokes[i].initial_authoritative = 0u;
    }
    rc = manager_start_worker_locked(manager);
    if (rc == OCR_OK && manager->sidecar_loaded && manager->enabled)
        manager_queue_dirty_locked(manager, (OcrBounds){0,0,0,0}, 1);
    manager_notify_locked(manager, OCR_MANAGER_EVENT_STATUS);
    (void)pthread_cond_signal(&manager->cond);
    pthread_mutex_unlock(&manager->mutex);
    return rc;
}

int ocr_manager_add_completed_stroke(OcrManager *manager,
                                     const OcrManagerStroke *stroke) {
    ManagerStroke copy;
    OcrBounds old_bounds;
    int replaced = 0, invalidated = 0, rc;
    if (!manager || !stroke) return OCR_ERR_INVALID;
    rc = manager_copy_input_stroke(stroke, &copy);
    if (rc != OCR_OK) return rc;
    if (copy.completed_ms <= 0) copy.completed_ms = manager_now_ms();
    pthread_mutex_lock(&manager->mutex);
    if (!manager->project_open) {
        rc = OCR_ERR_INVALID;
        goto unlock;
    }
    copy.initial_authoritative = manager->initial_scan_finished ? 0u : 1u;
    rc = manager_insert_stroke_locked(manager, &copy, &old_bounds, &replaced);
    if (rc != OCR_OK) goto unlock;
    ++manager->document_generation;
    if (!manager->initial_scan_finished) {
        rc = OCR_OK;
        goto unlock;
    }
    if (replaced) {
        invalidated |= manager_invalidate_bounds_locked(manager, old_bounds);
        manager_queue_dirty_locked(manager, old_bounds, 0);
    }
    invalidated |= manager_invalidate_bounds_locked(manager,
                                                     manager->strokes[
        (OcrU32)manager_find_stroke_slot(manager, stroke->runtime_index)].bounds);
    manager_queue_dirty_locked(manager, manager->strokes[
        (OcrU32)manager_find_stroke_slot(manager, stroke->runtime_index)].bounds, 0);
    rc = manager_start_worker_locked(manager);
    if (invalidated) manager_notify_locked(manager, OCR_MANAGER_EVENT_INDEX);
unlock:
    pthread_mutex_unlock(&manager->mutex);
    manager_free_stroke(&copy);
    return rc;
}

int ocr_manager_set_strokes_active(OcrManager *manager,
                                   const OcrI32 *runtime_indices,
                                   OcrU32 count, int active) {
    OcrBounds dirty = {0,0,0,0};
    OcrU32 i, j;
    int have_dirty = 0, changed = 0, invalidated = 0, rc = OCR_OK;
    if (!manager || (!runtime_indices && count) || count > MANAGER_MAX_STROKES)
        return OCR_ERR_INVALID;
    pthread_mutex_lock(&manager->mutex);
    if (!manager->project_open) {
        rc = OCR_ERR_INVALID;
        goto done;
    }
    for (i = 0; i < count; ++i) {
        for (j = 0; j < i; ++j)
            if (runtime_indices[j] == runtime_indices[i]) {
                rc = OCR_ERR_INVALID;
                goto done;
            }
    }
    if (!manager->initial_scan_finished) {
        for (i = 0; i < count; ++i) {
            OcrI32 slot = manager_find_stroke_slot(manager,
                                                   runtime_indices[i]);
            if (slot >= 0) {
                ManagerStroke *stroke = manager->strokes + (OcrU32)slot;
                stroke->active = active ? 1u : 0u;
                stroke->initial_authoritative = 1u;
            }
        }
        if (count) ++manager->document_generation;
        goto done;
    }
    for (i = 0; i < count; ++i)
        if (manager_find_stroke_slot(manager, runtime_indices[i]) < 0) {
            rc = OCR_ERR_NOT_FOUND;
            goto done;
        }
    for (i = 0; i < count; ++i) {
        OcrI32 slot = manager_find_stroke_slot(manager, runtime_indices[i]);
        ManagerStroke *stroke = manager->strokes + (OcrU32)slot;
        if ((stroke->active != 0) == (active != 0)) continue;
        stroke->active = active ? 1u : 0u;
        dirty = have_dirty ? manager_bounds_union(dirty, stroke->bounds)
                           : stroke->bounds;
        have_dirty = changed = 1;
    }
    if (!changed) goto done;
    for (i = 0; i < manager->region_count; ++i) {
        ManagerRegion *region = manager->regions + i;
        for (j = 0; j < count; ++j) if (manager_region_has_member(
                                               region, runtime_indices[j])) {
            const OcrIndexRecord *record =
                ocr_index_find(&manager->index, region->occurrence_id);
            if (record && (record->flags & OCR_RECORD_ACTIVE)) {
                manager_deactivate_occurrence_locked(manager,
                                                     region->occurrence_id);
                invalidated = 1;
            }
            dirty = manager_bounds_union(dirty, region->bounds);
            break;
        }
    }
    ++manager->document_generation;
    manager_queue_dirty_locked(manager, dirty, 0);
    rc = manager_start_worker_locked(manager);
    if (invalidated) manager_notify_locked(manager, OCR_MANAGER_EVENT_INDEX);
done:
    pthread_mutex_unlock(&manager->mutex);
    return rc;
}

static OcrI32 manager_translation_slot(
    const OcrManagerTranslation *translations, OcrU32 count,
    OcrI32 runtime_index) {
    OcrU32 i;
    for (i = 0; i < count; ++i)
        if (translations[i].runtime_index == runtime_index) return (OcrI32)i;
    return -1;
}

int ocr_manager_translate_strokes(OcrManager *manager,
                                  const OcrManagerTranslation *translations,
                                  OcrU32 count) {
    OcrU8 *affected_regions = 0, *covered_strokes = 0;
    OcrBounds dirty = {0,0,0,0};
    OcrU32 i, j;
    int safe = 1, have_dirty = 0, rc = OCR_OK;
    if (!manager || !translations || !count || count > MANAGER_MAX_STROKES)
        return OCR_ERR_INVALID;
    pthread_mutex_lock(&manager->mutex);
    if (!manager->project_open) {
        rc = OCR_ERR_INVALID;
        goto done;
    }
    if (!manager->initial_scan_finished) {
        for (i = 0; i < count; ++i) {
            OcrI32 slot;
            if (!manager_finite(translations[i].dx) ||
                !manager_finite(translations[i].dy)) {
                rc = OCR_ERR_INVALID; goto done;
            }
            for (j = 0; j < i; ++j)
                if (translations[j].runtime_index ==
                    translations[i].runtime_index) {
                    rc = OCR_ERR_INVALID; goto done;
                }
            slot = manager_find_stroke_slot(
                manager, translations[i].runtime_index);
            if (slot >= 0) {
                ManagerStroke *stroke = manager->strokes + (OcrU32)slot;
                OcrBounds moved = stroke->bounds;
                moved.minx += translations[i].dx;
                moved.maxx += translations[i].dx;
                moved.miny += translations[i].dy;
                moved.maxy += translations[i].dy;
                if (!manager_bounds_valid(&moved)) {
                    rc = OCR_ERR_LIMIT; goto done;
                }
            }
        }
        for (i = 0; i < count; ++i) {
            OcrI32 slot = manager_find_stroke_slot(
                manager, translations[i].runtime_index);
            if (slot >= 0) {
                ManagerStroke *stroke = manager->strokes + (OcrU32)slot;
                for (j = 0; j < stroke->point_count; ++j) {
                    stroke->points[j].x += translations[i].dx;
                    stroke->points[j].y += translations[i].dy;
                }
                stroke->bounds.minx += translations[i].dx;
                stroke->bounds.maxx += translations[i].dx;
                stroke->bounds.miny += translations[i].dy;
                stroke->bounds.maxy += translations[i].dy;
                stroke->initial_authoritative = 1u;
            }
        }
        ++manager->document_generation;
        ++manager->job_generation;
        safe = 0;
        goto done;
    }
    covered_strokes = (OcrU8 *)calloc(count, 1u);
    if (manager->region_count)
        affected_regions = (OcrU8 *)calloc(manager->region_count, 1u);
    if (!covered_strokes || (manager->region_count && !affected_regions)) {
        rc = OCR_ERR_NOMEM;
        goto done;
    }
    for (i = 0; i < count; ++i) {
        OcrI32 slot;
        OcrBounds moved;
        if (!manager_finite(translations[i].dx) ||
            !manager_finite(translations[i].dy)) {
            rc = OCR_ERR_INVALID; goto done;
        }
        for (j = 0; j < i; ++j)
            if (translations[j].runtime_index == translations[i].runtime_index) {
                rc = OCR_ERR_INVALID; goto done;
            }
        slot = manager_find_stroke_slot(manager,
                                        translations[i].runtime_index);
        if (slot < 0) { rc = OCR_ERR_NOT_FOUND; goto done; }
        moved = manager->strokes[(OcrU32)slot].bounds;
        moved.minx += translations[i].dx;
        moved.maxx += translations[i].dx;
        moved.miny += translations[i].dy;
        moved.maxy += translations[i].dy;
        if (!manager_bounds_valid(&moved)) { rc = OCR_ERR_LIMIT; goto done; }
        dirty = have_dirty
                    ? manager_bounds_union(dirty,
                        manager_bounds_union(manager->strokes[(OcrU32)slot].bounds,
                                             moved))
                    : manager_bounds_union(manager->strokes[(OcrU32)slot].bounds,
                                           moved);
        have_dirty = 1;
    }
    for (i = 0; i < manager->region_count; ++i) {
        ManagerRegion *region = manager->regions + i;
        OcrI32 first = -1;
        int touches = 0, complete = 1;
        for (j = 0; j < region->member_count; ++j) {
            OcrI32 translation = manager_translation_slot(
                translations, count, region->members[j]);
            if (translation >= 0) {
                touches = 1;
                covered_strokes[(OcrU32)translation] = 1u;
                if (first < 0) first = translation;
                else if (manager_absf(translations[(OcrU32)first].dx -
                                      translations[(OcrU32)translation].dx) >
                             1.0e-5f ||
                         manager_absf(translations[(OcrU32)first].dy -
                                      translations[(OcrU32)translation].dy) >
                             1.0e-5f)
                    complete = 0;
            } else if (touches || first >= 0) {
                complete = 0;
            }
        }
        if (touches) {
            const OcrIndexRecord *record =
                ocr_index_find(&manager->index, region->occurrence_id);
            for (j = 0; j < region->member_count; ++j)
                if (manager_translation_slot(translations, count,
                                             region->members[j]) < 0)
                    complete = 0;
            if (!record || !(record->flags & OCR_RECORD_ACTIVE)) complete = 0;
            affected_regions[i] = 1u;
            if (!complete) safe = 0;
        }
    }
    for (i = 0; i < count; ++i)
        if (!covered_strokes[i]) safe = 0;
    for (i = 0; i < count; ++i) {
        OcrI32 slot = manager_find_stroke_slot(manager,
                                               translations[i].runtime_index);
        ManagerStroke *stroke = manager->strokes + (OcrU32)slot;
        for (j = 0; j < stroke->point_count; ++j) {
            stroke->points[j].x += translations[i].dx;
            stroke->points[j].y += translations[i].dy;
        }
        stroke->bounds.minx += translations[i].dx;
        stroke->bounds.maxx += translations[i].dx;
        stroke->bounds.miny += translations[i].dy;
        stroke->bounds.maxy += translations[i].dy;
    }
    ++manager->document_generation;
    ++manager->job_generation; /* Cancel any snapshot using old coordinates. */
    if (safe) {
        for (i = 0; i < manager->region_count; ++i) if (affected_regions[i]) {
            ManagerRegion *region = manager->regions + i;
            OcrI32 map = manager_translation_slot(
                translations, count, region->members[0]);
            float dx = translations[(OcrU32)map].dx;
            float dy = translations[(OcrU32)map].dy;
            if (ocr_index_translate(&manager->index, region->occurrence_id,
                                    dx, dy) != OCR_OK) {
                safe = 0;
                break;
            }
            region->bounds.minx += dx; region->bounds.maxx += dx;
            region->bounds.miny += dy; region->bounds.maxy += dy;
        }
    }
    if (!safe) {
        for (i = 0; i < manager->region_count; ++i) if (affected_regions[i])
            manager_deactivate_occurrence_locked(
                manager, manager->regions[i].occurrence_id);
        manager_queue_dirty_locked(manager, dirty, 0);
        rc = manager_start_worker_locked(manager);
    } else {
        manager->checkpoint_pending = manager->sidecar_path[0] ? 1 : 0;
        (void)pthread_cond_signal(&manager->cond);
    }
    manager_notify_locked(manager, OCR_MANAGER_EVENT_INDEX |
                                   OCR_MANAGER_EVENT_STATUS);
done:
    if (affected_regions) free(affected_regions);
    if (covered_strokes) free(covered_strokes);
    pthread_mutex_unlock(&manager->mutex);
    return rc < 0 ? rc : safe;
}

static void manager_owned_view(const ManagerStroke *stroke,
                               OcrStrokeView *view) {
    memset(view, 0, sizeof(*view));
    view->points = stroke->points;
    view->point_count = stroke->point_count;
    view->base_width = stroke->base_width;
    view->bounds = stroke->bounds;
    view->runtime_index = stroke->runtime_index;
    view->completed_ms = stroke->completed_ms;
    view->active = stroke->active;
}

static OcrI32 manager_duplicate_slot(const OcrManagerDuplicate *mappings,
                                     OcrU32 count, OcrI32 source_index) {
    OcrU32 i;
    for (i = 0; i < count; ++i)
        if (mappings[i].source_runtime_index == source_index)
            return (OcrI32)i;
    return -1;
}

/* The caller owns manager->mutex. */
static int manager_duplicate_locked(OcrManager *manager,
                                    const OcrManagerDuplicate *mappings,
                                    OcrU32 count, int require_sources) {
    OcrU8 *covered = 0;
    OcrU32 i, j, cloned = 0, source_region_count;
    OcrBounds dirty = {0,0,0,0};
    int have_dirty = 0, rc = OCR_OK;
    if (!manager || !mappings || !count || count > MANAGER_MAX_STROKES)
        return OCR_ERR_INVALID;
    if (!manager->project_open) return OCR_ERR_INVALID;
    covered = (OcrU8 *)calloc(count, 1u);
    if (!covered) { rc = OCR_ERR_NOMEM; goto done; }
    for (i = 0; i < count; ++i) {
        if (mappings[i].source_runtime_index < 0 ||
            mappings[i].destination_runtime_index < 0 ||
            mappings[i].source_runtime_index ==
                mappings[i].destination_runtime_index ||
            manager_find_stroke_slot(manager,
                mappings[i].destination_runtime_index) < 0) {
            rc = OCR_ERR_NOT_FOUND; goto done;
        }
        if (require_sources && manager_find_stroke_slot(
                manager, mappings[i].source_runtime_index) < 0) {
            rc = OCR_ERR_NOT_FOUND; goto done;
        }
        for (j = 0; j < i; ++j)
            if (mappings[j].source_runtime_index ==
                    mappings[i].source_runtime_index ||
                mappings[j].destination_runtime_index ==
                    mappings[i].destination_runtime_index) {
                rc = OCR_ERR_INVALID; goto done;
            }
    }
    /* Clone only records that existed at entry; newly appended records are not
       recursively treated as sources. */
    source_region_count = manager->region_count;
    for (i = 0; i < source_region_count; ++i) {
        OcrU64 source_occurrence_id = manager->regions[i].occurrence_id;
        OcrBounds source_bounds = manager->regions[i].bounds;
        OcrU32 source_member_count = manager->regions[i].member_count;
        OcrI32 *source_members = 0;
        const OcrIndexRecord *record =
            ocr_index_find(&manager->index, source_occurrence_id);
        OcrI32 *destination_members = 0;
        float dx = 0.0f, dy = 0.0f;
        int touches = 0, complete = 1, have_translation = 0;
        if (!record || !(record->flags & OCR_RECORD_ACTIVE)) continue;
        if (source_member_count) {
            OcrSize member_bytes =
                (OcrSize)source_member_count * sizeof(OcrI32);
            source_members = (OcrI32 *)malloc(member_bytes);
            destination_members = (OcrI32 *)malloc(member_bytes);
            if (!source_members || !destination_members) {
                if (source_members) free(source_members);
                if (destination_members) free(destination_members);
                rc = OCR_ERR_NOMEM;
                goto done;
            }
            /* manager_set_region_locked can realloc manager->regions. */
            memcpy(source_members, manager->regions[i].members, member_bytes);
        }
        for (j = 0; j < source_member_count; ++j) {
            OcrI32 map = manager_duplicate_slot(mappings, count,
                                                source_members[j]);
            if (map < 0) { complete = 0; continue; }
            else {
                OcrI32 source_slot = manager_find_stroke_slot(
                    manager, mappings[(OcrU32)map].source_runtime_index);
                OcrI32 destination_slot = manager_find_stroke_slot(
                    manager, mappings[(OcrU32)map].destination_runtime_index);
                ManagerStroke *source;
                ManagerStroke *destination;
                OcrStrokeView source_view, destination_view;
                OcrU8 source_hash[OCR_HASH_BYTES], destination_hash[OCR_HASH_BYTES];
                float member_dx, member_dy;
                touches = 1;
                if (source_slot < 0 || destination_slot < 0) {
                    complete = 0;
                    continue;
                }
                source = manager->strokes + (OcrU32)source_slot;
                destination = manager->strokes + (OcrU32)destination_slot;
                member_dx = destination->bounds.minx - source->bounds.minx;
                member_dy = destination->bounds.miny - source->bounds.miny;
                manager_owned_view(source, &source_view);
                manager_owned_view(destination, &destination_view);
                if (!source->active || !destination->active ||
                    ocr_stroke_identity(&manager->config, &source_view,
                                        source_hash) != OCR_OK ||
                    ocr_stroke_identity(&manager->config, &destination_view,
                                        destination_hash) != OCR_OK ||
                    memcmp(source_hash, destination_hash,
                           OCR_HASH_BYTES) != 0) complete = 0;
                if (!have_translation) {
                    dx = member_dx;
                    dy = member_dy;
                    have_translation = 1;
                }
                else if (manager_absf(dx - member_dx) > 1.0e-5f ||
                         manager_absf(dy - member_dy) > 1.0e-5f)
                    complete = 0;
                destination_members[j] = destination->runtime_index;
            }
        }
        if (touches && complete) {
            OcrU64 new_id = manager->index.next_occurrence_id;
            int clone_rc = ocr_index_duplicate_translated(
                &manager->index, source_occurrence_id, new_id, dx, dy);
            if (clone_rc == OCR_OK) {
                OcrBounds bounds = source_bounds;
                bounds.minx += dx; bounds.maxx += dx;
                bounds.miny += dy; bounds.maxy += dy;
                clone_rc = manager_set_region_locked(
                    manager, new_id, destination_members,
                    source_member_count, bounds);
                if (clone_rc == OCR_OK) {
                    for (j = 0; j < source_member_count; ++j) {
                        OcrI32 map = manager_duplicate_slot(
                            mappings, count, source_members[j]);
                        covered[(OcrU32)map] = 1u;
                    }
                    ++cloned;
                } else {
                    (void)ocr_index_set_active(&manager->index, new_id, 0);
                }
            }
            if (clone_rc != OCR_OK) rc = clone_rc;
        }
        if (source_members) free(source_members);
        free(destination_members);
        if (rc != OCR_OK) goto done;
    }
    ++manager->job_generation;
    if (!manager->initial_scan_finished) {
        manager_notify_locked(manager, OCR_MANAGER_EVENT_INDEX |
                                       OCR_MANAGER_EVENT_STATUS);
        goto done;
    }
    for (i = 0; i < count; ++i) if (!covered[i]) {
        OcrI32 slot = manager_find_stroke_slot(
            manager, mappings[i].destination_runtime_index);
        OcrBounds bounds = manager->strokes[(OcrU32)slot].bounds;
        dirty = have_dirty ? manager_bounds_union(dirty, bounds) : bounds;
        have_dirty = 1;
    }
    if (have_dirty) {
        (void)manager_invalidate_bounds_locked(manager, dirty);
        manager_queue_dirty_locked(manager, dirty, 0);
        rc = manager_start_worker_locked(manager);
    } else {
        manager->checkpoint_pending = manager->sidecar_path[0] ? 1 : 0;
        (void)pthread_cond_signal(&manager->cond);
    }
    manager_notify_locked(manager, OCR_MANAGER_EVENT_INDEX |
                                   OCR_MANAGER_EVENT_STATUS);
done:
    if (covered) free(covered);
    return rc < 0 ? rc : (int)cloned;
}

int ocr_manager_duplicate_mappings(OcrManager *manager,
                                   const OcrManagerDuplicate *mappings,
                                   OcrU32 count) {
    int rc;
    if (!manager || !mappings || !count || count > MANAGER_MAX_STROKES)
        return OCR_ERR_INVALID;
    pthread_mutex_lock(&manager->mutex);
    rc = manager_duplicate_locked(manager, mappings, count, 1);
    pthread_mutex_unlock(&manager->mutex);
    return rc;
}

int ocr_manager_duplicate_strokes(OcrManager *manager,
                                  const OcrManagerStroke *destinations,
                                  const OcrManagerDuplicate *mappings,
                                  OcrU32 count) {
    ManagerStroke *copies = 0;
    OcrU32 i, j;
    int rc = OCR_OK;
    if (!manager || !destinations || !mappings || !count ||
        count > MANAGER_MAX_STROKES) return OCR_ERR_INVALID;
    copies = (ManagerStroke *)calloc(count, sizeof(ManagerStroke));
    if (!copies) return OCR_ERR_NOMEM;
    for (i = 0; i < count; ++i) {
        if (destinations[i].runtime_index !=
                mappings[i].destination_runtime_index ||
            mappings[i].source_runtime_index < 0 ||
            mappings[i].destination_runtime_index < 0 ||
            mappings[i].source_runtime_index ==
                mappings[i].destination_runtime_index) {
            rc = OCR_ERR_INVALID;
            goto done;
        }
        for (j = 0; j < i; ++j)
            if (mappings[j].source_runtime_index ==
                    mappings[i].source_runtime_index ||
                mappings[j].destination_runtime_index ==
                    mappings[i].destination_runtime_index) {
                rc = OCR_ERR_INVALID;
                goto done;
            }
        rc = manager_copy_input_stroke(destinations + i, copies + i);
        if (rc != OCR_OK) goto done;
    }
    pthread_mutex_lock(&manager->mutex);
    if (!manager->project_open) {
        rc = OCR_ERR_INVALID;
        goto unlock;
    }
    for (i = 0; i < count; ++i)
        if (manager_find_stroke_slot(
                manager, mappings[i].destination_runtime_index) >= 0) {
            rc = OCR_ERR_INVALID;
            goto unlock;
        }
    rc = manager_reserve_strokes(manager, manager->stroke_count + count);
    if (rc != OCR_OK) goto unlock;
    for (i = 0; i < count; ++i) {
        copies[i].initial_authoritative =
            manager->initial_scan_finished ? 0u : 1u;
        /* Capacity was reserved and every destination is known to be new. */
        rc = manager_insert_stroke_locked(manager, copies + i, 0, 0);
        if (rc != OCR_OK) goto unlock;
    }
    ++manager->document_generation;
    rc = manager_duplicate_locked(manager, mappings, count, 0);
unlock:
    pthread_mutex_unlock(&manager->mutex);
done:
    for (i = 0; i < count; ++i) manager_free_stroke(copies + i);
    free(copies);
    return rc;
}

static int manager_bytes_contains(const char *haystack, OcrU32 haystack_len,
                                  const char *needle, OcrU32 needle_len) {
    OcrU32 i, j;
    if (!needle_len) return 1;
    if (needle_len > haystack_len) return 0;
    for (i = 0; i + needle_len <= haystack_len; ++i) {
        for (j = 0; j < needle_len && haystack[i + j] == needle[j]; ++j) {}
        if (j == needle_len) return 1;
    }
    return 0;
}

static void *manager_current_jni_env(OcrManager *manager) {
#if defined(__ANDROID__)
    void *env = 0;
    JavaVM *vm = (JavaVM *)manager->java_vm;
    if (vm && (*vm)->GetEnv(vm, &env, JNI_VERSION_1_6) == JNI_OK) return env;
#else
    (void)manager;
#endif
    return 0;
}

static void manager_copy_label(char destination[OCR_MANAGER_SEARCH_LABEL_BYTES],
                               const char *source, OcrU32 length) {
    OcrU32 count = length;
    if (count >= OCR_MANAGER_SEARCH_LABEL_BYTES)
        count = OCR_MANAGER_SEARCH_LABEL_BYTES - 1u;
    while (count && ((OcrU8)source[count] & 0xc0u) == 0x80u) --count;
    if (count) memcpy(destination, source, count);
    destination[count] = 0;
}

static void manager_fill_hit(const OcrIndexRecord *record,
                             OcrManagerSearchHit *hit) {
    memset(hit, 0, sizeof(*hit));
    hit->handle = record->occurrence_id;
    hit->bounds = record->bounds;
    hit->confidence = record->confidence;
    manager_copy_label(hit->label, record->original_text,
                       record->original_len);
}

int ocr_manager_search(OcrManager *manager, const char *query,
                       OcrManagerSearchHit *out_hits, OcrU32 capacity) {
    char *normalized = 0;
    OcrU32 query_len = 0, normalized_len = 0, i, found = 0;
    int rc;
    void *env;
    if (!manager || !query || (!out_hits && capacity)) return OCR_ERR_INVALID;
    while (query[query_len]) {
        if (query_len >= OCR_MAX_TEXT_BYTES) return OCR_ERR_LIMIT;
        ++query_len;
    }
    env = manager_current_jni_env(manager);
    rc = ocr_unicode_normalize_android(env, query, query_len,
                                       &normalized, &normalized_len);
    if (rc != OCR_OK && env)
        rc = ocr_normalize_basic(query, query_len,
                                 &normalized, &normalized_len);
    if (rc != OCR_OK) return rc;
    if (!normalized_len) { free(normalized); return 0; }
    pthread_mutex_lock(&manager->mutex);
    if (manager->enabled && manager->project_open) {
        for (i = 0; i < manager->index.count && found < capacity; ++i) {
            const OcrIndexRecord *record = manager->index.records + i;
            if (!(record->flags & OCR_RECORD_ACTIVE) ||
                record->confidence < manager->config.search_min_confidence)
                continue;
            if (manager_bytes_contains(record->normalized_text,
                                       record->normalized_len,
                                       normalized, normalized_len))
                manager_fill_hit(record, out_hits + found++);
        }
    }
    pthread_mutex_unlock(&manager->mutex);
    free(normalized);
    return (int)found;
}

int ocr_manager_lookup(OcrManager *manager, OcrU64 handle,
                       OcrManagerSearchHit *out_hit) {
    const OcrIndexRecord *record;
    int rc = OCR_ERR_NOT_FOUND;
    if (!manager || !handle || !out_hit) return OCR_ERR_INVALID;
    pthread_mutex_lock(&manager->mutex);
    record = ocr_index_find(&manager->index, handle);
    if (manager->enabled && manager->project_open && record &&
        (record->flags & OCR_RECORD_ACTIVE) &&
        record->confidence >= manager->config.search_min_confidence) {
        manager_fill_hit(record, out_hit);
        rc = OCR_OK;
    }
    pthread_mutex_unlock(&manager->mutex);
    return rc;
}

int ocr_manager_set_enabled(OcrManager *manager, int enabled) {
    int rc = OCR_OK;
    if (!manager) return OCR_ERR_INVALID;
    pthread_mutex_lock(&manager->mutex);
    enabled = enabled != 0;
    if (manager->enabled != enabled) {
        manager->enabled = enabled;
        ++manager->job_generation;
        if (!enabled) {
            manager->state = OCR_MANAGER_DISABLED;
        } else if (manager->project_open) {
            manager->state = manager->sidecar_load_pending
                                 ? OCR_MANAGER_LOADING_CACHE
                                 : OCR_MANAGER_INDEXING;
            rc = manager_start_worker_locked(manager);
            if (rc == OCR_OK && manager->initial_scan_finished &&
                manager->sidecar_loaded && !manager->dirty_all &&
                !manager->dirty_count)
                manager_queue_dirty_locked(manager, (OcrBounds){0,0,0,0}, 1);
        } else {
            manager->state = OCR_MANAGER_READY;
        }
        manager_notify_locked(manager, OCR_MANAGER_EVENT_STATUS |
                                       OCR_MANAGER_EVENT_INDEX);
        (void)pthread_cond_broadcast(&manager->cond);
    }
    pthread_mutex_unlock(&manager->mutex);
    return rc;
}

int ocr_manager_rebuild(OcrManager *manager) {
    char path[MANAGER_PATH_BYTES];
    int rc;
    if (!manager) return OCR_ERR_INVALID;
    path[0] = 0;
    pthread_mutex_lock(&manager->mutex);
    if (!manager->project_open) {
        pthread_mutex_unlock(&manager->mutex);
        return OCR_ERR_INVALID;
    }
    while (manager->checkpoint_running)
        (void)pthread_cond_wait(&manager->cond, &manager->mutex);
    manager_copy_string(path, MANAGER_PATH_BYTES, manager->sidecar_path);
    ocr_index_free(&manager->index);
    ocr_index_init(&manager->index, manager->project_key);
    manager_free_regions(manager);
    manager->dirty_count = 0;
    manager->dirty_all = 0;
    manager->sidecar_load_pending = 0;
    manager->sidecar_loaded = 1;
    manager->checkpoint_pending = 0;
    manager->rebuilding = 1;
    ++manager->document_generation;
    ++manager->job_generation;
    manager->state = manager->enabled ? OCR_MANAGER_REBUILDING
                                      : OCR_MANAGER_DISABLED;
    manager_set_error_locked(manager, "");
    if (path[0]) (void)remove(path);
    if (manager->enabled && manager->initial_scan_finished)
        manager_queue_dirty_locked(manager, (OcrBounds){0,0,0,0}, 1);
    rc = manager_start_worker_locked(manager);
    manager_notify_locked(manager, OCR_MANAGER_EVENT_INDEX |
                                   OCR_MANAGER_EVENT_STATUS);
    (void)pthread_cond_broadcast(&manager->cond);
    pthread_mutex_unlock(&manager->mutex);
    return rc;
}

void ocr_manager_get_status(OcrManager *manager, OcrManagerStatus *out_status) {
    OcrU32 i;
    if (!out_status) return;
    memset(out_status, 0, sizeof(*out_status));
    if (!manager) return;
    pthread_mutex_lock(&manager->mutex);
    out_status->state = manager->state;
    out_status->sidecar_status = manager->sidecar_status;
    out_status->enabled = manager->enabled;
    out_status->project_open = manager->project_open;
    out_status->initial_scan_finished = manager->initial_scan_finished;
    out_status->worker_started = manager->worker_started;
    out_status->session_ready = manager->session_ready;
    out_status->stroke_count = manager->stroke_count;
    out_status->cached_record_count = manager->index.count;
    for (i = 0; i < manager->index.count; ++i)
        if (manager->index.records[i].flags & OCR_RECORD_ACTIVE)
            ++out_status->active_record_count;
    out_status->dirty_region_count = manager->dirty_all
                                         ? 1u : manager->dirty_count;
    out_status->project_generation = manager->project_generation;
    out_status->document_generation = manager->document_generation;
    out_status->job_generation = manager->job_generation;
    out_status->last_timing = manager->last_timing;
    manager_copy_string(out_status->last_error, OCR_MANAGER_ERROR_BYTES,
                        manager->last_error);
    pthread_mutex_unlock(&manager->mutex);
}

void ocr_manager_get_last_timing(OcrManager *manager,
                                 OcrManagerTiming *out_timing) {
    if (!out_timing) return;
    memset(out_timing, 0, sizeof(*out_timing));
    if (!manager) return;
    pthread_mutex_lock(&manager->mutex);
    *out_timing = manager->last_timing;
    pthread_mutex_unlock(&manager->mutex);
}

int ocr_manager_checkpoint(OcrManager *manager) {
    int rc;
    if (!manager) return OCR_ERR_INVALID;
    pthread_mutex_lock(&manager->mutex);
    if (!manager->project_open || !manager->sidecar_path[0]) {
        pthread_mutex_unlock(&manager->mutex);
        return OCR_ERR_INVALID;
    }
    manager->checkpoint_pending = 1;
    rc = manager_start_worker_locked(manager);
    (void)pthread_cond_signal(&manager->cond);
    pthread_mutex_unlock(&manager->mutex);
    return rc;
}

void ocr_manager_set_interaction_active(OcrManager *manager, int active,
                                        OcrI64 defer_ms) {
    OcrI64 now;
    if (!manager) return;
    if (defer_ms < 0) defer_ms = 0;
    pthread_mutex_lock(&manager->mutex);
    now = manager_now_ms();
    manager->interaction_active = active != 0;
    if (active) {
        if (manager->defer_until_ms < now)
            manager->defer_until_ms = now;
    } else {
        manager->defer_until_ms = now + defer_ms;
    }
    (void)pthread_cond_broadcast(&manager->cond);
    pthread_mutex_unlock(&manager->mutex);
}

int ocr_manager_event_fd(const OcrManager *manager) {
    return manager ? manager->event_read_fd : -1;
}

OcrU32 ocr_manager_drain_events(OcrManager *manager) {
    OcrU32 events;
    if (!manager) return 0u;
    pthread_mutex_lock(&manager->mutex);
    events = manager->pending_events;
    if (events && manager->event_read_fd >= 0) {
#if defined(_WIN32)
        unsigned char byte;
        (void)_read(manager->event_read_fd, &byte, 1u);
#else
        OcrU64 count;
        (void)read(manager->event_read_fd, &count, sizeof(count));
#endif
    }
    manager->pending_events = 0u;
    pthread_mutex_unlock(&manager->mutex);
    return events;
}
