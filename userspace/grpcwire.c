/* Lot 5 (US-067, partial): gRPC wire encoding. See grpcwire.h. */
#include "grpcwire.h"

static int g_len(const char* s) { int n = 0; while (s && s[n]) n++; return n; }
static int g_eq(const char* a, const char* b) {
    int i = 0;
    while (a[i] && a[i] == b[i]) i++;
    return a[i] == b[i];
}
static void g_copy(char* d, const char* s, int cap) {
    int i = 0;
    if (cap <= 0) return;
    while (s[i] && i < cap - 1) { d[i] = s[i]; i++; }
    d[i] = 0;
}
static int g_put(unsigned char* out, int cap, int pos, const void* src, int n) {
    const unsigned char* s = (const unsigned char*)src;
    int i;
    if (pos < 0 || n < 0 || pos + n > cap) return GW_ERR_LIMIT;
    for (i = 0; i < n; i++) out[pos + i] = s[i];
    return pos + n;
}

/* ---------------------------------------------------------------- protobuf */
int gw_varint_put(unsigned char* out, int cap, unsigned int v) {
    int n = 0;
    do {
        unsigned char b = (unsigned char)(v & 0x7FU);
        v >>= 7;
        if (v) b |= 0x80U;
        if (n >= cap) return GW_ERR_LIMIT;
        out[n++] = b;
    } while (v);
    return n;
}

int gw_varint_get(const unsigned char* in, int len, unsigned int* v) {
    unsigned int r = 0;
    int i;
    for (i = 0; i < len && i < 5; i++) {
        r |= (unsigned int)(in[i] & 0x7FU) << (7 * i);
        if (!(in[i] & 0x80U)) {
            if (i == 4 && in[i] > 0x0FU) return GW_ERR_FORMAT;   /* > 32 bits */
            *v = r;
            return i + 1;
        }
    }
    return i >= 5 ? GW_ERR_FORMAT : GW_ERR_SHORT;
}

int gw_pb_put_string(unsigned char* out, int cap, int field, const char* s) {
    int n = gw_varint_put(out, cap, ((unsigned int)field << 3) | 2U), m, l = g_len(s);
    if (n < 0) return n;
    m = gw_varint_put(out + n, cap - n, (unsigned int)l);
    if (m < 0) return m;
    return g_put(out, cap, n + m, s, l);
}

int gw_pb_put_uint(unsigned char* out, int cap, int field, unsigned int v) {
    int n = gw_varint_put(out, cap, (unsigned int)field << 3), m;
    if (n < 0) return n;
    m = gw_varint_put(out + n, cap - n, v);
    return m < 0 ? m : n + m;
}

/* Walks the message; on the wanted field returns its wire type and value. */
static int pb_find(const unsigned char* in, int len, int field, int* wt, unsigned int* v,
                   const unsigned char** data) {
    int pos = 0;
    while (pos < len) {
        unsigned int key, val = 0;
        int n = gw_varint_get(in + pos, len - pos, &key), w;
        if (n < 0) return n;
        pos += n;
        w = (int)(key & 7U);
        if (w == 0 || w == 2) {
            n = gw_varint_get(in + pos, len - pos, &val);
            if (n < 0) return n;
            pos += n;
        } else if (w == 1) { val = 0; if (pos + 8 > len) return GW_ERR_SHORT; pos += 8; }
        else if (w == 5) { val = 0; if (pos + 4 > len) return GW_ERR_SHORT; pos += 4; }
        else return GW_ERR_FORMAT;                       /* groups unsupported */
        if (w == 2) {
            if (val > (unsigned int)(len - pos)) return GW_ERR_SHORT;
            if ((int)(key >> 3) == field) { *wt = 2; *v = val; *data = in + pos; return 1; }
            pos += (int)val;
            continue;
        }
        if ((int)(key >> 3) == field && w == 0) { *wt = 0; *v = val; *data = 0; return 1; }
    }
    return 0;
}

int gw_pb_get_string(const unsigned char* in, int len, int field, char* out, int cap) {
    int wt = 0, r, i;
    unsigned int v = 0;
    const unsigned char* d = 0;
    r = pb_find(in, len, field, &wt, &v, &d);
    if (r <= 0) return r;
    if (wt != 2) return GW_ERR_FORMAT;
    if ((int)v >= cap) return GW_ERR_LIMIT;
    for (i = 0; i < (int)v; i++) out[i] = (char)d[i];
    out[v] = 0;
    return 1;
}

