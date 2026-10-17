#include "../../framework/unity.h"
#include "../../../kernel/usb_dma.h"
#include "../../../kernel/io_bitmap.h"

#define TEST_ASSERT_TRUE_U32(e, a) TEST_ASSERT_TRUE((uint32_t)(e) == (uint32_t)(a))
#define DATA_PHYS 0x00500000U
#define SCHED_PHYS 0x00501000U

void setUp(void) {}
void tearDown(void) {}

static os_usb_td_req_t req(uint8_t slot, uint8_t pid, uint16_t off, uint16_t len, uint8_t next) {
    os_usb_td_req_t r;
    r.slot = slot; r.pid = pid; r.dev_addr = 2U; r.endpoint = 1U; r.toggle = 1U; r.low_speed = 0U;
    r.next = next; r.pad = 0U; r.buf_offset = off; r.length = len;
    return r;
}

void test_td_words_match_uhci_layout(void) {
    os_usb_td_req_t r = req(2U, OS_USB_PID_IN, 0x100U, 8U, OS_USB_TD_END);
    os_usb_td_t td;
    TEST_ASSERT_EQUAL(USBDMA_OK, usbdma_build_td(&r, DATA_PHYS, SCHED_PHYS, &td));
    TEST_ASSERT_TRUE_U32(USBDMA_LINK_TERM, td.link);
    TEST_ASSERT_TRUE_U32(OS_USB_TD_ACTIVE | (3U << 27), td.status);
    TEST_ASSERT_TRUE_U32((7U << 21) | (1U << 19) | (1U << 15) | (2U << 8) | 0x69U, td.token);
    TEST_ASSERT_TRUE_U32(DATA_PHYS + 0x100U, td.buffer);
    r = req(0U, OS_USB_PID_SETUP, 0U, 8U, 1U);
    TEST_ASSERT_EQUAL(USBDMA_OK, usbdma_build_td(&r, DATA_PHYS, SCHED_PHYS, &td));
    TEST_ASSERT_TRUE_U32((SCHED_PHYS + 16U + 32U) | USBDMA_LINK_DEPTH, td.link);
    r = req(1U, OS_USB_PID_IN, 0U, 0U, OS_USB_TD_END);
    TEST_ASSERT_EQUAL(USBDMA_OK, usbdma_build_td(&r, DATA_PHYS, SCHED_PHYS, &td));
    TEST_ASSERT_TRUE_U32(0x7FFU, td.token >> 21);
    TEST_ASSERT_TRUE_U32(0U, td.buffer);
}

/* The controller may only DMA into the driver's data page. */
void test_td_buffer_never_leaves_the_data_page(void) {
    os_usb_td_req_t r;
    os_usb_td_t td;
    r = req(0U, OS_USB_PID_IN, OS_USB_DATA_SIZE - 8U, 8U, OS_USB_TD_END);
    TEST_ASSERT_EQUAL(USBDMA_OK, usbdma_build_td(&r, DATA_PHYS, SCHED_PHYS, &td));
    r = req(0U, OS_USB_PID_IN, OS_USB_DATA_SIZE - 7U, 8U, OS_USB_TD_END);
    TEST_ASSERT_EQUAL(USBDMA_E_BOUNDS, usbdma_build_td(&r, DATA_PHYS, SCHED_PHYS, &td));
    r = req(0U, OS_USB_PID_IN, 0xFFFFU, 8U, OS_USB_TD_END);
    TEST_ASSERT_EQUAL(USBDMA_E_BOUNDS, usbdma_build_td(&r, DATA_PHYS, SCHED_PHYS, &td));
    r = req(0U, OS_USB_PID_IN, 0U, OS_USB_TD_MAX_LEN + 1U, OS_USB_TD_END);
    TEST_ASSERT_EQUAL(USBDMA_E_BOUNDS, usbdma_build_td(&r, DATA_PHYS, SCHED_PHYS, &td));
}

void test_td_rejects_bad_slot_pid_address_link(void) {
    os_usb_td_req_t r;
    os_usb_td_t td;
    r = req(OS_USB_TD_SLOTS, OS_USB_PID_IN, 0U, 8U, OS_USB_TD_END);
    TEST_ASSERT_EQUAL(USBDMA_E_SLOT, usbdma_build_td(&r, DATA_PHYS, SCHED_PHYS, &td));
    r = req(0U, 0x55U, 0U, 8U, OS_USB_TD_END);
    TEST_ASSERT_EQUAL(USBDMA_E_PID, usbdma_build_td(&r, DATA_PHYS, SCHED_PHYS, &td));
    r = req(0U, OS_USB_PID_IN, 0U, 8U, OS_USB_TD_END); r.dev_addr = 128U;
    TEST_ASSERT_EQUAL(USBDMA_E_ADDR, usbdma_build_td(&r, DATA_PHYS, SCHED_PHYS, &td));
    r = req(0U, OS_USB_PID_IN, 0U, 8U, OS_USB_TD_END); r.toggle = 2U;
    TEST_ASSERT_EQUAL(USBDMA_E_ADDR, usbdma_build_td(&r, DATA_PHYS, SCHED_PHYS, &td));
    r = req(3U, OS_USB_PID_IN, 0U, 8U, 3U);
    TEST_ASSERT_EQUAL(USBDMA_E_LINK, usbdma_build_td(&r, DATA_PHYS, SCHED_PHYS, &td));
    r = req(3U, OS_USB_PID_IN, 0U, 8U, OS_USB_TD_SLOTS);
    TEST_ASSERT_EQUAL(USBDMA_E_LINK, usbdma_build_td(&r, DATA_PHYS, SCHED_PHYS, &td));
}

