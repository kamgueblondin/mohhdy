/* gfx_desktop.c - Compositor pixel du bureau Mohhdy (QEMU VBE, pas HTML). */
#include "gfx_desktop.h"
#include "gfx_fb.h"
#include "gfx_font8.h"
#include "input/usb_tablet.h"
#include "keyboard.h"
#include "vga_console.h"
#include "gfx_wm.h"

#ifdef KERNEL_TEST
static void wm_log(const char *s) { (void)s; }
#else
extern void print_string_serial(const char* str);
static void wm_log(const char *s) { print_string_serial(s); }
#endif

#define RGB(r, g, b) ((uint32_t)(r) << 16 | (uint32_t)(g) << 8 | (uint32_t)(b))
static int clampi_k(int v, int lo, int hi) { return v < lo ? lo : v > hi ? hi : v; }

static int g_mouse_x = -1;
static int g_mouse_y = -1;
static uint8_t g_mouse_buttons = 0;
static uint8_t g_prev_buttons = 0;
static os_fb_scene_t g_last_scene;
static int g_last_scene_valid = 0;
/* Window manager state (kernel/gfx_wm.c): one per desktop window. */
static gfx_wm_win_t g_wm_pane, g_wm_chat;
static int g_wm_pane_id = -1;
/* app windows (scene v7): explorer slot 0, editor slot 1 */
static gfx_wm_win_t g_wm_app[OS_FB_APPS];
static uint8_t g_app_kind_prev[OS_FB_APPS];
static uint8_t g_app_focus_prev = 0xFF;
/* z-order, last on top, and the window with the keyboard (-1 desktop) */
static int g_z[GFX_WIN_COUNT] = { GFX_WIN_PANE, GFX_WIN_CHAT, GFX_WIN_APP0, GFX_WIN_APP1 };
static int g_focus = -1;
static int g_hover_dock = -1;
/* press in an app grid: click now, drag on release */
static int g_press_slot = -1, g_press_row, g_press_col;
static int g_last_click_slot = -1, g_last_click_row = -1;
static uint32_t g_last_click_tick;

const char *const gfx_desktop_dock_names[GFX_DESKTOP_DOCK] = {
    "Chat", "Navigateur", "Terminal", "Admin", "Support", "Statut", "Fichiers", "Editeur"
};

void gfx_desktop_wm_reset(void) {
    int i;
    gfx_wm_reset(&g_wm_pane);
    gfx_wm_reset(&g_wm_chat);
    for (i = 0; i < OS_FB_APPS; i++) { gfx_wm_reset(&g_wm_app[i]); g_app_kind_prev[i] = 0; }
    g_wm_pane_id = -1;
    g_focus = -1;
    g_app_focus_prev = 0xFF;
    g_press_slot = -1;
}

#ifdef KERNEL_TEST
static uint32_t g_test_ticks;
static uint32_t desk_ticks(void) { return g_test_ticks += 10; }  /* two calls = a double click */
#else
#include "timer.h"
static uint32_t desk_ticks(void) { return timer_get_ticks(); }
#endif

static void z_raise(int win) {
    int i, k = 0, tmp[GFX_WIN_COUNT];
    for (i = 0; i < GFX_WIN_COUNT; i++) if (g_z[i] != win) tmp[k++] = g_z[i];
    tmp[k] = win;
    for (i = 0; i < GFX_WIN_COUNT; i++) g_z[i] = tmp[i];
}

static void put_uint(unsigned v) {
    char t[12];
    int n = 0;
    do { t[n++] = (char)('0' + v % 10U); v /= 10U; } while (v && n < 11);
    while (n) kbd_put_char(t[--n]);
}

/* Desktop event to the Ring 3 loop: OS_FB_EVT, text, newline. */
static void evt_begin(void) { kbd_put_char((char)OS_FB_EVT); }
static void evt_end(void) { kbd_put_char('\n'); }

static void evt_focus(int win) {
    int was = g_focus;
    g_focus = win;
    if (win == GFX_WIN_APP0 || win == GFX_WIN_APP1) {
        if (was == win) return;
        evt_begin(); kbd_put_string("focus "); put_uint((unsigned)(win - GFX_WIN_APP0)); evt_end();
    } else if (was == GFX_WIN_APP0 || was == GFX_WIN_APP1) {
        evt_begin(); kbd_put_string("focus d"); evt_end();
    }
}

static void evt_app(int slot, const char *what, int n, int a, int b, int c, int d) {
    evt_begin();
    kbd_put_string("app ");
    put_uint((unsigned)slot);
    kbd_put_char(' ');
    kbd_put_string(what);
    if (n > 0) { kbd_put_char(' '); put_uint((unsigned)a); }
    if (n > 1) { kbd_put_char(' '); put_uint((unsigned)b); }
    if (n > 2) { kbd_put_char(' '); put_uint((unsigned)c); }
    if (n > 3) { kbd_put_char(' '); put_uint((unsigned)d); }
    evt_end();
}

/* A desktop command line (typed form): the desktop gets the keyboard. */
static void desk_cmd(const char *line) {
    if (g_focus == GFX_WIN_APP0 || g_focus == GFX_WIN_APP1) evt_focus(-1);
    kbd_put_string(line);
}

static gfx_wm_rect_t to_wm(const gfx_rect_t *r) {
    gfx_wm_rect_t o;
    o.x = r->x; o.y = r->y; o.w = r->w; o.h = r->h;
    return o;
}

static gfx_rect_t from_wm(gfx_wm_rect_t r) {
    gfx_rect_t o;
    o.x = r.x; o.y = r.y; o.w = r.w; o.h = r.h;
    return o;
}

/* minimize, maximize, close button rectangles of a placed window */
static void wm_buttons(const gfx_rect_t *box, int title_h, gfx_rect_t out[3]) {
    int b;
    for (b = 0; b < 3; b++) {
        int x = 0, y = 0;
        if (box->w > 0) gfx_wm_button_pos(to_wm(box), title_h, 1, GFX_WM_HIT_MIN + b, &x, &y);
        out[b].x = x; out[b].y = y;
        out[b].w = box->w > 0 ? GFX_WM_BTN : 0;
        out[b].h = box->w > 0 ? GFX_WM_BTN : 0;
    }
}

/* menu / dock index of a pane: Chat, Browser-OS, Shell, Admin, Support,
 * Statut, FS. */
static int pane_index(int pane) {
    switch (pane) {
    case OS_FB_PANE_NONE: return 0;
    case OS_FB_PANE_BROWSER: return 1;
    case OS_FB_PANE_SHELL: return 2;
    case OS_FB_PANE_ADMIN: return 3;
    case OS_FB_PANE_SUPPORT: return 4;
    case OS_FB_PANE_STATUS: return 5;
    case OS_FB_PANE_FS: return 6;
    default: return -1;
    }
}

static int in_rect(int px, int py, const gfx_rect_t *r) {
    if (!r || r->w <= 0 || r->h <= 0) return 0;
    return px >= r->x && px < r->x + r->w && py >= r->y && py < r->y + r->h;
}

static const char *const k_open_cmds[GFX_DESKTOP_DOCK] = {
    "/center\n", "/browser\n", "/shell\n", "/admin\n", "/support\n", "/status\n", "/files\n", "/edit\n"
};

static void open_target(int i) {
    const os_fb_scene_t *sc = g_last_scene_valid ? &g_last_scene : 0;
    if (i >= 6 && i <= 7 && sc && sc->version >= 7 && sc->app[i - 6].kind) {
        /* open app: restore if minimized, bring to front, keyboard */
        int slot = i - 6;
        if (!gfx_wm_visible(&g_wm_app[slot])) { gfx_wm_restore(&g_wm_app[slot]); wm_log("gfx wm app restore\n"); }
        z_raise(GFX_WIN_APP0 + slot);
        evt_focus(GFX_WIN_APP0 + slot);
        gfx_fb_redraw_desktop();
        return;
    }
    if (i == 0 && !gfx_wm_visible(&g_wm_chat)) {
        gfx_wm_restore(&g_wm_chat);
        z_raise(GFX_WIN_CHAT);
        wm_log("gfx wm chat restore\n");
        gfx_fb_redraw_desktop();
        return;
    }
    if (i > 0 && sc && pane_index(sc->pane) == i && !gfx_wm_visible(&g_wm_pane)) {
        gfx_wm_restore(&g_wm_pane);
        z_raise(GFX_WIN_PANE);
        wm_log("gfx wm pane restore\n");
        gfx_fb_redraw_desktop();
        return;
    }
    if (i == 0) z_raise(GFX_WIN_CHAT);
    else if (i < 6 && i != 2) z_raise(GFX_WIN_PANE);
    desk_cmd(k_open_cmds[i]);
}

/* grid cell of an app window under (x, y); returns 0 outside */
static int app_cell(const gfx_desktop_layout_t *l, int slot, int x, int y, int *row, int *col) {
    const gfx_rect_t *g = &l->app_win[slot].grid;
    if (!in_rect(x, y, g)) return 0;
    *row = (y - g->y) / GFX_APP_ROW_H;
    *col = (x - g->x) / 8;
    if (*row >= OS_FB_APP_ROWS) *row = OS_FB_APP_ROWS - 1;
    if (*col >= OS_FB_APP_COLS - 1) *col = OS_FB_APP_COLS - 2;
    return 1;
}