int gw_pb_get_uint(const unsigned char* in, int len, int field, unsigned int* v) {
    int wt = 0, r;
    const unsigned char* d = 0;
    r = pb_find(in, len, field, &wt, v, &d);
    if (r <= 0) return r;
    return wt == 0 ? 1 : GW_ERR_FORMAT;
}

/* -------------------------------------------------- Length-Prefixed-Message */
int gw_lpm_put(unsigned char* out, int cap, const unsigned char* msg, int len) {
    if (len < 0 || cap < 5 + len) return GW_ERR_LIMIT;
    out[0] = 0;
    out[1] = (unsigned char)(len >> 24); out[2] = (unsigned char)(len >> 16);
    out[3] = (unsigned char)(len >> 8); out[4] = (unsigned char)len;
    return g_put(out, cap, 5, msg, len);
}

int gw_lpm_get(const unsigned char* in, int len, const unsigned char** msg, int* mlen) {
    unsigned int n;
    if (len < 5) return GW_ERR_SHORT;
    if (in[0] != 0) return GW_ERR_FORMAT;               /* compression unsupported */
    n = ((unsigned int)in[1] << 24) | ((unsigned int)in[2] << 16) | ((unsigned int)in[3] << 8) | in[4];
    if (n > (unsigned int)(len - 5)) return GW_ERR_SHORT;
    *msg = in + 5;
    *mlen = (int)n;
    return 5 + (int)n;
}

/* -------------------------------------------------------- HTTP/2 framing */
int gw_h2_frame_put(unsigned char* out, int cap, int type, int flags, unsigned int stream,
                    const unsigned char* payload, int len) {
    if (len < 0 || len > 16384 || cap < 9 + len) return GW_ERR_LIMIT;
    out[0] = (unsigned char)(len >> 16); out[1] = (unsigned char)(len >> 8); out[2] = (unsigned char)len;
    out[3] = (unsigned char)type; out[4] = (unsigned char)flags;
    stream &= 0x7FFFFFFFU;
    out[5] = (unsigned char)(stream >> 24); out[6] = (unsigned char)(stream >> 16);
    out[7] = (unsigned char)(stream >> 8); out[8] = (unsigned char)stream;
    return g_put(out, cap, 9, payload, len);
}

int gw_h2_frame_get(const unsigned char* in, int len, int* type, int* flags, unsigned int* stream,
                    const unsigned char** payload, int* plen) {
    int n;
    if (len < 9) return GW_ERR_SHORT;
    n = (in[0] << 16) | (in[1] << 8) | in[2];
    if (n > len - 9) return GW_ERR_SHORT;
    *type = in[3]; *flags = in[4];
    *stream = (((unsigned int)in[5] << 24) | ((unsigned int)in[6] << 16) |
               ((unsigned int)in[7] << 8) | in[8]) & 0x7FFFFFFFU;
    *payload = in + 9;
    *plen = n;
    return 9 + n;
}

int gw_h2_preface(unsigned char* out, int cap) {
    const char* p = "PRI * HTTP/2.0\r\n\r\nSM\r\n\r\n";
    int n = g_put(out, cap, 0, p, 24);
    if (n < 0) return n;
    {
        int m = gw_h2_frame_put(out + n, cap - n, GW_H2_SETTINGS, 0, 0, 0, 0);
        return m < 0 ? m : n + m;
    }
}

