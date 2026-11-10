/* Lot 89: internationalised host names (wf_idna.c). Expected ASCII forms
 * were taken from Python's punycode codec and the IANA test TLDs. */
#include "../../framework/unity.h"
#include <string.h>
#include <stdint.h>
#include "wf_idna.h"

static char g_o[300];

static const char *ascii(const char *h) {
    if (wf_idna_to_ascii(h, g_o, (int)sizeof(g_o)) != 0) return "ERR";
    return g_o;
}

static const char *uni(const char *h) {
    if (wf_idna_to_unicode(h, g_o, (int)sizeof(g_o)) != 0) return "ERR";
    return g_o;
}

static void test_rfc_labels(void) {
    TEST_ASSERT_EQUAL_STRING("xn--bcher-kva", ascii("b\xc3\xbc" "cher"));
    TEST_ASSERT_EQUAL_STRING("xn--mnchen-3ya", ascii("m\xc3\xbc" "nchen"));
    TEST_ASSERT_EQUAL_STRING("xn--tda", ascii("\xc3\xbc"));
    TEST_ASSERT_EQUAL_STRING("xn--maa-8ma", ascii("ma\xc3\xb1" "a"));
    /* nontransitional: sharp s is kept */
    TEST_ASSERT_EQUAL_STRING("xn--strae-oqa", ascii("stra\xc3\x9f" "e"));
}

static void test_scripts(void) {
    /* Greek, Cyrillic, CJK, kana, hangul (IANA test names) */
    TEST_ASSERT_EQUAL_STRING("xn--hxajbheg2az3al", ascii("\xcf\x80\xce\xb1\xcf\x81\xce\xac\xce\xb4\xce\xb5\xce\xb9\xce\xb3\xce\xbc\xce\xb1"));
    TEST_ASSERT_EQUAL_STRING("xn--e1afmkfd", ascii("\xd0\xbf\xd1\x80\xd0\xb8\xd0\xbc\xd0\xb5\xd1\x80"));
    TEST_ASSERT_EQUAL_STRING("xn--fsqu00a", ascii("\xe4\xbe\x8b\xe5\xad\x90"));
    TEST_ASSERT_EQUAL_STRING("xn--zckzah", ascii("\xe3\x83\x86\xe3\x82\xb9\xe3\x83\x88"));
    TEST_ASSERT_EQUAL_STRING("xn--9t4b11yi5a", ascii("\xed\x85\x8c\xec\x8a\xa4\xed\x8a\xb8"));
    TEST_ASSERT_EQUAL_STRING("xn--80akhbyknj4f", ascii("\xd0\xb8\xd1\x81\xd0\xbf\xd1\x8b\xd1\x82\xd0\xb0\xd0\xbd\xd0\xb8\xd0\xb5"));
}

static void test_case_and_dots(void) {
    /* upper case folds before encoding: Bucher and BUCHER are one host */
    TEST_ASSERT_EQUAL_STRING("www.xn--bcher-kva.example", ascii("WWW.B\xc3\x9c" "CHER.Example"));
    /* Greek and Cyrillic capitals */
    TEST_ASSERT_EQUAL_STRING("xn--e1afmkfd", ascii("\xd0\x9f\xd0\xa0\xd0\x98\xd0\x9c\xd0\x95\xd0\xa0"));
    TEST_ASSERT_EQUAL_STRING("xn--jxalpdlp", ascii("\xce\x94\xce\x9f\xce\x9a\xce\x99\xce\x9c\xce\x89"));
    /* ideographic and full-width full stops are dots */
    TEST_ASSERT_EQUAL_STRING("xn--fsqu00a.xn--zckzah", ascii("\xe4\xbe\x8b\xe5\xad\x90\xe3\x80\x82\xe3\x83\x86\xe3\x82\xb9\xe3\x83\x88"));
    TEST_ASSERT_EQUAL_STRING("a.b", ascii("a\xef\xbc\x8e" "b"));
    TEST_ASSERT_EQUAL_STRING("example.com", ascii("Example.COM"));
}

