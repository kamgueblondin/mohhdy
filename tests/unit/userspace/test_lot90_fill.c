/* Lot 90 (Lot 89 gap 4): autofill dates per value and per accepted fill,
 * the automatic expiry, the time zone, and the version 3 session file. */
#include "../../framework/unity.h"
#include <string.h>
#include "wf_autofill.h"

static wf_autofill_t g_a, g_b;
static char g_buf[8192];

static void test_dates_per_value(void) {
    wf_autofill_init(&g_a);
    wf_autofill_set_today(&g_a, 20000U);
    TEST_ASSERT_EQUAL(0, wf_autofill_put(&g_a, "ex.am", "q", "one", "text"));
    wf_autofill_set_today(&g_a, 20010U);
    TEST_ASSERT_EQUAL(0, wf_autofill_put(&g_a, "ex.am", "q", "two", "text"));
    wf_autofill_set_today(&g_a, 20030U);
    TEST_ASSERT_EQUAL(0, wf_autofill_value_age(&g_a, "ex.am", "q", 0) - 20);
    TEST_ASSERT_EQUAL(20, wf_autofill_value_age(&g_a, "ex.am", "q", 0));
    TEST_ASSERT_EQUAL(30, wf_autofill_value_age(&g_a, "ex.am", "q", 1));
    TEST_ASSERT_EQUAL(-1, wf_autofill_value_age(&g_a, "ex.am", "q", 2));
    TEST_ASSERT_EQUAL(-1, wf_autofill_value_age(&g_a, "ex.am", "q", -1));
    TEST_ASSERT_EQUAL(-1, wf_autofill_value_age(&g_a, "ex.am", "nope", 0));
    /* typed again: "one" is the newest and dated today; "two" keeps 20010 */
    TEST_ASSERT_EQUAL(0, wf_autofill_put(&g_a, "ex.am", "q", "one", "text"));
    TEST_ASSERT_EQUAL(0, wf_autofill_value_age(&g_a, "ex.am", "q", 0));
    TEST_ASSERT_EQUAL(20, wf_autofill_value_age(&g_a, "ex.am", "q", 1));
    TEST_ASSERT_EQUAL(0, wf_autofill_age(&g_a, "ex.am", "q"));
    /* the clock unknown: a value already there keeps its own date */
    wf_autofill_set_today(&g_a, 0U);
    TEST_ASSERT_EQUAL(0, wf_autofill_put(&g_a, "ex.am", "q", "two", "text"));
    wf_autofill_set_today(&g_a, 20040U);
    TEST_ASSERT_EQUAL(30, wf_autofill_value_age(&g_a, "ex.am", "q", 0));
    TEST_ASSERT_EQUAL(10, wf_autofill_value_age(&g_a, "ex.am", "q", 1));
    /* a new value with no clock is undated */
    wf_autofill_set_today(&g_a, 0U);
    TEST_ASSERT_EQUAL(0, wf_autofill_put(&g_a, "ex.am", "q", "three", "text"));
    wf_autofill_set_today(&g_a, 20040U);
    TEST_ASSERT_EQUAL(-1, wf_autofill_value_age(&g_a, "ex.am", "q", 0));
    TEST_ASSERT_EQUAL(30, wf_autofill_value_age(&g_a, "ex.am", "q", 1));
    TEST_ASSERT_EQUAL(10, wf_autofill_value_age(&g_a, "ex.am", "q", 2));
    /* the oldest falls off with its date */
    TEST_ASSERT_EQUAL(0, wf_autofill_put(&g_a, "ex.am", "q", "four", "text"));
    TEST_ASSERT_EQUAL(0, wf_autofill_value_age(&g_a, "ex.am", "q", 0));
    TEST_ASSERT_EQUAL(-1, wf_autofill_value_age(&g_a, "ex.am", "q", 1));
    TEST_ASSERT_EQUAL(30, wf_autofill_value_age(&g_a, "ex.am", "q", 2));
}

