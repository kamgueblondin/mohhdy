/* kernel/net_wire.c - Tranche 5 slice 3: worker-only NE2000 wire path.
 * See net_wire.h. Everything here runs in Ring 0 on behalf of the Ring 3
 * net-driver worker; the syscall layer refuses every other caller. */

#include "net_wire.h"

extern uint32_t timer_get_ticks(void) __attribute__((weak));

static net_wire_binding_t g_bind[NET_WIRE_BINDINGS];
static os_net_wire_status_t g_stat;
static uint8_t g_payload[OS_NET_WIRE_MAX_IO];
static uint8_t g_segment[NET_TCP_HEADER_SIZE + OS_NET_WIRE_MAX_IO];

typedef struct {
    int target;
    uint8_t established;
    uint8_t data;
    uint8_t acked;
    uint8_t fin;
    uint8_t reset;
    uint32_t want_ack; /* local_sequence the peer must acknowledge */
} wire_observe_t;

static void wire_pause(void) {
    uint32_t before, spins = 0U;
    if (!timer_get_ticks) return;
    before = timer_get_ticks();
    while (timer_get_ticks() == before && spins < 20000000U) {
#ifdef __i386__
        __asm__ volatile ("pause" : : : "memory");
#endif
        ++spins;
    }
}

static int ip_eq(const uint8_t* a, const uint8_t* b) {
    return a[0] == b[0] && a[1] == b[1] && a[2] == b[2] && a[3] == b[3];
}

static int ip_zero(const uint8_t* a) {
    return a[0] == 0U && a[1] == 0U && a[2] == 0U && a[3] == 0U;
}

static int map_socket_error(int status) {
    switch (status) {
        case NET_SOCKET_BAD_ARGUMENT: return OS_SOCKET_BAD_ARGUMENT;
        case NET_SOCKET_NO_SLOT: return OS_SOCKET_NO_SLOT;
        case NET_SOCKET_NOT_OPEN: return OS_SOCKET_NOT_OPEN;
        case NET_SOCKET_NOT_CONNECTED: return OS_SOCKET_NOT_CONNECTED;
        case NET_SOCKET_BUFFER_SMALL: return OS_SOCKET_BUFFER_SMALL;
        default: return OS_SOCKET_PROTOCOL;
    }
}

static uint16_t clamp_attempts(uint16_t attempts) {
    if (attempts == 0U) return NET_WIRE_DEFAULT_ATTEMPTS;
    return attempts > NET_WIRE_MAX_ATTEMPTS ? NET_WIRE_MAX_ATTEMPTS : attempts;
}

void net_wire_reset(void) {
    uint32_t i;
    uint8_t* p = (uint8_t*)&g_stat;
    for (i = 0U; i < NET_WIRE_BINDINGS; i++) g_bind[i].bound = 0U;
    for (i = 0U; i < sizeof(g_stat); i++) p[i] = 0U;
}

int net_wire_bind(int socket_id, const uint8_t local_ip[4], const uint8_t remote_ip[4],
                  uint16_t local_port, uint16_t remote_port) {
    uint32_t i;
    if (socket_id < 0 || (uint32_t)socket_id >= NET_WIRE_BINDINGS || !local_ip || !remote_ip ||
        ip_zero(local_ip) || ip_zero(remote_ip) || local_port == 0U || remote_port == 0U)
        return -1;
    /* A 4-tuple is owned by one socket only, so the demux is unambiguous. */
    for (i = 0U; i < NET_WIRE_BINDINGS; i++) {
        if ((int)i == socket_id || !g_bind[i].bound) continue;
        if (g_bind[i].local_port == local_port && g_bind[i].remote_port == remote_port &&
            ip_eq(g_bind[i].local_ip, local_ip) && ip_eq(g_bind[i].remote_ip, remote_ip))
            return -2;
    }
    for (i = 0U; i < 4U; i++) {
        g_bind[socket_id].local_ip[i] = local_ip[i];
        g_bind[socket_id].remote_ip[i] = remote_ip[i];
    }
    g_bind[socket_id].local_port = local_port;
    g_bind[socket_id].remote_port = remote_port;
    g_bind[socket_id].bound = 1U;
    return 0;
}

