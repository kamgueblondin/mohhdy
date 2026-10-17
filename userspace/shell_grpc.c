/* userspace/shell_grpc.c - Lot 6 (US-067): one gRPC unary call over a real
 * TCP connection between two guests.
 *   grpc-serve PORT [N] [SECONDS]: plain TCP listening slots of the Ring 3
 *     networker web table (SYS_PEER_DATA OS_PEER_WEB_*), reads the HTTP/2
 *     client stream (preface, SETTINGS, HEADERS, DATA), answers with
 *     SETTINGS, SETTINGS ACK, HEADERS, DATA, trailers, then closes.
 *   grpc-call LOCAL_IP REMOTE_IP PORT SERVICE METHOD [PAYLOAD]: client side
 *     with SYS_SOCKET_CONNECT / SEND / RECEIVE / CLOSE (relayed to the
 *     networker, real TCP over the NE2000).
 *   grpc-calls LOCAL_IP REMOTE_IP PORT CALLS WINDOW SERVICE METHOD [PAYLOAD]
 *     (Lot 7): CALLS unary calls pipelined on ONE connection (streams 1, 3,
 *     5, ...) with a stream receive window of WINDOW bytes, so the server
 *     holds its DATA until our WINDOW_UPDATE frames reopen the window.
 *   grpc-tls-serve [CALLS] [SECONDS] / grpc-tls-call CALLS SERVICE METHOD
 *     [PAYLOAD] (Lot 9): the same HTTP/2 engine inside the guest-guest TLS
 *     1.2 session (AES-128-GCM records sealed and opened by the networker,
 *     SYS_PEER_DATA OS_PEER_TLS_APP_*): server on the accepted peer session
 *     (slot 0), client on the ai-acquire/ai-tls-poll session (slot 1).
 * grpc-serve/grpc-call/grpc-calls are cleartext HTTP/2 (h2c with prior
 * knowledge). The connection engine (flow control, several streams) is
 * userspace/grpcwire.c. */
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

static const char* g_over = " from guest over tcp";
static const char* g_tr = " transport=tcp\n";

