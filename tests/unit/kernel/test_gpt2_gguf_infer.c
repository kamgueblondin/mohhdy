#include "../../framework/unity.h"
#include "../../../kernel/llm/gpt2_gguf_infer.h"

static void test_rejects_generation_without_ready_fat16_profile(void) {
    uint32_t token = 0U;
    uint32_t next = 0U;
    uint32_t rng = 1U;
    TEST_ASSERT_EQUAL(0, gpt2_gguf_infer_ready());
    TEST_ASSERT_EQUAL(-1, gpt2_gguf_infer_init_fat16(0, "GPT2.GGU"));
    TEST_ASSERT_EQUAL(0, gpt2_gguf_infer_ready());
    TEST_ASSERT_EQUAL(-1, gpt2_gguf_generate_next_sampled(&token, 1U, 0U, &next, &rng));
}

int main(void) {
    unity_init();
    RUN_TEST(test_rejects_generation_without_ready_fat16_profile);
    unity_print_results();
    unity_cleanup();
    return unity_stats.tests_failed == 0 ? 0 : 1;
}
