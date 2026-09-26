/* userspace/net_worker.c - net-driver worker (Tranche 5).
 * Registers the service name "net-driver". The NE2000 driver and the
 * TCP/socket registry stay in Ring 0; this worker is the only task allowed
 * to call the network syscalls while registered, and since slice 2 it runs
 * the socket syscalls relayed to it by the kernel over IPC.
 */

#include "os_syscalls.h"

static void putc(char value) {
    asm volatile("int $0x80" : : "a"(SYS_PUTC), "b"(value));
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
    if (socket_id < 0) return 0;
    if (net_call1(SYS_SOCKET_CLOSE, (uint32_t)socket_id) != 0) return 0;
    if (net_call1(SYS_PEER_LISTEN, 0U) == OS_NET_WORKER_REQUIRED) return 0;
    return 1;
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

static void copy_bytes(uint8_t* dst, const uint8_t* src, uint32_t n) {
    uint32_t i;
    for (i = 0U; i < n; i++) dst[i] = src[i];
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
            return net_call1(SYS_SOCKET_CLOSE, req->arg0);
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
            r.socket_id = (int32_t)req->arg0; r.buffer = reply->out; r.capacity = (uint16_t)cap;
            r.out_length = &out_length;
            rc = net_call1(SYS_SOCKET_RECEIVE, (uint32_t)&r);
            if (rc == 0) reply->out_length = out_length;
            return rc;
        }
        default:
            return OS_SOCKET_BAD_ARGUMENT;
    }
}

void main(void) {
    static os_ipc_message_t message;
    static os_net_relay_request_t req;
    static os_net_relay_reply_t reply;
    uint32_t relayed = 0U;
    uint32_t ignored = 0U;
    int rc;

    if (service_register("net-driver") != 0) {
        puts("net-driver register failed\n");
        for (;;) yield();
    }
    puts("net-driver ready\n");
    if (net_worker_gate_self_check()) puts("net-driver gated syscalls ok\n");
    else puts("net-driver gated syscalls unexpected\n");

    for (;;) {
        if (ipc_receive(&message) != 0) {
            yield();
            continue;
        }
        /* Only the kernel (sender 0) forwards relay requests. */
        if (message.sender_pid != 0 || message.type != OS_IPC_NET_RELAY_REQUEST ||
            message.size != sizeof(req)) {
            ignored++;
            puts("net-driver received IPC message\n");
            continue;
        }
        copy_bytes((uint8_t*)&req, message.data, sizeof(req));
        reply.job_id = req.job_id;
        reply.result = relay_execute(&req, &reply);
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
        putc('\n');
    }
    (void)ignored;
    (void)ipc_send;
}
