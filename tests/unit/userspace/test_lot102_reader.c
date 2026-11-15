/* Lot 102 (US-031): hand-written cases next to the generated suites
 * (test_lot102_none.c, test_lot102_range.c, test_lot102_scope.c,
 * test_lot102_sheet.c): the none keyword and missing components (CSS
 * Color 4 sections 4.4 and 12.2), scroll() and view() in
 * animation-timeline, animation-range and timeline-scope (CSS
 * Scroll-driven Animations 1), and the API guards. Each value below is
 * worked out from the specs in docs/lot102_plan.md. */
#include "../../framework/unity.h"
#include <string.h>
#include <stdio.h>
#include "wf_sitecss.h"
#include "wf_cssprop.h"

static wf_site_t g_d;
static wf_cssprop_t g_p;
static char g_html[8192];
static char g_o[1400];
static char g_r[4][64];
static int g_ri;

#define KB "@keyframes k { from { color: #000000 } to { color: #ffffff } } "
#define PR "@property --p { syntax: '<length>'; inherits: false; initial-value: 0px } "

static void prep(const char *css, int ms, int pm) {
    wf_site_set_time(ms);
    wf_site_set_scroll(pm);
    snprintf(g_html, sizeof(g_html), "<html><head><style>%s</style></head><body>"
             "<div id=o><p id=i>i</p></div><p id=q>q</p></body></html>", css);
    TEST_ASSERT_EQUAL(0, wf_site_prepare_ext(&g_d, g_html, (int)strlen(g_html), 640, 480, 0, 0));
    wf_site_set_time(0);
    wf_site_set_scroll(0);
}

static void prep_doc(const char *html, int pm) {
    wf_site_set_scroll(pm);
    TEST_ASSERT_EQUAL(0, wf_site_prepare_ext(&g_d, html, (int)strlen(html), 640, 480, 0, 0));
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
    if (!wf_site_anim_text(&g_d, g_o, (int)sizeof(g_o))) return "(empty)";
    return g_o;
}

static const char *rc(const char *v) {
    unsigned int c = 0x12345678u;
    int a = -3;
    char *o = g_r[g_ri++ & 3];
    if (!wf_cssprop_color(v, (int)strlen(v), &c, &a)) return c == 0x12345678u && a == -3 ? "-" : "WRITTEN";
    snprintf(o, 64, "#%06x %d", c, a);
    return o;
}

static int syn(const char *s) {
    memset(&g_p, 0, sizeof(g_p));
    g_p.syntax = (unsigned char)wf_cssprop_syntax_parse(s, (int)strlen(s), &g_p);
    return g_p.syntax != 0;
}

static const char *ip(const char *a, const char *b, int t) {
    static char out[160];
    if (!syn("'<color>'")) return "BAD SYNTAX";
    if (wf_cssprop_interp(&g_p, a, b, t, out, (int)sizeof(out)) < 0) return "-";
    return out;
}

static const char *ad(const char *a, const char *b) {
    static char out[160];
    if (!syn("'<color>'")) return "BAD SYNTAX";
    if (wf_cssprop_add(&g_p, a, b, out, (int)sizeof(out)) < 0) return "-";
    return out;
}

static int mt(const char *v) {
    if (!syn("'<color>'")) return -1;
    return wf_cssprop_match_prop(&g_p, v) != 0;
}

/* none in each colour function */
static void test_l102_none_forms(void) {
    TEST_ASSERT_EQUAL_STRING("#000000 255", rc("rgb(none 0 0)"));
    TEST_ASSERT_EQUAL_STRING("#00ff00 255", rc("rgb(none 255 none)"));
    TEST_ASSERT_EQUAL_STRING("#0a141e 0", rc("rgb(10 20 30 / none)"));
    TEST_ASSERT_EQUAL_STRING("#0a141e 0", rc("rgba(10 20 30 / NONE)"));
    /* the comma forms take no none */
    TEST_ASSERT_EQUAL_STRING("-", rc("rgb(none, 0, 0)"));
    TEST_ASSERT_EQUAL_STRING("-", rc("rgba(1, 2, 3, none)"));
    TEST_ASSERT_EQUAL_STRING("-", rc("hsl(none, 100%, 50%)"));
    TEST_ASSERT_EQUAL_STRING("-", rc("hsla(0, 100%, 50%, none)"));
    TEST_ASSERT_EQUAL_STRING("-", rc("rgb(nonee 0 0)"));
    TEST_ASSERT_EQUAL_STRING("-", rc("rgb(none 0)"));
    /* hsl(): none is 0 for the hue, saturation and lightness */
    TEST_ASSERT_EQUAL_STRING("#ff0000 255", rc("hsl(none 100% 50%)"));
    TEST_ASSERT_EQUAL_STRING("#808080 255", rc("hsl(120 none 50%)"));
    TEST_ASSERT_EQUAL_STRING("#000000 255", rc("hsl(120 100% none)"));
    TEST_ASSERT_EQUAL_STRING("#00ff00 0", rc("hsla(120 100% 50% / none)"));
    /* hwb() */
    TEST_ASSERT_EQUAL_STRING("#ff0000 255", rc("hwb(none 0% 0%)"));
    TEST_ASSERT_EQUAL_STRING("#ff0000 255", rc("hwb(0 none none)"));
    TEST_ASSERT_EQUAL_STRING("#ffffff 0", rc("hwb(0 100% 0% / none)"));
    /* lab(), lch(), oklab(), oklch(): 0 */
    TEST_ASSERT_EQUAL_STRING("#000000 255", rc("lab(none 0 0)"));
    TEST_ASSERT_EQUAL_STRING("#ffffff 255", rc("lab(100 none none)"));
    TEST_ASSERT_EQUAL_STRING("#ffffff 255", rc("lch(100 0 none)"));
    TEST_ASSERT_EQUAL_STRING(rc("lch(50 30 0)"), rc("lch(50 30 none)"));
    TEST_ASSERT_EQUAL_STRING("#ffffff 255", rc("oklch(1 none none)"));
    TEST_ASSERT_EQUAL_STRING("#000000 0", rc("oklab(0 0 0 / none)"));
    /* color() */
    TEST_ASSERT_EQUAL_STRING("#00ff00 255", rc("color(srgb none 1 0)"));
    TEST_ASSERT_EQUAL_STRING("#ffffff 0", rc("color(srgb 1 1 1 / none)"));
    TEST_ASSERT_EQUAL_STRING("#000000 255", rc("color(display-p3 none none none)"));
    TEST_ASSERT_EQUAL_STRING("#000000 255", rc("color(xyz none 0 none)"));
    TEST_ASSERT_EQUAL_STRING("-", rc("color(none 1 0 0)"));
    /* registered <color>: rgb() and hsl() with none by their conversion */
    TEST_ASSERT_EQUAL(1, mt("rgb(none 0 0)"));
    TEST_ASSERT_EQUAL(1, mt("rgba(none 0 0 / none)"));
    TEST_ASSERT_EQUAL(1, mt("hsl(none 50% 50%)"));
    TEST_ASSERT_EQUAL(1, mt("hsla(1 2% 3% / none)"));
    TEST_ASSERT_EQUAL(0, mt("rgb(none, 0, 0)"));
    TEST_ASSERT_EQUAL(0, mt("hsl(none, 1%, 1%)"));
    TEST_ASSERT_EQUAL(0, mt("rgb(nonex 0 0)"));
    TEST_ASSERT_EQUAL(1, mt("rgb(1 2 3)"));
}

