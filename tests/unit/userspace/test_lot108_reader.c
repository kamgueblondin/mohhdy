/* Lot 108 (US-031): relative colours (CSS Color 5) in the reader's
 * colours, box scrollers along x (overflow-x with width and max-width,
 * min-height) and scroll timelines on the inline axis of a box scroller.
 * Hand cases next to the generated test_lot108_*.c; expected values
 * follow docs/lot108_plan.md. */
#include "../../framework/unity.h"
#include <string.h>
#include <stdio.h>
#include "wf_cssprop.h"
#include "wf_sitecss.h"

static char g_o[512];
static wf_site_t g_d;
static char g_html[4096];
static wf_geom_t g_g;

static const char *col(const char *v) {
    unsigned int rgb = 0;
    int a = 0;
    if (!wf_cssprop_color(v, (int)strlen(v), &rgb, &a)) return "0";
    snprintf(g_o, sizeof(g_o), "%06x %d", rgb, a);
    return g_o;
}

static void prep(const char *css) {
    snprintf(g_html, sizeof(g_html), "<html><head><style>%s</style></head><body><div id=o><div id=b><p id=i>x</p>"
             "</div></div></body></html>", css);
    TEST_ASSERT_EQUAL(0, wf_site_prepare_ext(&g_d, g_html, (int)strlen(g_html), 640, 480, 0, 0));
}

static const char *style_of(const char *id) {
    int i;
    g_o[0] = 0;
    for (i = 0; i < g_d.nnodes; i++)
        if (!strcmp(g_d.node[i].id, id)) wf_site_style_attr(&g_d, g_d.node[i].start, g_o, (int)sizeof(g_o));
    return g_o;
}

static const char *style_of_tag(const char *tag) {
    int i;
    g_o[0] = 0;
    for (i = 0; i < g_d.nnodes; i++)
        if (!strcmp(g_d.node[i].tag, tag)) wf_site_style_attr(&g_d, g_d.node[i].start, g_o, (int)sizeof(g_o));
    return g_o;
}

/* the keywords, the alpha left out (the origin's) or given, calc() */
static void test_l108_rel_basic(void) {
    TEST_ASSERT_EQUAL_STRING("ff0000 255", col("rgb(from red r g b)"));
    TEST_ASSERT_EQUAL_STRING("0000ff 255", col("rgb(from red b g r)"));
    TEST_ASSERT_EQUAL_STRING("ff0000 128", col("rgb(from #ff000080 r g b)"));
    TEST_ASSERT_EQUAL_STRING("ff0000 64", col("rgb(from #ff000080 r g b / calc(alpha / 2))"));
    TEST_ASSERT_EQUAL_STRING("ff0000 255", col("rgb(from #ff000080 r g b / 1)"));
    TEST_ASSERT_EQUAL_STRING("3d6699 128", col("rgba(from #336699 calc(r + 10) g b/0.5)"));
    /* 127.5 rounds up; a channel past 255 is clamped as in rgb() */
    TEST_ASSERT_EQUAL_STRING("80ff00 255", col("rgb(from red calc(r / 2) calc(r * 2) none)"));
    /* hsl: h in degrees, s and l numbers of 0..100 */
    TEST_ASSERT_EQUAL_STRING("00ff00 255", col("hsl(from red calc(h + 120) s l)"));
    TEST_ASSERT_EQUAL_STRING("990000 255", col("hsla(from red h s calc(l - 20))"));
    TEST_ASSERT_EQUAL_STRING("808080 255", col("hsl(from #808080 calc(h + 90) s l)"));     /* a grey: h 0, s 0 */
    /* oklab and oklch from their own values, from sRGB otherwise */
    TEST_ASSERT_EQUAL_STRING("ff0000 255", col("oklab(from #ff0000 l a b)"));
    TEST_ASSERT_EQUAL_STRING("636363 255", col("oklab(from black calc(l + 0.5) a b)"));
    TEST_ASSERT_EQUAL_STRING("d58679 255", col("oklch(from oklch(0.7 0.1 30) l c h)"));
    TEST_ASSERT_EQUAL_STRING("009cb2 255", col("oklch(from red l c calc(h + 180))"));
    /* any letter case */
    TEST_ASSERT_EQUAL_STRING("ff0000 255", col("RGB(FROM Red R G B)"));
}

