/* shell_util.c - pure helpers of the guest shell (see shell_util.h). */
#include "shell_util.h"

int shell_parse_line(const char *input, char *command, int cmd_max,
                     char args[][SHELL_ARG_MAX_LEN], int max_args,
                     int *arg_count, int *truncated) {
    int i = 0, cmd_len = 0, char_idx = 0, in_word = 0, in_quotes = 0, cut = 0;
    *arg_count = 0;
    if (cmd_max > 0) command[0] = '\0';
    if (!input) {
        if (truncated) *truncated = 0;
        return 0;
    }
    while (input[i] == ' ' || input[i] == '\t') i++;
    while (input[i] != '\0' && input[i] != ' ' && input[i] != '\t') {
        if (cmd_len < cmd_max - 1) command[cmd_len++] = input[i];
        else cut |= SHELL_PARSE_COMMAND_CUT;
        i++;
    }
    if (cmd_max > 0) command[cmd_len] = '\0';
    for (; input[i] != '\0'; i++) {
        char c = input[i];
        /* Same rules as the historical parser: a blank ends a word outside
         * quotes and is skipped inside quotes; a quote starts a word. */
        if (c == ' ' || c == '\t') {
            if (in_word && !in_quotes) {
                args[*arg_count][char_idx] = '\0';
                (*arg_count)++;
                in_word = 0;
                char_idx = 0;
            }
            continue;
        }
        if (!in_word) {
            if (*arg_count >= max_args) {
                cut |= SHELL_PARSE_WORDS_DROPPED;
                break;
            }
            in_word = 1;
            char_idx = 0;
        }
        if (c == '"') {
            in_quotes = !in_quotes;
            continue;
        }
        if (char_idx < SHELL_ARG_MAX_LEN - 1) args[*arg_count][char_idx++] = c;
        else cut |= SHELL_PARSE_WORD_CUT;
    }
    if (in_word) {
        args[*arg_count][char_idx] = '\0';
        (*arg_count)++;
    }
    if (truncated) *truncated = cut;
    return cmd_len > 0 ? 1 : 0;
}

void shell_ai_latency(uint32_t ticks, uint32_t hz, uint32_t tokens,
                      uint32_t *total_ms, uint32_t *per_token_ms) {
    uint32_t ms;
    if (hz == 0) hz = 100U;
    /* ticks * 1000 / hz without overflowing 32 bits for long runs. */
    ms = (ticks / hz) * 1000U + ((ticks % hz) * 1000U) / hz;
    if (total_ms) *total_ms = ms;
    if (per_token_ms) *per_token_ms = tokens ? ms / tokens : 0U;
}
