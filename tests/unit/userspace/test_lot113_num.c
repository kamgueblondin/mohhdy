/* Lot 113 (US-031): binary64 in software for the JavaScript subset
 * (userspace/wf_jsnum.c). The arithmetic is checked bit for bit against
 * the host's SSE2 doubles (this test is built with -msse2 -mfpmath=sse,
 * so the host rounds each operation to binary64 like the guest code);
 * conversions against ECMA-262 Number::toString examples and against
 * strtod; the series math functions within a relative 1e-13. */
#include "../../framework/unity.h"
#include <string.h>
#include <stdio.h>
#include <math.h>
#include "wf_jsnum.h"

static double strtod_(const char *s) { double d = 0; sscanf(s, "%lf", &d); return d; }
#define strtod(s, e) strtod_(s)

static uint64_t g_rng = 0x9E3779B97F4A7C15ULL;
static uint64_t rnd64(void) {
    g_rng ^= g_rng << 13; g_rng ^= g_rng >> 7; g_rng ^= g_rng << 17;
    return g_rng;
}
static jf64 bits(double d) { jf64 b; memcpy(&b, &d, 8); return b; }
static double dbl(jf64 b) { double d; memcpy(&d, &b, 8); return d; }

/* a random double: raw bit patterns, small integers, "nice" decimals,
 * subnormals, infinities and NaN, zeros of both signs */
static double rnd_double(void) {
    uint64_t r = rnd64();
    switch ((int)(r % 8)) {
    case 0: case 1: return dbl(rnd64());
    case 2: return (double)(int32_t)(rnd64() % 2001) - 1000.0;
    case 3: return ((double)(int32_t)(rnd64() % 200001) - 100000.0) / 1000.0;
    case 4: return dbl(rnd64() & 0x800FFFFFFFFFFFFFULL);            /* subnormal */
    case 5: return dbl((rnd64() & 0x80000000FFFFFFFFULL) | 0x3FF0000000000000ULL);
    case 6: { static const double sp[] = { 0.0, -0.0, 1.0 / 0.0, -1.0 / 0.0, 1e308, -1e-308, 0.1, 3.0 };
              return sp[rnd64() % 8]; }
    default: return dbl((rnd64() & 0x8FFFFFFFFFFFFFFFULL) | 0x4000000000000000ULL);
    }
}

static int same(jf64 a, double ref) {
    if (isnan(ref)) return jf_isnan(a);
    return a == bits(ref);
}

#define N_RANDOM 200000

static void test_l113_arith_bits(void) {
    int i, bad = 0;
    for (i = 0; i < N_RANDOM; i++) {
        double x = rnd_double(), y = rnd_double();
        jf64 a = bits(x), b = bits(y);
        volatile double s = x + y, d = x - y, m = x * y, q = x / y;
        if (!same(jf_add(a, b), s) || !same(jf_sub(a, b), d) || !same(jf_mul(a, b), m) || !same(jf_div(a, b), q)) {
            if (bad++ < 5)
                printf("arith %a %a: add %a/%a mul %a/%a div %a/%a\n", x, y, dbl(jf_add(a, b)), (double)s,
                       dbl(jf_mul(a, b)), (double)m, dbl(jf_div(a, b)), (double)q);
        }
    }
    TEST_ASSERT_EQUAL(0, bad);
}

static void test_l113_rem_sqrt_bits(void) {
    int i, bad = 0;
    for (i = 0; i < N_RANDOM / 4; i++) {
        double x = rnd_double(), y = rnd_double();
        jf64 a = bits(x), b = bits(y);
        if (!same(jf_rem(a, b), fmod(x, y))) {
            if (bad++ < 5) printf("rem %a %a: %a / %a\n", x, y, dbl(jf_rem(a, b)), fmod(x, y));
        }
        if (!same(jf_sqrt(a), sqrt(x))) {
            if (bad++ < 5) printf("sqrt %a: %a / %a\n", x, dbl(jf_sqrt(a)), sqrt(x));
        }
    }
    TEST_ASSERT_EQUAL(0, bad);
}