/* Title drag or text selection released (the press was handled by the click). */
static void wm_release(int x, int y) {
    gfx_desktop_layout_t layout;
    int w = gfx_fb_width(), h = gfx_fb_height(), i, any = 0;
    for (i = 0; i < OS_FB_APPS; i++) any |= g_wm_app[i].drag;
    if (!g_wm_chat.drag && !g_wm_pane.drag && !any && g_press_slot < 0) return;
    gfx_desktop_get_layout(g_last_scene_valid ? &g_last_scene : 0, w, h, &layout);
    if (g_press_slot >= 0) {
        int r, c, slot = g_press_slot;
        g_press_slot = -1;
        if (app_cell(&layout, slot, x, y, &r, &c) && (r != g_press_row || c != g_press_col))
            evt_app(slot, "drag", 4, g_press_row, g_press_col, r, c);
        if (!g_wm_chat.drag && !g_wm_pane.drag && !any) return;
    }
    if (g_wm_chat.drag &&
        gfx_wm_mouse(&g_wm_chat, to_wm(&layout.chat_win.box), to_wm(&layout.chat_win.base),
                     GFX_DESKTOP_CHAT_TITLE_H, 1, x, y, 0, 1) == GFX_WM_ACT_MOVED)
        wm_log("gfx wm chat moved\n");
    if (g_wm_pane.drag &&
        gfx_wm_mouse(&g_wm_pane, to_wm(&layout.pane_win.box), to_wm(&layout.pane_win.base),
                     GFX_DESKTOP_PANE_TITLE_H, 1, x, y, 0, 1) == GFX_WM_ACT_MOVED)
        wm_log("gfx wm pane moved\n");
    for (i = 0; i < OS_FB_APPS; i++)
        if (g_wm_app[i].drag &&
            gfx_wm_mouse(&g_wm_app[i], to_wm(&layout.app_win[i].box), to_wm(&layout.app_win[i].base),
                         GFX_DESKTOP_APP_TITLE_H, 1, x, y, 0, 1) == GFX_WM_ACT_MOVED)
            wm_log("gfx wm app moved\n");
    gfx_fb_redraw_desktop();
}

static int win_box(const gfx_desktop_layout_t *l, int win, gfx_rect_t *box) {
    if (win == GFX_WIN_PANE) { *box = l->pane_win.box; return l->pane_win.active; }
    if (win == GFX_WIN_CHAT) { *box = l->chat_win.box; return l->chat_win.visible; }
    *box = l->app_win[win - GFX_WIN_APP0].box;
    return l->app_win[win - GFX_WIN_APP0].visible;
}

/* Click on an app window (slot): chrome, toolbar, grid. */
static void app_click(const gfx_desktop_layout_t *l, int slot, int act, int x, int y) {
    int r, c;
    if (act == GFX_WM_ACT_CLOSE) { wm_log("gfx wm app close\n"); evt_app(slot, "close", 0, 0, 0, 0, 0); return; }
    if (act == GFX_WM_ACT_MINIMIZE || act == GFX_WM_ACT_MAXIMIZE) {
        wm_log(act == GFX_WM_ACT_MINIMIZE ? "gfx wm app minimize\n"
                    : g_wm_app[slot].maximized ? "gfx wm app maximize on\n" : "gfx wm app maximize off\n");
        if (act == GFX_WM_ACT_MINIMIZE) evt_focus(-1);
        gfx_fb_redraw_desktop();
        return;
    }
    if (act != GFX_WM_ACT_BODY) { gfx_fb_redraw_desktop(); return; }
    if (in_rect(x, y, &l->app_win[slot].tool)) {
        evt_app(slot, "tool", 1, (x - l->app_win[slot].tool.x) / 8, 0, 0, 0);
        return;
    }
    if (app_cell(l, slot, x, y, &r, &c)) {
        uint32_t now = desk_ticks();
        int dbl = g_last_click_slot == slot && g_last_click_row == r && now - g_last_click_tick <= 50U; /* 0.5 s at 100 Hz */
        evt_app(slot, dbl ? "dclick" : "click", 2, r, c, 0, 0);
        g_last_click_slot = dbl ? -1 : slot;
        g_last_click_row = r;
        g_last_click_tick = now;
        g_press_slot = slot; g_press_row = r; g_press_col = c;
        return;
    }
    gfx_fb_redraw_desktop();
}

static void gfx_desktop_handle_click(int x, int y) {
    gfx_desktop_layout_t layout;
    int w = gfx_fb_width();
    int h = gfx_fb_height();
    int i, k;

    gfx_desktop_get_layout(g_last_scene_valid ? &g_last_scene : 0, w, h, &layout);

    /* 0. Windows, topmost first (z-order): the clicked window comes to the
     * front and takes the keyboard; gfx_wm.c decides close / minimize /
     * maximize / drag. */
    for (k = GFX_WIN_COUNT - 1; k >= 0; k--) {
        int win = layout.z[k], act;
        gfx_rect_t box;
        if (!win_box(&layout, win, &box) || !in_rect(x, y, &box)) continue;
        if (layout.z[GFX_WIN_COUNT - 1] != win) z_raise(win);
        if (win == GFX_WIN_CHAT) {
            act = gfx_wm_mouse(&g_wm_chat, to_wm(&layout.chat_win.box), to_wm(&layout.chat_win.base),
                               GFX_DESKTOP_CHAT_TITLE_H, 1, x, y, 1, 0);
            evt_focus(-1);
            if (act == GFX_WM_ACT_CLOSE || act == GFX_WM_ACT_MINIMIZE || act == GFX_WM_ACT_MAXIMIZE) {
                wm_log(act == GFX_WM_ACT_CLOSE ? "gfx wm chat close\n"
                                    : act == GFX_WM_ACT_MINIMIZE ? "gfx wm chat minimize\n"
                                    : g_wm_chat.maximized ? "gfx wm chat maximize on\n" : "gfx wm chat maximize off\n");
                gfx_fb_redraw_desktop();
                return;
            }
            if (act == GFX_WM_ACT_BODY && in_rect(x, y, &layout.chat_win.send_btn)) { kbd_put_string("\n"); return; }
            gfx_fb_redraw_desktop();
            return;
        }
        if (win == GFX_WIN_PANE) {
            act = gfx_wm_mouse(&g_wm_pane, to_wm(&layout.pane_win.box), to_wm(&layout.pane_win.base),
                               GFX_DESKTOP_PANE_TITLE_H, 1, x, y, 1, 0);
            evt_focus(-1);
            if (act == GFX_WM_ACT_CLOSE) {
                wm_log("gfx wm pane close\n");
                gfx_wm_reset(&g_wm_pane);
                kbd_put_string("/center\n");
                return;
            }
            if (act == GFX_WM_ACT_MINIMIZE || act == GFX_WM_ACT_MAXIMIZE) {
                wm_log(act == GFX_WM_ACT_MINIMIZE ? "gfx wm pane minimize\n"
                                    : g_wm_pane.maximized ? "gfx wm pane maximize on\n" : "gfx wm pane maximize off\n");
            }
            gfx_fb_redraw_desktop();
            return;
        }
        {
            int slot = win - GFX_WIN_APP0;
            act = gfx_wm_mouse(&g_wm_app[slot], to_wm(&layout.app_win[slot].box), to_wm(&layout.app_win[slot].base),
                               GFX_DESKTOP_APP_TITLE_H, 1, x, y, 1, 0);
            if (act != GFX_WM_ACT_CLOSE) evt_focus(win);
            app_click(&layout, slot, act, x, y);
            return;
        }
    }

    if (g_last_scene_valid && (g_last_scene.pane == OS_FB_PANE_IDE || g_last_scene.pane == OS_FB_PANE_WEB)
        && in_rect(x, y, &layout.pane_win.traffic)) {
        desk_cmd("/center\n");
        return;
    }

    /* 3. Dock icons, 4. top menu bar: a minimized or closed window comes
     * back without a command; otherwise the command opens it. */
    for (i = 0; i < GFX_DESKTOP_DOCK; i++) {
        if (in_rect(x, y, &layout.dock.icons[i]) || (i < layout.menu_count && in_rect(x, y, &layout.menu_items[i]))) {
            open_target(i);
            return;
        }
    }

    /* 4b. Desktop icons on the right (Browser-OS ... Fichiers) */
    for (i = 0; i < layout.desk.count; i++) {
        if (in_rect(x, y, &layout.desk.icons[i])) {
            open_target(i + 1);
            return;
        }
    }

    /* 5. Scene IA Mode Pills */
    if (in_rect(x, y, &layout.scene_ia.pills[0])) {
        desk_cmd("/stage-prompt reflexion\n");
        return;
    }
    if (in_rect(x, y, &layout.scene_ia.pills[1])) {
        desk_cmd("/stage-prompt simule action\n");
        return;
    }
    if (in_rect(x, y, &layout.scene_ia.pills[2])) {
        desk_cmd("/stage-prompt resultats\n");
        return;
    }
    if (in_rect(x, y, &layout.scene_ia.box)) {
        desk_cmd("/plan\n");
        return;
    }

    /* 6. Stage mode badge */
    if (in_rect(x, y, &layout.stage_badge)) {
        desk_cmd("/stage\n");
        return;
    }
    /* empty desktop: the keyboard goes back to the desktop input */
    if (g_focus >= 0) evt_focus(-1);
}

/* Dock icon under the pointer: a tooltip with its name (redraw on change). */
static void hover_update(int x, int y) {
    int w = gfx_fb_width(), h = gfx_fb_height(), i, idx = -1;
    int bar_h = clampi_k(h / 22, 28, 40);
    int dock_w = GFX_DESKTOP_DOCK * 44 + 16, dock_x, dock_y;
    if (!vga_desktop_active()) return;
    if (dock_w > w - 16) dock_w = w - 16;
    dock_x = (w - dock_w) / 2;
    dock_y = h - 48;
    if (dock_y < bar_h + 8) dock_y = h - 36;
    for (i = 0; i < GFX_DESKTOP_DOCK; i++)
        if (x >= dock_x + 8 + i * 44 && x < dock_x + 8 + i * 44 + 36 && y >= dock_y + 4 && y < dock_y + 36) idx = i;
    if (idx != g_hover_dock) {
        g_hover_dock = idx;
        gfx_fb_redraw_desktop();
    }
}

