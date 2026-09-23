#ifdef VAST_GPU
// Vast 2.0 hybrid GPU compositor. The document/brush engine stays CPU-side;
// OpenGL ES owns the spatial canvas, camera reprojection, atmosphere and glass.
typedef void* EGLDisplay; typedef void* EGLConfig; typedef void* EGLSurface; typedef void* EGLContext; typedef void* EGLNativeDisplayType; typedef void* EGLNativeWindowType; typedef int EGLint; typedef unsigned int EGLBoolean;
typedef unsigned int GLenum,GLuint,GLbitfield; typedef int GLint,GLsizei; typedef float GLfloat; typedef char GLchar; typedef unsigned char GLboolean; typedef void GLvoid;
extern EGLDisplay eglGetDisplay(EGLNativeDisplayType); extern EGLBoolean eglGetConfigAttrib(EGLDisplay,EGLConfig,EGLint,EGLint*); extern EGLBoolean eglInitialize(EGLDisplay,EGLint*,EGLint*); extern EGLBoolean eglChooseConfig(EGLDisplay,const EGLint*,EGLConfig*,EGLint,EGLint*); extern EGLSurface eglCreateWindowSurface(EGLDisplay,EGLConfig,EGLNativeWindowType,const EGLint*); extern EGLContext eglCreateContext(EGLDisplay,EGLConfig,EGLContext,const EGLint*); extern EGLBoolean eglMakeCurrent(EGLDisplay,EGLSurface,EGLSurface,EGLContext); extern EGLBoolean eglSwapBuffers(EGLDisplay,EGLSurface); extern EGLBoolean eglDestroySurface(EGLDisplay,EGLSurface); extern EGLBoolean eglDestroyContext(EGLDisplay,EGLContext); extern EGLBoolean eglTerminate(EGLDisplay); extern EGLBoolean eglSwapInterval(EGLDisplay,EGLint); extern EGLBoolean eglQuerySurface(EGLDisplay,EGLSurface,EGLint,EGLint*);
extern void glGetIntegerv(GLenum,GLint*); extern void glViewport(GLint,GLint,GLsizei,GLsizei); extern void glClearColor(GLfloat,GLfloat,GLfloat,GLfloat); extern void glClear(GLbitfield); extern GLuint glCreateShader(GLenum); extern void glShaderSource(GLuint,GLsizei,const GLchar*const*,const GLint*); extern void glCompileShader(GLuint); extern void glGetShaderiv(GLuint,GLenum,GLint*); extern void glGetShaderInfoLog(GLuint,GLsizei,GLsizei*,GLchar*); extern void glDeleteShader(GLuint); extern GLuint glCreateProgram(void); extern void glAttachShader(GLuint,GLuint); extern void glBindAttribLocation(GLuint,GLuint,const GLchar*); extern void glLinkProgram(GLuint); extern void glGetProgramiv(GLuint,GLenum,GLint*); extern void glGetProgramInfoLog(GLuint,GLsizei,GLsizei*,GLchar*); extern void glDeleteProgram(GLuint); extern void glUseProgram(GLuint); extern GLint glGetUniformLocation(GLuint,const GLchar*); extern void glUniform1i(GLint,GLint); extern void glUniform1f(GLint,GLfloat); extern void glUniform2f(GLint,GLfloat,GLfloat); extern void glUniform3f(GLint,GLfloat,GLfloat,GLfloat); extern void glUniform4f(GLint,GLfloat,GLfloat,GLfloat,GLfloat); extern void glGenTextures(GLsizei,GLuint*); extern void glDeleteTextures(GLsizei,const GLuint*); extern void glBindTexture(GLenum,GLuint); extern void glTexParameteri(GLenum,GLenum,GLint); extern void glTexImage2D(GLenum,GLint,GLint,GLsizei,GLsizei,GLint,GLenum,GLenum,const GLvoid*); extern void glTexSubImage2D(GLenum,GLint,GLint,GLint,GLsizei,GLsizei,GLenum,GLenum,const GLvoid*); extern void glActiveTexture(GLenum); extern void glPixelStorei(GLenum,GLint); extern void glEnableVertexAttribArray(GLuint); extern void glVertexAttribPointer(GLuint,GLint,GLenum,GLboolean,GLsizei,const GLvoid*); extern void glDrawArrays(GLenum,GLint,GLsizei); extern void glDisableVertexAttribArray(GLuint);
extern int32_t ANativeWindow_getWidth(ANativeWindow*); extern int32_t ANativeWindow_getHeight(ANativeWindow*);

#define EGL_DEFAULT_DISPLAY ((void*)0)
#define EGL_NO_DISPLAY ((void*)0)
#define EGL_NO_SURFACE ((void*)0)
#define EGL_NO_CONTEXT ((void*)0)
#define EGL_SURFACE_TYPE 0x3033
#define EGL_WINDOW_BIT 0x0004
#define EGL_RENDERABLE_TYPE 0x3040
#define EGL_OPENGL_ES2_BIT 0x0004
#define EGL_RED_SIZE 0x3024
#define EGL_GREEN_SIZE 0x3023
#define EGL_BLUE_SIZE 0x3022
#define EGL_ALPHA_SIZE 0x3021
#define EGL_NONE 0x3038
#define EGL_CONTEXT_CLIENT_VERSION 0x3098
#define EGL_WIDTH 0x3057
#define EGL_HEIGHT 0x3056
#define EGL_NATIVE_VISUAL_ID 0x302E
#define GL_VERTEX_SHADER 0x8B31
#define GL_FRAGMENT_SHADER 0x8B30
#define GL_COMPILE_STATUS 0x8B81
#define GL_LINK_STATUS 0x8B82
#define GL_TEXTURE_2D 0x0DE1
#define GL_TEXTURE_MIN_FILTER 0x2801
#define GL_TEXTURE_MAG_FILTER 0x2800
#define GL_TEXTURE_WRAP_S 0x2802
#define GL_TEXTURE_WRAP_T 0x2803
#define GL_LINEAR 0x2601
#define GL_CLAMP_TO_EDGE 0x812F
#define GL_RGBA 0x1908
#define GL_UNSIGNED_BYTE 0x1401
#define GL_TEXTURE0 0x84C0
#define GL_TEXTURE1 0x84C1
#define GL_TEXTURE2 0x84C2
#define GL_FLOAT 0x1406
#define GL_TRIANGLE_STRIP 0x0005
#define GL_COLOR_BUFFER_BIT 0x4000
#define GL_UNPACK_ALIGNMENT 0x0CF5
#define GL_MAX_TEXTURE_SIZE 0x0D33
#define GL_ALPHA 0x1906

// forward declarations supplied by canvas.c immediately before this include
static void draw_canvas_world(Surf*s);
static void draw_ui_overlay(Surf*s);

