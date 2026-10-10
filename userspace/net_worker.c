/* userspace/net_worker.c - net-driver worker (Tranche 5).
 * Registers the service name "net-driver" and runs the network syscalls the
 * kernel relays to it over IPC. Once it owns the NE2000 (Tranche 5 suite)
 * the whole stack runs here at CPL 3 (Tranche 5 pile): socket registry,
 * TCP, ARP/IPv4 framing and demux, and the DHCP/DNS/TLS/HTTP LLM client.
 */

#include "os_syscalls.h"
#include "ne2k.h"
#include "net_stack_exec.h"
#include "net_llm_client.h"

static char log_line[OS_NET_NIC_LOG_MAX];
static uint32_t log_length;

static void log_flush(void) {
    int rc;
    uint32_t i;
    if (log_length == 0U) return;
    asm volatile("int $0x80" : "=a"(rc) : "a"(SYS_NET_NIC), "b"(OS_NET_NIC_LOG), "c"(log_line), "d"(log_length));
    if (rc != 0)
        for (i = 0U; i < log_length; i++) asm volatile("int $0x80" : : "a"(SYS_PUTC), "b"(log_line[i]));
    log_length = 0U;
}

/* Lines go out in one syscall: a preempted worker no longer splits them. */
static void putc(char value) {
    log_line[log_length++] = value;
    if (value == '\n' || log_length == sizeof(log_line)) log_flush();
}

static void puts(const char* text) {
    uint32_t index = 0U;
    while (text[index] != '\0') putc(text[index++]);
}

static int ipc_receive(os_ipc_message_t* message) {
    int result;
    asm volatile("int $0x80" : "=a"(result) : "a"(SYS_IPC_RECV), "b"(message));
    return result;
}

static int ipc_send(int target_pid, const os_ipc_payload_t* payload) {
    int result;
    asm volatile("int $0x80" : "=a"(result) : "a"(SYS_IPC_SEND), "b"(target_pid), "c"(payload));
    return result;
}

static int service_register(const char* name) {
    int result;
    asm volatile("int $0x80" : "=a"(result) : "a"(SYS_SERVICE_REGISTER), "b"(name));
    return result;
}

static int relay_reply(const os_net_relay_reply_t* reply) {
    int result;
    asm volatile("int $0x80" : "=a"(result) : "a"(SYS_NET_RELAY_REPLY), "b"(reply));
    return result;
}

static void yield(void) {
    asm volatile("int $0x80" : : "a"(SYS_YIELD));
}

static int net_call1(uint32_t number, uint32_t a) {
    int result;
    asm volatile("int $0x80" : "=a"(result) : "a"(number), "b"(a));
    return result;
}

static int net_call2(uint32_t number, uint32_t a, uint32_t b) {
    int result;
    asm volatile("int $0x80" : "=a"(result) : "a"(number), "b"(a), "c"(b));
    return result;
}

/* Tranche 5 positive proof: once registered as net-driver, this PID still
 * reaches the gated socket and peer syscalls (the NE2000 driver itself stays
 * in Ring 0; the worker is the only task allowed to drive it). */
static int net_worker_gate_self_check(void) {
    int socket_id = net_call2(SYS_SOCKET_LISTEN, 7101U, 1U);
    /* Kernel built with NET_RING0_FALLBACK=0: the Ring 0 socket and peer
     * stacks are refused to every task, this worker included. */
    if (socket_id == OS_NET_WORKER_REQUIRED &&
        net_call1(SYS_PEER_LISTEN, 0U) == OS_NET_WORKER_REQUIRED) return 2;
    if (socket_id < 0) return 0;
    if (net_call1(SYS_SOCKET_CLOSE, (uint32_t)socket_id) != 0) return 0;
    if (net_call1(SYS_PEER_LISTEN, 0U) == OS_NET_WORKER_REQUIRED) return 0;
    return 1;
}

