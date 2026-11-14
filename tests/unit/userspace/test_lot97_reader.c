/* Lot 97 (US-031): hand-written reader cases next to the generated suites
 * (test_lot97_ease.c, test_lot97_interp.c, test_lot97_timing.c,
 * test_lot97_sheet.c): font-relative registered values, @starting-style,
 * @keyframes (names, table, selectors, frame values), !important against
 * an animation, the cascade again after the animations, the summary line,
 * the clock bounds and rgb() in the reader colour. */
#include "../../framework/unity.h"
#include <string.h>
#include <stdio.h>
#include "wf_sitecss.h"

static wf_site_t g_d;
static char g_html[8192];
static char g_o[1400];

#define PL "@property --p { syntax: '<length>'; inherits: true; initial-value: 0px } "
#define PN "@property --p { syntax: '<length>'; inherits: false; initial-value: 0px } "
#define PI "@property --p { syntax: '<custom-ident>'; inherits: false; initial-value: alpha } "

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

/* em, rem, ex and ch on an own registered value; the child inherits the
 * computed length, not the em */
static void test_l97_font_units(void) {
    prep(PL "#o { font-size: 20px; --p: 2em } #i { font-size: 10px }", 0);
    TEST_ASSERT_EQUAL_STRING("40px", nv("o"));
    TEST_ASSERT_EQUAL_STRING("40px", nv("i"));
    TEST_ASSERT_EQUAL(1, g_d.prop_font);
    TEST_ASSERT_EQUAL_STRING("keyframes 0 animated 0 transitioned 0 font 1 at 0ms", anim_text());
    prep(PL "html { font-size: 10px } #o { font-size: 30px; --p: 3rem }", 0);
    TEST_ASSERT_EQUAL_STRING("30px", nv("o"));
    prep(PL "#o { font-size: 20px; --p: 1ex }", 0);
    TEST_ASSERT_EQUAL_STRING("10px", nv("o"));
    prep(PL "#o { font-size: 20px; --p: 3ch }", 0);
    TEST_ASSERT_EQUAL_STRING("30px", nv("o"));
    prep(PL "#o { font-size: 20px; --p: calc(1em + 5px) }", 0);
    TEST_ASSERT_EQUAL_STRING("25px", nv("o"));
    /* an own value on the child is computed with the child's font */
    prep(PL "#o { font-size: 20px; --p: 1em } #i { font-size: 10px; --p: 1em }", 0);
    TEST_ASSERT_EQUAL_STRING("20px", nv("o"));
    TEST_ASSERT_EQUAL_STRING("10px", nv("i"));
    TEST_ASSERT_EQUAL(2, g_d.prop_font);
    /* not registered: the text stays */
    prep("#o { font-size: 20px; --u: 2em }", 0);
    TEST_ASSERT_EQUAL_STRING("2em", wf_site_node_var(&g_d, node("o"), "--u"));
    TEST_ASSERT_EQUAL(0, g_d.prop_font);
    TEST_ASSERT_EQUAL_STRING("(empty)", anim_text());
    /* not inherited: the child gets the initial value */
    prep(PN "#o { font-size: 20px; --p: 2em }", 0);
    TEST_ASSERT_EQUAL_STRING("40px", nv("o"));
    TEST_ASSERT_EQUAL_STRING("0px", nv("i"));
    /* no font unit: nothing computed */
    prep(PL "#o { --p: 12px }", 0);
    TEST_ASSERT_EQUAL_STRING("12px", nv("o"));
    TEST_ASSERT_EQUAL(0, g_d.prop_font);
}

/* @starting-style keeps custom properties only, and alone gives nothing */
static void test_l97_starting_style(void) {
    prep("@starting-style { #o { color: red; font-weight: bold } }", 0);
    TEST_ASSERT_EQUAL_STRING("", style_of("o"));
    prep("#o { color: red }", 0);
    TEST_ASSERT_NOT_EQUAL(0, (int)strlen(style_of("o")));
    prep(PL "#o { --p: 5px } @starting-style { #o { --p: 9px } }", 300);
    TEST_ASSERT_EQUAL_STRING("5px", nv("o"));
    prep(PL "@starting-style { #o { --p: 9px } }", 300);
    TEST_ASSERT_EQUAL_STRING("0px", nv("o"));
    /* a selector list inside @starting-style keeps the flag on each part */
    prep(PL "#o, #q { --p: 100px; transition: --p 1000ms linear } @starting-style { #o, #q { --p: 0px } }", 250);
    TEST_ASSERT_EQUAL_STRING("25px", nv("o"));
    TEST_ASSERT_EQUAL_STRING("25px", nv("q"));
}

