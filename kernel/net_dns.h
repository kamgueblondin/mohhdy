#ifndef MOHHDY_NET_DNS_H
#define MOHHDY_NET_DNS_H

#include <stdint.h>

#define NET_DNS_HEADER_SIZE 12U
#define NET_DNS_TYPE_A 1U
#define NET_DNS_CLASS_IN 1U

typedef struct {
    uint16_t id;
    uint8_t address[4];
} net_dns_a_result_t;

int net_dns_build_a_query(uint8_t* packet, uint32_t capacity,
                          uint16_t id, const char* hostname);
int net_dns_parse_a_response(const uint8_t* packet, uint32_t length,
                             uint16_t expected_id, net_dns_a_result_t* out);

#endif
