#include "net_dns.h"

static uint16_t get16(const uint8_t* p) { return (uint16_t)(((uint16_t)p[0] << 8) | p[1]); }
static void put16(uint8_t* p, uint16_t v) { p[0] = (uint8_t)(v >> 8); p[1] = (uint8_t)v; }

int net_dns_build_a_query(uint8_t* packet, uint32_t capacity,
                          uint16_t id, const char* hostname) {
    uint32_t pos = NET_DNS_HEADER_SIZE, label_start, label_len = 0U;
    const char* cursor;
    uint32_t i;
    if (!packet || !hostname || capacity < NET_DNS_HEADER_SIZE + 5U) return -1;
    for (i = 0; i < NET_DNS_HEADER_SIZE; ++i) packet[i] = 0U;
    put16(packet, id); packet[2] = 1U; packet[5] = 1U;
    cursor = hostname;
    while (*cursor) {
        label_start = pos; pos++;
        label_len = 0U;
        while (cursor[label_len] && cursor[label_len] != '.') {
            if (label_len >= 63U) return -2;
            label_len++;
        }
        if (pos + label_len + 1U + 4U > capacity) return -3;
        packet[label_start] = (uint8_t)label_len;
        for (i = 0; i < label_len; ++i) packet[pos + i] = (uint8_t)cursor[i];
        pos += label_len;
        cursor += label_len;
        if (*cursor == '.') cursor++;
    }
    if (label_len == 0U || pos + 5U > capacity) return -4;
    packet[pos++] = 0U; put16(packet + pos, NET_DNS_TYPE_A); pos += 2U;
    put16(packet + pos, NET_DNS_CLASS_IN); pos += 2U;
    return (int)pos;
}

int net_dns_parse_a_response(const uint8_t* packet, uint32_t length,
                             uint16_t expected_id, net_dns_a_result_t* out) {
    uint16_t qd, an; uint32_t pos; uint16_t i;
    if (!packet || !out || length < NET_DNS_HEADER_SIZE || get16(packet) != expected_id ||
        (packet[2] & 0x80U) == 0U || (packet[3] & 0x0fU) != 0U) return -1;
    qd = get16(packet + 4); an = get16(packet + 6);
    if (qd == 0U || an == 0U) return -2;
    pos = NET_DNS_HEADER_SIZE;
    for (i = 0; i < qd; ++i) {
        uint32_t guard = 0U;
        while (pos < length && packet[pos] != 0U && guard++ < length) {
            uint8_t n = packet[pos++];
            if ((n & 0xc0U) != 0U || n > 63U || pos + n > length) return -3;
            pos += n;
        }
        if (pos >= length || ++pos + 4U > length) return -3;
        pos += 4U;
    }
    for (i = 0; i < an; ++i) {
        uint16_t type, class_code, rdlen;
        if (pos + 12U > length) return -4;
        if ((packet[pos] & 0xc0U) == 0xc0U) pos += 2U;
        else { while (pos < length && packet[pos] != 0U) { uint8_t n=packet[pos++]; if (n>63U || pos+n>length) return -4; pos+=n; } if (pos>=length) return -4; pos++; }
        if (pos + 10U > length) return -4;
        type = get16(packet + pos); class_code = get16(packet + pos + 2U); rdlen = get16(packet + pos + 8U); pos += 10U;
        if (pos + rdlen > length) return -4;
        if (type == NET_DNS_TYPE_A && class_code == NET_DNS_CLASS_IN && rdlen == 4U) {
            out->id = expected_id; out->address[0]=packet[pos]; out->address[1]=packet[pos+1]; out->address[2]=packet[pos+2]; out->address[3]=packet[pos+3]; return 0;
        }
        pos += rdlen;
    }
    return -5;
}
