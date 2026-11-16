/* Lot 108 (US-031): horizontal box scrolling in the engine (overflow-x
 * auto or scroll, the bar, clicks on its track, the offset kept in the
 * forms), the reader's width, max-width and min-height on box scrollers
 * laid out by the engine, and scroll timelines on the inline axis of a
 * box scroller fed back from the geometry (wf_html, wf_sitecss, webeng).
 * Hand cases next to the generated test_lot108_*.c; expected values
 * follow docs/lot108_plan.md. */
#include "../../framework/unity.h"
#include <string.h>
#include <stdio.h>
#include "webeng.h"
#include "wf_html.h"

int we_geom(const we_doc_t *d, int node, wf_geom_box_t *g);

static we_doc_t g_doc;
static we_forms_t g_forms;
static wf_geom_t g_g;
static char g_html[8192], g_out[65536], g_o[200];

static void lay(we_doc_t *d, const char *html) {
    TEST_ASSERT_EQUAL(0, we_parse(d, html, (int)strlen(html)));
    TEST_ASSERT_EQUAL(0, we_style(d));
    TEST_ASSERT_EQUAL(0, we_layout_viewport(d, 640, 480));
}

static int node_of(const char *id) {
    int i;
    for (i = 0; i < (int)g_doc.nnodes; i++)
        if (!strcmp(g_doc.nodes[i].id, id)) return i;
    return -1;
}

/* a click, then the page laid out again (as the shell does) */
static int click(int x, int y) {
    int r = we_box_scroll_click(&g_doc, x, y);
    if (we_layout_viewport(&g_doc, 640, 480) != 0) return -99;
    return r;
}

#define KF "@keyframes k { from { color: #000000 } to { color: #ffffff } } "
#define WIDE "<p id=i style=\"white-space:nowrap\">aaaaaaaaaa bbbbbbbbbb cccccccccc dddddddddd eeeeeeeeee ffffffffff</p>"
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

/* a box both ways: the x offset and the y offset each kept in the same
 * forms entry, restored by a new parse */
static void test_l108_both_axes(void) {
    static const char page[] = "<html><body><div id=o style=\"overflow:auto;width:200px;height:40px\">"
                               "<div style=\"width:600px;height:12px\"></div><div style=\"width:20px;height:12px\"></div>"
                               "<div style=\"width:20px;height:12px\"></div><div style=\"width:20px;height:12px\"></div>"
                               "<div style=\"width:20px;height:12px\"></div></div></body></html>";
    int o;
    we_forms_init(&g_forms);
    we_set_forms(&g_doc, &g_forms);
    lay(&g_doc, page);
    o = node_of("o");
    TEST_ASSERT_TRUE(o > 0);
    /* the x track stops left of the vertical bar (8..202 at y 42..48):
     * the corner is the vertical track's; its click makes the entry,
     * which keeps the x offset (0) too */
    TEST_ASSERT_EQUAL(2, click(202, 44));
    TEST_ASSERT_EQUAL(1, g_forms.nscroll);
    TEST_ASSERT_EQUAL(0, g_forms.scroll_x[0]);
    TEST_ASSERT_EQUAL(18, g_forms.scroll_y[0]);        /* 20 * 36 / 40 */
    /* the x track: the entry is updated, the y offset kept */
    TEST_ASSERT_EQUAL(2, click(105, 44));
    TEST_ASSERT_EQUAL(1, g_forms.nscroll);
    TEST_ASSERT_EQUAL(200, g_forms.scroll_x[0]);       /* 400 * 97 / 194 */
    TEST_ASSERT_EQUAL(18, g_forms.scroll_y[0]);
    TEST_ASSERT_EQUAL(200, g_doc.nodes[o].box_scroll_x);
    TEST_ASSERT_EQUAL(2, click(205, 47));
    TEST_ASSERT_EQUAL(200, g_forms.scroll_x[0]);
    TEST_ASSERT_EQUAL(19, g_forms.scroll_y[0]);
    /* a new parse restores both */
    lay(&g_doc, page);
    o = node_of("o");
    TEST_ASSERT_EQUAL(200, g_doc.nodes[o].box_scroll_x);
    TEST_ASSERT_EQUAL(19, g_doc.nodes[o].box_scroll_y);
    TEST_ASSERT_EQUAL(8 - 200, g_doc.nodes[o + 1].x);
    TEST_ASSERT_EQUAL(8 - 19, g_doc.nodes[o + 1].y);
    /* an x click then: the y offset stays */
    TEST_ASSERT_EQUAL(2, click(8, 44));
    TEST_ASSERT_EQUAL(0, g_forms.scroll_x[0]);
    TEST_ASSERT_EQUAL(19, g_forms.scroll_y[0]);
    we_set_forms(&g_doc, 0);
}

