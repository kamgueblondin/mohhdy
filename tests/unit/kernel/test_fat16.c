#include "../../framework/unity.h"
#include "../../../kernel/fs/fat16.h"
#include "../../../kernel/llm/gpt2_gguf_loader.h"
#include "../../../kernel/llm/gpt2_quant.h"

#define TEST_SECTORS 32768U
#define BLOCK_CHANNELS GPT2_QK_K
#define BLOCK_HIDDEN (4U * GPT2_QK_K)
#define BLOCK_QKV_BYTES (3U * BLOCK_CHANNELS * GPT2_Q4_K_BLOCK_BYTES)
#define BLOCK_ATTN_BYTES (BLOCK_CHANNELS * GPT2_Q4_K_BLOCK_BYTES)
#define BLOCK_UP_BYTES (BLOCK_HIDDEN * GPT2_Q4_K_BLOCK_BYTES)
#define BLOCK_DOWN_BYTES (BLOCK_HIDDEN * GPT2_Q4_K_BLOCK_BYTES)
#define BLOCK_TOKEN_BYTES (BLOCK_CHANNELS * 4U * 4U)
#define BLOCK_POSITION_BYTES (BLOCK_CHANNELS * 2U * 2U)
#define BLOCK_NORM_BYTES (BLOCK_CHANNELS * 4U)
#define BLOCK_FFN_UP_BIAS_BYTES (BLOCK_HIDDEN * 4U)
#define BLOCK_FFN_DOWN_BIAS_BYTES (BLOCK_CHANNELS * 4U)
#define BLOCK_OUTPUT_BYTES (4U * GPT2_Q4_K_BLOCK_BYTES)
#define BLOCK_GLOBAL_BYTES (BLOCK_TOKEN_BYTES + BLOCK_POSITION_BYTES + 2U * BLOCK_NORM_BYTES + BLOCK_OUTPUT_BYTES)
#define BLOCK_MODEL_BUFFER_BYTES 500000U
static uint8_t disk[TEST_SECTORS * 512U];
static uint8_t block_model_buffer[BLOCK_MODEL_BUFFER_BYTES];
static uint32_t read_sector_calls;
static uint32_t read_sectors_calls;

static uint16_t le16(uint32_t off) {
    return (uint16_t)disk[off] | ((uint16_t)disk[off + 1U] << 8);
}

static void put16(uint32_t off, uint16_t value) {
    disk[off] = (uint8_t)value;
    disk[off + 1U] = (uint8_t)(value >> 8);
}

static void put32(uint32_t off, uint32_t value) {
    put16(off, (uint16_t)value);
    put16(off + 2U, (uint16_t)(value >> 16));
}

static int read_sector(uint32_t lba, void* out) {
    uint32_t i;
    if (!out || lba >= TEST_SECTORS) return -1;
    read_sector_calls++;
    for (i = 0U; i < 512U; i++) ((uint8_t*)out)[i] = disk[lba * 512U + i];
    return 0;
}
static int read_sectors(uint32_t lba, uint32_t count, void* out) {
    uint32_t sector_index;
    uint32_t i;
    if (!out || count == 0U || lba >= TEST_SECTORS || count > TEST_SECTORS - lba) return -1;
    read_sectors_calls++;
    for (sector_index = 0U; sector_index < count; sector_index++) {
        for (i = 0U; i < 512U; i++) {
            ((uint8_t*)out)[sector_index * 512U + i] = disk[(lba + sector_index) * 512U + i];
        }
    }
    return 0;
}

static int write_sector(uint32_t lba, const void* in) {
    uint32_t i;
    if (!in || lba >= TEST_SECTORS) return -1;
    for (i = 0U; i < 512U; i++) disk[lba * 512U + i] = ((const uint8_t*)in)[i];
    return 0;
}

static void make_volume(void) {
    uint32_t fat;
    uint32_t root = (1U + 2U * 17U) * 512U;
    uint32_t data = (root / 512U + 2U) * 512U;
    uint32_t i;
    read_sector_calls = 0U;
    for (i = 0U; i < sizeof(disk); i++) disk[i] = 0U;
    put16(11U, 512U);
    disk[13] = 1U;
    put16(14U, 1U);
    disk[16] = 2U;
    put16(17U, 32U);
    put16(19U, TEST_SECTORS);
    disk[21] = 0xF8U;
    put16(22U, 17U);
    put16(510U, 0xAA55U);
    for (fat = 1U; fat <= 2U; fat++) {
        uint32_t base = (1U + (fat - 1U) * 17U) * 512U;
        put16(base, 0xFFF8U);
        put16(base + 2U, 0xFFFFU);
        put16(base + 4U, 0xFFFFU);
    }
    disk[root + 0U] = 'F'; disk[root + 1U] = 'A'; disk[root + 2U] = 'T';
    disk[root + 3U] = 'O'; disk[root + 4U] = 'K'; disk[root + 5U] = ' '; disk[root + 6U] = ' '; disk[root + 7U] = ' ';
    disk[root + 8U] = 'T'; disk[root + 9U] = 'X'; disk[root + 10U] = 'T';
    disk[root + 11U] = 0x20U;
    put16(root + 26U, 2U);
    put32(root + 28U, 5U);
    disk[data + 0U] = 'h'; disk[data + 1U] = 'e'; disk[data + 2U] = 'l'; disk[data + 3U] = 'l'; disk[data + 4U] = 'o';
}

static void put64_at(uint32_t off, uint64_t value) {
    put32(off, (uint32_t)value);
    put32(off + 4U, (uint32_t)(value >> 32));
}

static void put_text_at(uint32_t* cursor, const char* text) {
    uint32_t i = 0U;
    while (text[i]) i++;
    put64_at(*cursor, i);
    *cursor += 8U;
    while (*text) disk[(*cursor)++] = (uint8_t)*text++;
}

static void make_gguf_file(void) {
    uint32_t root = (1U + 2U * 17U) * 512U;
    uint32_t data = (root / 512U + 2U) * 512U;
    uint32_t fat;
    uint32_t p = data + 512U;
    uint32_t end;
    uint32_t tensor_data;
    make_volume();
    for (fat = 1U; fat <= 2U; fat++) put16((1U + (fat - 1U) * 17U) * 512U + 6U, 0xFFFFU);
    disk[root + 32U] = 'G'; disk[root + 33U] = 'P'; disk[root + 34U] = 'T'; disk[root + 35U] = '2';
    disk[root + 36U] = ' '; disk[root + 37U] = ' '; disk[root + 38U] = ' '; disk[root + 39U] = ' ';
    disk[root + 40U] = 'G'; disk[root + 41U] = 'G'; disk[root + 42U] = 'U'; disk[root + 43U] = 0x20U;
    put16(root + 32U + 26U, 3U);
    put32(root + 32U + 28U, 480U);
    put32(p, GPT2_GGUF_MAGIC); p += 4U;
    put32(p, GPT2_GGUF_VERSION); p += 4U;
    put64_at(p, 1U); p += 8U;
    put64_at(p, 2U); p += 8U;
    put_text_at(&p, "general.architecture"); put32(p, GPT2_GGUF_VALUE_STRING); p += 4U; put_text_at(&p, "gpt2");
    put_text_at(&p, "general.alignment"); put32(p, GPT2_GGUF_VALUE_UINT32); p += 4U; put32(p, 32U); p += 4U;
    put_text_at(&p, "output.weight"); put32(p, 2U); p += 4U; put64_at(p, GPT2_QK_K); p += 8U;
    put64_at(p, 2U); p += 8U;
    put32(p, GPT2_GGUF_TENSOR_Q4_K); p += 4U; put64_at(p, 0U); p += 8U;
    end = data + 512U + 480U;
    tensor_data = p;
    while ((tensor_data & 31U) != 0U) tensor_data++;
    while (p < end) disk[p++] = 0U;
    disk[tensor_data] = 0x11U;
    disk[tensor_data + GPT2_Q4_K_BLOCK_BYTES] = 0x77U;
}

static void test_indexes_gpt2_header_from_fat16(void) {
    fat16_volume_t volume;
    gpt2_gguf_loaded_model_t model;
    gpt2_gguf_tensor_t tensor;
    float decoded[GPT2_QK_K] = {0.0f};
    uint8_t row[GPT2_Q4_K_BLOCK_BYTES] = {0U};
    uint8_t header[160];
    uint8_t short_header[120];
    make_gguf_file();
    TEST_ASSERT_EQUAL(0, fat16_mount(&volume, read_sector, 0U));
    TEST_ASSERT_EQUAL(0, gpt2_gguf_load_fat16_header(&volume, "gpt2.ggu", header,
                                                      sizeof(header), &model));
    TEST_ASSERT_EQUAL(sizeof(header), model.bytes_loaded);
    TEST_ASSERT_EQUAL(480U, model.index.blob_size);
    TEST_ASSERT_EQUAL(1U, model.index.tensor_count);
    TEST_ASSERT_EQUAL(0, gpt2_gguf_index_find(&model.index, "output.weight", &tensor));
    TEST_ASSERT_EQUAL(GPT2_GGUF_TENSOR_Q4_K, tensor.type);
    TEST_ASSERT_EQUAL(0, gpt2_gguf_read_quant_row_dequant_fat16(&volume, "gpt2.ggu", &model,
                                                                 &tensor, 0U, row, sizeof(row),
                                                                 decoded, GPT2_QK_K));
    TEST_ASSERT_EQUAL(-6, gpt2_gguf_read_quant_row_dequant_fat16(&volume, "gpt2.ggu", &model,
                                                                  &tensor, 0U, row, sizeof(row),
                                                                  decoded, GPT2_QK_K - 1U));
    TEST_ASSERT_TRUE(gpt2_gguf_load_fat16_header(&volume, "gpt2.ggu", short_header,
                                                 sizeof(short_header), &model) != 0);
}

