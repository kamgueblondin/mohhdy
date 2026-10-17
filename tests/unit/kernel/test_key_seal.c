/* Lot 6 (US-059/060): kernel sealing of the local CA private key. */
#include "../../framework/unity.h"
#include "../../../kernel/key_seal.h"
#include <string.h>

static const uint8_t SK[20] = {1,2,3,4,5,6,7,8,9,10,11,12,13,14,15,16,17,18,19,20};

static void test_no_key_before_init(void) {
    uint8_t out[64];
    keyseal_reset();
    TEST_ASSERT_EQUAL(0, keyseal_ready());
    TEST_ASSERT_EQUAL(KEYSEAL_E_NOKEY, keyseal_seal("shell", SK, 20, out, sizeof(out)));
    TEST_ASSERT_EQUAL(KEYSEAL_E_NOKEY, keyseal_unseal("shell", out, 44, out, sizeof(out)));
}

static void test_roundtrip_hides_plaintext(void) {
    uint8_t blob[64], back[32];
    uint32_t s, u, f;
    int n, i, found = 0;
    keyseal_reset();
    keyseal_init((const uint8_t*)"entropy-a", 9);
    TEST_ASSERT_EQUAL(1, keyseal_ready());
    n = keyseal_seal("shell", SK, 20, blob, sizeof(blob));
    TEST_ASSERT_EQUAL(20 + (int)KEYSEAL_OVERHEAD, n);
    /* the key bytes do not appear in the blob */
    for (i = 0; i + 4 <= n; i++) if (memcmp(blob + i, SK + 4, 4) == 0) found = 1;
    TEST_ASSERT_EQUAL(0, found);
    TEST_ASSERT_EQUAL(20, keyseal_unseal("shell", blob, (uint32_t)n, back, sizeof(back)));
    TEST_ASSERT_EQUAL(0, memcmp(back, SK, 20));
    keyseal_stats(&s, &u, &f);
    TEST_ASSERT_EQUAL(1U, s);
    TEST_ASSERT_EQUAL(1U, u);
    TEST_ASSERT_EQUAL(0U, f);
}

static void test_label_binding_and_tamper(void) {
    uint8_t blob[64], back[32];
    uint32_t f;
    int n;
    keyseal_reset();
    keyseal_init((const uint8_t*)"entropy-b", 9);
    n = keyseal_seal("shell", SK, 20, blob, sizeof(blob));
    TEST_ASSERT_EQUAL(44, n);
    TEST_ASSERT_EQUAL(KEYSEAL_E_AUTH, keyseal_unseal("rogue", blob, (uint32_t)n, back, sizeof(back)));
    blob[10] ^= 1;
    TEST_ASSERT_EQUAL(KEYSEAL_E_AUTH, keyseal_unseal("shell", blob, (uint32_t)n, back, sizeof(back)));
    blob[10] ^= 1;
    blob[n - 1] ^= 0x80;
    TEST_ASSERT_EQUAL(KEYSEAL_E_AUTH, keyseal_unseal("shell", blob, (uint32_t)n, back, sizeof(back)));
    blob[n - 1] ^= 0x80;
    TEST_ASSERT_EQUAL(20, keyseal_unseal("shell", blob, (uint32_t)n, back, sizeof(back)));
    keyseal_stats(0, 0, &f);
    TEST_ASSERT_EQUAL(3U, f);
}

static void test_other_boot_key_cannot_open(void) {
    uint8_t blob[64], back[32];
    int n;
    keyseal_reset();
    keyseal_init((const uint8_t*)"boot-1", 6);
    n = keyseal_seal("shell", SK, 20, blob, sizeof(blob));
    keyseal_reset();
    keyseal_init((const uint8_t*)"boot-2", 6);
    TEST_ASSERT_EQUAL(KEYSEAL_E_AUTH, keyseal_unseal("shell", blob, (uint32_t)n, back, sizeof(back)));
}

static void test_nonce_unique_and_bounds(void) {
    uint8_t a[64], b[64], big[300], back[8];
    keyseal_reset();
    keyseal_init((const uint8_t*)"entropy-c", 9);
    TEST_ASSERT_EQUAL(44, keyseal_seal("shell", SK, 20, a, sizeof(a)));
    TEST_ASSERT_EQUAL(44, keyseal_seal("shell", SK, 20, b, sizeof(b)));
    TEST_ASSERT_NOT_EQUAL(0, memcmp(a, b, 8));
    TEST_ASSERT_NOT_EQUAL(0, memcmp(a + 8, b + 8, 20));
    TEST_ASSERT_EQUAL(KEYSEAL_E_ARG, keyseal_seal("shell", SK, 20, a, 43));
    TEST_ASSERT_EQUAL(KEYSEAL_E_ARG, keyseal_seal("shell", SK, 0, a, sizeof(a)));
    memset(big, 0, sizeof(big));
    TEST_ASSERT_EQUAL(KEYSEAL_E_ARG, keyseal_seal("shell", big, KEYSEAL_MAX_PLAIN + 1U, big, sizeof(big)));
    TEST_ASSERT_EQUAL(KEYSEAL_E_ARG, keyseal_seal("a-label-that-is-way-too-long-for-the-aad", SK, 20, a, sizeof(a)));
    TEST_ASSERT_EQUAL(KEYSEAL_E_ARG, keyseal_unseal("shell", a, 24, back, sizeof(back)));
    TEST_ASSERT_EQUAL(KEYSEAL_E_ARG, keyseal_unseal("shell", a, 44, back, sizeof(back)));
}

int main(void) {
    unity_init();
    RUN_TEST(test_no_key_before_init);
    RUN_TEST(test_roundtrip_hides_plaintext);
    RUN_TEST(test_label_binding_and_tamper);
    RUN_TEST(test_other_boot_key_cannot_open);
    RUN_TEST(test_nonce_unique_and_bounds);
    unity_print_results();
    unity_cleanup();
    return unity_stats.tests_failed == 0 ? 0 : 1;
}
