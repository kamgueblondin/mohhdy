/* Lot 5 US-067 (partial): gRPC wire encoding (userspace/grpcwire.c).
 * Vectors from the protobuf encoding guide and RFC 7541 appendix C. */
#include "../../framework/unity.h"
#include "../../../userspace/grpcwire.h"
#include <stdio.h>
#include <string.h>

void setUp(void) {}
void tearDown(void) {}

static void test_protobuf_wire(void) {
    unsigned char b[64];
    char s[16];
    unsigned int v = 0;
    static const unsigned char k150[] = {0x08, 0x96, 0x01};
    static const unsigned char ktesting[] = {0x12, 0x07, 't', 'e', 's', 't', 'i', 'n', 'g'};
    TEST_ASSERT_EQUAL(3, gw_pb_put_uint(b, 64, 1, 150));
    TEST_ASSERT_EQUAL(0, memcmp(b, k150, 3));
    TEST_ASSERT_EQUAL(9, gw_pb_put_string(b + 3, 61, 2, "testing"));
    TEST_ASSERT_EQUAL(0, memcmp(b + 3, ktesting, 9));
    TEST_ASSERT_EQUAL(1, gw_pb_get_uint(b, 12, 1, &v));
    TEST_ASSERT_EQUAL(150U, v);
    TEST_ASSERT_EQUAL(1, gw_pb_get_string(b, 12, 2, s, 16));     /* skips field 1 */
    TEST_ASSERT_EQUAL_STRING("testing", s);
    TEST_ASSERT_EQUAL(0, gw_pb_get_string(b, 12, 7, s, 16));
    TEST_ASSERT_EQUAL(GW_ERR_LIMIT, gw_pb_get_string(b, 12, 2, s, 7));
    TEST_ASSERT_EQUAL(GW_ERR_SHORT, gw_pb_get_string(b, 10, 2, s, 16));   /* truncated */
    TEST_ASSERT_EQUAL(5, gw_varint_put(b, 64, 0xFFFFFFFFU));
    TEST_ASSERT_EQUAL(5, gw_varint_get(b, 5, &v));
    TEST_ASSERT_EQUAL(0xFFFFFFFFU, v);
    b[4] = 0x7F;
    TEST_ASSERT_EQUAL(GW_ERR_FORMAT, gw_varint_get(b, 5, &v));       /* > 32 bits */
}

static void test_lpm_and_h2_frames(void) {
    unsigned char b[64];
    const unsigned char* p;
    int n, t, f, l;
    unsigned int st;
    static const unsigned char ksettings[] = {0, 0, 0, 4, 0, 0, 0, 0, 0};
    n = gw_lpm_put(b, 64, (const unsigned char*)"abc", 3);
    TEST_ASSERT_EQUAL(8, n);
    TEST_ASSERT_EQUAL(0, b[0]);
    TEST_ASSERT_EQUAL(3, b[4]);
    TEST_ASSERT_EQUAL(8, gw_lpm_get(b, 8, &p, &l));
    TEST_ASSERT_EQUAL(3, l);
    TEST_ASSERT_EQUAL(GW_ERR_SHORT, gw_lpm_get(b, 7, &p, &l));
    b[0] = 1;
    TEST_ASSERT_EQUAL(GW_ERR_FORMAT, gw_lpm_get(b, 8, &p, &l));    /* compressed */
    n = gw_h2_preface(b, 64);
    TEST_ASSERT_EQUAL(33, n);
    TEST_ASSERT_EQUAL(0, memcmp(b, "PRI * HTTP/2.0\r\n\r\nSM\r\n\r\n", 24));
    TEST_ASSERT_EQUAL(0, memcmp(b + 24, ksettings, 9));
    n = gw_h2_frame_put(b, 64, GW_H2_DATA, GW_H2_FLAG_END_STREAM, 0x80000003U, (const unsigned char*)"xy", 2);
    TEST_ASSERT_EQUAL(11, n);
    TEST_ASSERT_EQUAL(0, b[5] & 0x80);                              /* reserved bit clear */
    TEST_ASSERT_EQUAL(11, gw_h2_frame_get(b, 11, &t, &f, &st, &p, &l));
    TEST_ASSERT_EQUAL(GW_H2_DATA, t);
    TEST_ASSERT_EQUAL(GW_H2_FLAG_END_STREAM, f);
    TEST_ASSERT_EQUAL(3U, st);
    TEST_ASSERT_EQUAL(2, l);
    TEST_ASSERT_EQUAL(GW_ERR_SHORT, gw_h2_frame_get(b, 10, &t, &f, &st, &p, &l));
}

