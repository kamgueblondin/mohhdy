/* Lot 104 (US-031): the layout geometry the engine hands back to the
 * reader (webeng_geom.inc, wf_html geometry marks) and the two passes the
 * browser runs: reduce with marks, lay out, feed the boxes back, reduce
 * again. Hand cases next to the generated test_lot104_geom.c; expected
 * values follow docs/lot104_plan.md (boxes stack by their margin boxes,
 * no margin collapsing; positions from the nearest scroller's content
 * start). */
#include "../../framework/unity.h"
#include <string.h>
#include <stdio.h>
#include "webeng.h"
#include "wf_html.h"

static we_doc_t g_doc, g_doc2;
static we_forms_t g_forms;
static wf_geom_t g_g;
static char g_html[4096], g_out[65536], g_out2[65536], g_o[200];

static void lay(we_doc_t *d, const char *html, int vh) {
    TEST_ASSERT_EQUAL(0, we_parse(d, html, (int)strlen(html)));
    TEST_ASSERT_EQUAL(0, we_style(d));
    TEST_ASSERT_EQUAL(0, we_layout_viewport(d, 320, vh));
}

static void page(const char *body) {
    snprintf(g_html, sizeof(g_html), "<html><body style='margin:0'>%s</body></html>", body);
    lay(&g_doc, g_html, 480);
}

static const char *box(int k) {
    const wf_geom_box_t *b = &g_g.box[k];
    if (!b->ok) return "none";
    snprintf(g_o, sizeof(g_o), "%d %d %d+%d %d+%d vp %d/%d sc %d/%d rg %d/%d", b->root, b->bvert, b->b_off, b->b_size,
             b->i_off, b->i_size, b->vp_b, b->vp_i, b->sc_b, b->sc_i, b->rg_b, b->rg_i);
    return g_o;
}

/* we_geom on single nodes: text nodes, hidden boxes, bounds */
static void test_l104_geom_node(void) {
    wf_geom_box_t b;
    int i;
    page("<div id=a style='height:100px;margin:10px 20px'>t</div><div style='display:none'><p id=h>x</p></div>");
    i = we_find_id(&g_doc, "a");
    TEST_ASSERT_EQUAL(0, we_geom(&g_doc, i, &b));
    TEST_ASSERT_EQUAL(1, b.ok);
    TEST_ASSERT_EQUAL(10, b.b_off);
    TEST_ASSERT_EQUAL(100, b.b_size);
    TEST_ASSERT_EQUAL(20, b.i_off);
    TEST_ASSERT_EQUAL(280, b.i_size);
    /* its text node has no box of its own */
    TEST_ASSERT_TRUE(g_doc.nodes[i + 1].is_text);
    b.ok = 7;
    TEST_ASSERT_EQUAL(-1, we_geom(&g_doc, i + 1, &b));
    TEST_ASSERT_EQUAL(0, b.ok);
    /* inside display:none */
    TEST_ASSERT_EQUAL(-1, we_geom(&g_doc, we_find_id(&g_doc, "h"), &b));
    TEST_ASSERT_EQUAL(-1, we_geom(&g_doc, -1, &b));
    TEST_ASSERT_EQUAL(-1, we_geom(&g_doc, (int)g_doc.nnodes, &b));
    /* even when the slot past the last node looks like an element
     * (mutation witness) */
    g_doc.nodes[g_doc.nnodes] = g_doc.nodes[i];
    TEST_ASSERT_EQUAL(-1, we_geom(&g_doc, (int)g_doc.nnodes, &b));
    TEST_ASSERT_EQUAL(-1, we_geom(0, 0, &b));
    TEST_ASSERT_EQUAL(-1, we_geom(&g_doc, i, 0));
}

/* the marks: wg<index>, no leading zero, below WF_GEOM_MAX, the first one
 * wins */
