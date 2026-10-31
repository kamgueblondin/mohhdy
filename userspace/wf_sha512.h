/* wf_sha512.h - Lot 67: SHA-512 and SHA-384 (FIPS 180-4), freestanding. */
#ifndef WF_SHA512_H
#define WF_SHA512_H
#include <stdint.h>

typedef struct {
    uint64_t h[8];
    uint64_t bytes;
    uint8_t block[128];
    uint32_t used;
    uint32_t out_len;   /* 64 (SHA-512) or 48 (SHA-384) */
} wf_sha512_ctx_t;

void wf_sha512_init(wf_sha512_ctx_t* c);
void wf_sha384_init(wf_sha512_ctx_t* c);
void wf_sha512_update(wf_sha512_ctx_t* c, const uint8_t* d, uint32_t n);
/* Writes c->out_len bytes. */
void wf_sha512_final(wf_sha512_ctx_t* c, uint8_t* out);
void wf_sha384(const uint8_t* d, uint32_t n, uint8_t out[48]);
void wf_sha512(const uint8_t* d, uint32_t n, uint8_t out[64]);

/* One-shot digest by id: WF_HASH_SHA256, WF_HASH_SHA384, WF_HASH_SHA512.
 * Returns the digest length or -1. */
#define WF_HASH_SHA256 1
#define WF_HASH_SHA384 2
#define WF_HASH_SHA512 3
#define WF_HASH_SHA1 4   /* recognised only to be refused (no SHA-1 trust) */
int wf_hash(int id, const uint8_t* d, uint32_t n, uint8_t* out);
#endif
