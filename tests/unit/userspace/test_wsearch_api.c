/* Lot 71: search engines usable from the real guest (userspace/wsearch_api.c):
 * configuration (/persist/search.cfg format, key masking), request URLs and
 * headers, SearXNG HTML and JSON, Brave API JSON, Google CSE JSON, the
 * Google basic HTML results page and its bot check, JSON string decoding.
 * Fixtures in tests/fixtures/search are synthetic pages that follow the
 * markup of each engine (no secret, no saved personal data). */
#include "../../framework/unity.h"
#include "../../../userspace/wsearch_api.h"
#include <stdio.h>
#include <string.h>

static char g_buf[65536];
static wsearch_page_t g_pg;

static int load(const char *name) {
    static const char *const pre[] = { "fixtures/search/", "tests/fixtures/search/", "../fixtures/search/", "../../fixtures/search/", 0 };
    char path[256];
    int i, n;
    for (i = 0; pre[i]; i++) {
        FILE *f;
        snprintf(path, sizeof(path), "%s%s", pre[i], name);
        f = fopen(path, "rb");
        if (!f) continue;
        n = (int)fread(g_buf, 1, sizeof(g_buf) - 1, f);
        fclose(f);
        g_buf[n] = 0;
        return n;
    }
    return -1;
}

static void test_cfg(void) {
    wsearch_cfg_t c;
    char o[512], m[64];
    wsearch_cfg_default(&c);
    TEST_ASSERT_EQUAL_STRING("ddg", c.engine);
    TEST_ASSERT_EQUAL(4, wsearch_cfg_parse(&c, "# config\nengine = searx\r\nsearx=https://searx.example.org/\nbad line\nsearx_json=1\nbrave_key=BSAabcdefghijklmnop\nunknown=1\n", 120));
    TEST_ASSERT_EQUAL_STRING("searx", c.engine);
    TEST_ASSERT_EQUAL_STRING("https://searx.example.org", c.searx);
    TEST_ASSERT_EQUAL(1, c.searx_json);
    TEST_ASSERT_EQUAL(-1, wsearch_cfg_set(&c, "engine", "bing"));
    TEST_ASSERT_EQUAL(-1, wsearch_cfg_set(&c, "searx", "ftp://x"));
    TEST_ASSERT_EQUAL(-1, wsearch_cfg_set(&c, "brave_key", "a b"));
    TEST_ASSERT_EQUAL(-1, wsearch_cfg_set(&c, "google_key", "a\r\nX: y"));
    TEST_ASSERT_EQUAL(0, wsearch_cfg_set(&c, "google_key", "AIzaSyExampleKey123"));
    TEST_ASSERT_EQUAL(0, wsearch_cfg_set(&c, "google_cx", "0123456789abc"));
    wsearch_cfg_format(&c, o, (int)sizeof(o));
    TEST_ASSERT_NOT_NULL(strstr(o, "engine=searx\n"));
    TEST_ASSERT_NOT_NULL(strstr(o, "brave_key=BSAabcdefghijklmnop\n"));
    {
        wsearch_cfg_t d;
        wsearch_cfg_default(&d);
        wsearch_cfg_parse(&d, o, (int)strlen(o));
        TEST_ASSERT_EQUAL_MEMORY(&c, &d, sizeof(c));
    }
    wsearch_key_mask(c.brave_key, m, (int)sizeof(m));
    TEST_ASSERT_EQUAL_STRING("BSAa...mnop (19 chars)", m);
    TEST_ASSERT_NULL(strstr(m, "defghijkl"));
    wsearch_key_mask("abc", m, (int)sizeof(m));
    TEST_ASSERT_EQUAL_STRING("*** (3 chars)", m);
    wsearch_key_mask("", m, (int)sizeof(m));
    TEST_ASSERT_EQUAL_STRING("(none)", m);
}

