#ifndef MOHHDY_GRPCWIRE_H
#define MOHHDY_GRPCWIRE_H
#include <stdint.h>
/* Lot 5 (US-067, partial): gRPC wire encoding in guest C, no libc.
 * Protocol Buffers wire format (varint, length-delimited), gRPC
 * Length-Prefixed-Message, HTTP/2 frame headers (RFC 9113 4.1) and an HPACK
 * subset (RFC 7541: indexed static entries, literals without indexing, no
 * Huffman, no dynamic table). The request and response are built and parsed
 * as real HTTP/2 frames, but they are exchanged in memory or inside the
 * sealed P2P datagram channel, not over a TCP HTTP/2 connection. */

#define GW_OK 0
#define GW_ERR_SHORT -1     /* truncated input */
#define GW_ERR_LIMIT -2     /* output buffer too small */
#define GW_ERR_FORMAT -3    /* malformed or unsupported encoding */

#define GW_H2_DATA 0x0
#define GW_H2_HEADERS 0x1
#define GW_H2_SETTINGS 0x4
#define GW_H2_FLAG_END_STREAM 0x1
#define GW_H2_FLAG_END_HEADERS 0x4

/* protobuf */
int gw_varint_put(unsigned char* out, int cap, unsigned int v);
int gw_varint_get(const unsigned char* in, int len, unsigned int* v);   /* bytes used */
int gw_pb_put_string(unsigned char* out, int cap, int field, const char* s);
int gw_pb_put_uint(unsigned char* out, int cap, int field, unsigned int v);
/* Finds field `field`: string copied into out (cap) or varint into *v.
 * Unknown fields are skipped. Returns 1 found, 0 absent, <0 error. */
int gw_pb_get_string(const unsigned char* in, int len, int field, char* out, int cap);
int gw_pb_get_uint(const unsigned char* in, int len, int field, unsigned int* v);

/* gRPC Length-Prefixed-Message: flag byte (0, no compression) + u32 BE */
int gw_lpm_put(unsigned char* out, int cap, const unsigned char* msg, int len);
int gw_lpm_get(const unsigned char* in, int len, const unsigned char** msg, int* mlen);

/* HTTP/2 frame header */
int gw_h2_frame_put(unsigned char* out, int cap, int type, int flags, unsigned int stream,
                    const unsigned char* payload, int len);
int gw_h2_frame_get(const unsigned char* in, int len, int* type, int* flags, unsigned int* stream,
                    const unsigned char** payload, int* plen);   /* bytes used */

/* HPACK subset */
int gw_hpack_put(unsigned char* out, int cap, const char* name, const char* value);
int grpcwire_compress_header(const char* name, const char* val, uint8_t* out, int max_out);
typedef struct { char name[32]; char value[64]; } gw_header_t;
int gw_hpack_decode(const unsigned char* in, int len, gw_header_t* hdrs, int max);   /* count */

/* Client and server halves of one unary call on stream 1. */
int gw_build_request(unsigned char* out, int cap, const char* service, const char* method,
                     const char* payload);
typedef int (*gw_handler_t)(const char* path, const char* payload, char* reply, int cap);
int gw_serve(const unsigned char* req, int len, gw_handler_t handler, unsigned char* out, int cap,
             char* path, int path_cap);
/* Parses a response: :status, grpc-status trailer and protobuf field 1. */
int gw_parse_response(const unsigned char* in, int len, int* http_status, int* grpc_status,
                      char* message, int cap, int* frames);
/* Client connection preface + empty SETTINGS frame (RFC 9113 3.4). */
int gw_h2_preface(unsigned char* out, int cap);

/* Connection level (Lot 6, US-067): the bytes each side writes on one TCP
 * connection. Client: preface + SETTINGS + request (stream 1). Server:
 * SETTINGS + SETTINGS ACK + response. No flow control, one unary call per
 * connection, no PING/GOAWAY/WINDOW_UPDATE. */
int gw_client_stream(unsigned char* out, int cap, const char* service, const char* method,
                     const char* payload);
