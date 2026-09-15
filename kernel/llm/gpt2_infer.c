#include "gpt2_infer.h"
#include "gpt2_model.h"
#include "gpt2_sample.h"

#define GPT2_BAREMETAL_MAX_CONTEXT 64U
#define GPT2_BAREMETAL_MAX_CHANNELS 768U
#define GPT2_BAREMETAL_MAX_LAYERS 12U
#define GPT2_BAREMETAL_MAX_VOCAB 50304U

typedef struct {
    const float* wte;
    const float* wpe;
    const float* ln1w;
    const float* ln1b;
    const float* qkvw;
    const float* qkvb;
    const float* attprojw;
    const float* attprojb;
    const float* ln2w;
    const float* ln2b;
    const float* fcw;
    const float* fcb;
    const float* fcprojw;
    const float* fcprojb;
    const float* lnfw;
    const float* lnfb;
} gpt2_params_t;

typedef struct {
    float* residual;
    float* temporary;
    float* normalized;
    float* qkv;
    float* attention;
    float* scores;
    float* hidden;
    float* logits;
    float* kv_cache;
    uint32_t context;
    uint32_t channels;
    uint32_t vocab;
    uint32_t cache_count;
    uint32_t cache_tokens[GPT2_BAREMETAL_MAX_CONTEXT];
    const gpt2_model_t* cache_model;
    int ready;
} gpt2_workspace_t;

static float workspace_residual[GPT2_BAREMETAL_MAX_CHANNELS];
static float workspace_temporary[GPT2_BAREMETAL_MAX_CHANNELS];
static float workspace_normalized[GPT2_BAREMETAL_MAX_CHANNELS];
static float workspace_qkv[3U * GPT2_BAREMETAL_MAX_CHANNELS];
static float workspace_attention[GPT2_BAREMETAL_MAX_CHANNELS];
static float workspace_scores[GPT2_BAREMETAL_MAX_CONTEXT];
static float workspace_hidden[4U * GPT2_BAREMETAL_MAX_CHANNELS];
static float workspace_logits[GPT2_BAREMETAL_MAX_VOCAB];
static float workspace_kv_cache[GPT2_BAREMETAL_MAX_LAYERS * GPT2_BAREMETAL_MAX_CONTEXT * 2U * GPT2_BAREMETAL_MAX_CHANNELS];
static gpt2_workspace_t workspace;
static const char* infer_status = "GPT-2: moteur CPU non initialise";

static float gpt2_inv_sqrt(float value) {
    union { float f; uint32_t u; } convert;
    float half = 0.5f * value;
    convert.f = value;
    convert.u = 0x5f3759dfU - (convert.u >> 1);
    convert.f = convert.f * (1.5f - half * convert.f * convert.f);
    return convert.f;
}

static float gpt2_fast_exp(float value) {
    union { float f; uint32_t u; } convert;
    if (value < -80.0f) return 0.0f;
    if (value > 80.0f) value = 80.0f;
    convert.u = (uint32_t)(12102203.0f * value + 1064866805.0f);
    return convert.f;
}

static float gpt2_gelu(float value) {
    float x3 = value * value * value;
    float inner = 0.7978845608f * (value + 0.044715f * x3);
    float inner2 = inner * inner;
    float tanh_approx = inner * (27.0f + inner2) / (27.0f + 9.0f * inner2);
    return 0.5f * value * (1.0f + tanh_approx);
}

