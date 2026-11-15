/* Lot 107 (US-031): the reader's relative heights and max-height on box
 * scrollers laid out by the engine, the scroll-padding of the root as the
 * auto insets of a view timeline, and the body's own style written by the
 * reader (wf_html, wf_sitecss, webeng). Hand cases next to the generated
 * test_lot107_*.c; expected values follow docs/lot107_plan.md. */
#include "../../framework/unity.h"
#include <string.h>
#include <stdio.h>
#include "webeng.h"
#include "wf_html.h"

static we_doc_t g_doc;
static wf_geom_t g_g;
static char g_html[8192], g_out[65536], g_o[200];

static void lay(we_doc_t *d, const char *html, int vh) {
    TEST_ASSERT_EQUAL(0, we_parse(d, html, (int)strlen(html)));
    TEST_ASSERT_EQUAL(0, we_style(d));
    TEST_ASSERT_EQUAL(0, we_layout_viewport(d, 320, vh));
}

#define KF "@keyframes k { from { color: #000000 } to { color: #ffffff } } "
#define FILL "<p>line</p><p>line</p><p>line</p><p>line</p><p>line</p><p>line</p><p>line</p><p>line</p><p>line</p><p>line</p>"

static int reduce(const char *css, const char *body, int marks) {
    wf_html_stats_t st;
    int n;
    snprintf(g_html, sizeof(g_html), "<html><head><style>" KF "%s</style></head><body><main>%s</main></body></html>", css, body);
    wf_html_set_sitecss(1);
    wf_html_set_geommarks(marks);
    n = wf_html_reduce(g_html, (int)strlen(g_html), 0, g_out, 65536, &st);
    wf_html_set_geommarks(0);
    wf_html_set_sitecss(0);
    return n;
}

static const char *box(int k) {
    const wf_geom_box_t *b = &g_g.box[k];
    if (!b->ok) return "none";
    snprintf(g_o, sizeof(g_o), "%d+%d vp %d sv %d own %d/%d/%d", b->b_off, b->b_size, b->vp_b, b->sv, b->s_vp, b->s_sc,
             b->s_rg);
    return g_o;
}

/* the body's own style is written (Lot 106 wrote it bare) */
static void test_l107_body_style(void) {
    TEST_ASSERT_TRUE(reduce("body { color: #102030 }", "<p>x</p>", 0) > 0);
    TEST_ASSERT_NOT_NULL(strstr(g_out, "<body style=\"color:#102030\">"));
    /* no style: bare */
    TEST_ASSERT_TRUE(reduce("p { color: #102030 }", "<p>x</p>", 0) > 0);
    TEST_ASSERT_NOT_NULL(strstr(g_out, "<body>"));
    /* the body's own animation on the root scroll timeline, at pm 5000 */
    wf_html_set_cssscroll(5000);
    TEST_ASSERT_TRUE(reduce("body { animation: k 1s linear; animation-timeline: scroll(root) }", "<p>x</p>", 0) > 0);
    wf_html_set_cssscroll(0);
    TEST_ASSERT_NOT_NULL(strstr(g_out, "<body style=\"color:#808080\">"));
    TEST_ASSERT_NULL(strstr(g_out, "<body>"));
    lay(&g_doc, g_out, 480);
    /* the reader off: a bare body (nothing kept from the last pass) */
    {
        wf_html_stats_t st;
        TEST_ASSERT_TRUE(wf_html_reduce(g_html, (int)strlen(g_html), 0, g_out, 65536, &st) > 0);
        TEST_ASSERT_NOT_NULL(strstr(g_out, "<body>"));
    }
}

/* a box scroller 10em high at a 10 px font size, read by scroll(nearest):
 * the reader passes 100 px, the engine lays it out 100 px high */
static void test_l107_em_height(void) {
    const char *css = "#o { overflow: auto; height: 10em; font-size: 10px } "
                      "#i { animation: k 1s linear; animation-timeline: scroll(nearest) }";
    TEST_ASSERT_TRUE(reduce(css, "<div id=o><p id=i>i</p>" FILL "</div>", 1) > 0);
    TEST_ASSERT_NOT_NULL(strstr(g_out, "font-size:10px;overflow-y:auto;height:100px"));
    lay(&g_doc, g_out, 480);
    memset(&g_g, 0, sizeof(g_g));
    TEST_ASSERT_EQUAL(1, we_geom_feed(&g_doc, "wg", &g_g));
    TEST_ASSERT_EQUAL(100, g_g.box[4].b_size);
    TEST_ASSERT_EQUAL(1, g_g.box[4].sv);
    TEST_ASSERT_EQUAL(100, g_g.box[4].s_vp);
    TEST_ASSERT_TRUE(g_g.box[4].s_rg > 0);
    /* 25vh of 480 */
    reduce("#o { overflow: scroll; height: 25vh } #i { animation: k 1s linear; animation-timeline: scroll(nearest) }",
           "<div id=o><p id=i>i</p>" FILL "</div>", 1);
    TEST_ASSERT_NOT_NULL(strstr(g_out, "style=\"overflow-y:scroll;height:120px\""));
    lay(&g_doc, g_out, 480);
    (void)we_geom_feed(&g_doc, "wg", &g_g);
    TEST_ASSERT_EQUAL(120, g_g.box[4].s_vp);
    /* calc() of rem and px, the root at 20 px: 2rem + 10px = 50 px */
    reduce("html { font-size: 20px } #o { overflow: auto; height: calc(2rem + 10px) } "
           "#i { animation: k 1s linear; animation-timeline: scroll(nearest) }", "<div id=o><p id=i>i</p>" FILL "</div>", 1);
    TEST_ASSERT_NOT_NULL(strstr(g_out, "overflow-y:auto;height:50px"));
    lay(&g_doc, g_out, 480);
    (void)we_geom_feed(&g_doc, "wg", &g_g);
    TEST_ASSERT_EQUAL(50, g_g.box[4].s_vp);
}

