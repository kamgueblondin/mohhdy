#include "net_dhcp.h"

static void put_be32(uint8_t* p, uint32_t value) {
    p[0] = (uint8_t)(value >> 24); p[1] = (uint8_t)(value >> 16);
    p[2] = (uint8_t)(value >> 8); p[3] = (uint8_t)value;
}
static uint32_t get_be32(const uint8_t* p) {
    return ((uint32_t)p[0] << 24) | ((uint32_t)p[1] << 16) |
           ((uint32_t)p[2] << 8) | p[3];
}
static void copy_bytes(uint8_t* dst, const uint8_t* src, uint32_t n) {
    uint32_t i; for (i = 0; i < n; ++i) dst[i] = src[i];
}

int net_dhcp_build_discover(uint8_t* packet, uint32_t capacity,
                            uint32_t xid, const uint8_t mac[6]) {
    uint32_t i;
    if (!packet || !mac || capacity < 249U) return -1;
    for (i = 0; i < 249U; ++i) packet[i] = 0U;
    packet[0] = 1U; packet[1] = 1U; packet[2] = 6U;
    put_be32(packet + 4, xid);
    copy_bytes(packet + 28, mac, 6);
    put_be32(packet + 236, NET_DHCP_MAGIC_COOKIE);
    packet[240] = NET_DHCP_OPTION_MESSAGE_TYPE; packet[241] = 1U;
    packet[242] = NET_DHCP_DISCOVER;
    packet[243] = NET_DHCP_OPTION_PARAMETER_REQUEST_LIST; packet[244] = 3U;
    packet[245] = 1U; packet[246] = NET_DHCP_OPTION_ROUTER; packet[247] = NET_DHCP_OPTION_DNS;
    packet[248] = NET_DHCP_OPTION_END;
    return 249;
}

int net_dhcp_build_request(uint8_t* packet, uint32_t capacity,
                           uint32_t xid, const uint8_t mac[6],
                           const uint8_t requested_ip[4], const uint8_t server_ip[4]) {
    uint32_t i;
    if (!packet || !mac || !requested_ip || !server_ip || capacity < 252U) return -1;
    for (i = 0; i < 252U; ++i) packet[i] = 0U;
    packet[0] = 1U; packet[1] = 1U; packet[2] = 6U; put_be32(packet + 4, xid);
    copy_bytes(packet + 28, mac, 6U); put_be32(packet + 236, NET_DHCP_MAGIC_COOKIE);
    packet[240] = NET_DHCP_OPTION_MESSAGE_TYPE; packet[241] = 1U; packet[242] = NET_DHCP_REQUEST;
    packet[243] = NET_DHCP_OPTION_SERVER_ID; packet[244] = 4U; copy_bytes(packet + 245, server_ip, 4U);
    packet[249] = 50U; packet[250] = 4U; copy_bytes(packet + 251, requested_ip, 4U);
    return 255;
}

int net_dhcp_build_renew(uint8_t* packet, uint32_t capacity,
                         uint32_t xid, const uint8_t mac[6],
                         const uint8_t client_ip[4]) {
    uint32_t i;
    if (!packet || !mac || !client_ip || capacity < 249U) return -1;
    for (i = 0U; i < 249U; i++) packet[i] = 0U;
    packet[0] = 1U; packet[1] = 1U; packet[2] = 6U; put_be32(packet + 4, xid);
    copy_bytes(packet + 12, client_ip, 4U); copy_bytes(packet + 28, mac, 6U);
    put_be32(packet + 236, NET_DHCP_MAGIC_COOKIE);
    packet[240] = NET_DHCP_OPTION_MESSAGE_TYPE; packet[241] = 1U; packet[242] = NET_DHCP_REQUEST;
    packet[243] = NET_DHCP_OPTION_PARAMETER_REQUEST_LIST; packet[244] = 3U;
    packet[245] = NET_DHCP_OPTION_SUBNET_MASK; packet[246] = NET_DHCP_OPTION_ROUTER;
    packet[247] = NET_DHCP_OPTION_DNS; packet[248] = NET_DHCP_OPTION_END;
    return 249;
}

