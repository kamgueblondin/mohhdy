/* wf_sha512.c - Lot 67: SHA-512 / SHA-384. */
#include "wf_sha512.h"
#include "../kernel/sha256.h"

static const uint64_t K[80] = {
    0x428a2f98d728ae22ULL, 0x7137449123ef65cdULL, 0xb5c0fbcfec4d3b2fULL, 0xe9b5dba58189dbbcULL,
    0x3956c25bf348b538ULL, 0x59f111f1b605d019ULL, 0x923f82a4af194f9bULL, 0xab1c5ed5da6d8118ULL,
    0xd807aa98a3030242ULL, 0x12835b0145706fbeULL, 0x243185be4ee4b28cULL, 0x550c7dc3d5ffb4e2ULL,
    0x72be5d74f27b896fULL, 0x80deb1fe3b1696b1ULL, 0x9bdc06a725c71235ULL, 0xc19bf174cf692694ULL,
    0xe49b69c19ef14ad2ULL, 0xefbe4786384f25e3ULL, 0x0fc19dc68b8cd5b5ULL, 0x240ca1cc77ac9c65ULL,
    0x2de92c6f592b0275ULL, 0x4a7484aa6ea6e483ULL, 0x5cb0a9dcbd41fbd4ULL, 0x76f988da831153b5ULL,
    0x983e5152ee66dfabULL, 0xa831c66d2db43210ULL, 0xb00327c898fb213fULL, 0xbf597fc7beef0ee4ULL,
    0xc6e00bf33da88fc2ULL, 0xd5a79147930aa725ULL, 0x06ca6351e003826fULL, 0x142929670a0e6e70ULL,
    0x27b70a8546d22ffcULL, 0x2e1b21385c26c926ULL, 0x4d2c6dfc5ac42aedULL, 0x53380d139d95b3dfULL,
    0x650a73548baf63deULL, 0x766a0abb3c77b2a8ULL, 0x81c2c92e47edaee6ULL, 0x92722c851482353bULL,
    0xa2bfe8a14cf10364ULL, 0xa81a664bbc423001ULL, 0xc24b8b70d0f89791ULL, 0xc76c51a30654be30ULL,
    0xd192e819d6ef5218ULL, 0xd69906245565a910ULL, 0xf40e35855771202aULL, 0x106aa07032bbd1b8ULL,
    0x19a4c116b8d2d0c8ULL, 0x1e376c085141ab53ULL, 0x2748774cdf8eeb99ULL, 0x34b0bcb5e19b48a8ULL,
    0x391c0cb3c5c95a63ULL, 0x4ed8aa4ae3418acbULL, 0x5b9cca4f7763e373ULL, 0x682e6ff3d6b2b8a3ULL,
    0x748f82ee5defb2fcULL, 0x78a5636f43172f60ULL, 0x84c87814a1f0ab72ULL, 0x8cc702081a6439ecULL,
    0x90befffa23631e28ULL, 0xa4506cebde82bde9ULL, 0xbef9a3f7b2c67915ULL, 0xc67178f2e372532bULL,
    0xca273eceea26619cULL, 0xd186b8c721c0c207ULL, 0xeada7dd6cde0eb1eULL, 0xf57d4f7fee6ed178ULL,
    0x06f067aa72176fbaULL, 0x0a637dc5a2c898a6ULL, 0x113f9804bef90daeULL, 0x1b710b35131c471bULL,
    0x28db77f523047d84ULL, 0x32caab7b40c72493ULL, 0x3c9ebe0a15c9bebcULL, 0x431d67c49c100d4cULL,
    0x4cc5d4becb3e42b6ULL, 0x597f299cfc657e2aULL, 0x5fcb6fab3ad6faecULL, 0x6c44198c4a475817ULL,
};

#define ROTR(x, n) (((x) >> (n)) | ((x) << (64 - (n))))

