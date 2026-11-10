/* Lot 89 (gap 6): NE2000 receive ring recovery. The fake card keeps a real
 * 16 KB packet memory, CURR/BNRY in their pages and a remote DMA that wraps
 * from PSTOP to PSTART like QEMU's ne2000. Before Lot 89 a full 1514 byte
 * frame (packet length 1518) was refused and BNRY never moved, so the ring
 * stayed stuck and every later connect timed out. */
#include "../../framework/unity.h"
#include <string.h>
#include "../../../kernel/ne2k.h"

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

void test_full_size_frame_is_read_and_ring_moves_on(void) {
    uint16_t len = 0U;
    card_reset();
    card_store(NE2K_ETHERNET_MAX_FRAME, 7U, 0U);
    card_store(60U, 40U, 0U);
    TEST_ASSERT_EQUAL(0, ne2k_rx_poll(&dev, &io, frame, sizeof(frame), &len));
    TEST_ASSERT_EQUAL(NE2K_ETHERNET_MAX_FRAME, len);
    TEST_ASSERT_EQUAL(7, frame[0]);
    TEST_ASSERT_EQUAL((uint8_t)(7U + 1513U), frame[1513]);
    TEST_ASSERT_EQUAL(0, ne2k_rx_poll(&dev, &io, frame, sizeof(frame), &len));
    TEST_ASSERT_EQUAL(60, len);
    TEST_ASSERT_EQUAL(40, frame[0]);
    TEST_ASSERT_EQUAL(1, ne2k_rx_poll(&dev, &io, frame, sizeof(frame), &len));
    TEST_ASSERT_EQUAL(0, dev.rx_dropped);
    TEST_ASSERT_EQUAL(0, dev.rx_resyncs);
}

void test_trailing_crc_is_dropped(void) {
    uint16_t len = 0U;
    card_reset();
    card_store(NE2K_ETHERNET_MAX_FRAME, 1U, NE2K_RX_CRC_SIZE);
    TEST_ASSERT_EQUAL(0, ne2k_rx_poll(&dev, &io, frame, sizeof(frame), &len));
    TEST_ASSERT_EQUAL(NE2K_ETHERNET_MAX_FRAME, len);
    TEST_ASSERT_EQUAL(1, ne2k_rx_poll(&dev, &io, frame, sizeof(frame), &len));
}

void test_frames_across_the_ring_end(void) {
    uint16_t len = 0U, n = 0U;
    int rc;
    card_reset();
    /* 30 full frames is more than the 26 page ring: read as we go. */
    while (n < 30U) {
        card_store(NE2K_ETHERNET_MAX_FRAME, (uint8_t)n, 0U);
        rc = ne2k_rx_poll(&dev, &io, frame, sizeof(frame), &len);
        TEST_ASSERT_EQUAL(0, rc);
        TEST_ASSERT_EQUAL(NE2K_ETHERNET_MAX_FRAME, len);
        TEST_ASSERT_EQUAL((uint8_t)n, frame[0]);
        TEST_ASSERT_EQUAL((uint8_t)(n + 1000U), frame[1000]);
        n++;
    }
    TEST_ASSERT_EQUAL(1, ne2k_rx_poll(&dev, &io, frame, sizeof(frame), &len));
}

void test_too_big_for_caller_is_skipped_not_stuck(void) {
    uint8_t small[64];
    uint16_t len = 0U;
    card_reset();
    card_store(200U, 3U, 0U);
    card_store(60U, 9U, 0U);
    TEST_ASSERT_EQUAL(-5, ne2k_rx_poll(&dev, &io, small, sizeof(small), &len));
    TEST_ASSERT_EQUAL(0, len);
    TEST_ASSERT_EQUAL(1, dev.rx_dropped);
    TEST_ASSERT_EQUAL(0, ne2k_rx_poll(&dev, &io, small, sizeof(small), &len));
    TEST_ASSERT_EQUAL(60, len);
    TEST_ASSERT_EQUAL(9, small[0]);
}

