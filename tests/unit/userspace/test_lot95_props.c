/* Lot 95: @property limits (8 alternatives, 31-byte names, 127-byte values,
 * 16 registrations, 32 per-element names), the Lot 95 types and the
 * computed values of registered values (plain dimensions, lists, known
 * length-percentage mixes, viewport units, transform functions). Hand
 * written; the generated suites are test_lot95_{syntax,match,compute,sheet}.c
 * (scripts/lot95_propgen.py). */
#include "../../framework/unity.h"
#include <string.h>
#include <stdio.h>
#include "wf_cssprop.h"
#include "wf_sitecss.h"

static wf_cssprop_t g_p;
static wf_site_t g_d;
static char g_o[128], g_html[8192], g_css[6144], g_t[256];

static const char *cvx(const char *syn, const char *v, const wf_cssprop_ctx_t *c) {
    memset(&g_p, 0, sizeof(g_p));
    g_p.syntax = (unsigned char)wf_cssprop_syntax_parse(syn, (int)strlen(syn), &g_p);
    if (!g_p.syntax) return "BAD SYNTAX";
    if (!wf_cssprop_compute_ctx(&g_p, v, c, g_o, (int)sizeof(g_o))) return g_o[0] ? "KEEP BUT WRITTEN" : "KEEP";
    return g_o;
}

static const char *cv(const char *syn, const char *v) { return cvx(syn, v, 0); }

static const char *cvp(const char *syn, const char *v, int vw, int vh) {
    wf_cssprop_ctx_t c;
    c.vw = vw; c.vh = vh;
    return cvx(syn, v, &c);
}

static void prep(const char *css, const char *body, int vw, int vh) {
    snprintf(g_html, sizeof(g_html), "<html><head><style>%s</style></head><body>%s</body></html>", css, body);
    TEST_ASSERT_EQUAL(0, wf_site_prepare_ext(&g_d, g_html, (int)strlen(g_html), vw, vh, 0, 0));
}

static const char *nv(const char *id, const char *name) {
    int i;
    for (i = 0; i < g_d.nnodes; i++)
        if (!strcmp(g_d.node[i].id, id)) { const char *v = wf_site_node_var(&g_d, i, name); return v ? v : "(none)"; }
    return "(no node)";
}

static const char *tail(void) {
    const char *s;
    wf_site_props_text(&g_d, g_t, (int)sizeof(g_t));
    s = strstr(g_t, " simplified ");
    return s ? s : "";
}

static void fill(char *b, char c, int n) { memset(b, c, (size_t)n); b[n] = 0; }

/* ---- the limits ---- */
static void test_limits(void) {
    char name[40], body[300], v[200];
    TEST_ASSERT_EQUAL(8, WF_CSSPROP_ALTS);
    TEST_ASSERT_EQUAL(32, WF_CSSPROP_NAME);
    TEST_ASSERT_EQUAL(128, WF_CSSPROP_VAL);
    TEST_ASSERT_EQUAL(16, WF_SITE_PROPS);
    TEST_ASSERT_EQUAL(32, WF_SITE_VNAMES);
    TEST_ASSERT_EQUAL(96, WF_SITE_EVARS);
    TEST_ASSERT_EQUAL(128, WF_SITE_VVAL);
    TEST_ASSERT_EQUAL(32, WF_SITE_VNAME);
    /* eight alternatives, each kept in order; nine refused */
    {
        const char *s8 = "'<url> | <image># | <resolution>+ | <transform-function> | <transform-list> | <string> | a | <length>'";
        TEST_ASSERT_EQUAL(WF_CSSPROP_COMPLEX, wf_cssprop_syntax_parse(s8, (int)strlen(s8), &g_p));
    }
    TEST_ASSERT_EQUAL(8, g_p.nalt);
    TEST_ASSERT_EQUAL(WF_CSSPROP_URL, g_p.alt[0].type);
    TEST_ASSERT_EQUAL(WF_CSSPROP_IMAGE, g_p.alt[1].type);
    TEST_ASSERT_EQUAL(WF_CSSPROP_HASH, g_p.alt[1].mult);
    TEST_ASSERT_EQUAL(WF_CSSPROP_RESOLUTION, g_p.alt[2].type);
    TEST_ASSERT_EQUAL(WF_CSSPROP_PLUS, g_p.alt[2].mult);
    TEST_ASSERT_EQUAL(WF_CSSPROP_TFUNC, g_p.alt[3].type);
    TEST_ASSERT_EQUAL(WF_CSSPROP_TLIST, g_p.alt[4].type);
    TEST_ASSERT_EQUAL(WF_CSSPROP_STRING, g_p.alt[5].type);
    TEST_ASSERT_EQUAL(WF_CSSPROP_KEYWORD, g_p.alt[6].type);
    TEST_ASSERT_EQUAL_STRING("a", g_p.alt[6].ident);
    TEST_ASSERT_EQUAL(WF_CSSPROP_LENGTH, g_p.alt[7].type);
    TEST_ASSERT_EQUAL(WF_CSSPROP_BAD, wf_cssprop_syntax("'a | b | c | d | e | f | g | h | <url>'", 39));
    /* <transform-list> takes no multiplier (it is a list already) */
    TEST_ASSERT_EQUAL(WF_CSSPROP_TLIST, wf_cssprop_syntax("'<transform-list>'", 18));
    TEST_ASSERT_EQUAL(WF_CSSPROP_BAD, wf_cssprop_syntax("'<transform-list>+'", 19));
    TEST_ASSERT_EQUAL(WF_CSSPROP_BAD, wf_cssprop_syntax("'<transform-list>#'", 19));
    TEST_ASSERT_EQUAL(WF_CSSPROP_COMPLEX, wf_cssprop_syntax("'<transform-function>+'", 23));
    /* names: 31 bytes, the 32nd refused */
    name[0] = '-'; name[1] = '-';
    fill(name + 2, 'n', 29);
    snprintf(body, sizeof(body), "syntax: '<length>'; inherits: false; initial-value: 1px");
    TEST_ASSERT_EQUAL(1, wf_cssprop_parse(name, 31, body, (int)strlen(body), &g_p));
    TEST_ASSERT_EQUAL_STRING(name, g_p.name);
    fill(name + 2, 'n', 30);
    TEST_ASSERT_EQUAL(0, wf_cssprop_parse(name, 32, body, (int)strlen(body), &g_p));
    /* initial values: 127 bytes, the 128th refused */
    fill(v, 'v', 127);
    snprintf(body, sizeof(body), "syntax: '*'; inherits: false; initial-value: %s", v);
    TEST_ASSERT_EQUAL(1, wf_cssprop_parse("--x", 3, body, (int)strlen(body), &g_p));
    TEST_ASSERT_EQUAL_STRING(v, g_p.initial);
    fill(v, 'v', 128);
    snprintf(body, sizeof(body), "syntax: '*'; inherits: false; initial-value: %s", v);
    TEST_ASSERT_EQUAL(0, wf_cssprop_parse("--x", 3, body, (int)strlen(body), &g_p));
}

