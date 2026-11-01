/* wf_aead.h - Lot 72: AEAD ciphers for the Ring 3 TLS client, freestanding.
 *   WF_AEAD_AES128_GCM, WF_AEAD_AES256_GCM (NIST SP 800-38D, 96-bit nonce)
 *   WF_AEAD_CHACHA20_POLY1305 (RFC 8439)
 * One context per direction, no allocation. GHASH, ChaCha20 and Poly1305
 * are written without secret-dependent branches or table lookups; the AES
 * rounds use the classic S-box table (lookups indexed by secret bytes, so
 * not hardened against cache-timing on shared hardware - documented). */
#ifndef WF_AEAD_H
#define WF_AEAD_H
#include <stdint.h>

#define WF_AEAD_AES128_GCM 1
#define WF_AEAD_AES256_GCM 2
#define WF_AEAD_CHACHA20_POLY1305 3
#define WF_AEAD_TAG 16

typedef struct {
    int alg;
    int rounds;                 /* AES: 10 or 14 */
    uint8_t rk[240];            /* AES round keys */
    uint32_t h[4];              /* GHASH key, big-endian words */
    uint8_t key[32];            /* ChaCha20 key */
} wf_aead_t;

/* Key length in bytes for alg (16 or 32), 0 if unknown. */
int wf_aead_key_len(int alg);
const char* wf_aead_name(int alg);
/* 0 or -1 (unknown alg). */
int wf_aead_init(wf_aead_t* c, int alg, const uint8_t* key);
/* ct may equal pt. Writes n bytes and a 16-byte tag. 0 or -1. */
int wf_aead_seal(const wf_aead_t* c, const uint8_t nonce[12], const uint8_t* aad, int aad_len,
                 const uint8_t* pt, int n, uint8_t* ct, uint8_t tag[16]);
/* Checks the tag first (constant time), then decrypts. 0 or -1; on -1
 * nothing is written to pt. pt may equal ct. */
int wf_aead_open(const wf_aead_t* c, const uint8_t nonce[12], const uint8_t* aad, int aad_len,
                 const uint8_t* ct, int n, const uint8_t tag[16], uint8_t* pt);
void wf_aead_wipe(wf_aead_t* c);

/* Building blocks, exported for the known-answer tests. */
void wf_aes_encrypt_block(const wf_aead_t* c, const uint8_t in[16], uint8_t out[16]);
void wf_chacha20_block(const uint8_t key[32], uint32_t counter, const uint8_t nonce[12], uint8_t out[64]);
void wf_poly1305(const uint8_t key[32], const uint8_t* m, int n, uint8_t tag[16]);
#endif
