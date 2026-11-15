/* Lot 101 (US-031): hand-written cases next to the generated suites
 * (test_lot101_color.c, test_lot101_mix.c, test_lot101_tl.c,
 * test_lot101_paint.c, test_lot101_scroll.c): lab(), lch(), oklab(),
 * oklch() and color() (CSS Color 4 sections 9 and 10), currentcolor in
 * the reader paint, the scroll and view timeline properties and the
 * scroll timeline of the root element (CSS Scroll-driven Animations 1),
 * the summary line and the API guards. Each value below is worked out
 * from the specs in docs/lot101_plan.md; the witnesses on the grey axis
 * have a closed form (a grey stays grey through every matrix: the
 * channel is the sRGB curve of the linear value). */
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

#define KB "@keyframes k { from { color: #000000 } to { color: #ffffff } } "

static void prep(const char *css, int ms, int pm) {
    wf_site_set_time(ms);
    wf_site_set_scroll(pm);
    snprintf(g_html, sizeof(g_html), "<html><head><style>%s</style></head><body>"
             "<div id=o><p id=i>i</p></div><p id=q>q</p></body></html>", css);
    TEST_ASSERT_EQUAL(0, wf_site_prepare_ext(&g_d, g_html, (int)strlen(g_html), 640, 480, 0, 0));
    wf_site_set_time(0);
    wf_site_set_scroll(0);
}

/* a whole document (the scroll position only) */
static void prep_doc(const char *html, int pm) {
    wf_site_set_scroll(pm);
    TEST_ASSERT_EQUAL(0, wf_site_prepare_ext(&g_d, html, (int)strlen(html), 640, 480, 0, 0));
    wf_site_set_scroll(0);
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

static const char *anim_text(void) {
    if (!wf_site_anim_text(&g_d, g_o, (int)sizeof(g_o))) return "(empty)";
    return g_o;
}

/* the reader colour of v as "#rrggbb alpha", "-" when refused (four
 * buffers in turn, so two results can be compared) */
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
    if (wf_cssprop_interp(&g_p, a, b, t, out, (int)sizeof(out)) <= 0) return "(none)";
    return out;
}

/* lab() and lch(): CIE Lab with the D50 white (CSS Color 4 section 9) */
static void test_l101_lab(void) {
    /* L 50: Y = (66 / 116)^3 = 0.184187, the sRGB curve 0.466327, 118.913 */
    TEST_ASSERT_EQUAL_STRING("#777777 255", rc("lab(50 0 0)"));
    TEST_ASSERT_EQUAL_STRING("#777777 255", rc("lab(50% 0 0)"));
    TEST_ASSERT_EQUAL_STRING("#777777 255", rc("lch(50 0 0)"));
    TEST_ASSERT_EQUAL_STRING("#777777 255", rc("lch(50 0 123deg)"));     /* no chroma: the hue does not count */
    TEST_ASSERT_EQUAL_STRING("#777777 128", rc("lab(50 0 0 / 0.5)"));
    TEST_ASSERT_EQUAL_STRING("#777777 64", rc("LAB(50 0 0 / 25%)"));
    TEST_ASSERT_EQUAL_STRING("#ffffff 255", rc("lab(100 0 0)"));
    TEST_ASSERT_EQUAL_STRING("#000000 255", rc("lab(0 0 0)"));
    /* L is clamped to 0..100 */
    TEST_ASSERT_EQUAL_STRING("#ffffff 255", rc("lab(150 0 0)"));
    TEST_ASSERT_EQUAL_STRING("#000000 255", rc("lab(-5 0 0)"));
    /* sRGB red, written in Lab (CSS Color 4 example), lands on red */
    TEST_ASSERT_EQUAL_STRING("#ff0000 255", rc("lab(54.29 80.82 69.88)"));
    /* the percentage references: a and b 125, C 150 */
    TEST_ASSERT_EQUAL_STRING(rc("lab(60 125 -62.5)"), rc("lab(60 100% -50%)"));
    TEST_ASSERT_EQUAL_STRING(rc("lch(60 150 30)"), rc("lch(60 100% 30)"));
    /* the hue: C cos H and C sin H */
    TEST_ASSERT_EQUAL_STRING(rc("lab(50 0 30)"), rc("lch(50 30 90)"));
    TEST_ASSERT_EQUAL_STRING(rc("lab(50 -30 0)"), rc("lch(50 30 180deg)"));
    TEST_ASSERT_EQUAL_STRING(rc("lab(50 -30 0)"), rc("lch(50 30 0.5turn)"));
    TEST_ASSERT_EQUAL_STRING(rc("lab(50 0 -30)"), rc("lch(50 30 -90)"));
    TEST_ASSERT_EQUAL_STRING(rc("lab(50 0 -30)"), rc("lch(50 30 300grad)"));
    /* a negative chroma is 0 */
    TEST_ASSERT_EQUAL_STRING("#777777 255", rc("lch(50 -20 40)"));
    /* a, b and C are kept within 10000 */
    TEST_ASSERT_EQUAL_STRING(rc("lab(50 10000 0)"), rc("lab(50 20000 0)"));
    /* refused: commas, counts, units (Lot 102, converted: none is 0) */
    TEST_ASSERT_EQUAL_STRING("-", rc("lab(50, 0, 0)"));
    TEST_ASSERT_EQUAL_STRING("-", rc("lab(50 0)"));
    TEST_ASSERT_EQUAL_STRING("-", rc("lab(50 0 0 0)"));
    TEST_ASSERT_EQUAL_STRING("#000000 255", rc("lab(none 0 0)"));
    TEST_ASSERT_EQUAL_STRING("-", rc("lab(50px 0 0)"));
    TEST_ASSERT_EQUAL_STRING("-", rc("lch(50 30deg 90)"));
    TEST_ASSERT_EQUAL_STRING("-", rc("lch(50 30 90px)"));
    TEST_ASSERT_EQUAL_STRING("-", rc("lab(50 0 0 / 1 / 1)"));
    TEST_ASSERT_EQUAL_STRING("-", rc("laba(50 0 0)"));
    TEST_ASSERT_EQUAL_STRING("-", rc("lab(50 0 0"));
    TEST_ASSERT_EQUAL_STRING("-", rc("lab()"));
    TEST_ASSERT_EQUAL_STRING("-", rc("lab (50 0 0)"));
}