/* a first click along x stores the y offset too (0 here), and without
 * forms the offset stays on the node only */
static void test_l108_persist(void) {
    static const char page[] = "<html><body><div id=o style=\"overflow-x:scroll;width:100px\">"
                               "<div style=\"width:300px;height:12px\"></div></div></body></html>";
    int o;
    we_forms_init(&g_forms);
    we_set_forms(&g_doc, 0);
    lay(&g_doc, page);
    o = node_of("o");
    /* track 8..108 at y 20 - 6 = 14 */
    TEST_ASSERT_EQUAL(2, we_box_scroll_click(&g_doc, 58, 15));
    TEST_ASSERT_EQUAL(100, g_doc.nodes[o].box_scroll_x);
    TEST_ASSERT_EQUAL(0, g_forms.nscroll);
    TEST_ASSERT_EQUAL(1, we_box_scroll_click(&g_doc, 58, 15));
    lay(&g_doc, page);     /* no forms: a new parse starts at 0 */
    TEST_ASSERT_EQUAL(0, g_doc.nodes[node_of("o")].box_scroll_x);
    /* nothing to scroll: the track is not painted, a click there is 0 */
    lay(&g_doc, "<html><body><div id=o style=\"overflow-x:auto;width:100px\"><div style=\"width:60px;height:12px\">"
                "</div></div></body></html>");
    TEST_ASSERT_EQUAL(0, we_box_scroll_click(&g_doc, 58, 15));
    TEST_ASSERT_EQUAL(0, g_doc.nodes[node_of("o")].box_scroll_x);
}

/* the reader passes overflow-x and width for a box scroller a timeline
 * reads along x; the engine scrolls it along x, its geometry gives the
 * range, and the timeline runs on it */
static void test_l108_flow_inline(void) {
    const char *css = "#o { overflow-x: auto; width: 200px } "
                      "#i { animation: k 1000ms linear; animation-timeline: scroll(nearest inline) }";
    wf_geom_box_t b;
    int rg;
    TEST_ASSERT_TRUE(reduce(css, "<div id=o>" WIDE "</div>", 1) > 0);
    TEST_ASSERT_NOT_NULL(strstr(g_out, "style=\"overflow-x:auto;width:200px\""));
    lay(&g_doc, g_out);
    memset(&g_g, 0, sizeof(g_g));
    TEST_ASSERT_EQUAL(1, we_geom_feed(&g_doc, "wg", &g_g));
    b = g_g.box[4];
    TEST_ASSERT_EQUAL(1, b.ok);
    TEST_ASSERT_EQUAL(1, b.svx);
    TEST_ASSERT_EQUAL(0, b.sv);
    TEST_ASSERT_EQUAL(200, b.x_vp);
    TEST_ASSERT_EQUAL(0, b.x_sc);
    TEST_ASSERT_TRUE(b.x_rg > 0);
    rg = b.x_rg;
    /* at offset 0: the from colour; at half the range: half way */
    wf_html_set_cssgeom(&g_g);
    reduce(css, "<div id=o>" WIDE "</div>", 1);
    TEST_ASSERT_NOT_NULL(strstr(g_out, "color:#000000"));
    g_g.box[4].x_sc = rg / 2;
    wf_html_set_cssgeom(&g_g);      /* the reader keeps its own copy */
    reduce(css, "<div id=o>" WIDE "</div>", 1);
    snprintf(g_o, sizeof(g_o), "color:#%02x%02x%02x", (255 * (rg / 2) * 1000 / rg + 500) / 1000,
             (255 * (rg / 2) * 1000 / rg + 500) / 1000, (255 * (rg / 2) * 1000 / rg + 500) / 1000);
    TEST_ASSERT_NOT_NULL(strstr(g_out, g_o));
    g_g.box[4].x_sc = rg;
    wf_html_set_cssgeom(&g_g);
    reduce(css, "<div id=o>" WIDE "</div>", 1);
    TEST_ASSERT_NULL(strstr(g_out, "color:#"));      /* at the end, fill none: no effect */
    reduce("#o { overflow-x: auto; width: 200px } #i { animation: k 1000ms linear both; "
           "animation-timeline: scroll(nearest inline) }", "<div id=o>" WIDE "</div>", 1);
    TEST_ASSERT_NOT_NULL(strstr(g_out, "color:#ffffff"));
    /* the block axis of the same box: no range along y, no effect */
    reduce("#o { overflow-x: auto; width: 200px } "
           "#i { animation: k 1000ms linear; animation-timeline: scroll(nearest block) }", "<div id=o>" WIDE "</div>", 1);
    TEST_ASSERT_NULL(strstr(g_out, "color:#"));
    wf_html_set_cssgeom(0);
}

