/* Lot 103 (US-031): hand-written cases next to the generated suites
 * (test_lot103_gamut.c, test_lot103_mix.c, test_lot103_hue.c,
 * test_lot103_sheet.c): gamut mapping (CSS Color 4 section 13.2.2),
 * color-mix() in the CSS Color 5 spaces with the hue methods, and the
 * analogous sets of missing components (CSS Color 4 section 12). Each
 * value below is worked out from the specs in docs/lot103_plan.md. */
#include "../../framework/unity.h"
#include <string.h>
#include <stdio.h>
#include "wf_sitecss.h"
#include "wf_cssprop.h"

static wf_site_t g_d;
static wf_cssprop_t g_p;
static char g_html[8192];
static char g_o[1400];
static char g_r[4][64];
static int g_ri;

static void prep(const char *css) {
    snprintf(g_html, sizeof(g_html), "<html><head><style>%s</style></head><body>"
             "<div id=o><p id=i>i</p></div><p id=q>q</p></body></html>", css);
    TEST_ASSERT_EQUAL(0, wf_site_prepare_ext(&g_d, g_html, (int)strlen(g_html), 640, 480, 0, 0));
}

static int node(const char *id) {
    int i;
    for (i = 0; i < g_d.nnodes; i++) if (!strcmp(g_d.node[i].id, id)) return i;
    return -1;
}

static const char *style_of(const char *id) {
    int i = node(id);
    g_o[0] = 0;
    if (i >= 0) wf_site_style_attr(&g_d, g_d.node[i].start, g_o, (int)sizeof(g_o));
    return g_o;
}

static const char *rc(const char *v) {
    unsigned int c = 0x12345678u;
    int a = -3;
    char *o = g_r[g_ri++ & 3];
    if (!wf_cssprop_color(v, (int)strlen(v), &c, &a)) return c == 0x12345678u && a == -3 ? "-" : "WRITTEN";
    snprintf(o, 64, "#%06x %d", c, a);
    return o;
}

static int syn(const char *s) {
    memset(&g_p, 0, sizeof(g_p));
    g_p.syntax = (unsigned char)wf_cssprop_syntax_parse(s, (int)strlen(s), &g_p);
    return g_p.syntax != 0;
}

static const char *ip(const char *a, const char *b, int t) {
    static char out[160];
    if (!syn("'<color>'")) return "BAD SYNTAX";
    if (wf_cssprop_interp(&g_p, a, b, t, out, (int)sizeof(out)) < 0) return "-";
    return out;
}

static int mt(const char *v) {
    if (!syn("'<color>'")) return -1;
    return wf_cssprop_match_prop(&g_p, v) != 0;
}

/* CSS Color 4 13.2.2: in-gamut colours are kept, L >= 1 is white, L <= 0
 * black, a clipped colour within the JND (deltaE OK 0.02) is kept, else
 * the OKLCh chroma is reduced */
static void test_l103_gamut(void) {
    TEST_ASSERT_EQUAL_STRING("#777777 255", rc("lab(50 0 0)"));
    TEST_ASSERT_EQUAL_STRING("#96a85e 255", rc("oklch(0.7 0.1 120)"));
    TEST_ASSERT_EQUAL_STRING("#ffffff 255", rc("oklch(1 0.3 30)"));
    TEST_ASSERT_EQUAL_STRING("#000000 255", rc("oklch(0 0.3 30)"));
    TEST_ASSERT_EQUAL_STRING("#ffffff 128", rc("oklab(1 0.2 -0.2 / 50%)"));
    /* linear 1.01 and -0.01: the clipped colour is within the JND */
    TEST_ASSERT_EQUAL_STRING("#ffffff 255", rc("color(srgb-linear 1.01 1 1)"));
    TEST_ASSERT_EQUAL_STRING("#00bcbc 255", rc("color(srgb-linear -0.01 0.5 0.5)"));
    /* display-p3 red: clipping (#ff0000) is 0.05 away in OKLab, beyond
     * the JND; the chroma at L 0.6487, hue 28.96 is reduced until the
     * clipped candidate is within it */
    TEST_ASSERT_EQUAL_STRING("#ff0b0c 255", rc("color(display-p3 1 0 0)"));
    TEST_ASSERT_EQUAL_STRING("#00fb29 255", rc("color(display-p3 0 1 0)"));
    TEST_ASSERT_EQUAL_STRING("#00f272 255", rc("color(rec2020 0 1 0)"));
    TEST_ASSERT_EQUAL_STRING("#ff5843 255", rc("oklch(0.7 0.4 30)"));
    TEST_ASSERT_EQUAL_STRING("#f20080 255", rc("lab(50 120 0)"));
    /* the colour space functions of sRGB keep their clamp */
    TEST_ASSERT_EQUAL_STRING("#ff0000 255", rc("color(srgb 1.2 0 0)"));
}