static void put(os_usb_sched_t* s, uint8_t slot, uint8_t next) {
    os_usb_td_req_t r = req(slot, OS_USB_PID_IN, (uint16_t)(slot * 8U), 8U, next);
    TEST_ASSERT_EQUAL(USBDMA_OK, usbdma_build_td(&r, DATA_PHYS, SCHED_PHYS, &s->td[slot]));
}

void test_chain_check_counts_and_refuses_cycles(void) {
    static os_usb_sched_t s;
    put(&s, 0U, 1U); put(&s, 1U, 2U); put(&s, 2U, OS_USB_TD_END);
    TEST_ASSERT_EQUAL(3, usbdma_chain_check(&s, SCHED_PHYS, DATA_PHYS, 0U));
    TEST_ASSERT_EQUAL(1, usbdma_chain_check(&s, SCHED_PHYS, DATA_PHYS, 2U));
    TEST_ASSERT_EQUAL(0, usbdma_chain_check(&s, SCHED_PHYS, DATA_PHYS, OS_USB_TD_END));
    put(&s, 2U, 0U);
    TEST_ASSERT_EQUAL(USBDMA_E_CYCLE, usbdma_chain_check(&s, SCHED_PHYS, DATA_PHYS, 0U));
}

/* Defense in depth: a TD that was not built by usbdma_build_td (corrupted
 * schedule) is refused before the controller is pointed at it. */
void test_chain_check_refuses_foreign_pointers(void) {
    static os_usb_sched_t s;
    put(&s, 0U, OS_USB_TD_END);
    s.td[0].buffer = 0x00100000U;
    TEST_ASSERT_EQUAL(USBDMA_E_BOUNDS, usbdma_chain_check(&s, SCHED_PHYS, DATA_PHYS, 0U));
    put(&s, 0U, OS_USB_TD_END);
    s.td[0].link = 0x00200000U | USBDMA_LINK_DEPTH;
    TEST_ASSERT_EQUAL(USBDMA_E_LINK, usbdma_chain_check(&s, SCHED_PHYS, DATA_PHYS, 0U));
    s.td[0].link = (SCHED_PHYS + 16U + 16U) | USBDMA_LINK_DEPTH;  /* inside a TD, not its start */
    TEST_ASSERT_EQUAL(USBDMA_E_LINK, usbdma_chain_check(&s, SCHED_PHYS, DATA_PHYS, 0U));
    s.td[0].link = SCHED_PHYS | USBDMA_LINK_QH;
    TEST_ASSERT_EQUAL(USBDMA_E_LINK, usbdma_chain_check(&s, SCHED_PHYS, DATA_PHYS, 0U));
}

/* USBCMD and FLBASEADD stay closed in the driver's IOPB. */
void test_port_whitelist_matches_iopb(void) {
    static uint8_t map[IO_BITMAP_BYTES];
    uint16_t base = 0xC040U, off;
    io_bitmap_deny_all(map, sizeof(map));
    io_bitmap_apply_usb(map, sizeof(map), base, 1);
    for (off = 0U; off < USBDMA_IO_SPAN; off++)
        TEST_ASSERT_EQUAL(usbdma_port_allowed(off), io_bitmap_port_allowed(map, sizeof(map), (uint16_t)(base + off)));
    TEST_ASSERT_FALSE(usbdma_port_allowed(USBDMA_REG_CMD));
    TEST_ASSERT_FALSE(usbdma_port_allowed(USBDMA_REG_FLBASE));
    TEST_ASSERT_FALSE(usbdma_port_allowed(USBDMA_REG_FLBASE + 3U));
    TEST_ASSERT_TRUE(usbdma_port_allowed(USBDMA_REG_PORTSC1));
    io_bitmap_apply_usb(map, sizeof(map), base, 0);
    for (off = 0U; off < USBDMA_IO_SPAN; off++)
        TEST_ASSERT_FALSE(io_bitmap_port_allowed(map, sizeof(map), (uint16_t)(base + off)));
}

void test_frame_list_and_scale(void) {
    static uint32_t fl[1024];
    int x, y;
    usbdma_frame_list_init(fl, SCHED_PHYS);
    TEST_ASSERT_TRUE_U32(SCHED_PHYS | USBDMA_LINK_QH, fl[0]);
    TEST_ASSERT_TRUE_U32(SCHED_PHYS | USBDMA_LINK_QH, fl[1023]);
    usbdma_scale(16384U, 32767U, 1024, 768, &x, &y);
    TEST_ASSERT_EQUAL(512, x);
    TEST_ASSERT_EQUAL(767, y);
    usbdma_scale(0xFFFFU, 0U, 1024, 768, &x, &y);
    TEST_ASSERT_EQUAL(1023, x);
    TEST_ASSERT_EQUAL(0, y);
}

int main(void) {
    unity_init();
    RUN_TEST(test_td_words_match_uhci_layout);
    RUN_TEST(test_td_buffer_never_leaves_the_data_page);
    RUN_TEST(test_td_rejects_bad_slot_pid_address_link);
    RUN_TEST(test_chain_check_counts_and_refuses_cycles);
    RUN_TEST(test_chain_check_refuses_foreign_pointers);
    RUN_TEST(test_port_whitelist_matches_iopb);
    RUN_TEST(test_frame_list_and_scale);
    unity_print_results();
    unity_cleanup();
    return unity_stats.tests_failed == 0 ? 0 : 1;
}
