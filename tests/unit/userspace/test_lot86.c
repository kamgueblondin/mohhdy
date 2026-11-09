/* Lot 86: history, bookmarks, settings, cookie expiry and persistence,
 * HTTP cache Age/Vary, and the reader tables. */
#include "../../framework/unity.h"
#include "wf_nav.h"
#include "desk_cfg.h"
#include "wf_store.h"
#include "wf_cookie.h"
#include "wf_cache.h"
#include "wf_blocks.h"
#include "wf_fetch.h"
#include "os_ui_scale.h"
#include <string.h>
#include <stdio.h>

static void test_nav_resolve_and_stack(void) {
    wf_nav_t n;
    char url[200], back[200], cur[200], saved[4000];
    int i;
    wf_nav_init(&n);
    TEST_ASSERT_EQUAL(0, wf_nav_resolve(0, "https://Example.COM:443/a/../b?q=1#frag", url, (int)sizeof(url)));
    TEST_ASSERT_EQUAL_STRING("https://example.com/b?q=1", url);
    TEST_ASSERT_EQUAL(0, wf_nav_resolve(url, "c", back, (int)sizeof(back)));
    TEST_ASSERT_EQUAL_STRING("https://example.com/c", back);
    TEST_ASSERT_EQUAL(0, wf_nav_resolve(url, "/z", back, (int)sizeof(back)));
    TEST_ASSERT_EQUAL_STRING("https://example.com/z", back);
    TEST_ASSERT_EQUAL(-1, wf_nav_resolve(0, "javascript:alert(1)", url, (int)sizeof(url)));
    TEST_ASSERT_EQUAL(-1, wf_nav_visit(&n, "javascript:x", "no", 0));
    TEST_ASSERT_EQUAL(0, wf_nav_visit(&n, "https://example.com/a", "  One  ", "20261109120000"));
    TEST_ASSERT_EQUAL(0, wf_nav_visit(&n, "https://example.com/a", "One", "20261109120000"));
    TEST_ASSERT_EQUAL(0, n.cursor);
    TEST_ASSERT_EQUAL(0, wf_nav_visit(&n, "https://example.com/b", "Two", 0));
    TEST_ASSERT_EQUAL(0, wf_nav_back(&n, back, (int)sizeof(back)));
    TEST_ASSERT_EQUAL_STRING("https://example.com/a", back);
    TEST_ASSERT_EQUAL(0, wf_nav_forward(&n, back, (int)sizeof(back)));
    TEST_ASSERT_EQUAL_STRING("https://example.com/b", back);
    TEST_ASSERT_EQUAL(-1, wf_nav_forward(&n, back, (int)sizeof(back)));
    TEST_ASSERT_EQUAL(0, wf_nav_mark_add(&n, "https://example.com/a", "One"));
    TEST_ASSERT_EQUAL(0, wf_nav_mark_add(&n, "https://example.com/a", "Renamed"));
    TEST_ASSERT_EQUAL(1, wf_nav_mark_count(&n));
    TEST_ASSERT_EQUAL_STRING("Renamed", wf_nav_mark_at(&n, 0)->title);
    TEST_ASSERT_EQUAL(0, wf_nav_mark_find(&n, "Renamed"));
    TEST_ASSERT_EQUAL(0, wf_nav_mark_del(&n, "https://example.com/a"));
    TEST_ASSERT_EQUAL(-1, wf_nav_mark_del(&n, "https://example.com/a"));
    TEST_ASSERT(wf_nav_hist_count(&n) >= 2);
    TEST_ASSERT(wf_nav_save(&n, saved, (int)sizeof(saved)) > 0);
    {
        wf_nav_t m;
        wf_nav_init(&m);
        TEST_ASSERT_EQUAL(0, wf_nav_load(&m, saved, (int)strlen(saved)));
        TEST_ASSERT_EQUAL(0, wf_nav_current(&m, cur, (int)sizeof(cur)));
        TEST_ASSERT_EQUAL_STRING("https://example.com/b", cur);
        TEST_ASSERT_EQUAL(wf_nav_hist_count(&n), wf_nav_hist_count(&m));
    }
    for (i = 0; i < WF_NAV_STACK + 4; i++) {
        char u[64];
        snprintf(u, sizeof(u), "https://example.com/p%d", i);
        TEST_ASSERT_EQUAL(0, wf_nav_visit(&n, u, "t", 0));
    }
    TEST_ASSERT(n.stack_n <= WF_NAV_STACK);
    TEST_ASSERT(wf_nav_text(&n, saved, 80) > 0);
    TEST_ASSERT(strstr(saved, "nav visits ") != 0);
}

