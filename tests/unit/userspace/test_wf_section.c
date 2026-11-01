/* Lot 71: long pages in sections (userspace/wf_section.c): a reduced page
 * of 1500 paragraphs and a 600-item list is cut into complete documents
 * that each fit the engine budget, nothing is lost or duplicated, open
 * lists are reopened, form placeholders take the current value. */
#include "../../framework/unity.h"
#include "../../../userspace/wf_section.h"
#include "../../../userspace/wf_html.h"
#include <string.h>
#include <stdio.h>

static char g_page[600000];
static char g_red[400 * 1024];
static char g_sec[16384];
static wf_sections_t g_s;

static int atoi(const char *s) { int v = 0; while (*s >= '0' && *s <= '9') v = v * 10 + (*s++ - '0'); return v; }
static int count(const char *s, const char *p) {
    int n = 0;
    const char *q = s;
    while ((q = strstr(q, p)) != 0) { n++; q += strlen(p); }
    return n;
}
static int opens(const char *s) {
    int n = 0, i;
    for (i = 0; s[i]; i++) if (s[i] == '<' && s[i + 1] != '/') n++;
    return n;
}

static void test_long_page_sections(void) {
    int n = 0, i, m, k, total_p = 0, last = -1;
    wf_html_stats_t st;
    n += sprintf(g_page + n, "<html><head><title>Longue page</title></head><body><h1>Titre</h1>");
    for (i = 0; i < 1500; i++) n += sprintf(g_page + n, "<p>paragraphe %04d avec un peu de texte</p>", i);
    n += sprintf(g_page + n, "<ul>");
    for (i = 0; i < 600; i++) n += sprintf(g_page + n, "<li>item %03d</li>", i);
    n += sprintf(g_page + n, "</ul><p>fin de page</p></body></html>");
    TEST_ASSERT_TRUE(n > 256 * 1024 / 4);
    m = wf_html_reduce_ex(g_page, n, 0, g_red, (int)sizeof(g_red), &st, 8000, 0);
    TEST_ASSERT_FALSE(st.truncated);
    TEST_ASSERT_TRUE(wf_section_index(g_red, m, &g_s) > 10);
    TEST_ASSERT_FALSE(g_s.cut);
    TEST_ASSERT_TRUE(g_s.nodes > 2000);
    for (k = 0; k < g_s.n; k++) {
        int len = wf_section_render(g_red, m, &g_s, k, 0, g_sec, (int)sizeof(g_sec));
        TEST_ASSERT_TRUE(len > 0);
        TEST_ASSERT_TRUE(opens(g_sec) <= 155);               /* fits WE_MAX_NODES (160) */
        TEST_ASSERT_NOT_NULL(strstr(g_sec, "<title>Longue page</title>"));
        TEST_ASSERT_NOT_NULL(strstr(g_sec, "</body></html>"));
        TEST_ASSERT_EQUAL(count(g_sec, "<ul>"), count(g_sec, "</ul>"));
        TEST_ASSERT_EQUAL(count(g_sec, "<li>"), count(g_sec, "</li>"));
        total_p += count(g_sec, "paragraphe ");
        /* paragraphs come in order, none twice */
        {
            const char *q = g_sec;
            while ((q = strstr(q, "paragraphe ")) != 0) {
                int v = atoi(q + 11);
                TEST_ASSERT_EQUAL(last + 1, v);
                last = v;
                q += 11;
            }
        }
    }
    TEST_ASSERT_EQUAL(1500, total_p);
    /* the list cut across sections is reopened */
    {
        int with_items = 0, ul_reopen = 0;
        for (k = 0; k < g_s.n; k++) {
            wf_section_render(g_red, m, &g_s, k, 0, g_sec, (int)sizeof(g_sec));
            if (strstr(g_sec, "item ")) with_items++;
            if (g_s.depth[k] > 0 && strcmp(g_s.open[k][0], "ul") == 0) ul_reopen++;
        }
        TEST_ASSERT_TRUE(with_items >= 3);
        TEST_ASSERT_TRUE(ul_reopen >= 2);
    }
    wf_section_render(g_red, m, &g_s, g_s.n - 1, 0, g_sec, (int)sizeof(g_sec));
    TEST_ASSERT_NOT_NULL(strstr(g_sec, "fin de page"));
    TEST_ASSERT_NOT_NULL(strstr(g_sec, "fin de la page]"));
    wf_section_render(g_red, m, &g_s, 0, 0, g_sec, (int)sizeof(g_sec));
    TEST_ASSERT_NOT_NULL(strstr(g_sec, "suite]"));
    TEST_ASSERT_EQUAL(-1, wf_section_render(g_red, m, &g_s, g_s.n, 0, g_sec, (int)sizeof(g_sec)));
    TEST_ASSERT_EQUAL(0, wf_section_of(&g_s, 0));
    TEST_ASSERT_EQUAL(g_s.n - 1, wf_section_of(&g_s, m));
}

static void test_short_page_one_section(void) {
    static const char red[] = "<html><head><title>T</title></head><body><p>court</p></body></html>";
    TEST_ASSERT_EQUAL(1, wf_section_index(red, (int)sizeof(red) - 1, &g_s));
    TEST_ASSERT_TRUE(wf_section_render(red, (int)sizeof(red) - 1, &g_s, 0, 0, g_sec, (int)sizeof(g_sec)) > 0);
    TEST_ASSERT_EQUAL_STRING("<html><head><title>T</title></head><body><p>court</p></body></html>", g_sec);
    TEST_ASSERT_EQUAL(-1, wf_section_index("<p>x</p>", 8, &g_s));
}