static void test_requests(void) {
    wsearch_cfg_t c;
    char url[512], extra[256];
    wsearch_cfg_default(&c);
    TEST_ASSERT_EQUAL(WSEARCH_KIND_SEARX, wsearch_kind("searx"));
    TEST_ASSERT_EQUAL(WSEARCH_KIND_BRAVE, wsearch_kind("brave-api"));
    TEST_ASSERT_EQUAL(WSEARCH_KIND_GOOGLE, wsearch_kind("google"));
    TEST_ASSERT_EQUAL(WSEARCH_KIND_DDG, wsearch_kind("ddg-lite"));
    TEST_ASSERT_EQUAL(WSEARCH_KIND_URL, wsearch_kind("http://10.0.2.2:8080/s?q=%s"));
    TEST_ASSERT_EQUAL(-1, wsearch_kind("bing"));
    TEST_ASSERT_EQUAL(-1, wsearch_api_request(&c, WSEARCH_KIND_SEARX, "x", url, (int)sizeof(url), extra, (int)sizeof(extra)));
    TEST_ASSERT_EQUAL(-1, wsearch_api_request(&c, WSEARCH_KIND_BRAVE, "x", url, (int)sizeof(url), extra, (int)sizeof(extra)));
    TEST_ASSERT_EQUAL(-1, wsearch_api_request(&c, WSEARCH_KIND_GOOGLE_API, "x", url, (int)sizeof(url), extra, (int)sizeof(extra)));
    wsearch_cfg_set(&c, "searx", "https://s.example");
    TEST_ASSERT_EQUAL(0, wsearch_api_request(&c, WSEARCH_KIND_SEARX, "caf\xc3\xa9 os", url, (int)sizeof(url), extra, (int)sizeof(extra)));
    TEST_ASSERT_EQUAL_STRING("https://s.example/search?q=caf%C3%A9+os&categories=general&language=fr", url);
    wsearch_cfg_set(&c, "searx_json", "1");
    wsearch_api_request(&c, WSEARCH_KIND_SEARX, "a", url, (int)sizeof(url), extra, (int)sizeof(extra));
    TEST_ASSERT_EQUAL_STRING("https://s.example/search?q=a&format=json&language=fr", url);
    wsearch_cfg_set(&c, "brave_key", "KEY123");
    TEST_ASSERT_EQUAL(0, wsearch_api_request(&c, WSEARCH_KIND_BRAVE, "a b", url, (int)sizeof(url), extra, (int)sizeof(extra)));
    TEST_ASSERT_EQUAL_STRING("https://api.search.brave.com/res/v1/web/search?q=a+b&count=10", url);
    TEST_ASSERT_EQUAL_STRING("Accept: application/json\r\nX-Subscription-Token: KEY123\r\n", extra);
    wsearch_cfg_set(&c, "google_key", "K");
    wsearch_cfg_set(&c, "google_cx", "CX");
    TEST_ASSERT_EQUAL(0, wsearch_api_request(&c, WSEARCH_KIND_GOOGLE_API, "q", url, (int)sizeof(url), extra, (int)sizeof(extra)));
    TEST_ASSERT_EQUAL_STRING("https://www.googleapis.com/customsearch/v1?key=K&cx=CX&q=q", url);
    TEST_ASSERT_EQUAL(0, wsearch_api_request(&c, WSEARCH_KIND_GOOGLE, "mohhdy", url, (int)sizeof(url), extra, (int)sizeof(extra)));
    TEST_ASSERT_EQUAL_STRING("https://www.google.com/search?q=mohhdy&hl=fr&gbv=1&num=10", url);
    TEST_ASSERT_EQUAL(0, wsearch_api_request(&c, WSEARCH_KIND_DDG, "x", url, (int)sizeof(url), extra, (int)sizeof(extra)));
    TEST_ASSERT_NOT_NULL(strstr(url, "duckduckgo.com"));
    TEST_ASSERT_EQUAL(-1, wsearch_api_request(&c, WSEARCH_KIND_GOOGLE, "", url, (int)sizeof(url), extra, (int)sizeof(extra)));
    TEST_ASSERT_EQUAL(-1, wsearch_api_request(&c, WSEARCH_KIND_GOOGLE, "x", url, 20, extra, (int)sizeof(extra)));
}

static void test_json_string(void) {
    char o[64];
    TEST_ASSERT_EQUAL(14, wsearch_json_string("\"a\\\"b\\u00e9\\n\" rest", 20, o, (int)sizeof(o)));
    TEST_ASSERT_EQUAL_STRING("a\"b\xc3\xa9\n", o);
    wsearch_json_string("\"\\ud83d\\ude00\"", 14, o, (int)sizeof(o));
    TEST_ASSERT_EQUAL_STRING("\xf0\x9f\x98\x80", o);
    wsearch_json_string("\"\\ud83d x\"", 10, o, (int)sizeof(o));
    TEST_ASSERT_EQUAL_STRING("\xef\xbf\xbd x", o);
    TEST_ASSERT_EQUAL(-1, wsearch_json_string("\"open", 5, o, (int)sizeof(o)));
    TEST_ASSERT_EQUAL(-1, wsearch_json_string("\"\\u12\"", 6, o, (int)sizeof(o)));
}

