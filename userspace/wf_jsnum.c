/* wf_jsnum.c - Lot 113: binary64 in software for the JavaScript subset.
 * The add/sub/mul/div core follows the classic SoftFloat layout (a
 * significand with the binary point between bits 62 and 61, sticky
 * jamming, round to nearest even); see wf_jsnum.h for the contract. */
#include "wf_jsnum.h"

#define F_SIGN(x) ((uint32_t)((x) >> 63))
#define F_EXP(x)  ((int)(((x) >> 52) & 0x7FF))
#define F_FRAC(x) ((x) & 0x000FFFFFFFFFFFFFULL)
#define HIDDEN    0x0010000000000000ULL

int jf_isnan(jf64 a) { return F_EXP(a) == 0x7FF && F_FRAC(a) != 0; }
int jf_isinf(jf64 a) { return F_EXP(a) == 0x7FF && F_FRAC(a) == 0; }
int jf_isfinite(jf64 a) { return F_EXP(a) != 0x7FF; }
int jf_iszero(jf64 a) { return (a << 1) == 0; }
int jf_signbit(jf64 a) { return (int)F_SIGN(a); }
jf64 jf_neg(jf64 a) { return a ^ 0x8000000000000000ULL; }
jf64 jf_abs(jf64 a) { return a & 0x7FFFFFFFFFFFFFFFULL; }

static jf64 pack(uint32_t s, int e, uint64_t sig) {
    return ((uint64_t)s << 63) + ((uint64_t)(uint32_t)e << 52) + sig;
}

static int clz64(uint64_t a) {
    int n = 0;
    if (a == 0) return 64;
    while (!(a & 0x8000000000000000ULL)) { a <<= 1; n++; }
    return n;
}

static uint64_t jam_right(uint64_t a, int count) {
    if (count == 0) return a;
    if (count < 64) return (a >> count) | ((a << (64 - count)) != 0);
    return a != 0;
}

/* value = sig * 2^(e - 1084), sig with bit 62 the leading one (or less
 * for subnormal results); see wf_jsnum.h */
static jf64 round_pack(uint32_t s, int e, uint64_t sig) {
    int round_bits = (int)(sig & 0x3FF);
    if (e >= 0x7FD) {
        if (e > 0x7FD || (int64_t)(sig + 0x200) < 0) return pack(s, 0x7FF, 0);
    }
    if (e < 0) {
        sig = jam_right(sig, -e);
        e = 0;
        round_bits = (int)(sig & 0x3FF);
    }
    sig = (sig + 0x200) >> 10;
    if ((round_bits ^ 0x200) == 0) sig &= ~1ULL;
    if (sig == 0) e = 0;
    return pack(s, e, sig);
}

static jf64 norm_round_pack(uint32_t s, int e, uint64_t sig) {
    int shift = clz64(sig) - 1;
    if (sig == 0) return pack(s, 0, 0);
    return round_pack(s, e - shift, sig << shift);
}

static void norm_sub(uint64_t frac, int *e, uint64_t *sig) {
    int shift = clz64(frac) - 11;
    *sig = frac << shift;
    *e = 1 - shift;
}

static void mul64(uint64_t a, uint64_t b, uint64_t *hi, uint64_t *lo) {
    uint64_t a0 = (uint32_t)a, a1 = a >> 32, b0 = (uint32_t)b, b1 = b >> 32;
    uint64_t p00 = a0 * b0, p01 = a0 * b1, p10 = a1 * b0, p11 = a1 * b1;
    uint64_t mid = (p00 >> 32) + (uint32_t)p01 + (uint32_t)p10;
    *lo = (mid << 32) | (uint32_t)p00;
    *hi = p11 + (p01 >> 32) + (p10 >> 32) + (mid >> 32);
}

/* (hi:lo) / d with hi < d, by long division (no __udivdi3) */
static uint64_t div128(uint64_t hi, uint64_t lo, uint64_t d, uint64_t *rem) {
    uint64_t r = hi, q = 0;
    int i;
    for (i = 0; i < 64; i++) {
        uint64_t carry = r >> 63;
        r = (r << 1) | (lo >> 63);
        lo <<= 1;
        q <<= 1;
        if (carry || r >= d) { r -= d; q |= 1; }
    }
    if (rem) *rem = r;
    return q;
}

static uint64_t udiv64(uint64_t n, uint64_t d, uint64_t *rem) {
    return div128(0, n, d, rem);
}

jf64 jf_from_i32(int32_t v) {
    uint32_t s = v < 0, a = s ? 0U - (uint32_t)v : (uint32_t)v;
    int shift;
    uint64_t sig;
    if (v == 0) return 0;
    sig = a;
    shift = clz64(sig) - 11;
    return pack(s, 0x432 - shift, sig << shift);
}

jf64 jf_from_u32(uint32_t v) {
    int shift;
    uint64_t sig = v;
    if (v == 0) return 0;
    shift = clz64(sig) - 11;
    return pack(0, 0x432 - shift, sig << shift);
}

jf64 jf_from_u64(uint64_t v) {
    if (v == 0) return 0;
    if (v >> 63) return round_pack(0, 0x43D, jam_right(v, 1));
    return norm_round_pack(0, 0x43C, v);
}

jf64 jf_from_i64(int64_t v) {
    uint64_t a = v < 0 ? 0ULL - (uint64_t)v : (uint64_t)v;
    jf64 r = jf_from_u64(a);
    return v < 0 ? jf_neg(r) : r;
}

/* the integer part of |a| modulo 2^64 when it is finite, else 0 */
static uint64_t int_mod64(jf64 a) {
    int e = F_EXP(a) - 1075;
    uint64_t sig = F_FRAC(a) | HIDDEN;
    if (F_EXP(a) == 0x7FF || F_EXP(a) == 0) return 0;
    if (e >= 64) return 0;
    if (e >= 0) return sig << e;
    if (e <= -64) return 0;
    return sig >> -e;
}

uint32_t jf_to_u32(jf64 a) {
    uint32_t v = (uint32_t)int_mod64(a);
    return F_SIGN(a) ? 0U - v : v;
}

int32_t jf_to_i32(jf64 a) { return (int32_t)jf_to_u32(a); }

int jf_is_integer(jf64 a) {
    int e = F_EXP(a);
    if (e == 0x7FF) return 0;
    if (jf_iszero(a)) return 1;
    if (e < 1023) return 0;
    if (e >= 1075) return 1;
    return (F_FRAC(a) & ((1ULL << (1075 - e)) - 1)) == 0;
}

int jf_to_i64(jf64 a, int64_t *out) {
    uint64_t v;
    if (!jf_is_integer(a)) return 0;
    if (F_EXP(a) >= 1023 + 63) return 0;
    v = int_mod64(a);
    *out = F_SIGN(a) ? -(int64_t)v : (int64_t)v;
    return 1;
}

