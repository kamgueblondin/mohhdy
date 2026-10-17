/* Lot 5 (US-001): bookkeeping of the Ring 3 PS/2 and COM1 drivers. */
#include "../../framework/unity.h"
#include "../../../kernel/device_service.h"

static void test_owner_handoff_and_respawn_budget(void) {
    os_device_status_t st;
    uint32_t i;
    devsvc_reset();
    TEST_ASSERT_EQUAL(0, devsvc_owner(DEV_PS2));
    TEST_ASSERT_EQUAL(0, devsvc_handed_off(DEV_PS2));
    /* never claimed: no respawn (boot spawn failed, Ring 0 keeps the device) */
    TEST_ASSERT_EQUAL(0, devsvc_should_respawn(DEV_PS2, 0));
    devsvc_set_owner(DEV_PS2, 4);
    TEST_ASSERT_EQUAL(4, devsvc_owner(DEV_PS2));
    TEST_ASSERT_EQUAL(1, devsvc_handed_off(DEV_PS2));
    TEST_ASSERT_EQUAL(0, devsvc_should_respawn(DEV_PS2, 0));
    TEST_ASSERT_EQUAL(0, devsvc_owner(DEV_SERIAL));
    devsvc_owner_gone(DEV_PS2);
    TEST_ASSERT_EQUAL(0, devsvc_owner(DEV_PS2));
    TEST_ASSERT_EQUAL(1, devsvc_handed_off(DEV_PS2));
    TEST_ASSERT_EQUAL(0, devsvc_should_respawn(DEV_PS2, 1)); /* one is starting */
    for (i = 0; i < DEVICE_RESPAWN_LIMIT; i++) {
        TEST_ASSERT_EQUAL(1, devsvc_should_respawn(DEV_PS2, 0));
        devsvc_note_respawn(DEV_PS2);
    }
    TEST_ASSERT_EQUAL(0, devsvc_should_respawn(DEV_PS2, 0));
    TEST_ASSERT_EQUAL(0, devsvc_should_respawn(5, 0));
    devsvc_status(&st);
    TEST_ASSERT_EQUAL(DEVICE_RESPAWN_LIMIT, st.ps2_respawns);
    TEST_ASSERT_EQUAL(0U, st.serial_respawns);
    TEST_ASSERT_EQUAL(1U, st.ps2_handed_off);
    TEST_ASSERT_EQUAL(0U, st.serial_handed_off);
}

static void test_ps2_irq_sequence_and_served(void) {
    os_device_status_t st;
    uint32_t s1, s2;
    devsvc_reset();
    devsvc_set_owner(DEV_PS2, 4);
    TEST_ASSERT_EQUAL(0, devsvc_ps2_pending());
    TEST_ASSERT_EQUAL(0, devsvc_wants_cpu(DEV_PS2, 0));
    s1 = devsvc_ps2_irq(100);
    TEST_ASSERT_EQUAL(1, devsvc_ps2_pending());
    TEST_ASSERT_EQUAL(1, devsvc_wants_cpu(DEV_PS2, 100));
    s2 = devsvc_ps2_irq(101);
    TEST_ASSERT_EQUAL(s1 + 1U, s2);
    /* the driver read after s1 only: a newer IRQ keeps it pending */
    TEST_ASSERT_EQUAL(0, devsvc_ps2_served(s1, 1, 102));
    TEST_ASSERT_EQUAL(1, devsvc_ps2_pending());
    TEST_ASSERT_EQUAL(1, devsvc_ps2_served(s2, 2, 103));
    TEST_ASSERT_EQUAL(0, devsvc_ps2_pending());
    devsvc_status(&st);
    TEST_ASSERT_EQUAL(2U, st.ps2_irqs);
    TEST_ASSERT_EQUAL(3U, st.ps2_events);
    TEST_ASSERT_EQUAL(0U, st.ps2_stalls);
}

