/* Lot 96 (US-031): the reader takes repeating gradients, conic stops in
 * hundredths of a degree and radian conic "from", writes radians to 1/100
 * degree, and checks a registered <image> gradient as the background
 * reader does. Hand-written cases next to the generated suites
 * (test_lot96_bgval.c, test_lot96_accept.c, test_lot96_sheet.c). */
#include "../../framework/unity.h"
#include <string.h>
#include <stdio.h>
#include "wf_sitecss.h"
#include "wf_cssbg.h"
#include "wf_cssprop.h"

static wf_site_t g_d;
static char g_o[1400];
static char g_html[8192];
static char g_v[WF_CSSBG_VAL];
static wf_cssbg_t g_bg;
static wf_cssprop_t g_p;

static int mp(const char *syn, const char *v) {
    memset(&g_p, 0, sizeof(g_p));
    g_p.syntax = (unsigned char)wf_cssprop_syntax_parse(syn, (int)strlen(syn), &g_p);
    if (g_p.syntax == WF_CSSPROP_BAD) return -1;
    return wf_cssprop_match_prop(&g_p, v);
}

static void prep(const char *css, const char *body) {
    snprintf(g_html, sizeof(g_html), "<html><head><style>%s</style></head><body>%s</body></html>", css, body);
    TEST_ASSERT_EQUAL(0, wf_site_prepare(&g_d, g_html, (int)strlen(g_html)));
}

static const char *style_of(const char *id) {
    int i;
    g_o[0] = 0;
    for (i = 0; i < g_d.nnodes; i++)
        if (!strcmp(g_d.node[i].id, id)) wf_site_style_attr(&g_d, g_d.node[i].start, g_o, (int)sizeof(g_o));
    return g_o;
}

static const char *grad(const char *v) {
    int r = wf_cssbg_gradient(v, (int)strlen(v), g_v, (int)sizeof(g_v));
    return r > 0 ? g_v : "REFUSED";
}

static void test_repeating(void) {
    TEST_ASSERT_EQUAL_STRING("repeating-linear-gradient(red, blue 10%)", grad("  Repeating-Linear-Gradient( red ,  blue 10% ) "));
    TEST_ASSERT_EQUAL_STRING("repeating-radial-gradient(circle 4px at 1px 2px, red, blue 10%)",
                             grad("repeating-radial-gradient(circle 4px at 1px 2px, red, blue 10%)"));
    TEST_ASSERT_EQUAL_STRING("repeating-conic-gradient(from 10.5deg, red 0deg, blue 30.25deg)",
                             grad("repeating-conic-gradient(from 10.5deg, red 0deg, blue 30.25deg)"));
    /* the period: the last stop past the first */
    TEST_ASSERT_EQUAL_STRING("REFUSED", grad("repeating-linear-gradient(red 10%, blue 10%)"));
    TEST_ASSERT_EQUAL_STRING("REFUSED", grad("repeating-linear-gradient(red 10%, blue 5%)"));
    TEST_ASSERT_EQUAL_STRING("REFUSED", grad("repeating-linear-gradient(red 100%, blue)"));
    /* lime 40% is raised to 50%, blue has the default 100% */
    TEST_ASSERT_EQUAL_STRING("repeating-linear-gradient(red 50%, lime 40%, blue)", grad("repeating-linear-gradient(red 50%, lime 40%, blue)"));
    TEST_ASSERT_EQUAL_STRING("repeating-linear-gradient(red 50%, lime 60%, blue 0%)", grad("repeating-linear-gradient(red 50%, lime 60%, blue 0%)"));
    TEST_ASSERT_EQUAL_STRING("REFUSED", grad("repeating-linear-gradient(red 50%, lime 40%, blue 0%)"));
    TEST_ASSERT_EQUAL_STRING("REFUSED", grad("repeating-conic-gradient(red 36deg, blue 10%)"));
    TEST_ASSERT_EQUAL_STRING("repeating-conic-gradient(red 36deg, blue 10.1%)", grad("repeating-conic-gradient(red 36deg, blue 10.1%)"));
    /* stop positions: 10.5deg is 292/10000 of a turn (2.9% is 290),
     * 0.35deg 10/10000 (rounded), 0.34deg 9/10000; 10.5% is 1050 */
    TEST_ASSERT_EQUAL_STRING("REFUSED", grad("repeating-conic-gradient(red 10.5deg, blue 2.9%)"));
    TEST_ASSERT_EQUAL_STRING("repeating-conic-gradient(red 10.5deg, blue 3%)", grad("repeating-conic-gradient(red 10.5deg, blue 3%)"));
    TEST_ASSERT_EQUAL_STRING("REFUSED", grad("repeating-conic-gradient(red 0.35deg, blue 0.1%)"));
    TEST_ASSERT_EQUAL_STRING("repeating-conic-gradient(red 0.34deg, blue 0.1%)", grad("repeating-conic-gradient(red 0.34deg, blue 0.1%)"));
    TEST_ASSERT_EQUAL_STRING("repeating-linear-gradient(red 10%, blue 10.5%)", grad("repeating-linear-gradient(red 10%, blue 10.5%)"));
    TEST_ASSERT_EQUAL_STRING("REFUSED", grad("repeating-linear-gradient(red, blue 0%)"));
    TEST_ASSERT_EQUAL_STRING("repeating-linear-gradient(red, blue 0.1%)", grad("repeating-linear-gradient(red, blue 0.1%)"));
    /* a gradient that does not repeat keeps a period of 0 */
    TEST_ASSERT_EQUAL_STRING("linear-gradient(red 10%, blue 10%)", grad("linear-gradient(red 10%, blue 10%)"));
    /* the name */
    TEST_ASSERT_EQUAL_STRING("REFUSED", grad("repeating-gradient(red, blue)"));
    TEST_ASSERT_EQUAL_STRING("REFUSED", grad("repeating -linear-gradient(red, blue)"));
    TEST_ASSERT_EQUAL_STRING("REFUSED", grad("repeating-repeating-linear-gradient(red, blue)"));
    /* the written form past 127 characters: refused */
    TEST_ASSERT_EQUAL_STRING("REFUSED", grad("repeating-radial-gradient(circle closest-corner at 54% 76%, rgb(187, 223, 167), "
                                             "rgb(89, 69, 225) 77.1%, rgb(91, 251, 3), gray 60.1%)"));
}

