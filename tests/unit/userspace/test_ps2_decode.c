/* Lot 5 (US-001): scan code set 1 and mouse packet decoder of ps2driver. */
#include "../../framework/unity.h"
#include "../../../userspace/ps2_decode.h"

static ps2_decoder_t d;
static os_input_event_t ev[8];
static uint32_t n;

static void feed(const uint8_t* bytes, uint32_t len, int aux) {
    uint32_t i;
    for (i = 0; i < len; i++) (void)ps2_decode_byte(&d, bytes[i], aux, ev, &n, 8);
}

static void test_letters_digits_and_breaks(void) {
    const uint8_t keys[] = {0x23, 0xA3, 0x17, 0x97, 0x02, 0x82, 0x1C, 0x9C, 0x39};
    ps2_decoder_init(&d); n = 0;
    feed(keys, sizeof(keys), 0);
    TEST_ASSERT_EQUAL(5U, n);
    TEST_ASSERT_EQUAL(OS_INPUT_EV_CHAR, ev[0].type);
    TEST_ASSERT_EQUAL('h', ev[0].code);
    TEST_ASSERT_EQUAL('i', ev[1].code);
    TEST_ASSERT_EQUAL('1', ev[2].code);
    TEST_ASSERT_EQUAL('\n', ev[3].code);
    TEST_ASSERT_EQUAL(' ', ev[4].code);
}

static void test_shift_and_caps(void) {
    const uint8_t seq[] = {0x2A, 0x1E, 0x02, 0xAA, 0x1E, 0x3A, 0xBA, 0x1E, 0x2A, 0x1E, 0xAA, 0x0C};
    ps2_decoder_init(&d); n = 0;
    feed(seq, sizeof(seq), 0);
    TEST_ASSERT_EQUAL(6U, n);
    TEST_ASSERT_EQUAL('A', ev[0].code);   /* shift + a */
    TEST_ASSERT_EQUAL('!', ev[1].code);   /* shift + 1 */
    TEST_ASSERT_EQUAL('a', ev[2].code);
    TEST_ASSERT_EQUAL('A', ev[3].code);   /* caps */
    TEST_ASSERT_EQUAL('a', ev[4].code);   /* caps + shift */
    TEST_ASSERT_EQUAL('-', ev[5].code);   /* caps leaves symbols alone */
}

static void test_escape_extended_and_controller_bytes(void) {
    const uint8_t seq[] = {0xFA, 0x01, 0x81, 0xE0, 0x48, 0xE0, 0xC8, 0xE0, 0x4B, 0xFE, 0x3B};
    ps2_decoder_init(&d); n = 0;
    feed(seq, sizeof(seq), 0);
    TEST_ASSERT_EQUAL(3U, n);
    TEST_ASSERT_EQUAL(OS_INPUT_EV_ESC, ev[0].type);
    TEST_ASSERT_EQUAL(OS_INPUT_EV_EXT, ev[1].type);
    TEST_ASSERT_EQUAL(0x48, ev[1].code);  /* up */
    TEST_ASSERT_EQUAL(OS_INPUT_EV_EXT, ev[2].type);
    TEST_ASSERT_EQUAL(0x4B, ev[2].code);  /* left */
    TEST_ASSERT_EQUAL(3U, d.ignored);     /* FA, FE, F1 (unmapped) */
}

static void test_mouse_packets_and_resync(void) {
    const uint8_t pkt[] = {0x09, 0x05, 0xFD};           /* left button, dx +5, dy -3 */
    const uint8_t junk[] = {0x00, 0x18, 0x10, 0x02};    /* no sync bit, then a packet */
    ps2_decoder_init(&d); n = 0;
    feed(pkt, sizeof(pkt), 1);
    TEST_ASSERT_EQUAL(1U, n);
    TEST_ASSERT_EQUAL(OS_INPUT_EV_MOUSE, ev[0].type);
    TEST_ASSERT_EQUAL(1, ev[0].code);
    TEST_ASSERT_EQUAL(5, ev[0].dx);
    TEST_ASSERT_EQUAL(-3, ev[0].dy);
    feed(junk, sizeof(junk), 1);
    TEST_ASSERT_EQUAL(2U, n);
    TEST_ASSERT_EQUAL(0, ev[1].code);
    TEST_ASSERT_EQUAL(16, ev[1].dx);
    TEST_ASSERT_EQUAL(2, ev[1].dy);
    TEST_ASSERT_EQUAL(1U, d.mouse_dropped);
    TEST_ASSERT_EQUAL(2U, d.mouse_packets);
    /* keyboard bytes between mouse packets are not confused with them */
    (void)ps2_decode_byte(&d, 0x1E, 0, ev, &n, 8);
    TEST_ASSERT_EQUAL('a', ev[2].code);
}

static void test_capacity_is_respected(void) {
    uint32_t i;
    ps2_decoder_init(&d); n = 0;
    for (i = 0; i < 12; i++) (void)ps2_decode_byte(&d, 0x1E, 0, ev, &n, 8);
    TEST_ASSERT_EQUAL(8U, n);
    TEST_ASSERT_EQUAL(0, ps2_decode_byte(&d, 0x1E, 0, ev, &n, 8));
    TEST_ASSERT_EQUAL(0, ps2_decode_byte(0, 0x1E, 0, ev, &n, 8));
}

int main(void) {
    unity_init();
    RUN_TEST(test_letters_digits_and_breaks);
    RUN_TEST(test_shift_and_caps);
    RUN_TEST(test_escape_extended_and_controller_bytes);
    RUN_TEST(test_mouse_packets_and_resync);
    RUN_TEST(test_capacity_is_respected);
    unity_print_results();
    unity_cleanup();
    return unity_stats.tests_failed == 0 ? 0 : 1;
}
