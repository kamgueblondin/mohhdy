/* Lot 93 (US-031): the math functions the reader now simplifies, in
 * wf_cssprop.c: sin(), cos(), tan(), asin(), acos(), atan(), atan2(),
 * pow(), sqrt(), hypot(), log(), exp() and the constants e, pi,
 * infinity, -infinity and nan. Hand-picked edge cases next to the
 * generated suites (test_lot93_trig.c, test_lot93_powlog.c,
 * test_lot93_mtype.c, test_lot93_nested.c, test_lot93_compute.c,
 * test_lot93_propsheet.c). Expected values come from
 * scripts/lot93_mathmodel.py. */
#include "../../framework/unity.h"
#include <string.h>
#include <stdio.h>
#include "wf_sitecss.h"
#include "wf_cssprop.h"

static wf_site_t g_d;
static char g_html[8192], g_o[64], g_t[256];
static wf_cssprop_t g_p;

static const char *ev(const char *v) {
    int t, r = wf_cssprop_math_eval(v, &t, g_o, (int)sizeof(g_o));
    return r == 1 ? g_o : r == 0 ? "KEEP" : "BAD";
}

static const char *cv(const char *syn, const char *v) {
    memset(&g_p, 0, sizeof(g_p));
    g_p.syntax = (unsigned char)wf_cssprop_syntax_parse(syn, (int)strlen(syn), &g_p);
    return wf_cssprop_compute(&g_p, v, g_o, (int)sizeof(g_o)) ? g_o : "KEEP";
}

static const char *nv(const char *id, const char *name) {
    int i;
    for (i = 0; i < g_d.nnodes; i++)
        if (!strcmp(g_d.node[i].id, id)) { const char *v = wf_site_node_var(&g_d, i, name); return v ? v : "(none)"; }
    return "(no node)";
}

static void test_constants(void) {
    TEST_ASSERT_EQUAL_STRING("3.1416", ev("calc(pi)"));
    TEST_ASSERT_EQUAL_STRING("2.7183", ev("calc(E)"));
    TEST_ASSERT_EQUAL_STRING("3141.6", ev("calc(pi * 1000)"));      /* the constant is kept in 1/10000 */
    TEST_ASSERT_EQUAL_STRING("6.2832", ev("calc(2*pi)"));
    TEST_ASSERT_EQUAL_STRING("7.3892", ev("calc(e * e)"));
    TEST_ASSERT_EQUAL_STRING("2.1416", ev("calc(pi - 1)"));
    TEST_ASSERT_EQUAL_STRING("31.416px", ev("calc(pi * 10px)"));
    TEST_ASSERT_EQUAL_STRING("KEEP", ev("calc(infinity)"));
    TEST_ASSERT_EQUAL_STRING("KEEP", ev("calc(-INFINITY * 1px)"));
    TEST_ASSERT_EQUAL_STRING("KEEP", ev("calc(NaN)"));
    TEST_ASSERT_EQUAL_STRING("KEEP", ev("sign(infinity)"));
    TEST_ASSERT_EQUAL_STRING("KEEP", ev("min(1, nan)"));
    TEST_ASSERT_EQUAL(WF_CSSPROP_NUMBER, wf_cssprop_math_type("calc(-infinity)"));
    TEST_ASSERT_EQUAL(WF_CSSPROP_LENGTH, wf_cssprop_math_type("calc(infinity * 1px)"));
    TEST_ASSERT_EQUAL_STRING("BAD", ev("calc(-pi)"));
    TEST_ASSERT_EQUAL_STRING("BAD", ev("calc(pi-1)"));             /* one identifier */
    TEST_ASSERT_EQUAL_STRING("BAD", ev("calc(2pi)"));
    TEST_ASSERT_EQUAL_STRING("BAD", ev("calc(pie)"));
    TEST_ASSERT_EQUAL_STRING("BAD", ev("calc(- infinity)"));
    TEST_ASSERT_EQUAL_STRING("BAD", ev("calc(pi + 1px)"));
    TEST_ASSERT_EQUAL_STRING("BAD", ev("pi"));                     /* not a math function */
}

