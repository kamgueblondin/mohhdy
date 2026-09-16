/* gfx_desktop.c - Compositor pixel du bureau Mohhdy (QEMU VBE, pas HTML). */
#include "gfx_desktop.h"
#include "gfx_font8.h"

#define RGB(r, g, b) ((uint32_t)(r) << 16 | (uint32_t)(g) << 8 | (uint32_t)(b))

static uint32_t pix(const uint32_t *fb, int w, int h, int x, int y) {
    if (!fb || x < 0 || y < 0 || x >= w || y >= h) return 0;
    return fb[y * w + x];
}

uint32_t gfx_desktop_pixel(const uint32_t *fb, int w, int h, int x, int y) {
    return pix(fb, w, h, x, y);
}

static void put(uint32_t *fb, int w, int h, int x, int y, uint32_t c) {
    if (!fb || x < 0 || y < 0 || x >= w || y >= h) return;
    fb[y * w + x] = c;
}

static uint32_t blend(uint32_t dst, uint32_t src, unsigned a) {
    unsigned dr = (dst >> 16) & 255u, dg = (dst >> 8) & 255u, db = dst & 255u;
    unsigned sr = (src >> 16) & 255u, sg = (src >> 8) & 255u, sb = src & 255u;
    unsigned ia = 255u - a;
    unsigned r = (sr * a + dr * ia) / 255u;
    unsigned g = (sg * a + dg * ia) / 255u;
    unsigned b = (sb * a + db * ia) / 255u;
    return RGB(r, g, b);
}

static int clampi(int v, int lo, int hi) {
    if (v < lo) return lo;
    if (v > hi) return hi;
    return v;
}

static void fill_rect(uint32_t *fb, int w, int h, int x, int y, int rw, int rh, uint32_t c) {
    int yy, xx, x1, y1;
    x1 = x + rw;
    y1 = y + rh;
    if (x < 0) x = 0;
    if (y < 0) y = 0;
    if (x1 > w) x1 = w;
    if (y1 > h) y1 = h;
    for (yy = y; yy < y1; yy++) {
        for (xx = x; xx < x1; xx++) fb[yy * w + xx] = c;
    }
}

static void fill_rect_a(uint32_t *fb, int w, int h, int x, int y, int rw, int rh, uint32_t c, unsigned a) {
    int yy, xx, x1, y1;
    x1 = x + rw;
    y1 = y + rh;
    if (x < 0) x = 0;
    if (y < 0) y = 0;
    if (x1 > w) x1 = w;
    if (y1 > h) y1 = h;
    for (yy = y; yy < y1; yy++) {
        for (xx = x; xx < x1; xx++) {
            fb[yy * w + xx] = blend(fb[yy * w + xx], c, a);
        }
    }
}

static int in_round_rect(int px, int py, int x, int y, int rw, int rh, int r) {
    int cx, cy, dx, dy;
    if (px < x || py < y || px >= x + rw || py >= y + rh) return 0;
    if (px >= x + r && px < x + rw - r) return 1;
    if (py >= y + r && py < y + rh - r) return 1;
    if (px < x + r && py < y + r) {
        cx = x + r;
        cy = y + r;
    } else if (px >= x + rw - r && py < y + r) {
        cx = x + rw - r - 1;
        cy = y + r;
    } else if (px < x + r && py >= y + rh - r) {
        cx = x + r;
        cy = y + rh - r - 1;
    } else {
        cx = x + rw - r - 1;
        cy = y + rh - r - 1;
    }
    dx = px - cx;
    dy = py - cy;
    return dx * dx + dy * dy <= r * r;
}

static void round_rect_a(uint32_t *fb, int w, int h, int x, int y, int rw, int rh, int r, uint32_t c, unsigned a) {
    int yy, xx, x0, y0, x1, y1;
    if (r < 1) r = 1;
    if (r * 2 > rw) r = rw / 2;
    if (r * 2 > rh) r = rh / 2;
    x0 = x < 0 ? 0 : x;
    y0 = y < 0 ? 0 : y;
    x1 = x + rw;
    y1 = y + rh;
    if (x1 > w) x1 = w;
    if (y1 > h) y1 = h;
    for (yy = y0; yy < y1; yy++) {
        for (xx = x0; xx < x1; xx++) {
            if (in_round_rect(xx, yy, x, y, rw, rh, r)) {
                fb[yy * w + xx] = blend(fb[yy * w + xx], c, a);
            }
        }
    }
}

