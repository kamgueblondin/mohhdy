#ifndef MOHHDY_GPT2_GGUF_LOADER_H
#define MOHHDY_GPT2_GGUF_LOADER_H

#include "gpt2_gguf.h"
#include "gpt2_quant.h"
#include "gpt2_sample.h"
#include "../fs/fat16.h"
#include "../fs/fat32.h"

typedef struct {
    gpt2_gguf_index_t index;
    uint32_t bytes_loaded;
} gpt2_gguf_loaded_model_t;

typedef struct {
    const gpt2_gguf_loaded_model_t* model;
    gpt2_gguf_layer_t layer;
    uint8_t* scratch;
    uint32_t scratch_capacity;
    uint32_t channels;
    uint32_t position;
} gpt2_gguf_forward_context_t;

typedef struct {
    float* storage;
    uint32_t layers;
    uint32_t max_positions;
    uint32_t channels;
    uint32_t count;
} gpt2_gguf_kv_cache_t;

typedef struct {
    const gpt2_gguf_loaded_model_t* model;
    gpt2_gguf_layer_t* layers;
    uint32_t layer_count;
    uint32_t channels;
    uint8_t ready;
} gpt2_gguf_runtime_t;

/* Espace de travail d’un bloc GPT-2 GGUF, entièrement fourni par l’appelant.
 * Les capacités des vecteurs float sont exprimées en nombre d’éléments. */
typedef struct {
    float* norm; uint32_t norm_capacity;
    float* query; uint32_t query_capacity;
    float* key; uint32_t key_capacity;
    float* value; uint32_t value_capacity;
    float* head_outputs; uint32_t head_output_capacity;
    float* key_scratch; uint32_t key_scratch_capacity;
    float* scores; uint32_t score_capacity;
    float* attention; uint32_t attention_capacity;
    float* projected; uint32_t projected_capacity;
    float* hidden; uint32_t hidden_capacity;
    float* mlp_output; uint32_t mlp_output_capacity;
    uint8_t* row_buffer; uint32_t row_capacity;
} gpt2_gguf_block_workspace_t;

/* Contexte préparé pour une génération GPT-2 GGUF. Les descripteurs de couche
 * et le buffer de noms restent détenus par l’appelant. */
typedef struct {
    gpt2_gguf_runtime_t runtime;
    gpt2_gguf_tensor_t token_embedding;
    gpt2_gguf_tensor_t position_embedding;
    gpt2_gguf_tensor_t output_norm_weight;
    gpt2_gguf_tensor_t output_norm_bias;
    gpt2_gguf_tensor_t output_weight;
    uint32_t vocabulary;
    uint32_t max_positions;
    uint8_t ready;
} gpt2_gguf_generation_t;

/* Espace de travail caller-owned pour un token GPT-2 GGUF complet. Les
 * capacités de vecteurs sont en éléments float, sauf les scratchs en octets. */
typedef struct {
    gpt2_gguf_block_workspace_t block;
    uint8_t* dense_scratch; uint32_t dense_scratch_capacity;
    float* position_embedding; uint32_t position_embedding_capacity;
    float* attention_gamma; uint32_t attention_gamma_capacity;
    float* attention_beta; uint32_t attention_beta_capacity;
    float* qkv_bias; uint32_t qkv_bias_capacity;
    float* attention_output_bias; uint32_t attention_output_bias_capacity;
    float* ffn_gamma; uint32_t ffn_gamma_capacity;
    float* ffn_beta; uint32_t ffn_beta_capacity;
    float* ffn_up_bias; uint32_t ffn_up_bias_capacity;
    float* ffn_down_bias; uint32_t ffn_down_bias_capacity;
    float* final_gamma; uint32_t final_gamma_capacity;
    float* final_beta; uint32_t final_beta_capacity;
    float* final_hidden; uint32_t final_hidden_capacity;
} gpt2_gguf_generation_workspace_t;

