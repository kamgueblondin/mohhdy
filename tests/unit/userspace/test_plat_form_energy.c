/* Lot 68: US-080 form-factor plan, US-081 applied power profiles and the
 * simulated battery (userspace/plat_form.c, userspace/plat_energy.c). */
#include "../../framework/unity.h"
#include "../../../userspace/plat_form.h"
#include "../../../userspace/plat_energy.h"
#include "../../../userspace/platform.h"
#include <string.h>

static void test_isqrt(void) {
    TEST_ASSERT_EQUAL(0U, plat_form_isqrt(0));
    TEST_ASSERT_EQUAL(1U, plat_form_isqrt(3));
    TEST_ASSERT_EQUAL(1280U, plat_form_isqrt(1280U * 1280U));
    TEST_ASSERT_EQUAL(2202U, plat_form_isqrt(1920U * 1920U + 1080U * 1080U));
    TEST_ASSERT_EQUAL(11585U, plat_form_isqrt(8192U * 8192U * 2U));
}

static void test_plan_from_pixels(void) {
    plat_form_plan_t p;
    TEST_ASSERT_EQUAL(0, plat_form_plan(1024, 768, 0, 0, &p));
    TEST_ASSERT_EQUAL(PF_DESKTOP, p.form);
    TEST_ASSERT_EQUAL(4, p.ui_q);
    TEST_ASSERT_EQUAL(0, p.ppi);
    TEST_ASSERT_EQUAL(640, p.web_css_w);
    TEST_ASSERT_EQUAL(0, plat_form_plan(3840, 2160, 0, 0, &p));
    TEST_ASSERT_EQUAL(12, p.ui_q);                 /* 4K: 3x */
    TEST_ASSERT_EQUAL(24, p.glyph_px);
    TEST_ASSERT_EQUAL(PF_CHAT_FLOAT, p.chat_mode);
    TEST_ASSERT_EQUAL(0, plat_form_plan(390, 844, 0, 0, &p));
    TEST_ASSERT_EQUAL(PF_PHONE, p.form);
    TEST_ASSERT_TRUE(p.portrait);
    TEST_ASSERT_EQUAL(6, p.ui_q);                  /* touch: 1.5x */
    TEST_ASSERT_EQUAL(PF_CHAT_FULL, p.chat_mode);
    TEST_ASSERT_TRUE(p.compact_menu);
    TEST_ASSERT_TRUE(p.touch_px >= 44);
    TEST_ASSERT_EQUAL(0, plat_form_plan(800, 1280, 0, 0, &p));
    TEST_ASSERT_EQUAL(PF_TABLET, p.form);
    TEST_ASSERT_EQUAL(-1, plat_form_plan(100, 100, 0, 0, &p));
    TEST_ASSERT_EQUAL(-1, plat_form_plan(1024, 768, -1, 0, &p));
    TEST_ASSERT_EQUAL(-1, plat_form_plan(1024, 768, 0, 9, &p));
    TEST_ASSERT_EQUAL(-1, plat_form_plan(1024, 768, 0, 0, 0));
}

static void test_plan_with_diagonal(void) {
    plat_form_plan_t p;
    /* 13.3 inch 2560x1600 laptop: about 227 ppi -> 2x or 3x for 2.8 mm */
    TEST_ASSERT_EQUAL(0, plat_form_plan(2560, 1600, 133, 0, &p));
    TEST_ASSERT_EQUAL(PF_DESKTOP, p.form);
    TEST_ASSERT_TRUE(p.ppi >= 220 && p.ppi <= 232);
    TEST_ASSERT_TRUE(p.ui_q == 12 || p.ui_q == 8);
    TEST_ASSERT_TRUE(p.glyph_mm10 >= 20 && p.glyph_mm10 <= 35);
    /* 6.1 inch phone 1170x2532: ~458 ppi -> 4x cap */
    TEST_ASSERT_EQUAL(0, plat_form_plan(1170, 2532, 61, 0, &p));
    TEST_ASSERT_EQUAL(PF_PHONE, p.form);
    TEST_ASSERT_EQUAL(16, p.ui_q);
    TEST_ASSERT_TRUE(p.touch_px > 200);            /* 44 pt at 458 ppi */
    /* 55 inch 4K tv: far away, big glyphs */
    TEST_ASSERT_EQUAL(0, plat_form_plan(3840, 2160, 550, 0, &p));
    TEST_ASSERT_EQUAL(PF_TV, p.form);
    TEST_ASSERT_TRUE(p.ui_q >= 6);
    /* forced form wins */
    TEST_ASSERT_EQUAL(0, plat_form_plan(1024, 768, 0, PF_PHONE, &p));
    TEST_ASSERT_EQUAL(PF_PHONE, p.form);
    TEST_ASSERT_TRUE(p.forced);
}

