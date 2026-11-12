/* Lot 89: autofill keys carry the day they were last stored, the
 * session file keeps it, and "forget-older DAYS" drops the stale ones
 * (Lot 87 gap 2: no expiry). */
#include "../../framework/unity.h"
#include "wf_autofill.h"
#include <string.h>

static wf_autofill_t g_a, g_b;
static char g_buf[4096];

static void test_day_numbers(void) {
    /* 1970-01-01 is day 1; checked against Python's date arithmetic */
    TEST_ASSERT_EQUAL(1U, wf_autofill_day("19700101000000Z"));
    TEST_ASSERT_EQUAL(32U, wf_autofill_day("19700201"));
    TEST_ASSERT_EQUAL(11018U, wf_autofill_day("20000301"));   /* 2000 is leap */
    TEST_ASSERT_EQUAL(19783U, wf_autofill_day("20240229"));
    TEST_ASSERT_EQUAL(20768U, wf_autofill_day("20261110120000Z"));
    TEST_ASSERT_EQUAL(WF_AF_DAY_MAX, wf_autofill_day("21991231"));
    /* not a date, or out of range */
    TEST_ASSERT_EQUAL(0U, wf_autofill_day("20230229"));        /* 2023 is not */
    TEST_ASSERT_EQUAL(0U, wf_autofill_day("21000229"));        /* 2100 is not */
    TEST_ASSERT_EQUAL(0U, wf_autofill_day("20261131"));
    TEST_ASSERT_EQUAL(0U, wf_autofill_day("20261300"));
    TEST_ASSERT_EQUAL(0U, wf_autofill_day("20260100"));
    TEST_ASSERT_EQUAL(0U, wf_autofill_day("19691231"));
    TEST_ASSERT_EQUAL(0U, wf_autofill_day("22000101"));
    TEST_ASSERT_EQUAL(0U, wf_autofill_day("2026111"));
    TEST_ASSERT_EQUAL(0U, wf_autofill_day("2026-11-10"));
    TEST_ASSERT_EQUAL(0U, wf_autofill_day(0));
}

static void test_store_dates_and_age(void) {
    wf_autofill_init(&g_a);
    /* no clock: stores stay undated */
    TEST_ASSERT_EQUAL(0, wf_autofill_put(&g_a, "ex.am", "q", "a", "text"));
    TEST_ASSERT_EQUAL(-1, wf_autofill_age(&g_a, "ex.am", "q"));
    wf_autofill_set_today(&g_a, 20000U);
    TEST_ASSERT_EQUAL(-1, wf_autofill_age(&g_a, "ex.am", "q"));
    TEST_ASSERT_EQUAL(0, wf_autofill_put(&g_a, "ex.am", "user", "ada", "text"));
    TEST_ASSERT_EQUAL(0, wf_autofill_age(&g_a, "ex.am", "user"));
    wf_autofill_set_today(&g_a, 20010U);
    TEST_ASSERT_EQUAL(10, wf_autofill_age(&g_a, "ex.am", "user"));
    /* storing again (a new or the same value) dates the key today */
    TEST_ASSERT_EQUAL(0, wf_autofill_put(&g_a, "ex.am", "user", "ada", "text"));
    TEST_ASSERT_EQUAL(0, wf_autofill_age(&g_a, "ex.am", "user"));
    TEST_ASSERT_EQUAL(0, wf_autofill_put(&g_a, "ex.am", "q", "b", "text"));
    TEST_ASSERT_EQUAL(0, wf_autofill_age(&g_a, "ex.am", "q"));
    /* a refused store does not touch the date */
    wf_autofill_set_today(&g_a, 20020U);
    TEST_ASSERT_EQUAL(-1, wf_autofill_put(&g_a, "ex.am", "q", "pw", "password"));
    TEST_ASSERT_EQUAL(10, wf_autofill_age(&g_a, "ex.am", "q"));
    /* a clock set back: a date after today reads 0 */
    wf_autofill_set_today(&g_a, 19990U);
    TEST_ASSERT_EQUAL(0, wf_autofill_age(&g_a, "ex.am", "q"));
    TEST_ASSERT_EQUAL(-1, wf_autofill_age(&g_a, "ex.am", "nope"));
    TEST_ASSERT_EQUAL(-1, wf_autofill_age(0, "ex.am", "q"));
}