/* the forms that are not relative colours */
static void test_l108_rel_invalid(void) {
    TEST_ASSERT_EQUAL_STRING("0", col("rgb(from red, r g b)"));
    TEST_ASSERT_EQUAL_STRING("0", col("rgb(from red r, g, b)"));
    TEST_ASSERT_EQUAL_STRING("0", col("rgb(from red r g)"));
    TEST_ASSERT_EQUAL_STRING("0", col("rgb(from red r g b b)"));
    TEST_ASSERT_EQUAL_STRING("0", col("rgb(from red r g b / alpha / 1)"));
    TEST_ASSERT_EQUAL_STRING("0", col("rgb(from red r g / b alpha)"));
    TEST_ASSERT_EQUAL_STRING("0", col("rgb(from red r (g) b)"));       /* a group is no math function */
    TEST_ASSERT_EQUAL_STRING("0", col("rgb(from red r g calc(b + 10%))"));   /* a number and a percentage */
    TEST_ASSERT_EQUAL_STRING("0", col("rgb(from red r g calc(b + 1px))"));
    TEST_ASSERT_EQUAL_STRING("0", col("rgb(from red r g h)"));         /* not a keyword of rgb() */
    TEST_ASSERT_EQUAL_STRING("0", col("rgb(from red r g calc(b + x))"));
    TEST_ASSERT_EQUAL_STRING("0", col("rgb(from nocolour r g b)"));
    TEST_ASSERT_EQUAL_STRING("0", col("rgb(fromred r g b)"));
    TEST_ASSERT_EQUAL_STRING("0", col("rgb(from red r g b))"));
    TEST_ASSERT_EQUAL_STRING("0", col("rgb(from red r g calc(b)"));
    TEST_ASSERT_EQUAL_STRING("0", col("rgb(from red) r g b)"));
    TEST_ASSERT_EQUAL_STRING("0", col("hwb(from red h w b)"));          /* only rgb, hsl, oklab and oklch */
    TEST_ASSERT_EQUAL_STRING("0", col("rgb(from currentcolor r g b)")); /* no current colour */
    TEST_ASSERT_EQUAL_STRING("0", col("oklaba(from red l a b)"));       /* an a only after rgb and hsl */
    TEST_ASSERT_EQUAL_STRING("0", col("rgb(fromxred r g b)"));          /* from and a space */
    TEST_ASSERT_EQUAL_STRING("0", col("rgb(from red calc(2r) g b)"));   /* no keyword glued to a number */
    TEST_ASSERT_EQUAL_STRING("0", col("rgb(from red r g b / / 1)"));
    TEST_ASSERT_EQUAL_STRING("0", col("rgb(from red calc(r / 0) g b)")); /* no known value */
    TEST_ASSERT_EQUAL_STRING("0", col("rgb(from red calc(infinity) g b)"));
    TEST_ASSERT_EQUAL_STRING("0", col("hsl(from red 0, 100%, 50%)"));
    TEST_ASSERT_EQUAL_STRING("0", col("rgb(from red r g calc(b)))"));
    TEST_ASSERT_EQUAL(0, wf_cssprop_match(WF_CSSPROP_COLOR, "rgb(from red r g)"));
    TEST_ASSERT_EQUAL(1, wf_cssprop_match(WF_CSSPROP_COLOR, "rgb(from red r g b)"));
    /* an origin longer than a value buffer (wf_cssprop_match takes any
     * length): refused, nothing copied past the buffer */
    {
        static char big[400];
        int k;
        strcpy(big, "rgb(from color-mix(in srgb, ");
        for (k = 0; k < 100; k++) strcat(big, " ");
        strcat(big, "red, blue) r g b)");
        TEST_ASSERT_EQUAL(0, wf_cssprop_match(WF_CSSPROP_COLOR, big));
    }
}

