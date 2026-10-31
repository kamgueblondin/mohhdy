/* wf_bn.c - Lot 67: bounded big numbers, Montgomery arithmetic (CIOS). */
#include "wf_bn.h"

void wf_bn_zero(wf_bn_t* a) {
    int i;
    for (i = 0; i < WF_BN_LIMBS; i++) a->v[i] = 0U;
}

void wf_bn_copy(wf_bn_t* d, const wf_bn_t* s) {
    int i;
    if (d == s) return;
    for (i = 0; i < WF_BN_LIMBS; i++) d->v[i] = s->v[i];
}

void wf_bn_set_u32(wf_bn_t* a, uint32_t x) {
    wf_bn_zero(a);
    a->v[0] = x;
}

int wf_bn_from_bytes(wf_bn_t* a, const uint8_t* b, int len) {
    int i;
    wf_bn_zero(a);
    while (len > 0 && b[0] == 0U) { b++; len--; }
    if (len > (WF_BN_LIMBS - 2) * 4) return -1;
    for (i = 0; i < len; i++) {
        int bytepos = len - 1 - i;
        a->v[i / 4] |= (uint32_t)b[bytepos] << (8 * (i % 4));
    }
    return 0;
}

int wf_bn_to_bytes(const wf_bn_t* a, uint8_t* b, int len) {
    int i;
    if (wf_bn_bits(a) > len * 8) return -1;
    for (i = 0; i < len; i++) {
        int limb = i / 4;
        b[len - 1 - i] = limb < WF_BN_LIMBS ? (uint8_t)(a->v[limb] >> (8 * (i % 4))) : 0U;
    }
    return 0;
}

int wf_bn_cmp(const wf_bn_t* a, const wf_bn_t* b) {
    int i;
    for (i = WF_BN_LIMBS - 1; i >= 0; i--) {
        if (a->v[i] > b->v[i]) return 1;
        if (a->v[i] < b->v[i]) return -1;
    }
    return 0;
}

int wf_bn_is_zero(const wf_bn_t* a) {
    int i;
    for (i = 0; i < WF_BN_LIMBS; i++)
        if (a->v[i]) return 0;
    return 1;
}

int wf_bn_bits(const wf_bn_t* a) {
    int i, b;
    for (i = WF_BN_LIMBS - 1; i >= 0; i--) {
        if (!a->v[i]) continue;
        for (b = 31; b >= 0; b--)
            if (a->v[i] & (1U << b)) return i * 32 + b + 1;
    }
    return 0;
}

int wf_bn_bit(const wf_bn_t* a, int i) {
    if (i < 0 || i >= WF_BN_LIMBS * 32) return 0;
    return (int)((a->v[i / 32] >> (i % 32)) & 1U);
}

uint32_t wf_bn_sub(wf_bn_t* r, const wf_bn_t* a, const wf_bn_t* b) {
    uint32_t borrow = 0U;
    int i;
    for (i = 0; i < WF_BN_LIMBS; i++) {
        uint64_t d = (uint64_t)a->v[i] - b->v[i] - borrow;
        r->v[i] = (uint32_t)d;
        borrow = (uint32_t)((d >> 32) & 1U);
    }
    return borrow;
}

uint32_t wf_bn_add(wf_bn_t* r, const wf_bn_t* a, const wf_bn_t* b) {
    uint64_t carry = 0U;
    int i;
    for (i = 0; i < WF_BN_LIMBS; i++) {
        carry += (uint64_t)a->v[i] + b->v[i];
        r->v[i] = (uint32_t)carry;
        carry >>= 32;
    }
    return (uint32_t)carry;
}

void wf_bn_reduce_once(wf_bn_t* a, const wf_bn_t* m) {
    if (wf_bn_cmp(a, m) >= 0) (void)wf_bn_sub(a, a, m);
}

static void bn_shl1(wf_bn_t* a) {
    int i;
    for (i = WF_BN_LIMBS - 1; i > 0; i--) a->v[i] = (a->v[i] << 1) | (a->v[i - 1] >> 31);
    a->v[0] <<= 1;
}

void wf_bn_mod(wf_bn_t* r, const wf_bn_t* a, const wf_bn_t* m) {
    /* Bitwise long division: r = (r << 1 | bit) mod m, top bit first. */
    static wf_bn_t acc;
    int i, top = wf_bn_bits(a);
    wf_bn_zero(&acc);
    for (i = top - 1; i >= 0; i--) {
        bn_shl1(&acc);
        acc.v[0] |= (uint32_t)wf_bn_bit(a, i);
        wf_bn_reduce_once(&acc, m);
    }
    wf_bn_copy(r, &acc);
}

