/* Lot 105 (US-031): box scrollers in the reader style engine and the GUI
 * scroll position. Hand cases next to the generated test_lot105_overflow.c
 * and test_lot105_sheet.c; expected values follow docs/lot105_plan.md
 * (CSS Overflow 3 scroll containers, CSS Scroll-driven Animations 1). The
 * page has fixed element indexes: html 0, head 1, body 2, #o 3, #b 4,
 * #i 5, #r 6, #q 7. */
#include "../../framework/unity.h"
#include <string.h>
#include <stdio.h>
#include "wf_sitecss.h"

static wf_site_t g_d;
static wf_geom_t g_g;
static char g_html[4096], g_o[1024];
#define KF "@keyframes k { from { color: #000000 } to { color: #ffffff } } "

static void prep(const char *css) {
    snprintf(g_html, sizeof(g_html), "<html><head><style>%s</style></head><body><div id=o><div id=b><p id=i>i</p>"
             "<p id=r>r</p></div></div><p id=q>q</p></body></html>", css);
    TEST_ASSERT_EQUAL(0, wf_site_prepare_ext(&g_d, g_html, (int)strlen(g_html), 640, 480, 0, 0));
    TEST_ASSERT_EQUAL(8, g_d.nnodes);
}

static const char *style_of(int k) {
    g_o[0] = 0;
    wf_site_style_attr(&g_d, g_d.node[k].start, g_o, (int)sizeof(g_o));
    return g_o;
}

static const char *anim(void) {
    g_o[0] = 0;
    wf_site_anim_text(&g_d, g_o, (int)sizeof(g_o));
    return g_o;
}

static wf_geom_box_t *box(int k, int root, int sm) {
    wf_geom_box_t *b = &g_g.box[k];
    memset(b, 0, sizeof(*b));
    b->ok = 1; b->root = (unsigned char)root; b->bvert = 1; b->sm = sm;
    g_g.n++;
    return b;
}

/* overflow, overflow-x, overflow-y: keywords, the shorthand's two values,
 * what is dropped */
