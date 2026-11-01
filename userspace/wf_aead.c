/* wf_aead.c - Lot 72: AES-GCM (128/256) and ChaCha20-Poly1305 (see wf_aead.h). */
#include "wf_aead.h"

static void mcpy(uint8_t* d, const uint8_t* s, int n) { int i; for (i = 0; i < n; i++) d[i] = s[i]; }
static uint32_t be32(const uint8_t* p) { return ((uint32_t)p[0] << 24) | ((uint32_t)p[1] << 16) | ((uint32_t)p[2] << 8) | p[3]; }
static void pbe32(uint8_t* p, uint32_t v) { p[0] = (uint8_t)(v >> 24); p[1] = (uint8_t)(v >> 16); p[2] = (uint8_t)(v >> 8); p[3] = (uint8_t)v; }
static uint32_t le32(const uint8_t* p) { return (uint32_t)p[0] | ((uint32_t)p[1] << 8) | ((uint32_t)p[2] << 16) | ((uint32_t)p[3] << 24); }
static void ple32(uint8_t* p, uint32_t v) { p[0] = (uint8_t)v; p[1] = (uint8_t)(v >> 8); p[2] = (uint8_t)(v >> 16); p[3] = (uint8_t)(v >> 24); }

int wf_aead_key_len(int alg) {
    if (alg == WF_AEAD_AES128_GCM) return 16;
    if (alg == WF_AEAD_AES256_GCM || alg == WF_AEAD_CHACHA20_POLY1305) return 32;
    return 0;
}

const char* wf_aead_name(int alg) {
    if (alg == WF_AEAD_AES128_GCM) return "AES128-GCM";
    if (alg == WF_AEAD_AES256_GCM) return "AES256-GCM";
    if (alg == WF_AEAD_CHACHA20_POLY1305) return "CHACHA20-POLY1305";
    return "none";
}

/* ---------------------------------------------------------------- AES */

static const uint8_t SBOX[256] = {
    0x63,0x7c,0x77,0x7b,0xf2,0x6b,0x6f,0xc5,0x30,0x01,0x67,0x2b,0xfe,0xd7,0xab,0x76,
    0xca,0x82,0xc9,0x7d,0xfa,0x59,0x47,0xf0,0xad,0xd4,0xa2,0xaf,0x9c,0xa4,0x72,0xc0,
    0xb7,0xfd,0x93,0x26,0x36,0x3f,0xf7,0xcc,0x34,0xa5,0xe5,0xf1,0x71,0xd8,0x31,0x15,
    0x04,0xc7,0x23,0xc3,0x18,0x96,0x05,0x9a,0x07,0x12,0x80,0xe2,0xeb,0x27,0xb2,0x75,
    0x09,0x83,0x2c,0x1a,0x1b,0x6e,0x5a,0xa0,0x52,0x3b,0xd6,0xb3,0x29,0xe3,0x2f,0x84,
    0x53,0xd1,0x00,0xed,0x20,0xfc,0xb1,0x5b,0x6a,0xcb,0xbe,0x39,0x4a,0x4c,0x58,0xcf,
    0xd0,0xef,0xaa,0xfb,0x43,0x4d,0x33,0x85,0x45,0xf9,0x02,0x7f,0x50,0x3c,0x9f,0xa8,
    0x51,0xa3,0x40,0x8f,0x92,0x9d,0x38,0xf5,0xbc,0xb6,0xda,0x21,0x10,0xff,0xf3,0xd2,
    0xcd,0x0c,0x13,0xec,0x5f,0x97,0x44,0x17,0xc4,0xa7,0x7e,0x3d,0x64,0x5d,0x19,0x73,
    0x60,0x81,0x4f,0xdc,0x22,0x2a,0x90,0x88,0x46,0xee,0xb8,0x14,0xde,0x5e,0x0b,0xdb,
    0xe0,0x32,0x3a,0x0a,0x49,0x06,0x24,0x5c,0xc2,0xd3,0xac,0x62,0x91,0x95,0xe4,0x79,
    0xe7,0xc8,0x37,0x6d,0x8d,0xd5,0x4e,0xa9,0x6c,0x56,0xf4,0xea,0x65,0x7a,0xae,0x08,
    0xba,0x78,0x25,0x2e,0x1c,0xa6,0xb4,0xc6,0xe8,0xdd,0x74,0x1f,0x4b,0xbd,0x8b,0x8a,
    0x70,0x3e,0xb5,0x66,0x48,0x03,0xf6,0x0e,0x61,0x35,0x57,0xb9,0x86,0xc1,0x1d,0x9e,
    0xe1,0xf8,0x98,0x11,0x69,0xd9,0x8e,0x94,0x9b,0x1e,0x87,0xe9,0xce,0x55,0x28,0xdf,
    0x8c,0xa1,0x89,0x0d,0xbf,0xe6,0x42,0x68,0x41,0x99,0x2d,0x0f,0xb0,0x54,0xbb,0x16};

