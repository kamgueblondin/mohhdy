#include "gpt2_gguf_loader.h"

int gpt2_gguf_load_fat16(const fat16_volume_t* volume, const char* filename,
                         uint8_t* buffer, uint32_t capacity,
                         gpt2_gguf_loaded_model_t* out) {
    int bytes;
    int status;
    if (!volume || !filename || !buffer || capacity == 0U || !out) return -1;
    if (!fat16_is_mounted(volume)) return OS_FAT16_NOT_MOUNTED;
    bytes = fat16_read_file(volume, filename, (char*)buffer, capacity);
    if (bytes < 0) return bytes;
    if (bytes == 0) return -2;
    status = gpt2_gguf_build_index(buffer, (uint32_t)bytes, &out->index);
    if (status != 0) return -100 + status;
    out->bytes_loaded = (uint32_t)bytes;
    return 0;
}

int gpt2_gguf_load_fat16_header(const fat16_volume_t* volume, const char* filename,
                                uint8_t* header, uint32_t header_capacity,
                                gpt2_gguf_loaded_model_t* out) {
    fat16_file_t file;
    uint32_t read = 0U;
    uint32_t requested;
    int status;
    if (!volume || !filename || !header || header_capacity == 0U || !out) return -1;
    if (!fat16_is_mounted(volume)) return OS_FAT16_NOT_MOUNTED;
    status = fat16_open_file(volume, filename, &file);
    if (status != 0) return status;
    if (file.size == 0U) return -2;
    requested = file.size < header_capacity ? file.size : header_capacity;
    status = fat16_read_file_range(volume, filename, 0U, header, requested, &read);
    if (status != 0) return status;
    if (read == 0U) return -2;
    status = gpt2_gguf_build_index_header(header, read, file.size, &out->index);
    if (status != 0) return -100 + status;
    out->bytes_loaded = read;
    return 0;
}

static int string_equals_loader(const char* s1, const char* s2) {
    if (!s1 || !s2) return 0;
    while (*s1 && *s2) {
        if (*s1 != *s2) return 0;
        s1++;
        s2++;
    }
    return (*s1 == *s2);
}

int gpt2_gguf_load_fat32_header(const fat32_volume_t* volume, const char* filename,
                                uint8_t* header, uint32_t header_capacity,
                                gpt2_gguf_loaded_model_t* out) {
    uint32_t read = 0U;
    uint32_t total_size = 0U;
    uint32_t requested;
    uint32_t start = 0U;
    os_fat16_dirent_t page[8];
    int status;
    int found = 0;

    if (!volume || !filename || !header || header_capacity == 0U || !out) return -1;
    if (!fat32_is_mounted(volume)) return OS_FAT16_NOT_MOUNTED;

    while (1) {
        int count = fat32_list_root_page(volume, start, page, 8U);
        if (count <= 0) break;
        for (int i = 0; i < count; i++) {
            if (string_equals_loader(page[i].name, filename)) {
                total_size = page[i].size;
                found = 1;
                break;
            }
        }
        if (found) break;
        start += (uint32_t)count;
    }
    if (!found) return OS_FAT16_NOT_FOUND;
    if (total_size == 0U) return -2;

    requested = total_size < header_capacity ? total_size : header_capacity;
    status = fat32_read_file_range(volume, filename, 0U, header, requested, &read);
    if (status != 0) return status;
    if (read == 0U) return -2;

    status = gpt2_gguf_build_index_header(header, read, total_size, &out->index);
    if (status != 0) return -100 + status;
    out->bytes_loaded = read;
    return 0;
}


int gpt2_gguf_read_tensor_fat16(const fat16_volume_t* volume, const char* filename,
                                const gpt2_gguf_loaded_model_t* model,
                                const gpt2_gguf_tensor_t* tensor,
                                uint32_t tensor_offset, uint8_t* buffer,
                                uint32_t capacity, uint32_t* out_read) {
    uint32_t absolute;
    if (out_read) *out_read = 0U;
    if (!volume || !filename || !model || !tensor || !buffer || capacity == 0U || !out_read) return -1;
    if (!model->index.info.is_valid || tensor_offset > tensor->byte_size) return -3;
    if (capacity > tensor->byte_size - tensor_offset) capacity = tensor->byte_size - tensor_offset;
    if (model->index.info.tensor_data_offset > 0xFFFFFFFFU - tensor->data_offset) return -3;
    absolute = model->index.info.tensor_data_offset + tensor->data_offset;
    if (absolute > 0xFFFFFFFFU - tensor_offset) return -3;
    absolute += tensor_offset;
    return fat16_read_file_range(volume, filename, absolute, buffer, capacity, out_read);
}


int gpt2_gguf_read_dense_row_fat16(const fat16_volume_t* volume, const char* filename,
                                   const gpt2_gguf_loaded_model_t* model,
                                   const gpt2_gguf_tensor_t* tensor,
                                   uint32_t row_index, uint32_t width,
                                   uint8_t* scratch, uint32_t scratch_capacity,
                                   float* output, uint32_t output_capacity) {
    uint64_t rows = 1U;
    uint64_t row_bytes;
    uint64_t row_offset;
    uint32_t element_bytes;
    uint32_t read = 0U;
    uint32_t index;
    int status;
    if (!volume || !filename || !model || !tensor || !scratch || !output) return -1;
    if (tensor->dimensions == 0U || tensor->dimensions > 2U || width == 0U ||
        width > 0xFFFFFFFFU / (uint32_t)sizeof(float) ||
        tensor->shape[0] != width) return -9;
    if (tensor->dimensions == 2U) rows = tensor->shape[1];
    if (rows == 0U || row_index >= rows) return -9;
    if (tensor->type == GPT2_GGUF_TENSOR_F32) element_bytes = 4U;
    else if (tensor->type == GPT2_GGUF_TENSOR_F16) element_bytes = 2U;
    else return -4;
    row_bytes = (uint64_t)width * element_bytes;
    row_offset = row_bytes * row_index;
    if (row_bytes > 0xFFFFFFFFULL || row_offset > 0xFFFFFFFFULL ||
        row_offset > tensor->byte_size || row_bytes > tensor->byte_size - row_offset ||
        scratch_capacity < (uint32_t)row_bytes ||
        output_capacity < width * (uint32_t)sizeof(float)) return -6;
    status = gpt2_gguf_read_tensor_fat16(volume, filename, model, tensor,
                                          (uint32_t)row_offset, scratch,
                                          (uint32_t)row_bytes, &read);
    if (status != 0) return status;
    if (read != (uint32_t)row_bytes) return -8;
    for (index = 0U; index < width; index++) {
        uint32_t offset = index * element_bytes;
        if (element_bytes == 4U) {
            union { uint32_t bits; float value; } convert;
            convert.bits = (uint32_t)scratch[offset] |
                           ((uint32_t)scratch[offset + 1U] << 8U) |
                           ((uint32_t)scratch[offset + 2U] << 16U) |
                           ((uint32_t)scratch[offset + 3U] << 24U);
            output[index] = convert.value;
        } else {
            uint16_t bits = (uint16_t)scratch[offset] |
                            (uint16_t)((uint16_t)scratch[offset + 1U] << 8U);
            output[index] = gpt2_f16_to_f32(bits);
        }
    }
    return 0;
}

int gpt2_gguf_read_quant_block_fat16(const fat16_volume_t* volume, const char* filename,
                                     const gpt2_gguf_loaded_model_t* model,
                                     const gpt2_gguf_tensor_t* tensor,
                                     uint32_t block_index, uint8_t* buffer,
                                     uint32_t capacity, uint32_t* out_read) {
    uint32_t block_bytes;
    uint32_t block_offset;
    if (out_read) *out_read = 0U;
    if (!volume || !filename || !model || !tensor || !buffer || !out_read) return -1;
    if (tensor->type == GPT2_GGUF_TENSOR_Q3_K) block_bytes = GPT2_Q3_K_BLOCK_BYTES;
    else if (tensor->type == GPT2_GGUF_TENSOR_Q4_K) block_bytes = GPT2_Q4_K_BLOCK_BYTES;
    else if (tensor->type == GPT2_GGUF_TENSOR_Q6_K) block_bytes = GPT2_Q6_K_BLOCK_BYTES;
    else return -4;
    if (block_index > 0xFFFFFFFFU / block_bytes) return -5;
    block_offset = block_index * block_bytes;
    if (block_offset > tensor->byte_size || block_bytes > tensor->byte_size - block_offset) return -5;
    if (capacity < block_bytes) return -6;
    return gpt2_gguf_read_tensor_fat16(volume, filename, model, tensor,
                                       block_offset, buffer, block_bytes, out_read);
}


int gpt2_gguf_dot_quant_block_fat16(const fat16_volume_t* volume, const char* filename,
                                    const gpt2_gguf_loaded_model_t* model,
                                    const gpt2_gguf_tensor_t* tensor,
                                    uint32_t block_index, const float* input,
                                    uint32_t count, uint8_t* scratch,
                                    uint32_t scratch_capacity, float* out_dot) {
    uint32_t block_bytes;
    uint32_t read = 0U;
    int status;
    if (!input || !scratch || !out_dot) return -1;
    if (count != GPT2_QK_K) return -7;
    if (!tensor) return -1;
    if (tensor->type == GPT2_GGUF_TENSOR_Q3_K) block_bytes = GPT2_Q3_K_BLOCK_BYTES;
    else if (tensor->type == GPT2_GGUF_TENSOR_Q4_K) block_bytes = GPT2_Q4_K_BLOCK_BYTES;
    else if (tensor->type == GPT2_GGUF_TENSOR_Q6_K) block_bytes = GPT2_Q6_K_BLOCK_BYTES;
    else return -4;
    if (scratch_capacity < block_bytes) return -6;
    status = gpt2_gguf_read_quant_block_fat16(volume, filename, model, tensor,
                                               block_index, scratch, scratch_capacity, &read);
    if (status != 0) return status;
    if (read != block_bytes) return -8;
    if (tensor->type == GPT2_GGUF_TENSOR_Q3_K) *out_dot = gpt2_q3_k_dot_f32(input, scratch, count);
    else if (tensor->type == GPT2_GGUF_TENSOR_Q4_K) *out_dot = gpt2_q4_k_dot_f32(input, scratch, count);
    else *out_dot = gpt2_q6_k_dot_f32(input, scratch, count);
    return 0;
}