static int handler(const char* path, const char* payload, char* reply, int cap) {
    int k = 0;
    const char* h;
    reply[0] = 0;
    /* "/mohhdy.echo/hello" (Lot 9): same echo, lower-case name so the
     * QEMU keyboard harnesses can type it */
    if (s_eq(path, "/echo.EchoService/SayHello") || s_eq(path, "/mohhdy.echo/hello")) {
        h = "Hello "; while (*h && k < cap - 1) reply[k++] = *h++;
        while (*payload && k < cap - 1) reply[k++] = *payload++;
        h = g_over; while (*h && k < cap - 1) reply[k++] = *h++;
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
/* Lot 7: one HTTP/2 connection engine per slot (several calls per
 * connection, flow control). */
static gw_conn_t g_conn[OS_PEER_WEB_SLOTS];
static int g_conn_on[OS_PEER_WEB_SLOTS];
static int g_cur_slot;
static gw_conn_t* g_cur_conn;

static int serve_handler(const char* path, const char* payload, char* reply, int cap) {
    say("grpc-serve call slot ", g_cur_slot, "");
    say(" stream ", (int)g_cur_conn->cur_stream, " path ");
    print_string(path);
    say(" client_settings ", g_cur_conn->settings_seen, "");
    say(" peer_window ", g_cur_conn->peer_init_win, g_tr);
    return handler(path, payload, reply, cap);
}

static void conn_end(int i, int* conns, int* wu_s, int* wu_r, int* blocked) {
    if (!g_conn_on[i]) return;
    say("grpc-serve connection slot ", i, "");
    say(" calls ", g_conn[i].calls_done, "");
    say(" window_updates_sent ", g_conn[i].wu_sent, "");
    say(" window_updates_recv ", g_conn[i].wu_recv, "");
    say(" data_frames_sent ", g_conn[i].data_sent, "");
    say(" blocked ", g_conn[i].blocked, "");
    say(" goaway ", g_conn[i].goaway, "\n");
    (*conns)++;
    *wu_s += g_conn[i].wu_sent; *wu_r += g_conn[i].wu_recv; *blocked += g_conn[i].blocked;
    g_conn_on[i] = 0;
    g_rx_len[i] = 0;
}

static int grpc_serve(const char* rest) {
    char a[16], b[16], c[16];
    static unsigned char out[OS_PEER_DATA_MAX];
    int port, want, secs, served = 0, failed = 0, rc, i, k, conns = 0, wu_s = 0, wu_r = 0, blocked = 0;
    unsigned t0, linger = 0;
    rest = word(rest, a, (int)sizeof(a));
    rest = word(rest, b, (int)sizeof(b));
    rest = word(rest, c, (int)sizeof(c));
    port = to_int(a);
    want = b[0] ? to_int(b) : 1;
    secs = c[0] ? to_int(c) : 120;
    if (port <= 0 || port > 65535 || want <= 0 || want > 64 || secs <= 0 || secs > 600) {
        print_string("grpc-serve error usage: grpc-serve PORT [CALLS] [SECONDS]\n");
        return 1;
    }
    g_io.port = (uint16_t)port; g_io.flags = 0U;
    rc = web(OS_PEER_WEB_OPEN, 0);
    if (rc != 0) { say("grpc-serve error listen rc ", rc, "\n"); return 1; }
    for (i = 0; i < (int)OS_PEER_WEB_SLOTS; i++) { g_rx_len[i] = 0; g_conn_on[i] = 0; }
    say("grpc-serve listening port ", port, " transport=tcp h2c\n");
    t0 = ticks();
    for (;;) {
        unsigned now = ticks();
        int open = 0;
        for (i = 0; i < (int)OS_PEER_WEB_SLOTS; i++) open += g_conn_on[i];
        if (now - t0 >= (unsigned)secs * 100U) break;
        /* all calls answered: linger (2 s at most) until the clients send
         * GOAWAY or close, so their last WINDOW_UPDATE frames still land */
        if (served + failed >= want) {
            if (!open) break;
            if (!linger) linger = now;
            else if (now - linger > 200U) break;
        }
        unsigned char st[OS_PEER_WEB_SLOTS];
        g_io.attempts = 16U;
        rc = web(OS_PEER_WEB_POLL, 0);
        for (i = 0; i < (int)OS_PEER_WEB_SLOTS; i++) st[i] = rc == 0 ? g_io.data[i] : 0U;
        for (i = 0; i < (int)OS_PEER_WEB_SLOTS; i++) {
            int used = 0, m, before;
            gw_conn_t* cn = &g_conn[i];
            if (!(st[i] & OS_PEER_SLOT_DATA)) {
                if ((st[i] & (OS_PEER_SLOT_PEER_CLOSED | OS_PEER_SLOT_FAILED)) && g_conn_on[i]) {
                    if (gw_conn_busy(cn)) { say("grpc-serve slot ", i, " closed with calls in flight\n"); failed++; }
                    conn_end(i, &conns, &wu_s, &wu_r, &blocked);
                    (void)web(OS_PEER_WEB_CLOSE, i);
                }
                continue;
            }
            rc = web(OS_PEER_WEB_RECV, i);
            if (rc != 0) continue;
            if (!g_conn_on[i]) { gw_conn_init(cn, 1, GW_WIN_DEFAULT, serve_handler); g_conn_on[i] = 1; g_rx_len[i] = 0; }
            for (k = 0; k < g_io.length && g_rx_len[i] < GS_BUF; k++) g_rx[i][g_rx_len[i]++] = g_io.data[k];
            g_cur_slot = i; g_cur_conn = cn;
            before = cn->calls_done;
            m = gw_conn_feed(cn, g_rx[i], g_rx_len[i], &used, out, (int)sizeof(out));
            if (m < 0 || (used == 0 && g_rx_len[i] >= GS_BUF)) {
                say("grpc-serve slot ", i, " bad stream rc ");
                say("", m < 0 ? m : GW_ERR_LIMIT, "\n");
                failed++;
                conn_end(i, &conns, &wu_s, &wu_r, &blocked);
                (void)web(OS_PEER_WEB_CLOSE, i);
                continue;
            }
            for (k = used; k < g_rx_len[i]; k++) g_rx[i][k - used] = g_rx[i][k];
            g_rx_len[i] -= used;
            if (m > 0) {
                for (k = 0; k < m; k++) g_io.data[k] = out[k];
                g_io.length = (uint16_t)m;
                rc = web(OS_PEER_WEB_SEND, i);
                if (rc != 0) {
                    say("grpc-serve slot ", i, " send rc ");
                    say("", rc, "\n");
                    failed += gw_conn_busy(cn) + (cn->calls_done - before);
                    conn_end(i, &conns, &wu_s, &wu_r, &blocked);
                    (void)web(OS_PEER_WEB_CLOSE, i);
                    continue;
                }
            }
            served += cn->calls_done - before;
            if (cn->goaway && !gw_conn_busy(cn)) {
                conn_end(i, &conns, &wu_s, &wu_r, &blocked);
                (void)web(OS_PEER_WEB_CLOSE, i);
            }
        }
        nap();
    }
    for (i = 0; i < (int)OS_PEER_WEB_SLOTS; i++) conn_end(i, &conns, &wu_s, &wu_r, &blocked);
    (void)web(OS_PEER_WEB_STOP, 0);
    say("grpc-serve ok served ", served, "");
    say(" failed ", failed, "");
    say(" connections ", conns, "");
    say(" window_updates_sent ", wu_s, "");
    say(" window_updates_recv ", wu_r, "");
    say(" blocked ", blocked, "\n");
    return served == want ? 0 : 2;
}

/* ------------------------------------------------------------ client side */
#define GC_PIECE ((int)OS_NET_RELAY_MAX_IN)
static gw_conn_t g_cc;
static int g_segs;

/* The relayed SYS_SOCKET_SEND carries up to OS_NET_RELAY_MAX_IN bytes: one
 * TCP segment per piece, the peer reassembles the stream. */
static int send_all(int sid, const unsigned char* b, int n) {
    static unsigned char seg[640];
    os_socket_send_request_t s;
    uint16_t seg_len = 0U;
    int i, rc;
    for (i = 0; i < n; i += GC_PIECE) {
        int k = n - i > GC_PIECE ? GC_PIECE : n - i;
        s.socket_id = sid; s.payload = b + i; s.length = (uint16_t)k;
        s.segment = seg; s.capacity = sizeof(seg); s.out_length = &seg_len;
        rc = sys1(SYS_SOCKET_SEND, &s);
        if (rc != 0) return rc;
        g_segs++;
    }
    return 0;
}

/* Opens the connection, runs `calls` unary calls (pipelined, streams 1, 3,
 * ...) with stream receive window `window`, closes with GOAWAY. */
static int run_calls(const char* tag, const char* rest, int multi) {
    char lip[20], rip[20], ps[8], ns[8], ws[8], svc[48], meth[32], pay[48];
    static unsigned char buf[OS_PEER_DATA_MAX], rx[1024];
    os_socket_connect_request_t c;
    os_socket_receive_request_t r;
    uint16_t got = 0U;
    int port, sid, rc = 0, i, k, rounds, idle = 0, rlen = 0, calls = 1, window = GW_WIN_DEFAULT, n, used, ok = 0;
    int rx_bytes = 0, req_bytes = 0, send_fail = 0;
    unsigned t0;
    rest = word(rest, lip, (int)sizeof(lip));
    rest = word(rest, rip, (int)sizeof(rip));
    rest = word(rest, ps, (int)sizeof(ps));
    if (multi) {
        rest = word(rest, ns, (int)sizeof(ns));
        rest = word(rest, ws, (int)sizeof(ws));
        calls = to_int(ns);
        window = to_int(ws);
    }
    rest = word(rest, svc, (int)sizeof(svc));
    rest = word(rest, meth, (int)sizeof(meth));
    rest = word(rest, pay, (int)sizeof(pay));
    for (i = 0; i < (int)sizeof(c); i++) ((uint8_t*)&c)[i] = 0U;
    port = to_int(ps);
    if (parse_ip(lip, c.local_ip) != 0 || parse_ip(rip, c.remote_ip) != 0 || port <= 0 || port > 65535 ||
        !svc[0] || !meth[0] || calls < 1 || calls > GW_MAX_STREAMS || window < 1 || window > GW_WIN_DEFAULT) {
        print_string(tag);
        print_string(multi ? " error usage: grpc-calls LOCAL_IP REMOTE_IP PORT CALLS WINDOW SERVICE METHOD [PAYLOAD]\n"
                           : " error usage: grpc-call LOCAL_IP REMOTE_IP PORT SERVICE METHOD [PAYLOAD]\n");
        return 1;
    }
    gw_conn_init(&g_cc, 0, window, 0);
    n = gw_conn_start(&g_cc, buf, (int)sizeof(buf));
    for (i = 0; i < calls && n > 0; i++) {
        char p[56];
        int m = 0, j;
        for (j = 0; pay[j] && m < 40; j++) p[m++] = pay[j];
        if (multi) { p[m++] = '-'; p[m++] = (char)('1' + i); }
        p[m] = 0;
        k = gw_conn_call(&g_cc, svc, meth, p, buf + n, (int)sizeof(buf) - n);
        n = k < 0 ? k : n + k;
    }
    if (n < 0) { print_string(tag); say(" error encode ", n, "\n"); return 1; }
    t0 = ticks();
    g_segs = 0;
    c.local_port = (uint16_t)(41000U + (t0 & 0x3FFU));
    c.remote_port = (uint16_t)port;
    c.local_sequence = 0x67A00000U ^ (t0 << 8);
    c.attempts = 400U;
    sid = sys1(SYS_SOCKET_CONNECT, &c);
    if (sid < 0) { print_string(tag); say(" error connect rc ", sid, "\n"); return 1; }
    print_string(tag); say(" connected socket ", sid, ""); say(" local_port ", c.local_port, "\n");
    rc = send_all(sid, buf, n);
    if (rc != 0) { print_string(tag); say(" error send rc ", rc, "\n"); (void)sys1(SYS_SOCKET_CLOSE, (void*)sid); return 1; }
    req_bytes = n;
    print_string(tag); say(" sent request_bytes ", n, ""); say(" segments ", g_segs, ""); say(" calls ", calls, "\n");
    for (rounds = 0; rounds < 400 && g_cc.calls_done < g_cc.calls_started && idle < 60; rounds++) {
        got = 0U;
        r.socket_id = sid; r.buffer = rx + rlen; r.capacity = (uint16_t)(sizeof(rx) - (unsigned)rlen);
        if (r.capacity > 256U) r.capacity = 256U;
        r.out_length = &got;
        rc = sys1(SYS_SOCKET_RECEIVE, &r);
        if (rc != 0 && rc != OS_NET_WIRE_TIMEOUT) break;
        if (got == 0U) { idle++; continue; }
        idle = 0;
        rlen += got; rx_bytes += got;
        n = gw_conn_feed(&g_cc, rx, rlen, &used, buf, (int)sizeof(buf));
        if (n < 0) { rc = n; break; }
        for (k = used; k < rlen; k++) rx[k - used] = rx[k];
        rlen -= used;
        /* a failed WINDOW_UPDATE send is not fatal: the rest of the
         * response may already be on its way */
        if (n > 0 && send_all(sid, buf, n) != 0) send_fail++;
        if (rlen >= (int)sizeof(rx)) { rc = GW_ERR_LIMIT; break; }
    }
    if (g_cc.calls_done == g_cc.calls_started) {
        n = gw_conn_goaway(&g_cc, buf, (int)sizeof(buf));
        if (n > 0) (void)send_all(sid, buf, n);
    }
    (void)sys1(SYS_SOCKET_CLOSE, (void*)sid);
    if (g_cc.calls_done != g_cc.calls_started) {
        print_string(tag); say(" error response incomplete calls_done ", g_cc.calls_done, "");
        say(" bytes ", rx_bytes, ""); say(" send_fail ", send_fail, ""); say(" rc ", rc, "\n");
        return 2;
    }
    for (i = 0; i < calls; i++) {
        gw_h2stream_t* s = gw_conn_stream(&g_cc, (unsigned)(1 + 2 * i));
        if (!s) continue;
        print_string(tag);
        if (multi) say(" stream ", (int)s->id, "");
        say(" ok http ", s->http_status, "");
        say(" grpc-status ", s->grpc_status, "");
        say(" frames ", s->frames, "");
        if (!multi) {
            say(" server_settings ", g_cc.settings_seen, "");
            say(" settings_ack ", g_cc.settings_acks, "");
            say(" response_bytes ", rx_bytes, "");
            say(" ms ", (int)((ticks() - t0) * 10U), "");
        }
        print_string(" message ");
        print_string(s->message);
        print_string(" transport=tcp\n");
        if (s->http_status == 200 && s->grpc_status == 0) ok++;
    }
    if (multi) {
        print_string(tag);
        say(" ok calls ", calls, "");
        say(" succeeded ", ok, "");
        say(" connections 1 window ", window, "");
        say(" request_bytes ", req_bytes, "");
        say(" response_bytes ", rx_bytes, "");
        say(" window_updates_sent ", g_cc.wu_sent, "");
        say(" window_updates_recv ", g_cc.wu_recv, "");
        say(" data_frames_recv ", g_cc.data_recv, "");
        say(" server_settings ", g_cc.settings_seen, "");
        say(" ms ", (int)((ticks() - t0) * 10U), " transport=tcp\n");
    }
    return ok == calls ? 0 : 3;
}

static int grpc_call(const char* rest) { return run_calls("grpc-call", rest, 0); }
static int grpc_calls(const char* rest) { return run_calls("grpc-calls", rest, 1); }


/* ----------------------------------------------- Lot 9: gRPC inside TLS */
static int g_tls_rec_sent, g_tls_rec_recv, g_tls_bytes_sent, g_tls_bytes_recv;

/* One application_data record per OS_PEER_TLS_APP_MAX bytes. */
static int tls_send_all(int slot, const unsigned char* b, int n) {
    int i, k, j, rc;
    for (i = 0; i < n; i += (int)OS_PEER_TLS_APP_MAX) {
        k = n - i > (int)OS_PEER_TLS_APP_MAX ? (int)OS_PEER_TLS_APP_MAX : n - i;
        for (j = 0; j < k; j++) g_io.data[j] = b[i + j];
        g_io.length = (uint16_t)k;
        rc = web(OS_PEER_TLS_APP_SEND, slot);
        if (rc != 0) return rc;
        g_tls_rec_sent++;
        g_tls_bytes_sent += k;
    }
    return 0;
}

/* 0 = data appended, 1 = nothing yet, <0 = error. */
static int tls_recv(int slot, unsigned char* rx, int* rlen, int cap) {
    int rc, k;
    g_io.attempts = 24U;
    rc = web(OS_PEER_TLS_APP_RECV, slot);
    if (rc != 0) return rc;
    if (*rlen + (int)g_io.length > cap) return GW_ERR_LIMIT;
    for (k = 0; k < g_io.length; k++) rx[(*rlen)++] = g_io.data[k];
    g_tls_rec_recv++;
    g_tls_bytes_recv += g_io.length;
    return 0;
}

static void tls_counters(void) {
    say(" records_sent ", g_tls_rec_sent, "");
    say(" records_recv ", g_tls_rec_recv, "");
    say(" bytes_sent ", g_tls_bytes_sent, "");
    say(" bytes_recv ", g_tls_bytes_recv, "");
}

static int grpc_tls_serve(const char* rest) {
    char a[16], b[16];
    static unsigned char out[OS_PEER_DATA_MAX];
    gw_conn_t* cn = &g_conn[0];
    int want, secs, served = 0, rc, k, used, m, before, rlen = 0;
    unsigned t0, linger = 0;
    rest = word(rest, a, (int)sizeof(a));
    rest = word(rest, b, (int)sizeof(b));
    want = a[0] ? to_int(a) : 1;
    secs = b[0] ? to_int(b) : 120;
    if (want <= 0 || want > GW_MAX_STREAMS || secs <= 0 || secs > 600) {
        print_string("grpc-tls-serve error usage: grpc-tls-serve [CALLS] [SECONDS]\n");
        return 1;
    }
    g_tls_rec_sent = g_tls_rec_recv = g_tls_bytes_sent = g_tls_bytes_recv = 0;
    g_over = " from guest over tls"; g_tr = " transport=tls\n";
    gw_conn_init(cn, 1, GW_WIN_DEFAULT, serve_handler);
    g_cur_slot = 0; g_cur_conn = cn;
    print_string("grpc-tls-serve ready session=peer-tls12 transport=tls\n");
    t0 = ticks();
    for (;;) {
        unsigned now = ticks();
        if (now - t0 >= (unsigned)secs * 100U) break;
        if (served >= want) {
            if (cn->goaway) break;
            if (!linger) linger = now;
            else if (now - linger > 300U) break;
        }
        rc = tls_recv(0, g_rx[0], &rlen, GS_BUF);
        if (rc == 1) { nap(); continue; }
        if (rc != 0) { say("grpc-tls-serve error recv rc ", rc, "\n"); break; }
        before = cn->calls_done;
        m = gw_conn_feed(cn, g_rx[0], rlen, &used, out, (int)sizeof(out));
        if (m < 0) { say("grpc-tls-serve error stream rc ", m, "\n"); break; }
        for (k = used; k < rlen; k++) g_rx[0][k - used] = g_rx[0][k];
        rlen -= used;
        if (m > 0 && (rc = tls_send_all(0, out, m)) != 0) { say("grpc-tls-serve error send rc ", rc, "\n"); break; }
        served += cn->calls_done - before;
    }
    g_over = " from guest over tcp"; g_tr = " transport=tcp\n";
    print_string(served == want ? "grpc-tls-serve ok" : "grpc-tls-serve incomplete");
    say(" served ", served, "");
    say(" window_updates_sent ", cn->wu_sent, "");
    say(" goaway ", cn->goaway, "");
    tls_counters();
    print_string(" cipher=aes128-gcm transport=tls\n");
    return served == want ? 0 : 2;
}

static int grpc_tls_call(const char* rest) {
    char ns[8], svc[48], meth[32], pay[48];
    static unsigned char buf[OS_PEER_DATA_MAX], rx[1024];
    int calls, n, k, i, rc = 0, rounds, idle = 0, rlen = 0, used, ok = 0;
    unsigned t0;
    rest = word(rest, ns, (int)sizeof(ns));
    rest = word(rest, svc, (int)sizeof(svc));
    rest = word(rest, meth, (int)sizeof(meth));
    rest = word(rest, pay, (int)sizeof(pay));
    calls = to_int(ns);
    if (calls < 1 || calls > GW_MAX_STREAMS || !svc[0] || !meth[0]) {
        print_string("grpc-tls-call error usage: grpc-tls-call CALLS SERVICE METHOD [PAYLOAD]\n");
        return 1;
    }
    g_tls_rec_sent = g_tls_rec_recv = g_tls_bytes_sent = g_tls_bytes_recv = 0;
    gw_conn_init(&g_cc, 0, GW_WIN_DEFAULT, 0);
    n = gw_conn_start(&g_cc, buf, (int)sizeof(buf));
    for (i = 0; i < calls && n > 0; i++) {
        char p[56];
        int m = 0, j;
        for (j = 0; pay[j] && m < 40; j++) p[m++] = pay[j];
        p[m++] = '-'; p[m++] = (char)('1' + i); p[m] = 0;
        k = gw_conn_call(&g_cc, svc, meth, p, buf + n, (int)sizeof(buf) - n);
        n = k < 0 ? k : n + k;
    }
    if (n < 0) { say("grpc-tls-call error encode ", n, "\n"); return 1; }
    t0 = ticks();
    rc = tls_send_all(1, buf, n);
    if (rc != 0) { say("grpc-tls-call error send rc ", rc, "\n"); return 1; }
    say("grpc-tls-call sent request_bytes ", n, "");
    say(" records ", g_tls_rec_sent, "");
    say(" calls ", calls, " session=llm-tls12 transport=tls\n");
    for (rounds = 0; rounds < 600 && g_cc.calls_done < g_cc.calls_started && idle < 120; rounds++) {
        rc = tls_recv(1, rx, &rlen, (int)sizeof(rx));
        if (rc == 1) { idle++; nap(); continue; }
        if (rc != 0) break;
        idle = 0;
        n = gw_conn_feed(&g_cc, rx, rlen, &used, buf, (int)sizeof(buf));
        if (n < 0) { rc = n; break; }
        for (k = used; k < rlen; k++) rx[k - used] = rx[k];
        rlen -= used;
        if (n > 0 && (rc = tls_send_all(1, buf, n)) != 0) break;
        rc = 0;
    }
    if (g_cc.calls_done == g_cc.calls_started) {
        n = gw_conn_goaway(&g_cc, buf, (int)sizeof(buf));
        if (n > 0) (void)tls_send_all(1, buf, n);
    } else {
        say("grpc-tls-call error response incomplete calls_done ", g_cc.calls_done, "");
        say(" rc ", rc, "");
        tls_counters();
        print_string("\n");
        return 2;
    }
    for (i = 0; i < calls; i++) {
        gw_h2stream_t* s = gw_conn_stream(&g_cc, (unsigned)(1 + 2 * i));
        if (!s) continue;
        say("grpc-tls-call stream ", (int)s->id, "");
        say(" ok http ", s->http_status, "");
        say(" grpc-status ", s->grpc_status, "");
        print_string(" message ");
        print_string(s->message);
        print_string(" transport=tls\n");
        if (s->http_status == 200 && s->grpc_status == 0) ok++;
    }
    say("grpc-tls-call ok calls ", calls, "");
    say(" succeeded ", ok, "");
    say(" server_settings ", g_cc.settings_seen, "");
    tls_counters();
    say(" ms ", (int)((ticks() - t0) * 10U), " cipher=aes128-gcm transport=tls\n");
    return ok == calls ? 0 : 3;
}

int shell_grpc_is(const char* line) {
    return (s_pre(line, "grpc-tls-serve") && (line[14] == ' ' || !line[14])) ||
           (s_pre(line, "grpc-tls-call") && (line[13] == ' ' || !line[13])) ||
           (s_pre(line, "grpc-serve") && (line[10] == ' ' || !line[10])) ||
           (s_pre(line, "grpc-calls") && (line[10] == ' ' || !line[10])) ||
           (s_pre(line, "grpc-call") && (line[9] == ' ' || !line[9]));
}

int shell_grpc_line(const char* line) {
    char cmd[20];
    const char* rest = word(line, cmd, (int)sizeof(cmd));
    if (s_eq(cmd, "grpc-serve")) return grpc_serve(rest);
    if (s_eq(cmd, "grpc-call")) return grpc_call(rest);
    if (s_eq(cmd, "grpc-calls")) return grpc_calls(rest);
    if (s_eq(cmd, "grpc-tls-serve")) return grpc_tls_serve(rest);
    if (s_eq(cmd, "grpc-tls-call")) return grpc_tls_call(rest);
    return 1;
}
