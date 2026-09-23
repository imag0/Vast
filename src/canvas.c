typedef unsigned long size_t;
typedef signed int int32_t;
typedef unsigned int uint32_t;
typedef unsigned short uint16_t;
typedef unsigned char uint8_t;
typedef signed char int8_t;
typedef signed long long int64_t;
typedef unsigned long long uint64_t;

#include "jni.h"
#include "font_atlas.h"
#include "ocr_manager.h"

// Minimal Android NDK declarations so the project can build without a local Android SDK.
typedef struct ANativeActivity ANativeActivity;
typedef struct ANativeWindow ANativeWindow;
typedef struct AInputQueue AInputQueue;
typedef struct AInputEvent AInputEvent;
typedef struct ALooper ALooper;
typedef struct AAssetManager AAssetManager;

typedef struct { int32_t left, top, right, bottom; } ARect;
typedef struct { int32_t width, height, stride, format; void* bits; uint32_t reserved[6]; } ANativeWindow_Buffer;
typedef struct ANativeActivityCallbacks {
    void (*onStart)(ANativeActivity*); void (*onResume)(ANativeActivity*); void* (*onSaveInstanceState)(ANativeActivity*, size_t*);
    void (*onPause)(ANativeActivity*); void (*onStop)(ANativeActivity*); void (*onDestroy)(ANativeActivity*);
    void (*onWindowFocusChanged)(ANativeActivity*, int); void (*onNativeWindowCreated)(ANativeActivity*, ANativeWindow*);
    void (*onNativeWindowResized)(ANativeActivity*, ANativeWindow*); void (*onNativeWindowRedrawNeeded)(ANativeActivity*, ANativeWindow*);
    void (*onNativeWindowDestroyed)(ANativeActivity*, ANativeWindow*); void (*onInputQueueCreated)(ANativeActivity*, AInputQueue*);
    void (*onInputQueueDestroyed)(ANativeActivity*, AInputQueue*); void (*onContentRectChanged)(ANativeActivity*, const ARect*);
    void (*onConfigurationChanged)(ANativeActivity*); void (*onLowMemory)(ANativeActivity*);
} ANativeActivityCallbacks;
/* Keep this ABI declaration byte-for-byte compatible with
   android/native_activity.h.  The instance slot is between sdkVersion and
   assetManager; omitting it reads instance (normally NULL) as the asset
   manager and silently disables every AAsset-backed subsystem. */
struct ANativeActivity { ANativeActivityCallbacks* callbacks; JavaVM* vm; JNIEnv* env; jobject clazz; const char* internalDataPath; const char* externalDataPath; int32_t sdkVersion; void* instance; AAssetManager* assetManager; const char* obbPath; };

extern void* malloc(size_t); extern void* realloc(void*, size_t); extern void free(void*); extern void* memset(void*, int, size_t); extern void* memcpy(void*, const void*, size_t);
extern int snprintf(char*, size_t, const char*, ...); extern void* fopen(const char*, const char*); extern size_t fread(void*, size_t, size_t, void*); extern size_t fwrite(const void*, size_t, size_t, void*); extern int ferror(void*); extern int fflush(void*); extern int fclose(void*); extern int fileno(void*); extern int fsync(int); extern long syscall(long,...); extern int open(const char*,int,...); extern int rename(const char*,const char*); extern int remove(const char*);
extern float sqrtf(float); extern float floorf(float); extern float ceilf(float); extern float fabsf(float); extern float atan2f(float,float); extern float sinf(float); extern float cosf(float); extern float expf(float);
extern long read(int, void*, size_t); extern int close(int); extern int timerfd_create(int,int);
typedef long time_t; typedef struct { time_t tv_sec; long tv_nsec; } timespec_t; typedef struct { timespec_t it_interval; timespec_t it_value; } itimerspec_t; extern int timerfd_settime(int,int,const itimerspec_t*,itimerspec_t*); extern int clock_gettime(int,timespec_t*);
extern int32_t ANativeWindow_lock(ANativeWindow*, ANativeWindow_Buffer*, ARect*); extern int32_t ANativeWindow_unlockAndPost(ANativeWindow*); extern int32_t ANativeWindow_getWidth(ANativeWindow*); extern int32_t ANativeWindow_getHeight(ANativeWindow*); extern int32_t ANativeWindow_setBuffersGeometry(ANativeWindow*,int32_t,int32_t,int32_t);
extern void ANativeActivity_setWindowFlags(ANativeActivity*, uint32_t, uint32_t); extern ALooper* ALooper_forThread(void); extern ALooper* ALooper_prepare(int); extern int ALooper_addFd(ALooper*,int,int,int,int (*)(int,int,void*),void*);
extern void AInputQueue_attachLooper(AInputQueue*, ALooper*, int, int (*)(int,int,void*), void*); extern void AInputQueue_detachLooper(AInputQueue*);
extern int32_t AInputQueue_getEvent(AInputQueue*, AInputEvent**); extern int32_t AInputQueue_preDispatchEvent(AInputQueue*, AInputEvent*); extern void AInputQueue_finishEvent(AInputQueue*, AInputEvent*, int);
extern int32_t AInputEvent_getType(const AInputEvent*); extern int32_t AMotionEvent_getAction(const AInputEvent*); extern size_t AMotionEvent_getPointerCount(const AInputEvent*);
extern float AMotionEvent_getX(const AInputEvent*, size_t); extern float AMotionEvent_getY(const AInputEvent*, size_t); extern float AMotionEvent_getPressure(const AInputEvent*, size_t);
extern int32_t AMotionEvent_getPointerId(const AInputEvent*, size_t);
extern int32_t AMotionEvent_getToolType(const AInputEvent*, size_t); extern size_t AMotionEvent_getHistorySize(const AInputEvent*);
extern float AMotionEvent_getHistoricalX(const AInputEvent*, size_t, size_t); extern float AMotionEvent_getHistoricalY(const AInputEvent*, size_t, size_t); extern float AMotionEvent_getHistoricalPressure(const AInputEvent*, size_t, size_t);
extern int64_t AMotionEvent_getHistoricalEventTime(const AInputEvent*, size_t);
extern int32_t AMotionEvent_getButtonState(const AInputEvent*); extern int64_t AMotionEvent_getEventTime(const AInputEvent*);

#define AINPUT_EVENT_TYPE_MOTION 2
#define AMOTION_ACTION_MASK 0xff
#define AMOTION_ACTION_DOWN 0
#define AMOTION_ACTION_UP 1
#define AMOTION_ACTION_MOVE 2
#define AMOTION_ACTION_CANCEL 3
#define AMOTION_ACTION_POINTER_DOWN 5
#define AMOTION_ACTION_POINTER_UP 6
#define AMOTION_ACTION_HOVER_MOVE 7
#define AMOTION_ACTION_HOVER_ENTER 9
#define AMOTION_ACTION_HOVER_EXIT 10
#define AMOTION_ACTION_BUTTON_PRESS 11
#define AMOTION_ACTION_BUTTON_RELEASE 12
#define TOOL_FINGER 1
#define TOOL_STYLUS 2
#define TOOL_ERASER 4
#define BUTTON_STYLUS_PRIMARY 32
#define BUTTON_STYLUS_SECONDARY 64
#define ALOOPER_EVENT_INPUT 1
#define CLOCK_MONOTONIC 1
#define O_RDONLY_LOCAL 0
#define O_DIRECTORY_LOCAL 00200000
#if defined(__aarch64__)
#define SYS_SYNCFS_AARCH64 267
#endif
#define FLAG_KEEP_SCREEN_ON 0x00000080u
#define FLAG_FULLSCREEN 0x00000400u
#define FORMAT_RGBA8888 1
#define FORMAT_RGBX8888 2
#define FORMAT_RGB565 4
#define FORMAT_RGBA_LAYER 99
#define PI 3.14159265358979323846f

// ---------- Core model ----------
typedef OcrPoint Point;
typedef struct { Point* pts; int n,cap; uint32_t color; float baseWidth; float minx,miny,maxx,maxy; int active; } Stroke;
typedef enum { ACT_DRAW=1, ACT_ERASE=2, ACT_CLEAR=3, ACT_ERASE_ALL=4 } ActionType;
typedef struct { ActionType type; int* ids; int n,cap; int* refTypes,*refIndices; int refN,refCap; } Action;
typedef struct { float ax,ay,bx,by; int active; } Measure;
typedef enum { MODE_PEN=0, MODE_ERASE=1, MODE_MEASURE=2, MODE_HIGHLIGHTER=3, MODE_SELECT=4, MODE_SHAPE=5, MODE_ERASE_ALL=6 } ToolMode;
typedef struct { uint32_t bg,grid,gridMajor,axis,glass,glass2,glass3,border,text,muted,accent,cyan,violet,warn,danger,green,black; } Theme;
typedef struct { uint32_t* px; int pw,ph; float x,y,w,h,rot; uint32_t avgColor; int active; } ImageObj;
typedef struct { float x,y,w,h; uint32_t color; int id,active; } FrameObj;
/* VWS1 records stay unchanged on disk; full text is an optional trailing block. */
typedef struct { float x,y,w,h; uint32_t color; int id,active,type,locked; char text[96]; } LegacyNoteObj;
#define NOTE_TEXT_CAP 4096
typedef struct { float x,y,w,h; uint32_t color; int id,active,type,locked; char text[NOTE_TEXT_CAP]; } NoteObj;
typedef struct { float x0,y0,x1,y1; uint32_t color; float width; int id,active,type,locked; } ShapeObj;
typedef struct { float x,y,scale; int id,active; char name[32]; } BookmarkObj;
typedef struct { int type,index; } ObjRef;
typedef struct { ObjRef refs[48]; int n,id,active; } GroupObj;
typedef union { ImageObj image; Measure measure; NoteObj note; ShapeObj shape; } TransformSnapshot;
enum { NOTE_TEXT=0, NOTE_STICKY=1 };
enum { SHAPE_LINE=0, SHAPE_RECT=1, SHAPE_ELLIPSE=2, SHAPE_ARROW=3 };
enum { SEL_NONE=0, SEL_STROKES=1, SEL_IMAGE=2, SEL_MEASURE=3, SEL_NOTE=4, SEL_SHAPE=5, SEL_FRAME=6, SEL_GROUP=7, SEL_BOOKMARK=8, SEL_HANDWRITING=9 };
typedef struct { uint32_t* px; int w,h; int64_t mediaId; } Thumb;
enum { BA_NONE=0,BA_UNDO,BA_REDO,BA_QUICK_ERASE,BA_ERASER,BA_PEN,BA_MEASURE,BA_FIT,BA_NEXT_COLOR,BA_PRESSURE,BA_SETTINGS,BA_IMPORT,BA_HIGHLIGHTER,BA_COLOR_PICKER,BA_COUNT };

typedef struct {
    ANativeActivity* activity; ANativeWindow* window; AInputQueue* input;
    Stroke* strokes; int strokeN,strokeCap; Action* actions; int actionN,actionCap,actionCursor;
    int currentStroke; Action eraseAction; int erasing,eraseEverythingActive;
    ToolMode tool; uint32_t color; float brush;
    float pressureMin,pressureMax; int pressureCurve; int pressurePanel;
    float pressureSmoothing,strokeSmoothing,highlighterOpacity; float filteredPressure,smoothWorldX,smoothWorldY; int smoothingInit;
    float sampleRawX,sampleRawY,sampleDt; int64_t sampleTimeNs,previousSampleNs;
    float scale,offX,offY; int viewInitialized;
    float lastWriteX,lastWriteY,lastWriteScale; int lastWriteValid;
    int stylusDown,stylusPointerId,stylusToolType; int fingerCount,fingerPanning,fingerMoved,fingerId0,fingerId1,fingerRevealChromeCandidate; float lastFingerX,lastFingerY,lastMidX,lastMidY,lastPinchDist,fingerDownX,fingerDownY; ObjRef fingerDownRef;
    int flingActive,flingSampleN; float flingVX,flingVY,flingTrackX,flingTrackY,flingSampleX[12],flingSampleY[12]; int64_t flingSampleMs[12];
    int twoTapCandidate,threeTapCandidate; int64_t twoTapStartMs,lastTwoTapMs,threeTapStartMs; float twoTapStartMidX,twoTapStartMidY,twoTapStartDist,twoTapMaxMove,lastTwoTapX,lastTwoTapY,threeTapStartX,threeTapStartY,threeTapMaxMove;
    Measure measures[64]; int measureN; int measuring; Measure liveMeasure; int snap; int selectedMeasure; float cadScale; int cadCalibrated,cadUnit; int measureLoadMap[64],measureLoadMapCount;
    int clearArmed; int saveDirty; int metaDirty; int screenW,screenH;
    int inkSavePending,workspaceSavePending,ocrPending,ocrPendingFrom;int64_t interactionMs;
    int toast; int64_t toastMs; int64_t lastRenderEventMs;
    float dockPointerX,dockPointerY,dockSmoothX; int dockPointerActive,dockHot; float lastRawPressure;
    Theme theme; float uiScale; int settingsPanel,settingsTab,themeRole;
    int colorPickerOpen; float pickerHue,pickerSat,pickerVal; int pickerDragging,pickerApplySelection;
    int colorRailDragging,pickerSessionTouched; float colorRailAnim,colorRailVel,colorRailDragStartX,colorRailDragStartAnim; uint32_t pickerSessionStartColor;
    uint32_t recentColors[6]; int recentColorN; char inkPrefsPath[512];
    int radialOpen,radialHot,penHoldArmed,penHoldMoved; float radialX,radialY,penHoldX,penHoldY; int penHoldTimerFd; float penHoldMaxMove,radialHoldSec,penHoldVisual,radialSizePreview,radialSizeStart; int radialSizing,radialSizeActive,touchHoldMode,touchHoldImage; int64_t penHoldStartMs;
    int buttonPressAction,buttonHoldAction,buttonDoubleAction,buttonBindingsEnabled; int stylusButtonDown,buttonHoldFired,buttonDoubleSecond,buttonQuickErase; int pendingButtonSingle; int buttonTimerFd,buttonTimerMode; int64_t buttonDownMs;
    ImageObj images[32],imageCancelStart; int imageN,imagesDirty,imagePixelsDirty,imageSavePending,imageContentRevision; char imagePath[512],imageMetaPath[512]; int photoMode,selectedImage,imageDragging,imageGesture,imageCancelValid,photoAngleSnap; float imageStartX,imageStartY,imageStartW,imageStartH,imageStartRot,imageStartCX,imageStartCY,imageGestureDist,imageGestureAngle,imageGestureMidX,imageGestureMidY;
    int photoImportActive; int64_t photoImportMediaId; uint32_t*photoImportPixels; jobject photoImportClass; int photoImportSourceW,photoImportSourceH,photoImportSample,photoImportOrientation,photoImportW,photoImportH,photoImportLeft,photoImportTop;
    FrameObj frames[48],liveFrame,frameGestureStart; int frameN,nextFrameId,selectedFrame,framePanel,frameCreating,frameDragging,frameRevision,frameMoveMode,frameMoveDragging,framePage,frameResizeDragging,frameResizeCorner; float frameMoveLastX,frameMoveLastY,frameMoveTotalX,frameMoveTotalY,frameResizeOppX,frameResizeOppY; char framePath[512]; int visInk,visMarker,visPhotos,visCAD,visFrames;
    char frameNames[48][32]; int frameLocked[48],frameSizeLocked[48];
    NoteObj notes[96]; int noteN,nextNoteId,selectedNote;
    ShapeObj shapes[128],liveShape; int shapeN,nextShapeId,selectedShape,shapeDrawing,shapeType;
    BookmarkObj bookmarks[32]; int bookmarkN,nextBookmarkId,selectedBookmark;
    GroupObj groups[32]; int groupN,nextGroupId,selectedGroup;
    int selectedStrokes[256],selectedStrokeN; ObjRef selectedRefs[256],transformSnapshotRefs[256]; TransformSnapshot transformSnapshots[256]; int selectedRefN,transformSnapshotN; int selectionType,selectionDragging,selectionGesture,selectionMoreOpen; float selectionLastWX,selectionLastWY,selectionDragTotalX,selectionDragTotalY,selectionGestureDist,selectionGestureAngle,selectionGestureMidWX,selectionGestureMidWY,gestureStartOffX,gestureStartOffY,gestureStartScale;
    Point lassoPts[256]; int lassoN,lassoActive;
    int snapGuideX,snapGuideY; float snapGuideWX,snapGuideWY;
    int addPanel,layersPanel,searchPanel,bookmarksPanel,projectsPanel,presentationMode,presentationIndex,zenMode,sceneRevision,pressedUi;
    int visNotes,visShapes; int lockInk,lockMarker,lockPhotos,lockCAD,lockNotes,lockShapes,lockFrames;
    int editorOpen,editorMode,editorTarget,editorLen,editorCreated,systemImeActive; char editorBuf[NOTE_TEXT_CAP],editorOriginal[NOTE_TEXT_CAP]; jobject systemEdit;
    char searchQuery[64]; int searchLen; ObjRef searchVisible[6]; int searchVisibleN;
    int projectIndex; char workspacePath[512],projectPrefsPath[512]; char projectNames[4][32];
    int64_t strokeLastMoveMs,strokeStartMs;
    int galleryOpen,galleryPage,galleryLoadedPage,mediaCount; int64_t mediaIds[96]; Thumb thumbs[12]; int thumbN; int mediaPermissionPending; int minimap; int railOpen,railTimerFd; int calibrationOpen; char calibText[24]; int calibLen; float calibValue;
    int animTimerFd; float railAnim,radialAnim,minimapAnim,railVel,radialVel,minimapVel; float lodSmoothPx,lodMotionMix; int minimapDirty,mapRenderMode,uiAnimating; uint32_t* minimapCache; int minimapCacheW,minimapCacheH; float minimapMinX,minimapMinY,minimapMaxX,minimapMaxY;
    int atmosphere,gridStyle,gridDepth,edgeGlass,planeElevation,regionGlow,motionStyle,farZoomMode,performanceMode; float motionIntensity; int gpuActive;
    int hoverActive; float hoverX,hoverY;
    char savePath[512]; char metaPath[512];
    OcrManager* ocr; int ocrEnabled,ocrInitFailed,ocrInitialCursor,ocrInitialPending,ocrEventRegistered;
    char ocrPrefsPath[512],ocrSidecarPath[512];
    OcrManagerSearchHit ocrSearchHits[6]; int ocrSearchHitN;
    OcrBounds ocrHighlightBounds; float ocrHighlightAlpha;
    OcrI32* ocrMovedIds; OcrI32* ocrMovedSlots; float* ocrMovedDx; float* ocrMovedDy; int ocrMovedN,ocrMovedCap,ocrMoveScope;
} App;
static App G;
static uint8_t* FONT_ATLAS;
static uint8_t* STROKE_MASK; static int STROKE_MASK_W,STROKE_MASK_H;
static void render(void);
static int android_ui_modal(void);
static int android_ui_available(void);
static void nui_destroy(void);
static void start_anim_timer(void);
static void minimap_camera_changed(void);
static int us(int v);
static void reset_view(void);
static int save_workspace(void);
typedef struct Surf Surf;
static int str_len_local(const char*);
static void copy_text_local(char*,int,const char*);
static void selection_clear(void);
static void selection_set_one(int,int);
static void selection_apply_color(void);
static int selection_bounds(float*,float*,float*,float*);
static int remap_object_refs(int,const int*,int);
static void sanitize_group_refs(void);
static int save_all_document(void);
static void draw_wrapped_text(Surf*,int,int,int,const char*,int,uint32_t,int);
static void draw_canvas_world(Surf*);
static const char* search_result_label(ObjRef,char*,int);

static float fmin2(float a,float b){return a<b?a:b;} static float fmax2(float a,float b){return a>b?a:b;} static float clampf(float v,float a,float b){return v<a?a:(v>b?b:v);} static int mini(int a,int b){return a<b?a:b;} static int maxi(int a,int b){return a>b?a:b;}
static float smoothstepf(float a,float b,float x){if(b<=a)return x>=b?1.0f:0.0f;float t=clampf((x-a)/(b-a),0.0f,1.0f);return t*t*(3.0f-2.0f*t);} 
static float sq(float x){return x*x;} static int64_t event_ms(const AInputEvent* e){return AMotionEvent_getEventTime(e)/1000000LL;}
static int64_t monotonic_ms(void){timespec_t t;if(clock_gettime(CLOCK_MONOTONIC,&t)!=0)return 0;return (int64_t)t.tv_sec*1000LL+(int64_t)t.tv_nsec/1000000LL;}
#include "interaction_profile.inc"

/* Model files are replaced only after a complete, durable write.  Keeping the
   temporary file beside the destination preserves POSIX rename atomicity on
   Android's app-private filesystem. */
static int fsync_parent_directory(const char*path){
    if(!path||!path[0])return 0;char dir[512];int n=0,last=-1;while(path[n]&&n<(int)sizeof(dir)-1){dir[n]=path[n];if(path[n]=='/')last=n;n++;}if(last<0)return 1;dir[last?last:1]=0;
    /* Some Android filesystems reject fsync on a directory even though the
       directory can be opened.  syncfs on that descriptor supplies the same
       durability barrier at filesystem scope.  A plain read-only open is a
       compatibility fallback for kernels that reject O_DIRECTORY here. */
    int fd=open(dir,O_RDONLY_LOCAL|O_DIRECTORY_LOCAL);if(fd<0)fd=open(dir,O_RDONLY_LOCAL);if(fd<0)return 0;int ok=fsync(fd)==0;
#if defined(__aarch64__)
    if(!ok)ok=syscall(SYS_SYNCFS_AARCH64,fd)==0;
#endif
    (void)close(fd);return ok;
}
enum { ATOMIC_WRITE_FAILED=0, ATOMIC_WRITE_COMMITTED=1, ATOMIC_WRITE_DURABLE=2 };
static void* atomic_write_begin(const char*path,char*tmp,int cap){
    if(!path||!path[0]||!tmp||cap<=8)return 0;int n=snprintf(tmp,(size_t)cap,"%s.tmp",path);if(n<=0||n>=cap)return 0;return fopen(tmp,"wb");
}
static int atomic_write_finish(void*f,const char*tmp,const char*path,int ok){
    if(!f)return 0;if(ok&&fflush(f)!=0)ok=0;if(ok){int fd=fileno(f);if(fd<0||fsync(fd)!=0)ok=0;}if(fclose(f)!=0)ok=0;
    /* A completed rename is a successful logical save even when a particular
       filesystem cannot expose a namespace durability barrier.  Transactional
       callers can still require the stronger DURABLE result. */
    if(ok&&rename(tmp,path)==0)return fsync_parent_directory(path)?ATOMIC_WRITE_DURABLE:ATOMIC_WRITE_COMMITTED;remove(tmp);return ATOMIC_WRITE_FAILED;
}

/* ---------- Offline handwriting search bridge ----------
   All geometry passed to the worker is copied by ocr_manager before these
   calls return.  The mutable canvas Stroke array therefore never crosses the
   thread boundary. */
typedef struct { char magic[4]; int32_t enabled; } OcrPrefs;
static void ocr_project_path_for(int idx,char*out,int cap){if(!out||cap<=0)return;out[0]=0;if(!G.activity||!G.activity->internalDataPath)return;idx=maxi(0,mini(3,idx));if(idx==0)snprintf(out,(size_t)cap,"%s/canvas.vastocr",G.activity->internalDataPath);else snprintf(out,(size_t)cap,"%s/canvas_%d.vastocr",G.activity->internalDataPath,idx);}
static void ocr_load_prefs(void){G.ocrEnabled=1;if(!G.ocrPrefsPath[0])return;void*f=fopen(G.ocrPrefsPath,"rb");if(!f)return;OcrPrefs p;if(fread(&p,sizeof(p),1,f)==1&&p.magic[0]=='V'&&p.magic[1]=='O'&&p.magic[2]=='P'&&p.magic[3]=='1')G.ocrEnabled=p.enabled?1:0;fclose(f);}
static void ocr_save_prefs(void){if(!G.ocrPrefsPath[0])return;char tmp[544];void*f=atomic_write_begin(G.ocrPrefsPath,tmp,sizeof(tmp));if(!f)return;OcrPrefs p={{'V','O','P','1'},G.ocrEnabled};int ok=fwrite(&p,sizeof(p),1,f)==1;(void)atomic_write_finish(f,tmp,G.ocrPrefsPath,ok);}
static void ocr_fill_stroke(int index,OcrI64 completed,OcrManagerStroke*out){Stroke*s;if(!out){return;}memset(out,0,sizeof(*out));if(index<0||index>=G.strokeN)return;s=&G.strokes[index];out->runtime_index=index;out->points=s->pts;out->point_count=s->n>0?(OcrU32)s->n:0;out->base_width=s->baseWidth;out->completed_ms=completed;out->active=s->active?1u:0u;}
static void ocr_set_interaction(int active){
#ifdef VAST_OCR
    if(G.ocr)ocr_manager_set_interaction_active(G.ocr,active,active?0:850);
#else
    (void)active;
#endif
}
static void ocr_submit_stroke(int index,OcrI64 completed){
#ifdef VAST_OCR
    if(G.ocr&&G.ocrEnabled&&index>=0&&index<G.strokeN){OcrManagerStroke s;ocr_fill_stroke(index,completed,&s);ocr_manager_add_completed_stroke(G.ocr,&s);}
#else
    (void)index;(void)completed;
#endif
}
static void ocr_set_action_active(const Action*a,int active){
#ifdef VAST_OCR
    if(G.ocr&&a&&a->ids&&a->n>0)ocr_manager_set_strokes_active(G.ocr,(const OcrI32*)a->ids,(OcrU32)a->n,active);
#else
    (void)a;(void)active;
#endif
}
static void ocr_begin_initial_scan(void){G.ocrInitialCursor=0;G.ocrInitialPending=(G.ocr&&G.ocrEnabled)?1:0;if(G.ocrInitialPending)start_anim_timer();}
static int ocr_initial_scan_step(void){
#ifdef VAST_OCR
    OcrManagerStroke batch[32];OcrU32 n=0;if(!G.ocr||!G.ocrInitialPending)return 0;if(G.stylusDown||G.fingerCount>0)return 1;while(G.ocrInitialCursor<G.strokeN&&n<32u){ocr_fill_stroke(G.ocrInitialCursor,0,batch+n);G.ocrInitialCursor++;n++;}if(n)ocr_manager_add_initial_strokes(G.ocr,batch,n);if(G.ocrInitialCursor>=G.strokeN){ocr_manager_finish_initial_scan(G.ocr);G.ocrInitialPending=0;}return G.ocrInitialPending;
#else
    G.ocrInitialPending=0;return 0;
#endif
}
static void ocr_open_current_project(void){
#ifdef VAST_OCR
    if(!G.ocr)return;ocr_project_path_for(G.projectIndex,G.ocrSidecarPath,sizeof(G.ocrSidecarPath));if(ocr_manager_open_project(G.ocr,(OcrU64)(G.projectIndex+1),G.ocrSidecarPath)==OCR_OK)ocr_begin_initial_scan();
#endif
}
static void ocr_close_current_project(int checkpoint){
#ifdef VAST_OCR
    if(G.ocr)ocr_manager_close_project(G.ocr,checkpoint);
#else
    (void)checkpoint;
#endif
    G.ocrInitialPending=0;G.ocrInitialCursor=0;G.ocrSearchHitN=0;G.ocrHighlightAlpha=0;
}
static void ocr_checkpoint(void){
#ifdef VAST_OCR
    if(G.ocr)ocr_manager_checkpoint(G.ocr);
#endif
}
static int ocr_clone_project_cache(int destination_index){
#ifdef VAST_OCR
    char path[512];if(!G.ocr)return 0;ocr_project_path_for(destination_index,path,sizeof(path));return ocr_manager_clone_project_cache(G.ocr,(OcrU64)(destination_index+1),path)==OCR_OK;
#else
    (void)destination_index;return 0;
#endif
}
static void ocr_initialize(void){
#ifdef VAST_OCR
    OcrManagerCreateInfo ci;int rc;if(!G.activity||!G.activity->assetManager||!G.activity->vm)return;memset(&ci,0,sizeof(ci));ci.asset_manager=G.activity->assetManager;ci.java_vm=G.activity->vm;ci.initially_enabled=G.ocrEnabled;rc=ocr_manager_create(&G.ocr,&ci);G.ocrInitFailed=rc!=OCR_OK;if(rc==OCR_OK)ocr_open_current_project();
#endif
}
static void ocr_shutdown(void){
#ifdef VAST_OCR
    if(G.ocr){ocr_manager_close_project(G.ocr,1);ocr_manager_shutdown(G.ocr);G.ocr=0;}
#endif
    if(G.ocrMovedIds)free(G.ocrMovedIds);if(G.ocrMovedSlots)free(G.ocrMovedSlots);if(G.ocrMovedDx)free(G.ocrMovedDx);if(G.ocrMovedDy)free(G.ocrMovedDy);G.ocrMovedIds=0;G.ocrMovedSlots=0;G.ocrMovedDx=G.ocrMovedDy=0;G.ocrMovedCap=G.ocrMovedN=0;
}
static void ocr_move_begin(void){int need=maxi(1,G.strokeN);if(need>G.ocrMovedCap){OcrI32*ids=(OcrI32*)malloc((size_t)need*sizeof(OcrI32));OcrI32*slots=(OcrI32*)malloc((size_t)need*sizeof(OcrI32));float*dx=(float*)malloc((size_t)need*sizeof(float)),*dy=(float*)malloc((size_t)need*sizeof(float));if(!ids||!slots||!dx||!dy){if(ids)free(ids);if(slots)free(slots);if(dx)free(dx);if(dy)free(dy);G.ocrMovedN=0;G.ocrMoveScope=0;return;}if(G.ocrMovedIds)free(G.ocrMovedIds);if(G.ocrMovedSlots)free(G.ocrMovedSlots);if(G.ocrMovedDx)free(G.ocrMovedDx);if(G.ocrMovedDy)free(G.ocrMovedDy);G.ocrMovedIds=ids;G.ocrMovedSlots=slots;G.ocrMovedDx=dx;G.ocrMovedDy=dy;G.ocrMovedCap=need;}memset(G.ocrMovedSlots,0xff,(size_t)G.ocrMovedCap*sizeof(OcrI32));G.ocrMovedN=0;G.ocrMoveScope=1;}
static void ocr_move_mark(int index,float dx,float dy){int slot;if(!G.ocrMoveScope||index<0||index>=G.ocrMovedCap)return;slot=G.ocrMovedSlots[index];if(slot<0){slot=G.ocrMovedN++;G.ocrMovedSlots[index]=slot;G.ocrMovedIds[slot]=(OcrI32)index;G.ocrMovedDx[slot]=G.ocrMovedDy[slot]=0;}G.ocrMovedDx[slot]+=dx;G.ocrMovedDy[slot]+=dy;}
static void ocr_move_commit(void){
#ifdef VAST_OCR
    if(G.ocr&&G.ocrMoveScope&&G.ocrMovedN>0){OcrManagerTranslation*t=(OcrManagerTranslation*)malloc((size_t)G.ocrMovedN*sizeof(*t));if(t){for(int i=0;i<G.ocrMovedN;i++){t[i].runtime_index=G.ocrMovedIds[i];t[i].dx=G.ocrMovedDx[i];t[i].dy=G.ocrMovedDy[i];}ocr_manager_translate_strokes(G.ocr,t,(OcrU32)G.ocrMovedN);free(t);}}
#endif
    G.ocrMoveScope=0;G.ocrMovedN=0;
}
static int ocr_event_cb(int fd,int events,void*data){(void)fd;(void)events;(void)data;
#ifdef VAST_OCR
    if(G.ocr)ocr_manager_drain_events(G.ocr);
#endif
    render();return 1;
}
static float pressure_map(float raw){float t=clampf(raw,0.0f,1.0f); if(G.pressureCurve==0)t=sqrtf(t); else if(G.pressureCurve==2)t=t*t; return G.pressureMin+t*(G.pressureMax-G.pressureMin);}
static float smooth_pressure(float raw){raw=clampf(raw,0.0f,1.0f);if(!G.smoothingInit){G.filteredPressure=raw;return raw;}float tau=clampf(G.pressureSmoothing,0,1)*.012f;float a=tau>0?1-expf(-G.sampleDt/tau):1;G.filteredPressure+=(raw-G.filteredPressure)*a;return G.filteredPressure;}
static void smooth_world(float rx,float ry,float*ox,float*oy){
    IP.samples++;
    float dt=G.previousSampleNs>0&&G.sampleTimeNs>G.previousSampleNs?(float)(G.sampleTimeNs-G.previousSampleNs)*1e-9f:1.0f/240.0f;
    G.sampleDt=clampf(dt,.0001f,.05f);
    if(!G.smoothingInit){G.smoothWorldX=rx;G.smoothWorldY=ry;}
    else{
        float dx=rx-G.smoothWorldX,dy=ry-G.smoothWorldY;
        float scale=fmax2(G.scale,.04f);
        float velocity=sqrtf(sq(rx-G.sampleRawX)+sq(ry-G.sampleRawY))*scale/G.sampleDt;
        float strength=clampf(G.strokeSmoothing,0,1);
        float tau=.006f*strength*(1-smoothstepf(40,600,velocity));
        float a=tau>.00001f?1-expf(-G.sampleDt/tau):1;
        G.smoothWorldX+=dx*a;G.smoothWorldY+=dy*a;
        /* A hard spatial bound prevents a noise filter becoming a tether. */
        dx=rx-G.smoothWorldX;dy=ry-G.smoothWorldY;
        float gap=sqrtf(dx*dx+dy*dy)*scale;
        if(gap>.65f){float keep=.65f/gap;G.smoothWorldX=rx-dx*keep;G.smoothWorldY=ry-dy*keep;}
    }
    G.sampleRawX=rx;G.sampleRawY=ry;G.previousSampleNs=G.sampleTimeNs;
    *ox=G.smoothWorldX;*oy=G.smoothWorldY;
}

static int ensure_strokes(int need){if(need<=G.strokeCap)return 1;int nc=G.strokeCap?G.strokeCap*2:128;while(nc<need){if(nc>1000000)return 0;nc*=2;}Stroke*p=(Stroke*)realloc(G.strokes,(size_t)nc*sizeof(Stroke));if(!p)return 0;G.strokes=p;memset(G.strokes+G.strokeCap,0,(size_t)(nc-G.strokeCap)*sizeof(Stroke));G.strokeCap=nc;return 1;}
static int ensure_points(Stroke*s,int need){if(need<=s->cap)return 1;int nc=s->cap?s->cap*2:64;while(nc<need){if(nc>1000000)return 0;nc*=2;}Point*p=(Point*)realloc(s->pts,(size_t)nc*sizeof(Point));if(!p)return 0;s->pts=p;s->cap=nc;return 1;}
static int action_add_id(Action*a,int id){for(int i=0;i<a->n;i++)if(a->ids[i]==id)return 1;if(a->n>=a->cap){int nc=a->cap?a->cap*2:32;int*p=(int*)realloc(a->ids,(size_t)nc*sizeof(int));if(!p)return 0;a->ids=p;a->cap=nc;}a->ids[a->n++]=id;return 1;}
static int action_add_ref(Action*a,int type,int index){
    for(int i=0;i<a->refN;i++)if(a->refTypes[i]==type&&a->refIndices[i]==index)return 1;
    if(a->refN>=a->refCap){int nc=a->refCap?a->refCap*2:16;int*types=(int*)malloc((size_t)nc*sizeof(int)),*indices=(int*)malloc((size_t)nc*sizeof(int));if(!types||!indices){free(types);free(indices);return 0;}if(a->refN){memcpy(types,a->refTypes,(size_t)a->refN*sizeof(int));memcpy(indices,a->refIndices,(size_t)a->refN*sizeof(int));}free(a->refTypes);free(a->refIndices);a->refTypes=types;a->refIndices=indices;a->refCap=nc;}
    a->refTypes[a->refN]=type;a->refIndices[a->refN]=index;a->refN++;return 1;
}
static int action_ref_locked(int type,int index){
    if(type==SEL_IMAGE)return G.lockPhotos;
    if(type==SEL_MEASURE)return G.lockCAD;
    if(type==SEL_NOTE)return G.lockNotes||(index>=0&&index<G.noteN&&G.notes[index].locked);
    if(type==SEL_SHAPE)return G.lockShapes||(index>=0&&index<G.shapeN&&G.shapes[index].locked);
    if(type==SEL_FRAME)return G.lockFrames||(index>=0&&index<G.frameN&&G.frameLocked[index]);
    return 0;
}
static void action_set_ref_active(int type,int index,int active){
    if(type==SEL_IMAGE&&index>=0&&index<G.imageN){G.images[index].active=active;G.imagesDirty=G.imagePixelsDirty=G.imageSavePending=1;}
    else if(type==SEL_MEASURE&&index>=0&&index<G.measureN){G.measures[index].active=active;G.metaDirty=1;}
    else if(type==SEL_NOTE&&index>=0&&index<G.noteN){G.notes[index].active=active;G.workspaceSavePending=1;}
    else if(type==SEL_SHAPE&&index>=0&&index<G.shapeN){G.shapes[index].active=active;G.workspaceSavePending=1;}
    else if(type==SEL_FRAME&&index>=0&&index<G.frameN){G.frames[index].active=active;G.frameRevision++;G.workspaceSavePending=1;}
}
static void action_set_refs_active(Action*a,int active){if(!a)return;for(int i=0;i<a->refN;i++)action_set_ref_active(a->refTypes[i],a->refIndices[i],active);if(a->refN){G.sceneRevision++;G.minimapDirty=1;start_anim_timer();}}
static void free_action(Action*a){if(a->ids)free(a->ids);if(a->refTypes)free(a->refTypes);if(a->refIndices)free(a->refIndices);memset(a,0,sizeof(*a));}
static int ensure_actions(int need){if(need<=G.actionCap)return 1;int nc=G.actionCap?G.actionCap*2:128;while(nc<need){if(nc>1000000)return 0;nc*=2;}Action*p=(Action*)realloc(G.actions,(size_t)nc*sizeof(Action));if(!p)return 0;G.actions=p;memset(G.actions+G.actionCap,0,(size_t)(nc-G.actionCap)*sizeof(Action));G.actionCap=nc;return 1;}
static void drop_redo(void){for(int i=G.actionCursor;i<G.actionN;i++)free_action(&G.actions[i]);G.actionN=G.actionCursor;}
static void push_action(Action*src){drop_redo();if(!ensure_actions(G.actionN+1)){free_action(src);G.saveDirty=1;return;}G.actions[G.actionN]=*src;memset(src,0,sizeof(*src));G.actionN++;G.actionCursor=G.actionN;G.saveDirty=1;}
static void push_draw_action(int id){Action a;memset(&a,0,sizeof(a));a.type=ACT_DRAW;action_add_id(&a,id);push_action(&a);}
static void set_toast(int code,int64_t ms){G.toast=code;G.toastMs=ms>0?ms:monotonic_ms();start_anim_timer();}
static int stroke_layer_locked(const Stroke*s){if(!s)return 1;int aa=(int)((s->color>>24)&255),marker=aa>0&&aa<248;return marker?G.lockMarker:G.lockInk;}
static int action_hits_locked_layer(const Action*a){if(!a)return 0;for(int i=0;i<a->n;i++){int id=a->ids[i];if(id>=0&&id<G.strokeN&&stroke_layer_locked(&G.strokes[id]))return 1;}for(int i=0;i<a->refN;i++)if(action_ref_locked(a->refTypes[i],a->refIndices[i]))return 1;return 0;}
static void undo_action(void){G.clearArmed=0;if(G.actionCursor<=0){set_toast(2,0);return;}Action*a=&G.actions[G.actionCursor-1];if(action_hits_locked_layer(a)){set_toast(9,0);return;}G.actionCursor--;if(a->type==ACT_DRAW){for(int i=0;i<a->n;i++)if(a->ids[i]>=0&&a->ids[i]<G.strokeN)G.strokes[a->ids[i]].active=0;ocr_set_action_active(a,0);}else{for(int i=0;i<a->n;i++)if(a->ids[i]>=0&&a->ids[i]<G.strokeN)G.strokes[a->ids[i]].active=1;ocr_set_action_active(a,1);action_set_refs_active(a,1);}G.saveDirty=1;G.minimapDirty=1;set_toast(1,0);}
static void redo_action(void){G.clearArmed=0;if(G.actionCursor>=G.actionN)return;Action*a=&G.actions[G.actionCursor];if(action_hits_locked_layer(a)){set_toast(9,0);return;}G.actionCursor++;if(a->type==ACT_DRAW){for(int i=0;i<a->n;i++)if(a->ids[i]>=0&&a->ids[i]<G.strokeN)G.strokes[a->ids[i]].active=1;ocr_set_action_active(a,1);}else{for(int i=0;i<a->n;i++)if(a->ids[i]>=0&&a->ids[i]<G.strokeN)G.strokes[a->ids[i]].active=0;ocr_set_action_active(a,0);action_set_refs_active(a,0);}G.saveDirty=1;G.minimapDirty=1;}
static void clear_canvas(void){Action a;int locked=0;memset(&a,0,sizeof(a));a.type=ACT_CLEAR;for(int i=0;i<G.strokeN;i++)if(G.strokes[i].active){if(stroke_layer_locked(&G.strokes[i])){locked=1;continue;}action_add_id(&a,i);G.strokes[i].active=0;}if(a.n){ocr_set_action_active(&a,0);push_action(&a);}else free_action(&a);if(locked)set_toast(9,0);G.clearArmed=0;}

static void append_point(Stroke*s,float x,float y,float p){if(s->n>0){Point q=s->pts[s->n-1];float dx=x-q.x,dy=y-q.y;if(dx==0&&dy==0&&p==q.p)return;}if(!ensure_points(s,s->n+1))return;s->pts[s->n].x=x;s->pts[s->n].y=y;s->pts[s->n].p=clampf(p,0.02f,1.8f);s->n++;if(s->n==1){s->minx=s->maxx=x;s->miny=s->maxy=y;}else{s->minx=fmin2(s->minx,x);s->maxx=fmax2(s->maxx,x);s->miny=fmin2(s->miny,y);s->maxy=fmax2(s->maxy,y);}}
static int start_stroke(float sx,float sy,float p){if(!ensure_strokes(G.strokeN+1))return -1;int id=G.strokeN++;Stroke*s=&G.strokes[id];memset(s,0,sizeof(*s));s->active=1;uint32_t rgb=G.color&0xffffffu;if(G.tool==MODE_HIGHLIGHTER){int aa=(int)(clampf(G.highlighterOpacity,0.08f,0.80f)*255.0f+0.5f);s->color=((uint32_t)aa<<24)|rgb;s->baseWidth=fmax2(18.0f,G.brush*4.2f);}else{s->color=rgb;s->baseWidth=G.brush;}G.smoothingInit=0;G.previousSampleNs=0;float wx=(sx-G.offX)/G.scale,wy=(sy-G.offY)/G.scale,fx,fy;smooth_world(wx,wy,&fx,&fy);G.filteredPressure=clampf(p,0,1);G.smoothingInit=1;float pp=G.tool==MODE_HIGHLIGHTER?1.0f:pressure_map(smooth_pressure(p));append_point(s,fx,fy,pp);if(s->n<=0){memset(s,0,sizeof(*s));G.strokeN--;return -1;}G.currentStroke=id;G.saveDirty=1;return id;}
static void add_stroke_point(float sx,float sy,float p){
    if(G.currentStroke<0)return;
    float wx=(sx-G.offX)/G.scale,wy=(sy-G.offY)/G.scale,fx,fy;smooth_world(wx,wy,&fx,&fy);
    Stroke*s=&G.strokes[G.currentStroke];int hi=((s->color>>24)&255)>0&&((s->color>>24)&255)<248;
    /* Store physical samples once. Interpolation belongs to retained geometry,
     * not the document, the eraser index, persistence or the OCR point stream. */
    float pp=hi?1.0f:pressure_map(smooth_pressure(p));append_point(s,fx,fy,pp);
}
static void cancel_current_stroke(void){if(G.currentStroke>=0&&G.currentStroke==G.strokeN-1){Stroke*s=&G.strokes[G.currentStroke];if(s->pts)free(s->pts);memset(s,0,sizeof(*s));G.strokeN--;G.currentStroke=-1;G.saveDirty=1;}else G.currentStroke=-1;}
static void end_stroke(void){if(G.currentStroke>=0){int completed=G.currentStroke;Stroke*st=&G.strokes[completed];if(st->n>0){Point p=st->pts[st->n-1];G.lastWriteX=p.x;G.lastWriteY=p.y;G.lastWriteScale=clampf(G.scale,.05f,20.0f);G.lastWriteValid=1;G.workspaceSavePending=1;}push_draw_action(completed);G.currentStroke=-1;G.minimapDirty=1;if(!G.ocrPending)G.ocrPendingFrom=completed;G.ocrPending=1;start_anim_timer();}G.smoothingInit=0;}
static void ocr_pending_flush(void){if(!G.ocrPending)return;for(int i=G.ocrPendingFrom;i<G.strokeN;i++)if(i!=G.currentStroke)ocr_submit_stroke(i,G.strokeLastMoveMs);G.ocrPending=0;}
static void schedule_ink_save(void){G.inkSavePending=1;G.interactionMs=monotonic_ms();start_anim_timer();}
static float point_seg_dist2(float px,float py,float ax,float ay,float bx,float by){float vx=bx-ax,vy=by-ay,wx=px-ax,wy=py-ay,c=vx*vx+vy*vy,t=c>0.00001f?(wx*vx+wy*vy)/c:0;t=clampf(t,0,1);float dx=px-(ax+t*vx),dy=py-(ay+t*vy);return dx*dx+dy*dy;}
#include "erase_spatial.inc"
static float snap_world(float v){if(!G.snap)return v;float step=16.0f;return floorf(v/step+0.5f)*step;}
static int remap_object_refs(int type,const int*map,int oldCount){
    int changed=0;for(int i=0;i<G.groupN;i++){GroupObj*g=&G.groups[i];int out=0;for(int k=0;k<g->n;k++){ObjRef r=g->refs[k];if(r.type==type){int ni=(r.index>=0&&r.index<oldCount)?map[r.index]:-1;if(ni<0){changed=1;continue;}if(ni!=r.index){r.index=ni;changed=1;}}g->refs[out++]=r;}for(int k=out;k<g->n;k++)g->refs[k]=(ObjRef){SEL_NONE,-1};g->n=out;if(g->active&&g->n<2){g->active=0;changed=1;}}
    int out=0;for(int i=0;i<G.selectedRefN;i++){ObjRef r=G.selectedRefs[i];if(r.type==type){int ni=(r.index>=0&&r.index<oldCount)?map[r.index]:-1;if(ni<0){changed=1;continue;}if(ni!=r.index){r.index=ni;changed=1;}}G.selectedRefs[out++]=r;}G.selectedRefN=out;if(G.selectedGroup>=0&&(G.selectedGroup>=G.groupN||!G.groups[G.selectedGroup].active))G.selectedGroup=-1;return changed;
}
static void start_measure(float sx,float sy){G.measuring=1;float x=snap_world((sx-G.offX)/G.scale),y=snap_world((sy-G.offY)/G.scale);G.liveMeasure.ax=G.liveMeasure.bx=x;G.liveMeasure.ay=G.liveMeasure.by=y;G.liveMeasure.active=1;}
static void update_measure(float sx,float sy){if(!G.measuring)return;G.liveMeasure.bx=snap_world((sx-G.offX)/G.scale);G.liveMeasure.by=snap_world((sy-G.offY)/G.scale);}
static void end_measure(void){if(!G.measuring)return;float dx=G.liveMeasure.bx-G.liveMeasure.ax,dy=G.liveMeasure.by-G.liveMeasure.ay;if(dx*dx+dy*dy>1.0f){if(G.measureN>=64){int map[64];map[0]=-1;for(int i=1;i<64;i++){G.measures[i-1]=G.measures[i];map[i]=i-1;}G.measureN=63;if(remap_object_refs(SEL_MEASURE,map,64))save_workspace();}G.measures[G.measureN++]=G.liveMeasure;G.selectedMeasure=G.measureN-1;G.metaDirty=1;G.minimapDirty=1;}G.measuring=0;memset(&G.liveMeasure,0,sizeof(G.liveMeasure));}
static void clear_measures(void){if(G.lockCAD)return;int old=G.measureN,map[64];for(int i=0;i<old;i++)map[i]=-1;if(old&&remap_object_refs(SEL_MEASURE,map,old))save_workspace();G.measureN=0;G.measuring=0;G.selectedMeasure=-1;memset(G.measures,0,sizeof(G.measures));G.metaDirty=1;G.minimapDirty=1;}

// ---------- Persistence ----------
typedef struct { char magic[4]; uint32_t count; } FileHead;
typedef struct { uint32_t color; float width; uint32_t n; } StrokeHead;
typedef struct { char magic[4]; float pmin,pmax; int32_t curve; int32_t snap; uint32_t count; } MetaHead2;
typedef struct { char magic[4]; float pmin,pmax; int32_t curve; int32_t snap; uint32_t count; float uiScale; Theme theme; int32_t pressAction,holdAction,doubleAction; } MetaHead3;
typedef struct { char magic[4]; float pmin,pmax; int32_t curve; int32_t snap; uint32_t count; float uiScale; Theme theme; int32_t pressAction,holdAction,doubleAction; float pressureSmoothing,strokeSmoothing,highlighterOpacity,brush; uint32_t inkColor; } MetaHead4;
typedef struct { char magic[4]; float pmin,pmax; int32_t curve; int32_t snap; uint32_t count; float uiScale; Theme theme; int32_t pressAction,holdAction,doubleAction; float pressureSmoothing,strokeSmoothing,highlighterOpacity,brush; uint32_t inkColor; float cadScale; int32_t cadCalibrated,cadUnit,minimap; } MetaHead5;
typedef struct { char magic[4]; float pmin,pmax; int32_t curve; int32_t snap; uint32_t count; float uiScale; Theme theme; int32_t pressAction,holdAction,doubleAction; float pressureSmoothing,strokeSmoothing,highlighterOpacity,brush; uint32_t inkColor; float cadScale; int32_t cadCalibrated,cadUnit,minimap; float radialHoldSec; int32_t buttonBindingsEnabled; } MetaHead6;
typedef struct { char magic[4]; float pmin,pmax; int32_t curve; int32_t snap; uint32_t count; float uiScale; Theme theme; int32_t pressAction,holdAction,doubleAction; float pressureSmoothing,strokeSmoothing,highlighterOpacity,brush; uint32_t inkColor; float cadScale; int32_t cadCalibrated,cadUnit,minimap; float radialHoldSec; int32_t buttonBindingsEnabled; int32_t atmosphere,gridStyle,gridDepth,edgeGlass,planeElevation,regionGlow,motionStyle,farZoomMode,performanceMode; float motionIntensity; } MetaHead7;
typedef struct { char magic[4]; float pmin,pmax; int32_t curve; int32_t snap; uint32_t count; float uiScale; Theme theme; int32_t pressAction,holdAction,doubleAction; float pressureSmoothing,strokeSmoothing,highlighterOpacity,brush; uint32_t inkColor; float cadScale; int32_t cadCalibrated,cadUnit,minimap; float radialHoldSec; int32_t buttonBindingsEnabled; int32_t atmosphere,gridStyle,gridDepth,edgeGlass,planeElevation,regionGlow,motionStyle,farZoomMode,performanceMode; float motionIntensity; int32_t photoAngleSnap; } MetaHead8;
typedef struct { char magic[4]; uint32_t count; } ImageFileHead;
typedef struct { int32_t pw,ph; float x,y,w,h; uint32_t pixelCount; } ImageHead;
typedef struct { int32_t pw,ph; float x,y,w,h,rot; uint32_t pixelCount; } ImageHead2;
typedef struct { char magic[4]; uint32_t count; } ImageMetaFileHead;
typedef struct { int32_t pw,ph; float x,y,w,h,rot; } ImageMetaRecord;

#define THEME_PRESET_COUNT 24
static const char*const THEME_PRESET_NAMES[THEME_PRESET_COUNT]={
    "Dark","OLED","Paper","Blueprint","Warm",
    "Graphite","Midnight","Forest","Aubergine","Espresso","Nord","Carbon",
    "Porcelain","Sand","Sage","Lavender","Ice",
    "Ocean","Mint","Rose","Sunset","Copper","Neon","Contrast"
};
/* Each preset is a complete, hand-tuned role set rather than an accent swap.
   The original five remain byte-for-byte identical for saved-theme matching. */
static const Theme THEME_PRESETS[THEME_PRESET_COUNT]={
    {0x090d13,0x121923,0x1b2633,0x334a60,0x121923,0x18222e,0x202d3b,0x304154,0xf4f7fb,0x8695a7,0x6aa9ff,0x4de0d0,0xb18cff,0xffc55d,0xff6475,0x5fe39a,0x05070a},
    {0x000000,0x101010,0x1b1b1b,0x393939,0x0b0b0d,0x151518,0x222228,0x34343c,0xf6f7fb,0x8a8d98,0x7aa7ff,0x55e6d1,0xb998ff,0xffca66,0xff6579,0x63e69c,0x000000},
    {0xebe7de,0xd8d2c8,0xc8c0b4,0xa9a095,0xf7f4ed,0xeee9df,0xe3ddd2,0xb9b0a4,0x1c2026,0x646a70,0x316fbd,0x168f8a,0x7751b5,0xa46d16,0xb43f50,0x2e8456,0xd6d0c6},
    {0x071a2c,0x0e2a43,0x174263,0x2a6b94,0x0b2237,0x10304c,0x17415e,0x2c6284,0xe8f6ff,0x83a8be,0x4dd7ff,0x58efdb,0xb69cff,0xffd66f,0xff7285,0x63e7a7,0x04111d},
    {0x15100e,0x241a16,0x35251e,0x604335,0x1d1512,0x2a1e19,0x382720,0x584238,0xfff4e9,0xb3a093,0xff9f63,0x67d7ca,0xc59cff,0xffcf6b,0xff6e72,0x7ae29b,0x090706},
    {0x111318,0x1a1e25,0x242a34,0x3d4654,0x181b21,0x20242c,0x292e38,0x3a424f,0xf3f4f6,0x979eaa,0x8ab4f8,0x74d4d4,0xb4a0e5,0xf2c66d,0xf07b8a,0x7bd8a0,0x08090b},
    {0x080b18,0x111832,0x1a2850,0x2a4578,0x0e1326,0x151d38,0x1d294b,0x2e4170,0xeef2ff,0x8f9bc4,0x7697ff,0x57d6e8,0xbf91ff,0xffc96b,0xff718a,0x65dda5,0x03040a},
    {0x08120f,0x11231c,0x1b382c,0x2d5a46,0x0e1c17,0x162a22,0x1e382d,0x315344,0xeaf6ef,0x8eaa9c,0x74d39f,0x5bd6c5,0xb29ce8,0xe6c66a,0xef7380,0x58d68d,0x030805},
    {0x140c18,0x24152b,0x382042,0x63366f,0x1e1223,0x2b1932,0x3a2144,0x593263,0xf8eef9,0xad8eae,0xd58ce4,0x70d4cc,0xb799ff,0xf3c46f,0xf47b91,0x75d99a,0x08050a},
    {0x160f0b,0x281a12,0x3b281c,0x6b4934,0x211711,0x2e2018,0x3f2b20,0x604534,0xfff2e5,0xb19a89,0xd89a68,0x6fcfbe,0xb99ae8,0xe7bd66,0xed7680,0x73cf91,0x090604},
    {0x2e3440,0x3b4252,0x434c5e,0x4c566a,0x343b49,0x3b4353,0x454f61,0x596579,0xeceff4,0xa3adbd,0x88c0d0,0x8fbcbb,0xb48ead,0xebcb8b,0xbf616a,0xa3be8c,0x1f242e},
    {0x0d1112,0x172021,0x203032,0x385052,0x131a1b,0x1b2526,0x263233,0x3b4b4c,0xedf5f4,0x90a4a2,0x63c7be,0x4ed9d0,0xa2a0e8,0xe8c76a,0xec727d,0x67d597,0x060809},
    {0xf5f6f4,0xe2e5e1,0xd1d6d0,0xa8b0aa,0xffffff,0xeff2ee,0xe4e9e3,0xbcc4bd,0x1d2429,0x616d72,0x356fbd,0x167f84,0x7555b5,0x9a6818,0xb2384b,0x287b50,0xd9ddda},
    {0xeee6d5,0xd9cfbb,0xc8baa0,0xaa9471,0xf9f3e7,0xece2d0,0xe1d4bd,0xbba98b,0x29231c,0x6b6255,0xa8642a,0x147f7c,0x7550a5,0x966617,0xb43d48,0x34784a,0xd4c8b3},
    {0xe4ebe3,0xcbd8cb,0xb7c9b8,0x8ead91,0xf2f6f1,0xe7eee6,0xdbe6da,0xa9bdaa,0x1e2920,0x5d6c60,0x37734d,0x167e78,0x6d579f,0x9a6c1b,0xb13f4b,0x2c7a49,0xc9d4c9},
    {0xeeeaf5,0xd9d2e6,0xc7bcd8,0x9e8bb8,0xf9f7fc,0xeeeaf6,0xe4deed,0xb9accb,0x282231,0x6b6175,0x6b55b5,0x197f85,0x8153ad,0x98651d,0xb23e55,0x347a55,0xd7d1df},
    {0xe8f1f5,0xcfdfe6,0xb9d0da,0x8fadb9,0xf5fafc,0xeaf3f7,0xddebf0,0xacc3cc,0x17272e,0x586d75,0x286e9e,0x087e82,0x6653a5,0x91681a,0xac3f52,0x247850,0xd0dde2},
    {0x061824,0x0b2a3d,0x10415c,0x1d6889,0x092130,0x0d2e41,0x123d55,0x205c78,0xeaf8ff,0x83aabb,0x40bde8,0x42d6c1,0x9e94eb,0xf3c665,0xf27783,0x55d796,0x020b10},
    {0x071916,0x0d2b25,0x144239,0x236f5d,0x0a231f,0x0f312b,0x17433a,0x256454,0xeafff8,0x83ada1,0x54d6a2,0x40d8c6,0xa395e8,0xe9c761,0xed7681,0x4cdb8b,0x020c0a},
    {0x1a0c12,0x2d141f,0x431d2d,0x73344c,0x241019,0x321723,0x452031,0x673046,0xfff0f5,0xb38b9b,0xf28aad,0x63d0c7,0xb59aee,0xf0c36a,0xf16f87,0x68d494,0x0c0508},
    {0x1c100b,0x321b11,0x4a2818,0x7d4a2d,0x28170f,0x382016,0x4c2c1e,0x70452f,0xfff3e8,0xb79a86,0xff985d,0x61d3c4,0xb49be9,0xf6c85f,0xee6e74,0x6fd491,0x0d0704},
    {0x17100d,0x2a1c16,0x3f2b21,0x704c37,0x221711,0x302119,0x422e23,0x644936,0xfff1e6,0xb49b8c,0xc9895b,0x64ccc1,0xad98df,0xe2b961,0xe87376,0x6dcc8c,0x0a0705},
    {0x08080f,0x151425,0x242240,0x474270,0x11101d,0x1a192d,0x262442,0x403d66,0xf7f7ff,0xa5a2c5,0x7d8cff,0x30efd0,0xd996ff,0xffd85d,0xff557c,0x55ef92,0x030306},
    {0x000000,0x1a1a1a,0x303030,0x656565,0x090909,0x171717,0x282828,0x5a5a5a,0xffffff,0xbdbdbd,0xffd400,0x00e5ff,0xc58cff,0xffdb4d,0xff496c,0x49f28f,0x000000}
};
static const char*theme_preset_name(int preset){return preset>=0&&preset<THEME_PRESET_COUNT?THEME_PRESET_NAMES[preset]:"Theme";}
static Theme theme_preset_value(int preset){return THEME_PRESETS[preset>=0&&preset<THEME_PRESET_COUNT?preset:0];}
static void theme_preset(int preset){G.theme=theme_preset_value(preset);G.sceneRevision++;G.minimapDirty=1;}
static int theme_is_preset(int preset){Theme t=theme_preset_value(preset);const uint8_t*a=(const uint8_t*)&t,*b=(const uint8_t*)&G.theme;for(size_t i=0;i<sizeof(Theme);i++)if(a[i]!=b[i])return 0;return 1;}
static uint32_t mix_color(uint32_t a,uint32_t b,int pct){int ar=(a>>16)&255,ag=(a>>8)&255,ab=a&255,br=(b>>16)&255,bg=(b>>8)&255,bb=b&255;int r=(ar*(100-pct)+br*pct)/100,g=(ag*(100-pct)+bg*pct)/100,bl=(ab*(100-pct)+bb*pct)/100;return ((uint32_t)r<<16)|((uint32_t)g<<8)|(uint32_t)bl;}
static void theme_role_set(int role,uint32_t c){Theme*t=&G.theme;switch(role){case 0:t->bg=c;t->black=mix_color(c,0x000000,62);break;case 1:t->glass=c;t->glass2=mix_color(c,t->text,7);t->glass3=mix_color(c,t->text,13);t->border=mix_color(c,t->text,22);break;case 2:t->grid=c;t->gridMajor=mix_color(c,t->text,16);t->axis=mix_color(c,t->text,34);break;case 3:t->text=c;t->muted=mix_color(c,t->bg,43);break;case 4:t->accent=c;break;case 5:t->cyan=c;break;case 6:t->violet=c;break;case 7:t->danger=c;break;}G.metaDirty=1;G.minimapDirty=1;G.sceneRevision++;}

static int save_canvas(void){
    if(!G.saveDirty)return 1;if(!G.savePath[0])return 0;char tmp[544];void*f=atomic_write_begin(G.savePath,tmp,sizeof(tmp));if(!f)return 0;uint32_t c=0;for(int i=0;i<G.strokeN;i++)if(G.strokes[i].active&&G.strokes[i].n>0)c++;
    FileHead h={{'I','C','V','1'},c};int ok=fwrite(&h,sizeof(h),1,f)==1;
    for(int i=0;i<G.strokeN&&ok;i++){Stroke*s=&G.strokes[i];if(!s->active||s->n<=0)continue;StrokeHead sh={s->color,s->baseWidth,(uint32_t)s->n};ok=fwrite(&sh,sizeof(sh),1,f)==1&&fwrite(s->pts,sizeof(Point),(size_t)s->n,f)==(size_t)s->n;}
    int committed=atomic_write_finish(f,tmp,G.savePath,ok);if(committed)G.saveDirty=0;return committed;
}
static void load_canvas(void){
    if(!G.savePath[0])return;void*f=fopen(G.savePath,"rb");if(!f)return;FileHead h;
    if(fread(&h,sizeof(h),1,f)!=1||h.magic[0]!='I'||h.magic[1]!='C'||h.magic[2]!='V'||h.magic[3]!='1'||h.count>200000){fclose(f);return;}
    uint64_t totalPoints=0;const uint64_t maxPoints=16000000u;
    for(uint32_t i=0;i<h.count;i++){
        StrokeHead sh;if(fread(&sh,sizeof(sh),1,f)!=1)break;
        if(sh.n==0||sh.n>1000000||totalPoints+(uint64_t)sh.n>maxPoints||!ensure_strokes(G.strokeN+1))break;
        Stroke*s=&G.strokes[G.strokeN];memset(s,0,sizeof(*s));
        if(!ensure_points(s,(int)sh.n))break;
        size_t got=fread(s->pts,sizeof(Point),(size_t)sh.n,f);
        if(got!=(size_t)sh.n){if(s->pts)free(s->pts);memset(s,0,sizeof(*s));break;}
        s->n=(int)got;s->active=1;s->color=sh.color;s->baseWidth=sh.width;G.strokeN++;totalPoints+=sh.n;
        s->minx=s->maxx=s->pts[0].x;s->miny=s->maxy=s->pts[0].y;
        for(int k=1;k<s->n;k++){s->minx=fmin2(s->minx,s->pts[k].x);s->maxx=fmax2(s->maxx,s->pts[k].x);s->miny=fmin2(s->miny,s->pts[k].y);s->maxy=fmax2(s->maxy,s->pts[k].y);}
    }
    fclose(f);G.saveDirty=0;
}
static int save_image_meta(void){
    if(!G.imagesDirty)return 1;if(!G.imageMetaPath[0])return 0;char tmp[544];void*f=atomic_write_begin(G.imageMetaPath,tmp,sizeof(tmp));if(!f)return 0;uint32_t c=0;for(int i=0;i<G.imageN;i++)if(G.images[i].active&&G.images[i].px)c++;
    ImageMetaFileHead h={{'I','C','M','1'},c};int ok=fwrite(&h,sizeof(h),1,f)==1;
    for(int i=0;i<G.imageN&&ok;i++){ImageObj*im=&G.images[i];if(!im->active||!im->px)continue;ImageMetaRecord m={im->pw,im->ph,im->x,im->y,im->w,im->h,im->rot};ok=fwrite(&m,sizeof(m),1,f)==1;}
    int committed=atomic_write_finish(f,tmp,G.imageMetaPath,ok);if(committed)G.imagesDirty=0;return committed;
}
static int save_images(void){
    int meta=save_image_meta();if(!G.imagePixelsDirty)return meta;if(!G.imagePath[0])return 0;char tmp[544];void*f=atomic_write_begin(G.imagePath,tmp,sizeof(tmp));if(!f)return 0;uint32_t c=0;for(int i=0;i<G.imageN;i++)if(G.images[i].active&&G.images[i].px)c++;
    ImageFileHead h={{'I','C','I','2'},c};int ok=fwrite(&h,sizeof(h),1,f)==1;
    for(int i=0;i<G.imageN&&ok;i++){ImageObj*im=&G.images[i];if(!im->active||!im->px)continue;uint32_t pc=(uint32_t)im->pw*(uint32_t)im->ph;ImageHead2 ih={im->pw,im->ph,im->x,im->y,im->w,im->h,im->rot,pc};ok=fwrite(&ih,sizeof(ih),1,f)==1&&fwrite(im->px,sizeof(uint32_t),pc,f)==pc;}
    int committed=atomic_write_finish(f,tmp,G.imagePath,ok);if(committed)G.imagePixelsDirty=G.imageSavePending=0;return committed&&meta;
}
static void load_image_meta(void){if(!G.imageMetaPath[0]||G.imageN<=0)return;void*f=fopen(G.imageMetaPath,"rb");if(!f)return;ImageMetaFileHead h;if(fread(&h,sizeof(h),1,f)!=1||h.magic[0]!='I'||h.magic[1]!='C'||h.magic[2]!='M'||h.magic[3]!='1'||h.count!=(uint32_t)G.imageN){fclose(f);return;}for(int i=0;i<G.imageN;i++){ImageMetaRecord m;if(fread(&m,sizeof(m),1,f)!=1||m.pw!=G.images[i].pw||m.ph!=G.images[i].ph){fclose(f);return;}G.images[i].x=m.x;G.images[i].y=m.y;G.images[i].w=m.w;G.images[i].h=m.h;G.images[i].rot=m.rot;}fclose(f);}
static void load_images(void){G.imageContentRevision++;if(!G.imagePath[0])return;void*f=fopen(G.imagePath,"rb");if(!f)return;ImageFileHead h;if(fread(&h,sizeof(h),1,f)!=1||h.magic[0]!='I'||h.magic[1]!='C'||h.magic[2]!='I'||(h.magic[3]!='1'&&h.magic[3]!='2')||h.count>32){fclose(f);return;}int v2=h.magic[3]=='2';for(uint32_t i=0;i<h.count&&G.imageN<32;i++){int32_t pw=0,ph=0;float x=0,y=0,w=0,hh=0,rot=0;uint32_t pc=0;if(v2){ImageHead2 ih;if(fread(&ih,sizeof(ih),1,f)!=1)break;pw=ih.pw;ph=ih.ph;x=ih.x;y=ih.y;w=ih.w;hh=ih.h;rot=ih.rot;pc=ih.pixelCount;}else{ImageHead ih;if(fread(&ih,sizeof(ih),1,f)!=1)break;pw=ih.pw;ph=ih.ph;x=ih.x;y=ih.y;w=ih.w;hh=ih.h;pc=ih.pixelCount;}uint64_t expected=(uint64_t)(uint32_t)pw*(uint64_t)(uint32_t)ph;if(pw<=0||ph<=0||pw>8192||ph>8192||expected>67108864u||pc!=(uint32_t)expected)break;ImageObj*im=&G.images[G.imageN++];memset(im,0,sizeof(*im));im->pw=pw;im->ph=ph;im->x=x;im->y=y;im->w=w;im->h=hh;im->rot=rot;im->active=1;im->px=(uint32_t*)malloc((size_t)pc*sizeof(uint32_t));if(!im->px||fread(im->px,sizeof(uint32_t),pc,f)!=pc){if(im->px)free(im->px);memset(im,0,sizeof(*im));G.imageN--;break;}}fclose(f);load_image_meta();G.imagesDirty=G.imagePixelsDirty=G.imageSavePending=0;}
static void meta_apply_v2(float pmin,float pmax,int curve,int snap){
    G.pressureMin=clampf(pmin,0.02f,1.2f);G.pressureMax=clampf(pmax,G.pressureMin+0.05f,1.8f);
    G.pressureCurve=maxi(0,mini(2,curve));G.snap=snap?1:0;
}
static void meta_apply_v3(float uiScale,Theme theme,int pressAction,int holdAction,int doubleAction){
    G.uiScale=clampf(uiScale,0.70f,1.55f);G.theme=theme;
    G.buttonPressAction=(pressAction>=0&&pressAction<BA_COUNT)?pressAction:BA_UNDO;
    G.buttonHoldAction=(holdAction>=0&&holdAction<BA_COUNT)?holdAction:BA_QUICK_ERASE;
    G.buttonDoubleAction=(doubleAction>=0&&doubleAction<BA_COUNT)?doubleAction:BA_REDO;
}
static void meta_apply_v4(float pressureSmoothing,float strokeSmoothing,float highlighterOpacity,float brush,uint32_t inkColor){
    G.pressureSmoothing=clampf(pressureSmoothing,0,1);G.strokeSmoothing=clampf(strokeSmoothing,0,1);
    G.highlighterOpacity=clampf(highlighterOpacity,0.08f,0.80f);G.brush=clampf(brush,1.0f,72.0f);G.color=inkColor&0xffffffu;
}
static void meta_apply_v5(float cadScale,int cadCalibrated,int cadUnit,int minimap){
    G.cadScale=cadScale>0.000001f?cadScale:1.0f;G.cadCalibrated=cadCalibrated?1:0;
    G.cadUnit=(cadUnit>=0&&cadUnit<5)?cadUnit:2;G.minimap=minimap?1:0;
}
static void meta_apply_v6(float radialHoldSec,int buttonBindingsEnabled){
    G.radialHoldSec=clampf(radialHoldSec,0.35f,3.0f);G.buttonBindingsEnabled=buttonBindingsEnabled?1:0;
}
static void meta_apply_v7(const MetaHead7*h){
    G.atmosphere=maxi(0,mini(2,h->atmosphere));G.gridStyle=maxi(0,mini(3,h->gridStyle));
    G.gridDepth=maxi(0,mini(2,h->gridDepth));G.edgeGlass=maxi(0,mini(4,h->edgeGlass));
    G.planeElevation=maxi(0,mini(2,h->planeElevation));G.regionGlow=maxi(0,mini(2,h->regionGlow));
    G.motionStyle=maxi(0,mini(2,h->motionStyle));G.farZoomMode=maxi(0,mini(2,h->farZoomMode));
    G.performanceMode=maxi(0,mini(2,h->performanceMode));G.motionIntensity=clampf(h->motionIntensity,0,1.5f);
}
static void load_measure_payload(void*f,uint32_t declared){
    uint32_t want=declared>64u?64u:declared;G.measureN=0;G.measureLoadMapCount=(int)want;memset(G.measures,0,sizeof(G.measures));for(int i=0;i<64;i++)G.measureLoadMap[i]=-1;
    if(want){Measure raw[64];memset(raw,0,sizeof(raw));int got=(int)fread(raw,sizeof(Measure),(size_t)want,f),out=0;for(int i=0;i<got;i++)if(raw[i].active){G.measureLoadMap[i]=out;G.measures[out++]=raw[i];}G.measureN=out;}
}
static int save_meta(void){
    if(!G.metaDirty)return 1;if(!G.metaPath[0])return 0;char tmp[544];void*f=atomic_write_begin(G.metaPath,tmp,sizeof(tmp));if(!f)return 0;
    MetaHead8 h={{'I','C','M','8'},G.pressureMin,G.pressureMax,G.pressureCurve,G.snap,(uint32_t)G.measureN,G.uiScale,G.theme,G.buttonPressAction,G.buttonHoldAction,G.buttonDoubleAction,G.pressureSmoothing,G.strokeSmoothing,G.highlighterOpacity,G.brush,G.color&0xffffffu,G.cadScale,G.cadCalibrated,G.cadUnit,G.minimap,G.radialHoldSec,G.buttonBindingsEnabled,G.atmosphere,G.gridStyle,G.gridDepth,G.edgeGlass,G.planeElevation,G.regionGlow,G.motionStyle,G.farZoomMode,G.performanceMode,G.motionIntensity,G.photoAngleSnap};
    int ok=fwrite(&h,sizeof(h),1,f)==1;
    if(ok&&G.measureN)ok=fwrite(G.measures,sizeof(Measure),(size_t)G.measureN,f)==(size_t)G.measureN;
    int committed=atomic_write_finish(f,tmp,G.metaPath,ok);if(committed)G.metaDirty=0;return committed;
}
static void load_meta(void){
    if(!G.metaPath[0])return;char magic[4];void*f=fopen(G.metaPath,"rb");if(!f)return;
    if(fread(magic,1,4,f)!=4||magic[0]!='I'||magic[1]!='C'||magic[2]!='M'){fclose(f);return;}
    fclose(f);f=fopen(G.metaPath,"rb");if(!f)return;int loaded=0;uint32_t count=0;
    if(magic[3]=='8'){
        MetaHead8 h;if(fread(&h,sizeof(h),1,f)==1){MetaHead7 h7;memcpy(&h7,&h,sizeof(h7));meta_apply_v2(h.pmin,h.pmax,h.curve,h.snap);meta_apply_v3(h.uiScale,h.theme,h.pressAction,h.holdAction,h.doubleAction);meta_apply_v4(h.pressureSmoothing,h.strokeSmoothing,h.highlighterOpacity,h.brush,h.inkColor);meta_apply_v5(h.cadScale,h.cadCalibrated,h.cadUnit,h.minimap);meta_apply_v6(h.radialHoldSec,h.buttonBindingsEnabled);meta_apply_v7(&h7);G.photoAngleSnap=h.photoAngleSnap?1:0;count=h.count;loaded=1;}
    }else if(magic[3]=='7'){
        MetaHead7 h;if(fread(&h,sizeof(h),1,f)==1){meta_apply_v2(h.pmin,h.pmax,h.curve,h.snap);meta_apply_v3(h.uiScale,h.theme,h.pressAction,h.holdAction,h.doubleAction);meta_apply_v4(h.pressureSmoothing,h.strokeSmoothing,h.highlighterOpacity,h.brush,h.inkColor);meta_apply_v5(h.cadScale,h.cadCalibrated,h.cadUnit,h.minimap);meta_apply_v6(h.radialHoldSec,h.buttonBindingsEnabled);meta_apply_v7(&h);count=h.count;loaded=1;}
    }else if(magic[3]=='6'){
        MetaHead6 h;if(fread(&h,sizeof(h),1,f)==1){meta_apply_v2(h.pmin,h.pmax,h.curve,h.snap);meta_apply_v3(h.uiScale,h.theme,h.pressAction,h.holdAction,h.doubleAction);meta_apply_v4(h.pressureSmoothing,h.strokeSmoothing,h.highlighterOpacity,h.brush,h.inkColor);meta_apply_v5(h.cadScale,h.cadCalibrated,h.cadUnit,h.minimap);meta_apply_v6(h.radialHoldSec,h.buttonBindingsEnabled);count=h.count;loaded=1;}
    }else if(magic[3]=='5'){
        MetaHead5 h;if(fread(&h,sizeof(h),1,f)==1){meta_apply_v2(h.pmin,h.pmax,h.curve,h.snap);meta_apply_v3(h.uiScale,h.theme,h.pressAction,h.holdAction,h.doubleAction);meta_apply_v4(h.pressureSmoothing,h.strokeSmoothing,h.highlighterOpacity,h.brush,h.inkColor);meta_apply_v5(h.cadScale,h.cadCalibrated,h.cadUnit,h.minimap);count=h.count;loaded=1;}
    }else if(magic[3]=='4'){
        MetaHead4 h;if(fread(&h,sizeof(h),1,f)==1){meta_apply_v2(h.pmin,h.pmax,h.curve,h.snap);meta_apply_v3(h.uiScale,h.theme,h.pressAction,h.holdAction,h.doubleAction);meta_apply_v4(h.pressureSmoothing,h.strokeSmoothing,h.highlighterOpacity,h.brush,h.inkColor);count=h.count;loaded=1;}
    }else if(magic[3]=='3'){
        MetaHead3 h;if(fread(&h,sizeof(h),1,f)==1){meta_apply_v2(h.pmin,h.pmax,h.curve,h.snap);meta_apply_v3(h.uiScale,h.theme,h.pressAction,h.holdAction,h.doubleAction);count=h.count;loaded=1;}
    }else if(magic[3]=='2'){
        MetaHead2 h;if(fread(&h,sizeof(h),1,f)==1){meta_apply_v2(h.pmin,h.pmax,h.curve,h.snap);count=h.count;loaded=1;}
    }
    if(loaded){load_measure_payload(f,count);G.metaDirty=0;}fclose(f);
}


// ---------- Android media / JNI bridge ----------
static int jni_clear(JNIEnv*e){if((*e)->ExceptionCheck(e)){(*e)->ExceptionClear(e);return 1;}return 0;}
static const char*photo_permission(void){return G.activity&&G.activity->sdkVersion>=33?"android.permission.READ_MEDIA_IMAGES":"android.permission.READ_EXTERNAL_STORAGE";}
static int check_permission_name(JNIEnv*e,const char*name){jclass ac=(*e)->GetObjectClass(e,G.activity->clazz);jmethodID mid=(*e)->GetMethodID(e,ac,"checkSelfPermission","(Ljava/lang/String;)I");jstring p=(*e)->NewStringUTF(e,name);jint r=(*e)->CallIntMethod(e,G.activity->clazz,mid,p);int bad=jni_clear(e);(*e)->DeleteLocalRef(e,p);(*e)->DeleteLocalRef(e,ac);return !bad&&r==0;}
static int photo_permission_granted(void){if(!G.activity||!G.activity->env)return 0;JNIEnv*e=G.activity->env;if(check_permission_name(e,photo_permission()))return 1;if(G.activity->sdkVersion>=34&&check_permission_name(e,"android.permission.READ_MEDIA_VISUAL_USER_SELECTED"))return 1;return 0;}
static void request_photo_permission(void){if(!G.activity||!G.activity->env)return;JNIEnv*e=G.activity->env;jclass ac=(*e)->GetObjectClass(e,G.activity->clazz);jmethodID mid=(*e)->GetMethodID(e,ac,"requestPermissions","([Ljava/lang/String;I)V");jclass sc=(*e)->FindClass(e,"java/lang/String");int n=G.activity->sdkVersion>=34?2:1;jobjectArray ar=(*e)->NewObjectArray(e,n,sc,0);jstring p=(*e)->NewStringUTF(e,photo_permission());(*e)->SetObjectArrayElement(e,ar,0,p);jstring p2=0;if(n==2){p2=(*e)->NewStringUTF(e,"android.permission.READ_MEDIA_VISUAL_USER_SELECTED");(*e)->SetObjectArrayElement(e,ar,1,p2);}(*e)->CallVoidMethod(e,G.activity->clazz,mid,ar,1701);jni_clear(e);if(p2)(*e)->DeleteLocalRef(e,p2);(*e)->DeleteLocalRef(e,p);(*e)->DeleteLocalRef(e,ar);(*e)->DeleteLocalRef(e,sc);(*e)->DeleteLocalRef(e,ac);G.mediaPermissionPending=1;}
static jobject get_resolver(JNIEnv*e){jclass ac=(*e)->GetObjectClass(e,G.activity->clazz);jmethodID mid=(*e)->GetMethodID(e,ac,"getContentResolver","()Landroid/content/ContentResolver;");jobject r=(*e)->CallObjectMethod(e,G.activity->clazz,mid);jni_clear(e);(*e)->DeleteLocalRef(e,ac);return r;}
static jobject media_base_uri(JNIEnv*e){jclass mc=(*e)->FindClass(e,"android/provider/MediaStore$Images$Media");if(!mc||jni_clear(e))return 0;jfieldID fid=(*e)->GetStaticFieldID(e,mc,"EXTERNAL_CONTENT_URI","Landroid/net/Uri;");jobject u=fid?(*e)->GetStaticObjectField(e,mc,fid):0;jni_clear(e);(*e)->DeleteLocalRef(e,mc);return u;}
static jobject media_uri_for_id(JNIEnv*e,int64_t id){jobject base=media_base_uri(e);if(!base)return 0;jclass cc=(*e)->FindClass(e,"android/content/ContentUris");jmethodID mid=cc?(*e)->GetStaticMethodID(e,cc,"withAppendedId","(Landroid/net/Uri;J)Landroid/net/Uri;"):0;jobject u=mid?(*e)->CallStaticObjectMethod(e,cc,mid,base,(jlong)id):0;jni_clear(e);if(cc)(*e)->DeleteLocalRef(e,cc);(*e)->DeleteLocalRef(e,base);return u;}
static void free_thumbs(void){for(int i=0;i<12;i++){if(G.thumbs[i].px)free(G.thumbs[i].px);memset(&G.thumbs[i],0,sizeof(Thumb));}G.thumbN=0;G.galleryLoadedPage=-1;}
static uint32_t* load_media_bitmap(int64_t id,int target,int*outW,int*outH){*outW=*outH=0;if(!G.activity||!G.activity->env||G.activity->sdkVersion<29)return 0;JNIEnv*e=G.activity->env;jobject resolver=get_resolver(e);jobject uri=media_uri_for_id(e,id);if(!resolver||!uri){if(resolver)(*e)->DeleteLocalRef(e,resolver);if(uri)(*e)->DeleteLocalRef(e,uri);return 0;}jclass szc=(*e)->FindClass(e,"android/util/Size");jmethodID ctor=szc?(*e)->GetMethodID(e,szc,"<init>","(II)V"):0;jobject size=ctor?(*e)->NewObject(e,szc,ctor,target,target):0;jclass rc=(*e)->GetObjectClass(e,resolver);jmethodID lm=(*e)->GetMethodID(e,rc,"loadThumbnail","(Landroid/net/Uri;Landroid/util/Size;Landroid/os/CancellationSignal;)Landroid/graphics/Bitmap;");jobject bm=(lm&&size)?(*e)->CallObjectMethod(e,resolver,lm,uri,size,0):0;if(jni_clear(e))bm=0;uint32_t*out=0;if(bm){jclass bc=(*e)->GetObjectClass(e,bm);jmethodID gw=(*e)->GetMethodID(e,bc,"getWidth","()I"),gh=(*e)->GetMethodID(e,bc,"getHeight","()I"),gp=(*e)->GetMethodID(e,bc,"getPixels","([IIIIIII)V");int w=gw?(*e)->CallIntMethod(e,bm,gw):0,h=gh?(*e)->CallIntMethod(e,bm,gh):0;if(w>0&&h>0&&w<=4096&&h<=4096&&gp){jsize n=(jsize)(w*h);jintArray ar=(*e)->NewIntArray(e,n);if(ar){(*e)->CallVoidMethod(e,bm,gp,ar,0,w,0,0,w,h);if(!jni_clear(e)){jint*tmp=(jint*)malloc((size_t)n*sizeof(jint));out=(uint32_t*)malloc((size_t)n*sizeof(uint32_t));if(tmp&&out){(*e)->GetIntArrayRegion(e,ar,0,n,tmp);if(!jni_clear(e)){for(int i=0;i<n;i++)out[i]=(uint32_t)tmp[i]&0x00ffffffu;*outW=w;*outH=h;}else{free(out);out=0;}}if(tmp)free(tmp);}(*e)->DeleteLocalRef(e,ar);}}jmethodID rec=(*e)->GetMethodID(e,bc,"recycle","()V");if(rec)(*e)->CallVoidMethod(e,bm,rec);jni_clear(e);(*e)->DeleteLocalRef(e,bc);(*e)->DeleteLocalRef(e,bm);}(*e)->DeleteLocalRef(e,rc);if(size)(*e)->DeleteLocalRef(e,size);if(szc)(*e)->DeleteLocalRef(e,szc);(*e)->DeleteLocalRef(e,uri);(*e)->DeleteLocalRef(e,resolver);return out;}
static void photo_decoder_close(void){if(!G.activity||!G.activity->env||!G.photoImportClass)return;JNIEnv*e=G.activity->env;jclass cls=(jclass)G.photoImportClass;jmethodID closeMethod=(*e)->GetStaticMethodID(e,cls,"photoClose","()V");if(closeMethod)(*e)->CallStaticVoidMethod(e,cls,closeMethod);jni_clear(e);}
static void photo_import_reset(int releasePixels){photo_decoder_close();if(G.activity&&G.activity->env&&G.photoImportClass)(*G.activity->env)->DeleteGlobalRef(G.activity->env,G.photoImportClass);G.photoImportClass=0;if(releasePixels&&G.photoImportPixels)free(G.photoImportPixels);G.photoImportPixels=0;G.photoImportActive=0;G.photoImportMediaId=0;G.photoImportSourceW=G.photoImportSourceH=G.photoImportSample=G.photoImportOrientation=G.photoImportW=G.photoImportH=G.photoImportLeft=G.photoImportTop=0;}
static int photo_import_begin(int64_t id){
    if(!G.activity||!G.activity->env||G.imageN>=32)return 0;photo_import_reset(1);JNIEnv*e=G.activity->env;jclass cls=(*e)->FindClass(e,"com/ayomi/infinitecanvas/VastUi");int classError=jni_clear(e);if(!cls||classError)return 0;G.photoImportClass=(*e)->NewGlobalRef(e,cls);if(!G.photoImportClass||jni_clear(e)){(*e)->DeleteLocalRef(e,cls);photo_import_reset(1);return 0;}
    jmethodID openMethod=(*e)->GetStaticMethodID(e,cls,"photoOpen","(Landroid/app/Activity;JI)[I");if(!openMethod||jni_clear(e)){(*e)->DeleteLocalRef(e,cls);photo_import_reset(1);return 0;}
    jintArray info=(jintArray)(*e)->CallStaticObjectMethod(e,cls,openMethod,G.activity->clazz,(jlong)id,(jint)8192);if(jni_clear(e)||!info||(*e)->GetArrayLength(e,info)<6){if(info)(*e)->DeleteLocalRef(e,info);(*e)->DeleteLocalRef(e,cls);photo_import_reset(1);return 0;}jint v[6];(*e)->GetIntArrayRegion(e,info,0,6,v);(*e)->DeleteLocalRef(e,info);(*e)->DeleteLocalRef(e,cls);if(jni_clear(e)){photo_import_reset(1);return 0;}
    uint64_t count=(uint64_t)(uint32_t)v[4]*(uint64_t)(uint32_t)v[5];if(v[0]<=0||v[1]<=0||v[2]<=0||v[4]<=0||v[5]<=0||v[4]>8192||v[5]>8192||count>67108864u){photo_import_reset(1);return 0;}
    G.photoImportPixels=(uint32_t*)malloc((size_t)count*sizeof(uint32_t));if(!G.photoImportPixels){photo_import_reset(1);return 0;}memset(G.photoImportPixels,0,(size_t)count*sizeof(uint32_t));G.photoImportMediaId=id;G.photoImportSourceW=v[0];G.photoImportSourceH=v[1];G.photoImportSample=v[2];G.photoImportOrientation=v[3];G.photoImportW=v[4];G.photoImportH=v[5];G.photoImportLeft=G.photoImportTop=0;G.photoImportActive=1;G.toast=12;G.toastMs=monotonic_ms();start_anim_timer();return 1;
}
static int photo_import_step(void){
    if(!G.photoImportActive)return 0;if(!G.activity||!G.activity->env||!G.photoImportClass){photo_import_reset(1);set_toast(8,0);return 1;}JNIEnv*e=G.activity->env;jclass cls=(jclass)G.photoImportClass;jmethodID tileMethod=(*e)->GetStaticMethodID(e,cls,"photoTile","(Landroid/app/Activity;JIIIII)[I");if(!tileMethod||jni_clear(e)){photo_import_reset(1);set_toast(8,0);return 1;}
    const int outputTile=1024,left=G.photoImportLeft,top=G.photoImportTop,step=outputTile*G.photoImportSample;int right=mini(G.photoImportSourceW,left+step),bottom=mini(G.photoImportSourceH,top+step);jintArray tile=(jintArray)(*e)->CallStaticObjectMethod(e,cls,tileMethod,G.activity->clazz,(jlong)G.photoImportMediaId,(jint)left,(jint)top,(jint)right,(jint)bottom,(jint)G.photoImportSample);if(jni_clear(e)||!tile||(*e)->GetArrayLength(e,tile)<2){if(tile)(*e)->DeleteLocalRef(e,tile);photo_import_reset(1);set_toast(8,0);return 1;}jint wh[2];(*e)->GetIntArrayRegion(e,tile,0,2,wh);int tw=wh[0],th=wh[1];jsize len=(*e)->GetArrayLength(e,tile);if(tw<=0||th<=0||(uint64_t)tw*(uint64_t)th>(uint64_t)(len-2)){(*e)->DeleteLocalRef(e,tile);photo_import_reset(1);set_toast(8,0);return 1;}jint*pixels=(jint*)malloc((size_t)tw*th*sizeof(jint));if(!pixels){(*e)->DeleteLocalRef(e,tile);photo_import_reset(1);set_toast(8,0);return 1;}(*e)->GetIntArrayRegion(e,tile,2,tw*th,pixels);(*e)->DeleteLocalRef(e,tile);if(jni_clear(e)){free(pixels);photo_import_reset(1);set_toast(8,0);return 1;}
    int baseX=left/G.photoImportSample,baseY=top/G.photoImportSample,w=G.photoImportW,h=G.photoImportH,o=G.photoImportOrientation;for(int yy=0;yy<th;yy++)for(int xx=0;xx<tw;xx++){int gx=baseX+xx,gy=baseY+yy,ox=gx,oy=gy;if(o==90){ox=w-1-gy;oy=gx;}else if(o==180){ox=w-1-gx;oy=h-1-gy;}else if(o==270){ox=gy;oy=h-1-gx;}if(ox>=0&&ox<w&&oy>=0&&oy<h)G.photoImportPixels[(size_t)oy*w+ox]=(uint32_t)pixels[yy*tw+xx]&0x00ffffffu;}free(pixels);
    G.photoImportLeft+=step;if(G.photoImportLeft>=G.photoImportSourceW){G.photoImportLeft=0;G.photoImportTop+=step;}G.toast=12;G.toastMs=monotonic_ms();if(G.photoImportTop<G.photoImportSourceH)return 1;
    uint32_t*complete=G.photoImportPixels;G.photoImportPixels=0;photo_import_reset(0);ImageObj*im=&G.images[G.imageN++];memset(im,0,sizeof(*im));im->px=complete;im->pw=w;im->ph=h;im->active=1;float ww=800.0f,hh=ww*(float)h/(float)w;if(hh>800.0f){hh=800.0f;ww=hh*(float)w/(float)h;}float cx=(G.screenW*.5f-G.offX)/G.scale,cy=(G.screenH*.5f-G.offY)/G.scale;im->w=ww;im->h=hh;im->x=cx-ww*.5f;im->y=cy-hh*.5f;G.imageContentRevision++;G.imagesDirty=G.imagePixelsDirty=G.imageSavePending=1;G.minimapDirty=1;G.selectedImage=G.imageN-1;G.photoMode=1;set_toast(3,0);return 1;
}
static int query_media(void){G.mediaCount=0;if(!photo_permission_granted())return 0;JNIEnv*e=G.activity->env;jobject resolver=get_resolver(e),uri=media_base_uri(e);if(!resolver||!uri){if(resolver)(*e)->DeleteLocalRef(e,resolver);if(uri)(*e)->DeleteLocalRef(e,uri);return 0;}jclass sc=(*e)->FindClass(e,"java/lang/String");jobjectArray proj=(*e)->NewObjectArray(e,1,sc,0);jstring idstr=(*e)->NewStringUTF(e,"_id");(*e)->SetObjectArrayElement(e,proj,0,idstr);jstring sort=(*e)->NewStringUTF(e,"date_added DESC");jclass rc=(*e)->GetObjectClass(e,resolver);jmethodID qm=(*e)->GetMethodID(e,rc,"query","(Landroid/net/Uri;[Ljava/lang/String;Ljava/lang/String;[Ljava/lang/String;Ljava/lang/String;)Landroid/database/Cursor;");jobject cur=qm?(*e)->CallObjectMethod(e,resolver,qm,uri,proj,0,0,sort):0;if(jni_clear(e))cur=0;if(cur){jclass cc=(*e)->GetObjectClass(e,cur);jmethodID mv=(*e)->GetMethodID(e,cc,"moveToNext","()Z"),gl=(*e)->GetMethodID(e,cc,"getLong","(I)J"),cl=(*e)->GetMethodID(e,cc,"close","()V");while(G.mediaCount<96&&mv&&(*e)->CallBooleanMethod(e,cur,mv)){if(jni_clear(e))break;G.mediaIds[G.mediaCount++]=(int64_t)(*e)->CallLongMethod(e,cur,gl,0);if(jni_clear(e))break;}if(cl)(*e)->CallVoidMethod(e,cur,cl);jni_clear(e);(*e)->DeleteLocalRef(e,cc);(*e)->DeleteLocalRef(e,cur);}(*e)->DeleteLocalRef(e,rc);(*e)->DeleteLocalRef(e,sort);(*e)->DeleteLocalRef(e,idstr);(*e)->DeleteLocalRef(e,proj);(*e)->DeleteLocalRef(e,sc);(*e)->DeleteLocalRef(e,uri);(*e)->DeleteLocalRef(e,resolver);return G.mediaCount;}
static void load_gallery_page(void){free_thumbs();int start=G.galleryPage*12;for(int i=0;i<12&&start+i<G.mediaCount;i++){Thumb*t=&G.thumbs[G.thumbN];t->mediaId=G.mediaIds[start+i];t->px=load_media_bitmap(t->mediaId,us(260),&t->w,&t->h);if(t->px)G.thumbN++;}G.galleryLoadedPage=G.galleryPage;}
static void open_photo_gallery(void){G.settingsPanel=0;G.pressurePanel=0;if(!photo_permission_granted()){request_photo_permission();set_toast(4,0);return;}if(query_media()<=0){set_toast(5,0);return;}G.galleryPage=0;G.galleryOpen=1;load_gallery_page();}
static void import_gallery_item(int idx){if(idx<0||idx>=G.thumbN||G.imageN>=32)return;int64_t id=G.thumbs[idx].mediaId;if(!photo_import_begin(id)){set_toast(8,0);return;}G.galleryOpen=0;free_thumbs();}


// ---------- Frames / workspace structure ----------
typedef struct { char magic[4]; int32_t count,nextId,visMask; } FrameHead;
typedef struct { FrameObj frame; char name[32]; int32_t locked; } FrameDisk2;
typedef struct { FrameObj frame; char name[32]; int32_t locked,sizeLocked; } FrameDisk3;
static int layer_mask(void){return (G.visInk?1:0)|(G.visMarker?2:0)|(G.visPhotos?4:0)|(G.visCAD?8:0)|(G.visFrames?16:0)|(G.visNotes?32:0)|(G.visShapes?64:0);}
static void apply_layer_mask(int m){G.visInk=(m&1)?1:0;G.visMarker=(m&2)?1:0;G.visPhotos=(m&4)?1:0;G.visCAD=(m&8)?1:0;G.visFrames=(m&16)?1:0;G.visNotes=(m&32)?1:0;G.visShapes=(m&64)?1:0;}
static int frame_default_name_number(const char*name){
    if(!name||name[0]!='F'||name[1]!='r'||name[2]!='a'||name[3]!='m'||name[4]!='e'||name[5]!=' ')return 0;
    int i=6,n=0,have=0;for(;name[i]>='0'&&name[i]<='9';i++){have=1;n=n*10+(name[i]-'0');if(n>9999)return 0;}
    return have&&name[i]==0?n:0;
}
static int frame_default_name_count(void){int n=0;for(int i=0;i<G.frameN;i++)if(frame_default_name_number(G.frameNames[i])>0)n++;return n;}
static void frame_compact_default_names(void){int n=1;for(int i=0;i<G.frameN;i++)if(frame_default_name_number(G.frameNames[i])>0)snprintf(G.frameNames[i],32,"Frame %d",n++);}
static int save_frames(void){
    if(!G.framePath[0])return 0;char tmp[544];void*f=atomic_write_begin(G.framePath,tmp,sizeof(tmp));if(!f)return 0;FrameHead h={{'V','F','R','3'},G.frameN,G.nextFrameId,layer_mask()};int ok=fwrite(&h,sizeof(h),1,f)==1;
    for(int i=0;i<G.frameN&&ok;i++){FrameDisk3 d;memset(&d,0,sizeof(d));d.frame=G.frames[i];memcpy(d.name,G.frameNames[i],32);d.locked=G.frameLocked[i];d.sizeLocked=G.frameSizeLocked[i];ok=fwrite(&d,sizeof(d),1,f)==1;}
    return atomic_write_finish(f,tmp,G.framePath,ok);
}
static void load_frames(void){
    if(!G.framePath[0])return;void*f=fopen(G.framePath,"rb");if(!f)return;FrameHead h;
    if(fread(&h,sizeof(h),1,f)!=1||h.magic[0]!='V'||h.magic[1]!='F'||h.magic[2]!='R'||(h.magic[3]!='1'&&h.magic[3]!='2'&&h.magic[3]!='3')||h.count<0||h.count>48){fclose(f);return;}
    memset(G.frames,0,sizeof(G.frames));memset(G.frameNames,0,sizeof(G.frameNames));memset(G.frameLocked,0,sizeof(G.frameLocked));memset(G.frameSizeLocked,0,sizeof(G.frameSizeLocked));
    G.frameN=0;G.nextFrameId=maxi(1,h.nextId);apply_layer_mask(h.visMask);
    int want=h.count;
    if(h.magic[3]=='3'){
        for(int i=0;i<want;i++){FrameDisk3 d;if(fread(&d,sizeof(d),1,f)!=1)break;G.frames[G.frameN]=d.frame;memcpy(G.frameNames[G.frameN],d.name,32);G.frameNames[G.frameN][31]=0;G.frameLocked[G.frameN]=d.locked?1:0;G.frameSizeLocked[G.frameN]=d.sizeLocked?1:0;G.frameN++;}
    }else if(h.magic[3]=='2'){
        for(int i=0;i<want;i++){FrameDisk2 d;if(fread(&d,sizeof(d),1,f)!=1)break;G.frames[G.frameN]=d.frame;memcpy(G.frameNames[G.frameN],d.name,32);G.frameNames[G.frameN][31]=0;G.frameLocked[G.frameN]=d.locked?1:0;G.frameSizeLocked[G.frameN]=0;G.frameN++;}
    }else{
        G.frameN=want?(int)fread(G.frames,sizeof(FrameObj),(size_t)want,f):0;
        for(int i=0;i<G.frameN;i++){snprintf(G.frameNames[i],32,"Frame %d",G.frames[i].id);G.frameLocked[i]=0;G.frameSizeLocked[i]=0;}
    }
    fclose(f);frame_compact_default_names();
}
static void frame_changed(void){G.frameRevision++;G.minimapDirty=1;save_frames();}
static void frame_normalize(FrameObj*f){if(f->w<0){f->x+=f->w;f->w=-f->w;}if(f->h<0){f->y+=f->h;f->h=-f->h;}}
static void frame_add_live(void){FrameObj f=G.liveFrame;frame_normalize(&f);if(f.w*G.scale<36.0f||f.h*G.scale<36.0f)return;if(G.frameN>=48)return;f.id=G.nextFrameId++;f.active=1;f.color=(f.id%3==0)?G.theme.cyan:((f.id%3==1)?G.theme.accent:G.theme.violet);int fi=G.frameN;G.frames[G.frameN++]=f;snprintf(G.frameNames[fi],32,"Frame %d",frame_default_name_count()+1);G.frameLocked[fi]=0;G.frameSizeLocked[fi]=0;G.selectedFrame=G.frameN-1;selection_set_one(SEL_FRAME,G.selectedFrame);frame_changed();}
static void frame_delete_selected(void){if(G.selectedFrame<0||G.selectedFrame>=G.frameN||G.lockFrames||G.frameLocked[G.selectedFrame])return;int removed=G.selectedFrame,old=G.frameN,map[48];for(int i=0;i<old;i++)map[i]=i<removed?i:(i==removed?-1:i-1);for(int i=removed+1;i<G.frameN;i++){G.frames[i-1]=G.frames[i];memcpy(G.frameNames[i-1],G.frameNames[i],32);G.frameLocked[i-1]=G.frameLocked[i];G.frameSizeLocked[i-1]=G.frameSizeLocked[i];}G.frameN--;if(G.frameN>=0){memset(&G.frames[G.frameN],0,sizeof(FrameObj));memset(G.frameNames[G.frameN],0,32);G.frameLocked[G.frameN]=G.frameSizeLocked[G.frameN]=0;}remap_object_refs(SEL_FRAME,map,old);frame_compact_default_names();if(G.frameN<=0)G.selectedFrame=-1;else if(G.selectedFrame>=G.frameN)G.selectedFrame=G.frameN-1;frame_changed();save_workspace();}
static void frame_jump(int idx){if(idx<0||idx>=G.frameN)return;FrameObj*f=&G.frames[idx];if(!f->active)return;float margin=120.0f;float sx=(G.screenW-2*margin)/fmax2(f->w,1.0f),sy=(G.screenH-2*margin)/fmax2(f->h,1.0f);G.scale=clampf(fmin2(sx,sy),0.02f,20.0f);G.offX=G.screenW*.5f-(f->x+f->w*.5f)*G.scale;G.offY=G.screenH*.5f-(f->y+f->h*.5f)*G.scale;G.selectedFrame=idx;G.minimapDirty=1;}
static int frame_contents_count(FrameObj*f){int n=0;float x0=f->x,y0=f->y,x1=f->x+f->w,y1=f->y+f->h;for(int i=0;i<G.strokeN;i++){Stroke*s=&G.strokes[i];if(!s->active)continue;float cx=(s->minx+s->maxx)*.5f,cy=(s->miny+s->maxy)*.5f;if(cx>=x0&&cx<=x1&&cy>=y0&&cy<=y1)n++;}for(int i=0;i<G.imageN;i++){ImageObj*im=&G.images[i];if(!im->active)continue;float cx=im->x+im->w*.5f,cy=im->y+im->h*.5f;if(cx>=x0&&cx<=x1&&cy>=y0&&cy<=y1)n++;}for(int i=0;i<G.measureN;i++){Measure*m=&G.measures[i];if(!m->active)continue;float cx=(m->ax+m->bx)*.5f,cy=(m->ay+m->by)*.5f;if(cx>=x0&&cx<=x1&&cy>=y0&&cy<=y1)n++;}for(int i=0;i<G.noteN;i++){NoteObj*q=&G.notes[i];if(q->active&&q->x+q->w*.5f>=x0&&q->x+q->w*.5f<=x1&&q->y+q->h*.5f>=y0&&q->y+q->h*.5f<=y1)n++;}for(int i=0;i<G.shapeN;i++){ShapeObj*q=&G.shapes[i];float cx=(q->x0+q->x1)*.5f,cy=(q->y0+q->y1)*.5f;if(q->active&&cx>=x0&&cx<=x1&&cy>=y0&&cy<=y1)n++;}return n;}
static int frame_contains_world(FrameObj*f,float x,float y){return f&&x>=f->x&&x<=f->x+f->w&&y>=f->y&&y<=f->y+f->h;}
static void frame_move_selected_by(float dx,float dy){if(G.selectedFrame<0||G.selectedFrame>=G.frameN||G.lockFrames||G.frameLocked[G.selectedFrame])return;FrameObj*f=&G.frames[G.selectedFrame];float x0=f->x,y0=f->y,x1=f->x+f->w,y1=f->y+f->h;for(int i=0;i<G.strokeN;i++){Stroke*st=&G.strokes[i];if(!st->active)continue;int aa=(int)((st->color>>24)&255),marker=aa>0&&aa<248;if((marker&&G.lockMarker)||(!marker&&G.lockInk))continue;float cx=(st->minx+st->maxx)*.5f,cy=(st->miny+st->maxy)*.5f;if(cx>=x0&&cx<=x1&&cy>=y0&&cy<=y1){for(int k=0;k<st->n;k++){st->pts[k].x+=dx;st->pts[k].y+=dy;}st->minx+=dx;st->maxx+=dx;st->miny+=dy;st->maxy+=dy;ocr_move_mark(i,dx,dy);G.saveDirty=1;}}for(int i=0;i<G.imageN;i++){ImageObj*im=&G.images[i];if(!im->active||G.lockPhotos)continue;float cx=im->x+im->w*.5f,cy=im->y+im->h*.5f;if(cx>=x0&&cx<=x1&&cy>=y0&&cy<=y1){im->x+=dx;im->y+=dy;G.imagesDirty=1;}}for(int i=0;i<G.measureN;i++){Measure*m=&G.measures[i];if(!m->active||G.lockCAD)continue;float cx=(m->ax+m->bx)*.5f,cy=(m->ay+m->by)*.5f;if(cx>=x0&&cx<=x1&&cy>=y0&&cy<=y1){m->ax+=dx;m->ay+=dy;m->bx+=dx;m->by+=dy;G.metaDirty=1;}}for(int i=0;i<G.noteN;i++){NoteObj*n=&G.notes[i];if(!n->active||n->locked||G.lockNotes)continue;float cx=n->x+n->w*.5f,cy=n->y+n->h*.5f;if(cx>=x0&&cx<=x1&&cy>=y0&&cy<=y1){n->x+=dx;n->y+=dy;}}for(int i=0;i<G.shapeN;i++){ShapeObj*q=&G.shapes[i];if(!q->active||q->locked||G.lockShapes)continue;float cx=(q->x0+q->x1)*.5f,cy=(q->y0+q->y1)*.5f;if(cx>=x0&&cx<=x1&&cy>=y0&&cy<=y1){q->x0+=dx;q->x1+=dx;q->y0+=dy;q->y1+=dy;}}f->x+=dx;f->y+=dy;G.sceneRevision++;G.frameRevision++;G.minimapDirty=1;}

// ---------- Vast 3 workspace objects / multi-project persistence ----------
typedef struct { char magic[4]; int32_t noteN,shapeN,bookmarkN,groupN,nextNoteId,nextShapeId,nextBookmarkId,nextGroupId; float scale,offX,offY; int32_t lockMask; } WorkspaceHead;
typedef struct { uint32_t magic; float x,y,scale; int32_t valid; } WriteCheckpointDisk;
static int workspace_lock_mask(void){return (G.lockInk?1:0)|(G.lockMarker?2:0)|(G.lockPhotos?4:0)|(G.lockCAD?8:0)|(G.lockNotes?16:0)|(G.lockShapes?32:0)|(G.lockFrames?64:0);}
static void apply_workspace_locks(int m){G.lockInk=(m&1)?1:0;G.lockMarker=(m&2)?1:0;G.lockPhotos=(m&4)?1:0;G.lockCAD=(m&8)?1:0;G.lockNotes=(m&16)?1:0;G.lockShapes=(m&32)?1:0;G.lockFrames=(m&64)?1:0;}
static int save_workspace(void){
    if(!G.workspacePath[0])return 0;char tmp[544];void*f=atomic_write_begin(G.workspacePath,tmp,sizeof(tmp));if(!f)return 0;
    WorkspaceHead h={{'V','W','S','1'},G.noteN,G.shapeN,G.bookmarkN,G.groupN,G.nextNoteId,G.nextShapeId,G.nextBookmarkId,G.nextGroupId,G.scale,G.offX,G.offY,workspace_lock_mask()};
    int ok=fwrite(&h,sizeof(h),1,f)==1;for(int i=0;ok&&i<G.noteN;i++){LegacyNoteObj disk;memcpy(&disk,&G.notes[i],sizeof(disk));copy_text_local(disk.text,96,G.notes[i].text);ok=fwrite(&disk,sizeof(disk),1,f)==1;}if(ok&&G.shapeN)ok=fwrite(G.shapes,sizeof(ShapeObj),(size_t)G.shapeN,f)==(size_t)G.shapeN;if(ok&&G.bookmarkN)ok=fwrite(G.bookmarks,sizeof(BookmarkObj),(size_t)G.bookmarkN,f)==(size_t)G.bookmarkN;if(ok&&G.groupN)ok=fwrite(G.groups,sizeof(GroupObj),(size_t)G.groupN,f)==(size_t)G.groupN;uint32_t ext[2]={0x31545856u,(uint32_t)G.noteN};if(ok)ok=fwrite(ext,sizeof(ext),1,f)==1;for(int i=0;ok&&i<G.noteN;i++){int32_t nh[2]={G.notes[i].id,str_len_local(G.notes[i].text)};ok=fwrite(nh,sizeof(nh),1,f)==1&&fwrite(G.notes[i].text,1,(size_t)nh[1],f)==(size_t)nh[1];}WriteCheckpointDisk cp={0x31504356u,G.lastWriteX,G.lastWriteY,G.lastWriteScale,G.lastWriteValid};if(ok)ok=fwrite(&cp,sizeof(cp),1,f)==1;return atomic_write_finish(f,tmp,G.workspacePath,ok);
}
static void load_workspace(void){
    if(!G.workspacePath[0])return;void*f=fopen(G.workspacePath,"rb");if(!f)return;WorkspaceHead h;
    if(fread(&h,sizeof(h),1,f)!=1||h.magic[0]!='V'||h.magic[1]!='W'||h.magic[2]!='S'||h.magic[3]!='1'){fclose(f);return;}
    memset(G.notes,0,sizeof(G.notes));memset(G.shapes,0,sizeof(G.shapes));memset(G.bookmarks,0,sizeof(G.bookmarks));memset(G.groups,0,sizeof(G.groups));G.lastWriteValid=0;
    G.noteN=G.shapeN=G.bookmarkN=G.groupN=0;
    G.nextNoteId=maxi(1,h.nextNoteId);G.nextShapeId=maxi(1,h.nextShapeId);G.nextBookmarkId=maxi(1,h.nextBookmarkId);G.nextGroupId=maxi(1,h.nextGroupId);apply_workspace_locks(h.lockMask);
    int declared=h.noteN,want=maxi(0,mini(96,declared));
    if(declared<0){fclose(f);return;}G.noteN=0;
    for(int i=0;i<want;i++){LegacyNoteObj disk;if(fread(&disk,sizeof(disk),1,f)!=1)break;disk.text[95]=0;memcpy(&G.notes[i],&disk,sizeof(disk));G.noteN++;}
    for(int i=0;i<G.noteN;i++)G.notes[i].text[95]=0;
    if(declared>96||G.noteN!=want){fclose(f);return;}
    declared=h.shapeN;want=maxi(0,mini(128,declared));
    if(declared<0){fclose(f);return;}G.shapeN=want?(int)fread(G.shapes,sizeof(ShapeObj),(size_t)want,f):0;
    if(declared>128||G.shapeN!=want){fclose(f);return;}
    declared=h.bookmarkN;want=maxi(0,mini(32,declared));
    if(declared<0){fclose(f);return;}G.bookmarkN=want?(int)fread(G.bookmarks,sizeof(BookmarkObj),(size_t)want,f):0;
    for(int i=0;i<G.bookmarkN;i++)G.bookmarks[i].name[31]=0;
    if(declared>32||G.bookmarkN!=want){fclose(f);return;}
    declared=h.groupN;want=maxi(0,mini(32,declared));
    if(declared<0){fclose(f);return;}G.groupN=want?(int)fread(G.groups,sizeof(GroupObj),(size_t)want,f):0;
    for(int i=0;i<G.groupN;i++)G.groups[i].n=maxi(0,mini(48,G.groups[i].n));
    if(G.measureLoadMapCount>0){remap_object_refs(SEL_MEASURE,G.measureLoadMap,G.measureLoadMapCount);G.measureLoadMapCount=0;}
    sanitize_group_refs();
    /* Additive extension: old workspaces load unchanged, and old readers can
       still read all object records plus a UTF-8-safe preview of each note. */
    if(declared==G.groupN){uint32_t ext[2];if(fread(ext,sizeof(ext),1,f)==1&&ext[0]==0x31545856u&&ext[1]<=96){
        for(uint32_t i=0;i<ext[1];i++){int32_t nh[2];char text[NOTE_TEXT_CAP];
            if(fread(nh,sizeof(nh),1,f)!=1||nh[1]<0||nh[1]>=NOTE_TEXT_CAP)break;
            if(fread(text,1,(size_t)nh[1],f)!=(size_t)nh[1])break;text[nh[1]]=0;
            for(int n=0;n<G.noteN;n++)if(G.notes[n].id==nh[0]){copy_text_local(G.notes[n].text,NOTE_TEXT_CAP,text);break;}
        }
        WriteCheckpointDisk cp;if(fread(&cp,sizeof(cp),1,f)==1&&cp.magic==0x31504356u&&cp.valid&&cp.scale>=.05f&&cp.scale<=20.0f){G.lastWriteX=cp.x;G.lastWriteY=cp.y;G.lastWriteScale=cp.scale;G.lastWriteValid=1;}
    }}
    if(h.scale>=0.05f&&h.scale<=20.0f){G.scale=h.scale;G.offX=h.offX;G.offY=h.offY;G.viewInitialized=1;}
    fclose(f);
}
typedef struct { char magic[4]; int32_t lastIndex; char names[4][32]; } ProjectPrefs;
static void save_project_prefs(void){if(!G.projectPrefsPath[0])return;char tmp[544];void*f=atomic_write_begin(G.projectPrefsPath,tmp,sizeof(tmp));if(!f)return;ProjectPrefs p;memset(&p,0,sizeof(p));p.magic[0]='V';p.magic[1]='P';p.magic[2]='R';p.magic[3]='1';p.lastIndex=G.projectIndex;for(int i=0;i<4;i++)memcpy(p.names[i],G.projectNames[i],32);int ok=fwrite(&p,sizeof(p),1,f)==1;(void)atomic_write_finish(f,tmp,G.projectPrefsPath,ok);}
static void load_project_prefs(void){for(int i=0;i<4;i++)snprintf(G.projectNames[i],32,"Project %d",i+1);G.projectIndex=0;if(!G.projectPrefsPath[0])return;void*f=fopen(G.projectPrefsPath,"rb");if(!f)return;ProjectPrefs p;if(fread(&p,sizeof(p),1,f)==1&&p.magic[0]=='V'&&p.magic[1]=='P'&&p.magic[2]=='R'&&p.magic[3]=='1'){G.projectIndex=maxi(0,mini(3,p.lastIndex));for(int i=0;i<4;i++){memcpy(G.projectNames[i],p.names[i],32);G.projectNames[i][31]=0;if(!G.projectNames[i][0])snprintf(G.projectNames[i],32,"Project %d",i+1);}}fclose(f);}
static void project_path_for(int idx,int kind,char*out,int cap){if(!out||cap<=0){return;}out[0]=0;if(!G.activity||!G.activity->internalDataPath)return;idx=maxi(0,mini(3,idx));const char*b=G.activity->internalDataPath;const char*base[6]={"canvas","canvas","images","frames","workspace","images"};const char*ext[6]={".icv",".meta",".icv",".vfr",".v3",".meta"};kind=maxi(0,mini(5,kind));if(idx==0){if(kind==0)snprintf(out,(size_t)cap,"%s/canvas.icv",b);else if(kind==1)snprintf(out,(size_t)cap,"%s/canvas.meta",b);else if(kind==2)snprintf(out,(size_t)cap,"%s/images.icv",b);else if(kind==3)snprintf(out,(size_t)cap,"%s/frames.vfr",b);else if(kind==4)snprintf(out,(size_t)cap,"%s/workspace.v3",b);else snprintf(out,(size_t)cap,"%s/images.meta",b);}else snprintf(out,(size_t)cap,"%s/%s_%d%s",b,base[kind],idx,ext[kind]);}
static int file_exists_local(const char*path){if(!path||!path[0])return 0;void*f=fopen(path,"rb");if(!f)return 0;fclose(f);return 1;}
static int copy_file_local(const char*src,const char*dst){void*in=fopen(src,"rb");if(!in)return 0;void*out=fopen(dst,"wb");if(!out){fclose(in);return 0;}uint8_t*b=(uint8_t*)malloc(65536);if(!b){fclose(in);fclose(out);remove(dst);return 0;}int ok=1;for(;;){size_t n=fread(b,1,65536,in);if(n==0){if(ferror(in))ok=0;break;}if(fwrite(b,1,n,out)!=n){ok=0;break;}}free(b);if(fclose(in)!=0)ok=0;if(ok&&fflush(out)!=0)ok=0;if(ok){int fd=fileno(out);if(fd<0||fsync(fd)!=0)ok=0;}if(fclose(out)!=0)ok=0;if(!ok)remove(dst);return ok;}
static void project_duplicate_marker_path(int idx,char*out,int cap){if(!out||cap<=0)return;out[0]=0;if(G.activity&&G.activity->internalDataPath)snprintf(out,(size_t)cap,"%s/project_%d.dup.pending",G.activity->internalDataPath,maxi(0,mini(3,idx)));}
static int remove_file_if_present(const char*path){if(!path||!path[0])return 0;if(remove(path)==0)return 1;return !file_exists_local(path);}
static int project_recover_incomplete_duplicate(int idx){
    char marker[544];project_duplicate_marker_path(idx,marker,sizeof(marker));if(!file_exists_local(marker))return 1;int ok=1;
    for(int k=0;k<6;k++){char out[512],stage[544];project_path_for(idx,k,out,sizeof(out));snprintf(stage,sizeof(stage),"%s.dup",out);if(!remove_file_if_present(stage))ok=0;if(!remove_file_if_present(out))ok=0;}
    /* Keep the marker durable until every partial destination deletion is
       durable.  A crash before that point will retry the same cleanup. */
    if(!fsync_parent_directory(marker))ok=0;if(!ok)return 0;if(!remove_file_if_present(marker))return 0;return fsync_parent_directory(marker);
}
static int project_slot_has_data(int idx){if(!project_recover_incomplete_duplicate(idx))return 1;char p[512];for(int k=0;k<6;k++){project_path_for(idx,k,p,512);if(file_exists_local(p))return 1;}return 0;}
static void set_project_paths(int idx){
    if(!G.activity||!G.activity->internalDataPath)return;idx=maxi(0,mini(3,idx));G.projectIndex=idx;project_path_for(idx,0,G.savePath,sizeof(G.savePath));project_path_for(idx,1,G.metaPath,sizeof(G.metaPath));project_path_for(idx,2,G.imagePath,sizeof(G.imagePath));project_path_for(idx,3,G.framePath,sizeof(G.framePath));project_path_for(idx,4,G.workspacePath,sizeof(G.workspacePath));project_path_for(idx,5,G.imageMetaPath,sizeof(G.imageMetaPath));
}
static void clear_document_memory(void){
    erase_index_reset();
    for(int i=0;i<G.imageN;i++)if(G.images[i].px){free(G.images[i].px);G.images[i].px=0;}memset(G.images,0,sizeof(G.images));G.imageN=0;G.imagesDirty=G.imagePixelsDirty=G.imageSavePending=0;
    for(int i=0;i<G.strokeN;i++)if(G.strokes[i].pts){free(G.strokes[i].pts);G.strokes[i].pts=0;}if(G.strokes){free(G.strokes);G.strokes=0;}G.strokeN=G.strokeCap=0;
    for(int i=0;i<G.actionN;i++)free_action(&G.actions[i]);if(G.actions){free(G.actions);G.actions=0;}G.actionN=G.actionCap=G.actionCursor=0;free_action(&G.eraseAction);G.currentStroke=-1;G.erasing=G.eraseEverythingActive=0;
    memset(G.measures,0,sizeof(G.measures));memset(G.frames,0,sizeof(G.frames));memset(&G.liveFrame,0,sizeof(G.liveFrame));memset(G.frameNames,0,sizeof(G.frameNames));memset(G.frameLocked,0,sizeof(G.frameLocked));memset(G.frameSizeLocked,0,sizeof(G.frameSizeLocked));
    memset(G.notes,0,sizeof(G.notes));memset(G.shapes,0,sizeof(G.shapes));memset(&G.liveShape,0,sizeof(G.liveShape));memset(G.bookmarks,0,sizeof(G.bookmarks));memset(G.groups,0,sizeof(G.groups));
    memset(G.selectedStrokes,0,sizeof(G.selectedStrokes));memset(G.selectedRefs,0,sizeof(G.selectedRefs));memset(G.lassoPts,0,sizeof(G.lassoPts));
    G.measureN=0;G.measureLoadMapCount=0;G.measuring=0;G.selectedMeasure=-1;G.frameN=0;G.nextFrameId=1;G.selectedFrame=-1;G.noteN=0;G.nextNoteId=1;G.selectedNote=-1;G.shapeN=0;G.nextShapeId=1;G.selectedShape=-1;G.bookmarkN=0;G.nextBookmarkId=1;G.selectedBookmark=-1;G.groupN=0;G.nextGroupId=1;G.selectedGroup=-1;
    G.selectedStrokeN=G.selectedRefN=0;G.selectionType=SEL_NONE;G.selectionDragging=G.selectionGesture=G.selectionMoreOpen=0;G.lassoN=G.lassoActive=0;G.snapGuideX=G.snapGuideY=0;
    G.photoMode=0;G.selectedImage=-1;G.imageDragging=G.imageGesture=G.imageCancelValid=0;memset(&G.imageCancelStart,0,sizeof(G.imageCancelStart));G.fingerId0=G.fingerId1=-1;G.frameCreating=G.frameDragging=G.frameMoveMode=G.frameMoveDragging=G.frameResizeDragging=0;G.framePage=0;G.shapeDrawing=0;
    G.addPanel=G.layersPanel=G.searchPanel=G.bookmarksPanel=G.framePanel=0;G.presentationMode=0;G.pressedUi=0;G.searchQuery[0]=0;G.searchLen=G.searchVisibleN=G.ocrSearchHitN=0;G.ocrHighlightAlpha=0;
    G.visInk=G.visMarker=G.visPhotos=G.visCAD=G.visFrames=G.visNotes=G.visShapes=1;apply_workspace_locks(0);G.lastWriteValid=0;
    G.saveDirty=G.metaDirty=0;G.inkSavePending=G.workspaceSavePending=G.ocrPending=0;G.minimapDirty=1;G.viewInitialized=0;G.sceneRevision++;
}
static int save_all_document(void){ocr_pending_flush();int a=save_canvas(),b=save_meta(),c=save_images(),d=save_frames(),e=save_workspace();ocr_checkpoint();return a&&b&&c&&d&&e;}
static void clear_current_project(void){
    clear_document_memory();G.tool=MODE_PEN;G.zenMode=0;G.railOpen=0;G.projectsPanel=0;reset_view();G.viewInitialized=1;
    G.saveDirty=G.metaDirty=G.imagesDirty=G.imagePixelsDirty=1;G.frameRevision++;G.sceneRevision++;G.minimapDirty=1;
#ifdef VAST_OCR
    if(G.ocr){ocr_manager_rebuild(G.ocr);ocr_begin_initial_scan();}
#endif
    if(save_all_document())set_toast(13,0);else set_toast(8,0);
}
static void switch_project(int idx){
    if(idx==G.projectIndex){G.projectsPanel=0;return;}if(!project_recover_incomplete_duplicate(idx)||!save_all_document()){set_toast(8,0);return;}ocr_close_current_project(0);clear_document_memory();set_project_paths(idx);load_canvas();load_meta();load_images();load_frames();load_workspace();ocr_open_current_project();if(!G.viewInitialized)reset_view();G.projectsPanel=0;G.minimapDirty=1;save_project_prefs();
}
static int duplicate_current_project(void){
    if(!save_all_document())return -2;int dst=-1;for(int i=0;i<4;i++)if(i!=G.projectIndex&&!project_slot_has_data(i)){dst=i;break;}if(dst<0)return -1;
    char src[6][512],out[6][512],stage[6][544],marker[544],markerTmp[576];memset(stage,0,sizeof(stage));int present[6]={0,0,0,0,0,0},copied=0,marked=0;project_duplicate_marker_path(dst,marker,sizeof(marker));
    for(int k=0;k<6;k++){project_path_for(G.projectIndex,k,src[k],512);project_path_for(dst,k,out[k],512);int n=snprintf(stage[k],sizeof(stage[k]),"%s.dup",out[k]);if(n<=0||n>=(int)sizeof(stage[k]))goto fail;remove(stage[k]);if(file_exists_local(src[k])){present[k]=1;if(!copy_file_local(src[k],stage[k]))goto fail;copied=1;}}
    if(!copied)goto fail;
    {void*mf=atomic_write_begin(marker,markerTmp,sizeof(markerTmp));uint8_t one=1;int markerCommit=mf?atomic_write_finish(mf,markerTmp,marker,fwrite(&one,1,1,mf)==1):ATOMIC_WRITE_FAILED;if(markerCommit==ATOMIC_WRITE_FAILED)goto fail;marked=1;if(markerCommit!=ATOMIC_WRITE_DURABLE)goto fail;}
    for(int k=0;k<6;k++)if(present[k]&&rename(stage[k],out[k])!=0)goto fail;
    /* First make every destination rename durable while recovery is still
       armed, then durably retire the marker. */
    if(!fsync_parent_directory(marker)||remove(marker)!=0)goto fail;marked=0;
    /* From here the complete destination set is already durable and recovery
       is disarmed in the live namespace.  If persisting the marker removal
       fails, report failure without risking a markerless partial cleanup. */
    if(!fsync_parent_directory(marker))return -2;
    (void)ocr_clone_project_cache(dst);snprintf(G.projectNames[dst],32,"%.24s copy",G.projectNames[G.projectIndex]);save_project_prefs();switch_project(dst);return dst;
fail:
    {int clean=1;for(int k=0;k<6;k++){if(stage[k][0]&&!remove_file_if_present(stage[k]))clean=0;if(marked&&out[k][0]&&!remove_file_if_present(out[k]))clean=0;}if(!fsync_parent_directory(marker))clean=0;if(marked&&clean){if(!remove_file_if_present(marker))clean=0;else if(!fsync_parent_directory(marker))clean=0;}(void)clean;}return -2;
}

typedef struct { char magic[4]; int32_t count; uint32_t colors[6]; } InkPrefs;
static void recent_colors_defaults(void){static const uint32_t d[6]={0xf4f6f8,0x111111,0x6aa9ff,0xff6475,0x5fe39a,0xffc55d};G.recentColorN=6;for(int i=0;i<6;i++)G.recentColors[i]=d[i];}
static void save_ink_prefs(void){if(!G.inkPrefsPath[0])return;char tmp[544];void*f=atomic_write_begin(G.inkPrefsPath,tmp,sizeof(tmp));if(!f)return;InkPrefs q;memset(&q,0,sizeof(q));q.magic[0]='V';q.magic[1]='I';q.magic[2]='P';q.magic[3]='1';q.count=maxi(0,mini(6,G.recentColorN));for(int i=0;i<q.count;i++)q.colors[i]=G.recentColors[i]&0xffffffu;int ok=fwrite(&q,sizeof(q),1,f)==1;(void)atomic_write_finish(f,tmp,G.inkPrefsPath,ok);}
static void load_ink_prefs(void){recent_colors_defaults();if(!G.inkPrefsPath[0])return;void*f=fopen(G.inkPrefsPath,"rb");if(!f)return;InkPrefs q;if(fread(&q,sizeof(q),1,f)==1&&q.magic[0]=='V'&&q.magic[1]=='I'&&q.magic[2]=='P'&&q.magic[3]=='1'&&q.count>0&&q.count<=6){G.recentColorN=q.count;for(int i=0;i<G.recentColorN;i++)G.recentColors[i]=q.colors[i]&0xffffffu;}fclose(f);}
static int recent_color_contains(uint32_t c){c&=0xffffffu;for(int i=0;i<G.recentColorN;i++)if((G.recentColors[i]&0xffffffu)==c)return 1;return 0;}
static void recent_color_add_from_picker(uint32_t c){c&=0xffffffu;if(recent_color_contains(c))return;int n=mini(5,G.recentColorN);for(int i=n;i>0;i--)G.recentColors[i]=G.recentColors[i-1];G.recentColors[0]=c;G.recentColorN=mini(6,G.recentColorN+1);save_ink_prefs();}
// ---------- Software renderer ----------
typedef struct Surf { uint8_t* bits; int w,h,stride,fmt; } Surf;
static uint32_t pack8888(uint32_t rgb){uint32_t r=(rgb>>16)&255,g=(rgb>>8)&255,b=rgb&255;return r|(g<<8)|(b<<16)|0xff000000u;} static uint16_t pack565(uint32_t rgb){uint32_t r=(rgb>>16)&255,g=(rgb>>8)&255,b=rgb&255;return (uint16_t)(((r>>3)<<11)|((g>>2)<<5)|(b>>3));}
static void putpx(Surf*s,int x,int y,uint32_t rgb){if((unsigned)x>=(unsigned)s->w||(unsigned)y>=(unsigned)s->h)return;if(s->fmt==FORMAT_RGB565)((uint16_t*)s->bits)[y*s->stride+x]=pack565(rgb);else((uint32_t*)s->bits)[y*s->stride+x]=pack8888(rgb);}
static void fill(Surf*s,uint32_t rgb){if(s->fmt==FORMAT_RGB565){uint16_t c=pack565(rgb);for(int y=0;y<s->h;y++){uint16_t*p=(uint16_t*)s->bits+y*s->stride;for(int x=0;x<s->w;x++)p[x]=c;}}else{uint32_t c=pack8888(rgb);for(int y=0;y<s->h;y++){uint32_t*p=(uint32_t*)s->bits+y*s->stride;for(int x=0;x<s->w;x++)p[x]=c;}}}
static void clear_transparent(Surf*s){if(!s||!s->bits)return;memset(s->bits,0,(size_t)s->stride*s->h*4);}
static void rect(Surf*s,int x0,int y0,int x1,int y1,uint32_t c){x0=maxi(x0,0);y0=maxi(y0,0);x1=mini(x1,s->w);y1=mini(y1,s->h);for(int y=y0;y<y1;y++)for(int x=x0;x<x1;x++)putpx(s,x,y,c);} static void hline(Surf*s,int y,int x0,int x1,uint32_t c){if(y<0||y>=s->h)return;if(x0>x1){int t=x0;x0=x1;x1=t;}x0=maxi(0,x0);x1=mini(s->w-1,x1);for(int x=x0;x<=x1;x++)putpx(s,x,y,c);} static void vline(Surf*s,int x,int y0,int y1,uint32_t c){if(x<0||x>=s->w)return;if(y0>y1){int t=y0;y0=y1;y1=t;}y0=maxi(0,y0);y1=mini(s->h-1,y1);for(int y=y0;y<=y1;y++)putpx(s,x,y,c);}
static void circle(Surf*s,float cx,float cy,float rr,uint32_t c){int r=(int)(rr+0.5f);if(r<1)r=1;int x0=(int)(cx-r),x1=(int)(cx+r),y0=(int)(cy-r),y1=(int)(cy+r);float r2=rr*rr;for(int y=y0;y<=y1;y++){float dy=(float)y-cy;for(int x=x0;x<=x1;x++){float dx=(float)x-cx;if(dx*dx+dy*dy<=r2)putpx(s,x,y,c);}}}
static void round_rect(Surf*s,int x0,int y0,int x1,int y1,int r,uint32_t c){if(r<1){rect(s,x0,y0,x1,y1,c);return;}rect(s,x0+r,y0,x1-r,y1,c);rect(s,x0,y0+r,x1,y1-r,c);circle(s,(float)(x0+r),(float)(y0+r),(float)r,c);circle(s,(float)(x1-r-1),(float)(y0+r),(float)r,c);circle(s,(float)(x0+r),(float)(y1-r-1),(float)r,c);circle(s,(float)(x1-r-1),(float)(y1-r-1),(float)r,c);}
static void aa_capsule(Surf*s,float x0,float y0,float r0,float x1,float y1,float r1,uint32_t c,int alpha);
static void segment(Surf*s,float x0,float y0,float x1,float y1,float r,uint32_t c){aa_capsule(s,x0,y0,r,x1,y1,r,c,255);}
static void thick_segment(Surf*s,float x0,float y0,float r0,float x1,float y1,float r1,uint32_t c){aa_capsule(s,x0,y0,r0,x1,y1,r1,c,255);}

// Smooth baked sans-serif UI font. The original typeface is rasterized into this atlas at build time;
// no font file is shipped in the APK.
static uint32_t getrgb(Surf*s,int x,int y);
static void blendpx(Surf*s,int x,int y,uint32_t rgb,int a);
static void font_init(void){
    if(FONT_ATLAS)return;
    FONT_ATLAS=(uint8_t*)malloc((size_t)FORMA_FONT_ATLAS_W*FORMA_FONT_ATLAS_H);
    if(!FONT_ATLAS)return;
    size_t out=0;
    for(size_t i=0;i+1<FORMA_FONT_RLE_LEN&&out<(size_t)FORMA_FONT_ATLAS_W*FORMA_FONT_ATLAS_H;i+=2){
        int n=forma_font_rle[i],v=forma_font_rle[i+1];
        for(int k=0;k<n&&out<(size_t)FORMA_FONT_ATLAS_W*FORMA_FONT_ATLAS_H;k++)FONT_ATLAS[out++]=(uint8_t)v;
    }
}
static int font_line_h(int px){return maxi(1,(px*FORMA_FONT_CELL_H+FORMA_FONT_BASE_SIZE/2)/FORMA_FONT_BASE_SIZE);}
static int font_adv(char ch,int px){unsigned c=(unsigned char)ch;if(c<32||c>126)c='?';int a=forma_font_adv64[c-32];return maxi(1,(a*px+FORMA_FONT_BASE_SIZE*32)/(FORMA_FONT_BASE_SIZE*64));}
static int text_w(const char*str,int px){int w=0;for(;*str;str++)w+=font_adv(*str,px);return w;}
static void text(Surf*s,int x,int y,const char*str,int px,uint32_t c){
    font_init(); if(!FONT_ATLAS||px<1)return;
    float sc=(float)px/(float)FORMA_FONT_BASE_SIZE;
    int dh=font_line_h(px),dw=maxi(1,(int)(FORMA_FONT_CELL_W*sc+0.5f));
    for(;*str;str++){
        unsigned cc=(unsigned char)*str;if(cc<32||cc>126)cc='?';int gi=(int)cc-32,gx=(gi%16)*FORMA_FONT_CELL_W,gy=(gi/16)*FORMA_FONT_CELL_H;
        if(cc!=' '){
            for(int dy=0;dy<dh;dy++){
                float syf=(dy+0.5f)/sc-0.5f;int sy=(int)floorf(syf);float fy=syf-sy;if(sy<0){sy=0;fy=0;}if(sy>=FORMA_FONT_CELL_H-1){sy=FORMA_FONT_CELL_H-2;fy=1;}
                for(int dx=0;dx<dw;dx++){
                    float sxf=(dx+0.5f)/sc-0.5f;int sx=(int)floorf(sxf);float fx=sxf-sx;if(sx<0){sx=0;fx=0;}if(sx>=FORMA_FONT_CELL_W-1){sx=FORMA_FONT_CELL_W-2;fx=1;}
                    int i00=(gy+sy)*FORMA_FONT_ATLAS_W+gx+sx,i10=i00+1,i01=i00+FORMA_FONT_ATLAS_W,i11=i01+1;
                    float a0=FONT_ATLAS[i00]*(1-fx)+FONT_ATLAS[i10]*fx,a1=FONT_ATLAS[i01]*(1-fx)+FONT_ATLAS[i11]*fx;
                    int a=(int)(a0*(1-fy)+a1*fy+0.5f);if(a>2)blendpx(s,x+dx,y+dy,c,a);
                }
            }
        }
        x+=font_adv((char)cc,px);
    }
}

// ---------- Visual system ----------
#define C_BG (G.theme.bg)
#define C_GRID (G.theme.grid)
#define C_GRID_MAJOR (G.theme.gridMajor)
#define C_AXIS (G.theme.axis)
#define C_GLASS (G.theme.glass)
#define C_GLASS2 (G.theme.glass2)
#define C_GLASS3 (G.theme.glass3)
#define C_BORDER (G.theme.border)
#define C_TEXT (G.theme.text)
#define C_MUTED (G.theme.muted)
#define C_ACCENT (G.theme.accent)
#define C_CYAN (G.theme.cyan)
#define C_VIOLET (G.theme.violet)
#define C_WARN (G.theme.warn)
#define C_DANGER (G.theme.danger)
#define C_GREEN (G.theme.green)
#define C_BLACK (G.theme.black)

static int us(int v){float q=(G.uiScale>0.01f?G.uiScale:1.0f);return maxi(1,(int)(v*q+0.5f));}
static int ts(int v){static const int base[5]={0,18,22,28,36};int b=(v>=1&&v<=4)?base[v]:17;float q=(G.uiScale>0.01f?G.uiScale:1.0f);return maxi(10,(int)(b*q+0.5f));}
static int ui_s(void){int h=G.screenH>0?G.screenH:1600;int base=maxi(78,mini(104,h/18));return maxi(58,mini(150,(int)(base*(G.uiScale>0.01f?G.uiScale:1.0f)+0.5f)));}
static int margin_ui(void){return ui_s()/4;}
static int hit_box(float x,float y,int x0,int y0,int x1,int y1){return x>=x0&&x<x1&&y>=y0&&y<y1;}
static uint32_t getrgb(Surf*s,int x,int y){if((unsigned)x>=(unsigned)s->w||(unsigned)y>=(unsigned)s->h)return 0;if(s->fmt==FORMAT_RGB565){uint16_t v=((uint16_t*)s->bits)[y*s->stride+x];uint32_t r=((v>>11)&31)*255/31,g=((v>>5)&63)*255/63,b=(v&31)*255/31;return (r<<16)|(g<<8)|b;}uint32_t v=((uint32_t*)s->bits)[y*s->stride+x];return ((v&255)<<16)|(v&0x0000ff00)|((v>>16)&255);}
static void blendpx(Surf*s,int x,int y,uint32_t rgb,int a){if(a<=0||(unsigned)x>=(unsigned)s->w||(unsigned)y>=(unsigned)s->h)return;if(s->fmt==FORMAT_RGBA_LAYER){uint32_t*dptr=((uint32_t*)s->bits)+y*s->stride+x;uint32_t dv=*dptr;int da=(dv>>24)&255,sr=(rgb>>16)&255,sg=(rgb>>8)&255,sb=rgb&255,dr=dv&255,dg=(dv>>8)&255,db=(dv>>16)&255;int oa=a+(da*(255-a)+127)/255;if(oa<=0){*dptr=0;return;}int keep=(da*(255-a)+127)/255;int rr=(sr*a+dr*keep)/oa,gg=(sg*a+dg*keep)/oa,bb=(sb*a+db*keep)/oa;*dptr=(uint32_t)rr|((uint32_t)gg<<8)|((uint32_t)bb<<16)|((uint32_t)oa<<24);return;}if(a>=255){putpx(s,x,y,rgb);return;}uint32_t d=getrgb(s,x,y);int sr=(rgb>>16)&255,sg=(rgb>>8)&255,sb=rgb&255,dr=(d>>16)&255,dg=(d>>8)&255,db=d&255;uint32_t o=(uint32_t)(((sr*a+dr*(255-a))/255)<<16)|((uint32_t)((sg*a+dg*(255-a))/255)<<8)|(uint32_t)((sb*a+db*(255-a))/255);putpx(s,x,y,o);}
static void alpha_rect(Surf*s,int x0,int y0,int x1,int y1,uint32_t c,int a){x0=maxi(x0,0);y0=maxi(y0,0);x1=mini(x1,s->w);y1=mini(y1,s->h);for(int y=y0;y<y1;y++)for(int x=x0;x<x1;x++)blendpx(s,x,y,c,a);}
static void alpha_circle(Surf*s,float cx,float cy,float rr,uint32_t c,int a){int r=(int)(rr+0.5f);if(r<1)r=1;int x0=(int)(cx-r),x1=(int)(cx+r),y0=(int)(cy-r),y1=(int)(cy+r);float r2=rr*rr;for(int y=y0;y<=y1;y++){float dy=(float)y-cy;for(int x=x0;x<=x1;x++){float dx=(float)x-cx;if(dx*dx+dy*dy<=r2)blendpx(s,x,y,c,a);}}}
static void alpha_thick_segment(Surf*s,float x0,float y0,float r0,float x1,float y1,float r1,uint32_t c,int a){aa_capsule(s,x0,y0,r0,x1,y1,r1,c,a);}
static void alpha_round_rect(Surf*s,int x0,int y0,int x1,int y1,int r,uint32_t c,int a){x0=maxi(x0,0);y0=maxi(y0,0);x1=mini(x1,s->w);y1=mini(y1,s->h);if(x1<=x0||y1<=y0||a<=0)return;int w=x1-x0,h=y1-y0,lim=w<h?w:h;if(r<1){alpha_rect(s,x0,y0,x1,y1,c,a);return;}if(r>lim/2)r=lim/2;for(int y=y0;y<y1;y++){int inset=0;if(y<y0+r){float dy=(float)(y0+r)-(float)y;float dx=sqrtf(fmax2(0.0f,(float)r*r-dy*dy));inset=maxi(0,(int)(((float)r-dx)+0.999f));}else if(y>y1-r-1){float dy=(float)y-(float)(y1-r-1);float dx=sqrtf(fmax2(0.0f,(float)r*r-dy*dy));inset=maxi(0,(int)(((float)r-dx)+0.999f));}for(int x=x0+inset;x<x1-inset;x++)blendpx(s,x,y,c,a);}}
static void aa_circle(Surf*s,float cx,float cy,float r,uint32_t c,int alpha){
    /* 3.2.5: true signed-distance edge coverage.  The old ~1 px ramp became
       visually too hard once the centreline was made much smoother; a 1.6 px
       transition survives the GPU cache resample and still looks crisp on a
       high-DPI tablet. */
    if(r<0.28f)r=0.28f;const float feather=.82f;float ro=r+feather;
    int x0=maxi(0,(int)floorf(cx-ro)),x1=mini(s->w-1,(int)floorf(cx+ro));
    int y0=maxi(0,(int)floorf(cy-ro)),y1=mini(s->h-1,(int)floorf(cy+ro));
    for(int y=y0;y<=y1;y++){float py=y+.5f,dy=py-cy;for(int x=x0;x<=x1;x++){float px=x+.5f,dx=px-cx;float sd=r-sqrtf(dx*dx+dy*dy);if(sd<=-feather)continue;float cov=smoothstepf(-feather,feather,sd);int a=(int)(alpha*cov+.5f);if(a>=255)putpx(s,x,y,c);else if(a>0)blendpx(s,x,y,c,a);}}
}
/* Fast vector-style tapered segment. The body is a convex quad with analytic
 * one-pixel edge coverage; round caps are drawn separately. This avoids the
 * old circle-stamping path which repeatedly touched the same pixels. */
static void aa_taper_quad(Surf*s,float x0,float y0,float r0,float x1,float y1,float r1,uint32_t c,int alpha){
    float dx=x1-x0,dy=y1-y0,d2=dx*dx+dy*dy;if(d2<0.0001f)return;
    float invd=1.0f/sqrtf(d2),nx=-dy*invd,ny=dx*invd;
    float vx[4]={x0+nx*r0,x1+nx*r1,x1-nx*r1,x0-nx*r0};
    float vy[4]={y0+ny*r0,y1+ny*r1,y1-ny*r1,y0-ny*r0};
    float area=(vx[1]-vx[0])*(vy[2]-vy[0])-(vy[1]-vy[0])*(vx[2]-vx[0]);
    float sg=area>=0.0f?1.0f:-1.0f;
    float ex[4],ey[4],invlen[4];
    float minx=vx[0],maxx=vx[0],miny=vy[0],maxy=vy[0];
    for(int i=0;i<4;i++){
        int j=(i+1)&3;ex[i]=vx[j]-vx[i];ey[i]=vy[j]-vy[i];
        float l2=ex[i]*ex[i]+ey[i]*ey[i];invlen[i]=l2>0.0001f?1.0f/sqrtf(l2):1.0f;
        minx=fmin2(minx,vx[i]);maxx=fmax2(maxx,vx[i]);miny=fmin2(miny,vy[i]);maxy=fmax2(maxy,vy[i]);
    }
    int ix0=maxi(0,(int)floorf(minx-1.2f)),ix1=mini(s->w-1,(int)floorf(maxx+1.2f));
    int iy0=maxi(0,(int)floorf(miny-1.2f)),iy1=mini(s->h-1,(int)floorf(maxy+1.2f));
    for(int y=iy0;y<=iy1;y++){
        float py=y+0.5f,px=ix0+0.5f;
        float e0=sg*(ex[0]*(py-vy[0])-ey[0]*(px-vx[0]));
        float e1=sg*(ex[1]*(py-vy[1])-ey[1]*(px-vx[1]));
        float e2=sg*(ex[2]*(py-vy[2])-ey[2]*(px-vx[2]));
        float e3=sg*(ex[3]*(py-vy[3])-ey[3]*(px-vx[3]));
        float de0=-sg*ey[0],de1=-sg*ey[1],de2=-sg*ey[2],de3=-sg*ey[3];
        for(int x=ix0;x<=ix1;x++,e0+=de0,e1+=de1,e2+=de2,e3+=de3){
            float md=e0*invlen[0];float q=e1*invlen[1];if(q<md)md=q;q=e2*invlen[2];if(q<md)md=q;q=e3*invlen[3];if(q<md)md=q;
            /* Signed distance to the nearest quad edge.  Use the same soft
               coverage profile as round caps so diagonals do not staircase. */
            const float feather=.82f;if(md<=-feather)continue;float cov=smoothstepf(-feather,feather,md);int a=(int)(alpha*cov+0.5f);if(a>=255)putpx(s,x,y,c);else if(a>0)blendpx(s,x,y,c,a);
        }
    }
}
static void aa_capsule(Surf*s,float x0,float y0,float r0,float x1,float y1,float r1,uint32_t c,int alpha){
    float dx=x1-x0,dy=y1-y0,d2=dx*dx+dy*dy;if(d2<0.01f){aa_circle(s,x0,y0,fmax2(r0,r1),c,alpha);return;}
    aa_taper_quad(s,x0,y0,r0,x1,y1,r1,c,alpha);aa_circle(s,x0,y0,r0,c,alpha);aa_circle(s,x1,y1,r1,c,alpha);
}

/* Coverage mask used for translucent marker strokes. It composites a stroke only
 * once, so adjacent vector segments do not create dark seams at every sample. */
static int ensure_stroke_mask(int w,int h){
    if(w<=0||h<=0)return 0;if(STROKE_MASK&&STROKE_MASK_W==w&&STROKE_MASK_H==h)return 1;
    if(STROKE_MASK)free(STROKE_MASK);STROKE_MASK=(uint8_t*)malloc((size_t)w*h);if(!STROKE_MASK){STROKE_MASK_W=STROKE_MASK_H=0;return 0;}
    STROKE_MASK_W=w;STROKE_MASK_H=h;memset(STROKE_MASK,0,(size_t)w*h);return 1;
}
static void mask_maxpx(int x,int y,int a){if((unsigned)x>=(unsigned)STROKE_MASK_W||(unsigned)y>=(unsigned)STROKE_MASK_H||a<=0)return;uint8_t*p=&STROKE_MASK[y*STROKE_MASK_W+x];if(a>*p)*p=(uint8_t)(a>255?255:a);}
static void mask_circle(float cx,float cy,float r){
    if(r<0.28f)r=0.28f;float ro=r+0.55f,ri=fmax2(0.0f,r-0.50f),ro2=ro*ro,ri2=ri*ri;
    int y0=maxi(0,(int)floorf(cy-ro)),y1=mini(STROKE_MASK_H-1,(int)floorf(cy+ro));
    for(int y=y0;y<=y1;y++){float py=y+0.5f,dy=py-cy,d2y=dy*dy;if(d2y>=ro2)continue;float ho=sqrtf(ro2-d2y),li=cx-ho,rr=cx+ho;int xl=maxi(0,(int)floorf(li)),xr=mini(STROKE_MASK_W-1,(int)floorf(rr));int fi0=xl,fi1=xr;
        if(d2y<ri2){float hi=sqrtf(ri2-d2y);fi0=maxi(xl,(int)(cx-hi+0.999f));fi1=mini(xr,(int)floorf(cx+hi));}
        for(int x=fi0;x<=fi1;x++)mask_maxpx(x,y,255);
        for(int x=xl;x<fi0;x++){float px=x+0.5f,dx=px-cx,d2=dx*dx+d2y,cov=clampf(0.5f+(r*r-d2)/fmax2(.35f,2*r),0,1);if(cov>0)mask_maxpx(x,y,(int)(255*cov+0.5f));}
        for(int x=fi1+1;x<=xr;x++){float px=x+0.5f,dx=px-cx,d2=dx*dx+d2y,cov=clampf(0.5f+(r*r-d2)/fmax2(.35f,2*r),0,1);if(cov>0)mask_maxpx(x,y,(int)(255*cov+0.5f));}
    }
}
static void mask_taper_quad(float x0,float y0,float r0,float x1,float y1,float r1){
    float dx=x1-x0,dy=y1-y0,d2=dx*dx+dy*dy;if(d2<0.0001f){mask_circle(x0,y0,fmax2(r0,r1));return;}float invd=1.0f/sqrtf(d2),ux=dx*invd,uy=dy*invd,nx=-uy,ny=ux;
    /* Slightly overlap neighboring quads. The mask uses max coverage, so this fills
       subpixel cracks without darkening joints. */
    x0-=ux*.85f;y0-=uy*.85f;x1+=ux*.85f;y1+=uy*.85f;
    float vx[4]={x0+nx*r0,x1+nx*r1,x1-nx*r1,x0-nx*r0},vy[4]={y0+ny*r0,y1+ny*r1,y1-ny*r1,y0-ny*r0};float area=(vx[1]-vx[0])*(vy[2]-vy[0])-(vy[1]-vy[0])*(vx[2]-vx[0]),sg=area>=0?1.0f:-1.0f;
    float ex[4],ey[4],invlen[4],minx=vx[0],maxx=vx[0],miny=vy[0],maxy=vy[0];for(int i=0;i<4;i++){int j=(i+1)&3;ex[i]=vx[j]-vx[i];ey[i]=vy[j]-vy[i];float l2=ex[i]*ex[i]+ey[i]*ey[i];invlen[i]=l2>.0001f?1.0f/sqrtf(l2):1.0f;minx=fmin2(minx,vx[i]);maxx=fmax2(maxx,vx[i]);miny=fmin2(miny,vy[i]);maxy=fmax2(maxy,vy[i]);}
    int ix0=maxi(0,(int)floorf(minx-1)),ix1=mini(STROKE_MASK_W-1,(int)floorf(maxx+1)),iy0=maxi(0,(int)floorf(miny-1)),iy1=mini(STROKE_MASK_H-1,(int)floorf(maxy+1));
    for(int y=iy0;y<=iy1;y++){float py=y+.5f,px=ix0+.5f,e0=sg*(ex[0]*(py-vy[0])-ey[0]*(px-vx[0])),e1=sg*(ex[1]*(py-vy[1])-ey[1]*(px-vx[1])),e2=sg*(ex[2]*(py-vy[2])-ey[2]*(px-vx[2])),e3=sg*(ex[3]*(py-vy[3])-ey[3]*(px-vx[3]));float d0=-sg*ey[0],d1=-sg*ey[1],d2x=-sg*ey[2],d3=-sg*ey[3];for(int x=ix0;x<=ix1;x++,e0+=d0,e1+=d1,e2+=d2x,e3+=d3){if(e1*invlen[1]<-.02f||e3*invlen[3]<-.02f)continue;float md=e0*invlen[0],q=e2*invlen[2];if(q<md)md=q;if(md<=-.55f)continue;mask_maxpx(x,y,(int)(255*clampf(md+.55f,0,1)+.5f));}}
}
static void mask_composite_clear(Surf*s,int x0,int y0,int x1,int y1,uint32_t c,int alpha){x0=maxi(0,x0);y0=maxi(0,y0);x1=mini(s->w,x1);y1=mini(s->h,y1);for(int y=y0;y<y1;y++){uint8_t*m=STROKE_MASK+y*STROKE_MASK_W+x0;for(int x=x0;x<x1;x++,m++){int cov=*m;if(cov){blendpx(s,x,y,c,(alpha*cov+127)/255);*m=0;}}}}

static void aa_vline(Surf*s,float xf,int y0,int y1,uint32_t c,int alpha){int x=(int)floorf(xf);float f=xf-x;int a0=(int)(alpha*(1-f)+0.5f),a1=(int)(alpha*f+0.5f);y0=maxi(0,y0);y1=mini(s->h-1,y1);for(int y=y0;y<=y1;y++){if(a0)blendpx(s,x,y,c,a0);if(a1)blendpx(s,x+1,y,c,a1);}}
static void aa_hline(Surf*s,float yf,int x0,int x1,uint32_t c,int alpha){int y=(int)floorf(yf);float f=yf-y;int a0=(int)(alpha*(1-f)+0.5f),a1=(int)(alpha*f+0.5f);x0=maxi(0,x0);x1=mini(s->w-1,x1);for(int x=x0;x<=x1;x++){if(a0)blendpx(s,x,y,c,a0);if(a1)blendpx(s,x,y+1,c,a1);}}

static void glass(Surf*s,int x0,int y0,int x1,int y1,int r){r=mini(r,us(22));int sh=us(2),in=maxi(1,us(1));if(s->fmt==FORMAT_RGBA_LAYER){alpha_round_rect(s,x0+sh,y0+sh,x1+sh,y1+sh,r,C_BLACK,38);alpha_round_rect(s,x0,y0,x1,y1,r,C_BORDER,112);alpha_round_rect(s,x0+in,y0+in,x1-in,y1-in,maxi(1,r-in),C_GLASS,222);}else{round_rect(s,x0+sh,y0+sh,x1+sh,y1+sh,r,C_BLACK);round_rect(s,x0,y0,x1,y1,r,C_BORDER);round_rect(s,x0+in,y0+in,x1-in,y1-in,maxi(1,r-in),C_GLASS);}}
static void button(Surf*s,int x0,int y0,int x1,int y1,const char*label,int active,uint32_t accent){int r=mini(us(16),maxi(us(10),(y1-y0)/3)),in=maxi(1,us(1));uint32_t edge=active?mix_color(accent,C_BORDER,18):C_BORDER,bg=C_GLASS2;if(s->fmt==FORMAT_RGBA_LAYER){alpha_round_rect(s,x0,y0,x1,y1,r,edge,active?164:94);alpha_round_rect(s,x0+in,y0+in,x1-in,y1-in,maxi(2,r-in),bg,active?196:166);}else{round_rect(s,x0,y0,x1,y1,r,edge);round_rect(s,x0+in,y0+in,x1-in,y1-in,maxi(2,r-in),bg);}int fs=maxi(ts(1),(int)((y1-y0)*0.30f));int tw=text_w(label,fs),th=font_line_h(fs);text(s,(x0+x1-tw)/2,(y0+y1-th)/2,label,fs,active?accent:C_TEXT);}
static void chip(Surf*s,int x0,int y0,const char*label,uint32_t c){int fs=ts(1),p=us(10),hh=us(30),ww=text_w(label,fs)+p*2,r=us(12),in=maxi(1,us(1));if(s->fmt==FORMAT_RGBA_LAYER){alpha_round_rect(s,x0,y0,x0+ww,y0+hh,r,C_BORDER,78);alpha_round_rect(s,x0+in,y0+in,x0+ww-in,y0+hh-in,maxi(2,r-in),C_GLASS2,142);}else{round_rect(s,x0,y0,x0+ww,y0+hh,r,C_BORDER);round_rect(s,x0+in,y0+in,x0+ww-in,y0+hh-in,maxi(2,r-in),C_GLASS2);}text(s,x0+p,y0+(hh-font_line_h(fs))/2,label,fs,c);}
static void ring(Surf*s,float cx,float cy,float r,float t,uint32_t c){circle(s,cx,cy,r,c);circle(s,cx,cy,fmax2(0,r-t),C_GLASS2);}
static void line_icon(Surf*s,int kind,float cx,float cy,float k,uint32_t c){float r=12*k;if(kind==0){segment(s,cx-r*.65f,cy+r*.65f,cx+r*.55f,cy-r*.55f,2.2f*k,c);segment(s,cx+r*.55f,cy-r*.55f,cx+r*.82f,cy-r*.82f,1.5f*k,c);segment(s,cx-r*.72f,cy+r*.72f,cx-r*.30f,cy+r*.60f,1.4f*k,c);}else if(kind==1){segment(s,cx-r*.65f,cy+r*.42f,cx+r*.42f,cy-r*.65f,6.0f*k,c);segment(s,cx-r*.52f,cy+r*.56f,cx-r*.22f,cy+r*.78f,4.5f*k,C_DANGER);}else if(kind==2){segment(s,cx-r*.76f,cy+r*.60f,cx+r*.68f,cy-r*.55f,2.2f*k,c);for(int i=-2;i<=2;i++){float t0=(float)(i+2)/4.0f;float x=cx-r*.55f+t0*r*.98f,y=cy+r*.43f-t0*r*.78f;segment(s,x,y,x+4*k,y+5*k,1.0f*k,c);}}else if(kind==3||kind==4){float d=kind==3?-1.0f:1.0f;segment(s,cx-d*r*.65f,cy-r*.1f,cx+d*r*.25f,cy-r*.1f,1.9f*k,c);segment(s,cx-d*r*.65f,cy-r*.1f,cx-d*r*.28f,cy-r*.46f,1.9f*k,c);segment(s,cx-d*r*.65f,cy-r*.1f,cx-d*r*.28f,cy+r*.26f,1.9f*k,c);segment(s,cx+d*r*.25f,cy-r*.1f,cx+d*r*.58f,cy+r*.32f,1.9f*k,c);}else if(kind==5){float q=r*.58f;segment(s,cx-q,cy-q,cx-q*.45f,cy-q,1.7f*k,c);segment(s,cx-q,cy-q,cx-q,cy-q*.45f,1.7f*k,c);segment(s,cx+q,cy-q,cx+q*.45f,cy-q,1.7f*k,c);segment(s,cx+q,cy-q,cx+q,cy-q*.45f,1.7f*k,c);segment(s,cx-q,cy+q,cx-q*.45f,cy+q,1.7f*k,c);segment(s,cx-q,cy+q,cx-q,cy+q*.45f,1.7f*k,c);segment(s,cx+q,cy+q,cx+q*.45f,cy+q,1.7f*k,c);segment(s,cx+q,cy+q,cx+q,cy+q*.45f,1.7f*k,c);}else if(kind==6){circle(s,cx-r*.45f,cy+r*.28f,2.6f*k,c);circle(s,cx,cy-r*.08f,4.2f*k,c);circle(s,cx+r*.48f,cy-r*.42f,6.0f*k,c);}else if(kind==8){float q=r*.62f;round_rect(s,(int)(cx-q),(int)(cy-q*.72f),(int)(cx+q),(int)(cy+q*.72f),maxi(2,(int)(4*k)),C_GLASS3);segment(s,cx-q*.75f,cy+q*.35f,cx-q*.20f,cy-q*.12f,1.8f*k,c);segment(s,cx-q*.20f,cy-q*.12f,cx+q*.12f,cy+q*.16f,1.8f*k,c);segment(s,cx+q*.12f,cy+q*.16f,cx+q*.72f,cy-q*.34f,1.8f*k,c);circle(s,cx+q*.42f,cy-q*.34f,3.0f*k,c);}else if(kind==9){for(int i=0;i<8;i++){float a=(float)i*PI/4.0f;float x0=cx+(r*.45f)*((i==0||i==4)?(i==0?1:-1):0),y0=cy; (void)x0;(void)y0;}circle(s,cx,cy,r*.52f,c);circle(s,cx,cy,r*.31f,C_GLASS2);for(int i=0;i<4;i++){float dx=i%2==0?r*.72f:0,dy=i%2? r*.72f:0;segment(s,cx-dx,cy-dy,cx+dx,cy+dy,2.1f*k,c);}}else{rect(s,(int)(cx-r*.42f),(int)(cy-r*.48f),(int)(cx+r*.42f),(int)(cy+r*.56f),c);rect(s,(int)(cx-r*.60f),(int)(cy-r*.72f),(int)(cx+r*.60f),(int)(cy-r*.52f),c);rect(s,(int)(cx-r*.20f),(int)(cy-r*.87f),(int)(cx+r*.20f),(int)(cy-r*.70f),c);rect(s,(int)(cx-r*.25f),(int)(cy-r*.25f),(int)(cx-r*.10f),(int)(cy+r*.35f),C_GLASS2);rect(s,(int)(cx+r*.10f),(int)(cy-r*.25f),(int)(cx+r*.25f),(int)(cy+r*.35f),C_GLASS2);}}
static const char*dock_name(int i){static const char*n[10]={"PEN","ERASER","MEASURE","IMPORT","Undo","REDO","FIT","PRESSURE","Settings","CLEAR"};return (i>=0&&i<10)?n[i]:"";}
static int dock_active(int i){return (i==0&&G.tool==MODE_PEN)||(i==1&&(G.tool==MODE_ERASE||G.tool==MODE_ERASE_ALL))||(i==2&&G.tool==MODE_MEASURE)||(i==7&&G.pressurePanel)||(i==8&&G.settingsPanel)||(i==3&&G.galleryOpen);}
static int dock_step(void){return maxi(us(58),mini(us(78),ui_s()*4/5));}
static int dock_base(void){return maxi(us(46),mini(us(64),ui_s()*3/5));}
static int dock_y(void){return G.screenH-margin_ui()-dock_base()/2-us(12);}
static int dock_x0(void){return G.screenW/2-(9*dock_step())/2;}
static int dock_nearest(float x){float f=(x-(float)dock_x0())/(float)dock_step();int i=(int)floorf(f+0.5f);return i<0?0:(i>9?9:i);}
static float dock_radius(int i){float base=(float)dock_base()*.5f,rr=base;if(dock_active(i))rr+=us(3);if(G.dockPointerActive){float dx=fabsf(G.dockSmoothX-(dock_x0()+i*dock_step())),t=clampf(1.0f-dx/(dock_step()*2.25f),0,1);t=t*t*(3.0f-2.0f*t);rr+=t*us(14);}return rr;}
static float grid_spacing(void){float spacing=64.0f;while(spacing*G.scale<36.0f)spacing*=2.0f;while(spacing*G.scale>104.0f)spacing*=0.5f;return spacing;}
static void draw_grid_level(Surf*s,float spacing,int alpha){if(alpha<=0)return;float wx0=(0-G.offX)/G.scale,wx1=(s->w-G.offX)/G.scale,wy0=(0-G.offY)/G.scale,wy1=(s->h-G.offY)/G.scale;int ix0=(int)floorf(wx0/spacing)-1,ix1=(int)floorf(wx1/spacing)+1,iy0=(int)floorf(wy0/spacing)-1,iy1=(int)floorf(wy1/spacing)+1;for(int i=ix0;i<=ix1;i++){float x=i*spacing*G.scale+G.offX;uint32_t c=i==0?C_AXIS:((i%4)==0?C_GRID_MAJOR:C_GRID);aa_vline(s,x,0,s->h-1,c,i==0?mini(235,alpha+72):alpha);}for(int i=iy0;i<=iy1;i++){float y=i*spacing*G.scale+G.offY;uint32_t c=i==0?C_AXIS:((i%4)==0?C_GRID_MAJOR:C_GRID);aa_hline(s,y,0,s->w-1,c,i==0?mini(235,alpha+72):alpha);}}
static void draw_grid(Surf*s){float spacing=grid_spacing(),px=spacing*G.scale;float t=clampf((68.0f-px)/32.0f,0.0f,1.0f);int a0=(int)(116.0f*(1.0f-t)),a1=(int)(132.0f*t);draw_grid_level(s,spacing,a0);if(t>.01f)draw_grid_level(s,spacing*2.0f,a1);}
static void draw_image_pixels(Surf*s,const uint32_t*px,int pw,int ph,int x0,int y0,int x1,int y1){
    if(!px||pw<=0||ph<=0||x1<=x0||y1<=y0)return;int cx0=maxi(0,x0),cy0=maxi(0,y0),cx1=mini(s->w,x1),cy1=mini(s->h,y1);if(cx1<=cx0||cy1<=cy0)return;int dw=x1-x0,dh=y1-y0;
    int64_t xstep=((int64_t)pw<<16)/dw,ystep=((int64_t)ph<<16)/dh;int64_t yfp=(int64_t)(cy0-y0)*ystep;
    if(s->fmt==FORMAT_RGB565){for(int y=cy0;y<cy1;y++,yfp+=ystep){int sy=(int)(yfp>>16);if(sy<0)sy=0;if(sy>=ph)sy=ph-1;uint16_t*dst=(uint16_t*)s->bits+y*s->stride;int64_t xfp=(int64_t)(cx0-x0)*xstep;for(int x=cx0;x<cx1;x++,xfp+=xstep){int sx=(int)(xfp>>16);if(sx<0)sx=0;if(sx>=pw)sx=pw-1;dst[x]=pack565(px[sy*pw+sx]);}}}
    else{for(int y=cy0;y<cy1;y++,yfp+=ystep){int sy=(int)(yfp>>16);if(sy<0)sy=0;if(sy>=ph)sy=ph-1;uint32_t*dst=(uint32_t*)s->bits+y*s->stride;int64_t xfp=(int64_t)(cx0-x0)*xstep;for(int x=cx0;x<cx1;x++,xfp+=xstep){int sx=(int)(xfp>>16);if(sx<0)sx=0;if(sx>=pw)sx=pw-1;dst[x]=pack8888(px[sy*pw+sx]);}}}
}

static uint32_t image_average(const ImageObj*im){if(!im||!im->px||im->pw<=0||im->ph<=0)return C_GLASS3;uint64_t rr=0,gg=0,bb=0,n=0;int sx=maxi(1,im->pw/24),sy=maxi(1,im->ph/24);for(int y=0;y<im->ph;y+=sy)for(int x=0;x<im->pw;x+=sx){uint32_t q=im->px[y*im->pw+x];rr+=(q>>16)&255;gg+=(q>>8)&255;bb+=q&255;n++;}if(!n)return C_GLASS3;return ((uint32_t)(rr/n)<<16)|((uint32_t)(gg/n)<<8)|(uint32_t)(bb/n);}
static void ensure_image_avg(ImageObj*im){if(im&&im->avgColor==0)im->avgColor=image_average(im)|0x01000000u;}
static void image_world_corners(ImageObj*im,float*x,float*y){float cx=im->x+im->w*.5f,cy=im->y+im->h*.5f,c=cosf(im->rot),sn=sinf(im->rot);float hx=im->w*.5f,hy=im->h*.5f;float lx[4]={-hx,hx,hx,-hx},ly[4]={-hy,-hy,hy,hy};for(int i=0;i<4;i++){x[i]=cx+lx[i]*c-ly[i]*sn;y[i]=cy+lx[i]*sn+ly[i]*c;}}
static int image_contains(ImageObj*im,float sx,float sy){if(!im||!im->active)return 0;float wx=(sx-G.offX)/G.scale,wy=(sy-G.offY)/G.scale,cx=im->x+im->w*.5f,cy=im->y+im->h*.5f,dx=wx-cx,dy=wy-cy,c=cosf(im->rot),sn=sinf(im->rot);float lx=dx*c+dy*sn,ly=-dx*sn+dy*c;return fabsf(lx)<=im->w*.5f&&fabsf(ly)<=im->h*.5f;}
static int image_hit(float sx,float sy){for(int i=G.imageN-1;i>=0;i--)if(G.images[i].active&&image_contains(&G.images[i],sx,sy))return i;return -1;}
static void draw_image_rotated(Surf*s,ImageObj*im){float wx[4],wy[4];image_world_corners(im,wx,wy);float minx=1e30f,miny=1e30f,maxx=-1e30f,maxy=-1e30f;for(int i=0;i<4;i++){float x=wx[i]*G.scale+G.offX,y=wy[i]*G.scale+G.offY;minx=fmin2(minx,x);maxx=fmax2(maxx,x);miny=fmin2(miny,y);maxy=fmax2(maxy,y);}int x0=maxi(0,(int)floorf(minx)),y0=maxi(0,(int)floorf(miny)),x1=mini(s->w,(int)maxx+1),y1=mini(s->h,(int)maxy+1);if(x1<=x0||y1<=y0)return;float cx=im->x+im->w*.5f,cy=im->y+im->h*.5f,c=cosf(im->rot),sn=sinf(im->rot),iw=1.0f/im->w,ih=1.0f/im->h,inv=1.0f/G.scale;
    for(int y=y0;y<y1;y++){if(s->fmt==FORMAT_RGB565){uint16_t*dst=(uint16_t*)s->bits+y*s->stride;for(int x=x0;x<x1;x++){float wxx=(x-G.offX)*inv,wyy=(y-G.offY)*inv,dx=wxx-cx,dy=wyy-cy,lx=dx*c+dy*sn+im->w*.5f,ly=-dx*sn+dy*c+im->h*.5f;if(lx>=0&&ly>=0&&lx<im->w&&ly<im->h){int ix=mini(im->pw-1,maxi(0,(int)(lx*iw*im->pw))),iy=mini(im->ph-1,maxi(0,(int)(ly*ih*im->ph)));dst[x]=pack565(im->px[iy*im->pw+ix]);}}}else{uint32_t*dst=(uint32_t*)s->bits+y*s->stride;for(int x=x0;x<x1;x++){float wxx=(x-G.offX)*inv,wyy=(y-G.offY)*inv,dx=wxx-cx,dy=wyy-cy,lx=dx*c+dy*sn+im->w*.5f,ly=-dx*sn+dy*c+im->h*.5f;if(lx>=0&&ly>=0&&lx<im->w&&ly<im->h){int ix=mini(im->pw-1,maxi(0,(int)(lx*iw*im->pw))),iy=mini(im->ph-1,maxi(0,(int)(ly*ih*im->ph)));dst[x]=pack8888(im->px[iy*im->pw+ix]);}}}}
}
static void draw_image_selection(Surf*s,ImageObj*im){float wx[4],wy[4],sx[4],sy[4];image_world_corners(im,wx,wy);for(int i=0;i<4;i++){sx[i]=wx[i]*G.scale+G.offX;sy[i]=wy[i]*G.scale+G.offY;}for(int i=0;i<4;i++){int j=(i+1)%4;segment(s,sx[i],sy[i],sx[j],sy[j],2.4f,C_CYAN);circle(s,sx[i],sy[i],us(7),C_CYAN);circle(s,sx[i],sy[i],us(3),C_BG);}}
static void draw_image_elevation(Surf*s,ImageObj*im){float wx[4],wy[4],sx[4],sy[4];image_world_corners(im,wx,wy);for(int i=0;i<4;i++){sx[i]=wx[i]*G.scale+G.offX+us(7);sy[i]=wy[i]*G.scale+G.offY+us(9);}for(int i=0;i<4;i++){int j=(i+1)%4;aa_capsule(s,sx[i],sy[i],us(5),sx[j],sy[j],us(5),C_BLACK,54);}}
static void draw_images(Surf*s){if(!G.visPhotos)return;for(int i=0;i<G.imageN;i++){ImageObj*im=&G.images[i];if(!im->active||!im->px)continue;float sw=fabsf(im->w*G.scale),sh=fabsf(im->h*G.scale);float cx=(im->x+im->w*.5f)*G.scale+G.offX,cy=(im->y+im->h*.5f)*G.scale+G.offY;if(cx+sw*.8f<-260||cy+sh*.8f<-260||cx-sw*.8f>s->w+260||cy-sh*.8f>s->h+260)continue;ensure_image_avg(im);if(G.photoMode&&i==G.selectedImage&&!G.mapRenderMode)draw_image_elevation(s,im);if(G.mapRenderMode){if(fabsf(im->rot)<0.001f){int x0=(int)floorf(im->x*G.scale+G.offX),y0=(int)floorf(im->y*G.scale+G.offY),x1=(int)((im->x+im->w)*G.scale+G.offX+0.5f),y1=(int)((im->y+im->h)*G.scale+G.offY+0.5f);if(x1>x0&&y1>y0)draw_image_pixels(s,im->px,im->pw,im->ph,x0,y0,x1,y1);}else draw_image_rotated(s,im);}else if(fmax2(sw,sh)<30.0f){float rr=clampf(fmax2(2.0f,fmin2(sw,sh)*.28f),2,9);aa_circle(s,cx,cy,rr,im->avgColor&0xffffffu,235);}else if(fmax2(sw,sh)<90.0f){uint32_t c=im->avgColor&0xffffffu;aa_capsule(s,cx-sw*.32f,cy,1.2f,cx+sw*.32f,cy,1.2f,c,220);aa_capsule(s,cx,cy-sh*.32f,1.2f,cx,cy+sh*.32f,1.2f,c,220);}else if(fabsf(im->rot)<0.001f){int x0=(int)floorf(im->x*G.scale+G.offX),y0=(int)floorf(im->y*G.scale+G.offY),x1=(int)((im->x+im->w)*G.scale+G.offX+0.5f),y1=(int)((im->y+im->h)*G.scale+G.offY+0.5f);if(x1>=0&&y1>=0&&x0<s->w&&y0<s->h)draw_image_pixels(s,im->px,im->pw,im->ph,x0,y0,x1,y1);}else draw_image_rotated(s,im);if(G.photoMode&&i==G.selectedImage)draw_image_selection(s,im);}}

/* Smooth pen path renderer.
 *
 * Input samples are intentionally kept as the document source of truth.  At
 * render time we resample them in screen space and run a midpoint quadratic
 * spline through the centerline.  This removes the visible polyline corners
 * that used to produce pointed / jagged stroke edges, while preserving the
 * original pressure data and document format.
 *
 * The spline is evaluated at roughly one screen pixel per sub-segment at
 * normal zoom, so the final edge remains round even when the canvas is shown
 * far below 100%.  Width is interpolated on the same curve as position.
 */
static Point smooth_mid_point(Point a,Point b){Point q;q.x=(a.x+b.x)*.5f;q.y=(a.y+b.y)*.5f;q.p=(a.p+b.p)*.5f;return q;}
static Point smooth_anchor_point(const Stroke*st,int idx,int live){
    Point raw=st->pts[idx];
    if(idx<=0||idx>=st->n-1)return raw;
    /* Five-tap binomial filter before spline construction. This removes the
       high-frequency zig-zag produced by digitizer noise without flattening
       deliberate large curves. Endpoints stay exact so taps and stroke ends
       still land where the stylus was placed. */
    static const float w[5]={1,4,6,4,1};float sx=0,sy=0,sp=0,ws=0;
    for(int k=-2;k<=2;k++){int j=idx+k;if(j<0)j=0;if(j>=st->n)j=st->n-1;float ww=w[k+2];sx+=st->pts[j].x*ww;sy+=st->pts[j].y*ww;sp+=st->pts[j].p*ww;ws+=ww;}
    Point avg={sx/ws,sy/ws,sp/ws};
    float amount=.38f+.54f*clampf(G.strokeSmoothing,0,1);
    if(live&&idx>=st->n-3)amount*=.72f; /* responsive tail while drawing */
    raw.x+=(avg.x-raw.x)*amount;raw.y+=(avg.y-raw.y)*amount;raw.p+=(avg.p-raw.p)*clampf(amount+.08f,0,1);
    return raw;
}
static int smooth_next_anchor(const Stroke*st,int from,float minPx){
    if(from>=st->n-1)return st->n-1;float q2=minPx*minPx;
    Point a=st->pts[from];
    for(int i=from+1;i<st->n-1;i++){float dx=(st->pts[i].x-a.x)*G.scale,dy=(st->pts[i].y-a.y)*G.scale;if(dx*dx+dy*dy>=q2)return i;}
    return st->n-1;
}
static void draw_pen_quad_piece(Surf*s,const Stroke*st,Point a,Point ctrl,Point b,uint32_t c,int alpha){
    float ax=a.x*G.scale+G.offX,ay=a.y*G.scale+G.offY,cx=ctrl.x*G.scale+G.offX,cy=ctrl.y*G.scale+G.offY,bx=b.x*G.scale+G.offX,by=b.y*G.scale+G.offY;
    float L=sqrtf(sq(cx-ax)+sq(cy-ay))+sqrtf(sq(bx-cx)+sq(by-cy));
    /* Sub-pixel tessellation keeps the outline circular instead of exposing
       the individual tapered quads on diagonal/curved handwriting. */
    int steps=maxi(3,mini(36,(int)(L/.68f)+1));
    float px=ax,py=ay,pp=a.p;
    for(int j=1;j<=steps;j++){
        float t=(float)j/(float)steps,u=1.0f-t;
        float x=u*u*ax+2.0f*u*t*cx+t*t*bx,y=u*u*ay+2.0f*u*t*cy+t*t*by;
        float pr=u*u*a.p+2.0f*u*t*ctrl.p+t*t*b.p;
        /* Pressure is intentionally smoother than the centreline. Abrupt width
           changes are what produced the little needles / scallops at joins. */
        pr=pp+(pr-pp)*.52f;
        float r0=clampf(st->baseWidth*pp*G.scale*.5f,.48f,80.0f),r1=clampf(st->baseWidth*pr*G.scale*.5f,.48f,80.0f);
        aa_capsule(s,px,py,r0,x,y,r1,c,alpha);px=x;py=y;pp=pr;
    }
}
#include "ink_curve.inc"
static void draw_pen_stroke_smooth(Surf*s,Stroke*st,uint32_t c,int alpha,int live){
    (void)live;if(!st||st->n<=0)return;
    Point a=st->pts[0];float rr=clampf(st->baseWidth*a.p*G.scale*.5f,.48f,80);
    aa_circle(s,a.x*G.scale+G.offX,a.y*G.scale+G.offY,rr,c,alpha);
    for(int i=1;i<st->n;i++){
        Point prev=st->pts[i>1?i-2:0],b=st->pts[i],p=a;
        int steps=ink_curve_steps(prev,a,b,i==1);
        for(int k=1;k<=steps;k++){
            Point q=ink_curve_point(prev,a,b,(float)k/steps,i==1);
            aa_capsule(s,p.x*G.scale+G.offX,p.y*G.scale+G.offY,clampf(st->baseWidth*p.p*G.scale*.5f,.48f,80),q.x*G.scale+G.offX,q.y*G.scale+G.offY,clampf(st->baseWidth*q.p*G.scale*.5f,.48f,80),c,alpha);p=q;
        }a=b;
    }
}

static void draw_strokes(Surf*s){
    /* Pre-cull well outside the viewport.  Geometry is already waiting before it
       becomes visible, so panning no longer reveals hard pop-in at the edges. */
    float cullPad=G.mapRenderMode?24.0f:340.0f;
    float inv=1.0f/G.scale,wx0=(-cullPad-G.offX)*inv,wy0=(-cullPad-G.offY)*inv,wx1=(s->w+cullPad-G.offX)*inv,wy1=(s->h+cullPad-G.offY)*inv;
    float vminx=fmin2(wx0,wx1),vmaxx=fmax2(wx0,wx1),vminy=fmin2(wy0,wy1),vmaxy=fmax2(wy0,wy1);
    int eraseView=G.erasing||G.tool==MODE_ERASE||G.tool==MODE_ERASE_ALL;
    int canvasMoving=(G.fingerCount>0||G.stylusDown),interactionStride=1,visiblePoints=0;
    static int64_t lodLastMs;int64_t lodNow=monotonic_ms();float lodDt=lodLastMs?clampf((lodNow-lodLastMs)*.001f,0,.25f):.016f;lodLastMs=lodNow;
    if(!G.mapRenderMode&&!eraseView){
        G.lodMotionMix+=((canvasMoving?1.0f:0.0f)-G.lodMotionMix)*(1.0f-expf(-12.0f*lodDt));
        if(!canvasMoving&&G.lodMotionMix<.01f)G.lodMotionMix=0.0f;
    }
    int interacting=!G.mapRenderMode&&!eraseView&&(canvasMoving||G.lodMotionMix>.0f);
    if(interacting){
        for(int vi=0;vi<(ER_active?ER_n:G.strokeN);vi++){Stroke*vs=&G.strokes[ER_active?ER_ids[vi]:vi];if(!vs->active||vs->n<=0)continue;float vm=vs->baseWidth*1.5f;if(vs->maxx+vm<vminx||vs->minx-vm>vmaxx||vs->maxy+vm<vminy||vs->miny-vm>vmaxy)continue;visiblePoints+=vs->n;}
        /* Continuous load response.  The stride is blended in and out with the
           interaction itself, so neither finger-down nor finger-up causes a pop. */
        float loadA=smoothstepf(1800.0f,14000.0f,(float)visiblePoints);
        float loadB=smoothstepf(14000.0f,60000.0f,(float)visiblePoints);
        float targetStride=6.0f+7.0f*loadA+20.0f*loadB;
        interactionStride=1+(int)((targetStride-1.0f)*G.lodMotionMix+0.5f);
    }
    float lodTarget;
    if(G.mapRenderMode)lodTarget=.45f;
    else{
        float z=fmax2(G.scale,.018f);
        float iz=1.0f/z;lodTarget=clampf(.50f+.14f*iz+.06f*sqrtf(iz),.62f,7.8f);
        if(interacting){
            float load=smoothstepf(5000.0f,60000.0f,(float)visiblePoints);
            float boost=2.05f+4.10f*load;
            lodTarget*=1.0f+(boost-1.0f)*G.lodMotionMix;
        }
        if(G.lodSmoothPx<=0.0f)G.lodSmoothPx=lodTarget;
        /* Hysteresis-like temporal smoothing hides quality transitions while zooming. */
        float rate=1.0f-expf(-(interacting?10.5f:16.5f)*lodDt);
        if(!eraseView){G.lodSmoothPx+=(lodTarget-G.lodSmoothPx)*rate;lodTarget=G.lodSmoothPx;}
    }
    float lodPx=lodTarget,lod2=lodPx*lodPx;
    for(int ri=0;ri<(ER_active?ER_n:G.strokeN);ri++){
        int i=ER_active?ER_ids[ri]:ri;Stroke*st=&G.strokes[i];if(!st->active||st->n<=0)continue;
        float wm=st->baseWidth*1.5f;if(st->maxx+wm<vminx||st->minx-wm>vmaxx||st->maxy+wm<vminy||st->miny-wm>vmaxy)continue;
        int aa=(int)((st->color>>24)&255);uint32_t c=st->color&0xffffffu;if(aa==0)aa=255;int highlighter=aa<248;if((highlighter&&!G.visMarker)||(!highlighter&&!G.visInk))continue;
        float bw=(st->maxx-st->minx)*G.scale,bh=(st->maxy-st->miny)*G.scale;
        if(!G.mapRenderMode&&G.scale<0.018f&&fmax2(bw,bh)<1.2f){float cx=(st->minx+st->maxx)*.5f*G.scale+G.offX,cy=(st->miny+st->maxy)*.5f*G.scale+G.offY;aa_circle(s,cx,cy,.65f,c,mini(aa,220));continue;}
        if(highlighter&&interacting&&!G.mapRenderMode&&interactionStride>=20&&i!=G.currentStroke){
            float rr=clampf(st->baseWidth*G.scale*.5f,.65f,110.0f);if(st->n==1){float x=st->pts[0].x*G.scale+G.offX,y=st->pts[0].y*G.scale+G.offY;aa_taper_quad(s,x-.6f,y,rr,x+.6f,y,rr,c,aa);continue;}int stride=interactionStride;float x0=st->pts[0].x*G.scale+G.offX,y0=st->pts[0].y*G.scale+G.offY;for(int k=mini(stride,st->n-1);k<st->n;){int knext=(k>=st->n-1)?st->n:mini(st->n-1,k+stride);Point b=st->pts[k];float x1=b.x*G.scale+G.offX,y1=b.y*G.scale+G.offY;if(!((x0<-340&&x1<-340)||(x0>s->w+340&&x1>s->w+340)||(y0<-340&&y1<-340)||(y0>s->h+340&&y1>s->h+340)))aa_taper_quad(s,x0,y0,rr,x1,y1,rr,c,aa);x0=x1;y0=y1;k=knext;}continue;
        }
        if(highlighter){
            if(!ensure_stroke_mask(s->w,s->h))continue;
            float rr=clampf(st->baseWidth*G.scale*.5f,.65f,110.0f);int bx0=s->w,by0=s->h,bx1=0,by1=0;
            if(st->n==1){float x=st->pts[0].x*G.scale+G.offX,y=st->pts[0].y*G.scale+G.offY;mask_taper_quad(x-.6f,y,rr,x+.6f,y,rr);bx0=maxi(0,(int)(x-rr-2));by0=maxi(0,(int)(y-rr-2));bx1=mini(s->w,(int)(x+rr+3));by1=mini(s->h,(int)(y+rr+3));}
            else{
                int rawStride=G.mapRenderMode?1:(G.scale>=.10f?1:(G.scale>=.035f?2:4));if(interacting&&!G.mapRenderMode)rawStride=maxi(rawStride,(i==G.currentStroke&&G.stylusDown)?1:interactionStride);
                int last=0;float x0=st->pts[0].x*G.scale+G.offX,y0=st->pts[0].y*G.scale+G.offY;
                for(int k=mini(rawStride,st->n-1);k<st->n;){int knext=(k>=st->n-1)?st->n:mini(st->n-1,k+rawStride);Point b=st->pts[k];float x1=b.x*G.scale+G.offX,y1=b.y*G.scale+G.offY,dx=x1-x0,dy=y1-y0;if(k<st->n-1&&dx*dx+dy*dy<lod2){k=knext;continue;}if(!((x0<-340&&x1<-340)||(x0>s->w+340&&x1>s->w+340)||(y0<-340&&y1<-340)||(y0>s->h+340&&y1>s->h+340))){mask_taper_quad(x0,y0,rr,x1,y1,rr);float rm=rr+2;bx0=mini(bx0,maxi(0,(int)(fmin2(x0,x1)-rm)));by0=mini(by0,maxi(0,(int)(fmin2(y0,y1)-rm)));bx1=maxi(bx1,mini(s->w,(int)(fmax2(x0,x1)+rm+1)));by1=maxi(by1,mini(s->h,(int)(fmax2(y0,y1)+rm+1)));}last=k;x0=x1;y0=y1;k=knext;}
            }
            if(bx1>bx0&&by1>by0)mask_composite_clear(s,bx0,by0,bx1,by1,c,aa);continue;
        }
        /* Pen strokes use a screen-space resampled quadratic centerline rather
           than connecting raw samples with visible straight segments. */
        draw_pen_stroke_smooth(s,st,c,aa,i==G.currentStroke&&G.stylusDown);
    }
}
static void draw_live_stroke_gpu_overlay(Surf*s){
    if(!G.gpuActive||!G.stylusDown||G.erasing||G.measuring||G.currentStroke<0||G.currentStroke>=G.strokeN)return;
    Stroke*st=&G.strokes[G.currentStroke];if(!st->active||st->n<=0)return;int aa=(int)((st->color>>24)&255);uint32_t c=st->color&0xffffffu;if(aa==0)aa=255;int highlighter=aa<248;
    if(highlighter){if(!ensure_stroke_mask(s->w,s->h))return;float rr=clampf(st->baseWidth*G.scale*.5f,.65f,110.0f);int bx0=s->w,by0=s->h,bx1=0,by1=0;if(st->n==1){float x=st->pts[0].x*G.scale+G.offX,y=st->pts[0].y*G.scale+G.offY;mask_taper_quad(x-.6f,y,rr,x+.6f,y,rr);bx0=maxi(0,(int)(x-rr-2));by0=maxi(0,(int)(y-rr-2));bx1=mini(s->w,(int)(x+rr+3));by1=mini(s->h,(int)(y+rr+3));}else{float x0=st->pts[0].x*G.scale+G.offX,y0=st->pts[0].y*G.scale+G.offY;for(int k=1;k<st->n;k++){float x1=st->pts[k].x*G.scale+G.offX,y1=st->pts[k].y*G.scale+G.offY;mask_taper_quad(x0,y0,rr,x1,y1,rr);float rm=rr+2;bx0=mini(bx0,maxi(0,(int)(fmin2(x0,x1)-rm)));by0=mini(by0,maxi(0,(int)(fmin2(y0,y1)-rm)));bx1=maxi(bx1,mini(s->w,(int)(fmax2(x0,x1)+rm+1)));by1=maxi(by1,mini(s->h,(int)(fmax2(y0,y1)+rm+1)));x0=x1;y0=y1;}}if(bx1>bx0&&by1>by0)mask_composite_clear(s,bx0,by0,bx1,by1,c,aa);return;}
    draw_pen_stroke_smooth(s,st,c,aa,1);
}
static float measure_len(Measure*m){return sqrtf(sq(m->bx-m->ax)+sq(m->by-m->ay));}
static const char*cad_unit_name(void){static const char*u[5]={"mm","cm","m","in","ft"};return (G.cadUnit>=0&&G.cadUnit<5)?u[G.cadUnit]:"m";}
static float measure_display_value(Measure*m){float v=measure_len(m);return G.cadCalibrated?v*G.cadScale:v;}
static float point_seg_dist(float px,float py,float ax,float ay,float bx,float by){float dx=bx-ax,dy=by-ay,l2=dx*dx+dy*dy;if(l2<.0001f)return sqrtf(sq(px-ax)+sq(py-ay));float t=((px-ax)*dx+(py-ay)*dy)/l2;t=clampf(t,0,1);float x=ax+t*dx,y=ay+t*dy;return sqrtf(sq(px-x)+sq(py-y));}
static int measure_hit(float sx,float sy){float best=(float)us(28);int bi=-1;for(int i=G.measureN-1;i>=0;i--){Measure*m=&G.measures[i];if(!m->active)continue;float ax=m->ax*G.scale+G.offX,ay=m->ay*G.scale+G.offY,bx=m->bx*G.scale+G.offX,by=m->by*G.scale+G.offY;float d=point_seg_dist(sx,sy,ax,ay,bx,by);if(d<best){best=d;bi=i;}}return bi;}
static void delete_selected_measure(void){if(G.selectedMeasure<0||G.selectedMeasure>=G.measureN||G.lockCAD)return;int removed=G.selectedMeasure,old=G.measureN,map[64];for(int i=0;i<old;i++)map[i]=i<removed?i:(i==removed?-1:i-1);for(int i=removed+1;i<G.measureN;i++)G.measures[i-1]=G.measures[i];G.measureN--;if(G.measureN>=0)memset(&G.measures[G.measureN],0,sizeof(Measure));if(G.selectedMeasure>=G.measureN)G.selectedMeasure=G.measureN-1;if(remap_object_refs(SEL_MEASURE,map,old))save_workspace();G.metaDirty=1;G.minimapDirty=1;}
static void draw_region_fields(Surf*s){if(G.regionGlow<=0)return;int alpha=G.regionGlow==1?12:22;int step=maxi(1,G.strokeN/28);for(int i=0;i<G.strokeN;i+=step){Stroke*st=&G.strokes[i];if(!st->active||st->n<2)continue;float x0=st->minx*G.scale+G.offX,y0=st->miny*G.scale+G.offY,x1=st->maxx*G.scale+G.offX,y1=st->maxy*G.scale+G.offY;if(x1<-120||y1<-120||x0>s->w+120||y0>s->h+120)continue;int pad=us(26);uint32_t c=(i&1)?C_VIOLET:C_ACCENT;alpha_round_rect(s,(int)x0-pad,(int)y0-pad,(int)x1+pad,(int)y1+pad,us(34),c,alpha);}for(int i=0;i<G.imageN;i++){ImageObj*im=&G.images[i];if(!im->active)continue;float x0=im->x*G.scale+G.offX,y0=im->y*G.scale+G.offY,x1=(im->x+im->w)*G.scale+G.offX,y1=(im->y+im->h)*G.scale+G.offY;int pad=us(20);alpha_round_rect(s,(int)x0-pad,(int)y0-pad,(int)x1+pad,(int)y1+pad,us(28),C_CYAN,alpha+5);}}
static void draw_constellation(Surf*s){if(G.farZoomMode<=0||G.scale>0.16f)return;int stride=maxi(1,G.strokeN/(G.performanceMode==0?60:(G.performanceMode==1?110:180))),have=0;float lx=0,ly=0;for(int i=0;i<G.strokeN;i+=stride){Stroke*st=&G.strokes[i];if(!st->active||st->n<=0)continue;float x=(st->minx+st->maxx)*.5f*G.scale+G.offX,y=(st->miny+st->maxy)*.5f*G.scale+G.offY;if(x<-50||x>s->w+50||y<-50||y>s->h+50)continue;float r=G.farZoomMode==2?2.8f:2.0f;aa_circle(s,x,y,r,C_ACCENT,210);if(G.farZoomMode==2&&have){float dx=x-lx,dy=y-ly,d2=dx*dx+dy*dy;if(d2<240.0f*240.0f)aa_capsule(s,lx,ly,.45f,x,y,.45f,C_VIOLET,45);}lx=x;ly=y;have=1;}for(int i=0;i<G.imageN;i++){ImageObj*im=&G.images[i];if(!im->active)continue;float x=(im->x+im->w*.5f)*G.scale+G.offX,y=(im->y+im->h*.5f)*G.scale+G.offY;aa_circle(s,x,y,G.farZoomMode==2?4.0f:3.0f,C_CYAN,230);}}
static void draw_dim_one(Surf*s,Measure*m,uint32_t c,int live,const char*tag,int selected){float ax=m->ax*G.scale+G.offX,ay=m->ay*G.scale+G.offY,bx=m->bx*G.scale+G.offX,by=m->by*G.scale+G.offY;float dx=bx-ax,dy=by-ay,d=sqrtf(dx*dx+dy*dy);if(d<1)return;if(selected)segment(s,ax,ay,bx,by,5.8f,C_WARN);segment(s,ax,ay,bx,by,live?2.7f:1.9f,c);float nx=-dy/d,ny=dx/d,tick=13.0f;segment(s,ax-nx*tick,ay-ny*tick,ax+nx*tick,ay+ny*tick,1.7f,c);segment(s,bx-nx*tick,by-ny*tick,bx+nx*tick,by+ny*tick,1.7f,c);circle(s,ax,ay,6.5f,C_BG);circle(s,ax,ay,2.2f,c);circle(s,bx,by,6.5f,C_BG);circle(s,bx,by,2.2f,c);float value=measure_display_value(m);char b[72];const char*unit=G.cadCalibrated?cad_unit_name():"U";if(tag&&tag[0])snprintf(b,sizeof(b),"%s  %.2f %s",tag,value,unit);else snprintf(b,sizeof(b),"%.2f %s",value,unit);int sc=2,tw=text_w(b,sc),mx=(int)((ax+bx)*0.5f+nx*21),my=(int)((ay+by)*0.5f+ny*21);round_rect(s,mx-tw/2-12,my-16,mx+tw/2+12,my+16,10,C_GLASS2);text(s,mx-tw/2,my-7,b,sc,selected?C_WARN:c);}
static void draw_measures(Surf*s){if(!G.visCAD)return;int a=G.measureN-2,b=G.measureN-1;for(int i=0;i<G.measureN;i++){if(!G.measures[i].active)continue;uint32_t c=(i==a)?C_VIOLET:(i==b?C_CYAN:0x4b5a6c);const char*tag=(i==a)?"A":((i==b)?"B":"");if(G.measureN==1&&i==0){c=C_CYAN;tag="A";}draw_dim_one(s,&G.measures[i],c,0,tag,i==G.selectedMeasure);}if(G.measuring)draw_dim_one(s,&G.liveMeasure,C_WARN,1,"LIVE",0);}
static const char*tool_name(void){if(G.photoMode)return "Photo";if(G.tool==MODE_PEN)return "Pen";if(G.tool==MODE_HIGHLIGHTER)return "Marker";if(G.tool==MODE_ERASE)return "Stroke eraser";if(G.tool==MODE_ERASE_ALL)return "Erase everything";if(G.tool==MODE_MEASURE)return "CAD";if(G.tool==MODE_SELECT)return "Select";if(G.tool==MODE_SHAPE)return "Shape";return "Tool";}
static void draw_brand_mark(Surf*s,float cx,float cy,float k){aa_circle(s,cx,cy,12*k,C_GLASS3,255);aa_capsule(s,cx-7*k,cy+5*k,1.8f*k,cx-1*k,cy-5*k,1.8f*k,C_CYAN,255);aa_capsule(s,cx-1*k,cy-5*k,1.8f*k,cx+7*k,cy+3*k,1.8f*k,C_ACCENT,255);aa_circle(s,cx+7*k,cy+3*k,2.3f*k,C_TEXT,255);}
static void draw_recent_colors(Surf*s,int m){
    int cy=m+us(80),x=m+us(46),step=us(34);for(int i=0;i<6;i++){uint32_t c=i<G.recentColorN?G.recentColors[i]:C_BORDER;float cx=(float)(x+i*step);aa_circle(s,cx,(float)cy,(float)us(11),C_BORDER,128);aa_circle(s,cx,(float)cy,(float)us(8),c,255);if(i<G.recentColorN&&(G.color&0xffffffu)==(c&0xffffffu)){aa_circle(s,cx,(float)cy,(float)us(13),C_TEXT,170);aa_circle(s,cx,(float)cy,(float)us(10),C_BG,255);aa_circle(s,cx,(float)cy,(float)us(7),c,255);}}
}
static void draw_header(Surf*s){int m=margin_ui();const char*pn=G.projectNames[G.projectIndex][0]?G.projectNames[G.projectIndex]:"Project";text(s,m+us(45),m+us(36),pn,ts(1),C_MUTED);draw_recent_colors(s,m);int h=us(44),gap=us(8),y=m;int x=s->w-m-us(612);button(s,x,y,x+us(112),y+h,"Search",G.searchPanel,C_ACCENT);x+=us(112)+gap;button(s,x,y,x+us(80),y+h,"Undo",0,C_ACCENT);x+=us(80)+gap;button(s,x,y,x+us(80),y+h,"Redo",0,C_ACCENT);x+=us(80)+gap;button(s,x,y,x+us(126),y+h,"Projects",G.projectsPanel,C_ACCENT);x+=us(126)+gap;button(s,x,y,x+us(92),y+h,"Focus",G.zenMode,C_ACCENT);}
static void draw_header_identity(Surf*s){(void)s;}
static void draw_context_strip(Surf*s){int dy=dock_y(),h=us(56),y=dy-dock_base()/2-us(20)-h,w=us(650),x=(s->w-w)/2;glass(s,x,y,x+w,y+h,us(19));int by=y+us(8),bh=h-us(16);if(G.tool==MODE_PEN){button(s,x+us(24),by,x+us(70),by+bh,"-",0,C_ACCENT);char bw[24];snprintf(bw,sizeof(bw),"%.1f",G.brush);button(s,x+us(78),by,x+us(148),by+bh,bw,1,G.color);button(s,x+us(156),by,x+us(202),by+bh,"+",0,C_ACCENT);text(s,x+us(224),y+us(20),"COLOR",ts(2),C_MUTED);uint32_t cols[5]={0xf4f6f8,0x6aa9ff,0xff6475,0x5fe39a,0xffc55d};int cx=x+us(330);for(int i=0;i<5;i++){float rr=(float)us(G.color==cols[i]?15:12);circle(s,(float)(cx+i*us(58)),(float)(y+h/2),rr+us(4),C_GLASS3);circle(s,(float)(cx+i*us(58)),(float)(y+h/2),rr,cols[i]);if(G.color==cols[i])circle(s,(float)(cx+i*us(58)),(float)(y+h/2),(float)us(4),C_TEXT);}}else if(G.tool==MODE_MEASURE){button(s,x+us(18),by,x+us(154),by+bh,G.snap?"SNAP 16U":"FREE",G.snap,C_CYAN);char qa[40],qb[40],qr[40];if(G.measureN>=2){float a=measure_len(&G.measures[G.measureN-2]),b=measure_len(&G.measures[G.measureN-1]);snprintf(qa,sizeof(qa),"A %.1f",a);snprintf(qb,sizeof(qb),"B %.1f",b);snprintf(qr,sizeof(qr),"B/A %.3f",a>0.0001f?b/a:0);}else if(G.measureN==1){float a=measure_len(&G.measures[0]);snprintf(qa,sizeof(qa),"A %.1f",a);snprintf(qb,sizeof(qb),"B ---");snprintf(qr,sizeof(qr),"DRAW B");}else{snprintf(qa,sizeof(qa),"A ---");snprintf(qb,sizeof(qb),"B ---");snprintf(qr,sizeof(qr),"DRAW A");}button(s,x+us(174),by,x+us(318),by+bh,qa,1,C_VIOLET);button(s,x+us(330),by,x+us(474),by+bh,qb,1,C_CYAN);button(s,x+us(486),by,x+w-us(18),by+bh,qr,0,C_TEXT);}else{text(s,x+us(72),y+us(20),G.tool==MODE_ERASE_ALL?"ERASE EVERYTHING":"STROKE ERASER",ts(2),C_DANGER);text(s,x+us(330),y+us(20),G.tool==MODE_ERASE_ALL?"UNLOCKED OBJECTS":"SIDE BUTTON MAPPABLE",ts(2),C_MUTED);}}
static void draw_dock(Surf*s){int base=dock_base(),step=dock_step(),cy=dock_y(),x0=dock_x0();int xL=x0-base/2-us(22),xR=x0+9*step+base/2+us(22),y0=cy-base/2-us(18),y1=cy+base/2+us(18);glass(s,xL,y0,xR,y1,us(30));static const int kinds[10]={0,1,2,8,3,4,5,6,9,7};for(int i=0;i<10;i++){float cx=(float)(x0+i*step),rr=dock_radius(i);int active=dock_active(i);uint32_t accent=i==0?C_ACCENT:(i==1?C_DANGER:(i==2?C_CYAN:(i==3?C_GREEN:(i==7?C_VIOLET:(i==8?C_WARN:(i==9?C_DANGER:C_TEXT))))));circle(s,cx,cy+us(5),rr+us(5),C_BLACK);if(active)circle(s,cx,cy,rr+us(3),accent);circle(s,cx,cy,rr,G.dockPointerActive&&i==G.dockHot?C_GLASS3:C_GLASS2);if(active)circle(s,cx,cy+rr-us(5),us(3),accent);line_icon(s,kinds[i],cx,cy,(rr/(base*.5f)),active?accent:C_TEXT);}if(G.dockPointerActive&&G.dockHot>=0&&G.dockHot<10){const char*n=(G.dockHot==9&&G.clearArmed)?"TAP AGAIN":dock_name(G.dockHot);int sc=ts(2),tw=text_w(n,sc),cx=x0+G.dockHot*step,yy=y0-us(54);round_rect(s,cx-tw/2-us(12),yy,cx+tw/2+us(12),yy+us(30),us(10),C_GLASS2);text(s,cx-tw/2,yy+us(8),n,sc,C_TEXT);}}

static uint32_t hsv_rgb(float h,float sat,float val){h=h-floorf(h);sat=clampf(sat,0,1);val=clampf(val,0,1);float q=h*6.0f;int i=(int)floorf(q);float f=q-i,p=val*(1-sat),r=val,g=val,b=val;float a=val*(1-sat*f),c=val*(1-sat*(1-f));switch(i%6){case 0:r=val;g=c;b=p;break;case 1:r=a;g=val;b=p;break;case 2:r=p;g=val;b=c;break;case 3:r=p;g=a;b=val;break;case 4:r=c;g=p;b=val;break;default:r=val;g=p;b=a;break;}return ((uint32_t)(r*255+.5f)<<16)|((uint32_t)(g*255+.5f)<<8)|(uint32_t)(b*255+.5f);}
static void rgb_hsv(uint32_t c,float*h,float*sat,float*val){float r=((c>>16)&255)/255.0f,g=((c>>8)&255)/255.0f,b=(c&255)/255.0f,mx=fmax2(r,fmax2(g,b)),mn=fmin2(r,fmin2(g,b)),d=mx-mn;*val=mx;*sat=mx<=.0001f?0:d/mx;float hh=0;if(d>.0001f){if(mx==r)hh=(g-b)/d+(g<b?6:0);else if(mx==g)hh=(b-r)/d+2;else hh=(r-g)/d+4;hh/=6;}*h=hh;}
static int color_drawer_w(void){return mini(us(470),maxi(us(360),G.screenW/3));}
static int color_handle_w(void){return us(58);}
static void color_drawer_geom_for(float anim,int*ox,int*oy,int*ow,int*oh,int*handleW){int w=color_drawer_w(),hh=mini(us(560),G.screenH-2*margin_ui()),hw=color_handle_w();float a=clampf(anim,0.0f,1.0f);int x=G.screenW-hw-(int)((w-hw)*a),y=(G.screenH-hh)/2;*ox=x;*oy=y;*ow=w;*oh=hh;*handleW=hw;}
static void color_drawer_geom(int*ox,int*oy,int*ow,int*oh,int*handleW){color_drawer_geom_for(G.colorRailAnim,ox,oy,ow,oh,handleW);}
static void picker_commit_session(void){if(G.pickerSessionTouched){recent_color_add_from_picker(G.color);G.pickerSessionTouched=0;}if(G.pickerApplySelection){selection_apply_color();G.pickerApplySelection=0;}}
static void open_color_picker(void){rgb_hsv(G.color,&G.pickerHue,&G.pickerSat,&G.pickerVal);G.pickerSessionStartColor=G.color;G.pickerSessionTouched=0;G.colorPickerOpen=1;G.settingsPanel=0;G.galleryOpen=0;G.pressurePanel=0;start_anim_timer();}
static void close_color_picker(void){picker_commit_session();G.colorPickerOpen=0;G.pickerDragging=0;G.colorRailDragging=0;start_anim_timer();}
static void picker_geometry_for(float anim,int*px,int*py,int*pw,int*ph,int*svx,int*svy,int*svw,int*svh,int*hx,int*hy,int*hw,int*hh){
    /* Blender-like picker layout: circular Hue/Saturation field + a tall
       Value slider. The function keeps the historic parameter names so the
       rest of the input router does not need a new ABI. */
    int x,y,w,h,tab;color_drawer_geom_for(anim,&x,&y,&w,&h,&tab);int cx=x+tab-us(4),cw=w-tab+us(4);
    *px=cx;*py=y;*pw=cw;*ph=h;
    int maxD=cw-us(112);int d=mini(us(300),maxi(us(220),maxD));
    *svx=cx+us(24);*svy=y+us(78);*svw=d;*svh=d;
    *hx=*svx+d+us(18);*hy=*svy;*hw=us(26);*hh=d;
}
static void picker_geometry(int*px,int*py,int*pw,int*ph,int*svx,int*svy,int*svw,int*svh,int*hx,int*hy,int*hw,int*hh){picker_geometry_for(G.colorRailAnim,px,py,pw,ph,svx,svy,svw,svh,hx,hy,hw,hh);}
static int color_handle_hit(float x,float y){int px,py,pw,ph,hw;color_drawer_geom(&px,&py,&pw,&ph,&hw);float cy=py+ph*.5f;return x>=px-us(4)&&x<=px+hw+us(10)&&y>=cy-us(62)&&y<=cy+us(62);}
static int color_panel_hit(float x,float y){int px,py,pw,ph,hw;color_drawer_geom(&px,&py,&pw,&ph,&hw);return G.colorRailAnim>.06f&&hit_box(x,y,px+hw-us(4),py,px+pw,py+ph);}
static int color_picker_recent_hit(float x,float y){int px,py,pw,ph,sx,sy,sw,sh,hx,hy,hw,hh;picker_geometry(&px,&py,&pw,&ph,&sx,&sy,&sw,&sh,&hx,&hy,&hw,&hh);int cy=py+ph-us(48),cx=px+pw-us(196);for(int i=0;i<G.recentColorN&&i<6;i++)if(sq(x-(cx+i*us(28)))+sq(y-cy)<=sq((float)us(16)))return i;return -1;}
static int color_picker_field_hit(float x,float y){int px,py,pw,ph,sx,sy,sw,sh,hx,hy,hw,hh;picker_geometry(&px,&py,&pw,&ph,&sx,&sy,&sw,&sh,&hx,&hy,&hw,&hh);float cx=sx+sw*.5f,cy=sy+sh*.5f,r=sw*.5f;return sq(x-cx)+sq(y-cy)<=sq(r+us(10))||hit_box(x,y,hx-us(8),hy-us(8),hx+hw+us(8),hy+hh+us(8));}
static void color_picker_touch(float x,float y){
    int px,py,pw,ph,sx,sy,sw,sh,hx,hy,hw,hh;picker_geometry(&px,&py,&pw,&ph,&sx,&sy,&sw,&sh,&hx,&hy,&hw,&hh);
    float cx=sx+sw*.5f,cy=sy+sh*.5f,r=sw*.5f,dx=x-cx,dy=y-cy,dist=sqrtf(dx*dx+dy*dy);
    int inWheel=dist<=r+us(10),inValue=hit_box(x,y,hx-us(8),hy-us(8),hx+hw+us(8),hy+hh+us(8));
    if(inWheel||G.pickerDragging==1){
        if(dist<.0001f){G.pickerSat=0.0f;}else{G.pickerSat=clampf(dist/r,0,1);float ang=atan2f(-dy,dx);if(ang<0)ang+=2.0f*PI;G.pickerHue=ang/(2.0f*PI);}
        G.color=hsv_rgb(G.pickerHue,G.pickerSat,G.pickerVal);G.metaDirty=1;G.pickerDragging=1;G.pickerSessionTouched=1;
    }else if(inValue||G.pickerDragging==2){
        G.pickerVal=clampf(1.0f-(y-hy)/(float)hh,0,1);G.color=hsv_rgb(G.pickerHue,G.pickerSat,G.pickerVal);G.metaDirty=1;G.pickerDragging=2;G.pickerSessionTouched=1;
    }
}
static void draw_picker_hs_wheel(Surf*s,int x0,int y0,int d){
    /* Cache the expensive atan2/sqrt wheel because its gamut never changes;
       only the selection cursor and the Value bar move during interaction. */
    static uint32_t*cache=0;static int cd=0;if(d<=2)return;
    if(cd!=d||!cache){if(cache)free(cache);cache=(uint32_t*)malloc((size_t)d*d*sizeof(uint32_t));cd=cache?d:0;if(!cache)return;float cx=d*.5f,cy=d*.5f,r=d*.5f-1.2f;for(int y=0;y<d;y++){float dy=(y+.5f)-cy;for(int x=0;x<d;x++){float dx=(x+.5f)-cx,dist=sqrtf(dx*dx+dy*dy),edge=r-dist;uint32_t v=0;if(edge>-1.1f){float a=smoothstepf(-1.1f,.85f,edge);float h=atan2f(-dy,dx);if(h<0)h+=2.0f*PI;float sat=clampf(dist/r,0,1);v=((uint32_t)(a*255.0f+.5f)<<24)|hsv_rgb(h/(2.0f*PI),sat,1.0f);}cache[y*d+x]=v;}}}
    float cx=x0+d*.5f,cy=y0+d*.5f,r=d*.5f;
    /* Draw the rim first. aa_circle() is a filled primitive; drawing it after
       the wheel would wash the gamut out with the border colour. */
    aa_circle(s,cx,cy,r+1.6f,C_BORDER,150);
    for(int y=0;y<d;y++)for(int x=0;x<d;x++){uint32_t v=cache[y*d+x];int a=(v>>24)&255;if(a)blendpx(s,x0+x,y0+y,v&0xffffffu,a);}
}
static void draw_picker_value_gradient(Surf*s,int x0,int y0,int w,int h){
    if(w<=2||h<=2)return;alpha_round_rect(s,x0-us(1),y0-us(1),x0+w+us(1),y0+h+us(1),us(9),C_BORDER,145);for(int y=y0;y<y0+h;y++){float v=1.0f-((float)(y-y0)+.5f)/(float)h;uint32_t c=hsv_rgb(G.pickerHue,G.pickerSat,v);for(int x=x0;x<x0+w;x++)putpx(s,x,y,c);}
}
static void draw_color_picker(Surf*s){
    if(android_ui_available()){if(G.presentationMode||android_ui_modal())return;float r=us(88),x=s->w-r*.43f,y=s->h-r*.43f;aa_circle(s,s->w,s->h,r+us(2),C_TEXT,65);aa_circle(s,s->w,s->h,r,mix_color(C_GLASS2,C_TEXT,7),233);aa_circle(s,x,y,us(18),C_TEXT,165);aa_circle(s,x,y,us(15),G.color,255);return;}
    if(G.presentationMode||G.settingsPanel||G.galleryOpen||G.calibrationOpen||G.projectsPanel||(G.editorOpen&&G.editorMode!=2&&G.editorMode!=5))return;
    int bx,by,bw,bh,tab;color_drawer_geom(&bx,&by,&bw,&bh,&tab);float cy=by+bh*.5f;
    if(G.colorRailAnim>.015f){int px,py,pw,ph,sx,sy,sw,sh,hx,hy,hw,hh;picker_geometry(&px,&py,&pw,&ph,&sx,&sy,&sw,&sh,&hx,&hy,&hw,&hh);
        /* Visual language follows Blender's compact dark color popup: one
           dominant HS wheel, a narrow Value strip, current/previous swatches,
           terse HSV readout and hex value. */
        uint32_t panel=mix_color(C_GLASS2,C_BLACK,24),well=mix_color(C_GLASS3,C_BLACK,12),edge=mix_color(C_BORDER,C_TEXT,8);
        alpha_round_rect(s,px,py,px+pw,py+ph,us(18),C_BLACK,94);alpha_round_rect(s,px+us(1),py+us(1),px+pw-us(1),py+ph-us(1),us(17),panel,252);
        aa_hline(s,(float)(py+us(62)),px+us(18),px+pw-us(18),edge,92);
        text(s,px+us(22),py+us(20),"Color",ts(2),C_TEXT);button(s,px+pw-us(88),py+us(14),px+pw-us(18),py+us(50),"Done",0,C_TEXT);
        draw_picker_hs_wheel(s,sx,sy,sw);draw_picker_value_gradient(s,hx,hy,hw,hh);
        float ang=G.pickerHue*2.0f*PI,rr=G.pickerSat*(sw*.5f-us(2));float wcx=sx+sw*.5f,wcy=sy+sh*.5f;float mx=wcx+cosf(ang)*rr,my=wcy-sinf(ang)*rr;uint32_t picked=hsv_rgb(G.pickerHue,G.pickerSat,G.pickerVal);aa_circle(s,mx,my,(float)us(9),C_BLACK,230);aa_circle(s,mx,my,(float)us(6),C_TEXT,255);aa_circle(s,mx,my,(float)us(3),picked,255);
        int vy=hy+(int)((1.0f-G.pickerVal)*hh);alpha_round_rect(s,hx-us(6),vy-us(3),hx+hw+us(6),vy+us(3),us(3),C_BLACK,225);alpha_round_rect(s,hx-us(4),vy-us(2),hx+hw+us(4),vy+us(2),us(2),C_TEXT,255);
        int infoY=sy+sh+us(18);alpha_round_rect(s,px+us(22),infoY,px+pw-us(18),infoY+us(76),us(10),well,238);
        /* Blender-style current / previous split swatch. */
        int swx=px+us(32),swy=infoY+us(13),sww=us(74),swh=us(48);alpha_round_rect(s,swx-us(1),swy-us(1),swx+sww+us(1),swy+swh+us(1),us(7),edge,180);alpha_rect(s,swx,swy,swx+sww/2,swy+swh,G.pickerSessionStartColor&0xffffffu,255);alpha_rect(s,swx+sww/2,swy,swx+sww,swy+swh,G.color,255);
        char hsv[72],hex[24];snprintf(hsv,sizeof(hsv),"H %3d deg   S %3d%%   V %3d%%",(int)(G.pickerHue*360.0f+.5f),(int)(G.pickerSat*100.0f+.5f),(int)(G.pickerVal*100.0f+.5f));snprintf(hex,sizeof(hex),"#%06X",G.color&0xffffffu);text(s,px+us(122),infoY+us(13),hsv,ts(1),C_MUTED);text(s,px+us(122),infoY+us(42),hex,ts(2),C_TEXT);
        int fy=py+ph-us(48),rx=px+pw-us(196);text(s,px+us(22),fy-us(7),"Recent",ts(1),C_MUTED);for(int i=0;i<6&&i<G.recentColorN;i++){uint32_t rc=G.recentColors[i];int sel=(G.color&0xffffffu)==(rc&0xffffffu);aa_circle(s,rx+i*us(28),fy,(float)us(sel?12:10),sel?C_TEXT:C_BORDER,sel?230:150);aa_circle(s,rx+i*us(28),fy,(float)us(8),rc,255);}
    }
    alpha_round_rect(s,bx-us(2),(int)(cy-us(58)),bx+tab+us(4),(int)(cy+us(58)),us(28),C_BORDER,116);alpha_round_rect(s,bx,(int)(cy-us(56)),bx+tab+us(2),(int)(cy+us(56)),us(27),mix_color(C_GLASS2,G.color,14),250);aa_circle(s,bx+tab*.48f,cy,(float)us(17),C_BLACK,155);aa_circle(s,bx+tab*.48f,cy,(float)us(13),G.color,255);
}
static const char*rail_name(int i){static const char*n[8]={"Add","Frames","Photos","CAD","Search","Layers","Map","Settings"};return (i>=0&&i<8)?n[i]:"";}
static int rail_active(int i){return (i==0&&G.addPanel)||(i==1&&G.framePanel)||(i==2&&G.photoMode)||(i==3&&G.tool==MODE_MEASURE)||(i==4&&G.searchPanel)||(i==5&&G.layersPanel)||(i==6&&G.minimap)||(i==7&&G.settingsPanel);}
static void rail_geom(int*iX,int*iY,int*iW,int*iH,int*iGap){*iW=us(124);*iH=us(48);*iGap=us(8);*iX=margin_ui();int total=8*(*iH)+7*(*iGap);*iY=(G.screenH-total)/2;}
static void draw_rail_closed(Surf*s){float cy=s->h*.5f,r=(float)us(34);alpha_circle(s,0,cy,r,C_BLACK,145);circle(s,0,cy,r,C_GLASS2);segment(s,us(12),cy-us(9),us(22),cy,2.2f,C_TEXT);segment(s,us(22),cy,us(12),cy+us(9),2.2f,C_TEXT);}
static void draw_utility_rail(Surf*s){float motion=clampf(G.railAnim,-.10f,1.14f),a=clampf(G.railAnim,0,1);if(a<.025f&&motion<=.03f){draw_rail_closed(s);return;}int x,y,w,h,g;rail_geom(&x,&y,&w,&h,&g);float e=motion;x-=(int)((1.0f-e)*(w+us(40)));glass(s,x-us(9),y-us(10),x+w+us(9),y+8*h+7*g+us(10),us(22));for(int i=0;i<8;i++){int yy=y+i*(h+g),active=rail_active(i);button(s,x,yy,x+w,yy+h,rail_name(i),active,C_ACCENT);if(active)alpha_round_rect(s,x+us(5),yy+us(10),x+us(8),yy+h-us(10),us(2),C_ACCENT,255);}if(a<.72f)draw_rail_closed(s);}


// ---------- Android Views shell (Vast 3.2) ----------
// Platform Views own ordinary app chrome; spatial feedback remains in the native renderer.
typedef struct {int ready,hostShown;jobject host,popup; jobject mark,title,project; jobject header[5],recent[6]; jobject railBg,rail[8],railHandle,inkToggle; jobject colorBg,colorTitle,colorClose,colorSV,colorHue,colorPreview,colorHex,colorTab,colorTabDot,colorCursor,colorHueCursor,colorRecent[6]; jobject searchQuery,searchResult[6]; float lastHue; uint64_t stateHash,themeHash,geomHash;} NativeUiShell;
static NativeUiShell NU;
static jint nui_argb(uint32_t rgb,int a){return (jint)(((uint32_t)(a&255)<<24)|(rgb&0xffffffu));}
static void nui_exc(JNIEnv*e){if((*e)->ExceptionCheck(e))(*e)->ExceptionClear(e);}
static jobject nui_round_drawable(JNIEnv*e,uint32_t fill,int fillA,uint32_t stroke,int strokeA,int strokePx,float radius,int oval){jclass c=(*e)->FindClass(e,"android/graphics/drawable/GradientDrawable");if(!c){nui_exc(e);return 0;}jmethodID ctor=(*e)->GetMethodID(e,c,"<init>","()V");jobject d=ctor?(*e)->NewObject(e,c,ctor):0;nui_exc(e);if(d){jmethodID shape=(*e)->GetMethodID(e,c,"setShape","(I)V"),col=(*e)->GetMethodID(e,c,"setColor","(I)V"),cr=(*e)->GetMethodID(e,c,"setCornerRadius","(F)V"),st=(*e)->GetMethodID(e,c,"setStroke","(II)V");if(shape)(*e)->CallVoidMethod(e,d,shape,oval?1:0);if(col)(*e)->CallVoidMethod(e,d,col,nui_argb(fill,fillA));if(!oval&&cr)(*e)->CallVoidMethod(e,d,cr,radius);if(st&&strokePx>0)(*e)->CallVoidMethod(e,d,st,strokePx,nui_argb(stroke,strokeA));nui_exc(e);}(*e)->DeleteLocalRef(e,c);return d;}
static void nui_set_bg(JNIEnv*e,jobject v,jobject d){if(!v)return;jclass c=(*e)->GetObjectClass(e,v);jmethodID m=c?(*e)->GetMethodID(e,c,"setBackground","(Landroid/graphics/drawable/Drawable;)V"):0;if(m)(*e)->CallVoidMethod(e,v,m,d);nui_exc(e);if(c)(*e)->DeleteLocalRef(e,c);}
static void nui_bg(JNIEnv*e,jobject v,uint32_t fill,int fillA,uint32_t stroke,int strokeA,int strokePx,float radius,int oval){jobject d=nui_round_drawable(e,fill,fillA,stroke,strokeA,strokePx,radius,oval);if(d){nui_set_bg(e,v,d);(*e)->DeleteLocalRef(e,d);}}
static void nui_text(JNIEnv*e,jobject v,const char*t){if(!v)return;jclass c=(*e)->GetObjectClass(e,v);jmethodID m=c?(*e)->GetMethodID(e,c,"setText","(Ljava/lang/CharSequence;)V"):0;jstring js=(*e)->NewStringUTF(e,t?t:"");if(m&&js)(*e)->CallVoidMethod(e,v,m,js);nui_exc(e);if(js)(*e)->DeleteLocalRef(e,js);if(c)(*e)->DeleteLocalRef(e,c);}
static void nui_text_color(JNIEnv*e,jobject v,uint32_t rgb,int a){if(!v)return;jclass c=(*e)->GetObjectClass(e,v);jmethodID m=c?(*e)->GetMethodID(e,c,"setTextColor","(I)V"):0;if(m)(*e)->CallVoidMethod(e,v,m,nui_argb(rgb,a));nui_exc(e);if(c)(*e)->DeleteLocalRef(e,c);}
static void nui_text_size(JNIEnv*e,jobject v,float px){if(!v)return;jclass c=(*e)->GetObjectClass(e,v);jmethodID m=c?(*e)->GetMethodID(e,c,"setTextSize","(IF)V"):0;if(m)(*e)->CallVoidMethod(e,v,m,0,px);nui_exc(e);if(c)(*e)->DeleteLocalRef(e,c);}
static void nui_vis(JNIEnv*e,jobject v,int show){if(!v)return;jclass c=(*e)->GetObjectClass(e,v);jmethodID m=c?(*e)->GetMethodID(e,c,"setVisibility","(I)V"):0;if(m)(*e)->CallVoidMethod(e,v,m,show?0:8);nui_exc(e);if(c)(*e)->DeleteLocalRef(e,c);}
static void nui_alpha(JNIEnv*e,jobject v,float a){if(!v)return;jclass c=(*e)->GetObjectClass(e,v);jmethodID m=c?(*e)->GetMethodID(e,c,"setAlpha","(F)V"):0;if(m)(*e)->CallVoidMethod(e,v,m,a);nui_exc(e);if(c)(*e)->DeleteLocalRef(e,c);}
static void nui_tx(JNIEnv*e,jobject v,float x){if(!v)return;jclass c=(*e)->GetObjectClass(e,v);jmethodID m=c?(*e)->GetMethodID(e,c,"setTranslationX","(F)V"):0;if(m)(*e)->CallVoidMethod(e,v,m,x);nui_exc(e);if(c)(*e)->DeleteLocalRef(e,c);}
static void nui_ty(JNIEnv*e,jobject v,float y){if(!v)return;jclass c=(*e)->GetObjectClass(e,v);jmethodID m=c?(*e)->GetMethodID(e,c,"setTranslationY","(F)V"):0;if(m)(*e)->CallVoidMethod(e,v,m,y);nui_exc(e);if(c)(*e)->DeleteLocalRef(e,c);}
static void nui_elevation(JNIEnv*e,jobject v,float z){if(!v)return;jclass c=(*e)->GetObjectClass(e,v);jmethodID m=c?(*e)->GetMethodID(e,c,"setElevation","(F)V"):0;if(m)(*e)->CallVoidMethod(e,v,m,z);nui_exc(e);if(c)(*e)->DeleteLocalRef(e,c);}
static void nui_frame(JNIEnv*e,jobject v,int x,int y,int w,int h){if(!v)return;jclass lc=(*e)->FindClass(e,"android/widget/FrameLayout$LayoutParams");jmethodID ctor=lc?(*e)->GetMethodID(e,lc,"<init>","(II)V"):0;jobject lp=ctor?(*e)->NewObject(e,lc,ctor,w,h):0;if(lp&&lc){jmethodID sm=(*e)->GetMethodID(e,lc,"setMargins","(IIII)V");if(sm)(*e)->CallVoidMethod(e,lp,sm,x,y,0,0);}jclass vc=(*e)->GetObjectClass(e,v);jmethodID sl=vc?(*e)->GetMethodID(e,vc,"setLayoutParams","(Landroid/view/ViewGroup$LayoutParams;)V"):0;if(sl&&lp)(*e)->CallVoidMethod(e,v,sl,lp);nui_exc(e);if(vc)(*e)->DeleteLocalRef(e,vc);if(lp)(*e)->DeleteLocalRef(e,lp);if(lc)(*e)->DeleteLocalRef(e,lc);}
static jobject nui_make_text(JNIEnv*e,const char*t,int sizePx,int gravity){jclass c=(*e)->FindClass(e,"android/widget/TextView");jmethodID ctor=c?(*e)->GetMethodID(e,c,"<init>","(Landroid/content/Context;)V"):0;jobject v=ctor?(*e)->NewObject(e,c,ctor,G.activity->clazz):0;if(v){jmethodID st=(*e)->GetMethodID(e,c,"setText","(Ljava/lang/CharSequence;)V"),sz=(*e)->GetMethodID(e,c,"setTextSize","(IF)V"),gr=(*e)->GetMethodID(e,c,"setGravity","(I)V"),ip=(*e)->GetMethodID(e,c,"setIncludeFontPadding","(Z)V"),cl=(*e)->GetMethodID(e,c,"setClickable","(Z)V"),fo=(*e)->GetMethodID(e,c,"setFocusable","(Z)V"),ll=(*e)->GetMethodID(e,c,"setLongClickable","(Z)V"),pad=(*e)->GetMethodID(e,c,"setPadding","(IIII)V");jstring js=(*e)->NewStringUTF(e,t?t:"");if(st&&js)(*e)->CallVoidMethod(e,v,st,js);if(js)(*e)->DeleteLocalRef(e,js);if(sz)(*e)->CallVoidMethod(e,v,sz,0,(jfloat)sizePx);if(gr)(*e)->CallVoidMethod(e,v,gr,gravity);if(ip)(*e)->CallVoidMethod(e,v,ip,(jboolean)0);if(cl)(*e)->CallVoidMethod(e,v,cl,(jboolean)0);if(fo)(*e)->CallVoidMethod(e,v,fo,(jboolean)0);if(ll)(*e)->CallVoidMethod(e,v,ll,(jboolean)0);if(pad)(*e)->CallVoidMethod(e,v,pad,us(8),0,us(8),0);nui_exc(e);}if(c)(*e)->DeleteLocalRef(e,c);return v;}
static int nui_host_create(JNIEnv*e){if(NU.hostShown&&NU.host&&NU.popup)return 1;jclass fc=(*e)->FindClass(e,"android/widget/FrameLayout");jmethodID fctor=fc?(*e)->GetMethodID(e,fc,"<init>","(Landroid/content/Context;)V"):0;jobject root=fctor?(*e)->NewObject(e,fc,fctor,G.activity->clazz):0;nui_exc(e);if(!root){if(fc)(*e)->DeleteLocalRef(e,fc);return 0;}jmethodID clip=(*e)->GetMethodID(e,fc,"setClipChildren","(Z)V"),clipPad=(*e)->GetMethodID(e,fc,"setClipToPadding","(Z)V");if(clip)(*e)->CallVoidMethod(e,root,clip,(jboolean)0);if(clipPad)(*e)->CallVoidMethod(e,root,clipPad,(jboolean)0);nui_exc(e);jclass pc=(*e)->FindClass(e,"android/widget/PopupWindow");jmethodID pctor=pc?(*e)->GetMethodID(e,pc,"<init>","(Landroid/content/Context;)V"):0;jobject pop=pctor?(*e)->NewObject(e,pc,pctor,G.activity->clazz):0;nui_exc(e);if(!pop){if(fc)(*e)->DeleteLocalRef(e,fc);if(pc)(*e)->DeleteLocalRef(e,pc);(*e)->DeleteLocalRef(e,root);return 0;}jmethodID scv=(*e)->GetMethodID(e,pc,"setContentView","(Landroid/view/View;)V"),sw=(*e)->GetMethodID(e,pc,"setWidth","(I)V"),sh=(*e)->GetMethodID(e,pc,"setHeight","(I)V"),sf=(*e)->GetMethodID(e,pc,"setFocusable","(Z)V"),st=(*e)->GetMethodID(e,pc,"setTouchable","(Z)V"),so=(*e)->GetMethodID(e,pc,"setOutsideTouchable","(Z)V"),scl=(*e)->GetMethodID(e,pc,"setClippingEnabled","(Z)V"),sbg=(*e)->GetMethodID(e,pc,"setBackgroundDrawable","(Landroid/graphics/drawable/Drawable;)V"),show=(*e)->GetMethodID(e,pc,"showAtLocation","(Landroid/view/View;III)V");if(scv)(*e)->CallVoidMethod(e,pop,scv,root);if(sw)(*e)->CallVoidMethod(e,pop,sw,-1);if(sh)(*e)->CallVoidMethod(e,pop,sh,-1);if(sf)(*e)->CallVoidMethod(e,pop,sf,(jboolean)0);if(st)(*e)->CallVoidMethod(e,pop,st,(jboolean)0);if(so)(*e)->CallVoidMethod(e,pop,so,(jboolean)0);if(scl)(*e)->CallVoidMethod(e,pop,scl,(jboolean)0);jclass cc=(*e)->FindClass(e,"android/graphics/drawable/ColorDrawable");jmethodID cctor=cc?(*e)->GetMethodID(e,cc,"<init>","(I)V"):0;jobject clear=cctor?(*e)->NewObject(e,cc,cctor,0):0;if(sbg&&clear)(*e)->CallVoidMethod(e,pop,sbg,clear);nui_exc(e);jclass ac=(*e)->GetObjectClass(e,G.activity->clazz);jmethodID gw=ac?(*e)->GetMethodID(e,ac,"getWindow","()Landroid/view/Window;"):0;jobject win=gw?(*e)->CallObjectMethod(e,G.activity->clazz,gw):0;nui_exc(e);jobject decor=0;if(win){jclass wc=(*e)->GetObjectClass(e,win);jmethodID gd=wc?(*e)->GetMethodID(e,wc,"getDecorView","()Landroid/view/View;"):0;decor=gd?(*e)->CallObjectMethod(e,win,gd):0;nui_exc(e);if(wc)(*e)->DeleteLocalRef(e,wc);}if(show&&decor)(*e)->CallVoidMethod(e,pop,show,decor,51,0,0);int bad=(*e)->ExceptionCheck(e);if(bad)(*e)->ExceptionClear(e);if(!bad&&decor){NU.host=(*e)->NewGlobalRef(e,root);NU.popup=(*e)->NewGlobalRef(e,pop);NU.hostShown=1;}if(decor)(*e)->DeleteLocalRef(e,decor);if(win)(*e)->DeleteLocalRef(e,win);if(ac)(*e)->DeleteLocalRef(e,ac);if(clear)(*e)->DeleteLocalRef(e,clear);if(cc)(*e)->DeleteLocalRef(e,cc);if(pc)(*e)->DeleteLocalRef(e,pc);if(fc)(*e)->DeleteLocalRef(e,fc);(*e)->DeleteLocalRef(e,pop);(*e)->DeleteLocalRef(e,root);return NU.hostShown;}
static void nui_attach(JNIEnv*e,jobject local,int x,int y,int w,int h,jobject*dst){if(!local||!NU.host){if(local)(*e)->DeleteLocalRef(e,local);return;}jclass lc=(*e)->FindClass(e,"android/widget/FrameLayout$LayoutParams");jmethodID ctor=lc?(*e)->GetMethodID(e,lc,"<init>","(II)V"):0;jobject lp=ctor?(*e)->NewObject(e,lc,ctor,w,h):0;if(lp&&lc){jmethodID sm=(*e)->GetMethodID(e,lc,"setMargins","(IIII)V");if(sm)(*e)->CallVoidMethod(e,lp,sm,x,y,0,0);}jclass hc=(*e)->GetObjectClass(e,NU.host);jmethodID add=hc?(*e)->GetMethodID(e,hc,"addView","(Landroid/view/View;Landroid/view/ViewGroup$LayoutParams;)V"):0;if(add&&lp)(*e)->CallVoidMethod(e,NU.host,add,local,lp);nui_exc(e);if(dst)*dst=(*e)->NewGlobalRef(e,local);if(hc)(*e)->DeleteLocalRef(e,hc);if(lp)(*e)->DeleteLocalRef(e,lp);if(lc)(*e)->DeleteLocalRef(e,lc);(*e)->DeleteLocalRef(e,local);}
static void nui_button_style(JNIEnv*e,jobject v,int active,int danger){uint32_t ac=danger?C_DANGER:C_ACCENT;uint32_t fill=mix_color(C_GLASS2,ac,active?14:3);uint32_t edge=active?ac:mix_color(C_BORDER,ac,6);nui_bg(e,v,fill,248,edge,active?210:132,maxi(1,us(1)),(float)us(14),0);nui_text_color(e,v,active?ac:C_TEXT,255);}
static jobject nui_gradient(JNIEnv*e,const jint*colors,int n,int vertical,float radius){jclass c=(*e)->FindClass(e,"android/graphics/drawable/GradientDrawable");if(!c||n<2){nui_exc(e);return 0;}jmethodID ctor=(*e)->GetMethodID(e,c,"<init>","()V");jobject d=ctor?(*e)->NewObject(e,c,ctor):0;if(!d){(*e)->DeleteLocalRef(e,c);return 0;}jintArray ar=(*e)->NewIntArray(e,n);if(ar)(*e)->SetIntArrayRegion(e,ar,0,n,colors);jmethodID sc=(*e)->GetMethodID(e,c,"setColors","([I)V");if(sc&&ar)(*e)->CallVoidMethod(e,d,sc,ar);jclass oc=(*e)->FindClass(e,"android/graphics/drawable/GradientDrawable$Orientation");if(oc){jfieldID f=(*e)->GetStaticFieldID(e,oc,vertical?"TOP_BOTTOM":"LEFT_RIGHT","Landroid/graphics/drawable/GradientDrawable$Orientation;");jobject o=f?(*e)->GetStaticObjectField(e,oc,f):0;jmethodID so=(*e)->GetMethodID(e,c,"setOrientation","(Landroid/graphics/drawable/GradientDrawable$Orientation;)V");if(so&&o)(*e)->CallVoidMethod(e,d,so,o);if(o)(*e)->DeleteLocalRef(e,o);(*e)->DeleteLocalRef(e,oc);}jmethodID cr=(*e)->GetMethodID(e,c,"setCornerRadius","(F)V");if(cr)(*e)->CallVoidMethod(e,d,cr,radius);nui_exc(e);if(ar)(*e)->DeleteLocalRef(e,ar);(*e)->DeleteLocalRef(e,c);return d;}
static jobject nui_layer_two(JNIEnv*e,jobject a,jobject b){jclass dc=(*e)->FindClass(e,"android/graphics/drawable/Drawable");jobjectArray ar=dc?(*e)->NewObjectArray(e,2,dc,0):0;if(ar){(*e)->SetObjectArrayElement(e,ar,0,a);(*e)->SetObjectArrayElement(e,ar,1,b);}jclass lc=(*e)->FindClass(e,"android/graphics/drawable/LayerDrawable");jmethodID ctor=lc?(*e)->GetMethodID(e,lc,"<init>","([Landroid/graphics/drawable/Drawable;)V"):0;jobject out=(ctor&&ar)?(*e)->NewObject(e,lc,ctor,ar):0;nui_exc(e);if(ar)(*e)->DeleteLocalRef(e,ar);if(lc)(*e)->DeleteLocalRef(e,lc);if(dc)(*e)->DeleteLocalRef(e,dc);return out;}
static void nui_update_sv(JNIEnv*e){uint32_t hue=hsv_rgb(G.pickerHue,1,1);jint hcols[2]={nui_argb(0xffffff,255),nui_argb(hue,255)},vcols[2]={nui_argb(0,0),nui_argb(0,255)};jobject h=nui_gradient(e,hcols,2,0,(float)us(12)),v=nui_gradient(e,vcols,2,1,(float)us(12)),l=(h&&v)?nui_layer_two(e,h,v):0;if(l){nui_set_bg(e,NU.colorSV,l);(*e)->DeleteLocalRef(e,l);}if(h)(*e)->DeleteLocalRef(e,h);if(v)(*e)->DeleteLocalRef(e,v);NU.lastHue=G.pickerHue;}
static void nui_create(void){if(NU.ready||!G.activity||!G.activity->env||G.screenW<=0||G.screenH<=0)return;JNIEnv*e=G.activity->env;memset(&NU,0,sizeof(NU));NU.lastHue=-10;if(!nui_host_create(e))return;int m=margin_ui();jobject v=nui_make_text(e,"",1,17);nui_attach(e,v,m+us(5),m+us(5),us(26),us(26),&NU.mark);nui_bg(e,NU.mark,C_ACCENT,255,C_ACCENT,255,0,(float)us(8),0);v=nui_make_text(e,"Vast",ts(3),19);nui_attach(e,v,m+us(38),m-us(2),us(160),us(38),&NU.title);nui_text_color(e,NU.title,C_TEXT,255);v=nui_make_text(e,"Project",ts(1),19);nui_attach(e,v,m+us(38),m+us(32),us(250),us(28),&NU.project);nui_text_color(e,NU.project,C_MUTED,255);const char*hn[5]={"Search","Undo","Redo","Projects","Focus"};int hw[5]={112,80,80,126,92},xx=G.screenW-m-us(612);for(int i=0;i<5;i++){v=nui_make_text(e,hn[i],ts(1),17);nui_attach(e,v,xx,m,us(hw[i]),us(44),&NU.header[i]);nui_button_style(e,NU.header[i],0,0);xx+=us(hw[i])+us(8);}for(int i=0;i<6;i++){v=nui_make_text(e,"",1,17);nui_attach(e,v,m+us(46+i*34)-us(12),m+us(68),us(24),us(24),&NU.recent[i]);}
int rx,ry,rw,rh,rg;rail_geom(&rx,&ry,&rw,&rh,&rg);v=nui_make_text(e,"",1,17);nui_attach(e,v,rx-us(9),ry-us(10),rw+us(18),8*rh+7*rg+us(20),&NU.railBg);nui_bg(e,NU.railBg,mix_color(C_GLASS,C_ACCENT,2),250,mix_color(C_BORDER,C_ACCENT,5),148,maxi(1,us(1)),(float)us(22),0);for(int i=0;i<8;i++){v=nui_make_text(e,rail_name(i),ts(1),17);nui_attach(e,v,rx,ry+i*(rh+rg),rw,rh,&NU.rail[i]);nui_button_style(e,NU.rail[i],0,0);}v=nui_make_text(e,"›",ts(3),17);nui_attach(e,v,0,G.screenH/2-us(34),us(44),us(68),&NU.railHandle);nui_bg(e,NU.railHandle,C_GLASS2,235,C_BORDER,108,maxi(1,us(1)),(float)us(28),0);nui_text_color(e,NU.railHandle,C_TEXT,255);v=nui_make_text(e,"P",ts(2),17);nui_attach(e,v,m,G.screenH-m-us(56),us(56),us(56),&NU.inkToggle);
int bx,by,bw,bh,tab,px,py,pw,ph,sx,sy,sw,sh,hx,hy,hhw,hhh;color_drawer_geom(&bx,&by,&bw,&bh,&tab);picker_geometry(&px,&py,&pw,&ph,&sx,&sy,&sw,&sh,&hx,&hy,&hhw,&hhh);v=nui_make_text(e,"",1,17);nui_attach(e,v,px,py,pw,ph,&NU.colorBg);nui_bg(e,NU.colorBg,mix_color(C_GLASS,C_ACCENT,2),252,mix_color(C_BORDER,C_ACCENT,5),150,maxi(1,us(1)),(float)us(22),0);v=nui_make_text(e,"Color",ts(3),19);nui_attach(e,v,px+us(16),py+us(12),us(180),us(54),&NU.colorTitle);nui_text_color(e,NU.colorTitle,C_TEXT,255);v=nui_make_text(e,"Done",ts(1),17);nui_attach(e,v,px+pw-us(108),py+us(18),us(88),us(40),&NU.colorClose);nui_button_style(e,NU.colorClose,0,0);v=nui_make_text(e,"",1,17);nui_attach(e,v,sx,sy,sw,sh,&NU.colorSV);v=nui_make_text(e,"",1,17);nui_attach(e,v,hx,hy,hhw,hhh,&NU.colorHue);jint rb[7]={nui_argb(0xff0000,255),nui_argb(0xffff00,255),nui_argb(0x00ff00,255),nui_argb(0x00ffff,255),nui_argb(0x0000ff,255),nui_argb(0xff00ff,255),nui_argb(0xff0000,255)};jobject gd=nui_gradient(e,rb,7,0,(float)us(12));if(gd){nui_set_bg(e,NU.colorHue,gd);(*e)->DeleteLocalRef(e,gd);}v=nui_make_text(e,"",1,17);nui_attach(e,v,px+us(26),py+ph-us(49),us(30),us(30),&NU.colorPreview);v=nui_make_text(e,"#FFFFFF",ts(1),19);nui_attach(e,v,px+us(60),py+ph-us(52),us(130),us(34),&NU.colorHex);nui_text_color(e,NU.colorHex,C_TEXT,255);v=nui_make_text(e,"",1,17);nui_attach(e,v,bx,by+bh/2-us(56),tab+us(2),us(112),&NU.colorTab);nui_bg(e,NU.colorTab,C_GLASS2,238,C_BORDER,116,maxi(1,us(1)),(float)us(27),0);v=nui_make_text(e,"",1,17);nui_attach(e,v,bx+tab/2-us(11),by+bh/2-us(11),us(22),us(22),&NU.colorTabDot);v=nui_make_text(e,"",1,17);nui_attach(e,v,sx-us(9),sy-us(9),us(18),us(18),&NU.colorCursor);nui_bg(e,NU.colorCursor,0,0,C_TEXT,255,maxi(1,us(2)),(float)us(9),1);v=nui_make_text(e,"",1,17);nui_attach(e,v,hx-us(3),hy-us(5),us(6),hhh+us(10),&NU.colorHueCursor);nui_bg(e,NU.colorHueCursor,C_TEXT,255,C_BLACK,120,maxi(1,us(1)),(float)us(3),0);
int rcy=py+ph-us(58),rcx=px+pw-us(196);for(int i=0;i<6;i++){v=nui_make_text(e,"",1,17);nui_attach(e,v,rcx+i*us(28)-us(10),rcy-us(10),us(20),us(20),&NU.colorRecent[i]);}
int spx=G.screenW-m-us(600),spy=m+us(82);v=nui_make_text(e,"",ts(2),19);nui_attach(e,v,spx+us(40),spy+us(90),us(520),us(56),&NU.searchQuery);nui_text_color(e,NU.searchQuery,C_TEXT,255);for(int i=0;i<6;i++){v=nui_make_text(e,"",ts(1),19);nui_attach(e,v,spx+us(40),spy+us(210)+i*us(62),us(520),us(50),&NU.searchResult[i]);nui_text_color(e,NU.searchResult[i],C_TEXT,255);}
nui_update_sv(e);NU.ready=1;}
static int nui_modal(void){int in=G.editorOpen&&(G.editorMode==2||G.editorMode==5);return G.settingsPanel||G.galleryOpen||G.calibrationOpen||(G.editorOpen&&!in)||G.projectsPanel;}
static uint64_t nui_hash_mix(uint64_t h,uint64_t v){h^=v+0x9e3779b97f4a7c15ULL+(h<<6)+(h>>2);return h;}
static uint64_t nui_hash_text(uint64_t h,const char*s){for(int i=0;s&&s[i];i++)h=nui_hash_mix(h,(uint8_t)s[i]);return h;}
static uint64_t nui_search_hash(void){uint64_t h=nui_hash_text(0x83d2e910ULL,G.searchQuery);h=nui_hash_mix(h,(uint32_t)G.searchVisibleN);for(int i=0;i<G.searchVisibleN&&i<6;i++){ObjRef r=G.searchVisible[i];h=nui_hash_mix(h,(uint32_t)r.type);h=nui_hash_mix(h,(uint32_t)r.index);if(r.type==SEL_NOTE&&r.index>=0&&r.index<G.noteN)h=nui_hash_text(h,G.notes[r.index].text);else if(r.type==SEL_FRAME&&r.index>=0&&r.index<G.frameN)h=nui_hash_text(h,G.frameNames[r.index]);else if(r.type==SEL_BOOKMARK&&r.index>=0&&r.index<G.bookmarkN)h=nui_hash_text(h,G.bookmarks[r.index].name);else if(r.type==SEL_HANDWRITING&&r.index>=0&&r.index<G.ocrSearchHitN)h=nui_hash_text(h,G.ocrSearchHits[r.index].label);}return h;}
static uint64_t nui_state_hash(void){uint64_t h=0xcbf29ce484222325ULL;h=nui_hash_mix(h,(uint32_t)G.screenW);h=nui_hash_mix(h,(uint32_t)G.screenH);h=nui_hash_mix(h,(uint32_t)(G.uiScale*1000));h=nui_hash_mix(h,(uint32_t)G.tool);h=nui_hash_mix(h,(uint32_t)G.color);h=nui_hash_mix(h,(uint32_t)(G.pickerHue*10000));h=nui_hash_mix(h,(uint32_t)(G.pickerSat*10000));h=nui_hash_mix(h,(uint32_t)(G.pickerVal*10000));h=nui_hash_mix(h,(uint32_t)G.railOpen);h=nui_hash_mix(h,(uint32_t)G.colorPickerOpen);h=nui_hash_mix(h,(uint32_t)G.zenMode);h=nui_hash_mix(h,(uint32_t)G.presentationMode);h=nui_hash_mix(h,(uint32_t)G.settingsPanel);h=nui_hash_mix(h,(uint32_t)G.galleryOpen);h=nui_hash_mix(h,(uint32_t)G.calibrationOpen);h=nui_hash_mix(h,(uint32_t)G.projectsPanel);h=nui_hash_mix(h,(uint32_t)G.addPanel);h=nui_hash_mix(h,(uint32_t)G.framePanel);h=nui_hash_mix(h,(uint32_t)G.photoMode);h=nui_hash_mix(h,(uint32_t)G.searchPanel);h=nui_hash_mix(h,(uint32_t)G.layersPanel);h=nui_hash_mix(h,(uint32_t)G.minimap);h=nui_hash_mix(h,(uint32_t)G.editorOpen);h=nui_hash_mix(h,(uint32_t)G.editorMode);h=nui_hash_mix(h,(uint32_t)G.projectIndex);h=nui_hash_mix(h,(uint32_t)G.recentColorN);for(int i=0;i<6;i++)h=nui_hash_mix(h,G.recentColors[i]);h=nui_hash_text(h,G.projectNames[G.projectIndex]);h=nui_hash_mix(h,C_BG);h=nui_hash_mix(h,C_GLASS2);h=nui_hash_mix(h,C_BORDER);h=nui_hash_mix(h,C_TEXT);h=nui_hash_mix(h,C_ACCENT);return nui_hash_mix(h,nui_search_hash());}
static uint64_t nui_theme_hash(void){uint64_t h=0x5f3759dfULL;h=nui_hash_mix(h,C_BG);h=nui_hash_mix(h,C_GLASS);h=nui_hash_mix(h,C_GLASS2);h=nui_hash_mix(h,C_GLASS3);h=nui_hash_mix(h,C_BORDER);h=nui_hash_mix(h,C_TEXT);h=nui_hash_mix(h,C_MUTED);h=nui_hash_mix(h,C_ACCENT);h=nui_hash_mix(h,C_DANGER);return h;}
static uint64_t nui_geom_hash(void){uint64_t h=0x1234abcdULL;h=nui_hash_mix(h,(uint32_t)G.screenW);h=nui_hash_mix(h,(uint32_t)G.screenH);h=nui_hash_mix(h,(uint32_t)(G.uiScale*1000));return h;}
static void nui_apply_theme(JNIEnv*e){
    nui_bg(e,NU.mark,C_ACCENT,255,C_ACCENT,255,0,(float)us(8),0);nui_text_color(e,NU.title,C_TEXT,255);nui_text_color(e,NU.project,C_MUTED,255);
    for(int i=0;i<5;i++){int active=(i==0&&G.searchPanel)||(i==3&&G.projectsPanel)||(i==4&&G.zenMode);nui_button_style(e,NU.header[i],active,0);}
    nui_bg(e,NU.railBg,mix_color(C_GLASS,C_ACCENT,2),250,mix_color(C_BORDER,C_ACCENT,5),148,maxi(1,us(1)),(float)us(22),0);nui_elevation(e,NU.railBg,(float)us(3));
    for(int i=0;i<8;i++)nui_button_style(e,NU.rail[i],rail_active(i),0);
    nui_bg(e,NU.railHandle,C_GLASS2,240,C_BORDER,118,maxi(1,us(1)),(float)us(28),0);nui_text_color(e,NU.railHandle,C_TEXT,255);
    nui_bg(e,NU.colorBg,mix_color(C_GLASS,C_ACCENT,2),252,mix_color(C_BORDER,C_ACCENT,5),150,maxi(1,us(1)),(float)us(22),0);nui_elevation(e,NU.colorBg,(float)us(4));nui_text_color(e,NU.colorTitle,C_TEXT,255);nui_button_style(e,NU.colorClose,0,0);nui_text_color(e,NU.colorHex,C_TEXT,255);
    nui_bg(e,NU.colorTab,C_GLASS2,242,C_BORDER,120,maxi(1,us(1)),(float)us(27),0);nui_bg(e,NU.colorCursor,0,0,C_TEXT,255,maxi(1,us(2)),(float)us(9),1);nui_bg(e,NU.colorHueCursor,C_TEXT,255,C_TEXT,255,0,(float)us(2),0);
    nui_text_color(e,NU.searchQuery,C_TEXT,255);for(int i=0;i<6;i++)nui_text_color(e,NU.searchResult[i],C_TEXT,255);
    NU.lastHue=-10.0f;
}

static void nui_sync(void){
    if(!G.activity||!G.activity->env||G.screenW<=0||G.screenH<=0)return;
    if(android_ui_available()){if(NU.ready)nui_destroy();return;}
    if(!NU.ready)nui_create();if(!NU.ready)return;
    nui_vis(G.activity->env,NU.host,!android_ui_modal());if(android_ui_modal())return;
    uint64_t shellHash=nui_state_hash(),themeHash=nui_theme_hash(),geomHash=nui_geom_hash();
    int railMoving=fabsf(G.railVel)>.001f;
    int colorMoving=G.colorRailDragging||fabsf(G.colorRailVel)>.001f;
    int moving=railMoving||colorMoving;
    int stateChanged=shellHash!=NU.stateHash,themeChanged=themeHash!=NU.themeHash,geomChanged=geomHash!=NU.geomHash;
    if(!stateChanged&&!themeChanged&&!geomChanged&&!moving)return;
    if(stateChanged)NU.stateHash=shellHash;if(themeChanged)NU.themeHash=themeHash;if(geomChanged)NU.geomHash=geomHash;
    JNIEnv*e=G.activity->env;
    int modal=nui_modal(),headerShow=!G.presentationMode,full=headerShow&&(!G.zenMode||modal||(G.editorOpen&&(G.editorMode==2||G.editorMode==5))),railShow=full&&!modal&&!G.zenMode;
    int m=margin_ui();

    if(themeChanged)nui_apply_theme(e);

    if(geomChanged){
        nui_frame(e,NU.mark,m+us(5),m+us(5),us(26),us(26));
        nui_frame(e,NU.title,m+us(38),m-us(2),us(160),us(38));nui_text_size(e,NU.title,(float)ts(3));
        nui_frame(e,NU.project,m+us(38),m+us(32),us(250),us(28));nui_text_size(e,NU.project,(float)ts(1));
        int hw[5]={112,80,80,126,92},xx=G.screenW-m-us(612);for(int i=0;i<5;i++){nui_frame(e,NU.header[i],xx,m,us(hw[i]),us(44));nui_text_size(e,NU.header[i],(float)ts(1));xx+=us(hw[i])+us(8);}
        for(int i=0;i<6;i++)nui_frame(e,NU.recent[i],m+us(46+i*34)-us(12),m+us(68),us(24),us(24));
        int rx,ry,rw,rh,rg;rail_geom(&rx,&ry,&rw,&rh,&rg);nui_frame(e,NU.railBg,rx-us(9),ry-us(10),rw+us(18),8*rh+7*rg+us(20));for(int i=0;i<8;i++){nui_frame(e,NU.rail[i],rx,ry+i*(rh+rg),rw,rh);nui_text_size(e,NU.rail[i],(float)ts(1));}
        nui_frame(e,NU.railHandle,0,G.screenH/2-us(34),us(44),us(68));nui_text_size(e,NU.railHandle,(float)ts(3));
        nui_frame(e,NU.inkToggle,m,G.screenH-m-us(56),us(56),us(56));nui_text_size(e,NU.inkToggle,(float)ts(2));
        int bx,by,bw,bh,tab,px,py,pw,ph,sx,sy,sw,sh,hx,hy,hhw,hhh;color_drawer_geom_for(1.0f,&bx,&by,&bw,&bh,&tab);picker_geometry_for(1.0f,&px,&py,&pw,&ph,&sx,&sy,&sw,&sh,&hx,&hy,&hhw,&hhh);
        nui_frame(e,NU.colorBg,px,py,pw,ph);nui_frame(e,NU.colorTitle,px+us(14),py+us(10),us(180),us(54));nui_text_size(e,NU.colorTitle,(float)ts(3));nui_frame(e,NU.colorClose,px+pw-us(100),py+us(16),us(82),us(40));nui_text_size(e,NU.colorClose,(float)ts(1));nui_frame(e,NU.colorSV,sx,sy,sw,sh);nui_frame(e,NU.colorHue,hx,hy,hhw,hhh);nui_frame(e,NU.colorPreview,px+us(22),py+ph-us(76),us(40),us(40));nui_frame(e,NU.colorHex,px+us(70),py+ph-us(73),us(126),us(34));nui_text_size(e,NU.colorHex,(float)ts(1));nui_frame(e,NU.colorTab,bx,by+bh/2-us(56),tab+us(2),us(112));nui_frame(e,NU.colorTabDot,bx+tab/2-us(11),by+bh/2-us(11),us(22),us(22));
        nui_frame(e,NU.colorCursor,sx-us(10),sy-us(10),us(20),us(20));nui_frame(e,NU.colorHueCursor,hx-us(3),hy-us(5),us(6),hhh+us(10));int rcy=py+ph-us(56),rcx=px+pw-us(196);for(int i=0;i<6;i++)nui_frame(e,NU.colorRecent[i],rcx+i*us(28)-us(10),rcy-us(10),us(20),us(20));
        int spx=G.screenW-m-us(600),spy=m+us(82);nui_frame(e,NU.searchQuery,spx+us(40),spy+us(90),us(520),us(56));nui_text_size(e,NU.searchQuery,(float)ts(2));for(int i=0;i<6;i++){nui_frame(e,NU.searchResult[i],spx+us(40),spy+us(210)+i*us(62),us(520),us(50));nui_text_size(e,NU.searchResult[i],(float)ts(1));}
    }

    if(stateChanged||themeChanged){
        nui_vis(e,NU.mark,headerShow);nui_vis(e,NU.title,headerShow);nui_vis(e,NU.project,headerShow);nui_text(e,NU.project,G.projectNames[G.projectIndex][0]?G.projectNames[G.projectIndex]:"Project");
        nui_text_color(e,NU.title,C_TEXT,255);nui_text_color(e,NU.project,C_MUTED,255);
        for(int i=0;i<5;i++)nui_vis(e,NU.header[i],full);
        for(int i=0;i<5;i++){int active=(i==0&&G.searchPanel)||(i==3&&G.projectsPanel)||(i==4&&G.zenMode);nui_button_style(e,NU.header[i],active,0);}
        for(int i=0;i<6;i++){int show=full&&i<G.recentColorN;nui_vis(e,NU.recent[i],show);if(show)nui_bg(e,NU.recent[i],G.recentColors[i],255,C_BORDER,150,maxi(1,us(1)),(float)us(12),1);}
        for(int i=0;i<8;i++)nui_button_style(e,NU.rail[i],rail_active(i),0);
        int tog=!G.presentationMode&&!G.settingsPanel&&!G.galleryOpen&&!G.calibrationOpen&&!G.projectsPanel&&!G.editorOpen;nui_vis(e,NU.inkToggle,tog);int er=G.tool==MODE_ERASE||G.tool==MODE_ERASE_ALL;nui_text(e,NU.inkToggle,er?"E":"P");nui_bg(e,NU.inkToggle,C_GLASS2,245,er?C_DANGER:C_ACCENT,210,maxi(1,us(2)),(float)us(28),1);nui_text_color(e,NU.inkToggle,er?C_DANGER:C_ACCENT,255);
        nui_bg(e,NU.colorPreview,G.color,255,C_BORDER,170,maxi(1,us(1)),(float)us(20),1);nui_bg(e,NU.colorTabDot,G.color,255,C_BORDER,170,maxi(1,us(1)),(float)us(11),1);char q[16];snprintf(q,sizeof(q),"#%06X",G.color&0xffffffu);nui_text(e,NU.colorHex,q);
        for(int i=0;i<6;i++){int show=i<G.recentColorN;nui_vis(e,NU.colorRecent[i],show);if(show)nui_bg(e,NU.colorRecent[i],G.recentColors[i],255,((G.color&0xffffffu)==(G.recentColors[i]&0xffffffu))?C_TEXT:C_BORDER,190,maxi(1,us(1)),(float)us(10),1);}
        if(fabsf(NU.lastHue-G.pickerHue)>.0035f)nui_update_sv(e);
        int px,py,pw,ph,sx,sy,sw,sh,hx,hy,hhw,hhh;picker_geometry_for(1.0f,&px,&py,&pw,&ph,&sx,&sy,&sw,&sh,&hx,&hy,&hhw,&hhh);nui_tx(e,NU.colorCursor,G.pickerSat*sw);nui_ty(e,NU.colorCursor,(1.0f-G.pickerVal)*sh);nui_tx(e,NU.colorHueCursor,G.pickerHue*hhw);nui_ty(e,NU.colorHueCursor,0);
        int showSearch=G.searchPanel&&!(G.editorOpen&&G.editorMode==3);char sq[128];snprintf(sq,sizeof(sq),G.searchQuery[0]?"Query: %s":"Tap to enter a search",G.searchQuery);nui_vis(e,NU.searchQuery,showSearch);if(showSearch)nui_text(e,NU.searchQuery,sq);for(int i=0;i<6;i++){int show=showSearch&&i<G.searchVisibleN;nui_vis(e,NU.searchResult[i],show);if(show){char label[256];search_result_label(G.searchVisible[i],label,(int)sizeof(label));nui_text(e,NU.searchResult[i],label);nui_text_color(e,NU.searchResult[i],G.searchVisible[i].type==SEL_HANDWRITING?C_CYAN:C_TEXT,255);}}
    }

    /* Animation-only path: translate Views instead of forcing Android layout every 8ms. */
    if(railMoving||stateChanged||geomChanged){
        int rx,ry,rw,rh,rg;rail_geom(&rx,&ry,&rw,&rh,&rg);float ra=clampf(G.railAnim,0,1),rm=clampf(G.railAnim,-.10f,1.14f);float rtx=-(1.0f-rm)*(rw+us(40));
        float rva=smoothstepf(0.0f,.28f,ra);nui_vis(e,NU.railBg,railShow&&ra>.025f);nui_alpha(e,NU.railBg,rva);nui_tx(e,NU.railBg,rtx);for(int i=0;i<8;i++){nui_vis(e,NU.rail[i],railShow&&ra>.025f);nui_alpha(e,NU.rail[i],rva);nui_tx(e,NU.rail[i],rtx);}nui_vis(e,NU.railHandle,railShow&&ra<.72f);nui_alpha(e,NU.railHandle,1-ra*.75f);
    }

    if(colorMoving||stateChanged||geomChanged){
        int allow=!G.presentationMode&&!G.settingsPanel&&!G.galleryOpen&&!G.calibrationOpen&&!G.projectsPanel&&(!G.editorOpen||G.editorMode==2||G.editorMode==5);float ca=clampf(G.colorRailAnim,0,1);float ctx=(float)(color_drawer_w()-color_handle_w())*(1.0f-ca);int open=allow&&ca>.015f;float panelAlpha=clampf((ca-.02f)/.32f,0.0f,1.0f);jobject panel[7]={NU.colorBg,NU.colorTitle,NU.colorClose,NU.colorSV,NU.colorHue,NU.colorPreview,NU.colorHex};for(int i=0;i<7;i++){nui_vis(e,panel[i],open);nui_alpha(e,panel[i],panelAlpha);nui_tx(e,panel[i],ctx);}nui_vis(e,NU.colorCursor,open);nui_vis(e,NU.colorHueCursor,open);nui_alpha(e,NU.colorCursor,panelAlpha);nui_alpha(e,NU.colorHueCursor,panelAlpha);int px,py,pw,ph,sx,sy,sw,sh,hx,hy,hhw,hhh;picker_geometry_for(1.0f,&px,&py,&pw,&ph,&sx,&sy,&sw,&sh,&hx,&hy,&hhw,&hhh);nui_tx(e,NU.colorCursor,ctx+G.pickerSat*sw);nui_ty(e,NU.colorCursor,(1.0f-G.pickerVal)*sh);nui_tx(e,NU.colorHueCursor,ctx+G.pickerHue*hhw);nui_ty(e,NU.colorHueCursor,0);for(int i=0;i<6;i++){int rs=open&&i<G.recentColorN;nui_vis(e,NU.colorRecent[i],rs);nui_alpha(e,NU.colorRecent[i],panelAlpha);nui_tx(e,NU.colorRecent[i],ctx);}nui_vis(e,NU.colorTab,allow);nui_vis(e,NU.colorTabDot,allow);nui_tx(e,NU.colorTab,ctx);nui_tx(e,NU.colorTabDot,ctx);
    }
    /* Android-native header remains visible, but the colour-critical rail,
       picker and pen/eraser toggle are renderer-owned in 3.2.6. Hide their
       old View counterparts so vendor tinting can never sit on top of them. */
    nui_vis(e,NU.railBg,0);nui_vis(e,NU.railHandle,0);nui_vis(e,NU.inkToggle,0);for(int i=0;i<8;i++)nui_vis(e,NU.rail[i],0);
    jobject canvasOwned[11]={NU.colorBg,NU.colorTitle,NU.colorClose,NU.colorSV,NU.colorHue,NU.colorPreview,NU.colorHex,NU.colorTab,NU.colorTabDot,NU.colorCursor,NU.colorHueCursor};for(int i=0;i<11;i++)nui_vis(e,canvasOwned[i],0);for(int i=0;i<6;i++)nui_vis(e,NU.colorRecent[i],0);
}

static void nui_destroy(void){if(!G.activity||!G.activity->env)return;JNIEnv*e=G.activity->env;if(NU.popup){jclass pc=(*e)->GetObjectClass(e,NU.popup);jmethodID d=pc?(*e)->GetMethodID(e,pc,"dismiss","()V"):0;if(d)(*e)->CallVoidMethod(e,NU.popup,d);nui_exc(e);if(pc)(*e)->DeleteLocalRef(e,pc);}jobject*refs[]={&NU.mark,&NU.title,&NU.project,&NU.railBg,&NU.railHandle,&NU.inkToggle,&NU.colorBg,&NU.colorTitle,&NU.colorClose,&NU.colorSV,&NU.colorHue,&NU.colorPreview,&NU.colorHex,&NU.colorTab,&NU.colorTabDot,&NU.colorCursor,&NU.colorHueCursor,&NU.searchQuery};for(int i=0;i<(int)(sizeof(refs)/sizeof(refs[0]));i++)if(*refs[i]){(*e)->DeleteGlobalRef(e,*refs[i]);*refs[i]=0;}for(int i=0;i<5;i++)if(NU.header[i]){(*e)->DeleteGlobalRef(e,NU.header[i]);NU.header[i]=0;}for(int i=0;i<6;i++)if(NU.recent[i]){(*e)->DeleteGlobalRef(e,NU.recent[i]);NU.recent[i]=0;}for(int i=0;i<6;i++)if(NU.colorRecent[i]){(*e)->DeleteGlobalRef(e,NU.colorRecent[i]);NU.colorRecent[i]=0;}for(int i=0;i<6;i++)if(NU.searchResult[i]){(*e)->DeleteGlobalRef(e,NU.searchResult[i]);NU.searchResult[i]=0;}for(int i=0;i<8;i++)if(NU.rail[i]){(*e)->DeleteGlobalRef(e,NU.rail[i]);NU.rail[i]=0;}if(NU.host){(*e)->DeleteGlobalRef(e,NU.host);NU.host=0;}if(NU.popup){(*e)->DeleteGlobalRef(e,NU.popup);NU.popup=0;}memset(&NU,0,sizeof(NU));}
static const float RAD_DX[6]={0.0f,0.866f,0.866f,0.0f,-0.866f,-0.866f};
static const float RAD_DY[6]={-1.0f,-0.5f,0.5f,1.0f,0.5f,-0.5f};
static const char*rad_name(int i){static const char*n[6]={"Pen","Marker","Erase","Size","Color","Select"};return (i>=0&&i<6)?n[i]:"";}
static void radial_update_hot(float x,float y){float dx=x-G.radialX,dy=y-G.radialY,d=sqrtf(dx*dx+dy*dy);if(d<us(42)){G.radialHot=-1;if(!G.radialSizeActive)G.radialSizing=0;return;}float best=-999;int bi=0;for(int i=0;i<6;i++){float q=(dx*RAD_DX[i]+dy*RAD_DY[i])/fmax2(d,1);if(q>best){best=q;bi=i;}}G.radialHot=bi;if(bi==3&&best>0.965f){if(!G.radialSizing&&d>=us(72)&&d<=us(138)){G.radialSizing=1;G.radialSizePreview=G.brush;}if(G.radialSizing&&d>us(150)){G.radialSizeActive=1;float inner=us(150),outer=us(248);float t=clampf((d-inner)/(outer-inner),0.0f,1.0f);G.brush=1.0f+t*71.0f;G.radialSizePreview=G.brush;G.metaDirty=1;}}else if(!G.radialSizeActive)G.radialSizing=0;}
static void draw_pen_hold_progress(Surf*s){if(!G.penHoldArmed||G.touchHoldMode!=1||G.fingerCount!=1||G.penHoldMoved||G.settingsPanel||G.galleryOpen||G.calibrationOpen||G.radialOpen)return;float p=clampf(G.penHoldVisual,0,1);if(p<=0.01f)return;float cx=G.penHoldX,cy=G.penHoldY,rr=us(48);alpha_circle(s,cx,cy,rr+us(6),C_BLACK,24);aa_circle(s,cx,cy,rr,C_BORDER,82);int segs=56;for(int i=0;i<segs;i++){float t0=(float)i/(float)segs,t1=(float)(i+1)/(float)segs;if(t0>=p)break;float a0=(-90.0f+t0*360.0f)*PI/180.0f,a1=(-90.0f+fmin2(t1,p)*360.0f)*PI/180.0f;float x0=cx+cosf(a0)*rr,y0=cy+sinf(a0)*rr,x1=cx+cosf(a1)*rr,y1=cy+sinf(a1)*rr;aa_capsule(s,x0,y0,us(4),x1,y1,us(4),C_ACCENT,240);}if(p>.16f){const char*q="Hold for tools";int sc=ts(1),tw=text_w(q,sc);text(s,(int)cx-tw/2,(int)(cy+rr+us(18)),q,sc,C_MUTED);}}
/* Rounded arc-distance petals, bounded to the menu's small on-screen region.
   Their material is composited once per pixel; no backdrop readback or blur. */
static void radial_petal(Surf*s,float cx,float cy,float ux,float uy,float orbit,float radius,uint32_t fill,uint32_t edge,int alpha){
    const float co=.990268f,si=.139173f;float ex0=(ux*co-uy*si)*orbit,ey0=(uy*co+ux*si)*orbit,ex1=(ux*co+uy*si)*orbit,ey1=(uy*co-ux*si)*orbit;
    float px=cx+ux*orbit,py=cy+uy*orbit,bound=radius+orbit*.15f+3;
    int x0=maxi(0,(int)(px-bound)),x1=mini(s->w-1,(int)(px+bound)),y0=maxi(0,(int)(py-bound)),y1=mini(s->h-1,(int)(py+bound));
    for(int y=y0;y<=y1;y++)for(int x=x0;x<=x1;x++){float dx=x+.5f-cx,dy=y+.5f-cy,d=sqrtf(dx*dx+dy*dy),dist;
        if(dx*ux+dy*uy>=d*co)dist=fabsf(d-orbit);else dist=sqrtf(fmin2(sq(dx-ex0)+sq(dy-ey0),sq(dx-ex1)+sq(dy-ey1)));
        float inside=radius-dist;if(inside<-.8f)continue;float cov=smoothstepf(-.8f,.8f,inside);uint32_t color=inside<1.1f?edge:fill;
        blendpx(s,x,y,color,(int)(alpha*cov));
    }
}
static void draw_radial_menu(Surf*s){
    float a=clampf(G.radialAnim,0,1);if(a<.015f)return;float e=.86f+.14f*a,cx=G.radialX,cy=G.radialY,orbit=us(108)*e,rr=us(34)*e;
    int light=((C_BG>>16)&255)>145;uint32_t panel=light?0xf2f0eb:0x32373f,edge=light?0xffffff:0x626970,ink=light?0x30343a:0xeceeea;
    for(int i=0;i<6;i++){
        int hot=i==G.radialHot;float ux=RAD_DX[i],uy=RAD_DY[i],o=orbit+(hot?us(3):0),x=cx+ux*o,y=cy+uy*o;
        radial_petal(s,cx,cy+us(4),ux,uy,o,rr+us(2),0,0,(int)(28*a));
        radial_petal(s,cx,cy,ux,uy,o,rr,hot?(light?0x363d45:0xf2f1ec):panel,hot?(light?0x60666c:0xffffff):edge,(int)((hot?250:227)*a));
        uint32_t ic=hot?(light?0xffffff:0x242a30):ink;float iy=y-us(10)*e,k=1.05f*e;
        if(i==4){aa_circle(s,x,iy,us(10)*e,ic,230);aa_circle(s,x,iy,us(7)*e,G.color,255);}
        else if(i==1){segment(s,x-us(8),iy+us(6),x+us(6),iy-us(8),us(3),ic);segment(s,x-us(9),iy+us(9),x+us(2),iy+us(9),us(1),ic);}
        else line_icon(s,i==0?0:i==2?1:i==3?6:5,x,iy,k,ic);
        const char*n=rad_name(i);int fs=maxi(1,ts(1));text(s,(int)x-text_w(n,fs)/2,(int)(y+us(11)*e),n,fs,ic);
    }
    aa_circle(s,cx,cy,us(43)*e,panel,(int)(210*a));aa_circle(s,cx,cy-us(9)*e,us(11)*e,G.color,255);
    char q[32];snprintf(q,sizeof(q),"%.1f",G.brush);int sc=ts(1);text(s,(int)cx-text_w(q,sc)/2,(int)(cy+us(12)*e),q,sc,ink);
    if(G.radialSizing){float t=clampf((G.brush-1)/71,0,1);aa_capsule(s,cx-us(45),cy+us(163),us(2),cx+us(45),cy+us(163),us(2),edge,180);aa_capsule(s,cx-us(45),cy+us(163),us(2),cx-us(45)+us(90)*t,cy+us(163),us(2),ink,255);}
}
static void radial_execute(int i){switch(i){case 0:G.tool=MODE_PEN;break;case 1:G.tool=MODE_HIGHLIGHTER;break;case 2:G.tool=MODE_ERASE;break;case 3:break;case 4:G.pickerApplySelection=0;open_color_picker();break;case 5:G.tool=MODE_SELECT;G.photoMode=0;selection_clear();break;default:break;}}
static void draw_pressure_curve(Surf*s,int x,int y,int w,int h){round_rect(s,x,y,x+w,y+h,14,C_BG);for(int i=1;i<4;i++)hline(s,y+i*h/4,x+8,x+w-8,C_GRID_MAJOR);float prevx=(float)x+6,raw=0,t=0,p=pressure_map(0),prevy=y+h-6-clampf(p/1.8f,0,1)*(h-12);for(int i=1;i<=32;i++){raw=(float)i/32.0f;t=raw;if(G.pressureCurve==0)t=sqrtf(t);else if(G.pressureCurve==2)t=t*t;p=G.pressureMin+t*(G.pressureMax-G.pressureMin);float xx=x+6+raw*(w-12),yy=y+h-6-clampf(p/1.8f,0,1)*(h-12);segment(s,prevx,prevy,xx,yy,1.7f,C_VIOLET);prevx=xx;prevy=yy;}}
static void meter(Surf*s,int x,int y,int w,float t,uint32_t c){round_rect(s,x,y,x+w,y+12,6,C_GLASS3);int fw=(int)(clampf(t,0,1)*w);if(fw>0)round_rect(s,x,y,x+fw,y+12,6,c);}
static void draw_frame_panel(Surf*s){
    if(!G.framePanel)return;int m=margin_ui(),w=us(560),x=s->w-m-w,y=m+us(74),h=mini(us(820),s->h-y-m);glass(s,x,y,x+w,y+h,us(24));
    if(G.editorOpen&&(G.editorMode==2||G.editorMode==5)){
        const char*what=G.editorMode==2?"Rename frame":"Rename place";text(s,x+us(24),y+us(22),what,ts(3),C_TEXT);text(s,x+us(24),y+us(58),"Android keyboard and hardware keyboard input",ts(1),C_MUTED);
        int fx=x+us(24),fy=y+us(92),fw=w-us(48),fh=us(58);alpha_round_rect(s,fx,fy,fx+fw,fy+fh,us(12),C_BG,245);aa_hline(s,(float)(fy+fh-1),fx+us(10),fx+fw-us(10),C_ACCENT,210);
        const char*shown=G.editorBuf[0]?G.editorBuf:"Name";text(s,fx+us(16),fy+(fh-font_line_h(ts(2)))/2,shown,ts(2),G.editorBuf[0]?C_TEXT:C_MUTED);
        text(s,x+us(24),y+us(164),"Type normally, then save. The name updates here while you type.",ts(1),C_MUTED);button(s,x+us(24),y+us(204),x+us(248),y+us(256),"Cancel",0,C_DANGER);button(s,x+us(262),y+us(204),x+w-us(24),y+us(256),"Save name",1,C_GREEN);return;
    }
    text(s,x+us(24),y+us(22),"Frames & places",ts(3),C_TEXT);text(s,x+us(24),y+us(58),"Named regions, saved views and frame navigation.",ts(1),C_MUTED);button(s,x+us(24),y+us(88),x+us(248),y+us(138),"New frame",1,C_GREEN);button(s,x+us(262),y+us(88),x+w-us(24),y+us(138),"Close",0,C_MUTED);text(s,x+us(24),y+us(164),"Canvas regions",ts(2),C_TEXT);
    int pages=maxi(1,(G.frameN+4)/5);if(G.framePage>=pages)G.framePage=pages-1;char pg[32];snprintf(pg,sizeof(pg),"%d / %d",G.framePage+1,pages);text(s,x+w-us(170),y+us(166),pg,ts(1),C_MUTED);button(s,x+w-us(112),y+us(152),x+w-us(68),y+us(190),"<",G.framePage>0,C_MUTED);button(s,x+w-us(62),y+us(152),x+w-us(18),y+us(190),">",G.framePage+1<pages,C_MUTED);
    int first=G.framePage*5,maxRows=mini(G.frameN-first,5);for(int i=0;i<maxRows;i++){int fi=first+i;FrameObj*f=&G.frames[fi];char q[112];snprintf(q,sizeof(q),"%s  -  %d objects%s",G.frameNames[fi][0]?G.frameNames[fi]:"Frame",frame_contents_count(f),G.frameSizeLocked[fi]?"  -  size locked":"");int yy=y+us(202)+i*us(54);button(s,x+us(24),yy,x+w-us(24),yy+us(46),q,fi==G.selectedFrame,C_ACCENT);}if(G.frameN==0)text(s,x+us(24),y+us(220),"Create a frame, then resize it by dragging a corner.",ts(2),C_MUTED);
    int ay=y+us(480);if(G.selectedFrame>=0&&G.selectedFrame<G.frameN){button(s,x+us(24),ay,x+us(142),ay+us(44),"Jump",1,C_CYAN);button(s,x+us(150),ay,x+us(268),ay+us(44),"Rename",0,C_TEXT);button(s,x+us(276),ay,x+us(394),ay+us(44),G.frameSizeLocked[G.selectedFrame]?"Size locked":"Size free",G.frameSizeLocked[G.selectedFrame],C_WARN);button(s,x+us(402),ay,x+w-us(24),ay+us(44),"Move",0,C_GREEN);ay+=us(52);int bw=(w-us(48))/5;const char*bn[5]={G.frameLocked[G.selectedFrame]?"Unlock":"Lock","Duplicate","Export","Present","Delete"};uint32_t bc[5]={C_WARN,C_ACCENT,C_CYAN,C_VIOLET,C_DANGER};for(int i=0;i<5;i++){int bx=x+us(24)+i*bw;button(s,bx,ay,bx+bw-us(6),ay+us(44),bn[i],i==0&&G.frameLocked[G.selectedFrame],bc[i]);}}
    text(s,x+us(24),y+us(594),"Saved places",ts(2),C_TEXT);int shown=mini(G.bookmarkN,3);for(int i=0;i<shown;i++){char q[64];snprintf(q,sizeof(q),"%s",G.bookmarks[i].name);int yy=y+us(628)+i*us(48);button(s,x+us(24),yy,x+w-us(204),yy+us(40),q,i==G.selectedBookmark,C_ACCENT);button(s,x+w-us(194),yy,x+w-us(108),yy+us(40),"Rename",0,C_TEXT);button(s,x+w-us(100),yy,x+w-us(24),yy+us(40),"Del",0,C_DANGER);}if(G.bookmarkN==0)text(s,x+us(24),y+us(636),"Add a bookmark from Add to save the current view.",ts(1),C_MUTED);
}
static void draw_frame_creation_hint(Surf*s){if(!G.frameCreating&&!G.frameMoveMode)return;const char*q=G.frameCreating?"Drag with the stylus to create a frame":"Drag inside the selected frame to move it with its contents";int fs=ts(2),tw=text_w(q,fs),x=(s->w-tw)/2,y=margin_ui()+us(18);alpha_round_rect(s,x-us(18),y-us(10),x+tw+us(18),y+font_line_h(fs)+us(10),us(12),C_GLASS2,220);text(s,x,y,q,fs,C_GREEN);}

// ---------- Vast 3 workspace UI ----------
static void start_anim_timer(void);
static int utf8_decode_one(const unsigned char*s,uint32_t*cp){
    if(!s||!s[0])return 0;unsigned char a=s[0];if(a<0x80){*cp=a;return 1;}if(a>=0xc2&&a<=0xdf&&s[1]&&((unsigned char)s[1]&0xc0)==0x80){*cp=((uint32_t)(a&0x1f)<<6)|((unsigned char)s[1]&0x3f);return 2;}if(a>=0xe0&&a<=0xef&&s[1]&&s[2]&&((unsigned char)s[1]&0xc0)==0x80&&((unsigned char)s[2]&0xc0)==0x80){unsigned char b=(unsigned char)s[1];if((a==0xe0&&b<0xa0)||(a==0xed&&b>=0xa0))return -1;*cp=((uint32_t)(a&0x0f)<<12)|((uint32_t)(b&0x3f)<<6)|((unsigned char)s[2]&0x3f);return 3;}if(a>=0xf0&&a<=0xf4&&s[1]&&s[2]&&s[3]&&((unsigned char)s[1]&0xc0)==0x80&&((unsigned char)s[2]&0xc0)==0x80&&((unsigned char)s[3]&0xc0)==0x80){unsigned char b=(unsigned char)s[1];if((a==0xf0&&b<0x90)||(a==0xf4&&b>=0x90))return -1;*cp=((uint32_t)(a&7)<<18)|((uint32_t)(b&0x3f)<<12)|((uint32_t)((unsigned char)s[2]&0x3f)<<6)|((unsigned char)s[3]&0x3f);return 4;}return -1;
}
static void copy_text_local(char*dst,int cap,const char*src){
    if(!dst||cap<=0)return;int in=0,out=0;while(src&&src[in]&&out<cap-1){uint32_t cp=0;int n=utf8_decode_one((const unsigned char*)src+in,&cp);if(n<1){dst[out++]='?';in++;continue;}if(out+n>cap-1)break;for(int k=0;k<n;k++)dst[out++]=src[in++];}dst[out]=0;
}
static void copy_utf16_to_utf8(char*dst,int cap,const jchar*src,int len){int out=0;if(!dst||cap<=0)return;for(int i=0;src&&i<len&&out<cap-1;i++){uint32_t cp=src[i];if(cp>=0xd800&&cp<=0xdbff&&i+1<len&&src[i+1]>=0xdc00&&src[i+1]<=0xdfff){cp=0x10000u+((cp-0xd800u)<<10)+(src[++i]-0xdc00u);}else if(cp>=0xd800&&cp<=0xdfff)cp='?';unsigned char b[4];int n;if(cp<0x80){b[0]=(unsigned char)cp;n=1;}else if(cp<0x800){b[0]=(unsigned char)(0xc0|(cp>>6));b[1]=(unsigned char)(0x80|(cp&0x3f));n=2;}else if(cp<0x10000){b[0]=(unsigned char)(0xe0|(cp>>12));b[1]=(unsigned char)(0x80|((cp>>6)&0x3f));b[2]=(unsigned char)(0x80|(cp&0x3f));n=3;}else{b[0]=(unsigned char)(0xf0|(cp>>18));b[1]=(unsigned char)(0x80|((cp>>12)&0x3f));b[2]=(unsigned char)(0x80|((cp>>6)&0x3f));b[3]=(unsigned char)(0x80|(cp&0x3f));n=4;}if(out+n>cap-1)break;for(int k=0;k<n;k++)dst[out++]=(char)b[k];}dst[out]=0;}
static int copy_utf8_to_utf16(jchar*out,int cap,const char*src){int in=0,n=0;if(!out||cap<=0)return 0;while(src&&src[in]&&n<cap){uint32_t cp=0;int used=utf8_decode_one((const unsigned char*)src+in,&cp);if(used<1){cp='?';used=1;}if(cp<=0xffff){out[n++]=(jchar)cp;}else{if(n+2>cap)break;cp-=0x10000u;out[n++]=(jchar)(0xd800u+(cp>>10));out[n++]=(jchar)(0xdc00u+(cp&0x3ff));}in+=used;}return n;}
#include "android_ui_editor.inc"
#include "note_text.inc"
static char upper_local(char c){if(c>='a'&&c<='z')return(char)(c-'a'+'A');return c;}
static int contains_ci(const char*hay,const char*needle){if(!needle||!needle[0])return 1;for(int i=0;hay&&hay[i];i++){int j=0;while(needle[j]&&hay[i+j]&&upper_local(hay[i+j])==upper_local(needle[j]))j++;if(!needle[j])return 1;}return 0;}
static void close_workspace_panels(void){G.addPanel=G.layersPanel=G.searchPanel=G.bookmarksPanel=G.projectsPanel=0;G.framePanel=0;}
static int text_equal_local(const char*a,const char*b){int i=0;if(!a)a="";if(!b)b="";while(a[i]&&b[i]&&a[i]==b[i])i++;return a[i]==b[i];}
static void note_autosize(NoteObj*n){
    if(!n||!n->active)return;
    NoteTextCache*layout=note_text_layout(n->text);if(layout){n->w=(float)layout->w+24;n->h=(float)layout->h+20;return;}
    const char*t=n->text;int total=0,longest=0,line=0,explicitLines=1;for(int i=0;t&&t[i];i++){unsigned char c=(unsigned char)t[i];if(c=='\n'){if(line>longest)longest=line;line=0;explicitLines++;continue;}if((c&0xc0)!=0x80){line++;total++;}}if(line>longest)longest=line;if(total<=0){n->w=48.0f;n->h=44.0f;return;}
    /* Short labels grow horizontally; longer notes stop at a readable width
       and then grow vertically. Dimensions live in canvas/world units. */
    int natural=maxi(1,longest)*10+30;float w=clampf((float)natural,48.0f,504.0f);int per=maxi(12,(int)((w-28.0f)/10.0f));int lines=0,run=0;for(int i=0;;i++){unsigned char c=(unsigned char)t[i];if(c==0||c=='\n'){lines+=maxi(1,(run+per-1)/per);run=0;if(c==0)break;continue;}if((c&0xc0)!=0x80)run++;}lines=maxi(lines,explicitLines);n->w=w;n->h=fmax2(44.0f,26.0f+(float)lines*27.0f);
}
static void editor_live_apply(void){
    if(!G.editorOpen)return;
    if(G.editorMode==1&&G.editorTarget>=0&&G.editorTarget<G.noteN){if(!text_equal_local(G.notes[G.editorTarget].text,G.editorBuf)){copy_text_local(G.notes[G.editorTarget].text,NOTE_TEXT_CAP,G.editorBuf);note_autosize(&G.notes[G.editorTarget]);G.sceneRevision++;G.minimapDirty=1;}}
    else if(G.editorMode==2&&G.editorTarget>=0&&G.editorTarget<G.frameN){if(!text_equal_local(G.frameNames[G.editorTarget],G.editorBuf)){copy_text_local(G.frameNames[G.editorTarget],32,G.editorBuf);G.frameRevision++;G.sceneRevision++;G.minimapDirty=1;}}
    else if(G.editorMode==3){if(!text_equal_local(G.searchQuery,G.editorBuf)){copy_text_local(G.searchQuery,64,G.editorBuf);G.searchLen=str_len_local(G.searchQuery);}}
    else if(G.editorMode==4&&G.editorTarget>=0&&G.editorTarget<4){if(!text_equal_local(G.projectNames[G.editorTarget],G.editorBuf))copy_text_local(G.projectNames[G.editorTarget],32,G.editorBuf);}
    else if(G.editorMode==5&&G.editorTarget>=0&&G.editorTarget<G.bookmarkN){if(!text_equal_local(G.bookmarks[G.editorTarget].name,G.editorBuf)){copy_text_local(G.bookmarks[G.editorTarget].name,32,G.editorBuf);G.sceneRevision++;}}
}
static void editor_restore_original(void){
    if(G.editorMode==1&&G.editorTarget>=0&&G.editorTarget<G.noteN){copy_text_local(G.notes[G.editorTarget].text,NOTE_TEXT_CAP,G.editorOriginal);note_autosize(&G.notes[G.editorTarget]);}
    else if(G.editorMode==2&&G.editorTarget>=0&&G.editorTarget<G.frameN){copy_text_local(G.frameNames[G.editorTarget],32,G.editorOriginal);G.frameRevision++;}
    else if(G.editorMode==3){copy_text_local(G.searchQuery,64,G.editorOriginal);G.searchLen=str_len_local(G.searchQuery);}
    else if(G.editorMode==4&&G.editorTarget>=0&&G.editorTarget<4)copy_text_local(G.projectNames[G.editorTarget],32,G.editorOriginal);
    else if(G.editorMode==5&&G.editorTarget>=0&&G.editorTarget<G.bookmarkN)copy_text_local(G.bookmarks[G.editorTarget].name,32,G.editorOriginal);
    G.sceneRevision++;G.minimapDirty=1;
}
static void editor_cancel(void){
    int mode=G.editorMode;
    if(G.editorCreated&&G.editorMode==1&&G.editorTarget>=0&&G.editorTarget<G.noteN){
        for(int i=G.editorTarget+1;i<G.noteN;i++)G.notes[i-1]=G.notes[i];
        G.noteN--;if(G.noteN>=0)memset(&G.notes[G.noteN],0,sizeof(NoteObj));selection_clear();G.sceneRevision++;G.minimapDirty=1;save_workspace();
    }else editor_restore_original();
    if(mode==1||mode==5)save_workspace();else if(mode==2)save_frames();else if(mode==4)save_project_prefs();
    system_text_stop();G.editorOpen=0;G.editorMode=0;G.editorTarget=-1;G.editorLen=0;G.editorCreated=0;G.editorBuf[0]=0;G.editorOriginal[0]=0;
}
static void editor_open(int mode,int target,const char*initial){G.editorOpen=1;G.editorMode=mode;G.editorTarget=target;G.editorCreated=0;copy_text_local(G.editorBuf,NOTE_TEXT_CAP,initial?initial:"");copy_text_local(G.editorOriginal,NOTE_TEXT_CAP,initial?initial:"");G.editorLen=str_len_local(G.editorBuf);if(mode==2||mode==5){G.framePanel=1;G.addPanel=G.layersPanel=G.searchPanel=0;}system_text_start(G.editorBuf,mode);start_anim_timer();}
static void editor_apply(void){
    if(G.systemImeActive)system_text_read(G.editorBuf,NOTE_TEXT_CAP);editor_live_apply();
    if(G.editorMode==1&&G.editorTarget>=0&&G.editorTarget<G.noteN){copy_text_local(G.notes[G.editorTarget].text,NOTE_TEXT_CAP,G.editorBuf);note_autosize(&G.notes[G.editorTarget]);G.sceneRevision++;save_workspace();}
    else if(G.editorMode==2&&G.editorTarget>=0&&G.editorTarget<G.frameN){copy_text_local(G.frameNames[G.editorTarget],32,G.editorBuf);G.sceneRevision++;if(!G.frameNames[G.editorTarget][0])snprintf(G.frameNames[G.editorTarget],32,"Frame %d",frame_default_name_count()+1);frame_compact_default_names();save_frames();}
    else if(G.editorMode==3){copy_text_local(G.searchQuery,64,G.editorBuf);G.searchLen=str_len_local(G.searchQuery);G.searchPanel=1;}
    else if(G.editorMode==4&&G.editorTarget>=0&&G.editorTarget<4){copy_text_local(G.projectNames[G.editorTarget],32,G.editorBuf);if(!G.projectNames[G.editorTarget][0])snprintf(G.projectNames[G.editorTarget],32,"Project %d",G.editorTarget+1);save_project_prefs();G.projectsPanel=1;}
    else if(G.editorMode==5&&G.editorTarget>=0&&G.editorTarget<G.bookmarkN){copy_text_local(G.bookmarks[G.editorTarget].name,32,G.editorBuf);if(!G.bookmarks[G.editorTarget].name[0])snprintf(G.bookmarks[G.editorTarget].name,32,"Place %d",G.bookmarks[G.editorTarget].id);save_workspace();G.framePanel=1;}
    system_text_stop();G.editorOpen=0;G.editorMode=0;G.editorTarget=-1;G.editorLen=0;G.editorCreated=0;G.editorOriginal[0]=0;
}
static void editor_add_char(char c){if(G.editorLen>=94)return;G.editorBuf[G.editorLen++]=c;G.editorBuf[G.editorLen]=0;}
static void add_note_at_center(int type){if(G.noteN>=96)return;float cx=(G.screenW*.5f-G.offX)/G.scale,cy=(G.screenH*.5f-G.offY)/G.scale;NoteObj*n=&G.notes[G.noteN];memset(n,0,sizeof(*n));n->id=G.nextNoteId++;n->active=1;n->type=type;n->locked=0;n->w=type==NOTE_STICKY?300.0f:360.0f;n->h=type==NOTE_STICKY?220.0f:130.0f;n->x=cx-n->w*.5f;n->y=cy-n->h*.5f;n->color=type==NOTE_STICKY?0xffd66f:C_GLASS2;snprintf(n->text,96,type==NOTE_STICKY?"New sticky":"New note");if(type==NOTE_TEXT)note_autosize(n);int id=G.noteN++;selection_set_one(SEL_NOTE,id);G.selectedNote=id;editor_open(1,id,n->text);G.editorCreated=1;G.addPanel=0;G.minimapDirty=1;save_workspace();}
static void add_bookmark_here(void){if(G.bookmarkN>=32)return;BookmarkObj*b=&G.bookmarks[G.bookmarkN];memset(b,0,sizeof(*b));b->id=G.nextBookmarkId++;b->active=1;b->x=(G.screenW*.5f-G.offX)/G.scale;b->y=(G.screenH*.5f-G.offY)/G.scale;b->scale=G.scale;snprintf(b->name,32,"Place %d",b->id);G.selectedBookmark=G.bookmarkN++;G.addPanel=0;G.framePanel=1;save_workspace();}
static void bookmark_jump(int idx){if(idx<0||idx>=G.bookmarkN||!G.bookmarks[idx].active)return;BookmarkObj*b=&G.bookmarks[idx];G.scale=clampf(b->scale,.05f,20.0f);G.offX=G.screenW*.5f-b->x*G.scale;G.offY=G.screenH*.5f-b->y*G.scale;G.selectedBookmark=idx;G.minimapDirty=1;}
static void bookmark_delete(int idx){if(idx<0||idx>=G.bookmarkN)return;for(int i=idx+1;i<G.bookmarkN;i++)G.bookmarks[i-1]=G.bookmarks[i];G.bookmarkN--;if(G.selectedBookmark>=G.bookmarkN)G.selectedBookmark=G.bookmarkN-1;save_workspace();}
static void shape_mode_start(void){G.tool=MODE_SHAPE;G.photoMode=0;G.shapeType=SHAPE_RECT;G.addPanel=0;selection_clear();}
static void frame_duplicate_selected(void){if(G.selectedFrame<0||G.selectedFrame>=G.frameN||G.frameN>=48||G.lockFrames||G.frameLocked[G.selectedFrame])return;int si=G.selectedFrame;FrameObj f=G.frames[si];f.id=G.nextFrameId++;f.x+=36.0f/G.scale;f.y+=36.0f/G.scale;int di=G.frameN++;G.frames[di]=f;snprintf(G.frameNames[di],32,"%s copy",G.frameNames[si][0]?G.frameNames[si]:"Frame");G.frameLocked[di]=0;G.frameSizeLocked[di]=0;G.selectedFrame=di;frame_changed();}
static int search_collect(ObjRef*out,int max){int n=0,existingMax=max;G.ocrSearchHitN=0;
#ifdef VAST_OCR
    if(G.ocr&&G.searchQuery[0]&&max>0){int cap=mini(6,max),hn=ocr_manager_search(G.ocr,G.searchQuery,G.ocrSearchHits,(OcrU32)cap);if(hn>0){G.ocrSearchHitN=mini(hn,cap);existingMax=max-mini(G.ocrSearchHitN,maxi(1,max/2));}}
#endif
    for(int i=0;i<G.noteN&&n<existingMax;i++)if(G.notes[i].active&&contains_ci(G.notes[i].text,G.searchQuery))out[n++]=(ObjRef){SEL_NOTE,i};for(int i=0;i<G.frameN&&n<existingMax;i++)if(G.frames[i].active&&contains_ci(G.frameNames[i],G.searchQuery))out[n++]=(ObjRef){SEL_FRAME,i};for(int i=0;i<G.bookmarkN&&n<existingMax;i++)if(G.bookmarks[i].active&&contains_ci(G.bookmarks[i].name,G.searchQuery))out[n++]=(ObjRef){SEL_BOOKMARK,i};
#ifdef VAST_OCR
    for(int i=0;i<G.ocrSearchHitN&&n<max;i++)out[n++]=(ObjRef){SEL_HANDWRITING,i};
#endif
    return n;}
static void search_jump(ObjRef r){if(r.type==SEL_FRAME){frame_jump(r.index);selection_set_one(SEL_FRAME,r.index);}else if(r.type==SEL_BOOKMARK){bookmark_jump(r.index);}else if(r.type==SEL_NOTE&&r.index>=0&&r.index<G.noteN){NoteObj*n=&G.notes[r.index];G.offX=G.screenW*.5f-(n->x+n->w*.5f)*G.scale;G.offY=G.screenH*.5f-(n->y+n->h*.5f)*G.scale;selection_set_one(SEL_NOTE,r.index);}else if(r.type==SEL_HANDWRITING&&r.index>=0&&r.index<G.ocrSearchHitN){OcrManagerSearchHit hit=G.ocrSearchHits[r.index];
#ifdef VAST_OCR
    if(!G.ocr||ocr_manager_lookup(G.ocr,hit.handle,&hit)!=OCR_OK){G.searchPanel=0;return;}
#endif
    float w=fmax2(hit.bounds.maxx-hit.bounds.minx,1.0f),h=fmax2(hit.bounds.maxy-hit.bounds.miny,1.0f),margin=(float)us(120),sx=(G.screenW-2*margin)/w,sy=(G.screenH-2*margin)/h;G.scale=clampf(fmin2(sx,sy),0.02f,8.0f);G.offX=G.screenW*.5f-(hit.bounds.minx+hit.bounds.maxx)*.5f*G.scale;G.offY=G.screenH*.5f-(hit.bounds.miny+hit.bounds.maxy)*.5f*G.scale;selection_clear();G.ocrHighlightBounds=hit.bounds;G.ocrHighlightAlpha=1.0f;start_anim_timer();}G.searchPanel=0;G.minimapDirty=1;}
static void draw_focus_handle(Surf*s){if(android_ui_available()&&!G.zenMode)return;float cy=s->h*.5f;alpha_round_rect(s,0,(int)cy-us(42),us(8),(int)cy+us(42),us(4),C_TEXT,115);}
static void start_presentation(int idx){if(G.frameN<=0)return;G.presentationMode=1;G.presentationIndex=maxi(0,mini(G.frameN-1,idx));G.framePanel=0;G.zenMode=1;frame_jump(G.presentationIndex);}
static void presentation_step(int d){if(!G.presentationMode)return;int n=G.presentationIndex+d;if(n<0)n=0;if(n>=G.frameN)n=G.frameN-1;if(n!=G.presentationIndex){G.presentationIndex=n;frame_jump(n);}}
static uint32_t png_crc32_step(uint32_t crc,const uint8_t*data,size_t n){crc=~crc;for(size_t i=0;i<n;i++){crc^=data[i];for(int k=0;k<8;k++)crc=(crc>>1)^(0xedb88320u&((uint32_t)-(int)(crc&1u)));}return ~crc;}
static uint32_t png_adler32(const uint8_t*data,size_t n){uint32_t a=1,b=0;for(size_t i=0;i<n;i++){a=(a+data[i])%65521u;b=(b+a)%65521u;}return (b<<16)|a;}
static void png_be32(uint8_t*out,uint32_t v){out[0]=(uint8_t)(v>>24);out[1]=(uint8_t)(v>>16);out[2]=(uint8_t)(v>>8);out[3]=(uint8_t)v;}
static int png_chunk(void*fp,const char type[4],const uint8_t*data,uint32_t n){uint8_t q[8];png_be32(q,n);q[4]=(uint8_t)type[0];q[5]=(uint8_t)type[1];q[6]=(uint8_t)type[2];q[7]=(uint8_t)type[3];if(fwrite(q,8,1,fp)!=1)return 0;if(n&&fwrite(data,n,1,fp)!=1)return 0;uint32_t crc=png_crc32_step(0,(const uint8_t*)type,4);if(n)crc=png_crc32_step(crc,data,n);png_be32(q,crc);return fwrite(q,4,1,fp)==1;}
static int write_png_rgba(const char*path,const uint32_t*px,int w,int h){if(!path||!px||w<=0||h<=0)return 0;size_t row=(size_t)w*4u+1u,rawN=row*(size_t)h;uint8_t*raw=(uint8_t*)malloc(rawN);if(!raw)return 0;for(int y=0;y<h;y++){uint8_t*d=raw+(size_t)y*row;d[0]=0;for(int x=0;x<w;x++){uint32_t v=px[(size_t)y*w+x];d[1+x*4+0]=(uint8_t)(v&255);d[1+x*4+1]=(uint8_t)((v>>8)&255);d[1+x*4+2]=(uint8_t)((v>>16)&255);d[1+x*4+3]=255;}}size_t blocks=(rawN+65534u)/65535u,zcap=2u+rawN+blocks*5u+4u;uint8_t*z=(uint8_t*)malloc(zcap);if(!z){free(raw);return 0;}size_t zp=0,rp=0;z[zp++]=0x78;z[zp++]=0x01;while(rp<rawN){uint16_t len=(uint16_t)mini(65535,(int)mini((size_t)2147483647u,rawN-rp));int final=(rp+(size_t)len>=rawN);z[zp++]=(uint8_t)(final?1:0);z[zp++]=(uint8_t)(len&255);z[zp++]=(uint8_t)(len>>8);uint16_t nl=(uint16_t)~len;z[zp++]=(uint8_t)(nl&255);z[zp++]=(uint8_t)(nl>>8);memcpy(z+zp,raw+rp,len);zp+=len;rp+=len;}uint32_t ad=png_adler32(raw,rawN);z[zp++]=(uint8_t)(ad>>24);z[zp++]=(uint8_t)(ad>>16);z[zp++]=(uint8_t)(ad>>8);z[zp++]=(uint8_t)ad;free(raw);void*fp=fopen(path,"wb");if(!fp){free(z);return 0;}static const uint8_t sig[8]={137,80,78,71,13,10,26,10};int ok=fwrite(sig,8,1,fp)==1;uint8_t ihdr[13];png_be32(ihdr,(uint32_t)w);png_be32(ihdr+4,(uint32_t)h);ihdr[8]=8;ihdr[9]=6;ihdr[10]=0;ihdr[11]=0;ihdr[12]=0;if(ok)ok=png_chunk(fp,"IHDR",ihdr,13);if(ok)ok=png_chunk(fp,"IDAT",z,(uint32_t)zp);if(ok)ok=png_chunk(fp,"IEND",0,0);fclose(fp);free(z);return ok;}
static int export_selected_frame(void){
    if(G.selectedFrame<0||G.selectedFrame>=G.frameN)return 0;FrameObj*f=&G.frames[G.selectedFrame];if(!f->active)return 0;
    int outW=1600;float asp=f->h/fmax2(f->w,1.0f);int outH=maxi(240,mini(2200,(int)(outW*asp+0.5f)));if(outH>=2200){outH=2200;outW=maxi(240,(int)(outH/asp+0.5f));}
    uint32_t*buf=(uint32_t*)malloc((size_t)outW*outH*4);if(!buf)return 0;
    float os=G.scale,oox=G.offX,ooy=G.offY;int osw=G.screenW,osh=G.screenH,ovf=G.visFrames,omr=G.mapRenderMode;
    G.screenW=outW;G.screenH=outH;G.scale=fmin2((outW-40.0f)/fmax2(f->w,1.0f),(outH-40.0f)/fmax2(f->h,1.0f));G.offX=outW*.5f-(f->x+f->w*.5f)*G.scale;G.offY=outH*.5f-(f->y+f->h*.5f)*G.scale;G.visFrames=0;G.mapRenderMode=0;
    Surf ss={(uint8_t*)buf,outW,outH,outW,FORMAT_RGBX8888};fill(&ss,C_BG);draw_grid(&ss);draw_canvas_world(&ss);
    G.scale=os;G.offX=oox;G.offY=ooy;G.screenW=osw;G.screenH=osh;G.visFrames=ovf;G.mapRenderMode=omr;
    const char*base=(G.activity&&G.activity->externalDataPath)?G.activity->externalDataPath:(G.activity?G.activity->internalDataPath:0);if(!base){free(buf);return 0;}char path[640];snprintf(path,sizeof(path),"%s/frame_%d.png",base,f->id);int ok=write_png_rgba(path,buf,outW,outH);free(buf);return ok;
}
static void draw_add_panel(Surf*s){if(!G.addPanel)return;int m=margin_ui(),w=us(430),h=us(430),x=m+us(150),y=(s->h-h)/2;glass(s,x,y,x+w,y+h,us(24));text(s,x+us(24),y+us(22),"Add to canvas",ts(3),C_TEXT);button(s,x+w-us(126),y+us(18),x+w-us(24),y+us(62),"Close",0,C_MUTED);const char*n[6]={"Text","Sticky","Shape","Photo","Frame","Bookmark"};uint32_t c[6]={C_ACCENT,C_ACCENT,C_ACCENT,C_ACCENT,C_ACCENT,C_ACCENT};for(int i=0;i<6;i++){int col=i%2,row=i/2,bx=x+us(24)+col*us(194),by=y+us(92)+row*us(92);button(s,bx,by,bx+us(178),by+us(68),n[i],0,c[i]);}text(s,x+us(24),y+h-us(48),"One place for insertion keeps the rail quiet.",ts(1),C_MUTED);}
static void draw_layers_panel(Surf*s){if(!G.layersPanel)return;int m=margin_ui(),w=us(480),h=us(650),x=s->w-m-w,y=m+us(82);glass(s,x,y,x+w,y+h,us(24));text(s,x+us(24),y+us(22),"Layers",ts(3),C_TEXT);button(s,x+w-us(126),y+us(18),x+w-us(24),y+us(62),"Close",0,C_MUTED);const char*names[7]={"Ink","Marker","Photos","CAD","Notes","Shapes","Frames"};int vis[7]={G.visInk,G.visMarker,G.visPhotos,G.visCAD,G.visNotes,G.visShapes,G.visFrames};int lock[7]={G.lockInk,G.lockMarker,G.lockPhotos,G.lockCAD,G.lockNotes,G.lockShapes,G.lockFrames};uint32_t cc[7]={C_ACCENT,C_ACCENT,C_ACCENT,C_ACCENT,C_ACCENT,C_ACCENT,C_ACCENT};for(int i=0;i<7;i++){int yy=y+us(94)+i*us(70);button(s,x+us(24),yy,x+us(300),yy+us(52),names[i],vis[i],cc[i]);button(s,x+us(316),yy,x+w-us(24),yy+us(52),lock[i]?"Locked":"Lock",lock[i],C_WARN);}text(s,x+us(24),y+h-us(42),"Hide or lock whole object classes without cluttering the canvas.",ts(1),C_MUTED);}
static const char*search_result_label(ObjRef r,char*b,int cap){if(r.type==SEL_NOTE&&r.index<G.noteN){snprintf(b,(size_t)cap,"Note  -  %s",G.notes[r.index].text);return b;}if(r.type==SEL_FRAME&&r.index<G.frameN){snprintf(b,(size_t)cap,"Frame  -  %s",G.frameNames[r.index]);return b;}if(r.type==SEL_BOOKMARK&&r.index<G.bookmarkN){snprintf(b,(size_t)cap,"Place  -  %s",G.bookmarks[r.index].name);return b;}if(r.type==SEL_HANDWRITING&&r.index>=0&&r.index<G.ocrSearchHitN){snprintf(b,(size_t)cap,"Handwriting  -  %s",G.ocrSearchHits[r.index].label);return b;}copy_text_local(b,cap,"Result");return b;}
static void draw_search_panel(Surf*s){if(!G.searchPanel){G.searchVisibleN=0;return;}int m=margin_ui(),w=us(600),h=us(620),x=s->w-m-w,y=m+us(82),nativeText=NU.ready&&NU.hostShown;glass(s,x,y,x+w,y+h,us(24));text(s,x+us(24),y+us(22),"Search",ts(3),C_TEXT);button(s,x+w-us(126),y+us(18),x+w-us(24),y+us(62),"Close",0,C_MUTED);char q[96];snprintf(q,sizeof(q),G.searchQuery[0]?"Query: %s":"Tap to enter a search",G.searchQuery);button(s,x+us(24),y+us(90),x+w-us(24),y+us(146),nativeText?"":q,1,C_ACCENT);G.searchVisibleN=search_collect(G.searchVisible,6);text(s,x+us(24),y+us(172),G.searchVisibleN?"Results":"No matching canvas objects",ts(2),G.searchVisibleN?C_TEXT:C_MUTED);for(int i=0;i<G.searchVisibleN;i++){char b[128];search_result_label(G.searchVisible[i],b,128);button(s,x+us(24),y+us(210)+i*us(62),x+w-us(24),y+us(260)+i*us(62),nativeText?"":b,0,G.searchVisible[i].type==SEL_FRAME?C_VIOLET:(G.searchVisible[i].type==SEL_NOTE?C_TEXT:(G.searchVisible[i].type==SEL_HANDWRITING?C_CYAN:C_ACCENT)));}}
static void draw_projects_panel(Surf*s){if(!G.projectsPanel)return;int m=margin_ui(),w=us(720),h=us(540),x=(s->w-w)/2,y=(s->h-h)/2;glass(s,x,y,x+w,y+h,us(28));text(s,x+us(28),y+us(24),"Projects",ts(3),C_TEXT);text(s,x+us(28),y+us(62),"Local canvases with independent frames, notes, photos and history.",ts(1),C_MUTED);button(s,x+w-us(132),y+us(20),x+w-us(24),y+us(64),"Close",0,C_MUTED);for(int i=0;i<4;i++){int col=i%2,row=i/2,bx=x+us(28)+col*us(330),by=y+us(116)+row*us(132);char q[64];int used=project_slot_has_data(i);snprintf(q,sizeof(q),"%s%s",G.projectNames[i][0]?G.projectNames[i]:"Project",used?"":"  -  empty");button(s,bx,by,bx+us(304),by+us(102),q,i==G.projectIndex,i==G.projectIndex?C_GREEN:(used?C_ACCENT:C_MUTED));}button(s,x+us(28),y+h-us(94),x+us(220),y+h-us(38),"Rename current",0,C_TEXT);button(s,x+us(234),y+h-us(94),x+us(438),y+h-us(38),"Duplicate",0,C_ACCENT);button(s,x+us(452),y+h-us(94),x+w-us(28),y+h-us(38),"Clear project",0,C_DANGER);}
static void draw_shape_panel(Surf*s){if(G.tool!=MODE_SHAPE)return;int m=margin_ui(),w=us(500),h=us(150),x=s->w-m-w,y=m+us(82);glass(s,x,y,x+w,y+h,us(22));text(s,x+us(20),y+us(18),"Shape",ts(2),C_TEXT);const char*n[4]={"Line","Rect","Ellipse","Arrow"};for(int i=0;i<4;i++)button(s,x+us(20)+i*us(112),y+us(56),x+us(120)+i*us(112),y+us(104),n[i],G.shapeType==i,C_ACCENT);button(s,x+w-us(100),y+us(112),x+w-us(20),y+us(142),"Done",0,C_MUTED);}
static void draw_live_frame_preview(Surf*s){
    if(!G.frameCreating||!G.frameDragging)return;FrameObj f=G.liveFrame;frame_normalize(&f);
    float x0=f.x*G.scale+G.offX,y0=f.y*G.scale+G.offY,x1=(f.x+f.w)*G.scale+G.offX,y1=(f.y+f.h)*G.scale+G.offY;
    if(f.w*G.scale<2.0f&&f.h*G.scale<2.0f){aa_circle(s,x0,y0,us(8),C_ACCENT,220);return;}
    int ix0=(int)fmin2(x0,x1),iy0=(int)fmin2(y0,y1),ix1=(int)fmax2(x0,x1),iy1=(int)fmax2(y0,y1);
    alpha_round_rect(s,ix0,iy0,ix1,iy1,us(8),C_ACCENT,22);
    aa_hline(s,y0,ix0,ix1,C_ACCENT,245);aa_hline(s,y1,ix0,ix1,C_ACCENT,245);aa_vline(s,x0,iy0,iy1,C_ACCENT,245);aa_vline(s,x1,iy0,iy1,C_ACCENT,245);
    aa_circle(s,x0,y0,us(5),C_ACCENT,255);aa_circle(s,x1,y0,us(5),C_ACCENT,255);aa_circle(s,x0,y1,us(5),C_ACCENT,255);aa_circle(s,x1,y1,us(5),C_ACCENT,255);
    const char*q="New frame";int fs=ts(1),tw=text_w(q,fs);int ty=iy0-font_line_h(fs)-us(8);if(ty<margin_ui())ty=iy0+us(8);alpha_round_rect(s,ix0,ty,ix0+tw+us(18),ty+font_line_h(fs)+us(8),us(7),C_GLASS2,220);text(s,ix0+us(9),ty+us(4),q,fs,C_ACCENT);
}
static int selection_single_lockable(void){if(G.selectedRefN!=1)return 0;int t=G.selectedRefs[0].type;return t==SEL_NOTE||t==SEL_SHAPE||t==SEL_FRAME;}
static int selection_single_locked(void){if(G.selectedRefN!=1)return 0;ObjRef r=G.selectedRefs[0];if(r.type==SEL_NOTE&&r.index>=0&&r.index<G.noteN)return G.notes[r.index].locked;if(r.type==SEL_SHAPE&&r.index>=0&&r.index<G.shapeN)return G.shapes[r.index].locked;if(r.type==SEL_FRAME&&r.index>=0&&r.index<G.frameN)return G.frameLocked[r.index];return 0;}
static int selection_single_colorable(void){if(G.selectedRefN!=1)return 0;int t=G.selectedRefs[0].type;return t==SEL_STROKES||t==SEL_NOTE||t==SEL_SHAPE||t==SEL_FRAME;}
static int objref_can_mutate(ObjRef r){
    if(r.type==SEL_STROKES&&r.index>=0&&r.index<G.strokeN){Stroke*s=&G.strokes[r.index];if(!s->active)return 0;int aa=(int)((s->color>>24)&255),marker=aa>0&&aa<248;return marker?!G.lockMarker:!G.lockInk;}
    if(r.type==SEL_IMAGE&&r.index>=0&&r.index<G.imageN)return G.images[r.index].active&&!G.lockPhotos;
    if(r.type==SEL_MEASURE&&r.index>=0&&r.index<G.measureN)return G.measures[r.index].active&&!G.lockCAD;
    if(r.type==SEL_NOTE&&r.index>=0&&r.index<G.noteN)return G.notes[r.index].active&&!G.lockNotes&&!G.notes[r.index].locked;
    if(r.type==SEL_SHAPE&&r.index>=0&&r.index<G.shapeN)return G.shapes[r.index].active&&!G.lockShapes&&!G.shapes[r.index].locked;
    if(r.type==SEL_FRAME&&r.index>=0&&r.index<G.frameN)return G.frames[r.index].active&&!G.lockFrames&&!G.frameLocked[r.index];
    return 0;
}
static int objref_exists(ObjRef r){if(r.type==SEL_STROKES)return r.index>=0&&r.index<G.strokeN&&G.strokes[r.index].active;if(r.type==SEL_IMAGE)return r.index>=0&&r.index<G.imageN&&G.images[r.index].active;if(r.type==SEL_MEASURE)return r.index>=0&&r.index<G.measureN&&G.measures[r.index].active;if(r.type==SEL_NOTE)return r.index>=0&&r.index<G.noteN&&G.notes[r.index].active;if(r.type==SEL_SHAPE)return r.index>=0&&r.index<G.shapeN&&G.shapes[r.index].active;if(r.type==SEL_FRAME)return r.index>=0&&r.index<G.frameN&&G.frames[r.index].active;return 0;}
static void sanitize_group_refs(void){for(int i=0;i<G.groupN;i++){GroupObj*g=&G.groups[i];int out=0;for(int k=0;k<g->n;k++){ObjRef r=g->refs[k];if(!objref_exists(r))continue;int duplicate=0;for(int j=0;j<out;j++)if(g->refs[j].type==r.type&&g->refs[j].index==r.index){duplicate=1;break;}if(!duplicate)g->refs[out++]=r;}for(int k=out;k<g->n;k++)g->refs[k]=(ObjRef){SEL_NONE,-1};g->n=out;if(g->n<2)g->active=0;}}
static int selection_has_mutable_object(void){for(int i=0;i<G.selectedRefN;i++)if(objref_can_mutate(G.selectedRefs[i]))return 1;return 0;}
static void selection_toggle_lock(void){
    if(G.selectedRefN!=1)return;ObjRef r=G.selectedRefs[0];
    if(r.type==SEL_NOTE&&r.index>=0&&r.index<G.noteN&&!G.lockNotes){G.notes[r.index].locked=!G.notes[r.index].locked;G.sceneRevision++;save_workspace();}
    else if(r.type==SEL_SHAPE&&r.index>=0&&r.index<G.shapeN&&!G.lockShapes){G.shapes[r.index].locked=!G.shapes[r.index].locked;G.sceneRevision++;save_workspace();}
    else if(r.type==SEL_FRAME&&r.index>=0&&r.index<G.frameN&&!G.lockFrames){G.frameLocked[r.index]=!G.frameLocked[r.index];frame_changed();}
}
static int context_toolbar_geometry(int*x0,int*y0,int*w,int*h){if(G.selectedRefN<=0)return 0;if(G.selectedRefN==1&&G.selectedRefs[0].type==SEL_IMAGE)return 0;float a,b,c,d;if(!selection_bounds(&a,&b,&c,&d))return 0;float sx0=a*G.scale+G.offX,sx1=c*G.scale+G.offX,sy0=b*G.scale+G.offY;*w=us(500);*h=us(58);*x0=(int)((sx0+sx1)*.5f)-*w/2;*y0=(int)sy0-us(76);*x0=maxi(margin_ui(),mini(G.screenW-margin_ui()-*w,*x0));if(*y0<margin_ui())*y0=(int)(b*G.scale+G.offY)+us(24);return 1;}
static void draw_context_toolbar(Surf*s){if(G.selectedRefN<=0||G.presentationMode)return;int x,y,w,h;if(!context_toolbar_geometry(&x,&y,&w,&h))return;glass(s,x,y,x+w,y+h,us(18));const char*first="More";int firstActive=0;if(G.selectedRefN==1){int t=G.selectedRefs[0].type;if(t==SEL_NOTE){first="Edit";firstActive=1;}else if(t==SEL_FRAME){first="Frames";firstActive=1;}else if(t==SEL_STROKES||t==SEL_SHAPE){first="Color";}else if(t==SEL_IMAGE){first="Photo";}else if(t==SEL_MEASURE){first="CAD";}}button(s,x+us(10),y+us(8),x+us(112),y+h-us(8),first,firstActive,C_ACCENT);button(s,x+us(120),y+us(8),x+us(244),y+h-us(8),"Duplicate",0,C_ACCENT);const char*third=G.selectionType==SEL_GROUP&&G.selectedGroup>=0?"Ungroup":(G.selectedRefN>1?"Group":"More");button(s,x+us(252),y+us(8),x+us(366),y+h-us(8),third,G.selectionMoreOpen||(G.selectedRefN>1),C_ACCENT);button(s,x+us(374),y+us(8),x+w-us(10),y+h-us(8),"Delete",0,C_DANGER);
    if(G.selectionMoreOpen&&G.selectedRefN==1){int mw=us(230),row=us(46),rows=1+(selection_single_colorable()?1:0)+(selection_single_lockable()?1:0),mx=x+us(252),my=y+h+us(8);if(my+rows*row>s->h-margin_ui())my=y-us(8)-rows*row;glass(s,mx,my,mx+mw,my+rows*row,us(14));int yy=my;if(selection_single_colorable()){button(s,mx+us(6),yy+us(5),mx+mw-us(6),yy+row-us(5),"Color",0,C_ACCENT);yy+=row;}if(selection_single_lockable()){button(s,mx+us(6),yy+us(5),mx+mw-us(6),yy+row-us(5),selection_single_locked()?"Unlock":"Lock",selection_single_locked(),C_WARN);yy+=row;}button(s,mx+us(6),yy+us(5),mx+mw-us(6),yy+row-us(5),"Deselect",0,C_MUTED);}
}
static void draw_editor(Surf*s){if(!G.editorOpen||G.editorMode==2||G.editorMode==5)return;int w=mini(us(780),s->w-2*margin_ui()),h=G.editorMode==1?us(320):us(180),x=(s->w-w)/2,y=margin_ui()+us(66);glass(s,x,y,x+w,y+h,us(24));const char*title=G.editorMode==3?"Search canvas":(G.editorMode==4?"Rename project":"Edit text");text(s,x+us(24),y+us(18),title,ts(3),C_TEXT);if(G.editorMode==1)text(s,x+us(190),y+us(24),"Text box sizes itself while you type",ts(1),C_MUTED);button(s,x+us(24),y+h-us(54),x+us(176),y+h-us(14),"Cancel",0,C_DANGER);button(s,x+w-us(176),y+h-us(54),x+w-us(24),y+h-us(14),"Done",1,C_GREEN);}
static void draw_presentation_overlay(Surf*s){if(!G.presentationMode)return;int m=margin_ui();char q[64];snprintf(q,sizeof(q),"Frame %d / %d",G.presentationIndex+1,maxi(1,G.frameN));int tw=text_w(q,ts(2));alpha_round_rect(s,s->w/2-tw/2-us(20),m,s->w/2+tw/2+us(20),m+us(44),us(14),C_GLASS2,190);text(s,s->w/2-tw/2,m+us(11),q,ts(2),C_TEXT);button(s,m,s->h-m-us(56),m+us(150),s->h-m,"Previous",G.presentationIndex>0,C_MUTED);button(s,s->w-m-us(150),s->h-m-us(56),s->w-m,s->h-m,"Next",G.presentationIndex+1<G.frameN,C_ACCENT);button(s,s->w-m-us(110),m,s->w-m,m+us(44),"Exit",0,C_DANGER);}
static void draw_pressure_panel(Surf*s){if(!G.pressurePanel)return;int m=margin_ui(),w=us(430),x=s->w-m-w,y=m+us(82),h=us(470);glass(s,x,y,x+w,y+h,us(26));text(s,x+us(24),y+us(22),"Pressure",ts(3),C_TEXT);text(s,x+us(24),y+us(56),"WIDTH RESPONSE",ts(2),C_MUTED);char sm[32];snprintf(sm,sizeof(sm),"SMOOTH %.0f%%",G.pressureSmoothing*100);text(s,x+us(24),y+us(438),sm,ts(2),C_MUTED);char q[64];snprintf(q,sizeof(q),"Live %.0f%%",clampf(G.lastRawPressure,0,1)*100.0f);text(s,x+us(276),y+us(56),q,ts(2),C_VIOLET);meter(s,x+us(24),y+us(82),w-us(48),clampf(G.lastRawPressure,0,1),C_VIOLET);draw_pressure_curve(s,x+us(24),y+us(112),w-us(48),us(118));int yy=y+us(252);text(s,x+us(24),yy,"CURVE",ts(2),C_MUTED);button(s,x+us(104),yy-us(12),x+us(190),yy+us(34),"SOFT",G.pressureCurve==0,C_VIOLET);button(s,x+us(198),yy-us(12),x+us(292),yy+us(34),"LINEAR",G.pressureCurve==1,C_VIOLET);button(s,x+us(300),yy-us(12),x+w-us(24),yy+us(34),"FIRM",G.pressureCurve==2,C_VIOLET);yy+=us(70);snprintf(q,sizeof(q),"Light touch  %.0f%%",G.pressureMin*100.0f);text(s,x+us(24),yy,q,ts(2),C_TEXT);meter(s,x+us(24),yy+us(28),us(230),G.pressureMin/1.2f,C_ACCENT);button(s,x+us(274),yy+us(8),x+us(322),yy+us(48),"-",0,C_VIOLET);button(s,x+us(334),yy+us(8),x+us(382),yy+us(48),"+",0,C_VIOLET);yy+=us(76);snprintf(q,sizeof(q),"Full pressure  %.0f%%",G.pressureMax*100.0f);text(s,x+us(24),yy,q,ts(2),C_TEXT);meter(s,x+us(24),yy+us(28),us(230),G.pressureMax/1.8f,C_VIOLET);button(s,x+us(274),yy+us(8),x+us(322),yy+us(48),"-",0,C_VIOLET);button(s,x+us(334),yy+us(8),x+us(382),yy+us(48),"+",0,C_VIOLET);}
static void draw_measure_panel(Surf*s){if(G.tool!=MODE_MEASURE)return;int m=margin_ui(),w=us(470),x=s->w-m-w,y=m+us(82),h=us(560);glass(s,x,y,x+w,y+h,us(26));text(s,x+us(24),y+us(22),"CAD dimensions",ts(3),C_TEXT);char sub[72];snprintf(sub,sizeof(sub),G.cadCalibrated?"CALIBRATED  1U = %.5f %s":"UNCALIBRATED CANVAS UNITS",G.cadScale,cad_unit_name());text(s,x+us(24),y+us(56),sub,ts(2),G.cadCalibrated?C_GREEN:C_MUTED);button(s,x+us(24),y+us(86),x+us(160),y+us(136),G.snap?"SNAP 16U":"FREE",G.snap,C_CYAN);button(s,x+us(172),y+us(86),x+us(306),y+us(136),"Calibrate",G.selectedMeasure>=0,C_GREEN);button(s,x+us(318),y+us(86),x+w-us(24),y+us(136),"Clear all",0,C_DANGER);
    int yy=y+us(164);char q[96];if(G.selectedMeasure>=0&&G.selectedMeasure<G.measureN){Measure*sm=&G.measures[G.selectedMeasure];snprintf(q,sizeof(q),"SELECTED  #%d   %.2f %s",G.selectedMeasure+1,measure_display_value(sm),G.cadCalibrated?cad_unit_name():"U");text(s,x+us(24),yy,q,ts(2),C_WARN);button(s,x+w-us(158),yy-us(12),x+w-us(24),yy+us(34),"Delete",1,C_DANGER);yy+=us(52);}else{text(s,x+us(24),yy,"Tap a dimension to select it",ts(2),C_MUTED);yy+=us(48);}if(G.measureN==0){text(s,x+us(24),yy,"Drag with the stylus to create a dimension",ts(2),C_MUTED);text(s,x+us(24),yy+us(34),"Select a dimension, then calibrate it to a known size.",ts(2),C_MUTED);return;}Measure*bm=&G.measures[G.measureN-1];float db=measure_display_value(bm);Measure*am=G.measureN>=2?&G.measures[G.measureN-2]:bm;float da=measure_display_value(am);const char*u=G.cadCalibrated?cad_unit_name():"U";snprintf(q,sizeof(q),"A   %.2f %s",da,u);text(s,x+us(24),yy,q,ts(3),G.measureN>=2?C_VIOLET:C_CYAN);yy+=us(44);if(G.measureN>=2){snprintf(q,sizeof(q),"B   %.2f %s",db,u);text(s,x+us(24),yy,q,ts(3),C_CYAN);yy+=us(52);float delta=db-da,ratio=da>0.0001f?db/da:0,pct=da>0.0001f?delta/da*100.0f:0;hline(s,yy,x+us(24),x+w-us(24),C_BORDER);yy+=us(22);snprintf(q,sizeof(q),"B / A   %.3f X",ratio);text(s,x+us(24),yy,q,ts(3),C_TEXT);snprintf(q,sizeof(q),"DELTA   %+.2f %s",delta,u);text(s,x+us(24),yy+us(42),q,ts(2),delta>=0?C_GREEN:C_DANGER);snprintf(q,sizeof(q),"CHANGE  %+.1f%%",pct);text(s,x+us(24),yy+us(72),q,ts(2),pct>=0?C_GREEN:C_DANGER);yy+=us(110);}float dx=fabsf(bm->bx-bm->ax)*(G.cadCalibrated?G.cadScale:1),dy=fabsf(bm->by-bm->ay)*(G.cadCalibrated?G.cadScale:1);snprintf(q,sizeof(q),"LATEST DX %.2f   DY %.2f %s",dx,dy,u);text(s,x+us(24),yy,q,ts(2),C_MUTED);}

static void draw_photo_panel(Surf*s){if(!G.photoMode)return;int m=margin_ui(),w=us(390),x=s->w-m-w,y=m+us(82),h=us(332);glass(s,x,y,x+w,y+h,us(24));text(s,x+us(24),y+us(22),"Photo editor",ts(3),C_TEXT);text(s,x+us(24),y+us(58),"1 finger moves  -  2 fingers scale + rotate",ts(2),C_MUTED);if(G.selectedImage>=0&&G.selectedImage<G.imageN&&G.images[G.selectedImage].active){ImageObj*im=&G.images[G.selectedImage];char q[80];snprintf(q,sizeof(q),"SIZE %.0f x %.0f   ROT %.1f DEG",im->w,im->h,im->rot*180.0f/PI);text(s,x+us(24),y+us(104),q,ts(2),C_CYAN);text(s,x+us(24),y+us(146),"Angle snap",ts(2),C_TEXT);button(s,x+w-us(170),y+us(132),x+w-us(24),y+us(182),G.photoAngleSnap?"15 DEG":"FREE",G.photoAngleSnap,C_ACCENT);button(s,x+us(24),y+us(246),x+us(176),y+us(298),"Delete",1,C_DANGER);button(s,x+us(190),y+us(246),x+w-us(24),y+us(298),"Done",0,C_GREEN);}else{text(s,x+us(24),y+us(112),"Press and hold a photo to select it",ts(2),C_WARN);button(s,x+us(24),y+us(240),x+w-us(24),y+us(292),"Done",0,C_GREEN);}}


// ---------- Vast 3 objects, selection, lasso and snap ----------
static int str_len_local(const char*s){int n=0;if(!s)return 0;while(s[n])n++;return n;}
static void draw_wrapped_text(Surf*s,int x,int y,int maxw,const char*src,int px,uint32_t c,int maxLines){
    if(!src||!src[0])return;char line[64];int li=0,lines=0,yy=y;
    for(int i=0;;i++){char ch=src[i];int end=(ch==0);if(ch=='\n'||end){line[li]=0;if(li)text(s,x,yy,line,px,c);yy+=font_line_h(px)+us(4);lines++;li=0;if(lines>=maxLines||end)break;continue;}
        if(li<62){line[li++]=ch;line[li]=0;if(text_w(line,px)>maxw){li--;line[li]=0;if(li){text(s,x,yy,line,px,c);yy+=font_line_h(px)+us(4);lines++;}li=0;if(lines>=maxLines)break;if(ch!=' ')line[li++]=ch;}}
    }
}
static void draw_notes(Surf*s){if(!G.visNotes)return;for(int i=0;i<G.noteN;i++){NoteObj*n=&G.notes[i];if(!n->active)continue;note_autosize(n);float x0=n->x*G.scale+G.offX,y0=n->y*G.scale+G.offY,x1=(n->x+n->w)*G.scale+G.offX,y1=(n->y+n->h)*G.scale+G.offY;if(x1<-80||y1<-80||x0>s->w+80||y0>s->h+80)continue;int ix0=(int)x0,iy0=(int)y0,ix1=(int)x1,iy1=(int)y1;if(n->type==NOTE_STICKY){alpha_round_rect(s,ix0,iy0,ix1,iy1,us(14),n->color,205);aa_hline(s,y0,ix0+us(10),ix1-us(10),mix_color(n->color,C_TEXT,30),105);}else{alpha_round_rect(s,ix0,iy0,ix1,iy1,us(12),C_GLASS2,142);}if(G.scale>.18f){if(note_text_draw(s,n,n->type==NOTE_STICKY?C_BLACK:C_TEXT))continue;int fs=maxi(ts(1),(int)(18.0f*clampf(G.scale,.65f,1.30f))),lh=font_line_h(fs)+us(4),maxLines=maxi(1,mini(24,(iy1-iy0-us(20))/maxi(1,lh)));draw_wrapped_text(s,ix0+us(12),iy0+us(10),maxi(us(40),ix1-ix0-us(24)),n->text,fs,n->type==NOTE_STICKY?C_BLACK:C_TEXT,maxLines);}}
}
static void draw_shape_one(Surf*s,ShapeObj*q,uint32_t c,int alpha){float x0=q->x0*G.scale+G.offX,y0=q->y0*G.scale+G.offY,x1=q->x1*G.scale+G.offX,y1=q->y1*G.scale+G.offY,r=clampf(q->width*G.scale*.5f,.8f,22.0f);if(q->type==SHAPE_LINE||q->type==SHAPE_ARROW){aa_capsule(s,x0,y0,r,x1,y1,r,c,alpha);if(q->type==SHAPE_ARROW){float dx=x1-x0,dy=y1-y0,d=sqrtf(dx*dx+dy*dy);if(d>3){float ux=dx/d,uy=dy/d,nx=-uy,ny=ux,L=fmin2(26.0f,d*.28f);aa_capsule(s,x1,y1,r,x1-ux*L+nx*L*.45f,y1-uy*L+ny*L*.45f,r,c,alpha);aa_capsule(s,x1,y1,r,x1-ux*L-nx*L*.45f,y1-uy*L-ny*L*.45f,r,c,alpha);}}return;}float lx=fmin2(x0,x1),rx=fmax2(x0,x1),ty=fmin2(y0,y1),by=fmax2(y0,y1);if(q->type==SHAPE_RECT){aa_capsule(s,lx,ty,r,rx,ty,r,c,alpha);aa_capsule(s,rx,ty,r,rx,by,r,c,alpha);aa_capsule(s,rx,by,r,lx,by,r,c,alpha);aa_capsule(s,lx,by,r,lx,ty,r,c,alpha);}else{float cx=(lx+rx)*.5f,cy=(ty+by)*.5f,ax=(rx-lx)*.5f,ay=(by-ty)*.5f;float px=cx+ax,py=cy;for(int k=1;k<=48;k++){float a=(float)k*(2*PI/48.0f),xx=cx+cosf(a)*ax,yy=cy+sinf(a)*ay;aa_capsule(s,px,py,r,xx,yy,r,c,alpha);px=xx;py=yy;}}}
static void draw_shapes(Surf*s){if(!G.visShapes)return;for(int i=0;i<G.shapeN;i++){ShapeObj*q=&G.shapes[i];if(!q->active)continue;draw_shape_one(s,q,q->color,235);}if(G.shapeDrawing)draw_shape_one(s,&G.liveShape,C_GREEN,230);}
static void selection_clear(void){G.selectionType=SEL_NONE;G.selectedRefN=0;G.selectedStrokeN=0;G.selectedImage=-1;G.selectedMeasure=-1;G.selectedNote=-1;G.selectedShape=-1;G.selectedFrame=-1;G.selectedGroup=-1;G.selectionDragging=0;G.selectionMoreOpen=0;G.snapGuideX=G.snapGuideY=0;}
static void selection_set_one(int type,int index){selection_clear();if(index<0)return;G.selectionType=type;G.selectedRefs[0]=(ObjRef){type,index};G.selectedRefN=1;if(type==SEL_STROKES){G.selectedStrokes[0]=index;G.selectedStrokeN=1;}else if(type==SEL_IMAGE)G.selectedImage=index;else if(type==SEL_MEASURE)G.selectedMeasure=index;else if(type==SEL_NOTE)G.selectedNote=index;else if(type==SEL_SHAPE)G.selectedShape=index;else if(type==SEL_FRAME)G.selectedFrame=index;}
static int objref_bounds(ObjRef r,float*x0,float*y0,float*x1,float*y1){
    if(r.type==SEL_STROKES&&r.index>=0&&r.index<G.strokeN){Stroke*st=&G.strokes[r.index];if(!st->active)return 0;*x0=st->minx;*y0=st->miny;*x1=st->maxx;*y1=st->maxy;return 1;}
    if(r.type==SEL_IMAGE&&r.index>=0&&r.index<G.imageN){ImageObj*im=&G.images[r.index];if(!im->active)return 0;float xx[4],yy[4];image_world_corners(im,xx,yy);*x0=*x1=xx[0];*y0=*y1=yy[0];for(int k=1;k<4;k++){*x0=fmin2(*x0,xx[k]);*x1=fmax2(*x1,xx[k]);*y0=fmin2(*y0,yy[k]);*y1=fmax2(*y1,yy[k]);}return 1;}
    if(r.type==SEL_MEASURE&&r.index>=0&&r.index<G.measureN){Measure*m=&G.measures[r.index];*x0=fmin2(m->ax,m->bx);*x1=fmax2(m->ax,m->bx);*y0=fmin2(m->ay,m->by);*y1=fmax2(m->ay,m->by);return 1;}
    if(r.type==SEL_NOTE&&r.index>=0&&r.index<G.noteN){NoteObj*n=&G.notes[r.index];if(!n->active)return 0;*x0=n->x;*y0=n->y;*x1=n->x+n->w;*y1=n->y+n->h;return 1;}
    if(r.type==SEL_SHAPE&&r.index>=0&&r.index<G.shapeN){ShapeObj*q=&G.shapes[r.index];if(!q->active)return 0;*x0=fmin2(q->x0,q->x1);*x1=fmax2(q->x0,q->x1);*y0=fmin2(q->y0,q->y1);*y1=fmax2(q->y0,q->y1);return 1;}
    if(r.type==SEL_FRAME&&r.index>=0&&r.index<G.frameN){FrameObj*f=&G.frames[r.index];if(!f->active)return 0;*x0=f->x;*y0=f->y;*x1=f->x+f->w;*y1=f->y+f->h;return 1;}return 0;
}
static int selection_bounds(float*x0,float*y0,float*x1,float*y1){int have=0;for(int i=0;i<G.selectedRefN;i++){float a,b,c,d;if(!objref_bounds(G.selectedRefs[i],&a,&b,&c,&d))continue;if(!have){*x0=a;*y0=b;*x1=c;*y1=d;have=1;}else{*x0=fmin2(*x0,a);*y0=fmin2(*y0,b);*x1=fmax2(*x1,c);*y1=fmax2(*y1,d);}}return have;}
static void move_objref(ObjRef r,float dx,float dy){if(r.type==SEL_STROKES&&r.index>=0&&r.index<G.strokeN){Stroke*st=&G.strokes[r.index];if(!st->active)return;int aa=(int)((st->color>>24)&255),marker=aa>0&&aa<248;if((marker&&G.lockMarker)||(!marker&&G.lockInk))return;for(int k=0;k<st->n;k++){st->pts[k].x+=dx;st->pts[k].y+=dy;}st->minx+=dx;st->maxx+=dx;st->miny+=dy;st->maxy+=dy;ocr_move_mark(r.index,dx,dy);G.saveDirty=1;}else if(r.type==SEL_IMAGE&&r.index>=0&&r.index<G.imageN&&!G.lockPhotos){ImageObj*im=&G.images[r.index];if(im->active){im->x+=dx;im->y+=dy;G.imagesDirty=1;}}else if(r.type==SEL_MEASURE&&r.index>=0&&r.index<G.measureN&&!G.lockCAD){Measure*m=&G.measures[r.index];m->ax+=dx;m->ay+=dy;m->bx+=dx;m->by+=dy;G.metaDirty=1;}else if(r.type==SEL_NOTE&&r.index>=0&&r.index<G.noteN&&!G.lockNotes&&!G.notes[r.index].locked){G.notes[r.index].x+=dx;G.notes[r.index].y+=dy;}else if(r.type==SEL_SHAPE&&r.index>=0&&r.index<G.shapeN&&!G.lockShapes&&!G.shapes[r.index].locked){G.shapes[r.index].x0+=dx;G.shapes[r.index].x1+=dx;G.shapes[r.index].y0+=dy;G.shapes[r.index].y1+=dy;}else if(r.type==SEL_FRAME&&r.index>=0&&r.index<G.frameN&&!G.frameLocked[r.index]&&!G.lockFrames){G.selectedFrame=r.index;frame_move_selected_by(dx,dy);}G.minimapDirty=1;}
static int ref_is_selected(int type,int index){for(int i=0;i<G.selectedRefN;i++)if(G.selectedRefs[i].type==type&&G.selectedRefs[i].index==index)return 1;return 0;}
static void snap_box_targets(float tx0,float ty0,float tx1,float ty1,float nx[3],float ny[3],float*bestX,float*bestY,float*fixX,float*fixY){float xx[3]={tx0,(tx0+tx1)*.5f,tx1},yy[3]={ty0,(ty0+ty1)*.5f,ty1};for(int a=0;a<3;a++)for(int b=0;b<3;b++){float d=fabsf(xx[a]-nx[b]);if(d<*bestX){*bestX=d;*fixX=xx[a]-nx[b];G.snapGuideWX=xx[a];}d=fabsf(yy[a]-ny[b]);if(d<*bestY){*bestY=d;*fixY=yy[a]-ny[b];G.snapGuideWY=yy[a];}}}
static void selection_snap_delta(float*dx,float*dy){if(!G.snap||G.selectedRefN<=0)return;if(G.selectedRefN==1&&G.selectedRefs[0].type==SEL_IMAGE){G.snapGuideX=G.snapGuideY=0;return;}float x0,y0,x1,y1;if(!selection_bounds(&x0,&y0,&x1,&y1))return;float threshold=12.0f/fmax2(G.scale,.05f),bestX=threshold+1,bestY=threshold+1,fixX=0,fixY=0;float nx[3]={x0+*dx,(x0+x1)*.5f+*dx,x1+*dx},ny[3]={y0+*dy,(y0+y1)*.5f+*dy,y1+*dy};float sp=grid_spacing();for(int k=0;k<3;k++){float g=floorf(nx[k]/sp+0.5f)*sp,d=fabsf(g-nx[k]);if(d<bestX){bestX=d;fixX=g-nx[k];G.snapGuideWX=g;}g=floorf(ny[k]/sp+0.5f)*sp;d=fabsf(g-ny[k]);if(d<bestY){bestY=d;fixY=g-ny[k];G.snapGuideWY=g;}}
    if(G.visFrames)for(int i=0;i<G.frameN;i++)if(G.frames[i].active&&!ref_is_selected(SEL_FRAME,i)){FrameObj*f=&G.frames[i];snap_box_targets(f->x,f->y,f->x+f->w,f->y+f->h,nx,ny,&bestX,&bestY,&fixX,&fixY);}
    if(G.visNotes)for(int i=0;i<G.noteN;i++)if(G.notes[i].active&&!ref_is_selected(SEL_NOTE,i)){NoteObj*n=&G.notes[i];snap_box_targets(n->x,n->y,n->x+n->w,n->y+n->h,nx,ny,&bestX,&bestY,&fixX,&fixY);}
    if(G.visShapes)for(int i=0;i<G.shapeN;i++)if(G.shapes[i].active&&!ref_is_selected(SEL_SHAPE,i)){ShapeObj*q=&G.shapes[i];snap_box_targets(fmin2(q->x0,q->x1),fmin2(q->y0,q->y1),fmax2(q->x0,q->x1),fmax2(q->y0,q->y1),nx,ny,&bestX,&bestY,&fixX,&fixY);}
    if(G.visPhotos)for(int i=0;i<G.imageN;i++)if(G.images[i].active&&!ref_is_selected(SEL_IMAGE,i)){ImageObj*im=&G.images[i];snap_box_targets(im->x,im->y,im->x+im->w,im->y+im->h,nx,ny,&bestX,&bestY,&fixX,&fixY);}
    G.snapGuideX=bestX<=threshold;G.snapGuideY=bestY<=threshold;if(G.snapGuideX)*dx+=fixX;if(G.snapGuideY)*dy+=fixY;
}
static void selection_move_by(float dx,float dy){selection_snap_delta(&dx,&dy);for(int i=0;i<G.selectedRefN;i++)move_objref(G.selectedRefs[i],dx,dy);G.selectionDragTotalX+=dx;G.selectionDragTotalY+=dy;G.sceneRevision++;}
static int point_in_poly(float x,float y,Point*p,int n){int c=0;for(int i=0,j=n-1;i<n;j=i++){float yi=p[i].y,yj=p[j].y,xi=p[i].x,xj=p[j].x;if(((yi>y)!=(yj>y))&&(x<(xj-xi)*(y-yi)/(yj-yi+0.000001f)+xi))c=!c;}return c;}
static void lasso_commit(void){selection_clear();if(G.lassoN<3)return;for(int i=0;i<G.strokeN&&G.selectedRefN<256;i++){Stroke*st=&G.strokes[i];if(!st->active)continue;int aa=(st->color>>24)&255,marker=aa>0&&aa<248;if((marker&&!G.visMarker)||(!marker&&!G.visInk))continue;float cx=(st->minx+st->maxx)*.5f,cy=(st->miny+st->maxy)*.5f;if(point_in_poly(cx,cy,G.lassoPts,G.lassoN))G.selectedRefs[G.selectedRefN++]=(ObjRef){SEL_STROKES,i};}if(G.visCAD)for(int i=0;i<G.measureN&&G.selectedRefN<256;i++){Measure*m=&G.measures[i];if(m->active&&point_in_poly((m->ax+m->bx)*.5f,(m->ay+m->by)*.5f,G.lassoPts,G.lassoN))G.selectedRefs[G.selectedRefN++]=(ObjRef){SEL_MEASURE,i};}if(G.visNotes)for(int i=0;i<G.noteN&&G.selectedRefN<256;i++){NoteObj*n=&G.notes[i];if(n->active&&point_in_poly(n->x+n->w*.5f,n->y+n->h*.5f,G.lassoPts,G.lassoN))G.selectedRefs[G.selectedRefN++]=(ObjRef){SEL_NOTE,i};}if(G.visShapes)for(int i=0;i<G.shapeN&&G.selectedRefN<256;i++){ShapeObj*q=&G.shapes[i];if(q->active&&point_in_poly((q->x0+q->x1)*.5f,(q->y0+q->y1)*.5f,G.lassoPts,G.lassoN))G.selectedRefs[G.selectedRefN++]=(ObjRef){SEL_SHAPE,i};}if(G.selectedRefN==1){selection_set_one(G.selectedRefs[0].type,G.selectedRefs[0].index);}else if(G.selectedRefN>1)G.selectionType=SEL_GROUP;}
static int stroke_hit_screen(float sx,float sy){float best=(float)us(20),bi=-1;for(int i=G.strokeN-1;i>=0;i--){Stroke*st=&G.strokes[i];if(!st->active||st->n<1)continue;float bx0=st->minx*G.scale+G.offX-us(20),bx1=st->maxx*G.scale+G.offX+us(20),by0=st->miny*G.scale+G.offY-us(20),by1=st->maxy*G.scale+G.offY+us(20);if(sx<bx0||sx>bx1||sy<by0||sy>by1)continue;if(st->n==1){float d=sqrtf(sq(sx-(st->pts[0].x*G.scale+G.offX))+sq(sy-(st->pts[0].y*G.scale+G.offY)));if(d<best){best=d;bi=i;}}else for(int k=1;k<st->n;k++){float ax=st->pts[k-1].x*G.scale+G.offX,ay=st->pts[k-1].y*G.scale+G.offY,bx=st->pts[k].x*G.scale+G.offX,by=st->pts[k].y*G.scale+G.offY,d=point_seg_dist(sx,sy,ax,ay,bx,by);if(d<best){best=d;bi=i;}}}return (int)bi;}
static int shape_hit_screen(ShapeObj*q,float sx,float sy){
    if(!q||!q->active)return 0;float x0=q->x0*G.scale+G.offX,y0=q->y0*G.scale+G.offY,x1=q->x1*G.scale+G.offX,y1=q->y1*G.scale+G.offY,t=(float)us(18);
    if(q->type==SHAPE_LINE||q->type==SHAPE_ARROW)return point_seg_dist(sx,sy,x0,y0,x1,y1)<=t;
    float l=fmin2(x0,x1),r=fmax2(x0,x1),top=fmin2(y0,y1),bottom=fmax2(y0,y1);if(sx<l-t||sx>r+t||sy<top-t||sy>bottom+t)return 0;
    if(q->type==SHAPE_RECT){float d=point_seg_dist(sx,sy,l,top,r,top);d=fmin2(d,point_seg_dist(sx,sy,r,top,r,bottom));d=fmin2(d,point_seg_dist(sx,sy,r,bottom,l,bottom));d=fmin2(d,point_seg_dist(sx,sy,l,bottom,l,top));return d<=t;}
    float cx=(l+r)*.5f,cy=(top+bottom)*.5f,ax=(r-l)*.5f,ay=(bottom-top)*.5f;if(ax<1||ay<1)return sqrtf(sq(sx-cx)+sq(sy-cy))<=t;float px=cx+ax,py=cy,best=1000000.0f;for(int k=1;k<=48;k++){float a=(float)k*(2*PI/48.0f),xx=cx+cosf(a)*ax,yy=cy+sinf(a)*ay;best=fmin2(best,point_seg_dist(sx,sy,px,py,xx,yy));px=xx;py=yy;}return best<=t;
}
static ObjRef object_hit(float sx,float sy){float wx=(sx-G.offX)/G.scale,wy=(sy-G.offY)/G.scale;if(G.visNotes)for(int i=G.noteN-1;i>=0;i--){NoteObj*n=&G.notes[i];if(n->active&&wx>=n->x&&wx<=n->x+n->w&&wy>=n->y&&wy<=n->y+n->h)return(ObjRef){SEL_NOTE,i};}if(G.visShapes)for(int i=G.shapeN-1;i>=0;i--)if(shape_hit_screen(&G.shapes[i],sx,sy))return(ObjRef){SEL_SHAPE,i};int im=G.visPhotos?image_hit(sx,sy):-1;if(im>=0)return(ObjRef){SEL_IMAGE,im};int mm=G.visCAD?measure_hit(sx,sy):-1;if(mm>=0)return(ObjRef){SEL_MEASURE,mm};int st=(G.visInk||G.visMarker)?stroke_hit_screen(sx,sy):-1;if(st>=0)return(ObjRef){SEL_STROKES,st};if(G.visFrames)for(int i=G.frameN-1;i>=0;i--){FrameObj*f=&G.frames[i];if(f->active&&frame_contains_world(f,wx,wy))return(ObjRef){SEL_FRAME,i};}return(ObjRef){SEL_NONE,-1};}
static int frame_border_hit_screen(int fi,float sx,float sy){if(fi<0||fi>=G.frameN||!G.frames[fi].active)return 0;FrameObj*f=&G.frames[fi];float x0=f->x*G.scale+G.offX,y0=f->y*G.scale+G.offY,x1=(f->x+f->w)*G.scale+G.offX,y1=(f->y+f->h)*G.scale+G.offY;float t=(float)us(16);if(sx<x0-t||sx>x1+t||sy<y0-t||sy>y1+t)return 0;float d=fmin2(fmin2(fabsf(sx-x0),fabsf(sx-x1)),fmin2(fabsf(sy-y0),fabsf(sy-y1)));return d<=t;}
static void erase_everything_at(float sx,float sy){
    erase_at(sx,sy);int changed=G.eraseAction.n>0;
    /* Remove every unlocked object stacked beneath the pen. Frames require a
       border hit so working inside a frame cannot accidentally delete it. */
    for(int pass=0;pass<64;pass++){ObjRef r=object_hit(sx,sy);if(r.type==SEL_NONE)break;
        if(r.type==SEL_STROKES){if(!objref_can_mutate(r)){set_toast(9,0);break;}int before=G.eraseAction.n;erase_at(sx,sy);if(G.eraseAction.n==before)break;changed=1;continue;}
        if(r.type==SEL_FRAME&&!frame_border_hit_screen(r.index,sx,sy))break;
        if(!objref_can_mutate(r)){set_toast(9,0);break;}if(!action_add_ref(&G.eraseAction,r.type,r.index)){set_toast(8,0);break;}action_set_ref_active(r.type,r.index,0);changed=1;
    }
    if(changed){selection_clear();G.sceneRevision++;G.minimapDirty=1;}
}
static ObjRef finger_object_hit(float sx,float sy){ObjRef r=object_hit(sx,sy);if(r.type==SEL_IMAGE)return(ObjRef){SEL_NONE,-1};if(r.type==SEL_FRAME&&!frame_border_hit_screen(r.index,sx,sy))return(ObjRef){SEL_NONE,-1};return r;}
static int frame_corner_hit(float sx,float sy){if(G.selectedFrame<0||G.selectedFrame>=G.frameN||G.lockFrames||G.frameLocked[G.selectedFrame]||G.frameSizeLocked[G.selectedFrame])return -1;FrameObj*f=&G.frames[G.selectedFrame];float hx[4]={f->x*G.scale+G.offX,(f->x+f->w)*G.scale+G.offX,(f->x+f->w)*G.scale+G.offX,f->x*G.scale+G.offX};float hy[4]={f->y*G.scale+G.offY,f->y*G.scale+G.offY,(f->y+f->h)*G.scale+G.offY,(f->y+f->h)*G.scale+G.offY};float rr=(float)us(28);for(int i=0;i<4;i++)if(sq(sx-hx[i])+sq(sy-hy[i])<=rr*rr)return i;return -1;}
static void frame_resize_begin(int corner){if(G.selectedFrame<0||G.selectedFrame>=G.frameN)return;FrameObj*f=&G.frames[G.selectedFrame];G.frameGestureStart=*f;G.frameResizeDragging=1;G.frameResizeCorner=corner;G.frameResizeOppX=(corner==0||corner==3)?f->x+f->w:f->x;G.frameResizeOppY=(corner==0||corner==1)?f->y+f->h:f->y;}
static void frame_resize_to(float sx,float sy){if(!G.frameResizeDragging||G.selectedFrame<0||G.selectedFrame>=G.frameN)return;FrameObj*f=&G.frames[G.selectedFrame];float wx=(sx-G.offX)/G.scale,wy=(sy-G.offY)/G.scale,minW=fmax2(40.0f,(float)us(72)/fmax2(G.scale,.05f)),minH=minW;float l,r,t,b;if(G.frameResizeCorner==0||G.frameResizeCorner==3){r=G.frameResizeOppX;l=fmin2(wx,r-minW);}else{l=G.frameResizeOppX;r=fmax2(wx,l+minW);}if(G.frameResizeCorner==0||G.frameResizeCorner==1){b=G.frameResizeOppY;t=fmin2(wy,b-minH);}else{t=G.frameResizeOppY;b=fmax2(wy,t+minH);}f->x=l;f->y=t;f->w=r-l;f->h=b-t;G.frameRevision++;G.sceneRevision++;G.minimapDirty=1;}
static void selection_delete(void){OcrI32 removed[256];uint8_t removeMeasures[64],removeFrames[48];memset(removeMeasures,0,sizeof(removeMeasures));memset(removeFrames,0,sizeof(removeFrames));int rn=0,measureChanged=0,framesChanged=0;for(int i=0;i<G.selectedRefN;i++){ObjRef r=G.selectedRefs[i];if(!objref_can_mutate(r))continue;if(r.type==SEL_STROKES){G.strokes[r.index].active=0;if(rn<256)removed[rn++]=r.index;G.saveDirty=1;}else if(r.type==SEL_IMAGE){G.images[r.index].active=0;G.imagesDirty=G.imagePixelsDirty=1;}else if(r.type==SEL_MEASURE){removeMeasures[r.index]=1;measureChanged=1;}else if(r.type==SEL_NOTE)G.notes[r.index].active=0;else if(r.type==SEL_SHAPE)G.shapes[r.index].active=0;else if(r.type==SEL_FRAME){removeFrames[r.index]=1;framesChanged=1;}}
#ifdef VAST_OCR
    if(G.ocr&&rn>0)ocr_manager_set_strokes_active(G.ocr,removed,(OcrU32)rn,0);
#endif
    if(measureChanged){int old=G.measureN,out=0,map[64];for(int i=0;i<old;i++){if(G.measures[i].active&&!removeMeasures[i]){map[i]=out;G.measures[out++]=G.measures[i];}else map[i]=-1;}for(int i=out;i<old;i++)memset(&G.measures[i],0,sizeof(Measure));G.measureN=out;remap_object_refs(SEL_MEASURE,map,old);G.metaDirty=1;}
    if(framesChanged){int old=G.frameN,out=0,map[48];for(int i=0;i<old;i++){if(G.frames[i].active&&!removeFrames[i]){map[i]=out;if(out!=i){G.frames[out]=G.frames[i];memcpy(G.frameNames[out],G.frameNames[i],32);G.frameLocked[out]=G.frameLocked[i];G.frameSizeLocked[out]=G.frameSizeLocked[i];}out++;}else map[i]=-1;}for(int i=out;i<old;i++){memset(&G.frames[i],0,sizeof(FrameObj));memset(G.frameNames[i],0,32);G.frameLocked[i]=G.frameSizeLocked[i]=0;}G.frameN=out;remap_object_refs(SEL_FRAME,map,old);frame_compact_default_names();G.frameRevision++;}
    selection_clear();G.sceneRevision++;G.minimapDirty=1;save_all_document();}
static void selection_apply_color(void){for(int i=0;i<G.selectedRefN;i++){ObjRef r=G.selectedRefs[i];if(!objref_can_mutate(r))continue;if(r.type==SEL_STROKES){Stroke*st=&G.strokes[r.index];st->color=(st->color&0xff000000u)|(G.color&0xffffffu);G.saveDirty=1;}else if(r.type==SEL_NOTE){G.notes[r.index].color=G.color&0xffffffu;}else if(r.type==SEL_SHAPE){G.shapes[r.index].color=G.color&0xffffffu;}else if(r.type==SEL_FRAME){G.frames[r.index].color=G.color&0xffffffu;G.frameRevision++;}}G.sceneRevision++;G.minimapDirty=1;save_all_document();}
static void selection_duplicate(void){
    ocr_pending_flush();
    float dx=28.0f/G.scale,dy=28.0f/G.scale;ObjRef newer[256];OcrManagerDuplicate pairs[256];OcrManagerStroke destinations[256];int nn=0,pairN=0;
    for(int i=0;i<G.selectedRefN&&nn<256;i++){
        ObjRef r=G.selectedRefs[i];if(!objref_can_mutate(r))continue;
        if(r.type==SEL_STROKES&&r.index>=0&&r.index<G.strokeN){
            int destination=G.strokeN;if(!ensure_strokes(G.strokeN+1))continue;Stroke*src=&G.strokes[r.index],*d=&G.strokes[destination];memset(d,0,sizeof(*d));d->active=1;d->color=src->color;d->baseWidth=src->baseWidth;if(!ensure_points(d,src->n)){memset(d,0,sizeof(*d));continue;}for(int k=0;k<src->n;k++){d->pts[k]=src->pts[k];d->pts[k].x+=dx;d->pts[k].y+=dy;}d->n=src->n;d->minx=src->minx+dx;d->maxx=src->maxx+dx;d->miny=src->miny+dy;d->maxy=src->maxy+dy;newer[nn++]=(ObjRef){SEL_STROKES,destination};G.strokeN++;pairs[pairN++]=(OcrManagerDuplicate){r.index,destination};G.saveDirty=1;
        }else if(r.type==SEL_IMAGE&&r.index>=0&&r.index<G.imageN&&G.imageN<32){ImageObj src=G.images[r.index],*d=&G.images[G.imageN];*d=src;d->x+=dx;d->y+=dy;d->px=0;if(src.px&&src.pw>0&&src.ph>0){size_t npx=(size_t)src.pw*(size_t)src.ph;d->px=(uint32_t*)malloc(npx*sizeof(uint32_t));if(d->px)memcpy(d->px,src.px,npx*sizeof(uint32_t));}if(!src.px||d->px){newer[nn++]=(ObjRef){SEL_IMAGE,G.imageN++};G.imagesDirty=G.imagePixelsDirty=1;}}
        else if(r.type==SEL_MEASURE&&r.index>=0&&r.index<G.measureN&&G.measureN<64){Measure m=G.measures[r.index];m.ax+=dx;m.bx+=dx;m.ay+=dy;m.by+=dy;m.active=1;G.measures[G.measureN]=m;newer[nn++]=(ObjRef){SEL_MEASURE,G.measureN++};G.metaDirty=1;}
        else if(r.type==SEL_NOTE&&r.index>=0&&r.index<G.noteN&&G.noteN<96){NoteObj n=G.notes[r.index];n.id=G.nextNoteId++;n.x+=dx;n.y+=dy;G.notes[G.noteN]=n;newer[nn++]=(ObjRef){SEL_NOTE,G.noteN++};}
        else if(r.type==SEL_SHAPE&&r.index>=0&&r.index<G.shapeN&&G.shapeN<128){ShapeObj q=G.shapes[r.index];q.id=G.nextShapeId++;q.x0+=dx;q.x1+=dx;q.y0+=dy;q.y1+=dy;G.shapes[G.shapeN]=q;newer[nn++]=(ObjRef){SEL_SHAPE,G.shapeN++};}
        else if(r.type==SEL_FRAME&&r.index>=0&&r.index<G.frameN&&G.frameN<48){FrameObj f=G.frames[r.index];f.id=G.nextFrameId++;f.x+=dx;f.y+=dy;int fi=G.frameN++;G.frames[fi]=f;snprintf(G.frameNames[fi],32,"%.24s copy",G.frameNames[r.index]);G.frameLocked[fi]=0;G.frameSizeLocked[fi]=0;newer[nn++]=(ObjRef){SEL_FRAME,fi};}
    }
#ifdef VAST_OCR
    if(G.ocr&&pairN>0){for(int i=0;i<pairN;i++)ocr_fill_stroke(pairs[i].destination_runtime_index,0,destinations+i);ocr_manager_duplicate_strokes(G.ocr,destinations,pairs,(OcrU32)pairN);}
#endif
    if(nn==1)selection_set_one(newer[0].type,newer[0].index);else if(nn>1){memcpy(G.selectedRefs,newer,(size_t)nn*sizeof(ObjRef));G.selectedRefN=nn;G.selectionType=SEL_GROUP;}G.sceneRevision++;G.minimapDirty=1;save_all_document();
}
static void selection_group(void){if(G.selectedRefN<2||G.groupN>=32)return;GroupObj*g=&G.groups[G.groupN++];memset(g,0,sizeof(*g));g->id=G.nextGroupId++;g->active=1;g->n=mini(G.selectedRefN,48);for(int i=0;i<g->n;i++)g->refs[i]=G.selectedRefs[i];G.selectedGroup=G.groupN-1;G.selectionType=SEL_GROUP;save_workspace();}
static void selection_ungroup(void){if(G.selectedGroup<0||G.selectedGroup>=G.groupN)return;G.groups[G.selectedGroup].active=0;G.selectedGroup=-1;G.selectionType=G.selectedRefN==1?G.selectedRefs[0].type:(G.selectedRefN>1?SEL_GROUP:SEL_NONE);G.sceneRevision++;save_workspace();}
static void draw_ocr_highlight(Surf*s){if(G.ocrHighlightAlpha<=0.0f)return;float a=clampf(G.ocrHighlightAlpha,0,1),pulse=1.0f-a;float pad=(float)us(12)+(float)us(20)*pulse,x0=G.ocrHighlightBounds.minx*G.scale+G.offX-pad,y0=G.ocrHighlightBounds.miny*G.scale+G.offY-pad,x1=G.ocrHighlightBounds.maxx*G.scale+G.offX+pad,y1=G.ocrHighlightBounds.maxy*G.scale+G.offY+pad;int aa=(int)(215.0f*a);aa_hline(s,y0,(int)x0,(int)x1,C_CYAN,aa);aa_hline(s,y1,(int)x0,(int)x1,C_CYAN,aa);aa_vline(s,x0,(int)y0,(int)y1,C_CYAN,aa);aa_vline(s,x1,(int)y0,(int)y1,C_CYAN,aa);}
static void draw_selection_overlay(Surf*s){if(G.selectedRefN<=0)return;float x0,y0,x1,y1;if(!selection_bounds(&x0,&y0,&x1,&y1))return;float sx0=x0*G.scale+G.offX,sy0=y0*G.scale+G.offY,sx1=x1*G.scale+G.offX,sy1=y1*G.scale+G.offY;aa_hline(s,sy0,(int)sx0,(int)sx1,C_ACCENT,205);aa_hline(s,sy1,(int)sx0,(int)sx1,C_ACCENT,205);aa_vline(s,sx0,(int)sy0,(int)sy1,C_ACCENT,205);aa_vline(s,sx1,(int)sy0,(int)sy1,C_ACCENT,205);int singleType=G.selectedRefN==1?G.selectedRefs[0].type:SEL_GROUP;int showHandles=singleType!=SEL_STROKES&&singleType!=SEL_IMAGE;if(singleType==SEL_FRAME&&G.selectedFrame>=0&&G.selectedFrame<G.frameN&&G.frameSizeLocked[G.selectedFrame])showHandles=0;if(showHandles){float hx[4]={sx0,sx1,sx1,sx0},hy[4]={sy0,sy0,sy1,sy1};float outer=singleType==SEL_FRAME?us(8):us(6),inner=singleType==SEL_FRAME?us(4):us(3);for(int i=0;i<4;i++){aa_circle(s,hx[i],hy[i],outer,C_BG,220);aa_circle(s,hx[i],hy[i],inner,C_ACCENT,240);}}if(G.snapGuideX){float gx=G.snapGuideWX*G.scale+G.offX;aa_vline(s,gx,0,s->h,C_ACCENT,118);}if(G.snapGuideY){float gy=G.snapGuideWY*G.scale+G.offY;aa_hline(s,gy,0,s->w,C_ACCENT,118);}}
static void draw_lasso(Surf*s){if(!G.lassoActive||G.lassoN<2)return;for(int i=1;i<G.lassoN;i++)aa_capsule(s,G.lassoPts[i-1].x*G.scale+G.offX,G.lassoPts[i-1].y*G.scale+G.offY,1.2f,G.lassoPts[i].x*G.scale+G.offX,G.lassoPts[i].y*G.scale+G.offY,1.2f,C_CYAN,220);}
static void world_bounds(float*minx,float*miny,float*maxx,float*maxy){float vx0=(0-G.offX)/G.scale,vy0=(0-G.offY)/G.scale,vx1=(G.screenW-G.offX)/G.scale,vy1=(G.screenH-G.offY)/G.scale;*minx=fmin2(vx0,vx1);*maxx=fmax2(vx0,vx1);*miny=fmin2(vy0,vy1);*maxy=fmax2(vy0,vy1);for(int i=0;i<G.strokeN;i++){Stroke*st=&G.strokes[i];if(!st->active||st->n<=0)continue;*minx=fmin2(*minx,st->minx);*maxx=fmax2(*maxx,st->maxx);*miny=fmin2(*miny,st->miny);*maxy=fmax2(*maxy,st->maxy);}for(int i=0;i<G.imageN;i++){ImageObj*im=&G.images[i];if(!im->active)continue;float x[4],y[4];image_world_corners(im,x,y);for(int k=0;k<4;k++){*minx=fmin2(*minx,x[k]);*maxx=fmax2(*maxx,x[k]);*miny=fmin2(*miny,y[k]);*maxy=fmax2(*maxy,y[k]);}}for(int i=0;i<G.measureN;i++){Measure*m=&G.measures[i];if(!m->active)continue;*minx=fmin2(*minx,fmin2(m->ax,m->bx));*maxx=fmax2(*maxx,fmax2(m->ax,m->bx));*miny=fmin2(*miny,fmin2(m->ay,m->by));*maxy=fmax2(*maxy,fmax2(m->ay,m->by));}for(int i=0;i<G.noteN;i++){NoteObj*n=&G.notes[i];if(!n->active)continue;*minx=fmin2(*minx,n->x);*maxx=fmax2(*maxx,n->x+n->w);*miny=fmin2(*miny,n->y);*maxy=fmax2(*maxy,n->y+n->h);}for(int i=0;i<G.shapeN;i++){ShapeObj*q=&G.shapes[i];if(!q->active)continue;*minx=fmin2(*minx,fmin2(q->x0,q->x1));*maxx=fmax2(*maxx,fmax2(q->x0,q->x1));*miny=fmin2(*miny,fmin2(q->y0,q->y1));*maxy=fmax2(*maxy,fmax2(q->y0,q->y1));}for(int i=0;i<G.frameN;i++){FrameObj*f=&G.frames[i];if(!f->active)continue;*minx=fmin2(*minx,f->x);*maxx=fmax2(*maxx,f->x+f->w);*miny=fmin2(*miny,f->y);*maxy=fmax2(*maxy,f->y+f->h);}if(*maxx-*minx<10){*minx-=5;*maxx+=5;}if(*maxy-*miny<10){*miny-=5;*maxy+=5;}float padx=(*maxx-*minx)*.10f,pady=(*maxy-*miny)*.10f;*minx-=padx;*maxx+=padx;*miny-=pady;*maxy+=pady;}
static void draw_frames(Surf*s);
static void rebuild_minimap_cache(int cw,int ch){
    if(cw<=8||ch<=8)return;size_t need=(size_t)cw*(size_t)ch;if(!G.minimapCache||G.minimapCacheW!=cw||G.minimapCacheH!=ch){if(G.minimapCache)free(G.minimapCache);G.minimapCache=(uint32_t*)malloc(need*sizeof(uint32_t));G.minimapCacheW=cw;G.minimapCacheH=ch;}if(!G.minimapCache)return;
    float minx,miny,maxx,maxy;world_bounds(&minx,&miny,&maxx,&maxy);float rx=(cw-4.0f)/(maxx-minx),ry=(ch-4.0f)/(maxy-miny),r=fmin2(rx,ry);float ox=2.0f+(cw-4.0f-(maxx-minx)*r)*.5f-minx*r,oy=2.0f+(ch-4.0f-(maxy-miny)*r)*.5f-miny*r;
    float os=G.scale,oox=G.offX,ooy=G.offY;int ofc=G.fingerCount,osd=G.stylusDown,opm=G.photoMode,osi=G.selectedImage,omr=G.mapRenderMode;G.scale=r;G.offX=ox;G.offY=oy;G.fingerCount=0;G.stylusDown=0;G.photoMode=0;G.selectedImage=-1;G.mapRenderMode=1;
    Surf ms={(uint8_t*)G.minimapCache,cw,ch,cw,FORMAT_RGBX8888};fill(&ms,C_BG);draw_grid(&ms);draw_frames(&ms);draw_images(&ms);draw_notes(&ms);draw_shapes(&ms);draw_strokes(&ms);for(int i=0;i<G.measureN;i++){Measure*m=&G.measures[i];if(m->active)aa_capsule(&ms,m->ax*r+ox,m->ay*r+oy,.65f,m->bx*r+ox,m->by*r+oy,.65f,C_CYAN,220);}G.scale=os;G.offX=oox;G.offY=ooy;G.fingerCount=ofc;G.stylusDown=osd;G.photoMode=opm;G.selectedImage=osi;G.mapRenderMode=omr;G.minimapMinX=minx;G.minimapMinY=miny;G.minimapMaxX=maxx;G.minimapMaxY=maxy;G.minimapDirty=0;
}
static void draw_minimap(Surf*s){if(!G.minimap&&G.minimapAnim<.015f)return;float a=clampf(G.minimapAnim,-.10f,1.14f);int m=margin_ui(),w=us(320),h=us(205),x=s->w-m-w+(int)((1-a)*us(46)),y=s->h-m-h-us(100)+(int)((1-a)*us(32));int active=G.fingerCount>0||G.stylusDown||G.framePanel||G.selectedFrame>=0||G.presentationMode;if(active){glass(s,x,y,x+w,y+h,us(20));text(s,x+us(16),y+us(12),"Overview",ts(1),C_MUTED);}else{alpha_round_rect(s,x,y,x+w,y+h,us(18),C_BORDER,46);alpha_round_rect(s,x+us(1),y+us(1),x+w-us(1),y+h-us(1),us(17),C_GLASS,76);}int pad=us(14),top=us(40),cw=w-2*pad,ch=h-top-pad;if(G.minimapCache&&G.minimapCacheW==cw&&G.minimapCacheH==ch){int dx=x+pad,dy=y+top;for(int yy=0;yy<ch;yy++){if(dy+yy<0||dy+yy>=s->h)continue;uint32_t*src=G.minimapCache+yy*cw;if(s->fmt==FORMAT_RGB565){uint16_t*dst=(uint16_t*)s->bits+(dy+yy)*s->stride;for(int xx=0;xx<cw;xx++){int xx2=dx+xx;if((unsigned)xx2>=(unsigned)s->w)continue;uint32_t q=src[xx],rgb=((q&255)<<16)|(((q>>8)&255)<<8)|((q>>16)&255);dst[xx2]=pack565(rgb);}}else{uint32_t*dst=(uint32_t*)s->bits+(dy+yy)*s->stride;int sx0=maxi(0,-dx),sx1=mini(cw,s->w-dx);if(sx1>sx0)memcpy(dst+dx+sx0,src+sx0,(size_t)(sx1-sx0)*sizeof(uint32_t));}}float minx=G.minimapMinX,miny=G.minimapMinY,maxx=G.minimapMaxX,maxy=G.minimapMaxY;float rx=(cw-4.0f)/(maxx-minx),ry=(ch-4.0f)/(maxy-miny),r=fmin2(rx,ry),ox=dx+2.0f+(cw-4.0f-(maxx-minx)*r)*.5f-minx*r,oy=dy+2.0f+(ch-4.0f-(maxy-miny)*r)*.5f-miny*r;float vx0=(0-G.offX)/G.scale,vy0=(0-G.offY)/G.scale,vx1=(G.screenW-G.offX)/G.scale,vy1=(G.screenH-G.offY)/G.scale;float qx0=ox+fmin2(vx0,vx1)*r,qx1=ox+fmax2(vx0,vx1)*r,qy0=oy+fmin2(vy0,vy1)*r,qy1=oy+fmax2(vy0,vy1)*r;aa_hline(s,qy0,(int)qx0,(int)qx1,C_ACCENT,255);aa_hline(s,qy1,(int)qx0,(int)qx1,C_ACCENT,255);aa_vline(s,qx0,(int)qy0,(int)qy1,C_ACCENT,255);aa_vline(s,qx1,(int)qy0,(int)qy1,C_ACCENT,255);for(int bi=0;bi<G.bookmarkN;bi++){BookmarkObj*b=&G.bookmarks[bi];if(!b->active)continue;float bx=ox+b->x*r,by=oy+b->y*r;aa_circle(s,bx,by,(float)us(3),C_WARN,235);}}}

static void calibration_reset_text(void){G.calibLen=0;G.calibText[0]=0;G.calibValue=0;}
static void calibration_append(char c){if(G.calibLen>=20)return;if(c=='.'){for(int i=0;i<G.calibLen;i++)if(G.calibText[i]=='.')return;if(G.calibLen==0){G.calibText[G.calibLen++]='0';}}G.calibText[G.calibLen++]=c;G.calibText[G.calibLen]=0;}
static void calibration_backspace(void){if(G.calibLen>0)G.calibText[--G.calibLen]=0;}
static float parse_simple_decimal(const char*s0){float v=0,frac=.1f;int dot=0;for(int i=0;s0[i];i++){char c=s0[i];if(c=='.'){dot=1;continue;}if(c<'0'||c>'9')continue;if(!dot)v=v*10+(c-'0');else{v+=(c-'0')*frac;frac*=.1f;}}return v;}
static void open_calibration(void){if(G.selectedMeasure<0||G.selectedMeasure>=G.measureN||G.lockCAD)return;G.calibrationOpen=1;calibration_reset_text();G.calibText[0]='1';G.calibText[1]='.';G.calibText[2]='0';G.calibText[3]='0';G.calibText[4]=0;G.calibLen=4;system_text_start(G.calibText,6);}
static void draw_calibration(Surf*s){if(!G.calibrationOpen)return;int w=us(520),h=us(650),x=(s->w-w)/2,y=(s->h-h)/2;glass(s,x,y,x+w,y+h,us(30));text(s,x+us(28),y+us(24),"Calibrate dimension",ts(3),C_TEXT);text(s,x+us(28),y+us(64),"Enter the real length of the selected dimension",ts(2),C_MUTED);round_rect(s,x+us(28),y+us(106),x+w-us(28),y+us(176),us(18),C_BG);char val[64];snprintf(val,sizeof(val),"%s %s",G.calibLen?G.calibText:"0",cad_unit_name());int sc=ts(4),tw=text_w(val,sc);text(s,x+w/2-tw/2,y+us(126),val,sc,C_CYAN);button(s,x+us(28),y+us(194),x+w-us(28),y+us(242),"Change unit",0,C_VIOLET);const char*keys[12]={"1","2","3","4","5","6","7","8","9",".","0","DEL"};for(int i=0;i<12;i++){int col=i%3,row=i/3,bx=x+us(28)+col*us(154),by=y+us(266)+row*us(72);button(s,bx,by,bx+us(138),by+us(58),keys[i],0,i==11?C_DANGER:(i==9?C_VIOLET:C_TEXT));}button(s,x+us(28),y+h-us(76),x+us(226),y+h-us(24),"Cancel",0,C_DANGER);button(s,x+us(242),y+h-us(76),x+w-us(28),y+h-us(24),"Apply",1,C_GREEN);}
static const char*button_action_name(int a){static const char*n[BA_COUNT]={"NONE","Undo","REDO","QUICK ERASE","ERASER","PEN","MEASURE","FIT VIEW","NEXT COLOR","PRESSURE","Settings","IMPORT","HIGHLIGHT","COLOR PICKER"};return (a>=0&&a<BA_COUNT)?n[a]:"NONE";}
static const char*theme_role_name(int r){static const char*n[8]={"Background","Panel","Grid","Text","Accent","CAD","Violet","Danger"};return (r>=0&&r<8)?n[r]:"COLOR";}
static const uint32_t THEME_PALETTE[12]={0x000000,0x090d13,0x172033,0xf4f7fb,0xe9e2d4,0x6aa9ff,0x4de0d0,0xb18cff,0xff6475,0xffc55d,0x5fe39a,0xff9f63};
static const char*display_level_name(int v){static const char*n[3]={"Off","Subtle","Full"};return n[maxi(0,mini(2,v))];}
static const char*grid_style_name(void){static const char*n[4]={"Dots","Crosses","Blueprint","Minimal"};return n[maxi(0,mini(3,G.gridStyle))];}
static const char*edge_glass_name(void){static const char*n[5]={"Off","Subtle","Medium","Full","Insane"};return n[maxi(0,mini(4,G.edgeGlass))];}
static const char*plane_name(void){static const char*n[3]={"Flat","Raised","Floating"};return n[maxi(0,mini(2,G.planeElevation))];}
static const char*motion_name(void){static const char*n[3]={"Calm","Playful","Bouncy"};return n[maxi(0,mini(2,G.motionStyle))];}
static const char*far_zoom_name(void){static const char*n[3]={"Simple","Cluster","Constellation"};return n[maxi(0,mini(2,G.farZoomMode))];}
static const char*perf_name(void){static const char*n[3]={"Battery","Balanced","Max smooth"};return n[maxi(0,mini(2,G.performanceMode))];}
static const char*ocr_status_name(OcrManagerStatus*out){if(out)memset(out,0,sizeof(*out));if(!G.ocr){return G.ocrInitFailed?"Unavailable":"Starting...";}
#ifdef VAST_OCR
    OcrManagerStatus s;ocr_manager_get_status(G.ocr,&s);if(out)*out=s;if(s.state==OCR_MANAGER_DISABLED)return "Disabled";if(s.state==OCR_MANAGER_MODEL_FAILED)return "Unavailable";if(s.state==OCR_MANAGER_LOADING_CACHE)return "Loading...";if(!s.initial_scan_finished)return "Indexing...";if(s.state==OCR_MANAGER_INDEXING)return "Indexing...";if(s.state==OCR_MANAGER_REBUILDING)return "Rebuilding...";return "Ready";
#else
    return "Offline OCR not linked in host test build";
#endif
}
static void draw_settings_panel(Surf*s){if(!G.settingsPanel)return;int m=margin_ui(),w=mini(us(1040),s->w-2*m),h=mini(us(780),s->h-2*m),x=(s->w-w)/2,y=(s->h-h)/2;glass(s,x,y,x+w,y+h,us(30));text(s,x+us(30),y+us(26),"Settings",ts(3),C_TEXT);button(s,x+w-us(144),y+us(18),x+w-us(24),y+us(62),"Close",0,C_DANGER);const char*tabs[5]={"Appearance","Stylus","Drawing","Display","Handwriting"};int gap=us(8),tw=(w-us(60)-gap*4)/5;for(int i=0;i<5;i++){int tx=x+us(30)+i*(tw+gap);button(s,tx,y+us(82),tx+tw,y+us(132),tabs[i],G.settingsTab==i,C_ACCENT);}hline(s,y+us(150),x+us(30),x+w-us(30),C_BORDER);
    if(G.settingsTab==0){int yy=y+us(178);char q[80];text(s,x+us(34),yy,"Interface scale",ts(2),C_MUTED);snprintf(q,sizeof(q),"%.0f%%",G.uiScale*100.0f);button(s,x+us(218),yy-us(12),x+us(298),yy+us(34),"-",0,C_ACCENT);button(s,x+us(310),yy-us(12),x+us(440),yy+us(34),q,1,C_ACCENT);button(s,x+us(452),yy-us(12),x+us(532),yy+us(34),"+",0,C_ACCENT);yy+=us(72);text(s,x+us(34),yy,"Curated themes",ts(2),C_MUTED);for(int i=0;i<THEME_PRESET_COUNT;i++){int col=i%8,row=i/8,bx=x+us(34+col*118),by=yy+us(30+row*44);button(s,bx,by,bx+us(110),by+us(38),theme_preset_name(i),theme_is_preset(i),THEME_PRESETS[i].accent);}yy+=us(165);text(s,x+us(34),yy,"Color role",ts(2),C_MUTED);for(int i=0;i<8;i++){int col=i%4,row=i/4,bx=x+us(34+col*235),by=yy+us(28+row*48);button(s,bx,by,bx+us(215),by+us(40),theme_role_name(i),G.themeRole==i,i==7?C_DANGER:(i==5?C_CYAN:(i==6?C_VIOLET:C_ACCENT)));}yy+=us(124);text(s,x+us(34),yy,"Palette",ts(2),C_MUTED);for(int i=0;i<12;i++){int col=i%6,row=i/6;float cx=(float)(x+us(120+col*135)),cy=(float)(yy+us(36+row*58));circle(s,cx,cy,us(23),C_BORDER);circle(s,cx,cy,us(18),THEME_PALETTE[i]);}yy+=us(140);text(s,x+us(34),yy,"Customize each role, then fine-tune the whole interface scale.",ts(2),C_MUTED);button(s,x+w-us(220),yy-us(12),x+w-us(34),yy+us(36),"Reset theme",0,C_DANGER);
    }else if(G.settingsTab==1){int yy=y+us(180);text(s,x+us(34),yy,"Stylus button mappings",ts(2),C_TEXT);button(s,x+us(286),yy-us(14),x+us(510),yy+us(36),G.buttonBindingsEnabled?"Enabled":"Disabled",G.buttonBindingsEnabled,G.buttonBindingsEnabled?C_GREEN:C_MUTED);text(s,x+us(536),yy,G.buttonBindingsEnabled?(G.stylusButtonDown?"Signal active":"Waiting for button") : "Button events ignored",ts(2),G.stylusButtonDown?C_GREEN:C_MUTED);yy+=us(82);const char*labs[3]={"Press","Hold","Double"};int acts[3]={G.buttonPressAction,G.buttonHoldAction,G.buttonDoubleAction};for(int r=0;r<3;r++){uint32_t ac=G.buttonBindingsEnabled?(r==1?C_VIOLET:(r==2?C_CYAN:C_ACCENT)):C_MUTED;text(s,x+us(34),yy+us(10),labs[r],ts(2),ac);button(s,x+us(210),yy-us(4),x+us(270),yy+us(46),"-",0,C_MUTED);button(s,x+us(282),yy-us(4),x+us(710),yy+us(46),button_action_name(acts[r]),G.buttonBindingsEnabled,ac);button(s,x+us(722),yy-us(4),x+us(782),yy+us(46),"+",0,C_MUTED);yy+=us(72);}yy+=us(26);text(s,x+us(34),yy,"Leave mappings disabled for pens that never expose side-button events.",ts(2),C_MUTED);text(s,x+us(34),yy+us(34),"Enable them on another tablet only if the signal indicator reacts.",ts(2),C_MUTED);
    }else if(G.settingsTab==2){int yy=y+us(184);char q[96];text(s,x+us(34),yy,"Pressure smoothing",ts(2),C_TEXT);snprintf(q,sizeof(q),"%.0f%%",G.pressureSmoothing*100);meter(s,x+us(300),yy+us(8),us(300),G.pressureSmoothing,C_VIOLET);button(s,x+us(620),yy-us(4),x+us(680),yy+us(46),"-",0,C_MUTED);button(s,x+us(692),yy-us(4),x+us(812),yy+us(46),q,1,C_VIOLET);button(s,x+us(824),yy-us(4),x+us(884),yy+us(46),"+",0,C_MUTED);yy+=us(88);text(s,x+us(34),yy,"Stroke smoothing",ts(2),C_TEXT);snprintf(q,sizeof(q),"%.0f%%",G.strokeSmoothing*100);meter(s,x+us(300),yy+us(8),us(300),G.strokeSmoothing,C_CYAN);button(s,x+us(620),yy-us(4),x+us(680),yy+us(46),"-",0,C_MUTED);button(s,x+us(692),yy-us(4),x+us(812),yy+us(46),q,1,C_CYAN);button(s,x+us(824),yy-us(4),x+us(884),yy+us(46),"+",0,C_MUTED);yy+=us(88);text(s,x+us(34),yy,"Highlighter opacity",ts(2),C_TEXT);snprintf(q,sizeof(q),"%.0f%%",G.highlighterOpacity*100);meter(s,x+us(300),yy+us(8),us(300),G.highlighterOpacity/.80f,C_WARN);button(s,x+us(620),yy-us(4),x+us(680),yy+us(46),"-",0,C_MUTED);button(s,x+us(692),yy-us(4),x+us(812),yy+us(46),q,1,C_WARN);button(s,x+us(824),yy-us(4),x+us(884),yy+us(46),"+",0,C_MUTED);yy+=us(88);text(s,x+us(34),yy,"Radial menu hold",ts(2),C_TEXT);snprintf(q,sizeof(q),"%.1f s",G.radialHoldSec);meter(s,x+us(300),yy+us(8),us(300),(G.radialHoldSec-.35f)/2.65f,C_GREEN);button(s,x+us(620),yy-us(4),x+us(680),yy+us(46),"-",0,C_MUTED);button(s,x+us(692),yy-us(4),x+us(812),yy+us(46),q,1,C_GREEN);button(s,x+us(824),yy-us(4),x+us(884),yy+us(46),"+",0,C_MUTED);yy+=us(96);text(s,x+us(34),yy,"The marker is now a flat rectangular ribbon with constant width.",ts(2),C_MUTED);text(s,x+us(34),yy+us(34),"Hold a finger on empty canvas for the radial menu; tune 0.4 to 3.0 s.",ts(2),C_MUTED);
    }else if(G.settingsTab==3){int yy=y+us(184);const char*labs[3]={"Grid style","Far zoom","Performance"};const char*vals[3]={grid_style_name(),far_zoom_name(),perf_name()};uint32_t ac[3]={C_ACCENT,C_CYAN,C_GREEN};for(int r=0;r<3;r++){text(s,x+us(34),yy+us(10),labs[r],ts(2),C_TEXT);button(s,x+us(560),yy-us(4),x+us(620),yy+us(46),"-",0,C_MUTED);button(s,x+us(632),yy-us(4),x+us(820),yy+us(46),vals[r],1,ac[r]);button(s,x+us(832),yy-us(4),x+us(892),yy+us(46),"+",0,C_MUTED);yy+=us(76);}yy+=us(18);text(s,x+us(34),yy,"Only controls that affect the simplified canvas remain here.",ts(2),C_MUTED);text(s,x+us(34),yy+us(34),G.gpuActive?"GPU renderer active":"GPU renderer unavailable - software fallback",ts(2),G.gpuActive?C_GREEN:C_WARN);
    }else{int yy=y+us(184);OcrManagerStatus st;const char*status=ocr_status_name(&st);text(s,x+us(34),yy,"Offline handwriting search",ts(3),C_TEXT);button(s,x+w-us(286),yy-us(10),x+w-us(34),yy+us(44),G.ocrEnabled?"Enabled":"Disabled",G.ocrEnabled,G.ocrEnabled?C_GREEN:C_MUTED);yy+=us(94);text(s,x+us(34),yy,"Status",ts(2),C_MUTED);text(s,x+us(184),yy,status,ts(2),st.state==OCR_MANAGER_MODEL_FAILED||G.ocrInitFailed?C_DANGER:(st.state==OCR_MANAGER_INDEXING||st.state==OCR_MANAGER_REBUILDING?C_WARN:C_GREEN));yy+=us(84);char q[96];snprintf(q,sizeof(q),"%u indexed regions  -  %u queued",st.active_record_count,st.dirty_region_count);text(s,x+us(34),yy,q,ts(2),C_MUTED);yy+=us(86);button(s,x+us(34),yy-us(12),x+us(376),yy+us(46),"Rebuild handwriting index",0,C_ACCENT);yy+=us(76);text(s,x+us(34),yy,"How offline OCR works",ts(2),C_TEXT);yy+=us(34);text(s,x+us(34),yy,"About 1 second after input stops, nearby pen strokes form likely text regions.",ts(1),C_MUTED);yy+=us(28);text(s,x+us(34),yy,"Each region becomes a small image read by the bundled on-device OCR model.",ts(1),C_MUTED);yy+=us(28);text(s,x+us(34),yy,"Search combines notes, frames, places and recognized ink; tap a result to jump.",ts(1),C_MUTED);yy+=us(28);text(s,x+us(34),yy,"OCR reads vector ink only - not photos, the grid, backgrounds or interface controls.",ts(1),C_MUTED);yy+=us(28);text(s,x+us(34),yy,"Recognition can make mistakes, and results may be incomplete while work is queued.",ts(1),C_MUTED);yy+=us(28);text(s,x+us(34),yy,"Ink, recognized text and searches stay on this device; no network is used.",ts(1),C_MUTED);yy+=us(28);text(s,x+us(34),yy,"Rebuild keeps every stroke and recreates only the disposable OCR search index.",ts(1),C_MUTED);}
}

static void draw_gallery(Surf*s){if(!G.galleryOpen)return;int m=margin_ui(),x=m,y=m,w=s->w-2*m,h=s->h-2*m;glass(s,x,y,x+w,y+h,us(30));text(s,x+us(30),y+us(24),"Import photos",ts(3),C_TEXT);char q[64];int pages=(G.mediaCount+11)/12;snprintf(q,sizeof(q),"Page %d / %d",G.galleryPage+1,maxi(1,pages));text(s,x+us(320),y+us(30),q,ts(2),C_MUTED);button(s,x+w-us(144),y+us(18),x+w-us(24),y+us(62),"Close",0,C_DANGER);int top=y+us(92),bottom=y+h-us(86),gw=w-us(60),gh=bottom-top,cellW=gw/4,cellH=gh/3;for(int i=0;i<G.thumbN;i++){int col=i%4,row=i/4,cx=x+us(30)+col*cellW,cy=top+row*cellH;round_rect(s,cx+us(8),cy+us(8),cx+cellW-us(8),cy+cellH-us(8),us(18),C_GLASS2);Thumb*t=&G.thumbs[i];if(t->px){int aw=cellW-us(30),ah=cellH-us(30);float r=fmin2((float)aw/t->w,(float)ah/t->h);int dw=(int)(t->w*r),dh=(int)(t->h*r),ix=cx+(cellW-dw)/2,iy=cy+(cellH-dh)/2;draw_image_pixels(s,t->px,t->w,t->h,ix,iy,ix+dw,iy+dh);hline(s,iy,ix,ix+dw,C_BORDER);hline(s,iy+dh-1,ix,ix+dw,C_BORDER);vline(s,ix,iy,iy+dh,C_BORDER);vline(s,ix+dw-1,iy,iy+dh,C_BORDER);}}button(s,x+us(30),y+h-us(64),x+us(190),y+h-us(18),"Previous",G.galleryPage>0,C_ACCENT);button(s,x+w-us(190),y+h-us(64),x+w-us(30),y+h-us(18),"Next",G.galleryPage+1<pages,C_ACCENT);text(s,x+us(220),y+h-us(48),"Tap a photo to place it at the center of your view",ts(2),C_MUTED);}
static void draw_gesture_hint(Surf*s){(void)s;}
static int eraser_contact_visible(void){return G.stylusDown&&G.erasing&&(G.stylusToolType==TOOL_STYLUS||G.stylusToolType==TOOL_ERASER)&&!G.settingsPanel&&!G.galleryOpen&&!G.calibrationOpen&&!G.radialOpen;}
static void draw_hover_preview(Surf*s){if(G.gpuActive||!eraser_contact_visible())return;float x=G.hoverX,y=G.hoverY;if(x<0||y<0||x>s->w||y>s->h)return;float r=26.0f;aa_circle(s,x,y,r,C_DANGER,190);aa_circle(s,x,y,r-1.5f,C_BG,150);}
static void draw_ink_toggle(Surf*s){if(G.presentationMode||G.settingsPanel||G.galleryOpen||G.calibrationOpen||G.projectsPanel||G.editorOpen)return;float r=(float)us(88),cx=0.0f,cy=(float)s->h,ix=r*.43f,iy=cy-r*.43f;int er=G.tool==MODE_ERASE||G.tool==MODE_ERASE_ALL;uint32_t ac=er?mix_color(C_TEXT,C_DANGER,22):C_TEXT;alpha_circle(s,cx,cy,r+us(4),C_BLACK,32);aa_circle(s,cx,cy,r+us(2),C_TEXT,65);aa_circle(s,cx,cy,r,mix_color(C_GLASS2,C_TEXT,7),233);line_icon(s,er?1:0,ix,iy,1.30f,ac);}
static void draw_toast(Surf*s){if(!G.toast)return;const char*msg="";uint32_t c=C_MUTED;switch(G.toast){case 1:msg="Undo";c=C_GREEN;break;case 2:msg="Nothing to undo";break;case 3:msg="Photo imported";c=C_GREEN;break;case 4:msg="Allow photo access";c=C_WARN;break;case 5:msg="No photos available";c=C_DANGER;break;case 6:msg="Stylus action";c=C_VIOLET;break;case 7:msg="Frame exported as PNG";c=C_GREEN;break;case 8:msg="Action failed";c=C_DANGER;break;case 9:msg="Layer locked";c=C_WARN;break;case 10:msg="No empty project slot";c=C_WARN;break;case 11:msg="Project duplicated";c=C_GREEN;break;case 12:msg="Importing photo...";c=C_CYAN;break;case 13:msg="Project cleared";c=C_GREEN;break;default:msg="Done";c=C_GREEN;break;}int sc=ts(2),w=text_w(msg,sc)+us(48),h=us(54),x=(s->w-w)/2,y=margin_ui()+us(82);glass(s,x,y,x+w,y+h,us(18));text(s,x+us(24),y+us(18),msg,sc,c);}
static void draw_frames(Surf*s){if(!G.visFrames)return;for(int i=0;i<G.frameN;i++){FrameObj*f=&G.frames[i];if(!f->active)continue;float x0=f->x*G.scale+G.offX,y0=f->y*G.scale+G.offY,x1=(f->x+f->w)*G.scale+G.offX,y1=(f->y+f->h)*G.scale+G.offY;if(x1<-120||y1<-120||x0>s->w+120||y0>s->h+120)continue;int selected=i==G.selectedFrame;float far=1.0f-clampf((G.scale-.14f)/.36f,0.0f,1.0f);uint32_t c=selected?C_ACCENT:mix_color(C_BORDER,C_ACCENT,(int)(far*24.0f));int aa=selected?224:(int)(64+far*76);aa_hline(s,y0,(int)x0,(int)x1,c,aa);aa_hline(s,y1,(int)x0,(int)x1,c,aa);aa_vline(s,x0,(int)y0,(int)y1,c,aa);aa_vline(s,x1,(int)y0,(int)y1,c,aa);if(G.scale>.08f){char q[48];copy_text_local(q,48,G.frameNames[i][0]?G.frameNames[i]:"Frame");int fs=(G.scale<.24f?ts(2):ts(1)),tw=text_w(q,fs),hh=font_line_h(fs)+us(10);int la=selected?214:(int)(118+far*62);alpha_round_rect(s,(int)x0,(int)y0-hh,(int)x0+tw+us(20),(int)y0,us(8),C_GLASS2,la);text(s,(int)x0+us(10),(int)y0-hh+us(5),q,fs,selected?C_ACCENT:C_TEXT);}}
}
static int GPU_WORLD_INK;
static void draw_canvas_world(Surf*s){draw_frames(s);draw_region_fields(s);draw_images(s);if(!GPU_WORLD_INK)draw_notes(s);draw_shapes(s);if(!GPU_WORLD_INK)draw_strokes(s);draw_measures(s);draw_constellation(s);}
static void draw_ui_overlay(Surf*s){draw_live_stroke_gpu_overlay(s);draw_ocr_highlight(s);draw_lasso(s);draw_selection_overlay(s);draw_live_frame_preview(s);if(android_ui_modal()){draw_toast(s);return;}int inlineEditor=G.editorOpen&&(G.editorMode==2||G.editorMode==5);int modal=G.settingsPanel||G.galleryOpen||G.calibrationOpen||(G.editorOpen&&!inlineEditor)||G.projectsPanel;if(!G.presentationMode){if(!android_ui_available()&&(!NU.ready||!NU.hostShown)){if(!G.zenMode)draw_header(s);else draw_header_identity(s);}/* The rail, ink toggle and colour picker deliberately stay in the native
renderer. On several Android 16 vendor builds, non-touchable PopupWindow child Views are system-tinted/desaturated. These three controls are colour-critical and share the canvas input router, so renderer-owned visuals are both more reliable and lower latency. */if(!G.zenMode&&!modal){if(!android_ui_available())draw_utility_rail(s);else if(!G.railOpen)draw_focus_handle(s);}draw_color_picker(s);draw_ink_toggle(s);}if(G.presentationMode){draw_presentation_overlay(s);draw_toast(s);return;}if(!G.zenMode||modal||inlineEditor){if(!modal){draw_minimap(s);draw_frame_creation_hint(s);if(G.addPanel)draw_add_panel(s);else if(G.framePanel)draw_frame_panel(s);else if(G.layersPanel)draw_layers_panel(s);else if(G.searchPanel)draw_search_panel(s);else if(G.photoMode){if(!android_ui_available())draw_photo_panel(s);}else if(G.tool==MODE_MEASURE){if(!android_ui_available())draw_measure_panel(s);}else if(G.tool==MODE_SHAPE){if(!android_ui_available())draw_shape_panel(s);}else {if(!android_ui_available())draw_pressure_panel(s);}{if(!android_ui_available())draw_context_toolbar(s);}}}else{draw_focus_handle(s);}draw_pen_hold_progress(s);draw_radial_menu(s);draw_settings_panel(s);draw_gallery(s);draw_calibration(s);draw_projects_panel(s);draw_editor(s);draw_hover_preview(s);draw_toast(s);}
#ifdef VAST_GPU
#include "gpu_renderer.inc"
#endif
static void render_software(void){if(!G.window)return;ANativeWindow_Buffer b;if(ANativeWindow_lock(G.window,&b,0)!=0)return;if(b.width<=0||b.height<=0||!b.bits){ANativeWindow_unlockAndPost(G.window);return;}G.screenW=b.width;G.screenH=b.height;if(!G.viewInitialized){G.scale=1.0f;G.offX=(float)b.width*0.18f;G.offY=(float)b.height*0.18f;G.viewInitialized=1;}int mmw=us(320),mmh=us(205),mmp=us(14),mmt=us(40),mcw=mmw-2*mmp,mch=mmh-mmt-mmp;if((G.minimap||G.minimapAnim>.01f)&&(G.minimapDirty||G.minimapCacheW!=mcw||G.minimapCacheH!=mch)&&G.fingerCount==0&&!G.stylusDown)rebuild_minimap_cache(mcw,mch);Surf s={(uint8_t*)b.bits,b.width,b.height,b.stride,b.format};fill(&s,C_BG);draw_grid(&s);draw_canvas_world(&s);draw_ui_overlay(&s);ANativeWindow_unlockAndPost(G.window);}
static int INPUT_BATCH_ACTIVE,INPUT_FRAME_PENDING,INPUT_FRAME_SCHEDULED;
typedef struct AChoreographer AChoreographer;
extern AChoreographer*AChoreographer_getInstance(void);
extern void AChoreographer_postFrameCallback(AChoreographer*,void(*)(long,void*),void*);
static void input_frame(long time,void*data){(void)time;(void)data;INPUT_FRAME_SCHEDULED=0;render();}
static void request_input_frame(void){if(!G.window||INPUT_FRAME_SCHEDULED)return;AChoreographer*c=AChoreographer_getInstance();if(!c){render();return;}INPUT_FRAME_SCHEDULED=1;AChoreographer_postFrameCallback(c,input_frame,0);}
static void render(void){if(INPUT_BATCH_ACTIVE){INPUT_FRAME_PENDING=1;return;}if(!G.window)return;if(!G.stylusDown&&(G.tool==MODE_ERASE||G.tool==MODE_ERASE_ALL))erase_index_prepare();int done=0;double frameStart=perf_us();
#ifdef VAST_GPU
    if(gpu_render_frame())done=1;
#endif
    if(!done){G.gpuActive=0;render_software();}nui_sync();android_ui_sync();double cost=perf_us()-frameStart;IP.frames++;IP.frameUs+=cost;if(cost>IP.maxFrameUs)IP.maxFrameUs=cost;perf_report();}
static void render_event(int64_t now,int force){(void)force;if(now>0)G.lastRenderEventMs=now;render();}

// ---------- UI interaction ----------
enum {
    UI_NONE=0,UI_PEN,UI_HIGHLIGHT,UI_ERASE,UI_ERASE_ALL,UI_MEASURE,UI_IMPORT,UI_PHOTO_MODE,UI_FRAMES,UI_UNDO,UI_REDO,UI_FIT,UI_PRESSURE,UI_MINIMAP,UI_SETTINGS,UI_CLEAR,UI_RAIL_OPEN,
    UI_BRUSH_MINUS,UI_BRUSH_PLUS,UI_COLOR_PICKER,UI_INK_TOGGLE,UI_RECENT_COLOR0,UI_RECENT_COLOR1,UI_RECENT_COLOR2,UI_RECENT_COLOR3,UI_RECENT_COLOR4,UI_RECENT_COLOR5,UI_COLOR_RAIL_OPEN,
    UI_PMIN_MINUS,UI_PMIN_PLUS,UI_PMAX_MINUS,UI_PMAX_PLUS,UI_CURVE_SOFT,UI_CURVE_LINEAR,UI_CURVE_FIRM,UI_SNAP,UI_CLEAR_DIM,UI_DELETE_DIM,UI_CALIBRATE,
    UI_PHOTO_DELETE,UI_PHOTO_DONE,UI_PHOTO_SNAP,UI_FRAME_NEW,UI_FRAME_CLOSE,UI_FRAME_PREV,UI_FRAME_NEXT,UI_FRAME_JUMP,UI_FRAME_DELETE,UI_FRAME_FIT,UI_LAYER_INK,UI_LAYER_MARKER,UI_LAYER_PHOTOS,UI_LAYER_CAD,UI_LAYER_FRAMES,UI_FRAME_ITEM0,UI_FRAME_ITEM1,UI_FRAME_ITEM2,UI_FRAME_ITEM3,UI_FRAME_ITEM4,UI_FRAME_ITEM5,
    UI_SETTINGS_CLOSE,UI_SETTINGS_APPEARANCE,UI_SETTINGS_STYLUS,UI_SETTINGS_DRAWING,UI_SETTINGS_DISPLAY,UI_SETTINGS_HANDWRITING,UI_OCR_TOGGLE,UI_OCR_REBUILD,UI_SCALE_MINUS,UI_SCALE_PLUS,UI_BUTTONS_TOGGLE,
    UI_PRESET0,UI_PRESET1,UI_PRESET2,UI_PRESET3,UI_PRESET4,UI_PRESET_LAST=UI_PRESET0+THEME_PRESET_COUNT-1,
    UI_ROLE0=UI_PRESET_LAST+1,UI_ROLE1,UI_ROLE2,UI_ROLE3,UI_ROLE4,UI_ROLE5,UI_ROLE6,UI_ROLE7,
    UI_PALETTE0,UI_PALETTE1,UI_PALETTE2,UI_PALETTE3,UI_PALETTE4,UI_PALETTE5,UI_PALETTE6,UI_PALETTE7,UI_PALETTE8,UI_PALETTE9,UI_PALETTE10,UI_PALETTE11,
    UI_THEME_RESET,UI_PRESS_MINUS,UI_PRESS_PLUS,UI_HOLD_MINUS,UI_HOLD_PLUS,UI_DOUBLE_MINUS,UI_DOUBLE_PLUS,
    UI_PSMOOTH_MINUS,UI_PSMOOTH_PLUS,UI_SSMOOTH_MINUS,UI_SSMOOTH_PLUS,UI_HILITE_MINUS,UI_HILITE_PLUS,UI_RADIAL_MINUS,UI_RADIAL_PLUS,
    UI_DISP0_MINUS,UI_DISP0_PLUS,UI_DISP1_MINUS,UI_DISP1_PLUS,UI_DISP2_MINUS,UI_DISP2_PLUS,UI_DISP3_MINUS,UI_DISP3_PLUS,UI_DISP4_MINUS,UI_DISP4_PLUS,UI_DISP5_MINUS,UI_DISP5_PLUS,UI_DISP6_MINUS,UI_DISP6_PLUS,UI_DISP7_MINUS,UI_DISP7_PLUS,UI_DISP8_MINUS,UI_DISP8_PLUS,UI_DISP9_MINUS,UI_DISP9_PLUS,
    UI_ADD,UI_SEARCH,UI_LAYERS,UI_PROJECTS,UI_FOCUS,UI_SELECT,UI_RETURN_WRITE,
    UI_ADD_CLOSE,UI_ADD_TEXT,UI_ADD_STICKY,UI_ADD_SHAPE,UI_ADD_PHOTO,UI_ADD_FRAME,UI_ADD_BOOKMARK,
    UI_LAYERS_CLOSE,UI_LAYER_NOTES,UI_LAYER_SHAPES,UI_LOCK_INK,UI_LOCK_MARKER,UI_LOCK_PHOTOS,UI_LOCK_CAD,UI_LOCK_NOTES,UI_LOCK_SHAPES,UI_LOCK_FRAMES,
    UI_SEARCH_CLOSE,UI_SEARCH_EDIT,UI_SEARCH_RESULT0,UI_SEARCH_RESULT1,UI_SEARCH_RESULT2,UI_SEARCH_RESULT3,UI_SEARCH_RESULT4,UI_SEARCH_RESULT5,
    UI_FRAME_RENAME,UI_FRAME_LOCK,UI_FRAME_SIZE_LOCK,UI_FRAME_DUP,UI_FRAME_EXPORT,UI_FRAME_PRESENT,UI_BOOKMARK_ITEM0,UI_BOOKMARK_ITEM1,UI_BOOKMARK_ITEM2,UI_BOOKMARK_ITEM3,UI_BOOKMARK_RENAME0,UI_BOOKMARK_RENAME1,UI_BOOKMARK_RENAME2,UI_BOOKMARK_DELETE0,UI_BOOKMARK_DELETE1,UI_BOOKMARK_DELETE2,
    UI_SELECTION_EDIT,UI_SELECTION_DUP,UI_SELECTION_GROUP,UI_SELECTION_DELETE,UI_SELECTION_MORE,UI_SELECTION_COLOR,UI_SELECTION_LOCK,UI_SELECTION_DESELECT,
    UI_SHAPE_LINE,UI_SHAPE_RECT,UI_SHAPE_ELLIPSE,UI_SHAPE_ARROW,UI_SHAPE_DONE,
    UI_PROJECT_CLOSE,UI_PROJECT_RENAME,UI_PROJECT_DUP,UI_PROJECT_CLEAR,UI_PROJECT0,UI_PROJECT1,UI_PROJECT2,UI_PROJECT3,
    UI_PRESENT_PREV,UI_PRESENT_NEXT,UI_PRESENT_EXIT,
    UI_PICKER_DONE,UI_GALLERY_CLOSE,UI_GALLERY_PREV,UI_GALLERY_NEXT,UI_GALLERY_ITEM0,
    UI_CAL_UNIT=300,UI_CAL_CANCEL,UI_CAL_APPLY,UI_CAL_KEY0,
    UI_EDITOR_CANCEL=400,UI_EDITOR_DONE,UI_EDITOR_SPACE,UI_EDITOR_BACKSPACE,UI_EDITOR_KEY0
};
static int rail_ui(int i){static const int ids[8]={UI_ADD,UI_FRAMES,UI_PHOTO_MODE,UI_MEASURE,UI_SEARCH,UI_LAYERS,UI_MINIMAP,UI_SETTINGS};return (i>=0&&i<8)?ids[i]:UI_NONE;}
static int ui_hit_settings(float x,float y){
    int m=margin_ui(),w=mini(us(1040),G.screenW-2*m),h=mini(us(780),G.screenH-2*m),px=(G.screenW-w)/2,py=(G.screenH-h)/2;
    if(hit_box(x,y,px+w-us(144),py+us(18),px+w-us(24),py+us(62)))return UI_SETTINGS_CLOSE;
    {static const int ids[5]={UI_SETTINGS_APPEARANCE,UI_SETTINGS_STYLUS,UI_SETTINGS_DRAWING,UI_SETTINGS_DISPLAY,UI_SETTINGS_HANDWRITING};int gap=us(8),tw=(w-us(60)-gap*4)/5;for(int i=0;i<5;i++){int tx=px+us(30)+i*(tw+gap);if(hit_box(x,y,tx,py+us(82),tx+tw,py+us(132)))return ids[i];}}
    if(G.settingsTab==0){int yy=py+us(178);if(hit_box(x,y,px+us(218),yy-us(12),px+us(298),yy+us(34)))return UI_SCALE_MINUS;if(hit_box(x,y,px+us(452),yy-us(12),px+us(532),yy+us(34)))return UI_SCALE_PLUS;yy+=us(72);for(int i=0;i<THEME_PRESET_COUNT;i++){int col=i%8,row=i/8,bx=px+us(34+col*118),by=yy+us(30+row*44);if(hit_box(x,y,bx,by,bx+us(110),by+us(38)))return UI_PRESET0+i;}yy+=us(165);for(int i=0;i<8;i++){int col=i%4,row=i/4,bx=px+us(34+col*235),by=yy+us(28+row*48);if(hit_box(x,y,bx,by,bx+us(215),by+us(40)))return UI_ROLE0+i;}yy+=us(124);for(int i=0;i<12;i++){int col=i%6,row=i/6,cx=px+us(120+col*135),cy=yy+us(36+row*58);if(sq(x-cx)+sq(y-cy)<=sq((float)us(31)))return UI_PALETTE0+i;}yy+=us(140);if(hit_box(x,y,px+w-us(220),yy-us(12),px+w-us(34),yy+us(36)))return UI_THEME_RESET;
    }else if(G.settingsTab==1){int yy=py+us(180);if(hit_box(x,y,px+us(286),yy-us(14),px+us(510),yy+us(36)))return UI_BUTTONS_TOGGLE;yy+=us(82);if(G.buttonBindingsEnabled){for(int r=0;r<3;r++){if(hit_box(x,y,px+us(210),yy-us(4),px+us(270),yy+us(46)))return r==0?UI_PRESS_MINUS:(r==1?UI_HOLD_MINUS:UI_DOUBLE_MINUS);if(hit_box(x,y,px+us(722),yy-us(4),px+us(782),yy+us(46)))return r==0?UI_PRESS_PLUS:(r==1?UI_HOLD_PLUS:UI_DOUBLE_PLUS);yy+=us(72);}}
    }else if(G.settingsTab==2){int yy=py+us(184);for(int r=0;r<4;r++){if(hit_box(x,y,px+us(620),yy-us(4),px+us(680),yy+us(46)))return r==0?UI_PSMOOTH_MINUS:(r==1?UI_SSMOOTH_MINUS:(r==2?UI_HILITE_MINUS:UI_RADIAL_MINUS));if(hit_box(x,y,px+us(824),yy-us(4),px+us(884),yy+us(46)))return r==0?UI_PSMOOTH_PLUS:(r==1?UI_SSMOOTH_PLUS:(r==2?UI_HILITE_PLUS:UI_RADIAL_PLUS));yy+=us(88);}}
    else if(G.settingsTab==3){int yy=py+us(184);for(int r=0;r<3;r++){if(hit_box(x,y,px+us(560),yy-us(4),px+us(620),yy+us(46)))return UI_DISP0_MINUS+r*2;if(hit_box(x,y,px+us(832),yy-us(4),px+us(892),yy+us(46)))return UI_DISP0_PLUS+r*2;yy+=us(76);}}
    else{int yy=py+us(184);if(hit_box(x,y,px+w-us(286),yy-us(10),px+w-us(34),yy+us(44)))return UI_OCR_TOGGLE;yy+=us(94+84+86);if(hit_box(x,y,px+us(34),yy-us(12),px+us(376),yy+us(46)))return UI_OCR_REBUILD;}
    return UI_NONE;
}
static int ui_hit_gallery(float x,float y){
    int m=margin_ui(),px=m,py=m,w=G.screenW-2*m,h=G.screenH-2*m;
    if(hit_box(x,y,px+w-us(144),py+us(18),px+w-us(24),py+us(62)))return UI_GALLERY_CLOSE;
    if(hit_box(x,y,px+us(30),py+h-us(64),px+us(190),py+h-us(18)))return UI_GALLERY_PREV;
    if(hit_box(x,y,px+w-us(190),py+h-us(64),px+w-us(30),py+h-us(18)))return UI_GALLERY_NEXT;
    int top=py+us(92),bottom=py+h-us(86),gw=w-us(60),gh=bottom-top,cellW=gw/4,cellH=gh/3;
    if(y>=top&&y<bottom&&x>=px+us(30)&&x<px+us(30)+gw){int col=(int)(x-(px+us(30)))/cellW,row=(int)(y-top)/cellH,idx=row*4+col;if(idx>=0&&idx<G.thumbN)return UI_GALLERY_ITEM0+idx;}
    return UI_NONE;
}
static int ui_hit_calibration(float x,float y){int w=us(520),h=us(650),px=(G.screenW-w)/2,py=(G.screenH-h)/2;if(hit_box(x,y,px+us(28),py+us(194),px+w-us(28),py+us(242)))return UI_CAL_UNIT;for(int i=0;i<12;i++){int col=i%3,row=i/3,bx=px+us(28)+col*us(154),by=py+us(266)+row*us(72);if(hit_box(x,y,bx,by,bx+us(138),by+us(58)))return UI_CAL_KEY0+i;}if(hit_box(x,y,px+us(28),py+h-us(76),px+us(226),py+h-us(24)))return UI_CAL_CANCEL;if(hit_box(x,y,px+us(242),py+h-us(76),px+w-us(28),py+h-us(24)))return UI_CAL_APPLY;return UI_NONE;}
static int ui_hit_header(float x,float y){int m=margin_ui(),h=us(44),gap=us(8),yy=m,xx=G.screenW-m-us(612);if(hit_box(x,y,xx,yy,xx+us(112),yy+h))return UI_SEARCH;xx+=us(112)+gap;if(hit_box(x,y,xx,yy,xx+us(80),yy+h))return UI_UNDO;xx+=us(80)+gap;if(hit_box(x,y,xx,yy,xx+us(80),yy+h))return UI_REDO;xx+=us(80)+gap;if(hit_box(x,y,xx,yy,xx+us(126),yy+h))return UI_PROJECTS;xx+=us(126)+gap;if(hit_box(x,y,xx,yy,xx+us(92),yy+h))return UI_FOCUS;int cy=m+us(80),cx=m+us(46),step=us(34),hit=us(17);for(int i=0;i<6&&i<G.recentColorN;i++)if(hit_box(x,y,cx+i*step-hit,cy-hit,cx+i*step+hit,cy+hit))return UI_RECENT_COLOR0+i;return UI_NONE;}
static int ui_hit_frames(float x,float y){int m=margin_ui(),w=us(560),px=G.screenW-m-w,py=m+us(74),h=mini(us(820),G.screenH-py-m);
    if(G.editorOpen&&(G.editorMode==2||G.editorMode==5)){if(hit_box(x,y,px+us(24),py+us(204),px+us(248),py+us(256)))return UI_EDITOR_CANCEL;if(hit_box(x,y,px+us(262),py+us(204),px+w-us(24),py+us(256)))return UI_EDITOR_DONE;return UI_NONE;}
    if(hit_box(x,y,px+us(24),py+us(88),px+us(248),py+us(138)))return UI_FRAME_NEW;if(hit_box(x,y,px+us(262),py+us(88),px+w-us(24),py+us(138)))return UI_FRAME_CLOSE;if(hit_box(x,y,px+w-us(112),py+us(152),px+w-us(68),py+us(190)))return UI_FRAME_PREV;if(hit_box(x,y,px+w-us(62),py+us(152),px+w-us(18),py+us(190)))return UI_FRAME_NEXT;int first=G.framePage*5;for(int i=0;i<mini(G.frameN-first,5);i++){int yy=py+us(202)+i*us(54);if(hit_box(x,y,px+us(24),yy,px+w-us(24),yy+us(46)))return UI_FRAME_ITEM0+i;}int ay=py+us(480);if(G.selectedFrame>=0&&G.selectedFrame<G.frameN){if(hit_box(x,y,px+us(24),ay,px+us(142),ay+us(44)))return UI_FRAME_JUMP;if(hit_box(x,y,px+us(150),ay,px+us(268),ay+us(44)))return UI_FRAME_RENAME;if(hit_box(x,y,px+us(276),ay,px+us(394),ay+us(44)))return UI_FRAME_SIZE_LOCK;if(hit_box(x,y,px+us(402),ay,px+w-us(24),ay+us(44)))return UI_FRAME_FIT;ay+=us(52);int bw=(w-us(48))/5;for(int i=0;i<5;i++){int bx=px+us(24)+i*bw;if(hit_box(x,y,bx,ay,bx+bw-us(6),ay+us(44)))return i==0?UI_FRAME_LOCK:(i==1?UI_FRAME_DUP:(i==2?UI_FRAME_EXPORT:(i==3?UI_FRAME_PRESENT:UI_FRAME_DELETE)));}}int shown=mini(G.bookmarkN,3);for(int i=0;i<shown;i++){int yy=py+us(628)+i*us(48);if(hit_box(x,y,px+us(24),yy,px+w-us(204),yy+us(40)))return UI_BOOKMARK_ITEM0+i;if(hit_box(x,y,px+w-us(194),yy,px+w-us(108),yy+us(40)))return UI_BOOKMARK_RENAME0+i;if(hit_box(x,y,px+w-us(100),yy,px+w-us(24),yy+us(40)))return UI_BOOKMARK_DELETE0+i;}return UI_NONE;}
static int ui_hit_add(float x,float y){int m=margin_ui(),w=us(430),h=us(430),px=m+us(150),py=(G.screenH-h)/2;if(hit_box(x,y,px+w-us(126),py+us(18),px+w-us(24),py+us(62)))return UI_ADD_CLOSE;int ids[6]={UI_ADD_TEXT,UI_ADD_STICKY,UI_ADD_SHAPE,UI_ADD_PHOTO,UI_ADD_FRAME,UI_ADD_BOOKMARK};for(int i=0;i<6;i++){int col=i%2,row=i/2,bx=px+us(24)+col*us(194),by=py+us(92)+row*us(92);if(hit_box(x,y,bx,by,bx+us(178),by+us(68)))return ids[i];}return UI_NONE;}
static int ui_hit_layers(float x,float y){int m=margin_ui(),w=us(480),h=us(650),px=G.screenW-m-w,py=m+us(82);if(hit_box(x,y,px+w-us(126),py+us(18),px+w-us(24),py+us(62)))return UI_LAYERS_CLOSE;int vids[7]={UI_LAYER_INK,UI_LAYER_MARKER,UI_LAYER_PHOTOS,UI_LAYER_CAD,UI_LAYER_NOTES,UI_LAYER_SHAPES,UI_LAYER_FRAMES};int lids[7]={UI_LOCK_INK,UI_LOCK_MARKER,UI_LOCK_PHOTOS,UI_LOCK_CAD,UI_LOCK_NOTES,UI_LOCK_SHAPES,UI_LOCK_FRAMES};for(int i=0;i<7;i++){int yy=py+us(94)+i*us(70);if(hit_box(x,y,px+us(24),yy,px+us(300),yy+us(52)))return vids[i];if(hit_box(x,y,px+us(316),yy,px+w-us(24),yy+us(52)))return lids[i];}return UI_NONE;}
static int ui_hit_search(float x,float y){int m=margin_ui(),w=us(600),px=G.screenW-m-w,py=m+us(82);if(hit_box(x,y,px+w-us(126),py+us(18),px+w-us(24),py+us(62)))return UI_SEARCH_CLOSE;if(hit_box(x,y,px+us(24),py+us(90),px+w-us(24),py+us(146)))return UI_SEARCH_EDIT;for(int i=0;i<G.searchVisibleN;i++)if(hit_box(x,y,px+us(24),py+us(210)+i*us(62),px+w-us(24),py+us(260)+i*us(62)))return UI_SEARCH_RESULT0+i;return UI_NONE;}
static int ui_hit_projects(float x,float y){int m=margin_ui(),w=us(720),h=us(540),px=(G.screenW-w)/2,py=(G.screenH-h)/2;if(hit_box(x,y,px+w-us(132),py+us(20),px+w-us(24),py+us(64)))return UI_PROJECT_CLOSE;for(int i=0;i<4;i++){int col=i%2,row=i/2,bx=px+us(28)+col*us(330),by=py+us(116)+row*us(132);if(hit_box(x,y,bx,by,bx+us(304),by+us(102)))return UI_PROJECT0+i;}if(hit_box(x,y,px+us(28),py+h-us(94),px+us(220),py+h-us(38)))return UI_PROJECT_RENAME;if(hit_box(x,y,px+us(234),py+h-us(94),px+us(438),py+h-us(38)))return UI_PROJECT_DUP;if(hit_box(x,y,px+us(452),py+h-us(94),px+w-us(28),py+h-us(38)))return UI_PROJECT_CLEAR;return UI_NONE;}
static int ui_hit_editor(float x,float y){int w=mini(us(780),G.screenW-2*margin_ui()),h=G.editorMode==1?us(320):us(180),px=(G.screenW-w)/2,py=margin_ui()+us(66);if(hit_box(x,y,px+us(24),py+h-us(54),px+us(176),py+h-us(14)))return UI_EDITOR_CANCEL;if(hit_box(x,y,px+w-us(176),py+h-us(54),px+w-us(24),py+h-us(14)))return UI_EDITOR_DONE;return UI_NONE;}
static int ui_hit_shape(float x,float y){int m=margin_ui(),w=us(500),px=G.screenW-m-w,py=m+us(82);int ids[4]={UI_SHAPE_LINE,UI_SHAPE_RECT,UI_SHAPE_ELLIPSE,UI_SHAPE_ARROW};for(int i=0;i<4;i++)if(hit_box(x,y,px+us(20)+i*us(112),py+us(56),px+us(120)+i*us(112),py+us(104)))return ids[i];if(hit_box(x,y,px+w-us(100),py+us(112),px+w-us(20),py+us(142)))return UI_SHAPE_DONE;return UI_NONE;}
static int ui_hit_context(float x,float y){if(G.selectedRefN<=0)return UI_NONE;int px,py,w,h;if(!context_toolbar_geometry(&px,&py,&w,&h))return UI_NONE;
    if(G.selectionMoreOpen&&G.selectedRefN==1){int mw=us(230),row=us(46),rows=1+(selection_single_colorable()?1:0)+(selection_single_lockable()?1:0),mx=px+us(252),my=py+h+us(8);if(my+rows*row>G.screenH-margin_ui())my=py-us(8)-rows*row;int yy=my;if(selection_single_colorable()){if(hit_box(x,y,mx,yy,mx+mw,yy+row))return UI_SELECTION_COLOR;yy+=row;}if(selection_single_lockable()){if(hit_box(x,y,mx,yy,mx+mw,yy+row))return UI_SELECTION_LOCK;yy+=row;}if(hit_box(x,y,mx,yy,mx+mw,yy+row))return UI_SELECTION_DESELECT;}
    if(hit_box(x,y,px+us(10),py+us(8),px+us(112),py+h-us(8)))return UI_SELECTION_EDIT;if(hit_box(x,y,px+us(120),py+us(8),px+us(244),py+h-us(8)))return UI_SELECTION_DUP;if(hit_box(x,y,px+us(252),py+us(8),px+us(366),py+h-us(8)))return G.selectedRefN==1?UI_SELECTION_MORE:UI_SELECTION_GROUP;if(hit_box(x,y,px+us(374),py+us(8),px+w-us(10),py+h-us(8)))return UI_SELECTION_DELETE;return UI_NONE;}
static int ui_hit_presentation(float x,float y){int m=margin_ui();if(hit_box(x,y,m,G.screenH-m-us(56),m+us(150),G.screenH-m))return UI_PRESENT_PREV;if(hit_box(x,y,G.screenW-m-us(150),G.screenH-m-us(56),G.screenW-m,G.screenH-m))return UI_PRESENT_NEXT;if(hit_box(x,y,G.screenW-m-us(110),m,G.screenW-m,m+us(44)))return UI_PRESENT_EXIT;return UI_NONE;}

static int ui_hit(float x,float y){
    if(android_ui_modal())return UI_NONE;
    if(G.presentationMode)return ui_hit_presentation(x,y);
    if(G.editorOpen&&(G.editorMode==2||G.editorMode==5))return ui_hit_frames(x,y);
    if(G.editorOpen)return ui_hit_editor(x,y);
    if(G.projectsPanel)return ui_hit_projects(x,y);
    if(G.calibrationOpen)return ui_hit_calibration(x,y);
    if(G.galleryOpen)return ui_hit_gallery(x,y);
    if(G.settingsPanel)return ui_hit_settings(x,y);
    {float r=(float)us(96),dy=G.screenH-y;if(x>=0&&y<=G.screenH&&sq(x)+sq(dy)<=r*r)return UI_INK_TOGGLE;}
    if(android_ui_available()){float r=us(96),dx=G.screenW-x,dy=G.screenH-y;if(dx>=0&&dy>=0&&dx*dx+dy*dy<=r*r)return G.colorPickerOpen?UI_PICKER_DONE:UI_COLOR_PICKER;}
    if(!android_ui_available()&&G.colorRailAnim>.08f){int px,py,pw,ph,sx,sy,sw,sh,hx,hy,hw,hh;picker_geometry(&px,&py,&pw,&ph,&sx,&sy,&sw,&sh,&hx,&hy,&hw,&hh);if(hit_box(x,y,px+pw-us(100),py+us(16),px+pw-us(18),py+us(56)))return UI_PICKER_DONE;}
    if(G.zenMode){float cy=G.screenH*.5f;if(hit_box(x,y,0,(int)cy-us(64),us(54),(int)cy+us(64)))return UI_FOCUS;return UI_NONE;}
    int hh=android_ui_available()?UI_NONE:ui_hit_header(x,y);if(hh)return hh;
    if(G.addPanel)return ui_hit_add(x,y);if(G.framePanel)return ui_hit_frames(x,y);if(G.layersPanel)return ui_hit_layers(x,y);if(G.searchPanel)return ui_hit_search(x,y);
    int ch=android_ui_available()?UI_NONE:ui_hit_context(x,y);if(ch)return ch;
    if(!G.railOpen){float cy=G.screenH*.5f,rr=(float)us(54);if(x>=0&&x<=us(58)&&sq(x)+sq(y-cy)<=rr*rr)return UI_RAIL_OPEN;}else if(!android_ui_available()){int rx,ry,rw,rh,rg;rail_geom(&rx,&ry,&rw,&rh,&rg);for(int i=0;i<8;i++)if(hit_box(x,y,rx,ry+i*(rh+rg),rx+rw,ry+i*(rh+rg)+rh))return rail_ui(i);}
    int m=margin_ui();
    if(android_ui_available())return UI_NONE;
    if(G.photoMode){int pw=us(390),px=G.screenW-m-pw,py=m+us(82);if(G.selectedImage>=0&&G.selectedImage<G.imageN&&G.images[G.selectedImage].active){if(hit_box(x,y,px+pw-us(170),py+us(132),px+pw-us(24),py+us(182)))return UI_PHOTO_SNAP;if(hit_box(x,y,px+us(24),py+us(246),px+us(176),py+us(298)))return UI_PHOTO_DELETE;if(hit_box(x,y,px+us(190),py+us(246),px+pw-us(24),py+us(298)))return UI_PHOTO_DONE;}else if(hit_box(x,y,px+us(24),py+us(240),px+pw-us(24),py+us(292)))return UI_PHOTO_DONE;return UI_NONE;}
    if(G.tool==MODE_SHAPE)return ui_hit_shape(x,y);
    int pw=G.tool==MODE_MEASURE?us(470):us(430),px=G.screenW-m-pw,py=m+us(82);
    if(G.tool==MODE_MEASURE){if(hit_box(x,y,px+us(24),py+us(86),px+us(160),py+us(136)))return UI_SNAP;if(hit_box(x,y,px+us(172),py+us(86),px+us(306),py+us(136)))return UI_CALIBRATE;if(hit_box(x,y,px+us(318),py+us(86),px+pw-us(24),py+us(136)))return UI_CLEAR_DIM;if(G.selectedMeasure>=0&&hit_box(x,y,px+pw-us(158),py+us(152),px+pw-us(24),py+us(198)))return UI_DELETE_DIM;}
    else if(G.pressurePanel){int cyy=py+us(252);if(hit_box(x,y,px+us(104),cyy-us(12),px+us(190),cyy+us(34)))return UI_CURVE_SOFT;if(hit_box(x,y,px+us(198),cyy-us(12),px+us(292),cyy+us(34)))return UI_CURVE_LINEAR;if(hit_box(x,y,px+us(300),cyy-us(12),px+pw-us(24),cyy+us(34)))return UI_CURVE_FIRM;cyy+=us(70);if(hit_box(x,y,px+us(274),cyy+us(8),px+us(322),cyy+us(48)))return UI_PMIN_MINUS;if(hit_box(x,y,px+us(334),cyy+us(8),px+us(382),cyy+us(48)))return UI_PMIN_PLUS;cyy+=us(76);if(hit_box(x,y,px+us(274),cyy+us(8),px+us(322),cyy+us(48)))return UI_PMAX_MINUS;if(hit_box(x,y,px+us(334),cyy+us(8),px+us(382),cyy+us(48)))return UI_PMAX_PLUS;}
    return UI_NONE;
}

static void arm_rail_timer(void);
static void cancel_rail_timer(void);
static void start_anim_timer(void);

static int64_t ANIM_LAST_MS;
static float ANIM_STEP_SECONDS=.008f;
static void cancel_anim_timer(void){G.uiAnimating=0;ANIM_LAST_MS=0;if(G.animTimerFd<0)return;itimerspec_t it;memset(&it,0,sizeof(it));timerfd_settime(G.animTimerFd,0,&it,0);}
static void arm_anim_timer_ms(int ms,int animating){
    if(!ANIM_LAST_MS)ANIM_LAST_MS=monotonic_ms();G.uiAnimating=animating;if(G.animTimerFd<0)return;itimerspec_t it;memset(&it,0,sizeof(it));
    it.it_value.tv_sec=it.it_interval.tv_sec=ms/1000;it.it_value.tv_nsec=it.it_interval.tv_nsec=(long)(ms%1000)*1000000L;
    timerfd_settime(G.animTimerFd,0,&it,0);
}
static void start_anim_timer(void){arm_anim_timer_ms(8,1);}
static void start_editor_poll_timer(void){/* Compatibility call sites: TextWatcher replaces polling. */}
static const float RADIAL_PROGRESS_DELAY=.32f;
static float radial_progress_value(int64_t now){float elapsedSec=(float)(now-G.penHoldStartMs)/1000.0f;return clampf((elapsedSec-RADIAL_PROGRESS_DELAY)/clampf(G.radialHoldSec,.35f,3),0,1);}
static void fling_cancel(void){
    if(G.flingActive)G.workspaceSavePending=1;
    G.flingActive=0;G.flingVX=G.flingVY=0;G.flingSampleN=0;
}
static void fling_track_reset(int64_t now){
    G.flingSampleN=1;G.flingTrackX=G.flingTrackY=0;
    G.flingSampleX[0]=G.flingSampleY[0]=0;G.flingSampleMs[0]=now;
}
static void fling_track_add(float dx,float dy,int64_t now){
    G.flingTrackX+=dx;G.flingTrackY+=dy;
    if(G.flingSampleN>0&&now<=G.flingSampleMs[G.flingSampleN-1]){int i=G.flingSampleN-1;G.flingSampleX[i]=G.flingTrackX;G.flingSampleY[i]=G.flingTrackY;return;}
    if(G.flingSampleN>=12){for(int i=1;i<12;i++){G.flingSampleX[i-1]=G.flingSampleX[i];G.flingSampleY[i-1]=G.flingSampleY[i];G.flingSampleMs[i-1]=G.flingSampleMs[i];}G.flingSampleN=11;}
    int i=G.flingSampleN++;G.flingSampleX[i]=G.flingTrackX;G.flingSampleY[i]=G.flingTrackY;G.flingSampleMs[i]=now;
}
static void fling_release(int64_t now){
    if(G.flingSampleN<2)return;int newest=G.flingSampleN-1;if(now-G.flingSampleMs[newest]>90){G.flingSampleN=0;return;}int oldest=newest;
    while(oldest>0&&G.flingSampleMs[newest]-G.flingSampleMs[oldest-1]<=110)oldest--;
    float sw=0,st=0,sx=0,sy=0;for(int i=oldest;i<=newest;i++){float age=(float)(G.flingSampleMs[newest]-G.flingSampleMs[i])/110.0f,w=1.0f+2.0f*(1.0f-clampf(age,0,1)),t=(float)(G.flingSampleMs[i]-G.flingSampleMs[newest])*.001f;sw+=w;st+=w*t;sx+=w*G.flingSampleX[i];sy+=w*G.flingSampleY[i];}
    float mt=st/fmax2(sw,.001f),mx=sx/fmax2(sw,.001f),my=sy/fmax2(sw,.001f),den=0,numx=0,numy=0;for(int i=oldest;i<=newest;i++){float age=(float)(G.flingSampleMs[newest]-G.flingSampleMs[i])/110.0f,w=1.0f+2.0f*(1.0f-clampf(age,0,1)),t=(float)(G.flingSampleMs[i]-G.flingSampleMs[newest])*.001f,dt=t-mt;den+=w*dt*dt;numx+=w*dt*(G.flingSampleX[i]-mx);numy+=w*dt*(G.flingSampleY[i]-my);}
    if(den<.00002f){G.flingSampleN=0;return;}float vx=numx/den,vy=numy/den;float releaseFade=clampf(1.0f-(float)(now-G.flingSampleMs[newest])/120.0f,0,1);vx*=releaseFade;vy*=releaseFade;
    float speed=sqrtf(vx*vx+vy*vy);const float minSpeed=180.0f,maxSpeed=3200.0f;
    G.flingSampleN=0;if(speed<minSpeed)return;
    if(speed>maxSpeed){float s=maxSpeed/speed;vx*=s;vy*=s;}
    G.flingVX=vx;G.flingVY=vy;G.flingActive=1;G.interactionMs=now;ANIM_LAST_MS=now;ocr_set_interaction(1);start_anim_timer();
}
static int fling_advance(float dt){
    if(!G.flingActive)return 0;
    dt=clampf(dt,0.0f,.05f);float vx=G.flingVX,vy=G.flingVY,speed=sqrtf(vx*vx+vy*vy);const float decel=6400.0f;
    if(speed<=0.01f){G.flingActive=0;G.flingVX=G.flingVY=0;G.workspaceSavePending=1;ocr_set_interaction(0);return 0;}
    float next=fmax2(0.0f,speed-decel*dt),distance=next<=0.0f?speed*speed/(2.0f*decel):(speed+next)*.5f*dt,ux=vx/speed,uy=vy/speed;
    G.offX+=ux*distance;G.offY+=uy*distance;G.flingVX=ux*next;G.flingVY=uy*next;G.interactionMs=monotonic_ms();minimap_camera_changed();
    if(next<=0.01f){G.flingActive=0;G.flingVX=G.flingVY=0;G.workspaceSavePending=1;ocr_set_interaction(0);}
    return 1;
}
static int spring_step(float*x,float*v,float target,float stiffness,float damping){
    /* Retarget immediately from current position; elapsed-time exponential
       settling has no overshoot and is identical at 60/90/120 Hz. */
    (void)stiffness;(void)damping;float before=*x;
    *x+=(target-*x)*(1.0f-expf(-ANIM_STEP_SECONDS*(target>before?30.0f:40.0f)));*v=0;
    if(fabsf(target-*x)<.002f){*x=target;return 1;}return 0;
}
static int anim_timer_cb(int fd,int events,void*data){
    (void)events;(void)data;uint64_t ticks=0;read(fd,&ticks,sizeof(ticks));int64_t now=monotonic_ms();
    float elapsed=ANIM_LAST_MS?clampf((float)(now-ANIM_LAST_MS)/1000,.001f,.256f):.008f;ANIM_LAST_MS=now;
    int n=maxi(1,mini(32,(int)(elapsed/.008f)+1));ANIM_STEP_SECONDS=elapsed/(float)n;
    int settled=1,textChanged=0,uiChanged=0,cameraChanged=0;
    if(G.photoImportActive){photo_import_step();uiChanged=1;if(G.photoImportActive)settled=0;}
    float oldRail=G.railAnim,oldRadial=G.radialAnim,oldMap=G.minimapAnim,oldColor=G.colorRailAnim,oldHold=G.penHoldVisual,oldHighlight=G.ocrHighlightAlpha;
    /* TextWatcher delivers edits without idle polling. */
    if(G.toast){int64_t now=monotonic_ms();if(G.toastMs>0&&now-G.toastMs>=1300){G.toast=0;uiChanged=1;}else settled=0;}
    float mi=clampf(G.motionIntensity,0,1.5f),rs,rd,ds,dd,ms,md;
    if(G.motionStyle==0){rs=248+44*mi;rd=27.5f-2.5f*mi;ds=280+52*mi;dd=25.5f-2.6f*mi;ms=228+40*mi;md=27.5f-2.5f*mi;}
    else if(G.motionStyle==1){rs=310+76*mi;rd=20.0f-2.4f*mi;ds=372+92*mi;dd=19.2f-2.2f*mi;ms=270+65*mi;md=20.2f-2.2f*mi;}
    else{rs=356+126*mi;rd=15.8f-2.2f*mi;ds=442+156*mi;dd=14.8f-2.0f*mi;ms=308+92*mi;md=15.8f-2.0f*mi;}
    for(int k=0;k<n;k++){
        if(fling_advance(ANIM_STEP_SECONDS)){cameraChanged=1;settled=0;}
        int r=spring_step(&G.railAnim,&G.railVel,G.railOpen?1.0f:0.0f,rs,rd),d=spring_step(&G.radialAnim,&G.radialVel,G.radialOpen?1.0f:0.0f,ds,dd),m=spring_step(&G.minimapAnim,&G.minimapVel,G.minimap?1.0f:0.0f,ms,md),cr=G.colorRailDragging?0:spring_step(&G.colorRailAnim,&G.colorRailVel,G.colorPickerOpen?1.0f:0.0f,rs,rd);
        if(!(r&&d&&m&&cr))settled=0;
        if(G.penHoldArmed&&G.touchHoldMode==1&&G.fingerCount==1&&!G.penHoldMoved&&!G.radialOpen){G.penHoldVisual=radial_progress_value(now);settled=0;}
        else if(G.penHoldVisual>0.0f&&!G.radialOpen){G.penHoldVisual*=expf(-40.0f*ANIM_STEP_SECONDS);if(G.penHoldVisual<0.01f)G.penHoldVisual=0.0f;else settled=0;}
    }
    if(android_ui_available()){G.railAnim=G.railOpen?1.0f:0.0f;G.railVel=0;G.colorRailAnim=G.colorPickerOpen?1.0f:0.0f;G.colorRailVel=0;}
    int idle=!G.stylusDown&&G.fingerCount==0&&!G.flingActive&&now-G.interactionMs>=400;
    int scanning=G.ocrInitialPending;if(idle&&!G.photoImportActive){if(G.workspaceSavePending&&save_workspace())G.workspaceSavePending=0;if(G.imageSavePending){if(save_images())G.imageSavePending=0;else set_toast(8,now);}perf_report();perf_dump();scanning=ocr_initial_scan_step();ocr_pending_flush();if(G.inkSavePending){if(save_canvas()){G.inkSavePending=0;ocr_checkpoint();}else set_toast(8,now);}}
    if(G.ocrHighlightAlpha>0.0f){G.ocrHighlightAlpha-=elapsed/1.15f;if(G.ocrHighlightAlpha<0.0f)G.ocrHighlightAlpha=0.0f;else settled=0;}
    int spatialChanged=oldRadial!=G.radialAnim||oldMap!=G.minimapAnim||oldColor!=G.colorRailAnim||oldHold!=G.penHoldVisual||oldHighlight!=G.ocrHighlightAlpha||(!android_ui_available()&&oldRail!=G.railAnim);
    G.uiAnimating=spatialChanged;
    int gpuPending=0;
#ifdef VAST_GPU
    gpuPending=scene_images_pending();if(gpuPending)settled=0;
#endif
    if(cameraChanged||spatialChanged||textChanged||uiChanged||gpuPending)render();
#ifdef VAST_GPU
    if(scene_images_pending())settled=0;
#endif
    if(settled){if(scanning||G.photoImportActive||G.imageSavePending||G.inkSavePending||G.workspaceSavePending||G.ocrPending||PERF_DIRTY)arm_anim_timer_ms(64,0);else cancel_anim_timer();}
    return 1;
}
static void cancel_button_timer(void);
static int action_cycle(int a,int dir){a+=dir;if(a<0)a=BA_COUNT-1;if(a>=BA_COUNT)a=0;return a;}
static void reset_view(void){G.scale=1.0f;if(G.screenW>0&&G.screenH>0){G.offX=G.screenW*0.18f;G.offY=G.screenH*0.18f;}}
static int write_checkpoint_away(void){
    if(!G.lastWriteValid||G.screenW<=0||G.screenH<=0)return 0;
    float cx=(G.screenW*.5f-G.offX)/fmax2(G.scale,.0001f),cy=(G.screenH*.5f-G.offY)/fmax2(G.scale,.0001f);
    float screenDistance=sqrtf(sq(cx-G.lastWriteX)+sq(cy-G.lastWriteY))*G.scale;
    float ratio=G.scale/fmax2(G.lastWriteScale,.0001f);if(ratio<1.0f)ratio=1.0f/ratio;
    return screenDistance>fmin2(G.screenW,G.screenH)*.28f||ratio>1.35f;
}
static void return_to_write_checkpoint(void){
    if(!G.lastWriteValid)return;fling_cancel();G.scale=clampf(G.lastWriteScale,.05f,20.0f);G.offX=G.screenW*.5f-G.lastWriteX*G.scale;G.offY=G.screenH*.5f-G.lastWriteY*G.scale;G.workspaceSavePending=1;minimap_camera_changed();start_anim_timer();
}
static void reveal_canvas_chrome(void){G.zenMode=0;G.railOpen=0;cancel_rail_timer();start_anim_timer();}
static float pen_hold_buffer_px(void){float inv=1.0f/sqrtf(fmax2(G.scale,0.22f));return clampf(8.0f*inv,8.0f,20.0f);}
static void delete_selected_image(void){G.imageContentRevision++;int id=G.selectedImage;if(id<0||id>=G.imageN||G.lockPhotos)return;if(G.images[id].px)free(G.images[id].px);for(int i=id+1;i<G.imageN;i++)G.images[i-1]=G.images[i];G.imageN--;if(G.imageN>=0)memset(&G.images[G.imageN],0,sizeof(ImageObj));G.selectedImage=-1;G.imagesDirty=G.imagePixelsDirty=1;G.minimapDirty=1;}
static int cycle_range(int v,int dir,int lo,int hi){v+=dir;if(v<lo)v=hi;if(v>hi)v=lo;return v;}
static void display_adjust(int row,int dir){switch(row){case 0:G.gridStyle=cycle_range(G.gridStyle,dir,0,3);break;case 1:G.farZoomMode=cycle_range(G.farZoomMode,dir,0,2);break;case 2:G.performanceMode=cycle_range(G.performanceMode,dir,0,2);break;default:break;}G.metaDirty=1;G.minimapDirty=1;}
static void handle_ui(int id,int64_t ms){
    if(G.photoImportActive)return;
    if(id!=UI_CLEAR)G.clearArmed=0;
    if(id>=UI_PRESET0&&id<=UI_PRESET_LAST){theme_preset(id-UI_PRESET0);G.metaDirty=1;save_meta();render();return;}
    switch(id){
        case UI_RAIL_OPEN:G.railOpen=1;G.zenMode=0;start_anim_timer();arm_rail_timer();break;
        case UI_ADD:close_workspace_panels();G.addPanel=1;G.photoMode=0;G.pressurePanel=0;break;
        case UI_SEARCH:close_workspace_panels();G.searchPanel=1;G.photoMode=0;break;
        case UI_LAYERS:close_workspace_panels();G.layersPanel=1;G.photoMode=0;break;
        case UI_PROJECTS:close_workspace_panels();G.projectsPanel=1;break;
        case UI_FOCUS:G.zenMode=!G.zenMode;if(!G.zenMode){G.railOpen=1;start_anim_timer();arm_rail_timer();}break;
        case UI_SELECT:G.tool=MODE_SELECT;G.photoMode=0;G.pressurePanel=0;selection_clear();break;
        case UI_RETURN_WRITE:return_to_write_checkpoint();break;
        case UI_ADD_CLOSE:G.addPanel=0;break;
        case UI_ADD_TEXT:add_note_at_center(NOTE_TEXT);break;
        case UI_ADD_STICKY:add_note_at_center(NOTE_STICKY);break;
        case UI_ADD_SHAPE:shape_mode_start();break;
        case UI_ADD_PHOTO:G.addPanel=0;open_photo_gallery();break;
        case UI_ADD_FRAME:G.addPanel=0;G.frameCreating=1;G.frameDragging=0;G.selectedFrame=-1;break;
        case UI_ADD_BOOKMARK:add_bookmark_here();break;
        case UI_LAYERS_CLOSE:G.layersPanel=0;break;
        case UI_LAYER_NOTES:G.visNotes=!G.visNotes;selection_clear();frame_changed();break;
        case UI_LAYER_SHAPES:G.visShapes=!G.visShapes;selection_clear();frame_changed();break;
        case UI_LOCK_INK:G.lockInk=!G.lockInk;save_workspace();break;
        case UI_LOCK_MARKER:G.lockMarker=!G.lockMarker;save_workspace();break;
        case UI_LOCK_PHOTOS:G.lockPhotos=!G.lockPhotos;save_workspace();break;
        case UI_LOCK_CAD:G.lockCAD=!G.lockCAD;save_workspace();break;
        case UI_LOCK_NOTES:G.lockNotes=!G.lockNotes;save_workspace();break;
        case UI_LOCK_SHAPES:G.lockShapes=!G.lockShapes;save_workspace();break;
        case UI_LOCK_FRAMES:G.lockFrames=!G.lockFrames;save_workspace();break;
        case UI_SEARCH_CLOSE:G.searchPanel=0;break;
        case UI_SEARCH_EDIT:editor_open(3,-1,G.searchQuery);break;
        case UI_FRAME_RENAME:if(G.selectedFrame>=0&&G.selectedFrame<G.frameN&&!G.lockFrames&&!G.frameLocked[G.selectedFrame])editor_open(2,G.selectedFrame,G.frameNames[G.selectedFrame]);break;
        case UI_FRAME_SIZE_LOCK:if(G.selectedFrame>=0&&G.selectedFrame<G.frameN&&!G.lockFrames&&!G.frameLocked[G.selectedFrame]){G.frameSizeLocked[G.selectedFrame]=!G.frameSizeLocked[G.selectedFrame];save_frames();}break;
        case UI_FRAME_LOCK:if(G.selectedFrame>=0&&G.selectedFrame<G.frameN&&!G.lockFrames){G.frameLocked[G.selectedFrame]=!G.frameLocked[G.selectedFrame];frame_changed();}break;
        case UI_FRAME_DUP:frame_duplicate_selected();break;
        case UI_FRAME_EXPORT:if(export_selected_frame()){set_toast(7,ms);}else set_toast(8,ms);break;
        case UI_FRAME_PRESENT:start_presentation(G.selectedFrame>=0?G.selectedFrame:0);break;
        case UI_SELECTION_EDIT:if(G.selectedRefN==1){ObjRef r=G.selectedRefs[0];if(!objref_can_mutate(r)){set_toast(9,ms);break;}if(r.type==SEL_NOTE)editor_open(1,r.index,G.notes[r.index].text);else if(r.type==SEL_FRAME){G.selectedFrame=r.index;G.framePanel=1;G.addPanel=G.layersPanel=G.searchPanel=0;}else if(r.type==SEL_STROKES||r.type==SEL_SHAPE){open_color_picker();G.pickerApplySelection=1;}else if(r.type==SEL_IMAGE){G.selectedImage=r.index;G.photoMode=1;}else if(r.type==SEL_MEASURE){G.selectedMeasure=r.index;G.tool=MODE_MEASURE;}}break;
        case UI_SELECTION_DUP:if(selection_has_mutable_object())selection_duplicate();else set_toast(9,ms);break;
        case UI_SELECTION_GROUP:if(G.selectedRefN==1&&G.selectedRefs[0].type==SEL_FRAME){G.selectedFrame=G.selectedRefs[0].index;G.framePanel=1;G.addPanel=G.layersPanel=G.searchPanel=0;}else if(G.selectionType==SEL_GROUP&&G.selectedGroup>=0)selection_ungroup();else if(G.selectedRefN>1)selection_group();break;
        case UI_SELECTION_DELETE:if(selection_has_mutable_object())selection_delete();else set_toast(9,ms);break;
        case UI_SELECTION_MORE:G.selectionMoreOpen=!G.selectionMoreOpen;break;
        case UI_SELECTION_COLOR:G.selectionMoreOpen=0;if(selection_has_mutable_object()){G.pickerApplySelection=1;open_color_picker();}else set_toast(9,ms);break;
        case UI_SELECTION_LOCK:G.selectionMoreOpen=0;selection_toggle_lock();break;
        case UI_SELECTION_DESELECT:selection_clear();break;
        case UI_SHAPE_LINE:G.shapeType=SHAPE_LINE;break;
        case UI_SHAPE_RECT:G.shapeType=SHAPE_RECT;break;
        case UI_SHAPE_ELLIPSE:G.shapeType=SHAPE_ELLIPSE;break;
        case UI_SHAPE_ARROW:G.shapeType=SHAPE_ARROW;break;
        case UI_SHAPE_DONE:G.tool=MODE_PEN;break;
        case UI_PROJECT_CLOSE:G.projectsPanel=0;break;case UI_PROJECT_RENAME:editor_open(4,G.projectIndex,G.projectNames[G.projectIndex]);break;case UI_PROJECT_DUP:{int q=duplicate_current_project();if(q>=0)set_toast(11,ms);else set_toast(q==-1?10:8,ms);}break;case UI_PROJECT_CLEAR:clear_current_project();break;
        case UI_PRESENT_PREV:presentation_step(-1);break;
        case UI_PRESENT_NEXT:presentation_step(1);break;
        case UI_PRESENT_EXIT:G.presentationMode=0;G.zenMode=0;G.railOpen=1;start_anim_timer();break;
        case UI_EDITOR_CANCEL:editor_cancel();break;
        case UI_EDITOR_DONE:editor_apply();break;
        case UI_EDITOR_SPACE:editor_add_char(' ');break;
        case UI_EDITOR_BACKSPACE:if(G.editorLen>0){G.editorBuf[--G.editorLen]=0;}break;
        case UI_FRAME_NEW:G.framePanel=0;G.frameCreating=1;G.frameDragging=0;G.selectedFrame=-1;break;
        case UI_FRAME_CLOSE:G.framePanel=0;break;
        case UI_FRAME_PREV:if(G.framePage>0)G.framePage--;break;case UI_FRAME_NEXT:if((G.framePage+1)*5<G.frameN)G.framePage++;break;
        case UI_FRAME_JUMP:frame_jump(G.selectedFrame);break;
        case UI_FRAME_DELETE:frame_delete_selected();selection_clear();break;
        case UI_FRAME_FIT:if(G.selectedFrame>=0&&G.selectedFrame<G.frameN&&!G.frameLocked[G.selectedFrame]&&!G.lockFrames){G.frameMoveMode=1;G.frameMoveDragging=0;G.framePanel=0;}break;
        case UI_LAYER_INK:G.visInk=!G.visInk;selection_clear();frame_changed();break;case UI_LAYER_MARKER:G.visMarker=!G.visMarker;selection_clear();frame_changed();break;case UI_LAYER_PHOTOS:G.visPhotos=!G.visPhotos;selection_clear();frame_changed();break;case UI_LAYER_CAD:G.visCAD=!G.visCAD;selection_clear();frame_changed();break;case UI_LAYER_FRAMES:G.visFrames=!G.visFrames;selection_clear();frame_changed();break;
        case UI_FRAME_ITEM0:case UI_FRAME_ITEM1:case UI_FRAME_ITEM2:case UI_FRAME_ITEM3:case UI_FRAME_ITEM4:case UI_FRAME_ITEM5:{int fi=G.framePage*5+(id-UI_FRAME_ITEM0);if(fi<G.frameN)selection_set_one(SEL_FRAME,fi);}break;
        case UI_PEN:G.tool=MODE_PEN;G.photoMode=0;G.pressurePanel=0;break;
        case UI_HIGHLIGHT:G.tool=MODE_HIGHLIGHTER;G.photoMode=0;G.pressurePanel=0;break;
        case UI_ERASE:G.tool=MODE_ERASE;G.photoMode=0;G.pressurePanel=0;break;
        case UI_ERASE_ALL:G.tool=MODE_ERASE_ALL;G.photoMode=0;G.pressurePanel=0;break;
        case UI_MEASURE:G.tool=MODE_MEASURE;G.photoMode=0;G.pressurePanel=0;G.framePanel=0;break;
        case UI_FRAMES:G.framePanel=!G.framePanel;G.settingsPanel=0;G.galleryOpen=0;G.pressurePanel=0;G.photoMode=0;break;
        case UI_PHOTO_MODE:G.photoMode=!G.photoMode;G.pressurePanel=0;if(!G.photoMode)G.selectedImage=-1;break;
        case UI_IMPORT:open_photo_gallery();break;
        case UI_UNDO:G.lastTwoTapMs=ms;undo_action();break;
        case UI_REDO:redo_action();break;
        case UI_FIT:reset_view();break;
        case UI_PRESSURE:G.pressurePanel=!G.pressurePanel;G.photoMode=0;G.settingsPanel=0;break;
        case UI_MINIMAP:G.minimap=!G.minimap;G.minimapDirty=1;G.metaDirty=1;start_anim_timer();break;
        case UI_SETTINGS:G.settingsPanel=!G.settingsPanel;G.galleryOpen=0;G.photoMode=0;G.pressurePanel=0;break;
        case UI_CLEAR:if(G.clearArmed)clear_canvas();else G.clearArmed=1;break;
        case UI_BRUSH_MINUS:G.brush=clampf(G.brush/1.20f,1.0f,72.0f);G.metaDirty=1;break;
        case UI_BRUSH_PLUS:G.brush=clampf(G.brush*1.20f,1.0f,72.0f);G.metaDirty=1;break;
        case UI_COLOR_PICKER:G.pickerApplySelection=0;open_color_picker();break;
        case UI_INK_TOGGLE:G.photoMode=0;G.pressurePanel=0;G.tool=(G.tool==MODE_ERASE||G.tool==MODE_ERASE_ALL)?MODE_PEN:MODE_ERASE;break;
        case UI_RECENT_COLOR0:case UI_RECENT_COLOR1:case UI_RECENT_COLOR2:case UI_RECENT_COLOR3:case UI_RECENT_COLOR4:case UI_RECENT_COLOR5:{int ci=id-UI_RECENT_COLOR0;if(ci>=0&&ci<G.recentColorN){G.color=G.recentColors[ci]&0xffffffu;G.tool=MODE_PEN;G.photoMode=0;G.pressurePanel=0;G.metaDirty=1;}}break;
        case UI_COLOR_RAIL_OPEN:open_color_picker();break;
        case UI_PMIN_MINUS:G.pressureMin=clampf(G.pressureMin-0.05f,0.02f,G.pressureMax-0.05f);G.metaDirty=1;break;
        case UI_PMIN_PLUS:G.pressureMin=clampf(G.pressureMin+0.05f,0.02f,G.pressureMax-0.05f);G.metaDirty=1;break;
        case UI_PMAX_MINUS:G.pressureMax=clampf(G.pressureMax-0.10f,G.pressureMin+0.05f,1.8f);G.metaDirty=1;break;
        case UI_PMAX_PLUS:G.pressureMax=clampf(G.pressureMax+0.10f,G.pressureMin+0.05f,1.8f);G.metaDirty=1;break;
        case UI_CURVE_SOFT:G.pressureCurve=0;G.metaDirty=1;break;case UI_CURVE_LINEAR:G.pressureCurve=1;G.metaDirty=1;break;case UI_CURVE_FIRM:G.pressureCurve=2;G.metaDirty=1;break;
        case UI_SNAP:G.snap=!G.snap;G.metaDirty=1;break;case UI_CLEAR_DIM:clear_measures();break;case UI_DELETE_DIM:delete_selected_measure();break;case UI_CALIBRATE:open_calibration();break;
        case UI_PHOTO_DELETE:delete_selected_image();break;case UI_PHOTO_DONE:G.photoMode=0;G.selectedImage=-1;G.imageDragging=G.imageGesture=0;break;case UI_PHOTO_SNAP:G.photoAngleSnap=!G.photoAngleSnap;G.metaDirty=1;break;
        case UI_CAL_UNIT:G.cadUnit=(G.cadUnit+1)%5;G.metaDirty=1;break;
        case UI_CAL_CANCEL:G.calibrationOpen=0;system_text_stop();break;
        case UI_CAL_APPLY:{float v=parse_simple_decimal(G.calibText);if(!G.lockCAD&&v>0.000001f&&G.selectedMeasure>=0&&G.selectedMeasure<G.measureN){float wl=measure_len(&G.measures[G.selectedMeasure]);if(wl>0.000001f){G.cadScale=v/wl;G.cadCalibrated=1;G.metaDirty=1;G.calibrationOpen=0;}}}break;
        case UI_SETTINGS_CLOSE:G.settingsPanel=0;break;case UI_SETTINGS_APPEARANCE:G.settingsTab=0;break;case UI_SETTINGS_STYLUS:G.settingsTab=1;break;case UI_SETTINGS_DRAWING:G.settingsTab=2;break;case UI_SETTINGS_DISPLAY:G.settingsTab=3;break;case UI_SETTINGS_HANDWRITING:G.settingsTab=4;break;
        case UI_OCR_TOGGLE:G.ocrEnabled=!G.ocrEnabled;ocr_save_prefs();
#ifdef VAST_OCR
            if(G.ocr){ocr_manager_set_enabled(G.ocr,G.ocrEnabled);if(G.ocrEnabled)ocr_begin_initial_scan();else G.ocrInitialPending=0;}
#endif
            break;
        case UI_OCR_REBUILD:
#ifdef VAST_OCR
            if(G.ocr)ocr_manager_rebuild(G.ocr);
#endif
            break;
        case UI_BUTTONS_TOGGLE:G.buttonBindingsEnabled=!G.buttonBindingsEnabled;if(!G.buttonBindingsEnabled){G.stylusButtonDown=0;G.buttonQuickErase=0;G.pendingButtonSingle=0;cancel_button_timer();}G.metaDirty=1;break;
        case UI_SCALE_MINUS:G.uiScale=clampf(G.uiScale-0.05f,0.70f,1.55f);G.metaDirty=1;break;case UI_SCALE_PLUS:G.uiScale=clampf(G.uiScale+0.05f,0.70f,1.55f);G.metaDirty=1;break;
        case UI_ROLE0:case UI_ROLE1:case UI_ROLE2:case UI_ROLE3:case UI_ROLE4:case UI_ROLE5:case UI_ROLE6:case UI_ROLE7:G.themeRole=id-UI_ROLE0;break;
        case UI_PALETTE0:case UI_PALETTE1:case UI_PALETTE2:case UI_PALETTE3:case UI_PALETTE4:case UI_PALETTE5:case UI_PALETTE6:case UI_PALETTE7:case UI_PALETTE8:case UI_PALETTE9:case UI_PALETTE10:case UI_PALETTE11:theme_role_set(G.themeRole,THEME_PALETTE[id-UI_PALETTE0]);break;
        case UI_THEME_RESET:theme_preset(0);G.metaDirty=1;break;
        case UI_PRESS_MINUS:G.buttonPressAction=action_cycle(G.buttonPressAction,-1);G.metaDirty=1;break;case UI_PRESS_PLUS:G.buttonPressAction=action_cycle(G.buttonPressAction,1);G.metaDirty=1;break;
        case UI_HOLD_MINUS:G.buttonHoldAction=action_cycle(G.buttonHoldAction,-1);G.metaDirty=1;break;case UI_HOLD_PLUS:G.buttonHoldAction=action_cycle(G.buttonHoldAction,1);G.metaDirty=1;break;
        case UI_DOUBLE_MINUS:G.buttonDoubleAction=action_cycle(G.buttonDoubleAction,-1);G.metaDirty=1;break;case UI_DOUBLE_PLUS:G.buttonDoubleAction=action_cycle(G.buttonDoubleAction,1);G.metaDirty=1;break;
        case UI_PSMOOTH_MINUS:G.pressureSmoothing=clampf(G.pressureSmoothing-0.05f,0,1);G.metaDirty=1;break;case UI_PSMOOTH_PLUS:G.pressureSmoothing=clampf(G.pressureSmoothing+0.05f,0,1);G.metaDirty=1;break;
        case UI_SSMOOTH_MINUS:G.strokeSmoothing=clampf(G.strokeSmoothing-0.05f,0,1);G.metaDirty=1;break;case UI_SSMOOTH_PLUS:G.strokeSmoothing=clampf(G.strokeSmoothing+0.05f,0,1);G.metaDirty=1;break;
        case UI_HILITE_MINUS:G.highlighterOpacity=clampf(G.highlighterOpacity-0.05f,0.08f,0.80f);G.metaDirty=1;break;case UI_HILITE_PLUS:G.highlighterOpacity=clampf(G.highlighterOpacity+0.05f,0.08f,0.80f);G.metaDirty=1;break;case UI_RADIAL_MINUS:G.radialHoldSec=clampf(G.radialHoldSec-0.1f,0.35f,3.0f);G.metaDirty=1;break;case UI_RADIAL_PLUS:G.radialHoldSec=clampf(G.radialHoldSec+0.1f,0.35f,3.0f);G.metaDirty=1;break;case UI_DISP0_MINUS:case UI_DISP0_PLUS:case UI_DISP1_MINUS:case UI_DISP1_PLUS:case UI_DISP2_MINUS:case UI_DISP2_PLUS:case UI_DISP3_MINUS:case UI_DISP3_PLUS:case UI_DISP4_MINUS:case UI_DISP4_PLUS:case UI_DISP5_MINUS:case UI_DISP5_PLUS:case UI_DISP6_MINUS:case UI_DISP6_PLUS:case UI_DISP7_MINUS:case UI_DISP7_PLUS:case UI_DISP8_MINUS:case UI_DISP8_PLUS:case UI_DISP9_MINUS:case UI_DISP9_PLUS:{int z=id-UI_DISP0_MINUS,row=z/2,dir=(z&1)?1:-1;display_adjust(row,dir);}break;
        case UI_PICKER_DONE:close_color_picker();break;
        case UI_GALLERY_CLOSE:G.galleryOpen=0;free_thumbs();break;
        case UI_GALLERY_PREV:if(G.galleryPage>0){G.galleryPage--;load_gallery_page();}break;
        case UI_GALLERY_NEXT:if((G.galleryPage+1)*12<G.mediaCount){G.galleryPage++;load_gallery_page();}break;
        default:
            if(id>=UI_SEARCH_RESULT0&&id<=UI_SEARCH_RESULT5){int ri=id-UI_SEARCH_RESULT0;if(ri<G.searchVisibleN)search_jump(G.searchVisible[ri]);}
            else if(id>=UI_BOOKMARK_ITEM0&&id<=UI_BOOKMARK_ITEM3){int bi=id-UI_BOOKMARK_ITEM0;if(bi<G.bookmarkN)bookmark_jump(bi);}
            else if(id>=UI_BOOKMARK_RENAME0&&id<=UI_BOOKMARK_RENAME2){int bi=id-UI_BOOKMARK_RENAME0;if(bi<G.bookmarkN)editor_open(5,bi,G.bookmarks[bi].name);}
            else if(id>=UI_BOOKMARK_DELETE0&&id<=UI_BOOKMARK_DELETE2){int bi=id-UI_BOOKMARK_DELETE0;if(bi<G.bookmarkN)bookmark_delete(bi);}
            else if(id>=UI_PROJECT0&&id<=UI_PROJECT3){switch_project(id-UI_PROJECT0);}
            else if(id>=UI_EDITOR_KEY0&&id<UI_EDITOR_KEY0+26){const char*keys="QWERTYUIOPASDFGHJKLZXCVBNM";editor_add_char(keys[id-UI_EDITOR_KEY0]);}
            else if(id>=UI_CAL_KEY0&&id<UI_CAL_KEY0+12){int k=id-UI_CAL_KEY0;const char keys[12]={'1','2','3','4','5','6','7','8','9','.','0','<'};if(keys[k]=='<')calibration_backspace();else calibration_append(keys[k]);}
            else if(id>=UI_GALLERY_ITEM0&&id<UI_GALLERY_ITEM0+12){import_gallery_item(id-UI_GALLERY_ITEM0);if(G.toastMs==0)G.toastMs=ms;}
            break;
    }
    if(G.railOpen&&(id==UI_ADD||id==UI_PHOTO_MODE||id==UI_MEASURE||id==UI_FRAMES||id==UI_SEARCH||id==UI_LAYERS||id==UI_MINIMAP||id==UI_SETTINGS))arm_rail_timer();
    if(G.toast&&G.toastMs==0)G.toastMs=ms;
    if(G.saveDirty){save_canvas();ocr_checkpoint();}save_meta();save_images();save_workspace();render();
}
#include "android_ui.inc"
static void minimap_camera_changed(void){
    if(!G.minimapCache||G.minimapMaxX<=G.minimapMinX||G.minimapMaxY<=G.minimapMinY){G.minimapDirty=1;return;}
    float vx0=(0-G.offX)/G.scale,vy0=(0-G.offY)/G.scale,vx1=(G.screenW-G.offX)/G.scale,vy1=(G.screenH-G.offY)/G.scale;
    if(fmin2(vx0,vx1)<G.minimapMinX||fmax2(vx0,vx1)>G.minimapMaxX||fmin2(vy0,vy1)<G.minimapMinY||fmax2(vy0,vy1)>G.minimapMaxY)G.minimapDirty=1;
}
static void zoom_about(float cx,float cy,float factor){float old=G.scale,ns=clampf(old*factor,0.05f,20.0f),wx=(cx-G.offX)/old,wy=(cy-G.offY)/old;G.scale=ns;G.offX=cx-wx*ns;G.offY=cy-wy*ns;minimap_camera_changed();}
static void register_two_finger_tap(float mx,float my,int64_t now){G.lastTwoTapMs=now;G.lastTwoTapX=mx;G.lastTwoTapY=my;undo_action();}


static void cancel_button_timer(void){if(G.buttonTimerFd<0)return;itimerspec_t it;memset(&it,0,sizeof(it));timerfd_settime(G.buttonTimerFd,0,&it,0);G.buttonTimerMode=0;}
static void arm_button_timer(int ms,int mode){if(G.buttonTimerFd<0)return;itimerspec_t it;memset(&it,0,sizeof(it));it.it_value.tv_sec=ms/1000;it.it_value.tv_nsec=(long)(ms%1000)*1000000L;timerfd_settime(G.buttonTimerFd,0,&it,0);G.buttonTimerMode=mode;}
static void cancel_rail_timer(void){if(G.railTimerFd<0)return;itimerspec_t it;memset(&it,0,sizeof(it));timerfd_settime(G.railTimerFd,0,&it,0);}
static void arm_rail_timer(void){if(G.railTimerFd<0)return;itimerspec_t it;memset(&it,0,sizeof(it));it.it_value.tv_sec=6;timerfd_settime(G.railTimerFd,0,&it,0);}
static int rail_timer_cb(int fd,int events,void*data){(void)events;(void)data;uint64_t ticks=0;read(fd,&ticks,sizeof(ticks));if(G.railOpen){G.railOpen=0;start_anim_timer();render();}return 1;}
static void next_ink_color(void){static const uint32_t cols[5]={0xf4f6f8,0x6aa9ff,0xff6475,0x5fe39a,0xffc55d};int best=0;for(int i=0;i<5;i++)if(G.color==cols[i]){best=(i+1)%5;G.color=cols[best];return;}G.color=cols[0];}
static void execute_button_action(int action,int fromHold){
    /* A side-button timer must not change history/tool ownership halfway
       through a canvas gesture. Quick erase is armed for the next contact. */
    if(G.stylusDown){if(action==BA_QUICK_ERASE&&fromHold)G.buttonQuickErase=1;return;}
    switch(action){
        case BA_UNDO:undo_action();break;case BA_REDO:redo_action();break;
        case BA_QUICK_ERASE:if(fromHold)G.buttonQuickErase=1;else{G.tool=MODE_ERASE;G.pressurePanel=0;}break;
        case BA_ERASER:G.tool=MODE_ERASE;G.pressurePanel=0;break;case BA_PEN:G.tool=MODE_PEN;break;case BA_MEASURE:G.tool=MODE_MEASURE;G.pressurePanel=0;break;
        case BA_FIT:reset_view();break;case BA_NEXT_COLOR:next_ink_color();G.tool=MODE_PEN;break;
        case BA_PRESSURE:G.pressurePanel=!G.pressurePanel;G.settingsPanel=0;break;
        case BA_HIGHLIGHTER:G.tool=MODE_HIGHLIGHTER;G.pressurePanel=0;break;case BA_COLOR_PICKER:G.pickerApplySelection=0;open_color_picker();break;
        case BA_SETTINGS:G.settingsPanel=!G.settingsPanel;G.pressurePanel=0;G.galleryOpen=0;break;
        case BA_IMPORT:open_photo_gallery();break;default:break;
    }
    if(action==BA_UNDO||action==BA_REDO)save_all_document();else save_meta();render();
}
static int button_timer_cb(int fd,int events,void*data){(void)events;(void)data;uint64_t ticks=0;read(fd,&ticks,sizeof(ticks));int mode=G.buttonTimerMode;G.buttonTimerMode=0;if(mode==1&&G.stylusButtonDown&&!G.buttonHoldFired){G.buttonHoldFired=1;G.pendingButtonSingle=0;execute_button_action(G.buttonHoldAction,1);}else if(mode==2&&G.pendingButtonSingle&&!G.stylusButtonDown){G.pendingButtonSingle=0;execute_button_action(G.buttonPressAction,0);}return 1;}
static void handle_button_state(int down,int64_t now){
    if(!G.buttonBindingsEnabled){G.stylusButtonDown=0;G.buttonQuickErase=0;G.pendingButtonSingle=0;cancel_button_timer();return;}
    if(down==G.stylusButtonDown)return;
    if(down){
        if(G.pendingButtonSingle){G.pendingButtonSingle=0;cancel_button_timer();G.buttonDoubleSecond=1;}else G.buttonDoubleSecond=0;
        G.stylusButtonDown=1;G.buttonHoldFired=0;G.buttonDownMs=now;arm_button_timer(430,1);
    }else{
        G.stylusButtonDown=0;if(G.buttonQuickErase)G.buttonQuickErase=0;cancel_button_timer();
        if(G.buttonHoldFired){G.buttonHoldFired=0;G.buttonDoubleSecond=0;render();return;}
        if(G.buttonDoubleSecond){G.buttonDoubleSecond=0;G.pendingButtonSingle=0;execute_button_action(G.buttonDoubleAction,0);}
        else{G.pendingButtonSingle=1;arm_button_timer(300,2);render();}
    }
}


static void cancel_pen_hold_timer(void){if(G.penHoldTimerFd>=0){itimerspec_t it;memset(&it,0,sizeof(it));timerfd_settime(G.penHoldTimerFd,0,&it,0);}G.penHoldArmed=0;G.penHoldVisual=0.0f;G.touchHoldMode=0;G.touchHoldImage=-1;G.radialSizing=0;G.radialSizeActive=0;}
static void arm_touch_hold_timer(int64_t now,int mode,int target,float sec){if(G.penHoldTimerFd<0)return;itimerspec_t it;memset(&it,0,sizeof(it));sec=clampf(sec,0.30f,3.0f);float timerSec=sec+(mode==1?RADIAL_PROGRESS_DELAY:0.0f);it.it_value.tv_sec=(time_t)timerSec;it.it_value.tv_nsec=(long)((timerSec-(float)it.it_value.tv_sec)*1000000000.0f);timerfd_settime(G.penHoldTimerFd,0,&it,0);G.penHoldArmed=1;G.penHoldMoved=0;G.penHoldMaxMove=0;G.penHoldVisual=0.0f;G.penHoldStartMs=now;G.touchHoldMode=mode;G.touchHoldImage=target;if(mode==1)start_anim_timer();}
static int finger_pan_sample(float x,float y,int64_t sampleMs){
    float dx0=x-G.fingerDownX,dy0=y-G.fingerDownY,md=sqrtf(dx0*dx0+dy0*dy0),slop=(float)us(6);if(md>G.penHoldMaxMove)G.penHoldMaxMove=md;
    if(G.penHoldArmed&&md>pen_hold_buffer_px()){G.penHoldMoved=1;cancel_pen_hold_timer();}
    if(!G.fingerPanning){if(md<=slop)return 0;float inv=1.0f/fmax2(md,.001f);G.lastFingerX=G.fingerDownX+dx0*inv*slop;G.lastFingerY=G.fingerDownY+dy0*inv*slop;G.fingerPanning=1;cancel_pen_hold_timer();}
    float panX=x-G.lastFingerX,panY=y-G.lastFingerY;if(panX==0&&panY==0)return 0;G.fingerMoved=1;G.offX+=panX;G.offY+=panY;fling_track_add(panX,panY,sampleMs);G.lastFingerX=x;G.lastFingerY=y;minimap_camera_changed();return 1;
}
static int pen_hold_timer_cb(int fd,int events,void*data){(void)events;(void)data;uint64_t ticks=0;read(fd,&ticks,sizeof(ticks));if(!G.penHoldArmed||G.fingerCount!=1||G.penHoldMoved||G.penHoldMaxMove>pen_hold_buffer_px())return 1;int mode=G.touchHoldMode,target=G.touchHoldImage;G.penHoldArmed=0;G.touchHoldMode=0;G.touchHoldImage=-1;if(mode==1&&!G.settingsPanel&&!G.galleryOpen&&!G.framePanel&&!G.calibrationOpen&&!G.photoMode){G.penHoldVisual=1.0f;float r=us(165);G.radialX=clampf(G.penHoldX,r,G.screenW-r);G.radialY=clampf(G.penHoldY,r,G.screenH-r);G.radialHot=-1;G.radialOpen=1;G.radialSizing=0;G.radialSizeActive=0;G.radialSizePreview=G.radialSizeStart=G.brush;G.radialAnim=0.0f;G.radialVel=0.0f;G.fingerPanning=0;start_anim_timer();render();}else if(mode==2&&target>=0&&target<G.imageN&&G.images[target].active&&!G.lockPhotos){selection_set_one(SEL_IMAGE,target);G.selectedImage=target;G.photoMode=1;G.zenMode=0;G.railOpen=1;G.imageDragging=1;G.imageCancelStart=G.images[target];G.imageCancelValid=1;G.imageStartX=G.images[target].x;G.imageStartY=G.images[target].y;G.lastFingerX=G.penHoldX;G.lastFingerY=G.penHoldY;G.penHoldVisual=0;render();}return 1;}
static void commit_live_shape(void){if(!G.shapeDrawing||G.shapeN>=128||G.lockShapes){G.shapeDrawing=0;return;}float dx=G.liveShape.x1-G.liveShape.x0,dy=G.liveShape.y1-G.liveShape.y0;if(sqrtf(dx*dx+dy*dy)*G.scale<10.0f){G.shapeDrawing=0;return;}G.liveShape.id=G.nextShapeId++;G.liveShape.active=1;G.liveShape.locked=0;G.liveShape.color=G.color;G.liveShape.width=fmax2(2.0f,G.brush);G.shapes[G.shapeN]=G.liveShape;selection_set_one(SEL_SHAPE,G.shapeN);G.selectedShape=G.shapeN++;G.shapeDrawing=0;G.sceneRevision++;G.minimapDirty=1;save_workspace();}
static int clean_current_stroke_to_shape(int64_t now){if(G.currentStroke<0||G.currentStroke>=G.strokeN||G.tool!=MODE_PEN||G.shapeN>=128||G.lockShapes)return 0;Stroke*st=&G.strokes[G.currentStroke];if(st->n<4||now-G.strokeLastMoveMs<320)return 0;Point a=st->pts[0],b=st->pts[st->n-1];float dx=b.x-a.x,dy=b.y-a.y,bird=sqrtf(dx*dx+dy*dy),path=0,maxDev=0;for(int i=1;i<st->n;i++){float ex=st->pts[i].x-st->pts[i-1].x,ey=st->pts[i].y-st->pts[i-1].y;path+=sqrtf(ex*ex+ey*ey);}float den=fmax2(bird,0.0001f);for(int i=1;i<st->n-1;i++){float dev=fabsf(dy*st->pts[i].x-dx*st->pts[i].y+b.x*a.y-b.y*a.x)/den;if(dev>maxDev)maxDev=dev;}float bw=st->maxx-st->minx,bh=st->maxy-st->miny,diag=sqrtf(bw*bw+bh*bh);int type=-1;if(bird>0.01f&&path/bird<1.18f&&maxDev*G.scale<7.0f)type=SHAPE_LINE;else if(diag>0.01f&&bird/diag<0.24f&&path>diag*2.0f)type=SHAPE_ELLIPSE;if(type<0)return 0;ShapeObj*q=&G.shapes[G.shapeN];memset(q,0,sizeof(*q));q->id=G.nextShapeId++;q->active=1;q->type=type;q->color=st->color&0xffffffu;q->width=fmax2(2.0f,st->baseWidth);if(type==SHAPE_LINE){q->x0=a.x;q->y0=a.y;q->x1=b.x;q->y1=b.y;}else{q->x0=st->minx;q->y0=st->miny;q->x1=st->maxx;q->y1=st->maxy;}int si=G.shapeN++;cancel_current_stroke();selection_set_one(SEL_SHAPE,si);G.selectedShape=si;G.sceneRevision++;G.minimapDirty=1;save_workspace();return 1;}
static void transform_xy(float*x,float*y,float cx,float cy,float sc,float ang,float tx,float ty){float dx=*x-cx,dy=*y-cy,ca=cosf(ang),sa=sinf(ang);float nx=(dx*ca-dy*sa)*sc,ny=(dx*sa+dy*ca)*sc;*x=cx+nx+tx;*y=cy+ny+ty;}
static void selection_snapshot_begin(void){
    G.transformSnapshotN=0;for(int i=0;i<G.selectedRefN&&G.transformSnapshotN<256;i++){ObjRef r=G.selectedRefs[i];TransformSnapshot*s=&G.transformSnapshots[G.transformSnapshotN];int ok=1;if(r.type==SEL_IMAGE&&r.index>=0&&r.index<G.imageN)s->image=G.images[r.index];else if(r.type==SEL_MEASURE&&r.index>=0&&r.index<G.measureN)s->measure=G.measures[r.index];else if(r.type==SEL_NOTE&&r.index>=0&&r.index<G.noteN)s->note=G.notes[r.index];else if(r.type==SEL_SHAPE&&r.index>=0&&r.index<G.shapeN)s->shape=G.shapes[r.index];else ok=0;if(ok)G.transformSnapshotRefs[G.transformSnapshotN++]=r;}
}
static void selection_snapshot_restore(void){
    for(int i=0;i<G.transformSnapshotN;i++){ObjRef r=G.transformSnapshotRefs[i];TransformSnapshot*s=&G.transformSnapshots[i];if(r.type==SEL_IMAGE&&r.index>=0&&r.index<G.imageN){G.images[r.index]=s->image;G.imagesDirty=1;}else if(r.type==SEL_MEASURE&&r.index>=0&&r.index<G.measureN){G.measures[r.index]=s->measure;G.metaDirty=1;}else if(r.type==SEL_NOTE&&r.index>=0&&r.index<G.noteN)G.notes[r.index]=s->note;else if(r.type==SEL_SHAPE&&r.index>=0&&r.index<G.shapeN)G.shapes[r.index]=s->shape;}G.transformSnapshotN=0;G.sceneRevision++;G.minimapDirty=1;
}
static void selection_drag_restore(void){if(G.selectionDragTotalX!=0||G.selectionDragTotalY!=0){for(int i=0;i<G.selectedRefN;i++)move_objref(G.selectedRefs[i],-G.selectionDragTotalX,-G.selectionDragTotalY);G.sceneRevision++;G.minimapDirty=1;}G.selectionDragTotalX=G.selectionDragTotalY=0;}
static void selection_transform_by(float oldMx,float oldMy,float newMx,float newMy,float sc,float ang){float tx=newMx-oldMx,ty=newMy-oldMy;sc=clampf(sc,.25f,4.0f);for(int i=0;i<G.selectedRefN;i++){ObjRef r=G.selectedRefs[i];if(r.type==SEL_STROKES&&r.index>=0&&r.index<G.strokeN){Stroke*st=&G.strokes[r.index];int aa=(st->color>>24)&255,marker=aa>0&&aa<248;if((marker&&G.lockMarker)||(!marker&&G.lockInk))continue;for(int k=0;k<st->n;k++)transform_xy(&st->pts[k].x,&st->pts[k].y,oldMx,oldMy,sc,ang,tx,ty);st->minx=st->maxx=st->pts[0].x;st->miny=st->maxy=st->pts[0].y;for(int k=1;k<st->n;k++){st->minx=fmin2(st->minx,st->pts[k].x);st->maxx=fmax2(st->maxx,st->pts[k].x);st->miny=fmin2(st->miny,st->pts[k].y);st->maxy=fmax2(st->maxy,st->pts[k].y);}G.saveDirty=1;}else if(r.type==SEL_IMAGE&&r.index>=0&&r.index<G.imageN&&!G.lockPhotos){ImageObj*im=&G.images[r.index];float cx=im->x+im->w*.5f,cy=im->y+im->h*.5f;transform_xy(&cx,&cy,oldMx,oldMy,sc,ang,tx,ty);im->w=fmax2(20,im->w*sc);im->h=fmax2(20,im->h*sc);im->x=cx-im->w*.5f;im->y=cy-im->h*.5f;im->rot+=ang;G.imagesDirty=1;}else if(r.type==SEL_MEASURE&&r.index>=0&&r.index<G.measureN&&!G.lockCAD){Measure*m=&G.measures[r.index];transform_xy(&m->ax,&m->ay,oldMx,oldMy,sc,ang,tx,ty);transform_xy(&m->bx,&m->by,oldMx,oldMy,sc,ang,tx,ty);G.metaDirty=1;}else if(r.type==SEL_NOTE&&r.index>=0&&r.index<G.noteN&&!G.lockNotes&&!G.notes[r.index].locked){NoteObj*n=&G.notes[r.index];float cx=n->x+n->w*.5f,cy=n->y+n->h*.5f;transform_xy(&cx,&cy,oldMx,oldMy,1.0f,0,tx,ty);note_autosize(n);n->x=cx-n->w*.5f;n->y=cy-n->h*.5f;}else if(r.type==SEL_SHAPE&&r.index>=0&&r.index<G.shapeN&&!G.lockShapes&&!G.shapes[r.index].locked){ShapeObj*q=&G.shapes[r.index];transform_xy(&q->x0,&q->y0,oldMx,oldMy,sc,ang,tx,ty);transform_xy(&q->x1,&q->y1,oldMx,oldMy,sc,ang,tx,ty);q->width=fmax2(.5f,q->width*sc);}else if(r.type==SEL_FRAME&&r.index>=0&&r.index<G.frameN&&!G.lockFrames&&!G.frameLocked[r.index]){FrameObj*f=&G.frames[r.index];f->x+=tx;f->y+=ty;G.frameRevision++;}}G.sceneRevision++;G.minimapDirty=1;}
static int selection_gesture_contains(float sx0,float sy0,float sx1,float sy1){for(int i=0;i<G.selectedRefN;i++)if(G.selectedRefs[i].type==SEL_STROKES||G.selectedRefs[i].type==SEL_FRAME)return 0;float x0,y0,x1,y1;if(!selection_bounds(&x0,&y0,&x1,&y1))return 0;float p=us(28),ax=x0*G.scale+G.offX-p,ay=y0*G.scale+G.offY-p,bx=x1*G.scale+G.offX+p,by=y1*G.scale+G.offY+p;return sx0>=ax&&sx0<=bx&&sy0>=ay&&sy0<=by&&sx1>=ax&&sx1<=bx&&sy1>=ay&&sy1<=by;}
static int group_for_ref(ObjRef r){for(int i=G.groupN-1;i>=0;i--){GroupObj*g=&G.groups[i];if(!g->active)continue;for(int k=0;k<g->n;k++)if(g->refs[k].type==r.type&&g->refs[k].index==r.index)return i;}return -1;}
static void select_hit_ref(ObjRef r){if(r.type==SEL_NONE){selection_clear();return;}int gi=group_for_ref(r);if(gi>=0){GroupObj*g=&G.groups[gi];selection_clear();G.selectedRefN=mini(g->n,256);for(int k=0;k<G.selectedRefN;k++)G.selectedRefs[k]=g->refs[k];G.selectionType=SEL_GROUP;G.selectedGroup=gi;return;}selection_set_one(r.type,r.index);}
// ---------- Input ----------
static int photo_begin_gesture(float x0,float y0,float x1,float y1){if(!G.photoMode||G.selectedImage<0||G.selectedImage>=G.imageN||G.lockPhotos)return 0;ImageObj*im=&G.images[G.selectedImage];if(!im->active)return 0;if(!image_contains(im,x0,y0)&&!image_contains(im,x1,y1)&&!image_contains(im,(x0+x1)*.5f,(y0+y1)*.5f))return 0;if(!G.imageCancelValid){G.imageCancelStart=*im;G.imageCancelValid=1;}float dx=x1-x0,dy=y1-y0;G.imageGesture=1;G.imageDragging=0;G.imageGestureDist=fmax2(4.0f,sqrtf(dx*dx+dy*dy));G.imageGestureAngle=atan2f(dy,dx);G.imageGestureMidX=(x0+x1)*.5f;G.imageGestureMidY=(y0+y1)*.5f;G.imageStartW=im->w;G.imageStartH=im->h;G.imageStartRot=im->rot;G.imageStartCX=im->x+im->w*.5f;G.imageStartCY=im->y+im->h*.5f;return 1;}
static void photo_update_gesture(float x0,float y0,float x1,float y1){if(!G.imageGesture||G.selectedImage<0||G.selectedImage>=G.imageN)return;ImageObj*im=&G.images[G.selectedImage];float dx=x1-x0,dy=y1-y0,d=fmax2(4.0f,sqrtf(dx*dx+dy*dy)),a=atan2f(dy,dx),f=clampf(d/G.imageGestureDist,.12f,8.0f),mx=(x0+x1)*.5f,my=(y0+y1)*.5f;im->w=fmax2(20.0f,G.imageStartW*f);im->h=fmax2(20.0f,G.imageStartH*f);im->rot=G.imageStartRot+(a-G.imageGestureAngle);if(G.photoAngleSnap){float step=15.0f*PI/180.0f;float q=im->rot/step;im->rot=floorf(q+(q>=0?0.5f:-0.5f))*step;}float cx=G.imageStartCX+(mx-G.imageGestureMidX)/G.scale,cy=G.imageStartCY+(my-G.imageGestureMidY)/G.scale;im->x=cx-im->w*.5f;im->y=cy-im->h*.5f;G.imagesDirty=1;G.minimapDirty=1;}
static void photo_drag_to(float x,float y){if(!G.imageDragging||G.selectedImage<0||G.selectedImage>=G.imageN)return;ImageObj*im=&G.images[G.selectedImage];im->x+=(x-G.lastFingerX)/G.scale;im->y+=(y-G.lastFingerY)/G.scale;G.lastFingerX=x;G.lastFingerY=y;G.imagesDirty=1;G.minimapDirty=1;}
static int minimap_navigate(float sx,float sy){if(!G.minimap||!G.minimapCache||G.minimapMaxX<=G.minimapMinX||G.minimapMaxY<=G.minimapMinY)return 0;int m=margin_ui(),w=us(320),h=us(205),x=G.screenW-m-w,y=G.screenH-m-h-us(100),pad=us(14),top=us(40),cw=w-2*pad,ch=h-top-pad,dx=x+pad,dy=y+top;if(!hit_box(sx,sy,dx,dy,dx+cw,dy+ch))return 0;float minx=G.minimapMinX,miny=G.minimapMinY,maxx=G.minimapMaxX,maxy=G.minimapMaxY,rx=(cw-4.0f)/(maxx-minx),ry=(ch-4.0f)/(maxy-miny),r=fmin2(rx,ry),ox=dx+2.0f+(cw-4.0f-(maxx-minx)*r)*.5f-minx*r,oy=dy+2.0f+(ch-4.0f-(maxy-miny)*r)*.5f-miny*r;float wx=(sx-ox)/r,wy=(sy-oy)/r;G.offX=G.screenW*.5f-wx*G.scale;G.offY=G.screenH*.5f-wy*G.scale;minimap_camera_changed();return 1;}
static int pointer_index_for_id(const AInputEvent*e,size_t pc,int id){for(size_t i=0;i<pc;i++)if(AMotionEvent_getPointerId(e,i)==id)return(int)i;return -1;}
static int finger_pair_indices(const AInputEvent*e,size_t pc,int*i0,int*i1){
    if(pc<2||!i0||!i1)return 0;int a=pointer_index_for_id(e,pc,G.fingerId0),b=pointer_index_for_id(e,pc,G.fingerId1);
    if(a<0||b<0||a==b){a=0;b=1;G.fingerId0=AMotionEvent_getPointerId(e,0);G.fingerId1=AMotionEvent_getPointerId(e,1);}*i0=a;*i1=b;return 1;
}
static int color_pointer_handle(int masked,float x,float y,int64_t now){
    if(android_ui_available())return 0;
    if(masked==AMOTION_ACTION_DOWN){
        if(color_handle_hit(x,y)){G.colorRailDragging=1;G.colorRailDragStartX=x;G.colorRailDragStartAnim=G.colorRailAnim;G.pickerDragging=0;return 1;}
        if(G.colorRailAnim>.08f&&color_picker_field_hit(x,y)){color_picker_touch(x,y);render_event(now,0);return 1;}
        if(G.colorRailAnim>.08f){int ri=color_picker_recent_hit(x,y);if(ri>=0){if(G.pickerSessionTouched)recent_color_add_from_picker(G.color);G.color=G.recentColors[ri]&0xffffffu;rgb_hsv(G.color,&G.pickerHue,&G.pickerSat,&G.pickerVal);G.metaDirty=1;G.pickerSessionTouched=0;render_event(now,0);return 1;}}
        if(color_panel_hit(x,y)){int phit=ui_hit(x,y);if(phit==UI_PICKER_DONE)return 0;return 1;}
    }else if(masked==AMOTION_ACTION_MOVE){
        if(G.colorRailDragging){float travel=(float)maxi(1,color_drawer_w()-color_handle_w());float delta=G.colorRailDragStartX-x;G.colorRailAnim=clampf(G.colorRailDragStartAnim+delta/travel,0.0f,1.0f);G.colorRailVel=0;render_event(now,0);return 1;}
        if(G.pickerDragging){color_picker_touch(x,y);render_event(now,0);return 1;}
    }else if(masked==AMOTION_ACTION_UP||masked==AMOTION_ACTION_CANCEL){
        if(G.pickerDragging){G.pickerDragging=0;render_event(now,1);return 1;}
        if(G.colorRailDragging){float moved=fabsf(x-G.colorRailDragStartX);G.colorRailDragging=0;if(masked==AMOTION_ACTION_CANCEL){G.colorPickerOpen=G.colorRailAnim>=.5f;}else if(moved<us(8)){if(G.colorPickerOpen)close_color_picker();else open_color_picker();render_event(now,1);return 1;}else{int want=G.colorRailAnim>=.35f;if(!want&&G.colorPickerOpen)picker_commit_session();if(want&&!G.colorPickerOpen){rgb_hsv(G.color,&G.pickerHue,&G.pickerSat,&G.pickerVal);G.pickerSessionStartColor=G.color;G.pickerSessionTouched=0;}G.colorPickerOpen=want;}start_anim_timer();render_event(now,1);return 1;}
    }
    return 0;
}
static int ui_pointer_route(int masked,float x,float y,int64_t now){
    if(masked==AMOTION_ACTION_DOWN){int hit=ui_hit(x,y);if(hit){G.pressedUi=hit;render_event(now,0);return 1;}return 0;}
    if(!G.pressedUi)return 0;
    if(masked==AMOTION_ACTION_POINTER_DOWN||masked==AMOTION_ACTION_CANCEL){G.pressedUi=0;render_event(now,1);return 1;}
    if(masked==AMOTION_ACTION_MOVE)return 1;
    if(masked==AMOTION_ACTION_UP){int id=G.pressedUi;G.pressedUi=0;if(ui_hit(x,y)==id)handle_ui(id,now);else render_event(now,1);return 1;}
    return 1;
}
static void cancel_canvas_contact(void){
    fling_cancel();
    if(G.stylusDown){
        if(G.erasing){for(int i=0;i<G.eraseAction.n;i++){int id=G.eraseAction.ids[i];if(id>=0&&id<G.strokeN)G.strokes[id].active=1;}action_set_refs_active(&G.eraseAction,1);free_action(&G.eraseAction);G.saveDirty=1;}
        else if(G.currentStroke>=0)cancel_current_stroke();
        if(G.frameResizeDragging&&G.selectedFrame>=0&&G.selectedFrame<G.frameN)G.frames[G.selectedFrame]=G.frameGestureStart;
        if(G.frameMoveDragging)frame_move_selected_by(-G.frameMoveTotalX,-G.frameMoveTotalY);
        if(G.selectionDragging)selection_drag_restore();
        G.sceneRevision++;G.minimapDirty=1;
    }
    G.stylusDown=G.erasing=G.eraseEverythingActive=G.hoverActive=G.stylusToolType=0;G.stylusPointerId=-1;
    G.frameResizeDragging=G.frameMoveDragging=G.frameDragging=G.shapeDrawing=G.measuring=G.lassoActive=G.lassoN=G.selectionDragging=0;
    G.frameMoveTotalX=G.frameMoveTotalY=G.selectionDragTotalX=G.selectionDragTotalY=0;
    G.pressedUi=0;G.fingerCount=0;G.fingerId0=G.fingerId1=-1;cancel_pen_hold_timer();ocr_set_interaction(0);ocr_move_commit();
}
static void handle_motion(AInputEvent*e){
    int action=AMotionEvent_getAction(e),masked=action&AMOTION_ACTION_MASK;size_t pc=AMotionEvent_getPointerCount(e);int64_t now=event_ms(e);
    G.sampleTimeNs=AMotionEvent_getEventTime(e);
    if(masked==AMOTION_ACTION_DOWN||masked==AMOTION_ACTION_POINTER_DOWN||masked==AMOTION_ACTION_CANCEL)fling_cancel();
    if(masked==AMOTION_ACTION_UP||masked==AMOTION_ACTION_CANCEL)start_anim_timer();
    if(masked==AMOTION_ACTION_DOWN||masked==AMOTION_ACTION_MOVE||masked==AMOTION_ACTION_UP||masked==AMOTION_ACTION_POINTER_DOWN||masked==AMOTION_ACTION_POINTER_UP)G.interactionMs=monotonic_ms();
    /* CANCEL is stream-wide, even when the cancelled pen is absent from the
       pointer array (or Android supplies no pointers at all). */
    if(masked==AMOTION_ACTION_CANCEL&&G.stylusDown){cancel_canvas_contact();render_event(now,1);return;}
    if(pc==0){if(masked==AMOTION_ACTION_CANCEL)cancel_canvas_contact();return;}
    if(G.toast&&G.toastMs>0&&now-G.toastMs>1300)G.toast=0;if(masked==AMOTION_ACTION_DOWN)ocr_set_interaction(1);else if(masked==AMOTION_ACTION_CANCEL||(masked==AMOTION_ACTION_UP&&!G.flingActive))ocr_set_interaction(0);
    int changed=(action>>8)&255,stylusIndex=-1;
    for(size_t i=0;i<pc;i++){int tt=AMotionEvent_getToolType(e,i);if((tt==TOOL_STYLUS||tt==TOOL_ERASER)&&(!G.stylusDown||AMotionEvent_getPointerId(e,i)==G.stylusPointerId)){stylusIndex=(int)i;break;}}
    if(G.stylusDown&&stylusIndex<0){cancel_canvas_contact();render_event(now,1);return;}
    int hasStylus=stylusIndex>=0;
    if(hasStylus&&(masked==AMOTION_ACTION_POINTER_DOWN||masked==AMOTION_ACTION_POINTER_UP)){
        if(changed!=stylusIndex)return;
        masked=masked==AMOTION_ACTION_POINTER_DOWN?AMOTION_ACTION_DOWN:AMOTION_ACTION_UP;
    }
    if(hasStylus){
        float sx=AMotionEvent_getX(e,(size_t)stylusIndex),sy=AMotionEvent_getY(e,(size_t)stylusIndex),p=AMotionEvent_getPressure(e,(size_t)stylusIndex);G.lastRawPressure=p;G.hoverX=sx;G.hoverY=sy;
        int buttons=AMotionEvent_getButtonState(e);int btnNow=(buttons&(BUTTON_STYLUS_PRIMARY|BUTTON_STYLUS_SECONDARY))?1:0;if(masked==AMOTION_ACTION_BUTTON_PRESS)btnNow=1;else if(masked==AMOTION_ACTION_BUTTON_RELEASE)btnNow=0;handle_button_state(btnNow,now);
        if(masked==AMOTION_ACTION_HOVER_ENTER||masked==AMOTION_ACTION_HOVER_MOVE||masked==AMOTION_ACTION_HOVER_EXIT){if(G.stylusDown){cancel_canvas_contact();render_event(now,1);}G.hoverActive=0;return;}
        if(masked==AMOTION_ACTION_BUTTON_PRESS||masked==AMOTION_ACTION_BUTTON_RELEASE)return;
        if(G.radialOpen)return;
        if(!G.stylusDown&&color_pointer_handle(masked,sx,sy,now))return;
        if(!G.stylusDown&&ui_pointer_route(masked,sx,sy,now))return;
        if(G.editorOpen||G.projectsPanel||G.settingsPanel||G.galleryOpen||G.calibrationOpen||G.addPanel||G.layersPanel||G.searchPanel||G.framePanel||G.presentationMode){render_event(now,masked==AMOTION_ACTION_UP||masked==AMOTION_ACTION_CANCEL);return;}
        if(masked==AMOTION_ACTION_DOWN){G.stylusPointerId=AMotionEvent_getPointerId(e,(size_t)stylusIndex);G.stylusToolType=AMotionEvent_getToolType(e,(size_t)stylusIndex);G.hoverActive=1;G.fingerCount=0;G.fingerId0=G.fingerId1=-1;G.fingerPanning=0;G.twoTapCandidate=G.threeTapCandidate=0;cancel_pen_hold_timer();}
        if(G.frameMoveMode){if(G.selectedFrame<0||G.selectedFrame>=G.frameN||G.lockFrames||G.frameLocked[G.selectedFrame]){G.frameMoveMode=0;return;}FrameObj*f=&G.frames[G.selectedFrame];float wx=(sx-G.offX)/G.scale,wy=(sy-G.offY)/G.scale;if(masked==AMOTION_ACTION_DOWN){if(!frame_contains_world(f,wx,wy)){G.frameMoveMode=0;G.framePanel=1;render_event(now,1);return;}G.frameMoveDragging=1;G.stylusDown=1;G.frameMoveTotalX=G.frameMoveTotalY=0;ocr_move_begin();G.frameMoveLastX=wx;G.frameMoveLastY=wy;return;}if(masked==AMOTION_ACTION_MOVE&&G.frameMoveDragging){float mdx=wx-G.frameMoveLastX,mdy=wy-G.frameMoveLastY;frame_move_selected_by(mdx,mdy);G.frameMoveTotalX+=mdx;G.frameMoveTotalY+=mdy;G.frameMoveLastX=wx;G.frameMoveLastY=wy;render_event(now,0);return;}if(masked==AMOTION_ACTION_UP||masked==AMOTION_ACTION_CANCEL){int cancelled=masked==AMOTION_ACTION_CANCEL;if(cancelled&&(G.frameMoveTotalX!=0||G.frameMoveTotalY!=0))frame_move_selected_by(-G.frameMoveTotalX,-G.frameMoveTotalY);G.frameMoveDragging=0;G.frameMoveMode=0;G.frameMoveTotalX=G.frameMoveTotalY=0;G.stylusDown=0;ocr_move_commit();if(!cancelled)save_all_document();G.framePanel=1;render_event(now,1);return;}return;}
        if(G.frameCreating){if(masked==AMOTION_ACTION_DOWN){G.stylusDown=1;G.frameDragging=1;float wx=(sx-G.offX)/G.scale,wy=(sy-G.offY)/G.scale;memset(&G.liveFrame,0,sizeof(G.liveFrame));G.liveFrame.x=wx;G.liveFrame.y=wy;return;}if(masked==AMOTION_ACTION_MOVE&&G.frameDragging){G.liveFrame.w=(sx-G.offX)/G.scale-G.liveFrame.x;G.liveFrame.h=(sy-G.offY)/G.scale-G.liveFrame.y;render_event(now,0);return;}if(masked==AMOTION_ACTION_UP||masked==AMOTION_ACTION_CANCEL){if(masked!=AMOTION_ACTION_CANCEL){G.liveFrame.w=(sx-G.offX)/G.scale-G.liveFrame.x;G.liveFrame.h=(sy-G.offY)/G.scale-G.liveFrame.y;frame_add_live();}G.frameDragging=0;G.frameCreating=0;G.stylusDown=0;G.framePanel=1;render_event(now,1);return;}return;}
        int tt=AMotionEvent_getToolType(e,(size_t)stylusIndex),eraseEverything=G.tool==MODE_ERASE_ALL&&tt==TOOL_STYLUS&&!G.buttonQuickErase,tempErase=G.tool==MODE_ERASE||eraseEverything||tt==TOOL_ERASER||G.buttonQuickErase;
        if(masked==AMOTION_ACTION_DOWN){
            if(tempErase&&!ensure_actions(G.actionN+1)){set_toast(8,now);render_event(now,1);return;}
            if((G.tool==MODE_PEN&&G.lockInk)||(G.tool==MODE_HIGHLIGHTER&&G.lockMarker)||(G.tool==MODE_MEASURE&&G.lockCAD)||(G.tool==MODE_SHAPE&&G.lockShapes)){set_toast(9,now);render_event(now,1);return;}
            if(G.tool==MODE_SELECT){int fc=frame_corner_hit(sx,sy);if(fc>=0){frame_resize_begin(fc);G.stylusDown=1;render_event(now,1);return;}}
            if(G.tool==MODE_MEASURE&&!tempErase){int mi=measure_hit(sx,sy);if(mi>=0){G.selectedMeasure=mi;render_event(now,1);return;}}
            G.stylusDown=1;G.strokeStartMs=G.strokeLastMoveMs=now;if(G.tool==MODE_SELECT)ocr_move_begin();
            if(G.tool==MODE_SELECT){ObjRef r=object_hit(sx,sy);if(r.type==SEL_IMAGE)r=(ObjRef){SEL_NONE,-1};if(r.type!=SEL_NONE){select_hit_ref(r);G.selectionDragging=1;G.selectionDragTotalX=G.selectionDragTotalY=0;G.selectionLastWX=(sx-G.offX)/G.scale;G.selectionLastWY=(sy-G.offY)/G.scale;}else{selection_clear();G.lassoActive=1;G.lassoN=1;G.lassoPts[0]=(Point){(sx-G.offX)/G.scale,(sy-G.offY)/G.scale,1};}}
            else if(G.tool==MODE_SHAPE&&!tempErase){G.shapeDrawing=1;memset(&G.liveShape,0,sizeof(G.liveShape));G.liveShape.type=G.shapeType;G.liveShape.x0=G.liveShape.x1=(sx-G.offX)/G.scale;G.liveShape.y0=G.liveShape.y1=(sy-G.offY)/G.scale;G.liveShape.color=G.color;G.liveShape.width=G.brush;}
            else if(G.tool==MODE_MEASURE&&!tempErase)start_measure(sx,sy);else if(tempErase){G.erasing=1;G.eraseEverythingActive=eraseEverything;memset(&G.eraseAction,0,sizeof(G.eraseAction));G.eraseAction.type=eraseEverything?ACT_ERASE_ALL:ACT_ERASE;if(eraseEverything)erase_everything_at(sx,sy);else erase_at(sx,sy);}else start_stroke(sx,sy,p);
            if(G.tool==MODE_PEN||G.tool==MODE_HIGHLIGHTER||tempErase){G.zenMode=1;G.railOpen=0;cancel_rail_timer();start_anim_timer();}
            render_event(now,1);
        }else if(masked==AMOTION_ACTION_MOVE&&G.stylusDown){
            G.strokeLastMoveMs=now;
            if(G.frameResizeDragging){frame_resize_to(sx,sy);render_event(now,0);return;}
            if(G.tool==MODE_SELECT){float wx=(sx-G.offX)/G.scale,wy=(sy-G.offY)/G.scale;if(G.selectionDragging){selection_move_by(wx-G.selectionLastWX,wy-G.selectionLastWY);G.selectionLastWX=wx;G.selectionLastWY=wy;}else if(G.lassoActive&&G.lassoN<256){Point q=G.lassoPts[G.lassoN-1];float dd=sqrtf(sq((wx-q.x)*G.scale)+sq((wy-q.y)*G.scale));if(dd>4.0f)G.lassoPts[G.lassoN++]=(Point){wx,wy,1};}}
            else if(G.tool==MODE_SHAPE&&G.shapeDrawing){G.liveShape.x1=(sx-G.offX)/G.scale;G.liveShape.y1=(sy-G.offY)/G.scale;}
            else{size_t hist=AMotionEvent_getHistorySize(e);for(size_t j=0;j<hist;j++){G.sampleTimeNs=AMotionEvent_getHistoricalEventTime(e,j);float hx=AMotionEvent_getHistoricalX(e,(size_t)stylusIndex,j),hy=AMotionEvent_getHistoricalY(e,(size_t)stylusIndex,j),hp=AMotionEvent_getHistoricalPressure(e,(size_t)stylusIndex,j);if(G.measuring)update_measure(hx,hy);else if(G.erasing){if(G.eraseEverythingActive)erase_everything_at(hx,hy);else erase_at(hx,hy);}else add_stroke_point(hx,hy,hp);}G.sampleTimeNs=AMotionEvent_getEventTime(e);if(G.measuring)update_measure(sx,sy);else if(G.erasing){if(G.eraseEverythingActive)erase_everything_at(sx,sy);else erase_at(sx,sy);}else add_stroke_point(sx,sy,p);}
            render_event(now,0);
        }else if((masked==AMOTION_ACTION_UP||masked==AMOTION_ACTION_CANCEL)&&G.stylusDown){
            int cancelled=masked==AMOTION_ACTION_CANCEL,wasErasing=G.erasing,wasEverything=G.eraseEverythingActive;
            if(wasErasing){if(!cancelled){if(wasEverything)erase_everything_at(sx,sy);else erase_at(sx,sy);}G.stylusDown=0;G.hoverActive=0;render_event(now,1);}
            if(G.frameResizeDragging){if(cancelled&&G.selectedFrame>=0&&G.selectedFrame<G.frameN)G.frames[G.selectedFrame]=G.frameGestureStart;G.frameResizeDragging=0;G.stylusDown=0;if(!cancelled)save_frames();G.sceneRevision++;G.minimapDirty=1;render_event(now,1);return;}
            if(G.tool==MODE_SELECT){if(cancelled&&G.selectionDragging)selection_drag_restore();if(!cancelled&&G.lassoActive)lasso_commit();G.lassoActive=0;G.lassoN=0;G.selectionDragging=0;G.selectionDragTotalX=G.selectionDragTotalY=0;G.snapGuideX=G.snapGuideY=0;ocr_move_commit();}
            else if(G.tool==MODE_SHAPE&&G.shapeDrawing){if(!cancelled){G.liveShape.x1=(sx-G.offX)/G.scale;G.liveShape.y1=(sy-G.offY)/G.scale;commit_live_shape();}else G.shapeDrawing=0;}
            else if(G.measuring){if(cancelled){G.measuring=0;memset(&G.liveMeasure,0,sizeof(G.liveMeasure));}else end_measure();}
            else if(G.erasing){if(cancelled){for(int i=0;i<G.eraseAction.n;i++)if(G.eraseAction.ids[i]>=0&&G.eraseAction.ids[i]<G.strokeN)G.strokes[G.eraseAction.ids[i]].active=1;action_set_refs_active(&G.eraseAction,1);free_action(&G.eraseAction);}else if(G.eraseAction.n||G.eraseAction.refN){ocr_set_action_active(&G.eraseAction,0);push_action(&G.eraseAction);}else free_action(&G.eraseAction);G.erasing=G.eraseEverythingActive=0;}
            else if(cancelled)cancel_current_stroke();else{if(G.currentStroke>=0){size_t hist=AMotionEvent_getHistorySize(e);for(size_t j=0;j<hist;j++){G.sampleTimeNs=AMotionEvent_getHistoricalEventTime(e,j);add_stroke_point(AMotionEvent_getHistoricalX(e,stylusIndex,j),AMotionEvent_getHistoricalY(e,stylusIndex,j),AMotionEvent_getHistoricalPressure(e,stylusIndex,j));}G.sampleTimeNs=AMotionEvent_getEventTime(e);add_stroke_point(sx,sy,p);}if(!clean_current_stroke_to_shape(now))end_stroke();}
            G.stylusDown=0;
            if(G.tool==MODE_SELECT&&!cancelled)save_all_document();
            else if(wasErasing){schedule_ink_save();if(wasEverything){save_meta();save_frames();G.workspaceSavePending=1;G.imageSavePending=1;start_anim_timer();}}
            else if(G.tool==MODE_MEASURE)save_meta();
            else if(G.tool==MODE_SHAPE)save_workspace();
            else schedule_ink_save();
            render_event(now,1);
        }
        return;
    }
    if(G.stylusDown)return;
    if(AMotionEvent_getToolType(e,0)!=TOOL_FINGER)return;
    float x=AMotionEvent_getX(e,0),y=AMotionEvent_getY(e,0);
    if(G.radialOpen){
        if(masked==AMOTION_ACTION_MOVE||masked==AMOTION_ACTION_DOWN){radial_update_hot(x,y);render_event(now,0);return;}
        if(masked==AMOTION_ACTION_POINTER_DOWN){G.radialOpen=0;G.radialSizing=G.radialSizeActive=0;G.radialHot=-1;start_anim_timer();return;}
        if(masked==AMOTION_ACTION_UP||masked==AMOTION_ACTION_CANCEL){int hot=G.radialHot,sized=G.radialSizing,sizeChanged=G.radialSizeActive;if(masked==AMOTION_ACTION_CANCEL&&sizeChanged){G.brush=G.radialSizeStart;G.metaDirty=1;}G.radialOpen=0;G.radialSizing=0;G.radialSizeActive=0;G.radialHot=-1;G.fingerCount=0;start_anim_timer();if(masked!=AMOTION_ACTION_CANCEL&&hot>=0&&!sized)radial_execute(hot);save_meta();render_event(now,1);return;}
        return;
    }
    if(color_pointer_handle(masked,x,y,now)){G.fingerCount=0;return;}
    if(ui_pointer_route(masked,x,y,now)){G.fingerCount=0;return;}
    if(masked==AMOTION_ACTION_DOWN){
        fling_track_reset(now);
        G.fingerId0=AMotionEvent_getPointerId(e,0);G.fingerId1=-1;
        G.gestureStartOffX=G.offX;G.gestureStartOffY=G.offY;G.gestureStartScale=G.scale;G.selectionDragTotalX=G.selectionDragTotalY=0;G.transformSnapshotN=0;G.imageCancelValid=0;
        if(G.editorOpen||G.projectsPanel||G.settingsPanel||G.galleryOpen||G.calibrationOpen||G.addPanel||G.layersPanel||G.searchPanel||G.framePanel||G.presentationMode){G.fingerCount=0;return;}
        ocr_move_begin();
        if(G.selectionMoreOpen){G.selectionMoreOpen=0;render_event(now,1);}
        ObjRef pressedObject=object_hit(x,y);G.fingerRevealChromeCandidate=G.zenMode&&pressedObject.type==SEL_NONE;
        /* Focus hides the minimap and object chrome, so finger navigation must
           win before any hidden hit target can consume the gesture. */
        if(G.zenMode||G.tool==MODE_ERASE||G.tool==MODE_ERASE_ALL){G.fingerCount=1;G.fingerPanning=G.zenMode?0:1;G.fingerMoved=0;G.fingerDownX=x;G.fingerDownY=y;G.lastFingerX=x;G.lastFingerY=y;G.fingerDownRef=(ObjRef){SEL_NONE,-1};G.selectionDragging=0;cancel_pen_hold_timer();render_event(now,1);return;}
        if(minimap_navigate(x,y)){G.fingerCount=1;G.fingerPanning=1;G.fingerMoved=1;G.lastFingerX=x;G.lastFingerY=y;G.fingerDownRef=(ObjRef){SEL_NONE,-1};cancel_pen_hold_timer();render_event(now,1);return;}
        G.fingerCount=1;G.fingerPanning=0;G.fingerMoved=0;G.fingerDownX=x;G.fingerDownY=y;G.lastFingerX=x;G.lastFingerY=y;G.fingerDownRef=(ObjRef){SEL_NONE,-1};G.selectionDragging=0;
        int corner=frame_corner_hit(x,y);if(corner>=0){frame_resize_begin(corner);cancel_pen_hold_timer();render_event(now,1);return;}
        int img=G.visPhotos?image_hit(x,y):-1;
        if(img>=0){
            if(G.photoMode&&G.selectedImage==img&&!G.lockPhotos){G.imageCancelStart=G.images[img];G.imageCancelValid=1;G.imageDragging=1;G.imageStartX=G.images[img].x;G.imageStartY=G.images[img].y;G.lastFingerX=x;G.lastFingerY=y;return;}
            G.penHoldX=x;G.penHoldY=y;arm_touch_hold_timer(now,2,img,0.45f);render_event(now,1);return;
        }
        ObjRef r=finger_object_hit(x,y);G.fingerDownRef=r;
        if(r.type!=SEL_NONE&&ref_is_selected(r.type,r.index)&&r.type!=SEL_STROKES&&r.type!=SEL_FRAME){selection_snapshot_begin();G.selectionDragging=1;G.selectionDragTotalX=G.selectionDragTotalY=0;G.selectionLastWX=(x-G.offX)/G.scale;G.selectionLastWY=(y-G.offY)/G.scale;}
        else if(r.type==SEL_NONE){G.penHoldX=x;G.penHoldY=y;arm_touch_hold_timer(now,1,-1,G.radialHoldSec);}
        G.twoTapCandidate=0;render_event(now,1);
    }else if(masked==AMOTION_ACTION_POINTER_DOWN&&pc>=2){
        cancel_pen_hold_timer();ocr_move_commit();G.snapGuideX=G.snapGuideY=0;G.fingerRevealChromeCandidate=0;
        if(G.editorOpen||G.projectsPanel||G.settingsPanel||G.galleryOpen||G.calibrationOpen||G.addPanel||G.layersPanel||G.searchPanel||G.framePanel||G.presentationMode)return;
        if(G.fingerId0<0)G.fingerId0=AMotionEvent_getPointerId(e,0);G.fingerId1=-1;for(size_t i=0;i<pc;i++){int id=AMotionEvent_getPointerId(e,i);if(id!=G.fingerId0){G.fingerId1=id;break;}}int pi0=0,pi1=1;if(!finger_pair_indices(e,pc,&pi0,&pi1))return;float x0=AMotionEvent_getX(e,(size_t)pi0),y0=AMotionEvent_getY(e,(size_t)pi0),x1=AMotionEvent_getX(e,(size_t)pi1),y1=AMotionEvent_getY(e,(size_t)pi1);float mx=(x0+x1)*.5f,my=(y0+y1)*.5f,dx=x1-x0,dy=y1-y0;
        if(G.tool!=MODE_ERASE&&G.tool!=MODE_ERASE_ALL&&G.photoMode&&photo_begin_gesture(x0,y0,x1,y1)){G.fingerCount=2;G.twoTapCandidate=0;return;}
        if(G.tool!=MODE_ERASE&&G.tool!=MODE_ERASE_ALL&&G.selectedRefN>0&&selection_gesture_contains(x0,y0,x1,y1)){if(G.transformSnapshotN<=0)selection_snapshot_begin();G.selectionDragging=0;G.selectionGesture=1;G.selectionGestureDist=fmax2(4.0f,sqrtf(dx*dx+dy*dy));G.selectionGestureAngle=atan2f(dy,dx);G.selectionGestureMidWX=(mx-G.offX)/G.scale;G.selectionGestureMidWY=(my-G.offY)/G.scale;G.fingerCount=2;G.twoTapCandidate=0;return;}
        int cleanTwoTap=pc==2&&!G.fingerMoved&&!G.fingerPanning&&!G.frameResizeDragging&&!G.selectionDragging&&!G.imageDragging;
        G.lastMidX=mx;G.lastMidY=my;G.lastPinchDist=sqrtf(dx*dx+dy*dy);fling_track_reset(now);G.fingerCount=(int)pc;G.twoTapCandidate=cleanTwoTap;G.twoTapStartMs=now;G.twoTapStartMidX=mx;G.twoTapStartMidY=my;G.twoTapStartDist=G.lastPinchDist;G.twoTapMaxMove=0;if(pc>=3){G.threeTapCandidate=1;G.threeTapStartMs=now;G.threeTapStartX=mx;G.threeTapStartY=my;G.threeTapMaxMove=0;G.twoTapCandidate=0;}
    }else if(masked==AMOTION_ACTION_MOVE){
        if(pc>=2){int pi0=0,pi1=1;if(!finger_pair_indices(e,pc,&pi0,&pi1))return;float x0=AMotionEvent_getX(e,(size_t)pi0),y0=AMotionEvent_getY(e,(size_t)pi0),x1=AMotionEvent_getX(e,(size_t)pi1),y1=AMotionEvent_getY(e,(size_t)pi1);float mx=(x0+x1)*.5f,my=(y0+y1)*.5f,dx=x1-x0,dy=y1-y0,d=sqrtf(dx*dx+dy*dy);if(G.imageGesture){photo_update_gesture(x0,y0,x1,y1);render_event(now,0);return;}if(G.selectionGesture){float a=atan2f(dy,dx),sc=d/fmax2(4.0f,G.selectionGestureDist),nwx=(mx-G.offX)/G.scale,nwy=(my-G.offY)/G.scale;selection_transform_by(G.selectionGestureMidWX,G.selectionGestureMidWY,nwx,nwy,sc,a-G.selectionGestureAngle);G.selectionGestureDist=d;G.selectionGestureAngle=a;G.selectionGestureMidWX=nwx;G.selectionGestureMidWY=nwy;render_event(now,0);return;}float move=sqrtf(sq(mx-G.twoTapStartMidX)+sq(my-G.twoTapStartMidY));if(move>G.twoTapMaxMove)G.twoTapMaxMove=move;if(move>22.0f||fabsf(d-G.twoTapStartDist)>24.0f)G.twoTapCandidate=0;if(G.threeTapCandidate&&move>24.0f)G.threeTapCandidate=0;float panX=mx-G.lastMidX,panY=my-G.lastMidY;G.fingerPanning=1;G.offX+=panX;G.offY+=panY;fling_track_add(panX,panY,now);if(G.lastPinchDist>4&&d>4)zoom_about(mx,my,d/G.lastPinchDist);else minimap_camera_changed();G.lastMidX=mx;G.lastMidY=my;G.lastPinchDist=d;render_event(now,0);}
        else if(pc==1&&G.fingerCount==1){float md=sqrtf(sq(x-G.fingerDownX)+sq(y-G.fingerDownY));if(md>G.penHoldMaxMove)G.penHoldMaxMove=md;if(G.penHoldArmed&&md>pen_hold_buffer_px()){G.penHoldMoved=1;cancel_pen_hold_timer();}
            if(G.frameResizeDragging){frame_resize_to(x,y);G.fingerMoved=1;render_event(now,0);return;}
            if(G.imageDragging&&G.photoMode&&G.selectedImage>=0){photo_drag_to(x,y);G.fingerMoved=1;render_event(now,0);return;}
            if(G.selectionDragging){float wx=(x-G.offX)/G.scale,wy=(y-G.offY)/G.scale;selection_move_by(wx-G.selectionLastWX,wy-G.selectionLastWY);G.selectionLastWX=wx;G.selectionLastWY=wy;G.fingerMoved=1;render_event(now,0);return;}
            int changed=0;size_t hist=AMotionEvent_getHistorySize(e);for(size_t j=0;j<hist;j++)changed|=finger_pan_sample(AMotionEvent_getHistoricalX(e,0,j),AMotionEvent_getHistoricalY(e,0,j),AMotionEvent_getHistoricalEventTime(e,j)/1000000LL);changed|=finger_pan_sample(x,y,now);if(changed){render_event(now,0);return;}
        }
    }else if(masked==AMOTION_ACTION_POINTER_UP){
        cancel_pen_hold_timer();if(G.selectionGesture){G.selectionGesture=0;G.transformSnapshotN=0;G.fingerCount=0;G.fingerId0=G.fingerId1=-1;G.fingerMoved=1;G.fingerDownRef=(ObjRef){SEL_NONE,-1};save_all_document();render_event(now,1);return;}if(G.imageGesture){G.imageGesture=0;G.imageDragging=0;G.imageCancelValid=0;G.fingerCount=0;G.fingerId0=G.fingerId1=-1;G.fingerMoved=1;G.fingerDownRef=(ObjRef){SEL_NONE,-1};save_images();render_event(now,1);return;}int historyChanged=0;if(pc>=3&&G.threeTapCandidate&&now-G.threeTapStartMs<=320){redo_action();G.threeTapCandidate=0;historyChanged=1;render_event(now,1);}else if(pc==2&&G.twoTapCandidate&&now-G.twoTapStartMs<=280){register_two_finger_tap(G.twoTapStartMidX,G.twoTapStartMidY,now);historyChanged=1;render_event(now,1);}if(historyChanged)save_all_document();if(G.fingerPanning&&!historyChanged)fling_release(now);G.fingerPanning=0;G.twoTapCandidate=G.threeTapCandidate=0;G.fingerCount=0;G.fingerId0=G.fingerId1=-1;G.fingerMoved=1;G.fingerDownRef=(ObjRef){SEL_NONE,-1};minimap_camera_changed();
    }else if(masked==AMOTION_ACTION_UP||masked==AMOTION_ACTION_CANCEL){
        int cancelled=masked==AMOTION_ACTION_CANCEL,wasPan=G.fingerPanning,wasMove=G.fingerMoved,wasImageDrag=G.imageDragging,wasImageGesture=G.imageGesture,wasResize=G.frameResizeDragging,wasSelectionDrag=G.selectionDragging,wasSelectionGesture=G.selectionGesture,wasReveal=G.fingerRevealChromeCandidate;ObjRef down=G.fingerDownRef;cancel_pen_hold_timer();
        if(wasResize){if(cancelled&&G.selectedFrame>=0&&G.selectedFrame<G.frameN)G.frames[G.selectedFrame]=G.frameGestureStart;G.frameResizeDragging=0;if(!cancelled)save_frames();}
        if((wasImageDrag||wasImageGesture)&&cancelled&&G.imageCancelValid&&G.selectedImage>=0&&G.selectedImage<G.imageN){G.images[G.selectedImage]=G.imageCancelStart;G.imagesDirty=1;G.minimapDirty=1;}if(wasImageDrag){G.imageDragging=0;if(!cancelled)save_images();}
        if(cancelled&&wasSelectionGesture)selection_snapshot_restore();else G.transformSnapshotN=0;
        if(cancelled&&wasSelectionDrag)selection_drag_restore();
        if(cancelled&&wasPan){G.offX=G.gestureStartOffX;G.offY=G.gestureStartOffY;G.scale=G.gestureStartScale;G.minimapDirty=1;}
        if(!cancelled&&!wasPan&&!wasMove&&!wasImageDrag&&!wasResize){if(wasReveal&&object_hit(x,y).type==SEL_NONE)reveal_canvas_chrome();if(down.type!=SEL_NONE)select_hit_ref(down);else if(image_hit(x,y)<0)selection_clear();}
        if(cancelled)fling_cancel();else if(wasPan)fling_release(now);ocr_move_commit();G.fingerCount=0;G.fingerId0=G.fingerId1=-1;G.fingerPanning=G.fingerMoved=G.fingerRevealChromeCandidate=0;G.twoTapCandidate=G.threeTapCandidate=0;G.selectionDragging=0;G.selectionDragTotalX=G.selectionDragTotalY=0;G.selectionGesture=0;G.transformSnapshotN=0;G.snapGuideX=G.snapGuideY=0;G.imageGesture=0;G.imageCancelValid=0;G.fingerDownRef=(ObjRef){SEL_NONE,-1};minimap_camera_changed();if(!cancelled&&wasSelectionDrag&&wasMove)save_all_document();else if(!cancelled&&wasPan){G.workspaceSavePending=1;start_anim_timer();}render_event(now,1);
    }
}


static int input_cb(int fd,int events,void*data){(void)fd;(void)events;(void)data;if(!G.input)return 1;AInputEvent*e=0;int batch=0;INPUT_BATCH_ACTIVE=1;while(AInputQueue_getEvent(G.input,&e)>=0){if(AInputQueue_preDispatchEvent(G.input,e))continue;int handled=0;if(AInputEvent_getType(e)==AINPUT_EVENT_TYPE_MOTION){double begin=perf_us(),age=begin-(double)AMotionEvent_getEventTime(e)/1000;if(age>IP.maxAgeUs)IP.maxAgeUs=age;int action=AMotionEvent_getAction(e)&255;if(action==AMOTION_ACTION_DOWN||action==AMOTION_ACTION_MOVE||action==AMOTION_ACTION_UP){size_t count=AMotionEvent_getPointerCount(e);for(size_t j=0;j<count;j++)if(AMotionEvent_getToolType(e,j)==TOOL_STYLUS||AMotionEvent_getToolType(e,j)==TOOL_ERASER)IP.available+=AMotionEvent_getHistorySize(e)+1;}handle_motion(e);IP.inputUs+=perf_us()-begin;IP.events++;batch++;handled=1;}AInputQueue_finishEvent(G.input,e,handled);}if(batch>IP.maxBatch)IP.maxBatch=batch;INPUT_BATCH_ACTIVE=0;if(INPUT_FRAME_PENDING){INPUT_FRAME_PENDING=0;request_input_frame();}return 1;}
static void cb_resume(ANativeActivity*a){(void)a;if(G.editorOpen&&G.systemImeActive)start_editor_poll_timer();if(G.mediaPermissionPending){G.mediaPermissionPending=0;if(photo_permission_granted()){if(query_media()>0){G.galleryPage=0;G.galleryOpen=1;load_gallery_page();G.toast=0;}else set_toast(5,0);}else set_toast(4,0);render();}}
static void cb_window_created(ANativeActivity*a,ANativeWindow*w){(void)a;G.window=w;AUI_PANEL_HASH=AUI_CHROME_HASH=AUI_COLOR_HASH=0;if(G.editorOpen)system_text_start(G.editorBuf,G.editorMode);else if(G.calibrationOpen)system_text_start(G.calibText,6);if(G.editorOpen&&G.systemImeActive)start_editor_poll_timer();else if(G.ocrInitialPending||G.uiAnimating||G.toast)start_anim_timer();render();}
static void cb_window_resized(ANativeActivity*a,ANativeWindow*w){(void)a;G.window=w;G.minimapDirty=1;render();}
static void cb_window_redraw(ANativeActivity*a,ANativeWindow*w){(void)a;G.window=w;render();}
static void cb_window_destroyed(ANativeActivity*a,ANativeWindow*w){(void)a;(void)w;cancel_anim_timer();photo_import_reset(1);save_all_document();save_project_prefs();nui_destroy();aui_void("destroy");G.systemImeActive=0;AUI_PANEL_HASH=AUI_CHROME_HASH=0;
#ifdef VAST_GPU
    gpu_destroy();
#endif
    G.window=0;}
static void cb_input_created(ANativeActivity*a,AInputQueue*q){(void)a;G.input=q;ALooper*l=ALooper_forThread();if(!l)l=ALooper_prepare(1);AInputQueue_attachLooper(q,l,1,input_cb,0);if(G.buttonTimerFd<0){G.buttonTimerFd=timerfd_create(CLOCK_MONOTONIC,0);if(G.buttonTimerFd>=0)ALooper_addFd(l,G.buttonTimerFd,2,ALOOPER_EVENT_INPUT,button_timer_cb,0);}if(G.penHoldTimerFd<0){G.penHoldTimerFd=timerfd_create(CLOCK_MONOTONIC,0);if(G.penHoldTimerFd>=0)ALooper_addFd(l,G.penHoldTimerFd,3,ALOOPER_EVENT_INPUT,pen_hold_timer_cb,0);}if(G.railTimerFd<0){G.railTimerFd=timerfd_create(CLOCK_MONOTONIC,0);if(G.railTimerFd>=0)ALooper_addFd(l,G.railTimerFd,4,ALOOPER_EVENT_INPUT,rail_timer_cb,0);}if(G.animTimerFd<0){G.animTimerFd=timerfd_create(CLOCK_MONOTONIC,0);if(G.animTimerFd>=0)ALooper_addFd(l,G.animTimerFd,5,ALOOPER_EVENT_INPUT,anim_timer_cb,0);}
#ifdef VAST_OCR
    if(G.ocr&&!G.ocrEventRegistered){int efd=ocr_manager_event_fd(G.ocr);if(efd>=0&&ALooper_addFd(l,efd,6,ALOOPER_EVENT_INPUT,ocr_event_cb,0)>=0)G.ocrEventRegistered=1;}
#endif
    if(G.ocrInitialPending)start_anim_timer();if(G.railOpen)arm_rail_timer();}
static void cb_input_destroyed(ANativeActivity*a,AInputQueue*q){(void)a;cancel_canvas_contact();if(q)AInputQueue_detachLooper(q);G.input=0;ocr_set_interaction(0);}
static void cb_pause(ANativeActivity*a){(void)a;cancel_canvas_contact();ocr_set_interaction(0);if(G.editorOpen&&G.systemImeActive){system_text_read(G.editorBuf,NOTE_TEXT_CAP);G.editorLen=str_len_local(G.editorBuf);editor_live_apply();}cancel_anim_timer();photo_import_reset(1);if(G.pickerSessionTouched||G.pickerApplySelection)picker_commit_session();save_all_document();save_project_prefs();}
static void cb_destroy(ANativeActivity*a){(void)a;photo_import_reset(1);if(G.pickerSessionTouched||G.pickerApplySelection)picker_commit_session();save_all_document();save_project_prefs();ocr_save_prefs();ocr_shutdown();
#ifdef VAST_GPU
    gpu_destroy();
#endif
erase_index_reset();free(ER_ids);free(ER_seen);ER_ids=ER_seen=0;ER_cap=ER_n=ER_epoch=ER_active=0;nui_destroy();AUI_PANEL_HASH=AUI_CHROME_HASH=0;note_text_cache_clear();aui_void("destroy");if(AUI){(*G.activity->env)->DeleteGlobalRef(G.activity->env,AUI);AUI=0;}system_text_stop();cancel_button_timer();cancel_pen_hold_timer();cancel_rail_timer();cancel_anim_timer();if(G.buttonTimerFd>=0)close(G.buttonTimerFd);if(G.penHoldTimerFd>=0)close(G.penHoldTimerFd);if(G.railTimerFd>=0)close(G.railTimerFd);if(G.animTimerFd>=0)close(G.animTimerFd);free_thumbs();for(int i=0;i<G.imageN;i++)if(G.images[i].px)free(G.images[i].px);for(int i=0;i<G.strokeN;i++)if(G.strokes[i].pts)free(G.strokes[i].pts);for(int i=0;i<G.actionN;i++)free_action(&G.actions[i]);free_action(&G.eraseAction);if(G.strokes)free(G.strokes);if(G.actions)free(G.actions);if(FONT_ATLAS){free(FONT_ATLAS);FONT_ATLAS=0;}if(STROKE_MASK){free(STROKE_MASK);STROKE_MASK=0;STROKE_MASK_W=STROKE_MASK_H=0;}if(G.minimapCache){free(G.minimapCache);G.minimapCache=0;}}
__attribute__((visibility("default"))) void ANativeActivity_onCreate(ANativeActivity*activity,void*savedState,size_t savedStateSize){
    (void)savedState;(void)savedStateSize;memset(&G,0,sizeof(G));G.activity=activity;G.currentStroke=-1;G.fingerId0=G.fingerId1=-1;G.tool=MODE_PEN;G.color=0xf4f6f8;G.brush=5.5f;G.pressureMin=0.18f;G.pressureMax=1.0f;G.pressureCurve=1;G.snap=1;G.scale=1.0f;G.uiScale=1.0f;G.themeRole=4;theme_preset(0);G.buttonPressAction=BA_UNDO;G.buttonHoldAction=BA_QUICK_ERASE;G.buttonDoubleAction=BA_REDO;G.buttonBindingsEnabled=0;G.buttonTimerFd=-1;G.penHoldTimerFd=-1;G.railTimerFd=-1;G.animTimerFd=-1;G.railOpen=1;G.railAnim=1.0f;G.radialHoldSec=1.0f;G.galleryLoadedPage=-1;G.pressureSmoothing=0.42f;G.strokeSmoothing=0.54f;G.highlighterOpacity=0.26f;G.radialHot=-1;G.selectedMeasure=-1;G.selectedImage=-1;G.cadScale=1.0f;G.cadUnit=2;G.minimap=1;G.minimapAnim=1.0f;G.minimapDirty=1;G.atmosphere=0;G.gridStyle=3;G.gridDepth=0;G.edgeGlass=0;G.planeElevation=0;G.regionGlow=0;G.motionStyle=0;G.motionIntensity=0.0f;G.farZoomMode=0;G.performanceMode=2;G.selectedFrame=-1;G.selectedNote=G.selectedShape=G.selectedBookmark=G.selectedGroup=-1;G.nextFrameId=G.nextNoteId=G.nextShapeId=G.nextBookmarkId=G.nextGroupId=1;G.visInk=G.visMarker=G.visPhotos=G.visCAD=G.visFrames=G.visNotes=G.visShapes=1;G.photoAngleSnap=1;G.sceneRevision=1;
    G.ocrEnabled=1;if(activity->internalDataPath){snprintf(G.projectPrefsPath,sizeof(G.projectPrefsPath),"%s/projects.v3",activity->internalDataPath);snprintf(G.inkPrefsPath,sizeof(G.inkPrefsPath),"%s/inkprefs.v1",activity->internalDataPath);snprintf(G.ocrPrefsPath,sizeof(G.ocrPrefsPath),"%s/ocrprefs.v1",activity->internalDataPath);load_ink_prefs();ocr_load_prefs();load_project_prefs();set_project_paths(G.projectIndex);}
    activity->callbacks->onResume=cb_resume;activity->callbacks->onPause=cb_pause;activity->callbacks->onDestroy=cb_destroy;activity->callbacks->onNativeWindowCreated=cb_window_created;activity->callbacks->onNativeWindowResized=cb_window_resized;activity->callbacks->onNativeWindowRedrawNeeded=cb_window_redraw;activity->callbacks->onNativeWindowDestroyed=cb_window_destroyed;activity->callbacks->onInputQueueCreated=cb_input_created;activity->callbacks->onInputQueueDestroyed=cb_input_destroyed;
    ANativeActivity_setWindowFlags(activity,FLAG_KEEP_SCREEN_ON|FLAG_FULLSCREEN,0);load_canvas();load_meta();load_images();load_frames();load_workspace();ocr_initialize();
}
