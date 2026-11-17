/* Lot 112 (US-031): line-height passed to the engine, line-height with a
 * percentage or of numbers in a math function, rlh, all and revert on
 * line-height, var() invalid at computed-value time for opacity and
 * line-height, comparable arguments of mixed min()/max(), var() and math
 * functions in opacity keyframes (wf_sitecss, wf_cssmath). Hand cases next
 * to the generated test_lot112_*.c; expected values follow
 * docs/lot112_plan.md. */
#include "../../framework/unity.h"
#include <string.h>
#include <stdio.h>
#include "wf_sitecss.h"
#include "wf_cssmath.h"

static char g_o[512];
static wf_site_t g_d;
static char g_html[4096];
static char g_rep[16384];

#define KF "@keyframes k { from { color: #000000 } to { color: #ffffff } } "
#define RD "#i { animation: k 1s linear; animation-timeline: scroll(nearest) } "

/* html 0, head 1, body 2, #o 3, #b 4, #i 5 */
static void prep_at(const char *css, int ms) {
    wf_site_set_time(ms);
    snprintf(g_html, sizeof(g_html), "<html><head><style>" KF "%s</style></head><body><div id=o><div id=b>"
             "<p id=i>i</p></div></div></body></html>", css);
    TEST_ASSERT_EQUAL(0, wf_site_prepare_ext(&g_d, g_html, (int)strlen(g_html), 640, 480, 0, 0));
    wf_site_set_time(0);
}

static void prep(const char *css) { prep_at(css, 0); }

static const char *style_of(const char *id) {
    int i;
    g_o[0] = 0;
    for (i = 0; i < g_d.nnodes; i++)
        if (!strcmp(g_d.node[i].id, id) || (!g_d.node[i].id[0] && !strcmp(g_d.node[i].tag, id)))
            wf_site_style_attr(&g_d, g_d.node[i].start, g_o, (int)sizeof(g_o));
    return g_o;
}

/* #b's style with css */
static const char *b_of(const char *css) {
    prep(css);
    return style_of("b");
}

/* the line heights the reader keeps for body, #o, #b and #i */
static const char *lh4(void) {
    snprintf(g_o, sizeof(g_o), "%d %d %d %d", g_d.lh[2], g_d.lh[3], g_d.lh[4], g_d.lh[5]);
    return g_o;
}

/* "r px" of wf_css_math_box_ctx (basis >= 0) or "r px out" */
static const char *mctx(const char *s, int lh, int rlh, int basis) {
    wf_css_box_ctx_t c;
    char out[64];
    int px = -7, r;
    c.vw = 640; c.vh = 480; c.em_px = 16000; c.rem_px = 20000; c.lh_px = lh; c.rlh_px = rlh; c.basis = basis;
    out[0] = 0;
    r = wf_css_math_box_ctx(s, &c, &px, out, (int)sizeof(out));
    snprintf(g_o, sizeof(g_o), "%d %d %s", r, px, out);
    return g_o;
}

/* line-height in the style attribute: whole px within the engine's
 * 10..40, cut toward zero; normal under an ancestor that is not normal;
 * nothing when the whole chain is normal */
