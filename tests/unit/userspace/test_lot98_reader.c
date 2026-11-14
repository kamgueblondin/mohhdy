/* Lot 98 (US-031): hand-written reader cases next to the generated suites
 * (test_lot98_hsl.c, test_lot98_add.c, test_lot98_longs.c,
 * test_lot98_sheet.c): ex and ch from the engine font, hsl() in the
 * reader, the longhand cascade, animation-composition, @keyframes inside
 * @scope, the summary line and the API guards. */
#include "../../framework/unity.h"
#include <string.h>
#include <stdio.h>
#include "wf_sitecss.h"
#include "wf_cssprop.h"
#include "webeng.h"
#include "kernel/gfx_font8.h"

static wf_site_t g_d;
static char g_html[8192];
static char g_o[1400];

#define PL "@property --p { syntax: '<length>'; inherits: true; initial-value: 0px } "
#define PN "@property --p { syntax: '<length>'; inherits: false; initial-value: 0px } "
#define PC "@property --p { syntax: '<color>'; inherits: false; initial-value: black } "
#define KF "@keyframes k { from { --p: 0px } to { --p: 100px } } "
#define KJ "@keyframes j { from { --p: 0px } to { --p: 10px } } "
#define ST "@starting-style { #o { --p: 0px } } "

static void prep(const char *css, int ms) {
    wf_site_set_time(ms);
    snprintf(g_html, sizeof(g_html), "<html><head><style>%s</style></head><body>"
             "<div id=o><p id=i>i</p></div><p id=q>q</p></body></html>", css);
    TEST_ASSERT_EQUAL(0, wf_site_prepare_ext(&g_d, g_html, (int)strlen(g_html), 640, 480, 0, 0));
    wf_site_set_time(0);
}

static const char *nv(const char *id) {
    int i;
    for (i = 0; i < g_d.nnodes; i++)
        if (!strcmp(g_d.node[i].id, id)) {
            const char *v = wf_site_node_var(&g_d, i, "--p");
            return v ? v : "(none)";
        }
    return "(no node)";
}

static const char *anim_text(void) {
    if (!wf_site_anim_text(&g_d, g_o, (int)sizeof(g_o))) return "(empty)";
    return g_o;
}

/* the inked rows of a glyph of the engine font */
static int inked_rows(int ch) {
    int r, n = 0;
    for (r = 0; r < 8; r++) if (gfx_font8[ch - 32][r]) n++;
    return n;
}

/* ex is the x height of the font the engine draws (the inked rows of 'x'
 * in an 8 px tall glyph for a 16 px font), ch the advance of '0' */
static void test_l98_font_metrics(void) {
    TEST_ASSERT_EQUAL(5, inked_rows('x'));
    TEST_ASSERT_EQUAL(inked_rows('x'), WF_CSSPROP_EX_NUM);
    TEST_ASSERT_EQUAL(inked_rows('z'), WF_CSSPROP_EX_NUM);
    TEST_ASSERT_EQUAL(WE_FONT_PX_DEFAULT, WF_CSSPROP_EX_DEN);
    TEST_ASSERT_EQUAL(WE_CHAR_W, WF_CSSPROP_CH_NUM);
    TEST_ASSERT_EQUAL(WE_FONT_PX_DEFAULT, WF_CSSPROP_CH_DEN);
    TEST_ASSERT_TRUE(8 * WF_CSSPROP_EX_NUM < 8 * WF_CSSPROP_EX_DEN / 2);   /* not the old half em */
    prep(PL "#o { font-size: 16px; --p: 1ex }", 0);
    TEST_ASSERT_EQUAL_STRING("5px", nv("o"));
    TEST_ASSERT_EQUAL_STRING("5px", nv("i"));
    prep(PL "#o { font-size: 16px; --p: 2ch }", 0);
    TEST_ASSERT_EQUAL_STRING("16px", nv("o"));
    prep(PL "#o { font-size: 32px; --p: calc(2ex + 1ch) }", 0);
    TEST_ASSERT_EQUAL_STRING("36px", nv("o"));
    prep(PL "#o { font-size: 10px; --p: 0.0001ex }", 0);
    TEST_ASSERT_EQUAL_STRING("0.0003px", nv("o"));
    prep(PL "#o { font-size: 10px; --p: 0.0001ch }", 0);
    TEST_ASSERT_EQUAL_STRING("0.0005px", nv("o"));
    prep(PL "#o { font-size: 10px; --p: -0.0001ex }", 0);
    TEST_ASSERT_EQUAL_STRING("-0.0003px", nv("o"));
    TEST_ASSERT_EQUAL_STRING("keyframes 0 animated 0 transitioned 0 font 1 at 0ms", anim_text());
}

