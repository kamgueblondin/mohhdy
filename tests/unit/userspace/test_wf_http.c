/* test_wf_http.c - Lot 67: URL, HTTP/1.1 response parser (Content-Length,
 * chunked, close-delimited), DNS codec, TLS PRF known answer, and wf_fetch
 * redirects over an in-memory transport. */
#include "../../framework/unity.h"
#include <string.h>
#include "wf_http.h"
#include "wf_dns.h"
#include "wf_tls.h"
#include "wf_fetch.h"

static void test_url_parse(void) {
    wf_url_t u;
    char out[256];
    TEST_ASSERT_EQUAL(0, wf_url_parse("https://Example.COM/a/b?x=1#frag", &u));
    TEST_ASSERT_EQUAL(1, u.https);
    TEST_ASSERT_EQUAL(443, u.port);
    TEST_ASSERT_EQUAL(0, strcmp(u.host, "example.com"));
    TEST_ASSERT_EQUAL(0, strcmp(u.path, "/a/b?x=1"));
    TEST_ASSERT_EQUAL(0, wf_url_parse("http://10.0.2.2:8080", &u));
    TEST_ASSERT_EQUAL(0, u.https);
    TEST_ASSERT_EQUAL(8080, u.port);
    TEST_ASSERT_EQUAL(0, strcmp(u.path, "/"));
    wf_url_format(&u, out, sizeof(out));
    TEST_ASSERT_EQUAL(0, strcmp(out, "http://10.0.2.2:8080/"));
    TEST_ASSERT_EQUAL(0, wf_url_parse("example.com", &u));
    TEST_ASSERT_EQUAL(1, u.https);
    TEST_ASSERT_EQUAL(0, wf_url_parse("http://h/?q", &u));
    TEST_ASSERT_EQUAL(0, strcmp(u.path, "/?q"));
    TEST_ASSERT_EQUAL(-1, wf_url_parse("ftp://x/", &u));
    TEST_ASSERT_EQUAL(-1, wf_url_parse("http://user@evil/", &u));
    TEST_ASSERT_EQUAL(-1, wf_url_parse("http://[::1]/", &u));
    TEST_ASSERT_EQUAL(-1, wf_url_parse("http://h:0/", &u));
    TEST_ASSERT_EQUAL(-1, wf_url_parse("http://h:99999/", &u));
    TEST_ASSERT_EQUAL(-1, wf_url_parse("http://h/a b", &u));
    TEST_ASSERT_EQUAL(-1, wf_url_parse("http://h/a\r\nX: y", &u));
    TEST_ASSERT_EQUAL(-1, wf_url_parse("http:///p", &u));
}

static void test_url_resolve(void) {
    wf_url_t b, o;
    TEST_ASSERT_EQUAL(0, wf_url_parse("https://a.test/dir/page?x", &b));
    TEST_ASSERT_EQUAL(0, wf_url_resolve(&b, "/root", &o));
    TEST_ASSERT_EQUAL(0, strcmp(o.path, "/root"));
    TEST_ASSERT_EQUAL(0, strcmp(o.host, "a.test"));
    TEST_ASSERT_EQUAL(0, wf_url_resolve(&b, "other#f", &o));
    TEST_ASSERT_EQUAL(0, strcmp(o.path, "/dir/other"));
    TEST_ASSERT_EQUAL(0, wf_url_resolve(&b, "//b.test/x", &o));
    TEST_ASSERT_EQUAL(0, strcmp(o.host, "b.test"));
    TEST_ASSERT_EQUAL(1, o.https);
    TEST_ASSERT_EQUAL(0, wf_url_resolve(&b, "http://c.test:81/y", &o));
    TEST_ASSERT_EQUAL(81, o.port);
    TEST_ASSERT_EQUAL(-1, wf_url_resolve(&b, "", &o));
}

static void test_build_get(void) {
    wf_url_t u;
    char r[512];
    TEST_ASSERT_EQUAL(0, wf_url_parse("http://h.test:8080/p?q", &u));
    TEST_ASSERT_TRUE(wf_http_build_get(&u, r, sizeof(r)) > 0);
    TEST_ASSERT_TRUE(strncmp(r, "GET /p?q HTTP/1.1\r\nHost: h.test:8080\r\n", 38) == 0);
    TEST_ASSERT_TRUE(strstr(r, "Connection: close\r\n\r\n") != 0);
    TEST_ASSERT_EQUAL(-1, wf_http_build_get(&u, r, 20));
}

