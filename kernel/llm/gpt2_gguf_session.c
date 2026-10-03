#include "gpt2_gguf_session.h"
#include "gpt2_generate.h"
#include "gpt2_tokenizer.h"

void gpt2_gguf_session_reset(gpt2_gguf_session_t* session) {
    uint32_t i;
    if (!session) return;
    for (i = 0U; i < GPT2_GGUF_SESSION_TOKENS; i++) session->tokens[i] = 0U;
    session->token_count = 0U;
    session->prompt_tokens = 0U;
    session->rng = 0U;
    session->active = 0U;
}

int gpt2_gguf_session_start(gpt2_gguf_session_t* session, const char* normalized,
                            int runtime_ready, uint32_t max_context) {
    uint32_t tokens[GPT2_GGUF_SESSION_TOKENS];
    uint32_t token_count = 0U, i;
    if (!session || !normalized) return -1;
    session->active = 0U;
    if (gpt2_tokenizer_encode(normalized, tokens, GPT2_GGUF_SESSION_TOKENS, &token_count) != 0)
        return -2;
    if (!runtime_ready || token_count > max_context) return -5;
    for (i = 0U; i < GPT2_GGUF_SESSION_TOKENS; i++)
        session->tokens[i] = i < token_count ? tokens[i] : 0U;
    session->token_count = token_count;
    session->prompt_tokens = token_count;
    session->rng = gpt2_generate_seed(normalized);
    session->active = 1U;
    return 0;
}

int gpt2_gguf_session_step(gpt2_gguf_session_t* session, gpt2_gguf_next_fn next, void* context,
                           char* out, uint32_t max) {
    const char* piece;
    uint32_t next_token = 0U, written = 0U, generated_count, i;
    int rc;
    if (!session || !next || !out || max < 2U) return -1;
    if (!session->active) return -6;
    if (session->token_count == 0U || session->token_count >= GPT2_GGUF_SESSION_TOKENS) {
        session->active = 0U;
        out[0] = '\0';
        return 0;
    }
    generated_count = session->token_count - session->prompt_tokens;
    rc = next(context, session->tokens, session->token_count, generated_count, &next_token,
              &session->rng);
    if (rc != 0) return -30 + rc;
    if (next_token == gpt2_tokenizer_eot()) {
        session->active = 0U;
        out[0] = '\0';
        return 0;
    }
    session->tokens[session->token_count++] = next_token;
    piece = gpt2_tokenizer_decode(next_token);
    if (!piece) return -4;
    for (i = 0U; piece[i] != '\0'; i++) {
        if (written + 1U >= max) break;
        out[written++] = piece[i];
    }
    out[written] = '\0';
    return (int)written;
}