/* max-width and min-height reach the engine: the scrollport is the
 * smaller of the content and max-width; min-height makes the box higher */
static void test_l108_flow_maxwidth(void) {
    const char *css = "#o { overflow: auto; max-width: 15em; height: 3em; min-height: 6em } "
                      "#i { animation: k 1s linear; animation-timeline: scroll(nearest x) }";
    TEST_ASSERT_TRUE(reduce(css, "<div id=o>" WIDE FILL "</div>", 1) > 0);
    TEST_ASSERT_NOT_NULL(strstr(g_out, "style=\"overflow-y:auto;height:48px;min-height:96px;overflow-x:auto;max-width:240px\""));
    lay(&g_doc, g_out);
    memset(&g_g, 0, sizeof(g_g));
    TEST_ASSERT_EQUAL(1, we_geom_feed(&g_doc, "wg", &g_g));
    TEST_ASSERT_EQUAL(240, g_g.box[4].x_vp);
    TEST_ASSERT_EQUAL(1, g_g.box[4].svx);
    TEST_ASSERT_EQUAL(96, g_g.box[4].b_size);
    TEST_ASSERT_EQUAL(96, g_g.box[4].s_vp);
    TEST_ASSERT_TRUE(g_g.box[4].x_rg > 0);
    /* a percentage width of the containing block (main, 624 px) */
    reduce("#o { overflow-x: scroll; width: 25% } #i { animation: k 1s linear; animation-timeline: scroll(x) }",
           "<div id=o>" WIDE "</div>", 1);
    TEST_ASSERT_NOT_NULL(strstr(g_out, "style=\"overflow-x:scroll;width:25%\""));
    lay(&g_doc, g_out);
    (void)we_geom_feed(&g_doc, "wg", &g_g);
    TEST_ASSERT_EQUAL(156, g_g.box[4].x_vp);
}

/* the content width: hidden and out-of-flow subtrees and the insides of a
 * box clipping along x do not count; a nested wider box does */
static void test_l108_content_width(void) {
    wf_geom_box_t g;
    lay(&g_doc, "<html><body><div id=o style=\"overflow-x:auto;width:100px\">"
                "<div style=\"width:50px\"><div style=\"width:250px;height:12px\"></div></div>"
                "<div style=\"overflow:hidden;width:80px\"><div style=\"width:900px;height:12px\"></div></div>"
                "<div style=\"position:absolute;width:700px;height:12px\"></div>"
                "<div style=\"display:none;width:800px\"></div></div></body></html>");
    TEST_ASSERT_EQUAL(0, we_geom(&g_doc, node_of("o"), &g));
    TEST_ASSERT_EQUAL(1, g.svx);
    TEST_ASSERT_EQUAL(100, g.x_vp);
    TEST_ASSERT_EQUAL(150, g.x_rg);
    TEST_ASSERT_EQUAL(250, g_doc.nodes[node_of("o")].box_cw);
    /* overflow-x hidden: clipped, no x scrolling, no range */
    lay(&g_doc, "<html><body><div id=o style=\"overflow-x:hidden;width:100px\">"
                "<div style=\"width:250px;height:12px\"></div></div></body></html>");
    TEST_ASSERT_EQUAL(0, we_geom(&g_doc, node_of("o"), &g));
    TEST_ASSERT_EQUAL(0, g.svx);
    TEST_ASSERT_EQUAL(0, g.x_rg);
    TEST_ASSERT_EQUAL(0, we_box_scroll_click(&g_doc, 50, 15));
    /* a box left of the page: a hidden box (laid out at 0, 0) still does
     * not count */
    lay(&g_doc, "<html><body><div id=o style=\"overflow-x:auto;width:50px;margin-left:-100px\">"
                "<div style=\"width:20px;height:12px\"></div><div style=\"display:none\"></div></div></body></html>");
    TEST_ASSERT_EQUAL(-92, g_doc.nodes[node_of("o")].x);
    TEST_ASSERT_EQUAL(20, g_doc.nodes[node_of("o")].box_cw);
}

