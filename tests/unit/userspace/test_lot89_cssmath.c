/* Lot 89 (US-031): calc(), min(), max() and clamp() resolved to one
 * length token for font-size, text-indent and letter-spacing. */
#include "../../framework/unity.h"
#include <string.h>
#include <stdio.h>
#include "wf_cssmath.h"
#include "wf_sitecss.h"

static char g_o[32];
static wf_site_t g_d;

static const char *m(const char *s) {
    int r = wf_css_math(s, 1000, 800, g_o, (int)sizeof(g_o));
    if (r == 0) return "NOT";
    if (r < 0) return "ERR";
    return g_o;
}

static void test_calc_units(void) {
    TEST_ASSERT_EQUAL_STRING("24px", m("calc(16px + 8px)"));
    TEST_ASSERT_EQUAL_STRING("24px", m("calc(1.5rem)"));
    TEST_ASSERT_EQUAL_STRING("16px", m("calc(12pt)"));
    TEST_ASSERT_EQUAL_STRING("32px", m("calc(2pc)"));
    TEST_ASSERT_EQUAL_STRING("25px", m("calc(2.5vw)"));          /* 1000 px wide */
    TEST_ASSERT_EQUAL_STRING("8px", m("calc(1vh)"));             /* 800 px high */
    TEST_ASSERT_EQUAL_STRING("16px", m("calc(2vmin)"));
    TEST_ASSERT_EQUAL_STRING("20px", m("calc(2vmax)"));
    TEST_ASSERT_EQUAL_STRING("1.5em", m("calc(1em + 0.5em)"));
    TEST_ASSERT_EQUAL_STRING("1.25em", m("calc(125%)"));
    TEST_ASSERT_EQUAL_STRING("33.33px", m("calc(100px / 3)"));
    TEST_ASSERT_EQUAL_STRING("30px", m("CALC(2 * 15px)"));
    TEST_ASSERT_EQUAL_STRING("30px", m("calc(15px * 2)"));
    TEST_ASSERT_EQUAL_STRING("-4px", m("calc(4px - 8px)"));
    TEST_ASSERT_EQUAL_STRING("22px", m("calc((1rem + 6px) * 1)"));
    TEST_ASSERT_EQUAL_STRING("0px", m("calc(1em - 1em)"));
}

static void test_min_max_clamp(void) {
    TEST_ASSERT_EQUAL_STRING("12px", m("min(12px, 2vw)"));
    TEST_ASSERT_EQUAL_STRING("20px", m("max(12px, 2vw)"));
    TEST_ASSERT_EQUAL_STRING("21px", m("clamp(1rem, 0.5rem + 1.3vw, 2rem)"));
    TEST_ASSERT_EQUAL_STRING("16px", m("clamp(1rem, 1vw, 2rem)"));     /* the floor */
    TEST_ASSERT_EQUAL_STRING("32px", m("clamp(1rem, 10vw, 2rem)"));    /* the ceiling */
    TEST_ASSERT_EQUAL_STRING("40px", m("clamp(40px, 10px, 20px)"));    /* MIN beats MAX */
    TEST_ASSERT_EQUAL_STRING("1.1em", m("clamp(1em, 110%, 2em)"));
    TEST_ASSERT_EQUAL_STRING("14px", m("min(20px, max(14px, 1vw), 30px)"));
    TEST_ASSERT_EQUAL_STRING("26px", m("calc(min(10px, 2vw) + 1rem)"));
}

