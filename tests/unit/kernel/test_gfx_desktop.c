/* test_gfx_desktop.c - Pixels du bureau VBE (glassmorphic, pas ASCII 80x25). */

#include "../../framework/unity.h"
#include "../../../kernel/gfx_desktop.h"
#include "../../../kernel/gfx_fb.h"
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

static void test_desktop_layout_and_hit_test(void) {
    gfx_desktop_layout_t layout;
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

    /* Move mouse over "Envoyer" button and press left button (0 -> 1) */
    int send_x = layout.chat_win.send_btn.x + 5;
    int send_y = layout.chat_win.send_btn.y + 5;
    gfx_desktop_set_mouse(send_x, send_y, 0);
    gfx_desktop_set_mouse(send_x, send_y, 1); /* Rising edge click */

    /* Verify character was queued in keyboard buffer */
    char c = 0;
    TEST_ASSERT(kbd_get_char_nonblock(&c));
    TEST_ASSERT_EQUAL('\n', c);

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
    RUN_TEST(test_parse_fit_line);
    RUN_TEST(test_mouse_cursor_rendering);
    RUN_TEST(test_fb_present_repeated);
    RUN_TEST(test_desktop_layout_and_hit_test);
    unity_print_results();
    unity_cleanup();
    return (unity_stats.tests_failed == 0) ? 0 : 1;
}
