/* userspace/shell_grpc.c - Lot 6 (US-067): one gRPC unary call over a real
 * TCP connection between two guests.
 *   grpc-serve PORT [N] [SECONDS]: plain TCP listening slots of the Ring 3
 *     networker web table (SYS_PEER_DATA OS_PEER_WEB_*), reads the HTTP/2
 *     client stream (preface, SETTINGS, HEADERS, DATA), answers with
 *     SETTINGS, SETTINGS ACK, HEADERS, DATA, trailers, then closes.
 *   grpc-call LOCAL_IP REMOTE_IP PORT SERVICE METHOD [PAYLOAD]: client side
 *     with SYS_SOCKET_CONNECT / SEND / RECEIVE / CLOSE (relayed to the
 *     networker, real TCP over the NE2000).
 * Cleartext HTTP/2 (h2c with prior knowledge), one call per connection, no
 * flow control, no TLS. The encoder is userspace/grpcwire.c. */
#include "shell_grpc.h"
#include "grpcwire.h"
#include "../include/os_syscalls.h"

void print_string(const char* s);

static os_peer_data_request_t g_io;

static int sys1(int nr, const void* a) { int r; asm volatile("int $0x80" : "=a"(r) : "a"(nr), "b"(a) : "memory"); return r; }
static unsigned ticks(void) { int t; asm volatile("int $0x80" : "=a"(t) : "a"(SYS_TICKS) : "memory"); return (unsigned)t; }
static void nap(void) { unsigned t0 = ticks(); while (ticks() - t0 < 2U) (void)sys1(SYS_YIELD, 0); }

static int s_eq(const char* a, const char* b) { while (*a && *a == *b) { a++; b++; } return *a == *b; }
static int s_pre(const char* s, const char* p) { while (*p && *s == *p) { s++; p++; } return *p == 0; }
static const char* skip(const char* s) { while (*s == ' ') s++; return s; }
static const char* word(const char* s, char* w, int cap) {
    int n = 0;
    s = skip(s);
    while (*s && *s != ' ') { if (n < cap - 1) w[n++] = *s; s++; }
    w[n] = 0;
    return skip(s);
}
static int parse_ip(const char* s, uint8_t ip[4]) {
    int part = 0, v = -1;
    for (;; s++) {
        if (*s >= '0' && *s <= '9') { v = (v < 0 ? 0 : v) * 10 + (*s - '0'); if (v > 255) return -1; }
        else if ((*s == '.' || !*s) && v >= 0 && part < 4) { ip[part++] = (uint8_t)v; v = -1; if (!*s) break; }
        else return -1;
    }
    return part == 4 ? 0 : -1;
}
static int to_int(const char* s) {
    int v = 0, any = 0;
    while (*s >= '0' && *s <= '9') { v = v * 10 + (*s++ - '0'); any = 1; if (v > 1000000) return -1; }
    return any && !*s ? v : -1;
}
static void print_int(int rc) {
    char n[12], t[12]; int i = 0, k = 0; unsigned v = (unsigned)(rc < 0 ? -rc : rc);
    do { t[i++] = (char)('0' + v % 10U); v /= 10U; } while (v);
    if (rc < 0) n[k++] = '-';
    while (i) n[k++] = t[--i];
    n[k] = 0;
    print_string(n);
}
static void say(const char* a, int v, const char* b) { print_string(a); print_int(v); print_string(b); }

static int handler(const char* path, const char* payload, char* reply, int cap) {
    int k = 0;
    const char* h;
    reply[0] = 0;
    if (s_eq(path, "/echo.EchoService/SayHello")) {
        h = "Hello "; while (*h && k < cap - 1) reply[k++] = *h++;
        while (*payload && k < cap - 1) reply[k++] = *payload++;
        h = " from guest over tcp"; while (*h && k < cap - 1) reply[k++] = *h++;
        reply[k] = 0;
        return 0;
    }
    h = "unknown method"; while (*h && k < cap - 1) reply[k++] = *h++;
    reply[k] = 0;
    return 12;   /* UNIMPLEMENTED */
}

static int web(unsigned op, int slot) { g_io.op = (uint8_t)op; g_io.slot = (uint8_t)slot; return sys1(SYS_PEER_DATA, &g_io); }

#define GS_BUF 768
static unsigned char g_rx[OS_PEER_WEB_SLOTS][GS_BUF];
static int g_rx_len[OS_PEER_WEB_SLOTS];

