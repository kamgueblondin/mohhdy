/* wf_idna.c - Lot 89: host names in UTF-8 and in their ASCII form, see
 * wf_idna.h. */
#include "wf_idna.h"

#define PB 36U
#define PTMIN 1U
#define PTMAX 26U
#define PSKEW 38U
#define PDAMP 700U
#define PBIAS 72U
#define PN0 128U

static int adapt(uint32_t delta, uint32_t npoints, int first) {
    uint32_t k = 0;
    delta = first ? delta / PDAMP : delta / 2U;
    delta += delta / npoints;
    while (delta > ((PB - PTMIN) * PTMAX) / 2U) {
        delta /= PB - PTMIN;
        k += PB;
    }
    return (int)(k + (PB - PTMIN + 1U) * delta / (delta + PSKEW));
}

static char digit_char(uint32_t d) {
    return (char)(d < 26U ? 'a' + d : '0' + (d - 26U));
}

static int digit_val(char c) {
    if (c >= 'a' && c <= 'z') return c - 'a';
    if (c >= 'A' && c <= 'Z') return c - 'A';
    if (c >= '0' && c <= '9') return c - '0' + 26;
    return -1;
}

int wf_puny_encode(const uint32_t *cp, int n, char *out, int cap) {
    uint32_t m, q, k, t, delta = 0, nn = PN0;
    int i, len = 0, b, h, bias = (int)PBIAS;
    if (!cp || !out || n < 0 || cap <= 0) return -1;
    for (i = 0; i < n; i++) {
        if (cp[i] < 0x80U) {
            if (len >= cap - 1) return -1;
            out[len++] = (char)cp[i];
        }
    }
    b = h = len;
    if (b > 0) {
        if (len >= cap - 1) return -1;
        out[len++] = '-';
    }
    while (h < n) {
        m = 0xFFFFFFFFU;
        for (i = 0; i < n; i++) if (cp[i] >= nn && cp[i] < m) m = cp[i];
        if (m - nn > (0xFFFFFFFFU - delta) / (uint32_t)(h + 1)) return -1;
        delta += (m - nn) * (uint32_t)(h + 1);
        nn = m;
        for (i = 0; i < n; i++) {
            if (cp[i] < nn) {
                if (++delta == 0U) return -1;
            }
            if (cp[i] == nn) {
                q = delta;
                for (k = PB;; k += PB) {
                    t = k <= (uint32_t)bias ? PTMIN : (k >= (uint32_t)bias + PTMAX ? PTMAX : k - (uint32_t)bias);
                    if (q < t) break;
                    if (len >= cap - 1) return -1;
                    out[len++] = digit_char(t + (q - t) % (PB - t));
                    q = (q - t) / (PB - t);
                }
                if (len >= cap - 1) return -1;
                out[len++] = digit_char(q);
                bias = adapt(delta, (uint32_t)(h + 1), h == b);
                delta = 0;
                h++;
            }
        }
        delta++;
        nn++;
    }
    out[len] = 0;
    return len;
}

