#include "../../framework/unity.h"
#include "../../../kernel/ecdsa_p256.h"

static const uint8_t p256_vector_public[65] = {
    0x04U,0x5dU,0xebU,0x15U,0x2dU,0x48U,0x73U,0xacU,0xcbU,0x8dU,0x83U,0x9aU,
    0x89U,0x4fU,0x86U,0xdbU,0x74U,0x05U,0xa7U,0x32U,0x20U,0xf2U,0x1cU,0x18U,
    0xbcU,0x1dU,0xa4U,0xc7U,0x78U,0xbeU,0x27U,0x16U,0x85U,0x4eU,0x62U,0xbbU,
    0xd9U,0xb3U,0xeeU,0xb7U,0x07U,0xdcU,0xa4U,0x52U,0x05U,0x5aU,0x1eU,0xa4U,
    0xc6U,0x2aU,0x86U,0xb7U,0xd8U,0x23U,0x41U,0x83U,0x90U,0xf7U,0x27U,0x16U,
    0xa9U,0xceU,0x44U,0xb9U,0x3cU
};
static const uint8_t p256_vector_hash[32] = {
    0x7fU,0xccU,0xe3U,0x92U,0x7eU,0x53U,0x7bU,0xa9U,0x72U,0x5fU,0xc9U,0xa9U,
    0xe3U,0x20U,0x50U,0x3aU,0xc8U,0x39U,0x1dU,0x53U,0xddU,0xcdU,0xb8U,0x5eU,
    0x3fU,0xbfU,0xc7U,0x3cU,0xe2U,0xd1U,0x03U,0xf2U
};
static const uint8_t p256_vector_signature[70] = {
    0x30U,0x44U,0x02U,0x20U,0x16U,0x49U,0x23U,0x7bU,0x44U,0xe2U,0x43U,0xabU,
    0x34U,0x57U,0xbfU,0x8dU,0x98U,0x27U,0xa7U,0xd4U,0x01U,0x15U,0x19U,0x45U,
    0x29U,0xbeU,0x3dU,0x77U,0x1cU,0x2eU,0x12U,0x13U,0x40U,0xabU,0x74U,0x6cU,
    0x02U,0x20U,0x7dU,0x28U,0xa6U,0x0cU,0x44U,0x19U,0xfeU,0xefU,0xbeU,0x25U,
    0x1eU,0x3eU,0x2aU,0x69U,0x09U,0xeeU,0x8dU,0xc1U,0xd9U,0x51U,0x02U,0x7cU,
    0x17U,0xb5U,0xddU,0xbdU,0xb6U,0x5fU,0x52U,0x59U,0x20U,0x41U
};

void test_ecdsa_p256_accepts_valid_signature(void) {
    uint32_t workspace[ECDSA_P256_WORKSPACE_WORDS] = {0};
    TEST_ASSERT_EQUAL(0,ecdsa_p256_sha256_verify(p256_vector_public,p256_vector_hash,
        p256_vector_signature,sizeof(p256_vector_signature),workspace,ECDSA_P256_WORKSPACE_WORDS));
}

void test_ecdsa_p256_rejects_modified_signature_and_point(void) {
    uint8_t signature[sizeof(p256_vector_signature)],public_key[sizeof(p256_vector_public)];
    uint32_t workspace[ECDSA_P256_WORKSPACE_WORDS] = {0};
    uint16_t i;
    for(i=0U;i<sizeof(signature);++i) signature[i]=p256_vector_signature[i];
    signature[sizeof(signature)-1U]^=1U;
    TEST_ASSERT_NOT_EQUAL(0,ecdsa_p256_sha256_verify(p256_vector_public,p256_vector_hash,signature,
        sizeof(signature),workspace,ECDSA_P256_WORKSPACE_WORDS));
    for(i=0U;i<sizeof(public_key);++i) public_key[i]=p256_vector_public[i];
    public_key[sizeof(public_key)-1U]^=1U;
    TEST_ASSERT_NOT_EQUAL(0,ecdsa_p256_sha256_verify(public_key,p256_vector_hash,p256_vector_signature,
        sizeof(p256_vector_signature),workspace,ECDSA_P256_WORKSPACE_WORDS));
}

void test_ecdsa_p256_rejects_noncanonical_der_and_short_workspace(void) {
    uint8_t signature[sizeof(p256_vector_signature)];
    uint32_t workspace[ECDSA_P256_WORKSPACE_WORDS] = {0};
    uint16_t i;
    for(i=0U;i<sizeof(signature);++i) signature[i]=p256_vector_signature[i];
    signature[2U]=0x00U;
    TEST_ASSERT_NOT_EQUAL(0,ecdsa_p256_sha256_verify(p256_vector_public,p256_vector_hash,signature,
        sizeof(signature),workspace,ECDSA_P256_WORKSPACE_WORDS));
    TEST_ASSERT_NOT_EQUAL(0,ecdsa_p256_sha256_verify(p256_vector_public,p256_vector_hash,p256_vector_signature,
        sizeof(p256_vector_signature),workspace,ECDSA_P256_WORKSPACE_WORDS-1U));
}

int main(void) {
    unity_init();
    RUN_TEST(test_ecdsa_p256_accepts_valid_signature);
    RUN_TEST(test_ecdsa_p256_rejects_modified_signature_and_point);
    RUN_TEST(test_ecdsa_p256_rejects_noncanonical_der_and_short_workspace);
    unity_print_results();
    unity_cleanup();
    return unity_stats.tests_failed==0?0:1;
}
