/* Lot 112 (US-031): the reader's Lot 112 values laid out by the engine:
 * line-height passed in the style attribute (whole px within 10..40, or
 * normal), rlh, comparable arguments of mixed min()/max(), var() in
 * opacity keyframes (wf_html, wf_sitecss, webeng). Hand cases next to the
 * generated test_lot112_*.c; expected values follow docs/lot112_plan.md. */
#include "../../framework/unity.h"
#include <string.h>
#include <stdio.h>
#include "webeng.h"
#include "wf_html.h"

static we_doc_t g_doc;
static we_forms_t g_forms;
static char g_html[8192], g_out[65536];
static we_node_t g_none;

static void lay(const char *html, int n) {
    we_forms_init(&g_forms);
    we_set_forms(&g_doc, &g_forms);
    TEST_ASSERT_EQUAL(0, we_parse(&g_doc, html, n));
    TEST_ASSERT_EQUAL(0, we_style(&g_doc));
    TEST_ASSERT_EQUAL(0, we_layout_viewport(&g_doc, 640, 480));
}

/* the k-th element (from 0) with this tag */
static const we_node_t *nth(const char *t, int k) {
    int i;
    for (i = 0; i < (int)g_doc.nnodes; i++)
        if (!strcmp(g_doc.nodes[i].tag, t) && !k--) return &g_doc.nodes[i];
    g_none.h = -1;
    g_none.y = -1;
    return &g_none;
}

#define KF "@keyframes k { from { color: #000000 } to { color: #ffffff } } "
#define TL "#i { animation: k 1s linear; animation-timeline: scroll(nearest) } "

/* the reader's page laid out at 640 x 480 */
static void reduce(const char *css, const char *body) {
    wf_html_stats_t st;
    int n;
    snprintf(g_html, sizeof(g_html), "<html><head><style>" KF "%s</style></head><body>%s</body></html>", css, body);
    wf_html_set_sitecss(1);
    n = wf_html_reduce(g_html, (int)strlen(g_html), 0, g_out, 65536, &st);
    wf_html_set_sitecss(0);
    TEST_ASSERT_TRUE(n > 0);
    lay(g_out, n);
}

/* the height of the k-th box scroller along y */
static int sh(int k) {
    int i;
    for (i = 0; i < (int)g_doc.nnodes; i++)
        if ((g_doc.nodes[i].overflow_scroll & 2) && !k--) return g_doc.nodes[i].h;
    return -1;
}

#define P3 "<p id=a>a</p><p id=b>b</p><p id=c>c</p>"

/* one-line paragraphs are as high as their line height */
static void test_l112_lh_layout(void) {
    reduce("#a { line-height: 30px } #b { line-height: calc(1 + 1) } #c { line-height: calc(100% + 4px) }", P3);
    TEST_ASSERT_NOT_NULL(strstr(g_out, "<p style=\"line-height:30px\">a</p><p style=\"line-height:32px\">b</p>"
                                       "<p style=\"line-height:20px\">c</p>"));
    TEST_ASSERT_EQUAL(30, nth("p", 0)->h);
    TEST_ASSERT_EQUAL(32, nth("p", 1)->h);
    TEST_ASSERT_EQUAL(20, nth("p", 2)->h);
    TEST_ASSERT_EQUAL(nth("p", 0)->y + 30 + 16, nth("p", 1)->y);       /* the 16px margin between them */
    /* the engine's bounds: 10..40 */
    reduce("#a { line-height: 100px } #b { line-height: 0.25 } #c { line-height: 0 }", P3);
    TEST_ASSERT_EQUAL(40, nth("p", 0)->h);
    TEST_ASSERT_EQUAL(10, nth("p", 1)->h);
    TEST_ASSERT_EQUAL(10, nth("p", 2)->h);
    /* the body's px reaches the paragraphs; normal under it is passed */
    reduce("html { line-height: 25px } #b { line-height: normal } #c { font-size: 20px; line-height: normal }", P3);
    TEST_ASSERT_EQUAL(25, nth("p", 0)->h);
    TEST_ASSERT_EQUAL(10, nth("p", 1)->h);
    TEST_ASSERT_EQUAL(13, nth("p", 2)->h);      /* normal of 20px in the engine: (10 * 5 + 2) / 4 */
    /* a number inherits as a number: each paragraph's font */
    reduce("body { line-height: 1.5 } #b { font-size: 24px } #c { font-size: 12px }", P3);
    TEST_ASSERT_EQUAL(24, nth("p", 0)->h);
    TEST_ASSERT_EQUAL(36, nth("p", 1)->h);
    TEST_ASSERT_EQUAL(18, nth("p", 2)->h);
    /* all, revert-layer, var() unset */
    reduce("body { line-height: 22px } @layer a { #b { line-height: 33px } } #a { line-height: 3; all: inherit } "
           "#b { line-height: 3; line-height: revert-layer } #c { line-height: 3; line-height: var(--u) }", P3);
    TEST_ASSERT_EQUAL(22, nth("p", 0)->h);
    TEST_ASSERT_EQUAL(33, nth("p", 1)->h);
    TEST_ASSERT_EQUAL(22, nth("p", 2)->h);
}

