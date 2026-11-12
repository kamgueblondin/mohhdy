/* Lot 91: autofill summer time rules, hand-written cases (the generated
 * suites are test_lot91_dst.c and test_lot91_dststore.c). */
#include "../../framework/unity.h"
#include <string.h>
#include "wf_autofill.h"

static wf_autofill_t g_a, g_b;
static char g_buf[4096];

static void test_rule_names(void) {
    TEST_ASSERT_EQUAL_STRING("off", wf_autofill_dst_name(WF_AF_DST_NONE));
    TEST_ASSERT_EQUAL_STRING("eu", wf_autofill_dst_name(WF_AF_DST_EU));
    TEST_ASSERT_EQUAL_STRING("us", wf_autofill_dst_name(WF_AF_DST_US));
    TEST_ASSERT_EQUAL_STRING("au", wf_autofill_dst_name(WF_AF_DST_AU));
    TEST_ASSERT_EQUAL_STRING("", wf_autofill_dst_name(4));
    TEST_ASSERT_EQUAL_STRING("", wf_autofill_dst_name(-1));
    TEST_ASSERT_EQUAL(WF_AF_DST_NONE, wf_autofill_dst_parse("off"));
    TEST_ASSERT_EQUAL(WF_AF_DST_NONE, wf_autofill_dst_parse("none"));
    TEST_ASSERT_EQUAL(WF_AF_DST_EU, wf_autofill_dst_parse("eu"));
    TEST_ASSERT_EQUAL(WF_AF_DST_US, wf_autofill_dst_parse("us"));
    TEST_ASSERT_EQUAL(WF_AF_DST_AU, wf_autofill_dst_parse("au"));
    TEST_ASSERT_EQUAL(-1, wf_autofill_dst_parse(""));
    TEST_ASSERT_EQUAL(-1, wf_autofill_dst_parse(NULL));
    TEST_ASSERT_EQUAL(-1, wf_autofill_dst_parse("e"));
    TEST_ASSERT_EQUAL(-1, wf_autofill_dst_parse("eu "));
    TEST_ASSERT_EQUAL(-1, wf_autofill_dst_parse("nonex"));
    TEST_ASSERT_EQUAL(-1, wf_autofill_dst_parse("non"));
    TEST_ASSERT_EQUAL(-1, wf_autofill_dst_parse("US"));
}

static void test_set_rule(void) {
    wf_autofill_init(&g_a);
    TEST_ASSERT_EQUAL(0, g_a.dst);
    TEST_ASSERT_EQUAL(0, wf_autofill_set_dst(&g_a, WF_AF_DST_AU));
    TEST_ASSERT_EQUAL(WF_AF_DST_AU, g_a.dst);
    TEST_ASSERT_EQUAL(-1, wf_autofill_set_dst(&g_a, 4));
    TEST_ASSERT_EQUAL(-1, wf_autofill_set_dst(&g_a, -1));
    TEST_ASSERT_EQUAL(WF_AF_DST_AU, g_a.dst);
    TEST_ASSERT_EQUAL(-1, wf_autofill_set_dst(NULL, 1));
    TEST_ASSERT_EQUAL(0, wf_autofill_set_dst(&g_a, WF_AF_DST_NONE));
    TEST_ASSERT_EQUAL(0, g_a.dst);
}

/* the three rules at their 2026 changes */
static void test_changes_2026(void) {
    /* eu: 29 March and 25 October, 01:00 UTC */
    TEST_ASSERT_EQUAL(0, wf_autofill_dst_offset("202603290059", 60, WF_AF_DST_EU));
    TEST_ASSERT_EQUAL(60, wf_autofill_dst_offset("202603290100", 60, WF_AF_DST_EU));
    TEST_ASSERT_EQUAL(60, wf_autofill_dst_offset("202610250059", 60, WF_AF_DST_EU));
    TEST_ASSERT_EQUAL(0, wf_autofill_dst_offset("202610250100", 60, WF_AF_DST_EU));
    /* the eu rule does not depend on the offset */
    TEST_ASSERT_EQUAL(60, wf_autofill_dst_offset("202603290100", -720, WF_AF_DST_EU));
    /* us, New York (-300): 8 March 07:00 UTC and 1 November 06:00 UTC */
    TEST_ASSERT_EQUAL(0, wf_autofill_dst_offset("202603080659", -300, WF_AF_DST_US));
    TEST_ASSERT_EQUAL(60, wf_autofill_dst_offset("202603080700", -300, WF_AF_DST_US));
    TEST_ASSERT_EQUAL(60, wf_autofill_dst_offset("202611010559", -300, WF_AF_DST_US));
    TEST_ASSERT_EQUAL(0, wf_autofill_dst_offset("202611010600", -300, WF_AF_DST_US));
    /* us, Los Angeles (-480): the same local times, three hours later */
    TEST_ASSERT_EQUAL(0, wf_autofill_dst_offset("202603080959", -480, WF_AF_DST_US));
    TEST_ASSERT_EQUAL(60, wf_autofill_dst_offset("202603081000", -480, WF_AF_DST_US));
    /* au, Sydney (600): 4 October 2026 02:00 local = 3 October 16:00 UTC;
     * 5 April 2026 03:00 summer = 4 April 16:00 UTC */
    TEST_ASSERT_EQUAL(0, wf_autofill_dst_offset("202610031559", 600, WF_AF_DST_AU));
    TEST_ASSERT_EQUAL(60, wf_autofill_dst_offset("202610031600", 600, WF_AF_DST_AU));
    TEST_ASSERT_EQUAL(60, wf_autofill_dst_offset("202604041559", 600, WF_AF_DST_AU));
    TEST_ASSERT_EQUAL(0, wf_autofill_dst_offset("202604041600", 600, WF_AF_DST_AU));
    /* au: January is summer time, July is not */
    TEST_ASSERT_EQUAL(60, wf_autofill_dst_offset("202601150000", 600, WF_AF_DST_AU));
    TEST_ASSERT_EQUAL(0, wf_autofill_dst_offset("202607150000", 600, WF_AF_DST_AU));
    /* the seconds are read but not used */
    TEST_ASSERT_EQUAL(60, wf_autofill_dst_offset("20260329010059", 60, WF_AF_DST_EU));
}

