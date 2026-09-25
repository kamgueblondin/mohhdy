#include <stddef.h>
#ifndef wchar_t
typedef unsigned int wchar_t;
#endif
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

#include <emmintrin.h>

float gpt2_q8_0_dot_f32(const float* input, const uint8_t* q8_blocks, uint32_t count) {
    uint32_t block;
    uint32_t blocks;
    float result = 0.0f;

    if (!input || !q8_blocks || count == 0U || (count % GPT2_Q8_0_BLOCK_SIZE) != 0U) return 0.0f;
    blocks = count / GPT2_Q8_0_BLOCK_SIZE;
    for (block = 0U; block < blocks; block++) {
        const uint8_t* raw = q8_blocks + block * GPT2_Q8_0_BLOCK_BYTES;
        float scale = gpt2_f16_to_f32(gpt2_read_u16(raw));
        const float* in_ptr = input + block * GPT2_Q8_0_BLOCK_SIZE;
        const int8_t* q_ptr = (const int8_t*)(raw + 2U);
        __m128 vsum = _mm_setzero_ps();
        uint32_t i;
        for (i = 0U; i < GPT2_Q8_0_BLOCK_SIZE; i += 4U) {
            __m128 vin = _mm_loadu_ps(in_ptr + i);
            __m128 vq = _mm_set_ps((float)q_ptr[i + 3U], (float)q_ptr[i + 2U], (float)q_ptr[i + 1U], (float)q_ptr[i + 0U]);
            vsum = _mm_add_ps(vsum, _mm_mul_ps(vin, vq));
        }
        vsum = _mm_add_ps(vsum, _mm_shuffle_ps(vsum, vsum, _MM_SHUFFLE(2, 3, 0, 1)));
        vsum = _mm_add_ps(vsum, _mm_shuffle_ps(vsum, vsum, _MM_SHUFFLE(1, 0, 3, 2)));
        float block_sum;
        _mm_store_ss(&block_sum, vsum);
        result += scale * block_sum;
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
        __m128 vsum = _mm_setzero_ps();
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
                __m128 vd0 = _mm_set1_ps(d0);
                __m128 vd1 = _mm_set1_ps(d1);

                const float* in0 = input + base;
                const float* in1 = input + base + 16U;

                for (l = 0U; l < 16U; l += 4U) {
                    uint8_t qb0_0 = raw[qbase + l + 0U];
                    uint8_t qb0_1 = raw[qbase + l + 1U];
                    uint8_t qb0_2 = raw[qbase + l + 2U];
                    uint8_t qb0_3 = raw[qbase + l + 3U];

                    int q0_0 = (int)((qb0_0 >> shift) & 3U) - (((raw[l + 0U] & mask) == 0U) ? 4 : 0);
                    int q0_1 = (int)((qb0_1 >> shift) & 3U) - (((raw[l + 1U] & mask) == 0U) ? 4 : 0);
                    int q0_2 = (int)((qb0_2 >> shift) & 3U) - (((raw[l + 2U] & mask) == 0U) ? 4 : 0);
                    int q0_3 = (int)((qb0_3 >> shift) & 3U) - (((raw[l + 3U] & mask) == 0U) ? 4 : 0);

                    uint8_t qb1_0 = raw[qbase + l + 16U + 0U];
                    uint8_t qb1_1 = raw[qbase + l + 16U + 1U];
                    uint8_t qb1_2 = raw[qbase + l + 16U + 2U];
                    uint8_t qb1_3 = raw[qbase + l + 16U + 3U];

                    int q1_0 = (int)((qb1_0 >> shift) & 3U) - (((raw[l + 16U + 0U] & mask) == 0U) ? 4 : 0);
                    int q1_1 = (int)((qb1_1 >> shift) & 3U) - (((raw[l + 16U + 1U] & mask) == 0U) ? 4 : 0);
                    int q1_2 = (int)((qb1_2 >> shift) & 3U) - (((raw[l + 16U + 2U] & mask) == 0U) ? 4 : 0);
                    int q1_3 = (int)((qb1_3 >> shift) & 3U) - (((raw[l + 16U + 3U] & mask) == 0U) ? 4 : 0);

                    __m128 vq0 = _mm_set_ps((float)q0_3, (float)q0_2, (float)q0_1, (float)q0_0);
                    __m128 vq1 = _mm_set_ps((float)q1_3, (float)q1_2, (float)q1_1, (float)q1_0);

                    vsum = _mm_add_ps(vsum, _mm_mul_ps(_mm_mul_ps(vd0, vq0), _mm_loadu_ps(in0 + l)));
                    vsum = _mm_add_ps(vsum, _mm_mul_ps(_mm_mul_ps(vd1, vq1), _mm_loadu_ps(in1 + l)));
                }
            }
        }
        vsum = _mm_add_ps(vsum, _mm_shuffle_ps(vsum, vsum, _MM_SHUFFLE(2, 3, 0, 1)));
        vsum = _mm_add_ps(vsum, _mm_shuffle_ps(vsum, vsum, _MM_SHUFFLE(1, 0, 3, 2)));
        float block_sum;
        _mm_store_ss(&block_sum, vsum);
        result += block_sum;
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
        __m128 vsum = _mm_setzero_ps();
        for (segment = 0U; segment < GPT2_QK_K; segment += 64U) {
            uint32_t l;
            uint8_t scale0, scale1, min0, min1;
            gpt2_q4_k_scale_min(raw + 4U, segment / 32U, &scale0, &min0);
            gpt2_q4_k_scale_min(raw + 4U, segment / 32U + 1U, &scale1, &min1);
            float scale_value0 = d * (float)scale0;
            float scale_value1 = d * (float)scale1;
            float min_value0 = minimum * (float)min0;
            float min_value1 = minimum * (float)min1;
            __m128 vscale0 = _mm_set1_ps(scale_value0);
            __m128 vscale1 = _mm_set1_ps(scale_value1);
            __m128 vmin0 = _mm_set1_ps(min_value0);
            __m128 vmin1 = _mm_set1_ps(min_value1);

            const float* in_ptr0 = input + block * GPT2_QK_K + segment;
            const float* in_ptr1 = in_ptr0 + 32U;

            for (l = 0U; l < 32U; l += 4U) {
                uint8_t p0 = raw[16U + segment / 2U + l + 0U];
                uint8_t p1 = raw[16U + segment / 2U + l + 1U];
                uint8_t p2 = raw[16U + segment / 2U + l + 2U];
                uint8_t p3 = raw[16U + segment / 2U + l + 3U];

                __m128 vq0 = _mm_set_ps((float)(p3 & 0x0fU), (float)(p2 & 0x0fU), (float)(p1 & 0x0fU), (float)(p0 & 0x0fU));
                __m128 vq1 = _mm_set_ps((float)(p3 >> 4), (float)(p2 >> 4), (float)(p1 >> 4), (float)(p0 >> 4));

                __m128 vw0 = _mm_sub_ps(_mm_mul_ps(vscale0, vq0), vmin0);
                __m128 vw1 = _mm_sub_ps(_mm_mul_ps(vscale1, vq1), vmin1);

                __m128 vin0 = _mm_loadu_ps(in_ptr0 + l);
                __m128 vin1 = _mm_loadu_ps(in_ptr1 + l);

                vsum = _mm_add_ps(vsum, _mm_mul_ps(vw0, vin0));
                vsum = _mm_add_ps(vsum, _mm_mul_ps(vw1, vin1));
            }
        }
        vsum = _mm_add_ps(vsum, _mm_shuffle_ps(vsum, vsum, _MM_SHUFFLE(2, 3, 0, 1)));
        vsum = _mm_add_ps(vsum, _mm_shuffle_ps(vsum, vsum, _MM_SHUFFLE(1, 0, 3, 2)));
        float block_sum;
        _mm_store_ss(&block_sum, vsum);
        result += block_sum;
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
        __m128 vsum = _mm_setzero_ps();
        for (n = 0U; n < GPT2_QK_K; n += 128U) {
            uint32_t l;
            uint32_t ql_base = n / 2U;
            uint32_t qh_base = 128U + n / 4U;
            uint32_t scale_base = 192U + n / 16U;
            float scale1 = d * (float)(int8_t)raw[scale_base];
            float scale2 = d * (float)(int8_t)raw[scale_base + 2U];
            float scale3 = d * (float)(int8_t)raw[scale_base + 4U];
            float scale4 = d * (float)(int8_t)raw[scale_base + 6U];
            __m128 vscale1 = _mm_set1_ps(scale1);
            __m128 vscale2 = _mm_set1_ps(scale2);
            __m128 vscale3 = _mm_set1_ps(scale3);
            __m128 vscale4 = _mm_set1_ps(scale4);

            const float* in1 = input + block * GPT2_QK_K + n;
            const float* in2 = in1 + 32U;
            const float* in3 = in1 + 64U;
            const float* in4 = in1 + 96U;

            for (l = 0U; l < 32U; l += 4U) {
                uint8_t qh0 = raw[qh_base + l + 0U];
                uint8_t qh1 = raw[qh_base + l + 1U];
                uint8_t qh2 = raw[qh_base + l + 2U];
                uint8_t qh3 = raw[qh_base + l + 3U];

                int q1_0 = (int)((raw[ql_base + l + 0U] & 0x0fU) | ((qh0 & 3U) << 4)) - 32;
                int q1_1 = (int)((raw[ql_base + l + 1U] & 0x0fU) | ((qh1 & 3U) << 4)) - 32;
                int q1_2 = (int)((raw[ql_base + l + 2U] & 0x0fU) | ((qh2 & 3U) << 4)) - 32;
                int q1_3 = (int)((raw[ql_base + l + 3U] & 0x0fU) | ((qh3 & 3U) << 4)) - 32;

                int q2_0 = (int)((raw[ql_base + l + 0U + 32U] & 0x0fU) | (((qh0 >> 2) & 3U) << 4)) - 32;
                int q2_1 = (int)((raw[ql_base + l + 1U + 32U] & 0x0fU) | (((qh1 >> 2) & 3U) << 4)) - 32;
                int q2_2 = (int)((raw[ql_base + l + 2U + 32U] & 0x0fU) | (((qh2 >> 2) & 3U) << 4)) - 32;
                int q2_3 = (int)((raw[ql_base + l + 3U + 32U] & 0x0fU) | (((qh3 >> 2) & 3U) << 4)) - 32;

                int q3_0 = (int)((raw[ql_base + l + 0U] >> 4) | (((qh0 >> 4) & 3U) << 4)) - 32;
                int q3_1 = (int)((raw[ql_base + l + 1U] >> 4) | (((qh1 >> 4) & 3U) << 4)) - 32;
                int q3_2 = (int)((raw[ql_base + l + 2U] >> 4) | (((qh2 >> 4) & 3U) << 4)) - 32;
                int q3_3 = (int)((raw[ql_base + l + 3U] >> 4) | (((qh3 >> 4) & 3U) << 4)) - 32;

                int q4_0 = (int)((raw[ql_base + l + 0U + 32U] >> 4) | (((qh0 >> 6) & 3U) << 4)) - 32;
                int q4_1 = (int)((raw[ql_base + l + 1U + 32U] >> 4) | (((qh1 >> 6) & 3U) << 4)) - 32;
                int q4_2 = (int)((raw[ql_base + l + 2U + 32U] >> 4) | (((qh2 >> 6) & 3U) << 4)) - 32;
                int q4_3 = (int)((raw[ql_base + l + 3U + 32U] >> 4) | (((qh3 >> 6) & 3U) << 4)) - 32;

                __m128 vq1 = _mm_set_ps((float)q1_3, (float)q1_2, (float)q1_1, (float)q1_0);
                __m128 vq2 = _mm_set_ps((float)q2_3, (float)q2_2, (float)q2_1, (float)q2_0);
                __m128 vq3 = _mm_set_ps((float)q3_3, (float)q3_2, (float)q3_1, (float)q3_0);
                __m128 vq4 = _mm_set_ps((float)q4_3, (float)q4_2, (float)q4_1, (float)q4_0);

                vsum = _mm_add_ps(vsum, _mm_mul_ps(_mm_mul_ps(vscale1, vq1), _mm_loadu_ps(in1 + l)));
                vsum = _mm_add_ps(vsum, _mm_mul_ps(_mm_mul_ps(vscale2, vq2), _mm_loadu_ps(in2 + l)));
                vsum = _mm_add_ps(vsum, _mm_mul_ps(_mm_mul_ps(vscale3, vq3), _mm_loadu_ps(in3 + l)));
                vsum = _mm_add_ps(vsum, _mm_mul_ps(_mm_mul_ps(vscale4, vq4), _mm_loadu_ps(in4 + l)));
            }
        }
        vsum = _mm_add_ps(vsum, _mm_shuffle_ps(vsum, vsum, _MM_SHUFFLE(2, 3, 0, 1)));
        vsum = _mm_add_ps(vsum, _mm_shuffle_ps(vsum, vsum, _MM_SHUFFLE(1, 0, 3, 2)));
        float block_sum;
        _mm_store_ss(&block_sum, vsum);
        result += block_sum;
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
