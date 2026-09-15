/* AOS-022 robustness checks for the bounded GGUF envelope reader. */
#include "../framework/unity.h"
#include "../../kernel/llm/gpt2_gguf.h"

static uint8_t blob[256];
static uint32_t cursor;

static void reset_blob(void) {
    uint32_t i;
    for (i = 0U; i < sizeof(blob); i++) blob[i] = 0U;
    cursor = 0U;
}

static void put_u32(uint32_t value) {
    blob[cursor++] = (uint8_t)value;
    blob[cursor++] = (uint8_t)(value >> 8);
    blob[cursor++] = (uint8_t)(value >> 16);
    blob[cursor++] = (uint8_t)(value >> 24);
}

static void put_u64(uint64_t value) {
    put_u32((uint32_t)value);
    put_u32((uint32_t)(value >> 32));
}

static void put_text(const char* text) {
    uint32_t length = 0U;
    while (text[length]) length++;
    put_u64(length);
    while (*text) blob[cursor++] = (uint8_t)*text++;
}

static uint32_t build_valid_minimal_gpt2(void) {
    reset_blob();
    put_u32(GPT2_GGUF_MAGIC);
    put_u32(GPT2_GGUF_VERSION);
    put_u64(0U);
    put_u64(1U);
    put_text("general.architecture");
    put_u32(GPT2_GGUF_VALUE_STRING);
    put_text("gpt2");
    while ((cursor & 31U) != 0U) blob[cursor++] = 0U;
    return cursor;
}

static void test_every_truncated_prefix_is_rejected(void) {
    gpt2_gguf_info_t info;
    uint32_t size = build_valid_minimal_gpt2();
    uint32_t prefix;
    TEST_ASSERT_EQUAL(0, gpt2_gguf_probe_blob(blob, size, &info));
    for (prefix = 0U; prefix < size; prefix++) {
        TEST_ASSERT_TRUE(gpt2_gguf_probe_blob(blob, prefix, &info) != 0);
    }
}

static void test_rejects_excessive_tensor_count(void) {
    gpt2_gguf_info_t info;
    reset_blob();
    put_u32(GPT2_GGUF_MAGIC);
    put_u32(GPT2_GGUF_VERSION);
    put_u64(513U);
    put_u64(0U);
    TEST_ASSERT_EQUAL(-2, gpt2_gguf_probe_blob(blob, cursor, &info));
}

static void test_rejects_invalid_alignment(void) {
    gpt2_gguf_info_t info;
    reset_blob();
    put_u32(GPT2_GGUF_MAGIC);
    put_u32(GPT2_GGUF_VERSION);
    put_u64(0U);
    put_u64(2U);
    put_text("general.architecture");
    put_u32(GPT2_GGUF_VALUE_STRING);
    put_text("gpt2");
    put_text("general.alignment");
    put_u32(GPT2_GGUF_VALUE_UINT32);
    put_u32(3U);
    TEST_ASSERT_EQUAL(-5, gpt2_gguf_probe_blob(blob, cursor, &info));
}

int main(void) {
    unity_init();
    RUN_TEST(test_every_truncated_prefix_is_rejected);
    RUN_TEST(test_rejects_excessive_tensor_count);
    RUN_TEST(test_rejects_invalid_alignment);
    unity_print_results();
    unity_cleanup();
    return unity_stats.tests_failed == 0 ? 0 : 1;
}
