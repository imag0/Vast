#include "src/ocr_core.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static int g_checks;
static int g_failures;

#define CHECK(expr) do {                                                     \
    ++g_checks;                                                              \
    if (!(expr)) {                                                           \
        ++g_failures;                                                        \
        printf("FAIL %s:%d: %s\n", __FILE__, __LINE__, #expr);             \
    }                                                                        \
} while (0)

static int hash_equal(const OcrU8 a[OCR_HASH_BYTES],
                      const OcrU8 b[OCR_HASH_BYTES]) {
    return memcmp(a, b, OCR_HASH_BYTES) == 0;
}

static OcrStrokeView stroke_view(const OcrPoint *points, OcrU32 count,
                                 float width, OcrU32 color,
                                 OcrI32 runtime_index, OcrI64 completed_ms) {
    OcrStrokeView s;
    memset(&s, 0, sizeof(s));
    s.points = points;
    s.point_count = count;
    s.base_width = width;
    s.color = color;
    s.runtime_index = runtime_index;
    s.completed_ms = completed_ms;
    s.active = 1u;
    return s;
}

static void test_sha256(void) {
    static const OcrU8 expected[OCR_HASH_BYTES] = {
        0xba,0x78,0x16,0xbf,0x8f,0x01,0xcf,0xea,
        0x41,0x41,0x40,0xde,0x5d,0xae,0x22,0x23,
        0xb0,0x03,0x61,0xa3,0x96,0x17,0x7a,0x9c,
        0xb4,0x10,0xff,0x61,0xf2,0x00,0x15,0xad
    };
    OcrU8 actual[OCR_HASH_BYTES];
    ocr_sha256("abc", 3u, actual);
    CHECK(hash_equal(actual, expected));
}

static void test_identity_invariance(void) {
    OcrConfig cfg;
    OcrPoint a0[] = {{0.0f,0.0f,0.35f},{2.0f,5.0f,0.55f},{7.0f,9.0f,0.8f}};
    OcrPoint a1[] = {{10.0f,2.0f,0.7f},{12.0f,8.0f,0.6f}};
    OcrPoint b0[] = {{500.0f,-200.0f,0.35f},{502.0f,-195.0f,0.55f},
                     {507.0f,-191.0f,0.8f}};
    OcrPoint b1[] = {{510.0f,-198.0f,0.7f},{512.0f,-192.0f,0.6f}};
    OcrStrokeView a[2], b[2];
    OcrU32 members[] = {0u,1u};
    OcrSourceRef as[2], bs[2];
    OcrBounds ab, bb;
    OcrU8 ah[OCR_HASH_BYTES], bh[OCR_HASH_BYTES], changed[OCR_HASH_BYTES];
    OcrU8 arh[OCR_HASH_BYTES], brh[OCR_HASH_BYTES];

    ocr_config_default(&cfg);
    a[0] = stroke_view(a0, 3u, 2.0f, 0xff112233u, 4, 1000);
    a[1] = stroke_view(a1, 2u, 4.0f, 0xff445566u, 8, 1100);
    b[0] = stroke_view(b0, 3u, 19.0f, 0xffabcdefu, 14, 1000);
    b[1] = stroke_view(b1, 2u, 0.5f, 0xff010203u, 18, 1100);

    CHECK(ocr_stroke_identity(&cfg, &a[0], ah) == OCR_OK);
    CHECK(ocr_stroke_identity(&cfg, &b[0], bh) == OCR_OK);
    CHECK(hash_equal(ah, bh)); /* translation, recolor and width invariant */

    b0[2].x += 1.0f;
    CHECK(ocr_stroke_identity(&cfg, &b[0], changed) == OCR_OK);
    CHECK(!hash_equal(ah, changed));
    b0[2].x -= 1.0f;

    CHECK(ocr_region_identity(&cfg, a, members, 2u, arh, as, 2u, &ab) == OCR_OK);
    CHECK(ocr_region_identity(&cfg, b, members, 2u, brh, bs, 2u, &bb) == OCR_OK);
    CHECK(hash_equal(arh, brh));
    CHECK(bb.minx - ab.minx == 500.0f);
    CHECK(bb.miny - ab.miny == -200.0f);
}

