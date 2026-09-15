#include "../../framework/unity.h"
#include "../../../kernel/net_dns.h"

void setUp(void) {}
void tearDown(void) {}

void test_dns_query_and_a_response(void) {
    uint8_t packet[128] = {0};
    net_dns_a_result_t result;
    int length = net_dns_build_a_query(packet, sizeof(packet), 0x4321, "example.com");
    TEST_ASSERT_GREATER_THAN(12, length);
    TEST_ASSERT_EQUAL(0x43, packet[0]); TEST_ASSERT_EQUAL(0x21, packet[1]);
    packet[2] = 0x81; packet[3] = 0x80; packet[6] = 0; packet[7] = 1;
    packet[length++] = 0xc0; packet[length++] = 0x0c;
    packet[length++] = 0; packet[length++] = 1; packet[length++] = 0; packet[length++] = 1;
    packet[length++] = 0; packet[length++] = 0; packet[length++] = 0; packet[length++] = 30;
    packet[length++] = 0; packet[length++] = 4; packet[length++] = 10; packet[length++] = 0; packet[length++] = 2; packet[length++] = 15;
    TEST_ASSERT_EQUAL(0, net_dns_parse_a_response(packet, (uint32_t)length, 0x4321, &result));
    TEST_ASSERT_EQUAL(10, result.address[0]); TEST_ASSERT_EQUAL(15, result.address[3]);
    TEST_ASSERT_NOT_EQUAL(0, net_dns_parse_a_response(packet, (uint32_t)length, 0x1111, &result));
}

int main(void) {
    unity_init();
    RUN_TEST(test_dns_query_and_a_response);
    unity_print_results();
    unity_cleanup();
    return (unity_stats.tests_failed == 0) ? 0 : 1;
}
