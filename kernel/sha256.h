#ifndef MOHHDY_SHA256_H
#define MOHHDY_SHA256_H
#include <stdint.h>
typedef struct { uint32_t state[8]; uint64_t bit_count; uint8_t block[64]; uint32_t block_len; } sha256_ctx_t;
void sha256_init(sha256_ctx_t* ctx);
void sha256_update(sha256_ctx_t* ctx, const uint8_t* data, uint32_t length);
void sha256_final(sha256_ctx_t* ctx, uint8_t digest[32]);
void hmac_sha256(const uint8_t* key, uint32_t key_length,
                 const uint8_t* message, uint32_t message_length,
                 uint8_t digest[32]);
#endif