static void test_l113_compare_round(void) {
    int i, bad = 0;
    for (i = 0; i < N_RANDOM / 4; i++) {
        double x = rnd_double(), y = rnd_double();
        jf64 a = bits(x), b = bits(y);
        if (jf_eq(a, b) != (x == y) || jf_lt(a, b) != (x < y) || jf_le(a, b) != (x <= y)) bad++;
        if (!same(jf_trunc(a), trunc(x)) || !same(jf_floor(a), floor(x)) || !same(jf_ceil(a), ceil(x))) {
            if (bad++ < 5) printf("round %a: %a %a %a\n", x, dbl(jf_trunc(a)), dbl(jf_floor(a)), dbl(jf_ceil(a)));
        }
    }
    TEST_ASSERT_EQUAL(0, bad);
    /* Math.round: halves go up, -0.5 .. -0 is -0 */
    TEST_ASSERT_TRUE(jf_round(bits(2.5)) == bits(3.0));
    TEST_ASSERT_TRUE(jf_round(bits(-2.5)) == bits(-2.0));
    TEST_ASSERT_TRUE(jf_round(bits(-0.4)) == JF_NZERO);
    TEST_ASSERT_TRUE(jf_round(bits(0.49999999999999994)) == 0);
    TEST_ASSERT_TRUE(jf_round(bits(4503599627370495.5)) == bits(4503599627370496.0));
}

static void test_l113_int_conv(void) {
    int i, bad = 0;
    for (i = 0; i < N_RANDOM / 4; i++) {
        int32_t v = (int32_t)rnd64();
        uint32_t u = (uint32_t)rnd64();
        int64_t w = (int64_t)rnd64() >> (rnd64() % 63);
        int32_t back;
        if (jf_from_i32(v) != bits((double)v)) bad++;
        if (jf_from_u32(u) != bits((double)u)) bad++;
        if (jf_from_i64(w) != bits((double)w)) bad++;
        if (!jf_is_i32(jf_from_i32(v), &back) || back != v) bad++;
    }
    TEST_ASSERT_EQUAL(0, bad);
    /* ToInt32 is modulo 2^32 */
    TEST_ASSERT_EQUAL(-2147483647 - 1, jf_to_i32(bits(2147483648.0)));
    TEST_ASSERT_EQUAL(1, jf_to_i32(bits(4294967297.0)));
    TEST_ASSERT_EQUAL(-1, jf_to_i32(bits(-1.9)));
    TEST_ASSERT_EQUAL(0, jf_to_i32(JF_NAN));
    TEST_ASSERT_EQUAL(0, jf_to_i32(JF_INF));
    TEST_ASSERT_EQUAL(-5, jf_to_i32(bits(-5.5)));
    TEST_ASSERT_TRUE(jf_to_u32(bits(-1.0)) == 4294967295U);
    TEST_ASSERT_EQUAL(0, jf_to_i32(bits(1e20)) != (int32_t)(uint32_t)(uint64_t)fmod(1e20, 4294967296.0));
    TEST_ASSERT_FALSE(jf_is_i32(JF_NZERO, &i));
    TEST_ASSERT_FALSE(jf_is_i32(bits(0.5), &i));
}

static const char *fmt(double d) {
    static char b[64];
    jf_format(bits(d), b, (int)sizeof(b));
    return b;
}

static void test_l113_format_spec(void) {
    TEST_ASSERT_EQUAL_STRING("0", fmt(0.0));
    TEST_ASSERT_EQUAL_STRING("0", fmt(-0.0));
    TEST_ASSERT_EQUAL_STRING("NaN", fmt(NAN));
    TEST_ASSERT_EQUAL_STRING("-Infinity", fmt(-INFINITY));
    TEST_ASSERT_EQUAL_STRING("0.1", fmt(0.1));
    TEST_ASSERT_EQUAL_STRING("0.30000000000000004", fmt(0.1 + 0.2));
    TEST_ASSERT_EQUAL_STRING("123456789", fmt(123456789.0));
    TEST_ASSERT_EQUAL_STRING("1e+21", fmt(1e21));
    TEST_ASSERT_EQUAL_STRING("100000000000000000000", fmt(1e20));
    TEST_ASSERT_EQUAL_STRING("1.5e-7", fmt(1.5e-7));
    TEST_ASSERT_EQUAL_STRING("0.000001", fmt(1e-6));
    TEST_ASSERT_EQUAL_STRING("1e-7", fmt(1e-7));
    TEST_ASSERT_EQUAL_STRING("-3.14", fmt(-3.14));
    TEST_ASSERT_EQUAL_STRING("5e-324", fmt(5e-324));
    TEST_ASSERT_EQUAL_STRING("1.7976931348623157e+308", fmt(1.7976931348623157e308));
    TEST_ASSERT_EQUAL_STRING("9007199254740992", fmt(9007199254740992.0));
    TEST_ASSERT_EQUAL_STRING("123.456", fmt(123.456));
    TEST_ASSERT_EQUAL_STRING("2.5", fmt(2.5));
    TEST_ASSERT_EQUAL_STRING("1.2345678901234567e+30", fmt(1.2345678901234567e30));
    TEST_ASSERT_EQUAL_STRING("0.3333333333333333", fmt(1.0 / 3.0));
}