static uint8_t xt(uint8_t x) { return (uint8_t)((x << 1) ^ (0x1b & (uint8_t)-(x >> 7))); }

static void aes_expand(wf_aead_t* c, const uint8_t* key, int nk) {
    int i, words = 4 * (c->rounds + 1);
    uint8_t rcon = 1, t[4];
    mcpy(c->rk, key, 4 * nk);
    for (i = nk; i < words; i++) {
        t[0] = c->rk[4 * i - 4]; t[1] = c->rk[4 * i - 3]; t[2] = c->rk[4 * i - 2]; t[3] = c->rk[4 * i - 1];
        if (i % nk == 0) {
            uint8_t u = t[0];
            t[0] = (uint8_t)(SBOX[t[1]] ^ rcon); t[1] = SBOX[t[2]]; t[2] = SBOX[t[3]]; t[3] = SBOX[u];
            rcon = xt(rcon);
        } else if (nk > 6 && i % nk == 4) {
            t[0] = SBOX[t[0]]; t[1] = SBOX[t[1]]; t[2] = SBOX[t[2]]; t[3] = SBOX[t[3]];
        }
        c->rk[4 * i + 0] = (uint8_t)(c->rk[4 * (i - nk) + 0] ^ t[0]);
        c->rk[4 * i + 1] = (uint8_t)(c->rk[4 * (i - nk) + 1] ^ t[1]);
        c->rk[4 * i + 2] = (uint8_t)(c->rk[4 * (i - nk) + 2] ^ t[2]);
        c->rk[4 * i + 3] = (uint8_t)(c->rk[4 * (i - nk) + 3] ^ t[3]);
    }
}

void wf_aes_encrypt_block(const wf_aead_t* c, const uint8_t in[16], uint8_t out[16]) {
    uint8_t s[16], t[16];
    int r, i;
    for (i = 0; i < 16; i++) s[i] = (uint8_t)(in[i] ^ c->rk[i]);
    for (r = 1; r <= c->rounds; r++) {
        /* SubBytes + ShiftRows (column-major state) */
        for (i = 0; i < 16; i++) t[i] = SBOX[s[(i + 4 * (i & 3)) & 15]];
        if (r != c->rounds) {
            for (i = 0; i < 4; i++) {
                uint8_t a0 = t[4 * i], a1 = t[4 * i + 1], a2 = t[4 * i + 2], a3 = t[4 * i + 3];
                uint8_t all = (uint8_t)(a0 ^ a1 ^ a2 ^ a3);
                s[4 * i + 0] = (uint8_t)(a0 ^ all ^ xt((uint8_t)(a0 ^ a1)));
                s[4 * i + 1] = (uint8_t)(a1 ^ all ^ xt((uint8_t)(a1 ^ a2)));
                s[4 * i + 2] = (uint8_t)(a2 ^ all ^ xt((uint8_t)(a2 ^ a3)));
                s[4 * i + 3] = (uint8_t)(a3 ^ all ^ xt((uint8_t)(a3 ^ a0)));
            }
        } else {
            mcpy(s, t, 16);
        }
        for (i = 0; i < 16; i++) s[i] ^= c->rk[16 * r + i];
    }
    mcpy(out, s, 16);
}

