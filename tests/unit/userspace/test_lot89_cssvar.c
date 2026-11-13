/* Lot 89 (US-031): var() anywhere in a value of the reader
 * sheet: inside calc(), nested fallbacks, custom properties defined from
 * other ones, cycles and bounds. */
#include "../../framework/unity.h"
#include <string.h>
#include "wf_cssvar.h"
#include "wf_sitecss.h"

static char g_o[48];
static wf_site_t g_d;

/* a small fixed table: --u 4px, --base 1rem, --gap var(--u), cycles */
static const char *get(const void *ctx, const char *name) {
    static const char *const tab[][2] = {
        {"--u", "4px"}, {"--base", "1rem"}, {"--gap", "var(--u)"}, {"--two", "var(--gap) var(--u)"},
        {"--a", "var(--b)"}, {"--b", "var(--a)"}, {"--self", "var(--self)"}, {"--empty", ""},
        {"--sp", "  2px  "}, {"--long", "0123456789012345678901234567890123456789"},
        {"--d1", "var(--d2)"}, {"--d2", "var(--d3)"}, {"--d3", "var(--d4)"}, {"--d4", "var(--d5)"},
        {"--d5", "var(--d6)"}, {"--d6", "var(--d7)"}, {"--d7", "var(--d8)"}, {"--d8", "1px"},
        {"--d0", "var(--d1)"}, {"--dd", "var(--d0)"},
    };
    unsigned i;
    (void)ctx;
    for (i = 0; i < sizeof(tab) / sizeof(tab[0]); i++) if (strcmp(tab[i][0], name) == 0) return tab[i][1];
    return 0;
}

static const char *sub(const char *s) {
    int r = wf_cssvar_subst(s, g_o, (int)sizeof(g_o), get, 0);
    if (r == 0) return "NOVAR";
    if (r < 0) return "INVALID";
    return g_o;
}

static void test_anywhere_in_a_value(void) {
    TEST_ASSERT_EQUAL_STRING("4px", sub("var(--u)"));
    TEST_ASSERT_EQUAL_STRING("calc(1rem + 4px)", sub("calc(var(--base) + var(--u))"));
    TEST_ASSERT_EQUAL_STRING("4px solid", sub("var(--u) solid"));
    TEST_ASSERT_EQUAL_STRING("4px", sub("VAR( --U )"));            /* case, spaces */
    TEST_ASSERT_EQUAL_STRING("2px", sub("var(--sp)"));               /* outer spaces trimmed */
    /* not a var(): copied, reported as such */
    TEST_ASSERT_EQUAL_STRING("NOVAR", sub("calc(1px + 2px)"));
    TEST_ASSERT_EQUAL_STRING("calc(1px + 2px)", g_o);
    TEST_ASSERT_EQUAL_STRING("NOVAR", sub("somevar(--u)"));
    TEST_ASSERT_EQUAL(2, wf_cssvar_count("calc(var(--a) + var(--b, 1px))"));
    TEST_ASSERT_EQUAL(0, wf_cssvar_count("covariant(--x)"));
    TEST_ASSERT_EQUAL(-1, wf_cssvar_count("var(--x"));
}

static void test_fallbacks(void) {
    TEST_ASSERT_EQUAL_STRING("7px", sub("var(--nope, 7px)"));
    TEST_ASSERT_EQUAL_STRING("4px", sub("var(--u, 7px)"));          /* known: no fallback */
    TEST_ASSERT_EQUAL_STRING("4px", sub("var(--nope, var(--u))"));  /* fallback holds a var() */
    TEST_ASSERT_EQUAL_STRING("9px", sub("var(--x, var(--y, 9px))"));
    TEST_ASSERT_EQUAL_STRING("calc(1px + 2px)", sub("var(--nope, calc(1px + 2px))"));
    TEST_ASSERT_EQUAL_STRING("a, b", sub("var(--nope, a, b)"));     /* commas belong to the fallback */
    /* an empty fallback is an empty substitution */
    TEST_ASSERT_EQUAL(1, wf_cssvar_subst("var(--nope,)", g_o, (int)sizeof(g_o), get, 0));
    TEST_ASSERT_EQUAL_STRING("", g_o);
    TEST_ASSERT_EQUAL(1, wf_cssvar_subst("var(--empty)", g_o, (int)sizeof(g_o), get, 0));
    TEST_ASSERT_EQUAL_STRING("", g_o);
    /* unknown and no fallback: invalid at computed-value time */
    TEST_ASSERT_EQUAL_STRING("INVALID", sub("var(--nope)"));
    TEST_ASSERT_EQUAL_STRING("INVALID", sub("calc(var(--nope) + 1px)"));
    TEST_ASSERT_EQUAL_STRING("INVALID", sub("var(--x, var(--y))"));
}

