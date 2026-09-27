#include "../../framework/unity.h"
#include "../../../kernel/net_wire.h"
#include "../../../kernel/net_relay.h"
#include <string.h>

/* Tranche 5 slice 3: bookkeeping and receive demux of the worker-only wire
 * path (pure parts; the NE2000-driving parts run in QEMU, make qemu-net-wire). */


static const uint8_t k_local[4] = {10, 32, 0, 15};
static const uint8_t k_remote[4] = {10, 32, 0, 2};

static uint16_t tcp_frame(uint8_t* f, const uint8_t* src, const uint8_t* dst, uint16_t sport,
                          uint16_t dport, uint16_t payload) {
    uint16_t ip_len = (uint16_t)(20U + 20U + payload);
    memset(f, 0, 128);
    f[12] = 0x08; f[13] = 0x00;
    f[14] = 0x45; f[16] = (uint8_t)(ip_len >> 8); f[17] = (uint8_t)ip_len; f[23] = 6;
    memcpy(f + 26, src, 4); memcpy(f + 30, dst, 4);
    f[34] = (uint8_t)(sport >> 8); f[35] = (uint8_t)sport;
    f[36] = (uint8_t)(dport >> 8); f[37] = (uint8_t)dport;
    f[46] = 0x50;
    return (uint16_t)(14U + ip_len);
}

static uint16_t arp_frame(uint8_t* f, uint16_t opcode, const uint8_t* target) {
    memset(f, 0, 64);
    f[12] = 0x08; f[13] = 0x06;
    f[14] = 0; f[15] = 1; f[16] = 0x08; f[17] = 0; f[18] = 6; f[19] = 4;
    f[20] = 0; f[21] = (uint8_t)opcode;
    f[22] = 0x52; f[23] = 0x54; f[27] = 2;
    memcpy(f + 28, k_remote, 4);
    memcpy(f + 38, target, 4);
    return 42U;
}

static void test_bind_rules(void) {
    net_wire_reset();
    const uint8_t zero[4] = {0, 0, 0, 0};
    TEST_ASSERT_EQUAL(0, net_wire_bind(1, k_local, k_remote, 40007, 7));
    TEST_ASSERT_TRUE(net_wire_is_bound(1));
    TEST_ASSERT_FALSE(net_wire_is_bound(0));
    TEST_ASSERT_EQUAL(1, (int)net_wire_bound_count());
    /* Same 4-tuple on another socket is refused: the demux stays unambiguous. */
    TEST_ASSERT_EQUAL(-2, net_wire_bind(2, k_local, k_remote, 40007, 7));
    TEST_ASSERT_EQUAL(0, net_wire_bind(2, k_local, k_remote, 40008, 7));
    TEST_ASSERT_EQUAL(-1, net_wire_bind(-1, k_local, k_remote, 1, 7));
    TEST_ASSERT_EQUAL(-1, net_wire_bind(NET_WIRE_BINDINGS, k_local, k_remote, 1, 7));
    TEST_ASSERT_EQUAL(-1, net_wire_bind(3, zero, k_remote, 1, 7));
    TEST_ASSERT_EQUAL(-1, net_wire_bind(3, k_local, k_remote, 0, 7));
    TEST_ASSERT_EQUAL(0, net_wire_unbind(1));
    TEST_ASSERT_EQUAL(-1, net_wire_unbind(1));
    TEST_ASSERT_EQUAL(1, (int)net_wire_bound_count());
}

