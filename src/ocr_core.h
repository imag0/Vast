#ifndef VAST_OCR_CORE_H
#define VAST_OCR_CORE_H

/*
 * Standalone, allocation-owning OCR core primitives for Vast.
 *
 * This header deliberately does not include a C runtime header.  Vast's
 * Android build is compiled without an SDK sysroot, so the implementation
 * files use compiler-provided scalar types and declare only the libc symbols
 * they actually need.  The API remains ordinary C and is host-testable.
 */

#if defined(__SIZE_TYPE__)
typedef __SIZE_TYPE__ OcrSize;
#else
typedef unsigned long OcrSize;
#endif

typedef unsigned char OcrU8;
typedef signed int OcrI32;
typedef unsigned int OcrU32;
typedef signed long long OcrI64;
typedef unsigned long long OcrU64;

#define OCR_HASH_BYTES 32u
#define OCR_MODEL_HASH_BYTES 32u
#define OCR_SIDECAR_FORMAT_VERSION 1u
#define OCR_MAX_SIDECAR_BYTES (64u * 1024u * 1024u)
#define OCR_MAX_RECORDS 100000u
#define OCR_MAX_SOURCES_PER_RECORD 4096u
#define OCR_MAX_TEXT_BYTES 16384u
#define OCR_RECORD_ACTIVE 1u

enum {
    OCR_OK = 0,
    OCR_ERR_INVALID = -1,
    OCR_ERR_NOMEM = -2,
    OCR_ERR_LIMIT = -3,
    OCR_ERR_NOT_FOUND = -4,
    OCR_ERR_IO = -5
};

typedef struct {
    float x;
    float y;
    float p;
} OcrPoint;

typedef struct {
    float minx;
    float miny;
    float maxx;
    float maxy;
} OcrBounds;

/* A borrowed view.  Callers retain ownership of points. */
typedef struct {
    const OcrPoint *points;
    OcrU32 point_count;
    float base_width;
    OcrU32 color;
    OcrBounds bounds;
    OcrI32 runtime_index;
    OcrI64 completed_ms;
    OcrU32 active;
} OcrStrokeView;

typedef struct {
    OcrU32 identity_version;
    OcrU32 index_version;
    OcrI64 idle_delay_ms;
    OcrI64 temporal_gap_ms;
    float horizontal_gap_factor;
    float vertical_gap_factor;
    float baseline_factor;
    float height_ratio_limit;
    float search_min_confidence;
    OcrU32 min_points_per_stroke;
    OcrU32 max_segment_strokes;
    OcrU32 raster_height;
    OcrU32 raster_padding;
    OcrU32 raster_max_width;
    float raster_min_radius_px;
    float raster_max_radius_px;
} OcrConfig;

void ocr_config_default(OcrConfig *out);

typedef struct {
    OcrU8 shape_hash[OCR_HASH_BYTES];
    OcrI32 local_x_q;
    OcrI32 local_y_q;
    OcrU32 point_count;
    OcrI32 runtime_index; /* Runtime-only; serialized as no value. */
} OcrSourceRef;

typedef struct {
    OcrU32 *members; /* Indices into the OcrStrokeView input array. */
    OcrU32 member_count;
    OcrBounds bounds;
    OcrI64 latest_completed_ms;
} OcrSegment;

typedef struct {
    OcrSegment *items;
    OcrU32 count;
} OcrSegmentList;

typedef struct {
    OcrU8 *pixels; /* 0 = black ink, 255 = white background. */
    OcrU32 width;
    OcrU32 height;
    OcrU32 stride;
    OcrBounds source_bounds;
} OcrGrayImage;

typedef struct {
    OcrU32 h[8];
    OcrU64 total_bytes;
    OcrU8 block[64];
    OcrU32 block_used;
} OcrSha256;

void ocr_sha256_init(OcrSha256 *ctx);
void ocr_sha256_update(OcrSha256 *ctx, const void *data, OcrSize len);
void ocr_sha256_final(OcrSha256 *ctx, OcrU8 out[OCR_HASH_BYTES]);
void ocr_sha256(const void *data, OcrSize len, OcrU8 out[OCR_HASH_BYTES]);
OcrU32 ocr_crc32(OcrU32 seed, const void *data, OcrSize len);