int wf_puny_decode(const char *in, int n, uint32_t *cp, int cap) {
    uint32_t nn = PN0, i = 0, oldi, w, k, t;
    int b = 0, j, out = 0, bias = (int)PBIAS, in_pos;
    if (!in || !cp || n < 0 || cap <= 0) return -1;
    for (j = 0; j < n; j++) if (in[j] == '-') b = j;
    for (j = 0; j < b; j++) {
        if ((unsigned char)in[j] >= 0x80U || out >= cap) return -1;
        cp[out++] = (uint32_t)(unsigned char)in[j];
    }
    in_pos = b > 0 ? b + 1 : 0;
    while (in_pos < n) {
        oldi = i;
        w = 1;
        for (k = PB;; k += PB) {
            int dv;
            if (in_pos >= n) return -1;
            dv = digit_val(in[in_pos++]);
            if (dv < 0) return -1;
            if ((uint32_t)dv > (0xFFFFFFFFU - i) / w) return -1;
            i += (uint32_t)dv * w;
            t = k <= (uint32_t)bias ? PTMIN : (k >= (uint32_t)bias + PTMAX ? PTMAX : k - (uint32_t)bias);
            if ((uint32_t)dv < t) break;
            if (w > 0xFFFFFFFFU / (PB - t)) return -1;
            w *= PB - t;
        }
        bias = adapt(i - oldi, (uint32_t)(out + 1), oldi == 0);
        if (i / (uint32_t)(out + 1) > 0x10FFFFU - nn) return -1;
        nn += i / (uint32_t)(out + 1);
        i %= (uint32_t)(out + 1);
        if (nn < 0x80U || (nn >= 0xD800U && nn <= 0xDFFFU) || out >= cap) return -1;
        for (j = out; j > (int)i; j--) cp[j] = cp[j - 1];
        cp[i++] = nn;
        out++;
    }
    return out;
}

/* ---- UTF-8 ------------------------------------------------------------ */

/* one code point at s[*i] (n bytes); -1 on bad, overlong or surrogate */
static int32_t utf8_next(const unsigned char *s, int n, int *i) {
    uint32_t c = s[*i], v;
    int need, k;
    if (c < 0x80U) { (*i)++; return (int32_t)c; }
    if (c >= 0xC2U && c <= 0xDFU) { need = 1; v = c & 0x1FU; }
    else if (c >= 0xE0U && c <= 0xEFU) { need = 2; v = c & 0x0FU; }
    else if (c >= 0xF0U && c <= 0xF4U) { need = 3; v = c & 0x07U; }
    else return -1;
    if (*i + need >= n) return -1;
    for (k = 1; k <= need; k++) {
        uint32_t d = s[*i + k];
        if ((d & 0xC0U) != 0x80U) return -1;
        v = (v << 6) | (d & 0x3FU);
    }
    if ((need == 2 && v < 0x800U) || (need == 3 && (v < 0x10000U || v > 0x10FFFFU))) return -1;
    if (v >= 0xD800U && v <= 0xDFFFU) return -1;
    *i += need + 1;
    return (int32_t)v;
}

static int utf8_put(uint32_t c, char *out, int cap, int len) {
    if (c < 0x80U) {
        if (len + 1 >= cap) return -1;
        out[len++] = (char)c;
    } else if (c < 0x800U) {
        if (len + 2 >= cap) return -1;
        out[len++] = (char)(0xC0U | (c >> 6));
        out[len++] = (char)(0x80U | (c & 0x3FU));
    } else if (c < 0x10000U) {
        if (len + 3 >= cap) return -1;
        out[len++] = (char)(0xE0U | (c >> 12));
        out[len++] = (char)(0x80U | ((c >> 6) & 0x3FU));
        out[len++] = (char)(0x80U | (c & 0x3FU));
    } else {
        if (len + 4 >= cap) return -1;
        out[len++] = (char)(0xF0U | (c >> 18));
        out[len++] = (char)(0x80U | ((c >> 12) & 0x3FU));
        out[len++] = (char)(0x80U | ((c >> 6) & 0x3FU));
        out[len++] = (char)(0x80U | (c & 0x3FU));
    }
    return len;
}

/* the small case map: upper case to lower case in the blocks named in
 * wf_idna.h; other code points are returned unchanged */
