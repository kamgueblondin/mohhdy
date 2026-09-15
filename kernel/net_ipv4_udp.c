#include "net_ipv4_udp.h"

static uint16_t get16(const uint8_t* p) { return (uint16_t)(((uint16_t)p[0] << 8) | p[1]); }
static void put16(uint8_t* p, uint16_t v) { p[0] = (uint8_t)(v >> 8); p[1] = (uint8_t)v; }
static void copy4(uint8_t* d, const uint8_t* s) { d[0]=s[0]; d[1]=s[1]; d[2]=s[2]; d[3]=s[3]; }

uint16_t net_ipv4_checksum(const uint8_t* header, uint16_t length) {
    uint32_t sum = 0U; uint16_t i;
    if (!header || (length & 1U) != 0U) return 0U;
    for (i = 0; i < length; i += 2U) {
        sum += get16(header + i);
        while (sum >> 16) sum = (sum & 0xffffU) + (sum >> 16);
    }
    return (uint16_t)~sum;
}

int net_udp_build_ipv4(uint8_t* packet, uint32_t capacity,
                       const uint8_t source_ip[4], const uint8_t destination_ip[4],
                       uint16_t source_port, uint16_t destination_port,
                       const uint8_t* payload, uint16_t payload_length) {
    uint16_t total = (uint16_t)(NET_IPV4_HEADER_SIZE + NET_UDP_HEADER_SIZE + payload_length);
    uint16_t i;
    if (!packet || !source_ip || !destination_ip || (payload_length && !payload) ||
        capacity < total || total < NET_IPV4_HEADER_SIZE + NET_UDP_HEADER_SIZE)
        return -1;
    for (i = 0; i < NET_IPV4_HEADER_SIZE + NET_UDP_HEADER_SIZE; ++i) packet[i] = 0U;
    packet[0] = 0x45U; put16(packet + 2, total); packet[6] = 0x40U;
    packet[8] = 64U; packet[9] = NET_IPV4_PROTOCOL_UDP;
    copy4(packet + 12, source_ip); copy4(packet + 16, destination_ip);
    put16(packet + 10, net_ipv4_checksum(packet, NET_IPV4_HEADER_SIZE));
    put16(packet + 20, source_port); put16(packet + 22, destination_port);
    put16(packet + 24, (uint16_t)(NET_UDP_HEADER_SIZE + payload_length));
    /* UDP checksum is optional for IPv4; zero is explicit and deterministic. */
    put16(packet + 26, 0U);
    for (i = 0; i < payload_length; ++i) packet[28U + i] = payload[i];
    return total;
}

int net_udp_parse_ipv4(const uint8_t* packet, uint32_t length,
                       net_udp_view_t* out) {
    uint16_t total; uint16_t udp_length;
    if (!packet || !out || length < 28U || (packet[0] >> 4) != 4U ||
        (packet[0] & 0x0fU) != 5U || packet[9] != NET_IPV4_PROTOCOL_UDP)
        return -1;
    total = get16(packet + 2); udp_length = get16(packet + 24);
    if (total < 28U || total > length || udp_length < 8U ||
        udp_length > total - 20U || net_ipv4_checksum(packet, 20U) != 0U)
        return -2;
    copy4(out->source_ip, packet + 12); copy4(out->destination_ip, packet + 16);
    out->source_port = get16(packet + 20); out->destination_port = get16(packet + 22);
    out->payload = packet + 28; out->payload_length = (uint16_t)(udp_length - 8U);
    return 0;
}

uint16_t net_icmp_checksum(const uint8_t* buffer, uint16_t length) {
    uint32_t sum = 0U; uint16_t i;
    if (!buffer) return 0U;
    for (i = 0U; i + 1U < length; i += 2U) {
        sum += get16(buffer + i);
        while (sum >> 16) sum = (sum & 0xffffU) + (sum >> 16);
    }
    if (i < length) {
        sum += (uint16_t)((uint16_t)buffer[i] << 8);
        while (sum >> 16) sum = (sum & 0xffffU) + (sum >> 16);
    }
    return (uint16_t)~sum;
}

int net_icmp_build_ipv4(uint8_t* packet, uint32_t capacity,
                        const uint8_t source_ip[4], const uint8_t destination_ip[4],
                        uint8_t type, uint8_t code, uint16_t identifier, uint16_t sequence,
                        const uint8_t* payload, uint16_t payload_length) {
    uint16_t total = (uint16_t)(NET_IPV4_HEADER_SIZE + NET_ICMP_HEADER_SIZE + payload_length);
    uint16_t i;
    uint16_t icmp_len;
    if (!packet || !source_ip || !destination_ip || (payload_length && !payload) ||
        capacity < total || total < NET_IPV4_HEADER_SIZE + NET_ICMP_HEADER_SIZE)
        return -1;
    for (i = 0; i < NET_IPV4_HEADER_SIZE + NET_ICMP_HEADER_SIZE; ++i) packet[i] = 0U;
    packet[0] = 0x45U; put16(packet + 2, total); packet[6] = 0x40U;
    packet[8] = 64U; packet[9] = NET_IPV4_PROTOCOL_ICMP;
    copy4(packet + 12, source_ip); copy4(packet + 16, destination_ip);
    put16(packet + 10, net_ipv4_checksum(packet, NET_IPV4_HEADER_SIZE));

    /* ICMP Header */
    packet[20] = type;
    packet[21] = code;
    put16(packet + 22, 0U); /* Checksum placeholder */
    put16(packet + 24, identifier);
    put16(packet + 26, sequence);
    for (i = 0; i < payload_length; ++i) packet[28U + i] = payload[i];

    icmp_len = (uint16_t)(NET_ICMP_HEADER_SIZE + payload_length);
    put16(packet + 22, net_icmp_checksum(packet + 20, icmp_len));
    return total;
}

int net_icmp_parse_ipv4(const uint8_t* packet, uint32_t length,
                        net_icmp_view_t* out) {
    uint16_t total; uint16_t icmp_length;
    if (!packet || !out || length < 28U || (packet[0] >> 4) != 4U ||
        (packet[0] & 0x0fU) != 5U || packet[9] != NET_IPV4_PROTOCOL_ICMP)
        return -1;
    total = get16(packet + 2);
    if (total < 28U || total > length || net_ipv4_checksum(packet, 20U) != 0U)
        return -2;
    icmp_length = (uint16_t)(total - NET_IPV4_HEADER_SIZE);
    if (net_icmp_checksum(packet + 20, icmp_length) != 0U)
        return -3;
    copy4(out->source_ip, packet + 12); copy4(out->destination_ip, packet + 16);
    out->type = packet[20];
    out->code = packet[21];
    out->identifier = get16(packet + 24);
    out->sequence = get16(packet + 26);
    out->payload = packet + 28;
    out->payload_length = (uint16_t)(icmp_length - NET_ICMP_HEADER_SIZE);
    return 0;
}
