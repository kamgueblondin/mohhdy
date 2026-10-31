/* test_wf_crypto.c - Lot 67: big numbers, SHA-384/512, ECDSA P-256/P-384
 * and RSA PKCS#1 v1.5 / PSS verification of the Ring 3 web stack, against
 * vectors produced by python cryptography (wf_vectors.h). */
#include "../../framework/unity.h"
#include <string.h>
#include "wf_bn.h"
#include "wf_sha512.h"
#include "wf_ec.h"
#include "wf_rsa.h"
#include "wf_vectors.h"

static const uint8_t E65537[3] = {1, 0, 1};

static void test_sha512_and_384(void) {
    uint8_t out[64];
    static uint8_t a1000[1000];
    wf_sha512((const uint8_t*)"abc", 3, out);
    TEST_ASSERT_EQUAL(0, memcmp(out, sha512_abc, 64));
    memset(a1000, 'a', sizeof(a1000));
    wf_sha384(a1000, sizeof(a1000), out);
    TEST_ASSERT_EQUAL(0, memcmp(out, sha384_long, 48));
    TEST_ASSERT_EQUAL(48, wf_hash(WF_HASH_SHA384, msg, sizeof(msg), out));
    TEST_ASSERT_EQUAL(0, memcmp(out, dig_sha384, 48));
    TEST_ASSERT_EQUAL(32, wf_hash(WF_HASH_SHA256, msg, sizeof(msg), out));
    TEST_ASSERT_EQUAL(0, memcmp(out, dig_sha256, 32));
    TEST_ASSERT_EQUAL(-1, wf_hash(WF_HASH_SHA1, msg, sizeof(msg), out));
}

static void test_bn_mont_exp(void) {
    /* 3^200 mod 1000003 = known by Python: pow(3,200,1000003) */
    wf_bn_t m, a, e, r;
    wf_mont_t ctx;
    uint8_t b[4];
    wf_bn_set_u32(&m, 1000003U);
    wf_bn_set_u32(&a, 3U);
    wf_bn_set_u32(&e, 200U);
    TEST_ASSERT_EQUAL(0, wf_mont_init(&ctx, &m));
    wf_mont_exp(&ctx, &r, &a, &e);
    TEST_ASSERT_EQUAL(0, wf_bn_to_bytes(&r, b, 4));
    TEST_ASSERT_TRUE(WF_TEST_POW3_200 == (uint32_t)((b[0] << 24) | (b[1] << 16) | (b[2] << 8) | b[3]));
    /* inverse modulo a prime: 3 * 3^-1 = 1 */
    wf_mont_inv_prime(&ctx, &r, &a);
    {
        wf_bn_t t, one;
        wf_mont_to(&ctx, &t, &r);
        wf_mont_mul(&ctx, &t, &t, &a);   /* (r R) a / R = r a */
        wf_bn_set_u32(&one, 1U);
        TEST_ASSERT_EQUAL(0, wf_bn_cmp(&t, &one));
    }
    TEST_ASSERT_EQUAL(-1, wf_mont_init(&ctx, &e));   /* even modulus refused */
}

static void test_ecdsa_p256_p384(void) {
    uint8_t bad[sizeof(p256_dig)];
    TEST_ASSERT_EQUAL(0, wf_ecdsa_verify_der(WF_CURVE_P256, p256_pub, sizeof(p256_pub), p256_dig, sizeof(p256_dig), p256_sig, sizeof(p256_sig)));
    TEST_ASSERT_EQUAL(0, wf_ecdsa_verify_der(WF_CURVE_P384, p384_pub, sizeof(p384_pub), p384_dig, sizeof(p384_dig), p384_sig, sizeof(p384_sig)));
    /* P-256 key signing a SHA-384 digest: leftmost 256 bits */
    TEST_ASSERT_EQUAL(0, wf_ecdsa_verify_der(WF_CURVE_P256, p256s384_pub, sizeof(p256s384_pub), p256s384_dig, sizeof(p256s384_dig), p256s384_sig, sizeof(p256s384_sig)));
    memcpy(bad, p256_dig, sizeof(bad));
    bad[5] ^= 1U;
    TEST_ASSERT_TRUE(wf_ecdsa_verify_der(WF_CURVE_P256, p256_pub, sizeof(p256_pub), bad, sizeof(bad), p256_sig, sizeof(p256_sig)) != 0);
    /* wrong key (the P-384 test key is not on P-256) */
    TEST_ASSERT_TRUE(wf_ecdsa_verify_der(WF_CURVE_P256, p256s384_pub, sizeof(p256s384_pub), p256_dig, sizeof(p256_dig), p256_sig, sizeof(p256_sig)) != 0);
    {
        uint8_t off[sizeof(p256_pub)];
        memcpy(off, p256_pub, sizeof(off));
        off[40] ^= 0x10U;
        TEST_ASSERT_EQUAL(-2, wf_ec_point_check(WF_CURVE_P256, off, sizeof(off)));
        TEST_ASSERT_EQUAL(0, wf_ec_point_check(WF_CURVE_P256, p256_pub, sizeof(p256_pub)));
    }
}

