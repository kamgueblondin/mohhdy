/* Tranche 5 suite: NE2000 ownership between the kernel and the Ring 3
 * net-driver worker (kernel/net_nic_owner.c, pure logic). */
#include "../../framework/unity.h"
#include "../../../kernel/net_nic_owner.h"

static void test_claim_rules(void) {
    nic_owner_init();
    TEST_ASSERT_EQUAL(0, (int)nic_owner_pid());
    TEST_ASSERT_TRUE(nic_owner_kernel_may_touch());
    /* Only the live net-driver worker may claim, and only a probed card. */
    TEST_ASSERT_EQUAL(OS_NET_WORKER_REQUIRED, nic_owner_claim(5, 3, 1));
    TEST_ASSERT_EQUAL(OS_NET_WORKER_REQUIRED, nic_owner_claim(0, 0, 1));
    TEST_ASSERT_EQUAL(OS_NET_WORKER_REQUIRED, nic_owner_claim(-1, -1, 1));
    TEST_ASSERT_EQUAL(OS_NET_NIC_ABSENT, nic_owner_claim(3, 3, 0));
    TEST_ASSERT_EQUAL(0, (int)nic_owner_pid());
    TEST_ASSERT_EQUAL(0, nic_owner_claim(3, 3, 1));
    TEST_ASSERT_EQUAL(3, (int)nic_owner_pid());
    TEST_ASSERT_FALSE(nic_owner_kernel_may_touch());
    /* Idempotent for the owner, refused for anybody else. */
    TEST_ASSERT_EQUAL(0, nic_owner_claim(3, 3, 1));
    TEST_ASSERT_EQUAL(OS_NET_WORKER_REQUIRED, nic_owner_claim(7, 7, 1));
    {
        os_net_nic_status_t st;
        nic_owner_fill_status(&st);
        TEST_ASSERT_EQUAL(1, (int)st.claims);
        TEST_ASSERT_EQUAL(3, (int)st.owner_pid);
    }
}

static void test_ports_follow_owner(void) {
    nic_owner_init();
    TEST_ASSERT_FALSE(nic_owner_ports_open(3, 3)); /* nobody claimed */
    TEST_ASSERT_EQUAL(0, nic_owner_claim(3, 3, 1));
    TEST_ASSERT_TRUE(nic_owner_ports_open(3, 3));
    TEST_ASSERT_FALSE(nic_owner_ports_open(4, 3)); /* another task */
    TEST_ASSERT_FALSE(nic_owner_ports_open(1, 3)); /* the shell */
    TEST_ASSERT_FALSE(nic_owner_ports_open(3, 8)); /* no longer the live worker */
    TEST_ASSERT_FALSE(nic_owner_ports_open(3, 0));
}

static void test_drop_and_reclaim(void) {
    nic_owner_init();
    TEST_ASSERT_EQUAL(0, nic_owner_drop_if_gone(0)); /* kernel already owns it */
    TEST_ASSERT_EQUAL(0, nic_owner_claim(3, 3, 1));
    TEST_ASSERT_EQUAL(0, nic_owner_drop_if_gone(3)); /* still alive */
    TEST_ASSERT_EQUAL(3, (int)nic_owner_pid());
    TEST_ASSERT_EQUAL(1, nic_owner_drop_if_gone(0)); /* worker gone */
    TEST_ASSERT_EQUAL(0, (int)nic_owner_pid());
    TEST_ASSERT_TRUE(nic_owner_kernel_may_touch());
    TEST_ASSERT_EQUAL(0, nic_owner_drop_if_gone(0)); /* reclaim happens once */
    nic_owner_note_reclaim();
    /* A new worker (new pid) can claim again. */
    TEST_ASSERT_EQUAL(0, nic_owner_claim(9, 9, 1));
    TEST_ASSERT_EQUAL(1, nic_owner_drop_if_gone(12)); /* replaced by another worker */
    {
        os_net_nic_status_t st;
        nic_owner_fill_status(&st);
        TEST_ASSERT_EQUAL(2, (int)st.claims);
        TEST_ASSERT_EQUAL(1, (int)st.reclaims);
        TEST_ASSERT_EQUAL(0, (int)st.owner_pid);
    }
}

