/* Lot 110 (US-031): the body element as a box scroller of its own when
 * html's overflow is not visible (CSS Overflow 3, 3.3), its timelines on
 * the body's geometry record, its percentage heights against html's
 * height; opacity values, keyframes, transitions and inherit
 * (wf_sitecss.c). Hand cases next to the generated test_lot110_*.c;
 * expected values follow docs/lot110_plan.md. */
#include "../../framework/unity.h"
#include <string.h>
#include <stdio.h>
#include "wf_sitecss.h"

static char g_o[512];
static wf_site_t g_d;
static char g_html[4096];
static wf_geom_t g_g;

#define KF "@keyframes k { from { color: #000000 } to { color: #ffffff } } "
#define RD "#i { animation: k 1s linear; animation-timeline: scroll(nearest) } "
#define HB "html { overflow: hidden } body { overflow: auto; height: 300px } "

/* html 0, head 1, body 2, #o 3, #b 4, #i 5 */
static void prep(const char *css) {
    snprintf(g_html, sizeof(g_html), "<html><head><style>" KF "%s</style></head><body><div id=o><div id=b>"
             "<p id=i>i</p></div></div></body></html>", css);
    TEST_ASSERT_EQUAL(0, wf_site_prepare_ext(&g_d, g_html, (int)strlen(g_html), 640, 480, 0, 0));
}

static void prep_at(const char *css, int ms) {
    wf_site_set_time(ms);
    prep(css);
    wf_site_set_time(0);
}

/* with the geometry g_g */
static void prep_g(const char *css) {
    wf_site_set_geom(&g_g);
    prep(css);
    wf_site_set_geom(0);
}

static const char *style_of(const char *id) {
    int i;
    g_o[0] = 0;
    for (i = 0; i < g_d.nnodes; i++)
        if (!strcmp(g_d.node[i].id, id) || (!g_d.node[i].id[0] && !strcmp(g_d.node[i].tag, id)))
            wf_site_style_attr(&g_d, g_d.node[i].start, g_o, (int)sizeof(g_o));
    return g_o;
}

static wf_geom_box_t *box(int k, int root, int sm) {
    wf_geom_box_t *b = k < 0 ? &g_g.body : &g_g.box[k];
    memset(b, 0, sizeof(*b));
    b->ok = 1; b->root = (unsigned char)root; b->bvert = 1; b->sm = sm;
    b->vp_b = 200; b->vp_i = 640; b->i_size = 600;
    return b;
}

static void geo_reset(void) {
    memset(&g_g, 0, sizeof(g_g));
    g_g.vw = 640; g_g.vh = 480; g_g.rg_y = 400;
}

/* the body keeps its overflow unless html's is visible along both axes */
static void test_l110_body_own(void) {
    prep(HB);
    TEST_ASSERT_EQUAL(WF_SITE_OV_AUTO, wf_site_box_scroller(&g_d, 2));
    TEST_ASSERT_EQUAL(0, wf_site_box_scroller(&g_d, 0));
    TEST_ASSERT_EQUAL(2, wf_site_scroller_of(&g_d, 5));
    TEST_ASSERT_EQUAL(-1, wf_site_scroller_of(&g_d, 2));
    /* html visible: the body's overflow is the viewport's */
    prep("html { overflow: visible } body { overflow: auto; height: 300px } ");
    TEST_ASSERT_EQUAL(0, wf_site_box_scroller(&g_d, 2));
    TEST_ASSERT_EQUAL(-1, wf_site_scroller_of(&g_d, 5));
    prep("body { overflow: scroll } ");
    TEST_ASSERT_EQUAL(0, wf_site_box_scroller(&g_d, 2));
    /* one axis of html is enough: overflow-x hidden makes y auto too */
    prep("html { overflow-x: hidden } body { overflow: scroll } ");
    TEST_ASSERT_EQUAL(WF_SITE_OV_SCROLL, wf_site_box_scroller(&g_d, 2));
    /* clip is not visible (visible clip: x visible, y clip) */
    prep("html { overflow: visible clip } body { overflow: hidden } ");
    TEST_ASSERT_EQUAL(WF_SITE_OV_HIDDEN, wf_site_box_scroller(&g_d, 2));
    prep("html { overflow: clip visible } body { overflow: hidden } ");
    TEST_ASSERT_EQUAL(WF_SITE_OV_HIDDEN, wf_site_box_scroller(&g_d, 2));
    /* the body itself needs a box value */
    prep("html { overflow: hidden } body { overflow: visible clip } ");
    TEST_ASSERT_EQUAL(0, wf_site_box_scroller(&g_d, 2));
    prep("html { overflow: hidden } ");
    TEST_ASSERT_EQUAL(0, wf_site_box_scroller(&g_d, 2));
    /* a box scroller inside the body is still the nearest */
    prep(HB "#o { overflow: auto; height: 50px }");
    TEST_ASSERT_EQUAL(3, wf_site_scroller_of(&g_d, 5));
    TEST_ASSERT_EQUAL(2, wf_site_scroller_of(&g_d, 3));
}