/* rlh and lh in box scroller heights next to the line heights passed */
static void test_l112_rlh_layout(void) {
    reduce("html { line-height: 15px } #o { overflow: auto; height: 4rlh; line-height: 2rlh } " TL,
           "<div id=o><p id=i>i</p><p>j</p><p>k</p><p>l</p></div>");
    TEST_ASSERT_NOT_NULL(strstr(g_out, "line-height:30px;overflow-y:auto;height:60px"));
    TEST_ASSERT_EQUAL(60, sh(0));
    TEST_ASSERT_EQUAL(30, nth("p", 1)->h);
    reduce("html { font-size: 20px } #o { overflow: auto; height: calc(2rlh + 10%) } " TL,
           "<div id=o><p id=i>i</p></div>");
    TEST_ASSERT_NOT_NULL(strstr(g_out, "height:calc(10% + 25px)"));
}

/* comparable arguments go: the engine takes what it could not before */
static void test_l112_dom_layout(void) {
    reduce("#p { overflow: auto; height: 200px } #o { overflow: auto; height: min(50%, 120px, 80%, 90px, 150px) } " TL
           "#j { animation: k 1s linear; animation-timeline: scroll(nearest) }",
           "<main><div id=p><p id=j>j</p><div id=o><p id=i>i</p><p>a</p><p>b</p><p>c</p><p>d</p><p>e</p></div></div></main>");
    TEST_ASSERT_NOT_NULL(strstr(g_out, "height:min(50%, 90px)"));
    TEST_ASSERT_EQUAL(90, sh(1));
}

/* opacity keyframes with var() of the animated element */
static void test_l112_op_frames(void) {
    wf_html_set_csstime(500);
    reduce("@keyframes f { from { opacity: var(--a) } to { opacity: calc(0.5 + 0.5) } } #a { --a: 0.2; animation: f 1s linear } "
           "#b { --a: 0.6; animation: f 1s linear }", P3);
    wf_html_set_csstime(0);
    TEST_ASSERT_NOT_NULL(strstr(g_out, "<p style=\"opacity:60%\">a</p><p style=\"opacity:80%\">b</p><p>c</p>"));
    TEST_ASSERT_EQUAL(40, nth("p", 0)->transparency);
    TEST_ASSERT_EQUAL(20, nth("p", 1)->transparency);
}

int main(void) {
    unity_init();
    RUN_TEST(test_l112_lh_layout);
    RUN_TEST(test_l112_rlh_layout);
    RUN_TEST(test_l112_dom_layout);
    RUN_TEST(test_l112_op_frames);
    unity_print_results();
    unity_cleanup();
    return unity_stats.tests_failed == 0 ? 0 : 1;
}
