/* Segment-chunk BVH. Inactive strokes stay indexed: undo/redo only changes
 * visibility, never geometry. Rebuild at the next gesture after geometry edits,
 * never after an erase sample. Each leaf tests at most 32 segments. */
#define VAST_ERASE_INDEX 1
typedef struct { float x0,y0,x1,y1; int stroke,first,last,left,right; } EraseNode;
static struct { EraseNode *nodes; int n,root,count,revision,valid; Stroke *source; } EI;
static void erase_index_reset(void){free(EI.nodes);memset(&EI,0,sizeof(EI));}
static int erase_build_node(int lo,int hi,int *next){
    if(hi-lo==1)return lo;
    int id=(*next)++;EraseNode *n=&EI.nodes[id];*n=EI.nodes[lo];n->stroke=-1;
    for(int i=lo+1;i<hi;i++){EraseNode*q=&EI.nodes[i];n->x0=fmin2(n->x0,q->x0);n->y0=fmin2(n->y0,q->y0);n->x1=fmax2(n->x1,q->x1);n->y1=fmax2(n->y1,q->y1);}
    int axis=n->x1-n->x0>=n->y1-n->y0;float mid=axis?(n->x0+n->x1):(n->y0+n->y1);int cut=lo;
    for(int i=lo;i<hi;i++){EraseNode*q=&EI.nodes[i];if((axis?q->x0+q->x1:q->y0+q->y1)<mid){EraseNode t=EI.nodes[cut];EI.nodes[cut++]=*q;*q=t;}}
    /* Bound tree depth even for extremely uneven coordinates. */
    if(cut-lo<(hi-lo)/8||hi-cut<(hi-lo)/8||cut==lo||cut==hi)cut=lo+(hi-lo)/2;
    n->left=erase_build_node(lo,cut,next);n->right=erase_build_node(cut,hi,next);return id;
}
static int erase_index_prepare(void){
    if(EI.valid&&EI.source==G.strokes&&EI.count==G.strokeN&&EI.revision==G.sceneRevision)return 1;
    erase_index_reset();size_t chunks=0;
    for(int i=0;i<G.strokeN;i++){int n=G.strokes[i].n;if(n>0)chunks+=(size_t)(n>1?(n-2)/32+1:1);}
    if(chunks>16000000)return 0;
    if(chunks){EI.nodes=(EraseNode*)malloc(chunks*2*sizeof(EraseNode));if(!EI.nodes)return 0;}
    int used=0;
    for(int i=0;i<G.strokeN;i++){Stroke*s=&G.strokes[i];for(int k=0;k<s->n;){int end=mini(s->n-1,k+32);EraseNode*n=&EI.nodes[used++];n->stroke=i;n->first=k;n->last=end;n->x0=n->x1=s->pts[k].x;n->y0=n->y1=s->pts[k].y;for(int p=k+1;p<=end;p++){n->x0=fmin2(n->x0,s->pts[p].x);n->x1=fmax2(n->x1,s->pts[p].x);n->y0=fmin2(n->y0,s->pts[p].y);n->y1=fmax2(n->y1,s->pts[p].y);}if(end==s->n-1)break;k=end;}}
    EI.n=used;EI.root=used?erase_build_node(0,used,&EI.n):-1;EI.source=G.strokes;EI.count=G.strokeN;EI.revision=G.sceneRevision;EI.valid=1;return 1;
}
static void erase_hit(int id){
    /* active is also the per-gesture deduplication bit; no quadratic ID scan. */
    Action*a=&G.eraseAction;if(!G.strokes[id].active)return;
    if(a->n==a->cap){int cap=a->cap?a->cap*2:32;int*p=(int*)realloc(a->ids,(size_t)cap*sizeof(int));if(!p)return;a->ids=p;a->cap=cap;}
    a->ids[a->n++]=id;G.strokes[id].active=0;G.saveDirty=1;G.minimapDirty=1;
}
static void erase_test_chunk(int id,int first,int last,float x,float y,float r2){
    Stroke*s=&G.strokes[id];if(!s->active||stroke_layer_locked(s))return;
    if(first==last){if(sq(x-s->pts[first].x)+sq(y-s->pts[first].y)<=r2)erase_hit(id);return;}
    for(int k=first+1;k<=last;k++)if(point_seg_dist2(x,y,s->pts[k-1].x,s->pts[k-1].y,s->pts[k].x,s->pts[k].y)<=r2){erase_hit(id);return;}
}
static void erase_query_node(int id,float x,float y,float r){
    EraseNode*n=&EI.nodes[id];if(x<n->x0-r||x>n->x1+r||y<n->y0-r||y>n->y1+r)return;
    if(n->stroke>=0)erase_test_chunk(n->stroke,n->first,n->last,x,y,r*r);
    else{erase_query_node(n->left,x,y,r);erase_query_node(n->right,x,y,r);}
}
static void erase_at(float sx,float sy){
    float x=(sx-G.offX)/G.scale,y=(sy-G.offY)/G.scale,r=26.0f/G.scale;
    if(erase_index_prepare()){if(EI.root>=0)erase_query_node(EI.root,x,y,r);return;}
    /* Allocation failure must not make ink impossible to erase. */
    for(int i=0;i<G.strokeN;i++){Stroke*s=&G.strokes[i];if(s->n>0&&x>=s->minx-r&&x<=s->maxx+r&&y>=s->miny-r&&y<=s->maxy+r)erase_test_chunk(i,0,s->n-1,x,y,r*r);}
}
/* Reuse the same tree to restrict damage repaint to local strokes. */
static int *ER_ids,*ER_seen,ER_cap,ER_n,ER_epoch,ER_active;
static void erase_render_query(int id,float x0,float y0,float x1,float y1){
    EraseNode*n=&EI.nodes[id];if(n->x1<x0||n->x0>x1||n->y1<y0||n->y0>y1)return;
    if(n->stroke<0){erase_render_query(n->left,x0,y0,x1,y1);erase_render_query(n->right,x0,y0,x1,y1);return;}
    int i=n->stroke;if(G.strokes[i].active&&ER_seen[i]!=ER_epoch){ER_seen[i]=ER_epoch;ER_ids[ER_n++]=i;}
}
static void erase_render_sift(int start,int end){
    int root=start;while(root*2+1<=end){int child=root*2+1;if(child<end&&ER_ids[child]<ER_ids[child+1])child++;if(ER_ids[root]>=ER_ids[child])return;int t=ER_ids[root];ER_ids[root]=ER_ids[child];ER_ids[child]=t;root=child;}
}
static int erase_render_candidates(float x0,float y0,float x1,float y1){
    if(!erase_index_prepare())return 0;
    if(ER_cap<G.strokeN){int*ids=(int*)malloc((size_t)G.strokeN*sizeof(int));int*seen=(int*)malloc((size_t)G.strokeN*sizeof(int));if(!ids||!seen){free(ids);free(seen);return 0;}free(ER_ids);free(ER_seen);ER_ids=ids;ER_seen=seen;ER_cap=G.strokeN;memset(ER_seen,0,(size_t)ER_cap*sizeof(int));ER_epoch=0;}
    if(ER_epoch==2147483647){memset(ER_seen,0,(size_t)ER_cap*sizeof(int));ER_epoch=0;}ER_epoch++;ER_n=0;
    if(EI.root>=0)erase_render_query(EI.root,x0,y0,x1,y1);
    /* Heap sort preserves document compositing order, O(k log k). */
    for(int i=ER_n/2-1;i>=0;i--)erase_render_sift(i,ER_n-1);
    for(int end=ER_n-1;end>0;end--){int t=ER_ids[0];ER_ids[0]=ER_ids[end];ER_ids[end]=t;erase_render_sift(0,end-1);}return 1;
}
