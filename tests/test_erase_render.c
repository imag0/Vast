#define VAST_GPU 1
#define main legacy_main_not_run
#include "../test_v30.c"
#undef main
#define glActiveTexture unused_glActiveTexture
#define glBindTexture unused_glBindTexture
#define glPixelStorei unused_glPixelStorei
#define glTexSubImage2D unused_glTexSubImage2D
#include "../stubs/gles2_stub.c"
#undef glActiveTexture
#undef glBindTexture
#undef glPixelStorei
#undef glTexSubImage2D
#include "../stubs/egl_stub.c"
int32_t ANativeWindow_getWidth(ANativeWindow*w){return 3000;}
int32_t ANativeWindow_getHeight(ANativeWindow*w){return 1920;}
int32_t ANativeWindow_setBuffersGeometry(ANativeWindow*w,int32_t a,int32_t b,int32_t c){return 0;}
static int uploads;static size_t pixels;
void glActiveTexture(GLenum t){}
void glBindTexture(GLenum t,GLuint id){}
void glPixelStorei(GLenum p,GLint v){}
void glTexSubImage2D(GLenum t,GLint l,GLint x,GLint y,GLsizei w,GLsizei h,GLenum f,GLenum type,const void*p){uploads++;pixels+=(size_t)w*h;}
int main(void){
    memset(&G,0,sizeof(G));G.scale=G.uiScale=1;G.screenW=VG.cw=3000;G.screenH=VG.ch=1920;G.visInk=G.visMarker=1;G.currentStroke=-1;G.tool=MODE_ERASE;
    ensure_strokes(1003);G.strokeN=1003;
    for(int i=0;i<G.strokeN;i++){Stroke*s=&G.strokes[i];s->active=1;s->baseWidth=i==1?24:4;s->color=i==1?0x4288ee33:0xffffff;float x=i<3?1100:20000+i*200,y=i<3?900+i*20:20000;append_point(s,x,y,.8f);append_point(s,x+80,y,.8f);}
    VG.canvasBuf=malloc((size_t)3000*1920*4);uint32_t*expected=malloc((size_t)3000*1920*4);Surf s={(uint8_t*)VG.canvasBuf,3000,1920,3000,FORMAT_RGBA_LAYER};
    VG.cacheScale=1;VG.cacheValid=1;clear_transparent(&s);draw_canvas_world(&s);gpu_remember_content();G.erasing=G.stylusDown=1;
    erase_at(10,10);check(gpu_refresh_erase()&&uploads==0&&!gpu_content_changed(),"erase miss performs no canvas upload");
    erase_at(1120,890);check(!G.strokes[0].active&&G.strokes[1].active,"fixture deletes only target stroke");
    check(gpu_refresh_erase()&&uploads==1&&pixels<3000*1920/3,"erase damage uploads only bounded affected region");
    s.bits=(uint8_t*)expected;clear_transparent(&s);draw_canvas_world(&s);int same=1;for(int i=0;i<3000*1920;i++)if(expected[i]!=VG.canvasBuf[i]){same=0;break;}check(same,"damage repaint equals full compositing including translucent marker");
    uploads=0;erase_at(1120,890);check(gpu_refresh_erase()&&uploads==0,"repeated sample on deleted ink reuses canvas");
    G.strokes[0].active=1;free_action(&G.eraseAction);G.erasing=0;G.sceneRevision++;check(gpu_content_changed(),"cancel invalidates retained deletion pixels");
    float timings[3];int hz[]={60,90,120};for(int j=0;j<3;j++){float x=0,v=0;ANIM_STEP_SECONDS=1.0f/hz[j];for(int i=0;i<hz[j]/5;i++)spring_step(&x,&v,1,0,0);timings[j]=x;}check(fabsf(timings[0]-timings[2])<.001&&fabsf(timings[1]-timings[2])<.001,"motion is refresh-rate independent");
    float x=.7f,v=5;ANIM_STEP_SECONDS=.008f;spring_step(&x,&v,0,0,0);check(x>=0&&x<.7f,"closing retargets immediately without overshoot");
    printf("erase render regression failures=%d\n",fail);return fail;
}