/* color-mix() in other spaces (CSS Color 5) */
static void test_l103_mix_spaces(void) {
    /* linear 0.5 of red and blue encodes to 187.5 */
    TEST_ASSERT_EQUAL_STRING("#bc00bc 255", rc("color-mix(in srgb-linear, red, blue)"));
    TEST_ASSERT_EQUAL_STRING("#bc00bc 255", rc("color-mix(in xyz, red, blue)"));
    TEST_ASSERT_EQUAL_STRING("#bc00bc 255", rc("color-mix(in xyz-d50, red, blue)"));
    /* oklab(0.5399 0.0957 -0.1062) (CSS Color 5 example) */
    TEST_ASSERT_EQUAL_STRING("#8c53a2 255", rc("color-mix(in oklab, red, blue)"));
    TEST_ASSERT_EQUAL_STRING("#9c00b7 255", rc("color-mix(in lab, red 30%, blue)"));
    TEST_ASSERT_EQUAL_STRING("#a21393 255", rc("color-mix(in rec2020, red, blue)"));
    /* the hue 330 of oklch, gamut-mapped */
    TEST_ASSERT_EQUAL_STRING("#b700be 255", rc("color-mix(in oklch, red, blue)"));
    /* weights under 100% scale the alpha: 40% of 255 */
    TEST_ASSERT_EQUAL_STRING("#8c53a2 102", rc("color-mix(in oklab, red 20%, blue 20%)"));
    /* premultiplied: transparent adds no colour; its hue is powerless */
    TEST_ASSERT_EQUAL_STRING("#ff0000 128", rc("color-mix(in hsl, red, transparent)"));
    TEST_ASSERT_EQUAL_STRING("#800080 255", rc("color-mix(in srgb, red, blue)"));
    TEST_ASSERT_EQUAL_STRING("#ff00ff 255", rc("COLOR-MIX(IN  HSL , RED , BLUE)"));
    /* invalid heads */
    TEST_ASSERT_EQUAL_STRING("-", rc("color-mix(in hsv, red, blue)"));
    TEST_ASSERT_EQUAL_STRING("-", rc("color-mix(in srgb longer hue, red, blue)"));
    TEST_ASSERT_EQUAL_STRING("-", rc("color-mix(in lab shorter hue, red, blue)"));
    TEST_ASSERT_EQUAL_STRING("-", rc("color-mix(in oklch longer, red, blue)"));
    TEST_ASSERT_EQUAL_STRING("-", rc("color-mix(in oklch hue longer, red, blue)"));
    TEST_ASSERT_EQUAL_STRING("-", rc("color-mix(in lch longer hue x, red, blue)"));
    TEST_ASSERT_EQUAL_STRING("-", rc("color-mix(in, red, blue)"));
    TEST_ASSERT_EQUAL_STRING("-", rc("color-mix(oklch, red, blue)"));
}

/* the hue methods (CSS Color 4 12.4): red 0, blue 240 */
static void test_l103_hue_methods(void) {
    TEST_ASSERT_EQUAL_STRING("#ff00ff 255", rc("color-mix(in hsl, red, blue)"));                  /* 300 */
    TEST_ASSERT_EQUAL_STRING("#ff00ff 255", rc("color-mix(in hsl shorter hue, red, blue)"));
    TEST_ASSERT_EQUAL_STRING("#00ff00 255", rc("color-mix(in hsl longer hue, red, blue)"));      /* 120 */
    TEST_ASSERT_EQUAL_STRING("#00ff00 255", rc("color-mix(in hsl increasing hue, red, blue)"));
    TEST_ASSERT_EQUAL_STRING("#ff00ff 255", rc("color-mix(in hsl decreasing hue, red, blue)"));
    TEST_ASSERT_EQUAL_STRING("#00ff00 255", rc("color-mix(in hwb longer hue, red, blue)"));
    /* 350 and 10: shorter through 0, longer through 180 */
    TEST_ASSERT_EQUAL_STRING(rc("lch(50 40 0)"), rc("color-mix(in lch shorter hue, lch(50 40 350), lch(50 40 10))"));
    TEST_ASSERT_EQUAL_STRING(rc("lch(50 40 180)"), rc("color-mix(in lch longer hue, lch(50 40 350), lch(50 40 10))"));
    TEST_ASSERT_EQUAL_STRING(rc("lch(50 40 0)"), rc("color-mix(in lch decreasing hue, lch(50 40 10), lch(50 40 350))"));
    TEST_ASSERT_EQUAL_STRING(rc("lch(50 40 180)"), rc("color-mix(in lch decreasing hue, lch(50 40 350), lch(50 40 10))"));
    TEST_ASSERT_EQUAL_STRING(rc("lch(50 40 180)"), rc("color-mix(in lch increasing hue, lch(50 40 10), lch(50 40 350))"));
    TEST_ASSERT_EQUAL_STRING("#b35979 255", rc("lch(50 40 0)"));
    TEST_ASSERT_EQUAL_STRING("#008875 255", rc("lch(50 40 180)"));
}