typedef struct {
    EGLDisplay dpy; EGLSurface surface; EGLContext ctx; EGLConfig config;
    GLuint prog,canvasTex,uiTex,liveTex; GLint uOverlay; int worldInk; uint32_t lastColor;
    GLint uRes,uCam,uCacheCam,uCacheRes,uBg,uGrid,uGridMajor,uAccent,uCyan,uViolet,uVisual,uVisual2,uTime,uGridSpacing,uCanvas,uUi,uLive,uLiveColor,uEraser,uEraserColor;
    uint32_t *canvasBuf,*uiBuf; int cw,ch,uw,uh,winW,winH;
    uint8_t *liveMask,*liveTail,*liveUpload; size_t liveUploadCap;
    int liveW,liveH,liveUsable,liveActive,liveStroke,liveNextStable,liveLastN;
    int liveTailX0,liveTailY0,liveTailX1,liveTailY1,liveTailValid;
    int liveUsedX0,liveUsedY0,liveUsedX1,liveUsedY1,liveUsedValid;
    float liveScale,liveOffX,liveOffY,liveBaseWidth; uint32_t liveColor;
    float cacheScale,cacheOffX,cacheOffY; int cacheValid,cacheWasInteraction;
    int lastStrokeN,lastCursor,lastImageN,lastMeasureN,lastCurrentN,lastEraseN,lastSelectedImage,lastSelectedMeasure,lastRegionGlow,lastFrameRevision,lastSceneRevision,lastCadUnit,lastCadCalibrated; uint32_t lastAccent,lastCyan,lastViolet; float lastCadScale;
    float lastUiScale,lastOffX,lastOffY,lastScale; int lastSettings,lastGallery,lastPicker,lastCalibration,lastTool,lastPhotoMode,lastRail,lastRadial,lastPressure,lastMinimap,lastStylusDown,lastAdd,lastLayers,lastSearch,lastProjects,lastEditor,lastFramePanel,lastZen,lastPresentation,lastSelectedRefN,lastSelectionMore,lastShapeDrawing,lastLasso,lastShapeType,lastPhotoAngleSnap,lastToast,lastUiSceneRevision; uint32_t lastSelectionHash,lastThemeHash;
    float time; int64_t timeOriginMs;
} VastGpu;
static VastGpu VG;
static uint32_t *erasePixels;static size_t erasePixelsCap;

static void gpu_rgb(GLint loc,uint32_t c){glUniform3f(loc,((c>>16)&255)/255.0f,((c>>8)&255)/255.0f,(c&255)/255.0f);}
static GLuint gpu_shader(GLenum type,const char*src){GLuint s=glCreateShader(type);glShaderSource(s,1,&src,0);glCompileShader(s);GLint ok=0;glGetShaderiv(s,GL_COMPILE_STATUS,&ok);if(!ok){glDeleteShader(s);return 0;}return s;}
static void gpu_tex_setup(GLuint t){glBindTexture(GL_TEXTURE_2D,t);glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MIN_FILTER,GL_LINEAR);glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MAG_FILTER,GL_LINEAR);glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_WRAP_S,GL_CLAMP_TO_EDGE);glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_WRAP_T,GL_CLAMP_TO_EDGE);}

static const char*VAST_VS=
"attribute vec2 aPos; varying vec2 vUv; void main(){ vUv=(aPos+1.0)*0.5; gl_Position=vec4(aPos,0.0,1.0); }";

static const char*VAST_FS=
"#ifdef GL_FRAGMENT_PRECISION_HIGH\nprecision highp float;\n#else\nprecision mediump float;\n#endif\n"
"uniform vec2 uRes; uniform vec3 uCam; uniform vec3 uCacheCam; uniform vec2 uCacheRes;\n"
"uniform vec3 uBg,uGrid,uGridMajor,uAccent,uCyan,uViolet;\n"
"uniform vec4 uVisual; uniform vec4 uVisual2; uniform float uTime,uGridSpacing;\n"
"uniform sampler2D uCanvas,uUi,uLive; uniform vec4 uLiveColor; varying vec2 vUv;\n"
"uniform vec4 uEraser; uniform vec3 uEraserColor; uniform float uOverlay;\n"
"float sat(float x){return clamp(x,0.0,1.0);}\n"
"float gridLine(vec2 w,float sp,float px){vec2 q=abs(fract(w/sp+0.5)-0.5)*sp;float d=min(q.x,q.y);return 1.0-smoothstep(px,px+1.15/max(uCam.z,0.02),d);}\n"
"float gridDot(vec2 w,float sp,float px){vec2 q=(fract(w/sp+0.5)-0.5)*sp;float d=length(q);return 1.0-smoothstep(px,px+1.25/max(uCam.z,0.02),d);}\n"
"vec4 canvasAt(vec2 p){vec2 w=(p-uCam.xy)/uCam.z;vec2 cp=w*uCacheCam.z+uCacheCam.xy;vec2 uv=cp/uCacheRes;if(uv.x<0.0||uv.y<0.0||uv.x>1.0||uv.y>1.0)return vec4(0.0);return texture2D(uCanvas,uv);}\n"
"void main(){\n"
" vec2 p=vec2(gl_FragCoord.x,uRes.y-gl_FragCoord.y); vec2 uv=p/uRes; float edge=min(uv.x,1.0-uv.x);\n"
" float glassAmt=uVisual.x/4.0; float edgeMask=(1.0-smoothstep(0.018,0.18,edge))*glassAmt; vec2 pGlass=p;\n"
" vec2 w=(pGlass-uCam.xy)/uCam.z; vec3 bg=uBg; float atmosphere=uVisual.y;\n"
" if(atmosphere>0.01){float vig=smoothstep(0.98,0.28,length((uv-0.5)*vec2(1.25,1.0)));float haze=0.5+0.5*sin(w.x*0.0016+w.y*0.0012+uTime*0.05);bg=mix(bg,mix(uAccent,uViolet,haze),0.020*atmosphere*vig);bg*=0.90+0.10*vig;}\n"
" float gd=uVisual.z; float style=uVisual.w; float inv=max(uCam.z,0.02); float a=0.0,b=0.0,c=0.0,d=0.0;\n"
" float s0=max(uGridSpacing,1.0); float s0b=s0*2.0; float s1=s0*4.0; float s2=s1*4.0; float s3=s2*4.0;\n"
" float p0=s0*uCam.z,p1=s1*uCam.z,p2=s2*uCam.z,p3=s3*uCam.z; float xf=1.0-smoothstep(38.0,68.0,p0); float f1=smoothstep(90.0,150.0,p1); float f2=smoothstep(210.0,380.0,p2); float f3=smoothstep(520.0,900.0,p3);\n"
" if(style<0.5){a=mix(gridDot(w,s0,0.72/inv),gridDot(w,s0b,0.78/inv),xf);b=gridDot(w,s1,0.92/inv)*f1;c=gridDot(w,s2,1.14/inv)*f2;d=gridDot(w,s3,1.46/inv)*f3;}\n"
" else if(style<1.5){a=mix(gridLine(w,s0,0.27/inv),gridLine(w,s0b,0.31/inv),xf);b=gridLine(w,s1,0.38/inv)*f1;c=gridLine(w,s2,0.62/inv)*f2;d=gridLine(w,s3,0.86/inv)*f3;}\n"
" else if(style<2.5){a=mix(gridLine(w,s0,0.31/inv),gridLine(w,s0b,0.35/inv),xf);b=gridLine(w,s1,0.46/inv)*f1;c=gridLine(w,s2,0.76/inv)*f2;d=gridLine(w,s3,1.05/inv)*f3;}\n"
" else {a=mix(gridDot(w,s0*2.0,0.42/inv),gridDot(w,s0*4.0,0.48/inv),xf);b=gridDot(w,s1*2.0,0.62/inv)*f1;c=gridDot(w,s2*2.0,0.92/inv)*f2;}\n"
" vec3 col=bg; col=mix(col,uGrid,a*(0.10+0.07*gd));col=mix(col,uGrid,b*(0.16+0.08*gd)); if(gd>0.5)col=mix(col,uGridMajor,c*(0.19+0.09*gd)); if(gd>1.5)col=mix(col,uAccent,d*0.14);\n"
" vec4 cv=canvasAt(pGlass); float region=uVisual2.x; if(region>0.01){vec4 h1=canvasAt(pGlass+vec2(14.0,0.0));vec4 h2=canvasAt(pGlass-vec2(14.0,0.0));vec4 h3=canvasAt(pGlass+vec2(0.0,14.0));vec4 h4=canvasAt(pGlass-vec2(0.0,14.0));float halo=max(max(h1.a,h2.a),max(h3.a,h4.a))*(1.0-cv.a);col=mix(col,mix(uCyan,uViolet,0.42),halo*0.075*region);}\n"
" col=mix(col,cv.rgb,cv.a);\n"
" float elev=uVisual2.y; if(elev>0.01){float rim=(1.0-smoothstep(0.01,0.065,edge))*elev;col+=vec3(rim*0.055);col*=1.0-rim*0.045;}\n"
" if(edgeMask>0.001){float shine=(1.0-smoothstep(0.0,0.026,abs(edge-0.034)))*edgeMask;col+=vec3(shine*(0.08+0.10*glassAmt));}\n"
" float liveA=texture2D(uLive,uv).a*uLiveColor.a; if(uOverlay>-.5)col=mix(col,uLiveColor.rgb,liveA);\n"
" vec4 ui=texture2D(uUi,uv); if(uOverlay>1.5){gl_FragColor=vec4(uLiveColor.rgb,liveA);return;} if(uOverlay>0.5){float ring=(1.0-smoothstep(0.6,1.6,abs(length(p-uEraser.xy)-uEraser.z)))*uEraser.w;float a=ui.a;vec3 rgb=ui.rgb;gl_FragColor=vec4(mix(rgb,uEraserColor,ring),max(a,ring));return;} if(uOverlay<-.5){gl_FragColor=vec4(col,1.);return;}\n"
" col=mix(col,ui.rgb,ui.a); float ring=1.0-smoothstep(0.6,1.6,abs(length(p-uEraser.xy)-uEraser.z)); col=mix(col,uEraserColor,ring*uEraser.w); gl_FragColor=vec4(col,1.0);\n"
"}\n";

