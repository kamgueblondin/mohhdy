/* Lot 107 (US-031): font and viewport heights, percentages and max-height
 * on box scrollers (CSS Values 4, CSS Overflow 3), scroll-padding as the
 * auto insets of view timelines (CSS Scroll Snap 1, CSS Scroll-driven
 * Animations 1), legacy interpolation past the ends (CSS Color 4 12.3)
 * and the body's start offset. Hand cases next to the generated
 * test_lot107_*.c; expected values follow docs/lot107_plan.md. */
#include "../../framework/unity.h"
#include <string.h>
#include <stdio.h>
#include "wf_cssprop.h"
#include "wf_sitecss.h"

static wf_cssprop_t g_p;
static char g_o[512];
static wf_site_t g_d;
static char g_html[4096];
static wf_geom_t g_g;

static const char *ip(const char *a, const char *b, int t) {
    memset(&g_p, 0, sizeof(g_p));
    g_p.syntax = (unsigned char)wf_cssprop_syntax_parse("'<color>'", 9, &g_p);
    if (wf_cssprop_interp(&g_p, a, b, t, g_o, (int)sizeof(g_o)) != 2) return "NOT INTERPOLATED";
    return g_o;
}

/* legacy pairs past the ends: premultiplied with the unclamped alpha,
 * divided by it, the alpha clamped after (CSS Color 4 12.3) */
static void test_l107_legacy_order(void) {
    wf_cssprop_set_current(1, 0x000000u, 255);
    /* alpha 1 - 1.25 = -0.25, red -63.75 / -0.25 = 255, alpha 0 */
    TEST_ASSERT_EQUAL_STRING("rgba(255, 0, 0, 0)", ip("red", "transparent", 12500));
    /* alpha 2: 476 / 2 = 238 (Lot 106 clamped the alpha first: white) */
    TEST_ASSERT_EQUAL_STRING("rgb(238, 232, 170)", ip("transparent", "palegoldenrod", 20000));
    /* opaque pairs are as before */
    TEST_ASSERT_EQUAL_STRING("rgb(255, 0, 0)", ip("red", "blue", -5000));
    TEST_ASSERT_EQUAL_STRING("rgb(0, 0, 255)", ip("red", "blue", 15000));
    /* translucent below 0: alpha 0.502 + 0.249 / 2 = 0.253, red 192 / 0.253 */
    TEST_ASSERT_EQUAL_STRING("rgba(255, 0, 0, 0.253)", ip("#ff000080", "blue", -5000));
    /* the unclamped alpha exactly 0: channels 0 */
    TEST_ASSERT_EQUAL_STRING("rgba(0, 0, 0, 0)", ip("rgba(255, 0, 0, 0.5)", "rgba(0, 0, 255, 0.25)", 20000));
    /* within the ends nothing changes */
    TEST_ASSERT_EQUAL_STRING("rgba(255, 0, 0, 0.5)", ip("red", "transparent", 5000));
}

#define KF "@keyframes k { from { color: #000000 } to { color: #ffffff } } "
#define RD "#i { animation: k 1s linear; animation-timeline: scroll(nearest) } "
#define PAGE "<div id=o><div id=b><p id=i>i</p></div></div>"

static void prep(const char *css) {
    snprintf(g_html, sizeof(g_html), "<html><head><style>" KF "%s</style></head><body>" PAGE "</body></html>", css);
    TEST_ASSERT_EQUAL(0, wf_site_prepare_ext(&g_d, g_html, (int)strlen(g_html), 640, 480, 0, 0));
}

static const char *style_of(const char *id) {
    int i;
    g_o[0] = 0;
    for (i = 0; i < g_d.nnodes; i++)
        if (!strcmp(g_d.node[i].id, id)) wf_site_style_attr(&g_d, g_d.node[i].start, g_o, (int)sizeof(g_o));
    return g_o;
}

static const wf_site_node_t *node(const char *id) {
    int i;
    for (i = 0; i < g_d.nnodes; i++) if (!strcmp(g_d.node[i].id, id)) return &g_d.node[i];
    return &g_d.node[0];
}

