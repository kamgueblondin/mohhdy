/* Lot 90 (Lot 89 gap 6): the Ring 3 net worker drains the NE2000 ring
 * into a frame FIFO between relay ops (userspace/net_rxq.c), and the
 * stack's poll serves the FIFO first, then the card. The fake card is the
 * Lot 89 model of QEMU's ne2000 (real 16 KB packet memory, CURR/BNRY,
 * remote DMA wrapping at PSTOP, a full ring refuses the packet). */
#include "../../framework/unity.h"
#include <string.h>
#include "../../../kernel/ne2k.h"
#include "../../../userspace/net_rxq.h"

typedef struct {
    uint8_t mem[0x60 * 256];
    uint8_t command, boundary, current_page;
    uint16_t rsar, rbcr;
} ring_card_t;

static ring_card_t card;
static ne2k_device_t dev;
static ne2k_io_t io;

void setUp(void) {}
void tearDown(void) {}

static uint8_t card_inb(void* context, uint16_t port) {
    ring_card_t* c = (ring_card_t*)context;
    uint8_t reg = (uint8_t)(port & 0x1fU);
    uint8_t v;
    if (reg == NE2K_REG_DATA) {
        v = c->mem[c->rsar];
        c->rsar++;
        if (c->rsar >= (uint16_t)(NE2K_RX_PAGE_STOP << 8))
            c->rsar = (uint16_t)(NE2K_RX_PAGE_START << 8);
        if (c->rbcr) c->rbcr--;
        return v;
    }
    if (reg == NE2K_REG_CURR && (c->command & 0xc0U) == NE2K_COMMAND_PAGE1)
        return c->current_page;
    if (reg == NE2K_REG_BNRY && (c->command & 0xc0U) == NE2K_COMMAND_PAGE0)
        return c->boundary;
    return 0U;
}

static void card_outb(void* context, uint16_t port, uint8_t value) {
    ring_card_t* c = (ring_card_t*)context;
    uint8_t reg = (uint8_t)(port & 0x1fU);
    if (reg == NE2K_REG_COMMAND) { c->command = value; return; }
    if ((c->command & 0xc0U) == NE2K_COMMAND_PAGE1) {
        if (reg == NE2K_REG_CURR) c->current_page = value;
        return;
    }
    if (reg == NE2K_REG_BNRY) c->boundary = value;
    else if (reg == NE2K_REG_RSAR0) c->rsar = (uint16_t)((c->rsar & 0xff00U) | value);
    else if (reg == NE2K_REG_RSAR0 + 1U) c->rsar = (uint16_t)((c->rsar & 0x00ffU) | ((uint16_t)value << 8));
    else if (reg == NE2K_REG_RBCR0) c->rbcr = (uint16_t)((c->rbcr & 0xff00U) | value);
    else if (reg == NE2K_REG_RBCR0 + 1U) c->rbcr = (uint16_t)((c->rbcr & 0x00ffU) | ((uint16_t)value << 8));
}

static void card_reset(void) {
    memset(&card, 0, sizeof(card));
    memset(&dev, 0, sizeof(dev));
    io.context = &card; io.inb = card_inb; io.outb = card_outb;
    dev.base_port = 0x300U; dev.initialized = 1U;
    card.boundary = NE2K_RX_PAGE_START;
    card.current_page = NE2K_RX_PAGE_START + 1U;
}

/* QEMU's ne2000_buffer_full(): note that CURR == BNRY reads as a whole
 * free ring, which is how unread packets get overwritten (see the overrun
 * test below). */
static int card_full(void) {
    int index = card.current_page << 8, boundary = card.boundary << 8, avail;
    int size = (NE2K_RX_PAGE_STOP - NE2K_RX_PAGE_START) << 8;
    if (index < boundary) avail = boundary - index;
    else avail = size - (index - boundary);
    return avail < (int)(NE2K_ETHERNET_MAX_FRAME + 4U);
}

/* Store one packet at CURR the way QEMU's ne2000_receive() does: header,
 * then data wrapping at PSTOP, next page counting 4 CRC bytes that are not
 * written. extra = CRC bytes a real card would keep. Returns the first page,
 * or 0 when the card refuses the packet (ring full). */