static void test_l97_transitions(void) {
    const char *t = PL "#o { --p: 100px; transition: %s } @starting-style { #o { --p: 0px } }";
    char css[512];
    snprintf(css, sizeof(css), t, "--p 1000ms linear");
    prep(css, 250);
    TEST_ASSERT_EQUAL_STRING("25px", nv("o"));
    TEST_ASSERT_EQUAL_STRING("25px", nv("i"));
    TEST_ASSERT_EQUAL(1, g_d.prop_transitioned);
    TEST_ASSERT_EQUAL_STRING("keyframes 0 animated 0 transitioned 1 font 0 at 250ms", anim_text());
    prep(css, 999);
    TEST_ASSERT_EQUAL_STRING("99.9px", nv("o"));
    prep(css, 1000);
    TEST_ASSERT_EQUAL_STRING("100px", nv("o"));
    TEST_ASSERT_EQUAL(0, g_d.prop_transitioned);
    snprintf(css, sizeof(css), t, "--p 1000ms 500ms linear");
    prep(css, 200);
    TEST_ASSERT_EQUAL_STRING("0px", nv("o"));
    TEST_ASSERT_EQUAL(1, g_d.prop_transitioned);
    prep(css, 1000);
    TEST_ASSERT_EQUAL_STRING("50px", nv("o"));
    snprintf(css, sizeof(css), t, "all 1000ms linear");
    prep(css, 500);
    TEST_ASSERT_EQUAL_STRING("50px", nv("o"));
    snprintf(css, sizeof(css), t, "--other 1000ms linear");
    prep(css, 500);
    TEST_ASSERT_EQUAL_STRING("100px", nv("o"));
    snprintf(css, sizeof(css), t, "--p 0ms linear");
    prep(css, 0);
    TEST_ASSERT_EQUAL_STRING("100px", nv("o"));
    TEST_ASSERT_EQUAL(0, g_d.prop_transitioned);
    /* a negative delay that ends the transition before it starts */
    snprintf(css, sizeof(css), t, "--p 100ms -100ms linear");
    prep(css, 0);
    TEST_ASSERT_EQUAL_STRING("100px", nv("o"));
    snprintf(css, sizeof(css), t, "--p 200ms -100ms linear");
    prep(css, 0);
    TEST_ASSERT_EQUAL_STRING("50px", nv("o"));
    /* the same start and end value: none */
    prep(PL "#o { --p: 7px; transition: --p 1000ms linear } @starting-style { #o { --p: 7px } }", 500);
    TEST_ASSERT_EQUAL(0, g_d.prop_transitioned);
    /* a starting value that does not match the syntax: none */
    prep(PL "#o { --p: 7px; transition: --p 1000ms linear } @starting-style { #o { --p: red } }", 500);
    TEST_ASSERT_EQUAL_STRING("7px", nv("o"));
    TEST_ASSERT_EQUAL(0, g_d.prop_transitioned);
    /* the starting value is computed with the font too */
    prep(PL "#o { font-size: 10px; --p: 100px; transition: --p 1000ms linear } @starting-style { #o { --p: 2em } }", 500);
    TEST_ASSERT_EQUAL_STRING("60px", nv("o"));
    /* discrete values need allow-discrete */
    prep(PI "#o { --p: beta; transition: --p 1000ms linear } @starting-style { #o { --p: gamma } }", 200);
    TEST_ASSERT_EQUAL_STRING("beta", nv("o"));
    TEST_ASSERT_EQUAL(0, g_d.prop_transitioned);
    prep(PI "#o { --p: beta; transition: --p 1000ms linear allow-discrete } @starting-style { #o { --p: gamma } }", 200);
    TEST_ASSERT_EQUAL_STRING("gamma", nv("o"));
    TEST_ASSERT_EQUAL(1, g_d.prop_transitioned);
    prep(PI "#o { --p: beta; transition: --p 1000ms linear allow-discrete } @starting-style { #o { --p: gamma } }", 600);
    TEST_ASSERT_EQUAL_STRING("beta", nv("o"));
    /* transition: none in a later rule */
    prep(PL "#o { --p: 100px; transition: --p 1000ms linear } #o { transition: none } @starting-style { #o { --p: 0px } }", 500);
    TEST_ASSERT_EQUAL_STRING("100px", nv("o"));
    /* an invalid list keeps the earlier one */
    prep(PL "#o { --p: 100px; transition: --p 1000ms linear } #o { transition: --p 1000ms bogus } @starting-style { #o { --p: 0px } }", 500);
    TEST_ASSERT_EQUAL_STRING("50px", nv("o"));
}

