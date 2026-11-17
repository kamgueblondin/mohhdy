/* Lot 109 (US-031): box scroller lengths with ex, ch and lh, and a
 * percentage inside calc()/min()/max()/clamp() passed on as the engine's
 * math (wf_cssmath.c wf_css_math_box, wf_sitecss.c); relative lab(),
 * lch(), hwb() and color(), and the unclamped channels of an origin
 * outside the sRGB gamut (wf_cssprop.c). Hand cases next to the generated
 * test_lot109_*.c; expected values follow docs/lot109_plan.md. */
#include "../../framework/unity.h"
#include <string.h>
#include <stdio.h>
#include "wf_cssmath.h"
#include "wf_cssprop.h"
#include "wf_sitecss.h"

static char g_o[512];
static wf_site_t g_d;
static char g_html[4096];

#define KF "@keyframes k { from { color: #000000 } to { color: #ffffff } } "
#define RD "#i { animation: k 1s linear; animation-timeline: scroll(nearest) } "
#define RX "#i { animation: k 1s linear; animation-timeline: scroll(nearest inline) } "

static void prep(const char *css) {
    snprintf(g_html, sizeof(g_html), "<html><head><style>" KF "%s</style></head><body><div id=o><div id=b>"
             "<p id=i>i</p></div></div></body></html>", css);
    TEST_ASSERT_EQUAL(0, wf_site_prepare_ext(&g_d, g_html, (int)strlen(g_html), 640, 480, 0, 0));
}

static const char *style_of(const char *id) {
    int i;
    g_o[0] = 0;
    for (i = 0; i < g_d.nnodes; i++)
        if (!strcmp(g_d.node[i].id, id)) wf_site_style_attr(&g_d, g_d.node[i].start, g_o, (int)sizeof(g_o));
    return g_o;
}

static const wf_site_node_t *node(const char *id) {
    int i;
    for (i = 0; i < g_d.nnodes; i++) if (!strcmp(g_d.node[i].id, id)) return &g_d.node[i];
    return &g_d.node[0];
}

static const char *col(const char *v) {
    unsigned int rgb = 0;
    int a = 0;
    if (!wf_cssprop_color(v, (int)strlen(v), &rgb, &a)) return "0";
    snprintf(g_o, sizeof(g_o), "%06x %d", rgb, a);
    return g_o;
}

/* wf_css_math_box itself: "r px out" */
static const char *mb(const char *s, int em, int cap) {
    static char out[80];
    int px = -7, r = wf_css_math_box(s, 0, 0, em, 16000, &px, out, cap);
    snprintf(g_o, sizeof(g_o), "%d %d %s", r, px, out);
    return g_o;
}

/* ex 5/16, ch 8/16, lh 10/16 of the element's font size */
static void test_l109_font_units(void) {
    prep("#b { overflow: auto; height: 10ex } " RD);
    TEST_ASSERT_EQUAL_STRING("~10ex", node("b")->htok);
    TEST_ASSERT_EQUAL_STRING(" style=\"overflow-y:auto;height:50px\"", style_of("b"));
    prep("#b { overflow: auto; height: 10ex; font-size: 24px } " RD);
    TEST_ASSERT_EQUAL_STRING(" style=\"font-size:24px;overflow-y:auto;height:75px\"", style_of("b"));
    prep("#b { overflow: auto; max-height: 2lh } " RD);
    TEST_ASSERT_EQUAL_STRING(" style=\"overflow-y:auto;max-height:20px\"", style_of("b"));
    prep("#b { overflow: auto; height: 3lh; min-height: 1.5lh } " RD);
    TEST_ASSERT_EQUAL_STRING(" style=\"overflow-y:auto;height:30px;min-height:15px\"", style_of("b"));
    prep("#b { overflow: auto; height: 0.5ex } " RD);       /* 2.5 px, rounded down */
    TEST_ASSERT_EQUAL_STRING(" style=\"overflow-y:auto;height:2px\"", style_of("b"));
    prep("#b { overflow: auto; width: 3ch } " RX);
    TEST_ASSERT_EQUAL_STRING(" style=\"overflow-x:auto;width:24px\"", style_of("b"));
    prep("#b { overflow: auto; width: 3ch; max-width: 2ch; font-size: 10px } " RX);
    TEST_ASSERT_EQUAL_STRING(" style=\"font-size:10px;overflow-x:auto;width:15px;max-width:10px\"", style_of("b"));
    /* in a math function, kept in the pool ("=N") and resolved against
     * the element's font size */
    prep("#b { overflow: auto; height: calc(2lh + 4px) } " RD);
    TEST_ASSERT_EQUAL('=', node("b")->htok[0]);
    TEST_ASSERT_EQUAL_STRING(" style=\"overflow-y:auto;height:24px\"", style_of("b"));
    prep("#b { overflow: auto; height: calc(2lh + 4px); font-size: 32px } " RD);
    TEST_ASSERT_EQUAL_STRING(" style=\"font-size:32px;overflow-y:auto;height:44px\"", style_of("b"));
    prep("#b { overflow: auto; height: max(1ex, 1ch, 1lh) } " RD);
    TEST_ASSERT_EQUAL_STRING(" style=\"overflow-y:auto;height:10px\"", style_of("b"));
    /* another unit is still kept unresolved */
    prep("#b { overflow: auto; height: 10cap } " RD);
    TEST_ASSERT_EQUAL_STRING("", node("b")->htok);
    TEST_ASSERT_EQUAL_STRING("", style_of("b"));
}

