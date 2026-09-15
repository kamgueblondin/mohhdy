#include "gpt2_gguf.h"
#include "gpt2_quant.h"

#define GGUF_DEFAULT_ALIGNMENT 32U
#define GGUF_MAX_TENSORS       512U
#define GGUF_MAX_METADATA      512U
#define GGUF_MAX_ARRAY_ELEMENTS 100000U
#define GGUF_MAX_DIMS          4U
#define GGUF_MAX_STRING        65535U

static int gguf_range(uint32_t offset, uint32_t count, uint32_t size) {
    return offset <= size && count <= size - offset;
}

static int gguf_read_u32(const uint8_t* blob, uint32_t size, uint32_t* offset, uint32_t* out) {
    uint32_t at = *offset;
    if (!gguf_range(at, 4U, size)) return -1;
    *out = (uint32_t)blob[at] |
           ((uint32_t)blob[at + 1U] << 8) |
           ((uint32_t)blob[at + 2U] << 16) |
           ((uint32_t)blob[at + 3U] << 24);
    *offset = at + 4U;
    return 0;
}

static int gguf_read_u64(const uint8_t* blob, uint32_t size, uint32_t* offset, uint64_t* out) {
    uint32_t lo;
    uint32_t hi;
    if (gguf_read_u32(blob, size, offset, &lo) != 0 ||
        gguf_read_u32(blob, size, offset, &hi) != 0) return -1;
    *out = (uint64_t)lo | ((uint64_t)hi << 32);
    return 0;
}

static int gguf_skip(const uint8_t* blob, uint32_t size, uint32_t* offset, uint32_t count) {
    (void)blob;
    if (!gguf_range(*offset, count, size)) return -1;
    *offset += count;
    return 0;
}

static int gguf_read_string(const uint8_t* blob, uint32_t size, uint32_t* offset,
                            const uint8_t** text, uint32_t* length) {
    uint64_t length64;
    if (gguf_read_u64(blob, size, offset, &length64) != 0 ||
        length64 > GGUF_MAX_STRING || length64 > 0xffffffffULL) return -1;
    if (!gguf_range(*offset, (uint32_t)length64, size)) return -1;
    if (text) *text = blob + *offset;
    if (length) *length = (uint32_t)length64;
    *offset += (uint32_t)length64;
    return 0;
}

static int gguf_key_equals(const uint8_t* key, uint32_t key_len, const char* expected) {
    uint32_t i = 0;
    while (expected[i]) {
        if (i >= key_len || key[i] != (uint8_t)expected[i]) return 0;
        i++;
    }
    return i == key_len;
}

static int gguf_value_size(uint32_t type, uint32_t* out) {
    switch (type) {
        case GPT2_GGUF_VALUE_UINT8:
        case GPT2_GGUF_VALUE_INT8:
        case GPT2_GGUF_VALUE_BOOL: *out = 1U; return 0;
        case GPT2_GGUF_VALUE_UINT16:
        case GPT2_GGUF_VALUE_INT16: *out = 2U; return 0;
        case GPT2_GGUF_VALUE_UINT32:
        case GPT2_GGUF_VALUE_INT32:
        case GPT2_GGUF_VALUE_FLOAT32: *out = 4U; return 0;
        case GPT2_GGUF_VALUE_UINT64:
        case GPT2_GGUF_VALUE_INT64:
        case GPT2_GGUF_VALUE_FLOAT64: *out = 8U; return 0;
        default: return -1;
    }
}

static int gguf_skip_value(const uint8_t* blob, uint32_t size, uint32_t* offset,
                           uint32_t type, uint32_t depth) {
    uint32_t item_size;
    uint32_t child_type;
    uint64_t count;
    if (depth > 4U) return -1;
    if (type == GPT2_GGUF_VALUE_STRING) return gguf_read_string(blob, size, offset, 0, 0);
    if (type == GPT2_GGUF_VALUE_ARRAY) {
        if (gguf_read_u32(blob, size, offset, &child_type) != 0 ||
            gguf_read_u64(blob, size, offset, &count) != 0 || count > GGUF_MAX_ARRAY_ELEMENTS) return -1;
        while (count-- > 0U) {
            if (gguf_skip_value(blob, size, offset, child_type, depth + 1U) != 0) return -1;
        }
        return 0;
    }
    if (gguf_value_size(type, &item_size) != 0) return -1;
    return gguf_skip(blob, size, offset, item_size);
}