static void compare_segment_topology(const OcrSegmentList *a,
                                     const OcrSegmentList *b) {
    OcrU32 i, j;
    CHECK(a->count == b->count);
    if (a->count != b->count) return;
    for (i = 0; i < a->count; ++i) {
        CHECK(a->items[i].member_count == b->items[i].member_count);
        if (a->items[i].member_count != b->items[i].member_count) continue;
        for (j = 0; j < a->items[i].member_count; ++j)
            CHECK(a->items[i].members[j] == b->items[i].members[j]);
    }
}

static void test_segmentation_world_invariance(void) {
    OcrConfig cfg;
    OcrPoint p0[] = {{0.0f,0.0f,0.5f},{1.0f,10.0f,0.5f}};
    OcrPoint p1[] = {{4.0f,0.0f,0.5f},{5.0f,10.0f,0.5f}};
    OcrPoint p2[] = {{100.0f,0.0f,0.5f},{101.0f,10.0f,0.5f}};
    OcrPoint p3[] = {{104.0f,0.0f,0.5f},{105.0f,10.0f,0.5f}};
    OcrPoint q0[] = {{900.0f,-700.0f,0.5f},{901.0f,-690.0f,0.5f}};
    OcrPoint q1[] = {{904.0f,-700.0f,0.5f},{905.0f,-690.0f,0.5f}};
    OcrPoint q2[] = {{1000.0f,-700.0f,0.5f},{1001.0f,-690.0f,0.5f}};
    OcrPoint q3[] = {{1004.0f,-700.0f,0.5f},{1005.0f,-690.0f,0.5f}};
    OcrStrokeView world[4], translated[4];
    OcrSegmentList zoom_a, zoom_b, moved;
    float viewport_zoom_a = 0.125f;
    float viewport_zoom_b = 12.0f;

    ocr_config_default(&cfg);
    world[0] = stroke_view(p0,2u,2.0f,1u,0,1000);
    world[1] = stroke_view(p1,2u,2.0f,2u,1,1100);
    world[2] = stroke_view(p2,2u,2.0f,3u,2,1200);
    world[3] = stroke_view(p3,2u,2.0f,4u,3,1300);
    translated[0] = stroke_view(q0,2u,9.0f,9u,0,1000);
    translated[1] = stroke_view(q1,2u,9.0f,9u,1,1100);
    translated[2] = stroke_view(q2,2u,9.0f,9u,2,1200);
    translated[3] = stroke_view(q3,2u,9.0f,9u,3,1300);

    /* Viewport zoom is deliberately not an input: both calls consume world data. */
    CHECK(viewport_zoom_a != viewport_zoom_b);
    CHECK(ocr_segment_world(&cfg, world, 4u, &zoom_a) == OCR_OK);
    CHECK(ocr_segment_world(&cfg, world, 4u, &zoom_b) == OCR_OK);
    CHECK(ocr_segment_world(&cfg, translated, 4u, &moved) == OCR_OK);
    CHECK(zoom_a.count == 2u);
    compare_segment_topology(&zoom_a, &zoom_b);
    compare_segment_topology(&zoom_a, &moved);
    CHECK(moved.items[0].bounds.minx - zoom_a.items[0].bounds.minx == 900.0f);
    CHECK(moved.items[0].bounds.miny - zoom_a.items[0].bounds.miny == -700.0f);

    ocr_segment_list_free(&zoom_a);
    ocr_segment_list_free(&zoom_b);
    ocr_segment_list_free(&moved);
}

