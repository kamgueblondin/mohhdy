/* wf_http.c - Lot 67: URL and HTTP/1.1 response parsing (see wf_http.h). */
#include "wf_http.h"

static int slen(const char* s) { int n = 0; while (s[n]) n++; return n; }
static char lc(char c) { return (c >= 'A' && c <= 'Z') ? (char)(c + 32) : c; }

static int put(char* o, int cap, int p, const char* s) {
    while (*s) { if (p >= cap - 1) return -1; o[p++] = *s++; }
    o[p] = 0;
    return p;
}

static int put_int(char* o, int cap, int p, int v) {
    char t[12]; int n = 0;
    if (v == 0) t[n++] = '0';
    while (v > 0 && n < 11) { t[n++] = (char)('0' + v % 10); v /= 10; }
    while (n > 0) { if (p >= cap - 1) return -1; o[p++] = t[--n]; }
    o[p] = 0;
    return p;
}

static int prefix_ci(const char* s, const char* pre) {
    int i;
    for (i = 0; pre[i]; i++) if (lc(s[i]) != pre[i]) return 0;
    return 1;
}

int wf_url_parse(const char* s, wf_url_t* u) {
    int i = 0, h = 0, p = 0, port = -1;
    if (!s || !u) return -1;
    while (*s == ' ' || *s == '\t') s++;
    u->https = 1;
    if (prefix_ci(s, "https://")) { s += 8; }
    else if (prefix_ci(s, "http://")) { s += 7; u->https = 0; }
    else {
        for (i = 0; s[i] && s[i] != '/' && s[i] != '?'; i++)
            if (s[i] == ':' && s[i + 1] == '/' && s[i + 2] == '/') return -1;   /* other scheme */
    }
    for (i = 0; s[i] && s[i] != '/' && s[i] != '?' && s[i] != '#' && s[i] != ':'; i++) {
        char c = lc(s[i]);
        if (c == '@' || c == '[' || c == ' ' || c == '\\') return -1;
        if (!((c >= 'a' && c <= 'z') || (c >= '0' && c <= '9') || c == '-' || c == '.' || c == '_')) return -1;
        if (h >= WF_URL_HOST_MAX - 1) return -1;
        u->host[h++] = c;
    }
    u->host[h] = 0;
    if (h == 0) return -1;
    if (s[i] == ':') {
        i++;
        port = 0;
        if (s[i] < '0' || s[i] > '9') return -1;
        while (s[i] >= '0' && s[i] <= '9') { port = port * 10 + (s[i] - '0'); if (port > 65535) return -1; i++; }
        if (port == 0) return -1;
        if (s[i] && s[i] != '/' && s[i] != '?' && s[i] != '#') return -1;
    }

    u->port = port > 0 ? port : (u->https ? 443 : 80);
    if (s[i] != '/') u->path[p++] = '/';
    for (; s[i] && s[i] != '#'; i++) {
        char c = s[i];
        if (c == ' ' || c == '\r' || c == '\n' || c == '\t') return -1;   /* no request splitting */
        if ((unsigned char)c < 0x20U || (unsigned char)c >= 0x7fU) return -1;
        if (p >= WF_URL_PATH_MAX - 1) return -1;
        u->path[p++] = c;
    }
    u->path[p] = 0;
    return 0;
}

int wf_url_format(const wf_url_t* u, char* o, int cap) {
    int p = 0;
    if (cap < 1) return 0;
    o[0] = 0;
    p = put(o, cap, p, u->https ? "https://" : "http://");
    if (p >= 0) p = put(o, cap, p, u->host);
    if (p >= 0 && u->port != (u->https ? 443 : 80)) { p = put(o, cap, p, ":"); if (p >= 0) p = put_int(o, cap, p, u->port); }
    if (p >= 0) p = put(o, cap, p, u->path);
    return p < 0 ? slen(o) : p;
}

int wf_url_resolve(const wf_url_t* b, const char* loc, wf_url_t* out) {
    static char tmp[WF_URL_HOST_MAX + WF_URL_PATH_MAX + 16];
    int p = 0, i, last = 0;
    if (!loc || !loc[0]) return -1;
    while (*loc == ' ') loc++;
    if (prefix_ci(loc, "http://") || prefix_ci(loc, "https://")) return wf_url_parse(loc, out);
    if (loc[0] == '/' && loc[1] == '/') {
        p = put(tmp, (int)sizeof(tmp), 0, b->https ? "https:" : "http:");
        if (p < 0 || put(tmp, (int)sizeof(tmp), p, loc) < 0) return -1;
        return wf_url_parse(tmp, out);
    }
    *out = *b;
    if (loc[0] == '/') {
        p = 0;
    } else {
        /* relative: keep the base directory (up to the last '/', before any query) */
        for (i = 0; b->path[i] && b->path[i] != '?'; i++) if (b->path[i] == '/') last = i;
        p = last + 1;
    }
    for (i = 0; loc[i] && loc[i] != '#'; i++) {
        char c = loc[i];
        if ((unsigned char)c <= 0x20U || (unsigned char)c >= 0x7fU) return -1;
        if (p >= WF_URL_PATH_MAX - 1) return -1;
        out->path[p++] = c;
    }
    out->path[p] = 0;
    if (out->path[0] != '/') return -1;
    return 0;
}