/* missing components in color-mix(), interpolation and addition */
static void test_l102_none_carry(void) {
    /* the missing red takes the other colour's: (255, 127.5, 127.5), not
     * a grey */
    TEST_ASSERT_EQUAL_STRING("#ff8080 255", rc("color-mix(in srgb, rgb(none 0 0), rgb(255 255 255))"));
    TEST_ASSERT_EQUAL_STRING("#ff8080 255", rc("color-mix(in srgb, rgb(255 255 255), rgb(none 0 0))"));
    /* a missing alpha: 0.5 both, so the channel is 127.5 */
    TEST_ASSERT_EQUAL_STRING("#808080 128", rc("color-mix(in srgb, rgb(0 0 0 / none), rgb(255 255 255 / 50%))"));
    /* missing in both: 0 */
    TEST_ASSERT_EQUAL_STRING("#008000 255", rc("color-mix(in srgb, rgb(none 0 0), rgb(none 255 0))"));
    TEST_ASSERT_EQUAL_STRING("#000000 0", rc("color-mix(in srgb, rgb(9 9 9 / none), rgb(9 9 9 / none))"));
    /* and it stays missing for an outer mix; missing in one only does not */
    TEST_ASSERT_EQUAL_STRING("#ff0000 255",
                             rc("color-mix(in srgb, color-mix(in srgb, rgb(none 0 0), rgb(none 0 0)), rgb(255 0 0))"));
    TEST_ASSERT_EQUAL_STRING("#850000 255",
                             rc("color-mix(in srgb, color-mix(in srgb, rgb(none 0 0), rgb(10 0 0)), rgb(255 0 0))"));
    /* hsl() has no channel to carry; color() xyz has (x is the red's
     * analogue) */
    TEST_ASSERT_EQUAL_STRING("#808080 255", rc("color-mix(in srgb, hsl(none 0% 0%), white)"));
    TEST_ASSERT_EQUAL_STRING("#ff0000 255", rc("color-mix(in srgb, color(xyz none 0 0), rgb(255 0 0))"));
    TEST_ASSERT_EQUAL_STRING("#00ff00 255", rc("color-mix(in srgb, color(srgb 0 none 0), rgb(0 255 0))"));
    TEST_ASSERT_EQUAL_STRING("#0000ff 255", rc("color-mix(in srgb, color(srgb 0 0 none), rgb(0 0 255) 50%)"));
    /* interpolation carries too */
    TEST_ASSERT_EQUAL_STRING("rgb(255, 128, 128)", ip("rgb(none 0 0)", "rgb(255 255 255)", 5000));
    TEST_ASSERT_EQUAL_STRING("rgb(255, 128, 128)", ip("rgb(255 255 255)", "rgb(none 0 0)", 5000));
    TEST_ASSERT_EQUAL_STRING("rgba(128, 0, 0, 0.5)", ip("rgb(0 0 0 / none)", "rgb(255 0 0 / 50%)", 5000));
    TEST_ASSERT_EQUAL_STRING("rgb(0, 128, 0)", ip("rgb(none 0 0)", "rgb(none 255 0)", 5000));
    /* addition takes the values as they convert */
    TEST_ASSERT_EQUAL_STRING("rgb(10, 20, 30)", ad("rgb(none 0 0)", "rgb(10 20 30)"));
    TEST_ASSERT_EQUAL_STRING("rgb(10, 20, 30)", ad("rgb(10 20 30)", "rgb(none 0 0)"));
    TEST_ASSERT_EQUAL_STRING("rgba(10, 20, 30, 0.5)", ad("rgb(0 0 0 / none)", "rgb(10 20 30 / 50%)"));
}

static const char *tlv(const wf_cssanim_t *a) {
    snprintf(g_o, sizeof(g_o), "%d %d %d", a->timeline, a->tlsrc, a->tlaxis);
    return g_o;
}