/* ========================================================================
 * Tranche 5 suite: the NE2000 driven from Ring 3.
 *
 * After OS_NET_NIC_CLAIM the kernel opens 0x300-0x31F for this PID in the
 * TSS I/O bitmap and stops touching the card; this task runs the NE2000
 * hardware core (kernel/ne2k_hw.c, linked here) with plain in/out at CPL 3,
 * enables the card's RX/TX interrupts and acks them itself (the kernel only
 * counts IRQ3 for us). A wire op started by SYS_NET_WIRE_* / relayed
 * SYS_SOCKET_CONNECT returns OS_NET_WIRE_PENDING: the kernel TCP/ARP engine
 * then runs one round per OS_NET_NIC_PUMP, frames in and out through here.
 * ======================================================================== */
static ne2k_device_t nic;
static ne2k_io_t nic_io;
static int nic_owned;
/* 1 once this worker answers relayed calls from its own Ring 3 stack (with
 * the NE2000, or loopback-only without a card on a strict kernel). */
static int stack_live;
static uint16_t nic_base;
static uint8_t nic_rx[OS_NET_NIC_FRAME_MAX];
static uint8_t nic_tx[OS_NET_NIC_PUMP_TX_MAX * OS_NET_NIC_FRAME_MAX];
static uint32_t nic_tx_ok, nic_tx_failed, nic_rx_frames, nic_irq, nic_pumps;
static net_stack_t stack;
static uint8_t stack_tx[KERNEL_LLM_FRAME_CAPACITY];
static uint8_t stack_rx[KERNEL_LLM_FRAME_CAPACITY];
static uint8_t bulk_in[OS_NET_RELAY_BULK_MAX];
static uint8_t bulk_out[OS_NET_RELAY_BULK_MAX];

static inline uint8_t port_inb(uint16_t port) {
    uint8_t v; asm volatile("inb %1, %0" : "=a"(v) : "Nd"(port)); return v;
}
static inline void port_outb(uint16_t port, uint8_t v) {
    asm volatile("outb %0, %1" : : "a"(v), "Nd"(port));
}
static uint8_t r3_inb(void* context, uint16_t port) { (void)context; return port_inb(port); }
static void r3_outb(void* context, uint16_t port, uint8_t value) { (void)context; port_outb(port, value); }

static void put_uint(uint32_t value) {
    char digits[11];
    int n = 0;
    if (value == 0U) { putc('0'); return; }
    while (value > 0U && n < 10) { digits[n++] = (char)('0' + value % 10U); value /= 10U; }
    while (n > 0) putc(digits[--n]);
}

static void put_int(int value) {
    if (value < 0) { putc('-'); put_uint((uint32_t)(-value)); }
    else put_uint((uint32_t)value);
}

static void put_hex8(uint8_t v) {
    const char* h = "0123456789abcdef";
    putc(h[v >> 4]); putc(h[v & 15U]);
}

/* Acknowledge the interrupt causes we serviced (RX/TX/errors/overflow),
 * never RDC (remote DMA complete, owned by ne2k_tx_submit). Dropping the
 * INT line lets the next event raise a fresh IRQ3 edge. */
static void nic_ack(void) {
    uint8_t isr;
    port_outb(nic_base, 0x22U);             /* page 0, started, no DMA */
    isr = port_inb((uint16_t)(nic_base + 0x07U));
    if (isr & 0x1FU) port_outb((uint16_t)(nic_base + 0x07U), (uint8_t)(isr & 0x1FU));
}

static int net_call3(uint32_t number, uint32_t a, uint32_t b, uint32_t c);

