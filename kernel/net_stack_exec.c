#include "net_stack_exec.h"
#include "net_llm_client.h"

static void stack_copy(void* dst, const void* src, uint32_t n) {
    uint8_t* d = (uint8_t*)dst;
    const uint8_t* s = (const uint8_t*)src;
    while (n--) *d++ = *s++;
}

void net_stack_init(net_stack_t* stack) {
    uint8_t* p = (uint8_t*)stack;
    uint32_t i;
    if (!stack) return;
    for (i = 0U; i < sizeof(*stack); i++) p[i] = 0U;
}

uint32_t net_stack_bulk_in_size(uint32_t op) {
    switch (op) {
        case SYS_LLM_ACQUIRE_START: return (uint32_t)sizeof(os_llm_acquire_start_request_t);
        case SYS_LLM_REQUEST: return (uint32_t)sizeof(os_llm_request_t);
        case SYS_LLM_OPENAI_CREDENTIAL: return (uint32_t)sizeof(os_llm_openai_credential_request_t);
        case SYS_PEER_DATA: return (uint32_t)sizeof(os_peer_data_request_t);
        default: return 0U;
    }
}

uint32_t net_stack_bulk_out_size(uint32_t op) {
    if (op == SYS_PEER_DATA) return (uint32_t)sizeof(os_peer_data_request_t);
    return (op == SYS_LLM_POLL_TEXT || op == SYS_LLM_POLL_SSE) ? (uint32_t)sizeof(os_llm_text_result_t) : 0U;
}

static int stack_emit(void* context, const uint8_t* frame, uint16_t length) {
    net_stack_t* st = (net_stack_t*)context;
    int rc = st->emit(st->emit_context, frame, length);
    if (rc == 0) st->report.frames_built++;
    return rc;
}

static int stack_ctx(net_stack_t* st, net_wire_ctx_t* ctx) {
    if (!st->emit || !st->device || !st->cache || !st->tx || !st->rx || st->capacity == 0U)
        return OS_NET_WIRE_UNAVAILABLE;
    ctx->device = st->device;
    ctx->io = 0;
    ctx->cache = st->cache;
    ctx->tx = st->tx;
    ctx->rx = st->rx;
    ctx->capacity = st->capacity;
    ctx->emit = stack_emit;
    ctx->emit_context = st;
    return 0;
}

static int32_t stack_run(net_stack_t* st, const net_wire_ctx_t* ctx, int running) {
    uint16_t length;
    st->report.wire_ops++;
    while (running) {
        length = 0U;
        st->report.rounds++;
        if (st->poll && st->poll(st->user, ctx->rx, ctx->capacity, &length) == 0 && length > 0U) {
            st->report.frames_parsed++;
            running = net_wire_op_step(ctx, 1, length);
        } else {
            if (st->idle) st->idle(st->user);
            running = net_wire_op_step(ctx, 0, 0U);
        }
        if (st->after_round) st->after_round(st->user);
    }
    return net_wire_op_result();
}

static void stack_view(net_tcp_view_t* v, uint16_t sp, uint16_t dp, uint32_t seq, uint32_t ack, uint8_t flags) {
    v->source_port = sp; v->destination_port = dp; v->sequence = seq;
    v->acknowledgment = ack; v->flags = flags; v->payload = 0; v->payload_length = 0U;
}

