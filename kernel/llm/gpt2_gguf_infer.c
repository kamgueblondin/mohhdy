#include "gpt2_gguf_infer.h"
#include "gpt2_gguf_loader.h"
#include "gpt2_sample.h"

#define GPT2_GGUF_INFER_HIDDEN (4U * GPT2_GGUF_INFER_MAX_CHANNELS)
#define GPT2_GGUF_INFER_DENSE_BYTES (GPT2_GGUF_INFER_HIDDEN * (uint32_t)sizeof(float))
/* ffn_down lit une ligne de largeur 4C : dimensionner pour le pire cas Q6_K. */
#define GPT2_GGUF_INFER_ROW_BYTES \
    ((GPT2_GGUF_INFER_HIDDEN / GPT2_QK_K) * GPT2_Q6_K_BLOCK_BYTES)
#define GPT2_GGUF_INFER_FILENAME_MAX 13U

static uint8_t gguf_header[GPT2_GGUF_INFER_HEADER_BYTES];
static gpt2_gguf_loaded_model_t gguf_model;
static gpt2_gguf_layer_t gguf_layers[GPT2_GGUF_INFER_MAX_LAYERS];
static gpt2_gguf_generation_t gguf_generation;
static gpt2_gguf_kv_cache_t gguf_cache;
static gpt2_gguf_generation_workspace_t gguf_workspace;
static const fat16_volume_t* gguf_volume;
static const fat32_volume_t* gguf_volume_fat32;
static char gguf_filename[GPT2_GGUF_INFER_FILENAME_MAX];
static char gguf_layer_name[64];
static uint32_t gguf_cache_tokens[GPT2_GGUF_INFER_MAX_CONTEXT];

static float gguf_kv_storage[GPT2_GGUF_INFER_MAX_LAYERS * GPT2_GGUF_INFER_MAX_CONTEXT *
                             2U * GPT2_GGUF_INFER_MAX_CHANNELS];
static uint8_t gguf_dense_scratch[GPT2_GGUF_INFER_DENSE_BYTES];
static uint8_t gguf_row_buffer[GPT2_GGUF_INFER_ROW_BYTES];
static float gguf_position_embedding[GPT2_GGUF_INFER_MAX_CHANNELS];
static float gguf_attention_gamma[GPT2_GGUF_INFER_MAX_CHANNELS];
static float gguf_attention_beta[GPT2_GGUF_INFER_MAX_CHANNELS];
static float gguf_qkv_bias[3U * GPT2_GGUF_INFER_MAX_CHANNELS];
static float gguf_attention_output_bias[GPT2_GGUF_INFER_MAX_CHANNELS];
static float gguf_ffn_gamma[GPT2_GGUF_INFER_MAX_CHANNELS];
static float gguf_ffn_beta[GPT2_GGUF_INFER_MAX_CHANNELS];
static float gguf_ffn_up_bias[GPT2_GGUF_INFER_HIDDEN];
static float gguf_ffn_down_bias[GPT2_GGUF_INFER_MAX_CHANNELS];
static float gguf_final_gamma[GPT2_GGUF_INFER_MAX_CHANNELS];
static float gguf_final_beta[GPT2_GGUF_INFER_MAX_CHANNELS];
static float gguf_final_hidden[GPT2_GGUF_INFER_MAX_CHANNELS];
static float gguf_norm[GPT2_GGUF_INFER_MAX_CHANNELS];
static float gguf_query[GPT2_GGUF_INFER_MAX_CHANNELS];
static float gguf_key[GPT2_GGUF_INFER_MAX_CHANNELS];
static float gguf_value[GPT2_GGUF_INFER_MAX_CHANNELS];
static float gguf_head_outputs[GPT2_GGUF_INFER_MAX_CHANNELS];
static float gguf_key_scratch[GPT2_GGUF_INFER_MAX_CHANNELS / GPT2_GGUF_INFER_HEADS];
static float gguf_scores[GPT2_GGUF_INFER_MAX_CONTEXT];
static float gguf_attention[GPT2_GGUF_INFER_MAX_CHANNELS];
static float gguf_projected[GPT2_GGUF_INFER_MAX_CHANNELS];
static float gguf_hidden[GPT2_GGUF_INFER_HIDDEN];
static float gguf_mlp_output[GPT2_GGUF_INFER_MAX_CHANNELS];
static gpt2_sample_top_k_state_t gguf_last_top_k;
static uint8_t gguf_last_top_k_ready;
static uint8_t gguf_ready;
static const char* gguf_status = "GGUF: profil local non initialise";

