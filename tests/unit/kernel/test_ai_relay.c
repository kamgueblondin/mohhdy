/* Inventory item 4: AI relay slot (kernel/ai_relay.c).
 *
 * The kernel glue (kernel/syscall/syscall.c: doorbell, caller block/wake,
 * IRQ0 watchdog, OS_AI_ENGINE_MAP) only maps these decisions to task
 * states; the QEMU contract qemu-ai-worker checks the integrated path. */
#include "../../framework/unity.h"
#include <stdint.h>
#include <string.h>
#include "../../../include/os_syscalls.h"
#include "../../../kernel/ai_relay.c"

static os_ai_engine_reply_t reply;
static os_ai_engine_job_t job;
static os_ai_engine_status_t st;

static void make_reply(uint32_t job_id, const char* text) {
    uint32_t n = (uint32_t)strlen(text);
    memset(&reply, 0, sizeof(reply));
    reply.job_id = job_id;
    reply.result = (int32_t)n;
    reply.text_length = n;
    memcpy(reply.text, text, n);
    reply.prompt_tokens = 2U;
    reply.token_count = 4U;
    reply.tokens[0] = 1U; reply.tokens[1] = 2U; reply.tokens[2] = 3U; reply.tokens[3] = 4U;
}

/* begin -> fetch -> complete -> take, job ids increase, prompt and max bounded. */
static void test_happy_path_and_bounds(void) {
    char long_prompt[300];
    int32_t a, b;
    ai_relay_init();
    a = ai_relay_begin(5, 9, "abc de", 384U, 100U);
    TEST_ASSERT_TRUE(a > 0);
    TEST_ASSERT_EQUAL(AI_RELAY_SENT, ai_relay_state());
    TEST_ASSERT_EQUAL(5, ai_relay_caller());
    TEST_ASSERT_EQUAL(9, ai_relay_worker());
    TEST_ASSERT_EQUAL(0, ai_relay_fetch(9, 9, (uint32_t)a, &job));
    TEST_ASSERT_EQUAL_STRING("abc de", job.prompt);
    TEST_ASSERT_EQUAL(6U, job.prompt_length);
    TEST_ASSERT_EQUAL(384U, job.max);
    make_reply((uint32_t)a, "dab");
    TEST_ASSERT_EQUAL(0, ai_relay_complete(9, 9, &reply));
    TEST_ASSERT_EQUAL(AI_RELAY_DONE, ai_relay_state());
    memset(&reply, 0, sizeof(reply));
    TEST_ASSERT_EQUAL(AI_RELAY_FREE, ai_relay_take(6, &reply)); /* not the caller */
    TEST_ASSERT_EQUAL(AI_RELAY_DONE, ai_relay_take(5, &reply));
    TEST_ASSERT_EQUAL_STRING("dab", reply.text);
    TEST_ASSERT_EQUAL(4U, reply.token_count);
    TEST_ASSERT_EQUAL(AI_RELAY_FREE, ai_relay_state());

    memset(long_prompt, 'x', sizeof(long_prompt));
    long_prompt[sizeof(long_prompt) - 1] = '\0';
    b = ai_relay_begin(5, 9, long_prompt, 100000U, 200U);
    TEST_ASSERT_EQUAL(a + 1, b);
    TEST_ASSERT_EQUAL(0, ai_relay_fetch(9, 9, (uint32_t)b, &job));
    TEST_ASSERT_EQUAL(OS_AI_ENGINE_PROMPT_MAX - 1U, job.prompt_length);
    TEST_ASSERT_EQUAL(OS_AI_ENGINE_TEXT_MAX, job.max);
    ai_relay_fill_status(&st, 9);
    TEST_ASSERT_EQUAL(2U, st.forwarded);
    TEST_ASSERT_EQUAL(1U, st.completed);
    TEST_ASSERT_EQUAL(1U, st.pending);
    TEST_ASSERT_EQUAL(9, st.worker_pid);
}