static void test_forget_older(void) {
    int und = -1;
    wf_autofill_init(&g_a);
    (void)wf_autofill_put(&g_a, "old.org", "q", "undated", "text");
    wf_autofill_set_today(&g_a, 20000U);
    (void)wf_autofill_put(&g_a, "ex.am", "a", "x", "text");
    wf_autofill_set_today(&g_a, 20030U);
    (void)wf_autofill_put(&g_a, "ex.am", "b", "y", "text");
    wf_autofill_set_today(&g_a, 20060U);
    (void)wf_autofill_put(&g_a, "ex.am", "c", "z", "text");
    TEST_ASSERT_EQUAL(4, g_a.n);
    /* ages 60, 30, 0 and undated: 45 days drops the first only */
    TEST_ASSERT_EQUAL(1, wf_autofill_forget_older(&g_a, 45U, &und));
    TEST_ASSERT_EQUAL(1, und);
    TEST_ASSERT_EQUAL(3, g_a.n);
    TEST_ASSERT_EQUAL(-1, wf_autofill_age(&g_a, "ex.am", "a"));
    /* the undated key was dated today: it is kept, age 0 */
    TEST_ASSERT_EQUAL(0, wf_autofill_age(&g_a, "old.org", "q"));
    /* exactly 30 days old is not older than 30 */
    TEST_ASSERT_EQUAL(0, wf_autofill_forget_older(&g_a, 30U, &und));
    TEST_ASSERT_EQUAL(0, und);
    TEST_ASSERT_EQUAL(1, wf_autofill_forget_older(&g_a, 29U, 0));
    TEST_ASSERT_EQUAL(2, g_a.n);
    /* 0 forgets what is older than today; today's keys stay */
    TEST_ASSERT_EQUAL(0, wf_autofill_forget_older(&g_a, 0U, 0));
    wf_autofill_set_today(&g_a, 20061U);
    TEST_ASSERT_EQUAL(2, wf_autofill_forget_older(&g_a, 0U, 0));
    TEST_ASSERT_EQUAL(0, g_a.n);
    /* the freed slots take new keys */
    TEST_ASSERT_EQUAL(0, wf_autofill_put(&g_a, "ex.am", "a", "again", "text"));
    TEST_ASSERT_EQUAL(1, g_a.n);
    /* no clock or a range out of bounds: refused, nothing changes */
    TEST_ASSERT_EQUAL(-1, wf_autofill_forget_older(&g_a, 36501U, &und));
    wf_autofill_set_today(&g_a, 0U);
    TEST_ASSERT_EQUAL(-1, wf_autofill_forget_older(&g_a, 1U, &und));
    TEST_ASSERT_EQUAL(0, und);
    TEST_ASSERT_EQUAL(1, g_a.n);
    TEST_ASSERT_EQUAL(-1, wf_autofill_forget_older(0, 1U, 0));
}

static void test_save_load_dates(void) {
    int n, m;
    wf_autofill_init(&g_a);
    (void)wf_autofill_put(&g_a, "old.org", "q", "u", "text");
    wf_autofill_set_today(&g_a, 20768U);
    (void)wf_autofill_put(&g_a, "ex.am", "q", "old", "text");
    (void)wf_autofill_put(&g_a, "ex.am", "q", "new", "text");
    n = wf_autofill_save(&g_a, g_buf, (int)sizeof(g_buf));
    TEST_ASSERT(n > 0);
    /* Lot 90 (converted, was version 2): version 3, no settings */
    TEST_ASSERT(strncmp(g_buf, "MOHHDY-AUTOFILL 3\n", 18) == 0);
    TEST_ASSERT(strstr(g_buf, "old.org\tq\tu\n") != 0);
    TEST_ASSERT(strstr(g_buf, "ex.am\tq\told\t20768\nex.am\tq\tnew\t20768\n") != 0);
    /* the loaded store keeps the dates and its own clock */
    wf_autofill_init(&g_b);
    wf_autofill_set_today(&g_b, 20800U);
    TEST_ASSERT_EQUAL(0, wf_autofill_load(&g_b, g_buf, n));
    TEST_ASSERT_EQUAL(20800U, g_b.today);
    TEST_ASSERT_EQUAL(32, wf_autofill_age(&g_b, "ex.am", "q"));
    TEST_ASSERT_EQUAL(-1, wf_autofill_age(&g_b, "old.org", "q"));
    TEST_ASSERT_EQUAL(2, wf_autofill_count(&g_b, "ex.am", "q"));
    m = wf_autofill_save(&g_b, g_buf + 2048, 2048);
    TEST_ASSERT_EQUAL(n, m);
    TEST_ASSERT_EQUAL(0, memcmp(g_buf, g_buf + 2048, (size_t)n));
}

