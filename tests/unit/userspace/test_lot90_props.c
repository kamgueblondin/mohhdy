/* Lot 90 (US-031): custom properties computed where they are declared,
 * CSS-wide keywords on them, the new bounds, and @property
 * (wf_cssprop.c) in the reader sheet. */
#include "../../framework/unity.h"
#include <string.h>
#include <stdio.h>
#include "wf_sitecss.h"
#include "wf_cssprop.h"

static wf_site_t g_d;
static char g_html[8192];

static void prep(const char *css, const char *body) {
    snprintf(g_html, sizeof(g_html), "<html><head><style>%s</style></head><body>%s</body></html>", css, body);
    TEST_ASSERT_EQUAL(0, wf_site_prepare(&g_d, g_html, (int)strlen(g_html)));
}

static int idx_of(const char *id) {
    int i;
    for (i = 0; i < g_d.nnodes; i++) if (!strcmp(g_d.node[i].id, id)) return i;
    return -1;
}

static const char *nv(const char *id, const char *name) {
    const char *v = wf_site_node_var(&g_d, idx_of(id), name);
    return v ? v : "(none)";
}

static const char *fs_tok(const char *id) {
    return g_d.node[idx_of(id)].tok[WF_SITE_K_FSIZE - WF_SITE_K_LIST];
}

/* ---- wf_cssprop.c ---- */

static void test_syntax(void) {
    TEST_ASSERT_EQUAL(WF_CSSPROP_ANY, wf_cssprop_syntax("'*'", 3));
    TEST_ASSERT_EQUAL(WF_CSSPROP_ANY, wf_cssprop_syntax("\" * \"", 5));
    TEST_ASSERT_EQUAL(WF_CSSPROP_LENGTH, wf_cssprop_syntax(" '<length>' ", 12));
    TEST_ASSERT_EQUAL(WF_CSSPROP_PERCENT, wf_cssprop_syntax("'<percentage>'", 14));
    TEST_ASSERT_EQUAL(WF_CSSPROP_LENPCT, wf_cssprop_syntax("\"<LENGTH-percentage>\"", 21));
    TEST_ASSERT_EQUAL(WF_CSSPROP_NUMBER, wf_cssprop_syntax("'<number>'", 10));
    TEST_ASSERT_EQUAL(WF_CSSPROP_INTEGER, wf_cssprop_syntax("'<integer>'", 11));
    TEST_ASSERT_EQUAL(WF_CSSPROP_COLOR, wf_cssprop_syntax("'<color>'", 9));
    /* no quotes, mismatched quotes, lists, alternatives, unknown types */
    TEST_ASSERT_EQUAL(WF_CSSPROP_BAD, wf_cssprop_syntax("<length>", 8));
    TEST_ASSERT_EQUAL(WF_CSSPROP_BAD, wf_cssprop_syntax("'<length>\"", 10));
    TEST_ASSERT_EQUAL(WF_CSSPROP_BAD, wf_cssprop_syntax("'<length>+'", 11));
    TEST_ASSERT_EQUAL(WF_CSSPROP_BAD, wf_cssprop_syntax("'<length>#'", 11));
    TEST_ASSERT_EQUAL(WF_CSSPROP_BAD, wf_cssprop_syntax("'<length> | auto'", 17));
    TEST_ASSERT_EQUAL(WF_CSSPROP_BAD, wf_cssprop_syntax("'<angle>'", 9));
    TEST_ASSERT_EQUAL(WF_CSSPROP_BAD, wf_cssprop_syntax("''", 2));
    TEST_ASSERT_EQUAL(WF_CSSPROP_BAD, wf_cssprop_syntax("'**'", 4));
    TEST_ASSERT_EQUAL(WF_CSSPROP_BAD, wf_cssprop_syntax(0, 3));
}

