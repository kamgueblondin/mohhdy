/* SYS_IPC_RECV_WAIT state machine (kernel/ipc_wait.c).
 *
 * The kernel glue (kernel/task/task.c, kernel/syscall/syscall.c) only maps
 * these decisions to TASK_BLOCKED_IPC / TASK_READY; the QEMU contract
 * qemu-ipc-foundation checks the integrated behaviour. */
#include "../../framework/unity.h"
#include <stdint.h>
#include <string.h>
#include "../../../include/os_syscalls.h"
#include "../../../kernel/ipc_wait.c"

static ipc_wait_t w;

static void reset(void) { memset(&w, 0, sizeof(w)); }

/* A pending message or an error answers at once; timeout 0 is a poll; a
 * task that may not block (kernel task, ATA RPC) gets the poll result. */
static void test_decide_returns_or_blocks(void) {
    TEST_ASSERT_EQUAL(IPC_WAIT_RETURN, ipc_wait_decide(0, OS_IPC_EMPTY, 100U, 1));
    TEST_ASSERT_EQUAL(IPC_WAIT_RETURN, ipc_wait_decide(OS_IPC_BAD_MESSAGE, OS_IPC_EMPTY, 100U, 1));
    TEST_ASSERT_EQUAL(IPC_WAIT_RETURN, ipc_wait_decide(OS_IPC_EMPTY, OS_IPC_EMPTY, 0U, 1));
    TEST_ASSERT_EQUAL(IPC_WAIT_RETURN, ipc_wait_decide(OS_IPC_EMPTY, OS_IPC_EMPTY, 100U, 0));
    TEST_ASSERT_EQUAL(IPC_WAIT_BLOCK, ipc_wait_decide(OS_IPC_EMPTY, OS_IPC_EMPTY, 100U, 1));
    TEST_ASSERT_EQUAL(IPC_WAIT_BLOCK,
                      ipc_wait_decide(OS_IPC_EMPTY, OS_IPC_EMPTY, OS_IPC_WAIT_FOREVER, 1));
}

/* A blocked waiter stays asleep on every tick before its deadline, so the
 * scheduler never selects it (no CPU switch while idle). */
static void test_blocked_waiter_sleeps_until_message(void) {
    int rc = 0;
    uint32_t t;
    reset();
    ipc_wait_begin(&w, 1000U, OS_IPC_WAIT_FOREVER, OS_IPC_WAIT_FOREVER);
    TEST_ASSERT_EQUAL(1, w.blocked);
    for (t = 1000U; t < 1000U + 100000U; t += 97U) {
        TEST_ASSERT_EQUAL(0, ipc_wait_on_tick(&w, t));
    }
    TEST_ASSERT_EQUAL(1, ipc_wait_on_message(&w));
    TEST_ASSERT_EQUAL(0, w.blocked);
    TEST_ASSERT_EQUAL(0, ipc_wait_on_message(&w)); /* second message: already ready */
    TEST_ASSERT_EQUAL(IPC_WAIT_RETURN,
                      ipc_wait_after_wake(&w, 0, OS_IPC_EMPTY, OS_IPC_TIMEOUT, 200000U, &rc));
    TEST_ASSERT_EQUAL(0, rc);
    TEST_ASSERT_EQUAL(1U, w.waits);
    TEST_ASSERT_EQUAL(1U, w.wakes);
    TEST_ASSERT_EQUAL(0U, w.timeouts);
}

/* Deadline path: ready exactly at now + timeout, then OS_IPC_TIMEOUT. */
static void test_deadline_wakes_with_timeout(void) {
    int rc = 0;
    reset();
    ipc_wait_begin(&w, 500U, 50U, OS_IPC_WAIT_FOREVER);
    TEST_ASSERT_EQUAL(0, ipc_wait_on_tick(&w, 549U));
    TEST_ASSERT_EQUAL(1, ipc_wait_on_tick(&w, 550U));
    TEST_ASSERT_EQUAL(0, ipc_wait_on_tick(&w, 551U)); /* only once */
    TEST_ASSERT_EQUAL(IPC_WAIT_RETURN,
                      ipc_wait_after_wake(&w, OS_IPC_EMPTY, OS_IPC_EMPTY, OS_IPC_TIMEOUT, 552U, &rc));
    TEST_ASSERT_EQUAL(OS_IPC_TIMEOUT, rc);
    TEST_ASSERT_EQUAL(1U, w.timeouts);
}

