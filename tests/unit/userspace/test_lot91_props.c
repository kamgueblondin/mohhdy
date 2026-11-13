/* Lot 91 (US-031): @property syntaxes with alternatives, lists and
 * keywords, the new types, and calc() type checking, in the reader sheet
 * and in wf_cssprop.c. Hand-written cases next to the generated suites
 * (test_lot91_propsyn.c, test_lot91_calc.c, test_lot91_propmatch.c,
 * test_lot91_proprule.c). */
#include "../../framework/unity.h"
#include <string.h>
#include <stdio.h>
#include "wf_sitecss.h"
#include "wf_cssprop.h"

static wf_site_t g_d;
static char g_html[8192];
static wf_cssprop_t g_p;

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

static int m(const char *syn, const char *v) {
    memset(&g_p, 0, sizeof(g_p));
    g_p.syntax = (unsigned char)wf_cssprop_syntax_parse(syn, (int)strlen(syn), &g_p);
    if (!g_p.syntax) return -1;
    return wf_cssprop_match_prop(&g_p, v);
}

static void test_syntax_parse_fields(void) {
    TEST_ASSERT_EQUAL(WF_CSSPROP_COMPLEX, wf_cssprop_syntax_parse("'<length>+ | AUTO | <Color>#'", (int)strlen("'<length>+ | AUTO | <Color>#'"), &g_p));
    TEST_ASSERT_EQUAL(3, g_p.nalt);
    TEST_ASSERT_EQUAL(WF_CSSPROP_LENGTH, g_p.alt[0].type);
    TEST_ASSERT_EQUAL(WF_CSSPROP_PLUS, g_p.alt[0].mult);
    TEST_ASSERT_EQUAL(WF_CSSPROP_KEYWORD, g_p.alt[1].type);
    TEST_ASSERT_EQUAL_STRING("auto", g_p.alt[1].ident);
    TEST_ASSERT_EQUAL(WF_CSSPROP_COLOR, g_p.alt[2].type);
    TEST_ASSERT_EQUAL(WF_CSSPROP_HASH, g_p.alt[2].mult);
    /* a keyword alone is COMPLEX (it has no type code) */
    TEST_ASSERT_EQUAL(WF_CSSPROP_COMPLEX, wf_cssprop_syntax("'auto'", (int)strlen("'auto'")));
    /* eight alternatives at most (Lot 95, converted: Lot 91 took four and
     * refused 'a|b|c|d|e'), idents of 15 bytes at most */
    TEST_ASSERT_EQUAL(WF_CSSPROP_COMPLEX, wf_cssprop_syntax("'a|b|c|d'", (int)strlen("'a|b|c|d'")));
    TEST_ASSERT_EQUAL(WF_CSSPROP_COMPLEX, wf_cssprop_syntax("'a|b|c|d|e'", (int)strlen("'a|b|c|d|e'")));
    TEST_ASSERT_EQUAL(WF_CSSPROP_COMPLEX, wf_cssprop_syntax("'a|b|c|d|e|f|g|h'", (int)strlen("'a|b|c|d|e|f|g|h'")));
    TEST_ASSERT_EQUAL(WF_CSSPROP_BAD, wf_cssprop_syntax("'a|b|c|d|e|f|g|h|i'", (int)strlen("'a|b|c|d|e|f|g|h|i'")));
    TEST_ASSERT_EQUAL(WF_CSSPROP_COMPLEX, wf_cssprop_syntax("'abcdefghijklmno'", (int)strlen("'abcdefghijklmno'")));
    TEST_ASSERT_EQUAL(WF_CSSPROP_BAD, wf_cssprop_syntax("'abcdefghijklmnop'", (int)strlen("'abcdefghijklmnop'")));
    TEST_ASSERT_EQUAL(WF_CSSPROP_BAD, wf_cssprop_syntax("'inherit'", (int)strlen("'inherit'")));
    TEST_ASSERT_EQUAL(WF_CSSPROP_BAD, wf_cssprop_syntax("'default | <length>'", (int)strlen("'default | <length>'")));
    TEST_ASSERT_EQUAL(WF_CSSPROP_BAD, wf_cssprop_syntax("'* | <length>'", (int)strlen("'* | <length>'")));
    TEST_ASSERT_EQUAL(WF_CSSPROP_BAD, wf_cssprop_syntax("'|<length>'", (int)strlen("'|<length>'")));
    TEST_ASSERT_EQUAL(WF_CSSPROP_BAD, wf_cssprop_syntax("'<length>||auto'", (int)strlen("'<length>||auto'")));
    TEST_ASSERT_EQUAL(WF_CSSPROP_TIME, wf_cssprop_syntax("'<time>'", (int)strlen("'<time>'")));
    TEST_ASSERT_EQUAL(WF_CSSPROP_CIDENT, wf_cssprop_syntax("'<custom-ident>'", (int)strlen("'<custom-ident>'")));
}

