#include "gpt2_tokenizer.h"
#include "../../fs/initrd.h"

#define GPT2_TOKENIZER_MAGIC 20240328U
#define GPT2_TOKENIZER_MAX_VOCAB 50257U
#define GPT2_TOKEN_HASH_SIZE 65536U
#define GPT2_HASH_EMPTY 0xFFFFFFFFU
#define GPT2_BPE_MAX_PARTS 160U
#define GPT2_DECODE_MAX 256U

typedef struct {
    const uint8_t* bytes;
    uint8_t length;
} gpt2_token_piece_t;

static gpt2_token_piece_t token_table[GPT2_TOKENIZER_MAX_VOCAB];
static uint32_t token_hash[GPT2_TOKEN_HASH_SIZE];
static uint32_t tokenizer_vocab_size;
static uint32_t tokenizer_eot;
static int tokenizer_ready;
static char decoded_piece[GPT2_DECODE_MAX];
static const char* tokenizer_status = "GPT-2 tokenizer: aucun vocabulaire charge";

static uint32_t gpt2_hash_bytes(const uint8_t* bytes, uint32_t length) {
    uint32_t hash = 2166136261U;
    for (uint32_t i = 0; i < length; i++) {
        hash ^= bytes[i];
        hash *= 16777619U;
    }
    return hash;
}

static int gpt2_bytes_equal(const uint8_t* a, const uint8_t* b, uint32_t length) {
    for (uint32_t i = 0; i < length; i++) {
        if (a[i] != b[i]) return 0;
    }
    return 1;
}

static void tokenizer_reset(const char* status) {
    tokenizer_vocab_size = 0;
    tokenizer_eot = 0;
    tokenizer_ready = 0;
    tokenizer_status = status;
    for (uint32_t i = 0; i < GPT2_TOKEN_HASH_SIZE; i++) token_hash[i] = GPT2_HASH_EMPTY;
}

static int gpt2_hash_insert(uint32_t token_id) {
    uint32_t length = token_table[token_id].length;
    uint32_t hash = gpt2_hash_bytes(token_table[token_id].bytes, length);
    for (uint32_t probe = 0; probe < GPT2_TOKEN_HASH_SIZE; probe++) {
        uint32_t slot = (hash + probe) & (GPT2_TOKEN_HASH_SIZE - 1U);
        if (token_hash[slot] == GPT2_HASH_EMPTY) {
            token_hash[slot] = token_id;
            return 0;
        }
    }
    return -1;
}

static uint32_t gpt2_lookup_token(const uint8_t* bytes, uint32_t length) {
    uint32_t hash;
    if (!bytes || length == 0) return GPT2_HASH_EMPTY;
    hash = gpt2_hash_bytes(bytes, length);
    for (uint32_t probe = 0; probe < GPT2_TOKEN_HASH_SIZE; probe++) {
        uint32_t slot = (hash + probe) & (GPT2_TOKEN_HASH_SIZE - 1U);
        uint32_t token_id = token_hash[slot];
        if (token_id == GPT2_HASH_EMPTY) return GPT2_HASH_EMPTY;
        if (token_table[token_id].length == length &&
            gpt2_bytes_equal(token_table[token_id].bytes, bytes, length)) {
            return token_id;
        }
    }
    return GPT2_HASH_EMPTY;
}

static int gpt2_is_digit(uint8_t c) {
    return c >= '0' && c <= '9';
}

static int gpt2_is_space(uint8_t c) {
    return c == ' ' || c == '\t' || c == '\n' || c == '\r';
}