/* A message that raced with the deadline wins over the timeout. */
static void test_message_wins_over_deadline(void) {
    int rc = 0;
    reset();
    ipc_wait_begin(&w, 10U, 5U, OS_IPC_WAIT_FOREVER);
    TEST_ASSERT_EQUAL(1, ipc_wait_on_tick(&w, 15U));
    TEST_ASSERT_EQUAL(IPC_WAIT_RETURN,
                      ipc_wait_after_wake(&w, 0, OS_IPC_EMPTY, OS_IPC_TIMEOUT, 16U, &rc));
    TEST_ASSERT_EQUAL(0, rc);
}

/* Spurious wake-up before the deadline: block again with the SAME
 * deadline (not a fresh timeout). After the deadline: timeout. */
static void test_spurious_wake_keeps_deadline(void) {
    int rc = 0;
    reset();
    ipc_wait_begin(&w, 100U, 20U, OS_IPC_WAIT_FOREVER);
    w.blocked = 0U; /* made READY by something else */
    TEST_ASSERT_EQUAL(IPC_WAIT_BLOCK,
                      ipc_wait_after_wake(&w, OS_IPC_EMPTY, OS_IPC_EMPTY, OS_IPC_TIMEOUT, 110U, &rc));
    TEST_ASSERT_EQUAL(1, w.blocked);
    TEST_ASSERT_EQUAL(120U, w.deadline);
    w.blocked = 0U;
    TEST_ASSERT_EQUAL(IPC_WAIT_RETURN,
                      ipc_wait_after_wake(&w, OS_IPC_EMPTY, OS_IPC_EMPTY, OS_IPC_TIMEOUT, 120U, &rc));
    TEST_ASSERT_EQUAL(OS_IPC_TIMEOUT, rc);
}

/* Tick counter wrap-around does not expire a wait early or late. */
static void test_deadline_survives_tick_wrap(void) {
    reset();
    ipc_wait_begin(&w, 0xFFFFFFF0U, 0x20U, OS_IPC_WAIT_FOREVER);
    TEST_ASSERT_EQUAL(0, ipc_wait_on_tick(&w, 0xFFFFFFFFU));
    TEST_ASSERT_EQUAL(0, ipc_wait_on_tick(&w, 0x0000000FU));
    TEST_ASSERT_EQUAL(1, ipc_wait_on_tick(&w, 0x00000010U));
}

/* A message for a task that is not blocked, or for a cancelled wait
 * (killed waiter), wakes nothing. */
static void test_message_without_waiter_and_cancel(void) {
    reset();
    TEST_ASSERT_EQUAL(0, ipc_wait_on_message(&w));
    ipc_wait_begin(&w, 0U, OS_IPC_WAIT_FOREVER, OS_IPC_WAIT_FOREVER);
    ipc_wait_cancel(&w);
    TEST_ASSERT_EQUAL(0, ipc_wait_on_message(&w));
    TEST_ASSERT_EQUAL(0, ipc_wait_on_tick(&w, 0xFFFFFFFFU));
    TEST_ASSERT_EQUAL(1U, w.waits);
    TEST_ASSERT_EQUAL(0U, w.wakes);
}

/* ABI constants stay consistent. */
static void test_abi_constants(void) {
    TEST_ASSERT_EQUAL(154, SYS_IPC_RECV_WAIT);
    TEST_ASSERT_TRUE(SYS_IPC_RECV_WAIT < MAX_SYSCALLS);
    TEST_ASSERT_EQUAL(155, MAX_SYSCALLS);
    TEST_ASSERT_TRUE(OS_IPC_TIMEOUT != OS_IPC_EMPTY);
    TEST_ASSERT_TRUE(OS_IPC_TIMEOUT != OS_IPC_SERVICE_FULL);
}

int main(void) {
    unity_init();
    RUN_TEST(test_decide_returns_or_blocks);
    RUN_TEST(test_blocked_waiter_sleeps_until_message);
    RUN_TEST(test_deadline_wakes_with_timeout);
    RUN_TEST(test_message_wins_over_deadline);
    RUN_TEST(test_spurious_wake_keeps_deadline);
    RUN_TEST(test_deadline_survives_tick_wrap);
    RUN_TEST(test_message_without_waiter_and_cancel);
    RUN_TEST(test_abi_constants);
    unity_print_results();
    unity_cleanup();
    return unity_stats.tests_failed == 0 ? 0 : 1;
}
