/* Shared window manager: button geometry, hit test, state machine. */
#include "../../framework/unity.h"
#include "gfx_wm.h"
#include <string.h>

static gfx_wm_rect_t R(int x, int y, int w, int h) { gfx_wm_rect_t r; r.x = x; r.y = y; r.w = w; r.h = h; return r; }

static void test_buttons_and_hit(void) {
    gfx_wm_rect_t win = R(100, 50, 400, 300);
    int b, bx, by, last = 1 << 30;
    for (b = GFX_WM_HIT_MIN; b <= GFX_WM_HIT_CLOSE; b++) {
        gfx_wm_button_pos(win, 30, 1, b, &bx, &by);
        TEST_ASSERT(bx + GFX_WM_BTN <= win.x + win.w);
        TEST_ASSERT(by >= win.y && by + GFX_WM_BTN <= win.y + 32);
        TEST_ASSERT_EQUAL(b, gfx_wm_hit(win, 30, 1, bx + 7, by + 7));
        if (b > GFX_WM_HIT_MIN) TEST_ASSERT(bx > last); /* close is the rightmost */
        last = bx;
    }
    TEST_ASSERT_EQUAL(GFX_WM_HIT_TITLE, gfx_wm_hit(win, 30, 1, 120, 60));
    TEST_ASSERT_EQUAL(GFX_WM_HIT_BODY, gfx_wm_hit(win, 30, 1, 120, 200));
    TEST_ASSERT_EQUAL(GFX_WM_HIT_NONE, gfx_wm_hit(win, 30, 1, 99, 60));
    TEST_ASSERT_EQUAL(GFX_WM_HIT_NONE, gfx_wm_hit(R(0, 0, 0, 0), 30, 1, 0, 0));
}

static void test_close_minimize_maximize_drag(void) {
    gfx_wm_win_t s;
    gfx_wm_rect_t base = R(100, 50, 400, 300), area = R(0, 30, 1024, 680), placed;
    int bx, by;
    memset(&s, 0, sizeof(s));
    gfx_wm_place(&s, base, area, &placed);
    TEST_ASSERT_EQUAL(100, placed.x);
    /* maximize covers the area, again restores */
    gfx_wm_button_pos(placed, 30, 1, GFX_WM_HIT_MAX, &bx, &by);
    TEST_ASSERT_EQUAL(GFX_WM_ACT_MAXIMIZE, gfx_wm_mouse(&s, placed, base, 30, 1, bx + 2, by + 2, 1, 0));
    gfx_wm_place(&s, base, area, &placed);
    TEST_ASSERT_EQUAL(0, placed.x); TEST_ASSERT_EQUAL(30, placed.y); TEST_ASSERT_EQUAL(1024, placed.w);
    gfx_wm_button_pos(placed, 30, 1, GFX_WM_HIT_MAX, &bx, &by);
    TEST_ASSERT_EQUAL(GFX_WM_ACT_MAXIMIZE, gfx_wm_mouse(&s, placed, base, 30, 1, bx + 2, by + 2, 1, 0));
    TEST_ASSERT_EQUAL(0, s.maximized);
    gfx_wm_place(&s, base, area, &placed);
    /* drag the title by (+50, +40) */
    TEST_ASSERT_EQUAL(GFX_WM_ACT_DRAG, gfx_wm_mouse(&s, placed, base, 30, 1, 150, 60, 1, 0));
    TEST_ASSERT_EQUAL(GFX_WM_ACT_MOVED, gfx_wm_mouse(&s, placed, base, 30, 1, 200, 100, 0, 1));
    gfx_wm_place(&s, base, area, &placed);
    TEST_ASSERT_EQUAL(150, placed.x); TEST_ASSERT_EQUAL(90, placed.y);
    /* dragged off screen: clamped inside the area */
    s.dx = 5000; s.dy = -5000;
    gfx_wm_place(&s, base, area, &placed);
    TEST_ASSERT_EQUAL(1024 - 400, placed.x); TEST_ASSERT_EQUAL(30, placed.y);
    /* minimize hides, restore shows at the same place */
    gfx_wm_button_pos(placed, 30, 1, GFX_WM_HIT_MIN, &bx, &by);
    TEST_ASSERT_EQUAL(GFX_WM_ACT_MINIMIZE, gfx_wm_mouse(&s, placed, base, 30, 1, bx + 1, by + 1, 1, 0));
    gfx_wm_place(&s, base, area, &placed);
    TEST_ASSERT_EQUAL(0, placed.w);
    TEST_ASSERT(!gfx_wm_visible(&s));
    gfx_wm_restore(&s);
    gfx_wm_place(&s, base, area, &placed);
    TEST_ASSERT_EQUAL(1024 - 400, placed.x);
    /* close is distinct: closed, not minimized, and maximize is dropped */
    s.maximized = 1;
    gfx_wm_place(&s, base, area, &placed);
    gfx_wm_button_pos(placed, 30, 1, GFX_WM_HIT_CLOSE, &bx, &by);
    TEST_ASSERT_EQUAL(GFX_WM_ACT_CLOSE, gfx_wm_mouse(&s, placed, base, 30, 1, bx + 1, by + 1, 1, 0));
    TEST_ASSERT(s.closed && !s.minimized && !s.maximized);
    /* body click and outside click */
    gfx_wm_reset(&s);
    gfx_wm_place(&s, base, area, &placed);
    TEST_ASSERT_EQUAL(GFX_WM_ACT_BODY, gfx_wm_mouse(&s, placed, base, 30, 1, 200, 200, 1, 0));
    TEST_ASSERT_EQUAL(GFX_WM_ACT_NONE, gfx_wm_mouse(&s, placed, base, 30, 1, 5, 5, 1, 0));
    /* no drag while maximized */
    s.maximized = 1;
    gfx_wm_place(&s, base, area, &placed);
    TEST_ASSERT_EQUAL(GFX_WM_ACT_BODY, gfx_wm_mouse(&s, placed, base, 30, 1, 40, 40, 1, 0));
    TEST_ASSERT_EQUAL(0, s.drag);
}