/* sin(): angles reduced exactly (exact values on the axes and at 30deg), radians up to 100 */
static void test_sin(void) {
    TEST_ASSERT_EQUAL_STRING("0", ev("sin(0)"));
    TEST_ASSERT_EQUAL_STRING("0.5", ev("sin(30deg)"));
    TEST_ASSERT_EQUAL_STRING("-0.5", ev("sin(-30deg)"));
    TEST_ASSERT_EQUAL_STRING("1", ev("sin(90deg)"));
    TEST_ASSERT_EQUAL_STRING("0", ev("sin(180deg)"));
    TEST_ASSERT_EQUAL_STRING("-1", ev("sin(270deg)"));
    TEST_ASSERT_EQUAL_STRING("0.5", ev("sin(390deg)"));
    TEST_ASSERT_EQUAL_STRING("0.5", ev("sin(-330deg)"));
    TEST_ASSERT_EQUAL_STRING("0", ev("sin(1turn)"));
    TEST_ASSERT_EQUAL_STRING("0.8415", ev("sin(1)"));
    TEST_ASSERT_EQUAL_STRING("-0.5064", ev("sin(100)"));
    TEST_ASSERT_EQUAL_STRING("0.5064", ev("sin(-100)"));
    TEST_ASSERT_EQUAL_STRING("KEEP", ev("sin(100.0001)"));
    TEST_ASSERT_EQUAL_STRING("KEEP", ev("sin(-100.0001)"));
    /* reduced angles near pi/4, where a short series would show */
    TEST_ASSERT_EQUAL_STRING("0.7548", ev("sin(49.0036deg)"));
    TEST_ASSERT_EQUAL_STRING("-0.7881", ev("sin(-52.0037deg)"));
    TEST_ASSERT_EQUAL_STRING("-0.8479", ev("sin(237.9786deg)"));
    TEST_ASSERT_EQUAL_STRING("1.0984", ev("tan(47.6835deg)"));
    TEST_ASSERT_EQUAL_STRING("0.9529", ev("tan(43.6199deg)"));
    TEST_ASSERT_EQUAL_STRING("-0.9374", ev("tan(136.8492deg)"));
}

/* cos() */
static void test_cos(void) {
    TEST_ASSERT_EQUAL_STRING("1", ev("cos(0)"));
    TEST_ASSERT_EQUAL_STRING("0.5", ev("cos(60deg)"));
    TEST_ASSERT_EQUAL_STRING("0", ev("cos(90deg)"));
    TEST_ASSERT_EQUAL_STRING("-1", ev("cos(180deg)"));
    TEST_ASSERT_EQUAL_STRING("-1", ev("cos(pi)"));
    TEST_ASSERT_EQUAL_STRING("0.5403", ev("cos(1)"));
    TEST_ASSERT_EQUAL_STRING("0.5403", ev("cos(-1)"));
    TEST_ASSERT_EQUAL_STRING("-1", ev("cos(200grad)"));
    TEST_ASSERT_EQUAL_STRING("0.8623", ev("cos(-100)"));
}

