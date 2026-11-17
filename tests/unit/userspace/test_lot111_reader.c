/* Lot 111 (US-031): lh against the element's line-height, relative
 * colours with missing origin components, mixed min()/max()/clamp()
 * nested or in an arithmetic, the body's percentage sizes against an
 * html size given as an expression, and opacity with calc(), var(), all,
 * revert and revert-layer (wf_sitecss.c, wf_cssmath.c, wf_cssprop.c).
 * Hand cases next to the generated test_lot111_*.c; expected values
 * follow docs/lot111_plan.md. */
#include "../../framework/unity.h"
#include <string.h>
#include <stdio.h>
#include "wf_sitecss.h"
#include "wf_cssmath.h"
#include "wf_cssprop.h"

static char g_o[512];
static wf_site_t g_d;
static char g_html[4096];
static char g_rep[16384];

#define KF "@keyframes k { from { color: #000000 } to { color: #ffffff } } "
#define RD "#i { animation: k 1s linear; animation-timeline: scroll(nearest) } "

/* html 0, head 1, body 2, #o 3, #b 4, #i 5 */
static void prep(const char *css) {
    snprintf(g_html, sizeof(g_html), "<html><head><style>" KF "%s</style></head><body><div id=o><div id=b>"
             "<p id=i>i</p></div></div></body></html>", css);
    TEST_ASSERT_EQUAL(0, wf_site_prepare_ext(&g_d, g_html, (int)strlen(g_html), 640, 480, 0, 0));
}

static const char *style_of(const char *id) {
    int i;
    g_o[0] = 0;
    for (i = 0; i < g_d.nnodes; i++)
        if (!strcmp(g_d.node[i].id, id) || (!g_d.node[i].id[0] && !strcmp(g_d.node[i].tag, id)))
            wf_site_style_attr(&g_d, g_d.node[i].start, g_o, (int)sizeof(g_o));
    return g_o;
}

/* the box scroller #b's style with css */
static const char *b_of(const char *css) {
    char t[400];
    snprintf(t, sizeof(t), "%s " RD, css);
    prep(t);
    return style_of("b");
}

static const char *col(const char *v) {
    unsigned int rgb = 0;
    int a = 0;
    if (!wf_cssprop_color(v, (int)strlen(v), &rgb, &a)) return "0";
    snprintf(g_o, sizeof(g_o), "%06x %d", rgb, a);
    return g_o;
}

/* "r px out" of wf_css_math_box_lh at 640 x 480, em and rem 16 px */
static const char *mlh(const char *s, int lh) {
    char out[64];
    int px = -7, r = wf_css_math_box_lh(s, 640, 480, 16000, 16000, lh, &px, out, (int)sizeof(out));
    snprintf(g_o, sizeof(g_o), "%d %d %s", r, px, out);
    return g_o;
}

/* the same with a 256-byte buffer (no reader value is that long) */
static const char *mbig(const char *s) {
    char out[256];
    int px = -7, r = wf_css_math_box_lh(s, 640, 480, 16000, 16000, -1, &px, out, (int)sizeof(out));
    snprintf(g_o, sizeof(g_o), "%d %d %s", r, px, out);
    return g_o;
}

/* "r px" of wf_css_math_box_at */
static const char *mat(const char *s, int lh, int basis) {
    int px = -7, r = wf_css_math_box_at(s, 640, 480, 16000, 16000, lh, basis, &px);
    snprintf(g_o, sizeof(g_o), "%d %d", r, px);
    return g_o;
}

/* lh is the element's computed line-height (CSS Values 4 6.1.1, CSS
 * Inline 3 4.2): a number times the element's font size (inherited as
 * the number), a percentage or a length as px (inherited as px), normal
 * 10/16 em (the reader's font) */
