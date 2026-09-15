/* test_gpt2_infer.c - workspace GPT-2 statique et bornes de configuration. */
#include "../../framework/unity.h"
#include "../../../kernel/llm/gpt2_infer.h"
#include "../../../kernel/llm/gpt2_model.h"

static gpt2_model_t test_model;
static float test_weights[28];

const gpt2_model_t* gpt2_model_current(void) {
    return &test_model;
}

static void prepare_tiny_model(void) {
    uint32_t index;
    for (index = 0U; index < sizeof(test_weights) / sizeof(test_weights[0]); ++index)
        test_weights[index] = 0.0f;
    test_model.blob = 0;
    test_model.blob_size = 0U;
    test_model.weights = test_weights;
    test_model.weight_count = sizeof(test_weights) / sizeof(test_weights[0]);
    test_model.config.max_seq_len = 2U;
    test_model.config.vocab_size = 2U;
    test_model.config.padded_vocab_size = 2U;
    test_model.config.num_layers = 1U;
    test_model.config.num_heads = 1U;
    test_model.config.channels = 1U;
    test_model.ready = 1;
}

void setUp(void) {}
void tearDown(void) {}

void test_gpt2_infer_uses_static_workspace_for_tiny_model(void) {
    uint32_t token = 0U;
    uint32_t next = 99U;
    prepare_tiny_model();
    TEST_ASSERT_EQUAL(0, gpt2_generate_next(&token, 1U, &next));
    TEST_ASSERT_EQUAL(0, next);
    TEST_ASSERT_NOT_NULL(gpt2_infer_status());
}

void test_gpt2_infer_rejects_configuration_above_static_capacity(void) {
    uint32_t token = 0U;
    uint32_t next = 99U;
    prepare_tiny_model();
    test_model.config.channels = 769U;
    TEST_ASSERT_EQUAL(-4, gpt2_generate_next(&token, 1U, &next));
    TEST_ASSERT_NOT_NULL(gpt2_infer_status());
}

int main(void) {
    unity_init();
    RUN_TEST(test_gpt2_infer_uses_static_workspace_for_tiny_model);
    RUN_TEST(test_gpt2_infer_rejects_configuration_above_static_capacity);
    unity_print_results();
    unity_cleanup();
    return unity_stats.tests_failed == 0 ? 0 : 1;
}
