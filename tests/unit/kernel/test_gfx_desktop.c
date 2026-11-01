#include <stdio.h>
/* test_gfx_desktop.c - Pixels du bureau VBE (glassmorphic, pas ASCII 80x25). */

#include "../../framework/unity.h"
#include "../../../include/os_ui_scale.h"
#include <string.h>
#include "../../../kernel/gfx_desktop.h"
#include "../../../kernel/gfx_fb.h"
#include "../../../kernel/gfx_wm.h"
#include "../../../kernel/clipboard.h"
#include "../../../kernel/vga_console.h"
#include "../../../kernel/keyboard.h"
#include <string.h>

#define W 1024
#define H 768

static uint32_t g_fb[W * H];
static os_fb_scene_t g_scene;

static unsigned red(uint32_t c) { return (c >> 16) & 255u; }
static unsigned grn(uint32_t c) { return (c >> 8) & 255u; }
static unsigned blu(uint32_t c) { return c & 255u; }

static int near_rgb(uint32_t c, unsigned r, unsigned g, unsigned b, unsigned tol) {
    unsigned dr = red(c) > r ? red(c) - r : r - red(c);
    unsigned dg = grn(c) > g ? grn(c) - g : g - grn(c);
    unsigned db = blu(c) > b ? blu(c) - b : b - blu(c);
    return dr <= tol && dg <= tol && db <= tol;
}

static int count_near_buf(uint32_t *fb, int w, int h, int x0, int y0, int x1, int y1,
                          unsigned r, unsigned g, unsigned b, unsigned tol) {
    int x, y, n = 0;
    for (y = y0; y < y1; y++) {
        for (x = x0; x < x1; x++) {
            if (near_rgb(gfx_desktop_pixel(fb, w, h, x, y), r, g, b, tol)) n++;
        }
    }
    return n;
}

static int count_near(int x0, int y0, int x1, int y1, unsigned r, unsigned g, unsigned b, unsigned tol) {
    return count_near_buf(g_fb, W, H, x0, y0, x1, y1, r, g, b, tol);
}

static void blank_scene(void) {
    memset(&g_scene, 0, sizeof(g_scene));
    g_scene.magic = OS_FB_MAGIC;
    g_scene.version = 1;
    g_scene.chat_mode = OS_FB_CHAT_CENTER;
    g_scene.pane = OS_FB_PANE_NONE;
    g_scene.stage_mode = OS_FB_STAGE_REFLECTING;
    g_scene.stage_kind = OS_FB_KIND_PLAN;
    memcpy(g_scene.session, "s0001", 6);
}

static void test_center_desktop_not_text_mode(void) {
    uint32_t a, b, c;
    blank_scene();
    gfx_desktop_draw(&g_scene, g_fb, W, H);
    a = gfx_desktop_pixel(g_fb, W, H, 8, 8);
    b = gfx_desktop_pixel(g_fb, W, H, 512, 40);
    c = gfx_desktop_pixel(g_fb, W, H, 40, 400);
    TEST_ASSERT(a != 0);
    TEST_ASSERT(a != 0x00ffffffu);
    TEST_ASSERT(a != b || b != c);
    /* Top bar is dark teal, wallpaper is a different landscape tone. */
    TEST_ASSERT(red(a) < 80);
    TEST_ASSERT(blu(c) > 20);
}

static void test_chat_and_dock_and_stage(void) {
    int teal;
    blank_scene();
    gfx_desktop_draw(&g_scene, g_fb, W, H);
    teal = count_near(400, 700, 640, 760, 61, 154, 138, 40);
    TEST_ASSERT(teal > 80);
    /* Scene IA pills around bottom-center. */
    TEST_ASSERT(count_near(300, 520, 760, 700, 43, 122, 110, 50) > 40);
    /* Right dock icon (Browser-OS blue). */
    TEST_ASSERT(count_near(940, 50, 1010, 120, 80, 140, 180, 70) > 20);
}

static void test_float_chat_and_browser_pane(void) {
    uint32_t center, right;
    blank_scene();
    g_scene.chat_mode = OS_FB_CHAT_FLOAT;
    g_scene.pane = OS_FB_PANE_BROWSER;
    g_scene.chat_x = 16; /* leftover center VGA column must not cover the pane */
    g_scene.chat_y = 5;
    gfx_desktop_draw(&g_scene, g_fb, W, H);
    center = gfx_desktop_pixel(g_fb, W, H, 200, 200);
    right = gfx_desktop_pixel(g_fb, W, H, 900, 500);
    TEST_ASSERT(center != right);
    TEST_ASSERT(count_near(40, 70, 400, 200, 28, 44, 56, 40) > 30);
    /* Floating chat stays on the right of the Browser-OS window. */
    TEST_ASSERT(count_near(640, 200, 1000, 620, 11, 16, 20, 20) > 40);
}

static void test_shell_pane_shows_prompt(void) {
    blank_scene();
    g_scene.chat_mode = OS_FB_CHAT_FLOAT;
    g_scene.pane = OS_FB_PANE_SHELL;
    g_scene.chat_x = 51;
    gfx_desktop_draw(&g_scene, g_fb, W, H);
    /* Title bar of Shell Multiboot. */
    TEST_ASSERT(count_near(40, 70, 400, 110, 28, 44, 56, 40) > 30);
    /* Terminal body is near-black, not the landscape. */
    TEST_ASSERT(count_near(50, 140, 400, 400, 8, 12, 16, 12) > 80);
    /* Prompt cursor near the bottom of the pane. */
    TEST_ASSERT(count_near(50, 540, 200, 640, 61, 154, 138, 50) > 8);
}

static void test_circle_kind_paints_ring(void) {
    blank_scene();
    g_scene.stage_kind = OS_FB_KIND_CIRCLE;
    g_scene.stage_mode = OS_FB_STAGE_PRESENTING;
    gfx_desktop_draw(&g_scene, g_fb, W, H);
    TEST_ASSERT(count_near(430, 560, 620, 700, 100, 180, 170, 80) > 10);
}

static int overlap(const gfx_rect_t *a, const gfx_rect_t *b) {
    return a->w > 0 && b->w > 0 && a->x < b->x + b->w && b->x < a->x + a->w
        && a->y < b->y + b->h && b->y < a->y + a->h;
}

/* Usability audit: with a pane open the Scene IA is hidden (it showed
 * through the pane and its pills stayed clickable under it), and the
 * floating chat keeps clear of the pane, the right icon column and the
 * dock, for every pane and at several screen sizes. */
static void test_pane_layout_has_no_overlaps(void) {
    static const int sizes[3][2] = { { 1024, 768 }, { 1280, 720 }, { 800, 600 } };
    gfx_desktop_layout_t l;
    int k, pane, i;
    for (k = 0; k < 3; k++) {
        for (pane = OS_FB_PANE_BROWSER; pane <= OS_FB_PANE_PROD; pane++) {
            blank_scene();
            g_scene.chat_mode = OS_FB_CHAT_FLOAT;
            g_scene.pane = (uint8_t)pane;
            gfx_desktop_get_layout(&g_scene, sizes[k][0], sizes[k][1], &l);
            TEST_ASSERT_EQUAL(0, l.scene_ia.box.w);
            for (i = 0; i < 3; i++) TEST_ASSERT_EQUAL(0, l.scene_ia.pills[i].w);
            TEST_ASSERT(!overlap(&l.chat_win.box, &l.pane_win.box));
            TEST_ASSERT(!overlap(&l.chat_win.box, &l.dock.box));
            for (i = 0; i < l.desk.count; i++) TEST_ASSERT(!overlap(&l.chat_win.box, &l.desk.icons[i]));
        }
    }
    blank_scene();
    gfx_desktop_get_layout(&g_scene, 1024, 768, &l);
    TEST_ASSERT(l.scene_ia.box.w > 0);   /* no pane: Scene IA is there */
    /* UX3: floating chat without a pane: it used to cover the Scene IA card */
    for (k = 0; k < 3; k++) {
        blank_scene();
        g_scene.chat_mode = OS_FB_CHAT_FLOAT;
        gfx_desktop_get_layout(&g_scene, sizes[k][0], sizes[k][1], &l);
        TEST_ASSERT(l.chat_win.box.h >= 120);
        TEST_ASSERT(!overlap(&l.chat_win.box, &l.scene_ia.box));
    }
}

static void test_layout_scales_with_framebuffer(void) {
    static uint32_t small[800 * 600];
    static uint32_t tiny[640 * 400];
    static uint32_t wide[1280 * 720];
    uint32_t a;
    blank_scene();
    gfx_desktop_draw(&g_scene, small, 800, 600);
    a = gfx_desktop_pixel(small, 800, 600, 12, 12);
    TEST_ASSERT(red(a) < 80);
    TEST_ASSERT(count_near_buf(small, 800, 600, 200, 540, 600, 598, 61, 154, 138, 40) > 40);

    gfx_desktop_draw(&g_scene, tiny, 640, 400);
    a = gfx_desktop_pixel(tiny, 640, 400, 12, 12);
    TEST_ASSERT(red(a) < 80);
    TEST_ASSERT(count_near_buf(tiny, 640, 400, 140, 350, 500, 398, 61, 154, 138, 40) > 20);
    TEST_ASSERT(count_near_buf(tiny, 640, 400, 150, 40, 490, 240, 16, 25, 32, 50) > 40);

    gfx_desktop_draw(&g_scene, wide, 1280, 720);
    a = gfx_desktop_pixel(wide, 1280, 720, 12, 12);
    TEST_ASSERT(red(a) < 80);
    TEST_ASSERT(count_near_buf(wide, 1280, 720, 500, 670, 780, 718, 61, 154, 138, 40) > 40);
}

static void test_parse_fit_line(void) {
    int w = 0, h = 0;
    TEST_ASSERT(gfx_fb_parse_fit_line("1280 720", &w, &h));
    TEST_ASSERT_EQUAL(1280, w);
    TEST_ASSERT_EQUAL(720, h);
    TEST_ASSERT(gfx_fb_parse_fit_line("800x600\n", &w, &h));
    TEST_ASSERT_EQUAL(800, w);
    TEST_ASSERT_EQUAL(600, h);
    TEST_ASSERT(gfx_fb_parse_fit_line("240 180", &w, &h));
    TEST_ASSERT_EQUAL(GFX_FB_MIN_WIDTH, w);
    TEST_ASSERT_EQUAL(GFX_FB_MIN_HEIGHT, h);
    TEST_ASSERT(!gfx_fb_parse_fit_line("nope", &w, &h));
}

static void test_mouse_cursor_rendering(void) {
    int mx = 0, my = 0;
    uint8_t btn = 0;
    blank_scene();
    gfx_desktop_set_mouse(150, 120, 0);
    gfx_desktop_get_mouse(&mx, &my, &btn);
    TEST_ASSERT_EQUAL(150, mx);
    TEST_ASSERT_EQUAL(120, my);
    TEST_ASSERT_EQUAL(0, btn);

    gfx_desktop_draw(&g_scene, g_fb, W, H);
    /* Top-left pixel of software cursor at (150, 120) should be border '#' -> RGB(10, 16, 20) */
    uint32_t cursor_px = gfx_desktop_pixel(g_fb, W, H, 150, 120);
    TEST_ASSERT(near_rgb(cursor_px, 10, 16, 20, 5));

    /* Move mouse (PS/2 relative mode x2 gain when !usb_tablet_present()) */
    gfx_desktop_move_mouse(10, 15, 1);
    gfx_desktop_get_mouse(&mx, &my, &btn);
    TEST_ASSERT_EQUAL(170, mx);
    TEST_ASSERT_EQUAL(150, my);
    TEST_ASSERT_EQUAL(1, btn);
}

static void test_fb_present_repeated(void) {
    blank_scene();
    TEST_ASSERT_EQUAL(0, gfx_fb_present(&g_scene));
    TEST_ASSERT_EQUAL(0, gfx_fb_present(&g_scene));
}

static void drain_kbd(void) {
    char c;
    while (kbd_get_char_nonblock(&c)) {
    }
}

static void read_kbd(char *buf, int max) {
    int n = 0;
    char c;
    if (!buf || max <= 0) return;
    while (n < max - 1 && kbd_get_char_nonblock(&c)) {
        buf[n++] = c;
    }
    buf[n] = 0;
}

static void click_at(int x, int y) {
    gfx_desktop_set_mouse(x, y, 0);
    gfx_desktop_set_mouse(x, y, 1);
    gfx_desktop_set_mouse(x, y, 0);
}

static int rect_center_x(const gfx_rect_t *r) {
    return r->x + r->w / 2;
}

static int rect_center_y(const gfx_rect_t *r) {
    return r->y + r->h / 2;
}

