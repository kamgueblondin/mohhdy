/* Lot 99 (US-031): hand-written cases next to the generated suites
 * (test_lot99_color.c, test_lot99_accum.c, test_lot99_timeline.c,
 * test_lot99_sheet.c): hue units, hsl() painted by the reader, add and
 * accumulate on transforms, the generated 0% and 100% keyframes,
 * animation-timeline, the summary line and the API guards. */
#include "../../framework/unity.h"
#include <string.h>
#include <stdio.h>
#include "wf_sitecss.h"
#include "wf_cssprop.h"
#include "wf_cssbg.h"

static wf_site_t g_d;
static wf_cssprop_t g_p;
static char g_html[8192];
static char g_o[1400];

#define PN "@property --p { syntax: '<length>'; inherits: false; initial-value: 0px } "
#define PT "@property --p { syntax: '<transform-list>'; inherits: false; initial-value: rotate(0deg) } "
#define KF "@keyframes k { from { --p: 0px } to { --p: 100px } } "

static void prep(const char *css, int ms) {
    wf_site_set_time(ms);
    snprintf(g_html, sizeof(g_html), "<html><head><style>%s</style></head><body>"
             "<div id=o><p id=i>i</p></div><p id=q>q</p></body></html>", css);
    TEST_ASSERT_EQUAL(0, wf_site_prepare_ext(&g_d, g_html, (int)strlen(g_html), 640, 480, 0, 0));
    wf_site_set_time(0);
}

static int node(const char *id) {
    int i;
    for (i = 0; i < g_d.nnodes; i++) if (!strcmp(g_d.node[i].id, id)) return i;
    return -1;
}

static const char *nv(const char *id) {
    int i = node(id);
    const char *v;
    if (i < 0) return "(no node)";
    v = wf_site_node_var(&g_d, i, "--p");
    return v ? v : "(none)";
}

static const char *style_of(const char *id) {
    int i = node(id);
    g_o[0] = 0;
    if (i >= 0) wf_site_style_attr(&g_d, g_d.node[i].start, g_o, (int)sizeof(g_o));
    return g_o;
}

static const char *anim_text(void) {
    if (!wf_site_anim_text(&g_d, g_o, (int)sizeof(g_o))) return "(empty)";
    return g_o;
}

static const char *rg(const char *v, int n) {
    unsigned int c = 0x12345678u;
    if (!wf_cssprop_rgb24(v, n, &c)) return c == 0x12345678u ? "-" : "WRITTEN";
    snprintf(g_o, sizeof(g_o), "#%06x", c);
    return g_o;
}

static int syn(const char *s) {
    memset(&g_p, 0, sizeof(g_p));
    g_p.syntax = (unsigned char)wf_cssprop_syntax_parse(s, (int)strlen(s), &g_p);
    return g_p.syntax != 0;
}

/* add (op 1) or accumulate (op 2) with syntax s into cap bytes: the
 * text, the code checked */
static const char *comb(int op, const char *s, const char *a, const char *b, int cap, int code) {
    int r;
    static char out[256];
    if (!syn(s)) return "BAD SYNTAX";
    memset(out, 'z', sizeof(out));
    r = op == 1 ? wf_cssprop_add(&g_p, a, b, out, cap) : wf_cssprop_accumulate(&g_p, a, b, out, cap);
    if (r != code) return "BAD CODE";
    return out;
}

