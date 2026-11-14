/* Lot 100 (US-031): hand-written cases next to the generated suites
 * (test_lot100_color.c, test_lot100_mix.c, test_lot100_paint.c,
 * test_lot100_anim.c): the CSS Color 4 names, hwb(), color-mix() in
 * srgb, translucent colours painted as #rrggbbaa, the background
 * shorthand colour word, @starting-style colours, keyframes and
 * transitions on color and background-color, the summary line and the
 * API guards. Each value below is worked out from the specs in
 * docs/lot100_plan.md. */
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

/* the reader colour of v[0..n) as "#rrggbb alpha", "-" when refused */
static const char *rcn(const char *v, int n) {
    unsigned int c = 0x12345678u;
    int a = -3;
    if (!wf_cssprop_color(v, n, &c, &a)) return c == 0x12345678u && a == -3 ? "-" : "WRITTEN";
    snprintf(g_o, sizeof(g_o), "#%06x %d", c, a);
    return g_o;
}

static const char *rc(const char *v) { return rcn(v, (int)strlen(v)); }

static int syn(const char *s) {
    memset(&g_p, 0, sizeof(g_p));
    g_p.syntax = (unsigned char)wf_cssprop_syntax_parse(s, (int)strlen(s), &g_p);
    return g_p.syntax != 0;
}

static const char *ip(const char *a, const char *b, int t) {
    static char out[160];
    if (!syn("'<color>'")) return "BAD SYNTAX";
    if (wf_cssprop_interp(&g_p, a, b, t, out, (int)sizeof(out)) <= 0) return "(none)";
    return out;
}

/* "red" wrapped n times in "color-mix(in srgb," and ",blue)" */
static const char *nest(int n) {
    static char t[256];
    int k;
    strcpy(t, "red");
    for (k = 0; k < n; k++) {
        char tmp[256];
        snprintf(tmp, sizeof(tmp), "color-mix(in srgb,%s,blue)", t);
        strcpy(t, tmp);
    }
    return t;
}

/* the CSS Color 4 names: the X11 values with the CSS differences */
static void test_l100_names(void) {
    TEST_ASSERT_EQUAL_STRING("#663399 255", rc("rebeccapurple"));
    TEST_ASSERT_EQUAL_STRING("#a9a9a9 255", rc("DarkGrey"));
    TEST_ASSERT_EQUAL_STRING("#a9a9a9 255", rc("darkgray"));
    TEST_ASSERT_EQUAL_STRING("#fafad2 255", rc("lightgoldenrodyellow"));
    /* the names where CSS and X11 differ */
    TEST_ASSERT_EQUAL_STRING("#808080 255", rc("gray"));
    TEST_ASSERT_EQUAL_STRING("#008000 255", rc("green"));
    TEST_ASSERT_EQUAL_STRING("#800000 255", rc("maroon"));
    TEST_ASSERT_EQUAL_STRING("#800080 255", rc("purple"));
    TEST_ASSERT_EQUAL_STRING("#00ffff 255", rc("aqua"));
    TEST_ASSERT_EQUAL_STRING("#00ffff 255", rc("cyan"));
    TEST_ASSERT_EQUAL_STRING("#ff00ff 255", rc("fuchsia"));
    TEST_ASSERT_EQUAL_STRING("#9acd32 255", rc("yellowgreen"));       /* the last entry */
    TEST_ASSERT_EQUAL_STRING("#f0f8ff 255", rc("aliceblue"));         /* the first one */
    TEST_ASSERT_EQUAL_STRING("#000000 0", rc("transparent"));
    /* currentcolor has no value of its own in the reader */
    TEST_ASSERT_EQUAL_STRING("-", rc("currentcolor"));
    TEST_ASSERT_EQUAL_STRING("-", rc("reds"));
    TEST_ASSERT_EQUAL_STRING("-", rc("re"));
    TEST_ASSERT_EQUAL_STRING("-", rc("aliceblu"));
    TEST_ASSERT_EQUAL_STRING("-", rc("yellowgreenx"));
    /* a registered <color> takes every name too (Lot 97 kept 18) */
    TEST_ASSERT(syn("'<color>'"));
    TEST_ASSERT(wf_cssprop_match_prop(&g_p, "rebeccapurple"));
    TEST_ASSERT(wf_cssprop_match_prop(&g_p, "hwb(1 2% 3%)"));
    TEST_ASSERT(!wf_cssprop_match_prop(&g_p, "hwb(1, 2%, 3%)"));
    TEST_ASSERT(!wf_cssprop_match_prop(&g_p, "rebeccapurplex"));
    TEST_ASSERT_EQUAL_STRING("rgb(128, 128, 128)", ip("white", "black", 5000));
    TEST_ASSERT_EQUAL_STRING("rgb(51, 26, 141)", ip("rebeccapurple", "navy", 5000));
}