/* the heights the reader passes on for #b, a box scroller #i reads */
static void test_l107_heights(void) {
    prep("#b { overflow: auto; height: 2em } " RD);
    TEST_ASSERT_EQUAL_STRING(" style=\"overflow-y:auto;height:32px\"", style_of("b"));
    TEST_ASSERT_EQUAL_STRING("~2em", node("b")->htok);
    TEST_ASSERT_EQUAL(0, node("b")->hpx);
    /* the element's own font size, inherited from #o */
    prep("#o { font-size: 12px } #b { overflow: auto; height: 2.5em } " RD);
    TEST_ASSERT_EQUAL_STRING(" style=\"overflow-y:auto;height:30px\"", style_of("b"));
    prep("#b { font-size: 12px; overflow: auto; height: 2.5rem } html { font-size: 10px } " RD);
    TEST_ASSERT_EQUAL_STRING(" style=\"font-size:12px;overflow-y:auto;height:25px\"", style_of("b"));
    /* the viewport: 640 x 480 */
    prep("#b { overflow: auto; height: 10vw } " RD);
    TEST_ASSERT_EQUAL_STRING(" style=\"overflow-y:auto;height:64px\"", style_of("b"));
    prep("#b { overflow: auto; height: 10vmax; max-height: 10vmin } " RD);
    TEST_ASSERT_EQUAL_STRING(" style=\"overflow-y:auto;height:64px;max-height:48px\"", style_of("b"));
    /* a long one goes to the pool ("=N"), resolved the same; past
     * 10000 px it does not resolve (wf_css_math_px) */
    prep("#b { overflow: auto; height: 123.45678em } " RD);
    TEST_ASSERT_EQUAL('=', node("b")->htok[0]);
    TEST_ASSERT_EQUAL_STRING(" style=\"overflow-y:auto;height:1975px\"", style_of("b"));
    prep("#b { overflow: auto; height: 625em } " RD);
    TEST_ASSERT_EQUAL_STRING(" style=\"overflow-y:auto;height:10000px\"", style_of("b"));
    prep("#b { overflow: auto; height: 626em } " RD);
    TEST_ASSERT_EQUAL_STRING("~626em", node("b")->htok);
    TEST_ASSERT_EQUAL_STRING("", style_of("b"));
    /* percentages: the integer part, a leading + dropped, capped */
    prep("#b { overflow: auto; height: +12.5% } " RD);
    TEST_ASSERT_EQUAL_STRING("%12.5", node("b")->htok);
    TEST_ASSERT_EQUAL_STRING(" style=\"overflow-y:auto;height:12%\"", style_of("b"));
    prep("#b { overflow: auto; height: 99999% } " RD);
    TEST_ASSERT_EQUAL_STRING(" style=\"overflow-y:auto;height:30000%\"", style_of("b"));
    /* ten digits: the reading stops past the cap (no int overflow) */
    prep("#b { overflow: auto; height: 3000000000% } " RD);
    TEST_ASSERT_EQUAL_STRING("%3000000000", node("b")->htok);
    TEST_ASSERT_EQUAL_STRING(" style=\"overflow-y:auto;height:30000%\"", style_of("b"));
    prep("#b { overflow: auto; height: 12345678901% } " RD);
    TEST_ASSERT_EQUAL_STRING("", node("b")->htok);
    TEST_ASSERT_EQUAL_STRING("", style_of("b"));
    prep("#b { overflow: auto; height: .5% } " RD);
    TEST_ASSERT_EQUAL_STRING(" style=\"overflow-y:auto;height:0%\"", style_of("b"));
    /* math: resolved without a percentage; a negative one is 0 */
    prep("#b { overflow: auto; height: calc(2em + 10px) } " RD);
    TEST_ASSERT_EQUAL_STRING(" style=\"overflow-y:auto;height:42px\"", style_of("b"));
    prep("#b { overflow: auto; height: calc(10px - 50px) } " RD);
    TEST_ASSERT_EQUAL_STRING(" style=\"overflow-y:auto;height:0px\"", style_of("b"));
    /* Lot 109 (converted): a percentage goes to the layout */
    prep("#b { overflow: auto; height: calc(50% + 10px) } " RD);
    TEST_ASSERT_EQUAL_STRING(" style=\"overflow-y:auto;height:calc(50% + 10px)\"", style_of("b"));
    prep("#b { overflow: auto; height: fit-content(10px) } " RD);
    TEST_ASSERT_EQUAL_STRING("", style_of("b"));
    prep("#b { overflow: auto; height: calc(10px + 1) } " RD);
    TEST_ASSERT_EQUAL_STRING("", node("b")->htok);
    TEST_ASSERT_EQUAL_STRING("", style_of("b"));
    /* a leading + is dropped from a font length */
    prep("#b { overflow: auto; height: +4em } " RD);
    TEST_ASSERT_EQUAL_STRING("~4em", node("b")->htok);
    TEST_ASSERT_EQUAL_STRING(" style=\"overflow-y:auto;height:64px\"", style_of("b"));
    /* a kept unit the reader does not resolve wins the cascade and
     * passes nothing; an invalid value is dropped */
    prep("#b { overflow: auto; height: 100px } #b { height: 10cap } " RD);
    TEST_ASSERT_EQUAL(0, node("b")->hpx);
    TEST_ASSERT_EQUAL_STRING("", node("b")->htok);
    TEST_ASSERT_EQUAL_STRING("", style_of("b"));
    /* Lot 109 (converted from 10ex above): ex resolves (5/16 em) */
    prep("#b { overflow: auto; height: 100px } #b { height: 10ex } " RD);
    TEST_ASSERT_EQUAL_STRING("~10ex", node("b")->htok);
    TEST_ASSERT_EQUAL_STRING(" style=\"overflow-y:auto;height:50px\"", style_of("b"));
    prep("#b { overflow: auto; height: 100px } #b { height: -1em; height: 1.2.3em; height: 1em% } " RD);
    TEST_ASSERT_EQUAL(101, node("b")->hpx);
    TEST_ASSERT_EQUAL_STRING(" style=\"overflow-y:auto;height:100px\"", style_of("b"));
    /* an absolute height after a relative one clears the token */
    prep("#b { overflow: auto; height: 3em } #b { height: 40px } " RD);
    TEST_ASSERT_EQUAL_STRING("", node("b")->htok);
    TEST_ASSERT_EQUAL_STRING(" style=\"overflow-y:auto;height:40px\"", style_of("b"));
    /* max-height: none is kept (nothing passed), auto is not valid */
    prep("#b { overflow: auto; max-height: 50px } #b { max-height: none } " RD);
    TEST_ASSERT_EQUAL(0, node("b")->mpx);
    TEST_ASSERT_EQUAL_STRING("", style_of("b"));
    prep("#b { overflow: auto; max-height: 50px } #b { max-height: auto } " RD);
    TEST_ASSERT_EQUAL(51, node("b")->mpx);
    TEST_ASSERT_EQUAL_STRING(" style=\"overflow-y:auto;max-height:50px\"", style_of("b"));
    prep("#b { overflow: auto; height: none } " RD);
    TEST_ASSERT_EQUAL_STRING("", style_of("b"));
    prep("#b { overflow: scroll; max-height: 25% !important } #b { max-height: 10px } " RD);
    TEST_ASSERT_EQUAL_STRING(" style=\"overflow-y:scroll;max-height:25%\"", style_of("b"));
    /* max-height alone makes a passed box scroller; hidden does not */
    prep("#b { overflow: hidden; max-height: 5em } " RD);
    TEST_ASSERT_EQUAL_STRING("", style_of("b"));
    /* the long value pool full (32 entries): a long length is not kept */
    {
        static char css[3072];
        int k, l = 0;
        for (k = 0; k < 32; k++) l += snprintf(css + l, sizeof(css) - (size_t)l, "#q { height: 1.%07dem } ", k + 1);
        snprintf(css + l, sizeof(css) - (size_t)l, "#b { overflow: auto; height: 2.0000001em } " RD);
        prep(css);
        TEST_ASSERT_TRUE(g_d.dropped_longs > 0);
        TEST_ASSERT_EQUAL_STRING("", node("b")->htok);
        TEST_ASSERT_EQUAL_STRING("", style_of("b"));
        /* with room left it is kept */
        l = 0;
        for (k = 0; k < 20; k++) l += snprintf(css + l, sizeof(css) - (size_t)l, "#q { height: 1.%07dem } ", k + 1);
        snprintf(css + l, sizeof(css) - (size_t)l, "#b { overflow: auto; height: 2.0000001em } " RD);
        prep(css);
        TEST_ASSERT_EQUAL('=', node("b")->htok[0]);
        TEST_ASSERT_EQUAL_STRING(" style=\"overflow-y:auto;height:32px\"", style_of("b"));
    }
}