/* a percentage in a math function: the engine's expression */
static void test_l109_pct_math(void) {
    prep("#b { overflow: auto; height: calc(50% - 10px) } " RD);
    TEST_ASSERT_EQUAL('@', node("b")->htok[0]);
    TEST_ASSERT_EQUAL_STRING(" style=\"overflow-y:auto;height:calc(50% - 10px)\"", style_of("b"));
    prep("#b { overflow: auto; height: min(50%, 100px) } " RD);
    TEST_ASSERT_EQUAL_STRING(" style=\"overflow-y:auto;height:min(50%, 100px)\"", style_of("b"));
    prep("#b { overflow: auto; max-height: max(10%, 2em); font-size: 20px } " RD);
    TEST_ASSERT_EQUAL_STRING(" style=\"font-size:20px;overflow-y:auto;max-height:max(10%, 40px)\"", style_of("b"));
    prep("#b { overflow: auto; height: clamp(1ex, 20%, 2lh) } " RD);
    TEST_ASSERT_EQUAL_STRING(" style=\"overflow-y:auto;height:clamp(5px, 20%, 20px)\"", style_of("b"));
    prep("#b { overflow: auto; height: 10px; min-height: calc(25% + 1ch) } " RD);
    TEST_ASSERT_EQUAL_STRING(" style=\"overflow-y:auto;height:10px;min-height:calc(25% + 8px)\"", style_of("b"));
    /* Lot 112 (converted, was max(50%, 1px, 32px)): 32px covers 1px */
    prep("#b { overflow: auto; height: max(50%, 1px, 2em) } " RD);
    TEST_ASSERT_EQUAL_STRING(" style=\"overflow-y:auto;height:max(50%, 32px)\"", style_of("b"));
    prep("#b { overflow: auto; height: max(50%, calc(10% + 1px), 2em) } " RD);
    TEST_ASSERT_EQUAL_STRING(" style=\"overflow-y:auto;height:max(50%, calc(10% + 1px), 32px)\"", style_of("b"));
    /* integers, half away from zero; signs */
    prep("#b { overflow: auto; height: calc(10.5% + 0.5px) } " RD);
    TEST_ASSERT_EQUAL_STRING(" style=\"overflow-y:auto;height:calc(11% + 1px)\"", style_of("b"));
    prep("#b { overflow: auto; height: calc(10.4% - 20.5px) } " RD);
    TEST_ASSERT_EQUAL_STRING(" style=\"overflow-y:auto;height:calc(10% - 21px)\"", style_of("b"));
    prep("#b { overflow: auto; height: calc(-10% + 20px) } " RD);
    TEST_ASSERT_EQUAL_STRING(" style=\"overflow-y:auto;height:calc(-10% + 20px)\"", style_of("b"));
    prep("#b { overflow: auto; height: min(-10%, -5px) } " RD);
    TEST_ASSERT_EQUAL_STRING(" style=\"overflow-y:auto;height:min(-10%, -5px)\"", style_of("b"));
    prep("#b { overflow: auto; height: calc(0.4% + 10px) } " RD);
    TEST_ASSERT_EQUAL_STRING(" style=\"overflow-y:auto;height:calc(0% + 10px)\"", style_of("b"));
    /* percentages alone: one part */
    prep("#b { overflow: auto; height: calc(50% * 2) } " RD);
    TEST_ASSERT_EQUAL_STRING(" style=\"overflow-y:auto;height:100%\"", style_of("b"));
    prep("#b { overflow: auto; height: calc(2 * (10% + 5px)) } " RD);
    TEST_ASSERT_EQUAL_STRING(" style=\"overflow-y:auto;height:calc(20% + 10px)\"", style_of("b"));
    prep("#b { overflow: auto; height: calc(50% / 4) } " RD);
    TEST_ASSERT_EQUAL_STRING(" style=\"overflow-y:auto;height:13%\"", style_of("b"));
    prep("#b { overflow: auto; height: max(10%, 30%) } " RD);
    TEST_ASSERT_EQUAL_STRING(" style=\"overflow-y:auto;height:30%\"", style_of("b"));
    prep("#b { overflow: auto; height: min(30%, 10%, 20%) } " RD);
    TEST_ASSERT_EQUAL_STRING(" style=\"overflow-y:auto;height:10%\"", style_of("b"));
    /* no percentage left: a px value ("=N") */
    prep("#b { overflow: auto; height: calc(50% - 50% + 7px) } " RD);
    TEST_ASSERT_EQUAL('=', node("b")->htok[0]);
    TEST_ASSERT_EQUAL_STRING(" style=\"overflow-y:auto;height:7px\"", style_of("b"));
    prep("#b { overflow: auto; height: min(0px, 50%) } " RD);   /* zero counts as either part */
    TEST_ASSERT_EQUAL_STRING(" style=\"overflow-y:auto;height:0px\"", style_of("b"));
    /* widths too */
    prep("#b { overflow: auto; width: calc(100% - 2em) } " RX);
    TEST_ASSERT_EQUAL_STRING(" style=\"overflow-x:auto;width:calc(100% - 32px)\"", style_of("b"));
    prep("#b { overflow: auto; width: 50%; max-width: min(90%, 30ch) } " RX);
    TEST_ASSERT_EQUAL_STRING(" style=\"overflow-x:auto;width:50%;max-width:min(90%, 240px)\"", style_of("b"));
}

