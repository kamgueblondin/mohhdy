#include <stdio.h>
/* test_gpt2_gguf.c - bounded structural parsing for GGUF v3. */

#include "../../framework/unity.h"
#include "../../../kernel/llm/gpt2_gguf.h"
#include "../../../kernel/llm/gpt2_gguf_loader.h"
#include "../../../kernel/llm/gpt2_quant.h"

#define BUF_SIZE 700000U

static uint8_t blob[BUF_SIZE];
static uint32_t cursor;

static void reset_blob(void) {
    uint32_t i;
    for (i = 0U; i < BUF_SIZE; i++) blob[i] = 0U;
    cursor = 0U;
}

static void put_u32(uint32_t value) {
    blob[cursor++] = (uint8_t)(value & 0xffU);
    blob[cursor++] = (uint8_t)((value >> 8) & 0xffU);
    blob[cursor++] = (uint8_t)((value >> 16) & 0xffU);
    blob[cursor++] = (uint8_t)((value >> 24) & 0xffU);
}

static void put_u64(uint64_t value) {
    put_u32((uint32_t)value);
    put_u32((uint32_t)(value >> 32));
}

static void put_text(const char* text) {
    uint32_t count = 0U;
    while (text[count]) count++;
    put_u64(count);
    while (*text) blob[cursor++] = (uint8_t)*text++;
}

static void put_metadata_string(const char* key, const char* value) {
    put_text(key);
    put_u32(GPT2_GGUF_VALUE_STRING);
    put_text(value);
}

static void put_metadata_u32(const char* key, uint32_t value) {
    put_text(key);
    put_u32(GPT2_GGUF_VALUE_UINT32);
    put_u32(value);
}

static void put_tensor(const char* name, uint32_t dimensions, uint64_t first,
                       uint64_t second, uint32_t type, uint64_t offset) {
    put_text(name);
    put_u32(dimensions);
    put_u64(first);
    if (dimensions > 1U) put_u64(second);
    put_u32(type);
    put_u64(offset);
}

static uint32_t make_valid_gpt2(uint32_t type, uint64_t data_offset) {
    uint32_t padded;
    reset_blob();
    put_u32(GPT2_GGUF_MAGIC);
    put_u32(GPT2_GGUF_VERSION);
    put_u64(1U);
    put_u64(2U);
    put_metadata_string("general.architecture", "gpt2");
    put_metadata_u32("general.alignment", 32U);
    put_tensor("token_embd.weight", 2U, 768U, 50257U, type, data_offset);
    padded = (cursor + 31U) & ~31U;
    while (cursor < padded) blob[cursor++] = 0U;
    return cursor;
}

static uint32_t make_small_tensor(uint32_t type, uint64_t data_offset) {
    uint32_t padded;
    uint32_t i;
    reset_blob();
    put_u32(GPT2_GGUF_MAGIC);
    put_u32(GPT2_GGUF_VERSION);
    put_u64(1U);
    put_u64(2U);
    put_metadata_string("general.architecture", "gpt2");
    put_metadata_u32("general.alignment", 32U);
    put_tensor("blk.0.weight", 1U, GPT2_QK_K, 0U, type, data_offset);
    padded = (cursor + 31U) & ~31U;
    while (cursor < padded) blob[cursor++] = 0U;
    for (i = 0U; i < GPT2_Q4_K_BLOCK_BYTES; i++) blob[cursor++] = 0U;
    return cursor;
}

static uint32_t make_role_tensors(void) {
    static const char* const names[] = {
        "token_embd.weight", "position_embd.weight", "output_norm.weight",
        "output_norm.bias", "output.weight",
        "blk.0.attn_norm.weight", "blk.0.attn_norm.bias",
        "blk.0.attn_qkv.weight", "blk.0.attn_qkv.bias",
        "blk.0.attn_output.weight", "blk.0.attn_output.bias",
        "blk.0.ffn_norm.weight", "blk.0.ffn_norm.bias",
        "blk.0.ffn_up.weight", "blk.0.ffn_down.weight",
        "blk.0.ffn_up.bias", "blk.0.ffn_down.bias"
    };
    uint32_t i;
    uint32_t padded;
    reset_blob();
    put_u32(GPT2_GGUF_MAGIC);
    put_u32(GPT2_GGUF_VERSION);
    put_u64(17U);
    put_u64(2U);
    put_metadata_string("general.architecture", "gpt2");
    put_metadata_u32("general.alignment", 32U);
    for (i = 0U; i < 17U; i++) {
        put_tensor(names[i], 1U, GPT2_QK_K, 0U, GPT2_GGUF_TENSOR_Q4_K, (uint64_t)(i * 160U));
    }
    padded = 4096U;
    while (cursor < padded) blob[cursor++] = 0U;
    return cursor;
}

