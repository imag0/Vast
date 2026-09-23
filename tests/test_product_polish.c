#define main legacy_main_not_run
#define check legacy_check_not_run
#include "../test_v30.c"
#undef main
#undef check
static int checks,failures;
static void check(int value,const char*name){checks++;printf("%s %s\n",value?"PASS":"FAIL",name);if(!value)failures++;}
int main(void){
    memset(&G,0,sizeof(G));G.scale=G.uiScale=1;G.animTimerFd=-1;G.noteN=1;G.editorTarget=-1;
    NoteObj*n=&G.notes[0];n->active=1;n->id=73;n->type=NOTE_TEXT;copy_text_local(n->text,NOTE_TEXT_CAP,"Hi");note_autosize(n);float shortW=n->w;
    copy_text_local(n->text,NOTE_TEXT_CAP,"A somewhat longer note");note_autosize(n);check(n->w>shortW,"short text grows naturally");
    char paragraph[NOTE_TEXT_CAP];int len=0;for(int i=0;i<25;i++)len+=snprintf(paragraph+len,sizeof(paragraph)-len,"Line %d: English, Chinese 世界 and emoji 😀.\n",i);
    copy_text_local(n->text,NOTE_TEXT_CAP,paragraph);note_autosize(n);check(n->w<=504&&n->h>420,"long multiline text grows vertically without old clipping cap");
    float longH=n->h;copy_text_local(n->text,NOTE_TEXT_CAP,"Hi");note_autosize(n);check(n->h<longH&&n->w==shortW,"deleting text shrinks the object");
    editor_open(1,0,n->text);copy_text_local(G.editorBuf,NOTE_TEXT_CAP,paragraph);editor_live_apply();check(str_len_local(n->text)==len,"live note editing preserves paragraphs beyond 95 bytes");editor_cancel();check(text_equal_local(n->text,"Hi"),"cancel restores the original note");
    editor_open(1,0,n->text);copy_text_local(G.editorBuf,NOTE_TEXT_CAP,paragraph);editor_apply();check(text_equal_local(n->text,paragraph),"save preserves full UTF-8 paragraphs");
    snprintf(G.workspacePath,sizeof(G.workspacePath),"/tmp/vast-polish-%lld.v3",monotonic_ms());check(save_workspace()!=0,"extended workspace saves atomically");
    char path[512];copy_text_local(path,sizeof(path),G.workspacePath);memset(G.notes,0,sizeof(G.notes));G.noteN=0;load_workspace();check(G.noteN==1&&G.notes[0].id==73&&text_equal_local(G.notes[0].text,paragraph),"extended text survives reload");
    void*f=fopen(path,"rb");WorkspaceHead h;LegacyNoteObj preview;int ok=f&&fread(&h,sizeof(h),1,f)==1&&fread(&preview,sizeof(preview),1,f)==1;if(f)fclose(f);check(ok&&h.magic[3]=='1'&&preview.id==73&&preview.text[95]==0,"legacy VWS1 prefix remains readable");remove(path);
    jchar u16[NOTE_TEXT_CAP];char back[NOTE_TEXT_CAP];int units=copy_utf8_to_utf16(u16,NOTE_TEXT_CAP,paragraph);copy_utf16_to_utf8(back,sizeof(back),u16,units);check(text_equal_local(back,paragraph),"long Chinese and supplementary characters survive JNI conversion");
    G.settingsPanel=1;G.settingsTab=0;aui_begin("Quote \" and newline\n",UI_SETTINGS_CLOSE);aui_text_row("世界 \\ 😀");aui_append("]}");check(str_len_local(AUI_JSON)>40,"native panel JSON accepts Unicode and escapes");
    G.selectedRefN=1;G.selectedRefs[0]=(ObjRef){SEL_NOTE,0};G.selectedNote=0;handle_ui(UI_SELECTION_MORE,1);check(G.selectionMoreOpen,"More opens");handle_ui(UI_SELECTION_LOCK,2);check(G.notes[0].locked&&!G.selectionMoreOpen,"More lock works and closes");handle_ui(UI_SELECTION_LOCK,3);check(!G.notes[0].locked,"More unlock works");
    G.searchQuery[0]=0;copy_text_local(G.searchQuery,sizeof(G.searchQuery),"世界");ObjRef results[6];check(search_collect(results,6)>0,"Chinese note is searchable after reload");
    aui_begin("Settings",UI_SETTINGS_CLOSE);aui_text_row("A sufficiently long unchanged prefix to put the updated field beyond sixty-four bytes.");aui_toggle("Visible",1,0);aui_append("]}");uint64_t h0=aui_json_hash(1);
    aui_begin("Settings",UI_SETTINGS_CLOSE);aui_text_row("A sufficiently long unchanged prefix to put the updated field beyond sixty-four bytes.");aui_toggle("Visible",1,1);aui_append("]}");check(h0!=aui_json_hash(1),"native panel refresh hashes all rows, not only first 64 bytes");
    printf("Product polish checks: %d, failures: %d\n",checks,failures);return failures?1:0;
}