static void test_searx_html(void) {
    int n = load("searx_results.html");
    TEST_ASSERT_TRUE(n > 0);
    TEST_ASSERT_EQUAL(2, wsearch_api_parse(WSEARCH_KIND_SEARX, g_buf, n, &g_pg));
    TEST_ASSERT_EQUAL(WSEARCH_LAYOUT_SEARX, g_pg.layout);
    TEST_ASSERT_EQUAL_STRING("https://github.com/kamgueblondin/mohhdy", g_pg.r[0].url);
    TEST_ASSERT_EQUAL_STRING("Mohhdy OS \xe2\x80\x94 syst\xc3\xa8me i386", g_pg.r[0].title);
    TEST_ASSERT_EQUAL_STRING("Un OS freestanding en C, d\xc3\xa9marr\xc3\xa9 par Multiboot.", g_pg.r[0].snippet);
    TEST_ASSERT_EQUAL_STRING("Syst\xc3\xa8me d'exploitation \xe2\x80\x94 Wikip\xc3\xa9" "dia", g_pg.r[1].title);
    TEST_ASSERT_TRUE(g_pg.skipped >= 2);   /* javascript: and the duplicate */
}

static void test_searx_json(void) {
    int n = load("searx.json");
    TEST_ASSERT_EQUAL(3, wsearch_api_parse(WSEARCH_KIND_SEARX, g_buf, n, &g_pg));
    TEST_ASSERT_EQUAL(WSEARCH_LAYOUT_JSON, g_pg.layout);
    TEST_ASSERT_EQUAL_STRING("https://example.org/a", g_pg.r[0].url);
    TEST_ASSERT_EQUAL_STRING("R\xc3\xa9sultat \"un\"", g_pg.r[0].title);
    TEST_ASSERT_EQUAL_STRING("Texte \xc3\xa0 lire gras", g_pg.r[0].snippet);
    TEST_ASSERT_EQUAL_STRING("Emoji \xf0\x9f\x98\x80 et \xc5\x93uvre", g_pg.r[1].title);
    TEST_ASSERT_EQUAL_STRING("https://example.org/info", g_pg.r[2].url);
}

static void test_brave_json(void) {
    int n = load("brave.json");
    TEST_ASSERT_EQUAL(2, wsearch_api_parse(WSEARCH_KIND_BRAVE, g_buf, n, &g_pg));
    TEST_ASSERT_EQUAL_STRING("Mohhdy OS on GitHub", g_pg.r[0].title);
    TEST_ASSERT_EQUAL_STRING("https://github.com/kamgueblondin/mohhdy", g_pg.r[0].url);
    TEST_ASSERT_EQUAL_STRING("A freestanding i386 OS & more.", g_pg.r[0].snippet);
    TEST_ASSERT_EQUAL_STRING("Deuxi\xc3\xa8me", g_pg.r[1].snippet);
}

static void test_google_cse_json(void) {
    int n = load("google_cse.json");
    TEST_ASSERT_EQUAL(2, wsearch_api_parse(WSEARCH_KIND_GOOGLE_API, g_buf, n, &g_pg));
    TEST_ASSERT_EQUAL_STRING("https://docs.example.com/mohhdy", g_pg.r[0].url);
    TEST_ASSERT_EQUAL_STRING("Mohhdy - Documentation", g_pg.r[0].title);
    TEST_ASSERT_EQUAL_STRING("La documentation du syst\xc3\xa8me Mohhdy.", g_pg.r[0].snippet);
}