static void test_sheet_sixteen_registrations(void) {
    int k, n = 0;
    g_css[0] = 0;
    for (k = 1; k <= 17; k++)
        n += snprintf(g_css + n, sizeof(g_css) - (size_t)n,
                      "@property --p%d { syntax: '<length>'; inherits: false; initial-value: %dpx } ", k, k);
    /* the same name again replaces its registration (not counted) */
    snprintf(g_css + n, sizeof(g_css) - (size_t)n,
             "@property --p3 { syntax: '<number>'; inherits: true; initial-value: 33 }");
    prep(g_css, "<p id=a>a</p>", 0, 0);
    TEST_ASSERT_EQUAL(16, g_d.nprops);
    TEST_ASSERT_EQUAL(1, g_d.dropped_props);
    TEST_ASSERT_NOT_NULL(wf_site_prop(&g_d, "--p16"));
    TEST_ASSERT_NULL(wf_site_prop(&g_d, "--p17"));
    TEST_ASSERT_EQUAL_STRING("16px", nv("a", "--p16"));
    TEST_ASSERT_EQUAL_STRING("33", nv("a", "--p3"));
    TEST_ASSERT_EQUAL_STRING("(none)", nv("a", "--p17"));
}

static void test_sheet_names_and_values(void) {
    int k, n = 0;
    char v127[130], v128[130], text[160], name31[40], name32[40];
    fill(v127, 'a', 127);
    fill(v128, 'b', 128);
    name31[0] = name32[0] = '-'; name31[1] = name32[1] = '-';
    fill(name31 + 2, 'm', 29);
    fill(name32 + 2, 'w', 30);
    g_css[0] = 0;
    for (k = 1; k <= 30; k++) n += snprintf(g_css + n, sizeof(g_css) - (size_t)n, ".a{--v%d:%dpx}", k, k);
    n += snprintf(g_css + n, sizeof(g_css) - (size_t)n, ".a { %s: m } .a { %s: w }", name31, name32);
    n += snprintf(g_css + n, sizeof(g_css) - (size_t)n, ".a { --long: %s } .a { --long2: 1px } .a { --long2: %s }", v127, v128);
    prep(g_css, "<p id=a class=a>a</p>", 0, 0);
    /* 30 + the 31-byte name + --long: 32 names; the 32-byte name and
     * --long2 are left out */
    TEST_ASSERT_EQUAL(WF_SITE_VNAMES, g_d.nvnames);
    TEST_ASSERT_EQUAL_STRING("30px", nv("a", "--v30"));
    TEST_ASSERT_EQUAL_STRING("m", nv("a", name31));
    TEST_ASSERT_EQUAL_STRING("(none)", nv("a", name32));
    TEST_ASSERT_EQUAL_STRING(v127, nv("a", "--long"));
    TEST_ASSERT_EQUAL_STRING("(none)", nv("a", "--long2"));
    TEST_ASSERT_TRUE(g_d.dropped_vars >= 2);
    TEST_ASSERT(wf_site_vars_text(&g_d, text, (int)sizeof(text)) > 0);
    TEST_ASSERT_NOT_NULL(strstr(text, "names 32 "));
    /* 127 bytes on an element; 128 bytes: dropped as a custom property */
    snprintf(g_css, sizeof(g_css), ".x { --s: %s } .x { --t: 1px } .x { --t: %s }", v127, v128);
    prep(g_css, "<p id=p class=x>p</p>", 0, 0);
    TEST_ASSERT_EQUAL_STRING(v127, nv("p", "--s"));
    TEST_ASSERT_EQUAL_STRING("1px", nv("p", "--t"));
    TEST_ASSERT_EQUAL(1, g_d.dropped_vars);
    TEST_ASSERT_EQUAL(0, g_d.long_values);
}

static void test_long_declarations(void) {
    /* the declaration buffer has one byte more for custom properties
     * only: a 128-byte value of another property is still too long */
    char v127[130], v128[130];
    fill(v127, 'a', 127);
    fill(v128, 'b', 128);
    snprintf(g_css, sizeof(g_css), "p { font-family: %s } p { font-family: x, %s }", v127, v128 + 3);
    prep(g_css, "<p id=p>p</p>", 0, 0);
    TEST_ASSERT_EQUAL(1, g_d.long_values);
    snprintf(g_css, sizeof(g_css), "p { font-family: %s }", v127);
    prep(g_css, "<p id=p>p</p>", 0, 0);
    TEST_ASSERT_EQUAL(0, g_d.long_values);
    snprintf(g_css, sizeof(g_css), "p { font-family: %s } p { --c: %sx }", v128, v128);
    prep(g_css, "<p id=p>p</p>", 0, 0);
    TEST_ASSERT_EQUAL(2, g_d.long_values);
}

