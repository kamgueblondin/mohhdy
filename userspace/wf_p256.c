/* wf_p256.c - Lot 72: constant-time P-256 (see wf_p256.h). */
#include "wf_p256.h"

typedef uint32_t fe[8];                  /* little-endian 32-bit limbs */
typedef struct { fe x, y, z; } pt_t;     /* homogeneous projective, Montgomery form */

static const fe P = {0xffffffffU, 0xffffffffU, 0xffffffffU, 0x00000000U, 0x00000000U, 0x00000000U, 0x00000001U, 0xffffffffU};
static const fe N = {0xfc632551U, 0xf3b9cac2U, 0xa7179e84U, 0xbce6faadU, 0xffffffffU, 0xffffffffU, 0x00000000U, 0xffffffffU};
static const fe RR = {0x00000003U, 0x00000000U, 0xffffffffU, 0xfffffffbU, 0xfffffffeU, 0xffffffffU, 0xfffffffdU, 0x00000004U};   /* 2^512 mod p */
static const fe ONE = {1, 0, 0, 0, 0, 0, 0, 0};
static const uint8_t GX[32] = {
    0x6b,0x17,0xd1,0xf2,0xe1,0x2c,0x42,0x47,0xf8,0xbc,0xe6,0xe5,0x63,0xa4,0x40,0xf2,
    0x77,0x03,0x7d,0x81,0x2d,0xeb,0x33,0xa0,0xf4,0xa1,0x39,0x45,0xd8,0x98,0xc2,0x96};
static const uint8_t GY[32] = {
    0x4f,0xe3,0x42,0xe2,0xfe,0x1a,0x7f,0x9b,0x8e,0xe7,0xeb,0x4a,0x7c,0x0f,0x9e,0x16,
    0x2b,0xce,0x33,0x57,0x6b,0x31,0x5e,0xce,0xcb,0xb6,0x40,0x68,0x37,0xbf,0x51,0xf5};
static const uint8_t B[32] = {
    0x5a,0xc6,0x35,0xd8,0xaa,0x3a,0x93,0xe7,0xb3,0xeb,0xbd,0x55,0x76,0x98,0x86,0xbc,
    0x65,0x1d,0x06,0xb0,0xcc,0x53,0xb0,0xf6,0x3b,0xce,0x3c,0x3e,0x27,0xd2,0x60,0x4b};

static uint32_t g_muls;
uint32_t wf_p256_mul_count(void) { return g_muls; }
void wf_p256_mul_count_reset(void) { g_muls = 0; }

static void fe_copy(fe r, const fe a) { int i; for (i = 0; i < 8; i++) r[i] = a[i]; }

/* r = mask ? a : b, mask is 0 or all ones */
static void fe_select(fe r, const fe a, const fe b, uint32_t mask) {
    int i;
    for (i = 0; i < 8; i++) r[i] = (a[i] & mask) | (b[i] & ~mask);
}

/* r = a - b, returns the borrow (0 or 1) */
static uint32_t sub8(fe r, const fe a, const fe b) {
    uint64_t t;
    uint32_t borrow = 0;
    int i;
    for (i = 0; i < 8; i++) {
        t = (uint64_t)a[i] - b[i] - borrow;
        r[i] = (uint32_t)t;
        borrow = (uint32_t)(t >> 32) & 1U;
    }
    return borrow;
}

static void fe_add(fe r, const fe a, const fe b) {
    fe s, t;
    uint64_t c = 0;
    uint32_t borrow, carry;
    int i;
    for (i = 0; i < 8; i++) { c += (uint64_t)a[i] + b[i]; s[i] = (uint32_t)c; c >>= 32; }
    carry = (uint32_t)c;
    borrow = sub8(t, s, P);
    fe_select(r, t, s, (uint32_t)0 - (carry | (borrow ^ 1U)));
}

static void fe_sub(fe r, const fe a, const fe b) {
    fe d, pm;
    uint32_t borrow = sub8(d, a, b), mask = (uint32_t)0 - borrow;
    uint64_t c = 0;
    int i;
    for (i = 0; i < 8; i++) pm[i] = P[i] & mask;
    for (i = 0; i < 8; i++) { c += (uint64_t)d[i] + pm[i]; r[i] = (uint32_t)c; c >>= 32; }
}

