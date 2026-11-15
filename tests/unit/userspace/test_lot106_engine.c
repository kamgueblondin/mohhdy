/* Lot 106 (US-031): the body box in the layout geometry
 * (webeng_geom.inc, wf_geom_t.body) and the two passes with a view
 * timeline whose subject is the body (wf_html, wf_sitecss). Hand cases
 * next to the generated test_lot106_geom.c; expected values follow
 * docs/lot106_plan.md. */
#include "../../framework/unity.h"
#include <string.h>
#include <stdio.h>
#include "webeng.h"
#include "wf_html.h"

static we_doc_t g_doc, g_doc2;
static wf_geom_t g_g;
static char g_html[8192], g_out[65536], g_o[200];

static void lay(we_doc_t *d, const char *html, int vh) {
    TEST_ASSERT_EQUAL(0, we_parse(d, html, (int)strlen(html)));
    TEST_ASSERT_EQUAL(0, we_style(d));
    TEST_ASSERT_EQUAL(0, we_layout_viewport(d, 320, vh));
}

static const char *body_box(void) {
    const wf_geom_box_t *b = &g_g.body;
    if (!b->ok) return "none";
    snprintf(g_o, sizeof(g_o), "root %d %d+%d %d+%d vp %d sc %d rg %d sv %d sm %d", b->root, b->b_off, b->b_size, b->i_off,
             b->i_size, b->vp_b, b->sc_b, b->rg_b, b->sv, b->sm);
    return g_o;
}

/* the body record from the layout: the default 8 px margin, a styled
 * body, a hidden one, the marked boxes beside it */
