/* Lot 110 (US-031): the body box scroller laid out by the engine from the
 * reader's style (html's overflow not visible), its percentage heights
 * made px against html's, opacity passed on, and the mark of a box in
 * the unmarked body box scroller (we_geom_feed sm WF_GEOM_SM_BODY;
 * wf_html, wf_sitecss, webeng). Hand cases next to the generated
 * test_lot110_*.c; expected values follow docs/lot110_plan.md. */
#include "../../framework/unity.h"
#include <string.h>
#include <stdio.h>
#include "webeng.h"
#include "wf_html.h"

static we_doc_t g_doc;
static we_forms_t g_forms;
static wf_geom_t g_g;
static char g_html[8192], g_out[65536];
static we_node_t g_none;
static int n_keep;

static void lay(const char *html) {
    we_forms_init(&g_forms);
    we_set_forms(&g_doc, &g_forms);
    TEST_ASSERT_EQUAL(0, we_parse(&g_doc, html, (int)strlen(html)));
    TEST_ASSERT_EQUAL(0, we_style(&g_doc));
    TEST_ASSERT_EQUAL(0, we_layout_viewport(&g_doc, 640, 480));
}

static const we_node_t *tag(const char *t) {
    int i;
    for (i = 0; i < (int)g_doc.nnodes; i++)
        if (!strcmp(g_doc.nodes[i].tag, t)) return &g_doc.nodes[i];
    g_none.h = -1;
    return &g_none;
}

/* the k-th element (from 0) with this tag */
static const we_node_t *nth(const char *t, int k) {
    int i;
    for (i = 0; i < (int)g_doc.nnodes; i++)
        if (!strcmp(g_doc.nodes[i].tag, t) && !k--) return &g_doc.nodes[i];
    g_none.h = -1;
    g_none.transparency = 99;
    return &g_none;
}

#define KF "@keyframes k { from { color: #000000 } to { color: #ffffff } } "
#define TL "#i { animation: k 1s linear; animation-timeline: scroll(nearest) } "
#define ROWS "<p>a</p><p>b</p><p>c</p><p>d</p><p>e</p><p>f</p><p>g</p><p>h</p><p>i</p><p>j</p><p>k</p><p>l</p>" \
             "<p>m</p><p>n</p><p>o</p><p>p</p><p>q</p><p>r</p><p>s</p><p>t</p><p>u</p><p>v</p><p>w</p><p>x</p>"

/* the reader's page (with the geometry marks), laid out at 640 x 480 */
static void reduce(const char *css, const char *body) {
    wf_html_stats_t st;
    int n;
    snprintf(g_html, sizeof(g_html), "<html><head><style>" KF "%s</style></head><body>%s</body></html>", css, body);
    wf_html_set_sitecss(1);
    n = wf_html_reduce(g_html, (int)strlen(g_html), 0, g_out, 65536, &st);
    wf_html_set_sitecss(0);
    TEST_ASSERT_TRUE(n > 0);
    g_out[n] = 0;
    lay(g_out);
}

/* the body as a box scroller: its height, a scroll range */
static void test_l110_body_layout(void) {
    reduce("html { overflow: hidden } body { overflow: auto; height: 300px } " TL, "<p id=i>x</p>" ROWS);
    TEST_ASSERT_NOT_NULL(strstr(g_out, "<body style=\"overflow-y:auto;height:300px\">"));
    TEST_ASSERT_TRUE(tag("body")->overflow_scroll & 2);
    TEST_ASSERT_EQUAL(300, tag("body")->h);
    /* html visible: the body is not passed, it grows with its content */
    reduce("body { overflow: auto; height: 300px } " TL, "<p id=i>x</p>" ROWS);
    TEST_ASSERT_NOT_NULL(strstr(g_out, "<body>"));
    TEST_ASSERT_FALSE(tag("body")->overflow_scroll & 2);
    TEST_ASSERT_TRUE(tag("body")->h > 300);
    /* html and body at 100%: the viewport height */
    reduce("html { overflow: hidden; height: 100% } body { overflow: auto; height: 100%; margin: 0 } " TL,
           "<p id=i>x</p>" ROWS);
    TEST_ASSERT_NOT_NULL(strstr(g_out, "overflow-y:auto;height:480px"));
    TEST_ASSERT_EQUAL(480, tag("body")->h);
    TEST_ASSERT_TRUE(tag("body")->overflow_scroll & 2);
}