static void test_l104_geom_marks(void) {
    page("<div id=wg0 style='height:10px'></div><div id=wg0 style='height:20px'></div>"
         "<div id=wg191 style='height:30px'></div><div id=wg07 style='height:5px'></div>"
         "<div id=wg5x style='height:5px'></div><div id=xwg5 style='height:5px'></div>"
         "<div id=w9 style='height:5px'></div><div id=wg1234567890123 style='height:5px'></div>");
    TEST_ASSERT_EQUAL(2, we_geom_feed(&g_doc, "wg", &g_g));
    /* a part of the prefix, a long index: no box (mutation witnesses) */
    TEST_ASSERT_EQUAL_STRING("none", box(9));
    TEST_ASSERT_EQUAL_STRING("none", box(12));
    TEST_ASSERT_EQUAL_STRING("1 1 0+10 0+320 vp 480/320 sc 0/0 rg 0/0", box(0));
    TEST_ASSERT_EQUAL_STRING("1 1 30+30 0+320 vp 480/320 sc 0/0 rg 0/0", box(191));
    TEST_ASSERT_EQUAL_STRING("none", box(7));
    TEST_ASSERT_EQUAL_STRING("none", box(5));
    /* another prefix */
    TEST_ASSERT_EQUAL(1, we_geom_feed(&g_doc, "xwg", &g_g));
    TEST_ASSERT_EQUAL_STRING("1 1 70+5 0+320 vp 480/320 sc 0/0 rg 0/0", box(5));
    TEST_ASSERT_EQUAL(1, we_geom_feed(&g_doc, "w", &g_g));      /* "g0" is not an index, w9 is 9 */
    TEST_ASSERT_EQUAL_STRING("none", box(0));
    TEST_ASSERT_EQUAL_STRING("1 1 75+5 0+320 vp 480/320 sc 0/0 rg 0/0", box(9));
    /* no prefix, no document: nothing, the record cleared */
    memset(&g_g, 0x5a, sizeof(g_g));
    TEST_ASSERT_EQUAL(0, we_geom_feed(&g_doc, "", &g_g));
    TEST_ASSERT_EQUAL(0, g_g.vw);
    TEST_ASSERT_EQUAL_STRING("none", box(0));
    memset(&g_g, 0x5a, sizeof(g_g));
    TEST_ASSERT_EQUAL(0, we_geom_feed(&g_doc, 0, &g_g));
    TEST_ASSERT_EQUAL(0, g_g.n);
    memset(&g_g, 0x5a, sizeof(g_g));
    TEST_ASSERT_EQUAL(0, we_geom_feed(0, "wg", &g_g));
    TEST_ASSERT_EQUAL(0, g_g.rg_y);
    TEST_ASSERT_EQUAL(-1, we_geom_feed(&g_doc, "wg", 0));
}

/* the scroll offsets: the root's scroll_y, a box scroller's offset (its
 * children keep their content positions) */
static void test_l104_geom_scroll(void) {
    int s;
    page("<div id=wg1 style='height:100px'></div><div style='height:900px'></div>");
    TEST_ASSERT_EQUAL(100, we_scroll(&g_doc, 100));
    TEST_ASSERT_EQUAL(1, we_geom_feed(&g_doc, "wg", &g_g));
    TEST_ASSERT_EQUAL_STRING("1 1 0+100 0+320 vp 480/320 sc 100/0 rg 520/0", box(1));
    TEST_ASSERT_EQUAL(520, g_g.rg_y);
    TEST_ASSERT_EQUAL(0, g_g.rg_x);
    /* a box scroller: content 150 + 40 + 100 in a 100 px view, scrolled
     * by a click on its track; the box keeps b 150, sc follows */
    we_forms_init(&g_forms);
    we_set_forms(&g_doc, &g_forms);
    page("<div id=s style='height:100px;overflow:auto'><div style='height:150px'></div>"
         "<div id=wg3 style='height:40px'></div><div style='height:100px'></div></div><div style='height:900px'></div>");
    TEST_ASSERT_EQUAL(1, we_geom_feed(&g_doc, "wg", &g_g));
    TEST_ASSERT_EQUAL_STRING("0 1 150+40 0+320 vp 100/320 sc 0/0 rg 190/0", box(3));
    s = we_find_id(&g_doc, "s");
    TEST_ASSERT_EQUAL(2, we_box_scroll_click(&g_doc, 315, 50));
    TEST_ASSERT_EQUAL(0, we_layout_viewport(&g_doc, 320, 480));
    TEST_ASSERT_EQUAL(95, g_doc.nodes[s].box_scroll_y);
    TEST_ASSERT_EQUAL(55, g_doc.nodes[we_find_id(&g_doc, "wg3")].y);
    TEST_ASSERT_EQUAL(1, we_geom_feed(&g_doc, "wg", &g_g));
    TEST_ASSERT_EQUAL_STRING("0 1 150+40 0+320 vp 100/320 sc 95/0 rg 190/0", box(3));
    we_set_forms(&g_doc, 0);
}