static void test_drop_moves_dates(void) {
    wf_autofill_init(&g_a);
    wf_autofill_set_today(&g_a, 100U);
    (void)wf_autofill_put(&g_a, "ex.am", "q", "a", "text");
    wf_autofill_set_today(&g_a, 110U);
    (void)wf_autofill_put(&g_a, "ex.am", "q", "b", "text");
    wf_autofill_set_today(&g_a, 120U);
    (void)wf_autofill_put(&g_a, "ex.am", "q", "c", "text");
    wf_autofill_set_today(&g_a, 130U);
    /* drop the current: b (110) becomes current, a (100) next */
    TEST_ASSERT_EQUAL(2, wf_autofill_drop(&g_a, "ex.am", "q", "c"));
    TEST_ASSERT_EQUAL(20, wf_autofill_value_age(&g_a, "ex.am", "q", 0));
    TEST_ASSERT_EQUAL(30, wf_autofill_value_age(&g_a, "ex.am", "q", 1));
    (void)wf_autofill_put(&g_a, "ex.am", "q", "c", "text");
    /* drop the middle */
    TEST_ASSERT_EQUAL(2, wf_autofill_drop(&g_a, "ex.am", "q", "b"));
    TEST_ASSERT_EQUAL(0, wf_autofill_value_age(&g_a, "ex.am", "q", 0));
    TEST_ASSERT_EQUAL(30, wf_autofill_value_age(&g_a, "ex.am", "q", 1));
}

static void test_touch(void) {
    char v[64];
    wf_autofill_init(&g_a);
    wf_autofill_set_today(&g_a, 500U);
    (void)wf_autofill_put(&g_a, "ex.am", "q", "a", "text");
    (void)wf_autofill_put(&g_a, "ex.am", "q", "b", "text");
    wf_autofill_set_today(&g_a, 540U);
    /* "a" offered and accepted: newest, dated today, nothing stored */
    TEST_ASSERT_EQUAL(0, wf_autofill_touch(&g_a, "EX.am", "q", "a"));
    TEST_ASSERT_EQUAL(0, wf_autofill_get_at(&g_a, "ex.am", "q", 0, v, (int)sizeof(v)));
    TEST_ASSERT_EQUAL_STRING("a", v);
    TEST_ASSERT_EQUAL(0, wf_autofill_value_age(&g_a, "ex.am", "q", 0));
    TEST_ASSERT_EQUAL(40, wf_autofill_value_age(&g_a, "ex.am", "q", 1));
    TEST_ASSERT_EQUAL(1U, g_a.touched);
    TEST_ASSERT_EQUAL(2U, g_a.stored);
    /* the current value touched: dated only */
    wf_autofill_set_today(&g_a, 545U);
    TEST_ASSERT_EQUAL(0, wf_autofill_touch(&g_a, "ex.am", "q", "a"));
    TEST_ASSERT_EQUAL(0, wf_autofill_value_age(&g_a, "ex.am", "q", 0));
    TEST_ASSERT_EQUAL(2, wf_autofill_count(&g_a, "ex.am", "q"));
    /* not there: nothing changes */
    TEST_ASSERT_EQUAL(-1, wf_autofill_touch(&g_a, "ex.am", "q", "zz"));
    TEST_ASSERT_EQUAL(-1, wf_autofill_touch(&g_a, "ex.am", "nope", "a"));
    TEST_ASSERT_EQUAL(-1, wf_autofill_touch(&g_a, "ex.am", "q", 0));
    TEST_ASSERT_EQUAL(-1, wf_autofill_touch(0, "ex.am", "q", "a"));
    TEST_ASSERT_EQUAL(2U, g_a.touched);
}