static void test_match(void) {
    TEST_ASSERT_EQUAL(1, wf_cssprop_match(WF_CSSPROP_LENGTH, "12px"));
    TEST_ASSERT_EQUAL(1, wf_cssprop_match(WF_CSSPROP_LENGTH, "-1.5em"));
    TEST_ASSERT_EQUAL(1, wf_cssprop_match(WF_CSSPROP_LENGTH, "0"));
    TEST_ASSERT_EQUAL(1, wf_cssprop_match(WF_CSSPROP_LENGTH, "+.5vmax"));
    TEST_ASSERT_EQUAL(1, wf_cssprop_match(WF_CSSPROP_LENGTH, "calc(1px + 2em)"));
    TEST_ASSERT_EQUAL(0, wf_cssprop_match(WF_CSSPROP_LENGTH, "calc(1px + 2em"));
    TEST_ASSERT_EQUAL(0, wf_cssprop_match(WF_CSSPROP_LENGTH, "calc(1px) 2px"));
    TEST_ASSERT_EQUAL(0, wf_cssprop_match(WF_CSSPROP_LENGTH, "calc(1px))"));
    TEST_ASSERT_EQUAL(0, wf_cssprop_match(WF_CSSPROP_LENGTH, "3"));
    TEST_ASSERT_EQUAL(0, wf_cssprop_match(WF_CSSPROP_LENGTH, "10%"));
    TEST_ASSERT_EQUAL(0, wf_cssprop_match(WF_CSSPROP_LENGTH, "1.px"));
    TEST_ASSERT_EQUAL(0, wf_cssprop_match(WF_CSSPROP_LENGTH, "1pz"));
    TEST_ASSERT_EQUAL(0, wf_cssprop_match(WF_CSSPROP_LENGTH, "px"));
    TEST_ASSERT_EQUAL(0, wf_cssprop_match(WF_CSSPROP_LENGTH, ""));
    TEST_ASSERT_EQUAL(1, wf_cssprop_match(WF_CSSPROP_PERCENT, "10%"));
    TEST_ASSERT_EQUAL(0, wf_cssprop_match(WF_CSSPROP_PERCENT, "10%%"));
    TEST_ASSERT_EQUAL(0, wf_cssprop_match(WF_CSSPROP_PERCENT, "0"));
    TEST_ASSERT_EQUAL(1, wf_cssprop_match(WF_CSSPROP_LENPCT, "10%"));
    TEST_ASSERT_EQUAL(1, wf_cssprop_match(WF_CSSPROP_LENPCT, "1in"));
    TEST_ASSERT_EQUAL(1, wf_cssprop_match(WF_CSSPROP_LENPCT, "0.0"));
    TEST_ASSERT_EQUAL(0, wf_cssprop_match(WF_CSSPROP_LENPCT, "0.1"));
    TEST_ASSERT_EQUAL(1, wf_cssprop_match(WF_CSSPROP_NUMBER, "1.25"));
    TEST_ASSERT_EQUAL(0, wf_cssprop_match(WF_CSSPROP_NUMBER, "1px"));
    TEST_ASSERT_EQUAL(1, wf_cssprop_match(WF_CSSPROP_INTEGER, "-7"));
    TEST_ASSERT_EQUAL(0, wf_cssprop_match(WF_CSSPROP_INTEGER, "7.0"));
    TEST_ASSERT_EQUAL(1, wf_cssprop_match(WF_CSSPROP_COLOR, "#abc"));
    TEST_ASSERT_EQUAL(1, wf_cssprop_match(WF_CSSPROP_COLOR, "#aabbccdd"));
    TEST_ASSERT_EQUAL(0, wf_cssprop_match(WF_CSSPROP_COLOR, "#abcde"));
    TEST_ASSERT_EQUAL(0, wf_cssprop_match(WF_CSSPROP_COLOR, "#ggg"));
    TEST_ASSERT_EQUAL(1, wf_cssprop_match(WF_CSSPROP_COLOR, "navy"));
    TEST_ASSERT_EQUAL(1, wf_cssprop_match(WF_CSSPROP_COLOR, "currentcolor"));
    TEST_ASSERT_EQUAL(0, wf_cssprop_match(WF_CSSPROP_COLOR, "navyblue"));
    TEST_ASSERT_EQUAL(1, wf_cssprop_match(WF_CSSPROP_COLOR, "rgb(1, 2, 3)"));
    TEST_ASSERT_EQUAL(1, wf_cssprop_match(WF_CSSPROP_COLOR, "hsl(120deg 50% 50% / 0.5)"));
    TEST_ASSERT_EQUAL(0, wf_cssprop_match(WF_CSSPROP_COLOR, "rgb(1, 2, x)"));
    TEST_ASSERT_EQUAL(0, wf_cssprop_match(WF_CSSPROP_COLOR, "rgb(1, (2), 3)"));
    TEST_ASSERT_EQUAL(0, wf_cssprop_match(WF_CSSPROP_COLOR, "rgb()"));
    TEST_ASSERT_EQUAL(0, wf_cssprop_match(WF_CSSPROP_COLOR, "rgb(1, 2, 3"));
    TEST_ASSERT_EQUAL(1, wf_cssprop_match(WF_CSSPROP_ANY, ""));
    TEST_ASSERT_EQUAL(1, wf_cssprop_match(WF_CSSPROP_ANY, "anything at all"));
    TEST_ASSERT_EQUAL(0, wf_cssprop_match(WF_CSSPROP_BAD, "1px"));
    TEST_ASSERT_EQUAL(0, wf_cssprop_match(WF_CSSPROP_LENGTH, 0));
}