static int gpt2_utf8_decode(const uint8_t* text, uint32_t remaining,
                            uint32_t* codepoint, uint32_t* width) {
    uint8_t a;
    uint8_t b;
    uint8_t c;
    uint8_t d;
    if (!text || remaining == 0U || !codepoint || !width) return 0;
    a = text[0];
    if (a < 0x80U) {
        *codepoint = a;
        *width = 1U;
        return 1;
    }
    if (a >= 0xC2U && a <= 0xDFU && remaining >= 2U) {
        b = text[1];
        if ((b & 0xC0U) != 0x80U) return 0;
        *codepoint = ((uint32_t)(a & 0x1FU) << 6) | (uint32_t)(b & 0x3FU);
        *width = 2U;
        return 1;
    }
    if (a >= 0xE0U && a <= 0xEFU && remaining >= 3U) {
        b = text[1];
        c = text[2];
        if ((b & 0xC0U) != 0x80U || (c & 0xC0U) != 0x80U ||
            (a == 0xE0U && b < 0xA0U) || (a == 0xEDU && b >= 0xA0U)) return 0;
        *codepoint = ((uint32_t)(a & 0x0FU) << 12) |
                     ((uint32_t)(b & 0x3FU) << 6) | (uint32_t)(c & 0x3FU);
        *width = 3U;
        return 1;
    }
    if (a >= 0xF0U && a <= 0xF4U && remaining >= 4U) {
        b = text[1];
        c = text[2];
        d = text[3];
        if ((b & 0xC0U) != 0x80U || (c & 0xC0U) != 0x80U || (d & 0xC0U) != 0x80U ||
            (a == 0xF0U && b < 0x90U) || (a == 0xF4U && b > 0x8FU)) return 0;
        *codepoint = ((uint32_t)(a & 0x07U) << 18) |
                     ((uint32_t)(b & 0x3FU) << 12) |
                     ((uint32_t)(c & 0x3FU) << 6) | (uint32_t)(d & 0x3FU);
        *width = 4U;
        return 1;
    }
    return 0;
}

/* Sous-ensemble sans table externe de \p{L}, couvrant les écritures courantes
 * des prompts GPT-2. Les séparateurs, chiffres, marques et emoji restent hors
 * de la classe lettre comme dans le découpage regex de référence. */
static int gpt2_is_unicode_letter(uint32_t cp) {
    if ((cp >= 'A' && cp <= 'Z') || (cp >= 'a' && cp <= 'z')) return 1;
    if ((cp >= 0x00C0U && cp <= 0x00D6U) || (cp >= 0x00D8U && cp <= 0x00F6U) ||
        (cp >= 0x00F8U && cp <= 0x02C1U) || (cp >= 0x02C6U && cp <= 0x02D1U) ||
        (cp >= 0x02E0U && cp <= 0x02E4U) || (cp >= 0x0370U && cp <= 0x052FU) ||
        (cp >= 0x0531U && cp <= 0x0588U) || (cp >= 0x05D0U && cp <= 0x05EAU) ||
        (cp >= 0x05F0U && cp <= 0x05F2U) || (cp >= 0x0620U && cp <= 0x063FU) ||
        (cp >= 0x0641U && cp <= 0x064AU) || (cp >= 0x066EU && cp <= 0x066FU) ||
        (cp >= 0x0671U && cp <= 0x06D3U) || (cp >= 0x0904U && cp <= 0x0939U) ||
        (cp >= 0x3041U && cp <= 0x3096U) || (cp >= 0x30A1U && cp <= 0x30FAU) ||
        (cp >= 0x3400U && cp <= 0x4DBFU) || (cp >= 0x4E00U && cp <= 0x9FFFU) ||
        (cp >= 0xAC00U && cp <= 0xD7A3U)) return 1;
    return 0;
}

static int gpt2_is_letter_at(const uint8_t* text, uint32_t length, uint32_t pos,
                             uint32_t* width) {
    uint32_t cp;
    uint32_t local_width;
    if (pos >= length || !gpt2_utf8_decode(text + pos, length - pos, &cp, &local_width)) return 0;
    if (width) *width = local_width;
    return gpt2_is_unicode_letter(cp);
}

static int gpt2_match_ci(const uint8_t* text, uint32_t remaining, const char* lit) {
    uint32_t i = 0;
    while (lit[i] != '\0') {
        uint8_t expected = (uint8_t)lit[i];
        uint8_t got;
        if (i >= remaining) return 0;
        got = text[i];
        if (expected >= 'a' && expected <= 'z') {
            if (got != expected && got != (uint8_t)(expected - 32U)) return 0;
        } else if (got != expected) {
            return 0;
        }
        i++;
    }
    return 1;
}