/* what does not pass: bounds, the expression's size (Lot 111: a mixed
 * function in an arithmetic or nested passes, see the end) */
static void test_l109_pct_refused(void) {
    static const char *const bad[] = {"calc(3000px + 1%)", "calc(2001% + 1px)",
                                      "min(50%, 2)", "calc(50% * 2%)", "calc(50% + 1)",
                                      "max(10%, 1px) * 2", "calc(10% / 0)", "clamp(10%, 1px)", 0};
    int k;
    char css[160];
    for (k = 0; bad[k]; k++) {
        snprintf(css, sizeof(css), "#b { overflow: auto; height: %s } " RD, bad[k]);
        prep(css);
        TEST_ASSERT_EQUAL_STRING("", node("b")->htok);
        TEST_ASSERT_EQUAL_STRING("", style_of("b"));
    }
    /* 63 characters pass, 64 do not (WF_SITE_BOXEX) */
    prep("#b { overflow: auto; height: clamp(calc(1% - 1999px), calc(1% + 1999px), calc(10% - 1998px)) } " RD);
    TEST_ASSERT_EQUAL_STRING(" style=\"overflow-y:auto;height:clamp(calc(1% - 1999px), calc(1% + 1999px), calc(10% - 1998px))\"",
                             style_of("b"));
    prep("#b { overflow: auto; height: clamp(calc(1% - 1999px), calc(1% + 1999px), calc(100% - 1998px)) } " RD);
    TEST_ASSERT_EQUAL_STRING("", style_of("b"));
    /* 63 characters at 16 px (124ch is 992px) and 64 at the element's
     * 20 px (1240px): kept as a token, not passed */
    prep("#b { overflow: auto; height: clamp(calc(1% - 124ch), calc(1% + 1999px), calc(100% - 1998px)); font-size: 20px } " RD);
    TEST_ASSERT_EQUAL('@', node("b")->htok[0]);
    TEST_ASSERT_EQUAL_STRING(" style=\"font-size:20px\"", style_of("b"));
    prep("#b { overflow: auto; height: clamp(calc(1% - 124ch), calc(1% + 1999px), calc(100% - 1998px)) } " RD);
    TEST_ASSERT_EQUAL_STRING(" style=\"overflow-y:auto;height:clamp(calc(1% - 992px), calc(1% + 1999px), calc(100% - 1998px))\"",
                             style_of("b"));
    /* the literal bound: 2000 passes, 2000.5 rounds past it */
    prep("#b { overflow: auto; height: calc(2000% - 2000px) } " RD);
    TEST_ASSERT_EQUAL_STRING(" style=\"overflow-y:auto;height:calc(2000% - 2000px)\"", style_of("b"));
    prep("#b { overflow: auto; height: calc(1% - 2000.5px) } " RD);
    TEST_ASSERT_EQUAL_STRING("", style_of("b"));
    /* a percentage token of a font size against 16 px at parse time and
     * the element's size later: min(50%, calc(1em - 16px)) has no px
     * part at 16 px ("=N", 0 px) but a mixed one at 20 px */
    prep("#b { overflow: auto; height: min(50%, calc(1em - 16px)); font-size: 20px } " RD);
    TEST_ASSERT_EQUAL('=', node("b")->htok[0]);
    TEST_ASSERT_EQUAL_STRING(" style=\"font-size:20px;overflow-y:auto;height:min(50%, 4px)\"", style_of("b"));
    /* Lot 111 (converted from the list above): the sum goes into the
     * arguments, a nested min() of the same kind is flattened */
    prep("#b { overflow: auto; height: calc(min(50%, 10px) + 1px) } " RD);
    TEST_ASSERT_EQUAL_STRING(" style=\"overflow-y:auto;height:min(calc(50% + 1px), 11px)\"", style_of("b"));
    /* Lot 112 (converted, was min(50%, 1px, 2px)): 1px covers 2px */
    prep("#b { overflow: auto; height: min(min(50%, 1px), 2px) } " RD);
    TEST_ASSERT_EQUAL_STRING(" style=\"overflow-y:auto;height:min(50%, 1px)\"", style_of("b"));
    prep("#b { overflow: auto; height: min(min(50%, 1px), calc(2px - 10%)) } " RD);
    TEST_ASSERT_EQUAL_STRING(" style=\"overflow-y:auto;height:min(50%, 1px, calc(-10% + 2px))\"", style_of("b"));
}