static void test_l105_overflow_values(void) {
    prep("#o { overflow: Overlay } #b { overflow: hidden SCROLL } #i { overflow-x: clip; overflow-y: auto } "
         "#r { overflow: auto; overflow: inherit; overflow-y: revert; overflow-x: none } "
         "#q { overflow: scroll auto hidden }");
    TEST_ASSERT_EQUAL(WF_SITE_OV_AUTO, g_d.node[3].ov[0]);
    TEST_ASSERT_EQUAL(WF_SITE_OV_AUTO, g_d.node[3].ov[1]);
    TEST_ASSERT_EQUAL(WF_SITE_OV_HIDDEN, g_d.node[4].ov[0]);
    TEST_ASSERT_EQUAL(WF_SITE_OV_SCROLL, g_d.node[4].ov[1]);
    TEST_ASSERT_EQUAL(WF_SITE_OV_CLIP, g_d.node[5].ov[0]);
    TEST_ASSERT_EQUAL(WF_SITE_OV_AUTO, g_d.node[5].ov[1]);
    /* inherit, revert and an unknown keyword are dropped: auto stays */
    TEST_ASSERT_EQUAL(WF_SITE_OV_AUTO, g_d.node[6].ov[0]);
    TEST_ASSERT_EQUAL(WF_SITE_OV_AUTO, g_d.node[6].ov[1]);
    /* three keywords: dropped */
    TEST_ASSERT_EQUAL(0, g_d.node[7].ov[0]);
    TEST_ASSERT_EQUAL(0, g_d.node[7].ov[1]);
    /* initial and unset are visible, set (they win over an earlier value) */
    prep("#o { overflow: scroll; overflow: initial } #b { overflow-y: auto; overflow-y: unset } "
         "#i { overflow-x: scroll; overflow-x: initial }");
    TEST_ASSERT_EQUAL(WF_SITE_OV_VISIBLE, g_d.node[3].ov[0]);
    TEST_ASSERT_EQUAL(WF_SITE_OV_VISIBLE, g_d.node[3].ov[1]);
    TEST_ASSERT_EQUAL(0, g_d.node[4].ov[0]);
    TEST_ASSERT_EQUAL(WF_SITE_OV_VISIBLE, g_d.node[4].ov[1]);
    TEST_ASSERT_EQUAL(WF_SITE_OV_VISIBLE, g_d.node[5].ov[0]);
    TEST_ASSERT_EQUAL(0, g_d.node[5].ov[1]);
    /* a keyword longer than any (11 bytes kept) and one glued to a comma */
    prep("#o { overflow: scrollscrollscroll } #b { overflow: auto,scroll } #i { overflow: auto ; } "
         "#r { overflow-y: auto; overflow-y: ; overflow-x: scroll; overflow-x:   } "
         "#q { overflow-y: auto; overflow-y: hidden scroll; overflow-x: clip; overflow-x: auto auto }");
    TEST_ASSERT_EQUAL(0, g_d.node[3].ov[1]);
    TEST_ASSERT_EQUAL(0, g_d.node[4].ov[1]);
    TEST_ASSERT_EQUAL(WF_SITE_OV_AUTO, g_d.node[5].ov[1]);
    /* an empty value, two keywords on a longhand: dropped */
    TEST_ASSERT_EQUAL(WF_SITE_OV_SCROLL, g_d.node[6].ov[0]);
    TEST_ASSERT_EQUAL(WF_SITE_OV_AUTO, g_d.node[6].ov[1]);
    TEST_ASSERT_EQUAL(WF_SITE_OV_CLIP, g_d.node[7].ov[0]);
    TEST_ASSERT_EQUAL(WF_SITE_OV_AUTO, g_d.node[7].ov[1]);
    /* the cascade: importance, specificity, then order */
    prep("#o { overflow-y: scroll !important; overflow: hidden } div { overflow: auto } "
         "#b { overflow: hidden } div { overflow-y: clip !important }");
    TEST_ASSERT_EQUAL(WF_SITE_OV_HIDDEN, g_d.node[3].ov[0]);
    TEST_ASSERT_EQUAL(WF_SITE_OV_SCROLL, g_d.node[3].ov[1]);
    TEST_ASSERT_EQUAL(WF_SITE_OV_HIDDEN, g_d.node[4].ov[0]);
    TEST_ASSERT_EQUAL(WF_SITE_OV_CLIP, g_d.node[4].ov[1]);
}

/* height: the px + 1 kept for an absolute length, 0 for a value kept but
 * not passed on, dropped values */