static void test_hpack_rfc7541_vectors(void) {
    unsigned char b[64];
    gw_header_t h[4];
    /* C.2.2: literal without indexing, indexed name :path */
    static const unsigned char kpath[] = {0x04, 0x0c, '/', 's', 'a', 'm', 'p', 'l', 'e', '/', 'p', 'a', 't', 'h'};
    /* C.2.4: indexed :method GET */
    static const unsigned char kget[] = {0x82};
    /* C.2.1 (literal with incremental indexing, new name) decodes too */
    static const unsigned char kcustom[] = {0x40, 0x0a, 'c', 'u', 's', 't', 'o', 'm', '-', 'k', 'e', 'y',
                                            0x0d, 'c', 'u', 's', 't', 'o', 'm', '-', 'h', 'e', 'a', 'd', 'e', 'r'};
    static const unsigned char khuff[] = {0x04, 0x81, 0x00};
    TEST_ASSERT_EQUAL(14, gw_hpack_put(b, 64, ":path", "/sample/path"));
    TEST_ASSERT_EQUAL(0, memcmp(b, kpath, 14));
    TEST_ASSERT_EQUAL(1, gw_hpack_put(b, 64, ":method", "GET"));
    TEST_ASSERT_EQUAL(0, memcmp(b, kget, 1));
    TEST_ASSERT_EQUAL(1, gw_hpack_decode(kcustom, (int)sizeof(kcustom), h, 4));
    TEST_ASSERT_EQUAL_STRING("custom-key", h[0].name);
    TEST_ASSERT_EQUAL_STRING("custom-header", h[0].value);
    TEST_ASSERT_EQUAL(1, gw_hpack_decode(kpath, 14, h, 4));
    TEST_ASSERT_EQUAL_STRING(":path", h[0].name);
    TEST_ASSERT_EQUAL_STRING("/sample/path", h[0].value);
    TEST_ASSERT_EQUAL(GW_ERR_FORMAT, gw_hpack_decode(khuff, 3, h, 4));   /* Huffman refused */
    b[0] = 0xBE;                                                        /* index 62: dynamic */
    TEST_ASSERT_EQUAL(GW_ERR_FORMAT, gw_hpack_decode(b, 1, h, 4));
    /* new name and a long value (multi-byte integer) round trip */
    {
        char longv[60];
        int n, i;
        for (i = 0; i < 59; i++) longv[i] = (char)('a' + i % 26);
        longv[59] = 0;
        n = gw_hpack_put(b, 64, "grpc-status", "0");
        TEST_ASSERT_EQUAL(0x00, b[0]);
        TEST_ASSERT_EQUAL(1, gw_hpack_decode(b, n, h, 4));
        TEST_ASSERT_EQUAL_STRING("grpc-status", h[0].name);
        n = gw_hpack_put(b, 64, "x", longv);
        TEST_ASSERT_TRUE(n > 0);
        TEST_ASSERT_EQUAL(1, gw_hpack_decode(b, n, h, 4));
        TEST_ASSERT_EQUAL_STRING(longv, h[0].value);
    }
}

static int handler(const char* path, const char* payload, char* reply, int cap) {
    if (strcmp(path, "/echo.EchoService/SayHello") == 0) {
        snprintf(reply, (size_t)cap, "Hello %s", payload);
        return 0;
    }
    snprintf(reply, (size_t)cap, "unknown %s", path);
    return 12;    /* UNIMPLEMENTED */
}