static void test_degrees(void) {
    TEST_ASSERT_EQUAL_STRING("linear-gradient(57.3deg, red, blue)", grad("linear-gradient(1rad, red, blue)"));
    TEST_ASSERT_EQUAL_STRING("linear-gradient(90deg, red, blue)", grad("linear-gradient(1.5708rad, red, blue)"));
    TEST_ASSERT_EQUAL_STRING("linear-gradient(-28.65deg, red, blue)", grad("linear-gradient(-0.5rad, red, blue)"));
    TEST_ASSERT_EQUAL_STRING("linear-gradient(0.01deg, red, blue)", grad("linear-gradient(0.0002rad, red, blue)"));
    TEST_ASSERT_EQUAL_STRING("linear-gradient(0deg, red, blue)", grad("linear-gradient(0.0000rad, red, blue)"));
    TEST_ASSERT_EQUAL_STRING("linear-gradient(0.1deg, red, blue)", grad("linear-gradient(0.0017rad, red, blue)"));
    TEST_ASSERT_EQUAL_STRING("conic-gradient(from 57.3deg at left, red, blue)", grad("conic-gradient(from 1RAD at left, red, blue)"));
    TEST_ASSERT_EQUAL_STRING("conic-gradient(from 0deg, red, blue)", grad("conic-gradient(from -0rad, red, blue)"));
    TEST_ASSERT_EQUAL_STRING("REFUSED", grad("conic-gradient(from 1.rad, red, blue)"));
    TEST_ASSERT_EQUAL_STRING("REFUSED", grad("conic-gradient(from 100rad, red, blue)"));
    TEST_ASSERT_EQUAL_STRING("REFUSED", grad("conic-gradient(from 1rad at, red, blue)"));
    /* conic stops: at most two decimals, 0..360 */
    TEST_ASSERT_EQUAL_STRING("conic-gradient(red 0.5deg, blue 359.99deg)", grad("conic-gradient(red 0.5deg, blue 359.99deg)"));
    TEST_ASSERT_EQUAL_STRING("conic-gradient(red 360.0deg, blue)", grad("conic-gradient(red 360.0deg, blue)"));
    TEST_ASSERT_EQUAL_STRING("REFUSED", grad("conic-gradient(red 360.01deg, blue)"));
    TEST_ASSERT_EQUAL_STRING("REFUSED", grad("conic-gradient(red 1.255deg, blue)"));
    TEST_ASSERT_EQUAL_STRING("REFUSED", grad("conic-gradient(red 1.deg, blue)"));
    TEST_ASSERT_EQUAL_STRING("REFUSED", grad("conic-gradient(red .5deg, blue)"));
    TEST_ASSERT_EQUAL_STRING("REFUSED", grad("conic-gradient(red 1000deg, blue)"));
    TEST_ASSERT_EQUAL_STRING("REFUSED", grad("linear-gradient(red 0.5deg, blue)"));
}