static void test_l111_lh_values(void) {
    TEST_ASSERT_EQUAL_STRING(" style=\"line-height:32px;overflow-y:auto;height:96px\"", b_of("#b { overflow: auto; line-height: 2; height: 3lh }"));
    TEST_ASSERT_EQUAL_STRING(" style=\"line-height:20px;overflow-y:auto;height:40px\"", b_of("#b { overflow: auto; line-height: 20px; height: 2lh }"));
    TEST_ASSERT_EQUAL_STRING(" style=\"font-size:20px;line-height:30px;overflow-y:auto;height:30px\"",
                             b_of("#b { overflow: auto; font-size: 20px; line-height: 150%; height: 1lh }"));
    TEST_ASSERT_EQUAL_STRING(" style=\"overflow-y:auto;height:20px\"", b_of("#b { overflow: auto; height: 2lh }"));
    TEST_ASSERT_EQUAL_STRING(" style=\"line-height:20px;overflow-y:auto;height:20px\"", b_of("#b { overflow: auto; line-height: calc(1em + 4px); height: 1lh }"));
    TEST_ASSERT_EQUAL_STRING(" style=\"line-height:24px;overflow-y:auto;height:24px\"", b_of("#b { overflow: auto; line-height: 1.5rem; height: 1lh }"));
    TEST_ASSERT_EQUAL_STRING(" style=\"line-height:40px;overflow-y:auto;height:48px\"", b_of("#b { overflow: auto; line-height: 10vh; height: 1lh }"));
    /* with a percentage of the containing block: the engine's expression */
    TEST_ASSERT_EQUAL_STRING(" style=\"line-height:32px;overflow-y:auto;height:calc(10% + 32px)\"",
                             b_of("#b { overflow: auto; line-height: 2; height: calc(1lh + 10%) }"));
    TEST_ASSERT_EQUAL_STRING(" style=\"line-height:32px;overflow-y:auto;height:max(10%, 64px)\"",
                             b_of("#b { overflow: auto; line-height: 2; height: max(10%, 2lh) }"));
}

/* inheritance: numbers stay numbers, the rest is the parent's px */
static void test_l111_lh_inherit(void) {
    TEST_ASSERT_EQUAL_STRING(" style=\"font-size:10px;line-height:20px;overflow-y:auto;height:20px\"",
                             b_of("#o { line-height: 2 } #b { overflow: auto; font-size: 10px; height: 1lh }"));
    TEST_ASSERT_EQUAL_STRING(" style=\"font-size:32px;line-height:24px;overflow-y:auto;height:24px\"",
                             b_of("#o { line-height: 150% } #b { overflow: auto; font-size: 32px; height: 1lh }"));
    TEST_ASSERT_EQUAL_STRING(" style=\"font-size:8px;line-height:32px;overflow-y:auto;height:32px\"",
                             b_of("#o { line-height: 2em } #b { overflow: auto; font-size: 8px; height: 1lh }"));
    TEST_ASSERT_EQUAL_STRING(" style=\"font-size:10px;line-height:30px;overflow-y:auto;height:30px\"",
                             b_of("#o { line-height: 3 } #b { overflow: auto; line-height: inherit; font-size: 10px; height: 1lh }"));
    TEST_ASSERT_EQUAL_STRING(" style=\"line-height:normal;overflow-y:auto;height:10px\"",
                             b_of("#o { line-height: 3 } #b { overflow: auto; line-height: initial; height: 1lh }"));
    TEST_ASSERT_EQUAL_STRING(" style=\"line-height:40px;overflow-y:auto;height:48px\"",
                             b_of("#o { line-height: 3 } #b { overflow: auto; line-height: unset; height: 1lh }"));
    TEST_ASSERT_EQUAL_STRING(" style=\"line-height:40px;overflow-y:auto;height:48px\"",
                             b_of("body { line-height: 3 } #b { overflow: auto; height: 1lh }"));
    /* lh inside line-height is the parent's line height */
    TEST_ASSERT_EQUAL_STRING(" style=\"line-height:40px;overflow-y:auto;height:60px\"",
                             b_of("#o { line-height: 30px } #b { overflow: auto; line-height: 2lh; height: 1lh }"));
    TEST_ASSERT_EQUAL_STRING(" style=\"line-height:20px;overflow-y:auto;height:20px\"",
                             b_of("#b { overflow: auto; line-height: 2lh; height: 1lh }"));
    /* on the root, lh in line-height is of the initial value (normal at
     * the initial 16px), not of the root's own font */
    TEST_ASSERT_EQUAL_STRING(" style=\"line-height:20px;overflow-y:auto;height:20px\"",
                             b_of("html { font-size: 20px; line-height: 2lh } #b { overflow: auto; height: 1lh }"));
}