/* ---------------------------------------------------------------- GHASH */

/* X = X * H in GF(2^128), bit-serial with masks (no secret branches). */
static void gmul(uint32_t x[4], const uint32_t h[4]) {
    uint32_t z0 = 0, z1 = 0, z2 = 0, z3 = 0, v0 = h[0], v1 = h[1], v2 = h[2], v3 = h[3];
    int i, j;
    for (i = 0; i < 4; i++) {
        uint32_t w = x[i];
        for (j = 31; j >= 0; j--) {
            uint32_t m = (uint32_t)0 - ((w >> j) & 1U), lsb;
            z0 ^= v0 & m; z1 ^= v1 & m; z2 ^= v2 & m; z3 ^= v3 & m;
            lsb = (uint32_t)0 - (v3 & 1U);
            v3 = (v3 >> 1) | (v2 << 31);
            v2 = (v2 >> 1) | (v1 << 31);
            v1 = (v1 >> 1) | (v0 << 31);
            v0 = (v0 >> 1) ^ (0xe1000000U & lsb);
        }
    }
    x[0] = z0; x[1] = z1; x[2] = z2; x[3] = z3;
}

static void ghash_update(uint32_t y[4], const uint32_t h[4], const uint8_t* d, int n) {
    while (n > 0) {
        uint8_t b[16] = {0};
        int k = n < 16 ? n : 16;
        mcpy(b, d, k);
        y[0] ^= be32(b); y[1] ^= be32(b + 4); y[2] ^= be32(b + 8); y[3] ^= be32(b + 12);
        gmul(y, h);
        d += k; n -= k;
    }
}

static void gcm_ctr(const wf_aead_t* c, const uint8_t nonce[12], const uint8_t* in, int n, uint8_t* out) {
    uint8_t ctr[16], ks[16];
    uint32_t cnt = 2;
    int i, k;
    mcpy(ctr, nonce, 12);
    while (n > 0) {
        pbe32(ctr + 12, cnt++);
        wf_aes_encrypt_block(c, ctr, ks);
        k = n < 16 ? n : 16;
        for (i = 0; i < k; i++) out[i] = (uint8_t)(in[i] ^ ks[i]);
        in += k; out += k; n -= k;
    }
}

static void gcm_tag(const wf_aead_t* c, const uint8_t nonce[12], const uint8_t* aad, int al, const uint8_t* ct, int n, uint8_t tag[16]) {
    uint32_t y[4] = {0, 0, 0, 0};
    uint8_t j0[16], ek[16], lens[16];
    int i;
    ghash_update(y, c->h, aad, al);
    ghash_update(y, c->h, ct, n);
    pbe32(lens, 0); pbe32(lens + 4, (uint32_t)al << 3);
    pbe32(lens + 8, 0); pbe32(lens + 12, (uint32_t)n << 3);
    ghash_update(y, c->h, lens, 16);
    mcpy(j0, nonce, 12); pbe32(j0 + 12, 1);
    wf_aes_encrypt_block(c, j0, ek);
    for (i = 0; i < 4; i++) pbe32(tag + 4 * i, y[i]);
    for (i = 0; i < 16; i++) tag[i] ^= ek[i];
}

/* ---------------------------------------------------------------- ChaCha20 */

#define ROTL(v, n) (((v) << (n)) | ((v) >> (32 - (n))))
#define QR(a, b, c, d) \
    a += b; d ^= a; d = ROTL(d, 16); c += d; b ^= c; b = ROTL(b, 12); \
    a += b; d ^= a; d = ROTL(d, 8);  c += d; b ^= c; b = ROTL(b, 7)