static void test_touch_site(void) {
    wf_autofill_init(&g_a);
    wf_autofill_set_today(&g_a, 700U);
    (void)wf_autofill_put(&g_a, "login.ex.am", "user", "me", "text");
    (void)wf_autofill_put(&g_a, "www.ex.am", "user", "you", "text");
    (void)wf_autofill_put(&g_a, "other.org", "user", "x", "text");
    wf_autofill_set_today(&g_a, 710U);
    TEST_ASSERT_EQUAL(0, wf_autofill_touch_site(&g_a, "www.ex.am", "user", "you"));
    TEST_ASSERT_EQUAL(0, wf_autofill_age(&g_a, "www.ex.am", "user"));
    /* offered from another host of the site: dated on that host */
    TEST_ASSERT_EQUAL(1, wf_autofill_touch_site(&g_a, "www.ex.am", "user", "me"));
    TEST_ASSERT_EQUAL(0, wf_autofill_age(&g_a, "login.ex.am", "user"));
    TEST_ASSERT_EQUAL(1, wf_autofill_count(&g_a, "www.ex.am", "user"));
    /* another site is never touched */
    TEST_ASSERT_EQUAL(-1, wf_autofill_touch_site(&g_a, "www.ex.am", "user", "x"));
    TEST_ASSERT_EQUAL(10, wf_autofill_age(&g_a, "other.org", "user"));
    TEST_ASSERT_EQUAL(-1, wf_autofill_touch_site(&g_a, 0, "user", "x"));
    TEST_ASSERT_EQUAL(2U, g_a.touched);
}

static void test_forget_older_per_value(void) {
    int und = -1, vals = -1;
    wf_autofill_init(&g_a);
    wf_autofill_set_today(&g_a, 1000U);
    (void)wf_autofill_put(&g_a, "ex.am", "q", "old", "text");
    (void)wf_autofill_put(&g_a, "ex.am", "k", "gone", "text");
    wf_autofill_set_today(&g_a, 1050U);
    (void)wf_autofill_put(&g_a, "ex.am", "q", "new", "text");
    wf_autofill_set_today(&g_a, 1060U);
    /* Lot 89 kept "old" (the key was stored 10 days ago); now it goes */
    TEST_ASSERT_EQUAL(1, wf_autofill_forget_older_values(&g_a, 30U, &und, &vals));
    TEST_ASSERT_EQUAL(0, und);
    TEST_ASSERT_EQUAL(2, vals);
    TEST_ASSERT_EQUAL(1, wf_autofill_count(&g_a, "ex.am", "q"));
    TEST_ASSERT_EQUAL(0, wf_autofill_count(&g_a, "ex.am", "k"));
    TEST_ASSERT_EQUAL(1, g_a.n);
    /* exactly the limit stays */
    TEST_ASSERT_EQUAL(0, wf_autofill_forget_older_values(&g_a, 10U, &und, &vals));
    TEST_ASSERT_EQUAL(0, vals);
    TEST_ASSERT_EQUAL(1, wf_autofill_forget_older_values(&g_a, 9U, &und, &vals));
    TEST_ASSERT_EQUAL(1, vals);
    TEST_ASSERT_EQUAL(0, g_a.n);
    /* an undated older value gets today's date */
    wf_autofill_set_today(&g_a, 0U);
    (void)wf_autofill_put(&g_a, "ex.am", "q", "u", "text");
    wf_autofill_set_today(&g_a, 2000U);
    (void)wf_autofill_put(&g_a, "ex.am", "q", "d", "text");
    TEST_ASSERT_EQUAL(0, wf_autofill_forget_older_values(&g_a, 0U, &und, &vals));
    TEST_ASSERT_EQUAL(1, und);
    TEST_ASSERT_EQUAL(0, wf_autofill_value_age(&g_a, "ex.am", "q", 1));
    /* a future date (clock set back) stays */
    wf_autofill_set_today(&g_a, 1500U);
    TEST_ASSERT_EQUAL(0, wf_autofill_forget_older_values(&g_a, 0U, &und, &vals));
    TEST_ASSERT_EQUAL(2, wf_autofill_count(&g_a, "ex.am", "q"));
    TEST_ASSERT_EQUAL(-1, wf_autofill_forget_older_values(&g_a, 36501U, &und, &vals));
    wf_autofill_set_today(&g_a, 0U);
    TEST_ASSERT_EQUAL(-1, wf_autofill_forget_older_values(&g_a, 1U, &und, &vals));
}

