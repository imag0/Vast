/* Native product panels. IDs route to the same tested C actions as canvas tools. */
static void JNICALL aui_text(JNIEnv*e,jclass cls,jstring value){
    (void)cls;if(!value)return;const jchar*p=(*e)->GetStringChars(e,value,0);if(!p)return;
    if(G.calibrationOpen){copy_utf16_to_utf8(G.calibText,sizeof(G.calibText),p,(*e)->GetStringLength(e,value));G.calibLen=str_len_local(G.calibText);}
    else if(G.editorOpen){copy_utf16_to_utf8(G.editorBuf,sizeof(G.editorBuf),p,(*e)->GetStringLength(e,value));G.editorLen=str_len_local(G.editorBuf);editor_live_apply();}
    (*e)->ReleaseStringChars(e,value,p);
    /* Android paints each edit immediately. Do not block its UI thread with a
       full document raster underneath an opaque editing dialog. The changed
       scene is presented when the dialog closes or the window redraws. */
}
static void JNICALL aui_cancel(JNIEnv*e,jclass cls){
    (void)e;(void)cls;if(G.calibrationOpen){G.cadUnit=AUI_CALIB_UNIT;G.calibrationOpen=0;system_text_stop();}else if(G.editorOpen)editor_cancel();render();
}
static void JNICALL aui_unit(JNIEnv*e,jclass cls,jint unit){(void)e;(void)cls;G.cadUnit=maxi(0,mini(4,unit));G.metaDirty=1;}
static void JNICALL aui_color(JNIEnv*e,jclass cls,jint color){(void)e;(void)cls;G.color=(uint32_t)color&0xffffffu;rgb_hsv(G.color,&G.pickerHue,&G.pickerSat,&G.pickerVal);G.metaDirty=1;G.pickerSessionTouched=1;request_input_frame();}
static jintArray JNICALL aui_thumbnail(JNIEnv*e,jclass cls,jint index){
    (void)cls;if(index<0||index>=G.thumbN)return 0;Thumb*t=&G.thumbs[index];if(!t->px||t->w<=0||t->h<=0||t->w>1024||t->h>1024)return 0;
    jintArray array=(*e)->NewIntArray(e,2+t->w*t->h);if(!array)return 0;jint size[2]={t->w,t->h};(*e)->SetIntArrayRegion(e,array,0,2,size);
    jint row[1024];for(int y=0;y<t->h;y++){for(int x=0;x<t->w;x++)row[x]=(jint)(t->px[y*t->w+x]|0xff000000u);(*e)->SetIntArrayRegion(e,array,2+y*t->w,t->w,row);}return array;
}
static int AUI_FRAME_SECTION;
static void JNICALL aui_action(JNIEnv*e,jclass cls,jint action){
    if(action>=6000&&action<6000+MAX_PROJECTS){switch_project(action-6000);render();return;}
    if(action==6100){if(project_create_new()<0)set_toast(8,monotonic_ms());render();return;}
    if(action==6101){set_toast(project_request_export()?11:8,monotonic_ms());render();return;}
    if(action==6102){project_request_import();return;}
    if(action==6103){project_snapshot_maybe(1);set_toast(11,monotonic_ms());render();return;}
    if(action==6104){set_toast(project_restore_latest()?11:8,monotonic_ms());render();return;}
    if(action==6105){G.projectFavorite[G.projectIndex]=!G.projectFavorite[G.projectIndex];save_project_prefs();render();return;}
    if(action==6106){G.projectArchived[G.projectIndex]=!G.projectArchived[G.projectIndex];save_project_prefs();render();return;}
    if(action==6107){editor_open(7,G.projectIndex,G.projectTags[G.projectIndex]);render();return;}
    if(action==6500){G.leftHanded=!G.leftHanded;save_project_prefs();render();return;}
    if(action==5300){G.railOpen=!G.railOpen;start_anim_timer();if(G.railOpen)arm_rail_timer();else cancel_rail_timer();render();return;}
    if(action==5200||action==5201){AUI_FRAME_SECTION=action-5200;render();return;}
    if(action==5000||action==5001){G.settingsTab=action==5000?5:0;render();return;}
    if(action>=5100&&action<5196){int index=(action-5100)/3,op=(action-5100)%3;if(index<G.bookmarkN){if(op==0){bookmark_jump(index);G.framePanel=0;}else if(op==1)editor_open(5,index,G.bookmarks[index].name);else bookmark_delete(index);}save_workspace();render();return;}
    (void)e;(void)cls;handle_ui(action,monotonic_ms());
    if(action==UI_CAL_APPLY&&!G.calibrationOpen)system_text_stop();
}
static char AUI_JSON[24000];static int AUI_LEN,AUI_ROWS;
static uint64_t aui_json_hash(uint64_t seed){uint64_t h=seed;for(int i=0;i<AUI_LEN;i++)h=nui_hash_mix(h,(uint8_t)AUI_JSON[i]);return h;}
static void aui_append(const char*s){int n=str_len_local(s);if(AUI_LEN+n<(int)sizeof(AUI_JSON)){memcpy(AUI_JSON+AUI_LEN,s,n);AUI_LEN+=n;AUI_JSON[AUI_LEN]=0;}}
static void aui_quote(const char*s){aui_append("\"");for(int i=0;s&&s[i];i++){char c=s[i],b[8];if(c=='\"'||c=='\\'){b[0]='\\';b[1]=c;b[2]=0;}else if((unsigned char)c<32)snprintf(b,sizeof(b),"\\u%04x",(unsigned char)c);else{b[0]=c;b[1]=0;}aui_append(b);}aui_append("\"");}
static void aui_begin(const char*title,int close){char b[64];AUI_LEN=AUI_ROWS=0;AUI_JSON[0]=0;aui_append("{\"title\":");aui_quote(title);snprintf(b,sizeof(b),",\"close\":%d,\"rows\":[",close);aui_append(b);}
static void aui_row(const char*kind,const char*title,int id,int checked,int enabled){char b[120];if(AUI_ROWS++)aui_append(",");aui_append("{\"kind\":");aui_quote(kind);aui_append(",\"text\":");aui_quote(title);snprintf(b,sizeof(b),",\"id\":%d,\"checked\":%s,\"enabled\":%s}",id,checked?"true":"false",enabled?"true":"false");aui_append(b);}
static void aui_button(const char*t,int id){aui_row("button",t,id,0,1);}
static void aui_danger(const char*t,int id,int enabled){aui_row("danger",t,id,0,enabled);}
static void aui_text_row(const char*t){aui_row("text",t,0,0,1);}
static void aui_heading(const char*t){aui_row("heading",t,0,0,1);}
static void aui_toggle(const char*t,int id,int checked){aui_row("toggle",t,id,checked,1);}
static void aui_step(const char*t,const char*v,int minus,int plus){char b[100];if(AUI_ROWS++)aui_append(",");aui_append("{\"kind\":\"step\",\"text\":");aui_quote(t);aui_append(",\"value\":");aui_quote(v);snprintf(b,sizeof(b),",\"minus\":%d,\"plus\":%d}",minus,plus);aui_append(b);}
static void aui_percent(const char*t,float v,int minus,int plus){char b[32];snprintf(b,sizeof(b),"%.0f%%",v*100);aui_step(t,b,minus,plus);}
static int android_ui_modal(void){return AUI&&(G.editorOpen||G.calibrationOpen||G.settingsPanel||G.projectsPanel||G.addPanel||G.layersPanel||G.searchPanel||G.framePanel||G.selectionMoreOpen||G.galleryOpen);}
static void android_chrome_sync(void){
    uint64_t hash=nui_hash_text((uint64_t)C_BG,G.projectNames[G.projectIndex]);
    hash=nui_hash_mix(hash,(uint32_t)(G.zenMode|G.railOpen<<1|android_ui_modal()<<2|G.presentationMode<<3|G.galleryOpen<<4|G.leftHanded<<5));hash=nui_hash_mix(hash,G.screenW);hash=nui_hash_mix(hash,G.screenH);
    hash=nui_hash_mix(hash,G.selectedRefN);hash=nui_hash_mix(hash,G.photoMode);hash=nui_hash_mix(hash,G.tool);hash=nui_hash_mix(hash,G.selectedMeasure+1);hash=nui_hash_mix(hash,G.selectedImage+1);hash=nui_hash_mix(hash,G.snap);hash=nui_hash_mix(hash,G.photoAngleSnap);hash=nui_hash_mix(hash,G.pressurePanel);hash=nui_hash_mix(hash,G.shapeType);hash=nui_hash_mix(hash,G.cadCalibrated);hash=nui_hash_mix(hash,G.cadUnit);hash=nui_hash_mix(hash,G.measureN);hash=nui_hash_mix(hash,G.sceneRevision);hash=nui_hash_mix(hash,G.actionCursor);hash=nui_hash_mix(hash,G.actionN);hash=nui_hash_mix(hash,workspace_lock_mask());hash=nui_hash_mix(hash,write_checkpoint_away());hash=nui_hash_mix(hash,G.projectFavorite[G.projectIndex]|G.projectArchived[G.projectIndex]<<1|G.projectSnapshotCursor[G.projectIndex]<<8);hash=nui_hash_text(hash,G.projectTags[G.projectIndex]);
    if(hash==AUI_CHROME_HASH)return;AUI_CHROME_HASH=hash;AUI_LEN=AUI_ROWS=0;AUI_JSON[0]=0;aui_append("{\"project\":");aui_quote(G.projectNames[G.projectIndex]);
    char b[160];snprintf(b,sizeof(b),",\"hidden\":%s,\"zen\":%s,\"rail\":%s,\"leftHanded\":%s,\"header\":[",(android_ui_modal()||G.presentationMode||G.galleryOpen)?"true":"false",G.zenMode?"true":"false",G.railOpen?"true":"false",G.leftHanded?"true":"false");aui_append(b);
    if(write_checkpoint_away())aui_button("Return",UI_RETURN_WRITE);const char*headers[]={"Search","Undo","Redo","Projects","Focus"};int ids[]={UI_SEARCH,UI_UNDO,UI_REDO,UI_PROJECTS,UI_FOCUS};for(int i=0;i<5;i++){int enabled=i==1?(G.actionCursor>0&&!action_hits_locked_layer(&G.actions[G.actionCursor-1])):i==2?(G.actionCursor<G.actionN&&!action_hits_locked_layer(&G.actions[G.actionCursor])):1;aui_row("button",headers[i],ids[i],0,enabled);}
    aui_append("],\"tools\":[");AUI_ROWS=0;const char*labels[]={"Add","Frames","Photos","Measure","Search","Layers","Map","Settings"};for(int i=0;i<8;i++)aui_row("button",labels[i],rail_ui(i),rail_active(i),1);
    aui_append("],\"context\":[");AUI_ROWS=0;if(G.selectedRefN>0){int single=G.selectedRefN==1,t=G.selectedRefs[0].type;const char*first=!single?"Deselect":t==SEL_NOTE?"Edit":t==SEL_FRAME?"Frames":t==SEL_MEASURE?"Dimensions":t==SEL_IMAGE?"Photo":"Color";aui_row("button",first,single?UI_SELECTION_EDIT:UI_SELECTION_DESELECT,0,!single||objref_can_mutate(G.selectedRefs[0]));aui_button("Duplicate",UI_SELECTION_DUP);aui_button(G.selectionType==SEL_GROUP?"Ungroup":single?"More":"Group",single?UI_SELECTION_MORE:UI_SELECTION_GROUP);aui_row("danger","Delete",UI_SELECTION_DELETE,0,selection_has_mutable_object());}
    aui_append("],\"tool\":[");AUI_ROWS=0;
    if(G.photoMode){aui_text_row("Photo");aui_button(G.photoAngleSnap?"Angle snap: 15°":"Angle snap: Off",UI_PHOTO_SNAP);if(G.selectedImage>=0)aui_danger("Delete photo",UI_PHOTO_DELETE,!G.lockPhotos);else aui_text_row("Hold a photo to select it.");aui_button("Done",UI_PHOTO_DONE);}
    else if(G.tool==MODE_ERASE||G.tool==MODE_ERASE_ALL){aui_text_row("Eraser tools");aui_row("choice","Stroke eraser",UI_ERASE,G.tool==MODE_ERASE,1);aui_row("choice","Erase everything",UI_ERASE_ALL,G.tool==MODE_ERASE_ALL,1);aui_text_row(G.tool==MODE_ERASE_ALL?"Removes unlocked ink, highlighter, photos, dimensions, text, shapes, and frame borders.":"Removes complete ink and highlighter strokes touched by the pen.");aui_button("Done",UI_PEN);}
    else if(G.tool==MODE_MEASURE){aui_text_row("Dimensions");aui_button(G.snap?"Grid snap: On":"Grid snap: Off",UI_SNAP);aui_row("button","Calibrate",UI_CALIBRATE,0,G.selectedMeasure>=0&&!G.lockCAD);if(G.selectedMeasure>=0){char val[80];snprintf(val,sizeof(val),"%.2f %s",measure_display_value(&G.measures[G.selectedMeasure]),G.cadCalibrated?cad_unit_name():"canvas units");aui_text_row(val);aui_danger("Delete dimension",UI_DELETE_DIM,!G.lockCAD);}else aui_text_row("Draw a dimension, then select it to set its real length.");aui_button("Done",UI_PEN);}
    else if(G.tool==MODE_SHAPE){aui_text_row("Shape");aui_row("choice","Line",UI_SHAPE_LINE,G.shapeType==0,1);aui_row("choice","Rectangle",UI_SHAPE_RECT,G.shapeType==1,1);aui_row("choice","Ellipse",UI_SHAPE_ELLIPSE,G.shapeType==2,1);aui_row("choice","Arrow",UI_SHAPE_ARROW,G.shapeType==3,1);aui_button("Done",UI_SHAPE_DONE);}
    else if(G.pressurePanel){aui_text_row("Pressure response");aui_row("choice","Soft",UI_CURVE_SOFT,G.pressureCurve==0,1);aui_row("choice","Linear",UI_CURVE_LINEAR,G.pressureCurve==1,1);aui_row("choice","Firm",UI_CURVE_FIRM,G.pressureCurve==2,1);aui_button("Done",UI_PRESSURE);}
    aui_append("]}");
    JNIEnv*e=G.activity->env;jmethodID method=(*e)->GetStaticMethodID(e,AUI,"showChrome","(Landroid/app/Activity;Ljava/lang/String;I)V");jstring json=aui_string(e,AUI_JSON);
    if(method&&json)(*e)->CallStaticVoidMethod(e,AUI,method,G.activity->clazz,json,(jint)C_BG);if(json)(*e)->DeleteLocalRef(e,json);jni_clear(e);
}
static uint64_t AUI_COLOR_HASH;
static void android_color_sync(void){
    int open=G.colorPickerOpen&&!android_ui_modal()&&!G.presentationMode;
    uint64_t hash=nui_hash_mix(G.color,(uint32_t)open);hash=nui_hash_mix(hash,C_BG);hash=nui_hash_mix(hash,G.screenW);hash=nui_hash_mix(hash,G.screenH);for(int i=0;i<G.recentColorN;i++)hash=nui_hash_mix(hash,G.recentColors[i]);
    if(hash==AUI_COLOR_HASH)return;AUI_COLOR_HASH=hash;
    AUI_LEN=AUI_ROWS=0;char b[180];snprintf(b,sizeof(b),"{\"open\":%s,\"color\":%u,\"previous\":%u,\"close\":%d,\"recent\":[",open?"true":"false",G.color,G.pickerSessionStartColor,UI_PICKER_DONE);aui_append(b);
    for(int i=0;i<G.recentColorN&&i<6;i++){snprintf(b,sizeof(b),"%s%u",i?",":"",G.recentColors[i]);aui_append(b);}aui_append("]}");
    JNIEnv*e=G.activity->env;jmethodID m=(*e)->GetStaticMethodID(e,AUI,"showColorPicker","(Landroid/app/Activity;Ljava/lang/String;I)V");jstring json=aui_string(e,AUI_JSON);if(m&&json)(*e)->CallStaticVoidMethod(e,AUI,m,G.activity->clazz,json,(jint)C_BG);if(json)(*e)->DeleteLocalRef(e,json);jni_clear(e);
}
static void android_ui_sync(void){
    if(!android_ui_ready())return;android_color_sync();android_chrome_sync();if(G.stylusDown)return;
    if(G.editorOpen||G.calibrationOpen){AUI_PANEL_HASH=0;return;}
    if(G.selectionMoreOpen){aui_begin("Selection",UI_SELECTION_MORE);if(selection_single_colorable())aui_button("Color",UI_SELECTION_COLOR);if(selection_single_lockable())aui_button(selection_single_locked()?"Unlock":"Lock",UI_SELECTION_LOCK);aui_button("Deselect",UI_SELECTION_DESELECT);
    }else if(G.settingsPanel&&G.settingsTab==5){
        aui_begin("Customize theme",UI_SETTINGS_CLOSE);aui_button("Back to Settings",5001);
        const char*roles[]={"Background","Panels","Grid","Text","Accent","Dimensions","Secondary accent","Destructive actions"};
        for(int i=0;i<8;i++)aui_row("nav",roles[i],UI_ROLE0+i,G.themeRole==i,1);
        aui_heading("Choose a color");for(int i=0;i<12;i++){char label[40];snprintf(label,sizeof(label),"#%06X",THEME_PALETTE[i]);aui_row("swatch",label,UI_PALETTE0+i,0,1);}aui_button("Reset theme",UI_THEME_RESET);
    }else if(G.settingsPanel){
        aui_begin("Settings",UI_SETTINGS_CLOSE);
        const char*sections[]={"Appearance","Stylus button","Drawing","Canvas","Handwriting Search"};
        int sectionIds[]={UI_SETTINGS_APPEARANCE,UI_SETTINGS_STYLUS,UI_SETTINGS_DRAWING,UI_SETTINGS_DISPLAY,UI_SETTINGS_HANDWRITING};
        int order[]={2,3,4,0,1};for(int j=0;j<5;j++){int i=order[j];aui_row("nav",sections[i],sectionIds[i],G.settingsTab==i,1);}
        if(G.settingsTab==2){
        aui_heading("Drawing");aui_percent("Pressure smoothing",G.pressureSmoothing,UI_PSMOOTH_MINUS,UI_PSMOOTH_PLUS);
        const char*curves[]={"Soft","Linear","Firm"};int curveIds[]={UI_CURVE_SOFT,UI_CURVE_LINEAR,UI_CURVE_FIRM};int curve=maxi(0,mini(2,G.pressureCurve));aui_step("Pressure response",curves[curve],curveIds[(curve+2)%3],curveIds[(curve+1)%3]);
        aui_percent("Stroke smoothing",G.strokeSmoothing,UI_SSMOOTH_MINUS,UI_SSMOOTH_PLUS);
        aui_percent("Highlighter opacity",G.highlighterOpacity,UI_HILITE_MINUS,UI_HILITE_PLUS);
        char pressure[32];snprintf(pressure,sizeof(pressure),"%.2f",G.pressureMin);aui_step("Minimum pressure",pressure,UI_PMIN_MINUS,UI_PMIN_PLUS);snprintf(pressure,sizeof(pressure),"%.2f",G.pressureMax);aui_step("Maximum pressure",pressure,UI_PMAX_MINUS,UI_PMAX_PLUS);
        char value[40];snprintf(value,sizeof(value),"%.1f s",G.radialHoldSec);aui_step("Hold for tools",value,UI_RADIAL_MINUS,UI_RADIAL_PLUS);
        }else if(G.settingsTab==4){aui_heading("Handwriting Search");aui_toggle("Offline handwriting search",UI_OCR_TOGGLE,G.ocrEnabled);
        aui_text_row("Search text you've written by hand. Recognition runs entirely on this device and does not change your handwriting. Nothing is uploaded.");
        OcrManagerStatus status;aui_heading("Status");aui_text_row(ocr_status_name(&status));aui_button("Rebuild handwriting index",UI_OCR_REBUILD);
        }else if(G.settingsTab==3){aui_heading("Canvas");aui_step("Grid",grid_style_name(),UI_DISP0_MINUS,UI_DISP0_PLUS);
        aui_step("Zoomed-out view",far_zoom_name(),UI_DISP1_MINUS,UI_DISP1_PLUS);aui_heading("Accessibility");aui_toggle("Left-handed controls",6500,G.leftHanded);aui_heading("Advanced");aui_step("Rendering quality",perf_name(),UI_DISP2_MINUS,UI_DISP2_PLUS);aui_heading("Updates");aui_text_row("Vast checks the official imag0/Vast repository periodically. Every APK is checksum-verified and must match this app's signing certificate before Android can install it.");aui_button("Check for updates",5400);
        }else if(G.settingsTab==0){aui_heading("Appearance");aui_percent("Canvas controls",G.uiScale,UI_SCALE_MINUS,UI_SCALE_PLUS);
        aui_heading("Essentials");for(int i=0;i<5;i++)aui_row("choice",theme_preset_name(i),UI_PRESET0+i,theme_is_preset(i),1);
        aui_heading("Dark collection");for(int i=5;i<12;i++)aui_row("choice",theme_preset_name(i),UI_PRESET0+i,theme_is_preset(i),1);
        aui_heading("Light collection");for(int i=12;i<17;i++)aui_row("choice",theme_preset_name(i),UI_PRESET0+i,theme_is_preset(i),1);
        aui_heading("Chromatic collection");for(int i=17;i<THEME_PRESET_COUNT;i++)aui_row("choice",theme_preset_name(i),UI_PRESET0+i,theme_is_preset(i),1);
        aui_button("Customize theme",5000);
        }else if(G.settingsTab==1){aui_heading("Stylus button");aui_toggle("Use stylus button",UI_BUTTONS_TOGGLE,G.buttonBindingsEnabled);
        if(G.buttonBindingsEnabled){
        aui_step("Press",button_action_name(G.buttonPressAction),UI_PRESS_MINUS,UI_PRESS_PLUS);
        aui_step("Hold",button_action_name(G.buttonHoldAction),UI_HOLD_MINUS,UI_HOLD_PLUS);
        aui_step("Double press",button_action_name(G.buttonDoubleAction),UI_DOUBLE_MINUS,UI_DOUBLE_PLUS);}}
    }else if(G.galleryOpen){aui_begin("Import photos",UI_GALLERY_CLOSE);if(!G.thumbN)aui_text_row("No photos found.");for(int i=0;i<G.thumbN;i++){char name[40];snprintf(name,sizeof(name),"Photo %d",G.galleryPage*12+i+1);aui_row("photo",name,UI_GALLERY_ITEM0+i,i,1);}if(G.galleryPage>0)aui_button("Previous",UI_GALLERY_PREV);if((G.galleryPage+1)*12<G.mediaCount)aui_button("Next",UI_GALLERY_NEXT);
    }else if(G.projectsPanel){aui_begin("Projects",UI_PROJECT_CLOSE);aui_heading("Project library");for(int pass=0;pass<2;pass++)for(int i=0;i<MAX_PROJECTS;i++){int used=project_slot_has_data(i)||i==G.projectIndex;if(!used||G.projectArchived[i]||(pass==0&&!G.projectFavorite[i])||(pass==1&&G.projectFavorite[i]))continue;char label[128];snprintf(label,sizeof(label),"%s%s%s%s",G.projectFavorite[i]?"★  ":"",G.projectNames[i],G.projectTags[i][0]?"  ·  ":"",G.projectTags[i]);aui_row("choice",label,6000+i,i==G.projectIndex,1);}aui_button("New project",6100);aui_heading("Current project");aui_button("Rename",UI_PROJECT_RENAME);aui_button("Edit tags",6107);aui_button(G.projectFavorite[G.projectIndex]?"Remove favorite":"Add favorite",6105);aui_button(G.projectArchived[G.projectIndex]?"Unarchive":"Archive",6106);aui_button("Duplicate",UI_PROJECT_DUP);aui_heading("Backup and transfer");aui_button("Export .vast project",6101);aui_button("Import .vast project",6102);aui_button("Create recovery snapshot",6103);aui_danger("Restore latest snapshot",6104,1);aui_heading("Danger zone");aui_danger("Clear project",UI_PROJECT_CLEAR,1);
    }else if(G.addPanel){aui_begin("Add to canvas",UI_ADD_CLOSE);aui_heading("Content");aui_button("Text",UI_ADD_TEXT);aui_button("Sticky note",UI_ADD_STICKY);aui_button("Shape",UI_ADD_SHAPE);aui_button("Photo",UI_ADD_PHOTO);aui_heading("Organization");aui_button("Frame",UI_ADD_FRAME);aui_button("Save this place",UI_ADD_BOOKMARK);
    }else if(G.layersPanel){aui_begin("Layers",UI_LAYERS_CLOSE);
        const char*names[]={"Ink","Highlighter","Photos","Dimensions","Text and notes","Shapes","Frames"};
        int visible[]={G.visInk,G.visMarker,G.visPhotos,G.visCAD,G.visNotes,G.visShapes,G.visFrames};
        int locks[]={G.lockInk,G.lockMarker,G.lockPhotos,G.lockCAD,G.lockNotes,G.lockShapes,G.lockFrames};
        int ids[]={UI_LAYER_INK,UI_LAYER_MARKER,UI_LAYER_PHOTOS,UI_LAYER_CAD,UI_LAYER_NOTES,UI_LAYER_SHAPES,UI_LAYER_FRAMES};
        for(int i=0;i<7;i++){char b[180];if(AUI_ROWS++)aui_append(",");aui_append("{\"kind\":\"layer\",\"text\":");aui_quote(names[i]);snprintf(b,sizeof(b),",\"id\":%d,\"checked\":%s,\"lockId\":%d,\"locked\":%s}",ids[i],visible[i]?"true":"false",UI_LOCK_INK+i,locks[i]?"true":"false");aui_append(b);}
    }else if(G.searchPanel){aui_begin("Search",UI_SEARCH_CLOSE);aui_button(G.searchQuery[0]?G.searchQuery:"Search canvas",UI_SEARCH_EDIT);G.searchVisibleN=search_collect(G.searchVisible,6);if(!G.searchVisibleN)aui_text_row("No matching notes, frames, places or handwriting.");for(int i=0;i<G.searchVisibleN;i++){char label[256];search_result_label(G.searchVisible[i],label,sizeof(label));aui_button(label,UI_SEARCH_RESULT0+i);}
    }else if(G.framePanel){aui_begin("Frames and places",UI_FRAME_CLOSE);
        aui_row("nav","Frames",5200,AUI_FRAME_SECTION==0,1);aui_row("nav","Saved places",5201,AUI_FRAME_SECTION==1,1);
        if(AUI_FRAME_SECTION==0){aui_button("New frame",UI_FRAME_NEW);aui_heading("Frames");
        for(int i=0;i<5;i++){int fi=G.framePage*5+i;if(fi<G.frameN)aui_row("choice",G.frameNames[fi],UI_FRAME_ITEM0+i,fi==G.selectedFrame,1);}
        if(G.framePage>0)aui_button("Previous frames",UI_FRAME_PREV);if((G.framePage+1)*5<G.frameN)aui_button("Next frames",UI_FRAME_NEXT);
        int f=G.selectedFrame;if(f>=0&&f<G.frameN){aui_heading(G.frameNames[f]);aui_button("Go to frame",UI_FRAME_JUMP);aui_row("button","Rename",UI_FRAME_RENAME,0,!G.lockFrames&&!G.frameLocked[f]);aui_row("button","Move with contents",UI_FRAME_FIT,0,!G.lockFrames&&!G.frameLocked[f]);aui_heading("Properties");aui_toggle("Locked",UI_FRAME_LOCK,G.frameLocked[f]);aui_toggle("Lock size",UI_FRAME_SIZE_LOCK,G.frameSizeLocked[f]);aui_heading("Organize and share");aui_button("Duplicate",UI_FRAME_DUP);aui_button("Export image",UI_FRAME_EXPORT);aui_button("Present frames",UI_FRAME_PRESENT);aui_heading("Remove");aui_danger("Delete frame",UI_FRAME_DELETE,!G.lockFrames&&!G.frameLocked[f]);}
        }else{aui_heading("Saved places");for(int i=0;i<G.bookmarkN;i++){aui_heading(G.bookmarks[i].name);aui_button("Go to place",5100+i*3);aui_button("Rename place",5101+i*3);aui_danger("Delete place",5102+i*3,1);}if(!G.bookmarkN)aui_text_row("Use Add → Save this place to keep a view.");}
    }else{AUI_JSON[0]=0;AUI_LEN=0;}
    if(AUI_LEN)aui_append("]}");
    uint64_t hash=aui_json_hash(C_BG);if(hash==AUI_PANEL_HASH)return;AUI_PANEL_HASH=hash;
    JNIEnv*e=G.activity->env;jmethodID show=(*e)->GetStaticMethodID(e,AUI,"showPanel","(Landroid/app/Activity;Ljava/lang/String;I)V");jstring json=aui_string(e,AUI_JSON);
    if(show&&json)(*e)->CallStaticVoidMethod(e,AUI,show,G.activity->clazz,json,(jint)C_BG);if(json)(*e)->DeleteLocalRef(e,json);jni_clear(e);
}
