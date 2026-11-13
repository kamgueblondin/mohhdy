/* Lot 91 (US-031): the reader sheet passes explicit radial radii, angles
 * in gradians, background-clip and background-origin (longhands and the
 * box keywords of the background shorthand). Hand-written cases next to
 * the generated suites (test_lot91_bgradial.c, test_lot91_bgbox.c). */
#include "../../framework/unity.h"
#include <string.h>
#include <stdio.h>
#include "wf_sitecss.h"
#include "wf_cssbg.h"

static wf_site_t g_d;
static char g_o[1400];
static char g_html[8192];
static char g_v[WF_CSSBG_VAL];
static wf_cssbg_t g_bg;

static void prep(const char *css, const char *body) {
    snprintf(g_html, sizeof(g_html), "<html><head><style>%s</style></head><body>%s</body></html>", css, body);
    TEST_ASSERT_EQUAL(0, wf_site_prepare(&g_d, g_html, (int)strlen(g_html)));
}

static const wf_site_node_t *by_id(const char *id) {
    int i;
    for (i = 0; i < g_d.nnodes; i++) if (!strcmp(g_d.node[i].id, id)) return &g_d.node[i];
    return &g_d.node[0];
}

static const char *style_of(const char *id) {
    g_o[0] = 0;
    wf_site_style_attr(&g_d, by_id(id)->start, g_o, (int)sizeof(g_o));
    return g_o;
}

static const char *grad(const char *v) {
    int r = wf_cssbg_gradient(v, (int)strlen(v), g_v, (int)sizeof(g_v));
    return r > 0 ? g_v : "REFUSED";
}

#define L(k) (WF_SITE_K_##k - WF_SITE_K_BGIMG)

static void test_kinds_and_bounds(void) {
    /* two more pool kinds after list-style-position; all and var moved */
    TEST_ASSERT_EQUAL(23, WF_SITE_K_BGCLIP);
    TEST_ASSERT_EQUAL(24, WF_SITE_K_BGORIGIN);
    TEST_ASSERT_EQUAL(WF_SITE_K_BGORIGIN, WF_SITE_K_MAX);
    TEST_ASSERT_EQUAL(WF_SITE_K_MAX + 1, WF_SITE_K_ALL);
    TEST_ASSERT_EQUAL(WF_SITE_K_BGORIGIN - WF_SITE_K_BGIMG + 1, WF_SITE_LKINDS);
}

static void test_radial_radii(void) {
    TEST_ASSERT_EQUAL_STRING("radial-gradient(circle 40px, red, blue)", grad("radial-gradient(circle 40px, red, blue)"));
    TEST_ASSERT_EQUAL_STRING("radial-gradient(40px circle, red, blue)", grad("radial-gradient(40PX  Circle, red, blue)"));
    TEST_ASSERT_EQUAL_STRING("radial-gradient(40px, red, blue)", grad("radial-gradient(40px, red, blue)"));
    TEST_ASSERT_EQUAL_STRING("radial-gradient(ellipse 30px 20% at left top, red, blue)",
                             grad("radial-gradient(ellipse 30px 20% at left top, red, blue)"));
    TEST_ASSERT_EQUAL_STRING("radial-gradient(30% 20%, red, blue)", grad("radial-gradient(30% 20%, red, blue)"));
    TEST_ASSERT_EQUAL_STRING("radial-gradient(0 0 ellipse, red, blue)", grad("radial-gradient(0 0 ellipse, red, blue)"));
    TEST_ASSERT_EQUAL_STRING("radial-gradient(circle 0, red, blue)", grad("radial-gradient(circle 0, red, blue)"));
    /* refusals: what the engine does not draw, and the CSS grammar */
    TEST_ASSERT_EQUAL_STRING("REFUSED", grad("radial-gradient(circle 40%, red, blue)"));      /* circle in % */
    TEST_ASSERT_EQUAL_STRING("REFUSED", grad("radial-gradient(40%, red, blue)"));             /* implied circle */
    TEST_ASSERT_EQUAL_STRING("REFUSED", grad("radial-gradient(circle 10px 20px, red, blue)")); /* two radii */
    TEST_ASSERT_EQUAL_STRING("REFUSED", grad("radial-gradient(ellipse 10px, red, blue)"));    /* one radius */
    TEST_ASSERT_EQUAL_STRING("REFUSED", grad("radial-gradient(10px circle 20px, red, blue)")); /* split radii */
    TEST_ASSERT_EQUAL_STRING("REFUSED", grad("radial-gradient(closest-side 10px, red, blue)"));
    TEST_ASSERT_EQUAL_STRING("REFUSED", grad("radial-gradient(10px closest-side, red, blue)"));
    TEST_ASSERT_EQUAL_STRING("REFUSED", grad("radial-gradient(-10px, red, blue)"));
    TEST_ASSERT_EQUAL_STRING("REFUSED", grad("radial-gradient(10px 20px 30px, red, blue)"));
    TEST_ASSERT_EQUAL_STRING("REFUSED", grad("radial-gradient(10em, red, blue)"));
    TEST_ASSERT_EQUAL_STRING("REFUSED", grad("radial-gradient(circle circle 4px, red, blue)"));
    TEST_ASSERT_EQUAL_STRING("REFUSED", grad("radial-gradient(4px at, red, blue)"));
    TEST_ASSERT_EQUAL_STRING("REFUSED", grad("conic-gradient(red, blue)"));
}