static void test_independent(void) {
    TEST_ASSERT_EQUAL(1, wf_cssprop_independent("12px"));
    TEST_ASSERT_EQUAL(1, wf_cssprop_independent("5vw"));
    TEST_ASSERT_EQUAL(1, wf_cssprop_independent("h1 2px"));
    TEST_ASSERT_EQUAL(0, wf_cssprop_independent("1em"));
    TEST_ASSERT_EQUAL(0, wf_cssprop_independent("calc(2px + 1.5rem)"));
    TEST_ASSERT_EQUAL(0, wf_cssprop_independent("2ex"));
    TEST_ASSERT_EQUAL(0, wf_cssprop_independent("3CH"));
    TEST_ASSERT_EQUAL(0, wf_cssprop_independent("var(--x)"));
    TEST_ASSERT_EQUAL(1, wf_cssprop_independent("somevar(--x)"));
    TEST_ASSERT_EQUAL(1, wf_cssprop_independent("2emx"));
    TEST_ASSERT_EQUAL(0, wf_cssprop_independent(0));
}

static void test_parse(void) {
    wf_cssprop_t p;
    const char *b1 = " syntax: '<length>'; inherits: false; initial-value: 4px ";
    TEST_ASSERT_EQUAL(1, wf_cssprop_parse(" --Gap ", 7, b1, (int)strlen(b1), &p));
    TEST_ASSERT_EQUAL_STRING("--gap", p.name);
    TEST_ASSERT_EQUAL(WF_CSSPROP_LENGTH, p.syntax);
    TEST_ASSERT_EQUAL(0, p.inherits);
    TEST_ASSERT_EQUAL(1, p.has_initial);
    TEST_ASSERT_EQUAL_STRING("4px", p.initial);
    /* '*' needs no initial value; ';' inside a string does not end it */
    {
        const char *b = "syntax: \"*\" ; inherits: TRUE; foo: bar";
        TEST_ASSERT_EQUAL(1, wf_cssprop_parse("--any", 5, b, (int)strlen(b), &p));
        TEST_ASSERT_EQUAL(1, p.inherits);
        TEST_ASSERT_EQUAL(0, p.has_initial);
    }
    {
        const char *b = "syntax: '<color>'; inherits: true; initial-value: rgb(1, 2, 3)";
        TEST_ASSERT_EQUAL(1, wf_cssprop_parse("--c", 3, b, (int)strlen(b), &p));
        TEST_ASSERT_EQUAL_STRING("rgb(1, 2, 3)", p.initial);
    }
    {
        static const char *const bad[] = {
            "inherits: false; initial-value: 4px",                         /* no syntax */
            "syntax: '<length>'; initial-value: 4px",                      /* no inherits */
            "syntax: '<length>'; inherits: maybe; initial-value: 4px",     /* bad inherits */
            "syntax: '<length>'; inherits: false",                         /* no initial */
            "syntax: '<length>'; inherits: false; initial-value: red",     /* wrong type */
            "syntax: '<length>'; inherits: false; initial-value: 2em",     /* not independent */
            "syntax: '*'; inherits: false; initial-value: var(--y)",       /* not independent */
            "syntax: '<angle>'; inherits: false; initial-value: 1deg",     /* unsupported */
            "syntax: '<length>'; inherits: false; initial-value: 4px; broken",
            "syntax: '<length>; inherits: false; initial-value: 4px",       /* open string */
            0
        };
        int k;
        for (k = 0; bad[k]; k++) {
            TEST_ASSERT_EQUAL(0, wf_cssprop_parse("--x", 3, bad[k], (int)strlen(bad[k]), &p));
            TEST_ASSERT_EQUAL(0, p.name[0]);
        }
    }
    /* bad names */
    TEST_ASSERT_EQUAL(0, wf_cssprop_parse("x", 1, b1, (int)strlen(b1), &p));
    TEST_ASSERT_EQUAL(0, wf_cssprop_parse("--", 2, b1, (int)strlen(b1), &p));
    TEST_ASSERT_EQUAL(0, wf_cssprop_parse("--a b", 5, b1, (int)strlen(b1), &p));
    TEST_ASSERT_EQUAL(0, wf_cssprop_parse("--aaaaaaaaaaaaaaaaaaaaaaa", 25, b1, (int)strlen(b1), &p));
    TEST_ASSERT_EQUAL(0, wf_cssprop_parse(0, 0, b1, 1, &p));
    TEST_ASSERT_EQUAL(0, wf_cssprop_parse("--x", 3, b1, (int)strlen(b1), 0));
    {
        const char *b = "syntax: '<length>'; inherits: false; initial-value: 01234567890123456789012345678901234567890123456789012345678901234px";
        TEST_ASSERT_EQUAL(0, wf_cssprop_parse("--x", 3, b, (int)strlen(b), &p));
    }
}

