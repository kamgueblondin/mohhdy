/* Lot 73: the HTTP cache (wf_cache.c) and its use by wf_fetch: freshness
 * from max-age, no-store / no-cache / Pragma, ETag and Last-Modified sent
 * back, 304 refresh, 200 replace, LRU eviction in a bounded pool, and the
 * fetch path (fresh answer without a request, validators on the wire,
 * POST and redirects not cached). */
#include "../../framework/unity.h"
#include "wf_cache.h"
#include "wf_fetch.h"
#include <string.h>

static wf_cache_t g_c;
static wf_cache_meta_t g_m;
static uint8_t g_body[4096];
static char g_txt[256];

static void meta(const char *head) { wf_cache_meta(head, (int)strlen(head), &g_m); }

static void test_meta_policy(void) {
    meta("HTTP/1.1 200 OK\r\nCache-Control: max-age=60\r\nETag: \"v1\"\r\nLast-Modified: Mon, 02 Nov 2026 10:00:00 GMT\r\n\r\n");
    TEST_ASSERT_EQUAL(1, g_m.store);
    TEST_ASSERT_EQUAL(0, g_m.no_cache);
    TEST_ASSERT_EQUAL(60, g_m.max_age);
    TEST_ASSERT_EQUAL_STRING("\"v1\"", g_m.etag);
    TEST_ASSERT_EQUAL_STRING("Mon, 02 Nov 2026 10:00:00 GMT", g_m.last_modified);
    meta("HTTP/1.1 200 OK\r\ncache-control: public, NO-STORE, max-age=60\r\n\r\n");
    TEST_ASSERT_EQUAL(0, g_m.store);
    meta("HTTP/1.1 200 OK\r\nCache-Control: no-cache\r\nETag: W/\"x\"\r\n\r\n");
    TEST_ASSERT_EQUAL(1, g_m.store);
    TEST_ASSERT_EQUAL(1, g_m.no_cache);
    TEST_ASSERT_EQUAL_STRING("W/\"x\"", g_m.etag);
    meta("HTTP/1.1 200 OK\r\nCache-Control:max-age=\"30\" , must-revalidate\r\n\r\n");
    TEST_ASSERT_EQUAL(30, g_m.max_age);
    TEST_ASSERT_EQUAL(1, g_m.store);
    /* Pragma counts only without Cache-Control */
    meta("HTTP/1.1 200 OK\r\nPragma: no-cache\r\nETag: \"p\"\r\n\r\n");
    TEST_ASSERT_EQUAL(0, g_m.store);
    meta("HTTP/1.1 200 OK\r\nPragma: no-cache\r\nCache-Control: max-age=5\r\n\r\n");
    TEST_ASSERT_EQUAL(1, g_m.store);
    /* nothing to reuse */
    meta("HTTP/1.1 200 OK\r\nContent-Type: text/html\r\n\r\n");
    TEST_ASSERT_EQUAL(0, g_m.store);
    TEST_ASSERT_EQUAL(-1, g_m.max_age);
    meta("HTTP/1.1 200 OK\r\nCache-Control: max-age=0\r\n\r\n");
    TEST_ASSERT_EQUAL(0, g_m.store);
    meta("HTTP/1.1 200 OK\r\nCache-Control: max-age=x\r\nLast-Modified: d\r\n\r\n");
    TEST_ASSERT_EQUAL(-1, g_m.max_age);
    TEST_ASSERT_EQUAL(1, g_m.store);
    /* s-maxage is not max-age; header names exact */
    meta("HTTP/1.1 200 OK\r\nCache-Control: s-maxage=60\r\nX-Cache-Control: max-age=9\r\nETags: \"n\"\r\n\r\n");
    TEST_ASSERT_EQUAL(-1, g_m.max_age);
    TEST_ASSERT_EQUAL(0, g_m.store);
    /* bare LF lines, spaces around the value */
    meta("HTTP/1.1 200 OK\nETag:   \"lf\"  \n\n");
    TEST_ASSERT_EQUAL_STRING("\"lf\"", g_m.etag);
}