/* ------------------------------------------------------------ HPACK subset */
static const char* const k_static[61][2] = {
    {":authority", ""}, {":method", "GET"}, {":method", "POST"}, {":path", "/"},
    {":path", "/index.html"}, {":scheme", "http"}, {":scheme", "https"}, {":status", "200"},
    {":status", "204"}, {":status", "206"}, {":status", "304"}, {":status", "400"},
    {":status", "404"}, {":status", "500"}, {"accept-charset", ""}, {"accept-encoding", "gzip, deflate"},
    {"accept-language", ""}, {"accept-ranges", ""}, {"accept", ""}, {"access-control-allow-origin", ""},
    {"age", ""}, {"allow", ""}, {"authorization", ""}, {"cache-control", ""},
    {"content-disposition", ""}, {"content-encoding", ""}, {"content-language", ""}, {"content-length", ""},
    {"content-location", ""}, {"content-range", ""}, {"content-type", ""}, {"cookie", ""},
    {"date", ""}, {"etag", ""}, {"expect", ""}, {"expires", ""},
    {"from", ""}, {"host", ""}, {"if-match", ""}, {"if-modified-since", ""},
    {"if-none-match", ""}, {"if-range", ""}, {"if-unmodified-since", ""}, {"last-modified", ""},
    {"link", ""}, {"location", ""}, {"max-forwards", ""}, {"proxy-authenticate", ""},
    {"proxy-authorization", ""}, {"range", ""}, {"referer", ""}, {"refresh", ""},
    {"retry-after", ""}, {"server", ""}, {"set-cookie", ""}, {"strict-transport-security", ""},
    {"transfer-encoding", ""}, {"user-agent", ""}, {"vary", ""}, {"via", ""},
    {"www-authenticate", ""}
};

static int hp_int_put(unsigned char* out, int cap, int pos, int prefix_bits, unsigned char first,
                      unsigned int v) {
    unsigned int max = (1U << prefix_bits) - 1U;
    if (pos >= cap) return GW_ERR_LIMIT;
    if (v < max) { out[pos++] = (unsigned char)(first | v); return pos; }
    out[pos++] = (unsigned char)(first | max);
    v -= max;
    while (v >= 128U) {
        if (pos >= cap) return GW_ERR_LIMIT;
        out[pos++] = (unsigned char)((v & 0x7FU) | 0x80U);
        v >>= 7;
    }
    if (pos >= cap) return GW_ERR_LIMIT;
    out[pos++] = (unsigned char)v;
    return pos;
}

static int hp_int_get(const unsigned char* in, int len, int pos, int prefix_bits, unsigned int* v) {
    unsigned int max = (1U << prefix_bits) - 1U, r, m = 0;
    if (pos >= len) return GW_ERR_SHORT;
    r = in[pos++] & max;
    if (r < max) { *v = r; return pos; }
    for (;;) {
        if (pos >= len) return GW_ERR_SHORT;
        if (m > 21) return GW_ERR_FORMAT;
        r += (unsigned int)(in[pos] & 0x7FU) << m;
        m += 7;
        if (!(in[pos++] & 0x80U)) break;
    }
    *v = r;
    return pos;
}

static int hp_str_put(unsigned char* out, int cap, int pos, const char* s) {
    int l = g_len(s);
    pos = hp_int_put(out, cap, pos, 7, 0x00, (unsigned int)l);    /* H = 0, no Huffman */
    if (pos < 0) return pos;
    return g_put(out, cap, pos, s, l);
}

int gw_hpack_put(unsigned char* out, int cap, const char* name, const char* value) {
    int i, name_idx = 0, pos;
    for (i = 0; i < 61; i++) {
        if (!g_eq(k_static[i][0], name)) continue;
        if (k_static[i][1][0] && g_eq(k_static[i][1], value))
            return hp_int_put(out, cap, 0, 7, 0x80, (unsigned int)(i + 1));   /* indexed */
        if (!name_idx) name_idx = i + 1;
    }
    if (name_idx) {                                      /* literal w/o indexing, indexed name */
        pos = hp_int_put(out, cap, 0, 4, 0x00, (unsigned int)name_idx);
    } else {                                             /* literal w/o indexing, new name */
        pos = hp_int_put(out, cap, 0, 4, 0x00, 0U);
        if (pos >= 0) pos = hp_str_put(out, cap, pos, name);
    }
    if (pos < 0) return pos;
    return hp_str_put(out, cap, pos, value);
}

static int hp_str_get(const unsigned char* in, int len, int pos, char* out, int cap) {
    unsigned int l;
    int i;
    if (pos >= len) return GW_ERR_SHORT;
    if (in[pos] & 0x80U) return GW_ERR_FORMAT;           /* Huffman: unsupported */
    pos = hp_int_get(in, len, pos, 7, &l);
    if (pos < 0) return pos;
    if (l > (unsigned int)(len - pos)) return GW_ERR_SHORT;
    if ((int)l >= cap) return GW_ERR_LIMIT;
    for (i = 0; i < (int)l; i++) out[i] = (char)in[pos + i];
    out[l] = 0;
    return pos + (int)l;
}

