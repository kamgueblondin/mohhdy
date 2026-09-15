#ifndef MOHHDY_GPT2_GGUF_H
#define MOHHDY_GPT2_GGUF_H

#include <stdint.h>

/* GGUF v3 is little-endian. The reader intentionally accepts only the
 * structural envelope: execution requires explicit quantization kernels. */
#define GPT2_GGUF_MAGIC   0x46554747U /* bytes: G G U F */
#define GPT2_GGUF_VERSION 3U

#define GPT2_GGUF_VALUE_UINT8    0U
#define GPT2_GGUF_VALUE_INT8     1U
#define GPT2_GGUF_VALUE_UINT16   2U
#define GPT2_GGUF_VALUE_INT16    3U
#define GPT2_GGUF_VALUE_UINT32   4U
#define GPT2_GGUF_VALUE_INT32    5U
#define GPT2_GGUF_VALUE_FLOAT32  6U
#define GPT2_GGUF_VALUE_BOOL     7U
#define GPT2_GGUF_VALUE_STRING   8U
#define GPT2_GGUF_VALUE_ARRAY    9U
#define GPT2_GGUF_VALUE_UINT64  10U
#define GPT2_GGUF_VALUE_INT64   11U
#define GPT2_GGUF_VALUE_FLOAT64 12U

#define GPT2_GGUF_TENSOR_F32  0U
#define GPT2_GGUF_TENSOR_F16  1U
#define GPT2_GGUF_TENSOR_Q4_0 2U
#define GPT2_GGUF_TENSOR_Q8_0 8U
#define GPT2_GGUF_TENSOR_Q3_K 11U
#define GPT2_GGUF_TENSOR_Q4_K 12U
#define GPT2_GGUF_TENSOR_Q6_K 14U

#define GPT2_GGUF_MAX_TENSORS 512U

/* A structural report. No pointer from an untrusted blob is exposed. */
typedef struct {
    const uint8_t* name;
    uint32_t name_length;
    uint32_t dimensions;
    uint64_t shape[4];
    uint32_t type;
    uint32_t data_offset;
    uint32_t byte_size;
} gpt2_gguf_tensor_t;

typedef enum {
    GPT2_GGUF_ROLE_TOKEN_EMBEDDING = 0U,
    GPT2_GGUF_ROLE_POSITION_EMBEDDING = 1U,
    GPT2_GGUF_ROLE_OUTPUT_NORM_WEIGHT = 2U,
    GPT2_GGUF_ROLE_OUTPUT_NORM_BIAS = 3U,
    GPT2_GGUF_ROLE_OUTPUT_WEIGHT = 4U,
    GPT2_GGUF_ROLE_LAYER_ATTN_NORM_WEIGHT = 5U,
    GPT2_GGUF_ROLE_LAYER_ATTN_NORM_BIAS = 6U,
    GPT2_GGUF_ROLE_LAYER_ATTN_QKV_WEIGHT = 7U,
    GPT2_GGUF_ROLE_LAYER_ATTN_QKV_BIAS = 8U,
    GPT2_GGUF_ROLE_LAYER_ATTN_OUTPUT_WEIGHT = 9U,
    GPT2_GGUF_ROLE_LAYER_ATTN_OUTPUT_BIAS = 10U,
    GPT2_GGUF_ROLE_LAYER_FFN_NORM_WEIGHT = 11U,
    GPT2_GGUF_ROLE_LAYER_FFN_NORM_BIAS = 12U,
    GPT2_GGUF_ROLE_LAYER_FFN_UP_WEIGHT = 13U,
    GPT2_GGUF_ROLE_LAYER_FFN_DOWN_WEIGHT = 14U,
    GPT2_GGUF_ROLE_LAYER_FFN_UP_BIAS = 15U,
    GPT2_GGUF_ROLE_LAYER_FFN_DOWN_BIAS = 16U
} gpt2_gguf_role_t;

