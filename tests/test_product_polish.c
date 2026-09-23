#define main legacy_main_not_run
#define check legacy_check_not_run
#include "../test_v30.c"
#undef main
#undef check
#include <math.h>
#include <string.h>
static int checks,failures;
static void check(int value,const char*name){checks++;printf("%s %s\n",value?"PASS":"FAIL",name);if(!value)failures++;}
static float theme_channel(int c){float v=c/255.0f;return v<=0.04045f?v/12.92f:powf((v+0.055f)/1.055f,2.4f);}
static float theme_luminance(uint32_t c){return .2126f*theme_channel((c>>16)&255)+.7152f*theme_channel((c>>8)&255)+.0722f*theme_channel(c&255);}
static float theme_contrast(uint32_t a,uint32_t b){float x=theme_luminance(a),y=theme_luminance(b);if(x<y){float t=x;x=y;y=t;}return (x+.05f)/(y+.05f);}
int main(void){
    memset(&G,0,sizeof(G));G.scale=G.uiScale=1;G.animTimerFd=-1;G.noteN=1;G.editorTarget=-1;
    int uniqueNames=1,uniqueThemes=1,readableText=1,readableMuted=1,selectable=1;
    for(int i=0;i<THEME_PRESET_COUNT;i++){
        Theme t=theme_preset_value(i);if(!theme_preset_name(i)[0]||theme_contrast(t.text,t.bg)<4.5f||theme_contrast(t.text,t.glass)<4.5f)readableText=0;if(theme_contrast(t.muted,t.bg)<3.0f)readableMuted=0;
        for(int j=0;j<i;j++){if(text_equal_local(theme_preset_name(i),theme_preset_name(j)))uniqueNames=0;if(memcmp(&t,&THEME_PRESETS[j],sizeof(t))==0)uniqueThemes=0;}
        theme_preset(i);if(!theme_is_preset(i))selectable=0;
    }
    check(THEME_PRESET_COUNT>=20,"appearance offers a large curated theme collection");check(uniqueNames&&uniqueThemes,"curated themes have unique names and complete role palettes");check(readableText,"every curated theme keeps primary text readable on canvas and panels");check(readableMuted,"every curated theme keeps secondary text distinguishable");check(selectable&&UI_ROLE0==UI_PRESET0+THEME_PRESET_COUNT,"every curated theme has a collision-free selectable action");theme_preset(0);
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