int ocr_stroke_bounds(const OcrStrokeView *stroke, OcrBounds *out);
int ocr_stroke_identity(const OcrConfig *cfg, const OcrStrokeView *stroke,
                        OcrU8 out_hash[OCR_HASH_BYTES]);
int ocr_region_identity(const OcrConfig *cfg,
                        const OcrStrokeView *strokes,
                        const OcrU32 *members, OcrU32 member_count,
                        OcrU8 out_hash[OCR_HASH_BYTES],
                        OcrSourceRef *out_sources, OcrU32 source_capacity,
                        OcrBounds *out_bounds);

int ocr_segment_world(const OcrConfig *cfg, const OcrStrokeView *strokes,
                      OcrU32 stroke_count, OcrSegmentList *out);
void ocr_segment_list_free(OcrSegmentList *list);

int ocr_rasterize_segment(const OcrConfig *cfg,
                          const OcrStrokeView *strokes,
                          OcrU32 stroke_count,
                          const OcrSegment *segment,
                          OcrGrayImage *out);
void ocr_gray_image_free(OcrGrayImage *image);

typedef struct {
    OcrU64 occurrence_id;
    OcrU8 source_hash[OCR_HASH_BYTES];
    OcrBounds bounds;
    float confidence;
    OcrU32 flags;
    OcrSourceRef *sources;
    OcrU32 source_count;
    char *original_text;
    OcrU32 original_len;
    char *normalized_text;
    OcrU32 normalized_len;
} OcrIndexRecord;

typedef struct {
    OcrIndexRecord *records;
    OcrU32 count;
    OcrU32 capacity;
    OcrU64 next_occurrence_id;
    OcrU64 project_key;
} OcrIndex;

typedef struct {
    OcrU64 occurrence_id;
    OcrBounds bounds;
    float confidence;
    const char *original_text;
    OcrU32 original_len;
} OcrSearchHit;

void ocr_index_init(OcrIndex *index, OcrU64 project_key);
void ocr_index_free(OcrIndex *index);
int ocr_index_put(OcrIndex *index, const OcrIndexRecord *record);
const OcrIndexRecord *ocr_index_find(const OcrIndex *index,
                                     OcrU64 occurrence_id);
int ocr_index_set_active(OcrIndex *index, OcrU64 occurrence_id, int active);
int ocr_index_translate(OcrIndex *index, OcrU64 occurrence_id,
                        float dx, float dy);
int ocr_index_duplicate_translated(OcrIndex *index,
                                   OcrU64 source_occurrence_id,
                                   OcrU64 new_occurrence_id,
                                   float dx, float dy);
int ocr_index_search(const OcrIndex *index, const char *query,
                     float minimum_confidence, OcrSearchHit *out,
                     OcrU32 capacity);

int ocr_utf8_valid(const char *text, OcrU32 len);
int ocr_normalize_basic(const char *text, OcrU32 len,
                        char **out_text, OcrU32 *out_len);

typedef enum {
    OCR_SIDECAR_OK = 0,
    OCR_SIDECAR_MISSING = 1,
    OCR_SIDECAR_CORRUPT = 2,
    OCR_SIDECAR_INDEX_VERSION_MISMATCH = 3,
    OCR_SIDECAR_MODEL_MISMATCH = 4,
    OCR_SIDECAR_PROJECT_MISMATCH = 5,
    OCR_SIDECAR_IO_ERROR = 6,
    OCR_SIDECAR_NO_MEMORY = 7
} OcrSidecarStatus;

OcrSidecarStatus ocr_sidecar_save_atomic(
    const char *path, const OcrIndex *index, OcrU32 index_version,
    const OcrU8 model_hash[OCR_MODEL_HASH_BYTES]);
OcrSidecarStatus ocr_sidecar_load(
    const char *path, OcrU32 expected_index_version,
    const OcrU8 expected_model_hash[OCR_MODEL_HASH_BYTES],
    OcrU64 expected_project_key, OcrIndex *out_index);

#endif
