/* Lot 46: bounded capture of the output of programs launched from a
 * desktop window (kernel/out_capture.c, SYS_OUT_CAPTURE). */
#include "../../framework/unity.h"
#include "kernel/out_capture.h"
#include "include/os_syscalls.h"
#include <string.h>

static char g_rd[OS_OUTCAP_MAX + 64];

static void test_only_descendants_of_the_owner_are_captured(void) {
    out_cap_reset();
    TEST_ASSERT_EQUAL(0, out_cap_put(5, 'x'));            /* no capture open */
    TEST_ASSERT_EQUAL(0, out_cap_begin(1));
    TEST_ASSERT_EQUAL(OS_OUTCAP_BUSY, out_cap_begin(2));   /* one owner at a time */
    TEST_ASSERT_EQUAL(1, out_cap_child(1, 5));              /* child of the owner */
    TEST_ASSERT_EQUAL(1, out_cap_child(5, 6));              /* grandchild */
    TEST_ASSERT_EQUAL(0, out_cap_child(3, 7));              /* unrelated task */
    TEST_ASSERT_EQUAL(0, out_cap_put(1, 'o'));              /* the owner prints as before */
    TEST_ASSERT_EQUAL(0, out_cap_put(7, 'u'));
    TEST_ASSERT_EQUAL(1, out_cap_put(5, 'a'));
    TEST_ASSERT_EQUAL(1, out_cap_put(6, 'b'));
    TEST_ASSERT_EQUAL(OS_OUTCAP_BAD, out_cap_read(5, g_rd, 16));   /* only the owner reads */
    TEST_ASSERT_EQUAL(2, out_cap_read(1, g_rd, 16));
    TEST_ASSERT_EQUAL_STRING("ab", g_rd);
    TEST_ASSERT_EQUAL(0, out_cap_read(1, g_rd, 16));
    TEST_ASSERT_EQUAL(OS_OUTCAP_BAD, out_cap_end(5));
    TEST_ASSERT_EQUAL(0, out_cap_end(1));
    TEST_ASSERT_EQUAL(0, out_cap_put(5, 'c'));              /* closed: console again */
    TEST_ASSERT_EQUAL(0, out_cap_owner());
}

static void test_bound_counts_dropped_bytes(void) {
    int i, n;
    out_cap_reset();
    TEST_ASSERT_EQUAL(0, out_cap_begin(1));
    TEST_ASSERT_EQUAL(1, out_cap_child(1, 9));
    for (i = 0; i < OS_OUTCAP_MAX + 100; i++) TEST_ASSERT_EQUAL(1, out_cap_put(9, (char)('a' + i % 26)));
    TEST_ASSERT_EQUAL(100, (int)out_cap_dropped());
    TEST_ASSERT_EQUAL(OS_OUTCAP_MAX + 100, (int)out_cap_total());
    /* a small reader takes it in pieces, oldest first */
    n = out_cap_read(1, g_rd, 11);
    TEST_ASSERT_EQUAL(10, n);
    TEST_ASSERT_EQUAL_STRING("abcdefghij", g_rd);
    n = out_cap_read(1, g_rd, (int)sizeof(g_rd));
    TEST_ASSERT_EQUAL(OS_OUTCAP_MAX - 10, n);
    TEST_ASSERT_EQUAL('k', g_rd[0]);
    TEST_ASSERT_EQUAL(100, out_cap_end(1));                 /* end reports the cut */
}

static void test_pid_table_is_bounded(void) {
    int k;
    out_cap_reset();
    TEST_ASSERT_EQUAL(OS_OUTCAP_BAD, out_cap_begin(0));
    TEST_ASSERT_EQUAL(0, out_cap_begin(1));
    for (k = 0; k < OS_OUTCAP_PIDS; k++) TEST_ASSERT_EQUAL(1, out_cap_child(1, 100 + k));
    TEST_ASSERT_EQUAL(0, out_cap_child(1, 500));
    TEST_ASSERT_EQUAL(1, (int)out_cap_untracked());
    TEST_ASSERT_EQUAL(OS_OUTCAP_PIDS, out_cap_pid_count());
    TEST_ASSERT_EQUAL(100, out_cap_pid_at(0));
    TEST_ASSERT_EQUAL(0, out_cap_child(1, 1));              /* the owner never joins */
    TEST_ASSERT_EQUAL(0, out_cap_end(1));
    TEST_ASSERT_EQUAL(0, out_cap_pid_count());
}

/* Lot 46 review: a relaunched driver (service registered) leaves the
 * capture, a dead owner releases it at once. */
static void test_daemon_leaves_and_dead_owner_releases(void) {
    out_cap_reset();
    TEST_ASSERT_EQUAL(0, out_cap_begin(1));
    TEST_ASSERT_EQUAL(1, out_cap_child(1, 5));
    TEST_ASSERT_EQUAL(1, out_cap_child(1, 9));
    TEST_ASSERT_EQUAL(2, out_cap_pid_count());
    TEST_ASSERT_EQUAL(1, out_cap_forget(9));                /* networker registered net-driver */
    TEST_ASSERT_EQUAL(1, out_cap_pid_count());
    TEST_ASSERT_EQUAL(0, out_cap_put(9, 'n'));              /* its output: console */
    TEST_ASSERT_EQUAL(0, out_cap_forget(9));
    TEST_ASSERT_EQUAL(1, out_cap_put(5, 'a'));
    TEST_ASSERT_EQUAL(2, out_cap_task_gone(5));             /* a tracked child ended */
    TEST_ASSERT_EQUAL(0, out_cap_task_gone(77));
    TEST_ASSERT_EQUAL(1, out_cap_task_gone(1));             /* the owner shell died */
    TEST_ASSERT_EQUAL(0, out_cap_owner());
    TEST_ASSERT_EQUAL(0, out_cap_begin(2));                 /* free for another shell at once */
    TEST_ASSERT_EQUAL(0, out_cap_end(2));
}

int main(void) {
    unity_init();
    RUN_TEST(test_only_descendants_of_the_owner_are_captured);
    RUN_TEST(test_bound_counts_dropped_bytes);
    RUN_TEST(test_pid_table_is_bounded);
    RUN_TEST(test_daemon_leaves_and_dead_owner_releases);
    unity_print_results();
    unity_cleanup();
    return (unity_stats.tests_failed == 0) ? 0 : 1;
}