int net_wire_unbind(int socket_id) {
    if (!net_wire_is_bound(socket_id)) return -1;
    g_bind[socket_id].bound = 0U;
    return 0;
}

int net_wire_is_bound(int socket_id) {
    return socket_id >= 0 && (uint32_t)socket_id < NET_WIRE_BINDINGS && g_bind[socket_id].bound;
}

uint32_t net_wire_bound_count(void) {
    uint32_t i, n = 0U;
    for (i = 0U; i < NET_WIRE_BINDINGS; i++) if (g_bind[i].bound) n++;
    return n;
}

static int local_ip_bound(const uint8_t* ip) {
    uint32_t i;
    for (i = 0U; i < NET_WIRE_BINDINGS; i++)
        if (g_bind[i].bound && ip_eq(g_bind[i].local_ip, ip)) return 1;
    return 0;
}

int net_wire_demux(const uint8_t* frame, uint16_t length, uint16_t* tcp_offset,
                   uint16_t* tcp_length) {
    uint16_t ethertype, ihl, ip_length, src_port, dst_port;
    const uint8_t* ip;
    uint32_t i;
    if (!frame || length < NET_ETHERNET_HEADER_SIZE) return NET_WIRE_DEMUX_DROP;
    ethertype = (uint16_t)(((uint16_t)frame[12] << 8) | frame[13]);
    if (ethertype == NET_ETHERTYPE_ARP) {
        net_arp_packet_t arp;
        if (net_arp_parse(frame, length, &arp) != 0) return NET_WIRE_DEMUX_DROP;
        if (!local_ip_bound(arp.target_ipv4)) return NET_WIRE_DEMUX_DROP;
        if (arp.opcode == NET_ARP_OPCODE_REQUEST) return NET_WIRE_DEMUX_ARP_REQUEST;
        if (arp.opcode == NET_ARP_OPCODE_REPLY) return NET_WIRE_DEMUX_ARP_REPLY;
        return NET_WIRE_DEMUX_DROP;
    }
    if (ethertype != NET_ETHERTYPE_IPV4 || length < NET_ETHERNET_HEADER_SIZE + 20U)
        return NET_WIRE_DEMUX_DROP;
    ip = frame + NET_ETHERNET_HEADER_SIZE;
    if ((ip[0] >> 4) != 4U) return NET_WIRE_DEMUX_DROP;
    ihl = (uint16_t)((ip[0] & 0x0fU) * 4U);
    ip_length = (uint16_t)(((uint16_t)ip[2] << 8) | ip[3]);
    if (ihl < 20U || ip[9] != NET_TCP_PROTOCOL || ip_length < ihl + NET_TCP_HEADER_SIZE ||
        (uint32_t)NET_ETHERNET_HEADER_SIZE + ip_length > length)
        return NET_WIRE_DEMUX_DROP;
    /* Fragments are not reassembled on this path. */
    if ((ip[6] & 0x3fU) != 0U || ip[7] != 0U) return NET_WIRE_DEMUX_DROP;
    src_port = (uint16_t)(((uint16_t)ip[ihl] << 8) | ip[ihl + 1U]);
    dst_port = (uint16_t)(((uint16_t)ip[ihl + 2U] << 8) | ip[ihl + 3U]);
    for (i = 0U; i < NET_WIRE_BINDINGS; i++) {
        if (!g_bind[i].bound) continue;
        if (g_bind[i].local_port != dst_port || g_bind[i].remote_port != src_port) continue;
        if (!ip_eq(ip + 16U, g_bind[i].local_ip) || !ip_eq(ip + 12U, g_bind[i].remote_ip)) continue;
        if (tcp_offset) *tcp_offset = (uint16_t)(NET_ETHERNET_HEADER_SIZE + ihl);
        if (tcp_length) *tcp_length = (uint16_t)(ip_length - ihl);
        return (int)i;
    }
    return NET_WIRE_DEMUX_DROP;
}