/* what line-height does not take: the value is normal then */
static void test_l111_lh_refused(void) {
    static const char *const bad[] = {"-1", "-10px", "2deg", "auto", "1 2", "calc(1 + 1px)", "-5%", "calc(1px + 1)",
                                      "calc(2 * 3px * 4px)", 0};
    char css[200];
    int k;
    for (k = 0; bad[k]; k++) {
        snprintf(css, sizeof(css), "#b { overflow: auto; line-height: %s; height: 2lh }", bad[k]);
        TEST_ASSERT_EQUAL_STRING(" style=\"overflow-y:auto;height:20px\"", b_of(css));
    }
    /* Lot 112 (converted, was refused): a percentage inside a math
     * function is of the element's font size (16px: 16 + 2) */
    TEST_ASSERT_EQUAL_STRING(" style=\"line-height:18px;overflow-y:auto;height:36px\"",
                             b_of("#b { overflow: auto; line-height: calc(100% + 2px); height: 2lh }"));
    /* a refused value leaves the earlier one */
    TEST_ASSERT_EQUAL_STRING(" style=\"line-height:40px;overflow-y:auto;height:60px\"",
                             b_of("#b { overflow: auto; line-height: 3; line-height: -1; height: 1.25lh }"));
}

/* the edges of a line-height value: signs, dots, tails and the limits
 * (a number at most 1000, a length at most WF_CSSMATH_MAX px/1000) */
static void test_l111_lh_edges(void) {
    static const char *const bad[] = {"-1em", "-0.5vh", "+-1px", "1.", "2.%", "50%x", "50%%", "1.5x", 0};
    char css[200];
    int k;
    for (k = 0; bad[k]; k++) {
        snprintf(css, sizeof(css), "#b { overflow: auto; line-height: %s; height: 2lh }", bad[k]);
        TEST_ASSERT_EQUAL_STRING(" style=\"overflow-y:auto;height:20px\"", b_of(css));
    }
    TEST_ASSERT_EQUAL_STRING(" style=\"line-height:32px;overflow-y:auto;height:32px\"", b_of("#b { overflow: auto; line-height: +2; height: 1lh }"));
    TEST_ASSERT_EQUAL_STRING(" style=\"line-height:24px;overflow-y:auto;height:24px\"", b_of("#b { overflow: auto; line-height: +150%; height: 1lh }"));
    TEST_ASSERT_EQUAL_STRING(" style=\"line-height:40px;overflow-y:auto;height:10000px\"", b_of("#b { overflow: auto; line-height: 20000px; height: 1lh }"));
    TEST_ASSERT_EQUAL_STRING(" style=\"font-size:1px;line-height:40px;overflow-y:auto;height:1000px\"",
                             b_of("#b { overflow: auto; font-size: 1px; line-height: 5000; height: 1lh }"));
    TEST_ASSERT_EQUAL_STRING(" style=\"font-size:1px;line-height:40px;overflow-y:auto;height:500px\"",
                             b_of("#b { overflow: auto; font-size: 1px; line-height: 50000%; height: 1lh }"));
    TEST_ASSERT_EQUAL_STRING(" style=\"font-size:1px;line-height:40px;overflow-y:auto;height:1000px\"",
                             b_of("#b { overflow: auto; font-size: 1px; line-height: 500000%; height: 1lh }"));
    TEST_ASSERT_EQUAL_STRING(" style=\"font-size:1px;line-height:40px;overflow-y:auto;height:1000px\"",
                             b_of("#b { overflow: auto; font-size: 1px; line-height: 99999999999; height: 1lh }"));
    /* a big font times a big number: at most WF_CSSMATH_MAX px/1000
     * (a font size this big is not written in the style) */
    TEST_ASSERT_EQUAL_STRING(" style=\"line-height:40px;overflow-y:auto;height:10000px\"",
                             b_of("#b { overflow: auto; font-size: 1000px; line-height: 1000; height: 1lh }"));
    TEST_ASSERT_EQUAL_STRING(" style=\"line-height:40px;overflow-y:auto;height:10000px\"",
                             b_of("#b { overflow: auto; font-size: 10000px; line-height: 1000; height: 1lh }"));
    /* 96 px (the largest font) times 44739.243%: 96 * 44739243 is 2^32 +
     * 32 in the high part, saturated (not wrapped to 0) */
    TEST_ASSERT_EQUAL_STRING(" style=\"font-size:96px;line-height:40px;overflow-y:auto;height:10000px\"",
                             b_of("#b { overflow: auto; font-size: 96px; line-height: 44739.243%; height: 1lh }"));
    /* a math function past 10000 px at the element's font (600em of
     * 20px) is normal, not the parent's value */
    TEST_ASSERT_EQUAL_STRING(" style=\"font-size:20px;line-height:normal;overflow-y:auto;height:12px\"",
                             b_of("#o { line-height: 3 } #b { overflow: auto; font-size: 20px; line-height: calc(600em); height: 1lh }"));
    /* the strongest declaration wins: importance, then specificity */
    TEST_ASSERT_EQUAL_STRING(" style=\"line-height:40px;overflow-y:auto;height:48px\"",
                             b_of("#b { overflow: auto; line-height: 3 !important; line-height: 2; height: 1lh }"));
    TEST_ASSERT_EQUAL_STRING(" style=\"line-height:40px;overflow-y:auto;height:48px\"",
                             b_of("#b { overflow: auto; line-height: 3; height: 1lh } div { line-height: 2 }"));
}

