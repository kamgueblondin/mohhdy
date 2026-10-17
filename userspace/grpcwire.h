#ifndef MOHHDY_GRPCWIRE_H
#define MOHHDY_GRPCWIRE_H
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

#endif
