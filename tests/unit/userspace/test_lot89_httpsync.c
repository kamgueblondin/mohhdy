/* test_lot89_httpsync.c - Lot 89 (gap 6, second half): an HTTP answer
 * followed by bytes nobody asked for. The parser counts the bytes after the
 * end of the message (trailing) and wf_fetch never keeps that connection,
 * so the next request is not answered by the stray bytes. In-memory
 * transport: each connection plays a script of reads. */
#include "../../framework/unity.h"
#include <string.h>
#include "wf_http.h"
#include "wf_fetch.h"

static uint8_t body[2048];
static wf_http_resp_t resp;

static int feed_str(const char* s) { return wf_http_feed(&resp, (const uint8_t*)s, (int)strlen(s)); }

static void test_trailing_after_content_length(void) {
    wf_http_resp_init(&resp, body, (int)sizeof(body));
    TEST_ASSERT_EQUAL(1, feed_str("HTTP/1.1 200 OK\r\nContent-Length: 3\r\n\r\nabcHTTP/1.1 404 x\r\n"));
    TEST_ASSERT_EQUAL(3, resp.body_len);
    TEST_ASSERT_EQUAL(16, resp.trailing);
    TEST_ASSERT_EQUAL(0, memcmp(body, "abc", 3));
}

static void test_no_trailing_when_exact_or_split(void) {
    wf_http_resp_init(&resp, body, (int)sizeof(body));
    TEST_ASSERT_EQUAL(0, feed_str("HTTP/1.1 200 OK\r\nContent-Length: 5\r\n\r\nab"));
    TEST_ASSERT_EQUAL(1, feed_str("cde"));
    TEST_ASSERT_EQUAL(0, resp.trailing);
    TEST_ASSERT_EQUAL(5, resp.body_len);
}

static void test_trailing_after_chunked_and_empty_body(void) {
    wf_http_resp_init(&resp, body, (int)sizeof(body));
    TEST_ASSERT_EQUAL(1, feed_str("HTTP/1.1 200 OK\r\nTransfer-Encoding: chunked\r\n\r\n2\r\nhi\r\n0\r\n\r\nXY"));
    TEST_ASSERT_EQUAL(2, resp.trailing);
    TEST_ASSERT_EQUAL(2, resp.body_len);
    wf_http_resp_init(&resp, body, (int)sizeof(body));
    TEST_ASSERT_EQUAL(1, feed_str("HTTP/1.1 200 OK\r\nContent-Length: 0\r\n\r\nZ"));
    TEST_ASSERT_EQUAL(1, resp.trailing);
}

/* ---- wf_fetch over a scripted transport ---- */
#define MAX_READS 4
typedef struct { const char* reads[MAX_READS]; int next; int open; } fake_conn_t;
static fake_conn_t conns[4];
static int nconn, script_n;
static const char* scripts[4][MAX_READS];

static int f_resolve(void* n, const char* h, uint8_t ip[4]) { (void)n; (void)h; ip[0] = 10; ip[1] = 0; ip[2] = 2; ip[3] = 2; return 0; }
static int f_connect(void* n, const uint8_t ip[4], int port) {
    int k;
    (void)n; (void)ip; (void)port;
    if (nconn >= 4 || nconn >= script_n) return -1;
    for (k = 0; k < MAX_READS; k++) conns[nconn].reads[k] = scripts[nconn][k];
    conns[nconn].next = 0; conns[nconn].open = 1;
    return nconn++;
}
static int f_send(void* n, int h, const uint8_t* d, int len) { (void)n; (void)d; return conns[h].open ? len : -1; }
static int f_recv(void* n, int h, uint8_t* d, int cap) {
    const char* s;
    int len;
    (void)n;
    if (!conns[h].open || conns[h].next >= MAX_READS || !conns[h].reads[conns[h].next]) return 0;
    s = conns[h].reads[conns[h].next++];
    len = (int)strlen(s);
    if (len > cap) len = cap;
    memcpy(d, s, (size_t)len);
    return len;
}
static void f_close(void* n, int h) { (void)n; if (h >= 0 && h < 4) conns[h].open = 0; }
static int f_rand(void* n, uint8_t* o, int len) { (void)n; memset(o, 7, (size_t)len); return 0; }

static wf_fetch_env_t env;
static wf_fetch_result_t res;