static void test_refused(void) {
    TEST_ASSERT_EQUAL_STRING("ERR", ascii(""));
    TEST_ASSERT_EQUAL_STRING("ERR", ascii("a..b"));
    TEST_ASSERT_EQUAL_STRING("ERR", ascii("example.com."));
    TEST_ASSERT_EQUAL_STRING("ERR", ascii("a b.com"));
    TEST_ASSERT_EQUAL_STRING("ERR", ascii("a_b.com"));
    TEST_ASSERT_EQUAL_STRING("ERR", ascii("\xc3\x97" "x.com"));          /* multiplication sign */
    TEST_ASSERT_EQUAL_STRING("ERR", ascii("a\xe2\x80\x8b" "b.com"));     /* zero-width space */
    TEST_ASSERT_EQUAL_STRING("ERR", ascii("\xc4\xb0stanbul.tr"));        /* capital I with dot */
    /* bad UTF-8: lone continuation, overlong, surrogate, truncated */
    TEST_ASSERT_EQUAL_STRING("ERR", ascii("\x80" "a.com"));
    TEST_ASSERT_EQUAL_STRING("ERR", ascii("\xc0\xaf" ".com"));
    TEST_ASSERT_EQUAL_STRING("ERR", ascii("\xed\xa0\x80" ".com"));
    TEST_ASSERT_EQUAL_STRING("ERR", ascii("a\xc3"));
    /* a label over 63 ASCII bytes */
    TEST_ASSERT_EQUAL_STRING("ERR", ascii("aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa.com"));
    /* the output buffer too small */
    TEST_ASSERT_EQUAL(-1, wf_idna_to_ascii("b\xc3\xbc" "cher", g_o, 8));
    TEST_ASSERT_EQUAL(-1, wf_idna_to_ascii(0, g_o, 8));
}

static void test_to_unicode(void) {
    TEST_ASSERT_EQUAL_STRING("www.b\xc3\xbc" "cher.example", uni("www.xn--bcher-kva.example"));
    TEST_ASSERT_EQUAL_STRING("www.b\xc3\xbc" "cher.example", uni("WWW.XN--BCHER-KVA.example"));
    TEST_ASSERT_EQUAL_STRING("\xe4\xbe\x8b\xe5\xad\x90", uni("xn--fsqu00a"));
    TEST_ASSERT_EQUAL_STRING("example.com", uni("Example.com"));
    /* does not decode */
    TEST_ASSERT_EQUAL_STRING("ERR", uni("xn--bcher-kv!.example"));
    TEST_ASSERT_EQUAL_STRING("ERR", uni("xn--999999999999.com"));
    /* decodes to an ASCII-only label or to upper case: not canonical */
    TEST_ASSERT_EQUAL_STRING("ERR", uni("xn--abc-.com"));
    TEST_ASSERT_EQUAL_STRING("ERR", uni("a..b"));
    TEST_ASSERT_EQUAL_STRING("ERR", uni("a.b."));
}

static void test_round_trip(void) {
    static const char *hosts[] = {
        "b\xc3\xbc" "cher.example", "\xd0\xbf\xd1\x80\xd0\xb8\xd0\xbc\xd0\xb5\xd1\x80.\xd1\x80\xd1\x84",
        "\xe4\xbe\x8b\xe5\xad\x90.\xe6\xb5\x8b\xe8\xaf\x95", "caf\xc3\xa9.fr", "\xc5\xbe" "lu\xc5\xa5ou\xc4\x8d" "k\xc3\xbd.cz", 0
    };
    char a[300], u[300];
    int i;
    for (i = 0; hosts[i]; i++) {
        TEST_ASSERT_EQUAL(0, wf_idna_to_ascii(hosts[i], a, (int)sizeof(a)));
        TEST_ASSERT_EQUAL(0, wf_idna_to_unicode(a, u, (int)sizeof(u)));
        TEST_ASSERT_EQUAL_STRING(hosts[i], u);
    }
}

static void test_puny_raw(void) {
    uint32_t cp[8] = { 0x4F8B, 0x5B50 }, back[8];
    char o[32];
    TEST_ASSERT_EQUAL(7, wf_puny_encode(cp, 2, o, (int)sizeof(o)));
    TEST_ASSERT_EQUAL_STRING("fsqu00a", o);
    TEST_ASSERT_EQUAL(2, wf_puny_decode("fsqu00a", 7, back, 8));
    TEST_ASSERT_EQUAL(0x4F8B, back[0]);
    TEST_ASSERT_EQUAL(0x5B50, back[1]);
    TEST_ASSERT_EQUAL(-1, wf_puny_encode(cp, 2, o, 4));
    TEST_ASSERT_EQUAL(-1, wf_puny_decode("fsqu00a", 7, back, 1));
    TEST_ASSERT_EQUAL(-1, wf_puny_decode("zzzzzzzzzzzzzzzzzz", 18, back, 8));   /* overflow */
    TEST_ASSERT_EQUAL(1, wf_idna_needs_map("Example.com"));
    TEST_ASSERT_EQUAL(1, wf_idna_needs_map("b\xc3\xbc" "cher"));
    TEST_ASSERT_EQUAL(0, wf_idna_needs_map("xn--bcher-kva.example"));
}

int main(void) {
    unity_init();
    RUN_TEST(test_rfc_labels);
    RUN_TEST(test_scripts);
    RUN_TEST(test_case_and_dots);
    RUN_TEST(test_refused);
    RUN_TEST(test_to_unicode);
    RUN_TEST(test_round_trip);
    RUN_TEST(test_puny_raw);
    unity_print_results();
    unity_cleanup();
    return unity_stats.tests_failed == 0 ? 0 : 1;
}
