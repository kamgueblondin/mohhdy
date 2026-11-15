/* Lot 104 (US-031): hand-written cases next to the generated suites
 * (test_lot104_view.c, test_lot104_parse.c, test_lot104_sheet.c): view
 * progress timelines in the reader from a layout geometry (view(), named
 * view timelines with view-timeline-inset, the named ranges and length
 * edges, scroll timelines with a length edge), the counters and the
 * witnesses of the new branches. Each value below is worked out from CSS
 * Scroll-driven Animations 1 as docs/lot104_plan.md sets it: with S the
 * subject start, E its end and V the scrollport, cover is
 * [S - V + ie, E - is]; 50% of linear black to white is #808080, 25%
 * #404040. */
#include "../../framework/unity.h"
#include <string.h>
#include <stdio.h>
#include "wf_sitecss.h"
#include "wf_cssprop.h"

#define KB "@keyframes k { from { color: #000000 } to { color: #ffffff } } "
#define AN "#i { animation: k 1000ms linear; animation-timeline: "
#define GRAY " style=\"color:#808080\""
#define QUARTER " style=\"color:#404040\""

static wf_site_t g_d;
static wf_geom_t g_g;
static char g_html[8192];
static char g_o[1400];

static void prep(const char *css, int pm) {
    snprintf(g_html, sizeof(g_html), "<html><head><style>%s</style></head><body>"
             "<div id=o><p id=i>i</p></div><p id=q>q</p></body></html>", css);
    wf_site_set_scroll(pm);
    TEST_ASSERT_EQUAL(0, wf_site_prepare_ext(&g_d, g_html, (int)strlen(g_html), 640, 480, 0, 0));
    wf_site_set_scroll(0);
}

static int node(const char *id) {
    int i;
    for (i = 0; i < g_d.nnodes; i++) if (!strcmp(g_d.node[i].id, id)) return i;
    return -1;
}

static const char *style_of(const char *id) {
    int i = node(id);
    g_o[0] = 0;
    if (i >= 0) wf_site_style_attr(&g_d, g_d.node[i].start, g_o, (int)sizeof(g_o));
    return g_o;
}

static const char *anim_text(void) {
    static char t[256];
    wf_site_anim_text(&g_d, t, (int)sizeof(t));
    return t;
}

/* the root scroller (640 x 480, vertical range rg) */
static void geo_root(int rg) {
    memset(&g_g, 0, sizeof(g_g));
    g_g.vw = 640; g_g.vh = 480; g_g.rg_y = rg;
}

/* a box on the root scroller (horizontal-tb: block = y) for element id,
 * found on a first read of the page */
static void geo_box(const char *css, const char *id, int root, int bvert, int b_off, int b_size) {
    int i;
    wf_geom_box_t *b;
    wf_site_set_geom(0);
    prep(css, 0);
    i = node(id);
    TEST_ASSERT_TRUE(i >= 0 && i < WF_GEOM_MAX);
    b = &g_g.box[i];
    b->ok = 1; b->root = (unsigned char)root; b->bvert = (unsigned char)bvert;
    b->b_off = b_off; b->b_size = b_size; b->i_off = 0; b->i_size = 640;
    b->vp_b = bvert ? 480 : 640; b->vp_i = bvert ? 640 : 480;
    b->rg_b = bvert ? g_g.rg_y : 0; b->rg_i = bvert ? 0 : g_g.rg_y;
    g_g.n++;
}

static void gprep(const char *css, int pm) {
    wf_site_set_geom(&g_g);
    prep(css, pm);
    wf_site_set_geom(0);
}

/* view() on #i: S 1100, E 1200, V 480, so cover is [620, 1200]; the
 * root range 1000 makes pm 9100 the offset 910, its middle */