#include "gpu_ink.inc"
#include "gpu_notes.inc"
#include "gpu_scene.inc"
static void gpu_free_buffers(void){
    free(erasePixels);erasePixels=0;erasePixelsCap=0;
    if(VG.canvasBuf){free(VG.canvasBuf);VG.canvasBuf=0;}
    if(VG.uiBuf){free(VG.uiBuf);VG.uiBuf=0;}
    if(VG.liveMask){free(VG.liveMask);VG.liveMask=0;}
    if(VG.liveTail){free(VG.liveTail);VG.liveTail=0;}
    if(VG.liveUpload){free(VG.liveUpload);VG.liveUpload=0;}
    VG.liveUploadCap=0;VG.liveW=VG.liveH=0;VG.liveUsable=VG.liveActive=0;VG.liveStroke=-1;VG.liveNextStable=0;VG.liveTailValid=VG.liveUsedValid=0;
    VG.cw=VG.ch=VG.uw=VG.uh=0;VG.cacheValid=0;
}
static void gpu_destroy(void){scene_destroy();gn_destroy();gi_destroy();if(VG.dpy&&VG.dpy!=EGL_NO_DISPLAY){eglMakeCurrent(VG.dpy,EGL_NO_SURFACE,EGL_NO_SURFACE,EGL_NO_CONTEXT);if(VG.canvasTex)glDeleteTextures(1,&VG.canvasTex);if(VG.uiTex)glDeleteTextures(1,&VG.uiTex);if(VG.liveTex)glDeleteTextures(1,&VG.liveTex);if(VG.prog)glDeleteProgram(VG.prog);if(VG.surface)eglDestroySurface(VG.dpy,VG.surface);if(VG.ctx)eglDestroyContext(VG.dpy,VG.ctx);eglTerminate(VG.dpy);}gpu_free_buffers();memset(&VG,0,sizeof(VG));VG.liveStroke=-1;G.gpuActive=0;}

static int gpu_init(void){if(!G.window)return 0;if(VG.dpy&&VG.surface&&VG.ctx)return 1;gpu_destroy();EGLint cfgattr[]={EGL_SURFACE_TYPE,EGL_WINDOW_BIT,EGL_RENDERABLE_TYPE,EGL_OPENGL_ES2_BIT,EGL_RED_SIZE,8,EGL_GREEN_SIZE,8,EGL_BLUE_SIZE,8,EGL_ALPHA_SIZE,8,EGL_NONE};EGLint ctxattr[]={EGL_CONTEXT_CLIENT_VERSION,3,EGL_NONE};EGLint n=0;VG.dpy=eglGetDisplay(EGL_DEFAULT_DISPLAY);if(!VG.dpy||!eglInitialize(VG.dpy,0,0))goto fail;if(!eglChooseConfig(VG.dpy,cfgattr,&VG.config,1,&n)||n<1)goto fail;EGLint visual=0;eglGetConfigAttrib(VG.dpy,VG.config,EGL_NATIVE_VISUAL_ID,&visual);if(visual)ANativeWindow_setBuffersGeometry(G.window,0,0,visual);VG.surface=eglCreateWindowSurface(VG.dpy,VG.config,(EGLNativeWindowType)G.window,0);if(!VG.surface)goto fail;VG.ctx=eglCreateContext(VG.dpy,VG.config,EGL_NO_CONTEXT,ctxattr);if(!VG.ctx){ctxattr[1]=2;VG.ctx=eglCreateContext(VG.dpy,VG.config,EGL_NO_CONTEXT,ctxattr);}if(!VG.ctx)goto fail;VG.worldInk=ctxattr[1]==3;if(!eglMakeCurrent(VG.dpy,VG.surface,VG.surface,VG.ctx))goto fail;eglSwapInterval(VG.dpy,1);GLuint vs=gpu_shader(GL_VERTEX_SHADER,VAST_VS),fs=gpu_shader(GL_FRAGMENT_SHADER,VAST_FS);if(!vs||!fs){if(vs)glDeleteShader(vs);if(fs)glDeleteShader(fs);goto fail;}VG.prog=glCreateProgram();glAttachShader(VG.prog,vs);glAttachShader(VG.prog,fs);glBindAttribLocation(VG.prog,0,"aPos");glLinkProgram(VG.prog);glDeleteShader(vs);glDeleteShader(fs);GLint ok=0;glGetProgramiv(VG.prog,GL_LINK_STATUS,&ok);if(!ok)goto fail;glUseProgram(VG.prog);
#define UL(N) VG.N=glGetUniformLocation(VG.prog,#N)
    UL(uRes);UL(uCam);UL(uCacheCam);UL(uCacheRes);UL(uBg);UL(uGrid);UL(uGridMajor);UL(uAccent);UL(uCyan);UL(uViolet);UL(uVisual);UL(uVisual2);UL(uTime);UL(uGridSpacing);UL(uCanvas);UL(uUi);UL(uLive);UL(uLiveColor);UL(uEraser);UL(uEraserColor);UL(uOverlay);
#undef UL
    glGenTextures(1,&VG.canvasTex);gpu_tex_setup(VG.canvasTex);glGenTextures(1,&VG.uiTex);gpu_tex_setup(VG.uiTex);glGenTextures(1,&VG.liveTex);gpu_tex_setup(VG.liveTex);glUniform1i(VG.uCanvas,0);glUniform1i(VG.uUi,1);glUniform1i(VG.uLive,2);VG.liveStroke=-1;G.gpuActive=1;if(VG.worldInk&&(!gi_init()||!gn_init()))VG.worldInk=0;return 1;
fail: gpu_destroy();return 0;}

