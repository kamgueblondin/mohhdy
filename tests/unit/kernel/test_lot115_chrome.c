/* Lot 115: window titles and desktop labels at every text scale.
 *
 * At 1.5x (the default on 1024x768) and 2x the title buttons grow with
 * the bar (Lot 90), but the titles kept their fixed widths (rw - 80,
 * rw - 110...): a long app or page title ran up to, or under, the
 * minimize button, and was cut in the middle of a glyph run without a
 * mark. The desktop icon labels were centred under their tiles at the
 * text scale, so at 2x "Browser-OS" and the others left the screen on the
 * right. Now a title stops GFX_WM_TITLE_GAP pixels before the minimize
 * button (gfx_wm_title_room), a cut title ends with "..." (at a word when
 * one ends in its last third), and a label is moved back inside the
 * screen (gfx_desktop_label_x). The checks below render the real desktop
 * at 1x .. 2x (and 3x on a wide screen) and look at the pixels. */
#include "../../framework/unity.h"
#include "../../../include/os_ui_scale.h"
#include <string.h>
#include "../../../kernel/gfx_desktop.h"
#include "../../../kernel/gfx_fb.h"
#include "../../../kernel/gfx_wm.h"

#define MAXW 1920
#define MAXH 1080
#define TITLE_H 30

static uint32_t g_fb[MAXW * MAXH];
static os_fb_scene_t g_scene;
static const char k_long[] = "Quarterly report of the northern region with every appendix and the review notes";

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

static int title_ink(uint32_t c) {
    return ((c >> 16) & 255u) == 232 && ((c >> 8) & 255u) == 238 && (c & 255u) == 244;
}

/* title ink in the rows of a title bar between x0 and x1 */
static int ink(int w, int h, int x0, int x1, int y0, int y1) {
    int x, y, n = 0;
    for (y = y0; y < y1; y++)
        for (x = x0; x < x1; x++)
            if (title_ink(gfx_desktop_pixel(g_fb, w, h, x, y))) n++;
    return n;
}

/* the rightmost column holding title ink in the rows y0..y1 */
static int ink_right(int w, int h, int x0, int x1, int y0, int y1) {
    int x, y;
    for (x = x1 - 1; x >= x0; x--)
        for (y = y0; y < y1; y++)
            if (title_ink(gfx_desktop_pixel(g_fb, w, h, x, y))) return x;
    return -1;
}

static void test_title_fit(void) {
    int kb;
    TEST_ASSERT_EQUAL(0, gfx_desktop_title_fit("Shell", 10, &kb));
    TEST_ASSERT_EQUAL(5, kb);
    TEST_ASSERT_EQUAL(0, gfx_desktop_title_fit("exactly10c", 10, &kb));
    TEST_ASSERT_EQUAL(10, kb);
    /* cut: maxc - 3 cells, back to the last word end in the last third */
    TEST_ASSERT_EQUAL(1, gfx_desktop_title_fit("one two three four", 12, &kb));
    TEST_ASSERT_EQUAL(7, kb);                       /* "one two" + "..." */
    TEST_ASSERT_EQUAL(1, gfx_desktop_title_fit("abcdefghijklmnop", 10, &kb));
    TEST_ASSERT_EQUAL(7, kb);                       /* no space: 7 cells */
    TEST_ASSERT_EQUAL(1, gfx_desktop_title_fit("abc defghijklmnop", 10, &kb));
    TEST_ASSERT_EQUAL(7, kb);                       /* the space is too early */
    TEST_ASSERT_EQUAL(1, gfx_desktop_title_fit("abcdef", 3, &kb));
    TEST_ASSERT_EQUAL(0, kb);
    TEST_ASSERT_EQUAL(0, gfx_desktop_title_fit("", 0, &kb));
    TEST_ASSERT_EQUAL(1, gfx_desktop_title_fit("x", 0, &kb));
    TEST_ASSERT_EQUAL(0, kb);
}