static void test_demux(void) {
    net_wire_reset();
    uint8_t f[128];
    uint16_t off = 0, len = 0, n;
    const uint8_t other[4] = {10, 32, 0, 16};
    TEST_ASSERT_EQUAL(0, net_wire_bind(2, k_local, k_remote, 40007, 7));
    n = tcp_frame(f, k_remote, k_local, 7, 40007, 5);
    TEST_ASSERT_EQUAL(2, net_wire_demux(f, n, &off, &len));
    TEST_ASSERT_EQUAL(34, off);
    TEST_ASSERT_EQUAL(25, len);
    /* Trailing Ethernet padding / FCS is ignored (IPv4 total length wins). */
    TEST_ASSERT_EQUAL(2, net_wire_demux(f, (uint16_t)(n + 4U), &off, &len));
    TEST_ASSERT_EQUAL(25, len);
    /* Wrong port, wrong peer, wrong destination, truncated: dropped. */
    n = tcp_frame(f, k_remote, k_local, 7, 40008, 0);
    TEST_ASSERT_EQUAL(NET_WIRE_DEMUX_DROP, net_wire_demux(f, n, &off, &len));
    n = tcp_frame(f, other, k_local, 7, 40007, 0);
    TEST_ASSERT_EQUAL(NET_WIRE_DEMUX_DROP, net_wire_demux(f, n, &off, &len));
    n = tcp_frame(f, k_remote, other, 7, 40007, 0);
    TEST_ASSERT_EQUAL(NET_WIRE_DEMUX_DROP, net_wire_demux(f, n, &off, &len));
    n = tcp_frame(f, k_remote, k_local, 7, 40007, 10);
    TEST_ASSERT_EQUAL(NET_WIRE_DEMUX_DROP, net_wire_demux(f, (uint16_t)(n - 1U), &off, &len));
    /* Fragment: not reassembled here. */
    n = tcp_frame(f, k_remote, k_local, 7, 40007, 0);
    f[20] = 0x20;
    TEST_ASSERT_EQUAL(NET_WIRE_DEMUX_DROP, net_wire_demux(f, n, &off, &len));
    /* UDP: dropped. */
    n = tcp_frame(f, k_remote, k_local, 7, 40007, 0);
    f[23] = 17;
    TEST_ASSERT_EQUAL(NET_WIRE_DEMUX_DROP, net_wire_demux(f, n, &off, &len));
    /* Unbound socket: nothing matches any more. */
    TEST_ASSERT_EQUAL(0, net_wire_unbind(2));
    n = tcp_frame(f, k_remote, k_local, 7, 40007, 0);
    TEST_ASSERT_EQUAL(NET_WIRE_DEMUX_DROP, net_wire_demux(f, n, &off, &len));
}

static void test_demux_arp(void) {
    net_wire_reset();
    uint8_t f[64];
    const uint8_t other[4] = {10, 32, 0, 16};
    uint16_t n = arp_frame(f, NET_ARP_OPCODE_REQUEST, k_local);
    /* No binding for 10.32.0.15 yet: the wire path does not answer. */
    TEST_ASSERT_EQUAL(NET_WIRE_DEMUX_DROP, net_wire_demux(f, n, 0, 0));
    TEST_ASSERT_EQUAL(0, net_wire_bind(0, k_local, k_remote, 40007, 7));
    TEST_ASSERT_EQUAL(NET_WIRE_DEMUX_ARP_REQUEST, net_wire_demux(f, n, 0, 0));
    n = arp_frame(f, NET_ARP_OPCODE_REPLY, k_local);
    TEST_ASSERT_EQUAL(NET_WIRE_DEMUX_ARP_REPLY, net_wire_demux(f, n, 0, 0));
    n = arp_frame(f, NET_ARP_OPCODE_REQUEST, other);
    TEST_ASSERT_EQUAL(NET_WIRE_DEMUX_DROP, net_wire_demux(f, n, 0, 0));
}

static void test_status_and_refused(void) {
    net_wire_reset();
    os_net_wire_status_t st;
    net_wire_note_refused();
    net_wire_note_refused();
    TEST_ASSERT_EQUAL(0, net_wire_bind(3, k_local, k_remote, 40007, 7));
    net_wire_fill_status(&st, 42);
    TEST_ASSERT_EQUAL(2, (int)st.refused);
    TEST_ASSERT_EQUAL(1, (int)st.bound);
    TEST_ASSERT_EQUAL(42, st.worker_pid);
    TEST_ASSERT_EQUAL(0, (int)st.frames_tx);
    net_wire_reset();
    net_wire_fill_status(&st, 0);
    TEST_ASSERT_EQUAL(0, (int)st.refused);
    TEST_ASSERT_EQUAL(0, (int)st.bound);
}

static void test_ops_without_device(void) {
    net_wire_reset();
    os_net_wire_connect_t c;
    uint8_t buf[8];
    uint16_t n = 0;
    memset(&c, 0, sizeof(c));
    /* Unbound sockets answer NOT_BOUND so the worker keeps the slice 2 path. */
    TEST_ASSERT_EQUAL(OS_NET_WIRE_NOT_BOUND, net_wire_send(0, 1, buf, 1, 0, 0, 0, 0));
    TEST_ASSERT_EQUAL(OS_NET_WIRE_NOT_BOUND, net_wire_recv(0, 1, buf, sizeof(buf), &n, 0));
    TEST_ASSERT_EQUAL(OS_NET_WIRE_NOT_BOUND, net_wire_close(0, 1, 0));
    TEST_ASSERT_EQUAL(OS_NET_WIRE_UNAVAILABLE, net_wire_connect(0, &c));
}