/* the hue units and the reader colour */
static void test_l99_hue_units(void) {
    TEST_ASSERT_EQUAL_STRING("#00ffff", rg("hsl(0.5turn 100% 50%)", 21));
    TEST_ASSERT_EQUAL_STRING("#00ffff", rg("hsl(200grad, 100%, 50%)", 23));
    TEST_ASSERT_EQUAL_STRING("#ff0000", rg("hsl(400grad 100% 50%)", 21));
    TEST_ASSERT_EQUAL_STRING("#8000ff", rg("hsl(-0.25turn 100% 50%)", 23));
    /* the reader's pi: 3.1416rad is 180.0004deg, 1.5708rad 90.0002deg
     * (red 127.4991, rounded down), 1rad 57.2958deg */
    TEST_ASSERT_EQUAL_STRING("#00ffff", rg("hsl(3.1416rad 100% 50%)", 23));
    TEST_ASSERT_EQUAL_STRING("#7fff00", rg("hsl(1.5708rad 100% 50%)", 23));
    TEST_ASSERT_EQUAL_STRING("#fff400", rg("hsl(1rad 100% 50%)", 18));
    TEST_ASSERT_EQUAL_STRING("#80ff00", rg("hsl(90deg 100% 50%)", 19));
    TEST_ASSERT_EQUAL_STRING("#80ff00", rg("hsl(90 100% 50%)", 16));
    TEST_ASSERT_EQUAL_STRING("#008000", rg("HSLA(120, 100%, 25%, 1)", 23));
    TEST_ASSERT_EQUAL_STRING("#00ffff", rg("hsl(0.5TURN 100% 50%)", 21));
    TEST_ASSERT_EQUAL_STRING("#00ffff", rg("  hsl(0.5turn 100% 50%)  ; x", 28));
    TEST_ASSERT_EQUAL_STRING("#00ffff", rg("hsl(0.5turn 100% 50%) !important", 32));
    TEST_ASSERT_EQUAL_STRING("#00ffff", rg("hsl(0.5turn 100% 50% / 2)", 25));      /* alpha clamped to 1 */
    TEST_ASSERT_EQUAL_STRING("#00ffff", rg("hsl(0.5turn 100% 50% / 100%)", 28));
    /* not taken: an alpha under 1, other colours, another unit, junk */
    TEST_ASSERT_EQUAL_STRING("-", rg("hsl(0.5turn 100% 50% / 0.9999)", 30));
    TEST_ASSERT_EQUAL_STRING("-", rg("hsla(0.5turn, 100%, 50%, 0)", 27));
    TEST_ASSERT_EQUAL_STRING("-", rg("rgb(1, 2, 3)", 12));
    TEST_ASSERT_EQUAL_STRING("-", rg("red", 3));
    TEST_ASSERT_EQUAL_STRING("-", rg("#00ffff", 7));
    TEST_ASSERT_EQUAL_STRING("-", rg("hsl(0.5turns 100% 50%)", 22));
    TEST_ASSERT_EQUAL_STRING("-", rg("hsl(0.5tur 100% 50%)", 20));
    TEST_ASSERT_EQUAL_STRING("-", rg("hsl(1s 100% 50%)", 16));
    TEST_ASSERT_EQUAL_STRING("-", rg("hsl(0.5turn 100% 50%) x", 23));
    TEST_ASSERT_EQUAL_STRING("-", rg("hsl(0.5turn 100% 50%", 20));
    TEST_ASSERT_EQUAL_STRING("-", rg("hsl(0.5turn 100% 50%)", 20));          /* n cuts the ')' */
    TEST_ASSERT_EQUAL_STRING("-", rg("hsl(0.5turn, 100, 50%)", 22));         /* commas need % */
    TEST_ASSERT_EQUAL_STRING("-", rg("rgb(0.5turn 100% 50%)", 21));
    TEST_ASSERT_EQUAL_STRING("-", rg("hsl(tan 100% 50%)", 17));
    TEST_ASSERT_EQUAL_STRING("-", rg("hsl(0.5turn 100% 50%)", 0));
    TEST_ASSERT_EQUAL_STRING("-", rg("hsl(0.5turn 100% 50%)", -1));
    TEST_ASSERT_EQUAL(0, wf_cssprop_rgb24(0, 3, 0));
    TEST_ASSERT_EQUAL(0, wf_cssprop_rgb24("hsl(0 0% 0%)", 12, 0));
    {
        /* a text of 128 bytes or more up to its ')' is not taken (the
         * spaces before it do not count) */
        static char big[200];
        unsigned int c = 7;
        memset(big, ' ', sizeof(big));
        memcpy(big, "hsl(0", 5);
        memcpy(big + 122, "0% 0%)", 6);
        TEST_ASSERT_EQUAL(0, wf_cssprop_rgb24(big, 128, &c));
        TEST_ASSERT_EQUAL(7, (int)c);
        memcpy(big + 5, "  ", 2);
        memcpy(big + 116, "0% 0%)      ", 12);
        TEST_ASSERT_EQUAL(1, wf_cssprop_rgb24(big, 128, &c));
        TEST_ASSERT_EQUAL(0, (int)c);
        memset(big, ' ', sizeof(big));
        memcpy(big + 150, "hsl(0 0% 0%)", 12);
        TEST_ASSERT_EQUAL(1, wf_cssprop_rgb24(big, 162, &c));
    }
    /* a registered <color> takes the units; it interpolates in sRGB */
    TEST_ASSERT_TRUE(syn("'<color>'"));
    TEST_ASSERT_EQUAL(2, wf_cssprop_interp(&g_p, "hsl(0.5turn 100% 50%)", "hsl(0rad 100% 50%)", 5000, g_o, 160));
    TEST_ASSERT_EQUAL_STRING("rgb(128, 128, 128)", g_o);
    TEST_ASSERT_EQUAL(1, wf_cssprop_interp(&g_p, "hsl(0.5turn 100% 50%)", "hsl(1x 100% 50%)", 2500, g_o, 160));
    /* rgb() does not take the letters of the hue units */
    TEST_ASSERT_EQUAL(1, wf_cssprop_interp(&g_p, "rgb(1, 2, 3)", "rgb(1turn, 2, 3)", 2500, g_o, 160));
    TEST_ASSERT_EQUAL_STRING("rgb(1, 2, 3)", g_o);
    TEST_ASSERT_EQUAL(0, wf_cssprop_match_prop(&g_p, "rgb(1turn, 2, 3)"));
    TEST_ASSERT_EQUAL(0, wf_cssprop_match_prop(&g_p, "rgba(1, 2, 3, 1rad)"));
    TEST_ASSERT_EQUAL(1, wf_cssprop_match_prop(&g_p, "hsl(1turn, 2%, 3%)"));
    TEST_ASSERT_EQUAL(1, wf_cssprop_match_prop(&g_p, "hsla(1grad 2% 3%)"));
    /* the background reader keeps its own colour check (Lot 90) */
    TEST_ASSERT_EQUAL(0, wf_cssbg_color("hsl(0.5turn 100% 50%)", 21));
}