static void test_irq_forwarding(void) {
    nic_owner_init();
    /* Kernel owner: IRQ3 is serviced in Ring 0. */
    TEST_ASSERT_EQUAL(0, nic_owner_irq());
    TEST_ASSERT_EQUAL(0, (int)nic_owner_irq_take(3));
    TEST_ASSERT_EQUAL(0, nic_owner_claim(3, 3, 1));
    TEST_ASSERT_EQUAL(1, nic_owner_irq());
    TEST_ASSERT_EQUAL(1, nic_owner_irq());
    TEST_ASSERT_EQUAL(0, (int)nic_owner_irq_take(4)); /* not the owner */
    TEST_ASSERT_EQUAL(2, (int)nic_owner_irq_take(3));
    TEST_ASSERT_EQUAL(0, (int)nic_owner_irq_take(3)); /* taken once */
    TEST_ASSERT_EQUAL(1, nic_owner_irq());
    TEST_ASSERT_EQUAL(1, nic_owner_drop_if_gone(0)); /* pending events die with it */
    TEST_ASSERT_EQUAL(0, nic_owner_claim(5, 5, 1));
    TEST_ASSERT_EQUAL(0, (int)nic_owner_irq_take(5));
    {
        os_net_nic_status_t st;
        nic_owner_fill_status(&st);
        TEST_ASSERT_EQUAL(3, (int)st.irq_forwarded);
    }
}

static void test_counters(void) {
    os_net_nic_status_t st;
    nic_owner_init();
    nic_owner_note_kernel_refused();
    nic_owner_note_kernel_gated();
    nic_owner_note_kernel_gated();
    nic_owner_note_pump(2U, 1U, 2U, 0U);
    nic_owner_note_pump(1U, 0U, 0U, 1U);
    nic_owner_fill_status(&st);
    TEST_ASSERT_EQUAL(1, (int)st.kernel_refused);
    TEST_ASSERT_EQUAL(2, (int)st.kernel_gated);
    TEST_ASSERT_EQUAL(2, (int)st.pumps);
    TEST_ASSERT_EQUAL(3, (int)st.frames_out);
    TEST_ASSERT_EQUAL(1, (int)st.frames_in);
    TEST_ASSERT_EQUAL(2, (int)st.worker_tx_ok);
    TEST_ASSERT_EQUAL(1, (int)st.worker_tx_failed);
    nic_owner_fill_status(0); /* no crash */
    nic_owner_init();
    nic_owner_fill_status(&st);
    TEST_ASSERT_EQUAL(0, (int)st.pumps);
    TEST_ASSERT_EQUAL(0, (int)st.kernel_refused);
}

static void test_abi(void) {
    /* Window and pump ABI shared with userspace/net_worker.c. */
    TEST_ASSERT_EQUAL(0x300, (int)OS_NET_NIC_BASE_PORT);
    TEST_ASSERT_EQUAL(0x31F, (int)OS_NET_NIC_LAST_PORT);
    TEST_ASSERT_EQUAL(3, (int)OS_NET_NIC_IRQ_LINE);
    TEST_ASSERT_TRUE(SYS_NET_NIC < MAX_SYSCALLS);
    TEST_ASSERT_TRUE(OS_NET_NIC_WORKER_OWNED != OS_NET_WORKER_REQUIRED);
    TEST_ASSERT_TRUE(OS_NET_WIRE_PENDING != OS_NET_WIRE_UNAVAILABLE);
    TEST_ASSERT_TRUE(OS_NET_NIC_ABSENT != OS_NET_WIRE_UNAVAILABLE);
    TEST_ASSERT_TRUE(OS_NET_NIC_FRAME_MAX >= 1514U);
}