/* hsl() through the reader: a transition between two hsl() colours */
static void test_l98_hsl(void) {
    char o[64];
    static wf_cssprop_t p;
    prep(PC "#o { --p: hsl(240, 100%, 50%); transition: --p 1000ms linear } @starting-style { #o { --p: hsl(0 100% 50%) } }", 500);
    TEST_ASSERT_EQUAL_STRING("rgb(128, 0, 128)", nv("o"));
    TEST_ASSERT_EQUAL(1, g_d.prop_transitioned);
    prep(PC "#o { --p: hsla(120, 100%, 25%, 0.5); transition: --p 1000ms linear } @starting-style { #o { --p: hsl(120deg 100% 25%) } }", 0);
    TEST_ASSERT_EQUAL_STRING("hsl(120deg 100% 25%)", nv("o"));
    prep(PC "#o { --p: hsla(120, 100%, 25%, 0.5); transition: --p 1000ms linear } @starting-style { #o { --p: hsl(120deg 100% 25%) } }", 500);
    TEST_ASSERT_EQUAL_STRING("rgba(0, 128, 0, 0.75)", nv("o"));
    /* the six hue sectors and the hue wrapping */
    p.syntax = (unsigned char)wf_cssprop_syntax_parse("'<color>'", 9, &p);
    TEST_ASSERT_EQUAL(2, wf_cssprop_interp(&p, "hsl(30, 100%, 50%)", "hsl(30, 100%, 50%)", 5000, o, (int)sizeof(o)));
    TEST_ASSERT_EQUAL_STRING("rgb(255, 128, 0)", o);
    TEST_ASSERT_EQUAL(2, wf_cssprop_interp(&p, "hsl(90, 100%, 50%)", "hsl(150, 100%, 50%)", 5000, o, (int)sizeof(o)));
    TEST_ASSERT_EQUAL_STRING("rgb(64, 255, 64)", o);
    TEST_ASSERT_EQUAL(2, wf_cssprop_interp(&p, "hsl(210, 100%, 50%)", "hsl(270, 100%, 50%)", 5000, o, (int)sizeof(o)));
    TEST_ASSERT_EQUAL_STRING("rgb(64, 64, 255)", o);
    TEST_ASSERT_EQUAL(2, wf_cssprop_interp(&p, "hsl(330, 100%, 50%)", "hsl(-30deg 100% 50%)", 5000, o, (int)sizeof(o)));
    TEST_ASSERT_EQUAL_STRING("rgb(255, 0, 128)", o);
    TEST_ASSERT_EQUAL(2, wf_cssprop_interp(&p, "hsl(720, 100%, 50%)", "hsl(-360, 100%, 50%)", 5000, o, (int)sizeof(o)));
    TEST_ASSERT_EQUAL_STRING("rgb(255, 0, 0)", o);
    /* lightness past one half, grey without saturation, clamps */
    TEST_ASSERT_EQUAL(2, wf_cssprop_interp(&p, "hsl(0, 100%, 75%)", "hsl(0, 0%, 50%)", 5000, o, (int)sizeof(o)));
    TEST_ASSERT_EQUAL_STRING("rgb(191, 128, 128)", o);
    TEST_ASSERT_EQUAL(2, wf_cssprop_interp(&p, "hsl(0 150% -10%)", "hsl(0 100 110 / 150%)", 5000, o, (int)sizeof(o)));
    TEST_ASSERT_EQUAL_STRING("rgb(128, 128, 128)", o);
    /* refused forms stay discrete */
    TEST_ASSERT_EQUAL(1, wf_cssprop_interp(&p, "hsl(0, 100, 50%)", "red", 5000, o, (int)sizeof(o)));
    TEST_ASSERT_EQUAL(1, wf_cssprop_interp(&p, "hsl(0 100% 50% 1)", "red", 5000, o, (int)sizeof(o)));
    TEST_ASSERT_EQUAL(1, wf_cssprop_interp(&p, "hsl(0%, 100%, 50%)", "red", 5000, o, (int)sizeof(o)));
    TEST_ASSERT_EQUAL(1, wf_cssprop_interp(&p, "hsl(0, 100%)", "red", 5000, o, (int)sizeof(o)));
    TEST_ASSERT_EQUAL(1, wf_cssprop_interp(&p, "hsl(0, 100%, 50%, 1, 1)", "red", 5000, o, (int)sizeof(o)));
    TEST_ASSERT_EQUAL(1, wf_cssprop_interp(&p, "hsl(0, 100%, 50%, x)", "red", 5000, o, (int)sizeof(o)));
    TEST_ASSERT_EQUAL(1, wf_cssprop_interp(&p, "hsl(0, 100%, 50%, 0.5deg)", "red", 5000, o, (int)sizeof(o)));
    TEST_ASSERT_EQUAL(1, wf_cssprop_interp(&p, "hsl(0, 100%, 1e)", "red", 5000, o, (int)sizeof(o)));
    TEST_ASSERT_EQUAL(1, wf_cssprop_interp(&p, "hsl(0dege, 100%, 50%)", "red", 5000, o, (int)sizeof(o)));
    TEST_ASSERT_EQUAL(1, wf_cssprop_interp(&p, "hsl(10-5, 100%, 50%)", "red", 5000, o, (int)sizeof(o)));
    TEST_ASSERT_EQUAL(1, wf_cssprop_interp(&p, "hsl(1.5.5 100% 50%)", "red", 5000, o, (int)sizeof(o)));
    TEST_ASSERT_EQUAL(1, wf_cssprop_interp(&p, "hsl(10000000000, 100%, 50%)", "red", 5000, o, (int)sizeof(o)));
    /* an argument longer than 31 bytes is refused (as for rgb()), even
     * right after a hue that parses */
    TEST_ASSERT_EQUAL(2, wf_cssprop_interp(&p, "hsl(120, 100%, 50%)", "hsl(120, 100%, 50%)", 5000, o, (int)sizeof(o)));
    TEST_ASSERT_EQUAL(1, wf_cssprop_interp(&p, "hsl(0.000000000000000000000000000000001, 100%, 50%)", "red", 5000, o,
                                           (int)sizeof(o)));
    TEST_ASSERT_EQUAL(1, wf_cssprop_interp(&p, "hsl(120, 100%, 50%)", "hsl(0.000000000000000000000000000000001, 100%, 50%)",
                                           5000, o, (int)sizeof(o)));
    /* the legacy form is told by its first separator, after spaces */
    TEST_ASSERT_EQUAL(1, wf_cssprop_interp(&p, "hsl(120 , 100, 50%)", "red", 5000, o, (int)sizeof(o)));
    TEST_ASSERT_EQUAL(2, wf_cssprop_interp(&p, "hsl(120 , 100%, 50%)", "hsl(120 , 100%, 50%)", 5000, o, (int)sizeof(o)));
    TEST_ASSERT_EQUAL_STRING("rgb(0, 255, 0)", o);
    /* five arguments (the bound of the argument table) */
    TEST_ASSERT_EQUAL(1, wf_cssprop_interp(&p, "rgb(1, 2, 3, 0.5, 1)", "red", 5000, o, (int)sizeof(o)));
    TEST_ASSERT_EQUAL(1, wf_cssprop_interp(&p, "hsl(1 2% 3% / 0.5 1)", "red", 5000, o, (int)sizeof(o)));
    TEST_ASSERT_EQUAL(1, wf_cssprop_interp(&p, "rgb(1, , 3)", "red", 5000, o, (int)sizeof(o)));
    TEST_ASSERT_EQUAL(1, wf_cssprop_interp(&p, "rgb(, 2, 3)", "red", 5000, o, (int)sizeof(o)));
    TEST_ASSERT_EQUAL(1, wf_cssprop_interp(&p, "hsl(0123456789012345678901234567890123, 100%, 50%)", "red", 5000, o,
                                           (int)sizeof(o)));
    TEST_ASSERT_EQUAL(1, wf_cssprop_interp(&p, "hsl(0, 0123456789012345678901234567890123%, 50%)", "red", 5000, o,
                                           (int)sizeof(o)));
    TEST_ASSERT_EQUAL(1, wf_cssprop_interp(&p, "hsl(0, 100%, 50%)x", "red", 5000, o, (int)sizeof(o)));
    TEST_ASSERT_EQUAL(1, wf_cssprop_interp(&p, "hsl()", "red", 5000, o, (int)sizeof(o)));
    TEST_ASSERT_EQUAL(1, wf_cssprop_interp(&p, "hsla()", "red", 5000, o, (int)sizeof(o)));
}