static void test_names_and_report(void) {
    plat_form_plan_t p;
    char out[512];
    TEST_ASSERT_EQUAL(PF_TABLET, plat_form_kind_parse("tablet"));
    TEST_ASSERT_EQUAL(0, plat_form_kind_parse("auto"));
    TEST_ASSERT_EQUAL(-1, plat_form_kind_parse("watch"));
    TEST_ASSERT_EQUAL_STRING("tv", plat_form_kind_name(PF_TV));
    (void)plat_form_plan(1920, 1080, 156, 0, &p);
    TEST_ASSERT_TRUE(plat_form_report(&p, out, (int)sizeof(out)) > 0);
    TEST_ASSERT_TRUE(strstr(out, "form-factor ok 1920x1080 form desktop (auto) landscape ppi 141") != 0);
    TEST_ASSERT_TRUE(strstr(out, " ui-scale ") != 0);
    TEST_ASSERT_TRUE(plat_form_report(&p, out, 16) < 16);
}

static void test_energy_apply(void) {
    plat_energy_t e;
    plat_energy_init(&e);
    TEST_ASSERT_EQUAL(0, plat_energy_apply(&e, PLAT_PWR_SAVER));
    TEST_ASSERT_EQUAL(64000U, e.pace_cap_bps);
    TEST_ASSERT_EQUAL(15U, e.refresh_hz);
    TEST_ASSERT_EQUAL(250U, e.background_ms);
    TEST_ASSERT_EQUAL(0, plat_energy_apply(&e, PLAT_PWR_PERFORMANCE));
    TEST_ASSERT_EQUAL(0U, e.pace_cap_bps);
    TEST_ASSERT_EQUAL(60U, e.refresh_hz);
    TEST_ASSERT_EQUAL(-1, plat_energy_apply(&e, 99));
    TEST_ASSERT_EQUAL(2U, e.applied);
    TEST_ASSERT_TRUE(plat_energy_draw_mw(PLAT_PWR_PERFORMANCE, 50, 60) > plat_energy_draw_mw(PLAT_PWR_SAVER, 50, 15));
    TEST_ASSERT_TRUE(plat_energy_draw_mw(PLAT_PWR_BALANCED, 100, 30) > plat_energy_draw_mw(PLAT_PWR_BALANCED, 0, 30));
}

static void test_battery_sim(void) {
    plat_energy_t e;
    char out[512];
    uint32_t m1, i;
    plat_energy_init(&e);
    (void)plat_energy_apply(&e, PLAT_PWR_BALANCED);
    TEST_ASSERT_EQUAL(0U, plat_energy_pct(&e));
    TEST_ASSERT_EQUAL(-1, plat_energy_sim_start(&e, 10, 50));
    TEST_ASSERT_EQUAL(-1, plat_energy_sim_start(&e, 50000, 101));
    TEST_ASSERT_EQUAL(0, plat_energy_sim_start(&e, 50000, 100));
    TEST_ASSERT_EQUAL(100U, plat_energy_pct(&e));
    /* one hour at 50 % busy, balanced: 2200+4000+1500+600 = 8300 mW */
    for (i = 0; i < 60; i++) (void)plat_energy_sim_tick(&e, 60000U, 50U);
    TEST_ASSERT_EQUAL(8300U, e.draw_mw);
    TEST_ASSERT_TRUE(plat_energy_pct(&e) >= 82U && plat_energy_pct(&e) <= 84U);  /* 50 - 8.3 Wh */
    m1 = plat_energy_minutes_left(&e);
    TEST_ASSERT_TRUE(m1 >= 290U && m1 <= 310U);
    /* auto mode switches to saver under 20 % */
    e.auto_mode = 1;
    for (i = 0; i < 400 && plat_energy_pct(&e) >= 20U; i++) (void)plat_energy_sim_tick(&e, 60000U, 50U);
    (void)plat_energy_sim_tick(&e, 1000U, 50U);
    TEST_ASSERT_EQUAL(PLAT_PWR_SAVER, e.profile);
    TEST_ASSERT_TRUE(e.auto_switches >= 1U);
    TEST_ASSERT_EQUAL(64000U, e.pace_cap_bps);
    /* empty stays at 0, charging refills up to full */
    for (i = 0; i < 400; i++) (void)plat_energy_sim_tick(&e, 3600000U, 100U);
    TEST_ASSERT_EQUAL(0U, plat_energy_pct(&e));
    e.charging = 1;
    for (i = 0; i < 10; i++) (void)plat_energy_sim_tick(&e, 3600000U, 0U);
    TEST_ASSERT_EQUAL(100U, plat_energy_pct(&e));
    TEST_ASSERT_TRUE(plat_energy_report(&e, out, (int)sizeof(out)) > 0);
    TEST_ASSERT_TRUE(strstr(out, "battery simule pct 100") != 0);
    plat_energy_sim_stop(&e);
    (void)plat_energy_report(&e, out, (int)sizeof(out));
    TEST_ASSERT_TRUE(strstr(out, "battery none (emulated pc, mains)") != 0);
}

int main(void) {
    unity_init();
    RUN_TEST(test_isqrt);
    RUN_TEST(test_plan_from_pixels);
    RUN_TEST(test_plan_with_diagonal);
    RUN_TEST(test_names_and_report);
    RUN_TEST(test_energy_apply);
    RUN_TEST(test_battery_sim);
    unity_print_results();
    unity_cleanup();
    return unity_stats.tests_failed == 0 ? 0 : 1;
}
