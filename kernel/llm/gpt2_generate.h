#ifndef MOHHDY_GPT2_GENERATE_H
#define MOHHDY_GPT2_GENERATE_H

/* FP32 GPT-2 text generation loop shared by the Ring 0 fallback
 * (SYS_GPT2_GENERATE in kernel/syscall/syscall.c) and the Ring 3 aiworker
 * (userspace/ai_worker.c). Both builds compile this file and the llm.c
 * forward pass (gpt2_infer.c) with the same -O3 -msse2 -mfpmath=sse flags,
 * so the same checkpoint and prompt give the same token ids. */

#include <stdint.h>

#define GPT2_GENERATE_PROMPT_MAX 128U
#define GPT2_GENERATE_MAX_TOKENS 64U
#define GPT2_GENERATE_STEPS 12U

typedef struct {
    uint32_t tokens[GPT2_GENERATE_MAX_TOKENS];
    uint32_t prompt_tokens;
    uint32_t token_count; /* prompt + generated */
} gpt2_generate_trace_t;

/* Prompt normalisation of SYS_GPT2_GENERATE: tabs/newlines become spaces,
 * other control bytes are dropped, runs of spaces collapse, at most
 * input_limit bytes are read. Returns the length written to out. */
uint32_t gpt2_generate_normalize(const char* prompt, uint32_t input_limit,
                                 char* out, uint32_t capacity);
/* Seed of the deterministic top-k sampler for a normalised prompt. */
uint32_t gpt2_generate_seed(const char* normalized);
/* Runs up to GPT2_GENERATE_STEPS steps on the current model/tokenizer.
 * Returns the bytes written to out (NUL terminated) or the historical
 * negative codes: -1 bad args, -2 tokenizer, -5 model not ready / prompt
 * too long, -4 decode, -30 + rc forward pass. trace may be NULL. */
int gpt2_generate_fp32(const char* normalized, char* out, uint32_t max,
                       gpt2_generate_trace_t* trace);

#endif