static void test_unary_call_round_trip(void) {
    unsigned char req[512], resp[512];
    char path[64], msg[128];
    gw_header_t h[8];
    const unsigned char* p;
    int n, r, hs, gs, frames, t, f, l, nh;
    unsigned int st;
    n = gw_build_request(req, (int)sizeof(req), "echo.EchoService", "SayHello", "mohhdy");
    TEST_ASSERT_TRUE(n > 0);
    /* first frame: HEADERS with END_HEADERS on stream 1 */
    TEST_ASSERT_TRUE(gw_h2_frame_get(req, n, &t, &f, &st, &p, &l) > 0);
    TEST_ASSERT_EQUAL(GW_H2_HEADERS, t);
    TEST_ASSERT_EQUAL(GW_H2_FLAG_END_HEADERS, f);
    TEST_ASSERT_EQUAL(1U, st);
    TEST_ASSERT_EQUAL(0x83, p[0]);                  /* :method POST, static index 3 */
    TEST_ASSERT_EQUAL(0x86, p[1]);                  /* :scheme http, static index 6 */
    nh = gw_hpack_decode(p, l, h, 8);
    TEST_ASSERT_EQUAL(6, nh);
    TEST_ASSERT_EQUAL_STRING("/echo.EchoService/SayHello", h[2].value);
    TEST_ASSERT_EQUAL_STRING("application/grpc", h[4].value);
    TEST_ASSERT_EQUAL_STRING("trailers", h[5].value);
    r = gw_serve(req, n, handler, resp, (int)sizeof(resp), path, 64);
    TEST_ASSERT_TRUE(r > 0);
    TEST_ASSERT_EQUAL_STRING("/echo.EchoService/SayHello", path);
    TEST_ASSERT_EQUAL(GW_OK, gw_parse_response(resp, r, &hs, &gs, msg, 128, &frames));
    TEST_ASSERT_EQUAL(200, hs);
    TEST_ASSERT_EQUAL(0, gs);
    TEST_ASSERT_EQUAL(3, frames);                   /* HEADERS, DATA, trailers */
    TEST_ASSERT_EQUAL_STRING("Hello mohhdy", msg);
    /* unimplemented method: no DATA, grpc-status 12 and grpc-message */
    n = gw_build_request(req, (int)sizeof(req), "x.Y", "Z", "");
    r = gw_serve(req, n, handler, resp, (int)sizeof(resp), path, 64);
    TEST_ASSERT_EQUAL(GW_OK, gw_parse_response(resp, r, &hs, &gs, msg, 128, &frames));
    TEST_ASSERT_EQUAL(12, gs);
    TEST_ASSERT_EQUAL(2, frames);
    TEST_ASSERT_EQUAL_STRING("unknown /x.Y/Z", msg);
    /* truncated request and missing trailers are refused */
    n = gw_build_request(req, (int)sizeof(req), "echo.EchoService", "SayHello", "a");
    TEST_ASSERT_TRUE(gw_serve(req, n - 1, handler, resp, (int)sizeof(resp), path, 64) < 0);
    r = gw_serve(req, n, handler, resp, (int)sizeof(resp), path, 64);
    TEST_ASSERT_EQUAL(GW_ERR_SHORT, gw_parse_response(resp, r - 12, &hs, &gs, msg, 128, &frames));
    TEST_ASSERT_EQUAL(GW_ERR_FORMAT, gw_parse_response(resp, r - 24, &hs, &gs, msg, 128, &frames));
}

/* Lot 6 US-067: connection-level bytes, fed in TCP-sized pieces. */
static void test_connection_stream(void) {
    unsigned char c[512], s[512];
    char path[64], msg[128];
    int n, i, r, hs, gs, frames, cs, ss, acks;
    n = gw_client_stream(c, (int)sizeof(c), "echo.EchoService", "SayHello", "guest-a");
    TEST_ASSERT_TRUE(n > 33);
    TEST_ASSERT_EQUAL(0, memcmp(c, "PRI * HTTP/2.0\r\n\r\nSM\r\n\r\n", 24));
    /* incomplete at every split point, complete only with the last byte */
    for (i = 0; i < n; i++) TEST_ASSERT_EQUAL(0, gw_stream_complete(c, i, 1));
    TEST_ASSERT_EQUAL(1, gw_stream_complete(c, n, 1));
    TEST_ASSERT_EQUAL(GW_ERR_SHORT, gw_serve_stream(c, 10, handler, s, 512, path, 64, &cs));
    r = gw_serve_stream(c, n, handler, s, (int)sizeof(s), path, 64, &cs);
    TEST_ASSERT_TRUE(r > 18);
    TEST_ASSERT_EQUAL(1, cs);
    TEST_ASSERT_EQUAL_STRING("/echo.EchoService/SayHello", path);
    /* server: SETTINGS then SETTINGS ACK on stream 0 first */
    TEST_ASSERT_EQUAL(GW_H2_SETTINGS, s[3]); TEST_ASSERT_EQUAL(0, s[4]);
    TEST_ASSERT_EQUAL(GW_H2_SETTINGS, s[12]); TEST_ASSERT_EQUAL(1, s[13]);
    for (i = 0; i < r; i++) TEST_ASSERT_EQUAL(0, gw_stream_complete(s, i, 0));
    TEST_ASSERT_EQUAL(1, gw_stream_complete(s, r, 0));
    TEST_ASSERT_EQUAL(GW_OK, gw_parse_stream_response(s, r, &hs, &gs, msg, 128, &frames, &ss, &acks));
    TEST_ASSERT_EQUAL(200, hs); TEST_ASSERT_EQUAL(0, gs); TEST_ASSERT_EQUAL(3, frames);
    TEST_ASSERT_EQUAL(1, ss); TEST_ASSERT_EQUAL(1, acks);
    TEST_ASSERT_EQUAL_STRING("Hello guest-a", msg);
    /* bad preface, missing client SETTINGS, foreign stream are refused */
    c[0] = 'X';
    TEST_ASSERT_EQUAL(GW_ERR_FORMAT, gw_stream_complete(c, n, 1));
    TEST_ASSERT_EQUAL(GW_ERR_FORMAT, gw_serve_stream(c, n, handler, s, 512, path, 64, &cs));
    c[0] = 'P';
    TEST_ASSERT_EQUAL(GW_ERR_FORMAT, gw_serve_stream(c, 24, handler, s, 512, path, 64, &cs));
    {
        unsigned char d[512];
        memcpy(d, c, 33);
        r = gw_build_request(d + 33, 400, "a.B", "C", "x");
        d[33 + 8] = 3;                     /* HEADERS on stream 3 */
        TEST_ASSERT_EQUAL(GW_ERR_FORMAT, gw_serve_stream(d, 33 + r, handler, s, 512, path, 64, &cs));
    }
}

