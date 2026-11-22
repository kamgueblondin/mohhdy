/* Lot 116 (US-031): the reader's side of the Lot 116 engine changes: the
 * line-height it passes (a number as the number, px with hundredths, up
 * to 400 px), the engine's math bounds mirrored (32 nodes, 8 functions
 * deep, 8 arguments, a 128-byte expression), var() invalid at
 * computed-value time unset for the box lengths, the overflow, the
 * timelines, scroll-padding and the animation lists (read per element
 * when it names an element's custom property), and the body's
 * percentage width against html's margin and padding, cap, ic and the
 * root's rex, rch, rcap and ric of the engine's font (wf_sitecss,
 * wf_cssmath). Hand cases next to the generated test_lot116_*.c;
 * expected values follow docs/lot116_plan.md. */
#include "../../framework/unity.h"
#include <string.h>
#include <stdio.h>
#include "wf_sitecss.h"
#include "wf_cssmath.h"

static char g_o[1024];
static wf_site_t g_d;
static char g_html[4096];
static char g_rep[16384];

#define KF "@keyframes k { from { color: #000000 } to { color: #ffffff } } "
#define RD "#i { animation: k 1s linear; animation-timeline: scroll(nearest) } "

/* html 0, head 1, body 2, #o 3, #b 4, #i 5 */
static void prep(const char *css) {
    snprintf(g_html, sizeof(g_html), "<html><head><style>" KF "%s</style></head><body><div id=o><div id=b>"
             "<p id=i>i</p><p>a</p></div></div></body></html>", css);
    TEST_ASSERT_EQUAL(0, wf_site_prepare_ext(&g_d, g_html, (int)strlen(g_html), 640, 480, 0, 0));
}

static const char *style_of(const char *id) {
    int i;
    g_o[0] = 0;
    for (i = 0; i < g_d.nnodes; i++)
        if (!strcmp(g_d.node[i].id, id) || (!g_d.node[i].id[0] && !strcmp(g_d.node[i].tag, id)))
            wf_site_style_attr(&g_d, g_d.node[i].start, g_o, (int)sizeof(g_o));
    return g_o;
}

/* the report's node line of this id ("" when none) */
static const char *line_of(const char *id) {
    char key[40];
    const char *p, *e;
    snprintf(key, sizeof(key), " #%s ", id);
    g_o[0] = 0;
    if (wf_site_report(&g_d, g_rep, (int)sizeof(g_rep)) <= 0) return g_o;
    for (p = g_rep; (p = strstr(p, "node ")) != 0; p = e) {
        e = strchr(p, '\n');
        if (!e) e = p + strlen(p);
        if ((int)(e - p) < (int)sizeof(g_o) && strstr(p, key) && strstr(p, key) < e) {
            memcpy(g_o, p, (size_t)(e - p));
            g_o[e - p] = 0;
            return g_o;
        }
    }
    return g_o;
}

/* "r px out" of wf_css_math_box with a WF_SITE_BOXEX buffer */
static const char *mb(const char *s) {
    static char r[WF_SITE_BOXEX + 32];
    char out[WF_SITE_BOXEX];
    int px = -7, k = wf_css_math_box(s, 640, 480, 16000, 16000, &px, out, (int)sizeof(out));
    snprintf(r, sizeof(r), "%d %d %s", k, px, out);
    return r;
}

