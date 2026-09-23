#include "ocr_core.h"

extern void *malloc(OcrSize);
extern void *realloc(void *,OcrSize);
extern void free(void *);
extern void *memcpy(void *,const void *,OcrSize);
extern void *memset(void *,int,OcrSize);
extern int memcmp(const void *,const void *,OcrSize);
extern void *fopen(const char *,const char *);
extern OcrSize fread(void *,OcrSize,OcrSize,void *);
extern OcrSize fwrite(const void *,OcrSize,OcrSize,void *);
extern int fflush(void *);
extern int fclose(void *);
extern int remove(const char *);

#if defined(_WIN32)
extern int _fileno(void *);
extern int _commit(int);
__declspec(dllimport) int __stdcall MoveFileExA(const char *,const char *,unsigned long);
static int sync_file(void*f){int fd=_fileno(f);return fd<0?-1:_commit(fd);}
static int replace_file(const char*a,const char*b){return MoveFileExA(a,b,1u|8u)?0:-1;}
#else
extern int fileno(void *);
extern int fsync(int);
extern int rename(const char *,const char *);
static int sync_file(void*f){int fd=fileno(f);return fd<0?-1:fsync(fd);}
static int replace_file(const char*a,const char*b){return rename(a,b);}
#endif

#define SIDECAR_HEADER_BYTES 72u
#define SIDECAR_SOURCE_BYTES 44u
#define SIDECAR_RECORD_FIXED_BYTES 76u

typedef struct {
    OcrU8 *data;
    OcrU32 size;
    OcrU32 capacity;
} ByteBuffer;

typedef struct {
    const OcrU8 *data;
    OcrU32 size;
    OcrU32 at;
} ByteReader;

static const OcrU8 SIDECAR_MAGIC[8]={'V','A','S','T','O','C','R',0};

static int finite_f32(float v) {
    OcrU32 u=0;memcpy(&u,&v,4);return (u&0x7f800000u)!=0x7f800000u;
}

static int bounds_valid(const OcrBounds*b) {
    return b&&finite_f32(b->minx)&&finite_f32(b->miny)&&finite_f32(b->maxx)&&
           finite_f32(b->maxy)&&b->minx<=b->maxx&&b->miny<=b->maxy&&
           b->minx>=-1.0e12f&&b->maxx<=1.0e12f&&b->miny>=-1.0e12f&&b->maxy<=1.0e12f;
}

static void put_le32_raw(OcrU8*p,OcrU32 v) {
    p[0]=(OcrU8)v;p[1]=(OcrU8)(v>>8);p[2]=(OcrU8)(v>>16);p[3]=(OcrU8)(v>>24);
}

static void put_le64_raw(OcrU8*p,OcrU64 v) {
    OcrU32 i;for(i=0;i<8u;++i)p[i]=(OcrU8)(v>>(i*8u));
}

static OcrU32 get_le32_raw(const OcrU8*p) {
    return (OcrU32)p[0]|((OcrU32)p[1]<<8)|((OcrU32)p[2]<<16)|((OcrU32)p[3]<<24);
}

static OcrU64 get_le64_raw(const OcrU8*p) {
    OcrU64 v=0;OcrU32 i;for(i=0;i<8u;++i)v|=(OcrU64)p[i]<<(i*8u);return v;
}

static int buffer_reserve(ByteBuffer*b,OcrU32 add) {
    OcrU32 need,nc;
    OcrU8*p;
    if (!b||add>OCR_MAX_SIDECAR_BYTES-b->size) return 0;
    need=b->size+add;
    if (need<=b->capacity) return 1;
    nc=b->capacity?b->capacity:1024u;
    while (nc<need) {
        if (nc>OCR_MAX_SIDECAR_BYTES/2u) {nc=OCR_MAX_SIDECAR_BYTES;break;}
        nc*=2u;
    }
    p=(OcrU8*)realloc(b->data,nc);
    if (!p) return 0;
    b->data=p;b->capacity=nc;return 1;
}

static int buffer_bytes(ByteBuffer*b,const void*p,OcrU32 n) {
    if (!buffer_reserve(b,n)) return 0;
    if (n) memcpy(b->data+b->size,p,n);
    b->size+=n;return 1;
}

static int buffer_u32(ByteBuffer*b,OcrU32 v) {
    OcrU8 q[4];put_le32_raw(q,v);return buffer_bytes(b,q,4);
}

static int buffer_i32(ByteBuffer*b,OcrI32 v){return buffer_u32(b,(OcrU32)v);}

