/* Lot 88: registrable domains (wf_regdom) and autofill shared between the
 * hosts of one site; IP literals, single labels and hosting suffixes never
 * share. */
#include "../../framework/unity.h"
#include "wf_regdom.h"
#include "wf_autofill.h"
#include "wf_formfill.h"
#include <string.h>
#include <stdio.h>

static wf_autofill_t g_a;
static wf_form_page_t g_p;

static void rd(const char *host, const char *want) {
    char out[WF_REGDOM_MAX];
    if (!want) {
        TEST_ASSERT_EQUAL(-1, wf_regdom(host, out, (int)sizeof(out)));
        TEST_ASSERT_EQUAL_STRING("", out);
        return;
    }
    TEST_ASSERT_EQUAL(0, wf_regdom(host, out, (int)sizeof(out)));
    TEST_ASSERT_EQUAL_STRING(want, out);
}

static void test_regdom_rules(void) {
    rd("www.example.com", "example.com");
    rd("a.b.c.example.com", "example.com");
    rd("example.com", "example.com");
    rd("WWW.Example.COM", "example.com");
    rd("news.bbc.co.uk", "bbc.co.uk");
    rd("bbc.co.uk", "bbc.co.uk");
    rd("shop.example.com.au", "example.com.au");
    rd("www.govmu.org", "govmu.org");
    rd("portal.gov.mu", "portal.gov.mu");
    rd("me.github.io", "me.github.io");
    rd("docs.me.github.io", "me.github.io");
}

static void test_regdom_none(void) {
    rd("co.uk", 0);                         /* a public suffix */
    rd("github.io", 0);
    rd("gov.mu", 0);
    rd("com", 0);
    rd("localhost", 0);
    rd("10.0.2.2", 0);
    rd("192.168.1.20", 0);
    rd("[::1]", 0);
    rd("fe80::1", 0);
    rd("", 0);
    rd("bad_host.com", 0);
    rd("a..b.com", 0);
    rd(".example.com", 0);
    rd("example.com.", 0);
    TEST_ASSERT_EQUAL(-1, wf_regdom(0, 0, 0));
    TEST_ASSERT(wf_regdom_is_suffix("co.uk"));
    TEST_ASSERT(wf_regdom_is_suffix("fr"));
    TEST_ASSERT(!wf_regdom_is_suffix("example.fr"));
    TEST_ASSERT(wf_regdom_is_ip("10.0.2.2"));
    TEST_ASSERT(!wf_regdom_is_ip("10.0.2"));
    TEST_ASSERT(!wf_regdom_is_ip("1234.0.0.1"));
    TEST_ASSERT(!wf_regdom_is_ip("123.com"));
}

static void test_same_site(void) {
    TEST_ASSERT(wf_regdom_same_site("www.example.com", "login.example.com"));
    TEST_ASSERT(wf_regdom_same_site("example.com", "m.example.com"));
    TEST_ASSERT(!wf_regdom_same_site("example.com", "example.org"));
    TEST_ASSERT(!wf_regdom_same_site("a.co.uk", "b.co.uk"));
    TEST_ASSERT(!wf_regdom_same_site("alice.github.io", "bob.github.io"));
    TEST_ASSERT(wf_regdom_same_site("www.alice.github.io", "alice.github.io"));
    /* an IP is its own site only when it is the same text */
    TEST_ASSERT(wf_regdom_same_site("10.0.2.2", "10.0.2.2"));
    TEST_ASSERT(!wf_regdom_same_site("10.0.2.2", "10.0.2.3"));
    TEST_ASSERT(!wf_regdom_same_site("localhost", "intranet"));
    TEST_ASSERT(!wf_regdom_same_site("", ""));
}

static void get_site(const char *h, const char *n, int idx, const char *want, int want_other) {
    char v[WF_AF_VALUE];
    int other = -1;
    if (!want) { TEST_ASSERT_EQUAL(-1, wf_autofill_get_site(&g_a, h, n, idx, v, (int)sizeof(v), &other)); return; }
    TEST_ASSERT_EQUAL(0, wf_autofill_get_site(&g_a, h, n, idx, v, (int)sizeof(v), &other));
    TEST_ASSERT_EQUAL_STRING(want, v);
    TEST_ASSERT_EQUAL(want_other, other);
}

