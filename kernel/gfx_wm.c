/* gfx_wm.c - see gfx_wm.h. */
#include "gfx_wm.h"

void gfx_wm_button_pos(gfx_wm_rect_t win, int title_h, int scale, int which, int *x, int *y) {
    int k = which == GFX_WM_HIT_CLOSE ? 1 : (which == GFX_WM_HIT_MAX ? 2 : 3);
    if (scale < 1) scale = 1;
    if (x) *x = win.x + win.w - GFX_WM_PAD - k * (GFX_WM_BTN + 4) * scale;
    if (y) *y = win.y + GFX_WM_PAD + (title_h - GFX_WM_BTN * scale) / 2;
}

int gfx_wm_hit(gfx_wm_rect_t win, int title_h, int scale, int x, int y) {
    int b;
    if (scale < 1) scale = 1;
    if (win.w <= 0 || win.h <= 0) return GFX_WM_HIT_NONE;
    if (x < win.x || y < win.y || x >= win.x + win.w || y >= win.y + win.h) return GFX_WM_HIT_NONE;
    for (b = GFX_WM_HIT_MIN; b <= GFX_WM_HIT_CLOSE; b++) {
        int bx, by;
        gfx_wm_button_pos(win, title_h, scale, b, &bx, &by);
        if (x >= bx && y >= by && x < bx + GFX_WM_BTN * scale && y < by + GFX_WM_BTN * scale) return b;
    }
    if (y < win.y + GFX_WM_PAD + title_h) return GFX_WM_HIT_TITLE;
    return GFX_WM_HIT_BODY;
}

int gfx_wm_visible(const gfx_wm_win_t *s) { return !s || (!s->closed && !s->minimized); }

void gfx_wm_place(const gfx_wm_win_t *s, gfx_wm_rect_t base, gfx_wm_rect_t area, gfx_wm_rect_t *out) {
    gfx_wm_rect_t r = base;
    if (!out) return;
    if (s && !gfx_wm_visible(s)) { r.w = r.h = 0; *out = r; return; }
    if (s && s->maximized) { *out = area; return; }
    if (s) { r.x += s->dx; r.y += s->dy; }
    if (r.x + r.w > area.x + area.w) r.x = area.x + area.w - r.w;
    if (r.y + r.h > area.y + area.h) r.y = area.y + area.h - r.h;
    if (r.x < area.x) r.x = area.x;
    if (r.y < area.y) r.y = area.y;
    *out = r;
}

int gfx_wm_mouse(gfx_wm_win_t *s, gfx_wm_rect_t placed, gfx_wm_rect_t base, int title_h, int scale,
                 int x, int y, int pressed, int released) {
    if (!s) return GFX_WM_ACT_NONE;
    if (released && s->drag) {
        s->drag = 0;
        s->dx = (x - s->grab_x) - base.x;
        s->dy = (y - s->grab_y) - base.y;
        return GFX_WM_ACT_MOVED;
    }
    if (!pressed) return GFX_WM_ACT_NONE;
    switch (gfx_wm_hit(placed, title_h, scale, x, y)) {
    case GFX_WM_HIT_CLOSE:
        s->closed = 1; s->maximized = 0; s->drag = 0;
        return GFX_WM_ACT_CLOSE;
    case GFX_WM_HIT_MIN:
        s->minimized = 1; s->drag = 0;
        return GFX_WM_ACT_MINIMIZE;
    case GFX_WM_HIT_MAX:
        s->maximized = !s->maximized;
        return GFX_WM_ACT_MAXIMIZE;
    case GFX_WM_HIT_TITLE:
        if (s->maximized) return GFX_WM_ACT_BODY;
        s->drag = 1;
        s->grab_x = x - placed.x;
        s->grab_y = y - placed.y;
        return GFX_WM_ACT_DRAG;
    case GFX_WM_HIT_BODY:
        return GFX_WM_ACT_BODY;
    default:
        return GFX_WM_ACT_NONE;
    }
}

void gfx_wm_restore(gfx_wm_win_t *s) { if (s) { s->closed = 0; s->minimized = 0; } }

void gfx_wm_reset(gfx_wm_win_t *s) {
    if (!s) return;
    s->closed = s->minimized = s->maximized = 0;
    s->dx = s->dy = 0;
    s->drag = 0;
}

static void px(uint32_t *fb, int fw, int fh, int pitch, int x, int y, uint32_t c) {
    if (!fb || x < 0 || y < 0 || x >= fw || y >= fh) return;
    fb[y * pitch + x] = c;
}

void gfx_wm_draw_buttons(uint32_t *fb, int fw, int fh, int pitch, gfx_wm_rect_t win, int title_h, int scale) {
    static const uint32_t col[3] = { GFX_WM_RGB_MIN, GFX_WM_RGB_MAX, GFX_WM_RGB_CLOSE };
    int b, r, c, s = scale < 1 ? 1 : scale, n = GFX_WM_BTN * (scale < 1 ? 1 : scale);
    if (pitch <= 0) pitch = fw;
    for (b = GFX_WM_HIT_MIN; b <= GFX_WM_HIT_CLOSE; b++) {
        int bx, by;
        uint32_t ink = 0x101418u;
        gfx_wm_button_pos(win, title_h, scale, b, &bx, &by);
        for (r = 0; r < n; r++)
            for (c = 0; c < n; c++) {
                /* rounded corners: drop the 4 corner pixels */
                if ((r == 0 || r == n - 1) && (c == 0 || c == n - 1)) continue;
                px(fb, fw, fh, pitch, bx + c, by + r, col[b - GFX_WM_HIT_MIN]);
            }
        /* glyphs: '-' minimize, square maximize, 'x' close */
        for (r = 3 * s; r < n - 3 * s; r++) {
            if (b == GFX_WM_HIT_MIN) { px(fb, fw, fh, pitch, bx + r, by + n / 2, ink); px(fb, fw, fh, pitch, bx + r, by + n / 2 - 1, ink); }
            if (b == GFX_WM_HIT_MAX) {
                px(fb, fw, fh, pitch, bx + r, by + 3 * s, ink); px(fb, fw, fh, pitch, bx + r, by + n - 3 * s - 1, ink);
                px(fb, fw, fh, pitch, bx + 3 * s, by + r, ink); px(fb, fw, fh, pitch, bx + n - 3 * s - 1, by + r, ink);
            }
            if (b == GFX_WM_HIT_CLOSE) { px(fb, fw, fh, pitch, bx + r, by + r, ink); px(fb, fw, fh, pitch, bx + n - 1 - r, by + r, ink); }
        }
    }
}