static uint8_t g_body[256];

static int feed_all(wf_http_resp_t* r, const char* s, int bytewise) {
    int n = (int)strlen(s), i, rc = 0;
    if (!bytewise) return wf_http_feed(r, (const uint8_t*)s, n);
    for (i = 0; i < n; i++) { rc = wf_http_feed(r, (const uint8_t*)s + i, 1); if (rc != 0) return rc; }
    return rc;
}

static void test_resp_content_length(void) {
    wf_http_resp_t r;
    int bw;
    for (bw = 0; bw < 2; bw++) {
        wf_http_resp_init(&r, g_body, sizeof(g_body));
        TEST_ASSERT_EQUAL(1, feed_all(&r, "HTTP/1.1 200 OK\r\nContent-Type: text/html\r\nContent-Length: 5\r\n\r\nhello", bw));
        TEST_ASSERT_EQUAL(200, r.status);
        TEST_ASSERT_EQUAL(5, r.body_len);
        TEST_ASSERT_EQUAL(0, memcmp(g_body, "hello", 5));
        TEST_ASSERT_EQUAL(0, strcmp(r.content_type, "text/html"));
        TEST_ASSERT_EQUAL(0, strcmp(r.reason, "OK"));
    }
    wf_http_resp_init(&r, g_body, sizeof(g_body));
    TEST_ASSERT_EQUAL(-1, feed_all(&r, "HTTP/1.1 200 OK\r\nContent-Length: 5\r\nContent-Length: 6\r\n\r\n", 0));
    wf_http_resp_init(&r, g_body, sizeof(g_body));
    TEST_ASSERT_EQUAL(-1, feed_all(&r, "HTTP/1.1 200 OK\r\nContent-Length: -1\r\n\r\n", 0));
    wf_http_resp_init(&r, g_body, sizeof(g_body));
    TEST_ASSERT_EQUAL(0, feed_all(&r, "HTTP/1.1 200 OK\r\nContent-Length: 10\r\n\r\nabc", 0));
    TEST_ASSERT_TRUE(wf_http_eof(&r) < 0);
    wf_http_resp_init(&r, g_body, sizeof(g_body));
    TEST_ASSERT_EQUAL(1, feed_all(&r, "HTTP/1.1 204 No Content\r\n\r\n", 0));
    wf_http_resp_init(&r, g_body, sizeof(g_body));
    TEST_ASSERT_EQUAL(-1, feed_all(&r, "SSH-2.0-OpenSSH\r\n\r\n", 0));
    wf_http_resp_init(&r, g_body, sizeof(g_body));
    TEST_ASSERT_EQUAL(-1, feed_all(&r, "HTTP/1.1 200 OK\r\nContent-Encoding: gzip\r\n\r\n", 0));
}

static void test_resp_chunked(void) {
    wf_http_resp_t r;
    int bw;
    const char* m = "HTTP/1.1 200 OK\r\nTransfer-Encoding: chunked\r\n\r\n"
                    "5\r\nhello\r\n7;ext=1\r\n, world\r\n0\r\nX-Trailer: 1\r\n\r\n";
    for (bw = 0; bw < 2; bw++) {
        wf_http_resp_init(&r, g_body, sizeof(g_body));
        TEST_ASSERT_EQUAL(1, feed_all(&r, m, bw));
        TEST_ASSERT_EQUAL(12, r.body_len);
        TEST_ASSERT_EQUAL(0, memcmp(g_body, "hello, world", 12));
    }
    wf_http_resp_init(&r, g_body, sizeof(g_body));
    TEST_ASSERT_EQUAL(-1, feed_all(&r, "HTTP/1.1 200 OK\r\nTransfer-Encoding: chunked\r\n\r\nzz\r\n", 0));
    wf_http_resp_init(&r, g_body, sizeof(g_body));
    TEST_ASSERT_EQUAL(-1, feed_all(&r, "HTTP/1.1 200 OK\r\nTransfer-Encoding: chunked\r\n\r\n3\r\nabcX\r\n", 0));
    wf_http_resp_init(&r, g_body, sizeof(g_body));
    TEST_ASSERT_EQUAL(0, feed_all(&r, "HTTP/1.1 200 OK\r\nTransfer-Encoding: chunked\r\n\r\n3\r\nab", 0));
    TEST_ASSERT_TRUE(wf_http_eof(&r) < 0);
    /* chunked wins over Content-Length */
    wf_http_resp_init(&r, g_body, sizeof(g_body));
    TEST_ASSERT_EQUAL(1, feed_all(&r, "HTTP/1.1 200 OK\r\nContent-Length: 99\r\nTransfer-Encoding: chunked\r\n\r\n1\r\nA\r\n0\r\n\r\n", 0));
    TEST_ASSERT_EQUAL(1, r.body_len);
}