static void test_loads_gpt2_from_fat16(void) {
    fat16_volume_t volume;
    gpt2_gguf_loaded_model_t model;
    gpt2_gguf_tensor_t tensor;
    uint8_t buffer[512];
    make_gguf_file();
    TEST_ASSERT_EQUAL(0, fat16_mount(&volume, read_sector, 0U));
    TEST_ASSERT_EQUAL(0, gpt2_gguf_load_fat16(&volume, "gpt2.ggu", buffer, sizeof(buffer), &model));
    {
        gpt2_gguf_forward_context_t context;
        uint8_t context_scratch[GPT2_Q4_K_BLOCK_BYTES];
        char context_name[40];
        TEST_ASSERT_EQUAL(-1, gpt2_gguf_forward_context_init(&model, 0U, 0U, 0U,
                                                               context_name, sizeof(context_name),
                                                               context_scratch, sizeof(context_scratch), &context));
        TEST_ASSERT_EQUAL(-1, gpt2_gguf_forward_context_init(&model, 0U, 256U, 0U,
                                                               context_name, sizeof(context_name),
                                                               0, sizeof(context_scratch), &context));
    }
    {
        float storage[2U * 3U * 2U * 4U] = {0.0f};
        float key[4] = {1.0f, 2.0f, 3.0f, 4.0f};
        float value[4] = {5.0f, 6.0f, 7.0f, 8.0f};
        float key_out[4] = {0.0f};
        float value_out[4] = {0.0f};
        gpt2_gguf_kv_cache_t cache;
        TEST_ASSERT_EQUAL(0, gpt2_gguf_kv_cache_init(storage, 48U, 2U, 3U, 4U, &cache));
        TEST_ASSERT_EQUAL(0, gpt2_gguf_kv_cache_put(&cache, 1U, 0U, key, value));
        key[0] = 9.0f; value[0] = 19.0f;
        TEST_ASSERT_EQUAL(0, gpt2_gguf_kv_cache_put(&cache, 1U, 1U, key, value));
        key[0] = 1.0f; value[0] = 5.0f;
        TEST_ASSERT_EQUAL(0, gpt2_gguf_kv_cache_put(&cache, 1U, 2U, key, value));
        TEST_ASSERT_EQUAL(3, (int)cache.count);
        TEST_ASSERT_EQUAL(0, gpt2_gguf_kv_cache_get(&cache, 1U, 2U, key_out, value_out));
        TEST_ASSERT_EQUAL(1, (int)key_out[0]);
        TEST_ASSERT_EQUAL(8, (int)value_out[3]);
        TEST_ASSERT_EQUAL(-9, gpt2_gguf_kv_cache_get(&cache, 2U, 0U, key_out, value_out));
        TEST_ASSERT_EQUAL(-6, gpt2_gguf_kv_cache_init(storage, 47U, 2U, 3U, 4U, &cache));
        TEST_ASSERT_EQUAL(-1, gpt2_gguf_kv_cache_put(&cache, 0U, 0U, 0, value));
        {
            float history_key[12] = {0.0f};
            float history_value[12] = {0.0f};
            uint32_t history_count = 0U;
            TEST_ASSERT_EQUAL(0, gpt2_gguf_kv_cache_copy_history(&cache, 1U, 0U, 3U,
                                                                  history_key, 12U, history_value, 12U,
                                                                  &history_count));
            TEST_ASSERT_EQUAL(3, (int)history_count);
            TEST_ASSERT_EQUAL(1, (int)history_key[0]);
            TEST_ASSERT_EQUAL(9, (int)history_key[4]);
            TEST_ASSERT_EQUAL(19, (int)history_value[4]);
            TEST_ASSERT_EQUAL(-9, gpt2_gguf_kv_cache_copy_history(&cache, 0U, 2U, 2U,
                                                                   history_key, 12U, history_value, 12U,
                                                                   &history_count));
            TEST_ASSERT_EQUAL(-6, gpt2_gguf_kv_cache_copy_history(&cache, 1U, 0U, 3U,
                                                                   history_key, 8U, history_value, 12U,
                                                                   &history_count));
            {
                float query[4] = {1.0f, 1.0f, 1.0f, 1.0f};
                float key_scratch[4] = {0.0f};
                float scores[3] = {0.0f};
                TEST_ASSERT_EQUAL(0, gpt2_gguf_kv_cache_query_scores(&cache, 1U, 0U, 3U,
                                                                      query, key_scratch, 4U,
                                                                      scores, 3U, &history_count));
                TEST_ASSERT_EQUAL(10, (int)scores[0]);
                TEST_ASSERT_EQUAL(18, (int)scores[1]);
                TEST_ASSERT_EQUAL(10, (int)scores[2]);
                TEST_ASSERT_EQUAL(-6, gpt2_gguf_kv_cache_query_scores(&cache, 1U, 0U, 3U,
                                                                       query, key_scratch, 3U,
                                                                       scores, 3U, &history_count));
                TEST_ASSERT_EQUAL(-9, gpt2_gguf_kv_cache_query_scores(&cache, 1U, 2U, 2U,
                                                                       query, key_scratch, 4U,
                                                                       scores, 3U, &history_count));
                {
                    float weights[3] = {0.2f, 0.3f, 0.5f};
                    float output[4] = {99.0f, 99.0f, 99.0f, 99.0f};
                    TEST_ASSERT_EQUAL(0, gpt2_gguf_kv_cache_accumulate_values(&cache, 1U, 0U, 3U,
                                                                                weights, 3U, output, 4U,
                                                                                &history_count));
                    TEST_ASSERT_EQUAL(4, (int)history_count);
                    TEST_ASSERT_EQUAL(92, (int)(output[0] * 10.0f));
                    TEST_ASSERT_EQUAL(60, (int)(output[1] * 10.0f));
                    TEST_ASSERT_EQUAL(70, (int)(output[2] * 10.0f));
                    TEST_ASSERT_EQUAL(80, (int)(output[3] * 10.0f));
                    TEST_ASSERT_EQUAL(-6, gpt2_gguf_kv_cache_accumulate_values(&cache, 1U, 0U, 3U,
                                                                                 weights, 2U, output, 4U,
                                                                                 &history_count));
                    TEST_ASSERT_EQUAL(-6, gpt2_gguf_kv_cache_accumulate_values(&cache, 1U, 0U, 3U,
                                                                                 weights, 3U, output, 3U,
                                                                                 &history_count));
                    TEST_ASSERT_EQUAL(-9, gpt2_gguf_kv_cache_accumulate_values(&cache, 1U, 2U, 2U,
                                                                                 weights, 3U, output, 4U,
                                                                                 &history_count));
                }
                {
                    float scaled[2] = {4.0f, 0.0f};
                    float probabilities[3] = {0.0f, 1.0f, 2.0f};
                    uint32_t attention_count = 0U;
                    TEST_ASSERT_EQUAL(0, gpt2_gguf_attention_scale_scores(scaled, 2U, 4U));
                    TEST_ASSERT_EQUAL(199, (int)(scaled[0] * 100.0f));
                    TEST_ASSERT_EQUAL(0, (int)(scaled[1] * 100.0f));
                    TEST_ASSERT_EQUAL(-1, gpt2_gguf_attention_scale_scores(scaled, 2U, 0U));
                    TEST_ASSERT_EQUAL(0, gpt2_gguf_attention_softmax(probabilities, 3U, &attention_count));
                    TEST_ASSERT_EQUAL(3, (int)attention_count);
                    TEST_ASSERT_TRUE(probabilities[0] < probabilities[1]);
                    TEST_ASSERT_TRUE(probabilities[1] < probabilities[2]);
                    TEST_ASSERT_EQUAL(100, (int)((probabilities[0] + probabilities[1] + probabilities[2]) * 100.0f));
                    TEST_ASSERT_EQUAL(0, gpt2_gguf_attention_softmax(probabilities, 0U, &attention_count));
                    TEST_ASSERT_EQUAL(0, (int)attention_count);
                    TEST_ASSERT_EQUAL(-1, gpt2_gguf_attention_softmax(0, 3U, &attention_count));
                }
                {
                    float head_query[2] = {1.0f, 1.0f};
                    float head_key_scratch[2] = {0.0f};
                    float head_scores[3] = {0.0f};
                    float head_output[2] = {0.0f};
                    uint32_t attention_count = 0U;
                    TEST_ASSERT_EQUAL(0, gpt2_gguf_kv_cache_attention_head(&cache, 1U, 0U, 3U,
                                                                            head_query, 2U, 0U,
                                                                            head_key_scratch, 2U,
                                                                            head_scores, 3U,
                                                                            head_output, 2U,
                                                                            &attention_count));
                    TEST_ASSERT_EQUAL(2, (int)attention_count);
                    TEST_ASSERT_TRUE(head_output[0] > 15.0f);
                    TEST_ASSERT_TRUE(head_output[0] < 19.5f);
                    TEST_ASSERT_TRUE(head_output[1] > 5.9f);
                    TEST_ASSERT_TRUE(head_output[1] < 6.1f);
                    TEST_ASSERT_EQUAL(0, gpt2_gguf_kv_cache_attention_head(&cache, 1U, 0U, 3U,
                                                                            head_query, 2U, 1U,
                                                                            head_key_scratch, 2U,
                                                                            head_scores, 3U,
                                                                            head_output, 2U,
                                                                            &attention_count));
                    TEST_ASSERT_TRUE(head_output[0] > 6.9f);
                    TEST_ASSERT_TRUE(head_output[0] < 7.1f);
                    TEST_ASSERT_TRUE(head_output[1] > 7.9f);
                    TEST_ASSERT_TRUE(head_output[1] < 8.1f);
                    TEST_ASSERT_EQUAL(-9, gpt2_gguf_kv_cache_attention_head(&cache, 1U, 0U, 3U,
                                                                            head_query, 2U, 2U,
                                                                            head_key_scratch, 2U,
                                                                            head_scores, 3U,
                                                                            head_output, 2U,
                                                                            &attention_count));
                    TEST_ASSERT_EQUAL(-6, gpt2_gguf_kv_cache_attention_head(&cache, 1U, 0U, 3U,
                                                                            head_query, 2U, 0U,
                                                                            head_key_scratch, 1U,
                                                                            head_scores, 3U,
                                                                            head_output, 2U,
                                                                            &attention_count));
                }
                {
                    float heads[4] = {1.0f, 2.0f, 3.0f, 4.0f};
                    float concatenated[4] = {0.0f};
                    uint32_t concat_count = 0U;
                    TEST_ASSERT_EQUAL(0, gpt2_gguf_attention_concat_heads(heads, 2U, 2U,
                                                                          concatenated, 4U,
                                                                          &concat_count));
                    TEST_ASSERT_EQUAL(4, (int)concat_count);
                    TEST_ASSERT_EQUAL(1, (int)concatenated[0]);
                    TEST_ASSERT_EQUAL(4, (int)concatenated[3]);
                    TEST_ASSERT_EQUAL(-6, gpt2_gguf_attention_concat_heads(heads, 2U, 2U,
                                                                            concatenated, 3U,
                                                                            &concat_count));
                    TEST_ASSERT_EQUAL(-9, gpt2_gguf_attention_concat_heads(heads, 0U, 2U,
                                                                            concatenated, 4U,
                                                                            &concat_count));
                    {
                        float multi_query[4] = {1.0f, 1.0f, 1.0f, 1.0f};
                        float multi_heads[4] = {0.0f};
                        float multi_output[4] = {0.0f};
                        float multi_key[2] = {0.0f};
                        float multi_scores[3] = {0.0f};
                        TEST_ASSERT_EQUAL(0, gpt2_gguf_kv_cache_attention_multi_head(
                            &cache, 1U, 0U, 3U, multi_query, 2U,
                            multi_heads, 4U, multi_key, 2U, multi_scores, 3U,
                            multi_output, 4U, &concat_count));
                        TEST_ASSERT_EQUAL(4, (int)concat_count);
                        TEST_ASSERT_TRUE(multi_output[0] > 15.0f);
                        TEST_ASSERT_TRUE(multi_output[2] > 6.9f);
                        TEST_ASSERT_EQUAL(-9, gpt2_gguf_kv_cache_attention_multi_head(
                            &cache, 1U, 0U, 3U, multi_query, 3U,
                            multi_heads, 4U, multi_key, 2U, multi_scores, 3U,
                            multi_output, 4U, &concat_count));
                        TEST_ASSERT_EQUAL(-6, gpt2_gguf_kv_cache_attention_multi_head(
                            &cache, 1U, 0U, 3U, multi_query, 2U,
                            multi_heads, 3U, multi_key, 2U, multi_scores, 3U,
                            multi_output, 4U, &concat_count));
                    }
                    {
                        float residual[4] = {1.0f, 2.0f, 3.0f, 4.0f};
                        float attention[4] = {0.5f, 1.5f, 2.5f, 3.5f};
                        TEST_ASSERT_EQUAL(0, gpt2_gguf_add_residual(residual, 4U, attention, 4U));
                        TEST_ASSERT_EQUAL(1, (int)residual[0]);
                        TEST_ASSERT_EQUAL(3, (int)residual[1]);
                        TEST_ASSERT_EQUAL(5, (int)residual[2]);
                        TEST_ASSERT_EQUAL(7, (int)residual[3]);
                        TEST_ASSERT_EQUAL(-6, gpt2_gguf_add_residual(residual, 3U, attention, 4U));
                        TEST_ASSERT_EQUAL(-1, gpt2_gguf_add_residual(0, 4U, attention, 4U));
                    }
                    {
                        float norm_input[4] = {1.0f, 2.0f, 3.0f, 4.0f};
                        float norm_gamma[4] = {1.0f, 1.0f, 1.0f, 1.0f};
                        float norm_beta[4] = {0.0f, 0.0f, 0.0f, 0.0f};
                        float norm_output[4] = {0.0f};
                        float gelu_output[3] = {0.0f};
                        TEST_ASSERT_EQUAL(0, gpt2_gguf_layernorm(norm_input, 4U, norm_gamma,
                                                                  norm_beta, 0.0001f,
                                                                  norm_output, 4U));
                        TEST_ASSERT_TRUE(norm_output[0] < -1.2f);
                        TEST_ASSERT_TRUE(norm_output[3] > 1.2f);
                        TEST_ASSERT_EQUAL(-6, gpt2_gguf_layernorm(norm_input, 4U, norm_gamma,
                                                                  norm_beta, 0.0f,
                                                                  norm_output, 4U));
                        TEST_ASSERT_EQUAL(0, gpt2_gguf_gelu((float[]){-1.0f, 0.0f, 1.0f}, 3U,
                                                            gelu_output, 3U));
                        TEST_ASSERT_TRUE(gelu_output[0] < 0.0f);
                        TEST_ASSERT_EQUAL(0, (int)gelu_output[1]);
                        TEST_ASSERT_TRUE(gelu_output[2] > 0.5f);
                        TEST_ASSERT_EQUAL(-6, gpt2_gguf_gelu(norm_input, 4U, gelu_output, 3U));
                    }
                    {
                        float mlp_input[256] = {0.0f};
                        float mlp_hidden[2] = {0.0f};
                        float mlp_output[256] = {0.0f};
                        uint8_t mlp_row[2U * GPT2_Q6_K_BLOCK_BYTES];
                        float block_gamma[256] = {0.0f};
                        float block_beta[256] = {0.0f};
                        float block_norm[256] = {0.0f};
                        gpt2_gguf_runtime_t runtime = {0};
                        TEST_ASSERT_EQUAL(-9, gpt2_gguf_mlp_forward_fat16(
                            &volume, "gpt2.ggu", &model, &tensor, &tensor,
                            GPT2_QK_K, 2U, mlp_input, mlp_row, sizeof(mlp_row), 0, 0,
                            mlp_hidden, 2U, mlp_output, GPT2_QK_K));
                        TEST_ASSERT_EQUAL(-6, gpt2_gguf_mlp_forward_fat16(
                            &volume, "gpt2.ggu", &model, &tensor, &tensor,
                            GPT2_QK_K, 2U, mlp_input, mlp_row, sizeof(mlp_row), 0, 0,
                            mlp_hidden, 1U, mlp_output, GPT2_QK_K));
                        TEST_ASSERT_EQUAL(-6, gpt2_gguf_mlp_forward_add_residual_fat16(
                            &volume, "gpt2.ggu", &model, &tensor, &tensor,
                            GPT2_QK_K, 2U, mlp_input, mlp_row, sizeof(mlp_row), 0, 0,
                            mlp_hidden, 2U, mlp_output, GPT2_QK_K - 1U));
                        TEST_ASSERT_EQUAL(-1, gpt2_gguf_mlp_forward_add_residual_fat16(
                            &volume, "gpt2.ggu", &model, &tensor, &tensor,
                            GPT2_QK_K, 2U, mlp_input, mlp_row, sizeof(mlp_row), 0, 0,
                            mlp_hidden, 2U, 0, GPT2_QK_K));
                        TEST_ASSERT_EQUAL(-6, gpt2_gguf_block_mlp_forward_fat16(
                            &volume, "gpt2.ggu", &model, &tensor, &tensor,
                            GPT2_QK_K, 2U, mlp_input, block_gamma, block_beta, 0.0001f,
                            block_norm, GPT2_QK_K - 1U, mlp_row, sizeof(mlp_row),
                            0, 0, mlp_hidden, 2U, mlp_output, GPT2_QK_K));
                        TEST_ASSERT_EQUAL(-1, gpt2_gguf_block_mlp_forward_fat16(
                            &volume, "gpt2.ggu", &model, &tensor, &tensor,
                            GPT2_QK_K, 2U, mlp_input, block_gamma, block_beta, 0.0001f,
                            block_norm, GPT2_QK_K, mlp_row, sizeof(mlp_row),
                            0, 0, mlp_hidden, 2U, 0, GPT2_QK_K));
                        TEST_ASSERT_EQUAL(-9, gpt2_gguf_attention_output_add_residual_fat16(
                            &volume, "gpt2.ggu", &model, &tensor, GPT2_QK_K,
                            mlp_input, mlp_row, sizeof(mlp_row), mlp_output,
                            GPT2_QK_K, 0, mlp_output, GPT2_QK_K));
                        TEST_ASSERT_EQUAL(-6, gpt2_gguf_attention_output_add_residual_fat16(
                            &volume, "gpt2.ggu", &model, &tensor, 2U,
                            mlp_input, mlp_row, sizeof(mlp_row), mlp_output,
                            2U, 0, mlp_output, 1U));
                        TEST_ASSERT_EQUAL(-1, gpt2_gguf_block_attention_forward_fat16(
                            0, 0U, 0U, 0U, 0, 0, 0, 0.0001f,
                            0, 0U, 0U, 0U, 0, 0U, 0, 0U,
                            0, 0U, 0, 0U, 0, 0, 0, 0, 0, 0U,
                            0, 0U, 0, 0, 0U));
                        TEST_ASSERT_EQUAL(0, gpt2_gguf_kv_cache_reset(&cache));
                        TEST_ASSERT_EQUAL(0, (int)cache.count);
                        TEST_ASSERT_EQUAL(-1, gpt2_gguf_kv_cache_reset(0));
                        TEST_ASSERT_EQUAL(-1, gpt2_gguf_runtime_prepare(
                            0, 1U, GPT2_QK_K, (char*)mlp_row, sizeof(mlp_row),
                            (gpt2_gguf_layer_t*)mlp_output, 1U, 0));
                        TEST_ASSERT_EQUAL(-1, gpt2_gguf_runtime_get_layer(0, 0U, 0));
                        TEST_ASSERT_EQUAL(-1, gpt2_gguf_runtime_get_layer(&runtime, 0U, 0));
                        TEST_ASSERT_EQUAL(-6, gpt2_gguf_runtime_prepare(
                            &model, 1U, GPT2_QK_K, (char*)mlp_row, sizeof(mlp_row),
                            (gpt2_gguf_layer_t*)mlp_output, 0U, &runtime));
                    }
                }
            }
        }
    }
    TEST_ASSERT_EQUAL(480, (int)model.bytes_loaded);
    TEST_ASSERT_EQUAL(1, model.index.tensor_count);
    TEST_ASSERT_EQUAL(0, gpt2_gguf_map_role(&model.index, GPT2_GGUF_ROLE_OUTPUT_WEIGHT, &tensor));
    TEST_ASSERT_EQUAL(2, (int)tensor.dimensions);
    TEST_ASSERT_EQUAL(GPT2_GGUF_TENSOR_Q4_K, tensor.type);
    {
        uint8_t tensor_bytes[4] = {0xFFU, 0xFFU, 0xFFU, 0xFFU};
        uint32_t tensor_read = 0U;
        TEST_ASSERT_EQUAL(0, gpt2_gguf_read_tensor_fat16(&volume, "gpt2.ggu", &model, &tensor, 0U, tensor_bytes, sizeof(tensor_bytes), &tensor_read));
        TEST_ASSERT_EQUAL(4, (int)tensor_read);
        TEST_ASSERT_EQUAL(0x11, tensor_bytes[0]);
        TEST_ASSERT_EQUAL(0, tensor_bytes[1]);
        TEST_ASSERT_EQUAL(0, tensor_bytes[2]);
        TEST_ASSERT_EQUAL(0, tensor_bytes[3]);
        {
            gpt2_gguf_tensor_t dense = tensor;
            uint8_t dense_scratch[4] = {0U};
            float dense_output[1] = {0.0f};
            dense.dimensions = 1U;
            dense.shape[0] = 1U;
            dense.shape[1] = 0U;
            dense.type = GPT2_GGUF_TENSOR_F32;
            dense.byte_size = 4U;
            TEST_ASSERT_EQUAL(0, gpt2_gguf_read_dense_row_fat16(
                &volume, "gpt2.ggu", &model, &dense, 0U, 1U,
                dense_scratch, sizeof(dense_scratch), dense_output, sizeof(dense_output)));
            TEST_ASSERT_TRUE(dense_output[0] > 0.0f);
            TEST_ASSERT_EQUAL(-6, gpt2_gguf_read_dense_row_fat16(
                &volume, "gpt2.ggu", &model, &dense, 0U, 1U,
                dense_scratch, 1U, dense_output, sizeof(dense_output)));
            dense.type = GPT2_GGUF_TENSOR_F16;
            dense.byte_size = 2U;
            TEST_ASSERT_EQUAL(0, gpt2_gguf_read_dense_row_fat16(
                &volume, "gpt2.ggu", &model, &dense, 0U, 1U,
                dense_scratch, sizeof(dense_scratch), dense_output, sizeof(dense_output)));
            TEST_ASSERT_TRUE(dense_output[0] > 0.0f);
            dense.type = GPT2_GGUF_TENSOR_Q4_K;
            TEST_ASSERT_EQUAL(-4, gpt2_gguf_read_dense_row_fat16(
                &volume, "gpt2.ggu", &model, &dense, 0U, 1U,
                dense_scratch, sizeof(dense_scratch), dense_output, sizeof(dense_output)));
        }
    }
    {
        uint8_t block[GPT2_Q4_K_BLOCK_BYTES];
        uint32_t block_read = 0U;
        TEST_ASSERT_EQUAL(0, gpt2_gguf_read_quant_block_fat16(&volume, "gpt2.ggu", &model, &tensor, 0U, block, sizeof(block), &block_read));
        TEST_ASSERT_EQUAL(GPT2_Q4_K_BLOCK_BYTES, (int)block_read);
        TEST_ASSERT_EQUAL(0x11, block[0]);
        TEST_ASSERT_EQUAL(0, gpt2_gguf_read_quant_block_fat16(&volume, "gpt2.ggu", &model, &tensor, 1U, block, sizeof(block), &block_read));
        TEST_ASSERT_EQUAL(GPT2_Q4_K_BLOCK_BYTES, (int)block_read);
        TEST_ASSERT_EQUAL(0x77, block[0]);
        TEST_ASSERT_EQUAL(-6, gpt2_gguf_read_quant_block_fat16(&volume, "gpt2.ggu", &model, &tensor, 0U, block, 1U, &block_read));
        {
            uint8_t row[2U * GPT2_Q6_K_BLOCK_BYTES];
            uint32_t row_read = 0U;
            gpt2_gguf_tensor_t variant = tensor;
            TEST_ASSERT_EQUAL(0, gpt2_gguf_read_quant_row_fat16(&volume, "gpt2.ggu", &model, &tensor, 0U, row, sizeof(row), &row_read));
            TEST_ASSERT_EQUAL(GPT2_Q4_K_BLOCK_BYTES, (int)row_read);
            TEST_ASSERT_EQUAL(0x11, row[0]);
            TEST_ASSERT_EQUAL(0, gpt2_gguf_read_quant_row_fat16(&volume, "gpt2.ggu", &model, &tensor, 1U, row, sizeof(row), &row_read));
            TEST_ASSERT_EQUAL(0x77, row[0]);
            variant.type = GPT2_GGUF_TENSOR_Q3_K;
            variant.byte_size = 2U * GPT2_Q3_K_BLOCK_BYTES;
            TEST_ASSERT_EQUAL(0, gpt2_gguf_read_quant_row_fat16(&volume, "gpt2.ggu", &model, &variant, 0U, row, sizeof(row), &row_read));
            TEST_ASSERT_EQUAL(GPT2_Q3_K_BLOCK_BYTES, (int)row_read);
            variant.type = GPT2_GGUF_TENSOR_Q6_K;
            variant.byte_size = 2U * GPT2_Q6_K_BLOCK_BYTES;
            TEST_ASSERT_EQUAL(0, gpt2_gguf_read_quant_row_fat16(&volume, "gpt2.ggu", &model, &variant, 0U, row, sizeof(row), &row_read));
            TEST_ASSERT_EQUAL(GPT2_Q6_K_BLOCK_BYTES, (int)row_read);
            TEST_ASSERT_EQUAL(-6, gpt2_gguf_read_quant_row_fat16(&volume, "gpt2.ggu", &model, &tensor, 0U, row, 1U, &row_read));
            {
                float input[GPT2_QK_K] = {0.0f};
                float dot = 1.0f;
                TEST_ASSERT_EQUAL(0, gpt2_gguf_dot_quant_row_buffer(&tensor, row, sizeof(row), input, GPT2_QK_K, &dot));
                TEST_ASSERT_EQUAL(0, (int)dot);
                variant.type = GPT2_GGUF_TENSOR_Q3_K;
                TEST_ASSERT_EQUAL(0, gpt2_gguf_dot_quant_row_buffer(&variant, row, sizeof(row), input, GPT2_QK_K, &dot));
                variant.type = GPT2_GGUF_TENSOR_Q6_K;
                TEST_ASSERT_EQUAL(0, gpt2_gguf_dot_quant_row_buffer(&variant, row, sizeof(row), input, GPT2_QK_K, &dot));
                TEST_ASSERT_EQUAL(-6, gpt2_gguf_dot_quant_row_buffer(&tensor, row, 1U, input, GPT2_QK_K, &dot));
                TEST_ASSERT_EQUAL(-7, gpt2_gguf_dot_quant_row_buffer(&tensor, row, sizeof(row), input, 32U, &dot));
                {
                    float projected[2] = {99.0f, 99.0f};
                    TEST_ASSERT_EQUAL(0, gpt2_gguf_project_matrix_fat16(&volume, "gpt2.ggu", &model,
                                                                        &tensor, GPT2_QK_K, 2U,
                                                                        input, row, sizeof(row),
                                                                        projected, 2U * sizeof(float)));
                    TEST_ASSERT_EQUAL(0, (int)projected[0]);
                    TEST_ASSERT_EQUAL(0, (int)projected[1]);
                    TEST_ASSERT_EQUAL(-6, gpt2_gguf_project_matrix_fat16(&volume, "gpt2.ggu", &model,
                                                                         &tensor, GPT2_QK_K, 2U,
                                                                         input, row, sizeof(row),
                                                                         projected, sizeof(float)));
                    TEST_ASSERT_EQUAL(-9, gpt2_gguf_project_matrix_fat16(&volume, "gpt2.ggu", &model,
                                                                         &tensor, 32U, 2U,
                                                                         input, row, sizeof(row),
                                                                         projected, 2U * sizeof(float)));
                    {
                        float logits[2] = {99.0f, 99.0f};
                        TEST_ASSERT_EQUAL(0, gpt2_gguf_forward_output_logits_fat16(
                            &volume, "gpt2.ggu", &model, &tensor, GPT2_QK_K, 2U,
                            input, row, sizeof(row), logits, sizeof(logits)));
                        TEST_ASSERT_EQUAL(0, (int)logits[0]);
                        TEST_ASSERT_EQUAL(0, (int)logits[1]);
                        TEST_ASSERT_EQUAL(-6, gpt2_gguf_forward_output_logits_fat16(
                            &volume, "gpt2.ggu", &model, &tensor, GPT2_QK_K, 2U,
                            input, row, sizeof(row), logits, sizeof(float)));
                        TEST_ASSERT_EQUAL(-9, gpt2_gguf_forward_output_logits_fat16(
                            &volume, "gpt2.ggu", &model, &tensor, 32U, 2U,
                            input, row, sizeof(row), logits, sizeof(logits)));
                    }
                }
            {
                gpt2_gguf_tensor_t qkv = tensor;
                qkv.shape[1] = 3U * GPT2_QK_K;
                qkv.byte_size = 3U * GPT2_QK_K * GPT2_Q4_K_BLOCK_BYTES;
                TEST_ASSERT_EQUAL(0, gpt2_gguf_project_qkv_row_fat16(&volume, "gpt2.ggu", &model, &qkv,
                                                                       GPT2_QK_K, 0U, input, row, sizeof(row), &dot));
                TEST_ASSERT_EQUAL(0, (int)dot);
                TEST_ASSERT_EQUAL(-9, gpt2_gguf_project_qkv_row_fat16(&volume, "gpt2.ggu", &model, &qkv,
                                                                        GPT2_QK_K, 3U * GPT2_QK_K, input, row, sizeof(row), &dot));
                qkv.shape[1] = 2U * GPT2_QK_K;
                TEST_ASSERT_EQUAL(-9, gpt2_gguf_project_qkv_row_fat16(&volume, "gpt2.ggu", &model, &qkv,
                                                                        GPT2_QK_K, 0U, input, row, sizeof(row), &dot));
                {
                    float query[GPT2_QK_K] = {0.0f};
                    float key[GPT2_QK_K] = {0.0f};
                    float value[GPT2_QK_K] = {0.0f};
                    TEST_ASSERT_EQUAL(-6, gpt2_gguf_project_qkv_fat16(&volume, "gpt2.ggu", &model, &qkv,
                                                                       GPT2_QK_K, input, row, sizeof(row),
                                                                       query, sizeof(query) - 1U, key, sizeof(key), value, sizeof(value)));
                    TEST_ASSERT_EQUAL(-6, gpt2_gguf_project_qkv_fat16(&volume, "gpt2.ggu", &model, &qkv,
                                                                       GPT2_QK_K, input, row, sizeof(row),
                                                                       query, sizeof(query), key, sizeof(key), 0, sizeof(value)));
                }
            }
            }
        }
        {
            float input[GPT2_QK_K] = {0.0f};
            float dot = 1.0f;
            TEST_ASSERT_EQUAL(0, gpt2_gguf_dot_quant_block_fat16(&volume, "gpt2.ggu", &model, &tensor, 0U, input, GPT2_QK_K, block, sizeof(block), &dot));
            TEST_ASSERT_EQUAL(0, (int)dot);
            TEST_ASSERT_EQUAL(-7, gpt2_gguf_dot_quant_block_fat16(&volume, "gpt2.ggu", &model, &tensor, 0U, input, 32U, block, sizeof(block), &dot));
            {
                float row[GPT2_QK_K] = {0.0f};
                float tensor_input[GPT2_QK_K * 2U] = {0.0f};
                TEST_ASSERT_EQUAL(0, gpt2_gguf_dot_quant_tensor_fat16(&volume, "gpt2.ggu", &model, &tensor, tensor_input, GPT2_QK_K * 2U, block, sizeof(block), &dot));
                TEST_ASSERT_EQUAL(0, (int)dot);
                TEST_ASSERT_EQUAL(0, gpt2_gguf_dot_quant_row_fat16(&volume, "gpt2.ggu", &model, &tensor, 0U, row, GPT2_QK_K, block, sizeof(block), &dot));
                TEST_ASSERT_EQUAL(0, (int)dot);
                TEST_ASSERT_EQUAL(0, gpt2_gguf_dot_quant_row_fat16(&volume, "gpt2.ggu", &model, &tensor, 1U, row, GPT2_QK_K, block, sizeof(block), &dot));
                TEST_ASSERT_EQUAL(0, (int)dot);
                TEST_ASSERT_EQUAL(-9, gpt2_gguf_dot_quant_row_fat16(&volume, "gpt2.ggu", &model, &tensor, 2U, row, GPT2_QK_K, block, sizeof(block), &dot));
            }
        }
    }
}

