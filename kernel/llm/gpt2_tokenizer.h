#ifndef MOHHDY_GPT2_TOKENIZER_H
#define MOHHDY_GPT2_TOKENIZER_H

#include <stdint.h>

/* Tokenizer llm.c (magic 20240328) : vocabulaire en octets bruts tiktoken. */
int gpt2_tokenizer_load_from_buffer(const uint8_t* blob, uint32_t blob_size);
int gpt2_tokenizer_load_from_initrd(const char* path);
/* BPE GPT-2 (fusions par plus petit id, decoupage type regex). */
int gpt2_tokenizer_encode(const char* text, uint32_t* out_tokens,
                          uint32_t max_tokens, uint32_t* out_count);
int gpt2_tokenizer_encode_ascii(const char* text, uint32_t* out_tokens,
                                uint32_t max_tokens, uint32_t* out_count);
const char* gpt2_tokenizer_decode(uint32_t token_id);
uint32_t gpt2_tokenizer_eot(void);
const char* gpt2_tokenizer_status(void);

#endif
