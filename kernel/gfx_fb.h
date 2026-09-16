#ifndef GFX_FB_H
#define GFX_FB_H

#include <stdint.h>
#include "os_syscalls.h"
#include "gfx_desktop.h"

int gfx_fb_present(const os_fb_scene_t *scene);
void gfx_fb_leave(void);
int gfx_fb_active(void);
int gfx_fb_width(void);
int gfx_fb_height(void);
int gfx_fb_parse_fit_line(const char *s, int *w, int *h);

#endif