static int gguf_read_alignment_value(const uint8_t* blob, uint32_t size, uint32_t* offset,
                                     uint32_t type, uint32_t* alignment) {
    uint32_t value;
    if (type != GPT2_GGUF_VALUE_UINT32 || gguf_read_u32(blob, size, offset, &value) != 0) return -1;
    if (value < 8U || (value & 7U) != 0U) return -1;
    *alignment = value;
    return 0;
}

static int gguf_is_gpt2_value(const uint8_t* blob, uint32_t size, uint32_t* offset,
                               uint32_t type, uint8_t* is_gpt2) {
    const uint8_t* text;
    uint32_t length;
    static const char gpt2[] = "gpt2";
    uint32_t i;
    if (type != GPT2_GGUF_VALUE_STRING ||
        gguf_read_string(blob, size, offset, &text, &length) != 0 || length != 4U) return -1;
    for (i = 0; i < 4U; i++) if (text[i] != (uint8_t)gpt2[i]) return -1;
    *is_gpt2 = 1U;
    return 0;
}

static int gguf_align(uint32_t offset, uint32_t alignment, uint32_t* out) {
    uint32_t remainder;
    if (alignment == 0U) return -1;
    remainder = offset % alignment;
    if (remainder != 0U && offset > 0xffffffffU - (alignment - remainder)) return -1;
    *out = remainder ? offset + (alignment - remainder) : offset;
    return 0;
}

int gpt2_gguf_probe_blob(const uint8_t* blob, uint32_t blob_size, gpt2_gguf_info_t* out) {
    uint32_t offset = 0;
    uint32_t magic;
    uint32_t version;
    uint64_t tensor_count64;
    uint64_t metadata_count64;
    uint32_t tensor_count;
    uint32_t metadata_count;
    uint32_t i;
    gpt2_gguf_info_t info;

    if (!blob || !out) return -1;
    info.version = 0U;
    info.tensor_count = 0U;
    info.metadata_count = 0U;
    info.alignment = GGUF_DEFAULT_ALIGNMENT;
    info.tensor_data_offset = 0U;
    info.f32_tensors = 0U;
    info.q8_0_tensors = 0U;
    info.q3_k_tensors = 0U;
    info.q4_k_tensors = 0U;
    info.q6_k_tensors = 0U;
    info.unsupported_quantized_tensors = 0U;
    info.is_gpt2 = 0U;
    info.is_valid = 0U;

    if (gguf_read_u32(blob, blob_size, &offset, &magic) != 0 || magic != GPT2_GGUF_MAGIC ||
        gguf_read_u32(blob, blob_size, &offset, &version) != 0 || version != GPT2_GGUF_VERSION ||
        gguf_read_u64(blob, blob_size, &offset, &tensor_count64) != 0 ||
        gguf_read_u64(blob, blob_size, &offset, &metadata_count64) != 0 ||
        tensor_count64 > GGUF_MAX_TENSORS || metadata_count64 > GGUF_MAX_METADATA) return -2;

    tensor_count = (uint32_t)tensor_count64;
    metadata_count = (uint32_t)metadata_count64;
    info.version = version;
    info.tensor_count = tensor_count;
    info.metadata_count = metadata_count;

    for (i = 0U; i < metadata_count; i++) {
        const uint8_t* key;
        uint32_t key_len;
        uint32_t type;
        if (gguf_read_string(blob, blob_size, &offset, &key, &key_len) != 0 ||
            gguf_read_u32(blob, blob_size, &offset, &type) != 0) return -3;
        if (gguf_key_equals(key, key_len, "general.architecture")) {
            if (gguf_is_gpt2_value(blob, blob_size, &offset, type, &info.is_gpt2) != 0) return -4;
        } else if (gguf_key_equals(key, key_len, "general.alignment")) {
            if (gguf_read_alignment_value(blob, blob_size, &offset, type, &info.alignment) != 0) return -5;
        } else if (gguf_skip_value(blob, blob_size, &offset, type, 0U) != 0) return -6;
    }

    for (i = 0U; i < tensor_count; i++) {
        uint32_t dimensions;
        uint32_t type;
        uint64_t data_offset;
        uint32_t j;
        if (gguf_read_string(blob, blob_size, &offset, 0, 0) != 0 ||
            gguf_read_u32(blob, blob_size, &offset, &dimensions) != 0 ||
            dimensions == 0U || dimensions > GGUF_MAX_DIMS) return -7;
        for (j = 0U; j < dimensions; j++) {
            uint64_t dimension;
            if (gguf_read_u64(blob, blob_size, &offset, &dimension) != 0 || dimension == 0U) return -8;
        }
        if (gguf_read_u32(blob, blob_size, &offset, &type) != 0 ||
            gguf_read_u64(blob, blob_size, &offset, &data_offset) != 0 ||
            data_offset > 0xffffffffULL || ((uint32_t)data_offset % info.alignment) != 0U) return -9;
        if (type == GPT2_GGUF_TENSOR_F32) info.f32_tensors++;
        else if (type == GPT2_GGUF_TENSOR_Q8_0) info.q8_0_tensors++;
        else if (type == GPT2_GGUF_TENSOR_Q3_K) info.q3_k_tensors++;
        else if (type == GPT2_GGUF_TENSOR_Q4_K) info.q4_k_tensors++;
        else if (type == GPT2_GGUF_TENSOR_Q6_K) info.q6_k_tensors++;
        else if (type != GPT2_GGUF_TENSOR_F16) info.unsupported_quantized_tensors++;
    }

    if (!info.is_gpt2 || gguf_align(offset, info.alignment, &info.tensor_data_offset) != 0 ||
        info.tensor_data_offset > blob_size) return -10;
    info.is_valid = 1U;
    *out = info;
    return 0;
}