static void test_abi(void) {
    net_wire_reset();
    /* The worker copies the relayed connect request into the wire request. */
    TEST_ASSERT_EQUAL((int)sizeof(os_net_wire_connect_t), (int)sizeof(os_socket_connect_request_t));
    TEST_ASSERT_TRUE(sizeof(os_socket_connect_request_t) <= OS_NET_RELAY_MAX_IN);
    TEST_ASSERT_TRUE(net_relay_supported(SYS_SOCKET_CONNECT));
    TEST_ASSERT_FALSE(net_relay_supported(SYS_NET_WIRE_CONNECT));
    TEST_ASSERT_FALSE(net_relay_supported(SYS_NET_WIRE_SEND));
    TEST_ASSERT_FALSE(net_relay_supported(SYS_NET_WIRE_STATUS));
    TEST_ASSERT_TRUE(SYS_SOCKET_CONNECT < MAX_SYSCALLS);
    TEST_ASSERT_TRUE(OS_NET_WIRE_TIMEOUT != OS_NET_RELAY_TIMEOUT);
    TEST_ASSERT_TRUE(OS_NET_WIRE_UNAVAILABLE != OS_PEER_UNAVAILABLE);
    TEST_ASSERT_TRUE(OS_NET_WIRE_NOT_BOUND != OS_SOCKET_NOT_OPEN);
    TEST_ASSERT_TRUE(OS_NET_WIRE_NOT_BOUND != OS_LLM_ACQUIRE_DHCP_ACK_TIMEOUT);
}

/* ---- Tranche 5 suite: resumable engine driven through an emit sink, the
 * way SYS_NET_NIC pumps drive it for the Ring 3 NE2000 owner. ---- */
static uint8_t g_sink[4][1536];
static uint16_t g_sink_len[4];
static int g_sink_count;
static int g_sink_fail;

static int sink_emit(void* context, const uint8_t* frame, uint16_t length) {
    (void)context;
    if (g_sink_fail || g_sink_count >= 4) return -1;
    memcpy(g_sink[g_sink_count], frame, length);
    g_sink_len[g_sink_count++] = length;
    return 0;
}

static ne2k_device_t g_dev;
static net_arp_cache_t g_cache;
static uint8_t g_tx[1536], g_rx[1536];

static void emit_ctx(net_wire_ctx_t* ctx) {
    memset(&g_dev, 0, sizeof(g_dev));
    g_dev.base_port = 0x300U;
    g_dev.mac[0] = 0x52; g_dev.mac[1] = 0x54; g_dev.mac[5] = 0x56;
    g_dev.mac_valid = 1U;
    (void)net_arp_cache_init(&g_cache);
    memset(ctx, 0, sizeof(*ctx));
    ctx->device = &g_dev; ctx->io = 0; ctx->cache = &g_cache;
    ctx->tx = g_tx; ctx->rx = g_rx; ctx->capacity = sizeof(g_tx);
    ctx->emit = sink_emit; ctx->emit_context = 0;
    g_sink_count = 0; g_sink_fail = 0;
}

static void connect_req(os_net_wire_connect_t* c, uint16_t attempts) {
    memset(c, 0, sizeof(*c));
    memcpy(c->local_ip, k_local, 4); memcpy(c->remote_ip, k_remote, 4);
    c->local_port = 40007; c->remote_port = 7; c->attempts = attempts;
}

