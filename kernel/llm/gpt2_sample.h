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
 * Near-greedy top-k (temperature 0.2). The previous emitted token is banned
 * so a continuation cannot repeat itself immediately. A mild frequency
 * penalty applies only to tokens already emitted, never to the prompt: one
 * use does not knock a clearly leading word out of the top of the list.
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

/* Liveness and cancellation hook. Called once per transformer layer (FP32
 * and GGUF) and once per generated token. The aiworker points it at an
 * OS_AI_ENGINE_HEARTBEAT call (progress for the kernel watchdog), the
 * kernel at its Ring 0 cancel flag. A non-zero return asks the token loop
 * to stop (gpt2_generate_fp32 returns GPT2_GENERATE_CANCELLED); the layer
 * calls ignore it. NULL by default (host tests). */
extern int (*gpt2_progress_hook)(void);
int gpt2_progress_note(void);

#endif