/* add and accumulate on transforms (CSS Transforms 2 section 15) */
static void test_l99_accumulate_api(void) {
    TEST_ASSERT_EQUAL_STRING("translate(10px, 0px) rotate(45deg)",
                             comb(1, "'<transform-list>'", "translate(10px, 0px)", "rotate(45deg)", 160, 2));
    TEST_ASSERT_EQUAL_STRING("rotate(45deg)", comb(2, "'<transform-list>'", "translate(10px, 0px)", "rotate(45deg)", 160, 1));
    TEST_ASSERT_EQUAL_STRING("translate(11px, 7px) rotate(30deg) scale(2)",
                             comb(2, "'<transform-list>'", "translate(10px, 5px) rotate(10deg)",
                                  "translate(1px, 2px) rotate(20deg) scale(2)", 160, 2));
    TEST_ASSERT_EQUAL_STRING("translate(11px, 7px) rotate(10deg) skew(1deg)",
                             comb(2, "'<transform-list>'", "translate(10px, 5px) rotate(10deg) skew(1deg)",
                                  "translate(1px, 2px)", 160, 2));
    TEST_ASSERT_EQUAL_STRING("scale(4, 0.5)", comb(2, "'<transform-list>'", "scale(2, 50%)", "scale(3, 1)", 160, 2));
    TEST_ASSERT_EQUAL_STRING("scale(-1)", comb(2, "'<transform-list>'", "scale(0)", "scale(0)", 160, 2));
    TEST_ASSERT_EQUAL_STRING("rotate(190deg)", comb(2, "'<transform-list>'", "rotate(0.5turn)", "rotate(10deg)", 160, 2));
    TEST_ASSERT_EQUAL_STRING("translate(calc(50% + 10px))",
                             comb(2, "'<transform-list>'", "translate(10px)", "translate(50%)", 160, 2));
    TEST_ASSERT_EQUAL_STRING("rotate3d(0, 0, 1, 30deg)",
                             comb(2, "'<transform-list>'", "rotate3d(0, 0, 1, 10deg)", "rotate3d(0, 0, 1, 20deg)", 160, 2));
    /* replaced: another axis, matrix(), perspective(), another function,
     * another argument count */
    TEST_ASSERT_EQUAL_STRING("rotate3d(1, 0, 0, 20deg)",
                             comb(2, "'<transform-list>'", "rotate3d(0, 0, 1, 10deg)", "rotate3d(1, 0, 0, 20deg)", 160, 1));
    TEST_ASSERT_EQUAL_STRING("matrix(1, 0, 0, 1, 0, 0)",
                             comb(2, "'<transform-list>'", "matrix(1, 0, 0, 1, 0, 0)", "matrix(1, 0, 0, 1, 0, 0)", 160, 1));
    TEST_ASSERT_EQUAL_STRING("rotate3d(0.5, 0, 1, 20deg)",
                             comb(2, "'<transform-list>'", "rotate3d(0, 0, 1, 10deg)", "rotate3d(0.5, 0, 1, 20deg)", 160, 1));
    TEST_ASSERT_EQUAL_STRING("rotate3d(0, 0, 1, 20deg)",
                             comb(2, "'<transform-list>'", "rotate3d(0.5, 0, 1, 10deg)", "rotate3d(0, 0, 1, 20deg)", 160, 1));
    /* a scale that is not a plain number, or a sum over the limit */
    TEST_ASSERT_EQUAL_STRING("scale(2)", comb(2, "'<transform-list>'", "scale(calc(2))", "scale(2)", 160, 1));
    TEST_ASSERT_EQUAL_STRING("scale(calc(2))", comb(2, "'<transform-list>'", "scale(2)", "scale(calc(2))", 160, 1));
    TEST_ASSERT_EQUAL_STRING("scale(60000)", comb(2, "'<transform-list>'", "scale(60000)", "scale(60000)", 160, 1));
    TEST_ASSERT_EQUAL_STRING("scale(99999)", comb(2, "'<transform-list>'", "scale(50000)", "scale(50000)", 160, 2));
    TEST_ASSERT_EQUAL_STRING("scale(-99999)", comb(2, "'<transform-list>'", "scale(-50000)", "scale(-49998)", 160, 2));
    TEST_ASSERT_EQUAL_STRING("scale(-50000)", comb(2, "'<transform-list>'", "scale(-50000)", "scale(-50000)", 160, 1));
    TEST_ASSERT_EQUAL_STRING("translatez(3px)", comb(2, "'<transform-list>'", "translatez(1px)", "translatez(2px)", 160, 2));
    TEST_ASSERT_EQUAL_STRING("translatez(5%)", comb(2, "'<transform-list>'", "translatez(1px)", "translatez(5%)", 160, 1));
    TEST_ASSERT_EQUAL_STRING("skew(10deg, 5deg)", comb(2, "'<transform-list>'", "skew(5deg, 5deg)", "skew(5deg, 0deg)", 160, 2));
    TEST_ASSERT_EQUAL_STRING("rotate(1deg)", comb(2, "'<transform-list>'", "rotate(1deg)", "rotate(0deg)", 160, 2));
    TEST_ASSERT_EQUAL_STRING("translate(calc(5% + 5px), calc(5% + 5px))",
                             comb(2, "'<transform-list>'", "translate(5px, 5%)", "translate(5%, 5px)", 160, 2));
    TEST_ASSERT_EQUAL_STRING("perspective(10px)", comb(2, "'<transform-list>'", "perspective(5px)", "perspective(10px)", 160, 1));
    TEST_ASSERT_EQUAL_STRING("rotate(1deg)", comb(2, "'<transform-list>'", "translate(10px)", "rotate(1deg)", 160, 1));
    TEST_ASSERT_EQUAL_STRING("translate(1px, 2px)", comb(2, "'<transform-list>'", "translate(10px)", "translate(1px, 2px)", 160, 1));
    TEST_ASSERT_EQUAL_STRING("rotate(1deg) skewx(2deg)",
                             comb(2, "'<transform-list>'", "rotate(5deg) scale(2)", "rotate(1deg) skewx(2deg)", 160, 1));
    /* spaces around the lists and the arguments */
    TEST_ASSERT_EQUAL_STRING("rotate(5deg) rotate(1deg)", comb(1, "'<transform-list>'", "  rotate(5deg)  ", " rotate(1deg) ", 160, 2));
    TEST_ASSERT_EQUAL_STRING("rotate3d(0, 0, 1, 20deg)",
                             comb(2, "'<transform-list>'", "rotate3d(0 , 0 ,1, 10deg)", "rotate3d( 0, 0,  1 ,10deg )", 160, 2));
    TEST_ASSERT_EQUAL_STRING("rotate(3deg), scale(4)",
                             comb(2, "'<transform-function>#'", "rotate(1deg) ,  scale(2)", "rotate(2deg),scale(3) ", 160, 2));
    /* arguments with parentheses; a bare 0 is a length, not an angle */
    TEST_ASSERT_EQUAL_STRING("translate(calc(50% + 11px), 3px)",
                             comb(2, "'<transform-list>'", "translate(calc(50% + 10px), 1px)", "translate(1px, 2px)", 160, 2));
    TEST_ASSERT_EQUAL_STRING("translate(1px)", comb(2, "'<transform-list>'", "translate(0)", "translate(1px)", 160, 2));
    TEST_ASSERT_EQUAL_STRING("rotate(0)", comb(2, "'<transform-list>'", "rotate(1deg)", "rotate(0)", 160, 1));
    TEST_ASSERT_EQUAL_STRING("rotate(0)", comb(2, "'<transform-list>'", "rotate(0)", "rotate(0)", 160, 2));
    /* a percentage scale is divided by 100, rounded half away from zero */
    TEST_ASSERT_EQUAL_STRING("scale(0.1235)", comb(2, "'<transform-list>'", "scale(12.345%)", "scale(1)", 160, 2));
    TEST_ASSERT_EQUAL_STRING("scale(-0.1235)", comb(2, "'<transform-list>'", "scale(1)", "scale(-12.345%)", 160, 2));
    /* the room: a piece that does not fit gives 0 even when a later one
     * would (the name, a scale, a tail function) */
    TEST_ASSERT_EQUAL_STRING("", comb(2, "'<transform-list>'", "translatex(1px)", "translatex(2px)", 10, 0));
    TEST_ASSERT_EQUAL_STRING("", comb(2, "'<transform-list>'", "scale(5)", "scale(1.5)", 9, 0));
    TEST_ASSERT_EQUAL_STRING("", comb(2, "'<transform-list>'", "rotate(1deg)", "rotate(2deg) translatex(10px) scale(2)", 23, 0));
    /* every piece is checked when written: a long rotate3d() axis that
     * does not fit is not skipped */
    TEST_ASSERT_EQUAL_STRING("", comb(2, "'<transform-list>'", "rotate3d(0.000000000000000001, 1, 0, 5deg)",
                                      "rotate3d(0.000000000000000001, 1, 0, 5deg)", 24, 0));
    /* the first failure decides: the room for the ", " (or the space
     * between two functions) before a pair that does not sum */
    TEST_ASSERT_EQUAL_STRING("", comb(2, "'<transform-list>'", "translate(calc(50% + 10px), 99999px)", "translate(1px, 2px)", 28, 0));
    TEST_ASSERT_EQUAL_STRING("translate(1px, 2px)",
                             comb(2, "'<transform-list>'", "translate(calc(50% + 10px), 99999px)", "translate(1px, 2px)", 29, 1));
    TEST_ASSERT_EQUAL_STRING("", comb(2, "'<transform-list>'", "translate(calc(50% + 10px), 1px) scale(2)", "translate(1px, 2px) rotate(0)", 33, 0));
    TEST_ASSERT_EQUAL_STRING("translate(1px, 2px) rotate(0)",
                             comb(2, "'<transform-list>'", "translate(calc(50% + 10px), 1px) scale(2)", "translate(1px, 2px) rotate(0)", 34, 1));
    TEST_ASSERT_EQUAL_STRING("rotate(3deg) translatex(10px) scale(2)",
                             comb(2, "'<transform-list>'", "rotate(1deg)", "rotate(2deg) translatex(10px) scale(2)", 39, 2));
    /* the room: short gives 0 and an empty text */
    TEST_ASSERT_EQUAL_STRING("", comb(2, "'<transform-list>'", "rotate(5deg)", "rotate(1deg)", 12, 0));
    TEST_ASSERT_EQUAL_STRING("rotate(6deg)", comb(2, "'<transform-list>'", "rotate(5deg)", "rotate(1deg)", 13, 2));
    TEST_ASSERT_EQUAL_STRING("", comb(1, "'<transform-list>'", "rotate(5deg)", "rotate(1deg)", 25, 0));
    TEST_ASSERT_EQUAL_STRING("rotate(5deg) rotate(1deg)", comb(1, "'<transform-list>'", "rotate(5deg)", "rotate(1deg)", 26, 2));
    TEST_ASSERT_EQUAL_STRING("", comb(2, "'<transform-list>'", "scale(5)", "scale(1.5)", 10, 0));
    TEST_ASSERT_EQUAL_STRING("scale(5.5)", comb(2, "'<transform-list>'", "scale(5)", "scale(1.5)", 11, 2));
    /* one function: add has no one-function sum */
    TEST_ASSERT_EQUAL_STRING("rotate(2deg)", comb(1, "'<transform-function>'", "rotate(1deg)", "rotate(2deg)", 160, 1));
    TEST_ASSERT_EQUAL_STRING("rotate(3deg)", comb(2, "'<transform-function>'", "rotate(1deg)", "rotate(2deg)", 160, 2));
    TEST_ASSERT_EQUAL_STRING("scale(2)", comb(2, "'<transform-function>'", "rotate(1deg)", "scale(2)", 160, 1));
    TEST_ASSERT_EQUAL_STRING("rotate(1deg) scale(2) rotate(2deg)",
                             comb(1, "'<transform-function>+'", "rotate(1deg) scale(2)", "rotate(2deg)", 160, 2));
    TEST_ASSERT_EQUAL_STRING("rotate(3deg) scale(2)",
                             comb(2, "'<transform-function>+'", "rotate(1deg) scale(2)", "rotate(2deg)", 160, 2));
    TEST_ASSERT_EQUAL_STRING("rotate(3deg), scale(4)",
                             comb(2, "'<transform-function>#'", "rotate(1deg), scale(2)", "rotate(2deg), scale(3)", 160, 2));
    TEST_ASSERT_EQUAL_STRING("rotate(2deg), scale(3)",
                             comb(1, "'<transform-function>#'", "rotate(1deg), scale(2)", "rotate(2deg), scale(3)", 160, 1));
    TEST_ASSERT_EQUAL_STRING("rotate(2deg), scale(3)",
                             comb(2, "'<transform-function>#'", "rotate(1deg)", "rotate(2deg), scale(3)", 160, 1));
    TEST_ASSERT_EQUAL_STRING("rotate(2deg)",
                             comb(2, "'<transform-function>#'", "rotate(1deg), scale(3)", "rotate(2deg)", 160, 1));
    /* other types: accumulate is add */
    TEST_ASSERT_EQUAL_STRING("15px", comb(2, "'<length>'", "10px", "5px", 160, 2));
    TEST_ASSERT_EQUAL_STRING("15px", comb(1, "'<length>'", "10px", "5px", 160, 2));
    TEST_ASSERT_EQUAL_STRING("rgb(255, 0, 255)", comb(2, "'<color>'", "red", "blue", 160, 2));
    TEST_ASSERT_EQUAL_STRING("3px", comb(2, "'<length> | <transform-list>'", "translate(1px)", "3px", 160, 1));
    /* guards */
    TEST_ASSERT_EQUAL(1, wf_cssprop_accumulate(0, "rotate(1deg)", "rotate(2deg)", g_o, 160));
    TEST_ASSERT_EQUAL_STRING("rotate(2deg)", g_o);
    TEST_ASSERT_TRUE(syn("'<transform-list>'"));
    TEST_ASSERT_EQUAL(0, wf_cssprop_accumulate(&g_p, 0, "rotate(2deg)", g_o, 160));
    TEST_ASSERT_EQUAL(0, wf_cssprop_accumulate(&g_p, "rotate(1deg)", 0, g_o, 160));
    TEST_ASSERT_EQUAL(0, wf_cssprop_accumulate(&g_p, "rotate(1deg)", "rotate(2deg)", 0, 160));
    TEST_ASSERT_EQUAL(0, wf_cssprop_accumulate(&g_p, "rotate(1deg)", "rotate(2deg)", g_o, 0));
}

