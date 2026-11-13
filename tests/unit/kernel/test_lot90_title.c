/* Lot 90: the title bars follow the chrome scale (gfx_wm_title_h). At 1x
 * nothing moves; at 1.5x and 2x the pane, chat and app title bars are
 * taller, their buttons may grow up to title - 6, and everything under a
 * title bar (address bar, web origin, app toolbar and grid) moves down by
 * the growth. */
#include "../../framework/unity.h"
#include "../../../include/os_ui_scale.h"
#include <string.h>
#include "../../../kernel/gfx_desktop.h"
#include "../../../kernel/gfx_fb.h"
#include "../../../kernel/gfx_wm.h"
#include "../../../kernel/edid.h"

#define W 1024
#define H 768

static uint32_t g_fb[W * H];
static os_fb_scene_t g_scene;

static void scene(int q, int pane) {
    memset(&g_scene, 0, sizeof(g_scene));
    g_scene.magic = OS_FB_MAGIC;
    g_scene.version = 8;
    g_scene.ui_scale_q = (uint8_t)q;
    g_scene.chat_mode = OS_FB_CHAT_FLOAT;
    g_scene.pane = (uint8_t)pane;
    g_scene.stage_mode = OS_FB_STAGE_REFLECTING;
    g_scene.stage_kind = OS_FB_KIND_PLAN;
    memcpy(g_scene.session, "s0001", 6);
}

static int title_rgb(uint32_t c) {
    return ((c >> 16) & 255u) == 28 && ((c >> 8) & 255u) == 44 && (c & 255u) == 56;
}

static void test_title_h_formula(void) {
    TEST_ASSERT_EQUAL(30, gfx_wm_title_h(30, 4));
    TEST_ASSERT_EQUAL(30, gfx_wm_title_h(30, 1));      /* integer 1x */
    TEST_ASSERT_EQUAL(30, gfx_wm_title_h(30, 0));
    TEST_ASSERT_EQUAL(38, gfx_wm_title_h(30, 5));      /* (150 + 2) / 4 */
    TEST_ASSERT_EQUAL(45, gfx_wm_title_h(30, 6));
    TEST_ASSERT_EQUAL(51, gfx_wm_title_h(34, 6));
    TEST_ASSERT_EQUAL(60, gfx_wm_title_h(30, 8));
    TEST_ASSERT_EQUAL(60, gfx_wm_title_h(30, 2));      /* integer 2x */
    TEST_ASSERT_EQUAL(60, gfx_wm_title_h(30, 12));     /* capped at twice */
    TEST_ASSERT_EQUAL(68, gfx_wm_title_h(34, 16));
    TEST_ASSERT_EQUAL(0, gfx_wm_title_h(0, 8));
    TEST_ASSERT_EQUAL(0, gfx_wm_title_h(-3, 8));
}

static void test_buttons_grow_with_the_bar(void) {
    /* Lot 88 capped the 2x buttons at 24 (title 30 - 6); with a 60 px bar
     * they take their full 2x size, 28 */
    TEST_ASSERT_EQUAL(24, gfx_wm_btn(30, 8));
    TEST_ASSERT_EQUAL(28, gfx_wm_btn(gfx_wm_title_h(30, 8), 8));
    TEST_ASSERT_EQUAL(21, gfx_wm_btn(gfx_wm_title_h(30, 6), 6));
    TEST_ASSERT_EQUAL(14, gfx_wm_btn(gfx_wm_title_h(30, 4), 4));
}

static void test_one_x_does_not_move(void) {
    gfx_desktop_layout_t l;
    int ox, oy, pw, ph;
    scene(4, OS_FB_PANE_BROWSER);
    gfx_desktop_get_layout(&g_scene, W, H, &l);
    TEST_ASSERT_EQUAL(4, l.chrome_q);
    TEST_ASSERT_TRUE(l.pane_win.has_addr);
    TEST_ASSERT_EQUAL(l.pane_win.box.y + 44, l.pane_win.addr.y);
    scene(4, OS_FB_PANE_WEB);
    gfx_desktop_get_layout(&g_scene, W, H, &l);
    gfx_desktop_web_origin(W, H, &ox, &oy, &pw, &ph);
    {
        gfx_rect_t f;
        gfx_desktop_full_rect(W, H, &f);
        TEST_ASSERT_EQUAL(f.y + 40, oy);
        TEST_ASSERT_EQUAL(f.h - 48, ph);
    }
}

