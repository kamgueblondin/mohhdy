/* Lot 69: Internet search without JavaScript (userspace/wsearch.c).
 * Real DuckDuckGo pages saved on 2026-10-31 (tests/fixtures/search) and
 * synthetic pages for a bot check and a changed markup. */
#include "../../framework/unity.h"
#include "../../../userspace/wsearch.h"
#include <stdio.h>
#include <string.h>

static char g_html[65536];
static wsearch_page_t g_pg;
static char g_out[16384];

static int load(const char *name) {
    static const char *const pre[] = { "fixtures/search/", "tests/fixtures/search/", "../fixtures/search/", "../../fixtures/search/", 0 };
    char path[256];
    int i, n;
    for (i = 0; pre[i]; i++) {
        FILE *f;
        snprintf(path, sizeof(path), "%s%s", pre[i], name);
        f = fopen(path, "rb");
        if (!f) continue;
        n = (int)fread(g_html, 1, sizeof(g_html) - 1, f);
        fclose(f);
        g_html[n] = 0;
        return n;
    }
    return -1;
}

static void test_urlencode(void) {
    char o[64];
    TEST_ASSERT_EQUAL(17, wsearch_urlencode("mohhdy os 2026!", o, (int)sizeof(o)));
    TEST_ASSERT_EQUAL_STRING("mohhdy+os+2026%21", o);
    wsearch_urlencode("a&b=c/d?e#f%", o, (int)sizeof(o));
    TEST_ASSERT_EQUAL_STRING("a%26b%3Dc%2Fd%3Fe%23f%25", o);
    wsearch_urlencode("caf\xc3\xa9 -_.~", o, (int)sizeof(o));
    TEST_ASSERT_EQUAL_STRING("caf%C3%A9+-_.~", o);
    TEST_ASSERT_EQUAL(-1, wsearch_urlencode("abcdef", o, 4));
    TEST_ASSERT_EQUAL(-1, wsearch_urlencode("%", o, 3));
    TEST_ASSERT_EQUAL(0, wsearch_urlencode("", o, 4));
}

static void test_urldecode(void) {
    char o[64];
    TEST_ASSERT_EQUAL(19, wsearch_urldecode("https%3A%2F%2Fa.b%2F", 20, o, (int)sizeof(o)) + 7);
    TEST_ASSERT_EQUAL_STRING("https://a.b/", o);
    wsearch_urldecode("a+b%2", 6, o, (int)sizeof(o));
    TEST_ASSERT_EQUAL_STRING("a b%2", o);
    wsearch_urldecode("%zz%41", 6, o, (int)sizeof(o));
    TEST_ASSERT_EQUAL_STRING("%zzA", o);
}

static void test_build_url(void) {
    char o[300];
    TEST_ASSERT_EQUAL(0, wsearch_build_url("ddg", "hello world", o, (int)sizeof(o)));
    TEST_ASSERT_EQUAL_STRING("https://html.duckduckgo.com/html/?q=hello+world", o);
    TEST_ASSERT_EQUAL(0, wsearch_build_url("ddg-lite", "x", o, (int)sizeof(o)));
    TEST_ASSERT_EQUAL_STRING("https://lite.duckduckgo.com/lite/?q=x", o);
    TEST_ASSERT_EQUAL(0, wsearch_build_url(0, "x", o, (int)sizeof(o)));
    TEST_ASSERT_EQUAL_STRING("https://html.duckduckgo.com/html/?q=x", o);
    TEST_ASSERT_EQUAL(0, wsearch_build_url("http://10.0.2.2:8080/html/?kl=fr&q=%s&s=0", "a b", o, (int)sizeof(o)));
    TEST_ASSERT_EQUAL_STRING("http://10.0.2.2:8080/html/?kl=fr&q=a+b&s=0", o);
    TEST_ASSERT_EQUAL(0, wsearch_build_url("https://search.example/?q=", "z", o, (int)sizeof(o)));
    TEST_ASSERT_EQUAL_STRING("https://search.example/?q=z", o);
    TEST_ASSERT_EQUAL(-1, wsearch_build_url("https://search.example/", "z", o, (int)sizeof(o)));
    TEST_ASSERT_EQUAL(-1, wsearch_build_url("ftp://x/?q=%s", "z", o, (int)sizeof(o)));
    TEST_ASSERT_EQUAL(-1, wsearch_build_url("bing", "z", o, (int)sizeof(o)));
    TEST_ASSERT_EQUAL(-1, wsearch_build_url("ddg", "", o, (int)sizeof(o)));
    TEST_ASSERT_EQUAL(-1, wsearch_build_url("ddg", "x", o, 20));
    TEST_ASSERT_NULL(wsearch_engine_template("nope"));
    TEST_ASSERT_NOT_NULL(wsearch_engine_template("ddg-html"));
}