/* oklab() and oklch() (CSS Color 4 section 9.2) */
static void test_l101_oklab(void) {
    /* L 0.5: l = m = s = 0.125, the D65 white times 0.125, the sRGB curve
     * 0.388574, 99.086 */
    TEST_ASSERT_EQUAL_STRING("#636363 255", rc("oklab(0.5 0 0)"));
    TEST_ASSERT_EQUAL_STRING("#636363 255", rc("oklab(50% 0 0)"));
    TEST_ASSERT_EQUAL_STRING("#636363 255", rc("oklch(0.5 0 200)"));
    TEST_ASSERT_EQUAL_STRING("#ffffff 255", rc("oklab(1 0 0)"));
    TEST_ASSERT_EQUAL_STRING("#ffffff 255", rc("oklab(2 0 0)"));         /* L clamped to 0..1 */
    TEST_ASSERT_EQUAL_STRING("#000000 255", rc("oklch(0 0 0)"));
    /* sRGB red in Oklab (CSS Color 4 example) */
    TEST_ASSERT_EQUAL_STRING("#ff0000 255", rc("oklab(0.627955 0.224863 0.125846)"));
    /* a, b and C percentages of 0.4 */
    TEST_ASSERT_EQUAL_STRING(rc("oklab(0.6 0.4 -0.2)"), rc("oklab(0.6 100% -50%)"));
    TEST_ASSERT_EQUAL_STRING(rc("oklch(0.6 0.2 40)"), rc("oklch(0.6 50% 40)"));
    TEST_ASSERT_EQUAL_STRING(rc("oklab(0.6 0 -0.1)"), rc("oklch(0.6 0.1 270)"));
    TEST_ASSERT_EQUAL_STRING(rc("oklab(0.5 10 0)"), rc("oklab(0.5 50 0)"));   /* within 10 */
    /* a -10 and b -10 (kept within 10) */
    TEST_ASSERT_EQUAL_STRING(rc("oklab(0.5 -10 -10)"), rc("oklab(0.5 -40 -20)"));
    /* Lot 103, converted (was blue, the clipped a -10 b -10 at L 0): gamut
     * mapping makes L 0 black (CSS Color 4 13.2.2) */
    TEST_ASSERT_EQUAL_STRING("#000000 255", rc("oklab(0 -40 -20)"));
    TEST_ASSERT_EQUAL_STRING("#636363 128", rc("OKLCH(0.5 0 0 / 50%)"));
    TEST_ASSERT_EQUAL_STRING("-", rc("oklab(0.5, 0, 0)"));
    TEST_ASSERT_EQUAL_STRING("-", rc("oklch(0.5 0.1)"));
    TEST_ASSERT_EQUAL_STRING("-", rc("ok lab(0.5 0 0)"));
}