static void compress(wf_sha512_ctx_t* c, const uint8_t* p) {
    uint64_t w[80], a, b, cc, d, e, f, g, h, t1, t2;
    int i;
    for (i = 0; i < 16; i++) {
        int k;
        w[i] = 0;
        for (k = 0; k < 8; k++) w[i] = (w[i] << 8) | p[i * 8 + k];
    }
    for (i = 16; i < 80; i++) {
        uint64_t s0 = ROTR(w[i - 15], 1) ^ ROTR(w[i - 15], 8) ^ (w[i - 15] >> 7);
        uint64_t s1 = ROTR(w[i - 2], 19) ^ ROTR(w[i - 2], 61) ^ (w[i - 2] >> 6);
        w[i] = w[i - 16] + s0 + w[i - 7] + s1;
    }
    a = c->h[0]; b = c->h[1]; cc = c->h[2]; d = c->h[3];
    e = c->h[4]; f = c->h[5]; g = c->h[6]; h = c->h[7];
    for (i = 0; i < 80; i++) {
        t1 = h + (ROTR(e, 14) ^ ROTR(e, 18) ^ ROTR(e, 41)) + ((e & f) ^ (~e & g)) + K[i] + w[i];
        t2 = (ROTR(a, 28) ^ ROTR(a, 34) ^ ROTR(a, 39)) + ((a & b) ^ (a & cc) ^ (b & cc));
        h = g; g = f; f = e; e = d + t1; d = cc; cc = b; b = a; a = t1 + t2;
    }
    c->h[0] += a; c->h[1] += b; c->h[2] += cc; c->h[3] += d;
    c->h[4] += e; c->h[5] += f; c->h[6] += g; c->h[7] += h;
}

void wf_sha512_init(wf_sha512_ctx_t* c) {
    static const uint64_t iv[8] = {
        0x6a09e667f3bcc908ULL, 0xbb67ae8584caa73bULL, 0x3c6ef372fe94f82bULL, 0xa54ff53a5f1d36f1ULL,
        0x510e527fade682d1ULL, 0x9b05688c2b3e6c1fULL, 0x1f83d9abfb41bd6bULL, 0x5be0cd19137e2179ULL};
    int i;
    for (i = 0; i < 8; i++) c->h[i] = iv[i];
    c->bytes = 0; c->used = 0; c->out_len = 64;
}

void wf_sha384_init(wf_sha512_ctx_t* c) {
    static const uint64_t iv[8] = {
        0xcbbb9d5dc1059ed8ULL, 0x629a292a367cd507ULL, 0x9159015a3070dd17ULL, 0x152fecd8f70e5939ULL,
        0x67332667ffc00b31ULL, 0x8eb44a8768581511ULL, 0xdb0c2e0d64f98fa7ULL, 0x47b5481dbefa4fa4ULL};
    int i;
    for (i = 0; i < 8; i++) c->h[i] = iv[i];
    c->bytes = 0; c->used = 0; c->out_len = 48;
}

void wf_sha512_update(wf_sha512_ctx_t* c, const uint8_t* d, uint32_t n) {
    uint32_t i;
    for (i = 0; i < n; i++) {
        c->block[c->used++] = d[i];
        if (c->used == 128U) { compress(c, c->block); c->used = 0; }
    }
    c->bytes += n;
}

void wf_sha512_final(wf_sha512_ctx_t* c, uint8_t* out) {
    uint64_t bits = c->bytes * 8U;
    uint32_t i;
    c->block[c->used++] = 0x80U;
    if (c->used > 112U) {
        while (c->used < 128U) c->block[c->used++] = 0;
        compress(c, c->block);
        c->used = 0;
    }
    while (c->used < 120U) c->block[c->used++] = 0;
    for (i = 0; i < 8; i++) c->block[120 + i] = (uint8_t)(bits >> (56 - 8 * i));
    compress(c, c->block);
    for (i = 0; i < c->out_len; i++) out[i] = (uint8_t)(c->h[i / 8] >> (56 - 8 * (i % 8)));
}

void wf_sha384(const uint8_t* d, uint32_t n, uint8_t out[48]) {
    wf_sha512_ctx_t c;
    wf_sha384_init(&c); wf_sha512_update(&c, d, n); wf_sha512_final(&c, out);
}

void wf_sha512(const uint8_t* d, uint32_t n, uint8_t out[64]) {
    wf_sha512_ctx_t c;
    wf_sha512_init(&c); wf_sha512_update(&c, d, n); wf_sha512_final(&c, out);
}

int wf_hash(int id, const uint8_t* d, uint32_t n, uint8_t* out) {
    if (id == WF_HASH_SHA256) {
        sha256_ctx_t c;
        sha256_init(&c); sha256_update(&c, d, n); sha256_final(&c, out);
        return 32;
    }
    if (id == WF_HASH_SHA384) { wf_sha384(d, n, out); return 48; }
    if (id == WF_HASH_SHA512) { wf_sha512(d, n, out); return 64; }
    return -1;
}
