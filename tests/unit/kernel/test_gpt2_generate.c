/* Inventory item 4: shared FP32 generation loop (kernel/llm/gpt2_generate.c)
 * used by both the Ring 0 fallback and the Ring 3 aiworker, plus the
 * buffer loaders the worker uses on its read-only initrd mapping. Built
 * with -DMOHHDY_RING3 (no initrd), as in userspace/Makefile. */
#include "../../framework/unity.h"
#include <stdint.h>
#include <string.h>
#include "../../../kernel/llm/gpt2_generate.h"
#include "../../../kernel/llm/gpt2_model.h"
#include "../../../kernel/llm/gpt2_tokenizer.h"

/* T=32 V=16 L=2 heads=2 C=32, like tests/scripts/ai_worker_fixture.py. */
#define T 32U
#define V 16U
#define L 2U
#define C 32U
#define PARAMS (V * C + T * C + L * C + L * C + L * 3U * C * C + L * 3U * C + \
                L * C * C + L * C + L * C + L * C + L * 4U * C * C + L * 4U * C + \
                L * C * 4U * C + L * C + C + C)

static uint32_t checkpoint[256U + PARAMS];
static uint8_t tokenizer[1024U + 64U];
static uint32_t tokenizer_size;

static void build_fixture(void) {
    uint32_t i, state = 12345U;
    float* w = (float*)&checkpoint[256];
    const char* pieces = "abcdefghijklmn ";
    memset(checkpoint, 0, sizeof(checkpoint));
    checkpoint[0] = GPT2_CHECKPOINT_MAGIC;
    checkpoint[1] = GPT2_CHECKPOINT_VERSION;
    checkpoint[2] = T; checkpoint[3] = V; checkpoint[4] = L;
    checkpoint[5] = 2U; checkpoint[6] = C; checkpoint[7] = V;
    for (i = 0U; i < PARAMS; i++) {
        state = state * 1664525U + 1013904223U;
        w[i] = ((float)(state >> 8) / 16777216.0f) - 0.5f;
    }
    memset(tokenizer, 0, sizeof(tokenizer));
    ((uint32_t*)tokenizer)[0] = 20240328U;
    ((uint32_t*)tokenizer)[1] = 2U;
    ((uint32_t*)tokenizer)[2] = V;
    ((uint32_t*)tokenizer)[3] = 15U;
    tokenizer_size = 1024U;
    for (i = 0U; i < 15U; i++) {
        tokenizer[tokenizer_size++] = 1U;
        tokenizer[tokenizer_size++] = (uint8_t)pieces[i];
    }
    tokenizer[tokenizer_size++] = 3U;
    memcpy(&tokenizer[tokenizer_size], "EOT", 3U);
    tokenizer_size += 3U;
}

static void load_fixture(void) {
    build_fixture();
    TEST_ASSERT_EQUAL(0, gpt2_tokenizer_load_from_buffer(tokenizer, tokenizer_size));
    TEST_ASSERT_EQUAL(0, gpt2_model_load_from_buffer((const uint8_t*)checkpoint, sizeof(checkpoint)));
}

static void test_normalize_rules(void) {
    char out[GPT2_GENERATE_PROMPT_MAX];
    char big[400];
    TEST_ASSERT_EQUAL(7U, gpt2_generate_normalize("  ab\t\tc\n de\x01", 255U, out, sizeof(out)));
    TEST_ASSERT_EQUAL_STRING("ab c de", out);
    TEST_ASSERT_EQUAL(2U, gpt2_generate_normalize("abcdef", 2U, out, sizeof(out)));
    TEST_ASSERT_EQUAL_STRING("ab", out);
    memset(big, 'z', sizeof(big));
    big[sizeof(big) - 1] = '\0';
    TEST_ASSERT_EQUAL(GPT2_GENERATE_PROMPT_MAX - 1U, gpt2_generate_normalize(big, 255U, out, sizeof(out)));
    TEST_ASSERT_EQUAL(0U, gpt2_generate_normalize(0, 255U, out, sizeof(out)));
    TEST_ASSERT_EQUAL_STRING("", out);
}

/* Historical SYS_GPT2_GENERATE seed: 0x9e3779b9, x = x*16777619 + byte. */
static void test_seed_formula(void) {
    uint32_t x = 0x9e3779b9U;
    x = x * 16777619U + (uint8_t)'a';
    x = x * 16777619U + (uint8_t)'b';
    TEST_ASSERT_TRUE(x == gpt2_generate_seed("ab"));
    TEST_ASSERT_TRUE(0x9e3779b9U == gpt2_generate_seed(""));
}

/* Same model + prompt => same token ids and text (what the QEMU contract
 * compares between Ring 0 and Ring 3). */
