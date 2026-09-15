#include "gpt2_quant.h"

static uint16_t gpt2_read_u16(const uint8_t* data) {
    return (uint16_t)data[0] | ((uint16_t)data[1] << 8);
}

float gpt2_f16_to_f32(uint16_t bits) {
    union { uint32_t u; float f; } out;
    uint32_t sign = ((uint32_t)bits & 0x8000U) << 16;
    uint32_t exponent = ((uint32_t)bits >> 10) & 0x1fU;
    uint32_t fraction = (uint32_t)bits & 0x03ffU;

    if (exponent == 0U) {
        if (fraction == 0U) {
            out.u = sign;
            return out.f;
        }
        while ((fraction & 0x0400U) == 0U) {
            fraction <<= 1;
            exponent--;
        }
        fraction &= 0x03ffU;
        exponent = 1U;
    } else if (exponent == 31U) {
        out.u = sign | 0x7f800000U | (fraction << 13);
        return out.f;
    }

    exponent = exponent + (127U - 15U);
    out.u = sign | (exponent << 23) | (fraction << 13);
    return out.f;
}

float gpt2_q8_0_dot_f32(const float* input, const uint8_t* q8_blocks, uint32_t count) {
    uint32_t block;
    uint32_t blocks;
    float result = 0.0f;

    if (!input || !q8_blocks || count == 0U || (count % GPT2_Q8_0_BLOCK_SIZE) != 0U) return 0.0f;
    blocks = count / GPT2_Q8_0_BLOCK_SIZE;
    for (block = 0U; block < blocks; block++) {
        const uint8_t* raw = q8_blocks + block * GPT2_Q8_0_BLOCK_BYTES;
        float scale = gpt2_f16_to_f32(gpt2_read_u16(raw));
        float sum0 = 0.0f, sum1 = 0.0f, sum2 = 0.0f, sum3 = 0.0f;
        const float* in_ptr = input + block * GPT2_Q8_0_BLOCK_SIZE;
        const int8_t* q_ptr = (const int8_t*)(raw + 2U);
        uint32_t i;
        for (i = 0U; i < GPT2_Q8_0_BLOCK_SIZE; i += 4U) {
            sum0 += in_ptr[i + 0U] * (float)q_ptr[i + 0U];
            sum1 += in_ptr[i + 1U] * (float)q_ptr[i + 1U];
            sum2 += in_ptr[i + 2U] * (float)q_ptr[i + 2U];
            sum3 += in_ptr[i + 3U] * (float)q_ptr[i + 3U];
        }
        result += scale * ((sum0 + sum1) + (sum2 + sum3));
    }
    return result;
}

float gpt2_q3_k_dot_f32(const float* input, const uint8_t* q3_blocks, uint32_t count) {
    uint32_t block;
    float result = 0.0f;
    if (!input || !q3_blocks || count == 0U || (count % GPT2_QK_K) != 0U) return 0.0f;

    for (block = 0U; block < count / GPT2_QK_K; block++) {
        const uint8_t* raw = q3_blocks + block * GPT2_Q3_K_BLOCK_BYTES;
        const float d = gpt2_f16_to_f32(gpt2_read_u16(raw + 108U));
        int8_t scales[16];
        uint32_t i;
        uint32_t n;
        for (i = 0U; i < 4U; i++) {
            scales[i]      = (int8_t)((raw[96U + i] & 0x0fU) | ((raw[104U + i] & 3U) << 4));
            scales[4U + i] = (int8_t)((raw[100U + i] & 0x0fU) | (((raw[104U + i] >> 2) & 3U) << 4));
            scales[8U + i] = (int8_t)(((raw[96U + i] >> 4) & 0x0fU) | (((raw[104U + i] >> 4) & 3U) << 4));
            scales[12U + i] = (int8_t)(((raw[100U + i] >> 4) & 0x0fU) | (((raw[104U + i] >> 6) & 3U) << 4));
        }
        for (n = 0U; n < GPT2_QK_K; n += 128U) {
            uint32_t j;
            uint32_t qbase = 32U + n / 4U;
            uint32_t mask_base = n == 0U ? 0U : 4U;
            for (j = 0U; j < 4U; j++) {
                uint32_t l;
                uint32_t base = block * GPT2_QK_K + n + j * 32U;
                uint32_t shift = j * 2U;
                uint32_t mask = 1U << (mask_base + j);
                float d0 = d * (float)(scales[2U * j] - 32);
                float d1 = d * (float)(scales[2U * j + 1U] - 32);
                for (l = 0U; l < 16U; l++) {
                    int q0 = (int)((raw[qbase + l] >> shift) & 3U) -
                             (((raw[l] & mask) == 0U) ? 4 : 0);
                    int q1 = (int)((raw[qbase + l + 16U] >> shift) & 3U) -
                             (((raw[l + 16U] & mask) == 0U) ? 4 : 0);
                    result += d0 * (float)q0 * input[base + l];
                    result += d1 * (float)q1 * input[base + 16U + l];
                }
            }
        }
    }
    return result;
}