static void test_l98_add_api(void) {
    char o[64];
    static wf_cssprop_t p, q;
    p.syntax = (unsigned char)wf_cssprop_syntax_parse("'<length>'", 10, &p);
    q.syntax = (unsigned char)wf_cssprop_syntax_parse("'<color>'", 9, &q);
    TEST_ASSERT_EQUAL(2, wf_cssprop_add(&p, "10px", "5px", o, (int)sizeof(o)));
    TEST_ASSERT_EQUAL_STRING("15px", o);
    /* the sum is not the endpoint at t = 10000 */
    TEST_ASSERT_EQUAL(2, wf_cssprop_add(&p, "0px", "5px", o, (int)sizeof(o)));
    TEST_ASSERT_EQUAL_STRING("5px", o);
    TEST_ASSERT_EQUAL(2, wf_cssprop_add(&p, "-5px", "5px", o, (int)sizeof(o)));
    TEST_ASSERT_EQUAL_STRING("0px", o);
    TEST_ASSERT_EQUAL(0, wf_cssprop_add(&p, "10px", "5px", o, 3));
    TEST_ASSERT_EQUAL_STRING("", o);
    TEST_ASSERT_EQUAL(0, wf_cssprop_add(&p, "10px", "5px", o, 0));
    TEST_ASSERT_EQUAL(0, wf_cssprop_add(&p, 0, "5px", o, (int)sizeof(o)));
    TEST_ASSERT_EQUAL(0, wf_cssprop_add(&p, "5px", 0, o, (int)sizeof(o)));
    TEST_ASSERT_EQUAL(0, wf_cssprop_add(&p, "5px", "5px", 0, 8));
    /* '*' and a value of another type replace */
    TEST_ASSERT_EQUAL(1, wf_cssprop_add(0, "10px", "5px", o, (int)sizeof(o)));
    TEST_ASSERT_EQUAL_STRING("5px", o);
    TEST_ASSERT_EQUAL(1, wf_cssprop_add(&q, "red", "currentcolor", o, (int)sizeof(o)));
    TEST_ASSERT_EQUAL_STRING("currentcolor", o);
    TEST_ASSERT_EQUAL(0, wf_cssprop_add(&q, "red", "currentcolor", o, 5));
    /* colours: the alpha clamps to 1, the channels to 255 */
    TEST_ASSERT_EQUAL(2, wf_cssprop_add(&q, "rgb(200, 0, 0)", "rgba(200, 0, 100, 0.6)", o, (int)sizeof(o)));
    TEST_ASSERT_EQUAL_STRING("rgb(255, 0, 60)", o);
    TEST_ASSERT_EQUAL(2, wf_cssprop_add(&q, "rgba(200, 0, 0, 0.25)", "rgba(0, 0, 100, 0.25)", o, (int)sizeof(o)));
    TEST_ASSERT_EQUAL_STRING("rgba(100, 0, 50, 0.5)", o);
    TEST_ASSERT_EQUAL(2, wf_cssprop_add(&q, "transparent", "transparent", o, (int)sizeof(o)));
    TEST_ASSERT_EQUAL_STRING("rgba(0, 0, 0, 0)", o);
    TEST_ASSERT_EQUAL(2, wf_cssprop_add(&q, "hsl(0, 100%, 25%)", "hsl(240, 100%, 25%)", o, (int)sizeof(o)));
    TEST_ASSERT_EQUAL_STRING("rgb(128, 0, 128)", o);
}

