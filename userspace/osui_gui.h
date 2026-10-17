/* osui_gui.h - boucle gui guest + bureau VBE QEMU (pas une fenetre HTML). */
#ifndef OSUI_GUI_H
#define OSUI_GUI_H

#include <stdint.h>
#include "os_syscalls.h"

#define OSUI_SNAP_MAX 3072

typedef int (*osui_program_eval_t)(const char *line, char *captured, int cap_max);

void osui_gui_set_program_eval(osui_program_eval_t fn);

/* US-050: the full-screen pm-ide surface. The shell owns the file system
 * and the PromptMessage VM; the desktop owns the editor. load returns the
 * byte count or -1, save returns 0 or -1, exec (OSUI_IDE_CHECK or
 * OSUI_IDE_RUN) writes a one-line verdict in msg and returns 0 on success. */
typedef int (*osui_ide_load_t)(const char *file, char *buf, int max);
typedef int (*osui_ide_save_t)(const char *file, const char *buf, int len);
typedef int (*osui_ide_exec_t)(int action, const char *file, const char *src, int len, char *msg, int max);
void osui_gui_set_ide_ops(osui_ide_load_t load, osui_ide_save_t save, osui_ide_exec_t exec);
int osui_gui_ide_active(void);
void osui_gui_ide_frame_row(int r, char *dst, int max);
/* Program output of an OSUI_IDE_RUN, called by exec while the VM runs:
 * goes to the editor output pane. */
void osui_gui_ide_output(const char *text);
/* Lot 6 (US-031): "web FILE" on the desktop renders FILE with the guest
 * web engine (userspace/webeng.c) into the web pane. Uses the IDE loader. */
#define OSUI_WEB_SRC_MAX 4096
#define OSUI_WEB_VIEWPORT 640
int osui_gui_web_load(const char *file, char *status, int max);
int osui_gui_web_active(void);
int osui_gui_web_sanitized(void); /* Lot 7 US-039: items removed by the page sanitizer */
/* Lot 9 (US-077): a display list made elsewhere (the mobile shell) shown
 * in the web pane of the VBE desktop. "mobile-ui ARGS" typed on the
 * desktop goes to the hook, which calls osui_gui_show_items; while that
 * pane is up the desktop keeps the input line so taps can be typed. */
typedef int (*osui_mobile_op_t)(const char *args, char *out, int out_max);
void osui_gui_set_mobile_op(osui_mobile_op_t fn);
int osui_gui_show_items(const char *title, const os_fb_web_item_t *items, int n, int page_w, int page_h);
int osui_gui_mobile_active(void);
void osui_gui_term_reset(void);
int osui_gui_term_count(void);
void osui_gui_term_at(int i, char *dst, int max);
void osui_gui_term_push(const char *text);

void osui_gui_render(uint16_t *cells);
void osui_gui_fill_scene(os_fb_scene_t *scene);
void osui_gui_ascii_row(const uint16_t *cells, int y, char *dst, int max);
void osui_gui_write_snap(char *dst, int max);
int osui_gui_feed_key(int key, char *out, int out_max);
void osui_gui_set_input(const char *text);
const char *osui_gui_input(void);
void osui_gui_run(void);

#endif