static void round_rect(uint32_t *fb, int w, int h, int x, int y, int rw, int rh, int r, uint32_t c) {
    round_rect_a(fb, w, h, x, y, rw, rh, r, c, 255);
}

static void stroke_round(uint32_t *fb, int w, int h, int x, int y, int rw, int rh, int r, uint32_t c, int t) {
    int i;
    for (i = 0; i < t; i++) {
        int yy, xx;
        int x0 = x + i, y0 = y + i, rw0 = rw - i * 2, rh0 = rh - i * 2;
        int x1 = x0 + rw0, y1 = y0 + rh0;
        if (rw0 < 4 || rh0 < 4) return;
        for (yy = y0; yy < y1; yy++) {
            for (xx = x0; xx < x1; xx++) {
                if (in_round_rect(xx, yy, x0, y0, rw0, rh0, r > i ? r - i : 1)
                    && !in_round_rect(xx, yy, x0 + 1, y0 + 1, rw0 - 2, rh0 - 2, r > i + 1 ? r - i - 1 : 1)) {
                    put(fb, w, h, xx, yy, c);
                }
            }
        }
    }
}

static void draw_char(uint32_t *fb, int w, int h, int x, int y, char ch, uint32_t c) {
    int gy, gx;
    unsigned idx;
    const uint8_t *g;
    if ((unsigned char)ch < 32 || (unsigned char)ch > 126) ch = '?';
    idx = (unsigned char)ch - 32u;
    g = gfx_font8[idx];
    for (gy = 0; gy < 8; gy++) {
        uint8_t bits = g[gy];
        for (gx = 0; gx < 8; gx++) {
            if (bits & (1u << gx)) put(fb, w, h, x + gx, y + gy, c);
        }
    }
}

static void draw_text(uint32_t *fb, int w, int h, int x, int y, const char *s, uint32_t c) {
    int cx = x;
    if (!s) return;
    while (*s) {
        if (*s == '\n') {
            y += 10;
            cx = x;
        } else {
            draw_char(fb, w, h, cx, y, *s, c);
            cx += 8;
        }
        s++;
    }
}

static void draw_text_n(uint32_t *fb, int w, int h, int x, int y, const char *s, int maxc, uint32_t c) {
    int cx = x, n = 0;
    if (!s) return;
    while (*s && n < maxc) {
        draw_char(fb, w, h, cx, y, *s, c);
        cx += 8;
        s++;
        n++;
    }
}

static void landscape(uint32_t *fb, int w, int h) {
    int x, y;
    for (y = 0; y < h; y++) {
        int t = (y * 255) / (h > 1 ? h - 1 : 1);
        int r0 = 22 + (8 - 22) * t / 255;
        int g0 = 52 + (20 - 52) * t / 255;
        int b0 = 69 + (28 - 69) * t / 255;
        for (x = 0; x < w; x++) {
            int dx = x - w / 5;
            int dy = y - h / 14;
            int d2 = dx * dx / 48 + dy * dy / 18;
            int glow = 90 - d2;
            int dx2 = x - (w * 9) / 10;
            int dy2 = y - h / 5;
            int d22 = dx2 * dx2 / 70 + dy2 * dy2 / 40;
            int glow2 = 55 - d22;
            int warm = ((y - (h * 6) / 10) * 28) / (h / 3);
            int r = r0, g = g0, b = b0;
            if (glow < 0) glow = 0;
            if (glow2 < 0) glow2 = 0;
            if (warm < 0) warm = 0;
            if (warm > 28) warm = 28;
            r += glow / 6 + glow2 / 10 + warm;
            g += glow / 4 + glow2 / 8;
            b += glow / 5 + glow2 / 6;
            if (r > 255) r = 255;
            if (g > 255) g = 255;
            if (b > 255) b = 255;
            fb[y * w + x] = RGB(r, g, b);
        }
    }
}

