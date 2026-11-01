/* Lot 72: AES-128/256-GCM and ChaCha20-Poly1305 of the Ring 3 TLS client
 * (wf_aead.c): FIPS-197 / RFC 8439 known answers and 30 vectors made with
 * OpenSSL (scripts/gen_aead_vectors.py), tampering refused. */
#include "../../framework/unity.h"
#include "wf_aead.h"
#include "wf_aead_vectors.h"
#include <string.h>
#include <stdio.h>

static wf_aead_t g_c;
static uint8_t g_buf[1024], g_buf2[1024];

static void hex(const char* s, uint8_t* out) {
    int i;
    for (i = 0; s[2 * i]; i++) {
        unsigned v;
        sscanf(s + 2 * i, "%2x", &v);
        out[i] = (uint8_t)v;
    }
}

static void test_aes_fips197(void) {
    uint8_t key[32], pt[16], out[16], want[16];
    hex("000102030405060708090a0b0c0d0e0f101112131415161718191a1b1c1d1e1f", key);
    hex("00112233445566778899aabbccddeeff", pt);
    TEST_ASSERT_EQUAL(0, wf_aead_init(&g_c, WF_AEAD_AES128_GCM, key));
    wf_aes_encrypt_block(&g_c, pt, out);
    hex("69c4e0d86a7b0430d8cdb78070b4c55a", want);
    TEST_ASSERT_EQUAL_MEMORY(want, out, 16);
    TEST_ASSERT_EQUAL(0, wf_aead_init(&g_c, WF_AEAD_AES256_GCM, key));
    wf_aes_encrypt_block(&g_c, pt, out);
    hex("8ea2b7ca516745bfeafc49904b496089", want);
    TEST_ASSERT_EQUAL_MEMORY(want, out, 16);
    TEST_ASSERT_EQUAL(14, g_c.rounds);
}

static void test_chacha20_rfc8439_block(void) {
    uint8_t key[32], nonce[12], out[64], want[64];
    hex("000102030405060708090a0b0c0d0e0f101112131415161718191a1b1c1d1e1f", key);
    hex("000000090000004a00000000", nonce);
    wf_chacha20_block(key, 1, nonce, out);
    hex("10f1e7e4d13b5915500fdd1fa32071c4c7d1f4c733c068030422aa9ac3d46c4e"
        "d2826446079faa0914c2d705d98b02a2b5129cd1de164eb9cbd083e8a2503c4e", want);
    TEST_ASSERT_EQUAL_MEMORY(want, out, 64);
}

static void test_poly1305_rfc8439(void) {
    uint8_t key[32], tag[16], want[16];
    const char* msg = "Cryptographic Forum Research Group";
    hex("85d6be7857556d337f4452fe42d506a80103808afb0db2fd4abff6af4149f51b", key);
    wf_poly1305(key, (const uint8_t*)msg, (int)strlen(msg), tag);
    hex("a8061dc1305136c6c22b8baf0c0127a9", want);
    TEST_ASSERT_EQUAL_MEMORY(want, tag, 16);
}

static void test_chapoly_rfc8439_aead(void) {
    uint8_t key[32], nonce[12], aad[12], tag[16], want[16];
    const char* pt = "Ladies and Gentlemen of the class of '99: If I could offer you only one tip for the future, sunscreen would be it.";
    int n = (int)strlen(pt);
    hex("808182838485868788898a8b8c8d8e8f909192939495969798999a9b9c9d9e9f", key);
    hex("070000004041424344454647", nonce);
    hex("50515253c0c1c2c3c4c5c6c7", aad);
    TEST_ASSERT_EQUAL(0, wf_aead_init(&g_c, WF_AEAD_CHACHA20_POLY1305, key));
    TEST_ASSERT_EQUAL(0, wf_aead_seal(&g_c, nonce, aad, 12, (const uint8_t*)pt, n, g_buf, tag));
    hex("d31a8d34648e60db7b86afbc53ef7ec2", want);
    TEST_ASSERT_EQUAL_MEMORY(want, g_buf, 16);
    hex("1ae10b594f09e26a7e902ecbd0600691", want);
    TEST_ASSERT_EQUAL_MEMORY(want, tag, 16);
    TEST_ASSERT_EQUAL(0, wf_aead_open(&g_c, nonce, aad, 12, g_buf, n, tag, g_buf2));
    TEST_ASSERT_EQUAL_MEMORY(pt, g_buf2, n);
}

