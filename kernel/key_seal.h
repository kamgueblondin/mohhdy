/* Lot 6 (US-059/060): kernel-held sealing key. Ring 3 never sees the key;
 * it hands plaintext in and gets an AES-128-GCM blob back, bound to a label
 * (the caller task name) through the AAD. The key is derived per boot from
 * TSC/PIT jitter (no TPM, no hardware root of trust: simule). */
#ifndef MOHHDY_KEY_SEAL_H
#define MOHHDY_KEY_SEAL_H
#include <stdint.h>

#define KEYSEAL_NONCE 8U
#define KEYSEAL_TAG 16U
#define KEYSEAL_OVERHEAD (KEYSEAL_NONCE + KEYSEAL_TAG)
#define KEYSEAL_MAX_PLAIN 256U
#define KEYSEAL_LABEL_MAX 32U

#define KEYSEAL_OK 0
#define KEYSEAL_E_ARG (-1)
#define KEYSEAL_E_NOKEY (-2)
#define KEYSEAL_E_AUTH (-3)

/* Mix entropy into the key (may be called several times). */
void keyseal_init(const uint8_t* entropy, uint32_t length);
void keyseal_reset(void);
int keyseal_ready(void);
/* out = nonce(8) | ciphertext | tag(16). Returns blob length or < 0. */
int keyseal_seal(const char* label, const uint8_t* plain, uint32_t length, uint8_t* out, uint32_t capacity);
/* Returns plaintext length or < 0 (KEYSEAL_E_AUTH: wrong label, tampered). */
int keyseal_unseal(const char* label, const uint8_t* blob, uint32_t length, uint8_t* out, uint32_t capacity);
/* Number of seal / unseal / auth failures since reset. */
void keyseal_stats(uint32_t* seals, uint32_t* unseals, uint32_t* failures);
#endif