int jf_is_i32(jf64 a, int32_t *out) {
    int64_t v;
    if (a == JF_NZERO) return 0;
    if (!jf_to_i64(a, &v)) return 0;
    if (v < -2147483647LL - 1 || v > 2147483647LL) return 0;
    *out = (int32_t)v;
    return 1;
}

static jf64 add_sigs(jf64 a, jf64 b, uint32_t zs) {
    int ae = F_EXP(a), be = F_EXP(b), ze, diff = ae - be;
    uint64_t as = F_FRAC(a) << 9, bs = F_FRAC(b) << 9, zsig;
    if (diff > 0) {
        if (ae == 0x7FF) return as ? JF_NAN : a;
        if (be == 0) --diff; else bs |= 0x2000000000000000ULL;
        bs = jam_right(bs, diff);
        ze = ae;
    } else if (diff < 0) {
        if (be == 0x7FF) return bs ? JF_NAN : pack(zs, 0x7FF, 0);
        if (ae == 0) ++diff; else as |= 0x2000000000000000ULL;
        as = jam_right(as, -diff);
        ze = be;
    } else {
        if (ae == 0x7FF) return (as | bs) ? JF_NAN : a;
        if (ae == 0) return pack(zs, 0, (as + bs) >> 9);
        zsig = 0x4000000000000000ULL + as + bs;
        return round_pack(zs, ae, zsig);
    }
    as |= 0x2000000000000000ULL;
    zsig = (as + bs) << 1;
    --ze;
    if ((int64_t)zsig < 0) { zsig = as + bs; ++ze; }
    return round_pack(zs, ze, zsig);
}

static jf64 sub_sigs(jf64 a, jf64 b, uint32_t zs) {
    int ae = F_EXP(a), be = F_EXP(b), ze, diff = ae - be;
    uint64_t as = F_FRAC(a) << 10, bs = F_FRAC(b) << 10, zsig;
    if (diff > 0) goto a_exp_bigger;
    if (diff < 0) goto b_exp_bigger;
    if (ae == 0x7FF) return JF_NAN;
    if (ae == 0) { ae = 1; be = 1; }
    if (bs < as) goto a_bigger;
    if (as < bs) goto b_bigger;
    return 0;
b_exp_bigger:
    if (be == 0x7FF) return bs ? JF_NAN : pack(zs ^ 1, 0x7FF, 0);
    if (ae == 0) ++diff; else as |= 0x4000000000000000ULL;
    as = jam_right(as, -diff);
    bs |= 0x4000000000000000ULL;
b_bigger:
    zsig = bs - as;
    ze = be;
    zs ^= 1;
    goto norm;
a_exp_bigger:
    if (ae == 0x7FF) return as ? JF_NAN : a;
    if (be == 0) --diff; else bs |= 0x4000000000000000ULL;
    bs = jam_right(bs, diff);
    as |= 0x4000000000000000ULL;
a_bigger:
    zsig = as - bs;
    ze = ae;
norm:
    --ze;
    return norm_round_pack(zs, ze, zsig);
}

jf64 jf_add(jf64 a, jf64 b) {
    if (jf_isnan(a) || jf_isnan(b)) return JF_NAN;
    return F_SIGN(a) == F_SIGN(b) ? add_sigs(a, b, F_SIGN(a)) : sub_sigs(a, b, F_SIGN(a));
}

jf64 jf_sub(jf64 a, jf64 b) {
    if (jf_isnan(a) || jf_isnan(b)) return JF_NAN;
    return F_SIGN(a) == F_SIGN(b) ? sub_sigs(a, b, F_SIGN(a)) : add_sigs(a, b, F_SIGN(a));
}

jf64 jf_mul(jf64 a, jf64 b) {
    int ae = F_EXP(a), be = F_EXP(b), ze;
    uint64_t as = F_FRAC(a), bs = F_FRAC(b), z0, z1;
    uint32_t zs = F_SIGN(a) ^ F_SIGN(b);
    if (jf_isnan(a) || jf_isnan(b)) return JF_NAN;
    if (ae == 0x7FF) return (be | bs) == 0 ? JF_NAN : pack(zs, 0x7FF, 0);
    if (be == 0x7FF) return (ae | as) == 0 ? JF_NAN : pack(zs, 0x7FF, 0);
    if (ae == 0) { if (as == 0) return pack(zs, 0, 0); norm_sub(as, &ae, &as); }
    if (be == 0) { if (bs == 0) return pack(zs, 0, 0); norm_sub(bs, &be, &bs); }
    ze = ae + be - 0x3FF;
    as = (as | HIDDEN) << 10;
    bs = (bs | HIDDEN) << 11;
    mul64(as, bs, &z0, &z1);
    z0 |= (z1 != 0);
    if ((int64_t)(z0 << 1) >= 0) { z0 <<= 1; --ze; }
    return round_pack(zs, ze, z0);
}

jf64 jf_div(jf64 a, jf64 b) {
    int ae = F_EXP(a), be = F_EXP(b), ze;
    uint64_t as = F_FRAC(a), bs = F_FRAC(b), q, r;
    uint32_t zs = F_SIGN(a) ^ F_SIGN(b);
    if (jf_isnan(a) || jf_isnan(b)) return JF_NAN;
    if (ae == 0x7FF) return be == 0x7FF ? JF_NAN : pack(zs, 0x7FF, 0);
    if (be == 0x7FF) return pack(zs, 0, 0);
    if (be == 0) {
        if (bs == 0) return (ae | as) == 0 ? JF_NAN : pack(zs, 0x7FF, 0);
        norm_sub(bs, &be, &bs);
    }
    if (ae == 0) { if (as == 0) return pack(zs, 0, 0); norm_sub(as, &ae, &as); }
    ze = ae - be + 0x3FD;
    as = (as | HIDDEN) << 10;
    bs = (bs | HIDDEN) << 11;
    if (bs <= as + as) { as >>= 1; ++ze; }
    q = div128(as, 0, bs, &r);
    q |= (r != 0);
    return round_pack(zs, ze, q);
}

/* |x| = sig * 2^(e - 1075) with sig in [2^52, 2^53) (subnormals too) */
static void unpack_n(jf64 a, uint64_t *sig, int *e) {
    int ex = F_EXP(a);
    uint64_t f = F_FRAC(a);
    if (ex == 0) {
        int s = clz64(f) - 11;
        *sig = f << s;
        *e = 1 - s;
    } else {
        *sig = f | HIDDEN;
        *e = ex;
    }
}

jf64 jf_rem(jf64 a, jf64 b) {
    uint64_t as, bs;
    int ae, be;
    if (jf_isnan(a) || jf_isnan(b) || jf_isinf(a) || jf_iszero(b)) return JF_NAN;
    if (jf_isinf(b) || jf_iszero(a)) return a;
    unpack_n(a, &as, &ae);
    unpack_n(b, &bs, &be);
    if (ae < be || (ae == be && as < bs)) return a;
    while (ae > be) {
        if (as >= bs) as -= bs;
        as <<= 1;
        ae--;
    }
    if (as >= bs) as -= bs;
    if (as == 0) return pack(F_SIGN(a), 0, 0);
    return norm_round_pack(F_SIGN(a), be + 9, as);
}

