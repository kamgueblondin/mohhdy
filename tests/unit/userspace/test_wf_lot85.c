/* Lot 85: the reader keeps the article and skips boilerplate. */
#include "../../framework/unity.h"
#include "../../../userspace/wf_html.h"
#include "../../../userspace/wf_read.h"
#include <stdio.h>
#include <string.h>

static char g_out[8192];

static void test_root_is_the_article(void) {
    const char *page =
        "<html><body><nav><a href=\"/a\">Home</a><a href=\"/b\">News</a></nav>"
        "<article><h1>Mohhdy</h1><p>Un systeme invite, pas un site.</p></article>"
        "<footer><a href=\"/c\">About</a></footer></body></html>";
    wf_read_info_t info;
    int lo = 0, hi = 0;
    char outline[128];
    TEST_ASSERT_EQUAL(1, wf_read_root(page, (int)strlen(page), &lo, &hi, &info));
    TEST_ASSERT_EQUAL(1, info.kind);
    TEST_ASSERT_TRUE(lo > 0 && hi > lo);
    TEST_ASSERT_TRUE(strstr(page + lo, "Mohhdy") == page + lo || strstr(page + lo, "<h1>Mohhdy") != 0);
    TEST_ASSERT_TRUE(wf_read_outline(page, lo, hi, outline, (int)sizeof(outline), 4) > 0);
    TEST_ASSERT_TRUE(strstr(outline, "1 Mohhdy") != 0);
}

static void test_boilerplate_classes(void) {
    TEST_ASSERT_EQUAL(1, wf_read_chrome("toc", ""));
    TEST_ASSERT_EQUAL(1, wf_read_chrome("", "vector-toc-contents"));
    TEST_ASSERT_EQUAL(1, wf_read_chrome("", "navbox"));
    TEST_ASSERT_EQUAL(1, wf_read_chrome("", "cookie-banner"));
    TEST_ASSERT_EQUAL(0, wf_read_chrome("mw-content-text", "mw-parser-output"));
    TEST_ASSERT_EQUAL(0, wf_read_chrome("", "reference-implementation"));
}

static void test_reduce_drops_the_nav_around_main(void) {
    const char *page =
        "<html><head><title>T</title></head><body>"
        "<div class=\"navbox\"><a href=\"/x\">ChromeLink</a></div>"
        "<main><h1>Titre</h1><p>Corps de la page.</p></main>"
        "<div id=\"catlinks\"><a href=\"/y\">Categorie</a></div>"
        "</body></html>";
    wf_html_stats_t st;
    int n = wf_html_reduce(page, (int)strlen(page), 0, g_out, (int)sizeof(g_out), &st);
    TEST_ASSERT_TRUE(n > 0);
    TEST_ASSERT_NOT_NULL(strstr(g_out, "Titre"));
    TEST_ASSERT_NOT_NULL(strstr(g_out, "Corps de la page."));
    TEST_ASSERT_NULL(strstr(g_out, "ChromeLink"));
    TEST_ASSERT_NULL(strstr(g_out, "Categorie"));
    TEST_ASSERT_EQUAL(0, st.truncated);
}

static void test_page_without_a_root_is_kept(void) {
    const char *page = "<html><body><h1>Seul</h1><p>Pas de racine.</p></body></html>";
    wf_html_stats_t st;
    wf_read_info_t info;
    int lo, hi;
    TEST_ASSERT_EQUAL(0, wf_read_root(page, (int)strlen(page), &lo, &hi, &info));
    TEST_ASSERT_EQUAL(0, lo);
    wf_html_reduce(page, (int)strlen(page), 0, g_out, (int)sizeof(g_out), &st);
    TEST_ASSERT_NOT_NULL(strstr(g_out, "Seul"));
    TEST_ASSERT_NOT_NULL(strstr(g_out, "Pas de racine."));
}

int main(void) {
    unity_init();
    RUN_TEST(test_root_is_the_article);
    RUN_TEST(test_boilerplate_classes);
    RUN_TEST(test_reduce_drops_the_nav_around_main);
    RUN_TEST(test_page_without_a_root_is_kept);
    unity_print_results();
    unity_cleanup();
    return unity_stats.tests_failed == 0 ? 0 : 1;
}