/* alpha in the reader colour: hex, rgb() and hsl() */
static void test_l100_alpha(void) {
    TEST_ASSERT_EQUAL_STRING("#ff0000 128", rc("#ff000080"));
    TEST_ASSERT_EQUAL_STRING("#ff0000 136", rc("#f008"));
    TEST_ASSERT_EQUAL_STRING("#0000ff 128", rc("rgba(0, 0, 255, 0.5)"));      /* 127.5 rounds up */
    TEST_ASSERT_EQUAL_STRING("#0000ff 64", rc("rgb(0 0 255 / 25%)"));         /* 63.75 */
    TEST_ASSERT_EQUAL_STRING("#00ff00 0", rc("hsl(120 100% 50% / 0)"));
    TEST_ASSERT_EQUAL_STRING("#ff0000 255", rc("rgb(300, -5, 0, 2)"));         /* clamped */
    TEST_ASSERT_EQUAL_STRING("#80ff00 255", rc("rgb(50% 100% 0%)"));
    TEST_ASSERT_EQUAL_STRING("-", rc("rgb(1 2 3 4)"));
    TEST_ASSERT_EQUAL_STRING("-", rc("rgb(1, 2 3)"));
    TEST_ASSERT_EQUAL_STRING("-", rc("#ff00008"));
}

/* hwb(): CSS Color 4 section 8 */
static void test_l100_hwb(void) {
    TEST_ASSERT_EQUAL_STRING("#ff0000 255", rc("hwb(0 0% 0%)"));
    /* g = 1 x 0.5 + 0.3 = 0.8 (204), r = b = 0.3 (76.5 rounds up) */
    TEST_ASSERT_EQUAL_STRING("#4dcc4d 255", rc("hwb(120 30% 20%)"));
    TEST_ASSERT_EQUAL_STRING("#0000ff 128", rc("hwb(240 0% 0% / 0.5)"));
    /* W + B over 100%: the grey W / (W + B) */
    TEST_ASSERT_EQUAL_STRING("#808080 255", rc("hwb(0 60% 60%)"));
    TEST_ASSERT_EQUAL_STRING("#ffffff 255", rc("hwb(90deg 100% 0%)"));
    TEST_ASSERT_EQUAL_STRING("#000000 255", rc("hwb(90deg 0% 100%)"));
    /* numbers are percentages; the hue in any angle unit */
    TEST_ASSERT_EQUAL_STRING("#00ffff 255", rc("hwb(0.5turn 0 0)"));
    TEST_ASSERT_EQUAL_STRING("#00ffff 255", rc("HWB(200GRAD 0% 0%)"));
    TEST_ASSERT_EQUAL_STRING("#ff0000 255", rc("hwb(-360deg 0% 0%)"));
    TEST_ASSERT_EQUAL_STRING("#ffffff 255", rc("hwb(1 150% 0%)"));           /* W clamped to 100% */
    TEST_ASSERT_EQUAL_STRING("#ff0000 255", rc("hwb(0 -10% 0%)"));
    TEST_ASSERT_EQUAL_STRING("-", rc("hwb(120, 30%, 20%)"));                 /* no legacy commas */
    TEST_ASSERT_EQUAL_STRING("-", rc("hwb(120 30%, 20%)"));
    TEST_ASSERT_EQUAL_STRING("-", rc("hwb(1 2% 3% 4%)"));
    TEST_ASSERT_EQUAL_STRING("-", rc("hwb(1 2%)"));
    TEST_ASSERT_EQUAL_STRING("-", rc("hwba(1 2% 3%)"));
    TEST_ASSERT_EQUAL_STRING("-", rc("hwb(1px 2% 3%)"));
    TEST_ASSERT_EQUAL_STRING("-", rc("hwb(1 2% 3%"));
    TEST_ASSERT_EQUAL_STRING("-", rc("hwb()"));
    TEST_ASSERT_EQUAL_STRING("-", rc("hwb(0 0% 0% / x)"));                   /* the alpha is a number */
    /* each channel is rounded to 1/10000 first: 255 x 2.549% is 6.49995,
     * so 6.5000 then 7 (a floor would give 6) */
    TEST_ASSERT_EQUAL_STRING("#070000 255", rc("hwb(0 0% 97.451%)"));
    TEST_ASSERT_EQUAL_STRING("#060000 255", rc("hwb(0 0% 97.4511%)"));
}