int gpt2_gguf_dot_quant_tensor_fat16(const fat16_volume_t* volume, const char* filename,
                                     const gpt2_gguf_loaded_model_t* model,
                                     const gpt2_gguf_tensor_t* tensor,
                                     const float* input, uint32_t count,
                                     uint8_t* scratch, uint32_t scratch_capacity,
                                     float* out_dot) {
    uint32_t blocks;
    uint32_t block;
    float total = 0.0f;
    int status;
    if (!input || !scratch || !out_dot || !tensor) return -1;
    if (count == 0U || (count % GPT2_QK_K) != 0U) return -7;
    blocks = count / GPT2_QK_K;
    for (block = 0U; block < blocks; block++) {
        float partial = 0.0f;
        status = gpt2_gguf_dot_quant_block_fat16(volume, filename, model, tensor,
                                                  block, input + block * GPT2_QK_K,
                                                  GPT2_QK_K, scratch, scratch_capacity,
                                                  &partial);
        if (status != 0) return status;
        total += partial;
    }
    *out_dot = total;
    return 0;
}


int gpt2_gguf_dot_quant_row_fat16(const fat16_volume_t* volume, const char* filename,
                                  const gpt2_gguf_loaded_model_t* model,
                                  const gpt2_gguf_tensor_t* tensor,
                                  uint32_t row_index, const float* input,
                                  uint32_t count, uint8_t* scratch,
                                  uint32_t scratch_capacity, float* out_dot) {
    uint64_t width;
    uint64_t rows = 1U;
    uint32_t block_bytes;
    uint64_t row_bytes;
    uint64_t row_offset;
    uint32_t first_block;
    uint32_t blocks;
    uint32_t block;
    float total = 0.0f;
    int status;
    if (!tensor || tensor->dimensions == 0U || tensor->dimensions > 2U) return -9;
    width = tensor->shape[0];
    if (tensor->dimensions == 2U) rows = tensor->shape[1];
    if (width == 0U || width > 0xFFFFFFFFU || rows == 0U || row_index >= rows) return -9;
    if (count != (uint32_t)width || (count % GPT2_QK_K) != 0U) return -7;
    if (tensor->type == GPT2_GGUF_TENSOR_Q3_K) block_bytes = GPT2_Q3_K_BLOCK_BYTES;
    else if (tensor->type == GPT2_GGUF_TENSOR_Q4_K) block_bytes = GPT2_Q4_K_BLOCK_BYTES;
    else if (tensor->type == GPT2_GGUF_TENSOR_Q6_K) block_bytes = GPT2_Q6_K_BLOCK_BYTES;
    else return -4;
    row_bytes = (width / GPT2_QK_K) * block_bytes;
    row_offset = row_bytes * row_index;
    if (row_offset > 0xFFFFFFFFU) return -9;
    first_block = (uint32_t)(row_offset / block_bytes);
    blocks = (uint32_t)(width / GPT2_QK_K);
    for (block = 0U; block < blocks; block++) {
        float partial = 0.0f;
        if (first_block > 0xFFFFFFFFU - block) return -9;
        status = gpt2_gguf_dot_quant_block_fat16(volume, filename, model, tensor,
                                                  first_block + block,
                                                  input + block * GPT2_QK_K,
                                                  GPT2_QK_K, scratch,
                                                  scratch_capacity, &partial);
        if (status != 0) return status;
        total += partial;
    }
    *out_dot = total;
    return 0;
}


int gpt2_gguf_runtime_prepare(const gpt2_gguf_loaded_model_t* model,
                              uint32_t layer_count, uint32_t channels,
                              char* name, uint32_t name_capacity,
                              gpt2_gguf_layer_t* layers,
                              uint32_t layer_capacity,
                              gpt2_gguf_runtime_t* out) {
    uint32_t layer;
    int status;
    if (!model || !name || !layers || !out) return -1;
    if (layer_count == 0U || channels == 0U || name_capacity == 0U ||
        layer_capacity < layer_count) return -6;
    for (layer = 0U; layer < layer_count; layer++) {
        status = gpt2_gguf_map_layer(&model->index, layer, name, name_capacity,
                                     &layers[layer]);
        if (status != 0) return status;
        status = gpt2_gguf_validate_gpt2_layer_storage(&layers[layer], channels);
        if (status != 0) return status;
    }
    out->model = model;
    out->layers = layers;
    out->layer_count = layer_count;
    out->channels = channels;
    out->ready = 1U;
    return 0;
}


int gpt2_gguf_runtime_get_layer(const gpt2_gguf_runtime_t* runtime,
                                uint32_t layer_index, gpt2_gguf_layer_t* out) {
    if (!runtime || !out || !runtime->ready || !runtime->layers) return -1;
    if (layer_index >= runtime->layer_count) return -6;
    *out = runtime->layers[layer_index];
    return 0;
}


int gpt2_gguf_generation_prepare(const gpt2_gguf_loaded_model_t* model,
                                 char* name, uint32_t name_capacity,
                                 gpt2_gguf_layer_t* layers, uint32_t layer_capacity,
                                 gpt2_gguf_generation_t* out) {
    gpt2_gguf_generation_t prepared;
    gpt2_gguf_tensor_t probe;
    uint32_t layer_count = 0U;
    uint32_t channels;
    int status;
    if (!model || !name || !layers || !out || name_capacity == 0U || layer_capacity == 0U ||
        !model->index.info.is_valid) return -1;
    status = gpt2_gguf_map_role(&model->index, GPT2_GGUF_ROLE_TOKEN_EMBEDDING,
                                &prepared.token_embedding);
    if (status != 0) return status;
    status = gpt2_gguf_map_role(&model->index, GPT2_GGUF_ROLE_POSITION_EMBEDDING,
                                &prepared.position_embedding);
    if (status != 0) return status;
    status = gpt2_gguf_map_role(&model->index, GPT2_GGUF_ROLE_OUTPUT_NORM_WEIGHT,
                                &prepared.output_norm_weight);
    if (status != 0) return status;
    status = gpt2_gguf_map_role(&model->index, GPT2_GGUF_ROLE_OUTPUT_NORM_BIAS,
                                &prepared.output_norm_bias);
    if (status != 0) return status;
    status = gpt2_gguf_map_role(&model->index, GPT2_GGUF_ROLE_OUTPUT_WEIGHT,
                                &prepared.output_weight);
    if (status != 0) return status;
    if (prepared.token_embedding.dimensions != 2U ||
        prepared.position_embedding.dimensions != 2U ||
        prepared.output_norm_weight.dimensions != 1U ||
        prepared.output_norm_bias.dimensions != 1U ||
        prepared.output_weight.dimensions != 2U) return -9;
    if (prepared.token_embedding.shape[0] > 0xFFFFFFFFULL ||
        prepared.token_embedding.shape[1] > 0xFFFFFFFFULL ||
        prepared.position_embedding.shape[0] > 0xFFFFFFFFULL ||
        prepared.position_embedding.shape[1] > 0xFFFFFFFFULL ||
        prepared.output_norm_weight.shape[0] > 0xFFFFFFFFULL ||
        prepared.output_norm_bias.shape[0] > 0xFFFFFFFFULL ||
        prepared.output_weight.shape[0] > 0xFFFFFFFFULL ||
        prepared.output_weight.shape[1] > 0xFFFFFFFFULL) return -9;
    channels = (uint32_t)prepared.token_embedding.shape[0];
    if (channels == 0U || channels > 0xFFFFFFFFU / (uint32_t)sizeof(float) ||
        prepared.token_embedding.shape[1] == 0U ||
        prepared.position_embedding.shape[0] != channels ||
        prepared.position_embedding.shape[1] == 0U ||
        prepared.output_norm_weight.shape[0] != channels ||
        prepared.output_norm_bias.shape[0] != channels ||
        prepared.output_weight.shape[0] != channels ||
        prepared.output_weight.shape[1] != prepared.token_embedding.shape[1]) return -9;
    if ((prepared.token_embedding.type != GPT2_GGUF_TENSOR_F32 &&
         prepared.token_embedding.type != GPT2_GGUF_TENSOR_F16 &&
         prepared.token_embedding.type != GPT2_GGUF_TENSOR_Q3_K &&
         prepared.token_embedding.type != GPT2_GGUF_TENSOR_Q4_K &&
         prepared.token_embedding.type != GPT2_GGUF_TENSOR_Q6_K) ||
        (prepared.position_embedding.type != GPT2_GGUF_TENSOR_F32 &&
         prepared.position_embedding.type != GPT2_GGUF_TENSOR_F16) ||
        (prepared.output_norm_weight.type != GPT2_GGUF_TENSOR_F32 &&
         prepared.output_norm_weight.type != GPT2_GGUF_TENSOR_F16) ||
        (prepared.output_norm_bias.type != GPT2_GGUF_TENSOR_F32 &&
         prepared.output_norm_bias.type != GPT2_GGUF_TENSOR_F16) ||
        (prepared.output_weight.type != GPT2_GGUF_TENSOR_Q3_K &&
         prepared.output_weight.type != GPT2_GGUF_TENSOR_Q4_K &&
         prepared.output_weight.type != GPT2_GGUF_TENSOR_Q6_K)) return -9;
    status = gpt2_gguf_validate_tensor_size(&prepared.token_embedding);
    if (status != 0) return status;
    status = gpt2_gguf_validate_tensor_size(&prepared.position_embedding);
    if (status != 0) return status;
    status = gpt2_gguf_validate_tensor_size(&prepared.output_norm_weight);
    if (status != 0) return status;
    status = gpt2_gguf_validate_tensor_size(&prepared.output_norm_bias);
    if (status != 0) return status;
    status = gpt2_gguf_validate_tensor_size(&prepared.output_weight);
    if (status != 0) return status;
    while (layer_count < layer_capacity) {
        status = gpt2_gguf_map_layer_role(&model->index, layer_count,
                                          GPT2_GGUF_ROLE_LAYER_ATTN_NORM_WEIGHT,
                                          name, name_capacity, &probe);
        if (status == -8) break;
        if (status != 0) return status;
        status = gpt2_gguf_map_layer(&model->index, layer_count, name, name_capacity,
                                     &layers[layer_count]);
        if (status != 0) return status;
        status = gpt2_gguf_validate_gpt2_layer_storage(&layers[layer_count], channels);
        if (status != 0) return status;
        layer_count++;
    }
    if (layer_count == 0U) return -8;
    if (layer_count == layer_capacity &&
        gpt2_gguf_map_layer_role(&model->index, layer_count,
                                 GPT2_GGUF_ROLE_LAYER_ATTN_NORM_WEIGHT,
                                 name, name_capacity, &probe) == 0) return -6;
    prepared.runtime.model = model;
    prepared.runtime.layers = layers;
    prepared.runtime.layer_count = layer_count;
    prepared.runtime.channels = channels;
    prepared.runtime.ready = 1U;
    prepared.vocabulary = (uint32_t)prepared.token_embedding.shape[1];
    prepared.max_positions = (uint32_t)prepared.position_embedding.shape[1];
    prepared.ready = 1U;
    *out = prepared;
    return 0;
}