static uint8_t card_store(uint16_t length, uint8_t fill, uint16_t extra) {
    uint8_t start = card.current_page;
    uint16_t total = (uint16_t)(NE2K_RX_HEADER_SIZE + length + extra);
    uint16_t addr = (uint16_t)(start << 8), i;
    uint8_t pages = (uint8_t)((total + 4U + 255U) / 256U);
    uint8_t next = (uint8_t)(start + pages);
    if (card_full()) return 0U;
    if (next >= NE2K_RX_PAGE_STOP) next = (uint8_t)(next - (NE2K_RX_PAGE_STOP - NE2K_RX_PAGE_START));
    for (i = 0; i < total; ++i) {
        uint8_t v;
        if (i == 0U) v = NE2K_RX_STATUS_OK;
        else if (i == 1U) v = next;
        else if (i == 2U) v = (uint8_t)total;
        else if (i == 3U) v = (uint8_t)(total >> 8);
        else v = (uint8_t)(fill + (i - NE2K_RX_HEADER_SIZE));
        card.mem[addr] = v;
        addr++;
        if (addr >= (uint16_t)(NE2K_RX_PAGE_STOP << 8)) addr = (uint16_t)(NE2K_RX_PAGE_START << 8);
    }
    card.current_page = next;
    return start;
}


static uint8_t frame[NE2K_ETHERNET_MAX_FRAME];
static uint8_t scratch[NET_RXQ_FRAME_MAX];
static uint8_t qmem[NET_RXQ_BYTES];
static net_rxq_t q;

static int card_poll(void* context, uint8_t* out, uint16_t capacity, uint16_t* length) {
    (void)context;
    return ne2k_rx_poll(&dev, &io, out, capacity, length) != 0 ? -1 : 0;
}

/* the worker's r3_poll: the FIFO, then the card */
static int stack_poll(uint8_t* out, uint16_t capacity, uint16_t* length) {
    if (net_rxq_pop(&q, out, capacity, length) == 0) return 0;
    return card_poll(0, out, capacity, length);
}

static void fifo_reset(void) {
    TEST_ASSERT_EQUAL(0, net_rxq_init(&q, qmem, (uint32_t)sizeof(qmem)));
}

void test_fifo_order_wrap_and_bounds(void) {
    uint8_t small[8];
    uint16_t len = 0U;
    int k;
    char text[160];
    TEST_ASSERT_EQUAL(-1, net_rxq_init(&q, qmem, NET_RXQ_FRAME_MAX + 1U));
    TEST_ASSERT_EQUAL(-1, net_rxq_init(0, qmem, sizeof(qmem)));
    fifo_reset();
    TEST_ASSERT_EQUAL(-1, net_rxq_pop(&q, frame, sizeof(frame), &len));
    /* frames of 1000 bytes (24 fit): keep ten held while 200 go through,
     * so the byte ring wraps many times; each pop is the oldest */
    {
        int next = 0;
        for (k = 0; k < 200; k++) {
            memset(frame, k, 1000);
            frame[999] = (uint8_t)(k ^ 0x5aU);
            TEST_ASSERT_EQUAL(0, net_rxq_push(&q, frame, 1000U));
            if (q.frames >= 10U) {
                TEST_ASSERT_EQUAL(0, net_rxq_pop(&q, frame, sizeof(frame), &len));
                TEST_ASSERT_EQUAL(1000, len);
                TEST_ASSERT_EQUAL((uint8_t)next, frame[0]);
                TEST_ASSERT_EQUAL((uint8_t)(next ^ 0x5a), frame[999]);
                next++;
            }
        }
        TEST_ASSERT_EQUAL(10U, q.peak);
        TEST_ASSERT_EQUAL(200U, q.pushed);
        TEST_ASSERT_EQUAL(191, next);
        TEST_ASSERT_EQUAL(9U * 1002U, q.used);
    }
    /* refusals: empty, too long, no room */
    TEST_ASSERT_EQUAL(-1, net_rxq_push(&q, frame, 0U));
    TEST_ASSERT_EQUAL(-1, net_rxq_push(&q, frame, (uint16_t)(NET_RXQ_FRAME_MAX + 1U)));
    fifo_reset();
    for (k = 0; k < 16; k++) TEST_ASSERT_EQUAL(0, net_rxq_push(&q, frame, (uint16_t)NET_RXQ_FRAME_MAX));
    TEST_ASSERT_EQUAL(0, net_rxq_room(&q, 1U));
    TEST_ASSERT_EQUAL(-1, net_rxq_push(&q, frame, 1U));
    TEST_ASSERT_EQUAL(1U, q.full);
    /* a frame longer than the reader's buffer is skipped, the next served */
    fifo_reset();
    memset(frame, 9, 100);
    TEST_ASSERT_EQUAL(0, net_rxq_push(&q, frame, 100U));
    memset(frame, 3, 4);
    TEST_ASSERT_EQUAL(0, net_rxq_push(&q, frame, 4U));
    TEST_ASSERT_EQUAL(0, net_rxq_pop(&q, small, sizeof(small), &len));
    TEST_ASSERT_EQUAL(4, len);
    TEST_ASSERT_EQUAL(3, small[0]);
    TEST_ASSERT_EQUAL(1U, q.oversize);
    TEST_ASSERT_EQUAL(0U, q.frames);
    TEST_ASSERT_EQUAL(0U, q.used);
    TEST_ASSERT(net_rxq_text(&q, text, (int)sizeof(text)) > 0);
    TEST_ASSERT_EQUAL_STRING("rxq held 0 peak 2 in 2 out 1 full 0 oversize 1 drains 0", text);
    TEST_ASSERT_EQUAL(-1, net_rxq_text(&q, text, 10));
}