/* Lot 7 (US-067): several calls per connection and flow control. */
static gw_conn_t g_cli, g_srv;
static unsigned char q_cs[4096], q_sc[4096];   /* client->server, server->client */
static int n_cs, n_sc;

static void q_add(unsigned char* q, int* n, const unsigned char* b, int len) {
    TEST_ASSERT_TRUE(len >= 0 && *n + len <= 4096);
    memcpy(q + *n, b, (size_t)len); *n += len;
}
/* Delivers at most `chunk` bytes each way per round (TCP segmentation). */
static int pump(int chunk, int rounds) {
    static unsigned char rx_s[4096], rx_c[4096], out[1024];
    static int l_s, l_c;
    int r;
    for (r = 0; r < rounds && g_cli.calls_done < g_cli.calls_started; r++) {
        int k, used, o;
        k = n_cs < chunk ? n_cs : chunk;
        memcpy(rx_s + l_s, q_cs, (size_t)k); l_s += k;
        memmove(q_cs, q_cs + k, (size_t)(n_cs - k)); n_cs -= k;
        o = gw_conn_feed(&g_srv, rx_s, l_s, &used, out, (int)sizeof(out));
        if (o < 0) return o;
        memmove(rx_s, rx_s + used, (size_t)(l_s - used)); l_s -= used;
        q_add(q_sc, &n_sc, out, o);
        k = n_sc < chunk ? n_sc : chunk;
        memcpy(rx_c + l_c, q_sc, (size_t)k); l_c += k;
        memmove(q_sc, q_sc + k, (size_t)(n_sc - k)); n_sc -= k;
        o = gw_conn_feed(&g_cli, rx_c, l_c, &used, out, (int)sizeof(out));
        if (o < 0) return o;
        memmove(rx_c, rx_c + used, (size_t)(l_c - used)); l_c -= used;
        q_add(q_cs, &n_cs, out, o);
    }
    return r;
}