/* a body without an html element around it is the root's (no body_own) */
static void test_l110_body_top(void) {
    static const char page[] = "<body><style>body { overflow: auto } div { overflow: hidden }</style>"
                               "<div id=o><p id=i>i</p></div></body>";
    int k, b = -1;
    TEST_ASSERT_EQUAL(0, wf_site_prepare_ext(&g_d, page, (int)strlen(page), 640, 480, 0, 0));
    for (k = 0; k < g_d.nnodes; k++) if (!strcmp(g_d.node[k].tag, "body")) b = k;
    TEST_ASSERT_TRUE(b >= 0);
    TEST_ASSERT_EQUAL(-1, g_d.node[b].parent);
    TEST_ASSERT_EQUAL(0, wf_site_box_scroller(&g_d, b));
}

/* scroll timelines read the body's own record */
static void test_l110_body_scroll(void) {
    wf_geom_box_t *b;
    geo_reset();
    b = box(-1, 1, 0);
    b->sv = 1; b->s_vp = 300; b->s_rg = 1000; b->s_sc = 500;
    /* box[2] (the body's index) is not read */
    b = box(2, 1, 0);
    b->sv = 1; b->s_rg = 1000; b->s_sc = 0;
    prep_g(HB RD);
    TEST_ASSERT_EQUAL_STRING(" style=\"color:#808080\"", style_of("i"));
    TEST_ASSERT_EQUAL_STRING(" style=\"overflow-y:auto;height:300px\"", style_of("body"));
    TEST_ASSERT_EQUAL(1, g_d.node[2].scr);
    TEST_ASSERT_EQUAL(1, g_d.anim_box);
    /* scroll(self) on the body, a named scroll timeline on it */
    prep_g(HB "body { animation: k 1s linear; animation-timeline: scroll(self) }");
    TEST_ASSERT_EQUAL_STRING(" style=\"color:#808080;overflow-y:auto;height:300px\"", style_of("body"));
    prep_g(HB "body { scroll-timeline: --b } #i { animation: k 1s linear; animation-timeline: --b }");
    TEST_ASSERT_EQUAL_STRING(" style=\"color:#808080\"", style_of("i"));
    /* without sv on the record: no effect */
    g_g.body.sv = 0;
    prep_g(HB RD);
    TEST_ASSERT_EQUAL_STRING("", style_of("i"));
    TEST_ASSERT_EQUAL(1, g_d.node[2].scr);
    /* html visible: the root's range (rg_y 400 at the reader position 0) */
    g_g.body.sv = 1;
    prep_g("body { overflow: auto; height: 300px } " RD);
    TEST_ASSERT_EQUAL_STRING(" style=\"color:#000000\"", style_of("i"));
    TEST_ASSERT_EQUAL(0, g_d.node[2].scr);
    TEST_ASSERT_EQUAL_STRING("", style_of("body"));
    /* no body record: no effect */
    g_g.body.ok = 0;
    prep_g(HB RD);
    TEST_ASSERT_EQUAL_STRING("", style_of("i"));
}

/* along x: overflow-x and a width on the body, svx of its record */
static void test_l110_body_x(void) {
    wf_geom_box_t *b;
    geo_reset();
    b = box(-1, 1, 0);
    b->svx = 1; b->x_vp = 300; b->x_rg = 400; b->x_sc = 100;
    prep_g("html { overflow: hidden } body { overflow-x: auto; width: 300px } "
           "#i { animation: k 1s linear; animation-timeline: scroll(nearest x) }");
    TEST_ASSERT_EQUAL_STRING(" style=\"color:#404040\"", style_of("i"));
    TEST_ASSERT_EQUAL_STRING(" style=\"overflow-x:auto;width:300px\"", style_of("body"));
}