static const char *tlf(const wf_cssanim_t *a) {
    snprintf(g_o, sizeof(g_o), "%d %s", a->timeline, a->timeline == WF_CSSANIM_TL_NAMED ? a->tlname : "-");
    return g_o;
}

static void test_l99_timeline_api(void) {
    static wf_cssanim_t a[WF_CSSANIM_MAX];
    const char *v[WF_CSSANIM_LONGS] = {0};
    unsigned char sh[WF_CSSANIM_LONGS] = {0};
    TEST_ASSERT_EQUAL(9, WF_CSSANIM_L_TIMELINE);
    TEST_ASSERT_EQUAL(10, WF_CSSANIM_LONGS);
    TEST_ASSERT_EQUAL(0, WF_CSSANIM_TL_AUTO);
    TEST_ASSERT_EQUAL(3, wf_cssanim_long(WF_CSSANIM_L_TIMELINE, "--t, NONE, auto", a, WF_CSSANIM_MAX));
    TEST_ASSERT_EQUAL_STRING("2 --t", tlf(&a[0]));
    TEST_ASSERT_EQUAL_STRING("1 -", tlf(&a[1]));
    TEST_ASSERT_EQUAL_STRING("0 -", tlf(&a[2]));
    TEST_ASSERT_EQUAL(1, wf_cssanim_long(WF_CSSANIM_L_TIMELINE, "--Scroller_9", a, WF_CSSANIM_MAX));
    TEST_ASSERT_EQUAL_STRING("2 --Scroller_9", tlf(&a[0]));
    TEST_ASSERT_EQUAL(1, wf_cssanim_long(WF_CSSANIM_L_TIMELINE, "--xxxxxxxxxxxxxxxxxxxxxxxxxxxxx", a, WF_CSSANIM_MAX));
    TEST_ASSERT_EQUAL(-1, wf_cssanim_long(WF_CSSANIM_L_TIMELINE, "--xxxxxxxxxxxxxxxxxxxxxxxxxxxxxx", a, WF_CSSANIM_MAX));
    TEST_ASSERT_EQUAL(-1, wf_cssanim_long(WF_CSSANIM_L_TIMELINE, "scroll()", a, WF_CSSANIM_MAX));
    TEST_ASSERT_EQUAL(-1, wf_cssanim_long(WF_CSSANIM_L_TIMELINE, "view(block)", a, WF_CSSANIM_MAX));
    TEST_ASSERT_EQUAL(-1, wf_cssanim_long(WF_CSSANIM_L_TIMELINE, "auto, scroll()", a, WF_CSSANIM_MAX));
    TEST_ASSERT_EQUAL(-1, wf_cssanim_long(WF_CSSANIM_L_TIMELINE, "--", a, WF_CSSANIM_MAX));
    TEST_ASSERT_EQUAL(-1, wf_cssanim_long(WF_CSSANIM_L_TIMELINE, "-t", a, WF_CSSANIM_MAX));
    TEST_ASSERT_EQUAL(-1, wf_cssanim_long(WF_CSSANIM_L_TIMELINE, "t", a, WF_CSSANIM_MAX));
    TEST_ASSERT_EQUAL(-1, wf_cssanim_long(WF_CSSANIM_L_TIMELINE, "a-b", a, WF_CSSANIM_MAX));
    TEST_ASSERT_EQUAL(-1, wf_cssanim_long(WF_CSSANIM_L_TIMELINE, "-ab", a, WF_CSSANIM_MAX));
    TEST_ASSERT_EQUAL(1, wf_cssanim_long(WF_CSSANIM_L_TIMELINE, "--a", a, WF_CSSANIM_MAX));
    TEST_ASSERT_EQUAL_STRING("2 --a", tlf(&a[0]));
    TEST_ASSERT_EQUAL(-1, wf_cssanim_long(WF_CSSANIM_L_TIMELINE, "--1 x", a, WF_CSSANIM_MAX));
    TEST_ASSERT_EQUAL(-1, wf_cssanim_long(WF_CSSANIM_L_TIMELINE, "add", a, WF_CSSANIM_MAX));
    /* coordinated: repeated cyclically, unset is auto, a shorthand slot
     * is auto */
    v[WF_CSSANIM_L_NAME] = "a, b, c";
    v[WF_CSSANIM_L_TIMELINE] = "none, --t";
    TEST_ASSERT_EQUAL(3, wf_cssanim_coord(v, sh, a, WF_CSSANIM_MAX));
    TEST_ASSERT_EQUAL_STRING("1 -", tlf(&a[0]));
    TEST_ASSERT_EQUAL_STRING("2 --t", tlf(&a[1]));
    TEST_ASSERT_EQUAL_STRING("1 -", tlf(&a[2]));
    v[WF_CSSANIM_L_TIMELINE] = 0;
    TEST_ASSERT_EQUAL(3, wf_cssanim_coord(v, sh, a, WF_CSSANIM_MAX));
    TEST_ASSERT_EQUAL_STRING("0 -", tlf(&a[1]));
    v[WF_CSSANIM_L_NAME] = "k 1s";
    v[WF_CSSANIM_L_TIMELINE] = "k 1s";
    sh[WF_CSSANIM_L_NAME] = sh[WF_CSSANIM_L_TIMELINE] = 1;
    TEST_ASSERT_EQUAL(1, wf_cssanim_coord(v, sh, a, WF_CSSANIM_MAX));
    TEST_ASSERT_EQUAL_STRING("k", a[0].name);
    TEST_ASSERT_EQUAL_STRING("0 -", tlf(&a[0]));
    /* the shorthand does not take a timeline */
    TEST_ASSERT_EQUAL(0, wf_cssanim_parse("k 1s none", a, WF_CSSANIM_MAX) > 0 && a[0].timeline != 0);
}

