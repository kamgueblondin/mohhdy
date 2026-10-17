/* Lot 8 (US-077): mobile shell layout, touch targets, navigation, gestures. */
#include "../../framework/unity.h"
#include "../../../userspace/mobile_ui.h"

void setUp(void) {}
void tearDown(void) {}

static int center_x(const mui_layout_t* l, int id) {
    int i;
    for (i = 0; i < l->ntargets; i++) if (l->t[i].id == id) return l->t[i].x + l->t[i].w / 2;
    return -1;
}
static int center_y(const mui_layout_t* l, int id) {
    int i;
    for (i = 0; i < l->ntargets; i++) if (l->t[i].id == id) return l->t[i].y + l->t[i].h / 2;
    return -1;
}

void test_phone_portrait_layout_keeps_touch_targets(void) {
    mui_layout_t l;
    int i, j;
    TEST_ASSERT_EQUAL(0, mui_layout(412, 732, &l));
    TEST_ASSERT_EQUAL(PLAT_FORM_PHONE, l.form);
    TEST_ASSERT_EQUAL(1, l.portrait);
    TEST_ASSERT_EQUAL(4, l.cols);
    TEST_ASSERT_EQUAL(2, l.rows);
    TEST_ASSERT_EQUAL(MUI_APPS + 3, l.ntargets);
    TEST_ASSERT_TRUE(l.min_target >= MUI_MIN_TARGET);
    for (i = 0; i < l.ntargets; i++) {        /* inside the screen, no overlap */
        TEST_ASSERT_TRUE(l.t[i].x >= 0 && l.t[i].y >= 0);
        TEST_ASSERT_TRUE(l.t[i].x + l.t[i].w <= l.w && l.t[i].y + l.t[i].h <= l.h);
        for (j = i + 1; j < l.ntargets; j++) {
            int ox = l.t[i].x < l.t[j].x + l.t[j].w && l.t[j].x < l.t[i].x + l.t[i].w;
            int oy = l.t[i].y < l.t[j].y + l.t[j].h && l.t[j].y < l.t[i].y + l.t[i].h;
            TEST_ASSERT_FALSE(ox && oy);
        }
    }
}

void test_landscape_tablet_and_small_screens(void) {
    mui_layout_t l;
    TEST_ASSERT_EQUAL(0, mui_layout(732, 412, &l));
    TEST_ASSERT_EQUAL(6, l.cols);
    TEST_ASSERT_EQUAL(0, l.portrait);
    TEST_ASSERT_EQUAL(0, mui_layout(800, 1280, &l));
    TEST_ASSERT_EQUAL(PLAT_FORM_TABLET, l.form);
    TEST_ASSERT_EQUAL(5, l.cols);
    TEST_ASSERT_TRUE(l.min_target >= MUI_MIN_TARGET);
    TEST_ASSERT_EQUAL(0, mui_layout(240, 320, &l));          /* columns reduced */
    TEST_ASSERT_TRUE(l.cols < 4);
    TEST_ASSERT_TRUE(l.min_target >= MUI_MIN_TARGET);
    TEST_ASSERT_EQUAL(-1, mui_layout(160, 120, &l));         /* no room for targets */
    TEST_ASSERT_EQUAL(-1, mui_layout(100, 100, &l));
}

void test_tap_open_back_home_recents(void) {
    mui_layout_t l;
    mui_state_t s;
    mui_layout(412, 732, &l);
    mui_init(&s);
    TEST_ASSERT_EQUAL(MUI_ACT_NONE, mui_tap(&s, &l, 2, 2));  /* status bar */
    TEST_ASSERT_EQUAL(MUI_ACT_OPEN, mui_tap(&s, &l, center_x(&l, 1), center_y(&l, 1)));
    TEST_ASSERT_EQUAL(MUI_SCREEN_APP, s.screen);
    TEST_ASSERT_EQUAL(1, s.app);
    /* icons are not live inside an app */
    TEST_ASSERT_EQUAL(MUI_ACT_NONE, mui_tap(&s, &l, center_x(&l, 2), center_y(&l, 2)));
    TEST_ASSERT_EQUAL(MUI_ACT_HOME, mui_tap(&s, &l, center_x(&l, MUI_NAV_HOME), center_y(&l, MUI_NAV_HOME)));
    TEST_ASSERT_EQUAL(MUI_ACT_OPEN, mui_tap(&s, &l, center_x(&l, 2), center_y(&l, 2)));
    TEST_ASSERT_EQUAL(2, s.app);
    TEST_ASSERT_EQUAL(MUI_ACT_RECENTS, mui_tap(&s, &l, center_x(&l, MUI_NAV_RECENTS), center_y(&l, MUI_NAV_RECENTS)));
    TEST_ASSERT_EQUAL(MUI_SCREEN_RECENTS, s.screen);
    TEST_ASSERT_EQUAL(MUI_ACT_BACK, mui_tap(&s, &l, center_x(&l, MUI_NAV_BACK), center_y(&l, MUI_NAV_BACK)));
    TEST_ASSERT_EQUAL(MUI_SCREEN_APP, s.screen);
    TEST_ASSERT_EQUAL(MUI_ACT_BACK, mui_tap(&s, &l, center_x(&l, MUI_NAV_BACK), center_y(&l, MUI_NAV_BACK)));
    TEST_ASSERT_EQUAL(MUI_SCREEN_HOME, s.screen);
    TEST_ASSERT_EQUAL(-1, s.app);
    TEST_ASSERT_TRUE(s.opens == 2U);
}