static void test_decode_href(void) {
    char o[WSEARCH_URL];
    TEST_ASSERT_EQUAL(0, wsearch_decode_href("//duckduckgo.com/l/?uddg=https%3A%2F%2Fgithub.com%2Fx%3Fa%3D1&amp;rut=abc", o, (int)sizeof(o)));
    TEST_ASSERT_EQUAL_STRING("https://github.com/x?a=1", o);
    TEST_ASSERT_EQUAL(0, wsearch_decode_href("/l/?kh=-1&uddg=http%3A%2F%2Fa.org%2F", o, (int)sizeof(o)));
    TEST_ASSERT_EQUAL_STRING("http://a.org/", o);
    TEST_ASSERT_EQUAL(0, wsearch_decode_href("//example.org/p", o, (int)sizeof(o)));
    TEST_ASSERT_EQUAL_STRING("https://example.org/p", o);
    TEST_ASSERT_EQUAL(0, wsearch_decode_href("https://plain.example/a&amp;b", o, (int)sizeof(o)));
    TEST_ASSERT_EQUAL_STRING("https://plain.example/a&b", o);
    TEST_ASSERT_EQUAL(-1, wsearch_decode_href("javascript:alert(1)", o, (int)sizeof(o)));
    TEST_ASSERT_EQUAL(-1, wsearch_decode_href("/relative", o, (int)sizeof(o)));
    TEST_ASSERT_EQUAL(-1, wsearch_decode_href("//duckduckgo.com/l/?uddg=javascript%3Ax", o, (int)sizeof(o)));
    TEST_ASSERT_EQUAL(-1, wsearch_decode_href("//d.com/l/?uddg=https%3A%2F%2Fa%20b", o, (int)sizeof(o)));
    TEST_ASSERT_EQUAL(-1, wsearch_decode_href("", o, (int)sizeof(o)));
}

static void test_real_ddg_html(void) {
    int n = load("ddg_html_real.html"), i;
    TEST_ASSERT_TRUE(n > 1000);
    TEST_ASSERT_EQUAL(10, wsearch_parse(g_html, n, &g_pg));
    TEST_ASSERT_EQUAL(WSEARCH_LAYOUT_DDG_HTML, g_pg.layout);
    TEST_ASSERT_EQUAL(0, g_pg.blocked);
    TEST_ASSERT_EQUAL_STRING("https://kernelos.org/", g_pg.r[0].url);
    TEST_ASSERT_TRUE(strstr(g_pg.r[0].title, "KernelOS & !K3rnalyze") != 0);
    TEST_ASSERT_EQUAL_STRING("https://github.com/moodyhunter/MOS", g_pg.r[2].url);
    for (i = 0; i < g_pg.n; i++) {
        TEST_ASSERT_TRUE(strncmp(g_pg.r[i].url, "http", 4) == 0);
        TEST_ASSERT_TRUE(strstr(g_pg.r[i].url, "duckduckgo.com") == 0);
        TEST_ASSERT_TRUE(g_pg.r[i].title[0] != 0);
        TEST_ASSERT_TRUE(strlen(g_pg.r[i].snippet) > 10);
        TEST_ASSERT_TRUE(strchr(g_pg.r[i].snippet, '<') == 0);
    }
}

static void test_real_ddg_lite(void) {
    int n = load("ddg_lite_real.html");
    TEST_ASSERT_TRUE(n > 1000);
    TEST_ASSERT_EQUAL(10, wsearch_parse(g_html, n, &g_pg));
    TEST_ASSERT_EQUAL(WSEARCH_LAYOUT_DDG_LITE, g_pg.layout);
    TEST_ASSERT_EQUAL_STRING("https://stackoverflow.com/questions/17692428/what-is-ffreestanding-option-in-gcc", g_pg.r[0].url);
    TEST_ASSERT_TRUE(strstr(g_pg.r[0].title, "ffreestanding") != 0);
    TEST_ASSERT_TRUE(strlen(g_pg.r[0].snippet) > 10);
}

static void test_bot_check_page(void) {
    const char *p = "<html><body><div class=\"anomaly-modal__title\">Unfortunately, bots use DuckDuckGo too.</div>"
                    "<a href=\"https://duckduckgo.com/feedback\">feedback</a><a href=\"https://other.example/\">x</a></body></html>";
    TEST_ASSERT_EQUAL(0, wsearch_parse(p, (int)strlen(p), &g_pg));
    TEST_ASSERT_EQUAL(1, g_pg.blocked);
    TEST_ASSERT_EQUAL(WSEARCH_LAYOUT_NONE, g_pg.layout);
}

static void test_changed_markup_generic(void) {
    const char *p = "<html><body><nav><a href=\"/settings\">Settings</a><a href=\"https://duckduckgo.com/about\">About</a></nav>"
                    "<div class=\"res-x\"><h2><a data-x=1 href=\"//duckduckgo.com/l/?uddg=https%3A%2F%2Fone.example%2F\">One <b>bold</b></a></h2></div>"
                    "<div class=res-x><a href='https://two.example/p?a=1&amp;b=2'>Two &#38; &quot;quoted&quot;</a></div>"
                    "<a href=\"https://two.example/p?a=1&amp;b=2\">dup</a><a href=\"javascript:void(0)\">js</a><a href=\"https://three.example/\"></a>"
                    "</body></html>";
    TEST_ASSERT_EQUAL(2, wsearch_parse(p, (int)strlen(p), &g_pg));
    TEST_ASSERT_EQUAL(WSEARCH_LAYOUT_GENERIC, g_pg.layout);
    TEST_ASSERT_EQUAL_STRING("https://one.example/", g_pg.r[0].url);
    TEST_ASSERT_EQUAL_STRING("One bold", g_pg.r[0].title);
    TEST_ASSERT_EQUAL_STRING("https://two.example/p?a=1&b=2", g_pg.r[1].url);
    TEST_ASSERT_EQUAL_STRING("Two & \"quoted\"", g_pg.r[1].title);
    TEST_ASSERT_TRUE(g_pg.skipped >= 4);
}