static void test_fresh_stale_and_validators(void) {
    int idx, n;
    wf_cache_init(&g_c);
    TEST_ASSERT_EQUAL(WF_CACHE_MISS, wf_cache_lookup(&g_c, "http://a/x", 0, &idx));
    TEST_ASSERT_EQUAL(-1, idx);
    meta("HTTP/1.1 200 OK\r\nCache-Control: max-age=60\r\nETag: \"v1\"\r\n\r\n");
    TEST_ASSERT_TRUE(wf_cache_store(&g_c, "http://a/x", &g_m, "text/html", (const uint8_t *)"hello", 5, 100) >= 0);
    TEST_ASSERT_EQUAL(WF_CACHE_FRESH, wf_cache_lookup(&g_c, "http://a/x", 159, &idx));
    TEST_ASSERT_EQUAL(5, wf_cache_body(&g_c, idx, g_body, (int)sizeof(g_body)));
    TEST_ASSERT_EQUAL(0, memcmp(g_body, "hello", 5));
    TEST_ASSERT_EQUAL(-1, wf_cache_body(&g_c, idx, g_body, 4));     /* does not fit */
    TEST_ASSERT_EQUAL_STRING("text/html", g_c.e[idx].ctype);
    TEST_ASSERT_EQUAL(WF_CACHE_STALE, wf_cache_lookup(&g_c, "http://a/x", 160, &idx));
    TEST_ASSERT_EQUAL(WF_CACHE_STALE, wf_cache_lookup(&g_c, "http://a/x", 99, &idx));    /* clock went back */
    n = wf_cache_validators(&g_c, idx, g_txt, (int)sizeof(g_txt));
    TEST_ASSERT_EQUAL_STRING("If-None-Match: \"v1\"\r\n", g_txt);
    TEST_ASSERT_EQUAL((int)strlen(g_txt), n);
    TEST_ASSERT_EQUAL(-1, wf_cache_validators(&g_c, idx, g_txt, 10));
    TEST_ASSERT_EQUAL(-1, wf_cache_validators(&g_c, idx, g_txt, 21));    /* the line fits, not its CRLF and end */
    TEST_ASSERT_EQUAL(21, wf_cache_validators(&g_c, idx, g_txt, 22));
    TEST_ASSERT_EQUAL(0, wf_cache_validators(&g_c, -1, g_txt, 10));
    /* 304: new lifetime and a new last-modified, the ETag kept */
    meta("HTTP/1.1 304 Not Modified\r\nCache-Control: max-age=10\r\nLast-Modified: L2\r\n\r\n");
    wf_cache_refresh(&g_c, idx, &g_m, 200);
    TEST_ASSERT_EQUAL(WF_CACHE_FRESH, wf_cache_lookup(&g_c, "http://a/x", 209, &idx));
    TEST_ASSERT_EQUAL(WF_CACHE_STALE, wf_cache_lookup(&g_c, "http://a/x", 210, &idx));
    wf_cache_validators(&g_c, idx, g_txt, (int)sizeof(g_txt));
    TEST_ASSERT_EQUAL_STRING("If-None-Match: \"v1\"\r\nIf-Modified-Since: L2\r\n", g_txt);
    /* no-cache: always revalidated */
    meta("HTTP/1.1 200 OK\r\nCache-Control: no-cache, max-age=600\r\nETag: \"n\"\r\n\r\n");
    wf_cache_store(&g_c, "http://a/nc", &g_m, "", (const uint8_t *)"z", 1, 0);
    TEST_ASSERT_EQUAL(WF_CACHE_STALE, wf_cache_lookup(&g_c, "http://a/nc", 1, &idx));
    wf_cache_text(&g_c, g_txt, (int)sizeof(g_txt));
    TEST_ASSERT_EQUAL_STRING("entries 2 bytes 6 hits 2 revalidated 1 misses 1 stored 2 evicted 0 refused 0", g_txt);
}