/* Decoupage proche de la regex GPT-2 (ASCII + sequences UTF-8). */
static uint32_t gpt2_next_chunk(const uint8_t* text, uint32_t length, uint32_t pos) {
    uint32_t start;
    uint32_t i;
    if (pos >= length) return 0;

    if (text[pos] == '\'' &&
        (gpt2_match_ci(text + pos, length - pos, "'s") ||
         gpt2_match_ci(text + pos, length - pos, "'t") ||
         gpt2_match_ci(text + pos, length - pos, "'m") ||
         gpt2_match_ci(text + pos, length - pos, "'d"))) {
        return 2;
    }
    if (text[pos] == '\'' &&
        (gpt2_match_ci(text + pos, length - pos, "'re") ||
         gpt2_match_ci(text + pos, length - pos, "'ve") ||
         gpt2_match_ci(text + pos, length - pos, "'ll"))) {
        return 3;
    }

    start = pos;
    if (text[pos] == ' ' && pos + 1U < length &&
        (gpt2_is_letter_at(text, length, pos + 1U, 0) || gpt2_is_digit(text[pos + 1U]) ||
         !gpt2_is_space(text[pos + 1U]))) {
        pos++;
    }
    if (pos < length && gpt2_is_letter_at(text, length, pos, 0)) {
        uint32_t letter_width;
        while (pos < length && gpt2_is_letter_at(text, length, pos, &letter_width)) pos += letter_width;
        return pos - start;
    }
    if (pos < length && gpt2_is_digit(text[pos])) {
        pos++;
        while (pos < length && gpt2_is_digit(text[pos])) pos++;
        return pos - start;
    }
    if (pos < length && !gpt2_is_space(text[pos]) && !gpt2_is_letter_at(text, length, pos, 0) &&
        !gpt2_is_digit(text[pos])) {
        uint32_t punct_width = 1U;
        uint32_t ignored_cp;
        if (gpt2_utf8_decode(text + pos, length - pos, &ignored_cp, &punct_width)) pos += punct_width;
        else pos++;
        while (pos < length && !gpt2_is_space(text[pos]) &&
               !gpt2_is_letter_at(text, length, pos, 0) && !gpt2_is_digit(text[pos])) {
            if (gpt2_utf8_decode(text + pos, length - pos, &ignored_cp, &punct_width)) pos += punct_width;
            else pos++;
        }
        return pos - start;
    }

    if (gpt2_is_space(text[start])) {
        i = start;
        while (i < length && gpt2_is_space(text[i])) i++;
        return i - start;
    }
    return 1;
}

static int gpt2_bpe_chunk(const uint8_t* chunk, uint32_t chunk_len,
                          uint32_t* out_tokens, uint32_t max_tokens,
                          uint32_t* inout_count) {
    uint32_t starts[GPT2_BPE_MAX_PARTS];
    uint32_t lengths[GPT2_BPE_MAX_PARTS];
    uint32_t parts;
    uint32_t count = *inout_count;

    if (chunk_len == 0) return 0;
    if (chunk_len > GPT2_BPE_MAX_PARTS) {
        tokenizer_status = "GPT-2 tokenizer: morceau trop long";
        return -3;
    }

    parts = chunk_len;
    for (uint32_t i = 0; i < chunk_len; i++) {
        starts[i] = i;
        lengths[i] = 1;
        if (gpt2_lookup_token(chunk + i, 1) == GPT2_HASH_EMPTY) {
            tokenizer_status = "GPT-2 tokenizer: octet absent du vocabulaire";
            return -2;
        }
    }

    while (parts > 1U) {
        uint32_t best_id = GPT2_HASH_EMPTY;
        uint32_t best_index = 0;
        for (uint32_t i = 0; i + 1U < parts; i++) {
            uint32_t merged_len = lengths[i] + lengths[i + 1U];
            uint32_t token_id = gpt2_lookup_token(chunk + starts[i], merged_len);
            if (token_id == GPT2_HASH_EMPTY) continue;
            if (best_id == GPT2_HASH_EMPTY || token_id < best_id) {
                best_id = token_id;
                best_index = i;
            }
        }
        if (best_id == GPT2_HASH_EMPTY) break;
        lengths[best_index] += lengths[best_index + 1U];
        for (uint32_t i = best_index + 1U; i + 1U < parts; i++) {
            starts[i] = starts[i + 1U];
            lengths[i] = lengths[i + 1U];
        }
        parts--;
    }

    if (count + parts > max_tokens) {
        tokenizer_status = "GPT-2 tokenizer: contexte trop long";
        return -3;
    }
    for (uint32_t i = 0; i < parts; i++) {
        uint32_t token_id = gpt2_lookup_token(chunk + starts[i], lengths[i]);
        if (token_id == GPT2_HASH_EMPTY) return -2;
        out_tokens[count++] = token_id;
    }
    *inout_count = count;
    return 0;
}