/* color() (CSS Color 4 section 10): nine spaces */
static void test_l101_color_fn(void) {
    /* srgb: the channels themselves: 127.5, 63.75 and 255, alpha 127.5 */
    TEST_ASSERT_EQUAL_STRING("#8040ff 128", rc("color(srgb 0.5 0.25 1 / 50%)"));
    TEST_ASSERT_EQUAL_STRING("#8040ff 255", rc("color(srgb 50% 25% 100%)"));
    /* 1.7647% of 255 is 4.4999985, rounded once to 4.5 then to 5 (a
     * floor gives 4.4999, painted 4) */
    TEST_ASSERT_EQUAL_STRING("#050000 255", rc("color(srgb 1.7647% 0 0)"));
    TEST_ASSERT_EQUAL_STRING("#ff0000 255", rc("color(srgb 2 -1 0)"));       /* clamped */
    /* srgb-linear 0.5: the sRGB curve 0.735357, 187.516 */
    TEST_ASSERT_EQUAL_STRING("#bcbcbc 255", rc("color(srgb-linear 0.5 0.5 0.5)"));
    /* a98-rgb 0.5: 0.5^(563/256) = 0.217638, 128.518: a floor would give
     * 128 */
    TEST_ASSERT_EQUAL_STRING("#818181 255", rc("color(a98-rgb 0.5 0.5 0.5)"));
    /* prophoto-rgb 0.5: 0.5^1.8 = 0.287175 (D50, through Bradford), 145.938 */
    TEST_ASSERT_EQUAL_STRING("#929292 255", rc("color(prophoto-rgb 0.5 0.5 0.5)"));
    /* prophoto-rgb under 1/32 is linear, v / 16: 0.001875, 6.177 (v / 15
     * would give 6.59) */
    TEST_ASSERT_EQUAL_STRING("#060606 255", rc("color(prophoto-rgb 0.03 0.03 0.03)"));
    TEST_ASSERT_EQUAL_STRING("#ffffff 255", rc("color(display-p3 1 1 1)"));
    /* display-p3 red is outside sRGB; Lot 103, converted (was #ff0000,
     * clipped): gamut-mapped by the OKLCh chroma at its L 0.6487 */
    TEST_ASSERT_EQUAL_STRING("#ff0b0c 255", rc("color(display-p3 1 0 0)"));
    TEST_ASSERT_EQUAL_STRING("#ffffff 255", rc("color(rec2020 1 1 1)"));
    TEST_ASSERT_EQUAL_STRING("#000000 255", rc("color(rec2020 0 0 0)"));
    TEST_ASSERT_EQUAL_STRING("#000000 255", rc("color(xyz 0 0 0)"));
    /* the D65 and D50 whites */
    TEST_ASSERT_EQUAL_STRING("#ffffff 255", rc("color(xyz-d65 0.95047 1 1.08883)"));
    TEST_ASSERT_EQUAL_STRING("#ffffff 255", rc("color(xyz-d50 0.96422 1 0.82521)"));
    TEST_ASSERT_EQUAL_STRING(rc("color(xyz-d65 0.2 0.3 0.4)"), rc("color(xyz 0.2 0.3 0.4)"));
    TEST_ASSERT_EQUAL_STRING(rc("color(xyz 100 0 0)"), rc("color(xyz 1000 0 0)"));   /* within 100 */
    TEST_ASSERT_EQUAL_STRING("#ffffff 255", rc("Color(Display-P3 1 1 1)"));
    TEST_ASSERT_EQUAL_STRING("#ffffff 255", rc("color( srgb 1 1 1 )"));
    TEST_ASSERT_EQUAL_STRING("-", rc("color(p3 1 0 0)"));
    TEST_ASSERT_EQUAL_STRING("-", rc("color(--custom 1 0 0)"));
    TEST_ASSERT_EQUAL_STRING("-", rc("color(srgb 1 0)"));
    TEST_ASSERT_EQUAL_STRING("-", rc("color(srgb 1 0 0 0)"));
    TEST_ASSERT_EQUAL_STRING("-", rc("color(srgb, 1, 0, 0)"));
    TEST_ASSERT_EQUAL_STRING("-", rc("color(srgb 1, 0, 0)"));
    TEST_ASSERT_EQUAL_STRING("-", rc("color(1 0 0)"));
    TEST_ASSERT_EQUAL_STRING("-", rc("color()"));
    TEST_ASSERT_EQUAL_STRING("-", rc("color(srgb 1 0 0 / 1 / 1)"));
    TEST_ASSERT_EQUAL_STRING("-", rc("color(srgb 1px 0 0)"));
    /* in color-mix() and registered <color> values */
    TEST_ASSERT_EQUAL_STRING("#bbbbbb 255", rc("color-mix(in srgb, lab(50 0 0), white)"));   /* 186.957 */
    TEST_ASSERT(syn("'<color>'"));
    TEST_ASSERT(wf_cssprop_match_prop(&g_p, "lab(50 0 0)"));
    TEST_ASSERT(wf_cssprop_match_prop(&g_p, "color(display-p3 1 0 0 / 0.5)"));
    TEST_ASSERT(!wf_cssprop_match_prop(&g_p, "lab(50, 0, 0)"));
    TEST_ASSERT(wf_cssprop_match_prop(&g_p, "currentcolor"));        /* a keyword there (Lot 97) */
    /* Lot 106 (converted, were rgb(187, 187, 187) and rgb(128, 0, 128) in
     * sRGB): lab() and color() are not legacy forms, interpolated in Oklab */
    TEST_ASSERT_EQUAL_STRING("rgb(185, 185, 185)", ip("lab(50 0 0)", "white", 5000));
    TEST_ASSERT_EQUAL_STRING("rgb(140, 83, 162)", ip("color(srgb 1 0 0)", "color(srgb 0 0 1)", 5000));
    TEST_ASSERT_EQUAL_STRING("rgb(128, 0, 128)", ip("rgb(255 0 0)", "rgb(0 0 255)", 5000));
}

/* currentcolor in wf_cssprop: only while the reader sets it */
static void test_l101_current_api(void) {
    TEST_ASSERT_EQUAL_STRING("-", rc("currentcolor"));
    TEST_ASSERT_EQUAL_STRING("-", rc("color-mix(in srgb, currentcolor, white)"));
    wf_cssprop_set_current(1, 0x112233u, 128);
    TEST_ASSERT_EQUAL_STRING("#112233 128", rc("currentcolor"));
    TEST_ASSERT_EQUAL_STRING("#112233 128", rc("CurrentColor"));
    wf_cssprop_set_current(1, 0x000000u, 255);
    TEST_ASSERT_EQUAL_STRING("#808080 255", rc("color-mix(in srgb, currentcolor, white)"));
    wf_cssprop_set_current(0, 0, 0);
    TEST_ASSERT_EQUAL_STRING("-", rc("currentcolor"));
    /* wf_cssprop_color_cur: valid with some current colour, the state kept */
    TEST_ASSERT_EQUAL(1, wf_cssprop_color_cur("currentcolor", 12));
    TEST_ASSERT_EQUAL(1, wf_cssprop_color_cur("color-mix(in srgb, currentcolor, red)", 37));
    TEST_ASSERT_EQUAL(0, wf_cssprop_color_cur("currentcolors", 13));
    TEST_ASSERT_EQUAL(0, wf_cssprop_color_cur("currentcolor", 7));
    TEST_ASSERT_EQUAL_STRING("-", rc("currentcolor"));
    wf_cssprop_set_current(1, 0xff0000u, 255);
    TEST_ASSERT_EQUAL(1, wf_cssprop_color_cur("lab(50 0 0)", 11));
    TEST_ASSERT_EQUAL_STRING("#ff0000 255", rc("currentcolor"));
    wf_cssprop_set_current(0, 0, 0);
}