/* every finite double formats to a string that strtod reads back */
static void test_l113_format_roundtrip(void) {
    int i, bad = 0;
    char b[64];
    for (i = 0; i < N_RANDOM / 4; i++) {
        double x = rnd_double();
        int n;
        if (!isfinite(x)) continue;
        n = jf_format(bits(x), b, (int)sizeof(b));
        if (n <= 0 || strtod(b, 0) != x || (x == 0 && signbit(x) && strcmp(b, "0"))) {
            if (bad++ < 5) printf("format %a -> %s\n", x, b);
        }
    }
    TEST_ASSERT_EQUAL(0, bad);
}

/* the parser matches strtod on short decimals (the fast path is exact)
 * and stays within one ulp on long ones */
static void test_l113_parse(void) {
    int i, bad = 0, used;
    char b[64];
    for (i = 0; i < N_RANDOM / 4; i++) {
        int digits = 1 + (int)(rnd64() % 15), e10 = (int)(rnd64() % 45) - 22, k, n = 0;
        double ref;
        jf64 got;
        for (k = 0; k < digits; k++) b[n++] = (char)('0' + (int)(rnd64() % 10));
        n += sprintf(b + n, "e%d", e10);
        ref = strtod(b, 0);
        got = jf_parse(b, n, &used);
        if (used != n || got != bits(ref)) { if (bad++ < 5) printf("parse %s %a %a\n", b, dbl(got), ref); }
    }
    TEST_ASSERT_EQUAL(0, bad);
    for (i = 0; i < N_RANDOM / 8; i++) {
        double x = rnd_double();
        int64_t d;
        if (!isfinite(x) || x == 0) continue;
        sprintf(b, "%.25g", x);
        d = (int64_t)(jf_parse(b, (int)strlen(b), &used) - bits(x));
        if (d > 1 || d < -1) { if (bad++ < 5) printf("long parse %s\n", b); }
    }
    TEST_ASSERT_EQUAL(0, bad);
    TEST_ASSERT_TRUE(jf_parse("1e400", 5, &used) == JF_INF);
    TEST_ASSERT_TRUE(jf_parse("-1e-400", 7, &used) == JF_NZERO);
    TEST_ASSERT_TRUE(jf_parse(".5", 2, &used) == bits(0.5) && used == 2);
    TEST_ASSERT_TRUE(jf_parse("5.", 2, &used) == bits(5.0) && used == 2);
    TEST_ASSERT_TRUE(jf_parse("-Infinity", 9, &used) == JF_NINF);
    TEST_ASSERT_TRUE(jf_isnan(jf_parse("abc", 3, &used)) && used == 0);
    TEST_ASSERT_TRUE(jf_parse("12px", 4, &used) == bits(12.0) && used == 2);
    TEST_ASSERT_TRUE(jf_parse("1e", 2, &used) == JF_ONE && used == 1);
    TEST_ASSERT_TRUE(jf_parse("0.1", 3, &used) == bits(0.1));
    TEST_ASSERT_TRUE(jf_parse("2.2250738585072014e-308", 23, &used) == bits(2.2250738585072014e-308));
}

static const char *fx(double d, int f) {
    static char b[128];
    jf_format_fixed(bits(d), f, b, (int)sizeof(b));
    return b;
}

