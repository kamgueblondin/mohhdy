#ifndef GFX_FB_H
#define GFX_FB_H

#include <stdint.h>
#include "os_syscalls.h"
#include "gfx_desktop.h"

int gfx_fb_present(const os_fb_scene_t *scene);
void gfx_fb_leave(void);
/* Show the text console in a VBE window (desktop must be live). */
int gfx_fb_term_enter(void);
int gfx_fb_term_active(void);
/* Mouse over the terminal window: title bar buttons and drag. */
int gfx_fb_term_minimized(void);
void gfx_fb_term_poll(void);
void gfx_fb_redraw_desktop(void);
void gfx_fb_term_mouse(int x, int y, uint8_t buttons, uint8_t prev);
int gfx_fb_active(void);
int gfx_fb_width(void);
int gfx_fb_height(void);
int gfx_fb_parse_fit_line(const char *s, int *w, int *h);
void gfx_fb_update_cursor(void);

#endif