void net_wire_note_refused(void) {
    g_stat.refused++;
}

void net_wire_fill_status(os_net_wire_status_t* out, int32_t worker_pid) {
    if (!out) return;
    *out = g_stat;
    out->bound = net_wire_bound_count();
    out->worker_pid = worker_pid;
}

/* ---------------------------------------------------------------- driver */

static int wire_tx_segment(const net_wire_ctx_t* ctx, int socket_id, const uint8_t* segment,
                           uint16_t length) {
    int status = ne2k_tcp_segment(ctx->device, ctx->io, ctx->cache, ctx->tx, ctx->capacity,
                                  g_bind[socket_id].local_ip, g_bind[socket_id].remote_ip,
                                  segment, length);
    if (status == 0) g_stat.frames_tx++;
    return status;
}

static int wire_tx_ack(const net_wire_ctx_t* ctx, int socket_id, uint32_t sequence,
                       uint32_t acknowledgment) {
    uint8_t segment[NET_TCP_HEADER_SIZE];
    int built = net_tcp_build_ack(segment, sizeof(segment), g_bind[socket_id].local_port,
                                  g_bind[socket_id].remote_port, sequence, acknowledgment);
    if (built < 0) return -1;
    return wire_tx_segment(ctx, socket_id, segment, (uint16_t)built);
}

static void wire_handle_arp(const net_wire_ctx_t* ctx, uint16_t length, int kind) {
    net_arp_packet_t arp;
    int built;
    if (net_arp_parse(ctx->rx, length, &arp) != 0) return;
    if (kind == NET_WIRE_DEMUX_ARP_REPLY) {
        (void)net_arp_cache_put(ctx->cache, arp.sender_ipv4, arp.sender_mac);
        return;
    }
    /* Learn the requester, answer for our bound address. */
    (void)net_arp_cache_put(ctx->cache, arp.sender_ipv4, arp.sender_mac);
    built = net_arp_build_reply(ctx->tx, ctx->capacity, &arp, ctx->device->mac, arp.target_ipv4);
    if (built > 0 && ne2k_tx_submit(ctx->device, ctx->io, ctx->tx, (uint16_t)built) == 0) {
        g_stat.frames_tx++;
        g_stat.arp_replies++;
    }
}

static void wire_handle_tcp(const net_wire_ctx_t* ctx, int s, uint16_t offset, uint16_t length,
                            wire_observe_t* obs) {
    net_tcp_view_t view;
    net_tcp_connection_t conn;
    uint8_t state = 0U;
    int mine = obs && obs->target == s;
    if (net_tcp_parse(ctx->rx + offset, length, &view) != 0) { g_stat.dropped++; return; }
    if (net_socket_get_state(s, &state) != 0) { g_stat.dropped++; return; }
    g_stat.demuxed++;
    if (view.flags & NET_TCP_FLAG_RST) {
        if (mine) obs->reset = 1U;
        return;
    }
    if (state == NET_TCP_STATE_SYN_SENT) {
        if ((view.flags & (NET_TCP_FLAG_SYN | NET_TCP_FLAG_ACK)) !=
                (NET_TCP_FLAG_SYN | NET_TCP_FLAG_ACK) ||
            net_socket_accept_syn_ack(s, &view) != 0)
            return;
        if (net_socket_connection_snapshot(s, &conn) == 0)
            (void)wire_tx_ack(ctx, s, conn.local_sequence, conn.remote_sequence);
        if (mine) obs->established = 1U;
        return;
    }
    if (state == NET_TCP_STATE_ESTABLISHED) {
        if (net_socket_feed(s, ctx->rx + offset, length) != 0) return; /* dup / out of order */
        if (net_socket_connection_snapshot(s, &conn) != 0) return;
        if (mine && (view.flags & NET_TCP_FLAG_ACK) && view.acknowledgment == obs->want_ack)
            obs->acked = 1U;
        if (view.flags & NET_TCP_FLAG_FIN) {
            /* Peer closed first: consume its FIN and move to CLOSE_WAIT. */
            conn.remote_sequence++;
            conn.state = NET_TCP_STATE_CLOSE_WAIT;
            (void)net_socket_connection_restore(s, &conn);
            g_stat.peer_fins++;
            if (mine) obs->fin = 1U;
        }
        if (view.payload_length > 0U || (view.flags & NET_TCP_FLAG_FIN)) {
            (void)wire_tx_ack(ctx, s, conn.local_sequence, conn.remote_sequence);
            if (mine && view.payload_length > 0U) obs->data = 1U;
        }
        return;
    }
    if (state == NET_TCP_STATE_FIN_WAIT_1 || state == NET_TCP_STATE_FIN_WAIT_2 ||
        state == NET_TCP_STATE_LAST_ACK) {
        if (net_socket_connection_snapshot(s, &conn) != 0) return;
        if ((view.flags & NET_TCP_FLAG_ACK) && view.acknowledgment == conn.local_sequence && mine)
            obs->acked = 1U;
        if (view.flags & NET_TCP_FLAG_FIN) {
            g_stat.peer_fins++;
            (void)wire_tx_ack(ctx, s, conn.local_sequence,
                              view.sequence + view.payload_length + 1U);
            if (mine) obs->fin = 1U;
        }
    }
}

