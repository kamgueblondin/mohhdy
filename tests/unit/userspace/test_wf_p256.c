/* Lot 72: constant-time P-256 (wf_p256.c): base point and ECDH answers
 * made with OpenSSL (Python cryptography), edge scalars 1 and n-1, bad
 * scalars and off-curve points refused, and the same number of field
 * multiplications for every scalar (no secret-dependent work). */
#include "../../framework/unity.h"
#include "wf_p256.h"
#include "wf_ec.h"
#include <string.h>
#include <stdio.h>

typedef struct { const char *k, *pub, *peer, *shared; } p256_vec_t;
static const p256_vec_t VECS[] = {
    {"ffffffff00000000ffffffffffffffffbce6faada7179e84f3b9cac2fc632550", "046b17d1f2e12c4247f8bce6e563a440f277037d812deb33a0f4a13945d898c296b01cbd1c01e58065711814b583f061e9d431cca994cea1313449bf97c840ae0a", "0446eccbe9fa9b957f1a338c341e529480453f04f742bf4af271c2819731389a786212760b85334c16ca8d8d36031b726385a7b2e872156d0fb76406a49606b397", "46eccbe9fa9b957f1a338c341e529480453f04f742bf4af271c2819731389a78"},
    {"0000000000000000000000000000000000000000000000000000000000000001", "046b17d1f2e12c4247f8bce6e563a440f277037d812deb33a0f4a13945d898c2964fe342e2fe1a7f9b8ee7eb4a7c0f9e162bce33576b315ececbb6406837bf51f5", "04a8456063ce46558f17d782609d65eea2825c35c090762c581c89fa9a35959d4c8a8564d84105daed2478419f2f7577755b608868472376d0babee06126465a96", "a8456063ce46558f17d782609d65eea2825c35c090762c581c89fa9a35959d4c"},
    {"8000000000000000000000000000000000000000000000000000000000003039", "0437936c6a2b0125cf9dbe930274075817c7fa3cdbadecb85204292502d88d17bc16b9fd9da1b864148efae6a94264737e67d8a7f83f762ee7084f9548c0302eb4", "04467d9adc23cffe297b5743b6a8f6a38154c3e9efcc5bf3477004e9987753706961f34c1c7f637519378cb5076f8285f714f4deb6338550b43785ff38cb1fd056", "27921d5f853dff7b3f7b62fc8a07412553ca33d710c67ee2d9867af948720806"},
    {"5e0466a76c3472ad2271615630ce9ba502f93eb042e9c091a7d0ba3f0605fca3", "045d2f90b6ed4388cb7f5449737072de4507b58bc49cae4a48abc5fe669843e62a2b022fdae2d569dfb6f4670aa2f889e8da5b177d1ef40d84f3673b6df738dc5f", "047aaeb00b480fbd3965a16dd9d0391213ab3e09b929c32684435b01ab054f5f4755a407c2d7a0e171255ce4807196c02c3652c8da44db075eebb6e1c1603d8e91", "fe6f99752946bf4f15c164a74cd703d82815eee8f4ecd57db30ec2853ba21433"},
    {"4121f6bb4fd3386e8af6c3d82958f8e113b4655fe0629f22f3f763a8c9cc3e8b", "046497fb95d95ef76faf7cfee38ad425d4db6eeaf1f721c57ad7bb5c36e62c586ed3390cbcffca4d374cd01be8edde9c9599c7379803fea8b1cb0d509ff3cd763f", "04f1ef0b9cf136ca3cc32c0b5c5945b8d6788ec3e1495f7a6ba9a1cdf6447535371b86696998710c5948a6a725fd78d4c1a6beec18be23bf79beda4ea7b7f4e02d", "96e29000a286cede1e35abf64ed03a1270053940463c2809f13e81eb4285f8ad"},
    {"101d3e1b03d7fc1abef23d456d8c72afa9577aee91ae5d80d346ca86eee02c32", "04a6357c9acb865e17258eba204ecdded4bc23ddf894f3ccbd0786c4e5088f45d04a567eb16160dcce9ff3244a7f61d912fe0d2a4d59e096d6bd44a693cb2c8209", "04958c946d09ed45b8669ba17f266c63658542bff5147bc7c0088d18aee6b7e8ae759b3d960936144783eccedb0243ffe24cff1cb3bc4b7c758683791f496450f1", "e15860baed5af52474478b78c70c331eba910e89bacd74edbfc8eb01b37031d4"},
};

