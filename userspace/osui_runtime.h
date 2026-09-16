/* osui_runtime.h - runtime OS-UI Ring 3 (chat, scene VGA, sessions, MCP). */
#ifndef OSUI_RUNTIME_H
#define OSUI_RUNTIME_H

#define OSUI_OUT_MAX 2048
#define OSUI_CANVAS_ROWS 22
#define OSUI_CANVAS_COLS 78

void osui_runtime_init(void);
int osui_is_command(const char *cmd);
int osui_is_linux_trap(const char *cmd);
int osui_dispatch_line(const char *line, char *out, int out_max);

int osui_guest_command_count(void);

const char *osui_get_chat_mode(void);
const char *osui_get_pane(void);
const char *osui_get_stage_mode(void);
const char *osui_get_stage_kind(void);
const char *osui_get_session_id(void);
int osui_get_chat_x(void);
int osui_get_chat_y(void);
int osui_move_chat(int dx, int dy);
int osui_msg_count(void);
void osui_msg_at(int i, char *dst, int max);
void osui_canvas_row(int r, char *dst, int max);
int osui_gui_should_enter(void);
void osui_gui_ack_enter(void);
int osui_gui_should_leave(void);
void osui_gui_ack_leave(void);
int osui_stage_tick(char *out, int out_max);

#endif