/* max-height: a scroll box as high as its content up to the limit */
static void test_l107_max_height(void) {
    const char *css = "#o { overflow: auto; max-height: 5em } "
                      "#i { animation: k 1s linear; animation-timeline: scroll(nearest) }";
    reduce(css, "<div id=o><p id=i>i</p>" FILL "</div>", 1);
    TEST_ASSERT_NOT_NULL(strstr(g_out, "style=\"overflow-y:auto;max-height:80px\""));
    lay(&g_doc, g_out, 480);
    memset(&g_g, 0, sizeof(g_g));
    TEST_ASSERT_EQUAL(1, we_geom_feed(&g_doc, "wg", &g_g));
    TEST_ASSERT_EQUAL(80, g_g.box[4].b_size);
    TEST_ASSERT_EQUAL(1, g_g.box[4].sv);
    TEST_ASSERT_EQUAL(80, g_g.box[4].s_vp);
    TEST_ASSERT_TRUE(g_g.box[4].s_rg > 0);
    /* short content: below the limit, nothing to scroll */
    reduce(css, "<div id=o><p id=i>i</p></div>", 1);
    lay(&g_doc, g_out, 480);
    memset(&g_g, 0, sizeof(g_g));
    (void)we_geom_feed(&g_doc, "wg", &g_g);
    TEST_ASSERT_TRUE(g_g.box[4].b_size > 0 && g_g.box[4].b_size < 80);
    TEST_ASSERT_EQUAL(0, g_g.box[4].s_rg);
    /* height and max-height: both passed, the smaller wins in the engine */
    reduce("#o { overflow: auto; height: 200px; max-height: 3em } "
           "#i { animation: k 1s linear; animation-timeline: scroll(nearest) }", "<div id=o><p id=i>i</p>" FILL "</div>", 1);
    TEST_ASSERT_NOT_NULL(strstr(g_out, "overflow-y:auto;height:200px;max-height:48px"));
    lay(&g_doc, g_out, 480);
    (void)we_geom_feed(&g_doc, "wg", &g_g);
    TEST_ASSERT_EQUAL(48, g_g.box[4].b_size);
}

/* the root's scroll-padding as the auto insets of #i's view(): cover
 * [S - 480 + 50, E - 100] at pm 3000 */
static void test_l107_root_padding(void) {
    static char first[65536];
    const char *css = "html { scroll-padding: 100px 0 50px } #i { animation: k 1000ms linear; animation-timeline: view() }";
    const char *page = FILL FILL "<p id=i>i</p>" FILL FILL FILL;
    wf_geom_box_t b;
    long long cs, ce;
    int ms, c;
    char want[64];
    TEST_ASSERT_TRUE(reduce(css, page, 1) > 0);
    TEST_ASSERT_EQUAL(1, wf_html_sitecss_geom_need());
    memcpy(first, g_out, sizeof(first));
    lay(&g_doc, first, 480);
    memset(&g_g, 0, sizeof(g_g));
    TEST_ASSERT_EQUAL(1, we_geom_feed(&g_doc, "wg", &g_g));
    {
        int k, at = -1;
        for (k = 0; k < WF_GEOM_MAX; k++) if (g_g.box[k].ok) at = k;
        TEST_ASSERT_TRUE(at >= 0);
        b = g_g.box[at];
    }
    TEST_ASSERT_EQUAL(1, b.root);
    TEST_ASSERT_TRUE(g_g.rg_y > 0);
    cs = (long long)b.b_off - 480 + 50;
    ce = (long long)b.b_off + b.b_size - 100;
    ms = (int)((1000LL * (g_g.rg_y * 3000LL - cs * 10000)) / ((ce - cs) * 10000));
    TEST_ASSERT_TRUE(ms > 0 && ms < 1000);
    c = (255 * ms + 500) / 1000;
    snprintf(want, sizeof(want), " style=\"color:#%02x%02x%02x\">i</p>", c, c, c);
    wf_html_set_cssgeom(&g_g);
    wf_html_set_cssscroll(3000);
    reduce(css, page, 1);
    TEST_ASSERT_NOT_NULL(strstr(g_out, want));
    /* without the padding: the plain cover range gives another colour */
    reduce("#i { animation: k 1000ms linear; animation-timeline: view() }", page, 1);
    TEST_ASSERT_NULL(strstr(g_out, want));
    TEST_ASSERT_NOT_NULL(strstr(g_out, " style=\"color:#"));
    wf_html_set_cssgeom(0);
    wf_html_set_cssscroll(0);
}

int main(void) {
    unity_init();
    RUN_TEST(test_l107_body_style);
    RUN_TEST(test_l107_em_height);
    RUN_TEST(test_l107_max_height);
    RUN_TEST(test_l107_root_padding);
    unity_print_results();
    unity_cleanup();
    return unity_stats.tests_failed == 0 ? 0 : 1;
}
