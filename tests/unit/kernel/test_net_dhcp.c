#include "../../framework/unity.h"
#include "../../../kernel/net_dhcp.h"

void setUp(void) {}
void tearDown(void) {}

void test_build_and_parse_dhcp_offer(void) {
    uint8_t packet[256] = {0};
    uint8_t mac[6] = {2, 0, 0, 0, 0, 1};
    net_dhcp_offer_t offer;
    TEST_ASSERT_EQUAL(249, net_dhcp_build_discover(packet, sizeof(packet), 0x12345678U, mac));
    TEST_ASSERT_EQUAL(1, packet[0]);
    TEST_ASSERT_EQUAL(0x63, packet[236]);
    TEST_ASSERT_EQUAL(NET_DHCP_OPTION_PARAMETER_REQUEST_LIST, packet[243]);
    TEST_ASSERT_EQUAL(3, packet[244]);
    TEST_ASSERT_EQUAL(NET_DHCP_OPTION_ROUTER, packet[246]);
    TEST_ASSERT_EQUAL(NET_DHCP_OPTION_DNS, packet[247]);
    packet[0] = 2;
    packet[16] = 10; packet[17] = 0; packet[18] = 2; packet[19] = 15;
    packet[240] = NET_DHCP_OPTION_MESSAGE_TYPE; packet[241] = 1; packet[242] = NET_DHCP_OFFER;
    packet[243] = NET_DHCP_OPTION_SERVER_ID; packet[244] = 4;
    packet[245] = 10; packet[246] = 0; packet[247] = 2; packet[248] = 2;
    packet[249] = NET_DHCP_OPTION_END;
    TEST_ASSERT_EQUAL(0, net_dhcp_parse_offer(packet, 250, 0x12345678U, &offer));
    TEST_ASSERT_EQUAL(10, offer.offered_ip[0]);
    TEST_ASSERT_EQUAL(2, offer.server_ip[3]);
    { net_dhcp_lease_t lease = {0};
      TEST_ASSERT_EQUAL(0, net_dhcp_lease_apply(&lease, &offer));
      TEST_ASSERT_EQUAL(1, lease.valid); TEST_ASSERT_EQUAL(10, lease.ipv4[0]);
      TEST_ASSERT_EQUAL(2, lease.server_ipv4[3]);
      net_dhcp_lease_clear(&lease); TEST_ASSERT_EQUAL(0, lease.valid); TEST_ASSERT_EQUAL(0, lease.router_valid); TEST_ASSERT_EQUAL(0, lease.dns_valid); }
    TEST_ASSERT_NOT_EQUAL(0, net_dhcp_parse_offer(packet, 250, 0x87654321U, &offer));
    { uint8_t request[290] = {0}; uint8_t requested[4] = {10,0,2,15}; uint8_t server[4] = {10,0,2,2}; uint8_t local[4] = {10,0,2,99}; uint8_t remote[4] = {1,1,1,1}; uint8_t hop[4] = {9,9,9,9};
      net_dhcp_lease_t ack = {0}, preserved;
      TEST_ASSERT_EQUAL(255, net_dhcp_build_request(request, sizeof(request), 0x12345678U, mac, requested, server));
      TEST_ASSERT_EQUAL(NET_DHCP_REQUEST, request[242]);
      TEST_ASSERT_EQUAL(249, net_dhcp_build_renew(request, sizeof(request), 0x87654321U, mac, requested));
      TEST_ASSERT_EQUAL(10U, request[12]); TEST_ASSERT_EQUAL(15U, request[15]);
      TEST_ASSERT_EQUAL(NET_DHCP_REQUEST, request[242]);
      TEST_ASSERT_EQUAL(NET_DHCP_OPTION_PARAMETER_REQUEST_LIST, request[243]);
      TEST_ASSERT_EQUAL(NET_DHCP_OPTION_SUBNET_MASK, request[245]); TEST_ASSERT_EQUAL(NET_DHCP_OPTION_ROUTER, request[246]); TEST_ASSERT_EQUAL(NET_DHCP_OPTION_DNS, request[247]);
      TEST_ASSERT_NOT_EQUAL(0, net_dhcp_build_renew(request, 248U, 0x87654321U, mac, requested));
      TEST_ASSERT_EQUAL(255, net_dhcp_build_request(request, sizeof(request), 0x12345678U, mac, requested, server));
      request[0] = 2; request[16] = 10; request[17] = 0; request[18] = 2; request[19] = 15;
      request[242] = NET_DHCP_ACK;
      request[255] = NET_DHCP_OPTION_SUBNET_MASK; request[256] = 4; request[257] = 255; request[258] = 255; request[259] = 255; request[260] = 0;
      request[261] = NET_DHCP_OPTION_ROUTER; request[262] = 4; request[263] = 10; request[264] = 0; request[265] = 2; request[266] = 2;
      request[267] = NET_DHCP_OPTION_DNS; request[268] = 8; request[269] = 1; request[270] = 1; request[271] = 1; request[272] = 1; request[273] = 8; request[274] = 8; request[275] = 8; request[276] = 8; request[277] = NET_DHCP_OPTION_LEASE_TIME; request[278] = 4; request[279] = 0; request[280] = 0; request[281] = 0; request[282] = 100; request[283] = NET_DHCP_OPTION_END;
      TEST_ASSERT_EQUAL(0, net_dhcp_parse_ack(request, 284, 0x12345678U, &ack));
      TEST_ASSERT_EQUAL(1, ack.valid); TEST_ASSERT_EQUAL(100U, ack.lease_seconds); TEST_ASSERT_EQUAL(10, ack.ipv4[0]); TEST_ASSERT_EQUAL(1, ack.subnet_valid); TEST_ASSERT_EQUAL(0, net_dhcp_lease_mark_acquired(&ack,1000U)); TEST_ASSERT_EQUAL(1, net_dhcp_lease_is_valid_at(&ack,1049U)); TEST_ASSERT_EQUAL(1, net_dhcp_lease_renewal_due(&ack,1050U)); TEST_ASSERT_EQUAL(1, net_dhcp_lease_is_valid_at(&ack,1099U)); TEST_ASSERT_EQUAL(0, net_dhcp_lease_is_valid_at(&ack,1100U)); TEST_ASSERT_EQUAL(255, ack.subnet_mask[0]); TEST_ASSERT_EQUAL(1, ack.router_valid); TEST_ASSERT_EQUAL(10, ack.router_ipv4[0]); TEST_ASSERT_EQUAL(2, ack.router_ipv4[3]); TEST_ASSERT_EQUAL(1, ack.dns_valid); TEST_ASSERT_EQUAL(1, ack.dns_ipv4[0]);
      TEST_ASSERT_EQUAL(0, net_dhcp_lease_next_hop(&ack, local, hop)); TEST_ASSERT_EQUAL(10, hop[0]); TEST_ASSERT_EQUAL(99, hop[3]);
      TEST_ASSERT_EQUAL(0, net_dhcp_lease_next_hop(&ack, remote, hop)); TEST_ASSERT_EQUAL(10, hop[0]); TEST_ASSERT_EQUAL(2, hop[3]);
      ack.subnet_valid = 0; hop[0] = 9; TEST_ASSERT_NOT_EQUAL(0, net_dhcp_lease_next_hop(&ack, remote, hop)); TEST_ASSERT_EQUAL(9, hop[0]); ack.subnet_valid = 1;
      preserved = ack; request[262] = 3;
      TEST_ASSERT_NOT_EQUAL(0, net_dhcp_parse_ack(request, 284, 0x12345678U, &ack));
      TEST_ASSERT_EQUAL(preserved.valid, ack.valid); TEST_ASSERT_EQUAL(preserved.subnet_valid, ack.subnet_valid); TEST_ASSERT_EQUAL(preserved.router_valid, ack.router_valid); TEST_ASSERT_EQUAL(preserved.dns_valid, ack.dns_valid); TEST_ASSERT_EQUAL(preserved.dns_ipv4[0], ack.dns_ipv4[0]); }
}

int main(void) {
    unity_init();
    RUN_TEST(test_build_and_parse_dhcp_offer);
    unity_print_results();
    unity_cleanup();
    return (unity_stats.tests_failed == 0) ? 0 : 1;
}
