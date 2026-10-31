/* wf_ec.c - Lot 67: ECDSA verification on P-256 and P-384. */
#include "wf_ec.h"

typedef struct {
    int size;               /* bytes */
    const uint8_t* p;
    const uint8_t* b;
    const uint8_t* n;
    const uint8_t* gx;
    const uint8_t* gy;
} curve_def_t;

static const uint8_t P256_P[32] = {
    0xff,0xff,0xff,0xff,0x00,0x00,0x00,0x01,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
    0x00,0x00,0x00,0x00,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff};
static const uint8_t P256_B[32] = {
    0x5a,0xc6,0x35,0xd8,0xaa,0x3a,0x93,0xe7,0xb3,0xeb,0xbd,0x55,0x76,0x98,0x86,0xbc,
    0x65,0x1d,0x06,0xb0,0xcc,0x53,0xb0,0xf6,0x3b,0xce,0x3c,0x3e,0x27,0xd2,0x60,0x4b};
static const uint8_t P256_N[32] = {
    0xff,0xff,0xff,0xff,0x00,0x00,0x00,0x00,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,
    0xbc,0xe6,0xfa,0xad,0xa7,0x17,0x9e,0x84,0xf3,0xb9,0xca,0xc2,0xfc,0x63,0x25,0x51};
static const uint8_t P256_GX[32] = {
    0x6b,0x17,0xd1,0xf2,0xe1,0x2c,0x42,0x47,0xf8,0xbc,0xe6,0xe5,0x63,0xa4,0x40,0xf2,
    0x77,0x03,0x7d,0x81,0x2d,0xeb,0x33,0xa0,0xf4,0xa1,0x39,0x45,0xd8,0x98,0xc2,0x96};
static const uint8_t P256_GY[32] = {
    0x4f,0xe3,0x42,0xe2,0xfe,0x1a,0x7f,0x9b,0x8e,0xe7,0xeb,0x4a,0x7c,0x0f,0x9e,0x16,
    0x2b,0xce,0x33,0x57,0x6b,0x31,0x5e,0xce,0xcb,0xb6,0x40,0x68,0x37,0xbf,0x51,0xf5};

static const uint8_t P384_P[48] = {
    0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,
    0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xfe,
    0xff,0xff,0xff,0xff,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0xff,0xff,0xff,0xff};
static const uint8_t P384_B[48] = {
    0xb3,0x31,0x2f,0xa7,0xe2,0x3e,0xe7,0xe4,0x98,0x8e,0x05,0x6b,0xe3,0xf8,0x2d,0x19,
    0x18,0x1d,0x9c,0x6e,0xfe,0x81,0x41,0x12,0x03,0x14,0x08,0x8f,0x50,0x13,0x87,0x5a,
    0xc6,0x56,0x39,0x8d,0x8a,0x2e,0xd1,0x9d,0x2a,0x85,0xc8,0xed,0xd3,0xec,0x2a,0xef};
static const uint8_t P384_N[48] = {
    0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,
    0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xc7,0x63,0x4d,0x81,0xf4,0x37,0x2d,0xdf,
    0x58,0x1a,0x0d,0xb2,0x48,0xb0,0xa7,0x7a,0xec,0xec,0x19,0x6a,0xcc,0xc5,0x29,0x73};
static const uint8_t P384_GX[48] = {
    0xaa,0x87,0xca,0x22,0xbe,0x8b,0x05,0x37,0x8e,0xb1,0xc7,0x1e,0xf3,0x20,0xad,0x74,
    0x6e,0x1d,0x3b,0x62,0x8b,0xa7,0x9b,0x98,0x59,0xf7,0x41,0xe0,0x82,0x54,0x2a,0x38,
    0x55,0x02,0xf2,0x5d,0xbf,0x55,0x29,0x6c,0x3a,0x54,0x5e,0x38,0x72,0x76,0x0a,0xb7};
static const uint8_t P384_GY[48] = {
    0x36,0x17,0xde,0x4a,0x96,0x26,0x2c,0x6f,0x5d,0x9e,0x98,0xbf,0x92,0x92,0xdc,0x29,
    0xf8,0xf4,0x1d,0xbd,0x28,0x9a,0x14,0x7c,0xe9,0xda,0x31,0x13,0xb5,0xf0,0xb8,0xc0,
    0x0a,0x60,0xb1,0xce,0x1d,0x7e,0x81,0x9d,0x7a,0x43,0x1d,0x7c,0x90,0xea,0x0e,0x5f};