static void test_l104_view_fn(void) {
    const char *css = KB AN "view() }";
    geo_root(1000);
    geo_box(css, "i", 1, 1, 1100, 100);
    gprep(css, 9100);
    TEST_ASSERT_EQUAL_STRING(GRAY, style_of("i"));
    TEST_ASSERT_EQUAL(1, g_d.anim_view);
    TEST_ASSERT_EQUAL(0, g_d.anim_scroll);
    TEST_ASSERT_EQUAL(1, g_d.geom_need);
    TEST_ASSERT_EQUAL(1, g_d.geom_used);
    TEST_ASSERT_EQUAL_STRING("keyframes 1 animated 0 transitioned 0 font 0 at 0ms color 1 0 view 1 need 1 geom 1",
                             anim_text());
    /* a quarter: 620 + 145 = 765 */
    gprep(css, 7650);
    TEST_ASSERT_EQUAL_STRING(QUARTER, style_of("i"));
    /* before the cover range (fill none): no effect, still on a view
     * timeline */
    gprep(css, 6000);
    TEST_ASSERT_EQUAL_STRING("", style_of("i"));
    TEST_ASSERT_EQUAL(1, g_d.anim_view);
    /* no geometry: the timeline is inactive and asks for it */
    prep(css, 9100);
    TEST_ASSERT_EQUAL_STRING("", style_of("i"));
    TEST_ASSERT_EQUAL(0, g_d.anim_view);
    TEST_ASSERT_EQUAL(0, g_d.anim_scroll);
    TEST_ASSERT_EQUAL(1, g_d.geom_need);
    TEST_ASSERT_EQUAL(0, g_d.geom_used);
    TEST_ASSERT_EQUAL_STRING("keyframes 1 animated 0 transitioned 0 font 0 at 0ms view 0 need 1 geom 0", anim_text());
    /* a box scroller (not the root): inactive, the box was read */
    geo_root(1000);
    geo_box(css, "i", 0, 1, 1100, 100);
    gprep(css, 9100);
    TEST_ASSERT_EQUAL_STRING("", style_of("i"));
    TEST_ASSERT_EQUAL(0, g_d.anim_view);
    TEST_ASSERT_EQUAL(1, g_d.geom_used);
    /* a box record that is not filled (ok 0) is not read, whatever its
     * other fields say (mutation witness) */
    geo_root(1000);
    geo_box(css, "i", 1, 1, 1100, 100);
    g_g.box[node("i")].ok = 0;
    gprep(css, 9100);
    TEST_ASSERT_EQUAL_STRING("", style_of("i"));
    TEST_ASSERT_EQUAL(0, g_d.anim_view);
    TEST_ASSERT_EQUAL(0, g_d.geom_used);
    /* another element's box does not count */
    geo_root(1000);
    geo_box(css, "q", 1, 1, 1100, 100);
    gprep(css, 9100);
    TEST_ASSERT_EQUAL_STRING("", style_of("i"));
    TEST_ASSERT_EQUAL(1, g_d.geom_need);
    TEST_ASSERT_EQUAL(0, g_d.geom_used);
    /* no @keyframes for the name: nothing is asked */
    prep("#i { animation: zz 1s linear; animation-timeline: view() }", 0);
    TEST_ASSERT_EQUAL(0, g_d.geom_need);
    TEST_ASSERT_EQUAL_STRING("keyframes 0 animated 0 transitioned 0 font 0 at 0ms", anim_text());
}

