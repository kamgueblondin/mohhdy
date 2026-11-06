/* Lot 75: reader-view emits engine forms; host only, no DNS. */
#include "../../framework/unity.h"
#include "../../../userspace/wf_html.h"
#include "../../../userspace/wf_form.h"
#include "../../../userspace/wf_section.h"
#include <string.h>
#include <stdio.h>

static char g_red[8192], g_sec[8192];
static wf_form_page_t g_fp;
static wf_sections_t g_s;

static void test_reader_engine_get(void) {
    wf_html_stats_t st;
    wf_url_t base;
    int m, n;
    const char *page =
        "<html><body><form action=\"/echo\" method=\"GET\">"
        "<input type=hidden name=h value=1>"
        "<input name=q value=\"hi\">"
        "<input type=checkbox name=c value=on checked>"
        "<select name=s><option value=a>A</option><option value=b selected>B</option></select>"
        "<textarea name=t>note</textarea>"
        "<button type=submit name=go value=1>Go</button>"
        "</form></body></html>";
    TEST_ASSERT_EQUAL(0, wf_url_parse("http://127.0.0.1:18075/", &base));
    m = wf_html_reduce_ex(page, (int)strlen(page), &base, g_red, (int)sizeof(g_red), &st, 8000, &g_fp);
    TEST_ASSERT_TRUE(m > 0);
    TEST_ASSERT_EQUAL(1, g_fp.nforms);
    TEST_ASSERT_EQUAL_STRING("http://127.0.0.1:18075/echo", g_fp.form[0].action);
    TEST_ASSERT_EQUAL(WF_METHOD_GET, g_fp.form[0].method);
    TEST_ASSERT_NULL(strstr(g_red, "mohhdy-form"));
    TEST_ASSERT_NOT_NULL(strstr(g_red, "<form action=\"http://127.0.0.1:18075/echo\" method=\"get\">"));
    TEST_ASSERT_TRUE(wf_section_index(g_red, m, &g_s) >= 1);
    n = wf_section_render(g_red, m, &g_s, 0, &g_fp, g_sec, (int)sizeof(g_sec));
    TEST_ASSERT_TRUE(n > 0);
    TEST_ASSERT_NULL(strchr(g_sec, '\001'));
    TEST_ASSERT_NOT_NULL(strstr(g_sec, "<input type=\"hidden\" name=\"h\" value=\"1\">"));
    TEST_ASSERT_NOT_NULL(strstr(g_sec, "<input type=\"text\" name=\"q\" value=\"hi\""));
    TEST_ASSERT_NOT_NULL(strstr(g_sec, "checked>"));
    TEST_ASSERT_NOT_NULL(strstr(g_sec, "<option value=\"b\" selected>"));
    TEST_ASSERT_NOT_NULL(strstr(g_sec, "<textarea name=\"t\">note</textarea>"));
    TEST_ASSERT_NOT_NULL(strstr(g_sec, "<button type=\"submit\" name=\"go\" value=\"1\">Go</button>"));
}

static void test_reader_engine_post(void) {
    wf_html_stats_t st;
    wf_url_t base;
    int m, n;
    const char *page =
        "<html><body><form action=/save method=POST enctype=\"multipart/form-data\">"
        "<input name=msg value=\"a b\"><input type=file name=f>"
        "<input type=submit value=Send></form></body></html>";
    TEST_ASSERT_EQUAL(0, wf_url_parse("http://127.0.0.1:18075/page", &base));
    m = wf_html_reduce_ex(page, (int)strlen(page), &base, g_red, (int)sizeof(g_red), &st, 4000, &g_fp);
    TEST_ASSERT_EQUAL(1, g_fp.nforms);
    TEST_ASSERT_EQUAL(WF_METHOD_POST, g_fp.form[0].method);
    TEST_ASSERT_EQUAL(1, g_fp.form[0].multipart);
    TEST_ASSERT_EQUAL_STRING("http://127.0.0.1:18075/save", g_fp.form[0].action);
    TEST_ASSERT_NOT_NULL(strstr(g_red, "enctype=\"multipart/form-data\""));
    TEST_ASSERT_EQUAL(1, wf_section_index(g_red, m, &g_s));
    n = wf_section_render(g_red, m, &g_s, 0, &g_fp, g_sec, (int)sizeof(g_sec));
    TEST_ASSERT_TRUE(n > 0);
    TEST_ASSERT_NOT_NULL(strstr(g_sec, "type=\"file\""));
    TEST_ASSERT_NOT_NULL(strstr(g_sec, "method=\"post\""));
}

static void test_reader_live_value_update(void) {
    wf_html_stats_t st;
    wf_url_t base;
    int m;
    const char *page = "<html><body><form action=/s><input name=q value=a>"
                       "<input type=submit value=Go></form></body></html>";
    TEST_ASSERT_EQUAL(0, wf_url_parse("http://127.0.0.1/", &base));
    m = wf_html_reduce_ex(page, (int)strlen(page), &base, g_red, (int)sizeof(g_red), &st, 4000, &g_fp);
    TEST_ASSERT_EQUAL(1, wf_section_index(g_red, m, &g_s));
    wf_section_render(g_red, m, &g_s, 0, &g_fp, g_sec, (int)sizeof(g_sec));
    TEST_ASSERT_NOT_NULL(strstr(g_sec, "value=\"a\""));
    wf_form_focus(&g_fp, 0);
    wf_form_insert(&g_fp, 'Z');
    wf_section_render(g_red, m, &g_s, 0, &g_fp, g_sec, (int)sizeof(g_sec));
    TEST_ASSERT_NOT_NULL(strstr(g_sec, "value=\"aZ\""));
}

static void test_emit_html(void) {
    static wf_form_page_t fp;
    static char out[512];
    wf_url_t base;
    TEST_ASSERT_EQUAL(0, wf_url_parse("http://127.0.0.1/", &base));
    wf_form_reset(&fp);
    TEST_ASSERT_EQUAL(0, wf_form_begin(&fp, &base, "/s", "get", 0));
    TEST_ASSERT_EQUAL(0, wf_form_add(&fp, &base, "email", "e", "a@b.c", "mail", 0, 0));
    TEST_ASSERT_EQUAL(1, wf_form_add(&fp, &base, "file", "f", "", 0, 0, 0));
    TEST_ASSERT_TRUE(wf_form_emit_html(&fp, 0, out, (int)sizeof(out)) > 0);
    TEST_ASSERT_NOT_NULL(strstr(out, "type=\"email\""));
    TEST_ASSERT_NOT_NULL(strstr(out, "name=\"e\""));
    TEST_ASSERT_TRUE(wf_form_emit_html(&fp, 1, out, (int)sizeof(out)) > 0);
    TEST_ASSERT_NOT_NULL(strstr(out, "type=\"file\""));
    TEST_ASSERT_NOT_NULL(strstr(out, "name=\"f\""));
}

int main(void) {
    RUN_TEST(test_emit_html);

    unity_init();
    RUN_TEST(test_reader_engine_get);
    RUN_TEST(test_reader_engine_post);
    RUN_TEST(test_reader_live_value_update);
    unity_print_results();
    unity_cleanup();
    return unity_stats.tests_failed == 0 ? 0 : 1;
}