static void test_desktop_layout_and_hit_test(void) {
    gfx_desktop_layout_t layout;
    char got[48];
    blank_scene();
    gfx_desktop_get_layout(&g_scene, W, H, &layout);

    /* Verify layout fields calculated properly */
    TEST_ASSERT(layout.bar_h >= 28 && layout.bar_h <= 40);
    TEST_ASSERT_EQUAL(GFX_DESKTOP_MENUS, layout.menu_count);
    TEST_ASSERT(layout.dock.box.w > 200);
    TEST_ASSERT(layout.chat_win.send_btn.w > 50);
    TEST_ASSERT(layout.scene_ia.pills[0].w > 20);

    /* Click on "Envoyer" button when desktop active */
    vga_desktop_set(1);
    gfx_desktop_draw_no_cursor(&g_scene, g_fb, W, H);
    drain_kbd();

    click_at(layout.chat_win.send_btn.x + 5, layout.chat_win.send_btn.y + 5);
    read_kbd(got, (int)sizeof(got));
    TEST_ASSERT_EQUAL_STRING("\n", got);

    vga_desktop_set(0);
}

static void test_hit_regions_dock_menu_stage_close(void) {
    gfx_desktop_layout_t layout;
    char got[48];

    blank_scene();
    vga_desktop_set(1);
    gfx_desktop_draw_no_cursor(&g_scene, g_fb, W, H);
    gfx_desktop_get_layout(&g_scene, W, H, &layout);
    drain_kbd();

    /* Dock /shell (index 2) */
    click_at(rect_center_x(&layout.dock.icons[2]), rect_center_y(&layout.dock.icons[2]));
    read_kbd(got, (int)sizeof(got));
    TEST_ASSERT_EQUAL_STRING("\x16" "cmd /shell\n", got);

    /* Top menu Browser-OS (index 1) */
    drain_kbd();
    click_at(rect_center_x(&layout.menu_items[1]), rect_center_y(&layout.menu_items[1]));
    read_kbd(got, (int)sizeof(got));
    TEST_ASSERT_EQUAL_STRING("\x16" "cmd /browser\n", got);

    /* Stage mode badge */
    drain_kbd();
    click_at(rect_center_x(&layout.stage_badge), rect_center_y(&layout.stage_badge));
    read_kbd(got, (int)sizeof(got));
    TEST_ASSERT_EQUAL_STRING("\x16" "cmd /stage\n", got);

    /* Scene IA first pill */
    drain_kbd();
    click_at(rect_center_x(&layout.scene_ia.pills[0]), rect_center_y(&layout.scene_ia.pills[0]));
    read_kbd(got, (int)sizeof(got));
    TEST_ASSERT_EQUAL_STRING("\x16" "cmd /stage-prompt reflexion\n", got);

    /* Pane close (traffic) requires an active pane in last scene */
    blank_scene();
    g_scene.chat_mode = OS_FB_CHAT_FLOAT;
    g_scene.pane = OS_FB_PANE_SHELL;
    gfx_desktop_draw_no_cursor(&g_scene, g_fb, W, H);
    gfx_desktop_get_layout(&g_scene, W, H, &layout);
    TEST_ASSERT(layout.pane_win.active);
    drain_kbd();
    click_at(rect_center_x(&layout.pane_win.traffic), rect_center_y(&layout.pane_win.traffic));
    read_kbd(got, (int)sizeof(got));
    TEST_ASSERT_EQUAL_STRING("/center\n", got);

    vga_desktop_set(0);
}

/* UI test: the desktop icons on the right open their pane on click,
 * except where the floating chat window covers them. */
static void test_desktop_icons_click(void) {
    gfx_desktop_layout_t layout;
    char got[48];
    blank_scene();
    vga_desktop_set(1);
    gfx_desktop_draw_no_cursor(&g_scene, g_fb, W, H);
    gfx_desktop_get_layout(&g_scene, W, H, &layout);
    TEST_ASSERT_EQUAL(6, layout.desk.count);
    drain_kbd();
    click_at(rect_center_x(&layout.desk.icons[4]), rect_center_y(&layout.desk.icons[4]));
    read_kbd(got, (int)sizeof(got));
    TEST_ASSERT_EQUAL_STRING("\x16" "cmd /status\n", got);
    drain_kbd();
    click_at(rect_center_x(&layout.desk.icons[5]), rect_center_y(&layout.desk.icons[5]));
    read_kbd(got, (int)sizeof(got));
    /* the Fichiers icon opens the explorer app; the FS pane stays on /fs */
    TEST_ASSERT_EQUAL_STRING("\x16" "cmd /files\n", got);
    /* the icon tile really is drawn inside the hit box (Statut grey) */
    TEST_ASSERT(count_near(layout.desk.icons[4].x, layout.desk.icons[4].y,
                           layout.desk.icons[4].x + layout.desk.icons[4].w,
                           layout.desk.icons[4].y + 42, 100, 120, 136, 60) > 100);
    /* floating chat beside the icons: icons stay clickable */
    blank_scene();
    g_scene.chat_mode = OS_FB_CHAT_FLOAT;
    gfx_desktop_draw_no_cursor(&g_scene, g_fb, W, H);
    gfx_desktop_get_layout(&g_scene, W, H, &layout);
    drain_kbd();
    {
        int k, hit = -1;
        for (k = 0; k < layout.desk.count; k++) {
            int cx = rect_center_x(&layout.desk.icons[k]), cy = rect_center_y(&layout.desk.icons[k]);
            if (cx >= layout.chat_win.box.x && cx < layout.chat_win.box.x + layout.chat_win.box.w &&
                cy >= layout.chat_win.box.y && cy < layout.chat_win.box.y + layout.chat_win.box.h &&
                !(cx >= layout.chat_win.send_btn.x && cx < layout.chat_win.send_btn.x + layout.chat_win.send_btn.w &&
                  cy >= layout.chat_win.send_btn.y && cy < layout.chat_win.send_btn.y + layout.chat_win.send_btn.h)) {
                hit = k;
                break;
            }
        }
        /* the floating chat no longer covers any desktop icon (audit fix):
         * every icon stays clickable while the chat floats */
        TEST_ASSERT_EQUAL(-1, hit);
        click_at(rect_center_x(&layout.desk.icons[4]), rect_center_y(&layout.desk.icons[4]));
        read_kbd(got, (int)sizeof(got));
        TEST_ASSERT_EQUAL_STRING("\x16" "cmd /status\n", got);
        drain_kbd();
    }
    /* full screen web pane: no desktop icons */
    blank_scene();
    g_scene.version = 4;
    g_scene.pane = OS_FB_PANE_WEB;
    gfx_desktop_get_layout(&g_scene, W, H, &layout);
    TEST_ASSERT_EQUAL(0, layout.desk.count);
    vga_desktop_set(0);
}

/* UI test: from scene version 5 the labels name the real llm; older
 * scenes keep the stub_echo pixels. */
static void test_llm_label_from_scene(void) {
    static uint32_t a[W * H];
    int x, y, diff = 0, diff_old = 0;
    blank_scene();
    g_scene.version = 4;
    gfx_desktop_draw(&g_scene, a, W, H);
    blank_scene();
    g_scene.version = 4;
    memcpy(g_scene.llm, "gpt2_local", 11);
    gfx_desktop_draw(&g_scene, g_fb, W, H);
    for (y = 0; y < 80; y++)
        for (x = 0; x < W; x++)
            if (gfx_desktop_pixel(a, W, H, x, y) != gfx_desktop_pixel(g_fb, W, H, x, y)) diff_old++;
    TEST_ASSERT_EQUAL(0, diff_old);
    g_scene.version = 5;
    gfx_desktop_draw(&g_scene, g_fb, W, H);
    /* top bar label and stage badge line (y < 80) changed */
    for (y = 0; y < 80; y++)
        for (x = 0; x < W; x++)
            if (gfx_desktop_pixel(a, W, H, x, y) != gfx_desktop_pixel(g_fb, W, H, x, y)) diff++;
    TEST_ASSERT(diff > 50);
    /* version 5 with stub_echo draws like the old scene */
    memcpy(g_scene.llm, "stub_echo", 10);
    gfx_desktop_draw(&g_scene, g_fb, W, H);
    diff = 0;
    for (y = 0; y < H; y++)
        for (x = 0; x < W; x++)
            if (gfx_desktop_pixel(a, W, H, x, y) != gfx_desktop_pixel(g_fb, W, H, x, y)) diff++;
    TEST_ASSERT_EQUAL(0, diff);
}

static void test_p2p_collab_prod_panes_drawing(void) {
    blank_scene();
    g_scene.chat_mode = OS_FB_CHAT_FLOAT;
    g_scene.pane = OS_FB_PANE_P2P;
    gfx_desktop_draw(&g_scene, g_fb, W, H);
    TEST_ASSERT(count_near(40, 70, 400, 110, 28, 44, 56, 40) > 30);

    g_scene.pane = OS_FB_PANE_COLLAB;
    gfx_desktop_draw(&g_scene, g_fb, W, H);
    TEST_ASSERT(count_near(40, 70, 400, 110, 28, 44, 56, 40) > 30);

    g_scene.pane = OS_FB_PANE_PROD;
    gfx_desktop_draw(&g_scene, g_fb, W, H);
    TEST_ASSERT(count_near(40, 70, 400, 110, 28, 44, 56, 40) > 30);
}

static void test_vbe_layout_automation_under_load(void) {
    gfx_desktop_layout_t layout;
    int resolutions[][2] = {
        {640, 480}, {800, 600}, {1024, 768}, {1280, 720}, {1920, 1080}
    };
    int num_res = (int)(sizeof(resolutions) / sizeof(resolutions[0]));
    int i, step;

    blank_scene();
    /* Repeated resolution changes, pane switches and chat window movement simulation */
    for (step = 0; step < 50; step++) {
        int w = resolutions[step % num_res][0];
        int h = resolutions[step % num_res][1];

        g_scene.chat_mode = (step % 2 == 0) ? OS_FB_CHAT_CENTER : OS_FB_CHAT_FLOAT;
        g_scene.pane = (uint8_t)(step % 10);
        g_scene.chat_x = (uint16_t)((step * 3) % 60);
        g_scene.chat_y = (uint16_t)((step * 2) % 15);

        gfx_desktop_get_layout(&g_scene, w, h, &layout);

        /* Invariant checks on calculated layout */
        TEST_ASSERT(layout.bar_h >= 24 && layout.bar_h <= 60);
        TEST_ASSERT_EQUAL(GFX_DESKTOP_MENUS, layout.menu_count);
        TEST_ASSERT(layout.dock.box.w > 0 && layout.dock.box.h > 0);
        TEST_ASSERT(layout.chat_win.box.w > 0 && layout.chat_win.box.h > 0);
        TEST_ASSERT(layout.chat_win.send_btn.w > 0);

        for (i = 0; i < layout.menu_count; i++) {
            TEST_ASSERT(layout.menu_items[i].w > 0);
            TEST_ASSERT(layout.menu_items[i].h > 0);
        }
    }
}

/* Lot 6 (US-031): the web pane paints the engine display list, clipped
 * to the page area, from the origin given by gfx_desktop_web_origin. */
static void test_web_pane_display_list(void) {
    int ox, oy, pw, ph;
    memset(&g_scene, 0, sizeof(g_scene));
    g_scene.magic = OS_FB_MAGIC;
    g_scene.version = 4;
    g_scene.pane = OS_FB_PANE_WEB;
    strcpy(g_scene.web_title, "web demo.html");
    g_scene.nweb = 4;
    g_scene.web[0].kind = OS_FB_WEB_RECT; g_scene.web[0].x = 10; g_scene.web[0].y = 10;
    g_scene.web[0].w = 30; g_scene.web[0].h = 20; g_scene.web[0].color = 0xFF0000;
    g_scene.web[1].kind = OS_FB_WEB_BORDER; g_scene.web[1].x = 100; g_scene.web[1].y = 10;
    g_scene.web[1].w = 40; g_scene.web[1].h = 40; g_scene.web[1].bold = 3; g_scene.web[1].color = 0x0000FF;
    g_scene.web[2].kind = OS_FB_WEB_TEXT; g_scene.web[2].x = 10; g_scene.web[2].y = 60;
    g_scene.web[2].color = 0x00AA00; strcpy(g_scene.web[2].text, "HELLO");
    /* far outside the page: clipped */
    g_scene.web[3].kind = OS_FB_WEB_RECT; g_scene.web[3].x = -500; g_scene.web[3].y = 2000;
    g_scene.web[3].w = 3000; g_scene.web[3].h = 30; g_scene.web[3].color = 0xFF00FF;
    gfx_desktop_draw(&g_scene, g_fb, W, H);
    gfx_desktop_web_origin(W, H, &ox, &oy, &pw, &ph);
    TEST_ASSERT_EQUAL(16, ox);
    TEST_ASSERT_TRUE(oy > 28 && oy < 100);
    TEST_ASSERT_EQUAL(30 * 20, count_near(ox + 10, oy + 10, ox + 40, oy + 30, 255, 0, 0, 0));
    TEST_ASSERT_EQUAL(0, count_near(ox + 41, oy + 10, ox + 60, oy + 30, 255, 0, 0, 0));
    TEST_ASSERT_TRUE(near_rgb(gfx_desktop_pixel(g_fb, W, H, ox + 5, oy + 5), 255, 255, 255, 0));
    /* border: 3 px frame, white inside */
    TEST_ASSERT_EQUAL(40 * 3, count_near(ox + 100, oy + 10, ox + 140, oy + 13, 0, 0, 255, 0));
    TEST_ASSERT_TRUE(near_rgb(gfx_desktop_pixel(g_fb, W, H, ox + 120, oy + 30), 255, 255, 255, 0));
    TEST_ASSERT_TRUE(count_near(ox + 10, oy + 60, ox + 50, oy + 68, 0, 0xAA, 0, 0) > 20);
    TEST_ASSERT_EQUAL(0, count_near(0, 0, W, H, 255, 0, 255, 0));
    /* version 3 scene: the web list is ignored */
    g_scene.version = 3;
    gfx_desktop_draw(&g_scene, g_fb, W, H);
    TEST_ASSERT_EQUAL(0, count_near(ox + 10, oy + 10, ox + 40, oy + 30, 255, 0, 0, 0));
}


