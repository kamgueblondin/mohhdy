/* Lot 71: bounded cookie jar (userspace/wf_cookie.c). */
#include "../../framework/unity.h"
#include "../../../userspace/wf_cookie.h"
#include <string.h>

static wf_cookie_jar_t g_j;
static char g_h[WF_COOKIE_HDR];
#define NOW "20261101120000"

static const char* hdr(const char* host, const char* path, int https) {
    if (wf_cookie_header(&g_j, host, path, https, g_h, (int)sizeof(g_h)) < 0) strcpy(g_h, "<overflow>");
    return g_h;
}

static void test_basic_set_and_send(void) {
    wf_cookie_jar_init(&g_j);
    TEST_ASSERT_EQUAL(1, wf_cookie_set(&g_j, "www.example.com", "/login?x=1", 1, "sid=abc123; Path=/; HttpOnly", NOW));
    TEST_ASSERT_EQUAL(1, wf_cookie_set(&g_j, "www.example.com", "/a/b/page", 1, " lang = fr ", NOW));
    TEST_ASSERT_EQUAL_STRING("Cookie: sid=abc123\r\n", hdr("www.example.com", "/", 1));
    TEST_ASSERT_EQUAL_STRING("Cookie: lang=fr; sid=abc123\r\n", hdr("www.example.com", "/a/b/other", 1));   /* longer path first */
    TEST_ASSERT_EQUAL_STRING("Cookie: sid=abc123\r\n", hdr("www.example.com", "/a/bc", 1));    /* /a/b does not match /a/bc */
    TEST_ASSERT_EQUAL_STRING("", hdr("example.com", "/", 1));                                /* host-only */
    TEST_ASSERT_EQUAL_STRING("", hdr("other.org", "/", 1));
    TEST_ASSERT_EQUAL(2, wf_cookie_count(&g_j));
    TEST_ASSERT_EQUAL(1, wf_cookie_set(&g_j, "www.example.com", "/", 1, "sid=new; Path=/", NOW));
    TEST_ASSERT_EQUAL(1U, g_j.replaced);
    TEST_ASSERT_EQUAL_STRING("Cookie: sid=new\r\n", hdr("www.example.com", "/x", 1));
}

static void test_domain_rules(void) {
    wf_cookie_jar_init(&g_j);
    TEST_ASSERT_EQUAL(1, wf_cookie_set(&g_j, "accounts.example.com", "/", 1, "a=1; Domain=.example.com", NOW));
    TEST_ASSERT_EQUAL_STRING("Cookie: a=1\r\n", hdr("www.example.com", "/", 1));
    TEST_ASSERT_EQUAL_STRING("Cookie: a=1\r\n", hdr("example.com", "/", 1));
    TEST_ASSERT_EQUAL_STRING("", hdr("badexample.com", "/", 1));
    TEST_ASSERT_EQUAL(-1, wf_cookie_set(&g_j, "www.example.com", "/", 1, "b=1; Domain=other.com", NOW));
    TEST_ASSERT_EQUAL(-1, wf_cookie_set(&g_j, "www.example.com", "/", 1, "b=1; Domain=com", NOW));
    TEST_ASSERT_EQUAL(-1, wf_cookie_set(&g_j, "1.2.3.4", "/", 0, "b=1; Domain=2.3.4", NOW));
    TEST_ASSERT_EQUAL(3U, g_j.refused);
}

static void test_secure_and_delete(void) {
    wf_cookie_jar_init(&g_j);
    TEST_ASSERT_EQUAL(-1, wf_cookie_set(&g_j, "s.test", "/", 0, "t=1; Secure", NOW));     /* over http */
    TEST_ASSERT_EQUAL(1, wf_cookie_set(&g_j, "s.test", "/", 1, "t=1; Secure", NOW));
    TEST_ASSERT_EQUAL_STRING("", hdr("s.test", "/", 0));
    TEST_ASSERT_EQUAL_STRING("Cookie: t=1\r\n", hdr("s.test", "/", 1));
    TEST_ASSERT_EQUAL(2, wf_cookie_set(&g_j, "s.test", "/", 1, "t=; Max-Age=0", NOW));
    TEST_ASSERT_EQUAL(0, wf_cookie_count(&g_j));
    TEST_ASSERT_EQUAL(1, wf_cookie_set(&g_j, "s.test", "/", 1, "u=1; Expires=Wed, 21 Oct 2099 07:28:00 GMT", NOW));
    TEST_ASSERT_EQUAL(2, wf_cookie_set(&g_j, "s.test", "/", 1, "u=1; Expires=Thu, 01-Jan-1970 00:00:01 GMT", NOW));
    TEST_ASSERT_EQUAL(0, wf_cookie_set(&g_j, "s.test", "/", 1, "v=1; Max-Age=-5", NOW));   /* nothing to delete */
    TEST_ASSERT_EQUAL(1, wf_cookie_set(&g_j, "s.test", "/", 1, "w=1; Expires=Thu, 01-Jan-1970 00:00:01 GMT", 0));   /* no clock: kept */
}