static void test_l105_height_values(void) {
    prep("#o { height: 100px } #b { height: 1in } #i { height: 2.54cm } #r { height: 12pt } #q { height: 0 }");
    TEST_ASSERT_EQUAL(101, g_d.node[3].hpx);
    TEST_ASSERT_EQUAL(97, g_d.node[4].hpx);
    TEST_ASSERT_EQUAL(97, g_d.node[5].hpx);
    TEST_ASSERT_EQUAL(17, g_d.node[6].hpx);
    TEST_ASSERT_EQUAL(1, g_d.node[7].hpx);
    /* rounded down; the largest passed on; +, upper case */
    prep("#o { height: 12.99px } #b { height: 99999px } #i { height: 30000px } #r { height: +5px } #q { height: 10PX }");
    TEST_ASSERT_EQUAL(13, g_d.node[3].hpx);
    TEST_ASSERT_EQUAL(WF_SITE_HMAX + 1, g_d.node[4].hpx);
    TEST_ASSERT_EQUAL(30001, g_d.node[5].hpx);
    TEST_ASSERT_EQUAL(6, g_d.node[6].hpx);
    TEST_ASSERT_EQUAL(11, g_d.node[7].hpx);
    /* kept, not passed on: they win over the px value before them */
    prep("#o { height: 10px; height: auto } #b { height: 10px; height: 50% } #i { height: 10px; height: 2em } "
         "#r { height: 10px; height: calc(10px) } #q { height: 10px; height: 100001px }");
    TEST_ASSERT_EQUAL(0, g_d.node[3].hpx);
    TEST_ASSERT_EQUAL(0, g_d.node[4].hpx);
    TEST_ASSERT_EQUAL(0, g_d.node[5].hpx);
    TEST_ASSERT_EQUAL(0, g_d.node[6].hpx);
    TEST_ASSERT_EQUAL(0, g_d.node[7].hpx);
    prep("#o { height: 10px; height: min-content } #b { height: 10px; height: fit-content(5px) } "
         "#i { height: 10px; height: max(1px, 2px) } #r { height: 10px; height: initial } #q { height: 10px; height: unset }");
    TEST_ASSERT_EQUAL(0, g_d.node[3].hpx);
    TEST_ASSERT_EQUAL(0, g_d.node[4].hpx);
    TEST_ASSERT_EQUAL(0, g_d.node[5].hpx);
    TEST_ASSERT_EQUAL(0, g_d.node[6].hpx);
    TEST_ASSERT_EQUAL(0, g_d.node[7].hpx);
    /* dropped: the px value stays */
    prep("#o { height: 10px; height: -1px } #b { height: 10px; height: 5 } #i { height: 10px; height: -5% } "
         "#r { height: 10px; height: inherit } #q { height: 10px; height: 10px 20px }");
    TEST_ASSERT_EQUAL(11, g_d.node[3].hpx);
    TEST_ASSERT_EQUAL(11, g_d.node[4].hpx);
    TEST_ASSERT_EQUAL(11, g_d.node[5].hpx);
    TEST_ASSERT_EQUAL(11, g_d.node[6].hpx);
    TEST_ASSERT_EQUAL(11, g_d.node[7].hpx);
    prep("#o { height: 10px; height: px } #b { height: 10px; height: 1e2px } #i { height: 10px; height: calc(1px } "
         "#r { height: 10px; height: 5%x } #q { height: 10px; height: 1.5.5px }");
    TEST_ASSERT_EQUAL(11, g_d.node[3].hpx);
    TEST_ASSERT_EQUAL(11, g_d.node[4].hpx);
    TEST_ASSERT_EQUAL(11, g_d.node[5].hpx);
    TEST_ASSERT_EQUAL(11, g_d.node[6].hpx);
    TEST_ASSERT_EQUAL(11, g_d.node[7].hpx);
    /* one dot at most; an em value with a fraction is kept */
    prep("#o { height: 10px; height: 1.5em } #b { height: 10px; height: .5vh } #i { height: 10px; height: 1..5em } "
         "#r { height: 10px; height: +2em } #q { height: 10px; height: +-2em }");
    TEST_ASSERT_EQUAL(0, g_d.node[3].hpx);
    TEST_ASSERT_EQUAL(0, g_d.node[4].hpx);
    TEST_ASSERT_EQUAL(11, g_d.node[5].hpx);
    TEST_ASSERT_EQUAL(0, g_d.node[6].hpx);
    TEST_ASSERT_EQUAL(11, g_d.node[7].hpx);
}