static void gpt2_q4_k_scale_min(const uint8_t* scales, uint32_t index,
                                uint8_t* scale, uint8_t* minimum) {
    if (index < 4U) {
        *scale = scales[index] & 63U;
        *minimum = scales[index + 4U] & 63U;
    } else {
        *scale = (uint8_t)((scales[index + 4U] & 0x0fU) |
                           ((scales[index - 4U] >> 6) << 4));
        *minimum = (uint8_t)((scales[index + 4U] >> 4) |
                             ((scales[index] >> 6) << 4));
    }
}

float gpt2_q4_k_dot_f32(const float* input, const uint8_t* q4_blocks, uint32_t count) {
    uint32_t block;
    float result = 0.0f;
    if (!input || !q4_blocks || count == 0U || (count % GPT2_QK_K) != 0U) return 0.0f;

    for (block = 0U; block < count / GPT2_QK_K; block++) {
        const uint8_t* raw = q4_blocks + block * GPT2_Q4_K_BLOCK_BYTES;
        const float d = gpt2_f16_to_f32(gpt2_read_u16(raw));
        const float minimum = gpt2_f16_to_f32(gpt2_read_u16(raw + 2U));
        uint32_t segment;
        for (segment = 0U; segment < GPT2_QK_K; segment += 64U) {
            uint32_t l;
            uint8_t scale0;
            uint8_t scale1;
            uint8_t min0;
            uint8_t min1;
            float scale_value0;
            float scale_value1;
            float min_value0;
            float min_value1;
            gpt2_q4_k_scale_min(raw + 4U, segment / 32U, &scale0, &min0);
            gpt2_q4_k_scale_min(raw + 4U, segment / 32U + 1U, &scale1, &min1);
            scale_value0 = d * (float)scale0;
            scale_value1 = d * (float)scale1;
            min_value0 = minimum * (float)min0;
            min_value1 = minimum * (float)min1;
            for (l = 0U; l < 32U; l++) {
                uint8_t packed = raw[16U + segment / 2U + l];
                result += (scale_value0 * (float)(packed & 0x0fU) - min_value0) *
                          input[block * GPT2_QK_K + segment + l];
                result += (scale_value1 * (float)(packed >> 4) - min_value1) *
                          input[block * GPT2_QK_K + segment + 32U + l];
            }
        }
    }
    return result;
}

float gpt2_q6_k_dot_f32(const float* input, const uint8_t* q6_blocks, uint32_t count) {
    uint32_t block;
    float result = 0.0f;
    if (!input || !q6_blocks || count == 0U || (count % GPT2_QK_K) != 0U) return 0.0f;

    for (block = 0U; block < count / GPT2_QK_K; block++) {
        const uint8_t* raw = q6_blocks + block * GPT2_Q6_K_BLOCK_BYTES;
        const float d = gpt2_f16_to_f32(gpt2_read_u16(raw + 208U));
        uint32_t n;
        for (n = 0U; n < GPT2_QK_K; n += 128U) {
            uint32_t l;
            uint32_t ql_base = n / 2U;
            uint32_t qh_base = 128U + n / 4U;
            uint32_t scale_base = 192U + n / 16U;
            float scale1 = d * (float)(int8_t)raw[scale_base];
            float scale2 = d * (float)(int8_t)raw[scale_base + 2U];
            float scale3 = d * (float)(int8_t)raw[scale_base + 4U];
            float scale4 = d * (float)(int8_t)raw[scale_base + 6U];
            for (l = 0U; l < 32U; l++) {
                const uint8_t qh = raw[qh_base + l];
                int q1 = (int)((raw[ql_base + l] & 0x0fU) | ((qh & 3U) << 4)) - 32;
                int q2 = (int)((raw[ql_base + l + 32U] & 0x0fU) | (((qh >> 2) & 3U) << 4)) - 32;
                int q3 = (int)((raw[ql_base + l] >> 4) | (((qh >> 4) & 3U) << 4)) - 32;
                int q4 = (int)((raw[ql_base + l + 32U] >> 4) | (((qh >> 6) & 3U) << 4)) - 32;
                result += scale1 * (float)q1 * input[block * GPT2_QK_K + n + l];
                result += scale2 * (float)q2 * input[block * GPT2_QK_K + n + 32U + l];
                result += scale3 * (float)q3 * input[block * GPT2_QK_K + n + 64U + l];
                result += scale4 * (float)q4 * input[block * GPT2_QK_K + n + 96U + l];
            }
        }
    }
    return result;
}