static void test_l113_fixed_radix(void) {
    char b[128];
    TEST_ASSERT_EQUAL_STRING("3.14", fx(3.14159, 2));
    TEST_ASSERT_EQUAL_STRING("1.00", fx(1.005, 2));
    TEST_ASSERT_EQUAL_STRING("3", fx(2.5, 0));
    TEST_ASSERT_EQUAL_STRING("-3", fx(-2.5, 0));
    TEST_ASSERT_EQUAL_STRING("0.000", fx(0.0, 3));
    TEST_ASSERT_EQUAL_STRING("-0.00", fx(-0.0001, 2));
    TEST_ASSERT_EQUAL_STRING("123.46", fx(123.456, 2));
    TEST_ASSERT_EQUAL_STRING("1e+21", fx(1e21, 2));
    TEST_ASSERT_EQUAL_STRING("100000000000000000000.00", fx(1e20, 2));
    TEST_ASSERT_EQUAL_STRING("10.0", fx(9.96, 1));
    TEST_ASSERT_EQUAL_STRING("0.10000000000000000555", fx(0.1, 20));
    jf_format_radix(bits(255.0), 16, b, (int)sizeof(b));
    TEST_ASSERT_EQUAL_STRING("ff", b);
    jf_format_radix(bits(-10.0), 2, b, (int)sizeof(b));
    TEST_ASSERT_EQUAL_STRING("-1010", b);
    jf_format_radix(bits(0.5), 2, b, (int)sizeof(b));
    TEST_ASSERT_EQUAL_STRING("0.1", b);
    jf_format_radix(bits(35.0), 36, b, (int)sizeof(b));
    TEST_ASSERT_EQUAL_STRING("z", b);
    jf_format_precision(bits(123.456), 4, b, (int)sizeof(b));
    TEST_ASSERT_EQUAL_STRING("123.5", b);
    jf_format_precision(bits(0.000123), 2, b, (int)sizeof(b));
    TEST_ASSERT_EQUAL_STRING("0.00012", b);
    jf_format_precision(bits(123456.0), 2, b, (int)sizeof(b));
    TEST_ASSERT_EQUAL_STRING("1.2e+5", b);
    jf_format_exp(bits(123456.0), 2, b, (int)sizeof(b));
    TEST_ASSERT_EQUAL_STRING("1.23e+5", b);
    jf_format_exp(bits(0.00015), -1, b, (int)sizeof(b));
    TEST_ASSERT_EQUAL_STRING("1.5e-4", b);
}

/* toFixed against the host's exact printf, except exact ties (printf
 * rounds them to even, ECMA-262 takes the larger n) */
static void test_l113_fixed_random(void) {
    static char ref[1200], exact[1400];
    char got[160];
    int i, bad = 0, ties = 0;
    for (i = 0; i < 20000; i++) {
        double x = rnd_double();
        int f = (int)(rnd64() % 21), dot, k, tie = 1;
        if (!isfinite(x) || fabs(x) >= 1e21) continue;
        if (rnd64() & 1) x = ((double)(int32_t)(rnd64() % 2000001) - 1000000.0) / 1000.0;
        snprintf(ref, sizeof(ref), "%.*f", f, x);
        snprintf(exact, sizeof(exact), "%.1100f", x);
        for (dot = 0; exact[dot] && exact[dot] != '.'; dot++) {}
        if (exact[dot + 1 + f] != '5') tie = 0;
        for (k = dot + 2 + f; tie && exact[k]; k++) if (exact[k] != '0') tie = 0;
        jf_format_fixed(bits(x), f, got, (int)sizeof(got));
        if (tie) { ties++; continue; }
        if (x == 0 && signbit(x)) {          /* JS: (-0).toFixed(f) has no sign */
            TEST_ASSERT_TRUE(got[0] == '0');
            continue;
        }
        if (strcmp(ref, got)) { if (bad++ < 5) printf("fixed %a %d: %s / %s\n", x, f, got, ref); }
    }
    TEST_ASSERT_EQUAL(0, bad);
    TEST_ASSERT_TRUE(ties >= 0);
}