/* the line-height the reader passes: the number, px with hundredths */
static void test_l116_lh_pass(void) {
    prep("#b { line-height: 1.25 } #i { line-height: 15.5px }");
    TEST_ASSERT_EQUAL_STRING(" style=\"line-height:1.25\"", style_of("b"));
    TEST_ASSERT_EQUAL_STRING(" style=\"line-height:15.5px\"", style_of("i"));
    /* 400 px kept, past it 400 px; a number past 40 as px (the engine's
     * number bound) */
    prep("#b { line-height: 450px } #i { line-height: 41 }");
    TEST_ASSERT_EQUAL_STRING(" style=\"line-height:400px\"", style_of("b"));
    TEST_ASSERT_EQUAL_STRING(" style=\"line-height:400px\"", style_of("i"));
    prep("#b { line-height: 9px } #i { line-height: 0.333 }");
    TEST_ASSERT_EQUAL_STRING(" style=\"line-height:9px\"", style_of("b"));
    TEST_ASSERT_EQUAL_STRING(" style=\"line-height:0.33\"", style_of("i"));
    /* Lot 117: a number with a font size in hundredths is passed as the
     * number (the engine keeps the hundredths; Lot 116: 21px); with
     * thousandths (smaller: 13.333 px) as px of its font */
    prep("#b { font-size: 10.5px; line-height: 2 }");
    TEST_ASSERT_EQUAL_STRING(" style=\"font-size:10.5px;line-height:2\"", style_of("b"));
    prep("#b { font-size: smaller; line-height: 2 }");
    TEST_ASSERT_EQUAL_STRING(" style=\"font-size:smaller;line-height:26.67px\"", style_of("b"));
    prep("#o { font-size: 10.5px } #b { font-size: 1.1em; line-height: 2 }");
    TEST_ASSERT_EQUAL_STRING(" style=\"font-size:1.1em;line-height:2\"", style_of("b"));
    /* the report: a number as "lh num", px with hundredths */
    prep("#b { line-height: 1.5 } #i { line-height: 12.25px }");
    TEST_ASSERT_NOT_NULL(strstr(line_of("b"), " lh num 1.5"));
    TEST_ASSERT_NOT_NULL(strstr(line_of("i"), " lh 12.25"));
}

/* the math bounds mirrored: 8 arguments, 8 deep, 32 nodes, 127 bytes */
static void test_l116_math_mirror(void) {
    TEST_ASSERT_EQUAL(32, WF_CSSMATH_NODES);
    TEST_ASSERT_EQUAL(8, WF_CSSMATH_FN_DEPTH);
    TEST_ASSERT_EQUAL(8, WF_CSSMATH_ARGS);
    TEST_ASSERT_EQUAL(128, WF_SITE_BOXEX);
    TEST_ASSERT_EQUAL_STRING("2 0 min(1%, calc(2% - 1px), calc(3% - 2px), calc(4% - 3px), calc(5% - 4px), calc(6% - 5px))",
                             mb("min(1%, calc(2% - 1px), calc(3% - 2px), calc(4% - 3px), calc(5% - 4px), calc(6% - 5px))"));
    TEST_ASSERT_EQUAL_STRING("2 0 max(min(10%, 50px), min(20%, 40px), min(30%, 35px), min(5%, 60px), min(1%, 70px))",
                             mb("max(min(10%, 50px), min(20%, 40px), min(30%, 35px), min(5%, 60px), min(1%, 70px))"));
    /* eight functions deep and their leaves parse; nine do not */
    TEST_ASSERT_EQUAL_STRING("2 0 min(9%, max(1px, min(8%, max(2px, min(7%, max(3px, min(6%, max(4px, 5%))))))))",
                             mb("min(9%, max(1px, min(8%, max(2px, min(7%, max(3px, min(6%, max(4px, 5%))))))))"));
    TEST_ASSERT_EQUAL_STRING("-1 0 ", mb("min(9%, max(1px, min(8%, max(2px, min(7%, max(3px, min(6%, max(4px, "
                                         "min(5%, 9px))))))))))"));
    /* the parser's own nesting: a plain length 9 factors deep, not 10 */
    {
        char out[16];
        /* calc() and 7 parentheses around the leaf: 9 */
        TEST_ASSERT_EQUAL(1, wf_css_math("calc((((((((1px))))))))", 640, 480, out, (int)sizeof(out)));
        TEST_ASSERT_EQUAL_STRING("1px", out);
        TEST_ASSERT_EQUAL(-1, wf_css_math("calc(((((((((1px)))))))))", 640, 480, out, (int)sizeof(out)));
    }
}

/* var() invalid at computed-value time: unset for the properties with a
 * clear initial value outside the kinds */