static int gpu_alloc(int w,int h){float mult=G.performanceMode==0?1.25f:(G.performanceMode==2?1.62f:1.45f);int cw=(int)(w*mult),ch=(int)(h*mult);cw=(cw+15)&~15;ch=(ch+15)&~15;GLint maxTex=4096;glGetIntegerv(GL_MAX_TEXTURE_SIZE,&maxTex);if(maxTex<2048)maxTex=2048;if(cw>maxTex)cw=maxTex;if(ch>maxTex)ch=maxTex;if(VG.worldInk)cw=ch=1;if(VG.canvasBuf&&VG.uiBuf&&VG.liveMask&&VG.liveTail&&VG.cw==cw&&VG.ch==ch&&VG.uw==w&&VG.uh==h)return 1;gpu_free_buffers();VG.canvasBuf=(uint32_t*)malloc((size_t)cw*ch*4);VG.uiBuf=(uint32_t*)malloc((size_t)w*h*4);VG.liveMask=(uint8_t*)malloc((size_t)w*h);VG.liveTail=(uint8_t*)malloc((size_t)w*h);if(!VG.canvasBuf||!VG.uiBuf||!VG.liveMask||!VG.liveTail){gpu_free_buffers();return 0;}memset(VG.liveMask,0,(size_t)w*h);memset(VG.liveTail,0,(size_t)w*h);VG.cw=cw;VG.ch=ch;VG.uw=w;VG.uh=h;VG.liveW=w;VG.liveH=h;VG.liveUsable=1;VG.liveStroke=-1;glActiveTexture(GL_TEXTURE0);gpu_tex_setup(VG.canvasTex);glTexImage2D(GL_TEXTURE_2D,0,GL_RGBA,cw,ch,0,GL_RGBA,GL_UNSIGNED_BYTE,0);glActiveTexture(GL_TEXTURE1);gpu_tex_setup(VG.uiTex);glTexImage2D(GL_TEXTURE_2D,0,GL_RGBA,w,h,0,GL_RGBA,GL_UNSIGNED_BYTE,0);glActiveTexture(GL_TEXTURE2);gpu_tex_setup(VG.liveTex);glPixelStorei(GL_UNPACK_ALIGNMENT,1);glTexImage2D(GL_TEXTURE_2D,0,GL_ALPHA,w,h,0,GL_ALPHA,GL_UNSIGNED_BYTE,VG.liveMask);glPixelStorei(GL_UNPACK_ALIGNMENT,4);VG.cacheValid=0;return 1;}

/* Live coverage is append-only. Causal pieces use the same bounded world curve
 * as retained GPU geometry and CPU export. No provisional future-looking tail. */
