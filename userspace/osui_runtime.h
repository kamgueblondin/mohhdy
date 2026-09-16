/* osui_runtime.h - runtime OS-UI Ring 3 (chat, scene VGA, sessions, MCP). */
#ifndef OSUI_RUNTIME_H
#define OSUI_RUNTIME_H

#define OSUI_OUT_MAX 2048

void osui_runtime_init(void);
int osui_is_command(const char *cmd);
int osui_is_linux_trap(const char *cmd);
int osui_dispatch_line(const char *line, char *out, int out_max);

int osui_guest_command_count(void);

#endif
