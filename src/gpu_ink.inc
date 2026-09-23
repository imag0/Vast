/* Retained world-space ink, GLES 3 (the compositor remains GLES 2 compatible).
 * Fixed-size chunks bound allocation/upload work during arbitrarily long
 * strokes. Camera changes update uniforms only. Chunks have world bounds for
 * rejection without traversing their samples. No sampling or AA LOD changes. */
extern void glGenBuffers(GLsizei,GLuint*);extern void glDeleteBuffers(GLsizei,const GLuint*);
extern void glBindBuffer(GLenum,GLuint);extern void glBufferData(GLenum,long,const void*,GLenum);
extern void glBufferSubData(GLenum,long,long,const void*);
extern void glEnable(GLenum);extern void glDisable(GLenum);extern void glBlendFunc(GLenum,GLenum);extern void glBlendEquation(GLenum);
extern void glGenFramebuffers(GLsizei,GLuint*);extern void glDeleteFramebuffers(GLsizei,const GLuint*);
extern void glBindFramebuffer(GLenum,GLuint);extern void glFramebufferTexture2D(GLenum,GLenum,GLenum,GLuint,GLint);
extern GLenum glCheckFramebufferStatus(GLenum);extern void glScissor(GLint,GLint,GLsizei,GLsizei);
#define GI_BUFFER 0x8892
#define GI_DYNAMIC 0x88E8
#define GI_BLEND 0x0BE2
#define GI_SRC_ALPHA 0x0302
#define GI_ONE_MINUS_SRC_ALPHA 0x0303
#define GI_ADD 0x8006
#define GI_MAX 0x8008
#define GI_FBO 0x8D40
#define GI_COLOR_ATTACHMENT 0x8CE0
#define GI_COMPLETE 0x8CD5
#define GI_SCISSOR 0x0C11
#define GI_TRIANGLES 0x0004
#define GI_SEGMENTS 256
typedef struct InkChunk {GLuint buffer;int segments;float x0,y0,x1,y1;struct InkChunk*next;} InkChunk;
typedef struct {InkChunk*first,*last;Point*source;int n,revision;float width;uint32_t color;} InkMesh;
static struct {GLuint program,maskProgram,maskTex,fbo;GLint cam,res,color,maskColor,limits;InkMesh*meshes;int cap,ready,w,h;uint64_t segmentsBuilt,uploadBytes;} GI;
static float GI_VERTICES[GI_SEGMENTS*6*8];
static InkChunk*GI_PENDING;static int GI_PENDING_START;
static void gi_flush(void){if(!GI_PENDING)return;int n=GI_PENDING->segments-GI_PENDING_START;glBindBuffer(GI_BUFFER,GI_PENDING->buffer);glBufferSubData(GI_BUFFER,(long)GI_PENDING_START*192,(long)n*192,GI_VERTICES+GI_PENDING_START*48);GI.uploadBytes+=(uint64_t)n*192;GI_PENDING=0;}
static const char*GI_VS=
"precision highp float;attribute vec4 aEnds;attribute vec4 aShape;uniform vec3 uCam;uniform vec2 uRes;uniform vec2 uLimits;varying vec2 vPoint;varying vec4 vEnds;varying vec2 vRadius;void main(){"
"vec2 a=aEnds.xy*uCam.z+uCam.xy,b=aEnds.zw*uCam.z+uCam.xy;vec2 delta=b-a;float len=length(delta);vec2 dir=len>.0001?delta/len:vec2(1.,0.);vec2 normal=vec2(-dir.y,dir.x);vec2 radii=clamp(aShape.xy*uCam.z,vec2(uLimits.x),vec2(uLimits.y));float pad=max(radii.x,radii.y)+1.;vec2 p=mix(a,b,aShape.z)+dir*((aShape.z*2.-1.)*pad)+normal*aShape.w*pad;vPoint=p;vEnds=vec4(a,b);vRadius=radii;gl_Position=vec4(p.x/uRes.x*2.-1.,1.-p.y/uRes.y*2.,0.,1.);}";
static const char*GI_FS=
"precision highp float;varying vec2 vPoint;varying vec4 vEnds;varying vec2 vRadius;uniform vec4 uColor;void main(){vec2 ab=vEnds.zw-vEnds.xy;float t=clamp(dot(vPoint-vEnds.xy,ab)/max(dot(ab,ab),.000001),0.,1.);float radius=mix(vRadius.x,vRadius.y,t);float d=length(vPoint-mix(vEnds.xy,vEnds.zw,t));float coverage=clamp(radius+.5-d,0.,1.);gl_FragColor=vec4(uColor.rgb,coverage*uColor.a);}";
static const char*GI_MASK_FS=
"precision mediump float;varying vec2 vUv;uniform sampler2D uMask;uniform vec4 uColor;void main(){gl_FragColor=vec4(uColor.rgb,texture2D(uMask,vUv).a*uColor.a);}";
static GLuint gi_program(const char*vs,const char*fs,int ink){
    GLuint v=gpu_shader(GL_VERTEX_SHADER,vs),f=gpu_shader(GL_FRAGMENT_SHADER,fs);if(!v||!f){if(v)glDeleteShader(v);if(f)glDeleteShader(f);return 0;}
    GLuint p=glCreateProgram();glAttachShader(p,v);glAttachShader(p,f);glBindAttribLocation(p,0,ink?"aEnds":"aPos");if(ink)glBindAttribLocation(p,1,"aShape");glLinkProgram(p);glDeleteShader(v);glDeleteShader(f);GLint ok=0;glGetProgramiv(p,GL_LINK_STATUS,&ok);if(!ok){glDeleteProgram(p);return 0;}return p;
}
static void gi_free_mesh(InkMesh*m){InkChunk*c=m->first;while(c){InkChunk*n=c->next;glDeleteBuffers(1,&c->buffer);free(c);c=n;}memset(m,0,sizeof(*m));}
static void gi_destroy(void){
    GI_PENDING=0;for(int i=0;i<GI.cap;i++)gi_free_mesh(&GI.meshes[i]);free(GI.meshes);
    if(GI.program)glDeleteProgram(GI.program);if(GI.maskProgram)glDeleteProgram(GI.maskProgram);
    if(GI.maskTex)glDeleteTextures(1,&GI.maskTex);if(GI.fbo)glDeleteFramebuffers(1,&GI.fbo);memset(&GI,0,sizeof(GI));
}
static int gi_init(void){
    GI.program=gi_program(GI_VS,GI_FS,1);GI.maskProgram=gi_program(VAST_VS,GI_MASK_FS,0);if(!GI.program||!GI.maskProgram){gi_destroy();return 0;}
    GI.cam=glGetUniformLocation(GI.program,"uCam");GI.res=glGetUniformLocation(GI.program,"uRes");GI.color=glGetUniformLocation(GI.program,"uColor");GI.limits=glGetUniformLocation(GI.program,"uLimits");GI.maskColor=glGetUniformLocation(GI.maskProgram,"uColor");
    glUseProgram(GI.maskProgram);glUniform1i(glGetUniformLocation(GI.maskProgram,"uMask"),0);glGenTextures(1,&GI.maskTex);glGenFramebuffers(1,&GI.fbo);GI.ready=1;return 1;
}
static int gi_size(int w,int h){if(GI.w==w&&GI.h==h)return 1;glActiveTexture(GL_TEXTURE0);gpu_tex_setup(GI.maskTex);glTexImage2D(GL_TEXTURE_2D,0,GL_RGBA,w,h,0,GL_RGBA,GL_UNSIGNED_BYTE,0);glBindFramebuffer(GI_FBO,GI.fbo);glFramebufferTexture2D(GI_FBO,GI_COLOR_ATTACHMENT,GL_TEXTURE_2D,GI.maskTex,0);int ok=glCheckFramebufferStatus(GI_FBO)==GI_COMPLETE;glBindFramebuffer(GI_FBO,0);if(ok){GI.w=w;GI.h=h;}return ok;}
static int gi_visible(float x0,float y0,float x1,float y1){return x1*G.scale+G.offX>=-2&&y1*G.scale+G.offY>=-2&&x0*G.scale+G.offX<=G.screenW+2&&y0*G.scale+G.offY<=G.screenH+2;}
static InkChunk*gi_chunk(InkMesh*m){
    if(m->last&&m->last->segments<GI_SEGMENTS)return m->last;
    gi_flush();InkChunk*c=malloc(sizeof(*c));if(!c)return 0;memset(c,0,sizeof(*c));glGenBuffers(1,&c->buffer);glBindBuffer(GI_BUFFER,c->buffer);glBufferData(GI_BUFFER,sizeof(GI_VERTICES),0,GI_DYNAMIC);
    if(m->last)m->last->next=c;else m->first=c;m->last=c;return c;
}
static int gi_segment(InkMesh*m,Point a,Point b,float width,int marker){
    InkChunk*c=gi_chunk(m);if(!c)return 0;
    float ra=width*(marker?1:a.p)*.5f,rb=width*(marker?1:b.p)*.5f;
    static const float corners[12]={0,-1,1,-1,0,1,0,1,1,-1,1,1};float v[48];
    for(int k=0;k<6;k++){float*p=v+k*8;p[0]=a.x;p[1]=a.y;p[2]=b.x;p[3]=b.y;p[4]=ra;p[5]=rb;p[6]=corners[k*2];p[7]=corners[k*2+1];}
    if(GI_PENDING!=c){gi_flush();GI_PENDING=c;GI_PENDING_START=c->segments;}memcpy(GI_VERTICES+c->segments*48,v,sizeof(v));GI.segmentsBuilt++;
    float pad=fmax2(ra,rb),x0=fmin2(a.x,b.x)-pad,y0=fmin2(a.y,b.y)-pad,x1=fmax2(a.x,b.x)+pad,y1=fmax2(a.y,b.y)+pad;
    if(!c->segments){c->x0=x0;c->y0=y0;c->x1=x1;c->y1=y1;}else{c->x0=fmin2(c->x0,x0);c->y0=fmin2(c->y0,y0);c->x1=fmax2(c->x1,x1);c->y1=fmax2(c->y1,y1);}c->segments++;return 1;
}
static int gi_sync_mesh(InkMesh*m,Stroke*s){
    if(m->revision!=G.sceneRevision||m->n>s->n||m->width!=s->baseWidth||m->color!=s->color||(m->n>=s->n&&m->source!=s->pts))gi_free_mesh(m);
    m->revision=G.sceneRevision;m->source=s->pts;m->width=s->baseWidth;m->color=s->color;
    int aa=(s->color>>24)&255,marker=aa>0&&aa<248;
    if(!m->n&&s->n){if(!gi_segment(m,s->pts[0],s->pts[0],s->baseWidth,marker))return 0;m->n=1;}
    for(int i=m->n;i<s->n;i++){
        Point a=s->pts[i-1],b=s->pts[i],prev=s->pts[i>1?i-2:0],p=a;int steps=marker?1:ink_curve_steps(prev,a,b,i==1);
        for(int k=1;k<=steps;k++){Point q=marker?b:ink_curve_point(prev,a,b,(float)k/steps,i==1);if(!gi_segment(m,p,q,s->baseWidth,marker))return 0;p=q;}m->n=i+1;
    }gi_flush();return 1;
}
static void gi_draw_mesh_style(InkMesh*m,uint32_t color,float alpha,float minR,float maxR){
    glUseProgram(GI.program);glUniform3f(GI.cam,G.offX,G.offY,G.scale);glUniform2f(GI.res,G.screenW,G.screenH);glUniform4f(GI.color,((color>>16)&255)/255.f,((color>>8)&255)/255.f,(color&255)/255.f,alpha);glUniform2f(GI.limits,minR,maxR);
    glEnableVertexAttribArray(0);glEnableVertexAttribArray(1);
    for(InkChunk*c=m->first;c;c=c->next){if(!gi_visible(c->x0,c->y0,c->x1,c->y1))continue;glBindBuffer(GI_BUFFER,c->buffer);glVertexAttribPointer(0,4,GL_FLOAT,0,32,0);glVertexAttribPointer(1,4,GL_FLOAT,0,32,(void*)16);glDrawArrays(GI_TRIANGLES,0,c->segments*6);}
    glDisableVertexAttribArray(1);glDisableVertexAttribArray(0);glBindBuffer(GI_BUFFER,0);
}
static void gi_draw_mesh(InkMesh*m,uint32_t color){int a=(color>>24)&255;gi_draw_mesh_style(m,color,1,.48f,a>0&&a<248?110:80);}
static int gi_draw(void){
    if(!GI.ready||!gi_size(G.screenW,G.screenH))return 0;
    if(G.strokeN>GI.cap){int cap=maxi(G.strokeN,GI.cap?GI.cap*2:128);InkMesh*p=realloc(GI.meshes,(size_t)cap*sizeof(*p));if(!p)return 0;memset(p+GI.cap,0,(size_t)(cap-GI.cap)*sizeof(*p));GI.meshes=p;GI.cap=cap;}
    glEnable(GI_BLEND);glBlendEquation(GI_ADD);glBlendFunc(GI_SRC_ALPHA,GI_ONE_MINUS_SRC_ALPHA);
    for(int i=0;i<G.strokeN;i++){
        Stroke*s=&G.strokes[i];int aa=(s->color>>24)&255,marker=aa>0&&aa<248;if(!s->active||!s->n||(marker?!G.visMarker:!G.visInk))continue;
        float pad=s->baseWidth;if(!gi_visible(s->minx-pad,s->miny-pad,s->maxx+pad,s->maxy+pad))continue;
        InkMesh*m=&GI.meshes[i];double buildStart=perf_us();if(!gi_sync_mesh(m,s)){gi_flush();glDisable(GI_BLEND);return 0;}IP.geometryUs+=perf_us()-buildStart;if(VG.liveActive&&i==G.currentStroke)continue;
        if(!marker){gi_draw_mesh(m,s->color);continue;}
        /* A marker stroke is a single coverage layer, including self-crossings.
         * MAX accumulation avoids dark seams and repeated alpha at joins. */
        int x=maxi(0,(int)floorf((s->minx-pad)*G.scale+G.offX-2)),y=maxi(0,(int)floorf((s->miny-pad)*G.scale+G.offY-2));
        int x1=mini(G.screenW,(int)ceilf((s->maxx+pad)*G.scale+G.offX+2)),y1=mini(G.screenH,(int)ceilf((s->maxy+pad)*G.scale+G.offY+2));
        glEnable(GI_SCISSOR);glScissor(x,G.screenH-y1,maxi(0,x1-x),maxi(0,y1-y));glBindFramebuffer(GI_FBO,GI.fbo);glClearColor(0,0,0,0);glClear(GL_COLOR_BUFFER_BIT);glBlendEquation(GI_MAX);gi_draw_mesh(m,s->color);
        glBindFramebuffer(GI_FBO,0);glBlendEquation(GI_ADD);glUseProgram(GI.maskProgram);glUniform4f(GI.maskColor,((s->color>>16)&255)/255.f,((s->color>>8)&255)/255.f,(s->color&255)/255.f,aa/255.f);glActiveTexture(GL_TEXTURE0);glBindTexture(GL_TEXTURE_2D,GI.maskTex);
        static const float q[]={-1,-1,1,-1,-1,1,1,1};glEnableVertexAttribArray(0);glVertexAttribPointer(0,2,GL_FLOAT,0,0,q);glDrawArrays(GL_TRIANGLE_STRIP,0,4);glDisableVertexAttribArray(0);glDisable(GI_SCISSOR);
    }glDisable(GI_BLEND);return 1;
}