static void gpu_live_union(int*x0,int*y0,int*x1,int*y1,int ax0,int ay0,int ax1,int ay1){
    if(ax1<=ax0||ay1<=ay0)return;if(ax0<*x0)*x0=ax0;if(ay0<*y0)*y0=ay0;if(ax1>*x1)*x1=ax1;if(ay1>*y1)*y1=ay1;
}
static void gpu_live_mark(uint8_t*mask,int x,int y,int a){
    if(!mask||(unsigned)x>=(unsigned)VG.liveW||(unsigned)y>=(unsigned)VG.liveH||a<=0)return;uint8_t*p=mask+(size_t)y*VG.liveW+x;if(a>*p)*p=(uint8_t)(a>255?255:a);
}
static void gpu_live_circle(uint8_t*mask,float cx,float cy,float r,int*x0,int*y0,int*x1,int*y1){
    if(r<.28f)r=.28f;float ro=r+.55f,ri=fmax2(0.0f,r-.50f),ro2=ro*ro,ri2=ri*ri;int ly0=maxi(0,(int)floorf(cy-ro)),ly1=mini(VG.liveH-1,(int)floorf(cy+ro));
    int lx0=maxi(0,(int)floorf(cx-ro)),lx1=mini(VG.liveW-1,(int)floorf(cx+ro));gpu_live_union(x0,y0,x1,y1,lx0,ly0,lx1+1,ly1+1);
    for(int y=ly0;y<=ly1;y++){float py=y+.5f,dy=py-cy,d2y=dy*dy;if(d2y>=ro2)continue;float ho=sqrtf(ro2-d2y);int xl=maxi(0,(int)floorf(cx-ho)),xr=mini(VG.liveW-1,(int)floorf(cx+ho)),fi0=xl,fi1=xr;if(d2y<ri2){float hi=sqrtf(ri2-d2y);fi0=maxi(xl,(int)(cx-hi+.999f));fi1=mini(xr,(int)floorf(cx+hi));}for(int x=fi0;x<=fi1;x++)gpu_live_mark(mask,x,y,255);for(int x=xl;x<fi0;x++){float dx=x+.5f-cx,d2=dx*dx+d2y,cov=clampf(.5f+(r*r-d2)/fmax2(.35f,2*r),0,1);if(cov>0)gpu_live_mark(mask,x,y,(int)(255*cov+.5f));}for(int x=fi1+1;x<=xr;x++){float dx=x+.5f-cx,d2=dx*dx+d2y,cov=clampf(.5f+(r*r-d2)/fmax2(.35f,2*r),0,1);if(cov>0)gpu_live_mark(mask,x,y,(int)(255*cov+.5f));}}
}
static void gpu_live_taper(uint8_t*mask,float ax,float ay,float ar,float bx,float by,float br,int*x0,int*y0,int*x1,int*y1){
    float dx=bx-ax,dy=by-ay,d2=dx*dx+dy*dy;if(d2<.0001f){gpu_live_circle(mask,ax,ay,fmax2(ar,br),x0,y0,x1,y1);return;}float invd=1.0f/sqrtf(d2),ux=dx*invd,uy=dy*invd,nx=-uy,ny=ux;ax-=ux*.85f;ay-=uy*.85f;bx+=ux*.85f;by+=uy*.85f;
    float vx[4]={ax+nx*ar,bx+nx*br,bx-nx*br,ax-nx*ar},vy[4]={ay+ny*ar,by+ny*br,by-ny*br,ay-ny*ar};float area=(vx[1]-vx[0])*(vy[2]-vy[0])-(vy[1]-vy[0])*(vx[2]-vx[0]),sg=area>=0?1.0f:-1.0f;
    float ex[4],ey[4],invlen[4],minx=vx[0],maxx=vx[0],miny=vy[0],maxy=vy[0];for(int i=0;i<4;i++){int j=(i+1)&3;ex[i]=vx[j]-vx[i];ey[i]=vy[j]-vy[i];float l2=ex[i]*ex[i]+ey[i]*ey[i];invlen[i]=l2>.0001f?1.0f/sqrtf(l2):1.0f;minx=fmin2(minx,vx[i]);maxx=fmax2(maxx,vx[i]);miny=fmin2(miny,vy[i]);maxy=fmax2(maxy,vy[i]);}
    int ix0=maxi(0,(int)floorf(minx-1)),ix1=mini(VG.liveW-1,(int)floorf(maxx+1)),iy0=maxi(0,(int)floorf(miny-1)),iy1=mini(VG.liveH-1,(int)floorf(maxy+1));gpu_live_union(x0,y0,x1,y1,ix0,iy0,ix1+1,iy1+1);
    for(int y=iy0;y<=iy1;y++){float py=y+.5f,px=ix0+.5f,e0=sg*(ex[0]*(py-vy[0])-ey[0]*(px-vx[0])),e1=sg*(ex[1]*(py-vy[1])-ey[1]*(px-vx[1])),e2=sg*(ex[2]*(py-vy[2])-ey[2]*(px-vx[2])),e3=sg*(ex[3]*(py-vy[3])-ey[3]*(px-vx[3]));float d0=-sg*ey[0],d1=-sg*ey[1],d2x=-sg*ey[2],d3=-sg*ey[3];for(int x=ix0;x<=ix1;x++,e0+=d0,e1+=d1,e2+=d2x,e3+=d3){if(e1*invlen[1]<-.02f||e3*invlen[3]<-.02f)continue;float md=e0*invlen[0],q=e2*invlen[2];if(q<md)md=q;if(md<=-.55f)continue;gpu_live_mark(mask,x,y,(int)(255*clampf(md+.55f,0,1)+.5f));}}
}
static float gpu_live_cr(float p0,float p1,float p2,float p3,float t){float t2=t*t,t3=t2*t;return .5f*((2*p1)+(-p0+p2)*t+(2*p0-5*p1+4*p2-p3)*t2+(-p0+3*p1-3*p2+p3)*t3);}
static void gpu_live_curve(uint8_t*mask,const Stroke*st,Point p0,Point p1,Point p2,Point p3,int*x0,int*y0,int*x1,int*y1){
    float x0s=p1.x*VG.liveScale+VG.liveOffX,y0s=p1.y*VG.liveScale+VG.liveOffY,x2s=p2.x*VG.liveScale+VG.liveOffX,y2s=p2.y*VG.liveScale+VG.liveOffY;int first=p0.x==p1.x&&p0.y==p1.y;int steps=ink_curve_steps(p0,p1,p2,first);int aa=(int)((st->color>>24)&255),marker=aa>0&&aa<248;float maxr=marker?110.0f:80.0f,px=x0s,py=y0s,pp=clampf(p1.p,.02f,1.8f),r0=clampf(st->baseWidth*pp*VG.liveScale*.5f,.48f,maxr);gpu_live_circle(mask,px,py,r0,x0,y0,x1,y1);
    for(int j=1;j<=steps;j++){float t=(float)j/(float)steps;Point cp=ink_curve_point(p0,p1,p2,t,first);float wx=cp.x,wy=cp.y,pr=cp.p;float x=wx*VG.liveScale+VG.liveOffX,y=wy*VG.liveScale+VG.liveOffY,r1=clampf(st->baseWidth*pr*VG.liveScale*.5f,.48f,maxr);gpu_live_taper(mask,px,py,r0,x,y,r1,x0,y0,x1,y1);gpu_live_circle(mask,x,y,r1,x0,y0,x1,y1);px=x;py=y;r0=r1;}
}
static void gpu_live_marker_piece(uint8_t*mask,const Stroke*st,Point a,Point b,int*x0,int*y0,int*x1,int*y1){
    float ax=a.x*VG.liveScale+VG.liveOffX,ay=a.y*VG.liveScale+VG.liveOffY,bx=b.x*VG.liveScale+VG.liveOffX,by=b.y*VG.liveScale+VG.liveOffY,rr=clampf(st->baseWidth*VG.liveScale*.5f,.65f,110.0f);gpu_live_taper(mask,ax,ay,rr,bx,by,rr,x0,y0,x1,y1);
}
static void gpu_live_clear_tail(int*x0,int*y0,int*x1,int*y1){
    if(!VG.liveTailValid)return;gpu_live_union(x0,y0,x1,y1,VG.liveTailX0,VG.liveTailY0,VG.liveTailX1,VG.liveTailY1);for(int y=VG.liveTailY0;y<VG.liveTailY1;y++)memset(VG.liveTail+(size_t)y*VG.liveW+VG.liveTailX0,0,(size_t)(VG.liveTailX1-VG.liveTailX0));VG.liveTailValid=0;
}
static int gpu_live_upload_rect(int x0,int y0,int x1,int y1){
    x0=maxi(0,x0);y0=maxi(0,y0);x1=mini(VG.liveW,x1);y1=mini(VG.liveH,y1);if(x1<=x0||y1<=y0)return 1;int w=x1-x0,h=y1-y0;size_t need=(size_t)w*h;if(need>VG.liveUploadCap){size_t nc=VG.liveUploadCap?VG.liveUploadCap:4096;while(nc<need){if(nc>((size_t)-1)/2){nc=need;break;}nc*=2;}uint8_t*p=(uint8_t*)realloc(VG.liveUpload,nc);if(!p){VG.liveUsable=VG.liveActive=0;return 0;}VG.liveUpload=p;VG.liveUploadCap=nc;}for(int y=0;y<h;y++){uint8_t*d=VG.liveUpload+(size_t)y*w,*a=VG.liveMask+(size_t)(y0+y)*VG.liveW+x0,*b=VG.liveTail+(size_t)(y0+y)*VG.liveW+x0;for(int x=0;x<w;x++)d[x]=a[x]>b[x]?a[x]:b[x];}glActiveTexture(GL_TEXTURE2);glBindTexture(GL_TEXTURE_2D,VG.liveTex);glPixelStorei(GL_UNPACK_ALIGNMENT,1);glTexSubImage2D(GL_TEXTURE_2D,0,x0,y0,w,h,GL_ALPHA,GL_UNSIGNED_BYTE,VG.liveUpload);glPixelStorei(GL_UNPACK_ALIGNMENT,4);return 1;
}
static void gpu_live_append(const Stroke*st){
    if(!VG.liveActive||!st||st->n<=0||st->n==VG.liveLastN)return;int dx0=VG.liveW,dy0=VG.liveH,dx1=0,dy1=0;gpu_live_clear_tail(&dx0,&dy0,&dx1,&dy1);
    int aa=(int)((st->color>>24)&255),marker=aa>0&&aa<248;int tx0=VG.liveW,ty0=VG.liveH,tx1=0,ty1=0;
    if(marker){
        while(VG.liveNextStable+1<st->n){int j=VG.liveNextStable;gpu_live_marker_piece(VG.liveMask,st,st->pts[j],st->pts[j+1],&dx0,&dy0,&dx1,&dy1);VG.liveNextStable++;}
        if(st->n==1){Point p=st->pts[0],a=p,b=p;a.x-=.6f/fmax2(VG.liveScale,.0001f);b.x+=.6f/fmax2(VG.liveScale,.0001f);gpu_live_marker_piece(VG.liveTail,st,a,b,&tx0,&ty0,&tx1,&ty1);}
    }else{
        while(VG.liveNextStable+1<st->n){int j=VG.liveNextStable;Point p0=st->pts[j>0?j-1:j],p1=st->pts[j],p2=st->pts[j+1],p3=p2;gpu_live_curve(VG.liveMask,st,p0,p1,p2,p3,&dx0,&dy0,&dx1,&dy1);VG.liveNextStable++;}
        if(st->n==1){Point p=st->pts[0];float r=clampf(st->baseWidth*clampf(p.p,.02f,1.8f)*VG.liveScale*.5f,.48f,80.0f);gpu_live_circle(VG.liveMask,p.x*VG.liveScale+VG.liveOffX,p.y*VG.liveScale+VG.liveOffY,r,&dx0,&dy0,&dx1,&dy1);}
    }
    if(tx1>tx0&&ty1>ty0){VG.liveTailX0=tx0;VG.liveTailY0=ty0;VG.liveTailX1=tx1;VG.liveTailY1=ty1;VG.liveTailValid=1;gpu_live_union(&dx0,&dy0,&dx1,&dy1,tx0,ty0,tx1,ty1);}VG.liveLastN=st->n;if(!gpu_live_upload_rect(dx0,dy0,dx1,dy1)){VG.liveActive=0;return;}if(dx1>dx0&&dy1>dy0){if(!VG.liveUsedValid){VG.liveUsedX0=dx0;VG.liveUsedY0=dy0;VG.liveUsedX1=dx1;VG.liveUsedY1=dy1;VG.liveUsedValid=1;}else gpu_live_union(&VG.liveUsedX0,&VG.liveUsedY0,&VG.liveUsedX1,&VG.liveUsedY1,dx0,dy0,dx1,dy1);}
}
static int gpu_live_begin(int stroke){
    if(!VG.liveUsable||stroke<0||stroke>=G.strokeN)return 0;Stroke*st=&G.strokes[stroke];if(!st->active||st->n<=0)return 0;if(VG.liveUsedValid){int x0=VG.liveUsedX0,y0=VG.liveUsedY0,x1=VG.liveUsedX1,y1=VG.liveUsedY1;for(int y=y0;y<y1;y++){memset(VG.liveMask+(size_t)y*VG.liveW+x0,0,(size_t)(x1-x0));memset(VG.liveTail+(size_t)y*VG.liveW+x0,0,(size_t)(x1-x0));}if(!gpu_live_upload_rect(x0,y0,x1,y1))return 0;}VG.liveUsedValid=0;VG.liveActive=1;VG.liveStroke=stroke;VG.liveNextStable=0;VG.liveLastN=0;VG.liveTailValid=0;VG.liveScale=G.scale;VG.liveOffX=G.offX;VG.liveOffY=G.offY;VG.liveColor=st->color;VG.liveBaseWidth=st->baseWidth;gpu_live_append(st);return VG.liveActive;
}
static void gpu_live_sync(void){
    int stroke=G.currentStroke;int wanted=G.stylusDown&&!G.erasing&&!G.measuring&&stroke>=0&&stroke<G.strokeN&&G.strokes[stroke].active&&G.strokes[stroke].n>0;if(!wanted){VG.liveActive=0;VG.liveStroke=-1;VG.liveTailValid=0;return;}Stroke*st=&G.strokes[stroke];if(!VG.liveUsable)return;if(!VG.liveActive||VG.liveStroke!=stroke||VG.liveScale!=G.scale||VG.liveOffX!=G.offX||VG.liveOffY!=G.offY||VG.liveColor!=st->color||VG.liveBaseWidth!=st->baseWidth||st->n<VG.liveLastN){gpu_live_begin(stroke);return;}gpu_live_append(st);
}