/* A reply from any task that is not the live worker is refused and
 * counted; the job stays in flight for the real worker. */
static void test_rogue_reply_refused(void) {
    int32_t j;
    ai_relay_init();
    j = ai_relay_begin(5, 9, "abc", 64U, 0U);
    make_reply((uint32_t)j, "bad");
    TEST_ASSERT_EQUAL(OS_AI_ENGINE_REQUIRED, ai_relay_complete(7, 9, &reply));
    TEST_ASSERT_EQUAL(OS_AI_ENGINE_REQUIRED, ai_relay_complete(9, 0, &reply)); /* no live worker */
    TEST_ASSERT_EQUAL(OS_AI_ENGINE_REQUIRED, ai_relay_fetch(7, 9, (uint32_t)j, &job));
    TEST_ASSERT_EQUAL(AI_RELAY_SENT, ai_relay_state());
    ai_relay_fill_status(&st, 9);
    TEST_ASSERT_EQUAL(2U, st.rogue_refused);
    make_reply((uint32_t)j, "ok");
    TEST_ASSERT_EQUAL(0, ai_relay_complete(9, 9, &reply));
    TEST_ASSERT_EQUAL(AI_RELAY_DONE, ai_relay_take(5, &reply));
    TEST_ASSERT_EQUAL_STRING("ok", reply.text);
}

/* Wrong job id, a second reply and a reply with nothing in flight are stale. */
static void test_stale_replies_refused(void) {
    int32_t j;
    ai_relay_init();
    make_reply(1U, "x");
    TEST_ASSERT_EQUAL(OS_AI_ENGINE_STALE, ai_relay_complete(9, 9, &reply));
    j = ai_relay_begin(5, 9, "abc", 64U, 0U);
    make_reply((uint32_t)j + 1U, "x");
    TEST_ASSERT_EQUAL(OS_AI_ENGINE_STALE, ai_relay_complete(9, 9, &reply));
    TEST_ASSERT_EQUAL(OS_AI_ENGINE_STALE, ai_relay_fetch(9, 9, (uint32_t)j + 1U, &job));
    make_reply((uint32_t)j, "x");
    TEST_ASSERT_EQUAL(0, ai_relay_complete(9, 9, &reply));
    TEST_ASSERT_EQUAL(OS_AI_ENGINE_STALE, ai_relay_complete(9, 9, &reply)); /* replay */
    ai_relay_fill_status(&st, 9);
    TEST_ASSERT_EQUAL(4U, st.stale_refused);
    TEST_ASSERT_EQUAL(0U, st.rogue_refused);
}

/* Malformed replies are refused without consuming the job. */
static void test_malformed_reply(void) {
    int32_t j;
    ai_relay_init();
    j = ai_relay_begin(5, 9, "abc", 8U, 0U);
    make_reply((uint32_t)j, "12345678"); /* 8 bytes do not fit max 8 */
    TEST_ASSERT_EQUAL(OS_AI_ENGINE_BAD_ARGUMENT, ai_relay_complete(9, 9, &reply));
    make_reply((uint32_t)j, "ab");
    reply.token_count = OS_AI_ENGINE_TOKENS_MAX + 1U;
    TEST_ASSERT_EQUAL(OS_AI_ENGINE_BAD_ARGUMENT, ai_relay_complete(9, 9, &reply));
    make_reply((uint32_t)j, "ab");
    reply.result = 5;
    TEST_ASSERT_EQUAL(OS_AI_ENGINE_BAD_ARGUMENT, ai_relay_complete(9, 9, &reply));
    make_reply((uint32_t)j, "ab");
    reply.prompt_tokens = 9U;
    TEST_ASSERT_EQUAL(OS_AI_ENGINE_BAD_ARGUMENT, ai_relay_complete(9, 9, &reply));
    TEST_ASSERT_EQUAL(AI_RELAY_SENT, ai_relay_state());
    TEST_ASSERT_EQUAL(OS_AI_ENGINE_BAD_ARGUMENT, ai_relay_complete(9, 9, 0));
    /* A worker-side generation error is a valid reply (no text). */
    make_reply((uint32_t)j, "");
    reply.result = -5;
    TEST_ASSERT_EQUAL(0, ai_relay_complete(9, 9, &reply));
    TEST_ASSERT_EQUAL(AI_RELAY_DONE, ai_relay_take(5, &reply));
    TEST_ASSERT_EQUAL(-5, reply.result);
}