/* ---- the Lot 95 types ---- */
static void test_types(void) {
    TEST_ASSERT_EQUAL(1, wf_cssprop_match(WF_CSSPROP_URL, "url(a.png)"));
    TEST_ASSERT_EQUAL(1, wf_cssprop_match(WF_CSSPROP_URL, "url( 'a b.png' )"));
    TEST_ASSERT_EQUAL(1, wf_cssprop_match(WF_CSSPROP_URL, "url(\"a'b\")"));
    TEST_ASSERT_EQUAL(1, wf_cssprop_match(WF_CSSPROP_URL, "url(a\\(b)"));
    TEST_ASSERT_EQUAL(0, wf_cssprop_match(WF_CSSPROP_URL, "url(a b)"));
    TEST_ASSERT_EQUAL(0, wf_cssprop_match(WF_CSSPROP_URL, "url(a(b))"));
    TEST_ASSERT_EQUAL(0, wf_cssprop_match(WF_CSSPROP_URL, "url('a)"));
    TEST_ASSERT_EQUAL(0, wf_cssprop_match(WF_CSSPROP_URL, "url(a) x"));
    TEST_ASSERT_EQUAL(0, wf_cssprop_match(WF_CSSPROP_URL, "uri(a)"));
    TEST_ASSERT_EQUAL(1, wf_cssprop_match(WF_CSSPROP_STRING, "'a\\'b'"));
    TEST_ASSERT_EQUAL(1, wf_cssprop_match(WF_CSSPROP_STRING, "\"\""));
    TEST_ASSERT_EQUAL(0, wf_cssprop_match(WF_CSSPROP_STRING, "'a'b'"));
    TEST_ASSERT_EQUAL(0, wf_cssprop_match(WF_CSSPROP_STRING, "'a\\'"));
    TEST_ASSERT_EQUAL(0, wf_cssprop_match(WF_CSSPROP_STRING, "abc"));
    TEST_ASSERT_EQUAL(1, wf_cssprop_match(WF_CSSPROP_IMAGE, "url(a.png)"));
    TEST_ASSERT_EQUAL(1, wf_cssprop_match(WF_CSSPROP_IMAGE, "linear-gradient(red, blue)"));
    /* Lot 96 (converted): the stops are checked by the background reader,
     * "(blue)" is not a colour (Lot 95: 1, balanced parentheses only) */
    TEST_ASSERT_EQUAL(0, wf_cssprop_match(WF_CSSPROP_IMAGE, "repeating-conic-gradient(from 1deg, red, (blue))"));
    TEST_ASSERT_EQUAL(1, wf_cssprop_match(WF_CSSPROP_IMAGE, "repeating-conic-gradient(from 1deg, red, blue)"));
    TEST_ASSERT_EQUAL(0, wf_cssprop_match(WF_CSSPROP_IMAGE, "linear-gradient( )"));
    TEST_ASSERT_EQUAL(0, wf_cssprop_match(WF_CSSPROP_IMAGE, "linear-gradient(red, blue"));
    TEST_ASSERT_EQUAL(0, wf_cssprop_match(WF_CSSPROP_IMAGE, "linear-gradient(red) x"));
    TEST_ASSERT_EQUAL(0, wf_cssprop_match(WF_CSSPROP_IMAGE, "element(#a)"));
    TEST_ASSERT_EQUAL(1, wf_cssprop_match(WF_CSSPROP_RESOLUTION, "96dpi"));
    TEST_ASSERT_EQUAL(1, wf_cssprop_match(WF_CSSPROP_RESOLUTION, "2x"));
    TEST_ASSERT_EQUAL(1, wf_cssprop_match(WF_CSSPROP_RESOLUTION, "calc(1dppx + 10dpcm)"));
    TEST_ASSERT_EQUAL(0, wf_cssprop_match(WF_CSSPROP_RESOLUTION, "2"));
    TEST_ASSERT_EQUAL(0, wf_cssprop_match(WF_CSSPROP_RESOLUTION, "calc(1x + 1px)"));
    TEST_ASSERT_EQUAL(1, wf_cssprop_match(WF_CSSPROP_TFUNC, "rotate(0)"));
    TEST_ASSERT_EQUAL(1, wf_cssprop_match(WF_CSSPROP_TFUNC, "translate(1px, 10%)"));
    TEST_ASSERT_EQUAL(1, wf_cssprop_match(WF_CSSPROP_TFUNC, "matrix(1, 0, 0, 1, 0, 0)"));
    TEST_ASSERT_EQUAL(0, wf_cssprop_match(WF_CSSPROP_TFUNC, "matrix(1, 0, 0, 1, 0)"));
    TEST_ASSERT_EQUAL(0, wf_cssprop_match(WF_CSSPROP_TFUNC, "translatez(10%)"));
    TEST_ASSERT_EQUAL(0, wf_cssprop_match(WF_CSSPROP_TFUNC, "rotate(calc(0))"));
    TEST_ASSERT_EQUAL(0, wf_cssprop_match(WF_CSSPROP_TFUNC, "translate(1px 2px)"));
    TEST_ASSERT_EQUAL(0, wf_cssprop_match(WF_CSSPROP_TFUNC, "rotate(1deg) scale(2)"));
    TEST_ASSERT_EQUAL(1, wf_cssprop_match(WF_CSSPROP_TLIST, "rotate(1deg) scale(2)"));
    TEST_ASSERT_EQUAL(1, wf_cssprop_match(WF_CSSPROP_TLIST, "scale(2)"));
    TEST_ASSERT_EQUAL(0, wf_cssprop_match(WF_CSSPROP_TLIST, "rotate(1deg), scale(2)"));
    TEST_ASSERT_EQUAL(0, wf_cssprop_match(WF_CSSPROP_TLIST, "none"));
    TEST_ASSERT_EQUAL(0, wf_cssprop_match(WF_CSSPROP_TLIST, ""));
}