/* scroll containers: the visible/clip rule, html and body, the nearest */
static void test_l105_containers(void) {
    prep("html { overflow: auto } body { overflow: scroll } #o { overflow-x: auto } #b { overflow-x: clip } "
         "#i { overflow: clip hidden } #q { overflow-x: visible; overflow-y: clip }");
    TEST_ASSERT_EQUAL(0, wf_site_box_scroller(&g_d, 0));
    TEST_ASSERT_EQUAL(0, wf_site_box_scroller(&g_d, 2));
    /* visible next to auto: auto */
    TEST_ASSERT_EQUAL(WF_SITE_OV_AUTO, wf_site_box_scroller(&g_d, 3));
    /* clip and visible: not a container */
    TEST_ASSERT_EQUAL(0, wf_site_box_scroller(&g_d, 4));
    /* clip next to hidden: hidden (y is hidden) */
    TEST_ASSERT_EQUAL(WF_SITE_OV_HIDDEN, wf_site_box_scroller(&g_d, 5));
    TEST_ASSERT_EQUAL(0, wf_site_box_scroller(&g_d, 7));
    TEST_ASSERT_EQUAL(3, wf_site_scroller_of(&g_d, 5));
    TEST_ASSERT_EQUAL(3, wf_site_scroller_of(&g_d, 4));
    TEST_ASSERT_EQUAL(-1, wf_site_scroller_of(&g_d, 3));
    TEST_ASSERT_EQUAL(-1, wf_site_scroller_of(&g_d, 7));
    TEST_ASSERT_EQUAL(-1, wf_site_scroller_of(&g_d, 0));
    /* x clip next to y scroll: x hidden; y stays scroll */
    prep("#o { overflow: clip scroll } #b { overflow: scroll clip } #i { overflow-y: hidden }");
    TEST_ASSERT_EQUAL(WF_SITE_OV_SCROLL, wf_site_box_scroller(&g_d, 3));
    TEST_ASSERT_EQUAL(WF_SITE_OV_HIDDEN, wf_site_box_scroller(&g_d, 4));
    TEST_ASSERT_EQUAL(4, wf_site_scroller_of(&g_d, 5));
    TEST_ASSERT_EQUAL(4, wf_site_scroller_of(&g_d, 6));
    /* bounds */
    TEST_ASSERT_EQUAL(0, wf_site_box_scroller(&g_d, -1));
    TEST_ASSERT_EQUAL(0, wf_site_box_scroller(&g_d, 8));
    TEST_ASSERT_EQUAL(0, wf_site_box_scroller(0, 3));
    TEST_ASSERT_EQUAL(-1, wf_site_scroller_of(&g_d, 8));
    TEST_ASSERT_EQUAL(-1, wf_site_scroller_of(&g_d, -1));
    TEST_ASSERT_EQUAL(-1, wf_site_scroller_of(0, 5));
    /* even when the slot past the last node looks like a box scroller, or
     * like an element in one (mutation witnesses) */
    g_d.node[8] = g_d.node[3];
    TEST_ASSERT_EQUAL(WF_SITE_OV_SCROLL, wf_site_box_scroller(&g_d, 3));
    TEST_ASSERT_EQUAL(0, wf_site_box_scroller(&g_d, 8));
    g_d.node[8] = g_d.node[5];
    TEST_ASSERT_EQUAL(-1, wf_site_scroller_of(&g_d, 8));
}

/* the style passed on: a box scroller a timeline reads, auto or scroll
 * along y, with a passed height */