int gpt2_gguf_forward_context_init(const gpt2_gguf_loaded_model_t* model,
                                   uint32_t layer_index, uint32_t channels,
                                   uint32_t position, char* name, uint32_t name_capacity,
                                   uint8_t* scratch, uint32_t scratch_capacity,
                                   gpt2_gguf_forward_context_t* out) {
    int status;
    if (!model || !model->index.info.is_valid || channels == 0U || !name ||
        name_capacity == 0U || !scratch || scratch_capacity == 0U || !out) return -1;
    status = gpt2_gguf_map_layer(&model->index, layer_index, name, name_capacity, &out->layer);
    if (status != 0) return status;
    status = gpt2_gguf_validate_gpt2_layer_storage(&out->layer, channels);
    if (status != 0) return status;
    out->model = model;
    out->scratch = scratch;
    out->scratch_capacity = scratch_capacity;
    out->channels = channels;
    out->position = position;
    return 0;
}


int gpt2_gguf_read_quant_row_fat16(const fat16_volume_t* volume, const char* filename,
                                   const gpt2_gguf_loaded_model_t* model,
                                   const gpt2_gguf_tensor_t* tensor,
                                   uint32_t row_index, uint8_t* buffer,
                                   uint32_t capacity, uint32_t* out_read) {
    uint64_t width;
    uint64_t rows = 1U;
    uint32_t block_bytes;
    uint64_t row_bytes;
    uint64_t row_offset;
    int status;
    if (out_read) *out_read = 0U;
    if (!volume || !filename || !model || !tensor || !buffer || !out_read) return -1;
    if (!model->index.info.is_valid || tensor->dimensions == 0U || tensor->dimensions > 2U) return -9;
    width = tensor->shape[0];
    if (tensor->dimensions == 2U) rows = tensor->shape[1];
    if (width == 0U || width > 0xFFFFFFFFULL || rows == 0U || row_index >= rows) return -9;
    if (tensor->type == GPT2_GGUF_TENSOR_Q3_K) block_bytes = GPT2_Q3_K_BLOCK_BYTES;
    else if (tensor->type == GPT2_GGUF_TENSOR_Q4_K) block_bytes = GPT2_Q4_K_BLOCK_BYTES;
    else if (tensor->type == GPT2_GGUF_TENSOR_Q6_K) block_bytes = GPT2_Q6_K_BLOCK_BYTES;
    else return -4;
    if ((width % GPT2_QK_K) != 0U) return -7;
    row_bytes = (width / GPT2_QK_K) * block_bytes;
    row_offset = row_bytes * row_index;
    if (row_bytes > 0xFFFFFFFFULL || row_offset > 0xFFFFFFFFULL) return -9;
    if (capacity < (uint32_t)row_bytes) return -6;
    status = gpt2_gguf_read_tensor_fat16(volume, filename, model, tensor,
                                          (uint32_t)row_offset, buffer,
                                          (uint32_t)row_bytes, out_read);
    if (status != 0) return status;
    if (*out_read != (uint32_t)row_bytes) return -8;
    return 0;
}

int gpt2_gguf_read_quant_row_dequant_fat16(const fat16_volume_t* volume,
                                           const char* filename,
                                           const gpt2_gguf_loaded_model_t* model,
                                           const gpt2_gguf_tensor_t* tensor,
                                           uint32_t row_index, uint8_t* row_buffer,
                                           uint32_t row_capacity, float* output,
                                           uint32_t output_capacity) {
    uint32_t read = 0U;
    uint32_t width;
    int status;
    if (!tensor || !row_buffer || !output || tensor->dimensions == 0U ||
        tensor->dimensions > 2U || tensor->shape[0] == 0U ||
        tensor->shape[0] > 0xFFFFFFFFULL) return -1;
    width = (uint32_t)tensor->shape[0];
    if (output_capacity < width) return -6;
    status = gpt2_gguf_read_quant_row_fat16(volume, filename, model, tensor,
                                             row_index, row_buffer, row_capacity, &read);
    if (status != 0) return status;
    if (tensor->type == GPT2_GGUF_TENSOR_Q3_K)
        return gpt2_q3_k_dequantize(row_buffer, width, output);
    if (tensor->type == GPT2_GGUF_TENSOR_Q4_K)
        return gpt2_q4_k_dequantize(row_buffer, width, output);
    if (tensor->type == GPT2_GGUF_TENSOR_Q6_K)
        return gpt2_q6_k_dequantize(row_buffer, width, output);
    return -4;
}


int gpt2_gguf_dot_quant_row_buffer(const gpt2_gguf_tensor_t* tensor,
                                   const uint8_t* row_buffer, uint32_t row_capacity,
                                   const float* input, uint32_t count, float* out_dot) {
    uint32_t block_bytes;
    uint32_t blocks;
    uint32_t block;
    float total = 0.0f;
    if (!tensor || !row_buffer || !input || !out_dot || tensor->dimensions == 0U || tensor->dimensions > 2U) return -1;
    if (tensor->shape[0] == 0U || tensor->shape[0] > 0xFFFFFFFFULL) return -9;
    if (count != (uint32_t)tensor->shape[0] || (count % GPT2_QK_K) != 0U) return -7;
    if (tensor->type == GPT2_GGUF_TENSOR_Q3_K) block_bytes = GPT2_Q3_K_BLOCK_BYTES;
    else if (tensor->type == GPT2_GGUF_TENSOR_Q4_K) block_bytes = GPT2_Q4_K_BLOCK_BYTES;
    else if (tensor->type == GPT2_GGUF_TENSOR_Q6_K) block_bytes = GPT2_Q6_K_BLOCK_BYTES;
    else return -4;
    blocks = count / GPT2_QK_K;
    if (blocks > 0xFFFFFFFFU / block_bytes || row_capacity < blocks * block_bytes) return -6;
    for (block = 0U; block < blocks; block++) {
        const float* values = input + block * GPT2_QK_K;
        const uint8_t* encoded = row_buffer + block * block_bytes;
        if (tensor->type == GPT2_GGUF_TENSOR_Q3_K) total += gpt2_q3_k_dot_f32(values, encoded, GPT2_QK_K);
        else if (tensor->type == GPT2_GGUF_TENSOR_Q4_K) total += gpt2_q4_k_dot_f32(values, encoded, GPT2_QK_K);
        else total += gpt2_q6_k_dot_f32(values, encoded, GPT2_QK_K);
    }
    *out_dot = total;
    return 0;
}


int gpt2_gguf_project_qkv_row_fat16(const fat16_volume_t* volume, const char* filename,
                                    const gpt2_gguf_loaded_model_t* model,
                                    const gpt2_gguf_tensor_t* tensor, uint32_t channels,
                                    uint32_t output_index, const float* input,
                                    uint8_t* row_buffer, uint32_t row_capacity,
                                    float* out_value) {
    uint32_t read = 0U;
    int status;
    if (!tensor || channels == 0U || !input || !row_buffer || !out_value) return -1;
    if (tensor->dimensions != 2U || tensor->shape[0] != channels ||
        tensor->shape[1] != 3ULL * channels || output_index >= 3U * channels) return -9;
    status = gpt2_gguf_read_quant_row_fat16(volume, filename, model, tensor,
                                             output_index, row_buffer, row_capacity, &read);
    if (status != 0) return status;
    return gpt2_gguf_dot_quant_row_buffer(tensor, row_buffer, read, input, channels, out_value);
}