/* ---- computed values ---- */
static void test_compute_plain(void) {
    TEST_ASSERT_EQUAL_STRING("96px", cv("'<length>'", "1in"));
    TEST_ASSERT_EQUAL_STRING("0px", cv("'<length>'", "0"));
    TEST_ASSERT_EQUAL_STRING("0px", cv("'<length-percentage>'", "-0.0"));
    TEST_ASSERT_EQUAL_STRING("KEEP", cv("'<number>'", "0"));
    TEST_ASSERT_EQUAL_STRING("1.5", cv("'<number>'", "+1.50"));
    TEST_ASSERT_EQUAL_STRING("2", cv("'<integer>'", "+2"));
    TEST_ASSERT_EQUAL_STRING("90deg", cv("'<angle>'", "100grad"));
    TEST_ASSERT_EQUAL_STRING("0.5s", cv("'<time>'", "500ms"));
    TEST_ASSERT_EQUAL_STRING("1dppx", cv("'<resolution>'", "96dpi"));
    TEST_ASSERT_EQUAL_STRING("2dppx", cv("'<resolution>'", "2x"));
    TEST_ASSERT_EQUAL_STRING("0.0265dppx", cv("'<resolution>'", "1dpcm"));
    TEST_ASSERT_EQUAL_STRING("KEEP", cv("'<length>'", "12px"));
    TEST_ASSERT_EQUAL_STRING("KEEP", cv("'<length>'", "2em"));
    TEST_ASSERT_EQUAL_STRING("KEEP", cv("'<length>'", "150000px"));
    TEST_ASSERT_EQUAL_STRING("KEEP", cv("'<url> | <string>'", "'s'"));
    TEST_ASSERT_EQUAL_STRING("KEEP", cv("'auto | <length>'", "auto"));
    TEST_ASSERT_EQUAL_STRING("KEEP", cv("'*'", "1in"));
    /* the first alternative that matches computes */
    TEST_ASSERT_EQUAL_STRING("KEEP", cv("'<number> | <length>'", "0"));
    TEST_ASSERT_EQUAL_STRING("0px", cv("'<length> | <number>'", "0"));
}

static void test_compute_viewport(void) {
    TEST_ASSERT_EQUAL_STRING("KEEP", cv("'<length>'", "50vw"));
    TEST_ASSERT_EQUAL_STRING("400px", cvp("'<length>'", "50vw", 800, 600));
    TEST_ASSERT_EQUAL_STRING("300px", cvp("'<length>'", "50vh", 800, 600));
    TEST_ASSERT_EQUAL_STRING("60px", cvp("'<length>'", "10vmin", 800, 600));
    TEST_ASSERT_EQUAL_STRING("80px", cvp("'<length>'", "10vmax", 800, 600));
    TEST_ASSERT_EQUAL_STRING("0.08px", cvp("'<length>'", "0.01vw", 800, 600));
    TEST_ASSERT_EQUAL_STRING("KEEP", cvp("'<length>'", "50vw", 0, 600));
    TEST_ASSERT_EQUAL_STRING("KEEP", cvp("'<length>'", "50vh", 800, 0));
    TEST_ASSERT_EQUAL_STRING("calc(10% + 410px)", cvp("'<length-percentage>'", "calc(50vw + 10px + 10%)", 800, 600));
    TEST_ASSERT_EQUAL_STRING("400px, 1em", cvp("'<length>#'", "50vw, 1em", 800, 600));
}