static void icon_tile(uint32_t *fb, int w, int h, int x, int y, uint32_t c0, uint32_t c1, const char *label) {
    int yy, xx;
    for (yy = 0; yy < 42; yy++) {
        uint32_t c = blend(c0, c1, (unsigned)(yy * 180 / 42));
        for (xx = 0; xx < 42; xx++) {
            int dx = xx - 21, dy = yy - 21;
            if (dx * dx + dy * dy < 20 * 20 || (xx > 6 && xx < 36 && yy > 6 && yy < 36)) {
                if (in_round_rect(xx, yy, 0, 0, 42, 42, 10))
                    put(fb, w, h, x + xx, y + yy, c);
            }
        }
    }
    round_rect(fb, w, h, x, y, 42, 42, 10, c0);
    for (yy = 0; yy < 42; yy++) {
        uint32_t c = blend(c0, c1, (unsigned)(yy * 200 / 42));
        for (xx = 0; xx < 42; xx++) {
            if (in_round_rect(xx, yy, 0, 0, 42, 42, 10))
                put(fb, w, h, x + xx, y + yy, c);
        }
    }
    if (label) {
        int len = 0;
        while (label[len]) len++;
        draw_text(fb, w, h, x + 21 - (len * 8) / 2, y + 46, label, RGB(232, 238, 244));
    }
}

static void traffic(uint32_t *fb, int w, int h, int x, int y) {
    int i;
    uint32_t cols[3] = { RGB(201, 162, 39), RGB(61, 154, 138), RGB(196, 92, 92) };
    for (i = 0; i < 3; i++) {
        int cx = x + i * 14 + 5, cy = y + 5, dx, dy;
        for (dy = -5; dy <= 5; dy++) {
            for (dx = -5; dx <= 5; dx++) {
                if (dx * dx + dy * dy <= 20) put(fb, w, h, cx + dx, cy + dy, cols[i]);
            }
        }
    }
}

static void pill(uint32_t *fb, int w, int h, int x, int y, int rw, int rh, uint32_t c, const char *label, int muted) {
    unsigned a = muted ? 90u : 230u;
    round_rect_a(fb, w, h, x, y, rw, rh, rh / 2 > 6 ? 10 : 6, c, a);
    if (label) {
        int len = 0;
        while (label[len]) len++;
        draw_text(fb, w, h, x + (rw - len * 8) / 2, y + (rh - 8) / 2, label, RGB(232, 238, 244));
    }
}

static void draw_circle_kind(uint32_t *fb, int w, int h, int cx, int cy, int rad, uint32_t c) {
    int x, y;
    for (y = -rad - 2; y <= rad + 2; y++) {
        for (x = -rad - 2; x <= rad + 2; x++) {
            int d2 = x * x + y * y;
            int outer = rad * rad;
            int inner = (rad - 3) * (rad - 3);
            if (d2 <= outer && d2 >= inner) put(fb, w, h, cx + x, cy + y, c);
            if (d2 <= 36) put(fb, w, h, cx + x, cy + y, RGB(61, 154, 138));
        }
    }
}

static void draw_graph(uint32_t *fb, int w, int h, int x, int y, int rw, int rh) {
    int i, n = 6;
    int pts[8][2];
    int px, py, ox, oy;
    (void)rh;
    pts[0][0] = x + 8;
    pts[0][1] = y + 70;
    pts[1][0] = x + rw / 5;
    pts[1][1] = y + 30;
    pts[2][0] = x + (rw * 2) / 5;
    pts[2][1] = y + 50;
    pts[3][0] = x + (rw * 3) / 5;
    pts[3][1] = y + 16;
    pts[4][0] = x + (rw * 4) / 5;
    pts[4][1] = y + 40;
    pts[5][0] = x + rw - 8;
    pts[5][1] = y + 22;
    ox = pts[0][0];
    oy = pts[0][1];
    for (i = 1; i < n; i++) {
        int x0 = ox, y0 = oy, x1 = pts[i][0], y1 = pts[i][1];
        int dx = x1 - x0, dy = y1 - y0, steps, s;
        if (dx < 0) dx = -dx;
        if (dy < 0) dy = -dy;
        steps = dx > dy ? dx : dy;
        if (steps < 1) steps = 1;
        for (s = 0; s <= steps; s++) {
            px = x0 + (pts[i][0] - x0) * s / steps;
            py = y0 + (pts[i][1] - y0) * s / steps;
            put(fb, w, h, px, py, RGB(122, 212, 196));
            put(fb, w, h, px, py + 1, RGB(61, 154, 138));
        }
        ox = pts[i][0];
        oy = pts[i][1];
    }
}

