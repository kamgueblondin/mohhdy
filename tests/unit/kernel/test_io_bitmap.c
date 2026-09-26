/* Tranche 4: TSS I/O bitmap capability logic. */
#include "../../framework/unity.h"
#include "../../../kernel/io_bitmap.h"

static uint8_t map[IO_BITMAP_BYTES];

static void test_deny_all_blocks_every_probe_port(void) {
    io_bitmap_deny_all(map, sizeof(map));
    TEST_ASSERT_FALSE(io_bitmap_port_allowed(map, sizeof(map), 0x1F0));
    TEST_ASSERT_FALSE(io_bitmap_port_allowed(map, sizeof(map), 0x3F6));
    TEST_ASSERT_FALSE(io_bitmap_port_allowed(map, sizeof(map), 0x60));
    TEST_ASSERT_FALSE(io_bitmap_port_allowed(map, sizeof(map), 0xFFFF));
}

static void test_ata_grant_opens_only_ata_ports(void) {
    uint16_t port;
    io_bitmap_deny_all(map, sizeof(map));
    io_bitmap_apply_ata(map, sizeof(map), 1);
    for (port = 0x1F0; port <= 0x1F7; port++)
        TEST_ASSERT_TRUE(io_bitmap_port_allowed(map, sizeof(map), port));
    TEST_ASSERT_TRUE(io_bitmap_port_allowed(map, sizeof(map), 0x3F6));
    /* Neighbours of the ATA block stay denied. */
    TEST_ASSERT_FALSE(io_bitmap_port_allowed(map, sizeof(map), 0x1EF));
    TEST_ASSERT_FALSE(io_bitmap_port_allowed(map, sizeof(map), 0x1F8));
    TEST_ASSERT_FALSE(io_bitmap_port_allowed(map, sizeof(map), 0x3F5));
    TEST_ASSERT_FALSE(io_bitmap_port_allowed(map, sizeof(map), 0x3F7));
    /* No unrelated device (PIC, PS/2, NE2000, serial) becomes reachable. */
    TEST_ASSERT_FALSE(io_bitmap_port_allowed(map, sizeof(map), 0x20));
    TEST_ASSERT_FALSE(io_bitmap_port_allowed(map, sizeof(map), 0x60));
    TEST_ASSERT_FALSE(io_bitmap_port_allowed(map, sizeof(map), 0x300));
    TEST_ASSERT_FALSE(io_bitmap_port_allowed(map, sizeof(map), 0x3F8));
}

static void test_ata_grant_is_revocable(void) {
    io_bitmap_deny_all(map, sizeof(map));
    io_bitmap_apply_ata(map, sizeof(map), 1);
    TEST_ASSERT_TRUE(io_bitmap_port_allowed(map, sizeof(map), 0x1F0));
    io_bitmap_apply_ata(map, sizeof(map), 0);
    TEST_ASSERT_FALSE(io_bitmap_port_allowed(map, sizeof(map), 0x1F0));
    TEST_ASSERT_FALSE(io_bitmap_port_allowed(map, sizeof(map), 0x3F6));
}

static void test_single_port_toggle(void) {
    io_bitmap_deny_all(map, sizeof(map));
    io_bitmap_set_port(map, sizeof(map), 0x1F0, 1);
    TEST_ASSERT_TRUE(io_bitmap_port_allowed(map, sizeof(map), 0x1F0));
    TEST_ASSERT_FALSE(io_bitmap_port_allowed(map, sizeof(map), 0x1F1));
    io_bitmap_set_port(map, sizeof(map), 0x1F0, 0);
    TEST_ASSERT_FALSE(io_bitmap_port_allowed(map, sizeof(map), 0x1F0));
}

int main(void) {
    unity_init();
    RUN_TEST(test_deny_all_blocks_every_probe_port);
    RUN_TEST(test_ata_grant_opens_only_ata_ports);
    RUN_TEST(test_ata_grant_is_revocable);
    RUN_TEST(test_single_port_toggle);
    unity_print_results();
    unity_cleanup();
    return unity_stats.tests_failed == 0 ? 0 : 1;
}