static void test_bad_input_and_bounds(void) {
    char big[WF_COOKIE_VALUE + 16], list[2048];
    int i;
    wf_cookie_jar_init(&g_j);
    TEST_ASSERT_EQUAL(-1, wf_cookie_set(&g_j, "h.test", "/", 1, "=novalue", NOW));
    TEST_ASSERT_EQUAL(-1, wf_cookie_set(&g_j, "h.test", "/", 1, "noequals", NOW));
    TEST_ASSERT_EQUAL(-1, wf_cookie_set(&g_j, "h.test", "/", 1, "a b=1", NOW));
    TEST_ASSERT_EQUAL(-1, wf_cookie_set(&g_j, "h.test", "/", 1, "a=1\001", NOW));
    memset(big, 'v', sizeof(big)); memcpy(big, "x=", 2); big[sizeof(big) - 1] = 0;
    TEST_ASSERT_EQUAL(-1, wf_cookie_set(&g_j, "h.test", "/", 1, big, NOW));
    for (i = 0; i < WF_COOKIE_MAX + 3; i++) {
        char v[16];
        v[0] = 'c'; v[1] = (char)('a' + i % 26); v[2] = (char)('a' + i / 26); v[3] = '='; v[4] = '1'; v[5] = 0;
        TEST_ASSERT_EQUAL(1, wf_cookie_set(&g_j, "h.test", "/", 1, v, NOW));
    }
    TEST_ASSERT_EQUAL(WF_COOKIE_MAX, wf_cookie_count(&g_j));
    TEST_ASSERT_EQUAL(3U, g_j.evicted);
    TEST_ASSERT_NULL(strstr(hdr("h.test", "/", 1), "caa=1"));    /* the oldest went */
    TEST_ASSERT_NULL(strstr(g_h, "cca=1"));
    TEST_ASSERT_NOT_NULL(strstr(g_h, "cda=1"));
    TEST_ASSERT_EQUAL(-1, wf_cookie_header(&g_j, "h.test", "/", 1, list, 20));   /* too small */
    TEST_ASSERT_EQUAL_STRING("", list);
    wf_cookie_list(&g_j, list, (int)sizeof(list));
    TEST_ASSERT_NOT_NULL(strstr(list, "h.test / cda len=1 host-only\n"));
    TEST_ASSERT_NULL(strstr(list, "cda=1"));                       /* values are not listed */
    TEST_ASSERT_EQUAL(WF_COOKIE_MAX, wf_cookie_clear(&g_j, "h.test"));
    TEST_ASSERT_EQUAL(0, wf_cookie_count(&g_j));
    g_j.enabled = 0;
    TEST_ASSERT_EQUAL(0, wf_cookie_set(&g_j, "h.test", "/", 1, "a=1", NOW));
}

static void test_from_head(void) {
    static const char head[] =
        "HTTP/1.1 302 Found\r\nLocation: /home\r\nset-cookie: a=1; Path=/\r\nContent-Length: 0\r\n"
        "Set-Cookie:b=2\r\nX-Set-Cookie: c=3\r\n\r\n";
    wf_cookie_jar_init(&g_j);
    TEST_ASSERT_EQUAL(2, wf_cookie_from_head(&g_j, "site.test", "/login", 0, head, (int)strlen(head), NOW));
    TEST_ASSERT_EQUAL_STRING("Cookie: a=1; b=2\r\n", hdr("site.test", "/home", 0));
}

int main(void) {
    unity_init();
    RUN_TEST(test_basic_set_and_send);
    RUN_TEST(test_domain_rules);
    RUN_TEST(test_secure_and_delete);
    RUN_TEST(test_bad_input_and_bounds);
    RUN_TEST(test_from_head);
    unity_print_results();
    unity_cleanup();
    return unity_stats.tests_failed == 0 ? 0 : 1;
}