static int32_t stack_socket(net_stack_t* st, const os_net_relay_request_t* req,
                            uint8_t* out, uint16_t* out_length) {
    net_wire_ctx_t ctx;
    net_tcp_view_t view;
    int id = (int)req->arg0;
    uint16_t in_length = req->in_length <= OS_NET_RELAY_MAX_IN ? req->in_length : 0U;
    uint16_t cap = req->out_capacity <= OS_NET_RELAY_MAX_OUT ? req->out_capacity : OS_NET_RELAY_MAX_OUT;
    int rc;
    st->report.socket_ops++;
    switch (req->op) {
        case SYS_SOCKET_OPEN:
            return net_socket_open((uint16_t)req->arg0, (uint16_t)req->arg1, req->arg2);
        case SYS_SOCKET_LISTEN:
            return net_socket_listen((uint16_t)req->arg0, req->arg1);
        case SYS_SOCKET_CLOSE:
            if (!net_wire_is_bound(id)) return net_socket_close(id);
            if (stack_ctx(st, &ctx) != 0) return net_wire_close(0, id, 0U);
            return stack_run(st, &ctx, net_wire_op_close(&ctx, id, 0U));
        case SYS_SOCKET_CONNECT: {
            os_socket_connect_request_t u;
            os_net_wire_connect_t c;
            if (in_length != sizeof(u)) return OS_SOCKET_BAD_ARGUMENT;
            stack_copy(&u, req->in, sizeof(u));
            c.local_port = u.local_port; c.remote_port = u.remote_port;
            stack_copy(c.local_ip, u.local_ip, 4U); stack_copy(c.remote_ip, u.remote_ip, 4U);
            c.local_sequence = u.local_sequence; c.attempts = u.attempts;
            rc = stack_ctx(st, &ctx);
            if (rc != 0) return rc;
            return stack_run(st, &ctx, net_wire_op_connect(&ctx, &c));
        }
        case SYS_SOCKET_ACCEPT_SYN_ACK: {
            os_socket_syn_ack_t v;
            if (in_length != sizeof(v)) return OS_SOCKET_BAD_ARGUMENT;
            stack_copy(&v, req->in, sizeof(v));
            stack_view(&view, v.source_port, v.destination_port, v.sequence, v.acknowledgment, v.flags);
            return net_socket_accept_syn_ack(id, &view);
        }
        case SYS_SOCKET_ACCEPT_SYN:
        case SYS_SOCKET_ACCEPT_ACK: {
            os_socket_passive_view_t v;
            if (in_length != sizeof(v)) return OS_SOCKET_BAD_ARGUMENT;
            stack_copy(&v, req->in, sizeof(v));
            stack_view(&view, v.source_port, v.destination_port, v.sequence, v.acknowledgment, v.flags);
            return req->op == SYS_SOCKET_ACCEPT_SYN ? net_socket_accept_syn(id, &view)
                                                    : net_socket_accept_ack(id, &view);
        }
        case SYS_SOCKET_BUILD_SYN_ACK:
            rc = net_socket_build_syn_ack(id, out, cap, out_length);
            if (rc != 0) *out_length = 0U;
            return rc;
        case SYS_SOCKET_SEND:
            if (net_wire_is_bound(id)) {
                rc = stack_ctx(st, &ctx);
                if (rc != 0) return rc;
                return stack_run(st, &ctx, net_wire_op_send(&ctx, id, req->in, in_length, out, cap,
                                                            out_length, 0U));
            }
            rc = net_socket_send(id, req->in, in_length, out, cap, out_length);
            if (rc != 0) *out_length = 0U;
            return rc;
        case SYS_SOCKET_FEED:
            return net_socket_feed(id, req->in, in_length);
        case SYS_SOCKET_RECEIVE:
            if (net_wire_is_bound(id)) {
                rc = stack_ctx(st, &ctx);
                if (rc != 0) return rc;
                return stack_run(st, &ctx, net_wire_op_recv(&ctx, id, out, cap, out_length, 0U));
            }
            rc = net_socket_receive(id, out, cap, out_length);
            if (rc != 0) *out_length = 0U;
            return rc;
        default:
            st->report.socket_ops--;
            return OS_SOCKET_BAD_ARGUMENT;
    }
}

static int32_t stack_peer(const os_net_relay_request_t* req, uint8_t* out, uint16_t* out_length) {
    uint16_t in_length = req->in_length <= OS_NET_RELAY_MAX_IN ? req->in_length : 0U;
    switch (req->op) {
        case SYS_PEER_LISTEN: {
            os_peer_listen_request_t r;
            if (in_length != sizeof(r)) return OS_PEER_BAD_REQUEST;
            stack_copy(&r, req->in, sizeof(r));
            return kernel_peer_listen(&r);
        }
        case SYS_PEER_ACCEPT: {
            os_peer_accept_request_t r;
            if (in_length != sizeof(r)) return OS_PEER_BAD_REQUEST;
            stack_copy(&r, req->in, sizeof(r));
            return kernel_peer_accept(&r);
        }
        case SYS_PEER_TLS_POLL: {
            os_peer_tls_poll_request_t r;
            if (in_length == sizeof(r)) {
                stack_copy(&r, req->in, sizeof(r));
                return kernel_peer_tls_poll(&r);
            }
            return kernel_peer_tls_poll(0);
        }
        default:
            return OS_PEER_BAD_REQUEST;
    }
}

