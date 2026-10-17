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

int main(void) {
    unity_init();
    RUN_TEST(test_protobuf_wire);
    RUN_TEST(test_lpm_and_h2_frames);
    RUN_TEST(test_hpack_rfc7541_vectors);
    RUN_TEST(test_unary_call_round_trip);
    RUN_TEST(test_connection_stream);
    unity_print_results();
    unity_cleanup();
    return unity_stats.tests_failed == 0 ? 0 : 1;
}