static void test_l105_passed(void) {
    prep(KF "#o { overflow: auto; height: 120px; color: #102030 } #b { overflow-y: scroll; height: 50px } "
         "#i { animation: k 1s; animation-timeline: scroll(nearest) } #r { animation: k 1s; animation-timeline: view() }");
    /* #b is the nearest scroller of both */
    TEST_ASSERT_EQUAL(0, g_d.node[3].scr);
    TEST_ASSERT_EQUAL(1, g_d.node[4].scr);
    TEST_ASSERT_EQUAL_STRING(" style=\"color:#102030\"", style_of(3));
    TEST_ASSERT_EQUAL_STRING(" style=\"overflow-y:scroll;height:50px\"", style_of(4));
    /* scroll(self) on #b reads #b, scroll(nearest) on #b reads #o */
    prep(KF "#o { overflow: auto; height: 120px; color: #102030 } #b { overflow-y: scroll; height: 50px; "
         "animation: k 1s, k 2s; animation-timeline: scroll(self), scroll(nearest) }");
    TEST_ASSERT_EQUAL(1, g_d.node[3].scr);
    TEST_ASSERT_EQUAL(1, g_d.node[4].scr);
    TEST_ASSERT_EQUAL_STRING(" style=\"color:#102030;overflow-y:auto;height:120px\"", style_of(3));
    /* hidden, or no passed height: not passed on (still read) */
    prep(KF "#o { overflow: hidden; height: 120px } #b { overflow: auto; height: 50% } "
         "#b { animation: k 1s, k 2s; animation-timeline: scroll(nearest), scroll(self) } "
         "#i { animation: k 1s; animation-timeline: scroll(self) }");
    TEST_ASSERT_EQUAL(1, g_d.node[3].scr);
    TEST_ASSERT_EQUAL(1, g_d.node[4].scr);
    TEST_ASSERT_EQUAL(0, g_d.node[5].scr);
    TEST_ASSERT_EQUAL_STRING("", style_of(3));
    /* Lot 107 conversion (review point): a percentage height is passed
       on now; Lot 105 refused it ("") */
    TEST_ASSERT_EQUAL_STRING(" style=\"overflow-y:auto;height:50%\"", style_of(4));
    /* a height with no length the reader passes (fit-content): not passed */
    prep(KF "#b { overflow: auto; height: fit-content; animation: k 1s; animation-timeline: scroll(self) }");
    TEST_ASSERT_EQUAL(1, g_d.node[4].scr);
    TEST_ASSERT_EQUAL_STRING("", style_of(4));
    /* a box scroller no timeline reads is not passed on */
    prep(KF "#o { overflow: auto; height: 120px } #i { animation: k 1s; animation-timeline: scroll(root) }");
    TEST_ASSERT_EQUAL(0, g_d.node[3].scr);
    TEST_ASSERT_EQUAL_STRING("", style_of(3));
    /* scroll(self) on the root element: the root, no box */
    prep(KF "html { animation: k 1s; animation-timeline: scroll(self) } #o { overflow: auto; height: 9px }");
    TEST_ASSERT_EQUAL(0, g_d.node[3].scr);
    TEST_ASSERT_EQUAL(1, g_d.anim_scroll);
}