/* the axes of a vertical scroller: block along x from the block start */
static void test_l104_geom_vertical(void) {
    lay(&g_doc, "<html style='writing-mode:vertical-rl'><body style='margin:0'><div id=wg1 style='width:40px'></div>"
                "<div id=wg2 style='width:200px'></div></body></html>", 480);
    TEST_ASSERT_EQUAL(2, we_geom_feed(&g_doc, "wg", &g_g));
    /* laid out from the left: wg2 at x 0, wg1 at x 200; the block start
     * is the root element's right edge, 240 */
    TEST_ASSERT_EQUAL(200, g_doc.nodes[we_find_id(&g_doc, "wg1")].x);
    TEST_ASSERT_EQUAL_STRING("1 0 0+40 0+8 vp 320/480 sc 0/0 rg 0/0", box(1));
    TEST_ASSERT_EQUAL_STRING("1 0 40+200 0+8 vp 320/480 sc 0/0 rg 0/0", box(2));
    lay(&g_doc, "<html style='writing-mode:vertical-lr'><body style='margin:0'><div id=wg1 style='width:40px'></div>"
                "<div id=wg2 style='width:200px'></div></body></html>", 480);
    TEST_ASSERT_EQUAL(2, we_geom_feed(&g_doc, "wg", &g_g));
    TEST_ASSERT_EQUAL_STRING("1 0 0+40 0+8 vp 320/480 sc 0/0 rg 0/0", box(1));
    TEST_ASSERT_EQUAL_STRING("1 0 40+200 0+8 vp 320/480 sc 0/0 rg 0/0", box(2));
    /* text before html is a node of the document: the root element is
     * still html (mutation witness) */
    lay(&g_doc, "x<html style='writing-mode:vertical-rl'><body style='margin:0'><div id=wg1 style='width:40px'></div>"
                "<div id=wg2 style='width:200px'></div></body></html>", 480);
    TEST_ASSERT_TRUE(g_doc.nodes[1].is_text);
    TEST_ASSERT_EQUAL(2, we_geom_feed(&g_doc, "wg", &g_g));
    TEST_ASSERT_EQUAL(0, g_g.box[1].bvert);
    TEST_ASSERT_EQUAL(0, g_g.box[2].bvert);
    /* a vertical-rl box scroller in a horizontal page: from its content
     * box's right edge (200) */
    lay(&g_doc, "<html><body style='margin:0'><div style='writing-mode:vertical-rl;width:200px;height:100px;"
                "overflow:auto'><div id=wg4 style='width:30px'></div></div></body></html>", 480);
    TEST_ASSERT_EQUAL(1, we_geom_feed(&g_doc, "wg", &g_g));
    TEST_ASSERT_EQUAL_STRING("0 0 0+30 0+100 vp 200/100 sc 0/0 rg 0/0", box(4));
}

/* feeding reads the layout only */
static void test_l104_geom_readonly(void) {
    int i;
    page("<div id=wg1 style='height:100px;padding:5px'>a</div><div id=wg2 style='height:600px'>b</div>");
    memcpy(&g_doc2, &g_doc, sizeof(g_doc));
    TEST_ASSERT_EQUAL(2, we_geom_feed(&g_doc, "wg", &g_g));
    TEST_ASSERT_EQUAL(0, memcmp(&g_doc, &g_doc2, sizeof(g_doc)));
    for (i = 0; i < 2; i++) TEST_ASSERT_EQUAL(0, we_geom(&g_doc, we_find_id(&g_doc, i ? "wg2" : "wg1"), &g_g.box[50]));
    TEST_ASSERT_EQUAL(0, memcmp(&g_doc, &g_doc2, sizeof(g_doc)));
}

/* the two passes: the reader view of a page with a view() subject */
#define FILL "<p>line</p><p>line</p><p>line</p><p>line</p><p>line</p><p>line</p><p>line</p><p>line</p><p>line</p><p>line</p>"
static const char *PAGE =
    "<html><head><style>@keyframes k { from { color: #000000 } to { color: #ffffff } } "
    "#s { animation: k 1000ms linear; animation-timeline: view() }</style></head><body>"
    "<div>a</div>" FILL FILL FILL FILL "<div id=s>subject</div>" FILL FILL FILL "</body></html>";

static int reduce(char *out, int marks) {
    wf_html_stats_t st;
    int n;
    wf_html_set_sitecss(1);
    wf_html_set_geommarks(marks);
    n = wf_html_reduce(PAGE, (int)strlen(PAGE), 0, out, 65536, &st);
    wf_html_set_geommarks(0);
    wf_html_set_sitecss(0);
    return n;
}

static int same_boxes(const we_doc_t *a, const we_doc_t *b) {
    int i;
    if (a->nnodes != b->nnodes || a->perf.page_h != b->perf.page_h) return 0;
    for (i = 0; i < (int)a->nnodes; i++)
        if (a->nodes[i].x != b->nodes[i].x || a->nodes[i].y != b->nodes[i].y || a->nodes[i].w != b->nodes[i].w ||
            a->nodes[i].h != b->nodes[i].h)
            return 0;
    return 1;
}