/* view timelines of boxes in the body box scroller: sm WF_GEOM_SM_BODY */
static void test_l110_body_view(void) {
    wf_geom_box_t *b;
    geo_reset();
    (void)box(-1, 1, 0);
    b = box(5, 0, WF_GEOM_SM_BODY);
    b->b_off = 300; b->b_size = 100; b->rg_b = 1000; b->sc_b = 250;
    prep_g(HB "#i { animation: k 1s linear; animation-timeline: view() }");
    TEST_ASSERT_EQUAL_STRING(" style=\"color:#808080\"", style_of("i"));
    TEST_ASSERT_EQUAL(1, g_d.anim_box);
    TEST_ASSERT_EQUAL(1, g_d.anim_view);
    /* the body's index + 1 is not its mark */
    g_g.box[5].sm = 3;
    prep_g(HB "#i { animation: k 1s linear; animation-timeline: view() }");
    TEST_ASSERT_EQUAL_STRING("", style_of("i"));
    /* a marked box scroller keeps index + 1 */
    g_g.box[5].sm = 4;
    prep_g(HB "#o { overflow: auto; height: 50px } #i { animation: k 1s linear; animation-timeline: view() }");
    TEST_ASSERT_EQUAL_STRING(" style=\"color:#808080\"", style_of("i"));
    g_g.box[5].sm = WF_GEOM_SM_BODY;
    prep_g(HB "#o { overflow: auto; height: 50px } #i { animation: k 1s linear; animation-timeline: view() }");
    TEST_ASSERT_EQUAL_STRING("", style_of("i"));
    /* the body's own view timeline is the root's (480 / 680 of the
     * way at the reader position 0); nothing reads the body as a
     * scroller, so no box is passed */
    g_g.body.b_off = 0; g_g.body.b_size = 200; g_g.body.vp_b = 480; g_g.body.rg_b = 400; g_g.body.sc_b = 0;
    prep_g(HB "body { animation: k 1s linear; animation-timeline: view() }");
    TEST_ASSERT_EQUAL_STRING(" style=\"color:#b4b4b4\"", style_of("body"));
    TEST_ASSERT_EQUAL(0, g_d.node[2].scr);
    /* a body record that says it is in a box scroller: no effect */
    g_g.body.root = 0;
    prep_g(HB "body { animation: k 1s linear; animation-timeline: view() }");
    TEST_ASSERT_EQUAL_STRING("", style_of("body"));
}

/* percentages of the body box scroller against html's height */
static void test_l110_body_pct(void) {
    prep("html { overflow: hidden; height: 400px } body { overflow: auto; height: 50% } " RD);
    TEST_ASSERT_EQUAL_STRING(" style=\"overflow-y:auto;height:200px\"", style_of("body"));
    prep("html { overflow: hidden; height: 50% } body { overflow: auto; height: 50% } " RD);
    TEST_ASSERT_EQUAL_STRING(" style=\"overflow-y:auto;height:120px\"", style_of("body"));
    prep("html { overflow: hidden; height: 100% } body { overflow: auto; height: 100%; max-height: 75%; min-height: 10% } " RD);
    TEST_ASSERT_EQUAL_STRING(" style=\"overflow-y:auto;height:480px;max-height:360px;min-height:48px\"", style_of("body"));
    /* truncated at each step: 480 * 33 / 100 = 158, 158 * 33 / 100 = 52 */
    prep("html { overflow: hidden; height: 33% } body { overflow: auto; height: 33% } " RD);
    TEST_ASSERT_EQUAL_STRING(" style=\"overflow-y:auto;height:52px\"", style_of("body"));
    /* html in viewport units or em: px */
    prep("html { overflow: hidden; height: 50vh } body { overflow: auto; height: 50% } " RD);
    TEST_ASSERT_EQUAL_STRING(" style=\"overflow-y:auto;height:120px\"", style_of("body"));
    /* html auto: left as it is (Lot 111: an html height given as an
     * expression is resolved against the viewport, 50% of 480 - 20) */
    prep("html { overflow: hidden } body { overflow: auto; height: 50% } " RD);
    TEST_ASSERT_EQUAL_STRING(" style=\"overflow-y:auto;height:50%\"", style_of("body"));
    prep("html { overflow: hidden; height: calc(100% - 20px) } body { overflow: auto; height: 50% } " RD);
    TEST_ASSERT_EQUAL_STRING(" style=\"overflow-y:auto;height:230px\"", style_of("body"));
    /* the body's own px or expression is not touched */
    prep("html { overflow: hidden; height: 400px } body { overflow: auto; height: 300px } " RD);
    TEST_ASSERT_EQUAL_STRING(" style=\"overflow-y:auto;height:300px\"", style_of("body"));
    prep("html { overflow: hidden; height: 400px } body { overflow: auto; height: calc(50% + 1px) } " RD);
    TEST_ASSERT_EQUAL_STRING(" style=\"overflow-y:auto;height:calc(50% + 1px)\"", style_of("body"));
    /* another box scroller's percentage is of its containing block */
    prep("html { overflow: hidden; height: 400px } #o { overflow: auto; height: 50% } " RD);
    TEST_ASSERT_EQUAL_STRING(" style=\"overflow-y:auto;height:50%\"", style_of("o"));
    /* bounds: html 10000% of 480 is 48000, capped at 30000 first */
    prep("html { overflow: hidden; height: 10000% } body { overflow: auto; height: 50% } " RD);
    TEST_ASSERT_EQUAL_STRING(" style=\"overflow-y:auto;height:15000px\"", style_of("body"));
    prep("html { overflow: hidden; height: 20000px } body { overflow: auto; height: 1000% } " RD);
    TEST_ASSERT_EQUAL_STRING(" style=\"overflow-y:auto;height:30000px\"", style_of("body"));
}

