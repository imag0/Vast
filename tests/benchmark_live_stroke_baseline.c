/* The old full-stroke algorithm is retained as the allocation-failure fallback.
 * This reproduces that CPU raster+full-clear path without a historical checkout.
 * Not digitizer latency or Android frame times. */
#define main vast_baseline_main_not_run
#include "../test_v30.c"
#undef main

static double now_us(void) {
    timespec_t t;
    clock_gettime(CLOCK_MONOTONIC, &t);
    return (double)t.tv_sec * 1000000.0 + (double)t.tv_nsec / 1000.0;
}

int main(void) {
    memset(&G, 0, sizeof(G));
    G.screenW = 3000; G.screenH = 1920; G.scale = 1;
    G.uiScale = 1; G.strokeSmoothing = .54f; G.gpuActive = 1;
    G.stylusDown = 1; G.currentStroke = 0; G.strokeN = 1;
    Stroke st = {0}; G.strokes = &st;
    st.active = 1; st.color = 0xeeeeee; st.baseWidth = 5.5f;
    st.pts = malloc(20032 * sizeof(Point));
    uint32_t *pixels = malloc(3000 * 1920 * 4);
    if (!st.pts || !pixels) return 1;
    Surf surface = {(uint8_t *)pixels,3000,1920,3000,FORMAT_RGBA_LAYER};
    for (int i = 0; i < 20032; ++i)
        st.pts[i] = (Point){1500+700*cosf(i*.002f),960+600*sinf(i*.002f),.65f};
    int counts[] = {20,200,2000,20000};
    for (int k=0;k<4;++k) {
        st.n=counts[k];
        double start=now_us();
        for(int repeat=0;repeat<20;++repeat) {
            clear_transparent(&surface);
            draw_live_stroke_gpu_overlay(&surface);
        }
        char report[256];
        int length=snprintf(report,sizeof(report),"baseline points=%d raster_and_clear_us=%.3f repeats=20 screen=3000x1920\n",
               st.n,(now_us()-start)/20.0);
        extern long write(int,const void*,size_t);
        write(1,report,(size_t)length);
    }
    free(pixels);free(st.pts);return 0;
}
#if defined(__aarch64__)
void _start(void) { int result=main(); syscall(93,result); }
#endif