static void test_ps2_stall_hands_back_to_ring0(void) {
    os_device_status_t st;
    devsvc_reset();
    devsvc_set_owner(DEV_PS2, 4);
    (void)devsvc_ps2_irq(200);
    TEST_ASSERT_EQUAL(0U, devsvc_watchdog(200 + DEVICE_STALL_TICKS - 1U) & DEVSVC_W_PS2_DRAIN);
    TEST_ASSERT_EQUAL(DEVSVC_W_PS2_DRAIN, devsvc_watchdog(200 + DEVICE_STALL_TICKS) & DEVSVC_W_PS2_DRAIN);
    TEST_ASSERT_EQUAL(0, devsvc_ps2_pending());
    devsvc_ps2_note_ring0(3);
    /* no owner: the IRQ handler reads itself, the watchdog never drains */
    devsvc_owner_gone(DEV_PS2);
    (void)devsvc_ps2_irq(300);
    TEST_ASSERT_EQUAL(0U, devsvc_watchdog(400) & DEVSVC_W_PS2_DRAIN);
    TEST_ASSERT_EQUAL(0, devsvc_wants_cpu(DEV_PS2, 400));
    devsvc_status(&st);
    TEST_ASSERT_EQUAL(1U, st.ps2_stalls);
    TEST_ASSERT_EQUAL(3U, st.ps2_ring0_bytes);
}

static void test_serial_direct_before_claim_and_queue_order(void) {
    uint8_t out[8];
    uint8_t c;
    devsvc_reset();
    /* early boot: Ring 0 writes itself */
    TEST_ASSERT_EQUAL(DEVSVC_DIRECT, devsvc_serial_put('a'));
    devsvc_set_owner(DEV_SERIAL, 5);
    TEST_ASSERT_EQUAL(DEVSVC_QUEUED, devsvc_serial_put('h'));
    TEST_ASSERT_EQUAL(DEVSVC_QUEUED, devsvc_serial_put('i'));
    TEST_ASSERT_EQUAL(DEVSVC_QUEUED, devsvc_serial_put('!'));
    TEST_ASSERT_EQUAL(3U, devsvc_serial_queued());
    TEST_ASSERT_EQUAL(2U, devsvc_serial_take(out, 2, 10));
    TEST_ASSERT_EQUAL('h', out[0]);
    TEST_ASSERT_EQUAL('i', out[1]);
    /* chunk in flight: the driver is writing, no stall yet */
    TEST_ASSERT_EQUAL(0, devsvc_wants_cpu(DEV_SERIAL, 30));
    TEST_ASSERT_EQUAL(0U, devsvc_watchdog(30) & DEVSVC_W_SERIAL_FLUSH);
    devsvc_serial_done(31);
    TEST_ASSERT_EQUAL(1U, devsvc_serial_take(out, 8, 31));
    TEST_ASSERT_EQUAL('!', out[0]);
    TEST_ASSERT_EQUAL(0U, devsvc_serial_take(out, 8, 32));
    TEST_ASSERT_EQUAL(0, devsvc_serial_pop_ring0(&c));
}

