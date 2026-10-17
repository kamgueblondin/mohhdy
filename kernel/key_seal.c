/* Lot 6 (US-059/060): see key_seal.h. */
#include "key_seal.h"
#include "aes_gcm.h"
#include "sha256.h"

static uint8_t g_seal_key[16];
static uint8_t g_seal_fixed[4];
static uint8_t g_pool[32];
static int g_seal_ready;
static uint32_t g_nonce_ctr;
static uint32_t g_seals, g_unseals, g_failures;

static void wipe(volatile uint8_t* p, uint32_t n) { while (n--) *p++ = 0; }

void keyseal_reset(void) {
    wipe(g_seal_key, sizeof(g_seal_key));
    wipe(g_seal_fixed, sizeof(g_seal_fixed));
    wipe(g_pool, sizeof(g_pool));
    g_seal_ready = 0;
    g_nonce_ctr = 0;
    g_seals = g_unseals = g_failures = 0;
}

void keyseal_init(const uint8_t* entropy, uint32_t length) {
    sha256_ctx_t c;
    uint8_t d[32];
    uint32_t i;
    if (!entropy || length == 0U) return;
    sha256_init(&c);
    sha256_update(&c, (const uint8_t*)"mohhdy-seal-pool", 16U);
    sha256_update(&c, g_pool, 32U);
    sha256_update(&c, entropy, length);
    sha256_final(&c, g_pool);
    hmac_sha256(g_pool, 32U, (const uint8_t*)"seal-key", 8U, d);
    for (i = 0; i < 16U; i++) g_seal_key[i] = d[i];
    for (i = 0; i < 4U; i++) g_seal_fixed[i] = d[16U + i];
    wipe(d, sizeof(d));
    g_seal_ready = 1;
}

int keyseal_ready(void) { return g_seal_ready; }

static int build_aad(const char* label, uint8_t aad[16U + KEYSEAL_LABEL_MAX]) {
    const char* p = "mohhdy-sealed1:";
    int n = 0, i = 0;
    while (p[i]) aad[n++] = (uint8_t)p[i++];
    i = 0;
    while (label[i]) {
        if (i >= (int)KEYSEAL_LABEL_MAX) return -1;
        aad[n++] = (uint8_t)label[i++];
    }
    return n;
}

int keyseal_seal(const char* label, const uint8_t* plain, uint32_t length, uint8_t* out, uint32_t capacity) {
    uint8_t aad[16U + KEYSEAL_LABEL_MAX], nonce[8], mac[32];
    int alen;
    uint32_t i;
    if (!g_seal_ready) return KEYSEAL_E_NOKEY;
    if (!label || !plain || !out || length == 0U || length > KEYSEAL_MAX_PLAIN ||
        capacity < length + KEYSEAL_OVERHEAD) return KEYSEAL_E_ARG;
    alen = build_aad(label, aad);
    if (alen < 0) return KEYSEAL_E_ARG;
    /* nonce: counter plus a keyed hash of it, never repeats within a boot */
    g_nonce_ctr++;
    nonce[0] = (uint8_t)(g_nonce_ctr >> 24); nonce[1] = (uint8_t)(g_nonce_ctr >> 16);
    nonce[2] = (uint8_t)(g_nonce_ctr >> 8); nonce[3] = (uint8_t)g_nonce_ctr;
    hmac_sha256(g_pool, 32U, nonce, 4U, mac);
    for (i = 0; i < 4U; i++) nonce[4U + i] = mac[i];
    for (i = 0; i < 8U; i++) out[i] = nonce[i];
    if (aes128_gcm_encrypt(g_seal_key, g_seal_fixed, nonce, aad, (uint16_t)alen, plain, (uint16_t)length,
                           out + KEYSEAL_NONCE, out + KEYSEAL_NONCE + length) != 0) return KEYSEAL_E_ARG;
    g_seals++;
    return (int)(length + KEYSEAL_OVERHEAD);
}

int keyseal_unseal(const char* label, const uint8_t* blob, uint32_t length, uint8_t* out, uint32_t capacity) {
    uint8_t aad[16U + KEYSEAL_LABEL_MAX];
    uint32_t n;
    int alen;
    if (!g_seal_ready) return KEYSEAL_E_NOKEY;
    if (!label || !blob || !out || length <= KEYSEAL_OVERHEAD) return KEYSEAL_E_ARG;
    n = length - KEYSEAL_OVERHEAD;
    if (n > KEYSEAL_MAX_PLAIN || capacity < n) return KEYSEAL_E_ARG;
    alen = build_aad(label, aad);
    if (alen < 0) return KEYSEAL_E_ARG;
    if (aes128_gcm_decrypt(g_seal_key, g_seal_fixed, blob, aad, (uint16_t)alen, blob + KEYSEAL_NONCE, (uint16_t)n,
                           blob + KEYSEAL_NONCE + n, out) != 0) {
        wipe(out, n);
        g_failures++;
        return KEYSEAL_E_AUTH;
    }
    g_unseals++;
    return (int)n;
}

void keyseal_stats(uint32_t* seals, uint32_t* unseals, uint32_t* failures) {
    if (seals) *seals = g_seals;
    if (unseals) *unseals = g_unseals;
    if (failures) *failures = g_failures;
}