/* CSS Animations 2: the 0% and 100% keyframes a rule leaves out take the
 * element's own value with the animation's composition */
static void test_l99_generated_frames(void) {
    prep(PN "@keyframes k { to { --p: 100px } } #o { --p: 10px; animation: k 1000ms linear; animation-composition: add }", 0);
    TEST_ASSERT_EQUAL_STRING("20px", nv("o"));
    TEST_ASSERT_EQUAL(1, g_d.prop_composed);
    prep(PN "@keyframes k { to { --p: 100px } } #o { --p: 10px; animation: k 1000ms linear; animation-composition: add }", 500);
    TEST_ASSERT_EQUAL_STRING("65px", nv("o"));
    prep(PN "@keyframes k { from { --p: 100px } } #o { --p: 10px; animation: k 1000ms linear; animation-composition: accumulate }", 500);
    TEST_ASSERT_EQUAL_STRING("65px", nv("o"));
    /* replace: the element's own value, not the value the animations
     * before give (Lot 98 took the value below: 25px here) */
    prep(PN "@keyframes a { from { --p: 50px } to { --p: 50px } } @keyframes b { 50% { --p: 0px } } "
         "#o { --p: 10px; animation: a 1000ms linear, b 1000ms linear }", 250);
    TEST_ASSERT_EQUAL_STRING("5px", nv("o"));
    TEST_ASSERT_EQUAL(0, g_d.prop_composed);
    /* add: the own value put on the value below (50px + 10px), the 50%
     * keyframe too (0px + 50px) */
    prep(PN "@keyframes a { from { --p: 50px } to { --p: 50px } } @keyframes b { 50% { --p: 0px } } "
         "#o { --p: 10px; animation: a 1000ms linear, b 1000ms linear; animation-composition: replace, add }", 250);
    TEST_ASSERT_EQUAL_STRING("55px", nv("o"));
    TEST_ASSERT_EQUAL(1, g_d.prop_composed);
    /* a keyframe's own composition does not reach the generated ones */
    prep(PN "@keyframes k { 50% { --p: 100px; animation-composition: add } } #o { --p: 10px; animation: k 1000ms linear }", 250);
    TEST_ASSERT_EQUAL_STRING("60px", nv("o"));
    prep(PN "@keyframes k { 50% { --p: 100px; animation-composition: replace } } "
         "#o { --p: 10px; animation: k 1000ms linear; animation-composition: add }", 250);
    TEST_ASSERT_EQUAL_STRING("60px", nv("o"));
    TEST_ASSERT_EQUAL(1, g_d.prop_composed);
    /* only the generated 100% composes: still flagged */
    prep(PN "@keyframes k { from { --p: 100px; animation-composition: replace } } "
         "#o { --p: 10px; animation: k 1000ms linear; animation-composition: add }", 500);
    TEST_ASSERT_EQUAL_STRING("60px", nv("o"));
    TEST_ASSERT_EQUAL(1, g_d.prop_composed);
    /* a composite that does not combine leaves the own value */
    prep("@property --p { syntax: '<length> | <custom-ident>'; inherits: false; initial-value: 0px } "
         "@keyframes k { 50% { --p: 100px } } #o { --p: abc; animation: k 1000ms linear; animation-composition: add }", 200);
    TEST_ASSERT_EQUAL_STRING("abc", nv("o"));
    TEST_ASSERT_EQUAL(0, g_d.prop_composed);
}