/* Construit et valide une table de couches persistante caller-owned. */
int gpt2_gguf_runtime_prepare(const gpt2_gguf_loaded_model_t* model,
                              uint32_t layer_count, uint32_t channels,
                              char* name, uint32_t name_capacity,
                              gpt2_gguf_layer_t* layers,
                              uint32_t layer_capacity,
                              gpt2_gguf_runtime_t* out);
/* Retourne une vue bornée d’une couche déjà préparée. */
int gpt2_gguf_runtime_get_layer(const gpt2_gguf_runtime_t* runtime,
                                uint32_t layer_index, gpt2_gguf_layer_t* out);
/* Résout les rôles globaux, déduit C/V/T des formes et prépare toutes les couches
 * contiguës à partir de `blk.0`, sans allocation dynamique. */
int gpt2_gguf_generation_prepare(const gpt2_gguf_loaded_model_t* model,
                                 char* name, uint32_t name_capacity,
                                 gpt2_gguf_layer_t* layers, uint32_t layer_capacity,
                                 gpt2_gguf_generation_t* out);

/* Prépare une couche pour le futur forward sans prendre possession des buffers. */
int gpt2_gguf_forward_context_init(const gpt2_gguf_loaded_model_t* model,
                                   uint32_t layer_index, uint32_t channels,
                                   uint32_t position, char* name, uint32_t name_capacity,
                                   uint8_t* scratch, uint32_t scratch_capacity,
                                   gpt2_gguf_forward_context_t* out);
/* Initialise un cache KV fourni par l’appelant. */
int gpt2_gguf_kv_cache_init(float* storage, uint32_t storage_floats,
                            uint32_t layers, uint32_t max_positions,
                            uint32_t channels, gpt2_gguf_kv_cache_t* out);
/* Réarme le cache KV en O(1) sans effacer le stockage caller-owned. */
int gpt2_gguf_kv_cache_reset(gpt2_gguf_kv_cache_t* cache);
/* Écrit les K/V d’une couche et d’une position dans le cache. */
int gpt2_gguf_kv_cache_put(gpt2_gguf_kv_cache_t* cache, uint32_t layer,
                           uint32_t position, const float* key, const float* value);
/* Relit les K/V d’une couche et d’une position dans les buffers appelant. */
int gpt2_gguf_kv_cache_get(const gpt2_gguf_kv_cache_t* cache, uint32_t layer,
                           uint32_t position, float* key, float* value);
/* Copie un intervalle de positions historiques d’une couche. */
int gpt2_gguf_kv_cache_copy_history(const gpt2_gguf_kv_cache_t* cache, uint32_t layer,
                                    uint32_t start_position, uint32_t position_count,
                                    float* key_out, uint32_t key_capacity,
                                    float* value_out, uint32_t value_capacity,
                                    uint32_t* out_count);
/* Calcule les scores query-key bruts sur un historique d’une couche. */
int gpt2_gguf_kv_cache_query_scores(const gpt2_gguf_kv_cache_t* cache, uint32_t layer,
                                    uint32_t start_position, uint32_t position_count,
                                    const float* query, float* key_scratch,
                                    uint32_t key_scratch_capacity, float* scores,
                                    uint32_t score_capacity, uint32_t* out_count);
/* Accumule les values historiques avec les poids fournis par l’appelant. */
int gpt2_gguf_kv_cache_accumulate_values(const gpt2_gguf_kv_cache_t* cache, uint32_t layer,
                                         uint32_t start_position, uint32_t position_count,
                                         const float* weights, uint32_t weight_capacity,
                                         float* output, uint32_t output_capacity,
                                         uint32_t* out_count);