jf64 jf_sqrt(jf64 a) {
    uint64_t s, root = 0, rem = 0;
    int e, i;
    if (jf_isnan(a)) return JF_NAN;
    if (jf_iszero(a)) return a;
    if (F_SIGN(a)) return JF_NAN;
    if (jf_isinf(a)) return a;
    unpack_n(a, &s, &e);
    e -= 1075;
    if (e & 1) { s <<= 1; e -= 1; }
    /* root = floor(sqrt(s * 2^62)), two radicand bits per step */
    for (i = 57; i >= 0; i--) {
        int j = 2 * i + 1;
        uint64_t two = 0, trial;
        if (j >= 62 && j - 62 < 64) two |= ((s >> (j - 62)) & 1) << 1;
        if (j - 1 >= 62 && j - 1 - 62 < 64) two |= (s >> (j - 1 - 62)) & 1;
        rem = (rem << 2) | two;
        trial = (root << 2) | 1;
        if (rem >= trial) { rem -= trial; root = (root << 1) | 1; }
        else root <<= 1;
    }
    return norm_round_pack(0, e / 2 - 31 + 1084, root | (rem != 0));
}

int jf_eq(jf64 a, jf64 b) {
    if (jf_isnan(a) || jf_isnan(b)) return 0;
    return a == b || ((a | b) << 1) == 0;
}

int jf_lt(jf64 a, jf64 b) {
    uint32_t as = F_SIGN(a), bs = F_SIGN(b);
    if (jf_isnan(a) || jf_isnan(b)) return 0;
    if (as != bs) return as && ((a | b) << 1) != 0;
    return a != b && (as ^ (a < b));
}

int jf_le(jf64 a, jf64 b) {
    uint32_t as = F_SIGN(a), bs = F_SIGN(b);
    if (jf_isnan(a) || jf_isnan(b)) return 0;
    if (as != bs) return as || ((a | b) << 1) == 0;
    return a == b || (as ^ (a < b));
}

int jf_same_value(jf64 a, jf64 b) {
    if (jf_isnan(a) && jf_isnan(b)) return 1;
    return a == b;
}

jf64 jf_trunc(jf64 a) {
    int e = F_EXP(a);
    if (e == 0x7FF) return a;
    if (e < 1023) return pack(F_SIGN(a), 0, 0);
    if (e >= 1075) return a;
    return a & ~((1ULL << (1075 - e)) - 1);
}

jf64 jf_floor(jf64 a) {
    jf64 t = jf_trunc(a);
    if (F_SIGN(a) && t != a && !jf_isnan(a)) return jf_sub(t, JF_ONE);
    if (F_SIGN(a) && jf_iszero(t) && !jf_iszero(a)) return JF_NZERO;
    return t;
}

jf64 jf_ceil(jf64 a) {
    jf64 t = jf_trunc(a);
    if (!F_SIGN(a) && t != a && !jf_isnan(a)) return jf_add(t, JF_ONE);
    return t;
}

jf64 jf_round(jf64 a) {
    jf64 f, d;
    if (!jf_isfinite(a) || jf_is_integer(a)) return a;
    if (F_SIGN(a) && jf_le(jf_neg(a), 0x3FE0000000000000ULL)) return JF_NZERO;   /* -0.5 <= a < 0 */
    f = jf_floor(a);
    d = jf_sub(a, f);
    return jf_lt(d, 0x3FE0000000000000ULL) ? f : jf_add(f, JF_ONE);
}

jf64 jf_ldexp(jf64 a, int k) {
    uint64_t s;
    int e;
    if (!jf_isfinite(a) || jf_iszero(a)) return a;
    if (k > 4000) k = 4000;
    if (k < -4000) k = -4000;
    unpack_n(a, &s, &e);
    return norm_round_pack(F_SIGN(a), e + k + 9, s);
}

/* ---- extended significand for decimal conversion ---------------- */

typedef struct { uint64_t m; int e; int st; } jx_t;   /* m * 2^e */

static jx_t jx_u64(uint64_t m) {
    jx_t x;
    int s = clz64(m);
    x.m = m << s;
    x.e = -s;
    x.st = 0;
    return x;
}

static jx_t jx_mul(jx_t a, jx_t b) {
    jx_t z;
    uint64_t hi, lo;
    mul64(a.m, b.m, &hi, &lo);
    if (!(hi >> 63)) {
        hi = (hi << 1) | (lo >> 63);
        lo <<= 1;
        z.e = a.e + b.e + 63;
    } else {
        z.e = a.e + b.e + 64;
    }
    z.st = a.st | b.st | (lo != 0);
    if (lo >> 63) {
        hi++;
        if (hi == 0) { hi = 0x8000000000000000ULL; z.e++; }
    }
    z.m = hi;
    return z;
}

static jx_t jx_div(jx_t a, jx_t b) {
    jx_t z;
    uint64_t r;
    if (a.m >= b.m) {
        z.m = div128(a.m >> 1, a.m << 63, b.m, &r);
        z.e = a.e - b.e - 63;
    } else {
        z.m = div128(a.m, 0, b.m, &r);
        z.e = a.e - b.e - 64;
    }
    z.st = a.st | b.st | (r != 0);
    if (r != 0 && r >= b.m - r) {     /* round half up on the remainder */
        z.m++;
        if (z.m == 0) { z.m = 0x8000000000000000ULL; z.e++; }
    }
    return z;
}

static jx_t jx_pow10(int k) {
    jx_t r, b;
    r.m = 0x8000000000000000ULL; r.e = -63; r.st = 0;
    b.m = 0xA000000000000000ULL; b.e = -60; b.st = 0;
    while (k > 0) {
        if (k & 1) r = jx_mul(r, b);
        k >>= 1;
        if (k) b = jx_mul(b, b);
    }
    return r;
}

static jx_t jx_scale10(jx_t x, int k) {
    if (k > 0) return jx_mul(x, jx_pow10(k));
    if (k < 0) return jx_div(x, jx_pow10(-k));
    return x;
}

static jf64 jx_to_f64(uint32_t sign, jx_t x) {
    uint64_t sig;
    if (x.m == 0) return pack(sign, 0, 0);
    sig = (x.m >> 1) | (x.m & 1) | (uint64_t)(x.st != 0);
    return norm_round_pack(sign, x.e + 1 + 1084, sig);
}

static jx_t jx_f64(jf64 a) {
    uint64_t s;
    int e;
    jx_t x;
    unpack_n(a, &s, &e);
    x = jx_u64(s);
    x.e += e - 1075;
    return x;
}