static void test_title_room(void) {
    gfx_wm_rect_t r;
    int q;
    r.x = 100; r.y = 50; r.w = 600; r.h = 400;
    for (q = 4; q <= 8; q++) {
        int th = gfx_wm_title_h(TITLE_H, q), bx, room = gfx_wm_title_room(r, th, q, r.x + 14);
        gfx_wm_button_pos(r, th, q, GFX_WM_HIT_MIN, &bx, 0);
        TEST_ASSERT_EQUAL(bx - GFX_WM_TITLE_GAP - (r.x + 14), room);
        TEST_ASSERT_TRUE(room < 600 - 14);
    }
    /* the fixed widths used before were wider than the room at 1.5x and 2x */
    TEST_ASSERT_TRUE(gfx_wm_title_room(r, gfx_wm_title_h(TITLE_H, 6), 6, r.x + 14) < 600 - 80);
    TEST_ASSERT_TRUE(gfx_wm_title_room(r, gfx_wm_title_h(TITLE_H, 8), 8, r.x + 14) < 600 - 110);
    r.w = 0;
    TEST_ASSERT_EQUAL(0, gfx_wm_title_room(r, TITLE_H, 4, 10));
    r.w = 60;
    TEST_ASSERT_EQUAL(0, gfx_wm_title_room(r, TITLE_H, 8, r.x + 14));
}

/* an app window with a long title: the ink stops before the gap at every
 * scale, and the title still fills most of its room */
static void test_app_title_clear_of_buttons(void) {
    gfx_desktop_layout_t l;
    int q;
    for (q = 4; q <= 8; q++) {
        const gfx_rect_t *b, *bmin;
        int th, right;
        scene(q, OS_FB_PANE_NONE);
        g_scene.app[0].kind = 1;
        memcpy(g_scene.app[0].title, k_long, sizeof(g_scene.app[0].title) - 1);
        gfx_desktop_draw_no_cursor(&g_scene, g_fb, 1024, 768);
        gfx_desktop_get_layout(&g_scene, 1024, 768, &l);
        TEST_ASSERT_TRUE(l.app_win[0].visible);
        b = &l.app_win[0].box;
        bmin = &l.app_win[0].btn[0];
        th = gfx_wm_title_h(GFX_DESKTOP_APP_TITLE_H, q);
        right = ink_right(1024, 768, b->x, b->x + b->w, b->y, b->y + th);
        TEST_ASSERT_TRUE(right > b->x + 14);
        TEST_ASSERT_TRUE(right < bmin->x - GFX_WM_TITLE_GAP);
        TEST_ASSERT_EQUAL(0, ink(1024, 768, bmin->x - GFX_WM_TITLE_GAP, b->x + b->w, b->y, b->y + th));
        /* most of the room is used: the cut is the ellipsis, not a margin */
        TEST_ASSERT_TRUE(right > b->x + 14 + (bmin->x - GFX_WM_TITLE_GAP - b->x - 14) / 2);
    }
}

/* a short title is drawn whole, with no ellipsis */
static void test_short_title_whole(void) {
    gfx_desktop_layout_t l;
    int q;
    for (q = 4; q <= 8; q += 2) {
        const gfx_rect_t *b;
        int th, cw = os_ui_char_w(q), x0, r5, r8;
        scene(q, OS_FB_PANE_NONE);
        g_scene.app[0].kind = 1;
        memcpy(g_scene.app[0].title, "Notes", 6);
        gfx_desktop_draw_no_cursor(&g_scene, g_fb, 1024, 768);
        gfx_desktop_get_layout(&g_scene, 1024, 768, &l);
        b = &l.app_win[0].box;
        th = gfx_wm_title_h(GFX_DESKTOP_APP_TITLE_H, q);
        x0 = b->x + 14;
        r5 = ink_right(1024, 768, b->x, b->x + b->w, b->y, b->y + th);
        memcpy(g_scene.app[0].title, "Notes...", 9);
        gfx_desktop_draw_no_cursor(&g_scene, g_fb, 1024, 768);
        r8 = ink_right(1024, 768, b->x, b->x + b->w, b->y, b->y + th);
        /* five cells and nothing after them; the same title with dots is
         * three cells longer, so "Notes" alone got none */
        TEST_ASSERT_TRUE(r5 > x0 + 4 * cw && r5 < x0 + 5 * cw);
        TEST_ASSERT_TRUE(r8 > x0 + 7 * cw && r8 < x0 + 8 * cw);
    }
}

