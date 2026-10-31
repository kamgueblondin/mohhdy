/* osui_scale.h - Lot 68: text / UI scale of the Mohhdy desktop (Ring 3).
 * One state shared by the console command (`ui-scale`, `text-size`) and the
 * GUI (Ctrl+'+', Ctrl+'-', Ctrl+'0' on the focused window). The kernel
 * renderer reads it from os_fb_scene_t (ui_scale_q, zoom_q) and scales the
 * 8x8 font and the layout metrics (include/os_ui_scale.h). */
#ifndef OSUI_SCALE_H
#define OSUI_SCALE_H
#include <stdint.h>
#include "../include/os_ui_scale.h"

#define OSUI_SCALE_CFG "/persist/ui.cfg"

typedef struct osui_scale_s {
    uint8_t ui_q;                     /* 0 = auto */
    uint8_t zoom[OS_UI_Z_COUNT];      /* per window, 4 = none */
    int fb_w, fb_h;                   /* last screen size reported by the kernel */
    int web_pw, web_ph, web_q;        /* web view size (screen px) and scale */
    uint32_t changes;                 /* bumps on every change (persist when dirty) */
    uint32_t saved_at;                /* changes value written last */
} osui_scale_t;

void osui_scale_init(osui_scale_t *s);
/* global scale, quarters (0 = auto); returns 0 or -1 out of range */
int osui_scale_set_ui(osui_scale_t *s, int q);
/* zoom of one window: dir +1 / -1 step, 0 reset; returns the new zoom */
int osui_scale_zoom(osui_scale_t *s, int slot, int dir);
/* effective scale of a window on the last known screen (1024x768 if none) */
int osui_scale_effective(const osui_scale_t *s, int slot);
/* the kernel reported "web view PW PH Q FBW FBH" */
void osui_scale_web_view(osui_scale_t *s, int pw, int ph, int q, int fbw, int fbh);
/* page viewport for the web engine in CSS px (pw * 4 / q, at least 240) */
int osui_scale_web_viewport(const osui_scale_t *s, int fallback);
/* window name -> slot ("web", "chat", "shell", "pane", "ide", "app0".."app5"), -1 */
int osui_scale_slot(const char *name);
const char *osui_scale_slot_name(int slot);
/* "ui_scale=6 zoom=4,4,8,..." config text and back; returns length / 0 or -1 */
int osui_scale_save(const osui_scale_t *s, char *buf, int cap);
int osui_scale_load(osui_scale_t *s, const char *buf, int len);
/* the console / GUI command; writes a one-line report to out.
 *   ui-scale                 show
 *   ui-scale auto|1|1.5|2|3|150%|+|-|0
 *   text-size [WINDOW] N|+|-|0
 * returns 0 on success, 1 on a usage error, -1 if not a scale command. */
int osui_scale_command(osui_scale_t *s, const char *line, char *out, int cap);
#endif
