#ifndef MOHHDY_GPT2_MODEL_H
#define MOHHDY_GPT2_MODEL_H

#include <stdint.h>

/*
 * Compatible with the CPU GPT-2 checkpoint header used by llm.c v3.
 * The payload remains a read-only Multiboot/initrd blob: weights are not
 * copied during probing, which keeps the initial boot memory footprint low.
 */
#define GPT2_CHECKPOINT_MAGIC   20240326U
#define GPT2_CHECKPOINT_VERSION 3U
#define GPT2_HEADER_WORDS       256U
#define GPT2_HEADER_BYTES       (GPT2_HEADER_WORDS * sizeof(uint32_t))

typedef struct {
    uint32_t max_seq_len;
    uint32_t vocab_size;
    uint32_t padded_vocab_size;
    uint32_t num_layers;
    uint32_t num_heads;
    uint32_t channels;
} gpt2_config_t;

typedef struct {
    const uint8_t* blob;
    uint32_t blob_size;
    const float* weights;
    uint64_t weight_count;
    gpt2_config_t config;
    int ready;
} gpt2_model_t;

/* Probe a checkpoint in the initrd without copying its weight payload. */
int gpt2_model_load_from_initrd(const char* path);
const gpt2_model_t* gpt2_model_current(void);
const char* gpt2_model_status(void);

#endif