/* Lot 72: images of the web pane come from the atlas (SYS_VGA_BLIT 6),
 * one OS_FB_WEB_IMAGE item per image, scaled and clipped; a bad atlas is
 * refused whole; the favicon is drawn before the title. */
static os_fb_webimg_t g_atlas;
static void test_web_pane_atlas_images(void) {
    int ox, oy, pw, ph, k;
    memset(&g_atlas, 0, sizeof(g_atlas));
    g_atlas.magic = OS_FB_WEBIMG_MAGIC;
    /* slot 1: 2x1, red then blue */
    g_atlas.slot[1].w = 2; g_atlas.slot[1].h = 1; g_atlas.slot[1].off = 300;
    g_atlas.rgb[900] = 255; g_atlas.rgb[901] = 0; g_atlas.rgb[902] = 0;
    g_atlas.rgb[903] = 0; g_atlas.rgb[904] = 0; g_atlas.rgb[905] = 255;
    /* slot 0 favicon: 16x16 green */
    g_atlas.slot[0].w = 16; g_atlas.slot[0].h = 16;
    for (k = 0; k < 256; k++) { g_atlas.rgb[k * 3] = 0; g_atlas.rgb[k * 3 + 1] = 200; g_atlas.rgb[k * 3 + 2] = 0; }
    TEST_ASSERT_EQUAL(0, gfx_desktop_set_webimg(&g_atlas));
    memset(&g_scene, 0, sizeof(g_scene));
    g_scene.magic = OS_FB_MAGIC;
    g_scene.version = 4;
    g_scene.pane = OS_FB_PANE_WEB;
    strcpy(g_scene.web_title, "images");
    g_scene.nweb = 2;
    g_scene.web[0].kind = OS_FB_WEB_IMAGE; g_scene.web[0].x = 10; g_scene.web[0].y = 10;
    g_scene.web[0].w = 40; g_scene.web[0].h = 20; g_scene.web[0].pad = 1;
    /* an empty slot paints nothing */
    g_scene.web[1].kind = OS_FB_WEB_IMAGE; g_scene.web[1].x = 100; g_scene.web[1].y = 10;
    g_scene.web[1].w = 40; g_scene.web[1].h = 20; g_scene.web[1].pad = 7;
    gfx_desktop_draw(&g_scene, g_fb, W, H);
    gfx_desktop_web_origin(W, H, &ox, &oy, &pw, &ph);
    TEST_ASSERT_EQUAL(20 * 20, count_near(ox + 10, oy + 10, ox + 30, oy + 30, 255, 0, 0, 0));
    TEST_ASSERT_EQUAL(20 * 20, count_near(ox + 30, oy + 10, ox + 50, oy + 30, 0, 0, 255, 0));
    TEST_ASSERT_EQUAL(0, count_near(ox + 100, oy + 10, ox + 140, oy + 30, 255, 0, 0, 0));
    TEST_ASSERT_TRUE(near_rgb(gfx_desktop_pixel(g_fb, W, H, ox + 120, oy + 20), 255, 255, 255, 0));
    /* favicon: green square in the title bar, above the page */
    TEST_ASSERT_TRUE(count_near(0, 0, W, oy, 0, 200, 0, 0) >= 16 * 16);
    /* a slot outside the pixel array: refused, nothing drawn */
    g_atlas.slot[2].w = 300; g_atlas.slot[2].h = 300; g_atlas.slot[2].off = 0;
    TEST_ASSERT_EQUAL(0, gfx_desktop_webimg_valid(&g_atlas));
    TEST_ASSERT_EQUAL(-1, gfx_desktop_set_webimg(&g_atlas));
    g_atlas.slot[2].w = 1; g_atlas.slot[2].h = 1; g_atlas.slot[2].off = OS_FB_WEBIMG_PIXELS;
    TEST_ASSERT_EQUAL(0, gfx_desktop_webimg_valid(&g_atlas));
    g_atlas.slot[2].off = 0xFFFFFFF0U;
    TEST_ASSERT_EQUAL(0, gfx_desktop_webimg_valid(&g_atlas));
    g_atlas.magic = 0;
    TEST_ASSERT_EQUAL(0, gfx_desktop_webimg_valid(&g_atlas));
    gfx_desktop_draw(&g_scene, g_fb, W, H);
    TEST_ASSERT_EQUAL(0, count_near(ox + 10, oy + 10, ox + 30, oy + 30, 255, 0, 0, 0));
    TEST_ASSERT_EQUAL(0, gfx_desktop_set_webimg(0));
}

/* Window manager on the OS-UI windows (kernel/gfx_wm.c): every pane and
 * the chat have minimize / maximize / close, a title drag, the dock and
 * the menu restore a minimized window, the menu marks the active one. */
static void drag(int x0, int y0, int x1, int y1) {
    gfx_desktop_set_mouse(x0, y0, 0);
    gfx_desktop_set_mouse(x0, y0, 1);
    gfx_desktop_set_mouse(x1, y1, 1);
    gfx_desktop_set_mouse(x1, y1, 0);
}

static void redraw(gfx_desktop_layout_t *l) {
    gfx_desktop_draw_no_cursor(&g_scene, g_fb, W, H);
    gfx_desktop_get_layout(&g_scene, W, H, l);
}

static void test_window_manager_panes_and_chat(void) {
    static const uint8_t panes[6] = { OS_FB_PANE_BROWSER, OS_FB_PANE_ADMIN, OS_FB_PANE_SUPPORT,
                                      OS_FB_PANE_STATUS, OS_FB_PANE_FS, OS_FB_PANE_SHELL };
    gfx_desktop_layout_t l, m;
    char got[64];
    int k;
    vga_desktop_set(1);
    for (k = 0; k < 6; k++) {
        int idx;
        gfx_desktop_wm_reset();
        blank_scene();
        g_scene.chat_mode = OS_FB_CHAT_FLOAT;
        g_scene.pane = panes[k];
        redraw(&l);
        TEST_ASSERT(l.pane_win.active);
        idx = l.menu_active;
        TEST_ASSERT(idx >= 1 && idx <= 6);
        TEST_ASSERT_EQUAL(1, l.dock_state[idx]);
        /* the three buttons are drawn in their colours */
        TEST_ASSERT_EQUAL(GFX_WM_RGB_CLOSE, g_fb[(l.pane_win.btn[2].y + 2) * W + l.pane_win.btn[2].x + 7]);
        TEST_ASSERT_EQUAL(GFX_WM_RGB_MIN, g_fb[(l.pane_win.btn[0].y + 2) * W + l.pane_win.btn[0].x + 7]);
        /* maximize: the pane takes the work area, again: back */
        drain_kbd();
        click_at(rect_center_x(&l.pane_win.btn[1]), rect_center_y(&l.pane_win.btn[1]));
        redraw(&m);
        TEST_ASSERT(m.pane_win.maximized);
        TEST_ASSERT_EQUAL(0, m.pane_win.box.x);
        TEST_ASSERT_EQUAL(W, m.pane_win.box.w);
        click_at(rect_center_x(&m.pane_win.btn[1]), rect_center_y(&m.pane_win.btn[1]));
        redraw(&m);
        TEST_ASSERT(!m.pane_win.maximized);
        TEST_ASSERT_EQUAL(l.pane_win.box.x, m.pane_win.box.x);
        /* drag the title bar */
        drag(l.pane_win.box.x + 60, l.pane_win.box.y + 10, l.pane_win.box.x + 100, l.pane_win.box.y + 50);
        redraw(&m);
        TEST_ASSERT_EQUAL(l.pane_win.box.x + 40, m.pane_win.box.x);
        TEST_ASSERT_EQUAL(l.pane_win.box.y + 40, m.pane_win.box.y);
        /* minimize: hidden, amber dock mark, no command sent */
        click_at(rect_center_x(&m.pane_win.btn[0]), rect_center_y(&m.pane_win.btn[0]));
        redraw(&m);
        TEST_ASSERT(!m.pane_win.active);
        TEST_ASSERT(m.pane_win.minimized);
        TEST_ASSERT_EQUAL(2, m.dock_state[idx]);
        read_kbd(got, (int)sizeof(got));
        TEST_ASSERT_EQUAL_STRING("", got);
        /* the dock icon restores it in place, still without a command */
        click_at(rect_center_x(&m.dock.icons[idx]), rect_center_y(&m.dock.icons[idx]));
        redraw(&m);
        TEST_ASSERT(m.pane_win.active);
        TEST_ASSERT_EQUAL(l.pane_win.box.x + 40, m.pane_win.box.x);
        read_kbd(got, (int)sizeof(got));
        TEST_ASSERT_EQUAL_STRING("", got);
        /* minimize again, the menu item restores too */
        click_at(rect_center_x(&m.pane_win.btn[0]), rect_center_y(&m.pane_win.btn[0]));
        redraw(&m);
        TEST_ASSERT(!m.pane_win.active);
        click_at(rect_center_x(&m.menu_items[idx]), rect_center_y(&m.menu_items[idx]));
        redraw(&m);
        TEST_ASSERT(m.pane_win.active);
        /* close sends the command that closes the pane */
        click_at(rect_center_x(&m.pane_win.btn[2]), rect_center_y(&m.pane_win.btn[2]));
        read_kbd(got, (int)sizeof(got));
        /* Lot 69: an event for the desktop, typed text for the shell terminal */
        TEST_ASSERT_EQUAL_STRING(panes[k] == OS_FB_PANE_SHELL ? "/center\n" : "\x16" "cmd /center\n", got);
    }
    /* chat: close hides it, the Chat menu reopens it without a command */
    gfx_desktop_wm_reset();
    blank_scene();
    redraw(&l);
    TEST_ASSERT(l.chat_win.visible);
    TEST_ASSERT_EQUAL(0, l.menu_active);
    TEST_ASSERT_EQUAL(GFX_WM_RGB_CLOSE, g_fb[(l.chat_win.btn[2].y + 2) * W + l.chat_win.btn[2].x + 7]);
    drain_kbd();
    click_at(rect_center_x(&l.chat_win.btn[2]), rect_center_y(&l.chat_win.btn[2]));
    redraw(&m);
    TEST_ASSERT(!m.chat_win.visible);
    TEST_ASSERT_EQUAL(0, m.dock_state[0]);
    TEST_ASSERT_EQUAL(-1, m.menu_active);
    TEST_ASSERT_EQUAL(0, m.chat_win.send_btn.w);   /* hidden chat takes no click */
    click_at(rect_center_x(&l.chat_win.send_btn), rect_center_y(&l.chat_win.send_btn));
    click_at(rect_center_x(&m.menu_items[0]), rect_center_y(&m.menu_items[0]));
    redraw(&m);
    TEST_ASSERT(m.chat_win.visible);
    read_kbd(got, (int)sizeof(got));
    TEST_ASSERT_EQUAL_STRING("", got);
    /* chat minimize: amber dock mark, the dock Chat icon restores */
    click_at(rect_center_x(&m.chat_win.btn[0]), rect_center_y(&m.chat_win.btn[0]));
    redraw(&m);
    TEST_ASSERT(!m.chat_win.visible);
    TEST_ASSERT_EQUAL(2, m.dock_state[0]);
    click_at(rect_center_x(&m.dock.icons[0]), rect_center_y(&m.dock.icons[0]));
    redraw(&m);
    TEST_ASSERT(m.chat_win.visible);
    /* chat maximize and drag */
    click_at(rect_center_x(&m.chat_win.btn[1]), rect_center_y(&m.chat_win.btn[1]));
    redraw(&m);
    TEST_ASSERT(m.chat_win.maximized);
    TEST_ASSERT_EQUAL(W, m.chat_win.box.w);
    click_at(rect_center_x(&m.chat_win.btn[1]), rect_center_y(&m.chat_win.btn[1]));
    redraw(&m);
    drag(m.chat_win.box.x + 40, m.chat_win.box.y + 12, m.chat_win.box.x + 10, m.chat_win.box.y + 62);
    redraw(&l);
    TEST_ASSERT_EQUAL(m.chat_win.box.x - 30, l.chat_win.box.x);
    TEST_ASSERT_EQUAL(m.chat_win.box.y + 50, l.chat_win.box.y);
    /* the send button follows the moved chat */
    drain_kbd();
    click_at(rect_center_x(&l.chat_win.send_btn), rect_center_y(&l.chat_win.send_btn));
    read_kbd(got, (int)sizeof(got));
    TEST_ASSERT_EQUAL_STRING("\n", got);
    gfx_desktop_wm_reset();
    vga_desktop_set(0);
}

