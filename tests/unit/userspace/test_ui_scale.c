/* Lot 68: text / UI scale (include/os_ui_scale.h, userspace/osui_scale.c). */
#include "../../framework/unity.h"
#include "../../../userspace/osui_scale.h"
#include <string.h>

static void test_auto_scale_from_screen(void) {
    TEST_ASSERT_EQUAL(4, os_ui_auto_q(640, 480));
    TEST_ASSERT_EQUAL(4, os_ui_auto_q(1024, 768));
    TEST_ASSERT_EQUAL(6, os_ui_auto_q(1920, 1080));
    TEST_ASSERT_EQUAL(8, os_ui_auto_q(2560, 1440));
    TEST_ASSERT_EQUAL(12, os_ui_auto_q(3840, 2160));
    /* portrait: the shorter side decides */
    TEST_ASSERT_EQUAL(6, os_ui_auto_q(1080, 1920));
}

static void test_parse_and_format(void) {
    char b[16];
    TEST_ASSERT_EQUAL(0, os_ui_q_parse("auto"));
    TEST_ASSERT_EQUAL(4, os_ui_q_parse("1"));
    TEST_ASSERT_EQUAL(6, os_ui_q_parse("1.5"));
    TEST_ASSERT_EQUAL(8, os_ui_q_parse("2x"));
    TEST_ASSERT_EQUAL(12, os_ui_q_parse("3"));
    TEST_ASSERT_EQUAL(6, os_ui_q_parse("150%"));
    TEST_ASSERT_TRUE(os_ui_q_parse("abc") < 0);
    TEST_ASSERT_TRUE(os_ui_q_parse("9") < 0);
    os_ui_q_format(6, b); TEST_ASSERT_EQUAL_STRING("1.5x", b);
    os_ui_q_format(8, b); TEST_ASSERT_EQUAL_STRING("2x", b);
    os_ui_q_format(4, b); TEST_ASSERT_EQUAL_STRING("1x", b);
}

static void test_metrics_and_steps(void) {
    TEST_ASSERT_EQUAL(8, os_ui_char_w(4));
    TEST_ASSERT_EQUAL(12, os_ui_char_w(6));
    TEST_ASSERT_EQUAL(16, os_ui_char_w(8));
    TEST_ASSERT_EQUAL(24, os_ui_char_w(12));
    TEST_ASSERT_EQUAL(10, os_ui_line_h(4));
    TEST_ASSERT_EQUAL(20, os_ui_line_h(8));
    TEST_ASSERT_EQUAL(30, os_ui_line_h(12));
    TEST_ASSERT_EQUAL(5, os_ui_q_step(4, 1));
    TEST_ASSERT_EQUAL(4, os_ui_q_step(4, -1));    /* bounded below */
    TEST_ASSERT_EQUAL(16, os_ui_q_step(16, 1));   /* bounded above */
    TEST_ASSERT_EQUAL(10, os_ui_q_step(8, 1));
    /* effective: global x window zoom, clamped */
    TEST_ASSERT_EQUAL(8, os_ui_effective_q(8, 4, 1024, 768));
    TEST_ASSERT_EQUAL(12, os_ui_effective_q(8, 6, 1024, 768));
    TEST_ASSERT_EQUAL(16, os_ui_effective_q(12, 8, 1024, 768));
    TEST_ASSERT_EQUAL(6, os_ui_effective_q(0, 4, 1920, 1080));
}