static void test_l116_unset_props(void) {
    /* height auto: no box scroller left */
    prep("#o { overflow: auto; height: 50px; height: var(--nope) } " RD);
    TEST_ASSERT_EQUAL_STRING("", style_of("o"));
    /* max-height none, the height stays */
    prep("#o { overflow: auto; height: 50px; max-height: 20px; max-height: var(--nope) } " RD);
    TEST_ASSERT_EQUAL_STRING(" style=\"overflow-y:auto;height:50px\"", style_of("o"));
    /* overflow visible: no box scroller */
    prep("#o { overflow: auto; height: 50px; overflow: var(--nope) } " RD);
    TEST_ASSERT_EQUAL_STRING("", style_of("o"));
    /* a value the property does not take is unset too */
    prep(":root { --c: red } #o { overflow: auto; height: 50px; height: var(--c) } " RD);
    TEST_ASSERT_EQUAL_STRING("", style_of("o"));
    prep(":root { --c: 12px } #o { overflow: auto; height: 50px; overflow: var(--c) } " RD);
    TEST_ASSERT_EQUAL_STRING("", style_of("o"));
    /* overflow-y unset is visible, which computes to auto next to
     * overflow-x auto (CSS Overflow 3 3.1): still a box scroller */
    prep(":root { --c: 12px } #o { overflow: auto; height: 50px; overflow-y: var(--c) } " RD);
    TEST_ASSERT_EQUAL_STRING(" style=\"overflow-y:auto;height:50px\"", style_of("o"));
    TEST_ASSERT_EQUAL(1, g_d.node[3].ov[1]);
    /* width (along x) */
    prep("#o { overflow: auto; height: 50px; width: 200px; width: var(--nope) } " RD);
    TEST_ASSERT_EQUAL_STRING(" style=\"overflow-y:auto;height:50px\"", style_of("o"));
    /* the animation lists: animation-timeline auto, the document's
     * timeline (no scroll timeline left for #i) */
    prep("#o { overflow: auto; height: 50px } " RD "#i { animation-timeline: var(--nope) }");
    TEST_ASSERT_EQUAL_STRING("", style_of("o"));
    /* a property the reader does not keep: still left out */
    prep("#o { overflow: auto; height: 50px; float: var(--nope) } " RD);
    TEST_ASSERT_EQUAL_STRING(" style=\"overflow-y:auto;height:50px\"", style_of("o"));
}

/* an element's own custom properties are read per element */
static void test_l116_unset_per_element(void) {
    prep("#o { --h: 40px; overflow: auto; height: 50px; height: var(--h) } " RD);
    TEST_ASSERT_EQUAL_STRING(" style=\"overflow-y:auto;height:40px\"", style_of("o"));
    prep("#o { --h: red; overflow: auto; height: 50px; height: var(--h) } " RD);
    TEST_ASSERT_EQUAL_STRING("", style_of("o"));
    /* inherited from body */
    prep("body { --h: 30%; --ov: scroll } #o { overflow: auto; height: 50px; height: var(--h); overflow-y: var(--ov) } " RD);
    TEST_ASSERT_EQUAL_STRING(" style=\"overflow-y:scroll;height:30%\"", style_of("o"));
    /* two elements, two values */
    snprintf(g_html, sizeof(g_html), "<html><head><style>" KF "#o { --h: 40px } #q { --h: nosuch } #o, #q { overflow: auto; "
             "height: 50px; height: var(--h) } #i, #j { animation: k 1s linear; animation-timeline: scroll(nearest) }</style>"
             "</head><body><div id=o><p id=i>i</p></div><div id=q><p id=j>j</p></div></body></html>");
    TEST_ASSERT_EQUAL(0, wf_site_prepare_ext(&g_d, g_html, (int)strlen(g_html), 640, 480, 0, 0));
    TEST_ASSERT_EQUAL_STRING(" style=\"overflow-y:auto;height:40px\"", style_of("o"));
    TEST_ASSERT_EQUAL_STRING("", style_of("q"));
    /* a long property name is kept for the per element read (dprop 28) */
    prep("#o { overflow: auto; height: 50px; --o: hidden; overflow-y: auto; --p: 7px; "
         "scroll-padding-inline-start: var(--p) } " RD);
    TEST_ASSERT_EQUAL_STRING(" style=\"overflow-y:auto;height:50px\"", style_of("o"));
}