static uint32_t descriptor_bytes(uint32_t type, uint32_t width, uint32_t rows) {
    uint32_t elements = width * rows;
    if (type == GPT2_GGUF_TENSOR_F32) return elements * 4U;
    if (type == GPT2_GGUF_TENSOR_F16) return elements * 2U;
    if (type == GPT2_GGUF_TENSOR_Q3_K) return (elements / GPT2_QK_K) * GPT2_Q3_K_BLOCK_BYTES;
    if (type == GPT2_GGUF_TENSOR_Q4_K) return (elements / GPT2_QK_K) * GPT2_Q4_K_BLOCK_BYTES;
    return (elements / GPT2_QK_K) * GPT2_Q6_K_BLOCK_BYTES;
}

static void set_descriptor(gpt2_gguf_tensor_t* tensor, const char* name,
                           uint32_t width, uint32_t rows, uint32_t type) {
    uint32_t length = 0U;
    while (name[length]) length++;
    tensor->name = (const uint8_t*)name;
    tensor->name_length = length;
    tensor->dimensions = rows == 1U ? 1U : 2U;
    tensor->shape[0] = width;
    tensor->shape[1] = rows;
    tensor->shape[2] = 0U;
    tensor->shape[3] = 0U;
    tensor->type = type;
    tensor->data_offset = 0U;
    tensor->byte_size = descriptor_bytes(type, width, rows);
}

static void test_prepares_generation_context_from_descriptors(void) {
    static const char* const names[] = {
        "token_embd.weight", "position_embd.weight", "output_norm.weight",
        "output_norm.bias", "output.weight",
        "blk.0.attn_norm.weight", "blk.0.attn_norm.bias",
        "blk.0.attn_qkv.weight", "blk.0.attn_qkv.bias",
        "blk.0.attn_output.weight", "blk.0.attn_output.bias",
        "blk.0.ffn_norm.weight", "blk.0.ffn_norm.bias",
        "blk.0.ffn_up.weight", "blk.0.ffn_down.weight",
        "blk.0.ffn_up.bias", "blk.0.ffn_down.bias"
    };
    gpt2_gguf_loaded_model_t model = {0};
    gpt2_gguf_layer_t layers[1];
    gpt2_gguf_generation_t generation = {0};
    char name[64];
    uint32_t i;
    model.index.info.is_valid = 1U;
    model.index.tensor_count = 17U;
    set_descriptor(&model.index.tensors[0], names[0], GPT2_QK_K, 4U, GPT2_GGUF_TENSOR_F32);
    set_descriptor(&model.index.tensors[1], names[1], GPT2_QK_K, 2U, GPT2_GGUF_TENSOR_F16);
    set_descriptor(&model.index.tensors[2], names[2], GPT2_QK_K, 1U, GPT2_GGUF_TENSOR_F32);
    set_descriptor(&model.index.tensors[3], names[3], GPT2_QK_K, 1U, GPT2_GGUF_TENSOR_F16);
    set_descriptor(&model.index.tensors[4], names[4], GPT2_QK_K, 4U, GPT2_GGUF_TENSOR_Q4_K);
    for (i = 5U; i < 17U; i++) {
        uint32_t width = GPT2_QK_K;
        uint32_t rows = 1U;
        uint32_t type = GPT2_GGUF_TENSOR_F32;
        if (i == 7U) { rows = 3U * GPT2_QK_K; type = GPT2_GGUF_TENSOR_Q4_K; }
        else if (i == 8U) width = 3U * GPT2_QK_K;
        else if (i == 9U) { rows = GPT2_QK_K; type = GPT2_GGUF_TENSOR_Q4_K; }
        else if (i == 13U) { rows = 4U * GPT2_QK_K; type = GPT2_GGUF_TENSOR_Q4_K; }
        else if (i == 14U) { width = 4U * GPT2_QK_K; rows = GPT2_QK_K; type = GPT2_GGUF_TENSOR_Q4_K; }
        else if (i == 15U) width = 4U * GPT2_QK_K;
        set_descriptor(&model.index.tensors[i], names[i], width, rows, type);
    }
    TEST_ASSERT_EQUAL(0, gpt2_gguf_generation_prepare(&model, name, sizeof(name),
                                                       layers, 1U, &generation));
    TEST_ASSERT_EQUAL(1, generation.ready);
    TEST_ASSERT_EQUAL(1, generation.runtime.ready);
    TEST_ASSERT_EQUAL(1, generation.runtime.layer_count);
    TEST_ASSERT_EQUAL(GPT2_QK_K, generation.runtime.channels);
    TEST_ASSERT_EQUAL(4, generation.vocabulary);
    TEST_ASSERT_EQUAL(2, generation.max_positions);
    TEST_ASSERT_EQUAL(0, generation.runtime.layers[0].layer_index);
    model.index.tensors[3].type = GPT2_GGUF_TENSOR_Q4_K;
    TEST_ASSERT_EQUAL(-9, gpt2_gguf_generation_prepare(&model, name, sizeof(name),
                                                        layers, 1U, &generation));
}