/* scroll() and view() in animation-timeline */
static void test_l102_anon_api(void) {
    static wf_cssanim_t a[WF_CSSANIM_MAX];
    TEST_ASSERT_EQUAL(3, WF_CSSANIM_TL_SCROLL);
    TEST_ASSERT_EQUAL(4, WF_CSSANIM_TL_VIEW);
    TEST_ASSERT_EQUAL(1, wf_cssanim_long(WF_CSSANIM_L_TIMELINE, "scroll()", a, 4));
    TEST_ASSERT_EQUAL_STRING("3 0 0", tlv(&a[0]));
    TEST_ASSERT_EQUAL(1, wf_cssanim_long(WF_CSSANIM_L_TIMELINE, "scroll(root)", a, 4));
    TEST_ASSERT_EQUAL_STRING("3 1 0", tlv(&a[0]));
    TEST_ASSERT_EQUAL(1, wf_cssanim_long(WF_CSSANIM_L_TIMELINE, "scroll(self x)", a, 4));
    TEST_ASSERT_EQUAL_STRING("3 2 2", tlv(&a[0]));
    TEST_ASSERT_EQUAL(1, wf_cssanim_long(WF_CSSANIM_L_TIMELINE, "scroll(y nearest)", a, 4));
    TEST_ASSERT_EQUAL_STRING("3 0 3", tlv(&a[0]));
    TEST_ASSERT_EQUAL(1, wf_cssanim_long(WF_CSSANIM_L_TIMELINE, "scroll( inline )", a, 4));
    TEST_ASSERT_EQUAL_STRING("3 0 1", tlv(&a[0]));
    TEST_ASSERT_EQUAL(1, wf_cssanim_long(WF_CSSANIM_L_TIMELINE, "view()", a, 4));
    TEST_ASSERT_EQUAL_STRING("4 0 0", tlv(&a[0]));
    TEST_ASSERT_EQUAL(1, wf_cssanim_long(WF_CSSANIM_L_TIMELINE, "view(inline 10% auto)", a, 4));
    TEST_ASSERT_EQUAL_STRING("4 0 1", tlv(&a[0]));
    TEST_ASSERT_EQUAL(1, wf_cssanim_long(WF_CSSANIM_L_TIMELINE, "view(10px 0 y)", a, 4));
    TEST_ASSERT_EQUAL_STRING("4 0 3", tlv(&a[0]));
    TEST_ASSERT_EQUAL(1, wf_cssanim_long(WF_CSSANIM_L_TIMELINE, "view(auto)", a, 4));
    /* a name list item after it keeps its own name */
    TEST_ASSERT_EQUAL(2, wf_cssanim_long(WF_CSSANIM_L_TIMELINE, "scroll(root), --t", a, 4));
    TEST_ASSERT_EQUAL_STRING("2 --t", (snprintf(g_o, sizeof(g_o), "%d %s", a[1].timeline, a[1].tlname), g_o));
    /* refused */
    TEST_ASSERT_EQUAL(-1, wf_cssanim_long(WF_CSSANIM_L_TIMELINE, "scroll(root self)", a, 4));
    TEST_ASSERT_EQUAL(-1, wf_cssanim_long(WF_CSSANIM_L_TIMELINE, "scroll(y x)", a, 4));
    TEST_ASSERT_EQUAL(-1, wf_cssanim_long(WF_CSSANIM_L_TIMELINE, "scroll(10%)", a, 4));
    TEST_ASSERT_EQUAL(-1, wf_cssanim_long(WF_CSSANIM_L_TIMELINE, "scroll(top)", a, 4));
    TEST_ASSERT_EQUAL(-1, wf_cssanim_long(WF_CSSANIM_L_TIMELINE, "scroll(", a, 4));
    TEST_ASSERT_EQUAL(-1, wf_cssanim_long(WF_CSSANIM_L_TIMELINE, "scroll()x", a, 4));
    /* mutation witnesses: no closing parenthesis, a tail after an inset,
     * an inset longer than 8 bytes, the name of an earlier value cleared */
    TEST_ASSERT_EQUAL(-1, wf_cssanim_long(WF_CSSANIM_L_TIMELINE, "scroll(x", a, 4));
    TEST_ASSERT_EQUAL(-1, wf_cssanim_long(WF_CSSANIM_L_TIMELINE, "view(y", a, 4));
    TEST_ASSERT_EQUAL(-1, wf_cssanim_long(WF_CSSANIM_L_TIMELINE, "view(5%%)", a, 4));
    TEST_ASSERT_EQUAL(-1, wf_cssanim_long(WF_CSSANIM_L_TIMELINE, "view(10px10px)", a, 4));
    TEST_ASSERT_EQUAL(1, wf_cssanim_long(WF_CSSANIM_L_TIMELINE, "view(10.12345px)", a, 4));
    TEST_ASSERT_EQUAL(1, wf_cssanim_long(WF_CSSANIM_L_TIMELINE, "view(10px 1.123456789%)", a, 4));
    a[0].timeline = WF_CSSANIM_TL_NAMED;
    a[0].tlname[0] = '-'; a[0].tlname[1] = '-'; a[0].tlname[2] = 'o'; a[0].tlname[3] = 0;
    TEST_ASSERT_EQUAL(1, wf_cssanim_long(WF_CSSANIM_L_TIMELINE, "scroll()", a, 4));
    TEST_ASSERT_EQUAL_STRING("", a[0].tlname);
    TEST_ASSERT_EQUAL(-1, wf_cssanim_long(WF_CSSANIM_L_TIMELINE, "scroll(a)(b)", a, 4));
    TEST_ASSERT_EQUAL(-1, wf_cssanim_long(WF_CSSANIM_L_TIMELINE, "scroll(root)(y)", a, 4));
    TEST_ASSERT_EQUAL(-1, wf_cssanim_long(WF_CSSANIM_L_TIMELINE, "view(1px 2px 3px)", a, 4));
    TEST_ASSERT_EQUAL(-1, wf_cssanim_long(WF_CSSANIM_L_TIMELINE, "view(1px y 2px)", a, 4));
    TEST_ASSERT_EQUAL(-1, wf_cssanim_long(WF_CSSANIM_L_TIMELINE, "view(y 1px x)", a, 4));
    TEST_ASSERT_EQUAL(-1, wf_cssanim_long(WF_CSSANIM_L_TIMELINE, "view(5)", a, 4));
    TEST_ASSERT_EQUAL(-1, wf_cssanim_long(WF_CSSANIM_L_TIMELINE, "view(root)", a, 4));
    TEST_ASSERT_EQUAL(-1, wf_cssanim_long(WF_CSSANIM_L_TIMELINE, "view(10ms)", a, 4));
    TEST_ASSERT_EQUAL(-1, wf_cssanim_long(WF_CSSANIM_L_TIMELINE, "view(1pxx)", a, 4));
    TEST_ASSERT_EQUAL(-1, wf_cssanim_long(WF_CSSANIM_L_TIMELINE, "view(calc(1px))", a, 4));
    TEST_ASSERT_EQUAL(-1, wf_cssanim_long(WF_CSSANIM_L_TIMELINE, "view(1234567890123456789012345678901px)", a, 4));
    TEST_ASSERT_EQUAL(-1, wf_cssanim_long(WF_CSSANIM_L_TIMELINE, "scroll(root) y", a, 4));
}

static const char *edges(const wf_cssanim_t *a) {
    snprintf(g_o, sizeof(g_o), "%d %d %d %d", a->rs, a->rsp, a->re, a->rep);
    return g_o;
}