/* the full screen web window: a long page title stops before the buttons */
static void test_web_title_clear_of_buttons(void) {
    int q;
    for (q = 4; q <= 8; q++) {
        gfx_rect_t f;
        gfx_wm_rect_t r;
        int th, bx;
        scene(q, OS_FB_PANE_WEB);
        memcpy(g_scene.web_title, k_long, sizeof(k_long));
        gfx_desktop_draw_no_cursor(&g_scene, g_fb, 1024, 768);
        gfx_desktop_full_rect(1024, 768, &f);
        r.x = f.x; r.y = f.y; r.w = f.w; r.h = f.h;
        th = gfx_wm_title_h(30, q);
        gfx_wm_button_pos(r, th, q, GFX_WM_HIT_MIN, &bx, 0);
        TEST_ASSERT_TRUE(ink(1024, 768, f.x, bx - GFX_WM_TITLE_GAP, f.y, f.y + th) > 0);
        TEST_ASSERT_EQUAL(0, ink(1024, 768, bx - GFX_WM_TITLE_GAP, f.x + f.w, f.y, f.y + th));
    }
}

static void test_label_x(void) {
    TEST_ASSERT_EQUAL(400, gfx_desktop_label_x(1024, 450, 100));                /* centred */
    TEST_ASSERT_EQUAL(1024 - 4 - 160, gfx_desktop_label_x(1024, 959, 160));     /* pulled in */
    TEST_ASSERT_EQUAL(4, gfx_desktop_label_x(1024, 10, 100));                   /* left edge */
    TEST_ASSERT_EQUAL(4, gfx_desktop_label_x(100, 50, 200));                    /* too wide: from the left */
}

/* the desktop labels stay on the screen: no label ink in the last
 * GFX_LABEL_MARGIN columns, every label drawn (ink on each tile row) */
static void desk_labels(int w, int h, int q) {
    gfx_desktop_layout_t l;
    int i;
    scene(q, OS_FB_PANE_NONE);
    g_scene.chat_mode = OS_FB_CHAT_CENTER;      /* nothing over the icons */
    gfx_desktop_draw_no_cursor(&g_scene, g_fb, w, h);
    gfx_desktop_get_layout(&g_scene, w, h, &l);
    TEST_ASSERT_TRUE(l.desk.count >= 3);
    for (i = 0; i < l.desk.count; i++) {
        const gfx_rect_t *ic = &l.desk.icons[i];
        int cq = q > OS_UI_Q_CHROME_MAX ? OS_UI_Q_CHROME_MAX : q;
        int y0 = ic->y + 46, y1 = ic->y + 46 + os_ui_px(8, cq);
        /* the hit box holds the tile and its label (Lot 115: the rows the
         * layout reports are the rows drawn) */
        TEST_ASSERT_EQUAL(50 + os_ui_px(8, cq), ic->h);
        TEST_ASSERT_EQUAL(0, ink(w, h, w - GFX_LABEL_MARGIN, w, y0, y1));
        TEST_ASSERT_TRUE(ink(w, h, w - 300, w - GFX_LABEL_MARGIN, y0, y1) > 0);
    }
}

static void test_desk_labels_on_screen(void) {
    int q;
    for (q = 4; q <= 8; q++) desk_labels(1024, 768, q);
    desk_labels(1920, 1080, 8);
    desk_labels(1920, 1080, 12);
}

int main(void) {
    unity_init();
    RUN_TEST(test_title_fit);
    RUN_TEST(test_title_room);
    RUN_TEST(test_app_title_clear_of_buttons);
    RUN_TEST(test_short_title_whole);
    RUN_TEST(test_web_title_clear_of_buttons);
    RUN_TEST(test_label_x);
    RUN_TEST(test_desk_labels_on_screen);
    unity_print_results();
    unity_cleanup();
    return unity_stats.tests_failed == 0 ? 0 : 1;
}