int gpt2_gguf_project_qkv_fat16(const fat16_volume_t* volume, const char* filename,
                                const gpt2_gguf_loaded_model_t* model,
                                const gpt2_gguf_tensor_t* tensor, uint32_t channels,
                                const float* input, uint8_t* row_buffer,
                                uint32_t row_capacity, float* query, uint32_t query_capacity,
                                float* key, uint32_t key_capacity,
                                float* value, uint32_t value_capacity) {
    fat16_file_t file;
    uint32_t output_index;
    uint32_t block_bytes;
    uint32_t row_bytes;
    uint32_t offset;
    int status;
    if (!volume || !filename || !model || !tensor || !input || !row_buffer ||
        !query || !key || !value || channels == 0U ||
        query_capacity < channels * sizeof(float) ||
        key_capacity < channels * sizeof(float) ||
        value_capacity < channels * sizeof(float)) return -6;
    if (tensor->dimensions != 2U || tensor->shape[0] != channels ||
        tensor->shape[1] != 3ULL * channels) return -9;
    if (tensor->type == GPT2_GGUF_TENSOR_Q3_K) block_bytes = GPT2_Q3_K_BLOCK_BYTES;
    else if (tensor->type == GPT2_GGUF_TENSOR_Q4_K) block_bytes = GPT2_Q4_K_BLOCK_BYTES;
    else if (tensor->type == GPT2_GGUF_TENSOR_Q6_K) block_bytes = GPT2_Q6_K_BLOCK_BYTES;
    else return -4;
    if ((channels % GPT2_QK_K) != 0U ||
        channels / GPT2_QK_K > 0xFFFFFFFFU / block_bytes) return -7;
    row_bytes = (channels / GPT2_QK_K) * block_bytes;
    if (row_bytes == 0U || row_capacity < row_bytes ||
        model->index.info.tensor_data_offset > 0xFFFFFFFFU - tensor->data_offset) return -6;
    offset = model->index.info.tensor_data_offset + tensor->data_offset;
    status = fat16_open_file(volume, filename, &file);
    if (status != 0) return status;
    if (offset > file.size || tensor->byte_size > file.size - offset) return -3;
    status = fat16_file_seek(&file, offset);
    if (status != 0) return status;
    for (output_index = 0U; output_index < 3U * channels; output_index++) {
        float result = 0.0f;
        uint32_t read = 0U;
        status = fat16_file_read(&file, row_buffer, row_bytes, &read);
        if (status != 0) return status;
        if (read != row_bytes) return -8;
        status = gpt2_gguf_dot_quant_row_buffer(tensor, row_buffer, read,
                                                input, channels, &result);
        if (status != 0) return status;
        if (output_index < channels) query[output_index] = result;
        else if (output_index < 2U * channels) key[output_index - channels] = result;
        else value[output_index - 2U * channels] = result;
    }
    return 0;
}


int gpt2_gguf_project_matrix_fat16(const fat16_volume_t* volume, const char* filename,
                                   const gpt2_gguf_loaded_model_t* model,
                                   const gpt2_gguf_tensor_t* tensor,
                                   uint32_t input_channels, uint32_t output_channels,
                                   const float* input, uint8_t* row_buffer,
                                   uint32_t row_capacity, float* output,
                                   uint32_t output_capacity) {
    fat16_file_t file;
    uint32_t row;
    uint32_t block_bytes;
    uint32_t row_bytes;
    uint32_t offset;
    int status;
    if (!volume || !filename || !model || !tensor || !input || !row_buffer || !output) return -1;
    if (input_channels == 0U || output_channels == 0U ||
        tensor->dimensions != 2U || tensor->shape[0] != input_channels ||
        tensor->shape[1] != output_channels) return -9;
    if (output_capacity < output_channels * sizeof(float)) return -6;
    if (tensor->type == GPT2_GGUF_TENSOR_Q3_K) block_bytes = GPT2_Q3_K_BLOCK_BYTES;
    else if (tensor->type == GPT2_GGUF_TENSOR_Q4_K) block_bytes = GPT2_Q4_K_BLOCK_BYTES;
    else if (tensor->type == GPT2_GGUF_TENSOR_Q6_K) block_bytes = GPT2_Q6_K_BLOCK_BYTES;
    else return -4;
    if ((input_channels % GPT2_QK_K) != 0U ||
        input_channels / GPT2_QK_K > 0xFFFFFFFFU / block_bytes) return -7;
    row_bytes = (input_channels / GPT2_QK_K) * block_bytes;
    if (row_bytes == 0U || row_capacity < row_bytes ||
        model->index.info.tensor_data_offset > 0xFFFFFFFFU - tensor->data_offset) return -6;
    offset = model->index.info.tensor_data_offset + tensor->data_offset;
    status = fat16_open_file(volume, filename, &file);
    if (status != 0) return status;
    if (offset > file.size || tensor->byte_size > file.size - offset) return -3;
    status = fat16_file_seek(&file, offset);
    if (status != 0) return status;
    for (row = 0U; row < output_channels; row++) {
        uint32_t read = 0U;
        status = fat16_file_read(&file, row_buffer, row_bytes, &read);
        if (status != 0) return status;
        if (read != row_bytes) return -8;
        status = gpt2_gguf_dot_quant_row_buffer(tensor, row_buffer, read,
                                                input, input_channels, &output[row]);
        if (status != 0) return status;
    }
    return 0;
}


int gpt2_gguf_forward_output_logits_fat16(const fat16_volume_t* volume,
                                          const char* filename,
                                          const gpt2_gguf_loaded_model_t* model,
                                          const gpt2_gguf_tensor_t* output_tensor,
                                          uint32_t channels, uint32_t vocabulary,
                                          const float* hidden, uint8_t* row_buffer,
                                          uint32_t row_capacity, float* logits,
                                          uint32_t logits_capacity) {
    if (!volume || !filename || !model || !output_tensor || !hidden ||
        !row_buffer || !logits || channels == 0U || vocabulary == 0U) return -1;
    if (vocabulary > 0xFFFFFFFFU / (uint32_t)sizeof(float)) return -9;
    if (output_tensor->dimensions != 2U || output_tensor->shape[0] != channels ||
        output_tensor->shape[1] != vocabulary ||
        (output_tensor->type != GPT2_GGUF_TENSOR_Q3_K &&
         output_tensor->type != GPT2_GGUF_TENSOR_Q4_K &&
         output_tensor->type != GPT2_GGUF_TENSOR_Q6_K)) return -9;
    fat16_file_t file;
    uint32_t block_bytes;
    uint32_t row_bytes;
    uint32_t offset;
    uint32_t row;
    int status;
    if (logits_capacity < vocabulary * (uint32_t)sizeof(float)) return -6;
    if (output_tensor->type == GPT2_GGUF_TENSOR_Q3_K) block_bytes = GPT2_Q3_K_BLOCK_BYTES;
    else if (output_tensor->type == GPT2_GGUF_TENSOR_Q4_K) block_bytes = GPT2_Q4_K_BLOCK_BYTES;
    else block_bytes = GPT2_Q6_K_BLOCK_BYTES;
    if ((channels % GPT2_QK_K) != 0U ||
        channels / GPT2_QK_K > 0xFFFFFFFFU / block_bytes) return -7;
    row_bytes = (channels / GPT2_QK_K) * block_bytes;
    if (row_bytes == 0U || row_capacity < row_bytes ||
        model->index.info.tensor_data_offset > 0xFFFFFFFFU - output_tensor->data_offset) return -6;
    offset = model->index.info.tensor_data_offset + output_tensor->data_offset;
    status = fat16_open_file(volume, filename, &file);
    if (status != 0) return status;
    if (offset > file.size || output_tensor->byte_size > file.size - offset) return -3;
    status = fat16_file_seek(&file, offset);
    if (status != 0) return status;
    for (row = 0U; row < vocabulary; row++) {
        uint32_t read = 0U;
        status = fat16_file_read(&file, row_buffer, row_bytes, &read);
        if (status != 0) return status;
        if (read != row_bytes) return -8;
        status = gpt2_gguf_dot_quant_row_buffer(output_tensor, row_buffer, read,
                                                hidden, channels, &logits[row]);
        if (status != 0) return status;
    }
    return 0;
}

int gpt2_gguf_forward_output_top_k_fat16(const fat16_volume_t* volume,
                                         const char* filename,
                                         const gpt2_gguf_loaded_model_t* model,
                                         const gpt2_gguf_tensor_t* output_tensor,
                                         uint32_t channels, uint32_t vocabulary,
                                         const float* hidden, uint8_t* row_buffer,
                                         uint32_t row_capacity,
                                         gpt2_sample_top_k_state_t* top_k) {
    fat16_file_t file;
    uint32_t block_bytes;
    uint32_t row_bytes;
    uint32_t offset;
    uint32_t row;
    int status;
    if (!volume || !filename || !model || !output_tensor || !hidden ||
        !row_buffer || !top_k || channels == 0U || vocabulary == 0U) return -1;
    if (output_tensor->dimensions != 2U || output_tensor->shape[0] != channels ||
        output_tensor->shape[1] != vocabulary ||
        (output_tensor->type != GPT2_GGUF_TENSOR_Q3_K &&
         output_tensor->type != GPT2_GGUF_TENSOR_Q4_K &&
         output_tensor->type != GPT2_GGUF_TENSOR_Q6_K)) return -9;
    if (output_tensor->type == GPT2_GGUF_TENSOR_Q3_K) block_bytes = GPT2_Q3_K_BLOCK_BYTES;
    else if (output_tensor->type == GPT2_GGUF_TENSOR_Q4_K) block_bytes = GPT2_Q4_K_BLOCK_BYTES;
    else block_bytes = GPT2_Q6_K_BLOCK_BYTES;
    if ((channels % GPT2_QK_K) != 0U ||
        channels / GPT2_QK_K > 0xFFFFFFFFU / block_bytes) return -7;
    row_bytes = (channels / GPT2_QK_K) * block_bytes;
    if (row_bytes == 0U || row_capacity < row_bytes ||
        model->index.info.tensor_data_offset > 0xFFFFFFFFU - output_tensor->data_offset) return -6;
    offset = model->index.info.tensor_data_offset + output_tensor->data_offset;
    status = fat16_open_file(volume, filename, &file);
    if (status != 0) return status;
    if (offset > file.size || output_tensor->byte_size > file.size - offset) return -3;
    status = fat16_file_seek(&file, offset);
    if (status != 0) return status;
    for (row = 0U; row < vocabulary; row++) {
        float logit = 0.0f;
        uint32_t read = 0U;
        status = fat16_file_read(&file, row_buffer, row_bytes, &read);
        if (status != 0) return status;
        if (read != row_bytes) return -8;
        status = gpt2_gguf_dot_quant_row_buffer(output_tensor, row_buffer, read,
                                                hidden, channels, &logit);
        if (status != 0) return status;
        gpt2_sample_top_k_offer(top_k, row, logit);
    }
    return 0;
}