static void test_raster_translation_and_antialias(void) {
    OcrConfig cfg;
    OcrPoint p[] = {{0.0f,0.0f,0.35f},{8.0f,11.0f,0.8f},{15.0f,2.0f,0.5f}};
    OcrPoint q[] = {{2000.0f,-900.0f,0.35f},{2008.0f,-889.0f,0.8f},
                    {2015.0f,-898.0f,0.5f}};
    OcrStrokeView a, b;
    OcrU32 member = 0u;
    OcrSegment sa, sb;
    OcrGrayImage ia, ib;
    OcrSize n, i;
    int has_black = 0, has_gray = 0;

    ocr_config_default(&cfg);
    a = stroke_view(p,3u,1.25f,0xff000000u,0,1000);
    b = stroke_view(q,3u,1.25f,0xffffffffu,0,1000);
    memset(&sa,0,sizeof(sa)); memset(&sb,0,sizeof(sb));
    sa.members=&member; sa.member_count=1u;
    sb.members=&member; sb.member_count=1u;
    CHECK(ocr_rasterize_segment(&cfg,&a,1u,&sa,&ia) == OCR_OK);
    CHECK(ocr_rasterize_segment(&cfg,&b,1u,&sb,&ib) == OCR_OK);
    CHECK(ia.width == ib.width && ia.height == ib.height && ia.stride == ib.stride);
    n=(OcrSize)ia.stride*ia.height;
    CHECK(n == (OcrSize)ib.stride*ib.height);
    CHECK(memcmp(ia.pixels,ib.pixels,n) == 0);
    for (i=0;i<n;++i) {
        if (ia.pixels[i]==0u) has_black=1;
        if (ia.pixels[i]>0u&&ia.pixels[i]<255u) has_gray=1;
    }
    CHECK(has_black);
    CHECK(has_gray);
    ocr_gray_image_free(&ia);
    ocr_gray_image_free(&ib);
}

static void fill_hash(OcrU8 hash[OCR_HASH_BYTES], OcrU8 seed) {
    OcrU32 i;
    for (i=0;i<OCR_HASH_BYTES;++i) hash[i]=(OcrU8)(seed+i*13u);
}

static OcrIndexRecord make_record(OcrU64 id, const char *text, float confidence,
                                  OcrBounds bounds, OcrSourceRef *source,
                                  OcrU8 hash_seed) {
    OcrIndexRecord r;
    memset(&r,0,sizeof(r));
    r.occurrence_id=id;
    fill_hash(r.source_hash,hash_seed);
    r.bounds=bounds;
    r.confidence=confidence;
    r.flags=OCR_RECORD_ACTIVE;
    r.sources=source;
    r.source_count=source?1u:0u;
    r.original_text=(char *)text;
    r.original_len=(OcrU32)strlen(text);
    return r;
}