/* opacity values: number or percentage, clamped, 1/10000 truncated */
static void test_l110_op_values(void) {
    static const struct { const char *v; const char *out; } c[] = {
        {"0.5", "50"}, {".25", "25"}, {"+.75", "75"}, {"1", "100"}, {"0", "0"}, {"-0.5", "0"}, {"-0", "0"},
        {"2", "100"}, {"100000000", "100"}, {"50%", "50"}, {"50.5%", "50"}, {"0.505", "50"}, {"150%", "100"},
        {"-10%", "0"}, {"0.009", "0"}, {"0.0199", "1"}, {"99.99%", "99"}, {"0.99999", "99"}, {"1000000%", "100"},
        {"7%", "7"}, {"0.07", "7"}, {"calc(0.5)", "50"}};     /* Lot 111: calc(0.5) is a value */
    /* Lot 111: calc(0.5) is a value (above) and revert rolls back (below) */
    static const char *const bad[] = {"0.5px", "1e-1", "5.", ".", "%", "half", "50 %", "+-1", "--x",
                                      "0.5 0.5", "none", "1.5.5", ""};
    char css[160], want[64];
    int k;
    for (k = 0; k < (int)(sizeof(c) / sizeof(c[0])); k++) {
        snprintf(css, sizeof(css), "#o { opacity: %s }", c[k].v);
        prep(css);
        snprintf(want, sizeof(want), " style=\"opacity:%s%%\"", c[k].out);
        TEST_ASSERT_EQUAL_STRING(want, style_of("o"));
    }
    for (k = 0; k < (int)(sizeof(bad) / sizeof(bad[0])); k++) {
        snprintf(css, sizeof(css), "#o { opacity: 0.3; opacity: %s }", bad[k]);
        prep(css);
        TEST_ASSERT_EQUAL_STRING(" style=\"opacity:30%\"", style_of("o"));
    }
    /* Lot 111: revert leaves no author opacity: 1, not written */
    prep("#o { opacity: 0.3; opacity: revert }");
    TEST_ASSERT_EQUAL_STRING("", style_of("o"));
    /* initial and unset: 1, not written; the strongest wins */
    prep("#o { opacity: 0.3; opacity: initial }");
    TEST_ASSERT_EQUAL_STRING("", style_of("o"));
    prep("#o { opacity: 0.3; opacity: unset }");
    TEST_ASSERT_EQUAL_STRING("", style_of("o"));
    prep("#o { opacity: 0.3 !important } #o { opacity: 0.6 } div { opacity: 0.9 }");
    TEST_ASSERT_EQUAL_STRING(" style=\"opacity:30%\"", style_of("o"));
    prep("div#o { opacity: 0.2 } #o { opacity: 0.6 }");
    TEST_ASSERT_EQUAL_STRING(" style=\"opacity:20%\"", style_of("o"));
    /* a style attribute wins */
    snprintf(g_html, sizeof(g_html), "<html><head><style>#o { opacity: 0.2 }</style></head><body>"
             "<div id=o style=\"opacity: 0.8\">o</div></body></html>");
    TEST_ASSERT_EQUAL(0, wf_site_prepare_ext(&g_d, g_html, (int)strlen(g_html), 640, 480, 0, 0));
    TEST_ASSERT_EQUAL_STRING(" style=\"opacity:80%\"", style_of("o"));
    /* with other properties, before the box */
    prep(HB "body { color: red; opacity: 25% } " RD);
    TEST_ASSERT_EQUAL_STRING(" style=\"color:#ff0000;opacity:25%;overflow-y:auto;height:300px\"", style_of("body"));
}

