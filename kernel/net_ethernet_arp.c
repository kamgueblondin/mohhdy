#include "net_ethernet_arp.h"

static uint16_t read_be16(const uint8_t* p) {
    return (uint16_t)(((uint16_t)p[0] << 8) | p[1]);
}

static void write_be16(uint8_t* p, uint16_t value) {
    p[0] = (uint8_t)(value >> 8);
    p[1] = (uint8_t)value;
}

static void copy_bytes(uint8_t* dst, const uint8_t* src, uint32_t count) {
    uint32_t i;
    for (i = 0; i < count; ++i) dst[i] = src[i];
}

static int bytes_equal(const uint8_t* a, const uint8_t* b, uint32_t count) {
    uint32_t i;
    for (i = 0; i < count; ++i) if (a[i] != b[i]) return 0;
    return 1;
}

int net_ethernet_parse(const uint8_t* frame, uint32_t length,
                       net_ethernet_header_t* out) {
    if (!frame || !out || length < NET_ETHERNET_HEADER_SIZE) return -1;
    copy_bytes(out->destination, frame, 6);
    copy_bytes(out->source, frame + 6, 6);
    out->ethertype = read_be16(frame + 12);
    return 0;
}

int net_arp_parse(const uint8_t* frame, uint32_t length,
                  net_arp_packet_t* out) {
    net_ethernet_header_t ethernet;
    const uint8_t* arp;
    if (!out || !frame || length < NET_ETHERNET_HEADER_SIZE + NET_ARP_PACKET_SIZE)
        return -1;
    if (net_ethernet_parse(frame, length, &ethernet) != 0 ||
        ethernet.ethertype != NET_ETHERTYPE_ARP)
        return -2;
    arp = frame + NET_ETHERNET_HEADER_SIZE;
    out->hardware_type = read_be16(arp + 0);
    out->protocol_type = read_be16(arp + 2);
    out->hardware_size = arp[4];
    out->protocol_size = arp[5];
    out->opcode = read_be16(arp + 6);
    copy_bytes(out->sender_mac, arp + 8, 6);
    copy_bytes(out->sender_ipv4, arp + 14, 4);
    copy_bytes(out->target_mac, arp + 18, 6);
    copy_bytes(out->target_ipv4, arp + 24, 4);
    if (out->hardware_type != NET_ARP_HTYPE_ETHERNET ||
        out->protocol_type != NET_ARP_PTYPE_IPV4 ||
        out->hardware_size != 6 || out->protocol_size != 4)
        return -3;
    return 0;
}

int net_arp_build_request(uint8_t* frame, uint32_t capacity,
                          const uint8_t sender_mac[6], const uint8_t sender_ipv4[4],
                          const uint8_t target_ipv4[4]) {
    uint8_t* arp;
    static const uint8_t broadcast[6] = {0xff, 0xff, 0xff, 0xff, 0xff, 0xff};
    static const uint8_t zero_mac[6] = {0, 0, 0, 0, 0, 0};
    if (!frame || !sender_mac || !sender_ipv4 || !target_ipv4 ||
        capacity < NET_ETHERNET_HEADER_SIZE + NET_ARP_PACKET_SIZE)
        return -1;
    copy_bytes(frame, broadcast, 6);
    copy_bytes(frame + 6, sender_mac, 6);
    write_be16(frame + 12, NET_ETHERTYPE_ARP);
    arp = frame + NET_ETHERNET_HEADER_SIZE;
    write_be16(arp + 0, NET_ARP_HTYPE_ETHERNET);
    write_be16(arp + 2, NET_ARP_PTYPE_IPV4);
    arp[4] = 6;
    arp[5] = 4;
    write_be16(arp + 6, NET_ARP_OPCODE_REQUEST);
    copy_bytes(arp + 8, sender_mac, 6);
    copy_bytes(arp + 14, sender_ipv4, 4);
    copy_bytes(arp + 18, zero_mac, 6);
    copy_bytes(arp + 24, target_ipv4, 4);
    return (int)(NET_ETHERNET_HEADER_SIZE + NET_ARP_PACKET_SIZE);
}