/* UX3: explorer / editor app windows from the scene (v7): z-order, focus,
 * toolbar / grid / close events on the OS_FB_EVT channel, dock tooltip. */
static void app_scene(void) {
    int i;
    blank_scene();
    g_scene.version = 7;
    g_scene.chat_mode = OS_FB_CHAT_FLOAT;
    g_scene.app_focus = 0xFF;
    g_scene.app[0].kind = OS_FB_APP_FILES;
    g_scene.app[0].nrows = 4;
    strcpy(g_scene.app[0].title, "Fichiers - /home");
    strcpy(g_scene.app[0].tool, "[Haut] [Ouvrir]");
    for (i = 0; i < 4; i++) strcpy(g_scene.app[0].rows[i], "entry");
    strcpy(g_scene.app[0].status, "4 elements");
}

static void test_app_windows_events_and_z_order(void) {
    gfx_desktop_layout_t l, m;
    char got[96];
    int i, gx, gy;
    vga_desktop_set(1);
    gfx_desktop_wm_reset();
    app_scene();
    redraw(&l);
    /* a newly opened app comes to the front, above the chat */
    TEST_ASSERT(l.app_win[0].visible);
    TEST_ASSERT(!l.app_win[1].visible);
    TEST_ASSERT_EQUAL(GFX_WIN_APP0, l.z[GFX_WIN_COUNT - 1]);
    TEST_ASSERT_EQUAL(1, l.dock_state[6]);
    TEST_ASSERT_EQUAL(6, l.menu_active);
    /* the window is painted (title band) */
    TEST_ASSERT(count_near(l.app_win[0].box.x + 4, l.app_win[0].box.y + 4,
                           l.app_win[0].box.x + 200, l.app_win[0].box.y + 20, 24, 34, 44, 6) > 500);

    /* toolbar click: focus event then "app 0 tool C" */
    drain_kbd();
    click_at(l.app_win[0].tool.x + 8 * 9 + 2, l.app_win[0].tool.y + 6);
    read_kbd(got, (int)sizeof(got));
    TEST_ASSERT_EQUAL_STRING("\x16" "focus 0\n" "\x16" "app 0 tool 9\n", got);

    /* the scene reports the focus; a grid click is "click R C", a second
     * one on the same row is a double click */
    g_scene.app_focus = 0;
    redraw(&l);
    gx = l.app_win[0].grid.x + 3 * 8 + 2;
    gy = l.app_win[0].grid.y + 2 * GFX_APP_ROW_H + 3;
    drain_kbd();
    click_at(gx, gy);
    read_kbd(got, (int)sizeof(got));
    TEST_ASSERT_EQUAL_STRING("\x16" "app 0 click 2 3\n", got);
    click_at(gx, gy);
    read_kbd(got, (int)sizeof(got));
    TEST_ASSERT_EQUAL_STRING("\x16" "app 0 dclick 2 3\n", got);
    /* drag inside the grid: a selection */
    drain_kbd();
    gfx_desktop_set_mouse(gx, gy, 0);
    gfx_desktop_set_mouse(gx, gy, 1);
    gfx_desktop_set_mouse(gx + 16, gy + GFX_APP_ROW_H, 1);
    gfx_desktop_set_mouse(gx + 16, gy + GFX_APP_ROW_H, 0);
    read_kbd(got, (int)sizeof(got));
    TEST_ASSERT(strstr(got, "app 0 drag 2 3 3 5\n") != 0);

    /* a second app opens above the first; clicking the first raises it */
    g_scene.app[1].kind = OS_FB_APP_EDIT;
    g_scene.app[1].nrows = 1;
    strcpy(g_scene.app[1].title, "Editeur - /home/a.txt");
    g_scene.app[1].dirty = 1;
    g_scene.app_focus = 1;
    redraw(&l);
    TEST_ASSERT(l.app_win[1].visible);
    TEST_ASSERT_EQUAL(GFX_WIN_APP1, l.z[GFX_WIN_COUNT - 1]);
    TEST_ASSERT_EQUAL(GFX_WIN_APP1, l.focus);
    /* UX4: windows are opaque, nothing behind bleeds through the frame */
    TEST_ASSERT_EQUAL(0x0E141Bu, g_fb[(l.app_win[1].box.y + GFX_APP_GRID_Y + 40) * W + l.app_win[1].box.x + 4] & 0xFFFFFFu);
    TEST_ASSERT_EQUAL(7, l.menu_active);
    drain_kbd();
    /* a point of app 0 not covered by app 1 */
    click_at(l.app_win[0].box.x + 10, l.app_win[0].box.y + l.app_win[0].box.h - 8);
    read_kbd(got, (int)sizeof(got));
    TEST_ASSERT_EQUAL_STRING("\x16" "focus 0\n", got);
    gfx_desktop_get_layout(&g_scene, W, H, &m);
    TEST_ASSERT_EQUAL(GFX_WIN_APP0, m.z[GFX_WIN_COUNT - 1]);

    /* close button: "app 0 close" */
    drain_kbd();
    click_at(rect_center_x(&m.app_win[0].btn[2]), rect_center_y(&m.app_win[0].btn[2]));
    read_kbd(got, (int)sizeof(got));
    TEST_ASSERT_EQUAL_STRING("\x16" "app 0 close\n", got);
    /* minimize is local: hidden, amber dock mark, no event */
    g_scene.app_focus = 1;
    redraw(&l);
    drain_kbd();
    click_at(rect_center_x(&l.app_win[1].btn[0]), rect_center_y(&l.app_win[1].btn[0]));
    redraw(&m);
    TEST_ASSERT(!m.app_win[1].visible);
    TEST_ASSERT_EQUAL(2, m.dock_state[7]);
    read_kbd(got, (int)sizeof(got));
    TEST_ASSERT(strstr(got, "close") == 0);
    /* the Editeur dock icon restores it without a command */
    drain_kbd();
    click_at(rect_center_x(&m.dock.icons[7]), rect_center_y(&m.dock.icons[7]));
    redraw(&l);
    TEST_ASSERT(l.app_win[1].visible);
    read_kbd(got, (int)sizeof(got));
    TEST_ASSERT(strstr(got, "/edit") == 0);

    /* desktop commands from the dock while an app has the focus: the
     * desktop takes the keyboard back first */
    g_scene.app_focus = 0;
    g_scene.app[1].kind = 0;
    redraw(&l);
    drain_kbd();
    click_at(rect_center_x(&l.dock.icons[5]), rect_center_y(&l.dock.icons[5]));
    read_kbd(got, (int)sizeof(got));
    TEST_ASSERT_EQUAL_STRING("\x16" "focus d\n" "\x16" "cmd /status\n", got);

    /* dock hover: tooltip with the name, pictograms (no letters) */
    for (i = 0; i < GFX_DESKTOP_DOCK; i++) TEST_ASSERT(gfx_desktop_dock_names[i][0] != 0);
    gfx_desktop_set_mouse(rect_center_x(&l.dock.icons[6]), rect_center_y(&l.dock.icons[6]), 0);
    redraw(&l);
    TEST_ASSERT_EQUAL(6, l.hover_dock);
    TEST_ASSERT(count_near(l.dock.icons[6].x - 30, l.dock.icons[6].y - 32,
                           l.dock.icons[6].x + 66, l.dock.icons[6].y - 8, 232, 238, 244, 4) > 200);
    gfx_desktop_set_mouse(10, H / 2, 0);
    redraw(&l);
    TEST_ASSERT_EQUAL(-1, l.hover_dock);
    vga_desktop_set(0);
}


/* UX4: generic slots - six windows of any kind, cascaded, raised on click,
 * events carry the slot index. */
static void test_generic_app_slots_z_order(void) {
    gfx_desktop_layout_t l;
    char got[64];
    int i;
    static const int kinds[OS_FB_APPS] = {OS_FB_APP_FILES, OS_FB_APP_TASKS, OS_FB_APP_VIEW,
                                          OS_FB_APP_SETTINGS, OS_FB_APP_P2P, OS_FB_APP_PMRUN};
    vga_desktop_set(1);
    gfx_desktop_wm_reset();
    app_scene();
    for (i = 0; i < OS_FB_APPS; i++) {
        g_scene.app[i].kind = (uint8_t)kinds[i];
        g_scene.app[i].nrows = 2;
        strcpy(g_scene.app[i].title, "App");
        strcpy(g_scene.app[i].rows[0], "row");
        g_scene.app_focus = (uint8_t)i;
        redraw(&l);
        TEST_ASSERT(l.app_win[i].visible);
        TEST_ASSERT_EQUAL(GFX_WIN_APP0 + i, l.z[GFX_WIN_COUNT - 1]);
        if (i > 0) TEST_ASSERT(l.app_win[i].box.x > l.app_win[i - 1].box.x || l.app_win[i].box.y > l.app_win[i - 1].box.y);
    }
    /* an app kind without a dock entry clears the menu highlight */
    TEST_ASSERT_EQUAL(-1, l.menu_active);
    /* click a visible part of slot 2: focus event with its index */
    drain_kbd();
    click_at(l.app_win[2].box.x + 10, l.app_win[2].box.y + 30);
    read_kbd(got, (int)sizeof(got));
    TEST_ASSERT_EQUAL_STRING("\x16" "focus 2\n", got);
    redraw(&l);
    TEST_ASSERT_EQUAL(GFX_WIN_APP0 + 2, l.z[GFX_WIN_COUNT - 1]);
    for (i = 0; i < OS_FB_APPS; i++) g_scene.app[i].kind = 0;
    vga_desktop_set(0);
}

/* UX4: one clipboard for the terminal, the editor and the explorer */
static void test_shared_clipboard(void) {
    static char big[OS_CLIP_MAX + 100];
    char out[16];
    unsigned seq = clipboard_seq();
    TEST_ASSERT_EQUAL(3, clipboard_set("abc", 3));
    TEST_ASSERT_EQUAL(seq + 1, clipboard_seq());
    TEST_ASSERT_EQUAL(3, clipboard_get(out, (int)sizeof(out)));
    TEST_ASSERT_EQUAL_STRING("abc", out);
    memset(big, 'x', sizeof(big));
    TEST_ASSERT_EQUAL(OS_CLIP_MAX, clipboard_set(big, (int)sizeof(big)));
    TEST_ASSERT_EQUAL(OS_CLIP_MAX, clipboard_len());
    TEST_ASSERT_EQUAL(15, clipboard_get(out, (int)sizeof(out)));   /* bounded by the caller */
    {
        /* Lot 45: multi-line text well beyond the old 4 KiB bound */
        static char ml[20000], back[20001];
        int i;
        TEST_ASSERT(OS_CLIP_MAX >= 65536);
        for (i = 0; i < (int)sizeof(ml); i++) ml[i] = (i % 61 == 60) ? '\n' : (char)('a' + i % 26);
        TEST_ASSERT_EQUAL(20000, clipboard_set(ml, 20000));
        TEST_ASSERT_EQUAL(20000, clipboard_get(back, (int)sizeof(back)));
        TEST_ASSERT_EQUAL(0, memcmp(ml, back, 20000));
    }
}

/* UX4: web pane navigation buttons and page clicks go to Ring 3 */
static void test_web_nav_events(void) {
    gfx_desktop_layout_t l;
    gfx_rect_t nav;
    char got[64];
    int ox, oy, pw, ph;
    vga_desktop_set(1);
    gfx_desktop_wm_reset();
    blank_scene();
    g_scene.version = 7;
    g_scene.pane = OS_FB_PANE_WEB;
    g_scene.app_focus = 0xFF;
    g_scene.web_scroll = 30;
    redraw(&l);
    gfx_desktop_web_nav(W, H, 0, &nav);
    drain_kbd();
    click_at(nav.x + 4, nav.y + 4);
    read_kbd(got, (int)sizeof(got));
    TEST_ASSERT_EQUAL_STRING("\x16" "web back\n", got);
    gfx_desktop_web_nav(W, H, 1, &nav);
    click_at(nav.x + 4, nav.y + 4);
    read_kbd(got, (int)sizeof(got));
    TEST_ASSERT_EQUAL_STRING("\x16" "web fwd\n", got);
    gfx_desktop_web_origin(W, H, &ox, &oy, &pw, &ph);
    click_at(ox + 10, oy + 5);
    read_kbd(got, (int)sizeof(got));
    TEST_ASSERT_EQUAL_STRING("\x16" "web click 10 35\n", got);   /* page coordinates: scroll added */
    vga_desktop_set(0);
}


