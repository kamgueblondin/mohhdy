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

#define GFX_DESKTOP_MENUS 8      /* Chat ... Statut, Fichiers, Editeur */
#define GFX_DESKTOP_DOCK 8
/* windows of the z-order (gfx_desktop_layout_t.z) */
#define GFX_WIN_PANE 0
#define GFX_WIN_CHAT 1
#define GFX_WIN_APP0 2           /* first app window slot (UX4: any app) */
#define GFX_WIN_APP1 3
#define GFX_WIN_COUNT (2 + OS_FB_APPS)
#define GFX_IS_APP_WIN(w) ((w) >= GFX_WIN_APP0 && (w) < GFX_WIN_COUNT)
#define GFX_DESKTOP_APP_TITLE_H 30
#define GFX_APP_ROW_H 14
#define GFX_APP_TOOL_Y 36        /* toolbar text row, from the window top */
#define GFX_APP_GRID_Y 58        /* first grid row, from the window top */

typedef struct {
    int bar_h;
    gfx_rect_t menu_items[GFX_DESKTOP_MENUS];
    int menu_count;
    int chrome_q;            /* Lot 68: menu bar text scale (quarters) */
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
        /* Lot 69: Browser-OS address / search bar (has_addr set) */
        int has_addr;
        gfx_rect_t addr, addr_back, addr_fwd, addr_go;
    } pane_win;
    struct {
        int visible, minimized, maximized;
        gfx_rect_t box;      /* w = h = 0 when closed or minimized */
        gfx_rect_t base;
        gfx_rect_t input;
        gfx_rect_t send_btn;
        gfx_rect_t btn[3];   /* minimize, maximize, close */
    } chat_win;
    struct {
        int visible, minimized, maximized;
        gfx_rect_t box;      /* w = h = 0 when closed or minimized */
        gfx_rect_t base;
        gfx_rect_t btn[3];   /* minimize, maximize, close */
        gfx_rect_t tool;     /* toolbar band */
        gfx_rect_t grid;     /* content grid */
    } app_win[OS_FB_APPS];
    int z[GFX_WIN_COUNT];    /* drawing order, last on top */
    int focus;               /* GFX_WIN_* with the keyboard, -1 desktop */
    int menu_active;         /* index in menu_items of the active window, -1 none */
    int dock_state[GFX_DESKTOP_DOCK];   /* 0 nothing, 1 open, 2 minimized */
    int hover_dock;          /* dock icon under the pointer (tooltip), -1 */
    struct {
        gfx_rect_t box;
        gfx_rect_t icons[GFX_DESKTOP_DOCK];
    } dock;
    struct {
        int count; /* desktop icons drawn on the right (0 to 6) */
        gfx_rect_t icons[6];
    } desk;
} gfx_desktop_layout_t;

/* Lot 6 (US-031): page origin and size of the web pane. */
/* UX4: web pane back (0) / forward (1) buttons */
void gfx_desktop_web_nav(int w, int h, int which, gfx_rect_t *r);
void gfx_desktop_web_origin(int w, int h, int *ox, int *oy, int *pw, int *ph);
/* UX5: viewer image (copied; 0 or a bad image clears). Returns 0 / -1. */
int gfx_desktop_set_image(const os_fb_image_t *img);
/* Rectangle the image of slot occupies in box (0 size when none). */
void gfx_desktop_image_rect(const gfx_rect_t *box, gfx_rect_t *out);
void gfx_desktop_app_grid_fit(const gfx_rect_t *box, int *rows, int *cols);
/* Lot 68: text scale (quarters, include/os_ui_scale.h) */
void gfx_desktop_app_grid_fit_q(const gfx_rect_t *box, int q, int *rows, int *cols);
int gfx_desktop_app_row_h(int q);
int gfx_desktop_app_grid_y(int q);
int gfx_desktop_surface_q(const os_fb_scene_t *sc, int zslot, int w, int h);
int gfx_desktop_chrome_q(const os_fb_scene_t *sc, int w, int h);
int gfx_glyph_pixel(char ch, int q, int dx, int dy);
/* Lot 69: 0 native fonts (default), 1 smoothed 8x8 upscale */
void gfx_desktop_set_font_mode(int mode);
int gfx_desktop_font_mode(void);
int gfx_glyph_source_cell(int q);
int gfx_desktop_text_q(void);
void gfx_desktop_set_text_q(int q);
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
/* Dock labels (tooltips) and the user-facing name of an llm label. */
extern const char *const gfx_desktop_dock_names[GFX_DESKTOP_DOCK];
const char *gfx_desktop_llm_name(const char *llm);
#define GFX_DESKTOP_PANE_TITLE_H 30
#define GFX_DESKTOP_CHAT_TITLE_H 34

/* Lot 69 (US-080): dock side and compact menus from the scene */
void gfx_desktop_set_form(int dock_left, int compact_menu);
int gfx_desktop_dock_left(void);
int gfx_desktop_menu_compact(void);
#endif