const char* gpt2_gguf_probe_status(int status) {
    switch (status) {
        case 0: return "GGUF GPT-2 v3 valide (kernels Q3_K/Q4_K/Q6_K disponibles)";
        case -1: return "GGUF: argument invalide";
        case -2: return "GGUF: magic, version ou compte invalide";
        case -3: return "GGUF: metadonnees tronquees";
        case -4: return "GGUF: architecture GPT-2 absente";
        case -5: return "GGUF: alignement invalide";
        case -6: return "GGUF: type de metadonnee invalide";
        case -7: return "GGUF: descripteur de tenseur invalide";
        case -8: return "GGUF: dimension de tenseur invalide";
        case -9: return "GGUF: type ou offset de tenseur invalide";
        default: return "GGUF: donnees invalides";
    }
}


static int gguf_tensor_byte_size(uint32_t type, const uint64_t* shape,
                                  uint32_t dimensions, uint32_t* out) {
    uint64_t elements = 1U;
    uint64_t bytes;
    uint32_t i;
    uint32_t block_size;
    uint32_t block_bytes;

    if (!shape || !out || dimensions == 0U || dimensions > GGUF_MAX_DIMS) return -1;
    for (i = 0U; i < dimensions; i++) {
        uint64_t next;
        if (shape[i] == 0U || shape[i] > 0xffffffffULL) return -1;
        next = elements * shape[i];
        if (next > 0xffffffffULL) return -1;
        elements = next;
    }
    switch (type) {
        case GPT2_GGUF_TENSOR_F32: block_size = 1U; block_bytes = 4U; break;
        case GPT2_GGUF_TENSOR_F16: block_size = 1U; block_bytes = 2U; break;
        case GPT2_GGUF_TENSOR_Q8_0: block_size = GPT2_Q8_0_BLOCK_SIZE; block_bytes = GPT2_Q8_0_BLOCK_BYTES; break;
        case GPT2_GGUF_TENSOR_Q3_K: block_size = GPT2_QK_K; block_bytes = GPT2_Q3_K_BLOCK_BYTES; break;
        case GPT2_GGUF_TENSOR_Q4_K: block_size = GPT2_QK_K; block_bytes = GPT2_Q4_K_BLOCK_BYTES; break;
        case GPT2_GGUF_TENSOR_Q6_K: block_size = GPT2_QK_K; block_bytes = GPT2_Q6_K_BLOCK_BYTES; break;
        default: return -1;
    }
    if (block_size == GPT2_Q8_0_BLOCK_SIZE) {
        if ((elements & (GPT2_Q8_0_BLOCK_SIZE - 1U)) != 0U) return -1;
        bytes = (elements >> 5U) * block_bytes;
    } else if (block_size == GPT2_QK_K) {
        if ((elements & (GPT2_QK_K - 1U)) != 0U) return -1;
        bytes = (elements >> 8U) * block_bytes;
    } else {
        bytes = elements * block_bytes;
    }
    if (bytes > 0xffffffffULL) return -1;
    *out = (uint32_t)bytes;
    return 0;
}