static void test_l112_lh_style(void) {
    TEST_ASSERT_EQUAL_STRING(" style=\"line-height:20px\"", b_of("#b { line-height: 20px }"));
    TEST_ASSERT_EQUAL_STRING(" style=\"line-height:24px\"", b_of("#b { line-height: 1.5 }"));
    TEST_ASSERT_EQUAL_STRING(" style=\"line-height:20px\"", b_of("#b { line-height: 20.9px }"));
    TEST_ASSERT_EQUAL_STRING(" style=\"line-height:40px\"", b_of("#b { line-height: 41px }"));
    TEST_ASSERT_EQUAL_STRING(" style=\"line-height:10px\"", b_of("#b { line-height: 9px }"));
    TEST_ASSERT_EQUAL_STRING(" style=\"line-height:10px\"", b_of("#b { line-height: 0 }"));
    TEST_ASSERT_EQUAL_STRING("", b_of("#b { line-height: normal }"));
    TEST_ASSERT_EQUAL_STRING("", b_of("#i { line-height: 20px }"));
    TEST_ASSERT_EQUAL_STRING(" style=\"line-height:normal\"", b_of("#o { line-height: 20px } #b { line-height: normal }"));
    TEST_ASSERT_EQUAL_STRING(" style=\"line-height:normal\"", style_of("i"));
    TEST_ASSERT_EQUAL_STRING(" style=\"line-height:20px\"", b_of("html { line-height: 20px }"));
    TEST_ASSERT_EQUAL_STRING(" style=\"line-height:20px\"", style_of("body"));
    /* a number inherits as a number: #b's own font gives its px */
    TEST_ASSERT_EQUAL_STRING(" style=\"font-size:10px;line-height:20px\"", b_of("#o { line-height: 2 } #b { font-size: 10px }"));
    /* with other properties, before the box (the engine reads it with
     * the font size) */
    TEST_ASSERT_EQUAL_STRING(" style=\"line-height:30px;overflow-y:auto;height:60px\"",
                             b_of("#b { line-height: 30px; overflow: auto; height: 2lh } " RD));
    /* the report's node line names it */
    prep("#o { line-height: 2 } #b { line-height: normal }");
    TEST_ASSERT_TRUE(wf_site_report(&g_d, g_rep, (int)sizeof(g_rep)) > 0);
    TEST_ASSERT_NOT_NULL(strstr(g_rep, "node div #o display auto lh 32\n"));
    TEST_ASSERT_NOT_NULL(strstr(g_rep, "node div #b display auto lh normal\n"));
    TEST_ASSERT_NOT_NULL(strstr(g_rep, "node p #i display auto lh normal\n"));
    TEST_ASSERT_NOT_NULL(strstr(g_rep, "node body display auto\n"));
}

/* a percentage inside a math function is of the element's font size; a
 * math function of numbers is a number (below 0 is 0) */
static void test_l112_lh_math(void) {
    prep("#o { line-height: calc(100% + 4px) } #b { font-size: 32px; line-height: calc(50% + 1em) } "
         "#i { line-height: min(150%, 30px) }");
    TEST_ASSERT_EQUAL_STRING("0 20001 48001 30001", lh4());     /* #i: 150% of 32px is past 30px */
    prep("#o { line-height: calc(1 + 0.5) } #b { font-size: 10px } #i { line-height: calc(1 - 2) }");
    TEST_ASSERT_EQUAL_STRING("0 24001 15001 1", lh4());     /* the number inherits; 0 px is not normal */
    TEST_ASSERT_EQUAL_STRING(" style=\"line-height:10px\"", style_of("i"));
    prep("#o { line-height: max(1, 0.5 * 3); line-height: clamp(10px, 125%, 30px) } #b { line-height: calc(2lh - 50%) }");
    TEST_ASSERT_EQUAL_STRING("0 20001 32001 32001", lh4());
    prep("#o { line-height: 2; line-height: calc(1 + 1px) } #b { line-height: calc(10% * 3) }");
    TEST_ASSERT_EQUAL_STRING("0 32001 4801 4801", lh4());
    /* past 1000 a number is 1000 (lh_val's limit); a literal past 2000 in
     * a math function is refused (WF_CSSMATH_LIT): the parent's stays */
    prep("#o { font-size: 1px; line-height: calc(2000 * 1) } #b { line-height: calc(100000% + 1px) }");
    TEST_ASSERT_EQUAL_STRING("0 1000001 1000001 1000001", lh4());
    /* absolute lengths the box math does not read (px), below 0 is 0; an
     * angle is no line height */
    prep("#o { line-height: calc(1in + 1px) } #b { line-height: calc(-2deg * 1) } #i { line-height: calc(0.5in - 1in) }");
    TEST_ASSERT_EQUAL_STRING("0 97001 97001 1", lh4());
    /* a sheet-wide var() that is no line height: unset (inherited) */
    prep(":root { --x: 1deg } #o { line-height: 30px } #b { line-height: 3; line-height: var(--x) }");
    TEST_ASSERT_EQUAL_STRING("0 30001 30001 30001", lh4());
}

/* rlh: the root's line height (normal: 10/16 of its font size); in the
 * root's own line-height the initial normal at 16px */
