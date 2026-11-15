/* Lot 105 (US-031): a box scroller's own scroll state in the layout
 * geometry (webeng_geom.inc), the divs the reader view keeps (wf_html),
 * and the two passes with a box scroller and with the GUI scroll
 * position. Hand cases next to the generated test_lot105_geom.c; expected
 * values follow docs/lot105_plan.md. */
#include "../../framework/unity.h"
#include <string.h>
#include <stdio.h>
#include "webeng.h"
#include "wf_html.h"

static we_doc_t g_doc, g_doc2;
static we_forms_t g_forms;
static wf_geom_t g_g;
static char g_html[8192], g_out[65536], g_o[200];

static void lay(we_doc_t *d, const char *html, int vh) {
    TEST_ASSERT_EQUAL(0, we_parse(d, html, (int)strlen(html)));
    TEST_ASSERT_EQUAL(0, we_style(d));
    TEST_ASSERT_EQUAL(0, we_layout_viewport(d, 320, vh));
}

static void page(const char *body) {
    snprintf(g_html, sizeof(g_html), "<html><body style='margin:0'>%s</body></html>", body);
    lay(&g_doc, g_html, 480);
}

static const char *own(int k) {
    const wf_geom_box_t *b = &g_g.box[k];
    if (!b->ok) return "none";
    snprintf(g_o, sizeof(g_o), "root %d sm %d sv %d vp %d sc %d rg %d / sc %d rg %d", b->root, b->sm, b->sv, b->s_vp,
             b->s_sc, b->s_rg, b->sc_b, b->rg_b);
    return g_o;
}

/* a box scroller's own scrollport, offset and range; which marked box
 * scroller holds a box */