/* opacity reaches the engine (transparency 100 - opacity%) */
static void test_l110_opacity(void) {
    reduce("#o { opacity: 0.25 } #b { opacity: 100% }", "<div id=o>o</div><div id=b>b</div>");
    TEST_ASSERT_NOT_NULL(strstr(g_out, "<p style=\"opacity:25%\">o</p><p style=\"opacity:100%\">b</p>"));
    TEST_ASSERT_EQUAL(75, nth("p", 0)->transparency);
    TEST_ASSERT_EQUAL(0, nth("p", 1)->transparency);
    wf_html_set_csstime(500);
    reduce("@keyframes f { from { opacity: 0 } to { opacity: 0.5 } } #o { animation: f 1s linear }", "<div id=o>o</div>");
    wf_html_set_csstime(0);
    TEST_ASSERT_NOT_NULL(strstr(g_out, "<p style=\"opacity:25%\">o</p>"));
    TEST_ASSERT_EQUAL(75, tag("p")->transparency);
}

/* we_geom_feed: a box in the unmarked body box scroller says
 * WF_GEOM_SM_BODY; the body record has its own scroll state */
static void test_l110_geom_sm(void) {
    lay("<html><body style='margin:0;height:200px;overflow:auto'><div id=wg4 style='height:100px'></div>"
        "<div id=wg6 style='height:300px'></div></body></html>");
    TEST_ASSERT_EQUAL(2, we_geom_feed(&g_doc, "wg", &g_g));
    TEST_ASSERT_EQUAL(WF_GEOM_SM_BODY, g_g.box[4].sm);
    TEST_ASSERT_EQUAL(WF_GEOM_SM_BODY, g_g.box[6].sm);
    TEST_ASSERT_EQUAL(0, g_g.box[4].root);
    TEST_ASSERT_EQUAL(1, g_g.body.sv);
    TEST_ASSERT_EQUAL(200, g_g.body.s_vp);
    TEST_ASSERT_EQUAL(200, g_g.body.s_rg);
    TEST_ASSERT_EQUAL(0, g_g.body.sm);
    TEST_ASSERT_EQUAL(1, g_g.body.root);
    /* a marked body keeps its mark */
    lay("<html><body id=wg7 style='margin:0;height:200px;overflow:auto'><div id=wg4 style='height:300px'></div></body></html>");
    TEST_ASSERT_EQUAL(2, we_geom_feed(&g_doc, "wg", &g_g));
    TEST_ASSERT_EQUAL(8, g_g.box[4].sm);
    /* an unmarked box scroller inside: 0 (not the body's) */
    lay("<html><body style='margin:0;height:200px;overflow:auto'><div style='height:50px;overflow:auto'>"
        "<div id=wg4 style='height:300px'></div></div></body></html>");
    TEST_ASSERT_EQUAL(1, we_geom_feed(&g_doc, "wg", &g_g));
    TEST_ASSERT_EQUAL(0, g_g.box[4].sm);
    TEST_ASSERT_EQUAL(0, g_g.box[4].root);
    /* a body that does not scroll: the root */
    lay("<html><body style='margin:0'><div id=wg4 style='height:300px'></div></body></html>");
    TEST_ASSERT_EQUAL(1, we_geom_feed(&g_doc, "wg", &g_g));
    TEST_ASSERT_EQUAL(0, g_g.box[4].sm);
    TEST_ASSERT_EQUAL(1, g_g.box[4].root);
    TEST_ASSERT_EQUAL(0, g_g.body.sv);
    /* a body that is not the root element's child is not the body */
    lay("<html><div><body style='height:200px;overflow:auto'><div id=wg4 style='height:300px'></div></body></div></html>");
    (void)we_geom_feed(&g_doc, "wg", &g_g);
    TEST_ASSERT_TRUE(g_g.box[4].sm != WF_GEOM_SM_BODY);
}