static void test_bounds(void) {
    static char big[65536];
    int n = 0, i;
    for (i = 0; i < 40; i++)
        n += snprintf(big + n, sizeof(big) - n, "<a class=\"result__a\" href=\"https://r%d.example/\">Result %d "
                      "with a very long title that goes on and on and on and on and on and on and on and on and on</a>"
                      "<a class=\"result__snippet\">%s</a>", i, i,
                      "snippet snippet snippet snippet snippet snippet snippet snippet snippet snippet snippet snippet "
                      "snippet snippet snippet snippet snippet snippet snippet snippet snippet snippet snippet snippet "
                      "snippet snippet snippet snippet snippet snippet snippet snippet snippet snippet snippet snippet");
    TEST_ASSERT_EQUAL(WSEARCH_MAX, wsearch_parse(big, n, &g_pg));
    TEST_ASSERT_EQUAL(WSEARCH_TITLE - 1, (int)strlen(g_pg.r[0].title));
    TEST_ASSERT_EQUAL(WSEARCH_SNIPPET - 1, (int)strlen(g_pg.r[9].snippet));
    /* unterminated tag and empty input */
    TEST_ASSERT_EQUAL(0, wsearch_parse("<a class=\"result__a\" href=\"https://x/", 37, &g_pg));
    TEST_ASSERT_EQUAL(0, wsearch_parse("", 0, &g_pg));
    TEST_ASSERT_EQUAL(0, wsearch_parse(0, 10, &g_pg));
}

static void test_format_and_page(void) {
    int n = load("ddg_html_real.html");
    wsearch_parse(g_html, n, &g_pg);
    TEST_ASSERT_TRUE(wsearch_format(&g_pg, g_out, (int)sizeof(g_out)) > 100);
    TEST_ASSERT_TRUE(strncmp(g_out, "search 1 ", 9) == 0);
    TEST_ASSERT_TRUE(strstr(g_out, "\nsearch 10 ") != 0);
    TEST_ASSERT_TRUE(strstr(g_out, "\n  https://kernelos.org/\n") != 0);
    TEST_ASSERT_TRUE(wsearch_results_html(&g_pg, "a<b", "https://html.duckduckgo.com/html/?q=", g_out, (int)sizeof(g_out)) > 0);
    TEST_ASSERT_TRUE(strstr(g_out, "<title>Recherche : a&lt;b</title>") != 0);
    TEST_ASSERT_TRUE(strstr(g_out, "<a href=\"https://github.com/moodyhunter/MOS\">") != 0);
    TEST_ASSERT_TRUE(strstr(g_out, "10 resultats") != 0);
    memset(&g_pg, 0, sizeof(g_pg));
    g_pg.blocked = 1;
    wsearch_results_html(&g_pg, "q", "e", g_out, (int)sizeof(g_out));
    TEST_ASSERT_TRUE(strstr(g_out, "anti-robot") != 0);
    TEST_ASSERT_EQUAL(-1, wsearch_results_html(&g_pg, "q", "e", g_out, 100));
}

static void test_is_query(void) {
    TEST_ASSERT_TRUE(wsearch_is_query("chats et chiens"));
    TEST_ASSERT_TRUE(wsearch_is_query("mohhdy"));
    TEST_ASSERT_FALSE(wsearch_is_query("https://example.com"));
    TEST_ASSERT_FALSE(wsearch_is_query("/web/demo.html"));
    TEST_ASSERT_FALSE(wsearch_is_query("example.com"));
    TEST_ASSERT_TRUE(wsearch_is_query("what is example.com"));
    TEST_ASSERT_FALSE(wsearch_is_query("   "));
    TEST_ASSERT_FALSE(wsearch_is_query(0));
}

int main(void) {
    unity_init();
    RUN_TEST(test_urlencode);
    RUN_TEST(test_urldecode);
    RUN_TEST(test_build_url);
    RUN_TEST(test_decode_href);
    RUN_TEST(test_real_ddg_html);
    RUN_TEST(test_real_ddg_lite);
    RUN_TEST(test_bot_check_page);
    RUN_TEST(test_changed_markup_generic);
    RUN_TEST(test_bounds);
    RUN_TEST(test_format_and_page);
    RUN_TEST(test_is_query);
    unity_print_results();
    unity_cleanup();
    return unity_stats.tests_failed == 0 ? 0 : 1;
}