static void env_reset(void) {
    memset(&env, 0, sizeof(env));
    memset(conns, 0, sizeof(conns));
    memset(scripts, 0, sizeof(scripts));
    nconn = 0; script_n = 0;
    env.resolve = f_resolve; env.connect = f_connect; env.send = f_send; env.recv = f_recv;
    env.close = f_close; env.rand = f_rand; env.keepalive = 1; env.max_redirects = 2;
}

#define OK1 "HTTP/1.1 200 OK\r\nContent-Length: 4\r\n\r\none\n"
#define OK2 "HTTP/1.1 200 OK\r\nContent-Length: 4\r\n\r\ntwo\n"
#define STRAY "HTTP/1.1 404 Not Found\r\nContent-Length: 9\r\n\r\nnot found"

static void test_fetch_kept_connection_is_reused(void) {
    int before = wf_fetch_unsolicited();
    env_reset();
    scripts[0][0] = OK1; scripts[0][1] = OK2; script_n = 1;
    TEST_ASSERT_EQUAL(0, wf_fetch(&env, "http://10.0.2.2:8080/a", body, (int)sizeof(body), &res));
    TEST_ASSERT_EQUAL(0, res.unsolicited);
    TEST_ASSERT_EQUAL(0, wf_fetch(&env, "http://10.0.2.2:8080/b", body, (int)sizeof(body), &res));
    TEST_ASSERT_EQUAL(200, res.status);
    TEST_ASSERT_EQUAL(1, res.reused);
    TEST_ASSERT_EQUAL(0, memcmp(body, "two\n", 4));
    TEST_ASSERT_EQUAL(1, nconn);
    TEST_ASSERT_EQUAL(before, wf_fetch_unsolicited());
    wf_fetch_drop(&env);
}

static void test_fetch_stray_answer_drops_connection(void) {
    int before = wf_fetch_unsolicited();
    env_reset();
    /* first connection: the answer and a stray 404 in one read, then (if
     * it were reused) nothing more; second connection: the real answer */
    scripts[0][0] = OK1 STRAY;
    scripts[1][0] = OK2;
    script_n = 2;
    TEST_ASSERT_EQUAL(0, wf_fetch(&env, "http://10.0.2.2:8080/a", body, (int)sizeof(body), &res));
    TEST_ASSERT_EQUAL(200, res.status);
    TEST_ASSERT_EQUAL(4, res.body_len);
    TEST_ASSERT_EQUAL((int)strlen(STRAY), res.unsolicited);
    TEST_ASSERT_EQUAL(0, conns[0].open);
    TEST_ASSERT_EQUAL(before + 1, wf_fetch_unsolicited());
    TEST_ASSERT_EQUAL(0, wf_fetch(&env, "http://10.0.2.2:8080/b", body, (int)sizeof(body), &res));
    TEST_ASSERT_EQUAL(200, res.status);
    TEST_ASSERT_EQUAL(0, res.reused);
    TEST_ASSERT_EQUAL(1, res.connections);
    TEST_ASSERT_EQUAL(0, memcmp(body, "two\n", 4));
    TEST_ASSERT_EQUAL(2, nconn);
    wf_fetch_drop(&env);
}

static void test_fetch_stray_without_keepalive_counted(void) {
    int before = wf_fetch_unsolicited();
    env_reset();
    env.keepalive = 0;
    scripts[0][0] = OK1 STRAY;
    script_n = 1;
    TEST_ASSERT_EQUAL(0, wf_fetch(&env, "http://10.0.2.2:8080/a", body, (int)sizeof(body), &res));
    TEST_ASSERT_EQUAL(200, res.status);
    TEST_ASSERT_EQUAL(0, memcmp(body, "one\n", 4));
    TEST_ASSERT_EQUAL(before + 1, wf_fetch_unsolicited());
    TEST_ASSERT_EQUAL(0, conns[0].open);
}

int main(void) {
    unity_init();
    RUN_TEST(test_trailing_after_content_length);
    RUN_TEST(test_no_trailing_when_exact_or_split);
    RUN_TEST(test_trailing_after_chunked_and_empty_body);
    RUN_TEST(test_fetch_kept_connection_is_reused);
    RUN_TEST(test_fetch_stray_answer_drops_connection);
    RUN_TEST(test_fetch_stray_without_keepalive_counted);
    unity_print_results();
    unity_cleanup();
    return unity_stats.tests_failed == 0 ? 0 : 1;
}