static void test_l104_two_passes(void) {
    const char *p;
    int n, n2, k, s;
    long long cs, ce, pos, ms;
    const wf_geom_box_t *b;
    char want[64];
    /* without marks (the default) the reduced page keeps its bytes */
    n2 = reduce(g_out2, 0);
    TEST_ASSERT_TRUE(n2 > 0);
    TEST_ASSERT_NULL(strstr(g_out2, "id=\"wg"));
    n = reduce(g_out, 1);
    TEST_ASSERT_EQUAL(1, wf_html_sitecss_geom_need());
    p = strstr(g_out, "<p id=\"wg");
    TEST_ASSERT_NOT_NULL(p);
    TEST_ASSERT_EQUAL(0, strncmp("<p id=\"wg", p, 9));
    TEST_ASSERT_NULL(strstr(p + 9, "id=\"wg"));      /* the subject only */
    k = 0;
    for (p += 9; *p >= '0' && *p <= '9'; p++) k = k * 10 + (*p - '0');
    TEST_ASSERT_EQUAL('"', *p);
    TEST_ASSERT_EQUAL(n2 + 9 + (k > 9) + (k > 99), n);     /* ' id="wgN"' */
    /* the marks do not move a box */
    lay(&g_doc, g_out, 480);
    lay(&g_doc2, g_out2, 480);
    TEST_ASSERT_TRUE(same_boxes(&g_doc, &g_doc2));
    /* the geometry of the subject: at pm 6000 */
    TEST_ASSERT_EQUAL(1, we_geom_feed(&g_doc, "wg", &g_g));
    b = &g_g.box[k];
    TEST_ASSERT_EQUAL(1, b->ok);
    TEST_ASSERT_EQUAL(1, b->root);
    TEST_ASSERT_EQUAL(480, b->vp_b);
    TEST_ASSERT_EQUAL((int)g_doc.perf.page_h - 480, b->rg_b);
    TEST_ASSERT_TRUE(b->rg_b > 0);
    s = we_find_id(&g_doc, "wgX");
    TEST_ASSERT_EQUAL(-1, s);
    /* cover [S - 480, E], the offset 6000 / 10000 of the range; linear
     * black to white at ms / 1000, rounded (255 ms / 1000 + 0.5) */
    cs = (long long)b->b_off - 480;
    ce = (long long)b->b_off + b->b_size;
    pos = (long long)b->rg_b * 6000 / 10000;     /* exact: rg_b * 0.6 */
    ms = (1000LL * (b->rg_b * 6000LL - cs * 10000)) / ((ce - cs) * 10000);
    TEST_ASSERT_TRUE(pos > cs && pos < ce);
    snprintf(want, sizeof(want), "<p id=\"wg%d\" style=\"color:#%02x%02x%02x\">", k, (int)((255 * ms + 500) / 1000),
             (int)((255 * ms + 500) / 1000), (int)((255 * ms + 500) / 1000));
    wf_html_set_cssgeom(&g_g);
    wf_html_set_cssscroll(6000);
    n = reduce(g_out, 1);
    TEST_ASSERT_NOT_NULL(strstr(g_out, want));
    /* the page laid out again: the same boxes (colours only) */
    lay(&g_doc2, g_out, 480);
    TEST_ASSERT_TRUE(same_boxes(&g_doc, &g_doc2));
    /* this layout: S 1082, E 1092, range 1382: 829.2 is 463 ms, #767676 */
    TEST_ASSERT_EQUAL(1082, b->b_off);
    TEST_ASSERT_EQUAL(10, b->b_size);
    TEST_ASSERT_EQUAL(1382, b->rg_b);
    TEST_ASSERT_NOT_NULL(strstr(g_out, "style=\"color:#767676\""));
    /* the geometry cleared: no colour, the page asks again */
    wf_html_set_cssgeom(0);
    n = reduce(g_out, 1);
    TEST_ASSERT_NULL(strstr(g_out, "color:#"));
    TEST_ASSERT_EQUAL(1, wf_html_sitecss_geom_need());
    wf_html_set_cssscroll(0);
    /* site CSS off: nothing is needed */
    {
        wf_html_stats_t st;
        wf_html_set_geommarks(1);
        n = wf_html_reduce(PAGE, (int)strlen(PAGE), 0, g_out, 65536, &st);
        wf_html_set_geommarks(0);
        TEST_ASSERT_TRUE(n > 0);
        TEST_ASSERT_NULL(strstr(g_out, "id=\"wg"));
        TEST_ASSERT_EQUAL(0, wf_html_sitecss_geom_need());
    }
}

int main(void) {
    unity_init();
    RUN_TEST(test_l104_geom_node);
    RUN_TEST(test_l104_geom_marks);
    RUN_TEST(test_l104_geom_scroll);
    RUN_TEST(test_l104_geom_vertical);
    RUN_TEST(test_l104_geom_readonly);
    RUN_TEST(test_l104_two_passes);
    unity_print_results();
    unity_cleanup();
    return unity_stats.tests_failed == 0 ? 0 : 1;
}