/* animation-range: the longhands, the shorthand and the text */
static void test_l102_range_api(void) {
    static wf_cssanim_t a[WF_CSSANIM_MAX];
    char t[64];
    TEST_ASSERT_EQUAL(10, WF_CSSANIM_L_RANGE_START);
    TEST_ASSERT_EQUAL(11, WF_CSSANIM_L_RANGE_END);
    memset(a, 0, sizeof(a));
    TEST_ASSERT_EQUAL(1, wf_cssanim_long(WF_CSSANIM_L_RANGE_START, "entry", a, 4));
    TEST_ASSERT_EQUAL(1, wf_cssanim_long(WF_CSSANIM_L_RANGE_END, "entry", a, 4));
    TEST_ASSERT_EQUAL_STRING("4 0 4 1000000", edges(&a[0]));
    TEST_ASSERT_EQUAL(2, wf_cssanim_long(WF_CSSANIM_L_RANGE_START, "exit-crossing 12.5%, 0", a, 4));
    TEST_ASSERT_EQUAL_STRING("7 125000 4 1000000", edges(&a[0]));
    TEST_ASSERT_EQUAL_STRING("1 0 0 0", edges(&a[1]));
    TEST_ASSERT_EQUAL(1, wf_cssanim_long(WF_CSSANIM_L_RANGE_END, "normal", a, 4));
    TEST_ASSERT_EQUAL_STRING("7 125000 0 0", edges(&a[0]));
    TEST_ASSERT_EQUAL(-1, wf_cssanim_long(WF_CSSANIM_L_RANGE_START, "5", a, 4));
    TEST_ASSERT_EQUAL(-1, wf_cssanim_long(WF_CSSANIM_L_RANGE_START, "10px", a, 4));
    TEST_ASSERT_EQUAL(-1, wf_cssanim_long(WF_CSSANIM_L_RANGE_START, "normal 10%", a, 4));
    TEST_ASSERT_EQUAL(-1, wf_cssanim_long(WF_CSSANIM_L_RANGE_START, "cover 10% 20%", a, 4));
    TEST_ASSERT_EQUAL(-1, wf_cssanim_long(WF_CSSANIM_L_RANGE_END, "10% entry", a, 4));
    TEST_ASSERT_EQUAL(-1, wf_cssanim_long(WF_CSSANIM_L_RANGE_END, "entry entry", a, 4));
    TEST_ASSERT_EQUAL(-1, wf_cssanim_long(WF_CSSANIM_L_RANGE_END, "", a, 4));
    /* the shorthand: an end left out */
    TEST_ASSERT_EQUAL(1, wf_cssanim_range("entry", a, 4));
    TEST_ASSERT_EQUAL_STRING("4 0 4 1000000", edges(&a[0]));
    TEST_ASSERT_EQUAL(1, wf_cssanim_range("cover 20%", a, 4));
    TEST_ASSERT_EQUAL_STRING("2 200000 2 1000000", edges(&a[0]));
    TEST_ASSERT_EQUAL(1, wf_cssanim_range("10%", a, 4));
    TEST_ASSERT_EQUAL_STRING("1 100000 0 0", edges(&a[0]));
    TEST_ASSERT_EQUAL(1, wf_cssanim_range("normal", a, 4));
    TEST_ASSERT_EQUAL_STRING("0 0 0 0", edges(&a[0]));
    TEST_ASSERT_EQUAL(3, wf_cssanim_range("10% 90%, entry 10% exit 90%, entry 10% 90%", a, 4));
    TEST_ASSERT_EQUAL_STRING("1 100000 1 900000", edges(&a[0]));
    TEST_ASSERT_EQUAL_STRING("4 100000 5 900000", edges(&a[1]));
    TEST_ASSERT_EQUAL_STRING("4 100000 1 900000", edges(&a[2]));
    TEST_ASSERT_EQUAL(1, wf_cssanim_range("entry exit", a, 4));
    TEST_ASSERT_EQUAL_STRING("4 0 5 1000000", edges(&a[0]));
    TEST_ASSERT_EQUAL(-1, wf_cssanim_range("10% 20% 30%", a, 4));
    TEST_ASSERT_EQUAL(-1, wf_cssanim_range("10% bogus", a, 4));
    TEST_ASSERT_EQUAL(-1, wf_cssanim_range("a, b, c, d, e", a, 8));
    TEST_ASSERT_EQUAL(-1, wf_cssanim_range(0, a, 4));
    TEST_ASSERT_EQUAL(-1, wf_cssanim_range("10%", 0, 4));
    TEST_ASSERT_EQUAL(-1, wf_cssanim_range("10%", a, 0));
    /* the text of each side */
    TEST_ASSERT_EQUAL(2, wf_cssanim_range("entry 10% 90%, 12.5%", a, 4));
    TEST_ASSERT_EQUAL(16, wf_cssanim_range_text(a, 2, 0, t, (int)sizeof(t)));
    TEST_ASSERT_EQUAL_STRING("entry 10%, 12.5%", t);
    TEST_ASSERT_EQUAL(11, wf_cssanim_range_text(a, 2, 1, t, (int)sizeof(t)));
    TEST_ASSERT_EQUAL_STRING("90%, normal", t);
    TEST_ASSERT_EQUAL(0, wf_cssanim_range_text(a, 2, 1, t, 11));
    TEST_ASSERT_EQUAL_STRING("", t);
    TEST_ASSERT_EQUAL(0, wf_cssanim_range_text(a, 2, 1, t, 4));
    TEST_ASSERT_EQUAL(0, wf_cssanim_range_text(a, 2, 0, t, 6));
    TEST_ASSERT_EQUAL(0, wf_cssanim_range_text(a, 2, 0, t, 9));
    TEST_ASSERT_EQUAL(0, wf_cssanim_range_text(0, 1, 0, t, 8));
    TEST_ASSERT_EQUAL_STRING("", t);
    TEST_ASSERT_EQUAL(0, wf_cssanim_range_text(a, 1, 0, 0, 8));
    TEST_ASSERT_EQUAL(0, wf_cssanim_range_text(a, 1, 0, t, 0));
    a[0].rs = WF_CSSANIM_R_SCROLL + 1;     /* not a kind */
    TEST_ASSERT_EQUAL(0, wf_cssanim_range_text(a, 1, 0, t, (int)sizeof(t)));
    a[0].rs = WF_CSSANIM_R_SCROLL;
    a[0].rsp = -50000;
    TEST_ASSERT_EQUAL(10, wf_cssanim_range_text(a, 1, 0, t, (int)sizeof(t)));
    TEST_ASSERT_EQUAL_STRING("scroll -5%", t);
}