static void test_auto_expiry(void) {
    wf_autofill_init(&g_a);
    TEST_ASSERT_EQUAL(-1, wf_autofill_set_expire(&g_a, 36501U));
    TEST_ASSERT_EQUAL(-1, wf_autofill_set_expire(0, 3U));
    wf_autofill_set_today(&g_a, 3000U);
    (void)wf_autofill_put(&g_a, "ex.am", "q", "a", "text");
    wf_autofill_set_today(&g_a, 3005U);
    (void)wf_autofill_put(&g_a, "ex.am", "q", "b", "text");
    /* off: a tick forgets nothing */
    TEST_ASSERT_EQUAL(0, wf_autofill_tick(&g_a, 3100U));
    TEST_ASSERT_EQUAL(2, wf_autofill_count(&g_a, "ex.am", "q"));
    TEST_ASSERT_EQUAL(0, wf_autofill_set_expire(&g_a, 97U));
    /* on: the first tick of a day runs it (a is 100 days old, b 95) */
    TEST_ASSERT_EQUAL(1, wf_autofill_tick(&g_a, 3100U));
    TEST_ASSERT_EQUAL(1, wf_autofill_count(&g_a, "ex.am", "q"));
    TEST_ASSERT_EQUAL(3100U, g_a.expire_ran);
    TEST_ASSERT_EQUAL(1U, g_a.expired);
    /* the same day again: not run (b would still stay) */
    TEST_ASSERT_EQUAL(0, wf_autofill_tick(&g_a, 3100U));
    /* a new day: b (98 days) goes */
    TEST_ASSERT_EQUAL(1, wf_autofill_tick(&g_a, 3103U));
    TEST_ASSERT_EQUAL(0, g_a.n);
    TEST_ASSERT_EQUAL(2U, g_a.expired);
    /* no clock */
    TEST_ASSERT_EQUAL(-1, wf_autofill_tick(&g_a, 0U));
    TEST_ASSERT_EQUAL(0U, g_a.today);
    TEST_ASSERT_EQUAL(-1, wf_autofill_tick(0, 5U));
}

static void test_time_zone(void) {
    /* 2026-11-12 is day 20770 (1970-01-01 = 1) */
    uint32_t d = wf_autofill_day("20261112");
    TEST_ASSERT_EQUAL(d, wf_autofill_day_tz("20261112230000", 0));
    TEST_ASSERT_EQUAL(d + 1U, wf_autofill_day_tz("20261112230000", 60));
    TEST_ASSERT_EQUAL(d, wf_autofill_day_tz("20261112225900", 60));
    TEST_ASSERT_EQUAL(d + 1U, wf_autofill_day_tz("20261112200000", 240));
    TEST_ASSERT_EQUAL(d, wf_autofill_day_tz("20261112195959", 240));
    TEST_ASSERT_EQUAL(d - 1U, wf_autofill_day_tz("20261112010000", -120));
    TEST_ASSERT_EQUAL(d, wf_autofill_day_tz("20261112020000", -120));
    /* month and year ends */
    TEST_ASSERT_EQUAL(wf_autofill_day("20270101"), wf_autofill_day_tz("20261231233000", 30));
    TEST_ASSERT_EQUAL(wf_autofill_day("20240229"), wf_autofill_day_tz("20240301003000", -60));
    /* bounds */
    TEST_ASSERT_EQUAL(0U, wf_autofill_day_tz("19700101000000", -1));
    TEST_ASSERT_EQUAL(0U, wf_autofill_day_tz("21991231235900", 60));
    TEST_ASSERT_EQUAL(0U, wf_autofill_day_tz("20261112", 60));
    TEST_ASSERT_EQUAL(0U, wf_autofill_day_tz("20261112246000", 60));
    TEST_ASSERT_EQUAL(0U, wf_autofill_day_tz("20261112236000", 60));
    TEST_ASSERT_EQUAL(0U, wf_autofill_day_tz("20261112230000", 841));
    TEST_ASSERT_EQUAL(0U, wf_autofill_day_tz("20261112230000", -721));
    TEST_ASSERT_EQUAL(d + 1U, wf_autofill_day_tz("20261112100000", 840));
    TEST_ASSERT_EQUAL(d - 1U, wf_autofill_day_tz("20261112110000", -720));
    wf_autofill_init(&g_a);
    TEST_ASSERT_EQUAL(0, wf_autofill_set_tz(&g_a, 240));
    TEST_ASSERT_EQUAL(-1, wf_autofill_set_tz(&g_a, 900));
    TEST_ASSERT_EQUAL(240, g_a.tz_min);
}