static void swipe(plat_point_t* p, int x0, int y0, int x1, int y1) {
    p[0].x = x0; p[0].y = y0; p[0].t_ms = 0U; p[0].down = 1;
    p[1].x = (x0 + x1) / 2; p[1].y = (y0 + y1) / 2; p[1].t_ms = 80U; p[1].down = 1;
    p[2].x = x1; p[2].y = y1; p[2].t_ms = 160U; p[2].down = 1;
    p[3].x = x1; p[3].y = y1; p[3].t_ms = 170U; p[3].down = 0;
}

void test_gestures_shade_home_back(void) {
    mui_layout_t l;
    mui_state_t s;
    plat_point_t p[4];
    mui_layout(412, 732, &l);
    mui_init(&s);
    mui_tap(&s, &l, center_x(&l, 0), center_y(&l, 0));
    swipe(p, 200, 5, 200, 300);                               /* from the status bar */
    TEST_ASSERT_EQUAL(MUI_ACT_SHADE_OPEN, mui_gesture(&s, &l, p, 4));
    TEST_ASSERT_EQUAL(1, s.shade);
    swipe(p, 200, 300, 200, 100);
    TEST_ASSERT_EQUAL(MUI_ACT_SHADE_CLOSE, mui_gesture(&s, &l, p, 4));
    swipe(p, 200, 400, 200, 600);                             /* not from the status bar */
    TEST_ASSERT_EQUAL(MUI_ACT_NONE, mui_gesture(&s, &l, p, 4));
    swipe(p, 4, 300, 200, 300);                               /* edge swipe = back */
    TEST_ASSERT_EQUAL(MUI_ACT_BACK, mui_gesture(&s, &l, p, 4));
    TEST_ASSERT_EQUAL(MUI_SCREEN_HOME, s.screen);
    mui_tap(&s, &l, center_x(&l, 3), center_y(&l, 3));
    swipe(p, 200, 720, 200, 400);                             /* from the nav bar = home */
    TEST_ASSERT_EQUAL(MUI_ACT_HOME, mui_gesture(&s, &l, p, 4));
    TEST_ASSERT_EQUAL(MUI_SCREEN_HOME, s.screen);
}

void test_pixels_follow_state(void) {
    mui_layout_t l;
    mui_state_t s;
    uint32_t home, app;
    int ix, iy;
    mui_layout(412, 732, &l);
    mui_init(&s);
    ix = center_x(&l, 4); iy = center_y(&l, 4);
    TEST_ASSERT_TRUE(mui_pixel(&l, &s, ix, iy) == 0x6A1B9AU);          /* p2p icon */
    TEST_ASSERT_TRUE(mui_pixel(&l, &s, 1, 1) == 0x101010U);            /* status bar */
    home = mui_frame_hash(&l, &s);
    TEST_ASSERT_TRUE(home == mui_frame_hash(&l, &s));                  /* deterministic */
    mui_tap(&s, &l, ix, iy);
    TEST_ASSERT_TRUE(mui_pixel(&l, &s, 200, l.status_h + 10) == 0x6A1B9AU); /* app bar */
    TEST_ASSERT_TRUE(mui_pixel(&l, &s, ix, iy) == 0xFAFAFAU);
    app = mui_frame_hash(&l, &s);
    TEST_ASSERT_TRUE(app != home);
    TEST_ASSERT_TRUE(mui_pixel(&l, &s, -1, 0) == 0U);
}

int main(void) {
    unity_init();
    RUN_TEST(test_phone_portrait_layout_keeps_touch_targets);
    RUN_TEST(test_landscape_tablet_and_small_screens);
    RUN_TEST(test_tap_open_back_home_recents);
    RUN_TEST(test_gestures_shade_home_back);
    RUN_TEST(test_pixels_follow_state);
    unity_print_results();
    unity_cleanup();
    return unity_stats.tests_failed == 0 ? 0 : 1;
}