/* ---- in the sheet ---- */

static void test_computed_where_declared(void) {
    /* --b is computed on #o with #o's --a; #i redefines --a but inherits
     * the computed --b (Lot 89 read --b again on #i: 20px) */
    prep("#o { --a: 10px; --b: var(--a) } #i { --a: 20px } p { font-size: var(--b) }",
         "<div id=o><div id=i><p id=p>p</p></div></div>");
    TEST_ASSERT_EQUAL_STRING("10px", nv("i", "--b"));
    TEST_ASSERT_EQUAL_STRING("20px", nv("i", "--a"));
    TEST_ASSERT_EQUAL_STRING("10px", fs_tok("p"));
    TEST_ASSERT(g_d.var_computed >= 1);
}

static void test_sheet_wide_computed_on_root(void) {
    /* :root --b reads :root --a only: .x's --a is not seen */
    prep(":root { --b: var(--a, 7px) } .x { --a: 30px } p { font-size: var(--b) }",
         "<div class=x><p id=p>p</p></div>");
    TEST_ASSERT_EQUAL_STRING("7px", wf_site_var(&g_d, "--b"));
    TEST_ASSERT_EQUAL_STRING("7px", fs_tok("p"));
    /* no fallback: guaranteed-invalid, var(--b, 9px) takes its fallback */
    prep(":root { --b: var(--a) } .x { --a: 30px } p { font-size: var(--b, 9px) }",
         "<div class=x><p id=p>p</p></div>");
    TEST_ASSERT_NULL(wf_site_var(&g_d, "--b"));
    TEST_ASSERT_EQUAL_STRING("9px", fs_tok("p"));
    TEST_ASSERT_EQUAL(1, g_d.var_invalid);
}

static void test_invalid_is_inherited(void) {
    /* #o's --b does not resolve: guaranteed-invalid on #o and its
     * descendants, even when :root has a --b (no fallback to it) */
    prep(":root { --b: 5px } #o { --b: var(--nope) } p { font-size: var(--b, 11px) } span { font-size: var(--b) }",
         "<div id=o><p id=p>p<span id=s>s</span></p></div><p id=q>q</p>");
    TEST_ASSERT_EQUAL_STRING("(none)", nv("p", "--b"));
    TEST_ASSERT_EQUAL_STRING("11px", fs_tok("p"));
    TEST_ASSERT_EQUAL_STRING("", fs_tok("s"));
    TEST_ASSERT_EQUAL_STRING("5px", fs_tok("q"));
}

static void test_cycle_on_element(void) {
    prep("#o { --a: var(--b, 1px); --b: var(--a, 2px) } p { font-size: var(--a, 3px) }",
         "<div id=o><p id=p>p</p></div>");
    TEST_ASSERT_EQUAL_STRING("(none)", nv("o", "--a"));
    TEST_ASSERT_EQUAL_STRING("(none)", nv("o", "--b"));
    TEST_ASSERT_EQUAL_STRING("3px", fs_tok("p"));
}