static void test_save_load_v3(void) {
    int n, m;
    wf_autofill_init(&g_a);
    TEST_ASSERT_EQUAL(0, wf_autofill_set_expire(&g_a, 90U));
    TEST_ASSERT_EQUAL(0, wf_autofill_set_tz(&g_a, -300));
    wf_autofill_set_today(&g_a, 20000U);
    (void)wf_autofill_put(&g_a, "ex.am", "q", "a", "text");
    wf_autofill_set_today(&g_a, 20005U);
    (void)wf_autofill_put(&g_a, "ex.am", "q", "b", "text");
    n = wf_autofill_save(&g_a, g_buf, (int)sizeof(g_buf));
    TEST_ASSERT(n > 0);
    TEST_ASSERT_EQUAL_STRING("MOHHDY-AUTOFILL 3 expire 90 tz -300\nex.am\tq\ta\t20000\nex.am\tq\tb\t20005\n", g_buf);
    wf_autofill_init(&g_b);
    wf_autofill_set_today(&g_b, 20010U);
    TEST_ASSERT_EQUAL(0, wf_autofill_load(&g_b, g_buf, n));
    TEST_ASSERT_EQUAL(90U, g_b.expire_days);
    TEST_ASSERT_EQUAL(-300, g_b.tz_min);
    TEST_ASSERT_EQUAL(0U, g_b.expire_ran);
    TEST_ASSERT_EQUAL(5, wf_autofill_value_age(&g_b, "ex.am", "q", 0));
    TEST_ASSERT_EQUAL(10, wf_autofill_value_age(&g_b, "ex.am", "q", 1));
    m = wf_autofill_save(&g_b, g_buf + 4096, 4096);
    TEST_ASSERT_EQUAL(n, m);
    TEST_ASSERT_EQUAL(0, memcmp(g_buf, g_buf + 4096, (size_t)n));
    /* positive offset, order of the settings, no settings */
    TEST_ASSERT_EQUAL(0, wf_autofill_load(&g_b, "MOHHDY-AUTOFILL 3 tz 330 expire 7\n", 34));
    TEST_ASSERT_EQUAL(330, g_b.tz_min);
    TEST_ASSERT_EQUAL(7U, g_b.expire_days);
    TEST_ASSERT_EQUAL(0, wf_autofill_load(&g_b, "MOHHDY-AUTOFILL 3\n", 18));
    TEST_ASSERT_EQUAL(0, g_b.tz_min);
    TEST_ASSERT_EQUAL(0U, g_b.expire_days);
}

