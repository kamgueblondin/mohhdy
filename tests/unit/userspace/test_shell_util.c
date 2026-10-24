/* Guest shell helpers (userspace/shell_util.c): bounded command parsing
 * with truncation flags, and whole-answer AI latency. */
#include "../../framework/unity.h"
#include "../../../userspace/shell_util.h"
#include <stdio.h>
#include <string.h>

static char cmd[64];
static char args[64][SHELL_ARG_MAX_LEN];

static void test_parse_basic_and_quotes(void) {
    int n = -1, t = -1;
    TEST_ASSERT_EQUAL(1, shell_parse_line("ai hello  world", cmd, 64, args, 64, &n, &t));
    TEST_ASSERT_EQUAL_STRING("ai", cmd);
    TEST_ASSERT_EQUAL(2, n);
    TEST_ASSERT_EQUAL_STRING("hello", args[0]);
    TEST_ASSERT_EQUAL_STRING("world", args[1]);
    TEST_ASSERT_EQUAL(0, t);
    TEST_ASSERT_EQUAL(1, shell_parse_line("echo \"a b\" c", cmd, 64, args, 64, &n, &t));
    TEST_ASSERT_EQUAL_STRING("echo", cmd);
    TEST_ASSERT_EQUAL(2, n);
    /* Historical shell semantics, kept as is: quotes are dropped and a
     * space inside them is skipped, so "a b" becomes one word ab. */
    TEST_ASSERT_EQUAL_STRING("ab", args[0]);
    TEST_ASSERT_EQUAL_STRING("c", args[1]);
    TEST_ASSERT_EQUAL(0, shell_parse_line("   ", cmd, 64, args, 64, &n, &t));
    TEST_ASSERT_EQUAL(0, n);
}

/* The old shell kept 32 words and dropped the rest without a word. */
static void test_parse_long_prompt_flags_truncation(void) {
    char line[1024];
    int i, n = 0, t = 0, p = 0;
    p += sprintf(line + p, "ai");
    for (i = 0; i < 40; i++) p += sprintf(line + p, " w%d", i);
    TEST_ASSERT_EQUAL(1, shell_parse_line(line, cmd, 64, args, 64, &n, &t));
    TEST_ASSERT_EQUAL(40, n);
    TEST_ASSERT_EQUAL(0, t);
    TEST_ASSERT_EQUAL_STRING("w39", args[39]);
    TEST_ASSERT_EQUAL(1, shell_parse_line(line, cmd, 64, args, 32, &n, &t));
    TEST_ASSERT_EQUAL(32, n);
    TEST_ASSERT_TRUE(t & SHELL_PARSE_WORDS_DROPPED);
}

static void test_parse_long_word_and_command_are_bounded(void) {
    char line[400];
    int n = 0, t = 0;
    memset(line, 0, sizeof(line));
    strcpy(line, "ai ");
    memset(line + 3, 'x', 200);
    TEST_ASSERT_EQUAL(1, shell_parse_line(line, cmd, 64, args, 64, &n, &t));
    TEST_ASSERT_EQUAL(1, n);
    TEST_ASSERT_EQUAL(SHELL_ARG_MAX_LEN - 1, (int)strlen(args[0]));
    TEST_ASSERT_TRUE(t & SHELL_PARSE_WORD_CUT);
    memset(line, 'c', 100);
    line[100] = 0;
    TEST_ASSERT_EQUAL(1, shell_parse_line(line, cmd, 16, args, 64, &n, &t));
    TEST_ASSERT_EQUAL(15, (int)strlen(cmd));
    TEST_ASSERT_TRUE(t & SHELL_PARSE_COMMAND_CUT);
}

/* The old ai-runtime json divided the last GGUF step latency by 24. */
static void test_ai_latency(void) {
    uint32_t ms = 1, per = 1;
    shell_ai_latency(1500U, 100U, 24U, &ms, &per);
    TEST_ASSERT_EQUAL(15000, (int)ms);
    TEST_ASSERT_EQUAL(625, (int)per);
    shell_ai_latency(300U, 100U, 1U, &ms, &per);
    TEST_ASSERT_EQUAL(3000, (int)ms);
    TEST_ASSERT_EQUAL(3000, (int)per);
    shell_ai_latency(0U, 100U, 0U, &ms, &per);
    TEST_ASSERT_EQUAL(0, (int)ms);
    TEST_ASSERT_EQUAL(0, (int)per);
    shell_ai_latency(0xFFFFFFFFU, 100U, 2U, &ms, &per);
    TEST_ASSERT_TRUE(ms > 4000000000U);
    TEST_ASSERT_EQUAL((int)(ms / 2U), (int)per);
}

int main(void) {
    unity_init();
    RUN_TEST(test_parse_basic_and_quotes);
    RUN_TEST(test_parse_long_prompt_flags_truncation);
    RUN_TEST(test_parse_long_word_and_command_are_bounded);
    RUN_TEST(test_ai_latency);
    unity_print_results();
    unity_cleanup();
    return unity_stats.tests_failed == 0 ? 0 : 1;
}