/* box scroller timelines read the box's own scroll state */
static void test_l105_box_clock(void) {
    wf_geom_box_t *b;
    memset(&g_g, 0, sizeof(g_g));
    b = box(4, 0, 4);
    b->sv = 1; b->s_vp = 50; b->s_rg = 400; b->s_sc = 100;
    wf_site_set_geom(&g_g);
    prep(KF "#b { overflow: auto; height: 50px } #i { animation: k 1000ms linear; animation-timeline: scroll(nearest) }");
    /* 100 / 400: 250 ms, #404040 */
    TEST_ASSERT_EQUAL_STRING(" style=\"color:#404040\"", style_of(5));
    TEST_ASSERT_EQUAL(1, g_d.anim_scroll);
    TEST_ASSERT_EQUAL(1, g_d.anim_box);
    TEST_ASSERT_EQUAL(1, g_d.geom_live);
    TEST_ASSERT_EQUAL(1, g_d.geom_need);
    TEST_ASSERT_EQUAL(1, g_d.geom_used);
    TEST_ASSERT_EQUAL_STRING("keyframes 1 animated 0 transitioned 0 font 0 at 0ms scroll 1 pos 0 color 1 0 view 0 need 1 "
                             "geom 1 box 1", anim());
    /* a named scroll timeline on the box, an animation-range on it */
    prep(KF "#b { overflow: auto; height: 50px; scroll-timeline: --s } "
         "#i { animation: k 1000ms linear; animation-timeline: --s; animation-range: 0px 200px }");
    TEST_ASSERT_EQUAL_STRING(" style=\"color:#808080\"", style_of(5));
    /* not a scroll box in the layout (sv 0), or no scroll range: no effect */
    b->sv = 0;
    prep(KF "#b { overflow: auto; height: 50px } #i { animation: k 1000ms linear; animation-timeline: scroll(nearest) }");
    TEST_ASSERT_EQUAL_STRING("", style_of(5));
    TEST_ASSERT_EQUAL(0, g_d.anim_scroll);
    TEST_ASSERT_EQUAL(0, g_d.anim_box);
    TEST_ASSERT_EQUAL(1, g_d.geom_used);
    b->sv = 1; b->s_rg = 0;
    prep(KF "#b { overflow: auto; height: 50px } #i { animation: k 1000ms linear; animation-timeline: scroll(nearest) }");
    TEST_ASSERT_EQUAL_STRING("", style_of(5));
    b->s_rg = 400;
    /* a box record the feed did not fill (ok 0) */
    b->ok = 0;
    prep(KF "#b { overflow: auto; height: 50px } #i { animation: k 1000ms linear; animation-timeline: scroll(nearest) }");
    TEST_ASSERT_EQUAL_STRING("", style_of(5));
    TEST_ASSERT_EQUAL(0, g_d.geom_used);
    b->ok = 1;
    /* a scroll-timeline-name on an element that is not a scroll container:
     * no effect, nothing read */
    prep(KF "#b { scroll-timeline: --s } #i { animation: k 1000ms linear; animation-timeline: --s }");
    TEST_ASSERT_EQUAL_STRING("", style_of(5));
    TEST_ASSERT_EQUAL(0, g_d.node[4].scr);
    TEST_ASSERT_EQUAL(0, g_d.geom_live);
    /* an inline or x axis (Lot 108, converted): the box scroller is read
     * along x; this geometry has no range along x (svx 0): no effect */
    prep(KF "#b { overflow: auto; height: 50px; scroll-timeline: --s x } "
         "#i { animation: k 1000ms linear; animation-timeline: --s, scroll(nearest inline) }");
    TEST_ASSERT_EQUAL_STRING("", style_of(5));
    TEST_ASSERT_EQUAL(1, g_d.node[4].scr);
    TEST_ASSERT_EQUAL(1, g_d.geom_live);
    /* no geometry: idle, but the box is still asked for */
    wf_site_set_geom(0);
    prep(KF "#b { overflow: auto; height: 50px } #i { animation: k 1000ms linear; animation-timeline: scroll(nearest) }");
    TEST_ASSERT_EQUAL(1, g_d.node[4].scr);
    TEST_ASSERT_EQUAL(1, g_d.geom_need);
    TEST_ASSERT_EQUAL(0, g_d.geom_used);
    TEST_ASSERT_EQUAL(1, g_d.geom_live);
    TEST_ASSERT_EQUAL(4, wf_site_geom_subject(&g_d, g_d.node[4].start));
}

