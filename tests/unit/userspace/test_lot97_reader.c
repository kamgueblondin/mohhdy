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
#include "wf_cssprop.h"

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
    /* Lot 98: ex is 5/16 em (the engine font's x height; was 1/2 em) */
    prep(PL "#o { font-size: 20px; --p: 1ex }", 0);
    TEST_ASSERT_EQUAL_STRING("6.25px", nv("o"));
    prep(PL "#o { font-size: 20px; --p: 3ch }", 0);
    TEST_ASSERT_EQUAL_STRING("30px", nv("o"));
    prep(PL "#o { font-size: 20px; --p: calc(1em + 5px) }", 0);
    TEST_ASSERT_EQUAL_STRING("25px", nv("o"));
    /* rounded half away from zero to 1/10000 px: 1.0001ex of 15px
     * (Lot 98: 4.68796875px; was 7.5008px with ex = 1/2 em) */
    prep(PL "#o { font-size: 15px; --p: 1.0001ex }", 0);
    TEST_ASSERT_EQUAL_STRING("4.688px", nv("o"));
    prep(PL "#o { font-size: 20px; --p: -2em }", 0);
    TEST_ASSERT_EQUAL_STRING("-40px", nv("o"));
    /* past the fixed point range: kept as written, not counted */
    prep(PL "#o { font-size: 16px; --p: 99999999em }", 0);
    TEST_ASSERT_EQUAL_STRING("99999999em", nv("o"));
    TEST_ASSERT_EQUAL(0, g_d.prop_font);
    prep(PL "#o { font-size: 16px; --p: 9999em }", 0);
    TEST_ASSERT_EQUAL_STRING("9999em", nv("o"));
    TEST_ASSERT_EQUAL(0, g_d.prop_font);
    /* a font unit inside a string or a name is not one */
    prep("@property --p { syntax: '<string>'; inherits: true; initial-value: 'a' } #o { --p: '2em' }", 0);
    TEST_ASSERT_EQUAL_STRING("'2em'", nv("o"));
    TEST_ASSERT_EQUAL(0, g_d.prop_font);
    prep("@property --p { syntax: '<custom-ident>'; inherits: true; initial-value: a } #o { --p: h1em }", 0);
    TEST_ASSERT_EQUAL_STRING("h1em", nv("o"));
    TEST_ASSERT_EQUAL(0, g_d.prop_font);
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
    /* a rule after the block is a plain one again */
    prep(PL "@starting-style { #o { --p: 9px } } #o { --p: 5px; transition: --p 1000ms linear }", 500);
    TEST_ASSERT_EQUAL_STRING("7px", nv("o"));
    prep(PL "@starting-style { #q { --p: 9px } } #o { --p: 5px }", 0);
    TEST_ASSERT_EQUAL_STRING("5px", nv("o"));
    TEST_ASSERT_EQUAL_STRING("0px", nv("q"));
    /* an @scope block inside @starting-style */
    prep(PL "#o { --p: 100px; transition: --p 1000ms linear } @starting-style { @scope (#o) { :scope { --p: 0px } } }", 250);
    TEST_ASSERT_EQUAL_STRING("25px", nv("o"));
    /* bare declarations of an @scope block inside @starting-style */
    prep(PL "#o { --p: 100px; transition: --p 1000ms linear } @starting-style { @scope (#o) { --p: 0px } }", 250);
    TEST_ASSERT_EQUAL_STRING("25px", nv("o"));
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
    /* transition: unset, and a weaker transition: none */
    prep(PL "#o { --p: 100px; transition: --p 1000ms linear } #o { transition: unset } @starting-style { #o { --p: 0px } }", 500);
    TEST_ASSERT_EQUAL_STRING("100px", nv("o"));
    prep(PL "#o { --p: 100px; transition: --p 1000ms linear } div { transition: none } @starting-style { #o { --p: 0px } }", 500);
    TEST_ASSERT_EQUAL_STRING("50px", nv("o"));
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
    /* with the universal syntax the matcher takes anything: !important and
     * var() frames are still left out (the own value z fills 100%) */
    prep("@property --p { syntax: '*'; inherits: false } @keyframes k { from { --p: x } to { --p: y !important } } "
         "#o { --p: z; animation: k 1000ms linear }", 750);
    TEST_ASSERT_EQUAL_STRING("z", nv("o"));
    prep("@property --p { syntax: '*'; inherits: false } @keyframes k { from { --p: x } to { --p: var(--q) } } "
         "#o { --p: z; --q: w; animation: k 1000ms linear }", 750);
    TEST_ASSERT_EQUAL_STRING("z", nv("o"));
    /* at a frame offset the next segment applies (its step-start jumps) */
    prep(PN "@keyframes k { from { --p: 0px } 50% { --p: 50px; animation-timing-function: step-start } to { --p: 100px } } "
         "#o { animation: k 1000ms linear }", 500);
    TEST_ASSERT_EQUAL_STRING("100px", nv("o"));
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
    /* animation: none (initial, unset), and an invalid list keeping the
     * earlier one */
    prep(PN KF "#o { animation: k 1000ms linear } #o { animation: none }", 300);
    TEST_ASSERT_EQUAL_STRING("0px", nv("o"));
    prep(PN KF "#o { animation: k 1000ms linear } #o { animation: initial }", 300);
    TEST_ASSERT_EQUAL_STRING("0px", nv("o"));
    prep(PN KF "#o { animation: k 1000ms linear } #o { animation: unset }", 300);
    TEST_ASSERT_EQUAL_STRING("0px", nv("o"));
    /* the stronger selector wins, not the later rule */
    prep(PN KF "#o { animation: k 1000ms linear } div { animation: none }", 300);
    TEST_ASSERT_EQUAL_STRING("30px", nv("o"));
    prep(PN KF "#o { animation: none } div { animation: k 1000ms linear }", 300);
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
    /* a value valid before the animation and invalid after it: the colour
     * is dropped */
    prep("@property --p { syntax: '*'; inherits: true } @keyframes k { from { --p: red } to { --p: 5px } } "
         "#o { --p: red; color: var(--p); animation: k 1000ms linear }", 750);
    TEST_ASSERT_EQUAL(1, g_d.recascades);
    TEST_ASSERT_EQUAL_STRING("", style_of("o"));
    prep("@property --p { syntax: '*'; inherits: true } #o { --p: red; color: var(--p) }", 0);
    TEST_ASSERT_EQUAL_STRING(" style=\"color:#ff0000\"", style_of("o"));
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
    prep("#o { color: blue } #o { color: rgbx1, 2, 3) }", 0);
    TEST_ASSERT_EQUAL_STRING(" style=\"color:#0000ff\"", style_of("o"));
    prep("#o { color: blue } #o { color: rgb 1, 2, 3 }", 0);
    TEST_ASSERT_EQUAL_STRING(" style=\"color:#0000ff\"", style_of("o"));
    prep("#o { color: blue } #o { color: rgb(, 2, 3) }", 0);
    TEST_ASSERT_EQUAL_STRING(" style=\"color:#0000ff\"", style_of("o"));
    prep("#o { color: blue } #o { color: rgb(1, 2, 3 }", 0);
    TEST_ASSERT_EQUAL_STRING(" style=\"color:#0000ff\"", style_of("o"));
}