/* the math API with a line height and with a basis */
static void test_l111_math_api(void) {
    TEST_ASSERT_EQUAL_STRING("1 50000 ", mlh("calc(2lh)", 25000));
    TEST_ASSERT_EQUAL_STRING("1 20000 ", mlh("calc(2lh)", -1));
    TEST_ASSERT_EQUAL_STRING("1 0 ", mlh("calc(2lh)", 0));
    /* ex and ch keep the font (5/16 and 8/16 em), not the line height */
    TEST_ASSERT_EQUAL_STRING("1 10000 ", mlh("calc(2ex)", 25000));
    TEST_ASSERT_EQUAL_STRING("1 16000 ", mlh("calc(2ch)", 25000));
    TEST_ASSERT_EQUAL_STRING("2 0 calc(50% + 50px)", mlh("calc(50% + 2lh)", 25000));
    TEST_ASSERT_EQUAL_STRING("-1 0 ", mlh("calc(2lh)", 10000001));
    TEST_ASSERT_EQUAL_STRING("1 150000", mat("calc(50% + 2lh)", 25000, 200000));
    TEST_ASSERT_EQUAL_STRING("1 5000", mat("min(10%, 5px)", -1, 100000));
    TEST_ASSERT_EQUAL_STRING("1 10000", mat("calc(min(10%, 5px) * 2)", -1, 100000));
    TEST_ASSERT_EQUAL_STRING("1 33333", mat("calc(100% / 3)", -1, 100000));
    TEST_ASSERT_EQUAL_STRING("1 0", mat("50%", -1, 0));
    TEST_ASSERT_EQUAL_STRING("-1 0", mat("50%", -1, -1));
    TEST_ASSERT_EQUAL_STRING("-1 0", mat("50%", -1, 10000001));
    TEST_ASSERT_EQUAL_STRING("-1 0", mat("calc(50% + 1)", -1, 100000));
    TEST_ASSERT_EQUAL_STRING("1 10000000", mat("100%", -1, 10000000));
}

/* mixed min()/max()/clamp() nested or in an arithmetic (CSS Values 4
 * 10): the sum or the factor goes into the arguments */