static void test_layers_and_shorthand(void) {
    TEST_ASSERT(wf_cssbg_image("url(a.png), repeating-linear-gradient(red, blue 10%)", g_v, (int)sizeof(g_v)) > 0);
    TEST_ASSERT_EQUAL_STRING("url(a.png), repeating-linear-gradient(red, blue 10%)", g_v);
    TEST_ASSERT_EQUAL(0, wf_cssbg_image("repeating-linear-gradient(red, blue 10%), url(a.png)", g_v, (int)sizeof(g_v)));
    TEST_ASSERT_EQUAL(0, wf_cssbg_image("repeating-linear-gradient(red 10%, blue 10%)", g_v, (int)sizeof(g_v)));
    TEST_ASSERT(wf_cssbg_shorthand("repeating-radial-gradient(red, blue 10%) no-repeat center", &g_bg));
    TEST_ASSERT_EQUAL_STRING("repeating-radial-gradient(red, blue 10%)", g_bg.image);
    TEST_ASSERT_EQUAL_STRING("no-repeat", g_bg.repeat);
    /* "repeating-" alone is no image word: a colour, refused */
    TEST_ASSERT_EQUAL(0, wf_cssbg_shorthand("repeating-none", &g_bg));
    TEST_ASSERT_EQUAL(0, wf_cssbg_shorthand("repeating-url(a.png)", &g_bg));
    prep("#a { background: repeating-conic-gradient(from 0.5rad, red, blue 25%) } "
         "#b { background-image: repeating-linear-gradient(red 10%, blue 10%) } "
         "#c { background-image: url(a.png), repeating-linear-gradient(1rad, red, blue 10%) }",
         "<div id=a>a</div><div id=b>b</div><div id=c>c</div>");
    TEST_ASSERT_EQUAL_STRING(" style=\"background-image:repeating-conic-gradient(from 28.65deg, red, blue 25%)\"", style_of("a"));
    TEST_ASSERT_EQUAL_STRING("", style_of("b"));
    TEST_ASSERT_EQUAL_STRING(" style=\"background-image:url(a.png), repeating-linear-gradient(57.3deg, red, blue 10%)\"", style_of("c"));
}

static void test_registered_image(void) {
    TEST_ASSERT_EQUAL(1, wf_cssprop_match(WF_CSSPROP_IMAGE, "repeating-linear-gradient(red, blue 10%)"));
    TEST_ASSERT_EQUAL(0, wf_cssprop_match(WF_CSSPROP_IMAGE, "repeating-linear-gradient(red 10%, blue 10%)"));
    TEST_ASSERT_EQUAL(0, wf_cssprop_match(WF_CSSPROP_IMAGE, "linear-gradient(red)"));
    TEST_ASSERT_EQUAL(0, wf_cssprop_match(WF_CSSPROP_IMAGE, "linear-gradient(red, 10px)"));
    TEST_ASSERT_EQUAL(0, wf_cssprop_match(WF_CSSPROP_IMAGE, "conic-gradient(red 400deg, blue)"));
    TEST_ASSERT_EQUAL(1, wf_cssprop_match(WF_CSSPROP_IMAGE, "conic-gradient(from 1rad, red 0.25deg, blue)"));
    TEST_ASSERT_EQUAL(1, wf_cssprop_match(WF_CSSPROP_IMAGE, "url(a.png)"));
    /* a list of images: each one is checked */
    TEST_ASSERT_EQUAL(1, mp("'<image>+'", "linear-gradient(red, blue) repeating-radial-gradient(red, blue 5%)"));
    TEST_ASSERT_EQUAL(0, mp("'<image>+'", "linear-gradient(red, blue) repeating-radial-gradient(red 5%, blue 5%)"));
    TEST_ASSERT_EQUAL(1, mp("'<image># | <color>'", "url(a.png), repeating-conic-gradient(red, blue 1deg)"));
    TEST_ASSERT_EQUAL(0, mp("'<image># | <color>'", "url(a.png), repeating-conic-gradient(red 1.111deg, blue)"));
}

int main(void) {
    unity_init();
    RUN_TEST(test_repeating);
    RUN_TEST(test_degrees);
    RUN_TEST(test_layers_and_shorthand);
    RUN_TEST(test_registered_image);
    unity_print_results();
    unity_cleanup();
    return unity_stats.tests_failed == 0 ? 0 : 1;
}