static void test_generation_is_deterministic(void) {
    gpt2_generate_trace_t a, b;
    char out_a[384], out_b[384];
    int ra, rb;
    uint32_t i;
    load_fixture();
    ra = gpt2_generate_fp32("abc de", out_a, sizeof(out_a), &a);
    rb = gpt2_generate_fp32("abc de", out_b, sizeof(out_b), &b);
    TEST_ASSERT_TRUE(ra >= 0);
    TEST_ASSERT_EQUAL(ra, rb);
    TEST_ASSERT_EQUAL_STRING(out_a, out_b);
    TEST_ASSERT_EQUAL((uint32_t)ra, (uint32_t)strlen(out_a));
    TEST_ASSERT_EQUAL(6U, a.prompt_tokens);
    TEST_ASSERT_EQUAL(0U, a.tokens[0]);
    TEST_ASSERT_EQUAL(14U, a.tokens[3]); /* ' ' */
    TEST_ASSERT_EQUAL(a.token_count, b.token_count);
    TEST_ASSERT_TRUE(a.token_count <= a.prompt_tokens + GPT2_GENERATE_STEPS);
    for (i = 0U; i < a.token_count; i++) TEST_ASSERT_EQUAL(a.tokens[i], b.tokens[i]);
}

static void test_output_bound_and_errors(void) {
    gpt2_generate_trace_t t;
    char out[384];
    char long_prompt[40];
    load_fixture();
    TEST_ASSERT_EQUAL(-1, gpt2_generate_fp32("abc", out, 1U, &t));
    TEST_ASSERT_EQUAL(-1, gpt2_generate_fp32(0, out, sizeof(out), &t));
    TEST_ASSERT_TRUE(gpt2_generate_fp32("abc de", out, 3U, &t) <= 2);
    TEST_ASSERT_TRUE(strlen(out) <= 2U);
    memset(long_prompt, 'a', sizeof(long_prompt));
    long_prompt[sizeof(long_prompt) - 1] = '\0';
    TEST_ASSERT_EQUAL(-5, gpt2_generate_fp32(long_prompt, out, sizeof(out), &t)); /* > T tokens */
    checkpoint[0] = 1U;
    TEST_ASSERT_EQUAL(-2, gpt2_model_load_from_buffer((const uint8_t*)checkpoint, sizeof(checkpoint)));
    TEST_ASSERT_EQUAL(-5, gpt2_generate_fp32("abc", out, sizeof(out), &t));
    load_fixture();
    TEST_ASSERT_EQUAL(-5, gpt2_model_load_from_buffer((const uint8_t*)checkpoint, sizeof(checkpoint) - 4U));
    TEST_ASSERT_EQUAL(-1, gpt2_model_load_from_buffer(0, 0U));
    TEST_ASSERT_EQUAL(-1, gpt2_tokenizer_load_from_buffer(0, 0U));
    TEST_ASSERT_EQUAL(0, gpt2_tokenizer_ready());
    TEST_ASSERT_EQUAL(-2, gpt2_generate_fp32("abc", out, sizeof(out), &t));
}

/* Liveness / cancel hook: called once per layer and once per token; a
 * non-zero answer stops the token loop with GPT2_GENERATE_CANCELLED. */
static uint32_t hook_calls;
static uint32_t hook_cancel_at;

static int test_hook(void) {
    hook_calls++;
    return hook_cancel_at != 0U && hook_calls >= hook_cancel_at;
}

static void test_progress_hook_and_cancel(void) {
    gpt2_generate_trace_t t, ref;
    char out[384], ref_out[384];
    int rc, ref_rc;
    load_fixture();
    gpt2_progress_hook = 0;
    ref_rc = gpt2_generate_fp32("abc de", ref_out, sizeof(ref_out), &ref);
    TEST_ASSERT_TRUE(ref_rc >= 0);
    /* Observing hook: same tokens, called at least once per layer. */
    hook_calls = 0U;
    hook_cancel_at = 0U;
    gpt2_progress_hook = test_hook;
    rc = gpt2_generate_fp32("abc de", out, sizeof(out), &t);
    TEST_ASSERT_EQUAL(ref_rc, rc);
    TEST_ASSERT_EQUAL_STRING(ref_out, out);
    TEST_ASSERT_TRUE(hook_calls >= gpt2_model_current()->config.num_layers);
    /* Cancel at the first token boundary. */
    hook_calls = 0U;
    hook_cancel_at = 1U;
    TEST_ASSERT_EQUAL(GPT2_GENERATE_CANCELLED, gpt2_generate_fp32("abc de", out, sizeof(out), &t));
    TEST_ASSERT_EQUAL(1U, hook_calls);
    /* Cancel mid-generation: the layer calls ignore it, the next token stops. */
    hook_calls = 0U;
    hook_cancel_at = 3U;
    TEST_ASSERT_EQUAL(GPT2_GENERATE_CANCELLED, gpt2_generate_fp32("abc de", out, sizeof(out), &t));
    gpt2_progress_hook = 0;
}

int main(void) {
    unity_init();
    RUN_TEST(test_normalize_rules);
    RUN_TEST(test_seed_formula);
    RUN_TEST(test_generation_is_deterministic);
    RUN_TEST(test_output_bound_and_errors);
    RUN_TEST(test_progress_hook_and_cancel);
    unity_print_results();
    unity_cleanup();
    return unity_stats.tests_failed == 0 ? 0 : 1;
}
