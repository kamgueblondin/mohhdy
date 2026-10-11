/* userspace/csig.c - per-node Schnorr signatures for the collab ledger.
 * Group: RFC 5114 section 2.1 (1024-bit MODP prime p, 160-bit prime-order
 * subgroup q, generator g), checked: q | p-1 and g^q = 1 mod p.
 * Deterministic nonce k = SHA-256(sk || msg) mod q (EdDSA style, no RNG
 * needed for signing). Signature = e || s, 20 bytes each:
 *   r = g^k, e = SHA-256(r || msg) mod q, s = k - x*e mod q
 *   verify: r' = g^s * y^e mod p, accept iff SHA-256(r' || msg) mod q == e.
 * Uses the same bigint code as the guest TLS stack. Not constant time. */
#include "csig.h"
#include "../kernel/bigint.h"
#include "../kernel/sha256.h"

static const uint8_t k_p[128] = {
    0xb1, 0x0b, 0x8f, 0x96, 0xa0, 0x80, 0xe0, 0x1d, 0xde, 0x92, 0xde, 0x5e, 0xae, 0x5d, 0x54, 0xec,
    0x52, 0xc9, 0x9f, 0xbc, 0xfb, 0x06, 0xa3, 0xc6, 0x9a, 0x6a, 0x9d, 0xca, 0x52, 0xd2, 0x3b, 0x61,
    0x60, 0x73, 0xe2, 0x86, 0x75, 0xa2, 0x3d, 0x18, 0x98, 0x38, 0xef, 0x1e, 0x2e, 0xe6, 0x52, 0xc0,
    0x13, 0xec, 0xb4, 0xae, 0xa9, 0x06, 0x11, 0x23, 0x24, 0x97, 0x5c, 0x3c, 0xd4, 0x9b, 0x83, 0xbf,
    0xac, 0xcb, 0xdd, 0x7d, 0x90, 0xc4, 0xbd, 0x70, 0x98, 0x48, 0x8e, 0x9c, 0x21, 0x9a, 0x73, 0x72,
    0x4e, 0xff, 0xd6, 0xfa, 0xe5, 0x64, 0x47, 0x38, 0xfa, 0xa3, 0x1a, 0x4f, 0xf5, 0x5b, 0xcc, 0xc0,
    0xa1, 0x51, 0xaf, 0x5f, 0x0d, 0xc8, 0xb4, 0xbd, 0x45, 0xbf, 0x37, 0xdf, 0x36, 0x5c, 0x1a, 0x65,
    0xe6, 0x8c, 0xfd, 0xa7, 0x6d, 0x4d, 0xa7, 0x08, 0xdf, 0x1f, 0xb2, 0xbc, 0x2e, 0x4a, 0x43, 0x71,
};
static const uint8_t k_g[128] = {
    0xa4, 0xd1, 0xcb, 0xd5, 0xc3, 0xfd, 0x34, 0x12, 0x67, 0x65, 0xa4, 0x42, 0xef, 0xb9, 0x99, 0x05,
    0xf8, 0x10, 0x4d, 0xd2, 0x58, 0xac, 0x50, 0x7f, 0xd6, 0x40, 0x6c, 0xff, 0x14, 0x26, 0x6d, 0x31,
    0x26, 0x6f, 0xea, 0x1e, 0x5c, 0x41, 0x56, 0x4b, 0x77, 0x7e, 0x69, 0x0f, 0x55, 0x04, 0xf2, 0x13,
    0x16, 0x02, 0x17, 0xb4, 0xb0, 0x1b, 0x88, 0x6a, 0x5e, 0x91, 0x54, 0x7f, 0x9e, 0x27, 0x49, 0xf4,
    0xd7, 0xfb, 0xd7, 0xd3, 0xb9, 0xa9, 0x2e, 0xe1, 0x90, 0x9d, 0x0d, 0x22, 0x63, 0xf8, 0x0a, 0x76,
    0xa6, 0xa2, 0x4c, 0x08, 0x7a, 0x09, 0x1f, 0x53, 0x1d, 0xbf, 0x0a, 0x01, 0x69, 0xb6, 0xa2, 0x8a,
    0xd6, 0x62, 0xa4, 0xd1, 0x8e, 0x73, 0xaf, 0xa3, 0x2d, 0x77, 0x9d, 0x59, 0x18, 0xd0, 0x8b, 0xc8,
    0x85, 0x8f, 0x4d, 0xce, 0xf9, 0x7c, 0x2a, 0x24, 0x85, 0x5e, 0x6e, 0xeb, 0x22, 0xb3, 0xb2, 0xe5,
};
static const uint8_t k_q[20] = {
    0xf5, 0x18, 0xaa, 0x87, 0x81, 0xa8, 0xdf, 0x27, 0x8a, 0xba, 0x4e, 0x7d, 0x64, 0xb7, 0xcb, 0x9d,
    0x49, 0x46, 0x23, 0x53,
};