/* the contents move along x as a whole: grandchildren and their display
 * items too (a red box at 250 px in a 270 px content, scrolled by 168 px,
 * is painted at 90 px) */
static void test_l108_shift(void) {
    static const char page[] = "<html><body><div id=o style=\"overflow-x:auto;width:100px\"><div id=c style=\"width:300px\">"
                               "<div id=g style=\"margin-left:250px;width:20px;height:10px;background:#ff0000\"></div>"
                               "</div></div></body></html>";
    int o, g, i, hit = 0;
    we_set_forms(&g_doc, 0);
    lay(&g_doc, page);
    o = node_of("o");
    g = node_of("g");
    TEST_ASSERT_EQUAL(258, g_doc.nodes[g].x);
    TEST_ASSERT_EQUAL(300, g_doc.nodes[o].box_cw);
    /* track 8..108 at the bottom: 200 * 99 / 100 */
    TEST_ASSERT_EQUAL(2, click(107, g_doc.nodes[o].y + g_doc.nodes[o].h - 3));
    TEST_ASSERT_EQUAL(198, g_doc.nodes[o].box_scroll_x);
    TEST_ASSERT_EQUAL(8 - 198, g_doc.nodes[node_of("c")].x);
    TEST_ASSERT_EQUAL(258 - 198, g_doc.nodes[g].x);
    for (i = 0; i < (int)g_doc.nitems; i++)
        if (g_doc.item_owner[i] == g && g_doc.items[i].kind == OS_FB_WEB_RECT && g_doc.items[i].color == 0xFF0000) {
            TEST_ASSERT_EQUAL(258 - 198, g_doc.items[i].x);
            TEST_ASSERT_EQUAL(20, g_doc.items[i].w);
            hit++;
        }
    TEST_ASSERT_EQUAL(1, hit);
}

/* a kept offset out of the range is clamped and stored; a box scrolling
 * along x only keeps no y offset */
static void test_l108_clamp(void) {
    static const char page[] = "<html><body><div id=o style=\"overflow-x:auto;width:100px;height:20px\">"
                               "<div style=\"width:300px;height:50px\"></div></div></body></html>";
    int o;
    we_forms_init(&g_forms);
    g_forms.nscroll = 1;
    strcpy(g_forms.scroll_id[0], "o");
    g_forms.scroll_x[0] = 1000;
    g_forms.scroll_y[0] = 10;
    we_set_forms(&g_doc, &g_forms);
    lay(&g_doc, page);
    o = node_of("o");
    TEST_ASSERT_EQUAL(200, g_doc.nodes[o].box_scroll_x);
    TEST_ASSERT_EQUAL(0, g_doc.nodes[o].box_scroll_y);
    TEST_ASSERT_EQUAL(8 - 200, g_doc.nodes[o + 1].x);
    g_forms.scroll_x[0] = -5;
    lay(&g_doc, page);
    TEST_ASSERT_EQUAL(0, g_doc.nodes[node_of("o")].box_scroll_x);
    TEST_ASSERT_EQUAL(8, g_doc.nodes[node_of("o") + 1].x);
    we_set_forms(&g_doc, 0);
}

/* the corner at the end of the y range is still the vertical track's;
 * a track narrower than 4 px is not one */
static void test_l108_corner(void) {
    static const char page[] = "<html><body><div id=o style=\"overflow:auto;width:200px;height:40px\">"
                               "<div style=\"width:600px;height:12px\"></div><div style=\"width:20px;height:12px\"></div>"
                               "<div style=\"width:20px;height:12px\"></div><div style=\"width:20px;height:12px\"></div>"
                               "<div style=\"width:20px;height:12px\"></div></div></body></html>";
    we_forms_init(&g_forms);
    g_forms.nscroll = 1;
    strcpy(g_forms.scroll_id[0], "o");
    g_forms.scroll_y[0] = 20;
    we_set_forms(&g_doc, &g_forms);
    lay(&g_doc, page);
    TEST_ASSERT_EQUAL(20, g_doc.nodes[node_of("o")].box_scroll_y);
    (void)click(204, 44);
    TEST_ASSERT_EQUAL(0, g_doc.nodes[node_of("o")].box_scroll_x);
    TEST_ASSERT_EQUAL(0, g_forms.scroll_x[0]);
    we_set_forms(&g_doc, 0);
    lay(&g_doc, "<html><body><div id=o style=\"overflow-x:scroll;width:3px\"><div style=\"width:300px;height:12px\"></div>"
                "</div></body></html>");
    TEST_ASSERT_EQUAL(0, we_box_scroll_click(&g_doc, 9, 15));
    TEST_ASSERT_EQUAL(0, g_doc.nodes[node_of("o")].box_scroll_x);
}