static void test_rsa_pkcs1_and_pss(void) {
    uint8_t bad[sizeof(rsa2048_sha256_sig)];
    TEST_ASSERT_EQUAL(0, wf_rsa_pkcs1_verify(rsa2048_n, sizeof(rsa2048_n), E65537, 3, WF_HASH_SHA256, dig_sha256, 32, rsa2048_sha256_sig, sizeof(rsa2048_sha256_sig)));
    TEST_ASSERT_EQUAL(0, wf_rsa_pkcs1_verify(rsa2048_n, sizeof(rsa2048_n), E65537, 3, WF_HASH_SHA384, dig_sha384, 48, rsa2048_sha384_sig, sizeof(rsa2048_sha384_sig)));
    TEST_ASSERT_EQUAL(0, wf_rsa_pkcs1_verify(rsa2048_n, sizeof(rsa2048_n), E65537, 3, WF_HASH_SHA512, dig_sha512, 64, rsa2048_sha512_sig, sizeof(rsa2048_sha512_sig)));
    TEST_ASSERT_EQUAL(0, wf_rsa_pkcs1_verify(rsa4096_n, sizeof(rsa4096_n), E65537, 3, WF_HASH_SHA256, dig_sha256, 32, rsa4096_sha256_sig, sizeof(rsa4096_sha256_sig)));
    TEST_ASSERT_EQUAL(0, wf_rsa_pss_verify(rsa2048_n, sizeof(rsa2048_n), E65537, 3, WF_HASH_SHA256, dig_sha256, 32, rsa2048_pss_sig, sizeof(rsa2048_pss_sig)));
    TEST_ASSERT_EQUAL(0, wf_rsa_pss_verify(rsa4096_n, sizeof(rsa4096_n), E65537, 3, WF_HASH_SHA256, dig_sha256, 32, rsa4096_pss_sig, sizeof(rsa4096_pss_sig)));
    /* a PKCS#1 signature is not a PSS one, and the reverse */
    TEST_ASSERT_TRUE(wf_rsa_pss_verify(rsa2048_n, sizeof(rsa2048_n), E65537, 3, WF_HASH_SHA256, dig_sha256, 32, rsa2048_sha256_sig, sizeof(rsa2048_sha256_sig)) != 0);
    TEST_ASSERT_TRUE(wf_rsa_pkcs1_verify(rsa2048_n, sizeof(rsa2048_n), E65537, 3, WF_HASH_SHA256, dig_sha256, 32, rsa2048_pss_sig, sizeof(rsa2048_pss_sig)) != 0);
    memcpy(bad, rsa2048_sha256_sig, sizeof(bad));
    bad[100] ^= 4U;
    TEST_ASSERT_TRUE(wf_rsa_pkcs1_verify(rsa2048_n, sizeof(rsa2048_n), E65537, 3, WF_HASH_SHA256, dig_sha256, 32, bad, sizeof(bad)) != 0);
    /* digest of another hash with the same key */
    TEST_ASSERT_TRUE(wf_rsa_pkcs1_verify(rsa2048_n, sizeof(rsa2048_n), E65537, 3, WF_HASH_SHA256, dig_sha256, 32, rsa2048_sha384_sig, sizeof(rsa2048_sha384_sig)) != 0);
    /* wrong key size for the signature */
    TEST_ASSERT_TRUE(wf_rsa_pkcs1_verify(rsa4096_n, sizeof(rsa4096_n), E65537, 3, WF_HASH_SHA256, dig_sha256, 32, rsa2048_sha256_sig, sizeof(rsa2048_sha256_sig)) != 0);
}

int main(void) {
    unity_init();
    RUN_TEST(test_sha512_and_384);
    RUN_TEST(test_bn_mont_exp);
    RUN_TEST(test_ecdsa_p256_p384);
    RUN_TEST(test_rsa_pkcs1_and_pss);
    unity_print_results();
    unity_cleanup();
    return (unity_stats.tests_failed == 0) ? 0 : 1;
}