static void test_chains_and_cycles(void) {
    TEST_ASSERT_EQUAL_STRING("4px", sub("var(--gap)"));
    TEST_ASSERT_EQUAL_STRING("4px 4px", sub("var(--two)"));
    /* a chain of eight references (--d1 .. --d8) resolves, nine do not */
    TEST_ASSERT_EQUAL_STRING("1px", sub("var(--d1)"));
    TEST_ASSERT_EQUAL_STRING("INVALID", sub("var(--d0)"));
    TEST_ASSERT_EQUAL_STRING("INVALID", sub("var(--dd)"));
    /* a cycle is invalid even with a fallback (the cycle is in the value) */
    TEST_ASSERT_EQUAL_STRING("INVALID", sub("var(--a)"));
    TEST_ASSERT_EQUAL_STRING("INVALID", sub("var(--self, 1px)"));
    TEST_ASSERT_EQUAL_STRING("", g_o);
}

static void test_malformed_and_bounds(void) {
    TEST_ASSERT_EQUAL_STRING("INVALID", sub("var(u)"));
    TEST_ASSERT_EQUAL_STRING("INVALID", sub("var(--)"));
    TEST_ASSERT_EQUAL_STRING("INVALID", sub("var(--u"));
    TEST_ASSERT_EQUAL_STRING("INVALID", sub("var(--u x)"));
    TEST_ASSERT_EQUAL_STRING("INVALID", sub("var(--nope, (1px)"));
    TEST_ASSERT_EQUAL_STRING("INVALID", sub("var(--abcdefghijklmnopqrstuvwxyz)"));   /* name too long */
    /* the result must fit: never a cut value */
    TEST_ASSERT_EQUAL_STRING("INVALID", sub("var(--long) var(--long)"));
    TEST_ASSERT_EQUAL(1, wf_cssvar_subst("var(--u)", g_o, 4, get, 0));
    TEST_ASSERT_EQUAL_STRING("4px", g_o);
    TEST_ASSERT_EQUAL(-1, wf_cssvar_subst("var(--u)", g_o, 3, get, 0));
    TEST_ASSERT_EQUAL_STRING("", g_o);
    TEST_ASSERT_EQUAL(-1, wf_cssvar_subst("abcdef", g_o, 4, get, 0));
    TEST_ASSERT_EQUAL(-1, wf_cssvar_subst(0, g_o, 4, get, 0));
    TEST_ASSERT_EQUAL(-1, wf_cssvar_subst("x", 0, 4, get, 0));
    /* no lookup: every var() is unknown */
    TEST_ASSERT_EQUAL(1, wf_cssvar_subst("var(--u, 3px)", g_o, (int)sizeof(g_o), 0, 0));
    TEST_ASSERT_EQUAL_STRING("3px", g_o);
}

static const wf_site_node_t *by_id(const char *id) {
    int i;
    static wf_site_node_t none;
    for (i = 0; i < g_d.nnodes; i++) if (strcmp(g_d.node[i].id, id) == 0) return &g_d.node[i];
    return &none;
}

static void test_in_the_sheet(void) {
    static const char html[] =
        "<html><head><style>"
        ":root { --base: 1rem; --u: 2px; --gap: var(--u); --c: #336699; --a: var(--b); --b: var(--a) }"
        "#a { font-size: calc(var(--base) + var(--u)) }"
        "#b { text-indent: var(--nope, var(--gap)) }"
        "#c { color: var(--c) }"
        "#d { letter-spacing: var(--a, 1px) }"
        "#e { display: var(--nope) }"
        "#f { letter-spacing: calc(var(--gap) * 2) }"
        "</style></head><body><p id=a>a</p><p id=b>b</p><p id=c>c</p><p id=d>d</p><p id=e>e</p><p id=f>f</p></body></html>";
    TEST_ASSERT_EQUAL(0, wf_site_prepare_vw(&g_d, html, (int)strlen(html), 1000));
    TEST_ASSERT_EQUAL_STRING("18px", by_id("a")->tok[WF_SITE_K_FSIZE - WF_SITE_K_LIST]);
    TEST_ASSERT_EQUAL_STRING("2px", by_id("b")->tok[WF_SITE_K_INDENT - WF_SITE_K_LIST]);
    TEST_ASSERT_EQUAL_STRING("4px", by_id("f")->tok[WF_SITE_K_LSPACE - WF_SITE_K_LIST]);
    /* Lot 90 (converted, was ""): a property in a cycle is computed on
     * the root and is guaranteed-invalid there, so var(--a, 1px) takes its
     * fallback (CSS Variables 1, section 2.3 and 3) */
    TEST_ASSERT_EQUAL_STRING("1px", by_id("d")->tok[WF_SITE_K_LSPACE - WF_SITE_K_LIST]);
    TEST_ASSERT_NULL(wf_site_var(&g_d, "--a"));
    TEST_ASSERT_NULL(wf_site_var(&g_d, "--b"));
    {
        char style[200];
        const char *p = strstr(html, "<p id=c>");
        TEST_ASSERT(wf_site_style_attr(&g_d, (int)(p - html), style, (int)sizeof(style)) > 0);
        TEST_ASSERT_NOT_NULL(strstr(style, "color:#336699"));
    }
}

int main(void) {
    unity_init();
    RUN_TEST(test_anywhere_in_a_value);
    RUN_TEST(test_fallbacks);
    RUN_TEST(test_chains_and_cycles);
    RUN_TEST(test_malformed_and_bounds);
    RUN_TEST(test_in_the_sheet);
    unity_print_results();
    unity_cleanup();
    return unity_stats.tests_failed == 0 ? 0 : 1;
}