/* Worker lost, replaced or stalled: the job fails once, the caller gets
 * AI_RELAY_FAILED (Ring 0 fallback) and a late reply is stale. */
static void test_worker_loss_and_timeout(void) {
    int32_t j;
    ai_relay_init();
    j = ai_relay_begin(5, 9, "abc", 64U, 1000U);
    TEST_ASSERT_EQUAL(0, ai_relay_should_fail(9, 1000U + AI_RELAY_TIMEOUT_TICKS));
    TEST_ASSERT_EQUAL(1, ai_relay_should_fail(0, 1001U));
    TEST_ASSERT_EQUAL(1, ai_relay_should_fail(11, 1001U));
    TEST_ASSERT_EQUAL(1, ai_relay_should_fail(9, 1001U + AI_RELAY_TIMEOUT_TICKS));
    ai_relay_fail();
    ai_relay_fail(); /* only once */
    TEST_ASSERT_EQUAL(0, ai_relay_should_fail(0, 2000U));
    make_reply((uint32_t)j, "late");
    TEST_ASSERT_EQUAL(OS_AI_ENGINE_STALE, ai_relay_complete(9, 9, &reply));
    TEST_ASSERT_EQUAL(AI_RELAY_FAILED, ai_relay_take(5, &reply));
    TEST_ASSERT_EQUAL(AI_RELAY_FREE, ai_relay_state());
    ai_relay_fill_status(&st, 0);
    TEST_ASSERT_EQUAL(1U, st.aborted);
    TEST_ASSERT_EQUAL(0U, st.completed);
    TEST_ASSERT_EQUAL(0, st.worker_pid);
    /* Tick wrap does not fake a timeout. */
    j = ai_relay_begin(5, 9, "abc", 64U, 0xFFFFFFF0U);
    TEST_ASSERT_TRUE(j > 0);
    TEST_ASSERT_EQUAL(0, ai_relay_should_fail(9, 0x00000010U));
}

/* One job at a time; bad begins; cancel; caller death. */
static void test_slot_rules(void) {
    int32_t j;
    ai_relay_init();
    TEST_ASSERT_EQUAL(-1, ai_relay_begin(5, 5, "abc", 64U, 0U)); /* worker calling itself */
    TEST_ASSERT_EQUAL(-1, ai_relay_begin(0, 9, "abc", 64U, 0U));
    TEST_ASSERT_EQUAL(-1, ai_relay_begin(5, 0, "abc", 64U, 0U));
    TEST_ASSERT_EQUAL(-1, ai_relay_begin(5, 9, 0, 64U, 0U));
    TEST_ASSERT_EQUAL(-1, ai_relay_begin(5, 9, "abc", 1U, 0U));
    j = ai_relay_begin(5, 9, "abc", 64U, 0U);
    TEST_ASSERT_TRUE(j > 0);
    TEST_ASSERT_EQUAL(-1, ai_relay_begin(6, 9, "abc", 64U, 0U)); /* busy */
    ai_relay_cancel();
    ai_relay_fill_status(&st, 9);
    TEST_ASSERT_EQUAL(0U, st.forwarded);
    TEST_ASSERT_EQUAL(0U, st.pending);
    j = ai_relay_begin(6, 9, "abc", 64U, 0U);
    TEST_ASSERT_TRUE(j > 0);
    ai_relay_drop_caller();
    TEST_ASSERT_EQUAL(AI_RELAY_FREE, ai_relay_state());
    make_reply((uint32_t)j, "x");
    TEST_ASSERT_EQUAL(OS_AI_ENGINE_STALE, ai_relay_complete(9, 9, &reply));
    ai_relay_fill_status(&st, 9);
    TEST_ASSERT_EQUAL(1U, st.aborted);
}

