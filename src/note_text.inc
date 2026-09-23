/* Platform-shaped text, including color emoji. No JNI or allocation on ink frames. */
typedef struct {uint64_t hash,used;uint32_t*pixels;int w,h;} NoteTextCache;
static NoteTextCache NOTE_CACHE[128];static uint64_t NOTE_CACHE_TICK;
static void note_text_cache_clear(void){for(int i=0;i<128;i++){free(NOTE_CACHE[i].pixels);memset(&NOTE_CACHE[i],0,sizeof(NOTE_CACHE[i]));}}
static NoteTextCache*note_text_layout(const char*text){
    if(!G.activity)return 0;
    uint64_t hash=nui_hash_text(0x123456781ULL,text);int slot=0;
    for(int i=0;i<128;i++){if(NOTE_CACHE[i].pixels&&NOTE_CACHE[i].hash==hash){NOTE_CACHE[i].used=++NOTE_CACHE_TICK;return &NOTE_CACHE[i];}if(NOTE_CACHE[i].used<NOTE_CACHE[slot].used)slot=i;}
    if(G.stylusDown||!android_ui_ready())return 0;
    JNIEnv*e=G.activity->env;jmethodID m=(*e)->GetStaticMethodID(e,AUI,"rasterText","(Ljava/lang/String;)[I");jstring value=aui_string(e,text);
    jintArray pixels=m&&value?(jintArray)(*e)->CallStaticObjectMethod(e,AUI,m,value):0;
    if(value)(*e)->DeleteLocalRef(e,value);if(jni_clear(e)||!pixels)return 0;
    jint size[2];(*e)->GetIntArrayRegion(e,pixels,0,2,size);
    int w=size[0],h=size[1];NoteTextCache*cache=&NOTE_CACHE[slot];
    if(w>0&&w<=480&&h>0&&h<=150000&&(size_t)w*h<=8000000u&&(*e)->GetArrayLength(e,pixels)==w*h+2){
        size_t bytes=(size_t)w*h*4;for(int i=0;i<128;i++)if(i!=slot&&NOTE_CACHE[i].pixels)bytes+=(size_t)NOTE_CACHE[i].w*NOTE_CACHE[i].h*4;
        while(bytes>16u*1024u*1024u){int victim=-1;for(int i=0;i<128;i++)if(i!=slot&&NOTE_CACHE[i].pixels&&(victim<0||NOTE_CACHE[i].used<NOTE_CACHE[victim].used))victim=i;if(victim<0)break;bytes-=(size_t)NOTE_CACHE[victim].w*NOTE_CACHE[victim].h*4;free(NOTE_CACHE[victim].pixels);memset(&NOTE_CACHE[victim],0,sizeof(NOTE_CACHE[victim]));}
        uint32_t*rgba=malloc((size_t)w*h*4);if(rgba){(*e)->GetIntArrayRegion(e,pixels,2,w*h,(jint*)rgba);
            free(cache->pixels);cache->pixels=rgba;cache->w=w;cache->h=h;cache->hash=hash;cache->used=++NOTE_CACHE_TICK;}
    }
    (*e)->DeleteLocalRef(e,pixels);jni_clear(e);return cache->hash==hash?cache:0;
}
static int note_text_draw(Surf*s,NoteObj*n,uint32_t color){
    NoteTextCache*c=note_text_layout(n->text);if(!c)return 0;
    float scale=G.scale,x0=(n->x+12)*scale+G.offX,y0=(n->y+10)*scale+G.offY;
    int left=maxi(0,(int)floorf(x0)),top=maxi(0,(int)floorf(y0));
    int right=mini(s->w,(int)(x0+c->w*scale+.5f)),bottom=mini(s->h,(int)(y0+c->h*scale+.5f));
    for(int y=top;y<bottom;y++){float sy=(y+.5f-y0)/scale-.5f;int iy=(int)floorf(sy);float fy=sy-iy;int ya=maxi(0,mini(c->h-1,iy)),yb=maxi(0,mini(c->h-1,iy+1));
        for(int x=left;x<right;x++){float sx=(x+.5f-x0)/scale-.5f;int ix=(int)floorf(sx);float fx=sx-ix;int xa=maxi(0,mini(c->w-1,ix)),xb=maxi(0,mini(c->w-1,ix+1));
            uint32_t samples[4]={c->pixels[ya*c->w+xa],c->pixels[ya*c->w+xb],c->pixels[yb*c->w+xa],c->pixels[yb*c->w+xb]};float weights[4]={(1-fx)*(1-fy),fx*(1-fy),(1-fx)*fy,fx*fy};float a=0,r=0,g=0,b=0;
            for(int i=0;i<4;i++){uint32_t q=samples[i],rgb=(q&0xffffffu)==0xffffffu?color:q&0xffffffu;float aw=(q>>24)*weights[i];a+=aw;r+=((rgb>>16)&255)*aw;g+=((rgb>>8)&255)*aw;b+=(rgb&255)*aw;}if(a>.01f)blendpx(s,x,y,((uint32_t)(r/a+.5f)<<16)|((uint32_t)(g/a+.5f)<<8)|(uint32_t)(b/a+.5f),(int)(a+.5f));}}
    return 1;
}
