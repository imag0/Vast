#define main legacy_main_not_run
#define check legacy_check
#include "../test_v30.c"
#undef main
#undef check
static int checks,failures;
static void check(int ok,const char*name){checks++;if(!ok){failures++;printf("FAIL %s\n",name);}}
static unsigned seed=4931;
static float rnd(void){seed=1664525u*seed+1013904223u;return (seed>>8)/16777216.0f;}
static int brute(Stroke*s,float x,float y,float r){if(stroke_layer_locked(s))return 0;for(int j=1;j<s->n;j++)if(point_seg_dist2(x,y,s->pts[j-1].x,s->pts[j-1].y,s->pts[j].x,s->pts[j].y)<=r*r)return 1;return 0;}
int main(void){
    memset(&G,0,sizeof(G));G.scale=G.uiScale=1;G.currentStroke=-1;ensure_strokes(2000);G.strokeN=2000;
    for(int i=0;i<G.strokeN;i++){Stroke*s=&G.strokes[i];s->active=1;s->baseWidth=4;float x=rnd()*12000,y=rnd()*12000;for(int k=0;k<80;k++)append_point(s,x+k*3,y+sinf(k*.3f)*40,.6f);}
    check(erase_index_prepare(),"build");EraseNode*original=EI.nodes;int expected[2000];
    for(int q=0;q<250;q++){float x=rnd()*12000,y=rnd()*12000;free_action(&G.eraseAction);for(int i=0;i<G.strokeN;i++){G.strokes[i].active=1;expected[i]=brute(&G.strokes[i],x,y,26);}erase_at(x,y);for(int i=0;i<G.strokeN;i++)check(G.strokes[i].active==!expected[i],"indexed result matches exhaustive segment test");check(EI.nodes==original,"erase does not rebuild index");}
    free_action(&G.eraseAction);for(int i=0;i<G.strokeN;i++)G.strokes[i].active=1;
    Stroke*s=&G.strokes[0];float x=s->pts[0].x,y=s->pts[0].y;G.lockInk=1;erase_at(x,y);check(s->active,"locked ink retained");G.lockInk=0;erase_at(x,y);check(!s->active,"nearby ink removed");int count=G.eraseAction.n;erase_at(x,y);check(G.eraseAction.n==count,"no duplicate undo IDs");
    G.eraseAction.type=ACT_ERASE;push_action(&G.eraseAction);undo_action();check(s->active,"undo restores indexed stroke");redo_action();check(!s->active,"redo removes indexed stroke");undo_action();
    for(int k=0;k<s->n;k++){s->pts[k].x+=20000;s->pts[k].y+=20000;}s->minx+=20000;s->maxx+=20000;s->miny+=20000;s->maxy+=20000;G.sceneRevision++;
    erase_at(x,y);check(s->active,"movement invalidates old location");erase_at(x+20000,y+20000);check(!s->active,"movement indexes new location");
    check(erase_render_candidates(x-400,y-400,x+400,y+400),"damage candidates available");for(int i=1;i<ER_n;i++)check(ER_ids[i]>ER_ids[i-1],"damage candidates unique and document ordered");
    clear_document_memory();check(!EI.valid,"project clear discards index");
    printf("erase spatial checks=%d failures=%d\n",checks,failures);return failures?1:0;
}