/* the axes: the reader scrolls the root along y only */
static void test_l104_view_axes(void) {
    /* inline on a horizontal-tb scroller is x, at 0: I 0..640 in a 640
     * view, cover [-640, 640], 0 is its middle at any pm */
    const char *css = KB AN "view(inline) }";
    geo_root(1000);
    geo_box(css, "i", 1, 1, 1100, 100);
    gprep(css, 0);
    TEST_ASSERT_EQUAL_STRING(GRAY, style_of("i"));
    gprep(css, 10000);
    TEST_ASSERT_EQUAL_STRING(GRAY, style_of("i"));
    /* a horizontal scroll range is not scrolled by the reader: still the
     * middle (at 1000 px it would be past cover; mutation witness) */
    g_g.box[node("i")].rg_i = 1000;
    gprep(css, 10000);
    TEST_ASSERT_EQUAL_STRING(GRAY, style_of("i"));
    css = KB AN "view(x) }";
    gprep(css, 3000);
    TEST_ASSERT_EQUAL_STRING(GRAY, style_of("i"));
    css = KB AN "view(y) }";
    gprep(css, 9100);
    TEST_ASSERT_EQUAL_STRING(GRAY, style_of("i"));
    /* a vertical scroller (bvert 0): block is x at 0, y is inline; the
     * box B 0..640 in a 640 view gives the middle at 0 */
    css = KB AN "view(block) }";
    geo_root(1000);
    geo_box(css, "i", 1, 0, 0, 640);
    gprep(css, 4000);
    TEST_ASSERT_EQUAL_STRING(GRAY, style_of("i"));
    /* y: the inline axis, I 0..640 (i_size) in a 480 view, the range
     * 1000: cover [-480, 640], its middle 80 is pm 800 */
    css = KB AN "view(y) }";
    gprep(css, 800);
    TEST_ASSERT_EQUAL_STRING(GRAY, style_of("i"));
    gprep(css, 0);
    TEST_ASSERT_EQUAL_STRING(" style=\"color:#6d6d6d\"", style_of("i"));
}

/* insets: view(10px 20%): is 10, ie 96 (20% of 480); cover
 * [1100 - 480 + 96, 1200 - 10] = [716, 1190], middle 953 */
static void test_l104_view_insets(void) {
    const char *css = KB AN "view(10px 20%) }";
    geo_root(1000);
    geo_box(css, "i", 1, 1, 1100, 100);
    gprep(css, 9530);
    TEST_ASSERT_EQUAL_STRING(GRAY, style_of("i"));
    /* one inset is both: 50px, cover [670, 1150], middle 910 */
    css = KB AN "view(50px) }";
    gprep(css, 9100);
    TEST_ASSERT_EQUAL_STRING(GRAY, style_of("i"));
    /* auto is 0 */
    css = KB AN "view(block auto) }";
    gprep(css, 9100);
    TEST_ASSERT_EQUAL_STRING(GRAY, style_of("i"));
}

/* named view timelines on #o (S 1100, E 1200) */
static void test_l104_view_named(void) {
    const char *css = KB "#o { view-timeline: --v block 10px 20% } " AN "--v }";
    geo_root(1000);
    geo_box(css, "o", 1, 1, 1100, 100);
    gprep(css, 9530);
    TEST_ASSERT_EQUAL_STRING(GRAY, style_of("i"));
    TEST_ASSERT_EQUAL(1, g_d.anim_view);
    TEST_ASSERT_EQUAL(1, g_d.geom_need);
    TEST_ASSERT_EQUAL(1, g_d.geom_used);
    /* view-timeline-inset after the shorthand */
    css = KB "#o { view-timeline: --v block 10px 20%; view-timeline-inset: 50px } " AN "--v }";
    gprep(css, 9100);
    TEST_ASSERT_EQUAL_STRING(GRAY, style_of("i"));
    /* initial resets the insets; an invalid one is dropped */
    css = KB "#o { view-timeline-name: --v; view-timeline-inset: 50px; view-timeline-inset: initial } " AN "--v }";
    gprep(css, 9100);
    TEST_ASSERT_EQUAL_STRING(GRAY, style_of("i"));
    css = KB "#o { view-timeline-name: --v; view-timeline-inset: 50px; view-timeline-inset: 1em } " AN "--v }";
    gprep(css, 9100);
    TEST_ASSERT_EQUAL_STRING(GRAY, style_of("i"));
    /* view-timeline initial clears the name, axis and insets */
    css = KB "#o { view-timeline: --v 50px; view-timeline: initial } " AN "--v }";
    gprep(css, 9100);
    TEST_ASSERT_EQUAL_STRING("", style_of("i"));
    TEST_ASSERT_EQUAL(0, g_d.geom_need);
    /* the insets follow the name list: --v gets the second */
    css = KB "#o { view-timeline-name: --u, --v; view-timeline-inset: 0px, 10px 20% } " AN "--v }";
    gprep(css, 9530);
    TEST_ASSERT_EQUAL_STRING(GRAY, style_of("i"));
    /* view-timeline-axis inline: the subject's x, at 0: I 0..640, cover
     * [-640, 640] */
    css = KB "#o { view-timeline-name: --v; view-timeline-axis: inline } " AN "--v }";
    gprep(css, 0);
    TEST_ASSERT_EQUAL_STRING(GRAY, style_of("i"));
    /* a scroll timeline defined on #o: not the root, no effect */
    css = KB "#o { scroll-timeline: --v } " AN "--v }";
    gprep(css, 9100);
    TEST_ASSERT_EQUAL_STRING("", style_of("i"));
    TEST_ASSERT_EQUAL(0, g_d.geom_need);
}