static int gguf_name_equals_cstr(const uint8_t* name, uint32_t length, const char* expected) {
    uint32_t i = 0U;
    if (!name || !expected) return 0;
    while (expected[i]) {
        if (i >= length || name[i] != (uint8_t)expected[i]) return 0;
        i++;
    }
    return i == length;
}

int gpt2_gguf_find_tensor(const uint8_t* blob, uint32_t blob_size,
                          const char* name, gpt2_gguf_tensor_t* out) {
    gpt2_gguf_info_t info;
    uint32_t offset = 0U;
    uint32_t magic;
    uint32_t version;
    uint64_t tensor_count64;
    uint64_t metadata_count64;
    uint32_t tensor_count;
    uint32_t metadata_count;
    uint32_t i;
    int probe_status;

    if (!blob || !name || !out) return -1;
    probe_status = gpt2_gguf_probe_blob(blob, blob_size, &info);
    if (probe_status != 0) return probe_status;
    if (gguf_read_u32(blob, blob_size, &offset, &magic) != 0 ||
        gguf_read_u32(blob, blob_size, &offset, &version) != 0 ||
        gguf_read_u64(blob, blob_size, &offset, &tensor_count64) != 0 ||
        gguf_read_u64(blob, blob_size, &offset, &metadata_count64) != 0) return -2;
    tensor_count = (uint32_t)tensor_count64;
    metadata_count = (uint32_t)metadata_count64;
    for (i = 0U; i < metadata_count; i++) {
        uint32_t key_length;
        uint32_t type;
        if (gguf_read_string(blob, blob_size, &offset, 0, &key_length) != 0 ||
            gguf_read_u32(blob, blob_size, &offset, &type) != 0 ||
            gguf_skip_value(blob, blob_size, &offset, type, 0U) != 0) return -3;
    }
    for (i = 0U; i < tensor_count; i++) {
        const uint8_t* tensor_name;
        uint32_t name_length;
        uint32_t dimensions;
        uint64_t shape[GGUF_MAX_DIMS] = {0U, 0U, 0U, 0U};
        uint32_t type;
        uint64_t data_offset64;
        uint32_t byte_size;
        uint32_t j;
        if (gguf_read_string(blob, blob_size, &offset, &tensor_name, &name_length) != 0 ||
            gguf_read_u32(blob, blob_size, &offset, &dimensions) != 0 ||
            dimensions == 0U || dimensions > GGUF_MAX_DIMS) return -4;
        for (j = 0U; j < dimensions; j++) {
            if (gguf_read_u64(blob, blob_size, &offset, &shape[j]) != 0) return -5;
        }
        if (gguf_read_u32(blob, blob_size, &offset, &type) != 0 ||
            gguf_read_u64(blob, blob_size, &offset, &data_offset64) != 0 ||
            data_offset64 > 0xffffffffULL ||
            gguf_tensor_byte_size(type, shape, dimensions, &byte_size) != 0) return -6;
        if (!gguf_name_equals_cstr(tensor_name, name_length, name)) continue;
        if (data_offset64 > (uint64_t)(blob_size - info.tensor_data_offset) ||
            byte_size > blob_size - info.tensor_data_offset - (uint32_t)data_offset64) return -7;
        out->name = tensor_name;
        out->name_length = name_length;
        out->dimensions = dimensions;
        for (j = 0U; j < GGUF_MAX_DIMS; j++) out->shape[j] = shape[j];
        out->type = type;
        out->data_offset = (uint32_t)data_offset64;
        out->byte_size = byte_size;
        return 0;
    }
    return -8;
}


