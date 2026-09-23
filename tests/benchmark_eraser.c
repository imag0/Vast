/* Production erase lookup and old full-cache preparation, no GPU driver time. */
#define main legacy_main_not_run
#include "../test_v30.c"
#undef main
static void*bench_alloc(size_t n,size_t s){void*p=malloc(n*s);if(p)memset(p,0,n*s);return p;}
static double bench_us(void){timespec_t t;clock_gettime(CLOCK_MONOTONIC,&t);return t.tv_sec*1000000.0+t.tv_nsec/1000.0;}
static void emit(const char*s){extern long write(int,const void*,size_t);write(1,s,str_len_local(s));}
int main(void){
    memset(&G,0,sizeof(G));G.scale=G.uiScale=1;G.screenW=3000;G.screenH=1920;G.visInk=1;G.currentStroke=-1;
    int counts[]={10,200,2000,10000};
    for(int c=0;c<4;c++){
        G.strokeN=counts[c];G.strokes=bench_alloc(G.strokeN,sizeof(Stroke));
        for(int i=0;i<G.strokeN;i++){Stroke*s=&G.strokes[i];s->n=64;s->pts=malloc(s->n*sizeof(Point));s->active=1;s->baseWidth=4;s->color=0xffffff;
            for(int k=0;k<s->n;k++)s->pts[k]=(Point){10000+(i%100)*200+k,10000+(i/100)*200+10*sinf(k*.3f),.6f};s->minx=s->pts[0].x;s->maxx=s->minx+64;s->miny=s->pts[0].y-10;s->maxy=s->miny+20;
        }
        /* Local stroke's bbox overlaps, but none of its segments hits the probe. */
        Stroke*s=&G.strokes[0];for(int k=0;k<64;k++)s->pts[k]=(Point){900+k*2,700+100*sinf(k*.1f),.6f};s->minx=900;s->maxx=1028;s->miny=600;s->maxy=800;
#ifdef VAST_ERASE_INDEX
        erase_index_prepare();
#endif
        double start=bench_us();for(int n=0;n<10000;n++)erase_at(1000,800);double time=(bench_us()-start)/10000;
        char line[180];snprintf(line,sizeof(line),"erase strokes=%d lookup_us=%.3f repeats=10000\n",G.strokeN,time);emit(line);
        for(int i=0;i<G.strokeN;i++)free(G.strokes[i].pts);free(G.strokes);G.strokes=0;
#ifdef VAST_ERASE_INDEX
        erase_index_reset();
#endif
    }
    G.strokeN=1;G.strokes=bench_alloc(1,sizeof(Stroke));Stroke*s=G.strokes;s->n=200000;s->pts=malloc(s->n*sizeof(Point));s->active=1;s->baseWidth=4;s->color=0xffffff;
    for(int i=0;i<s->n;i++)s->pts[i]=(Point){500+800*cosf(i*.002f),900+600*sinf(i*.002f),.6f};s->minx=-300;s->maxx=1300;s->miny=300;s->maxy=1500;
#ifdef VAST_ERASE_INDEX
    erase_index_prepare();
#endif
    double start=bench_us();for(int n=0;n<500;n++)erase_at(500,900);char line[180];snprintf(line,sizeof(line),"erase points=200000 inside_bbox_miss_us=%.3f repeats=500\n",(bench_us()-start)/500);emit(line);
    uint32_t*pixels=malloc((size_t)3000*1920*4);Surf surface={(uint8_t*)pixels,3000,1920,3000,FORMAT_RGBA_LAYER};G.stylusDown=G.erasing=1;
    start=bench_us();for(int n=0;n<5;n++){clear_transparent(&surface);draw_canvas_world(&surface);}snprintf(line,sizeof(line),"erase legacy_full_scene_prepare_us=%.3f repeats=5\n",(bench_us()-start)/5);emit(line);
    free(pixels);free(s->pts);free(G.strokes);return 0;
}
#if defined(__aarch64__)
void _start(void){syscall(93,main());}
#endif