static void test_keywords(void) {
    prep(":root { --r: 4px } #o { --a: 6px } #i { --a: initial } #j { --a: inherit } #k { --a: unset } #m { --r: initial }"
         "#n { --a: revert }",
         "<div id=o><p id=i>i</p><p id=j>j</p><p id=k>k</p><p id=m>m</p><p id=n>n</p></div>");
    TEST_ASSERT_EQUAL_STRING("(none)", nv("i", "--a"));
    TEST_ASSERT_EQUAL_STRING("6px", nv("j", "--a"));
    TEST_ASSERT_EQUAL_STRING("6px", nv("k", "--a"));
    TEST_ASSERT_EQUAL_STRING("6px", nv("n", "--a"));
    /* initial on a name also kept sheet-wide: guaranteed-invalid there */
    TEST_ASSERT_EQUAL_STRING("(none)", nv("m", "--r"));
    TEST_ASSERT_EQUAL_STRING("4px", nv("o", "--r"));
    /* sheet-wide keywords: nothing above the root */
    prep(":root { --a: inherit; --b: 2px }", "<p id=p>p</p>");
    TEST_ASSERT_NULL(wf_site_var(&g_d, "--a"));
    TEST_ASSERT_EQUAL_STRING("2px", wf_site_var(&g_d, "--b"));
}

static void test_bounds(void) {
    /* a 63-byte value fits; a 64-byte one is left out (never cut) */
    char css[1024];
    char v63[70], v64[70];
    memset(v63, 'a', 63); v63[63] = 0;
    memset(v64, 'b', 64); v64[64] = 0;
    snprintf(css, sizeof(css), ".x { --s: %s } .x { --t: 1px } .x { --t: %s } :root { --u: %s } :root { --w: %s }", v63, v64, v63, v64);
    prep(css, "<p id=p class=x>p</p>");
    TEST_ASSERT_EQUAL_STRING(v63, nv("p", "--s"));
    TEST_ASSERT_EQUAL_STRING("1px", nv("p", "--t"));
    TEST_ASSERT_EQUAL_STRING(v63, wf_site_var(&g_d, "--u"));
    TEST_ASSERT_NULL(wf_site_var(&g_d, "--w"));
    TEST_ASSERT_EQUAL(2, g_d.dropped_vars);
    /* a substitution longer than 63 bytes is invalid */
    snprintf(css, sizeof(css), ".x { --s: %s; --t: var(--s) x }", v63);
    prep(css, "<p id=p class=x>p</p>");
    TEST_ASSERT_EQUAL_STRING("(none)", nv("p", "--t"));
}

static void test_property_registered(void) {
    /* not declared anywhere: the initial value on every element */
    prep("@property --gap { syntax: '<length>'; inherits: false; initial-value: 12px }"
         "#o { --gap: 20px } p { font-size: var(--gap) }",
         "<div id=o><p id=p>p</p></div><p id=q>q</p>");
    TEST_ASSERT_EQUAL(1, g_d.nprops);
    TEST_ASSERT_NOT_NULL(wf_site_prop(&g_d, "--gap"));
    TEST_ASSERT_EQUAL_STRING("20px", nv("o", "--gap"));
    /* inherits: false: the child of #o gets the initial value */
    TEST_ASSERT_EQUAL_STRING("12px", nv("p", "--gap"));
    TEST_ASSERT_EQUAL_STRING("12px", fs_tok("p"));
    TEST_ASSERT_EQUAL_STRING("12px", fs_tok("q"));
}

static void test_property_inherits_and_type(void) {
    prep("@property --gap { syntax: '<length>'; inherits: true; initial-value: 12px }"
         "#o { --gap: 20px } #b { --gap: red } p { font-size: var(--gap) }",
         "<div id=o><p id=p>p</p><div id=b><p id=c>c</p></div></div>");
    TEST_ASSERT_EQUAL_STRING("20px", fs_tok("p"));
    /* a value of the wrong type acts as unset: inherited (20px) */
    TEST_ASSERT_EQUAL_STRING("20px", nv("b", "--gap"));
    TEST_ASSERT_EQUAL_STRING("20px", fs_tok("c"));
    TEST_ASSERT_EQUAL(1, g_d.prop_unset);
    {
        char t[120];
        TEST_ASSERT(wf_site_props_text(&g_d, t, (int)sizeof(t)) > 0);
        TEST_ASSERT_EQUAL(0, strncmp("registered 1 rejected 0 computed 0 invalid 0 unset 1 initial ", t, 61));
    }
}

static void test_property_root_is_per_element(void) {
    /* a registered name on :root goes the per-element way; var() in it
     * with the wrong type after substitution: unset (initial) */
    prep("@property --n { syntax: '<number>'; inherits: true; initial-value: 1 }"
         ":root { --n: 2 } .w { --n: var(--px) } .w { --px: 3px }",
         "<p id=a>a</p><p id=b class=w>b</p>");
    TEST_ASSERT_NULL(wf_site_var(&g_d, "--n"));
    TEST_ASSERT_EQUAL_STRING("2", nv("a", "--n"));
    /* "3px" is not a number: unset, it inherits from html (2) */
    TEST_ASSERT_EQUAL_STRING("2", nv("b", "--n"));
}