static int gpt2_gguf_build_index_sized(const uint8_t* blob, uint32_t header_size,
                                         uint32_t file_size, gpt2_gguf_index_t* out) {
    uint32_t offset = 0U;
    uint32_t magic;
    uint32_t version;
    uint64_t tensor_count64;
    uint64_t metadata_count64;
    uint32_t tensor_count;
    uint32_t metadata_count;
    uint32_t i;
    gpt2_gguf_info_t info;

    if (!blob || !out || header_size == 0U || file_size < header_size) return -1;
    info.version = 0U;
    info.tensor_count = 0U;
    info.metadata_count = 0U;
    info.alignment = GGUF_DEFAULT_ALIGNMENT;
    info.tensor_data_offset = 0U;
    info.f32_tensors = 0U;
    info.q8_0_tensors = 0U;
    info.q3_k_tensors = 0U;
    info.q4_k_tensors = 0U;
    info.q6_k_tensors = 0U;
    info.unsupported_quantized_tensors = 0U;
    info.is_gpt2 = 0U;
    info.is_valid = 0U;

    if (gguf_read_u32(blob, header_size, &offset, &magic) != 0 || magic != GPT2_GGUF_MAGIC ||
        gguf_read_u32(blob, header_size, &offset, &version) != 0 || version != GPT2_GGUF_VERSION ||
        gguf_read_u64(blob, header_size, &offset, &tensor_count64) != 0 ||
        gguf_read_u64(blob, header_size, &offset, &metadata_count64) != 0 ||
        tensor_count64 > GPT2_GGUF_MAX_TENSORS || metadata_count64 > GGUF_MAX_METADATA) return -2;
    tensor_count = (uint32_t)tensor_count64;
    metadata_count = (uint32_t)metadata_count64;
    info.version = version;
    info.tensor_count = tensor_count;
    info.metadata_count = metadata_count;

    for (i = 0U; i < metadata_count; i++) {
        const uint8_t* key;
        uint32_t key_length;
        uint32_t type;
        if (gguf_read_string(blob, header_size, &offset, &key, &key_length) != 0 ||
            gguf_read_u32(blob, header_size, &offset, &type) != 0) return -3;
        if (gguf_key_equals(key, key_length, "general.architecture")) {
            if (gguf_is_gpt2_value(blob, header_size, &offset, type, &info.is_gpt2) != 0) return -4;
        } else if (gguf_key_equals(key, key_length, "general.alignment")) {
            if (gguf_read_alignment_value(blob, header_size, &offset, type, &info.alignment) != 0) return -5;
        } else if (gguf_skip_value(blob, header_size, &offset, type, 0U) != 0) return -6;
    }

    for (i = 0U; i < tensor_count; i++) {
        gpt2_gguf_tensor_t* tensor = &out->tensors[i];
        uint64_t data_offset64;
        uint32_t j;
        if (gguf_read_string(blob, header_size, &offset, &tensor->name, &tensor->name_length) != 0 ||
            gguf_read_u32(blob, header_size, &offset, &tensor->dimensions) != 0 ||
            tensor->dimensions == 0U || tensor->dimensions > GGUF_MAX_DIMS) return -7;
        for (j = 0U; j < GGUF_MAX_DIMS; j++) tensor->shape[j] = 0U;
        for (j = 0U; j < tensor->dimensions; j++) {
            if (gguf_read_u64(blob, header_size, &offset, &tensor->shape[j]) != 0 ||
                tensor->shape[j] == 0U) return -8;
        }
        if (gguf_read_u32(blob, header_size, &offset, &tensor->type) != 0 ||
            gguf_read_u64(blob, header_size, &offset, &data_offset64) != 0 ||
            data_offset64 > 0xffffffffULL || ((uint32_t)data_offset64 % info.alignment) != 0U ||
            gguf_tensor_byte_size(tensor->type, tensor->shape, tensor->dimensions, &tensor->byte_size) != 0) return -9;
        tensor->data_offset = (uint32_t)data_offset64;
        if (tensor->type == GPT2_GGUF_TENSOR_F32) info.f32_tensors++;
        else if (tensor->type == GPT2_GGUF_TENSOR_Q8_0) info.q8_0_tensors++;
        else if (tensor->type == GPT2_GGUF_TENSOR_Q3_K) info.q3_k_tensors++;
        else if (tensor->type == GPT2_GGUF_TENSOR_Q4_K) info.q4_k_tensors++;
        else if (tensor->type == GPT2_GGUF_TENSOR_Q6_K) info.q6_k_tensors++;
        else if (tensor->type != GPT2_GGUF_TENSOR_F16) info.unsupported_quantized_tensors++;
    }
    if (!info.is_gpt2 || gguf_align(offset, info.alignment, &info.tensor_data_offset) != 0 ||
        info.tensor_data_offset > file_size) return -10;
    for (i = 0U; i < tensor_count; i++) {
        gpt2_gguf_tensor_t* tensor = &out->tensors[i];
        if (tensor->data_offset > file_size - info.tensor_data_offset ||
            tensor->byte_size > file_size - info.tensor_data_offset - tensor->data_offset) return -11;
    }
    info.is_valid = 1U;
    out->info = info;
    out->blob = blob;
    out->blob_size = file_size;
    out->tensor_count = tensor_count;
    return 0;
}