static void test_l112_rlh(void) {
    prep("html { line-height: 24px } #o { line-height: 2rlh } #b { line-height: calc(1rlh + 1px) }");
    TEST_ASSERT_EQUAL_STRING("24001 48001 25001 25001", lh4());
    prep("html { font-size: 20px } #o { line-height: 2rlh }");
    TEST_ASSERT_EQUAL_STRING("0 25001 25001 25001", lh4());
    prep("html { font-size: 20px; line-height: 3rlh } #o { line-height: 1rlh }");
    TEST_ASSERT_EQUAL_STRING("30001 30001 30001 30001", lh4());
    /* box scroller lengths in rlh */
    TEST_ASSERT_EQUAL_STRING(" style=\"overflow-y:auto;height:30px\"", b_of("html { font-size: 24px } #b { overflow: auto; height: 2rlh } " RD));
    TEST_ASSERT_EQUAL_STRING(" style=\"line-height:20px;overflow-y:auto;height:calc(10% + 40px)\"",
                             b_of("html { line-height: 20px } #b { overflow: auto; height: calc(2rlh + 10%) } " RD));
    /* rlh is a box unit only: font-size in rlh is refused */
    prep("#o { font-size: 2rlh }");
    TEST_ASSERT_EQUAL(16000, g_d.fs[3]);
}

/* all, revert and revert-layer on line-height */
static void test_l112_lh_cascade(void) {
    prep("#o { line-height: 30px } #b { line-height: 3; all: initial }");
    TEST_ASSERT_EQUAL_STRING("0 30001 0 0", lh4());
    TEST_ASSERT_NOT_NULL(strstr(style_of("b"), "line-height:normal"));
    /* all: unset inherits the parent's computed line height, not its
     * value (2lh would be twice the parent's again) */
    prep("body { line-height: 20px } #o { line-height: 2lh } #b { line-height: 3; all: unset }");
    TEST_ASSERT_EQUAL_STRING("20001 40001 40001 40001", lh4());
    prep("#o { line-height: 30px } #b { line-height: 3; all: inherit }");
    TEST_ASSERT_EQUAL_STRING("0 30001 30001 30001", lh4());
    prep("#o { line-height: 30px } #b { line-height: 3; all: unset }");
    TEST_ASSERT_EQUAL_STRING("0 30001 30001 30001", lh4());
    prep("#o { line-height: 2 } #b { font-size: 10px; line-height: 3; all: revert }");
    TEST_ASSERT_EQUAL_STRING("0 32001 32001 32001", lh4());     /* back to inherit (no user-agent value) */
    prep("@layer a { #b { line-height: 25px } } #b { line-height: 3; line-height: revert-layer }");
    TEST_ASSERT_EQUAL_STRING("0 0 25001 25001", lh4());
    prep("@layer a { #b { line-height: 25px } } #b { line-height: 3; all: revert-layer }");
    TEST_ASSERT_EQUAL_STRING("0 0 25001 25001", lh4());
    prep("@layer a { #b { line-height: 25px } } #b { line-height: 3; line-height: revert }");
    TEST_ASSERT_EQUAL_STRING("0 0 0 0", lh4());
    prep("@layer a { #b { line-height: revert-layer } } #o { line-height: 22px } #b { line-height: revert-layer }");
    TEST_ASSERT_EQUAL_STRING("0 22001 22001 22001", lh4());
    /* a later value beats an earlier revert */
    prep("#b { line-height: revert; line-height: 20px }");
    TEST_ASSERT_EQUAL_STRING("0 0 20001 20001", lh4());
    /* the style attribute */
    snprintf(g_html, sizeof(g_html), "<html><head><style>#b { line-height: 30px }</style></head><body><div id=o>"
             "<div id=b style=\"line-height: revert\"><p id=i>i</p></div></div></body></html>");
    TEST_ASSERT_EQUAL(0, wf_site_prepare_ext(&g_d, g_html, (int)strlen(g_html), 640, 480, 0, 0));
    TEST_ASSERT_EQUAL(0, g_d.lh[4]);
}

/* var(): per element, and invalid at computed-value time (unset:
 * line-height inherited, opacity 1), no longer dropped */