/* currentcolor in the reader paint */
static void test_l101_current_paint(void) {
    /* background-color: the element's own colour, else the inherited one */
    prep("#o { color: red; background-color: currentcolor }", 0, 0);
    TEST_ASSERT_EQUAL_STRING(" style=\"color:#ff0000;background-color:#ff0000\"", style_of("o"));
    prep("#o { color: red } #i { background-color: currentcolor }", 0, 0);
    TEST_ASSERT_EQUAL_STRING(" style=\"background-color:#ff0000\"", style_of("i"));
    prep("#o { color: rgb(255 0 0 / 50%) } #i { background-color: currentColor }", 0, 0);
    TEST_ASSERT_EQUAL_STRING(" style=\"background-color:#ff000080\"", style_of("i"));
    prep("#i { background-color: currentcolor }", 0, 0);
    TEST_ASSERT_EQUAL_STRING(" style=\"background-color:#000000\"", style_of("i"));     /* black at the top */
    /* color: currentcolor is inherit */
    prep("#o { color: currentcolor }", 0, 0);
    TEST_ASSERT_EQUAL_STRING("", style_of("o"));
    prep("body { color: lime } #o { color: currentcolor }", 0, 0);
    TEST_ASSERT_EQUAL_STRING(" style=\"color:#00ff00\"", style_of("o"));
    prep("#o { color: blue } #o { color: currentcolor }", 0, 0);
    TEST_ASSERT_EQUAL_STRING("", style_of("o"));
    /* in color-mix(): in color the parent's colour */
    prep("#o { color: red } #i { color: color-mix(in srgb, currentcolor, blue) }", 0, 0);
    TEST_ASSERT_EQUAL_STRING(" style=\"color:#800080\"", style_of("i"));
    prep("#o { color: color-mix(in srgb, currentcolor, white) }", 0, 0);
    TEST_ASSERT_EQUAL_STRING(" style=\"color:#808080\"", style_of("o"));
    prep("#o { color: lab(50 0 0); background: currentcolor }", 0, 0);
    TEST_ASSERT_EQUAL_STRING(" style=\"color:#777777;background-color:#777777\"", style_of("o"));
    prep("#o { color: red; background-color: color-mix(in srgb, currentcolor 25%, blue) }", 0, 0);
    TEST_ASSERT_EQUAL_STRING(" style=\"color:#ff0000;background-color:#4000bf\"", style_of("o"));
    TEST_ASSERT_EQUAL(1, g_d.cur_resolved);
    /* the shorthand colour word stays under 32 bytes: dropped (a gap) */
    prep("#o { color: red; background: color-mix(in srgb, currentcolor, red) }", 0, 0);
    TEST_ASSERT_EQUAL_STRING(" style=\"color:#ff0000\"", style_of("o"));
    /* !important */
    prep("#o { color: lime; background-color: currentcolor !important; background-color: blue }", 0, 0);
    TEST_ASSERT_EQUAL_STRING(" style=\"color:#00ff00;background-color:#00ff00\"", style_of("o"));
    prep("#o { color: red; background-color: currentcolor } #i { color: color-mix(in srgb, currentcolor, blue) }", 0, 0);
    TEST_ASSERT_EQUAL(2, g_d.cur_resolved);
    /* inherit takes the parent's resolved colour */
    prep("#o { color: color-mix(in srgb, currentcolor, white) } #i { color: inherit }", 0, 0);
    TEST_ASSERT_EQUAL_STRING(" style=\"color:#808080\"", style_of("i"));
    prep("body { color: red } #o { color: color-mix(in srgb, currentcolor, blue) } #i { background-color: inherit; "
         "color: inherit }", 0, 0);
    TEST_ASSERT_EQUAL_STRING(" style=\"color:#800080\"", style_of("i"));
    /* a weaker colour with currentcolor under a winning inherit */
    prep("#o { color: red } #i { color: color-mix(in srgb, currentcolor, blue) } #o #i { color: inherit }", 0, 0);
    TEST_ASSERT_EQUAL_STRING(" style=\"color:#ff0000\"", style_of("i"));
    TEST_ASSERT_EQUAL(0, g_d.cur_resolved);
    /* the pool holds WF_SITE_LONGS texts: the next one is dropped */
    {
        static char css[4096];
        int k, m = snprintf(css, sizeof(css), "#o { color: red }");
        for (k = 1; k <= WF_SITE_LONGS + 1; k++)
            m += snprintf(css + m, sizeof(css) - (size_t)m, " #o { background-color: color-mix(in srgb, currentcolor %d%%, blue) }", k);
        prep(css, 0, 0);
        /* 32% of red: 81.6 and 173.4 */
        TEST_ASSERT_EQUAL_STRING(" style=\"color:#ff0000;background-color:#5200ad\"", style_of("o"));
        TEST_ASSERT_EQUAL(WF_SITE_LONGS, g_d.nlongs);
    }
    /* the reader leaves no current colour behind */
    TEST_ASSERT_EQUAL_STRING("-", rc("currentcolor"));
}

/* currentcolor in keyframes and starting values */
static void test_l101_current_anim(void) {
    /* color: from the parent's red to blue */
    prep("@keyframes k { from { color: currentcolor } to { color: blue } } #o { color: red } "
         "#i { animation: k 1000ms linear }", 500, 0);
    TEST_ASSERT_EQUAL_STRING(" style=\"color:#800080\"", style_of("i"));
    /* background-color: the element's (animated) colour */
    prep("@keyframes k { from { background-color: currentcolor } to { background-color: currentcolor } } "
         "#o { color: lime; animation: k 1000ms }", 500, 0);
    TEST_ASSERT_EQUAL_STRING(" style=\"color:#00ff00;background-color:#00ff00\"", style_of("o"));
    prep("@starting-style { #o { background-color: currentcolor } } "
         "#o { color: red; background-color: blue; transition: background-color 1000ms linear }", 500, 0);
    TEST_ASSERT_EQUAL_STRING(" style=\"color:#ff0000;background-color:#800080\"", style_of("o"));
}

