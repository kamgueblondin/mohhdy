/* Lot 73: constant-time AES S-box (include/os_aes_ct.h) used by the kernel
 * AES-GCM (kernel/aes_gcm.c) and the Ring 3 TLS AEAD (userspace/wf_aead.c).
 * The S-box is checked on all 256 inputs against an independent reference
 * (inverse in GF(2^8) by exponentiation, then the affine map), every byte
 * lane of the 16- and 4-byte passes is checked, and AES-128/256 and GCM are
 * checked with FIPS-197 and SP 800-38D known answers. */
#include "../../framework/unity.h"
#include "os_aes_ct.h"
#include "wf_aead.h"
#include "../../../kernel/aes_gcm.h"
#include <string.h>
#include <stdio.h>

static uint8_t gf_mul(uint8_t a, uint8_t b) {
    uint8_t r = 0;
    int i;
    for (i = 0; i < 8; i++) {
        if (b & 1) r ^= a;
        a = (uint8_t)((a << 1) ^ ((a & 0x80) ? 0x1b : 0));
        b >>= 1;
    }
    return r;
}

static uint8_t ref_sbox(uint8_t x) {
    uint8_t inv = 1, r;
    int i;
    for (i = 0; i < 254; i++) inv = gf_mul(inv, x);   /* x^254 = x^-1, 0 -> 0 */
    if (x == 0) inv = 0;
    r = inv;
    for (i = 1; i <= 4; i++) r ^= (uint8_t)((inv << i) | (inv >> (8 - i)));
    return (uint8_t)(r ^ 0x63);
}

static void hex(const char* s, uint8_t* out) {
    int i;
    for (i = 0; s[2 * i]; i++) {
        unsigned v;
        sscanf(s + 2 * i, "%2x", &v);
        out[i] = (uint8_t)v;
    }
}

static void test_sbox_all_inputs(void) {
    int x, bad = 0;
    for (x = 0; x < 256; x++)
        if (os_aes_ct_sbox((uint8_t)x) != ref_sbox((uint8_t)x)) bad++;
    TEST_ASSERT_EQUAL(0, bad);
    TEST_ASSERT_EQUAL(0x63, os_aes_ct_sbox(0x00));
    TEST_ASSERT_EQUAL(0xed, os_aes_ct_sbox(0x53));
    TEST_ASSERT_EQUAL(0x16, os_aes_ct_sbox(0xff));
}

static void test_sub16_every_lane(void) {
    uint8_t s[16], want[16];
    int r, i;
    for (r = 0; r < 256; r++) {
        for (i = 0; i < 16; i++) {
            s[i] = (uint8_t)(r * 7 + i * 29 + (i * i) * 3);
            want[i] = ref_sbox(s[i]);
        }
        os_aes_ct_sub16(s);
        TEST_ASSERT_EQUAL_MEMORY(want, s, 16);
    }
}

static void test_sub4_every_lane(void) {
    uint8_t w[4], want[4], keep = 0xa5;
    int r, i;
    uint8_t guard[6];
    for (r = 0; r < 256; r++) {
        for (i = 0; i < 4; i++) { w[i] = (uint8_t)(r + 64 * i); want[i] = ref_sbox(w[i]); }
        os_aes_ct_sub4(w);
        TEST_ASSERT_EQUAL_MEMORY(want, w, 4);
    }
    /* only the 4 bytes are written */
    memset(guard, keep, sizeof(guard));
    guard[1] = 0; guard[2] = 1; guard[3] = 2; guard[4] = 3;
    os_aes_ct_sub4(guard + 1);
    TEST_ASSERT_EQUAL(keep, guard[0]);
    TEST_ASSERT_EQUAL(keep, guard[5]);
    TEST_ASSERT_EQUAL(0x7c, guard[2]);
}

static void test_xtime_all_inputs(void) {
    int x, bad = 0;
    for (x = 0; x < 256; x++)
        if (os_aes_ct_xtime((uint8_t)x) != gf_mul((uint8_t)x, 2)) bad++;
    TEST_ASSERT_EQUAL(0, bad);
}

static void test_wf_aes_fips197_and_key_schedule(void) {
    wf_aead_t c;
    uint8_t key[32], pt[16], out[16], want[16], w[4];
    /* FIPS-197 C.1 and C.3 */
    hex("000102030405060708090a0b0c0d0e0f101112131415161718191a1b1c1d1e1f", key);
    hex("00112233445566778899aabbccddeeff", pt);
    wf_aead_init(&c, WF_AEAD_AES128_GCM, key);
    wf_aes_encrypt_block(&c, pt, out);
    hex("69c4e0d86a7b0430d8cdb78070b4c55a", want);
    TEST_ASSERT_EQUAL_MEMORY(want, out, 16);
    wf_aead_init(&c, WF_AEAD_AES256_GCM, key);
    wf_aes_encrypt_block(&c, pt, out);
    hex("8ea2b7ca516745bfeafc49904b496089", want);
    TEST_ASSERT_EQUAL_MEMORY(want, out, 16);
    /* FIPS-197 A.1: w[43] = b6630ca6; A.3: w[8] = 9ba35411, w[12] = a8b09c1a
     * and w[59] = 706c631e
     * (the extra S-box step of the 256-bit schedule) */
    hex("2b7e151628aed2a6abf7158809cf4f3c", key);
    wf_aead_init(&c, WF_AEAD_AES128_GCM, key);
    hex("b6630ca6", w);
    TEST_ASSERT_EQUAL_MEMORY(w, c.rk + 4 * 43, 4);
    hex("603deb1015ca71be2b73aef0857d77811f352c073b6108d72d9810a30914dff4", key);
    wf_aead_init(&c, WF_AEAD_AES256_GCM, key);
    hex("9ba35411", w);
    TEST_ASSERT_EQUAL_MEMORY(w, c.rk + 4 * 8, 4);
    hex("a8b09c1a", w);
    TEST_ASSERT_EQUAL_MEMORY(w, c.rk + 4 * 12, 4);
    hex("706c631e", w);
    TEST_ASSERT_EQUAL_MEMORY(w, c.rk + 4 * 59, 4);
    /* SP 800-38A F.1.5 ECB-AES256 block 1 */
    hex("6bc1bee22e409f96e93d7e117393172a", pt);
    wf_aes_encrypt_block(&c, pt, out);
    hex("f3eed1bdb5d2a03c064b5a7e3db181f8", want);
    TEST_ASSERT_EQUAL_MEMORY(want, out, 16);
}

