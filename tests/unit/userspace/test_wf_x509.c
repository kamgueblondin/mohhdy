/* test_wf_x509.c - Lot 67: X.509 parsing, host names, PEM root store and
 * chain validation against a throw-away test PKI (wf_x509_fixtures.h), the
 * real example.com chain and the production root bundle. */
#include "../../framework/unity.h"
#include <stdio.h>
#include <string.h>
#include "wf_x509.h"
#include "wf_x509_fixtures.h"
#include "wf_real_chain.h"

#define NOW "20260601120000"
static wf_trust_t g_trust;
static uint8_t g_der[64 * 1024];

static void trust_test_root(void) {
    memset(&g_trust, 0, sizeof(g_trust));
    TEST_ASSERT_EQUAL(0, wf_x509_parse(fx_root, sizeof(fx_root), &g_trust.roots[0]));
    g_trust.count = 1;
}

static void test_parse_fields(void) {
    wf_cert_t c;
    TEST_ASSERT_EQUAL(0, wf_x509_parse(fx_leaf, sizeof(fx_leaf), &c));
    TEST_ASSERT_EQUAL(3, c.version);
    TEST_ASSERT_EQUAL(WF_KEY_EC_P256, c.key_type);
    TEST_ASSERT_EQUAL(WF_SIG_RSA_SHA256, c.sig_alg);
    TEST_ASSERT_EQUAL(0, strcmp(c.cn, "mohhdy.test"));
    TEST_ASSERT_EQUAL(2, c.san_count);
    TEST_ASSERT_EQUAL(1, c.san_ip_count);
    TEST_ASSERT_EQUAL(0, c.is_ca);
    TEST_ASSERT_EQUAL(0, strcmp(c.not_before, "20260101000000"));
    TEST_ASSERT_EQUAL(0, wf_x509_parse(fx_inter, sizeof(fx_inter), &c));
    TEST_ASSERT_EQUAL(WF_KEY_RSA, c.key_type);
    TEST_ASSERT_EQUAL(WF_SIG_ECDSA_SHA384, c.sig_alg);
    TEST_ASSERT_EQUAL(1, c.is_ca);
    TEST_ASSERT_EQUAL(0, c.path_len);
    TEST_ASSERT_EQUAL(1, c.key_cert_sign);
    TEST_ASSERT_EQUAL(0, wf_x509_parse(fx_root, sizeof(fx_root), &c));
    TEST_ASSERT_EQUAL(WF_KEY_EC_P384, c.key_type);
    TEST_ASSERT_EQUAL(0, wf_x509_parse(fx_leaf_pss, sizeof(fx_leaf_pss), &c));
    TEST_ASSERT_EQUAL(WF_SIG_RSA_PSS_SHA256, c.sig_alg);
    TEST_ASSERT_EQUAL(0, wf_x509_parse(fx_crit, sizeof(fx_crit), &c));
    TEST_ASSERT_EQUAL(1, c.unknown_critical);
}

static void test_parse_rejects_garbage(void) {
    wf_cert_t c;
    static uint8_t buf[sizeof(fx_leaf)];
    int i, bad = 0;
    TEST_ASSERT_TRUE(wf_x509_parse(fx_leaf, sizeof(fx_leaf) - 1, &c) != 0);
    TEST_ASSERT_TRUE(wf_x509_parse(fx_leaf, 3, &c) != 0);
    /* every truncation fails cleanly (no read past the buffer under ASan) */
    for (i = 0; i < (int)sizeof(fx_leaf); i += 7) {
        memcpy(buf, fx_leaf, (size_t)i);
        if (wf_x509_parse(buf, i, &c) != 0) bad++;
    }
    TEST_ASSERT_EQUAL(((int)sizeof(fx_leaf) + 6) / 7, bad);
    /* long-form length that is not minimal */
    {
        static const uint8_t nm[] = {0x30, 0x81, 0x01, 0x00};
        int pos = 0;
        wf_der_t el;
        TEST_ASSERT_EQUAL(-1, wf_der_next(nm, sizeof(nm), &pos, &el));
    }
}

