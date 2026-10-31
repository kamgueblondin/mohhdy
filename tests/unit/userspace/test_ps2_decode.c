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

/* Characters only (Lot 6: modifier changes are also reported as MODS). */
static uint32_t chars(char* out, uint32_t cap) {
    uint32_t i, k = 0;
    for (i = 0; i < n && k < cap; i++) if (ev[i].type == OS_INPUT_EV_CHAR) out[k++] = (char)ev[i].code;
    return k;
}

static void test_shift_and_caps(void) {
    const uint8_t seq[] = {0x2A, 0x1E, 0x02, 0xAA, 0x1E, 0x3A, 0xBA, 0x1E, 0x2A, 0x1E, 0xAA, 0x0C};
    os_input_event_t big[16];
    char c[16];
    uint32_t i, k;
    ps2_decoder_init(&d); n = 0;
    for (i = 0; i < sizeof(seq); i++) (void)ps2_decode_byte(&d, seq[i], 0, big, &n, 16);
    for (i = 0; i < n && i < 8; i++) ev[i] = big[i];
    k = 0;
    for (i = 0; i < n; i++) if (big[i].type == OS_INPUT_EV_CHAR) c[k++] = (char)big[i].code;
    TEST_ASSERT_EQUAL(6U, k);
    TEST_ASSERT_EQUAL('A', c[0]);   /* shift + a */
    TEST_ASSERT_EQUAL('!', c[1]);   /* shift + 1 */
    TEST_ASSERT_EQUAL('a', c[2]);
    TEST_ASSERT_EQUAL('A', c[3]);   /* caps */
    TEST_ASSERT_EQUAL('a', c[4]);   /* caps + shift */
    TEST_ASSERT_EQUAL('-', c[5]);   /* caps leaves symbols alone */
    TEST_ASSERT_EQUAL(11U, n);      /* + 5 MODS: shift, unshift, caps, shift, unshift */
    (void)chars;
}

/* Lot 6 (US-001): modifier changes are reported; a new decoder seeded
 * with the kernel state keeps a Shift held across a driver death. */