int gw_hpack_decode(const unsigned char* in, int len, gw_header_t* hdrs, int max) {
    int pos = 0, count = 0;
    while (pos < len) {
        unsigned char b = in[pos];
        unsigned int idx;
        if (count >= max) return GW_ERR_LIMIT;
        if (b & 0x80U) {                                 /* indexed field */
            pos = hp_int_get(in, len, pos, 7, &idx);
            if (pos < 0) return pos;
            if (idx == 0U || idx > 61U) return GW_ERR_FORMAT;   /* no dynamic table */
            g_copy(hdrs[count].name, k_static[idx - 1][0], 32);
            g_copy(hdrs[count].value, k_static[idx - 1][1], 64);
            count++;
            continue;
        }
        if ((b & 0xE0U) == 0x20U) {                      /* table size update */
            pos = hp_int_get(in, len, pos, 5, &idx);
            if (pos < 0) return pos;
            continue;
        }
        /* literal: with incremental indexing (6-bit) is decoded but not
         * stored; without indexing / never indexed (4-bit) */
        pos = hp_int_get(in, len, pos, (b & 0x40U) ? 6 : 4, &idx);
        if (pos < 0) return pos;
        if (idx) {
            if (idx > 61U) return GW_ERR_FORMAT;
            g_copy(hdrs[count].name, k_static[idx - 1][0], 32);
        } else {
            pos = hp_str_get(in, len, pos, hdrs[count].name, 32);
            if (pos < 0) return pos;
        }
        pos = hp_str_get(in, len, pos, hdrs[count].value, 64);
        if (pos < 0) return pos;
        count++;
    }
    return count;
}

/* ------------------------------------------------------------ unary call */
static int hdr_block(unsigned char* out, int cap, const char* const* kv, int pairs) {
    int i, pos = 0;
    for (i = 0; i < pairs; i++) {
        int n = gw_hpack_put(out + pos, cap - pos, kv[2 * i], kv[2 * i + 1]);
        if (n < 0) return n;
        pos += n;
    }
    return pos;
}

static int data_frame(unsigned char* out, int cap, int flags, const char* text) {
    unsigned char pb[200], lpm[210];
    int n = gw_pb_put_string(pb, (int)sizeof(pb), 1, text), m;
    if (n < 0) return n;
    m = gw_lpm_put(lpm, (int)sizeof(lpm), pb, n);
    if (m < 0) return m;
    return gw_h2_frame_put(out, cap, GW_H2_DATA, flags, 1U, lpm, m);
}

int gw_build_request(unsigned char* out, int cap, const char* service, const char* method,
                     const char* payload) {
    char path[64];
    unsigned char hb[256];
    const char* kv[12];
    int n, pos, k = 0, i;
    path[k++] = '/';
    for (i = 0; service[i] && k < 62; i++) path[k++] = service[i];
    if (k < 62) path[k++] = '/';
    for (i = 0; method[i] && k < 63; i++) path[k++] = method[i];
    path[k] = 0;
    kv[0] = ":method"; kv[1] = "POST"; kv[2] = ":scheme"; kv[3] = "http";
    kv[4] = ":path"; kv[5] = path; kv[6] = ":authority"; kv[7] = "mohhdy";
    kv[8] = "content-type"; kv[9] = "application/grpc"; kv[10] = "te"; kv[11] = "trailers";
    n = hdr_block(hb, (int)sizeof(hb), kv, 6);
    if (n < 0) return n;
    pos = gw_h2_frame_put(out, cap, GW_H2_HEADERS, GW_H2_FLAG_END_HEADERS, 1U, hb, n);
    if (pos < 0) return pos;
    n = data_frame(out + pos, cap - pos, GW_H2_FLAG_END_STREAM, payload ? payload : "");
    return n < 0 ? n : pos + n;
}

static const char* hdr_find(const gw_header_t* h, int n, const char* name) {
    int i;
    for (i = 0; i < n; i++) if (g_eq(h[i].name, name)) return h[i].value;
    return 0;
}