static void test_refused(void) {
    TEST_ASSERT_EQUAL_STRING("NOT", m("16px"));
    TEST_ASSERT_EQUAL_STRING("NOT", m("large"));
    TEST_ASSERT_EQUAL_STRING("ERR", m("calc(1px+2px)"));          /* + needs spaces */
    TEST_ASSERT_EQUAL_STRING("ERR", m("calc(1px +2px)"));
    TEST_ASSERT_EQUAL_STRING("ERR", m("calc(1em + 2px)"));        /* both parts */
    TEST_ASSERT_EQUAL_STRING("ERR", m("min(1em, 2px)"));          /* not comparable */
    TEST_ASSERT_EQUAL_STRING("ERR", m("calc(2px * 3px)"));
    TEST_ASSERT_EQUAL_STRING("ERR", m("calc(2 / 3px)"));
    TEST_ASSERT_EQUAL_STRING("ERR", m("calc(2px / 0)"));
    TEST_ASSERT_EQUAL_STRING("ERR", m("calc(2)"));                /* no length */
    TEST_ASSERT_EQUAL_STRING("ERR", m("calc(2px"));
    TEST_ASSERT_EQUAL_STRING("ERR", m("calc(2px) x"));
    TEST_ASSERT_EQUAL_STRING("ERR", m("calc(2furlongs)"));
    TEST_ASSERT_EQUAL_STRING("ERR", m("clamp(1px, 2px)"));
    TEST_ASSERT_EQUAL_STRING("ERR", m("calc(99999px * 999)"));    /* bounds */
    TEST_ASSERT_EQUAL_STRING("ERR", m("calc(((((((((1px)))))))))"));   /* nesting 8 */
    TEST_ASSERT_EQUAL_STRING("2px", m("calc((((((1px))))) + 1px)"));   /* nesting 6 is fine */
    TEST_ASSERT_EQUAL(-1, wf_css_math("calc(100px / 3)", 0, 0, g_o, 4));
    TEST_ASSERT_EQUAL(-1, wf_css_math(0, 0, 0, g_o, 8));
    TEST_ASSERT_EQUAL(1, wf_css_is_math("Clamp(1px, 2px, 3px)"));
    TEST_ASSERT_EQUAL(0, wf_css_is_math("calculate"));
}

static void test_default_viewport(void) {
    TEST_ASSERT_EQUAL(1, wf_css_math("calc(10vw)", 0, 0, g_o, (int)sizeof(g_o)));
    TEST_ASSERT_EQUAL_STRING("102.4px", g_o);
    TEST_ASSERT_EQUAL(1, wf_css_math("calc(10vh)", 0, 0, g_o, (int)sizeof(g_o)));
    TEST_ASSERT_EQUAL_STRING("76.8px", g_o);
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
        "#a { font-size: clamp(1rem, 0.5rem + 1.3vw, 2rem) }"
        "#b { text-indent: calc(1em + 0.5em) }"
        "#c { letter-spacing: min(1px, 0.1em) }"
        "#d { font-size: calc(200px) }"
        "#e { font-size: max(10px, 2vw) }"
        "#f { letter-spacing: calc(2px * 1) }"
        "</style></head><body><p id=a>a</p><p id=b>b</p><p id=c>c</p><p id=d>d</p><p id=e>e</p><p id=f>f</p></body></html>";
    char style[200];
    TEST_ASSERT_EQUAL(0, wf_site_prepare_vw(&g_d, html, (int)strlen(html), 1000));
    TEST_ASSERT_EQUAL_STRING("21px", by_id("a")->tok[WF_SITE_K_FSIZE - WF_SITE_K_LIST]);
    TEST_ASSERT_EQUAL_STRING("1.5em", by_id("b")->tok[WF_SITE_K_INDENT - WF_SITE_K_LIST]);
    /* px against em: not resolved, the declaration is dropped */
    TEST_ASSERT_EQUAL_STRING("", by_id("c")->tok[WF_SITE_K_LSPACE - WF_SITE_K_LIST]);
    /* resolved but out of the font-size bounds (96 px): dropped */
    TEST_ASSERT_EQUAL_STRING("", by_id("d")->tok[WF_SITE_K_FSIZE - WF_SITE_K_LIST]);
    TEST_ASSERT_EQUAL_STRING("20px", by_id("e")->tok[WF_SITE_K_FSIZE - WF_SITE_K_LIST]);
    TEST_ASSERT_EQUAL_STRING("2px", by_id("f")->tok[WF_SITE_K_LSPACE - WF_SITE_K_LIST]);
    {
        const char *p = strstr(html, "<p id=a>");
        TEST_ASSERT(wf_site_style_attr(&g_d, (int)(p - html), style, (int)sizeof(style)) > 0);
        TEST_ASSERT_NOT_NULL(strstr(style, "font-size:21px"));
    }
}

int main(void) {
    unity_init();
    RUN_TEST(test_calc_units);
    RUN_TEST(test_min_max_clamp);
    RUN_TEST(test_refused);
    RUN_TEST(test_default_viewport);
    RUN_TEST(test_in_the_sheet);
    unity_print_results();
    unity_cleanup();
    return unity_stats.tests_failed == 0 ? 0 : 1;
}