static void test_store_refusals_and_replace(void) {
    int idx;
    static uint8_t big[WF_CACHE_POOL / 4 + 1];
    char url[WF_CACHE_URL + 8];
    wf_cache_init(&g_c);
    meta("HTTP/1.1 200 OK\r\nCache-Control: max-age=60\r\n\r\n");
    TEST_ASSERT_EQUAL(-1, wf_cache_store(&g_c, "http://a/big", &g_m, "", big, (int)sizeof(big), 0));
    TEST_ASSERT_TRUE(wf_cache_store(&g_c, "http://a/q", &g_m, "", big, WF_CACHE_POOL / 4, 0) >= 0);   /* the limit itself */
    memset(url, 'u', sizeof(url)); url[sizeof(url) - 1] = 0;
    TEST_ASSERT_EQUAL(-1, wf_cache_store(&g_c, url, &g_m, "", big, 1, 0));
    url[WF_CACHE_URL] = 0;                                       /* 300 chars: no room for the end */
    TEST_ASSERT_EQUAL(-1, wf_cache_store(&g_c, url, &g_m, "", big, 1, 0));
    url[WF_CACHE_URL - 1] = 0;                                   /* 299 chars: fits */
    TEST_ASSERT_TRUE(wf_cache_store(&g_c, url, &g_m, "", big, 1, 0) >= 0);
    TEST_ASSERT_EQUAL(-1, wf_cache_store(&g_c, "http://a/n", &g_m, "", big, -1, 0));
    /* a replaced answer: the new body, the old bytes freed */
    wf_cache_store(&g_c, "http://a/r", &g_m, "", (const uint8_t *)"old", 3, 0);
    wf_cache_store(&g_c, "http://a/r", &g_m, "", (const uint8_t *)"newer", 5, 0);
    TEST_ASSERT_EQUAL(WF_CACHE_FRESH, wf_cache_lookup(&g_c, "http://a/r", 1, &idx));
    TEST_ASSERT_EQUAL(5, wf_cache_body(&g_c, idx, g_body, (int)sizeof(g_body)));
    TEST_ASSERT_EQUAL(0, memcmp(g_body, "newer", 5));
    TEST_ASSERT_EQUAL((uint32_t)(WF_CACHE_POOL / 4 + 1 + 5), g_c.pool_used);
    /* a no-store answer drops the older copy */
    meta("HTTP/1.1 200 OK\r\nCache-Control: no-store\r\n\r\n");
    TEST_ASSERT_EQUAL(-1, wf_cache_store(&g_c, "http://a/r", &g_m, "", (const uint8_t *)"x", 1, 0));
    TEST_ASSERT_EQUAL(WF_CACHE_MISS, wf_cache_lookup(&g_c, "http://a/r", 1, &idx));
    TEST_ASSERT_EQUAL(5, (int)g_c.refused);
    /* drop: bodies after it move down */
    meta("HTTP/1.1 200 OK\r\nCache-Control: max-age=60\r\n\r\n");
    wf_cache_init(&g_c);
    wf_cache_store(&g_c, "http://a/1", &g_m, "", (const uint8_t *)"AAA", 3, 0);
    wf_cache_store(&g_c, "http://a/2", &g_m, "", (const uint8_t *)"BB", 2, 0);
    TEST_ASSERT_EQUAL(1, wf_cache_drop(&g_c, "http://a/1"));
    TEST_ASSERT_EQUAL(0, wf_cache_drop(&g_c, "http://a/1"));
    TEST_ASSERT_EQUAL(WF_CACHE_FRESH, wf_cache_lookup(&g_c, "http://a/2", 1, &idx));
    TEST_ASSERT_EQUAL(2, wf_cache_body(&g_c, idx, g_body, (int)sizeof(g_body)));
    TEST_ASSERT_EQUAL(0, memcmp(g_body, "BB", 2));
    TEST_ASSERT_EQUAL(0u, g_c.e[idx].off);
    TEST_ASSERT_EQUAL(2u, g_c.pool_used);
}