static void test_l106_geom_body(void) {
    lay(&g_doc, "<html><body><div style='height:100px'></div></body></html>", 480);
    TEST_ASSERT_EQUAL(0, we_geom_feed(&g_doc, "wg", &g_g));
    TEST_ASSERT_EQUAL_STRING("root 1 8+100 8+304 vp 480 sc 0 rg 0 sv 0 sm 0", body_box());
    /* 8 + 1000 + 8 = 1016: range 536, scrolled 100 */
    lay(&g_doc, "<html><body><div id=wg3 style='height:1000px'></div></body></html>", 480);
    TEST_ASSERT_EQUAL(100, we_scroll(&g_doc, 100));
    TEST_ASSERT_EQUAL(1, we_geom_feed(&g_doc, "wg", &g_g));
    TEST_ASSERT_EQUAL_STRING("root 1 8+1000 8+304 vp 480 sc 100 rg 536 sv 0 sm 0", body_box());
    TEST_ASSERT_EQUAL(1, g_g.box[3].ok);
    TEST_ASSERT_EQUAL(8, g_g.box[3].b_off);
    TEST_ASSERT_EQUAL(1, g_g.n);
    /* margins, padding and a border: the border box */
    lay(&g_doc, "<html><body style='margin:20px 10px 5px 30px;padding:4px;border:2px solid red'>"
                "<div style='height:50px'></div></body></html>", 480);
    (void)we_geom_feed(&g_doc, "wg", &g_g);
    TEST_ASSERT_EQUAL_STRING("root 1 20+62 30+280 vp 480 sc 0 rg 0 sv 0 sm 0", body_box());
    /* a hidden body has no box; the root state is still there */
    lay(&g_doc, "<html><body style='display:none'><div style='height:900px'></div></body></html>", 480);
    memset(&g_g, 0x5a, sizeof(g_g));
    TEST_ASSERT_EQUAL(0, we_geom_feed(&g_doc, "wg", &g_g));
    TEST_ASSERT_EQUAL_STRING("none", body_box());
    TEST_ASSERT_EQUAL(320, g_g.vw);
    /* a body with overflow is not a box scroller of its own (its
     * overflow is the viewport's) */
    lay(&g_doc, "<html><body style='height:100px;overflow:auto'><div style='height:300px'></div></body></html>", 480);
    (void)we_geom_feed(&g_doc, "wg", &g_g);
    TEST_ASSERT_EQUAL(1, g_g.body.root);
    /* a body that is not a child of the root element is not the body */
    lay(&g_doc, "<html><div><body><div style='height:50px'></div></body></div></html>", 480);
    memset(&g_g, 0x5a, sizeof(g_g));
    (void)we_geom_feed(&g_doc, "wg", &g_g);
    TEST_ASSERT_EQUAL_STRING("none", body_box());
    /* no root element: no body */
    lay(&g_doc, "", 480);
    memset(&g_g, 0x5a, sizeof(g_g));
    (void)we_geom_feed(&g_doc, "wg", &g_g);
    TEST_ASSERT_EQUAL_STRING("none", body_box());
    /* we_geom_feed without a prefix leaves everything zero */
    memset(&g_g, 0x5a, sizeof(g_g));
    TEST_ASSERT_EQUAL(0, we_geom_feed(&g_doc, "", &g_g));
    TEST_ASSERT_EQUAL(0, g_g.body.ok);
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

/* the two passes with the body as the subject of #i's named timeline:
 * no mark (the body is written bare), its record from the layout */
static void test_l106_two_passes_body(void) {
    static char first[65536];
    const char *css = "body { view-timeline: --b } #i { animation: k 1000ms linear; animation-timeline: --b }";
    const wf_geom_box_t *b = &g_g.body;
    long long cs, ce;
    int ms, c;
    char want[64];
    TEST_ASSERT_TRUE(reduce(css, "<p id=i>i</p>" FILL FILL FILL, 1) > 0);
    TEST_ASSERT_NULL(strstr(g_out, "id=\"wg"));
    TEST_ASSERT_NULL(strstr(g_out, "color:#"));
    TEST_ASSERT_NOT_NULL(strstr(g_out, "<body>"));
    TEST_ASSERT_EQUAL(1, wf_html_sitecss_geom_need());
    memcpy(first, g_out, sizeof(first));
    lay(&g_doc, first, 480);
    memset(&g_g, 0, sizeof(g_g));
    TEST_ASSERT_EQUAL(0, we_geom_feed(&g_doc, "wg", &g_g));
    TEST_ASSERT_EQUAL(1, b->ok);
    TEST_ASSERT_EQUAL(1, b->root);
    TEST_ASSERT_EQUAL(480, b->vp_b);
    TEST_ASSERT_EQUAL((int)g_doc.perf.page_h - 480, b->rg_b);
    TEST_ASSERT_TRUE(b->rg_b > 0);
    /* at pm 6000: cover [S - 480, E], linear black to white */
    cs = (long long)b->b_off - 480;
    ce = (long long)b->b_off + b->b_size;
    ms = (int)((1000LL * (b->rg_b * 6000LL - cs * 10000)) / ((ce - cs) * 10000));
    c = (255 * ms + 500) / 1000;
    snprintf(want, sizeof(want), "<p style=\"color:#%02x%02x%02x\">i</p>", c, c, c);
    wf_html_set_cssgeom(&g_g);
    wf_html_set_cssscroll(6000);
    reduce(css, "<p id=i>i</p>" FILL FILL FILL, 1);
    TEST_ASSERT_NOT_NULL(strstr(g_out, want));
    TEST_ASSERT_TRUE(c > 0 && c < 255);
    /* the boxes do not move */
    lay(&g_doc2, g_out, 480);
    TEST_ASSERT_EQUAL(g_doc.perf.page_h, g_doc2.perf.page_h);
    /* html's view timeline: inactive, no geometry asked */
    reduce("html { view-timeline: --h } #i { animation: k 1000ms linear; animation-timeline: --h }",
           "<p id=i>i</p>" FILL FILL FILL, 1);
    TEST_ASSERT_NULL(strstr(g_out, "color:#"));
    TEST_ASSERT_EQUAL(0, wf_html_sitecss_geom_need());
    wf_html_set_cssgeom(0);
    reduce(css, "<p id=i>i</p>" FILL FILL FILL, 1);
    TEST_ASSERT_NULL(strstr(g_out, "color:#"));
    wf_html_set_cssscroll(0);
}

int main(void) {
    unity_init();
    RUN_TEST(test_l106_geom_body);
    RUN_TEST(test_l106_two_passes_body);
    unity_print_results();
    unity_cleanup();
    return unity_stats.tests_failed == 0 ? 0 : 1;
}