/* a new style pass starts the box at 0 along x as along y (no forms) */
static void test_l108_restyle(void) {
    static const char page[] = "<html><body><div id=o style=\"overflow-x:scroll;width:100px\">"
                               "<div style=\"width:300px;height:12px\"></div></div></body></html>";
    we_set_forms(&g_doc, 0);
    lay(&g_doc, page);
    TEST_ASSERT_EQUAL(2, we_box_scroll_click(&g_doc, 58, 15));
    TEST_ASSERT_EQUAL(100, g_doc.nodes[node_of("o")].box_scroll_x);
    TEST_ASSERT_EQUAL(0, we_style(&g_doc));
    TEST_ASSERT_EQUAL(0, we_layout_viewport(&g_doc, 640, 480));
    TEST_ASSERT_EQUAL(0, g_doc.nodes[node_of("o")].box_scroll_x);
    TEST_ASSERT_EQUAL(8, g_doc.nodes[node_of("o") + 1].x);
}

/* a border box narrower than its padding: the layout widens it to the
 * padding, the scrollport is 0 px and the range the whole content */
static void test_l108_narrowpad(void) {
    wf_geom_box_t g;
    lay(&g_doc, "<html><body><div id=o style=\"overflow-x:auto;box-sizing:border-box;width:10px;padding:0 20px\">"
                "<div style=\"width:30px;height:12px\"></div></div></body></html>");
    TEST_ASSERT_EQUAL(0, we_geom(&g_doc, node_of("o"), &g));
    TEST_ASSERT_EQUAL(40, g_doc.nodes[node_of("o")].w);
    TEST_ASSERT_EQUAL(0, g.x_vp);
    TEST_ASSERT_EQUAL(30, g.x_rg);
}

/* the scroller of an element inside a box scrolling along x only: its
 * range along x, none along y; along y only: none along x */
static void test_l108_child_ranges(void) {
    wf_geom_box_t g;
    lay(&g_doc, "<html><body><div id=o style=\"overflow-x:auto;width:100px;height:20px\">"
                "<div id=c style=\"width:300px;height:50px\"></div></div></body></html>");
    TEST_ASSERT_EQUAL(0, we_geom(&g_doc, node_of("c"), &g));
    TEST_ASSERT_EQUAL(0, g.root);
    TEST_ASSERT_EQUAL(100, g.vp_i);
    TEST_ASSERT_EQUAL(200, g.rg_i);
    TEST_ASSERT_EQUAL(20, g.vp_b);
    TEST_ASSERT_EQUAL(0, g.rg_b);
    lay(&g_doc, "<html><body><div id=o style=\"overflow-y:auto;width:100px;height:20px\">"
                "<div id=c style=\"width:300px;height:50px\"></div></div></body></html>");
    TEST_ASSERT_EQUAL(0, we_geom(&g_doc, node_of("c"), &g));
    TEST_ASSERT_EQUAL(0, g.root);
    TEST_ASSERT_EQUAL(30, g.rg_b);
    TEST_ASSERT_EQUAL(0, g.rg_i);
}

int main(void) {
    unity_init();
    RUN_TEST(test_l108_both_axes);
    RUN_TEST(test_l108_persist);
    RUN_TEST(test_l108_flow_inline);
    RUN_TEST(test_l108_flow_maxwidth);
    RUN_TEST(test_l108_content_width);
    RUN_TEST(test_l108_shift);
    RUN_TEST(test_l108_clamp);
    RUN_TEST(test_l108_corner);
    RUN_TEST(test_l108_restyle);
    RUN_TEST(test_l108_narrowpad);
    RUN_TEST(test_l108_child_ranges);
    unity_print_results();
    unity_cleanup();
    return unity_stats.tests_failed == 0 ? 0 : 1;
}
