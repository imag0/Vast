#include "ocr_core.h"

extern int memcmp(const void *, const void *, OcrSize);
extern void *memcpy(void *, const void *, OcrSize);

static int finite_f32(float v) {
    OcrU32 u=0;
    memcpy(&u,&v,4);
    return (u&0x7f800000u)!=0x7f800000u;
}

static float minf(float a,float b){return a<b?a:b;}
static float maxf(float a,float b){return a>b?a:b;}

static void hash_u32(OcrSha256 *h,OcrU32 v) {
    OcrU8 b[4];
    b[0]=(OcrU8)v; b[1]=(OcrU8)(v>>8); b[2]=(OcrU8)(v>>16); b[3]=(OcrU8)(v>>24);
    ocr_sha256_update(h,b,4);
}

static void hash_i32(OcrSha256 *h,OcrI32 v){hash_u32(h,(OcrU32)v);}

static int quantize_i32(double value,double scale,OcrI32*out) {
    double q=value*scale;
    OcrI64 r;
    if (!(q==q) || q>2147483647.0 || q<-2147483648.0) return 0;
    r=(OcrI64)(q>=0.0?q+0.5:q-0.5);
    if (r>2147483647LL||r<-2147483648LL) return 0;
    *out=(OcrI32)r;
    return 1;
}

void ocr_config_default(OcrConfig *out) {
    if (!out) return;
    out->identity_version=1u;
    out->index_version=1u;
    out->idle_delay_ms=850;
    out->temporal_gap_ms=1800;
    out->horizontal_gap_factor=1.80f;
    out->vertical_gap_factor=0.78f;
    out->baseline_factor=1.15f;
    out->height_ratio_limit=5.0f;
    out->search_min_confidence=0.40f;
    out->min_points_per_stroke=2u;
    out->max_segment_strokes=2048u;
    out->raster_height=48u;
    out->raster_padding=4u;
    out->raster_max_width=1600u;
    out->raster_min_radius_px=0.65f;
    out->raster_max_radius_px=5.5f;
}

int ocr_stroke_bounds(const OcrStrokeView *stroke,OcrBounds*out) {
    OcrU32 i;
    OcrBounds b;
    if (!stroke||!out||!stroke->points||stroke->point_count==0u) return OCR_ERR_INVALID;
    if (!finite_f32(stroke->points[0].x)||!finite_f32(stroke->points[0].y)||
        !finite_f32(stroke->points[0].p)) return OCR_ERR_INVALID;
    b.minx=b.maxx=stroke->points[0].x;
    b.miny=b.maxy=stroke->points[0].y;
    for (i=1;i<stroke->point_count;++i) {
        const OcrPoint*p=stroke->points+i;
        if (!finite_f32(p->x)||!finite_f32(p->y)||!finite_f32(p->p)) return OCR_ERR_INVALID;
        b.minx=minf(b.minx,p->x); b.maxx=maxf(b.maxx,p->x);
        b.miny=minf(b.miny,p->y); b.maxy=maxf(b.maxy,p->y);
    }
    if (b.minx<-1.0e12f||b.maxx>1.0e12f||b.miny<-1.0e12f||b.maxy>1.0e12f)
        return OCR_ERR_LIMIT;
    *out=b;
    return OCR_OK;
}

int ocr_stroke_identity(const OcrConfig *cfg,const OcrStrokeView *stroke,
                        OcrU8 out_hash[OCR_HASH_BYTES]) {
    OcrSha256 h;
    OcrU32 i;
    OcrI32 qp;
    OcrBounds b;
    if (!cfg||!stroke||!out_hash||ocr_stroke_bounds(stroke,&b)!=OCR_OK)
        return OCR_ERR_INVALID;
    if (stroke->point_count>1000000u) return OCR_ERR_LIMIT;
    ocr_sha256_init(&h);
    hash_u32(&h,0x5354524bu); /* STRK */
    hash_u32(&h,cfg->identity_version);
    hash_u32(&h,stroke->point_count);
    if (!quantize_i32((double)stroke->points[0].p,1024.0,&qp)) return OCR_ERR_INVALID;
    hash_i32(&h,qp);
    for (i=1;i<stroke->point_count;++i) {
        OcrI32 qx,qy;
        double dx=(double)stroke->points[i].x-(double)stroke->points[i-1u].x;
        double dy=(double)stroke->points[i].y-(double)stroke->points[i-1u].y;
        if (!quantize_i32(dx,64.0,&qx)||!quantize_i32(dy,64.0,&qy)||
            !quantize_i32((double)stroke->points[i].p,1024.0,&qp)) return OCR_ERR_LIMIT;
        hash_i32(&h,qx); hash_i32(&h,qy); hash_i32(&h,qp);
    }
    ocr_sha256_final(&h,out_hash);
    (void)b;
    return OCR_OK;
}