/* color-mix(in srgb, ...): CSS Color 5 section 2 */
static void test_l100_mix(void) {
    TEST_ASSERT_EQUAL_STRING("#800080 255", rc("color-mix(in srgb, red, blue)"));
    TEST_ASSERT_EQUAL_STRING("#4000bf 255", rc("color-mix(in srgb, red 25%, blue)"));
    TEST_ASSERT_EQUAL_STRING("#4000bf 255", rc("color-mix(in srgb, red, blue 75%)"));
    TEST_ASSERT_EQUAL_STRING("#4000bf 255", rc("color-mix(in srgb, 25% red, 75% blue)"));
    /* a sum under 100% scales the alpha: 0.4 x 255 = 102 */
    TEST_ASSERT_EQUAL_STRING("#800080 102", rc("color-mix(in srgb, red 20%, blue 20%)"));
    /* a sum over 100% normalises */
    TEST_ASSERT_EQUAL_STRING("#800080 255", rc("color-mix(in srgb, red 80%, blue 80%)"));
    /* premultiplied: transparent does not darken */
    TEST_ASSERT_EQUAL_STRING("#ff0000 128", rc("color-mix(in srgb, red, transparent)"));
    TEST_ASSERT_EQUAL_STRING("#000000 0", rc("color-mix(in srgb, transparent, #0000)"));
    TEST_ASSERT_EQUAL_STRING("#ff0000 255", rc("color-mix(in srgb, red 100%, blue)"));
    TEST_ASSERT_EQUAL_STRING("#0000ff 255", rc("color-mix(in srgb, red 0%, blue)"));
    TEST_ASSERT_EQUAL_STRING("#800080 255", rc("COLOR-MIX(IN  SRGB , RED , BLUE)"));
    TEST_ASSERT_EQUAL_STRING("-", rc("color-mix(in srgb, red 0%, blue 0%)"));
    TEST_ASSERT_EQUAL_STRING("-", rc("color-mix(in srgb, red 101%, blue)"));
    TEST_ASSERT_EQUAL_STRING("-", rc("color-mix(in srgb, red -1%, blue)"));
    TEST_ASSERT_EQUAL_STRING("-", rc("color-mix(in srgb, red 50, blue)"));
    TEST_ASSERT_EQUAL_STRING("-", rc("color-mix(in srgb, red 10% 20%, blue)"));
    TEST_ASSERT_EQUAL_STRING("-", rc("color-mix(in hsl, red, blue)"));
    TEST_ASSERT_EQUAL_STRING("-", rc("color-mix(in srgb-linear, red, blue)"));
    TEST_ASSERT_EQUAL_STRING("-", rc("color-mix(insrgb, red, blue)"));
    TEST_ASSERT_EQUAL_STRING("-", rc("color-mix(srgb, red, blue)"));
    TEST_ASSERT_EQUAL_STRING("-", rc("color-mix(on srgb, red, blue)"));
    TEST_ASSERT_EQUAL_STRING("-", rc("color-mix(in srgb, red)"));
    TEST_ASSERT_EQUAL_STRING("-", rc("color-mix(in srgb, red, blue, lime)"));
    TEST_ASSERT_EQUAL_STRING("-", rc("color-mix(in srgb, , blue)"));
    TEST_ASSERT_EQUAL_STRING("-", rc("color-mix(in srgb, currentcolor, blue)"));
    TEST_ASSERT_EQUAL_STRING("-", rc("color-mix(in srgb, red), blue)"));
    /* a run of spaces inside an item is one separator */
    TEST_ASSERT_EQUAL_STRING("#800080 255", rc("color-mix(in srgb, red  50%, blue)"));
    TEST_ASSERT_EQUAL_STRING("#800080 255", rc("color-mix(in srgb, red 50%,  blue  )"));
    /* stray or unclosed parentheses leave no colour */
    TEST_ASSERT(!wf_cssprop_match(WF_CSSPROP_COLOR, "color-mix(in srgb, red, blue))"));
    TEST_ASSERT(!wf_cssprop_match(WF_CSSPROP_COLOR, "color-mix(in srgb, red), (blue)"));
    TEST_ASSERT(!wf_cssprop_match(WF_CSSPROP_COLOR, "color-mix(in srgb, red, rgb(1, 2, 3)"));
    TEST_ASSERT(!wf_cssprop_match(WF_CSSPROP_COLOR, "color-mix(in srgb, red ) (, blue)"));
    /* nesting: four color-mix() levels, not five */
    TEST_ASSERT_EQUAL(99, (int)strlen(nest(4)));
    TEST_ASSERT_EQUAL_STRING("#1000ef 255", rc(nest(4)));
    TEST_ASSERT_EQUAL(123, (int)strlen(nest(5)));
    TEST_ASSERT_EQUAL_STRING("-", rc(nest(5)));
    /* interpolated as a registered <color> (premultiplied sRGB) */
    TEST_ASSERT_EQUAL_STRING("rgb(191, 0, 64)", ip("color-mix(in srgb, red, red)", "hwb(240 0% 0%)", 2500));
    /* a colour part of 128 bytes or more is not taken (the item buffer):
     * the interpolation flips */
    {
        static char v[300], out[160];
        strcpy(v, "color-mix(in srgb, rgb(1, 2, 3");
        while (strlen(v) < 160) strcat(v, " ");
        strcat(v, "), red)");
        TEST_ASSERT(syn("'<color>'"));
        TEST_ASSERT_EQUAL(1, wf_cssprop_interp(&g_p, v, "red", 5000, out, (int)sizeof(out)));
        TEST_ASSERT_EQUAL_STRING("red", out);
        TEST_ASSERT_EQUAL(2, wf_cssprop_interp(&g_p, "color-mix(in srgb, rgb(1, 2, 3), red)", "red", 5000, out,
                                               (int)sizeof(out)));
        TEST_ASSERT_EQUAL_STRING("rgb(192, 1, 1)", out);
        /* the <color> match takes any length: the 160-byte item is refused
         * by the item buffer bound */
        TEST_ASSERT(!wf_cssprop_match(WF_CSSPROP_COLOR, v));
        TEST_ASSERT(wf_cssprop_match(WF_CSSPROP_COLOR, "color-mix(in srgb, rgb(1, 2, 3), red)"));
    }
}

