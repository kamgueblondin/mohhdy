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
    c.ex_px = c.ch_px = -1;
    out[0] = 0;
    r = wf_css_math_box_ctx(s, &c, &px, out, (int)sizeof(out));
    snprintf(g_o, sizeof(g_o), "%d %d %s", r, px, out);
    return g_o;
}

/* line-height in the style attribute: Lot 116 (converted): px with two
 * decimals within the engine's 0..400 and a number as the number (was
 * whole px within 10..40, cut toward zero); normal under an ancestor
 * that is not normal; nothing when the whole chain is normal */
static void test_l112_lh_style(void) {
    TEST_ASSERT_EQUAL_STRING(" style=\"line-height:20px\"", b_of("#b { line-height: 20px }"));
    TEST_ASSERT_EQUAL_STRING(" style=\"line-height:1.5\"", b_of("#b { line-height: 1.5 }"));
    TEST_ASSERT_EQUAL_STRING(" style=\"line-height:20.9px\"", b_of("#b { line-height: 20.9px }"));
    TEST_ASSERT_EQUAL_STRING(" style=\"line-height:41px\"", b_of("#b { line-height: 41px }"));
    TEST_ASSERT_EQUAL_STRING(" style=\"line-height:9px\"", b_of("#b { line-height: 9px }"));
    TEST_ASSERT_EQUAL_STRING(" style=\"line-height:0\"", b_of("#b { line-height: 0 }"));
    TEST_ASSERT_EQUAL_STRING("", b_of("#b { line-height: normal }"));
    TEST_ASSERT_EQUAL_STRING("", b_of("#i { line-height: 20px }"));
    TEST_ASSERT_EQUAL_STRING(" style=\"line-height:normal\"", b_of("#o { line-height: 20px } #b { line-height: normal }"));
    TEST_ASSERT_EQUAL_STRING(" style=\"line-height:normal\"", style_of("i"));
    TEST_ASSERT_EQUAL_STRING(" style=\"line-height:20px\"", b_of("html { line-height: 20px }"));
    TEST_ASSERT_EQUAL_STRING(" style=\"line-height:20px\"", style_of("body"));
    /* a number inherits as a number: #b's own font gives its px (Lot
     * 116: the number is passed, the engine multiplies) */
    TEST_ASSERT_EQUAL_STRING(" style=\"font-size:10px;line-height:2\"", b_of("#o { line-height: 2 } #b { font-size: 10px }"));
    /* with other properties, before the box (the engine reads it with
     * the font size) */
    TEST_ASSERT_EQUAL_STRING(" style=\"line-height:30px;overflow-y:auto;height:60px\"",
                             b_of("#b { line-height: 30px; overflow: auto; height: 2lh } " RD));
    /* the report's node line names it */
    prep("#o { line-height: 2 } #b { line-height: normal }");
    TEST_ASSERT_TRUE(wf_site_report(&g_d, g_rep, (int)sizeof(g_rep)) > 0);
    TEST_ASSERT_NOT_NULL(strstr(g_rep, "node div #o display auto lh num 2\n"));     /* Lot 116: was lh 32 */
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
    TEST_ASSERT_EQUAL_STRING(" style=\"line-height:0\"", style_of("i"));
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
    /* font-size in rlh: the root's line height (normal at 16px: 10px) */
    prep("#o { font-size: 2rlh }");
    TEST_ASSERT_EQUAL(20000, g_d.fs[3]);
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
    /* a registered property's keyframes take var() too (from #ff0000) */
    prep_at("@property --x { syntax: '<color>'; inherits: false; initial-value: #000000 } "
            "@keyframes f { from { --x: var(--c) } to { --x: #ffffff } } #b { --c: #ff0000; color: var(--x); animation: f 1s linear }", 500);
    TEST_ASSERT_EQUAL_STRING(" style=\"color:#ff8080\"", style_of("b"));
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

/* "r px" of wf_css_math_box_ctx with ex and ch given (px thousandths) */
static const char *mfont(const char *s, int em, int ex, int ch) {
    wf_css_box_ctx_t c;
    char out[64];
    int px = -7, r;
    c.vw = 640; c.vh = 480; c.em_px = em; c.rem_px = 16000; c.lh_px = -1; c.rlh_px = -1; c.basis = -1;
    c.ex_px = ex; c.ch_px = ch;
    out[0] = 0;
    r = wf_css_math_box_ctx(s, &c, &px, out, (int)sizeof(out));
    snprintf(g_o, sizeof(g_o), "%d %d %s", r, px, out);
    return g_o;
}

/* ex and ch of the font the engine draws: scale q = (px + 2) / 4 within
 * 4..16 of the rounded font px (4..64), ch 2q, ex (5q + 2) / 4 */
static void test_l112_font_metrics(void) {
    TEST_ASSERT_EQUAL_STRING("1 14000 ", mfont("2ex", 16000, 7000, 11000));
    TEST_ASSERT_EQUAL_STRING("1 11000 ", mfont("1ch", 16000, 7000, 11000));
    TEST_ASSERT_EQUAL_STRING("1 31000 ", mfont("calc(2ex + 1.5ch - 1px)", 16000, 7000, 12000));
    TEST_ASSERT_EQUAL_STRING("2 0 calc(50% + 14px)", mfont("calc(50% + 2ex)", 16000, 7000, 11000));
    /* negative: the 16px proportions (5/16 and 8/16 em) */
    TEST_ASSERT_EQUAL_STRING("1 15000 ", mfont("2ex", 24000, -1, -1));
    TEST_ASSERT_EQUAL_STRING("1 12000 ", mfont("1ch", 24000, -1, -1));
    TEST_ASSERT_EQUAL_STRING("1 7000 ", mfont("1ch", 24000, -1, 7000));
    TEST_ASSERT_EQUAL_STRING("1 15000 ", mfont("2ex", 24000, -1, 7000));
    TEST_ASSERT_EQUAL_STRING("-1 0 ", mfont("1ex", 16000, 10001000, 8000));
    TEST_ASSERT_EQUAL_STRING("-1 0 ", mfont("1ch", 16000, 5000, 10001000));
    /* the inputs are bounded even when the value does not use them */
    TEST_ASSERT_EQUAL_STRING("-1 0 ", mfont("1px", 16000, 10001000, 8000));
    TEST_ASSERT_EQUAL_STRING("-1 0 ", mfont("0ex", 16000, 5000, 10001000));
    TEST_ASSERT_EQUAL_STRING("1 1000 ", mfont("1px", 16000, 10000000, 10000000));
    /* box scroller heights against the element's font */
    TEST_ASSERT_EQUAL_STRING(" style=\"overflow-y:auto;height:50px\"", b_of("#b { overflow: auto; height: 10ex } " RD));
    TEST_ASSERT_EQUAL_STRING(" style=\"font-size:24px;overflow-y:auto;height:36px\"",
                             b_of("#b { font-size: 24px; overflow: auto; height: 3ch } " RD));
    TEST_ASSERT_EQUAL_STRING(" style=\"font-size:18px;overflow-y:auto;height:24px\"",
                             b_of("#b { font-size: 18px; overflow: auto; height: 4ex } " RD));
    TEST_ASSERT_EQUAL_STRING(" style=\"font-size:8px;overflow-y:auto;height:24px\"",
                             b_of("#b { font-size: 8px; overflow: auto; height: 3ch } " RD));
    TEST_ASSERT_EQUAL_STRING(" style=\"font-size:64px;overflow-y:auto;height:40px\"",
                             b_of("#b { font-size: 64px; overflow: auto; height: 2ex } " RD));
    /* past the engine's 64px: its largest scale */
    TEST_ASSERT_EQUAL_STRING(" style=\"font-size:90px;overflow-y:auto;height:32px\"",
                             b_of("#b { font-size: 90px; overflow: auto; height: 1ch } " RD));
    /* the font px is rounded: 17.5px draws at 18px (scale 5), 17.4px at 17px (scale 4) */
    TEST_ASSERT_EQUAL_STRING(" style=\"font-size:17.5px;overflow-y:auto;height:10px\"",
                             b_of("#b { font-size: 17.5px; overflow: auto; height: 1ch } " RD));
    TEST_ASSERT_EQUAL_STRING(" style=\"font-size:17.4px;overflow-y:auto;height:8px\"",
                             b_of("#b { font-size: 17.4px; overflow: auto; height: 1ch } " RD));
    TEST_ASSERT_EQUAL_STRING(" style=\"font-size:2px;overflow-y:auto;height:8px\"",
                             b_of("#b { font-size: 2px; overflow: auto; height: 1ch } " RD));
    /* in line-height, of the element's own font */
    prep("#o { font-size: 18px; line-height: 4ex } #b { font-size: 24px; line-height: 3ch }");
    TEST_ASSERT_EQUAL_STRING("0 24001 36001 36001", lh4());
    /* inherited from html; html's own lengths (the body's 50% of 10ex) */
    TEST_ASSERT_EQUAL_STRING(" style=\"overflow-y:auto;height:40px\"",
                             b_of("html { font-size: 24px } #b { overflow: auto; height: 5ex } " RD));
    prep("html { overflow: hidden; font-size: 24px; height: 10ex } body { overflow: auto; height: 50% } " RD);
    TEST_ASSERT_EQUAL_STRING(" style=\"overflow-y:auto;height:40px\"", style_of("body"));
}

/* font-size in ex, ch, lh and rlh: the parent's font and line height,
 * the root's line height (CSS Values 4 6.1.1) */
static void test_l112_fs_units(void) {
    prep("#o { font-size: 24px } #b { font-size: 2ex }");
    TEST_ASSERT_EQUAL(16000, g_d.fs[4]);
    TEST_ASSERT_EQUAL_STRING(" style=\"font-size:16px\"", style_of("b"));
    TEST_ASSERT_EQUAL(16000, g_d.fs[5]);
    prep("#o { font-size: 18px } #b { font-size: 3ch }");
    TEST_ASSERT_EQUAL(30000, g_d.fs[4]);
    prep("#o { line-height: 30px } #b { font-size: 1lh }");
    TEST_ASSERT_EQUAL(30000, g_d.fs[4]);
    prep("#o { font-size: 20px; line-height: 1.5 } #b { font-size: 0.5lh }");
    TEST_ASSERT_EQUAL(15000, g_d.fs[4]);
    prep("#b { font-size: 2lh }");
    TEST_ASSERT_EQUAL(20000, g_d.fs[4]);
    prep("html { line-height: 15px } #b { font-size: 2rlh }");
    TEST_ASSERT_EQUAL(30000, g_d.fs[4]);
    /* in the root's own font-size: rlh and lh are the initial normal */
    prep("html { font-size: 2rlh; line-height: 40px }");
    TEST_ASSERT_EQUAL(20000, g_d.fs[0]);
    prep("html { font-size: 3lh }");
    TEST_ASSERT_EQUAL(30000, g_d.fs[0]);
    /* math, with a percentage of the parent's size */
    prep("#o { font-size: 20px } #b { font-size: calc(50% + 1ex) }");
    TEST_ASSERT_EQUAL(16000, g_d.fs[4]);
    prep("#b { font-size: max(1lh, 1ch, 12px) }");
    TEST_ASSERT_EQUAL(12000, g_d.fs[4]);
    prep("#o { font-size: 2ex } #b { font-size: 2ex }");
    TEST_ASSERT_EQUAL(10000, g_d.fs[3]);
    TEST_ASSERT_EQUAL(10000, g_d.fs[4]);
    /* refused or out of range: the parent's size, nothing passed */
    prep("#o { font-size: 20px } #b { font-size: -1ex }");
    TEST_ASSERT_EQUAL(20000, g_d.fs[4]);
    TEST_ASSERT_EQUAL_STRING("", style_of("b"));
    prep("#b { font-size: 0ch }");
    TEST_ASSERT_EQUAL_STRING("", style_of("b"));
    prep("#b { font-size: calc(1ex - 20px) }");
    TEST_ASSERT_EQUAL(16000, g_d.fs[4]);
    TEST_ASSERT_EQUAL_STRING("", style_of("b"));
    /* refused when it is read: the earlier declaration stays */
    prep("#b { font-size: 20px; font-size: -1ex }");
    TEST_ASSERT_EQUAL(20000, g_d.fs[4]);
    prep("#b { font-size: 20px; font-size: 0lh }");
    TEST_ASSERT_EQUAL(20000, g_d.fs[4]);
    prep("#b { font-size: 20px; font-size: calc(1ex - 20px) }");
    TEST_ASSERT_EQUAL(16000, g_d.fs[4]);
    prep("#b { font-size: 2ex 3px }");
    TEST_ASSERT_EQUAL_STRING("", style_of("b"));
    /* Lot 116 (converted): cap is read now (7 rows at 16px: 14px); cqh
     * stands for a unit font-size does not take */
    prep("#b { font-size: 2cap }");
    TEST_ASSERT_EQUAL_STRING(" style=\"font-size:14px\"", style_of("b"));
    prep("#b { font-size: 2cqh }");
    TEST_ASSERT_EQUAL_STRING("", style_of("b"));
    prep("#b { font-size: calc(1000ch) }");
    TEST_ASSERT_EQUAL(16000, g_d.fs[4]);
    prep("#b { font-size: 12000ch }");
    TEST_ASSERT_EQUAL(16000, g_d.fs[4]);
    /* the earlier paths are unchanged */
    prep("#o { font-size: 20px } #b { font-size: 1.5em }");
    TEST_ASSERT_EQUAL_STRING(" style=\"font-size:1.5em\"", style_of("b"));
    TEST_ASSERT_EQUAL(30000, g_d.fs[4]);
    prep("#b { font-size: calc(1em + 2px) }");
    TEST_ASSERT_EQUAL_STRING(" style=\"font-size:18px\"", style_of("b"));
    /* the lh of a font-size in lh: the element's own line height follows */
    prep("#o { line-height: 1.5 } #b { font-size: 1lh }");
    TEST_ASSERT_EQUAL(24000, g_d.fs[4]);
    TEST_ASSERT_EQUAL_STRING("0 24001 36001 36001", lh4());
}

/* var() in a registered property's keyframes and transition start: the
 * element's values below its own animations */
static void test_l112_reg_frames(void) {
    const char *P = "@property --x { syntax: '<length>'; inherits: false; initial-value: 0px } ";
    char css[1024];
    snprintf(css, sizeof(css), "%s@keyframes f { from { --x: var(--a) } to { --x: 100px } } #b { --a: 20px; animation: f 1s linear }", P);
    prep_at(css, 500);
    TEST_ASSERT_EQUAL_STRING("60px", wf_site_node_var(&g_d, 4, "--x"));
    TEST_ASSERT_EQUAL(1, g_d.prop_animated);
    /* an inherited custom property */
    snprintf(css, sizeof(css), "%s@keyframes f { from { --x: var(--a) } to { --x: 100px } } #o { --a: 40px } #b { animation: f 1s linear }", P);
    prep_at(css, 500);
    TEST_ASSERT_EQUAL_STRING("70px", wf_site_node_var(&g_d, 4, "--x"));
    /* invalid after substitution: the frame is left out (0% from the own value) */
    snprintf(css, sizeof(css), "%s@keyframes f { from { --x: var(--a) } to { --x: 100px } } #b { --a: red; animation: f 1s linear }", P);
    prep_at(css, 500);
    TEST_ASSERT_EQUAL_STRING("50px", wf_site_node_var(&g_d, 4, "--x"));
    snprintf(css, sizeof(css), "%s@keyframes f { from { --x: var(--zz) } to { --x: 100px } } #b { --x: 20px; animation: f 1s linear }", P);
    prep_at(css, 500);
    TEST_ASSERT_EQUAL_STRING("60px", wf_site_node_var(&g_d, 4, "--x"));
    snprintf(css, sizeof(css), "%s@keyframes f { from { --x: var(--zz, 50px) } to { --x: 100px } } #b { animation: f 1s linear }", P);
    prep_at(css, 500);
    TEST_ASSERT_EQUAL_STRING("75px", wf_site_node_var(&g_d, 4, "--x"));
    /* another animated registered property is read below its animation */
    prep_at("@property --y { syntax: '<length>'; inherits: false; initial-value: 0px } "
            "@property --x { syntax: '<length>'; inherits: false; initial-value: 0px } "
            "@keyframes ky { to { --y: 140px } } @keyframes kx { from { --x: var(--y) } to { --x: 100px } } "
            "#b { --y: 40px; animation: ky 1s linear, kx 1s linear }", 500);
    TEST_ASSERT_EQUAL_STRING("90px", wf_site_node_var(&g_d, 4, "--y"));
    TEST_ASSERT_EQUAL_STRING("70px", wf_site_node_var(&g_d, 4, "--x"));
    /* an inherited registered property: the child takes the parent's
     * final value, read by its own keyframes too */
    prep_at("@property --y { syntax: '<length>'; inherits: true; initial-value: 0px } "
            "@property --x { syntax: '<length>'; inherits: false; initial-value: 0px } "
            "@keyframes ky { to { --y: 140px } } @keyframes kx { from { --x: var(--y) } to { --x: 100px } } "
            "#o { --y: 40px; animation: ky 1s linear } #b { animation: kx 1s linear }", 500);
    TEST_ASSERT_EQUAL_STRING("90px", wf_site_node_var(&g_d, 4, "--y"));
    TEST_ASSERT_EQUAL_STRING("95px", wf_site_node_var(&g_d, 4, "--x"));
    /* a transition from a @starting-style value with var() */
    snprintf(css, sizeof(css), "%s@starting-style { #b { --x: var(--a) } } #b { --a: 0px; --x: 100px; transition: --x 1s linear }", P);
    prep_at(css, 500);
    TEST_ASSERT_EQUAL_STRING("50px", wf_site_node_var(&g_d, 4, "--x"));
    TEST_ASSERT_EQUAL(1, g_d.prop_transitioned);
}

/* var() invalid at computed-value time is unset for every property:
 * inherited ones inherit, the others take their initial value */
static void test_l112_unset_all(void) {
    prep("#o { color: #ff0000 } #b { color: #0000ff; color: var(--u) }");
    TEST_ASSERT_EQUAL_STRING(" style=\"color:#ff0000\"", style_of("b"));
    prep("#o { --c: 12px; color: #ff0000 } #b { color: #0000ff; color: var(--c) }");
    TEST_ASSERT_EQUAL_STRING(" style=\"color:#ff0000\"", style_of("b"));
    prep("#b { background-color: #ff0000; background-color: var(--u) }");
    TEST_ASSERT_EQUAL_STRING("", style_of("b"));
    prep("#o { font-size: 20px } #b { font-size: 30px; font-size: var(--u) }");
    TEST_ASSERT_EQUAL(20000, g_d.fs[4]);
    prep("#b { margin: 5px; margin: var(--u) }");      /* initial: 0, passed as such */
    TEST_ASSERT_EQUAL_STRING(" style=\"margin:0px 0px 0px 0px\"", style_of("b"));
    prep("#o { text-align: center } #b { text-align: right; text-align: var(--u, 12px) }");
    TEST_ASSERT_EQUAL_STRING(" style=\"text-align:center\"", style_of("b"));
    prep("#b { display: none; display: var(--u) }");
    TEST_ASSERT_TRUE(wf_site_report(&g_d, g_rep, (int)sizeof(g_rep)) > 0);
    TEST_ASSERT_NULL(strstr(g_rep, "#b display none"));
    /* !important stays important */
    prep("#b { color: var(--u) !important } #b { color: #0000ff }");
    TEST_ASSERT_EQUAL_STRING("", style_of("b"));
    /* a per-element value: unset where it does not fit, the value where it does */
    prep("#o { --c: 12px } #b { --c: #00ff00 } #o, #b { color: #0000ff; color: var(--c) }");
    TEST_ASSERT_EQUAL_STRING("", style_of("o"));
    TEST_ASSERT_EQUAL_STRING(" style=\"color:#00ff00\"", style_of("b"));
    /* a per-element var() whose sheet-wide value does not fit: still read
     * per element (was dropped); unset where the element's own does not fit */
    prep(":root { --w: 12px } #b { --w: bold } #i { font-weight: var(--w) }");
    TEST_ASSERT_EQUAL_STRING(" style=\"font-weight:bold\"", style_of("i"));
    prep("#b { --w: center } :root { --w: 1px } #i { text-align: var(--w) }");
    TEST_ASSERT_EQUAL_STRING(" style=\"text-align:center\"", style_of("i"));
    prep("#b { --w: bold } * { --w: 12px } #i { font-weight: var(--w) }");
    TEST_ASSERT_EQUAL_STRING("", style_of("i"));
    prep(":root { --w: 12px } #o { font-style: italic } #b { --w: 2px } #i { font-style: normal; font-style: var(--w) }");
    TEST_ASSERT_EQUAL_STRING(" style=\"font-style:italic\"", style_of("o"));
    TEST_ASSERT_EQUAL_STRING("", style_of("i"));     /* inherited from #o (italic) */
    /* Lot 116 (converted): not a property the reader keeps as a kind but
     * one with a clear initial value: unset, height auto, so #b passes no
     * height and is no box scroller (was dropped, the earlier height
     * stayed: " style=\"overflow-y:auto;height:50px\"") */
    prep("#b { overflow: auto; height: 50px; height: var(--u) } " RD);
    TEST_ASSERT_EQUAL_STRING("", style_of("b"));
    /* font-weight: a number 1..1000 with nothing after it */
    prep("#b { font-weight: bold; font-weight: 12px }");
    TEST_ASSERT_EQUAL_STRING(" style=\"font-weight:bold\"", style_of("b"));
    prep("#b { font-weight: normal; font-weight: 1001 }");
    TEST_ASSERT_EQUAL_STRING("", style_of("b"));
    prep("#b { font-weight: normal; font-weight: 1000 }");
    TEST_ASSERT_EQUAL_STRING(" style=\"font-weight:bold\"", style_of("b"));
    prep("#b { font-weight: 700 }");
    TEST_ASSERT_EQUAL_STRING(" style=\"font-weight:bold\"", style_of("b"));
    prep("#b { font-weight: bold; font-weight: 1001 }");
    TEST_ASSERT_EQUAL_STRING(" style=\"font-weight:bold\"", style_of("b"));
    prep("#b { font-weight: bold; font-weight: 1000 }");
    TEST_ASSERT_EQUAL_STRING(" style=\"font-weight:bold\"", style_of("b"));
    prep("#b { font-weight: 600 }");
    TEST_ASSERT_EQUAL_STRING(" style=\"font-weight:bold\"", style_of("b"));
    prep("#b { font-weight: bold; font-weight: 599 }");
    TEST_ASSERT_EQUAL_STRING("", style_of("b"));
    prep("#b { font-weight: bold; font-weight: 0 }");
    TEST_ASSERT_EQUAL_STRING(" style=\"font-weight:bold\"", style_of("b"));
    prep("#b { font-weight: bold; font-weight: 99999999999 }");
    TEST_ASSERT_EQUAL_STRING(" style=\"font-weight:bold\"", style_of("b"));
}

/* html's width auto: the body's percentage width is of the viewport */
static void test_l112_html_auto(void) {
    prep("html { overflow: hidden } body { overflow: auto; width: 25%; max-width: 50%; height: 100px } " RD);
    TEST_ASSERT_EQUAL_STRING(" style=\"overflow-y:auto;height:100px;overflow-x:auto;width:160px;max-width:320px\"",
                             style_of("body"));
    prep("html { overflow: hidden; width: auto } body { overflow: auto; width: 25%; height: 100px } " RD);
    TEST_ASSERT_EQUAL_STRING(" style=\"overflow-y:auto;height:100px;overflow-x:auto;width:160px\"", style_of("body"));
    /* Lot 116 (converted, were "width:25%" left as it was): html's
     * margin and padding come off the viewport's width, (640 - 20) / 4
     * and (640 - 8) / 4 */
    prep("html { overflow: hidden; margin: 0 10px } body { overflow: auto; width: 25%; height: 100px } " RD);
    TEST_ASSERT_NOT_NULL(strstr(style_of("body"), "width:155px"));
    prep("html { overflow: hidden; padding: 4px } body { overflow: auto; width: 25%; height: 100px } " RD);
    TEST_ASSERT_NOT_NULL(strstr(style_of("body"), "width:158px"));
    /* html with a max-width: left as it was */
    prep("html { overflow: hidden; max-width: 500px } body { overflow: auto; width: 25%; height: 100px } " RD);
    TEST_ASSERT_NOT_NULL(strstr(style_of("body"), "width:25%"));
    prep("html { overflow: hidden; max-width: 50% } body { overflow: auto; width: 25%; height: 100px } " RD);
    TEST_ASSERT_NOT_NULL(strstr(style_of("body"), "width:25%"));
    /* an html width that does not resolve (past 10000 px) is not auto */
    prep("html { overflow: hidden; width: calc(2000% + 1px) } body { overflow: auto; width: 25%; height: 100px } " RD);
    TEST_ASSERT_NOT_NULL(strstr(style_of("body"), "width:25%"));
    /* the height still needs html's height (auto: left as it is) */
    prep("html { overflow: hidden } body { overflow: auto; height: 50% } " RD);
    TEST_ASSERT_EQUAL_STRING(" style=\"overflow-y:auto;height:50%\"", style_of("body"));
    /* not the body: unchanged */
    prep("html { overflow: hidden } #o { overflow: auto; width: 25%; height: 100px } " RD);
    TEST_ASSERT_NOT_NULL(strstr(style_of("o"), "width:25%"));
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
    RUN_TEST(test_l112_font_metrics);
    RUN_TEST(test_l112_fs_units);
    RUN_TEST(test_l112_reg_frames);
    RUN_TEST(test_l112_unset_all);
    RUN_TEST(test_l112_html_auto);
    unity_print_results();
    unity_cleanup();
    return unity_stats.tests_failed == 0 ? 0 : 1;
}