/* Met les scores query-key à l’échelle par l’inverse de sqrt(head_size). */
int gpt2_gguf_attention_scale_scores(float* scores, uint32_t score_count, uint32_t head_size);
/* Transforme des scores en probabilités par softmax stable, sans allocation. */
int gpt2_gguf_attention_softmax(float* scores, uint32_t score_count, uint32_t* out_count);
/* Exécute l’attention autoregressive complète pour une tête du cache KV. */
int gpt2_gguf_kv_cache_attention_head(const gpt2_gguf_kv_cache_t* cache, uint32_t layer,
                                      uint32_t start_position, uint32_t position_count,
                                      const float* query, uint32_t head_count,
                                      uint32_t head_index, float* key_scratch,
                                      uint32_t key_scratch_capacity, float* scores,
                                      uint32_t score_capacity, float* output,
                                      uint32_t output_capacity, uint32_t* out_count);
/* Concatène les sorties de têtes dans le vecteur d’attention caller-owned. */
int gpt2_gguf_attention_concat_heads(const float* head_outputs, uint32_t head_count,
                                      uint32_t head_size, float* output,
                                      uint32_t output_capacity, uint32_t* out_count);
/* Exécute l’attention complète de toutes les têtes puis concatène les sorties. */
int gpt2_gguf_kv_cache_attention_multi_head(const gpt2_gguf_kv_cache_t* cache, uint32_t layer,
                                            uint32_t start_position, uint32_t position_count,
                                            const float* query, uint32_t head_count,
                                            float* head_outputs, uint32_t head_output_capacity,
                                            float* key_scratch, uint32_t key_scratch_capacity,
                                            float* scores, uint32_t score_capacity,
                                            float* output, uint32_t output_capacity,
                                            uint32_t* out_count);
/* Ajoute la sortie d’attention au résiduel dans le buffer de destination. */
int gpt2_gguf_add_residual(float* residual, uint32_t residual_capacity,
                           const float* attention, uint32_t attention_count);
/* Applique LayerNorm avec gamma/beta caller-owned. */
int gpt2_gguf_layernorm(const float* input, uint32_t input_count,
                        const float* gamma, const float* beta,
                        float epsilon, float* output, uint32_t output_capacity);
/* Applique une approximation GELU freestanding vers un buffer caller-owned. */
int gpt2_gguf_gelu(const float* input, uint32_t input_count,
                   float* output, uint32_t output_capacity);
/* Exécute ffn_up -> biais optionnel -> GELU -> ffn_down -> biais optionnel. */
int gpt2_gguf_mlp_forward_fat16(const fat16_volume_t* volume, const char* filename,
                                const gpt2_gguf_loaded_model_t* model,
                                const gpt2_gguf_tensor_t* up_tensor,
                                const gpt2_gguf_tensor_t* down_tensor,
                                uint32_t channels, uint32_t hidden_channels,
                                const float* input, uint8_t* row_buffer,
                                uint32_t row_capacity, const float* up_bias,
                                const float* down_bias, float* hidden,
                                uint32_t hidden_capacity, float* output,
                                uint32_t output_capacity);
/* Exécute le MLP puis ajoute sa sortie au résiduel caller-owned. */
int gpt2_gguf_mlp_forward_add_residual_fat16(const fat16_volume_t* volume,
                                             const char* filename,
                                             const gpt2_gguf_loaded_model_t* model,
                                             const gpt2_gguf_tensor_t* up_tensor,
                                             const gpt2_gguf_tensor_t* down_tensor,
                                             uint32_t channels, uint32_t hidden_channels,
                                             const float* input, uint8_t* row_buffer,
                                             uint32_t row_capacity, const float* up_bias,
                                             const float* down_bias, float* hidden,
                                             uint32_t hidden_capacity, float* residual,
                                             uint32_t residual_capacity);
/* LayerNorm pré-MLP puis MLP et ajout au résiduel. */
int gpt2_gguf_block_mlp_forward_fat16(const fat16_volume_t* volume,
                                      const char* filename,
                                      const gpt2_gguf_loaded_model_t* model,
                                      const gpt2_gguf_tensor_t* up_tensor,
                                      const gpt2_gguf_tensor_t* down_tensor,
                                      uint32_t channels, uint32_t hidden_channels,
                                      const float* input, const float* gamma,
                                      const float* beta, float epsilon,
                                      float* norm, uint32_t norm_capacity,
                                      uint8_t* row_buffer, uint32_t row_capacity,
                                      const float* up_bias, const float* down_bias,
                                      float* hidden, uint32_t hidden_capacity,
                                      float* residual, uint32_t residual_capacity);