static const char *rm(int delay, int iters, int prog, int rs, int rsp, int re, int rep) {
    wf_cssanim_t a;
    int ms = -7, r;
    memset(&a, 0, sizeof(a));
    a.dur = 1000; a.delay = delay; a.iters = iters;
    a.rs = (unsigned char)rs; a.rsp = rsp; a.re = (unsigned char)re; a.rep = rep;
    r = wf_cssanim_range_ms(&a, prog, &ms);
    snprintf(g_o, sizeof(g_o), "%d %d", r, ms);
    return g_o;
}

/* the local time within the range */
static void test_l102_range_ms(void) {
    TEST_ASSERT_EQUAL_STRING("1 250", rm(0, 10000, 2500, 0, 0, 0, 0));
    /* 25% .. 75%: half way at 50% */
    TEST_ASSERT_EQUAL_STRING("1 500", rm(0, 10000, 5000, 1, 250000, 1, 750000));
    TEST_ASSERT_EQUAL_STRING("1 0", rm(0, 10000, 2500, 1, 250000, 1, 750000));
    TEST_ASSERT_EQUAL_STRING("1 -500", rm(0, 10000, 0, 1, 250000, 1, 750000));
    TEST_ASSERT_EQUAL_STRING("1 1500", rm(0, 10000, 10000, 1, 250000, 1, 750000));
    /* rounded down: -428.43 is -429 */
    TEST_ASSERT_EQUAL_STRING("1 -429", rm(0, 10000, 1, 1, 300000, 0, 0));
    TEST_ASSERT_EQUAL_STRING("1 428", rm(0, 10000, 4000, 1, 100000, 1, 800000));
    /* kept within -2e8 .. 2e8 */
    TEST_ASSERT_EQUAL_STRING("1 200000000", rm(0, 10000, 10000, 0, 0, 1, 1));
    TEST_ASSERT_EQUAL_STRING("1 -200000000", rm(0, 10000, 0, 1, 999999, 1, 1000000));
    TEST_ASSERT_EQUAL_STRING("1 1000", rm(0, 10000, 0, 1, -1, 1, 0));
    /* a range name is normal on a scroll timeline */
    TEST_ASSERT_EQUAL_STRING("1 250", rm(0, 10000, 2500, WF_CSSANIM_R_ENTRY, 100000, WF_CSSANIM_R_EXIT, 900000));
    TEST_ASSERT_EQUAL_STRING("1 250", rm(0, 10000, 2500, WF_CSSANIM_R_COVER, 0, 0, 0));
    /* the delay counts: (1000 + 1000) (50 - 25) / 50 */
    TEST_ASSERT_EQUAL_STRING("1 1000", rm(1000, 10000, 5000, 1, 250000, 1, 750000));
    /* an empty range, the Lot 101 checks */
    TEST_ASSERT_EQUAL_STRING("0 0", rm(0, 10000, 5000, 1, 500000, 1, 500000));
    TEST_ASSERT_EQUAL_STRING("0 0", rm(0, 10000, 5000, 1, 600000, 1, 400000));
    TEST_ASSERT_EQUAL_STRING("0 0", rm(0, 10000, 5000, 1, 1000000, 0, 0));
    TEST_ASSERT_EQUAL_STRING("0 0", rm(0, -1, 5000, 0, 0, 0, 0));
    TEST_ASSERT_EQUAL_STRING("0 0", rm(0, 10000, 10001, 0, 0, 0, 0));
    TEST_ASSERT_EQUAL_STRING("0 0", rm(0, 10000, -1, 0, 0, 0, 0));
    TEST_ASSERT_EQUAL_STRING("0 0", rm(-1000, 10000, 5000, 0, 0, 0, 0));
    {
        int ms = 5;
        TEST_ASSERT_EQUAL(0, wf_cssanim_range_ms(0, 0, &ms));
    }
}

/* timeline-scope */
static void test_l102_scope_api(void) {
    static wf_csstl_t t[WF_CSSANIM_MAX];
    TEST_ASSERT_EQUAL(3, WF_CSSTL_SCOPE);
    TEST_ASSERT_EQUAL(1, wf_csstl_parse(WF_CSSTL_SCOPE, "all", t, 4));
    TEST_ASSERT_EQUAL_STRING("", t[0].name);
    TEST_ASSERT_EQUAL(WF_CSSTL_ALL, t[0].axis);
    TEST_ASSERT_EQUAL(1, wf_csstl_parse(WF_CSSTL_SCOPE, "none", t, 4));
    TEST_ASSERT_EQUAL(0, t[0].axis);
    TEST_ASSERT_EQUAL(2, wf_csstl_parse(WF_CSSTL_SCOPE, "--a, --b", t, 4));
    TEST_ASSERT_EQUAL_STRING("--b", t[1].name);
    TEST_ASSERT_EQUAL(-1, wf_csstl_parse(WF_CSSTL_SCOPE, "--a, all", t, 4));
    TEST_ASSERT_EQUAL(-1, wf_csstl_parse(WF_CSSTL_SCOPE, "none, --a", t, 4));
    TEST_ASSERT_EQUAL(-1, wf_csstl_parse(WF_CSSTL_SCOPE, "auto", t, 4));
    TEST_ASSERT_EQUAL(-1, wf_csstl_parse(WF_CSSTL_SCOPE, "--a --b", t, 4));
    TEST_ASSERT_EQUAL(-1, wf_csstl_parse(WF_CSSTL_SCOPE + 1, "block", t, 4));     /* mutation witness */
    TEST_ASSERT_EQUAL(2, wf_csstl_scoped("all", "--t"));
    TEST_ASSERT_EQUAL(1, wf_csstl_scoped("--t, --u", "--u"));
    TEST_ASSERT_EQUAL(0, wf_csstl_scoped("--t", "--u"));
    TEST_ASSERT_EQUAL(0, wf_csstl_scoped("--tt", "--t"));
    TEST_ASSERT_EQUAL(0, wf_csstl_scoped("none", "--t"));
    TEST_ASSERT_EQUAL(0, wf_csstl_scoped("none", ""));
    TEST_ASSERT_EQUAL(0, wf_csstl_scoped("all, --t", "--t"));
    TEST_ASSERT_EQUAL(0, wf_csstl_scoped(0, "--t"));
    TEST_ASSERT_EQUAL(0, wf_csstl_scoped("--t", 0));
}

#define AN "#i { animation: k 1000ms linear; animation-timeline: "

