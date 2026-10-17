/* US-074: STUN (RFC 5389) Binding codec and mapping classification. */
#include "../../framework/unity.h"
#include <string.h>
#include "../../../userspace/stun.h"

/* RFC 5769 section 2.2: sample IPv4 Binding success response. */
static const uint8_t rfc5769_ipv4[] = {
    0x01, 0x01, 0x00, 0x3c, 0x21, 0x12, 0xa4, 0x42,
    0xb7, 0xe7, 0xa7, 0x01, 0xbc, 0x34, 0xd6, 0x86, 0xfa, 0x87, 0xdf, 0xae,
    0x80, 0x22, 0x00, 0x0b, 0x74, 0x65, 0x73, 0x74, 0x20, 0x76, 0x65, 0x63, 0x74, 0x6f, 0x72, 0x20,
    0x00, 0x20, 0x00, 0x08, 0x00, 0x01, 0xa1, 0x47, 0xe1, 0x12, 0xa6, 0x43,
    0x00, 0x08, 0x00, 0x14, 0x2b, 0x91, 0xf5, 0x99, 0xfd, 0x9e, 0x90, 0xc3, 0x8c, 0x74,
    0x89, 0xf9, 0x2a, 0xf9, 0xba, 0x53, 0xf0, 0x6b, 0xe7, 0xd7,
    0x80, 0x28, 0x00, 0x04, 0xc0, 0x7d, 0x4c, 0x96,
};

void test_rfc5769_ipv4_response(void) {
    uint8_t ip[4], txid[12];
    uint16_t port = 0, type = 0;
    TEST_ASSERT_EQUAL(0, stun_parse_header(rfc5769_ipv4, sizeof(rfc5769_ipv4), &type, txid));
    TEST_ASSERT_EQUAL(STUN_BINDING_SUCCESS, type);
    TEST_ASSERT_EQUAL(0, stun_parse_mapped(rfc5769_ipv4, sizeof(rfc5769_ipv4), rfc5769_ipv4 + 8, ip, &port));
    TEST_ASSERT_EQUAL(32853, port);
    TEST_ASSERT_EQUAL(192, ip[0]); TEST_ASSERT_EQUAL(0, ip[1]);
    TEST_ASSERT_EQUAL(2, ip[2]); TEST_ASSERT_EQUAL(1, ip[3]);
    /* another transaction id: refused */
    memcpy(txid, rfc5769_ipv4 + 8, 12); txid[11] ^= 1;
    TEST_ASSERT_EQUAL(-3, stun_parse_mapped(rfc5769_ipv4, sizeof(rfc5769_ipv4), txid, ip, &port));
}

void test_build_and_parse_roundtrip(void) {
    static const uint8_t txid[12] = {1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12};
    static const uint8_t addr[4] = {203, 0, 113, 7};
    uint8_t m[64], ip[4];
    uint16_t port = 0, type = 0;
    int len = stun_build_binding_request(m, sizeof(m), txid);
    TEST_ASSERT_EQUAL(STUN_HEADER, len);
    TEST_ASSERT_EQUAL(0, stun_parse_header(m, len, &type, 0));
    TEST_ASSERT_EQUAL(STUN_BINDING_REQUEST, type);
    TEST_ASSERT_EQUAL(-2, stun_parse_mapped(m, len, txid, ip, &port));
    len = stun_build_binding_success(m, sizeof(m), txid, addr, 7700);
    TEST_ASSERT_EQUAL(STUN_BINDING_SUCCESS_LENGTH, len);
    TEST_ASSERT_EQUAL(0, stun_parse_mapped(m, len, txid, ip, &port));
    TEST_ASSERT_EQUAL(7700, port);
    TEST_ASSERT_EQUAL(0, memcmp(ip, addr, 4));
    /* the address travels XORed with the magic cookie */
    TEST_ASSERT_TRUE(memcmp(m + STUN_HEADER + 8, addr, 4) != 0);
    TEST_ASSERT_EQUAL(-1, stun_build_binding_success(m, 10, txid, addr, 1));
}

void test_malformed_messages(void) {
    static const uint8_t txid[12] = {0};
    uint8_t m[64], ip[4];
    uint16_t port;
    int len = stun_build_binding_success(m, sizeof(m), txid, (const uint8_t*)"\x01\x02\x03\x04", 9);
    TEST_ASSERT_EQUAL(-1, stun_parse_header(m, 10, 0, 0));
    m[4] ^= 0xff;                                   /* bad magic cookie */
    TEST_ASSERT_EQUAL(-3, stun_parse_header(m, len, 0, 0));
    m[4] ^= 0xff; m[0] |= 0x80;                     /* top bits set */
    TEST_ASSERT_EQUAL(-2, stun_parse_header(m, len, 0, 0));
    m[0] &= 0x3f;
    TEST_ASSERT_EQUAL(-4, stun_parse_header(m, len - 1, 0, 0));
    m[STUN_HEADER + 3] = 40;                        /* attribute overruns */
    TEST_ASSERT_EQUAL(-4, stun_parse_mapped(m, len, txid, ip, &port));
    m[STUN_HEADER + 3] = 8; m[STUN_HEADER + 1] = 0x99; /* unknown attribute only */
    TEST_ASSERT_EQUAL(-5, stun_parse_mapped(m, len, txid, ip, &port));
}

void test_nat_classification(void) {
    static const uint8_t local[4] = {10, 77, 0, 1}, pub[4] = {203, 0, 113, 7}, pub2[4] = {203, 0, 113, 9};
    TEST_ASSERT_EQUAL(STUN_NAT_DIRECT, stun_nat_classify(local, 7700, local, 7700, 0, 0, 0));
    TEST_ASSERT_EQUAL(STUN_NAT_DIRECT, stun_nat_classify(local, 7700, local, 7700, 1, local, 7700));
    TEST_ASSERT_EQUAL(STUN_NAT_UNKNOWN, stun_nat_classify(local, 7700, pub, 7700, 0, 0, 0));
    TEST_ASSERT_EQUAL(STUN_NAT_UNKNOWN, stun_nat_classify(local, 7700, local, 4000, 0, 0, 0));
    TEST_ASSERT_EQUAL(STUN_NAT_INDEPENDENT, stun_nat_classify(local, 7700, pub, 40000, 1, pub, 40000));
    TEST_ASSERT_EQUAL(STUN_NAT_DEPENDENT, stun_nat_classify(local, 7700, pub, 40000, 1, pub, 40001));
    TEST_ASSERT_EQUAL(STUN_NAT_DEPENDENT, stun_nat_classify(local, 7700, pub, 7700, 1, pub2, 7700));
    TEST_ASSERT_EQUAL(-1, stun_nat_classify(0, 0, pub, 1, 0, 0, 0));
    TEST_ASSERT_EQUAL_STRING("endpoint_dependent", stun_nat_name(STUN_NAT_DEPENDENT));
    TEST_ASSERT_EQUAL_STRING("invalid", stun_nat_name(42));
}

int main(void) {
    unity_init();
    RUN_TEST(test_rfc5769_ipv4_response);
    RUN_TEST(test_build_and_parse_roundtrip);
    RUN_TEST(test_malformed_messages);
    RUN_TEST(test_nat_classification);
    unity_print_results();
    unity_cleanup();
    return (unity_stats.tests_failed == 0) ? 0 : 1;
}