static int grpc_serve(const char* rest) {
    char a[16], b[16], c[16];
    static unsigned char out[512];
    char path[64];
    int port, want, secs, served = 0, failed = 0, rc, i, k;
    unsigned t0;
    rest = word(rest, a, (int)sizeof(a));
    rest = word(rest, b, (int)sizeof(b));
    rest = word(rest, c, (int)sizeof(c));
    port = to_int(a);
    want = b[0] ? to_int(b) : 1;
    secs = c[0] ? to_int(c) : 120;
    if (port <= 0 || port > 65535 || want <= 0 || want > 16 || secs <= 0 || secs > 600) {
        print_string("grpc-serve error usage: grpc-serve PORT [N] [SECONDS]\n");
        return 1;
    }
    g_io.port = (uint16_t)port; g_io.flags = 0U;
    rc = web(OS_PEER_WEB_OPEN, 0);
    if (rc != 0) { say("grpc-serve error listen rc ", rc, "\n"); return 1; }
    for (i = 0; i < (int)OS_PEER_WEB_SLOTS; i++) g_rx_len[i] = 0;
    say("grpc-serve listening port ", port, " transport=tcp h2c\n");
    t0 = ticks();
    while (served + failed < want && ticks() - t0 < (unsigned)secs * 100U) {
        unsigned char st[OS_PEER_WEB_SLOTS];
        g_io.attempts = 16U;
        rc = web(OS_PEER_WEB_POLL, 0);
        for (i = 0; i < (int)OS_PEER_WEB_SLOTS; i++) st[i] = rc == 0 ? g_io.data[i] : 0U;
        for (i = 0; i < (int)OS_PEER_WEB_SLOTS && served + failed < want; i++) {
            int done = 0, cs = 0, m;
            if (!(st[i] & OS_PEER_SLOT_DATA)) {
                if ((st[i] & (OS_PEER_SLOT_PEER_CLOSED | OS_PEER_SLOT_FAILED)) && g_rx_len[i] > 0) {
                    say("grpc-serve slot ", i, " closed before a full request\n");
                    g_rx_len[i] = 0; failed++;
                    (void)web(OS_PEER_WEB_CLOSE, i);
                }
                continue;
            }
            rc = web(OS_PEER_WEB_RECV, i);
            if (rc != 0) continue;
            for (k = 0; k < g_io.length && g_rx_len[i] < GS_BUF; k++) g_rx[i][g_rx_len[i]++] = g_io.data[k];
            done = gw_stream_complete(g_rx[i], g_rx_len[i], 1);
            if (done == 0 && g_rx_len[i] < GS_BUF) continue;
            m = done == 1 ? gw_serve_stream(g_rx[i], g_rx_len[i], handler, out, (int)sizeof(out), path,
                                            (int)sizeof(path), &cs)
                          : (done < 0 ? done : GW_ERR_LIMIT);
            if (m < 0) {
                say("grpc-serve slot ", i, " bad stream rc ");
                say("", m, "\n");
                failed++;
            } else {
                for (k = 0; k < m; k++) g_io.data[k] = out[k];
                g_io.length = (uint16_t)m;
                rc = web(OS_PEER_WEB_SEND, i);
                say("grpc-serve call slot ", i, " path ");
                print_string(path);
                say(" client_settings ", cs, "");
                say(" request_bytes ", g_rx_len[i], "");
                say(" response_bytes ", m, "");
                say(" send rc ", rc, " transport=tcp\n");
                if (rc == 0) served++; else failed++;
            }
            g_rx_len[i] = 0;
            (void)web(OS_PEER_WEB_CLOSE, i);
        }
        nap();
    }
    (void)web(OS_PEER_WEB_STOP, 0);
    say("grpc-serve ok served ", served, "");
    say(" failed ", failed, "\n");
    return served == want ? 0 : 2;
}