static void test_bad_headers(void) {
    static const char *const bad[] = {
        "MOHHDY-AUTOFILL 3 expire 0\n", "MOHHDY-AUTOFILL 3 expire 36501\n", "MOHHDY-AUTOFILL 3 expire\n",
        "MOHHDY-AUTOFILL 3 tz 841\n", "MOHHDY-AUTOFILL 3 tz -721\n", "MOHHDY-AUTOFILL 3 tz 0\n",
        "MOHHDY-AUTOFILL 3 tz -0\n", "MOHHDY-AUTOFILL 3 tz 60 tz 60\n", "MOHHDY-AUTOFILL 3 expire 5 expire 6\n",
        "MOHHDY-AUTOFILL 3 color red\n", "MOHHDY-AUTOFILL 3  expire 5\n", "MOHHDY-AUTOFILL 3 expire 5x\n",
        "MOHHDY-AUTOFILL 3 expire 123456\n", 0
    };
    int k;
    wf_autofill_init(&g_b);
    (void)wf_autofill_put(&g_b, "keep.me", "q", "v", "text");
    (void)wf_autofill_set_expire(&g_b, 4U);
    for (k = 0; bad[k]; k++) {
        TEST_ASSERT_EQUAL(-1, wf_autofill_load(&g_b, bad[k], (int)strlen(bad[k])));
        TEST_ASSERT_EQUAL(1, g_b.n);
        TEST_ASSERT_EQUAL(4U, g_b.expire_days);
    }
    /* version 2 keeps loading, every value with the key's day */
    TEST_ASSERT_EQUAL(0, wf_autofill_load(&g_b, "MOHHDY-AUTOFILL 2\nex.am\tq\ta\t50\nex.am\tq\tb\t50\n", 43));
    wf_autofill_set_today(&g_b, 60U);
    TEST_ASSERT_EQUAL(10, wf_autofill_value_age(&g_b, "ex.am", "q", 1));
    TEST_ASSERT_EQUAL(0U, g_b.expire_days);
}

static void test_status_and_value_ages(void) {
    char out[600];
    wf_autofill_init(&g_a);
    (void)wf_autofill_set_expire(&g_a, 30U);
    (void)wf_autofill_set_tz(&g_a, -90);
    wf_autofill_set_today(&g_a, 100U);
    (void)wf_autofill_put(&g_a, "ex.am", "q", "a", "text");
    wf_autofill_set_today(&g_a, 0U);
    (void)wf_autofill_put(&g_a, "ex.am", "q", "b", "text");
    wf_autofill_set_today(&g_a, 104U);
    (void)wf_autofill_touch(&g_a, "ex.am", "q", "b");
    TEST_ASSERT(wf_autofill_status(&g_a, out, (int)sizeof(out)) > 0);
    TEST_ASSERT_EQUAL_STRING("autofill status expire 30 tz -90 today 104 touched 1 expired 0 keys 1 values 2\n", out);
    TEST_ASSERT(wf_autofill_value_ages(&g_a, out, (int)sizeof(out)) > 0);
    TEST_ASSERT_EQUAL_STRING("autofill value ex.am q 0 0d\nautofill value ex.am q 1 4d\n", out);
    wf_autofill_set_today(&g_a, 0U);
    TEST_ASSERT(wf_autofill_value_ages(&g_a, out, (int)sizeof(out)) > 0);
    TEST_ASSERT_EQUAL_STRING("autofill value ex.am q 0 undated\nautofill value ex.am q 1 undated\n", out);
    TEST_ASSERT_EQUAL(-1, wf_autofill_value_ages(&g_a, out, 20));
    TEST_ASSERT_EQUAL(-1, wf_autofill_status(&g_a, out, 20));
}

int main(void) {
    unity_init();
    RUN_TEST(test_dates_per_value);
    RUN_TEST(test_drop_moves_dates);
    RUN_TEST(test_touch);
    RUN_TEST(test_touch_site);
    RUN_TEST(test_forget_older_per_value);
    RUN_TEST(test_auto_expiry);
    RUN_TEST(test_time_zone);
    RUN_TEST(test_save_load_v3);
    RUN_TEST(test_bad_headers);
    RUN_TEST(test_status_and_value_ages);
    unity_print_results();
    unity_cleanup();
    return unity_stats.tests_failed == 0 ? 0 : 1;
}