/* Ring 0 accounting: the exclusion counter only moves for a kernel run
 * with a live worker and no relay failure. */
static void test_kernel_accounting_and_trace(void) {
    uint32_t tokens[3] = {7U, 8U, 9U};
    ai_relay_init();
    ai_relay_note_kernel_infer(0, 0);
    ai_relay_note_kernel_infer(1, 1);
    ai_relay_record_last(OS_AI_PATH_KERNEL_FALLBACK, 3, tokens, 1U, 3U);
    ai_relay_note_mapped(OS_AI_ENGINE_BLOB_CHECKPOINT, 109056U);
    ai_relay_note_mapped(OS_AI_ENGINE_BLOB_TOKENIZER, 1068U);
    ai_relay_fill_status(&st, 9);
    TEST_ASSERT_EQUAL(2U, st.kernel_infer);
    TEST_ASSERT_EQUAL(1U, st.fallbacks);
    TEST_ASSERT_EQUAL(0U, st.kernel_infer_while_live);
    TEST_ASSERT_EQUAL(OS_AI_PATH_KERNEL_FALLBACK, st.last_path);
    TEST_ASSERT_EQUAL(3U, st.last_token_count);
    TEST_ASSERT_EQUAL(1U, st.last_prompt_tokens);
    TEST_ASSERT_EQUAL(9U, st.last_tokens[2]);
    TEST_ASSERT_EQUAL(0U, st.last_tokens[3]);
    TEST_ASSERT_EQUAL(109056U, st.checkpoint_mapped);
    TEST_ASSERT_EQUAL(1068U, st.tokenizer_mapped);
    ai_relay_note_kernel_infer(1, 0);
    ai_relay_worker_reset();
    ai_relay_fill_status(&st, 9);
    TEST_ASSERT_EQUAL(1U, st.kernel_infer_while_live);
    TEST_ASSERT_EQUAL(0U, st.checkpoint_mapped);
}

/* GGUF slice: one 109/110 sampling step relayed with the whole session
 * state; kind checked on reply; separate counters; GGUF worker identity. */
