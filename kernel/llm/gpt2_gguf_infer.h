#ifndef MOHHDY_GPT2_GGUF_INFER_H
#define MOHHDY_GPT2_GGUF_INFER_H

#include <stdint.h>
#include "../fs/fat16.h"
#include "../fs/fat32.h"

/* Limites statiques du profil GPT-2 124M GGUF local. Les poids restent sur
 * FAT16 ; seul le catalogue et le workspace sont résidents. */
#define GPT2_GGUF_INFER_HEADER_BYTES (2U * 1024U * 1024U)
#define GPT2_GGUF_INFER_MAX_CONTEXT 64U
#define GPT2_GGUF_INFER_MAX_LAYERS 12U
#define GPT2_GGUF_INFER_MAX_CHANNELS 768U
#define GPT2_GGUF_INFER_MAX_VOCAB 50257U
#define GPT2_GGUF_INFER_HEADS 12U

/* Initialise le profil GGUF depuis un fichier FAT16 8.3, sans charger ses
 * poids. Le fichier doit contenir un GPT-2 compatible avec les bornes ci-dessus. */
int gpt2_gguf_infer_init_fat16(const fat16_volume_t* volume, const char* filename);
int gpt2_gguf_infer_init_fat32(const fat32_volume_t* volume, const char* filename);
/* Échantillonne le prochain token avec cache KV persistant et préfixe borné. */
int gpt2_gguf_generate_next_sampled(const uint32_t* tokens, uint32_t token_count,
                                    uint32_t generated_count, uint32_t* next_token,
                                    uint32_t* rng_state);
/* Indique la disponibilité du profil GGUF local. */
int gpt2_gguf_infer_ready(void);
const char* gpt2_gguf_infer_status(void);

#endif