static void test_wf_aes256_gcm_nist(void) {
    /* McGrew-Viega GCM test case 14: AES-256, zero key and IV, one zero block */
    wf_aead_t c;
    uint8_t key[32] = {0}, nonce[12] = {0}, pt[16] = {0}, ct[16], tag[16], want[16], back[16];
    wf_aead_init(&c, WF_AEAD_AES256_GCM, key);
    TEST_ASSERT_EQUAL(0, wf_aead_seal(&c, nonce, 0, 0, pt, 16, ct, tag));
    hex("cea7403d4d606b6e074ec5d3baf39d18", want);
    TEST_ASSERT_EQUAL_MEMORY(want, ct, 16);
    hex("d0d1c8a799996bf0265b98b5d48ab919", want);
    TEST_ASSERT_EQUAL_MEMORY(want, tag, 16);
    TEST_ASSERT_EQUAL(0, wf_aead_open(&c, nonce, 0, 0, ct, 16, tag, back));
    TEST_ASSERT_EQUAL_MEMORY(pt, back, 16);
}

static void test_kernel_aes128_fips197(void) {
    aes128_ctx_t c;
    uint8_t key[16], pt[16], out[16], want[16];
    hex("000102030405060708090a0b0c0d0e0f", key);
    hex("00112233445566778899aabbccddeeff", pt);
    TEST_ASSERT_EQUAL(0, aes128_init(&c, key));
    aes128_encrypt_block(&c, pt, out);
    hex("69c4e0d86a7b0430d8cdb78070b4c55a", want);
    TEST_ASSERT_EQUAL_MEMORY(want, out, 16);
    hex("2b7e151628aed2a6abf7158809cf4f3c", key);
    aes128_init(&c, key);
    hex("b6630ca6", want);
    TEST_ASSERT_EQUAL_MEMORY(want, c.round_keys + 172, 4);
}

static void test_kernel_gcm_nist_and_tamper(void) {
    /* McGrew-Viega GCM test cases 2 and 4 (96-bit IV split 4 + 8) */
    uint8_t key[16] = {0}, iv[12] = {0}, pt[64] = {0}, ct[64], tag[16], want[64], back[64], aad[20];
    TEST_ASSERT_EQUAL(0, aes128_gcm_encrypt(key, iv, iv + 4, 0, 0, pt, 16, ct, tag));
    hex("0388dace60b6a392f328c2b971b2fe78", want);
    TEST_ASSERT_EQUAL_MEMORY(want, ct, 16);
    hex("ab6e47d42cec13bdf53a67b21257bddf", want);
    TEST_ASSERT_EQUAL_MEMORY(want, tag, 16);
    hex("feffe9928665731c6d6a8f9467308308", key);
    hex("cafebabefacedbaddecaf888", iv);
    hex("d9313225f88406e5a55909c5aff5269a86a7a9531534f7da2e4c303d8a318a721c3c0c95956809532fcf0e2449a6b525b16aedf5aa0de657ba637b39", pt);
    hex("feedfacedeadbeeffeedfacedeadbeefabaddad2", aad);
    TEST_ASSERT_EQUAL(0, aes128_gcm_encrypt(key, iv, iv + 4, aad, 20, pt, 60, ct, tag));
    hex("42831ec2217774244b7221b784d0d49ce3aa212f2c02a4e035c17e2329aca12e21d514b25466931c7d8f6a5aac84aa051ba30b396a0aac973d58e091", want);
    TEST_ASSERT_EQUAL_MEMORY(want, ct, 60);
    hex("5bc94fbc3221a5db94fae95ae7121a47", want);
    TEST_ASSERT_EQUAL_MEMORY(want, tag, 16);
    TEST_ASSERT_EQUAL(0, aes128_gcm_decrypt(key, iv, iv + 4, aad, 20, ct, 60, tag, back));
    TEST_ASSERT_EQUAL_MEMORY(pt, back, 60);
    tag[15] ^= 1;
    TEST_ASSERT_EQUAL(-3, aes128_gcm_decrypt(key, iv, iv + 4, aad, 20, ct, 60, tag, back));
}

int main(void) {
    unity_init();
    RUN_TEST(test_sbox_all_inputs);
    RUN_TEST(test_sub16_every_lane);
    RUN_TEST(test_sub4_every_lane);
    RUN_TEST(test_xtime_all_inputs);
    RUN_TEST(test_wf_aes_fips197_and_key_schedule);
    RUN_TEST(test_wf_aes256_gcm_nist);
    RUN_TEST(test_kernel_aes128_fips197);
    RUN_TEST(test_kernel_gcm_nist_and_tamper);
    unity_print_results();
    unity_cleanup();
    return (unity_stats.tests_failed == 0) ? 0 : 1;
}