/* nesting: an origin may be relative, four levels deep at most */
static void test_l108_rel_nest(void) {
    TEST_ASSERT_EQUAL_STRING("ff0000 255", col("rgb(from rgb(from blue b g r) r g b)"));
    TEST_ASSERT_EQUAL_STRING("0000ff 255", col("rgb(from rgb(from rgb(from rgb(from red b g r) b g r) b g r) r g b)"));
    TEST_ASSERT_EQUAL_STRING("0", col("rgb(from rgb(from rgb(from rgb(from rgb(from red b g r) b g r) b g r) r g b) r g b)"));
    /* a color-mix() origin */
    TEST_ASSERT_EQUAL_STRING("800080 255", col("rgb(from color-mix(in srgb, red, blue) r g b)"));
    /* currentcolor once it has a value */
    wf_cssprop_set_current(1, 0x336699u, 255);
    TEST_ASSERT_EQUAL_STRING("996633 255", col("rgb(from currentcolor b g r)"));
    TEST_ASSERT_EQUAL(1, wf_cssprop_color_cur("rgb(from currentcolor r g b)", 28));
    wf_cssprop_set_current(0, 0, 0);
}

/* the reader paints them: own colours, currentcolor, keyframes */
static void test_l108_rel_reader(void) {
    prep("#i { color: rgb(from #336699 r g calc(b - 16)) } #o { background-color: hsl(from red calc(h + 120) s l / 0.5) }");
    TEST_ASSERT_EQUAL_STRING(" style=\"color:#336689\"", style_of("i"));
    TEST_ASSERT_EQUAL_STRING(" style=\"background-color:#00ff0080\"", style_of("o"));
    prep("#o { color: #102030 } #i { color: rgb(from currentcolor calc(r * 2) g b) }");
    TEST_ASSERT_EQUAL_STRING(" style=\"color:#202030\"", style_of("i"));
    prep("@keyframes k { from { color: rgb(from red r g b) } to { color: rgb(from red b g r) } } "
         "#i { animation: k 1s linear -0.5s paused }");
    TEST_ASSERT_EQUAL_STRING(" style=\"color:#800080\"", style_of("i"));
}

/* box scrollers along x: overflow-x with width or max-width passed for a
 * box a timeline reads; min-height with a box passed along y */
static void test_l108_xbox(void) {
    prep("#o { overflow-x: auto; width: 10em; max-width: none } #i { animation: k 1s; animation-timeline: scroll(x) } "
         "@keyframes k { to { color: red } }");
    TEST_ASSERT_EQUAL_STRING(" style=\"overflow-x:auto;width:160px\"", style_of("o"));
    /* max-width: none wins over an earlier max-width */
    prep("#o { overflow-x: auto; width: 10em; max-width: 50px } #o { max-width: none } "
         "#i { animation: k 1s; animation-timeline: scroll(x) } @keyframes k { to { color: red } }");
    TEST_ASSERT_EQUAL_STRING(" style=\"overflow-x:auto;width:160px\"", style_of("o"));
    /* html and body are no box scrollers: their timelines are the root's,
     * nothing is passed on for them */
    prep("body { overflow-x: auto; width: 300px; animation: k 1s; animation-timeline: scroll(self x) } "
         "@keyframes k { to { color: red } }");
    TEST_ASSERT_EQUAL_STRING("", style_of_tag("body"));
    prep("body { overflow-y: auto; height: 100px; scroll-timeline: --b } html { overflow: auto; height: 50px; scroll-timeline: --h x } "
         "#i { animation: k 1s; animation-timeline: --b } #b { animation: k 1s; animation-timeline: --h } @keyframes k { to { color: red } }");
    TEST_ASSERT_EQUAL(0, g_d.node[0].scr);
    TEST_ASSERT_EQUAL(0, g_d.node[2].scr);
    TEST_ASSERT_EQUAL_STRING("", style_of_tag("body"));
    prep("#o { overflow-x: auto; width: 10em; max-width: none } #i { animation: k 1s; animation-timeline: scroll(x) } "
         "@keyframes k { to { color: red } }");
    /* overflow-x alone: y computes to auto, but no height to pass */
    TEST_ASSERT_EQUAL(4, wf_site_box_scroller(&g_d, 3));
    /* min-height alone passes nothing; with a height it goes along */
    prep("#o { overflow: auto; min-height: 50px } #i { animation: k 1s; animation-timeline: scroll() } "
         "@keyframes k { to { color: red } }");
    TEST_ASSERT_EQUAL_STRING("", style_of("o"));
    prep("#o { overflow: auto; min-height: 50px; max-height: 10vh; max-width: 50% } "
         "#i { animation: k 1s; animation-timeline: scroll() } @keyframes k { to { color: red } }");
    TEST_ASSERT_EQUAL_STRING(" style=\"overflow-y:auto;max-height:48px;min-height:50px;overflow-x:auto;max-width:50%\"",
                             style_of("o"));
    /* hidden along x: only y passed */
    prep("#o { overflow-x: hidden; overflow-y: scroll; height: 40px; width: 100px } "
         "#i { animation: k 1s; animation-timeline: scroll(inline) } @keyframes k { to { color: red } }");
    TEST_ASSERT_EQUAL_STRING(" style=\"overflow-y:scroll;height:40px\"", style_of("o"));
    /* the root: no inline axis */
    prep("#i { animation: k 1s; animation-timeline: scroll(root inline) } @keyframes k { to { color: red } }");
    TEST_ASSERT_EQUAL(0, g_d.col_animated);
    TEST_ASSERT_EQUAL(0, g_d.anim_scroll);
    TEST_ASSERT_EQUAL(0, g_d.geom_live);
    prep("#i { animation: k 1s; animation-timeline: scroll(root) } @keyframes k { to { color: red } }");
    TEST_ASSERT_EQUAL(1, g_d.col_animated);
    TEST_ASSERT_EQUAL(1, g_d.anim_scroll);
}