static int gpt2_gguf_copy_filename(const char* filename) {
    uint32_t i = 0U;
    if (!filename) return -1;
    while (filename[i] != '\0') {
        if (i + 1U >= GPT2_GGUF_INFER_FILENAME_MAX) return -1;
        gguf_filename[i] = filename[i];
        i++;
    }
    if (i == 0U) return -1;
    gguf_filename[i] = '\0';
    return 0;
}

static void gpt2_gguf_workspace_bind(void) {
    gguf_workspace.dense_scratch = gguf_dense_scratch;
    gguf_workspace.dense_scratch_capacity = sizeof(gguf_dense_scratch);
    gguf_workspace.position_embedding = gguf_position_embedding;
    gguf_workspace.position_embedding_capacity = GPT2_GGUF_INFER_MAX_CHANNELS;
    gguf_workspace.attention_gamma = gguf_attention_gamma;
    gguf_workspace.attention_gamma_capacity = GPT2_GGUF_INFER_MAX_CHANNELS;
    gguf_workspace.attention_beta = gguf_attention_beta;
    gguf_workspace.attention_beta_capacity = GPT2_GGUF_INFER_MAX_CHANNELS;
    gguf_workspace.qkv_bias = gguf_qkv_bias;
    gguf_workspace.qkv_bias_capacity = 3U * GPT2_GGUF_INFER_MAX_CHANNELS;
    gguf_workspace.attention_output_bias = gguf_attention_output_bias;
    gguf_workspace.attention_output_bias_capacity = GPT2_GGUF_INFER_MAX_CHANNELS;
    gguf_workspace.ffn_gamma = gguf_ffn_gamma;
    gguf_workspace.ffn_gamma_capacity = GPT2_GGUF_INFER_MAX_CHANNELS;
    gguf_workspace.ffn_beta = gguf_ffn_beta;
    gguf_workspace.ffn_beta_capacity = GPT2_GGUF_INFER_MAX_CHANNELS;
    gguf_workspace.ffn_up_bias = gguf_ffn_up_bias;
    gguf_workspace.ffn_up_bias_capacity = GPT2_GGUF_INFER_HIDDEN;
    gguf_workspace.ffn_down_bias = gguf_ffn_down_bias;
    gguf_workspace.ffn_down_bias_capacity = GPT2_GGUF_INFER_MAX_CHANNELS;
    gguf_workspace.final_gamma = gguf_final_gamma;
    gguf_workspace.final_gamma_capacity = GPT2_GGUF_INFER_MAX_CHANNELS;
    gguf_workspace.final_beta = gguf_final_beta;
    gguf_workspace.final_beta_capacity = GPT2_GGUF_INFER_MAX_CHANNELS;
    gguf_workspace.final_hidden = gguf_final_hidden;
    gguf_workspace.final_hidden_capacity = GPT2_GGUF_INFER_MAX_CHANNELS;
    gguf_workspace.block.norm = gguf_norm;
    gguf_workspace.block.norm_capacity = GPT2_GGUF_INFER_MAX_CHANNELS;
    gguf_workspace.block.query = gguf_query;
    gguf_workspace.block.query_capacity = GPT2_GGUF_INFER_MAX_CHANNELS;
    gguf_workspace.block.key = gguf_key;
    gguf_workspace.block.key_capacity = GPT2_GGUF_INFER_MAX_CHANNELS;
    gguf_workspace.block.value = gguf_value;
    gguf_workspace.block.value_capacity = GPT2_GGUF_INFER_MAX_CHANNELS;
    gguf_workspace.block.head_outputs = gguf_head_outputs;
    gguf_workspace.block.head_output_capacity = GPT2_GGUF_INFER_MAX_CHANNELS;
    gguf_workspace.block.key_scratch = gguf_key_scratch;
    gguf_workspace.block.key_scratch_capacity = GPT2_GGUF_INFER_MAX_CHANNELS / GPT2_GGUF_INFER_HEADS;
    gguf_workspace.block.scores = gguf_scores;
    gguf_workspace.block.score_capacity = GPT2_GGUF_INFER_MAX_CONTEXT;
    gguf_workspace.block.attention = gguf_attention;
    gguf_workspace.block.attention_capacity = GPT2_GGUF_INFER_MAX_CHANNELS;
    gguf_workspace.block.projected = gguf_projected;
    gguf_workspace.block.projected_capacity = GPT2_GGUF_INFER_MAX_CHANNELS;
    gguf_workspace.block.hidden = gguf_hidden;
    gguf_workspace.block.hidden_capacity = GPT2_GGUF_INFER_HIDDEN;
    gguf_workspace.block.mlp_output = gguf_mlp_output;
    gguf_workspace.block.mlp_output_capacity = GPT2_GGUF_INFER_MAX_CHANNELS;
    gguf_workspace.block.row_buffer = gguf_row_buffer;
    gguf_workspace.block.row_capacity = sizeof(gguf_row_buffer);
}

