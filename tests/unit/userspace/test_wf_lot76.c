/* Lot 76: reader emits type=color and type=date for the engine. Host only. */
#include "../../framework/unity.h"
#include "../../../userspace/wf_html.h"
#include "../../../userspace/wf_form.h"
#include "../../../userspace/wf_section.h"
#include <string.h>
#include <stdio.h>

static char g_red[8192], g_sec[8192];
static wf_form_page_t g_fp;
static wf_sections_t g_s;

static void test_wf_form_add_color_date(void) {
    static wf_form_page_t fp;
    static char out[512];
    wf_url_t base;
    TEST_ASSERT_EQUAL(0, wf_url_parse("http://127.0.0.1/", &base));
    wf_form_reset(&fp);
    TEST_ASSERT_EQUAL(0, wf_form_begin(&fp, &base, "/a", "get", 0));
    TEST_ASSERT_EQUAL(0, wf_form_add(&fp, &base, "color", "c", "#00ff00", 0, 0, 0));
    TEST_ASSERT_EQUAL(1, wf_form_add(&fp, &base, "date", "d", "2020-01-01", 0, 0, 0));
    TEST_ASSERT_EQUAL_STRING("color", fp.field[0].itype);
    TEST_ASSERT_EQUAL_STRING("date", fp.field[1].itype);
    TEST_ASSERT_TRUE(wf_form_emit_html(&fp, 0, out, (int)sizeof(out)) > 0);
    TEST_ASSERT_NOT_NULL(strstr(out, "type=\"color\""));
    TEST_ASSERT_NOT_NULL(strstr(out, "#00ff00"));
    TEST_ASSERT_TRUE(wf_form_emit_html(&fp, 1, out, (int)sizeof(out)) > 0);
    TEST_ASSERT_NOT_NULL(strstr(out, "type=\"date\""));
    TEST_ASSERT_NOT_NULL(strstr(out, "2020-01-01"));
}

static void test_reader_color_date(void) {
    wf_html_stats_t st;
    wf_url_t base;
    int m, n;
    const char *page =
        "<html><body><form action=\"/echo\" method=\"GET\">"
        "<input type=color name=c value=\"#ff0000\">"
        "<input type=date name=d value=\"2024-02-29\">"
        "<input type=submit name=go value=Go>"
        "</form></body></html>";
    TEST_ASSERT_EQUAL(0, wf_url_parse("http://127.0.0.1:18076/", &base));
    m = wf_html_reduce_ex(page, (int)strlen(page), &base, g_red, (int)sizeof(g_red), &st, 8000, &g_fp);
    TEST_ASSERT_TRUE(m > 0);
    TEST_ASSERT_EQUAL(1, g_fp.nforms);
    TEST_ASSERT_TRUE(g_fp.nfields >= 3);
    TEST_ASSERT_TRUE(wf_section_index(g_red, m, &g_s) >= 1);
    n = wf_section_render(g_red, m, &g_s, 0, &g_fp, g_sec, (int)sizeof(g_sec));
    TEST_ASSERT_TRUE(n > 0);
    TEST_ASSERT_NOT_NULL(strstr(g_sec, "type=\"color\""));
    TEST_ASSERT_NOT_NULL(strstr(g_sec, "name=\"c\""));
    TEST_ASSERT_NOT_NULL(strstr(g_sec, "#ff0000"));
    TEST_ASSERT_NOT_NULL(strstr(g_sec, "type=\"date\""));
    TEST_ASSERT_NOT_NULL(strstr(g_sec, "2024-02-29"));
}

int main(void) {
    unity_init();
    RUN_TEST(test_wf_form_add_color_date);
    RUN_TEST(test_reader_color_date);
    unity_print_results();
    unity_cleanup();
    return unity_stats.tests_failed == 0 ? 0 : 1;
}
