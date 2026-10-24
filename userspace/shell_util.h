/* shell_util.h - pure helpers of the guest shell, also built on the host
 * for unit tests (no syscalls, no libc). */
#ifndef SHELL_UTIL_H
#define SHELL_UTIL_H

#include <stdint.h>

#define SHELL_ARG_MAX_LEN 128

/* Bits of the *truncated output of shell_parse_line. */
#define SHELL_PARSE_WORDS_DROPPED 1  /* more words than max_args */
#define SHELL_PARSE_WORD_CUT      2  /* a word longer than 127 characters */
#define SHELL_PARSE_COMMAND_CUT   4  /* command name longer than cmd_max - 1 */

/* Splits input into a command and up to max_args arguments (double quotes
 * group words). Never writes past cmd_max or SHELL_ARG_MAX_LEN. Returns 1
 * when a command was found, 0 otherwise. *truncated reports what was lost
 * (0 when nothing was). */
int shell_parse_line(const char *input, char *command, int cmd_max,
                     char args[][SHELL_ARG_MAX_LEN], int max_args,
                     int *arg_count, int *truncated);

/* Latency of a whole local generation from PIT ticks (hz ticks per second)
 * and the number of generated tokens. per_token_ms is 0 when tokens is 0. */
void shell_ai_latency(uint32_t ticks, uint32_t hz, uint32_t tokens,
                      uint32_t *total_ms, uint32_t *per_token_ms);

#endif