static int buffer_u64(ByteBuffer*b,OcrU64 v) {
    OcrU8 q[8];put_le64_raw(q,v);return buffer_bytes(b,q,8);
}

static int buffer_f32(ByteBuffer*b,float v) {
    OcrU32 u=0;memcpy(&u,&v,4);return buffer_u32(b,u);
}

static int read_bytes(ByteReader*r,void*out,OcrU32 n) {
    if (!r||n>r->size-r->at) return 0;
    if (n&&out) memcpy(out,r->data+r->at,n);
    r->at+=n;return 1;
}

static int read_u32(ByteReader*r,OcrU32*out) {
    if (!r||!out||4u>r->size-r->at) return 0;
    *out=get_le32_raw(r->data+r->at);r->at+=4u;return 1;
}

static int read_i32(ByteReader*r,OcrI32*out) {
    OcrU32 u;if(!read_u32(r,&u))return 0;*out=(OcrI32)u;return 1;
}

static int read_u64(ByteReader*r,OcrU64*out) {
    if (!r||!out||8u>r->size-r->at) return 0;
    *out=get_le64_raw(r->data+r->at);r->at+=8u;return 1;
}

static int read_f32(ByteReader*r,float*out) {
    OcrU32 u;if(!read_u32(r,&u))return 0;memcpy(out,&u,4);return 1;
}

static OcrU32 path_length(const char*s) {
    OcrU32 n=0;if(!s)return 0;while(s[n]){if(n>=4090u)return 0;++n;}return n;
}

static int make_temp_path(const char*path,char out[4096]) {
    static const char suffix[5]={'.','t','m','p',0};
    OcrU32 n=path_length(path);
    if (!n||n+4u>=4096u) return 0;
    memcpy(out,path,n);memcpy(out+n,suffix,5);return 1;
}

static int serialize_record(ByteBuffer*b,const OcrIndexRecord*r) {
    OcrU32 i,start,body;
    OcrU64 source_bytes;
    if (!b||!r||!r->occurrence_id||!bounds_valid(&r->bounds)||
        !finite_f32(r->confidence)||r->confidence<0.0f||r->confidence>1.0f||
        r->source_count>OCR_MAX_SOURCES_PER_RECORD||r->original_len>OCR_MAX_TEXT_BYTES||
        r->normalized_len>OCR_MAX_TEXT_BYTES||(r->source_count&&!r->sources)||
        !ocr_utf8_valid(r->original_text,r->original_len)||
        !ocr_utf8_valid(r->normalized_text,r->normalized_len)) return 0;
    source_bytes=(OcrU64)r->source_count*SIDECAR_SOURCE_BYTES;
    if (source_bytes>0xffffffffu||SIDECAR_RECORD_FIXED_BYTES+source_bytes+
        r->original_len+r->normalized_len>0xffffffffu) return 0;
    start=b->size;
    if (!buffer_u32(b,0)||!buffer_u64(b,r->occurrence_id)||
        !buffer_bytes(b,r->source_hash,OCR_HASH_BYTES)||
        !buffer_f32(b,r->bounds.minx)||!buffer_f32(b,r->bounds.miny)||
        !buffer_f32(b,r->bounds.maxx)||!buffer_f32(b,r->bounds.maxy)||
        !buffer_f32(b,r->confidence)||!buffer_u32(b,r->flags)||
        !buffer_u32(b,r->source_count)||!buffer_u32(b,r->original_len)||
        !buffer_u32(b,r->normalized_len)) return 0;
    for (i=0;i<r->source_count;++i) {
        const OcrSourceRef*s=r->sources+i;
        if (!buffer_bytes(b,s->shape_hash,OCR_HASH_BYTES)||
            !buffer_i32(b,s->local_x_q)||!buffer_i32(b,s->local_y_q)||
            !buffer_u32(b,s->point_count)) return 0;
    }
    if (!buffer_bytes(b,r->original_text,r->original_len)||
        !buffer_bytes(b,r->normalized_text,r->normalized_len)) return 0;
    body=b->size-start-4u;
    put_le32_raw(b->data+start,body);
    return 1;
}