int gpt2_tokenizer_load_from_buffer(const uint8_t* blob, uint32_t blob_size) {
    const uint32_t* header;
    uint32_t offset = 1024;

    tokenizer_reset("GPT-2 tokenizer: verification du vocabulaire");
    if (!blob || blob_size < 1024) {
        tokenizer_reset("GPT-2 tokenizer: fichier absent de l'initrd");
        return -1;
    }
    header = (const uint32_t*)blob;
    if (header[0] != GPT2_TOKENIZER_MAGIC || (header[1] != 1U && header[1] != 2U)) {
        tokenizer_reset("GPT-2 tokenizer: en-tete non supporte");
        return -2;
    }
    if (header[2] == 0 || header[2] > GPT2_TOKENIZER_MAX_VOCAB) {
        tokenizer_reset("GPT-2 tokenizer: taille de vocabulaire invalide");
        return -3;
    }

    tokenizer_vocab_size = header[2];
    tokenizer_eot = header[1] == 1U ? 50256U : header[3];
    for (uint32_t token = 0; token < tokenizer_vocab_size; token++) {
        uint8_t length;
        if (offset >= blob_size) {
            tokenizer_reset("GPT-2 tokenizer: vocabulaire tronque");
            return -4;
        }
        length = blob[offset++];
        if (length == 0 || offset + length > blob_size) {
            tokenizer_reset("GPT-2 tokenizer: entree de vocabulaire invalide");
            return -5;
        }
        token_table[token].bytes = blob + offset;
        token_table[token].length = length;
        offset += length;
        if (gpt2_hash_insert(token) != 0) {
            tokenizer_reset("GPT-2 tokenizer: table de hachage pleine");
            return -6;
        }
    }
    tokenizer_ready = 1;
    tokenizer_status = "GPT-2 tokenizer: vocabulaire local valide";
    return 0;
}

int gpt2_tokenizer_load_from_initrd(const char* path) {
    const uint8_t* blob = (const uint8_t*)initrd_read_file(path);
    uint32_t blob_size = initrd_get_file_size(path);
    return gpt2_tokenizer_load_from_buffer(blob, blob_size);
}

int gpt2_tokenizer_encode(const char* text, uint32_t* out_tokens,
                          uint32_t max_tokens, uint32_t* out_count) {
    const uint8_t* bytes;
    uint32_t length = 0;
    uint32_t pos = 0;
    uint32_t count = 0;
    int rc;

    if (!text || !out_tokens || !out_count || !tokenizer_ready) return -1;
    bytes = (const uint8_t*)text;
    while (bytes[length] != '\0') length++;

    while (pos < length) {
        uint32_t chunk_len = gpt2_next_chunk(bytes, length, pos);
        if (chunk_len == 0) break;
        rc = gpt2_bpe_chunk(bytes + pos, chunk_len, out_tokens, max_tokens, &count);
        if (rc != 0) return rc;
        pos += chunk_len;
    }
    if (count == 0) {
        if (max_tokens == 0) return -3;
        out_tokens[count++] = tokenizer_eot;
    }
    *out_count = count;
    tokenizer_status = "GPT-2 tokenizer: encodage BPE termine";
    return 0;
}

int gpt2_tokenizer_encode_ascii(const char* text, uint32_t* out_tokens,
                                uint32_t max_tokens, uint32_t* out_count) {
    return gpt2_tokenizer_encode(text, out_tokens, max_tokens, out_count);
}

const char* gpt2_tokenizer_decode(uint32_t token_id) {
    uint32_t length;
    uint32_t out = 0;
    if (!tokenizer_ready || token_id >= tokenizer_vocab_size) return 0;
    length = token_table[token_id].length;
    for (uint32_t i = 0; i < length && out + 1U < GPT2_DECODE_MAX; i++) {
        uint8_t value = token_table[token_id].bytes[i];
        if (value == 0) continue;
        if (value < 32U && value != '\t' && value != '\n' && value != '\r') continue;
        if (value == 127U) continue;
        decoded_piece[out++] = (char)value;
    }
    decoded_piece[out] = '\0';
    return decoded_piece;
}

uint32_t gpt2_tokenizer_eot(void) {
    return tokenizer_eot;
}

const char* gpt2_tokenizer_status(void) {
    return tokenizer_status;
}