static void test_l112_var_unset(void) {
    prep("#o { --v: 2 } #b { --v: 30px } #i { line-height: var(--v) } #o { line-height: var(--v) }");
    TEST_ASSERT_EQUAL_STRING("0 32001 32001 30001", lh4());
    /* the sheet-wide value of a per-element var() is no line-height: the
     * value still comes per element (was dropped) */
    prep("body { --v: auto } #o { --v: 2; line-height: var(--v) }");
    TEST_ASSERT_EQUAL_STRING("0 32001 32001 32001", lh4());
    prep("body { --v: auto } #o { --v: 0.5; opacity: var(--v) }");
    TEST_ASSERT_EQUAL_STRING(" style=\"opacity:50%\"", style_of("o"));
    /* invalid after substitution: unset beats the earlier value */
    prep("#o { line-height: 30px } #b { line-height: 3; line-height: var(--u) }");
    TEST_ASSERT_EQUAL_STRING("0 30001 30001 30001", lh4());
    prep("#o { line-height: 30px } #b { --v: auto; line-height: 3; line-height: var(--v) }");
    TEST_ASSERT_EQUAL_STRING("0 30001 30001 30001", lh4());
    prep("#b { line-height: 3; line-height: var(--u, 20px) }");
    TEST_ASSERT_EQUAL_STRING("0 0 20001 20001", lh4());
    prep("#o { opacity: 0.5; opacity: var(--u) }");
    TEST_ASSERT_EQUAL_STRING("", style_of("o"));
    prep("#o { --a: red; opacity: 0.5; opacity: var(--a) }");
    TEST_ASSERT_EQUAL_STRING("", style_of("o"));
    prep("#o { opacity: 0.5; opacity: var(--u, 30%) }");
    TEST_ASSERT_EQUAL_STRING(" style=\"opacity:30%\"", style_of("o"));
    /* a sheet-wide one */
    prep(":root { --a: 1px } #o { opacity: 0.5; opacity: var(--a) }");
    TEST_ASSERT_EQUAL_STRING("", style_of("o"));
}

/* opacity keyframes: var() of the animated element and math functions;
 * transitions from a var() starting value */