/* scroll-padding by logical side: block-start, block-end, inline-start,
 * inline-end */
static void test_l107_padding_parse(void) {
    const wf_site_node_t *n;
    prep("#o { scroll-padding: 10px 2em }");
    n = node("o");
    TEST_ASSERT_EQUAL_STRING("#100000", n->sptok[0]);
    TEST_ASSERT_EQUAL_STRING("#100000", n->sptok[1]);
    TEST_ASSERT_EQUAL_STRING("~2em", n->sptok[2]);
    TEST_ASSERT_EQUAL_STRING("~2em", n->sptok[3]);
    /* three values: top, left and right, bottom */
    prep("#o { scroll-padding: 1px 2px 3px }");
    n = node("o");
    TEST_ASSERT_EQUAL_STRING("#10000", n->sptok[0]);
    TEST_ASSERT_EQUAL_STRING("#30000", n->sptok[1]);
    TEST_ASSERT_EQUAL_STRING("#20000", n->sptok[2]);
    TEST_ASSERT_EQUAL_STRING("#20000", n->sptok[3]);
    /* four: top, right, bottom, left */
    prep("#o { scroll-padding: 1px 2px 3px 4.5px }");
    n = node("o");
    TEST_ASSERT_EQUAL_STRING("#10000", n->sptok[0]);
    TEST_ASSERT_EQUAL_STRING("#30000", n->sptok[1]);
    TEST_ASSERT_EQUAL_STRING("#45000", n->sptok[2]);
    TEST_ASSERT_EQUAL_STRING("#20000", n->sptok[3]);
    /* five values, a negative one, none, fit-content: dropped */
    prep("#o { scroll-padding: 7px } #o { scroll-padding: 1px 2px 3px 4px 5px; scroll-padding: -1px; "
         "scroll-padding: none; scroll-padding: fit-content(1px); scroll-padding: min-content }");
    TEST_ASSERT_EQUAL_STRING("#70000", node("o")->sptok[0]);
    TEST_ASSERT_EQUAL_STRING("#70000", node("o")->sptok[3]);
    /* the block and inline shorthands; a math with % is auto (Lot 109,
     * converted: kept as "@N", which an inset reads as auto) */
    prep("#o { scroll-padding-inline: 5% auto; scroll-padding-block: calc(10% + 1px) 0 }");
    n = node("o");
    TEST_ASSERT_EQUAL('@', n->sptok[0][0]);
    TEST_ASSERT_EQUAL_STRING("#0", n->sptok[1]);
    TEST_ASSERT_EQUAL_STRING("%5", n->sptok[2]);
    TEST_ASSERT_EQUAL_STRING("", n->sptok[3]);
    prep("#o { scroll-padding-block: 1px 2px 3px; scroll-padding-inline: 4vh }");
    TEST_ASSERT_EQUAL_STRING("", node("o")->sptok[0]);
    TEST_ASSERT_EQUAL_STRING("~4vh", node("o")->sptok[2]);
    TEST_ASSERT_EQUAL_STRING("~4vh", node("o")->sptok[3]);
    /* the longhands, logical and physical (horizontal-tb ltr) */
    prep("#o { scroll-padding-top: 1px; scroll-padding-bottom: 2px; scroll-padding-left: 3px; scroll-padding-right: 4px }");
    n = node("o");
    TEST_ASSERT_EQUAL_STRING("#10000", n->sptok[0]);
    TEST_ASSERT_EQUAL_STRING("#20000", n->sptok[1]);
    TEST_ASSERT_EQUAL_STRING("#30000", n->sptok[2]);
    TEST_ASSERT_EQUAL_STRING("#40000", n->sptok[3]);
    prep("#o { scroll-padding: 9px; scroll-padding-block-end: 1em; scroll-padding-inline-start: 2% }");
    n = node("o");
    TEST_ASSERT_EQUAL_STRING("#90000", n->sptok[0]);
    TEST_ASSERT_EQUAL_STRING("~1em", n->sptok[1]);
    TEST_ASSERT_EQUAL_STRING("%2", n->sptok[2]);
    TEST_ASSERT_EQUAL_STRING("#90000", n->sptok[3]);
    /* initial and unset are auto; !important wins over a later one */
    prep("#o { scroll-padding: 9px } #o { scroll-padding-block: initial; scroll-padding-inline-end: unset }");
    n = node("o");
    TEST_ASSERT_EQUAL_STRING("", n->sptok[0]);
    TEST_ASSERT_EQUAL_STRING("", n->sptok[1]);
    TEST_ASSERT_EQUAL_STRING("#90000", n->sptok[2]);
    TEST_ASSERT_EQUAL_STRING("", n->sptok[3]);
    prep("#o { scroll-padding-top: 5px !important } #o { scroll-padding-top: 6px }");
    TEST_ASSERT_EQUAL_STRING("#50000", node("o")->sptok[0]);
    /* an empty value and auto */
    prep("#o { scroll-padding: 7px } #o { scroll-padding: ; scroll-padding-block: auto none }");
    TEST_ASSERT_EQUAL_STRING("#70000", node("o")->sptok[0]);
    prep("#o { scroll-padding: 7px } #o { scroll-padding-block-start: auto }");
    TEST_ASSERT_EQUAL_STRING("", node("o")->sptok[0]);
    TEST_ASSERT_EQUAL_STRING("#70000", node("o")->sptok[1]);
    /* a long font length goes to the pool */
    prep("#o { scroll-padding-top: 1234.5678em }");
    TEST_ASSERT_EQUAL('=', node("o")->sptok[0][0]);
    /* a math function in parentheses with spaces is one value */
    prep("#o { scroll-padding: calc(1em + 2px) max(1px, 2px) }");
    n = node("o");
    TEST_ASSERT_EQUAL('=', n->sptok[0][0]);
    TEST_ASSERT_EQUAL('=', n->sptok[2][0]);
}

