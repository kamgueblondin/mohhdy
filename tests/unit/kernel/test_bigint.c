#include "../../framework/unity.h"
#include "../../../kernel/bigint.h"

void test_bigint_arithmetic_and_modexp(void){
    uint32_t a_limbs[2],b_limbs[2],m_limbs[1],n_limbs[2],o_limbs[2],workspace[8]={0};
    uint8_t bytes[2]={1,2},out[2]={0};
    bigint_t a,b,m,n,o;
    TEST_ASSERT_EQUAL(0,bigint_init(&a,a_limbs,2));
    TEST_ASSERT_EQUAL(0,bigint_init(&b,b_limbs,2));
    TEST_ASSERT_EQUAL(0,bigint_init(&m,m_limbs,1));
    TEST_ASSERT_EQUAL(0,bigint_init(&n,n_limbs,2));
    TEST_ASSERT_EQUAL(0,bigint_init(&o,o_limbs,2));
    TEST_ASSERT_EQUAL(0,bigint_from_be(&a,bytes,2));
    TEST_ASSERT_EQUAL(0x0102,a.limbs[0]);
    TEST_ASSERT_EQUAL(0,bigint_to_be(&a,out,2));
    TEST_ASSERT_EQUAL(1,out[0]);TEST_ASSERT_EQUAL(2,out[1]);
    b.limbs[0]=0x0103;b.length=1;TEST_ASSERT_EQUAL(-1,bigint_compare(&a,&b));
    a.limbs[0]=0xffffffffU;a.length=1;b.limbs[0]=2;b.length=1;
    TEST_ASSERT_EQUAL(0,bigint_add(&o,&a,&b));TEST_ASSERT_EQUAL(2,o.length);TEST_ASSERT_EQUAL(1,o.limbs[1]);TEST_ASSERT_EQUAL(1,o.limbs[0]);
    TEST_ASSERT_EQUAL(0,bigint_subtract(&o,&a,&b));TEST_ASSERT_EQUAL(0xfffffffdU,o.limbs[0]);
    a.limbs[0]=0x10000U;a.length=1;b.limbs[0]=0x10000U;b.length=1;
    TEST_ASSERT_EQUAL(0,bigint_multiply(&o,&a,&b));TEST_ASSERT_EQUAL(1,o.limbs[1]);TEST_ASSERT_EQUAL(0,o.limbs[0]);
    m.limbs[0]=13;m.length=1;a.limbs[0]=5;a.length=1;
    TEST_ASSERT_EQUAL(0,bigint_modexp_u32(&o,&a,3,&m,workspace,4));TEST_ASSERT_EQUAL(8,o.limbs[0]);
    /* 2^32 == -1 (mod 2^32+1), donc (2^32)^5 == 2^32. */
    n.limbs[0]=1U;n.limbs[1]=1U;n.length=2;a.limbs[0]=0U;a.limbs[1]=1U;a.length=2;
    TEST_ASSERT_EQUAL(0,bigint_modexp_u32(&o,&a,5U,&n,workspace,8));
    TEST_ASSERT_EQUAL(2,o.length);TEST_ASSERT_EQUAL(0U,o.limbs[0]);TEST_ASSERT_EQUAL(1U,o.limbs[1]);
}

void test_bigint_modexp_multilimb(void){uint32_t base_limbs[1],exponent_limbs[2],modulus_limbs[1],output_limbs[1],workspace[4]={0};bigint_t base,exponent,modulus,output;TEST_ASSERT_EQUAL(0,bigint_init(&base,base_limbs,1));TEST_ASSERT_EQUAL(0,bigint_init(&exponent,exponent_limbs,2));TEST_ASSERT_EQUAL(0,bigint_init(&modulus,modulus_limbs,1));TEST_ASSERT_EQUAL(0,bigint_init(&output,output_limbs,1));base.limbs[0]=7U;base.length=1U;exponent.limbs[0]=1U;exponent.limbs[1]=1U;exponent.length=2U;modulus.limbs[0]=101U;modulus.length=1U;TEST_ASSERT_EQUAL(0,bigint_modexp(&output,&base,&exponent,&modulus,workspace,4U));TEST_ASSERT_EQUAL(48U,output.limbs[0]);TEST_ASSERT_EQUAL(-2,bigint_modexp(&output,&base,&exponent,&modulus,workspace,3U));}

