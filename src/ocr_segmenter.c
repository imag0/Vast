#include "ocr_core.h"

extern void *malloc(OcrSize);
extern void *calloc(OcrSize,OcrSize);
extern void free(void *);
extern void *memset(void *,int,OcrSize);

static float minf(float a,float b){return a<b?a:b;}
static float maxf(float a,float b){return a>b?a:b;}
static float absf_local(float a){return a<0?-a:a;}

static OcrU32 uf_find(OcrU32 *p,OcrU32 x) {
    OcrU32 r=x;
    while (p[r]!=r) r=p[r];
    while (p[x]!=x) { OcrU32 n=p[x]; p[x]=r; x=n; }
    return r;
}

static void uf_join(OcrU32*p,OcrU32*sz,OcrU32 a,OcrU32 b,OcrU32 limit) {
    a=uf_find(p,a); b=uf_find(p,b);
    if (a==b||sz[a]+sz[b]>limit) return;
    if (sz[a]<sz[b]) { OcrU32 t=a; a=b; b=t; }
    p[b]=a; sz[a]+=sz[b];
}

static int time_compatible(const OcrConfig*cfg,const OcrStrokeView*a,
                           const OcrStrokeView*b) {
    OcrI64 d;
    if (a->completed_ms<=0||b->completed_ms<=0) return 1;
    d=a->completed_ms-b->completed_ms;
    if (d<0) d=-d;
    return d<=cfg->temporal_gap_ms;
}

static int spatially_compatible(const OcrConfig*cfg,const OcrBounds*a,
                                const OcrBounds*b) {
    float ha=maxf(a->maxy-a->miny,0.5f),hb=maxf(b->maxy-b->miny,0.5f);
    float wa=maxf(a->maxx-a->minx,0.5f),wb=maxf(b->maxx-b->minx,0.5f);
    float maxh=maxf(ha,hb),minh=minf(ha,hb);
    float hgap=0,vgap=0,overlap,baseline;
    float small_limit=maxh*0.34f;
    if (a->maxx<b->minx) hgap=b->minx-a->maxx;
    else if (b->maxx<a->minx) hgap=a->minx-b->maxx;
    if (a->maxy<b->miny) vgap=b->miny-a->maxy;
    else if (b->maxy<a->miny) vgap=a->miny-b->maxy;
    overlap=minf(a->maxy,b->maxy)-maxf(a->miny,b->miny);
    baseline=absf_local(a->maxy-b->maxy);
    if (minh>=small_limit&&maxh/minh>cfg->height_ratio_limit) return 0;
    if (hgap>cfg->horizontal_gap_factor*maxh+0.20f*(wa+wb)) return 0;
    if (vgap>cfg->vertical_gap_factor*maxh) return 0;
    if (overlap<0.05f*minh&&baseline>cfg->baseline_factor*maxh&&minh>=small_limit)
        return 0;
    return 1;
}

static int member_less(OcrU32 a,OcrU32 b,const OcrStrokeView*s,
                       const OcrBounds*bounds) {
    OcrI64 ta=s[a].completed_ms,tb=s[b].completed_ms;
    if (ta>0&&tb>0&&ta!=tb) return ta<tb;
    if (bounds[a].minx!=bounds[b].minx) return bounds[a].minx<bounds[b].minx;
    if (bounds[a].miny!=bounds[b].miny) return bounds[a].miny<bounds[b].miny;
    return a<b;
}

static int segment_less(const OcrSegment*a,const OcrSegment*b) {
    float ah=maxf(a->bounds.maxy-a->bounds.miny,0.5f);
    float bh=maxf(b->bounds.maxy-b->bounds.miny,0.5f);
    float tol=minf(ah,bh)*0.45f;
    if (a->bounds.miny+tol<b->bounds.miny) return 1;
    if (b->bounds.miny+tol<a->bounds.miny) return 0;
    return a->bounds.minx<b->bounds.minx;
}

void ocr_segment_list_free(OcrSegmentList*list) {
    OcrU32 i;
    if (!list) return;
    for (i=0;i<list->count;++i) if (list->items[i].members) free(list->items[i].members);
    if (list->items) free(list->items);
    list->items=0; list->count=0;
}

