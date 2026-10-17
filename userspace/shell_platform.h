/* userspace/shell_platform.h - Phase 6 shell commands (docs/platform.md). */
#ifndef MOHHDY_SHELL_PLATFORM_H
#define MOHHDY_SHELL_PLATFORM_H
/* 1 if the first word is a platform command. */
int shell_platform_is(const char* line);
int shell_platform_line(const char* line);
/* Lot 9 (US-077): hook of the OS-UI VBE desktop for "mobile-ui ARGS". */
int shell_mobile_gui(const char* args, char* out, int out_max);

#endif