int gw_serve(const unsigned char* req, int len, gw_handler_t handler, unsigned char* out, int cap,
             char* path, int path_cap) {
    gw_header_t h[12];
    char payload[160], reply[160], status[4];
    unsigned char hb[128];
    const char* kv[4];
    int pos = 0, nh = -1, have_data = 0, gst, n, o;
    path[0] = 0; payload[0] = 0;
    while (pos < len) {
        int type, flags, plen, used;
        unsigned int stream;
        const unsigned char* p;
        used = gw_h2_frame_get(req + pos, len - pos, &type, &flags, &stream, &p, &plen);
        if (used < 0) return used;
        pos += used;
        if (stream != 1U) return GW_ERR_FORMAT;
        if (type == GW_H2_HEADERS) {
            nh = gw_hpack_decode(p, plen, h, 12);
            if (nh < 0) return nh;
        } else if (type == GW_H2_DATA) {
            const unsigned char* msg;
            int mlen, r = gw_lpm_get(p, plen, &msg, &mlen);
            if (r < 0) return r;
            r = gw_pb_get_string(msg, mlen, 1, payload, (int)sizeof(payload));
            if (r < 0) return r;
            have_data = 1;
        }
    }
    if (nh < 0 || !have_data) return GW_ERR_FORMAT;
    {
        const char* m = hdr_find(h, nh, ":method");
        const char* ct = hdr_find(h, nh, "content-type");
        const char* pa = hdr_find(h, nh, ":path");
        if (!m || !g_eq(m, "POST") || !ct || !g_eq(ct, "application/grpc") || !pa) return GW_ERR_FORMAT;
        g_copy(path, pa, path_cap);
    }
    reply[0] = 0;
    gst = handler(path, payload, reply, (int)sizeof(reply));
    kv[0] = ":status"; kv[1] = "200"; kv[2] = "content-type"; kv[3] = "application/grpc";
    n = hdr_block(hb, (int)sizeof(hb), kv, 2);
    if (n < 0) return n;
    o = gw_h2_frame_put(out, cap, GW_H2_HEADERS, GW_H2_FLAG_END_HEADERS, 1U, hb, n);
    if (o < 0) return o;
    if (gst == 0) {
        n = data_frame(out + o, cap - o, 0, reply);
        if (n < 0) return n;
        o += n;
    }
    if (gst < 0 || gst > 16) gst = 13;                    /* INTERNAL */
    if (gst >= 10) { status[0] = (char)('0' + gst / 10); status[1] = (char)('0' + gst % 10); status[2] = 0; }
    else { status[0] = (char)('0' + gst); status[1] = 0; }
    kv[0] = "grpc-status"; kv[1] = status;
    kv[2] = "grpc-message"; kv[3] = gst ? reply : "";
    n = hdr_block(hb, (int)sizeof(hb), kv, gst ? 2 : 1);
    if (n < 0) return n;
    n = gw_h2_frame_put(out + o, cap - o, GW_H2_HEADERS, GW_H2_FLAG_END_HEADERS | GW_H2_FLAG_END_STREAM,
                        1U, hb, n);
    return n < 0 ? n : o + n;
}

int gw_parse_response(const unsigned char* in, int len, int* http_status, int* grpc_status,
                      char* message, int cap, int* frames) {
    int pos = 0, end_stream = 0;
    *http_status = -1; *grpc_status = -1; *frames = 0;
    if (cap > 0) message[0] = 0;
    while (pos < len) {
        gw_header_t h[8];
        int type, flags, plen, used, nh, i;
        unsigned int stream;
        const unsigned char* p;
        used = gw_h2_frame_get(in + pos, len - pos, &type, &flags, &stream, &p, &plen);
        if (used < 0) return used;
        pos += used;
        (*frames)++;
        if (flags & GW_H2_FLAG_END_STREAM) end_stream = 1;
        if (type == GW_H2_HEADERS) {
            nh = gw_hpack_decode(p, plen, h, 8);
            if (nh < 0) return nh;
            for (i = 0; i < nh; i++) {
                const char* v = h[i].value;
                int x = 0, k = 0;
                while (v[k] >= '0' && v[k] <= '9') x = x * 10 + (v[k++] - '0');
                if (g_eq(h[i].name, ":status")) *http_status = x;
                else if (g_eq(h[i].name, "grpc-status")) *grpc_status = x;
                else if (g_eq(h[i].name, "grpc-message") && v[0]) g_copy(message, v, cap);
            }
        } else if (type == GW_H2_DATA) {
            const unsigned char* msg;
            int mlen, r = gw_lpm_get(p, plen, &msg, &mlen);
            if (r < 0) return r;
            r = gw_pb_get_string(msg, mlen, 1, message, cap);
            if (r < 0) return r;
        }
    }
    if (!end_stream || *http_status < 0 || *grpc_status < 0) return GW_ERR_FORMAT;
    return GW_OK;
}