static void test_l99_reader_transforms(void) {
    prep(PT "@keyframes k { from { --p: rotate(10deg) } to { --p: rotate(30deg) } } "
         "#o { --p: rotate(5deg); animation: k 1000ms linear; animation-composition: accumulate }", 250);
    TEST_ASSERT_EQUAL_STRING("rotate(15deg)", nv("o"));
    TEST_ASSERT_EQUAL(1, g_d.prop_composed);
    prep(PT "@keyframes k { from { --p: rotate(10deg) } to { --p: rotate(30deg) } } "
         "#o { --p: rotate(5deg); animation: k 1000ms linear; animation-composition: accumulate }", 750);
    TEST_ASSERT_EQUAL_STRING("rotate(35deg)", nv("o"));
    prep(PT "@keyframes k { from { --p: rotate(10deg) } to { --p: rotate(30deg) } } "
         "#o { --p: rotate(5deg); animation: k 1000ms linear; animation-composition: add }", 250);
    TEST_ASSERT_EQUAL_STRING("rotate(5deg) rotate(10deg)", nv("o"));
    prep(PT "@keyframes k { to { --p: scale(2) } } "
         "#o { --p: scale(3); animation: k 1000ms linear; animation-composition: accumulate }", 250);
    TEST_ASSERT_EQUAL_STRING("scale(5)", nv("o"));      /* the generated 0%: 3 + 3 - 1 */
    prep(PT "@keyframes k { to { --p: scale(2) } } "
         "#o { --p: scale(3); animation: k 1000ms linear; animation-composition: add }", 250);
    TEST_ASSERT_EQUAL_STRING("scale(3) scale(3)", nv("o"));
    /* another function: accumulate replaces */
    prep(PT "@keyframes k { from { --p: translate(1px) } to { --p: translate(1px) } } "
         "#o { --p: rotate(5deg); animation: k 1000ms linear; animation-composition: accumulate }", 250);
    TEST_ASSERT_EQUAL_STRING("translate(1px)", nv("o"));
    TEST_ASSERT_EQUAL(0, g_d.prop_composed);
}