/* inherit: the parent's final opacity, 1 when it has none */
static void test_l110_op_inherit(void) {
    prep("#o { opacity: 0.4 } #b { opacity: inherit } #i { opacity: inherit }");
    TEST_ASSERT_EQUAL_STRING(" style=\"opacity:40%\"", style_of("b"));
    TEST_ASSERT_EQUAL_STRING(" style=\"opacity:40%\"", style_of("i"));
    prep("#b { opacity: inherit }");
    TEST_ASSERT_EQUAL_STRING(" style=\"opacity:100%\"", style_of("b"));
    prep("#o { opacity: initial } #b { opacity: inherit }");
    TEST_ASSERT_EQUAL_STRING(" style=\"opacity:100%\"", style_of("b"));
    /* not inherited by default */
    prep("#o { opacity: 0.4 }");
    TEST_ASSERT_EQUAL_STRING("", style_of("b"));
    /* the parent's animated value */
    prep_at("@keyframes f { from { opacity: 0 } to { opacity: 1 } } #o { animation: f 1s linear } #b { opacity: inherit }", 250);
    TEST_ASSERT_EQUAL_STRING(" style=\"opacity:25%\"", style_of("o"));
    TEST_ASSERT_EQUAL_STRING(" style=\"opacity:25%\"", style_of("b"));
}