static void test_l111_mixed(void) {
    TEST_ASSERT_EQUAL_STRING("2 0 min(max(60%, calc(50% + 20px)), max(calc(10% + 100px), 120px))",
                             mlh("calc(min(50%, 100px) + max(10%, 20px))", -1));
    TEST_ASSERT_EQUAL_STRING("2 0 min(40%, calc(10% + 40px), calc(30% + 20px), 60px)",
                             mlh("calc(min(10%, 20px) + min(30%, 40px))", -1));
    TEST_ASSERT_EQUAL_STRING("2 0 clamp(20px, 40%, 200px)", mlh("calc(2 * clamp(10px, 20%, 100px))", -1));
    TEST_ASSERT_EQUAL_STRING("2 0 min(80px, max(calc(-40% + 100px), -100px))",
                             mlh("calc(100px - 2 * clamp(10px, 20%, 100px))", -1));
    TEST_ASSERT_EQUAL_STRING("2 0 max(-20%, -40px)", mlh("calc(min(10%, 20px) * -2)", -1));
    TEST_ASSERT_EQUAL_STRING("2 0 min(calc(5% + 3px), 13px)", mlh("calc((min(10%, 20px) + 5px) / 2)", -1));
    /* Lot 112 (converted): 3% covers calc(5% + 1px) in a min(), so each
     * function has one argument left (was the whole tree) */
    TEST_ASSERT_EQUAL_STRING("2 0 1%", mlh("min(1%, max(2%, min(3%, calc(5% + 1px))))", -1));
    TEST_ASSERT_EQUAL_STRING("2 0 min(calc(9% - 9px), max(calc(1% + 9px), min(calc(9% - 8px), calc(1% + 8px))))",
                             mbig("min(calc(9% - 9px), max(calc(1% + 9px), min(calc(9% - 8px), calc(1% + 8px))))"));
    /* a min() of a min() is one min(); comparable leaves are compared
     * (Lot 112, converted: 5px covers 20px, was min(10%, 20px, 5px)) */
    TEST_ASSERT_EQUAL_STRING("2 0 min(10%, 5px)", mlh("min(min(10%, 20px), 5px)", -1));
    TEST_ASSERT_EQUAL_STRING("1 5000 ", mlh("calc(min(10%, 20px) * 0 + 5px)", -1));
    TEST_ASSERT_EQUAL_STRING("2 0 max(calc(10% - 5px), 15px)", mlh("calc(max(10%, 20px) - 5px + 5% - 5%)", -1));
    /* one argument left: the argument itself; leaves of one part compared */
    TEST_ASSERT_EQUAL_STRING("2 0 min(10%, 5px)", mlh("max(min(10%, 5px))", -1));
    TEST_ASSERT_EQUAL_STRING("1 5000 ", mlh("calc(min(10% + 5px, 10% + 7px) - 10%)", -1));
    TEST_ASSERT_EQUAL_STRING("1 7000 ", mlh("calc(max(10% + 5px, 10% + 7px) - 10%)", -1));
    TEST_ASSERT_EQUAL_STRING("2 0 10%", mlh("calc(min(10% + 5px, 20% + 5px) - 5px)", -1));
    /* an argument past 10000px in a sum or a product is refused, even
     * when a later factor brings it back */
    TEST_ASSERT_EQUAL_STRING("2 0 min(calc(1% + 600px), 601px)", mlh("calc((min(10%, 5px) + 1500px * 4) * 0.1)", -1));
    TEST_ASSERT_EQUAL_STRING("-1 0 ", mlh("calc((min(10%, 5px) + 1500px * 4 + 1500px * 4) * 0.1)", -1));
    TEST_ASSERT_EQUAL_STRING("2 0 min(1%, 600px)", mlh("calc(min(10%, 1500px * 4) * 0.1)", -1));
    TEST_ASSERT_EQUAL_STRING("-1 0 ", mlh("calc(min(10%, 1500px * 4) * 2 * 0.1)", -1));
    /* the node count with a buffer past 63 bytes: a calc() argument is 3
     * nodes, a clamp() 1 more than its arguments (24 taken, 26 not) */
    TEST_ASSERT_EQUAL_STRING("2 0 max(min(calc(1% + 4px), calc(2% + 3px), calc(3% + 2px), calc(4% + 1px)), "
                             "clamp(calc(9% + 9px), calc(10% + 10px), 1%))",
                             mbig("max(min(calc(1% + 4px), calc(2% + 3px), calc(3% + 2px), calc(4% + 1px)), "
                                  "clamp(calc(9% + 9px), calc(10% + 10px), 1%))"));
    TEST_ASSERT_EQUAL_STRING("-1 0 ", mbig("max(min(calc(1% + 4px), calc(2% + 3px), calc(3% + 2px), calc(4% + 1px)), "
                                           "clamp(calc(9% + 9px), calc(10% + 10px), calc(1% + 1px)))"));
    TEST_ASSERT_EQUAL_STRING("-1 0 ", mbig("max(min(calc(1% + 4px), calc(2% + 3px), calc(3% + 2px), calc(4% + 1px)), "
                                           "min(calc(5% + 8px), calc(6% + 7px), calc(7% + 6px), calc(8% + 5px)))"));
    /* Lot 112 (converted): the same values with comparable leaves fit
     * (calc(1% + 1px) covers the other leaves of its min()); were -1 for
     * the last two */
    TEST_ASSERT_EQUAL_STRING("2 0 max(calc(1% + 1px), clamp(calc(9% + 9px), calc(10% + 10px), 1%))",
                             mbig("max(min(calc(1% + 1px), calc(2% + 2px), calc(3% + 3px), calc(4% + 4px)), "
                                  "clamp(calc(9% + 9px), calc(10% + 10px), 1%))"));
    TEST_ASSERT_EQUAL_STRING("2 0 max(calc(1% + 1px), clamp(calc(9% + 9px), calc(10% + 10px), calc(1% + 1px)))",
                             mbig("max(min(calc(1% + 1px), calc(2% + 2px), calc(3% + 3px), calc(4% + 4px)), "
                                  "clamp(calc(9% + 9px), calc(10% + 10px), calc(1% + 1px)))"));
    TEST_ASSERT_EQUAL_STRING("2 0 calc(5% + 5px)", mbig("max(min(calc(1% + 1px), calc(2% + 2px), calc(3% + 3px), calc(4% + 4px)), "
                                                      "min(calc(5% + 5px), calc(6% + 6px), calc(7% + 7px), calc(8% + 8px)))"));
    /* the engine's bounds: 4 arguments, 4 functions deep, 24 nodes, the
     * buffer; no mixed value times a length or over a mixed value */
    TEST_ASSERT_EQUAL_STRING("2 0 min(1%, calc(2% - 1px), calc(3% - 2px), calc(4% - 3px))",
                             mlh("min(1%, calc(2% - 1px), calc(3% - 2px), calc(4% - 3px))", -1));
    TEST_ASSERT_EQUAL_STRING("-1 0 ", mbig("min(1%, calc(2% - 1px), calc(3% - 2px), calc(4% - 3px), calc(5% - 4px))"));
    TEST_ASSERT_EQUAL_STRING("-1 0 ", mlh("calc(min(1%, 1px) + min(2%, 2px) + min(3%, 3px))", -1));
    TEST_ASSERT_EQUAL_STRING("-1 0 ", mbig("min(calc(9% - 9px), max(calc(1% + 9px), min(calc(9% - 8px), "
                                           "max(calc(1% + 8px), calc(5% + 1px)))))"));
    /* Lot 112 (converted): comparable leaves go first, so these fit
     * (were -1 0 for the last two) */
    TEST_ASSERT_EQUAL_STRING("2 0 min(1%, 1px)", mlh("min(1%, 1px, 2%, 2px)", -1));
    TEST_ASSERT_EQUAL_STRING("2 0 min(1%, 1px)", mlh("min(1%, 1px, 2%, 2px, 3%)", -1));
    TEST_ASSERT_EQUAL_STRING("2 0 1%", mlh("min(1%, max(2%, min(3%, max(4%, calc(5% + 1px)))))", -1));
    TEST_ASSERT_EQUAL_STRING("-1 0 ", mlh("calc(min(10%, 1px) * 2px)", -1));
    TEST_ASSERT_EQUAL_STRING("-1 0 ", mlh("calc(2 / min(10%, 1px))", -1));
    TEST_ASSERT_EQUAL_STRING("-1 0 ", mlh("calc(min(10%, 1px) / min(10%, 1px))", -1));
    TEST_ASSERT_EQUAL_STRING("-1 0 ", mlh("calc(min(10%, 1px) / 0)", -1));
    TEST_ASSERT_EQUAL_STRING("-1 0 ", mlh("calc(min(10%, 1px) + 1)", -1));
    TEST_ASSERT_EQUAL_STRING("-1 0 ", mlh("calc(min(10%, 1px) * 3000)", -1));
    /* through the reader */
    TEST_ASSERT_EQUAL_STRING(" style=\"overflow-y:auto;height:min(calc(50% + 1px), 11px)\"",
                             b_of("#b { overflow: auto; height: calc(min(50%, 10px) + 1px) }"));
    TEST_ASSERT_EQUAL_STRING(" style=\"line-height:32px;overflow-y:auto;height:max(calc(10% + 20px), 52px)\"",
                             b_of("#b { overflow: auto; line-height: 2; height: calc(max(10%, 1lh) + 20px) }"));
    TEST_ASSERT_EQUAL_STRING("", b_of("#b { overflow: auto; height: min(1%, calc(2% - 1px), calc(3% - 2px), calc(4% - 3px), "
                                      "calc(5% - 4px)) }"));
    /* Lot 112 (converted, was ""): five arguments, two of them left */
    TEST_ASSERT_EQUAL_STRING(" style=\"overflow-y:auto;height:min(1%, 1px)\"",
                             b_of("#b { overflow: auto; height: min(1%, 1px, 2%, 2px, 3%) }"));
}