/* a value from var() the property does not take is unset (the initial
 * value: no list, auto), not the earlier declaration: timelines,
 * scroll-padding, animation-range and the animation and transition
 * lists, read in the element's fields (lpool index + 1, 0 initial; the
 * scroll-padding token, "" auto); each next to the sheet without it */
static void test_l116_unset_taken(void) {
    int k;
    prep("#o { --c: 12px; scroll-timeline: --t }");
    TEST_ASSERT_TRUE(g_d.node[3].tl[0] && g_d.node[3].tl[1]);
    prep("#o { --c: 12px; scroll-timeline: --t; scroll-timeline: var(--c) }");
    for (k = 0; k < WF_SITE_TLS; k++) TEST_ASSERT_EQUAL(0, g_d.node[3].tl[k]);
    prep("#o { --c: red; scroll-padding-top: 10px }");
    TEST_ASSERT_EQUAL_STRING("#100000", g_d.node[3].sptok[0]);
    prep("#o { --c: red; scroll-padding-top: 10px; scroll-padding-top: var(--c) }");
    TEST_ASSERT_EQUAL_STRING("", g_d.node[3].sptok[0]);
    prep("#i { --c: red; animation-range: 10% 90% }");
    TEST_ASSERT_TRUE(g_d.node[5].al[10] && g_d.node[5].al[11]);
    prep("#i { --c: red; animation-range: 10% 90%; animation-range: var(--c) }");
    TEST_ASSERT_EQUAL(0, g_d.node[5].al[10]);
    TEST_ASSERT_EQUAL(0, g_d.node[5].al[11]);
    prep("#i { --c: 12px; animation: k 1s linear }");
    TEST_ASSERT_TRUE(g_d.node[5].al[0] != 0);
    prep("#i { --c: 12px; animation: k 1s linear; animation: var(--c) }");
    for (k = 0; k < WF_SITE_ALONGS; k++) TEST_ASSERT_EQUAL(0, g_d.node[5].al[k]);
    prep("#i { --c: 12px; transition: color 1s }");
    TEST_ASSERT_TRUE(g_d.node[5].al[12] != 0);
    prep("#i { --c: 12px; transition: color 1s; transition: var(--c) }");
    for (k = 0; k < WF_SITE_ALONGS; k++) TEST_ASSERT_EQUAL(0, g_d.node[5].al[k]);
    /* the longest name read per element (dprop 28):
     * scroll-padding-inline-start from the element's --p */
    prep("#o { --p: 7px; scroll-padding-inline-start: var(--p) }");
    TEST_ASSERT_EQUAL_STRING("#70000", g_d.node[3].sptok[2]);
}

/* the passed line height: a number up to 40 as the number, hundredths
 * rounded half up (1.255 is 1.26, 15.505px is 15.51px), a math function
 * of numbers a number (the box math's 0px of calc(0 * 1) too); cap of a 17.5px font: q 5 of the rounded 18 px (os_ui_px(7, 5) = 9) */
static void test_l116_lh_round(void) {
    prep("#b { line-height: 5 } #i { line-height: 40 }");
    TEST_ASSERT_EQUAL_STRING(" style=\"line-height:5\"", style_of("b"));
    TEST_ASSERT_EQUAL_STRING(" style=\"line-height:40\"", style_of("i"));
    prep("#b { line-height: 1.255 } #i { line-height: 15.505px }");
    TEST_ASSERT_EQUAL_STRING(" style=\"line-height:1.26\"", style_of("b"));
    TEST_ASSERT_EQUAL_STRING(" style=\"line-height:15.51px\"", style_of("i"));
    prep("#b { line-height: calc(1 + 0.5) } #i { line-height: calc(2 * 0.75) }");
    TEST_ASSERT_EQUAL_STRING(" style=\"line-height:1.5\"", style_of("b"));
    TEST_ASSERT_EQUAL_STRING(" style=\"line-height:1.5\"", style_of("i"));
    /* calc(0 * 1), which the box math takes as 0px, is the number 0 */
    prep("#i { line-height: calc(0 * 1) }");
    TEST_ASSERT_NOT_NULL(strstr(line_of("i"), " lh num 0"));
    prep("#i { font-size: 17.5px; line-height: 2cap }");
    TEST_ASSERT_EQUAL_STRING(" style=\"font-size:17.5px;line-height:18px\"", style_of("i"));
}