int wf_http_build_get(const wf_url_t* u, char* o, int cap) {
    int p = 0;
    p = put(o, cap, p, "GET ");
    if (p >= 0) p = put(o, cap, p, u->path);
    if (p >= 0) p = put(o, cap, p, " HTTP/1.1\r\nHost: ");
    if (p >= 0) p = put(o, cap, p, u->host);
    if (p >= 0 && u->port != (u->https ? 443 : 80)) { p = put(o, cap, p, ":"); if (p >= 0) p = put_int(o, cap, p, u->port); }
    if (p >= 0) p = put(o, cap, p, "\r\nUser-Agent: Mohhdy-fetch/1.0\r\nAccept: text/html,text/plain,*/*\r\n"
                                   "Accept-Encoding: identity\r\nConnection: close\r\n\r\n");
    return p;
}

/* ---------------------------------------------------------------- response */

#define ST_HEAD 0
#define ST_BODY_LEN 1
#define ST_BODY_CLOSE 2
#define ST_CHUNK_SIZE 3
#define ST_CHUNK_DATA 4
#define ST_CHUNK_CRLF 5
#define ST_TRAILER 6
#define ST_DONE 7

void wf_http_resp_init(wf_http_resp_t* r, uint8_t* body, int cap) {
    uint8_t* z = (uint8_t*)r;
    int i;
    for (i = 0; i < (int)sizeof(*r); i++) z[i] = 0;
    r->body = body;
    r->body_cap = cap;
    r->content_length = -1;
}

static int fail(wf_http_resp_t* r, const char* why) {
    r->error = -1;
    r->error_text = why;
    return -1;
}

static void body_put(wf_http_resp_t* r, const uint8_t* d, int n) {
    int i;
    for (i = 0; i < n; i++) {
        if (r->body_len < r->body_cap) r->body[r->body_len++] = d[i];
        else r->truncated = 1;
    }
    r->total_body += n;
}

static void copy_val(const char* v, int n, char* out, int cap) {
    int i, k = 0;
    while (n > 0 && (*v == ' ' || *v == '\t')) { v++; n--; }
    while (n > 0 && (v[n - 1] == ' ' || v[n - 1] == '\t')) n--;
    for (i = 0; i < n && k < cap - 1; i++) out[k++] = v[i];
    out[k] = 0;
}

static int parse_head(wf_http_resp_t* r) {
    char* h = r->hdr;
    int n = r->hdr_len, p = 0, i, saw_len = 0;
    /* status line */
    if (n < 12 || !(h[0] == 'H' && h[1] == 'T' && h[2] == 'T' && h[3] == 'P' && h[4] == '/' && h[5] == '1' && h[6] == '.'))
        return fail(r, "not an HTTP/1.x response");
    if (h[8] != ' ' || h[9] < '1' || h[9] > '5' || h[10] < '0' || h[10] > '9' || h[11] < '0' || h[11] > '9')
        return fail(r, "bad status line");
    r->status = (h[9] - '0') * 100 + (h[10] - '0') * 10 + (h[11] - '0');
    for (p = 12; p < n && h[p] != '\n'; p++) {}
    {
        int s = 13 < p ? 13 : p, e = p;
        if (e > s && h[e - 1] == '\r') e--;
        copy_val(h + s, e - s, r->reason, (int)sizeof(r->reason));
    }
    p++;
    while (p < n) {
        int ls = p, le, colon = -1;
        for (le = p; le < n && h[le] != '\n'; le++) {}
        p = le + 1;
        if (le > ls && h[le - 1] == '\r') le--;
        if (le == ls) break;
        for (i = ls; i < le; i++) if (h[i] == ':') { colon = i; break; }
        if (colon <= ls) return fail(r, "bad header line");
        {
            int nl = colon - ls;
            const char* name = h + ls;
            const char* val = h + colon + 1;
            int vl = le - colon - 1;
            if (nl == 14 && prefix_ci(name, "content-length")) {
                long v = 0;
                char tmp[24];
                copy_val(val, vl, tmp, (int)sizeof(tmp));
                if (!tmp[0]) return fail(r, "bad Content-Length");
                for (i = 0; tmp[i]; i++) {
                    if (tmp[i] < '0' || tmp[i] > '9' || v > 100000000L) return fail(r, "bad Content-Length");
                    v = v * 10 + (tmp[i] - '0');
                }
                if (saw_len && v != r->content_length) return fail(r, "conflicting Content-Length");
                r->content_length = v;
                saw_len = 1;
            } else if (nl == 17 && prefix_ci(name, "transfer-encoding")) {
                char tmp[64];
                int k;
                copy_val(val, vl, tmp, (int)sizeof(tmp));
                for (k = 0; tmp[k]; k++) tmp[k] = lc(tmp[k]);
                k = slen(tmp);
                if (k >= 7 && tmp[k - 7] == 'c' && prefix_ci(tmp + k - 7, "chunked")) r->chunked = 1;
                else if (!(k == 8 && prefix_ci(tmp, "identity"))) return fail(r, "unsupported Transfer-Encoding");
            } else if (nl == 8 && prefix_ci(name, "location")) {
                copy_val(val, vl, r->location, (int)sizeof(r->location));
            } else if (nl == 12 && prefix_ci(name, "content-type")) {
                copy_val(val, vl, r->content_type, (int)sizeof(r->content_type));
            } else if (nl == 16 && prefix_ci(name, "content-encoding")) {
                char tmp[32];
                copy_val(val, vl, tmp, (int)sizeof(tmp));
                if (!(prefix_ci(tmp, "identity") && slen(tmp) == 8)) return fail(r, "compressed body not supported");
            }
        }
    }
    if (r->chunked) { r->content_length = -1; r->state = ST_CHUNK_SIZE; r->line_len = 0; }
    else if (r->status == 204 || r->status == 304 || (r->status >= 100 && r->status < 200)) r->state = ST_DONE;
    else if (r->content_length >= 0) { r->remaining = r->content_length; r->state = r->remaining ? ST_BODY_LEN : ST_DONE; }
    else r->state = ST_BODY_CLOSE;
    return 0;
}