static void test_gradians(void) {
    TEST_ASSERT_EQUAL_STRING("linear-gradient(100grad, red, blue)", grad("linear-gradient(100GRAD, red, blue)"));
    TEST_ASSERT_EQUAL_STRING("linear-gradient(0grad, red, blue)", grad("linear-gradient(0grad, red, blue)"));
    TEST_ASSERT_EQUAL_STRING("linear-gradient(-300grad, red, blue)", grad("linear-gradient(-300grad, red, blue)"));
    TEST_ASSERT_EQUAL_STRING("linear-gradient(4000grad, red, blue)", grad("linear-gradient(4000grad, red, blue)"));
    TEST_ASSERT_EQUAL_STRING("REFUSED", grad("linear-gradient(4100grad, red, blue)"));
    TEST_ASSERT_EQUAL_STRING("REFUSED", grad("linear-gradient(50grad, red, blue)"));
    TEST_ASSERT_EQUAL_STRING("REFUSED", grad("linear-gradient(100.0grad, red, blue)"));
    /* Lot 92 (converted): a radian quarter turn is taken as 90deg */
    TEST_ASSERT_EQUAL_STRING("linear-gradient(90deg, red, blue)", grad("linear-gradient(1.5708rad, red, blue)"));
    TEST_ASSERT_EQUAL_STRING("REFUSED", grad("linear-gradient(45deg, red, blue)"));
}

static void test_box_longhands(void) {
    TEST_ASSERT(wf_cssbg_box("Content-Box", g_v, (int)sizeof(g_v)) > 0);
    TEST_ASSERT_EQUAL_STRING("content-box", g_v);
    TEST_ASSERT(wf_cssbg_box("padding-box, padding-box,padding-box", g_v, (int)sizeof(g_v)) > 0);
    TEST_ASSERT_EQUAL_STRING("padding-box", g_v);
    TEST_ASSERT_EQUAL(0, wf_cssbg_box("padding-box, border-box", g_v, (int)sizeof(g_v)));
    TEST_ASSERT_EQUAL_STRING("", g_v);
    TEST_ASSERT_EQUAL(0, wf_cssbg_box("text", g_v, (int)sizeof(g_v)));
    TEST_ASSERT_EQUAL(0, wf_cssbg_box("border-box border-box", g_v, (int)sizeof(g_v)));
    TEST_ASSERT_EQUAL(0, wf_cssbg_box("a, a, a, a, a", g_v, (int)sizeof(g_v)));
    TEST_ASSERT_EQUAL(0, wf_cssbg_box("border-box", g_v, 4));      /* no room */
    TEST_ASSERT_EQUAL(0, wf_cssbg_box(0, g_v, (int)sizeof(g_v)));
}

static void test_shorthand_boxes(void) {
    TEST_ASSERT_EQUAL(1, wf_cssbg_shorthand("url(a.png) content-box", &g_bg));
    TEST_ASSERT_EQUAL_STRING("content-box", g_bg.origin);
    TEST_ASSERT_EQUAL_STRING("content-box", g_bg.clip);
    TEST_ASSERT_EQUAL(1, g_bg.has_clip);
    /* two: the origin, then the clip */
    TEST_ASSERT_EQUAL(1, wf_cssbg_shorthand("border-box red padding-box", &g_bg));
    TEST_ASSERT_EQUAL_STRING("border-box", g_bg.origin);
    TEST_ASSERT_EQUAL_STRING("padding-box", g_bg.clip);
    TEST_ASSERT_EQUAL_STRING("red", g_bg.color);
    /* none: the initial pair, not given */
    TEST_ASSERT_EQUAL(1, wf_cssbg_shorthand("url(a.png)", &g_bg));
    TEST_ASSERT_EQUAL_STRING("padding-box", g_bg.origin);
    TEST_ASSERT_EQUAL_STRING("border-box", g_bg.clip);
    TEST_ASSERT_EQUAL(0, g_bg.has_origin);
    /* layers that agree (the second says nothing but ends the same) */
    TEST_ASSERT_EQUAL(1, wf_cssbg_shorthand("url(a.png) padding-box border-box, url(b.png)", &g_bg));
    TEST_ASSERT_EQUAL(2, g_bg.layers);
    TEST_ASSERT_EQUAL(1, g_bg.has_clip);
    /* layers that do not: the engine has one box per element */
    TEST_ASSERT_EQUAL(0, wf_cssbg_shorthand("url(a.png) content-box, url(b.png)", &g_bg));
    TEST_ASSERT_EQUAL(0, wf_cssbg_shorthand("url(a.png) border-box border-box border-box", &g_bg));
    TEST_ASSERT_EQUAL(0, wf_cssbg_shorthand("url(a.png) text", &g_bg));
}