static void test_lru_eviction(void) {
    static uint8_t chunk[WF_CACHE_POOL / 4];
    char url[32];
    int k, idx;
    wf_cache_init(&g_c);
    meta("HTTP/1.1 200 OK\r\nCache-Control: max-age=60\r\n\r\n");
    /* entry count: the 17th evicts the least recently used */
    for (k = 0; k < WF_CACHE_ENTRIES; k++) {
        url[0] = 'h'; url[1] = (char)('a' + k); url[2] = 0;
        TEST_ASSERT_TRUE(wf_cache_store(&g_c, url, &g_m, "", (const uint8_t *)"x", 1, 0) >= 0);
    }
    TEST_ASSERT_EQUAL(WF_CACHE_FRESH, wf_cache_lookup(&g_c, "ha", 1, &idx));     /* ha used: hb is the oldest */
    wf_cache_store(&g_c, "new", &g_m, "", (const uint8_t *)"y", 1, 0);
    TEST_ASSERT_EQUAL(WF_CACHE_FRESH, wf_cache_lookup(&g_c, "ha", 1, &idx));
    TEST_ASSERT_EQUAL(WF_CACHE_MISS, wf_cache_lookup(&g_c, "hb", 1, &idx));
    TEST_ASSERT_EQUAL(1, (int)g_c.evicted);
    /* bytes: four quarters fill the pool, a fifth evicts the oldest */
    wf_cache_init(&g_c);
    for (k = 0; k < 4; k++) {
        url[0] = 'q'; url[1] = (char)('0' + k); url[2] = 0;
        TEST_ASSERT_TRUE(wf_cache_store(&g_c, url, &g_m, "", chunk, (int)sizeof(chunk), 0) >= 0);
    }
    TEST_ASSERT_EQUAL((uint32_t)WF_CACHE_POOL, g_c.pool_used);
    TEST_ASSERT_EQUAL(0, (int)g_c.evicted);
    TEST_ASSERT_TRUE(wf_cache_store(&g_c, "q4", &g_m, "", chunk, 1, 0) >= 0);
    TEST_ASSERT_EQUAL(1, (int)g_c.evicted);
    TEST_ASSERT_EQUAL(WF_CACHE_MISS, wf_cache_lookup(&g_c, "q0", 1, &idx));
    TEST_ASSERT_EQUAL(WF_CACHE_FRESH, wf_cache_lookup(&g_c, "q1", 1, &idx));
    TEST_ASSERT_EQUAL((uint32_t)(3 * (WF_CACHE_POOL / 4) + 1), g_c.pool_used);
}

/* ---- wf_fetch with the cache: in-memory transport ---- */
static const char *g_replies[8];
static int g_reply_count, g_next, g_cur_off, g_connects;
static char g_last_req[1024];

static int m_resolve(void *n, const char *h, uint8_t ip[4]) { (void)n; (void)h; ip[0] = 10; ip[1] = 0; ip[2] = 2; ip[3] = 2; return 0; }
static int m_connect(void *n, const uint8_t ip[4], int port) {
    (void)n; (void)ip; (void)port;
    if (g_next >= g_reply_count) return -1;
    g_cur_off = 0; g_connects++;
    return g_next++;
}
static int m_send(void *n, int h, const uint8_t *d, int len) {
    (void)n; (void)h;
    if (len > (int)sizeof(g_last_req) - 1) len = (int)sizeof(g_last_req) - 1;
    memcpy(g_last_req, d, (size_t)len); g_last_req[len] = 0;
    return len;
}
static int m_recv(void *n, int h, uint8_t *d, int cap) {
    const char *s = g_replies[h];
    int left = (int)strlen(s) - g_cur_off, k;
    (void)n;
    if (left <= 0) return 0;
    k = left < cap ? left : cap;
    memcpy(d, s + g_cur_off, (size_t)k);
    g_cur_off += k;
    return k;
}
static void m_close(void *n, int h) { (void)n; (void)h; }
static int m_rand(void *n, uint8_t *o, int len) { (void)n; memset(o, 7, (size_t)len); return 0; }

static wf_fetch_env_t g_e;
static wf_fetch_result_t g_r;

static void env_init(void) {
    memset(&g_e, 0, sizeof(g_e));
    g_e.resolve = m_resolve; g_e.connect = m_connect; g_e.send = m_send; g_e.recv = m_recv;
    g_e.close = m_close; g_e.rand = m_rand; g_e.max_redirects = 5;
    g_e.cache = &g_c;
    g_next = 0; g_connects = 0; g_last_req[0] = 0;
    wf_cache_init(&g_c);
}