static void test_host_match(void) {
    wf_cert_t c;
    TEST_ASSERT_EQUAL(0, wf_x509_parse(fx_leaf, sizeof(fx_leaf), &c));
    TEST_ASSERT_EQUAL(0, wf_x509_host_match(&c, "mohhdy.test"));
    TEST_ASSERT_EQUAL(0, wf_x509_host_match(&c, "MOHHDY.Test."));
    TEST_ASSERT_EQUAL(0, wf_x509_host_match(&c, "www.mohhdy.test"));
    TEST_ASSERT_EQUAL(-1, wf_x509_host_match(&c, "a.b.mohhdy.test"));
    TEST_ASSERT_EQUAL(-1, wf_x509_host_match(&c, "evilmohhdy.test"));
    TEST_ASSERT_EQUAL(-1, wf_x509_host_match(&c, "mohhdy.test.evil"));
    TEST_ASSERT_EQUAL(-1, wf_x509_host_match(&c, ".mohhdy.test"));
    TEST_ASSERT_EQUAL(0, wf_x509_host_match(&c, "10.0.2.2"));
    TEST_ASSERT_EQUAL(-1, wf_x509_host_match(&c, "10.0.2.3"));
    TEST_ASSERT_EQUAL(-1, wf_x509_host_match(&c, ""));
}

static void test_chain_ok(void) {
    wf_cert_t ch[2];
    wf_chain_info_t info;
    trust_test_root();
    TEST_ASSERT_EQUAL(0, wf_x509_parse(fx_leaf, sizeof(fx_leaf), &ch[0]));
    TEST_ASSERT_EQUAL(0, wf_x509_parse(fx_inter, sizeof(fx_inter), &ch[1]));
    TEST_ASSERT_EQUAL(WF_CHAIN_OK, wf_chain_verify(ch, 2, &g_trust, "mohhdy.test", NOW, &info));
    TEST_ASSERT_EQUAL(3, info.depth);
    TEST_ASSERT_EQUAL(1, info.time_checked);
    TEST_ASSERT_EQUAL(0, strcmp(info.anchor_cn, "MOHHDY Test Root"));
    /* no clock: dates skipped, and the result says so */
    TEST_ASSERT_EQUAL(WF_CHAIN_OK, wf_chain_verify(ch, 2, &g_trust, "mohhdy.test", 0, &info));
    TEST_ASSERT_EQUAL(0, info.time_checked);
    /* PSS signed leaf */
    TEST_ASSERT_EQUAL(0, wf_x509_parse(fx_leaf_pss, sizeof(fx_leaf_pss), &ch[0]));
    TEST_ASSERT_EQUAL(WF_CHAIN_OK, wf_chain_verify(ch, 2, &g_trust, "pss.mohhdy.test", NOW, &info));
}

static void test_chain_failures(void) {
    wf_cert_t ch[3];
    wf_trust_t empty;
    trust_test_root();
    TEST_ASSERT_EQUAL(0, wf_x509_parse(fx_inter, sizeof(fx_inter), &ch[1]));
    TEST_ASSERT_EQUAL(0, wf_x509_parse(fx_leaf, sizeof(fx_leaf), &ch[0]));
    TEST_ASSERT_EQUAL(WF_CHAIN_E_HOST, wf_chain_verify(ch, 2, &g_trust, "example.com", NOW, 0));
    TEST_ASSERT_EQUAL(WF_CHAIN_E_UNTRUSTED, wf_chain_verify(ch, 1, &g_trust, "mohhdy.test", NOW, 0));
    memset(&empty, 0, sizeof(empty));
    TEST_ASSERT_EQUAL(WF_CHAIN_E_UNTRUSTED, wf_chain_verify(ch, 2, &empty, "mohhdy.test", NOW, 0));
    TEST_ASSERT_EQUAL(WF_CHAIN_E_NOT_YET, wf_chain_verify(ch, 2, &g_trust, "mohhdy.test", "20251231235959", 0));
    TEST_ASSERT_EQUAL(0, wf_x509_parse(fx_expired, sizeof(fx_expired), &ch[0]));
    TEST_ASSERT_EQUAL(WF_CHAIN_E_EXPIRED, wf_chain_verify(ch, 2, &g_trust, "old.mohhdy.test", NOW, 0));
    TEST_ASSERT_EQUAL(0, wf_x509_parse(fx_future, sizeof(fx_future), &ch[0]));
    TEST_ASSERT_EQUAL(WF_CHAIN_E_NOT_YET, wf_chain_verify(ch, 2, &g_trust, "new.mohhdy.test", NOW, 0));
    TEST_ASSERT_EQUAL(0, wf_x509_parse(fx_crit, sizeof(fx_crit), &ch[0]));
    TEST_ASSERT_EQUAL(WF_CHAIN_E_CRITICAL, wf_chain_verify(ch, 2, &g_trust, "crit.mohhdy.test", NOW, 0));
    TEST_ASSERT_EQUAL(0, wf_x509_parse(fx_forged, sizeof(fx_forged), &ch[0]));
    TEST_ASSERT_EQUAL(WF_CHAIN_E_SIGNATURE, wf_chain_verify(ch, 2, &g_trust, "mohhdy.test", NOW, 0));
    /* CA:false issuer */
    TEST_ASSERT_EQUAL(0, wf_x509_parse(fx_noca_leaf, sizeof(fx_noca_leaf), &ch[0]));
    TEST_ASSERT_EQUAL(0, wf_x509_parse(fx_noca, sizeof(fx_noca), &ch[1]));
    TEST_ASSERT_EQUAL(WF_CHAIN_E_NOT_CA, wf_chain_verify(ch, 2, &g_trust, "x.mohhdy.test", NOW, 0));
    /* pathlen 0 intermediate above another CA */
    TEST_ASSERT_EQUAL(0, wf_x509_parse(fx_sub_leaf, sizeof(fx_sub_leaf), &ch[0]));
    TEST_ASSERT_EQUAL(0, wf_x509_parse(fx_sub, sizeof(fx_sub), &ch[1]));
    TEST_ASSERT_EQUAL(0, wf_x509_parse(fx_inter, sizeof(fx_inter), &ch[2]));
    TEST_ASSERT_EQUAL(WF_CHAIN_E_PATHLEN, wf_chain_verify(ch, 3, &g_trust, "deep.mohhdy.test", NOW, 0));
    TEST_ASSERT_EQUAL(WF_CHAIN_E_EMPTY, wf_chain_verify(ch, 0, &g_trust, "x", NOW, 0));
}