static jf64 pow10_exact(int k) {      /* 10^k exact for 0 <= k <= 22 */
    jf64 r = JF_ONE, ten = 0x4024000000000000ULL;
    while (k-- > 0) r = jf_mul(r, ten);
    return r;
}

static const uint64_t k_pow10_u64[20] = {
    1ULL, 10ULL, 100ULL, 1000ULL, 10000ULL, 100000ULL, 1000000ULL, 10000000ULL, 100000000ULL,
    1000000000ULL, 10000000000ULL, 100000000000ULL, 1000000000000ULL, 10000000000000ULL,
    100000000000000ULL, 1000000000000000ULL, 10000000000000000ULL, 100000000000000000ULL,
    1000000000000000000ULL, 10000000000000000000ULL
};


/* ---- exact checks with a small bignum (32-bit limbs) ------------- */

#define BN_LIMBS 84
typedef struct { uint32_t w[BN_LIMBS]; int n; } bn_t;

static void bn_set(bn_t *a, uint64_t v) {
    a->n = 0;
    while (v) { a->w[a->n++] = (uint32_t)v; v >>= 32; }
}

static void bn_mul_small(bn_t *a, uint32_t m) {
    uint64_t carry = 0;
    int i;
    for (i = 0; i < a->n; i++) {
        uint64_t t = (uint64_t)a->w[i] * m + carry;
        a->w[i] = (uint32_t)t;
        carry = t >> 32;
    }
    if (carry && a->n < BN_LIMBS) a->w[a->n++] = (uint32_t)carry;
}

static void bn_mul_pow10(bn_t *a, int k) {
    static const uint32_t p[10] = { 1U, 10U, 100U, 1000U, 10000U, 100000U, 1000000U, 10000000U,
                                    100000000U, 1000000000U };
    while (k >= 9) { bn_mul_small(a, 1000000000U); k -= 9; }
    if (k > 0) bn_mul_small(a, p[k]);
}

static void bn_shl(bn_t *a, int k) {
    int limbs = k / 32, bitsh = k % 32, i;
    if (a->n == 0) return;
    if (a->n + limbs + 1 > BN_LIMBS) return;      /* bounded: callers stay far below */
    if (bitsh) {
        uint32_t carry = 0;
        for (i = 0; i < a->n; i++) {
            uint32_t w = a->w[i];
            a->w[i] = (w << bitsh) | carry;
            carry = w >> (32 - bitsh);
        }
        if (carry) a->w[a->n++] = carry;
    }
    if (limbs) {
        for (i = a->n - 1; i >= 0; i--) a->w[i + limbs] = a->w[i];
        for (i = 0; i < limbs; i++) a->w[i] = 0;
        a->n += limbs;
    }
}

static int bn_cmp(const bn_t *a, const bn_t *b) {
    int i;
    if (a->n != b->n) return a->n < b->n ? -1 : 1;
    for (i = a->n - 1; i >= 0; i--)
        if (a->w[i] != b->w[i]) return a->w[i] < b->w[i] ? -1 : 1;
    return 0;
}

/* D * 10^x against num * 2^pe, exactly */
static int cmp_dec_bin(uint64_t D, int x, uint64_t num, int pe) {
    static bn_t L, R;
    bn_set(&L, D);
    bn_set(&R, num);
    if (x >= 0) bn_mul_pow10(&L, x); else bn_mul_pow10(&R, -x);
    if (pe >= 0) bn_shl(&R, pe); else bn_shl(&L, -pe);
    return bn_cmp(&L, &R);
}

/* z (positive, within a few ulps of D * 10^x) moved to the correctly
 * rounded double; trunc: the true value is a little above D * 10^x */
static jf64 refine(jf64 z, uint64_t D, int x, int trunc) {
    int iter;
    for (iter = 0; iter < 8; iter++) {
        int expf, e, c;
        uint64_t frac, sig;
        if (z == JF_INF) {
            c = cmp_dec_bin(D, x, (((1ULL << 53) - 1) << 1) | 1, 970);
            if (c < 0) { z = JF_MAXV; continue; }
            return z;
        }
        expf = F_EXP(z);
        frac = F_FRAC(z);
        sig = expf ? (frac | HIDDEN) : frac;
        e = (expf ? expf : 1) - 1075;
        c = cmp_dec_bin(D, x, 2 * sig + 1, e - 1);
        if (c > 0 || (c == 0 && (trunc || (sig & 1)))) { z++; continue; }
        if (sig == 0) break;
        if (frac == 0 && expf > 1) c = cmp_dec_bin(D, x, 4 * sig - 1, e - 2);
        else c = cmp_dec_bin(D, x, 2 * sig - 1, e - 1);
        if (c < 0 || (c == 0 && (sig & 1) && !trunc)) { z--; continue; }
        break;
    }
    return z;
}

/* m * 10^e10 (m < 10^19), trunc: nonzero digits were dropped */
static jf64 dec_to_f64(uint32_t sign, uint64_t m, int e10, int trunc) {
    jx_t x;
    jf64 r;
    if (m == 0) return pack(sign, 0, 0);
    if (!trunc && m < (1ULL << 53)) {
        if (e10 >= 0 && e10 <= 22) {
            r = jf_mul(jf_from_u64(m), pow10_exact(e10));
            return sign ? jf_neg(r) : r;
        }
        if (e10 < 0 && e10 >= -22) {
            r = jf_div(jf_from_u64(m), pow10_exact(-e10));
            return sign ? jf_neg(r) : r;
        }
        if (e10 > 22 && e10 <= 22 + 15) {
            int extra = e10 - 22;
            uint64_t lim = (1ULL << 53);
            if (m < udiv64(lim, k_pow10_u64[extra], 0)) {
                r = jf_mul(jf_from_u64(m * k_pow10_u64[extra]), pow10_exact(22));
                return sign ? jf_neg(r) : r;
            }
        }
    }
    if (e10 > 330) return pack(sign, 0x7FF, 0);
    if (e10 < -360) return pack(sign, 0, 0);
    x = jx_u64(m);
    x.st = trunc;
    x = jx_scale10(x, e10);
    r = refine(jx_to_f64(0, x), m, e10, trunc);
    return sign ? jf_neg(r) : r;
}

static int is_dig(char c) { return c >= '0' && c <= '9'; }