/* the timeline lists (wf_cssprop) */
static void test_l101_tl_api(void) {
    static wf_csstl_t t[8];
    static char x[64];
    TEST_ASSERT_EQUAL(2, wf_csstl_parse(WF_CSSTL_NAMES, "--a, none", t, 4));
    TEST_ASSERT_EQUAL_STRING("--a", t[0].name);
    TEST_ASSERT_EQUAL_STRING("", t[1].name);
    TEST_ASSERT_EQUAL(WF_CSSTL_BLOCK, t[0].axis);
    TEST_ASSERT_EQUAL(3, wf_csstl_parse(WF_CSSTL_AXES, "inline, x, y", t, 4));
    TEST_ASSERT_EQUAL(WF_CSSTL_INLINE, t[0].axis);
    TEST_ASSERT_EQUAL(WF_CSSTL_X, t[1].axis);
    TEST_ASSERT_EQUAL(WF_CSSTL_Y, t[2].axis);
    TEST_ASSERT_EQUAL(2, wf_csstl_parse(WF_CSSTL_SHORT, "--a x, --b", t, 4));
    TEST_ASSERT_EQUAL(WF_CSSTL_X, t[0].axis);
    TEST_ASSERT_EQUAL(WF_CSSTL_BLOCK, t[1].axis);
    TEST_ASSERT_EQUAL(-1, wf_csstl_parse(WF_CSSTL_SHORT, "x --a", t, 4));       /* the name first */
    TEST_ASSERT_EQUAL(-1, wf_csstl_parse(WF_CSSTL_SHORT, "--a x y", t, 4));
    TEST_ASSERT_EQUAL(-1, wf_csstl_parse(WF_CSSTL_NAMES, "--a --b", t, 4));
    TEST_ASSERT_EQUAL(-1, wf_csstl_parse(WF_CSSTL_NAMES, "auto", t, 4));
    TEST_ASSERT_EQUAL(-1, wf_csstl_parse(WF_CSSTL_NAMES, "--", t, 4));
    TEST_ASSERT_EQUAL(-1, wf_csstl_parse(WF_CSSTL_AXES, "vertical", t, 4));
    TEST_ASSERT_EQUAL(-1, wf_csstl_parse(WF_CSSTL_NAMES, "--a, --b, --c, --d, --e", t, 8));   /* at most 4 */
    TEST_ASSERT_EQUAL(-1, wf_csstl_parse(WF_CSSTL_NAMES, "--a, --b", t, 1));
    TEST_ASSERT_EQUAL(-1, wf_csstl_parse(WF_CSSTL_NAMES, "--a", t, 0));
    TEST_ASSERT_EQUAL(-1, wf_csstl_parse(WF_CSSTL_NAMES, "--a,", t, 4));
    TEST_ASSERT_EQUAL(-1, wf_csstl_parse(6, "--a", t, 4));     /* Lot 102, converted: 3 is WF_CSSTL_SCOPE; Lot 104, converted: 4 and 5 are WF_CSSTL_INSETS and WF_CSSTL_VSHORT */
    TEST_ASSERT_EQUAL(-1, wf_csstl_parse(-1, "--a", t, 4));
    TEST_ASSERT_EQUAL(-1, wf_csstl_parse(3, "block", t, 4));
    TEST_ASSERT_EQUAL(-1, wf_csstl_parse(-1, "block", t, 4));
    TEST_ASSERT_EQUAL(-1, wf_csstl_parse(WF_CSSTL_NAMES, 0, t, 4));
    TEST_ASSERT_EQUAL(-1, wf_csstl_parse(WF_CSSTL_NAMES, "--a", 0, 4));
    /* 31 bytes is the longest name */
    TEST_ASSERT_EQUAL(1, wf_csstl_parse(WF_CSSTL_NAMES, "--abcdefghijklmnopqrstuvwxyz123", t, 4));
    TEST_ASSERT_EQUAL(-1, wf_csstl_parse(WF_CSSTL_NAMES, "--abcdefghijklmnopqrstuvwxyz1234", t, 4));
    /* coordinated: the axes repeat, block without them */
    TEST_ASSERT_EQUAL(3, wf_csstl_coord("--a, --b, --c", "x, y", t, 4));
    TEST_ASSERT_EQUAL(WF_CSSTL_X, t[2].axis);
    TEST_ASSERT_EQUAL(WF_CSSTL_Y, t[1].axis);
    TEST_ASSERT_EQUAL(1, wf_csstl_coord("--a", 0, t, 4));
    TEST_ASSERT_EQUAL(WF_CSSTL_BLOCK, t[0].axis);
    TEST_ASSERT_EQUAL(1, wf_csstl_coord("--a", "bad", t, 4));
    TEST_ASSERT_EQUAL(WF_CSSTL_BLOCK, t[0].axis);
    TEST_ASSERT_EQUAL(0, wf_csstl_coord(0, "x", t, 4));
    TEST_ASSERT_EQUAL(0, wf_csstl_coord("bad", "x", t, 4));
    TEST_ASSERT_EQUAL(0, wf_csstl_coord("--a", "x", 0, 4));
    /* the texts */
    TEST_ASSERT_EQUAL(2, wf_csstl_parse(WF_CSSTL_SHORT, "--a x, none", t, 4));
    TEST_ASSERT_EQUAL(9, wf_csstl_text(t, 2, 0, x, 64));
    TEST_ASSERT_EQUAL_STRING("--a, none", x);
    TEST_ASSERT_EQUAL(8, wf_csstl_text(t, 2, 1, x, 64));
    TEST_ASSERT_EQUAL_STRING("x, block", x);
    TEST_ASSERT_EQUAL(8, wf_csstl_text(t, 2, 1, x, 9));
    TEST_ASSERT_EQUAL(0, wf_csstl_text(t, 2, 1, x, 8));
    TEST_ASSERT_EQUAL_STRING("", x);
    {
        /* the separator after "--a" does not fit 4 bytes (no write past
         * them: checked with ASan) */
        char b4[4];
        TEST_ASSERT_EQUAL(0, wf_csstl_text(t, 2, 0, b4, 4));
        TEST_ASSERT_EQUAL_STRING("", b4);
    }
    TEST_ASSERT_EQUAL(0, wf_csstl_text(t, 0, 1, x, 64));
    TEST_ASSERT_EQUAL(0, wf_csstl_text(0, 1, 1, x, 64));
    TEST_ASSERT_EQUAL(0, wf_csstl_text(t, 1, 1, 0, 64));
    strcpy(x, "keep");
    TEST_ASSERT_EQUAL(0, wf_csstl_text(t, 1, 1, x, 0));
    TEST_ASSERT_EQUAL_STRING("keep", x);
}