/* 1 = nothing received, 0 = one frame consumed. */
static int wire_poll_once(const net_wire_ctx_t* ctx, wire_observe_t* obs) {
    uint16_t length = 0U, offset = 0U, tcp_length = 0U;
    int kind;
    if (ne2k_rx_poll(ctx->device, ctx->io, ctx->rx, ctx->capacity, &length) != 0) return 1;
    g_stat.frames_rx++;
    kind = net_wire_demux(ctx->rx, length, &offset, &tcp_length);
    if (kind >= 0) wire_handle_tcp(ctx, kind, offset, tcp_length, obs);
    else if (kind == NET_WIRE_DEMUX_ARP_REQUEST || kind == NET_WIRE_DEMUX_ARP_REPLY)
        wire_handle_arp(ctx, length, kind);
    else g_stat.dropped++;
    return 0;
}

static int wire_ctx_ok(const net_wire_ctx_t* ctx) {
    return ctx && ctx->device && ctx->io && ctx->cache && ctx->tx && ctx->rx &&
           ctx->capacity >= NET_ETHERNET_HEADER_SIZE + 20U + NET_TCP_HEADER_SIZE + OS_NET_WIRE_MAX_IO &&
           ctx->device->mac_valid;
}

static int wire_resolve(const net_wire_ctx_t* ctx, int s, uint16_t attempts) {
    uint8_t mac[6];
    uint16_t i;
    int built;
    for (i = 0U; i < attempts; i++) {
        if (net_arp_cache_lookup(ctx->cache, g_bind[s].remote_ip, mac) == 0) return 0;
        if ((i % 50U) == 0U) {
            built = net_arp_build_request(ctx->tx, ctx->capacity, ctx->device->mac,
                                          g_bind[s].local_ip, g_bind[s].remote_ip);
            if (built > 0 && ne2k_tx_submit(ctx->device, ctx->io, ctx->tx, (uint16_t)built) == 0) {
                g_stat.frames_tx++;
                g_stat.arp_tx++;
            }
        }
        if (wire_poll_once(ctx, 0) != 0) wire_pause();
    }
    return net_arp_cache_lookup(ctx->cache, g_bind[s].remote_ip, mac) == 0 ? 0 : -1;
}

