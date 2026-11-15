/* Lot 106 (US-031): the default Oklab interpolation of colours unless
 * both are legacy sRGB forms (CSS Color 4 12.1), the alpha past the ends,
 * add staying in sRGB, and body as a view timeline subject with its own
 * geometry record while html is inactive (CSS Scroll-driven Animations
 * 1). Hand cases next to the generated test_lot106_interp.c and
 * test_lot106_sheet.c; expected values follow docs/lot106_plan.md. */
#include "../../framework/unity.h"
#include <string.h>
#include <stdio.h>
#include "wf_cssprop.h"
#include "wf_sitecss.h"

static wf_cssprop_t g_p;
static char g_o[512];
static wf_site_t g_d;
static char g_html[4096];
static wf_geom_t g_g;

static const char *ip(const char *a, const char *b, int t) {
    memset(&g_p, 0, sizeof(g_p));
    g_p.syntax = (unsigned char)wf_cssprop_syntax_parse("'<color>'", 9, &g_p);
    if (wf_cssprop_interp(&g_p, a, b, t, g_o, (int)sizeof(g_o)) != 2) return "NOT INTERPOLATED";
    return g_o;
}

static const char *ad(const char *a, const char *b, int acc) {
    memset(&g_p, 0, sizeof(g_p));
    g_p.syntax = (unsigned char)wf_cssprop_syntax_parse("'<color>'", 9, &g_p);
    if ((acc ? wf_cssprop_accumulate(&g_p, a, b, g_o, (int)sizeof(g_o)) : wf_cssprop_add(&g_p, a, b, g_o, (int)sizeof(g_o))) != 2)
        return "NOT ADDED";
    return g_o;
}

/* the space: legacy pairs in sRGB, any other pair in Oklab */
static void test_l106_interp_space(void) {
    wf_cssprop_set_current(1, 0x000000u, 255);
    TEST_ASSERT_EQUAL_STRING("rgb(128, 0, 128)", ip("red", "blue", 5000));
    TEST_ASSERT_EQUAL_STRING("rgb(128, 0, 128)", ip("hsl(0 100% 50%)", "hwb(240 0% 0%)", 5000));
    TEST_ASSERT_EQUAL_STRING("rgb(128, 0, 128)", ip("#ff0000", "rgba(0, 0, 255, 1)", 5000));
    /* one side not legacy is enough: color(srgb) is the same blue */
    TEST_ASSERT_EQUAL_STRING("rgb(140, 83, 162)", ip("red", "color(srgb 0 0 1)", 5000));
    TEST_ASSERT_EQUAL_STRING("rgb(140, 83, 162)", ip("color(srgb 1 0 0)", "blue", 5000));
    TEST_ASSERT_EQUAL_STRING("rgb(140, 83, 162)", ip("color-mix(in srgb, red, red)", "blue", 5000));
    /* black to white: Oklab L 0.5 is not sRGB's 128 */
    TEST_ASSERT_EQUAL_STRING("rgb(99, 99, 99)", ip("oklab(0 0 0)", "oklab(1 0 0)", 5000));
    TEST_ASSERT_EQUAL_STRING("rgb(128, 128, 128)", ip("black", "white", 5000));
    /* currentcolor counts as legacy (black here) */
    TEST_ASSERT_EQUAL_STRING("rgb(99, 99, 99)", ip("currentcolor", "oklab(1 0 0)", 5000));
    TEST_ASSERT_EQUAL_STRING("rgb(128, 128, 128)", ip("currentcolor", "white", 5000));
    /* premultiplied: transparent takes nothing from its black */
    TEST_ASSERT_EQUAL_STRING("rgba(237, 118, 101, 0.5)", ip("transparent", "oklch(0.7 0.15 30)", 5000));
    TEST_ASSERT_EQUAL_STRING("rgba(179, 101, 89, 0.313)", ip("#ff000080", "lab(50 0 0 / 0.25)", 7500));
    /* missing components: carried within the analogous set (oklch
     * lightness to oklab L), else 0 (an lch hue, an rgb red) */
    TEST_ASSERT_EQUAL_STRING("rgb(194, 91, 96)", ip("oklch(0.6 0.2 none)", "oklch(0.6 0.2 120)", 2500));
    TEST_ASSERT_EQUAL_STRING("rgb(255, 255, 255)", ip("oklab(none none none)", "white", 5000));
    TEST_ASSERT_EQUAL_STRING("rgb(211, 77, 82)", ip("lch(50 30 none)", "red", 5000));
    TEST_ASSERT_EQUAL_STRING("rgb(74, 32, 11)", ip("rgb(none 0 0)", "oklab(0.6 0.1 0.1)", 5000));
    /* a mix in oklch near L 0 keeps its exact OKLab values (the way back
     * from linear sRGB gives 117 in the blue channel) */
    TEST_ASSERT_EQUAL_STRING("rgb(118, 118, 118)",
                             ip("color-mix(in oklch, oklab(0.00161 0.00299 0.00299), oklch(0.00075 0.00094 26) 90%)",
                                "oklab(1 0 0)", 5644));
    /* the ends give the values as written */
    TEST_ASSERT_EQUAL_STRING("lab(50 0 0)", ip("lab(50 0 0)", "red", 0));
    TEST_ASSERT_EQUAL_STRING("red", ip("lab(50 0 0)", "red", 10000));
    /* a list item by item */
    memset(&g_p, 0, sizeof(g_p));
    g_p.syntax = (unsigned char)wf_cssprop_syntax_parse("'<color>+'", 10, &g_p);
    TEST_ASSERT_EQUAL(2, wf_cssprop_interp(&g_p, "red oklab(0 0 0)", "blue oklab(1 0 0)", 5000, g_o, (int)sizeof(g_o)));
    TEST_ASSERT_EQUAL_STRING("rgb(128, 0, 128) rgb(99, 99, 99)", g_o);
    wf_cssprop_set_current(0, 0, 0);
    /* no current colour: currentcolor is no colour, the pair is discrete */
    memset(&g_p, 0, sizeof(g_p));
    g_p.syntax = (unsigned char)wf_cssprop_syntax_parse("'<color>'", 9, &g_p);
    TEST_ASSERT_EQUAL(1, wf_cssprop_interp(&g_p, "currentcolor", "oklab(1 0 0)", 4000, g_o, (int)sizeof(g_o)));
    TEST_ASSERT_EQUAL_STRING("currentcolor", g_o);
}