static void test_l98_long_api(void) {
    static wf_cssanim_t a[WF_CSSANIM_MAX + 2];
    static wf_csstrans_t t[WF_CSSANIM_MAX + 2];
    const char *v[WF_CSSANIM_LONGS] = {0};
    unsigned char sh[WF_CSSANIM_LONGS] = {0};
    /* Lot 99, converted: animation-timeline is the tenth (was 9) */
    TEST_ASSERT_EQUAL(10, WF_CSSANIM_LONGS);
    TEST_ASSERT_EQUAL(5, WF_CSSTRANS_LONGS);
    TEST_ASSERT_EQUAL(-1, wf_cssanim_long(WF_CSSANIM_L_NAME, 0, a, 4));
    TEST_ASSERT_EQUAL(-1, wf_cssanim_long(WF_CSSANIM_L_NAME, "k", 0, 4));
    TEST_ASSERT_EQUAL(-1, wf_cssanim_long(WF_CSSANIM_L_NAME, "k", a, 0));
    TEST_ASSERT_EQUAL(1, wf_cssanim_long(WF_CSSANIM_L_NAME, "k", a, 1));
    TEST_ASSERT_EQUAL(-1, wf_cssanim_long(-1, "add", a, 4));
    TEST_ASSERT_EQUAL(-1, wf_cssanim_long(WF_CSSANIM_LONGS, "add", a, 4));
    /* Lot 99, converted: the last longhand is animation-timeline (was
     * animation-composition, WF_CSSANIM_LONGS - 1 "add" gave 1) */
    TEST_ASSERT_EQUAL(1, wf_cssanim_long(WF_CSSANIM_L_COMPOSITION, "add", a, 4));
    TEST_ASSERT_EQUAL(-1, wf_cssanim_long(WF_CSSANIM_LONGS - 1, "add", a, 4));
    TEST_ASSERT_EQUAL(1, wf_cssanim_long(WF_CSSANIM_LONGS - 1, "none", a, 4));
    TEST_ASSERT_EQUAL(-1, wf_cssanim_long(WF_CSSANIM_L_NAME, "a, b, c", a, 2));
    TEST_ASSERT_EQUAL(4, wf_cssanim_long(WF_CSSANIM_L_NAME, "a, b, c, d", a, WF_CSSANIM_MAX + 2));
    TEST_ASSERT_EQUAL(-1, wf_cssanim_long(WF_CSSANIM_L_NAME, "a, b, c, d, e", a, WF_CSSANIM_MAX + 2));
    TEST_ASSERT_EQUAL(-1, wf_csstrans_long(WF_CSSTRANS_L_PROPERTY, 0, t, 4));
    TEST_ASSERT_EQUAL(-1, wf_csstrans_long(WF_CSSTRANS_L_PROPERTY, "all", 0, 4));
    TEST_ASSERT_EQUAL(-1, wf_csstrans_long(WF_CSSTRANS_L_PROPERTY, "all", t, 0));
    TEST_ASSERT_EQUAL(1, wf_csstrans_long(WF_CSSTRANS_L_PROPERTY, "all", t, 1));
    /* the bounds of one item: a 31-byte property, a count of 100000 */
    TEST_ASSERT_EQUAL(1, wf_csstrans_long(WF_CSSTRANS_L_PROPERTY, "aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa", t, 4));
    TEST_ASSERT_EQUAL(-1, wf_csstrans_long(WF_CSSTRANS_L_PROPERTY, "aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa", t, 4));
    TEST_ASSERT_EQUAL(1, wf_cssanim_long(WF_CSSANIM_L_COUNT, "100000", a, 4));
    TEST_ASSERT_EQUAL(100000 * 10000, a[0].iters);
    TEST_ASSERT_EQUAL(-1, wf_cssanim_long(WF_CSSANIM_L_COUNT, "100000.0001", a, 4));
    TEST_ASSERT_EQUAL(-1, wf_csstrans_long(-1, "normal", t, 4));
    TEST_ASSERT_EQUAL(-1, wf_csstrans_long(WF_CSSTRANS_LONGS, "normal", t, 4));
    TEST_ASSERT_EQUAL(1, wf_csstrans_long(WF_CSSTRANS_LONGS - 1, "normal", t, 4));
    TEST_ASSERT_EQUAL(-1, wf_csstrans_long(WF_CSSTRANS_L_DURATION, "1s, 2s, 3s", t, 2));
    TEST_ASSERT_EQUAL(4, wf_csstrans_long(WF_CSSTRANS_L_DURATION, "1s, 2s, 3s, 4s", t, WF_CSSANIM_MAX + 2));
    TEST_ASSERT_EQUAL(-1, wf_csstrans_long(WF_CSSTRANS_L_DURATION, "1s, 2s, 3s, 4s, 5s", t, WF_CSSANIM_MAX + 2));
    /* coordination guards; a list longer than max counts as not set */
    TEST_ASSERT_EQUAL(0, wf_cssanim_coord(0, sh, a, 4));
    TEST_ASSERT_EQUAL(0, wf_cssanim_coord(v, 0, a, 4));
    TEST_ASSERT_EQUAL(0, wf_cssanim_coord(v, sh, 0, 4));
    TEST_ASSERT_EQUAL(0, wf_cssanim_coord(v, sh, a, 0));
    TEST_ASSERT_EQUAL(0, wf_csstrans_coord(0, sh, t, 4));
    TEST_ASSERT_EQUAL(0, wf_csstrans_coord(v, 0, t, 4));
    TEST_ASSERT_EQUAL(0, wf_csstrans_coord(v, sh, 0, 4));
    TEST_ASSERT_EQUAL(0, wf_csstrans_coord(v, sh, t, 0));
    v[WF_CSSANIM_L_NAME] = "a, b";
    TEST_ASSERT_EQUAL(1, wf_cssanim_coord(v, sh, a, 1));
    TEST_ASSERT_EQUAL_STRING("", a[0].name);
    TEST_ASSERT_EQUAL(2, wf_cssanim_coord(v, sh, a, 2));
    TEST_ASSERT_EQUAL_STRING("b", a[1].name);
    TEST_ASSERT_EQUAL(10000, a[1].iters);
    TEST_ASSERT_EQUAL(WF_CSSANIM_REPLACE, a[1].comp);
    v[WF_CSSANIM_L_NAME] = 0;
    v[WF_CSSTRANS_L_DURATION] = "1s, 2s";
    TEST_ASSERT_EQUAL(1, wf_csstrans_coord(v, sh, t, 1));
    TEST_ASSERT_EQUAL_STRING("all", t[0].prop);
    TEST_ASSERT_EQUAL(0, t[0].dur);
    TEST_ASSERT_EQUAL(1, wf_csstrans_coord(v, sh, t, 2));
    TEST_ASSERT_EQUAL(1000, t[0].dur);
    TEST_ASSERT_EQUAL(0, t[0].discrete);
}

