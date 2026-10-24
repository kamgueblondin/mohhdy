/* gfx_term.c - VBE backend of the text console cell grid (see gfx_term.h). */
#include "gfx_term.h"

static const uint32_t k_pal[16] = {
    0x000000, 0x0000AA, 0x00AA00, 0x00AAAA, 0xAA0000, 0xAA00AA, 0xAA5500, 0xAAAAAA,
    0x555555, 0x5555FF, 0x55FF55, 0x55FFFF, 0xFF5555, 0xFF55FF, 0xFFFF55, 0xFFFFFF
};

uint32_t gfx_term_rgb(unsigned idx) { return k_pal[idx & 15u]; }

void gfx_term_layout(int fb_w, int fb_h, gfx_term_layout_t *out) {
    int gw = GFX_TERM_COLS * GFX_TERM_CELL_W;
    int gh = GFX_TERM_ROWS * GFX_TERM_CELL_H;
    int s = 1;
    if (!out) return;
    while ((s + 1) * gw + 8 <= fb_w && (s + 1) * (gh + GFX_TERM_TITLE_H) + 8 <= fb_h) s++;
    out->scale = s;
    out->w = s * gw + 4;
    out->h = s * (gh + GFX_TERM_TITLE_H) + 4;
    if (out->w > fb_w) out->w = fb_w;
    if (out->h > fb_h) out->h = fb_h;
    out->x = (fb_w - out->w) / 2;
    out->y = (fb_h - out->h) / 2;
    if (out->x < 0) out->x = 0;
    if (out->y < 0) out->y = 0;
    out->gx = out->x + 2;
    out->gy = out->y + 2 + s * GFX_TERM_TITLE_H;
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
    if (!title) return;
    for (i = 0; title[i] && (i + 1) * GFX_TERM_CELL_W * l->scale < l->w - 4; i++)
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
    int i, n = 0;
    if (!cells) return 0;
    for (i = 0; i < GFX_TERM_COLS * GFX_TERM_ROWS; i++) {
        if (!full && shown && shown[i] == cells[i]) continue;
        gfx_term_draw_cell(fb, fb_w, fb_h, pitch, l, font, i, cells[i]);
        if (shown) shown[i] = cells[i];
        n++;
    }
    return n;
}