static void test_load_old_and_bad_files(void) {
    static const char v1[] = "MOHHDY-AUTOFILL 1\nex.am\tq\tv\n";
    wf_autofill_init(&g_b);
    wf_autofill_set_today(&g_b, 20000U);
    /* a lot 87 file still loads, undated */
    TEST_ASSERT_EQUAL(0, wf_autofill_load(&g_b, v1, (int)strlen(v1)));
    TEST_ASSERT_EQUAL(1, g_b.n);
    TEST_ASSERT_EQUAL(-1, wf_autofill_age(&g_b, "ex.am", "q"));
    /* bad day fields refuse the whole file and keep the store */
    TEST_ASSERT_EQUAL(-1, wf_autofill_load(&g_b, "M\nex.am\tq\tv\t\n", 13));
    TEST_ASSERT_EQUAL(-1, wf_autofill_load(&g_b, "M\nex.am\tq\tv\t0\n", 14));
    TEST_ASSERT_EQUAL(-1, wf_autofill_load(&g_b, "M\nex.am\tq\tv\t12a\n", 16));
    TEST_ASSERT_EQUAL(-1, wf_autofill_load(&g_b, "M\nex.am\tq\tv\t84007\n", 18));
    TEST_ASSERT_EQUAL(-1, wf_autofill_load(&g_b, "M\nex.am\tq\tv\t99999999999\n", 24));
    TEST_ASSERT_EQUAL(-1, wf_autofill_load(&g_b, "M\nex.am\tq\tv\t5\t6\n", 16));
    TEST_ASSERT_EQUAL(1, g_b.n);
    TEST_ASSERT_EQUAL(-1, wf_autofill_age(&g_b, "ex.am", "q"));
    TEST_ASSERT_EQUAL(0, wf_autofill_load(&g_b, "M\nex.am\tq\tv\t84006\n", 18));
    TEST_ASSERT_EQUAL(0, wf_autofill_age(&g_b, "ex.am", "q"));   /* in the future: 0 */
}

static void test_ages_report(void) {
    char small[40];
    wf_autofill_init(&g_a);
    (void)wf_autofill_put(&g_a, "old.org", "q", "u", "text");
    wf_autofill_set_today(&g_a, 100U);
    (void)wf_autofill_put(&g_a, "ex.am", "user", "ada", "text");
    wf_autofill_set_today(&g_a, 107U);
    TEST_ASSERT(wf_autofill_ages(&g_a, g_buf, (int)sizeof(g_buf)) > 0);
    TEST_ASSERT_EQUAL_STRING("autofill age old.org q undated\nautofill age ex.am user 7d\n"
                             "autofill ages 2 dated 1\n", g_buf);
    /* the values are never listed */
    TEST_ASSERT(strstr(g_buf, "ada") == 0);
    /* too small: -1 and an empty string, never an overrun */
    memset(small, 'X', sizeof(small));
    TEST_ASSERT_EQUAL(-1, wf_autofill_ages(&g_a, small, 30));
    TEST_ASSERT_EQUAL(0, small[0]);
    TEST_ASSERT_EQUAL('X', small[30]);
    TEST_ASSERT_EQUAL(-1, wf_autofill_ages(0, g_buf, 10));
    wf_autofill_init(&g_b);
    TEST_ASSERT(wf_autofill_ages(&g_b, g_buf, (int)sizeof(g_buf)) > 0);
    TEST_ASSERT_EQUAL_STRING("autofill ages 0 dated 0\n", g_buf);
}

int main(void) {
    unity_init();
    RUN_TEST(test_day_numbers);
    RUN_TEST(test_store_dates_and_age);
    RUN_TEST(test_forget_older);
    RUN_TEST(test_save_load_dates);
    RUN_TEST(test_load_old_and_bad_files);
    RUN_TEST(test_ages_report);
    unity_print_results();
    unity_cleanup();
    return unity_stats.tests_failed == 0 ? 0 : 1;
}