/* keyframes, compositions, !important, transitions */
static void test_l110_op_anim(void) {
    prep_at("@keyframes f { from { opacity: 0 } to { opacity: 100% } } #o { animation: f 1s linear }", 250);
    TEST_ASSERT_EQUAL_STRING(" style=\"opacity:25%\"", style_of("o"));
    TEST_ASSERT_EQUAL(1, g_d.op_animated);
    /* keyframe values clamped when taken: 2 is 1 */
    prep_at("@keyframes f { from { opacity: -1 } to { opacity: 2 } } #o { animation: f 1s linear }", 500);
    TEST_ASSERT_EQUAL_STRING(" style=\"opacity:50%\"", style_of("o"));
    /* a keyframe that is no opacity value is left out (the own value);
     * Lot 112 (converted): calc(1) is a value now (was left out), so the
     * left-out keyframe is a length */
    prep_at("@keyframes f { from { opacity: 0 } to { opacity: 1px } } #o { opacity: 0.5; animation: f 1s linear }", 500);
    TEST_ASSERT_EQUAL_STRING(" style=\"opacity:25%\"", style_of("o"));
    prep_at("@keyframes f { from { opacity: 0 } to { opacity: calc(1) } } #o { opacity: 0.5; animation: f 1s linear }", 500);
    TEST_ASSERT_EQUAL_STRING(" style=\"opacity:50%\"", style_of("o"));
    /* add on the own value, clamped after */
    prep_at("@keyframes f { to { opacity: 0.6 } } #o { opacity: 0.5; animation: f 1s linear; animation-composition: add }", 500);
    TEST_ASSERT_EQUAL_STRING(" style=\"opacity:100%\"", style_of("o"));     /* 1.05 */
    TEST_ASSERT_EQUAL(1, g_d.prop_composed);
    prep_at("@keyframes f { from { opacity: 0.2 } to { opacity: 0.2 } } #o { opacity: 0.3; animation: f 1s linear; "
            "animation-composition: add }", 500);
    TEST_ASSERT_EQUAL_STRING(" style=\"opacity:50%\"", style_of("o"));
    prep_at("@keyframes f { from { opacity: 0.8 } to { opacity: 0.8 } } #o { opacity: 0.5; animation: f 1s linear; "
            "animation-composition: accumulate }", 500);
    TEST_ASSERT_EQUAL_STRING(" style=\"opacity:100%\"", style_of("o"));
    /* !important own value: no animation */
    prep_at("@keyframes f { from { opacity: 0 } to { opacity: 1 } } #o { opacity: 0.7 !important; animation: f 1s linear }", 500);
    TEST_ASSERT_EQUAL_STRING(" style=\"opacity:70%\"", style_of("o"));
    TEST_ASSERT_EQUAL(0, g_d.op_animated);
    /* a transition from @starting-style */
    prep_at("@starting-style { #o { opacity: 0 } } #o { opacity: 0.8; transition: opacity 1s linear }", 500);
    TEST_ASSERT_EQUAL_STRING(" style=\"opacity:40%\"", style_of("o"));
    TEST_ASSERT_EQUAL(1, g_d.op_transitioned);
    prep_at("@starting-style { #o { opacity: initial } } #o { opacity: 0.2; transition: all 1s linear }", 500);
    TEST_ASSERT_EQUAL_STRING(" style=\"opacity:60%\"", style_of("o"));
    /* inherit is no starting value; ended or other properties: none */
    prep_at("@starting-style { #o { opacity: inherit } } #o { opacity: 0.8; transition: opacity 1s linear }", 500);
    TEST_ASSERT_EQUAL_STRING(" style=\"opacity:80%\"", style_of("o"));
    TEST_ASSERT_EQUAL(0, g_d.op_transitioned);
    prep_at("@starting-style { #o { opacity: 0 } } #o { opacity: 0.8; transition: opacity 1s linear }", 1000);
    TEST_ASSERT_EQUAL_STRING(" style=\"opacity:80%\"", style_of("o"));
    prep_at("@starting-style { #o { opacity: 0 } } #o { opacity: 0.8; transition: color 1s linear }", 500);
    TEST_ASSERT_EQUAL_STRING(" style=\"opacity:80%\"", style_of("o"));
    /* an element without opacity transitions from its starting value to 1 */
    prep_at("@starting-style { #o { opacity: 0 } } #o { transition: opacity 1s linear }", 500);
    TEST_ASSERT_EQUAL_STRING(" style=\"opacity:50%\"", style_of("o"));
    /* the strongest starting value wins */
    prep_at("@starting-style { #o { opacity: 0 } div { opacity: 0.6 } } #o { transition: opacity 1s linear }", 500);
    TEST_ASSERT_EQUAL_STRING(" style=\"opacity:50%\"", style_of("o"));
    /* the counters in the animation text */
    {
        char t[256];
        prep_at("@keyframes f { to { opacity: 0 } } #o { animation: f 1s linear }", 500);
        TEST_ASSERT_TRUE(wf_site_anim_text(&g_d, t, (int)sizeof(t)) > 0);
        TEST_ASSERT_NOT_NULL(strstr(t, " opacity 1 0"));
        prep_at("@keyframes f { to { color: red } } #o { animation: f 1s linear }", 500);
        TEST_ASSERT_TRUE(wf_site_anim_text(&g_d, t, (int)sizeof(t)) > 0);
        TEST_ASSERT_NULL(strstr(t, "opacity"));
    }
    /* scroll-driven opacity on the body box scroller */
    {
        wf_geom_box_t *b;
        geo_reset();
        b = box(-1, 1, 0);
        b->sv = 1; b->s_rg = 1000; b->s_sc = 750;
        prep_g(HB "@keyframes f { from { opacity: 0 } to { opacity: 1 } } #i { animation: f 1s linear; "
               "animation-timeline: scroll(nearest) }");
        TEST_ASSERT_EQUAL_STRING(" style=\"opacity:75%\"", style_of("i"));
    }
}

/* a body nested in a div is not the root's body (its parent's overflow
 * says nothing); body percentages of a viewport other than 480; a
 * dropped opacity keeps the declarations after it; integer parts far
 * past the int range */