void test_bad_header_resyncs_on_curr(void) {
    uint16_t len = 0U;
    uint8_t first;
    card_reset();
    first = card_store(100U, 5U, 0U);
    card.mem[(uint16_t)(first << 8)] = 0U; /* status without RX OK */
    TEST_ASSERT_EQUAL(-3, ne2k_rx_poll(&dev, &io, frame, sizeof(frame), &len));
    TEST_ASSERT_EQUAL(1, dev.rx_resyncs);
    TEST_ASSERT_EQUAL(1, ne2k_rx_poll(&dev, &io, frame, sizeof(frame), &len));
    card_store(70U, 11U, 0U);
    TEST_ASSERT_EQUAL(0, ne2k_rx_poll(&dev, &io, frame, sizeof(frame), &len));
    TEST_ASSERT_EQUAL(70, len);
    TEST_ASSERT_EQUAL(11, frame[0]);
}

void test_bad_next_page_and_length_resync(void) {
    uint16_t len = 0U;
    uint8_t first;
    card_reset();
    first = card_store(100U, 5U, 0U);
    card.mem[(uint16_t)((first << 8) + 1U)] = 0x20U;
    TEST_ASSERT_EQUAL(-3, ne2k_rx_poll(&dev, &io, frame, sizeof(frame), &len));
    first = card_store(100U, 5U, 0U);
    card.mem[(uint16_t)((first << 8) + 3U)] = 0x09U; /* 2300+ bytes */
    TEST_ASSERT_EQUAL(-3, ne2k_rx_poll(&dev, &io, frame, sizeof(frame), &len));
    first = card_store(100U, 5U, 0U);
    card.mem[(uint16_t)((first << 8) + 2U)] = 6U;
    card.mem[(uint16_t)((first << 8) + 3U)] = 0U; /* shorter than a header */
    TEST_ASSERT_EQUAL(-3, ne2k_rx_poll(&dev, &io, frame, sizeof(frame), &len));
    TEST_ASSERT_EQUAL(3, dev.rx_resyncs);
    TEST_ASSERT_EQUAL(1, ne2k_rx_poll(&dev, &io, frame, sizeof(frame), &len));
}

/* The gap 6 sequence: nobody polls while a stream keeps arriving. 19 one
 * page packets and one six page packet end exactly on BNRY; QEMU then sees
 * a free ring and writes the next packet over unread data, so BNRY + 1 is in
 * the middle of that packet. The reader must resync on CURR and keep going
 * instead of refusing the same header forever. */
void test_qemu_overrun_resyncs_and_recovers(void) {
    uint16_t len = 0U, n;
    card_reset();
    for (n = 0U; n < 19U; n++) TEST_ASSERT_NOT_EQUAL(0, card_store(60U, 1U, 0U));
    TEST_ASSERT_NOT_EQUAL(0, card_store(NE2K_ETHERNET_MAX_FRAME, 1U, 0U));
    TEST_ASSERT_EQUAL(card.boundary, card.current_page);
    /* Payload byte 252 sits at the start of page BNRY + 2 - 1: fill 4 makes
     * it 0, a header without the RX OK bit, like the 0 0 0 4 seen in QEMU. */
    TEST_ASSERT_EQUAL(NE2K_RX_PAGE_START, card_store(NE2K_ETHERNET_MAX_FRAME, 4U, 0U));
    TEST_ASSERT_EQUAL(-3, ne2k_rx_poll(&dev, &io, frame, sizeof(frame), &len));
    TEST_ASSERT_EQUAL(1, dev.rx_resyncs);
    TEST_ASSERT_EQUAL(0, dev.rx_bad_header[0]);
    TEST_ASSERT_EQUAL(NE2K_RX_PAGE_START + 1U, dev.rx_bad_page);
    TEST_ASSERT_EQUAL(1, ne2k_rx_poll(&dev, &io, frame, sizeof(frame), &len));
    for (n = 0U; n < 40U; n++) {
        TEST_ASSERT_NOT_EQUAL(0, card_store(300U, (uint8_t)n, 0U));
        TEST_ASSERT_EQUAL(0, ne2k_rx_poll(&dev, &io, frame, sizeof(frame), &len));
        TEST_ASSERT_EQUAL(300, len);
        TEST_ASSERT_EQUAL((uint8_t)n, frame[0]);
    }
    TEST_ASSERT_EQUAL(1, dev.rx_resyncs);
}

