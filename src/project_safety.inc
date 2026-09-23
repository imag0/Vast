/* Vast 4 project portability and recovery.  Packages contain the exact six
 * versioned project files, so export/import never translates user geometry. */
typedef struct __attribute__((packed)) { char magic[8]; uint32_t version,count; uint64_t totalBytes; char name[32],tags[64]; uint32_t flags; } VastPackageHead;
typedef struct __attribute__((packed)) { uint32_t kind,checksum; uint64_t bytes; } VastPackageEntry;
typedef struct __attribute__((packed)) { char magic[4],name[32],tags[64]; uint32_t flags; } VastSnapshotHead;

static uint32_t project_checksum_stream(void*f,uint64_t bytes){
    uint8_t*b=(uint8_t*)malloc(65536);uint32_t h=2166136261u;if(!b)return 0;
    while(bytes){size_t want=(size_t)(bytes>65536?65536:bytes),n=fread(b,1,want,f);if(n!=want){h=0;break;}for(size_t i=0;i<n;i++){h^=b[i];h*=16777619u;}bytes-=n;}
    free(b);return h;
}
static uint64_t project_file_size(void*f){if(!f||fseek(f,0,SEEK_END_LOCAL)!=0)return 0;long n=ftell(f);fseek(f,0,SEEK_SET_LOCAL);return n>0?(uint64_t)n:0;}
static int project_package_export_to(const char*path){
    if(!path||!path[0]||!save_all_document())return 0;void*out=fopen(path,"wb");if(!out)return 0;
    VastPackageHead h;memset(&h,0,sizeof(h));memcpy(h.magic,"VASTPKG",7);h.version=1;copy_text_local(h.name,32,G.projectNames[G.projectIndex]);copy_text_local(h.tags,64,G.projectTags[G.projectIndex]);h.flags=(G.projectFavorite[G.projectIndex]?1u:0u)|(G.projectArchived[G.projectIndex]?2u:0u);
    if(fwrite(&h,sizeof(h),1,out)!=1){fclose(out);remove(path);return 0;}int ok=1;
    for(int k=0;k<6;k++){char src[512];project_path_for(G.projectIndex,k,src,sizeof(src));void*in=fopen(src,"rb");if(!in)continue;uint64_t bytes=project_file_size(in);uint32_t checksum=project_checksum_stream(in,bytes);fseek(in,0,SEEK_SET_LOCAL);if(!checksum&&bytes){fclose(in);ok=0;break;}VastPackageEntry e={(uint32_t)k,checksum,bytes};if(fwrite(&e,sizeof(e),1,out)!=1){fclose(in);ok=0;break;}uint8_t*b=(uint8_t*)malloc(65536);if(!b){fclose(in);ok=0;break;}uint64_t left=bytes;while(left){size_t want=(size_t)(left>65536?65536:left),n=fread(b,1,want,in);if(n!=want||fwrite(b,1,n,out)!=n){ok=0;break;}left-=n;}free(b);fclose(in);if(!ok)break;h.count++;h.totalBytes+=bytes;}
    if(ok&&fseek(out,0,SEEK_SET_LOCAL)==0&&fwrite(&h,sizeof(h),1,out)==1&&fflush(out)==0&&fsync(fileno(out))==0){if(fclose(out)==0)return 1;out=0;}
    if(out)fclose(out);remove(path);return 0;
}
static void project_snapshot_path(int project,int slot,int kind,char*out,int cap){snprintf(out,(size_t)cap,"%s/project_%d.snapshot_%d.%d",G.activity->internalDataPath,project,slot,kind);}
static void project_snapshot_ready_path(int project,int slot,char*out,int cap){snprintf(out,(size_t)cap,"%s/project_%d.snapshot_%d.ready",G.activity->internalDataPath,project,slot);}
static void project_snapshot_maybe(int force){
    if(!G.activity||!G.activity->internalDataPath||!project_slot_has_data(G.projectIndex))return;int64_t now=monotonic_ms();if(!force&&G.projectLastSnapshotMs[G.projectIndex]&&now-G.projectLastSnapshotMs[G.projectIndex]<120000)return;
    int slot=G.projectSnapshotCursor[G.projectIndex]%PROJECT_SNAPSHOT_COUNT;char ready[544];project_snapshot_ready_path(G.projectIndex,slot,ready,sizeof(ready));remove(ready);int ok=1;
    for(int k=0;k<6;k++){char src[512],dst[544];project_path_for(G.projectIndex,k,src,sizeof(src));project_snapshot_path(G.projectIndex,slot,k,dst,sizeof(dst));remove(dst);if(file_exists_local(src)&&link(src,dst)!=0&&!copy_file_local(src,dst)){ok=0;break;}}
    if(ok){char tmp[576];void*f=atomic_write_begin(ready,tmp,sizeof(tmp));VastSnapshotHead h;memset(&h,0,sizeof(h));memcpy(h.magic,"VSS1",4);copy_text_local(h.name,32,G.projectNames[G.projectIndex]);copy_text_local(h.tags,64,G.projectTags[G.projectIndex]);h.flags=(G.projectFavorite[G.projectIndex]?1u:0u)|(G.projectArchived[G.projectIndex]?2u:0u);if(!f||atomic_write_finish(f,tmp,ready,fwrite(&h,sizeof(h),1,f)==1)==ATOMIC_WRITE_FAILED)ok=0;}
    if(!ok){for(int k=0;k<6;k++){char dst[544];project_snapshot_path(G.projectIndex,slot,k,dst,sizeof(dst));remove(dst);}remove(ready);return;}
    G.projectSnapshotCursor[G.projectIndex]=(uint8_t)((slot+1)%PROJECT_SNAPSHOT_COUNT);G.projectLastSnapshotMs[G.projectIndex]=now;save_project_prefs();
}
static void project_restore_marker_path(int project,char*out,int cap){snprintf(out,(size_t)cap,"%s/project_%d.restore.pending",G.activity->internalDataPath,project);}
static int project_recover_incomplete_restore(int project){
    char marker[544];project_restore_marker_path(project,marker,sizeof(marker));void*f=fopen(marker,"rb");
    if(!f){/* A committed restore may leave harmless backup files after a power loss. */for(int k=0;k<6;k++){char dst[512],old[544],fresh[544];project_path_for(project,k,dst,sizeof(dst));snprintf(old,sizeof(old),"%s.restore-old",dst);snprintf(fresh,sizeof(fresh),"%s.restore-new",dst);remove(old);remove(fresh);}return 1;}
    uint8_t originalMask=0;if(fread(&originalMask,1,1,f)!=1){fclose(f);return 0;}fclose(f);int ok=1;
    for(int k=0;k<6;k++){char dst[512],old[544],fresh[544];project_path_for(project,k,dst,sizeof(dst));snprintf(old,sizeof(old),"%s.restore-old",dst);snprintf(fresh,sizeof(fresh),"%s.restore-new",dst);if(originalMask&(1u<<k)){if(file_exists_local(old)){if(!remove_file_if_present(dst)||rename(old,dst)!=0)ok=0;}}else if(!remove_file_if_present(dst))ok=0;if(!remove_file_if_present(fresh))ok=0;}
    if(!fsync_parent_directory(marker))ok=0;if(!ok)return 0;if(!remove_file_if_present(marker))return 0;return fsync_parent_directory(marker);
}
static int project_restore_latest(void){
    int slot=(G.projectSnapshotCursor[G.projectIndex]+PROJECT_SNAPSHOT_COUNT-1)%PROJECT_SNAPSHOT_COUNT;char ready[544];project_snapshot_ready_path(G.projectIndex,slot,ready,sizeof(ready));void*rf=fopen(ready,"rb");VastSnapshotHead sh;if(!rf||fread(&sh,sizeof(sh),1,rf)!=1||memcmp(sh.magic,"VSS1",4)!=0){if(rf)fclose(rf);return 0;}fclose(rf);
    if(!project_recover_incomplete_restore(G.projectIndex))return 0;char stage[6][544],backup[6][544],dst[6][512],marker[544],markerTmp[576];int present[6]={0};uint8_t originalMask=0;for(int k=0;k<6;k++){char src[544];project_snapshot_path(G.projectIndex,slot,k,src,sizeof(src));project_path_for(G.projectIndex,k,dst[k],sizeof(dst[k]));snprintf(stage[k],sizeof(stage[k]),"%s.restore-new",dst[k]);snprintf(backup[k],sizeof(backup[k]),"%s.restore-old",dst[k]);remove(stage[k]);remove(backup[k]);if(file_exists_local(dst[k]))originalMask|=(uint8_t)(1u<<k);if(file_exists_local(src)){present[k]=1;if(!copy_file_local(src,stage[k]))goto fail;}}
    project_restore_marker_path(G.projectIndex,marker,sizeof(marker));{void*mf=atomic_write_begin(marker,markerTmp,sizeof(markerTmp));int committed=mf?atomic_write_finish(mf,markerTmp,marker,fwrite(&originalMask,1,1,mf)==1):ATOMIC_WRITE_FAILED;if(committed!=ATOMIC_WRITE_DURABLE)goto fail;}
    for(int k=0;k<6;k++){if(originalMask&(1u<<k)){if(rename(dst[k],backup[k])!=0)goto rollback;}if(present[k]&&rename(stage[k],dst[k])!=0)goto rollback;}
    if(!fsync_parent_directory(marker)||remove(marker)!=0||!fsync_parent_directory(marker))goto rollback;for(int k=0;k<6;k++){remove(backup[k]);remove(stage[k]);}fsync_parent_directory(dst[0]);copy_text_local(G.projectNames[G.projectIndex],32,sh.name);copy_text_local(G.projectTags[G.projectIndex],64,sh.tags);G.projectFavorite[G.projectIndex]=(sh.flags&1)!=0;G.projectArchived[G.projectIndex]=(sh.flags&2)!=0;save_project_prefs();ocr_close_current_project(0);clear_document_memory();set_project_paths(G.projectIndex);load_canvas();load_meta();load_images();load_frames();load_workspace();ocr_open_current_project();if(!G.viewInitialized)reset_view();G.projectsPanel=0;G.minimapDirty=1;return 1;
rollback:(void)project_recover_incomplete_restore(G.projectIndex);return 0;
fail:for(int k=0;k<6;k++){remove(stage[k]);remove(backup[k]);}return 0;
}
static int project_import_archive(const char*path){
    void*in=fopen(path,"rb");if(!in)return -1;VastPackageHead h;if(fread(&h,sizeof(h),1,in)!=1||memcmp(h.magic,"VASTPKG",7)!=0||h.version!=1||h.count>6||h.totalBytes>(uint64_t)1024*1024*1024){fclose(in);return -1;}int dst=-1;for(int i=0;i<MAX_PROJECTS;i++)if(!project_slot_has_data(i)){dst=i;break;}if(dst<0){fclose(in);return -2;}
    char stage[6][544],out[6][512];int seen[6]={0};memset(stage,0,sizeof(stage));uint64_t total=0;int ok=1;
    for(uint32_t n=0;n<h.count;n++){VastPackageEntry e;if(fread(&e,sizeof(e),1,in)!=1||e.kind>=6||seen[e.kind]||e.bytes>(uint64_t)1024*1024*1024||total+e.bytes>h.totalBytes){ok=0;break;}seen[e.kind]=1;project_path_for(dst,(int)e.kind,out[e.kind],sizeof(out[e.kind]));snprintf(stage[e.kind],sizeof(stage[e.kind]),"%s.dup",out[e.kind]);remove(stage[e.kind]);void*f=fopen(stage[e.kind],"wb");uint8_t*b=(uint8_t*)malloc(65536);uint32_t sum=2166136261u;uint64_t left=e.bytes;if(!f||!b){if(f)fclose(f);free(b);ok=0;break;}while(left){size_t want=(size_t)(left>65536?65536:left),got=fread(b,1,want,in);if(got!=want||fwrite(b,1,got,f)!=got){ok=0;break;}for(size_t i=0;i<got;i++){sum^=b[i];sum*=16777619u;}left-=got;}free(b);if(fflush(f)!=0||fsync(fileno(f))!=0||fclose(f)!=0)ok=0;if(!ok||sum!=e.checksum){ok=0;break;}total+=e.bytes;}
    fclose(in);if(!ok||total!=h.totalBytes||h.count==0)goto fail;char marker[544],markerTmp[576];project_duplicate_marker_path(dst,marker,sizeof(marker));{void*mf=atomic_write_begin(marker,markerTmp,sizeof(markerTmp));uint8_t one=1;int committed=mf?atomic_write_finish(mf,markerTmp,marker,fwrite(&one,1,1,mf)==1):ATOMIC_WRITE_FAILED;if(committed!=ATOMIC_WRITE_DURABLE)goto fail;}for(int k=0;k<6;k++)if(seen[k]&&rename(stage[k],out[k])!=0)goto rollback;if(!fsync_parent_directory(marker)||remove(marker)!=0||!fsync_parent_directory(marker))goto rollback;copy_text_local(G.projectNames[dst],32,h.name[0]?h.name:"Imported project");copy_text_local(G.projectTags[dst],64,h.tags);G.projectFavorite[dst]=(h.flags&1)!=0;G.projectArchived[dst]=(h.flags&2)!=0;save_project_prefs();remove(path);switch_project(dst);return dst;
rollback:(void)project_recover_incomplete_duplicate(dst);
fail:for(int k=0;k<6;k++)if(stage[k][0])remove(stage[k]);remove(path);return -1;
}
static void project_file_dialog(int mode,const char*source,const char*name,const char*pending){
    if(!android_ui_ready())return;JNIEnv*e=G.activity->env;jmethodID m=(*e)->GetStaticMethodID(e,AUI,"launchFileTransfer","(Landroid/app/Activity;ILjava/lang/String;Ljava/lang/String;Ljava/lang/String;)V");jstring a=aui_string(e,source?source:""),b=aui_string(e,name?name:"Vast project.vast"),c=aui_string(e,pending?pending:"");if(m&&a&&b&&c)(*e)->CallStaticVoidMethod(e,AUI,m,G.activity->clazz,(jint)mode,a,b,c);if(a)(*e)->DeleteLocalRef(e,a);if(b)(*e)->DeleteLocalRef(e,b);if(c)(*e)->DeleteLocalRef(e,c);jni_clear(e);
}
static int project_request_export(void){if(!G.activity||!G.activity->externalDataPath)return 0;char path[544],name[96];snprintf(path,sizeof(path),"%s/Vast-project-%d.vast",G.activity->externalDataPath,G.projectIndex+1);snprintf(name,sizeof(name),"%.64s.vast",G.projectNames[G.projectIndex]);if(!project_package_export_to(path))return 0;project_file_dialog(1,path,name,"");return 1;}
static void project_request_import(void){if(!G.activity||!G.activity->internalDataPath)return;char pending[544],error[576];snprintf(pending,sizeof(pending),"%s/project-import.pending",G.activity->internalDataPath);snprintf(error,sizeof(error),"%s.error",pending);remove(pending);remove(error);project_file_dialog(2,"","Vast project",pending);}
static void project_check_pending_import(void){if(!G.activity||!G.activity->internalDataPath)return;char pending[544],error[576];snprintf(pending,sizeof(pending),"%s/project-import.pending",G.activity->internalDataPath);snprintf(error,sizeof(error),"%s.error",pending);if(file_exists_local(error)){remove(error);set_toast(8,monotonic_ms());}else if(file_exists_local(pending)){int r=project_import_archive(pending);set_toast(r>=0?11:8,monotonic_ms());}}
static int project_create_new(void){int dst=-1;for(int i=0;i<MAX_PROJECTS;i++)if(!project_slot_has_data(i)){dst=i;break;}if(dst<0)return -1;snprintf(G.projectNames[dst],32,"Project %d",dst+1);G.projectTags[dst][0]=0;G.projectFavorite[dst]=G.projectArchived[dst]=0;save_project_prefs();switch_project(dst);return dst;}