int net_wire_connect(const net_wire_ctx_t* ctx, const os_net_wire_connect_t* request) {
    wire_observe_t obs;
    uint16_t attempts, i, syn_length = 0U;
    uint8_t syn[NET_TCP_HEADER_SIZE + 8U];
    int s, status;
    if (!wire_ctx_ok(ctx)) return OS_NET_WIRE_UNAVAILABLE;
    if (!request || request->local_port == 0U || request->remote_port == 0U ||
        ip_zero(request->local_ip) || ip_zero(request->remote_ip))
        return OS_SOCKET_BAD_ARGUMENT;
    attempts = clamp_attempts(request->attempts);
    s = net_socket_open(request->local_port, request->remote_port,
                        request->local_sequence ? request->local_sequence : 0x51A70000U);
    if (s < 0) return map_socket_error(s);
    if (net_wire_bind(s, request->local_ip, request->remote_ip, request->local_port,
                      request->remote_port) != 0) {
        (void)net_socket_close(s);
        return OS_SOCKET_BAD_ARGUMENT;
    }
    if (wire_resolve(ctx, s, attempts) != 0) {
        (void)net_wire_unbind(s);
        (void)net_socket_close(s);
        return OS_NET_WIRE_TIMEOUT;
    }
    obs.target = s; obs.established = 0U; obs.data = 0U; obs.acked = 0U; obs.fin = 0U;
    obs.reset = 0U; obs.want_ack = 0U;
    status = net_socket_build_syn(s, syn, sizeof(syn), &syn_length);
    if (status != 0) {
        (void)net_wire_unbind(s);
        (void)net_socket_close(s);
        return map_socket_error(status);
    }
    for (i = 0U; i < attempts && !obs.established && !obs.reset; i++) {
        /* SYN, retried every 100 idle rounds (no retransmit timer yet). */
        if ((i % 100U) == 0U) (void)wire_tx_segment(ctx, s, syn, syn_length);
        if (wire_poll_once(ctx, &obs) != 0) wire_pause();
    }
    if (!obs.established) {
        (void)net_wire_unbind(s);
        (void)net_socket_close(s);
        return obs.reset ? OS_SOCKET_PROTOCOL : OS_NET_WIRE_TIMEOUT;
    }
    g_stat.connects++;
    return s;
}

int net_wire_send(const net_wire_ctx_t* ctx, int socket_id, const uint8_t* data,
                  uint16_t length, uint8_t* segment_out, uint16_t segment_capacity,
                  uint16_t* segment_length, uint16_t attempts) {
    wire_observe_t obs;
    net_tcp_connection_t snapshot;
    uint16_t built = 0U, i, n;
    int status;
    if (!net_wire_is_bound(socket_id)) return OS_NET_WIRE_NOT_BOUND;
    if (!wire_ctx_ok(ctx)) return OS_NET_WIRE_UNAVAILABLE;
    if (!data || length == 0U) return OS_SOCKET_BAD_ARGUMENT;
    if (length > OS_NET_WIRE_MAX_IO) return OS_SOCKET_BUFFER_SMALL;
    for (i = 0U; i < length; i++) g_payload[i] = data[i];
    if (net_socket_connection_snapshot(socket_id, &snapshot) != 0) return OS_SOCKET_NOT_OPEN;
    status = net_socket_send_limit(socket_id, g_payload, length, g_segment, sizeof(g_segment),
                                   &built, 0U);
    if (status != 0) return map_socket_error(status);
    if (wire_tx_segment(ctx, socket_id, g_segment, built) != 0) {
        (void)net_socket_connection_restore(socket_id, &snapshot);
        return OS_NET_WIRE_UNAVAILABLE;
    }
    g_stat.sends++;
    if (segment_out && segment_length) {
        n = built < segment_capacity ? built : segment_capacity;
        for (i = 0U; i < n; i++) segment_out[i] = g_segment[i];
        *segment_length = n;
    }
    obs.target = socket_id; obs.established = 1U; obs.data = 0U; obs.acked = 0U; obs.fin = 0U;
    obs.reset = 0U; obs.want_ack = snapshot.local_sequence + length;
    attempts = clamp_attempts(attempts);
    for (i = 0U; i < attempts && !obs.acked && !obs.reset; i++)
        if (wire_poll_once(ctx, &obs) != 0) wire_pause();
    if (obs.reset) return OS_SOCKET_PROTOCOL;
    return obs.acked ? 0 : OS_NET_WIRE_TIMEOUT;
}

