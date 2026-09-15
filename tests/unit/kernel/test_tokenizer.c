/* test_tokenizer.c - BPE GPT-2 (entree) et decode (sortie) */

#include <string.h>
#include "../../framework/unity.h"
#include "../../../kernel/llm/gpt2_tokenizer.h"

char* initrd_read_file(const char* filename) {
    (void)filename;
    return 0;
}

uint32_t initrd_get_file_size(const char* filename) {
    (void)filename;
    return 0;
}

static uint8_t g_blob[4096] __attribute__((aligned(4)));

static void blob_put_piece(uint32_t* offset, const char* bytes, uint8_t length) {
    g_blob[(*offset)++] = length;
    for (uint8_t i = 0; i < length; i++) g_blob[(*offset)++] = (uint8_t)bytes[i];
}

static int load_synthetic_vocab(void) {
    uint32_t header[256];
    uint32_t offset;
    memset(header, 0, sizeof(header));
    header[0] = 20240328U;
    header[1] = 2;
    header[2] = 29;
    header[3] = 20; /* EOT */
    memcpy(g_blob, header, sizeof(header));
    offset = 1024;
    blob_put_piece(&offset, "a", 1);
    blob_put_piece(&offset, "b", 1);
    blob_put_piece(&offset, "c", 1);
    blob_put_piece(&offset, "h", 1);
    blob_put_piece(&offset, "e", 1);
    blob_put_piece(&offset, "l", 1);
    blob_put_piece(&offset, "o", 1);
    blob_put_piece(&offset, " ", 1);
    blob_put_piece(&offset, "he", 2);
    blob_put_piece(&offset, "hel", 3);
    blob_put_piece(&offset, "hell", 4);
    blob_put_piece(&offset, "hello", 5);
    blob_put_piece(&offset, "ab", 2);
    blob_put_piece(&offset, "abc", 3);
    blob_put_piece(&offset, "!", 1);
    blob_put_piece(&offset, "\n", 1);
    blob_put_piece(&offset, "\xC3", 1);
    blob_put_piece(&offset, "\xA9", 1);
    blob_put_piece(&offset, "\xC3\xA9", 2);
    blob_put_piece(&offset, "\x01", 1);
    blob_put_piece(&offset, "E", 1);
    blob_put_piece(&offset, "\xCE", 1);
    blob_put_piece(&offset, "\xB1", 1);
    blob_put_piece(&offset, "\xCE\xB1", 2);
    blob_put_piece(&offset, "\xF0", 1);
    blob_put_piece(&offset, "\x9F", 1);
    blob_put_piece(&offset, "\x98", 1);
    blob_put_piece(&offset, "\x80", 1);
    blob_put_piece(&offset, "\xF0\x9F\x98\x80\xCE\xB1", 6);
    return gpt2_tokenizer_load_from_buffer(g_blob, offset);
}

void setUp(void) {
    TEST_ASSERT_EQUAL(0, load_synthetic_vocab());
}

void tearDown(void) {
}

static void test_load_rejects_short_buffer(void) {
    uint8_t tiny[8];
    setUp();
    TEST_ASSERT_TRUE(gpt2_tokenizer_load_from_buffer(tiny, 8) < 0);
    TEST_ASSERT_EQUAL(0, load_synthetic_vocab());
}

static void test_encode_empty_yields_eot(void) {
    uint32_t tokens[8];
    uint32_t count = 0;
    setUp();
    TEST_ASSERT_EQUAL(0, gpt2_tokenizer_encode("", tokens, 8, &count));
    TEST_ASSERT_EQUAL(1, count);
    TEST_ASSERT_EQUAL(20, tokens[0]);
    TEST_ASSERT_EQUAL(20, gpt2_tokenizer_eot());
}

static void test_encode_bpe_merges_abc(void) {
    uint32_t tokens[8];
    uint32_t count = 0;
    setUp();
    TEST_ASSERT_EQUAL(0, gpt2_tokenizer_encode("abc", tokens, 8, &count));
    TEST_ASSERT_EQUAL(1, count);
    TEST_ASSERT_EQUAL(13, tokens[0]);
}

static void test_encode_bpe_partial_ab(void) {
    uint32_t tokens[8];
    uint32_t count = 0;
    setUp();
    TEST_ASSERT_EQUAL(0, gpt2_tokenizer_encode("ab", tokens, 8, &count));
    TEST_ASSERT_EQUAL(1, count);
    TEST_ASSERT_EQUAL(12, tokens[0]);
}

static void test_encode_hello_uses_merge_chain(void) {
    uint32_t tokens[8];
    uint32_t count = 0;
    setUp();
    TEST_ASSERT_EQUAL(0, gpt2_tokenizer_encode("hello", tokens, 8, &count));
    TEST_ASSERT_EQUAL(1, count);
    TEST_ASSERT_EQUAL(11, tokens[0]);
}