/* the named ranges: S 500, E 600, V 480: cover [20, 600], contain
 * [120, 500], entry [20, 120], exit [500, 600], entry-crossing [20, 120],
 * exit-crossing [500, 600] */
static void test_l104_view_ranges(void) {
    static const struct { const char *range; int pm; } c[] = {
        {"entry", 700},          {"exit", 5500},           {"contain", 3100},        {"cover", 3100},
        {"entry-crossing", 700}, {"exit-crossing", 5500},  {"scroll", 5000},
        {"cover 100px cover 300px", 2200}, {"entry 50% exit 50%", 3100}, {"100px 300px", 2200}};
    char css[256];
    int k;
    geo_root(1000);
    snprintf(css, sizeof(css), KB AN "view(); animation-range: entry }");
    geo_box(css, "i", 1, 1, 500, 100);
    for (k = 0; k < (int)(sizeof(c) / sizeof(c[0])); k++) {
        snprintf(css, sizeof(css), KB AN "view(); animation-range: %s }", c[k].range);
        gprep(css, c[k].pm);
        TEST_ASSERT_EQUAL_STRING(GRAY, style_of("i"));
    }
    /* contain 10px is 130, contain 40% 120 + 152 = 272: the middle 201 */
    snprintf(css, sizeof(css), KB AN "view(); animation-range: contain 10px contain 40% }");
    gprep(css, 2010);
    TEST_ASSERT_EQUAL_STRING(GRAY, style_of("i"));
}

/* a scroll timeline with a length edge runs on the root range */
static void test_l104_scroll_px(void) {
    const char *css = KB AN "scroll(); animation-range: 100px 300px }";
    geo_root(1000);
    gprep(css, 2000);
    TEST_ASSERT_EQUAL_STRING(GRAY, style_of("i"));
    TEST_ASSERT_EQUAL(1, g_d.anim_scroll);
    TEST_ASSERT_EQUAL(0, g_d.anim_view);
    TEST_ASSERT_EQUAL(1, g_d.geom_need);
    TEST_ASSERT_EQUAL(0, g_d.geom_used);
    /* a range name counts as normal (Lot 102), its offset too: entry
     * 100px is 0, so [0, 300] at 200 is 666 ms, #aaaaaa */
    css = KB AN "scroll(); animation-range: entry 100px 300px }";
    gprep(css, 2000);
    TEST_ASSERT_EQUAL_STRING(" style=\"color:#aaaaaa\"", style_of("i"));
    /* no geometry, or an empty range: no effect */
    css = KB AN "scroll(); animation-range: 100px 300px }";
    prep(css, 2000);
    TEST_ASSERT_EQUAL_STRING("", style_of("i"));
    TEST_ASSERT_EQUAL(1, g_d.geom_need);
    TEST_ASSERT_EQUAL(0, g_d.anim_scroll);
    geo_root(0);
    gprep(css, 2000);
    TEST_ASSERT_EQUAL_STRING("", style_of("i"));
    /* a named scroll timeline on the root takes the same path */
    css = KB "html { scroll-timeline: --t } " AN "--t; animation-range: 100px 300px }";
    geo_root(1000);
    gprep(css, 2000);
    TEST_ASSERT_EQUAL_STRING(GRAY, style_of("i"));
    /* percentages alone keep the Lot 102 formula, with no geometry */
    css = KB AN "scroll(); animation-range: 0% 100% }";
    prep(css, 5000);
    TEST_ASSERT_EQUAL_STRING(GRAY, style_of("i"));
    TEST_ASSERT_EQUAL(0, g_d.geom_need);
}