static uint32_t gpu_selection_hash(void){uint32_t h=2166136261u;for(int i=0;i<G.selectedRefN;i++){h=(h^(uint32_t)G.selectedRefs[i].type)*16777619u;h=(h^(uint32_t)G.selectedRefs[i].index)*16777619u;}return h;}
static uint32_t gpu_theme_hash(void){Theme*t=&G.theme;uint32_t h=t->bg;h=h*33u^t->grid;h=h*33u^t->gridMajor;h=h*33u^t->axis;h=h*33u^t->glass;h=h*33u^t->glass2;h=h*33u^t->glass3;h=h*33u^t->border;h=h*33u^t->text;h=h*33u^t->muted;h=h*33u^t->accent;h=h*33u^t->cyan;h=h*33u^t->violet;h=h*33u^t->warn;h=h*33u^t->danger;h=h*33u^t->green;h=h*33u^t->black;return h;}
static int gpu_content_changed(void){int curN=(G.currentStroke>=0&&G.currentStroke<G.strokeN)?G.strokes[G.currentStroke].n:-1;int en=G.eraseAction.n;int liveInk=G.stylusDown&&!G.erasing&&!G.measuring&&G.currentStroke>=0;if(!VG.cacheValid)return 1;if(VG.lastImageN!=G.imageN||VG.lastMeasureN!=G.measureN||(!VG.worldInk&&VG.lastEraseN!=en)||VG.lastSelectedImage!=G.selectedImage||VG.lastSelectedMeasure!=G.selectedMeasure||VG.lastRegionGlow!=G.regionGlow||VG.lastFrameRevision!=G.frameRevision||VG.lastSceneRevision!=G.sceneRevision||VG.lastAccent!=C_ACCENT||VG.lastCyan!=C_CYAN||VG.lastViolet!=C_VIOLET||VG.lastCadUnit!=G.cadUnit||VG.lastCadCalibrated!=G.cadCalibrated||VG.lastCadScale!=G.cadScale)return 1;if(!VG.worldInk&&!liveInk&&(VG.lastStrokeN!=G.strokeN||VG.lastCursor!=G.actionCursor||VG.lastCurrentN!=curN||(!G.erasing&&G.saveDirty)))return 1;if(G.imagesDirty||G.measuring||G.imageDragging||G.imageGesture)return 1;return 0;}
static int gpu_camera_inside(void){if(!VG.cacheValid)return 0;if(VG.worldInk)return 1;float ratio=G.scale/VG.cacheScale;float lo=G.performanceMode==2?.66f:(G.performanceMode==0?.82f:.74f),hi=G.performanceMode==2?1.52f:(G.performanceMode==0?1.22f:1.38f);if(ratio<lo||ratio>hi)return 0;float margin=48.0f;float sx[4]={0,(float)G.screenW,0,(float)G.screenW},sy[4]={0,0,(float)G.screenH,(float)G.screenH};for(int i=0;i<4;i++){float wx=(sx[i]-G.offX)/G.scale,wy=(sy[i]-G.offY)/G.scale;float cx=wx*VG.cacheScale+VG.cacheOffX,cy=wy*VG.cacheScale+VG.cacheOffY;if(cx<margin||cy<margin||cx>VG.cw-margin||cy>VG.ch-margin)return 0;}return 1;}
static void gpu_remember_content(void){VG.lastStrokeN=G.strokeN;VG.lastCursor=G.actionCursor;VG.lastImageN=G.imageN;VG.lastMeasureN=G.measureN;VG.lastCurrentN=(G.currentStroke>=0&&G.currentStroke<G.strokeN)?G.strokes[G.currentStroke].n:-1;VG.lastEraseN=G.eraseAction.n;VG.lastSelectedImage=G.selectedImage;VG.lastSelectedMeasure=G.selectedMeasure;VG.lastRegionGlow=G.regionGlow;VG.lastFrameRevision=G.frameRevision;VG.lastSceneRevision=G.sceneRevision;VG.lastAccent=C_ACCENT;VG.lastCyan=C_CYAN;VG.lastViolet=C_VIOLET;VG.lastCadUnit=G.cadUnit;VG.lastCadCalibrated=G.cadCalibrated;VG.lastCadScale=G.cadScale;}
static void gpu_refresh_canvas(void){if(VG.worldInk){VG.canvasBuf[0]=0;glActiveTexture(GL_TEXTURE0);glBindTexture(GL_TEXTURE_2D,VG.canvasTex);glTexSubImage2D(GL_TEXTURE_2D,0,0,0,1,1,GL_RGBA,GL_UNSIGNED_BYTE,VG.canvasBuf);VG.cacheScale=1;VG.cacheOffX=VG.cacheOffY=0;VG.cacheValid=1;gpu_remember_content();return;}float os=G.scale,oox=G.offX,ooy=G.offY;int osw=G.screenW,osh=G.screenH;float cx=(osw*.5f-oox)/os,cy=(osh*.5f-ooy)/os;VG.cacheScale=os;VG.cacheOffX=VG.cw*.5f-cx*VG.cacheScale;VG.cacheOffY=VG.ch*.5f-cy*VG.cacheScale;G.scale=VG.cacheScale;G.offX=VG.cacheOffX;G.offY=VG.cacheOffY;G.screenW=VG.cw;G.screenH=VG.ch;int hidden=-1,wasActive=0;if(VG.liveActive&&G.currentStroke>=0&&G.currentStroke<G.strokeN){hidden=G.currentStroke;wasActive=G.strokes[hidden].active;G.strokes[hidden].active=0;}Surf s={(uint8_t*)VG.canvasBuf,VG.cw,VG.ch,VG.cw,FORMAT_RGBA_LAYER};clear_transparent(&s);GPU_WORLD_INK=VG.worldInk;draw_canvas_world(&s);GPU_WORLD_INK=0;if(hidden>=0)G.strokes[hidden].active=wasActive;G.scale=os;G.offX=oox;G.offY=ooy;G.screenW=osw;G.screenH=osh;glActiveTexture(GL_TEXTURE0);glBindTexture(GL_TEXTURE_2D,VG.canvasTex);glPixelStorei(GL_UNPACK_ALIGNMENT,4);glTexSubImage2D(GL_TEXTURE_2D,0,0,0,VG.cw,VG.ch,GL_RGBA,GL_UNSIGNED_BYTE,VG.canvasBuf);VG.cacheValid=1;VG.cacheWasInteraction=(G.fingerCount>0);gpu_remember_content();}
static int gpu_ui_changed(void){if(VG.lastColor!=G.color||VG.lastUiScale!=G.uiScale||VG.lastSettings!=G.settingsPanel||VG.lastGallery!=G.galleryOpen||VG.lastPicker!=G.colorPickerOpen||VG.lastCalibration!=G.calibrationOpen||VG.lastTool!=(int)G.tool||VG.lastPhotoMode!=G.photoMode||VG.lastRail!=G.railOpen||VG.lastRadial!=G.radialOpen||VG.lastPressure!=G.pressurePanel||VG.lastMinimap!=G.minimap||VG.lastStylusDown!=G.stylusDown||VG.lastAdd!=G.addPanel||VG.lastLayers!=G.layersPanel||VG.lastSearch!=G.searchPanel||VG.lastProjects!=G.projectsPanel||VG.lastEditor!=G.editorOpen||VG.lastFramePanel!=G.framePanel||VG.lastZen!=G.zenMode||VG.lastPresentation!=G.presentationMode||VG.lastSelectedRefN!=G.selectedRefN||VG.lastSelectionMore!=G.selectionMoreOpen||VG.lastShapeDrawing!=G.shapeDrawing||VG.lastLasso!=G.lassoActive||VG.lastShapeType!=G.shapeType||VG.lastPhotoAngleSnap!=G.photoAngleSnap||VG.lastToast!=G.toast||VG.lastUiSceneRevision!=G.sceneRevision||VG.lastSelectionHash!=gpu_selection_hash()||VG.lastThemeHash!=gpu_theme_hash()||(((G.minimap&&!G.zenMode)||G.selectedRefN||G.ocrHighlightAlpha>0||G.frameCreating||G.presentationMode)&&(VG.lastOffX!=G.offX||VG.lastOffY!=G.offY||VG.lastScale!=G.scale)))return 1;return 0;}
static void gpu_refresh_ui(void){VG.lastColor=G.color;Surf s={(uint8_t*)VG.uiBuf,G.screenW,G.screenH,G.screenW,FORMAT_RGBA_LAYER};clear_transparent(&s);int ga=G.gpuActive;if(VG.liveActive||VG.worldInk)G.gpuActive=0;draw_ui_overlay(&s);G.gpuActive=ga;glActiveTexture(GL_TEXTURE1);glBindTexture(GL_TEXTURE_2D,VG.uiTex);glPixelStorei(GL_UNPACK_ALIGNMENT,4);glTexSubImage2D(GL_TEXTURE_2D,0,0,0,G.screenW,G.screenH,GL_RGBA,GL_UNSIGNED_BYTE,VG.uiBuf);VG.lastUiScale=G.uiScale;VG.lastSettings=G.settingsPanel;VG.lastGallery=G.galleryOpen;VG.lastPicker=G.colorPickerOpen;VG.lastCalibration=G.calibrationOpen;VG.lastTool=(int)G.tool;VG.lastPhotoMode=G.photoMode;VG.lastRail=G.railOpen;VG.lastRadial=G.radialOpen;VG.lastPressure=G.pressurePanel;VG.lastMinimap=G.minimap;VG.lastStylusDown=G.stylusDown;VG.lastAdd=G.addPanel;VG.lastLayers=G.layersPanel;VG.lastSearch=G.searchPanel;VG.lastProjects=G.projectsPanel;VG.lastEditor=G.editorOpen;VG.lastFramePanel=G.framePanel;VG.lastZen=G.zenMode;VG.lastPresentation=G.presentationMode;VG.lastSelectedRefN=G.selectedRefN;VG.lastSelectionMore=G.selectionMoreOpen;VG.lastShapeDrawing=G.shapeDrawing;VG.lastLasso=G.lassoActive;VG.lastShapeType=G.shapeType;VG.lastPhotoAngleSnap=G.photoAngleSnap;VG.lastToast=G.toast;VG.lastUiSceneRevision=G.sceneRevision;VG.lastSelectionHash=gpu_selection_hash();VG.lastThemeHash=gpu_theme_hash();VG.lastOffX=G.offX;VG.lastOffY=G.offY;VG.lastScale=G.scale;}