static void test_gguf_step(void) {
    uint32_t toks[4] = {0U, 1U, 2U, 14U};
    int32_t j, k;
    ai_relay_init();
    TEST_ASSERT_EQUAL(-1, ai_relay_begin_gguf(5, 5, toks, 4U, 0U, 7U, 1U)); /* worker as caller */
    TEST_ASSERT_EQUAL(-1, ai_relay_begin_gguf(5, 9, toks, 0U, 0U, 7U, 1U));
    TEST_ASSERT_EQUAL(-1, ai_relay_begin_gguf(5, 9, toks, OS_AI_ENGINE_TOKENS_MAX + 1U, 0U, 7U, 1U));
    TEST_ASSERT_EQUAL(-1, ai_relay_begin_gguf(5, 9, toks, 4U, 5U, 7U, 1U)); /* generated > count */
    j = ai_relay_begin_gguf(5, 9, toks, 4U, 1U, 12345U, 10U);
    TEST_ASSERT_TRUE(j > 0);
    TEST_ASSERT_EQUAL(OS_AI_JOB_GGUF_STEP, ai_relay_kind());
    TEST_ASSERT_EQUAL(-1, ai_relay_begin(6, 9, "x", 8U, 11U)); /* single slot */
    TEST_ASSERT_EQUAL(0, ai_relay_fetch(9, 9, (uint32_t)j, &job));
    TEST_ASSERT_EQUAL(OS_AI_JOB_GGUF_STEP, job.kind);
    TEST_ASSERT_EQUAL(12345U, job.rng_state);
    TEST_ASSERT_EQUAL(1U, job.generated);
    TEST_ASSERT_EQUAL(4U, job.token_count);
    TEST_ASSERT_EQUAL(14U, job.tokens[3]);
    /* An FP32-shaped reply (text, tokens, wrong kind) is malformed here. */
    make_reply((uint32_t)j, "dab");
    TEST_ASSERT_EQUAL(OS_AI_ENGINE_BAD_ARGUMENT, ai_relay_complete(9, 9, &reply));
    memset(&reply, 0, sizeof(reply));
    reply.job_id = (uint32_t)j;
    reply.kind = OS_AI_JOB_GGUF_STEP;
    reply.result = 1;
    TEST_ASSERT_EQUAL(OS_AI_ENGINE_BAD_ARGUMENT, ai_relay_complete(9, 9, &reply)); /* result > 0 */
    reply.result = 0;
    reply.next_token = 6U;
    reply.rng_state = 777U;
    TEST_ASSERT_EQUAL(OS_AI_ENGINE_REQUIRED, ai_relay_complete(8, 9, &reply)); /* rogue */
    TEST_ASSERT_EQUAL(0, ai_relay_complete(9, 9, &reply));
    memset(&reply, 0, sizeof(reply));
    TEST_ASSERT_EQUAL(AI_RELAY_DONE, ai_relay_take(5, &reply));
    TEST_ASSERT_EQUAL(6U, reply.next_token);
    TEST_ASSERT_EQUAL(777U, reply.rng_state);
    TEST_ASSERT_EQUAL(0U, ai_relay_kind());

    /* Second step: worker lost -> FAILED, caller runs the Ring 0 step. */
    k = ai_relay_begin_gguf(5, 9, toks, 4U, 2U, 777U, 20U);
    TEST_ASSERT_EQUAL(j + 1, k);
    TEST_ASSERT_EQUAL(1, ai_relay_should_fail(0, 21U));
    ai_relay_fail();
    TEST_ASSERT_EQUAL(AI_RELAY_FAILED, ai_relay_take(5, &reply));
    ai_relay_note_gguf_kernel(0, 1);
    ai_relay_note_gguf_kernel(0, 0);
    /* Cancelled doorbell is not counted. */
    k = ai_relay_begin_gguf(5, 9, toks, 4U, 2U, 777U, 30U);
    ai_relay_cancel();
    TEST_ASSERT_EQUAL(AI_RELAY_FREE, ai_relay_state());

    ai_relay_set_gguf_worker(9, 4024704U);
    TEST_ASSERT_EQUAL(9, ai_relay_gguf_worker(9));
    TEST_ASSERT_EQUAL(0, ai_relay_gguf_worker(10)); /* another ai-engine owner */
    TEST_ASSERT_EQUAL(0, ai_relay_gguf_worker(0));
    ai_relay_note_gguf_kernel(1, 0);
    ai_relay_record_gguf(OS_AI_PATH_KERNEL_FALLBACK, 0, toks, 3U, 4U);
    ai_relay_record_gguf(OS_AI_PATH_NONE, 0, toks, 3U, 3U); /* snapshot only */
    ai_relay_fill_status(&st, 9);
    TEST_ASSERT_EQUAL(9, st.gguf_worker_pid);
    TEST_ASSERT_EQUAL(4024704U, st.gguf_bytes_loaded);
    TEST_ASSERT_EQUAL(2U, st.gguf_forwarded);
    TEST_ASSERT_EQUAL(1U, st.gguf_completed);
    TEST_ASSERT_EQUAL(0U, st.forwarded);
    TEST_ASSERT_EQUAL(0U, st.completed);
    TEST_ASSERT_EQUAL(1U, st.aborted);
    TEST_ASSERT_EQUAL(3U, st.gguf_kernel);
    TEST_ASSERT_EQUAL(1U, st.gguf_fallbacks);
    TEST_ASSERT_EQUAL(1U, st.gguf_kernel_while_live);
    TEST_ASSERT_EQUAL(OS_AI_PATH_KERNEL_FALLBACK, st.gguf_last_path);
    TEST_ASSERT_EQUAL(3U, st.gguf_token_count);
    TEST_ASSERT_EQUAL(3U, st.gguf_prompt_tokens);
    TEST_ASSERT_EQUAL(2U, st.gguf_tokens[2]);
    TEST_ASSERT_EQUAL(1U, st.rogue_refused);
    ai_relay_fill_status(&st, 10);
    TEST_ASSERT_EQUAL(0, st.gguf_worker_pid);
    TEST_ASSERT_EQUAL(0U, st.gguf_bytes_loaded);
    ai_relay_worker_reset();
    ai_relay_fill_status(&st, 9);
    TEST_ASSERT_EQUAL(0, st.gguf_worker_pid);
    /* GGUF window below the tokenizer window, chunks bounded. */
    TEST_ASSERT_TRUE(OS_AI_ENGINE_TOKENIZER_WINDOW + OS_AI_ENGINE_TOKENIZER_WINDOW_MAX <=
                     OS_AI_ENGINE_GGUF_WINDOW);
    TEST_ASSERT_TRUE(OS_AI_ENGINE_GGUF_WINDOW + OS_AI_ENGINE_GGUF_WINDOW_MAX <= 0xB0000000U - 16U * 4096U);
    TEST_ASSERT_TRUE(OS_AI_ENGINE_GGUF_CHUNK_MAX <= OS_AI_ENGINE_GGUF_WINDOW_MAX);
}