static void test_property_invalid_rules(void) {
    prep("@property --a { syntax: '<length>'; inherits: false; initial-value: 2em }"
         "@property --b { syntax: '<length>'; inherits: false }"
         "@property --c { syntax: '*'; inherits: true }"
         "@property --c { syntax: '<integer>'; inherits: true; initial-value: 5 }"
         "@media (max-width: 100px) { @property --d { syntax: '*'; inherits: true } }"
         "@media (min-width: 100px) { @property --e { syntax: '<color>'; inherits: false; initial-value: navy } }"
         "p { --a: 1px }",
         "<p id=p>p</p>");
    TEST_ASSERT_EQUAL(2, g_d.nprops);
    TEST_ASSERT_EQUAL(2, g_d.dropped_props);
    TEST_ASSERT_NULL(wf_site_prop(&g_d, "--a"));
    TEST_ASSERT_NULL(wf_site_prop(&g_d, "--d"));
    /* the last valid rule for a name wins */
    TEST_ASSERT_EQUAL(WF_CSSPROP_INTEGER, wf_site_prop(&g_d, "--c")->syntax);
    TEST_ASSERT_EQUAL_STRING("5", nv("p", "--c"));
    TEST_ASSERT_EQUAL_STRING("navy", nv("p", "--e"));
    /* an unregistered name is not given an initial value */
    TEST_ASSERT_EQUAL_STRING("1px", nv("p", "--a"));
}

static void test_property_table_full(void) {
    char css[2048];
    int k, n = 0;
    for (k = 0; k < WF_SITE_PROPS + 2; k++)
        n += snprintf(css + n, sizeof(css) - n, "@property --p%d { syntax: '*'; inherits: true }", k);
    prep(css, "<p id=p>p</p>");
    TEST_ASSERT_EQUAL(WF_SITE_PROPS, g_d.nprops);
    TEST_ASSERT_EQUAL(2, g_d.dropped_props);
    /* '*' without an initial value: guaranteed-invalid, the fallback */
    prep("@property --z { syntax: '*'; inherits: true } p { font-size: var(--z, 13px) }", "<p id=p>p</p>");
    TEST_ASSERT_EQUAL_STRING("13px", fs_tok("p"));
}

static void test_property_layer_and_supports(void) {
    prep("@layer base { @property --l { syntax: '<length>'; inherits: true; initial-value: 3px } }"
         "@supports (display: block) { @property --s { syntax: '<length>'; inherits: true; initial-value: 4px } }"
         "@scope (.x) { @property --q { syntax: '*'; inherits: true } }",
         "<p id=p>p</p>");
    TEST_ASSERT_EQUAL_STRING("3px", nv("p", "--l"));
    TEST_ASSERT_EQUAL_STRING("4px", nv("p", "--s"));
    TEST_ASSERT_NULL(wf_site_prop(&g_d, "--q"));
}

static void test_style_attribute_computed(void) {
    prep(".o { --a: 1px }", "<div class=o style=\"--a: 8px; --b: var(--a)\"><p id=p style=\"--a: 9px\">p</p></div>");
    TEST_ASSERT_EQUAL_STRING("8px", nv("p", "--b"));
    TEST_ASSERT_EQUAL_STRING("9px", nv("p", "--a"));
}

int main(void) {
    unity_init();
    RUN_TEST(test_syntax);
    RUN_TEST(test_match);
    RUN_TEST(test_independent);
    RUN_TEST(test_parse);
    RUN_TEST(test_computed_where_declared);
    RUN_TEST(test_sheet_wide_computed_on_root);
    RUN_TEST(test_invalid_is_inherited);
    RUN_TEST(test_cycle_on_element);
    RUN_TEST(test_keywords);
    RUN_TEST(test_bounds);
    RUN_TEST(test_property_registered);
    RUN_TEST(test_property_inherits_and_type);
    RUN_TEST(test_property_root_is_per_element);
    RUN_TEST(test_property_invalid_rules);
    RUN_TEST(test_property_table_full);
    RUN_TEST(test_property_layer_and_supports);
    RUN_TEST(test_style_attribute_computed);
    unity_print_results();
    unity_cleanup();
    return unity_stats.tests_failed == 0 ? 0 : 1;
}