static void test_desk_scale_net_engine(void) {
    desk_cfg_t c;
    char buf[400], again[400];
    desk_cfg_init(&c);
    TEST_ASSERT_EQUAL(6, c.scale_q);
    TEST_ASSERT_EQUAL(1, c.net_on);
    TEST_ASSERT_EQUAL_STRING("wikipedia", desk_cfg_engine_name(c.engine));
    TEST_ASSERT_EQUAL(-1, desk_cfg_set_scale(&c, 9));
    TEST_ASSERT_EQUAL(6, c.scale_q);
    TEST_ASSERT_EQUAL(0, desk_cfg_set_scale_word(&c, "2x"));
    TEST_ASSERT_EQUAL(8, c.scale_q);
    TEST_ASSERT_EQUAL(0, desk_cfg_step_scale(&c, 1));
    TEST_ASSERT_EQUAL(10, c.scale_q);
    TEST_ASSERT_EQUAL(0, desk_cfg_set_net(&c, 0));
    TEST_ASSERT_EQUAL(0, desk_cfg_set_engine(&c, "LOCAL"));
    TEST_ASSERT_EQUAL_STRING("local", desk_cfg_engine_name(c.engine));
    TEST_ASSERT_EQUAL(-1, desk_cfg_set_engine(&c, "not-an-engine"));
    TEST_ASSERT_EQUAL(1, desk_cfg_button(&c, 3));
    TEST_ASSERT_EQUAL(1, c.net_on);
    TEST_ASSERT(desk_cfg_save(&c, buf, (int)sizeof(buf)) > 0);
    {
        desk_cfg_t d;
        desk_cfg_init(&d);
        TEST_ASSERT_EQUAL(0, desk_cfg_load(&d, buf, (int)strlen(buf)));
        TEST_ASSERT_EQUAL(c.scale_q, d.scale_q);
        TEST_ASSERT_EQUAL(c.net_on, d.net_on);
        TEST_ASSERT_EQUAL(c.engine, d.engine);
        TEST_ASSERT_EQUAL(-1, desk_cfg_load(&d, "settings scale 9\n", 17));
        TEST_ASSERT_EQUAL(c.scale_q, d.scale_q);
    }
    TEST_ASSERT(desk_cfg_report(&c, again, (int)sizeof(again)) > 0);
    TEST_ASSERT(strstr(again, "reseau") != 0);
    TEST_ASSERT(strstr(again, "local") != 0);
    TEST_ASSERT_EQUAL(0, desk_cfg_chrome_audit(buf, (int)sizeof(buf)));
    TEST_ASSERT(strstr(buf, "ok") != 0);
    TEST_ASSERT_EQUAL(OS_UI_Q_CHROME_MAX, desk_cfg_chrome_q(16));
    TEST_ASSERT_EQUAL(8, desk_cfg_chrome_q(8));
    wf_fetch_set_offline(1);
    TEST_ASSERT_EQUAL(1, wf_fetch_offline());
    wf_fetch_set_offline(0);
    TEST_ASSERT_EQUAL(0, wf_fetch_offline());
}

static void test_cookie_expiry_and_store(void) {
    wf_cookie_jar_t j, k;
    char blob[4000];
    int n;
    wf_cookie_jar_init(&j);
    TEST_ASSERT_EQUAL(1, wf_cookie_set(&j, "example.com", "/", 1, "sid=abc; Max-Age=60; Secure", "20261109120000"));
    TEST_ASSERT_EQUAL(14, (int)strlen(j.c[0].expires));
    TEST_ASSERT_EQUAL(0, wf_cookie_expire(&j, "20261109120030"));
    TEST_ASSERT_EQUAL(1, wf_cookie_count(&j));
    TEST_ASSERT_EQUAL(1, wf_cookie_expire(&j, "20261109120130"));
    TEST_ASSERT_EQUAL(0, wf_cookie_count(&j));
    TEST_ASSERT_EQUAL(1, wf_cookie_set(&j, "example.com", "/app", 0, "a=b; Path=/app", "20261109120000"));
    n = wf_store_cookies_save(&j, blob, (int)sizeof(blob));
    TEST_ASSERT(n > 0);
    TEST_ASSERT(strstr(blob, "MOHHDY-COOKIES 1") == blob);
    wf_cookie_jar_init(&k);
    TEST_ASSERT_EQUAL(0, wf_store_cookies_load(&k, blob, n));
    TEST_ASSERT_EQUAL(1, wf_cookie_count(&k));
    TEST_ASSERT_EQUAL_STRING("a", k.c[0].name);
    TEST_ASSERT_EQUAL_STRING("/app", k.c[0].path);
    TEST_ASSERT_EQUAL(-1, wf_store_cookies_load(&k, "nope\n", 5));
    TEST_ASSERT_EQUAL(1, wf_cookie_count(&k));
}