#define KF "@keyframes k { from { --p: 0px } to { --p: 100px } } "

static void test_l97_animations(void) {
    prep(PN KF "#o { animation: k 1000ms linear }", 300);
    TEST_ASSERT_EQUAL_STRING("30px", nv("o"));
    TEST_ASSERT_EQUAL_STRING("0px", nv("i"));
    TEST_ASSERT_EQUAL(1, g_d.prop_animated);
    TEST_ASSERT_EQUAL_STRING("keyframes 1 animated 1 transitioned 0 font 0 at 300ms", anim_text());
    prep(PN KF "#o { animation: k 1000ms linear }", 1000);
    TEST_ASSERT_EQUAL_STRING("0px", nv("o"));
    TEST_ASSERT_EQUAL(0, g_d.prop_animated);
    prep(PN KF "#o { animation: k 1000ms linear forwards }", 5000);
    TEST_ASSERT_EQUAL_STRING("100px", nv("o"));
    /* an unknown name, and keyframes without the property */
    prep(PN KF "#o { animation: nope 1000ms linear }", 300);
    TEST_ASSERT_EQUAL_STRING("0px", nv("o"));
    prep(PN "@keyframes k { from { --q: 0px } to { --q: 9px } } #o { animation: k 1000ms linear }", 300);
    TEST_ASSERT_EQUAL_STRING("0px", nv("o"));
    TEST_ASSERT_EQUAL(0, g_d.prop_animated);
    /* implicit 0% and 100% take the value under the animation */
    prep(PN "@keyframes k { 50% { --p: 100px } } #o { --p: 20px; animation: k 1000ms linear }", 250);
    TEST_ASSERT_EQUAL_STRING("60px", nv("o"));
    prep(PN "@keyframes k { 50% { --p: 100px } } #o { --p: 20px; animation: k 1000ms linear }", 750);
    TEST_ASSERT_EQUAL_STRING("60px", nv("o"));
    /* !important, var() and a value off the syntax are left out */
    prep(PN "@keyframes k { from { --p: 0px } to { --p: 50px !important } } #o { --p: 20px; animation: k 1000ms linear }", 500);
    TEST_ASSERT_EQUAL_STRING("10px", nv("o"));
    prep(PN "@keyframes k { from { --p: 0px } to { --p: var(--x) } } #o { --p: 20px; animation: k 1000ms linear }", 500);
    TEST_ASSERT_EQUAL_STRING("10px", nv("o"));
    prep(PN "@keyframes k { from { --p: 0px } to { --p: red } } #o { --p: 20px; animation: k 1000ms linear }", 500);
    TEST_ASSERT_EQUAL_STRING("10px", nv("o"));
    /* the later block wins at one offset; a list of selectors */
    prep(PN "@keyframes k { to { --p: 50px } from { --p: 0px } to { --p: 100px } } #o { animation: k 1000ms linear }", 500);
    TEST_ASSERT_EQUAL_STRING("50px", nv("o"));
    prep(PN "@keyframes k { from, to { --p: 40px } 50% { --p: 0px } } #o { animation: k 1000ms linear }", 250);
    TEST_ASSERT_EQUAL_STRING("20px", nv("o"));
    /* a timing function in a frame applies to its interval */
    prep(PN "@keyframes k { from { --p: 0px; animation-timing-function: steps(2) } to { --p: 100px } } #o { animation: k 1000ms linear }", 300);
    TEST_ASSERT_EQUAL_STRING("0px", nv("o"));
    prep(PN "@keyframes k { from { --p: 0px; animation-timing-function: steps(2) } to { --p: 100px } } #o { animation: k 1000ms linear }", 600);
    TEST_ASSERT_EQUAL_STRING("50px", nv("o"));
    prep(PN "@keyframes k { from { --p: 0px; animation-timing-function: bogus } to { --p: 100px } } #o { animation: k 1000ms linear }", 300);
    TEST_ASSERT_EQUAL_STRING("30px", nv("o"));
    /* frame values are computed with the font */
    prep(PN "@keyframes k { from { --p: 0px } to { --p: 10em } } #o { font-size: 10px; animation: k 1000ms linear }", 500);
    TEST_ASSERT_EQUAL_STRING("50px", nv("o"));
    /* inherited: the child takes the animated value */
    prep(PL KF "#o { animation: k 1000ms linear }", 300);
    TEST_ASSERT_EQUAL_STRING("30px", nv("i"));
    /* two animations: the later one wins */
    prep(PN KF "@keyframes j { from { --p: 0px } to { --p: 10px } } #o { animation: k 1000ms linear, j 1000ms linear }", 500);
    TEST_ASSERT_EQUAL_STRING("5px", nv("o"));
    prep(PN KF "@keyframes j { from { --p: 0px } to { --p: 10px } } #o { animation: j 1000ms linear, k 1000ms linear }", 500);
    TEST_ASSERT_EQUAL_STRING("50px", nv("o"));
    /* an animation wins over a transition */
    prep(PN KF "#o { --p: 7px; animation: k 1000ms linear; transition: --p 1000ms linear } @starting-style { #o { --p: 0px } }", 500);
    TEST_ASSERT_EQUAL_STRING("50px", nv("o"));
    TEST_ASSERT_EQUAL(1, g_d.prop_animated);
    TEST_ASSERT_EQUAL(0, g_d.prop_transitioned);
    /* animation: none, and an invalid list keeping the earlier one */
    prep(PN KF "#o { animation: k 1000ms linear } #o { animation: none }", 300);
    TEST_ASSERT_EQUAL_STRING("0px", nv("o"));
    prep(PN KF "#o { animation: k 1000ms linear } #o { animation: k 1000ms bogus junk }", 300);
    TEST_ASSERT_EQUAL_STRING("30px", nv("o"));
}

