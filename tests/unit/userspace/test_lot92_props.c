/* Lot 92 (US-031): math functions abs, sign, round, mod, rem and the
 * trigonometric ones, and the computed value of registered properties,
 * in wf_cssprop.c and the reader sheet. Hand-written cases next to the
 * generated suites (test_lot92_mathtype.c, test_lot92_matheval.c,
 * test_lot92_compute.c, test_lot92_propsheet.c). */
#include "../../framework/unity.h"
#include <string.h>
#include <stdio.h>
#include "wf_sitecss.h"
#include "wf_cssprop.h"

static wf_site_t g_d;
static char g_html[8192], g_o[64], g_t[256];
static wf_cssprop_t g_p;

static void prep(const char *css, const char *body) {
    snprintf(g_html, sizeof(g_html), "<html><head><style>%s</style></head><body>%s</body></html>", css, body);
    TEST_ASSERT_EQUAL(0, wf_site_prepare(&g_d, g_html, (int)strlen(g_html)));
}

static const char *nv(const char *id, const char *name) {
    int i;
    for (i = 0; i < g_d.nnodes; i++)
        if (!strcmp(g_d.node[i].id, id)) { const char *v = wf_site_node_var(&g_d, i, name); return v ? v : "(none)"; }
    return "(no node)";
}

static const char *ev(const char *v) {
    int t, r = wf_cssprop_math_eval(v, &t, g_o, (int)sizeof(g_o));
    return r == 1 ? g_o : r == 0 ? "KEEP" : "BAD";
}

static const char *cv(const char *syn, const char *v) {
    memset(&g_p, 0, sizeof(g_p));
    g_p.syntax = (unsigned char)wf_cssprop_syntax_parse(syn, (int)strlen(syn), &g_p);
    return wf_cssprop_compute(&g_p, v, g_o, (int)sizeof(g_o)) ? g_o : "KEEP";
}

static void test_round_strategies(void) {
    TEST_ASSERT_EQUAL_STRING("8px", ev("round(7.5px, 2px)"));        /* 3.75 steps -> 4 */
    TEST_ASSERT_EQUAL_STRING("8px", ev("round(7px, 2px)"));          /* 3.5 steps, half toward +infinity -> 4 */
    TEST_ASSERT_EQUAL_STRING("-6px", ev("round(-7px, 2px)"));        /* -3.5 steps -> -3 */
    TEST_ASSERT_EQUAL_STRING("6px", ev("round(down, 7px, 2px)"));
    TEST_ASSERT_EQUAL_STRING("8px", ev("ROUND(UP, 6.1px, 2px)"));
    TEST_ASSERT_EQUAL_STRING("-6px", ev("round(to-zero, -7.9px, 2px)"));
}

static void test_round_half_toward_infinity(void) {
    TEST_ASSERT_EQUAL_STRING("4", ev("round(3.5)"));
    TEST_ASSERT_EQUAL_STRING("-3", ev("round(-3.5)"));
    TEST_ASSERT_EQUAL_STRING("-3", ev("round(nearest, -3.5, 1)"));
    TEST_ASSERT_EQUAL_STRING("4", ev("round(up, 3.1)"));
    TEST_ASSERT_EQUAL_STRING("-3", ev("round(up, -3.9)"));
    TEST_ASSERT_EQUAL_STRING("3", ev("round(down, 3.9)"));
    TEST_ASSERT_EQUAL_STRING("-4", ev("round(down, -3.1)"));
    TEST_ASSERT_EQUAL_STRING("3", ev("round(to-zero, 3.9)"));
    TEST_ASSERT_EQUAL_STRING("-3", ev("round(to-zero, -3.9)"));
    /* the sign of B does not matter */
    TEST_ASSERT_EQUAL_STRING("10px", ev("round(up, 7px, -5px)"));
    TEST_ASSERT_EQUAL_STRING("90deg", ev("round(100deg, 0.25turn)"));
    TEST_ASSERT_EQUAL_STRING("KEEP", ev("round(7px, 0px)"));
    TEST_ASSERT_EQUAL_STRING("BAD", ev("round(7px)"));
    TEST_ASSERT_EQUAL_STRING("BAD", ev("round(up)"));
    TEST_ASSERT_EQUAL_STRING("BAD", ev("round(up 7, 2)"));
    TEST_ASSERT_EQUAL_STRING("BAD", ev("round(ceil, 7, 2)"));
}

