/* Real production incremental raster/packing; GL calls are counted, not timed.
 * Build with function/data sections and --gc-sections to discard Android UI. */
#define VAST_GPU 1
#define main legacy_main_not_run
#include "../test_v30.c"
#undef main
static int uploads;static size_t uploaded;
void glActiveTexture(GLenum t){(void)t;}
void glBindTexture(GLenum t,GLuint id){(void)t;(void)id;}
void glPixelStorei(GLenum p,GLint v){(void)p;(void)v;}
void glTexSubImage2D(GLenum t,GLint l,GLint x,GLint y,GLsizei w,GLsizei h,GLenum f,GLenum type,const void*p){(void)t;(void)l;(void)x;(void)y;(void)f;(void)type;(void)p;uploads++;uploaded+=(size_t)w*h;}
static double now_us(void){timespec_t t;clock_gettime(CLOCK_MONOTONIC,&t);return (double)t.tv_sec*1000000+(double)t.tv_nsec/1000;}
static void report(const char*s){extern long write(int,const void*,size_t);write(1,s,str_len_local(s));}
int main(void){
    memset(&G,0,sizeof(G));memset(&VG,0,sizeof(VG));
    G.screenW=VG.liveW=3000;G.screenH=VG.liveH=1920;G.scale=1;G.uiScale=1;G.stylusDown=1;G.currentStroke=0;G.strokeN=1;
    VG.liveUsable=1;VG.liveMask=malloc(3000*1920);VG.liveTail=malloc(3000*1920);
    Stroke st={0};G.strokes=&st;st.active=1;st.color=0xeeeeee;st.baseWidth=5.5f;st.pts=malloc(20032*sizeof(Point));
    if(!st.pts||!VG.liveMask||!VG.liveTail)return 1;memset(VG.liveMask,0,3000*1920);memset(VG.liveTail,0,3000*1920);
    for(int i=0;i<20032;i++)st.pts[i]=(Point){1500+700*cosf(i*.002f),960+600*sinf(i*.002f),.65f};
    int counts[]={20,200,2000,20000};int failures=0;
    for(int k=0;k<4;k++){double total=0;size_t bytes=0;
        for(int repeat=0;repeat<50;repeat++){st.n=counts[k]-1;gpu_live_begin(0);int before=VG.liveNextStable;st.n++;uploads=0;uploaded=0;double start=now_us();gpu_live_append(&st);total+=now_us()-start;bytes+=uploaded;
            if(VG.liveNextStable!=before+1||VG.liveNextStable!=st.n-1||uploads!=1||uploaded>=4096)failures++;
            uploads=0;gpu_live_append(&st);if(uploads)failures++;
        }
        char line[256];snprintf(line,sizeof(line),"incremental points=%d append_raster_pack_us=%.3f upload_bytes=%lu repeats=50 screen=3000x1920\n",counts[k],total/50,(unsigned long)(bytes/50));report(line);
    }
    /* Stable + provisional result is identical whether batched or appended. */
    st.n=1;gpu_live_begin(0);for(int n=2;n<=2000;n++){st.n=n;gpu_live_append(&st);}
    uint8_t*expected=malloc(3000*1920);if(!expected)return 1;
    for(int i=0;i<3000*1920;i++)expected[i]=maxi(VG.liveMask[i],VG.liveTail[i]);
    gpu_live_begin(0);for(int i=0;i<3000*1920;i++)if(expected[i]!=maxi(VG.liveMask[i],VG.liveTail[i])){failures++;break;}
    G.stylusDown=0;gpu_live_sync();if(VG.liveActive)failures++;
    G.stylusDown=1;st.n=20;gpu_live_sync();if(!VG.liveActive||VG.liveLastN!=20||VG.liveNextStable!=19)failures++;
    G.offX=12;gpu_live_sync();if(VG.liveOffX!=12||VG.liveNextStable!=19)failures++;
    /* Flat marker coverage also advances once and matches a batched render. */
    st.color=0x42ffee00;st.baseWidth=24;st.n=1;gpu_live_begin(0);
    for(int n=2;n<=2000;n++){st.n=n;uploads=0;gpu_live_append(&st);if(VG.liveNextStable!=n-1||uploads>1)failures++;}
    for(int i=0;i<3000*1920;i++)expected[i]=maxi(VG.liveMask[i],VG.liveTail[i]);
    gpu_live_begin(0);for(int i=0;i<3000*1920;i++)if(expected[i]!=maxi(VG.liveMask[i],VG.liveTail[i])){failures++;break;}
    char result[150];snprintf(result,sizeof(result),"incremental regression failures=%d (constant work, pen/marker batch equivalence, UP, shrink, camera)\n",failures);report(result);
    free(expected);free(st.pts);free(VG.liveMask);free(VG.liveTail);free(VG.liveUpload);return failures?1:0;
}
#if defined(__aarch64__)
void _start(void){int result=main();syscall(93,result);}
#endif