static const curve_def_t* curve_of(int id) {
    static const curve_def_t p256 = {32, P256_P, P256_B, P256_N, P256_GX, P256_GY};
    static const curve_def_t p384 = {48, P384_P, P384_B, P384_N, P384_GX, P384_GY};
    if (id == WF_CURVE_P256) return &p256;
    if (id == WF_CURVE_P384) return &p384;
    return 0;
}

int wf_ec_size(int curve) {
    const curve_def_t* c = curve_of(curve);
    return c ? c->size : 0;
}

/* Field context (static: the stack is small in Ring 3 programs). */
typedef struct {
    wf_mont_t f;     /* field modulus p */
    wf_bn_t b;       /* curve b in Montgomery form */
    wf_bn_t three;   /* 3 in Montgomery form */
} field_t;

typedef struct { wf_bn_t x, y, z; int inf; } jpoint_t;

static int field_init(field_t* F, const curve_def_t* c) {
    wf_bn_t t;
    if (wf_bn_from_bytes(&t, c->p, c->size) != 0 || wf_mont_init(&F->f, &t) != 0) return -1;
    (void)wf_bn_from_bytes(&t, c->b, c->size);
    wf_mont_to(&F->f, &F->b, &t);
    wf_bn_set_u32(&t, 3U);
    wf_mont_to(&F->f, &F->three, &t);
    return 0;
}

#define FMUL(r, a, b) wf_mont_mul(&F->f, (r), (a), (b))
#define FADD(r, a, b) wf_mont_add(&F->f, (r), (a), (b))
#define FSUB(r, a, b) wf_mont_sub(&F->f, (r), (a), (b))

/* y^2 = x^3 - 3x + b, all values in Montgomery form. */
static int on_curve(const field_t* F, const wf_bn_t* x, const wf_bn_t* y) {
    wf_bn_t l, r, t;
    FMUL(&l, y, y);
    FMUL(&t, x, x);
    FMUL(&r, &t, x);
    FMUL(&t, &F->three, x);
    FSUB(&r, &r, &t);
    FADD(&r, &r, &F->b);
    return wf_bn_cmp(&l, &r) == 0;
}

/* dbl-2001-b (a = -3). */
static void jdouble(const field_t* F, jpoint_t* r, const jpoint_t* p) {
    wf_bn_t delta, gamma, beta, alpha, t1, t2;
    if (p->inf || wf_bn_is_zero(&p->y)) { r->inf = 1; return; }
    FMUL(&delta, &p->z, &p->z);
    FMUL(&gamma, &p->y, &p->y);
    FMUL(&beta, &p->x, &gamma);
    FSUB(&t1, &p->x, &delta);
    FADD(&t2, &p->x, &delta);
    FMUL(&alpha, &t1, &t2);
    FMUL(&alpha, &alpha, &F->three);
    /* z3 = (y + z)^2 - gamma - delta */
    FADD(&t1, &p->y, &p->z);
    FMUL(&t1, &t1, &t1);
    FSUB(&t1, &t1, &gamma);
    FSUB(&r->z, &t1, &delta);
    /* x3 = alpha^2 - 8 beta */
    FADD(&t2, &beta, &beta);   /* 2b */
    FADD(&t2, &t2, &t2);       /* 4b */
    FMUL(&t1, &alpha, &alpha);
    FSUB(&r->x, &t1, &t2);
    FSUB(&r->x, &r->x, &t2);
    /* y3 = alpha (4 beta - x3) - 8 gamma^2 */
    FSUB(&t2, &t2, &r->x);
    FMUL(&t1, &alpha, &t2);
    FMUL(&gamma, &gamma, &gamma);
    FADD(&gamma, &gamma, &gamma);
    FADD(&gamma, &gamma, &gamma);
    FADD(&gamma, &gamma, &gamma);
    FSUB(&r->y, &t1, &gamma);
    r->inf = 0;
}