#define CAP 68
typedef struct { uint32_t l[CAP]; bigint_t b; } num_t;
static num_t P, G, Q;
static int g_ready;

static void num(num_t* n) { bigint_init(&n->b, n->l, CAP); }
static void load(num_t* n, const uint8_t* be, int len) { num(n); bigint_from_be(&n->b, be, (uint16_t)len); }
static void setup(void) {
    if (g_ready) return;
    load(&P, k_p, 128); load(&G, k_g, 128); load(&Q, k_q, 20);
    g_ready = 1;
}
static int is_zero(const num_t* n) { int i; for (i = 0; i < n->b.length; i++) if (n->b.limbs[i]) return 0; return 1; }
static void hash_mod_q(const uint8_t* a, int alen, const uint8_t* m, int mlen, num_t* out) {
    sha256_ctx_t h; uint8_t d[32]; num_t t;
    sha256_init(&h); sha256_update(&h, a, (uint32_t)alen); sha256_update(&h, m, (uint32_t)mlen); sha256_final(&h, d);
    load(&t, d, 32); num(out); bigint_mod_reduce(&out->b, &t.b, &Q.b);
}
/* Montgomery arithmetic mod p (32 limbs, CIOS), ~60x faster than the
 * generic shift-add reduction; only 32x32->64 products, no division. */
#define NL 32
static uint32_t g_pl[NL], g_pinv, g_r2[NL], g_one_m[NL];
static int g_mont;
static int geq(const uint32_t* a, const uint32_t* b) { int i; for (i = NL - 1; i >= 0; i--) { if (a[i] != b[i]) return a[i] > b[i]; } return 1; }
static void subp(uint32_t* a) { uint64_t br = 0; int i; for (i = 0; i < NL; i++) { uint64_t d = (uint64_t)a[i] - g_pl[i] - br; a[i] = (uint32_t)d; br = (d >> 63) & 1U; } }
static void mont_mul(uint32_t* out, const uint32_t* a, const uint32_t* b) {
    uint32_t t[NL + 2];
    int i, j;
    for (i = 0; i < NL + 2; i++) t[i] = 0;
    for (i = 0; i < NL; i++) {
        uint64_t c = 0, m;
        for (j = 0; j < NL; j++) { c += (uint64_t)t[j] + (uint64_t)a[j] * b[i]; t[j] = (uint32_t)c; c >>= 32; }
        c += t[NL]; t[NL] = (uint32_t)c; t[NL + 1] = (uint32_t)(c >> 32);
        m = (uint32_t)(t[0] * g_pinv);
        c = (uint64_t)t[0] + m * g_pl[0]; c >>= 32;
        for (j = 1; j < NL; j++) { c += (uint64_t)t[j] + m * g_pl[j]; t[j - 1] = (uint32_t)c; c >>= 32; }
        c += t[NL]; t[NL - 1] = (uint32_t)c; c >>= 32;
        t[NL] = t[NL + 1] + (uint32_t)c; t[NL + 1] = 0;
    }
    if (t[NL] || geq(t, g_pl)) subp(t);
    for (i = 0; i < NL; i++) out[i] = t[i];
}
static void to_limbs(const num_t* n, uint32_t* l) { int i; for (i = 0; i < NL; i++) l[i] = i < n->b.length ? n->b.limbs[i] : 0; }
static void from_limbs(num_t* n, const uint32_t* l) {
    uint8_t be[128]; int i;
    for (i = 0; i < NL; i++) { uint32_t v = l[NL - 1 - i]; be[4 * i] = (uint8_t)(v >> 24); be[4 * i + 1] = (uint8_t)(v >> 16); be[4 * i + 2] = (uint8_t)(v >> 8); be[4 * i + 3] = (uint8_t)v; }
    load(n, be, 128);
}
static void mont_setup(void) {
    uint32_t inv = 1, x[NL];
    int i;
    if (g_mont) return;
    to_limbs(&P, g_pl);
    for (i = 0; i < 5; i++) inv *= 2U - g_pl[0] * inv;   /* p^-1 mod 2^32 */
    g_pinv = (uint32_t)(0U - inv);
    /* R mod p = 2^1024 mod p, then R^2 by 1024 modular doublings */
    for (i = 0; i < NL; i++) x[i] = 0;
    x[0] = 1;
    for (i = 0; i < 2048; i++) {
        uint32_t carry = 0; int j;
        for (j = 0; j < NL; j++) { uint32_t v = x[j]; x[j] = (v << 1) | carry; carry = v >> 31; }
        if (carry || geq(x, g_pl)) subp(x);
        if (i == 1023) for (j = 0; j < NL; j++) g_one_m[j] = x[j];
    }
    for (i = 0; i < NL; i++) g_r2[i] = x[i];
    g_mont = 1;
}
static int modexp(num_t* out, const num_t* base, const num_t* e) {
    uint32_t b[NL], acc[NL], one[NL];
    int i, bits;
    num_t red;
    mont_setup();
    num(&red);
    if (bigint_mod_reduce(&red.b, &base->b, &P.b) != 0) return -1;
    to_limbs(&red, b);
    mont_mul(b, b, g_r2);                 /* to Montgomery form */
    for (i = 0; i < NL; i++) acc[i] = g_one_m[i];
    bits = e->b.length * 32;
    for (i = bits - 1; i >= 0; i--) {
        mont_mul(acc, acc, acc);
        if ((e->b.limbs[i / 32] >> (i % 32)) & 1U) mont_mul(acc, acc, b);
    }
    for (i = 0; i < NL; i++) one[i] = 0;
    one[0] = 1;
    mont_mul(acc, acc, one);              /* out of Montgomery form */
    from_limbs(out, acc);
    return 0;
}