int ocr_segment_world(const OcrConfig*cfg,const OcrStrokeView*strokes,
                      OcrU32 stroke_count,OcrSegmentList*out) {
    OcrU32 *parent=0,*usize=0,*counts=0;
    OcrI32 *root_map=0;
    OcrU8 *eligible=0;
    OcrBounds *bounds=0;
    OcrU32 i,j,seg_count=0;
    int rc=OCR_OK;
    if (!cfg||!strokes||!out) return OCR_ERR_INVALID;
    out->items=0; out->count=0;
    if (!stroke_count) return OCR_OK;
    if (stroke_count>65536u||cfg->max_segment_strokes==0u) return OCR_ERR_LIMIT;
    parent=(OcrU32*)malloc((OcrSize)stroke_count*sizeof(OcrU32));
    usize=(OcrU32*)malloc((OcrSize)stroke_count*sizeof(OcrU32));
    eligible=(OcrU8*)calloc(stroke_count,1);
    bounds=(OcrBounds*)malloc((OcrSize)stroke_count*sizeof(OcrBounds));
    root_map=(OcrI32*)malloc((OcrSize)stroke_count*sizeof(OcrI32));
    if (!parent||!usize||!eligible||!bounds||!root_map) {rc=OCR_ERR_NOMEM;goto done;}
    for (i=0;i<stroke_count;++i) {
        parent[i]=i; usize[i]=1u; root_map[i]=-1;
        if (strokes[i].active&&strokes[i].point_count>=cfg->min_points_per_stroke&&
            ocr_stroke_bounds(strokes+i,bounds+i)==OCR_OK) eligible[i]=1u;
    }
    for (i=0;i<stroke_count;++i) if (eligible[i]) {
        for (j=i+1u;j<stroke_count;++j) if (eligible[j]&&
            time_compatible(cfg,strokes+i,strokes+j)&&
            spatially_compatible(cfg,bounds+i,bounds+j))
            uf_join(parent,usize,i,j,cfg->max_segment_strokes);
    }
    for (i=0;i<stroke_count;++i) if (eligible[i]) {
        OcrU32 r=uf_find(parent,i);
        if (root_map[r]<0) root_map[r]=(OcrI32)seg_count++;
    }
    if (!seg_count) goto done;
    out->items=(OcrSegment*)calloc(seg_count,sizeof(OcrSegment));
    counts=(OcrU32*)calloc(seg_count,sizeof(OcrU32));
    if (!out->items||!counts) {rc=OCR_ERR_NOMEM;goto done;}
    out->count=seg_count;
    for (i=0;i<stroke_count;++i) if (eligible[i]) {
        OcrU32 r=uf_find(parent,i),s=(OcrU32)root_map[r]; counts[s]++;
    }
    for (i=0;i<seg_count;++i) {
        out->items[i].members=(OcrU32*)malloc((OcrSize)counts[i]*sizeof(OcrU32));
        if (!out->items[i].members) {rc=OCR_ERR_NOMEM;goto done;}
        out->items[i].member_count=0;
    }
    for (i=0;i<stroke_count;++i) if (eligible[i]) {
        OcrU32 r=uf_find(parent,i),s=(OcrU32)root_map[r];
        OcrSegment*seg=out->items+s;
        seg->members[seg->member_count++]=i;
        if (seg->member_count==1u) seg->bounds=bounds[i];
        else {
            seg->bounds.minx=minf(seg->bounds.minx,bounds[i].minx);
            seg->bounds.miny=minf(seg->bounds.miny,bounds[i].miny);
            seg->bounds.maxx=maxf(seg->bounds.maxx,bounds[i].maxx);
            seg->bounds.maxy=maxf(seg->bounds.maxy,bounds[i].maxy);
        }
        if (strokes[i].completed_ms>seg->latest_completed_ms)
            seg->latest_completed_ms=strokes[i].completed_ms;
    }
    for (i=0;i<seg_count;++i) {
        OcrSegment*seg=out->items+i;
        OcrU32 a;
        for (a=1;a<seg->member_count;++a) {
            OcrU32 key=seg->members[a],b=a;
            while (b>0u&&member_less(key,seg->members[b-1u],strokes,bounds)) {
                seg->members[b]=seg->members[b-1u]; --b;
            }
            seg->members[b]=key;
        }
    }
    for (i=1;i<seg_count;++i) {
        OcrSegment key=out->items[i]; j=i;
        while (j>0u&&segment_less(&key,&out->items[j-1u])) {
            out->items[j]=out->items[j-1u]; --j;
        }
        out->items[j]=key;
    }
done:
    if (rc!=OCR_OK) ocr_segment_list_free(out);
    if (parent) free(parent);
    if (usize) free(usize);
    if (counts) free(counts);
    if (eligible) free(eligible);
    if (bounds) free(bounds);
    if (root_map) free(root_map);
    return rc;
}
