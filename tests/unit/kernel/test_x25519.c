#include "../../framework/unity.h"
#include "../../../kernel/x25519.h"

void test_x25519_rfc7748_vector(void){
    static const uint8_t scalar[32]={0xa5U,0x46U,0xe3U,0x6bU,0xf0U,0x52U,0x7cU,0x9dU,0x3bU,0x16U,0x15U,0x4bU,0x82U,0x46U,0x5eU,0xddU,0x62U,0x14U,0x4cU,0x0aU,0xc1U,0xfcU,0x5aU,0x18U,0x50U,0x6aU,0x22U,0x44U,0xbaU,0x44U,0x9aU,0xc4U};
    static const uint8_t u[32]={0xe6U,0xdbU,0x68U,0x67U,0x58U,0x30U,0x30U,0xdbU,0x35U,0x94U,0xc1U,0xa4U,0x24U,0xb1U,0x5fU,0x7cU,0x72U,0x66U,0x24U,0xecU,0x26U,0xb3U,0x35U,0x3bU,0x10U,0xa9U,0x03U,0xa6U,0xd0U,0xabU,0x1cU,0x4cU};
    static const uint8_t expected[32]={0xc3U,0xdaU,0x55U,0x37U,0x9dU,0xe9U,0xc6U,0x90U,0x8eU,0x94U,0xeaU,0x4dU,0xf2U,0x8dU,0x08U,0x4fU,0x32U,0xecU,0xcfU,0x03U,0x49U,0x1cU,0x71U,0xf7U,0x54U,0xb4U,0x07U,0x55U,0x77U,0xa2U,0x85U,0x52U};
    uint8_t output[32]={0};uint32_t workspace[X25519_WORKSPACE_LIMBS]={0};uint16_t i;
    TEST_ASSERT_EQUAL(0,x25519_scalar_mult(output,scalar,u,workspace,X25519_WORKSPACE_LIMBS));
    for(i=0U;i<32U;i++)TEST_ASSERT_EQUAL(expected[i],output[i]);
}

void test_x25519_rejects_zero_secret(void){
    uint8_t private_key[32]={7U},peer_public[32]={0},output[32]={0};uint32_t workspace[X25519_WORKSPACE_LIMBS]={0};
    TEST_ASSERT_EQUAL(-9,x25519_shared_secret(output,private_key,peer_public,workspace,X25519_WORKSPACE_LIMBS));
    TEST_ASSERT_NOT_EQUAL(0,x25519_scalar_mult(output,private_key,peer_public,workspace,X25519_WORKSPACE_LIMBS-1U));
}

#include "x25519_ref.inc"

/* Lot 48 (A2): RFC 7748 section 5.2 iterated vector, 1 and 1000 rounds. */
void test_x25519_rfc7748_iterated(void){
    static const uint8_t after1[32]={0x42U,0x2cU,0x8eU,0x7aU,0x62U,0x27U,0xd7U,0xbcU,0xa1U,0x35U,0x0bU,0x3eU,0x2bU,0xb7U,0x27U,0x9fU,0x78U,0x97U,0xb8U,0x7bU,0xb6U,0x85U,0x4bU,0x78U,0x3cU,0x60U,0xe8U,0x03U,0x11U,0xaeU,0x30U,0x79U};
    static const uint8_t after1000[32]={0x68U,0x4cU,0xf5U,0x9bU,0xa8U,0x33U,0x09U,0x55U,0x28U,0x00U,0xefU,0x56U,0x6fU,0x2fU,0x4dU,0x3cU,0x1cU,0x38U,0x87U,0xc4U,0x93U,0x60U,0xe3U,0x87U,0x5fU,0x2eU,0xb9U,0x4dU,0x99U,0x53U,0x2cU,0x51U};
    uint8_t k[32]={9U},u[32]={9U},out[32];uint32_t ws[X25519_WORKSPACE_LIMBS];uint16_t i,n;
    for(n=1U;n<=1000U;n++){
        TEST_ASSERT_EQUAL(0,x25519_scalar_mult(out,k,u,ws,X25519_WORKSPACE_LIMBS));
        for(i=0U;i<32U;i++){u[i]=k[i];k[i]=out[i];}
        if(n==1U)for(i=0U;i<32U;i++)TEST_ASSERT_EQUAL(after1[i],k[i]);
    }
    for(i=0U;i<32U;i++)TEST_ASSERT_EQUAL(after1000[i],k[i]);
}
/* Lot 48 (A2): the fast field code equals the former generic bigint code
 * on pseudo-random scalars and u values, including u with the top bit set
 * and non-canonical u (p <= u < 2^255) and edge values 0, 1, p-1, p. */
void test_x25519_matches_reference(void){
    uint8_t k[32],u[32],a[32],b[32];uint32_t ws[X25519_WORKSPACE_LIMBS],wr[X25519_WORKSPACE_LIMBS];
    uint32_t seed=0x2545f491U;uint16_t n,i;
    for(n=0U;n<48U;n++){
        for(i=0U;i<32U;i++){seed=seed*1103515245U+12345U;k[i]=(uint8_t)(seed>>16);seed=seed*1103515245U+12345U;u[i]=(uint8_t)(seed>>16);}
        if(n<8U){for(i=0U;i<32U;i++)u[i]=0xffU;u[0]=(uint8_t)(0xedU+n);}   /* around p and above */
        if(n==8U)for(i=0U;i<32U;i++)u[i]=0U;
        if(n==9U){for(i=0U;i<32U;i++)u[i]=0U;u[0]=1U;}
        if(n==10U){for(i=0U;i<32U;i++)u[i]=0xffU;u[0]=0xecU;u[31]=0x7fU;}  /* p-1 */
        TEST_ASSERT_EQUAL(0,x25519_scalar_mult(a,k,u,ws,X25519_WORKSPACE_LIMBS));
        TEST_ASSERT_EQUAL(0,ref_x25519_scalar_mult(b,k,u,wr,X25519_WORKSPACE_LIMBS));
        for(i=0U;i<32U;i++)TEST_ASSERT_EQUAL(b[i],a[i]);
    }
}
int main(void){unity_init();RUN_TEST(test_x25519_rfc7748_vector);RUN_TEST(test_x25519_rejects_zero_secret);RUN_TEST(test_x25519_rfc7748_iterated);RUN_TEST(test_x25519_matches_reference);unity_print_results();unity_cleanup();return unity_stats.tests_failed==0?0:1;}