static void test_sheet_cascade(void) {
    prep("p { background-clip: content-box; background-origin: border-box }"
         ".a { background: url(a.png) padding-box } #c { background-clip: padding-box, content-box }"
         "#d { background-clip: content-box !important } p#d { background-clip: border-box }",
         "<p id=t>t</p><p id=a class=a>a</p><p id=c>c</p><p id=d>d</p>");
    TEST_ASSERT_EQUAL_STRING(" style=\"background-clip:content-box;background-origin:border-box\"", style_of("t"));
    /* the shorthand sets both from its one keyword */
    TEST_ASSERT_EQUAL_STRING(" style=\"background-image:url(a.png);background-clip:padding-box;background-origin:padding-box\"",
                             style_of("a"));
    /* a list that does not agree is dropped: p's values stay */
    TEST_ASSERT_EQUAL_STRING(" style=\"background-clip:content-box;background-origin:border-box\"", style_of("c"));
    TEST_ASSERT_EQUAL_STRING(" style=\"background-clip:content-box;background-origin:border-box\"", style_of("d"));
}

static void test_shorthand_resets_boxes(void) {
    /* a later shorthand without a box puts both back to initial (not written) */
    prep("p { background-clip: content-box } p { background: red }", "<p id=t>t</p>");
    TEST_ASSERT_EQUAL_STRING(" style=\"background-color:#ff0000\"", style_of("t"));
    TEST_ASSERT_EQUAL(1, by_id("t")->lv_val[L(BGCLIP)]);
    TEST_ASSERT_EQUAL(1, by_id("t")->lv_val[L(BGORIGIN)]);
}

static void test_keywords(void) {
    prep("div { background-clip: content-box } div p { background-clip: inherit } div i { background-origin: inherit }"
         "p.u { background-clip: unset } p.a { all: initial }",
         "<div id=d><p id=p>p</p><i id=i>i</i><p id=u class=u>u</p><p id=a class=a>a</p></div>");
    TEST_ASSERT_EQUAL_STRING(" style=\"background-clip:content-box\"", style_of("p"));
    /* the parent has no origin: nothing to inherit */
    TEST_ASSERT_EQUAL_STRING("", style_of("i"));
    /* unset: they do not inherit, so initial */
    TEST_ASSERT_EQUAL_STRING("", style_of("u"));
    /* all: initial writes the Lot 87 initial values, never a box */
    TEST_ASSERT(strstr(style_of("a"), "background-clip") == 0);
    TEST_ASSERT_EQUAL(1, by_id("a")->lv_val[L(BGCLIP)]);
    /* not inherited without a keyword */
    prep("div { background-origin: content-box }", "<div id=d><p id=p>p</p></div>");
    TEST_ASSERT_EQUAL_STRING("", style_of("p"));
}

static void test_report_names(void) {
    static char rep[4096];
    prep("#t { background: url(a.png) content-box padding-box }", "<p id=t>t</p>");
    TEST_ASSERT(wf_site_report(&g_d, rep, (int)sizeof(rep)) > 0);
    TEST_ASSERT(strstr(rep, " bgclip padding-box") != 0);
    TEST_ASSERT(strstr(rep, " bgorigin content-box") != 0);
}

static void test_inline_style_wins(void) {
    /* the element's own style attribute is the engine's; the sheet adds its values in front */
    prep("p { background: radial-gradient(circle 12px at center, red, blue) border-box content-box }", "<p id=t>t</p>");
    TEST_ASSERT_EQUAL_STRING(" style=\"background-image:radial-gradient(circle 12px at center, red, blue);"
                             "background-clip:content-box;background-origin:border-box\"", style_of("t"));
}

int main(void) {
    unity_init();
    RUN_TEST(test_kinds_and_bounds);
    RUN_TEST(test_radial_radii);
    RUN_TEST(test_gradians);
    RUN_TEST(test_box_longhands);
    RUN_TEST(test_shorthand_boxes);
    RUN_TEST(test_sheet_cascade);
    RUN_TEST(test_shorthand_resets_boxes);
    RUN_TEST(test_keywords);
    RUN_TEST(test_report_names);
    RUN_TEST(test_inline_style_wins);
    unity_print_results();
    unity_cleanup();
    return unity_stats.tests_failed == 0 ? 0 : 1;
}
