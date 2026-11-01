/* test_wf_http.c - Lot 67: URL, HTTP/1.1 response parser (Content-Length,
 * chunked, close-delimited), DNS codec, TLS PRF known answer, and wf_fetch
 * redirects over an in-memory transport. */
#include "../../framework/unity.h"
#include <string.h>
#include <stdio.h>
#include "wf_http.h"
#include "wf_dns.h"
#include "wf_tls.h"
#include "wf_ec.h"
#include "wf_inflate.h"
#include "wf_inflate_vectors.h"
#include "wf_fetch.h"
#include "wf_html.h"

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
    /* Lot 69: gzip/deflate are decoded by wf_fetch; other codings stay refused */
    TEST_ASSERT_EQUAL(-1, feed_all(&r, "HTTP/1.1 200 OK\r\nContent-Encoding: br\r\n\r\n", 0));
    wf_http_resp_init(&r, g_body, sizeof(g_body));
    TEST_ASSERT_EQUAL(-1, feed_all(&r, "HTTP/1.1 200 OK\r\nContent-Encoding: gzip, br\r\n\r\n", 0));
    wf_http_resp_init(&r, g_body, sizeof(g_body));
    TEST_ASSERT_EQUAL(1, feed_all(&r, "HTTP/1.1 200 OK\r\nContent-Encoding: gzip\r\nContent-Length: 0\r\n\r\n", 0));
    TEST_ASSERT_EQUAL(1, r.content_encoding);
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


/* Lot 69: TLS 1.3 key schedule, RFC 8448 section 3 (simple 1-RTT). */
static void test_tls13_rfc8448_key_schedule(void) {
    static const uint8_t zero[32] = {0};
    static const uint8_t empty_hash[32] = {0xe3,0xb0,0xc4,0x42,0x98,0xfc,0x1c,0x14,0x9a,0xfb,0xf4,0xc8,0x99,0x6f,0xb9,0x24,0x27,0xae,0x41,0xe4,0x64,0x9b,0x93,0x4c,0xa4,0x95,0x99,0x1b,0x78,0x52,0xb8,0x55};
    static const uint8_t shared[32] = {0x8b,0xd4,0x05,0x4f,0xb5,0x5b,0x9d,0x63,0xfd,0xfb,0xac,0xf9,0xf0,0x4b,0x9f,0x0d,0x35,0xe6,0xd6,0x3f,0x53,0x75,0x63,0xef,0xd4,0x62,0x72,0x90,0x0f,0x89,0x49,0x2d};
    static const uint8_t th[32] = {0x86,0x0c,0x06,0xed,0xc0,0x78,0x58,0xee,0x8e,0x78,0xf0,0xe7,0x42,0x8c,0x58,0xed,0xd6,0xb4,0x3f,0x2c,0xa3,0xe6,0xe9,0x5f,0x02,0xed,0x06,0x3c,0xf0,0xe1,0xca,0xd8};
    static const uint8_t w_early[32] = {0x33,0xad,0x0a,0x1c,0x60,0x7e,0xc0,0x3b,0x09,0xe6,0xcd,0x98,0x93,0x68,0x0c,0xe2,0x10,0xad,0xf3,0x00,0xaa,0x1f,0x26,0x60,0xe1,0xb2,0x2e,0x10,0xf1,0x70,0xf9,0x2a}, w_derived[32] = {0x6f,0x26,0x15,0xa1,0x08,0xc7,0x02,0xc5,0x67,0x8f,0x54,0xfc,0x9d,0xba,0xb6,0x97,0x16,0xc0,0x76,0x18,0x9c,0x48,0x25,0x0c,0xeb,0xea,0xc3,0x57,0x6c,0x36,0x11,0xba}, w_hs[32] = {0x1d,0xc8,0x26,0xe9,0x36,0x06,0xaa,0x6f,0xdc,0x0a,0xad,0xc1,0x2f,0x74,0x1b,0x01,0x04,0x6a,0xa6,0xb9,0x9f,0x69,0x1e,0xd2,0x21,0xa9,0xf0,0xca,0x04,0x3f,0xbe,0xac}, w_chs[32] = {0xb3,0xed,0xdb,0x12,0x6e,0x06,0x7f,0x35,0xa7,0x80,0xb3,0xab,0xf4,0x5e,0x2d,0x8f,0x3b,0x1a,0x95,0x07,0x38,0xf5,0x2e,0x96,0x00,0x74,0x6a,0x0e,0x27,0xa5,0x5a,0x21}, w_shs[32] = {0xb6,0x7b,0x7d,0x69,0x0c,0xc1,0x6c,0x4e,0x75,0xe5,0x42,0x13,0xcb,0x2d,0x37,0xb4,0xe9,0xc9,0x12,0xbc,0xde,0xd9,0x10,0x5d,0x42,0xbe,0xfd,0x59,0xd3,0x91,0xad,0x38};
    static const uint8_t w_key[16] = {0x3f,0xce,0x51,0x60,0x09,0xc2,0x17,0x27,0xd0,0xf2,0xe4,0xe8,0x6e,0xe4,0x03,0xbc}, w_iv[12] = {0x5d,0x31,0x3e,0xb2,0x67,0x12,0x76,0xee,0x13,0x00,0x0b,0x30};
    uint8_t early[32], derived[32], hs[32], chs[32], shs[32], key[16], iv[12];
    wf_hkdf_extract(0, 0, zero, 32, early);
    TEST_ASSERT_EQUAL(0, memcmp(early, w_early, 32));
    wf_hkdf_expand_label(early, "derived", empty_hash, 32, derived, 32);
    TEST_ASSERT_EQUAL(0, memcmp(derived, w_derived, 32));
    wf_hkdf_extract(derived, 32, shared, 32, hs);
    TEST_ASSERT_EQUAL(0, memcmp(hs, w_hs, 32));
    wf_hkdf_expand_label(hs, "c hs traffic", th, 32, chs, 32);
    TEST_ASSERT_EQUAL(0, memcmp(chs, w_chs, 32));
    wf_hkdf_expand_label(hs, "s hs traffic", th, 32, shs, 32);
    TEST_ASSERT_EQUAL(0, memcmp(shs, w_shs, 32));
    wf_hkdf_expand_label(shs, "key", 0, 0, key, 16);
    wf_hkdf_expand_label(shs, "iv", 0, 0, iv, 12);
    TEST_ASSERT_EQUAL(0, memcmp(key, w_key, 16));
    TEST_ASSERT_EQUAL(0, memcmp(iv, w_iv, 12));
}