static void test_placeholders(void) {
    static wf_form_page_t fp;
    static const char page[] = "<html><body><form action=/s><input name=q placeholder=\"Chercher\"><input type=submit value=Go></form></body></html>";
    wf_url_t base;
    wf_html_stats_t st;
    int m;
    TEST_ASSERT_EQUAL(0, wf_url_parse("http://h/", &base));
    m = wf_html_reduce_ex(page, (int)sizeof(page) - 1, &base, g_red, (int)sizeof(g_red), &st, 8000, &fp);
    TEST_ASSERT_EQUAL(1, wf_section_index(g_red, m, &g_s));
    wf_section_render(g_red, m, &g_s, 0, &fp, g_sec, (int)sizeof(g_sec));
    TEST_ASSERT_NOT_NULL(strstr(g_sec, "<a href=\"mohhdy-form:f:0\">[ Chercher"));
    TEST_ASSERT_NOT_NULL(strstr(g_sec, "<a href=\"mohhdy-form:s:1\">[ Go ]</a>"));
    TEST_ASSERT_NULL(strchr(g_sec, '\001'));
    wf_form_focus(&fp, 0);
    wf_form_insert(&fp, '<');
    wf_form_insert(&fp, (char)0xE9);
    wf_section_render(g_red, m, &g_s, 0, &fp, g_sec, (int)sizeof(g_sec));
    TEST_ASSERT_NOT_NULL(strstr(g_sec, "[ &lt;\xe9|"));
}

static void test_one_huge_paragraph(void) {
    int n = 0, i, m, k, words = 0;
    wf_html_stats_t st;
    n += sprintf(g_page + n, "<html><head><title>P</title></head><body><p>");
    for (i = 0; i < 6000; i++) n += sprintf(g_page + n, "mot%d ", i);
    n += sprintf(g_page + n, "</p></body></html>");
    m = wf_html_reduce_ex(g_page, n, 0, g_red, (int)sizeof(g_red), &st, 8000, 0);
    TEST_ASSERT_TRUE(wf_section_index(g_red, m, &g_s) >= 2);
    for (k = 0; k < g_s.n; k++) {
        int len = wf_section_render(g_red, m, &g_s, k, 0, g_sec, (int)sizeof(g_sec));
        TEST_ASSERT_TRUE(len > 0 && len < (int)sizeof(g_sec));
        TEST_ASSERT_EQUAL(count(g_sec, "<p>"), count(g_sec, "</p>"));
        words += count(g_sec, "mot");
    }
    TEST_ASSERT_EQUAL(6000, words);
}

/* Lot 71: web-find over the whole page */
static void test_find_across_sections(void) {
    int n = 0, i, m, k, cnt = -1, last;
    wf_html_stats_t st;
    n += sprintf(g_page + n, "<html><head><title>Cherche</title></head><body>");
    for (i = 0; i < 1200; i++) n += sprintf(g_page + n, "<p>ligne %04d du texte</p>", i);
    n += sprintf(g_page + n, "<p>Le <b>Caf&Eacute;</b> &amp; th&eacute; au bout</p><p>caf&eacute; encore</p></body></html>");
    m = wf_html_reduce_ex(g_page, n, 0, g_red, (int)sizeof(g_red), &st, 8000, 0);
    TEST_ASSERT_TRUE(wf_section_index(g_red, m, &g_s) > 5);
    last = g_s.n - 1;
    k = wf_section_find(g_red, m, &g_s, "ligne 0000", 0, &cnt);
    TEST_ASSERT_EQUAL(0, k);
    TEST_ASSERT_EQUAL(1, cnt);
    k = wf_section_find(g_red, m, &g_s, "LIGNE 1199", 0, &cnt);           /* case folded */
    TEST_ASSERT_TRUE(k > 0);
    TEST_ASSERT_EQUAL(1, cnt);
    k = wf_section_find(g_red, m, &g_s, "caf\xe9 & th\xe9", 0, &cnt);     /* across a tag, entity decoded, Latin-1 fold */
    TEST_ASSERT_EQUAL(last, k);
    TEST_ASSERT_EQUAL(1, cnt);
    k = wf_section_find(g_red, m, &g_s, "CAF\xc9", 0, &cnt);
    TEST_ASSERT_EQUAL(2, cnt);
    TEST_ASSERT_EQUAL(last, k);
    k = wf_section_find(g_red, m, &g_s, "ligne 0001", last, &cnt);        /* wraps to the start */
    TEST_ASSERT_EQUAL(0, k);
    TEST_ASSERT_EQUAL(-1, wf_section_find(g_red, m, &g_s, "introuvable", 0, &cnt));
    TEST_ASSERT_EQUAL(0, cnt);
    TEST_ASSERT_EQUAL(-1, wf_section_find(g_red, m, &g_s, "", 0, &cnt));
    TEST_ASSERT_EQUAL(-1, wf_section_find(g_red, m, &g_s, "<p>", 0, &cnt)); /* markup is not text */
}

int main(void) {
    unity_init();
    RUN_TEST(test_long_page_sections);
    RUN_TEST(test_find_across_sections);
    RUN_TEST(test_short_page_one_section);
    RUN_TEST(test_placeholders);
    RUN_TEST(test_one_huge_paragraph);
    unity_print_results();
    unity_cleanup();
    return unity_stats.tests_failed == 0 ? 0 : 1;
}