/* an !important own declaration keeps its value, a plain one does not */
static void test_l97_important(void) {
    prep(PN KF "#o { --p: 7px !important; animation: k 1000ms linear }", 300);
    TEST_ASSERT_EQUAL_STRING("7px", nv("o"));
    TEST_ASSERT_EQUAL(0, g_d.prop_animated);
    prep(PN KF "#o { --p: 7px; animation: k 1000ms linear }", 300);
    TEST_ASSERT_EQUAL_STRING("30px", nv("o"));
}

static void test_l97_keyframe_selectors(void) {
    const char *t = PN "@keyframes k { from { --p: 0px } %s { --p: 100px } } #o { animation: k 1000ms linear }";
    char css[512];
    snprintf(css, sizeof(css), t, "12.5%");
    prep(css, 100);
    TEST_ASSERT_EQUAL_STRING("80px", nv("o"));
    snprintf(css, sizeof(css), t, "TO");
    prep(css, 500);
    TEST_ASSERT_EQUAL_STRING("50px", nv("o"));
    /* refused selectors: the block is left out (the implicit 100% is 0px) */
    snprintf(css, sizeof(css), t, "1.234%");
    prep(css, 500);
    TEST_ASSERT_EQUAL_STRING("0px", nv("o"));
    snprintf(css, sizeof(css), t, "101%");
    prep(css, 500);
    TEST_ASSERT_EQUAL_STRING("0px", nv("o"));
    snprintf(css, sizeof(css), t, "5.%");
    prep(css, 500);
    TEST_ASSERT_EQUAL_STRING("0px", nv("o"));
    snprintf(css, sizeof(css), t, ".5%");
    prep(css, 500);
    TEST_ASSERT_EQUAL_STRING("0px", nv("o"));
    snprintf(css, sizeof(css), t, "50");
    prep(css, 500);
    TEST_ASSERT_EQUAL_STRING("0px", nv("o"));
    snprintf(css, sizeof(css), t, "5a%");
    prep(css, 500);
    TEST_ASSERT_EQUAL_STRING("0px", nv("o"));
    snprintf(css, sizeof(css), t, "50%, bogus");
    prep(css, 500);
    TEST_ASSERT_EQUAL_STRING("0px", nv("o"));
    snprintf(css, sizeof(css), t, "100.00%");
    prep(css, 500);
    TEST_ASSERT_EQUAL_STRING("50px", nv("o"));
    snprintf(css, sizeof(css), t, "99.5%");
    prep(css, 995);
    TEST_ASSERT_EQUAL_STRING("100px", nv("o"));
}