static void test_openssl_vectors(void) {
    int i, k;
    int count[4] = {0, 0, 0, 0};
    for (i = 0; i < (int)(sizeof(AEAD_VECS) / sizeof(AEAD_VECS[0])); i++) {
        const aead_vec_t* v = &AEAD_VECS[i];
        uint8_t tag[16], bad[16];
        TEST_ASSERT_EQUAL(v->kl, wf_aead_key_len(v->alg));
        TEST_ASSERT_EQUAL(0, wf_aead_init(&g_c, v->alg, v->key));
        TEST_ASSERT_EQUAL(0, wf_aead_seal(&g_c, v->nonce, v->aad, v->al, v->pt, v->pl, g_buf, tag));
        if (v->pl) TEST_ASSERT_EQUAL_MEMORY(v->ct, g_buf, v->pl);
        TEST_ASSERT_EQUAL_MEMORY(v->tag, tag, 16);
        memset(g_buf2, 0xAA, sizeof(g_buf2));
        TEST_ASSERT_EQUAL(0, wf_aead_open(&g_c, v->nonce, v->aad, v->al, v->ct, v->pl, v->tag, g_buf2));
        if (v->pl) TEST_ASSERT_EQUAL_MEMORY(v->pt, g_buf2, v->pl);
        /* in place */
        memcpy(g_buf, v->pt, (size_t)v->pl);
        TEST_ASSERT_EQUAL(0, wf_aead_seal(&g_c, v->nonce, v->aad, v->al, g_buf, v->pl, g_buf, tag));
        TEST_ASSERT_EQUAL(0, wf_aead_open(&g_c, v->nonce, v->aad, v->al, g_buf, v->pl, tag, g_buf));
        if (v->pl) TEST_ASSERT_EQUAL_MEMORY(v->pt, g_buf, v->pl);
        /* any flipped bit in tag, ciphertext or aad is refused and nothing is written */
        memcpy(bad, v->tag, 16); bad[i % 16] ^= 0x01;
        memset(g_buf2, 0x55, 64);
        TEST_ASSERT_EQUAL(-1, wf_aead_open(&g_c, v->nonce, v->aad, v->al, v->ct, v->pl, bad, g_buf2));
        for (k = 0; k < 64; k++) TEST_ASSERT_EQUAL(0x55, g_buf2[k]);
        if (v->pl) {
            memcpy(g_buf, v->ct, (size_t)v->pl);
            g_buf[v->pl - 1] ^= 0x80;
            TEST_ASSERT_EQUAL(-1, wf_aead_open(&g_c, v->nonce, v->aad, v->al, g_buf, v->pl, v->tag, g_buf2));
        }
        if (v->al) {
            uint8_t aad[32];
            memcpy(aad, v->aad, (size_t)v->al);
            aad[0] ^= 0x04;
            TEST_ASSERT_EQUAL(-1, wf_aead_open(&g_c, v->nonce, aad, v->al, v->ct, v->pl, v->tag, g_buf2));
        }
        count[v->alg]++;
    }
    TEST_ASSERT_EQUAL(10, count[1]);
    TEST_ASSERT_EQUAL(10, count[2]);
    TEST_ASSERT_EQUAL(10, count[3]);
}

static void test_unknown_alg(void) {
    uint8_t key[32] = {0}, tag[16];
    TEST_ASSERT_EQUAL(-1, wf_aead_init(&g_c, 9, key));
    TEST_ASSERT_EQUAL(-1, wf_aead_seal(&g_c, key, 0, 0, key, 1, g_buf, tag));
    TEST_ASSERT_EQUAL(0, wf_aead_key_len(9));
    TEST_ASSERT_EQUAL_STRING("CHACHA20-POLY1305", wf_aead_name(WF_AEAD_CHACHA20_POLY1305));
    wf_aead_init(&g_c, WF_AEAD_AES256_GCM, key);
    wf_aead_wipe(&g_c);
    TEST_ASSERT_EQUAL(0, g_c.alg);
}

int main(void) {
    unity_init();
    RUN_TEST(test_aes_fips197);
    RUN_TEST(test_chacha20_rfc8439_block);
    RUN_TEST(test_poly1305_rfc8439);
    RUN_TEST(test_chapoly_rfc8439_aead);
    RUN_TEST(test_openssl_vectors);
    RUN_TEST(test_unknown_alg);
    unity_print_results();
    unity_cleanup();
    return (unity_stats.tests_failed == 0) ? 0 : 1;
}