/* scroll-padding: ex, ch, lh resolve; a percentage in math is auto */
static void test_l109_padding(void) {
    prep("#o { scroll-padding-top: 2ex; scroll-padding-bottom: calc(1lh + 1px); scroll-padding-left: calc(10% + 1ch) }");
    TEST_ASSERT_EQUAL_STRING("~2ex", node("o")->sptok[0]);
    TEST_ASSERT_EQUAL('=', node("o")->sptok[1][0]);
    TEST_ASSERT_EQUAL('@', node("o")->sptok[2][0]);
}

/* the box mode alone; the font-size path keeps refusing ex, ch, lh */
static void test_l109_math_api(void) {
    char m[16];
    int px = 5;
    TEST_ASSERT_EQUAL_STRING("1 10000 ", mb("2EX", 16000, 80));
    TEST_ASSERT_EQUAL_STRING("1 10000 ", mb("1Lh", 16000, 80));
    TEST_ASSERT_EQUAL_STRING("1 8000 ", mb("1cH", 16000, 80));
    TEST_ASSERT_EQUAL_STRING("1 8000 ", mb("1Ch", 16000, 80));
    TEST_ASSERT_EQUAL_STRING("1 -10000 ", mb("-2ex", 16000, 80));
    TEST_ASSERT_EQUAL_STRING("1 3125 ", mb("1ex", 10000, 80));     /* 10 * 5 / 16 */
    TEST_ASSERT_EQUAL_STRING("1 6250 ", mb("1lh", 10000, 80));
    TEST_ASSERT_EQUAL_STRING("1 9 ", mb("0.001ex", 30000, 80));    /* 30 * 5 / 16 thousandths, cut */
    TEST_ASSERT_EQUAL_STRING("1 0 ", mb("0", 16000, 80));
    TEST_ASSERT_EQUAL_STRING("0 0 ", mb("", 16000, 80));
    TEST_ASSERT_EQUAL_STRING("-1 0 ", mb("5", 16000, 80));
    TEST_ASSERT_EQUAL_STRING("-1 0 ", mb("2ex2", 16000, 80));
    TEST_ASSERT_EQUAL_STRING("-1 0 ", mb("2em", 0, 80));
    TEST_ASSERT_EQUAL_STRING("2 0 50%", mb("50%", 16000, 80));
    TEST_ASSERT_EQUAL_STRING("2 0 calc(50% - 10px)", mb("calc(50% - 10px)", 16000, 80));
    TEST_ASSERT_EQUAL_STRING("-1 0 ", mb("calc(50% - 10px)", 16000, 16));     /* 16 bytes: too small */
    TEST_ASSERT_EQUAL_STRING("2 0 calc(50% - 10px)", mb("calc(50% - 10px)", 16000, 17));
    /* a mixed min()/max() in a product or a quotient (Lot 111, converted
     * from -1: the factor goes into the arguments) */
    TEST_ASSERT_EQUAL_STRING("2 0 min(20%, 2px)", mb("calc(min(10%, 1px) * 2)", 16000, 80));
    TEST_ASSERT_EQUAL_STRING("2 0 max(20%, 2px)", mb("calc(2 * max(10%, 1px))", 16000, 80));
    TEST_ASSERT_EQUAL_STRING("2 0 max(5%, 1px)", mb("calc(max(10%, 1px) / 2)", 16000, 80));
    TEST_ASSERT_EQUAL(-1, wf_css_math_box(0, 0, 0, 16000, 16000, &px, m, 16));
    TEST_ASSERT_EQUAL(-1, wf_css_math_box("1px", 0, 0, 16000, 16000, &px, m, 0));
    TEST_ASSERT_EQUAL(-1, wf_css_math_box("1px", 0, 0, 16000, 0, &px, m, 16));
    TEST_ASSERT_EQUAL(-1, wf_css_math_box("1px", 0, 0, 10000001, 16000, &px, m, 16));
    TEST_ASSERT_EQUAL(1, wf_css_math_box("1px", 0, 0, 10000000, 10000000, &px, m, 16));
    TEST_ASSERT_EQUAL(-1, wf_css_math("calc(2ex)", 0, 0, m, (int)sizeof(m)));
    TEST_ASSERT_EQUAL(-1, wf_css_math_px("2ex", 0, 0, 16000, 16000, &px));
    TEST_ASSERT_EQUAL(-1, wf_css_math_px("min(1lh, 2px)", 0, 0, 16000, 16000, &px));
    TEST_ASSERT_EQUAL(1, wf_css_math_px("calc(50% + 0px)", 0, 0, 16000, 16000, &px));
    TEST_ASSERT_EQUAL(8000, px);                               /* % of the font there */
    TEST_ASSERT_EQUAL(-1, wf_css_math("min(50%, 10px)", 0, 0, m, (int)sizeof(m)));
}