static void test_accepts_gpt2_q8_0_envelope(void) {
    gpt2_gguf_info_t info;
    uint32_t size = make_valid_gpt2(GPT2_GGUF_TENSOR_Q8_0, 0U);
    TEST_ASSERT_EQUAL(0, gpt2_gguf_probe_blob(blob, size, &info));
    TEST_ASSERT_EQUAL(1, info.is_valid);
    TEST_ASSERT_EQUAL(1, info.is_gpt2);
    TEST_ASSERT_EQUAL(3, info.version);
    TEST_ASSERT_EQUAL(1, info.tensor_count);
    TEST_ASSERT_EQUAL(1, info.q8_0_tensors);
    TEST_ASSERT_EQUAL(0, info.unsupported_quantized_tensors);
    TEST_ASSERT_EQUAL(32, info.alignment);
}

static void test_reports_supported_k_quantized_tensors(void) {
    gpt2_gguf_info_t info;
    uint32_t size;
    size = make_valid_gpt2(GPT2_GGUF_TENSOR_Q3_K, 0U);
    TEST_ASSERT_EQUAL(0, gpt2_gguf_probe_blob(blob, size, &info));
    TEST_ASSERT_EQUAL(1, info.q3_k_tensors);
    TEST_ASSERT_EQUAL(0, info.unsupported_quantized_tensors);
    size = make_valid_gpt2(GPT2_GGUF_TENSOR_Q4_K, 0U);
    TEST_ASSERT_EQUAL(0, gpt2_gguf_probe_blob(blob, size, &info));
    TEST_ASSERT_EQUAL(1, info.q4_k_tensors);
    TEST_ASSERT_EQUAL(0, info.unsupported_quantized_tensors);
    size = make_valid_gpt2(GPT2_GGUF_TENSOR_Q6_K, 0U);
    TEST_ASSERT_EQUAL(0, gpt2_gguf_probe_blob(blob, size, &info));
    TEST_ASSERT_EQUAL(1, info.q6_k_tensors);
    TEST_ASSERT_EQUAL(0, info.unsupported_quantized_tensors);
}

static void test_finds_bounded_q4_tensor(void) {
    gpt2_gguf_tensor_t tensor;
    uint32_t size = make_small_tensor(GPT2_GGUF_TENSOR_Q4_K, 0U);
    TEST_ASSERT_EQUAL(0, gpt2_gguf_find_tensor(blob, size, "blk.0.weight", &tensor));
    TEST_ASSERT_EQUAL(GPT2_GGUF_TENSOR_Q4_K, tensor.type);
    TEST_ASSERT_EQUAL(1, tensor.dimensions);
    TEST_ASSERT_EQUAL(GPT2_QK_K, (int)tensor.shape[0]);
    TEST_ASSERT_EQUAL(GPT2_Q4_K_BLOCK_BYTES, tensor.byte_size);
    TEST_ASSERT_EQUAL(0, tensor.data_offset);
    TEST_ASSERT_EQUAL(-8, gpt2_gguf_find_tensor(blob, size, "missing", &tensor));
    size = make_small_tensor(GPT2_GGUF_TENSOR_Q4_K, 32U);
    TEST_ASSERT_EQUAL(-7, gpt2_gguf_find_tensor(blob, size, "blk.0.weight", &tensor));
}

