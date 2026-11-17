/* wf_jsnum.h - Lot 113: IEEE 754 binary64 arithmetic in software for the
 * guest JavaScript subset (userspace/wf_js.c).
 *
 * Why software: the kernel does not save the x87/SSE state of Ring 3
 * tasks on a switch (no fxsave in kernel/), and the shell is built
 * without float flags, so the interpreter must not touch the FPU. A
 * number is the 64-bit pattern of a double (jf64). add, sub, mul, div,
 * rem (JS %), sqrt and the integer conversions are exact IEEE operations
 * with round to nearest, ties to even (the host test checks them bit for
 * bit against the host's SSE2 doubles). No 64-bit division is used (the
 * guest has no __udivdi3): quotients are long divisions.
 *
 * Decimal conversion: jf_parse is correctly rounded when the significand
 * has at most 19 digits, fits 53 bits and |exp10| <= 22 (exact fast
 * path); otherwise it goes through a 64-bit extended significand and is
 * within one unit in the last place. jf_format writes the shortest digit
 * string that parses back to the same number (ECMA-262 Number::toString)
 * by trying 1..17 digits. The math functions (exp, log, sin, cos, atan,
 * pow with a fractional exponent) are series in software: a few units in
 * the last place, not correctly rounded. */
#ifndef WF_JSNUM_H
#define WF_JSNUM_H

#include <stdint.h>

typedef uint64_t jf64;

#define JF_NAN      0x7FF8000000000000ULL
#define JF_INF      0x7FF0000000000000ULL
#define JF_NINF     0xFFF0000000000000ULL
#define JF_ZERO     0x0000000000000000ULL
#define JF_NZERO    0x8000000000000000ULL
#define JF_ONE      0x3FF0000000000000ULL
#define JF_PI       0x400921FB54442D18ULL
#define JF_E        0x4005BF0A8B145769ULL
#define JF_LN2      0x3FE62E42FEFA39EFULL
#define JF_LN10     0x40026BB1BBB55516ULL
#define JF_LOG2E    0x3FF71547652B82FEULL
#define JF_LOG10E   0x3FDBCB7B1526E50EULL
#define JF_SQRT2    0x3FF6A09E667F3BCDULL
#define JF_SQRT1_2  0x3FE6A09E667F3BCDULL
#define JF_MAXV     0x7FEFFFFFFFFFFFFFULL
#define JF_MINV     0x0000000000000001ULL
#define JF_EPS      0x3CB0000000000000ULL
#define JF_MAXSAFE  0x433FFFFFFFFFFFFFULL

int  jf_isnan(jf64 a);
int  jf_isinf(jf64 a);
int  jf_isfinite(jf64 a);
int  jf_iszero(jf64 a);
int  jf_signbit(jf64 a);
jf64 jf_neg(jf64 a);
jf64 jf_abs(jf64 a);

jf64 jf_from_i32(int32_t v);
jf64 jf_from_u32(uint32_t v);
jf64 jf_from_u64(uint64_t v);
jf64 jf_from_i64(int64_t v);
/* ECMA-262 ToInt32 / ToUint32 (modulo 2^32, NaN and infinities give 0) */
int32_t  jf_to_i32(jf64 a);
uint32_t jf_to_u32(jf64 a);
/* 1 when a is an integer with |a| < 2^63, its value in *out */
int  jf_to_i64(jf64 a, int64_t *out);
/* 1 when a is exactly an int32 (not -0) */
int  jf_is_i32(jf64 a, int32_t *out);
int  jf_is_integer(jf64 a);

jf64 jf_add(jf64 a, jf64 b);
jf64 jf_sub(jf64 a, jf64 b);
jf64 jf_mul(jf64 a, jf64 b);
jf64 jf_div(jf64 a, jf64 b);
jf64 jf_rem(jf64 a, jf64 b);     /* fmod: sign of a, truncated quotient */
jf64 jf_sqrt(jf64 a);

int  jf_eq(jf64 a, jf64 b);      /* IEEE: NaN != NaN, +0 == -0 */
int  jf_lt(jf64 a, jf64 b);
int  jf_le(jf64 a, jf64 b);
int  jf_same_value(jf64 a, jf64 b); /* Object.is */

jf64 jf_trunc(jf64 a);
jf64 jf_floor(jf64 a);
jf64 jf_ceil(jf64 a);
jf64 jf_round(jf64 a);           /* JS Math.round: halves toward +inf */
jf64 jf_ldexp(jf64 a, int k);    /* a * 2^k, rounded */

jf64 jf_exp(jf64 a);
jf64 jf_log(jf64 a);
jf64 jf_sin(jf64 a);
jf64 jf_cos(jf64 a);
jf64 jf_tan(jf64 a);
jf64 jf_atan(jf64 a);
jf64 jf_atan2(jf64 y, jf64 x);
jf64 jf_asin(jf64 a);
jf64 jf_acos(jf64 a);
jf64 jf_pow(jf64 x, jf64 y);
jf64 jf_cbrt(jf64 a);
jf64 jf_hypot(jf64 a, jf64 b);

/* Decimal text: digits, optional '.', optional exponent. Leading '+'/'-'
 * accepted. *used gets the bytes read (0 when no number). */
jf64 jf_parse(const char *s, int len, int *used);
/* Number::toString(10). Returns the length written (out NUL-terminated);
 * max >= 32 always fits. */
int  jf_format(jf64 v, char *out, int max);
/* Number.prototype.toFixed(digits), digits 0..20 */
int  jf_format_fixed(jf64 v, int digits, char *out, int max);
/* Number.prototype.toString(radix), radix 2..36 */
int  jf_format_radix(jf64 v, int radix, char *out, int max);
/* Number.prototype.toPrecision(p), p 1..21 */
int  jf_format_precision(jf64 v, int p, char *out, int max);
/* Number.prototype.toExponential(f), f 0..20, f < 0: as many as needed */
int  jf_format_exp(jf64 v, int f, char *out, int max);

#endif