/* the local date moves with the summer hour */
static void test_day_moves(void) {
    uint32_t d = wf_autofill_day("20260701");
    TEST_ASSERT_TRUE(d > 0);
    /* 23:30 UTC on 1 July in Paris: 00:30 or 01:30 on 2 July either way */
    TEST_ASSERT_EQUAL((int)d + 1, (int)wf_autofill_day_rule("202607012330", 60, WF_AF_DST_EU));
    /* 22:30 UTC, Paris: 23:30 standard, 00:30 summer */
    TEST_ASSERT_EQUAL((int)d, (int)wf_autofill_day_rule("202607012230", 60, WF_AF_DST_NONE));
    TEST_ASSERT_EQUAL((int)d + 1, (int)wf_autofill_day_rule("202607012230", 60, WF_AF_DST_EU));
    /* 03:30 UTC on 2 July, New York: 22:30 standard, 23:30 summer, both 1 July */
    TEST_ASSERT_EQUAL((int)d, (int)wf_autofill_day_rule("202607020330", -300, WF_AF_DST_US));
    /* 04:30 UTC: 23:30 standard (1 July), 00:30 summer (2 July) */
    TEST_ASSERT_EQUAL((int)d, (int)wf_autofill_day_rule("202607020430", -300, WF_AF_DST_NONE));
    TEST_ASSERT_EQUAL((int)d + 1, (int)wf_autofill_day_rule("202607020430", -300, WF_AF_DST_US));
    /* July is winter for au: 13:30 UTC in Sydney is 23:30 on 1 July */
    TEST_ASSERT_EQUAL((int)d, (int)wf_autofill_day_rule("202607011330", 600, WF_AF_DST_AU));
    TEST_ASSERT_EQUAL((int)d, (int)wf_autofill_day_rule("202607011330", 600, WF_AF_DST_NONE));
    /* in January it is 00:30 on the next day */
    TEST_ASSERT_EQUAL((int)wf_autofill_day("20260116"), (int)wf_autofill_day_rule("202601151330", 600, WF_AF_DST_AU));
    /* the end of the range */
    TEST_ASSERT_EQUAL((int)WF_AF_DAY_MAX, (int)wf_autofill_day_rule("219912312259", 0, WF_AF_DST_AU));
    TEST_ASSERT_EQUAL(0, (int)wf_autofill_day_rule("219912312300", 0, WF_AF_DST_AU));
    TEST_ASSERT_EQUAL(1, (int)wf_autofill_day_rule("197001010000", 0, WF_AF_DST_US));
    TEST_ASSERT_EQUAL(0, (int)wf_autofill_day_rule("197001010000", -1, WF_AF_DST_US));
}

