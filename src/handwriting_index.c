#include "ocr_core.h"

extern void *malloc(OcrSize);
extern void *realloc(void *,OcrSize);
extern void free(void *);
extern void *memcpy(void *,const void *,OcrSize);
extern void *memset(void *,int,OcrSize);

static int finite_f32(float v) {
    OcrU32 u=0; memcpy(&u,&v,4);
    return (u&0x7f800000u)!=0x7f800000u;
}

static int bounds_valid(const OcrBounds*b) {
    if (!b||!finite_f32(b->minx)||!finite_f32(b->miny)||
        !finite_f32(b->maxx)||!finite_f32(b->maxy)) return 0;
    if (b->minx>b->maxx||b->miny>b->maxy) return 0;
    return b->minx>=-1.0e12f&&b->maxx<=1.0e12f&&
           b->miny>=-1.0e12f&&b->maxy<=1.0e12f;
}

int ocr_utf8_valid(const char*text,OcrU32 len) {
    OcrU32 i=0;
    if (!text&&len) return 0;
    while (i<len) {
        OcrU8 c=(OcrU8)text[i++];
        OcrU32 cp,need,j;
        if (c<0x80u) {if (c==0u) return 0;continue;}
        if (c>=0xc2u&&c<=0xdfu) {cp=c&0x1fu;need=1u;}
        else if (c>=0xe0u&&c<=0xefu) {cp=c&0x0fu;need=2u;}
        else if (c>=0xf0u&&c<=0xf4u) {cp=c&0x07u;need=3u;}
        else return 0;
        if (need>len-i) return 0;
        for (j=0;j<need;++j) {
            OcrU8 d=(OcrU8)text[i++];
            if ((d&0xc0u)!=0x80u) return 0;
            cp=(cp<<6)|(d&0x3fu);
        }
        if ((need==2u&&cp<0x800u)||(need==3u&&cp<0x10000u)||
            (cp>=0xd800u&&cp<=0xdfffu)||cp>0x10ffffu) return 0;
    }
    return 1;
}

int ocr_normalize_basic(const char*text,OcrU32 len,char**out_text,OcrU32*out_len) {
    char*out;
    OcrU32 i,n=0;
    int pending_space=0;
    if (!out_text||!out_len||len>OCR_MAX_TEXT_BYTES||!ocr_utf8_valid(text,len))
        return OCR_ERR_INVALID;
    out=(char*)malloc((OcrSize)len+1u);
    if (!out) return OCR_ERR_NOMEM;
    for (i=0;i<len;++i) {
        OcrU8 c=(OcrU8)text[i];
        if (c==' '||c=='\t'||c=='\n'||c=='\r'||c=='\v'||c=='\f') {
            if (n) pending_space=1;
            continue;
        }
        if (pending_space) {out[n++]=' ';pending_space=0;}
        if (c>='A'&&c<='Z') c=(OcrU8)(c-'A'+'a');
        out[n++]=(char)c;
    }
    out[n]=0; *out_text=out; *out_len=n;
    return OCR_OK;
}

static void record_clear(OcrIndexRecord*r) {
    if (!r) return;
    if (r->sources) free(r->sources);
    if (r->original_text) free(r->original_text);
    if (r->normalized_text) free(r->normalized_text);
    memset(r,0,sizeof(*r));
}

void ocr_index_init(OcrIndex*index,OcrU64 project_key) {
    if (!index) return;
    memset(index,0,sizeof(*index));
    index->project_key=project_key;
    index->next_occurrence_id=1u;
}

void ocr_index_free(OcrIndex*index) {
    OcrU32 i;
    if (!index) return;
    for (i=0;i<index->count;++i) record_clear(index->records+i);
    if (index->records) free(index->records);
    memset(index,0,sizeof(*index));
}

static OcrI32 find_slot(const OcrIndex*index,OcrU64 id) {
    OcrU32 i;
    if (!index) return -1;
    for (i=0;i<index->count;++i) if (index->records[i].occurrence_id==id)
        return (OcrI32)i;
    return -1;
}