int net_wire_recv(const net_wire_ctx_t* ctx, int socket_id, uint8_t* buffer,
                  uint16_t capacity, uint16_t* out_length, uint16_t attempts) {
    wire_observe_t obs;
    uint16_t i, got = 0U;
    uint8_t state = 0U;
    int status;
    if (!net_wire_is_bound(socket_id)) return OS_NET_WIRE_NOT_BOUND;
    if (!wire_ctx_ok(ctx)) return OS_NET_WIRE_UNAVAILABLE;
    if (!buffer || !out_length) return OS_SOCKET_BAD_ARGUMENT;
    *out_length = 0U;
    status = net_socket_receive(socket_id, buffer, capacity, &got);
    if (status != 0) return map_socket_error(status);
    if (got == 0U) {
        obs.target = socket_id; obs.established = 1U; obs.data = 0U; obs.acked = 0U;
        obs.fin = 0U; obs.reset = 0U; obs.want_ack = 0xFFFFFFFFU;
        attempts = clamp_attempts(attempts);
        for (i = 0U; i < attempts && !obs.data && !obs.fin && !obs.reset; i++) {
            if (net_socket_get_state(socket_id, &state) == 0 && state != NET_TCP_STATE_ESTABLISHED)
                break;
            if (wire_poll_once(ctx, &obs) != 0) wire_pause();
        }
        status = net_socket_receive(socket_id, buffer, capacity, &got);
        if (status != 0) return map_socket_error(status);
        if (got == 0U && obs.reset) return OS_SOCKET_PROTOCOL;
        if (got == 0U && !obs.fin) return OS_NET_WIRE_TIMEOUT;
    }
    if (got > 0U) g_stat.recvs++;
    *out_length = got;
    return 0;
}

int net_wire_close(const net_wire_ctx_t* ctx, int socket_id, uint16_t attempts) {
    wire_observe_t obs;
    net_tcp_connection_t conn;
    uint8_t segment[NET_TCP_HEADER_SIZE];
    uint8_t state = 0U;
    uint16_t i;
    int built;
    if (!net_wire_is_bound(socket_id)) return OS_NET_WIRE_NOT_BOUND;
    obs.target = socket_id; obs.established = 1U; obs.data = 0U; obs.acked = 0U; obs.fin = 0U;
    obs.reset = 0U; obs.want_ack = 0U;
    if (wire_ctx_ok(ctx) && net_socket_get_state(socket_id, &state) == 0) {
        if (state == NET_TCP_STATE_ESTABLISHED) {
            if (ne2k_socket_fin(ctx->device, ctx->io, ctx->cache, ctx->tx, ctx->capacity,
                                g_bind[socket_id].local_ip, g_bind[socket_id].remote_ip,
                                socket_id) == 0) {
                g_stat.frames_tx++;
                g_stat.closes++;
                attempts = clamp_attempts(attempts);
                for (i = 0U; i < attempts && !obs.fin && !obs.reset; i++)
                    if (wire_poll_once(ctx, &obs) != 0) wire_pause();
            }
        } else if (state == NET_TCP_STATE_CLOSE_WAIT &&
                   net_socket_connection_snapshot(socket_id, &conn) == 0) {
            /* Peer FIN already consumed: our FIN is the last one. */
            built = net_tcp_build_fin_ack(segment, sizeof(segment), conn.local_port,
                                          conn.remote_port, conn.local_sequence,
                                          conn.remote_sequence);
            if (built > 0 && wire_tx_segment(ctx, socket_id, segment, (uint16_t)built) == 0)
                g_stat.closes++;
        }
    }
    (void)net_wire_unbind(socket_id);
    (void)net_socket_close(socket_id);
    return 0;
}
