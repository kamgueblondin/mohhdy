/* gfx_term.c - VBE backend of the text console cell grid (see gfx_term.h). */
#include "gfx_term.h"
#include "gfx_wm.h"

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
    int s = 1, cw, ch;
    if (!out) return;
    out->maximized = maximized ? 1 : 0;
    while ((s + 1) * gw + 8 <= fb_w && (s + 1) * (gh + GFX_TERM_TITLE_H) + 8 <= fb_h) s++;
    out->scale = s;
    cw = GFX_TERM_CELL_W * s;
    ch = GFX_TERM_CELL_H * s;
    if (maximized) {
        out->cols = (fb_w - 4) / cw;
        out->rows = (fb_h - 4 - GFX_TERM_TITLE_H * s) / ch;
    } else {
        out->cols = GFX_TERM_COLS;
        out->rows = GFX_TERM_ROWS;
        if (out->cols * cw + 4 > fb_w) out->cols = (fb_w - 4) / cw;
        if (out->rows * ch + GFX_TERM_TITLE_H * s + 4 > fb_h) out->rows = (fb_h - 4 - GFX_TERM_TITLE_H * s) / ch;
    }
    if (out->cols > GFX_TERM_MAX_COLS) out->cols = GFX_TERM_MAX_COLS;
    if (out->rows > GFX_TERM_MAX_ROWS) out->rows = GFX_TERM_MAX_ROWS;
    if (out->cols < 8) out->cols = 8;
    if (out->rows < 2) out->rows = 2;
    if (maximized) {
        out->x = 0; out->y = 0; out->w = fb_w; out->h = fb_h;
        out->gx = 2 + ((fb_w - 4) - out->cols * cw) / 2;
        out->gy = 2 + GFX_TERM_TITLE_H * s;
        return;
    }
    out->w = out->cols * cw + 4;
    out->h = out->rows * ch + GFX_TERM_TITLE_H * s + 4;
    if (out->w > fb_w) out->w = fb_w;
    if (out->h > fb_h) out->h = fb_h;
    out->x = (fb_w - out->w) / 2 + dx;
    out->y = (fb_h - out->h) / 2 + dy;
    if (out->x > fb_w - out->w) out->x = fb_w - out->w;
    if (out->y > fb_h - out->h) out->y = fb_h - out->h;
    if (out->x < 0) out->x = 0;
    if (out->y < 0) out->y = 0;
    out->gx = out->x + 2;
    out->gy = out->y + 2 + s * GFX_TERM_TITLE_H;
}

static gfx_wm_rect_t wm_rect(const gfx_term_layout_t *l) {
    gfx_wm_rect_t r;
    r.x = l->x; r.y = l->y; r.w = l->w; r.h = l->h;
    return r;
}

/* Title bar buttons: one implementation in gfx_wm.c for every window. */
void gfx_term_button_pos(const gfx_term_layout_t *l, int which, int *x, int *y) {
    if (!l) { if (x) *x = 0; if (y) *y = 0; return; }
    gfx_wm_button_pos(wm_rect(l), GFX_TERM_TITLE_H * l->scale, l->scale, which, x, y);
}

int gfx_term_hit(const gfx_term_layout_t *l, int x, int y) {
    if (!l) return GFX_TERM_HIT_NONE;
    return gfx_wm_hit(wm_rect(l), GFX_TERM_TITLE_H * l->scale, l->scale, x, y);
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
    gfx_wm_draw_buttons(fb, fb_w, fb_h, pitch, wm_rect(l), GFX_TERM_TITLE_H * l->scale, l->scale);
    if (!title) return;
    for (i = 0; title[i] && (i + 1) * GFX_TERM_CELL_W * l->scale < l->w - 4 - 3 * (GFX_TERM_BTN + 4) * l->scale; i++)
        glyph(fb, fb_w, fb_h, pitch, l->gx + i * GFX_TERM_CELL_W * l->scale,
              l->y + 2 + l->scale * 2, l->scale, font, (unsigned char)title[i], 0xFFFFFF, bar);
}

void gfx_term_draw_cell(uint32_t *fb, int fb_w, int fb_h, int pitch,
                        const gfx_term_layout_t *l, const uint8_t *font, int i, uint16_t cell) {
    unsigned attr = (unsigned)(cell >> 8);
    int col, row;
    if (!fb || !l || l->cols <= 0 || i < 0 || i >= l->cols * l->rows) return;
    col = i % l->cols;
    row = i / l->cols;
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
        for (i = 0; i < l->cols * l->rows; i++) {
            if (!full && shown && shown[i] == cells[i]) continue;
            gfx_term_draw_cell(fb, fb_w, fb_h, pitch, l, font, i, cells[i]);
            if (shown) shown[i] = cells[i];
            if (lo < 0) lo = i / l->cols;
            hi = i / l->cols;
            n++;
        }
    }
    if (row_min) *row_min = lo;
    if (row_max) *row_max = hi;
    return n;
}

int gfx_term_cell_at(const gfx_term_layout_t *l, int x, int y, int *col, int *row) {
    int cw, ch, c, r;
    if (!l || l->scale <= 0) return 0;
    cw = GFX_TERM_CELL_W * l->scale;
    ch = GFX_TERM_CELL_H * l->scale;
    if (x < l->gx || y < l->gy) return 0;
    c = (x - l->gx) / cw;
    r = (y - l->gy) / ch;
    if (c >= l->cols || r >= l->rows) return 0;
    if (col) *col = c;
    if (row) *row = r;
    return 1;
}

int gfx_term_sel_text(const uint16_t *cells, int cols, int rows, int a, int b, char *out, int max) {
    int n = 0, i, r, t;
    if (!out || max <= 0) return 0;
    out[0] = 0;
    if (!cells || cols <= 0 || rows <= 0) return 0;
    if (a > b) { t = a; a = b; b = t; }
    if (a < 0) a = 0;
    if (b >= cols * rows) b = cols * rows - 1;
    for (r = a / cols; r <= b / cols; r++) {
        int from = r == a / cols ? a % cols : 0;
        int to = r == b / cols ? b % cols : cols - 1;
        int end = to;
        while (end >= from && ((char)(cells[r * cols + end] & 0xFF) == ' ' || (cells[r * cols + end] & 0xFF) == 0)) end--;
        if (r != a / cols && n < max - 1) out[n++] = '\n';
        for (i = from; i <= end && n < max - 1; i++) {
            char c = (char)(cells[r * cols + i] & 0xFF);
            out[n++] = (c >= 32 && c <= 126) ? c : ' ';
        }
    }
    out[n] = 0;
    return n;
}

void gfx_term_sel_apply(uint16_t *cells, int n, int a, int b) {
    int i, t;
    if (!cells) return;
    if (a > b) { t = a; a = b; b = t; }
    for (i = a < 0 ? 0 : a; i <= b && i < n; i++) {
        uint16_t v = cells[i], at = (uint16_t)(v >> 8);
        at = (uint16_t)(((at & 0x0F) << 4) | ((at >> 4) & 0x0F));
        cells[i] = (uint16_t)((v & 0xFF) | (at << 8));
    }
}