int net_arp_build_reply(uint8_t* frame, uint32_t capacity,
                        const net_arp_packet_t* request,
                        const uint8_t local_mac[6], const uint8_t local_ipv4[4]) {
    uint8_t* arp;
    if (!frame || !request || !local_mac || !local_ipv4 ||
        capacity < NET_ETHERNET_HEADER_SIZE + NET_ARP_PACKET_SIZE ||
        request->opcode != NET_ARP_OPCODE_REQUEST ||
        request->hardware_type != NET_ARP_HTYPE_ETHERNET ||
        request->protocol_type != NET_ARP_PTYPE_IPV4 ||
        request->hardware_size != 6U || request->protocol_size != 4U)
        return -1;
    copy_bytes(frame, request->sender_mac, 6);
    copy_bytes(frame + 6, local_mac, 6);
    write_be16(frame + 12, NET_ETHERTYPE_ARP);
    arp = frame + NET_ETHERNET_HEADER_SIZE;
    write_be16(arp + 0, NET_ARP_HTYPE_ETHERNET);
    write_be16(arp + 2, NET_ARP_PTYPE_IPV4);
    arp[4] = 6U; arp[5] = 4U;
    write_be16(arp + 6, NET_ARP_OPCODE_REPLY);
    copy_bytes(arp + 8, local_mac, 6);
    copy_bytes(arp + 14, local_ipv4, 4);
    copy_bytes(arp + 18, request->sender_mac, 6);
    copy_bytes(arp + 24, request->sender_ipv4, 4);
    return (int)(NET_ETHERNET_HEADER_SIZE + NET_ARP_PACKET_SIZE);
}

int net_arp_cache_init(net_arp_cache_t* cache) {
    uint32_t i;
    if (!cache) return -1;
    for (i = 0; i < NET_ARP_CACHE_CAPACITY; ++i) cache->entries[i].valid = 0U;
    return 0;
}

int net_arp_cache_put(net_arp_cache_t* cache, const uint8_t ipv4[4], const uint8_t mac[6]) {
    uint32_t i; uint32_t free_index = NET_ARP_CACHE_CAPACITY;
    if (!cache || !ipv4 || !mac) return -1;
    for (i = 0; i < NET_ARP_CACHE_CAPACITY; ++i) {
        if (cache->entries[i].valid && bytes_equal(cache->entries[i].ipv4, ipv4, 4U)) {
            copy_bytes(cache->entries[i].mac, mac, 6U); return 0;
        }
        if (!cache->entries[i].valid && free_index == NET_ARP_CACHE_CAPACITY) free_index = i;
    }
    if (free_index == NET_ARP_CACHE_CAPACITY) return -2;
    cache->entries[free_index].valid = 1U;
    copy_bytes(cache->entries[free_index].ipv4, ipv4, 4U);
    copy_bytes(cache->entries[free_index].mac, mac, 6U);
    return 0;
}

int net_arp_cache_lookup(const net_arp_cache_t* cache, const uint8_t ipv4[4], uint8_t mac[6]) {
    uint32_t i;
    if (!cache || !ipv4 || !mac) return -1;
    for (i = 0; i < NET_ARP_CACHE_CAPACITY; ++i)
        if (cache->entries[i].valid && bytes_equal(cache->entries[i].ipv4, ipv4, 4U)) {
            copy_bytes(mac, cache->entries[i].mac, 6U); return 0;
        }
    return -2;
}

int net_arp_cache_invalidate(net_arp_cache_t* cache, const uint8_t ipv4[4]) {
    uint32_t i;
    if (!cache || !ipv4) return -1;
    for (i = 0; i < NET_ARP_CACHE_CAPACITY; ++i)
        if (cache->entries[i].valid && bytes_equal(cache->entries[i].ipv4, ipv4, 4U)) {
            cache->entries[i].valid = 0U; return 0;
        }
    return -2;
}

int net_arp_is_reply_for(const net_arp_packet_t* packet,
                         const uint8_t local_ipv4[4],
                         const uint8_t requested_ipv4[4]) {
    if (!packet || !local_ipv4 || !requested_ipv4) return 0;
    return packet->opcode == NET_ARP_OPCODE_REPLY &&
           bytes_equal(packet->target_ipv4, local_ipv4, 4) &&
           bytes_equal(packet->sender_ipv4, requested_ipv4, 4);
}