#include "gpu_erase.inc"
static int gpu_render_frame(void){if(!G.window)return 0;if(!gpu_init())return 0;EGLint w=0,h=0;eglQuerySurface(VG.dpy,VG.surface,EGL_WIDTH,&w);eglQuerySurface(VG.dpy,VG.surface,EGL_HEIGHT,&h);if(w<=0||h<=0){w=ANativeWindow_getWidth(G.window);h=ANativeWindow_getHeight(G.window);}if(w<=0||h<=0)return 0;VG.winW=w;VG.winH=h;G.screenW=w;G.screenH=h;if(!G.viewInitialized){G.scale=1.0f;G.offX=w*.18f;G.offY=h*.18f;G.viewInitialized=1;}if(!gpu_alloc(w,h))return 0;
    double liveStart=perf_us();gpu_live_sync();IP.liveUs+=perf_us()-liveStart;
    /* Committing an already-painted erase action changes history, not pixels. */
    if(!G.erasing&&VG.lastEraseN>0&&G.actionCursor==VG.lastCursor+1&&G.actionCursor<=G.actionN&&G.actions[G.actionCursor-1].type==ACT_ERASE&&G.actions[G.actionCursor-1].n==VG.lastEraseN&&VG.lastSceneRevision==G.sceneRevision)gpu_remember_content();
    int mmw=us(320),mmh=us(205),mmp=us(14),mmt=us(40),mcw=mmw-2*mmp,mch=mmh-mmt-mmp;if((G.minimap||G.minimapAnim>.01f)&&(G.minimapDirty||G.minimapCacheW!=mcw||G.minimapCacheH!=mch)&&G.fingerCount==0&&!G.stylusDown&&monotonic_ms()-G.interactionMs>=400)rebuild_minimap_cache(mcw,mch);
    int interacting=(G.flingActive||G.fingerCount>0||G.stylusDown||G.imageDragging||G.imageGesture||G.measuring||G.selectionDragging||G.selectionGesture||G.shapeDrawing||G.lassoActive);
    if(!VG.worldInk&&G.erasing&&gpu_camera_inside())gpu_refresh_erase();
    if(!VG.cacheValid||(!VG.worldInk&&(gpu_content_changed()||!gpu_camera_inside()||(!interacting&&VG.cacheWasInteraction))))gpu_refresh_canvas();
    if((G.stylusDown&&!VG.worldInk&&!VG.liveActive&&!G.erasing)||G.imageDragging||G.imageGesture||G.measuring||G.uiAnimating||G.radialOpen||gpu_ui_changed()||(!android_ui_available()&&G.colorPickerOpen)||G.presentationMode||G.selectionDragging||G.selectionGesture||G.shapeDrawing||G.lassoActive)gpu_refresh_ui();
    glUseProgram(VG.prog);glUniform4f(VG.uEraser,G.hoverX,G.hoverY,26.0f,eraser_contact_visible()?1.0f:0.0f);gpu_rgb(VG.uEraserColor,C_DANGER);
    glViewport(0,0,w,h);glClearColor(0,0,0,1);glClear(GL_COLOR_BUFFER_BIT);glUseProgram(VG.prog);glUniform1f(VG.uOverlay,VG.worldInk?-1:0);int64_t frameMs=monotonic_ms();if(VG.timeOriginMs<=0)VG.timeOriginMs=frameMs;VG.time=(float)(frameMs-VG.timeOriginMs)*.001f;glUniform2f(VG.uRes,(float)w,(float)h);glUniform3f(VG.uCam,G.offX,G.offY,G.scale);glUniform3f(VG.uCacheCam,VG.cacheOffX,VG.cacheOffY,VG.cacheScale);glUniform2f(VG.uCacheRes,(float)VG.cw,(float)VG.ch);gpu_rgb(VG.uBg,C_BG);gpu_rgb(VG.uGrid,C_GRID);gpu_rgb(VG.uGridMajor,C_GRID_MAJOR);gpu_rgb(VG.uAccent,C_ACCENT);gpu_rgb(VG.uCyan,C_CYAN);gpu_rgb(VG.uViolet,C_VIOLET);glUniform4f(VG.uVisual,(float)G.edgeGlass,(float)G.atmosphere,(float)G.gridDepth,(float)G.gridStyle);glUniform4f(VG.uVisual2,(float)G.regionGlow,(float)G.planeElevation,G.motionIntensity,(float)G.farZoomMode);glUniform1f(VG.uTime,VG.time);glUniform1f(VG.uGridSpacing,grid_spacing());if(VG.liveActive){int aa=(int)((VG.liveColor>>24)&255);if(aa==0)aa=255;glUniform4f(VG.uLiveColor,((VG.liveColor>>16)&255)/255.0f,((VG.liveColor>>8)&255)/255.0f,(VG.liveColor&255)/255.0f,aa/255.0f);}else glUniform4f(VG.uLiveColor,0,0,0,0);glActiveTexture(GL_TEXTURE0);glBindTexture(GL_TEXTURE_2D,VG.canvasTex);glActiveTexture(GL_TEXTURE1);glBindTexture(GL_TEXTURE_2D,VG.uiTex);glActiveTexture(GL_TEXTURE2);glBindTexture(GL_TEXTURE_2D,VG.liveTex);static const GLfloat q[]={-1,-1,1,-1,-1,1,1,1};glEnableVertexAttribArray(0);glVertexAttribPointer(0,2,GL_FLOAT,0,0,q);glDrawArrays(GL_TRIANGLE_STRIP,0,4);glDisableVertexAttribArray(0);if(VG.worldInk){
        scene_frames();scene_regions();scene_images();gn_draw();scene_shapes();
        if(!gi_draw()){VG.worldInk=0;VG.cacheValid=0;}
        glUseProgram(VG.prog);glUniform1f(VG.uOverlay,2);glActiveTexture(GL_TEXTURE2);glBindTexture(GL_TEXTURE_2D,VG.liveTex);glBindBuffer(GI_BUFFER,0);glEnable(GI_BLEND);glBlendEquation(GI_ADD);glBlendFunc(GI_SRC_ALPHA,GI_ONE_MINUS_SRC_ALPHA);glEnableVertexAttribArray(0);glVertexAttribPointer(0,2,GL_FLOAT,0,0,q);glDrawArrays(GL_TRIANGLE_STRIP,0,4);glDisableVertexAttribArray(0);glDisable(GI_BLEND);
        scene_measures();scene_constellation();
        glUseProgram(VG.prog);glUniform1f(VG.uOverlay,1);glActiveTexture(GL_TEXTURE1);glBindTexture(GL_TEXTURE_2D,VG.uiTex);glBindBuffer(GI_BUFFER,0);glEnable(GI_BLEND);glBlendEquation(GI_ADD);glBlendFunc(GI_SRC_ALPHA,GI_ONE_MINUS_SRC_ALPHA);glEnableVertexAttribArray(0);glVertexAttribPointer(0,2,GL_FLOAT,0,0,q);glDrawArrays(GL_TRIANGLE_STRIP,0,4);glDisableVertexAttribArray(0);glDisable(GI_BLEND);
    }if(!eglSwapBuffers(VG.dpy,VG.surface)){gpu_destroy();return 0;}return 1;}
#endif
