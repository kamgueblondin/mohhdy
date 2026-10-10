#include "gpt2_generate.h"
#include "gpt2_sample.h"
#include "gpt2_infer.h"
#include "gpt2_model.h"
#include "gpt2_tokenizer.h"

uint32_t gpt2_generate_normalize(const char* prompt, uint32_t input_limit,
                                 char* out, uint32_t capacity) {
    uint32_t input_pos = 0U;
    uint32_t output_pos = 0U;
    if (!out || capacity == 0U) return 0U;
    if (!prompt) {
        out[0] = '\0';
        return 0U;
    }
    /* Keeps punctuation, apostrophes, single spaces and UTF-8. */
    while (input_pos < input_limit && prompt[input_pos] != '\0' && output_pos + 1U < capacity) {
        uint8_t ch = (uint8_t)prompt[input_pos++];
        if (ch == '\t' || ch == '\r' || ch == '\n') ch = ' ';
        if (ch < 32U || ch == 127U) continue;
        if (ch == ' ' && (output_pos == 0U || (uint8_t)out[output_pos - 1U] == ' ')) continue;
        out[output_pos++] = (char)ch;
    }
    out[output_pos] = '\0';
    return output_pos;
}

uint32_t gpt2_generate_seed(const char* normalized) {
    uint32_t rng_state = 0x9e3779b9U;
    uint32_t i;
    for (i = 0U; normalized && normalized[i] != '\0'; i++) {
        rng_state = rng_state * 16777619U + (uint8_t)normalized[i];
    }
    if (rng_state == 0U) rng_state = 1U;
    return rng_state;
}

int gpt2_generate_fp32(const char* normalized, char* out, uint32_t max,
                       gpt2_generate_trace_t* trace) {
    uint32_t tokens[GPT2_GENERATE_MAX_TOKENS];
    uint32_t token_count = 0U;
    uint32_t written = 0U;
    uint32_t rng_state;
    uint32_t prompt_tokens;
    uint32_t prev_generated = 0xFFFFFFFFu;
    const gpt2_model_t* model;
    int rc;

    if (trace) {
        trace->prompt_tokens = 0U;
        trace->token_count = 0U;
    }
    if (!normalized || !out || max < 2U) return -1;
    rc = gpt2_tokenizer_encode(normalized, tokens, GPT2_GENERATE_MAX_TOKENS, &token_count);
    if (rc != 0) return -2;
    model = gpt2_model_current();
    if (!model->ready || token_count > model->config.max_seq_len) return -5;
    prompt_tokens = token_count;
    rng_state = gpt2_generate_seed(normalized);

    for (uint32_t step = 0U; step < GPT2_GENERATE_STEPS && token_count < GPT2_GENERATE_MAX_TOKENS &&
         token_count < model->config.max_seq_len; step++) {
        uint32_t next_token = 0U;
        const char* piece;
        int saw_stop = 0;
        uint32_t generated_count = token_count - prompt_tokens;
        if (gpt2_progress_note() != 0) return GPT2_GENERATE_CANCELLED;
        rc = gpt2_generate_next_sampled(tokens, token_count, generated_count,
                                        &next_token, &rng_state);
        if (rc != 0) return -30 + rc;
        if (next_token == gpt2_tokenizer_eot()) break;
        if (next_token == prev_generated) break;
        prev_generated = next_token;
        tokens[token_count++] = next_token;
        piece = gpt2_tokenizer_decode(next_token);
        if (!piece) return -4;
        for (uint32_t i = 0U; piece[i] != '\0'; i++) {
            char ch;
            if (written + 1U >= max) {
                out[written] = '\0';
                goto done;
            }
            ch = piece[i];
            out[written++] = ch;
            if (ch == '\n' || ch == '.' || ch == '!' || ch == '?') saw_stop = 1;
        }
        if (saw_stop && written >= GPT2_GENERATE_MIN_CHARS) break;
    }
    out[written] = '\0';
done:
    if (trace) {
        for (uint32_t i = 0U; i < token_count; i++) trace->tokens[i] = tokens[i];
        trace->prompt_tokens = prompt_tokens;
        trace->token_count = token_count;
    }
    return (int)written;
}
