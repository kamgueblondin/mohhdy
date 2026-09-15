#ifndef MOHHDY_AES_GCM_H
#define MOHHDY_AES_GCM_H
#include <stdint.h>
#define AES128_BLOCK_SIZE 16U
#define AES128_GCM_TAG_SIZE 16U
typedef struct { uint8_t round_keys[176]; } aes128_ctx_t;
int aes128_init(aes128_ctx_t* context, const uint8_t key[16]);
int aes128_encrypt_block(const aes128_ctx_t* context, const uint8_t input[16], uint8_t output[16]);
int aes128_gcm_encrypt(const uint8_t key[16], const uint8_t fixed_iv[4], const uint8_t explicit_nonce[8], const uint8_t* additional_data, uint16_t additional_length, const uint8_t* plaintext, uint16_t plaintext_length, uint8_t* ciphertext, uint8_t tag[16]);
int aes128_gcm_decrypt(const uint8_t key[16], const uint8_t fixed_iv[4], const uint8_t explicit_nonce[8], const uint8_t* additional_data, uint16_t additional_length, const uint8_t* ciphertext, uint16_t ciphertext_length, const uint8_t tag[16], uint8_t* plaintext);
#endif
