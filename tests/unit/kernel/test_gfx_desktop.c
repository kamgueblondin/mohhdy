/* test_gfx_desktop.c - Pixels du bureau VBE (glassmorphic, pas ASCII 80x25). */

#include "../../framework/unity.h"
#include "../../../kernel/gfx_desktop.h"
#include "../../../kernel/gfx_fb.h"
#include "../../../kernel/gfx_wm.h"
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
    TEST_ASSERT_EQUAL(7, layout.menu_count);
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
    TEST_ASSERT_EQUAL_STRING("/shell\n", got);

    /* Top menu Browser-OS (index 1) */
    drain_kbd();
    click_at(rect_center_x(&layout.menu_items[1]), rect_center_y(&layout.menu_items[1]));
    read_kbd(got, (int)sizeof(got));
    TEST_ASSERT_EQUAL_STRING("/browser\n", got);

    /* Stage mode badge */
    drain_kbd();
    click_at(rect_center_x(&layout.stage_badge), rect_center_y(&layout.stage_badge));
    read_kbd(got, (int)sizeof(got));
    TEST_ASSERT_EQUAL_STRING("/stage\n", got);

    /* Scene IA first pill */
    drain_kbd();
    click_at(rect_center_x(&layout.scene_ia.pills[0]), rect_center_y(&layout.scene_ia.pills[0]));
    read_kbd(got, (int)sizeof(got));
    TEST_ASSERT_EQUAL_STRING("/stage-prompt reflexion\n", got);

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
    TEST_ASSERT_EQUAL_STRING("/status\n", got);
    drain_kbd();
    click_at(rect_center_x(&layout.desk.icons[5]), rect_center_y(&layout.desk.icons[5]));
    read_kbd(got, (int)sizeof(got));
    TEST_ASSERT_EQUAL_STRING("/fs\n", got);
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
        TEST_ASSERT_EQUAL_STRING("/status\n", got);
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
        TEST_ASSERT_EQUAL(7, layout.menu_count);
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
        TEST_ASSERT_EQUAL_STRING("/center\n", got);
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

int main(void) {
    unity_init();
    RUN_TEST(test_center_desktop_not_text_mode);
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
    RUN_TEST(test_window_manager_panes_and_chat);
    unity_print_results();
    unity_cleanup();
    return (unity_stats.tests_failed == 0) ? 0 : 1;
}
