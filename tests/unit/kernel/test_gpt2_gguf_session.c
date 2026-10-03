/* GGUF 109/110 session (kernel/llm/gpt2_gguf_session.c): the code the Ring 0
 * fallback and the Ring 3 aiworker both run (encode, seed, step, decode).
 * The forward pass is a deterministic fake; the tokenizer is the real one
 * on a 16-piece fixture (as tests/scripts/ai_worker_fixture.py). */
#include "../../framework/unity.h"
#include <stdint.h>
#include <string.h>
#include "../../../kernel/llm/gpt2_gguf_session.h"
#include "../../../kernel/llm/gpt2_generate.h"
#include "../../../kernel/llm/gpt2_tokenizer.h"

#define V 16U
static uint8_t tokenizer[1024U + 64U];
static uint32_t tokenizer_size;
static int g_calls, g_fail_at;

static void load_tokenizer(void) {
    const char* pieces = "abcdefghijklmn ";
    uint32_t i;
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
    TEST_ASSERT_EQUAL(0, gpt2_tokenizer_load_from_buffer(tokenizer, tokenizer_size));
}

/* Deterministic stand-in for gpt2_gguf_generate_next_sampled(): depends on
 * the whole context, the generated count and the sampler state, and
 * advances the state, so any divergence in session bookkeeping shows. */
static int fake_next(void* context, const uint32_t* tokens, uint32_t token_count,
                     uint32_t generated_count, uint32_t* next_token, uint32_t* rng_state) {
    uint32_t h = *rng_state ^ (generated_count * 2654435761U), i;
    (void)context;
    g_calls++;
    if (g_fail_at && g_calls == g_fail_at) return 3;
    for (i = 0U; i < token_count; i++) h = h * 31U + tokens[i] + 7U;
    *rng_state = *rng_state * 1664525U + 1013904223U;
    *next_token = (h >> 7) % 15U; /* never EOT */
    return 0;
}

static int eot_next(void* context, const uint32_t* tokens, uint32_t token_count,
                    uint32_t generated_count, uint32_t* next_token, uint32_t* rng_state) {
    (void)context; (void)tokens; (void)token_count; (void)generated_count; (void)rng_state;
    *next_token = gpt2_tokenizer_eot();
    return 0;
}

static void test_start_encodes_and_seeds(void) {
    gpt2_gguf_session_t s;
    uint32_t direct[64], n = 0U, i;
    load_tokenizer();
    gpt2_gguf_session_reset(&s);
    TEST_ASSERT_EQUAL(0, gpt2_gguf_session_start(&s, "abc de", 1, 64U));
    TEST_ASSERT_EQUAL(0, gpt2_tokenizer_encode("abc de", direct, 64U, &n));
    TEST_ASSERT_EQUAL((int)n, (int)s.token_count);
    TEST_ASSERT_EQUAL((int)n, (int)s.prompt_tokens);
    for (i = 0U; i < n; i++) TEST_ASSERT_EQUAL((int)direct[i], (int)s.tokens[i]);
    TEST_ASSERT_EQUAL((int)gpt2_generate_seed("abc de"), (int)s.rng);
    TEST_ASSERT_EQUAL(1, (int)s.active);
    /* Runtime not ready or prompt longer than the context: -5, inactive. */
    TEST_ASSERT_EQUAL(-5, gpt2_gguf_session_start(&s, "abc", 0, 64U));
    TEST_ASSERT_EQUAL(0, (int)s.active);
    TEST_ASSERT_EQUAL(-5, gpt2_gguf_session_start(&s, "abcdef", 1, 3U));
    TEST_ASSERT_EQUAL(-1, gpt2_gguf_session_start(0, "abc", 1, 64U));
}