static void test_google_html(void) {
    int n = load("google_basic.html");
    TEST_ASSERT_EQUAL(2, wsearch_api_parse(WSEARCH_KIND_GOOGLE, g_buf, n, &g_pg));
    TEST_ASSERT_EQUAL(WSEARCH_LAYOUT_GOOGLE, g_pg.layout);
    TEST_ASSERT_EQUAL_STRING("https://github.com/kamgueblondin/mohhdy", g_pg.r[0].url);
    TEST_ASSERT_NOT_NULL(strstr(g_pg.r[0].title, "kamgueblondin/mohhdy: Mohhdy OS - GitHub"));
    TEST_ASSERT_NOT_NULL(strstr(g_pg.r[0].snippet, "Syst\xc3\xa8me d'exploitation freestanding"));
    TEST_ASSERT_EQUAL_STRING("https://fr.wikipedia.org/wiki/Syst%C3%A8me", g_pg.r[1].url);
    TEST_ASSERT_TRUE(g_pg.skipped >= 2);   /* maps.google.com, accounts.google.com */
    TEST_ASSERT_FALSE(g_pg.blocked);
    n = load("google_sorry.html");
    TEST_ASSERT_EQUAL(0, wsearch_api_parse(WSEARCH_KIND_GOOGLE, g_buf, n, &g_pg));
    TEST_ASSERT_TRUE(g_pg.blocked);
    /* Lot 71: the page Google gives clients without JavaScript */
    n = load("google_enablejs.html");
    TEST_ASSERT_EQUAL(0, wsearch_api_parse(WSEARCH_KIND_GOOGLE, g_buf, n, &g_pg));
    TEST_ASSERT_EQUAL(2, g_pg.blocked);
}

static void test_json_bounds(void) {
    static char deep[4096];
    int i, n = 0;
    for (i = 0; i < 40; i++) deep[n++] = '[';
    deep[n] = 0;
    TEST_ASSERT_EQUAL(0, wsearch_parse_json(deep, n, &g_pg));
    n = sprintf(deep, "{\"results\":[");
    for (i = 0; i < 30; i++) n += sprintf(deep + n, "%s{\"url\":\"https://e.org/%d\",\"title\":\"t%d\"}", i ? "," : "", i, i);
    n += sprintf(deep + n, "]}");
    TEST_ASSERT_EQUAL(WSEARCH_MAX, wsearch_parse_json(deep, n, &g_pg));
    TEST_ASSERT_EQUAL_STRING("https://e.org/9", g_pg.r[9].url);
    TEST_ASSERT_EQUAL(0, wsearch_parse_json("{\"url\":\"https://x\",\"title\":\"unterminated", 40, &g_pg));
}

static void test_html_text(void) {
    char o[64];
    wsearch_html_text("<b>caf&eacute;</b>  &amp;\n<i>th\xc3\xa9</i> ", 37, o, (int)sizeof(o));
    TEST_ASSERT_EQUAL_STRING("caf\xc3\xa9 & th\xc3\xa9", o);
}

/* Lot 71: Wikipedia full text search (no key, no JavaScript) */
static void test_wiki(void) {
    static wsearch_cfg_t c;
    static char url[600], extra[64];
    int n;
    wsearch_cfg_default(&c);
    TEST_ASSERT_EQUAL(WSEARCH_KIND_WIKI, wsearch_kind("wikipedia"));
    TEST_ASSERT_EQUAL(0, wsearch_api_request(&c, WSEARCH_KIND_WIKI, "syst\xc3\xa8me os", url, (int)sizeof(url), extra, (int)sizeof(extra)));
    TEST_ASSERT_EQUAL_STRING("https://fr.wikipedia.org/w/api.php?action=query&list=search&format=json&utf8=1&srlimit=10&srsearch=syst%C3%A8me+os", url);
    TEST_ASSERT_EQUAL(-1, wsearch_cfg_set(&c, "wiki_lang", "F\""));
    TEST_ASSERT_EQUAL(0, wsearch_cfg_set(&c, "wiki_lang", "en"));
    n = load("wiki_search.json");
    TEST_ASSERT_EQUAL(0, wsearch_api_request(&c, WSEARCH_KIND_WIKI, "os", url, (int)sizeof(url), extra, (int)sizeof(extra)));
    TEST_ASSERT_EQUAL(2, wsearch_api_parse(WSEARCH_KIND_WIKI, g_buf, n, &g_pg));
    TEST_ASSERT_EQUAL(WSEARCH_LAYOUT_WIKI, g_pg.layout);
    TEST_ASSERT_EQUAL_STRING("Syst\xc3\xa8me d'exploitation", g_pg.r[0].title);
    TEST_ASSERT_EQUAL_STRING("https://en.wikipedia.org/wiki/Syst%C3%A8me_d%27exploitation", g_pg.r[0].url);
    TEST_ASSERT_NOT_NULL(strstr(g_pg.r[0].snippet, "syst\xc3\xa8me d'exploitation (OS)"));
    TEST_ASSERT_EQUAL_STRING("https://en.wikipedia.org/wiki/Mohhdy_(noyau)", g_pg.r[1].url);
    TEST_ASSERT_EQUAL_STRING("noyau i386 & Ring 3", g_pg.r[1].snippet);
}