static void test_compute_mixes(void) {
    TEST_ASSERT_EQUAL_STRING("calc(8% + 5px)", cv("'<length-percentage>'", "calc(10% + 5px - 2%)"));
    TEST_ASSERT_EQUAL_STRING("calc(-10% + 5px)", cv("'<length-percentage>'", "calc(5px - 10%)"));
    TEST_ASSERT_EQUAL_STRING("KEEP", cv("'<length-percentage>'", "calc(10% - 5px)"));     /* the same text */
    TEST_ASSERT_EQUAL_STRING("calc(10% - 5px)", cv("'<length-percentage>'", "calc(10% - 5px + 0px)"));
    TEST_ASSERT_EQUAL_STRING("calc(4% + 2px)", cv("'<length-percentage>'", "calc((1px + 2%) * 2)"));
    TEST_ASSERT_EQUAL_STRING("calc(4% + 2px)", cv("'<length-percentage>'", "calc(2 * (1px + 2%))"));
    TEST_ASSERT_EQUAL_STRING("calc(1% + 0.5px)", cv("'<length-percentage>'", "calc((1px + 2%) / 2)"));
    TEST_ASSERT_EQUAL_STRING("calc(0% + 1px)", cv("'<length-percentage>'", "calc(1px + 0%)"));
    TEST_ASSERT_EQUAL_STRING("calc(2% + 96px)", cv("'<length-percentage>'", "calc(calc(1in + 2%))"));
    TEST_ASSERT_EQUAL_STRING("KEEP", cv("'<length-percentage>'", "calc(2% + 1px)"));
    /* a mix is known only through calc() */
    TEST_ASSERT_EQUAL_STRING("KEEP", cv("'<length-percentage>'", "min(1px, 2%)"));
    TEST_ASSERT_EQUAL_STRING("KEEP", cv("'<length-percentage>'", "abs(calc(1px + 2%))"));
    TEST_ASSERT_EQUAL_STRING("KEEP", cv("'<length-percentage>'", "max(calc(1px + 2%), 3px)"));
    TEST_ASSERT_EQUAL_STRING("KEEP", cv("'<length-percentage>'", "calc(1em + 2%)"));
    TEST_ASSERT_EQUAL_STRING("KEEP", cv("'<length-percentage>'", "calc(1px + 200000%)"));
    TEST_ASSERT_EQUAL_STRING("KEEP", cv("'<length-percentage>'", "calc((1px + 2%) * 100000)"));
    TEST_ASSERT_EQUAL_STRING("KEEP", cv("'<length-percentage>'", "calc((1px + 2%) / 0)"));
    TEST_ASSERT_EQUAL_STRING("KEEP", cv("'<length>'", "calc(1px + 2%)"));
    TEST_ASSERT_EQUAL_STRING("3px", cv("'<length-percentage>'", "calc(1px + 2px)"));
    TEST_ASSERT_EQUAL_STRING("3%", cv("'<length-percentage>'", "calc(1% + 2%)"));
    {
        int t = -1;
        TEST_ASSERT_EQUAL(1, wf_cssprop_math_eval("calc(1px + 5% * 2)", &t, g_o, (int)sizeof(g_o)));
        TEST_ASSERT_EQUAL(WF_CSSPROP_LENPCT, t);
        TEST_ASSERT_EQUAL_STRING("calc(10% + 1px)", g_o);
    }
}

static void test_compute_lists(void) {
    TEST_ASSERT_EQUAL_STRING("96px 2px 0px", cv("'<length>+'", "1in  2px\t0"));
    TEST_ASSERT_EQUAL_STRING("96px, 2.6667px, 3em", cv("'<length>#'", "1in,2pt , 3em"));
    TEST_ASSERT_EQUAL_STRING("2, 3", cv("'<integer>#'", "calc(3 / 2), +3"));
    TEST_ASSERT_EQUAL_STRING("-1 2", cv("'<integer>+'", "calc(-3 / 2) calc(5 / 2 - 0.5)"));
    TEST_ASSERT_EQUAL_STRING("KEEP", cv("'<length>#'", "1px, 2px"));
    TEST_ASSERT_EQUAL_STRING("KEEP", cv("'<url>#'", "url(a), url(b)"));
    TEST_ASSERT_EQUAL_STRING("1dppx 2dppx", cv("'<resolution>+'", "96dpi 2x"));
    TEST_ASSERT_EQUAL_STRING("calc(1% + 1px), 2px", cv("'<length-percentage>#'", "calc(1px + 1%), calc(1px * 2)"));
    /* a computed text longer than the buffer: kept, never cut */
    {
        int r;
        memset(&g_p, 0, sizeof(g_p));
        g_p.syntax = (unsigned char)wf_cssprop_syntax_parse("'<length>+'", 11, &g_p);
        r = wf_cssprop_compute(&g_p, "1in 1in 1in 1in", g_o, 20);
        TEST_ASSERT_EQUAL(1, r);
        TEST_ASSERT_EQUAL_STRING("96px 96px 96px 96px", g_o);
        r = wf_cssprop_compute(&g_p, "1in 1in 1in 1in", g_o, 19);
        TEST_ASSERT_EQUAL(0, r);
        TEST_ASSERT_EQUAL_STRING("", g_o);
        r = wf_cssprop_compute(&g_p, "1in", g_o, 5);
        TEST_ASSERT_EQUAL(1, r);
        TEST_ASSERT_EQUAL_STRING("96px", g_o);
        r = wf_cssprop_compute(&g_p, "1in", g_o, 4);
        TEST_ASSERT_EQUAL(0, r);
        TEST_ASSERT_EQUAL_STRING("", g_o);
    }
}

static void test_compute_transforms(void) {
    TEST_ASSERT_EQUAL_STRING("rotate(0deg)", cv("'<transform-function>'", "rotate(0)"));
    TEST_ASSERT_EQUAL_STRING("rotate(90deg)", cv("'<transform-function>'", "rotate(100grad)"));
    TEST_ASSERT_EQUAL_STRING("translate(96px, 10%)", cv("'<transform-function>'", "translate( 1in ,10% )"));
    TEST_ASSERT_EQUAL_STRING("KEEP", cv("'<transform-function>'", "translate(96px, 10%)"));
    TEST_ASSERT_EQUAL_STRING("translate(0px)", cv("'<transform-function>'", "translate(0)"));
    TEST_ASSERT_EQUAL_STRING("scale(2, 50%)", cv("'<transform-function>'", "scale(2,50%)"));
    TEST_ASSERT_EQUAL_STRING("matrix(1, 0, 0, 1, 0, 0)", cv("'<transform-function>'", "matrix(1,0,0,1,0,+0)"));
    TEST_ASSERT_EQUAL_STRING("rotate3d(1, 0, 0, 90deg)", cv("'<transform-function>'", "rotate3d(1,0,0,0.25turn)"));
    TEST_ASSERT_EQUAL_STRING("perspective(2em)", cv("'<transform-function>'", "perspective( 2em )"));
    TEST_ASSERT_EQUAL_STRING("translatex(96px) rotate(90deg) scale(2, 50%)", cv("'<transform-list>'", "translateX(1in)  rotate(0.25turn) scale(2,50%)"));
    TEST_ASSERT_EQUAL_STRING("translate(400px) skew(0deg, 1deg)", cvp("'<transform-list>'", "translate(50vw)skew(0,1deg)", 800, 600));
    TEST_ASSERT_EQUAL_STRING("translate(calc(10% + 1px))", cv("'<transform-function>'", "translate(calc(1px + 10%))"));
    TEST_ASSERT_EQUAL_STRING("KEEP", cv("'<transform-function> | <length>'", "1px"));
}