/* Lot 69: P-256 ECDH, RFC 5903 section 8.1. */
static void test_ecdh_p256_rfc5903(void) {
    static const uint8_t i_priv[32] = {0xc8,0x8f,0x01,0xf5,0x10,0xd9,0xac,0x3f,0x70,0xa2,0x92,0xda,0xa2,0x31,0x6d,0xe5,0x44,0xe9,0xaa,0xb8,0xaf,0xe8,0x40,0x49,0xc6,0x2a,0x9c,0x57,0x86,0x2d,0x14,0x33};
    static const uint8_t r_pub[65] = {0x04,0xd1,0x2d,0xfb,0x52,0x89,0xc8,0xd4,0xf8,0x12,0x08,0xb7,0x02,0x70,0x39,0x8c,0x34,0x22,0x96,0x97,0x0a,0x0b,0xcc,0xb7,0x4c,0x73,0x6f,0xc7,0x55,0x44,0x94,0xbf,0x63,0x56,0xfb,0xf3,0xca,0x36,0x6c,0xc2,0x3e,0x81,0x57,0x85,0x4c,0x13,0xc5,0x8d,0x6a,0xac,0x23,0xf0,0x46,0xad,0xa3,0x0f,0x83,0x53,0xe7,0x4f,0x33,0x03,0x98,0x72,0xab};
    static const uint8_t w_pub[65] = {0x04,0xda,0xd0,0xb6,0x53,0x94,0x22,0x1c,0xf9,0xb0,0x51,0xe1,0xfe,0xca,0x57,0x87,0xd0,0x98,0xdf,0xe6,0x37,0xfc,0x90,0xb9,0xef,0x94,0x5d,0x0c,0x37,0x72,0x58,0x11,0x80,0x52,0x71,0xa0,0x46,0x1c,0xdb,0x82,0x52,0xd6,0x1f,0x1c,0x45,0x6f,0xa3,0xe5,0x9a,0xb1,0xf4,0x5b,0x33,0xac,0xcf,0x5f,0x58,0x38,0x9e,0x05,0x77,0xb8,0x99,0x0b,0xb3};
    static const uint8_t w_z[32] = {0xd6,0x84,0x0f,0x6b,0x42,0xf6,0xed,0xaf,0xd1,0x31,0x16,0xe0,0xe1,0x25,0x65,0x20,0x2f,0xef,0x8e,0x9e,0xce,0x7d,0xce,0x03,0x81,0x24,0x64,0xd0,0x4b,0x94,0x42,0xde};
    uint8_t pub[65], z[32];
    TEST_ASSERT_EQUAL(0, wf_ecdh_p256(i_priv, r_pub, 65, pub, z));
    TEST_ASSERT_EQUAL(0, memcmp(pub, w_pub, 65));
    TEST_ASSERT_EQUAL(0, memcmp(z, w_z, 32));
    /* a point off the curve is refused */
    {
        uint8_t bad[65];
        memcpy(bad, r_pub, 65); bad[64] ^= 1;
        TEST_ASSERT_TRUE(wf_ecdh_p256(i_priv, bad, 65, 0, z) != 0);
    }
}


