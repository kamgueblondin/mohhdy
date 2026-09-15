#ifndef MOHHDY_GPT2_INFER_H
#define MOHHDY_GPT2_INFER_H

#include <stdint.h>

/*
 * Generate one token from a sequence of GPT-2 token identifiers. The current
 * CPU reference backend caps the context at 64 tokens to bound activation
 * memory during the first bare-metal implementation.
 */
int gpt2_generate_next(const uint32_t* tokens, uint32_t token_count, uint32_t* next_token);
/* Top-k basse temperature ; penalite et ban uniquement sur les jetons deja emis. */
int gpt2_generate_next_sampled(const uint32_t* tokens, uint32_t token_count,
                               uint32_t generated_count, uint32_t* next_token,
                               uint32_t* rng_state);
const char* gpt2_infer_status(void);

#endif