/* the text bounds of wf_cssprop_color: n, terminators, 128 bytes */
static void test_l100_bounds(void) {
    char t[200];
    int k;
    unsigned int c = 7;
    int a = 9;
    TEST_ASSERT_EQUAL_STRING("#ff0000 255", rc("  red  "));
    TEST_ASSERT_EQUAL_STRING("#ff0000 255", rc("red;x"));
    TEST_ASSERT_EQUAL_STRING("#ff0000 255", rc("red !important"));
    TEST_ASSERT_EQUAL_STRING("-", rc("red x"));
    TEST_ASSERT_EQUAL_STRING("-", rc("#abc x"));        /* Lot 100: no trailing word (was taken) */
    TEST_ASSERT_EQUAL_STRING("-", rc("hwb(1 2% 3%) x"));
    TEST_ASSERT_EQUAL_STRING("-", rc("red)"));
    TEST_ASSERT_EQUAL_STRING("-", rc(""));
    TEST_ASSERT_EQUAL_STRING("-", rc("   "));
    TEST_ASSERT_EQUAL_STRING("#ff0000 255", rcn("redblue", 3));
    TEST_ASSERT_EQUAL_STRING("-", rcn("hwb(0 0% 0%)", 11));
    TEST_ASSERT_EQUAL_STRING("#ff0000 255", rcn("hwb(0 0% 0%)xyz", 12));
    /* 127 bytes are taken, 128 are not (the buffer) */
    strcpy(t, "color-mix(in");
    for (k = 0; k < 127 - 12 - (int)strlen(" srgb, red, blue)"); k++) strcat(t, " ");
    strcat(t, " srgb, red, blue)");
    TEST_ASSERT_EQUAL(127, (int)strlen(t));
    TEST_ASSERT_EQUAL_STRING("#800080 255", rc(t));
    memmove(t + 13, t + 12, strlen(t + 12) + 1);
    t[12] = ' ';
    TEST_ASSERT_EQUAL(128, (int)strlen(t));
    TEST_ASSERT_EQUAL_STRING("-", rc(t));
    /* a percentage token of 32 bytes inside color-mix() is not a number
     * (31 is) */
    TEST_ASSERT(syn("'<color>'"));
    TEST_ASSERT(!wf_cssprop_match_prop(&g_p, "color-mix(in srgb, red 1.000000000000000000000000000000%, blue)"));
    TEST_ASSERT(wf_cssprop_match_prop(&g_p, "color-mix(in srgb, red 1.0000000000000000000000000000%, blue)"));
    /* the guards */
    TEST_ASSERT_EQUAL(0, wf_cssprop_color(0, 3, &c, &a));
    TEST_ASSERT_EQUAL(0, wf_cssprop_color("red", 3, 0, &a));
    TEST_ASSERT_EQUAL(0, wf_cssprop_color("red", 3, &c, 0));
    TEST_ASSERT_EQUAL(0, wf_cssprop_color("red", 0, &c, &a));
    TEST_ASSERT_EQUAL(7, (int)c);
    TEST_ASSERT_EQUAL(9, a);
}

