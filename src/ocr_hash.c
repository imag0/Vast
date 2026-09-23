#include "ocr_core.h"

extern void *memset(void *, int, OcrSize);
extern void *memcpy(void *, const void *, OcrSize);

static OcrU32 rotr32(OcrU32 x, OcrU32 n) {
    return (x >> n) | (x << (32u - n));
}

static OcrU32 read_be32(const OcrU8 *p) {
    return ((OcrU32)p[0] << 24) | ((OcrU32)p[1] << 16) |
           ((OcrU32)p[2] << 8) | (OcrU32)p[3];
}

static void write_be32(OcrU8 *p, OcrU32 v) {
    p[0] = (OcrU8)(v >> 24);
    p[1] = (OcrU8)(v >> 16);
    p[2] = (OcrU8)(v >> 8);
    p[3] = (OcrU8)v;
}

static void sha256_compress(OcrSha256 *ctx, const OcrU8 block[64]) {
    static const OcrU32 k[64] = {
        0x428a2f98u,0x71374491u,0xb5c0fbcfu,0xe9b5dba5u,
        0x3956c25bu,0x59f111f1u,0x923f82a4u,0xab1c5ed5u,
        0xd807aa98u,0x12835b01u,0x243185beu,0x550c7dc3u,
        0x72be5d74u,0x80deb1feu,0x9bdc06a7u,0xc19bf174u,
        0xe49b69c1u,0xefbe4786u,0x0fc19dc6u,0x240ca1ccu,
        0x2de92c6fu,0x4a7484aau,0x5cb0a9dcu,0x76f988dau,
        0x983e5152u,0xa831c66du,0xb00327c8u,0xbf597fc7u,
        0xc6e00bf3u,0xd5a79147u,0x06ca6351u,0x14292967u,
        0x27b70a85u,0x2e1b2138u,0x4d2c6dfcu,0x53380d13u,
        0x650a7354u,0x766a0abbu,0x81c2c92eu,0x92722c85u,
        0xa2bfe8a1u,0xa81a664bu,0xc24b8b70u,0xc76c51a3u,
        0xd192e819u,0xd6990624u,0xf40e3585u,0x106aa070u,
        0x19a4c116u,0x1e376c08u,0x2748774cu,0x34b0bcb5u,
        0x391c0cb3u,0x4ed8aa4au,0x5b9cca4fu,0x682e6ff3u,
        0x748f82eeu,0x78a5636fu,0x84c87814u,0x8cc70208u,
        0x90befffau,0xa4506cebu,0xbef9a3f7u,0xc67178f2u
    };
    OcrU32 w[64];
    OcrU32 a,b,c,d,e,f,g,h;
    OcrU32 i;
    for (i = 0; i < 16; ++i) w[i] = read_be32(block + i * 4u);
    for (i = 16; i < 64; ++i) {
        OcrU32 x = w[i - 15u];
        OcrU32 y = w[i - 2u];
        OcrU32 s0 = rotr32(x,7) ^ rotr32(x,18) ^ (x >> 3);
        OcrU32 s1 = rotr32(y,17) ^ rotr32(y,19) ^ (y >> 10);
        w[i] = w[i - 16u] + s0 + w[i - 7u] + s1;
    }
    a=ctx->h[0]; b=ctx->h[1]; c=ctx->h[2]; d=ctx->h[3];
    e=ctx->h[4]; f=ctx->h[5]; g=ctx->h[6]; h=ctx->h[7];
    for (i = 0; i < 64; ++i) {
        OcrU32 s1 = rotr32(e,6) ^ rotr32(e,11) ^ rotr32(e,25);
        OcrU32 ch = (e & f) ^ ((~e) & g);
        OcrU32 t1 = h + s1 + ch + k[i] + w[i];
        OcrU32 s0 = rotr32(a,2) ^ rotr32(a,13) ^ rotr32(a,22);
        OcrU32 maj = (a & b) ^ (a & c) ^ (b & c);
        OcrU32 t2 = s0 + maj;
        h=g; g=f; f=e; e=d+t1; d=c; c=b; b=a; a=t1+t2;
    }
    ctx->h[0]+=a; ctx->h[1]+=b; ctx->h[2]+=c; ctx->h[3]+=d;
    ctx->h[4]+=e; ctx->h[5]+=f; ctx->h[6]+=g; ctx->h[7]+=h;
}

void ocr_sha256_init(OcrSha256 *ctx) {
    if (!ctx) return;
    ctx->h[0]=0x6a09e667u; ctx->h[1]=0xbb67ae85u;
    ctx->h[2]=0x3c6ef372u; ctx->h[3]=0xa54ff53au;
    ctx->h[4]=0x510e527fu; ctx->h[5]=0x9b05688cu;
    ctx->h[6]=0x1f83d9abu; ctx->h[7]=0x5be0cd19u;
    ctx->total_bytes=0; ctx->block_used=0;
    memset(ctx->block,0,sizeof(ctx->block));
}

void ocr_sha256_update(OcrSha256 *ctx, const void *data, OcrSize len) {
    const OcrU8 *p=(const OcrU8*)data;
    if (!ctx || (!p && len)) return;
    ctx->total_bytes += (OcrU64)len;
    while (len) {
        OcrU32 room=64u-ctx->block_used;
        OcrU32 take=(len<(OcrSize)room)?(OcrU32)len:room;
        memcpy(ctx->block+ctx->block_used,p,take);
        ctx->block_used+=take; p+=take; len-=take;
        if (ctx->block_used==64u) {
            sha256_compress(ctx,ctx->block);
            ctx->block_used=0;
        }
    }
}

void ocr_sha256_final(OcrSha256 *ctx, OcrU8 out[OCR_HASH_BYTES]) {
    OcrU64 bits;
    OcrU32 i;
    if (!ctx || !out) return;
    bits=ctx->total_bytes*8u;
    ctx->block[ctx->block_used++]=0x80u;
    if (ctx->block_used>56u) {
        while (ctx->block_used<64u) ctx->block[ctx->block_used++]=0;
        sha256_compress(ctx,ctx->block); ctx->block_used=0;
    }
    while (ctx->block_used<56u) ctx->block[ctx->block_used++]=0;
    for (i=0;i<8u;++i) ctx->block[63u-i]=(OcrU8)(bits>>(i*8u));
    sha256_compress(ctx,ctx->block);
    for (i=0;i<8u;++i) write_be32(out+i*4u,ctx->h[i]);
    memset(ctx,0,sizeof(*ctx));
}

void ocr_sha256(const void *data, OcrSize len, OcrU8 out[OCR_HASH_BYTES]) {
    OcrSha256 ctx;
    ocr_sha256_init(&ctx);
    ocr_sha256_update(&ctx,data,len);
    ocr_sha256_final(&ctx,out);
}

OcrU32 ocr_crc32(OcrU32 seed, const void *data, OcrSize len) {
    const OcrU8 *p=(const OcrU8*)data;
    OcrU32 crc=~seed;
    OcrSize i;
    for (i=0;i<len;++i) {
        OcrU32 x=(crc^(OcrU32)p[i])&255u;
        OcrU32 k;
        for (k=0;k<8u;++k) x=(x>>1)^((0u-(x&1u))&0xedb88320u);
        crc=(crc>>8)^x;
    }
    return ~crc;
}