void wf_chacha20_block(const uint8_t key[32], uint32_t counter, const uint8_t nonce[12], uint8_t out[64]) {
    uint32_t s[16], x[16];
    int i;
    s[0] = 0x61707865U; s[1] = 0x3320646eU; s[2] = 0x79622d32U; s[3] = 0x6b206574U;
    for (i = 0; i < 8; i++) s[4 + i] = le32(key + 4 * i);
    s[12] = counter;
    s[13] = le32(nonce); s[14] = le32(nonce + 4); s[15] = le32(nonce + 8);
    for (i = 0; i < 16; i++) x[i] = s[i];
    for (i = 0; i < 10; i++) {
        QR(x[0], x[4], x[8], x[12]); QR(x[1], x[5], x[9], x[13]); QR(x[2], x[6], x[10], x[14]); QR(x[3], x[7], x[11], x[15]);
        QR(x[0], x[5], x[10], x[15]); QR(x[1], x[6], x[11], x[12]); QR(x[2], x[7], x[8], x[13]); QR(x[3], x[4], x[9], x[14]);
    }
    for (i = 0; i < 16; i++) ple32(out + 4 * i, x[i] + s[i]);
}

static void chacha_xor(const uint8_t key[32], const uint8_t nonce[12], uint32_t ctr, const uint8_t* in, int n, uint8_t* out) {
    uint8_t ks[64];
    int i, k;
    while (n > 0) {
        wf_chacha20_block(key, ctr++, nonce, ks);
        k = n < 64 ? n : 64;
        for (i = 0; i < k; i++) out[i] = (uint8_t)(in[i] ^ ks[i]);
        in += k; out += k; n -= k;
    }
}

/* ---------------------------------------------------------------- Poly1305 */

/* 26-bit limbs, 32x32->64 products only (poly1305-donna style). */
typedef struct { uint32_t r[5], h[5], pad[4]; } poly_t;

static void poly_init(poly_t* p, const uint8_t key[32]) {
    p->r[0] = (le32(key + 0)) & 0x3ffffffU;
    p->r[1] = (le32(key + 3) >> 2) & 0x3ffff03U;
    p->r[2] = (le32(key + 6) >> 4) & 0x3ffc0ffU;
    p->r[3] = (le32(key + 9) >> 6) & 0x3f03fffU;
    p->r[4] = (le32(key + 12) >> 8) & 0x00fffffU;
    p->h[0] = p->h[1] = p->h[2] = p->h[3] = p->h[4] = 0;
    p->pad[0] = le32(key + 16); p->pad[1] = le32(key + 20); p->pad[2] = le32(key + 24); p->pad[3] = le32(key + 28);
}

static void poly_block(poly_t* p, const uint8_t m[16], uint32_t hibit) {
    uint32_t r0 = p->r[0], r1 = p->r[1], r2 = p->r[2], r3 = p->r[3], r4 = p->r[4];
    uint32_t s1 = r1 * 5, s2 = r2 * 5, s3 = r3 * 5, s4 = r4 * 5;
    uint32_t h0 = p->h[0], h1 = p->h[1], h2 = p->h[2], h3 = p->h[3], h4 = p->h[4], c;
    uint64_t d0, d1, d2, d3, d4;
    h0 += (le32(m + 0)) & 0x3ffffffU;
    h1 += (le32(m + 3) >> 2) & 0x3ffffffU;
    h2 += (le32(m + 6) >> 4) & 0x3ffffffU;
    h3 += (le32(m + 9) >> 6) & 0x3ffffffU;
    h4 += (le32(m + 12) >> 8) | hibit;
    d0 = (uint64_t)h0 * r0 + (uint64_t)h1 * s4 + (uint64_t)h2 * s3 + (uint64_t)h3 * s2 + (uint64_t)h4 * s1;
    d1 = (uint64_t)h0 * r1 + (uint64_t)h1 * r0 + (uint64_t)h2 * s4 + (uint64_t)h3 * s3 + (uint64_t)h4 * s2;
    d2 = (uint64_t)h0 * r2 + (uint64_t)h1 * r1 + (uint64_t)h2 * r0 + (uint64_t)h3 * s4 + (uint64_t)h4 * s3;
    d3 = (uint64_t)h0 * r3 + (uint64_t)h1 * r2 + (uint64_t)h2 * r1 + (uint64_t)h3 * r0 + (uint64_t)h4 * s4;
    d4 = (uint64_t)h0 * r4 + (uint64_t)h1 * r3 + (uint64_t)h2 * r2 + (uint64_t)h3 * r1 + (uint64_t)h4 * r0;
    c = (uint32_t)(d0 >> 26); h0 = (uint32_t)d0 & 0x3ffffffU;
    d1 += c; c = (uint32_t)(d1 >> 26); h1 = (uint32_t)d1 & 0x3ffffffU;
    d2 += c; c = (uint32_t)(d2 >> 26); h2 = (uint32_t)d2 & 0x3ffffffU;
    d3 += c; c = (uint32_t)(d3 >> 26); h3 = (uint32_t)d3 & 0x3ffffffU;
    d4 += c; c = (uint32_t)(d4 >> 26); h4 = (uint32_t)d4 & 0x3ffffffU;
    h0 += c * 5; c = h0 >> 26; h0 &= 0x3ffffffU;
    h1 += c;
    p->h[0] = h0; p->h[1] = h1; p->h[2] = h2; p->h[3] = h3; p->h[4] = h4;
}