static void test_l97_keyframes_table(void) {
    char css[2048];
    int k, n = 0;
    prep(PN "@keyframes \"k\" { to { --p: 100px } } #o { animation: k 1000ms linear }", 500);
    TEST_ASSERT_EQUAL_STRING("50px", nv("o"));
    prep(PN "@keyframes 'K' { to { --p: 100px } } #o { animation: k 1000ms linear }", 500);
    TEST_ASSERT_EQUAL_STRING("50px", nv("o"));
    prep(PN "@keyframes a.b { to { --p: 100px } } @keyframes \"\" { to { --p: 1px } }", 0);
    TEST_ASSERT_EQUAL(0, g_d.nkf);
    TEST_ASSERT_EQUAL(2, g_d.dropped_kf);
    TEST_ASSERT_EQUAL_STRING("keyframes 0 animated 0 transitioned 0 font 0 at 0ms dropped 2", anim_text());
    prep(PN "@keyframes k { to { --p: 100px } } @keyframes k { to { --p: 10px } } #o { animation: k 1000ms linear }", 500);
    TEST_ASSERT_EQUAL(1, g_d.nkf);
    TEST_ASSERT_EQUAL_STRING("5px", nv("o"));
    n += snprintf(css + n, sizeof(css) - (size_t)n, PN);
    for (k = 0; k <= WF_SITE_KFS; k++)
        n += snprintf(css + n, sizeof(css) - (size_t)n, "@keyframes n%d { to { --p: %dpx } } ", k, 10 * (k + 1));
    snprintf(css + n, sizeof(css) - (size_t)n, "#o { animation: n%d 1000ms linear } #q { animation: n%d 1000ms linear }",
             WF_SITE_KFS - 1, WF_SITE_KFS);
    prep(css, 500);
    TEST_ASSERT_EQUAL(WF_SITE_KFS, g_d.nkf);
    TEST_ASSERT_EQUAL(1, g_d.dropped_kf);
    TEST_ASSERT_EQUAL_STRING("40px", nv("o"));
    TEST_ASSERT_EQUAL_STRING("0px", nv("q"));
    /* inside an applying @media */
    prep(PN "@media (min-width: 100px) { @keyframes k { to { --p: 100px } } } #o { animation: k 1000ms linear }", 500);
    TEST_ASSERT_EQUAL_STRING("50px", nv("o"));
    prep(PN "@media (max-width: 100px) { @keyframes k { to { --p: 100px } } } #o { animation: k 1000ms linear }", 500);
    TEST_ASSERT_EQUAL(0, g_d.nkf);
}

/* the reader properties read the animated value (cascade again) */
static void test_l97_recascade(void) {
    prep("@property --c { syntax: '<color>'; inherits: true; initial-value: black } "
         "@keyframes k { from { --c: rgb(0, 0, 0) } to { --c: rgb(200, 100, 0) } } "
         "#o { color: var(--c); animation: k 1000ms linear }", 500);
    TEST_ASSERT_EQUAL(1, g_d.recascades);
    TEST_ASSERT_EQUAL_STRING(" style=\"color:#643200\"", style_of("o"));
    prep("@property --c { syntax: '<color>'; inherits: true; initial-value: black } #o { color: var(--c) }", 500);
    TEST_ASSERT_EQUAL(0, g_d.recascades);
    TEST_ASSERT_EQUAL_STRING(" style=\"color:#000000\"", style_of("o"));
    prep("#o { color: red }", 0);
    TEST_ASSERT_EQUAL(0, g_d.recascades);
}