/* the longhand cascade on the reader sheet */
static void test_l98_longhands(void) {
    prep(PN KF "#o { animation-name: k; animation-duration: 1000ms; animation-timing-function: linear }", 300);
    TEST_ASSERT_EQUAL_STRING("30px", nv("o"));
    TEST_ASSERT_EQUAL_STRING("keyframes 1 animated 1 transitioned 0 font 0 at 300ms", anim_text());
    /* a longhand after the shorthand changes one field */
    prep(PN KF "#o { animation: k 1000ms linear; animation-duration: 2000ms }", 500);
    TEST_ASSERT_EQUAL_STRING("25px", nv("o"));
    /* the shorthand after a longhand sets it again */
    prep(PN KF "#o { animation-duration: 2000ms; animation: k 1000ms linear }", 500);
    TEST_ASSERT_EQUAL_STRING("50px", nv("o"));
    /* a stronger longhand against a later, weaker shorthand */
    prep(PN KF "#o { animation-duration: 2000ms } div { animation: k 1000ms linear }", 500);
    TEST_ASSERT_EQUAL_STRING("25px", nv("o"));
    prep(PN KF "div { animation-duration: 2000ms } #o { animation: k 1000ms linear }", 500);
    TEST_ASSERT_EQUAL_STRING("50px", nv("o"));
    /* the names give the count, the other lists repeat */
    prep(PN KF KJ "#o { animation-name: j, k; animation-duration: 1000ms; animation-timing-function: linear }", 500);
    TEST_ASSERT_EQUAL_STRING("50px", nv("o"));
    prep(PN KF KJ "#o { animation-name: k, j; animation-duration: 1000ms, 2000ms; animation-timing-function: linear }", 500);
    TEST_ASSERT_EQUAL_STRING("2.5px", nv("o"));
    prep(PN KF "#o { animation-name: k; animation-duration: 1000ms, 2000ms, 4000ms; animation-timing-function: linear }", 500);
    TEST_ASSERT_EQUAL_STRING("50px", nv("o"));
    /* every longhand */
    prep(PN KF "#o { animation-name: k; animation-duration: 1s; animation-timing-function: steps(4); animation-delay: -250ms;"
         " animation-iteration-count: 2; animation-direction: reverse; animation-fill-mode: forwards;"
         " animation-play-state: running }", 2500);
    TEST_ASSERT_EQUAL_STRING("0px", nv("o"));
    prep(PN KF "#o { animation-name: k; animation-duration: 1s; animation-timing-function: linear; animation-delay: 250ms;"
         " animation-fill-mode: backwards; animation-direction: alternate }", 100);
    TEST_ASSERT_EQUAL_STRING("0px", nv("o"));
    TEST_ASSERT_EQUAL(1, g_d.prop_animated);
    prep(PN KF "#o { animation-name: k; animation-duration: 1s; animation-timing-function: linear; animation-play-state: paused;"
         " animation-delay: -400ms }", 900);
    TEST_ASSERT_EQUAL_STRING("40px", nv("o"));
    prep(PN KF "#o { animation-name: k; animation-duration: 1s; animation-timing-function: linear; animation-iteration-count: infinite }", 2300);
    TEST_ASSERT_EQUAL_STRING("30px", nv("o"));
    /* initial and unset reset one longhand; inherit and invalid lists
     * are dropped */
    prep(PN KF "#o { animation: k 1000ms linear; animation-duration: initial }", 500);
    TEST_ASSERT_EQUAL_STRING("0px", nv("o"));
    TEST_ASSERT_EQUAL(0, g_d.prop_animated);
    prep(PN KF "#o { animation: k 1000ms linear; animation-duration: unset }", 500);
    TEST_ASSERT_EQUAL(0, g_d.prop_animated);
    prep(PN KF "#o { animation: k 1000ms linear; animation-duration: inherit }", 500);
    TEST_ASSERT_EQUAL_STRING("50px", nv("o"));
    prep(PN KF "#o { animation: k 1000ms linear; animation-duration: -1s }", 500);
    TEST_ASSERT_EQUAL_STRING("50px", nv("o"));
    prep(PN KF "#o { animation: k 1000ms linear; animation-duration: 2s,, 3s; animation-name: k j }", 500);
    TEST_ASSERT_EQUAL_STRING("50px", nv("o"));
    prep(PN KF "#o { animation: k 1000ms linear; ANIMATION-DURATION: 2S }", 500);
    TEST_ASSERT_EQUAL_STRING("25px", nv("o"));
    /* animation: none resets the eight */
    prep(PN KF "#o { animation-name: k; animation-duration: 1s; animation: none; animation-name: k }", 500);
    TEST_ASSERT_EQUAL(0, g_d.prop_animated);
    /* a longhand alone shows the summary line */
    prep(PN "#o { animation-play-state: paused }", 0);
    TEST_ASSERT_EQUAL_STRING("keyframes 0 animated 0 transitioned 0 font 0 at 0ms", anim_text());
    prep(PN "#o { transition-behavior: normal }", 0);
    TEST_ASSERT_EQUAL_STRING("keyframes 0 animated 0 transitioned 0 font 0 at 0ms", anim_text());
    prep(PN "#o { --p: 1px }", 0);
    TEST_ASSERT_EQUAL_STRING("(empty)", anim_text());
}