static void test_l110_edges(void) {
    snprintf(g_html, sizeof(g_html), "<html><head><style>div { overflow: hidden } body { overflow: auto; height: 100px }"
             "</style></head><body><div id=o><body id=n><p>x</p></body></div></body></html>");
    TEST_ASSERT_EQUAL(0, wf_site_prepare_ext(&g_d, g_html, (int)strlen(g_html), 640, 480, 0, 0));
    TEST_ASSERT_EQUAL_STRING("body", g_d.node[4].tag);
    TEST_ASSERT_EQUAL(3, g_d.node[4].parent);
    TEST_ASSERT_EQUAL(0, wf_site_box_scroller(&g_d, 4));
    TEST_ASSERT_EQUAL(WF_SITE_OV_HIDDEN, wf_site_box_scroller(&g_d, 3));
    /* html 50% of a 600 px viewport: 300, the body 50% of it */
    snprintf(g_html, sizeof(g_html), "<html><head><style>" KF "html { overflow: hidden; height: 50%% } body { overflow: auto; "
             "height: 50%% } " RD "</style></head><body><div id=o><div id=b><p id=i>i</p></div></div></body></html>");
    TEST_ASSERT_EQUAL(0, wf_site_prepare_ext(&g_d, g_html, (int)strlen(g_html), 640, 600, 0, 0));
    TEST_ASSERT_EQUAL_STRING(" style=\"overflow-y:auto;height:150px\"", style_of("body"));
    prep("#o { opacity: 2px; color: red }");
    TEST_ASSERT_EQUAL_STRING(" style=\"color:#ff0000\"", style_of("o"));
    prep("#o { opacity: half; opacity: 0.4; color: red }");
    TEST_ASSERT_EQUAL_STRING(" style=\"color:#ff0000;opacity:40%\"", style_of("o"));
    prep("#o { opacity: 99999999999999999999 }");
    TEST_ASSERT_EQUAL_STRING(" style=\"opacity:100%\"", style_of("o"));
    prep("#o { opacity: 4294967296.5% }");
    TEST_ASSERT_EQUAL_STRING(" style=\"opacity:100%\"", style_of("o"));
    prep("#o { opacity: 00000000000000000000.25 }");
    TEST_ASSERT_EQUAL_STRING(" style=\"opacity:25%\"", style_of("o"));
    /* 429497 * 10000 wraps to 2704 in 32 bits: still 1 */
    prep("#o { opacity: 429497 }");
    TEST_ASSERT_EQUAL_STRING(" style=\"opacity:100%\"", style_of("o"));
    /* the fourth decimal is kept between the passes: (0.0199 + 0.0203) / 2 */
    prep_at("@keyframes f { to { opacity: 0.0203 } } #o { opacity: 0.0199; animation: f 1s linear }", 500);
    TEST_ASSERT_EQUAL_STRING(" style=\"opacity:2%\"", style_of("o"));
    /* inherit on the root element: the initial value 1 */
    prep("html { opacity: inherit }");
    TEST_ASSERT_EQUAL_STRING(" style=\"opacity:100%\"", style_of("html"));
    /* opacity alone before the box: one ; between them */
    prep(HB "body { opacity: 25% } " RD);
    TEST_ASSERT_EQUAL_STRING(" style=\"opacity:25%;overflow-y:auto;height:300px\"", style_of("body"));
    /* a weaker @starting-style rule still starts the transition (its own
     * rank, not the element's value's) */
    prep_at("#o { opacity: 0.8; transition: opacity 1s linear } @starting-style { div { opacity: 0 } }", 500);
    TEST_ASSERT_EQUAL_STRING(" style=\"opacity:40%\"", style_of("o"));
    /* a transition alone: the css-anim text says opacity 0 1 */
    {
        char t[512];
        wf_site_anim_text(&g_d, t, (int)sizeof(t));
        TEST_ASSERT_NOT_NULL(strstr(t, " opacity 0 1"));
    }
}

int main(void) {
    unity_init();
    RUN_TEST(test_l110_body_own);
    RUN_TEST(test_l110_body_top);
    RUN_TEST(test_l110_body_scroll);
    RUN_TEST(test_l110_body_x);
    RUN_TEST(test_l110_body_view);
    RUN_TEST(test_l110_body_pct);
    RUN_TEST(test_l110_op_values);
    RUN_TEST(test_l110_op_inherit);
    RUN_TEST(test_l110_op_anim);
    RUN_TEST(test_l110_edges);
    unity_print_results();
    unity_cleanup();
    return unity_stats.tests_failed == 0 ? 0 : 1;
}