static void test_mod_and_rem_signs(void) {
    TEST_ASSERT_EQUAL_STRING("2px", ev("mod(-7px, 3px)"));
    TEST_ASSERT_EQUAL_STRING("-1px", ev("rem(-7px, 3px)"));
    TEST_ASSERT_EQUAL_STRING("-2px", ev("mod(7px, -3px)"));
    TEST_ASSERT_EQUAL_STRING("1px", ev("rem(7px, -3px)"));
    TEST_ASSERT_EQUAL_STRING("0.5s", ev("mod(2500ms, 1s)"));
    TEST_ASSERT_EQUAL_STRING("KEEP", ev("mod(7px, 0px)"));
    TEST_ASSERT_EQUAL_STRING("KEEP", ev("rem(7px, 1em)"));
    TEST_ASSERT_EQUAL_STRING("BAD", ev("mod(7px, 3deg)"));
    TEST_ASSERT_EQUAL(WF_CSSPROP_LENPCT, wf_cssprop_math_type("mod(7px, 3%)"));
}

static void test_abs_sign_and_trig_types(void) {
    TEST_ASSERT_EQUAL_STRING("1.5s", ev("abs(-1500ms)"));
    TEST_ASSERT_EQUAL_STRING("-1", ev("sign(-0.0001px)"));
    TEST_ASSERT_EQUAL_STRING("0", ev("sign(-0.00004px)"));       /* rounds to 0 at the fourth decimal */
    TEST_ASSERT_EQUAL_STRING("1", ev("sign(2turn)"));
    TEST_ASSERT_EQUAL_STRING("KEEP", ev("sign(1em)"));
    TEST_ASSERT_EQUAL(WF_CSSPROP_NUMBER, wf_cssprop_math_type("sign(1px + 2%)"));
    TEST_ASSERT_EQUAL(WF_CSSPROP_NUMBER, wf_cssprop_math_type("sin(30deg)"));
    TEST_ASSERT_EQUAL(WF_CSSPROP_NUMBER, wf_cssprop_math_type("cos(0.5)"));
    TEST_ASSERT_EQUAL(WF_CSSPROP_BAD, wf_cssprop_math_type("tan(1px)"));
    TEST_ASSERT_EQUAL(WF_CSSPROP_ANGLE, wf_cssprop_math_type("asin(0.5)"));
    TEST_ASSERT_EQUAL(WF_CSSPROP_BAD, wf_cssprop_math_type("acos(1deg)"));
    TEST_ASSERT_EQUAL(WF_CSSPROP_ANGLE, wf_cssprop_math_type("atan2(1px, 2%)"));
    TEST_ASSERT_EQUAL(WF_CSSPROP_ANGLE, wf_cssprop_math_type("atan2(1s, 2ms)"));
    TEST_ASSERT_EQUAL(WF_CSSPROP_ANGLE, wf_cssprop_math_type("ATAN(1)"));
    TEST_ASSERT_EQUAL(WF_CSSPROP_BAD, wf_cssprop_math_type("atan2(1px)"));
    TEST_ASSERT_EQUAL_STRING("KEEP", ev("sin(30deg)"));
    TEST_ASSERT_EQUAL(WF_CSSPROP_BAD, wf_cssprop_math_type("pow(2, 3)"));
    TEST_ASSERT_EQUAL(WF_CSSPROP_BAD, wf_cssprop_math_type("calc(pi)"));
}

