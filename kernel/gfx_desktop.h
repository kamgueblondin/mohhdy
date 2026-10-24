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
        int active;          /* a pane window is drawn (open, not minimized) */
        int minimized, maximized;
        gfx_rect_t box;
        gfx_rect_t base;     /* default place, before drag / maximize */
        gfx_rect_t traffic;  /* close button (kept name: old traffic light) */
        gfx_rect_t btn[3];   /* minimize, maximize, close */
    } pane_win;
    struct {
        int visible, minimized, maximized;
        gfx_rect_t box;      /* w = h = 0 when closed or minimized */
        gfx_rect_t base;
        gfx_rect_t input;
        gfx_rect_t send_btn;
        gfx_rect_t btn[3];   /* minimize, maximize, close */
    } chat_win;
    int menu_active;         /* index in menu_items of the active window, -1 none */
    int dock_state[7];       /* 0 nothing, 1 open, 2 minimized */
    struct {
        gfx_rect_t box;
        gfx_rect_t icons[7];
    } dock;
    struct {
        int count; /* desktop icons drawn on the right (0 to 6) */
        gfx_rect_t icons[6];
    } desk;
} gfx_desktop_layout_t;

/* Lot 6 (US-031): page origin and size of the web pane. */
void gfx_desktop_web_origin(int w, int h, int *ox, int *oy, int *pw, int *ph);
void gfx_desktop_get_layout(const os_fb_scene_t *scene, int w, int h, gfx_desktop_layout_t *layout);
void gfx_desktop_draw(const os_fb_scene_t *scene, uint32_t *fb, int w, int h);
void gfx_desktop_draw_no_cursor(const os_fb_scene_t *scene, uint32_t *fb, int w, int h);
void gfx_desktop_draw_cursor(uint32_t *fb, int w, int h, int pitch, int mx, int my, uint8_t buttons);
uint32_t gfx_desktop_pixel(const uint32_t *fb, int w, int h, int x, int y);

void gfx_desktop_set_mouse(int x, int y, uint8_t buttons);
void gfx_desktop_get_mouse(int *x, int *y, uint8_t *buttons);
void gfx_desktop_move_mouse(int dx, int dy, uint8_t buttons);
/* Window manager state of the pane and chat windows (gfx_wm.c). */
void gfx_desktop_wm_reset(void);
#define GFX_DESKTOP_PANE_TITLE_H 30
#define GFX_DESKTOP_CHAT_TITLE_H 34

#endif
