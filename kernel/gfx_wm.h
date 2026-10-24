/* gfx_wm.h - one window manager for every desktop window: the console
 * terminal window (gfx_fb.c), the OS-UI pane windows and the chat window
 * (gfx_desktop.c). Title bar buttons (geometry, hit test, drawing) and the
 * close / minimize / maximize / drag state machine live only here. */
#ifndef GFX_WM_H
#define GFX_WM_H
#include <stdint.h>

typedef struct { int x, y, w, h; } gfx_wm_rect_t;

#define GFX_WM_BTN 14       /* button side at scale 1 */
#define GFX_WM_PAD 2        /* frame border */
#define GFX_WM_HIT_NONE 0
#define GFX_WM_HIT_BODY 1
#define GFX_WM_HIT_TITLE 2
#define GFX_WM_HIT_MIN 3
#define GFX_WM_HIT_MAX 4
#define GFX_WM_HIT_CLOSE 5

#define GFX_WM_ACT_NONE 0     /* click not on this window */
#define GFX_WM_ACT_BODY 1     /* click inside, nothing for the WM */
#define GFX_WM_ACT_CLOSE 2
#define GFX_WM_ACT_MINIMIZE 3
#define GFX_WM_ACT_MAXIMIZE 4 /* toggled, see maximized */
#define GFX_WM_ACT_DRAG 5     /* title grabbed */
#define GFX_WM_ACT_MOVED 6    /* drag released, dx/dy updated */

typedef struct {
    int closed, minimized, maximized;
    int dx, dy;               /* moved from the default place */
    int drag, grab_x, grab_y;
} gfx_wm_win_t;

/* Buttons right to left: close, maximize, minimize. title_h is the title
 * bar height below the top border. */
void gfx_wm_button_pos(gfx_wm_rect_t win, int title_h, int scale, int which, int *x, int *y);
int gfx_wm_hit(gfx_wm_rect_t win, int title_h, int scale, int x, int y);
/* Where the window is: base moved by dx/dy and kept inside area, or the
 * whole area when maximized. w = h = 0 when closed or minimized. */
void gfx_wm_place(const gfx_wm_win_t *s, gfx_wm_rect_t base, gfx_wm_rect_t area, gfx_wm_rect_t *out);
int gfx_wm_visible(const gfx_wm_win_t *s);
/* Mouse edge on a placed window. base is the unmoved place (for dx/dy). */
int gfx_wm_mouse(gfx_wm_win_t *s, gfx_wm_rect_t placed, gfx_wm_rect_t base, int title_h, int scale,
                 int x, int y, int pressed, int released);
void gfx_wm_restore(gfx_wm_win_t *s);   /* un-minimize / reopen */
void gfx_wm_reset(gfx_wm_win_t *s);     /* default place and state */
void gfx_wm_draw_buttons(uint32_t *fb, int fw, int fh, int pitch, gfx_wm_rect_t win, int title_h, int scale);
#define GFX_WM_RGB_CLOSE 0xE0524Bu
#define GFX_WM_RGB_MIN 0xE6B43Cu
#define GFX_WM_RGB_MAX 0x3DB45Au
#endif