int wf_mont_init(wf_mont_t* ctx, const wf_bn_t* m) {
    uint32_t inv = 1U;
    int i, n;
    if (!(m->v[0] & 1U)) return -1;
    if (m->v[WF_BN_LIMBS - 1] || m->v[WF_BN_LIMBS - 2]) return -1;
    wf_bn_copy(&ctx->m, m);
    ctx->bits = wf_bn_bits(m);
    n = (ctx->bits + 31) / 32;
    if (n < 1) return -1;
    ctx->n = n;
    /* Newton iteration for m0^-1 mod 2^32. */
    for (i = 0; i < 5; i++) inv *= 2U - m->v[0] * inv;
    ctx->m0inv = (uint32_t)(0U - inv);
    /* one = R mod m and r2 = R^2 mod m by doubling (R = 2^(32n)). */
    wf_bn_set_u32(&ctx->one, 1U);
    for (i = 0; i < 32 * n; i++) {
        bn_shl1(&ctx->one);
        wf_bn_reduce_once(&ctx->one, m);
    }
    wf_bn_copy(&ctx->r2, &ctx->one);
    for (i = 0; i < 32 * n; i++) {
        bn_shl1(&ctx->r2);
        wf_bn_reduce_once(&ctx->r2, m);
    }
    return 0;
}

void wf_mont_mul(const wf_mont_t* ctx, wf_bn_t* r, const wf_bn_t* a, const wf_bn_t* b) {
    uint32_t t[WF_BN_LIMBS + 2];
    int i, j, n = ctx->n;
    wf_bn_t out;
    for (i = 0; i < n + 2; i++) t[i] = 0U;
    for (i = 0; i < n; i++) {
        uint64_t c = 0U;
        uint32_t mfac;
        for (j = 0; j < n; j++) {
            c += (uint64_t)t[j] + (uint64_t)a->v[j] * b->v[i];
            t[j] = (uint32_t)c;
            c >>= 32;
        }
        c += t[n];
        t[n] = (uint32_t)c;
        t[n + 1] = (uint32_t)(c >> 32);
        mfac = t[0] * ctx->m0inv;
        c = (uint64_t)t[0] + (uint64_t)mfac * ctx->m.v[0];
        c >>= 32;
        for (j = 1; j < n; j++) {
            c += (uint64_t)t[j] + (uint64_t)mfac * ctx->m.v[j];
            t[j - 1] = (uint32_t)c;
            c >>= 32;
        }
        c += t[n];
        t[n - 1] = (uint32_t)c;
        t[n] = t[n + 1] + (uint32_t)(c >> 32);
    }
    wf_bn_zero(&out);
    for (i = 0; i <= n && i < WF_BN_LIMBS; i++) out.v[i] = t[i];
    wf_bn_reduce_once(&out, &ctx->m);
    wf_bn_copy(r, &out);
}

void wf_mont_to(const wf_mont_t* ctx, wf_bn_t* r, const wf_bn_t* a) {
    wf_mont_mul(ctx, r, a, &ctx->r2);
}

void wf_mont_from(const wf_mont_t* ctx, wf_bn_t* r, const wf_bn_t* a) {
    wf_bn_t one;
    wf_bn_set_u32(&one, 1U);
    wf_mont_mul(ctx, r, a, &one);
}

void wf_mont_add(const wf_mont_t* ctx, wf_bn_t* r, const wf_bn_t* a, const wf_bn_t* b) {
    (void)wf_bn_add(r, a, b);
    wf_bn_reduce_once(r, &ctx->m);
}

void wf_mont_sub(const wf_mont_t* ctx, wf_bn_t* r, const wf_bn_t* a, const wf_bn_t* b) {
    if (wf_bn_sub(r, a, b)) (void)wf_bn_add(r, r, &ctx->m);
}

void wf_mont_exp(const wf_mont_t* ctx, wf_bn_t* r, const wf_bn_t* a, const wf_bn_t* e) {
    wf_bn_t base, acc, am;
    int i, top = wf_bn_bits(e);
    wf_bn_copy(&am, a);
    if (wf_bn_cmp(&am, &ctx->m) >= 0) wf_bn_mod(&am, &am, &ctx->m);
    wf_mont_to(ctx, &base, &am);
    wf_bn_copy(&acc, &ctx->one);
    for (i = top - 1; i >= 0; i--) {
        wf_mont_mul(ctx, &acc, &acc, &acc);
        if (wf_bn_bit(e, i)) wf_mont_mul(ctx, &acc, &acc, &base);
    }
    wf_mont_from(ctx, r, &acc);
}

void wf_mont_inv_prime(const wf_mont_t* ctx, wf_bn_t* r, const wf_bn_t* a) {
    wf_bn_t e, two;
    wf_bn_set_u32(&two, 2U);
    (void)wf_bn_sub(&e, &ctx->m, &two);
    wf_mont_exp(ctx, r, a, &e);
}