static void test_l99_timeline_reader(void) {
    prep(PN KF "#o { --p: 10px; animation: k 1000ms linear; animation-timeline: none }", 500);
    TEST_ASSERT_EQUAL_STRING("10px", nv("o"));
    TEST_ASSERT_EQUAL(1, g_d.anim_idle);
    TEST_ASSERT_EQUAL_STRING("keyframes 1 animated 0 transitioned 0 font 0 at 500ms idle 1", anim_text());
    prep(PN KF "#o { --p: 10px; animation: k 1000ms linear; animation-timeline: --t }", 500);
    TEST_ASSERT_EQUAL_STRING("10px", nv("o"));
    TEST_ASSERT_EQUAL(1, g_d.anim_idle);
    prep(PN KF "#o { --p: 10px; animation: k 1000ms linear; animation-timeline: AUTO }", 500);
    TEST_ASSERT_EQUAL_STRING("50px", nv("o"));
    TEST_ASSERT_EQUAL(0, g_d.anim_idle);
    TEST_ASSERT_EQUAL_STRING("keyframes 1 animated 1 transitioned 0 font 0 at 500ms", anim_text());
    /* scroll() and view() are not taken: the declaration is dropped */
    prep(PN KF "#o { --p: 10px; animation: k 1000ms linear; animation-timeline: none; animation-timeline: scroll() }", 500);
    TEST_ASSERT_EQUAL_STRING("10px", nv("o"));
    prep(PN KF "#o { --p: 10px; animation: k 1000ms linear; animation-timeline: view() }", 500);
    TEST_ASSERT_EQUAL_STRING("50px", nv("o"));
    /* the shorthand resets the timeline to auto */
    prep(PN KF "#o { --p: 10px; animation-timeline: none; animation: k 1000ms linear }", 500);
    TEST_ASSERT_EQUAL_STRING("50px", nv("o"));
    prep(PN KF "#o { --p: 10px; animation-timeline: none !important; animation: k 1000ms linear }", 500);
    TEST_ASSERT_EQUAL_STRING("10px", nv("o"));
    /* per animation, cyclically */
    prep(PN KF "@keyframes j { from { --p: 1000px } to { --p: 1000px } } "
         "#o { --p: 10px; animation: k 1000ms linear, j 1000ms linear; animation-timeline: auto, none }", 500);
    TEST_ASSERT_EQUAL_STRING("50px", nv("o"));
    TEST_ASSERT_EQUAL(1, g_d.anim_idle);
    /* a name no @keyframes has is not idle, it is nothing */
    prep(PN KF "#o { --p: 10px; animation: zz 1000ms linear; animation-timeline: none }", 500);
    TEST_ASSERT_EQUAL(0, g_d.anim_idle);
    TEST_ASSERT_EQUAL_STRING("keyframes 1 animated 0 transitioned 0 font 0 at 500ms", anim_text());
    /* inherit is not taken; a later shorthand of the same weight resets
     * the timeline */
    prep(PN KF "#o, #i { --p: 10px; animation: k 1000ms linear } #o { animation-timeline: none } #i { animation-timeline: inherit }", 500);
    TEST_ASSERT_EQUAL_STRING("10px", nv("o"));
    TEST_ASSERT_EQUAL_STRING("50px", nv("i"));
    prep(PN KF "#o { animation-timeline: none } #o, #i { --p: 10px; animation: k 1000ms linear }", 500);
    TEST_ASSERT_EQUAL_STRING("50px", nv("o"));
    /* a weaker selector loses to the shorthand's reset */
    prep(PN KF "#o, #i { --p: 10px; animation: k 1000ms linear } div { animation-timeline: none }", 500);
    TEST_ASSERT_EQUAL_STRING("50px", nv("o"));
    TEST_ASSERT_EQUAL_STRING("50px", nv("i"));
}