static void put_tensor_at(uint32_t* cursor, const char* name, uint64_t width,
                          uint64_t rows, uint32_t type, uint64_t data_offset) {
    put_text_at(cursor, name);
    put32(*cursor, 2U); *cursor += 4U;
    put64_at(*cursor, width); *cursor += 8U;
    put64_at(*cursor, rows); *cursor += 8U;
    put32(*cursor, type); *cursor += 4U;
    put64_at(*cursor, data_offset); *cursor += 8U;
}

static void put_vector_at(uint32_t* cursor, const char* name, uint64_t width,
                          uint32_t type, uint64_t data_offset) {
    put_text_at(cursor, name);
    put32(*cursor, 1U); *cursor += 4U;
    put64_at(*cursor, width); *cursor += 8U;
    put32(*cursor, type); *cursor += 4U;
    put64_at(*cursor, data_offset); *cursor += 8U;
}

static void make_block_gguf_file(void) {
    uint32_t root = (1U + 2U * 17U) * 512U;
    uint32_t data = (root / 512U + 2U) * 512U;
    uint32_t file_start = data + 512U;
    uint32_t cursor = file_start;
    uint32_t tensor_data;
    uint32_t file_size;
    uint32_t clusters;
    uint32_t cluster;
    uint32_t fat;
    uint32_t total_tensor_bytes = BLOCK_GLOBAL_BYTES + 4U * BLOCK_NORM_BYTES +
                                  3U * BLOCK_CHANNELS * 4U + BLOCK_QKV_BYTES +
                                  BLOCK_ATTN_BYTES + BLOCK_UP_BYTES + BLOCK_DOWN_BYTES +
                                  BLOCK_FFN_UP_BIAS_BYTES + BLOCK_FFN_DOWN_BIAS_BYTES;
    make_volume();
    put32(cursor, GPT2_GGUF_MAGIC); cursor += 4U;
    put32(cursor, GPT2_GGUF_VERSION); cursor += 4U;
    put64_at(cursor, 17U); cursor += 8U;
    put64_at(cursor, 2U); cursor += 8U;
    put_text_at(&cursor, "general.architecture");
    put32(cursor, GPT2_GGUF_VALUE_STRING); cursor += 4U;
    put_text_at(&cursor, "gpt2");
    put_text_at(&cursor, "general.alignment");
    put32(cursor, GPT2_GGUF_VALUE_UINT32); cursor += 4U;
    put32(cursor, 32U); cursor += 4U;
    put_tensor_at(&cursor, "token_embd.weight", BLOCK_CHANNELS, 4U,
                  GPT2_GGUF_TENSOR_F32, 0U);
    put_tensor_at(&cursor, "position_embd.weight", BLOCK_CHANNELS, 2U,
                  GPT2_GGUF_TENSOR_F16, BLOCK_TOKEN_BYTES);
    put_vector_at(&cursor, "output_norm.weight", BLOCK_CHANNELS,
                  GPT2_GGUF_TENSOR_F32, BLOCK_TOKEN_BYTES + BLOCK_POSITION_BYTES);
    put_vector_at(&cursor, "output_norm.bias", BLOCK_CHANNELS,
                  GPT2_GGUF_TENSOR_F32, BLOCK_TOKEN_BYTES + BLOCK_POSITION_BYTES + BLOCK_NORM_BYTES);
    put_tensor_at(&cursor, "output.weight", BLOCK_CHANNELS, 4U,
                  GPT2_GGUF_TENSOR_Q4_K, BLOCK_TOKEN_BYTES + BLOCK_POSITION_BYTES + 2U * BLOCK_NORM_BYTES);
    put_vector_at(&cursor, "blk.0.attn_norm.weight", BLOCK_CHANNELS,
                  GPT2_GGUF_TENSOR_F32, BLOCK_GLOBAL_BYTES);
    put_vector_at(&cursor, "blk.0.attn_norm.bias", BLOCK_CHANNELS,
                  GPT2_GGUF_TENSOR_F32, BLOCK_GLOBAL_BYTES + BLOCK_NORM_BYTES);
    put_tensor_at(&cursor, "blk.0.attn_qkv.weight", BLOCK_CHANNELS,
                  3U * BLOCK_CHANNELS, GPT2_GGUF_TENSOR_Q4_K, BLOCK_GLOBAL_BYTES + 2U * BLOCK_NORM_BYTES);
    put_vector_at(&cursor, "blk.0.attn_qkv.bias", 3U * BLOCK_CHANNELS,
                  GPT2_GGUF_TENSOR_F32, BLOCK_GLOBAL_BYTES + 2U * BLOCK_NORM_BYTES + BLOCK_QKV_BYTES);
    put_tensor_at(&cursor, "blk.0.attn_output.weight", BLOCK_CHANNELS,
                  BLOCK_CHANNELS, GPT2_GGUF_TENSOR_Q4_K, BLOCK_GLOBAL_BYTES + 2U * BLOCK_NORM_BYTES + BLOCK_QKV_BYTES + 3U * BLOCK_CHANNELS * 4U);
    put_vector_at(&cursor, "blk.0.attn_output.bias", BLOCK_CHANNELS,
                  GPT2_GGUF_TENSOR_F32, BLOCK_GLOBAL_BYTES + 2U * BLOCK_NORM_BYTES + BLOCK_QKV_BYTES + 3U * BLOCK_CHANNELS * 4U + BLOCK_ATTN_BYTES);
    put_vector_at(&cursor, "blk.0.ffn_norm.weight", BLOCK_CHANNELS,
                  GPT2_GGUF_TENSOR_F32, BLOCK_GLOBAL_BYTES + 2U * BLOCK_NORM_BYTES + BLOCK_QKV_BYTES + 3U * BLOCK_CHANNELS * 4U + BLOCK_ATTN_BYTES + BLOCK_NORM_BYTES);
    put_vector_at(&cursor, "blk.0.ffn_norm.bias", BLOCK_CHANNELS,
                  GPT2_GGUF_TENSOR_F32, BLOCK_GLOBAL_BYTES + 3U * BLOCK_NORM_BYTES + BLOCK_QKV_BYTES + 3U * BLOCK_CHANNELS * 4U + BLOCK_ATTN_BYTES);
    put_tensor_at(&cursor, "blk.0.ffn_up.weight", BLOCK_CHANNELS,
                  BLOCK_HIDDEN, GPT2_GGUF_TENSOR_Q4_K,
                  BLOCK_GLOBAL_BYTES + 4U * BLOCK_NORM_BYTES + BLOCK_QKV_BYTES + 3U * BLOCK_CHANNELS * 4U + BLOCK_ATTN_BYTES);
    put_tensor_at(&cursor, "blk.0.ffn_down.weight", BLOCK_HIDDEN,
                  BLOCK_CHANNELS, GPT2_GGUF_TENSOR_Q4_K,
                  BLOCK_GLOBAL_BYTES + 4U * BLOCK_NORM_BYTES + BLOCK_QKV_BYTES + 3U * BLOCK_CHANNELS * 4U + BLOCK_ATTN_BYTES + BLOCK_UP_BYTES);
    put_vector_at(&cursor, "blk.0.ffn_up.bias", BLOCK_HIDDEN,
                  GPT2_GGUF_TENSOR_F32,
                  BLOCK_GLOBAL_BYTES + 4U * BLOCK_NORM_BYTES + BLOCK_QKV_BYTES + 3U * BLOCK_CHANNELS * 4U + BLOCK_ATTN_BYTES + BLOCK_UP_BYTES + BLOCK_DOWN_BYTES);
    put_vector_at(&cursor, "blk.0.ffn_down.bias", BLOCK_CHANNELS,
                  GPT2_GGUF_TENSOR_F32,
                  BLOCK_GLOBAL_BYTES + 4U * BLOCK_NORM_BYTES + BLOCK_QKV_BYTES + 3U * BLOCK_CHANNELS * 4U + BLOCK_ATTN_BYTES + BLOCK_UP_BYTES + BLOCK_DOWN_BYTES + BLOCK_FFN_UP_BIAS_BYTES);
    tensor_data = (cursor + 31U) & ~31U;
    file_size = tensor_data - file_start + total_tensor_bytes;
    clusters = (file_size + 511U) / 512U;
    for (fat = 1U; fat <= 2U; fat++) {
        uint32_t base = (1U + (fat - 1U) * 17U) * 512U;
        for (cluster = 3U; cluster < 3U + clusters; cluster++)
            put16(base + cluster * 2U, cluster + 1U < 3U + clusters ? cluster + 1U : 0xFFFFU);
    }
    disk[root + 32U] = 'B'; disk[root + 33U] = 'L'; disk[root + 34U] = 'O';
    disk[root + 35U] = 'C'; disk[root + 36U] = 'K'; disk[root + 37U] = ' ';
    disk[root + 38U] = ' '; disk[root + 39U] = ' ';
    disk[root + 40U] = 'G'; disk[root + 41U] = 'G'; disk[root + 42U] = 'U';
    disk[root + 43U] = 0x20U;
    put16(root + 32U + 26U, 3U);
    put32(root + 32U + 28U, file_size);
}