static uint32_t fold(uint32_t c) {
    if (c >= 'A' && c <= 'Z') return c + 32U;
    if (c >= 0xC0U && c <= 0xDEU && c != 0xD7U) return c + 32U;          /* Latin-1 */
    if (c == 0x130U) return c;                                            /* I with dot: refused by cp_ok */
    if (c >= 0x100U && c <= 0x137U) return c | 1U;                        /* A-macron .. k-cedilla: pairs */
    if (c >= 0x139U && c <= 0x148U) return (c & 1U) ? c + 1U : c;         /* L-acute .. n-caron: odd upper */
    if (c >= 0x14AU && c <= 0x177U) return c | 1U;                        /* ENG .. y-circumflex: pairs */
    if (c == 0x178U) return 0xFFU;                                        /* Y-diaeresis */
    if (c >= 0x179U && c <= 0x17EU) return (c & 1U) ? c + 1U : c;         /* Z-acute .. z-caron */
    if (c == 0x386U) return 0x3ACU;                                       /* Greek tonos capitals */
    if (c >= 0x388U && c <= 0x38AU) return c + 37U;
    if (c == 0x38CU) return 0x3CCU;
    if (c == 0x38EU || c == 0x38FU) return c + 63U;
    if (c >= 0x391U && c <= 0x3ABU && c != 0x3A2U) return c + 32U;        /* Greek capitals */
    if (c >= 0x400U && c <= 0x40FU) return c + 80U;                       /* Cyrillic Ie-grave .. Dzhe */
    if (c >= 0x410U && c <= 0x42FU) return c + 32U;                       /* Cyrillic A .. Ya */
    return c;
}

/* code points a host label may hold: a-z 0-9 '-' and anything from
 * U+00A0 up except the Latin-1 punctuation and symbols, the
 * multiplication and division signs, and non-characters */
static int cp_ok(uint32_t c) {
    if (c < 0x80U) return (c >= 'a' && c <= 'z') || (c >= '0' && c <= '9') || c == '-';
    if (c < 0xC0U) return c == 0xAAU || c == 0xB5U || c == 0xBAU;         /* feminine/masculine ordinals, micro */
    if (c == 0xD7U || c == 0xF7U || c == 0x130U) return 0;   /* x, divide, I with dot (maps to two code points) */
    if (c >= 0xFDD0U && c <= 0xFDEFU) return 0;
    if ((c & 0xFFFEU) == 0xFFFEU) return 0;
    if (c >= 0x2000U && c <= 0x206FU) return 0;                           /* general punctuation, spaces, joiners */
    if (c == 0x3000U || c == 0xFEFFU) return 0;                           /* ideographic space, BOM */
    return 1;
}

static int is_dot(uint32_t c) {
    return c == '.' || c == 0x3002U || c == 0xFF0EU || c == 0xFF61U;
}

static int s_len(const char *s) { int n = 0; while (s && s[n]) n++; return n; }

int wf_idna_needs_map(const char *host) {
    int i;
    if (!host) return 0;
    for (i = 0; host[i]; i++) {
        unsigned char c = (unsigned char)host[i];
        if (c >= 0x80U || (c >= 'A' && c <= 'Z')) return 1;
    }
    return 0;
}

/* one label of code points to ASCII at out+len; returns the new length */
static int put_label(const uint32_t *cp, int ncp, char *out, int cap, int len) {
    int i, ascii = 1, start = len, w;
    if (ncp <= 0) return -1;
    for (i = 0; i < ncp; i++) if (cp[i] >= 0x80U) ascii = 0;
    if (ascii) {
        if (ncp > WF_IDNA_LABEL || len + ncp >= cap) return -1;
        for (i = 0; i < ncp; i++) out[len++] = (char)cp[i];
        return len;
    }
    if (len + 4 >= cap) return -1;
    out[len++] = 'x'; out[len++] = 'n'; out[len++] = '-'; out[len++] = '-';
    w = wf_puny_encode(cp, ncp, out + len, cap - len);
    if (w < 0) return -1;
    len += w;
    if (len - start > WF_IDNA_LABEL) return -1;
    return len;
}