/* Leading-zero skip in bigint_modexp. Vectors are pow(base, exp, mod). */
static void modexp_u32(uint32_t base_v, uint32_t exp_v, uint32_t mod_v, uint32_t expect) {
    uint32_t base_limbs[1], exp_limbs[1], mod_limbs[1], out_limbs[1], workspace[4];
    bigint_t base, exponent, modulus, output;
    TEST_ASSERT_EQUAL(0, bigint_init(&base, base_limbs, 1));
    TEST_ASSERT_EQUAL(0, bigint_init(&exponent, exp_limbs, 1));
    TEST_ASSERT_EQUAL(0, bigint_init(&modulus, mod_limbs, 1));
    TEST_ASSERT_EQUAL(0, bigint_init(&output, out_limbs, 1));
    base.limbs[0] = base_v; base.length = 1U;
    if (exp_v == 0U) exponent.length = 0U;
    else { exponent.limbs[0] = exp_v; exponent.length = 1U; }
    modulus.limbs[0] = mod_v; modulus.length = 1U;
    TEST_ASSERT_EQUAL(0, bigint_modexp(&output, &base, &exponent, &modulus, workspace, 4U));
    TEST_ASSERT_EQUAL(1U, output.length);
    TEST_ASSERT_EQUAL(expect, output.limbs[0]);
}

void test_bigint_modexp_leading_zeros(void) {
    uint32_t base_limbs[1], exp_limbs[2], mod_limbs[1], out_limbs[1], workspace[4];
    bigint_t base, exponent, modulus, output;
    /* 7^10 mod 101 = 65. Exponent 10 is 0b1010: many leading zero bits. */
    modexp_u32(7U, 10U, 101U, 65U);
    /* Zero exponent leaves 1. A zero high limb is skipped the same way. */
    modexp_u32(5U, 0U, 13U, 1U);
    /* Single set bit: the first square is skipped because the result starts at 1. */
    modexp_u32(2U, 1U, 13U, 2U);
    modexp_u32(9U, 6U, 97U, 75U);
    modexp_u32(4U, 32U, 97U, 61U);
    /* MSB of a 32-bit limb is set: (1<<31)+3. */
    modexp_u32(7U, 2147483651U, 101U, 94U);
    TEST_ASSERT_EQUAL(0, bigint_init(&base, base_limbs, 1));
    TEST_ASSERT_EQUAL(0, bigint_init(&exponent, exp_limbs, 2));
    TEST_ASSERT_EQUAL(0, bigint_init(&modulus, mod_limbs, 1));
    TEST_ASSERT_EQUAL(0, bigint_init(&output, out_limbs, 1));
    base.limbs[0] = 7U; base.length = 1U;
    modulus.limbs[0] = 101U; modulus.length = 1U;
    /* 7^(5 * 2^32) mod 101 = 95. High limb is small, so its leading zeros are skipped. */
    exponent.limbs[0] = 0U; exponent.limbs[1] = 5U; exponent.length = 2U;
    TEST_ASSERT_EQUAL(0, bigint_modexp(&output, &base, &exponent, &modulus, workspace, 4U));
    TEST_ASSERT_EQUAL(95U, output.limbs[0]);
    /* 7^(2^32 + 1) mod 101 = 48. Same vector as test_bigint_modexp_multilimb. */
    exponent.limbs[0] = 1U; exponent.limbs[1] = 1U; exponent.length = 2U;
    TEST_ASSERT_EQUAL(0, bigint_modexp(&output, &base, &exponent, &modulus, workspace, 4U));
    TEST_ASSERT_EQUAL(48U, output.limbs[0]);
    /* High limb 0, low limb 13: 6^13 mod 50 = 16. The zero limb is fully skipped. */
    base.limbs[0] = 6U; base.length = 1U;
    modulus.limbs[0] = 50U; modulus.length = 1U;
    exponent.limbs[0] = 13U; exponent.limbs[1] = 0U; exponent.length = 2U;
    TEST_ASSERT_EQUAL(0, bigint_modexp(&output, &base, &exponent, &modulus, workspace, 4U));
    TEST_ASSERT_EQUAL(16U, output.limbs[0]);
}

