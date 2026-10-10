/* kernel/net_p2p.h - Phase 5 P2P datagrams (US-061): Ethernet broadcast +
 * IPv4/UDP frames on the shared segment, built and parsed without state.
 * Used by the SYS_PEER_DATA P2P sub-operations (Ring 3 networker on the
 * strict kernel, Ring 0 on the legacy image). */
#ifndef MOHHDY_NET_P2P_H
#define MOHHDY_NET_P2P_H
#include <stdint.h>

#define NET_P2P_ETH_HEADER 14U
#define NET_P2P_MIN_FRAME 60U

typedef struct {
    uint8_t source_mac[6];
    uint8_t source_ip[4];
    uint8_t destination_ip[4];
    const uint8_t* payload;
    uint16_t payload_length;
} net_p2p_view_t;

/* Broadcast frame (dst MAC ff:ff:ff:ff:ff:ff) carrying a UDP datagram from
 * src_ip:port to dst_ip:port. Returns the frame length or -1. */
int net_p2p_build(uint8_t* frame, uint16_t capacity, const uint8_t mac[6],
                  const uint8_t src_ip[4], const uint8_t dst_ip[4], uint16_t port,
                  const uint8_t* payload, uint16_t length);
/* 1 if the frame is an IPv4/UDP datagram to `port` not sent by self_mac,
 * 0 otherwise (other traffic, own echo, malformed). */
int net_p2p_parse(const uint8_t* frame, uint16_t length, const uint8_t self_mac[6],
                  uint16_t port, net_p2p_view_t* out);
#endif