static void draw_stage_body(uint32_t *fb, int w, int h, int x, int y, int rw, int rh, const os_fb_scene_t *sc) {
    uint8_t kind = sc ? sc->stage_kind : 0;
    uint8_t mode = sc ? sc->stage_mode : 0;
    int bw = (rw - 40) / 3;
    if (kind == OS_FB_KIND_CIRCLE) {
        draw_circle_kind(fb, w, h, x + rw / 2, y + rh / 2, 36, RGB(122, 212, 196));
        return;
    }
    if (kind == OS_FB_KIND_BOXES) {
        pill(fb, w, h, x + 8, y + 8, bw, 36, RGB(43, 122, 110), "A", 0);
        pill(fb, w, h, x + 16 + bw, y + 8, bw, 36, RGB(43, 122, 110), "B", 0);
        pill(fb, w, h, x + 24 + 2 * bw, y + 8, bw, 36, RGB(43, 122, 110), "C", 0);
        return;
    }
    if (kind == OS_FB_KIND_GRAPH) {
        draw_graph(fb, w, h, x, y, rw, rh);
        return;
    }
    if (kind == OS_FB_KIND_TREE) {
        draw_text(fb, w, h, x + 12, y + 8, "root", RGB(232, 238, 244));
        draw_text(fb, w, h, x + 28, y + 22, "shell.c", RGB(154, 168, 181));
        draw_text(fb, w, h, x + 28, y + 34, "osui_runtime.c", RGB(154, 168, 181));
        draw_text(fb, w, h, x + 28, y + 46, "vfs", RGB(154, 168, 181));
        return;
    }
    if (kind == OS_FB_KIND_CLOCK) {
        draw_circle_kind(fb, w, h, x + rw / 2, y + 40, 28, RGB(122, 212, 196));
        return;
    }
    if (kind == OS_FB_KIND_SIM || mode == OS_FB_STAGE_ACTING) {
        pill(fb, w, h, x + 16, y + 12, 70, 32, RGB(61, 154, 138), "1", 0);
        pill(fb, w, h, x + 110, y + 28, 70, 32, RGB(201, 162, 39), "2", 0);
        pill(fb, w, h, x + 204, y + 16, 70, 32, RGB(122, 212, 196), "3", 0);
        return;
    }
    pill(fb, w, h, x + 8, y + 8, bw, 40, RGB(43, 122, 110), "reflexion", mode != OS_FB_STAGE_REFLECTING);
    pill(fb, w, h, x + 16 + bw, y + 8, bw, 40, RGB(43, 122, 110), "action", mode != OS_FB_STAGE_ACTING);
    pill(fb, w, h, x + 24 + 2 * bw, y + 8, bw, 40, RGB(43, 122, 110), "resultats", mode != OS_FB_STAGE_PRESENTING);
}

static const char *pane_title(uint8_t pane) {
    switch (pane) {
    case OS_FB_PANE_BROWSER: return "Browser-OS";
    case OS_FB_PANE_SHELL: return "Shell Multiboot";
    case OS_FB_PANE_ADMIN: return "Admin";
    case OS_FB_PANE_SUPPORT: return "Support";
    case OS_FB_PANE_STATUS: return "Statut";
    case OS_FB_PANE_FS: return "Fichiers";
    default: return 0;
    }
}

