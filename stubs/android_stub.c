typedef unsigned long size_t;
typedef signed int int32_t;
typedef signed char int8_t;
typedef signed long long int64_t;
typedef unsigned int uint32_t;
typedef void A; typedef void B; typedef void C;
typedef struct {int32_t a,b,c,d;} R; typedef struct{int32_t w,h,s,f;void*bits;uint32_t r[6];} Buf;
typedef void AAssetManager; typedef void AAsset;
int32_t ANativeWindow_lock(A*a,Buf*b,R*r){return 0;} int32_t ANativeWindow_unlockAndPost(A*a){return 0;} int32_t ANativeWindow_setFrameRate(A*a,float f,int8_t c){(void)a;(void)f;(void)c;return 0;}
void ANativeActivity_setWindowFlags(A*a,uint32_t b,uint32_t c){} C* ALooper_forThread(void){return 0;} C* ALooper_prepare(int x){return 0;} int ALooper_addFd(C*l,int fd,int ident,int events,int(*cb)(int,int,void*),void*d){return 1;}
void AInputQueue_attachLooper(A*a,C*b,int c,int(*d)(int,int,void*),void*e){} void AInputQueue_detachLooper(A*a){}
int32_t AInputQueue_getEvent(A*a,B**b){return -1;} int32_t AInputQueue_preDispatchEvent(A*a,B*b){return 0;} void AInputQueue_finishEvent(A*a,B*b,int c){}
int32_t AInputEvent_getType(const B*b){return 0;} int32_t AMotionEvent_getAction(const B*b){return 0;} size_t AMotionEvent_getPointerCount(const B*b){return 0;}
float AMotionEvent_getX(const B*b,size_t i){return 0;} float AMotionEvent_getY(const B*b,size_t i){return 0;} float AMotionEvent_getPressure(const B*b,size_t i){return 0;}
int32_t AMotionEvent_getPointerId(const B*b,size_t i){return (int32_t)i;}
int32_t AMotionEvent_getToolType(const B*b,size_t i){return 0;} size_t AMotionEvent_getHistorySize(const B*b){return 0;}
float AMotionEvent_getHistoricalX(const B*b,size_t i,size_t j){return 0;} float AMotionEvent_getHistoricalY(const B*b,size_t i,size_t j){return 0;} float AMotionEvent_getHistoricalPressure(const B*b,size_t i,size_t j){return 0;}
int32_t AMotionEvent_getButtonState(const B*b){return 0;}

int64_t AMotionEvent_getEventTime(const B*b){return 0;}
int64_t AMotionEvent_getHistoricalEventTime(const B*b,size_t j){return 0;}
void*AChoreographer_getInstance(void){return 0;}void AChoreographer_postFrameCallback(void*c,void(*fn)(long,void*),void*d){}
int32_t ANativeWindow_getWidth(A*a){return 2560;} int32_t ANativeWindow_getHeight(A*a){return 1600;}
 int ANativeWindow_setBuffersGeometry(void*w,int a,int b,int f){return 0;}
AAsset* AAssetManager_open(AAssetManager*m,const char*name,int mode){return 0;}
long AAsset_getLength(AAsset*a){return 0;}
int AAsset_read(AAsset*a,void*buffer,size_t count){return 0;}
void AAsset_close(AAsset*a){}
