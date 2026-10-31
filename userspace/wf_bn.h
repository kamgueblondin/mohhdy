/* wf_bn.h - Lot 67: bounded big numbers for the Ring 3 web stack.
 *
 * Fixed-size little-endian 32-bit limbs, no allocation. Up to 4096-bit
 * moduli (RSA) and the P-256 / P-384 fields. Modular arithmetic is done in
 * Montgomery form (CIOS). Freestanding, host testable. Not constant time:
 * it only verifies public signatures; the one secret it touches is never
 * passed here (the TLS key exchange uses X25519 from kernel/x25519.c). */
#ifndef WF_BN_H
#define WF_BN_H
#include <stdint.h>

#define WF_BN_MAX_BITS 4096
#define WF_BN_LIMBS (WF_BN_MAX_BITS / 32 + 2)

typedef struct {
    uint32_t v[WF_BN_LIMBS];
} wf_bn_t;

/* Montgomery context for an odd modulus of n limbs (n <= WF_BN_LIMBS - 2). */
typedef struct {
    wf_bn_t m;        /* modulus */
    wf_bn_t r2;       /* R^2 mod m, R = 2^(32*n) */
    wf_bn_t one;      /* R mod m (1 in Montgomery form) */
    uint32_t m0inv;   /* -m^-1 mod 2^32 */
    int n;            /* limbs in use */
    int bits;         /* bit length of m */
} wf_mont_t;

void wf_bn_zero(wf_bn_t* a);
void wf_bn_copy(wf_bn_t* d, const wf_bn_t* s);
void wf_bn_set_u32(wf_bn_t* a, uint32_t x);
/* Big-endian bytes -> number. Returns -1 if it does not fit. */
int wf_bn_from_bytes(wf_bn_t* a, const uint8_t* b, int len);
/* Number -> big-endian, exactly len bytes (left padded). -1 if too big. */
int wf_bn_to_bytes(const wf_bn_t* a, uint8_t* b, int len);
int wf_bn_cmp(const wf_bn_t* a, const wf_bn_t* b);
int wf_bn_is_zero(const wf_bn_t* a);
int wf_bn_bits(const wf_bn_t* a);
int wf_bn_bit(const wf_bn_t* a, int i);
/* r = a - b over all limbs, returns the borrow. */
uint32_t wf_bn_sub(wf_bn_t* r, const wf_bn_t* a, const wf_bn_t* b);
uint32_t wf_bn_add(wf_bn_t* r, const wf_bn_t* a, const wf_bn_t* b);
/* a mod m for a < 2*m (one conditional subtraction). */
void wf_bn_reduce_once(wf_bn_t* a, const wf_bn_t* m);
/* r = a mod m for any a below 2^(32*WF_BN_LIMBS) (shift and subtract). */
void wf_bn_mod(wf_bn_t* r, const wf_bn_t* a, const wf_bn_t* m);

/* 0 on success, -1 for an even or oversized modulus. */
int wf_mont_init(wf_mont_t* ctx, const wf_bn_t* m);
/* r = a*b*R^-1 mod m; a, b < m (a may be any value below R). */
void wf_mont_mul(const wf_mont_t* ctx, wf_bn_t* r, const wf_bn_t* a, const wf_bn_t* b);
void wf_mont_to(const wf_mont_t* ctx, wf_bn_t* r, const wf_bn_t* a);
void wf_mont_from(const wf_mont_t* ctx, wf_bn_t* r, const wf_bn_t* a);
/* Montgomery-form arithmetic modulo m. */
void wf_mont_add(const wf_mont_t* ctx, wf_bn_t* r, const wf_bn_t* a, const wf_bn_t* b);
void wf_mont_sub(const wf_mont_t* ctx, wf_bn_t* r, const wf_bn_t* a, const wf_bn_t* b);
/* r = a^e mod m, a and r in normal form. */
void wf_mont_exp(const wf_mont_t* ctx, wf_bn_t* r, const wf_bn_t* a, const wf_bn_t* e);
/* r = a^-1 mod m for a prime m (Fermat), normal form; a != 0 mod m. */
void wf_mont_inv_prime(const wf_mont_t* ctx, wf_bn_t* r, const wf_bn_t* a);
#endif