/* missing components by analogous sets, powerless hues */
static void test_l103_missing(void) {
    /* hsl: hue 120 carried, S (50 + 100) / 2, L 50 */
    TEST_ASSERT_EQUAL_STRING("#20df20 255", rc("color-mix(in hsl, hsl(none 50% 50%), hsl(120 100% 50%))"));
    TEST_ASSERT_EQUAL_STRING(rc("color-mix(in lch, lch(60 50 240), lch(60 50 240))"),
                             rc("color-mix(in lch, lch(50 40 none), lch(70 60 240))"));
    /* lightness carries from lab to lch; opponent a has no lch analogue */
    TEST_ASSERT_EQUAL_STRING(rc("color-mix(in lch, lab(80 20 20), lch(80 30 200))"),
                             rc("color-mix(in lch, lab(none 20 20), lch(80 30 200))"));
    TEST_ASSERT_EQUAL_STRING(rc("color-mix(in lch, lab(50 0 20), lch(80 30 200))"),
                             rc("color-mix(in lch, lab(50 none 20), lch(80 30 200))"));
    TEST_ASSERT_EQUAL_STRING("#b6ce97 255", rc("color-mix(in lch, lab(none 20 20), lch(80 30 200))"));
    TEST_ASSERT_EQUAL_STRING("#7ea883 255", rc("color-mix(in lch, lab(50 none 20), lch(80 30 200))"));
    /* srgb has no hue: the none of hsl() is 0 */
    TEST_ASSERT_EQUAL_STRING(rc("color-mix(in srgb, hsl(0 80% 50%), blue)"),
                             rc("color-mix(in srgb, hsl(none 80% 50%), blue)"));
    /* missing in both stays missing in the result */
    TEST_ASSERT_EQUAL_STRING(rc("color-mix(in lch, lch(55 40 90), lch(50 40 90))"),
                             rc("color-mix(in lch, color-mix(in lch, lch(50 40 none), lch(60 40 none)), lch(50 40 90))"));
    TEST_ASSERT_EQUAL_STRING(rc("color-mix(in srgb, lch(55 40 0), red)"),
                             rc("color-mix(in srgb, color-mix(in lch, lch(50 40 none), lch(60 40 none)), red)"));
    TEST_ASSERT_EQUAL_STRING(rc("oklch(0.6 0.1 0)"), rc("color-mix(in oklch, oklch(0.5 0.1 none), oklch(0.7 0.1 none))"));
    /* white has a powerless hue in oklch: blue's hue is taken */
    TEST_ASSERT_EQUAL_STRING(rc("color-mix(in oklch, oklch(1 0 264.052), blue)"), rc("color-mix(in oklch, white, blue)"));
    TEST_ASSERT_EQUAL_STRING("#74a3ff 255", rc("color-mix(in oklch, white, blue)"));
    TEST_ASSERT_EQUAL_STRING(rc("color-mix(in hwb, hwb(0 0% 20%), hwb(240 40% 0%))"),
                             rc("color-mix(in hwb, hwb(0 none 20%), hwb(240 40% 0%))"));
    /* a registered <color> takes the new mixes; interpolation keeps the
     * Lot 102 channels */
    TEST_ASSERT_EQUAL(1, mt("color-mix(in oklch longer hue, red, blue)"));
    TEST_ASSERT_EQUAL(1, mt("color-mix(in display-p3, red, hsl(none 50% 50%))"));
    TEST_ASSERT_EQUAL(0, mt("color-mix(in srgb longer hue, red, blue)"));
    /* magenta (the hsl mix) halfway to lime */
    TEST_ASSERT_EQUAL_STRING("rgb(128, 128, 128)", ip("color-mix(in hsl, red, blue)", "lime", 5000));
    /* the missing hue of hsl() is red (0) in sRGB, halfway to blue */
    TEST_ASSERT_EQUAL_STRING("rgb(128, 0, 128)", ip("hsl(none 100% 50%)", "blue", 5000));
}