static void test_executes_q4_block_from_fat16(void) {
    fat16_volume_t volume;
    gpt2_gguf_loaded_model_t model;
    gpt2_gguf_tensor_t qkv, attention_output, ffn_up, ffn_down;
    float cache_storage[2U * 2U * BLOCK_CHANNELS] = {0.0f};
    float residual[BLOCK_CHANNELS] = {0.0f};
    float gamma[BLOCK_CHANNELS];
    float beta[BLOCK_CHANNELS] = {0.0f};
    float norm[BLOCK_CHANNELS] = {0.0f};
    float query[BLOCK_CHANNELS] = {0.0f};
    float key[BLOCK_CHANNELS] = {0.0f};
    float value[BLOCK_CHANNELS] = {0.0f};
    float head_outputs[BLOCK_CHANNELS] = {0.0f};
    float key_scratch[BLOCK_CHANNELS] = {0.0f};
    float scores[2U] = {0.0f};
    float attention[BLOCK_CHANNELS] = {0.0f};
    float projected[BLOCK_CHANNELS] = {0.0f};
    float hidden[BLOCK_HIDDEN] = {0.0f};
    float mlp_output[BLOCK_CHANNELS] = {0.0f};
    uint8_t row[4U * GPT2_Q4_K_BLOCK_BYTES] = {0U};
    gpt2_gguf_kv_cache_t cache;
    gpt2_gguf_block_workspace_t workspace;
    uint32_t i;
    make_block_gguf_file();
    for (i = 0U; i < BLOCK_CHANNELS; i++) gamma[i] = 1.0f;
    TEST_ASSERT_EQUAL(0, fat16_mount(&volume, read_sector, 0U));
    TEST_ASSERT_EQUAL(0, gpt2_gguf_load_fat16(&volume, "block.ggu", block_model_buffer,
                                               sizeof(block_model_buffer), &model));
    TEST_ASSERT_EQUAL(0, gpt2_gguf_index_find(&model.index, "blk.0.attn_qkv.weight", &qkv));
    TEST_ASSERT_EQUAL(0, gpt2_gguf_index_find(&model.index, "blk.0.attn_output.weight", &attention_output));
    TEST_ASSERT_EQUAL(0, gpt2_gguf_index_find(&model.index, "blk.0.ffn_up.weight", &ffn_up));
    TEST_ASSERT_EQUAL(0, gpt2_gguf_index_find(&model.index, "blk.0.ffn_down.weight", &ffn_down));
    TEST_ASSERT_EQUAL(0, gpt2_gguf_kv_cache_init(cache_storage, 2U * 2U * BLOCK_CHANNELS,
                                                  1U, 2U, BLOCK_CHANNELS, &cache));
    workspace.norm = norm; workspace.norm_capacity = BLOCK_CHANNELS;
    workspace.query = query; workspace.query_capacity = BLOCK_CHANNELS;
    workspace.key = key; workspace.key_capacity = BLOCK_CHANNELS;
    workspace.value = value; workspace.value_capacity = BLOCK_CHANNELS;
    workspace.head_outputs = head_outputs; workspace.head_output_capacity = BLOCK_CHANNELS;
    workspace.key_scratch = key_scratch; workspace.key_scratch_capacity = BLOCK_CHANNELS;
    workspace.scores = scores; workspace.score_capacity = 2U;
    workspace.attention = attention; workspace.attention_capacity = BLOCK_CHANNELS;
    workspace.projected = projected; workspace.projected_capacity = BLOCK_CHANNELS;
    workspace.hidden = hidden; workspace.hidden_capacity = BLOCK_HIDDEN;
    workspace.mlp_output = mlp_output; workspace.mlp_output_capacity = BLOCK_CHANNELS;
    workspace.row_buffer = row; workspace.row_capacity = sizeof(row);
    TEST_ASSERT_EQUAL(0, gpt2_gguf_block_forward_fat16(
        &cache, 0U, 0U, residual, BLOCK_CHANNELS, BLOCK_CHANNELS, 1U, BLOCK_HIDDEN,
        gamma, beta, &qkv, 0, &attention_output, 0, gamma, beta, &ffn_up, &ffn_down,
        0, 0, 0.0001f, &volume, "block.ggu", &model, &workspace));
    TEST_ASSERT_EQUAL(1, (int)cache.count);
    TEST_ASSERT_EQUAL(0, (int)residual[0]);
    TEST_ASSERT_EQUAL(0, gpt2_gguf_block_forward_fat16(
        &cache, 0U, 1U, residual, BLOCK_CHANNELS, BLOCK_CHANNELS, 1U, BLOCK_HIDDEN,
        gamma, beta, &qkv, 0, &attention_output, 0, gamma, beta, &ffn_up, &ffn_down,
        0, 0, 0.0001f, &volume, "block.ggu", &model, &workspace));
    TEST_ASSERT_EQUAL(2, (int)cache.count);
    workspace.score_capacity = 1U;
    TEST_ASSERT_EQUAL(-6, gpt2_gguf_block_forward_fat16(
        &cache, 0U, 1U, residual, BLOCK_CHANNELS, BLOCK_CHANNELS, 1U, BLOCK_HIDDEN,
        gamma, beta, &qkv, 0, &attention_output, 0, gamma, beta, &ffn_up, &ffn_down,
        0, 0, 0.0001f, &volume, "block.ggu", &model, &workspace));
}