/* hsl() painted directly: color, background-color, the background
 * shorthand and var() */
static void test_l99_hsl_paint(void) {
    prep("#o { color: hsl(0.5turn 100% 50%) }", 0);
    TEST_ASSERT_EQUAL_STRING(" style=\"color:#00ffff\"", style_of("o"));
    prep("#o { color: HSLA(200grad, 100%, 50%, 1) }", 0);
    TEST_ASSERT_EQUAL_STRING(" style=\"color:#00ffff\"", style_of("o"));
    prep("#o { color: hsl(1rad 100% 50%) !important } #o { color: blue }", 0);
    TEST_ASSERT_EQUAL_STRING(" style=\"color:#fff400\"", style_of("o"));
    prep("#o { color: blue } #o { color: hsl(0.5turn 100% 50% / 0.5) }", 0);
    TEST_ASSERT_EQUAL_STRING(" style=\"color:#0000ff\"", style_of("o"));
    prep("#o { color: blue } #o { color: hsl(0.5turn 100% 50%) x }", 0);
    TEST_ASSERT_EQUAL_STRING(" style=\"color:#0000ff\"", style_of("o"));
    prep("#o { color: blue } #o { color: hsl(0.5turns 100% 50%) }", 0);
    TEST_ASSERT_EQUAL_STRING(" style=\"color:#0000ff\"", style_of("o"));
    prep("#o { background-color: hsl(120, 100%, 25%) }", 0);
    TEST_ASSERT_NOT_NULL(strstr(style_of("o"), "background-color:#008000"));
    prep("#o { background: url(x.png) hsl(60 100% 50%) }", 0);
    TEST_ASSERT_NOT_NULL(strstr(style_of("o"), "background-color:#ffff00;background-image:url(x.png)"));
    prep("#o { background-color: blue } #o { background: hsl(60 100% 50% / 0.5) }", 0);
    TEST_ASSERT_NOT_NULL(strstr(style_of("o"), "background-color:#0000ff"));
    /* the colour word of the shorthand stays under 32 bytes (Lot 90) */
    prep("#o { background-color: blue } #o { background: hsla(60deg 100.000% 50.000% / 1) }", 0);
    TEST_ASSERT_NOT_NULL(strstr(style_of("o"), "background-color:#0000ff"));
    /* a gradient stop is not painted in hsl() (no change there) */
    prep("#o { background-color: blue } #o { background-image: linear-gradient(hsl(0 0% 0%), red) }", 0);
    TEST_ASSERT_NULL(strstr(style_of("o"), "linear-gradient"));
    prep("@property --c { syntax: '<color>'; inherits: false; initial-value: hsl(0.25turn 100% 50%) } "
         "#o { background-color: var(--c) }", 0);
    TEST_ASSERT_NOT_NULL(strstr(style_of("o"), "background-color:#80ff00"));
}

int main(void) {
    unity_init();
    RUN_TEST(test_l99_hue_units);
    RUN_TEST(test_l99_accumulate_api);
    RUN_TEST(test_l99_timeline_api);
    RUN_TEST(test_l99_generated_frames);
    RUN_TEST(test_l99_reader_transforms);
    RUN_TEST(test_l99_timeline_reader);
    RUN_TEST(test_l99_hsl_paint);
    unity_print_results();
    unity_cleanup();
    return unity_stats.tests_failed == 0 ? 0 : 1;
}