static void test_cache_age_vary_and_store(void) {
    wf_cache_t c, d;
    wf_cache_meta_t m;
    int idx;
    char blob[8000];
    uint8_t body[32];
    int n;
    wf_cache_init(&c);
    {
        const char *h = "HTTP/1.1 200 OK\r\nCache-Control: max-age=60\r\nAge: 50\r\nETag: \"e\"\r\n\r\n";
        wf_cache_meta(h, (int)strlen(h), &m);
    }
    TEST_ASSERT_EQUAL(50, m.age);
    TEST_ASSERT_EQUAL(1, m.store);
    TEST_ASSERT(wf_cache_store(&c, "http://a/age", &m, "text/plain", (const uint8_t *)"Z", 1, 1000) >= 0);
    TEST_ASSERT_EQUAL(WF_CACHE_FRESH, wf_cache_lookup(&c, "http://a/age", 1009, &idx));
    TEST_ASSERT_EQUAL(WF_CACHE_STALE, wf_cache_lookup(&c, "http://a/age", 1010, &idx));
    {
        const char *h = "HTTP/1.1 200 OK\r\nCache-Control: max-age=30\r\nVary: *\r\nETag: \"e\"\r\n\r\n";
        wf_cache_meta(h, (int)strlen(h), &m);
    }
    TEST_ASSERT_EQUAL(0, m.store);
    {
        const char *h = "HTTP/1.1 200 OK\r\nCache-Control: max-age=30\r\nVary: Accept-Encoding\r\nETag: \"e\"\r\n\r\n";
        wf_cache_meta(h, (int)strlen(h), &m);
    }
    TEST_ASSERT_EQUAL(1, m.no_cache);
    TEST_ASSERT(strstr(m.vary, "Accept") != 0);
    memset(body, 0xAB, sizeof(body));
    wf_cache_init(&c);
    m.store = 1; m.no_cache = 0; m.max_age = 10; m.age = 0; m.etag[0] = 0; m.last_modified[0] = 0; m.vary[0] = 0;
    TEST_ASSERT(wf_cache_store(&c, "http://a/s", &m, "text/plain", body, 32, 5) >= 0);
    n = wf_store_cache_save(&c, blob, (int)sizeof(blob));
    TEST_ASSERT(n > 0);
    TEST_ASSERT(strstr(blob, "MOHHDY-CACHE 1") == blob);
    wf_cache_init(&d);
    TEST_ASSERT_EQUAL(0, wf_store_cache_load(&d, blob, n));
    TEST_ASSERT_EQUAL(WF_CACHE_FRESH, wf_cache_lookup(&d, "http://a/s", 10, &idx));
    TEST_ASSERT_EQUAL(32, wf_cache_body(&d, idx, body, 32));
    TEST_ASSERT_EQUAL(0xAB, body[0]);
}

static void test_blocks_table_dl_details(void) {
    const char *html =
        "<html><body><script>var x = '<table>';</script>"
        "<table><tr><th>Nom</th><th>Val</th></tr><tr><td>A &amp; B</td><td>1</td></tr></table>"
        "<dl><dt>Terme</dt><dd>Definition</dd></dl>"
        "<details><summary>Plus</summary><p>cache</p></details>"
        "<details open><summary>Ouvert</summary><p>visible</p></details>"
        "<figure><img src=\"a.png\"><figcaption>Legende</figcaption></figure>"
        "</body></html>";
    char out[1500];
    wf_blocks_stats_t st;
    int n = wf_blocks_text(html, (int)strlen(html), out, (int)sizeof(out), &st);
    TEST_ASSERT(n > 0);
    TEST_ASSERT_EQUAL(1, st.tables);
    TEST_ASSERT(st.rows >= 2);
    TEST_ASSERT(strstr(out, "A & B") != 0);
    TEST_ASSERT(strstr(out, "Nom | Val") != 0);
    TEST_ASSERT(strstr(out, "Terme: Definition") != 0);
    TEST_ASSERT(strstr(out, "details Plus") != 0);
    TEST_ASSERT(strstr(out, "cache") == 0);
    TEST_ASSERT(strstr(out, "details open Ouvert") != 0);
    TEST_ASSERT(strstr(out, "figure Legende") != 0);
    TEST_ASSERT_EQUAL(1, st.figures);
    TEST_ASSERT_EQUAL(2, st.details);
}

int main(void) {
    unity_init();
    RUN_TEST(test_nav_resolve_and_stack);
    RUN_TEST(test_desk_scale_net_engine);
    RUN_TEST(test_cookie_expiry_and_store);
    RUN_TEST(test_cache_age_vary_and_store);
    RUN_TEST(test_blocks_table_dl_details);
    unity_print_results();
    unity_cleanup();
    return unity_stats.tests_failed ? 1 : 0;
}