static void test_encode_splits_punctuation(void) {
    uint32_t tokens[8];
    uint32_t count = 0;
    setUp();
    TEST_ASSERT_EQUAL(0, gpt2_tokenizer_encode("hello!", tokens, 8, &count));
    TEST_ASSERT_EQUAL(2, count);
    TEST_ASSERT_EQUAL(11, tokens[0]);
    TEST_ASSERT_EQUAL(14, tokens[1]);
}

static void test_encode_keeps_space_chunk(void) {
    uint32_t tokens[8];
    uint32_t count = 0;
    setUp();
    TEST_ASSERT_EQUAL(0, gpt2_tokenizer_encode("ab c", tokens, 8, &count));
    TEST_ASSERT_EQUAL(3, count);
    TEST_ASSERT_EQUAL(12, tokens[0]);
    TEST_ASSERT_EQUAL(7, tokens[1]);
    TEST_ASSERT_EQUAL(2, tokens[2]);
}

static void test_encode_unknown_byte_fails(void) {
    uint32_t tokens[8];
    uint32_t count = 0;
    char text[2];
    setUp();
    text[0] = 2;
    text[1] = 0;
    TEST_ASSERT_TRUE(gpt2_tokenizer_encode(text, tokens, 8, &count) < 0);
}

static void test_decode_keeps_ascii_and_space(void) {
    setUp();
    TEST_ASSERT_EQUAL_STRING("hello", gpt2_tokenizer_decode(11));
    TEST_ASSERT_EQUAL_STRING(" ", gpt2_tokenizer_decode(7));
    TEST_ASSERT_EQUAL_STRING("\n", gpt2_tokenizer_decode(15));
}

static void test_decode_keeps_utf8(void) {
    setUp();
    TEST_ASSERT_EQUAL_STRING("\xC3\xA9", gpt2_tokenizer_decode(18));
}

static void test_decode_drops_control_bytes(void) {
    setUp();
    TEST_ASSERT_EQUAL_STRING("", gpt2_tokenizer_decode(19));
}

static void test_encode_utf8_letter_chunk(void) {
    uint32_t tokens[8];
    uint32_t count = 0;
    setUp();
    TEST_ASSERT_EQUAL(0, gpt2_tokenizer_encode("\xC3\xA9", tokens, 8, &count));
    TEST_ASSERT_EQUAL(1, count);
    TEST_ASSERT_EQUAL(18, tokens[0]);
}

static void test_encode_greek_letter_uses_utf8_bpe_piece(void) {
    uint32_t tokens[8];
    uint32_t count = 0;
    setUp();
    TEST_ASSERT_EQUAL(0, gpt2_tokenizer_encode("\xCE\xB1", tokens, 8, &count));
    TEST_ASSERT_EQUAL(1, count);
    TEST_ASSERT_EQUAL(23, tokens[0]);
}

static void test_emoji_is_not_merged_with_following_unicode_letter(void) {
    uint32_t tokens[8];
    uint32_t count = 0;
    setUp();
    TEST_ASSERT_EQUAL(0, gpt2_tokenizer_encode("\xF0\x9F\x98\x80\xCE\xB1", tokens, 8, &count));
    /* The six-byte cross-class token exists in the vocabulary but must not be
     * selected: emoji is punctuation, alpha is a Unicode letter. */
    TEST_ASSERT_EQUAL(5, count);
    TEST_ASSERT_EQUAL(24, tokens[0]);
    TEST_ASSERT_EQUAL(25, tokens[1]);
    TEST_ASSERT_EQUAL(26, tokens[2]);
    TEST_ASSERT_EQUAL(27, tokens[3]);
    TEST_ASSERT_EQUAL(23, tokens[4]);
}

static void test_encode_ascii_wrapper(void) {
    uint32_t tokens[8];
    uint32_t count = 0;
    setUp();
    TEST_ASSERT_EQUAL(0, gpt2_tokenizer_encode_ascii("a", tokens, 8, &count));
    TEST_ASSERT_EQUAL(1, count);
    TEST_ASSERT_EQUAL(0, tokens[0]);
}

int main(void) {
    unity_init();
    RUN_TEST(test_load_rejects_short_buffer);
    RUN_TEST(test_encode_empty_yields_eot);
    RUN_TEST(test_encode_bpe_merges_abc);
    RUN_TEST(test_encode_bpe_partial_ab);
    RUN_TEST(test_encode_hello_uses_merge_chain);
    RUN_TEST(test_encode_splits_punctuation);
    RUN_TEST(test_encode_keeps_space_chunk);
    RUN_TEST(test_encode_unknown_byte_fails);
    RUN_TEST(test_decode_keeps_ascii_and_space);
    RUN_TEST(test_decode_keeps_utf8);
    RUN_TEST(test_decode_drops_control_bytes);
    RUN_TEST(test_encode_utf8_letter_chunk);
    RUN_TEST(test_encode_greek_letter_uses_utf8_bpe_piece);
    RUN_TEST(test_emoji_is_not_merged_with_following_unicode_letter);
    RUN_TEST(test_encode_ascii_wrapper);
    unity_print_results();
    unity_cleanup();
    return (unity_stats.tests_failed == 0) ? 0 : 1;
}