static void test_builds_index_and_maps_gpt2_roles(void) {
    gpt2_gguf_index_t index;
    gpt2_gguf_tensor_t tensor;
    uint32_t size = make_role_tensors();
    TEST_ASSERT_EQUAL(0, gpt2_gguf_build_index(blob, size, &index));
    char name[40];
    TEST_ASSERT_EQUAL(17, index.tensor_count);
    TEST_ASSERT_EQUAL(0, gpt2_gguf_index_find(&index, "output.weight", &tensor));
    TEST_ASSERT_EQUAL(GPT2_GGUF_TENSOR_Q4_K, tensor.type);
    TEST_ASSERT_EQUAL(4 * 160, (int)tensor.data_offset);
    TEST_ASSERT_EQUAL(0, gpt2_gguf_validate_tensor_size(&tensor));
    tensor.byte_size--;
    TEST_ASSERT_EQUAL(-9, gpt2_gguf_validate_tensor_size(&tensor));
    tensor.byte_size++;
    tensor.shape[0] = GPT2_QK_K - 1U;
    TEST_ASSERT_EQUAL(-9, gpt2_gguf_validate_tensor_size(&tensor));
    TEST_ASSERT_EQUAL(0, gpt2_gguf_map_role(&index, GPT2_GGUF_ROLE_TOKEN_EMBEDDING, &tensor));
    TEST_ASSERT_EQUAL(0, gpt2_gguf_map_role(&index, GPT2_GGUF_ROLE_POSITION_EMBEDDING, &tensor));
    TEST_ASSERT_EQUAL(0, gpt2_gguf_map_role(&index, GPT2_GGUF_ROLE_OUTPUT_NORM_WEIGHT, &tensor));
    TEST_ASSERT_EQUAL(0, gpt2_gguf_map_role(&index, GPT2_GGUF_ROLE_OUTPUT_NORM_BIAS, &tensor));
    TEST_ASSERT_EQUAL(0, gpt2_gguf_map_role(&index, GPT2_GGUF_ROLE_OUTPUT_WEIGHT, &tensor));
    TEST_ASSERT_EQUAL(-1, gpt2_gguf_map_role(&index, (gpt2_gguf_role_t)99, &tensor));
    TEST_ASSERT_EQUAL(0, gpt2_gguf_map_layer_role(&index, 0U, GPT2_GGUF_ROLE_LAYER_ATTN_NORM_WEIGHT, name, sizeof(name), &tensor));
    TEST_ASSERT_EQUAL_STRING("blk.0.attn_norm.weight", name);
    TEST_ASSERT_EQUAL(0, gpt2_gguf_map_layer_role(&index, 0U, GPT2_GGUF_ROLE_LAYER_ATTN_QKV_WEIGHT, name, sizeof(name), &tensor));
    TEST_ASSERT_EQUAL_STRING("blk.0.attn_qkv.weight", name);
    TEST_ASSERT_EQUAL(0, gpt2_gguf_map_layer_role(&index, 0U, GPT2_GGUF_ROLE_LAYER_FFN_UP_BIAS, name, sizeof(name), &tensor));
    TEST_ASSERT_EQUAL_STRING("blk.0.ffn_up.bias", name);
    TEST_ASSERT_EQUAL(-8, gpt2_gguf_map_layer_role(&index, 1U, GPT2_GGUF_ROLE_LAYER_ATTN_NORM_WEIGHT, name, sizeof(name), &tensor));
    TEST_ASSERT_EQUAL(-2, gpt2_gguf_map_layer_role(&index, 0U, GPT2_GGUF_ROLE_LAYER_ATTN_QKV_WEIGHT, name, 8U, &tensor));
    {
        gpt2_gguf_layer_t layer;
        TEST_ASSERT_EQUAL(0, gpt2_gguf_map_layer(&index, 0U, name, sizeof(name), &layer));
        TEST_ASSERT_EQUAL(0xFFF, (int)layer.present_mask);
        TEST_ASSERT_EQUAL(0, (int)layer.layer_index);
        TEST_ASSERT_EQUAL(0, gpt2_gguf_layer_get(&layer, GPT2_GGUF_ROLE_LAYER_FFN_DOWN_WEIGHT, &tensor));
        TEST_ASSERT_EQUAL(0, gpt2_gguf_layer_get(&layer, GPT2_GGUF_ROLE_LAYER_FFN_UP_BIAS, &tensor));
        TEST_ASSERT_EQUAL(-1, gpt2_gguf_layer_get(&layer, GPT2_GGUF_ROLE_OUTPUT_WEIGHT, &tensor));
        layer.tensors[0].dimensions = 1U; layer.tensors[0].shape[0] = GPT2_QK_K;
        layer.tensors[1].dimensions = 1U; layer.tensors[1].shape[0] = GPT2_QK_K;
        layer.tensors[2].dimensions = 2U; layer.tensors[2].shape[0] = GPT2_QK_K; layer.tensors[2].shape[1] = 3U * GPT2_QK_K;
        layer.tensors[3].dimensions = 1U; layer.tensors[3].shape[0] = 3U * GPT2_QK_K;
        layer.tensors[4].dimensions = 2U; layer.tensors[4].shape[0] = GPT2_QK_K; layer.tensors[4].shape[1] = GPT2_QK_K;
        layer.tensors[5].dimensions = 1U; layer.tensors[5].shape[0] = GPT2_QK_K;
        layer.tensors[6].dimensions = 1U; layer.tensors[6].shape[0] = GPT2_QK_K;
        layer.tensors[7].dimensions = 1U; layer.tensors[7].shape[0] = GPT2_QK_K;
        layer.tensors[8].dimensions = 2U; layer.tensors[8].shape[0] = GPT2_QK_K; layer.tensors[8].shape[1] = 4U * GPT2_QK_K;
        layer.tensors[9].dimensions = 2U; layer.tensors[9].shape[0] = 4U * GPT2_QK_K; layer.tensors[9].shape[1] = GPT2_QK_K;
        layer.tensors[10].dimensions = 1U; layer.tensors[10].shape[0] = 4U * GPT2_QK_K; layer.tensors[10].type = GPT2_GGUF_TENSOR_F32;
        layer.tensors[11].dimensions = 1U; layer.tensors[11].shape[0] = GPT2_QK_K; layer.tensors[11].type = GPT2_GGUF_TENSOR_F32;
        layer.tensors[0].byte_size = GPT2_Q4_K_BLOCK_BYTES;
        layer.tensors[1].byte_size = GPT2_Q4_K_BLOCK_BYTES;
        layer.tensors[2].byte_size = 3U * GPT2_QK_K * GPT2_Q4_K_BLOCK_BYTES;
        layer.tensors[3].byte_size = 3U * GPT2_Q4_K_BLOCK_BYTES;
        layer.tensors[4].byte_size = GPT2_QK_K * GPT2_Q4_K_BLOCK_BYTES;
        layer.tensors[5].byte_size = GPT2_Q4_K_BLOCK_BYTES;
        layer.tensors[6].byte_size = GPT2_Q4_K_BLOCK_BYTES;
        layer.tensors[7].byte_size = GPT2_Q4_K_BLOCK_BYTES;
        layer.tensors[8].byte_size = 4U * GPT2_QK_K * GPT2_Q4_K_BLOCK_BYTES;
        layer.tensors[9].byte_size = 4U * GPT2_QK_K * GPT2_Q4_K_BLOCK_BYTES;
        layer.tensors[10].byte_size = 4U * GPT2_QK_K * 4U;
        layer.tensors[11].byte_size = GPT2_QK_K * 4U;
        TEST_ASSERT_EQUAL(0, gpt2_gguf_validate_gpt2_layer(&layer, GPT2_QK_K));
        TEST_ASSERT_EQUAL(0, gpt2_gguf_validate_gpt2_layer_storage(&layer, GPT2_QK_K));
        layer.tensors[8].byte_size--;
        TEST_ASSERT_EQUAL(-9, gpt2_gguf_validate_gpt2_layer_storage(&layer, GPT2_QK_K));
        layer.tensors[8].byte_size++;
        layer.tensors[2].shape[1] = 2U * GPT2_QK_K;
        TEST_ASSERT_EQUAL(-9, gpt2_gguf_validate_gpt2_layer(&layer, GPT2_QK_K));
        layer.tensors[2].shape[1] = 3U * GPT2_QK_K;
        TEST_ASSERT_EQUAL(0, gpt2_gguf_validate_layer(&layer, GPT2_QK_K));
        TEST_ASSERT_EQUAL(-9, gpt2_gguf_validate_layer(&layer, GPT2_QK_K - 1U));
        layer.tensors[0].shape[0] = GPT2_QK_K - 1U;
        TEST_ASSERT_EQUAL(-9, gpt2_gguf_validate_layer(&layer, GPT2_QK_K));
        layer.tensors[0].shape[0] = GPT2_QK_K;
        layer.present_mask = 0U;
        TEST_ASSERT_EQUAL(-8, gpt2_gguf_layer_get(&layer, GPT2_GGUF_ROLE_LAYER_FFN_DOWN_WEIGHT, &tensor));
        TEST_ASSERT_EQUAL(-8, gpt2_gguf_validate_layer(&layer, GPT2_QK_K));
        TEST_ASSERT_EQUAL(-8, gpt2_gguf_map_layer(&index, 1U, name, sizeof(name), &layer));
    }
}