/* relative lab(), lch(), hwb() and color() */
static void test_l109_rel_spaces(void) {
    TEST_ASSERT_EQUAL_STRING("ff0000 255", col("hwb(from red h w b)"));
    TEST_ASSERT_EQUAL_STRING("00ff00 255", col("hwb(from red calc(h + 120) w b)"));
    TEST_ASSERT_EQUAL_STRING("808080 255", col("hwb(from #808080 calc(h + 90) w b)"));     /* a grey: h 0 */
    TEST_ASSERT_EQUAL_STRING("e27e4b 128", col("hwb(from rgb(200 100 50) h calc(w + 10) calc(b - 10) / 0.5)"));
    TEST_ASSERT_EQUAL_STRING("ff0000 255", col("lab(from red l a b)"));
    TEST_ASSERT_EQUAL_STRING("0082a9 255", col("lab(from lab(50 20 -30) l calc(a * -1) b)"));
    TEST_ASSERT_EQUAL_STRING("259dcd 255", col("lch(from lch(60 40 120) l c calc(h + 120))"));
    TEST_ASSERT_EQUAL_STRING("808080 255", col("lch(from #808080 l c calc(h + 120))"));    /* powerless hue */
    TEST_ASSERT_EQUAL_STRING("8f0000 255", col("lch(from red calc(l / 2) c h)"));
    TEST_ASSERT_EQUAL_STRING("ff0000 255", col("lch(from lab(from red l a b) l c h)"));
    TEST_ASSERT_EQUAL_STRING("ff0000 255", col("color(from red srgb r g b)"));
    TEST_ASSERT_EQUAL_STRING("800000 255", col("color(from red srgb calc(r / 2) g b)"));
    TEST_ASSERT_EQUAL_STRING("bc0000 255", col("color(from red srgb-linear calc(r / 2) g b)"));
    TEST_ASSERT_EQUAL_STRING("cf004c 255", col("color(from red xyz x calc(y / 2) z)"));
    TEST_ASSERT_EQUAL_STRING("ff0000 64", col("color(from red xyz-d50 x y z / 0.25)"));
    TEST_ASSERT_EQUAL_STRING("92431a 255", col("color(from green rec2020 g r b)"));
    TEST_ASSERT_EQUAL_STRING("ff0000 255", col("COLOR(FROM red SRGB r g b)"));
    TEST_ASSERT_EQUAL(1, wf_cssprop_match(WF_CSSPROP_COLOR, "color(from red display-p3 r g b)"));
    TEST_ASSERT_EQUAL(1, wf_cssprop_match(WF_CSSPROP_COLOR, "lab(from red l a b)"));
    /* refused: a space color() has not, the other space's keywords, the
     * count of arguments, no current colour */
    TEST_ASSERT_EQUAL_STRING("0", col("color(from red lab l a b)"));
    TEST_ASSERT_EQUAL_STRING("0", col("color(from red hsl h s l)"));
    TEST_ASSERT_EQUAL_STRING("0", col("color(from red oklab l a b)"));
    TEST_ASSERT_EQUAL_STRING("0", col("color(from red xyz r g b)"));
    TEST_ASSERT_EQUAL_STRING("0", col("color(from red srgb x y z)"));
    TEST_ASSERT_EQUAL_STRING("0", col("color(from red srgb r g)"));
    TEST_ASSERT_EQUAL_STRING("0", col("color(from red srgb r g b b)"));
    TEST_ASSERT_EQUAL_STRING("0", col("color(from red srgb r g b / alpha / 1)"));
    TEST_ASSERT_EQUAL_STRING("0", col("color(from red srgb r g / b alpha)"));
    TEST_ASSERT_EQUAL_STRING("0", col("color(from red)"));
    TEST_ASSERT_EQUAL_STRING("0", col("hwba(from red h w b)"));
    TEST_ASSERT_EQUAL_STRING("0", col("lab(from red l a b, 1)"));
    TEST_ASSERT_EQUAL_STRING("0", col("hwb(from red h s l)"));
    TEST_ASSERT_EQUAL_STRING("0", col("lch(from red l a b)"));
    TEST_ASSERT_EQUAL_STRING("0", col("color(from currentcolor srgb r g b)"));
    /* seven tokens, six for the others */
    TEST_ASSERT_EQUAL_STRING("0", col("color(from red srgb r g b / 1 2)"));
    TEST_ASSERT_EQUAL_STRING("0", col("color(from red srgb r g b 1 2)"));
    TEST_ASSERT_EQUAL_STRING("0", col("rgb(from red r g b / 1 2)"));
    TEST_ASSERT_EQUAL(0, wf_cssprop_match(WF_CSSPROP_COLOR, "color(from red lab l a b)"));
}

