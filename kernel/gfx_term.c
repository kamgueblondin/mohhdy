/* gfx_term.c - VBE backend of the text console cell grid (see gfx_term.h). */
#include "gfx_term.h"

static const uint32_t k_pal[16] = {
    0x000000, 0x0000AA, 0x00AA00, 0x00AAAA, 0xAA0000, 0xAA00AA, 0xAA5500, 0xAAAAAA,
    0x555555, 0x5555FF, 0x55FF55, 0x55FFFF, 0xFF5555, 0xFF55FF, 0xFFFF55, 0xFFFFFF
};

uint32_t gfx_term_rgb(unsigned idx) { return k_pal[idx & 15u]; }

void gfx_term_layout(int fb_w, int fb_h, gfx_term_layout_t *out) {
    gfx_term_layout_at(fb_w, fb_h, 0, 0, 0, out);
}

void gfx_term_layout_at(int fb_w, int fb_h, int maximized, int dx, int dy, gfx_term_layout_t *out) {
    int gw = GFX_TERM_COLS * GFX_TERM_CELL_W;
    int gh = GFX_TERM_ROWS * GFX_TERM_CELL_H;
    int s = 1;
    if (!out) return;
    out->maximized = maximized ? 1 : 0;
    while ((s + 1) * gw + 8 <= fb_w && (s + 1) * (gh + GFX_TERM_TITLE_H) + 8 <= fb_h) s++;
    out->scale = s;
    out->w = s * gw + 4;
    out->h = s * (gh + GFX_TERM_TITLE_H) + 4;
    if (out->w > fb_w) out->w = fb_w;
    if (out->h > fb_h) out->h = fb_h;
    out->x = (fb_w - out->w) / 2 + (maximized ? 0 : dx);
    out->y = (fb_h - out->h) / 2 + (maximized ? 0 : dy);
    if (out->x > fb_w - out->w) out->x = fb_w - out->w;
    if (out->y > fb_h - out->h) out->y = fb_h - out->h;
    if (out->x < 0) out->x = 0;
    if (out->y < 0) out->y = 0;
    out->gx = out->x + 2;
    out->gy = out->y + 2 + s * GFX_TERM_TITLE_H;
    if (maximized) {
        /* the window takes the screen, the grid stays centred inside */
        int inner_w = out->w, inner_h = out->h;
        out->x = 0; out->y = 0; out->w = fb_w; out->h = fb_h;
        out->gx = (fb_w - inner_w) / 2 + 2;
        out->gy = s * GFX_TERM_TITLE_H + 2 + (fb_h - inner_h) / 2;
    }
}

void gfx_term_button_pos(const gfx_term_layout_t *l, int which, int *x, int *y) {
    int s = l ? l->scale : 1;
    int k = which == GFX_TERM_HIT_CLOSE ? 1 : (which == GFX_TERM_HIT_MAX ? 2 : 3);
    int bx = l ? l->x + l->w - 2 - k * (GFX_TERM_BTN + 4) * s : 0;
    int by = l ? l->y + 2 + (GFX_TERM_TITLE_H * s - GFX_TERM_BTN * s) / 2 : 0;
    if (x) *x = bx;
    if (y) *y = by;
}

int gfx_term_hit(const gfx_term_layout_t *l, int x, int y) {
    int w;
    if (!l || x < l->x || y < l->y || x >= l->x + l->w || y >= l->y + l->h) return GFX_TERM_HIT_NONE;
    for (w = GFX_TERM_HIT_MIN; w <= GFX_TERM_HIT_CLOSE; w++) {
        int bx, by;
        gfx_term_button_pos(l, w, &bx, &by);
        if (x >= bx && y >= by && x < bx + GFX_TERM_BTN * l->scale && y < by + GFX_TERM_BTN * l->scale) return w;
    }
    if (y < l->y + 2 + l->scale * GFX_TERM_TITLE_H) return GFX_TERM_HIT_TITLE;
    return GFX_TERM_HIT_GRID;
}

static void put(uint32_t *fb, int w, int h, int pitch, int x, int y, uint32_t c) {
    if (x < 0 || y < 0 || x >= w || y >= h) return;
    fb[y * pitch + x] = c;
}