static void draw_window(uint32_t *fb, int w, int h, int x, int y, int rw, int rh, const char *title, const char *body) {
    round_rect_a(fb, w, h, x, y, rw, rh, 10, RGB(18, 28, 36), 230);
    stroke_round(fb, w, h, x, y, rw, rh, 10, RGB(61, 154, 138), 1);
    fill_rect(fb, w, h, x, y, rw, 32, RGB(28, 44, 56));
    traffic(fb, w, h, x + 8, y + 8);
    draw_text(fb, w, h, x + 56, y + 12, title ? title : "", RGB(232, 238, 244));
    fill_rect_a(fb, w, h, x + 12, y + 44, rw - 24, 40, RGB(42, 36, 16), 220);
    draw_text(fb, w, h, x + 18, y + 48, "llm=stub_echo  us031_complete=false", RGB(243, 230, 176));
    if (body) draw_text(fb, w, h, x + 18, y + 96, body, RGB(201, 232, 223));
}

static void draw_chat(uint32_t *fb, int w, int h, int x, int y, int rw, int rh, const os_fb_scene_t *sc) {
    int log_y, i, n;
    const char *msg;
    round_rect_a(fb, w, h, x, y, rw, rh, 14, RGB(12, 18, 24), 210);
    stroke_round(fb, w, h, x, y, rw, rh, 14, RGB(61, 154, 138), 2);
    fill_rect(fb, w, h, x + 1, y + 1, rw - 2, 36, RGB(28, 44, 56));
    traffic(fb, w, h, x + 10, y + 10);
    draw_text(fb, w, h, x + 58, y + 14, "Mohhdy", RGB(232, 238, 244));
    draw_text(fb, w, h, x + rw - 8 * 14 - 12, y + 14, "llm=stub_echo", RGB(243, 230, 176));
    fill_rect_a(fb, w, h, x + 14, y + 46, rw - 28, 44, RGB(42, 36, 16), 230);
    draw_text(fb, w, h, x + 20, y + 50, "Surface de commande du SE Multiboot.", RGB(243, 230, 176));
    draw_text(fb, w, h, x + 20, y + 62, "llm=stub_echo  pas un moteur de production.", RGB(243, 230, 176));
    log_y = y + 98;
    fill_rect(fb, w, h, x + 14, log_y, rw - 28, rh - 180, RGB(11, 16, 20));
    n = sc ? (int)sc->nmsg : 0;
    if (n > OS_FB_MSG_MAX) n = OS_FB_MSG_MAX;
    if (n <= 0) {
        draw_text(fb, w, h, x + 20, log_y + 8, "os . llm=stub_echo", RGB(154, 168, 181));
        draw_text(fb, w, h, x + 20, log_y + 22, "Mohhdy OS - SE Multiboot dirige par prompts.", RGB(201, 232, 223));
        draw_text(fb, w, h, x + 20, log_y + 34, "us031_complete=false  phase3_complete=false", RGB(201, 232, 223));
    } else {
        for (i = 0; i < n; i++) {
            msg = sc->messages[i];
            draw_text(fb, w, h, x + 20, log_y + 8 + i * 22, "session locale, stub", RGB(154, 168, 181));
            draw_text_n(fb, w, h, x + 20, log_y + 18 + i * 22, msg, (rw - 48) / 8, RGB(232, 238, 244));
        }
    }
    fill_rect(fb, w, h, x + 14, y + rh - 70, rw - 28, 28, RGB(16, 25, 32));
    if (sc && sc->input[0]) draw_text_n(fb, w, h, x + 20, y + rh - 62, sc->input, (rw - 48) / 8, RGB(232, 238, 244));
    else draw_text(fb, w, h, x + 20, y + rh - 62, "Prompt ou /help, /browser, /shell ...", RGB(154, 168, 181));
    round_rect(fb, w, h, x + 14, y + rh - 34, 88, 22, 8, RGB(61, 154, 138));
    draw_text(fb, w, h, x + 26, y + rh - 28, "Envoyer", RGB(15, 20, 25));
    draw_text(fb, w, h, x + 110, y + rh - 28, sc && sc->session[0] ? sc->session : "s0001", RGB(154, 168, 181));
    draw_text(fb, w, h, x + 170, y + rh - 28, "stub", RGB(154, 168, 181));
}