/* Tranche 5 pile: the Ring 3 stack publishes its counters. */
static void test_stack_publish(void) {
    os_net_stack_report_t r, got;
    os_net_wire_status_t w;
    uint32_t i;
    uint8_t* p = (uint8_t*)&r;
    for (i = 0U; i < sizeof(r); i++) p[i] = 0U;
    nic_owner_init();
    r.wire.connects = 1U; r.wire.frames_tx = 7U; r.wire.frames_rx = 4U; r.wire.bound = 1U;
    r.llm_status = 0x1FU; r.socket_ops = 3U;
    /* Nobody owns the card: publishing is refused, nothing to report. */
    TEST_ASSERT_EQUAL(OS_NET_WORKER_REQUIRED, nic_owner_publish(3, &r));
    TEST_ASSERT_EQUAL(0, nic_owner_stack(&got));
    TEST_ASSERT_EQUAL(0x5U, nic_owner_llm_status(0x5U));
    TEST_ASSERT_EQUAL(0, nic_owner_claim(3, 3, 1));
    TEST_ASSERT_EQUAL(OS_NET_WORKER_REQUIRED, nic_owner_publish(4, &r)); /* not the owner */
    TEST_ASSERT_EQUAL(OS_NET_WORKER_REQUIRED, nic_owner_publish(3, 0));
    TEST_ASSERT_EQUAL(0x5U, nic_owner_llm_status(0x5U)); /* owner, not published yet */
    TEST_ASSERT_EQUAL(0, nic_owner_publish(3, &r));
    TEST_ASSERT_EQUAL(1, nic_owner_stack(&got));
    TEST_ASSERT_EQUAL(3, (int)got.wire.worker_pid);
    TEST_ASSERT_EQUAL(1, (int)got.reports);
    TEST_ASSERT_EQUAL(3, (int)got.socket_ops);
    TEST_ASSERT_EQUAL(0x1FU, nic_owner_llm_status(0x5U));
    /* Kernel counters + live Ring 3 counters. */
    for (i = 0U, p = (uint8_t*)&w; i < sizeof(w); i++) p[i] = 0U;
    w.refused = 2U;
    nic_owner_merge_wire(&w);
    TEST_ASSERT_EQUAL(7, (int)w.frames_tx);
    TEST_ASSERT_EQUAL(2, (int)w.refused);
    TEST_ASSERT_EQUAL(1, (int)w.bound);
    /* Owner lost: counters retire (totals never go backwards), bound and
     * the session word do not survive. */
    TEST_ASSERT_EQUAL(1, nic_owner_drop_if_gone(0));
    TEST_ASSERT_EQUAL(0, nic_owner_stack(&got));
    TEST_ASSERT_EQUAL(1, (int)got.reports);
    TEST_ASSERT_EQUAL(0x5U, nic_owner_llm_status(0x5U));
    for (i = 0U, p = (uint8_t*)&w; i < sizeof(w); i++) p[i] = 0U;
    nic_owner_merge_wire(&w);
    TEST_ASSERT_EQUAL(7, (int)w.frames_tx);
    TEST_ASSERT_EQUAL(1, (int)w.connects);
    TEST_ASSERT_EQUAL(0, (int)w.bound);
    /* A new owner adds on top of the retired base. */
    TEST_ASSERT_EQUAL(0, nic_owner_claim(9, 9, 1));
    r.wire.frames_tx = 3U;
    TEST_ASSERT_EQUAL(0, nic_owner_publish(9, &r));
    for (i = 0U, p = (uint8_t*)&w; i < sizeof(w); i++) p[i] = 0U;
    nic_owner_merge_wire(&w);
    TEST_ASSERT_EQUAL(10, (int)w.frames_tx);
    TEST_ASSERT_EQUAL(2, (int)w.connects);
    nic_owner_merge_wire(0);
}

int main(void) {
    unity_init();
    RUN_TEST(test_claim_rules);
    RUN_TEST(test_ports_follow_owner);
    RUN_TEST(test_drop_and_reclaim);
    RUN_TEST(test_irq_forwarding);
    RUN_TEST(test_counters);
    RUN_TEST(test_abi);
    RUN_TEST(test_stack_publish);
    unity_print_results();
    unity_cleanup();
    return unity_stats.tests_failed == 0 ? 0 : 1;
}