static void test_multi_call_flow_control(void) {
    unsigned char b[1024];
    gw_h2stream_t* s;
    int n, i;
    char pay[16];
    n_cs = n_sc = 0;
    gw_conn_init(&g_cli, 0, 16, 0);              /* client stream window: 16 bytes */
    gw_conn_init(&g_srv, 1, GW_WIN_DEFAULT, handler);
    n = gw_conn_start(&g_cli, b, (int)sizeof(b));
    TEST_ASSERT_TRUE(n > 24);
    q_add(q_cs, &n_cs, b, n);
    for (i = 0; i < 3; i++) {                    /* pipelined: streams 1, 3, 5 */
        snprintf(pay, sizeof(pay), "call-%d", i + 1);
        n = gw_conn_call(&g_cli, "echo.EchoService", "SayHello", pay, b, (int)sizeof(b));
        TEST_ASSERT_TRUE(n > 0);
        q_add(q_cs, &n_cs, b, n);
    }
    n = gw_conn_call(&g_cli, "x.Nope", "Missing", "", b, (int)sizeof(b));
    TEST_ASSERT_TRUE(n > 0);
    q_add(q_cs, &n_cs, b, n);
    TEST_ASSERT_EQUAL(9U, g_cli.next_stream);
    TEST_ASSERT_TRUE(pump(7, 4000) > 0);
    TEST_ASSERT_EQUAL(4, g_cli.calls_done);
    for (i = 0; i < 3; i++) {
        char want[32];
        s = gw_conn_stream(&g_cli, (unsigned int)(1 + 2 * i));
        TEST_ASSERT_NOT_NULL(s);
        snprintf(want, sizeof(want), "Hello call-%d", i + 1);
        TEST_ASSERT_EQUAL(200, s->http_status);
        TEST_ASSERT_EQUAL(0, s->grpc_status);
        TEST_ASSERT_EQUAL_STRING(want, s->message);
        /* 19-byte LPM in 16-byte windows: HEADERS, 2 DATA, trailers */
        TEST_ASSERT_EQUAL(4, s->frames);
    }
    s = gw_conn_stream(&g_cli, 7U);
    TEST_ASSERT_EQUAL(12, s->grpc_status);
    TEST_ASSERT_EQUAL_STRING("unknown /x.Nope/Missing", s->message);
    TEST_ASSERT_EQUAL(2, s->frames);
    TEST_ASSERT_EQUAL(4, g_srv.calls_done);
    TEST_ASSERT_EQUAL(16, g_srv.peer_init_win);
    TEST_ASSERT_TRUE(g_srv.blocked >= 3);         /* DATA held by the 16-byte window */
    TEST_ASSERT_TRUE(g_srv.wu_recv >= 6);         /* the client reopened it */
    TEST_ASSERT_TRUE(g_cli.wu_sent >= 6);
    TEST_ASSERT_TRUE(g_cli.wu_recv >= 4);         /* server replenished request DATA */
    TEST_ASSERT_EQUAL(6, g_srv.data_sent);
    TEST_ASSERT_EQUAL(1, g_cli.settings_seen);
    TEST_ASSERT_EQUAL(1, g_srv.settings_acks);
    TEST_ASSERT_EQUAL(1, g_cli.settings_acks);
    TEST_ASSERT_EQUAL(GW_WIN_DEFAULT, g_srv.conn_recv_win);   /* fully replenished */
    TEST_ASSERT_EQUAL(0, gw_conn_busy(&g_srv));
    /* GOAWAY ends the connection */
    n = gw_conn_goaway(&g_cli, b, (int)sizeof(b));
    TEST_ASSERT_EQUAL(17, n);
    {
        int used;
        unsigned char o[256];
        TEST_ASSERT_EQUAL(0, gw_conn_feed(&g_srv, b, n, &used, o, (int)sizeof(o)));
        TEST_ASSERT_EQUAL(1, g_srv.goaway);
        TEST_ASSERT_EQUAL(GW_ERR_PROTO, gw_conn_call(&g_srv, "a", "b", "", b, (int)sizeof(b)));
    }
}

/* raw server connection after preface + SETTINGS exchange */
static int srv_ready(int window) {
    unsigned char b[256], o[512];
    int n, used;
    gw_conn_init(&g_srv, 1, window, handler);
    gw_conn_init(&g_cli, 0, GW_WIN_DEFAULT, 0);
    n = gw_conn_start(&g_cli, b, (int)sizeof(b));
    return gw_conn_feed(&g_srv, b, n, &used, o, (int)sizeof(o)) > 0 && used == n ? 0 : -1;
}
static int srv_frame(int type, int flags, unsigned int id, const unsigned char* p, int len) {
    unsigned char b[512], o[1024];
    int n = gw_h2_frame_put(b, (int)sizeof(b), type, flags, id, p, len), used;
    return gw_conn_feed(&g_srv, b, n, &used, o, (int)sizeof(o));
}