int wf_idna_to_ascii(const char *host, char *out, int cap) {
    const unsigned char *s = (const unsigned char *)host;
    uint32_t cp[WF_IDNA_CPS];
    int n, i = 0, ncp = 0, len = 0;
    if (!host || !out || cap <= 0) return -1;
    out[0] = 0;
    n = s_len(host);
    if (n == 0 || n > 4 * WF_IDNA_MAX) return -1;
    while (i <= n) {
        int32_t c;
        int end = i == n;
        if (end) c = '.';
        else {
            c = utf8_next(s, n, &i);
            if (c < 0) { out[0] = 0; return -1; }
        }
        if (is_dot((uint32_t)c)) {
            /* a dot at the very end leaves an empty last label: refused */
            len = put_label(cp, ncp, out, cap, len);
            if (len < 0) { out[0] = 0; return -1; }
            ncp = 0;
            if (end) break;
            if (len + 1 >= cap) { out[0] = 0; return -1; }
            out[len++] = '.';
            continue;
        }
        c = (int32_t)fold((uint32_t)c);
        if (!cp_ok((uint32_t)c) || ncp >= WF_IDNA_CPS) { out[0] = 0; return -1; }
        cp[ncp++] = (uint32_t)c;
    }
    if (len > WF_IDNA_MAX) { out[0] = 0; return -1; }
    out[len] = 0;
    return 0;
}

int wf_idna_to_unicode(const char *host, char *out, int cap) {
    uint32_t cp[WF_IDNA_CPS];
    char back[WF_IDNA_LABEL + 1];
    int n, i = 0, len = 0;
    if (!host || !out || cap <= 0) return -1;
    out[0] = 0;
    n = s_len(host);
    if (n == 0 || n > WF_IDNA_MAX) return -1;
    while (i < n) {
        int e = i, k, ncp;
        while (e < n && host[e] != '.') e++;
        if (e == i) { out[0] = 0; return -1; }
        if (e - i > 4 && (host[i] | 32) == 'x' && (host[i + 1] | 32) == 'n' && host[i + 2] == '-' && host[i + 3] == '-') {
            /* the ASCII form is case-insensitive: decode it lower-cased */
            char ace[WF_IDNA_LABEL + 1];
            if (e - i - 4 > WF_IDNA_LABEL) { out[0] = 0; return -1; }
            for (k = 0; k < e - i - 4; k++) {
                char c = host[i + 4 + k];
                ace[k] = (c >= 'A' && c <= 'Z') ? (char)(c + 32) : c;
            }
            ncp = wf_puny_decode(ace, e - i - 4, cp, WF_IDNA_CPS);
            if (ncp <= 0) { out[0] = 0; return -1; }
            /* a label must encode back to itself (no upper case, no
             * ASCII-only label in disguise) */
            for (k = 0; k < ncp; k++) if (!cp_ok(cp[k]) || fold(cp[k]) != cp[k]) { out[0] = 0; return -1; }
            if (put_label(cp, ncp, back, (int)sizeof(back), 0) < 0) { out[0] = 0; return -1; }
            for (k = 0; back[k] || k < e - i; k++) {
                char a = host[i + k];
                if (a >= 'A' && a <= 'Z') a = (char)(a + 32);
                if (k >= e - i || back[k] != a) { out[0] = 0; return -1; }
            }
            for (k = 0; k < ncp; k++) {
                len = utf8_put(cp[k], out, cap, len);
                if (len < 0) { out[0] = 0; return -1; }
            }
        } else {
            for (k = i; k < e; k++) {
                char c = host[k];
                if ((unsigned char)c >= 0x80U) { out[0] = 0; return -1; }
                if (c >= 'A' && c <= 'Z') c = (char)(c + 32);
                if (len + 1 >= cap) { out[0] = 0; return -1; }
                out[len++] = c;
            }
        }
        if (e < n) {
            if (len + 1 >= cap) { out[0] = 0; return -1; }
            out[len++] = '.';
        }
        i = e + 1;
        if (e == n - 1) { out[0] = 0; return -1; }   /* trailing dot */
    }
    out[len] = 0;
    return 0;
}