static void test_state_zoom_and_viewport(void) {
    osui_scale_t s;
    int i;
    osui_scale_init(&s);
    TEST_ASSERT_EQUAL(0, s.ui_q);
    for (i = 0; i < OS_UI_Z_COUNT; i++) TEST_ASSERT_EQUAL(4, s.zoom[i]);
    TEST_ASSERT_EQUAL(4, osui_scale_effective(&s, OS_UI_Z_WEB));
    TEST_ASSERT_EQUAL(5, osui_scale_zoom(&s, OS_UI_Z_WEB, 1));
    TEST_ASSERT_EQUAL(6, osui_scale_zoom(&s, OS_UI_Z_WEB, 1));
    TEST_ASSERT_EQUAL(5, osui_scale_zoom(&s, OS_UI_Z_WEB, -1));
    TEST_ASSERT_EQUAL(4, osui_scale_zoom(&s, OS_UI_Z_WEB, 0));
    TEST_ASSERT_EQUAL(4, s.zoom[OS_UI_Z_CHAT]);
    TEST_ASSERT_TRUE(s.changes >= 4U);
    TEST_ASSERT_EQUAL(0, osui_scale_set_ui(&s, 8));
    TEST_ASSERT_EQUAL(-1, osui_scale_set_ui(&s, 99));
    TEST_ASSERT_EQUAL(8, osui_scale_effective(&s, OS_UI_Z_CHAT));
    osui_scale_web_view(&s, 992, 700, 8, 1024, 768);
    TEST_ASSERT_EQUAL(496, osui_scale_web_viewport(&s, 640));
    osui_scale_web_view(&s, 992, 700, 16, 1024, 768);
    TEST_ASSERT_EQUAL(248, osui_scale_web_viewport(&s, 640));
}

static void test_slot_names(void) {
    TEST_ASSERT_EQUAL(OS_UI_Z_WEB, osui_scale_slot("web"));
    TEST_ASSERT_EQUAL(OS_UI_Z_WEB, osui_scale_slot("browser"));
    TEST_ASSERT_EQUAL(OS_UI_Z_SHELL, osui_scale_slot("terminal"));
    TEST_ASSERT_EQUAL(OS_UI_Z_IDE, osui_scale_slot("editor"));
    TEST_ASSERT_EQUAL(OS_UI_Z_APP0 + 2, osui_scale_slot("app2"));
    TEST_ASSERT_EQUAL(-1, osui_scale_slot("nope"));
    TEST_ASSERT_EQUAL_STRING("chat", osui_scale_slot_name(OS_UI_Z_CHAT));
}

static void test_save_load_roundtrip(void) {
    osui_scale_t a, b;
    char buf[160];
    int n;
    osui_scale_init(&a);
    (void)osui_scale_set_ui(&a, 6);
    (void)osui_scale_zoom(&a, OS_UI_Z_IDE, 1);
    (void)osui_scale_zoom(&a, OS_UI_Z_IDE, 1);
    n = osui_scale_save(&a, buf, (int)sizeof(buf));
    TEST_ASSERT_TRUE(n > 0);
    TEST_ASSERT_TRUE(strstr(buf, "ui_scale=6") != 0);
    osui_scale_init(&b);
    TEST_ASSERT_EQUAL(0, osui_scale_load(&b, buf, n));
    TEST_ASSERT_EQUAL(6, b.ui_q);
    TEST_ASSERT_EQUAL(a.zoom[OS_UI_Z_IDE], b.zoom[OS_UI_Z_IDE]);
    TEST_ASSERT_TRUE(osui_scale_effective(&b, OS_UI_Z_IDE) > 6);
    TEST_ASSERT_EQUAL(4, b.zoom[OS_UI_Z_WEB]);
    /* garbage keeps defaults and is refused */
    osui_scale_init(&b);
    TEST_ASSERT_EQUAL(-1, osui_scale_load(&b, "ui_scale=77", 11));
    TEST_ASSERT_EQUAL(0, b.ui_q);
    /* tiny buffer: no overflow */
    TEST_ASSERT_TRUE(osui_scale_save(&a, buf, 4) <= 0);
}