/* the local time on a scroll timeline */
static void test_l101_scroll_ms(void) {
    wf_cssanim_t a;
    int ms;
    memset(&a, 0, sizeof(a));
    a.dur = 1000; a.iters = 10000;
    TEST_ASSERT_EQUAL(1, wf_cssanim_scroll_ms(&a, 5000, &ms));
    TEST_ASSERT_EQUAL(500, ms);
    a.delay = 1000;                   /* the delay shares the timeline */
    TEST_ASSERT_EQUAL(1, wf_cssanim_scroll_ms(&a, 5000, &ms));
    TEST_ASSERT_EQUAL(1000, ms);
    a.delay = -500;
    TEST_ASSERT_EQUAL(1, wf_cssanim_scroll_ms(&a, 10000, &ms));
    TEST_ASSERT_EQUAL(500, ms);
    a.delay = 0; a.dur = 3; a.iters = 10000;
    TEST_ASSERT_EQUAL(1, wf_cssanim_scroll_ms(&a, 5000, &ms));
    TEST_ASSERT_EQUAL(1, ms);                         /* 1.5 rounded down */
    a.dur = 1000; a.iters = 25000;                    /* 2.5 iterations */
    TEST_ASSERT_EQUAL(1, wf_cssanim_scroll_ms(&a, 10000, &ms));
    TEST_ASSERT_EQUAL(2500, ms);
    a.iters = -1;                                     /* infinite */
    TEST_ASSERT_EQUAL(0, wf_cssanim_scroll_ms(&a, 5000, &ms));
    TEST_ASSERT_EQUAL(0, ms);
    a.iters = 10000; a.dur = 0;
    TEST_ASSERT_EQUAL(0, wf_cssanim_scroll_ms(&a, 5000, &ms));
    a.dur = 1000; a.delay = -1000;
    TEST_ASSERT_EQUAL(0, wf_cssanim_scroll_ms(&a, 5000, &ms));
    a.delay = 100000000; a.dur = 0;                   /* 1e8 ms: the longest */
    TEST_ASSERT_EQUAL(1, wf_cssanim_scroll_ms(&a, 10000, &ms));
    TEST_ASSERT_EQUAL(100000000, ms);
    a.dur = 1;
    TEST_ASSERT_EQUAL(0, wf_cssanim_scroll_ms(&a, 10000, &ms));
    a.delay = 0; a.dur = 1000;
    TEST_ASSERT_EQUAL(0, wf_cssanim_scroll_ms(&a, -1, &ms));
    TEST_ASSERT_EQUAL(0, wf_cssanim_scroll_ms(&a, 10001, &ms));
    TEST_ASSERT_EQUAL(1, wf_cssanim_scroll_ms(&a, 0, &ms));
    TEST_ASSERT_EQUAL(0, ms);
    TEST_ASSERT_EQUAL(0, wf_cssanim_scroll_ms(0, 5000, &ms));
    TEST_ASSERT_EQUAL(0, wf_cssanim_scroll_ms(&a, 5000, 0));
}

/* the cascade of the timeline properties */
static void test_l101_tl_cascade(void) {
    int o;
    /* a weaker later rule loses (:root over html), an equal later one wins */
    prep(KB ":root { scroll-timeline: --t } html { scroll-timeline: --t x } #o { animation: k 1000ms linear; "
         "animation-timeline: --t }", 0, 2500);
    TEST_ASSERT_EQUAL_STRING(" style=\"color:#404040\"", style_of("o"));
    prep(KB "html { scroll-timeline: --t x } html { scroll-timeline: --t } #o { animation: k 1000ms linear; "
         "animation-timeline: --t }", 0, 2500);
    TEST_ASSERT_EQUAL_STRING(" style=\"color:#404040\"", style_of("o"));
    /* in one rule the later declaration wins */
    prep(KB "html { scroll-timeline: --t x; scroll-timeline-axis: y } #o { animation: k 1000ms linear; "
         "animation-timeline: --t }", 0, 2500);
    TEST_ASSERT_EQUAL_STRING(" style=\"color:#404040\"", style_of("o"));
    /* the axis list of a shorthand has the shorthand's own layer (here
     * none), not the one of an earlier shorthand (list-style in layer
     * a): the unlayered x beats the y of layer b */
    prep(KB "@layer a, b; @layer a { #q { list-style: square } } @layer b { html { scroll-timeline-axis: y } } "
         "html { scroll-timeline: --t x } #o { animation: k 1000ms linear; animation-timeline: --t }", 0, 2500);
    TEST_ASSERT_EQUAL_STRING("", style_of("o"));
    /* the axis list of a shorthand goes to the shorthand's own element */
    prep(KB "#o { view-timeline: --q y } html { scroll-timeline: --t x } #o { animation: k 1000ms linear; "
         "animation-timeline: --t }", 0, 2500);
    TEST_ASSERT_EQUAL_STRING("", style_of("o"));
    /* the four longhands are kept as written (view timelines never run) */
    prep("#o { scroll-timeline-name: --a, --b; scroll-timeline-axis: y; view-timeline-name: --c; "
         "view-timeline-axis: inline, x }", 0, 0);
    o = node("o");
    TEST_ASSERT_TRUE(o >= 0 && g_d.node[o].tl[0] && g_d.node[o].tl[1] && g_d.node[o].tl[2] && g_d.node[o].tl[3]);
    TEST_ASSERT_EQUAL_STRING("--a, --b", g_d.lpool[g_d.node[o].tl[0] - 1]);
    TEST_ASSERT_EQUAL_STRING("y", g_d.lpool[g_d.node[o].tl[1] - 1]);
    TEST_ASSERT_EQUAL_STRING("--c", g_d.lpool[g_d.node[o].tl[2] - 1]);
    TEST_ASSERT_EQUAL_STRING("inline, x", g_d.lpool[g_d.node[o].tl[3] - 1]);
    /* an inherited currentcolor background takes the element's own colour */
    prep("#o { color: red; background-color: currentcolor } #i { color: blue; background-color: inherit }", 0, 0);
    TEST_ASSERT_EQUAL_STRING(" style=\"color:#0000ff;background-color:#0000ff\"", style_of("i"));
}