/* 1 when stream 1 ended (END_STREAM seen), 0 need more bytes, <0 error. */
int gw_stream_complete(const unsigned char* in, int len, int expect_preface);
int gw_serve_stream(const unsigned char* in, int len, gw_handler_t handler, unsigned char* out,
                    int cap, char* path, int path_cap, int* client_settings);
int gw_parse_stream_response(const unsigned char* in, int len, int* http_status, int* grpc_status,
                             char* message, int cap, int* frames, int* server_settings,
                             int* settings_acks);

/* Lot 7 (US-067): one HTTP/2 connection engine for both sides, with several
 * unary calls per connection (client streams 1, 3, 5, ... in flight at the
 * same time) and flow control (RFC 9113 5.2 and 6.9): connection and stream
 * windows, SETTINGS_INITIAL_WINDOW_SIZE applied to open streams, DATA held
 * back while a send window is empty and split to fit, WINDOW_UPDATE emitted
 * for every DATA frame received, FLOW_CONTROL errors when the peer overruns
 * a window. Also PING ACK, RST_STREAM, GOAWAY. Not supported: CONTINUATION,
 * HPACK dynamic table, priorities (ignored), server push, TLS. */
#define GW_H2_RST_STREAM 0x3
#define GW_H2_PING 0x6
#define GW_H2_GOAWAY 0x7
#define GW_H2_WINDOW_UPDATE 0x8
#define GW_H2_CONTINUATION 0x9
#define GW_ERR_FLOW -4      /* peer overran a flow control window */
#define GW_ERR_PROTO -5     /* protocol error (bad stream id, order) */
#define GW_MAX_STREAMS 8
#define GW_BODY_MAX 200
#define GW_WIN_DEFAULT 65535

typedef struct {
    unsigned int id;
    int state;               /* 0 free, 1 receiving, 2 sending, 3 done */
    char path[64];
    int valid;
    unsigned char body[GW_BODY_MAX];
    int blen;
    int recv_win, send_win;
    unsigned char pend[GW_BODY_MAX];
    int plen, poff;
    unsigned char trailer[96];
    int tlen;
    int stalled;
    int http_status, grpc_status, frames;
    char message[160];
    char grpc_message[64];
} gw_h2stream_t;

typedef struct {
    int server;
    int preface_ok;
    int settings_seen, settings_acks;
    int local_init_win;      /* our SETTINGS_INITIAL_WINDOW_SIZE (we receive) */
    int peer_init_win;       /* the peer's one (our stream send windows) */
    int conn_send_win, conn_recv_win;
    unsigned int last_stream, next_stream, cur_stream;
    int calls_started, calls_done;
    int wu_sent, wu_recv, data_sent, data_recv, blocked, pings, goaway, refused;
    gw_handler_t handler;
    gw_h2stream_t s[GW_MAX_STREAMS];
} gw_conn_t;

/* server: 1 server, 0 client. init_window: our stream receive window. */
void gw_conn_init(gw_conn_t* c, int server, int init_window, gw_handler_t handler);
/* Client: preface + SETTINGS(INITIAL_WINDOW_SIZE). The server writes its
 * SETTINGS when gw_conn_feed sees the preface. */
int gw_conn_start(gw_conn_t* c, unsigned char* out, int cap);
/* Client: opens the next stream with HEADERS and DATA (DATA as far as the
 * send windows allow). Returns bytes written; the stream id is
 * c->next_stream - 2. */
int gw_conn_call(gw_conn_t* c, const char* service, const char* method, const char* payload,
                 unsigned char* out, int cap);
/* Consumes complete frames from in (*used bytes), writes the replies
 * (SETTINGS ACK, WINDOW_UPDATE, PING ACK, responses, held DATA). */
int gw_conn_feed(gw_conn_t* c, const unsigned char* in, int len, int* used, unsigned char* out,
                 int cap);
int gw_conn_goaway(gw_conn_t* c, unsigned char* out, int cap);
gw_h2stream_t* gw_conn_stream(gw_conn_t* c, unsigned int id);
/* Streams still sending or receiving. */
int gw_conn_busy(const gw_conn_t* c);

#endif
