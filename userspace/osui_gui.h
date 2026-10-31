/* osui_gui.h - boucle gui guest + bureau VBE QEMU (pas une fenetre HTML). */
#ifndef OSUI_GUI_H
#define OSUI_GUI_H

#include <stdint.h>
#include "os_syscalls.h"

#define OSUI_SNAP_MAX 3072


/* The desktop loop ended to show the console terminal window. The pending
 * line (may be empty) runs at the shell prompt. take clears both. */
int osui_gui_term_requested(void);
int osui_gui_take_pending(char *dst, int max);
#define OSUI_GUI_ROUTE_HOST 0
#define OSUI_GUI_ROUTE_SHELL 1
#define OSUI_GUI_ROUTE_OSUI 2
int osui_gui_route_line(const char *line, const char *pane);

/* US-050: the full-screen pm-ide surface. The shell owns the file system
 * and the PromptMessage VM; the desktop owns the editor. load returns the
 * byte count or -1, save returns 0 or -1, exec (OSUI_IDE_CHECK or
 * OSUI_IDE_RUN) writes a one-line verdict in msg and returns 0 on success. */
typedef int (*osui_ide_load_t)(const char *file, char *buf, int max);
typedef int (*osui_ide_save_t)(const char *file, const char *buf, int len);
typedef int (*osui_ide_exec_t)(int action, const char *file, const char *src, int len, char *msg, int max);
void osui_gui_set_ide_ops(osui_ide_load_t load, osui_ide_save_t save, osui_ide_exec_t exec);
/* UX5: user heap (realloc-like) for big files and the editor line store. */
void osui_gui_set_heap(void *(*realloc_fn)(void *, unsigned));
int osui_gui_ide_active(void);
void osui_gui_ide_frame_row(int r, char *dst, int max);
/* Program output of an OSUI_IDE_RUN, called by exec while the VM runs:
 * goes to the editor output pane. */
void osui_gui_ide_output(const char *text);
/* Lot 6 (US-031): "web FILE" on the desktop renders FILE with the guest
 * web engine (userspace/webeng.c) into the web pane. Uses the IDE loader. */
/* Lot 67: 16 KiB (was 4 KiB) so the reader view of a real page fits */
#define OSUI_WEB_SRC_MAX 16384
#define OSUI_WEB_VIEWPORT 640
/* Lot 61: explicit viewport height for fixed boxes and the scroll bound. */
#define OSUI_WEB_VIEWPORT_H 480
int osui_gui_web_load(const char *file, char *status, int max);
int osui_gui_persist_read(const char *path, char *buf, int cap);
int osui_gui_persist_write(const char *path, const char *buf, int n);
/* Lot 67: "web https://..." (or http://) loads the page over the network
 * through this hook (set by the shell: shell_fetch_page, the same code as
 * the `fetch` command). Without a hook a URL is refused. Links of a
 * fetched page are absolute http(s) URLs and are followed the same way. */
typedef int (*osui_web_fetch_t)(const char *url, char *src, int src_max, char *final_url, int url_max,
                                char *status, int st_max);
void osui_gui_set_web_fetch(osui_web_fetch_t fn);
/* Lot 71: the fetched page kept by the shell (userspace/shell_fetch.c):
 * a long page shown by sections, its forms focused, typed and submitted.
 * Every member may be 0 (local pages, unit tests). */
typedef struct {
    int (*sections)(void);
    int (*current)(void);
    int (*section)(int k, char *src, int src_max);
    int (*form_link)(const char *href);   /* 0 focus, 1 submit button, -1 other link */
    int (*form_key)(int key);             /* 0 edited, 1 Enter, 2 ESC, -1 unused */
    int (*form_set)(int n, const char *value);
    int (*submit)(int n, char *src, int src_max, char *final_url, int url_max, char *status, int st_max);
    int (*forms_list)(char *out, int cap);
} osui_web_page_ops_t;
void osui_gui_set_web_page_ops(const osui_web_page_ops_t *ops);
/* "web-open URL|FILE", "web-forms", "web-section N|next|prev", "web-input N TEXT", "web-submit [N]": the
 * console and the GUI input line run the same code. 1 when handled. */
int osui_gui_web_command(const char *line, char *out, int out_max);
/* Lot 69: URL of result n (1-based) of the last search; 0 or -1 */
typedef int (*osui_search_url_t)(int n, char *url, int max);
void osui_gui_set_search_url(osui_search_url_t fn);
int osui_gui_web_is_url(const char *s);
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
void osui_gui_browser_inspect(void);
void osui_gui_vfs_inspect(const char *path);
void osui_gui_term_reset(void);
int osui_gui_term_count(void);
void osui_gui_term_at(int i, char *dst, int max);
void osui_gui_term_push(const char *text);

void osui_gui_render(uint16_t *cells);
void osui_gui_fill_scene(os_fb_scene_t *scene);
void osui_gui_ascii_row(const uint16_t *cells, int y, char *dst, int max);
void osui_gui_write_snap(char *dst, int max);
void osui_gui_set_scale_hint(int q);  /* Lot 69: first-boot scale pin, 0 = auto */
int osui_gui_feed_key(int key, char *out, int out_max);
/* Lot 68: shared text scale state (console ui-scale / text-size and GUI
 * Ctrl+'+'/'-'/'0'), and the window a zoom key applies to (OS_UI_Z_*). */
int osui_gui_zoom_target(void);
struct osui_scale_s *osui_gui_scale(void);
int osui_gui_scale_command(const char *line, char *out, int cap);
void osui_gui_set_input(const char *text);
const char *osui_gui_input(void);
void osui_gui_run(void);

#include "osui_panes.h"
void osui_gui_set_pane_source(const osui_pane_src_t *src);

/* Desktop apps (scene version 7): file explorer (slot 0) and text editor
 * (slot 1) over the shared file library, the same one as the shell
 * commands. "/files [DIR]" and "/edit [FILE]" typed on the desktop open
 * them; the dock icons do the same. */
#include "fsops.h"
void osui_gui_set_fs_io(const fsops_io_t *io);
int osui_gui_files_open(const char *dir, char *out, int out_max);
int osui_gui_edit_open(const char *file, char *out, int out_max);
/* Kernel compositor event (line after OS_FB_EVT): focus, app clicks. */
void osui_gui_event(const char *line, char *out, int out_max);
int osui_gui_focus(void);           /* focused app slot, -1 desktop */
/* UX4 tool apps (osui_tools.c): "/tasks", "/view F", "/settings",
 * "/peers", "/pmrun F", "/chat". run: allow-listed shell command with
 * captured output; ai_step: one GPT-2 step; screen: framebuffer size. */
void osui_gui_set_tool_known(int (*known)(const char *));
void osui_gui_set_tool_ops(int (*run)(const char *, char *, int),
                           int (*ai_step)(const char *, int, char *, int),
                           const char *(*ai_model)(void), int (*screen)(int *, int *));
int osui_gui_tool_open(int kind, const char *arg, char *out, int out_max);
/* idle tick of the desktop loop: chat streaming; 1 when the view changed */
int osui_gui_idle(char *out, int out_max);
/* UX4 browser: resolve a link of page base to a local page path; < 0
 * for a scheme (javascript:, http:), a query or a fragment */
int osui_gui_web_resolve(const char *base, const char *href, char *dst, int max);
#endif