static void test_units_and_bounds(void) {
    TEST_ASSERT_EQUAL_STRING("96px", ev("calc(1in)"));
    TEST_ASSERT_EQUAL_STRING("37.7953px", ev("calc(1cm)"));
    TEST_ASSERT_EQUAL_STRING("0.9449px", ev("calc(1q)"));
    TEST_ASSERT_EQUAL_STRING("1.3333px", ev("calc(1pt)"));
    TEST_ASSERT_EQUAL_STRING("90deg", ev("calc(100grad)"));
    TEST_ASSERT_EQUAL_STRING("57.2958deg", ev("calc(1rad)"));
    TEST_ASSERT_EQUAL_STRING("0.001s", ev("calc(1ms)"));
    TEST_ASSERT_EQUAL_STRING("1.2346px", ev("calc(1.23456px)"));
    TEST_ASSERT_EQUAL_STRING("1.2345px", ev("calc(1.23454999px)"));
    TEST_ASSERT_EQUAL_STRING("-1.2346px", ev("calc(-1.23455px)"));
    /* 100000 units at most, at every step */
    TEST_ASSERT_EQUAL_STRING("100000px", ev("calc(100000px)"));
    TEST_ASSERT_EQUAL_STRING("KEEP", ev("calc(100000.0001px)"));
    TEST_ASSERT_EQUAL_STRING("KEEP", ev("calc(50000px * 3 / 3)"));
    TEST_ASSERT_EQUAL_STRING("KEEP", ev("calc(2000in)"));
    TEST_ASSERT_EQUAL_STRING("KEEP", ev("calc(1000000000000px)"));
    TEST_ASSERT_EQUAL_STRING("0.3333px", ev("calc(1px / 3)"));
    TEST_ASSERT_EQUAL_STRING("0.9999px", ev("calc(1px / 3 * 3)"));
    TEST_ASSERT_EQUAL_STRING("KEEP", ev("calc(1px / 0)"));
    TEST_ASSERT_EQUAL_STRING("KEEP", ev("calc(1px + 5%)"));
    TEST_ASSERT_EQUAL_STRING("KEEP", ev("calc(1vw)"));
    TEST_ASSERT_EQUAL_STRING("0px", ev("calc(-0.00004px)"));
    TEST_ASSERT_EQUAL_STRING("20px", ev("clamp(1px, 50px, 20px)"));
    TEST_ASSERT_EQUAL_STRING("30px", ev("clamp(30px, 5px, 20px)"));   /* MIN wins over MAX */
    TEST_ASSERT_EQUAL_STRING("1.5", ev("min(2, 1.5, 3)"));
    TEST_ASSERT_EQUAL_STRING("KEEP", ev("min(2px, 1em)"));
}

static void test_compute_alternatives(void) {
    TEST_ASSERT_EQUAL_STRING("4", cv("'<integer>'", "calc(7 / 2)"));
    TEST_ASSERT_EQUAL_STRING("-3", cv("'<integer>'", "calc(-7 / 2)"));     /* half toward +infinity */
    TEST_ASSERT_EQUAL_STRING("3.5", cv("'<number> | <integer>'", "calc(7 / 2)"));
    TEST_ASSERT_EQUAL_STRING("4", cv("'<integer> | <number>'", "calc(7 / 2)"));
    TEST_ASSERT_EQUAL_STRING("KEEP", cv("'<length>+'", "calc(1px + 2px)"));
    TEST_ASSERT_EQUAL_STRING("KEEP", cv("'<length># | <length>'", "calc(1px + 2px)"));
    TEST_ASSERT_EQUAL_STRING("3px", cv("'auto | <length>'", "calc(1px + 2px)"));
    TEST_ASSERT_EQUAL_STRING("KEEP", cv("'*'", "calc(1px + 2px)"));
    TEST_ASSERT_EQUAL_STRING("KEEP", cv("'<length-percentage>'", "calc(1px + 2%)"));
    TEST_ASSERT_EQUAL_STRING("3%", cv("'<length-percentage>'", "calc(1% + 2%)"));
    TEST_ASSERT_EQUAL_STRING("KEEP", cv("'<length>'", "calc(1px + 2%)"));
    TEST_ASSERT_EQUAL_STRING("KEEP", cv("'<length>'", "12px"));
    TEST_ASSERT_EQUAL_STRING("KEEP", cv("'<angle>'", "calc(1px)"));
    TEST_ASSERT_EQUAL_STRING("180deg", cv("'<angle> | <length>'", "calc(0.25turn * 2)"));
    TEST_ASSERT_EQUAL_STRING("0.25s", cv("'<time>'", "max(250ms, 0.1s)"));
    TEST_ASSERT_EQUAL_STRING("KEEP", cv("'<number>'", "sin(30deg)"));
    /* a one-type registration filled in by hand (nalt 0) */
    memset(&g_p, 0, sizeof(g_p));
    g_p.syntax = WF_CSSPROP_LENGTH;
    TEST_ASSERT_EQUAL(1, wf_cssprop_compute(&g_p, "calc(2px * 3)", g_o, (int)sizeof(g_o)));
    TEST_ASSERT_EQUAL_STRING("6px", g_o);
    TEST_ASSERT_EQUAL(0, wf_cssprop_compute(&g_p, "calc(2deg * 3)", g_o, (int)sizeof(g_o)));
    TEST_ASSERT_EQUAL_STRING("", g_o);
    /* too small an output: kept */
    TEST_ASSERT_EQUAL(0, wf_cssprop_compute(&g_p, "calc(2px * 3)", g_o, 4));
}

