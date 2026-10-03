/* ggufclient: one SYS_GPT2_GGUF_GENERATE (109, fixed prompt) then
 * SYS_GPT2_GGUF_CONTINUE (110) until the session ends (at most 7 more
 * steps). Prints the path of every step (kernel / worker / fallback), the
 * kernel session token ids and the GGUF relay counters. */
#include "ai_common.h"

static const char* path_name(unsigned int path) {
    if (path == OS_AI_PATH_KERNEL) return "kernel";
    if (path == OS_AI_PATH_WORKER) return "worker";
    if (path == OS_AI_PATH_KERNEL_FALLBACK) return "fallback";
    return "none";
}

static int gguf_call(unsigned int number, const char* prompt, char* out, int max) {
    int result;
    asm volatile("int $0x80" : "=a"(result)
                 : "a"(number), "b"(prompt), "c"(out), "d"(max) : "memory");
    return result;
}

int main(void) {
    static char out[128];
    static os_ai_engine_status_t st;
    ai_line_t l;
    unsigned int i, step;
    int rc;
    ai_puts("ggufclient start\n");
    for (step = 0U; step < 8U; step++) {
        rc = step == 0U ? gguf_call(SYS_GPT2_GGUF_GENERATE, "abc de", out, (int)sizeof(out))
                        : gguf_call(SYS_GPT2_GGUF_CONTINUE, 0, out, (int)sizeof(out));
        (void)ai_engine(OS_AI_ENGINE_STATUS, (unsigned int)&st, 0U);
        ai_line_reset(&l);
        ai_line_add(&l, "ggufclient step ");
        ai_line_int(&l, (int)step);
        ai_line_add(&l, " rc ");
        ai_line_int(&l, rc);
        ai_line_add(&l, " path ");
        ai_line_add(&l, path_name(st.gguf_last_path));
        ai_line_add(&l, "\n");
        ai_puts(l.text);
        if (rc <= 0) break;
    }
    ai_line_reset(&l);
    ai_line_add(&l, "ggufclient tokens");
    for (i = 0U; i < st.gguf_token_count && i < OS_AI_ENGINE_TOKENS_MAX; i++) {
        ai_line_add(&l, i == st.gguf_prompt_tokens ? " |" : " ");
        ai_line_int(&l, (int)st.gguf_tokens[i]);
    }
    ai_line_add(&l, " end\n");
    ai_puts(l.text);
    ai_line_reset(&l);
    ai_line_add(&l, "ggufclient gguf worker ");
    ai_line_int(&l, st.gguf_worker_pid);
    ai_line_add(&l, " bytes ");
    ai_line_int(&l, (int)st.gguf_bytes_loaded);
    ai_line_add(&l, " fwd ");
    ai_line_int(&l, (int)st.gguf_forwarded);
    ai_line_add(&l, " done ");
    ai_line_int(&l, (int)st.gguf_completed);
    ai_line_add(&l, " kernel ");
    ai_line_int(&l, (int)st.gguf_kernel);
    ai_line_add(&l, " live ");
    ai_line_int(&l, (int)st.gguf_kernel_while_live);
    ai_line_add(&l, " fallback ");
    ai_line_int(&l, (int)st.gguf_fallbacks);
    ai_line_add(&l, " aborted ");
    ai_line_int(&l, (int)st.aborted);
    ai_line_add(&l, " rogue ");
    ai_line_int(&l, (int)st.rogue_refused);
    ai_line_add(&l, " pending ");
    ai_line_int(&l, (int)st.pending);
    ai_line_add(&l, " end\n");
    ai_puts(l.text);
    return 0;
}