static int gpt2_gguf_kv_cache_offset(const gpt2_gguf_kv_cache_t* cache,
                                     uint32_t layer, uint32_t position,
                                     uint32_t* out_offset) {
    uint64_t slots;
    uint64_t offset;
    if (!cache || !out_offset || !cache->storage || cache->layers == 0U ||
        cache->max_positions == 0U || cache->channels == 0U ||
        layer >= cache->layers || position >= cache->max_positions) return -9;
    slots = ((uint64_t)layer * cache->max_positions) + position;
    offset = slots * 2ULL * cache->channels;
    if (offset > 0xFFFFFFFFULL || offset + 2ULL * cache->channels > 0xFFFFFFFFULL) return -9;
    *out_offset = (uint32_t)offset;
    return 0;
}

int gpt2_gguf_kv_cache_init(float* storage, uint32_t storage_floats,
                            uint32_t layers, uint32_t max_positions,
                            uint32_t channels, gpt2_gguf_kv_cache_t* out) {
    uint64_t required;
    if (!storage || !out || layers == 0U || max_positions == 0U || channels == 0U) return -1;
    required = (uint64_t)layers * max_positions * 2ULL * channels;
    if (required > 0xFFFFFFFFULL || storage_floats < (uint32_t)required) return -6;
    out->storage = storage;
    out->layers = layers;
    out->max_positions = max_positions;
    out->channels = channels;
    out->count = 0U;
    return 0;
}

int gpt2_gguf_kv_cache_reset(gpt2_gguf_kv_cache_t* cache) {
    if (!cache) return -1;
    cache->count = 0U;
    return 0;
}


int gpt2_gguf_kv_cache_put(gpt2_gguf_kv_cache_t* cache, uint32_t layer,
                           uint32_t position, const float* key, const float* value) {
    uint32_t offset;
    uint32_t i;
    int status;
    if (!key || !value) return -1;
    status = gpt2_gguf_kv_cache_offset(cache, layer, position, &offset);
    if (status != 0) return status;
    for (i = 0U; i < cache->channels; i++) {
        cache->storage[offset + i] = key[i];
        cache->storage[offset + cache->channels + i] = value[i];
    }
    if (position + 1U > cache->count) cache->count = position + 1U;
    return 0;
}

int gpt2_gguf_kv_cache_get(const gpt2_gguf_kv_cache_t* cache, uint32_t layer,
                           uint32_t position, float* key, float* value) {
    uint32_t offset;
    uint32_t i;
    int status;
    if (!key || !value) return -1;
    status = gpt2_gguf_kv_cache_offset(cache, layer, position, &offset);
    if (status != 0) return status;
    for (i = 0U; i < cache->channels; i++) {
        key[i] = cache->storage[offset + i];
        value[i] = cache->storage[offset + cache->channels + i];
    }
    return 0;
}


int gpt2_gguf_kv_cache_copy_history(const gpt2_gguf_kv_cache_t* cache, uint32_t layer,
                                    uint32_t start_position, uint32_t position_count,
                                    float* key_out, uint32_t key_capacity,
                                    float* value_out, uint32_t value_capacity,
                                    uint32_t* out_count) {
    uint32_t position;
    uint32_t copied = 0U;
    int status;
    if (out_count) *out_count = 0U;
    if (!cache || !key_out || !value_out || !out_count) return -1;
    if (position_count == 0U) return 0;
    if (position_count > 0xFFFFFFFFU / cache->channels ||
        key_capacity < position_count * cache->channels ||
        value_capacity < position_count * cache->channels) return -6;
    if (start_position > cache->count || position_count > cache->count - start_position) return -9;
    for (position = 0U; position < position_count; position++) {
        status = gpt2_gguf_kv_cache_get(cache, layer, start_position + position,
                                        key_out + position * cache->channels,
                                        value_out + position * cache->channels);
        if (status != 0) return status;
        copied++;
    }
    *out_count = copied;
    return 0;
}


int gpt2_gguf_kv_cache_query_scores(const gpt2_gguf_kv_cache_t* cache, uint32_t layer,
                                    uint32_t start_position, uint32_t position_count,
                                    const float* query, float* key_scratch,
                                    uint32_t key_scratch_capacity, float* scores,
                                    uint32_t score_capacity, uint32_t* out_count) {
    uint32_t position;
    int status;
    if (out_count) *out_count = 0U;
    if (!cache || !query || !key_scratch || !scores || !out_count) return -1;
    if (key_scratch_capacity < cache->channels || score_capacity < position_count) return -6;
    if (position_count == 0U) return 0;
    if (start_position > cache->count || position_count > cache->count - start_position) return -9;
    for (position = 0U; position < position_count; position++) {
        uint32_t i;
        uint32_t offset;
        float dot = 0.0f;
        status = gpt2_gguf_kv_cache_offset(cache, layer, start_position + position, &offset);
        if (status != 0) return status;
        for (i = 0U; i < cache->channels; i++) {
            key_scratch[i] = cache->storage[offset + i];
            dot += query[i] * key_scratch[i];
        }
        scores[position] = dot;
    }
    *out_count = position_count;
    return 0;
}


int gpt2_gguf_kv_cache_accumulate_values(const gpt2_gguf_kv_cache_t* cache, uint32_t layer,
                                         uint32_t start_position, uint32_t position_count,
                                         const float* weights, uint32_t weight_capacity,
                                         float* output, uint32_t output_capacity,
                                         uint32_t* out_count) {
    uint32_t position;
    uint32_t channel;
    int status;
    if (out_count) *out_count = 0U;
    if (!cache || !weights || !output || !out_count) return -1;
    if (weight_capacity < position_count || output_capacity < cache->channels) return -6;
    if (position_count == 0U) return 0;
    if (start_position > cache->count || position_count > cache->count - start_position) return -9;
    for (channel = 0U; channel < cache->channels; channel++) output[channel] = 0.0f;
    for (position = 0U; position < position_count; position++) {
        uint32_t offset;
        float weight = weights[position];
        status = gpt2_gguf_kv_cache_offset(cache, layer, start_position + position, &offset);
        if (status != 0) return status;
        for (channel = 0U; channel < cache->channels; channel++) {
            output[channel] += weight * cache->storage[offset + cache->channels + channel];
        }
    }
    *out_count = cache->channels;
    return 0;
}


static float gpt2_gguf_attention_inv_sqrt(float value) {
    union { float f; uint32_t u; } convert;
    float half;
    convert.f = value;
    half = 0.5f * value;
    convert.u = 0x5f3759dfU - (convert.u >> 1);
    convert.f = convert.f * (1.5f - half * convert.f * convert.f);
    return convert.f;
}


static float gpt2_gguf_attention_fast_exp(float value) {
    union { float f; uint32_t u; } convert;
    if (value < -80.0f) return 0.0f;
    if (value > 80.0f) value = 80.0f;
    convert.u = (uint32_t)(12102203.0f * value + 1064866805.0f);
    return convert.f;
}


int gpt2_gguf_attention_scale_scores(float* scores, uint32_t score_count, uint32_t head_size) {
    uint32_t i;
    float scale;
    if (!scores || head_size == 0U) return -1;
    scale = gpt2_gguf_attention_inv_sqrt((float)head_size);
    for (i = 0U; i < score_count; i++) scores[i] *= scale;
    return 0;
}


int gpt2_gguf_attention_softmax(float* scores, uint32_t score_count, uint32_t* out_count) {
    uint32_t i;
    float maximum;
    float total = 0.0f;
    if (out_count) *out_count = 0U;
    if (!scores || !out_count) return -1;
    if (score_count == 0U) return 0;
    maximum = scores[0];
    for (i = 1U; i < score_count; i++) {
        if (scores[i] > maximum) maximum = scores[i];
    }
    for (i = 0U; i < score_count; i++) {
        scores[i] = gpt2_gguf_attention_fast_exp(scores[i] - maximum);
        total += scores[i];
    }
    if (total <= 0.0f) return -7;
    for (i = 0U; i < score_count; i++) scores[i] /= total;
    *out_count = score_count;
    return 0;
}