int gpt2_q3_k_dequantize(const uint8_t* q3_blocks, uint32_t count, float* output) {
    uint32_t block;
    if (!q3_blocks || !output || count == 0U || (count % GPT2_QK_K) != 0U) return -1;
    for (block = 0U; block < count / GPT2_QK_K; block++) {
        const uint8_t* raw = q3_blocks + block * GPT2_Q3_K_BLOCK_BYTES;
        const float d = gpt2_f16_to_f32(gpt2_read_u16(raw + 108U));
        int8_t scales[16];
        uint32_t i;
        uint32_t n;
        for (i = 0U; i < 4U; i++) {
            scales[i] = (int8_t)((raw[96U + i] & 0x0fU) | ((raw[104U + i] & 3U) << 4));
            scales[4U + i] = (int8_t)((raw[100U + i] & 0x0fU) | (((raw[104U + i] >> 2) & 3U) << 4));
            scales[8U + i] = (int8_t)(((raw[96U + i] >> 4) & 0x0fU) | (((raw[104U + i] >> 4) & 3U) << 4));
            scales[12U + i] = (int8_t)(((raw[100U + i] >> 4) & 0x0fU) | (((raw[104U + i] >> 6) & 3U) << 4));
        }
        for (n = 0U; n < GPT2_QK_K; n += 128U) {
            uint32_t j;
            uint32_t qbase = 32U + n / 4U;
            uint32_t mask_base = n == 0U ? 0U : 4U;
            for (j = 0U; j < 4U; j++) {
                uint32_t l;
                uint32_t base = block * GPT2_QK_K + n + j * 32U;
                uint32_t shift = j * 2U;
                uint32_t mask = 1U << (mask_base + j);
                float d0 = d * (float)(scales[2U * j] - 32);
                float d1 = d * (float)(scales[2U * j + 1U] - 32);
                for (l = 0U; l < 16U; l++) {
                    int q0 = (int)((raw[qbase + l] >> shift) & 3U);
                    int q1 = (int)((raw[qbase + l + 16U] >> shift) & 3U);
                    if ((raw[l] & mask) == 0U) q0 -= 4;
                    if ((raw[l + 16U] & mask) == 0U) q1 -= 4;
                    output[base + l] = d0 * (float)q0;
                    output[base + 16U + l] = d1 * (float)q1;
                }
            }
        }
    }
    return 0;
}

int gpt2_q4_k_dequantize(const uint8_t* q4_blocks, uint32_t count, float* output) {
    uint32_t block;
    if (!q4_blocks || !output || count == 0U || (count % GPT2_QK_K) != 0U) return -1;
    for (block = 0U; block < count / GPT2_QK_K; block++) {
        const uint8_t* raw = q4_blocks + block * GPT2_Q4_K_BLOCK_BYTES;
        const float d = gpt2_f16_to_f32(gpt2_read_u16(raw));
        const float minimum = gpt2_f16_to_f32(gpt2_read_u16(raw + 2U));
        uint32_t segment;
        for (segment = 0U; segment < GPT2_QK_K; segment += 64U) {
            uint32_t l;
            uint8_t scale0;
            uint8_t scale1;
            uint8_t min0;
            uint8_t min1;
            uint32_t base = block * GPT2_QK_K + segment;
            gpt2_q4_k_scale_min(raw + 4U, segment / 32U, &scale0, &min0);
            gpt2_q4_k_scale_min(raw + 4U, segment / 32U + 1U, &scale1, &min1);
            for (l = 0U; l < 32U; l++) {
                uint8_t packed = raw[16U + segment / 2U + l];
                output[base + l] = d * (float)scale0 * (float)(packed & 0x0fU) - minimum * (float)min0;
                output[base + 32U + l] = d * (float)scale1 * (float)(packed >> 4) - minimum * (float)min1;
            }
        }
    }
    return 0;
}

int gpt2_q6_k_dequantize(const uint8_t* q6_blocks, uint32_t count, float* output) {
    uint32_t block;
    if (!q6_blocks || !output || count == 0U || (count % GPT2_QK_K) != 0U) return -1;
    for (block = 0U; block < count / GPT2_QK_K; block++) {
        const uint8_t* raw = q6_blocks + block * GPT2_Q6_K_BLOCK_BYTES;
        const float d = gpt2_f16_to_f32(gpt2_read_u16(raw + 208U));
        uint32_t n;
        for (n = 0U; n < GPT2_QK_K; n += 128U) {
            uint32_t l;
            uint32_t ql_base = n / 2U;
            uint32_t qh_base = 128U + n / 4U;
            uint32_t scale_base = 192U + n / 16U;
            uint32_t base = block * GPT2_QK_K + n;
            for (l = 0U; l < 32U; l++) {
                const uint8_t qh = raw[qh_base + l];
                int q1 = (int)((raw[ql_base + l] & 0x0fU) | ((qh & 3U) << 4)) - 32;
                int q2 = (int)((raw[ql_base + l + 32U] & 0x0fU) | (((qh >> 2) & 3U) << 4)) - 32;
                int q3 = (int)((raw[ql_base + l] >> 4) | (((qh >> 4) & 3U) << 4)) - 32;
                int q4 = (int)((raw[ql_base + l + 32U] >> 4) | (((qh >> 6) & 3U) << 4)) - 32;
                output[base + l] = d * (float)(int8_t)raw[scale_base] * (float)q1;
                output[base + 32U + l] = d * (float)(int8_t)raw[scale_base + 2U] * (float)q2;
                output[base + 64U + l] = d * (float)(int8_t)raw[scale_base + 4U] * (float)q3;
                output[base + 96U + l] = d * (float)(int8_t)raw[scale_base + 6U] * (float)q4;
            }
        }
    }
    return 0;
}
