#include "../../framework/unity.h"
#include "../../../kernel/aes_gcm.h"

void test_aes128_fips_block(void){
    uint8_t key[16]={0,1,2,3,4,5,6,7,8,9,10,11,12,13,14,15};
    uint8_t input[16]={0,17,34,51,68,85,102,119,136,153,170,187,204,221,238,255};
    uint8_t expected[16]={0x69,0xc4,0xe0,0xd8,0x6a,0x7b,0x04,0x30,0xd8,0xcd,0xb7,0x80,0x70,0xb4,0xc5,0x5a};
    uint8_t output[16]={0},i; aes128_ctx_t context;
    TEST_ASSERT_EQUAL(0,aes128_init(&context,key)); TEST_ASSERT_EQUAL(0,aes128_encrypt_block(&context,input,output));
    for(i=0U;i<16U;i++)TEST_ASSERT_EQUAL(expected[i],output[i]);
}
void test_aes128_gcm_nist_and_tag_rejection(void){
    uint8_t key[16]={0},fixed_iv[4]={0},explicit_nonce[8]={0},plaintext[16]={0},ciphertext[16]={0},decoded[16]={0},tag[16]={0},bad_tag[16]={0},i;
    uint8_t expected_ciphertext[16]={0x03,0x88,0xda,0xce,0x60,0xb6,0xa3,0x92,0xf3,0x28,0xc2,0xb9,0x71,0xb2,0xfe,0x78};
    uint8_t expected_tag[16]={0xab,0x6e,0x47,0xd4,0x2c,0xec,0x13,0xbd,0xf5,0x3a,0x67,0xb2,0x12,0x57,0xbd,0xdf};
    TEST_ASSERT_EQUAL(0,aes128_gcm_encrypt(key,fixed_iv,explicit_nonce,0,0,plaintext,sizeof(plaintext),ciphertext,tag));
    for(i=0U;i<16U;i++){TEST_ASSERT_EQUAL(expected_ciphertext[i],ciphertext[i]);TEST_ASSERT_EQUAL(expected_tag[i],tag[i]);bad_tag[i]=tag[i];decoded[i]=0xaaU;}
    TEST_ASSERT_EQUAL(0,aes128_gcm_decrypt(key,fixed_iv,explicit_nonce,0,0,ciphertext,sizeof(ciphertext),tag,decoded));
    for(i=0U;i<16U;i++)TEST_ASSERT_EQUAL(0,decoded[i]);
    bad_tag[0]^=1U;for(i=0U;i<16U;i++)decoded[i]=0xaaU;TEST_ASSERT_NOT_EQUAL(0,aes128_gcm_decrypt(key,fixed_iv,explicit_nonce,0,0,ciphertext,sizeof(ciphertext),bad_tag,decoded));for(i=0U;i<16U;i++)TEST_ASSERT_EQUAL(0xaa,decoded[i]);
}
int main(void){unity_init();RUN_TEST(test_aes128_fips_block);RUN_TEST(test_aes128_gcm_nist_and_tag_rejection);unity_print_results();unity_cleanup();return unity_stats.tests_failed==0?0:1;}