static void test_rejects_non_gpt2_architecture(void) {
    gpt2_gguf_info_t info;
    uint32_t size;
    reset_blob();
    put_u32(GPT2_GGUF_MAGIC);
    put_u32(GPT2_GGUF_VERSION);
    put_u64(0U);
    put_u64(1U);
    put_metadata_string("general.architecture", "llama");
    size = cursor;
    TEST_ASSERT_EQUAL(-4, gpt2_gguf_probe_blob(blob, size, &info));
}

static void test_rejects_unaligned_tensor_offset(void) {
    gpt2_gguf_info_t info;
    uint32_t size = make_valid_gpt2(GPT2_GGUF_TENSOR_Q8_0, 1U);
    TEST_ASSERT_EQUAL(-9, gpt2_gguf_probe_blob(blob, size, &info));
}

static void test_rejects_bad_magic_and_truncation(void) {
    gpt2_gguf_info_t info;
    uint32_t size = make_valid_gpt2(GPT2_GGUF_TENSOR_Q8_0, 0U);
    blob[0] = 0U;
    TEST_ASSERT_EQUAL(-2, gpt2_gguf_probe_blob(blob, size, &info));
    TEST_ASSERT_TRUE(gpt2_gguf_probe_blob(blob, 3U, &info) != 0);
}