static void test_l112_op_frames(void) {
    prep_at("@keyframes f { from { opacity: var(--a) } to { opacity: 1 } } #o { --a: 0.2 } #b { animation: f 1s linear }", 500);
    TEST_ASSERT_EQUAL_STRING(" style=\"opacity:60%\"", style_of("b"));
    TEST_ASSERT_EQUAL(1, g_d.op_animated);
    prep_at("@keyframes f { from { opacity: var(--a) } to { opacity: 1 } } #b { animation: f 1s linear }", 500);
    TEST_ASSERT_EQUAL_STRING(" style=\"opacity:100%\"", style_of("b"));     /* the frame is left out */
    prep_at("@keyframes f { from { opacity: var(--a, 0) } to { opacity: 1 } } #b { animation: f 1s linear }", 500);
    TEST_ASSERT_EQUAL_STRING(" style=\"opacity:50%\"", style_of("b"));
    prep_at("@keyframes f { from { opacity: calc(0.1 * 2) } to { opacity: min(1, 0.6) } } #b { animation: f 1s linear }", 500);
    TEST_ASSERT_EQUAL_STRING(" style=\"opacity:40%\"", style_of("b"));
    prep_at("@keyframes f { from { opacity: calc(var(--a) * 2) } to { opacity: 1 } } #b { --a: 0.1; animation: f 1s linear }", 500);
    TEST_ASSERT_EQUAL_STRING(" style=\"opacity:60%\"", style_of("b"));
    /* two elements, each with its own --a */
    prep_at("@keyframes f { from { opacity: var(--a) } to { opacity: var(--a) } } #o { --a: 0.4; animation: f 1s linear } "
            "#b { --a: 0.8; animation: f 1s linear }", 500);
    TEST_ASSERT_EQUAL_STRING(" style=\"opacity:40%\"", style_of("o"));
    TEST_ASSERT_EQUAL_STRING(" style=\"opacity:80%\"", style_of("b"));
    /* a number mixed with a percentage is no value: the frame is left out */
    prep_at("@keyframes f { from { opacity: 0 } to { opacity: min(1, 60%) } } #b { opacity: 0.5; animation: f 1s linear }", 500);
    TEST_ASSERT_EQUAL_STRING(" style=\"opacity:25%\"", style_of("b"));
    /* color and background-color keyframes take var() too: the element's
     * own, inherited, a fallback, currentcolor */
    prep_at("@keyframes f { from { color: var(--c) } to { color: #ffffff } } #b { --c: #ff0000; animation: f 1s linear }", 500);
    TEST_ASSERT_EQUAL_STRING(" style=\"color:#ff8080\"", style_of("b"));
    prep_at("@keyframes f { from { color: var(--c) } to { color: var(--d, #0000ff) } } #o { --c: #ff0000 } "
            "#b { animation: f 1s linear }", 500);
    TEST_ASSERT_EQUAL_STRING(" style=\"color:#800080\"", style_of("b"));
    prep_at("@keyframes f { from { background-color: var(--c) } to { background-color: #ffffff } } "
            "#b { --c: #ff0000; background-color: #000000; animation: f 1s linear }", 500);
    TEST_ASSERT_EQUAL_STRING(" style=\"background-color:#ff8080\"", style_of("b"));
    prep_at("@keyframes f { from { color: var(--c) } to { color: #ffffff } } #o { color: #00ff00 } "
            "#b { --c: currentcolor; animation: f 1s linear }", 500);
    TEST_ASSERT_EQUAL_STRING(" style=\"color:#80ff80\"", style_of("b"));
    /* not a colour there: the frame is left out (from the element's own
     * black) */
    prep_at("@keyframes f { from { color: var(--c) } to { color: #ffffff } } #b { --c: 12px; animation: f 1s linear }", 500);
    TEST_ASSERT_EQUAL_STRING(" style=\"color:#808080\"", style_of("b"));
    /* a registered property's keyframes still leave var() out (from its
     * initial #000000) */
    prep_at("@property --x { syntax: '<color>'; inherits: false; initial-value: #000000 } "
            "@keyframes f { from { --x: var(--c) } to { --x: #ffffff } } #b { --c: #ff0000; color: var(--x); animation: f 1s linear }", 500);
    TEST_ASSERT_EQUAL_STRING(" style=\"color:#808080\"", style_of("b"));
    /* @starting-style with var(): the element's value */
    prep_at("#o { --s: 0 } @starting-style { #b { opacity: var(--s) } } #b { opacity: 0.8; transition: opacity 1s linear }", 500);
    TEST_ASSERT_EQUAL_STRING(" style=\"opacity:40%\"", style_of("b"));
    TEST_ASSERT_EQUAL(1, g_d.op_transitioned);
    prep_at("@starting-style { #b { opacity: 0 } } #b { --e: 0.8; opacity: var(--e); transition: opacity 1s linear }", 500);
    TEST_ASSERT_EQUAL_STRING(" style=\"opacity:40%\"", style_of("b"));
    /* an invalid starting var() is unset: 1 */
    prep_at("@starting-style { #b { opacity: var(--u) } } #b { opacity: 0.2; transition: opacity 1s linear }", 500);
    TEST_ASSERT_EQUAL_STRING(" style=\"opacity:60%\"", style_of("b"));
}

/* wf_css_math_box_ctx: rlh, a basis, the old entries unchanged */
static void test_l112_ctx(void) {
    TEST_ASSERT_EQUAL_STRING("1 48000 ", mctx("2rlh", -1, 24000, -1));
    TEST_ASSERT_EQUAL_STRING("1 25000 ", mctx("2rlh", -1, -1, -1));      /* 10/16 of rem 20px */
    TEST_ASSERT_EQUAL_STRING("1 40000 ", mctx("calc(1rlh + 1lh)", 16000, 24000, -1));
    TEST_ASSERT_EQUAL_STRING("2 0 calc(10% + 24px)", mctx("calc(1rlh + 10%)", -1, 24000, -1));
    TEST_ASSERT_EQUAL_STRING("1 34000 ", mctx("calc(1rlh + 10%)", -1, 24000, 100000));
    TEST_ASSERT_EQUAL_STRING("-1 0 ", mctx("calc(1rlh + 10%)", -1, 24000, 20000000));
    /* with a basis the result is 1 or -1: an empty value is -1 (0 without) */
    TEST_ASSERT_EQUAL_STRING("-1 0 ", mctx("", -1, 24000, 100000));
    TEST_ASSERT_EQUAL_STRING("0 0 ", mctx("", -1, 24000, -1));
    TEST_ASSERT_EQUAL_STRING("-1 0 ", mctx("1rlh", -1, 20000000, -1));
    TEST_ASSERT_EQUAL_STRING("-1 0 ", mctx("1rlhx", -1, 1000, -1));
    {
        int px = 5;
        TEST_ASSERT_EQUAL(-1, wf_css_math_box_ctx("1px", 0, &px, g_o, 8));
        TEST_ASSERT_EQUAL(0, px);
    }
    /* the old entries know rlh as normal of rem */
    {
        char out[64];
        int px = 0;
        TEST_ASSERT_EQUAL(1, wf_css_math_box("2rlh", 640, 480, 16000, 16000, &px, out, (int)sizeof(out)));
        TEST_ASSERT_EQUAL(20000, px);
        TEST_ASSERT_EQUAL(1, wf_css_math_box_at("calc(1rlh + 50%)", 640, 480, 16000, 16000, -1, 40000, &px));
        TEST_ASSERT_EQUAL(30000, px);
        TEST_ASSERT_EQUAL(-1, wf_css_math_box_at("1px", 640, 480, 16000, 16000, -1, -1, &px));
    }
}