/* tan(): the poles at odd multiples of 90deg are infinite (kept), values past 100000 kept */
static void test_tan(void) {
    TEST_ASSERT_EQUAL_STRING("1", ev("tan(45deg)"));
    TEST_ASSERT_EQUAL_STRING("-1", ev("tan(-45deg)"));
    TEST_ASSERT_EQUAL_STRING("-1", ev("tan(135deg)"));
    TEST_ASSERT_EQUAL_STRING("KEEP", ev("tan(90deg)"));
    TEST_ASSERT_EQUAL_STRING("KEEP", ev("tan(-90deg)"));
    TEST_ASSERT_EQUAL_STRING("KEEP", ev("tan(270deg)"));
    TEST_ASSERT_EQUAL_STRING("KEEP", ev("tan(100grad)"));
    TEST_ASSERT_EQUAL_STRING("0", ev("tan(180deg)"));
    TEST_ASSERT_EQUAL_STRING("572.9572", ev("tan(89.9deg)"));
    TEST_ASSERT_EQUAL_STRING("-572.9572", ev("tan(90.1deg)"));
    TEST_ASSERT_EQUAL_STRING("KEEP", ev("tan(89.9999deg)"));
    TEST_ASSERT_EQUAL_STRING("10381.3274", ev("tan(1.5707)"));
    TEST_ASSERT_EQUAL_STRING("KEEP", ev("tan(-1.5708)"));
    TEST_ASSERT_EQUAL_STRING("1.5574", ev("tan(1)"));
    TEST_ASSERT_EQUAL_STRING("-90747.2695", ev("tan(4.7124)"));
}

/* asin(), acos() (NaN past [-1, 1]) and atan() */
static void test_arc(void) {
    TEST_ASSERT_EQUAL_STRING("90deg", ev("asin(1)"));
    TEST_ASSERT_EQUAL_STRING("-90deg", ev("asin(-1)"));
    TEST_ASSERT_EQUAL_STRING("30deg", ev("asin(0.5)"));
    TEST_ASSERT_EQUAL_STRING("89.1897deg", ev("asin(0.9999)"));
    TEST_ASSERT_EQUAL_STRING("KEEP", ev("asin(1.0001)"));
    TEST_ASSERT_EQUAL_STRING("KEEP", ev("asin(-1.0001)"));
    TEST_ASSERT_EQUAL_STRING("0deg", ev("acos(1)"));
    TEST_ASSERT_EQUAL_STRING("90deg", ev("acos(0)"));
    TEST_ASSERT_EQUAL_STRING("180deg", ev("acos(-1)"));
    TEST_ASSERT_EQUAL_STRING("120deg", ev("acos(-0.5)"));
    TEST_ASSERT_EQUAL_STRING("KEEP", ev("acos(-1.0001)"));
    TEST_ASSERT_EQUAL_STRING("0.8103deg", ev("acos(0.9999)"));
    TEST_ASSERT_EQUAL_STRING("0deg", ev("atan(0)"));
    TEST_ASSERT_EQUAL_STRING("45deg", ev("atan(1)"));
    TEST_ASSERT_EQUAL_STRING("-45deg", ev("atan(-1)"));
    TEST_ASSERT_EQUAL_STRING("89.9994deg", ev("atan(100000)"));
    TEST_ASSERT_EQUAL_STRING("-22.4993deg", ev("atan(-0.4142)"));
    TEST_ASSERT_EQUAL_STRING("22.5042deg", ev("atan(0.4143)"));
}

/* atan2(): the four quadrants, the axes, mixed units of one type */
static void test_atan2(void) {
    TEST_ASSERT_EQUAL_STRING("0deg", ev("atan2(0, 0)"));
    TEST_ASSERT_EQUAL_STRING("180deg", ev("atan2(0, -1)"));
    TEST_ASSERT_EQUAL_STRING("-90deg", ev("atan2(-1, 0)"));
    TEST_ASSERT_EQUAL_STRING("90deg", ev("atan2(1, 0)"));
    TEST_ASSERT_EQUAL_STRING("45deg", ev("atan2(1px, 1px)"));
    TEST_ASSERT_EQUAL_STRING("-135deg", ev("atan2(-1s, -1s)"));
    TEST_ASSERT_EQUAL_STRING("153.4349deg", ev("atan2(1px, -2px)"));
    TEST_ASSERT_EQUAL_STRING("-116.5651deg", ev("atan2(-2px, -1px)"));
    TEST_ASSERT_EQUAL_STRING("0.5968deg", ev("atan2(1px, 1in)"));
    TEST_ASSERT_EQUAL_STRING("161.5651deg", ev("atan2(10%, -30%)"));
    TEST_ASSERT_EQUAL_STRING("KEEP", ev("atan2(1px, 1em)"));
    TEST_ASSERT_EQUAL_STRING("0.1592deg", ev("atan2(1deg, 1turn)"));
}