static void glyph(uint32_t *fb, int w, int h, int pitch, int x, int y, int s,
                  const uint8_t *font, unsigned char ch, uint32_t fg, uint32_t bg) {
    int r, c, dy, dx;
    for (r = 0; r < GFX_TERM_CELL_H; r++) {
        uint8_t bits = font ? font[(unsigned)ch * 32u + (unsigned)r] : 0;
        for (c = 0; c < GFX_TERM_CELL_W; c++) {
            uint32_t col = (bits & (0x80u >> c)) ? fg : bg;
            for (dy = 0; dy < s; dy++)
                for (dx = 0; dx < s; dx++)
                    put(fb, w, h, pitch, x + c * s + dx, y + r * s + dy, col);
        }
    }
}

void gfx_term_draw_frame(uint32_t *fb, int fb_w, int fb_h, int pitch,
                         const gfx_term_layout_t *l, const uint8_t *font, const char *title) {
    int x, y, i;
    uint32_t border = 0x3D9A6A, bar = 0x1F5C40;
    if (!fb || !l) return;
    for (y = l->y; y < l->y + l->h; y++)
        for (x = l->x; x < l->x + l->w; x++) {
            int edge = (x < l->x + 2 || x >= l->x + l->w - 2 || y < l->y + 2 || y >= l->y + l->h - 2);
            put(fb, fb_w, fb_h, pitch, x, y, edge ? border : (y < l->gy ? bar : 0));
        }
    {
        /* title bar buttons: minimize, maximize, close (red) */
        static const uint32_t col[3] = { 0x2E7D5B, 0x2E7D5B, 0xC0392B };
        static const char mark[3] = { '_', 'o', 'x' };
        int w, s = l->scale;
        for (w = GFX_TERM_HIT_MIN; w <= GFX_TERM_HIT_CLOSE; w++) {
            int bx, by, k = w - GFX_TERM_HIT_MIN;
            gfx_term_button_pos(l, w, &bx, &by);
            for (y = by; y < by + GFX_TERM_BTN * s; y++)
                for (x = bx; x < bx + GFX_TERM_BTN * s; x++)
                    put(fb, fb_w, fb_h, pitch, x, y, col[k]);
            glyph(fb, fb_w, fb_h, pitch, bx + 3 * s, by - s, s, font, (unsigned char)mark[k], 0xFFFFFF, col[k]);
        }
    }
    if (!title) return;
    for (i = 0; title[i] && (i + 1) * GFX_TERM_CELL_W * l->scale < l->w - 4 - 3 * (GFX_TERM_BTN + 4) * l->scale; i++)
        glyph(fb, fb_w, fb_h, pitch, l->gx + i * GFX_TERM_CELL_W * l->scale,
              l->y + 2 + l->scale * 2, l->scale, font, (unsigned char)title[i], 0xFFFFFF, bar);
}

void gfx_term_draw_cell(uint32_t *fb, int fb_w, int fb_h, int pitch,
                        const gfx_term_layout_t *l, const uint8_t *font, int i, uint16_t cell) {
    unsigned attr = (unsigned)(cell >> 8);
    int col = i % GFX_TERM_COLS, row = i / GFX_TERM_COLS;
    if (!fb || !l || i < 0 || i >= GFX_TERM_COLS * GFX_TERM_ROWS) return;
    glyph(fb, fb_w, fb_h, pitch,
          l->gx + col * GFX_TERM_CELL_W * l->scale, l->gy + row * GFX_TERM_CELL_H * l->scale,
          l->scale, font, (unsigned char)(cell & 0xFF), k_pal[attr & 15u], k_pal[(attr >> 4) & 7u]);
}

int gfx_term_render(uint32_t *fb, int fb_w, int fb_h, int pitch,
                    const gfx_term_layout_t *l, const uint8_t *font,
                    const uint16_t *cells, uint16_t *shown, int full) {
    return gfx_term_render_rows(fb, fb_w, fb_h, pitch, l, font, cells, shown, full, 0, 0);
}

int gfx_term_render_rows(uint32_t *fb, int fb_w, int fb_h, int pitch,
                         const gfx_term_layout_t *l, const uint8_t *font,
                         const uint16_t *cells, uint16_t *shown, int full,
                         int *row_min, int *row_max) {
    int i, n = 0, lo = -1, hi = -1;
    if (cells) {
        for (i = 0; i < GFX_TERM_COLS * GFX_TERM_ROWS; i++) {
            if (!full && shown && shown[i] == cells[i]) continue;
            gfx_term_draw_cell(fb, fb_w, fb_h, pitch, l, font, i, cells[i]);
            if (shown) shown[i] = cells[i];
            if (lo < 0) lo = i / GFX_TERM_COLS;
            hi = i / GFX_TERM_COLS;
            n++;
        }
    }
    if (row_min) *row_min = lo;
    if (row_max) *row_max = hi;
    return n;
}
