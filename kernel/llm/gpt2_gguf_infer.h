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
/* Pré-charge et réutilise le cache KV pour un contexte multi-tours sans ré-évaluation complète. */
int gpt2_gguf_infer_preload_kv_cache(const uint32_t* tokens, uint32_t token_count);
/* Retourne le nombre de jetons actuellement enregistrés dans le cache KV. */
uint32_t gpt2_gguf_infer_kv_cache_count(void);
/* Indique la disponibilité du profil GGUF local. */
int gpt2_gguf_infer_ready(void);
/* Taille du fichier GGUF FAT16 (0 si pas pret ou FAT32). Reste valide apres
 * gpt2_gguf_infer_release_resident(), qui oublie l'instantane. Le nom 8.3
 * sert la lecture bulk du worker. */
uint32_t gpt2_gguf_infer_resident_size(void);
/* Rend les pages de l'instantane au PMM et l'oublie (lectures suivantes sur
 * le disque). 0 s'il n'y en a plus, ou dans le binaire Ring 3. La taille
 * enregistree reste. */
uint32_t gpt2_gguf_infer_release_resident(void);
const char* gpt2_gguf_infer_filename(void);
const char* gpt2_gguf_infer_status(void);

#endif