int gpt2_gguf_build_index(const uint8_t* blob, uint32_t blob_size,
                          gpt2_gguf_index_t* out) {
    return gpt2_gguf_build_index_sized(blob, blob_size, blob_size, out);
}

int gpt2_gguf_build_index_header(const uint8_t* blob, uint32_t header_size,
                                 uint32_t file_size, gpt2_gguf_index_t* out) {
    return gpt2_gguf_build_index_sized(blob, header_size, file_size, out);
}

int gpt2_gguf_index_find(const gpt2_gguf_index_t* index, const char* name,
                         gpt2_gguf_tensor_t* out) {
    uint32_t i;
    if (!index || !name || !out || index->tensor_count > GPT2_GGUF_MAX_TENSORS) return -1;
    for (i = 0U; i < index->tensor_count; i++) {
        if (gguf_name_equals_cstr(index->tensors[i].name, index->tensors[i].name_length, name)) {
            *out = index->tensors[i];
            return 0;
        }
    }
    return -8;
}

int gpt2_gguf_map_role(const gpt2_gguf_index_t* index, gpt2_gguf_role_t role,
                       gpt2_gguf_tensor_t* out) {
    static const char* const names[] = {
        "token_embd.weight",
        "position_embd.weight",
        "output_norm.weight",
        "output_norm.bias",
        "output.weight"
    };
    if ((uint32_t)role >= 5U) return -1;
    return gpt2_gguf_index_find(index, names[(uint32_t)role], out);
}

static int gguf_append_name(char* name, uint32_t capacity, uint32_t* position,
                            const char* suffix) {
    uint32_t i = 0U;
    if (!name || !position || !suffix) return -1;
    while (suffix[i]) {
        if (*position + 1U >= capacity) return -2;
        name[(*position)++] = suffix[i++];
    }
    return 0;
}