int gpt2_gguf_kv_cache_attention_head(const gpt2_gguf_kv_cache_t* cache, uint32_t layer,
                                      uint32_t start_position, uint32_t position_count,
                                      const float* query, uint32_t head_count,
                                      uint32_t head_index, float* key_scratch,
                                      uint32_t key_scratch_capacity, float* scores,
                                      uint32_t score_capacity, float* output,
                                      uint32_t output_capacity, uint32_t* out_count) {
    uint32_t head_size;
    uint32_t head_base;
    uint32_t position;
    uint32_t channel;
    int status;
    if (out_count) *out_count = 0U;
    if (!cache || !query || !key_scratch || !scores || !output || !out_count) return -1;
    if (head_count == 0U || head_index >= head_count ||
        cache->channels == 0U || (cache->channels % head_count) != 0U) return -9;
    head_size = cache->channels / head_count;
    head_base = head_index * head_size;
    if (key_scratch_capacity < head_size || score_capacity < position_count ||
        output_capacity < head_size) return -6;
    if (position_count == 0U) return 0;
    if (start_position > cache->count || position_count > cache->count - start_position) return -9;
    for (position = 0U; position < position_count; position++) {
        uint32_t offset;
        float dot = 0.0f;
        status = gpt2_gguf_kv_cache_offset(cache, layer, start_position + position, &offset);
        if (status != 0) return status;
        for (channel = 0U; channel < head_size; channel++) {
            key_scratch[channel] = cache->storage[offset + head_base + channel];
            dot += query[channel] * key_scratch[channel];
        }
        scores[position] = dot;
    }
    status = gpt2_gguf_attention_scale_scores(scores, position_count, head_size);
    if (status != 0) return status;
    status = gpt2_gguf_attention_softmax(scores, position_count, out_count);
    if (status != 0) return status;
    for (channel = 0U; channel < head_size; channel++) output[channel] = 0.0f;
    for (position = 0U; position < position_count; position++) {
        uint32_t offset;
        status = gpt2_gguf_kv_cache_offset(cache, layer, start_position + position, &offset);
        if (status != 0) return status;
        for (channel = 0U; channel < head_size; channel++) {
            output[channel] += scores[position] *
                               cache->storage[offset + cache->channels + head_base + channel];
        }
    }
    *out_count = head_size;
    return 0;
}


int gpt2_gguf_attention_concat_heads(const float* head_outputs, uint32_t head_count,
                                      uint32_t head_size, float* output,
                                      uint32_t output_capacity, uint32_t* out_count) {
    uint32_t total;
    uint32_t i;
    if (out_count) *out_count = 0U;
    if (!head_outputs || !output || !out_count) return -1;
    if (head_count == 0U || head_size == 0U) return -9;
    if (head_count > 0xFFFFFFFFU / head_size) return -9;
    total = head_count * head_size;
    if (output_capacity < total) return -6;
    for (i = 0U; i < total; i++) output[i] = head_outputs[i];
    *out_count = total;
    return 0;
}


int gpt2_gguf_kv_cache_attention_multi_head(const gpt2_gguf_kv_cache_t* cache, uint32_t layer,
                                            uint32_t start_position, uint32_t position_count,
                                            const float* query, uint32_t head_count,
                                            float* head_outputs, uint32_t head_output_capacity,
                                            float* key_scratch, uint32_t key_scratch_capacity,
                                            float* scores, uint32_t score_capacity,
                                            float* output, uint32_t output_capacity,
                                            uint32_t* out_count) {
    uint32_t head_size;
    uint32_t head;
    uint32_t channels;
    uint32_t produced;
    int status;
    if (out_count) *out_count = 0U;
    if (!cache || !query || !head_outputs || !key_scratch || !scores || !output || !out_count) return -1;
    if (head_count == 0U || cache->channels == 0U || (cache->channels % head_count) != 0U) return -9;
    channels = cache->channels;
    head_size = channels / head_count;
    if (head_output_capacity < channels || output_capacity < channels ||
        key_scratch_capacity < head_size || score_capacity < position_count) return -6;
    for (head = 0U; head < head_count; head++) {
        status = gpt2_gguf_kv_cache_attention_head(cache, layer, start_position,
                                                    position_count, query, head_count, head,
                                                    key_scratch, key_scratch_capacity, scores,
                                                    score_capacity, head_outputs + head * head_size,
                                                    head_size, &produced);
        if (status != 0) return status;
        if (produced != head_size) return -7;
    }
    status = gpt2_gguf_attention_concat_heads(head_outputs, head_count, head_size,
                                               output, output_capacity, out_count);
    return status;
}


int gpt2_gguf_add_residual(float* residual, uint32_t residual_capacity,
                           const float* attention, uint32_t attention_count) {
    uint32_t i;
    if (!residual || !attention) return -1;
    if (residual_capacity < attention_count) return -6;
    for (i = 0U; i < attention_count; i++) residual[i] += attention[i];
    return 0;
}


int gpt2_gguf_layernorm(const float* input, uint32_t input_count,
                        const float* gamma, const float* beta,
                        float epsilon, float* output, uint32_t output_capacity) {
    uint32_t i;
    float mean = 0.0f;
    float variance = 0.0f;
    float inverse;
    if (!input || !gamma || !beta || !output) return -1;
    if (input_count == 0U || output_capacity < input_count || epsilon <= 0.0f) return -6;
    for (i = 0U; i < input_count; i++) mean += input[i];
    mean /= (float)input_count;
    for (i = 0U; i < input_count; i++) {
        float delta = input[i] - mean;
        variance += delta * delta;
    }
    variance /= (float)input_count;
    inverse = gpt2_gguf_attention_inv_sqrt(variance + epsilon);
    for (i = 0U; i < input_count; i++)
        output[i] = (input[i] - mean) * inverse * gamma[i] + beta[i];
    return 0;
}


int gpt2_gguf_gelu(const float* input, uint32_t input_count,
                   float* output, uint32_t output_capacity) {
    uint32_t i;
    if (!input || !output) return -1;
    if (output_capacity < input_count) return -6;
    for (i = 0U; i < input_count; i++) {
        float value = input[i];
        float exponent = gpt2_gguf_attention_fast_exp(-1.702f * value);
        float sigmoid = 1.0f / (1.0f + exponent);
        output[i] = value * sigmoid;
    }
    return 0;
}


