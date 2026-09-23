/* Damage repaint uses the retained canvas, not a new document snapshot. A
 * whole-stroke eraser damages the removed stroke's visible bounds, not just
 * the contact circle. One reusable buffer handles the union per display frame. */
static int gpu_refresh_erase(void){
    if(!VG.cacheValid||!G.erasing||VG.lastEraseN>G.eraseAction.n||VG.lastStrokeN!=G.strokeN||VG.lastSceneRevision!=G.sceneRevision)return 0;
    if(VG.lastEraseN==G.eraseAction.n)return 1;
    /* Constellation connections can extend beyond a removed stroke. */
    if(G.farZoomMode&&G.scale<=.16f)return 0;
    int x0=VG.cw,y0=VG.ch,x1=0,y1=0;
    for(int i=VG.lastEraseN;i<G.eraseAction.n;i++){
        Stroke*s=&G.strokes[G.eraseAction.ids[i]];int a=(s->color>>24)&255,marker=a>0&&a<248;
        /* Raster caps are 80px for pressure ink and 110px for marker. The
           former 340px viewport prefetch margin is not a damage radius. */
        float pad=(marker?clampf(s->baseWidth*VG.cacheScale*.5f,.65f,110):clampf(s->baseWidth*VG.cacheScale*.9f,.48f,80))+4;
        if(G.regionGlow)pad=fmax2(pad,(float)us(64));
        x0=mini(x0,maxi(0,(int)floorf(s->minx*VG.cacheScale+VG.cacheOffX-pad)));
        y0=mini(y0,maxi(0,(int)floorf(s->miny*VG.cacheScale+VG.cacheOffY-pad)));
        x1=maxi(x1,mini(VG.cw,1+(int)floorf(s->maxx*VG.cacheScale+VG.cacheOffX+pad)));
        y1=maxi(y1,mini(VG.ch,1+(int)floorf(s->maxy*VG.cacheScale+VG.cacheOffY+pad)));
    }
    if(x1<=x0||y1<=y0){gpu_remember_content();return 1;}
    int w=x1-x0,h=y1-y0;size_t need=(size_t)w*h;
    if(need>erasePixelsCap){uint32_t*p=(uint32_t*)realloc(erasePixels,need*4);if(!p)return 0;erasePixels=p;erasePixelsCap=need;}
    float pad=120.0f/VG.cacheScale;
    if(!erase_render_candidates((x0-VG.cacheOffX)/VG.cacheScale-pad,(y0-VG.cacheOffY)/VG.cacheScale-pad,(x1-VG.cacheOffX)/VG.cacheScale+pad,(y1-VG.cacheOffY)/VG.cacheScale+pad))return 0;
    float os=G.scale,ox=G.offX,oy=G.offY,lod=G.lodSmoothPx,mix=G.lodMotionMix;int sw=G.screenW,sh=G.screenH;
    G.scale=VG.cacheScale;G.offX=VG.cacheOffX-x0;G.offY=VG.cacheOffY-y0;G.screenW=w;G.screenH=h;ER_active=1;
    Surf s={(uint8_t*)erasePixels,w,h,w,FORMAT_RGBA_LAYER};clear_transparent(&s);draw_canvas_world(&s);ER_active=0;
    G.scale=os;G.offX=ox;G.offY=oy;G.screenW=sw;G.screenH=sh;G.lodSmoothPx=lod;G.lodMotionMix=mix;
    for(int y=0;y<h;y++)memcpy(VG.canvasBuf+(size_t)(y+y0)*VG.cw+x0,erasePixels+(size_t)y*w,(size_t)w*4);
    glActiveTexture(GL_TEXTURE0);glBindTexture(GL_TEXTURE_2D,VG.canvasTex);glPixelStorei(GL_UNPACK_ALIGNMENT,4);
    glTexSubImage2D(GL_TEXTURE_2D,0,x0,y0,w,h,GL_RGBA,GL_UNSIGNED_BYTE,erasePixels);gpu_remember_content();return 1;
}
