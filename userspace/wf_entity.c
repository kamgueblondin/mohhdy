/* wf_entity.c - Lot 71: character references, page charsets and display
 * text (see wf_entity.h). */
#include "wf_entity.h"
#include "../include/os_charset.h"
#include "wf_entity_table.inc"

static int e_low(int c) { return c >= 'A' && c <= 'Z' ? c + 32 : c; }
static int e_alpha(int c) { return (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z'); }
static int e_alnum(int c) { return e_alpha(c) || (c >= '0' && c <= '9'); }

static int name_cmp(const char *a, const char *b, int bl) {
    int i;
    for (i = 0; i < bl; i++) {
        unsigned char x = (unsigned char)a[i], y = (unsigned char)b[i];
        if (!x) return -1;
        if (x != y) return x < y ? -1 : 1;
    }
    return a[bl] ? 1 : 0;
}

uint32_t wf_entity_lookup(const char *name, int len) {
    int lo = 0, hi = WF_ENTITY_COUNT - 1;
    if (len <= 0 || len > 10) return 0;
    while (lo <= hi) {
        int mid = (lo + hi) / 2, c = name_cmp(wf_entity_table[mid].name, name, len);
        if (c == 0) return wf_entity_table[mid].cp;
        if (c < 0) lo = mid + 1;
        else hi = mid - 1;
    }
    return 0;
}

uint32_t wf_entity_decode(const char *s, int n, int *used) {
    int i = 1;
    *used = 0;
    if (n < 3 || s[0] != '&') return 0;
    if (s[1] == '#') {
        uint32_t v = 0;
        int hex = 0, digits = 0;
        i = 2;
        if (i < n && (s[i] == 'x' || s[i] == 'X')) { hex = 1; i++; }
        while (i < n && digits < 8) {
            int c = s[i], d;
            if (c >= '0' && c <= '9') d = c - '0';
            else if (hex && e_low(c) >= 'a' && e_low(c) <= 'f') d = e_low(c) - 'a' + 10;
            else break;
            v = v * (hex ? 16u : 10u) + (uint32_t)d;
            digits++;
            i++;
        }
        if (!digits) return 0;
        while (i < n && ((s[i] >= '0' && s[i] <= '9') || (hex && e_alnum(s[i])))) { i++; v = 0x110000u; }
        if (i < n && s[i] == ';') i++;
        *used = i;
        if (v == 0 || v > 0x10FFFFu || (v >= 0xD800u && v <= 0xDFFFu)) return OS_UNICODE_BAD;
        /* numeric references in 0x80..0x9F mean CP1252 (HTML spec) */
        if (v >= 0x80u && v <= 0x9Fu) return os_cp1252_high[v - 0x80u] ? os_cp1252_high[v - 0x80u] : OS_UNICODE_BAD;
        return v;
    }
    while (i < n && i < 12 && e_alnum(s[i])) i++;
    if (i == 1) return 0;
    {
        uint32_t cp = wf_entity_lookup(s + 1, i - 1);
        if (cp && i < n && s[i] == ';') { *used = i + 1; return cp; }
        if (cp && cp <= 0xFFu) { *used = i; return cp; }   /* legacy: "&eacute" without ';' */
        /* longest known prefix without ';' (e.g. "&copy2026") for Latin-1 names */
        {
            int k;
            for (k = i - 1; k >= 2; k--) {
                cp = wf_entity_lookup(s + 1, k);
                if (cp && cp <= 0xFFu && cp >= 0xA0u) { *used = k + 1; return cp; }
                if (cp && (cp == '&' || cp == '<' || cp == '>' || cp == '"')) { *used = k + 1; return cp; }
            }
        }
    }
    return 0;
}

static int ci_eq(const char *a, int an, const char *b) {
    int i;
    for (i = 0; i < an; i++) if (!b[i] || e_low((unsigned char)a[i]) != b[i]) return 0;
    return b[an] == 0;
}

int wf_charset_from(const char *v, int n) {
    int i = 0, s;
    static const char *const latin[] = { "iso-8859-1", "iso8859-1", "latin1", "latin-1", "windows-1252", "cp1252",
                                         "us-ascii", "ascii", "iso-8859-15", "l1", 0 };
    if (!v) return WF_CS_UTF8;
    if (n < 0) { n = 0; while (v[n]) n++; }
    /* "text/html; charset=..." or a bare name */
    s = 0;
    for (i = 0; i + 8 <= n; i++)
        if (ci_eq(v + i, 8, "charset=")) { s = i + 8; break; }
    i = s;
    while (i < n && (v[i] == ' ' || v[i] == '"' || v[i] == '\'')) i++;
    s = i;
    while (i < n && v[i] != ';' && v[i] != '"' && v[i] != '\'' && v[i] != ' ' && v[i] != '>' && v[i] != '/') i++;
    {
        int k;
        for (k = 0; latin[k]; k++) if (ci_eq(v + s, i - s, latin[k])) return WF_CS_LATIN1;
    }
    return WF_CS_UTF8;
}

int wf_charset_header(const char *v, int n) {
    int i;
    if (!v) return WF_CS_NONE;
    if (n < 0) { n = 0; while (v[n]) n++; }
    for (i = 0; i + 8 <= n; i++)
        if (ci_eq(v + i, 8, "charset=")) return wf_charset_from(v, n);
    return WF_CS_NONE;
}

int wf_charset_sniff(const char *h, int n) {
    int i, lim = n < 2048 ? n : 2048;
    for (i = 0; i + 6 < lim; i++) {
        if (h[i] == '<' && e_low(h[i + 1]) == 'm' && e_low(h[i + 2]) == 'e' && e_low(h[i + 3]) == 't' && e_low(h[i + 4]) == 'a') {
            int e = i + 5, k;
            while (e < lim && h[e] != '>') e++;
            for (k = i + 5; k + 8 <= e; k++)
                if (ci_eq(h + k, 8, "charset=")) return wf_charset_from(h + k, e - k);
            i = e;
        }
    }
    return -1;
}

uint32_t wf_text_next(const char *s, int n, int *i, int cs, int ent) {
    unsigned char c;
    if (*i >= n) return 0;
    c = (unsigned char)s[*i];
    if (ent && c == '&') {
        int used = 0;
        uint32_t cp = wf_entity_decode(s + *i, n - *i, &used);
        if (used) { *i += used; return cp; }
    }
    if (c < 0x80u) { (*i)++; return c; }
    if (cs == WF_CS_LATIN1) {
        (*i)++;
        if (c >= 0x80u && c <= 0x9Fu) return os_cp1252_high[c - 0x80u] ? os_cp1252_high[c - 0x80u] : OS_UNICODE_BAD;
        return c;
    }
    return os_utf8_next(s, n, i);
}

/* Lot 72: characters outside CP1252 get their glyph page selector; the
 * text ends in page 0 (one byte is kept for the closing selector). */
int wf_text_display(const char *s, int n, int cs, int collapse, char *out, int cap) {
    int i = 0, o = 0, sp = 1, page = 0;
    if (cap <= 0) return 0;
    while (i < n && o < cap - 2) {
        uint32_t cp = wf_text_next(s, n, &i, cs, 1);
        int m;
        if (cp == 0) break;
        if (collapse && (cp == ' ' || cp == '\t' || cp == '\n' || cp == '\r' || cp == '\f' || cp == 0xA0u)) {
            if (!sp) { out[o++] = ' '; sp = 1; }
            continue;
        }
        if (cp == '\r') continue;
        m = os_display_put(cp, &page, out + o, cap - 2 - o);
        if (m < 0) break;
        if (m == 0) continue;
        o += m;
        sp = 0;
    }
    if (collapse) while (o > 0 && out[o - 1] == ' ') o--;
    if (page) out[o++] = (char)os_page_sel[0];
    out[o] = 0;
    return o;
}

int wf_display_to_utf8(const char *s, int n, char *out, int cap) {
    int i, o = 0, page = 0;
    if (cap <= 0) return 0;
    for (i = 0; i < n && s[i]; i++) {
        char t[4];
        int k, j, q = os_page_of_sel((unsigned char)s[i]);
        if (q >= 0) { page = q; continue; }   /* Lot 72: glyph page selector */
        k = os_display_to_utf8_page((unsigned)page, (unsigned char)s[i], t, 4);
        if (o + k > cap - 1) break;
        for (j = 0; j < k; j++) out[o++] = t[j];
    }
    out[o] = 0;
    return o;
}