static void test_l98_transition_longhands(void) {
    prep(PN "#o { --p: 100px; transition-property: --p; transition-duration: 1000ms; transition-timing-function: linear } " ST, 250);
    TEST_ASSERT_EQUAL_STRING("25px", nv("o"));
    TEST_ASSERT_EQUAL_STRING("keyframes 0 animated 0 transitioned 1 font 0 at 250ms", anim_text());
    /* all by default; the delay; the last matching item */
    prep(PN "#o { --p: 100px; transition-duration: 1000ms; transition-timing-function: linear; transition-delay: 500ms } " ST, 750);
    TEST_ASSERT_EQUAL_STRING("25px", nv("o"));
    prep(PN "#o { --p: 100px; transition-property: --p, --x, --p; transition-duration: 4s, 1s, 2s;"
         " transition-timing-function: linear } " ST, 500);
    TEST_ASSERT_EQUAL_STRING("25px", nv("o"));
    /* the durations repeat: the fourth property takes the second */
    prep(PN "#o { --p: 100px; transition-property: --a, --b, --c, --p; transition-duration: 1000ms, 4000ms;"
         " transition-timing-function: linear } " ST, 1000);
    TEST_ASSERT_EQUAL_STRING("25px", nv("o"));
    /* transition: none keeps its property none under a later duration */
    prep(PN "#o { --p: 100px; transition: none; transition-duration: 1000ms } " ST, 500);
    TEST_ASSERT_EQUAL_STRING("100px", nv("o"));
    TEST_ASSERT_EQUAL(0, g_d.prop_transitioned);
    prep(PN "#o { --p: 100px; transition: initial; transition-duration: 1000ms; transition-timing-function: linear } " ST, 500);
    TEST_ASSERT_EQUAL_STRING("50px", nv("o"));
    prep(PN "#o { --p: 100px; transition-property: none; transition-duration: 1000ms } " ST, 500);
    TEST_ASSERT_EQUAL(0, g_d.prop_transitioned);
    prep(PN "#o { --p: 100px; transition-property: none, --p; transition-duration: 1000ms } " ST, 500);
    TEST_ASSERT_EQUAL(1, g_d.prop_transitioned);
    /* the shorthand sets the behavior too, the longhand changes it */
    prep("@property --p { syntax: '<custom-ident>'; inherits: false; initial-value: a } "
         "#o { --p: b; transition: --p 1000ms allow-discrete; transition-behavior: normal } @starting-style { #o { --p: c } }", 200);
    TEST_ASSERT_EQUAL_STRING("b", nv("o"));
    prep("@property --p { syntax: '<custom-ident>'; inherits: false; initial-value: a } "
         "#o { --p: b; transition: --p 1000ms; transition-behavior: allow-discrete } @starting-style { #o { --p: c } }", 200);
    TEST_ASSERT_EQUAL_STRING("c", nv("o"));
    prep("@property --p { syntax: '<custom-ident>'; inherits: false; initial-value: a } "
         "#o { --p: b; transition-behavior: allow-discrete; transition: --p 1000ms } @starting-style { #o { --p: c } }", 200);
    TEST_ASSERT_EQUAL_STRING("b", nv("o"));
}