/* timeline-scope in the reader */
static void test_l102_scope_reader(void) {
    prep(KB "html { scroll-timeline: --t } #i { animation: k 1000ms linear; animation-timeline: --t }", 0, 2500);
    TEST_ASSERT_EQUAL_STRING(" style=\"color:#404040\"", style_of("i"));
    /* body scopes the name: the timeline would be a descendant's */
    prep(KB "html { scroll-timeline: --t } body { timeline-scope: --t } " AN "--t }", 0, 2500);
    TEST_ASSERT_EQUAL_STRING("", style_of("i"));
    TEST_ASSERT_EQUAL(0, g_d.anim_scroll);
    prep(KB "html { scroll-timeline: --t } body { timeline-scope: --u, --t } " AN "--t }", 0, 2500);
    TEST_ASSERT_EQUAL_STRING("", style_of("i"));
    prep(KB "html { scroll-timeline: --t } body { timeline-scope: --u } " AN "--t }", 0, 2500);
    TEST_ASSERT_EQUAL_STRING(" style=\"color:#404040\"", style_of("i"));
    prep(KB "html { scroll-timeline: --t } #o { timeline-scope: --t } " AN "--t }", 0, 2500);
    TEST_ASSERT_EQUAL_STRING("", style_of("i"));
    /* on the element itself: its own name decides first */
    prep(KB "html { scroll-timeline: --t } #i { timeline-scope: --t } " AN "--t }", 0, 2500);
    TEST_ASSERT_EQUAL_STRING("", style_of("i"));
    prep(KB "html { scroll-timeline: --t; timeline-scope: --t } " AN "--t }", 0, 2500);
    TEST_ASSERT_EQUAL_STRING(" style=\"color:#404040\"", style_of("i"));
    prep(KB "html { timeline-scope: --t } " AN "--t }", 0, 2500);
    TEST_ASSERT_EQUAL_STRING("", style_of("i"));
    /* all: only names a descendant defines */
    prep(KB "html { scroll-timeline: --t } body { timeline-scope: all } " AN "--t }", 0, 2500);
    TEST_ASSERT_EQUAL_STRING(" style=\"color:#404040\"", style_of("i"));
    prep(KB "html { scroll-timeline: --t } body { timeline-scope: all } #q { view-timeline: --t } " AN "--t }", 0, 2500);
    TEST_ASSERT_EQUAL_STRING("", style_of("i"));
    prep(KB "html { scroll-timeline: --t } body { timeline-scope: all } #q { view-timeline: --u } " AN "--t }", 0, 2500);
    TEST_ASSERT_EQUAL_STRING(" style=\"color:#404040\"", style_of("i"));
    prep(KB "html { scroll-timeline: --t } body { timeline-scope: all } #q { scroll-timeline: --u, --t } " AN "--t }",
         0, 2500);
    TEST_ASSERT_EQUAL_STRING("", style_of("i"));
    /* a sibling's name scoped at body: none, so it does not reach html */
    prep(KB "html { scroll-timeline: --t } body { timeline-scope: --t } #q { scroll-timeline: --t } " AN "--t }", 0, 2500);
    TEST_ASSERT_EQUAL_STRING("", style_of("i"));
    /* a descendant name already scoped below is not taken by all */
    prep_doc("<html><head><style>" KB "html { scroll-timeline: --t } body { timeline-scope: all } "
             "#q { timeline-scope: --t } #s { view-timeline: --t } " AN "--t }</style></head><body>"
             "<div id=o><p id=i>i</p></div><div id=q><p id=s>s</p></div></body></html>", 2500);
    TEST_ASSERT_EQUAL_STRING(" style=\"color:#404040\"", style_of("i"));
    prep_doc("<html><head><style>" KB "html { scroll-timeline: --t } body { timeline-scope: all } "
             "#q { timeline-scope: all } #s { view-timeline: --t } " AN "--t }</style></head><body>"
             "<div id=o><p id=i>i</p></div><div id=q><p id=s>s</p></div></body></html>", 2500);
    TEST_ASSERT_EQUAL_STRING(" style=\"color:#404040\"", style_of("i"));
    prep_doc("<html><head><style>" KB "html { scroll-timeline: --t } body { timeline-scope: all } "
             "#q { timeline-scope: --u } #s { view-timeline: --t } " AN "--t }</style></head><body>"
             "<div id=o><p id=i>i</p></div><div id=q><p id=s>s</p></div></body></html>", 2500);
    TEST_ASSERT_EQUAL_STRING("", style_of("i"));
    /* a descendant of the animated element counts too */
    prep_doc("<html><head><style>" KB "html { scroll-timeline: --t } #o { timeline-scope: all } "
             "#s { scroll-timeline: --t } " AN "--t }</style></head><body>"
             "<div id=o><p id=i>i<b id=s>s</b></p></div></body></html>", 2500);
    TEST_ASSERT_EQUAL_STRING("", style_of("i"));
    /* initial and unset reset it; a bad value is dropped */
    prep(KB "html { scroll-timeline: --t } body { timeline-scope: --t; timeline-scope: initial } " AN "--t }", 0, 2500);
    TEST_ASSERT_EQUAL_STRING(" style=\"color:#404040\"", style_of("i"));
    prep(KB "html { scroll-timeline: --t } body { timeline-scope: --t; timeline-scope: unset } " AN "--t }", 0, 2500);
    TEST_ASSERT_EQUAL_STRING(" style=\"color:#404040\"", style_of("i"));
    prep(KB "html { scroll-timeline: --t } body { timeline-scope: --t; timeline-scope: auto } " AN "--t }", 0, 2500);
    TEST_ASSERT_EQUAL_STRING("", style_of("i"));
    prep(KB "html { scroll-timeline: --t } body { timeline-scope: --t; timeline-scope: none } " AN "--t }", 0, 2500);
    TEST_ASSERT_EQUAL_STRING(" style=\"color:#404040\"", style_of("i"));
    /* the strongest declaration wins; the list is kept as written */
    prep(KB "html { scroll-timeline: --t } html body { timeline-scope: none } body { timeline-scope: --t } " AN "--t }",
         0, 2500);
    TEST_ASSERT_EQUAL_STRING(" style=\"color:#404040\"", style_of("i"));
    prep(KB "html { scroll-timeline: --t } body { timeline-scope: none } html body { timeline-scope: --t } " AN "--t }",
         0, 2500);
    TEST_ASSERT_EQUAL_STRING("", style_of("i"));
    prep(KB "body { timeline-scope: --A, --b }", 0, 0);
    TEST_ASSERT_TRUE(node("i") > 0 && g_d.node[g_d.node[node("o")].parent].tl[4] > 0);
    TEST_ASSERT_EQUAL_STRING("--a, --b", g_d.lpool[g_d.node[g_d.node[node("o")].parent].tl[4] - 1]);
}