int gpt2_gguf_map_layer_role(const gpt2_gguf_index_t* index, uint32_t layer,
                             gpt2_gguf_role_t role, char* name, uint32_t capacity,
                             gpt2_gguf_tensor_t* out) {
    static const char* const suffixes[] = {
        "attn_norm.weight", "attn_norm.bias", "attn_qkv.weight", "attn_qkv.bias",
        "attn_output.weight", "attn_output.bias", "ffn_norm.weight", "ffn_norm.bias",
        "ffn_up.weight", "ffn_down.weight", "ffn_up.bias", "ffn_down.bias"
    };
    uint32_t digits[10];
    uint32_t count = 0U;
    uint32_t position = 0U;
    uint32_t i;
    if (!index || !name || !out || capacity == 0U || (uint32_t)role < 5U ||
        (uint32_t)role >= 17U) return -1;
    if (gguf_append_name(name, capacity, &position, "blk." ) != 0) return -2;
    do {
        digits[count++] = layer % 10U;
        layer /= 10U;
    } while (layer != 0U && count < 10U);
    if (layer != 0U) return -2;
    for (i = count; i > 0U; i--) {
        if (position + 1U >= capacity) return -2;
        name[position++] = (char)('0' + digits[i - 1U]);
    }
    if (position + 1U >= capacity) return -2;
    name[position++] = '.';
    if (gguf_append_name(name, capacity, &position, suffixes[(uint32_t)role - 5U]) != 0) return -2;
    if (position >= capacity) return -2;
    name[position] = '\0';
    return gpt2_gguf_index_find(index, name, out);
}

int gpt2_gguf_map_layer(const gpt2_gguf_index_t* index, uint32_t layer,
                        char* name, uint32_t capacity, gpt2_gguf_layer_t* out) {
    uint32_t i;
    int status;
    if (!index || !name || !out || capacity == 0U) return -1;
    out->layer_index = layer;
    out->present_mask = 0U;
    for (i = 0U; i < 12U; i++) {
        status = gpt2_gguf_map_layer_role(index, layer,
            (gpt2_gguf_role_t)(GPT2_GGUF_ROLE_LAYER_ATTN_NORM_WEIGHT + i),
            name, capacity, &out->tensors[i]);
        if (status != 0) return status;
        out->present_mask |= (1U << i);
    }
    return 0;
}


int gpt2_gguf_layer_get(const gpt2_gguf_layer_t* layer, gpt2_gguf_role_t role,
                        gpt2_gguf_tensor_t* out) {
    uint32_t index;
    if (!layer || !out || (uint32_t)role < GPT2_GGUF_ROLE_LAYER_ATTN_NORM_WEIGHT ||
        (uint32_t)role > GPT2_GGUF_ROLE_LAYER_FFN_DOWN_BIAS) return -1;
    index = (uint32_t)role - GPT2_GGUF_ROLE_LAYER_ATTN_NORM_WEIGHT;
    if ((layer->present_mask & (1U << index)) == 0U) return -8;
    *out = layer->tensors[index];
    return 0;
}


int gpt2_gguf_validate_layer(const gpt2_gguf_layer_t* layer, uint32_t channels) {
    uint32_t i;
    if (!layer) return -1;
    if (layer->present_mask != 0xFFFU) return -8;
    if (channels == 0U || (channels % GPT2_QK_K) != 0U) return -9;
    for (i = 0U; i < 12U; i++) {
        const gpt2_gguf_tensor_t* tensor = &layer->tensors[i];
        if (tensor->dimensions == 0U || tensor->dimensions > 2U ||
            tensor->shape[0] == 0U || tensor->byte_size == 0U) return -9;
        if ((tensor->type == GPT2_GGUF_TENSOR_Q3_K ||
             tensor->type == GPT2_GGUF_TENSOR_Q4_K ||
             tensor->type == GPT2_GGUF_TENSOR_Q6_K) &&
            (tensor->shape[0] % GPT2_QK_K) != 0U) return -9;
    }
    return 0;
}


static int gpt2_gguf_shape_is(const gpt2_gguf_tensor_t* tensor, uint64_t first,
                              uint64_t second) {
    if (!tensor || tensor->dimensions != (second == 0U ? 1U : 2U)) return 0;
    if (tensor->shape[0] != first) return 0;
    if (second != 0U && tensor->shape[1] != second) return 0;
    return 1;
}

