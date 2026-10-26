/* osui_tools.h - the "tool" apps of the desktop that reuse terminal
 * features: task manager, image/text viewer, settings, P2P, PromptMessage
 * run output and the GPT-2 chat. Each one is a window of the generic app
 * slots (osui_gui.c) drawn through osui_app_view_t.
 *
 * No feature is reimplemented here: the content comes from the real shell
 * commands (ps, mem, kill, service-status, date, p2p-*, pm-run, ai-model)
 * run through the injected src->run (the shell captures what the command
 * prints, see shell_app_run), from file bytes (src->read) and from the
 * GPT-2 step syscalls (src->ai_step). Freestanding, host testable. */
#ifndef OSUI_TOOLS_H
#define OSUI_TOOLS_H
#include "os_syscalls.h"
#include "osui_app.h"

#include "osui_form.h"

#define OSUI_TOOL_LINES 128
/* Lot 45 (US-G15): line store as wide as a maximized window */
#define OSUI_TOOL_COLS OSUI_APP_MAX_COLS
#define OSUI_TOOL_OUT 65536         /* captured command output per call (64 KiB, a note marks a cut) */
#define OSUI_TOOL_INPUT 72

typedef struct {
    /* runs one shell command (checked by osui_tool_cmd_check against the
     * shell registry) and captures its output; returns the bytes or < 0 */
    int (*run)(const char *cmd, char *out, int max);
    /* reads a whole file; returns the byte count or < 0 */
    int (*read)(const char *path, char *buf, int max);
    /* one GPT-2 generation step: first=1 starts from prompt; returns the
     * piece length, 0 at the end, < 0 error (OS_AI_CANCELLED on Esc) */
    int (*ai_step)(const char *prompt, int first, char *piece, int max);
    const char *(*ai_model)(void);
    /* framebuffer size, 0 when unknown */
    int (*screen)(int *w, int *h);
    /* UX5: whole file of any size (heap); returns the bytes, 0 = use read */
    const char *(*load)(const char *path, int *n);
    /* UX5: 1 when NAME is a command of the shell registry (builtin or the
     * generated mohhdy_shell_commands list); 0 = old fixed allowlist */
    int (*known)(const char *name);
    int (*stat)(const char *path, os_dirent_t *out);
} osui_tool_src_t;

enum { OSUI_TOOL_NONE = 0, OSUI_TOOL_CLOSE = 1, OSUI_TOOL_EDIT = 2, OSUI_TOOL_CHANGED = 3 };

typedef struct {
    int kind, active;
    char arg[64];                              /* viewer / pm-run file */
    char lines[OSUI_TOOL_LINES][OSUI_TOOL_COLS];
    unsigned char cls[OSUI_TOOL_LINES];        /* OSUI_APP_A_* of each line */
    unsigned char cont[OSUI_TOOL_LINES];       /* Lot 46: 1 = wrapped part of the line above */
    int n, top, sel;
    int mode;                                  /* tasks: 0 processes, 1 services */
    int armed_pid;                             /* tasks: kill confirmation */
    char input[OSUI_TOOL_INPUT];
    int ilen;
    char status[OSUI_TOOL_COLS];
    char log[400];                             /* serial log of the last action (Lot 46: + tail lines) */
    /* chat */
    int gen;                                   /* 1 generating */
    int steps, cur_len;
    char prompt[OSUI_TOOL_INPUT];
    int image;                                 /* viewer: 1 PGM/PPM image */
    int img_w, img_h, img_src_w, img_src_h;    /* shown / file size */
    unsigned img_gen;                          /* bumps on each new image */
    int img_fmt;                               /* Lot 45: IMGDEC_* (0: ASCII PNM path) */
    int zoom;                                  /* Lot 45: 0 fit, else percent */
    int pan_x, pan_y;                          /* Lot 45: source pixels at zoom */
    char last[OSUI_TOOL_INPUT + 40];           /* parity: last command run */
    int psel;                                  /* parity: menu line kept across runs */
    /* PR 4: Form support */
    osui_form_t form;
    int has_form;
    int vrows, vcols;                          /* Lot 45: window grid, 0 default */
    int dropped;                               /* lines scrolled out by the last run */
} osui_tool_t;

const char *osui_tool_toolbar(int kind);
const char *osui_tool_name(int kind);
int osui_tool_cmd_allowed(const char *cmd);
/* UX5 rights model of the GUI tools: 0 when cmd may run from a window,
 * else the reason. A command may run when it is in the shell registry
 * (known), has no shell metacharacter and is not terminal-only
 * (interactive, desktop, session or nesting commands). Rights are then
 * the kernel's: the command runs in the shell task, as when typed. With
 * known == 0 the old fixed allowlist applies. */
#define OSUI_CMD_OK 0
#define OSUI_CMD_EMPTY 1
#define OSUI_CMD_META 2
#define OSUI_CMD_UNKNOWN 3
#define OSUI_CMD_TERMINAL 4
int osui_tool_cmd_check(const char *cmd, int (*known)(const char *name));
const char *osui_tool_cmd_reason(int code);
/* Lot 46: the note a window adds after the output of the programs a
 * command launched (spawn, exec; kernel capture SYS_OUT_CAPTURE): bytes
 * dropped past OS_OUTCAP_MAX, programs not captured, programs still
 * running. Returns its length, 0 when there is nothing to say. */
int osui_tool_spawn_note(char *out, int max, const os_outcap_stat_t *st);
/* 1 when kind is a UX5 parity panel (osui_parity.h) */
int osui_tool_is_panel(int kind);
void osui_tool_open(osui_tool_t *t, int kind, const char *arg, const osui_tool_src_t *src);
void osui_tool_refresh(osui_tool_t *t, const osui_tool_src_t *src);
int osui_tool_tool(osui_tool_t *t, int b, const osui_tool_src_t *src);
int osui_tool_click(osui_tool_t *t, int row, int col, int dbl, const osui_tool_src_t *src);
int osui_tool_key(osui_tool_t *t, int key, const osui_tool_src_t *src);
/* chat: one generation step when generating; 1 when the view changed */
int osui_tool_idle(osui_tool_t *t, const osui_tool_src_t *src);
void osui_tool_render(const osui_tool_t *t, osui_app_view_t *v);
/* Lot 45 (US-G15): the window grid changed (maximize / restore / resize).
 * Rows follow at once; new text wraps at the new width; views without side
 * effects (menus, viewer, tasks, settings) are rebuilt at the new size.
 * Lot 46: the output already shown (command output, P2P, PromptMessage run)
 * is wrapped again at the new width (soft wraps joined, hard line ends
 * kept); the store keeps OSUI_TOOL_LINES lines. */
void osui_tool_set_view(osui_tool_t *t, int rows, int cols, const osui_tool_src_t *src);
/* first decimal number of a ps line (the PID), -1 none */
int osui_tool_line_pid(const char *line);
/* PGM/PPM (P2, P3, P5, P6) to an ASCII preview of cols x rows; returns
 * the row count, < 0 when the bytes are not such an image */
int osui_tool_image_ascii(const char *buf, int n, char out[][OSUI_APP_COLS], int cols, int rows);
/* UX5: PGM/PPM to RGB pixels, nearest-neighbour downscale to fit max_w x
 * max_h (aspect kept). Returns 0 and the size, < 0 when not an image. */
int osui_tool_image_decode(const char *buf, int n, unsigned char *rgb, int max_w, int max_h,
                           int *w, int *h, int *src_w, int *src_h);
/* UX5: the pixels of the viewer (valid while t->image). */
const unsigned char *osui_tool_image_rgb(void);
#endif