const OcrIndexRecord*ocr_index_find(const OcrIndex*index,OcrU64 occurrence_id) {
    OcrI32 i=find_slot(index,occurrence_id);
    return i>=0?index->records+(OcrU32)i:0;
}

static int copy_record(OcrIndexRecord*dst,const OcrIndexRecord*src) {
    char *norm=0;
    OcrU32 norm_len=0;
    memset(dst,0,sizeof(*dst));
    if (!src||!src->occurrence_id||src->source_count>OCR_MAX_SOURCES_PER_RECORD||
        src->original_len>OCR_MAX_TEXT_BYTES||src->normalized_len>OCR_MAX_TEXT_BYTES||
        !bounds_valid(&src->bounds)||!finite_f32(src->confidence)||
        src->confidence<0.0f||src->confidence>1.0f||
        !ocr_utf8_valid(src->original_text,src->original_len)||
        (src->source_count&&!src->sources)) return OCR_ERR_INVALID;
    if (src->normalized_text) {
        if (!ocr_utf8_valid(src->normalized_text,src->normalized_len)) return OCR_ERR_INVALID;
        norm=(char*)malloc((OcrSize)src->normalized_len+1u);
        if (!norm) return OCR_ERR_NOMEM;
        if (src->normalized_len) memcpy(norm,src->normalized_text,src->normalized_len);
        norm[src->normalized_len]=0; norm_len=src->normalized_len;
    } else {
        int nr=ocr_normalize_basic(src->original_text,src->original_len,&norm,&norm_len);
        if (nr!=OCR_OK) return nr;
    }
    dst->original_text=(char*)malloc((OcrSize)src->original_len+1u);
    if (!dst->original_text) {free(norm);return OCR_ERR_NOMEM;}
    if (src->original_len) memcpy(dst->original_text,src->original_text,src->original_len);
    dst->original_text[src->original_len]=0;
    if (src->source_count) {
        dst->sources=(OcrSourceRef*)malloc((OcrSize)src->source_count*sizeof(OcrSourceRef));
        if (!dst->sources) {free(norm);record_clear(dst);return OCR_ERR_NOMEM;}
        memcpy(dst->sources,src->sources,(OcrSize)src->source_count*sizeof(OcrSourceRef));
        {
            OcrU32 i;
            for (i=0;i<src->source_count;++i) dst->sources[i].runtime_index=-1;
        }
    }
    dst->occurrence_id=src->occurrence_id;
    memcpy(dst->source_hash,src->source_hash,OCR_HASH_BYTES);
    dst->bounds=src->bounds; dst->confidence=src->confidence; dst->flags=src->flags;
    dst->source_count=src->source_count; dst->original_len=src->original_len;
    dst->normalized_text=norm; dst->normalized_len=norm_len;
    return OCR_OK;
}

int ocr_index_put(OcrIndex*index,const OcrIndexRecord*record) {
    OcrIndexRecord copy;
    OcrI32 slot;
    int rc;
    if (!index||!record) return OCR_ERR_INVALID;
    rc=copy_record(&copy,record);
    if (rc!=OCR_OK) return rc;
    slot=find_slot(index,record->occurrence_id);
    if (slot>=0) {
        record_clear(index->records+(OcrU32)slot);
        index->records[(OcrU32)slot]=copy;
    } else {
        if (index->count>=OCR_MAX_RECORDS) {record_clear(&copy);return OCR_ERR_LIMIT;}
        if (index->count==index->capacity) {
            OcrU32 nc=index->capacity?index->capacity*2u:16u;
            OcrIndexRecord*p;
            if (nc>OCR_MAX_RECORDS) nc=OCR_MAX_RECORDS;
            p=(OcrIndexRecord*)realloc(index->records,(OcrSize)nc*sizeof(OcrIndexRecord));
            if (!p) {record_clear(&copy);return OCR_ERR_NOMEM;}
            index->records=p; index->capacity=nc;
        }
        index->records[index->count++]=copy;
    }
    if (record->occurrence_id>=index->next_occurrence_id)
        index->next_occurrence_id=record->occurrence_id+1u;
    return OCR_OK;
}

