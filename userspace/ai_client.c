/* aiclient: one SYS_GPT2_GENERATE call (fixed prompt) and its evidence:
 * result, path (kernel / worker / kernel fallback), the token ids of the
 * generation and the relay counters. */
#include "ai_common.h"

static const char* path_name(unsigned int path) {
    if (path == OS_AI_PATH_KERNEL) return "kernel";
    if (path == OS_AI_PATH_WORKER) return "worker";
    if (path == OS_AI_PATH_KERNEL_FALLBACK) return "fallback";
    return "none";
}

int main(void) {
    static char out[384];
    static os_ai_engine_status_t st;
    ai_line_t l;
    unsigned int i;
    int rc;
    ai_puts("aiclient start\n");
    rc = ai_gpt2_generate("abc de", out, (int)sizeof(out));
    (void)ai_engine(OS_AI_ENGINE_STATUS, (unsigned int)&st, 0U);
    ai_line_reset(&l);
    ai_line_add(&l, "aiclient rc ");
    ai_line_int(&l, rc);
    ai_line_add(&l, " path ");
    ai_line_add(&l, path_name(st.last_path));
    ai_line_add(&l, " text [");
    ai_line_add(&l, rc >= 0 ? out : "");
    ai_line_add(&l, "]\n");
    ai_puts(l.text);
    ai_line_reset(&l);
    ai_line_add(&l, "aiclient tokens");
    for (i = 0U; i < st.last_token_count && i < OS_AI_ENGINE_TOKENS_MAX; i++) {
        ai_line_add(&l, i == st.last_prompt_tokens ? " |" : " ");
        ai_line_int(&l, (int)st.last_tokens[i]);
    }
    ai_line_add(&l, " end\n");
    ai_puts(l.text);
    ai_status_line(&l, "aiclient", &st);
    ai_puts(l.text);
    return 0;
}