static char* slurp(const char* path, int* len) {
    FILE* f = fopen(path, "rb");
    static char b[128 * 1024];
    size_t n;
    if (!f) return 0;
    n = fread(b, 1, sizeof(b) - 1, f);
    fclose(f);
    *len = (int)n;
    return b;
}

static void test_real_chain_with_production_roots(void) {
    wf_cert_t ch[RC_COUNT];
    const unsigned char* d[4] = {rc_0, rc_1, rc_2, rc_3};
    const int l[4] = {(int)sizeof(rc_0), (int)sizeof(rc_1), (int)sizeof(rc_2), (int)sizeof(rc_3)};
    wf_chain_info_t info;
    int len = 0, used = 0, i, n;
    char* pem = slurp("../initrd_content/etc/ca-roots.pem", &len);
    TEST_ASSERT_TRUE(pem != 0);
    memset(&g_trust, 0, sizeof(g_trust));
    n = wf_trust_add_pem(&g_trust, pem, len, g_der, (int)sizeof(g_der), &used);
    printf("    roots: %d parsed, %d skipped, %d bytes DER\n", n, g_trust.skipped, used);
    TEST_ASSERT_TRUE(n >= 30);
    TEST_ASSERT_EQUAL(0, g_trust.skipped);
    for (i = 0; i < RC_COUNT; i++) TEST_ASSERT_EQUAL(0, wf_x509_parse(d[i], l[i], &ch[i]));
    TEST_ASSERT_EQUAL(WF_CHAIN_OK, wf_chain_verify(ch, RC_COUNT, &g_trust, "example.com", "20261101000000", &info));
    printf("    example.com -> %s (depth %d)\n", info.anchor_cn, info.depth);
    TEST_ASSERT_EQUAL(WF_CHAIN_E_HOST, wf_chain_verify(ch, RC_COUNT, &g_trust, "example.org.evil", "20261101000000", 0));
    TEST_ASSERT_EQUAL(WF_CHAIN_E_EXPIRED, wf_chain_verify(ch, RC_COUNT, &g_trust, "example.com", "20270601000000", 0));
}

static void test_base64(void) {
    uint8_t out[8];
    TEST_ASSERT_EQUAL(3, wf_base64_decode("TWFu", 4, out, 8));
    TEST_ASSERT_EQUAL(0, memcmp(out, "Man", 3));
    TEST_ASSERT_EQUAL(2, wf_base64_decode("TW\nE=", 5, out, 8));
    TEST_ASSERT_EQUAL(-1, wf_base64_decode("TW=E", 4, out, 8));
    TEST_ASSERT_EQUAL(-1, wf_base64_decode("TWFuTWFuTWFu", 12, out, 8));
}

int main(void) {
    unity_init();
    RUN_TEST(test_parse_fields);
    RUN_TEST(test_parse_rejects_garbage);
    RUN_TEST(test_host_match);
    RUN_TEST(test_chain_ok);
    RUN_TEST(test_chain_failures);
    RUN_TEST(test_real_chain_with_production_roots);
    RUN_TEST(test_base64);
    unity_print_results();
    unity_cleanup();
    return (unity_stats.tests_failed == 0) ? 0 : 1;
}