/* pow(): integer exponents exact (ties rounded half away from zero), others through exp(B ln A) */
static void test_pow(void) {
    TEST_ASSERT_EQUAL_STRING("1024", ev("pow(2, 10)"));
    TEST_ASSERT_EQUAL_STRING("0.5", ev("pow(2, -1)"));
    TEST_ASSERT_EQUAL_STRING("0.0313", ev("pow(0.5, 5)"));
    TEST_ASSERT_EQUAL_STRING("-0.0313", ev("pow(-0.5, 5)"));
    TEST_ASSERT_EQUAL_STRING("0.0313", ev("pow(2, -5)"));
    TEST_ASSERT_EQUAL_STRING("-0.0313", ev("pow(-2, -5)"));
    TEST_ASSERT_EQUAL_STRING("7.5938", ev("pow(1.5, 5)"));
    TEST_ASSERT_EQUAL_STRING("100000", ev("pow(10, 5)"));
    TEST_ASSERT_EQUAL_STRING("KEEP", ev("pow(10, 6)"));
    TEST_ASSERT_EQUAL_STRING("1", ev("pow(0, 0)"));
    TEST_ASSERT_EQUAL_STRING("1", ev("pow(-3, 0)"));
    TEST_ASSERT_EQUAL_STRING("0", ev("pow(0, 2)"));
    TEST_ASSERT_EQUAL_STRING("KEEP", ev("pow(0, -1)"));
    TEST_ASSERT_EQUAL_STRING("0", ev("pow(0, 0.5)"));
    TEST_ASSERT_EQUAL_STRING("KEEP", ev("pow(0, -0.5)"));
    TEST_ASSERT_EQUAL_STRING("KEEP", ev("pow(-2, 0.5)"));
    TEST_ASSERT_EQUAL_STRING("2", ev("pow(4, 0.5)"));
    TEST_ASSERT_EQUAL_STRING("1.4142", ev("pow(2, 0.5)"));
    TEST_ASSERT_EQUAL_STRING("-512", ev("pow(-8, 3)"));
    TEST_ASSERT_EQUAL_STRING("1", ev("pow(1, 100000)"));
    TEST_ASSERT_EQUAL_STRING("0", ev("pow(0.0001, 2)"));
    TEST_ASSERT_EQUAL_STRING("KEEP", ev("pow(0.0001, -2)"));
    TEST_ASSERT_EQUAL_STRING("10000", ev("pow(0.01, -2)"));
    TEST_ASSERT_EQUAL_STRING("0.0002", ev("pow(3, -8)"));
    TEST_ASSERT_EQUAL_STRING("0.0001", ev("pow(3, -9)"));
    TEST_ASSERT_EQUAL_STRING("3814.6973", ev("pow(0.4, -9)"));      /* past -8: through exp(B ln A) */
    TEST_ASSERT_EQUAL_STRING("50805.2634", ev("pow(0.3, -9)"));
    TEST_ASSERT_EQUAL_STRING("-0.0001", ev("pow(-3, -9)"));
    TEST_ASSERT_EQUAL_STRING("22015.456", ev("pow(1.0001, 100000)"));
    TEST_ASSERT_EQUAL_STRING("3.5738", ev("pow(1.01, 128)"));
    TEST_ASSERT_EQUAL_STRING("3.5385", ev("pow(1.01, 127)"));
    TEST_ASSERT_EQUAL_STRING("-3.5385", ev("pow(-1.01, 127)"));
    TEST_ASSERT_EQUAL_STRING("0.1216", ev("pow(0.9, 20)"));
    TEST_ASSERT_EQUAL_STRING("3.9528", ev("pow(2.5, 1.5)"));
    TEST_ASSERT_EQUAL_STRING("100000", ev("pow(100000, 1)"));
    TEST_ASSERT_EQUAL_STRING("KEEP", ev("pow(100000, 2)"));
    TEST_ASSERT_EQUAL_STRING("1", ev("pow(1.1, 0.0001)"));
    TEST_ASSERT_EQUAL_STRING("7.3892", ev("pow(e, 2)"));
    TEST_ASSERT_EQUAL_STRING("99997.1946", ev("pow(2, 16.6096)"));
    /* exact ties of integer exponents (exp(B ln A) may land on either side) */
    TEST_ASSERT_EQUAL_STRING("525.2188", ev("pow(3.5, 5)"));
    TEST_ASSERT_EQUAL_STRING("-11602.9063", ev("pow(-6.5, 5)"));
    TEST_ASSERT_EQUAL_STRING("77378.0938", ev("pow(9.5, 5)"));
    TEST_ASSERT_EQUAL_STRING("0.7813", ev("pow(1.28, -1)"));
    TEST_ASSERT_EQUAL_STRING("0.0063", ev("pow(160, -1)"));
    TEST_ASSERT_EQUAL_STRING("-0.0013", ev("pow(-800, -1)"));
    /* 2^64 * 10000: past the limit, its low word is 0 */
    TEST_ASSERT_EQUAL_STRING("KEEP", ev("pow(65536, 4)"));
    TEST_ASSERT_EQUAL_STRING("65536", ev("pow(65536, 1)"));
    /* the decimals past the fourth are truncated, then rounded once */
    TEST_ASSERT_EQUAL_STRING("0.1176", ev("pow(0.7, 6)"));          /* 0.117649 */
    TEST_ASSERT_EQUAL_STRING("0", ev("pow(0.007, 2)"));             /* 0.000049 */
    TEST_ASSERT_EQUAL_STRING("26.8435", ev("pow(1.6, 7)"));         /* 26.8435456 */
}