static void test_index_move_duplicate_and_search(void) {
    OcrIndex index;
    OcrSourceRef source;
    OcrIndexRecord r, low, spaced;
    const OcrIndexRecord *before, *after, *duplicate;
    OcrBounds b={1.0f,2.0f,11.0f,12.0f};
    OcrBounds low_b={20.0f,2.0f,30.0f,12.0f};
    OcrBounds spaced_b={40.0f,2.0f,50.0f,12.0f};
    OcrU8 saved_hash[OCR_HASH_BYTES];
    OcrSearchHit hits[8];
    float saved_confidence;
    char saved_text[64];
    int n;

    memset(&source,0,sizeof(source));
    fill_hash(source.shape_hash,77u);
    source.local_x_q=3; source.local_y_q=7; source.point_count=4u;
    source.runtime_index=123;
    ocr_index_init(&index,0x123456789abcdef0ULL);
    r=make_record(7u,"Alpha \xe5\x8c\x97\xe4\xba\xac Marker",0.93f,b,&source,11u);
    CHECK(ocr_index_put(&index,&r)==OCR_OK);
    before=ocr_index_find(&index,7u);
    CHECK(before!=NULL);
    if (!before) {ocr_index_free(&index);return;}
    memcpy(saved_hash,before->source_hash,OCR_HASH_BYTES);
    saved_confidence=before->confidence;
    CHECK(before->original_len<sizeof(saved_text));
    memcpy(saved_text,before->original_text,before->original_len+1u);
    CHECK(before->sources[0].runtime_index==-1);

    CHECK(ocr_index_translate(&index,7u,30.0f,-5.0f)==OCR_OK);
    after=ocr_index_find(&index,7u);
    CHECK(after!=NULL);
    if (after) {
        CHECK(after->bounds.minx==31.0f&&after->bounds.miny==-3.0f);
        CHECK(hash_equal(after->source_hash,saved_hash));
        CHECK(after->confidence==saved_confidence);
        CHECK(strcmp(after->original_text,saved_text)==0);
    }

    CHECK(ocr_index_duplicate_translated(&index,7u,42u,100.0f,25.0f)==OCR_OK);
    duplicate=ocr_index_find(&index,42u);
    CHECK(duplicate!=NULL);
    if (after&&duplicate) {
        CHECK(duplicate->occurrence_id!=after->occurrence_id);
        CHECK(duplicate->bounds.minx==after->bounds.minx+100.0f);
        CHECK(duplicate->bounds.miny==after->bounds.miny+25.0f);
        CHECK(hash_equal(duplicate->source_hash,after->source_hash));
        CHECK(duplicate->confidence==after->confidence);
        CHECK(strcmp(duplicate->original_text,after->original_text)==0);
    }

    CHECK(ocr_index_set_active(&index,7u,0)==OCR_OK);
    n=ocr_index_search(&index,"aLpHa",0.4f,hits,8u);
    CHECK(n==1);
    if (n==1) CHECK(hits[0].occurrence_id==42u);
    n=ocr_index_search(&index,"\xe5\x8c\x97\xe4\xba\xac",0.4f,hits,8u);
    CHECK(n==1);
    if (n==1) CHECK(hits[0].occurrence_id==42u);
    CHECK(ocr_index_set_active(&index,42u,0)==OCR_OK);
    CHECK(ocr_index_search(&index,"alpha",0.4f,hits,8u)==0);
    CHECK(ocr_index_set_active(&index,7u,1)==OCR_OK);
    n=ocr_index_search(&index,"alpha",0.4f,hits,8u);
    CHECK(n==1);
    if (n==1) CHECK(hits[0].occurrence_id==7u);

    low=make_record(55u,"hidden low confidence",0.20f,low_b,NULL,31u);
    CHECK(ocr_index_put(&index,&low)==OCR_OK);
    n=ocr_index_search(&index,"hidden",0.4f,hits,8u);
    CHECK(n==0);
    n=ocr_index_search(&index,"hidden",0.1f,hits,8u);
    CHECK(n==1);

    spaced=make_record(56u,"  Mixed\t Case  ",0.8f,spaced_b,NULL,32u);
    CHECK(ocr_index_put(&index,&spaced)==OCR_OK);
    n=ocr_index_search(&index,"mixed case",0.4f,hits,8u);
    CHECK(n==1);
    if (n==1) CHECK(hits[0].occurrence_id==56u);
    ocr_index_free(&index);
}

static int write_bytes(const char *path, const void *data, size_t size) {
    FILE *f=fopen(path,"wb");
    int ok;
    if (!f) return 0;
    ok=fwrite(data,1u,size,f)==size;
    if (fclose(f)!=0) ok=0;
    return ok;
}

static int copy_with_last_byte_flipped(const char *source,const char *dest) {
    FILE *f=fopen(source,"rb");
    long size;
    unsigned char *data;
    int ok=0;
    if (!f) return 0;
    if (fseek(f,0,SEEK_END)!=0) {fclose(f);return 0;}
    size=ftell(f);
    if (size<=0||fseek(f,0,SEEK_SET)!=0) {fclose(f);return 0;}
    data=(unsigned char*)malloc((size_t)size);
    if (!data) {fclose(f);return 0;}
    if (fread(data,1u,(size_t)size,f)==(size_t)size) {
        data[(size_t)size-1u]^=0x5au;
        ok=write_bytes(dest,data,(size_t)size);
    }
    free(data);
    fclose(f);
    return ok;
}

static OcrU32 read_le32(const OcrU8 *p) {
    return (OcrU32)p[0]|((OcrU32)p[1]<<8)|((OcrU32)p[2]<<16)|
           ((OcrU32)p[3]<<24);
}

static void write_le32(OcrU8 *p,OcrU32 v) {
    p[0]=(OcrU8)v; p[1]=(OcrU8)(v>>8); p[2]=(OcrU8)(v>>16);
    p[3]=(OcrU8)(v>>24);
}