static void test_engine_emit_arp_then_syn(void) {
    net_wire_ctx_t ctx;
    os_net_wire_connect_t c;
    uint16_t n;
    net_wire_reset();
    emit_ctx(&ctx);
    connect_req(&c, 200);
    /* Begin: the op keeps running, the ARP request went to the sink (the
     * NIC owner transmits it), not to any port. */
    TEST_ASSERT_EQUAL(1, net_wire_op_connect(&ctx, &c));
    TEST_ASSERT_TRUE(net_wire_op_active());
    TEST_ASSERT_EQUAL(NET_WIRE_OP_CONNECT, (int)net_wire_op_kind());
    TEST_ASSERT_EQUAL(1, g_sink_count);
    TEST_ASSERT_EQUAL(0x08, g_sink[0][12]);
    TEST_ASSERT_EQUAL(0x06, g_sink[0][13]);
    /* A second op cannot start while one runs (one op at a time). */
    TEST_ASSERT_EQUAL(0, net_wire_op_close(&ctx, 0, 0));
    TEST_ASSERT_EQUAL(OS_NET_WIRE_UNAVAILABLE, net_wire_op_result());
    TEST_ASSERT_TRUE(net_wire_op_active());
    /* Idle round: no new frame (ARP retried every 50 rounds only). */
    TEST_ASSERT_EQUAL(1, net_wire_op_step(&ctx, 0, 0));
    TEST_ASSERT_EQUAL(1, g_sink_count);
    /* The worker feeds the ARP reply it polled: the engine learns the peer
     * and emits the SYN as an IPv4/TCP frame to the learned MAC. */
    n = arp_frame(g_rx, NET_ARP_OPCODE_REPLY, k_local);
    TEST_ASSERT_EQUAL(1, net_wire_op_step(&ctx, 1, n));
    TEST_ASSERT_EQUAL(2, g_sink_count);
    TEST_ASSERT_EQUAL(0x08, g_sink[1][12]);
    TEST_ASSERT_EQUAL(0x00, g_sink[1][13]);
    TEST_ASSERT_EQUAL(0x52, g_sink[1][0]);
    TEST_ASSERT_EQUAL(0x02, g_sink[1][5]);
    TEST_ASSERT_EQUAL(6, g_sink[1][23]);
    TEST_ASSERT_EQUAL(NET_TCP_FLAG_SYN, g_sink[1][47] & 0x3F);
    TEST_ASSERT_TRUE(net_wire_is_bound(0));
    /* Worker lost mid-op: cancel frees the socket binding. */
    net_wire_op_cancel();
    TEST_ASSERT_FALSE(net_wire_op_active());
    TEST_ASSERT_EQUAL(OS_NET_WIRE_UNAVAILABLE, net_wire_op_result());
    TEST_ASSERT_FALSE(net_wire_is_bound(0));
}

static void test_engine_emit_timeout_and_failed_sink(void) {
    net_wire_ctx_t ctx;
    os_net_wire_connect_t c;
    int rounds = 0;
    net_wire_reset();
    emit_ctx(&ctx);
    connect_req(&c, 3);
    /* Nobody answers ARP: the op ends with OS_NET_WIRE_TIMEOUT after the
     * requested number of rounds, and the socket is dropped. */
    TEST_ASSERT_EQUAL(1, net_wire_op_connect(&ctx, &c));
    while (net_wire_op_step(&ctx, 0, 0) && rounds < 10) rounds++;
    TEST_ASSERT_EQUAL(2, rounds);
    TEST_ASSERT_FALSE(net_wire_op_active());
    TEST_ASSERT_EQUAL(OS_NET_WIRE_TIMEOUT, net_wire_op_result());
    TEST_ASSERT_FALSE(net_wire_is_bound(0));
    /* A refusing sink transmits nothing and counts nothing. */
    net_wire_reset();
    emit_ctx(&ctx);
    g_sink_fail = 1;
    connect_req(&c, 200);
    TEST_ASSERT_EQUAL(1, net_wire_op_connect(&ctx, &c));
    TEST_ASSERT_EQUAL(0, g_sink_count);
    {
        os_net_wire_status_t st;
        net_wire_fill_status(&st, 0);
        TEST_ASSERT_EQUAL(0, (int)st.frames_tx);
    }
    net_wire_op_cancel();
    /* No sink and no io: the engine refuses to start. */
    emit_ctx(&ctx);
    ctx.emit = 0;
    TEST_ASSERT_EQUAL(0, net_wire_op_connect(&ctx, &c));
    TEST_ASSERT_EQUAL(OS_NET_WIRE_UNAVAILABLE, net_wire_op_result());
    /* The blocking Ring 0 entry points refuse a sink context. */
    emit_ctx(&ctx);
    TEST_ASSERT_EQUAL(OS_NET_WIRE_UNAVAILABLE, net_wire_connect(&ctx, &c));
}

int main(void) {
    unity_init();
    RUN_TEST(test_bind_rules);
    RUN_TEST(test_demux);
    RUN_TEST(test_demux_arp);
    RUN_TEST(test_status_and_refused);
    RUN_TEST(test_ops_without_device);
    RUN_TEST(test_abi);
    RUN_TEST(test_engine_emit_arp_then_syn);
    RUN_TEST(test_engine_emit_timeout_and_failed_sink);
    unity_print_results();
    unity_cleanup();
    return unity_stats.tests_failed == 0 ? 0 : 1;
}