/* past the ends: the premultiplied components divided by the alpha sum
 * as it is, the alpha clamped after; far past, with an alpha sum near 0,
 * L grows without bound: white or black */
static void test_l106_overshoot(void) {
    TEST_ASSERT_EQUAL_STRING("rgb(99, 99, 99)", ip("oklab(0.5 0 0 / 0.2)", "oklab(0.5 0 0)", 20000));
    TEST_ASSERT_EQUAL_STRING("rgba(99, 99, 99, 0)", ip("oklab(0.5 0 0 / 0.2)", "oklab(0.5 0 0)", -10000));
    /* L (-0.08 + 1.6) / 1.8: not the clamped alpha's 1.52 (white) */
    TEST_ASSERT_EQUAL_STRING("rgb(204, 204, 204)", ip("oklab(0.4 0 0 / 0.2)", "oklab(0.8 0 0)", 20000));
    /* L (0.3 - 0.4) / 0.25 below 0: black at alpha 0.25 */
    TEST_ASSERT_EQUAL_STRING("rgba(0, 0, 0, 0.25)", ip("oklab(0.4 0 0 / 0.5)", "oklab(0.8 0 0)", -5000));
    TEST_ASSERT_EQUAL_STRING("rgba(255, 255, 255, 0.0001)", ip("oklab(0.9 0 0 / 0.2)", "oklab(0.1 0 0)", -2499));
    TEST_ASSERT_EQUAL_STRING("rgba(0, 0, 0, 0.0001)", ip("oklab(0.1 0 0 / 0.2)", "oklab(0.9 0 0)", -2499));
    /* an alpha sum just below 0 far past an end: L is about -1.2e7
     * (black) or 8.4e6 (white), kept within +-32 so it does not wrap;
     * the alpha is clamped to 0 */
    TEST_ASSERT_EQUAL_STRING("rgba(0, 0, 0, 0)",
                             ip("oklab(0.7893 0.2902 -0.0485 / 0.8839)", "oklab(0.5118 -0.2537 0.2913 / 0.9835)", -88745));
    TEST_ASSERT_EQUAL_STRING("rgba(255, 255, 255, 0)",
                             ip("oklab(0.2502 -0.0485 0.0304 / 0.7319)", "oklab(0.0109 0.2690 -0.2628 / 0.7048)", 270074));
    /* the alpha sum 0 exactly: no colour */
    TEST_ASSERT_EQUAL_STRING("rgba(0, 0, 0, 0)", ip("oklab(0.9 0 0 / 0.2)", "oklab(0.1 0 0)", -2500));
    /* legacy pairs keep the Lot 97 sRGB path */
    TEST_ASSERT_EQUAL_STRING("rgb(0, 0, 0)", ip("white", "black", 20000));
}