void gfx_desktop_draw(const os_fb_scene_t *scene, uint32_t *fb, int w, int h) {
    os_fb_scene_t def;
    const os_fb_scene_t *sc = scene;
    int bar_h, chat_w, chat_h, chat_x, chat_y, scene_w, scene_h, scene_x, scene_y;
    int dock_w, dock_x, dock_y, i;
    uint8_t pane;
    const char *ptitle;
    const char *menus[7] = { "Chat", "Browser-OS", "Shell", "Admin", "Support", "Statut", "FS" };
    struct {
        uint32_t c0, c1;
        const char *label;
    } icons[6] = {
        { RGB(106, 160, 212), RGB(47, 95, 138), "Browser-OS" },
        { RGB(61, 154, 106), RGB(31, 92, 64), "Shell" },
        { RGB(212, 176, 106), RGB(138, 106, 47), "Admin" },
        { RGB(78, 182, 166), RGB(43, 122, 110), "Support" },
        { RGB(138, 160, 180), RGB(61, 77, 92), "Statut" },
        { RGB(196, 160, 106), RGB(106, 77, 47), "Fichiers" },
    };
    const char *dock[7] = { "C", "B", "$", "A", "S", "i", "F" };
    const char *mode_s;
    const char *caption;

    if (!fb || w < 160 || h < 120) return;
    if (!sc || sc->magic != OS_FB_MAGIC) {
        def.magic = OS_FB_MAGIC;
        def.version = 1;
        def.chat_mode = OS_FB_CHAT_CENTER;
        def.pane = OS_FB_PANE_NONE;
        def.stage_mode = OS_FB_STAGE_REFLECTING;
        def.stage_kind = OS_FB_KIND_PLAN;
        def.nmsg = 0;
        def.input[0] = 0;
        def.session[0] = 's';
        def.session[1] = '0';
        def.session[2] = '0';
        def.session[3] = '0';
        def.session[4] = '1';
        def.session[5] = 0;
        sc = &def;
    }

    landscape(fb, w, h);
    bar_h = clampi(h / 22, 28, 40);
    fill_rect_a(fb, w, h, 0, 0, w, bar_h, RGB(12, 28, 40), 235);
    fill_rect(fb, w, h, 12, (bar_h - 12) / 2, 12, 12, RGB(61, 154, 138));
    draw_text(fb, w, h, 30, (bar_h - 8) / 2, "Mohhdy OS", RGB(232, 238, 244));
    draw_text(fb, w, h, 30 + 9 * 8 + 8, (bar_h - 8) / 2, "SE Multiboot dirige par prompts", RGB(154, 168, 181));
    {
        int mx = 30 + 9 * 8 + 8 + 32 * 8;
        if (mx > w / 3) mx = 320;
        for (i = 0; i < 7; i++) {
            draw_text(fb, w, h, mx, (bar_h - 8) / 2, menus[i], RGB(232, 238, 244));
            mx += 8 * 12;
            if (mx > w - 360) break;
        }
    }
    draw_text(fb, w, h, w - 8 * 48, (bar_h - 8) / 2, "llm=stub_echo  us031_complete=false", RGB(243, 230, 176));

    mode_s = "reflecting";
    if (sc->stage_mode == OS_FB_STAGE_ACTING) mode_s = "acting";
    if (sc->stage_mode == OS_FB_STAGE_PRESENTING) mode_s = "presenting";
    round_rect_a(fb, w, h, 12, bar_h + 8, 96, 18, 9, RGB(11, 16, 20), 160);
    stroke_round(fb, w, h, 12, bar_h + 8, 96, 18, 9, RGB(61, 154, 138), 1);
    draw_text(fb, w, h, 22, bar_h + 13, mode_s, RGB(122, 212, 196));
    draw_text(fb, w, h, 118, bar_h + 13, "llm=stub_echo", RGB(243, 230, 176));
    draw_text(fb, w, h, 118 + 15 * 8, bar_h + 13, "bootstrap graphique : pas VGA ASCII", RGB(154, 168, 181));

    for (i = 0; i < 6; i++) {
        icon_tile(fb, w, h, w - 86, bar_h + 16 + i * 72, icons[i].c0, icons[i].c1, icons[i].label);
    }

    draw_text(fb, w, h, 16, h - 88, "Bureau = scene IA (reflexion / action / resultats).", RGB(154, 168, 181));
    draw_text(fb, w, h, 16, h - 76, "Pas un LLM de production. Pas US-031. Un SE Multiboot.", RGB(154, 168, 181));

    scene_w = clampi(w / 2, 360, 640);
    scene_h = 150;
    scene_x = (w - scene_w) / 2 - 20;
    scene_y = h - scene_h - 64;
    round_rect_a(fb, w, h, scene_x, scene_y, scene_w, scene_h, 16, RGB(11, 16, 20), 150);
    stroke_round(fb, w, h, scene_x, scene_y, scene_w, scene_h, 16, RGB(61, 154, 138), 1);
    draw_text(fb, w, h, scene_x + 16, scene_y + 12, "Scene IA", RGB(232, 238, 244));
    caption = "llm=stub_echo . en attente d'un prompt";
    if (sc->stage_mode == OS_FB_STAGE_ACTING) caption = "llm=stub_echo . action stub";
    if (sc->stage_mode == OS_FB_STAGE_PRESENTING) caption = "llm=stub_echo . resultats";
    draw_text(fb, w, h, scene_x + 16, scene_y + 28, caption, RGB(154, 168, 181));
    draw_stage_body(fb, w, h, scene_x + 12, scene_y + 48, scene_w - 24, scene_h - 60, sc);

    pane = sc->pane;
    ptitle = pane_title(pane);
    if (ptitle) {
        draw_window(fb, w, h, 36, bar_h + 28, clampi(w - 420, 420, 720), clampi(h - 220, 280, 520), ptitle,
                    pane == OS_FB_PANE_SHELL ? "prompt=MOHHDY> live_guest=true python_facade=false"
                    : pane == OS_FB_PANE_STATUS ? "chrome=qemu_fb display_surface=vbe_lfb"
                    : "Cerveau = osui_runtime.c. Pas Chromium.");
    }

    chat_w = sc->chat_mode == OS_FB_CHAT_FLOAT ? 360 : clampi(w / 2, 420, 640);
    chat_h = sc->chat_mode == OS_FB_CHAT_FLOAT ? 440 : clampi(h / 2, 320, 440);
    if (sc->chat_mode == OS_FB_CHAT_FLOAT) {
        chat_x = w - chat_w - 110;
        chat_y = h - chat_h - 70;
        chat_x = clampi(chat_x, 8, w - chat_w - 8);
        chat_y = clampi(chat_y, bar_h + 8, h - chat_h - 8);
        if (sc->chat_x || sc->chat_y) {
            chat_x = clampi((int)sc->chat_x * w / 80, 8, w - chat_w - 8);
            chat_y = clampi((int)sc->chat_y * h / 25, bar_h + 8, h - chat_h - 8);
        }
    } else {
        chat_x = (w - chat_w) / 2;
        chat_y = bar_h + (h / 14);
    }
    draw_chat(fb, w, h, chat_x, chat_y, chat_w, chat_h, sc);

    dock_w = 7 * 44 + 16;
    dock_x = (w - dock_w) / 2;
    dock_y = h - 48;
    round_rect_a(fb, w, h, dock_x, dock_y, dock_w, 40, 16, RGB(16, 26, 34), 220);
    for (i = 0; i < 7; i++) {
        round_rect(fb, w, h, dock_x + 8 + i * 44, dock_y + 4, 36, 32, 10, RGB(61, 154, 138));
        draw_text(fb, w, h, dock_x + 8 + i * 44 + 14, dock_y + 14, dock[i], RGB(15, 20, 25));
    }
}