static void test_l105_geom_own(void) {
    wf_geom_box_t b;
    int s;
    we_forms_init(&g_forms);
    we_set_forms(&g_doc, &g_forms);
    /* content 150 + 40 + 100 in a 100 px view: range 190 */
    page("<div id=wg1 style='height:100px;overflow:auto'><div style='height:150px'></div>"
         "<div id=wg3 style='height:40px'></div><div style='height:100px'></div></div><div style='height:900px'></div>");
    TEST_ASSERT_EQUAL(2, we_geom_feed(&g_doc, "wg", &g_g));
    TEST_ASSERT_EQUAL_STRING("root 1 sm 0 sv 1 vp 100 sc 0 rg 190 / sc 0 rg 520", own(1));
    TEST_ASSERT_EQUAL_STRING("root 0 sm 2 sv 0 vp 0 sc 0 rg 0 / sc 0 rg 190", own(3));
    TEST_ASSERT_EQUAL(0, g_g.sc_y);
    /* a click on its track: the box and its child follow */
    s = we_find_id(&g_doc, "wg1");
    TEST_ASSERT_EQUAL(2, we_box_scroll_click(&g_doc, 315, 50));
    TEST_ASSERT_EQUAL(0, we_layout_viewport(&g_doc, 320, 480));
    TEST_ASSERT_EQUAL(95, g_doc.nodes[s].box_scroll_y);
    TEST_ASSERT_EQUAL(100, we_scroll(&g_doc, 100));
    memset(&g_g, 0, sizeof(g_g));
    TEST_ASSERT_EQUAL(2, we_geom_feed(&g_doc, "wg", &g_g));
    TEST_ASSERT_EQUAL_STRING("root 1 sm 0 sv 1 vp 100 sc 95 rg 190 / sc 100 rg 520", own(1));
    TEST_ASSERT_EQUAL_STRING("root 0 sm 2 sv 0 vp 0 sc 0 rg 0 / sc 95 rg 190", own(3));
    TEST_ASSERT_EQUAL(100, g_g.sc_y);
    /* we_geom alone does not know the marks */
    TEST_ASSERT_EQUAL(0, we_geom(&g_doc, we_find_id(&g_doc, "wg3"), &b));
    TEST_ASSERT_EQUAL(0, b.sm);
    TEST_ASSERT_EQUAL(0, b.sv);
    TEST_ASSERT_EQUAL(0, we_geom(&g_doc, s, &b));
    TEST_ASSERT_EQUAL(1, b.sv);
    TEST_ASSERT_EQUAL(95, b.s_sc);
    we_set_forms(&g_doc, 0);
    /* an unmarked box scroller (sm 0, not the root), nested marked ones,
     * a hidden box (no scroll box), content that fits (range 0) */
    memset(&g_g, 0, sizeof(g_g));
    page("<div id=s style='height:50px;overflow:scroll'><div id=wg4 style='height:80px'></div></div>"
         "<div id=wg5 style='height:200px;overflow-y:auto'><div id=wg6 style='height:60px;overflow:auto'>"
         "<div id=wg7 style='height:70px'></div></div><div style='height:300px'></div></div>"
         "<div id=wg8 style='height:30px;overflow:hidden'><div style='height:90px'></div></div>"
         "<div id=wg9 style='height:30px;overflow:auto'><div id=wg10 style='height:10px'></div></div>"
         "<div id=wg12 style='height:40px;overflow:auto'><div><div id=wg13 style='height:90px'></div></div></div>");
    TEST_ASSERT_EQUAL(9, we_geom_feed(&g_doc, "wg", &g_g));
    TEST_ASSERT_EQUAL_STRING("root 0 sm 0 sv 0 vp 0 sc 0 rg 0 / sc 0 rg 30", own(4));
    TEST_ASSERT_EQUAL_STRING("root 1 sm 0 sv 1 vp 200 sc 0 rg 160 / sc 0 rg 0", own(5));
    TEST_ASSERT_EQUAL_STRING("root 0 sm 6 sv 1 vp 60 sc 0 rg 10 / sc 0 rg 160", own(6));
    TEST_ASSERT_EQUAL_STRING("root 0 sm 7 sv 0 vp 0 sc 0 rg 0 / sc 0 rg 10", own(7));
    TEST_ASSERT_EQUAL_STRING("root 1 sm 0 sv 0 vp 0 sc 0 rg 0 / sc 0 rg 0", own(8));
    TEST_ASSERT_EQUAL_STRING("root 1 sm 0 sv 1 vp 30 sc 0 rg 0 / sc 0 rg 0", own(9));
    TEST_ASSERT_EQUAL_STRING("root 0 sm 10 sv 0 vp 0 sc 0 rg 0 / sc 0 rg 0", own(10));
    /* the nearest scroller is not always the parent */
    TEST_ASSERT_EQUAL_STRING("root 1 sm 0 sv 1 vp 40 sc 0 rg 50 / sc 0 rg 0", own(12));
    TEST_ASSERT_EQUAL_STRING("root 0 sm 13 sv 0 vp 0 sc 0 rg 0 / sc 0 rg 50", own(13));
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

static const char *body_of(void) {
    const char *p = strstr(g_out, "<body>");
    return p ? p + 6 : "(no body)";
}

#define SCR "#o { overflow: auto; height: 60px } #i { animation: k 1000ms linear; animation-timeline: scroll(nearest) } "

/* the divs the reader view keeps: a read box scroller, a view subject
 * holding blocks; their end tags */
static void test_l105_reduce_divs(void) {
    int k;
    char deep[2048];
    TEST_ASSERT_TRUE(reduce(SCR, "<div id=o><div id=i>a</div><div>b</div></div><p>q</p>", 0) > 0);
    TEST_ASSERT_EQUAL_STRING("<div style=\"overflow-y:auto;height:60px\"><p>a</p><p>b</p></div><p>q</p></body></html>",
                             body_of());
    TEST_ASSERT_EQUAL(1, wf_html_sitecss_geom_need());
    TEST_ASSERT_EQUAL(1, wf_html_sitecss_geom_live());
    /* the mark (reader node 4: html, head, body, main), an empty
     * paragraph before it dropped */
    reduce(SCR, "<div><div id=o><section><p id=i>i</p></section><div></div></div></div>", 1);
    TEST_ASSERT_EQUAL_STRING("<div id=\"wg5\" style=\"overflow-y:auto;height:60px\"><p><p>i</p></div></body></html>",
                             body_of());
    /* a misnested end tag closes the open ones above it; a stray end tag
     * after a kept div adds nothing */
    reduce(SCR, "<div id=o><section><div id=i>a</section></div></div>z", 0);
    TEST_ASSERT_EQUAL_STRING("<div style=\"overflow-y:auto;height:60px\"><p>a</p></div>z</p></body></html>", body_of());
    /* the div's end tag closes an open paragraph left empty */
    reduce(SCR, "<div id=o><p id=i>i</p><section></div>z", 0);
    TEST_ASSERT_EQUAL_STRING("<div style=\"overflow-y:auto;height:60px\"><p>i</p></div>z</p></body></html>", body_of());
    /* another kind of end tag does not close the div */
    reduce(SCR, "<div id=o><section id=i>a</article>b</section>c</div>d", 0);
    TEST_ASSERT_EQUAL_STRING("<div style=\"overflow-y:auto;height:60px\"><p>a</p>b</p>c</div>d</p></body></html>",
                             body_of());
    /* view subjects: holding blocks (a div), inline only (a p); a box
     * scroller no timeline reads stays a p */
    reduce("#o { overflow: auto; height: 60px } .v { animation: k 1s; animation-timeline: view() }",
           "<div id=o><div>a</div></div><section class=v><div>x</div></section><div class=v>t <b>u</b></div>", 1);
    TEST_ASSERT_EQUAL_STRING("<p>a</p><div id=\"wg6\"><p>x</p></div><p id=\"wg8\">t <b>u</b></p></body></html>", body_of());
    TEST_ASSERT_EQUAL(2, wf_html_sitecss_geom_need());
    TEST_ASSERT_EQUAL(0, wf_html_sitecss_geom_live());
    /* 32 open div-like elements (main and 31 divs): a 33rd box scroller
     * stays a p (its end tag could not be matched) */
    deep[0] = 0;
    for (k = 0; k < 31; k++) strcat(deep, "<div>");
    strcat(deep, "<section id=o><p id=i>i</p></section>");
    for (k = 0; k < 31; k++) strcat(deep, "</div>");
    reduce(SCR, deep, 0);
    TEST_ASSERT_EQUAL_STRING("<p><p>i</p></body></html>", body_of());
    deep[0] = 0;
    for (k = 0; k < 30; k++) strcat(deep, "<div>");
    strcat(deep, "<section id=o><p id=i>i</p></section>");
    for (k = 0; k < 30; k++) strcat(deep, "</div>");
    reduce(SCR, deep, 0);
    TEST_ASSERT_EQUAL_STRING("<div style=\"overflow-y:auto;height:60px\"><p>i</p></div></body></html>", body_of());
    /* site CSS off: no div */
    {
        wf_html_stats_t st;
        TEST_ASSERT_TRUE(wf_html_reduce(g_html, (int)strlen(g_html), 0, g_out, 65536, &st) > 0);
        TEST_ASSERT_NULL(strstr(g_out, "<div"));
    }
}

static int find_mark(const char *out, const char *before) {
    const char *p = strstr(out, before);
    int k = 0;
    if (!p) return -1;
    for (p += strlen(before); *p >= '0' && *p <= '9'; p++) k = k * 10 + (*p - '0');
    return *p == '"' ? k : -1;
}

/* the two passes with a box scroller: its scroll offset drives the
 * colour, kept across renders by its mark */
static void test_l105_two_passes_box(void) {
    static char first[65536];
    const wf_geom_box_t *b;
    char want[64];
    int k, s, ms, c;
    reduce(SCR, "<div id=o><p id=i>i</p>" FILL "</div><p>q</p>", 1);
    k = find_mark(g_out, "<div id=\"wg");
    TEST_ASSERT_TRUE(k >= 0 && k < WF_GEOM_MAX);
    TEST_ASSERT_NOT_NULL(strstr(g_out, "style=\"overflow-y:auto;height:60px\""));
    TEST_ASSERT_NULL(strstr(g_out, "color:#"));
    memcpy(first, g_out, sizeof(first));
    we_forms_init(&g_forms);
    we_set_forms(&g_doc, &g_forms);
    lay(&g_doc, first, 480);
    memset(&g_g, 0, sizeof(g_g));
    TEST_ASSERT_EQUAL(1, we_geom_feed(&g_doc, "wg", &g_g));
    b = &g_g.box[k];
    TEST_ASSERT_EQUAL(1, b->sv);
    TEST_ASSERT_EQUAL(60, b->s_vp);
    TEST_ASSERT_EQUAL(0, b->s_sc);
    TEST_ASSERT_TRUE(b->s_rg > 60);
    /* at offset 0: the from colour */
    wf_html_set_cssgeom(&g_g);
    reduce(SCR, "<div id=o><p id=i>i</p>" FILL "</div><p>q</p>", 1);
    TEST_ASSERT_NOT_NULL(strstr(g_out, "<p style=\"color:#000000\">i</p>"));
    /* scrolled on its track */
    snprintf(want, sizeof(want), "wg%d", k);
    s = we_find_id(&g_doc, want);
    TEST_ASSERT_TRUE(s >= 0);
    TEST_ASSERT_EQUAL(2, we_box_scroll_click(&g_doc, g_doc.nodes[s].x + g_doc.nodes[s].w - 3, g_doc.nodes[s].y + 55));
    TEST_ASSERT_EQUAL(0, we_layout_viewport(&g_doc, 320, 480));
    TEST_ASSERT_TRUE(g_doc.nodes[s].box_scroll_y > 0);
    memset(&g_g, 0, sizeof(g_g));
    TEST_ASSERT_EQUAL(1, we_geom_feed(&g_doc, "wg", &g_g));
    TEST_ASSERT_EQUAL(g_doc.nodes[s].box_scroll_y, b->s_sc);
    ms = (int)(1000LL * b->s_sc / b->s_rg);
    c = (255 * ms + 500) / 1000;
    snprintf(want, sizeof(want), "<p style=\"color:#%02x%02x%02x\">i</p>", c, c, c);
    reduce(SCR, "<div id=o><p id=i>i</p>" FILL "</div><p>q</p>", 1);
    TEST_ASSERT_NOT_NULL(strstr(g_out, want));
    TEST_ASSERT_TRUE(c > 0);
    /* the restyled page laid out with the same forms keeps the offset */
    we_set_forms(&g_doc2, &g_forms);
    lay(&g_doc2, g_out, 480);
    TEST_ASSERT_EQUAL(g_doc.nodes[s].box_scroll_y, g_doc2.nodes[we_find_id(&g_doc2, g_doc.nodes[s].id)].box_scroll_y);
    we_set_forms(&g_doc2, 0);
    we_set_forms(&g_doc, 0);
    wf_html_set_cssgeom(0);
}

/* the GUI position: the root scroll offset of the layout */
static void test_l105_two_passes_gui(void) {
    static char first[65536];
    const char *css = "#i { animation: k 1000ms linear; animation-timeline: scroll(root) }";
    char want[64];
    int ms, c;
    wf_html_set_cssscroll(5000);
    reduce(css, "<p id=i>i</p>" FILL FILL FILL FILL, 0);
    /* the set position: half way, #808080; nothing live */
    TEST_ASSERT_NOT_NULL(strstr(g_out, "<p style=\"color:#808080\">i</p>"));
    TEST_ASSERT_EQUAL(0, wf_html_sitecss_geom_live());
    TEST_ASSERT_EQUAL(0, wf_html_sitecss_geom_need());
    wf_html_set_cssscroll_gui(1);
    reduce(css, "<p id=i>i</p>" FILL FILL FILL FILL, 0);
    TEST_ASSERT_NULL(strstr(g_out, "color:#"));
    TEST_ASSERT_EQUAL(1, wf_html_sitecss_geom_live());
    TEST_ASSERT_EQUAL(1, wf_html_sitecss_geom_need());
    memcpy(first, g_out, sizeof(first));
    lay(&g_doc, first, 480);
    TEST_ASSERT_EQUAL(200, we_scroll(&g_doc, 200));
    memset(&g_g, 0, sizeof(g_g));
    (void)we_geom_feed(&g_doc, "wg", &g_g);
    TEST_ASSERT_EQUAL(200, g_g.sc_y);
    TEST_ASSERT_TRUE(g_g.rg_y > 200);
    wf_html_set_cssgeom(&g_g);
    reduce(css, "<p id=i>i</p>" FILL FILL FILL FILL, 0);
    ms = (int)(1000LL * 200 / g_g.rg_y);
    c = (255 * ms + 500) / 1000;
    snprintf(want, sizeof(want), "<p style=\"color:#%02x%02x%02x\">i</p>", c, c, c);
    TEST_ASSERT_NOT_NULL(strstr(g_out, want));
    /* the boxes do not move: the root position stays */
    lay(&g_doc2, g_out, 480);
    TEST_ASSERT_EQUAL(g_doc.perf.page_h, g_doc2.perf.page_h);
    wf_html_set_cssscroll_gui(0);
    wf_html_set_cssgeom(0);
    reduce(css, "<p id=i>i</p>" FILL FILL FILL FILL, 0);
    TEST_ASSERT_NOT_NULL(strstr(g_out, "<p style=\"color:#808080\">i</p>"));
    TEST_ASSERT_EQUAL(0, wf_html_sitecss_geom_live());
    wf_html_set_cssscroll(0);
}

int main(void) {
    unity_init();
    RUN_TEST(test_l105_geom_own);
    RUN_TEST(test_l105_reduce_divs);
    RUN_TEST(test_l105_two_passes_box);
    RUN_TEST(test_l105_two_passes_gui);
    unity_print_results();
    unity_cleanup();
    return unity_stats.tests_failed == 0 ? 0 : 1;
}