/* Lot 72: the page Google gives gbv=1 clients today (measured 2026-11-01) */
static void test_google_update_browser(void) {
    int n = load("google_update_browser.html");
    TEST_ASSERT_EQUAL(0, wsearch_api_parse(WSEARCH_KIND_GOOGLE, g_buf, n, &g_pg));
    TEST_ASSERT_EQUAL(2, g_pg.blocked);
    /* the real result page is still parsed when Google sends one */
    n = load("google_basic.html");
    TEST_ASSERT_EQUAL(2, wsearch_api_parse(WSEARCH_KIND_GOOGLE, g_buf, n, &g_pg));
    TEST_ASSERT_EQUAL(0, g_pg.blocked);
}

static void test_google_form_target(void) {
    char q[64];
    TEST_ASSERT_EQUAL(1, wsearch_google_form("https://www.google.com/search?ie=ISO-8859-1&hl=fr&source=hp&q=mohhdy+os&btnG=Recherche", q, (int)sizeof(q)));
    TEST_ASSERT_EQUAL_STRING("mohhdy os", q);
    TEST_ASSERT_EQUAL(1, wsearch_google_form("https://google.fr/search?q=caf%C3%A9", q, (int)sizeof(q)));
    TEST_ASSERT_EQUAL_STRING("caf\xc3\xa9", q);
    TEST_ASSERT_EQUAL(1, wsearch_google_form("http://www.google.co.uk/webhp?hl=en&q=x#frag", q, (int)sizeof(q)));
    TEST_ASSERT_EQUAL_STRING("x", q);
    TEST_ASSERT_EQUAL(0, wsearch_google_form("https://www.google.com/search?hl=fr", q, (int)sizeof(q)));
    TEST_ASSERT_EQUAL(0, wsearch_google_form("https://www.google.com/search?q=", q, (int)sizeof(q)));
    TEST_ASSERT_EQUAL(0, wsearch_google_form("https://evilgoogle.com/search?q=x", q, (int)sizeof(q)));
    TEST_ASSERT_EQUAL(0, wsearch_google_form("https://www.google.com.evil.org/search?q=x", q, (int)sizeof(q)));
    TEST_ASSERT_EQUAL(0, wsearch_google_form("https://www.google.com/maps?q=x", q, (int)sizeof(q)));
    TEST_ASSERT_EQUAL(0, wsearch_google_form("https://duckduckgo.com/search?q=x", q, (int)sizeof(q)));
    TEST_ASSERT_EQUAL(0, wsearch_google_form("ftp://www.google.com/search?q=x", q, (int)sizeof(q)));
    TEST_ASSERT_EQUAL(1, wsearch_google_form("https://www.google.com:443/search?q=a&qq=b", q, (int)sizeof(q)));
    TEST_ASSERT_EQUAL_STRING("a", q);
}

static void test_results_note(void) {
    static char o[4096];
    memset(&g_pg, 0, sizeof(g_pg));
    TEST_ASSERT_TRUE(wsearch_results_html_note(&g_pg, "q", "https://e/x", "Google <exige> JavaScript", o, (int)sizeof(o)) > 0);
    TEST_ASSERT_NOT_NULL(strstr(o, "<p class=m>Google &lt;exige&gt; JavaScript</p>"));
    TEST_ASSERT_TRUE(wsearch_results_html(&g_pg, "q", "https://e/x", o, (int)sizeof(o)) > 0);
    TEST_ASSERT_NULL(strstr(o, "exige"));
}

int main(void) {
    unity_init();
    RUN_TEST(test_cfg);
    RUN_TEST(test_requests);
    RUN_TEST(test_json_string);
    RUN_TEST(test_searx_html);
    RUN_TEST(test_searx_json);
    RUN_TEST(test_brave_json);
    RUN_TEST(test_google_cse_json);
    RUN_TEST(test_google_html);
    RUN_TEST(test_google_update_browser);
    RUN_TEST(test_google_form_target);
    RUN_TEST(test_results_note);
    RUN_TEST(test_json_bounds);
    RUN_TEST(test_html_text);
    RUN_TEST(test_wiki);
    unity_print_results();
    unity_cleanup();
    return unity_stats.tests_failed == 0 ? 0 : 1;
}