int ocr_index_set_active(OcrIndex*index,OcrU64 occurrence_id,int active) {
    OcrI32 i=find_slot(index,occurrence_id);
    if (i<0) return OCR_ERR_NOT_FOUND;
    if (active) index->records[(OcrU32)i].flags|=OCR_RECORD_ACTIVE;
    else index->records[(OcrU32)i].flags&=~OCR_RECORD_ACTIVE;
    return OCR_OK;
}

int ocr_index_translate(OcrIndex*index,OcrU64 occurrence_id,float dx,float dy) {
    OcrI32 i=find_slot(index,occurrence_id);
    OcrBounds b;
    if (i<0||!finite_f32(dx)||!finite_f32(dy)) return OCR_ERR_NOT_FOUND;
    b=index->records[(OcrU32)i].bounds;
    b.minx+=dx; b.maxx+=dx; b.miny+=dy; b.maxy+=dy;
    if (!bounds_valid(&b)) return OCR_ERR_LIMIT;
    index->records[(OcrU32)i].bounds=b;
    return OCR_OK;
}

int ocr_index_duplicate_translated(OcrIndex*index,OcrU64 source_occurrence_id,
                                   OcrU64 new_occurrence_id,float dx,float dy) {
    OcrI32 i=find_slot(index,source_occurrence_id);
    OcrIndexRecord view;
    if (i<0||!new_occurrence_id||find_slot(index,new_occurrence_id)>=0)
        return OCR_ERR_INVALID;
    view=index->records[(OcrU32)i];
    view.occurrence_id=new_occurrence_id;
    view.bounds.minx+=dx; view.bounds.maxx+=dx;
    view.bounds.miny+=dy; view.bounds.maxy+=dy;
    if (!bounds_valid(&view.bounds)) return OCR_ERR_LIMIT;
    return ocr_index_put(index,&view);
}

static int bytes_contains(const char*hay,OcrU32 hn,const char*needle,OcrU32 nn) {
    OcrU32 i,j;
    if (!nn) return 1;
    if (nn>hn) return 0;
    for (i=0;i+nn<=hn;++i) {
        for (j=0;j<nn&&hay[i+j]==needle[j];++j) {}
        if (j==nn) return 1;
    }
    return 0;
}

int ocr_index_search(const OcrIndex*index,const char*query,
                     float minimum_confidence,OcrSearchHit*out,OcrU32 capacity) {
    char*nq=0;
    OcrU32 nq_len=0,i,n=0;
    int rc;
    OcrU32 qlen=0;
    if (!index||!query||(!out&&capacity)||!finite_f32(minimum_confidence))
        return OCR_ERR_INVALID;
    while (query[qlen]) {if (qlen>=OCR_MAX_TEXT_BYTES) return OCR_ERR_LIMIT;++qlen;}
    rc=ocr_normalize_basic(query,qlen,&nq,&nq_len);
    if (rc!=OCR_OK) return rc;
    if (!nq_len) {free(nq);return 0;}
    for (i=0;i<index->count&&n<capacity;++i) {
        const OcrIndexRecord*r=index->records+i;
        if (!(r->flags&OCR_RECORD_ACTIVE)||r->confidence<minimum_confidence) continue;
        if (bytes_contains(r->normalized_text,r->normalized_len,nq,nq_len)) {
            out[n].occurrence_id=r->occurrence_id; out[n].bounds=r->bounds;
            out[n].confidence=r->confidence; out[n].original_text=r->original_text;
            out[n].original_len=r->original_len; ++n;
        }
    }
    free(nq);
    return (int)n;
}