static void test_autofill_site_list(void) {
    wf_autofill_init(&g_a);
    (void)wf_autofill_put(&g_a, "login.example.com", "user", "ada", "text");
    (void)wf_autofill_put(&g_a, "login.example.com", "user", "bob", "text");
    (void)wf_autofill_put(&g_a, "www.example.com", "user", "cy", "text");
    (void)wf_autofill_put(&g_a, "www.example.com", "user", "ada", "text");      /* a repeat */
    (void)wf_autofill_put(&g_a, "evil-example.com", "user", "mallory", "text");
    (void)wf_autofill_put(&g_a, "www.example.com", "q", "kernel", "text");
    /* from www: its own two first (newest first), then login's bob */
    TEST_ASSERT_EQUAL(3, wf_autofill_count_site(&g_a, "www.example.com", "user"));
    get_site("www.example.com", "user", 0, "ada", 0);
    get_site("www.example.com", "user", 1, "cy", 0);
    get_site("www.example.com", "user", 2, "bob", 1);
    get_site("www.example.com", "user", 3, 0, 0);
    /* a host of the site with nothing of its own gets the others' */
    get_site("shop.example.com", "user", 0, "bob", 1);
    TEST_ASSERT_EQUAL(3, wf_autofill_count_site(&g_a, "shop.example.com", "user"));
    /* never across sites, never across field names */
    TEST_ASSERT_EQUAL(1, wf_autofill_count_site(&g_a, "evil-example.com", "user"));
    TEST_ASSERT_EQUAL(0, wf_autofill_count_site(&g_a, "example.org", "user"));
    TEST_ASSERT_EQUAL(1, wf_autofill_count_site(&g_a, "login.example.com", "q"));
    TEST_ASSERT_EQUAL(0, wf_autofill_count_site(&g_a, "", "user"));
    TEST_ASSERT_EQUAL(0, wf_autofill_count_site(0, "x.com", "user"));
}

/* Lot 89: internationalised hosts are compared in their ASCII form, so a
 * host typed in UTF-8 and the same host from a redirect share values. */
static void test_regdom_idn(void) {
    char a[WF_REGDOM_MAX];
    rd("www.b\xc3\xbc" "cher.example", "xn--bcher-kva.example");
    rd("shop.B\xc3\x9c" "CHER.example", "xn--bcher-kva.example");
    rd("www.xn--bcher-kva.example", "xn--bcher-kva.example");
    rd("\xd0\xbf\xd1\x80\xd0\xb8\xd0\xbc\xd0\xb5\xd1\x80.co.uk", "xn--e1afmkfd.co.uk");
    rd("\xe4\xbe\x8b\xe5\xad\x90\xe3\x80\x82\xe3\x83\x86\xe3\x82\xb9\xe3\x83\x88", "xn--fsqu00a.xn--zckzah");
    rd("bad\x80host.com", 0);
    rd("a\xe2\x80\x8b" "b.com", 0);
    TEST_ASSERT_EQUAL(1, wf_regdom_same_site("login.b\xc3\xbc" "cher.example", "www.xn--bcher-kva.example"));
    TEST_ASSERT_EQUAL(1, wf_regdom_same_site("B\xc3\x9c" "CHER.example", "xn--bcher-kva.example"));
    TEST_ASSERT_EQUAL(0, wf_regdom_same_site("b\xc3\xbc" "cher.example", "bucher.example"));
    TEST_ASSERT_EQUAL(0, wf_regdom_same_site("b\xc3\xbc" "cher.example", "xn--bcher-kva.example.org"));
    /* a single-label IDN host (no registrable domain) equals its ASCII form */
    TEST_ASSERT_EQUAL(1, wf_regdom_same_site("b\xc3\xbc" "cher", "xn--bcher-kva"));
    TEST_ASSERT_EQUAL(-1, wf_regdom("b\xc3\xbc" "cher", a, (int)sizeof(a)));
}

static void test_autofill_idn_site(void) {
    wf_autofill_init(&g_a);
    (void)wf_autofill_put(&g_a, "login.xn--bcher-kva.example", "user", "ada", "text");
    (void)wf_autofill_put(&g_a, "shop.example", "user", "eve", "text");
    TEST_ASSERT_EQUAL(1, wf_autofill_count_site(&g_a, "www.b\xc3\xbc" "cher.example", "user"));
    get_site("www.b\xc3\xbc" "cher.example", "user", 0, "ada", 1);
    TEST_ASSERT_EQUAL(0, wf_autofill_count_site(&g_a, "www.bucher.example", "user"));
}