/* relative colours: the origin's missing components stay missing in the
 * analogous channels (CSS Color 5 4.1, CSS Color 4 12.2), so color-mix()
 * takes the other colour's */
static void test_l111_rel_none(void) {
    TEST_ASSERT_EQUAL_STRING("000000 255", col("rgb(from rgb(none 0 0) r g b)"));
    TEST_ASSERT_EQUAL_STRING("c80000 255", col("color-mix(in srgb, rgb(from rgb(none 0 0) r g b), rgb(200 0 0))"));
    /* inside a math function none is 0: not missing */
    TEST_ASSERT_EQUAL_STRING("7d0000 255", col("color-mix(in srgb, rgb(from rgb(none 0 0) calc(r + 50) g b), rgb(200 0 0))"));
    /* a left-out alpha is the origin's, missing too */
    TEST_ASSERT_EQUAL_STRING("050a0f 128", col("color-mix(in srgb, rgb(from rgb(10 20 30 / none) r g b), rgb(0 0 0 / 0.5))"));
    TEST_ASSERT_EQUAL_STRING("798940 255", col("color-mix(in oklch, oklch(from oklch(0.6 0.1 none) l c h), oklch(0.6 0.1 120))"));
    TEST_ASSERT_EQUAL_STRING("206020 255", col("color-mix(in hsl, hsl(from rgb(none 0 0) h s l), hsl(120 100% 50%))"));
    /* lab's a has no analogue in hwb: whiteness and blackness stay */
    TEST_ASSERT_EQUAL_STRING("7bbb7b 255", col("color-mix(in hwb, hwb(from lab(50 none 0) h w b), hwb(120 50% 0%))"));
}