int gpt2_gguf_validate_gpt2_layer(const gpt2_gguf_layer_t* layer, uint32_t channels) {
    const gpt2_gguf_tensor_t* t;
    if (!layer || channels == 0U || layer->present_mask != 0xFFFU) return -8;
    if ((channels % GPT2_QK_K) != 0U) return -9;
    t = &layer->tensors[0]; if (!gpt2_gguf_shape_is(t, channels, 0U)) return -9;
    t = &layer->tensors[1]; if (!gpt2_gguf_shape_is(t, channels, 0U)) return -9;
    t = &layer->tensors[2]; if (!gpt2_gguf_shape_is(t, channels, 3U * channels)) return -9;
    t = &layer->tensors[3]; if (!gpt2_gguf_shape_is(t, 3U * channels, 0U)) return -9;
    t = &layer->tensors[4]; if (!gpt2_gguf_shape_is(t, channels, channels)) return -9;
    t = &layer->tensors[5]; if (!gpt2_gguf_shape_is(t, channels, 0U)) return -9;
    t = &layer->tensors[6]; if (!gpt2_gguf_shape_is(t, channels, 0U)) return -9;
    t = &layer->tensors[7]; if (!gpt2_gguf_shape_is(t, channels, 0U)) return -9;
    t = &layer->tensors[8]; if (!gpt2_gguf_shape_is(t, channels, 4U * channels)) return -9;
    t = &layer->tensors[9]; if (!gpt2_gguf_shape_is(t, 4U * channels, channels)) return -9;
    t = &layer->tensors[10]; if (!gpt2_gguf_shape_is(t, 4U * channels, 0U)) return -9;
    t = &layer->tensors[11]; if (!gpt2_gguf_shape_is(t, channels, 0U)) return -9;
    return 0;
}


int gpt2_gguf_validate_tensor_size(const gpt2_gguf_tensor_t* tensor) {
    uint64_t elements = 1U;
    uint64_t bytes;
    uint32_t i;
    uint32_t block_size = 1U;
    uint32_t block_bytes = 0U;
    if (!tensor || tensor->dimensions == 0U || tensor->dimensions > 4U) return -1;
    for (i = 0U; i < tensor->dimensions; i++) {
        uint64_t next;
        if (tensor->shape[i] == 0U || tensor->shape[i] > 0xFFFFFFFFULL) return -9;
        next = elements * tensor->shape[i];
        if (next > 0xFFFFFFFFULL) return -9;
        elements = next;
    }
    switch (tensor->type) {
        case GPT2_GGUF_TENSOR_F32: block_bytes = 4U; break;
        case GPT2_GGUF_TENSOR_F16: block_bytes = 2U; break;
        case GPT2_GGUF_TENSOR_Q3_K: block_size = GPT2_QK_K; block_bytes = GPT2_Q3_K_BLOCK_BYTES; break;
        case GPT2_GGUF_TENSOR_Q4_K: block_size = GPT2_QK_K; block_bytes = GPT2_Q4_K_BLOCK_BYTES; break;
        case GPT2_GGUF_TENSOR_Q6_K: block_size = GPT2_QK_K; block_bytes = GPT2_Q6_K_BLOCK_BYTES; break;
        default: return -4;
    }
    if (block_size != 1U) {
        if ((elements & (block_size - 1U)) != 0U) return -9;
        bytes = (elements >> 8U) * block_bytes;
    } else {
        bytes = elements * block_bytes;
    }
    if (bytes > 0xFFFFFFFFULL || tensor->byte_size != (uint32_t)bytes) return -9;
    return 0;
}


int gpt2_gguf_validate_gpt2_layer_storage(const gpt2_gguf_layer_t* layer, uint32_t channels) {
    uint32_t i;
    int status = gpt2_gguf_validate_gpt2_layer(layer, channels);
    if (status != 0) return status;
    for (i = 0U; i < 12U; i++) {
        status = gpt2_gguf_validate_tensor_size(&layer->tensors[i]);
        if (status != 0) return status;
    }
    return 0;
}