/* Data padded with zeros to 16 bytes (the AEAD layout), each block with the 2^128 bit. */
static void poly_padded(poly_t* p, const uint8_t* d, int n) {
    while (n > 0) {
        uint8_t b[16] = {0};
        int k = n < 16 ? n : 16;
        mcpy(b, d, k);
        poly_block(p, b, 1U << 24);
        d += k; n -= k;
    }
}

static void poly_finish(poly_t* p, uint8_t tag[16]) {
    uint32_t h0 = p->h[0], h1 = p->h[1], h2 = p->h[2], h3 = p->h[3], h4 = p->h[4], c;
    uint32_t g0, g1, g2, g3, g4, mask;
    uint64_t f;
    c = h1 >> 26; h1 &= 0x3ffffffU;
    h2 += c; c = h2 >> 26; h2 &= 0x3ffffffU;
    h3 += c; c = h3 >> 26; h3 &= 0x3ffffffU;
    h4 += c; c = h4 >> 26; h4 &= 0x3ffffffU;
    h0 += c * 5; c = h0 >> 26; h0 &= 0x3ffffffU;
    h1 += c;
    g0 = h0 + 5; c = g0 >> 26; g0 &= 0x3ffffffU;
    g1 = h1 + c; c = g1 >> 26; g1 &= 0x3ffffffU;
    g2 = h2 + c; c = g2 >> 26; g2 &= 0x3ffffffU;
    g3 = h3 + c; c = g3 >> 26; g3 &= 0x3ffffffU;
    g4 = h4 + c - (1U << 26);
    mask = (g4 >> 31) - 1U;          /* all ones when h >= p */
    g0 &= mask; g1 &= mask; g2 &= mask; g3 &= mask; g4 &= mask;
    mask = ~mask;
    h0 = (h0 & mask) | g0; h1 = (h1 & mask) | g1; h2 = (h2 & mask) | g2; h3 = (h3 & mask) | g3; h4 = (h4 & mask) | g4;
    h0 = (h0) | (h1 << 26);
    h1 = (h1 >> 6) | (h2 << 20);
    h2 = (h2 >> 12) | (h3 << 14);
    h3 = (h3 >> 18) | (h4 << 8);
    f = (uint64_t)h0 + p->pad[0]; h0 = (uint32_t)f;
    f = (uint64_t)h1 + p->pad[1] + (f >> 32); h1 = (uint32_t)f;
    f = (uint64_t)h2 + p->pad[2] + (f >> 32); h2 = (uint32_t)f;
    f = (uint64_t)h3 + p->pad[3] + (f >> 32); h3 = (uint32_t)f;
    ple32(tag, h0); ple32(tag + 4, h1); ple32(tag + 8, h2); ple32(tag + 12, h3);
}