static void test_autofill_site_ip_and_hosting(void) {
    wf_autofill_init(&g_a);
    (void)wf_autofill_put(&g_a, "10.0.2.2", "user", "lab", "text");
    (void)wf_autofill_put(&g_a, "alice.github.io", "user", "alice", "text");
    TEST_ASSERT_EQUAL(1, wf_autofill_count_site(&g_a, "10.0.2.2", "user"));
    TEST_ASSERT_EQUAL(0, wf_autofill_count_site(&g_a, "10.0.2.3", "user"));
    TEST_ASSERT_EQUAL(0, wf_autofill_count_site(&g_a, "bob.github.io", "user"));
    TEST_ASSERT_EQUAL(1, wf_autofill_count_site(&g_a, "www.alice.github.io", "user"));
}

static void test_drop_site(void) {
    wf_autofill_init(&g_a);
    (void)wf_autofill_put(&g_a, "login.example.com", "user", "ada", "text");
    (void)wf_autofill_put(&g_a, "www.example.com", "user", "cy", "text");
    /* from www, forget login's ada: it goes from login */
    TEST_ASSERT_EQUAL(0, wf_autofill_drop_site(&g_a, "www.example.com", "user", "ada"));
    TEST_ASSERT_EQUAL(0, wf_autofill_count(&g_a, "login.example.com", "user"));
    TEST_ASSERT_EQUAL(1, wf_autofill_count(&g_a, "www.example.com", "user"));
    TEST_ASSERT_EQUAL(-1, wf_autofill_drop_site(&g_a, "www.example.com", "user", "nobody"));
    /* another site's value is not reachable */
    (void)wf_autofill_put(&g_a, "example.org", "user", "zed", "text");
    TEST_ASSERT_EQUAL(-1, wf_autofill_drop_site(&g_a, "www.example.com", "user", "zed"));
    TEST_ASSERT_EQUAL(1, wf_autofill_count(&g_a, "example.org", "user"));
}

static void test_formfill_offers_site(void) {
    wf_fill_offer_t o[WF_FILL_OFFERS];
    wf_url_t base;
    int n, user, pw;
    wf_autofill_init(&g_a);
    (void)wf_autofill_put(&g_a, "login.example.com", "user", "ada", "text");
    (void)wf_autofill_put(&g_a, "www.example.com", "user", "cy", "text");
    (void)wf_autofill_put(&g_a, "www.example.com", "user", "ada", "text");
    TEST_ASSERT_EQUAL(0, wf_url_parse("https://www.example.com/a", &base));
    wf_form_reset(&g_p);
    TEST_ASSERT(wf_form_begin(&g_p, &base, "/go", "get", "") >= 0);
    user = wf_form_add(&g_p, &base, "text", "user", "", "", 0, 0);
    pw = wf_form_add(&g_p, &base, "password", "user", "", "", 0, 0);
    wf_form_end(&g_p);
    TEST_ASSERT_EQUAL(0, user);
    TEST_ASSERT_EQUAL(1, pw);
    n = wf_formfill_offers(&g_a, &g_p, 0, "https://www.example.com/a", o, WF_FILL_OFFERS);
    /* ada once (www's own), cy, nothing new from login */
    TEST_ASSERT_EQUAL(2, n);
    TEST_ASSERT_EQUAL_STRING("ada", o[0].value);
    TEST_ASSERT_EQUAL(0, o[0].site);
    TEST_ASSERT_EQUAL_STRING("cy", o[1].value);
    n = wf_formfill_offers(&g_a, &g_p, 0, "https://shop.example.com/", o, WF_FILL_OFFERS);
    TEST_ASSERT_EQUAL(2, n);
    TEST_ASSERT_EQUAL(1, o[0].site);
    TEST_ASSERT_EQUAL_STRING("login.example.com", o[0].host);
    TEST_ASSERT_EQUAL_STRING("ada", o[0].value);
    TEST_ASSERT_EQUAL_STRING("www.example.com", o[1].host);
    /* the password field is never offered anything */
    TEST_ASSERT_EQUAL(0, wf_formfill_offers(&g_a, &g_p, 1, "https://www.example.com/a", o, WF_FILL_OFFERS));
    /* another site gets nothing */
    TEST_ASSERT_EQUAL(0, wf_formfill_offers(&g_a, &g_p, 0, "https://example.org/", o, WF_FILL_OFFERS));
}

int main(void) {
    unity_init();
    RUN_TEST(test_regdom_rules);
    RUN_TEST(test_regdom_none);
    RUN_TEST(test_same_site);
    RUN_TEST(test_autofill_site_list);
    RUN_TEST(test_regdom_idn);
    RUN_TEST(test_autofill_idn_site);
    RUN_TEST(test_autofill_site_ip_and_hosting);
    RUN_TEST(test_drop_site);
    RUN_TEST(test_formfill_offers_site);
    unity_print_results();
    unity_cleanup();
    return unity_stats.tests_failed == 0 ? 0 : 1;
}