int gpt2_gguf_mlp_forward_fat16(const fat16_volume_t* volume, const char* filename,
                                const gpt2_gguf_loaded_model_t* model,
                                const gpt2_gguf_tensor_t* up_tensor,
                                const gpt2_gguf_tensor_t* down_tensor,
                                uint32_t channels, uint32_t hidden_channels,
                                const float* input, uint8_t* row_buffer,
                                uint32_t row_capacity, const float* up_bias,
                                const float* down_bias, float* hidden,
                                uint32_t hidden_capacity, float* output,
                                uint32_t output_capacity) {
    uint32_t i;
    int status;
    if (!volume || !filename || !model || !up_tensor || !down_tensor ||
        !input || !row_buffer || !hidden || !output) return -1;
    if (channels == 0U || hidden_channels == 0U ||
        channels > 0xFFFFFFFFU / (uint32_t)sizeof(float) ||
        hidden_channels > 0xFFFFFFFFU / (uint32_t)sizeof(float)) return -9;
    if (hidden_capacity < hidden_channels || output_capacity < channels) return -6;
    if (up_tensor->dimensions != 2U || down_tensor->dimensions != 2U ||
        up_tensor->shape[0] != channels || up_tensor->shape[1] != hidden_channels ||
        down_tensor->shape[0] != hidden_channels || down_tensor->shape[1] != channels) return -9;
    status = gpt2_gguf_project_matrix_fat16(volume, filename, model, up_tensor,
                                             channels, hidden_channels, input,
                                             row_buffer, row_capacity, hidden,
                                             hidden_capacity * (uint32_t)sizeof(float));
    if (status != 0) return status;
    if (up_bias) for (i = 0U; i < hidden_channels; i++) hidden[i] += up_bias[i];
    status = gpt2_gguf_gelu(hidden, hidden_channels, hidden, hidden_capacity);
    if (status != 0) return status;
    status = gpt2_gguf_project_matrix_fat16(volume, filename, model, down_tensor,
                                             hidden_channels, channels, hidden,
                                             row_buffer, row_capacity, output,
                                             output_capacity * (uint32_t)sizeof(float));
    if (status != 0) return status;
    if (down_bias) for (i = 0U; i < channels; i++) output[i] += down_bias[i];
    return 0;
}


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
                                             uint32_t residual_capacity) {
    int status;
    if (!residual) return -1;
    if (residual_capacity < channels) return -6;
    status = gpt2_gguf_mlp_forward_fat16(volume, filename, model, up_tensor,
                                         down_tensor, channels, hidden_channels,
                                         input, row_buffer, row_capacity, up_bias,
                                         down_bias, hidden, hidden_capacity, residual,
                                         residual_capacity);
    return status;
}


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
                                      float* residual, uint32_t residual_capacity) {
    int status;
    if (!norm || !gamma || !beta || !input || !residual) return -1;
    if (norm_capacity < channels || residual_capacity < channels) return -6;
    status = gpt2_gguf_layernorm(input, channels, gamma, beta, epsilon,
                                  norm, norm_capacity);
    if (status != 0) return status;
    return gpt2_gguf_mlp_forward_add_residual_fat16(
        volume, filename, model, up_tensor, down_tensor, channels,
        hidden_channels, norm, row_buffer, row_capacity, up_bias, down_bias,
        hidden, hidden_capacity, residual, residual_capacity);
}


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
                                      uint32_t residual_capacity) {
    uint32_t i;
    int status;
    if (!volume || !filename || !model || !output_tensor ||
        !attention_concat || !row_buffer || !projected || !residual) return -1;
    if (channels == 0U || channels > 0xFFFFFFFFU / (uint32_t)sizeof(float)) return -9;
    if (projected_capacity < channels || residual_capacity < channels) return -6;
    if (output_tensor->dimensions != 2U || output_tensor->shape[0] != channels ||
        output_tensor->shape[1] != channels) return -9;
    status = gpt2_gguf_project_matrix_fat16(
        volume, filename, model, output_tensor, channels, channels,
        attention_concat, row_buffer, row_capacity, projected,
        projected_capacity * (uint32_t)sizeof(float));
    if (status != 0) return status;
    if (bias) for (i = 0U; i < channels; i++) projected[i] += bias[i];
    return gpt2_gguf_add_residual(residual, residual_capacity, projected, channels);
}


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
                                      gpt2_gguf_block_workspace_t* workspace) {
    uint32_t position_count;
    uint32_t attention_count = 0U;
    uint32_t i;
    int status;
    if (!cache || !residual || !attention_gamma || !attention_beta || !qkv_tensor ||
        !attention_output_tensor || !ffn_gamma || !ffn_beta || !ffn_up_tensor ||
        !ffn_down_tensor || !volume || !filename || !model || !workspace ||
        !workspace->norm || !workspace->query || !workspace->key || !workspace->value ||
        !workspace->head_outputs || !workspace->key_scratch || !workspace->scores ||
        !workspace->attention || !workspace->projected || !workspace->hidden ||
        !workspace->mlp_output || !workspace->row_buffer) return -1;
    if (channels == 0U || head_count == 0U || hidden_channels == 0U ||
        channels % head_count != 0U || epsilon <= 0.0f ||
        channels > 0xFFFFFFFFU / (uint32_t)sizeof(float)) return -9;
    if (cache->channels != channels || layer >= cache->layers ||
        position >= cache->max_positions || position > cache->count) return -9;
    if (residual_capacity < channels || workspace->norm_capacity < channels ||
        workspace->query_capacity < channels || workspace->key_capacity < channels ||
        workspace->value_capacity < channels || workspace->head_output_capacity < channels ||
        workspace->key_scratch_capacity < channels / head_count ||
        workspace->score_capacity < position + 1U || workspace->attention_capacity < channels ||
        workspace->projected_capacity < channels || workspace->hidden_capacity < hidden_channels ||
        workspace->mlp_output_capacity < channels) return -6;
    if (qkv_tensor->dimensions != 2U || qkv_tensor->shape[0] != channels ||
        qkv_tensor->shape[1] != 3ULL * channels ||
        attention_output_tensor->dimensions != 2U || attention_output_tensor->shape[0] != channels ||
        attention_output_tensor->shape[1] != channels ||
        ffn_up_tensor->dimensions != 2U || ffn_up_tensor->shape[0] != channels ||
        ffn_up_tensor->shape[1] != hidden_channels ||
        ffn_down_tensor->dimensions != 2U || ffn_down_tensor->shape[0] != hidden_channels ||
        ffn_down_tensor->shape[1] != channels) return -9;
    status = gpt2_gguf_layernorm(residual, channels, attention_gamma, attention_beta,
                                  epsilon, workspace->norm, workspace->norm_capacity);
    if (status != 0) return status;
    status = gpt2_gguf_project_qkv_fat16(volume, filename, model, qkv_tensor, channels,
                                          workspace->norm, workspace->row_buffer,
                                          workspace->row_capacity, workspace->query,
                                          workspace->query_capacity * (uint32_t)sizeof(float),
                                          workspace->key, workspace->key_capacity * (uint32_t)sizeof(float),
                                          workspace->value, workspace->value_capacity * (uint32_t)sizeof(float));
    if (status != 0) return status;
    if (qkv_bias) {
        for (i = 0U; i < channels; i++) {
            workspace->query[i] += qkv_bias[i];
            workspace->key[i] += qkv_bias[channels + i];
            workspace->value[i] += qkv_bias[2U * channels + i];
        }
    }
    status = gpt2_gguf_kv_cache_put(cache, layer, position, workspace->key, workspace->value);
    if (status != 0) return status;
    position_count = position + 1U;
    status = gpt2_gguf_kv_cache_attention_multi_head(
        cache, layer, 0U, position_count, workspace->query, head_count,
        workspace->head_outputs, workspace->head_output_capacity,
        workspace->key_scratch, workspace->key_scratch_capacity,
        workspace->scores, workspace->score_capacity, workspace->attention,
        workspace->attention_capacity, &attention_count);
    if (status != 0) return status;
    if (attention_count != channels) return -7;
    status = gpt2_gguf_attention_output_add_residual_fat16(
        volume, filename, model, attention_output_tensor, channels, workspace->attention,
        workspace->row_buffer, workspace->row_capacity, workspace->projected,
        workspace->projected_capacity, attention_output_bias, residual, residual_capacity);
    if (status != 0) return status;
    status = gpt2_gguf_layernorm(residual, channels, ffn_gamma, ffn_beta, epsilon,
                                  workspace->norm, workspace->norm_capacity);
    if (status != 0) return status;
    status = gpt2_gguf_mlp_forward_fat16(
        volume, filename, model, ffn_up_tensor, ffn_down_tensor, channels,
        hidden_channels, workspace->norm, workspace->row_buffer, workspace->row_capacity,
        ffn_up_bias, ffn_down_bias, workspace->hidden, workspace->hidden_capacity,
        workspace->mlp_output, workspace->mlp_output_capacity);
    if (status != 0) return status;
    return gpt2_gguf_add_residual(residual, residual_capacity, workspace->mlp_output, channels);
}

