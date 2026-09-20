#ifndef GFX_DESKTOP_H
#define GFX_DESKTOP_H

#include <stdint.h>
#include "os_syscalls.h"

#define GFX_FB_WIDTH 1024
#define GFX_FB_HEIGHT 768
#define GFX_FB_MIN_WIDTH 640
#define GFX_FB_MIN_HEIGHT 400
#define GFX_FB_MAX_WIDTH 1920
#define GFX_FB_MAX_HEIGHT 1200

void gfx_desktop_draw(const os_fb_scene_t *scene, uint32_t *fb, int w, int h);
void gfx_desktop_draw_no_cursor(const os_fb_scene_t *scene, uint32_t *fb, int w, int h);
void gfx_desktop_draw_cursor(uint32_t *fb, int w, int h, int pitch, int mx, int my, uint8_t buttons);
uint32_t gfx_desktop_pixel(const uint32_t *fb, int w, int h, int x, int y);

void gfx_desktop_set_mouse(int x, int y, uint8_t buttons);
void gfx_desktop_get_mouse(int *x, int *y, uint8_t *buttons);
void gfx_desktop_move_mouse(int dx, int dy, uint8_t buttons);

#endif