static void geo_root(int rgy) {
    memset(&g_g, 0, sizeof(g_g));
    g_g.vw = 640; g_g.vh = 480; g_g.rg_y = rgy;
}

/* #i (element 5) on the root: b_off 1000, b_size 100, view 480, range
 * 2000, at pm 5000 (position 1000) */
static const char *root_view(const char *css) {
    wf_geom_box_t *b;
    geo_root(2000);
    b = &g_g.box[5];
    b->ok = 1; b->root = 1; b->bvert = 1; b->b_off = 1000; b->b_size = 100; b->i_off = 0; b->i_size = 300;
    b->vp_b = 480; b->vp_i = 640; b->rg_b = 2000;
    wf_site_set_geom(&g_g);
    wf_site_set_scroll(5000);
    prep(css);
    wf_site_set_scroll(0);
    wf_site_set_geom(0);
    return style_of("i");
}

/* the auto insets from the root element's scroll-padding */
static void test_l107_root_insets(void) {
    /* no padding: cover [520, 1100], (1000 - 520) / 580 = 828 ms */
    TEST_ASSERT_EQUAL_STRING(" style=\"color:#d3d3d3\"",
                             root_view("#i { animation: k 1s linear; animation-timeline: view() }"));
    /* 10% of 480 = 48 start, 2em at 10 px = 20 end: [540, 1052], 898 ms */
    TEST_ASSERT_EQUAL_STRING(" style=\"color:#e5e5e5\"",
                             root_view("html { font-size: 10px; scroll-padding-block: 10% 2em } "
                                       "#i { animation: k 1s linear; animation-timeline: view() }"));
    /* an explicit inset wins over the padding (start 0px, end auto: 20):
     * [540, 1100], 821 ms */
    TEST_ASSERT_EQUAL_STRING(" style=\"color:#d1d1d1\"",
                             root_view("html { font-size: 10px; scroll-padding-block: 10% 2em } "
                                       "#i { animation: k 1s linear; animation-timeline: view(block 0px auto) }"));
    /* 100px absolute start, 12.5% end (60): [580, 1000]: 1000 ms, the end */
    TEST_ASSERT_EQUAL_STRING(" style=\"color:#ffffff\"",
                             root_view("html { scroll-padding: 100px 0 12.5% } "
                                       "#i { animation: k 1s linear both; animation-timeline: view() }"));
    /* the inline sides do not count along the block axis */
    TEST_ASSERT_EQUAL_STRING(" style=\"color:#d3d3d3\"",
                             root_view("html { scroll-padding-inline: 100px } "
                                       "#i { animation: k 1s linear; animation-timeline: view() }"));
    /* four decimals of a percentage are read: 10.00009% is 10.0000%
     * (48 px), the same as 10% */
    TEST_ASSERT_EQUAL_STRING(" style=\"color:#e5e5e5\"",
                             root_view("html { font-size: 10px; scroll-padding-block: 10.00009% 2em } "
                                       "#i { animation: k 1s linear; animation-timeline: view() }"));
    /* past 10000 px a font length does not resolve: 0 (2000em at 10 px) */
    TEST_ASSERT_EQUAL_STRING(" style=\"color:#d3d3d3\"",
                             root_view("html { font-size: 10px; scroll-padding-block: 2000em } "
                                       "#i { animation: k 1s linear; animation-timeline: view() }"));
    /* a negative math result is 0 */
    TEST_ASSERT_EQUAL_STRING(" style=\"color:#d3d3d3\"",
                             root_view("html { scroll-padding-block: calc(10px - 1em) } "
                                       "#i { animation: k 1s linear; animation-timeline: view() }"));
    /* the padding of body or #o is not the root's */
    TEST_ASSERT_EQUAL_STRING(" style=\"color:#d3d3d3\"",
                             root_view("body, #o { scroll-padding: 100px } "
                                       "#i { animation: k 1s linear; animation-timeline: view() }"));
}