static void gpt2_map_params(gpt2_params_t* params, const gpt2_model_t* model) {
    const gpt2_config_t* cfg = &model->config;
    uint64_t sizes[16];
    uint64_t c = cfg->channels;
    uint64_t l = cfg->num_layers;
    uint64_t t = cfg->max_seq_len;
    uint64_t v = cfg->padded_vocab_size;
    const float* cursor = model->weights;

    sizes[0] = v * c; sizes[1] = t * c; sizes[2] = l * c; sizes[3] = l * c;
    sizes[4] = l * (3U * c) * c; sizes[5] = l * (3U * c);
    sizes[6] = l * c * c; sizes[7] = l * c; sizes[8] = l * c; sizes[9] = l * c;
    sizes[10] = l * (4U * c) * c; sizes[11] = l * (4U * c);
    sizes[12] = l * c * (4U * c); sizes[13] = l * c; sizes[14] = c; sizes[15] = c;

    params->wte = cursor; cursor += sizes[0];
    params->wpe = cursor; cursor += sizes[1];
    params->ln1w = cursor; cursor += sizes[2];
    params->ln1b = cursor; cursor += sizes[3];
    params->qkvw = cursor; cursor += sizes[4];
    params->qkvb = cursor; cursor += sizes[5];
    params->attprojw = cursor; cursor += sizes[6];
    params->attprojb = cursor; cursor += sizes[7];
    params->ln2w = cursor; cursor += sizes[8];
    params->ln2b = cursor; cursor += sizes[9];
    params->fcw = cursor; cursor += sizes[10];
    params->fcb = cursor; cursor += sizes[11];
    params->fcprojw = cursor; cursor += sizes[12];
    params->fcprojb = cursor; cursor += sizes[13];
    params->lnfw = cursor; cursor += sizes[14];
    params->lnfb = cursor;
}

static void gpt2_layernorm(float* out, const float* input, const float* weight,
                           const float* bias, uint32_t channels) {
    float mean = 0.0f;
    float variance = 0.0f;
    for (uint32_t i = 0; i < channels; i++) mean += input[i];
    mean /= (float)channels;
    for (uint32_t i = 0; i < channels; i++) {
        float delta = input[i] - mean;
        variance += delta * delta;
    }
    variance /= (float)channels;
    float scale = gpt2_inv_sqrt(variance + 0.00001f);
    for (uint32_t i = 0; i < channels; i++) out[i] = (input[i] - mean) * scale * weight[i] + bias[i];
}

static void gpt2_matmul_one(float* out, const float* input, const float* weight,
                            const float* bias, uint32_t input_size, uint32_t output_size) {
    for (uint32_t output = 0; output < output_size; output++) {
        float value = bias ? bias[output] : 0.0f;
        const float* row = weight + output * input_size;
        for (uint32_t input_index = 0; input_index < input_size; input_index++) value += input[input_index] * row[input_index];
        out[output] = value;
    }
}

static void gpt2_add_in_place(float* dst, const float* right, uint32_t count) {
    for (uint32_t i = 0; i < count; i++) dst[i] += right[i];
}

/* Attention pour un seul nouveau jeton; K et V des positions anterieures sont dans le cache. */
static void gpt2_attention_cached(float* out, float* scores, const float* qkv,
                                  uint32_t layer, uint32_t position,
                                  uint32_t channels, uint32_t heads) {
    uint32_t head_size = channels / heads;
    float scale = gpt2_inv_sqrt((float)head_size);
    for (uint32_t head = 0; head < heads; head++) {
        const float* query = qkv + head * head_size;
        float max_score = -3.4e38f;
        for (uint32_t previous = 0; previous <= position; previous++) {
            const float* entry = workspace.kv_cache +
                ((layer * GPT2_BAREMETAL_MAX_CONTEXT + previous) * 2U * channels);
            const float* key = entry + head * head_size;
            float score = 0.0f;
            for (uint32_t i = 0; i < head_size; i++) score += query[i] * key[i];
            score *= scale;
            scores[previous] = score;
            if (score > max_score) max_score = score;
        }
        float sum = 0.0f;
        for (uint32_t previous = 0; previous <= position; previous++) {
            scores[previous] = gpt2_fast_exp(scores[previous] - max_score);
            sum += scores[previous];
        }
        float* destination = out + head * head_size;
        for (uint32_t i = 0; i < head_size; i++) destination[i] = 0.0f;
        for (uint32_t previous = 0; previous <= position; previous++) {
            const float* entry = workspace.kv_cache +
                ((layer * GPT2_BAREMETAL_MAX_CONTEXT + previous) * 2U * channels);
            const float* value = entry + channels + head * head_size;
            float probability = scores[previous] / sum;
            for (uint32_t i = 0; i < head_size; i++) destination[i] += probability * value[i];
        }
    }
}