/* a session file without a rule is the Lot 90 file and loads with none */
static void test_file(void) {
    wf_autofill_init(&g_a);
    TEST_ASSERT_EQUAL(0, wf_autofill_set_tz(&g_a, 60));
    TEST_ASSERT_TRUE(wf_autofill_save(&g_a, g_buf, (int)sizeof(g_buf)) > 0);
    TEST_ASSERT_EQUAL_STRING("MOHHDY-AUTOFILL 3 tz 60\n", g_buf);
    TEST_ASSERT_EQUAL(0, wf_autofill_set_dst(&g_a, WF_AF_DST_EU));
    TEST_ASSERT_TRUE(wf_autofill_save(&g_a, g_buf, (int)sizeof(g_buf)) > 0);
    TEST_ASSERT_EQUAL_STRING("MOHHDY-AUTOFILL 3 tz 60 dst eu\n", g_buf);
    wf_autofill_init(&g_b);
    TEST_ASSERT_EQUAL(0, wf_autofill_load(&g_b, g_buf, (int)strlen(g_buf)));
    TEST_ASSERT_EQUAL(WF_AF_DST_EU, g_b.dst);
    TEST_ASSERT_EQUAL(60, g_b.tz_min);
    /* a file with no rule clears the one in memory */
    TEST_ASSERT_EQUAL(0, wf_autofill_load(&g_b, "MOHHDY-AUTOFILL 3 tz 60\n", 24));
    TEST_ASSERT_EQUAL(0, g_b.dst);
    /* the rule first, then the others */
    TEST_ASSERT_EQUAL(0, wf_autofill_load(&g_b, "MOHHDY-AUTOFILL 3 dst au tz 600 expire 9\n", 41));
    TEST_ASSERT_EQUAL(WF_AF_DST_AU, g_b.dst);
    TEST_ASSERT_EQUAL(600, g_b.tz_min);
    TEST_ASSERT_EQUAL(9, (int)g_b.expire_days);
    /* refused: twice, off, a fourth letter, a capital; the store is kept */
    TEST_ASSERT_EQUAL(-1, wf_autofill_load(&g_b, "MOHHDY-AUTOFILL 3 dst us dst us\n", 32));
    TEST_ASSERT_EQUAL(-1, wf_autofill_load(&g_b, "MOHHDY-AUTOFILL 3 dst off\n", 26));
    TEST_ASSERT_EQUAL(-1, wf_autofill_load(&g_b, "MOHHDY-AUTOFILL 3 dst euro\n", 27));
    TEST_ASSERT_EQUAL(-1, wf_autofill_load(&g_b, "MOHHDY-AUTOFILL 3 dst Eu\n", 25));
    TEST_ASSERT_EQUAL(WF_AF_DST_AU, g_b.dst);
}

/* the status line shows the rule only when one is set */
static void test_status(void) {
    wf_autofill_init(&g_a);
    (void)wf_autofill_tick(&g_a, 20000U);
    TEST_ASSERT_TRUE(wf_autofill_status(&g_a, g_buf, (int)sizeof(g_buf)) > 0);
    TEST_ASSERT_EQUAL_STRING("autofill status expire 0 tz 0 today 20000 touched 0 expired 0 keys 0 values 0\n", g_buf);
    TEST_ASSERT_EQUAL(0, wf_autofill_set_dst(&g_a, WF_AF_DST_US));
    TEST_ASSERT_TRUE(wf_autofill_status(&g_a, g_buf, (int)sizeof(g_buf)) > 0);
    TEST_ASSERT_EQUAL_STRING("autofill status expire 0 tz 0 today 20000 touched 0 expired 0 keys 0 values 0 dst us\n", g_buf);
    /* a short buffer: refused, empty */
    TEST_ASSERT_EQUAL(-1, wf_autofill_status(&g_a, g_buf, 80));
    TEST_ASSERT_EQUAL_STRING("", g_buf);
}

/* the summer hour starts the expiry one day earlier near midnight */
static void test_expiry_with_rule(void) {
    uint32_t d = wf_autofill_day("20260701");
    wf_autofill_init(&g_a);
    (void)wf_autofill_tick(&g_a, d - 1U);
    TEST_ASSERT_EQUAL(0, wf_autofill_put(&g_a, "ex.am", "u", "v", "text"));
    TEST_ASSERT_EQUAL(0, wf_autofill_set_expire(&g_a, 1U));
    TEST_ASSERT_EQUAL(0, wf_autofill_set_tz(&g_a, 60));
    /* 22:30 UTC on 1 July: still 1 July in standard time, value kept */
    TEST_ASSERT_EQUAL(0, wf_autofill_tick(&g_a, wf_autofill_day_rule("202607012230", g_a.tz_min, g_a.dst)));
    TEST_ASSERT_EQUAL(1, wf_autofill_count(&g_a, "ex.am", "u"));
    /* with eu, it is 2 July: two days old, forgotten */
    TEST_ASSERT_EQUAL(0, wf_autofill_set_dst(&g_a, WF_AF_DST_EU));
    TEST_ASSERT_EQUAL(1, wf_autofill_tick(&g_a, wf_autofill_day_rule("202607012230", g_a.tz_min, g_a.dst)));
    TEST_ASSERT_EQUAL(0, wf_autofill_count(&g_a, "ex.am", "u"));
}

int main(void) {
    unity_init();
    RUN_TEST(test_rule_names);
    RUN_TEST(test_set_rule);
    RUN_TEST(test_changes_2026);
    RUN_TEST(test_day_moves);
    RUN_TEST(test_file);
    RUN_TEST(test_status);
    RUN_TEST(test_expiry_with_rule);
    unity_print_results();
    unity_cleanup();
    return unity_stats.tests_failed == 0 ? 0 : 1;
}