static void test_flow_control_errors(void) {
    unsigned char hb[256], big[64], wu[4], ping[8], o[1024], b[512];
    int n, used;
    memset(big, 'x', sizeof(big));
    /* a client overrunning the server stream window (8 bytes) */
    TEST_ASSERT_EQUAL(0, srv_ready(8));
    n = gw_build_request(b, (int)sizeof(b), "echo.EchoService", "SayHello", "p");
    {
        int t, f, l; unsigned int id; const unsigned char* p;
        int u = gw_h2_frame_get(b, n, &t, &f, &id, &p, &l);
        memcpy(hb, p, (size_t)l);
        TEST_ASSERT_TRUE(u > 0);
        TEST_ASSERT_TRUE(srv_frame(GW_H2_HEADERS, GW_H2_FLAG_END_HEADERS, 1U, hb, l) >= 0);
        TEST_ASSERT_EQUAL(GW_ERR_FLOW, srv_frame(GW_H2_DATA, 0, 1U, big, 9));
        /* stream ids must grow and be odd */
        TEST_ASSERT_EQUAL(0, srv_ready(GW_WIN_DEFAULT));
        TEST_ASSERT_TRUE(srv_frame(GW_H2_HEADERS, GW_H2_FLAG_END_HEADERS, 5U, hb, l) >= 0);
        TEST_ASSERT_EQUAL(GW_ERR_PROTO, srv_frame(GW_H2_HEADERS, GW_H2_FLAG_END_HEADERS, 3U, hb, l));
        TEST_ASSERT_EQUAL(0, srv_ready(GW_WIN_DEFAULT));
        TEST_ASSERT_EQUAL(GW_ERR_PROTO, srv_frame(GW_H2_HEADERS, GW_H2_FLAG_END_HEADERS, 2U, hb, l));
        /* no CONTINUATION support: HEADERS without END_HEADERS is refused */
        TEST_ASSERT_EQUAL(0, srv_ready(GW_WIN_DEFAULT));
        TEST_ASSERT_EQUAL(GW_ERR_FORMAT, srv_frame(GW_H2_HEADERS, 0, 1U, hb, l));
        /* the 9th concurrent stream gets RST_STREAM REFUSED_STREAM */
        TEST_ASSERT_EQUAL(0, srv_ready(GW_WIN_DEFAULT));
        for (n = 0; n < GW_MAX_STREAMS; n++)
            TEST_ASSERT_EQUAL(0, srv_frame(GW_H2_HEADERS, GW_H2_FLAG_END_HEADERS, (unsigned)(1 + 2 * n), hb, l));
        TEST_ASSERT_EQUAL(13, srv_frame(GW_H2_HEADERS, GW_H2_FLAG_END_HEADERS, 17U, hb, l));
        TEST_ASSERT_EQUAL(1, g_srv.refused);
        /* padded DATA: padding counts against the window and is replenished */
        TEST_ASSERT_EQUAL(0, srv_ready(GW_WIN_DEFAULT));
        TEST_ASSERT_EQUAL(0, srv_frame(GW_H2_HEADERS, GW_H2_FLAG_END_HEADERS, 1U, hb, l));
        big[0] = 4;
        TEST_ASSERT_EQUAL(26, srv_frame(GW_H2_DATA, 0x8, 1U, big, 10));
        TEST_ASSERT_EQUAL(GW_WIN_DEFAULT, g_srv.conn_recv_win);
        TEST_ASSERT_EQUAL(5, g_srv.s[0].blen);
        TEST_ASSERT_EQUAL(GW_ERR_PROTO, srv_frame(GW_H2_DATA, 0x8, 1U, big, 3));
    }
    /* WINDOW_UPDATE: zero increment and overflow */
    TEST_ASSERT_EQUAL(0, srv_ready(GW_WIN_DEFAULT));
    memset(wu, 0, 4);
    TEST_ASSERT_EQUAL(GW_ERR_PROTO, srv_frame(GW_H2_WINDOW_UPDATE, 0, 0U, wu, 4));
    TEST_ASSERT_EQUAL(0, srv_ready(GW_WIN_DEFAULT));
    wu[0] = 0x7F; wu[1] = 0xFF; wu[2] = 0xFF; wu[3] = 0xFF;
    TEST_ASSERT_EQUAL(GW_ERR_FLOW, srv_frame(GW_H2_WINDOW_UPDATE, 0, 0U, wu, 4));
    TEST_ASSERT_EQUAL(0, srv_ready(GW_WIN_DEFAULT));
    TEST_ASSERT_EQUAL(GW_ERR_FORMAT, srv_frame(GW_H2_WINDOW_UPDATE, 0, 0U, wu, 3));
    /* PING is answered with the same payload and the ACK flag */
    memcpy(ping, "mohhdy!!", 8);
    {
        unsigned char f[32];
        int t, fl, l; unsigned int id; const unsigned char* p;
        n = gw_h2_frame_put(f, (int)sizeof(f), GW_H2_PING, 0, 0U, ping, 8);
        TEST_ASSERT_EQUAL(17, gw_conn_feed(&g_srv, f, n, &used, o, (int)sizeof(o)));
        TEST_ASSERT_EQUAL(17, gw_h2_frame_get(o, 17, &t, &fl, &id, &p, &l));
        TEST_ASSERT_EQUAL(GW_H2_PING, t);
        TEST_ASSERT_EQUAL(1, fl);
        TEST_ASSERT_EQUAL(0, memcmp(p, "mohhdy!!", 8));
    }
    /* frames before SETTINGS are a protocol error (RFC 9113 3.4) */
    gw_conn_init(&g_srv, 1, GW_WIN_DEFAULT, handler);
    memcpy(b, "PRI * HTTP/2.0\r\n\r\nSM\r\n\r\n", 24);
    n = 24 + gw_h2_frame_put(b + 24, 64, GW_H2_PING, 0, 0U, ping, 8);
    TEST_ASSERT_EQUAL(GW_ERR_PROTO, gw_conn_feed(&g_srv, b, n, &used, o, (int)sizeof(o)));
}