/* a view timeline subject in a box scroller */
static void test_l105_box_view(void) {
    wf_geom_box_t *b;
    memset(&g_g, 0, sizeof(g_g));
    /* #i at 150+20 in a 100 px scrollport scrolled by 100: cover [50, 170],
     * (100 - 50) / 120 of 1000 ms = 416 ms, #6a6a6a */
    b = box(5, 0, 5);
    b->b_off = 150; b->b_size = 20; b->vp_b = 100; b->rg_b = 300; b->sc_b = 100;
    b->i_size = 300; b->vp_i = 300;
    wf_site_set_geom(&g_g);
    prep(KF "#b { overflow: auto; height: 100px } #i { animation: k 1000ms linear; animation-timeline: view() }");
    TEST_ASSERT_EQUAL_STRING(" style=\"color:#6a6a6a\"", style_of(5));
    TEST_ASSERT_EQUAL(1, g_d.anim_view);
    TEST_ASSERT_EQUAL(1, g_d.anim_box);
    TEST_ASSERT_EQUAL(1, g_d.node[5].geo);
    TEST_ASSERT_EQUAL(1, g_d.node[4].scr);
    TEST_ASSERT_EQUAL(2, g_d.geom_need);
    TEST_ASSERT_EQUAL(1, g_d.geom_used);
    /* the set scroll position does not move a box scroller */
    wf_site_set_scroll(10000);
    prep(KF "#b { overflow: auto; height: 100px } #i { animation: k 1000ms linear; animation-timeline: view() }");
    wf_site_set_scroll(0);
    TEST_ASSERT_EQUAL_STRING(" style=\"color:#6a6a6a\"", style_of(5));
    /* the layout puts it in another box (sm), or the root: no effect */
    b->sm = 4;
    prep(KF "#b { overflow: auto; height: 100px } #i { animation: k 1000ms linear; animation-timeline: view() }");
    TEST_ASSERT_EQUAL_STRING("", style_of(5));
    b->sm = 5; b->root = 1;
    prep(KF "#b { overflow: auto; height: 100px } #i { animation: k 1000ms linear; animation-timeline: view() }");
    TEST_ASSERT_EQUAL_STRING("", style_of(5));
    /* the reader sees no box scroller (html is not one) but the layout
     * does: no effect */
    b->root = 0;
    prep(KF "html { overflow: auto } #i { animation: k 1000ms linear; animation-timeline: view() }");
    TEST_ASSERT_EQUAL_STRING("", style_of(5));
    /* the inline axis: the box's inline offset (a box scroller does not
     * scroll along x: sc_i 0) */
    b->i_off = 0; b->i_size = 100; b->vp_i = 200;
    prep(KF "#b { overflow: auto; height: 100px } #i { animation: k 1000ms linear; animation-timeline: view(inline) }");
    /* cover [-200, 100], pos 0: 666 ms, #aaaaaa */
    TEST_ASSERT_EQUAL_STRING(" style=\"color:#aaaaaa\"", style_of(5));
    wf_site_set_geom(0);
}

/* the GUI position: the root scroll offset of the geometry */
static void test_l105_gui(void) {
    wf_geom_box_t *b;
    memset(&g_g, 0, sizeof(g_g));
    g_g.vw = 640; g_g.vh = 480; g_g.rg_y = 1000; g_g.sc_y = 250;
    wf_site_set_geom(&g_g);
    wf_site_set_scroll(7500);
    prep(KF "#i { animation: k 1000ms linear; animation-timeline: scroll(root) }");
    TEST_ASSERT_EQUAL_STRING(" style=\"color:#bfbfbf\"", style_of(5));
    TEST_ASSERT_EQUAL(0, g_d.geom_live);
    TEST_ASSERT_EQUAL(0, g_d.geom_need);
    TEST_ASSERT_EQUAL(0, g_d.gui_y);
    wf_site_set_scroll_gui(1);
    prep(KF "#i { animation: k 1000ms linear; animation-timeline: scroll(root) }");
    TEST_ASSERT_EQUAL(1, g_d.scroll_gui);
    TEST_ASSERT_EQUAL_STRING(" style=\"color:#404040\"", style_of(5));
    TEST_ASSERT_EQUAL(1, g_d.geom_live);
    TEST_ASSERT_EQUAL(1, g_d.geom_need);
    TEST_ASSERT_EQUAL(250, g_d.gui_y);
    TEST_ASSERT_EQUAL_STRING("keyframes 1 animated 0 transitioned 0 font 0 at 0ms scroll 1 pos gui color 1 0 view 0 need 1 "
                             "geom 0 gui 250", anim());
    /* a view timeline on the root: the box's scroll offset (sc_b), not
     * the set position: S 600 E 700 in 480: cover [120, 700] at 250: 224
     * ms, #393939 */
    b = box(5, 1, 0);
    b->b_off = 600; b->b_size = 100; b->vp_b = 480; b->rg_b = 1000; b->sc_b = 250;
    prep(KF "#i { animation: k 1000ms linear; animation-timeline: view() }");
    TEST_ASSERT_EQUAL_STRING(" style=\"color:#393939\"", style_of(5));
    TEST_ASSERT_EQUAL_STRING("keyframes 1 animated 0 transitioned 0 font 0 at 0ms color 1 0 view 1 need 1 geom 1 gui 250",
                             anim());
    /* without geometry: no effect, the root range is asked for */
    wf_site_set_geom(0);
    prep(KF "#i { animation: k 1000ms linear; animation-timeline: scroll(root) }");
    TEST_ASSERT_EQUAL_STRING("", style_of(5));
    TEST_ASSERT_EQUAL(1, g_d.geom_need);
    TEST_ASSERT_EQUAL(0, g_d.gui_y);
    TEST_ASSERT_EQUAL_STRING("keyframes 1 animated 0 transitioned 0 font 0 at 0ms view 0 need 1 geom 0 gui 0", anim());
    /* the document timeline: nothing live, no gui part */
    prep(KF "#i { animation: k 1000ms linear }");
    TEST_ASSERT_EQUAL(0, g_d.geom_live);
    TEST_ASSERT_NULL(strstr(anim(), "gui"));
    wf_site_set_scroll_gui(0);
    wf_site_set_scroll(0);
    prep(KF "#i { animation: k 1000ms linear; animation-timeline: scroll(root) }");
    TEST_ASSERT_EQUAL(0, g_d.scroll_gui);
    TEST_ASSERT_EQUAL_STRING(" style=\"color:#000000\"", style_of(5));
}