void gfx_desktop_set_mouse(int x, int y, uint8_t buttons) {
    uint8_t left_now = (buttons & 1);
    uint8_t left_prev = (g_prev_buttons & 1);

    g_mouse_x = x;
    g_mouse_y = y;
    g_mouse_buttons = buttons;

    if (vga_desktop_active() && !left_prev && left_now) {
        gfx_desktop_handle_click(g_mouse_x, g_mouse_y);
    }
    if (vga_desktop_active() && left_prev && !left_now) wm_release(g_mouse_x, g_mouse_y);
    if (gfx_fb_term_active()) gfx_fb_term_mouse(g_mouse_x, g_mouse_y, buttons, g_prev_buttons);
    if (!left_now) hover_update(g_mouse_x, g_mouse_y);
    g_prev_buttons = buttons;

    gfx_fb_update_cursor();
}

void gfx_desktop_get_mouse(int *x, int *y, uint8_t *buttons) {
    if (x) *x = g_mouse_x;
    if (y) *y = g_mouse_y;
    if (buttons) *buttons = g_mouse_buttons;
}

void gfx_desktop_move_mouse(int dx, int dy, uint8_t buttons) {
    if (!usb_tablet_present()) {
        dx *= 2;
        dy *= 2;
    }
    if (g_mouse_x < 0) g_mouse_x = GFX_FB_WIDTH / 2;
    if (g_mouse_y < 0) g_mouse_y = GFX_FB_HEIGHT / 2;

    uint8_t left_now = (buttons & 1);
    uint8_t left_prev = (g_prev_buttons & 1);

    g_mouse_x += dx;
    g_mouse_y += dy;
    g_mouse_buttons = buttons;

    if (vga_desktop_active() && !left_prev && left_now) {
        gfx_desktop_handle_click(g_mouse_x, g_mouse_y);
    }
    if (vga_desktop_active() && left_prev && !left_now) wm_release(g_mouse_x, g_mouse_y);
    if (gfx_fb_term_active()) gfx_fb_term_mouse(g_mouse_x, g_mouse_y, buttons, g_prev_buttons);
    if (!left_now) hover_update(g_mouse_x, g_mouse_y);
    g_prev_buttons = buttons;

    gfx_fb_update_cursor();
}

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