/* html's margin and padding come off the body's percentage width */
static void test_l116_body_width(void) {
    prep("html { overflow: hidden; margin: 0 20px; padding: 0 10px } body { overflow: auto; width: 50%; height: 100px } " RD);
    TEST_ASSERT_NOT_NULL(strstr(style_of("body"), "width:290px"));
    /* only the left and right sides */
    prep("html { overflow: hidden; margin: 100px 0; padding: 50px 0 } body { overflow: auto; width: 50%; height: 100px } " RD);
    TEST_ASSERT_NOT_NULL(strstr(style_of("body"), "width:320px"));
    /* four values: right 30, left 10 */
    prep("html { overflow: hidden; margin: 0 30px 0 10px } body { overflow: auto; width: 100%; height: 100px } " RD);
    TEST_ASSERT_NOT_NULL(strstr(style_of("body"), "width:600px"));
    /* at least 0: 255 x 2 + 255 x 2 past 640 */
    prep("html { overflow: hidden; margin: 0 300px; padding: 0 300px } body { overflow: auto; width: 50%; height: 100px } " RD);
    TEST_ASSERT_NOT_NULL(strstr(style_of("body"), "width:0px"));
    /* max-width the same way */
    prep("html { overflow: hidden; padding: 0 20px } body { overflow: auto; width: 100%; max-width: 50%; height: 100px } " RD);
    TEST_ASSERT_NOT_NULL(strstr(style_of("body"), "width:600px;max-width:300px"));
    /* a margin the reader does not keep (a percentage) is dropped: the
     * viewport's width */
    prep("html { overflow: hidden; margin: 0 5% } body { overflow: auto; width: 50%; height: 100px } " RD);
    TEST_ASSERT_NOT_NULL(strstr(style_of("body"), "width:320px"));
    /* html's height auto: the body's percentage height is left (auto) */
    prep("html { overflow: hidden; margin: 10px } body { overflow: auto; height: 50% } " RD);
    TEST_ASSERT_EQUAL_STRING(" style=\"overflow-y:auto;height:50%\"", style_of("body"));
}

/* cap, ic and the root's rex, rch, rcap and ric (the engine's 8x8 font:
 * capitals 7 rows, x 5, cell 8 wide, no ideograph) */