static int gpt2_gguf_generation_token_impl(
                                      const gpt2_gguf_generation_t* generation,
                                      gpt2_gguf_kv_cache_t* cache,
                                      uint32_t token, uint32_t position,
                                      uint32_t head_count, float epsilon,
                                      const fat16_volume_t* volume, const char* filename,
                                      gpt2_gguf_generation_workspace_t* workspace,
                                      float* logits, uint32_t logits_capacity,
                                      gpt2_sample_top_k_state_t* top_k) {
    gpt2_gguf_layer_t layer;
    gpt2_gguf_tensor_t attention_gamma, attention_beta, qkv, qkv_bias;
    gpt2_gguf_tensor_t attention_output, attention_output_bias;
    gpt2_gguf_tensor_t ffn_gamma, ffn_beta, ffn_up, ffn_down;
    gpt2_gguf_tensor_t ffn_up_bias, ffn_down_bias;
    uint32_t channels;
    uint32_t hidden_channels;
    uint32_t layer_index;
    uint32_t i;
    int status;
    if (!generation || !generation->ready || !generation->runtime.ready || !cache ||
        !volume || !filename || !workspace || (!logits && !top_k) ||
        (logits && top_k) || !workspace->dense_scratch ||
        !workspace->position_embedding || !workspace->attention_gamma ||
        !workspace->attention_beta || !workspace->qkv_bias ||
        !workspace->attention_output_bias || !workspace->ffn_gamma ||
        !workspace->ffn_beta || !workspace->ffn_up_bias || !workspace->ffn_down_bias ||
        !workspace->final_gamma || !workspace->final_beta ||
        !workspace->final_hidden) return -1;
    channels = generation->runtime.channels;
    if (channels == 0U || channels > 0x3FFFFFFFU || head_count == 0U ||
        channels % head_count != 0U || epsilon <= 0.0f ||
        token >= generation->vocabulary || position >= generation->max_positions ||
        cache->layers != generation->runtime.layer_count || cache->channels != channels ||
        position >= cache->max_positions || position != cache->count) return -9;
    if (workspace->position_embedding_capacity < channels ||
        workspace->attention_gamma_capacity < channels ||
        workspace->attention_beta_capacity < channels ||
        workspace->qkv_bias_capacity < 3U * channels ||
        workspace->attention_output_bias_capacity < channels ||
        workspace->ffn_gamma_capacity < channels || workspace->ffn_beta_capacity < channels ||
        workspace->ffn_down_bias_capacity < channels ||
        workspace->final_gamma_capacity < channels || workspace->final_beta_capacity < channels ||
        workspace->final_hidden_capacity < channels ||
        (logits && logits_capacity < generation->vocabulary * (uint32_t)sizeof(float))) return -6;
    if (generation->token_embedding.type == GPT2_GGUF_TENSOR_F32 ||
        generation->token_embedding.type == GPT2_GGUF_TENSOR_F16) {
        status = gpt2_gguf_read_dense_row_fat16(volume, filename, generation->runtime.model,
                                                 &generation->token_embedding, token, channels,
                                                 workspace->dense_scratch, workspace->dense_scratch_capacity,
                                                 workspace->final_hidden,
                                                 workspace->final_hidden_capacity * (uint32_t)sizeof(float));
    } else {
        status = gpt2_gguf_read_quant_row_dequant_fat16(volume, filename,
                                                         generation->runtime.model,
                                                         &generation->token_embedding, token,
                                                         workspace->dense_scratch,
                                                         workspace->dense_scratch_capacity,
                                                         workspace->final_hidden,
                                                         workspace->final_hidden_capacity);
    }
    if (status != 0) return status;
    status = gpt2_gguf_read_dense_row_fat16(volume, filename, generation->runtime.model,
                                             &generation->position_embedding, position, channels,
                                             workspace->dense_scratch, workspace->dense_scratch_capacity,
                                             workspace->position_embedding,
                                             workspace->position_embedding_capacity * (uint32_t)sizeof(float));
    if (status != 0) return status;
    for (i = 0U; i < channels; i++) workspace->final_hidden[i] += workspace->position_embedding[i];
    for (layer_index = 0U; layer_index < generation->runtime.layer_count; layer_index++) {
        status = gpt2_gguf_runtime_get_layer(&generation->runtime, layer_index, &layer);
        if (status != 0) return status;
        if (gpt2_gguf_layer_get(&layer, GPT2_GGUF_ROLE_LAYER_ATTN_NORM_WEIGHT, &attention_gamma) != 0 ||
            gpt2_gguf_layer_get(&layer, GPT2_GGUF_ROLE_LAYER_ATTN_NORM_BIAS, &attention_beta) != 0 ||
            gpt2_gguf_layer_get(&layer, GPT2_GGUF_ROLE_LAYER_ATTN_QKV_WEIGHT, &qkv) != 0 ||
            gpt2_gguf_layer_get(&layer, GPT2_GGUF_ROLE_LAYER_ATTN_QKV_BIAS, &qkv_bias) != 0 ||
            gpt2_gguf_layer_get(&layer, GPT2_GGUF_ROLE_LAYER_ATTN_OUTPUT_WEIGHT, &attention_output) != 0 ||
            gpt2_gguf_layer_get(&layer, GPT2_GGUF_ROLE_LAYER_ATTN_OUTPUT_BIAS, &attention_output_bias) != 0 ||
            gpt2_gguf_layer_get(&layer, GPT2_GGUF_ROLE_LAYER_FFN_NORM_WEIGHT, &ffn_gamma) != 0 ||
            gpt2_gguf_layer_get(&layer, GPT2_GGUF_ROLE_LAYER_FFN_NORM_BIAS, &ffn_beta) != 0 ||
            gpt2_gguf_layer_get(&layer, GPT2_GGUF_ROLE_LAYER_FFN_UP_WEIGHT, &ffn_up) != 0 ||
            gpt2_gguf_layer_get(&layer, GPT2_GGUF_ROLE_LAYER_FFN_DOWN_WEIGHT, &ffn_down) != 0 ||
            gpt2_gguf_layer_get(&layer, GPT2_GGUF_ROLE_LAYER_FFN_UP_BIAS, &ffn_up_bias) != 0 ||
            gpt2_gguf_layer_get(&layer, GPT2_GGUF_ROLE_LAYER_FFN_DOWN_BIAS, &ffn_down_bias) != 0) return -8;
        hidden_channels = (uint32_t)ffn_up.shape[1];
        if (hidden_channels == 0U || hidden_channels > 0xFFFFFFFFU / (uint32_t)sizeof(float)) return -9;
        if (workspace->block.hidden_capacity < hidden_channels ||
            workspace->ffn_up_bias_capacity < hidden_channels) return -6;
        status = gpt2_gguf_read_dense_row_fat16(volume, filename, generation->runtime.model,
                                                 &attention_gamma, 0U, channels,
                                                 workspace->dense_scratch, workspace->dense_scratch_capacity,
                                                 workspace->attention_gamma,
                                                 workspace->attention_gamma_capacity * (uint32_t)sizeof(float));
        if (status != 0) return status;
        status = gpt2_gguf_read_dense_row_fat16(volume, filename, generation->runtime.model,
                                                 &attention_beta, 0U, channels,
                                                 workspace->dense_scratch, workspace->dense_scratch_capacity,
                                                 workspace->attention_beta,
                                                 workspace->attention_beta_capacity * (uint32_t)sizeof(float));
        if (status != 0) return status;
        status = gpt2_gguf_read_dense_row_fat16(volume, filename, generation->runtime.model,
                                                 &qkv_bias, 0U, 3U * channels,
                                                 workspace->dense_scratch, workspace->dense_scratch_capacity,
                                                 workspace->qkv_bias, workspace->qkv_bias_capacity * (uint32_t)sizeof(float));
        if (status != 0) return status;
        status = gpt2_gguf_read_dense_row_fat16(volume, filename, generation->runtime.model,
                                                 &attention_output_bias, 0U, channels,
                                                 workspace->dense_scratch, workspace->dense_scratch_capacity,
                                                 workspace->attention_output_bias,
                                                 workspace->attention_output_bias_capacity * (uint32_t)sizeof(float));
        if (status != 0) return status;
        status = gpt2_gguf_read_dense_row_fat16(volume, filename, generation->runtime.model,
                                                 &ffn_gamma, 0U, channels,
                                                 workspace->dense_scratch, workspace->dense_scratch_capacity,
                                                 workspace->ffn_gamma, workspace->ffn_gamma_capacity * (uint32_t)sizeof(float));
        if (status != 0) return status;
        status = gpt2_gguf_read_dense_row_fat16(volume, filename, generation->runtime.model,
                                                 &ffn_beta, 0U, channels,
                                                 workspace->dense_scratch, workspace->dense_scratch_capacity,
                                                 workspace->ffn_beta, workspace->ffn_beta_capacity * (uint32_t)sizeof(float));
        if (status != 0) return status;
        status = gpt2_gguf_read_dense_row_fat16(volume, filename, generation->runtime.model,
                                                 &ffn_up_bias, 0U, hidden_channels,
                                                 workspace->dense_scratch, workspace->dense_scratch_capacity,
                                                 workspace->ffn_up_bias,
                                                 workspace->ffn_up_bias_capacity * (uint32_t)sizeof(float));
        if (status != 0) return status;
        status = gpt2_gguf_read_dense_row_fat16(volume, filename, generation->runtime.model,
                                                 &ffn_down_bias, 0U, channels,
                                                 workspace->dense_scratch, workspace->dense_scratch_capacity,
                                                 workspace->ffn_down_bias,
                                                 workspace->ffn_down_bias_capacity * (uint32_t)sizeof(float));
        if (status != 0) return status;
        status = gpt2_gguf_block_forward_fat16(
            cache, layer_index, position, workspace->final_hidden, workspace->final_hidden_capacity,
            channels, head_count, hidden_channels, workspace->attention_gamma,
            workspace->attention_beta, &qkv, workspace->qkv_bias, &attention_output,
            workspace->attention_output_bias, workspace->ffn_gamma, workspace->ffn_beta,
            &ffn_up, &ffn_down, workspace->ffn_up_bias, workspace->ffn_down_bias,
            epsilon, volume, filename,
            generation->runtime.model, &workspace->block);
        if (status != 0) return status;
    }
    status = gpt2_gguf_read_dense_row_fat16(volume, filename, generation->runtime.model,
                                             &generation->output_norm_weight, 0U, channels,
                                             workspace->dense_scratch, workspace->dense_scratch_capacity,
                                             workspace->final_gamma, workspace->final_gamma_capacity * (uint32_t)sizeof(float));
    if (status != 0) return status;
    status = gpt2_gguf_read_dense_row_fat16(volume, filename, generation->runtime.model,
                                             &generation->output_norm_bias, 0U, channels,
                                             workspace->dense_scratch, workspace->dense_scratch_capacity,
                                             workspace->final_beta, workspace->final_beta_capacity * (uint32_t)sizeof(float));
    if (status != 0) return status;
    status = gpt2_gguf_layernorm(workspace->final_hidden, channels, workspace->final_gamma,
                                  workspace->final_beta, epsilon, workspace->block.norm,
                                  workspace->block.norm_capacity);
    if (status != 0) return status;
    if (top_k) return gpt2_gguf_forward_output_top_k_fat16(
        volume, filename, generation->runtime.model, &generation->output_weight,
        channels, generation->vocabulary, workspace->block.norm,
        workspace->block.row_buffer, workspace->block.row_capacity, top_k);
    return gpt2_gguf_forward_output_logits_fat16(
        volume, filename, generation->runtime.model, &generation->output_weight,
        channels, generation->vocabulary, workspace->block.norm,
        workspace->block.row_buffer, workspace->block.row_capacity, logits, logits_capacity);
}

int gpt2_gguf_generation_token_fat16(
                                      const gpt2_gguf_generation_t* generation,
                                      gpt2_gguf_kv_cache_t* cache,
                                      uint32_t token, uint32_t position,
                                      uint32_t head_count, float epsilon,
                                      const fat16_volume_t* volume, const char* filename,
                                      gpt2_gguf_generation_workspace_t* workspace,
                                      float* logits, uint32_t logits_capacity) {
    return gpt2_gguf_generation_token_impl(generation, cache, token, position, head_count,
                                            epsilon, volume, filename, workspace, logits,
                                            logits_capacity, 0);
}

int gpt2_gguf_generation_token_top_k_fat16(
                                      const gpt2_gguf_generation_t* generation,
                                      gpt2_gguf_kv_cache_t* cache,
                                      uint32_t token, uint32_t position,
                                      uint32_t head_count, float epsilon,
                                      const fat16_volume_t* volume, const char* filename,
                                      gpt2_gguf_generation_workspace_t* workspace,
                                      gpt2_sample_top_k_state_t* top_k) {
    return gpt2_gguf_generation_token_impl(generation, cache, token, position, head_count,
                                            epsilon, volume, filename, workspace, 0, 0U, top_k);
}

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
                                      uint32_t residual_capacity) {
    int status;
    if (!cache || !input || !gamma || !beta || !norm || !head_outputs ||
        !key_scratch || !scores || !attention_concat || !volume || !filename ||
        !model || !output_tensor || !row_buffer || !projected || !residual) return -1;
    if (channels == 0U || head_count == 0U || norm_capacity < channels ||
        attention_capacity < channels || projected_capacity < channels ||
        residual_capacity < channels) return -6;
    status = gpt2_gguf_layernorm(input, channels, gamma, beta, epsilon,
                                  norm, norm_capacity);
    if (status != 0) return status;
    status = gpt2_gguf_kv_cache_attention_multi_head(
        cache, layer, start_position, position_count, norm, head_count,
        head_outputs, head_output_capacity, key_scratch, key_scratch_capacity,
        scores, score_capacity, attention_concat, attention_capacity, 0);
    if (status != 0) return status;
    return gpt2_gguf_attention_output_add_residual_fat16(
        volume, filename, model, output_tensor, channels, attention_concat,
        row_buffer, row_capacity, projected, projected_capacity, bias,
        residual, residual_capacity);
}