static void test_serial_stall_death_and_overflow(void) {
    os_device_status_t st;
    uint8_t c;
    uint32_t i, n = 0;
    devsvc_reset();
    devsvc_set_owner(DEV_SERIAL, 5);
    (void)devsvc_serial_take(&c, 1, 0);       /* last take at 0, nothing */
    TEST_ASSERT_EQUAL(DEVSVC_QUEUED, devsvc_serial_put('x'));
    TEST_ASSERT_EQUAL(0, devsvc_wants_cpu(DEV_SERIAL, 1));
    TEST_ASSERT_EQUAL(1, devsvc_wants_cpu(DEV_SERIAL, 2));
    TEST_ASSERT_EQUAL(0U, devsvc_watchdog(DEVICE_STALL_TICKS - 1U) & DEVSVC_W_SERIAL_FLUSH);
    TEST_ASSERT_EQUAL(DEVSVC_W_SERIAL_FLUSH, devsvc_watchdog(DEVICE_STALL_TICKS) & DEVSVC_W_SERIAL_FLUSH);
    TEST_ASSERT_EQUAL(1, devsvc_serial_pop_ring0(&c));
    TEST_ASSERT_EQUAL('x', c);
    /* a chunk taken but not completed: the stall is counted, but Ring 0
     * never prints the later bytes ahead of it (order is kept) */
    TEST_ASSERT_EQUAL(DEVSVC_QUEUED, devsvc_serial_put('y'));
    TEST_ASSERT_EQUAL(DEVSVC_QUEUED, devsvc_serial_put('z'));
    TEST_ASSERT_EQUAL(1U, devsvc_serial_take(&c, 1, 100));
    TEST_ASSERT_EQUAL(0U, devsvc_watchdog(100 + DEVICE_INFLIGHT_STALE - 1U) & DEVSVC_W_SERIAL_FLUSH);
    TEST_ASSERT_EQUAL(0U, devsvc_watchdog(100 + DEVICE_INFLIGHT_STALE) & DEVSVC_W_SERIAL_FLUSH);
    TEST_ASSERT_EQUAL(0U, devsvc_watchdog(100 + 3U * DEVICE_INFLIGHT_STALE / 2U) & DEVSVC_W_SERIAL_FLUSH);
    /* owner gone with leftovers: they go out first, in order */
    devsvc_owner_gone(DEV_SERIAL);
    TEST_ASSERT_EQUAL(DEVSVC_FLUSH, devsvc_serial_put('w'));
    TEST_ASSERT_EQUAL(1, devsvc_serial_pop_ring0(&c));
    TEST_ASSERT_EQUAL('z', c);
    TEST_ASSERT_EQUAL(DEVSVC_DIRECT, devsvc_serial_put('w'));
    /* full ring while the driver lives: FLUSH verdict */
    devsvc_set_owner(DEV_SERIAL, 6);
    for (i = 0; i < DEVICE_SERIAL_RING; i++) {
        int v = devsvc_serial_put((uint8_t)i);
        if (v == DEVSVC_FLUSH) break;
        n++;
    }
    TEST_ASSERT_EQUAL(DEVICE_SERIAL_RING - 1U, n);
    devsvc_serial_note_overflow();
    devsvc_serial_note_rx(4);
    devsvc_status(&st);
    TEST_ASSERT_EQUAL(6, st.serial_owner);
    TEST_ASSERT_EQUAL(2U, st.serial_stalls);
    TEST_ASSERT_EQUAL(1U, st.serial_overflows);
    TEST_ASSERT_EQUAL(4U, st.serial_rx_bytes);
    TEST_ASSERT_EQUAL(2U, st.serial_ring0_bytes);
    TEST_ASSERT_EQUAL(1U, st.serial_ring3_bytes);
}

static void test_owner_check_period(void) {
    devsvc_reset();
    TEST_ASSERT_EQUAL(0U, devsvc_watchdog(20) & DEVSVC_W_CHECK_OWNERS); /* nothing owned */
    devsvc_set_owner(DEV_SERIAL, 5);
    TEST_ASSERT_EQUAL(DEVSVC_W_CHECK_OWNERS, devsvc_watchdog(40) & DEVSVC_W_CHECK_OWNERS);
    TEST_ASSERT_EQUAL(0U, devsvc_watchdog(45) & DEVSVC_W_CHECK_OWNERS);
    TEST_ASSERT_EQUAL(DEVSVC_W_CHECK_OWNERS, devsvc_watchdog(50) & DEVSVC_W_CHECK_OWNERS);
}

int main(void) {
    unity_init();
    RUN_TEST(test_owner_handoff_and_respawn_budget);
    RUN_TEST(test_ps2_irq_sequence_and_served);
    RUN_TEST(test_ps2_stall_hands_back_to_ring0);
    RUN_TEST(test_serial_direct_before_claim_and_queue_order);
    RUN_TEST(test_serial_stall_death_and_overflow);
    RUN_TEST(test_owner_check_period);
    unity_print_results();
    unity_cleanup();
    return unity_stats.tests_failed == 0 ? 0 : 1;
}