static void test_match_lists(void) {
    TEST_ASSERT_EQUAL(1, m("'<length>+'", "1px 2em  0"));
    TEST_ASSERT_EQUAL(0, m("'<length>+'", "1px, 2em"));
    TEST_ASSERT_EQUAL(1, m("'<length>#'", "1px, 2em,0"));
    TEST_ASSERT_EQUAL(0, m("'<length>#'", "1px,, 2em"));
    TEST_ASSERT_EQUAL(0, m("'<length>#'", "1px,"));
    TEST_ASSERT_EQUAL(1, m("'<color>+'", "rgb(1, 2, 3) red #abc"));
    TEST_ASSERT_EQUAL(1, m("'<color>#'", "rgb(1, 2, 3), red"));
    TEST_ASSERT_EQUAL(1, m("'<length>#'", "min(1px, 2px), 3px"));   /* commas inside a function */
    TEST_ASSERT_EQUAL(1, m("'<length>+'", "calc(1px + 2px) 3px"));  /* spaces inside a function */
    TEST_ASSERT_EQUAL(1, m("'auto | <length>'", "auto"));
    TEST_ASSERT_EQUAL(1, m("'auto | <length>'", "4px"));
    TEST_ASSERT_EQUAL(0, m("'auto | <length>'", "none"));
    TEST_ASSERT_EQUAL(1, m("'<custom-ident>'", "foo-bar"));
    TEST_ASSERT_EQUAL(0, m("'<custom-ident>'", "inherit"));
    TEST_ASSERT_EQUAL(0, m("'<custom-ident>'", "1st"));
    TEST_ASSERT_EQUAL(1, m("'<angle>'", "0.25turn"));
    TEST_ASSERT_EQUAL(0, m("'<angle>'", "0"));
    TEST_ASSERT_EQUAL(1, m("'<time>#'", "1s, 200ms"));
    TEST_ASSERT_EQUAL(0, m("'<time>'", "1"));
}

static void test_calc_types(void) {
    TEST_ASSERT_EQUAL(WF_CSSPROP_LENGTH, wf_cssprop_math_type("calc(1px + 2em)"));
    TEST_ASSERT_EQUAL(WF_CSSPROP_LENPCT, wf_cssprop_math_type("calc(1px + 2%)"));
    TEST_ASSERT_EQUAL(WF_CSSPROP_NUMBER, wf_cssprop_math_type("calc(2 * 3)"));
    TEST_ASSERT_EQUAL(WF_CSSPROP_LENGTH, wf_cssprop_math_type("calc(2 * 3px)"));
    TEST_ASSERT_EQUAL(WF_CSSPROP_LENGTH, wf_cssprop_math_type("calc(3px / 2)"));
    TEST_ASSERT_EQUAL(WF_CSSPROP_BAD, wf_cssprop_math_type("calc(3px / 2px)"));
    TEST_ASSERT_EQUAL(WF_CSSPROP_BAD, wf_cssprop_math_type("calc(1px+2px)"));
    TEST_ASSERT_EQUAL(WF_CSSPROP_LENGTH, wf_cssprop_math_type("calc(1px*2)"));
    TEST_ASSERT_EQUAL(WF_CSSPROP_ANGLE, wf_cssprop_math_type("max(1deg, 0.5turn)"));
    TEST_ASSERT_EQUAL(WF_CSSPROP_TIME, wf_cssprop_math_type("clamp(1s, 2ms, 3s)"));
    TEST_ASSERT_EQUAL(WF_CSSPROP_BAD, wf_cssprop_math_type("clamp(1s, 2ms)"));
    TEST_ASSERT_EQUAL(WF_CSSPROP_LENGTH, wf_cssprop_math_type("calc((1px + 2px) * 2)"));
    /* deep nesting is refused, never a crash */
    TEST_ASSERT_EQUAL(WF_CSSPROP_BAD,
                      wf_cssprop_math_type("calc(((((((((((((((((((1px)))))))))))))))))))"));
    TEST_ASSERT_EQUAL(WF_CSSPROP_BAD, wf_cssprop_math_type(0));
    /* the match follows the type */
    TEST_ASSERT_EQUAL(0, wf_cssprop_match(WF_CSSPROP_LENGTH, "calc(1px + 2%)"));
    TEST_ASSERT_EQUAL(1, wf_cssprop_match(WF_CSSPROP_LENPCT, "calc(1px + 2%)"));
    TEST_ASSERT_EQUAL(1, wf_cssprop_match(WF_CSSPROP_INTEGER, "calc(7 / 2)"));
    TEST_ASSERT_EQUAL(0, wf_cssprop_match(WF_CSSPROP_NUMBER, "calc(1px * 2)"));
    TEST_ASSERT_EQUAL(1, wf_cssprop_match(WF_CSSPROP_PERCENT, "min(10%, 20%)"));
}