/* add-2007-bl, general Jacobian addition. */
static void jadd(const field_t* F, jpoint_t* r, const jpoint_t* p, const jpoint_t* q) {
    wf_bn_t z1z1, z2z2, u1, u2, s1, s2, h, i, j, rr, v, t;
    if (p->inf) { *r = *q; return; }
    if (q->inf) { *r = *p; return; }
    FMUL(&z1z1, &p->z, &p->z);
    FMUL(&z2z2, &q->z, &q->z);
    FMUL(&u1, &p->x, &z2z2);
    FMUL(&u2, &q->x, &z1z1);
    FMUL(&t, &q->z, &z2z2);
    FMUL(&s1, &p->y, &t);
    FMUL(&t, &p->z, &z1z1);
    FMUL(&s2, &q->y, &t);
    FSUB(&h, &u2, &u1);
    FSUB(&rr, &s2, &s1);
    if (wf_bn_is_zero(&h)) {
        if (wf_bn_is_zero(&rr)) { jdouble(F, r, p); return; }
        r->inf = 1;
        return;
    }
    FADD(&i, &h, &h);
    FMUL(&i, &i, &i);
    FMUL(&j, &h, &i);
    FADD(&rr, &rr, &rr);
    FMUL(&v, &u1, &i);
    /* x3 = r^2 - j - 2v */
    FMUL(&t, &rr, &rr);
    FSUB(&t, &t, &j);
    FSUB(&t, &t, &v);
    FSUB(&r->x, &t, &v);
    /* y3 = r (v - x3) - 2 s1 j */
    FSUB(&t, &v, &r->x);
    FMUL(&t, &rr, &t);
    FMUL(&s1, &s1, &j);
    FADD(&s1, &s1, &s1);
    FSUB(&r->y, &t, &s1);
    /* z3 = ((z1 + z2)^2 - z1z1 - z2z2) h */
    FADD(&t, &p->z, &q->z);
    FMUL(&t, &t, &t);
    FSUB(&t, &t, &z1z1);
    FSUB(&t, &t, &z2z2);
    FMUL(&r->z, &t, &h);
    r->inf = 0;
}

static field_t g_field;
static wf_mont_t g_order;

static int load_point(const field_t* F, int size, const uint8_t* pub, int pub_len, jpoint_t* P) {
    wf_bn_t x, y;
    if (pub_len != 1 + 2 * size || pub[0] != 0x04U) return -1;
    if (wf_bn_from_bytes(&x, pub + 1, size) || wf_bn_from_bytes(&y, pub + 1 + size, size)) return -1;
    if (wf_bn_cmp(&x, &F->f.m) >= 0 || wf_bn_cmp(&y, &F->f.m) >= 0) return -1;
    wf_mont_to(&F->f, &P->x, &x);
    wf_mont_to(&F->f, &P->y, &y);
    wf_bn_copy(&P->z, &F->f.one);
    P->inf = 0;
    if (!on_curve(F, &P->x, &P->y)) return -2;
    return 0;
}

int wf_ec_point_check(int curve, const uint8_t* pub, int pub_len) {
    const curve_def_t* c = curve_of(curve);
    jpoint_t P;
    if (!c || field_init(&g_field, c) != 0) return -1;
    return load_point(&g_field, c->size, pub, pub_len, &P);
}

/* Digest -> integer e: leftmost bits of the digest, as many as n has. */
static void digest_to_e(wf_bn_t* e, const uint8_t* digest, int digest_len, int size, const wf_bn_t* n) {
    int len = digest_len > size ? size : digest_len;
    (void)wf_bn_from_bytes(e, digest, len);
    /* n has exactly size*8 bits for P-256 / P-384, so no extra shift. */
    wf_bn_reduce_once(e, n);
}