static void test_sheet_computed_and_inherited(void) {
    prep("@property --g { syntax: '<length>'; inherits: true; initial-value: calc(1in / 4) }"
         "#o { --g: calc(10px * 2 + 1pc) } #i { --h: calc(var(--g) * 2) } #j { --g: calc(1em * 2) }",
         "<div id=o><p id=i>i</p><p id=j>j</p></div><p id=q>q</p>");
    TEST_ASSERT_EQUAL_STRING("36px", nv("o", "--g"));
    TEST_ASSERT_EQUAL_STRING("36px", nv("i", "--g"));                    /* inherits the computed value */
    TEST_ASSERT_EQUAL_STRING("calc(36px * 2)", nv("i", "--h"));          /* not registered: text */
    TEST_ASSERT_EQUAL_STRING("calc(1em * 2)", nv("j", "--g"));           /* relative: kept */
    TEST_ASSERT_EQUAL_STRING("24px", nv("q", "--g"));                    /* the initial value, computed */
    wf_site_props_text(&g_d, g_t, (int)sizeof(g_t));
    TEST_ASSERT_NOT_NULL(strstr(g_t, " simplified 1"));
    TEST_ASSERT_EQUAL(1, g_d.prop_simplified);
}

static void test_sheet_without_math_keeps_the_lot90_line(void) {
    prep("@property --g { syntax: '<length>'; inherits: false; initial-value: 4px } #o { --g: 5px }",
         "<p id=o>o</p>");
    wf_site_props_text(&g_d, g_t, (int)sizeof(g_t));
    TEST_ASSERT_NULL(strstr(g_t, "simplified"));
    TEST_ASSERT_EQUAL(0, g_d.prop_simplified);
}

static void test_sheet_integer_and_gradient_radius(void) {
    prep("@property --n { syntax: '<integer>'; inherits: false; initial-value: 0 }"
         "@property --r { syntax: '<length>'; inherits: false; initial-value: 0px }"
         "#c { --n: round(up, 7 / 2) }"
         "#a { --r: calc(20px * 2); background-image: radial-gradient(circle var(--r), #ff0000, #0000ff) }"
         "#b { --u: calc(20px * 2); background-image: radial-gradient(circle var(--u), #ff0000, #0000ff) }",
         "<p id=c>c</p><div id=a>a</div><div id=b>b</div>");
    TEST_ASSERT_EQUAL_STRING("4", nv("c", "--n"));
    TEST_ASSERT_EQUAL_STRING("40px", nv("a", "--r"));
    TEST_ASSERT_EQUAL_STRING("calc(20px * 2)", nv("b", "--u"));
    TEST_ASSERT_EQUAL(2, g_d.prop_simplified);
}

int main(void) {
    unity_init();
    RUN_TEST(test_round_strategies);
    RUN_TEST(test_round_half_toward_infinity);
    RUN_TEST(test_mod_and_rem_signs);
    RUN_TEST(test_abs_sign_and_trig_types);
    RUN_TEST(test_units_and_bounds);
    RUN_TEST(test_compute_alternatives);
    RUN_TEST(test_sheet_computed_and_inherited);
    RUN_TEST(test_sheet_without_math_keeps_the_lot90_line);
    RUN_TEST(test_sheet_integer_and_gradient_radius);
    unity_print_results();
    unity_cleanup();
    return unity_stats.tests_failed == 0 ? 0 : 1;
}