void gfx_desktop_draw_cursor(uint32_t *fb, int w, int h, int pitch, int mx, int my, uint8_t buttons) {
    int cy, cx;
    uint32_t fill_color = (buttons & 1) ? RGB(122, 212, 196) : RGB(245, 248, 250);
    uint32_t border_color = RGB(10, 16, 20);
    if (!fb) return;
    if (pitch <= 0) pitch = w;

    static const char *shape[18] = {
        "#           ",
        "##          ",
        "#.#         ",
        "#..#        ",
        "#...#       ",
        "#....#      ",
        "#.....#     ",
        "#......#    ",
        "#.......#   ",
        "#........#  ",
        "#.....####  ",
        "#..##..#    ",
        "#.#  #..#   ",
        "##   #..#   ",
        "#     #..#  ",
        "      #..#  ",
        "       ##   ",
        "            "
    };

    for (cy = 0; cy < 18; cy++) {
        for (cx = 0; cx < 12; cx++) {
            char p = shape[cy][cx];
            int px = mx + cx;
            int py = my + cy;
            if (px >= 0 && px < w && py >= 0 && py < h) {
                if (p == '#') {
                    fb[py * pitch + px] = border_color;
                } else if (p == '.') {
                    fb[py * pitch + px] = fill_color;
                }
            }
        }
    }
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

static void draw_char_bits(uint32_t *fb, int w, int h, int x, int y, char ch, uint32_t c) {
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

static void draw_char(uint32_t *fb, int w, int h, int x, int y, char ch, uint32_t c) {
    draw_char_bits(fb, w, h, x + 1, y + 1, ch, RGB(8, 12, 16));
    draw_char_bits(fb, w, h, x, y, ch, c);
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
    if (kind == OS_FB_KIND_MEDIA) {
        int paused = sc && (sc->reserved & 0x40);
        uint8_t ticks = sc ? (sc->reserved & 0x3F) : 0;
        int i;
        int bar_w = (rw - 48) / 10;
        if (bar_w < 4) bar_w = 4;
        pill(fb, w, h, x + 8, y + 8, 72, 28, paused ? RGB(201, 162, 39) : RGB(61, 154, 138), paused ? "PAUSED" : "PLAYING", 0);
        draw_text(fb, w, h, x + 88, y + 16, "Media Stream Active", RGB(232, 238, 244));
        for (i = 0; i < 10; i++) {
            int h_bar = paused ? 12 : (6 + ((ticks + i * 5) % 22));
            fill_rect(fb, w, h, x + 12 + i * (bar_w + 3), y + 68 - h_bar, bar_w, h_bar, RGB(122, 212, 196));
        }
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
    case OS_FB_PANE_P2P: return "Reseau P2P";
    case OS_FB_PANE_COLLAB: return "Economie Collab";
    case OS_FB_PANE_PROD: return "Production Prod";
    default: return 0;
    }
}

/* llm label of the last chat turn: the scene carries it from version 5;
 * older scenes keep the historic stub_echo label. */
static const char *scene_llm(const os_fb_scene_t *sc) {
    if (sc && sc->magic == OS_FB_MAGIC && sc->version >= 5 && sc->llm[0]) return sc->llm;
    return "stub_echo";
}

/* User-facing name of the chat engine (the raw id stays in the serial log). */
static int str_eq(const char *a, const char *b) {
    while (*a && *a == *b) { a++; b++; }
    return *a == *b;
}
const char *gfx_desktop_llm_name(const char *llm) {
    if (!llm || !llm[0] || str_eq(llm, "stub_echo")) return "echo (sans modele)";
    if (str_eq(llm, "gpt2_local")) return "GPT-2 local";
    if (str_eq(llm, "gpt2_gguf_local")) return "GPT-2 GGUF local";
    return llm;
}

static void llm_text(char *d, int max, const char *pre, const os_fb_scene_t *sc, const char *post) {
    const char *parts[3];
    int n = 0, k, i;
    const char *name = gfx_desktop_llm_name(scene_llm(sc));
    char clean[OS_FB_LLM_LEN];
    for (i = 0; i < OS_FB_LLM_LEN - 1 && name[i]; i++)
        clean[i] = (name[i] >= 32 && name[i] <= 126) ? name[i] : '?';
    clean[i] = 0;
    parts[0] = pre ? pre : "";
    parts[1] = clean;
    parts[2] = post ? post : "";
    if (!d || max <= 0) return;
    for (k = 0; k < 3; k++)
        for (i = 0; parts[k][i] && n < max - 1; i++) d[n++] = parts[k][i];
    d[n] = 0;
}

/* Long input: show its end (where the user types), not its start. */
static const char *input_tail(const char *s, int maxc) {
    int n = 0;
    if (!s) return "";
    while (s[n]) n++;
    if (maxc > 0 && n > maxc) return s + (n - maxc);
    return s;
}

static void draw_wm_buttons(uint32_t *fb, int w, int h, int x, int y, int rw, int rh, int title_h) {
    gfx_wm_rect_t r;
    r.x = x; r.y = y; r.w = rw; r.h = rh;
    gfx_wm_draw_buttons(fb, w, h, w, r, title_h, 1);
}

static void draw_window(uint32_t *fb, int w, int h, int x, int y, int rw, int rh, const char *title, const char *body, const os_fb_scene_t *sc) {
    char lab[64];
    round_rect_a(fb, w, h, x, y, rw, rh, 10, RGB(18, 28, 36), 255);  /* opaque: no bleed from windows behind */
    stroke_round(fb, w, h, x, y, rw, rh, 10, RGB(61, 154, 138), 1);
    fill_rect(fb, w, h, x, y, rw, 32, RGB(28, 44, 56));
    draw_wm_buttons(fb, w, h, x, y, rw, rh, GFX_DESKTOP_PANE_TITLE_H);
    draw_text_n(fb, w, h, x + 14, y + 12, title ? title : "", (rw - 80) / 8, RGB(232, 238, 244));
    fill_rect(fb, w, h, x + 12, y + 44, rw - 24, 40, RGB(42, 36, 16));
    llm_text(lab, (int)sizeof(lab), "IA : ", sc, "");
    draw_text(fb, w, h, x + 18, y + 48, lab, RGB(243, 230, 176));
    if (sc && sc->version >= 6 && sc->pane_live && sc->nterm > 0) {
        /* live window content from guest syscalls (userspace/osui_panes.c) */
        int i, ty = y + 96, n = sc->nterm > OS_FB_TERM_MAX ? OS_FB_TERM_MAX : sc->nterm;
        for (i = 0; i < n && ty + 12 <= y + rh - 8; i++, ty += 14)
            draw_text_n(fb, w, h, x + 18, ty, sc->term[i], (rw - 36) / 8,
                        i == 0 ? RGB(122, 212, 196) : RGB(201, 232, 223));
    } else if (body) draw_text_n(fb, w, h, x + 18, y + 96, body, (rw - 36) / 8, RGB(201, 232, 223));
}

/* US-050: full-screen pm-ide. term[0] title, term[1..n-2] text rows with
 * a line-number gutter ('>' marks the cursor line), term[n-1] status.
 * The frame text comes from the Ring 3 editor (userspace/osui_ide.c). */
/* US-050: pm-ide syntax colors, one per class of os_fb_scene_t.term_attr. */
static uint32_t ide_class_rgb(uint8_t cls) {
    switch (cls) {
    case 1: return RGB(122, 180, 255);  /* keyword */
    case 2: return RGB(243, 190, 120);  /* string */
    case 3: return RGB(196, 160, 255);  /* number */
    case 4: return RGB(112, 132, 142);  /* comment */
    case 5: return RGB(96, 118, 128);   /* gutter */
    case 6: return RGB(232, 238, 244);  /* program output */
    case 7: return RGB(61, 154, 138);   /* output rule */
    default: return RGB(201, 232, 223);
    }
}

static void draw_ide_colored(uint32_t *fb, int w, int h, int x, int y, const char *text,
                             const uint8_t *attr, int maxc) {
    int k = 0;
    while (k < maxc && k < OS_FB_TERM_LEN && text[k]) {
        int e = k;
        while (e < maxc && e < OS_FB_TERM_LEN && text[e] && attr[e] == attr[k]) e++;
        draw_text_n(fb, w, h, x + k * 8, y, text + k, e - k, ide_class_rgb(attr[k]));
        k = e;
    }
}

static void draw_ide(uint32_t *fb, int w, int h, int x, int y, int rw, int rh, const os_fb_scene_t *sc) {
    int n = sc ? (int)sc->nterm : 0, i, ty, maxc, row_h = 20;
    if (n > OS_FB_TERM_MAX) n = OS_FB_TERM_MAX;
    round_rect_a(fb, w, h, x, y, rw, rh, 10, RGB(10, 14, 18), 255);
    stroke_round(fb, w, h, x, y, rw, rh, 10, RGB(61, 154, 138), 1);
    fill_rect(fb, w, h, x, y, rw, 32, RGB(28, 44, 56));
    traffic(fb, w, h, x + 8, y + 8);
    maxc = (rw - 40) / 8;
    if (maxc < 8) maxc = 8;
    if (n > 0) draw_text_n(fb, w, h, x + 56, y + 12, sc->term[0], maxc - 6, RGB(232, 238, 244));
    ty = y + 48;
    for (i = 1; i < n - 1; i++) {
        if (ty + row_h > y + rh - 40) break;
        if (sc->version >= 3 && (sc->term_attr[i][0] == 7 || sc->term_attr[i][5] == 6 || sc->term_attr[i][4] == 6))
            fill_rect(fb, w, h, x + 10, ty - 4, rw - 20, row_h, RGB(16, 22, 30));
        else if (sc->term[i][3] == '>') fill_rect(fb, w, h, x + 10, ty - 4, rw - 20, row_h, RGB(24, 40, 48));
        if (sc->version >= 3) draw_ide_colored(fb, w, h, x + 18, ty, sc->term[i], sc->term_attr[i], maxc);
        else draw_text_n(fb, w, h, x + 18, ty, sc->term[i], maxc, RGB(201, 232, 223));
        ty += row_h;
    }
    if (n > 1) {
        fill_rect(fb, w, h, x + 10, y + rh - 32, rw - 20, 24, RGB(42, 36, 16));
        draw_text_n(fb, w, h, x + 18, y + rh - 26, sc->term[n - 1], maxc, RGB(243, 230, 176));
    }
}

/* Lot 6 (US-031): page painted from the display list of the guest web
 * engine (userspace/webeng.c). The page origin is x + 8, y + 40; items are
 * clipped to the page area. Text uses the 8x8 font without shadow. */
static void web_fill_clip(uint32_t *fb, int w, int h, int x, int y, int rw, int rh, uint32_t c,
                          int cx0, int cy0, int cx1, int cy1) {
    int x1 = x + rw, y1 = y + rh;
    if (x < cx0) x = cx0;
    if (y < cy0) y = cy0;
    if (x1 > cx1) x1 = cx1;
    if (y1 > cy1) y1 = cy1;
    if (x1 > x && y1 > y) fill_rect(fb, w, h, x, y, x1 - x, y1 - y, c);
}

void gfx_desktop_web_origin(int w, int h, int *ox, int *oy, int *pw, int *ph) {
    int bar_h = clampi(h / 22, 28, 40);
    if (ox) *ox = 16;
    if (oy) *oy = bar_h + 48;
    if (pw) *pw = w - 32;
    if (ph) *ph = h - (bar_h + 48) - 16;
}

static void draw_web(uint32_t *fb, int w, int h, const os_fb_scene_t *sc) {
    int ox, oy, pw, ph, i, n, cx1, cy1;
    int bar_h = clampi(h / 22, 28, 40);
    gfx_desktop_web_origin(w, h, &ox, &oy, &pw, &ph);
    round_rect_a(fb, w, h, 8, bar_h + 8, w - 16, h - bar_h - 16, 10, RGB(10, 14, 18), 250);
    fill_rect(fb, w, h, 8, bar_h + 8, w - 16, 32, RGB(28, 44, 56));
    traffic(fb, w, h, 16, bar_h + 16);
    draw_text_n(fb, w, h, 64, bar_h + 20, sc->web_title, (w - 96) / 8, RGB(232, 238, 244));
    fill_rect(fb, w, h, ox, oy, pw, ph, RGB(255, 255, 255));
    cx1 = ox + pw;
    cy1 = oy + ph;
    n = sc->nweb > OS_FB_WEB_ITEMS ? OS_FB_WEB_ITEMS : (int)sc->nweb;
    for (i = 0; i < n; i++) {
        const os_fb_web_item_t *it = &sc->web[i];
        int x = ox + it->x, y = oy + it->y - (int)sc->web_scroll;
        uint32_t c = it->color & 0xFFFFFFu;
        if (it->kind == OS_FB_WEB_RECT) {
            web_fill_clip(fb, w, h, x, y, it->w, it->h, c, ox, oy, cx1, cy1);
        } else if (it->kind == OS_FB_WEB_BORDER) {
            int t = it->bold ? it->bold : 1;
            web_fill_clip(fb, w, h, x, y, it->w, t, c, ox, oy, cx1, cy1);
            web_fill_clip(fb, w, h, x, y + it->h - t, it->w, t, c, ox, oy, cx1, cy1);
            web_fill_clip(fb, w, h, x, y, t, it->h, c, ox, oy, cx1, cy1);
            web_fill_clip(fb, w, h, x + it->w - t, y, t, it->h, c, ox, oy, cx1, cy1);
        } else if (it->kind == OS_FB_WEB_TEXT) {
            int k;
            if (y < oy || y + 8 > cy1) continue;
            for (k = 0; k < OS_FB_WEB_TEXT_LEN && it->text[k]; k++) {
                int gx = x + k * 8;
                if (gx < ox || gx + 9 > cx1) continue;
                draw_char_bits(fb, w, h, gx, y, it->text[k], c);
                if (it->bold) draw_char_bits(fb, w, h, gx + 1, y, it->text[k], c);
            }
        }
    }
}

static void draw_shell(uint32_t *fb, int w, int h, int x, int y, int rw, int rh, const os_fb_scene_t *sc) {
    int ty, term_h, i, maxc, prompt_y, n;
    round_rect_a(fb, w, h, x, y, rw, rh, 10, RGB(10, 16, 20), 255);
    stroke_round(fb, w, h, x, y, rw, rh, 10, RGB(61, 154, 138), 1);
    fill_rect(fb, w, h, x, y, rw, 32, RGB(28, 44, 56));
    draw_wm_buttons(fb, w, h, x, y, rw, rh, GFX_DESKTOP_PANE_TITLE_H);
    draw_text(fb, w, h, x + 14, y + 12, "Shell Multiboot", RGB(232, 238, 244));
    if (rw > 8 * 34 + 80) draw_text(fb, w, h, x + rw - 8 * 18 - 70, y + 12, "prompt=MOHHDY>", RGB(122, 212, 196));
    term_h = rh - 44;
    if (term_h < 80) term_h = 80;
    fill_rect(fb, w, h, x + 10, y + 36, rw - 20, term_h, RGB(8, 12, 16));
    maxc = (rw - 40) / 8;
    if (maxc < 8) maxc = 8;
    prompt_y = y + rh - 22;
    if (prompt_y < y + 48) prompt_y = y + 48;
    ty = y + 44;
    n = sc ? (int)sc->nterm : 0;
    if (n > OS_FB_TERM_MAX) n = OS_FB_TERM_MAX;
    if (n <= 0) {
        draw_text(fb, w, h, x + 18, ty, "Shell Multiboot live_eval=true", RGB(122, 212, 196));
        ty += 14;
        draw_text(fb, w, h, x + 18, ty, "Pas un bash Linux. help ls date whoami ai", RGB(154, 168, 181));
        ty += 14;
    } else {
        for (i = 0; i < n; i++) {
            if (ty + 14 > prompt_y) break;
            draw_text_n(fb, w, h, x + 18, ty, sc->term[i], maxc, RGB(201, 232, 223));
            ty += 12;
        }
    }
    draw_text(fb, w, h, x + 18, prompt_y, "MOHHDY> ", RGB(201, 232, 223));
    if (sc && sc->input[0]) {
        draw_text_n(fb, w, h, x + 18 + 8 * 8, prompt_y, input_tail(sc->input, maxc - 8), maxc - 8, RGB(232, 238, 244));
    } else {
        fill_rect(fb, w, h, x + 18 + 8 * 8, prompt_y, 8, 10, RGB(61, 154, 138));
    }
}

static void draw_chat(uint32_t *fb, int w, int h, int x, int y, int rw, int rh, const os_fb_scene_t *sc) {
    int log_y, i, n;
    const char *msg;
    char lab[64];
    int stub = scene_llm(sc)[0] == 's' && scene_llm(sc)[1] == 't';
    round_rect_a(fb, w, h, x, y, rw, rh, 14, RGB(12, 18, 24), 255);
    stroke_round(fb, w, h, x, y, rw, rh, 14, RGB(61, 154, 138), 2);
    fill_rect(fb, w, h, x + 1, y + 1, rw - 2, 36, RGB(28, 44, 56));
    draw_wm_buttons(fb, w, h, x, y, rw, rh, GFX_DESKTOP_CHAT_TITLE_H);
    draw_text(fb, w, h, x + 16, y + 14, "Mohhdy", RGB(232, 238, 244));
    llm_text(lab, (int)sizeof(lab), "IA : ", sc, "");
    {
        int ln = 0;
        while (lab[ln]) ln++;
        if (ln < 14) ln = 14;
        if (x + 16 + 8 * 8 < x + rw - 8 * ln - 66)
            draw_text(fb, w, h, x + rw - 8 * ln - 66, y + 14, lab, RGB(243, 230, 176));
    }
    fill_rect_a(fb, w, h, x + 14, y + 46, rw - 28, 44, RGB(42, 36, 16), 230);
    draw_text_n(fb, w, h, x + 20, y + 50, "Posez une question ou tapez /help.", (rw - 40) / 8, RGB(243, 230, 176));
    llm_text(lab, (int)sizeof(lab), "Assistant local : ", sc, "");
    draw_text_n(fb, w, h, x + 20, y + 62, lab, (rw - 40) / 8, RGB(243, 230, 176));
    log_y = y + 98;
    fill_rect(fb, w, h, x + 14, log_y, rw - 28, rh - 180, RGB(11, 16, 20));
    n = sc ? (int)sc->nmsg : 0;
    if (n > OS_FB_MSG_MAX) n = OS_FB_MSG_MAX;
    if (n <= 0) {
        llm_text(lab, (int)sizeof(lab), "IA : ", sc, "");
        draw_text_n(fb, w, h, x + 20, log_y + 8, lab, (rw - 40) / 8, RGB(154, 168, 181));
        draw_text_n(fb, w, h, x + 20, log_y + 22, "Mohhdy OS - bureau pilote par prompts.", (rw - 40) / 8, RGB(201, 232, 223));
        draw_text_n(fb, w, h, x + 20, log_y + 34, "Fichiers et Editeur : icones du dock.", (rw - 40) / 8, RGB(201, 232, 223));
    } else {
        /* messages wrap on word boundaries; the newest stay visible */
        int maxc = (rw - 48) / 8, lines = 0, avail = (rh - 190) / 11, first = 0, yy, need;
        (void)stub;
        if (maxc < 8) maxc = 8;
        for (i = n - 1; i >= 0; i--) {
            int len = 0;
            while (sc->messages[i][len] && len < OS_FB_MSG_LEN) len++;
            need = (len + maxc - 1) / maxc + 1;  /* +1: wrap slack and gap */
            if (lines + need > avail && i < n - 1) { first = i + 1; break; }
            lines += need;
        }
        yy = log_y + 8;
        for (i = first; i < n; i++) {
            int pos = 0, len = 0;
            msg = sc->messages[i];
            while (msg[len] && len < OS_FB_MSG_LEN) len++;
            fill_rect(fb, w, h, x + 16, yy, 2, 9, RGB(61, 154, 138));
            while (pos < len && yy < log_y + rh - 190) {
                int take = len - pos, k;
                if (take > maxc) {
                    take = maxc;
                    for (k = maxc; k > maxc / 2; k--) if (msg[pos + k] == ' ') { take = k; break; }
                }
                draw_text_n(fb, w, h, x + 20, yy, msg + pos, take, RGB(232, 238, 244));
                pos += take;
                while (pos < len && msg[pos] == ' ') pos++;
                yy += 11;
            }
            yy += 4;
        }
    }
    fill_rect(fb, w, h, x + 14, y + rh - 70, rw - 28, 28, RGB(16, 25, 32));
    if (sc && sc->input[0]) draw_text_n(fb, w, h, x + 20, y + rh - 62, input_tail(sc->input, (rw - 48) / 8), (rw - 48) / 8, RGB(232, 238, 244));
    else draw_text_n(fb, w, h, x + 20, y + rh - 62, "Prompt ou /help, /browser, /shell ...", (rw - 40) / 8, RGB(154, 168, 181));
    round_rect(fb, w, h, x + 14, y + rh - 34, 88, 22, 8, RGB(61, 154, 138));
    draw_text(fb, w, h, x + 26, y + rh - 28, "Envoyer", RGB(15, 20, 25));
    draw_text(fb, w, h, x + 110, y + rh - 28, sc && sc->session[0] ? sc->session : "s0001", RGB(154, 168, 181));
    draw_text_n(fb, w, h, x + 170, y + rh - 28, gfx_desktop_llm_name(scene_llm(sc)), (rw - 190) / 8, RGB(154, 168, 181));
}

void gfx_desktop_get_layout(const os_fb_scene_t *scene, int w, int h, gfx_desktop_layout_t *out) {
    static os_fb_scene_t def; /* Lot 6: the scene carries the web display list, keep it off the 16 KB kernel stack */
    const os_fb_scene_t *sc = scene;
    int bar_h, chat_w, chat_h, chat_x, chat_y, scene_w, scene_h, scene_x, scene_y;
    int dock_w, dock_x, dock_y, bw, i, mx, ty, gap, flag_w;
    uint8_t pane;
    const char *menus[GFX_DESKTOP_MENUS] = { "Chat", "Browser-OS", "Shell", "Admin", "Support", "Statut", "Fichiers", "Editeur" };

    if (!out) return;

    if (w < 160) w = 160;
    if (h < 120) h = 120;

    if (!sc || sc->magic != OS_FB_MAGIC) {
        def.magic = OS_FB_MAGIC;
        def.version = 1;
        def.chat_mode = OS_FB_CHAT_CENTER;
        def.pane = OS_FB_PANE_NONE;
        def.stage_mode = OS_FB_STAGE_REFLECTING;
        def.stage_kind = OS_FB_KIND_PLAN;
        def.nmsg = 0;
        def.input[0] = 0;
        sc = &def;
    }

    bar_h = clampi(h / 22, 28, 40);
    out->bar_h = bar_h;

    mx = 122;
    ty = (bar_h - 8) / 2;
    gap = w < 720 ? 8 : 14;  /* 8 menus still fit at 640 px */
    flag_w = (w >= 920) ? (8 * 36 + 12) : ((w >= 720) ? (8 * 16 + 12) : 8);

    for (i = 0; i < GFX_DESKTOP_MENUS; i++) {
        int len = 0;
        while (menus[i][len]) len++;
        if (mx + len * 8 > w - flag_w - 8) break;
        out->menu_items[i].x = mx;
        out->menu_items[i].y = ty;
        out->menu_items[i].w = len * 8;
        out->menu_items[i].h = 8;
        mx += len * 8 + gap;
    }
    out->menu_count = i;

    out->stage_badge.x = 12;
    out->stage_badge.y = bar_h + 8;
    out->stage_badge.w = 96;
    out->stage_badge.h = 18;

    scene_w = clampi(w / 2, 200, w - 40);
    if (scene_w > 640) scene_w = 640;
    scene_h = clampi(h / 5, 90, 150);
    scene_x = (w - scene_w) / 2 - (w >= 780 ? 20 : 0);
    if (scene_x < 8) scene_x = 8;
    scene_y = h - scene_h - 64;
    if (scene_y < bar_h + 40) scene_y = bar_h + 40;

    out->scene_ia.box.x = scene_x;
    out->scene_ia.box.y = scene_y;
    out->scene_ia.box.w = scene_w;
    out->scene_ia.box.h = scene_h;

    bw = (scene_w - 24 - 40) / 3;
    out->scene_ia.pills[0].x = scene_x + 12 + 8;
    out->scene_ia.pills[0].y = scene_y + 48 + 8;
    out->scene_ia.pills[0].w = bw;
    out->scene_ia.pills[0].h = 40;

    out->scene_ia.pills[1].x = scene_x + 12 + 16 + bw;
    out->scene_ia.pills[1].y = scene_y + 48 + 8;
    out->scene_ia.pills[1].w = bw;
    out->scene_ia.pills[1].h = 40;

    out->scene_ia.pills[2].x = scene_x + 12 + 24 + 2 * bw;
    out->scene_ia.pills[2].y = scene_y + 48 + 8;
    out->scene_ia.pills[2].w = bw;
    out->scene_ia.pills[2].h = 40;

    pane = sc->pane;
    {
        gfx_wm_rect_t area;
        area.x = 0; area.y = bar_h + 4; area.w = w; area.h = h - bar_h - 4 - 52;
        if (area.h < 120) area.h = h - bar_h - 4;
        if (pane != g_wm_pane_id) {
            /* another pane: default place and state */
            gfx_wm_reset(&g_wm_pane);
            g_wm_pane_id = pane;
        }
        out->pane_win.active = 0;
        out->pane_win.minimized = 0;
        out->pane_win.maximized = 0;
        out->pane_win.box.x = out->pane_win.box.y = out->pane_win.box.w = out->pane_win.box.h = 0;
        out->pane_win.base = out->pane_win.box;
        if (pane != OS_FB_PANE_NONE && pane != OS_FB_PANE_IDE && pane != OS_FB_PANE_WEB) {
            gfx_wm_rect_t base, placed;
            base.x = 36; base.y = bar_h + 28;
            base.w = clampi(w - (w >= 780 ? 420 : 48), 240, w - 48);
            base.h = clampi(h - 220, 160, h - bar_h - 80);
            out->pane_win.base = from_wm(base);
            gfx_wm_place(&g_wm_pane, base, area, &placed);
            out->pane_win.box = from_wm(placed);
            out->pane_win.active = placed.w > 0;
            out->pane_win.minimized = !gfx_wm_visible(&g_wm_pane);
            out->pane_win.maximized = g_wm_pane.maximized && out->pane_win.active;
        }
        wm_buttons(&out->pane_win.box, GFX_DESKTOP_PANE_TITLE_H, out->pane_win.btn);
        out->pane_win.traffic = out->pane_win.btn[2];
        if (pane == OS_FB_PANE_IDE || pane == OS_FB_PANE_WEB) {
            /* full screen panes keep their traffic light close (and ESC) */
            out->pane_win.traffic.x = 36 + 8; out->pane_win.traffic.y = bar_h + 28 + 8;
            out->pane_win.traffic.w = 42; out->pane_win.traffic.h = 16;
        }
        if (out->pane_win.active || pane == OS_FB_PANE_IDE || pane == OS_FB_PANE_WEB) {
            /* A pane window covers the Scene IA: it is not drawn and its
             * pills are not clickable (they used to show through the pane). */
            out->scene_ia.box.w = out->scene_ia.box.h = 0;
            for (i = 0; i < 3; i++) out->scene_ia.pills[i].w = out->scene_ia.pills[i].h = 0;
        }
    }

    chat_w = sc->chat_mode == OS_FB_CHAT_FLOAT ? clampi(w / 3, 220, 360) : clampi(w / 2, 240, 640);
    if (chat_w > w - 24) chat_w = w - 24;
    chat_h = sc->chat_mode == OS_FB_CHAT_FLOAT ? clampi(h / 2, 180, 400) : clampi((h * 5) / 12, 160, 360);
    if (chat_h > h - bar_h - 56) chat_h = h - bar_h - 56;
    if (chat_h < 120) chat_h = 120;

    if (sc->chat_mode == OS_FB_CHAT_FLOAT) {
        int pane_open = out->pane_win.active && !out->pane_win.maximized;
        int pane_right = pane_open ? (out->pane_win.box.x + out->pane_win.box.w + 12) : 8;
        int max_x = w - chat_w - 8;
        if (pane_open && out->pane_win.box.x > w / 2) pane_right = 8;  /* pane dragged right */
        chat_x = w - chat_w - 16;
        chat_y = h - chat_h - 70;
        if (sc->chat_x || sc->chat_y) {
            chat_x = (int)sc->chat_x * w / 80;
            chat_y = (int)sc->chat_y * h / 25;
        }
        /* Keep the desktop icon column (w - 96) and the dock free: the
         * floating chat used to cover Statut/Fichiers and the dock. */
        if (w >= 780 && h >= 520) {
            int room = (w - 104) - pane_right;
            if (room < chat_w && room >= 220) chat_w = room;
            max_x = w - 104 - chat_w;
        }
        if (max_x < 8) max_x = 8;
        if (pane_right > max_x) pane_right = max_x;
        if (chat_x < pane_right) chat_x = pane_right;
        chat_x = clampi(chat_x, pane_right, max_x);
        if (chat_h > h - 56 - bar_h - 16) chat_h = h - 56 - bar_h - 16;
        chat_y = clampi(chat_y, bar_h + 8, h - 56 - chat_h);
    } else {
        chat_x = (w - chat_w) / 2;
        chat_y = bar_h + (h / 14);
        if (chat_y + chat_h > h - 52) chat_y = bar_h + 8;
    }
    if (sc->chat_mode == OS_FB_CHAT_FLOAT && out->scene_ia.box.w > 0 &&
        chat_x < out->scene_ia.box.x + out->scene_ia.box.w && chat_x + chat_w > out->scene_ia.box.x &&
        chat_y < out->scene_ia.box.y + out->scene_ia.box.h && chat_y + chat_h > out->scene_ia.box.y) {
        /* the chat window and the Scene IA card used to overlap: the chat
         * ends above the card (moved up first when it would get too short) */
        int room = out->scene_ia.box.y - 12 - chat_y;
        if (room < 160) { chat_y = bar_h + 8; room = out->scene_ia.box.y - 12 - chat_y; }
        if (room >= 120) chat_h = room;
        else out->scene_ia.box.w = out->scene_ia.box.h = 0;   /* no room: card hidden */
        if (out->scene_ia.box.w == 0) for (i = 0; i < 3; i++) out->scene_ia.pills[i].w = out->scene_ia.pills[i].h = 0;
    }
    {
        gfx_wm_rect_t base, placed, area;
        area.x = 0; area.y = bar_h + 4; area.w = w; area.h = h - bar_h - 4 - 52;
        if (area.h < 120) area.h = h - bar_h - 4;
        base.x = chat_x; base.y = chat_y; base.w = chat_w; base.h = chat_h;
        out->chat_win.base = from_wm(base);
        gfx_wm_place(&g_wm_chat, base, area, &placed);
        chat_x = placed.x; chat_y = placed.y; chat_w = placed.w; chat_h = placed.h;
        out->chat_win.visible = placed.w > 0;
        out->chat_win.minimized = g_wm_chat.minimized;
        out->chat_win.maximized = g_wm_chat.maximized && placed.w > 0;
    }

    out->chat_win.box.x = chat_x;
    out->chat_win.box.y = chat_y;
    out->chat_win.box.w = chat_w;
    out->chat_win.box.h = chat_h;

    out->chat_win.input.x = chat_x + 14;
    out->chat_win.input.y = chat_y + chat_h - 70;
    out->chat_win.input.w = chat_w - 28;
    out->chat_win.input.h = 28;

    out->chat_win.send_btn.x = chat_x + 14;
    out->chat_win.send_btn.y = chat_y + chat_h - 34;
    out->chat_win.send_btn.w = 88;
    out->chat_win.send_btn.h = 22;
    if (!out->chat_win.visible) {
        out->chat_win.input.w = out->chat_win.input.h = 0;
        out->chat_win.send_btn.w = out->chat_win.send_btn.h = 0;
    }
    wm_buttons(&out->chat_win.box, GFX_DESKTOP_CHAT_TITLE_H, out->chat_win.btn);

    /* active window in the menu bar and open / minimized marks in the dock */
    /* app windows (scene v7) */
    for (i = 0; i < OS_FB_APPS; i++) {
        gfx_wm_rect_t base, placed, area;
        int open = sc->version >= 7 && sc->app[i].kind != 0;
        area.x = 0; area.y = bar_h + 4; area.w = w; area.h = h - bar_h - 4 - 52;
        if (area.h < 120) area.h = h - bar_h - 4;
        base.w = (OS_FB_APP_COLS - 1) * 8 + 24;
        if (base.w > w - 16) base.w = w - 16;
        base.h = GFX_APP_GRID_Y + OS_FB_APP_ROWS * GFX_APP_ROW_H + 30;
        if (base.h > area.h) base.h = area.h;
        base.x = i == 0 ? 24 : w - base.w - 110;
        if (base.x < 8) base.x = 8;
        base.y = bar_h + (i == 0 ? 20 : 56);
        out->app_win[i].base = from_wm(base);
        out->app_win[i].visible = 0;
        out->app_win[i].minimized = open && g_wm_app[i].minimized;
        out->app_win[i].maximized = 0;
        out->app_win[i].box.x = out->app_win[i].box.y = out->app_win[i].box.w = out->app_win[i].box.h = 0;
        if (open && pane != OS_FB_PANE_IDE && pane != OS_FB_PANE_WEB) {
            gfx_wm_place(&g_wm_app[i], base, area, &placed);
            out->app_win[i].box = from_wm(placed);
            out->app_win[i].visible = placed.w > 0;
            out->app_win[i].maximized = g_wm_app[i].maximized && placed.w > 0;
        }
        wm_buttons(&out->app_win[i].box, GFX_DESKTOP_APP_TITLE_H, out->app_win[i].btn);
        out->app_win[i].tool.x = out->app_win[i].box.x + 12;
        out->app_win[i].tool.y = out->app_win[i].box.y + GFX_APP_TOOL_Y - 3;
        out->app_win[i].tool.w = out->app_win[i].box.w > 24 ? out->app_win[i].box.w - 24 : 0;
        out->app_win[i].tool.h = out->app_win[i].visible ? 16 : 0;
        out->app_win[i].grid.x = out->app_win[i].box.x + 12;
        out->app_win[i].grid.y = out->app_win[i].box.y + GFX_APP_GRID_Y;
        out->app_win[i].grid.w = out->app_win[i].tool.w;
        out->app_win[i].grid.h = out->app_win[i].visible ? OS_FB_APP_ROWS * GFX_APP_ROW_H : 0;
    }
    for (i = 0; i < GFX_WIN_COUNT; i++) out->z[i] = g_z[i];
    out->focus = g_focus;
    out->hover_dock = g_hover_dock;

    out->menu_active = -1;
    for (i = 0; i < GFX_DESKTOP_DOCK; i++) out->dock_state[i] = 0;
    {
        /* active item: an app window on top of the stack, else the pane,
         * else the chat */
        int k, top = -1;
        for (k = GFX_WIN_COUNT - 1; k >= 0 && top < 0; k--) {
            int win = g_z[k];
            if (win >= GFX_WIN_APP0 && out->app_win[win - GFX_WIN_APP0].visible) top = 6 + win - GFX_WIN_APP0;
            else if (win == GFX_WIN_PANE && out->pane_win.active) top = -2;
            else if (win == GFX_WIN_CHAT && out->chat_win.visible) top = -2;
        }
        if (top >= 0) out->menu_active = top;
        else if (out->pane_win.active && pane_index(pane) > 0) out->menu_active = pane_index(pane);
        else if (out->chat_win.visible) out->menu_active = 0;
    }
    out->dock_state[0] = out->chat_win.visible ? 1 : (g_wm_chat.minimized ? 2 : 0);
    if (pane_index(pane) > 0) out->dock_state[pane_index(pane)] = out->pane_win.active ? 1 : 2;
    if (gfx_fb_term_minimized()) out->dock_state[2] = 2;
    for (i = 0; i < OS_FB_APPS; i++)
        if (sc->version >= 7 && sc->app[i].kind && (i || !out->dock_state[6])) out->dock_state[6 + i] = out->app_win[i].visible ? 1 : 2;

    dock_w = GFX_DESKTOP_DOCK * 44 + 16;
    if (dock_w > w - 16) dock_w = w - 16;
    dock_x = (w - dock_w) / 2;
    dock_y = h - 48;
    if (dock_y < bar_h + 8) dock_y = h - 36;

    out->dock.box.x = dock_x;
    out->dock.box.y = dock_y;
    out->dock.box.w = dock_w;
    out->dock.box.h = 40;

    for (i = 0; i < GFX_DESKTOP_DOCK; i++) {
        out->dock.icons[i].x = dock_x + 8 + i * 44;
        out->dock.icons[i].y = dock_y + 4;
        out->dock.icons[i].w = 36;
        out->dock.icons[i].h = 32;
    }

    /* Same rule as the icon tiles drawn in gfx_desktop_draw_no_cursor:
     * a 42 px tile at w - 86 and its label below. */
    out->desk.count = 0;
    if (w >= 780 && h >= 520 && pane != OS_FB_PANE_IDE && pane != OS_FB_PANE_WEB) {
        int dgap = (h >= 700) ? 72 : 56;
        for (i = 0; i < 6; i++) {
            if (bar_h + 16 + i * dgap + 58 > h - 52) break;
            out->desk.icons[i].x = w - 96;
            out->desk.icons[i].y = bar_h + 16 + i * dgap;
            out->desk.icons[i].w = 62;
            out->desk.icons[i].h = 58;
            out->desk.count = i + 1;
        }
    }
}

/* App window (explorer / editor) from the scene grid. */
static uint32_t app_attr_rgb(uint8_t a) {
    switch (a) {
    case 1: return RGB(122, 196, 236);   /* directory */
    case 2: return RGB(120, 134, 148);   /* dim */
    case 4: return RGB(243, 230, 176);   /* link / breadcrumb */
    case 5: return RGB(240, 120, 110);   /* error */
    case 6: return RGB(250, 210, 90);    /* match */
    default: return RGB(226, 232, 238);
    }
}

static void draw_app(uint32_t *fb, int w, int h, const gfx_rect_t *box, const os_fb_app_t *a, int focused) {
    int x = box->x, y = box->y, rw = box->w, rh = box->h, r, c, maxc;
    char title[80];
    int n = 0, i;
    round_rect_a(fb, w, h, x, y, rw, rh, 12, RGB(14, 20, 27), 255);
    stroke_round(fb, w, h, x, y, rw, rh, 12, focused ? RGB(122, 212, 196) : RGB(52, 70, 84), focused ? 2 : 1);
    fill_rect(fb, w, h, x + 2, y + 2, rw - 4, GFX_DESKTOP_APP_TITLE_H - 4, focused ? RGB(28, 52, 62) : RGB(24, 34, 44));
    draw_wm_buttons(fb, w, h, x, y, rw, rh, GFX_DESKTOP_APP_TITLE_H);
    for (i = 0; a->title[i] && n < 70; i++) title[n++] = a->title[i];
    if (a->dirty && n < 76) { title[n++] = ' '; title[n++] = '*'; }
    title[n] = 0;
    draw_text_n(fb, w, h, x + 14, y + 11, title, (rw - 110) / 8, RGB(232, 238, 244));
    maxc = (rw - 24) / 8;
    if (maxc > OS_FB_APP_COLS - 1) maxc = OS_FB_APP_COLS - 1;
    fill_rect(fb, w, h, x + 8, y + GFX_APP_TOOL_Y - 5, rw - 16, 18, RGB(22, 32, 42));
    draw_text_n(fb, w, h, x + 12, y + GFX_APP_TOOL_Y, a->tool, maxc, RGB(122, 212, 196));
    fill_rect(fb, w, h, x + 8, y + GFX_APP_GRID_Y - 2, rw - 16, OS_FB_APP_ROWS * GFX_APP_ROW_H + 4, RGB(9, 13, 18));
    for (r = 0; r < OS_FB_APP_ROWS && r < (int)a->nrows; r++) {
        int gy = y + GFX_APP_GRID_Y + r * GFX_APP_ROW_H;
        if (gy + GFX_APP_ROW_H > y + rh - 26) break;
        for (c = 0; c < maxc && a->rows[r][c]; c++) {
            uint8_t at = a->attr[r][c];
            int gx = x + 12 + c * 8;
            if (at == 3) {
                fill_rect(fb, w, h, gx, gy, 8, GFX_APP_ROW_H, RGB(46, 110, 100));
                draw_char(fb, w, h, gx, gy + 3, a->rows[r][c], RGB(240, 248, 250));
            } else if (a->rows[r][c] != ' ')
                draw_char(fb, w, h, gx, gy + 3, a->rows[r][c], app_attr_rgb(at));
        }
    }
    if (focused && a->kind == OS_FB_APP_EDIT && a->cur_row < OS_FB_APP_ROWS && a->cur_col < maxc) {
        int gy = y + GFX_APP_GRID_Y + a->cur_row * GFX_APP_ROW_H;
        fill_rect(fb, w, h, x + 12 + a->cur_col * 8, gy + 1, 2, GFX_APP_ROW_H - 2, RGB(250, 210, 90));
    }
    fill_rect(fb, w, h, x + 8, y + rh - 22, rw - 16, 16, RGB(22, 32, 42));
    draw_text_n(fb, w, h, x + 12, y + rh - 18, a->status, maxc, RGB(201, 232, 223));
}

/* Dock pictograms (16x16-ish vector drawings, no font letters). */
static void dock_picto(uint32_t *fb, int w, int h, int x, int y, int i) {
    uint32_t k = RGB(15, 20, 25), l = RGB(232, 238, 244);
    switch (i) {
    case 0: /* chat bubble */
        round_rect(fb, w, h, x + 6, y + 6, 24, 15, 5, k);
        fill_rect(fb, w, h, x + 10, y + 21, 4, 4, k);
        fill_rect(fb, w, h, x + 11, y + 12, 3, 3, l); fill_rect(fb, w, h, x + 17, y + 12, 3, 3, l); fill_rect(fb, w, h, x + 23, y + 12, 3, 3, l);
        break;
    case 1: /* globe */
        draw_circle_kind(fb, w, h, x + 18, y + 16, 10, k);
        fill_rect(fb, w, h, x + 8, y + 15, 21, 2, l);
        fill_rect(fb, w, h, x + 17, y + 6, 2, 21, l);
        break;
    case 2: /* terminal */
        fill_rect(fb, w, h, x + 6, y + 6, 24, 20, k);
        draw_text(fb, w, h, x + 9, y + 12, ">_", l);
        break;
    case 3: /* gear */
        draw_circle_kind(fb, w, h, x + 18, y + 16, 9, k);
        fill_rect(fb, w, h, x + 16, y + 4, 4, 24, k); fill_rect(fb, w, h, x + 6, y + 14, 24, 4, k);
        draw_circle_kind(fb, w, h, x + 18, y + 16, 3, l);
        break;
    case 4: /* life buoy */
        draw_circle_kind(fb, w, h, x + 18, y + 16, 10, k);
        draw_circle_kind(fb, w, h, x + 18, y + 16, 5, RGB(61, 154, 138));
        fill_rect(fb, w, h, x + 8, y + 15, 4, 3, l); fill_rect(fb, w, h, x + 24, y + 15, 4, 3, l);
        break;
    case 5: /* bars */
        fill_rect(fb, w, h, x + 8, y + 18, 5, 8, k); fill_rect(fb, w, h, x + 15, y + 12, 5, 14, k); fill_rect(fb, w, h, x + 22, y + 7, 5, 19, k);
        break;
    case 6: /* folder */
        fill_rect(fb, w, h, x + 6, y + 8, 10, 4, k);
        fill_rect(fb, w, h, x + 6, y + 11, 24, 15, k);
        fill_rect(fb, w, h, x + 8, y + 14, 20, 1, l);
        break;
    default: /* page with lines */
        fill_rect(fb, w, h, x + 10, y + 5, 16, 22, k);
        fill_rect(fb, w, h, x + 13, y + 10, 10, 2, l); fill_rect(fb, w, h, x + 13, y + 15, 10, 2, l); fill_rect(fb, w, h, x + 13, y + 20, 7, 2, l);
        break;
    }
}

void gfx_desktop_draw_no_cursor(const os_fb_scene_t *scene, uint32_t *fb, int w, int h) {
    static os_fb_scene_t def;
    const os_fb_scene_t *sc = scene;
    int bar_h, chat_w, chat_h, chat_x, chat_y, scene_w, scene_h, scene_x, scene_y;
    int dock_w, dock_x, dock_y, i;
    uint8_t pane;
    const char *ptitle;
    gfx_desktop_layout_t lay;
    const char *menus[GFX_DESKTOP_MENUS] = { "Chat", "Browser-OS", "Shell", "Admin", "Support", "Statut", "Fichiers", "Editeur" };
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
    } else {
        g_last_scene = *sc;
        g_last_scene_valid = 1;
    }
    {
        /* a pane that opens (or changes) comes to the front */
        static uint8_t pane_prev;
        if (sc->pane != pane_prev && sc->pane != OS_FB_PANE_NONE) z_raise(GFX_WIN_PANE);
        pane_prev = sc->pane;
    }
    if (sc->version >= 7) {
        /* userspace owns the focus: an app that opens, or takes the focus,
         * comes to the front */
        for (i = 0; i < OS_FB_APPS; i++) {
            if (sc->app[i].kind && !g_app_kind_prev[i]) { gfx_wm_reset(&g_wm_app[i]); z_raise(GFX_WIN_APP0 + i); }
            g_app_kind_prev[i] = sc->app[i].kind;
        }
        if (sc->app_focus != g_app_focus_prev) {
            if (sc->app_focus < OS_FB_APPS && sc->app[sc->app_focus].kind) {
                z_raise(GFX_WIN_APP0 + sc->app_focus);
                g_wm_app[sc->app_focus].minimized = 0;
            }
            g_app_focus_prev = sc->app_focus;
        }
        g_focus = sc->app_focus < OS_FB_APPS ? GFX_WIN_APP0 + sc->app_focus : -1;
    }

    /* one layout for drawing and for the click hit test */
    gfx_desktop_get_layout(sc, w, h, &lay);
    landscape(fb, w, h);
    bar_h = clampi(h / 22, 28, 40);
    fill_rect_a(fb, w, h, 0, 0, w, bar_h, RGB(12, 28, 40), 235);
    fill_rect(fb, w, h, 12, (bar_h - 12) / 2, 12, 12, RGB(61, 154, 138));
    draw_text(fb, w, h, 30, (bar_h - 8) / 2, "Mohhdy OS", RGB(232, 238, 244));
    {
        int mx = 122;
        int ty = (bar_h - 8) / 2;
        int i;
        const int gap = w < 720 ? 8 : 14;
        int lens[GFX_DESKTOP_MENUS];
        int flag_w = (w >= 920) ? (8 * 36 + 12) : ((w >= 720) ? (8 * 16 + 12) : 8);
        for (i = 0; i < GFX_DESKTOP_MENUS; i++) {
            int n = 0;
            while (menus[i][n]) n++;
            lens[i] = n;
        }
        for (i = 0; i < GFX_DESKTOP_MENUS; i++) {
            if (mx + lens[i] * 8 > w - flag_w - 8) break;
            if (i == lay.menu_active) {
                /* active window: highlighted menu item */
                round_rect(fb, w, h, mx - 6, ty - 5, lens[i] * 8 + 12, 18, 8, RGB(61, 154, 138));
                draw_text(fb, w, h, mx, ty, menus[i], RGB(15, 20, 25));
            } else
                draw_text(fb, w, h, mx, ty, menus[i], RGB(232, 238, 244));
            mx += lens[i] * 8 + gap;
        }
        if (w >= 920) {
            char lab[64];
            llm_text(lab, (int)sizeof(lab), "IA : ", sc, "");
            draw_text_n(fb, w, h, w - 8 * 36 - 12, ty, lab, 36, RGB(243, 230, 176));
        } else if (w >= 720)
            draw_text(fb, w, h, w - 8 * 16 - 8, ty, "Mohhdy OS", RGB(243, 230, 176));
    }

    mode_s = "en attente";
    if (sc->stage_mode == OS_FB_STAGE_ACTING) mode_s = "en action";
    if (sc->stage_mode == OS_FB_STAGE_PRESENTING) mode_s = "resultats";
    round_rect_a(fb, w, h, 12, bar_h + 8, 96, 18, 9, RGB(11, 16, 20), 160);
    stroke_round(fb, w, h, 12, bar_h + 8, 96, 18, 9, RGB(61, 154, 138), 1);
    draw_text(fb, w, h, 22, bar_h + 13, mode_s, RGB(122, 212, 196));
    if (w >= 700) {
        char lab[64];
        int ln = 0;
        llm_text(lab, (int)sizeof(lab), "Assistant : ", sc, "");
        draw_text(fb, w, h, 118, bar_h + 13, lab, RGB(243, 230, 176));
        while (lab[ln]) ln++;
        if (ln < 13) ln = 13;
        if (w >= 980)
            draw_text(fb, w, h, 118 + (ln + 2) * 8, bar_h + 13, "Clic : ouvrir  Glisser : deplacer  /help", RGB(154, 168, 181));
    }

    if (w >= 780 && h >= 520) {
        int gap = (h >= 700) ? 72 : 56;
        for (i = 0; i < 6; i++) {
            if (bar_h + 16 + i * gap + 58 > h - 52) break;
            icon_tile(fb, w, h, w - 86, bar_h + 16 + i * gap, icons[i].c0, icons[i].c1, icons[i].label);
        }
    }

    scene_x = lay.scene_ia.box.x;
    scene_y = lay.scene_ia.box.y;
    scene_w = lay.scene_ia.box.w;
    scene_h = lay.scene_ia.box.h;
    if (scene_w > 0) {
    round_rect_a(fb, w, h, scene_x, scene_y, scene_w, scene_h, 16, RGB(11, 16, 20), 150);
    stroke_round(fb, w, h, scene_x, scene_y, scene_w, scene_h, 16, RGB(61, 154, 138), 1);
    draw_text(fb, w, h, scene_x + 16, scene_y + 12, "Scene IA", RGB(232, 238, 244));
    caption = " . en attente d'un prompt";
    if (sc->stage_mode == OS_FB_STAGE_ACTING) caption = " . action stub";
    if (sc->stage_mode == OS_FB_STAGE_PRESENTING) caption = " . resultats";
    {
        char lab[80];
        llm_text(lab, (int)sizeof(lab), "", sc, caption);
        draw_text(fb, w, h, scene_x + 16, scene_y + 28, lab, RGB(154, 168, 181));
    }
    draw_stage_body(fb, w, h, scene_x + 12, scene_y + 48, scene_w - 24, scene_h - 60, sc);
    }

    pane = sc->pane;
    if (pane == OS_FB_PANE_IDE) {
        draw_ide(fb, w, h, 8, bar_h + 8, w - 16, h - bar_h - 16, sc);
        return;
    }
    if (pane == OS_FB_PANE_WEB && sc->version >= 4) {
        draw_web(fb, w, h, sc);
        return;
    }
    ptitle = pane_title(pane);
    {
        int k;
        for (k = 0; k < GFX_WIN_COUNT; k++) {
            int win = lay.z[k];
            if (win == GFX_WIN_PANE) {
    if (ptitle && lay.pane_win.active) {
        int wx = lay.pane_win.box.x, wy = lay.pane_win.box.y;
        int ww = lay.pane_win.box.w;
        int wh = lay.pane_win.box.h;
        const char *body = "Cerveau = osui_runtime.c. Pas Chromium.";
        if (pane == OS_FB_PANE_BROWSER) body = "Browser-OS : pages locales /web, tapez web-render /web/<page>";
        else if (pane == OS_FB_PANE_FS) body = "Inspection VFS Sandboxee (FS-as-web) : initrd/, overlay/, fat16/, fat32/";
        else if (pane == OS_FB_PANE_STATUS) body = "chrome=qemu_fb display_surface=vbe_lfb";
        else if (pane == OS_FB_PANE_P2P) body = "P2P : sortie de p2p-stats (aucune sortie recue)";
        else if (pane == OS_FB_PANE_COLLAB) body = "Collab : sortie de collab-balances (aucune sortie recue)";
        else if (pane == OS_FB_PANE_PROD) body = "Production : sortie de prod-metrics (aucune sortie recue)";

        if (pane == OS_FB_PANE_SHELL)
            draw_shell(fb, w, h, wx, wy, ww, wh, sc);
        else
            draw_window(fb, w, h, wx, wy, ww, wh, ptitle, body, sc);
    }

            } else if (win == GFX_WIN_CHAT) {
    chat_x = lay.chat_win.box.x;
    chat_y = lay.chat_win.box.y;
    chat_w = lay.chat_win.box.w;
    chat_h = lay.chat_win.box.h;
    if (lay.chat_win.visible) draw_chat(fb, w, h, chat_x, chat_y, chat_w, chat_h, sc);

            } else {
                int slot = win - GFX_WIN_APP0;
                if (lay.app_win[slot].visible)
                    draw_app(fb, w, h, &lay.app_win[slot].box, &sc->app[slot], lay.focus == win);
            }
        }
    }

    dock_w = GFX_DESKTOP_DOCK * 44 + 16;
    if (dock_w > w - 16) dock_w = w - 16;
    dock_x = (w - dock_w) / 2;
    dock_y = h - 48;
    if (dock_y < bar_h + 8) dock_y = h - 36;
    round_rect_a(fb, w, h, dock_x, dock_y, dock_w, 40, 16, RGB(16, 26, 34), 220);
    for (i = 0; i < GFX_DESKTOP_DOCK; i++) {
        round_rect(fb, w, h, dock_x + 8 + i * 44, dock_y + 4, 36, 32, 10,
                   i == lay.hover_dock ? RGB(122, 212, 196) : RGB(61, 154, 138));
        dock_picto(fb, w, h, dock_x + 8 + i * 44, dock_y + 3, i);
        if (lay.dock_state[i]) {
            /* open (light) or minimized (amber) window mark under the icon */
            uint32_t c = lay.dock_state[i] == 2 ? RGB(230, 180, 60) : RGB(232, 238, 244);
            fill_rect(fb, w, h, dock_x + 8 + i * 44 + 14, dock_y + 36, 8, 3, c);
        }
    }
    if (lay.hover_dock >= 0 && lay.hover_dock < GFX_DESKTOP_DOCK) {
        /* tooltip above the hovered dock icon */
        const char *name = gfx_desktop_dock_names[lay.hover_dock];
        int n = 0, tx;
        while (name[n]) n++;
        tx = dock_x + 8 + lay.hover_dock * 44 + 18 - (n * 8 + 16) / 2;
        if (tx < 4) tx = 4;
        if (tx + n * 8 + 16 > w - 4) tx = w - 4 - n * 8 - 16;
        round_rect(fb, w, h, tx, dock_y - 26, n * 8 + 16, 20, 6, RGB(232, 238, 244));
        draw_text(fb, w, h, tx + 8, dock_y - 20, name, RGB(15, 20, 25));
    }

}

void gfx_desktop_draw(const os_fb_scene_t *scene, uint32_t *fb, int w, int h) {
    gfx_desktop_draw_no_cursor(scene, fb, w, h);
    if (g_mouse_x < 0) g_mouse_x = w / 2;
    if (g_mouse_y < 0) g_mouse_y = h / 2;
    if (g_mouse_x >= w) g_mouse_x = w - 1;
    if (g_mouse_y >= h) g_mouse_y = h - 1;

    gfx_desktop_draw_cursor(fb, w, h, w, g_mouse_x, g_mouse_y, g_mouse_buttons);
}