jf64 jf_parse(const char *s, int len, int *used) {
    int i = 0, nd = 0, e10 = 0, trunc = 0, any = 0;
    uint32_t sign = 0;
    uint64_t m = 0;
    if (used) *used = 0;
    if (i < len && (s[i] == '+' || s[i] == '-')) { sign = s[i] == '-'; i++; }
    if (len - i >= 8 && s[i] == 'I') {
        const char *w = "Infinity";
        int k;
        for (k = 0; k < 8 && s[i + k] == w[k]; k++) {}
        if (k == 8) { if (used) *used = i + 8; return pack(sign, 0x7FF, 0); }
        return JF_NAN;
    }
    while (i < len && is_dig(s[i])) {
        int d = s[i] - '0';
        any = 1;
        if (m == 0 && d == 0) {}
        else if (nd < 19) { m = m * 10 + (uint64_t)d; nd++; }
        else { e10++; if (d) trunc = 1; }
        i++;
    }
    if (i < len && s[i] == '.') {
        int j = i + 1;
        if (j < len && is_dig(s[j])) any = 1;
        if (any) {
            i++;
            while (i < len && is_dig(s[i])) {
                int d = s[i] - '0';
                if (m == 0 && d == 0) e10--;
                else if (nd < 19) { m = m * 10 + (uint64_t)d; nd++; e10--; }
                else if (d) trunc = 1;
                i++;
            }
        }
    }
    if (!any) return JF_NAN;
    if (i < len && (s[i] == 'e' || s[i] == 'E')) {
        int j = i + 1, es = 0, ev = 0;
        if (j < len && (s[j] == '+' || s[j] == '-')) { es = s[j] == '-'; j++; }
        if (j < len && is_dig(s[j])) {
            while (j < len && is_dig(s[j])) { if (ev < 100000) ev = ev * 10 + (s[j] - '0'); j++; }
            e10 += es ? -ev : ev;
            i = j;
        }
    }
    if (used) *used = i;
    return dec_to_f64(sign, m, e10, trunc);
}

/* ---- formatting -------------------------------------------------- */

static int floor_log10_pow2(int l) {    /* floor(l * log10(2)) */
    int64_t p = (int64_t)l * 78913;
    if (p >= 0) return (int)(p >> 18);
    return -(int)((-p + 262143) >> 18);
}

/* D with exactly k digits (1..19) and est such that v ~ D * 10^(est-k+1) */
static void digits_for(jf64 v, int k, uint64_t *D, int *est_out) {
    jx_t x = jx_f64(v), y;
    int est = floor_log10_pow2(x.e + 63), tries;
    uint64_t d = 0;
    for (tries = 0; tries < 6; tries++) {
        int sh;
        y = jx_scale10(x, k - 1 - est);
        if (y.e >= 0) { est++; continue; }
        sh = -y.e;
        if (sh >= 64) d = 0;
        else {
            d = y.m >> sh;
            if ((y.m >> (sh - 1)) & 1) d++;
        }
        if (d >= k_pow10_u64[k]) { est++; continue; }
        if (d < k_pow10_u64[k - 1]) { est--; continue; }
        break;
    }
    *D = d;
    *est_out = est;
}

static int u64_dec(uint64_t v, char *out) {     /* digits, no NUL */
    char t[24];
    int n = 0, i;
    do {
        uint64_t r;
        v = udiv64(v, 10, &r);
        t[n++] = (char)('0' + (int)r);
    } while (v);
    for (i = 0; i < n; i++) out[i] = t[n - 1 - i];
    return n;
}

static int put(char *out, int max, int n, char c) {
    if (n < max - 1) out[n] = c;
    return n + 1;
}

static int finish(char *out, int max, int n) {
    if (max <= 0) return 0;
    out[n < max - 1 ? n : max - 1] = 0;
    return n < max - 1 ? n : max - 1;
}

static int special(jf64 v, char *out, int max) {
    const char *s = 0;
    int n = 0;
    if (jf_isnan(v)) s = "NaN";
    else if (v == JF_INF) s = "Infinity";
    else if (v == JF_NINF) s = "-Infinity";
    else if (jf_iszero(v)) s = "0";
    if (!s) return -1;
    while (s[n]) { n = put(out, max, n, s[n]); }
    return finish(out, max, n);
}

/* shortest digits: str gets k digits, returns k; *n is the decimal exponent */
static int shortest(jf64 v, char *str, int *n_out) {
    uint64_t D = 0;
    int k, est = 0, len;
    for (k = 1; k <= 17; k++) {
        digits_for(v, k, &D, &est);
        if (dec_to_f64(0, D, est - k + 1, 0) == v) break;
        if (k == 17) {
            if (dec_to_f64(0, D + 1, est - k + 1, 0) == v) D++;
            else if (D > 0 && dec_to_f64(0, D - 1, est - k + 1, 0) == v) D--;
        }
    }
    if (k > 17) k = 17;
    while (k > 1) {
        uint64_t r, q = udiv64(D, 10, &r);
        if (r != 0) break;
        D = q;
        k--;
    }
    len = u64_dec(D, str);
    if (len != k) {          /* D rounded up to 10^k: one more digit */
        est += len - k;
        k = len;
        while (k > 1 && str[k - 1] == '0') k--;
    }
    *n_out = est + 1;
    return k;
}

int jf_format(jf64 v, char *out, int max) {
    char d[24];
    int n = 0, k, e, i, sp;
    int64_t iv;
    sp = special(v, out, max);
    if (sp >= 0) return sp;
    if (F_SIGN(v)) { n = put(out, max, n, '-'); v = jf_neg(v); }
    if (jf_to_i64(v, &iv) && iv < 1000000000000000000LL) {
        int len = u64_dec((uint64_t)iv, d);
        if (len <= 21) {
            for (i = 0; i < len; i++) n = put(out, max, n, d[i]);
            return finish(out, max, n);
        }
    }
    k = shortest(v, d, &e);
    if (k <= e && e <= 21) {
        for (i = 0; i < k; i++) n = put(out, max, n, d[i]);
        for (i = 0; i < e - k; i++) n = put(out, max, n, '0');
    } else if (0 < e && e <= 21) {
        for (i = 0; i < e; i++) n = put(out, max, n, d[i]);
        n = put(out, max, n, '.');
        for (; i < k; i++) n = put(out, max, n, d[i]);
    } else if (-6 < e && e <= 0) {
        n = put(out, max, n, '0');
        n = put(out, max, n, '.');
        for (i = 0; i < -e; i++) n = put(out, max, n, '0');
        for (i = 0; i < k; i++) n = put(out, max, n, d[i]);
    } else {
        char t[8];
        int ex = e - 1, tl;
        n = put(out, max, n, d[0]);
        if (k > 1) {
            n = put(out, max, n, '.');
            for (i = 1; i < k; i++) n = put(out, max, n, d[i]);
        }
        n = put(out, max, n, 'e');
        n = put(out, max, n, ex < 0 ? '-' : '+');
        tl = u64_dec((uint64_t)(ex < 0 ? -ex : ex), t);
        for (i = 0; i < tl; i++) n = put(out, max, n, t[i]);
    }
    return finish(out, max, n);
}