/* Projette la concaténation multi-têtes puis l’ajoute au résiduel. */
int gpt2_gguf_attention_output_add_residual_fat16(
                                      const fat16_volume_t* volume,
                                      const char* filename,
                                      const gpt2_gguf_loaded_model_t* model,
                                      const gpt2_gguf_tensor_t* output_tensor,
                                      uint32_t channels,
                                      const float* attention_concat,
                                      uint8_t* row_buffer, uint32_t row_capacity,
                                      float* projected, uint32_t projected_capacity,
                                      const float* bias, float* residual,
                                      uint32_t residual_capacity);
/* Exécute un bloc GPT-2 complet : LN, QKV, cache KV, attention, projection,
 * résiduel, LN, MLP et résiduel. Les tenseurs et tous les buffers sont caller-owned. */
int gpt2_gguf_block_forward_fat16(
                                      gpt2_gguf_kv_cache_t* cache,
                                      uint32_t layer, uint32_t position,
                                      float* residual, uint32_t residual_capacity,
                                      uint32_t channels, uint32_t head_count,
                                      uint32_t hidden_channels,
                                      const float* attention_gamma,
                                      const float* attention_beta,
                                      const gpt2_gguf_tensor_t* qkv_tensor,
                                      const float* qkv_bias,
                                      const gpt2_gguf_tensor_t* attention_output_tensor,
                                      const float* attention_output_bias,
                                      const float* ffn_gamma,
                                      const float* ffn_beta,
                                      const gpt2_gguf_tensor_t* ffn_up_tensor,
                                      const gpt2_gguf_tensor_t* ffn_down_tensor,
                                      const float* ffn_up_bias,
                                      const float* ffn_down_bias,
                                      float epsilon,
                                      const fat16_volume_t* volume, const char* filename,
                                      const gpt2_gguf_loaded_model_t* model,
                                      gpt2_gguf_block_workspace_t* workspace);

/* Exécute LayerNorm -> attention multi-têtes -> projection -> résiduel. */
/* Exécute un token complet : embeddings token/position, toutes les couches,
 * normalisation finale et logits quantifiés. */
int gpt2_gguf_generation_token_fat16(
                                      const gpt2_gguf_generation_t* generation,
                                      gpt2_gguf_kv_cache_t* cache,
                                      uint32_t token, uint32_t position,
                                      uint32_t head_count, float epsilon,
                                      const fat16_volume_t* volume, const char* filename,
                                      gpt2_gguf_generation_workspace_t* workspace,
                                      float* logits, uint32_t logits_capacity);
/* Exécute le même token complet mais accumule directement les candidats top-k. */
int gpt2_gguf_generation_token_top_k_fat16(
                                      const gpt2_gguf_generation_t* generation,
                                      gpt2_gguf_kv_cache_t* cache,
                                      uint32_t token, uint32_t position,
                                      uint32_t head_count, float epsilon,
                                      const fat16_volume_t* volume, const char* filename,
                                      gpt2_gguf_generation_workspace_t* workspace,
                                      gpt2_sample_top_k_state_t* top_k);

int gpt2_gguf_block_attention_forward_fat16(
                                      const gpt2_gguf_kv_cache_t* cache,
                                      uint32_t layer, uint32_t start_position,
                                      uint32_t position_count, const float* input,
                                      const float* gamma, const float* beta,
                                      float epsilon, float* norm,
                                      uint32_t norm_capacity, uint32_t channels,
                                      uint32_t head_count, float* head_outputs,
                                      uint32_t head_output_capacity, float* key_scratch,
                                      uint32_t key_scratch_capacity, float* scores,
                                      uint32_t score_capacity, float* attention_concat,
                                      uint32_t attention_capacity,
                                      const fat16_volume_t* volume, const char* filename,
                                      const gpt2_gguf_loaded_model_t* model,
                                      const gpt2_gguf_tensor_t* output_tensor,
                                      uint8_t* row_buffer, uint32_t row_capacity,
                                      float* projected, uint32_t projected_capacity,
                                      const float* bias, float* residual,
                                      uint32_t residual_capacity);

