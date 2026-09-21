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

typedef struct {
    int x, y, w, h;
} gfx_rect_t;

typedef struct {
    int bar_h;
    gfx_rect_t menu_items[7];
    int menu_count;
    gfx_rect_t stage_badge;
    struct {
        gfx_rect_t box;
        gfx_rect_t pills[3];
    } scene_ia;
    struct {
        int active;
        int focused;
        gfx_rect_t box;
        gfx_rect_t titlebar;
        gfx_rect_t traffic;
    } pane_win;
    struct {
        gfx_rect_t box;
        gfx_rect_t input;
        gfx_rect_t send_btn;
    } chat_win;
    struct {
        gfx_rect_t box;
        gfx_rect_t icons[7];
    } dock;
} gfx_desktop_layout_t;

void gfx_desktop_get_layout(const os_fb_scene_t *scene, int w, int h, gfx_desktop_layout_t *layout);
void gfx_desktop_draw(const os_fb_scene_t *scene, uint32_t *fb, int w, int h);
void gfx_desktop_draw_no_cursor(const os_fb_scene_t *scene, uint32_t *fb, int w, int h);
void gfx_desktop_draw_cursor(uint32_t *fb, int w, int h, int pitch, int mx, int my, uint8_t buttons);
uint32_t gfx_desktop_pixel(const uint32_t *fb, int w, int h, int x, int y);

void gfx_desktop_set_mouse(int x, int y, uint8_t buttons);
void gfx_desktop_get_mouse(int *x, int *y, uint8_t *buttons);
void gfx_desktop_move_mouse(int dx, int dy, uint8_t buttons);
void gfx_desktop_clamp_mouse(void);


/* OS-UI-2-W: native pane focus / drag offset (kernel-local). */
int gfx_desktop_pane_focused(void);
void gfx_desktop_get_pane_offset(int *dx, int *dy);
void gfx_desktop_reset_pane_windowing(void);

#endif