/* UX5: the app grid follows the window: default 22 x 77, a maximized
 * window sends "app K size R C" with the larger grid, restore sends back. */
static void test_app_grid_fits_window(void) {
    gfx_desktop_layout_t l, m;
    char got[96];
    int r = 0, c = 0, mr = 0, mc = 0;
    vga_desktop_set(1);
    gfx_desktop_wm_reset();
    blank_scene();
    g_scene.version = 7;
    g_scene.app[1].kind = OS_FB_APP_EDIT;
    g_scene.app[1].nrows = 1;
    drain_kbd();
    redraw(&l);
    TEST_ASSERT(l.app_win[1].visible);
    gfx_desktop_app_grid_fit(&l.app_win[1].box, &r, &c);
    TEST_ASSERT_EQUAL(OS_FB_APP_ROWS, r);
    TEST_ASSERT_EQUAL(OS_FB_APP_COLS - 1, c);
    read_kbd(got, (int)sizeof(got));
    TEST_ASSERT_EQUAL_STRING("", got);               /* default grid: no event */
    click_at(rect_center_x(&l.app_win[1].btn[1]), rect_center_y(&l.app_win[1].btn[1]));
    drain_kbd();
    redraw(&m);
    TEST_ASSERT(m.app_win[1].maximized);
    gfx_desktop_app_grid_fit(&m.app_win[1].box, &mr, &mc);
    TEST_ASSERT(mr > r);
    TEST_ASSERT(mc > c);
    TEST_ASSERT(mr <= OS_FB_APP_MAX_ROWS && mc <= OS_FB_APP_MAX_COLS - 1);
    TEST_ASSERT_EQUAL(mr * GFX_APP_ROW_H, m.app_win[1].grid.h);
    {
        char want[64], back[64];
        snprintf(want, sizeof(want), "\x16" "app 1 size %d %d\n", mr, mc);
        snprintf(back, sizeof(back), "\x16" "app 1 size %d %d\n", OS_FB_APP_ROWS, OS_FB_APP_COLS - 1);
        /* the maximize click redraws: the event is already in the stream */
        gfx_desktop_wm_reset();
        redraw(&l);
        drain_kbd();
        click_at(rect_center_x(&l.app_win[1].btn[1]), rect_center_y(&l.app_win[1].btn[1]));
        gfx_desktop_draw_no_cursor(&g_scene, g_fb, W, H);
        read_kbd(got, (int)sizeof(got));
        TEST_ASSERT_NOT_NULL(strstr(got, want));
        gfx_desktop_draw_no_cursor(&g_scene, g_fb, W, H);   /* same size: no new event */
        read_kbd(got, (int)sizeof(got));
        TEST_ASSERT_EQUAL_STRING("", got);
        gfx_desktop_get_layout(&g_scene, W, H, &m);
        click_at(rect_center_x(&m.app_win[1].btn[1]), rect_center_y(&m.app_win[1].btn[1]));
        gfx_desktop_draw_no_cursor(&g_scene, g_fb, W, H);
        read_kbd(got, (int)sizeof(got));
        TEST_ASSERT_NOT_NULL(strstr(got, back));
    }
    g_scene.app[1].kind = 0;
    vga_desktop_set(0);
}

/* UX5: the viewer image is drawn in pixels inside its window, scaled up,
 * only for the viewer slot; a bad image is refused and clears. */
static void test_viewer_image_pixels(void) {
    static os_fb_image_t img;
    gfx_desktop_layout_t l;
    gfx_rect_t r;
    int x, y;
    vga_desktop_set(1);
    gfx_desktop_wm_reset();
    blank_scene();
    g_scene.version = 7;
    g_scene.app[0].kind = OS_FB_APP_VIEW;
    g_scene.app[0].nrows = 1;
    memset(&img, 0, sizeof(img));
    img.magic = OS_FB_IMG_MAGIC; img.slot = 0; img.w = 2; img.h = 1;
    img.rgb[0] = 255; img.rgb[1] = 0; img.rgb[2] = 0;       /* red | blue */
    img.rgb[3] = 0; img.rgb[4] = 0; img.rgb[5] = 255;
    TEST_ASSERT_EQUAL(0, gfx_desktop_set_image(&img));
    redraw(&l);
    gfx_desktop_image_rect(&l.app_win[0].box, &r);
    TEST_ASSERT(r.w >= 16 && r.h >= 8);                     /* scaled up */
    TEST_ASSERT(r.x >= l.app_win[0].box.x && r.x + r.w <= l.app_win[0].box.x + l.app_win[0].box.w);
    x = r.x + r.w / 4; y = r.y + r.h / 2;
    TEST_ASSERT_EQUAL(0xFF0000, (int)(g_fb[y * W + x] & 0xFFFFFFu));
    x = r.x + (3 * r.w) / 4;
    TEST_ASSERT_EQUAL(0x0000FF, (int)(g_fb[y * W + x] & 0xFFFFFFu));
    /* another app kind in that slot: no image drawn */
    g_scene.app[0].kind = OS_FB_APP_TASKS;
    redraw(&l);
    TEST_ASSERT(g_fb[y * W + x] != 0x0000FFu);
    /* bad image refused, image cleared */
    img.w = OS_FB_IMG_MAX_W + 1;
    TEST_ASSERT_EQUAL(-1, gfx_desktop_set_image(&img));
    gfx_desktop_image_rect(&l.app_win[0].box, &r);
    TEST_ASSERT_EQUAL(0, r.w);
    TEST_ASSERT_EQUAL(0, gfx_desktop_set_image(0));
    g_scene.app[0].kind = 0;
    vga_desktop_set(0);
}

static void test_llm_friendly_names(void) {
    TEST_ASSERT_EQUAL_STRING("GPT-2 local", gfx_desktop_llm_name("gpt2_local"));
    TEST_ASSERT_EQUAL_STRING("GPT-2 GGUF local", gfx_desktop_llm_name("gpt2_gguf_local"));
    TEST_ASSERT_EQUAL_STRING("echo (sans modele)", gfx_desktop_llm_name("stub_echo"));
    TEST_ASSERT_EQUAL_STRING("echo (sans modele)", gfx_desktop_llm_name(""));
    TEST_ASSERT_EQUAL_STRING("other", gfx_desktop_llm_name("other"));
}


/* Lot 68: scaled glyphs keep the 1x shape (EPX smoothing only touches
 * edge pixels), metrics follow the scale, and the scene carries it. */
static int font_bit(char ch, int x, int y) { return gfx_glyph_pixel(ch, 4, x, y); }
static void test_lot68_scaled_glyphs(void) {
    static const int qs[] = {8, 12, 16};
    int c, k, x, y, on1, onq;
    gfx_desktop_set_font_mode(1);   /* Lot 69: the smoothed upscale is mode 1 */
    for (k = 0; k < 3; k++) {
        int q = qs[k], f = q / 4;
        TEST_ASSERT_EQUAL(0, gfx_glyph_pixel('A', q, -1, 0));
        TEST_ASSERT_EQUAL(0, gfx_glyph_pixel('A', q, 8 * f, 0));
        for (c = 33; c < 127; c++) {
            on1 = 0; onq = 0;
            for (y = 0; y < 8; y++) for (x = 0; x < 8; x++) {
                int all = 1, none = 1, dx, dy, sx, sy;
                on1 += font_bit((char)c, x, y);
                for (dy = -1; dy <= 1; dy++) for (dx = -1; dx <= 1; dx++) {
                    int b = font_bit((char)c, x + dx, y + dy);
                    if (x + dx < 0 || x + dx > 7 || y + dy < 0 || y + dy > 7) continue;
                    if (b) none = 0; else all = 0;
                }
                for (sy = 0; sy < f; sy++) for (sx = 0; sx < f; sx++) {
                    int p = gfx_glyph_pixel((char)c, q, x * f + sx, y * f + sy);
                    onq += p;
                    if (all) TEST_ASSERT_EQUAL(1, p);
                    if (none) TEST_ASSERT_EQUAL(0, p);
                }
            }
            /* ink grows with the area, within EPX corner changes */
            TEST_ASSERT_TRUE(onq * 2 >= on1 * f * f);
            TEST_ASSERT_TRUE(onq <= on1 * f * f * 2);
        }
    }
    /* 1.5x: nearest neighbour, 12 px cell */
    TEST_ASSERT_EQUAL(font_bit('H', 0, 0), gfx_glyph_pixel('H', 6, 0, 0));
    TEST_ASSERT_EQUAL(0, gfx_glyph_pixel('H', 6, 12, 0));
    gfx_desktop_set_font_mode(0);
}

static void test_lot68_scene_scale_and_grid(void) {
    static os_fb_scene_t sc;
    gfx_rect_t box;
    int r1, c1, r2, c2;
    memset(&sc, 0, sizeof(sc));
    sc.magic = OS_FB_MAGIC; sc.version = 7;
    TEST_ASSERT_EQUAL(4, gfx_desktop_surface_q(&sc, OS_UI_Z_CHAT, 3840, 2160)); /* old scene: 1x */
    sc.version = 8;
    TEST_ASSERT_EQUAL(6, gfx_desktop_surface_q(&sc, OS_UI_Z_CHAT, 1024, 768));  /* Lot 69: auto 1.5x */
    TEST_ASSERT_EQUAL(4, gfx_desktop_surface_q(&sc, OS_UI_Z_CHAT, 800, 600));
    TEST_ASSERT_EQUAL(12, gfx_desktop_surface_q(&sc, OS_UI_Z_CHAT, 3840, 2160)); /* auto */
    sc.ui_scale_q = 8;
    sc.zoom_q[OS_UI_Z_WEB] = 6;
    TEST_ASSERT_EQUAL(8, gfx_desktop_surface_q(&sc, OS_UI_Z_CHAT, 1024, 768));
    TEST_ASSERT_EQUAL(12, gfx_desktop_surface_q(&sc, OS_UI_Z_WEB, 1024, 768));
    sc.ui_scale_q = 12;
    TEST_ASSERT_EQUAL(8, gfx_desktop_chrome_q(&sc, 1024, 768)); /* chrome capped at 2x */
    gfx_desktop_set_text_q(8);
    TEST_ASSERT_EQUAL(8, gfx_desktop_text_q());
    gfx_desktop_set_text_q(4);
    box.x = 0; box.y = 0; box.w = 600; box.h = 400;
    gfx_desktop_app_grid_fit_q(&box, 4, &r1, &c1);
    gfx_desktop_app_grid_fit_q(&box, 8, &r2, &c2);
    TEST_ASSERT_TRUE(r1 > 0 && c1 > 0);
    TEST_ASSERT_TRUE(r2 * 2 <= r1 + 2);
    TEST_ASSERT_TRUE(c2 * 2 <= c1 + 2);
}


/* Lot 69: native fonts above 1x (gfx_font_hd.h): every glyph has ink
 * inside its cell, the space is empty, shapes differ, descenders go
 * lower than x-height letters, and in-between scales sample a source. */
static int ink(char c, int q, int *ymax) {
    int n = os_ui_px(8, q), x, y, k = 0;
    *ymax = -1;
    for (y = 0; y < n; y++) for (x = 0; x < n; x++) if (gfx_glyph_pixel(c, q, x, y)) { k++; *ymax = y; }
    return k;
}
static void test_lot69_native_fonts(void) {
    static const int qs[] = {5, 6, 7, 8, 10, 12, 14, 16};
    int k, c, ya, yg, d, x, y;
    TEST_ASSERT_EQUAL(0, gfx_desktop_font_mode());
    TEST_ASSERT_EQUAL(8, gfx_glyph_source_cell(4));
    TEST_ASSERT_EQUAL(12, gfx_glyph_source_cell(6));
    TEST_ASSERT_EQUAL(16, gfx_glyph_source_cell(7));
    TEST_ASSERT_EQUAL(24, gfx_glyph_source_cell(12));
    TEST_ASSERT_EQUAL(32, gfx_glyph_source_cell(14));
    for (k = 0; k < 8; k++) {
        int q = qs[k], n = os_ui_px(8, q);
        TEST_ASSERT_EQUAL(0, ink(' ', q, &ya));
        for (c = 33; c < 127; c++) TEST_ASSERT_TRUE(ink((char)c, q, &ya) > 0);
        TEST_ASSERT_EQUAL(0, gfx_glyph_pixel('A', q, n, 0));
        TEST_ASSERT_EQUAL(0, gfx_glyph_pixel('A', q, 0, n));
        (void)ink('a', q, &ya); (void)ink('g', q, &yg);
        TEST_ASSERT_TRUE(yg > ya);                       /* descender */
        for (d = 0, y = 0; y < n; y++) for (x = 0; x < n; x++)
            d += gfx_glyph_pixel('O', q, x, y) != gfx_glyph_pixel('Q', q, x, y);
        TEST_ASSERT_TRUE(d > 0);
        /* ink scales with the cell area (native, not a fixed bitmap) */
        TEST_ASSERT_TRUE(ink('M', q, &ya) * 64 >= ink('M', 4, &yg) * n * n / 4);
    }
}