/* the API witnesses */
static void test_l104_api(void) {
    wf_cssanim_t a;
    wf_cssview_t v;
    long long lo = -7, hi = -7;
    int ms = -7;
    memset(&a, 0, sizeof(a));
    a.dur = 1000; a.iters = 10000;
    /* Lot 102's formula has no scroll range for a length */
    a.rs = WF_CSSANIM_R_PCT; a.rsp = 100000; a.rsl = 1;
    TEST_ASSERT_EQUAL(0, wf_cssanim_range_ms(&a, 5000, &ms));
    TEST_ASSERT_EQUAL(0, ms);
    a.rsl = 0; a.re = WF_CSSANIM_R_PCT; a.rep = 1000000; a.rel = 1;
    ms = -7;
    TEST_ASSERT_EQUAL(0, wf_cssanim_range_ms(&a, 5000, &ms));
    a.rel = 0;
    TEST_ASSERT_EQUAL(1, wf_cssanim_range_ms(&a, 5000, &ms));
    /* -2e8 - 0.5 ms stays -2e8: T = 4e7 ms + 1000 x 1 ms, at -5 times
     * the range */
    memset(&a, 0, sizeof(a));
    a.delay = 40000000; a.dur = 1; a.iters = 1000 * 10000;
    TEST_ASSERT_EQUAL(1, wf_cssanim_scroll_px_ms(&a, 1000000, -5000000, &ms));
    TEST_ASSERT_EQUAL(-200000000, ms);
    TEST_ASSERT_EQUAL(1, wf_cssanim_scroll_px_ms(&a, 1000000, 6000000, &ms));
    TEST_ASSERT_EQUAL(200000000, ms);
    /* toward zero -2e8 exactly, floored -2e8 - 0.5: the clamp after the
     * floor keeps -2e8 (T = 1e8 ms, at -(2 + 5e-9) times the range;
     * mutation witness) */
    memset(&a, 0, sizeof(a));
    a.delay = 100000000; a.iters = 10000;
    TEST_ASSERT_EQUAL(1, wf_cssanim_scroll_px_ms(&a, 200000000, -400000001, &ms));
    TEST_ASSERT_EQUAL(-200000000, ms);
    TEST_ASSERT_EQUAL(1, wf_cssanim_scroll_px_ms(&a, 200000000, -399999997, &ms));
    TEST_ASSERT_EQUAL(-199999999, ms);     /* -199999998.5 floored */
    TEST_ASSERT_EQUAL(0, wf_cssanim_scroll_px_ms(&a, 0, 0, &ms));
    TEST_ASSERT_EQUAL(0, wf_cssanim_scroll_px_ms(0, 1000000, 0, &ms));
    TEST_ASSERT_EQUAL(0, wf_cssanim_scroll_px_ms(&a, 1000000, 0, 0));
    /* the ranges outside cover .. scroll, and null pointers */
    memset(&v, 0, sizeof(v));
    TEST_ASSERT_EQUAL(0, wf_cssview_range(&v, WF_CSSANIM_R_PCT, &lo, &hi));
    TEST_ASSERT_EQUAL(0, wf_cssview_range(&v, WF_CSSANIM_R_SCROLL + 1, &lo, &hi));
    TEST_ASSERT_EQUAL(0, wf_cssview_range(0, WF_CSSANIM_R_COVER, &lo, &hi));
    TEST_ASSERT_EQUAL(0, wf_cssview_range(&v, WF_CSSANIM_R_COVER, 0, &hi));
    TEST_ASSERT_EQUAL(0, wf_cssview_range(&v, WF_CSSANIM_R_COVER, &lo, 0));
    TEST_ASSERT_EQUAL(-7, (int)lo);
    TEST_ASSERT_EQUAL(0, wf_cssview_edge(0, 0, 0, 0, 0, &lo));
    TEST_ASSERT_EQUAL(0, wf_cssview_edge(&v, 0, 0, 0, 0, 0));
    TEST_ASSERT_EQUAL(0, wf_cssanim_view_ms(&a, &v, 0));
    TEST_ASSERT_EQUAL(0, wf_cssanim_view_ms(0, &v, &ms));
    TEST_ASSERT_EQUAL(0, wf_cssanim_view_ms(&a, 0, &ms));
    /* an infinite count has no effect */
    a.iters = -1;
    v.size = 1000000; v.view = 1000000;
    TEST_ASSERT_EQUAL(0, wf_cssanim_view_ms(&a, &v, &ms));
    /* auto is an inset, not a range edge length (mutation witness) */
    {
        wf_cssanim_t r[4];
        TEST_ASSERT_EQUAL(-1, wf_cssanim_long(WF_CSSANIM_L_RANGE_START, "auto", r, 4));
        TEST_ASSERT_EQUAL(-1, wf_cssanim_long(WF_CSSANIM_L_RANGE_END, "entry auto", r, 4));
        TEST_ASSERT_EQUAL(1, wf_cssanim_long(WF_CSSANIM_L_RANGE_END, "entry 2px", r, 4));
        TEST_ASSERT_EQUAL(20000, r[0].rep);
        TEST_ASSERT_EQUAL(1, r[0].rel);
    }
    /* the kinds after VSHORT are refused */
    {
        wf_csstl_t t[4];
        TEST_ASSERT_EQUAL(-1, wf_csstl_parse(WF_CSSTL_VSHORT + 1, "--a", t, 4));
        TEST_ASSERT_EQUAL(1, wf_csstl_parse(WF_CSSTL_VSHORT, "--a", t, 4));
        TEST_ASSERT_EQUAL(-1, wf_csstl_parse(WF_CSSTL_INSETS, "", t, 4));
    }
    TEST_ASSERT_EQUAL(6, WF_SITE_TLS);
    TEST_ASSERT_EQUAL(192, WF_GEOM_MAX);
}