static void test_draw_buttons_colors(void) {
    static uint32_t fb[200 * 60];
    gfx_wm_rect_t win = R(0, 0, 200, 60);
    int bx, by;
    memset(fb, 0, sizeof(fb));
    gfx_wm_draw_buttons(fb, 200, 60, 200, win, 30, 1);
    gfx_wm_button_pos(win, 30, 1, GFX_WM_HIT_CLOSE, &bx, &by);
    TEST_ASSERT_EQUAL(GFX_WM_RGB_CLOSE, fb[(by + 1) * 200 + bx + 6]);
    gfx_wm_button_pos(win, 30, 1, GFX_WM_HIT_MIN, &bx, &by);
    TEST_ASSERT_EQUAL(GFX_WM_RGB_MIN, fb[(by + 1) * 200 + bx + 6]);
    gfx_wm_button_pos(win, 30, 1, GFX_WM_HIT_MAX, &bx, &by);
    TEST_ASSERT_EQUAL(GFX_WM_RGB_MAX, fb[(by + 1) * 200 + bx + 6]);
}

/* Lot 69: buttons follow the chrome scale; corner / edge resize */
static void test_lot69_scale_and_resize(void) {
    gfx_wm_win_t s;
    gfx_wm_rect_t base = R(100, 50, 400, 300), area = R(0, 30, 1024, 680), placed;
    int bx, by, n6 = gfx_wm_btn(36, 6);
    TEST_ASSERT_EQUAL(GFX_WM_BTN, gfx_wm_btn(36, 1));
    TEST_ASSERT_EQUAL(GFX_WM_BTN, gfx_wm_btn(36, 4));
    TEST_ASSERT_EQUAL(21, n6);                       /* 1.5x */
    TEST_ASSERT_EQUAL(30, gfx_wm_btn(36, 12));       /* capped by the title bar */
    gfx_wm_button_pos(base, 36, 6, GFX_WM_HIT_CLOSE, &bx, &by);
    TEST_ASSERT(bx + n6 <= base.x + base.w);
    TEST_ASSERT_EQUAL(GFX_WM_HIT_CLOSE, gfx_wm_hit(base, 36, 6, bx + n6 - 2, by + n6 - 2));
    /* grip and edges */
    TEST_ASSERT_EQUAL(GFX_WM_HIT_RESIZE, gfx_wm_hit(base, 36, 6, 498, 348));
    TEST_ASSERT_EQUAL(GFX_WM_HIT_RESIZE, gfx_wm_hit(base, 36, 6, 498, 200));
    TEST_ASSERT_EQUAL(GFX_WM_HIT_RESIZE, gfx_wm_hit(base, 36, 6, 300, 348));
    TEST_ASSERT_EQUAL(GFX_WM_HIT_BODY, gfx_wm_hit(base, 36, 6, 300, 200));
    memset(&s, 0, sizeof(s));
    gfx_wm_place(&s, base, area, &placed);
    TEST_ASSERT_EQUAL(GFX_WM_ACT_DRAG, gfx_wm_mouse(&s, placed, base, 36, 6, 498, 348, 1, 0));
    TEST_ASSERT_EQUAL(1, s.resize);
    TEST_ASSERT_EQUAL(GFX_WM_ACT_RESIZED, gfx_wm_mouse(&s, placed, base, 36, 6, 598, 398, 0, 1));
    gfx_wm_place(&s, base, area, &placed);
    TEST_ASSERT_EQUAL(100, placed.x); TEST_ASSERT_EQUAL(500, placed.w); TEST_ASSERT_EQUAL(350, placed.h);
    /* shrink is bounded by the minimum size */
    TEST_ASSERT_EQUAL(GFX_WM_ACT_DRAG, gfx_wm_mouse(&s, placed, base, 36, 6, 598, 398, 1, 0));
    TEST_ASSERT_EQUAL(GFX_WM_ACT_RESIZED, gfx_wm_mouse(&s, placed, base, 36, 6, 0, 0, 0, 1));
    gfx_wm_place(&s, base, area, &placed);
    TEST_ASSERT_EQUAL(GFX_WM_MIN_W, placed.w); TEST_ASSERT_EQUAL(GFX_WM_MIN_H, placed.h);
    /* growth is clamped to the work area */
    TEST_ASSERT_EQUAL(GFX_WM_ACT_DRAG, gfx_wm_mouse(&s, placed, base, 36, 6, placed.x + placed.w - 2, placed.y + placed.h - 2, 1, 0));
    TEST_ASSERT_EQUAL(GFX_WM_ACT_RESIZED, gfx_wm_mouse(&s, placed, base, 36, 6, 5000, 5000, 0, 1));
    gfx_wm_place(&s, base, area, &placed);
    TEST_ASSERT(placed.x + placed.w <= area.x + area.w);
    TEST_ASSERT(placed.y + placed.h <= area.y + area.h);
    /* maximized: no resize */
    s.maximized = 1;
    gfx_wm_place(&s, base, area, &placed);
    TEST_ASSERT_EQUAL(GFX_WM_ACT_BODY, gfx_wm_mouse(&s, placed, base, 36, 6, placed.x + placed.w - 2, placed.y + placed.h - 2, 1, 0));
    TEST_ASSERT_EQUAL(0, s.resize);
}

int main(void) {
    unity_init();
    RUN_TEST(test_buttons_and_hit);
    RUN_TEST(test_close_minimize_maximize_drag);
    RUN_TEST(test_draw_buttons_colors);
    RUN_TEST(test_lot69_scale_and_resize);
    unity_print_results();
    unity_cleanup();
    return (unity_stats.tests_failed == 0) ? 0 : 1;
}
