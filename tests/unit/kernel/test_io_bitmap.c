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


/* Tranche 5 suite: the NE2000 window for the Ring 3 net-driver. */
static void test_ne2k_grant_opens_only_nic_window(void) {
    uint16_t port;
    io_bitmap_deny_all(map, sizeof(map));
    io_bitmap_apply_ne2k(map, sizeof(map), 1);
    for (port = 0x300; port <= 0x31F; port++)
        TEST_ASSERT_TRUE(io_bitmap_port_allowed(map, sizeof(map), port));
    TEST_ASSERT_FALSE(io_bitmap_port_allowed(map, sizeof(map), 0x2FF));
    TEST_ASSERT_FALSE(io_bitmap_port_allowed(map, sizeof(map), 0x320));
    /* No ATA, PIC, PIT, PS/2 or serial port comes with it. */
    TEST_ASSERT_FALSE(io_bitmap_port_allowed(map, sizeof(map), 0x1F0));
    TEST_ASSERT_FALSE(io_bitmap_port_allowed(map, sizeof(map), 0x3F6));
    TEST_ASSERT_FALSE(io_bitmap_port_allowed(map, sizeof(map), 0x20));
    TEST_ASSERT_FALSE(io_bitmap_port_allowed(map, sizeof(map), 0x40));
    TEST_ASSERT_FALSE(io_bitmap_port_allowed(map, sizeof(map), 0x60));
    TEST_ASSERT_FALSE(io_bitmap_port_allowed(map, sizeof(map), 0x3F8));
}

static void test_ne2k_and_ata_grants_are_independent(void) {
    io_bitmap_deny_all(map, sizeof(map));
    io_bitmap_apply_ata(map, sizeof(map), 1);
    io_bitmap_apply_ne2k(map, sizeof(map), 1);
    io_bitmap_apply_ne2k(map, sizeof(map), 0);
    TEST_ASSERT_FALSE(io_bitmap_port_allowed(map, sizeof(map), 0x300));
    TEST_ASSERT_FALSE(io_bitmap_port_allowed(map, sizeof(map), 0x31F));
    TEST_ASSERT_TRUE(io_bitmap_port_allowed(map, sizeof(map), 0x1F0));
    TEST_ASSERT_TRUE(io_bitmap_port_allowed(map, sizeof(map), 0x3F6));
    io_bitmap_apply_ne2k(map, sizeof(map), 1);
    io_bitmap_apply_ata(map, sizeof(map), 0);
    TEST_ASSERT_TRUE(io_bitmap_port_allowed(map, sizeof(map), 0x310));
    TEST_ASSERT_FALSE(io_bitmap_port_allowed(map, sizeof(map), 0x1F7));
}

static void test_pci_grant_opens_only_config_ports(void) {
    uint32_t port;
    io_bitmap_deny_all(map, sizeof(map));
    io_bitmap_apply_pci(map, sizeof(map), 1);
    for (port = 0xCF8U; port <= 0xCFFU; port++)
        TEST_ASSERT_TRUE(io_bitmap_port_allowed(map, sizeof(map), (uint16_t)port));
    TEST_ASSERT_FALSE(io_bitmap_port_allowed(map, sizeof(map), 0xCF7));
    TEST_ASSERT_FALSE(io_bitmap_port_allowed(map, sizeof(map), 0xD00));
    TEST_ASSERT_FALSE(io_bitmap_port_allowed(map, sizeof(map), 0x1F0));
    TEST_ASSERT_FALSE(io_bitmap_port_allowed(map, sizeof(map), 0x300));
    io_bitmap_apply_ata(map, sizeof(map), 1);
    io_bitmap_apply_pci(map, sizeof(map), 0);
    TEST_ASSERT_FALSE(io_bitmap_port_allowed(map, sizeof(map), 0xCF8));
    TEST_ASSERT_FALSE(io_bitmap_port_allowed(map, sizeof(map), 0xCFC));
    TEST_ASSERT_TRUE(io_bitmap_port_allowed(map, sizeof(map), 0x1F0));
}

/* Lot 5 (US-001): ps2driver gets 0x60/0x64 only, serialdriver 0x3F8-0x3FF. */
static void test_ps2_and_serial_grants_are_narrow(void) {
    uint32_t port;
    io_bitmap_deny_all(map, sizeof(map));
    io_bitmap_apply_ps2(map, sizeof(map), 1);
    TEST_ASSERT_TRUE(io_bitmap_port_allowed(map, sizeof(map), 0x60));
    TEST_ASSERT_TRUE(io_bitmap_port_allowed(map, sizeof(map), 0x64));
    TEST_ASSERT_FALSE(io_bitmap_port_allowed(map, sizeof(map), 0x61));
    TEST_ASSERT_FALSE(io_bitmap_port_allowed(map, sizeof(map), 0x20));
    TEST_ASSERT_FALSE(io_bitmap_port_allowed(map, sizeof(map), 0x3F8));
    io_bitmap_apply_serial(map, sizeof(map), 1);
    for (port = 0x3F8U; port <= 0x3FFU; port++)
        TEST_ASSERT_TRUE(io_bitmap_port_allowed(map, sizeof(map), (uint16_t)port));
    TEST_ASSERT_FALSE(io_bitmap_port_allowed(map, sizeof(map), 0x3F7));
    TEST_ASSERT_FALSE(io_bitmap_port_allowed(map, sizeof(map), 0x2F8));
    TEST_ASSERT_FALSE(io_bitmap_port_allowed(map, sizeof(map), 0x3F6));
    io_bitmap_apply_ps2(map, sizeof(map), 0);
    TEST_ASSERT_FALSE(io_bitmap_port_allowed(map, sizeof(map), 0x60));
    TEST_ASSERT_FALSE(io_bitmap_port_allowed(map, sizeof(map), 0x64));
    TEST_ASSERT_TRUE(io_bitmap_port_allowed(map, sizeof(map), 0x3F8));
    io_bitmap_apply_serial(map, sizeof(map), 0);
    TEST_ASSERT_FALSE(io_bitmap_port_allowed(map, sizeof(map), 0x3FD));
}

int main(void) {
    unity_init();
    RUN_TEST(test_deny_all_blocks_every_probe_port);
    RUN_TEST(test_ata_grant_opens_only_ata_ports);
    RUN_TEST(test_ata_grant_is_revocable);
    RUN_TEST(test_single_port_toggle);
    RUN_TEST(test_ne2k_grant_opens_only_nic_window);
    RUN_TEST(test_ne2k_and_ata_grants_are_independent);
    RUN_TEST(test_pci_grant_opens_only_config_ports);
    RUN_TEST(test_ps2_and_serial_grants_are_narrow);
    unity_print_results();
    unity_cleanup();
    return unity_stats.tests_failed == 0 ? 0 : 1;
}