static void test_fetch_fresh_then_304(void) {
    env_init();
    g_replies[0] = "HTTP/1.1 200 OK\r\nContent-Type: text/plain\r\nCache-Control: max-age=30\r\nETag: \"e1\"\r\nContent-Length: 4\r\n\r\nbody";
    g_replies[1] = "HTTP/1.1 304 Not Modified\r\nCache-Control: max-age=30\r\n\r\n";
    g_replies[2] = "HTTP/1.1 200 OK\r\nContent-Type: text/html\r\nETag: \"e2\"\r\nContent-Length: 3\r\n\r\nnew";
    g_reply_count = 3;
    g_e.now_s = 1000;
    TEST_ASSERT_EQUAL(0, wf_fetch(&g_e, "http://site.test/p", g_body, sizeof(g_body), &g_r));
    TEST_ASSERT_EQUAL(200, g_r.status);
    TEST_ASSERT_EQUAL(0, g_r.cached);
    TEST_ASSERT_EQUAL(1, g_connects);
    TEST_ASSERT_NULL(strstr(g_last_req, "If-None-Match"));
    /* fresh: no request at all */
    memset(g_body, 0, 8);
    g_e.now_s = 1029;
    TEST_ASSERT_EQUAL(0, wf_fetch(&g_e, "http://site.test/p", g_body, sizeof(g_body), &g_r));
    TEST_ASSERT_EQUAL(1, g_r.cached);
    TEST_ASSERT_EQUAL(1, g_connects);
    TEST_ASSERT_EQUAL(200, g_r.status);
    TEST_ASSERT_EQUAL(4, g_r.body_len);
    TEST_ASSERT_EQUAL(0, memcmp(g_body, "body", 4));
    TEST_ASSERT_EQUAL_STRING("text/plain", g_r.content_type);
    TEST_ASSERT_EQUAL_STRING("http://site.test/p", g_r.url);
    /* stale: If-None-Match sent, 304 gives the stored body */
    memset(g_body, 0, 8);
    g_e.now_s = 1030;
    TEST_ASSERT_EQUAL(0, wf_fetch(&g_e, "http://site.test/p", g_body, sizeof(g_body), &g_r));
    TEST_ASSERT_EQUAL(2, g_connects);
    TEST_ASSERT_NOT_NULL(strstr(g_last_req, "\r\nIf-None-Match: \"e1\"\r\n"));
    TEST_ASSERT_EQUAL(2, g_r.cached);
    TEST_ASSERT_EQUAL(200, g_r.status);
    TEST_ASSERT_EQUAL(4, g_r.body_len);
    TEST_ASSERT_EQUAL(0, memcmp(g_body, "body", 4));
    TEST_ASSERT_EQUAL_STRING("text/plain", g_r.content_type);
    /* refreshed: fresh again for 30 s */
    g_e.now_s = 1059;
    TEST_ASSERT_EQUAL(0, wf_fetch(&g_e, "http://site.test/p", g_body, sizeof(g_body), &g_r));
    TEST_ASSERT_EQUAL(1, g_r.cached);
    TEST_ASSERT_EQUAL(2, g_connects);
    /* stale again, the server sends a new 200: replaced */
    g_e.now_s = 1060;
    TEST_ASSERT_EQUAL(0, wf_fetch(&g_e, "http://site.test/p", g_body, sizeof(g_body), &g_r));
    TEST_ASSERT_EQUAL(0, g_r.cached);
    TEST_ASSERT_EQUAL(3, g_r.body_len);
    {
        int idx;
        TEST_ASSERT_EQUAL(WF_CACHE_STALE, wf_cache_lookup(&g_c, "http://site.test/p", 1060, &idx));
        wf_cache_validators(&g_c, idx, g_txt, (int)sizeof(g_txt));
        TEST_ASSERT_EQUAL_STRING("If-None-Match: \"e2\"\r\n", g_txt);
        TEST_ASSERT_EQUAL_STRING("text/html", g_c.e[idx].ctype);
    }
}