void test_bigint_robustness_even_modulus_base_larger_exponents(void) {
    uint32_t base_limbs[2], exp_limbs[1], mod_limbs[1], out_limbs[1], workspace[4];
    bigint_t base, exponent, modulus, output;

    /* 1. Even modulus and base larger than modulus */
    /* 25^3 mod 10 = 125 mod 10 = 5 */
    modexp_u32(25U, 3U, 10U, 5U);
    /* 14^4 mod 10 = 6 */
    modexp_u32(14U, 4U, 10U, 6U);
    /* 8^3 mod 14 = 512 mod 14 = 8 */
    modexp_u32(8U, 3U, 14U, 8U);

    /* 2. Base larger than modulus with odd/even modulus */
    /* 105^2 mod 13 = 1 (105 mod 13 = 1) */
    modexp_u32(105U, 2U, 13U, 1U);
    /* 100^3 mod 7 = 1 */
    modexp_u32(100U, 3U, 7U, 1U);

    /* 3. Exponent equal to 0 */
    /* 7^0 mod 10 = 1 */
    modexp_u32(7U, 0U, 10U, 1U);
    /* 25^0 mod 10 = 1 */
    modexp_u32(25U, 0U, 10U, 1U);
    /* 123^0 mod 14 = 1 */
    modexp_u32(123U, 0U, 14U, 1U);

    /* 4. Exponent equal to 1 */
    /* 7^1 mod 10 = 7 */
    modexp_u32(7U, 1U, 10U, 7U);
    /* 25^1 mod 10 = 5 (since 25 mod 10 = 5) */
    modexp_u32(25U, 1U, 10U, 5U);
    /* 123^1 mod 14 = 11 (since 123 mod 14 = 11) */
    modexp_u32(123U, 1U, 14U, 11U);

    /* 5. Multi-limb base larger than an even modulus with exp 0 and exp 1 */
    TEST_ASSERT_EQUAL(0, bigint_init(&base, base_limbs, 2));
    TEST_ASSERT_EQUAL(0, bigint_init(&exponent, exp_limbs, 1));
    TEST_ASSERT_EQUAL(0, bigint_init(&modulus, mod_limbs, 1));
    TEST_ASSERT_EQUAL(0, bigint_init(&output, out_limbs, 1));

    /* base = 2 * 2^32 + 5 = 8589934597. 8589934597 mod 10 = 7. */
    base.limbs[0] = 5U; base.limbs[1] = 2U; base.length = 2U;
    modulus.limbs[0] = 10U; modulus.length = 1U;

    /* exp = 0 */
    exponent.length = 0U;
    TEST_ASSERT_EQUAL(0, bigint_modexp(&output, &base, &exponent, &modulus, workspace, 4U));
    TEST_ASSERT_EQUAL(1U, output.length);
    TEST_ASSERT_EQUAL(1U, output.limbs[0]);

    /* exp = 1 */
    exponent.limbs[0] = 1U; exponent.length = 1U;
    TEST_ASSERT_EQUAL(0, bigint_modexp(&output, &base, &exponent, &modulus, workspace, 4U));
    TEST_ASSERT_EQUAL(1U, output.length);
    TEST_ASSERT_EQUAL(7U, output.limbs[0]);
}