static void test_address_bar_moves_down(void) {
    gfx_desktop_layout_t l;
    int q;
    for (q = 5; q <= 8; q++) {
        scene(q, OS_FB_PANE_BROWSER);
        gfx_desktop_get_layout(&g_scene, W, H, &l);
        TEST_ASSERT_EQUAL(q, l.chrome_q);
        TEST_ASSERT_EQUAL(l.pane_win.box.y + 44 + gfx_wm_title_h(30, q) - 30, l.pane_win.addr.y);
        TEST_ASSERT_EQUAL(l.pane_win.addr.y, l.pane_win.addr_back.y);
        TEST_ASSERT_EQUAL(l.pane_win.addr.y, l.pane_win.addr_go.y);
        /* the buttons sit inside the taller bar */
        TEST_ASSERT_EQUAL(gfx_wm_btn(gfx_wm_title_h(30, q), q), l.pane_win.btn[0].w);
        TEST_ASSERT_TRUE(l.pane_win.btn[0].y + l.pane_win.btn[0].h <= l.pane_win.box.y + 2 + gfx_wm_title_h(30, q));
    }
}

static void test_web_origin_moves_down(void) {
    gfx_desktop_layout_t l;
    gfx_rect_t f;
    int ox, oy, pw, ph;
    scene(8, OS_FB_PANE_WEB);
    gfx_desktop_get_layout(&g_scene, W, H, &l);
    gfx_desktop_full_rect(W, H, &f);
    gfx_desktop_web_origin(W, H, &ox, &oy, &pw, &ph);
    TEST_ASSERT_EQUAL(f.y + 70, oy);
    TEST_ASSERT_EQUAL(f.h - 78, ph);
    TEST_ASSERT_EQUAL(f.w - 16, pw);
    {
        gfx_rect_t nav;
        gfx_desktop_web_nav(W, H, 0, &nav);
        TEST_ASSERT_EQUAL(f.y + 6 + 15, nav.y);
        TEST_ASSERT_TRUE(nav.y + nav.h <= oy);
    }
}

static void test_app_grid_moves_down(void) {
    gfx_desktop_layout_t l;
    int y4, y8;
    scene(4, OS_FB_PANE_NONE);
    gfx_desktop_get_layout(&g_scene, W, H, &l);
    y4 = gfx_desktop_app_grid_y(4);
    TEST_ASSERT_EQUAL(GFX_APP_GRID_Y, y4);
    scene(8, OS_FB_PANE_NONE);
    gfx_desktop_get_layout(&g_scene, W, H, &l);
    y8 = gfx_desktop_app_grid_y(4);
    TEST_ASSERT_EQUAL(GFX_APP_GRID_Y + 30, y8);
    TEST_ASSERT_EQUAL(GFX_APP_TOOL_Y + os_ui_px(8, 8) + 14 + 30, gfx_desktop_app_grid_y(8));
}

static void test_app_window_toolbar_under_bar(void) {
    gfx_desktop_layout_t l;
    scene(8, OS_FB_PANE_NONE);
    g_scene.app[0].kind = 1;
    strcpy(g_scene.app[0].title, "App");
    gfx_desktop_get_layout(&g_scene, W, H, &l);
    TEST_ASSERT_TRUE(l.app_win[0].visible);
    TEST_ASSERT_EQUAL(l.app_win[0].box.y + GFX_APP_TOOL_Y + 30 - 3, l.app_win[0].tool.y);
    TEST_ASSERT_TRUE(l.app_win[0].tool.y >= l.app_win[0].box.y + 60);
    TEST_ASSERT_TRUE(l.app_win[0].grid.y > l.app_win[0].tool.y);
}

/* the painted title band of the browser pane is as tall as the bar */
static int band_rows(int q) {
    gfx_desktop_layout_t l;
    int y, rows = 0, x;
    scene(q, OS_FB_PANE_BROWSER);
    gfx_desktop_draw_no_cursor(&g_scene, g_fb, W, H);
    gfx_desktop_get_layout(&g_scene, W, H, &l);
    x = l.pane_win.box.x + 8;
    for (y = l.pane_win.box.y; y < l.pane_win.box.y + 90; y++)
        if (title_rgb(gfx_desktop_pixel(g_fb, W, H, x, y))) rows++;
    return rows;
}

static void test_painted_band_follows(void) {
    int r4 = band_rows(4), r6 = band_rows(6), r8 = band_rows(8);
    TEST_ASSERT_TRUE(r4 >= 28 && r4 <= 32);
    TEST_ASSERT_EQUAL(r4 + 15, r6);
    TEST_ASSERT_EQUAL(r4 + 30, r8);
}

static void test_chat_band_follows(void) {
    gfx_desktop_layout_t l;
    int q, y, rows[2], x;
    for (q = 0; q < 2; q++) {
        scene(q ? 8 : 4, OS_FB_PANE_NONE);
        gfx_desktop_draw_no_cursor(&g_scene, g_fb, W, H);
        gfx_desktop_get_layout(&g_scene, W, H, &l);
        x = l.chat_win.box.x + 10;
        rows[q] = 0;
        for (y = l.chat_win.box.y; y < l.chat_win.box.y + 100; y++)
            if (title_rgb(gfx_desktop_pixel(g_fb, W, H, x, y))) rows[q]++;
    }
    TEST_ASSERT_TRUE(rows[0] >= 34 && rows[0] <= 37);
    TEST_ASSERT_EQUAL(rows[0] + 34, rows[1]);
}