/* add and accumulate stay in sRGB, a missing component 0 */
static void test_l106_add_srgb(void) {
    TEST_ASSERT_EQUAL_STRING("rgb(119, 119, 219)", ad("lab(50 0 0)", "rgb(0 0 100)", 0));
    TEST_ASSERT_EQUAL_STRING("rgb(119, 119, 219)", ad("lab(50 0 0)", "rgb(0 0 100)", 1));
    TEST_ASSERT_EQUAL_STRING("rgb(255, 57, 123)", ad("oklch(0.6 0.2 none)", "red", 0));
    TEST_ASSERT_EQUAL_STRING("rgb(128, 10, 20)", ad("rgb(none 10 20)", "color(srgb 0.5 none 0)", 1));
}

/* ---- the reader */
static void prep(const char *css, const char *body, int ms, int pm, const wf_geom_t *g) {
    snprintf(g_html, sizeof(g_html), "<html><head><style>%s</style></head><body>%s</body></html>", css, body);
    wf_site_set_geom(g);
    wf_site_set_scroll(pm);
    wf_site_set_time(ms);
    TEST_ASSERT_EQUAL(0, wf_site_prepare_ext(&g_d, g_html, (int)strlen(g_html), 640, 480, 0, 0));
    wf_site_set_time(0);
    wf_site_set_scroll(0);
    wf_site_set_geom(0);
}

static int node(const char *id) {
    int i;
    for (i = 0; i < g_d.nnodes; i++)
        if (!strcmp(g_d.node[i].id, id) || (!g_d.node[i].id[0] && !strcmp(g_d.node[i].tag, id))) return i;
    return -1;
}

static const char *style_of(const char *id) {
    int i = node(id);
    g_o[0] = 0;
    if (i >= 0) wf_site_style_attr(&g_d, g_d.node[i].start, g_o, (int)sizeof(g_o));
    return g_o;
}

/* keyframes: a non-legacy frame interpolates in Oklab */
static void test_l106_keyframes(void) {
    prep("@keyframes k { from { color: oklab(0 0 0) } to { color: oklab(1 0 0) } } #i { animation: k 1s linear }",
         "<p id=i>i</p>", 500, 0, 0);
    TEST_ASSERT_EQUAL_STRING(" style=\"color:#636363\"", style_of("i"));
    prep("@keyframes k { from { color: black } to { color: white } } #i { animation: k 1s linear }", "<p id=i>i</p>", 500,
         0, 0);
    TEST_ASSERT_EQUAL_STRING(" style=\"color:#808080\"", style_of("i"));
    /* red to color(srgb 0 0 1), and the same frames as legacy colours */
    prep("@keyframes k { from { background-color: red } to { background-color: color(srgb 0 0 1) } }"
         " #i { animation: k 1s linear }", "<p id=i>i</p>", 500, 0, 0);
    TEST_ASSERT_EQUAL_STRING(" style=\"background-color:#8c53a2\"", style_of("i"));
    prep("@keyframes k { from { background-color: red } to { background-color: blue } }"
         " #i { animation: k 1s linear }", "<p id=i>i</p>", 500, 0, 0);
    TEST_ASSERT_EQUAL_STRING(" style=\"background-color:#800080\"", style_of("i"));
}

/* the body record: b_off 0, b_size 520 in a 480 px view, range 520 */
static void body_rec(int ok, int root) {
    memset(&g_g, 0, sizeof(g_g));
    g_g.vw = 640; g_g.vh = 480; g_g.rg_y = 520;
    g_g.body.ok = (unsigned char)ok; g_g.body.root = (unsigned char)root; g_g.body.bvert = 1;
    g_g.body.b_off = 0; g_g.body.b_size = 520; g_g.body.i_off = 0; g_g.body.i_size = 640;
    g_g.body.vp_b = 480; g_g.body.vp_i = 640; g_g.body.rg_b = 520;
}

/* a registration, so the animation pass (anim_idle) runs */
#define KF "@property --p { syntax: '<length>'; inherits: false; initial-value: 0px } " \
           "@keyframes k { from { color: #000000 } to { color: #ffffff } } "