typedef struct {
    uint32_t version;
    uint32_t tensor_count;
    uint32_t metadata_count;
    uint32_t alignment;
    uint32_t tensor_data_offset;
    uint32_t f32_tensors;
    uint32_t q8_0_tensors;
    uint32_t q3_k_tensors;
    uint32_t q4_k_tensors;
    uint32_t q6_k_tensors;
    uint32_t unsupported_quantized_tensors;
    uint8_t is_gpt2;
    uint8_t is_valid;
} gpt2_gguf_info_t;

typedef struct {
    gpt2_gguf_tensor_t tensors[12];
    uint32_t layer_index;
    uint32_t present_mask;
} gpt2_gguf_layer_t;

typedef struct {
    gpt2_gguf_info_t info;
    const uint8_t* blob;
    uint32_t blob_size;
    uint32_t tensor_count;
    gpt2_gguf_tensor_t tensors[GPT2_GGUF_MAX_TENSORS];
} gpt2_gguf_index_t;

/* Returns 0 only for a bounded, structurally valid GPT-2 GGUF v3 blob.
 * The function does not allocate and does not execute model data. */
int gpt2_gguf_probe_blob(const uint8_t* blob, uint32_t blob_size,
                         gpt2_gguf_info_t* out);

/* Human-readable status for `ai-runtime` and diagnostics. */
const char* gpt2_gguf_probe_status(int status);

/* Find one descriptor and validate its data range without copying the blob. */
int gpt2_gguf_find_tensor(const uint8_t* blob, uint32_t blob_size,
                          const char* name, gpt2_gguf_tensor_t* out);

/* Builds a caller-owned table so repeated lookups do not rescan the blob. */
int gpt2_gguf_build_index(const uint8_t* blob, uint32_t blob_size,
                          gpt2_gguf_index_t* out);
/* Indexe un en-tête GGUF déjà chargé. `header_size` borne toute lecture mémoire,
 * tandis que `file_size` valide les plages des tenseurs laissés sur FAT16. */
int gpt2_gguf_build_index_header(const uint8_t* blob, uint32_t header_size,
                                 uint32_t file_size, gpt2_gguf_index_t* out);
int gpt2_gguf_index_find(const gpt2_gguf_index_t* index, const char* name,
                         gpt2_gguf_tensor_t* out);
/* Maps the stable GPT-2 GGUF names to semantic model roles. */
int gpt2_gguf_map_role(const gpt2_gguf_index_t* index, gpt2_gguf_role_t role,
                       gpt2_gguf_tensor_t* out);
/* Construit le nom GGUF d’une couche dans le buffer fourni puis la résout. */
int gpt2_gguf_map_layer_role(const gpt2_gguf_index_t* index, uint32_t layer,
                             gpt2_gguf_role_t role, char* name, uint32_t capacity,
                             gpt2_gguf_tensor_t* out);
/* Résout les dix rôles d’une couche dans un descripteur caller-owned. */
int gpt2_gguf_map_layer(const gpt2_gguf_index_t* index, uint32_t layer,
                        char* name, uint32_t capacity, gpt2_gguf_layer_t* out);
/* Retourne une vue bornée d’un rôle déjà résolu dans une couche. */
int gpt2_gguf_layer_get(const gpt2_gguf_layer_t* layer, gpt2_gguf_role_t role,
                        gpt2_gguf_tensor_t* out);
/* Valide les invariants de forme nécessaires à une couche quantifiée. */
int gpt2_gguf_validate_layer(const gpt2_gguf_layer_t* layer, uint32_t channels);
/* Valide les rangs et axes GPT-2 legacy: C, 3C et 4C. */
int gpt2_gguf_validate_gpt2_layer(const gpt2_gguf_layer_t* layer, uint32_t channels);
/* Vérifie que byte_size correspond au produit des axes et au type GGUF. */
int gpt2_gguf_validate_tensor_size(const gpt2_gguf_tensor_t* tensor);
/* Combine la validation des axes GPT-2 et des tailles physiques d’une couche. */
int gpt2_gguf_validate_gpt2_layer_storage(const gpt2_gguf_layer_t* layer, uint32_t channels);

#endif