static int unhex(const char* s, uint8_t* out) {
    int i;
    for (i = 0; s[2 * i]; i++) { unsigned v; sscanf(s + 2 * i, "%2x", &v); out[i] = (uint8_t)v; }
    return i;
}

static void test_vectors_base_and_ecdh(void) {
    int i;
    for (i = 0; i < (int)(sizeof(VECS) / sizeof(VECS[0])); i++) {
        uint8_t k[32], pub[65], peer[65], sh[32], out[65], pub2[65], sh2[32];
        unhex(VECS[i].k, k); unhex(VECS[i].pub, pub); unhex(VECS[i].peer, peer); unhex(VECS[i].shared, sh);
        TEST_ASSERT_EQUAL(0, wf_p256_mul(k, 0, 0, out));
        TEST_ASSERT_EQUAL_MEMORY(pub, out, 65);
        TEST_ASSERT_EQUAL(0, wf_p256_mul(k, peer, 65, out));
        TEST_ASSERT_EQUAL_MEMORY(sh, out + 1, 32);
        /* the TLS entry point uses it */
        TEST_ASSERT_EQUAL(0, wf_ecdh_p256(k, peer, 65, pub2, sh2));
        TEST_ASSERT_EQUAL_MEMORY(pub, pub2, 65);
        TEST_ASSERT_EQUAL_MEMORY(sh, sh2, 32);
    }
}

static void test_bad_inputs(void) {
    uint8_t k[32] = {0}, out[65], g[65];
    unhex(VECS[1].pub, g);                                    /* G */
    TEST_ASSERT_EQUAL(-1, wf_p256_mul(k, 0, 0, out));         /* 0 */
    unhex("ffffffff00000000ffffffffffffffffbce6faada7179e84f3b9cac2fc632551", k);   /* n */
    TEST_ASSERT_EQUAL(-1, wf_p256_mul(k, 0, 0, out));
    memset(k, 0xff, 32);
    TEST_ASSERT_EQUAL(-1, wf_p256_mul(k, 0, 0, out));
    k[0] = 1; memset(k + 1, 0, 31);
    g[64] ^= 1;                                               /* off the curve */
    TEST_ASSERT_EQUAL(-2, wf_p256_mul(k, g, 65, out));
    g[64] ^= 1;
    TEST_ASSERT_EQUAL(-2, wf_p256_mul(k, g, 64, out));
    g[0] = 2;
    TEST_ASSERT_EQUAL(-2, wf_p256_mul(k, g, 65, out));
    g[0] = 4;
    memset(g + 1, 0xff, 32);                                  /* x >= p */
    TEST_ASSERT_EQUAL(-2, wf_p256_mul(k, g, 65, out));
}

static void test_constant_work(void) {
    static const char* ks[] = {
        "0000000000000000000000000000000000000000000000000000000000000001",
        "ffffffff00000000ffffffffffffffffbce6faada7179e84f3b9cac2fc632550",
        "8000000000000000000000000000000000000000000000000000000000000000",
        "00000000000000000000000000000000ffffffffffffffffffffffffffffffff",
        "5555555555555555555555555555555555555555555555555555555555555555"};
    uint8_t k[32], out[65], peer[65];
    uint32_t first = 0;
    int i;
    unhex(VECS[3].peer, peer);
    for (i = 0; i < 5; i++) {
        unhex(ks[i], k);
        wf_p256_mul_count_reset();
        TEST_ASSERT_EQUAL(0, wf_p256_mul(k, peer, 65, out));
        if (i == 0) first = wf_p256_mul_count();
        TEST_ASSERT_EQUAL(first, wf_p256_mul_count());
    }
    TEST_ASSERT_TRUE(first > 256 * 2 * 12);
}

int main(void) {
    unity_init();
    RUN_TEST(test_vectors_base_and_ecdh);
    RUN_TEST(test_bad_inputs);
    RUN_TEST(test_constant_work);
    unity_print_results();
    unity_cleanup();
    return (unity_stats.tests_failed == 0) ? 0 : 1;
}