/* Lot 69: Content-Encoding gzip / deflate, decoded in place. */
static uint32_t crc32_ref(const uint8_t* d, int n) {
    uint32_t c = 0xffffffffU; int i, k;
    for (i = 0; i < n; i++) { c ^= d[i]; for (k = 0; k < 8; k++) c = (c >> 1) ^ (0xedb88320U & (0U - (c & 1U))); }
    return ~c;
}
static void test_inflate_vectors(void) {
    static uint8_t buf[32768];
    int tr = -1, n, used;
    memcpy(buf, wf_inf_gzip, sizeof(wf_inf_gzip));
    n = wf_decode_body_inplace(WF_ENC_GZIP, buf, (int)sizeof(wf_inf_gzip), (int)sizeof(buf), &tr);
    TEST_ASSERT_EQUAL(WF_INF_PLAIN_LEN, n);
    TEST_ASSERT_EQUAL(0, tr);
    TEST_ASSERT_EQUAL(0, memcmp(buf, wf_inf_plain_line0, strlen(wf_inf_plain_line0)));
    TEST_ASSERT_TRUE(crc32_ref(buf, n) == wf_inf_plain_crc);
    memcpy(buf, wf_inf_zlib, sizeof(wf_inf_zlib));
    n = wf_decode_body_inplace(WF_ENC_DEFLATE, buf, (int)sizeof(wf_inf_zlib), (int)sizeof(buf), &tr);
    TEST_ASSERT_EQUAL(WF_INF_PLAIN_LEN, n);
    TEST_ASSERT_TRUE(crc32_ref(buf, n) == wf_inf_plain_crc);
    memcpy(buf, wf_inf_raw, sizeof(wf_inf_raw));      /* raw deflate under "deflate" */
    n = wf_decode_body_inplace(WF_ENC_DEFLATE, buf, (int)sizeof(wf_inf_raw), (int)sizeof(buf), &tr);
    TEST_ASSERT_EQUAL(WF_INF_PLAIN_LEN, n);
    TEST_ASSERT_TRUE(crc32_ref(buf, n) == wf_inf_plain_crc);
    n = wf_inflate(wf_inf_fixed, (int)sizeof(wf_inf_fixed), &used, buf, (int)sizeof(buf), &tr);
    TEST_ASSERT_EQUAL(WF_INF_PLAIN_LEN, n);
    TEST_ASSERT_EQUAL((int)sizeof(wf_inf_fixed), used);
    n = wf_inflate(wf_inf_stored, (int)sizeof(wf_inf_stored), &used, buf, (int)sizeof(buf), &tr);
    TEST_ASSERT_EQUAL(WF_INF_STORED_LEN, n);
    TEST_ASSERT_EQUAL(0, memcmp(buf, wf_inf_plain_line0, strlen(wf_inf_plain_line0)));
    /* bounded: a small buffer cuts the output and says so */
    memcpy(buf, wf_inf_gzip, sizeof(wf_inf_gzip));
    n = wf_decode_body_inplace(WF_ENC_GZIP, buf, (int)sizeof(wf_inf_gzip), 4096, &tr);
    TEST_ASSERT_TRUE(n > 0 && n < 4096);
    TEST_ASSERT_EQUAL(1, tr);
    TEST_ASSERT_EQUAL(0, memcmp(buf, wf_inf_plain_line0, strlen(wf_inf_plain_line0)));
    /* input cut short: partial output, truncated */
    memcpy(buf, wf_inf_gzip, sizeof(wf_inf_gzip) / 2);
    n = wf_decode_body_inplace(WF_ENC_GZIP, buf, (int)sizeof(wf_inf_gzip) / 2, (int)sizeof(buf), &tr);
    TEST_ASSERT_TRUE(n > 0 && n < WF_INF_PLAIN_LEN);
    TEST_ASSERT_EQUAL(1, tr);
    /* garbage is refused */
    memset(buf, 0xff, 64);
    TEST_ASSERT_EQUAL(-1, wf_decode_body_inplace(WF_ENC_GZIP, buf, 64, (int)sizeof(buf), &tr));
}