/* Lot 69: status row and icon labels grow with ui-scale */
static void test_lot69_status_row_scales(void) {
    int s1, s2, i1, i2;
    blank_scene();
    g_scene.version = 8;
    g_scene.ui_scale_q = 4;
    gfx_desktop_draw_no_cursor(&g_scene, g_fb, W, H);
    s1 = count_near(12, 0, 300, 90, 122, 212, 196, 6);
    i1 = count_near(W - 120, 40, W, H - 60, 232, 238, 244, 6);
    g_scene.ui_scale_q = 8;
    gfx_desktop_draw_no_cursor(&g_scene, g_fb, W, H);
    s2 = count_near(12, 0, 300, 90, 122, 212, 196, 6);
    i2 = count_near(W - 120, 40, W, H - 60, 232, 238, 244, 6);
    TEST_ASSERT_TRUE(s1 > 10);
    TEST_ASSERT_TRUE(s2 > s1 * 2);
    TEST_ASSERT_TRUE(i2 > i1);
}


/* Lot 69: a web TEXT item with pad = 8 (CSS 32 px) paints its glyphs at
 * twice the cell: more ink and about twice the width of pad 0. */
static int web_text_ink(int pad, int *maxx) {
    int ox, oy, pw, ph, x, y, n = 0;
    blank_scene();
    g_scene.version = 8;
    g_scene.ui_scale_q = 4;
    g_scene.pane = OS_FB_PANE_WEB;
    strcpy(g_scene.web_title, "font");
    g_scene.nweb = 1;
    g_scene.web[0].kind = OS_FB_WEB_TEXT; g_scene.web[0].x = 10; g_scene.web[0].y = 20;
    g_scene.web[0].w = (uint16_t)(5 * (pad ? 2 * pad : 8)); g_scene.web[0].h = (uint16_t)(pad ? 2 * pad : 8);
    g_scene.web[0].pad = (uint16_t)pad;
    g_scene.web[0].color = 0x00AA00; strcpy(g_scene.web[0].text, "HELLO");
    gfx_desktop_draw_no_cursor(&g_scene, g_fb, W, H);
    gfx_desktop_web_origin(W, H, &ox, &oy, &pw, &ph);
    *maxx = 0;
    for (y = oy; y < oy + 80 && y < oy + ph; y++)
        for (x = ox; x < ox + 200 && x < ox + pw; x++)
            if (near_rgb(gfx_desktop_pixel(g_fb, W, H, x, y), 0x00, 0xAA, 0x00, 8)) { n++; if (x - ox > *maxx) *maxx = x - ox; }
    return n;
}

static void test_lot69_web_item_font_scale(void) {
    int m1, m2, m3;
    int a = web_text_ink(0, &m1), b = web_text_ink(8, &m2), c = web_text_ink(99, &m3);
    TEST_ASSERT_TRUE(a > 20);
    TEST_ASSERT_TRUE(b > a * 3 / 2);   /* native 16 px strokes are thinner than doubled 8x8 ones */
    TEST_ASSERT_TRUE(m2 > m1 + 30);
    TEST_ASSERT_EQUAL(a, c);   /* out-of-range pad falls back to the 8x8 cell */
}


/* Lot 69: the dock tooltip grows with ui-scale and its text stays inside */
static int tooltip_rows(int q) {
    gfx_desktop_layout_t l;
    int y, rows = 0, cx;
    blank_scene();
    g_scene.version = 8;
    g_scene.ui_scale_q = (uint8_t)q;
    redraw(&l);
    cx = rect_center_x(&l.dock.icons[2]);
    gfx_desktop_set_mouse(cx, rect_center_y(&l.dock.icons[2]), 0);
    redraw(&l);
    if (l.hover_dock != 2) return -1;
    for (y = l.dock.icons[2].y - 80; y < l.dock.icons[2].y; y++)
        if (count_near(cx - 60, y, cx + 60, y + 1, 232, 238, 244, 4) > 30) rows++;
    gfx_desktop_set_mouse(10, H / 2, 0);
    return rows;
}

static void test_lot69_dock_tooltip_scales(void) {
    int r1, r2;
    vga_desktop_set(1);
    r1 = tooltip_rows(4);
    r2 = tooltip_rows(8);
    vga_desktop_set(0);
    TEST_ASSERT_TRUE(r1 >= 14);
    TEST_ASSERT_TRUE(r2 >= r1 + 6);
}


/* Lot 69 (US-080): dock on the left, compact menus, full screen chat */
static void test_lot69_form_layout(void) {
    gfx_desktop_layout_t l, m;
    int i, ink;
    blank_scene();
    g_scene.version = 8;
    g_scene.ui_scale_q = 4;
    redraw(&l);
    TEST_ASSERT_TRUE(l.dock.box.y > H / 2);
    TEST_ASSERT_EQUAL(0, gfx_desktop_dock_left());
    g_scene.scale_pad[1] = 1 | 2;
    redraw(&m);
    TEST_ASSERT_EQUAL(1, gfx_desktop_dock_left());
    TEST_ASSERT_EQUAL(1, gfx_desktop_menu_compact());
    TEST_ASSERT_EQUAL(8, m.dock.box.x);
    TEST_ASSERT_TRUE(m.dock.box.h > m.dock.box.w);
    for (i = 1; i < GFX_DESKTOP_DOCK; i++) {
        TEST_ASSERT_EQUAL(m.dock.icons[0].x, m.dock.icons[i].x);
        TEST_ASSERT_EQUAL(m.dock.icons[i - 1].y + 44, m.dock.icons[i].y);
    }
    /* pixels follow the hit boxes: the dock tile colour inside icon 3 */
    ink = count_near(m.dock.icons[3].x, m.dock.icons[3].y, m.dock.icons[3].x + 36, m.dock.icons[3].y + 32, 61, 154, 138, 10);
    TEST_ASSERT_TRUE(ink > 50);
    /* compact menus are narrower and all eight fit */
    TEST_ASSERT_EQUAL(GFX_DESKTOP_MENUS, m.menu_count);
    TEST_ASSERT_TRUE(m.menu_items[1].w < l.menu_items[1].w);
    TEST_ASSERT_TRUE(m.chat_win.box.x >= 60);
    /* version 7 scenes ignore the bits */
    g_scene.version = 7;
    redraw(&m);
    TEST_ASSERT_EQUAL(0, gfx_desktop_dock_left());
    /* full screen chat (phone plan) */
    g_scene.version = 8;
    g_scene.scale_pad[1] = 0;
    g_scene.chat_mode = OS_FB_CHAT_FULL;
    redraw(&m);
    TEST_ASSERT_EQUAL(8, m.chat_win.box.x);
    TEST_ASSERT_TRUE(m.chat_win.box.w >= W - 16 - 2);
    TEST_ASSERT_TRUE(m.chat_win.box.h > l.chat_win.box.h);
    TEST_ASSERT_EQUAL(0, m.scene_ia.box.w);
    g_scene.chat_mode = OS_FB_CHAT_CENTER;
    gfx_desktop_set_form(0, 0);
}

/* Lot 69: Browser-OS shows a clickable address / search bar with back,
 * forward and go buttons; typed text is drawn inside the field. */
/* Lot 69: every window type has the same chrome: the three title buttons
 * come from gfx_wm (same size, same place, same colours), floating windows
 * resize by the corner grip, the full screen web / editor windows close,
 * minimize to the dock and restore from it. */
static int same_buttons(const gfx_rect_t *box, const gfx_rect_t btn[3], int title_h, int q) {
    gfx_wm_rect_t r;
    int b, x, y, n = gfx_wm_btn(title_h, q);
    r.x = box->x; r.y = box->y; r.w = box->w; r.h = box->h;
    for (b = 0; b < 3; b++) {
        gfx_wm_button_pos(r, title_h, q, GFX_WM_HIT_MIN + b, &x, &y);
        if (btn[b].x != x || btn[b].y != y || btn[b].w != n || btn[b].h != n) return 0;
        if (g_fb[(y + n / 2) * W + x + n / 2] == 0) return 0;
    }
    return 1;
}

static void test_lot69_every_window_shared_chrome(void) {
    static const uint8_t panes[6] = { OS_FB_PANE_BROWSER, OS_FB_PANE_ADMIN, OS_FB_PANE_SUPPORT,
                                      OS_FB_PANE_STATUS, OS_FB_PANE_FS, OS_FB_PANE_SHELL };
    static const uint8_t full[2] = { OS_FB_PANE_WEB, OS_FB_PANE_IDE };
    gfx_desktop_layout_t l, m;
    char got[64];
    int k, q;
    vga_desktop_set(1);
    /* chat */
    gfx_desktop_wm_reset();
    blank_scene();
    g_scene.chat_mode = OS_FB_CHAT_FLOAT;
    redraw(&l);
    q = l.chrome_q < 4 ? 4 : l.chrome_q;
    TEST_ASSERT(same_buttons(&l.chat_win.box, l.chat_win.btn, GFX_DESKTOP_CHAT_TITLE_H, q));
    /* panes: same buttons, and the corner grip resizes */
    for (k = 0; k < 6; k++) {
        int gx, gy;
        gfx_desktop_wm_reset();
        blank_scene();
        g_scene.chat_mode = OS_FB_CHAT_FLOAT;
        g_scene.pane = panes[k];
        redraw(&l);
        TEST_ASSERT(same_buttons(&l.pane_win.box, l.pane_win.btn, GFX_DESKTOP_PANE_TITLE_H, q));
        gx = l.pane_win.box.x + l.pane_win.box.w - 3;
        gy = l.pane_win.box.y + l.pane_win.box.h - 3;
        drag(gx, gy, gx - 60, gy - 40);
        redraw(&m);
        TEST_ASSERT_EQUAL(l.pane_win.box.x, m.pane_win.box.x);
        TEST_ASSERT_EQUAL(l.pane_win.box.w - 60, m.pane_win.box.w);
        TEST_ASSERT_EQUAL(l.pane_win.box.h - 40, m.pane_win.box.h);
        TEST_ASSERT(same_buttons(&m.pane_win.box, m.pane_win.btn, GFX_DESKTOP_PANE_TITLE_H, q));
    }
    /* app window */
    gfx_desktop_wm_reset();
    app_scene();
    redraw(&l);
    TEST_ASSERT(l.app_win[0].visible);
    TEST_ASSERT(same_buttons(&l.app_win[0].box, l.app_win[0].btn, GFX_DESKTOP_APP_TITLE_H, q));
    {
        int ex = l.app_win[0].box.x + l.app_win[0].box.w - 2, ey = l.app_win[0].box.y + 150;
        drag(ex, ey, ex - 80, ey);
        redraw(&m);
        TEST_ASSERT_EQUAL(l.app_win[0].box.w - 80, m.app_win[0].box.w);
    }
    /* full screen web and editor */
    for (k = 0; k < 2; k++) {
        gfx_rect_t fb_box, btn[3];
        gfx_wm_rect_t r;
        int b, x, y, n = gfx_wm_btn(GFX_DESKTOP_PANE_TITLE_H, q);
        gfx_desktop_wm_reset();
        blank_scene();
        g_scene.version = 7;
        g_scene.app_focus = 0xFF;
        g_scene.pane = full[k];
        redraw(&l);
        fb_box.x = 8; fb_box.y = l.bar_h + 8; fb_box.w = W - 16; fb_box.h = H - l.bar_h - 16;
        r.x = fb_box.x; r.y = fb_box.y; r.w = fb_box.w; r.h = fb_box.h;
        for (b = 0; b < 3; b++) {
            gfx_wm_button_pos(r, GFX_DESKTOP_PANE_TITLE_H, q, GFX_WM_HIT_MIN + b, &x, &y);
            btn[b].x = x; btn[b].y = y; btn[b].w = n; btn[b].h = n;
        }
        TEST_ASSERT(same_buttons(&fb_box, btn, GFX_DESKTOP_PANE_TITLE_H, q));
        TEST_ASSERT_EQUAL(btn[2].x, l.pane_win.traffic.x);
        TEST_ASSERT_EQUAL(GFX_WM_RGB_CLOSE, g_fb[(btn[2].y + 2) * W + btn[2].x + n / 2]);
        /* minimize: the desktop shows, no command; the dock restores */
        drain_kbd();
        click_at(btn[0].x + n / 2, btn[0].y + n / 2);
        redraw(&m);
        TEST_ASSERT(GFX_WM_RGB_CLOSE != g_fb[(btn[2].y + 2) * W + btn[2].x + n / 2]);
        read_kbd(got, (int)sizeof(got));
        TEST_ASSERT_EQUAL_STRING("", got);
        click_at(rect_center_x(&m.dock.icons[k ? 7 : 1]), rect_center_y(&m.dock.icons[k ? 7 : 1]));
        redraw(&m);
        TEST_ASSERT_EQUAL(GFX_WM_RGB_CLOSE, g_fb[(btn[2].y + 2) * W + btn[2].x + n / 2]);
        read_kbd(got, (int)sizeof(got));
        TEST_ASSERT_EQUAL_STRING("", got);
        /* close sends the closing command */
        click_at(btn[2].x + n / 2, btn[2].y + n / 2);
        read_kbd(got, (int)sizeof(got));
        TEST_ASSERT_EQUAL_STRING("\x16" "cmd /center\n", got);
    }
    vga_desktop_set(0);
}