static void test_l98_composition(void) {
    prep(PN KF "#o { --p: 10px; animation: k 1000ms linear; animation-composition: add }", 500);
    TEST_ASSERT_EQUAL_STRING("60px", nv("o"));
    TEST_ASSERT_EQUAL_STRING("keyframes 1 animated 1 transitioned 0 font 0 at 500ms composed 1", anim_text());
    TEST_ASSERT_EQUAL(1, g_d.prop_composed);
    prep(PN KF "#o { --p: 10px; animation: k 1000ms linear; animation-composition: accumulate }", 500);
    TEST_ASSERT_EQUAL_STRING("60px", nv("o"));
    prep(PN KF "#o { --p: 10px; animation: k 1000ms linear; animation-composition: replace }", 500);
    TEST_ASSERT_EQUAL_STRING("50px", nv("o"));
    TEST_ASSERT_EQUAL(0, g_d.prop_composed);
    TEST_ASSERT_EQUAL_STRING("keyframes 1 animated 1 transitioned 0 font 0 at 500ms", anim_text());
    /* the shorthand does not reset the composition, in either order */
    prep(PN KF "#o { --p: 10px; animation-composition: add; animation: k 1000ms linear }", 500);
    TEST_ASSERT_EQUAL_STRING("60px", nv("o"));
    /* Lot 99, converted (was "filled ends stay the value below", 70px):
     * the generated 0% keyframe is the element's 20px added on the 20px
     * below (40px), 50% is 120px; at 250ms halfway: 80px (CSS Animations
     * 2, generate initial and final frames) */
    prep(PN "@keyframes k { 50% { --p: 100px } } #o { --p: 20px; animation: k 1000ms linear; animation-composition: add }", 250);
    TEST_ASSERT_EQUAL_STRING("80px", nv("o"));
    /* a keyframe's own composition */
    prep(PN "@keyframes k { from { --p: 0px; animation-composition: replace } to { --p: 100px } } "
         "#o { --p: 10px; animation: k 1000ms linear; animation-composition: add }", 500);
    TEST_ASSERT_EQUAL_STRING("55px", nv("o"));
    prep(PN "@keyframes k { from { --p: 0px } to { --p: 100px; animation-composition: add } } "
         "#o { --p: 10px; animation: k 1000ms linear }", 500);
    TEST_ASSERT_EQUAL_STRING("55px", nv("o"));
    TEST_ASSERT_EQUAL(1, g_d.prop_composed);
    /* written out of order: the composition moves with its frame (after
     * the case above, a stale slot 1 would say add) */
    prep(PN "@keyframes k { to { --p: 100px; animation-composition: replace } from { --p: 0px } } "
         "#o { --p: 10px; animation: k 1000ms linear; animation-composition: add }", 500);
    TEST_ASSERT_EQUAL_STRING("55px", nv("o"));
    prep(PN "@keyframes k { from { --p: 0px } to { --p: 100px; animation-composition: bogus } } "
         "#o { --p: 10px; animation: k 1000ms linear }", 500);
    TEST_ASSERT_EQUAL_STRING("50px", nv("o"));
    prep(PN "@keyframes k { from { --p: 0px } to { --p: 100px; animation-composition: add, add } } "
         "#o { --p: 10px; animation: k 1000ms linear }", 500);
    TEST_ASSERT_EQUAL_STRING("50px", nv("o"));
    /* the timing function next to it in a keyframe still counts */
    prep(PN "@keyframes k { from { --p: 0px; animation-timing-function: step-end; animation-composition: add } to { --p: 100px } } "
         "#o { --p: 10px; animation: k 1000ms linear }", 500);
    TEST_ASSERT_EQUAL_STRING("10px", nv("o"));
    /* stacked: the second adds on the first's value */
    prep(PN KF KJ "#o { --p: 10px; animation: k 1000ms linear, j 1000ms linear; animation-composition: replace, add }", 500);
    TEST_ASSERT_EQUAL_STRING("55px", nv("o"));
    TEST_ASSERT_EQUAL(1, g_d.prop_composed);
    prep(PN KF KJ "#o { --p: 10px; animation: k 1000ms linear, j 1000ms linear; animation-composition: add, replace }", 500);
    TEST_ASSERT_EQUAL_STRING("5px", nv("o"));
    TEST_ASSERT_EQUAL(1, g_d.prop_composed);
    /* not additive: replaced, not counted */
    prep("@property --p { syntax: '<custom-ident>'; inherits: false; initial-value: a } "
         "@keyframes k { from { --p: b } to { --p: c } } #o { animation: k 1000ms linear; animation-composition: add }", 600);
    TEST_ASSERT_EQUAL_STRING("c", nv("o"));
    TEST_ASSERT_EQUAL(0, g_d.prop_composed);
    /* an inactive animation composes nothing */
    prep(PN KF "#o { --p: 10px; animation: k 1000ms linear; animation-composition: add }", 1500);
    TEST_ASSERT_EQUAL_STRING("10px", nv("o"));
    TEST_ASSERT_EQUAL(0, g_d.prop_composed);
    /* colours */
    prep(PC "@keyframes k { from { --p: rgb(0, 0, 100) } to { --p: rgb(0, 0, 100) } } "
         "#o { --p: rgb(100, 0, 0); animation: k 1000ms linear; animation-composition: add }", 500);
    TEST_ASSERT_EQUAL_STRING("rgb(100, 0, 100)", nv("o"));
    /* !important keeps its value */
    prep(PN KF "#o { --p: 10px !important; animation: k 1000ms linear; animation-composition: add }", 500);
    TEST_ASSERT_EQUAL_STRING("10px", nv("o"));
    TEST_ASSERT_EQUAL(0, g_d.prop_composed);
    /* two elements count twice */
    prep(PN KF "#o, #q { --p: 10px; animation: k 1000ms linear; animation-composition: add }", 500);
    TEST_ASSERT_EQUAL(2, g_d.prop_composed);
    TEST_ASSERT_EQUAL_STRING("keyframes 1 animated 2 transitioned 0 font 0 at 500ms composed 2", anim_text());
}

