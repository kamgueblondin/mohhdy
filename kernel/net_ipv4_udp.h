#ifndef MOHHDY_NET_IPV4_UDP_H
#define MOHHDY_NET_IPV4_UDP_H

#include <stdint.h>

#define NET_IPV4_HEADER_SIZE 20U
#define NET_UDP_HEADER_SIZE 8U
#define NET_IPV4_PROTOCOL_ICMP 1U
#define NET_IPV4_PROTOCOL_UDP 17U
#define NET_ICMP_HEADER_SIZE 8U

#define NET_ICMP_TYPE_ECHO_REPLY 0U
#define NET_ICMP_TYPE_ECHO_REQUEST 8U

typedef struct {
    uint8_t source_ip[4];
    uint8_t destination_ip[4];
    uint16_t source_port;
    uint16_t destination_port;
    const uint8_t* payload;
    uint16_t payload_length;
} net_udp_view_t;

typedef struct {
    uint8_t source_ip[4];
    uint8_t destination_ip[4];
    uint8_t type;
    uint8_t code;
    uint16_t identifier;
    uint16_t sequence;
    const uint8_t* payload;
    uint16_t payload_length;
} net_icmp_view_t;

uint16_t net_ipv4_checksum(const uint8_t* header, uint16_t length);
int net_udp_build_ipv4(uint8_t* packet, uint32_t capacity,
                       const uint8_t source_ip[4], const uint8_t destination_ip[4],
                       uint16_t source_port, uint16_t destination_port,
                       const uint8_t* payload, uint16_t payload_length);
int net_udp_parse_ipv4(const uint8_t* packet, uint32_t length,
                       net_udp_view_t* out);

uint16_t net_icmp_checksum(const uint8_t* buffer, uint16_t length);
int net_icmp_build_ipv4(uint8_t* packet, uint32_t capacity,
                        const uint8_t source_ip[4], const uint8_t destination_ip[4],
                        uint8_t type, uint8_t code, uint16_t identifier, uint16_t sequence,
                        const uint8_t* payload, uint16_t payload_length);
int net_icmp_parse_ipv4(const uint8_t* packet, uint32_t length,
                        net_icmp_view_t* out);

#endif