/* Lit un fichier FAT16 dans le buffer fourni puis indexe son GGUF complet. */
int gpt2_gguf_load_fat16(const fat16_volume_t* volume, const char* filename,
                         uint8_t* buffer, uint32_t capacity,
                         gpt2_gguf_loaded_model_t* out);
int gpt2_gguf_load_fat32_header(const fat32_volume_t* volume, const char* filename,
                                uint8_t* header, uint32_t header_capacity,
                                gpt2_gguf_loaded_model_t* out);
/* Charge seulement le catalogue GGUF dans un buffer caller-owned. La taille du
 * fichier reste la borne logique des poids, lus ensuite par fenêtres FAT16/FAT32. */
int gpt2_gguf_load_fat16_header(const fat16_volume_t* volume, const char* filename,
                                uint8_t* header, uint32_t header_capacity,
                                gpt2_gguf_loaded_model_t* out);
/* Lit une fenêtre relative aux données d’un tenseur déjà indexé. */
int gpt2_gguf_read_tensor_fat16(const fat16_volume_t* volume, const char* filename,
                                const gpt2_gguf_loaded_model_t* model,
                                const gpt2_gguf_tensor_t* tensor,
                                uint32_t tensor_offset, uint8_t* buffer,
                                uint32_t capacity, uint32_t* out_read);
/* Décode une ligne dense F32/F16 dans un buffer float caller-owned. */
int gpt2_gguf_read_dense_row_fat16(const fat16_volume_t* volume, const char* filename,
                                   const gpt2_gguf_loaded_model_t* model,
                                   const gpt2_gguf_tensor_t* tensor,
                                   uint32_t row_index, uint32_t width,
                                   uint8_t* scratch, uint32_t scratch_capacity,
                                   float* output, uint32_t output_capacity);
/* Lit un super-bloc complet d’un tenseur Q3_K/Q4_K/Q6_K. */
int gpt2_gguf_read_quant_block_fat16(const fat16_volume_t* volume, const char* filename,
                                     const gpt2_gguf_loaded_model_t* model,
                                     const gpt2_gguf_tensor_t* tensor,
                                     uint32_t block_index, uint8_t* buffer,
                                     uint32_t capacity, uint32_t* out_read);
/* Lit puis exécute un super-bloc quantifié avec un scratch caller-owned. */
int gpt2_gguf_dot_quant_block_fat16(const fat16_volume_t* volume, const char* filename,
                                    const gpt2_gguf_loaded_model_t* model,
                                    const gpt2_gguf_tensor_t* tensor,
                                    uint32_t block_index, const float* input,
                                    uint32_t count, uint8_t* scratch,
                                    uint32_t scratch_capacity, float* out_dot);
/* Accumule plusieurs super-blocs sans charger le tenseur complet. */
int gpt2_gguf_dot_quant_tensor_fat16(const fat16_volume_t* volume, const char* filename,
                                     const gpt2_gguf_loaded_model_t* model,
                                     const gpt2_gguf_tensor_t* tensor,
                                     const float* input, uint32_t count,
                                     uint8_t* scratch, uint32_t scratch_capacity,
                                     float* out_dot);
/* Calcule une ligne `shape[0]` d’un tenseur 1D ou 2D. */
int gpt2_gguf_dot_quant_row_fat16(const fat16_volume_t* volume, const char* filename,
                                  const gpt2_gguf_loaded_model_t* model,
                                  const gpt2_gguf_tensor_t* tensor,
                                  uint32_t row_index, const float* input,
                                  uint32_t count, uint8_t* scratch,
                                  uint32_t scratch_capacity, float* out_dot);
