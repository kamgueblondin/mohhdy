/* IRQ0 preemption quantum (kernel/sched_quantum.h).
 *
 * Regression for the ipc-foundation CI flake (run 37091978840): the quantum
 * used to be measured from the last GLOBAL preemption, so a task woken by
 * SYS_IPC_SEND after a long idle period was preempted on its first tick. The
 * ipc server was cut after "ipc recv from ", the shell went back to SYS_GETS
 * (kernel frame, never preempted) and the rest of the line never came. The
 * quantum is now measured from the moment the current task was scheduled. */
#include "../../framework/unity.h"
#include <stdint.h>
#include "../../../kernel/sched_quantum.h"

/* A freshly scheduled task keeps the CPU for a whole quantum, whatever the
 * time elapsed since any earlier preemption. */
static void test_fresh_task_gets_full_quantum(void) {
    uint32_t scheduled_at = 100000U; /* long after the last preemption */
    uint32_t now;
    for (now = scheduled_at; now < scheduled_at + TIMER_PREEMPT_QUANTUM; now++) {
        TEST_ASSERT_EQUAL(0, sched_quantum_preempt_due(1, 1, 1, now, scheduled_at));
    }
    TEST_ASSERT_EQUAL(1, sched_quantum_preempt_due(1, 1, 1, now, scheduled_at));
}

/* Only a Ring 3 frame of a user task, with another user task ready. */
static void test_preemption_guards(void) {
    TEST_ASSERT_EQUAL(0, sched_quantum_preempt_due(0, 1, 1, 500U, 0U)); /* kernel frame */
    TEST_ASSERT_EQUAL(0, sched_quantum_preempt_due(1, 0, 1, 500U, 0U)); /* kernel task */
    TEST_ASSERT_EQUAL(0, sched_quantum_preempt_due(1, 1, 0, 500U, 0U)); /* alone */
    TEST_ASSERT_EQUAL(1, sched_quantum_preempt_due(1, 1, 1, 500U, 0U));
}

/* Tick counter wrap-around. */
static void test_quantum_survives_tick_wrap(void) {
    uint32_t scheduled_at = 0xFFFFFFF0U;
    TEST_ASSERT_EQUAL(0, sched_quantum_preempt_due(1, 1, 1, 0x00000003U, scheduled_at));
    TEST_ASSERT_EQUAL(1, sched_quantum_preempt_due(1, 1, 1, 0x00000004U, scheduled_at));
}

/* The ipc-foundation scenario: the shell runs alone for a long time (no
 * preemption), then SYS_IPC_SEND schedules the server, which needs a few
 * ticks to print its line and block again in SYS_IPC_RECEIVE. Every tick
 * that lands while it prints must leave it running. */
static void test_woken_server_finishes_its_line(void) {
    uint32_t woken_at = 4242U;
    uint32_t line_ticks = 3U;
    uint32_t t;
    for (t = 1U; t <= line_ticks; t++) {
        TEST_ASSERT_EQUAL(0, sched_quantum_preempt_due(1, 1, 1, woken_at + t, woken_at));
    }
}

int main(void) {
    unity_init();
    RUN_TEST(test_fresh_task_gets_full_quantum);
    RUN_TEST(test_preemption_guards);
    RUN_TEST(test_quantum_survives_tick_wrap);
    RUN_TEST(test_woken_server_finishes_its_line);
    unity_print_results();
    unity_cleanup();
    return unity_stats.tests_failed == 0 ? 0 : 1;
}