/* sqrt(): exact integer square root, rounded half away from zero */
static void test_sqrt(void) {
    TEST_ASSERT_EQUAL_STRING("0", ev("sqrt(0)"));
    TEST_ASSERT_EQUAL_STRING("1.4142", ev("sqrt(2)"));
    TEST_ASSERT_EQUAL_STRING("KEEP", ev("sqrt(-1)"));
    TEST_ASSERT_EQUAL_STRING("KEEP", ev("sqrt(-0.0001)"));
    TEST_ASSERT_EQUAL_STRING("0.01", ev("sqrt(0.0001)"));
    TEST_ASSERT_EQUAL_STRING("316.2278", ev("sqrt(100000)"));
    TEST_ASSERT_EQUAL_STRING("1.5", ev("sqrt(2.25)"));
    TEST_ASSERT_EQUAL_STRING("0.0141", ev("sqrt(0.0002)"));
}

/* hypot(): values of one type, exact */
static void test_hypot(void) {
    TEST_ASSERT_EQUAL_STRING("5px", ev("hypot(3px, 4px)"));
    TEST_ASSERT_EQUAL_STRING("3px", ev("hypot(-3px)"));
    TEST_ASSERT_EQUAL_STRING("13px", ev("hypot(3px, -4px, 12px)"));
    TEST_ASSERT_EQUAL_STRING("120px", ev("hypot(1in, 72px)"));
    TEST_ASSERT_EQUAL_STRING("100000px", ev("hypot(60000px, 80000px)"));
    TEST_ASSERT_EQUAL_STRING("KEEP", ev("hypot(60000.0001px, 80000px)"));
    TEST_ASSERT_EQUAL_STRING("KEEP", ev("hypot(1px, 1em)"));
    TEST_ASSERT_EQUAL_STRING("1s", ev("hypot(1s, 1ms)"));
    TEST_ASSERT_EQUAL_STRING("2.2361", ev("hypot(1, 1, 1, 1, 1)"));
    TEST_ASSERT_EQUAL_STRING("50%", ev("hypot(30%, 40%)"));
    TEST_ASSERT_EQUAL_STRING("0.0001px", ev("hypot(0.0001px, 0.0001px)"));
}

