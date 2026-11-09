/* Lot 85: plain text of a reader page. Headings, lists, tables and
 * preformatted blocks stay. Scripts and tags do not. */
#include "../../framework/unity.h"
#include "../../../userspace/wf_article.h"
#include <string.h>

static char g_out[2048];

static void test_heading_and_paragraph(void) {
    const char *html = "<h1>Harbour</h1><p>The lights came on early.</p>";
    int n = wf_article_text(html, (int)strlen(html), g_out, (int)sizeof(g_out));
    TEST_ASSERT_TRUE(n > 0);
    TEST_ASSERT_EQUAL_STRING("# Harbour\n\nThe lights came on early.", g_out);
}

static void test_list_and_nesting(void) {
    const char *html = "<ul><li>one</li><li>two<ul><li>inner</li></ul></li></ul>";
    wf_article_text(html, (int)strlen(html), g_out, (int)sizeof(g_out));
    TEST_ASSERT_NOT_NULL(strstr(g_out, "- one"));
    TEST_ASSERT_NOT_NULL(strstr(g_out, "- two"));
    TEST_ASSERT_NOT_NULL(strstr(g_out, "  - inner"));
}

static void test_table_row(void) {
    const char *html = "<table><tr><th>status</th><td>meaning</td></tr><tr><td>0</td><td>parsed</td></tr></table>";
    wf_article_text(html, (int)strlen(html), g_out, (int)sizeof(g_out));
    TEST_ASSERT_NOT_NULL(strstr(g_out, "status | meaning"));
    TEST_ASSERT_NOT_NULL(strstr(g_out, "0 | parsed"));
}

static void test_pre_keeps_spaces(void) {
    const char *html = "<pre>make qemu-lot85\n  next</pre>";
    wf_article_text(html, (int)strlen(html), g_out, (int)sizeof(g_out));
    TEST_ASSERT_NOT_NULL(strstr(g_out, "make qemu-lot85\n  next"));
}

static void test_script_is_dropped(void) {
    const char *html = "<p>Visible</p><script>var secret = 1;</script><p>After</p>";
    wf_article_text(html, (int)strlen(html), g_out, (int)sizeof(g_out));
    TEST_ASSERT_NOT_NULL(strstr(g_out, "Visible"));
    TEST_ASSERT_NOT_NULL(strstr(g_out, "After"));
    TEST_ASSERT_NULL(strstr(g_out, "secret"));
}

static void test_blockquote_and_levels(void) {
    const char *html = "<h2>Section</h2><blockquote>A note.</blockquote><h3>Smaller</h3><p>Body</p>";
    wf_article_text(html, (int)strlen(html), g_out, (int)sizeof(g_out));
    TEST_ASSERT_NOT_NULL(strstr(g_out, "## Section"));
    TEST_ASSERT_NOT_NULL(strstr(g_out, "> A note."));
    TEST_ASSERT_NOT_NULL(strstr(g_out, "### Smaller"));
    TEST_ASSERT_NOT_NULL(strstr(g_out, "Body"));
}

static void test_whitespace_collapses(void) {
    const char *html = "<p>one   two\nthree</p>";
    wf_article_text(html, (int)strlen(html), g_out, (int)sizeof(g_out));
    TEST_ASSERT_EQUAL_STRING("one two three", g_out);
}

static void test_comment_is_dropped(void) {
    const char *html = "<p>Yes</p><!-- hidden words --><p>No</p>";
    wf_article_text(html, (int)strlen(html), g_out, (int)sizeof(g_out));
    TEST_ASSERT_NULL(strstr(g_out, "hidden"));
    TEST_ASSERT_NOT_NULL(strstr(g_out, "Yes"));
    TEST_ASSERT_NOT_NULL(strstr(g_out, "No"));
}

static void test_bound_is_respected(void) {
    char small[8];
    const char *html = "<p>abcdefghijklmnop</p>";
    int n = wf_article_text(html, (int)strlen(html), small, (int)sizeof(small));
    TEST_ASSERT_TRUE(n < (int)sizeof(small));
    TEST_ASSERT_TRUE((int)strlen(small) < (int)sizeof(small));
}

static void test_empty(void) {
    TEST_ASSERT_EQUAL(0, wf_article_text("", 0, g_out, (int)sizeof(g_out)));
    TEST_ASSERT_EQUAL(0, g_out[0]);
    TEST_ASSERT_EQUAL(0, wf_article_text("<p>x</p>", 6, 0, 0));
}

int main(void) {
    unity_init();
    RUN_TEST(test_heading_and_paragraph);
    RUN_TEST(test_list_and_nesting);
    RUN_TEST(test_table_row);
    RUN_TEST(test_pre_keeps_spaces);
    RUN_TEST(test_script_is_dropped);
    RUN_TEST(test_blockquote_and_levels);
    RUN_TEST(test_whitespace_collapses);
    RUN_TEST(test_comment_is_dropped);
    RUN_TEST(test_bound_is_respected);
    RUN_TEST(test_empty);
    unity_print_results();
    unity_cleanup();
    return unity_stats.tests_failed == 0 ? 0 : 1;
}