static void test_resp_close_and_truncate(void) {
    wf_http_resp_t r;
    uint8_t small[4];
    wf_http_resp_init(&r, g_body, sizeof(g_body));
    TEST_ASSERT_EQUAL(0, feed_all(&r, "HTTP/1.0 200 OK\r\n\r\nuntil close", 0));
    TEST_ASSERT_EQUAL(1, wf_http_eof(&r));
    TEST_ASSERT_EQUAL(11, r.body_len);
    wf_http_resp_init(&r, small, sizeof(small));
    TEST_ASSERT_EQUAL(1, feed_all(&r, "HTTP/1.1 200 OK\r\nContent-Length: 6\r\n\r\nabcdef", 0));
    TEST_ASSERT_EQUAL(4, r.body_len);
    TEST_ASSERT_EQUAL(1, r.truncated);
    wf_http_resp_init(&r, g_body, sizeof(g_body));
    TEST_ASSERT_EQUAL(1, feed_all(&r, "HTTP/1.1 100 Continue\r\n\r\nHTTP/1.1 200 OK\r\nContent-Length: 1\r\n\r\nZ", 0));
    TEST_ASSERT_EQUAL(200, r.status);
    TEST_ASSERT_EQUAL(1, r.body_len);
    wf_http_resp_init(&r, g_body, sizeof(g_body));
    TEST_ASSERT_TRUE(wf_http_eof(&r) < 0);
}

static void test_dns_codec(void) {
    uint8_t q[300], ip[4];
    int n = wf_dns_build_query("Example.com", 0x1234, q, sizeof(q));
    /* answer: header + question + CNAME (example.com -> www.example.com via pointer) + A for www */
    static uint8_t a[300];
    int p;
    TEST_ASSERT_EQUAL(12 + 13 + 4, n);
    TEST_ASSERT_EQUAL(7, q[12]);
    TEST_ASSERT_EQUAL('e', q[13]);
    memcpy(a, q, (size_t)n);
    a[2] = 0x81; a[3] = 0x80; a[7] = 2;
    p = n;
    /* CNAME: name ptr 0x0c, type 5, class 1, ttl, rdlen, "www" + ptr */
    a[p++] = 0xc0; a[p++] = 0x0c; a[p++] = 0; a[p++] = 5; a[p++] = 0; a[p++] = 1;
    a[p++] = 0; a[p++] = 0; a[p++] = 0; a[p++] = 60; a[p++] = 0; a[p++] = 6;
    {
        int cname_at = p;
        a[p++] = 3; a[p++] = 'w'; a[p++] = 'w'; a[p++] = 'w'; a[p++] = 0xc0; a[p++] = 0x0c;
        a[p++] = 0xc0; a[p++] = (uint8_t)cname_at; a[p++] = 0; a[p++] = 1; a[p++] = 0; a[p++] = 1;
        a[p++] = 0; a[p++] = 0; a[p++] = 0; a[p++] = 60; a[p++] = 0; a[p++] = 4;
        a[p++] = 93; a[p++] = 184; a[p++] = 215; a[p++] = 14;
    }
    TEST_ASSERT_EQUAL(0, wf_dns_parse_a(a, p, 0x1234, "example.com", ip));
    TEST_ASSERT_EQUAL(93, ip[0]);
    TEST_ASSERT_EQUAL(14, ip[3]);
    TEST_ASSERT_EQUAL(-1, wf_dns_parse_a(a, p, 0x9999, "example.com", ip));
    TEST_ASSERT_EQUAL(-1, wf_dns_parse_a(a, p, 0x1234, "other.com", ip));
    TEST_ASSERT_EQUAL(-1, wf_dns_parse_a(a, p - 3, 0x1234, "example.com", ip));
    a[3] = 0x83;
    TEST_ASSERT_EQUAL(-2, wf_dns_parse_a(a, p, 0x1234, "example.com", ip));
    /* pointer loop */
    a[3] = 0x80;
    a[n] = 0xc0; a[n + 1] = (uint8_t)n;
    TEST_ASSERT_EQUAL(-1, wf_dns_parse_a(a, p, 0x1234, "example.com", ip));
    TEST_ASSERT_EQUAL(-1, wf_dns_build_query("a..b", 1, q, sizeof(q)));
    TEST_ASSERT_EQUAL(0, wf_ipv4_parse("10.0.2.2", ip));
    TEST_ASSERT_EQUAL(-1, wf_ipv4_parse("10.0.2", ip));
    TEST_ASSERT_EQUAL(-1, wf_ipv4_parse("1.2.3.256", ip));
    TEST_ASSERT_EQUAL(-1, wf_ipv4_parse("example.com", ip));
}