static void test_sheet_alternatives(void) {
    /* auto | <length>: both kinds of value are kept; a colour acts as
     * unset (inherits false: the initial value) */
    prep("@property --w { syntax: 'auto | <length>'; inherits: false; initial-value: auto }"
         "#a { --w: 12px } #b { --w: auto } #c { --w: red }",
         "<p id=a>a</p><p id=b>b</p><p id=c>c</p><p id=d>d</p>");
    TEST_ASSERT_EQUAL_STRING("12px", nv("a", "--w"));
    TEST_ASSERT_EQUAL_STRING("auto", nv("b", "--w"));
    TEST_ASSERT_EQUAL_STRING("auto", nv("c", "--w"));
    TEST_ASSERT_EQUAL_STRING("auto", nv("d", "--w"));
    TEST_ASSERT_EQUAL(1, g_d.nprops);
}

static void test_sheet_lists_and_calc(void) {
    /* a list registered with inherits: true; a percentage calc() is not a
     * <length> (Lot 90 took it), so the inherited value stays */
    prep("@property --l { syntax: '<length>#'; inherits: true; initial-value: 1px, 2px }"
         "#o { --l: 3px, 4px } #i { --l: calc(1px + 5%) } #j { --l: calc(1px + 5px), 0 }",
         "<div id=o><p id=i>i</p><p id=j>j</p></div><p id=q>q</p>");
    TEST_ASSERT_EQUAL_STRING("3px, 4px", nv("o", "--l"));
    TEST_ASSERT_EQUAL_STRING("3px, 4px", nv("i", "--l"));
    /* Lot 95 (converted): the items of a list are computed (Lot 91 kept
     * "calc(1px + 5px), 0") */
    TEST_ASSERT_EQUAL_STRING("6px, 0px", nv("j", "--l"));
    TEST_ASSERT_EQUAL_STRING("1px, 2px", nv("q", "--l"));
}

static void test_sheet_rejects_bad_rules(void) {
    /* initial-value outside the syntax, a reserved keyword, nine
     * alternatives: rejected and counted, the name stays unregistered.
     * Lot 95 (converted): nine alternatives (Lot 91: five) */
    prep("@property --a { syntax: '<length>+'; inherits: false; initial-value: red }"
         "@property --b { syntax: 'inherit | <length>'; inherits: false; initial-value: 1px }"
         "@property --c { syntax: 'a | b | c | d | e | f | g | h | i'; inherits: false; initial-value: a }"
         "@property --d { syntax: '<angle>'; inherits: true; initial-value: 90deg }",
         "<p id=p>p</p>");
    TEST_ASSERT_EQUAL(1, g_d.nprops);
    TEST_ASSERT_EQUAL(3, g_d.dropped_props);
    TEST_ASSERT_EQUAL_STRING("90deg", nv("p", "--d"));
}

int main(void) {
    unity_init();
    RUN_TEST(test_syntax_parse_fields);
    RUN_TEST(test_match_lists);
    RUN_TEST(test_calc_types);
    RUN_TEST(test_sheet_alternatives);
    RUN_TEST(test_sheet_lists_and_calc);
    RUN_TEST(test_sheet_rejects_bad_rules);
    unity_print_results();
    unity_cleanup();
    return unity_stats.tests_failed == 0 ? 0 : 1;
}