static void test_l116_font_units(void) {
    wf_css_box_ctx_t c;
    int v = 0;
    char ex[16];
    prep("#o { overflow: auto; height: 3cap } " RD);
    TEST_ASSERT_EQUAL_STRING(" style=\"overflow-y:auto;height:21px\"", style_of("o"));     /* 16px: q 4 */
    prep("#o { font-size: 32px; overflow: auto; height: 2cap } " RD);
    TEST_ASSERT_EQUAL_STRING(" style=\"font-size:32px;overflow-y:auto;height:28px\"", style_of("o"));   /* q 8 */
    prep("#o { overflow: auto; height: 3ic } " RD);
    TEST_ASSERT_EQUAL_STRING(" style=\"overflow-y:auto;height:48px\"", style_of("o"));     /* 1em */
    /* the root's: html 20px (q 5): x 6, cell 10, capitals 9 */
    prep("html { font-size: 20px } #o { font-size: 32px; overflow: auto; height: calc(2rex + 1rch + 1rcap + 1ric) } " RD);
    TEST_ASSERT_EQUAL_STRING(" style=\"font-size:32px;overflow-y:auto;height:51px\"", style_of("o"));
    /* in line-height and font-size (of the parent's font) */
    prep("#o { line-height: 4cap } " RD);
    TEST_ASSERT_EQUAL_STRING(" style=\"line-height:28px\"", style_of("o"));
    prep("#o { font-size: 24px } #b { font-size: 2cap } " RD);
    TEST_ASSERT_EQUAL_STRING(" style=\"font-size:22px\"", style_of("b"));                 /* q 6: (42 + 2) / 4 = 11 */
    prep("#b { font-size: 1.5ric } " RD);
    TEST_ASSERT_EQUAL_STRING(" style=\"font-size:24px\"", style_of("b"));
    /* a unit that only starts like one is not one: a unit the reader
     * does not resolve is an auto height (as before), not 14px */
    prep("#o { overflow: auto; height: 30px; height: 2caps } " RD);
    TEST_ASSERT_EQUAL_STRING("", style_of("o"));
    /* the proportions when the font's metrics are not given */
    c.vw = 640; c.vh = 480; c.em_px = 16000; c.rem_px = 32000; c.lh_px = c.rlh_px = c.ex_px = c.ch_px = -1; c.basis = -1;
    TEST_ASSERT_EQUAL(1, wf_css_math_box_ctx("calc(2cap)", &c, &v, ex, (int)sizeof(ex)));
    TEST_ASSERT_EQUAL(14000, v);                              /* 7/16 em */
    TEST_ASSERT_EQUAL(1, wf_css_math_box_ctx("calc(1rcap)", &c, &v, ex, (int)sizeof(ex)));
    TEST_ASSERT_EQUAL(14000, v);                              /* 7/16 rem */
    TEST_ASSERT_EQUAL(1, wf_css_math_box_ctx("calc(1rex + 1rch)", &c, &v, ex, (int)sizeof(ex)));
    TEST_ASSERT_EQUAL(26000, v);                              /* 10 + 16 */
    TEST_ASSERT_EQUAL(1, wf_css_math_box_ctx("calc(0.5ric)", &c, &v, ex, (int)sizeof(ex)));
    TEST_ASSERT_EQUAL(16000, v);
    TEST_ASSERT_EQUAL(-1, wf_css_math_box_ctx("calc(1rca)", &c, &v, ex, (int)sizeof(ex)));
    TEST_ASSERT_EQUAL(-1, wf_css_math_box_ctx("calc(1icx)", &c, &v, ex, (int)sizeof(ex)));
}

/* bug found: the root's own font-size was read twice, the second time
 * against its new size and line height */
static void test_l116_root_font(void) {
    prep("html { font-size: 4rex } " RD);
    TEST_ASSERT_EQUAL_STRING(" style=\"font-size:20px\"", style_of("html"));     /* 4 x 5 (16px), not 4 x 6 */
    prep("html { font-size: 10rch } " RD);
    TEST_ASSERT_EQUAL_STRING(" style=\"font-size:80px\"", style_of("html"));     /* was dropped (10 x 32) */
    /* rlh in the root's font-size is the initial normal at 16px (10px) */
    prep("html { font-size: 2rlh; line-height: 30px } " RD);
    TEST_ASSERT_EQUAL_STRING(" style=\"font-size:20px;line-height:30px\"", style_of("html"));
    /* elsewhere the root's size: #o's 2rex of html's 20px (q 5) */
    prep("html { font-size: 4rex } #o { font-size: 2rex } " RD);
    TEST_ASSERT_EQUAL_STRING(" style=\"font-size:12px\"", style_of("o"));
}

int main(void) {
    unity_init();
    RUN_TEST(test_l116_lh_pass);
    RUN_TEST(test_l116_math_mirror);
    RUN_TEST(test_l116_unset_props);
    RUN_TEST(test_l116_unset_per_element);
    RUN_TEST(test_l116_unset_taken);
    RUN_TEST(test_l116_lh_round);
    RUN_TEST(test_l116_body_width);
    RUN_TEST(test_l116_font_units);
    RUN_TEST(test_l116_root_font);
    unity_print_results();
    unity_cleanup();
    return unity_stats.tests_failed == 0 ? 0 : 1;
}