void test_burst_between_ops_is_kept(void) {
    uint16_t len = 0U;
    int k, stored = 0, got = 0;
    card_reset();
    fifo_reset();
    /* 12 full frames arrive between two ops; the idle loop drains after
     * every second one (the ring holds about four) */
    for (k = 0; k < 12; k++) {
        TEST_ASSERT(card_store(NE2K_ETHERNET_MAX_FRAME, (uint8_t)(k * 11), 0U) != 0U);
        stored++;
        if (k % 2 == 1) TEST_ASSERT_EQUAL(2U, net_rxq_drain(&q, card_poll, 0, scratch, NET_RXQ_IDLE_MAX));
    }
    TEST_ASSERT_EQUAL(12U, q.frames);
    TEST_ASSERT_EQUAL(6U, q.drains);
    /* the next op reads them all, in order */
    while (stack_poll(frame, sizeof(frame), &len) == 0) {
        TEST_ASSERT_EQUAL(NE2K_ETHERNET_MAX_FRAME, len);
        TEST_ASSERT_EQUAL((uint8_t)(got * 11), frame[0]);
        got++;
    }
    TEST_ASSERT_EQUAL(stored, got);
    TEST_ASSERT_EQUAL(0, dev.rx_dropped);
}

void test_without_drain_the_card_refuses(void) {
    /* the Lot 89 behaviour, kept as the control: no drain between ops, the
     * fifth full frame does not fit the ring */
    int k, refused = 0;
    card_reset();
    for (k = 0; k < 12; k++) if (card_store(NE2K_ETHERNET_MAX_FRAME, (uint8_t)k, 0U) == 0U) refused++;
    TEST_ASSERT(refused >= 7);
}

void test_drain_stops_when_full_and_order_is_kept(void) {
    uint16_t len = 0U;
    int k, got = 0;
    card_reset();
    /* a FIFO for two full frames only */
    TEST_ASSERT_EQUAL(0, net_rxq_init(&q, qmem, 2U * (NET_RXQ_FRAME_MAX + 2U)));
    for (k = 0; k < 4; k++) TEST_ASSERT(card_store(1000U, (uint8_t)k, 0U) != 0U);
    TEST_ASSERT_EQUAL(2U, net_rxq_drain(&q, card_poll, 0, scratch, 10U));
    /* no room for a full frame: nothing more is read off the card */
    TEST_ASSERT_EQUAL(0U, net_rxq_drain(&q, card_poll, 0, scratch, 10U));
    TEST_ASSERT_EQUAL(0U, q.full);
    /* new frames on the card behind the two left there */
    TEST_ASSERT(card_store(60U, 4U, 0U) != 0U);
    while (stack_poll(frame, sizeof(frame), &len) == 0) {
        TEST_ASSERT_EQUAL(got, frame[0]);
        got++;
    }
    TEST_ASSERT_EQUAL(5, got);
}

void test_drain_max_per_round_and_empty_card(void) {
    int k;
    card_reset();
    fifo_reset();
    for (k = 0; k < 3; k++) TEST_ASSERT(card_store(200U, (uint8_t)k, 0U) != 0U);
    TEST_ASSERT_EQUAL(2U, net_rxq_drain(&q, card_poll, 0, scratch, 2U));
    TEST_ASSERT_EQUAL(1U, net_rxq_drain(&q, card_poll, 0, scratch, 2U));
    TEST_ASSERT_EQUAL(0U, net_rxq_drain(&q, card_poll, 0, scratch, 2U));
    TEST_ASSERT_EQUAL(2U, q.drains);
    TEST_ASSERT_EQUAL(0U, net_rxq_drain(&q, 0, 0, scratch, 2U));
    TEST_ASSERT_EQUAL(0U, net_rxq_drain(&q, card_poll, 0, 0, 2U));
}

int main(void) {
    unity_init();
    RUN_TEST(test_fifo_order_wrap_and_bounds);
    RUN_TEST(test_burst_between_ops_is_kept);
    RUN_TEST(test_without_drain_the_card_refuses);
    RUN_TEST(test_drain_stops_when_full_and_order_is_kept);
    RUN_TEST(test_drain_max_per_round_and_empty_card);
    unity_print_results();
    unity_cleanup();
    return unity_stats.tests_failed == 0 ? 0 : 1;
}