static void test_fetch_not_cached_cases(void) {
    static const wf_fetch_req_t post = { "POST", "a=1", 3, 0, 0 };
    static const wf_fetch_req_t get_extra = { 0, 0, 0, 0, "X-Key: k\r\n" };
    int idx;
    env_init();
    /* a POST is never served from or stored in the cache */
    g_replies[0] = "HTTP/1.1 200 OK\r\nCache-Control: max-age=30\r\nContent-Length: 1\r\n\r\nP";
    g_replies[1] = "HTTP/1.1 200 OK\r\nCache-Control: max-age=30\r\nContent-Length: 1\r\n\r\nP";
    g_reply_count = 2;
    TEST_ASSERT_EQUAL(0, wf_fetch_ex(&g_e, "http://site.test/f", &post, g_body, sizeof(g_body), &g_r));
    TEST_ASSERT_EQUAL(WF_CACHE_MISS, wf_cache_lookup(&g_c, "http://site.test/f", 0, &idx));
    TEST_ASSERT_EQUAL(0, wf_fetch_ex(&g_e, "http://site.test/f", &post, g_body, sizeof(g_body), &g_r));
    TEST_ASSERT_EQUAL(2, g_connects);
    /* a redirect is not stored under the first URL */
    env_init();
    g_replies[0] = "HTTP/1.1 302 Found\r\nLocation: /b\r\nContent-Length: 0\r\n\r\n";
    g_replies[1] = "HTTP/1.1 200 OK\r\nCache-Control: max-age=30\r\nContent-Length: 1\r\n\r\nB";
    g_reply_count = 2;
    TEST_ASSERT_EQUAL(0, wf_fetch(&g_e, "http://site.test/a", g_body, sizeof(g_body), &g_r));
    TEST_ASSERT_EQUAL(WF_CACHE_MISS, wf_cache_lookup(&g_c, "http://site.test/a", 0, &idx));
    TEST_ASSERT_EQUAL(WF_CACHE_MISS, wf_cache_lookup(&g_c, "http://site.test/b", 0, &idx));
    /* a stale entry redirected: the validators are not sent to the new URL */
    env_init();
    g_replies[0] = "HTTP/1.1 200 OK\r\nETag: \"r\"\r\nContent-Length: 1\r\n\r\nR";
    g_replies[1] = "HTTP/1.1 302 Found\r\nLocation: /moved\r\nContent-Length: 0\r\n\r\n";
    g_replies[2] = "HTTP/1.1 200 OK\r\nContent-Length: 1\r\n\r\nM";
    g_reply_count = 3;
    TEST_ASSERT_EQUAL(0, wf_fetch(&g_e, "http://site.test/r", g_body, sizeof(g_body), &g_r));
    TEST_ASSERT_EQUAL(0, wf_fetch(&g_e, "http://site.test/r", g_body, sizeof(g_body), &g_r));
    TEST_ASSERT_EQUAL(1, g_r.redirects);
    TEST_ASSERT_TRUE(strncmp(g_last_req, "GET /moved ", 11) == 0);
    TEST_ASSERT_NULL(strstr(g_last_req, "If-None-Match"));
    /* a GET with extra lines: cached, the validators come after them */
    env_init();
    g_replies[0] = "HTTP/1.1 200 OK\r\nETag: \"k\"\r\nContent-Length: 1\r\n\r\nK";
    g_replies[1] = "HTTP/1.1 304 Not Modified\r\n\r\n";
    g_reply_count = 2;
    TEST_ASSERT_EQUAL(0, wf_fetch_ex(&g_e, "http://site.test/k", &get_extra, g_body, sizeof(g_body), &g_r));
    TEST_ASSERT_EQUAL(0, wf_fetch_ex(&g_e, "http://site.test/k", &get_extra, g_body, sizeof(g_body), &g_r));
    TEST_ASSERT_NOT_NULL(strstr(g_last_req, "X-Key: k\r\nIf-None-Match: \"k\"\r\n"));
    TEST_ASSERT_EQUAL(2, g_r.cached);
    TEST_ASSERT_EQUAL('K', g_body[0]);
    /* an error status drops the stored copy; a 304 without an entry passes through */
    env_init();
    g_replies[0] = "HTTP/1.1 200 OK\r\nETag: \"g\"\r\nContent-Length: 1\r\n\r\nG";
    g_replies[1] = "HTTP/1.1 404 Not Found\r\nContent-Length: 0\r\n\r\n";
    g_replies[2] = "HTTP/1.1 304 Not Modified\r\n\r\n";
    g_reply_count = 3;
    TEST_ASSERT_EQUAL(0, wf_fetch(&g_e, "http://site.test/g", g_body, sizeof(g_body), &g_r));
    TEST_ASSERT_EQUAL(0, wf_fetch(&g_e, "http://site.test/g", g_body, sizeof(g_body), &g_r));
    TEST_ASSERT_EQUAL(404, g_r.status);
    TEST_ASSERT_EQUAL(WF_CACHE_MISS, wf_cache_lookup(&g_c, "http://site.test/g", 0, &idx));
    TEST_ASSERT_EQUAL(0, wf_fetch(&g_e, "http://site.test/g", g_body, sizeof(g_body), &g_r));
    TEST_ASSERT_EQUAL(304, g_r.status);
    TEST_ASSERT_EQUAL(0, g_r.cached);
    TEST_ASSERT_NULL(strstr(g_last_req, "If-None-Match"));
    /* no cache in the env: never used */
    env_init();
    g_e.cache = 0;
    g_replies[0] = "HTTP/1.1 200 OK\r\nCache-Control: max-age=30\r\nContent-Length: 1\r\n\r\nN";
    g_replies[1] = "HTTP/1.1 200 OK\r\nCache-Control: max-age=30\r\nContent-Length: 1\r\n\r\nN";
    g_reply_count = 2;
    TEST_ASSERT_EQUAL(0, wf_fetch(&g_e, "http://site.test/n", g_body, sizeof(g_body), &g_r));
    TEST_ASSERT_EQUAL(0, wf_fetch(&g_e, "http://site.test/n", g_body, sizeof(g_body), &g_r));
    TEST_ASSERT_EQUAL(2, g_connects);
    /* a truncated body is not stored */
    env_init();
    g_replies[0] = "HTTP/1.1 200 OK\r\nCache-Control: max-age=30\r\nContent-Length: 10\r\n\r\n0123456789";
    g_reply_count = 1;
    TEST_ASSERT_EQUAL(0, wf_fetch(&g_e, "http://site.test/t", g_body, 4, &g_r));
    TEST_ASSERT_EQUAL(1, g_r.truncated);
    TEST_ASSERT_EQUAL(WF_CACHE_MISS, wf_cache_lookup(&g_c, "http://site.test/t", 0, &idx));
    /* a fresh entry too large for the caller buffer is fetched again */
    env_init();
    g_replies[0] = "HTTP/1.1 200 OK\r\nCache-Control: max-age=30\r\nContent-Length: 6\r\n\r\nabcdef";
    g_replies[1] = "HTTP/1.1 200 OK\r\nCache-Control: max-age=30\r\nContent-Length: 6\r\n\r\nabcdef";
    g_reply_count = 2;
    TEST_ASSERT_EQUAL(0, wf_fetch(&g_e, "http://site.test/s", g_body, sizeof(g_body), &g_r));
    TEST_ASSERT_EQUAL(0, wf_fetch(&g_e, "http://site.test/s", g_body, 4, &g_r));
    TEST_ASSERT_EQUAL(2, g_connects);
    TEST_ASSERT_EQUAL(0, g_r.cached);
}

int main(void) {
    unity_init();
    RUN_TEST(test_meta_policy);
    RUN_TEST(test_fresh_stale_and_validators);
    RUN_TEST(test_store_refusals_and_replace);
    RUN_TEST(test_lru_eviction);
    RUN_TEST(test_fetch_fresh_then_304);
    RUN_TEST(test_fetch_not_cached_cases);
    unity_print_results();
    unity_cleanup();
    return (unity_stats.tests_failed == 0) ? 0 : 1;
}
