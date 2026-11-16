/* Lot 109 (US-031): heights past 2000 px in the engine (parse_height, up
 * to WE_LAYOUT_MAX), and the reader's box scroller lengths laid out: a
 * percentage inside calc()/min()/max()/clamp() against the containing
 * block, ex, ch and lh (wf_html, wf_sitecss, webeng). Hand cases next to
 * the generated test_lot109_*.c; expected values follow
 * docs/lot109_plan.md. */
#include "../../framework/unity.h"
#include <string.h>
#include <stdio.h>
#include "webeng.h"
#include "wf_html.h"

static we_doc_t g_doc;
static char g_html[8192], g_out[65536];
static we_node_t g_none;

static void lay(const char *html) {
    g_none.h = g_none.w = g_none.y = -1;
    TEST_ASSERT_EQUAL(0, we_parse(&g_doc, html, (int)strlen(html)));
    TEST_ASSERT_EQUAL(0, we_style(&g_doc));
    TEST_ASSERT_EQUAL(0, we_layout_viewport(&g_doc, 640, 480));
}

static const we_node_t *nd(const char *id) {
    int i;
    for (i = 0; i < (int)g_doc.nnodes; i++)
        if (!strcmp(g_doc.nodes[i].id, id)) return &g_doc.nodes[i];
    return &g_none;     /* h -1: no such node */
}

static int has_diag(const char *t) {
    int i;
    for (i = 0; i < (int)g_doc.ndiag; i++) if (strstr(g_doc.diag[i], t)) return 1;
    return 0;
}

/* the k-th box scroller along y (k from 0) of the reduced page */
static const we_node_t *scroller(int k) {
    int i;
    for (i = 0; i < (int)g_doc.nnodes; i++)
        if ((g_doc.nodes[i].overflow_scroll & 2) && !k--) return &g_doc.nodes[i];
    return &g_none;     /* h -1: no such node */
}

static const we_node_t *xscroller(void) {
    int i;
    for (i = 0; i < (int)g_doc.nnodes; i++)
        if (g_doc.nodes[i].overflow_scroll & 1) return &g_doc.nodes[i];
    return &g_none;     /* h -1: no such node */
}

#define KF "@keyframes k { from { color: #000000 } to { color: #ffffff } } "
#define TL "#i, #j { animation: k 1s linear; animation-timeline: scroll(nearest) } "
#define NEST "<div id=p><p id=j>j</p><div id=o><p id=i>x</p><p>a</p><p>b</p></div></div>"

/* the reader's page, laid out at 640 x 480 */
static void reduce(const char *css, const char *body) {
    wf_html_stats_t st;
    int n;
    g_none.h = g_none.w = g_none.y = -1;
    snprintf(g_html, sizeof(g_html), "<html><head><style>" KF "%s</style></head><body><main>%s</main></body></html>", css, body);
    wf_html_set_sitecss(1);
    n = wf_html_reduce(g_html, (int)strlen(g_html), 0, g_out, 65536, &st);
    wf_html_set_sitecss(0);
    TEST_ASSERT_TRUE(n > 0);
    TEST_ASSERT_EQUAL(0, we_parse(&g_doc, g_out, n));
    TEST_ASSERT_EQUAL(0, we_style(&g_doc));
    TEST_ASSERT_EQUAL(0, we_layout_viewport(&g_doc, 640, 480));
}

/* height, min-height and max-height in px up to 16000 */
static void test_l109_tall_heights(void) {
    lay("<body style='margin:0'><div id=a style='height:5000px'></div><div id=b style='height:2001px'></div>"
        "<div id=c style='min-height:3000px'></div><div id=d style='height:9000px;max-height:2500px'></div>"
        "<div id=e style='height:16000px'></div></body>");
    TEST_ASSERT_EQUAL(5000, nd("a")->h);
    TEST_ASSERT_EQUAL(2001, nd("b")->h);
    TEST_ASSERT_EQUAL(5000, nd("b")->y);
    TEST_ASSERT_EQUAL(3000, nd("c")->h);
    TEST_ASSERT_EQUAL(3000, nd("c")->min_height);
    TEST_ASSERT_EQUAL(2500, nd("d")->h);
    TEST_ASSERT_EQUAL(16000, nd("e")->height);
    TEST_ASSERT_FALSE(has_diag("height refused"));
    TEST_ASSERT_FALSE(has_diag("height limit refused"));
    /* past 16000 px, or a percentage past 2000: refused (the earlier
     * value stays) */
    lay("<body style='margin:0'><div id=a style='height:30px;height:16001px'></div>"
        "<div id=b style='max-height:16001px'></div><div id=c style='height:2001%'></div>"
        "<div id=d style='height:40px;height:16000.5px'></div></body>");
    TEST_ASSERT_EQUAL(30, nd("a")->h);
    TEST_ASSERT_EQUAL(-1, nd("b")->max_height);
    TEST_ASSERT_EQUAL(0, nd("c")->height_percent);
    TEST_ASSERT_EQUAL(40, nd("d")->h);
    TEST_ASSERT_TRUE(has_diag("height refused"));
    TEST_ASSERT_TRUE(has_diag("height limit refused"));
    /* other lengths keep 2000 */
    lay("<body style='margin:0'><div id=a style='display:flex;column-gap:2001px'></div></body>");
    TEST_ASSERT_TRUE(nd("a")->col_gap < 2001);
}