static int read_sector_fat32_mock(uint32_t lba, void* buffer) {
    if (!buffer || (lba + 1U) * 512U > BUF_SIZE) return -1;
    uint32_t i;
    for (i = 0U; i < 512U; i++) ((uint8_t*)buffer)[i] = blob[lba * 512U + i];
    return 0;
}

static void put16_buf(uint8_t* p, uint16_t v) { p[0] = (uint8_t)v; p[1] = (uint8_t)(v >> 8U); }
static void put32_buf(uint8_t* p, uint32_t v) { p[0] = (uint8_t)v; p[1] = (uint8_t)(v >> 8U); p[2] = (uint8_t)(v >> 16U); p[3] = (uint8_t)(v >> 24U); }

static void test_fat32_gguf_header_and_read(void) {
    fat32_volume_t volume;
    gpt2_gguf_loaded_model_t model;
    uint8_t header[256];
    uint32_t gguf_len;
    uint32_t fat_lba = 32U;
    uint32_t root_lba = 432U;
    uint32_t data_lba = 432U;
    uint32_t file_cluster = 3U;

    reset_blob();
    // Créer une structure BPB FAT32 minimale dans blob
    blob[13] = 2U; // 2 secteurs par cluster
    put16_buf(blob + 11U, 512U);
    put16_buf(blob + 14U, 32U); // reserved
    blob[16] = 2U; // fats
    put32_buf(blob + 32U, 140000U); // total sectors
    put32_buf(blob + 36U, 200U); // fat sectors
    put32_buf(blob + 44U, 2U); // root cluster
    put16_buf(blob + 510U, 0xaa55U);

    // Entrée FAT pour le cluster 2 (racine) et cluster 3 (fichier)
    blob[fat_lba * 512U + 8U] = 0xf8U; blob[fat_lba * 512U + 9U] = 0xffU;
    blob[fat_lba * 512U + 10U] = 0xffU; blob[fat_lba * 512U + 11U] = 0x0fU;
    blob[fat_lba * 512U + 12U] = 0xf8U; blob[fat_lba * 512U + 13U] = 0xffU;
    blob[fat_lba * 512U + 14U] = 0xffU; blob[fat_lba * 512U + 15U] = 0x0fU;

    // Fichier GGUF simulé au cluster 3
    uint32_t p = (data_lba + (file_cluster - 2U) * 2U) * 512U;
    uint32_t file_start = p;
    put32_buf(blob + p, GPT2_GGUF_MAGIC); p += 4U;
    put32_buf(blob + p, GPT2_GGUF_VERSION); p += 4U;
    put32_buf(blob + p, 1U); put32_buf(blob + p + 4U, 0U); p += 8U; // 1 tensor
    put32_buf(blob + p, 2U); put32_buf(blob + p + 4U, 0U); p += 8U; // 2 metadata
    // Écrire directement dans blob
    const char* k1 = "general.architecture";
    uint32_t k1_len = 20U;
    put32_buf(blob + p, k1_len); put32_buf(blob + p + 4U, 0U); p += 8U;
    for (uint32_t i = 0U; i < k1_len; i++) blob[p++] = (uint8_t)k1[i];
    put32_buf(blob + p, GPT2_GGUF_VALUE_STRING); p += 4U;
    const char* v1 = "gpt2";
    uint32_t v1_len = 4U;
    put32_buf(blob + p, v1_len); put32_buf(blob + p + 4U, 0U); p += 8U;
    for (uint32_t i = 0U; i < v1_len; i++) blob[p++] = (uint8_t)v1[i];

    const char* k2 = "general.alignment";
    uint32_t k2_len = 17U;
    put32_buf(blob + p, k2_len); put32_buf(blob + p + 4U, 0U); p += 8U;
    for (uint32_t i = 0U; i < k2_len; i++) blob[p++] = (uint8_t)k2[i];
    put32_buf(blob + p, GPT2_GGUF_VALUE_UINT32); p += 4U;
    put32_buf(blob + p, 32U); p += 4U;

    // Tensor output.weight
    const char* tname = "output.weight";
    uint32_t tlen = 13U;
    put32_buf(blob + p, tlen); put32_buf(blob + p + 4U, 0U); p += 8U;
    for (uint32_t i = 0U; i < tlen; i++) blob[p++] = (uint8_t)tname[i];
    put32_buf(blob + p, 2U); p += 4U; // 2D
    put32_buf(blob + p, GPT2_QK_K); put32_buf(blob + p + 4U, 0U); p += 8U;
    put32_buf(blob + p, 2U); put32_buf(blob + p + 4U, 0U); p += 8U;
    put32_buf(blob + p, GPT2_GGUF_TENSOR_Q4_K); p += 4U;
    put32_buf(blob + p, 0U); put32_buf(blob + p + 4U, 0U); p += 8U;

    gguf_len = p - file_start;
    if (gguf_len < 512U) gguf_len = 512U;

    // Entrée de répertoire dans le root_cluster (LBA 432)
    uint8_t* dirent = blob + root_lba * 512U;
    const char* short_name = "MODEL   GGU";
    for (uint32_t i = 0U; i < 11U; i++) dirent[i] = (uint8_t)short_name[i];
    dirent[11] = 0x20U;
    dirent[20] = (uint8_t)(file_cluster >> 24U); dirent[21] = (uint8_t)(file_cluster >> 16U);
    dirent[26] = (uint8_t)file_cluster; dirent[27] = (uint8_t)(file_cluster >> 8U);
    put32_buf(dirent + 28U, gguf_len);

    // Initialiser le reste de l'entrée de répertoire à 0 (notamment les autres slots)
    for (uint32_t i = 32U; i < 512U; i++) dirent[i] = 0U;

    TEST_ASSERT_EQUAL(0, fat32_mount(&volume, read_sector_fat32_mock, 0U));
    int res = gpt2_gguf_load_fat32_header(&volume, "MODEL.GGU", header, sizeof(header), &model);
    TEST_ASSERT_EQUAL(0, res);
    TEST_ASSERT_EQUAL(1, (int)model.index.tensor_count);
    TEST_ASSERT_EQUAL(1, (int)model.index.info.is_valid);
}

int main(void) {
    unity_init();
    RUN_TEST(test_prepares_generation_context_from_descriptors);
    RUN_TEST(test_accepts_gpt2_q8_0_envelope);
    RUN_TEST(test_reports_supported_k_quantized_tensors);
    RUN_TEST(test_finds_bounded_q4_tensor);
    RUN_TEST(test_builds_index_and_maps_gpt2_roles);
    RUN_TEST(test_rejects_non_gpt2_architecture);
    RUN_TEST(test_rejects_unaligned_tensor_offset);
    RUN_TEST(test_rejects_bad_magic_and_truncation);
    RUN_TEST(test_fat32_gguf_header_and_read);
    unity_print_results();
    unity_cleanup();
    return unity_stats.tests_failed == 0 ? 0 : 1;
}