static int nic_claim(void) {
    os_net_nic_info_t info;
    uint32_t i;
    int same = 1, deferred = 1, rc;
    rc = net_call2(SYS_NET_NIC, OS_NET_NIC_CLAIM, (uint32_t)&info);
    if (rc != 0) {
        puts("net-driver nic claim "); put_int(rc); putc('\n');
        return -1;
    }
    nic_io.context = 0;
    nic_io.inb = r3_inb;
    nic_io.outb = r3_outb;
    nic_base = info.base_port;
    /* The probe, the reset (prepare) and the PROM MAC read are ours. A
     * strict kernel never touched the card (info.mac all zero) and learns
     * the result through OS_NET_NIC_REPORT. */
    for (i = 0; i < 6U; i++) if (info.mac[i] != 0U) deferred = 0;
    if (ne2k_probe(&nic, info.base_port, &nic_io) != 0 || ne2k_prepare(&nic, &nic_io) != 0 ||
        ne2k_read_mac(&nic, &nic_io) != 0 || ne2k_configure_rings(&nic, &nic_io) != 0) {
        if (deferred) {
            (void)net_call3(SYS_NET_NIC, OS_NET_NIC_REPORT, 0U, 0U);
            puts("net-driver nic ring3 probe: no NE2000\n");
            return -2;
        }
        puts("net-driver nic ring3 init failed\n");
        return -1;
    }
    if (deferred) {
        rc = net_call3(SYS_NET_NIC, OS_NET_NIC_REPORT, 1U, (uint32_t)nic.mac);
        if (rc != 0) {
            puts("net-driver nic report rc "); put_int(rc); putc('\n');
            return -1;
        }
    }
    for (i = 0; i < 6U; i++) if (nic.mac[i] != info.mac[i]) same = 0;
    port_outb(nic_base, 0x22U);
    port_outb((uint16_t)(nic_base + 0x07U), 0xFFU);  /* clear stale causes */
    port_outb((uint16_t)(nic_base + 0x0FU), 0x03U);  /* IMR: PRX | PTX -> IRQ3 */
    nic_owned = 1;
    puts("net-driver nic ring3 ok base "); put_uint(nic_base);
    puts(" irq "); put_uint(info.irq);
    puts(" mac ");
    for (i = 0; i < 6U; i++) { if (i) putc(':'); put_hex8(nic.mac[i]); }
    puts(deferred ? " prom-match ring3\n" : same ? " prom-match 1\n" : " prom-match 0\n");
    return 0;
}

/* Idle round: wait for the next timer tick like the Ring 0 engine does
 * (wire_pause), without yielding. The op then completes inside this task's
 * slice, as it did when the kernel busy-polled the card itself; yielding here
 * would hand one round per shell turn and blow the 5 s relay deadline. */
static void nic_pause(void) {
    int t = net_call1(SYS_TICKS, 0U);
    uint32_t spins = 0U;
    while (net_call1(SYS_TICKS, 0U) == t && spins < 2000000U) {
        asm volatile("pause" : : : "memory");
        spins++;
    }
}

/* Stack mode: pumps/out/in are the Ring 3 engine rounds and the frames it
 * framed / decoded; the kernel pump counters are printed apart (must stay 0). */
static void nic_report(void) {
    os_net_nic_status_t st;
    if (net_call2(SYS_NET_NIC, OS_NET_NIC_STATUS, (uint32_t)&st) != 0) return;
    puts("net-driver nic owner "); put_int(st.owner_pid);
    puts(" tx "); put_uint(nic_tx_ok);
    puts(" txfail "); put_uint(nic_tx_failed);
    puts(" rx "); put_uint(nic_rx_frames);
    puts(" irq "); put_uint(nic_irq);
    puts(" kirq "); put_uint(st.irq_forwarded);
    puts(" pumps "); put_uint(nic_owned ? stack.report.rounds : st.pumps);
    puts(" out "); put_uint(nic_owned ? stack.report.frames_built : st.frames_out);
    puts(" in "); put_uint(nic_owned ? stack.report.frames_parsed : st.frames_in);
    puts(" refused "); put_uint(st.kernel_refused);
    puts(" gated "); put_uint(st.kernel_gated);
    puts(" end\n");
    if (!nic_owned) return;
    puts("net-driver stack ring3 sockets "); put_uint(stack.report.socket_ops);
    puts(" wire "); put_uint(stack.report.wire_ops);
    puts(" llm "); put_uint(stack.report.llm_ops);
    puts(" rounds "); put_uint(stack.report.rounds);
    puts(" framed "); put_uint(stack.report.frames_built);
    puts(" decoded "); put_uint(stack.report.frames_parsed);
    puts(" kernel-pumps "); put_uint(st.pumps);
    puts(" kernel-out "); put_uint(st.frames_out);
    puts(" kernel-in "); put_uint(st.frames_in);
    puts(" end\n");
}