/* ---- TV safe area (US-080) ------------------------------------------ */

static void tv_sum(edid_cea_sum_t *s, int vic, int underscan) {
    memset(s, 0, sizeof(*s));
    s->status = 0;
    s->flags = (uint8_t)(EDID_SUM_HDMI | (underscan ? EDID_SUM_UNDERSCAN : 0));
    s->native_vic = (uint8_t)vic;
}

static void test_overscan_margin_rule(void) {
    edid_cea_sum_t s;
    int mx = -1, my = -1;
    tv_sum(&s, 16, 0);                       /* 1080p TV, no underscan bit */
    TEST_ASSERT_EQUAL(1, edid_overscan_margin(&s, 1000, 1024, 768, &mx, &my));
    TEST_ASSERT_EQUAL(26, mx);
    TEST_ASSERT_EQUAL(19, my);
    TEST_ASSERT_EQUAL(1, edid_overscan_margin(&s, 1000, 1920, 1080, &mx, &my));
    TEST_ASSERT_EQUAL(48, mx);
    TEST_ASSERT_EQUAL(27, my);
    tv_sum(&s, 16, 1);                       /* declares underscan: none */
    TEST_ASSERT_EQUAL(0, edid_overscan_margin(&s, 1000, 1024, 768, &mx, &my));
    TEST_ASSERT_EQUAL(0, mx);
    TEST_ASSERT_EQUAL(0, my);
    tv_sum(&s, 0, 0);                        /* a monitor with a size: not a TV */
    TEST_ASSERT_EQUAL(0, edid_overscan_margin(&s, 600, 1024, 768, &mx, &my));
    TEST_ASSERT_EQUAL(1, edid_overscan_margin(&s, 0, 1024, 768, &mx, &my));   /* no size: TV */
    s.flags = 0;                             /* no HDMI block */
    TEST_ASSERT_EQUAL(0, edid_overscan_margin(&s, 0, 1024, 768, &mx, &my));
    tv_sum(&s, 16, 0);
    s.status = 1;                            /* no CEA block */
    TEST_ASSERT_EQUAL(0, edid_overscan_margin(&s, 0, 1024, 768, &mx, &my));
    TEST_ASSERT_EQUAL(0, edid_overscan_margin(0, 0, 1024, 768, 0, 0));
    tv_sum(&s, 16, 0);
    TEST_ASSERT_EQUAL(0, edid_overscan_margin(&s, 0, 0, 768, &mx, &my));
}

static int inside(const gfx_rect_t *r, const gfx_rect_t *s) {
    return r->w <= 0 || (r->x >= s->x && r->y >= s->y && r->x + r->w <= s->x + s->w && r->y + r->h <= s->y + s->h);
}

static void test_safe_area_keeps_windows_in(void) {
    gfx_desktop_layout_t l;
    gfx_rect_t sa, f;
    int i;
    gfx_desktop_set_safe(26, 19);
    gfx_desktop_safe_rect(W, H, &sa);
    TEST_ASSERT_EQUAL(26, sa.x);
    TEST_ASSERT_EQUAL(19, sa.y);
    TEST_ASSERT_EQUAL(W - 52, sa.w);
    TEST_ASSERT_EQUAL(H - 38, sa.h);
    scene(4, OS_FB_PANE_BROWSER);
    g_scene.app[0].kind = 1;
    strcpy(g_scene.app[0].title, "App");
    gfx_desktop_get_layout(&g_scene, W, H, &l);
    TEST_ASSERT_TRUE(l.pane_win.box.w > 0);
    TEST_ASSERT_TRUE(inside(&l.pane_win.box, &sa));
    TEST_ASSERT_TRUE(inside(&l.chat_win.box, &sa));
    TEST_ASSERT_TRUE(inside(&l.app_win[0].box, &sa));
    TEST_ASSERT_TRUE(inside(&l.dock.box, &sa));
    for (i = 0; i < GFX_DESKTOP_DOCK; i++) TEST_ASSERT_TRUE(inside(&l.dock.icons[i], &sa));
    scene(4, OS_FB_PANE_WEB);
    gfx_desktop_get_layout(&g_scene, W, H, &l);
    gfx_desktop_full_rect(W, H, &f);
    TEST_ASSERT_TRUE(inside(&f, &sa));
    /* small screens: the default windows fill the area, so the clip shows */
    {
        static const int sz[3][2] = { { 640, 480 }, { 800, 600 }, { 720, 400 } };
        int k, pane;
        for (k = 0; k < 3; k++) {
            gfx_desktop_set_safe(sz[k][0] * 25 / 1000, sz[k][1] * 25 / 1000);
            gfx_desktop_safe_rect(sz[k][0], sz[k][1], &sa);
            for (pane = OS_FB_PANE_BROWSER; pane <= OS_FB_PANE_PROD; pane++) {
                scene(4, pane);
                g_scene.app[0].kind = 1;
                gfx_desktop_get_layout(&g_scene, sz[k][0], sz[k][1], &l);
                TEST_ASSERT_TRUE(inside(&l.pane_win.box, &sa));
                TEST_ASSERT_TRUE(inside(&l.chat_win.box, &sa));
                TEST_ASSERT_TRUE(inside(&l.app_win[0].box, &sa));
            }
        }
        gfx_desktop_set_safe(26, 19);
    }
    gfx_desktop_set_safe(0, 0);
    gfx_desktop_get_layout(&g_scene, W, H, &l);
    gfx_desktop_full_rect(W, H, &f);
    TEST_ASSERT_EQUAL(8, f.x);
}