/* ---------------------------------------------- connection level (Lot 6) */
static const char k_preface[] = "PRI * HTTP/2.0\r\n\r\nSM\r\n\r\n";
#define GW_PREFACE_LEN 24
#define GW_H2_FLAG_ACK 0x1

int gw_client_stream(unsigned char* out, int cap, const char* service, const char* method,
                     const char* payload) {
    int p = gw_h2_preface(out, cap), n;
    if (p < 0) return p;
    n = gw_build_request(out + p, cap - p, service, method, payload);
    return n < 0 ? n : p + n;
}

static int preface_check(const unsigned char* in, int len) {
    int i;
    for (i = 0; i < GW_PREFACE_LEN && i < len; i++)
        if (in[i] != (unsigned char)k_preface[i]) return GW_ERR_FORMAT;
    return len < GW_PREFACE_LEN ? 0 : GW_PREFACE_LEN;
}

int gw_stream_complete(const unsigned char* in, int len, int expect_preface) {
    int pos = 0;
    if (expect_preface) {
        pos = preface_check(in, len);
        if (pos <= 0) return pos;
    }
    while (pos < len) {
        int type, flags, plen, used;
        unsigned int stream;
        const unsigned char* p;
        used = gw_h2_frame_get(in + pos, len - pos, &type, &flags, &stream, &p, &plen);
        if (used == GW_ERR_SHORT) return 0;
        if (used < 0) return used;
        pos += used;
        if (stream == 1U && (flags & GW_H2_FLAG_END_STREAM)) return 1;
    }
    return 0;
}

/* Splits stream-0 control frames from stream-1 frames. Returns the stream-1
 * byte count copied to s1 (cap), counts SETTINGS (non ACK) and ACKs. */
static int split_streams(const unsigned char* in, int len, unsigned char* s1, int cap,
                         int* settings, int* acks) {
    int pos = 0, o = 0;
    *settings = 0; *acks = 0;
    while (pos < len) {
        int type, flags, plen, used;
        unsigned int stream;
        const unsigned char* p;
        used = gw_h2_frame_get(in + pos, len - pos, &type, &flags, &stream, &p, &plen);
        if (used < 0) return used;
        if (stream == 0U) {
            if (type == GW_H2_SETTINGS) {
                if (flags & GW_H2_FLAG_ACK) (*acks)++; else (*settings)++;
            }
        } else if (stream == 1U) {
            if (o + used > cap) return GW_ERR_LIMIT;
            g_put(s1 + o, cap - o, 0, in + pos, used);
            o += used;
        } else {
            return GW_ERR_FORMAT;
        }
        pos += used;
    }
    return o;
}

int gw_serve_stream(const unsigned char* in, int len, gw_handler_t handler, unsigned char* out,
                    int cap, char* path, int path_cap, int* client_settings) {
    unsigned char s1[512];
    int pre = preface_check(in, len), n, acks, o, m;
    *client_settings = 0;
    if (pre == 0) return GW_ERR_SHORT;
    if (pre < 0) return pre;
    n = split_streams(in + pre, len - pre, s1, (int)sizeof(s1), client_settings, &acks);
    if (n < 0) return n;
    if (*client_settings < 1) return GW_ERR_FORMAT;   /* RFC 9113 3.4: SETTINGS first */
    o = gw_h2_frame_put(out, cap, GW_H2_SETTINGS, 0, 0U, 0, 0);
    if (o < 0) return o;
    m = gw_h2_frame_put(out + o, cap - o, GW_H2_SETTINGS, GW_H2_FLAG_ACK, 0U, 0, 0);
    if (m < 0) return m;
    o += m;
    m = gw_serve(s1, n, handler, out + o, cap - o, path, path_cap);
    return m < 0 ? m : o + m;
}

int gw_parse_stream_response(const unsigned char* in, int len, int* http_status, int* grpc_status,
                             char* message, int cap, int* frames, int* server_settings,
                             int* settings_acks) {
    unsigned char s1[512];
    int n = split_streams(in, len, s1, (int)sizeof(s1), server_settings, settings_acks);
    if (n < 0) return n;
    return gw_parse_response(s1, n, http_status, grpc_status, message, cap, frames);
}