void wf_poly1305(const uint8_t key[32], const uint8_t* m, int n, uint8_t tag[16]) {
    poly_t p;
    poly_init(&p, key);
    while (n >= 16) { poly_block(&p, m, 1U << 24); m += 16; n -= 16; }
    if (n > 0) {
        uint8_t b[16] = {0};
        mcpy(b, m, n);
        b[n] = 1;
        poly_block(&p, b, 0);
    }
    poly_finish(&p, tag);
}

static void chapoly_tag(const uint8_t key[32], const uint8_t nonce[12], const uint8_t* aad, int al, const uint8_t* ct, int n, uint8_t tag[16]) {
    uint8_t blk[64], lens[16];
    poly_t p;
    wf_chacha20_block(key, 0, nonce, blk);
    poly_init(&p, blk);
    poly_padded(&p, aad, al);
    poly_padded(&p, ct, n);
    ple32(lens, (uint32_t)al); ple32(lens + 4, 0); ple32(lens + 8, (uint32_t)n); ple32(lens + 12, 0);
    poly_block(&p, lens, 1U << 24);
    poly_finish(&p, tag);
}

/* ---------------------------------------------------------------- AEAD */

int wf_aead_init(wf_aead_t* c, int alg, const uint8_t* key) {
    int i;
    for (i = 0; i < (int)sizeof(*c); i++) ((uint8_t*)c)[i] = 0;
    c->alg = alg;
    if (alg == WF_AEAD_AES128_GCM || alg == WF_AEAD_AES256_GCM) {
        uint8_t zero[16] = {0}, h[16];
        int nk = alg == WF_AEAD_AES128_GCM ? 4 : 8;
        c->rounds = nk + 6;
        aes_expand(c, key, nk);
        wf_aes_encrypt_block(c, zero, h);
        for (i = 0; i < 4; i++) c->h[i] = be32(h + 4 * i);
        return 0;
    }
    if (alg == WF_AEAD_CHACHA20_POLY1305) { mcpy(c->key, key, 32); return 0; }
    c->alg = 0;
    return -1;
}

int wf_aead_seal(const wf_aead_t* c, const uint8_t nonce[12], const uint8_t* aad, int al,
                 const uint8_t* pt, int n, uint8_t* ct, uint8_t tag[16]) {
    if (n < 0 || al < 0) return -1;
    if (c->alg == WF_AEAD_CHACHA20_POLY1305) {
        chacha_xor(c->key, nonce, 1, pt, n, ct);
        chapoly_tag(c->key, nonce, aad, al, ct, n, tag);
        return 0;
    }
    if (c->alg != WF_AEAD_AES128_GCM && c->alg != WF_AEAD_AES256_GCM) return -1;
    gcm_ctr(c, nonce, pt, n, ct);
    gcm_tag(c, nonce, aad, al, ct, n, tag);
    return 0;
}

int wf_aead_open(const wf_aead_t* c, const uint8_t nonce[12], const uint8_t* aad, int al,
                 const uint8_t* ct, int n, const uint8_t tag[16], uint8_t* pt) {
    uint8_t want[16], d = 0;
    int i;
    if (n < 0 || al < 0) return -1;
    if (c->alg == WF_AEAD_CHACHA20_POLY1305) chapoly_tag(c->key, nonce, aad, al, ct, n, want);
    else if (c->alg == WF_AEAD_AES128_GCM || c->alg == WF_AEAD_AES256_GCM) gcm_tag(c, nonce, aad, al, ct, n, want);
    else return -1;
    for (i = 0; i < 16; i++) d |= (uint8_t)(want[i] ^ tag[i]);
    if (d) return -1;
    if (c->alg == WF_AEAD_CHACHA20_POLY1305) chacha_xor(c->key, nonce, 1, ct, n, pt);
    else gcm_ctr(c, nonce, ct, n, pt);
    return 0;
}

void wf_aead_wipe(wf_aead_t* c) {
    volatile uint8_t* p = (volatile uint8_t*)c;
    int i;
    for (i = 0; i < (int)sizeof(*c); i++) p[i] = 0;
}