/* Montgomery product a * b / 2^256 mod p (CIOS; -p^-1 mod 2^32 = 1). */
static void fe_mul(fe r, const fe a, const fe b) {
    uint32_t t[10], m, borrow;
    fe u, v;
    uint64_t c;
    int i, j;
    g_muls++;
    for (i = 0; i < 10; i++) t[i] = 0;
    for (i = 0; i < 8; i++) {
        c = 0;
        for (j = 0; j < 8; j++) { c += (uint64_t)t[j] + (uint64_t)a[j] * b[i]; t[j] = (uint32_t)c; c >>= 32; }
        c += t[8]; t[8] = (uint32_t)c; t[9] = (uint32_t)(c >> 32);
        m = t[0];
        c = (uint64_t)t[0] + (uint64_t)m * P[0];
        c >>= 32;
        for (j = 1; j < 8; j++) { c += (uint64_t)t[j] + (uint64_t)m * P[j]; t[j - 1] = (uint32_t)c; c >>= 32; }
        c += t[8]; t[7] = (uint32_t)c;
        t[8] = t[9] + (uint32_t)(c >> 32);
    }
    for (i = 0; i < 8; i++) u[i] = t[i];
    borrow = sub8(v, u, P);
    fe_select(r, v, u, (uint32_t)0 - ((t[8] & 1U) | (borrow ^ 1U)));
}

static void fe_from_bytes(fe r, const uint8_t b[32]) {
    int i;
    for (i = 0; i < 8; i++)
        r[i] = ((uint32_t)b[31 - 4 * i]) | ((uint32_t)b[30 - 4 * i] << 8) | ((uint32_t)b[29 - 4 * i] << 16) | ((uint32_t)b[28 - 4 * i] << 24);
}

static void fe_to_bytes(uint8_t b[32], const fe a) {
    int i;
    for (i = 0; i < 8; i++) {
        b[31 - 4 * i] = (uint8_t)a[i]; b[30 - 4 * i] = (uint8_t)(a[i] >> 8);
        b[29 - 4 * i] = (uint8_t)(a[i] >> 16); b[28 - 4 * i] = (uint8_t)(a[i] >> 24);
    }
}

/* 1 if a < m (public values only) */
static int lt(const fe a, const fe m) { fe t; return (int)sub8(t, a, m); }
static int fe_is_zero(const fe a) { uint32_t d = 0; int i; for (i = 0; i < 8; i++) d |= a[i]; return d == 0; }

/* a^(p-2): fixed public exponent, the same squarings and products for every a */
static void fe_inv(fe r, const fe a) {
    fe acc;
    int i;
    fe_copy(acc, ONE);
    fe_mul(acc, acc, RR);                         /* 1 in Montgomery form */
    for (i = 255; i >= 0; i--) {
        uint32_t bit = ((P[i >> 5] - ((i >> 5) == 0 ? 2U : 0U)) >> (i & 31)) & 1U;   /* p - 2 */
        fe_mul(acc, acc, acc);
        if (bit) fe_mul(acc, acc, a);           /* branch on the public exponent */
    }
    fe_copy(r, acc);
}

/* Complete addition, a = -3 (RCB16 algorithm 4). r may alias p or q. */
static void pt_add(pt_t* r, const pt_t* p, const pt_t* q, const fe b) {
    fe t0, t1, t2, t3, t4, x3, y3, z3;
    fe_mul(t0, p->x, q->x); fe_mul(t1, p->y, q->y); fe_mul(t2, p->z, q->z);
    fe_add(t3, p->x, p->y); fe_add(t4, q->x, q->y); fe_mul(t3, t3, t4);
    fe_add(t4, t0, t1); fe_sub(t3, t3, t4); fe_add(t4, p->y, p->z);
    fe_add(x3, q->y, q->z); fe_mul(t4, t4, x3); fe_add(x3, t1, t2);
    fe_sub(t4, t4, x3); fe_add(x3, p->x, p->z); fe_add(y3, q->x, q->z);
    fe_mul(x3, x3, y3); fe_add(y3, t0, t2); fe_sub(y3, x3, y3);
    fe_mul(z3, b, t2); fe_sub(x3, y3, z3); fe_add(z3, x3, x3);
    fe_add(x3, x3, z3); fe_sub(z3, t1, x3); fe_add(x3, t1, x3);
    fe_mul(y3, b, y3); fe_add(t1, t2, t2); fe_add(t2, t1, t2);
    fe_sub(y3, y3, t2); fe_sub(y3, y3, t0); fe_add(t1, y3, y3);
    fe_add(y3, t1, y3); fe_add(t1, t0, t0); fe_add(t0, t1, t0);
    fe_sub(t0, t0, t2); fe_mul(t1, t4, y3); fe_mul(t2, t0, y3);
    fe_mul(y3, x3, z3); fe_add(y3, y3, t2); fe_mul(x3, t3, x3);
    fe_sub(x3, x3, t1); fe_mul(z3, t4, z3); fe_mul(t1, t3, t0);
    fe_add(z3, z3, t1);
    fe_copy(r->x, x3); fe_copy(r->y, y3); fe_copy(r->z, z3);
}

