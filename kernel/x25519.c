#include "x25519.h"

#define X25519_LIMBS 8U
#define X25519_ELEMENTS 17U

/* Lot 48 (A2): field arithmetic mod p = 2^255 - 19 on 8 x 32-bit limbs.
 * The former generic path multiplied by 256 double-and-add modular
 * additions (bigint_mod_multiply_ct); a 8x8 schoolbook product with the
 * 2^256 = 38 (mod p) fold is about 60x faster. Every function runs the same
 * instruction sequence for any input (no secret-dependent branch or index),
 * the ladder and its conditional swaps are unchanged (RFC 7748). Values are
 * kept below 2^256 and made canonical (< p) only once, at the end. */
typedef uint32_t fe_t[X25519_LIMBS];

static void fe_fold(fe_t out, uint32_t carry) {
    uint16_t i; uint64_t x; uint32_t round;
    for (round = 0U; round < 2U; round++) {
        x = (uint64_t)out[0] + (uint64_t)carry * 38U;
        out[0] = (uint32_t)x; x >>= 32;
        for (i = 1U; i < X25519_LIMBS; i++) { x += out[i]; out[i] = (uint32_t)x; x >>= 32; }
        carry = (uint32_t)x;
    }
}
static void fe_add(fe_t out, const fe_t a, const fe_t b) {
    uint16_t i; uint64_t x = 0U;
    for (i = 0U; i < X25519_LIMBS; i++) { x += (uint64_t)a[i] + b[i]; out[i] = (uint32_t)x; x >>= 32; }
    fe_fold(out, (uint32_t)x);
}
static void fe_sub(fe_t out, const fe_t a, const fe_t b) {
    /* a - b wraps by 2^256 on borrow; 2^256 = 38 (mod p), so subtract 38
     * per borrow (twice, fixed). */
    uint16_t i; uint32_t borrow = 0U, round; uint64_t x;
    for (i = 0U; i < X25519_LIMBS; i++) {
        x = (uint64_t)a[i] - b[i] - borrow; out[i] = (uint32_t)x; borrow = (uint32_t)(x >> 63);
    }
    for (round = 0U; round < 2U; round++) {
        x = (uint64_t)out[0] - (uint64_t)borrow * 38U; out[0] = (uint32_t)x; borrow = (uint32_t)(x >> 63);
        for (i = 1U; i < X25519_LIMBS; i++) {
            x = (uint64_t)out[i] - borrow; out[i] = (uint32_t)x; borrow = (uint32_t)(x >> 63);
        }
    }
}
static void fe_mul(fe_t out, const fe_t a, const fe_t b) {
    uint32_t r[2U * X25519_LIMBS]; uint16_t i, j; uint64_t x;
    for (i = 0U; i < 2U * X25519_LIMBS; i++) r[i] = 0U;
    for (i = 0U; i < X25519_LIMBS; i++) {
        x = 0U;
        for (j = 0U; j < X25519_LIMBS; j++) {
            x += (uint64_t)a[i] * b[j] + r[i + j]; r[i + j] = (uint32_t)x; x >>= 32;
        }
        r[i + X25519_LIMBS] = (uint32_t)x;
    }
    x = 0U;
    for (i = 0U; i < X25519_LIMBS; i++) {
        x += (uint64_t)r[i] + (uint64_t)r[i + X25519_LIMBS] * 38U; out[i] = (uint32_t)x; x >>= 32;
    }
    fe_fold(out, (uint32_t)x);
}
static void fe_canon(fe_t v) {
    /* v < 2^256 < 3p: subtract p when v >= p, twice, by mask. */
    static const uint32_t p[X25519_LIMBS] = {0xffffffedU, 0xffffffffU, 0xffffffffU, 0xffffffffU,
                                             0xffffffffU, 0xffffffffU, 0xffffffffU, 0x7fffffffU};
    uint32_t t[X25519_LIMBS], borrow, mask, round; uint16_t i; uint64_t x;
    for (round = 0U; round < 2U; round++) {
        borrow = 0U;
        for (i = 0U; i < X25519_LIMBS; i++) {
            x = (uint64_t)v[i] - p[i] - borrow; t[i] = (uint32_t)x; borrow = (uint32_t)(x >> 63);
        }
        mask = borrow - 1U;   /* all ones when no borrow, i.e. v >= p */
        for (i = 0U; i < X25519_LIMBS; i++) v[i] = (t[i] & mask) | (v[i] & ~mask);
    }
}
static void fe_swap(fe_t a, fe_t b, uint32_t swap) {
    uint16_t i; uint32_t mask = 0U - swap, d;
    for (i = 0U; i < X25519_LIMBS; i++) { d = mask & (a[i] ^ b[i]); a[i] ^= d; b[i] ^= d; }
}
static void fe_invert(fe_t out, const fe_t z, fe_t result) {
    /* z^(p-2), p - 2 = 2^255 - 21: public fixed exponent, square-and-multiply. */
    uint16_t bit, i;
    for (i = 0U; i < X25519_LIMBS; i++) result[i] = 0U;
    result[0] = 1U;
    for (bit = 256U; bit > 0U; bit--) {
        uint16_t b = (uint16_t)(bit - 1U);
        fe_mul(result, result, result);
        if ((b >= 5U && b <= 254U) || b == 3U || b == 1U || b == 0U) fe_mul(result, result, z);
    }
    for (i = 0U; i < X25519_LIMBS; i++) out[i] = result[i];
}