/* the elements the reader view keeps as a box of their own */
static void test_l105_geom_block(void) {
    prep(KF "#o { overflow: auto; height: 50px } #i { animation: k 1s; animation-timeline: scroll(nearest) } "
         "#q { animation: k 1s; animation-timeline: view() } #b { animation: k 1s; animation-timeline: view() }");
    /* the read box scroller */
    TEST_ASSERT_EQUAL(1, wf_site_geom_block(&g_d, g_d.node[3].start));
    /* a view subject holding blocks (#b holds two p) */
    TEST_ASSERT_EQUAL(1, wf_site_geom_block(&g_d, g_d.node[4].start));
    /* a view subject with inline content only */
    TEST_ASSERT_EQUAL(0, wf_site_geom_block(&g_d, g_d.node[7].start));
    /* neither */
    TEST_ASSERT_EQUAL(0, wf_site_geom_block(&g_d, g_d.node[5].start));
    TEST_ASSERT_EQUAL(0, wf_site_geom_block(&g_d, 3));
    TEST_ASSERT_EQUAL(3, wf_site_geom_subject(&g_d, g_d.node[3].start));
    TEST_ASSERT_EQUAL(-1, wf_site_geom_subject(&g_d, g_d.node[5].start));
}

/* a style attribute: its declarations share one rank, the later wins */
static void test_l105_inline(void) {
    static const char h[] = "<html><body><div id=o style=\"overflow: auto; overflow: hidden scroll; height: 10px; "
                            "height: 20px; overflow-x: clip\"><p id=i>i</p></div></body></html>";
    TEST_ASSERT_EQUAL(0, wf_site_prepare_ext(&g_d, h, (int)strlen(h), 640, 480, 0, 0));
    TEST_ASSERT_EQUAL(4, g_d.nnodes);
    TEST_ASSERT_EQUAL(WF_SITE_OV_CLIP, g_d.node[2].ov[0]);
    TEST_ASSERT_EQUAL(WF_SITE_OV_SCROLL, g_d.node[2].ov[1]);
    TEST_ASSERT_EQUAL(21, g_d.node[2].hpx);
    TEST_ASSERT_EQUAL(2, wf_site_scroller_of(&g_d, 3));
}

int main(void) {
    unity_init();
    RUN_TEST(test_l105_overflow_values);
    RUN_TEST(test_l105_height_values);
    RUN_TEST(test_l105_containers);
    RUN_TEST(test_l105_passed);
    RUN_TEST(test_l105_box_clock);
    RUN_TEST(test_l105_box_view);
    RUN_TEST(test_l105_gui);
    RUN_TEST(test_l105_geom_block);
    RUN_TEST(test_l105_inline);
    unity_print_results();
    unity_cleanup();
    return unity_stats.tests_failed == 0 ? 0 : 1;
}