static int gpt2_workspace_init(const gpt2_config_t* cfg) {
    uint32_t c;
    uint32_t v;
    if (!cfg) return -1;
    c = cfg->channels;
    v = cfg->padded_vocab_size;
    if (c == 0U || c > GPT2_BAREMETAL_MAX_CHANNELS ||
        cfg->num_layers == 0U || cfg->num_layers > GPT2_BAREMETAL_MAX_LAYERS ||
        v == 0U || v > GPT2_BAREMETAL_MAX_VOCAB) {
        infer_status = "GPT-2: configuration hors capacite statique";
        return -2;
    }
    if (workspace.ready && workspace.channels == c && workspace.vocab == v &&
        workspace.cache_model && workspace.cache_model->config.num_layers == cfg->num_layers)
        return 0;
    if (workspace.ready) {
        infer_status = "GPT-2: changement de configuration non supporte pendant la session";
        return -3;
    }
    workspace.residual = workspace_residual;
    workspace.temporary = workspace_temporary;
    workspace.normalized = workspace_normalized;
    workspace.qkv = workspace_qkv;
    workspace.attention = workspace_attention;
    workspace.scores = workspace_scores;
    workspace.hidden = workspace_hidden;
    workspace.logits = workspace_logits;
    workspace.kv_cache = workspace_kv_cache;
    workspace.context = GPT2_BAREMETAL_MAX_CONTEXT;
    workspace.channels = c;
    workspace.vocab = v;
    workspace.cache_count = 0;
    workspace.cache_model = 0;
    workspace.ready = 1;
    return 0;
}

static void gpt2_cache_reset(const gpt2_model_t* model) {
    workspace.cache_count = 0;
    workspace.cache_model = model;
}

static int gpt2_cache_matches(const gpt2_model_t* model, const uint32_t* tokens, uint32_t count) {
    if (workspace.cache_model != model || workspace.cache_count > count) return 0;
    for (uint32_t i = 0; i < workspace.cache_count; i++) {
        if (workspace.cache_tokens[i] != tokens[i]) return 0;
    }
    return 1;
}

/* Execute une seule position et ajoute ses K/V aux caches de toutes les couches. */
static void gpt2_forward_cached_token(const gpt2_params_t* params, const gpt2_config_t* cfg,
                                      uint32_t token_id, uint32_t position) {
    uint32_t c = cfg->channels;
    const float* embedding = params->wte + token_id * c;
    const float* positional = params->wpe + position * c;
    for (uint32_t i = 0; i < c; i++) workspace.residual[i] = embedding[i] + positional[i];

    for (uint32_t layer = 0; layer < cfg->num_layers; layer++) {
        const float* ln1w = params->ln1w + layer * c;
        const float* ln1b = params->ln1b + layer * c;
        const float* qkvw = params->qkvw + layer * (3U * c) * c;
        const float* qkvb = params->qkvb + layer * (3U * c);
        const float* attprojw = params->attprojw + layer * c * c;
        const float* attprojb = params->attprojb + layer * c;
        const float* ln2w = params->ln2w + layer * c;
        const float* ln2b = params->ln2b + layer * c;
        const float* fcw = params->fcw + layer * (4U * c) * c;
        const float* fcb = params->fcb + layer * (4U * c);
        const float* fcprojw = params->fcprojw + layer * c * (4U * c);
        const float* fcprojb = params->fcprojb + layer * c;
        float* entry = workspace.kv_cache +
            ((layer * GPT2_BAREMETAL_MAX_CONTEXT + position) * 2U * c);

        gpt2_layernorm(workspace.normalized, workspace.residual, ln1w, ln1b, c);
        gpt2_matmul_one(workspace.qkv, workspace.normalized, qkvw, qkvb, c, 3U * c);
        for (uint32_t i = 0; i < c; i++) {
            entry[i] = workspace.qkv[c + i];
            entry[c + i] = workspace.qkv[2U * c + i];
        }
        gpt2_attention_cached(workspace.attention, workspace.scores, workspace.qkv,
                              layer, position, c, cfg->num_heads);
        gpt2_matmul_one(workspace.temporary, workspace.attention, attprojw, attprojb, c, c);
        gpt2_add_in_place(workspace.residual, workspace.temporary, c);
        gpt2_layernorm(workspace.normalized, workspace.residual, ln2w, ln2b, c);
        gpt2_matmul_one(workspace.hidden, workspace.normalized, fcw, fcb, c, 4U * c);
        for (uint32_t i = 0; i < 4U * c; i++) workspace.hidden[i] = gpt2_gelu(workspace.hidden[i]);
        gpt2_matmul_one(workspace.temporary, workspace.hidden, fcprojw, fcprojb, 4U * c, c);
        gpt2_add_in_place(workspace.residual, workspace.temporary, c);
    }

    gpt2_layernorm(workspace.normalized, workspace.residual, params->lnfw, params->lnfb, c);
    for (uint32_t token = 0; token < cfg->vocab_size; token++) {
        const float* output_embedding = params->wte + token * c;
        float logit = 0.0f;
        for (uint32_t i = 0; i < c; i++) logit += workspace.normalized[i] * output_embedding[i];
        workspace.logits[token] = logit;
    }
}