static void pt_cswap(pt_t* a, pt_t* b, uint32_t mask) {
    int i;
    for (i = 0; i < 8; i++) {
        uint32_t t;
        t = (a->x[i] ^ b->x[i]) & mask; a->x[i] ^= t; b->x[i] ^= t;
        t = (a->y[i] ^ b->y[i]) & mask; a->y[i] ^= t; b->y[i] ^= t;
        t = (a->z[i] ^ b->z[i]) & mask; a->z[i] ^= t; b->z[i] ^= t;
    }
}

int wf_p256_mul(const uint8_t k[32], const uint8_t* point, int point_len, uint8_t out[65]) {
    static pt_t R0, R1, Pp;
    fe x, y, b, l, rr, t, zi;
    int i;
    uint32_t prev = 0;
    {
        fe kk;
        fe_from_bytes(kk, k);
        if (fe_is_zero(kk) || !lt(kk, N)) return -1;
    }
    fe_from_bytes(t, B); fe_mul(b, t, RR);
    if (point) {
        if (point_len != 65 || point[0] != 4) return -2;
        fe_from_bytes(x, point + 1); fe_from_bytes(y, point + 33);
    } else {
        fe_from_bytes(x, GX); fe_from_bytes(y, GY);
    }
    if (!lt(x, P) || !lt(y, P)) return -2;
    fe_mul(x, x, RR); fe_mul(y, y, RR);
    /* y^2 = x^3 - 3x + b (public input check) */
    fe_mul(l, y, y);
    fe_mul(rr, x, x); fe_mul(rr, rr, x);
    fe_add(t, x, x); fe_add(t, t, x);
    fe_sub(rr, rr, t); fe_add(rr, rr, b);
    for (i = 0; i < 8; i++) if (l[i] != rr[i]) return -2;
    fe_copy(Pp.x, x); fe_copy(Pp.y, y); fe_mul(Pp.z, ONE, RR);
    /* ladder: R0 = O (0:1:0), R1 = P */
    for (i = 0; i < 8; i++) { R0.x[i] = 0; R0.z[i] = 0; }
    fe_copy(R0.y, Pp.z);
    R1 = Pp;
    for (i = 255; i >= 0; i--) {
        uint32_t bit = (k[31 - (i >> 3)] >> (i & 7)) & 1U;
        pt_cswap(&R0, &R1, (uint32_t)0 - (bit ^ prev));
        prev = bit;
        pt_add(&R1, &R0, &R1, b);
        pt_add(&R0, &R0, &R0, b);
    }
    pt_cswap(&R0, &R1, (uint32_t)0 - prev);
    /* affine: x = X/Z, y = Y/Z */
    if (fe_is_zero(R0.z)) return -3;
    fe_inv(zi, R0.z);
    fe_mul(x, R0.x, zi); fe_mul(y, R0.y, zi);
    fe_mul(x, x, ONE); fe_mul(y, y, ONE);       /* out of Montgomery form */
    out[0] = 4;
    fe_to_bytes(out + 1, x);
    fe_to_bytes(out + 33, y);
    for (i = 0; i < 8; i++) { R0.x[i] = R0.y[i] = R0.z[i] = R1.x[i] = R1.y[i] = R1.z[i] = 0; }
    return 0;
}