int gpt2_gguf_infer_init_fat16(const fat16_volume_t* volume, const char* filename) {
    int status;
    gguf_ready = 0U;
    gguf_last_top_k_ready = 0U;
    gguf_status = "GGUF: profil local non initialise";
    if (!volume || gpt2_gguf_copy_filename(filename) != 0) {
        gguf_status = "GGUF: nom FAT16 invalide";
        return -1;
    }
    status = gpt2_gguf_load_fat16_header(volume, gguf_filename, gguf_header,
                                         sizeof(gguf_header), &gguf_model);
    if (status != 0) {
        gguf_status = "GGUF: catalogue FAT16 indisponible";
        return status;
    }
    status = gpt2_gguf_generation_prepare(&gguf_model, gguf_layer_name,
                                           sizeof(gguf_layer_name), gguf_layers,
                                           GPT2_GGUF_INFER_MAX_LAYERS,
                                           &gguf_generation);
    if (status != 0) {
        gguf_status = "GGUF: roles GPT-2 incompatibles";
        return status;
    }
    if (gguf_generation.runtime.layer_count == 0U ||
        gguf_generation.runtime.layer_count > GPT2_GGUF_INFER_MAX_LAYERS ||
        gguf_generation.runtime.channels == 0U ||
        gguf_generation.runtime.channels > GPT2_GGUF_INFER_MAX_CHANNELS ||
        gguf_generation.runtime.channels % GPT2_GGUF_INFER_HEADS != 0U ||
        gguf_generation.vocabulary == 0U ||
        gguf_generation.vocabulary > GPT2_GGUF_INFER_MAX_VOCAB ||
        gguf_generation.max_positions == 0U) {
        gguf_status = "GGUF: profil hors bornes statiques";
        return -6;
    }
    status = gpt2_gguf_kv_cache_init(gguf_kv_storage, sizeof(gguf_kv_storage) / sizeof(float),
                                      gguf_generation.runtime.layer_count,
                                      GPT2_GGUF_INFER_MAX_CONTEXT,
                                      gguf_generation.runtime.channels, &gguf_cache);
    if (status != 0) {
        gguf_status = "GGUF: cache KV statique insuffisant";
        return status;
    }
    gpt2_gguf_workspace_bind();
    gguf_volume = volume;
    gguf_volume_fat32 = 0;
    gguf_ready = 1U;
    gguf_status = "GGUF: profil local FAT16 pret (cache KV actif)";
    return 0;
}

