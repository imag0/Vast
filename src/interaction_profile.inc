/* Aggregate timings only: never logs coordinates, handwriting or document text.
 * Event age uses the same CLOCK_MONOTONIC nanosecond domain as NDK MotionEvent. */
extern void* dlopen(const char*,int);extern void* dlsym(void*,const char*);
static double perf_us(void){timespec_t t;clock_gettime(CLOCK_MONOTONIC,&t);return (double)t.tv_sec*1000000+(double)t.tv_nsec/1000;}
static struct {uint64_t samples,available,events,frames;double inputUs,geometryUs,liveUs,frameUs,maxFrameUs,maxAgeUs;int maxBatch;double since;} IP;
static char PERF_LINES[128][512];static int PERF_LINE_NEXT,PERF_LINE_COUNT,PERF_DIRTY;
static void perf_dump(void){if(!PERF_DIRTY||!G.activity||!G.activity->externalDataPath)return;char path[768];snprintf(path,sizeof(path),"%s/interaction-profile.txt",G.activity->externalDataPath);void*f=fopen(path,"wb");if(!f)return;for(int i=0;i<PERF_LINE_COUNT;i++){const char*p=PERF_LINES[(PERF_LINE_NEXT-PERF_LINE_COUNT+i+128)%128];int n=0;while(p[n])n++;fwrite(p,1,n,f);}fclose(f);PERF_DIRTY=0;}
static void perf_report(void){
    double now=perf_us();if(!IP.since){IP.since=now;return;}if(now-IP.since<1000000)return;
    if(IP.events||IP.frames){snprintf(PERF_LINES[PERF_LINE_NEXT],512,"interval_ms=%.1f samples=%llu rate_hz=%.1f received=%llu events=%llu input_us_per_sample=%.2f input_us_per_event=%.2f geometry_ms=%.3f live_mask_ms=%.3f frames=%llu frame_mean_ms=%.3f frame_max_ms=%.3f event_age_max_ms=%.3f queue_batch_max=%d\n",(now-IP.since)/1000,IP.samples,IP.samples*1000000/(now-IP.since),IP.available,IP.events,IP.samples?IP.inputUs/IP.samples:0,IP.inputUs/(IP.events?IP.events:1),IP.geometryUs/1000,IP.liveUs/1000,IP.frames,IP.frameUs/(1000*(IP.frames?IP.frames:1)),IP.maxFrameUs/1000,IP.maxAgeUs/1000,IP.maxBatch);PERF_LINE_NEXT=(PERF_LINE_NEXT+1)%128;if(PERF_LINE_COUNT<128)PERF_LINE_COUNT++;PERF_DIRTY=1;}
#if defined(__ANDROID__)
    static int(*logprint)(int,const char*,const char*,...);static int tried;
    if(!tried){tried=1;void*h=dlopen("liblog.so",2);if(h)logprint=(int(*)(int,const char*,const char*,...))dlsym(h,"__android_log_print");}
    if(logprint&&(IP.events||IP.frames))logprint(4,"VastPerf","%s",PERF_LINES[(PERF_LINE_NEXT+127)%128]);
#endif
    memset(&IP,0,sizeof(IP));IP.since=now;
}