/* GGUF session slice: START carries the normalised prompt, STEP carries
 * the kernel mirror; the reply (text + tokens + rng + active) is bounded
 * before it becomes the mirror; worker sessions and resumes are counted. */
static void test_gguf_session(void) {
    uint32_t toks[4] = {0U, 1U, 2U, 14U};
    int32_t j, k;
    ai_relay_init();
    TEST_ASSERT_EQUAL(-1, ai_relay_begin_gguf_session(5, 9, 3U, 1U, "ab", 64U, 0, 0U, 0U, 0U, 0U));
    TEST_ASSERT_EQUAL(-1, ai_relay_begin_gguf_session(5, 9, OS_AI_GGUF_SESSION_START, 1U, 0, 64U, 0, 0U, 0U, 0U, 0U));
    TEST_ASSERT_EQUAL(-1, ai_relay_begin_gguf_session(5, 5, OS_AI_GGUF_SESSION_START, 1U, "ab", 64U, 0, 0U, 0U, 0U, 0U));
    TEST_ASSERT_EQUAL(-1, ai_relay_begin_gguf_session(5, 9, OS_AI_GGUF_SESSION_STEP, 1U, 0, 64U, toks, 0U, 0U, 0U, 0U));
    TEST_ASSERT_EQUAL(-1, ai_relay_begin_gguf_session(5, 9, OS_AI_GGUF_SESSION_STEP, 1U, 0, 64U, toks, 2U, 3U, 0U, 0U));
    j = ai_relay_begin_gguf_session(5, 9, OS_AI_GGUF_SESSION_START, 7U, "bonjour", 64U, 0, 0U, 0U, 0U, 1U);
    TEST_ASSERT_TRUE(j > 0);
    TEST_ASSERT_EQUAL(OS_AI_JOB_GGUF_SESSION, ai_relay_kind());
    TEST_ASSERT_EQUAL(0, ai_relay_fetch(9, 9, (uint32_t)j, &job));
    TEST_ASSERT_EQUAL(OS_AI_GGUF_SESSION_START, job.session_op);
    TEST_ASSERT_EQUAL(7U, job.session_id);
    TEST_ASSERT_EQUAL_STRING("bonjour", job.prompt);
    TEST_ASSERT_EQUAL(0U, job.token_count);
    /* Malformed session replies never reach the mirror. */
    memset(&reply, 0, sizeof(reply));
    reply.job_id = (uint32_t)j;
    reply.kind = OS_AI_JOB_GGUF_STEP;
    TEST_ASSERT_EQUAL(OS_AI_ENGINE_BAD_ARGUMENT, ai_relay_complete(9, 9, &reply)); /* wrong kind */
    reply.kind = OS_AI_JOB_GGUF_SESSION;
    reply.result = 1;
    reply.text_length = 1U;
    reply.text[0] = 'a';
    reply.token_count = OS_AI_ENGINE_TOKENS_MAX + 1U;
    TEST_ASSERT_EQUAL(OS_AI_ENGINE_BAD_ARGUMENT, ai_relay_complete(9, 9, &reply));
    reply.token_count = 4U;
    reply.prompt_tokens = 5U;
    TEST_ASSERT_EQUAL(OS_AI_ENGINE_BAD_ARGUMENT, ai_relay_complete(9, 9, &reply));
    reply.prompt_tokens = 3U;
    reply.session_active = 2U;
    TEST_ASSERT_EQUAL(OS_AI_ENGINE_BAD_ARGUMENT, ai_relay_complete(9, 9, &reply));
    reply.session_active = 1U;
    reply.result = 2; /* != text_length */
    TEST_ASSERT_EQUAL(OS_AI_ENGINE_BAD_ARGUMENT, ai_relay_complete(9, 9, &reply));
    reply.result = 1;
    reply.text_length = 64U; /* >= max */
    TEST_ASSERT_EQUAL(OS_AI_ENGINE_BAD_ARGUMENT, ai_relay_complete(9, 9, &reply));
    reply.text_length = 1U;
    memcpy(reply.tokens, toks, sizeof(toks));
    reply.rng_state = 4242U;
    TEST_ASSERT_EQUAL(0, ai_relay_complete(9, 9, &reply));
    memset(&reply, 0, sizeof(reply));
    TEST_ASSERT_EQUAL(AI_RELAY_DONE, ai_relay_take(5, &reply));
    TEST_ASSERT_EQUAL(1U, reply.session_active);
    TEST_ASSERT_EQUAL(4242U, reply.rng_state);
    TEST_ASSERT_EQUAL(14U, reply.tokens[3]);
    TEST_ASSERT_EQUAL_STRING("a", reply.text);

    /* STEP carries the mirror; a resumed reply is counted. */
    k = ai_relay_begin_gguf_session(5, 9, OS_AI_GGUF_SESSION_STEP, 7U, 0, 64U, toks, 4U, 3U, 4242U, 2U);
    TEST_ASSERT_EQUAL(j + 1, k);
    TEST_ASSERT_EQUAL(0, ai_relay_fetch(9, 9, (uint32_t)k, &job));
    TEST_ASSERT_EQUAL(OS_AI_GGUF_SESSION_STEP, job.session_op);
    TEST_ASSERT_EQUAL(4U, job.token_count);
    TEST_ASSERT_EQUAL(3U, job.prompt_tokens);
    TEST_ASSERT_EQUAL(1U, job.generated);
    TEST_ASSERT_EQUAL(4242U, job.rng_state);
    TEST_ASSERT_EQUAL(14U, job.tokens[3]);
    TEST_ASSERT_EQUAL(0U, job.prompt_length);
    memset(&reply, 0, sizeof(reply));
    reply.job_id = (uint32_t)k;
    reply.kind = OS_AI_JOB_GGUF_SESSION;
    reply.result = 0; /* end of session */
    reply.token_count = 4U;
    reply.prompt_tokens = 3U;
    reply.session_resumed = 1U;
    TEST_ASSERT_EQUAL(0, ai_relay_complete(9, 9, &reply));
    TEST_ASSERT_EQUAL(AI_RELAY_DONE, ai_relay_take(5, &reply));
    /* Worker lost mid-step: FAILED, the caller falls back to Ring 0. */
    k = ai_relay_begin_gguf_session(5, 9, OS_AI_GGUF_SESSION_STEP, 7U, 0, 64U, toks, 4U, 3U, 1U, 3U);
    TEST_ASSERT_EQUAL(1, ai_relay_should_fail(0, 4U));
    ai_relay_fail();
    TEST_ASSERT_EQUAL(AI_RELAY_FAILED, ai_relay_take(5, &reply));
    ai_relay_note_gguf_session_kernel();
    ai_relay_fill_status(&st, 9);
    TEST_ASSERT_EQUAL(3U, st.gguf_forwarded);
    TEST_ASSERT_EQUAL(2U, st.gguf_completed);
    TEST_ASSERT_EQUAL(2U, st.gguf_session_worker);
    TEST_ASSERT_EQUAL(1U, st.gguf_session_resumed);
    TEST_ASSERT_EQUAL(1U, st.gguf_session_kernel);
    TEST_ASSERT_EQUAL(1U, st.aborted);
    TEST_ASSERT_EQUAL(0U, st.forwarded);
}