/* Without a reader the card stops taking packets (ring full) and starts
 * again once the reader has moved BNRY. */
void test_full_ring_refuses_then_drains(void) {
    uint16_t len = 0U, stored = 0U, read = 0U;
    card_reset();
    while (card_store(NE2K_ETHERNET_MAX_FRAME, (uint8_t)stored, 0U) != 0U) stored++;
    TEST_ASSERT_EQUAL(4, stored);
    while (ne2k_rx_poll(&dev, &io, frame, sizeof(frame), &len) == 0) {
        TEST_ASSERT_EQUAL((uint8_t)read, frame[0]);
        read++;
    }
    TEST_ASSERT_EQUAL(stored, read);
    TEST_ASSERT_NOT_EQUAL(0, card_store(60U, 9U, 0U));
    TEST_ASSERT_EQUAL(0, ne2k_rx_poll(&dev, &io, frame, sizeof(frame), &len));
    TEST_ASSERT_EQUAL(0, dev.rx_resyncs);
}

void test_header_check_limits(void) {
    uint8_t h[4] = {NE2K_RX_STATUS_OK, 0x50U, 0U, 0U};
    uint16_t pay = 9U;
    uint16_t full = (uint16_t)(NE2K_RX_HEADER_SIZE + NE2K_ETHERNET_MAX_FRAME);
    h[2] = (uint8_t)full; h[3] = (uint8_t)(full >> 8);
    TEST_ASSERT_EQUAL(0, ne2k_rx_header_check(h, 1514U, &pay));
    TEST_ASSERT_EQUAL(1514, pay);
    TEST_ASSERT_EQUAL(1, ne2k_rx_header_check(h, 1513U, &pay));
    TEST_ASSERT_EQUAL(0, pay);
    full = (uint16_t)(full + NE2K_RX_CRC_SIZE);
    h[2] = (uint8_t)full; h[3] = (uint8_t)(full >> 8);
    TEST_ASSERT_EQUAL(0, ne2k_rx_header_check(h, 1514U, &pay));
    TEST_ASSERT_EQUAL(1514, pay);
    full++;
    h[2] = (uint8_t)full; h[3] = (uint8_t)(full >> 8);
    TEST_ASSERT_EQUAL(2, ne2k_rx_header_check(h, 1514U, &pay));
    h[2] = 18U; h[3] = 0U;
    TEST_ASSERT_EQUAL(0, ne2k_rx_header_check(h, 64U, &pay));
    TEST_ASSERT_EQUAL(14, pay);
    h[2] = 17U;
    TEST_ASSERT_EQUAL(2, ne2k_rx_header_check(h, 64U, &pay));
    h[2] = 18U; h[1] = NE2K_RX_PAGE_STOP;
    TEST_ASSERT_EQUAL(2, ne2k_rx_header_check(h, 64U, &pay));
    TEST_ASSERT_EQUAL(2, ne2k_rx_header_check(0, 64U, &pay));
}

int main(void) {
    unity_init();
    RUN_TEST(test_full_size_frame_is_read_and_ring_moves_on);
    RUN_TEST(test_trailing_crc_is_dropped);
    RUN_TEST(test_frames_across_the_ring_end);
    RUN_TEST(test_too_big_for_caller_is_skipped_not_stuck);
    RUN_TEST(test_bad_header_resyncs_on_curr);
    RUN_TEST(test_bad_next_page_and_length_resync);
    RUN_TEST(test_qemu_overrun_resyncs_and_recovers);
    RUN_TEST(test_full_ring_refuses_then_drains);
    RUN_TEST(test_header_check_limits);
    unity_print_results();
    unity_cleanup();
    return unity_stats.tests_failed == 0 ? 0 : 1;
}