/* Drives the pending wire op to completion on this task's NIC. */
static int nic_pump_loop(void) {
    os_net_nic_pump_t p;
    uint16_t length = 0U, i;
    int irq;
    p.mode = OS_NET_NIC_PUMP_FETCH;
    p.rx = 0; p.rx_length = 0U; p.tx_sent = 0U; p.tx_failed = 0U;
    p.tx_count = 0U; p.tx = nic_tx; p.result = 0; p.done = 0U;
    for (;;) {
        if (net_call2(SYS_NET_NIC, OS_NET_NIC_PUMP, (uint32_t)&p) != 0) return OS_NET_WIRE_UNAVAILABLE;
        nic_pumps++;
        p.tx_sent = 0U; p.tx_failed = 0U;
        for (i = 0U; i < p.tx_count; i++) {
            if (ne2k_tx_submit(&nic, &nic_io, nic_tx + (uint32_t)i * OS_NET_NIC_FRAME_MAX,
                               p.tx_length[i]) == 0) { p.tx_sent++; nic_tx_ok++; }
            else { p.tx_failed++; nic_tx_failed++; }
        }
        nic_ack();
        if (p.done) return p.result;
        if (ne2k_rx_poll(&nic, &nic_io, nic_rx, sizeof(nic_rx), &length) == 0) {
            p.mode = OS_NET_NIC_PUMP_FRAME; p.rx = nic_rx; p.rx_length = length;
            nic_rx_frames++;
        } else {
            nic_pause();
            p.mode = OS_NET_NIC_PUMP_IDLE; p.rx = 0; p.rx_length = 0U;
        }
        nic_ack();
        irq = net_call2(SYS_NET_NIC, OS_NET_NIC_IRQ, 0U);
        if (irq > 0) nic_irq += (uint32_t)irq;
    }
}

static int wire_done(int rc) {
    if (rc != OS_NET_WIRE_PENDING) return rc;
    rc = nic_pump_loop();
    nic_report();
    return rc;
}

static int net_call3(uint32_t number, uint32_t a, uint32_t b, uint32_t c) {
    int result;
    asm volatile("int $0x80" : "=a"(result) : "a"(number), "b"(a), "c"(b), "d"(c));
    return result;
}

static int net_call4(uint32_t number, uint32_t a, uint32_t b, uint32_t c, uint32_t d) {
    int result;
    asm volatile("int $0x80" : "=a"(result) : "a"(number), "b"(a), "c"(b), "d"(c), "S"(d));
    return result;
}


static void copy_bytes(uint8_t* dst, const uint8_t* src, uint32_t n) {
    uint32_t i;
    for (i = 0U; i < n; i++) dst[i] = src[i];
}

/* Tranche 5 slice 3 counters, printed in the relay log lines. */
static uint32_t wire_calls;

/* Tranche 5 slice 3: for a wire-bound socket, SEND/RECEIVE/CLOSE go through
 * the worker-only SYS_NET_WIRE_* path (real NE2000 frames); any other socket
 * answers OS_NET_WIRE_NOT_BOUND and keeps the slice 2 in-registry path. */
static int32_t wire_io(uint32_t op, int32_t socket_id, const uint8_t* data, uint16_t length,
                       uint8_t* rx, uint16_t cap, uint32_t* out_length) {
    os_net_wire_io_t io;
    uint16_t got = 0U;
    int32_t rc;
    io.socket_id = socket_id; io.data = data; io.length = length;
    io.rx = rx; io.rx_capacity = cap; io.rx_length = &got; io.attempts = 0U;
    rc = wire_done(net_call1(op, (uint32_t)&io));
    if (rc != OS_NET_WIRE_NOT_BOUND) { wire_calls++; *out_length = got; }
    return rc;
}

/* Tranche 5 slice 2: run one relayed socket syscall through this worker's
 * own (privileged) path. The TCP/socket registry itself stays in Ring 0. */