static void test_generates_gguf_token_logits_from_fat16(void) {
    fat16_volume_t volume;
    gpt2_gguf_loaded_model_t model;
    gpt2_gguf_layer_t layers[1];
    gpt2_gguf_generation_t generation;
    gpt2_gguf_generation_workspace_t workspace;
    gpt2_gguf_kv_cache_t cache;
    float cache_storage[2U * 2U * BLOCK_CHANNELS] = {0.0f};
    float position[BLOCK_CHANNELS] = {0.0f}, ag[BLOCK_CHANNELS] = {0.0f}, ab[BLOCK_CHANNELS] = {0.0f};
    float qkv_bias[3U * BLOCK_CHANNELS] = {0.0f}, aob[BLOCK_CHANNELS] = {0.0f};
    float fg[BLOCK_CHANNELS] = {0.0f}, fb[BLOCK_CHANNELS] = {0.0f};
    float fub[BLOCK_HIDDEN] = {0.0f}, fdb[BLOCK_CHANNELS] = {0.0f};
    float final_gamma[BLOCK_CHANNELS] = {0.0f}, final_beta[BLOCK_CHANNELS] = {0.0f};
    float hidden[BLOCK_CHANNELS] = {0.0f}, norm[BLOCK_CHANNELS] = {0.0f};
    float query[BLOCK_CHANNELS] = {0.0f}, key[BLOCK_CHANNELS] = {0.0f}, value[BLOCK_CHANNELS] = {0.0f};
    float heads[BLOCK_CHANNELS] = {0.0f}, key_scratch[BLOCK_CHANNELS] = {0.0f}, scores[2U] = {0.0f};
    float attention[BLOCK_CHANNELS] = {0.0f}, projected[BLOCK_CHANNELS] = {0.0f};
    float mlp_hidden[BLOCK_HIDDEN] = {0.0f}, mlp_output[BLOCK_CHANNELS] = {0.0f};
    float logits[4U] = {0.0f};
    uint8_t dense_scratch[BLOCK_HIDDEN * 4U] = {0U};
    uint8_t row[4U * GPT2_Q4_K_BLOCK_BYTES] = {0U};
    char name[64];
    make_block_gguf_file();
    TEST_ASSERT_EQUAL(0, fat16_mount(&volume, read_sector, 0U));
    TEST_ASSERT_EQUAL(0, gpt2_gguf_load_fat16(&volume, "block.ggu", block_model_buffer,
                                               sizeof(block_model_buffer), &model));
    TEST_ASSERT_EQUAL(0, gpt2_gguf_generation_prepare(&model, name, sizeof(name), layers, 1U, &generation));
    TEST_ASSERT_EQUAL(0, gpt2_gguf_kv_cache_init(cache_storage, 2U * 2U * BLOCK_CHANNELS,
                                                  1U, 2U, BLOCK_CHANNELS, &cache));
    workspace.dense_scratch=dense_scratch; workspace.dense_scratch_capacity=sizeof(dense_scratch);
    workspace.position_embedding=position; workspace.position_embedding_capacity=BLOCK_CHANNELS;
    workspace.attention_gamma=ag; workspace.attention_gamma_capacity=BLOCK_CHANNELS;
    workspace.attention_beta=ab; workspace.attention_beta_capacity=BLOCK_CHANNELS;
    workspace.qkv_bias=qkv_bias; workspace.qkv_bias_capacity=3U*BLOCK_CHANNELS;
    workspace.attention_output_bias=aob; workspace.attention_output_bias_capacity=BLOCK_CHANNELS;
    workspace.ffn_gamma=fg; workspace.ffn_gamma_capacity=BLOCK_CHANNELS;
    workspace.ffn_beta=fb; workspace.ffn_beta_capacity=BLOCK_CHANNELS;
    workspace.ffn_up_bias=fub; workspace.ffn_up_bias_capacity=BLOCK_HIDDEN;
    workspace.ffn_down_bias=fdb; workspace.ffn_down_bias_capacity=BLOCK_CHANNELS;
    workspace.final_gamma=final_gamma; workspace.final_gamma_capacity=BLOCK_CHANNELS;
    workspace.final_beta=final_beta; workspace.final_beta_capacity=BLOCK_CHANNELS;
    workspace.final_hidden=hidden; workspace.final_hidden_capacity=BLOCK_CHANNELS;
    workspace.block.norm=norm; workspace.block.norm_capacity=BLOCK_CHANNELS;
    workspace.block.query=query; workspace.block.query_capacity=BLOCK_CHANNELS;
    workspace.block.key=key; workspace.block.key_capacity=BLOCK_CHANNELS;
    workspace.block.value=value; workspace.block.value_capacity=BLOCK_CHANNELS;
    workspace.block.head_outputs=heads; workspace.block.head_output_capacity=BLOCK_CHANNELS;
    workspace.block.key_scratch=key_scratch; workspace.block.key_scratch_capacity=BLOCK_CHANNELS;
    workspace.block.scores=scores; workspace.block.score_capacity=2U;
    workspace.block.attention=attention; workspace.block.attention_capacity=BLOCK_CHANNELS;
    workspace.block.projected=projected; workspace.block.projected_capacity=BLOCK_CHANNELS;
    workspace.block.hidden=mlp_hidden; workspace.block.hidden_capacity=BLOCK_HIDDEN;
    workspace.block.mlp_output=mlp_output; workspace.block.mlp_output_capacity=BLOCK_CHANNELS;
    workspace.block.row_buffer=row; workspace.block.row_capacity=sizeof(row);
    TEST_ASSERT_EQUAL(0, gpt2_gguf_generation_token_fat16(&generation, &cache, 0U, 0U, 1U,
                                                           0.0001f, &volume, "block.ggu", &workspace,
                                                           logits, sizeof(logits)));
    TEST_ASSERT_EQUAL(1, (int)cache.count);
    TEST_ASSERT_EQUAL(0, (int)logits[0]);
    TEST_ASSERT_EQUAL(0, gpt2_gguf_generation_token_fat16(&generation, &cache, 1U, 1U, 1U,
                                                           0.0001f, &volume, "block.ggu", &workspace,
                                                           logits, sizeof(logits)));
    TEST_ASSERT_EQUAL(2, (int)cache.count);
    TEST_ASSERT_EQUAL(-9, gpt2_gguf_generation_token_fat16(&generation, &cache, 4U, 1U, 1U,
                                                            0.0001f, &volume, "block.ggu", &workspace,
                                                            logits, sizeof(logits)));
}

static void test_streams_output_top_k_equivalently_from_fat16(void) {
    fat16_volume_t volume;
    gpt2_gguf_loaded_model_t model;
    gpt2_gguf_layer_t layers[1];
    gpt2_gguf_generation_t generation;
    float hidden[BLOCK_CHANNELS] = {0.0f};
    float logits[4U] = {0.0f};
    uint8_t row[4U * GPT2_Q4_K_BLOCK_BYTES] = {0U};
    uint32_t generated[1] = {1U};
    uint32_t rng_logits = 0x12345678U;
    uint32_t rng_streamed = 0x12345678U;
    uint32_t token_logits;
    uint32_t token_streamed;
    gpt2_sample_top_k_state_t top_k;
    char name[64];
    make_block_gguf_file();
    TEST_ASSERT_EQUAL(0, fat16_mount(&volume, read_sector, 0U));
    TEST_ASSERT_EQUAL(0, gpt2_gguf_load_fat16(&volume, "block.ggu", block_model_buffer,
                                               sizeof(block_model_buffer), &model));
    TEST_ASSERT_EQUAL(0, gpt2_gguf_generation_prepare(&model, name, sizeof(name), layers, 1U, &generation));
    TEST_ASSERT_EQUAL(0, gpt2_gguf_forward_output_logits_fat16(
        &volume, "block.ggu", &model, &generation.output_weight, BLOCK_CHANNELS, 4U,
        hidden, row, sizeof(row), logits, sizeof(logits)));
    gpt2_sample_top_k_init(&top_k, generated, 1U);
    TEST_ASSERT_EQUAL(0, gpt2_gguf_forward_output_top_k_fat16(
        &volume, "block.ggu", &model, &generation.output_weight, BLOCK_CHANNELS, 4U,
        hidden, row, sizeof(row), &top_k));
    token_logits = gpt2_sample_top_k(logits, 4U, generated, 1U, &rng_logits);
    token_streamed = gpt2_sample_top_k_finish(&top_k, &rng_streamed);
    TEST_ASSERT_EQUAL((int)token_logits, (int)token_streamed);
    TEST_ASSERT_EQUAL((int)rng_logits, (int)rng_streamed);
}

static void test_mount_list_and_read(void) {
    fat16_volume_t volume;
    os_fat16_dirent_t entries[4];
    char content[8] = {0};
    make_volume();
    TEST_ASSERT_EQUAL(0, fat16_mount(&volume, read_sector, 0U));
    TEST_ASSERT_TRUE(fat16_is_mounted(&volume));
    TEST_ASSERT_EQUAL(1, fat16_list_root(&volume, entries, 4U));
    TEST_ASSERT_EQUAL_STRING("FATOK.TXT", entries[0].name);
    TEST_ASSERT_EQUAL(5, (int)entries[0].size);
    TEST_ASSERT_EQUAL(5, fat16_read_file(&volume, "fatok.txt", content, sizeof(content)));
    TEST_ASSERT_EQUAL_STRING("hello", content);
}

static void test_lists_root_page_after_first_entry(void) {
    fat16_volume_t volume;
    os_fat16_dirent_t entries[2];
    uint8_t data[1] = {'2'};
    uint16_t first = 0U;
    make_volume();
    TEST_ASSERT_EQUAL(0, fat16_mount(&volume, read_sector, 0U));
    TEST_ASSERT_EQUAL(0, fat16_attach_writer(&volume, write_sector));
    TEST_ASSERT_EQUAL(0, fat16_create_file(&volume, "SECOND.TXT", 0x20U,
                                           data, sizeof(data), &first));
    TEST_ASSERT_EQUAL(1, fat16_list_root_page(&volume, 1U, entries, 2U));
    TEST_ASSERT_EQUAL_STRING("SECOND.TXT", entries[0].name);
    TEST_ASSERT_EQUAL(0, fat16_list_root_page(&volume, 2U, entries, 2U));
}

