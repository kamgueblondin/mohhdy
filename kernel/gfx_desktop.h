#ifndef GFX_DESKTOP_H
#define GFX_DESKTOP_H

#include <stdint.h>
#include "os_syscalls.h"

#define GFX_FB_WIDTH 1024
#define GFX_FB_HEIGHT 768

void gfx_desktop_draw(const os_fb_scene_t *scene, uint32_t *fb, int w, int h);
uint32_t gfx_desktop_pixel(const uint32_t *fb, int w, int h, int x, int y);

#endif