/* exact decimal digits of an integer-valued double (|v| >= 2^53) */
static int big_int_digits(jf64 v, char *buf, int cap) {
    uint64_t s;
    int e, len, i, k;
    unpack_n(v, &s, &e);
    e -= 1075;
    len = u64_dec(s, buf);
    for (k = 0; k < e; k++) {       /* double the decimal string e times */
        int carry = 0;
        for (i = len - 1; i >= 0; i--) {
            int d = (buf[i] - '0') * 2 + carry;
            buf[i] = (char)('0' + d % 10);
            carry = d / 10;
        }
        if (carry) {
            if (len >= cap) return len;
            for (i = len; i > 0; i--) buf[i] = buf[i - 1];
            buf[0] = (char)('0' + carry);
            len++;
        }
    }
    return len;
}

/* the bits of a at and above bit q (small: a < 10 * 2^q), and a cut to
 * its bits below q */
static uint32_t bn_split(bn_t *a, int q) {
    int limb = q / 32, bit = q % 32, i;
    uint32_t lo = limb < a->n ? a->w[limb] : 0, hi = limb + 1 < a->n ? a->w[limb + 1] : 0, r;
    r = bit ? ((hi << (32 - bit)) | (lo >> bit)) : lo;
    if (limb < a->n) a->w[limb] = bit ? (lo & ((1U << bit) - 1)) : 0;
    for (i = limb + 1; i < a->n; i++) a->w[i] = 0;
    while (a->n > 0 && a->w[a->n - 1] == 0) a->n--;
    return r;
}

/* 1 when 2 * a >= 2^q */
static int bn_half_up(const bn_t *a, int q) {
    static bn_t t, h;
    int i;
    for (i = 0; i < a->n; i++) t.w[i] = a->w[i];
    t.n = a->n;
    bn_shl(&t, 1);
    bn_set(&h, 1);
    bn_shl(&h, q);
    return bn_cmp(&t, &h) >= 0;
}

/* decimal string increment (no NUL); returns the new length */
static int dec_inc(char *d, int len, int cap) {
    int i = len - 1;
    while (i >= 0) {
        if (d[i] == '9') { d[i] = '0'; i--; continue; }
        d[i]++;
        return len;
    }
    if (len >= cap) return len;
    for (i = len; i > 0; i--) d[i] = d[i - 1];
    d[0] = '1';
    return len + 1;
}

int jf_format_fixed(jf64 v, int digits, char *out, int max) {
    static char ip[400];
    static bn_t F;
    char fd[24];
    int n = 0, i, il;
    jf64 t, fr;
    if (digits < 0) digits = 0;
    if (digits > 20) digits = 20;
    if (jf_isnan(v)) return special(v, out, max);
    if (!jf_lt(jf_abs(v), 0x444B1AE4D6E2EF50ULL)) return jf_format(v, out, max);   /* >= 1e21 */
    if (F_SIGN(v)) {
        if (!jf_iszero(v)) n = put(out, max, n, '-');
        v = jf_neg(v);
    }
    t = jf_trunc(v);
    fr = jf_sub(v, t);
    if (jf_lt(t, 0x4340000000000000ULL)) {
        int64_t iv = 0;
        (void)jf_to_i64(t, &iv);
        il = u64_dec((uint64_t)iv, ip);
    } else {
        il = big_int_digits(t, ip, (int)sizeof(ip) - 1);
    }
    for (i = 0; i < digits; i++) fd[i] = '0';
    if (!jf_iszero(fr)) {
        uint64_t sig;
        int e, q, up;
        unpack_n(fr, &sig, &e);
        q = 1075 - e;                       /* fr = sig / 2^q */
        bn_set(&F, sig);
        for (i = 0; i < digits; i++) {
            bn_mul_small(&F, 10);
            fd[i] = (char)('0' + bn_split(&F, q));
        }
        up = F.n > 0 && bn_half_up(&F, q);
        if (up) {
            int k = digits - 1;
            while (k >= 0) {
                if (fd[k] == '9') { fd[k] = '0'; k--; continue; }
                fd[k]++;
                break;
            }
            if (k < 0) il = dec_inc(ip, il, (int)sizeof(ip) - 1);
        }
    }
    for (i = 0; i < il; i++) n = put(out, max, n, ip[i]);
    if (digits > 0) {
        n = put(out, max, n, '.');
        for (i = 0; i < digits; i++) n = put(out, max, n, fd[i]);
    }
    return finish(out, max, n);
}

static int put_exp(char *out, int max, int n, int ex) {
    char t[8];
    int tl, i;
    n = put(out, max, n, 'e');
    n = put(out, max, n, ex < 0 ? '-' : '+');
    tl = u64_dec((uint64_t)(ex < 0 ? -ex : ex), t);
    for (i = 0; i < tl; i++) n = put(out, max, n, t[i]);
    return n;
}

int jf_format_precision(jf64 v, int p, char *out, int max) {
    char d[24];
    uint64_t D;
    int n = 0, est, i, len;
    if (!jf_isfinite(v)) return special(v, out, max);
    if (p < 1) p = 1;
    if (p > 19) p = 19;
    if (F_SIGN(v) && !jf_iszero(v)) { n = put(out, max, n, '-'); v = jf_neg(v); }
    if (jf_iszero(v)) {
        n = put(out, max, n, '0');
        if (p > 1) { n = put(out, max, n, '.'); for (i = 1; i < p; i++) n = put(out, max, n, '0'); }
        return finish(out, max, n);
    }
    digits_for(v, p, &D, &est);
    len = u64_dec(D, d);
    if (len > p) { est++; len = p; }
    if (est < -6 || est >= p) {
        n = put(out, max, n, d[0]);
        if (p > 1) { n = put(out, max, n, '.'); for (i = 1; i < p; i++) n = put(out, max, n, d[i]); }
        n = put_exp(out, max, n, est);
        return finish(out, max, n);
    }
    if (est >= 0) {
        for (i = 0; i <= est; i++) n = put(out, max, n, d[i]);
        if (est + 1 < p) {
            n = put(out, max, n, '.');
            for (i = est + 1; i < p; i++) n = put(out, max, n, d[i]);
        }
    } else {
        n = put(out, max, n, '0');
        n = put(out, max, n, '.');
        for (i = 0; i < -est - 1; i++) n = put(out, max, n, '0');
        for (i = 0; i < p; i++) n = put(out, max, n, d[i]);
    }
    return finish(out, max, n);
}

int jf_format_exp(jf64 v, int f, char *out, int max) {
    char d[24];
    uint64_t D;
    int n = 0, est, i, k;
    if (!jf_isfinite(v)) return special(v, out, max);
    if (f > 18) f = 18;
    if (F_SIGN(v) && !jf_iszero(v)) { n = put(out, max, n, '-'); v = jf_neg(v); }
    if (jf_iszero(v)) {
        n = put(out, max, n, '0');
        if (f > 0) { n = put(out, max, n, '.'); for (i = 0; i < f; i++) n = put(out, max, n, '0'); }
        n = put_exp(out, max, n, 0);
        return finish(out, max, n);
    }
    if (f < 0) {
        int e;
        k = shortest(v, d, &e);
        est = e - 1;
    } else {
        digits_for(v, f + 1, &D, &est);
        k = u64_dec(D, d);
        if (k > f + 1) { est++; k = f + 1; }
    }
    n = put(out, max, n, d[0]);
    if (k > 1) { n = put(out, max, n, '.'); for (i = 1; i < k; i++) n = put(out, max, n, d[i]); }
    n = put_exp(out, max, n, est);
    return finish(out, max, n);
}