static int write_absurd_length_copy(const char *source,const char *dest) {
    OcrU8 header[72];
    FILE *f=fopen(source,"rb");
    OcrU32 crc;
    int ok;
    if (!f) return 0;
    ok=fread(header,1u,sizeof(header),f)==sizeof(header);
    fclose(f);
    if (!ok) return 0;
    CHECK(read_le32(header+60u)<=OCR_MAX_SIDECAR_BYTES);
    write_le32(header+60u,OCR_MAX_SIDECAR_BYTES+1u);
    crc=ocr_crc32(0,header,68u);
    write_le32(header+68u,crc);
    return write_bytes(dest,header,sizeof(header));
}

enum SemanticCorruption {
    CORRUPT_NONFINITE_CONFIDENCE,
    CORRUPT_ORIGINAL_UTF8
};

static int write_checksum_valid_semantic_corruption(
    const char *source,const char *dest,enum SemanticCorruption kind) {
    FILE *f=fopen(source,"rb");
    long file_size;
    OcrU8 *data;
    OcrU32 payload_len,payload_crc,header_crc;
    int ok=0;
    if (!f) return 0;
    if (fseek(f,0,SEEK_END)!=0) {fclose(f);return 0;}
    file_size=ftell(f);
    if (file_size<72||fseek(f,0,SEEK_SET)!=0) {fclose(f);return 0;}
    data=(OcrU8*)malloc((size_t)file_size);
    if (!data) {fclose(f);return 0;}
    if (fread(data,1u,(size_t)file_size,f)!=(size_t)file_size) {
        free(data);fclose(f);return 0;
    }
    fclose(f);
    payload_len=read_le32(data+60u);
    if ((OcrU64)payload_len+72u!=(OcrU64)file_size||payload_len<81u) {
        free(data);return 0;
    }
    if (kind==CORRUPT_NONFINITE_CONFIDENCE) {
        /* payload: size[4], id[8], hash[32], bounds[16], confidence[4] */
        write_le32(data+72u+60u,0x7fc00000u);
    } else {
        /* This fixture has zero sources; original text begins after 4+76 bytes. */
        data[72u+80u]=0xc0u; /* forbidden overlong UTF-8 lead byte */
    }
    payload_crc=ocr_crc32(0,data+72u,payload_len);
    write_le32(data+64u,payload_crc);
    header_crc=ocr_crc32(0,data,68u);
    write_le32(data+68u,header_crc);
    ok=write_bytes(dest,data,(size_t)file_size);
    free(data);
    return ok;
}

