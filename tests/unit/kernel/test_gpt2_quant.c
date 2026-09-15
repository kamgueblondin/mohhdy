/* test_gpt2_quant.c - GGML Q8_0 math primitives without a model file. */

#include "../../framework/unity.h"
#include "../../../kernel/llm/gpt2_quant.h"

static void assert_close(float actual, float expected, float tolerance) {
    float delta = actual - expected;
    if (delta < 0.0f) delta = -delta;
    TEST_ASSERT_TRUE(delta <= tolerance);
}

static void test_f16_common_values(void) {
    assert_close(gpt2_f16_to_f32(0x0000U), 0.0f, 0.0f);
    assert_close(gpt2_f16_to_f32(0x3c00U), 1.0f, 0.0f);
    assert_close(gpt2_f16_to_f32(0xc000U), -2.0f, 0.0f);
    assert_close(gpt2_f16_to_f32(0x3800U), 0.5f, 0.0f);
}

static void test_f16_subnormal_and_infinity(void) {
    float smallest = gpt2_f16_to_f32(0x0001U);
    TEST_ASSERT_TRUE(smallest > 0.0f);
    TEST_ASSERT_TRUE(gpt2_f16_to_f32(0x7c00U) > 1000000.0f);
}

static void test_q8_dot_one_block(void) {
    float activation[GPT2_Q8_0_BLOCK_SIZE];
    uint8_t block[GPT2_Q8_0_BLOCK_BYTES];
    uint32_t i;
    for (i = 0U; i < GPT2_Q8_0_BLOCK_SIZE; i++) {
        activation[i] = 0.5f;
        block[2U + i] = 2U;
    }
    block[0] = 0x00U; /* binary16 1.0 */
    block[1] = 0x3cU;
    assert_close(gpt2_q8_0_dot_f32(activation, block, GPT2_Q8_0_BLOCK_SIZE), 32.0f, 0.0001f);
}

static void test_q8_dot_two_blocks_with_signs(void) {
    float activation[2U * GPT2_Q8_0_BLOCK_SIZE];
    uint8_t blocks[2U * GPT2_Q8_0_BLOCK_BYTES];
    uint32_t i;
    for (i = 0U; i < 2U * GPT2_Q8_0_BLOCK_SIZE; i++) activation[i] = 1.0f;
    blocks[0] = 0x00U; blocks[1] = 0x3cU; /* scale 1.0 */
    blocks[GPT2_Q8_0_BLOCK_BYTES] = 0x00U;
    blocks[GPT2_Q8_0_BLOCK_BYTES + 1U] = 0x38U; /* scale 0.5 */
    for (i = 0U; i < GPT2_Q8_0_BLOCK_SIZE; i++) {
        blocks[2U + i] = 3U;
        blocks[GPT2_Q8_0_BLOCK_BYTES + 2U + i] = (uint8_t)(int8_t)-2;
    }
    assert_close(gpt2_q8_0_dot_f32(activation, blocks, 2U * GPT2_Q8_0_BLOCK_SIZE), 64.0f, 0.0001f);
}

static void test_q8_rejects_invalid_length(void) {
    float activation[GPT2_Q8_0_BLOCK_SIZE] = {0.0f};
    uint8_t block[GPT2_Q8_0_BLOCK_BYTES] = {0U};
    TEST_ASSERT_EQUAL(0, (int)gpt2_q8_0_dot_f32(activation, block, 31U));
    TEST_ASSERT_EQUAL(0, (int)gpt2_q8_0_dot_f32(0, block, GPT2_Q8_0_BLOCK_SIZE));
}