static void test_step_decodes_and_ends(void) {
    gpt2_gguf_session_t s;
    char out[8];
    int rc, steps = 0;
    load_tokenizer();
    TEST_ASSERT_EQUAL(-6, (gpt2_gguf_session_reset(&s), gpt2_gguf_session_step(&s, fake_next, 0, out, sizeof(out))));
    TEST_ASSERT_EQUAL(0, gpt2_gguf_session_start(&s, "ab", 1, 64U));
    TEST_ASSERT_EQUAL(-1, gpt2_gguf_session_step(&s, fake_next, 0, out, 1U));
    g_calls = 0; g_fail_at = 0;
    while ((rc = gpt2_gguf_session_step(&s, fake_next, 0, out, sizeof(out))) > 0) {
        TEST_ASSERT_EQUAL(1, rc);
        TEST_ASSERT_EQUAL_STRING(gpt2_tokenizer_decode(s.tokens[s.token_count - 1U]), out);
        steps++;
    }
    /* 64-token cap: 62 generated after a 2-token prompt, then the end. */
    TEST_ASSERT_EQUAL(0, rc);
    TEST_ASSERT_EQUAL(62, steps);
    TEST_ASSERT_EQUAL(64, (int)s.token_count);
    TEST_ASSERT_EQUAL(0, (int)s.active);
    TEST_ASSERT_EQUAL(-6, gpt2_gguf_session_step(&s, fake_next, 0, out, sizeof(out)));
    /* EOT ends the session without appending. */
    TEST_ASSERT_EQUAL(0, gpt2_gguf_session_start(&s, "ab", 1, 64U));
    TEST_ASSERT_EQUAL(0, gpt2_gguf_session_step(&s, eot_next, 0, out, sizeof(out)));
    TEST_ASSERT_EQUAL(2, (int)s.token_count);
    TEST_ASSERT_EQUAL(0, (int)s.active);
}

static void test_step_error_keeps_session(void) {
    gpt2_gguf_session_t s;
    char out[8];
    load_tokenizer();
    TEST_ASSERT_EQUAL(0, gpt2_gguf_session_start(&s, "ab", 1, 64U));
    g_calls = 0; g_fail_at = 1;
    TEST_ASSERT_EQUAL(-27, gpt2_gguf_session_step(&s, fake_next, 0, out, sizeof(out)));
    TEST_ASSERT_EQUAL(1, (int)s.active);
    TEST_ASSERT_EQUAL(2, (int)s.token_count);
    g_fail_at = 0;
    TEST_ASSERT_EQUAL(1, gpt2_gguf_session_step(&s, fake_next, 0, out, sizeof(out)));
}

/* The worker path: the worker owns the session, the kernel only mirrors
 * each reply. A worker restarted mid-session adopts the mirror. Both must
 * produce exactly the tokens of an uninterrupted (Ring 0) session. */
static void test_mirror_adoption_identical_tokens(void) {
    gpt2_gguf_session_t ring0, worker, mirror, fresh;
    char out[8], out2[8];
    int i, rc1, rc2;
    load_tokenizer();
    g_fail_at = 0;
    TEST_ASSERT_EQUAL(0, gpt2_gguf_session_start(&ring0, "abc de", 1, 64U));
    TEST_ASSERT_EQUAL(0, gpt2_gguf_session_start(&worker, "abc de", 1, 64U));
    for (i = 0; i < 5; i++) {
        rc1 = gpt2_gguf_session_step(&ring0, fake_next, 0, out, sizeof(out));
        rc2 = gpt2_gguf_session_step(&worker, fake_next, 0, out2, sizeof(out2));
        TEST_ASSERT_EQUAL(rc1, rc2);
        TEST_ASSERT_EQUAL_STRING(out, out2);
    }
    memcpy(&mirror, &worker, sizeof(mirror));      /* kernel mirror of the last reply */
    gpt2_gguf_session_reset(&fresh);               /* restarted worker: no session */
    memcpy(fresh.tokens, mirror.tokens, sizeof(fresh.tokens));
    fresh.token_count = mirror.token_count;
    fresh.prompt_tokens = mirror.prompt_tokens;
    fresh.rng = mirror.rng;
    fresh.active = 1U;
    for (i = 0; i < 5; i++) {
        rc1 = gpt2_gguf_session_step(&ring0, fake_next, 0, out, sizeof(out));
        rc2 = gpt2_gguf_session_step(&fresh, fake_next, 0, out2, sizeof(out2));
        TEST_ASSERT_EQUAL(rc1, rc2);
        TEST_ASSERT_EQUAL_STRING(out, out2);
    }
    TEST_ASSERT_EQUAL((int)ring0.token_count, (int)fresh.token_count);
    TEST_ASSERT_EQUAL(0, memcmp(ring0.tokens, fresh.tokens, sizeof(ring0.tokens)));
    TEST_ASSERT_EQUAL((int)ring0.rng, (int)fresh.rng);
}

int main(void) {
    unity_init();
    RUN_TEST(test_start_encodes_and_seeds);
    RUN_TEST(test_step_decodes_and_ends);
    RUN_TEST(test_step_error_keeps_session);
    RUN_TEST(test_mirror_adoption_identical_tokens);
    unity_print_results();
    unity_cleanup();
    return unity_stats.tests_failed == 0 ? 0 : 1;
}
