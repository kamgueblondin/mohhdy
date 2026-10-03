#ifndef MOHHDY_GPT2_GGUF_SESSION_H
#define MOHHDY_GPT2_GGUF_SESSION_H

/* GGUF 109/110 session (prompt tokens, generated tokens, sampler state)
 * and its tokenizer use. Same code in the Ring 0 fallback (kernel) and in
 * the Ring 3 aiworker, which owns the session while it is GGUF-ready: the
 * worker encodes the prompt, samples and decodes each piece itself. The
 * forward pass + top-k step is a callback (local GGUF runtime). */

#include <stdint.h>

#define GPT2_GGUF_SESSION_TOKENS 64U

typedef struct {
    uint32_t tokens[GPT2_GGUF_SESSION_TOKENS];
    uint32_t token_count;
    uint32_t prompt_tokens;
    uint32_t rng;
    uint32_t active;
    uint32_t id;     /* set by the owner (kernel session id) */
} gpt2_gguf_session_t;

/* One sampling step: 0 and *next_token / *rng_state updated, else a small
 * positive error (the step returns -30 + rc, like the historical code). */
typedef int (*gpt2_gguf_next_fn)(void* context, const uint32_t* tokens, uint32_t token_count,
                                 uint32_t generated_count, uint32_t* next_token,
                                 uint32_t* rng_state);

void gpt2_gguf_session_reset(gpt2_gguf_session_t* session);
/* Encodes the already normalised prompt and seeds the sampler.
 * 0 ok, -2 encode failure, -5 runtime not ready or prompt too long. */
int gpt2_gguf_session_start(gpt2_gguf_session_t* session, const char* normalized,
                            int runtime_ready, uint32_t max_context);
/* One step: piece length written to out (NUL terminated), 0 at the end of
 * the session (EOT or 64 tokens), -1 bad args, -6 no active session,
 * -30 + rc on a step error (session kept), -4 undecodable token. */
int gpt2_gguf_session_step(gpt2_gguf_session_t* session, gpt2_gguf_next_fn next, void* context,
                           char* out, uint32_t max);

#endif