static void test_reads_bounded_file_range(void) {
    fat16_volume_t volume;
    uint8_t content[4] = {0U, 0U, 0U, 0U};
    uint32_t read = 0U;
    make_volume();
    TEST_ASSERT_EQUAL(0, fat16_mount(&volume, read_sector, 0U));
    TEST_ASSERT_EQUAL(0, fat16_read_file_range(&volume, "fatok.txt", 1U, content, 3U, &read));
    TEST_ASSERT_EQUAL(3, (int)read);
    TEST_ASSERT_EQUAL('e', content[0]);
    TEST_ASSERT_EQUAL('l', content[1]);
    TEST_ASSERT_EQUAL('l', content[2]);
    TEST_ASSERT_EQUAL(OS_FAT16_BAD_PATH, fat16_read_file_range(&volume, "fatok.txt", 6U, content, 1U, &read));
    TEST_ASSERT_EQUAL(0, (int)read);
}

static void test_cursor_reads_successive_windows(void) {
    fat16_volume_t volume;
    fat16_file_t file;
    uint8_t first[3] = {0U, 0U, 0U};
    uint8_t second[3] = {0U, 0U, 0U};
    uint32_t read = 0U;
    make_volume();
    TEST_ASSERT_EQUAL(0, fat16_mount(&volume, read_sector, 0U));
    TEST_ASSERT_EQUAL(0, fat16_open_file(&volume, "fatok.txt", &file));
    TEST_ASSERT_EQUAL(0, fat16_file_read(&file, first, sizeof(first), &read));
    TEST_ASSERT_EQUAL(3, (int)read);
    TEST_ASSERT_EQUAL('h', first[0]);
    TEST_ASSERT_EQUAL('e', first[1]);
    TEST_ASSERT_EQUAL('l', first[2]);
    TEST_ASSERT_EQUAL(0, fat16_file_read(&file, second, sizeof(second), &read));
    TEST_ASSERT_EQUAL(2, (int)read);
    TEST_ASSERT_EQUAL('l', second[0]);
    TEST_ASSERT_EQUAL('o', second[1]);
    TEST_ASSERT_EQUAL(0, fat16_file_read(&file, second, sizeof(second), &read));
    TEST_ASSERT_EQUAL(0, (int)read);
    TEST_ASSERT_EQUAL(0, fat16_file_seek(&file, 2U));
    TEST_ASSERT_EQUAL(0, fat16_file_read(&file, second, sizeof(second), &read));
    TEST_ASSERT_EQUAL(3, (int)read);
    TEST_ASSERT_EQUAL('l', second[0]);
    TEST_ASSERT_EQUAL('l', second[1]);
    TEST_ASSERT_EQUAL('o', second[2]);
    TEST_ASSERT_EQUAL(OS_FAT16_BAD_PATH, fat16_file_seek(&file, 6U));
}

static void test_cursor_caches_shared_sector(void) {
    fat16_volume_t volume;
    fat16_file_t file;
    uint8_t buffer[3] = {0U, 0U, 0U};
    uint32_t read = 0U;
    uint32_t after_first;
    make_volume();
    TEST_ASSERT_EQUAL(0, fat16_mount(&volume, read_sector, 0U));
    TEST_ASSERT_EQUAL(0, fat16_open_file(&volume, "fatok.txt", &file));
    TEST_ASSERT_EQUAL(0, fat16_file_read(&file, buffer, 3U, &read));
    TEST_ASSERT_EQUAL(3, (int)read);
    after_first = read_sector_calls;
    TEST_ASSERT_EQUAL(0, fat16_file_read(&file, buffer, 2U, &read));
    TEST_ASSERT_EQUAL(2, (int)read);
    TEST_ASSERT_EQUAL((int)after_first, (int)read_sector_calls);
}

static void test_rejects_bad_bpb(void) {
    fat16_volume_t volume;
    make_volume();
    put16(11U, 256U);
    TEST_ASSERT_EQUAL(OS_FAT16_CORRUPT, fat16_mount(&volume, read_sector, 0U));
    TEST_ASSERT_FALSE(fat16_is_mounted(&volume));
}

static void test_writes_only_with_explicit_writer(void) {
    fat16_volume_t volume;
    uint8_t buffer[512] = {0};
    uint8_t text[3] = {'A','I','!'};
    uint16_t allocated = 0U;
    make_volume();
    TEST_ASSERT_EQUAL(0, fat16_mount(&volume, read_sector, 0U));
    TEST_ASSERT_EQUAL(OS_FAT16_NOT_MOUNTED, fat16_write_sector(&volume, 2U, buffer));
    TEST_ASSERT_EQUAL(0, fat16_attach_writer(&volume, write_sector));
    buffer[0] = 0xa5U;
    TEST_ASSERT_EQUAL(0, fat16_write_sector(&volume, 2U, buffer));
    TEST_ASSERT_EQUAL(0xa5U, disk[2U * 512U]);
    TEST_ASSERT_EQUAL(0, fat16_write_cluster_range(&volume, 2U, 7U, text, 3U));
    TEST_ASSERT_EQUAL('A', disk[37U * 512U + 7U]);
    TEST_ASSERT_EQUAL('I', disk[37U * 512U + 8U]);
    TEST_ASSERT_EQUAL('!', disk[37U * 512U + 9U]);
    TEST_ASSERT_EQUAL(OS_FAT16_BUFFER_SMALL, fat16_write_cluster_range(&volume, 2U, 512U, text, 1U));
    TEST_ASSERT_EQUAL(0, fat16_allocate_cluster(&volume, &allocated));
    TEST_ASSERT_EQUAL(3U, allocated);
    TEST_ASSERT_EQUAL(0xF8U, disk[1U * 512U + 6U]);
    TEST_ASSERT_EQUAL(0xFFU, disk[1U * 512U + 7U]);
    TEST_ASSERT_EQUAL(0xF8U, disk[18U * 512U + 6U]);
    TEST_ASSERT_EQUAL(0xFFU, disk[18U * 512U + 7U]);
    TEST_ASSERT_EQUAL(0, fat16_link_clusters(&volume, 2U, 3U));
    TEST_ASSERT_EQUAL(3U, disk[1U * 512U + 4U]);
    TEST_ASSERT_EQUAL(0U, disk[1U * 512U + 5U]);
    TEST_ASSERT_EQUAL(3U, disk[18U * 512U + 4U]);
    TEST_ASSERT_EQUAL(0U, disk[18U * 512U + 5U]);
    TEST_ASSERT_EQUAL(OS_FAT16_CORRUPT, fat16_link_clusters(&volume, 2U, 2U));
    TEST_ASSERT_EQUAL(0, fat16_create_root_entry(&volume, "NOTE.TXT", 0x20U, 3U, 3U));
    TEST_ASSERT_EQUAL('N', disk[35U * 512U + 32U]);
    TEST_ASSERT_EQUAL('O', disk[35U * 512U + 32U + 1U]);
    TEST_ASSERT_EQUAL('T', disk[35U * 512U + 32U + 2U]);
    TEST_ASSERT_EQUAL('T', disk[35U * 512U + 32U + 8U]);
    TEST_ASSERT_EQUAL('T', disk[35U * 512U + 32U + 10U]);
    TEST_ASSERT_EQUAL(0x20U, disk[35U * 512U + 32U + 11U]);
    TEST_ASSERT_EQUAL(3U, disk[35U * 512U + 32U + 26U]);
    TEST_ASSERT_EQUAL(3U, disk[35U * 512U + 32U + 28U]);
    TEST_ASSERT_EQUAL(OS_FAT16_BAD_PATH, fat16_create_root_entry(&volume, "TOOLONG12.TXT", 0x20U, 3U, 3U));
    TEST_ASSERT_EQUAL(OS_FAT16_CORRUPT, fat16_write_sector(&volume, TEST_SECTORS, buffer));
}
static void test_creates_persistent_file(void) {
    fat16_volume_t volume;
    uint8_t data[600];
    uint8_t readback[600];
    uint16_t first = 0U;
    uint32_t i;
    make_volume();
    for (i = 0U; i < sizeof(data); i++) data[i] = (uint8_t)(i ^ 0x5AU);
    TEST_ASSERT_EQUAL(0, fat16_mount(&volume, read_sector, 0U));
    TEST_ASSERT_EQUAL(0, fat16_attach_writer(&volume, write_sector));
    TEST_ASSERT_EQUAL(0, fat16_create_file(&volume, "PERSIST.BIN", 0x20U,
                                           data, sizeof(data), &first));
    TEST_ASSERT_EQUAL(3U, first);
    TEST_ASSERT_EQUAL(4U, le16(1U * 512U + 6U));
    TEST_ASSERT_EQUAL(0xFFF8U, le16(1U * 512U + 8U));
    TEST_ASSERT_EQUAL(4U, le16(18U * 512U + 6U));
    TEST_ASSERT_EQUAL(0xFFF8U, le16(18U * 512U + 8U));
    TEST_ASSERT_EQUAL(600, fat16_read_file(&volume, "persist.bin", (char*)readback, sizeof(readback)));
    for (i = 0U; i < sizeof(data); i++) TEST_ASSERT_EQUAL(data[i], readback[i]);
    TEST_ASSERT_EQUAL(OS_FAT16_BAD_PATH, fat16_create_file(&volume, "PERSIST.BIN", 0x20U,
                                                            data, sizeof(data), &first));
}
static void test_unlinks_persistent_file_and_reuses_cluster(void) {
    fat16_volume_t volume;
    os_fat16_dirent_t entries[4];
    uint8_t data[600];
    uint8_t next_data[1] = {'R'};
    uint8_t trail_data[1] = {'T'};
    char readback[4] = {0};
    uint16_t first = 0U;
    uint32_t i;
    make_volume();
    for (i = 0U; i < sizeof(data); i++) data[i] = (uint8_t)(i ^ 0x5AU);
    TEST_ASSERT_EQUAL(0, fat16_mount(&volume, read_sector, 0U));
    TEST_ASSERT_EQUAL(0, fat16_attach_writer(&volume, write_sector));
    TEST_ASSERT_EQUAL(0, fat16_create_file(&volume, "PERSIST.BIN", 0x20U,
                                           data, sizeof(data), &first));
    TEST_ASSERT_EQUAL(3U, first);
    TEST_ASSERT_EQUAL(0, fat16_create_file(&volume, "TRAIL.BIN", 0x20U,
                                           trail_data, sizeof(trail_data), &first));
    TEST_ASSERT_EQUAL(5U, first);
    TEST_ASSERT_EQUAL(0, fat16_unlink_file(&volume, "persist.bin"));
    TEST_ASSERT_EQUAL(0xE5U, disk[35U * 512U + 32U]);
    TEST_ASSERT_EQUAL(0U, le16(1U * 512U + 6U));
    TEST_ASSERT_EQUAL(0U, le16(1U * 512U + 8U));
    TEST_ASSERT_EQUAL(0U, le16(18U * 512U + 6U));
    TEST_ASSERT_EQUAL(0U, le16(18U * 512U + 8U));
    TEST_ASSERT_EQUAL(OS_FAT16_NOT_FOUND,
                      fat16_read_file(&volume, "PERSIST.BIN", readback, sizeof(readback)));
    TEST_ASSERT_EQUAL(OS_FAT16_BAD_PATH, fat16_create_file(&volume, "TRAIL.BIN", 0x20U,
                                                            trail_data, sizeof(trail_data), &first));
    TEST_ASSERT_EQUAL('T', disk[35U * 512U + 64U]);
    TEST_ASSERT_EQUAL(0U, le16(1U * 512U + 6U));
    TEST_ASSERT_EQUAL(0U, le16(18U * 512U + 6U));
    TEST_ASSERT_EQUAL(2, fat16_list_root(&volume, entries, 4U));
    TEST_ASSERT_EQUAL_STRING("FATOK.TXT", entries[0].name);
    TEST_ASSERT_EQUAL_STRING("TRAIL.BIN", entries[1].name);
    TEST_ASSERT_EQUAL(0, fat16_create_file(&volume, "REUSE.BIN", 0x20U,
                                           next_data, sizeof(next_data), &first));
    TEST_ASSERT_EQUAL(3U, first);
}

