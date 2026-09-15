#include "gpt2_sample.h"

#define GPT2_REPEAT_PENALTY 3.0f
#define GPT2_SAMPLE_TEMPERATURE 0.60f

static float gpt2_fast_exp(float value) {
    union { float f; uint32_t u; } convert;
    if (value < -80.0f) return 0.0f;
    if (value > 80.0f) value = 80.0f;
    convert.u = (uint32_t)(12102203.0f * value + 1064866805.0f);
    return convert.f;
}

void gpt2_sample_top_k_init(gpt2_sample_top_k_state_t* state,
                            const uint32_t* generated, uint32_t generated_count) {
    if (!state) return;
    state->generated = generated;
    state->generated_count = generated_count;
    state->count = 0U;
}

void gpt2_sample_top_k_offer(gpt2_sample_top_k_state_t* state,
                             uint32_t token, float logit) {
    uint32_t banned;
    uint32_t repeats = 0U;
    uint32_t pos;
    float value = logit;
    if (!state) return;
    banned = (state->generated && state->generated_count > 0U)
        ? state->generated[state->generated_count - 1U] : 0xFFFFFFFFu;
    if (token == banned) return;
    if (state->generated) {
        for (uint32_t i = 0U; i < state->generated_count; i++) {
            if (state->generated[i] == token) repeats++;
        }
    }
    if (repeats > 0U) value -= GPT2_REPEAT_PENALTY * (float)repeats;
    pos = state->count < GPT2_SAMPLE_TOP_K ? state->count++ : GPT2_SAMPLE_TOP_K;
    while (pos > 0U && value > state->values[pos - 1U]) {
        if (pos < GPT2_SAMPLE_TOP_K) {
            state->values[pos] = state->values[pos - 1U];
            state->ids[pos] = state->ids[pos - 1U];
        }
        pos--;
    }
    if (pos < GPT2_SAMPLE_TOP_K) {
        state->values[pos] = value;
        state->ids[pos] = token;
    }
}

uint32_t gpt2_sample_top_k_finish(const gpt2_sample_top_k_state_t* state,
                                  uint32_t* rng_state) {
    float weights[GPT2_SAMPLE_TOP_K];
    uint32_t random_state = rng_state && *rng_state ? *rng_state : 0x9e3779b9U;
    uint32_t banned;
    float total = 0.0f;
    float target;
    if (!state) return 0U;
    banned = (state->generated && state->generated_count > 0U)
        ? state->generated[state->generated_count - 1U] : 0xFFFFFFFFu;
    if (state->count == 0U) return banned == 0xFFFFFFFFu ? 0U : banned;
    for (uint32_t i = 0U; i < state->count; i++) {
        weights[i] = gpt2_fast_exp((state->values[i] - state->values[0]) / GPT2_SAMPLE_TEMPERATURE);
        total += weights[i];
    }
    random_state = random_state * 1664525U + 1013904223U;
    if (rng_state) *rng_state = random_state;
    target = ((float)(random_state & 0x00ffffffU) / 16777216.0f) * total;
    for (uint32_t i = 0U; i < state->count; i++) {
        if (target <= weights[i]) return state->ids[i];
        target -= weights[i];
    }
    return state->ids[state->count - 1U];
}

uint32_t gpt2_sample_top_k(const float* logits, uint32_t vocab,
                           const uint32_t* generated, uint32_t generated_count,
                           uint32_t* rng_state) {
    gpt2_sample_top_k_state_t state;
    if (!logits || vocab == 0U) return 0U;
    gpt2_sample_top_k_init(&state, generated, generated_count);
    for (uint32_t i = 0U; i < vocab; i++) gpt2_sample_top_k_offer(&state, i, logits[i]);
    return gpt2_sample_top_k_finish(&state, rng_state);
}
