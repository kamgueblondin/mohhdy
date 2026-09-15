#include "gpt2_model.h"
#include "../../fs/initrd.h"

static gpt2_model_t current_model;
static const char* current_status = "GPT-2: aucun checkpoint charge";

static uint64_t gpt2_parameter_count(const gpt2_config_t* cfg) {
    uint64_t v = cfg->padded_vocab_size;
    uint64_t c = cfg->channels;
    uint64_t t = cfg->max_seq_len;
    uint64_t l = cfg->num_layers;

    return v * c +                 /* token embeddings */
           t * c +                 /* positional embeddings */
           l * c + l * c +         /* ln1 weights and bias */
           l * (3U * c) * c +      /* qkv weights */
           l * (3U * c) +          /* qkv bias */
           l * c * c + l * c +     /* attention projection */
           l * c + l * c +         /* ln2 weights and bias */
           l * (4U * c) * c +      /* MLP expansion */
           l * (4U * c) +          /* MLP expansion bias */
           l * c * (4U * c) +      /* MLP projection */
           l * c +                 /* MLP projection bias */
           c + c;                  /* final layer norm */
}

static void gpt2_model_reset(const char* status) {
    current_model.blob = 0;
    current_model.blob_size = 0;
    current_model.weights = 0;
    current_model.weight_count = 0;
    current_model.config.max_seq_len = 0;
    current_model.config.vocab_size = 0;
    current_model.config.padded_vocab_size = 0;
    current_model.config.num_layers = 0;
    current_model.config.num_heads = 0;
    current_model.config.channels = 0;
    current_model.ready = 0;
    current_status = status;
}

int gpt2_model_load_from_initrd(const char* path) {
    const uint8_t* blob;
    const uint32_t* header;
    uint32_t blob_size;
    uint64_t count;
    uint64_t required_size;

    gpt2_model_reset("GPT-2: verification du checkpoint");
    blob = (const uint8_t*)initrd_read_file(path);
    blob_size = initrd_get_file_size(path);
    if (!blob || blob_size < GPT2_HEADER_BYTES) {
        gpt2_model_reset("GPT-2: checkpoint absent de l'initrd");
        return -1;
    }

    header = (const uint32_t*)blob;
    if (header[0] != GPT2_CHECKPOINT_MAGIC) {
        gpt2_model_reset("GPT-2: magic de checkpoint invalide");
        return -2;
    }
    if (header[1] != GPT2_CHECKPOINT_VERSION) {
        gpt2_model_reset("GPT-2: version de checkpoint non supportee");
        return -3;
    }

    current_model.config.max_seq_len = header[2];
    current_model.config.vocab_size = header[3];
    current_model.config.num_layers = header[4];
    current_model.config.num_heads = header[5];
    current_model.config.channels = header[6];
    current_model.config.padded_vocab_size = header[7];

    if (current_model.config.max_seq_len == 0 ||
        current_model.config.vocab_size == 0 ||
        current_model.config.vocab_size > current_model.config.padded_vocab_size ||
        current_model.config.num_layers == 0 ||
        current_model.config.num_heads == 0 ||
        current_model.config.channels == 0 ||
        (current_model.config.channels % current_model.config.num_heads) != 0) {
        gpt2_model_reset("GPT-2: configuration de checkpoint invalide");
        return -4;
    }

    count = gpt2_parameter_count(&current_model.config);
    required_size = (uint64_t)GPT2_HEADER_BYTES + count * sizeof(float);
    if (required_size > blob_size) {
        gpt2_model_reset("GPT-2: checkpoint tronque");
        return -5;
    }

    current_model.blob = blob;
    current_model.blob_size = blob_size;
    current_model.weights = (const float*)(blob + GPT2_HEADER_BYTES);
    current_model.weight_count = count;
    current_model.ready = 1;
    current_status = "GPT-2: checkpoint local valide";
    return 0;
}

const gpt2_model_t* gpt2_model_current(void) {
    return &current_model;
}

const char* gpt2_model_status(void) {
    return current_status;
}