static void test_commands(void) {
    osui_scale_t s;
    char out[256];
    osui_scale_init(&s);
    TEST_ASSERT_EQUAL(0, osui_scale_command(&s, "ui-scale 2", out, (int)sizeof(out)));
    TEST_ASSERT_TRUE(strstr(out, "ui-scale ok") != 0);
    TEST_ASSERT_TRUE(strstr(out, "char 16px") != 0);
    TEST_ASSERT_EQUAL(8, s.ui_q);
    TEST_ASSERT_EQUAL(0, osui_scale_command(&s, "ui-scale +", out, (int)sizeof(out)));
    TEST_ASSERT_EQUAL(10, s.ui_q);
    TEST_ASSERT_EQUAL(0, osui_scale_command(&s, "ui-scale auto", out, (int)sizeof(out)));
    TEST_ASSERT_EQUAL(0, s.ui_q);
    TEST_ASSERT_EQUAL(0, osui_scale_command(&s, "ui-scale", out, (int)sizeof(out)));
    TEST_ASSERT_EQUAL(1, osui_scale_command(&s, "ui-scale banana", out, (int)sizeof(out)));
    TEST_ASSERT_EQUAL(0, osui_scale_command(&s, "text-size web 2", out, (int)sizeof(out)));
    TEST_ASSERT_TRUE(strstr(out, "text-size ok web") != 0);
    TEST_ASSERT_EQUAL(8, s.zoom[OS_UI_Z_WEB]);
    TEST_ASSERT_EQUAL(0, osui_scale_command(&s, "text-size terminal +", out, (int)sizeof(out)));
    TEST_ASSERT_EQUAL(5, s.zoom[OS_UI_Z_SHELL]);
    TEST_ASSERT_EQUAL(0, osui_scale_command(&s, "text-size web 0", out, (int)sizeof(out)));
    TEST_ASSERT_EQUAL(4, s.zoom[OS_UI_Z_WEB]);
    TEST_ASSERT_EQUAL(1, osui_scale_command(&s, "text-size nowhere 2", out, (int)sizeof(out)));
    TEST_ASSERT_EQUAL(-1, osui_scale_command(&s, "ls", out, (int)sizeof(out)));
    /* small output buffer stays terminated */
    TEST_ASSERT_EQUAL(0, osui_scale_command(&s, "ui-scale 3", out, 8));
    TEST_ASSERT_TRUE(strlen(out) < 8U);
}


/* Lot 69: font choice, saved with the scale, old files still load */
static void test_ui_font(void) {
    osui_scale_t s, b;
    char out[256], buf[160];
    int n;
    osui_scale_init(&s);
    TEST_ASSERT_EQUAL(0, osui_scale_command(&s, "ui-font", out, (int)sizeof(out)));
    TEST_ASSERT_TRUE(strstr(out, "ui-font ok native") != 0);
    TEST_ASSERT_EQUAL(0, osui_scale_command(&s, "ui-font smooth", out, (int)sizeof(out)));
    TEST_ASSERT_EQUAL(1, s.font_mode);
    TEST_ASSERT_EQUAL(1, osui_scale_command(&s, "ui-font comic", out, (int)sizeof(out)));
    TEST_ASSERT_EQUAL(1, s.font_mode);
    n = osui_scale_save(&s, buf, (int)sizeof(buf));
    TEST_ASSERT_TRUE(strstr(buf, " font=1") != 0);
    osui_scale_init(&b);
    TEST_ASSERT_EQUAL(0, osui_scale_load(&b, buf, n));
    TEST_ASSERT_EQUAL(1, b.font_mode);
    TEST_ASSERT_EQUAL(0, osui_scale_load(&b, "ui_scale=8 zoom=4,4,4,4,4,4,4,4,4,4,4,4\n", 39));
    TEST_ASSERT_EQUAL(0, b.font_mode);
    TEST_ASSERT_EQUAL(8, b.ui_q);
    TEST_ASSERT_EQUAL(-1, osui_scale_load(&b, "ui_scale=8 zoom=4,4,4,4,4,4,4,4,4,4,4,4 font=7", 47));
}

int main(void) {
    unity_init();
    RUN_TEST(test_auto_scale_from_screen);
    RUN_TEST(test_parse_and_format);
    RUN_TEST(test_metrics_and_steps);
    RUN_TEST(test_state_zoom_and_viewport);
    RUN_TEST(test_slot_names);
    RUN_TEST(test_save_load_roundtrip);
    RUN_TEST(test_commands);
    RUN_TEST(test_ui_font);
    unity_print_results();
    unity_cleanup();
    return unity_stats.tests_failed == 0 ? 0 : 1;
}