static void test_sidecar_hardening(void) {
    const char *valid_path="test_ocr_sidecar_valid.bin";
    const char *missing_path="test_ocr_sidecar_missing.bin";
    const char *corrupt_path="test_ocr_sidecar_corrupt.bin";
    const char *absurd_path="test_ocr_sidecar_absurd.bin";
    const char *crc_path="test_ocr_sidecar_crc.bin";
    const char *finite_path="test_ocr_sidecar_nonfinite.bin";
    const char *utf8_path="test_ocr_sidecar_utf8.bin";
    const char short_data[]="not a Vast OCR sidecar";
    OcrU8 model[OCR_MODEL_HASH_BYTES], wrong_model[OCR_MODEL_HASH_BYTES];
    OcrIndex index, loaded;
    OcrIndexRecord r;
    OcrBounds bounds={-5.0f,7.0f,80.0f,31.0f};
    OcrSidecarStatus status;
    OcrU32 i;

    remove(valid_path); remove(missing_path); remove(corrupt_path);
    remove(absurd_path); remove(crc_path); remove(finite_path); remove(utf8_path);
    for (i=0;i<OCR_MODEL_HASH_BYTES;++i) {
        model[i]=(OcrU8)(i*7u+1u);
        wrong_model[i]=(OcrU8)(model[i]^0xa5u);
    }
    ocr_index_init(&index,0xfeedface12345678ULL);
    r=make_record(99u,"Sidecar \xe6\xb5\x8b\xe8\xaf\x95",0.88f,bounds,NULL,101u);
    CHECK(ocr_index_put(&index,&r)==OCR_OK);
    CHECK(ocr_sidecar_save_atomic(valid_path,&index,7u,model)==OCR_SIDECAR_OK);

    ocr_index_init(&loaded,0u);
    status=ocr_sidecar_load(valid_path,7u,model,index.project_key,&loaded);
    CHECK(status==OCR_SIDECAR_OK);
    CHECK(loaded.project_key==index.project_key);
    CHECK(loaded.count==1u);
    CHECK(ocr_index_find(&loaded,99u)!=NULL);
    if (ocr_index_find(&loaded,99u))
        CHECK(strcmp(ocr_index_find(&loaded,99u)->original_text,
                     "Sidecar \xe6\xb5\x8b\xe8\xaf\x95")==0);
    ocr_index_free(&loaded);

    ocr_index_init(&loaded,0u);
    CHECK(ocr_sidecar_load(missing_path,7u,model,index.project_key,&loaded)==
          OCR_SIDECAR_MISSING);
    CHECK(write_bytes(corrupt_path,short_data,sizeof(short_data)-1u));
    CHECK(ocr_sidecar_load(corrupt_path,7u,model,index.project_key,&loaded)==
          OCR_SIDECAR_CORRUPT);
    CHECK(copy_with_last_byte_flipped(valid_path,crc_path));
    CHECK(ocr_sidecar_load(crc_path,7u,model,index.project_key,&loaded)==
          OCR_SIDECAR_CORRUPT);
    CHECK(write_absurd_length_copy(valid_path,absurd_path));
    CHECK(ocr_sidecar_load(absurd_path,7u,model,index.project_key,&loaded)==
          OCR_SIDECAR_CORRUPT);
    CHECK(write_checksum_valid_semantic_corruption(
          valid_path,finite_path,CORRUPT_NONFINITE_CONFIDENCE));
    CHECK(ocr_sidecar_load(finite_path,7u,model,index.project_key,&loaded)==
          OCR_SIDECAR_CORRUPT);
    CHECK(write_checksum_valid_semantic_corruption(
          valid_path,utf8_path,CORRUPT_ORIGINAL_UTF8));
    CHECK(ocr_sidecar_load(utf8_path,7u,model,index.project_key,&loaded)==
          OCR_SIDECAR_CORRUPT);
    CHECK(ocr_sidecar_load(valid_path,8u,model,index.project_key,&loaded)==
          OCR_SIDECAR_INDEX_VERSION_MISMATCH);
    CHECK(ocr_sidecar_load(valid_path,7u,wrong_model,index.project_key,&loaded)==
          OCR_SIDECAR_MODEL_MISMATCH);
    CHECK(ocr_sidecar_load(valid_path,7u,model,index.project_key+1u,&loaded)==
          OCR_SIDECAR_PROJECT_MISMATCH);
    CHECK(loaded.count==0u); /* failed loads never publish partial data */
    ocr_index_free(&loaded);
    ocr_index_free(&index);

    remove(valid_path); remove(missing_path); remove(corrupt_path);
    remove(absurd_path); remove(crc_path); remove(finite_path); remove(utf8_path);
    remove("test_ocr_sidecar_valid.bin.tmp");
}

static void test_invalid_utf8_rejected(void) {
    OcrIndex index;
    OcrIndexRecord r;
    OcrBounds bounds={0.0f,0.0f,1.0f,1.0f};
    const char invalid[]={(char)0xc0,(char)0xaf,0};
    ocr_index_init(&index,1u);
    r=make_record(1u,invalid,0.9f,bounds,NULL,1u);
    CHECK(!ocr_utf8_valid(invalid,2u));
    CHECK(ocr_index_put(&index,&r)==OCR_ERR_INVALID);
    CHECK(index.count==0u);
    ocr_index_free(&index);
}

int main(void) {
    test_sha256();
    test_identity_invariance();
    test_segmentation_world_invariance();
    test_raster_translation_and_antialias();
    test_index_move_duplicate_and_search();
    test_sidecar_hardening();
    test_invalid_utf8_rejected();
    if (g_failures) {
        printf("OCR core tests: %d/%d checks FAILED\n",g_failures,g_checks);
        return 1;
    }
    printf("OCR core tests: %d checks passed\n",g_checks);
    return 0;
}