int jf_format_radix(jf64 v, int radix, char *out, int max) {
    static const char dig[] = "0123456789abcdefghijklmnopqrstuvwxyz";
    char t[80];
    int n = 0, tl = 0, i, fr_digits = 0;
    int64_t iv;
    jf64 ip, fr;
    if (radix == 10 || radix < 2 || radix > 36) return jf_format(v, out, max);
    if (!jf_isfinite(v) || jf_iszero(v)) return special(v, out, max);
    if (F_SIGN(v)) { n = put(out, max, n, '-'); v = jf_neg(v); }
    ip = jf_trunc(v);
    fr = jf_sub(v, ip);
    if (!jf_to_i64(ip, &iv)) return jf_format(v, out, max);
    do {
        uint64_t r;
        iv = (int64_t)udiv64((uint64_t)iv, (uint64_t)radix, &r);
        t[tl++] = dig[r];
    } while (iv && tl < 70);
    for (i = tl - 1; i >= 0; i--) n = put(out, max, n, t[i]);
    if (!jf_iszero(fr)) {
        jf64 rb = jf_from_i32(radix);
        n = put(out, max, n, '.');
        while (!jf_iszero(fr) && fr_digits < 52) {
            int32_t dv;
            jf64 x = jf_mul(fr, rb), xi = jf_trunc(x);
            (void)jf_is_i32(xi, &dv);
            n = put(out, max, n, dig[dv]);
            fr = jf_sub(x, xi);
            fr_digits++;
        }
    }
    return finish(out, max, n);
}

/* ---- math functions (series, a few ulps) ------------------------- */

#define C_HALF   0x3FE0000000000000ULL
#define C_TWO    0x4000000000000000ULL
#define C_PIO2   0x3FF921FB54442D18ULL
#define C_PIO4   0x3FE921FB54442D18ULL
#define C_PIO6   0x3FE0C152382D7366ULL
#define C_SQRT3  0x3FFBB67AE8584CAAULL
#define C_LN2LO  0x3C7ABC9E3B39803FULL
#define C_TAN15  0x3FD126145E9ECD56ULL   /* tan(pi/12) */

jf64 jf_exp(jf64 a) {
    jf64 k, r, term, sum;
    int32_t ki = 0;
    int i;
    if (jf_isnan(a)) return a;
    if (a == JF_INF) return JF_INF;
    if (a == JF_NINF) return 0;
    if (jf_lt(0x40862E42FEFA39EFULL, a)) return JF_INF;     /* > 709.78 */
    if (jf_lt(a, 0xC0874910D52D3051ULL)) return 0;          /* < -745.13 */
    k = jf_abs(jf_round(jf_div(a, JF_LN2))) == 0 ? 0 : jf_round(jf_div(a, JF_LN2));
    (void)jf_is_i32(k, &ki);
    r = jf_sub(jf_sub(a, jf_mul(k, JF_LN2)), jf_mul(k, C_LN2LO));
    sum = JF_ONE;
    term = JF_ONE;
    for (i = 1; i < 30; i++) {
        term = jf_div(jf_mul(term, r), jf_from_i32(i));
        if (jf_iszero(term)) break;
        sum = jf_add(sum, term);
    }
    return jf_ldexp(sum, ki);
}

jf64 jf_log(jf64 a) {
    uint64_t s;
    int e, i;
    jf64 m, t, t2, term, sum;
    if (jf_isnan(a) || (F_SIGN(a) && !jf_iszero(a))) return JF_NAN;
    if (jf_iszero(a)) return JF_NINF;
    if (a == JF_INF) return a;
    if (a == JF_ONE) return 0;
    unpack_n(a, &s, &e);
    e -= 1023;                           /* a = (s / 2^52) * 2^e */
    m = pack(0, 1023, s & 0x000FFFFFFFFFFFFFULL);   /* [1, 2) */
    if (jf_lt(JF_SQRT2, m)) { m = jf_mul(m, C_HALF); e++; }
    t = jf_div(jf_sub(m, JF_ONE), jf_add(m, JF_ONE));
    t2 = jf_mul(t, t);
    sum = 0;
    term = t;
    for (i = 1; i < 60; i += 2) {
        jf64 add = jf_div(term, jf_from_i32(i));
        if (jf_iszero(add)) break;
        sum = jf_add(sum, add);
        term = jf_mul(term, t2);
    }
    sum = jf_mul(sum, C_TWO);
    return jf_add(jf_add(jf_mul(jf_from_i32(e), C_LN2LO), sum), jf_mul(jf_from_i32(e), JF_LN2));
}

static jf64 sin_series(jf64 x) {
    jf64 x2 = jf_mul(x, x), term = x, sum = x;
    int i;
    for (i = 1; i < 20; i++) {
        term = jf_div(jf_mul(term, jf_neg(x2)), jf_from_i32((2 * i) * (2 * i + 1)));
        if (jf_iszero(term)) break;
        sum = jf_add(sum, term);
    }
    return sum;
}

static jf64 cos_series(jf64 x) {
    jf64 x2 = jf_mul(x, x), term = JF_ONE, sum = JF_ONE;
    int i;
    for (i = 1; i < 20; i++) {
        term = jf_div(jf_mul(term, jf_neg(x2)), jf_from_i32((2 * i - 1) * (2 * i)));
        if (jf_iszero(term)) break;
        sum = jf_add(sum, term);
    }
    return sum;
}

/* x = q * pi/2 + r, |r| <= pi/4; q mod 4 */
/* Cody-Waite with pi/2 in three parts whose leading ones have 33 bits,
 * so q * part is exact for |q| < 2^20 (|x| below about 1.6e6) */
#define C_P1  0x3FF921FB54400000ULL
#define C_P2  0x3DD0B4611A600000ULL
#define C_P2T 0x3BA3198A2E037073ULL
static jf64 reduce_pio2(jf64 x, int *quad) {
    jf64 q = jf_round(jf_div(x, C_PIO2));
    int64_t qi = 0;
    if (jf_iszero(q)) q = 0;
    (void)jf_to_i64(q, &qi);
    *quad = (int)(qi & 3);
    if (jf_iszero(q)) return x;
    x = jf_sub(x, jf_mul(q, C_P1));
    x = jf_sub(x, jf_mul(q, C_P2));
    return jf_sub(x, jf_mul(q, C_P2T));
}

jf64 jf_sin(jf64 a) {
    int q;
    jf64 r;
    if (!jf_isfinite(a)) return JF_NAN;
    if (jf_iszero(a)) return a;
    r = reduce_pio2(a, &q);
    switch (q) {
    case 0: return sin_series(r);
    case 1: return cos_series(r);
    case 2: return jf_neg(sin_series(r));
    default: return jf_neg(cos_series(r));
    }
}