OcrSidecarStatus ocr_sidecar_save_atomic(const char*path,const OcrIndex*index,
    OcrU32 index_version,const OcrU8 model_hash[OCR_MODEL_HASH_BYTES]) {
    ByteBuffer payload={0,0,0};
    OcrU8 header[SIDECAR_HEADER_BYTES];
    char temp[4096];
    void*f=0;
    OcrU32 i,payload_crc,header_crc;
    int io_ok=1;
    if (!path||!index||!model_hash||index->count>OCR_MAX_RECORDS||
        !make_temp_path(path,temp)) return OCR_SIDECAR_IO_ERROR;
    for (i=0;i<index->count;++i) if (!serialize_record(&payload,index->records+i)) {
        if (payload.data) free(payload.data);
        return OCR_SIDECAR_CORRUPT;
    }
    memset(header,0,sizeof(header));
    memcpy(header,SIDECAR_MAGIC,8);
    put_le32_raw(header+8,OCR_SIDECAR_FORMAT_VERSION);
    put_le32_raw(header+12,index_version);
    memcpy(header+16,model_hash,OCR_MODEL_HASH_BYTES);
    put_le64_raw(header+48,index->project_key);
    put_le32_raw(header+56,index->count);
    put_le32_raw(header+60,payload.size);
    payload_crc=ocr_crc32(0,payload.data,payload.size);
    put_le32_raw(header+64,payload_crc);
    header_crc=ocr_crc32(0,header,68u);
    put_le32_raw(header+68,header_crc);
    f=fopen(temp,"wb");
    if (!f) {if(payload.data)free(payload.data);return OCR_SIDECAR_IO_ERROR;}
    if (fwrite(header,1,sizeof(header),f)!=sizeof(header)) io_ok=0;
    if (io_ok&&payload.size&&fwrite(payload.data,1,payload.size,f)!=payload.size) io_ok=0;
    if (io_ok&&fflush(f)!=0) io_ok=0;
    if (io_ok&&sync_file(f)!=0) io_ok=0;
    if (fclose(f)!=0) io_ok=0;
    if (payload.data) free(payload.data);
    if (!io_ok) {remove(temp);return OCR_SIDECAR_IO_ERROR;}
    if (replace_file(temp,path)!=0) {remove(temp);return OCR_SIDECAR_IO_ERROR;}
    return OCR_SIDECAR_OK;
}

static void free_temp_record(OcrIndexRecord*r) {
    if (!r) return;
    if (r->sources) free(r->sources);
    if (r->original_text) free(r->original_text);
    if (r->normalized_text) free(r->normalized_text);
    memset(r,0,sizeof(*r));
}

static int parse_record(ByteReader*payload,OcrIndex*index) {
    OcrU32 record_size,record_end,i;
    OcrIndexRecord r;
    int put_rc;
    memset(&r,0,sizeof(r));
    if (!read_u32(payload,&record_size)||record_size<SIDECAR_RECORD_FIXED_BYTES||
        record_size>payload->size-payload->at) return 0;
    record_end=payload->at+record_size;
    if (!read_u64(payload,&r.occurrence_id)||
        !read_bytes(payload,r.source_hash,OCR_HASH_BYTES)||
        !read_f32(payload,&r.bounds.minx)||!read_f32(payload,&r.bounds.miny)||
        !read_f32(payload,&r.bounds.maxx)||!read_f32(payload,&r.bounds.maxy)||
        !read_f32(payload,&r.confidence)||!read_u32(payload,&r.flags)||
        !read_u32(payload,&r.source_count)||!read_u32(payload,&r.original_len)||
        !read_u32(payload,&r.normalized_len)) return 0;
    if (!r.occurrence_id||!bounds_valid(&r.bounds)||!finite_f32(r.confidence)||
        r.confidence<0.0f||r.confidence>1.0f||
        r.source_count>OCR_MAX_SOURCES_PER_RECORD||r.original_len>OCR_MAX_TEXT_BYTES||
        r.normalized_len>OCR_MAX_TEXT_BYTES) return 0;
    if ((OcrU64)r.source_count*SIDECAR_SOURCE_BYTES+r.original_len+r.normalized_len>
        (OcrU64)(record_end-payload->at)) return 0;
    if (r.source_count) {
        r.sources=(OcrSourceRef*)malloc((OcrSize)r.source_count*sizeof(OcrSourceRef));
        if (!r.sources) return -1;
        for (i=0;i<r.source_count;++i) {
            if (!read_bytes(payload,r.sources[i].shape_hash,OCR_HASH_BYTES)||
                !read_i32(payload,&r.sources[i].local_x_q)||
                !read_i32(payload,&r.sources[i].local_y_q)||
                !read_u32(payload,&r.sources[i].point_count)) {free_temp_record(&r);return 0;}
            r.sources[i].runtime_index=-1;
        }
    }
    r.original_text=(char*)malloc((OcrSize)r.original_len+1u);
    r.normalized_text=(char*)malloc((OcrSize)r.normalized_len+1u);
    if (!r.original_text||!r.normalized_text) {free_temp_record(&r);return -1;}
    if (!read_bytes(payload,r.original_text,r.original_len)||
        !read_bytes(payload,r.normalized_text,r.normalized_len)) {free_temp_record(&r);return 0;}
    r.original_text[r.original_len]=0;r.normalized_text[r.normalized_len]=0;
    if (!ocr_utf8_valid(r.original_text,r.original_len)||
        !ocr_utf8_valid(r.normalized_text,r.normalized_len)||payload->at!=record_end) {
        free_temp_record(&r);return 0;
    }
    put_rc=ocr_index_put(index,&r);
    free_temp_record(&r);
    return put_rc==OCR_OK?1:(put_rc==OCR_ERR_NOMEM?-1:0);
}