static int32_t relay_execute(const os_net_relay_request_t* req, os_net_relay_reply_t* reply) {
    static uint8_t in[OS_NET_RELAY_MAX_IN];
    uint16_t out_length = 0U;
    uint32_t in_length = req->in_length <= OS_NET_RELAY_MAX_IN ? req->in_length : 0U;
    uint32_t cap = req->out_capacity <= OS_NET_RELAY_MAX_OUT ? req->out_capacity : OS_NET_RELAY_MAX_OUT;
    int32_t rc;
    copy_bytes(in, req->in, in_length);
    reply->out_length = 0U;
    switch (req->op) {
        case SYS_SOCKET_OPEN:
            return net_call3(SYS_SOCKET_OPEN, req->arg0, req->arg1, req->arg2);
        case SYS_SOCKET_LISTEN:
            return net_call2(SYS_SOCKET_LISTEN, req->arg0, req->arg1);
        case SYS_SOCKET_CLOSE:
            rc = wire_done(net_call1(SYS_NET_WIRE_CLOSE, req->arg0));
            if (rc != OS_NET_WIRE_NOT_BOUND) { wire_calls++; return rc; }
            return net_call1(SYS_SOCKET_CLOSE, req->arg0);
        case SYS_SOCKET_CONNECT: {
            os_net_wire_connect_t c;
            if (in_length != sizeof(os_socket_connect_request_t)) return OS_SOCKET_BAD_ARGUMENT;
            copy_bytes((uint8_t*)&c, in, sizeof(c));
            wire_calls++;
            return wire_done(net_call1(SYS_NET_WIRE_CONNECT, (uint32_t)&c));
        }
        case SYS_SOCKET_ACCEPT_SYN_ACK: {
            os_socket_syn_ack_t view;
            if (in_length != sizeof(view)) return OS_SOCKET_BAD_ARGUMENT;
            copy_bytes((uint8_t*)&view, in, sizeof(view));
            return net_call2(SYS_SOCKET_ACCEPT_SYN_ACK, req->arg0, (uint32_t)&view);
        }
        case SYS_SOCKET_ACCEPT_SYN:
        case SYS_SOCKET_ACCEPT_ACK: {
            os_socket_passive_view_t view;
            if (in_length != sizeof(view)) return OS_SOCKET_BAD_ARGUMENT;
            copy_bytes((uint8_t*)&view, in, sizeof(view));
            return net_call2(req->op, req->arg0, (uint32_t)&view);
        }
        case SYS_SOCKET_BUILD_SYN_ACK:
            rc = net_call4(SYS_SOCKET_BUILD_SYN_ACK, req->arg0, (uint32_t)reply->out, cap,
                           (uint32_t)&out_length);
            if (rc == 0) reply->out_length = out_length;
            return rc;
        case SYS_SOCKET_SEND: {
            os_socket_send_request_t r;
            rc = wire_io(SYS_NET_WIRE_SEND, (int32_t)req->arg0, in, (uint16_t)in_length,
                         reply->out, (uint16_t)cap, &reply->out_length);
            if (rc != OS_NET_WIRE_NOT_BOUND) return rc;
            r.socket_id = (int32_t)req->arg0; r.payload = in; r.length = (uint16_t)in_length;
            r.segment = reply->out; r.capacity = (uint16_t)cap; r.out_length = &out_length;
            rc = net_call1(SYS_SOCKET_SEND, (uint32_t)&r);
            if (rc == 0) reply->out_length = out_length;
            return rc;
        }
        case SYS_SOCKET_FEED: {
            os_socket_feed_request_t r;
            r.socket_id = (int32_t)req->arg0; r.segment = in; r.length = (uint16_t)in_length;
            return net_call1(SYS_SOCKET_FEED, (uint32_t)&r);
        }
        case SYS_SOCKET_RECEIVE: {
            os_socket_receive_request_t r;
            rc = wire_io(SYS_NET_WIRE_RECV, (int32_t)req->arg0, 0, 0U, reply->out,
                         (uint16_t)cap, &reply->out_length);
            if (rc != OS_NET_WIRE_NOT_BOUND) return rc;
            r.socket_id = (int32_t)req->arg0; r.buffer = reply->out; r.capacity = (uint16_t)cap;
            r.out_length = &out_length;
            rc = net_call1(SYS_SOCKET_RECEIVE, (uint32_t)&r);
            if (rc == 0) reply->out_length = out_length;
            return rc;
        }
        case SYS_PEER_LISTEN:
        case SYS_PEER_ACCEPT:
        case SYS_PEER_TLS_POLL:
            return net_call1(req->op, (uint32_t)in);
        default:
            return OS_SOCKET_BAD_ARGUMENT;
    }
}

