/* gfx_wm.c - see gfx_wm.h. */
#include "gfx_wm.h"

/* Lot 69: button side for a scale. scale >= 4 is in quarters (ui-scale,
 * 4 = 1x); 1..3 is the old integer multiplier. Capped so the buttons stay
 * inside the title bar whatever the scale. */
int gfx_wm_btn(int title_h, int scale) {
    int q = scale >= 4 ? scale : (scale < 1 ? 1 : scale) * 4;
    int n = (GFX_WM_BTN * q + 2) / 4;
    if (title_h > 0 && n > title_h - 6) n = title_h - 6;
    return n < GFX_WM_BTN ? GFX_WM_BTN : n;
}

void gfx_wm_button_pos(gfx_wm_rect_t win, int title_h, int scale, int which, int *x, int *y) {
    int k = which == GFX_WM_HIT_CLOSE ? 1 : (which == GFX_WM_HIT_MAX ? 2 : 3);
    int n = gfx_wm_btn(title_h, scale);
    if (x) *x = win.x + win.w - GFX_WM_PAD - k * (n + 4);
    if (y) *y = win.y + GFX_WM_PAD + (title_h - n) / 2;
}

int gfx_wm_hit(gfx_wm_rect_t win, int title_h, int scale, int x, int y) {
    int b, n = gfx_wm_btn(title_h, scale), g = n;
    if (win.w <= 0 || win.h <= 0) return GFX_WM_HIT_NONE;
    if (x < win.x || y < win.y || x >= win.x + win.w || y >= win.y + win.h) return GFX_WM_HIT_NONE;
    for (b = GFX_WM_HIT_MIN; b <= GFX_WM_HIT_CLOSE; b++) {
        int bx, by;
        gfx_wm_button_pos(win, title_h, scale, b, &bx, &by);
        /* 2 px of slack around each button: the hit box is never smaller
         * than what is drawn */
        if (x >= bx - 2 && y >= by - 2 && x < bx + n + 2 && y < by + n + 2) return b;
    }
    /* Lot 69: resize grip (bottom-right corner) and right / bottom edges */
    if (x >= win.x + win.w - g && y >= win.y + win.h - g) return GFX_WM_HIT_RESIZE;
    if (y >= win.y + GFX_WM_PAD + title_h && (x >= win.x + win.w - 5 || y >= win.y + win.h - 5)) return GFX_WM_HIT_RESIZE;
    if (y < win.y + GFX_WM_PAD + title_h) return GFX_WM_HIT_TITLE;
    return GFX_WM_HIT_BODY;
}

int gfx_wm_visible(const gfx_wm_win_t *s) { return !s || (!s->closed && !s->minimized); }

void gfx_wm_place(const gfx_wm_win_t *s, gfx_wm_rect_t base, gfx_wm_rect_t area, gfx_wm_rect_t *out) {
    gfx_wm_rect_t r = base;
    if (!out) return;
    if (s && !gfx_wm_visible(s)) { r.w = r.h = 0; *out = r; return; }
    if (s && s->maximized) { *out = area; return; }
    if (s) {
        r.x += s->dx; r.y += s->dy;
        r.w += s->dw; r.h += s->dh;
        if (r.w < GFX_WM_MIN_W) r.w = GFX_WM_MIN_W;
        if (r.h < GFX_WM_MIN_H) r.h = GFX_WM_MIN_H;
        if (r.w > area.w) r.w = area.w;
        if (r.h > area.h) r.h = area.h;
    }
    if (r.x + r.w > area.x + area.w) r.x = area.x + area.w - r.w;
    if (r.y + r.h > area.y + area.h) r.y = area.y + area.h - r.h;
    if (r.x < area.x) r.x = area.x;
    if (r.y < area.y) r.y = area.y;
    *out = r;
}

int gfx_wm_mouse(gfx_wm_win_t *s, gfx_wm_rect_t placed, gfx_wm_rect_t base, int title_h, int scale,
                 int x, int y, int pressed, int released) {
    if (!s) return GFX_WM_ACT_NONE;
    if (released && s->resize) {
        s->resize = 0;
        s->dw += x - s->grab_x;
        s->dh += y - s->grab_y;
        if (base.w + s->dw < GFX_WM_MIN_W) s->dw = GFX_WM_MIN_W - base.w;
        if (base.h + s->dh < GFX_WM_MIN_H) s->dh = GFX_WM_MIN_H - base.h;
        return GFX_WM_ACT_RESIZED;
    }
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
    case GFX_WM_HIT_RESIZE:
        if (s->maximized) return GFX_WM_ACT_BODY;
        s->resize = 1;
        s->grab_x = x;
        s->grab_y = y;
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
    s->dw = s->dh = 0;
    s->drag = s->resize = 0;
}

static void px(uint32_t *fb, int fw, int fh, int pitch, int x, int y, uint32_t c) {
    if (!fb || x < 0 || y < 0 || x >= fw || y >= fh) return;
    fb[y * pitch + x] = c;
}

void gfx_wm_draw_buttons(uint32_t *fb, int fw, int fh, int pitch, gfx_wm_rect_t win, int title_h, int scale) {
    static const uint32_t col[3] = { GFX_WM_RGB_MIN, GFX_WM_RGB_MAX, GFX_WM_RGB_CLOSE };
    int b, r, c, n = gfx_wm_btn(title_h, scale), s = n / GFX_WM_BTN < 1 ? 1 : n / GFX_WM_BTN;
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

/* Lot 69: resize grip, three short diagonals in the bottom-right corner */
void gfx_wm_draw_grip(uint32_t *fb, int fw, int fh, int pitch, gfx_wm_rect_t win, int title_h, int scale, uint32_t ink) {
    int n = gfx_wm_btn(title_h, scale), k, i;
    if (pitch <= 0) pitch = fw;
    if (win.w <= 0 || win.h <= 0) return;
    for (k = 1; k <= 3; k++) {
        int len = k * n / 3;
        for (i = 0; i < len; i++) {
            px(fb, fw, fh, pitch, win.x + win.w - 4 - i, win.y + win.h - 4 - (len - 1 - i), ink);
            px(fb, fw, fh, pitch, win.x + win.w - 5 - i, win.y + win.h - 4 - (len - 1 - i), ink);
        }
    }
}
