#ifndef MOHHDY_GPT2_SAMPLE_H
#define MOHHDY_GPT2_SAMPLE_H

#include <stdint.h>

#define GPT2_SAMPLE_TOP_K 8U

typedef struct {
    uint32_t ids[GPT2_SAMPLE_TOP_K];
    float values[GPT2_SAMPLE_TOP_K];
    const uint32_t* generated;
    uint32_t generated_count;
    uint32_t count;
} gpt2_sample_top_k_state_t;

/*
 * Near-greedy top-k (temperature 0.6). Frequency penalty and the previous-token
 * ban apply only to tokens already emitted, never to the prompt. That stops
 * GPT-2's " The The The" loop without derailing the continuation.
 */
void gpt2_sample_top_k_init(gpt2_sample_top_k_state_t* state,
                            const uint32_t* generated, uint32_t generated_count);
void gpt2_sample_top_k_offer(gpt2_sample_top_k_state_t* state,
                             uint32_t token, float logit);
uint32_t gpt2_sample_top_k_finish(const gpt2_sample_top_k_state_t* state,
                                  uint32_t* rng_state);
uint32_t gpt2_sample_top_k(const float* logits, uint32_t vocab,
                           const uint32_t* generated, uint32_t generated_count,
                           uint32_t* rng_state);

#endif