static const char *ip(const char *syn, const char *a, const char *b, int t) {
    static wf_cssprop_t p;
    static char v[128], out[160];
    int r;
    memset(&p, 0, sizeof(p));
    p.syntax = (unsigned char)wf_cssprop_syntax_parse(syn, (int)strlen(syn), &p);
    if (p.syntax == WF_CSSPROP_BAD || !wf_cssprop_match_prop(&p, a) || !wf_cssprop_match_prop(&p, b)) return "NO MATCH";
    r = wf_cssprop_interp(&p, a, b, t, v, (int)sizeof(v));
    snprintf(out, sizeof(out), "%d %s", r, v);
    return out;
}

/* interpolation cases next to test_lot97_interp.c: a unitless 0 length,
 * colours (transparent, named, percentages, an alpha past 1) and colour
 * texts the loose <color> match takes but the interpolation does not
 * (discrete: 1, the end value at 50%) */
static void test_l97_interp_cases(void) {
    TEST_ASSERT_EQUAL_STRING("2 5px", ip("'<length>'", "0", "10px", 5000));
    TEST_ASSERT_EQUAL_STRING("1 20px", ip("'*'", "10px", "20px", 5000));
    TEST_ASSERT_EQUAL_STRING("2 rgba(255, 0, 0, 0.5)", ip("'<color>'", "transparent", "red", 5000));
    TEST_ASSERT_EQUAL_STRING("2 rgb(128, 83, 0)", ip("'<color>'", "orange", "black", 5000));
    TEST_ASSERT_EQUAL_STRING("2 rgb(128, 0, 0)", ip("'<color>'", "rgb(100%, 0%, 0%)", "black", 5000));
    TEST_ASSERT_EQUAL_STRING("2 rgba(0, 0, 0, 0.5)", ip("'<color>'", "rgba(0, 0, 0, 2)", "rgba(0, 0, 0, 0)", 5000));
    TEST_ASSERT_EQUAL_STRING("2 rgba(1, 2, 3, 0.75)", ip("'<color>'", "rgb(1, 2, 3)", "rgb(1 2 3 / 50%)", 5000));
    TEST_ASSERT_EQUAL_STRING("1 black", ip("'<color>'", "rgb(1 2, 3)", "black", 5000));
    TEST_ASSERT_EQUAL_STRING("1 black", ip("'<color>'", "rgb(1, 2 / 0.5)", "black", 5000));
    TEST_ASSERT_EQUAL_STRING("1 black", ip("'<color>'", "rgb(1 2 / 0.5)", "black", 5000));
    TEST_ASSERT_EQUAL_STRING("1 black", ip("'<color>'", "rgb(1 2 3 4)", "black", 5000));
    TEST_ASSERT_EQUAL_STRING("1 black", ip("'<color>'", "rgb(1, 2)", "black", 5000));
}