static void test_mods_reported_and_seeded(void) {
    char c[8];
    ps2_decoder_init(&d); n = 0;
    (void)ps2_decode_byte(&d, 0x2A, 0, ev, &n, 8);              /* shift press */
    TEST_ASSERT_EQUAL(1U, n);
    TEST_ASSERT_EQUAL(OS_INPUT_EV_MODS, ev[0].type);
    TEST_ASSERT_EQUAL(OS_INPUT_MOD_SHIFT, ev[0].code);
    (void)ps2_decode_byte(&d, 0x36, 0, ev, &n, 8);              /* right shift: no change */
    TEST_ASSERT_EQUAL(1U, n);
    (void)ps2_decode_byte(&d, 0x3A, 0, ev, &n, 8);              /* caps */
    TEST_ASSERT_EQUAL(OS_INPUT_MOD_SHIFT | OS_INPUT_MOD_CAPS, ev[1].code);
    /* driver dies here; the kernel holds SHIFT|CAPS; the respawned one seeds */
    ps2_decoder_init(&d); n = 0;
    ps2_decoder_seed(&d, OS_INPUT_MOD_SHIFT);
    (void)ps2_decode_byte(&d, 0x30, 0, ev, &n, 8);              /* b */
    (void)ps2_decode_byte(&d, 0xAA, 0, ev, &n, 8);              /* shift release */
    (void)ps2_decode_byte(&d, 0x30, 0, ev, &n, 8);              /* b */
    TEST_ASSERT_EQUAL(2U, chars(c, 8));
    TEST_ASSERT_EQUAL('B', c[0]);
    TEST_ASSERT_EQUAL('b', c[1]);
    TEST_ASSERT_EQUAL(OS_INPUT_EV_MODS, ev[1].type);
    TEST_ASSERT_EQUAL(0, ev[1].code);
    /* unseeded (Lot 5 behaviour) the first b came out lower case */
    ps2_decoder_init(&d); n = 0;
    (void)ps2_decode_byte(&d, 0x30, 0, ev, &n, 8);
    TEST_ASSERT_EQUAL('b', ev[0].code);
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

/* UX4: the driver stalls, Ring 0 drains the Shift break (kernel mods = 0)
 * while this decoder still holds Shift. Re-seeding at each wake (as
 * ps2_driver.c now does) gives "/c", not the "?C" seen under GPT-2 load. */
static void test_reseed_after_ring0_drain(void) {
    char c[8];
    ps2_decoder_init(&d); n = 0;
    (void)ps2_decode_byte(&d, 0x2A, 0, ev, &n, 8);              /* shift press, driver */
    /* stall: 0xAA consumed by Ring 0, kernel mods now 0 */
    n = 0;
    ps2_decoder_seed(&d, 0U);                                   /* next wake */
    (void)ps2_decode_byte(&d, 0x35, 0, ev, &n, 8);              /* / */
    (void)ps2_decode_byte(&d, 0x2E, 0, ev, &n, 8);              /* c */
    TEST_ASSERT_EQUAL(2U, chars(c, 8));
    TEST_ASSERT_EQUAL('/', c[0]);
    TEST_ASSERT_EQUAL('c', c[1]);
    /* the other way: Ring 0 saw the make, the driver gets the break */
    ps2_decoder_init(&d); n = 0;
    ps2_decoder_seed(&d, OS_INPUT_MOD_SHIFT);
    (void)ps2_decode_byte(&d, 0x2E, 0, ev, &n, 8);              /* C */
    (void)ps2_decode_byte(&d, 0xAA, 0, ev, &n, 8);              /* break reaches the driver */
    (void)ps2_decode_byte(&d, 0x2E, 0, ev, &n, 8);              /* c */
    TEST_ASSERT_EQUAL(2U, chars(c, 8));
    TEST_ASSERT_EQUAL('C', c[0]);
    TEST_ASSERT_EQUAL('c', c[1]);
}


/* Lot 68: Ctrl+'+' / Ctrl+'-' / Ctrl+'0' (left or right Ctrl, main or
 * keypad keys) give zoom key codes; without Ctrl they stay characters. */
static void test_ctrl_zoom_keys(void) {
    char c[16];
    uint32_t k;
    ps2_decoder_init(&d); n = 0;
    (void)ps2_decode_byte(&d, 0x0D, 0, ev, &n, 8);                 /* '=' */
    TEST_ASSERT_EQUAL(1U, chars(c, 16));
    TEST_ASSERT_EQUAL('=', c[0]);
    ps2_decoder_init(&d); n = 0;
    {
        const uint8_t seq[] = {0x1D, 0x0D, 0x8D, 0x0C, 0x8C, 0x0B, 0x8B, 0x9D, 0x0B};
        feed(seq, sizeof(seq), 0);
    }
    k = chars(c, 16);
    TEST_ASSERT_EQUAL(4U, k);
    TEST_ASSERT_EQUAL(OS_KEY_ZOOM_IN, (uint8_t)c[0]);
    TEST_ASSERT_EQUAL(OS_KEY_ZOOM_OUT, (uint8_t)c[1]);
    TEST_ASSERT_EQUAL(OS_KEY_ZOOM_RESET, (uint8_t)c[2]);
    TEST_ASSERT_EQUAL('0', c[3]);                                    /* Ctrl released */
    ps2_decoder_init(&d); n = 0;
    {
        const uint8_t seq[] = {0xE0, 0x1D, 0x4E, 0x4A, 0x52, 0xE0, 0x9D};
        feed(seq, sizeof(seq), 0);
    }
    k = chars(c, 16);
    TEST_ASSERT_EQUAL(3U, k);
    TEST_ASSERT_EQUAL(OS_KEY_ZOOM_IN, (uint8_t)c[0]);
    TEST_ASSERT_EQUAL(OS_KEY_ZOOM_OUT, (uint8_t)c[1]);
    TEST_ASSERT_EQUAL(OS_KEY_ZOOM_RESET, (uint8_t)c[2]);
    TEST_ASSERT_EQUAL(0, ps2_zoom_key(0x1E));
    /* Ctrl is a reported modifier: a Ring 0 hand-over keeps it */
    ps2_decoder_init(&d); n = 0;
    (void)ps2_decode_byte(&d, 0x1D, 0, ev, &n, 8);
    TEST_ASSERT_EQUAL(1U, n);
    TEST_ASSERT_EQUAL(OS_INPUT_EV_MODS, ev[0].type);
    TEST_ASSERT_EQUAL(OS_INPUT_MOD_CTRL, ev[0].code);
    (void)ps2_decode_byte(&d, 0x1D, 0, ev, &n, 8);              /* typematic repeat: no new event */
    TEST_ASSERT_EQUAL(1U, n);
    ps2_decoder_init(&d); n = 0;
    ps2_decoder_seed(&d, OS_INPUT_MOD_CTRL);
    (void)ps2_decode_byte(&d, 0x0C, 0, ev, &n, 8);
    TEST_ASSERT_EQUAL(OS_KEY_ZOOM_OUT, ev[0].code);
    ps2_decoder_seed(&d, 0U);
    (void)ps2_decode_byte(&d, 0x0C, 0, ev, &n, 8);
    TEST_ASSERT_EQUAL('-', ev[1].code);
}

int main(void) {
    unity_init();
    RUN_TEST(test_letters_digits_and_breaks);
    RUN_TEST(test_shift_and_caps);
    RUN_TEST(test_mods_reported_and_seeded);
    RUN_TEST(test_escape_extended_and_controller_bytes);
    RUN_TEST(test_mouse_packets_and_resync);
    RUN_TEST(test_capacity_is_respected);
    RUN_TEST(test_reseed_after_ring0_drain);
    RUN_TEST(test_ctrl_zoom_keys);
    unity_print_results();
    unity_cleanup();
    return unity_stats.tests_failed == 0 ? 0 : 1;
}
