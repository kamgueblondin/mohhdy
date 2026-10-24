/* gfx_term.h - the text console drawn in a window of the VBE desktop.
 *
 * There is one console core (kernel/vga_console.c: cells, scrollback,
 * cursor, colours) fed by one shell (userspace/shell.c) and one keyboard
 * queue. This file is only a second backend for the same 80x25 cell grid:
 * the VGA text page is the first. It renders cells with the VGA font saved
 * from plane 2, so both backends show the same glyphs and attributes. */
#ifndef GFX_TERM_H
#define GFX_TERM_H
#include <stdint.h>

#define GFX_TERM_COLS 80      /* default grid (text mode size) */
#define GFX_TERM_ROWS 25
#define GFX_TERM_MAX_COLS 240 /* = VGA_MAX_COLS */
#define GFX_TERM_MAX_ROWS 80  /* = VGA_MAX_ROWS */
#define GFX_TERM_CELL_W 8
#define GFX_TERM_CELL_H 16
#define GFX_TERM_TITLE_H 20
#define GFX_TERM_FONT_BYTES (256 * 32)

typedef struct {
    int maximized;  /* window covers the whole screen */
    int scale;      /* integer zoom of the cell grid (1 or more) */
    int x, y;       /* window top left, title bar included */
    int w, h;       /* window size, title bar included */
    int gx, gy;     /* top left of the cell grid */
    int cols, rows; /* grid size: 80x25 in a window, the screen when maximized */
} gfx_term_layout_t;

/* Centered window; scale grows with the framebuffer. The window grid is
 * 80x25 (smaller if the screen is too small); maximized, the grid takes
 * the whole screen at the same scale. */
void gfx_term_layout(int fb_w, int fb_h, gfx_term_layout_t *out);
/* Same, moved by (dx, dy) from the centre (clamped on screen), or covering
 * the screen when maximized. */
void gfx_term_layout_at(int fb_w, int fb_h, int maximized, int dx, int dy, gfx_term_layout_t *out);

/* Title bar buttons, right to left: close, maximize, minimize. */
#include "gfx_wm.h"
#define GFX_TERM_BTN GFX_WM_BTN
#define GFX_TERM_HIT_NONE GFX_WM_HIT_NONE
#define GFX_TERM_HIT_GRID GFX_WM_HIT_BODY
#define GFX_TERM_HIT_TITLE GFX_WM_HIT_TITLE
#define GFX_TERM_HIT_MIN GFX_WM_HIT_MIN
#define GFX_TERM_HIT_MAX GFX_WM_HIT_MAX
#define GFX_TERM_HIT_CLOSE GFX_WM_HIT_CLOSE
int gfx_term_hit(const gfx_term_layout_t *l, int x, int y);
/* Top left of button GFX_TERM_HIT_MIN/MAX/CLOSE. */
void gfx_term_button_pos(const gfx_term_layout_t *l, int which, int *x, int *y);

/* VGA 16 colour palette as 0x00RRGGBB. */
uint32_t gfx_term_rgb(unsigned idx);

/* Paint the frame and title bar. */
void gfx_term_draw_frame(uint32_t *fb, int fb_w, int fb_h, int pitch,
                         const gfx_term_layout_t *l, const uint8_t *font, const char *title);

/* Paint cell i of a VGA cell array (char | attr << 8). */
void gfx_term_draw_cell(uint32_t *fb, int fb_w, int fb_h, int pitch,
                        const gfx_term_layout_t *l, const uint8_t *font, int i, uint16_t cell);

/* Paint every cell that differs from shown[] (all of them if full) and
 * update shown[]. Returns the number of cells painted. */
int gfx_term_render(uint32_t *fb, int fb_w, int fb_h, int pitch,
                    const gfx_term_layout_t *l, const uint8_t *font,
                    const uint16_t *cells, uint16_t *shown, int full);
/* Same, and reports the first and last grid row painted (-1 if none). */
int gfx_term_render_rows(uint32_t *fb, int fb_w, int fb_h, int pitch,
                         const gfx_term_layout_t *l, const uint8_t *font,
                         const uint16_t *cells, uint16_t *shown, int full,
                         int *row_min, int *row_max);
#endif