/* the bounds of cubic-bezier y and the fifth decimal of a time */
static void test_l97_ease_time_bounds(void) {
    wf_cssease_t e;
    wf_cssanim_t a[1];
    TEST_ASSERT_EQUAL(1, wf_cssease_parse("cubic-bezier(0, 100, 1, -100)", 29, &e));
    TEST_ASSERT_EQUAL(0, wf_cssease_parse("cubic-bezier(0, 100.0001, 1, 0)", 31, &e));
    TEST_ASSERT_EQUAL(0, wf_cssease_parse("cubic-bezier(0, -100.0001, 1, 0)", 32, &e));
    TEST_ASSERT_EQUAL(0, wf_cssease_parse("cubic-bezier(0, 0, 1, 100.0001)", 31, &e));
    TEST_ASSERT_EQUAL(0, wf_cssease_parse("cubic-bezier(0, 0, 1, -100.0001)", 32, &e));
    TEST_ASSERT_EQUAL(1, wf_cssanim_parse("k 0.00049s", a, 1));
    TEST_ASSERT_EQUAL(1, a[0].dur);
    TEST_ASSERT_EQUAL(1, wf_cssanim_parse("k 0.00044s", a, 1));
    TEST_ASSERT_EQUAL(0, a[0].dur);
    TEST_ASSERT_EQUAL(1, wf_cssanim_parse("k 0.5ms", a, 1));
    TEST_ASSERT_EQUAL(1, a[0].dur);
    TEST_ASSERT_EQUAL(1, wf_cssanim_parse("k 100000s", a, 1));
    TEST_ASSERT_EQUAL(100000000, a[0].dur);
    TEST_ASSERT_EQUAL(-1, wf_cssanim_parse("k 100000.001s", a, 1));
    TEST_ASSERT_EQUAL(-1, wf_cssanim_parse("k 1s -100000.001s", a, 1));
}

/* list lengths and the progress of 0 iterations and of an infinite 0s
 * animation */
static void test_l97_lists_progress(void) {
    static wf_cssanim_t a[WF_CSSANIM_MAX];
    int p, before;
    TEST_ASSERT_EQUAL(4, wf_cssanim_parse("a 1s, b 1s, c 1s, d 1s", a, WF_CSSANIM_MAX));
    TEST_ASSERT_EQUAL(-1, wf_cssanim_parse("a 1s, b 1s, c 1s, d 1s, e 1s", a, WF_CSSANIM_MAX));
    TEST_ASSERT_EQUAL(-1, wf_cssanim_parse("a 1s, b 1s", a, 1));
    TEST_ASSERT_EQUAL(2, wf_cssanim_parse("a steps(2, end), b cubic-bezier(0, 0, 1, 1)", a, WF_CSSANIM_MAX));
    TEST_ASSERT_EQUAL(-1, wf_cssanim_parse("a 1s,", a, WF_CSSANIM_MAX));
    TEST_ASSERT_EQUAL(1, wf_cssanim_parse("k 1000ms 0 forwards linear", a, 1));
    TEST_ASSERT_EQUAL(1, wf_cssanim_progress(&a[0], 5000, &p, &before));
    TEST_ASSERT_EQUAL(0, p);
    TEST_ASSERT_EQUAL(1, wf_cssanim_parse("k 0s infinite forwards linear", a, 1));
    TEST_ASSERT_EQUAL(0, wf_cssanim_progress(&a[0], 5000, &p, &before));
    TEST_ASSERT_EQUAL(1, wf_cssanim_parse("k 0s 2.5 both linear", a, 1));
    TEST_ASSERT_EQUAL(1, wf_cssanim_progress(&a[0], 0, &p, &before));
    TEST_ASSERT_EQUAL(5000, p);
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
    RUN_TEST(test_l97_interp_cases);
    RUN_TEST(test_l97_ease_time_bounds);
    RUN_TEST(test_l97_lists_progress);
    unity_print_results();
    unity_cleanup();
    return unity_stats.tests_failed == 0 ? 0 : 1;
}