#define GC_PIECE ((int)OS_NET_RELAY_MAX_IN)
static int grpc_call(const char* rest) {
    char lip[20], rip[20], ps[8], svc[48], meth[32], pay[64];
    static unsigned char req[512], seg[640], rx[768];
    os_socket_connect_request_t c;
    os_socket_send_request_t s;
    os_socket_receive_request_t r;
    uint16_t seg_len = 0U, got = 0U;
    int port, n, sid, rc, i, total = 0, rounds, done = 0, hs, gs, frames, ss, acks, segs = 0;
    char msg[160];
    unsigned t0;
    rest = word(rest, lip, (int)sizeof(lip));
    rest = word(rest, rip, (int)sizeof(rip));
    rest = word(rest, ps, (int)sizeof(ps));
    rest = word(rest, svc, (int)sizeof(svc));
    rest = word(rest, meth, (int)sizeof(meth));
    rest = word(rest, pay, (int)sizeof(pay));
    for (i = 0; i < (int)sizeof(c); i++) ((uint8_t*)&c)[i] = 0U;
    port = to_int(ps);
    if (parse_ip(lip, c.local_ip) != 0 || parse_ip(rip, c.remote_ip) != 0 || port <= 0 || port > 65535 ||
        !svc[0] || !meth[0]) {
        print_string("grpc-call error usage: grpc-call LOCAL_IP REMOTE_IP PORT SERVICE METHOD [PAYLOAD]\n");
        return 1;
    }
    n = gw_client_stream(req, (int)sizeof(req), svc, meth, pay);
    if (n < 0) { say("grpc-call error encode ", n, "\n"); return 1; }
    t0 = ticks();
    c.local_port = (uint16_t)(41000U + (t0 & 0x3FFU));
    c.remote_port = (uint16_t)port;
    c.local_sequence = 0x67A00000U ^ (t0 << 8);
    c.attempts = 400U;
    sid = sys1(SYS_SOCKET_CONNECT, &c);
    if (sid < 0) { say("grpc-call error connect rc ", sid, "\n"); return 1; }
    say("grpc-call connected socket ", sid, "");
    say(" local_port ", c.local_port, "\n");
    /* The relayed SYS_SOCKET_SEND carries up to OS_NET_RELAY_MAX_IN bytes:
     * one TCP segment per piece, the server reassembles the stream. */
    for (i = 0; i < n; i += GC_PIECE) {
        int k = n - i > GC_PIECE ? GC_PIECE : n - i;
        s.socket_id = sid; s.payload = req + i; s.length = (uint16_t)k;
        s.segment = seg; s.capacity = sizeof(seg); s.out_length = &seg_len;
        rc = sys1(SYS_SOCKET_SEND, &s);
        if (rc != 0) { say("grpc-call error send rc ", rc, "\n"); (void)sys1(SYS_SOCKET_CLOSE, (void*)sid); return 1; }
        segs++;
    }
    say("grpc-call sent request_bytes ", n, "");
    say(" segments ", segs, "\n");
    for (rounds = 0; rounds < 60 && !done; rounds++) {
        got = 0U;
        r.socket_id = sid; r.buffer = rx + total; r.capacity = (uint16_t)(sizeof(rx) - (unsigned)total);
        r.out_length = &got;
        rc = sys1(SYS_SOCKET_RECEIVE, &r);
        if (rc != 0 && rc != OS_NET_WIRE_TIMEOUT) break;
        total += got;
        done = gw_stream_complete(rx, total, 0);
        if (done < 0 || total >= (int)sizeof(rx)) break;
    }
    (void)sys1(SYS_SOCKET_CLOSE, (void*)sid);
    if (done != 1) { say("grpc-call error response incomplete bytes ", total, ""); say(" rc ", rc, "\n"); return 2; }
    if (gw_parse_stream_response(rx, total, &hs, &gs, msg, (int)sizeof(msg), &frames, &ss, &acks) != GW_OK) {
        say("grpc-call error parse bytes ", total, "\n");
        return 2;
    }
    say("grpc-call ok http ", hs, "");
    say(" grpc-status ", gs, "");
    say(" frames ", frames, "");
    say(" server_settings ", ss, "");
    say(" settings_ack ", acks, "");
    say(" response_bytes ", total, "");
    say(" ms ", (int)((ticks() - t0) * 10U), "");
    print_string(" message ");
    print_string(msg);
    print_string(" transport=tcp\n");
    return gs == 0 ? 0 : 3;
}

int shell_grpc_is(const char* line) {
    return (s_pre(line, "grpc-serve") && (line[10] == ' ' || !line[10])) ||
           (s_pre(line, "grpc-call") && (line[9] == ' ' || !line[9]));
}

int shell_grpc_line(const char* line) {
    char cmd[16];
    const char* rest = word(line, cmd, (int)sizeof(cmd));
    if (s_eq(cmd, "grpc-serve")) return grpc_serve(rest);
    if (s_eq(cmd, "grpc-call")) return grpc_call(rest);
    return 1;
}
