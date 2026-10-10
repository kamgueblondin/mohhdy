#include "net_p2p.h"
#include "net_ipv4_udp.h"

int net_p2p_build(uint8_t* frame, uint16_t capacity, const uint8_t mac[6],
                  const uint8_t src_ip[4], const uint8_t dst_ip[4], uint16_t port,
                  const uint8_t* payload, uint16_t length) {
    int ip;
    uint16_t i, total;
    if (!frame || !mac || !src_ip || !dst_ip || port == 0U || capacity < NET_P2P_MIN_FRAME) return -1;
    for (i = 0U; i < 6U; i++) { frame[i] = 0xffU; frame[6U + i] = mac[i]; }
    frame[12] = 0x08U; frame[13] = 0x00U;
    ip = net_udp_build_ipv4(frame + NET_P2P_ETH_HEADER, (uint32_t)(capacity - NET_P2P_ETH_HEADER),
                            src_ip, dst_ip, port, port, payload, length);
    if (ip < 0) return -1;
    total = (uint16_t)(NET_P2P_ETH_HEADER + (uint16_t)ip);
    while (total < NET_P2P_MIN_FRAME) frame[total++] = 0U;
    return total;
}

int net_p2p_parse(const uint8_t* frame, uint16_t length, const uint8_t self_mac[6],
                  uint16_t port, net_p2p_view_t* out) {
    net_udp_view_t udp;
    uint16_t i;
    int self = 1;
    if (!frame || !out || length < NET_P2P_ETH_HEADER + 28U) return 0;
    if (frame[12] != 0x08U || frame[13] != 0x00U) return 0;
    if (self_mac) { for (i = 0U; i < 6U; i++) if (frame[6U + i] != self_mac[i]) self = 0; }
    else self = 0;
    if (self) return 0;
    if (net_udp_parse_ipv4(frame + NET_P2P_ETH_HEADER, (uint32_t)(length - NET_P2P_ETH_HEADER), &udp) != 0) return 0;
    if (udp.destination_port != port) return 0;
    for (i = 0U; i < 6U; i++) out->source_mac[i] = frame[6U + i];
    for (i = 0U; i < 4U; i++) { out->source_ip[i] = udp.source_ip[i]; out->destination_ip[i] = udp.destination_ip[i]; }
    out->payload = udp.payload;
    out->payload_length = udp.payload_length;
    return 1;
}