/* the reader paints them: #rrggbb, or #rrggbbaa below alpha 255 */
static void test_l100_paint(void) {
    prep("#o { color: hwb(120 30% 20%); background-color: color-mix(in srgb, red 20%, blue 20%) }", 0);
    TEST_ASSERT_EQUAL_STRING(" style=\"color:#4dcc4d;background-color:#80008066\"", style_of("o"));
    prep("#o { color: RebeccaPurple }", 0);
    TEST_ASSERT_EQUAL_STRING(" style=\"color:#663399\"", style_of("o"));
    prep("#o { color: transparent }", 0);
    TEST_ASSERT_EQUAL_STRING(" style=\"color:#00000000\"", style_of("o"));
    /* background-color: transparent is still no background */
    prep("#o { background-color: blue } #o { background-color: transparent }", 0);
    TEST_ASSERT_EQUAL_STRING("", style_of("o"));
    prep("#o { background-color: blue } #o { background-color: rgba(0, 0, 0, 0) }", 0);
    TEST_ASSERT_EQUAL_STRING(" style=\"background-color:#00000000\"", style_of("o"));
    prep("#o { --c: hwb(0 0% 0% / 50%) } #o { background-color: var(--c) }", 0);
    TEST_ASSERT_EQUAL_STRING(" style=\"background-color:#ff000080\"", style_of("o"));
    prep("#o { color: blue } #o { color: red x }", 0);
    TEST_ASSERT_EQUAL_STRING(" style=\"color:#0000ff\"", style_of("o"));
    /* the background shorthand: any reader colour as its colour word */
    prep("#o { background: url(x.png) darkorange }", 0);
    TEST_ASSERT_EQUAL_STRING(" style=\"background-color:#ff8c00;background-image:url(x.png)\"", style_of("o"));
    prep("#o { background: hwb(0 0% 0% / 0.5) no-repeat }", 0);
    TEST_ASSERT_NOT_NULL(strstr(style_of("o"), "background-color:#ff000080"));
    /* a word with '!' or ';' is not a colour (it was taken in Lot 99) */
    prep("#o { background-color: blue } #o { background: red!x }", 0);
    TEST_ASSERT_EQUAL_STRING(" style=\"background-color:#0000ff\"", style_of("o"));
    prep("#o { background-color: blue } #o { background: url(a.png) hsl(1 2% 3%)!x }", 0);
    TEST_ASSERT_EQUAL_STRING(" style=\"background-color:#0000ff\"", style_of("o"));
    /* the colour word stays under 32 bytes (Lot 90): 31 and 32 */
    prep("#o { background-color: blue } #o { background: color-mix(in srgb,red,    blue) }", 0);
    TEST_ASSERT_EQUAL_STRING(" style=\"background-color:#800080\"", style_of("o"));
    prep("#o { background-color: blue } #o { background: color-mix(in srgb,red,     blue) }", 0);
    TEST_ASSERT_EQUAL_STRING(" style=\"background-color:#0000ff\"", style_of("o"));
    {
        wf_cssbg_t bg;
        TEST_ASSERT(wf_cssbg_shorthand("lightgoldenrodyellow", &bg));
        TEST_ASSERT_EQUAL_STRING("lightgoldenrodyellow", bg.color);
        TEST_ASSERT(!wf_cssbg_shorthand("red;", &bg));
        TEST_ASSERT(!wf_cssbg_shorthand("nosuchcolour", &bg));
    }
}