static void test_q3_k_dot_one_super_block(void) {
    float activation[GPT2_QK_K];
    float decoded[GPT2_QK_K];
    uint8_t block[GPT2_Q3_K_BLOCK_BYTES];
    uint32_t i;
    for (i = 0U; i < GPT2_QK_K; i++) activation[i] = 1.0f;
    for (i = 0U; i < 32U; i++) block[i] = 0xffU;
    for (i = 32U; i < 96U; i++) block[i] = 0x55U;
    for (i = 96U; i < 104U; i++) block[i] = 0x11U;
    for (i = 104U; i < 108U; i++) block[i] = 0xaaU;
    block[108] = 0x00U;
    block[109] = 0x3cU;
    assert_close(gpt2_q3_k_dot_f32(activation, block, GPT2_QK_K), 256.0f, 0.0001f);
    TEST_ASSERT_EQUAL(0, gpt2_q3_k_dequantize(block, GPT2_QK_K, decoded));
    { float total = 0.0f; for (i = 0U; i < GPT2_QK_K; i++) total += decoded[i]; assert_close(total, 256.0f, 0.0001f); }
    TEST_ASSERT_EQUAL(-1, gpt2_q3_k_dequantize(block, 128U, decoded));
    TEST_ASSERT_EQUAL(0, (int)gpt2_q3_k_dot_f32(activation, block, 128U));
}

static void test_q4_k_dot_one_super_block(void) {
    float activation[GPT2_QK_K];
    float decoded[GPT2_QK_K];
    uint8_t block[GPT2_Q4_K_BLOCK_BYTES];
    uint32_t i;
    for (i = 0U; i < GPT2_QK_K; i++) activation[i] = 1.0f;
    for (i = 0U; i < GPT2_Q4_K_BLOCK_BYTES; i++) block[i] = 0U;
    block[0] = 0x00U; block[1] = 0x3cU;
    for (i = 0U; i < 4U; i++) block[4U + i] = 1U;
    for (i = 0U; i < 4U; i++) block[12U + i] = 1U;
    for (i = 0U; i < 128U; i++) block[16U + i] = 0x11U;
    assert_close(gpt2_q4_k_dot_f32(activation, block, GPT2_QK_K), 256.0f, 0.0001f);
    TEST_ASSERT_EQUAL(0, gpt2_q4_k_dequantize(block, GPT2_QK_K, decoded));
    { float total = 0.0f; for (i = 0U; i < GPT2_QK_K; i++) total += decoded[i]; assert_close(total, 256.0f, 0.0001f); }
    TEST_ASSERT_EQUAL(-1, gpt2_q4_k_dequantize(block, 128U, decoded));
    TEST_ASSERT_EQUAL(0, (int)gpt2_q4_k_dot_f32(activation, block, 128U));
}

static void test_q6_k_dot_one_super_block(void) {
    float activation[GPT2_QK_K];
    float decoded[GPT2_QK_K];
    uint8_t block[GPT2_Q6_K_BLOCK_BYTES];
    uint32_t i;
    for (i = 0U; i < GPT2_QK_K; i++) activation[i] = 1.0f;
    for (i = 0U; i < 128U; i++) block[i] = 0x11U;
    for (i = 128U; i < 192U; i++) block[i] = 0xaaU;
    for (i = 192U; i < 208U; i++) block[i] = 1U;
    block[208] = 0x00U; block[209] = 0x3cU;
    assert_close(gpt2_q6_k_dot_f32(activation, block, GPT2_QK_K), 256.0f, 0.0001f);
    TEST_ASSERT_EQUAL(0, gpt2_q6_k_dequantize(block, GPT2_QK_K, decoded));
    { float total = 0.0f; for (i = 0U; i < GPT2_QK_K; i++) total += decoded[i]; assert_close(total, 256.0f, 0.0001f); }
    TEST_ASSERT_EQUAL(-1, gpt2_q6_k_dequantize(block, 128U, decoded));
    TEST_ASSERT_EQUAL(0, (int)gpt2_q6_k_dot_f32(activation, block, 128U));
}

int main(void) {
    unity_init();
    RUN_TEST(test_f16_common_values);
    RUN_TEST(test_f16_subnormal_and_infinity);
    RUN_TEST(test_q8_dot_one_block);
    RUN_TEST(test_q8_dot_two_blocks_with_signs);
    RUN_TEST(test_q8_rejects_invalid_length);
    RUN_TEST(test_q3_k_dot_one_super_block);
    RUN_TEST(test_q4_k_dot_one_super_block);
    RUN_TEST(test_q6_k_dot_one_super_block);
    unity_print_results();
    unity_cleanup();
    return unity_stats.tests_failed == 0 ? 0 : 1;
}
