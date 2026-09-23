#define VAST_GPU 1
#define main legacy_main_not_run
#include "../test_v30.c"
#undef main
#define glBufferSubData mock_buffer_subdata
#define glBufferData mock_buffer_data
#define glTexImage2D mock_tex_image
#define glTexSubImage2D mock_tex_sub_image
#include "../stubs/gles2_stub.c"
#undef glBufferSubData
#undef glBufferData
#undef glTexImage2D
#undef glTexSubImage2D
#include "../stubs/egl_stub.c"
int32_t ANativeWindow_getWidth(ANativeWindow*w){return 3000;}
int32_t ANativeWindow_getHeight(ANativeWindow*w){return 1920;}
int32_t ANativeWindow_setBuffersGeometry(ANativeWindow*w,int32_t a,int32_t b,int32_t c){return 0;}
static int uploads,allocations,textures;static long bytes;
void glTexImage2D(GLenum t,GLint l,GLint in,GLsizei w,GLsizei h,GLint b,GLenum f,GLenum ty,const void*d){textures++;}
void glTexSubImage2D(GLenum t,GLint l,GLint x,GLint y,GLsizei w,GLsizei h,GLenum f,GLenum ty,const void*d){textures++;}
void glBufferSubData(GLenum t,long offset,long n,const void*p){(void)t;(void)p;if(offset<0||offset+n>sizeof(GI_VERTICES))fail=1;uploads++;bytes+=n;}
void glBufferData(GLenum t,long n,const void*p,GLenum usage){(void)t;(void)p;(void)usage;allocations++;if(n!=sizeof(GI_VERTICES))fail=1;}
int main(void){
    memset(&G,0,sizeof(G));G.scale=1;G.screenW=3000;G.screenH=1920;G.strokeSmoothing=.7f;G.pressureSmoothing=.5f;G.pressureMin=.2f;G.pressureMax=1;G.currentStroke=-1;
    Stroke s={0};s.active=1;s.baseWidth=5;s.pts=malloc(20001*sizeof(Point));s.cap=20001;InkMesh mesh={0};
    for(int i=0;i<20001;i++)s.pts[i]=(Point){1500+650*cosf(i*.035f),900+500*sinf(i*.035f),.6f};
    int bounded=1;double early=0,late=0;
    for(int n=1;n<=20000;n++){s.n=n;uploads=0;bytes=0;double begin=perf_us();gi_sync_mesh(&mesh,&s);double time=perf_us()-begin;if(n>10&&n<=110)early+=time;if(n>19900)late+=time;if(bytes>64*192||uploads>2||mesh.n!=n)bounded=0;}
    check(bounded,"20k-sample append uses bounded geometry and at most two chunk uploads");
    printf("append mean_us early=%.3f late=%.3f allocations=%d chunks_fixed_bytes=%lu\n",early/100,late/100,allocations,(unsigned long)sizeof(GI_VERTICES));
    uint64_t built=GI.segmentsBuilt;uploads=0;for(int i=0;i<1000;i++){G.scale=.05f+19.95f*i/999;G.offX=i*10;G.offY=-i*4;gi_sync_mesh(&mesh,&s);}
    check(uploads==0&&GI.segmentsBuilt==built,"pan and full 0.05x-20x zoom never rebuild or upload cached ink");
    int monotone=1;for(int i=2;i<20000;i+=13){Point a=s.pts[i-1],b=s.pts[i],p=s.pts[i-2];for(int k=0;k<=64;k++){Point q=ink_curve_point(p,a,b,k/64.f,0);if(q.x<fmin2(a.x,b.x)-.001f||q.x>fmax2(a.x,b.x)+.001f||q.y<fmin2(a.y,b.y)-.001f||q.y>fmax2(a.y,b.y)+.001f)monotone=0;}}
    check(monotone,"causal local curves stay inside endpoint bounds without overshoot");
    gi_free_mesh(&mesh);free(s.pts);G.scale=1;G.offX=G.offY=0;G.sampleTimeNs=1000000000;start_stroke(0,0,.5f);int noLag=1;for(int i=1;i<=2000;i++){G.sampleTimeNs+=4000000;add_stroke_point(i*10.f,0,.6f);if(fabsf(G.smoothWorldX-i*10.f)>.001f)noLag=0;}
    check(noLag&&G.strokes[0].n==2001,"fast input reaches current sample and stores physical samples once");
    cancel_current_stroke();G.zenMode=1;G.tool=MODE_PEN;FakeEvent e;ev1(&e,AMOTION_ACTION_DOWN,TOOL_STYLUS,900,500,.6f);e.t=10000;handle_motion((AInputEvent*)&e);
    e.action=AMOTION_ACTION_MOVE;e.t=10012;e.hist=2;for(int j=0;j<2;j++){e.ht[j]=10004+j*4;e.hx[j][0]=910+j*10;e.hy[j][0]=500;e.hp[j][0]=.6f;}e.x[0]=930;handle_motion((AInputEvent*)&e);
    e.action=AMOTION_ACTION_UP;e.t=10024;for(int j=0;j<2;j++){e.ht[j]=10016+j*4;e.hx[j][0]=940+j*10;}e.x[0]=960;handle_motion((AInputEvent*)&e);
    check(G.strokes[0].n==7&&G.strokes[0].pts[6].x==960&&G.previousSampleNs==10024000000LL,"MOVE and UP consume timestamped historical samples before current sample");
    check(G.inkSavePending&&G.ocrPending,"pen-up schedules optional persistence and OCR after interaction");
    G.scale=G.uiScale=1;G.offX=G.offY=0;G.visShapes=G.visPhotos=G.visNotes=1;G.shapeN=1;
    G.shapes[0]=(ShapeObj){.active=1,.type=SHAPE_ELLIPSE,.x0=200,.y0=200,.x1=500,.y1=350,.width=4,.color=0x336699};
    uint32_t imagePixels[4]={0xff0000,0x00ff00,0x0000ff,0xffffff};G.imageN=1;
    G.images[0]=(ImageObj){.active=1,.px=imagePixels,.pw=2,.ph=2,.x=300,.y=300,.w=100,.h=100,.rot=.6f};
    scene_shapes();scene_images();built=GI.segmentsBuilt;uploads=textures=0;
    for(int i=0;i<1000;i++){G.scale=.05f+19.95f*i/999;G.offX=1500-350*G.scale;G.offY=900-300*G.scale;scene_shapes();scene_images();}
    check(!uploads&&!textures&&built==GI.segmentsBuilt,"cached shapes and rotated photos do not rebuild or upload during camera movement");
    G.imageContentRevision++;scene_images();check(textures==1,"image content revision invalidates reused pixel allocation exactly once");
    G.noteN=1;G.notes[0]=(NoteObj){.active=1,.id=1,.x=300,.y=250,.w=120,.h=80};
    GN[0].tiles=malloc(sizeof(GLuint));GN[0].tiles[0]=1;GN[0].count=1;GN[0].w=96;GN[0].h=60;GN[0].id=1;GN[0].revision=G.sceneRevision;
    textures=0;for(int i=0;i<1000;i++){G.scale=.05f+19.95f*i/999;G.offX=1500-350*G.scale;G.offY=900-300*G.scale;gn_draw();}
    check(!textures,"retained note tiles reuse layout and texture across full zoom range");
    VG.worldInk=1;VG.cacheValid=1;
    check(gpu_camera_inside(),"full scene camera path never asks for a bitmap repaint");
    scene_destroy();gn_destroy();
    return fail?1:0;
}