static int32_t stack_llm(net_stack_t* st, uint32_t op, const uint8_t* in, uint32_t in_length,
                         uint8_t* bulk_out, uint32_t* bulk_out_length) {
    static os_llm_acquire_start_request_t acquire;
    static os_llm_request_t request;
    static os_llm_openai_credential_request_t credential;
    static os_llm_text_result_t text;
    const net_stack_llm_ops_t* llm = st->llm;
    uint32_t want_in = net_stack_bulk_in_size(op);
    int32_t rc;
    if (!llm) return OS_LLM_TLS_UNCONFIGURED;
    if (want_in && (in_length != want_in || !in)) return OS_LLM_REQUEST_BAD_REQUEST;
    st->report.llm_ops++;
    switch (op) {
        case SYS_LLM_ACQUIRE_START:
            stack_copy(&acquire, in, sizeof(acquire));
            rc = llm->acquire_start(&acquire);
            break;
        case SYS_LLM_POLL_TLS: rc = llm->poll_tls(); break;
        case SYS_LLM_REQUEST:
            stack_copy(&request, in, sizeof(request));
            rc = llm->request(&request);
            break;
        case SYS_LLM_POLL_TEXT:
        case SYS_LLM_POLL_SSE:
            {
                uint32_t i;
                uint8_t* p = (uint8_t*)&text;
                for (i = 0U; i < sizeof(text); i++) p[i] = 0U;
            }
            rc = op == SYS_LLM_POLL_TEXT ? llm->poll_text(&text) : llm->poll_sse(&text);
            if (bulk_out && bulk_out_length) {
                stack_copy(bulk_out, &text, sizeof(text));
                *bulk_out_length = (uint32_t)sizeof(text);
            }
            break;
        case SYS_LLM_RESET_FOR_REQUEST: rc = llm->reset_for_request(); break;
        case SYS_LLM_CLOSE: rc = llm->close(); break;
        case SYS_LLM_OPENAI_CREDENTIAL:
            stack_copy(&credential, in, sizeof(credential));
            rc = llm->configure_openai(&credential);
            {
                uint32_t i;
                uint8_t* p = (uint8_t*)&credential;
                for (i = 0U; i < sizeof(credential); i++) p[i] = 0U; /* bearer lives in the client only */
            }
            break;
        default:
            st->report.llm_ops--;
            return OS_LLM_REQUEST_BAD_REQUEST;
    }
    if (llm->session_status) st->report.llm_status = llm->session_status();
    return rc;
}

int32_t net_stack_exec(net_stack_t* stack, const os_net_relay_request_t* request,
                       const uint8_t* bulk_in, uint32_t bulk_in_length,
                       uint8_t* out, uint16_t* out_length,
                       uint8_t* bulk_out, uint32_t* bulk_out_length) {
    uint16_t local_length = 0U;
    if (out_length) *out_length = 0U;
    if (bulk_out_length) *bulk_out_length = 0U;
    if (!stack || !request) return OS_SOCKET_BAD_ARGUMENT;
    if (request->op >= SYS_LLM_ACQUIRE_START && request->op <= SYS_LLM_OPENAI_CREDENTIAL)
        return stack_llm(stack, request->op, bulk_in, bulk_in_length, bulk_out, bulk_out_length);
    if (request->op == SYS_PEER_DATA) {
        static os_peer_data_request_t r;
        int32_t rc;
        if (bulk_in_length != sizeof(r) || !bulk_in) return OS_PEER_BAD_REQUEST;
        stack_copy(&r, bulk_in, sizeof(r));
        rc = kernel_peer_data(&r);
        if (bulk_out && bulk_out_length) {
            stack_copy(bulk_out, &r, sizeof(r));
            *bulk_out_length = (uint32_t)sizeof(r);
        }
        return rc;
    }
    if (request->op >= SYS_PEER_LISTEN && request->op <= SYS_PEER_TLS_POLL)
        return stack_peer(request, out, out_length);
    if (!out) return OS_SOCKET_BAD_ARGUMENT;
    {
        int32_t rc = stack_socket(stack, request, out, &local_length);
        if (out_length) *out_length = local_length;
        return rc;
    }
}