/* a px height after a math height replaces it (and the reverse) */
static void test_l109_math_then_px(void) {
    lay("<body style='margin:0'><div id=a style='height:calc(10px * 2);height:3000px'></div>"
        "<div id=b style='min-height:calc(5px + 5px);min-height:2500px'></div>"
        "<div id=c style='max-height:calc(5px);max-height:4000px;height:4500px'></div>"
        "<div id=d style='height:3000px;height:calc(10px * 2)'></div></body>");
    TEST_ASSERT_EQUAL(3000, nd("a")->h);
    TEST_ASSERT_EQUAL(2500, nd("b")->h);
    TEST_ASSERT_EQUAL(4000, nd("c")->h);
    TEST_ASSERT_EQUAL(20, nd("d")->h);
}

/* boxes past the layout bound stay there (positions clamp at 16000) */
static void test_l109_stack_clamp(void) {
    lay("<body style='margin:0'><div id=a style='height:16000px'></div><div id=b style='height:16000px'></div>"
        "<div id=c style='height:12000px'></div></body>");
    TEST_ASSERT_EQUAL(16000, nd("b")->y);
    TEST_ASSERT_EQUAL(16000, nd("c")->y);
    TEST_ASSERT_EQUAL(12000, nd("c")->h);
}

/* the reader's expression against the containing block: an outer box
 * scroller of 300 px (read by #j) holds #o */
static void test_l109_pct_flow(void) {
    reduce("#p { overflow: auto; height: 300px } #o { overflow: auto; height: calc(50% - 10px) } " TL, NEST);
    TEST_ASSERT_EQUAL(300, scroller(0)->h);
    TEST_ASSERT_EQUAL(140, scroller(1)->h);
    reduce("#p { overflow: auto; height: 300px } #o { overflow: auto; height: min(50%, 100px) } " TL, NEST);
    TEST_ASSERT_EQUAL(100, scroller(1)->h);
    reduce("#p { overflow: auto; height: 300px } #o { overflow: auto; height: clamp(1lh, 20%, 2em) } " TL, NEST);
    TEST_ASSERT_EQUAL(32, scroller(1)->h);           /* 60 capped at 32 */
    reduce("#p { overflow: auto; height: 300px } #o { overflow: auto; height: 20px; max-height: calc(5% + 1ex) } " TL, NEST);
    TEST_ASSERT_EQUAL(20, scroller(1)->h);           /* 15 + 5 = 20 */
    reduce("#p { overflow: auto; height: 300px } #o { overflow: auto; height: 20px; min-height: calc(30% - 1ch) } " TL, NEST);
    TEST_ASSERT_EQUAL(82, scroller(1)->h);           /* 90 - 8 */
    reduce("#p { overflow: auto; height: 300px } #o { overflow: auto; height: calc(-10% + 10px) } " TL, NEST);
    TEST_ASSERT_EQUAL(0, scroller(1)->h);            /* negative: 0 */
    /* the outer height in lh, the inner a percentage of it */
    reduce("#p { overflow: auto; height: 20lh } #o { overflow: auto; height: calc(50% + 2ex) } " TL, NEST);
    TEST_ASSERT_EQUAL(200, scroller(0)->h);
    TEST_ASSERT_EQUAL(110, scroller(1)->h);
    /* an auto containing block: the math refused, the content height */
    reduce("#o { overflow: auto; height: 30px } " TL, "<div id=o><p id=i>x</p><p>a</p><p>b</p></div>");
    TEST_ASSERT_EQUAL(30, scroller(0)->h);
    {
        int content;
        reduce("#o { overflow: auto; height: 9999px } " TL, "<div id=o><p id=i>x</p><p>a</p><p>b</p></div>");
        reduce("#o { overflow: auto; max-height: 9999px } " TL, "<div id=o><p id=i>x</p><p>a</p><p>b</p></div>");
        content = scroller(0)->h;
        reduce("#o { overflow: auto; height: calc(50% - 10px) } " TL, "<div id=o><p id=i>x</p><p>a</p><p>b</p></div>");
        TEST_ASSERT_EQUAL(content, scroller(0)->h);
    }
}

/* widths: the containing block is definite */
static void test_l109_width_flow(void) {
    const char *body = "<div id=o><p id=i style=\"white-space:nowrap\">aaaaaaaaaa bbbbbbbbbb cccccccccc dddddddddd "
                       "eeeeeeeeee ffffffffff gggggggggg hhhhhhhhhh iiiiiiiiii jjjjjjjjjj kkkkkkkkkk</p></div>";
    int full;
    reduce("#o { overflow-x: auto; width: 100% } #i { animation: k 1s linear; animation-timeline: scroll(nearest inline) }", body);
    full = xscroller()->w;
    TEST_ASSERT_TRUE(full > 300);
    reduce("#o { overflow-x: auto; width: calc(100% - 2em) } #i { animation: k 1s linear; animation-timeline: scroll(nearest inline) }", body);
    TEST_ASSERT_EQUAL(full - 32, xscroller()->w);
    reduce("#o { overflow-x: auto; width: min(50%, 30ch) } #i { animation: k 1s linear; animation-timeline: scroll(nearest inline) }", body);
    TEST_ASSERT_EQUAL(full / 2 < 240 ? full / 2 : 240, xscroller()->w);
    reduce("#o { overflow-x: auto; width: 25ch } #i { animation: k 1s linear; animation-timeline: scroll(nearest inline) }", body);
    TEST_ASSERT_EQUAL(200, xscroller()->w);
}

int main(void) {
    unity_init();
    RUN_TEST(test_l109_tall_heights);
    RUN_TEST(test_l109_math_then_px);
    RUN_TEST(test_l109_stack_clamp);
    RUN_TEST(test_l109_pct_flow);
    RUN_TEST(test_l109_width_flow);
    unity_print_results();
    unity_cleanup();
    return unity_stats.tests_failed == 0 ? 0 : 1;
}
