/* Lot 86: tabs, autofill, and the compat reading of measured pages. */
#include "../../framework/unity.h"
#include "wf_tabs.h"
#include "wf_autofill.h"
#include "wf_compat.h"
#include "wf_feed.h"
#include "wf_robots.h"
#include <stdio.h>
#include <string.h>

static char g_buf[8192];

static int load(const char *name) {
    static const char *const pre[] = {
        "fixtures/compat/", "tests/fixtures/compat/", "../fixtures/compat/", "../../fixtures/compat/", 0
    };
    char path[256];
    int i;
    for (i = 0; pre[i]; i++) {
        FILE *f;
        int n;
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

static void test_tabs_open_close_roundtrip(void) {
    wf_tabs_t t, u;
    char text[2000];
    int idx = -1, i;
    wf_tabs_init(&t);
    TEST_ASSERT_EQUAL(-1, wf_tabs_open(&t, "javascript:x", "no", &idx));
    TEST_ASSERT_EQUAL(0, wf_tabs_open(&t, "https://example.com/a", "  Alpha  ", &idx));
    TEST_ASSERT_EQUAL(0, idx);
    TEST_ASSERT_EQUAL(0, wf_tabs_open(&t, "https://example.com/a", "Alpha", &idx));
    TEST_ASSERT_EQUAL(1, t.n);
    TEST_ASSERT_EQUAL(0, wf_tabs_open(&t, "https://example.com/b", "Beta", &idx));
    TEST_ASSERT_EQUAL(1, idx);
    TEST_ASSERT_EQUAL(0, wf_tabs_set_scroll(&t, 1, 12));
    TEST_ASSERT_EQUAL(12, wf_tabs_at(&t, 1)->scroll);
    TEST_ASSERT_EQUAL(0, wf_tabs_next(&t, -1));
    TEST_ASSERT_EQUAL(0, t.current);
    TEST_ASSERT_EQUAL(0, wf_tabs_close(&t, 0));
    TEST_ASSERT_EQUAL(1, t.current);
    TEST_ASSERT(wf_tabs_save(&t, text, (int)sizeof(text)) > 0);
    TEST_ASSERT_EQUAL(0, wf_tabs_load(&u, text, (int)strlen(text)));
    TEST_ASSERT_EQUAL(1, u.n);
    TEST_ASSERT_EQUAL_STRING("https://example.com/b", wf_tabs_at(&u, u.current)->url);
    for (i = 0; i < WF_TABS_MAX + 2; i++) {
        char url[64];
        int rc;
        snprintf(url, sizeof(url), "https://example.com/n%d", i);
        rc = wf_tabs_open(&t, url, "n", &idx);
        if (rc != 0) break;
    }
    TEST_ASSERT_EQUAL(WF_TABS_MAX, t.n);
    TEST_ASSERT_EQUAL(-1, wf_tabs_open(&t, "https://example.com/overflow", "x", &idx));
    TEST_ASSERT(wf_tabs_text(&t, text, (int)sizeof(text)) > 0);
    TEST_ASSERT(strstr(text, "tabs ") == text);
}

static void test_autofill_refuses_secrets(void) {
    wf_autofill_t a, b;
    char v[80], blob[2000];
    int n;
    wf_autofill_init(&a);
    TEST_ASSERT_EQUAL(1, wf_autofill_secret("password"));
    TEST_ASSERT_EQUAL(1, wf_autofill_secret("hidden"));
    TEST_ASSERT_EQUAL(0, wf_autofill_secret("email"));
    TEST_ASSERT_EQUAL(-1, wf_autofill_put(&a, "example.com", "pw", "secret", "password"));
    TEST_ASSERT_EQUAL(0, a.n);
    TEST_ASSERT_EQUAL(0, wf_autofill_put(&a, "Example.COM", "q", "mohhdy", "search"));
    TEST_ASSERT_EQUAL(0, wf_autofill_get(&a, "example.com", "q", v, (int)sizeof(v)));
    TEST_ASSERT_EQUAL_STRING("mohhdy", v);
    TEST_ASSERT_EQUAL(0, wf_autofill_put(&a, "example.com", "q", "again", "text"));
    TEST_ASSERT_EQUAL(1, a.n);
    TEST_ASSERT_EQUAL(-1, wf_autofill_put(&a, "localhost", "q", "x", "text"));
    n = wf_autofill_save(&a, blob, (int)sizeof(blob));
    TEST_ASSERT(n > 0);
    TEST_ASSERT_EQUAL(0, wf_autofill_load(&b, blob, n));
    TEST_ASSERT_EQUAL(0, wf_autofill_get(&b, "example.com", "q", v, (int)sizeof(v)));
    TEST_ASSERT_EQUAL_STRING("again", v);
    TEST_ASSERT_EQUAL(1, wf_autofill_forget(&b, "example.com"));
    TEST_ASSERT_EQUAL(0, b.n);
    TEST_ASSERT(wf_autofill_text(&a, blob, (int)sizeof(blob)) > 0);
}

static void test_measured_pages(void) {
    static const char *const files[] = {
        "google_20261109.html", "wiki_20261109.json", "example_20261109.html", "hn_20261109.html"
    };
    static const char *const hosts[] = {
        "www.google.com", "en.wikipedia.org", "example.com", "news.ycombinator.com"
    };
    int i;
    TEST_ASSERT_EQUAL(4, wf_compat_expect_count());
    for (i = 0; i < 4; i++) {
        wf_compat_t c;
        const wf_compat_expect_t *e = wf_compat_expect_at(i);
        char line[400];
        int n = load(files[i]);
        TEST_ASSERT(n > 0);
        wf_compat_scan(g_buf, n, &c);
        TEST_ASSERT(e != 0);
        TEST_ASSERT_EQUAL_STRING(hosts[i], e->host);
        TEST_ASSERT(c.bytes >= e->min_bytes);
        TEST_ASSERT_EQUAL(e->js_wall, c.js_wall);
        TEST_ASSERT_EQUAL_STRING(e->verdict, wf_compat_verdict(&c));
        TEST_ASSERT(wf_compat_line(e->host, &c, line, (int)sizeof(line)) > 0);
        TEST_ASSERT(strstr(line, e->verdict) != 0);
    }
    {
        wf_compat_t c;
        wf_compat_scan(0, 0, &c);
        TEST_ASSERT_EQUAL_STRING("empty", wf_compat_verdict(&c));
    }
}

static void test_robots(void) {
    wf_robots_t r;
    const char *txt =
        "User-agent: *\n"
        "Disallow: /private\n"
        "\n"
        "User-agent: mohhdy\n"
        "Disallow: /secret\n"
        "Allow: /secret/ok$\n"
        "Crawl-delay: 2\n";
    TEST_ASSERT_EQUAL(0, wf_robots_parse(&r, txt, (int)strlen(txt), "Mohhdy"));
    TEST_ASSERT_EQUAL(0, wf_robots_allowed(&r, "/secret"));
    TEST_ASSERT_EQUAL(1, wf_robots_allowed(&r, "/secret/ok"));
    TEST_ASSERT_EQUAL(1, wf_robots_allowed(&r, "/public"));
    TEST_ASSERT_EQUAL(2, r.crawl_delay);
    TEST_ASSERT_EQUAL(0, wf_robots_parse(&r, "User-agent: *\nDisallow: /x\n", 28, "other"));
    TEST_ASSERT_EQUAL(0, wf_robots_allowed(&r, "/x/y"));
    TEST_ASSERT_EQUAL(1, wf_robots_allowed(&r, "/y"));
    TEST_ASSERT_EQUAL(1, wf_robots_allowed(&r, ""));
}

static void test_rss_feed(void) {
    wf_feed_t f;
    char text[800];
    int n = load("sample_20261109.rss");
    TEST_ASSERT(n > 0);
    TEST_ASSERT_EQUAL(1, wf_feed_looks(g_buf, n));
    TEST_ASSERT_EQUAL(0, wf_feed_parse(g_buf, n, &f));
    TEST_ASSERT_EQUAL(0, f.atom);
    TEST_ASSERT_EQUAL_STRING("Mohhdy notes", f.title);
    TEST_ASSERT_EQUAL(2, f.n);
    TEST_ASSERT_EQUAL_STRING("First & only", f.item[0].title);
    TEST_ASSERT_EQUAL_STRING("https://example.com/a", f.item[0].link);
    TEST_ASSERT(strstr(f.item[0].blurb, "short") != 0);
    TEST_ASSERT(wf_feed_text(&f, text, (int)sizeof(text)) > 0);
    TEST_ASSERT(strstr(text, "rss Mohhdy notes") == text);
    TEST_ASSERT_EQUAL(0, wf_feed_looks("<html></html>", 13));
}

int main(void) {
    unity_init();
    RUN_TEST(test_tabs_open_close_roundtrip);
    RUN_TEST(test_autofill_refuses_secrets);
    RUN_TEST(test_measured_pages);
    RUN_TEST(test_rss_feed);
    RUN_TEST(test_robots);
    unity_print_results();
    unity_cleanup();
    return unity_stats.tests_failed ? 1 : 0;
}