static void test_compute_api(void) {
    wf_cssprop_ctx_t c;
    c.vw = 800; c.vh = 600;
    memset(&g_p, 0, sizeof(g_p));
    g_p.syntax = (unsigned char)wf_cssprop_syntax_parse("'<length>'", 10, &g_p);
    strcpy(g_o, "junk");
    TEST_ASSERT_EQUAL(0, wf_cssprop_compute_ctx(0, "1in", &c, g_o, (int)sizeof(g_o)));
    TEST_ASSERT_EQUAL_STRING("", g_o);
    TEST_ASSERT_EQUAL(0, wf_cssprop_compute_ctx(&g_p, 0, &c, g_o, (int)sizeof(g_o)));
    TEST_ASSERT_EQUAL(0, wf_cssprop_compute_ctx(&g_p, "1in", &c, 0, 10));
    strcpy(g_o, "junk");
    TEST_ASSERT_EQUAL(0, wf_cssprop_compute_ctx(&g_p, "1in", &c, g_o, 1));
    TEST_ASSERT_EQUAL_STRING("", g_o);
    TEST_ASSERT_EQUAL(1, wf_cssprop_compute_ctx(&g_p, "1in", &c, g_o, (int)sizeof(g_o)));
    TEST_ASSERT_EQUAL_STRING("96px", g_o);
    /* the viewport is used for this call only */
    TEST_ASSERT_EQUAL(1, wf_cssprop_compute_ctx(&g_p, "1vw", &c, g_o, (int)sizeof(g_o)));
    TEST_ASSERT_EQUAL_STRING("8px", g_o);
    TEST_ASSERT_EQUAL(0, wf_cssprop_compute(&g_p, "1vw", g_o, (int)sizeof(g_o)));
    TEST_ASSERT_EQUAL_STRING("", g_o);
}

/* ---- in the reader sheet ---- */
static void test_sheet_viewport_and_inheritance(void) {
    prep("@property --w { syntax: '<length>'; inherits: true; initial-value: 10vw }"
         "@property --t { syntax: '<transform-list>'; inherits: false; initial-value: rotate(0) }"
         "@property --r { syntax: '<resolution>#'; inherits: true; initial-value: 1x }"
         ":root { --u: 10vw }"
         "#o { --w: 1in; --t: translate(1in) rotate(0.5turn); --r: 96dpi, 2x }"
         "#i { --w: 50vh; --r: url(a) }",
         "<div id=o><p id=i>i</p></div><p id=q>q</p>", 1000, 500);
    TEST_ASSERT_EQUAL(3, g_d.nprops);
    TEST_ASSERT_EQUAL_STRING("96px", nv("o", "--w"));
    TEST_ASSERT_EQUAL_STRING("250px", nv("i", "--w"));
    TEST_ASSERT_EQUAL_STRING("100px", nv("q", "--w"));
    TEST_ASSERT_EQUAL_STRING("translate(96px) rotate(180deg)", nv("o", "--t"));
    TEST_ASSERT_EQUAL_STRING("rotate(0deg)", nv("i", "--t"));
    TEST_ASSERT_EQUAL_STRING("rotate(0deg)", nv("q", "--t"));
    TEST_ASSERT_EQUAL_STRING("1dppx, 2dppx", nv("o", "--r"));
    TEST_ASSERT_EQUAL_STRING("1dppx, 2dppx", nv("i", "--r"));
    TEST_ASSERT_EQUAL_STRING("1dppx", nv("q", "--r"));
    /* an unregistered name keeps its text */
    TEST_ASSERT_EQUAL_STRING("10vw", wf_site_var(&g_d, "--u"));
    /* #o --w, --t, --r and #i --w changed their text */
    TEST_ASSERT_EQUAL_STRING(" simplified 4", tail());
    /* wf_site_prepare: the 640 x 480 default viewport */
    snprintf(g_html, sizeof(g_html), "<html><head><style>@property --w { syntax: '<length>'; inherits: true; "
             "initial-value: 10vw } #a { --w: 10vh }</style></head><body><p id=a>a</p><p id=b>b</p></body></html>");
    TEST_ASSERT_EQUAL(0, wf_site_prepare(&g_d, g_html, (int)strlen(g_html)));
    TEST_ASSERT_EQUAL_STRING("48px", nv("a", "--w"));
    TEST_ASSERT_EQUAL_STRING("64px", nv("b", "--w"));
}

static void test_sheet_counts_changed_text_only(void) {
    prep("@property --l { syntax: '<length>#'; inherits: true; initial-value: 0px }"
         "#o { --l: 1px, 2px } #i { --l: 1px, 0 }",
         "<div id=o><p id=i>i</p></div>", 0, 0);
    TEST_ASSERT_EQUAL_STRING("1px, 2px", nv("o", "--l"));
    TEST_ASSERT_EQUAL_STRING("1px, 0px", nv("i", "--l"));
    TEST_ASSERT_EQUAL_STRING(" simplified 1", tail());
    prep("@property --l { syntax: '<length>'; inherits: true; initial-value: 0 }"
         "#o { --l: 2px }", "<div id=o><p id=i>i</p></div><p id=q>q</p>", 0, 0);
    TEST_ASSERT_EQUAL_STRING("2px", nv("i", "--l"));
    TEST_ASSERT_EQUAL_STRING("0px", nv("q", "--l"));
    TEST_ASSERT_EQUAL_STRING("", tail());
}