/* ABI: syscall number, distinct errors, windows clear of the user ELF
 * (0x40000000) and of the user stack (16 pages below 0xB0000000). */
static void test_abi_constants(void) {
    TEST_ASSERT_EQUAL(155, SYS_AI_ENGINE);
    TEST_ASSERT_EQUAL(156, MAX_SYSCALLS);
    TEST_ASSERT_TRUE(OS_AI_ENGINE_REQUIRED != OS_AI_ENGINE_STALE);
    TEST_ASSERT_TRUE(OS_AI_ENGINE_BAD_ARGUMENT != OS_AI_ENGINE_NO_MODEL);
    TEST_ASSERT_TRUE(OS_AI_ENGINE_REQUIRED < OS_ATA_FS_BUSY);
    TEST_ASSERT_TRUE(OS_AI_ENGINE_CHECKPOINT_WINDOW >= 0x40000000U + 0x20000000U);
    TEST_ASSERT_TRUE(OS_AI_ENGINE_CHECKPOINT_WINDOW + OS_AI_ENGINE_CHECKPOINT_WINDOW_MAX <=
                     OS_AI_ENGINE_TOKENIZER_WINDOW);
    TEST_ASSERT_TRUE(OS_AI_ENGINE_TOKENIZER_WINDOW + OS_AI_ENGINE_TOKENIZER_WINDOW_MAX <=
                     0xB0000000U - 16U * 4096U);
    /* GPT-2 124M FP32 (124439808 params + 1 KiB header) fits the window. */
    TEST_ASSERT_TRUE(1024U + 124439808U * 4U < OS_AI_ENGINE_CHECKPOINT_WINDOW_MAX);
    TEST_ASSERT_TRUE(sizeof(os_ai_engine_job_t) <= 4096U);
    TEST_ASSERT_TRUE(3U * sizeof(uint32_t) <= OS_IPC_MAX_DATA); /* doorbell */
}

int main(void) {
    unity_init();
    RUN_TEST(test_happy_path_and_bounds);
    RUN_TEST(test_rogue_reply_refused);
    RUN_TEST(test_stale_replies_refused);
    RUN_TEST(test_malformed_reply);
    RUN_TEST(test_worker_loss_and_timeout);
    RUN_TEST(test_slot_rules);
    RUN_TEST(test_kernel_accounting_and_trace);
    RUN_TEST(test_gguf_step);
    RUN_TEST(test_gguf_session);
    RUN_TEST(test_abi_constants);
    unity_print_results();
    unity_cleanup();
    return unity_stats.tests_failed == 0 ? 0 : 1;
}