jf64 jf_cos(jf64 a) {
    int q;
    jf64 r;
    if (!jf_isfinite(a)) return JF_NAN;
    r = reduce_pio2(a, &q);
    switch (q) {
    case 0: return cos_series(r);
    case 1: return jf_neg(sin_series(r));
    case 2: return jf_neg(cos_series(r));
    default: return sin_series(r);
    }
}

jf64 jf_tan(jf64 a) {
    if (!jf_isfinite(a)) return JF_NAN;
    if (jf_iszero(a)) return a;
    return jf_div(jf_sin(a), jf_cos(a));
}

static jf64 atan_series(jf64 x) {       /* |x| <= tan(pi/12) */
    jf64 x2 = jf_mul(x, x), term = x, sum = x;
    int i;
    for (i = 1; i < 40; i++) {
        jf64 add;
        term = jf_mul(term, jf_neg(x2));
        add = jf_div(term, jf_from_i32(2 * i + 1));
        if (jf_iszero(add)) break;
        sum = jf_add(sum, add);
    }
    return sum;
}

jf64 jf_atan(jf64 a) {
    uint32_t neg = F_SIGN(a);
    jf64 x = jf_abs(a), r;
    int inv = 0, shift = 0;
    if (jf_isnan(a)) return a;
    if (jf_iszero(a)) return a;
    if (jf_isinf(a)) return neg ? jf_neg(C_PIO2) : C_PIO2;
    if (jf_lt(JF_ONE, x)) { x = jf_div(JF_ONE, x); inv = 1; }
    if (jf_lt(C_TAN15, x)) {
        x = jf_div(jf_sub(jf_mul(x, C_SQRT3), JF_ONE), jf_add(C_SQRT3, x));
        shift = 1;
    }
    r = atan_series(x);
    if (shift) r = jf_add(C_PIO6, r);
    if (inv) r = jf_sub(C_PIO2, r);
    return neg ? jf_neg(r) : r;
}

jf64 jf_atan2(jf64 y, jf64 x) {
    if (jf_isnan(y) || jf_isnan(x)) return JF_NAN;
    if (jf_iszero(y)) {
        if (F_SIGN(x)) return F_SIGN(y) ? jf_neg(JF_PI) : JF_PI;
        return y;
    }
    if (jf_iszero(x)) return F_SIGN(y) ? jf_neg(C_PIO2) : C_PIO2;
    if (jf_isinf(x)) {
        if (jf_isinf(y)) {
            jf64 r = F_SIGN(x) ? jf_mul(JF_PI, 0x3FE8000000000000ULL) : C_PIO4;
            return F_SIGN(y) ? jf_neg(r) : r;
        }
        if (F_SIGN(x)) return F_SIGN(y) ? jf_neg(JF_PI) : JF_PI;
        return F_SIGN(y) ? JF_NZERO : 0;
    }
    if (jf_isinf(y)) return F_SIGN(y) ? jf_neg(C_PIO2) : C_PIO2;
    {
        jf64 r = jf_atan(jf_div(y, x));
        if (F_SIGN(x)) r = F_SIGN(y) ? jf_sub(r, JF_PI) : jf_add(r, JF_PI);
        return r;
    }
}

jf64 jf_asin(jf64 a) {
    jf64 x = jf_abs(a);
    if (jf_isnan(a) || jf_lt(JF_ONE, x)) return JF_NAN;
    if (x == JF_ONE) return F_SIGN(a) ? jf_neg(C_PIO2) : C_PIO2;
    return jf_atan(jf_div(a, jf_sqrt(jf_sub(JF_ONE, jf_mul(a, a)))));
}

jf64 jf_acos(jf64 a) {
    jf64 x = jf_abs(a);
    if (jf_isnan(a) || jf_lt(JF_ONE, x)) return JF_NAN;
    return jf_sub(C_PIO2, jf_asin(a));
}

jf64 jf_pow(jf64 x, jf64 y) {
    int64_t yi;
    if (jf_isnan(y)) return JF_NAN;
    if (jf_iszero(y)) return JF_ONE;
    if (jf_isnan(x)) return JF_NAN;
    if (jf_isinf(y)) {
        jf64 ax = jf_abs(x);
        if (ax == JF_ONE) return JF_NAN;
        if (jf_lt(JF_ONE, ax)) return F_SIGN(y) ? 0 : JF_INF;
        return F_SIGN(y) ? JF_INF : 0;
    }
    if (jf_to_i64(y, &yi) && yi > -0x40000000LL && yi < 0x40000000LL) {
        jf64 r = JF_ONE, b = x;
        int64_t n = yi < 0 ? -yi : yi;
        if (jf_iszero(x) || jf_isinf(x)) {
            int odd = (int)(n & 1);
            uint32_t s = odd ? F_SIGN(x) : 0;
            int big = jf_isinf(x) ? yi > 0 : yi < 0;
            return big ? pack(s, 0x7FF, 0) : pack(s, 0, 0);
        }
        while (n) {
            if (n & 1) r = jf_mul(r, b);
            n >>= 1;
            if (n) b = jf_mul(b, b);
        }
        return yi < 0 ? jf_div(JF_ONE, r) : r;
    }
    if (jf_iszero(x)) return F_SIGN(y) ? JF_INF : 0;
    if (x == JF_INF) return F_SIGN(y) ? 0 : JF_INF;
    if (x == JF_NINF) return F_SIGN(y) ? 0 : JF_INF;
    if (F_SIGN(x)) return JF_NAN;
    return jf_exp(jf_mul(y, jf_log(x)));
}

jf64 jf_cbrt(jf64 a) {
    jf64 r, x = jf_abs(a);
    int i;
    if (!jf_isfinite(a) || jf_iszero(a)) return a;
    r = jf_exp(jf_div(jf_log(x), 0x4008000000000000ULL));
    for (i = 0; i < 2; i++)      /* Newton: r = r - (r^3 - x) / (3 r^2) */
        r = jf_sub(r, jf_div(jf_sub(jf_mul(jf_mul(r, r), r), x), jf_mul(0x4008000000000000ULL, jf_mul(r, r))));
    return F_SIGN(a) ? jf_neg(r) : r;
}

jf64 jf_hypot(jf64 a, jf64 b) {
    jf64 x = jf_abs(a), y = jf_abs(b), t;
    if (jf_isinf(a) || jf_isinf(b)) return JF_INF;
    if (jf_isnan(a) || jf_isnan(b)) return JF_NAN;
    if (jf_lt(x, y)) { t = x; x = y; y = t; }
    if (jf_iszero(x)) return 0;
    t = jf_div(y, x);
    return jf_mul(x, jf_sqrt(jf_add(JF_ONE, jf_mul(t, t))));
}