/* Lit les octets d’une ligne quantifiée sans allocation ni chargement complet. */
int gpt2_gguf_read_quant_row_fat16(const fat16_volume_t* volume, const char* filename,
                                   const gpt2_gguf_loaded_model_t* model,
                                   const gpt2_gguf_tensor_t* tensor,
                                   uint32_t row_index, uint8_t* buffer,
                                   uint32_t capacity, uint32_t* out_read);
/* Lit puis déquantifie une ligne Q3_K/Q4_K/Q6_K vers des floats caller-owned. */
int gpt2_gguf_read_quant_row_dequant_fat16(const fat16_volume_t* volume,
                                           const char* filename,
                                           const gpt2_gguf_loaded_model_t* model,
                                           const gpt2_gguf_tensor_t* tensor,
                                           uint32_t row_index, uint8_t* row_buffer,
                                           uint32_t row_capacity, float* output,
                                           uint32_t output_capacity);
/* Calcule une ligne quantifiée déjà présente dans un buffer caller-owned. */
int gpt2_gguf_dot_quant_row_buffer(const gpt2_gguf_tensor_t* tensor,
                                   const uint8_t* row_buffer, uint32_t row_capacity,
                                   const float* input, uint32_t count, float* out_dot);
/* Lit et calcule une sortie QKV unique d’une matrice GGUF `[C,3C]`. */
int gpt2_gguf_project_qkv_row_fat16(const fat16_volume_t* volume, const char* filename,
                                    const gpt2_gguf_loaded_model_t* model,
                                    const gpt2_gguf_tensor_t* tensor, uint32_t channels,
                                    uint32_t output_index, const float* input,
                                    uint8_t* row_buffer, uint32_t row_capacity,
                                    float* out_value);
/* Accumule les 3C sorties dans trois buffers query/key/value caller-owned. */
int gpt2_gguf_project_qkv_fat16(const fat16_volume_t* volume, const char* filename,
                                const gpt2_gguf_loaded_model_t* model,
                                const gpt2_gguf_tensor_t* tensor, uint32_t channels,
                                const float* input, uint8_t* row_buffer,
                                uint32_t row_capacity, float* query, uint32_t query_capacity,
                                float* key, uint32_t key_capacity,
                                float* value, uint32_t value_capacity);
/* Projette un vecteur dans une matrice GGUF quantifiée `[input, output]`. */
int gpt2_gguf_project_matrix_fat16(const fat16_volume_t* volume, const char* filename,
                                   const gpt2_gguf_loaded_model_t* model,
                                   const gpt2_gguf_tensor_t* tensor,
                                   uint32_t input_channels, uint32_t output_channels,
                                   const float* input, uint8_t* row_buffer,
                                   uint32_t row_capacity, float* output,
                                   uint32_t output_capacity);
/* Calcule les logits d’une tête de sortie quantifiée depuis un état caché. */
int gpt2_gguf_forward_output_logits_fat16(const fat16_volume_t* volume,
                                          const char* filename,
                                          const gpt2_gguf_loaded_model_t* model,
                                          const gpt2_gguf_tensor_t* output_tensor,
                                          uint32_t channels, uint32_t vocabulary,
                                          const float* hidden, uint8_t* row_buffer,
                                          uint32_t row_capacity, float* logits,
                                          uint32_t logits_capacity);
/* Projette toute la tête quantifiée en alimentant un top-k caller-owned. */
int gpt2_gguf_forward_output_top_k_fat16(const fat16_volume_t* volume,
                                         const char* filename,
                                         const gpt2_gguf_loaded_model_t* model,
                                         const gpt2_gguf_tensor_t* output_tensor,
                                         uint32_t channels, uint32_t vocabulary,
                                         const float* hidden, uint8_t* row_buffer,
                                         uint32_t row_capacity,
                                         gpt2_sample_top_k_state_t* top_k);

#endif