/* ---- Tranche 5 pile: Ring 3 stack glue ---- */
uint32_t timer_get_ticks(void) { return (uint32_t)net_call1(SYS_TICKS, 0U); }

int net_llm_client_utc(rtc_io_t* io, char* out, uint16_t capacity) {
    (void)io;
    if (capacity < 16U) return -1;
    return net_call2(SYS_NET_NIC, OS_NET_NIC_UTC, (uint32_t)out) == 0 ? 0 : -1;
}

static int r3_emit(void* context, const uint8_t* frame, uint16_t length) {
    (void)context;
    if (ne2k_tx_submit(&nic, &nic_io, frame, length) == 0) { nic_tx_ok++; return 0; }
    nic_tx_failed++;
    return -1;
}

static int r3_poll(void* user, uint8_t* frame, uint16_t capacity, uint16_t* length) {
    (void)user;
    if (ne2k_rx_poll(&nic, &nic_io, frame, capacity, length) != 0) return -1;
    nic_rx_frames++;
    return 0;
}

static void r3_idle(void* user) { (void)user; nic_pause(); }

static void r3_round(void* user) {
    int irq;
    (void)user;
    nic_ack();
    irq = net_call2(SYS_NET_NIC, OS_NET_NIC_IRQ, 0U);
    if (irq > 0) nic_irq += (uint32_t)irq;
}

static const net_stack_llm_ops_t r3_llm = {
    kernel_llm_acquire_start, kernel_llm_poll_tls, kernel_llm_request, kernel_llm_poll_text,
    kernel_llm_poll_sse, kernel_llm_reset_for_request, kernel_llm_close,
    kernel_llm_configure_openai, kernel_llm_session_status
};

static void stack_publish(void) {
    stack.report.llm_status = kernel_llm_session_status();
    stack.report.wire.worker_pid = 0;
    net_wire_fill_status(&stack.report.wire, 0);
    (void)net_call2(SYS_NET_NIC, OS_NET_NIC_PUBLISH, (uint32_t)&stack.report);
}

static void stack_start(void) {
    net_stack_init(&stack);
    net_socket_reset_all();
    net_wire_reset();
    net_llm_client_reset();
    net_llm_client_bind(&nic, &nic_io, 1);
    stack.emit = r3_emit;
    stack.poll = r3_poll;
    stack.idle = r3_idle;
    stack.after_round = r3_round;
    stack.device = &nic;
    stack.cache = &boot_llm_arp_cache;
    stack.tx = stack_tx;
    stack.rx = stack_rx;
    stack.capacity = (uint16_t)sizeof(stack_tx);
    stack.llm = &r3_llm;
    stack_publish();
    stack_live = 1;
    puts("net-driver stack ring3 ready arp ipv4 tcp tls llm-status ");
    put_uint(stack.report.llm_status);
    putc('\n');
}

/* Strict kernel (NET_RING0_FALLBACK=0) and no NE2000: the kernel socket
 * registry is refused to everyone, so this worker serves the socket calls
 * (loopback segments between local sockets) from its own registry. No
 * emit/device: wire ops (connect, bound send/recv) answer
 * OS_NET_WIRE_UNAVAILABLE; the LLM client is never bound, so LLM and peer
 * calls answer UNAVAILABLE ("NE2000 absent") from Ring 3. */