/* body as a subject, html inactive */
static void test_l106_body_subject(void) {
    int b, h;
    /* view() on body at pm 5000: pos 260 in cover [-480, 520]: 740 ms,
     * 255 x 0.74 = 188.7: #bdbdbd */
    body_rec(1, 1);
    prep(KF "body { animation: k 1000ms linear; animation-timeline: view() }", "<p id=i>i</p>", 0, 5000, &g_g);
    TEST_ASSERT_EQUAL_STRING(" style=\"color:#bdbdbd\"", style_of("body"));
    b = node("body");
    h = node("html");
    TEST_ASSERT_EQUAL(1, g_d.node[b].geo);
    TEST_ASSERT_EQUAL(1, g_d.anim_view);
    TEST_ASSERT_EQUAL(1, g_d.geom_need);
    TEST_ASSERT_EQUAL(1, g_d.geom_used);
    /* the box by index is not the body's: a box at body's index alone
     * gives nothing */
    memset(&g_g, 0, sizeof(g_g));
    g_g.vw = 640; g_g.vh = 480; g_g.rg_y = 520;
    g_g.box[b].ok = 1; g_g.box[b].root = 1; g_g.box[b].bvert = 1; g_g.box[b].b_size = 520; g_g.box[b].vp_b = 480;
    g_g.box[b].rg_b = 520;
    prep(KF "body { animation: k 1000ms linear; animation-timeline: view() }", "<p id=i>i</p>", 0, 5000, &g_g);
    TEST_ASSERT_EQUAL_STRING("", style_of("body"));
    TEST_ASSERT_EQUAL(1, g_d.anim_idle);
    TEST_ASSERT_EQUAL(1, g_d.geom_need);
    TEST_ASSERT_EQUAL(0, g_d.geom_used);
    /* no body box, or one not in the root scroller: no effect */
    body_rec(0, 1);
    prep(KF "body { animation: k 1000ms linear; animation-timeline: view() }", "<p id=i>i</p>", 0, 5000, &g_g);
    TEST_ASSERT_EQUAL_STRING("", style_of("body"));
    body_rec(1, 0);
    prep(KF "body { animation: k 1000ms linear; animation-timeline: view() }", "<p id=i>i</p>", 0, 5000, &g_g);
    TEST_ASSERT_EQUAL_STRING("", style_of("body"));
    TEST_ASSERT_EQUAL(1, g_d.geom_used);
    /* a named view timeline on body, read by #i */
    body_rec(1, 1);
    prep(KF "body { view-timeline: --b } #i { animation: k 1000ms linear; animation-timeline: --b }", "<p id=i>i</p>",
         0, 5000, &g_g);
    TEST_ASSERT_EQUAL_STRING(" style=\"color:#bdbdbd\"", style_of("i"));
    TEST_ASSERT_EQUAL(1, g_d.node[b].geo);
    TEST_ASSERT_EQUAL(0, g_d.node[node("i")].geo);
    /* an element named body inside a div is not the body: it defines
     * the name nearer to #i and reads its box by index */
    prep(KF "body { view-timeline: --b } #i { animation: k 1000ms linear; animation-timeline: --b }",
         "<div><body id=x><p id=i>i</p></body></div>", 0, 5000, &g_g);
    TEST_ASSERT_EQUAL_STRING("", style_of("i"));
    TEST_ASSERT_EQUAL(1, g_d.node[node("x")].geo);
    g_g.box[node("x")] = g_g.body;
    prep(KF "body { view-timeline: --b } #i { animation: k 1000ms linear; animation-timeline: --b }",
         "<div><body id=x><p id=i>i</p></body></div>", 0, 5000, &g_g);
    TEST_ASSERT_EQUAL_STRING(" style=\"color:#bdbdbd\"", style_of("i"));
    body_rec(1, 1);
    /* html: inactive, no geometry asked even with a box at its index */
    body_rec(1, 1);
    g_g.box[0] = g_g.body;
    prep(KF "html { animation: k 1000ms linear; animation-timeline: view() }", "<p id=i>i</p>", 0, 5000, &g_g);
    TEST_ASSERT_EQUAL_STRING("", style_of("html"));
    TEST_ASSERT_EQUAL(0, g_d.node[h].geo);
    TEST_ASSERT_EQUAL(0, g_d.geom_need);
    TEST_ASSERT_EQUAL(1, g_d.anim_idle);
    prep(KF "html { view-timeline: --h } #i { animation: k 1000ms linear; animation-timeline: --h }", "<p id=i>i</p>", 0,
         5000, &g_g);
    TEST_ASSERT_EQUAL_STRING("", style_of("i"));
    TEST_ASSERT_EQUAL(0, g_d.geom_need);
    /* the body defining the name nearer than html wins */
    prep(KF "html { view-timeline: --h } body { view-timeline: --h } #i { animation: k 1000ms linear; animation-timeline: --h }",
         "<p id=i>i</p>", 0, 5000, &g_g);
    TEST_ASSERT_EQUAL_STRING(" style=\"color:#bdbdbd\"", style_of("i"));
    /* html's scroll timeline is still the root's */
    prep(KF "html { scroll-timeline: --s } #i { animation: k 1000ms linear; animation-timeline: --s }", "<p id=i>i</p>", 0,
         5000, &g_g);
    TEST_ASSERT_EQUAL_STRING(" style=\"color:#808080\"", style_of("i"));
}