/* SETTINGS_INITIAL_WINDOW_SIZE lowers open streams' windows (may go
 * negative), and the server holds the response until WINDOW_UPDATE. */
static void test_settings_window_change(void) {
    unsigned char b[1024], o[1024], set[6];
    int n, used;
    n_cs = n_sc = 0;
    gw_conn_init(&g_cli, 0, GW_WIN_DEFAULT, 0);
    gw_conn_init(&g_srv, 1, GW_WIN_DEFAULT, handler);
    n = gw_conn_start(&g_cli, b, (int)sizeof(b));
    n += gw_conn_call(&g_cli, "echo.EchoService", "SayHello", "w", b + n, (int)sizeof(b) - n);
    /* the client then announces a zero window before the server answers */
    set[0] = 0; set[1] = 4; set[2] = set[3] = set[4] = set[5] = 0;
    n += gw_h2_frame_put(b + n, (int)sizeof(b) - n, GW_H2_SETTINGS, 0, 0U, set, 6);
    TEST_ASSERT_TRUE(gw_conn_feed(&g_srv, b, n, &used, o, (int)sizeof(o)) > 0);
    TEST_ASSERT_EQUAL(n, used);
    TEST_ASSERT_EQUAL(0, g_srv.peer_init_win);
    TEST_ASSERT_EQUAL(1, g_srv.blocked);
    TEST_ASSERT_EQUAL(0, g_srv.data_sent);
    TEST_ASSERT_EQUAL(1, gw_conn_busy(&g_srv));
    /* a stream WINDOW_UPDATE of 5 lets exactly 5 DATA bytes out */
    {
        unsigned char wu[4] = {0, 0, 0, 5};
        int t, fl, l; unsigned int id; const unsigned char* p;
        n = gw_h2_frame_put(b, (int)sizeof(b), GW_H2_WINDOW_UPDATE, 0, 1U, wu, 4);
        TEST_ASSERT_EQUAL(14, gw_conn_feed(&g_srv, b, n, &used, o, (int)sizeof(o)));
        TEST_ASSERT_EQUAL(14, gw_h2_frame_get(o, 14, &t, &fl, &id, &p, &l));
        TEST_ASSERT_EQUAL(GW_H2_DATA, t);
        TEST_ASSERT_EQUAL(5, l);
        TEST_ASSERT_EQUAL(1U, id);
        TEST_ASSERT_EQUAL(2, g_srv.blocked);
    }
}

int main(void) {
    unity_init();
    RUN_TEST(test_protobuf_wire);
    RUN_TEST(test_lpm_and_h2_frames);
    RUN_TEST(test_hpack_rfc7541_vectors);
    RUN_TEST(test_unary_call_round_trip);
    RUN_TEST(test_connection_stream);
    RUN_TEST(test_multi_call_flow_control);
    RUN_TEST(test_flow_control_errors);
    RUN_TEST(test_settings_window_change);
    unity_print_results();
    unity_cleanup();
    return unity_stats.tests_failed == 0 ? 0 : 1;
}