void test_bigint_constant_width_modular_primitives(void){uint32_t a_limbs[1],b_limbs[1],m_limbs[1],legacy_limbs[1],ct_limbs[1],temporary_limbs[1];bigint_t a,b,m,legacy,ct,temporary;TEST_ASSERT_EQUAL(0,bigint_init(&a,a_limbs,1));TEST_ASSERT_EQUAL(0,bigint_init(&b,b_limbs,1));TEST_ASSERT_EQUAL(0,bigint_init(&m,m_limbs,1));TEST_ASSERT_EQUAL(0,bigint_init(&legacy,legacy_limbs,1));TEST_ASSERT_EQUAL(0,bigint_init(&ct,ct_limbs,1));TEST_ASSERT_EQUAL(0,bigint_init(&temporary,temporary_limbs,1));a.limbs[0]=5U;a.length=1U;b.limbs[0]=11U;b.length=1U;m.limbs[0]=13U;m.length=1U;TEST_ASSERT_EQUAL(0,bigint_mod_add(&legacy,&a,&b,&m));TEST_ASSERT_EQUAL(0,bigint_mod_add_ct(&ct,&a,&b,&m));TEST_ASSERT_EQUAL(legacy.limbs[0],ct.limbs[0]);TEST_ASSERT_EQUAL(0,bigint_mod_subtract_ct(&ct,&a,&b,&m));TEST_ASSERT_EQUAL(7U,ct.limbs[0]);TEST_ASSERT_EQUAL(0,bigint_mod_multiply(&legacy,&a,&b,&m,&temporary));TEST_ASSERT_EQUAL(0,bigint_mod_multiply_ct(&ct,&a,&b,&m,&temporary));TEST_ASSERT_EQUAL(legacy.limbs[0],ct.limbs[0]);TEST_ASSERT_EQUAL(3U,ct.limbs[0]);}

#include "bigint_mont_vectors.inc"
/* Lot 48 (A2): bigint_modexp / bigint_modexp_u32 (Montgomery path for odd
 * moduli) against Python pow() on 32..4096-bit moduli, with bases 0, 1,
 * m-1, m and above m. */
void test_bigint_modexp_montgomery_vectors(void){
    static uint32_t ml[128],el[32],bl[130],ol[128],ws[4U*128U];
    bigint_t m,e,b,o;uint16_t k,i,len;
    for(k=0U;k<(uint16_t)(sizeof(mont_vectors)/sizeof(mont_vectors[0]));k++){
        TEST_ASSERT_EQUAL(0,bigint_init(&m,ml,mont_vectors[k].n));TEST_ASSERT_EQUAL(0,bigint_init(&e,el,32));
        TEST_ASSERT_EQUAL(0,bigint_init(&b,bl,130));TEST_ASSERT_EQUAL(0,bigint_init(&o,ol,mont_vectors[k].n));
        for(i=0U;i<mont_vectors[k].n;i++)ml[i]=mont_vectors[k].m[i];m.length=mont_vectors[k].n;
        for(i=0U;i<mont_vectors[k].ne;i++)el[i]=mont_vectors[k].e[i];e.length=mont_vectors[k].ne;
        for(i=0U;i<mont_vectors[k].nb;i++)bl[i]=mont_vectors[k].b[i];b.length=mont_vectors[k].nb;
        while(b.length>0U&&bl[b.length-1U]==0U)b.length--;
        if(mont_vectors[k].ne==1U&&el[0]==65537U)TEST_ASSERT_EQUAL(0,bigint_modexp_u32(&o,&b,65537U,&m,ws,(uint16_t)(4U*m.length)));
        else TEST_ASSERT_EQUAL(0,bigint_modexp(&o,&b,&e,&m,ws,(uint16_t)(4U*m.length)));
        len=mont_vectors[k].n;while(len>0U&&mont_vectors[k].r[len-1U]==0U)len--;
        TEST_ASSERT_EQUAL(len,o.length);
        for(i=0U;i<len;i++)TEST_ASSERT_EQUAL(mont_vectors[k].r[i],o.limbs[i]);
    }
}
int main(void){unity_init();RUN_TEST(test_bigint_modexp_montgomery_vectors);RUN_TEST(test_bigint_arithmetic_and_modexp);RUN_TEST(test_bigint_modexp_multilimb);RUN_TEST(test_bigint_modexp_leading_zeros);RUN_TEST(test_bigint_robustness_even_modulus_base_larger_exponents);RUN_TEST(test_bigint_constant_width_modular_primitives);unity_print_results();unity_cleanup();return unity_stats.tests_failed==0?0:1;}