static void test_safe_area_dock_moves_up(void) {
    gfx_desktop_layout_t l;
    int y0;
    scene(4, OS_FB_PANE_NONE);
    gfx_desktop_set_safe(0, 0);
    gfx_desktop_get_layout(&g_scene, W, H, &l);
    y0 = l.dock.box.y;
    TEST_ASSERT_EQUAL(H - 48, y0);
    gfx_desktop_set_safe(26, 19);
    gfx_desktop_get_layout(&g_scene, W, H, &l);
    TEST_ASSERT_EQUAL(y0 - 19, l.dock.box.y);
    TEST_ASSERT_EQUAL(l.dock.box.y + 4, l.dock.icons[0].y);
    gfx_desktop_set_safe(0, 0);
}

static void test_safe_area_band_painted(void) {
    uint32_t c_edge, c_in, c_line;
    scene(4, OS_FB_PANE_NONE);
    gfx_desktop_set_safe(0, 0);
    gfx_desktop_draw_no_cursor(&g_scene, g_fb, W, H);
    c_edge = gfx_desktop_pixel(g_fb, W, H, 5, H / 2);
    gfx_desktop_set_safe(26, 19);
    gfx_desktop_draw_no_cursor(&g_scene, g_fb, W, H);
    c_in = gfx_desktop_pixel(g_fb, W, H, 5, H / 2);
    c_line = gfx_desktop_pixel(g_fb, W, H, 26, H / 2);
    /* the band is darker than the wallpaper it covers */
    TEST_ASSERT_TRUE(((c_in >> 8) & 255u) < ((c_edge >> 8) & 255u));
    TEST_ASSERT_TRUE(((c_in >> 8) & 255u) * 2 <= ((c_edge >> 8) & 255u) + 2);
    /* and outlined in the accent colour */
    TEST_ASSERT_EQUAL(0x3D9A8Au, c_line & 0xFFFFFFu);
    gfx_desktop_set_safe(0, 0);
}

static void test_safe_area_bounds(void) {
    gfx_rect_t sa;
    gfx_desktop_set_safe(-5, 999);
    gfx_desktop_safe_rect(W, 1080, &sa);
    TEST_ASSERT_EQUAL(0, sa.x);
    TEST_ASSERT_EQUAL(200, sa.y);            /* clamped to 200 */
    TEST_ASSERT_EQUAL(680, sa.h);
    gfx_desktop_safe_rect(W, 600, &sa);      /* a margin of a quarter or more: ignored */
    TEST_ASSERT_EQUAL(0, sa.y);
    TEST_ASSERT_EQUAL(600, sa.h);
    gfx_desktop_set_safe(0, 0);
}

int main(void) {
    unity_init();
    RUN_TEST(test_title_h_formula);
    RUN_TEST(test_buttons_grow_with_the_bar);
    RUN_TEST(test_one_x_does_not_move);
    RUN_TEST(test_address_bar_moves_down);
    RUN_TEST(test_web_origin_moves_down);
    RUN_TEST(test_app_grid_moves_down);
    RUN_TEST(test_app_window_toolbar_under_bar);
    RUN_TEST(test_painted_band_follows);
    RUN_TEST(test_chat_band_follows);
    RUN_TEST(test_overscan_margin_rule);
    RUN_TEST(test_safe_area_keeps_windows_in);
    RUN_TEST(test_safe_area_dock_moves_up);
    RUN_TEST(test_safe_area_band_painted);
    RUN_TEST(test_safe_area_bounds);
    unity_print_results();
    unity_cleanup();
    return unity_stats.tests_failed == 0 ? 0 : 1;
}