static void test_tls_prf_known_answer(void) {
    static const uint8_t secret[16] = {0x9b,0xbe,0x43,0x6b,0xa9,0x40,0xf0,0x17,0xb1,0x76,0x52,0x84,0x9a,0x71,0xdb,0x35};
    static const uint8_t seed[16] = {0xa0,0xba,0x9f,0x93,0x6c,0xda,0x31,0x18,0x27,0xa6,0xf7,0x96,0xff,0xd5,0x19,0x8c};
    static const uint8_t want[100] = {
        0xe3,0xf2,0x29,0xba,0x72,0x7b,0xe1,0x7b,0x8d,0x12,0x26,0x20,0x55,0x7c,0xd4,0x53,0xc2,0xaa,0xb2,0x1d,
        0x07,0xc3,0xd4,0x95,0x32,0x9b,0x52,0xd4,0xe6,0x1e,0xdb,0x5a,0x6b,0x30,0x17,0x91,0xe9,0x0d,0x35,0xc9,
        0xc9,0xa4,0x6b,0x4e,0x14,0xba,0xf9,0xaf,0x0f,0xa0,0x22,0xf7,0x07,0x7d,0xef,0x17,0xab,0xfd,0x37,0x97,
        0xc0,0x56,0x4b,0xab,0x4f,0xbc,0x91,0x66,0x6e,0x9d,0xef,0x9b,0x97,0xfc,0xe3,0x4f,0x79,0x67,0x89,0xba,
        0xa4,0x80,0x82,0xd1,0x22,0xee,0x42,0xc5,0xa7,0x2e,0x5a,0x51,0x10,0xff,0xf7,0x01,0x87,0x34,0x7b,0x66};
    uint8_t out[100];
    wf_tls_prf(secret, 16, "test label", seed, 16, out, 100);
    TEST_ASSERT_EQUAL(0, memcmp(out, want, 100));
}

/* ---- in-memory transport: each connect replays the next canned reply ---- */
static const char* g_replies[8];
static int g_reply_count, g_next, g_cur_off, g_connects, g_resolves;
static char g_last_req[512];

static int m_resolve(void* n, const char* h, uint8_t ip[4]) {
    (void)n; g_resolves++;
    if (strcmp(h, "nx.test") == 0) return -1;
    ip[0] = 10; ip[1] = 0; ip[2] = 2; ip[3] = 2;
    return 0;
}
static int m_connect(void* n, const uint8_t ip[4], int port) {
    (void)n; (void)ip; (void)port;
    if (g_next >= g_reply_count) return -1;
    g_cur_off = 0; g_connects++;
    return g_next++;
}
static int m_send(void* n, int h, const uint8_t* d, int len) {
    (void)n; (void)h;
    if (len > (int)sizeof(g_last_req) - 1) len = (int)sizeof(g_last_req) - 1;
    memcpy(g_last_req, d, (size_t)len); g_last_req[len] = 0;
    return len;
}
static int m_recv(void* n, int h, uint8_t* d, int cap) {
    const char* s = g_replies[h];
    int left = (int)strlen(s) - g_cur_off, k;
    (void)n;
    if (left <= 0) return 0;
    k = left < 7 ? left : 7;   /* small reads exercise the incremental parser */
    if (k > cap) k = cap;
    memcpy(d, s + g_cur_off, (size_t)k);
    g_cur_off += k;
    return k;
}
static void m_close(void* n, int h) { (void)n; (void)h; }
static int m_rand(void* n, uint8_t* o, int len) { (void)n; memset(o, 7, (size_t)len); return 0; }