int wf_ecdsa_verify(int curve, const uint8_t* pub, int pub_len,
                    const uint8_t* digest, int digest_len,
                    const uint8_t* rb, int r_len, const uint8_t* sb, int s_len) {
    const curve_def_t* c = curve_of(curve);
    field_t* F = &g_field;
    static jpoint_t G, Q, GQ, R, T;
    wf_bn_t r, s, e, w, u1, u2, nn, x, zinv, zz, t;
    int i, top;
    if (!c || !pub || !digest || !rb || !sb) return -1;
    if (field_init(F, c) != 0) return -1;
    if (wf_bn_from_bytes(&nn, c->n, c->size) || wf_mont_init(&g_order, &nn) != 0) return -1;
    if (load_point(F, c->size, pub, pub_len, &Q) != 0) return -2;
    {
        uint8_t g[1 + 96];
        g[0] = 4U;
        for (i = 0; i < c->size; i++) { g[1 + i] = c->gx[i]; g[1 + c->size + i] = c->gy[i]; }
        if (load_point(F, c->size, g, 1 + 2 * c->size, &G) != 0) return -1;
    }
    if (r_len > c->size + 1 || s_len > c->size + 1) return -3;
    if (wf_bn_from_bytes(&r, rb, r_len) || wf_bn_from_bytes(&s, sb, s_len)) return -3;
    if (wf_bn_is_zero(&r) || wf_bn_is_zero(&s) || wf_bn_cmp(&r, &nn) >= 0 || wf_bn_cmp(&s, &nn) >= 0) return -3;
    digest_to_e(&e, digest, digest_len, c->size, &nn);
    wf_mont_inv_prime(&g_order, &w, &s);
    /* u1 = e w, u2 = r w (mod n), through Montgomery form. */
    wf_mont_to(&g_order, &t, &w);
    wf_mont_mul(&g_order, &u1, &t, &e);
    wf_mont_mul(&g_order, &u2, &t, &r);
    /* Shamir's trick: R = u1 G + u2 Q. */
    jadd(F, &GQ, &G, &Q);
    R.inf = 1;
    top = wf_bn_bits(&u1) > wf_bn_bits(&u2) ? wf_bn_bits(&u1) : wf_bn_bits(&u2);
    for (i = top - 1; i >= 0; i--) {
        int b1 = wf_bn_bit(&u1, i), b2 = wf_bn_bit(&u2, i);
        jdouble(F, &T, &R);
        R = T;
        if (b1 && b2) { jadd(F, &T, &R, &GQ); R = T; }
        else if (b1) { jadd(F, &T, &R, &G); R = T; }
        else if (b2) { jadd(F, &T, &R, &Q); R = T; }
    }
    if (R.inf) return -4;
    /* affine x = X / Z^2 */
    wf_mont_from(&F->f, &zz, &R.z);
    wf_mont_inv_prime(&F->f, &zinv, &zz);
    wf_mont_to(&F->f, &zinv, &zinv);
    FMUL(&zz, &zinv, &zinv);
    FMUL(&x, &R.x, &zz);
    wf_mont_from(&F->f, &x, &x);
    wf_bn_reduce_once(&x, &nn);
    return wf_bn_cmp(&x, &r) == 0 ? 0 : -5;
}

/* Minimal DER reader for SEQUENCE { INTEGER, INTEGER }. */
static int der_int(const uint8_t* d, int len, int* pos, const uint8_t** v, int* vlen) {
    int l;
    if (*pos + 2 > len || d[*pos] != 0x02U) return -1;
    l = d[*pos + 1];
    if (l & 0x80) return -1;
    if (*pos + 2 + l > len || l == 0) return -1;
    *v = d + *pos + 2;
    *vlen = l;
    *pos += 2 + l;
    return 0;
}

int wf_ecdsa_verify_der(int curve, const uint8_t* pub, int pub_len,
                        const uint8_t* digest, int digest_len,
                        const uint8_t* sig, int sig_len) {
    const uint8_t *r, *s;
    int rl, sl, pos, body;
    if (!sig || sig_len < 8 || sig[0] != 0x30U) return -3;
    if (sig[1] & 0x80) {
        if (sig[1] != 0x81U || sig_len < 3) return -3;
        body = sig[2];
        pos = 3;
    } else {
        body = sig[1];
        pos = 2;
    }
    if (pos + body != sig_len) return -3;
    if (der_int(sig, sig_len, &pos, &r, &rl) || der_int(sig, sig_len, &pos, &s, &sl) || pos != sig_len) return -3;
    return wf_ecdsa_verify(curve, pub, pub_len, digest, digest_len, r, rl, s, sl);
}
