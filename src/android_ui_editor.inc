/* Thin JNI adapter: Android owns editing, C owns document state. */
static jclass AUI;
static int android_ui_available(void){return AUI!=0;}
static uint64_t AUI_PANEL_HASH;
static uint64_t AUI_CHROME_HASH;
static int AUI_CALIB_UNIT;
static void JNICALL aui_text(JNIEnv*,jclass,jstring);
static void JNICALL aui_action(JNIEnv*,jclass,jint);
static void JNICALL aui_cancel(JNIEnv*,jclass);
static void JNICALL aui_unit(JNIEnv*,jclass,jint);
static void JNICALL aui_color(JNIEnv*,jclass,jint);
static jintArray JNICALL aui_thumbnail(JNIEnv*,jclass,jint);
static void android_ui_sync(void);
static int android_ui_ready(void){
    if(AUI)return 1;
    if(!G.activity||!G.activity->env)return 0;
    JNIEnv*e=G.activity->env;
    jclass ac=(*e)->GetObjectClass(e,G.activity->clazz);
    jmethodID loader=(*e)->GetMethodID(e,ac,"getClassLoader","()Ljava/lang/ClassLoader;");
    jobject cl=loader?(*e)->CallObjectMethod(e,G.activity->clazz,loader):0;
    jclass cc=cl?(*e)->GetObjectClass(e,cl):0;
    jmethodID load=cc?(*e)->GetMethodID(e,cc,"loadClass","(Ljava/lang/String;)Ljava/lang/Class;"):0;
    jstring name=(*e)->NewStringUTF(e,"com.ayomi.infinitecanvas.VastUi");
    jclass local=load?(jclass)(*e)->CallObjectMethod(e,cl,load,name):0;
    if(!jni_clear(e)&&local){
        JNINativeMethod methods[]={
            {"nativeText","(Ljava/lang/String;)V",(void*)aui_text},
            {"nativeAction","(I)V",(void*)aui_action},
            {"nativeCancel","()V",(void*)aui_cancel},
            {"nativeUnit","(I)V",(void*)aui_unit},
            {"nativeThumbnail","(I)[I",(void*)aui_thumbnail},
            {"nativeColor","(I)V",(void*)aui_color}};
        if((*e)->RegisterNatives(e,local,methods,6)==0)AUI=(*e)->NewGlobalRef(e,local);
    }
    jni_clear(e);
    if(local)(*e)->DeleteLocalRef(e,local);if(name)(*e)->DeleteLocalRef(e,name);
    if(cc)(*e)->DeleteLocalRef(e,cc);if(cl)(*e)->DeleteLocalRef(e,cl);if(ac)(*e)->DeleteLocalRef(e,ac);
    return AUI!=0;
}
static jstring aui_string(JNIEnv*e,const char*text){
    int cap=str_len_local(text)+1;jchar*units=malloc((size_t)cap*sizeof(jchar));if(!units)return 0;
    int n=copy_utf8_to_utf16(units,cap,text);jstring result=(*e)->NewString(e,units,n);free(units);return result;
}
static void aui_void(const char*method){
    if(!AUI||!G.activity)return;JNIEnv*e=G.activity->env;
    jmethodID m=(*e)->GetStaticMethodID(e,AUI,method,"()V");if(m)(*e)->CallStaticVoidMethod(e,AUI,m);jni_clear(e);
}
static void system_text_stop(void){aui_void("closeEditor");G.systemImeActive=0;G.systemEdit=0;}
static void system_text_read(char*out,int cap){
    if(!out||cap<=0||!AUI||!G.systemImeActive)return;JNIEnv*e=G.activity->env;
    jmethodID m=(*e)->GetStaticMethodID(e,AUI,"editorText","()Ljava/lang/String;");
    jstring value=m?(jstring)(*e)->CallStaticObjectMethod(e,AUI,m):0;
    if(!jni_clear(e)&&value){const jchar*chars=(*e)->GetStringChars(e,value,0);
        if(chars){copy_utf16_to_utf8(out,cap,chars,(*e)->GetStringLength(e,value));(*e)->ReleaseStringChars(e,value,chars);}}
    if(value)(*e)->DeleteLocalRef(e,value);
}
static int system_text_start(const char*initial,int mode){
    AUI_PANEL_HASH=0;
    if(mode==6)AUI_CALIB_UNIT=G.cadUnit;
    if(!android_ui_ready())return 0;JNIEnv*e=G.activity->env;
    int limit=mode==1?(int)sizeof(G.editorBuf)-1:(mode==3||mode==7)?63:mode==6?20:31;
    jmethodID m=(*e)->GetStaticMethodID(e,AUI,"showEditor","(Landroid/app/Activity;Ljava/lang/String;IIII)Z");
    jstring value=aui_string(e,initial?initial:"");
    G.systemImeActive=m&&value?(*e)->CallStaticBooleanMethod(e,AUI,m,G.activity->clazz,value,mode,limit,G.cadUnit,(jint)C_BG):0;
    if(jni_clear(e))G.systemImeActive=0;if(value)(*e)->DeleteLocalRef(e,value);return G.systemImeActive;
}