static void env_init(wf_fetch_env_t* e) {
    memset(e, 0, sizeof(*e));
    e->resolve = m_resolve; e->connect = m_connect; e->send = m_send; e->recv = m_recv;
    e->close = m_close; e->rand = m_rand; e->max_redirects = 5;
    g_next = 0; g_connects = 0; g_resolves = 0;
}

static void test_fetch_redirects(void) {
    static wf_fetch_env_t e;
    static wf_fetch_result_t r;
    static uint8_t body[128];
    env_init(&e);
    g_replies[0] = "HTTP/1.1 302 Found\r\nLocation: /next\r\nContent-Length: 0\r\n\r\n";
    g_replies[1] = "HTTP/1.1 301 Moved\r\nLocation: http://10.0.2.2:8080/final\r\nContent-Length: 0\r\n\r\n";
    g_replies[2] = "HTTP/1.1 200 OK\r\nContent-Type: text/plain\r\nTransfer-Encoding: chunked\r\n\r\n4\r\ndone\r\n0\r\n\r\n";
    g_reply_count = 3;
    TEST_ASSERT_EQUAL(0, wf_fetch(&e, "http://site.test/start", body, sizeof(body), &r));
    TEST_ASSERT_EQUAL(200, r.status);
    TEST_ASSERT_EQUAL(2, r.redirects);
    TEST_ASSERT_EQUAL(4, r.body_len);
    TEST_ASSERT_EQUAL(0, memcmp(body, "done", 4));
    TEST_ASSERT_EQUAL(0, strcmp(r.url, "http://10.0.2.2:8080/final"));
    TEST_ASSERT_EQUAL(2, g_resolves);   /* the IPv4 literal is not resolved */
    TEST_ASSERT_TRUE(strncmp(g_last_req, "GET /final HTTP/1.1\r\nHost: 10.0.2.2:8080\r\n", 42) == 0);

    /* loop: 6 redirects with a limit of 5 */
    env_init(&e);
    {
        int i;
        for (i = 0; i < 7; i++) g_replies[i] = "HTTP/1.1 302 Found\r\nLocation: /again\r\nContent-Length: 0\r\n\r\n";
        g_reply_count = 7;
    }
    TEST_ASSERT_EQUAL(-1, wf_fetch(&e, "http://site.test/", body, sizeof(body), &r));
    TEST_ASSERT_EQUAL(0, strcmp(r.err, "too many redirects"));
    TEST_ASSERT_EQUAL(6, g_connects);

    /* limit 0: the 302 itself is the answer when there is no Location */
    env_init(&e);
    g_replies[0] = "HTTP/1.1 302 Found\r\nContent-Length: 0\r\n\r\n";
    g_reply_count = 1;
    TEST_ASSERT_EQUAL(0, wf_fetch(&e, "http://site.test/", body, sizeof(body), &r));
    TEST_ASSERT_EQUAL(302, r.status);

    env_init(&e);
    g_reply_count = 0;
    TEST_ASSERT_EQUAL(-1, wf_fetch(&e, "http://nx.test/", body, sizeof(body), &r));
    TEST_ASSERT_TRUE(strstr(r.err, "DNS") != 0);
    env_init(&e);
    TEST_ASSERT_EQUAL(-1, wf_fetch(&e, "gopher://x/", body, sizeof(body), &r));
    env_init(&e);
    g_replies[0] = "HTTP/1.1 200 OK\r\nContent-Length: 9\r\n\r\nshort";
    g_reply_count = 1;
    TEST_ASSERT_EQUAL(-1, wf_fetch(&e, "http://site.test/", body, sizeof(body), &r));
    TEST_ASSERT_TRUE(strstr(r.err, "before the end of the body") != 0);
}

int main(void) {
    unity_init();
    RUN_TEST(test_url_parse);
    RUN_TEST(test_url_resolve);
    RUN_TEST(test_build_get);
    RUN_TEST(test_resp_content_length);
    RUN_TEST(test_resp_chunked);
    RUN_TEST(test_resp_close_and_truncate);
    RUN_TEST(test_dns_codec);
    RUN_TEST(test_tls_prf_known_answer);
    RUN_TEST(test_fetch_redirects);
    unity_print_results();
    unity_cleanup();
    return (unity_stats.tests_failed == 0) ? 0 : 1;
}
