/* wf_p256.h - Lot 72: constant-time P-256 scalar multiplication for ECDHE.
 * Fixed 8 x 32-bit limbs, Montgomery arithmetic with masked (branch-free)
 * reductions, complete projective addition formulas (Renes, Costello,
 * Batina 2016, a = -3) so no input is a special case, and a Montgomery
 * ladder with a masked conditional swap: the sequence of operations and
 * memory accesses does not depend on the secret scalar. The only branches
 * are on public data (point validation, the final infinity check). */
#ifndef WF_P256_H
#define WF_P256_H
#include <stdint.h>

/* out = k * P (P = 65-byte uncompressed point, or 0 for the base point).
 * 0 ok, -1 scalar not in [1, n-1], -2 point not on the curve, -3 result at
 * infinity. */
int wf_p256_mul(const uint8_t k[32], const uint8_t* point, int point_len, uint8_t out[65]);
/* Field multiplications done since the last reset (constant-time check in
 * the unit tests). */
uint32_t wf_p256_mul_count(void);
void wf_p256_mul_count_reset(void);
#endif