static int gpt2_generate_next_impl(const uint32_t* tokens, uint32_t token_count,
                                   uint32_t* next_token, int sampled,
                                   uint32_t generated_count, uint32_t* rng_state) {
    const gpt2_model_t* model = gpt2_model_current();
    const gpt2_config_t* cfg;
    gpt2_params_t params;
    uint32_t best_token = 0;
    float best_logit = -3.4e38f;

    if (!tokens || !next_token || !model->ready) { infer_status = "GPT-2: checkpoint local indisponible"; return -1; }
    cfg = &model->config;
    if (token_count == 0 || token_count > GPT2_BAREMETAL_MAX_CONTEXT || token_count > cfg->max_seq_len) {
        infer_status = "GPT-2: taille de contexte non supportee";
        return -2;
    }
    for (uint32_t i = 0; i < token_count; i++) if (tokens[i] >= cfg->vocab_size) {
        infer_status = "GPT-2: identifiant de jeton invalide";
        return -3;
    }
    if (gpt2_workspace_init(cfg) != 0) return -4;
    gpt2_map_params(&params, model);
    if (!gpt2_cache_matches(model, tokens, token_count)) gpt2_cache_reset(model);
    while (workspace.cache_count < token_count) {
        uint32_t position = workspace.cache_count;
        gpt2_forward_cached_token(&params, cfg, tokens[position], position);
        workspace.cache_tokens[position] = tokens[position];
        workspace.cache_count++;
    }
    for (uint32_t token = 0; token < cfg->vocab_size; token++) {
        if (workspace.logits[token] > best_logit) { best_logit = workspace.logits[token]; best_token = token; }
    }
    if (sampled) {
        uint32_t gen_n = generated_count;
        const uint32_t* generated = 0;
        if (gen_n > token_count) gen_n = token_count;
        if (gen_n > 0U) generated = tokens + (token_count - gen_n);
        *next_token = gpt2_sample_top_k(workspace.logits, cfg->vocab_size,
                                        generated, gen_n, rng_state);
        infer_status = "GPT-2: jeton echantillonne localement (cache KV actif)";
    } else {
        *next_token = best_token;
        infer_status = "GPT-2: prochain jeton genere localement (cache KV actif)";
    }
    return 0;
}

int gpt2_generate_next(const uint32_t* tokens, uint32_t token_count, uint32_t* next_token) {
    return gpt2_generate_next_impl(tokens, token_count, next_token, 0, 0, 0);
}

int gpt2_generate_next_sampled(const uint32_t* tokens, uint32_t token_count,
                               uint32_t generated_count, uint32_t* next_token,
                               uint32_t* rng_state) {
    return gpt2_generate_next_impl(tokens, token_count, next_token, 1, generated_count, rng_state);
}

const char* gpt2_infer_status(void) { return infer_status; }