/* an origin outside the sRGB gamut keeps its unclamped channels */
static void test_l109_rel_unclamped(void) {
    /* display-p3 red: sRGB 1.093 -0.227 -0.150, so r 278.7 */
    TEST_ASSERT_EQUAL_STRING("ef0000 255", col("rgb(from color(display-p3 1 0 0) calc(r - 40) g b)"));
    TEST_ASSERT_EQUAL_STRING("ff0000 255", col("rgb(from color(display-p3 1 0 0) r g b)"));
    TEST_ASSERT_EQUAL_STRING("fd0000 255", col("color(from color(display-p3 1 0 0) srgb calc(r - 0.1) g b)"));
    TEST_ASSERT_EQUAL_STRING("dd491b 255", col("rgb(from lab(100 100 100) calc(r / 2) calc(g / 2) calc(b / 2))"));
    /* display-p3 green: sRGB g 1.018 (259.7), r -0.512 */
    TEST_ASSERT_EQUAL_STRING("00c800 255", col("rgb(from color(display-p3 0 1 0) r calc(g - 60) calc(b + 40))"));
    TEST_ASSERT_EQUAL_STRING("00ff00 255", col("rgb(from color(display-p3 0 1 0) calc(r + 100) g b)"));
    /* an origin inside the gamut: its channels */
    TEST_ASSERT_EQUAL_STRING("7b6caa 255", col("rgb(from lab(50 20 -30) calc(r - 10) g b)"));
    /* color(srgb) keeps its own clamping (no linear values) */
    TEST_ASSERT_EQUAL_STRING("e60000 255", col("rgb(from color(srgb 1.5 0 0) calc(r - 25) g b)"));
}