static int source_less(const OcrSourceRef*a,const OcrSourceRef*b) {
    int c=memcmp(a->shape_hash,b->shape_hash,OCR_HASH_BYTES);
    if (c) return c<0;
    if (a->local_y_q!=b->local_y_q) return a->local_y_q<b->local_y_q;
    if (a->local_x_q!=b->local_x_q) return a->local_x_q<b->local_x_q;
    if (a->point_count!=b->point_count) return a->point_count<b->point_count;
    return 0;
}

int ocr_region_identity(const OcrConfig *cfg,const OcrStrokeView *strokes,
                        const OcrU32 *members,OcrU32 member_count,
                        OcrU8 out_hash[OCR_HASH_BYTES],
                        OcrSourceRef *out_sources,OcrU32 source_capacity,
                        OcrBounds *out_bounds) {
    OcrU32 i,j;
    OcrBounds rb;
    OcrSha256 h;
    if (!cfg||!strokes||!members||!member_count||!out_hash||!out_sources||
        source_capacity<member_count||member_count>cfg->max_segment_strokes)
        return OCR_ERR_INVALID;
    for (i=0;i<member_count;++i) {
        OcrU32 si=members[i];
        OcrBounds b;
        if (ocr_stroke_bounds(strokes+si,&b)!=OCR_OK) return OCR_ERR_INVALID;
        if (i==0u) rb=b;
        else {
            rb.minx=minf(rb.minx,b.minx); rb.miny=minf(rb.miny,b.miny);
            rb.maxx=maxf(rb.maxx,b.maxx); rb.maxy=maxf(rb.maxy,b.maxy);
        }
    }
    for (i=0;i<member_count;++i) {
        OcrU32 si=members[i];
        OcrBounds b;
        OcrSourceRef r;
        if (ocr_stroke_bounds(strokes+si,&b)!=OCR_OK||
            ocr_stroke_identity(cfg,strokes+si,r.shape_hash)!=OCR_OK||
            !quantize_i32((double)b.minx-(double)rb.minx,64.0,&r.local_x_q)||
            !quantize_i32((double)b.miny-(double)rb.miny,64.0,&r.local_y_q))
            return OCR_ERR_INVALID;
        r.point_count=strokes[si].point_count;
        r.runtime_index=strokes[si].runtime_index;
        out_sources[i]=r;
    }
    for (i=1;i<member_count;++i) {
        OcrSourceRef key=out_sources[i];
        j=i;
        while (j>0u&&source_less(&key,&out_sources[j-1u])) {
            out_sources[j]=out_sources[j-1u]; --j;
        }
        out_sources[j]=key;
    }
    ocr_sha256_init(&h);
    hash_u32(&h,0x5245474eu); /* REGN */
    hash_u32(&h,cfg->identity_version);
    hash_u32(&h,member_count);
    for (i=0;i<member_count;++i) {
        ocr_sha256_update(&h,out_sources[i].shape_hash,OCR_HASH_BYTES);
        hash_i32(&h,out_sources[i].local_x_q);
        hash_i32(&h,out_sources[i].local_y_q);
        hash_u32(&h,out_sources[i].point_count);
    }
    ocr_sha256_final(&h,out_hash);
    if (out_bounds) *out_bounds=rb;
    return OCR_OK;
}