/* every cap from 1 to the length + 1: written whole when it fits, else
 * refused; nothing is written at or past cap */
static void sweep(const char *syn, const char *v, const char *want) {
    char buf[160];
    int cap, r, len = (int)strlen(want), k;
    memset(&g_p, 0, sizeof(g_p));
    g_p.syntax = (unsigned char)wf_cssprop_syntax_parse(syn, (int)strlen(syn), &g_p);
    TEST_ASSERT_TRUE(g_p.syntax != WF_CSSPROP_BAD);
    for (cap = 1; cap <= len + 1; cap++) {
        memset(buf, 'Z', sizeof(buf));
        r = wf_cssprop_compute(&g_p, v, buf, cap);
        if (cap > len) {
            TEST_ASSERT_EQUAL(1, r);
            TEST_ASSERT_EQUAL_STRING(want, buf);
        } else {
            TEST_ASSERT_EQUAL(0, r);
            TEST_ASSERT_EQUAL(0, buf[0]);
        }
        for (k = cap; k < (int)sizeof(buf); k++) TEST_ASSERT_EQUAL('Z', buf[k]);
    }
}

static void test_compute_caps(void) {
    /* as-written items (colours) in a list */
    sweep("'<color>#'", "red,blue", "red, blue");
    sweep("'<length>'", "1in", "96px");
    sweep("'<length>'", "-0.375pt", "-0.5px");
    sweep("'<length>'", "0.5pt", "0.6667px");
    sweep("'<length>'", "0.06pt", "0.08px");
    sweep("'<length>#'", "1in, 2pt", "96px, 2.6667px");
    sweep("'<length>+'", "1in 1em", "96px 1em");
    sweep("'<length>+'", "1em 1in", "1em 96px");
    sweep("'<length-percentage>'", "calc(1px + 5%)", "calc(5% + 1px)");
    sweep("'<length-percentage>'", "calc(5% - 1.5pt)", "calc(5% - 2px)");
    sweep("'<length-percentage>#'", "calc(1px + 5%), 0", "calc(5% + 1px), 0px");
    sweep("'<transform-list>'", "translate(1in, 10%) rotate(0)", "translate(96px, 10%) rotate(0deg)");
    sweep("'<transform-function>'", "rotate3d(1,0,0,0.25turn)", "rotate3d(1, 0, 0, 90deg)");
    sweep("'<transform-function>'", "skew(0)", "skew(0deg)");
    sweep("'<transform-function>'", "scale(2,50%)", "scale(2, 50%)");
    sweep("'<integer>#'", "calc(3 / 2), +3", "2, 3");
    sweep("'<resolution>'", "96dpi", "1dppx");
    {
        int r;
        char buf[32];
        memset(buf, 'Z', sizeof(buf));
        r = wf_cssprop_math_eval("calc(1px + 5%)", 0, buf, 15);
        TEST_ASSERT_EQUAL(1, r);
        TEST_ASSERT_EQUAL_STRING("calc(5% + 1px)", buf);
        memset(buf, 'Z', sizeof(buf));
        r = wf_cssprop_math_eval("calc(1px + 5%)", 0, buf, 14);
        TEST_ASSERT_EQUAL(0, r);
        TEST_ASSERT_EQUAL('Z', buf[14]);
    }
}