int x25519_scalar_mult(uint8_t output[X25519_KEY_LENGTH],const uint8_t scalar[X25519_KEY_LENGTH],const uint8_t u[X25519_KEY_LENGTH],uint32_t* workspace,uint16_t workspace_length){
    uint8_t k[X25519_KEY_LENGTH];
    uint16_t i, bit; uint32_t swap = 0U;
    uint32_t *x1, *x2, *z2, *x3, *z3, *a, *aa, *b, *bb, *e, *c, *d, *da, *cb, *t0, *t1;
    static const fe_t a24 = {121665U, 0U, 0U, 0U, 0U, 0U, 0U, 0U};
    if(!output||!scalar||!u||!workspace||workspace_length<X25519_WORKSPACE_LIMBS)return -1;
    x1 = workspace + 1U * X25519_LIMBS; x2 = workspace + 2U * X25519_LIMBS; z2 = workspace + 3U * X25519_LIMBS;
    x3 = workspace + 4U * X25519_LIMBS; z3 = workspace + 5U * X25519_LIMBS; a = workspace + 6U * X25519_LIMBS;
    aa = workspace + 7U * X25519_LIMBS; b = workspace + 8U * X25519_LIMBS; bb = workspace + 9U * X25519_LIMBS;
    e = workspace + 10U * X25519_LIMBS; c = workspace + 11U * X25519_LIMBS; d = workspace + 12U * X25519_LIMBS;
    da = workspace + 13U * X25519_LIMBS; cb = workspace + 14U * X25519_LIMBS; t0 = workspace + 15U * X25519_LIMBS;
    t1 = workspace + 16U * X25519_LIMBS;
    for(i=0U;i<X25519_KEY_LENGTH;i++)k[i]=scalar[i];
    k[0]&=248U;k[31]&=127U;k[31]|=64U;
    for (i = 0U; i < X25519_LIMBS; i++) {
        x1[i] = (uint32_t)u[4U * i] | ((uint32_t)u[4U * i + 1U] << 8) | ((uint32_t)u[4U * i + 2U] << 16) |
                ((uint32_t)u[4U * i + 3U] << 24);
        x2[i] = 0U; z2[i] = 0U; z3[i] = 0U;
    }
    x1[7] &= 0x7fffffffU;   /* RFC 7748: mask the top bit of u */
    x2[0] = 1U; z3[0] = 1U;
    for (i = 0U; i < X25519_LIMBS; i++) x3[i] = x1[i];
    for(bit=255U;bit>0U;bit--){
        uint32_t current=(uint32_t)((k[(bit-1U)/8U]>>((bit-1U)&7U))&1U);
        swap ^= current; fe_swap(x2, x3, swap); fe_swap(z2, z3, swap); swap = current;
        fe_add(a, x2, z2); fe_mul(aa, a, a); fe_sub(b, x2, z2); fe_mul(bb, b, b); fe_sub(e, aa, bb);
        fe_add(c, x3, z3); fe_sub(d, x3, z3); fe_mul(da, d, a); fe_mul(cb, c, b);
        fe_add(t0, da, cb); fe_mul(x3, t0, t0); fe_sub(t0, da, cb); fe_mul(t1, t0, t0); fe_mul(z3, x1, t1);
        fe_mul(x2, aa, bb); fe_mul(t0, e, a24); fe_add(t0, aa, t0); fe_mul(z2, e, t0);
    }
    fe_swap(x2, x3, swap); fe_swap(z2, z3, swap);
    fe_invert(t0, z2, t1); fe_mul(x2, x2, t0); fe_canon(x2);
    for (i = 0U; i < X25519_LIMBS; i++) {
        output[4U * i] = (uint8_t)x2[i]; output[4U * i + 1U] = (uint8_t)(x2[i] >> 8);
        output[4U * i + 2U] = (uint8_t)(x2[i] >> 16); output[4U * i + 3U] = (uint8_t)(x2[i] >> 24);
    }
    for (i = 0U; i < X25519_KEY_LENGTH; i++) k[i] = 0U;
    return 0;
}

int x25519_public_key(uint8_t output[X25519_KEY_LENGTH],const uint8_t private_key[X25519_KEY_LENGTH],uint32_t* workspace,uint16_t workspace_length){
    uint8_t base[X25519_KEY_LENGTH]={9U};
    return x25519_scalar_mult(output,private_key,base,workspace,workspace_length);
}

int x25519_shared_secret(uint8_t output[X25519_KEY_LENGTH],const uint8_t private_key[X25519_KEY_LENGTH],const uint8_t peer_public[X25519_KEY_LENGTH],uint32_t* workspace,uint16_t workspace_length){
    uint8_t difference=0U;uint16_t i;int status=x25519_scalar_mult(output,private_key,peer_public,workspace,workspace_length);
    if(status!=0)return status;
    for(i=0U;i<X25519_KEY_LENGTH;i++)difference|=output[i];
    return difference==0U?-9:0;
}