/* inside a box scroller #o (element 3): its own padding, inline axis */
static void test_l107_box_insets(void) {
    wf_geom_box_t *b;
    geo_root(0);
    b = &g_g.box[5];
    b->ok = 1; b->root = 0; b->bvert = 1; b->b_off = 100; b->b_size = 20; b->i_off = 200; b->i_size = 100;
    b->vp_b = 200; b->vp_i = 400; b->rg_b = 0; b->rg_i = 600; b->sm = 4; b->sc_i = 300;
    wf_site_set_geom(&g_g);
    /* inline: cover [200 - 400 + 40, 300 - 10] = [-160, 290]; (300 + 160) / 450 = 1022: past */
    prep("#o { overflow: auto; scroll-padding-inline: 10px 10%; scroll-padding-block: 99px } "
         "#i { animation: k 1s linear; animation-timeline: view(inline) }");
    TEST_ASSERT_EQUAL_STRING("", style_of("i"));
    /* both: the end colour */
    prep("#o { overflow: auto; scroll-padding-inline: 10px 10%; scroll-padding-block: 99px } "
         "#i { animation: k 1s linear both; animation-timeline: view(inline) }");
    TEST_ASSERT_EQUAL_STRING(" style=\"color:#ffffff\"", style_of("i"));
    /* inline-start 1.5em at #o's 20 px = 30, end 25% of 400 = 100:
     * [200 - 400 + 100, 300 - 30] = [-100, 270]; 400 / 370 past; at
     * sc_i 100: 200 / 370 = 540 ms */
    b->sc_i = 100;
    prep("#o { overflow: auto; font-size: 20px; scroll-padding-inline: 1.5em 25% } "
         "#i { animation: k 1s linear; animation-timeline: view(inline) }");
    TEST_ASSERT_EQUAL_STRING(" style=\"color:#8a8a8a\"", style_of("i"));
    wf_site_set_geom(0);
}