static void test_more_edges(void) {
    char text[256];
    int k;
    /* strings and urls */
    TEST_ASSERT_EQUAL(0, wf_cssprop_match(WF_CSSPROP_STRING, "'a\nb'"));
    TEST_ASSERT_EQUAL(0, wf_cssprop_match(WF_CSSPROP_STRING, "'a\rb'"));
    TEST_ASSERT_EQUAL(1, wf_cssprop_match(WF_CSSPROP_STRING, "'a\\\nb'") == 0 ? 1 : 0);
    TEST_ASSERT_EQUAL(0, wf_cssprop_match(WF_CSSPROP_URL, "url(a(b.png)"));
    TEST_ASSERT_EQUAL(0, wf_cssprop_match(WF_CSSPROP_URL, "url(a\tb)"));
    TEST_ASSERT_EQUAL(1, wf_cssprop_match(WF_CSSPROP_URL, "url(\t'a b'\t)"));
    /* quotes inside a gradient do not close it; Lot 96 (converted): a
     * quoted stop is not a colour, the gradient is refused (Lot 95: 1) */
    TEST_ASSERT_EQUAL(0, wf_cssprop_match(WF_CSSPROP_IMAGE, "linear-gradient(red, ')')"));
    TEST_ASSERT_EQUAL(0, wf_cssprop_match(WF_CSSPROP_IMAGE, "linear-gradient(red, '\\')')"));
    TEST_ASSERT_EQUAL(0, wf_cssprop_match(WF_CSSPROP_IMAGE, "linear-gradient("));
    /* transform arguments: counts, math functions, digits in names */
    TEST_ASSERT_EQUAL(0, wf_cssprop_match(WF_CSSPROP_TFUNC, "translate(1px, 2px, 0)"));
    TEST_ASSERT_EQUAL(0, wf_cssprop_match(WF_CSSPROP_TFUNC, "matrix(1, 0, 0, 1, 0, 0, 0)"));
    TEST_ASSERT_EQUAL(1, wf_cssprop_match(WF_CSSPROP_TFUNC, "translate(max(1px, 2px), 0)"));
    TEST_ASSERT_EQUAL(1, wf_cssprop_match(WF_CSSPROP_TFUNC, "translate3d(1px, 2%, 0)"));
    TEST_ASSERT_EQUAL(0, wf_cssprop_match(WF_CSSPROP_TFUNC, "rotate(1)"));
    TEST_ASSERT_EQUAL(1, wf_cssprop_match(WF_CSSPROP_TLIST, "rotate(1deg)scale(2)"));
    TEST_ASSERT_EQUAL(0, wf_cssprop_match(WF_CSSPROP_TLIST, "red rotate(1deg)"));
    TEST_ASSERT_EQUAL_STRING("translate(2px, 0px)", cv("'<transform-function>'", "translate(max(1px, 2px), 0)"));
    /* a keyword list and '*' keep their text */
    TEST_ASSERT_EQUAL_STRING("KEEP", cv("'a+'", "a  a"));
    TEST_ASSERT_EQUAL_STRING("KEEP", cv("'*'", " 1in"));
    /* the viewport needs both sides */
    TEST_ASSERT_EQUAL_STRING("KEEP", cvp("'<length>'", "50vw", 800, 0));
    TEST_ASSERT_EQUAL_STRING("KEEP", cvp("'<length>'", "20000vw", 800, 600));
    /* a mix past 100000 units in either part */
    TEST_ASSERT_EQUAL_STRING("KEEP", cv("'<length-percentage>'", "calc(1px + 60000% + 60000%)"));
    TEST_ASSERT_EQUAL_STRING("KEEP", cv("'<length-percentage>'", "calc(1% + 60000px + 60000px)"));
    TEST_ASSERT_EQUAL_STRING("calc(1% + 100000px)", cv("'<length-percentage>'", "calc(1% + 60000px + 40000px)"));
    /* the viewport is gone after the call */
    {
        wf_cssprop_ctx_t c;
        int t;
        c.vw = 800; c.vh = 600;
        TEST_ASSERT_EQUAL_STRING("8px", cvx("'<length>'", "1vw", &c));
        TEST_ASSERT_EQUAL(0, wf_cssprop_math_eval("calc(1vw)", &t, g_o, (int)sizeof(g_o)));
        TEST_ASSERT_EQUAL(WF_CSSPROP_LENGTH, t);
    }
    /* Lot 95 mutation witnesses (run 1 survivors) */
    TEST_ASSERT_EQUAL_STRING("KEEP", cvp("'<length>'", "10vmax", 0, 600));       /* no width: not known */
    TEST_ASSERT_EQUAL_STRING("KEEP", cv("'<length-percentage>'", "calc(1% + 60000px + 60000px - 60000px)"));
    TEST_ASSERT_EQUAL(0, wf_cssprop_match(WF_CSSPROP_TFUNC, "rotate(0degX"));
    TEST_ASSERT_EQUAL(0, wf_cssprop_match(WF_CSSPROP_TFUNC, "scale(1x"));
    TEST_ASSERT_EQUAL(0, wf_cssprop_match(WF_CSSPROP_TFUNC, "translate3d(1px, 2px, 10%)"));
    TEST_ASSERT_EQUAL(1, wf_cssprop_match(WF_CSSPROP_TFUNC, "translate3d(1px, 2%, 3px)"));
    TEST_ASSERT_EQUAL(0, wf_cssprop_match(WF_CSSPROP_TLIST, "  "));
    TEST_ASSERT_EQUAL(0, wf_cssprop_match(WF_CSSPROP_URL, "url(a\\)"));
    TEST_ASSERT_EQUAL(1, wf_cssprop_match(WF_CSSPROP_URL, "url(a\\)b)"));
    TEST_ASSERT_EQUAL(0, wf_cssprop_match(WF_CSSPROP_URL, "url(a\001b)"));
    /* a 31-byte name set on the element and read there */
    {
        char name31[40];
        name31[0] = '-'; name31[1] = '-';
        fill(name31 + 2, 'm', 29);
        snprintf(g_css, sizeof(g_css), "#p { %s: 22px } #p { font-size: var(%s) }", name31, name31);
        prep(g_css, "<p id=p>p</p>", 0, 0);
        k = wf_site_report(&g_d, text, (int)sizeof(text));
        TEST_ASSERT_TRUE(k > 0);
        TEST_ASSERT_NOT_NULL(strstr(text, "node p #p display auto size 22px\n"));
    }
    /* a 31-byte name in var() */
    {
        char name31[40];
        name31[0] = '-'; name31[1] = '-';
        fill(name31 + 2, 'k', 29);
        snprintf(g_css, sizeof(g_css), ":root { %s: 21px } p { font-size: var(%s) }", name31, name31);
        prep(g_css, "<p id=p>p</p>", 0, 0);
        k = wf_site_report(&g_d, text, (int)sizeof(text));
        TEST_ASSERT_TRUE(k > 0);
        TEST_ASSERT_NOT_NULL(strstr(text, "node p #p display auto size 21px\n"));
    }
}

int main(void) {
    unity_init();
    RUN_TEST(test_limits);
    RUN_TEST(test_sheet_sixteen_registrations);
    RUN_TEST(test_sheet_names_and_values);
    RUN_TEST(test_long_declarations);
    RUN_TEST(test_types);
    RUN_TEST(test_compute_plain);
    RUN_TEST(test_compute_viewport);
    RUN_TEST(test_compute_mixes);
    RUN_TEST(test_compute_lists);
    RUN_TEST(test_compute_transforms);
    RUN_TEST(test_compute_api);
    RUN_TEST(test_sheet_viewport_and_inheritance);
    RUN_TEST(test_sheet_counts_changed_text_only);
    RUN_TEST(test_compute_caps);
    RUN_TEST(test_more_edges);
    unity_print_results();
    unity_cleanup();
    return unity_stats.tests_failed == 0 ? 0 : 1;
}