static void test_l98_scope(void) {
    prep(PN "@scope (#o) { " KF "} #o { animation: k 1000ms linear }", 500);
    TEST_ASSERT_EQUAL(1, g_d.nkf);
    TEST_ASSERT_EQUAL_STRING("50px", nv("o"));
    /* names are global: the scope root does not limit them */
    prep(PN "@scope (#q) { " KF "} #o { animation: k 1000ms linear }", 500);
    TEST_ASSERT_EQUAL_STRING("50px", nv("o"));
    prep(PN "@scope { @scope (div) { " KF "} } #o { animation: k 1000ms linear }", 500);
    TEST_ASSERT_EQUAL_STRING("50px", nv("o"));
    prep(PN "@scope (#o) { @media (min-width: 100px) { " KF "} } #o { animation: k 1000ms linear }", 500);
    TEST_ASSERT_EQUAL_STRING("50px", nv("o"));
    /* four levels deep at most */
    prep(PN "@scope { @scope { @scope { @scope { " KF "} } } } #o { animation: k 1000ms linear }", 500);
    TEST_ASSERT_EQUAL(1, g_d.nkf);
    prep(PN "@scope { @scope { @scope { @scope { @scope { " KF "} } } } } #o { animation: k 1000ms linear }", 500);
    TEST_ASSERT_EQUAL(0, g_d.nkf);
    prep(PN "@scope {} @scope (#o) { } #o { animation: k 1000ms linear }", 500);
    TEST_ASSERT_EQUAL(0, g_d.nkf);
    /* the later rule wins across a scope */
    prep(PN KF "@scope (#o) { @keyframes k { to { --p: 10px } } } #o { animation: k 1000ms linear }", 500);
    TEST_ASSERT_EQUAL_STRING("5px", nv("o"));
    /* @property is not taken inside @scope, even under @media */
    prep("@scope (#o) { @property --p { syntax: '<length>'; inherits: false; initial-value: 0px } }", 0);
    TEST_ASSERT_EQUAL(0, g_d.nprops);
    prep("@scope (#o) { @media (min-width: 100px) { @property --p { syntax: '<length>'; inherits: false; initial-value: 0px } } }", 0);
    TEST_ASSERT_EQUAL(0, g_d.nprops);
    prep("@media (min-width: 100px) { @property --p { syntax: '<length>'; inherits: false; initial-value: 0px } }", 0);
    TEST_ASSERT_EQUAL(1, g_d.nprops);
    prep("@scopex { @property --p { syntax: '<length>'; inherits: false; initial-value: 0px } " KF "}", 0);
    TEST_ASSERT_EQUAL(0, g_d.nkf);
}

int main(void) {
    unity_init();
    RUN_TEST(test_l98_font_metrics);
    RUN_TEST(test_l98_hsl);
    RUN_TEST(test_l98_add_api);
    RUN_TEST(test_l98_long_api);
    RUN_TEST(test_l98_longhands);
    RUN_TEST(test_l98_transition_longhands);
    RUN_TEST(test_l98_composition);
    RUN_TEST(test_l98_scope);
    unity_print_results();
    unity_cleanup();
    return unity_stats.tests_failed == 0 ? 0 : 1;
}