/* keyframes on color and background-color */
static void test_l100_keyframes(void) {
    prep("@keyframes k { from { color: red } to { color: blue } } #o { animation: k 1000ms linear }", 500);
    TEST_ASSERT_EQUAL_STRING(" style=\"color:#800080\"", style_of("o"));
    TEST_ASSERT_EQUAL(1, g_d.col_animated);
    TEST_ASSERT_EQUAL_STRING("keyframes 1 animated 0 transitioned 0 font 0 at 500ms color 1 0", anim_text());
    /* the child is not animated: it inherits in the engine */
    TEST_ASSERT_EQUAL_STRING("", style_of("i"));
    /* a missing 0% frame takes the own colour; the background below is
     * transparent: half way to red is red at alpha 0.5 */
    prep("@keyframes k { to { background-color: red } } #o { animation: k 1s linear }", 500);
    TEST_ASSERT_EQUAL_STRING(" style=\"background-color:#ff000080\"", style_of("o"));
    prep("@keyframes k { to { color: blue } } #o { color: red; animation: k 1s linear }", 250);
    TEST_ASSERT_EQUAL_STRING(" style=\"color:#bf0040\"", style_of("o"));
    /* with no own colour: the parent's, then black */
    prep("@keyframes k { to { color: blue } } #o { color: red } #i { animation: k 1s linear }", 500);
    TEST_ASSERT_EQUAL_STRING(" style=\"color:#800080\"", style_of("i"));
    TEST_ASSERT_EQUAL_STRING(" style=\"color:#ff0000\"", style_of("o"));
    /* the nearest ancestor with a colour: here the grandparent */
    prep("body { color: red } @keyframes k { to { color: blue } } #i { animation: k 1s linear }", 500);
    TEST_ASSERT_EQUAL_STRING(" style=\"color:#800080\"", style_of("i"));
    TEST_ASSERT_EQUAL_STRING("", style_of("o"));
    prep("@keyframes k { to { color: white } } #o { animation: k 1s linear }", 500);
    TEST_ASSERT_EQUAL_STRING(" style=\"color:#808080\"", style_of("o"));
    /* !important keeps its colour against the animation */
    prep("@keyframes k { to { color: blue } } #o { color: red !important; animation: k 1s linear }", 500);
    TEST_ASSERT_EQUAL_STRING(" style=\"color:#ff0000\"", style_of("o"));
    TEST_ASSERT_EQUAL(0, g_d.col_animated);
    /* add: the frames sit on the colour below (200 = 100 + 100 at 0%) */
    prep("@keyframes k { to { color: rgb(0, 0, 100) } } #o { color: rgb(100, 0, 0); animation: k 1s linear; "
         "animation-composition: add }", 500);
    TEST_ASSERT_EQUAL_STRING(" style=\"color:#960032\"", style_of("o"));
    TEST_ASSERT_EQUAL(1, g_d.prop_composed);
    /* hwb() and color-mix() frames; a frame that is not a colour is
     * dropped; after the end with no fill the own colour is back */
    prep("@keyframes k { 0% { color: hwb(0 0% 0%) } 100% { color: color-mix(in srgb, blue, blue) } 50% { color: 1px } }"
         " #o { animation: k 1s linear }", 750);
    TEST_ASSERT_EQUAL_STRING(" style=\"color:#4000bf\"", style_of("o"));
    prep("@keyframes k { to { color: blue } } #o { color: red; animation: k 1s linear }", 1500);
    TEST_ASSERT_EQUAL_STRING(" style=\"color:#ff0000\"", style_of("o"));
    TEST_ASSERT_EQUAL(0, g_d.col_animated);
    /* animation-timeline: none has no effect */
    prep("@keyframes k { to { color: blue } } #o { color: red; animation: k 1s linear; animation-timeline: none }", 500);
    TEST_ASSERT_EQUAL_STRING(" style=\"color:#ff0000\"", style_of("o"));
}