int net_dhcp_parse_offer(const uint8_t* packet, uint32_t length,
                         uint32_t expected_xid, net_dhcp_offer_t* out) {
    uint32_t pos;
    uint8_t type = 0U;
    uint8_t server_seen = 0U;
    if (!packet || !out || length < 244U || packet[0] != 2U ||
        get_be32(packet + 4) != expected_xid ||
        get_be32(packet + 236) != NET_DHCP_MAGIC_COOKIE)
        return -1;
    copy_bytes(out->offered_ip, packet + 16, 4);
    out->server_ip[0] = out->server_ip[1] = out->server_ip[2] = out->server_ip[3] = 0U;
    out->xid = expected_xid;
    for (pos = 240U; pos < length;) {
        uint8_t code = packet[pos++];
        uint8_t size;
        if (code == NET_DHCP_OPTION_END) break;
        if (code == 0U) continue;
        if (pos >= length) return -2;
        size = packet[pos++];
        if (pos + size > length) return -2;
        if (code == NET_DHCP_OPTION_MESSAGE_TYPE && size == 1U) type = packet[pos];
        if (code == NET_DHCP_OPTION_SERVER_ID && size == 4U) {
            copy_bytes(out->server_ip, packet + pos, 4); server_seen = 1U;
        }
        pos += size;
    }
    if (type != NET_DHCP_OFFER || !server_seen) return -3;
    out->message_type = type;
    return 0;
}
int net_dhcp_lease_apply(net_dhcp_lease_t* lease, const net_dhcp_offer_t* offer) {
    net_dhcp_lease_t next = {0};
    if (!lease || !offer || offer->message_type != NET_DHCP_OFFER) return -1;
    copy_bytes(next.ipv4, offer->offered_ip, 4U);
    copy_bytes(next.server_ipv4, offer->server_ip, 4U);
    next.xid = offer->xid;
    next.valid = 1U;
    *lease = next;
    return 0;
}
void net_dhcp_lease_clear(net_dhcp_lease_t* lease) {
    net_dhcp_lease_t empty = {0};
    if (!lease) return;
    *lease = empty;
}
int net_dhcp_parse_ack(const uint8_t* packet, uint32_t length,
                       uint32_t expected_xid, net_dhcp_lease_t* lease) {
    uint32_t pos; uint8_t type = 0U; uint8_t server_seen = 0U; net_dhcp_lease_t next = {0};
    if (!packet || !lease || length < 244U || packet[0] != 2U ||
        get_be32(packet + 4) != expected_xid || get_be32(packet + 236) != NET_DHCP_MAGIC_COOKIE)
        return -1;
    for (pos = 240U; pos < length;) {
        uint8_t code = packet[pos++], size;
        if (code == NET_DHCP_OPTION_END) break;
        if (code == 0U) continue;
        if (pos >= length) return -2;
        size = packet[pos++]; if (pos + size > length) return -2;
        if (code == NET_DHCP_OPTION_MESSAGE_TYPE && size == 1U) type = packet[pos];
        if (code == NET_DHCP_OPTION_SERVER_ID && size == 4U) { copy_bytes(next.server_ipv4, packet + pos, 4U); server_seen = 1U; }
        if (code == NET_DHCP_OPTION_SUBNET_MASK) { if (size != 4U) return -4; copy_bytes(next.subnet_mask, packet + pos, 4U); next.subnet_valid = 1U; }
        if (code == NET_DHCP_OPTION_ROUTER) { if (size < 4U || (size & 3U) != 0U) return -4; copy_bytes(next.router_ipv4, packet + pos, 4U); next.router_valid = 1U; }
        if (code == NET_DHCP_OPTION_DNS) { if (size < 4U || (size & 3U) != 0U) return -4; copy_bytes(next.dns_ipv4, packet + pos, 4U); next.dns_valid = 1U; }
        if (code == NET_DHCP_OPTION_LEASE_TIME) { if (size != 4U) return -4; next.lease_seconds = get_be32(packet + pos); }
        pos += size;
    }
    if (type != NET_DHCP_ACK || !server_seen) return -3;
    copy_bytes(next.ipv4, packet + 16, 4U); next.xid = expected_xid; next.valid = 1U;
    *lease = next;
    return 0;
}

int net_dhcp_lease_next_hop(const net_dhcp_lease_t* lease,const uint8_t destination[4],uint8_t next_hop[4]){uint8_t next[4],index,local=1U;if(!lease||!destination||!next_hop||!lease->valid||!lease->subnet_valid)return -1;for(index=0U;index<4U;index++)if((uint8_t)(lease->ipv4[index]&lease->subnet_mask[index])!=(uint8_t)(destination[index]&lease->subnet_mask[index]))local=0U;if(local)copy_bytes(next,destination,4U);else{if(!lease->router_valid)return -2;copy_bytes(next,lease->router_ipv4,4U);}copy_bytes(next_hop,next,4U);return 0;
}
int net_dhcp_lease_mark_acquired(net_dhcp_lease_t* lease,uint32_t now){if(!lease||!lease->valid||lease->lease_seconds==0U)return -1;lease->acquired_tick=now;return 0;}
int net_dhcp_lease_is_valid_at(const net_dhcp_lease_t* lease,uint32_t now){uint32_t elapsed;if(!lease||!lease->valid||lease->lease_seconds==0U)return 0;elapsed=now-lease->acquired_tick;return elapsed<lease->lease_seconds?1:0;}
int net_dhcp_lease_renewal_due(const net_dhcp_lease_t* lease,uint32_t now){uint32_t elapsed,renew_after;if(!lease||!lease->valid||lease->lease_seconds==0U)return 0;elapsed=now-lease->acquired_tick;renew_after=lease->lease_seconds-(lease->lease_seconds/2U);return elapsed>=renew_after?1:0;}