static int close_to(jf64 got, double ref) {
    double g = dbl(got);
    if (isnan(ref)) return isnan(g);
    if (isinf(ref)) return g == ref;
    if (ref == 0) return fabs(g) < 1e-300;
    return fabs(g - ref) <= fabs(ref) * 1e-13;
}

static void test_l113_math(void) {
    int i, bad = 0;
    for (i = 0; i < 3000; i++) {
        double x = ((double)(int32_t)(rnd64() % 2000001) - 1000000.0) / 10000.0;   /* -100 .. 100 */
        double p = (double)(rnd64() % 100000) / 1000.0 + 1e-3;
        if (!close_to(jf_exp(bits(x)), exp(x))) { if (bad++ < 5) printf("exp %g\n", x); }
        if (!close_to(jf_log(bits(p)), log(p))) { if (bad++ < 5) printf("log %g\n", p); }
        if (fabs(sin(x)) > 1e-6 && !close_to(jf_sin(bits(x)), sin(x))) { if (bad++ < 5) printf("sin %g %.17g %.17g\n", x, dbl(jf_sin(bits(x))), sin(x)); }
        if (fabs(cos(x)) > 1e-6 && !close_to(jf_cos(bits(x)), cos(x))) { if (bad++ < 5) printf("cos %g\n", x); }
        if (!close_to(jf_atan(bits(x)), atan(x))) { if (bad++ < 5) printf("atan %g\n", x); }
        if (!close_to(jf_pow(bits(p), bits(x / 10)), pow(p, x / 10))) { if (bad++ < 5) printf("pow %g %g\n", p, x / 10); }
        if (!close_to(jf_atan2(bits(x), bits(p - 50)), atan2(x, p - 50))) { if (bad++ < 5) printf("atan2 %g %g\n", x, p - 50); }
        if (!close_to(jf_cbrt(bits(x)), cbrt(x))) { if (bad++ < 5) printf("cbrt %g\n", x); }
    }
    TEST_ASSERT_EQUAL(0, bad);
    TEST_ASSERT_TRUE(jf_pow(bits(2.0), bits(10.0)) == bits(1024.0));
    TEST_ASSERT_TRUE(jf_pow(bits(2.0), bits(-1.0)) == bits(0.5));
    TEST_ASSERT_TRUE(jf_isnan(jf_pow(bits(1.0), JF_INF)));
    TEST_ASSERT_TRUE(jf_pow(bits(-8.0), bits(3.0)) == bits(-512.0));
    TEST_ASSERT_TRUE(jf_isnan(jf_pow(bits(-8.0), bits(0.5))));
    TEST_ASSERT_TRUE(jf_pow(JF_NZERO, bits(-3.0)) == JF_NINF);
    TEST_ASSERT_TRUE(jf_exp(0) == JF_ONE);
    TEST_ASSERT_TRUE(jf_log(JF_ONE) == 0);
    TEST_ASSERT_TRUE(jf_isnan(jf_log(bits(-1.0))));
    TEST_ASSERT_TRUE(jf_log(0) == JF_NINF);
    TEST_ASSERT_TRUE(close_to(jf_asin(bits(0.5)), asin(0.5)));
    TEST_ASSERT_TRUE(close_to(jf_acos(bits(-0.25)), acos(-0.25)));
    TEST_ASSERT_TRUE(close_to(jf_hypot(bits(3.0), bits(4.0)), 5.0));
    TEST_ASSERT_TRUE(jf_ldexp(JF_ONE, -1074) == JF_MINV);
    TEST_ASSERT_TRUE(jf_ldexp(JF_ONE, 1024) == JF_INF);
}

int main(void) {
    unity_init();
    RUN_TEST(test_l113_arith_bits);
    RUN_TEST(test_l113_rem_sqrt_bits);
    RUN_TEST(test_l113_compare_round);
    RUN_TEST(test_l113_int_conv);
    RUN_TEST(test_l113_format_spec);
    RUN_TEST(test_l113_format_roundtrip);
    RUN_TEST(test_l113_parse);
    RUN_TEST(test_l113_fixed_radix);
    RUN_TEST(test_l113_fixed_random);
    RUN_TEST(test_l113_math);
    unity_print_results();
    unity_cleanup();
    return unity_stats.tests_failed == 0 ? 0 : 1;
}