/* scroll() and view() in the reader */
static void test_l102_anon_reader(void) {
    prep(PR KB AN "scroll() }", 0, 2500);
    TEST_ASSERT_EQUAL_STRING(" style=\"color:#404040\"", style_of("i"));
    TEST_ASSERT_EQUAL(1, g_d.anim_scroll);
    TEST_ASSERT_EQUAL(0, g_d.anim_idle);
    TEST_ASSERT_EQUAL_STRING("keyframes 1 animated 0 transitioned 0 font 0 at 0ms scroll 1 pos 2500 color 1 0", anim_text());
    prep(KB AN "scroll(root) }", 750, 2500);
    TEST_ASSERT_EQUAL_STRING(" style=\"color:#404040\"", style_of("i"));
    prep(KB AN "scroll(nearest y) }", 0, 2500);
    TEST_ASSERT_EQUAL_STRING(" style=\"color:#404040\"", style_of("i"));
    prep(KB AN "scroll(root block) }", 0, 5000);
    TEST_ASSERT_EQUAL_STRING(" style=\"color:#808080\"", style_of("i"));
    /* self: only the root element; inline and x: no effect */
    prep(PR KB AN "scroll(self) }", 0, 2500);
    TEST_ASSERT_EQUAL_STRING("", style_of("i"));
    TEST_ASSERT_EQUAL(0, g_d.anim_scroll);
    TEST_ASSERT_EQUAL(1, g_d.anim_idle);
    prep(KB AN "scroll(inline) }", 0, 2500);
    TEST_ASSERT_EQUAL_STRING("", style_of("i"));
    prep(KB AN "scroll(x root) }", 0, 2500);
    TEST_ASSERT_EQUAL_STRING("", style_of("i"));
    /* view(): no layout, no effect */
    prep(PR KB AN "view() }", 0, 2500);
    TEST_ASSERT_EQUAL_STRING("", style_of("i"));
    TEST_ASSERT_EQUAL(1, g_d.anim_idle);
    prep(KB AN "view(block 10%) }", 500, 2500);
    TEST_ASSERT_EQUAL_STRING("", style_of("i"));
    /* infinite: no effect */
    prep(KB "#i { animation: k 1000ms linear infinite; animation-timeline: scroll() }", 0, 2500);
    TEST_ASSERT_EQUAL_STRING("", style_of("i"));
    /* the animation shorthand resets the timeline to auto (the clock) */
    prep(KB "#i { animation-timeline: scroll(); animation: k 1000ms linear }", 500, 2500);
    TEST_ASSERT_EQUAL_STRING(" style=\"color:#808080\"", style_of("i"));
    /* on the root element */
    prep_doc("<html id=h><head><style>" KB "html { animation: k 1000ms linear; animation-timeline: scroll(self) }"
             "</style></head><body>b</body></html>", 2500);
    TEST_ASSERT_EQUAL_STRING(" style=\"color:#404040\"", style_of("h"));
    prep_doc("<html id=h><head><style>" KB "html { animation: k 1000ms linear; animation-timeline: scroll(root) }"
             "</style></head><body>b</body></html>", 2500);
    TEST_ASSERT_EQUAL_STRING(" style=\"color:#404040\"", style_of("h"));
    prep_doc("<html id=h><head><style>" KB "html { animation: k 1000ms linear; animation-timeline: scroll() }"
             "</style></head><body>b</body></html>", 2500);
    TEST_ASSERT_EQUAL_STRING("", style_of("h"));
    /* a root body is not the root html element */
    prep_doc("<body id=h><style>" KB "body { animation: k 1000ms linear; animation-timeline: scroll(self) }"
             "</style>b</body>", 2500);
    TEST_ASSERT_EQUAL_STRING("", style_of("h"));
}