/* the body's start offset */
static void test_l107_body_start(void) {
    const char *at;
    TEST_ASSERT_EQUAL(-1, wf_site_body_start(0));
    memset(&g_d, 0, sizeof(g_d));
    TEST_ASSERT_EQUAL(-1, wf_site_body_start(&g_d));
    prep("body { color: #102030 }");
    at = strstr(g_html, "<body");
    TEST_ASSERT_EQUAL((int)(at - g_html), wf_site_body_start(&g_d));
    /* a site that is not ready has no body */
    g_d.ready = 0;
    TEST_ASSERT_EQUAL(-1, wf_site_body_start(&g_d));
    g_d.ready = 1;
    wf_site_style_attr(&g_d, wf_site_body_start(&g_d), g_o, (int)sizeof(g_o));
    TEST_ASSERT_EQUAL_STRING(" style=\"color:#102030\"", g_o);
    /* a body inside a div is not the body */
    snprintf(g_html, sizeof(g_html), "<html><head><style>body { color: red }</style></head><div><body>x</body></div></html>");
    TEST_ASSERT_EQUAL(0, wf_site_prepare_ext(&g_d, g_html, (int)strlen(g_html), 640, 480, 0, 0));
    TEST_ASSERT_EQUAL(-1, wf_site_body_start(&g_d));
}

int main(void) {
    unity_init();
    RUN_TEST(test_l107_legacy_order);
    RUN_TEST(test_l107_heights);
    RUN_TEST(test_l107_padding_parse);
    RUN_TEST(test_l107_root_insets);
    RUN_TEST(test_l107_box_insets);
    RUN_TEST(test_l107_body_start);
    unity_print_results();
    unity_cleanup();
    return unity_stats.tests_failed == 0 ? 0 : 1;
}
