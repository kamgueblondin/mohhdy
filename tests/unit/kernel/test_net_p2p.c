/* Phase 5: P2P datagram framing (broadcast Ethernet + IPv4/UDP). */
#include "../../framework/unity.h"
#include <string.h>
#include "../../../kernel/net_p2p.h"

static const uint8_t k_mac[6] = {0x52, 0x54, 0, 0xa0, 0x20, 0x0a};
static const uint8_t k_other[6] = {0x52, 0x54, 0, 0xa0, 0x20, 0x0b};
static const uint8_t k_src[4] = {10, 77, 0, 1}, k_dst[4] = {255, 255, 255, 255};

void test_build_and_parse_roundtrip(void) {
    uint8_t f[1600];
    net_p2p_view_t v;
    int n = net_p2p_build(f, sizeof(f), k_mac, k_src, k_dst, 7700, (const uint8_t*)"MP2 hello", 9);
    TEST_ASSERT_EQUAL(60, n); /* padded to the Ethernet minimum */
    TEST_ASSERT_EQUAL(0xff, f[0]); TEST_ASSERT_EQUAL(0xff, f[5]);
    TEST_ASSERT_EQUAL(0, memcmp(f + 6, k_mac, 6));
    TEST_ASSERT_EQUAL(1, net_p2p_parse(f, (uint16_t)n, k_other, 7700, &v));
    TEST_ASSERT_EQUAL(9, v.payload_length);
    TEST_ASSERT_EQUAL(0, memcmp(v.payload, "MP2 hello", 9));
    TEST_ASSERT_EQUAL(0, memcmp(v.source_mac, k_mac, 6));
    TEST_ASSERT_EQUAL(0, memcmp(v.source_ip, k_src, 4));
    TEST_ASSERT_EQUAL(0, memcmp(v.destination_ip, k_dst, 4));
}

void test_parse_rejects_own_other_port_and_garbage(void) {
    uint8_t f[1600], big[1000];
    net_p2p_view_t v;
    int n = net_p2p_build(f, sizeof(f), k_mac, k_src, k_dst, 7700, (const uint8_t*)"x", 1);
    TEST_ASSERT_EQUAL(0, net_p2p_parse(f, (uint16_t)n, k_mac, 7700, &v));   /* own echo */
    TEST_ASSERT_EQUAL(0, net_p2p_parse(f, (uint16_t)n, k_other, 7701, &v)); /* other port */
    f[12] = 0x08; f[13] = 0x06;                                              /* ARP */
    TEST_ASSERT_EQUAL(0, net_p2p_parse(f, (uint16_t)n, k_other, 7700, &v));
    f[13] = 0x00; f[14 + 10] ^= 0xff;                                        /* bad IP checksum */
    TEST_ASSERT_EQUAL(0, net_p2p_parse(f, (uint16_t)n, k_other, 7700, &v));
    TEST_ASSERT_EQUAL(0, net_p2p_parse(f, 20, k_other, 7700, &v));
    memset(big, 'a', sizeof(big));
    n = net_p2p_build(f, sizeof(f), k_mac, k_src, k_dst, 7700, big, sizeof(big));
    TEST_ASSERT_EQUAL(14 + 28 + 1000, n);
    TEST_ASSERT_EQUAL(-1, net_p2p_build(f, 100, k_mac, k_src, k_dst, 7700, big, sizeof(big)));
    TEST_ASSERT_EQUAL(-1, net_p2p_build(f, sizeof(f), k_mac, k_src, k_dst, 0, big, 1));
}

int main(void) {
    unity_init();
    RUN_TEST(test_build_and_parse_roundtrip);
    RUN_TEST(test_parse_rejects_own_other_port_and_garbage);
    unity_print_results();
    unity_cleanup();
    return (unity_stats.tests_failed == 0) ? 0 : 1;
}