int csig_keypair(const uint8_t seed[32], uint8_t sk[CSIG_SK], uint8_t pk[CSIG_PK]) {
    num_t x, y;
    setup();
    hash_mod_q((const uint8_t*)"mohhdy-collab-key", 17, seed, 32, &x);
    if (is_zero(&x)) return -1;
    if (modexp(&y, &G, &x) != 0) return -1;
    bigint_to_be(&x.b, sk, CSIG_SK); bigint_to_be(&y.b, pk, CSIG_PK);
    return 0;
}

int csig_sign(const uint8_t sk[CSIG_SK], const uint8_t* msg, int len, uint8_t sig[CSIG_SIG]) {
    num_t x, k, r, e, xe, s, tmp;
    uint8_t rb[128];
    setup();
    load(&x, sk, CSIG_SK);
    hash_mod_q(sk, CSIG_SK, msg, len, &k);
    if (is_zero(&k)) return -1;
    if (modexp(&r, &G, &k) != 0) return -1;
    bigint_to_be(&r.b, rb, 128);
    hash_mod_q(rb, 128, msg, len, &e);
    num(&xe); num(&tmp); num(&s);
    if (bigint_mod_multiply(&xe.b, &x.b, &e.b, &Q.b, &tmp.b) != 0) return -1;
    /* s = k - xe mod q = k + (q - xe) mod q */
    num(&tmp);
    if (is_zero(&xe)) { bigint_mod_reduce(&s.b, &k.b, &Q.b); }
    else { bigint_subtract(&tmp.b, &Q.b, &xe.b); bigint_mod_add(&s.b, &k.b, &tmp.b, &Q.b); }
    bigint_to_be(&e.b, sig, 20); bigint_to_be(&s.b, sig + 20, 20);
    return 0;
}

int csig_pk_valid(const uint8_t pk[CSIG_PK]) {
    num_t y, t, one;
    uint8_t ob = 1;
    setup();
    load(&y, pk, CSIG_PK); load(&one, &ob, 1);
    if (bigint_compare(&y.b, &one.b) <= 0 || bigint_compare(&y.b, &P.b) >= 0) return 0;
    if (modexp(&t, &y, &Q) != 0) return 0;
    return bigint_compare(&t.b, &one.b) == 0;
}

int csig_verify(const uint8_t pk[CSIG_PK], const uint8_t* msg, int len, const uint8_t sig[CSIG_SIG]) {
    num_t y, e, s, a, b, r, tmp, e2;
    uint8_t rb[128];
    setup();
    load(&y, pk, CSIG_PK); load(&e, sig, 20); load(&s, sig + 20, 20);
    if (bigint_compare(&e.b, &Q.b) >= 0 || bigint_compare(&s.b, &Q.b) >= 0) return 0;
    if (bigint_compare(&y.b, &P.b) >= 0 || is_zero(&y)) return 0;
    if (modexp(&a, &G, &s) != 0 || modexp(&b, &y, &e) != 0) return 0;
    num(&r); num(&tmp);
    if (bigint_mod_multiply(&r.b, &a.b, &b.b, &P.b, &tmp.b) != 0) return 0;
    bigint_to_be(&r.b, rb, 128);
    hash_mod_q(rb, 128, msg, len, &e2);
    return bigint_compare(&e2.b, &e.b) == 0;
}
