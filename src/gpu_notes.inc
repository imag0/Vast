/* Text is shaped by Android once per content change. World quads and retained
 * glyph textures are independent of camera scale. Tall text is tiled to respect
 * hardware texture limits; invisible tiles never draw. */
typedef struct {GLuint*tiles;int count,w,h,revision,id;uint64_t hash;} GpuNote;
static GpuNote GN[128];static GLuint GN_PROGRAM;static GLint GN_CAM,GN_RES,GN_BOX,GN_COLOR,GN_MODE,GN_RADIUS,GN_ROTATION;
static const char*GN_VS="precision highp float;attribute vec2 aPos;uniform vec4 uBox;uniform float uRotation;uniform vec3 uCam;uniform vec2 uRes;varying vec2 vUv;varying vec2 vSize;void main(){vUv=aPos;vSize=uBox.zw*uCam.z;vec2 local=(aPos-.5)*uBox.zw;float c=cos(uRotation),s=sin(uRotation);vec2 p=(uBox.xy+uBox.zw*.5+vec2(c*local.x-s*local.y,s*local.x+c*local.y))*uCam.z+uCam.xy;gl_Position=vec4(p.x/uRes.x*2.-1.,1.-p.y/uRes.y*2.,0.,1.);}";
static const char*GN_FS="precision highp float;varying vec2 vUv;varying vec2 vSize;uniform sampler2D uText;uniform vec4 uColor;uniform float uMode,uRadius;void main(){if(uMode>.5){vec4 p=texture2D(uText,vUv);if(uMode<1.5&&min(min(p.r,p.g),p.b)>.995)p.rgb=uColor.rgb;gl_FragColor=p;}else{float r=min(uRadius,min(vSize.x,vSize.y)*.5);vec2 q=abs((vUv-.5)*vSize)-vSize*.5+r;float d=length(max(q,0.))+min(max(q.x,q.y),0.)-r;gl_FragColor=vec4(uColor.rgb,uColor.a*clamp(.5-d,0.,1.));}}";
static void gn_free(GpuNote*n){if(n->tiles){glDeleteTextures(n->count,n->tiles);free(n->tiles);}memset(n,0,sizeof(*n));}
static void gn_destroy(void){for(int i=0;i<128;i++)gn_free(&GN[i]);if(GN_PROGRAM)glDeleteProgram(GN_PROGRAM);GN_PROGRAM=0;}
static int gn_init(void){if(GN_PROGRAM)return 1;GN_PROGRAM=gi_program(GN_VS,GN_FS,0);if(!GN_PROGRAM)return 0;GN_CAM=glGetUniformLocation(GN_PROGRAM,"uCam");GN_RES=glGetUniformLocation(GN_PROGRAM,"uRes");GN_BOX=glGetUniformLocation(GN_PROGRAM,"uBox");GN_COLOR=glGetUniformLocation(GN_PROGRAM,"uColor");GN_MODE=glGetUniformLocation(GN_PROGRAM,"uMode");GN_RADIUS=glGetUniformLocation(GN_PROGRAM,"uRadius");GN_ROTATION=glGetUniformLocation(GN_PROGRAM,"uRotation");glUseProgram(GN_PROGRAM);glUniform1i(glGetUniformLocation(GN_PROGRAM,"uText"),0);return 1;}
static int gn_sync(GpuNote*g,NoteObj*n){
    if(g->tiles&&g->revision==G.sceneRevision&&g->id==n->id)return 1;
    uint64_t hash=nui_hash_text(0x123456781ULL,n->text);if(g->tiles&&g->hash==hash){g->revision=G.sceneRevision;g->id=n->id;return 1;}
    NoteTextCache*c=note_text_layout(n->text);if(!c)return 0;
    int count=(c->h+2047)/2048;GLuint*tiles=malloc((size_t)count*sizeof(*tiles));uint32_t*pixels=malloc((size_t)c->w*mini(c->h,2048)*4);if(!tiles||!pixels){free(tiles);free(pixels);return 0;}glGenTextures(count,tiles);
    for(int k=0;k<count;k++){int h=mini(2048,c->h-k*2048);for(int i=0;i<c->w*h;i++){uint32_t p=c->pixels[k*2048*c->w+i];pixels[i]=(p&0xff00ff00u)|((p&255)<<16)|((p>>16)&255);}glActiveTexture(GL_TEXTURE0);gpu_tex_setup(tiles[k]);glTexImage2D(GL_TEXTURE_2D,0,GL_RGBA,c->w,h,0,GL_RGBA,GL_UNSIGNED_BYTE,pixels);}free(pixels);gn_free(g);g->tiles=tiles;g->count=count;g->w=c->w;g->h=c->h;g->hash=hash;g->revision=G.sceneRevision;g->id=n->id;n->w=c->w+24;n->h=c->h+20;return 1;
}
static void gn_color(uint32_t c,float a){glUniform4f(GN_COLOR,((c>>16)&255)/255.f,((c>>8)&255)/255.f,(c&255)/255.f,a);}
static void gn_draw(void){
    if(!G.visNotes||!gn_init())return;glEnable(GI_BLEND);glBlendEquation(GI_ADD);glBlendFunc(GI_SRC_ALPHA,GI_ONE_MINUS_SRC_ALPHA);glBindBuffer(GI_BUFFER,0);
    static const float quad[]={0,0,1,0,0,1,1,1};glUseProgram(GN_PROGRAM);glUniform1f(GN_ROTATION,0);glUniform3f(GN_CAM,G.offX,G.offY,G.scale);glUniform2f(GN_RES,G.screenW,G.screenH);glEnableVertexAttribArray(0);glVertexAttribPointer(0,2,GL_FLOAT,0,0,quad);
    for(int i=0;i<G.noteN&&i<128;i++){NoteObj*n=&G.notes[i];if(!n->active||!gi_visible(n->x,n->y,n->x+n->w,n->y+n->h))continue;GpuNote*g=&GN[i];int ready=gn_sync(g,n);glUseProgram(GN_PROGRAM);
        glUniform1f(GN_MODE,0);glUniform1f(GN_RADIUS,us(n->type==NOTE_STICKY?14:12));gn_color(n->type==NOTE_STICKY?n->color:C_GLASS2,n->type==NOTE_STICKY?205.f/255:142.f/255);glUniform4f(GN_BOX,n->x,n->y,n->w,n->h);glDrawArrays(GL_TRIANGLE_STRIP,0,4);
        if(!ready)continue;glUniform1f(GN_MODE,1);gn_color(n->type==NOTE_STICKY?C_BLACK:C_TEXT,1);
        for(int k=0;k<g->count;k++){float x=n->x+12,y=n->y+10+k*2048;int h=mini(2048,g->h-k*2048);if(!gi_visible(x,y,x+g->w,y+h))continue;glActiveTexture(GL_TEXTURE0);glBindTexture(GL_TEXTURE_2D,g->tiles[k]);glUniform4f(GN_BOX,x,y,g->w,h);glDrawArrays(GL_TRIANGLE_STRIP,0,4);}
    }glDisableVertexAttribArray(0);glDisable(GI_BLEND);
}