/* the whole path: reader, layout, geometry, reader again */
static void test_l110_round_trip(void) {
    wf_html_stats_t st;
    int n, k;
    static char out2[65536];
    snprintf(g_html, sizeof(g_html), "<html><head><style>" KF "html { overflow: hidden } body { overflow: auto; height: 200px; "
             "margin: 0 } #v { animation: k 1s linear; animation-timeline: view() } " TL "</style></head><body>"
             "<p id=i>x</p>" ROWS "<p id=v>v</p>" ROWS "</body></html>");
    wf_html_set_sitecss(1);
    wf_html_set_geommarks(1);
    n = wf_html_reduce(g_html, (int)strlen(g_html), 0, g_out, 65536, &st);
    TEST_ASSERT_TRUE(n > 0);
    g_out[n] = 0;
    lay(g_out);
    TEST_ASSERT_TRUE(tag("body")->overflow_scroll & 2);
    k = we_geom_feed(&g_doc, "wg", &g_g);
    TEST_ASSERT_TRUE(k >= 1);
    TEST_ASSERT_EQUAL(1, g_g.body.sv);
    TEST_ASSERT_TRUE(g_g.body.s_rg > 0);
    for (k = 0; k < WF_GEOM_MAX; k++) if (g_g.box[k].ok) TEST_ASSERT_EQUAL(WF_GEOM_SM_BODY, g_g.box[k].sm);
    /* at the top: #i black, #v below the scrollport (before its range) */
    wf_html_set_cssgeom(&g_g);
    n = wf_html_reduce(g_html, (int)strlen(g_html), 0, out2, 65536, &st);
    TEST_ASSERT_TRUE(n > 0);
    out2[n] = 0;
    TEST_ASSERT_NOT_NULL(strstr(out2, "<p style=\"color:#000000\">x</p>"));
    /* scrolled half way along the body's range (made 1000 px) */
    g_g.body.s_rg = 1000;
    g_g.body.s_sc = 500;
    n = wf_html_reduce(g_html, (int)strlen(g_html), 0, out2, 65536, &st);
    out2[n] = 0;
    TEST_ASSERT_NOT_NULL(strstr(out2, "<p style=\"color:#808080\">x</p>"));
    wf_html_set_cssgeom(0);
    wf_html_set_geommarks(0);
    wf_html_set_sitecss(0);
}

/* a parse with the forms kept (lay starts them again) */
static void relay(const char *html) {
    TEST_ASSERT_EQUAL(0, we_parse(&g_doc, html, (int)strlen(html)));
    TEST_ASSERT_EQUAL(0, we_style(&g_doc));
    TEST_ASSERT_EQUAL(0, we_layout_viewport(&g_doc, 640, 480));
}

/* the body box scroller's offset is kept in the forms under " body"
 * when the body has no id (the GUI lays the page out again after a
 * click), under its id otherwise */
static void test_l110_persist(void) {
    static char page[4096];
    snprintf(page, sizeof(page), "<html><body style=\"margin:0px 0px 0px 0px;overflow-y:auto;height:100px\">%s%s</body></html>",
             ROWS, ROWS);
    lay(page);
    TEST_ASSERT_EQUAL(2, we_box_scroll_click(&g_doc, 637, 50));
    TEST_ASSERT_EQUAL(1, g_forms.nscroll);
    TEST_ASSERT_EQUAL_STRING(" body", g_forms.scroll_id[0]);
    TEST_ASSERT_TRUE(g_forms.scroll_y[0] > 0);
    TEST_ASSERT_EQUAL(g_forms.scroll_y[0], tag("body")->box_scroll_y);
    n_keep = g_forms.scroll_y[0];
    relay(page);
    TEST_ASSERT_EQUAL(n_keep, tag("body")->box_scroll_y);
    /* a body with an id keeps it under the id */
    snprintf(page, sizeof(page), "<html><body id=bb style=\"margin:0px 0px 0px 0px;overflow-y:auto;height:100px\">%s%s</body></html>",
             ROWS, ROWS);
    lay(page);
    TEST_ASSERT_EQUAL(2, we_box_scroll_click(&g_doc, 637, 50));
    TEST_ASSERT_EQUAL_STRING("bb", g_forms.scroll_id[0]);
    relay(page);
    TEST_ASSERT_EQUAL(g_forms.scroll_y[0], tag("body")->box_scroll_y);
    /* a div without an id: no entry, a new parse starts at 0 */
    snprintf(page, sizeof(page), "<html><body style=\"margin:0px 0px 0px 0px\"><div style=\"overflow-y:auto;height:100px\">%s</div>"
             "</body></html>", ROWS);
    lay(page);
    TEST_ASSERT_EQUAL(2, we_box_scroll_click(&g_doc, 637, 50));
    TEST_ASSERT_EQUAL(0, g_forms.nscroll);
    relay(page);
    TEST_ASSERT_EQUAL(0, tag("div")->box_scroll_y);
    we_set_forms(&g_doc, 0);
}

int main(void) {
    unity_init();
    RUN_TEST(test_l110_body_layout);
    RUN_TEST(test_l110_opacity);
    RUN_TEST(test_l110_geom_sm);
    RUN_TEST(test_l110_round_trip);
    RUN_TEST(test_l110_persist);
    unity_print_results();
    unity_cleanup();
    return unity_stats.tests_failed == 0 ? 0 : 1;
}
