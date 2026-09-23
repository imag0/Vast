#include "ocr_core.h"

extern void *malloc(OcrSize);
extern void free(void *);
extern void *memset(void *,int,OcrSize);
extern float sqrtf(float);
extern float floorf(float);
extern float ceilf(float);

static float minf(float a,float b){return a<b?a:b;}
static float maxf(float a,float b){return a>b?a:b;}
static float clampf(float v,float a,float b){return v<a?a:(v>b?b:v);}
static OcrI32 mini(OcrI32 a,OcrI32 b){return a<b?a:b;}
static OcrI32 maxi(OcrI32 a,OcrI32 b){return a>b?a:b;}

static float point_segment_distance(float px,float py,float ax,float ay,
                                    float bx,float by) {
    float vx=bx-ax,vy=by-ay,wx=px-ax,wy=py-ay;
    float den=vx*vx+vy*vy;
    float t=den>1.0e-12f?(wx*vx+wy*vy)/den:0.0f;
    float dx,dy;
    t=clampf(t,0.0f,1.0f);
    dx=px-(ax+t*vx); dy=py-(ay+t*vy);
    return sqrtf(dx*dx+dy*dy);
}

static void darken(OcrGrayImage*im,OcrI32 x,OcrI32 y,float coverage) {
    OcrU8 *p;
    OcrI32 v;
    if ((OcrU32)x>=im->width||(OcrU32)y>=im->height||coverage<=0.0f) return;
    coverage=clampf(coverage,0.0f,1.0f);
    v=(OcrI32)(255.0f*(1.0f-coverage)+0.5f);
    p=im->pixels+(OcrSize)y*im->stride+(OcrU32)x;
    if (v<(OcrI32)*p) *p=(OcrU8)v;
}

static void draw_capsule(OcrGrayImage*im,float ax,float ay,float ar,
                         float bx,float by,float br) {
    float r=maxf(ar,br),pad=r+1.25f;
    OcrI32 x0=maxi(0,(OcrI32)floorf(minf(ax,bx)-pad));
    OcrI32 y0=maxi(0,(OcrI32)floorf(minf(ay,by)-pad));
    OcrI32 x1=mini((OcrI32)im->width-1,(OcrI32)ceilf(maxf(ax,bx)+pad));
    OcrI32 y1=mini((OcrI32)im->height-1,(OcrI32)ceilf(maxf(ay,by)+pad));
    float vx=bx-ax,vy=by-ay,den=vx*vx+vy*vy;
    OcrI32 x,y;
    for (y=y0;y<=y1;++y) for (x=x0;x<=x1;++x) {
        float px=(float)x+0.5f,py=(float)y+0.5f;
        float t=den>1.0e-12f?((px-ax)*vx+(py-ay)*vy)/den:0.0f;
        float radius,dist,cov;
        t=clampf(t,0.0f,1.0f);
        radius=ar+(br-ar)*t;
        dist=point_segment_distance(px,py,ax,ay,bx,by);
        cov=radius+0.5f-dist;
        darken(im,x,y,cov);
    }
}

void ocr_gray_image_free(OcrGrayImage*image) {
    if (!image) return;
    if (image->pixels) free(image->pixels);
    image->pixels=0; image->width=image->height=image->stride=0;
    image->source_bounds=(OcrBounds){0,0,0,0};
}

int ocr_rasterize_segment(const OcrConfig*cfg,const OcrStrokeView*strokes,
                          OcrU32 stroke_count,const OcrSegment*segment,
                          OcrGrayImage*out) {
    OcrBounds b={0,0,0,0};
    float content_w,content_h,avail_h,avail_w,scale,left,top;
    OcrU32 i,width,height,pad;
    int have=0;
    if (!cfg||!strokes||!segment||!out||!segment->members||
        !segment->member_count||cfg->raster_height<8u||
        cfg->raster_max_width<8u) return OCR_ERR_INVALID;
    out->pixels=0; out->width=out->height=out->stride=0;
    for (i=0;i<segment->member_count;++i) {
        OcrU32 si=segment->members[i]; OcrBounds sb;
        if (si>=stroke_count||ocr_stroke_bounds(strokes+si,&sb)!=OCR_OK)
            return OCR_ERR_INVALID;
        if (!have) {b=sb;have=1;}
        else {
            b.minx=minf(b.minx,sb.minx); b.miny=minf(b.miny,sb.miny);
            b.maxx=maxf(b.maxx,sb.maxx); b.maxy=maxf(b.maxy,sb.maxy);
        }
    }
    pad=cfg->raster_padding;
    if (pad*2u+2u>=cfg->raster_height||pad*2u+2u>=cfg->raster_max_width)
        return OCR_ERR_INVALID;
    content_w=maxf(b.maxx-b.minx,0.5f);
    content_h=maxf(b.maxy-b.miny,0.5f);
    avail_h=(float)(cfg->raster_height-pad*2u);
    avail_w=(float)(cfg->raster_max_width-pad*2u);
    scale=avail_h/content_h;
    if (content_w*scale>avail_w) scale=avail_w/content_w;
    if (!(scale>0.0f)||scale>100000.0f) return OCR_ERR_LIMIT;
    width=(OcrU32)ceilf(content_w*scale)+(pad*2u);
    if (width<8u) width=8u;
    if (width>cfg->raster_max_width) width=cfg->raster_max_width;
    height=cfg->raster_height;
    if ((OcrSize)width>~(OcrSize)0/(OcrSize)height) return OCR_ERR_LIMIT;
    out->pixels=(OcrU8*)malloc((OcrSize)width*height);
    if (!out->pixels) return OCR_ERR_NOMEM;
    out->width=width; out->height=height; out->stride=width; out->source_bounds=b;
    memset(out->pixels,255,(OcrSize)width*height);
    left=((float)width-content_w*scale)*0.5f;
    top=((float)height-content_h*scale)*0.5f;
    for (i=0;i<segment->member_count;++i) {
        OcrU32 si=segment->members[i],k;
        const OcrStrokeView*st=strokes+si;
        float bw=st->base_width>0.0f?st->base_width:2.0f;
        if (!st->points||!st->point_count) continue;
        if (st->point_count==1u) {
            float p=clampf(st->points[0].p,0.05f,2.0f);
            float x=(st->points[0].x-b.minx)*scale+left;
            float y=(st->points[0].y-b.miny)*scale+top;
            float r=clampf(bw*p*scale*0.5f,cfg->raster_min_radius_px,
                           cfg->raster_max_radius_px);
            draw_capsule(out,x,y,r,x+0.001f,y+0.001f,r);
            continue;
        }
        for (k=1;k<st->point_count;++k) {
            const OcrPoint*a=st->points+k-1u,*q=st->points+k;
            float ax=(a->x-b.minx)*scale+left,ay=(a->y-b.miny)*scale+top;
            float bx=(q->x-b.minx)*scale+left,by=(q->y-b.miny)*scale+top;
            float ar=clampf(bw*clampf(a->p,0.05f,2.0f)*scale*0.5f,
                              cfg->raster_min_radius_px,cfg->raster_max_radius_px);
            float br=clampf(bw*clampf(q->p,0.05f,2.0f)*scale*0.5f,
                              cfg->raster_min_radius_px,cfg->raster_max_radius_px);
            draw_capsule(out,ax,ay,ar,bx,by,br);
        }
    }
    return OCR_OK;
}