OcrSidecarStatus ocr_sidecar_load(const char*path,OcrU32 expected_index_version,
    const OcrU8 expected_model_hash[OCR_MODEL_HASH_BYTES],
    OcrU64 expected_project_key,OcrIndex*out_index) {
    OcrU8 header[SIDECAR_HEADER_BYTES],extra=0,*payload_data=0;
    OcrU32 format,index_version,record_count,payload_len,payload_crc,header_crc,i;
    OcrU64 project_key;
    void*f;
    ByteReader reader;
    OcrIndex temp;
    if (!path||!expected_model_hash||!out_index) return OCR_SIDECAR_CORRUPT;
    f=fopen(path,"rb");
    if (!f) return OCR_SIDECAR_MISSING;
    if (fread(header,1,sizeof(header),f)!=sizeof(header)) {fclose(f);return OCR_SIDECAR_CORRUPT;}
    if (memcmp(header,SIDECAR_MAGIC,8)!=0) {fclose(f);return OCR_SIDECAR_CORRUPT;}
    format=get_le32_raw(header+8);index_version=get_le32_raw(header+12);
    project_key=get_le64_raw(header+48);record_count=get_le32_raw(header+56);
    payload_len=get_le32_raw(header+60);payload_crc=get_le32_raw(header+64);
    header_crc=get_le32_raw(header+68);
    if (format!=OCR_SIDECAR_FORMAT_VERSION||header_crc!=ocr_crc32(0,header,68u)) {
        fclose(f);return OCR_SIDECAR_CORRUPT;
    }
    if (index_version!=expected_index_version) {fclose(f);return OCR_SIDECAR_INDEX_VERSION_MISMATCH;}
    if (memcmp(header+16,expected_model_hash,OCR_MODEL_HASH_BYTES)!=0) {
        fclose(f);return OCR_SIDECAR_MODEL_MISMATCH;
    }
    if (project_key!=expected_project_key) {fclose(f);return OCR_SIDECAR_PROJECT_MISMATCH;}
    if (record_count>OCR_MAX_RECORDS||payload_len>OCR_MAX_SIDECAR_BYTES) {
        fclose(f);return OCR_SIDECAR_CORRUPT;
    }
    if (payload_len) {
        payload_data=(OcrU8*)malloc(payload_len);
        if (!payload_data) {fclose(f);return OCR_SIDECAR_NO_MEMORY;}
        if (fread(payload_data,1,payload_len,f)!=payload_len) {
            free(payload_data);fclose(f);return OCR_SIDECAR_CORRUPT;
        }
    }
    if (fread(&extra,1,1,f)!=0u) {if(payload_data)free(payload_data);fclose(f);return OCR_SIDECAR_CORRUPT;}
    fclose(f);
    if (ocr_crc32(0,payload_data,payload_len)!=payload_crc) {
        if(payload_data)free(payload_data);
        return OCR_SIDECAR_CORRUPT;
    }
    ocr_index_init(&temp,project_key);
    reader.data=payload_data;reader.size=payload_len;reader.at=0;
    for (i=0;i<record_count;++i) {
        int pr=parse_record(&reader,&temp);
        if (pr<=0) {
            if(payload_data)free(payload_data);
            ocr_index_free(&temp);
            return pr<0?OCR_SIDECAR_NO_MEMORY:OCR_SIDECAR_CORRUPT;
        }
    }
    if (reader.at!=reader.size) {
        if(payload_data)free(payload_data);
        ocr_index_free(&temp);
        return OCR_SIDECAR_CORRUPT;
    }
    if(payload_data)free(payload_data);
    ocr_index_free(out_index);
    *out_index=temp;
    return OCR_SIDECAR_OK;
}
