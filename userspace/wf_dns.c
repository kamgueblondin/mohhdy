/* wf_dns.c - Lot 67: DNS A query/answer codec (see wf_dns.h). */
#include "wf_dns.h"

static char lc(char c) { return (c >= 'A' && c <= 'Z') ? (char)(c + 32) : c; }

int wf_ipv4_parse(const char* s, uint8_t ip[4]) {
    int part = 0, val = -1, i, digits = 0;
    for (i = 0;; i++) {
        char c = s[i];
        if (c >= '0' && c <= '9') {
            if (++digits > 3) return -1;
            val = (val < 0 ? 0 : val) * 10 + (c - '0');
            if (val > 255) return -1;
        } else if (c == '.' || c == 0) {
            if (val < 0 || part > 3) return -1;
            ip[part++] = (uint8_t)val;
            val = -1; digits = 0;
            if (c == 0) break;
        } else {
            return -1;
        }
    }
    return part == 4 ? 0 : -1;
}

int wf_dns_build_query(const char* host, uint16_t id, uint8_t* o, int cap) {
    int p = 12, lab, i = 0;
    if (cap < 12 + 4 + 2) return -1;
    o[0] = (uint8_t)(id >> 8); o[1] = (uint8_t)id;
    o[2] = 0x01; o[3] = 0x00;            /* RD */
    o[4] = 0; o[5] = 1; o[6] = 0; o[7] = 0; o[8] = 0; o[9] = 0; o[10] = 0; o[11] = 0;
    while (host[i]) {
        int n = 0;
        lab = p++;
        while (host[i] && host[i] != '.') {
            if (n >= 63 || p >= cap - 6) return -1;
            o[p++] = (uint8_t)lc(host[i++]);
            n++;
        }
        if (n == 0) return -1;
        o[lab] = (uint8_t)n;
        if (host[i] == '.') { i++; if (!host[i]) break; }
    }
    if (p == 12 || p > 12 + 255) return -1;
    o[p++] = 0;
    o[p++] = 0; o[p++] = 1;   /* A */
    o[p++] = 0; o[p++] = 1;   /* IN */
    return p;
}

/* Reads a (possibly compressed) name at *pos into out (lowercase dotted). */
static int read_name(const uint8_t* m, int len, int* pos, char* out, int cap) {
    int p = *pos, o = 0, jumped = 0, hops = 0;
    for (;;) {
        int l;
        if (p >= len) return -1;
        l = m[p];
        if ((l & 0xc0) == 0xc0) {
            if (p + 1 >= len || ++hops > 16) return -1;
            if (!jumped) *pos = p + 2;
            p = ((l & 0x3f) << 8) | m[p + 1];
            jumped = 1;
            continue;
        }
        if (l & 0xc0) return -1;
        p++;
        if (l == 0) break;
        if (p + l > len) return -1;
        if (o && o < cap - 1) out[o++] = '.';
        while (l-- > 0) {
            if (o >= cap - 1) return -1;
            out[o++] = lc((char)m[p++]);
        }
    }
    if (!jumped) *pos = p;
    out[o] = 0;
    return 0;
}

static int name_eq(const char* a, const char* b) {
    int i = 0;
    while (a[i] && b[i]) { if (lc(a[i]) != lc(b[i])) return 0; i++; }
    if (b[i] == '.' && !b[i + 1]) return !a[i];
    return a[i] == b[i];
}

int wf_dns_parse_a(const uint8_t* m, int len, uint16_t id, const char* host, uint8_t ip[4]) {
    static char name[256], want[256];
    int qd, an, p = 12, i, k, rcode;
    if (len < 12) return -1;
    if (((m[0] << 8) | m[1]) != id) return -1;
    if (!(m[2] & 0x80)) return -1;
    rcode = m[3] & 0x0f;
    if (rcode != 0) return -2;
    qd = (m[4] << 8) | m[5];
    an = (m[6] << 8) | m[7];
    if (qd != 1) return -1;
    if (read_name(m, len, &p, name, (int)sizeof(name)) != 0 || p + 4 > len) return -1;
    if (!name_eq(name, host)) return -1;
    p += 4;
    for (k = 0; host[k] && k < 254; k++) want[k] = lc(host[k]);
    if (k > 0 && want[k - 1] == '.') k--;
    want[k] = 0;
    /* CNAME chains: rescan the answers when the wanted name changes */
    for (k = 0; k < 8; k++) {
        int q = p, changed = 0;
        for (i = 0; i < an; i++) {
            int type, cls, rdl;
            if (read_name(m, len, &q, name, (int)sizeof(name)) != 0 || q + 10 > len) return -1;
            type = (m[q] << 8) | m[q + 1];
            cls = (m[q + 2] << 8) | m[q + 3];
            rdl = (m[q + 8] << 8) | m[q + 9];
            q += 10;
            if (q + rdl > len) return -1;
            if (cls == 1 && name_eq(name, want)) {
                if (type == 1 && rdl == 4) {
                    ip[0] = m[q]; ip[1] = m[q + 1]; ip[2] = m[q + 2]; ip[3] = m[q + 3];
                    return 0;
                }
                if (type == 5) {
                    int r = q;
                    if (read_name(m, len, &r, want, (int)sizeof(want)) != 0) return -1;
                    changed = 1;
                }
            }
            q += rdl;
        }
        if (!changed) break;
    }
    return -3;
}