/* log(A) and log(A, B) */
static void test_log(void) {
    TEST_ASSERT_EQUAL_STRING("0", ev("log(1)"));
    TEST_ASSERT_EQUAL_STRING("1", ev("log(e)"));
    TEST_ASSERT_EQUAL_STRING("KEEP", ev("log(0)"));
    TEST_ASSERT_EQUAL_STRING("KEEP", ev("log(-1)"));
    TEST_ASSERT_EQUAL_STRING("3", ev("log(8, 2)"));
    TEST_ASSERT_EQUAL_STRING("2", ev("log(100, 10)"));
    TEST_ASSERT_EQUAL_STRING("0.5", ev("log(2, 4)"));
    TEST_ASSERT_EQUAL_STRING("KEEP", ev("log(5, 1)"));
    TEST_ASSERT_EQUAL_STRING("KEEP", ev("log(5, 0)"));
    TEST_ASSERT_EQUAL_STRING("KEEP", ev("log(5, -2)"));
    TEST_ASSERT_EQUAL_STRING("KEEP", ev("log(1, 1)"));
    TEST_ASSERT_EQUAL_STRING("0", ev("log(1, 2)"));
    TEST_ASSERT_EQUAL_STRING("-9.2103", ev("log(0.0001)"));
    TEST_ASSERT_EQUAL_STRING("11.5129", ev("log(100000)"));
    TEST_ASSERT_EQUAL_STRING("6931.8184", ev("log(2, 1.0001)"));
    TEST_ASSERT_EQUAL_STRING("KEEP", ev("log(100000, 0.9999)"));
    TEST_ASSERT_EQUAL_STRING("0.5", ev("log(0.5, 0.25)"));
    TEST_ASSERT_EQUAL_STRING("0.0001", ev("log(1.0001)"));
    TEST_ASSERT_EQUAL_STRING("-0.0001", ev("log(0.9999)"));
    TEST_ASSERT_EQUAL_STRING("0.6931", ev("log(2)"));
    TEST_ASSERT_EQUAL_STRING("1.0986", ev("log(3)"));
}

/* exp(): past the limit kept, far below 1/20000 gives 0 */
static void test_exp(void) {
    TEST_ASSERT_EQUAL_STRING("1", ev("exp(0)"));
    TEST_ASSERT_EQUAL_STRING("2.7183", ev("exp(1)"));
    TEST_ASSERT_EQUAL_STRING("0.3679", ev("exp(-1)"));
    TEST_ASSERT_EQUAL_STRING("99997.4535", ev("exp(11.5129)"));
    TEST_ASSERT_EQUAL_STRING("KEEP", ev("exp(11.513)"));
    TEST_ASSERT_EQUAL_STRING("KEEP", ev("exp(11.52)"));
    TEST_ASSERT_EQUAL_STRING("0.0001", ev("exp(-9.9)"));
    TEST_ASSERT_EQUAL_STRING("0", ev("exp(-10)"));
    TEST_ASSERT_EQUAL_STRING("0", ev("exp(-12.5)"));
    TEST_ASSERT_EQUAL_STRING("0", ev("exp(-100000)"));
    TEST_ASSERT_EQUAL_STRING("KEEP", ev("exp(12.1)"));
    TEST_ASSERT_EQUAL_STRING("1.9999", ev("exp(0.6931)"));
    TEST_ASSERT_EQUAL_STRING("0.7071", ev("exp(-0.3466)"));
    TEST_ASSERT_EQUAL_STRING("1.4143", ev("exp(0.3466)"));
    TEST_ASSERT_EQUAL_STRING("148.4132", ev("exp(5)"));
}