/* Lot 69: real-page shapes (Wikipedia front page): 300 language links
 * before the content, nested empty divs, UTF-8 accents, hidden blocks. The
 * reader view must keep the content, drop the chrome and stay within the
 * layout engine node table (160). */
static void test_html_reduce_real_shapes(void) {
    static char page[120000], out[16384];
    wf_html_stats_t st;
    wf_url_t base;
    int n = 0, i, opens = 0, m;
    n += sprintf(page + n, "<!DOCTYPE html><html><head><title>Wikip\xc3\xa9" "dia, l'encyclop\xc3\xa9" "die libre</title>"
                 "<style>.x{}</style></head><body><a class=\"mw-jump-link skip-link\" href=\"#c\">Aller</a>"
                 "<nav><ul><li>menu</li></ul></nav><div role=\"navigation\"><a href=\"/a\">Nav</a></div>"
                 "<div hidden><p>secret</p></div><ul class=\"vector-menu-content-list\">");
    for (i = 0; i < 300; i++)
        n += sprintf(page + n, "<li class=\"interlanguage-link\"><a href=\"https://l%d.wikipedia.org/\" hreflang=\"l%d\" lang=\"l%d\">Lang%03d</a></li>", i, i, i, i);
    n += sprintf(page + n, "</ul><div><div><div></div></div></div><main><h1>Bienvenue sur Wikip\xc3\xa9" "dia</h1>"
                 "<p>L'encyclop\xc3\xa9" "die libre \xe2\x80\x94 que vous pouvez am\xc3\xa9liorer.</p>");
    for (i = 0; i < 400; i++) n += sprintf(page + n, "<div><p>Paragraphe %d <a href=\"/wiki/P%d\">lien</a></p></div>", i, i);
    n += sprintf(page + n, "</main></body></html>");
    TEST_ASSERT_EQUAL(0, wf_url_parse("https://fr.wikipedia.org/", &base));
    m = wf_html_reduce(page, n, &base, out, (int)sizeof(out), &st);
    TEST_ASSERT_TRUE(m > 0);
    /* Lot 71: accents are kept as display bytes (CP1252), the em dash too */
    TEST_ASSERT_NOT_NULL(strstr(st.title, "Wikip\xe9" "dia, l'encyclop\xe9" "die libre"));
    TEST_ASSERT_NOT_NULL(strstr(out, "<h1>Bienvenue sur Wikip\xe9" "dia</h1>"));
    TEST_ASSERT_NOT_NULL(strstr(out, "L'encyclop\xe9" "die libre \x97 que vous pouvez am\xe9liorer."));
    TEST_ASSERT_NULL(strstr(out, "Lang0"));
    TEST_ASSERT_NULL(strstr(out, "secret"));
    TEST_ASSERT_NULL(strstr(out, "Nav<"));
    TEST_ASSERT_NULL(strstr(out, "<p></p>"));
    TEST_ASSERT_NOT_NULL(strstr(out, "href=\"https://fr.wikipedia.org/wiki/P0\""));
    TEST_ASSERT_EQUAL(1, st.truncated);
    TEST_ASSERT_NOT_NULL(strstr(out, "[page tronquee]"));
    for (i = 0; out[i]; i++) if (out[i] == '<' && out[i + 1] != '/') opens++;
    TEST_ASSERT_TRUE(opens <= 150);   /* elements; with text runs the engine table (160) holds */
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

/* Lot 67: reader view of a real page */
static void test_html_reduce(void) {
    static const char page[] =
        "<!doctype html><html><head><title>Ex &amp; Co</title><style>p{color:red}</style>"
        "<script>var a='<p>evil</p>';</script></head><body onload=x()>"
        "<h1 class=big>Hello</h1><!-- hidden <p>no</p> --><svg><text>no</text></svg>"
        "<p>One <b>two</b> <a href=\"/docs?q=1\" onclick=\"y()\">docs</a> "
        "<a href=\"javascript:alert(1)\">bad</a></p><ul><li>a<li>b</ul></body></html>";
    static char out[2048];
    wf_url_t base;
    wf_html_stats_t st;
    int n;
    TEST_ASSERT_EQUAL(0, wf_url_parse("https://example.com/dir/page.html", &base));
    n = wf_html_reduce(page, (int)strlen(page), &base, out, (int)sizeof(out), &st);
    TEST_ASSERT_TRUE(n > 0 && n < (int)sizeof(out));
    TEST_ASSERT_EQUAL_STRING("Ex & Co", st.title);
    TEST_ASSERT_NOT_NULL(strstr(out, "<h1>Hello</h1>"));
    TEST_ASSERT_NOT_NULL(strstr(out, "href=\"https://example.com/docs?q=1\""));
    TEST_ASSERT_NULL(strstr(out, "evil"));
    TEST_ASSERT_NULL(strstr(out, "hidden"));
    TEST_ASSERT_NULL(strstr(out, "onclick"));
    TEST_ASSERT_NULL(strstr(out, "onload"));
    TEST_ASSERT_NULL(strstr(out, "javascript:"));
    TEST_ASSERT_NULL(strstr(out, "color:red"));
    TEST_ASSERT_TRUE(st.blocks_dropped >= 1);
    TEST_ASSERT_EQUAL(0, st.truncated);
}

static void test_html_reduce_budget(void) {
    static char big[20000];
    static char out[512];
    wf_html_stats_t st;
    int i, n, p = 0;
    for (i = 0; i < 600; i++) { memcpy(big + p, "<p>lorem ipsum</p>", 18); p += 18; }
    big[p] = 0;
    n = wf_html_reduce(big, p, 0, out, (int)sizeof(out), &st);
    TEST_ASSERT_TRUE(n < (int)sizeof(out));
    TEST_ASSERT_EQUAL(1, st.truncated);
    TEST_ASSERT_NOT_NULL(strstr(out, "[page tronquee]"));
    TEST_ASSERT_EQUAL(0, (int)out[n]);
}

static void test_html_text(void) {
    static const char page[] = "<p>a &lt;b&gt; &quot;c&quot; &#39;d&#39;</p><script>x</script><p>e&nbsp;f</p>";
    static char out[256];
    int n = wf_html_text(page, (int)strlen(page), out, (int)sizeof(out));
    TEST_ASSERT_TRUE(n > 0);
    TEST_ASSERT_NOT_NULL(strstr(out, "a <b> \"c\" 'd'"));
    TEST_ASSERT_NOT_NULL(strstr(out, "e f"));
    TEST_ASSERT_NULL(strstr(out, "x"));
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
    RUN_TEST(test_tls13_rfc8448_key_schedule);
    RUN_TEST(test_ecdh_p256_rfc5903);
    RUN_TEST(test_inflate_vectors);
    RUN_TEST(test_fetch_redirects);
    RUN_TEST(test_html_reduce);
    RUN_TEST(test_html_reduce_budget);
    RUN_TEST(test_html_text);
    RUN_TEST(test_html_reduce_real_shapes);
    unity_print_results();
    unity_cleanup();
    return (unity_stats.tests_failed == 0) ? 0 : 1;
}