/* Lot 71: the full-screen web / editor window restores, moves, resizes
 * and maximizes again with the shared buttons; the page origin and the
 * back / forward buttons follow the placed window. */
static void test_lot71_full_window_restore_resize(void) {
    static const uint8_t full[2] = { OS_FB_PANE_WEB, OS_FB_PANE_IDE };
    gfx_desktop_layout_t l;
    gfx_rect_t f0, f1, f2, f3, nav;
    gfx_wm_rect_t r;
    int k, x, y, n, ox, oy, pw, ph, q;
    vga_desktop_set(1);
    for (k = 0; k < 2; k++) {
        gfx_desktop_wm_reset();
        blank_scene();
        g_scene.version = 7;
        g_scene.app_focus = 0xFF;
        g_scene.pane = full[k];
        redraw(&l);
        q = l.chrome_q < 4 ? 4 : l.chrome_q;
        n = gfx_wm_btn(GFX_DESKTOP_PANE_TITLE_H, q);
        TEST_ASSERT(gfx_desktop_full_maximized());
        gfx_desktop_full_rect(W, H, &f0);
        TEST_ASSERT_EQUAL(8, f0.x);
        TEST_ASSERT_EQUAL(l.bar_h + 8, f0.y);
        TEST_ASSERT_EQUAL(W - 16, f0.w);
        gfx_desktop_web_origin(W, H, &ox, &oy, &pw, &ph);
        TEST_ASSERT_EQUAL(16, ox);
        TEST_ASSERT_EQUAL(l.bar_h + 48, oy);
        TEST_ASSERT_EQUAL(W - 32, pw);
        /* maximize button: restore to a smaller centered window */
        r.x = f0.x; r.y = f0.y; r.w = f0.w; r.h = f0.h;
        gfx_wm_button_pos(r, GFX_DESKTOP_PANE_TITLE_H, q, GFX_WM_HIT_MAX, &x, &y);
        click_at(x + n / 2, y + n / 2);
        redraw(&l);
        TEST_ASSERT(!gfx_desktop_full_maximized());
        gfx_desktop_full_rect(W, H, &f1);
        TEST_ASSERT(f1.w < f0.w && f1.h < f0.h && f1.x > f0.x && f1.y > f0.y);
        gfx_desktop_web_origin(W, H, &ox, &oy, &pw, &ph);
        TEST_ASSERT_EQUAL(f1.x + 8, ox);
        TEST_ASSERT_EQUAL(f1.w - 16, pw);
        gfx_desktop_web_nav(W, H, 0, &nav);
        TEST_ASSERT_EQUAL(f1.x + 8, nav.x);
        TEST_ASSERT_EQUAL(f1.y + 6, nav.y);
        /* the close button followed the window */
        r.x = f1.x; r.y = f1.y; r.w = f1.w; r.h = f1.h;
        gfx_wm_button_pos(r, GFX_DESKTOP_PANE_TITLE_H, q, GFX_WM_HIT_CLOSE, &x, &y);
        TEST_ASSERT_EQUAL(x, l.pane_win.traffic.x);
        /* title drag moves it */
        drag(f1.x + 200, f1.y + 8, f1.x + 150, f1.y + 28);
        redraw(&l);
        gfx_desktop_full_rect(W, H, &f2);
        TEST_ASSERT_EQUAL(f1.x - 50, f2.x);
        TEST_ASSERT_EQUAL(f1.y + 20, f2.y);
        TEST_ASSERT_EQUAL(f1.w, f2.w);
        /* the grip resizes it, the web view follows */
        drag(f2.x + f2.w - 3, f2.y + f2.h - 3, f2.x + f2.w - 83, f2.y + f2.h - 43);
        redraw(&l);
        gfx_desktop_full_rect(W, H, &f3);
        TEST_ASSERT_EQUAL(f2.w - 80, f3.w);
        TEST_ASSERT_EQUAL(f2.h - 40, f3.h);
        gfx_desktop_web_origin(W, H, &ox, &oy, &pw, &ph);
        TEST_ASSERT_EQUAL(f3.w - 16, pw);
        /* maximize again: the Lot 69 place */
        r.x = f3.x; r.y = f3.y; r.w = f3.w; r.h = f3.h;
        gfx_wm_button_pos(r, GFX_DESKTOP_PANE_TITLE_H, q, GFX_WM_HIT_MAX, &x, &y);
        click_at(x + n / 2, y + n / 2);
        redraw(&l);
        TEST_ASSERT(gfx_desktop_full_maximized());
        gfx_desktop_full_rect(W, H, &f1);
        TEST_ASSERT_EQUAL(f0.x, f1.x);
        TEST_ASSERT_EQUAL(f0.w, f1.w);
        TEST_ASSERT_EQUAL(f0.h, f1.h);
    }
    gfx_desktop_wm_reset();
    vga_desktop_set(0);
}

/* Lot 71: resizing or moving the pane keeps the floating chat width
 * (it used to follow the room right of the pane on every drag). */
static void test_lot71_chat_width_stable_on_pane_resize(void) {
    static const int sizes[3][2] = { { 1024, 768 }, { 1280, 720 }, { 800, 600 } };
    gfx_desktop_layout_t l, m;
    int k, gx, gy;
    vga_desktop_set(1);
    for (k = 0; k < 3; k++) {
        gfx_desktop_wm_reset();
        blank_scene();
        g_scene.chat_mode = OS_FB_CHAT_FLOAT;
        g_scene.pane = OS_FB_PANE_BROWSER;
        gfx_desktop_get_layout(&g_scene, sizes[k][0], sizes[k][1], &l);
        if (sizes[k][0] != W) continue;   /* the mouse runs on the W x H test screen */
        redraw(&l);
        gx = l.pane_win.box.x + l.pane_win.box.w - 3;
        gy = l.pane_win.box.y + l.pane_win.box.h - 3;
        drag(gx, gy, gx + 120, gy);
        redraw(&m);
        TEST_ASSERT_EQUAL(l.pane_win.box.w + 120, m.pane_win.box.w);
        TEST_ASSERT_EQUAL(l.chat_win.box.w, m.chat_win.box.w);
        TEST_ASSERT_EQUAL(l.chat_win.box.h, m.chat_win.box.h);
        drag(gx + 120, gy, gx - 100, gy);
        redraw(&m);
        TEST_ASSERT_EQUAL(l.chat_win.box.w, m.chat_win.box.w);
    }
    gfx_desktop_wm_reset();
    vga_desktop_set(0);
}

static void test_lot69_browser_address_bar(void) {
    gfx_desktop_layout_t l;
    char got[64];
    const gfx_rect_t *a;
    int x, y, dark = 0;
    vga_desktop_set(1);
    gfx_desktop_wm_reset();
    blank_scene();
    g_scene.chat_mode = OS_FB_CHAT_FLOAT;
    g_scene.pane = OS_FB_PANE_BROWSER;
    redraw(&l);
    TEST_ASSERT(l.pane_win.active);
    TEST_ASSERT_EQUAL(1, l.pane_win.has_addr);
    a = &l.pane_win.addr;
    TEST_ASSERT(a->w > 200 && a->h >= 24);
    TEST_ASSERT(l.pane_win.addr_back.x < l.pane_win.addr_fwd.x && l.pane_win.addr_fwd.x < a->x);
    TEST_ASSERT(a->x + a->w < l.pane_win.addr_go.x);
    /* white field, right part empty with the placeholder on the left */
    TEST_ASSERT_EQUAL(0xF4F7FAu, g_fb[(a->y + a->h / 2) * W + a->x + a->w - 6]);
    /* typed text: dark glyph pixels inside the field */
    strcpy(g_scene.input, "example.com");
    redraw(&l);
    for (y = a->y + 2; y < a->y + a->h - 2; y++)
        for (x = a->x + 6; x < a->x + 6 + 11 * 8; x++)
            if (g_fb[y * W + x] == 0x101820u) dark++;
    TEST_ASSERT(dark > 40);
    /* other panes have no bar */
    g_scene.pane = OS_FB_PANE_FS;
    redraw(&l);
    TEST_ASSERT_EQUAL(0, l.pane_win.has_addr);
    g_scene.pane = OS_FB_PANE_BROWSER;
    redraw(&l);
    /* buttons: back / forward are web events, Aller is Enter */
    drain_kbd();
    click_at(rect_center_x(&l.pane_win.addr_back), rect_center_y(&l.pane_win.addr_back));
    read_kbd(got, (int)sizeof(got));
    TEST_ASSERT_EQUAL_STRING("\x16" "web back\n", got);
    drain_kbd();
    click_at(rect_center_x(&l.pane_win.addr_fwd), rect_center_y(&l.pane_win.addr_fwd));
    read_kbd(got, (int)sizeof(got));
    TEST_ASSERT_EQUAL_STRING("\x16" "web fwd\n", got);
    drain_kbd();
    click_at(rect_center_x(&l.pane_win.addr_go), rect_center_y(&l.pane_win.addr_go));
    read_kbd(got, (int)sizeof(got));
    TEST_ASSERT_EQUAL_STRING("\n", got);
    drain_kbd();
    click_at(rect_center_x(a), rect_center_y(a));
    read_kbd(got, (int)sizeof(got));
    TEST_ASSERT_EQUAL_STRING("", got);
    g_scene.input[0] = 0;
    vga_desktop_set(0);
}

/* Lot 72: glyph pages. A byte of page 2 (Greek) draws its own glyph, not
 * the CP1252 letter of the same byte nor '?'; ASCII is the same in every
 * page. */
static void test_lot72_glyph_pages(void) {
    static const int qs[5] = { 4, 6, 8, 12, 16 };
    int k, x, y;
    for (k = 0; k < 5; k++) {
        int q = qs[k], n = os_ui_px(8, q), d_lat = 0, d_q = 0, ink = 0, same = 1;
        for (y = 0; y < n; y++)
            for (x = 0; x < n; x++) {
                int g = gfx_glyph_pixel_page(2, (char)0xE1, q, x, y);   /* alpha */
                ink += g;
                d_lat += g != gfx_glyph_pixel((char)0xE1, q, x, y);    /* a acute */
                d_q += g != gfx_glyph_pixel('?', q, x, y);
                if (gfx_glyph_pixel_page(3, 'A', q, x, y) != gfx_glyph_pixel('A', q, x, y)) same = 0;
            }
        TEST_ASSERT_TRUE(ink > 4);
        TEST_ASSERT_TRUE(d_lat > 2);
        TEST_ASSERT_TRUE(d_q > 2);
        TEST_ASSERT_TRUE(same);
        TEST_ASSERT_EQUAL(0, gfx_glyph_pixel_page(2, (char)0xE1, q, n, 0));
    }
    /* page 0 is the Lot 71 font */
    for (y = 0; y < 8; y++)
        for (x = 0; x < 8; x++) TEST_ASSERT_EQUAL(gfx_glyph_pixel((char)0xE9, 4, x, y), gfx_glyph_pixel_page(0, (char)0xE9, 4, x, y));
}

/* web TEXT item: pad bits 8..10 = page, low byte = font scale */
static void web_item_page(int pad, const char *text) {
    blank_scene();
    g_scene.version = 8;
    g_scene.ui_scale_q = 4;
    g_scene.pane = OS_FB_PANE_WEB;
    strcpy(g_scene.web_title, "pages");
    g_scene.nweb = 1;
    g_scene.web[0].kind = OS_FB_WEB_TEXT; g_scene.web[0].x = 10; g_scene.web[0].y = 20;
    g_scene.web[0].w = 40; g_scene.web[0].h = 16;
    g_scene.web[0].pad = (uint16_t)pad;
    g_scene.web[0].color = 0x00AA00; strcpy(g_scene.web[0].text, text);
    gfx_desktop_draw_no_cursor(&g_scene, g_fb, W, H);
}

static int ink_matches(int px, int py, int page, char ch, int q) {
    int x, y, n = os_ui_px(8, q), bad = 0;
    for (y = 0; y < n; y++)
        for (x = 0; x < n; x++) {
            int want = gfx_glyph_pixel_page(page, ch, q, x, y);
            int got = near_rgb(gfx_desktop_pixel(g_fb, W, H, px + x, py + y), 0x00, 0xAA, 0x00, 8);
            bad += want != got;
        }
    return bad == 0;
}