/* transitions from the @starting-style colours */
static void test_l100_starting(void) {
    prep("#o { color: blue; transition: color 1s linear } @starting-style { #o { color: red } }", 250);
    TEST_ASSERT_EQUAL_STRING(" style=\"color:#bf0040\"", style_of("o"));
    TEST_ASSERT_EQUAL(1, g_d.col_transitioned);
    TEST_ASSERT_EQUAL_STRING("keyframes 0 animated 0 transitioned 0 font 0 at 250ms color 0 1", anim_text());
    /* in the delay: the starting colour; after the end: the own one */
    prep("#o { color: blue; transition: color 1s 500ms linear } @starting-style { #o { color: red } }", 250);
    TEST_ASSERT_EQUAL_STRING(" style=\"color:#ff0000\"", style_of("o"));
    prep("#o { color: blue; transition: color 1s linear } @starting-style { #o { color: red } }", 1000);
    TEST_ASSERT_EQUAL_STRING(" style=\"color:#0000ff\"", style_of("o"));
    TEST_ASSERT_EQUAL(0, g_d.col_transitioned);
    /* a transition of another property does not run */
    prep("#o { color: blue; transition: background-color 1s linear } @starting-style { #o { color: red } }", 250);
    TEST_ASSERT_EQUAL_STRING(" style=\"color:#0000ff\"", style_of("o"));
    /* all; the background from transparent */
    prep("#o { background-color: red; transition: all 1s linear } @starting-style { #o { background-color: transparent } }",
         250);
    TEST_ASSERT_EQUAL_STRING(" style=\"background-color:#ff000040\"", style_of("o"));
    /* the later starting rule wins; no starting colour, no transition */
    prep("#o { color: blue; transition: color 1s linear } @starting-style { #o { color: red } } "
         "@starting-style { #o { color: lime } }", 500);
    TEST_ASSERT_EQUAL_STRING(" style=\"color:#008080\"", style_of("o"));
    /* the more specific starting rule wins, wherever it is */
    prep("#o { color: blue; transition: color 1s linear } @starting-style { div#o { color: red } } "
         "@starting-style { #o { color: lime } }", 500);
    TEST_ASSERT_EQUAL_STRING(" style=\"color:#800080\"", style_of("o"));
    /* revert and inherit are not starting colours */
    prep("#o { color: blue; transition: color 1s linear } @starting-style { #o { color: revert } }", 250);
    TEST_ASSERT_EQUAL_STRING(" style=\"color:#0000ff\"", style_of("o"));
    prep("#o { color: blue; transition: color 1s linear } @starting-style { #o { color: inherit } }", 250);
    TEST_ASSERT_EQUAL_STRING(" style=\"color:#0000ff\"", style_of("o"));
    /* initial is black */
    prep("#o { color: blue; transition: color 1s linear } @starting-style { #o { color: initial } }", 250);
    TEST_ASSERT_EQUAL_STRING(" style=\"color:#000040\"", style_of("o"));
    prep("#o { color: blue; transition: color 1s linear } @starting-style { #o { background-color: red } }", 250);
    TEST_ASSERT_EQUAL_STRING(" style=\"color:#0000ff\"", style_of("o"));
    TEST_ASSERT_EQUAL(0, g_d.col_transitioned);
    /* a starting colour alone is never painted */
    prep("@starting-style { #o { color: red } }", 0);
    TEST_ASSERT_EQUAL_STRING("", style_of("o"));
    /* a transition beats !important */
    prep("#o { color: blue !important; transition: color 1s linear } @starting-style { #o { color: red } }", 250);
    TEST_ASSERT_EQUAL_STRING(" style=\"color:#bf0040\"", style_of("o"));
    /* an animation wins over the transition */
    prep("@keyframes k { to { color: lime } } #o { color: blue; transition: color 1s linear; animation: k 1s linear } "
         "@starting-style { #o { color: red } }", 500);
    TEST_ASSERT_EQUAL_STRING(" style=\"color:#008080\"", style_of("o"));
    TEST_ASSERT_EQUAL(1, g_d.col_animated);
    TEST_ASSERT_EQUAL(0, g_d.col_transitioned);
    /* all: initial is not a starting rule (Lot 100 fix: all_kind) */
    prep("@starting-style { #o { color: red } } #o { all: initial }", 0);
    TEST_ASSERT_EQUAL_STRING(" style=\"color:#000000;margin:0px 0px 0px 0px;padding:0px 0px 0px 0px\"", style_of("o"));
}

int main(void) {
    unity_init();
    RUN_TEST(test_l100_names);
    RUN_TEST(test_l100_alpha);
    RUN_TEST(test_l100_hwb);
    RUN_TEST(test_l100_mix);
    RUN_TEST(test_l100_bounds);
    RUN_TEST(test_l100_paint);
    RUN_TEST(test_l100_keyframes);
    RUN_TEST(test_l100_starting);
    unity_print_results();
    unity_cleanup();
    return unity_stats.tests_failed == 0 ? 0 : 1;
}
