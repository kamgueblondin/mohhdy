/* Lot 111 (US-031): the reader's Lot 111 values laid out by the engine:
 * mixed min()/max()/clamp() nested or in an arithmetic, lh against the
 * element's line-height, the body's percentage width against html's
 * width given as an expression, and opacity from calc(), var() and
 * revert-layer (wf_html, wf_sitecss, webeng). Hand cases next to the
 * generated test_lot111_*.c; expected values follow docs/lot111_plan.md. */
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


/* the height of the k-th box scroller along y */
static int sh(int k) {
    int i;
    for (i = 0; i < (int)g_doc.nnodes; i++)
        if ((g_doc.nodes[i].overflow_scroll & 2) && !k--) return g_doc.nodes[i].h;
    return -1;
}

#define TJ "#j { animation: k 1s linear; animation-timeline: scroll(nearest) } "
#define PO "<main><div id=p><p id=j>j</p><div id=o><p id=i>i</p>" ROWS "</div></div></main>"

/* mixed values: the engine reads the reader's nested expression */
static void test_l111_mixed_layout(void) {
    reduce("#p { overflow: auto; height: 300px } #o { overflow: auto; height: calc(min(50%, 100px) + max(10%, 20px)) } "
           TL TJ, PO);
    TEST_ASSERT_NOT_NULL(strstr(g_out, "height:min(max(60%, calc(50% + 20px)), max(calc(10% + 100px), 120px))"));
    TEST_ASSERT_EQUAL(300, sh(0));
    TEST_ASSERT_EQUAL(130, sh(1));
    reduce("#p { overflow: auto; height: 300px } #o { overflow: auto; height: calc(200px - clamp(10px, 20%, 100px)) } "
           TL TJ, PO);
    TEST_ASSERT_NOT_NULL(strstr(g_out, "height:min(190px, max(calc(-20% + 200px), 100px))"));
    TEST_ASSERT_EQUAL(140, sh(1));
    reduce("#p { overflow: auto; height: 200px } #o { overflow: auto; height: 100px; max-height: calc(min(10%, 50px) * 3) } "
           TL TJ, PO);
    TEST_ASSERT_NOT_NULL(strstr(g_out, "max-height:min(30%, 150px)"));
    TEST_ASSERT_EQUAL(60, sh(1));
    /* five arguments: valid CSS, so the last height wins, but the reader
     * cannot pass it (the engine takes four): #o has no height and is
     * not passed as a box scroller (before Lot 111 the engine refused
     * the written min() and dropped the height alone) */
    reduce("#p { overflow: auto; height: 200px } #o { overflow: auto; height: 50px; height: min(90%, 1px, 80%, 2px, 70%) } "
           TL TJ, PO);
    TEST_ASSERT_NULL(strstr(g_out, "min(90%"));
    TEST_ASSERT_NULL(strstr(g_out, "height:50px"));
    TEST_ASSERT_EQUAL(200, sh(0));
    TEST_ASSERT_EQUAL(-1, sh(1));
}

/* lh against the element's line-height */
static void test_l111_lh_layout(void) {
    reduce("#p { overflow: auto; height: 300px } #o { overflow: auto; line-height: 2; height: 3lh } " TL TJ, PO);
    TEST_ASSERT_EQUAL(96, sh(1));
    reduce("#p { overflow: auto; height: 300px; line-height: 25px } #o { overflow: auto; height: calc(2lh + 10%) } "
           TL TJ, PO);
    TEST_ASSERT_NOT_NULL(strstr(g_out, "height:calc(10% + 50px)"));
    TEST_ASSERT_EQUAL(80, sh(1));
}

/* the body's percentage width against html's width as an expression */
static void test_l111_body_width(void) {
    reduce("html { overflow: hidden; width: calc(100% - 40px) } body { overflow: auto; width: 50%; height: 100px; "
           "margin: 0 } " TL, "<p id=i>x</p>" ROWS);
    TEST_ASSERT_NOT_NULL(strstr(g_out, "width:300px"));
    TEST_ASSERT_EQUAL(300, tag("body")->w);
    TEST_ASSERT_EQUAL(100, tag("body")->h);
    reduce("html { overflow: hidden; height: min(50%, 300px) } body { overflow: auto; height: 50%; margin: 0 } " TL,
           "<p id=i>x</p>" ROWS);
    TEST_ASSERT_EQUAL(120, tag("body")->h);
}

/* opacity from calc(), var() and revert-layer */
static void test_l111_opacity(void) {
    reduce("#o { opacity: calc(0.125 * 2) } body { --a: 0.4 } #b { opacity: var(--a) } "
           "@layer a { #c { opacity: 0.2 } } #c { opacity: 0.9; opacity: revert-layer }",
           "<div id=o>o</div><div id=b>b</div><div id=c>c</div>");
    TEST_ASSERT_NOT_NULL(strstr(g_out, "<p style=\"opacity:25%\">o</p><p style=\"opacity:40%\">b</p>"
                                       "<p style=\"opacity:20%\">c</p>"));
    TEST_ASSERT_EQUAL(75, nth("p", 0)->transparency);
    TEST_ASSERT_EQUAL(60, nth("p", 1)->transparency);
    TEST_ASSERT_EQUAL(80, nth("p", 2)->transparency);
}

int main(void) {
    unity_init();
    RUN_TEST(test_l111_mixed_layout);
    RUN_TEST(test_l111_lh_layout);
    RUN_TEST(test_l111_body_width);
    RUN_TEST(test_l111_opacity);
    unity_print_results();
    unity_cleanup();
    return unity_stats.tests_failed == 0 ? 0 : 1;
}