/* named scroll timelines in the reader */
static void test_l101_scroll_reader(void) {
    /* 25% of the root scroll range: 250 of 1000 ms, 63.75 */
    prep(KB "html { scroll-timeline: --t } #o { animation: k 1000ms linear; animation-timeline: --t }", 0, 2500);
    TEST_ASSERT_EQUAL_STRING(" style=\"color:#404040\"", style_of("o"));
    TEST_ASSERT_EQUAL(1, g_d.anim_scroll);
    TEST_ASSERT_EQUAL(2500, g_d.scroll_pm);
    TEST_ASSERT_EQUAL_STRING("keyframes 1 animated 0 transitioned 0 font 0 at 0ms scroll 1 pos 2500 color 1 0", anim_text());
    /* the reader clock does not move it */
    prep(KB "html { scroll-timeline: --t } #o { animation: k 1000ms linear; animation-timeline: --t }", 750, 2500);
    TEST_ASSERT_EQUAL_STRING(" style=\"color:#404040\"", style_of("o"));
    prep(KB "html { scroll-timeline: --t } #o { animation: k 1000ms linear; animation-timeline: --t }", 0, 0);
    TEST_ASSERT_EQUAL_STRING(" style=\"color:#000000\"", style_of("o"));
    /* the end: the after phase (no fill), then forwards */
    prep(KB "html { scroll-timeline: --t } #o { animation: k 1000ms linear; animation-timeline: --t }", 0, 10000);
    TEST_ASSERT_EQUAL_STRING("", style_of("o"));
    TEST_ASSERT_EQUAL(1, g_d.anim_scroll);
    prep(KB "html { scroll-timeline: --t } #o { animation: k 1000ms linear forwards; animation-timeline: --t }", 0, 10000);
    TEST_ASSERT_EQUAL_STRING(" style=\"color:#ffffff\"", style_of("o"));
    /* the delay takes the first half: 1500 ms at 75%, half way */
    prep(KB "html { scroll-timeline: --t } #o { animation: k 1000ms linear 1000ms; animation-timeline: --t }", 0, 7500);
    TEST_ASSERT_EQUAL_STRING(" style=\"color:#808080\"", style_of("o"));
    prep(KB "html { scroll-timeline: --t } #o { animation: k 1000ms linear 1000ms; animation-timeline: --t }", 0, 2500);
    TEST_ASSERT_EQUAL_STRING("", style_of("o"));
    /* two iterations: 1500 of 2000 ms */
    prep(KB "html { scroll-timeline: --t } #o { animation: k 1000ms linear 2; animation-timeline: --t }", 0, 7500);
    TEST_ASSERT_EQUAL_STRING(" style=\"color:#808080\"", style_of("o"));
    /* infinite: no effect */
    prep(KB "html { scroll-timeline: --t } #o { animation: k 1000ms linear infinite; animation-timeline: --t }", 0, 2500);
    TEST_ASSERT_EQUAL_STRING("", style_of("o"));
    TEST_ASSERT_EQUAL(0, g_d.anim_scroll);
    /* paused: the start */
    prep(KB "html { scroll-timeline: --t } #o { animation: k 1000ms linear paused; animation-timeline: --t }", 0, 5000);
    TEST_ASSERT_EQUAL_STRING(" style=\"color:#000000\"", style_of("o"));
    /* axes: block and y run, inline and x do not */
    prep(KB "html { scroll-timeline: --t y } #o { animation: k 1000ms linear; animation-timeline: --t }", 0, 2500);
    TEST_ASSERT_EQUAL_STRING(" style=\"color:#404040\"", style_of("o"));
    prep(KB "html { scroll-timeline: --t x } #o { animation: k 1000ms linear; animation-timeline: --t }", 0, 2500);
    TEST_ASSERT_EQUAL_STRING("", style_of("o"));
    prep(KB "html { scroll-timeline-name: --t; scroll-timeline-axis: inline } #o { animation: k 1000ms linear; "
         "animation-timeline: --t }", 0, 2500);
    TEST_ASSERT_EQUAL_STRING("", style_of("o"));
    /* the last name of a list wins */
    prep(KB "html { scroll-timeline-name: --t, --t; scroll-timeline-axis: x, y } #o { animation: k 1000ms linear; "
         "animation-timeline: --t }", 0, 2500);
    TEST_ASSERT_EQUAL_STRING(" style=\"color:#404040\"", style_of("o"));
    /* another element is not the root scroller; the nearest one wins */
    prep(KB "body { scroll-timeline: --t } #o { animation: k 1000ms linear; animation-timeline: --t }", 0, 2500);
    TEST_ASSERT_EQUAL_STRING("", style_of("o"));
    prep(KB "html { scroll-timeline: --t } body { scroll-timeline: --t } #o { animation: k 1000ms linear; "
         "animation-timeline: --t }", 0, 2500);
    TEST_ASSERT_EQUAL_STRING("", style_of("o"));
    prep(KB "html { scroll-timeline: --t } body { scroll-timeline: --u } #o { animation: k 1000ms linear; "
         "animation-timeline: --t }", 0, 2500);
    TEST_ASSERT_EQUAL_STRING(" style=\"color:#404040\"", style_of("o"));
    /* a sibling's timeline is not seen (no timeline-scope) */
    prep(KB "html { scroll-timeline: --u } #q { scroll-timeline: --t } #o { animation: k 1000ms linear; "
         "animation-timeline: --t }", 0, 2500);
    TEST_ASSERT_EQUAL_STRING("", style_of("o"));
    /* view timelines need a layout: no effect; a scroll timeline of the
     * same element wins over one */
    prep(KB "html { view-timeline: --t } #o { animation: k 1000ms linear; animation-timeline: --t }", 0, 2500);
    TEST_ASSERT_EQUAL_STRING("", style_of("o"));
    prep(KB "html { view-timeline: --t; scroll-timeline: --t } #o { animation: k 1000ms linear; animation-timeline: --t }",
         0, 2500);
    TEST_ASSERT_EQUAL_STRING(" style=\"color:#404040\"", style_of("o"));
    /* initial resets, inherit is dropped */
    prep(KB "html { scroll-timeline: --t; scroll-timeline-name: initial } #o { animation: k 1000ms linear; "
         "animation-timeline: --t }", 0, 2500);
    TEST_ASSERT_EQUAL_STRING("", style_of("o"));
    prep(KB "html { scroll-timeline: --t; scroll-timeline: inherit } #o { animation: k 1000ms linear; "
         "animation-timeline: --t }", 0, 2500);
    TEST_ASSERT_EQUAL_STRING(" style=\"color:#404040\"", style_of("o"));
    prep(KB "html { scroll-timeline: --t; scroll-timeline: unset } #o { animation: k 1000ms linear; "
         "animation-timeline: --t }", 0, 2500);
    TEST_ASSERT_EQUAL_STRING("", style_of("o"));
    prep(KB "html { scroll-timeline-name: --t; scroll-timeline-axis: x; scroll-timeline-axis: unset } "
         "#o { animation: k 1000ms linear; animation-timeline: --t }", 0, 2500);
    TEST_ASSERT_EQUAL_STRING(" style=\"color:#404040\"", style_of("o"));
    prep(KB "html { view-timeline: --t; view-timeline: initial; scroll-timeline: --t } #o { animation: k 1000ms linear; "
         "animation-timeline: --t }", 0, 2500);
    TEST_ASSERT_EQUAL_STRING(" style=\"color:#404040\"", style_of("o"));
    /* a nearer view timeline hides the root's scroll timeline */
    prep(KB "html { scroll-timeline: --t } #o { view-timeline: --t; animation: k 1000ms linear; animation-timeline: --t }",
         0, 2500);
    TEST_ASSERT_EQUAL_STRING("", style_of("o"));
    prep(KB "html { scroll-timeline: --t } body { view-timeline-name: --u, --t; view-timeline-axis: block } "
         "#o { animation: k 1000ms linear; animation-timeline: --t }", 0, 2500);
    TEST_ASSERT_EQUAL_STRING("", style_of("o"));
    /* only the root html element: not a root body, not an inner html */
    prep_doc("<body><style>" KB "body { scroll-timeline: --t } #o { animation: k 1000ms linear; animation-timeline: --t }"
             "</style><div id=o>o</div></body>", 2500);
    TEST_ASSERT_EQUAL_STRING("", style_of("o"));
    prep_doc("<html><head><style>" KB "#h { scroll-timeline: --t } #o { animation: k 1000ms linear; "
             "animation-timeline: --t }</style></head><body><html id=h><div id=o>o</div></html></body></html>", 2500);
    TEST_ASSERT_EQUAL_STRING("", style_of("o"));
    prep_doc("<html><head><style>" KB "html { scroll-timeline: --t } #o { animation: k 1000ms linear; "
             "animation-timeline: --t }</style></head><body><div id=o>o</div></body></html>", 2500);
    TEST_ASSERT_EQUAL_STRING(" style=\"color:#404040\"", style_of("o"));
    /* the reader lowercases values: names match without case (a gap) */
    prep(KB "html { scroll-timeline: --T } #o { animation: k 1000ms linear; animation-timeline: --t }", 0, 2500);
    TEST_ASSERT_EQUAL_STRING(" style=\"color:#404040\"", style_of("o"));
    /* a registered property on the scroll timeline (idle counts the rest) */
    prep("@property --p { syntax: '<length>'; inherits: false; initial-value: 0px } "
         "@keyframes kp { from { --p: 0px } to { --p: 100px } } html { scroll-timeline: --t } "
         "#o { animation: kp 1000ms linear, kp 1000ms linear; animation-timeline: --t, --u }", 0, 4000);
    TEST_ASSERT_EQUAL_STRING("40px", wf_site_node_var(&g_d, node("o"), "--p"));
    TEST_ASSERT_EQUAL(1, g_d.anim_idle);
    TEST_ASSERT_EQUAL(1, g_d.anim_scroll);
    /* the position is clamped */
    wf_site_set_scroll(-5);
    prep(KB, 0, 20000);
    TEST_ASSERT_EQUAL(10000, g_d.scroll_pm);
    prep(KB, 0, -5);
    TEST_ASSERT_EQUAL(0, g_d.scroll_pm);
}

int main(void) {
    unity_init();
    RUN_TEST(test_l101_lab);
    RUN_TEST(test_l101_oklab);
    RUN_TEST(test_l101_color_fn);
    RUN_TEST(test_l101_current_api);
    RUN_TEST(test_l101_current_paint);
    RUN_TEST(test_l101_current_anim);
    RUN_TEST(test_l101_tl_api);
    RUN_TEST(test_l101_scroll_ms);
    RUN_TEST(test_l101_tl_cascade);
    RUN_TEST(test_l101_scroll_reader);
    unity_print_results();
    unity_cleanup();
    return unity_stats.tests_failed == 0 ? 0 : 1;
}