/* hsl() and hwb() from an origin outside the gamut: from its unclamped
 * channels; a negative saturation (lightness over 100%) turns the hue by
 * 180 deg and is made positive (CSS Color 4 sample code) */
static void test_l109_rel_hsl_unclamped(void) {
    /* display-p3 red: hsl(356.5 152.3% 43.3%), hwb(356.5 -22.7% -9.3%) */
    TEST_ASSERT_EQUAL_STRING("dd000d 255", col("hsl(from color(display-p3 1 0 0) h s l)"));
    TEST_ASSERT_EQUAL_STRING("ff000f 255", col("hsl(from color(display-p3 1 0 0) h 100% 50%)"));
    TEST_ASSERT_EQUAL_STRING("ff000f 255", col("hwb(from color(display-p3 1 0 0) h w b)"));
    TEST_ASSERT_EQUAL_STRING("00ff00 255", col("hwb(from color(display-p3 0 1 0) calc(h - 7.88) 0% 0%)"));
    /* oklab(1 0.2 -0.1): lightness 105%, the hue 309.8 turned to 129.8,
     * the saturation 597.9% (not 0, not negative) */
    TEST_ASSERT_EQUAL_STRING("40bf54 255", col("hsl(from oklab(1 0.2 -0.1) h 50% 50%)"));
    TEST_ASSERT_EQUAL_STRING("00ff29 255", col("hwb(from oklab(1 0.2 -0.1) h 0% 0%)"));
    TEST_ASSERT_EQUAL_STRING("33cc4c 255", col("hsl(from oklab(1 0.2 -0.1) h calc(s / 10) 50%)"));
    TEST_ASSERT_EQUAL_STRING("b5bf40 255", col("hsl(from oklab(1 0.2 -0.1) calc(h / 2) 50% 50%)"));
    /* a grey past the gamut has no hue to turn: 0 */
    TEST_ASSERT_EQUAL_STRING("bf4040 255", col("hsl(from color(srgb-linear -0.1 -0.1 -0.1) h 50% 50%)"));
    TEST_ASSERT_EQUAL_STRING("bf4040 255", col("hsl(from color(srgb-linear 1.2 1.2 1.2) h 50% 50%)"));
}

int main(void) {
    unity_init();
    RUN_TEST(test_l109_font_units);
    RUN_TEST(test_l109_pct_math);
    RUN_TEST(test_l109_pct_refused);
    RUN_TEST(test_l109_padding);
    RUN_TEST(test_l109_math_api);
    RUN_TEST(test_l109_rel_spaces);
    RUN_TEST(test_l109_rel_unclamped);
    RUN_TEST(test_l109_rel_hsl_unclamped);
    unity_print_results();
    unity_cleanup();
    return unity_stats.tests_failed == 0 ? 0 : 1;
}