static void stack_start_loopback(void) {
    net_stack_init(&stack);
    net_socket_reset_all();
    net_wire_reset();
    net_llm_client_reset();
    stack.llm = &r3_llm;
    stack_live = 1;
    puts("net-driver stack ring3 ready loopback-only (no NE2000)\n");
}

static int32_t stack_execute(const os_net_relay_request_t* req, os_net_relay_reply_t* reply) {
    uint32_t in_length = 0U, out_length = 0U;
    uint16_t small = 0U;
    int32_t rc;
    int got;
    if (req->op >= SYS_LLM_ACQUIRE_START && req->op <= SYS_LLM_OPENAI_CREDENTIAL && req->arg0) {
        if (req->arg0 > sizeof(bulk_in)) return OS_LLM_REQUEST_BAD_REQUEST;
        asm volatile("int $0x80" : "=a"(got) : "a"(SYS_NET_RELAY_BULK), "b"(OS_NET_RELAY_BULK_FETCH),
                     "c"(req->job_id), "d"(bulk_in), "S"(req->arg0));
        if (got < 0) return got;
        in_length = (uint32_t)got;
    }
    rc = net_stack_exec(&stack, req, bulk_in, in_length, reply->out, &small, bulk_out, &out_length);
    reply->out_length = small;
    if (out_length) {
        asm volatile("int $0x80" : "=a"(got) : "a"(SYS_NET_RELAY_BULK), "b"(OS_NET_RELAY_BULK_PUT),
                     "c"(req->job_id), "d"(bulk_out), "S"(out_length));
        if (got != 0) return got;
    }
    return rc;
}

void main(void) {
    static os_ipc_message_t message;
    static os_net_relay_request_t req;
    static os_net_relay_reply_t reply;
    uint32_t relayed = 0U;
    uint32_t ignored = 0U;
    uint32_t last_wire_ops = 0U;
    int rc;

    if (service_register("net-driver") != 0) {
        puts("net-driver register failed\n");
        for (;;) yield();
    }
    puts("net-driver ready\n");
    rc = net_worker_gate_self_check();
    if (rc == 2) puts("net-driver kernel socket stack absent, ring3 only\n");
    else if (rc) puts("net-driver gated syscalls ok\n");
    else puts("net-driver gated syscalls unexpected\n");
    /* Tranche 5 suite: take the NE2000 over (no-op without a card). On a
     * strict kernel without a card, serve sockets loopback-only. A legacy
     * kernel without a card keeps the relay_execute path (kernel registry). */
    {
        int claim = nic_claim();
        if (claim == 0) stack_start();
        else if (rc == 2) stack_start_loopback();
    }

    for (;;) {
        if (ipc_receive(&message) != 0) {
            if (nic_owned) (void)kernel_llm_dhcp_maintenance(timer_get_ticks());
            yield();
            continue;
        }
        /* Only the kernel (sender 0) forwards relay requests. */
        if (message.sender_pid != 0 ||
            (message.type != OS_IPC_NET_RELAY_REQUEST && message.type != OS_IPC_NET_PEER_RELAY_REQUEST) ||
            message.size != sizeof(req)) {
            ignored++;
            puts("net-driver received IPC message\n");
            continue;
        }
        copy_bytes((uint8_t*)&req, message.data, sizeof(req));
        reply.job_id = req.job_id;
        if (stack_live) {
            reply.result = stack_execute(&req, &reply);
            wire_calls = stack.report.wire_ops;
        } else {
            reply.result = relay_execute(&req, &reply);
        }
        rc = relay_reply(&reply);
        relayed++;
        puts("net-driver relay op ");
        put_uint(req.op);
        puts(" rc ");
        put_int(reply.result);
        puts(" reply ");
        put_int(rc);
        puts(" total ");
        put_uint(relayed);
        if (wire_calls) { puts(" wire "); put_uint(wire_calls); }
        putc('\n');
        if (nic_owned) {
            stack_publish();
            if (req.op == SYS_SOCKET_CONNECT || (stack.report.wire_ops != last_wire_ops)) nic_report();
            last_wire_ops = stack.report.wire_ops;
        }
    }
    (void)ignored;
    (void)ipc_send;
}