static void test_lot72_web_item_page(void) {
    int ox, oy, pw, ph;
    gfx_desktop_web_origin(W, H, &ox, &oy, &pw, &ph);
    web_item_page(2 << 8, "\xe1\xe2");
    TEST_ASSERT_TRUE(ink_matches(ox + 10, oy + 20, 2, (char)0xE1, 4));
    TEST_ASSERT_TRUE(ink_matches(ox + 18, oy + 20, 2, (char)0xE2, 4));
    TEST_ASSERT_FALSE(ink_matches(ox + 10, oy + 20, 0, (char)0xE1, 4));
    web_item_page(0, "\xe1\xe2");
    TEST_ASSERT_TRUE(ink_matches(ox + 10, oy + 20, 0, (char)0xE1, 4));
    /* the page bits do not disturb the font scale: 2x Cyrillic */
    web_item_page((3 << 8) | 8, "\xb6");
    TEST_ASSERT_TRUE(ink_matches(ox + 10, oy + 20, 3, (char)0xB6, 8));
    web_item_page(8, "\xb6");
    TEST_ASSERT_TRUE(ink_matches(ox + 10, oy + 20, 0, (char)0xB6, 8));
}

/* GUI strings carry in-band selectors: they take no cell and switch the
 * page of the bytes after them (title of the full web window) */
static int title_ink(const char *title, uint32_t *sig) {
    int x, y, n = 0;
    uint32_t hsh = 2166136261u;
    blank_scene();
    g_scene.version = 8;
    g_scene.ui_scale_q = 4;
    g_scene.pane = OS_FB_PANE_WEB;
    strcpy(g_scene.web_title, title);
    gfx_desktop_draw_no_cursor(&g_scene, g_fb, W, H);
    for (y = 0; y < 80; y++)
        for (x = 0; x < W / 2; x++)
            if (near_rgb(gfx_desktop_pixel(g_fb, W, H, x, y), 232, 238, 244, 2)) { n++; hsh = (hsh ^ (uint32_t)(y * W + x)) * 16777619u; }
    *sig = hsh;
    return n;
}

static void test_lot72_title_selectors(void) {
    uint32_t a, b, c, d;
    int na = title_ink("web ab\x8f\xe1\xe2", &a);    /* Greek alpha beta */
    int nb = title_ink("web ab\x81\xe1\xe2", &b);    /* page 0: a acute, a circumflex */
    int nc = title_ink("web ab\xe1\xe2", &c);
    title_ink("web ab\x8f\xe1\xe2\x81", &d);         /* a trailing selector draws nothing */
    TEST_ASSERT_TRUE(na > 20 && nb > 20);
    TEST_ASSERT_TRUE(a != b);
    TEST_ASSERT_EQUAL(nb, nc);                       /* the page 0 selector takes no cell */
    TEST_ASSERT_EQUAL(b, c);
    TEST_ASSERT_EQUAL(a, d);
}

static void test_lot72_wrap_take_cells(void) {
    char t[128];
    int i, n = 0;
    /* 20 Greek letters, each behind its own selector: 40 bytes, 20 cells */
    for (i = 0; i < 20; i++) { t[n++] = (char)0x8F; t[n++] = (char)0xE1; }
    t[n] = 0;
    TEST_ASSERT_EQUAL(n, gfx_text_wrap_take(t, 0, n, 20));        /* fits one line of 20 cells */
    TEST_ASSERT_EQUAL(20, gfx_text_wrap_take(t, 0, n, 10));       /* 10 cells = 20 bytes */
    strcpy(t, "abc def ghij");
    TEST_ASSERT_EQUAL(7, gfx_text_wrap_take(t, 0, 12, 8));         /* cut at the space */
    TEST_ASSERT_EQUAL(12, gfx_text_wrap_take(t, 0, 12, 12));
    TEST_ASSERT_EQUAL(5, gfx_text_wrap_take("abcdefghij", 0, 10, 5));   /* no space: hard cut */
    /* a selector inside the line is not counted as a cell */
    strcpy(t, "ab\x8f\xe1\xe2 cd ef");
    TEST_ASSERT_EQUAL(8, gfx_text_wrap_take(t, 0, (int)strlen(t), 8));
    /* going back to the last space counts cells, not selector bytes */
    strcpy(t, "abcdef \x8f\xe1\x90\xb6\x8f\xe1\x90\xb6\x8f\xe1\x90\xb6");
    TEST_ASSERT_EQUAL(6, gfx_text_wrap_take(t, 0, (int)strlen(t), 10));
}

/* the chat keeps the messages that fit by cells: 4 messages of 30
 * alternating Greek / Cyrillic letters (60 bytes, 30 cells) take 2 lines
 * each at 30 cells, so 8 lines show them all */
static void test_lot72_chat_first_by_cells(void) {
    static os_fb_scene_t sc;
    int i, k;
    memset(&sc, 0, sizeof(sc));
    for (i = 0; i < 4; i++) {
        for (k = 0; k < 30; k++) { sc.messages[i][2 * k] = (char)(k & 1 ? 0x90 : 0x8F); sc.messages[i][2 * k + 1] = (char)0xE1; }
        sc.messages[i][60] = 0;
    }
    TEST_ASSERT_EQUAL(0, gfx_chat_first(&sc, 4, 30, 8));
    TEST_ASSERT_EQUAL(1, gfx_chat_first(&sc, 4, 30, 7));
    TEST_ASSERT_EQUAL(3, gfx_chat_first(&sc, 4, 30, 1));   /* the newest is always shown */
}

/* the input line keeps its page when only its end fits (address field) */
static int addr_field_sig(const char *input) {
    gfx_desktop_layout_t l;
    const gfx_rect_t *a;
    int x, y;
    uint32_t hsh = 2166136261u;
    vga_desktop_set(1);
    gfx_desktop_wm_reset();
    blank_scene();
    g_scene.chat_mode = OS_FB_CHAT_FLOAT;
    g_scene.pane = OS_FB_PANE_BROWSER;
    strcpy(g_scene.input, input);
    redraw(&l);
    a = &l.pane_win.addr;
    for (y = a->y + 2; y < a->y + a->h - 2; y++)
        for (x = a->x + 2; x < a->x + a->w - 2; x++)
            if (g_fb[y * W + x] == 0x101820u) hsh = (hsh ^ (uint32_t)(y * W + x)) * 16777619u;
    vga_desktop_set(0);
    return (int)hsh;
}

static void test_lot72_chat_input_tail_page(void) {
    static char greek[OS_FB_INPUT_LEN], latin[OS_FB_INPUT_LEN];
    gfx_desktop_layout_t l;
    int i, x, y;
    uint32_t ga = 0, la = 0;   /* unsigned: the hash wraps */
    /* chat input longer than the field: the tail starts after the
     * selector, and must still draw Greek */
    greek[0] = (char)0x8F;
    for (i = 1; i < OS_FB_INPUT_LEN - 1; i++) { greek[i] = (char)0xE1; latin[i - 1] = (char)0xE1; }
    greek[OS_FB_INPUT_LEN - 1] = 0;
    latin[OS_FB_INPUT_LEN - 2] = 0;
    blank_scene();
    g_scene.version = 8;
    strcpy(g_scene.input, greek);
    redraw(&l);
    for (y = 0; y < H; y++) for (x = 0; x < W; x++) if (near_rgb(g_fb[y * W + x], 232, 238, 244, 0)) ga = ga * 31 + x + y * W;
    strcpy(g_scene.input, latin);
    redraw(&l);
    for (y = 0; y < H; y++) for (x = 0; x < W; x++) if (near_rgb(g_fb[y * W + x], 232, 238, 244, 0)) la = la * 31 + x + y * W;
    TEST_ASSERT_TRUE(ga != la);
    /* the address field draws pages from the start of the text */
    TEST_ASSERT_TRUE(addr_field_sig("ab\x8f\xe1") != addr_field_sig("ab\xe1"));
    TEST_ASSERT_EQUAL(addr_field_sig("ab\x81\xe1"), addr_field_sig("ab\xe1"));
}

/* Lot 72: the favicon of the page also starts the Browser-OS address
 * field, and the text moves right of it */
static void test_lot72_favicon_in_address_bar(void) {
    gfx_desktop_layout_t l;
    const gfx_rect_t *a;
    int k, green, x, y, first_dark = -1;
    memset(&g_atlas, 0, sizeof(g_atlas));
    g_atlas.magic = OS_FB_WEBIMG_MAGIC;
    g_atlas.slot[0].w = 16; g_atlas.slot[0].h = 16;
    for (k = 0; k < 256; k++) { g_atlas.rgb[k * 3] = 0; g_atlas.rgb[k * 3 + 1] = 200; g_atlas.rgb[k * 3 + 2] = 0; }
    vga_desktop_set(1);
    gfx_desktop_wm_reset();
    blank_scene();
    g_scene.chat_mode = OS_FB_CHAT_FLOAT;
    g_scene.pane = OS_FB_PANE_BROWSER;
    strcpy(g_scene.input, "example.com");
    TEST_ASSERT_EQUAL(0, gfx_desktop_set_webimg(&g_atlas));
    redraw(&l);
    a = &l.pane_win.addr;
    green = count_near(a->x, a->y, a->x + 40, a->y + a->h, 0, 200, 0, 0);
    TEST_ASSERT_TRUE(green >= 12 * 12);
    for (x = a->x + 2; x < a->x + 80 && first_dark < 0; x++)
        for (y = a->y + 2; y < a->y + a->h - 2; y++)
            if (g_fb[y * W + x] == 0x101820u) { first_dark = x; break; }
    TEST_ASSERT_TRUE(first_dark >= a->x + 8 + 16);
    /* no favicon: nothing green, the text starts at the field edge */
    TEST_ASSERT_EQUAL(0, gfx_desktop_set_webimg(0));
    redraw(&l);
    TEST_ASSERT_EQUAL(0, count_near(a->x, a->y, a->x + 40, a->y + a->h, 0, 200, 0, 0));
    first_dark = -1;
    for (x = a->x + 2; x < a->x + 80 && first_dark < 0; x++)
        for (y = a->y + 2; y < a->y + a->h - 2; y++)
            if (g_fb[y * W + x] == 0x101820u) { first_dark = x; break; }
    TEST_ASSERT_TRUE(first_dark >= a->x + 8 && first_dark < a->x + 8 + 8);
    g_scene.input[0] = 0;
    vga_desktop_set(0);
}

int main(void) {
    unity_init();
    RUN_TEST(test_center_desktop_not_text_mode);
    RUN_TEST(test_lot72_glyph_pages);
    RUN_TEST(test_lot72_web_item_page);
    RUN_TEST(test_lot72_title_selectors);
    RUN_TEST(test_lot72_wrap_take_cells);
    RUN_TEST(test_lot72_chat_first_by_cells);
    RUN_TEST(test_lot72_chat_input_tail_page);
    RUN_TEST(test_lot72_favicon_in_address_bar);
    RUN_TEST(test_chat_and_dock_and_stage);
    RUN_TEST(test_float_chat_and_browser_pane);
    RUN_TEST(test_shell_pane_shows_prompt);
    RUN_TEST(test_circle_kind_paints_ring);
    RUN_TEST(test_layout_scales_with_framebuffer);
    RUN_TEST(test_pane_layout_has_no_overlaps);
    RUN_TEST(test_parse_fit_line);
    RUN_TEST(test_mouse_cursor_rendering);
    RUN_TEST(test_fb_present_repeated);
    RUN_TEST(test_desktop_layout_and_hit_test);
    RUN_TEST(test_hit_regions_dock_menu_stage_close);
    RUN_TEST(test_desktop_icons_click);
    RUN_TEST(test_llm_label_from_scene);
    RUN_TEST(test_p2p_collab_prod_panes_drawing);
    RUN_TEST(test_vbe_layout_automation_under_load);
    RUN_TEST(test_web_pane_display_list);
    RUN_TEST(test_web_pane_atlas_images);
    RUN_TEST(test_window_manager_panes_and_chat);
    RUN_TEST(test_app_windows_events_and_z_order);
    RUN_TEST(test_generic_app_slots_z_order);
    RUN_TEST(test_shared_clipboard);
    RUN_TEST(test_web_nav_events);
    RUN_TEST(test_app_grid_fits_window);
    RUN_TEST(test_viewer_image_pixels);
    RUN_TEST(test_llm_friendly_names);
    RUN_TEST(test_lot68_scaled_glyphs);
    RUN_TEST(test_lot68_scene_scale_and_grid);
    RUN_TEST(test_lot69_native_fonts);
    RUN_TEST(test_lot69_status_row_scales);
    RUN_TEST(test_lot69_web_item_font_scale);
    RUN_TEST(test_lot69_dock_tooltip_scales);
    RUN_TEST(test_lot69_form_layout);
    RUN_TEST(test_lot69_browser_address_bar);
    RUN_TEST(test_lot69_every_window_shared_chrome);
    RUN_TEST(test_lot71_full_window_restore_resize);
    RUN_TEST(test_lot71_chat_width_stable_on_pane_resize);
    unity_print_results();
    unity_cleanup();
    return (unity_stats.tests_failed == 0) ? 0 : 1;
}