/* comparable arguments of a mixed min()/max(): a leaf whose px and %
 * parts are both at least (min) or at most (max) another's goes */
static void test_l112_dominance(void) {
    TEST_ASSERT_EQUAL_STRING("2 0 min(10px, 5%)", mctx("min(10px, 20px, 5%)", -1, -1, -1));
    TEST_ASSERT_EQUAL_STRING("1 3000 ", mctx("min(3px, calc(10% + 5px))", -1, -1, -1));
    TEST_ASSERT_EQUAL_STRING("2 0 max(5px, 20%)", mctx("max(10%, 5px, 20%, 1px)", -1, -1, -1));
    TEST_ASSERT_EQUAL_STRING("2 0 min(10%, 5px)", mctx("min(10%, 10%, 5px)", -1, -1, -1));
    /* of equal leaves the first stays */
    TEST_ASSERT_EQUAL_STRING("2 0 min(calc(10% + 2px), calc(3% + 9px))",
                             mctx("min(calc(10% + 2px), calc(3% + 9px), calc(10% + 2px))", -1, -1, -1));
    TEST_ASSERT_EQUAL_STRING("2 0 3%", mctx("min(calc(10% + 2px), 3%, calc(10% + 2px))", -1, -1, -1));
    /* not comparable: a smaller % with a larger px */
    TEST_ASSERT_EQUAL_STRING("2 0 min(calc(10% + 1px), calc(5% + 9px))", mctx("min(calc(10% + 1px), calc(5% + 9px))", -1, -1, -1));
    TEST_ASSERT_EQUAL_STRING("2 0 max(calc(10% - 1px), calc(5% + 9px))", mctx("max(calc(10% - 1px), calc(5% + 9px))", -1, -1, -1));
    /* a negative factor turns min into max: the comparison follows */
    TEST_ASSERT_EQUAL_STRING("2 0 max(-10px, -5%)", mctx("calc(-1 * min(10px, 20px, 5%))", -1, -1, -1));
    /* nested: a min() in a max() is no leaf, it stays */
    TEST_ASSERT_EQUAL_STRING("2 0 max(1px, min(2px, 5%))", mctx("max(1px, min(2px, 3px, 5%))", -1, -1, -1));
    /* with a basis the value resolves */
    TEST_ASSERT_EQUAL_STRING("1 10000 ", mctx("min(10px, 20px, 5%)", -1, -1, 400000));
    /* five arguments fit once comparable ones go; five that are not are
     * refused (the engine takes four) */
    TEST_ASSERT_EQUAL_STRING("2 0 min(1%, 1px)", mctx("min(1%, 1px, 2%, 2px, 3%)", -1, -1, -1));
    TEST_ASSERT_EQUAL_STRING("-1 0 ", mctx("min(1%, calc(2% - 1px), calc(3% - 2px), calc(4% - 3px), calc(5% - 4px))", -1, -1, -1));
}

int main(void) {
    unity_init();
    RUN_TEST(test_l112_lh_style);
    RUN_TEST(test_l112_lh_math);
    RUN_TEST(test_l112_rlh);
    RUN_TEST(test_l112_lh_cascade);
    RUN_TEST(test_l112_var_unset);
    RUN_TEST(test_l112_op_frames);
    RUN_TEST(test_l112_ctx);
    RUN_TEST(test_l112_dominance);
    unity_print_results();
    unity_cleanup();
    return unity_stats.tests_failed == 0 ? 0 : 1;
}