/* opacity: calc(), var(), all, revert, revert-layer */
static void test_l111_opacity(void) {
    static const struct { const char *css; const char *out; } c[] = {
        {"#o { opacity: calc(0.25 + 0.5) }", "75"}, {"#o { opacity: min(0.2, 0.7) }", "20"},
        {"#o { opacity: calc(30% * 2) }", "60"}, {"#o { opacity: clamp(0, 2, 0.45) }", "45"},
        {"#o { opacity: 0.3; opacity: calc(0.5 + 10%) }", "30"}, {"#o { opacity: calc(-1) }", "0"},
        {"#o { --a: 0.4; opacity: var(--a) }", "40"}, {"#o { opacity: var(--n, 0.6) }", "60"},
        {"body { --a: 25% } #o { opacity: calc(var(--a) * 2) }", "50"},
        {"@layer a { #o { opacity: 0.2 } } @layer b { #o { opacity: 0.6; opacity: revert-layer } }", "20"},
        {"@layer a { #o { opacity: 0.2 } } #o { opacity: revert-layer }", "20"},
        {"#o { opacity: 0.4 } #b { opacity: 0.1 } #b { all: inherit }", "40"}};
    char want[64];
    int k;
    for (k = 0; k < (int)(sizeof(c) / sizeof(c[0])); k++) {
        prep(c[k].css);
        snprintf(want, sizeof(want), " style=\"opacity:%s%%\"", c[k].out);
        TEST_ASSERT_EQUAL_STRING(want, style_of(strstr(c[k].css, "all") ? "b" : "o"));
    }
    /* a math function of a length, an angle or a time is no opacity */
    prep("#o { opacity: 0.3; opacity: calc(0.5px); opacity: calc(1px - 1px); opacity: calc(2deg * 0.1); opacity: min(0.5s, 1s) }");
    TEST_ASSERT_EQUAL_STRING(" style=\"opacity:30%\"", style_of("o"));
    /* revert and all: initial leave opacity 1, not written (all writes
     * the initial values of the other properties the reader keeps) */
    prep("#o { opacity: 0.3; opacity: revert }");
    TEST_ASSERT_EQUAL_STRING("", style_of("o"));
    prep("#o { opacity: 0.3; all: initial }");
    TEST_ASSERT_EQUAL_STRING(" style=\"color:#000000;margin:0px 0px 0px 0px;padding:0px 0px 0px 0px\"", style_of("o"));
    prep("#o { opacity: 0.3; all: revert }");
    TEST_ASSERT_EQUAL_STRING("", style_of("o"));
    prep("@layer a { #o { opacity: 0.3 } } @layer b { #o { all: revert-layer } }");
    TEST_ASSERT_EQUAL_STRING(" style=\"opacity:30%\"", style_of("o"));
    prep("#o { opacity: 0.3 !important; all: initial }");
    TEST_ASSERT_EQUAL_STRING(" style=\"color:#000000;margin:0px 0px 0px 0px;padding:0px 0px 0px 0px;opacity:30%\"",
                             style_of("o"));
    /* a revert that rolls back to the style attribute: its value is kept
     * (the revert mark is cleared before the attribute is painted again) */
    snprintf(g_html, sizeof(g_html), "<html><head><style>@layer a { #o { opacity: revert-layer !important; "
             "color: revert-layer !important } }</style></head><body><div id=o style=\"opacity: 0.5; color: #102030\">"
             "o</div></body></html>");
    TEST_ASSERT_EQUAL(0, wf_site_prepare_ext(&g_d, g_html, (int)strlen(g_html), 640, 480, 0, 0));
    TEST_ASSERT_EQUAL_STRING(" style=\"color:#102030;opacity:50%\"", style_of("o"));
    /* the report's node line names the final opacity */
    prep("#o { opacity: 0.6 }");
    TEST_ASSERT_TRUE(wf_site_report(&g_d, g_rep, (int)sizeof(g_rep)) > 0);
    TEST_ASSERT_NOT_NULL(strstr(g_rep, " opacity 0.6000"));
    prep("#o { opacity: calc(0.125 * 2) }");
    TEST_ASSERT_TRUE(wf_site_report(&g_d, g_rep, (int)sizeof(g_rep)) > 0);
    TEST_ASSERT_NOT_NULL(strstr(g_rep, " opacity 0.2500"));
    prep("#o { opacity: 0.6; opacity: revert }");
    TEST_ASSERT_TRUE(wf_site_report(&g_d, g_rep, (int)sizeof(g_rep)) > 0);
    TEST_ASSERT_NULL(strstr(g_rep, " opacity 0."));
    prep("#o { opacity: 0.6; opacity: initial }");
    TEST_ASSERT_TRUE(wf_site_report(&g_d, g_rep, (int)sizeof(g_rep)) > 0);
    TEST_ASSERT_NULL(strstr(g_rep, " opacity "));
}