/* the subject marks: the element whose box a view timeline needs */
static void test_l104_subject(void) {
    const char *css = KB AN "view() }";
    prep(css, 0);
    TEST_ASSERT_EQUAL(node("i"), wf_site_geom_subject(&g_d, g_d.node[node("i")].start));
    TEST_ASSERT_EQUAL(-1, wf_site_geom_subject(&g_d, g_d.node[node("q")].start));
    TEST_ASSERT_EQUAL(-1, wf_site_geom_subject(&g_d, -5));
    TEST_ASSERT_EQUAL(-1, wf_site_geom_subject(0, 0));
    /* a named timeline marks the element defining it */
    prep(KB "#o { view-timeline-name: --v } " AN "--v }", 0);
    TEST_ASSERT_EQUAL(node("o"), wf_site_geom_subject(&g_d, g_d.node[node("o")].start));
    TEST_ASSERT_EQUAL(-1, wf_site_geom_subject(&g_d, g_d.node[node("i")].start));
    /* a geometry cleared before the next prepare */
    geo_root(1000);
    geo_box(css, "i", 1, 1, 1100, 100);
    gprep(css, 9100);
    TEST_ASSERT_EQUAL_STRING(GRAY, style_of("i"));
    prep(css, 9100);
    TEST_ASSERT_EQUAL_STRING("", style_of("i"));
}

int main(void) {
    unity_init();
    RUN_TEST(test_l104_view_fn);
    RUN_TEST(test_l104_view_axes);
    RUN_TEST(test_l104_view_insets);
    RUN_TEST(test_l104_view_named);
    RUN_TEST(test_l104_view_ranges);
    RUN_TEST(test_l104_scroll_px);
    RUN_TEST(test_l104_api);
    RUN_TEST(test_l104_subject);
    unity_print_results();
    unity_cleanup();
    return unity_stats.tests_failed == 0 ? 0 : 1;
}