static void test_types(void) {
    TEST_ASSERT_EQUAL(WF_CSSPROP_NUMBER, wf_cssprop_math_type("pow(2, 3)"));
    TEST_ASSERT_EQUAL(WF_CSSPROP_BAD, wf_cssprop_math_type("pow(2px, 3)"));
    TEST_ASSERT_EQUAL(WF_CSSPROP_BAD, wf_cssprop_math_type("pow(2, 3deg)"));
    TEST_ASSERT_EQUAL(WF_CSSPROP_BAD, wf_cssprop_math_type("pow(2px, 3px)"));   /* one type, still not numbers */
    TEST_ASSERT_EQUAL(WF_CSSPROP_BAD, wf_cssprop_math_type("pow(1deg, 2deg)"));
    TEST_ASSERT_EQUAL(WF_CSSPROP_BAD, wf_cssprop_math_type("pow(2)"));
    TEST_ASSERT_EQUAL(WF_CSSPROP_BAD, wf_cssprop_math_type("pow(2, 3, 4)"));
    TEST_ASSERT_EQUAL(WF_CSSPROP_BAD, wf_cssprop_math_type("sqrt(4px)"));
    TEST_ASSERT_EQUAL(WF_CSSPROP_BAD, wf_cssprop_math_type("sqrt(4, 2)"));
    TEST_ASSERT_EQUAL(WF_CSSPROP_BAD, wf_cssprop_math_type("exp(1s)"));
    TEST_ASSERT_EQUAL(WF_CSSPROP_BAD, wf_cssprop_math_type("exp(1, 2)"));
    TEST_ASSERT_EQUAL(WF_CSSPROP_BAD, wf_cssprop_math_type("log(1deg)"));
    TEST_ASSERT_EQUAL(WF_CSSPROP_BAD, wf_cssprop_math_type("log(8, 2px)"));
    TEST_ASSERT_EQUAL(WF_CSSPROP_BAD, wf_cssprop_math_type("log(8, 2, 2)"));
    TEST_ASSERT_EQUAL(WF_CSSPROP_BAD, wf_cssprop_math_type("hypot()"));
    TEST_ASSERT_EQUAL(WF_CSSPROP_BAD, wf_cssprop_math_type("hypot(1px, 1deg)"));
    TEST_ASSERT_EQUAL(WF_CSSPROP_LENPCT, wf_cssprop_math_type("hypot(3px, 4%)"));
    TEST_ASSERT_EQUAL_STRING("KEEP", ev("hypot(3px, 4%)"));
    TEST_ASSERT_EQUAL(WF_CSSPROP_ANGLE, wf_cssprop_math_type("hypot(1deg, 1turn)"));
    TEST_ASSERT_EQUAL(WF_CSSPROP_TIME, wf_cssprop_math_type("HYPOT(1s)"));
    TEST_ASSERT_EQUAL(WF_CSSPROP_ANGLE, wf_cssprop_math_type("atan2(1px, 2%)"));
    TEST_ASSERT_EQUAL_STRING("KEEP", ev("atan2(1px, 2%)"));
    TEST_ASSERT_EQUAL(WF_CSSPROP_LENGTH, wf_cssprop_math_type("calc(sqrt(2) * 1px)"));
    TEST_ASSERT_EQUAL_STRING("14.142px", ev("calc(sqrt(2) * 10px)"));
    TEST_ASSERT_EQUAL_STRING("0.5", ev("calc(sin(30deg) * cos(0))"));
    TEST_ASSERT_EQUAL_STRING("45deg", ev("atan2(sin(45deg), cos(45deg))"));
    TEST_ASSERT_EQUAL_STRING("30deg", ev("asin(sin(30deg))"));
}