/* the body's percentages against html's size given as an expression,
 * and its widths against html's width (CSS 2 10.2, 10.5) */
static void test_l111_body(void) {
    prep("html { overflow: hidden; height: min(50%, 300px) } body { overflow: auto; height: 50% } " RD);
    TEST_ASSERT_EQUAL_STRING(" style=\"overflow-y:auto;height:120px\"", style_of("body"));
    prep("html { overflow: hidden; height: calc(50% + 2em) } body { overflow: auto; height: 50% } " RD);
    TEST_ASSERT_EQUAL_STRING(" style=\"overflow-y:auto;height:136px\"", style_of("body"));
    prep("html { overflow: hidden; height: clamp(100px, 10%, 200px) } body { overflow: auto; height: 33% } " RD);
    TEST_ASSERT_EQUAL_STRING(" style=\"overflow-y:auto;height:33px\"", style_of("body"));
    /* an html expression below 0 is 0 */
    prep("html { overflow: hidden; height: calc(10% - 100px) } body { overflow: auto; height: 50% } " RD);
    TEST_ASSERT_EQUAL_STRING(" style=\"overflow-y:auto;height:0px\"", style_of("body"));
    prep("html { overflow: hidden; width: 50% } body { overflow: auto; width: 50%; height: 100px } " RD);
    TEST_ASSERT_EQUAL_STRING(" style=\"overflow-y:auto;height:100px;overflow-x:auto;width:160px\"", style_of("body"));
    prep("html { overflow: hidden; width: calc(100% - 40px) } body { overflow: auto; width: 50%; height: 100px } " RD);
    TEST_ASSERT_EQUAL_STRING(" style=\"overflow-y:auto;height:100px;overflow-x:auto;width:300px\"", style_of("body"));
    prep("html { overflow: hidden; width: min(80%, 400px) } body { overflow: auto; max-width: 50%; height: 100px } " RD);
    TEST_ASSERT_EQUAL_STRING(" style=\"overflow-y:auto;height:100px;overflow-x:auto;max-width:200px\"", style_of("body"));
    /* html's width auto: Lot 112: the viewport's width, in px (was left
     * to the engine, which lays it out the same) */
    prep("html { overflow: hidden } body { overflow: auto; width: 25%; height: 100px } " RD);
    TEST_ASSERT_EQUAL_STRING(" style=\"overflow-y:auto;height:100px;overflow-x:auto;width:160px\"", style_of("body"));
}

int main(void) {
    unity_init();
    RUN_TEST(test_l111_lh_values);
    RUN_TEST(test_l111_lh_inherit);
    RUN_TEST(test_l111_lh_refused);
    RUN_TEST(test_l111_lh_edges);
    RUN_TEST(test_l111_math_api);
    RUN_TEST(test_l111_mixed);
    RUN_TEST(test_l111_rel_none);
    RUN_TEST(test_l111_opacity);
    RUN_TEST(test_l111_body);
    unity_print_results();
    unity_cleanup();
    return unity_stats.tests_failed == 0 ? 0 : 1;
}
