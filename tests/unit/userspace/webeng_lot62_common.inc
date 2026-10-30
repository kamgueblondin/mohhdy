/* Lot 62 helpers: a host copy of the web pane painter (white canvas,
 * RECT fills, BORDER strokes, 8x8 glyphs from kernel/gfx_font8.h), so the
 * tests read the colour a pixel really gets. */
#include "../kernel/gfx_font8.h"

static uint32_t l62_pixel(int x, int y) {
    uint32_t i, color = 0xFFFFFF;
    for (i = 0; i < g_doc.nitems; i++) {
        const os_fb_web_item_t *it = &g_doc.items[i];
        if (it->kind == OS_FB_WEB_TEXT) {
            int k, pass;
            for (k = 0; it->text[k] && k < OS_FB_WEB_TEXT_LEN; k++) {
                int ch = (unsigned char)it->text[k];
                if (ch < 32 || ch > 126) ch = '?';
                for (pass = 0; pass <= (it->bold ? 1 : 0); pass++) {
                    int gx = x - (it->x + k * 8 + pass), gy = y - it->y;
                    if (gx >= 0 && gx < 8 && gy >= 0 && gy < 8 && ((gfx_font8[ch - 32][gy] >> gx) & 1u))
                        color = it->color & 0xFFFFFF;
                }
            }
            continue;
        }
        if (x < it->x || y < it->y || x >= it->x + it->w || y >= it->y + it->h) continue;
        if (it->kind == OS_FB_WEB_RECT) color = it->color & 0xFFFFFF;
        else if (it->kind == OS_FB_WEB_BORDER) {
            int t = it->bold ? it->bold : 1;
            if (x < it->x + t || y < it->y + t || x >= it->x + it->w - t || y >= it->y + it->h - t) color = it->color & 0xFFFFFF;
        }
    }
    return color;
}

static int l62_items_with(uint32_t color) {
    uint32_t i;
    int count = 0;
    for (i = 0; i < g_doc.nitems; i++) if (g_doc.items[i].color == color) count++;
    return count;
}

static void l62_no_alpha_left(void) {
    uint32_t i;
    for (i = 0; i < g_doc.nitems; i++) TEST_ASSERT_EQUAL(0, (int)(g_doc.items[i].color >> 24));
}

/* The glyph pixel of 'H' at row 0 column 1 and a background pixel. */
#define L62_H_INK(x, y) ((x) + 1), (y)
#define L62_H_GAP(x, y) ((x) + 3), (y)

static void l62_hex(uint32_t expected, uint32_t actual, int line) {
    if (expected != actual) printf("  line %d: expected 0x%08X, got 0x%08X\n", line, (unsigned)expected, (unsigned)actual);
}
#define L62_HEX(expected, actual) do { uint32_t e_ = (uint32_t)(expected), a_ = (uint32_t)(actual); \
    l62_hex(e_, a_, __LINE__); TEST_ASSERT_EQUAL((long)e_, (long)a_); } while (0)
