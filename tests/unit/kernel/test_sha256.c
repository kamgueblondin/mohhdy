#include "../../framework/unity.h"
#include "../../../kernel/sha256.h"
void setUp(void){} void tearDown(void){}
void test_sha256_empty(void){
    sha256_ctx_t c; uint8_t d[32]; sha256_init(&c); sha256_final(&c,d);
    TEST_ASSERT_EQUAL(0xe3,d[0]);
    TEST_ASSERT_EQUAL(0xb0,d[1]);
    TEST_ASSERT_EQUAL(0x42,d[3]);
    TEST_ASSERT_EQUAL(0x55,d[31]);
}
void test_hmac_sha256(void){
    uint8_t key[20], digest[32]; const uint8_t message[] = "Hi There"; uint32_t i;
    for (i=0;i<20;i++) key[i]=0x0b;
    hmac_sha256(key,20,message,8,digest);
    TEST_ASSERT_EQUAL(0xb0,digest[0]); TEST_ASSERT_EQUAL(0x34,digest[1]); TEST_ASSERT_EQUAL(0x4c,digest[2]); TEST_ASSERT_EQUAL(0xf7,digest[31]);
}
void test_sha256_fragmented_abc(void){
    sha256_ctx_t c; uint8_t d[32]; const uint8_t a[]={'a','b','c'};
    sha256_init(&c); sha256_update(&c,a,1); sha256_update(&c,a+1,2); sha256_final(&c,d);
    TEST_ASSERT_EQUAL(0xba,d[0]);
    TEST_ASSERT_EQUAL(0x78,d[1]);
    TEST_ASSERT_EQUAL(0x16,d[2]);
    TEST_ASSERT_EQUAL(0xad,d[31]);
}
int main(void){unity_init();RUN_TEST(test_sha256_empty);RUN_TEST(test_sha256_fragmented_abc);RUN_TEST(test_hmac_sha256);unity_print_results();unity_cleanup();return unity_stats.tests_failed==0?0:1;}