/* mutation witnesses (each value from the model and the spec rules) */
static void test_l103_witnesses(void) {
    /* display-p3 components under the sRGB-curve threshold 0.0031308 are
     * encoded linearly (12.92 v) before they are mixed */
    TEST_ASSERT_EQUAL_STRING("#0a0a0b 255", rc("color-mix(in display-p3, rgb(8 9 8), rgb(12 12 13))"));
    /* hsl lightness (M + m) / 2 rounded half away from zero, not down */
    TEST_ASSERT_EQUAL_STRING("#fc9808 255",
                             rc("color-mix(in hsl, rgb(159.5 253.1 24.4), rgb(145.7 4.2 213.7) 30%)"));
    /* an lch chroma of 0.001 (at most 0.0015) has a powerless hue: the
     * other hue, 200, is taken (with hue 30 the shorter mean is 115) */
    TEST_ASSERT_EQUAL_STRING(rc("lch(50 20.0005 200)"), rc("color-mix(in lch, lch(50 0.001 30), lch(50 40 200))"));
    TEST_ASSERT_EQUAL_STRING("#498082 255", rc("color-mix(in lch, lch(50 0.001 30), lch(50 40 200))"));
    /* shorter: 185 - 0 > 180, so 0 becomes 360: 272.5 */
    TEST_ASSERT_EQUAL_STRING(rc("hsl(272.5 50% 50%)"), rc("color-mix(in hsl, hsl(0 50% 50%), hsl(185 50% 50%))"));
    TEST_ASSERT_EQUAL_STRING("#8540bf 255", rc("color-mix(in hsl, hsl(0 50% 50%), hsl(185 50% 50%))"));
    /* longer: 0 - 175 is in -180..0, so 0 becomes 360: 267.5 */
    TEST_ASSERT_EQUAL_STRING(rc("hsl(267.5 50% 50%)"),
                             rc("color-mix(in hsl longer hue, hsl(175 50% 50%), hsl(0 50% 50%))"));
    TEST_ASSERT_EQUAL_STRING("#7a40bf 255", rc("color-mix(in hsl longer hue, hsl(175 50% 50%), hsl(0 50% 50%))"));
    /* both alphas 0: the premultiplied components have nothing to divide,
     * they are 0 (black) */
    TEST_ASSERT_EQUAL_STRING("#000000 0", rc("color-mix(in oklab, transparent, rgb(255 0 0 / 0))"));
    TEST_ASSERT_EQUAL_STRING("#000000 0", rc("color-mix(in hsl, rgb(255 0 0 / 0), rgb(0 0 255 / 0))"));
    /* an alpha missing in both stays missing: the outer mix takes blue's
     * alpha 1 for the black inner colour (kept, it would be #0000ff 128) */
    TEST_ASSERT_EQUAL_STRING("#000080 255",
                             rc("color-mix(in srgb, color-mix(in oklch, rgb(0 0 0 / none), rgb(255 0 0 / none)), blue)"));
}

/* the reader paint */
static void test_l103_reader(void) {
    prep("#o { color: color-mix(in oklch, red, blue); background-color: color(display-p3 0 1 0) } "
         "#i { color: color-mix(in hsl longer hue, red, blue) } "
         "#q { background-color: color-mix(in lch, lch(50 40 none), lch(70 60 240)) }");
    TEST_ASSERT_EQUAL_STRING(" style=\"color:#b700be;background-color:#00fb29\"", style_of("o"));
    TEST_ASSERT_EQUAL_STRING(" style=\"color:#00ff00\"", style_of("i"));
    TEST_ASSERT_EQUAL_STRING(" style=\"background-color:#009fda\"", style_of("q"));
}

int main(void) {
    unity_init();
    RUN_TEST(test_l103_gamut);
    RUN_TEST(test_l103_mix_spaces);
    RUN_TEST(test_l103_hue_methods);
    RUN_TEST(test_l103_missing);
    RUN_TEST(test_l103_witnesses);
    RUN_TEST(test_l103_reader);
    unity_print_results();
    unity_cleanup();
    return unity_stats.tests_failed == 0 ? 0 : 1;
}