/* registered values: one math function and one numeric alternative */
static void test_compute(void) {
    TEST_ASSERT_EQUAL_STRING("8", cv("'<number>'", "pow(2, 3)"));
    TEST_ASSERT_EQUAL_STRING("3", cv("'<integer>'", "sqrt(10)"));
    TEST_ASSERT_EQUAL_STRING("4", cv("'<integer>'", "sqrt(12.25)"));          /* 3.5, half toward +infinity */
    TEST_ASSERT_EQUAL_STRING("90deg", cv("'<angle>'", "atan2(1, 0)"));
    TEST_ASSERT_EQUAL_STRING("5px", cv("'<length>'", "hypot(3px, 4px)"));
    TEST_ASSERT_EQUAL_STRING("31.416px", cv("'<length> | <number>'", "calc(pi * 10px)"));
    TEST_ASSERT_EQUAL_STRING("2.7183", cv("'<length> | <number>'", "exp(1)"));
    TEST_ASSERT_EQUAL_STRING("KEEP", cv("'<number>'", "log(0)"));
    TEST_ASSERT_EQUAL_STRING("KEEP", cv("'<time>'", "exp(1)"));
    TEST_ASSERT_EQUAL_STRING("KEEP", cv("'<number>+'", "pow(2, 3)"));
    TEST_ASSERT_EQUAL_STRING("KEEP", cv("'<number>'", "calc(infinity)"));
    TEST_ASSERT_EQUAL_STRING("KEEP", cv("'<length>'", "hypot(1em, 1px)"));
}

/* the reader sheet computes them where they are declared */
static void test_sheet(void) {
    snprintf(g_html, sizeof(g_html), "<html><head><style>"
             "@property --a { syntax: '<angle>'; inherits: true; initial-value: atan2(1, 1) }"
             "@property --r { syntax: '<length>'; inherits: false; initial-value: hypot(6px, 8px) }"
             "@property --n { syntax: '<number>'; inherits: false; initial-value: 0 }"
             "#o { --a: acos(-1); --n: pow(2, 0.5) } #i { --r: calc(sqrt(16) * 1px); --n: tan(90deg) }"
             "</style></head><body><div id=o><p id=i>i</p></div><p id=q>q</p></body></html>");
    TEST_ASSERT_EQUAL(0, wf_site_prepare(&g_d, g_html, (int)strlen(g_html)));
    TEST_ASSERT_EQUAL(3, g_d.nprops);
    TEST_ASSERT_EQUAL_STRING("180deg", nv("o", "--a"));
    TEST_ASSERT_EQUAL_STRING("180deg", nv("i", "--a"));      /* inherits the computed value */
    TEST_ASSERT_EQUAL_STRING("45deg", nv("q", "--a"));       /* the initial value, computed */
    TEST_ASSERT_EQUAL_STRING("10px", nv("q", "--r"));
    TEST_ASSERT_EQUAL_STRING("4px", nv("i", "--r"));
    TEST_ASSERT_EQUAL_STRING("1.4142", nv("o", "--n"));
    TEST_ASSERT_EQUAL_STRING("tan(90deg)", nv("i", "--n"));  /* infinite: the text is kept */
    wf_site_props_text(&g_d, g_t, (int)sizeof(g_t));
    TEST_ASSERT(strstr(g_t, " simplified 3") != 0);   /* the three declared values computed (not the initial ones) */
}

int main(void) {
    unity_init();
    RUN_TEST(test_constants);
    RUN_TEST(test_sin);
    RUN_TEST(test_cos);
    RUN_TEST(test_tan);
    RUN_TEST(test_arc);
    RUN_TEST(test_atan2);
    RUN_TEST(test_pow);
    RUN_TEST(test_sqrt);
    RUN_TEST(test_hypot);
    RUN_TEST(test_log);
    RUN_TEST(test_exp);
    RUN_TEST(test_types);
    RUN_TEST(test_compute);
    RUN_TEST(test_sheet);
    unity_print_results();
    unity_cleanup();
    return unity_stats.tests_failed == 0 ? 0 : 1;
}
