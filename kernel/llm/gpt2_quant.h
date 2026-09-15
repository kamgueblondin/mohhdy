#ifndef MOHHDY_GPT2_QUANT_H
#define MOHHDY_GPT2_QUANT_H

#include <stdint.h>

/* GGML Q8_0: one IEEE-754 binary16 scale and 32 signed quantized values. */
#define GPT2_Q8_0_BLOCK_SIZE 32U
#define GPT2_Q8_0_BLOCK_BYTES 34U

/* GGML K-quants use 256 values per super-block. */
#define GPT2_QK_K 256U
#define GPT2_Q3_K_BLOCK_BYTES 110U
#define GPT2_Q4_K_BLOCK_BYTES 144U
#define GPT2_Q6_K_BLOCK_BYTES 210U

/* Convert an IEEE-754 binary16 bit pattern to FP32 without a libc dependency. */
float gpt2_f16_to_f32(uint16_t bits);

/*
 * Dot product of a FP32 activation vector and a contiguous GGML Q8_0 row.
 * `count` must be a non-zero multiple of 32. Returns 0.0f for invalid input.
 */
float gpt2_q8_0_dot_f32(const float* input, const uint8_t* q8_blocks, uint32_t count);

/* Dot products for the GGML Q3_K, Q4_K and Q6_K super-block layouts. */
float gpt2_q3_k_dot_f32(const float* input, const uint8_t* q3_blocks, uint32_t count);
float gpt2_q4_k_dot_f32(const float* input, const uint8_t* q4_blocks, uint32_t count);
float gpt2_q6_k_dot_f32(const float* input, const uint8_t* q6_blocks, uint32_t count);
/* Déquantifie des super-blocs K vers un buffer float caller-owned. `count` doit
 * être un multiple non nul de 256 ; 0 signifie succès. */
int gpt2_q3_k_dequantize(const uint8_t* q3_blocks, uint32_t count, float* output);
int gpt2_q4_k_dequantize(const uint8_t* q4_blocks, uint32_t count, float* output);
int gpt2_q6_k_dequantize(const uint8_t* q6_blocks, uint32_t count, float* output);

#endif