int wf_http_feed(wf_http_resp_t* r, const uint8_t* d, int n) {
    int i = 0;
    if (r->error) return r->error;
    while (i < n && r->state != ST_DONE) {
        if (r->state == ST_HEAD) {
            char c = (char)d[i++];
            if (r->hdr_len >= WF_HTTP_HDR_MAX - 1) return fail(r, "headers too large");
            r->hdr[r->hdr_len++] = c;
            r->hdr[r->hdr_len] = 0;
            if (c == '\n' && r->hdr_len >= 2 &&
                ((r->hdr_len >= 4 && r->hdr[r->hdr_len - 2] == '\r' && r->hdr[r->hdr_len - 3] == '\n') ||
                 r->hdr[r->hdr_len - 2] == '\n')) {
                if (parse_head(r) != 0) return -1;
                /* 1xx interim: start over */
                if (r->status >= 100 && r->status < 200) { r->hdr_len = 0; r->state = ST_HEAD; }
            }
        } else if (r->state == ST_BODY_LEN) {
            int k = n - i;
            if (k > r->remaining) k = (int)r->remaining;
            body_put(r, d + i, k);
            i += k;
            r->remaining -= k;
            if (r->remaining == 0) r->state = ST_DONE;
        } else if (r->state == ST_BODY_CLOSE) {
            body_put(r, d + i, n - i);
            i = n;
        } else if (r->state == ST_CHUNK_SIZE || r->state == ST_CHUNK_CRLF || r->state == ST_TRAILER) {
            char c = (char)d[i++];
            if (c != '\n') {
                if (r->line_len >= (int)sizeof(r->line) - 1) return fail(r, "chunk line too long");
                r->line[r->line_len++] = c;
                continue;
            }
            if (r->line_len > 0 && r->line[r->line_len - 1] == '\r') r->line_len--;
            r->line[r->line_len] = 0;
            if (r->state == ST_CHUNK_CRLF) {
                if (r->line_len != 0) return fail(r, "missing CRLF after chunk");
                r->state = ST_CHUNK_SIZE;
            } else if (r->state == ST_TRAILER) {
                if (r->line_len == 0) r->state = ST_DONE;
            } else {
                long v = 0;
                int k, digits = 0;
                for (k = 0; k < r->line_len && r->line[k] != ';'; k++) {
                    char h = lc(r->line[k]);
                    int x;
                    if (h == ' ' || h == '\t') continue;
                    if (h >= '0' && h <= '9') x = h - '0';
                    else if (h >= 'a' && h <= 'f') x = h - 'a' + 10;
                    else return fail(r, "bad chunk size");
                    if (v > 0x1000000L) return fail(r, "chunk too large");
                    v = v * 16 + x;
                    digits++;
                }
                if (!digits) return fail(r, "bad chunk size");
                if (v == 0) r->state = ST_TRAILER;
                else { r->remaining = v; r->state = ST_CHUNK_DATA; }
            }
            r->line_len = 0;
        } else if (r->state == ST_CHUNK_DATA) {
            int k = n - i;
            if (k > r->remaining) k = (int)r->remaining;
            body_put(r, d + i, k);
            i += k;
            r->remaining -= k;
            if (r->remaining == 0) { r->state = ST_CHUNK_CRLF; r->line_len = 0; }
        }
    }
    if (r->state == ST_DONE) { r->done = 1; return 1; }
    return 0;
}

int wf_http_eof(wf_http_resp_t* r) {
    if (r->error) return r->error;
    if (r->state == ST_DONE) { r->done = 1; return 1; }
    if (r->state == ST_BODY_CLOSE) { r->state = ST_DONE; r->done = 1; return 1; }
    if (r->state == ST_HEAD) return fail(r, "connection closed before the response headers");
    return fail(r, "connection closed before the end of the body");
}