int gpt2_gguf_infer_init_fat32(const fat32_volume_t* volume, const char* filename) {
    int status;
    gguf_ready = 0U;
    gguf_last_top_k_ready = 0U;
    gguf_status = "GGUF: profil local non initialise";
    if (!volume || gpt2_gguf_copy_filename(filename) != 0) {
        gguf_status = "GGUF: nom FAT32 invalide";
        return -1;
    }
    status = gpt2_gguf_load_fat32_header(volume, gguf_filename, gguf_header,
                                         sizeof(gguf_header), &gguf_model);
    if (status != 0) {
        gguf_status = "GGUF: catalogue FAT32 indisponible";
        return status;
    }
    status = gpt2_gguf_generation_prepare(&gguf_model, gguf_layer_name,
                                           sizeof(gguf_layer_name), gguf_layers,
                                           GPT2_GGUF_INFER_MAX_LAYERS,
                                           &gguf_generation);
    if (status != 0) {
        gguf_status = "GGUF: roles GPT-2 incompatibles";
        return status;
    }
    if (gguf_generation.runtime.layer_count == 0U ||
        gguf_generation.runtime.layer_count > GPT2_GGUF_INFER_MAX_LAYERS ||
        gguf_generation.runtime.channels == 0U ||
        gguf_generation.runtime.channels > GPT2_GGUF_INFER_MAX_CHANNELS ||
        gguf_generation.runtime.channels % GPT2_GGUF_INFER_HEADS != 0U ||
        gguf_generation.vocabulary == 0U ||
        gguf_generation.vocabulary > GPT2_GGUF_INFER_MAX_VOCAB ||
        gguf_generation.max_positions == 0U) {
        gguf_status = "GGUF: profil hors bornes statiques";
        return -6;
    }
    status = gpt2_gguf_kv_cache_init(gguf_kv_storage, sizeof(gguf_kv_storage) / sizeof(float),
                                      gguf_generation.runtime.layer_count,
                                      GPT2_GGUF_INFER_MAX_CONTEXT,
                                      gguf_generation.runtime.channels, &gguf_cache);
    if (status != 0) {
        gguf_status = "GGUF: cache KV statique insuffisant";
        return status;
    }
    gpt2_gguf_workspace_bind();
    gguf_volume = 0;
    gguf_volume_fat32 = volume;
    gguf_ready = 1U;
    gguf_status = "GGUF: profil local FAT32 pret (cache KV actif)";
    return 0;
}

static int gpt2_gguf_cache_matches(const uint32_t* tokens, uint32_t count) {
    uint32_t i;
    if (gguf_cache.count > count) return 0;
    for (i = 0U; i < gguf_cache.count; i++) {
        if (gguf_cache_tokens[i] != tokens[i]) return 0;
    }
    return 1;
}

int gpt2_gguf_generate_next_sampled(const uint32_t* tokens, uint32_t token_count,
                                    uint32_t generated_count, uint32_t* next_token,
                                    uint32_t* rng_state) {
    uint32_t generated = generated_count;
    uint32_t i;
    gpt2_sample_top_k_state_t top_k;
    int status;
    if (!tokens || !next_token || !gguf_ready || (!gguf_volume && !gguf_volume_fat32)) {
        gguf_status = "GGUF: profil local indisponible";
        return -1;
    }
    if (token_count == 0U || token_count > GPT2_GGUF_INFER_MAX_CONTEXT ||
        token_count > gguf_generation.max_positions) {
        gguf_status = "GGUF: taille de contexte non supportee";
        return -2;
    }
    for (i = 0U; i < token_count; i++) {
        if (tokens[i] >= gguf_generation.vocabulary) {
            gguf_status = "GGUF: identifiant de jeton invalide";
            return -3;
        }
    }
    if (!gpt2_gguf_cache_matches(tokens, token_count) ||
        (gguf_cache.count == token_count && generated != 0U)) {
        gpt2_gguf_kv_cache_reset(&gguf_cache);
        gguf_last_top_k_ready = 0U;
    }
    while (gguf_cache.count < token_count) {
        uint32_t position = gguf_cache.count;
        gpt2_sample_top_k_init(&top_k,
                                generated ? tokens + token_count - generated : 0,
                                generated);
        status = gpt2_gguf_generation_token_top_k_fat16(&gguf_generation, &gguf_cache,
                                                         tokens[position], position,
                                                         GPT2_GGUF_INFER_HEADS, 0.00001f,
                                                         gguf_volume, gguf_filename,
                                                         &gguf_workspace, &top_k);
        if (status != 0) {
            gguf_status = "GGUF: echec de lecture ou forward FAT16";
            return -20 + status;
        }
        gguf_cache_tokens[position] = tokens[position];
        gguf_last_top_k = top_k;
        gguf_last_top_k_ready = 1U;
    }
    if (!gguf_last_top_k_ready) {
        gguf_status = "GGUF: logits top-k indisponibles";
        return -4;
    }
    if (generated > token_count) generated = token_count;
    *next_token = gpt2_sample_top_k_finish(&gguf_last_top_k, rng_state);
    gguf_status = "GGUF: jeton top-k en flux (cache KV FAT16 actif)";
    return 0;
}

int gpt2_gguf_infer_ready(void) {
    return gguf_ready ? 1 : 0;
}

const char* gpt2_gguf_infer_status(void) {
    return gguf_status;
}