static void prep_raw(const char *html, const wf_geom_t *g) {
    snprintf(g_html, sizeof(g_html), "%s", html);
    wf_site_set_geom(g);
    wf_site_set_scroll(5000);
    TEST_ASSERT_EQUAL(0, wf_site_prepare_ext(&g_d, g_html, (int)strlen(g_html), 640, 480, 0, 0));
    wf_site_set_scroll(0);
    wf_site_set_geom(0);
}

#define NAMED(sel) "<style>" KF sel " { view-timeline: --b } #i { animation: k 1000ms linear; animation-timeline: --b }</style>"

/* which element is the body, which one is the inactive root */
static void test_l106_body_which(void) {
    int a;
    /* a top-level body without html is the body: it reads the body record */
    body_rec(1, 1);
    prep_raw(NAMED("body") "<body><p id=i>i</p></body>", &g_g);
    TEST_ASSERT_EQUAL_STRING(" style=\"color:#bdbdbd\"", style_of("i"));
    /* a top-level div is not the body, and its view timeline is active
     * (only the html root is inactive): it reads its box by index */
    prep_raw(NAMED("#a") "<div id=a><p id=i>i</p></div>", &g_g);
    TEST_ASSERT_EQUAL_STRING("", style_of("i"));
    TEST_ASSERT_EQUAL(1, g_d.geom_need);
    a = node("a");
    TEST_ASSERT_TRUE(a >= 0);
    g_g.box[a] = g_g.body;
    g_g.body.ok = 0;
    prep_raw(NAMED("#a") "<div id=a><p id=i>i</p></div>", &g_g);
    TEST_ASSERT_EQUAL_STRING(" style=\"color:#bdbdbd\"", style_of("i"));
    /* a body under a top-level div is not the body */
    body_rec(1, 1);
    prep_raw(NAMED("body") "<div><body id=x><p id=i>i</p></body></div>", &g_g);
    TEST_ASSERT_EQUAL_STRING("", style_of("i"));
    /* nor a body under an html that is not the root */
    prep_raw(NAMED("body") "<div><html><body id=x><p id=i>i</p></body></html></div>", &g_g);
    TEST_ASSERT_EQUAL_STRING("", style_of("i"));
    TEST_ASSERT_EQUAL(1, g_d.geom_need);
    /* an html that is not the root is a subject like any element */
    prep_raw(NAMED("#y") "<div><html id=y><p id=i>i</p></html></div>", &g_g);
    TEST_ASSERT_EQUAL_STRING("", style_of("i"));
    TEST_ASSERT_EQUAL(1, g_d.geom_need);
    g_g.box[node("y")] = g_g.body;
    prep_raw(NAMED("#y") "<div><html id=y><p id=i>i</p></html></div>", &g_g);
    TEST_ASSERT_EQUAL_STRING(" style=\"color:#bdbdbd\"", style_of("i"));
}

int main(void) {
    unity_init();
    RUN_TEST(test_l106_interp_space);
    RUN_TEST(test_l106_overshoot);
    RUN_TEST(test_l106_add_srgb);
    RUN_TEST(test_l106_keyframes);
    RUN_TEST(test_l106_body_subject);
    RUN_TEST(test_l106_body_which);
    unity_print_results();
    unity_cleanup();
    return unity_stats.tests_failed == 0 ? 0 : 1;
}