static void test_renames_persistent_file_without_moving_chain(void) {
    fat16_volume_t volume;
    uint8_t data[600];
    uint8_t target[1] = {'T'};
    uint8_t readback[600];
    uint16_t first = 0U;
    uint32_t i;
    make_volume();
    for (i = 0U; i < sizeof(data); i++) data[i] = (uint8_t)(i ^ 0x3CU);
    TEST_ASSERT_EQUAL(0, fat16_mount(&volume, read_sector, 0U));
    TEST_ASSERT_EQUAL(0, fat16_attach_writer(&volume, write_sector));
    TEST_ASSERT_EQUAL(0, fat16_create_file(&volume, "SOURCE.BIN", 0x20U,
                                           data, sizeof(data), &first));
    TEST_ASSERT_EQUAL(3U, first);
    TEST_ASSERT_EQUAL(0, fat16_create_file(&volume, "TARGET.BIN", 0x20U,
                                           target, sizeof(target), &first));
    TEST_ASSERT_EQUAL(5U, first);
    TEST_ASSERT_EQUAL(0, fat16_rename_file(&volume, "source.bin", "renamed.bin"));
    TEST_ASSERT_EQUAL('R', disk[35U * 512U + 32U]);
    TEST_ASSERT_EQUAL(4U, le16(1U * 512U + 6U));
    TEST_ASSERT_EQUAL(0xFFF8U, le16(1U * 512U + 8U));
    TEST_ASSERT_EQUAL(4U, le16(18U * 512U + 6U));
    TEST_ASSERT_EQUAL(0xFFF8U, le16(18U * 512U + 8U));
    TEST_ASSERT_EQUAL(600, fat16_read_file(&volume, "RENAMED.BIN", (char*)readback, sizeof(readback)));
    for (i = 0U; i < sizeof(data); i++) TEST_ASSERT_EQUAL(data[i], readback[i]);
    TEST_ASSERT_EQUAL(OS_FAT16_NOT_FOUND,
                      fat16_read_file(&volume, "SOURCE.BIN", (char*)readback, sizeof(readback)));
    TEST_ASSERT_EQUAL(OS_FAT16_BAD_PATH,
                      fat16_rename_file(&volume, "RENAMED.BIN", "TARGET.BIN"));
    TEST_ASSERT_EQUAL(600, fat16_read_file(&volume, "RENAMED.BIN", (char*)readback, sizeof(readback)));
}

static void test_creates_lfn_file(void) {
    fat16_volume_t volume;
    uint8_t data[3] = {'L', 'F', 'N'};
    uint16_t first = 0U;
    os_fat16_dirent_t entries[4];
    char readback[4] = {0};
    uint8_t range[2] = {0};
    uint32_t read = 0U;
    fat16_file_t file;
    uint32_t root = 35U * 512U;
    make_volume();
    TEST_ASSERT_EQUAL(0, fat16_mount(&volume, read_sector, 0U));
    TEST_ASSERT_EQUAL(0, fat16_attach_writer(&volume, write_sector));
    TEST_ASSERT_EQUAL(0, fat16_create_lfn_file(&volume, "Session-2026-A", "SESS01.TXT",
                                               0x20U, data, sizeof(data), &first));
    TEST_ASSERT_EQUAL(3U, first);
    TEST_ASSERT_EQUAL(0xE5U, disk[root + 32U]);
    TEST_ASSERT_EQUAL(0x42U, disk[root + 64U]);
    TEST_ASSERT_EQUAL(0x0FU, disk[root + 64U + 11U]);
    TEST_ASSERT_EQUAL('A', disk[root + 64U + 1U]);
    TEST_ASSERT_EQUAL(0x01U, disk[root + 96U]);
    TEST_ASSERT_EQUAL('S', disk[root + 96U + 1U]);
    TEST_ASSERT_EQUAL('S', disk[root + 128U]);
    TEST_ASSERT_EQUAL('E', disk[root + 128U + 1U]);
    TEST_ASSERT_EQUAL('S', disk[root + 128U + 2U]);
    TEST_ASSERT_EQUAL('S', disk[root + 128U + 3U]);
    TEST_ASSERT_EQUAL('T', disk[root + 128U + 8U]);
    TEST_ASSERT_EQUAL(2, fat16_list_root(&volume, entries, 4U));
    TEST_ASSERT_EQUAL('S', entries[1].name[0]);
    TEST_ASSERT_EQUAL('e', entries[1].name[1]);
    TEST_ASSERT_EQUAL('s', entries[1].name[2]);
    TEST_ASSERT_EQUAL('s', entries[1].name[3]);
    TEST_ASSERT_EQUAL('i', entries[1].name[4]);
    TEST_ASSERT_EQUAL('o', entries[1].name[5]);
    TEST_ASSERT_EQUAL('n', entries[1].name[6]);
    TEST_ASSERT_EQUAL('-', entries[1].name[7]);
    TEST_ASSERT_EQUAL(3, fat16_read_file(&volume, "session-2026-a", readback, sizeof(readback)));
    TEST_ASSERT_EQUAL_STRING("LFN", readback);
    TEST_ASSERT_EQUAL(0, fat16_read_file_range(&volume, "SESSION-2026-A", 1U, range,
                                               sizeof(range), &read));
    TEST_ASSERT_EQUAL(2U, read);
    TEST_ASSERT_EQUAL('F', range[0]);
    TEST_ASSERT_EQUAL('N', range[1]);
    TEST_ASSERT_EQUAL(0, fat16_open_file(&volume, "Session-2026-A", &file));
    TEST_ASSERT_EQUAL(0, fat16_file_read(&file, range, sizeof(range), &read));
    TEST_ASSERT_EQUAL(2U, read);
    TEST_ASSERT_EQUAL('L', range[0]);
    TEST_ASSERT_EQUAL('F', range[1]);
    TEST_ASSERT_EQUAL(OS_FAT16_NOT_FOUND, fat16_open_file(&volume, "Unknown-long-name", &file));
    TEST_ASSERT_EQUAL('2', entries[1].name[8]);
    TEST_ASSERT_EQUAL('0', entries[1].name[9]);
    TEST_ASSERT_EQUAL('2', entries[1].name[10]);
    TEST_ASSERT_EQUAL('6', entries[1].name[11]);
    TEST_ASSERT_EQUAL('-', entries[1].name[12]);
    TEST_ASSERT_EQUAL('A', entries[1].name[13]);
    TEST_ASSERT_EQUAL('\0', entries[1].name[14]);
    TEST_ASSERT_EQUAL(3U, entries[1].size);
}
static void test_creates_utf8_lfn_file(void) {
    fat16_volume_t volume;
    uint8_t data[4] = {'U', 'T', 'F', '8'};
    char readback[4] = {0};
    os_fat16_dirent_t entries[4];
    uint16_t first = 0U;
    uint32_t root = 35U * 512U;
    make_volume();
    TEST_ASSERT_EQUAL(0, fat16_mount(&volume, read_sector, 0U));
    TEST_ASSERT_EQUAL(0, fat16_attach_writer(&volume, write_sector));
    TEST_ASSERT_EQUAL(0, fat16_create_lfn_file(&volume, "caf\xC3\xA9-2026.txt", "CAFE26.TXT",
                                               0x20U, data, sizeof(data), &first));
    TEST_ASSERT_EQUAL(3U, first);
    TEST_ASSERT_EQUAL(0xE9U, disk[root + 64U + 7U]);
    TEST_ASSERT_EQUAL(0x00U, disk[root + 64U + 8U]);
    TEST_ASSERT_EQUAL(4, fat16_read_file(&volume, "caf\xC3\xA9-2026.txt", readback, sizeof(readback)));
    TEST_ASSERT_EQUAL_MEMORY(data, readback, sizeof(data));
    TEST_ASSERT_EQUAL(2, fat16_list_root(&volume, entries, 4U));
    TEST_ASSERT_EQUAL_STRING("caf\xC3\xA9-2026.txt", entries[1].name);
}

static void test_creates_non_bmp_lfn_file(void) {
    fat16_volume_t volume;
    uint8_t data[5] = {'A', 'S', 'T', 'R', 'A'};
    char readback[5] = {0};
    os_fat16_dirent_t entries[4];
    uint16_t first = 0U;
    uint32_t root = 35U * 512U;
    make_volume();
    TEST_ASSERT_EQUAL(0, fat16_mount(&volume, read_sector, 0U));
    TEST_ASSERT_EQUAL(0, fat16_attach_writer(&volume, write_sector));
    TEST_ASSERT_EQUAL(0, fat16_create_lfn_file(&volume, "rocket-\xF0\x9F\x98\x80.txt", "ROCKT1.TXT",
                                               0x20U, data, sizeof(data), &first));
    TEST_ASSERT_EQUAL(3U, first);
    TEST_ASSERT_EQUAL(0x3DU, disk[root + 64U + 18U]);
    TEST_ASSERT_EQUAL(0xD8U, disk[root + 64U + 19U]);
    TEST_ASSERT_EQUAL(0x00U, disk[root + 64U + 20U]);
    TEST_ASSERT_EQUAL(0xDEU, disk[root + 64U + 21U]);
    TEST_ASSERT_EQUAL(5, fat16_read_file(&volume, "rocket-\xF0\x9F\x98\x80.txt", readback, sizeof(readback)));
    TEST_ASSERT_EQUAL_MEMORY(data, readback, sizeof(data));
    TEST_ASSERT_EQUAL(2, fat16_list_root(&volume, entries, 4U));
    TEST_ASSERT_EQUAL_STRING("rocket-\xF0\x9F\x98\x80.txt", entries[1].name);
}

static void test_cursor_uses_attached_multisector_window(void) {
    fat16_volume_t volume;
    fat16_file_t file;
    uint8_t window[32U * 512U];
    uint8_t out[9U * 512U];
    uint32_t root = (1U + 2U * 17U) * 512U;
    uint32_t data = (root / 512U + 2U) * 512U;
    uint32_t file_data = data + 8U * 8U * 512U;
    uint32_t read = 0U;
    uint32_t i;
    uint32_t fat;
    make_volume();
    disk[13] = 8U;
    for (fat = 1U; fat <= 2U; fat++) {
        uint32_t base = (1U + (fat - 1U) * 17U) * 512U;
        put16(base + 20U, 11U);
        put16(base + 22U, 0xFFF8U);
    }
    put16(root + 26U, 10U);
    put32(root + 28U, sizeof(out));
    for (i = 0U; i < sizeof(out); i++) disk[file_data + i] = (uint8_t)(i ^ 0xA5U);
    TEST_ASSERT_EQUAL(0, fat16_mount(&volume, read_sector, 0U));
    TEST_ASSERT_EQUAL(0, fat16_attach_read_window(&volume, read_sectors, window, sizeof(window)));
    TEST_ASSERT_EQUAL(0, fat16_open_file(&volume, "fatok.txt", &file));
    read_sectors_calls = 0U;
    TEST_ASSERT_EQUAL(0, fat16_file_read(&file, out, sizeof(out), &read));
    TEST_ASSERT_EQUAL(sizeof(out), read);
    TEST_ASSERT_EQUAL(1U, read_sectors_calls);
    for (i = 0U; i < sizeof(out); i++) TEST_ASSERT_EQUAL((uint8_t)(i ^ 0xA5U), out[i]);
}

static void test_reads_deep_multisector_cluster_without_false_corruption(void) {
    fat16_volume_t volume;
    fat16_file_t file;
    uint8_t range[3000];
    uint8_t cursor[3000];
    uint32_t root = (1U + 2U * 17U) * 512U;
    uint32_t data = (root / 512U + 2U) * 512U;
    uint32_t clusters = (TEST_SECTORS - (data / 512U)) / 8U;
    uint32_t offset = (clusters - 3U) * 8U * 512U + 100U;
    uint32_t read = 0U;
    uint32_t i;
    uint32_t fat;
    make_volume();
    disk[13] = 8U;
    for (fat = 1U; fat <= 2U; fat++) {
        uint32_t base = (1U + (fat - 1U) * 17U) * 512U;
        for (i = 2U; i < clusters + 1U; i++) put16(base + i * 2U, (uint16_t)(i + 1U));
        put16(base + clusters * 2U, 0xFFF8U);
    }
    disk[root + 32U] = 'D'; disk[root + 33U] = 'E'; disk[root + 34U] = 'E'; disk[root + 35U] = 'P';
    disk[root + 36U] = ' '; disk[root + 37U] = ' '; disk[root + 38U] = ' '; disk[root + 39U] = ' ';
    disk[root + 40U] = 'B'; disk[root + 41U] = 'I'; disk[root + 42U] = 'N'; disk[root + 43U] = 0x20U;
    put16(root + 32U + 26U, 2U);
    put32(root + 32U + 28U, clusters * 8U * 512U);
    for (i = 0U; i < sizeof(range); i++) disk[data + offset + i] = (uint8_t)(i ^ 0x5AU);
    TEST_ASSERT_EQUAL(0, fat16_mount(&volume, read_sector, 0U));
    TEST_ASSERT_EQUAL(0, fat16_read_file_range(&volume, "deep.bin", offset,
                                                range, sizeof(range), &read));
    TEST_ASSERT_EQUAL(sizeof(range), read);
    for (i = 0U; i < sizeof(range); i++) TEST_ASSERT_EQUAL((uint8_t)(i ^ 0x5AU), range[i]);
    TEST_ASSERT_EQUAL(0, fat16_open_file(&volume, "deep.bin", &file));
    TEST_ASSERT_EQUAL(0, fat16_file_seek(&file, offset));
    TEST_ASSERT_EQUAL(0, fat16_file_read(&file, cursor, sizeof(cursor), &read));
    TEST_ASSERT_EQUAL(sizeof(cursor), read);
    for (i = 0U; i < sizeof(cursor); i++) TEST_ASSERT_EQUAL((uint8_t)(i ^ 0x5AU), cursor[i]);
}

static void test_rejects_bad_name_and_small_buffer(void) {
    fat16_volume_t volume;
    char content[4];
    make_volume();
    TEST_ASSERT_EQUAL(0, fat16_mount(&volume, read_sector, 0U));
    TEST_ASSERT_EQUAL(OS_FAT16_BAD_PATH, fat16_read_file(&volume, "../x", content, sizeof(content)));
    TEST_ASSERT_EQUAL(OS_FAT16_BUFFER_SMALL, fat16_read_file(&volume, "FATOK.TXT", content, sizeof(content)));
}