static void test_l97_clock(void) {
    wf_site_set_time(-5);
    snprintf(g_html, sizeof(g_html), "<html><head><style>%s</style></head><body><div id=o></div></body></html>",
             PN KF "#o { animation: k 1000ms linear }");
    TEST_ASSERT_EQUAL(0, wf_site_prepare(&g_d, g_html, (int)strlen(g_html)));
    TEST_ASSERT_EQUAL(0, g_d.anim_ms);
    wf_site_set_time(200000000);
    TEST_ASSERT_EQUAL(0, wf_site_prepare(&g_d, g_html, (int)strlen(g_html)));
    TEST_ASSERT_EQUAL(100000000, g_d.anim_ms);
    wf_site_set_time(100000000);
    TEST_ASSERT_EQUAL(0, wf_site_prepare(&g_d, g_html, (int)strlen(g_html)));
    TEST_ASSERT_EQUAL(100000000, g_d.anim_ms);
    wf_site_set_time(0);
    TEST_ASSERT_EQUAL(0, wf_site_prepare(&g_d, g_html, (int)strlen(g_html)));
    TEST_ASSERT_EQUAL(0, g_d.anim_ms);
}

static void test_l97_rgb_color(void) {
    prep("#o { color: rgb(255, 0, 16) }", 0);
    TEST_ASSERT_EQUAL_STRING(" style=\"color:#ff0010\"", style_of("o"));
    prep("#o { color: RGBA( 1 , 2 , 3 , 1 ) }", 0);
    TEST_ASSERT_EQUAL_STRING(" style=\"color:#010203\"", style_of("o"));
    prep("#o { color: rgb(0255, 0, 0) }", 0);
    TEST_ASSERT_EQUAL_STRING(" style=\"color:#ff0000\"", style_of("o"));
    prep("#o { color: rgb(1, 2, 3) !important } #o { color: blue }", 0);
    TEST_ASSERT_EQUAL_STRING(" style=\"color:#010203\"", style_of("o"));
    prep("#o { color: blue } #o { color: rgb(00255, 0, 0) }", 0);
    TEST_ASSERT_EQUAL_STRING(" style=\"color:#0000ff\"", style_of("o"));
    prep("#o { color: blue } #o { color: rgb(256, 0, 0) }", 0);
    TEST_ASSERT_EQUAL_STRING(" style=\"color:#0000ff\"", style_of("o"));
    prep("#o { color: blue } #o { color: rgba(1, 2, 3, 0) }", 0);
    TEST_ASSERT_EQUAL_STRING(" style=\"color:#0000ff\"", style_of("o"));
    prep("#o { color: blue } #o { color: rgb(1, 2, 3, 1) }", 0);
    TEST_ASSERT_EQUAL_STRING(" style=\"color:#0000ff\"", style_of("o"));
    prep("#o { color: blue } #o { color: rgb(1, 2) }", 0);
    TEST_ASSERT_EQUAL_STRING(" style=\"color:#0000ff\"", style_of("o"));
    prep("#o { color: blue } #o { color: rgb(1 2 3) }", 0);
    TEST_ASSERT_EQUAL_STRING(" style=\"color:#0000ff\"", style_of("o"));
    prep("#o { color: blue } #o { color: rgb(1, 2, 3) x }", 0);
    TEST_ASSERT_EQUAL_STRING(" style=\"color:#0000ff\"", style_of("o"));
    prep("#o { color: blue } #o { color: rgb 1, 2, 3 }", 0);
    TEST_ASSERT_EQUAL_STRING(" style=\"color:#0000ff\"", style_of("o"));
    prep("#o { color: blue } #o { color: rgb(, 2, 3) }", 0);
    TEST_ASSERT_EQUAL_STRING(" style=\"color:#0000ff\"", style_of("o"));
    prep("#o { color: blue } #o { color: rgb(1, 2, 3 }", 0);
    TEST_ASSERT_EQUAL_STRING(" style=\"color:#0000ff\"", style_of("o"));
}

int main(void) {
    RUN_TEST(test_l97_font_units);
    RUN_TEST(test_l97_starting_style);
    RUN_TEST(test_l97_transitions);
    RUN_TEST(test_l97_animations);
    RUN_TEST(test_l97_important);
    RUN_TEST(test_l97_keyframe_selectors);
    RUN_TEST(test_l97_keyframes_table);
    RUN_TEST(test_l97_recascade);
    RUN_TEST(test_l97_clock);
    RUN_TEST(test_l97_rgb_color);
    unity_print_results();
    unity_cleanup();
    return unity_stats.tests_failed == 0 ? 0 : 1;
}