/* animation-range in the reader */
static void test_l102_range_reader(void) {
    prep(KB AN "scroll(); animation-range: 25% 75% }", 0, 5000);
    TEST_ASSERT_EQUAL_STRING(" style=\"color:#808080\"", style_of("i"));
    prep(KB AN "scroll(); animation-range: 25% 75% }", 0, 2500);
    TEST_ASSERT_EQUAL_STRING(" style=\"color:#000000\"", style_of("i"));
    /* before the range: the before phase (backwards fill), after it the
     * after phase */
    prep(KB AN "scroll(); animation-range: 25% 75% }", 0, 1000);
    TEST_ASSERT_EQUAL_STRING("", style_of("i"));
    TEST_ASSERT_EQUAL(1, g_d.anim_scroll);
    prep(KB "#i { animation: k 1000ms linear both; animation-timeline: scroll(); animation-range: 25% 75% }", 0, 1000);
    TEST_ASSERT_EQUAL_STRING(" style=\"color:#000000\"", style_of("i"));
    prep(KB AN "scroll(); animation-range: 25% 75% }", 0, 7500);
    TEST_ASSERT_EQUAL_STRING("", style_of("i"));
    prep(KB "#i { animation: k 1000ms linear forwards; animation-timeline: scroll(); animation-range: 25% 75% }", 0, 9000);
    TEST_ASSERT_EQUAL_STRING(" style=\"color:#ffffff\"", style_of("i"));
    /* longhands */
    prep(KB AN "scroll(); animation-range-start: 50% }", 0, 7500);
    TEST_ASSERT_EQUAL_STRING(" style=\"color:#808080\"", style_of("i"));
    prep(KB AN "scroll(); animation-range-end: 50% }", 0, 2500);
    TEST_ASSERT_EQUAL_STRING(" style=\"color:#808080\"", style_of("i"));
    prep(KB AN "scroll(); animation-range-end: 50%; animation-range-start: 0 }", 0, 2500);
    TEST_ASSERT_EQUAL_STRING(" style=\"color:#808080\"", style_of("i"));
    /* a named timeline too */
    prep(KB "html { scroll-timeline: --t } " AN "--t; animation-range: 0% 50% }", 0, 2500);
    TEST_ASSERT_EQUAL_STRING(" style=\"color:#808080\"", style_of("i"));
    /* range names are normal on a scroll timeline */
    prep(KB AN "scroll(); animation-range: entry }", 0, 2500);
    TEST_ASSERT_EQUAL_STRING(" style=\"color:#404040\"", style_of("i"));
    prep(KB AN "scroll(); animation-range: entry 50% 75% }", 0, 2500);
    TEST_ASSERT_EQUAL_STRING(" style=\"color:#555555\"", style_of("i"));
    /* mutation witness: the end list keeps the shorthand's own layer (b),
     * not that of an earlier list-style shorthand in layer a, so a later
     * end in layer a loses: 20% to 60%, not to 100% (#101010) */
    prep(KB "@layer a, b; @layer a { #i { animation: k 1000ms linear; animation-timeline: scroll(); "
         "list-style: square } } @layer b { #i { animation-range: 20% 60% } } "
         "@layer a { #i { animation-range-end: 100% } }", 0, 2500);
    TEST_ASSERT_EQUAL_STRING(" style=\"color:#202020;list-style-type:square\"", style_of("i"));
    /* an empty range: no effect */
    prep(PR KB AN "scroll(); animation-range: 50% 50% }", 0, 5000);
    TEST_ASSERT_EQUAL_STRING("", style_of("i"));
    TEST_ASSERT_EQUAL(0, g_d.anim_scroll);
    TEST_ASSERT_EQUAL(1, g_d.anim_idle);
    /* the animation shorthand resets the range */
    prep(KB "#i { animation-range: 25% 75%; animation: k 1000ms linear; animation-timeline: scroll() }", 0, 2500);
    TEST_ASSERT_EQUAL_STRING(" style=\"color:#404040\"", style_of("i"));
    prep(KB "#i { animation-range-start: 25%; animation: k 1000ms linear; animation-timeline: scroll() }", 0, 2500);
    TEST_ASSERT_EQUAL_STRING(" style=\"color:#404040\"", style_of("i"));
    /* initial resets it, a bad value is dropped, the strongest wins */
    prep(KB AN "scroll(); animation-range: 25% 75%; animation-range: initial }", 0, 2500);
    TEST_ASSERT_EQUAL_STRING(" style=\"color:#404040\"", style_of("i"));
    prep(KB AN "scroll(); animation-range: 25% 75%; animation-range: unset }", 0, 2500);
    TEST_ASSERT_EQUAL_STRING(" style=\"color:#404040\"", style_of("i"));
    prep(KB AN "scroll(); animation-range: 25% 75%; animation-range: 10px }", 0, 2500);
    TEST_ASSERT_EQUAL_STRING(" style=\"color:#000000\"", style_of("i"));
    prep(KB AN "scroll(); animation-range-start: 25%; animation-range-start: initial }", 0, 2500);
    TEST_ASSERT_EQUAL_STRING(" style=\"color:#404040\"", style_of("i"));
    prep(KB AN "scroll() } p { animation-range: 0% 50% } #i { animation-range: 25% 75% }", 0, 2500);
    TEST_ASSERT_EQUAL_STRING(" style=\"color:#000000\"", style_of("i"));
    prep(KB AN "scroll(); animation-range: 25% 75% } p { animation-range: 0% 50% }", 0, 2500);
    TEST_ASSERT_EQUAL_STRING(" style=\"color:#000000\"", style_of("i"));
    /* the shorthand's lists are kept as longhand lists */
    prep(KB AN "scroll(); animation-range: entry, 10% }", 0, 2500);
    {
        int i = node("i");
        TEST_ASSERT_TRUE(i >= 0);
        TEST_ASSERT_EQUAL_STRING("entry 0%, 10%", g_d.lpool[g_d.node[i].al[WF_CSSANIM_L_RANGE_START] - 1]);
        TEST_ASSERT_EQUAL_STRING("entry 100%, normal", g_d.lpool[g_d.node[i].al[WF_CSSANIM_L_RANGE_END] - 1]);
    }
    /* the transition longhands moved two slots on (Lot 102) */
    prep("#i { color: red; transition-duration: 1s; transition-property: color }", 0, 0);
    TEST_ASSERT_TRUE(g_d.node[node("i")].al[WF_CSSANIM_LONGS + WF_CSSTRANS_L_DURATION] > 0);
    TEST_ASSERT_EQUAL_STRING("1s", g_d.lpool[g_d.node[node("i")].al[WF_CSSANIM_LONGS + WF_CSSTRANS_L_DURATION] - 1]);
    TEST_ASSERT_EQUAL(17, WF_SITE_ALONGS);
    TEST_ASSERT_EQUAL(5, WF_SITE_TLS);
    TEST_ASSERT_EQUAL(WF_SITE_K_LONG0 + WF_SITE_ALONGS, WF_SITE_K_TL0);
}

/* none in the reader paint and in keyframes */
static void test_l102_none_reader(void) {
    prep("#i { color: rgb(none 128 255 / none) } #q { background-color: hsl(none 100% 50%) }", 0, 0);
    TEST_ASSERT_EQUAL_STRING(" style=\"color:#0080ff00\"", style_of("i"));
    TEST_ASSERT_EQUAL_STRING(" style=\"background-color:#ff0000\"", style_of("q"));
    /* keyframes interpolate with the carried channel: at 50%, red stays */
    prep("@keyframes k { from { color: rgb(none 0 0) } to { color: rgb(255 255 255) } } "
         "#i { animation: k 1000ms linear }", 500, 0);
    TEST_ASSERT_EQUAL_STRING(" style=\"color:#ff8080\"", style_of("i"));
    prep("@keyframes k { from { color: rgb(none 0 0) } to { color: rgb(255 255 255) } } "
         "#i { animation: k 1000ms linear; animation-timeline: scroll() }", 0, 5000);
    TEST_ASSERT_EQUAL_STRING(" style=\"color:#ff8080\"", style_of("i"));
}

int main(void) {
    unity_init();
    RUN_TEST(test_l102_none_forms);
    RUN_TEST(test_l102_none_carry);
    RUN_TEST(test_l102_anon_api);
    RUN_TEST(test_l102_range_api);
    RUN_TEST(test_l102_range_ms);
    RUN_TEST(test_l102_scope_api);
    RUN_TEST(test_l102_scope_reader);
    RUN_TEST(test_l102_anon_reader);
    RUN_TEST(test_l102_range_reader);
    RUN_TEST(test_l102_none_reader);
    unity_print_results();
    unity_cleanup();
    return unity_stats.tests_failed == 0 ? 0 : 1;
}