static void test_unlinks_and_renames_lfn_file(void) {
    fat16_volume_t volume;
    uint8_t data[5] = {'D', 'A', 'T', 'A', '1'};
    char readback[5] = {0};
    uint16_t first = 0U;
    make_volume();
    TEST_ASSERT_EQUAL(0, fat16_mount(&volume, read_sector, 0U));
    TEST_ASSERT_EQUAL(0, fat16_attach_writer(&volume, write_sector));
    TEST_ASSERT_EQUAL(0, fat16_create_lfn_file(&volume, "long-file-2026.txt", "LONG01.TXT",
                                               0x20U, data, sizeof(data), &first));
    TEST_ASSERT_EQUAL(5, fat16_read_file(&volume, "long-file-2026.txt", readback, sizeof(readback)));
    TEST_ASSERT_EQUAL_MEMORY(data, readback, sizeof(data));

    // Test rename LFN -> LFN (cardinalité identique)
    TEST_ASSERT_EQUAL(0, fat16_rename_lfn_file(&volume, "long-file-2026.txt", "renamed-file-26.txt", "LONG01.TXT"));
    TEST_ASSERT_EQUAL(OS_FAT16_NOT_FOUND, fat16_read_file(&volume, "long-file-2026.txt", readback, sizeof(readback)));
    TEST_ASSERT_EQUAL(5, fat16_read_file(&volume, "renamed-file-26.txt", readback, sizeof(readback)));
    TEST_ASSERT_EQUAL_MEMORY(data, readback, sizeof(data));

    // Test unlink LFN
    TEST_ASSERT_EQUAL(0, fat16_unlink_file(&volume, "renamed-file-26.txt"));
    TEST_ASSERT_EQUAL(OS_FAT16_NOT_FOUND, fat16_read_file(&volume, "renamed-file-26.txt", readback, sizeof(readback)));
}

static void test_mutates_one_level_subdirectory_without_overwrite(void) {
    fat16_volume_t volume;
    os_fat16_dirent_t entries[4];
    uint8_t alpha[5] = {'a', 'l', 'p', 'h', 'a'};
    uint8_t beta[4] = {'b', 'e', 't', 'a'};
    char readback[5] = {0};
    uint16_t first = 0U;
    make_volume();
    TEST_ASSERT_EQUAL(0, fat16_mount(&volume, read_sector, 0U));
    TEST_ASSERT_EQUAL(0, fat16_attach_writer(&volume, write_sector));
    TEST_ASSERT_EQUAL(0, fat16_create_directory(&volume, "DIR"));
    TEST_ASSERT_EQUAL(OS_FAT16_BAD_PATH, fat16_create_directory(&volume, "DIR"));
    TEST_ASSERT_EQUAL(0, fat16_create_path_file(&volume, "DIR/CHILD.TXT", alpha,
                                                sizeof(alpha), &first));
    TEST_ASSERT_TRUE(first >= 2U);
    TEST_ASSERT_EQUAL(OS_FAT16_BAD_PATH, fat16_create_path_file(&volume, "DIR/CHILD.TXT", beta,
                                                                 sizeof(beta), &first));
    TEST_ASSERT_EQUAL(1, fat16_list_path_page(&volume, "DIR/", 0U, entries, 4U));
    TEST_ASSERT_EQUAL_STRING("CHILD.TXT", entries[0].name);
    TEST_ASSERT_EQUAL(5, fat16_read_path(&volume, "DIR/CHILD.TXT", readback, sizeof(readback)));
    TEST_ASSERT_EQUAL_MEMORY(alpha, readback, sizeof(alpha));
    TEST_ASSERT_EQUAL(0, fat16_rename_path_file(&volume, "DIR/CHILD.TXT", "DIR/RENAMED.TXT"));
    TEST_ASSERT_EQUAL(0, fat16_create_path_file(&volume, "DIR/OTHER.TXT", beta,
                                                sizeof(beta), &first));
    TEST_ASSERT_EQUAL(OS_FAT16_BAD_PATH, fat16_rename_path_file(&volume, "DIR/RENAMED.TXT",
                                                                 "DIR/OTHER.TXT"));
    TEST_ASSERT_EQUAL(0, fat16_create_path_file(&volume, "DIR/ONE.TXT", beta,
                                                sizeof(beta), &first));
    TEST_ASSERT_EQUAL(0, fat16_create_path_file(&volume, "DIR/TWO.TXT", beta,
                                                sizeof(beta), &first));
    TEST_ASSERT_EQUAL(0, fat16_create_path_file(&volume, "DIR/THREE.TXT", beta,
                                                sizeof(beta), &first));
    TEST_ASSERT_EQUAL(4, fat16_list_path_page(&volume, "DIR/", 0U, entries, 4U));
    TEST_ASSERT_EQUAL(1, fat16_list_path_page(&volume, "DIR/", 4U, entries, 4U));
    TEST_ASSERT_EQUAL_STRING("THREE.TXT", entries[0].name);
    TEST_ASSERT_EQUAL(OS_FAT16_BAD_PATH, fat16_remove_directory(&volume, "DIR"));
    TEST_ASSERT_EQUAL(0, fat16_unlink_path_file(&volume, "DIR/RENAMED.TXT"));
    TEST_ASSERT_EQUAL(0, fat16_unlink_path_file(&volume, "DIR/OTHER.TXT"));
    TEST_ASSERT_EQUAL(0, fat16_unlink_path_file(&volume, "DIR/ONE.TXT"));
    TEST_ASSERT_EQUAL(0, fat16_unlink_path_file(&volume, "DIR/TWO.TXT"));
    TEST_ASSERT_EQUAL(0, fat16_unlink_path_file(&volume, "DIR/THREE.TXT"));
    TEST_ASSERT_EQUAL(0, fat16_remove_directory(&volume, "DIR"));
}

static void test_mutates_multilevel_subdirectory(void) {
    fat16_volume_t volume;
    os_fat16_dirent_t entries[4];
    uint8_t payload[7] = {'N', 'E', 'S', 'T', 'E', 'D', '!'};
    char readback[8] = {0};
    uint16_t first = 0U;
    make_volume();
    TEST_ASSERT_EQUAL(0, fat16_mount(&volume, read_sector, 0U));
    TEST_ASSERT_EQUAL(0, fat16_attach_writer(&volume, write_sector));
    TEST_ASSERT_EQUAL(0, fat16_create_directory(&volume, "SUB1"));
    TEST_ASSERT_EQUAL(0, fat16_create_directory(&volume, "SUB1/SUB2"));
    TEST_ASSERT_EQUAL(0, fat16_create_path_file(&volume, "SUB1/SUB2/FILE.TXT", payload,
                                                sizeof(payload), &first));
    TEST_ASSERT_TRUE(first >= 2U);
    TEST_ASSERT_EQUAL(1, fat16_list_path_page(&volume, "SUB1/SUB2/", 0U, entries, 4U));
    TEST_ASSERT_EQUAL_STRING("FILE.TXT", entries[0].name);
    TEST_ASSERT_EQUAL(7, fat16_read_path(&volume, "SUB1/SUB2/FILE.TXT", readback, sizeof(readback)));
    TEST_ASSERT_EQUAL_MEMORY(payload, readback, sizeof(payload));
    TEST_ASSERT_EQUAL(0, fat16_rename_path_file(&volume, "SUB1/SUB2/FILE.TXT", "SUB1/SUB2/RENAMED.TXT"));
    TEST_ASSERT_EQUAL(7, fat16_read_path(&volume, "SUB1/SUB2/RENAMED.TXT", readback, sizeof(readback)));
    TEST_ASSERT_EQUAL(OS_FAT16_BAD_PATH, fat16_remove_directory(&volume, "SUB1/SUB2"));
    TEST_ASSERT_EQUAL(OS_FAT16_BAD_PATH, fat16_remove_directory(&volume, "SUB1"));
    TEST_ASSERT_EQUAL(0, fat16_unlink_path_file(&volume, "SUB1/SUB2/RENAMED.TXT"));
    TEST_ASSERT_EQUAL(0, fat16_remove_directory(&volume, "SUB1/SUB2"));
    TEST_ASSERT_EQUAL(0, fat16_remove_directory(&volume, "SUB1"));
}

static void test_creates_lfn_in_subdirectory(void) {
    fat16_volume_t volume;
    os_fat16_dirent_t entries[4];
    uint8_t payload[11] = {'S', 'U', 'B', '-', 'L', 'F', 'N', '-', 'O', 'K', '!'};
    char readback[12] = {0};
    uint16_t first = 0U;
    make_volume();
    TEST_ASSERT_EQUAL(0, fat16_mount(&volume, read_sector, 0U));
    TEST_ASSERT_EQUAL(0, fat16_attach_writer(&volume, write_sector));
    TEST_ASSERT_EQUAL(0, fat16_create_directory(&volume, "DIR1"));
    TEST_ASSERT_EQUAL(0, fat16_create_path_file(&volume, "DIR1/long-subdirectory-file.txt", payload,
                                                sizeof(payload), &first));
    TEST_ASSERT_TRUE(first >= 2U);
    TEST_ASSERT_EQUAL(1, fat16_list_path_page(&volume, "DIR1/", 0U, entries, 4U));
    TEST_ASSERT_EQUAL_STRING("long-subdirectory-file.txt", entries[0].name);
    TEST_ASSERT_EQUAL(11, fat16_read_path(&volume, "DIR1/long-subdirectory-file.txt", readback, sizeof(readback)));
    TEST_ASSERT_EQUAL_MEMORY(payload, readback, sizeof(payload));
    TEST_ASSERT_EQUAL(0, fat16_unlink_path_file(&volume, "DIR1/long-subdirectory-file.txt"));
    TEST_ASSERT_EQUAL(0, fat16_remove_directory(&volume, "DIR1"));
}

static void test_fat16_deep_nested_boundaries(void) {
    fat16_volume_t volume;
    os_fat16_dirent_t entries[4];
    uint8_t payload[9] = {'D', 'E', 'E', 'P', 'T', 'E', 'S', 'T', '!'};
    char readback[10] = {0};
    uint16_t first = 0U;
    make_volume();
    TEST_ASSERT_EQUAL(0, fat16_mount(&volume, read_sector, 0U));
    TEST_ASSERT_EQUAL(0, fat16_attach_writer(&volume, write_sector));
    TEST_ASSERT_EQUAL(0, fat16_create_directory(&volume, "SUB1"));
    TEST_ASSERT_EQUAL(0, fat16_create_directory(&volume, "SUB1/SUB2"));
    TEST_ASSERT_EQUAL(0, fat16_create_directory(&volume, "SUB1/SUB2/SUB3"));
    TEST_ASSERT_EQUAL(0, fat16_create_path_file(&volume, "SUB1/SUB2/SUB3/FILE.TXT", payload,
                                                sizeof(payload), &first));
    TEST_ASSERT_TRUE(first >= 2U);
    TEST_ASSERT_EQUAL(1, fat16_list_path_page(&volume, "SUB1/SUB2/SUB3/", 0U, entries, 4U));
    TEST_ASSERT_EQUAL_STRING("FILE.TXT", entries[0].name);
    TEST_ASSERT_EQUAL(9, fat16_read_path(&volume, "SUB1/SUB2/SUB3/FILE.TXT", readback, sizeof(readback)));
    TEST_ASSERT_EQUAL_MEMORY(payload, readback, sizeof(payload));
    TEST_ASSERT_EQUAL(0, fat16_unlink_path_file(&volume, "SUB1/SUB2/SUB3/FILE.TXT"));
    TEST_ASSERT_EQUAL(0, fat16_remove_directory(&volume, "SUB1/SUB2/SUB3"));
    TEST_ASSERT_EQUAL(0, fat16_remove_directory(&volume, "SUB1/SUB2"));
    TEST_ASSERT_EQUAL(0, fat16_remove_directory(&volume, "SUB1"));
}

int main(void) {
    unity_init();
    RUN_TEST(test_mount_list_and_read);
    RUN_TEST(test_lists_root_page_after_first_entry);
    RUN_TEST(test_loads_gpt2_from_fat16);
    RUN_TEST(test_indexes_gpt2_header_from_fat16);
    RUN_TEST(test_executes_q4_block_from_fat16);
    RUN_TEST(test_generates_gguf_token_logits_from_fat16);
    RUN_TEST(test_streams_output_top_k_equivalently_from_fat16);
    RUN_TEST(test_reads_bounded_file_range);
    RUN_TEST(test_cursor_reads_successive_windows);
    RUN_TEST(test_cursor_caches_shared_sector);
    RUN_TEST(test_rejects_bad_bpb);
    RUN_TEST(test_cursor_uses_attached_multisector_window);
    RUN_TEST(test_reads_deep_multisector_cluster_without_false_corruption);
    RUN_TEST(test_rejects_bad_name_and_small_buffer);
    RUN_TEST(test_writes_only_with_explicit_writer);
    RUN_TEST(test_creates_persistent_file);
    RUN_TEST(test_unlinks_persistent_file_and_reuses_cluster);
    RUN_TEST(test_renames_persistent_file_without_moving_chain);
    RUN_TEST(test_creates_lfn_file);
    RUN_TEST(test_creates_utf8_lfn_file);
    RUN_TEST(test_creates_non_bmp_lfn_file);
    RUN_TEST(test_unlinks_and_renames_lfn_file);
    RUN_TEST(test_mutates_one_level_subdirectory_without_overwrite);
    RUN_TEST(test_mutates_multilevel_subdirectory);
    RUN_TEST(test_creates_lfn_in_subdirectory);
    RUN_TEST(test_fat16_deep_nested_boundaries);
    unity_print_results();
    unity_cleanup();
    return 0;
}