/* a named scroll timeline on the x axis of a box scroller, run from the
 * geometry along x */
static void test_l108_named_x(void) {
    memset(&g_g, 0, sizeof(g_g));
    g_g.vw = 640; g_g.vh = 480;
    g_g.box[3].ok = 1;
    g_g.box[3].svx = 1; g_g.box[3].x_vp = 100; g_g.box[3].x_rg = 400; g_g.box[3].x_sc = 100;
    g_g.box[3].sv = 1; g_g.box[3].s_vp = 50; g_g.box[3].s_rg = 400; g_g.box[3].s_sc = 300;
    wf_site_set_geom(&g_g);
    prep("@keyframes k { from { color: #000000 } to { color: #ffffff } } "
         "#o { overflow: auto; width: 100px; scroll-timeline: --s x } #i { animation: k 1s linear; animation-timeline: --s }");
    TEST_ASSERT_EQUAL_STRING(" style=\"color:#404040\"", style_of("i"));      /* 100 / 400 */
    prep("@keyframes k { from { color: #000000 } to { color: #ffffff } } "
         "#o { overflow: auto; width: 100px; scroll-timeline: --s block } #i { animation: k 1s linear; animation-timeline: --s }");
    TEST_ASSERT_EQUAL_STRING(" style=\"color:#bfbfbf\"", style_of("i"));      /* 300 / 400 */
    /* no range along x in the geometry: no effect */
    g_g.box[3].svx = 0;
    wf_site_set_geom(&g_g);
    prep("@keyframes k { from { color: #000000 } to { color: #ffffff } } "
         "#o { overflow: auto; width: 100px; scroll-timeline: --s inline } #i { animation: k 1s linear; animation-timeline: --s }");
    TEST_ASSERT_EQUAL_STRING("", style_of("i"));
    TEST_ASSERT_EQUAL(1, g_d.node[3].scr);
    wf_site_set_geom(0);
}

int main(void) {
    unity_init();
    RUN_TEST(test_l108_rel_basic);
    RUN_TEST(test_l108_rel_invalid);
    RUN_TEST(test_l108_rel_nest);
    RUN_TEST(test_l108_rel_reader);
    RUN_TEST(test_l108_xbox);
    RUN_TEST(test_l108_named_x);
    unity_print_results();
    unity_cleanup();
    return unity_stats.tests_failed == 0 ? 0 : 1;
}
