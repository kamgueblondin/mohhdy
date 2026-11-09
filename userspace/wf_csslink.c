/* wf_csslink.c - Lot 87: see wf_csslink.h. Freestanding, no heap. */
#include "wf_csslink.h"
#include "wf_http.h"
#include "wf_sitecss.h"

static int slen(const char *s) { int n = 0; while (s && s[n]) n++; return n; }
static char lc(char c) { return (c >= 'A' && c <= 'Z') ? (char)(c - 'A' + 'a') : c; }
static int is_sp(char c) { return c == ' ' || c == '\t' || c == '\n' || c == '\r' || c == '\f'; }
static void zero(void *p, int n) { unsigned char *b = (unsigned char *)p; while (n-- > 0) *b++ = 0; }
static void copyn(char *d, int cap, const char *s, int n) {
    int i = 0;
    if (cap <= 0) return;
    while (i < n && s[i] && i < cap - 1) { d[i] = s[i]; i++; }
    d[i] = 0;
}
static int eqi(const char *a, const char *b) {
    int i = 0;
    while (a[i] && b[i] && lc(a[i]) == lc(b[i])) i++;
    return a[i] == 0 && b[i] == 0;
}
/* case-insensitive "word" at s[i] */
static int at_word(const char *s, int n, int i, const char *w) {
    int k = 0;
    while (w[k]) {
        if (i + k >= n || lc(s[i + k]) != w[k]) return 0;
        k++;
    }
    return 1;
}

static int resolve(const char *base_url, const char *href, char *out, int cap) {
    static wf_url_t base, abs;
    int i;
    out[0] = 0;
    if (!href || !href[0]) return -1;
    /* data:, javascript: and other schemes are not fetched */
    for (i = 0; href[i] && href[i] != '/' && href[i] != '?' && href[i] != '#'; i++) {
        if (href[i] == ':') {
            if ((i == 4 && at_word(href, i, 0, "http")) || (i == 5 && at_word(href, i, 0, "https"))) break;
            return -1;
        }
    }
    if (!base_url || wf_url_parse(base_url, &base) != 0) return -1;
    if (wf_url_resolve(&base, href, &abs) != 0) return -1;
    if (wf_url_format(&abs, out, cap) <= 0) return -1;
    return 0;
}

static int add_ref(wf_cssl_ref_t *out, int cap, int *count, const char *url, const char *media, int mn,
                   int from_import, wf_cssl_stats_t *st) {
    int k;
    for (k = 0; k < *count; k++) {
        int j = 0;
        while (out[k].url[j] && out[k].url[j] == url[j]) j++;
        if (!out[k].url[j] && !url[j]) return 0;   /* the same sheet twice */
    }
    if (*count >= cap) { if (st) st->dropped++; return -1; }
    copyn(out[*count].url, WF_CSSL_URL, url, slen(url));
    copyn(out[*count].media, (int)sizeof(out[0].media), media ? media : "", media ? mn : 0);
    out[*count].from_import = from_import;
    (*count)++;
    return 0;
}

/* attribute value at s[i] (after '='), quoted or not */
static int attr_val(const char *s, int end, int i, char *out, int cap) {
    int k = 0;
    char q = 0;
    while (i < end && is_sp(s[i])) i++;
    if (i < end && (s[i] == '"' || s[i] == '\'')) q = s[i++];
    while (i < end) {
        if (q ? s[i] == q : (is_sp(s[i]) || s[i] == '>')) break;
        if (k < cap - 1) out[k++] = s[i];
        i++;
    }
    if (q && i < end) i++;
    out[k] = 0;
    return i;
}

/* Leading @import rules of a CSS text: url(x), url("x") or "x", then an
 * optional media list up to ';'. Stops at the first rule body. */
static int css_imports(const char *css, int n, const char *base, wf_cssl_ref_t *out, int cap, int *count,
                       wf_cssl_stats_t *st) {
    int i = 0, got = 0;
    while (i < n) {
        int a, b;
        char href[WF_CSSL_URL], url[WF_CSSL_URL];
        while (i < n && is_sp(css[i])) i++;
        if (i + 1 < n && css[i] == '/' && css[i + 1] == '*') {
            i += 2;
            while (i + 1 < n && !(css[i] == '*' && css[i + 1] == '/')) i++;
            i += 2;
            continue;
        }
        if (i + 7 < n && at_word(css, n, i, "@charset")) { while (i < n && css[i] != ';') i++; i++; continue; }
        if (i + 7 > n || !at_word(css, n, i, "@import")) break;
        i += 7;
        while (i < n && is_sp(css[i])) i++;
        if (i + 4 <= n && at_word(css, n, i, "url(")) {
            i += 4;
            while (i < n && is_sp(css[i])) i++;
            if (i < n && (css[i] == '"' || css[i] == '\'')) {
                char q = css[i++];
                a = i;
                while (i < n && css[i] != q) i++;
                b = i;
                while (i < n && css[i] != ')') i++;
            } else {
                a = i;
                while (i < n && css[i] != ')' && !is_sp(css[i])) i++;
                b = i;
                while (i < n && css[i] != ')') i++;
            }
            if (i < n) i++;
        } else if (i < n && (css[i] == '"' || css[i] == '\'')) {
            char q = css[i++];
            a = i;
            while (i < n && css[i] != q) i++;
            b = i;
            if (i < n) i++;
        } else {
            while (i < n && css[i] != ';') i++;
            i++;
            continue;
        }
        while (i < n && is_sp(css[i])) i++;
        {
            int m = i, me;
            while (i < n && css[i] != ';') i++;
            me = i;
            while (me > m && is_sp(css[me - 1])) me--;
            if (i < n) i++;
            copyn(href, (int)sizeof(href), css + a, b - a);
            if (resolve(base, href, url, (int)sizeof(url)) != 0) { if (st) st->refused++; continue; }
            if (add_ref(out, cap, count, url, css + m, me - m, 1, st) == 0) got++;
        }
    }
    return got;
}

int wf_csslink_imports(const char *css, int n, const char *sheet_url, wf_cssl_ref_t *out, int cap) {
    int count = 0;
    if (!css || n <= 0 || !out || cap <= 0) return 0;
    (void)css_imports(css, n, sheet_url, out, cap, &count, 0);
    return count;
}

int wf_csslink_scan(const char *h, int n, const char *base_url, wf_cssl_ref_t *out, int cap, wf_cssl_stats_t *st) {
    int i = 0, count = 0;
    if (!h || n <= 0 || !out || cap <= 0) return 0;
    while (i < n) {
        int end;
        if (h[i] != '<') { i++; continue; }
        if (i + 3 < n && h[i + 1] == '!' && h[i + 2] == '-' && h[i + 3] == '-') {
            i += 4;
            while (i + 2 < n && !(h[i] == '-' && h[i + 1] == '-' && h[i + 2] == '>')) i++;
            i += 3;
            continue;
        }
        end = i + 1;
        while (end < n && h[end] != '>') {
            if (h[end] == '"' || h[end] == '\'') { char q = h[end++]; while (end < n && h[end] != q) end++; }
            if (end < n) end++;
        }
        if (at_word(h, n, i + 1, "link") && i + 5 < n && (is_sp(h[i + 5]) || h[i + 5] == '/')) {
            int p = i + 5, sheet = 0, alt = 0;
            char href[WF_CSSL_URL], media[32], url[WF_CSSL_URL];
            href[0] = media[0] = 0;
            while (p < end) {
                char name[16], val[WF_CSSL_URL];
                int k = 0;
                while (p < end && (is_sp(h[p]) || h[p] == '/')) p++;
                while (p < end && !is_sp(h[p]) && h[p] != '=' && h[p] != '>' && h[p] != '/') {
                    if (k < 15) name[k++] = lc(h[p]);
                    p++;
                }
                name[k] = 0;
                val[0] = 0;
                while (p < end && is_sp(h[p])) p++;
                if (p < end && h[p] == '=') p = attr_val(h, end, p + 1, val, (int)sizeof(val));
                if (!name[0]) { p++; continue; }
                if (eqi(name, "rel")) {
                    int q = 0;
                    while (val[q]) {
                        char w[16];
                        int a, m = 0;
                        while (val[q] && is_sp(val[q])) q++;
                        a = q;
                        while (val[q] && !is_sp(val[q])) q++;
                        while (a < q && m < 15) w[m++] = lc(val[a++]);
                        w[m] = 0;
                        if (eqi(w, "stylesheet")) sheet = 1;
                        if (eqi(w, "alternate")) alt = 1;
                    }
                } else if (eqi(name, "href")) copyn(href, (int)sizeof(href), val, slen(val));
                else if (eqi(name, "media")) copyn(media, (int)sizeof(media), val, slen(val));
            }
            if (sheet && !alt && href[0]) {
                if (st) st->found++;
                if (resolve(base_url, href, url, (int)sizeof(url)) != 0) { if (st) st->refused++; }
                else (void)add_ref(out, cap, &count, url, media, slen(media), 0, st);
            }
        } else if (at_word(h, n, i + 1, "style") && i + 6 < n && (is_sp(h[i + 6]) || h[i + 6] == '>')) {
            int body = end + 1, stop = body, before = count;
            while (stop + 7 < n && !(h[stop] == '<' && h[stop + 1] == '/' && at_word(h, n, stop + 2, "style"))) stop++;
            if (stop + 7 >= n) stop = n;
            (void)css_imports(h + body, stop - body, base_url, out, cap, &count, st);
            if (st) st->found += count - before;
            i = stop;
            continue;
        } else if ((at_word(h, n, i + 1, "script") && i + 7 < n) || at_word(h, n, i + 1, "textarea")) {
            /* raw text: a "<link" inside is not markup */
            const char *t = at_word(h, n, i + 1, "script") ? "script" : "textarea";
            int tl = slen(t), stop = end + 1;
            while (stop + tl + 2 < n && !(h[stop] == '<' && h[stop + 1] == '/' && at_word(h, n, stop + 2, t))) stop++;
            i = stop + 1;
            continue;
        }
        i = end + 1;
    }
    return count;
}

static int put(char *out, int cap, int n, const char *s, int len) {
    int i;
    for (i = 0; i < len && s[i]; i++) {
        if (n >= cap - 1) return -1;
        out[n++] = s[i];
    }
    out[n] = 0;
    return n;
}

/* Copy a sheet without its leading @charset/@import rules (they are
 * resolved separately). */
static int sheet_body_start(const char *css, int n) {
    int i = 0, last = 0;
    while (i < n) {
        while (i < n && is_sp(css[i])) i++;
        if (i + 1 < n && css[i] == '/' && css[i + 1] == '*') {
            i += 2;
            while (i + 1 < n && !(css[i] == '*' && css[i + 1] == '/')) i++;
            i += 2;
            last = i;
            continue;
        }
        if (i + 7 <= n && (at_word(css, n, i, "@import") || at_word(css, n, i, "@charset"))) {
            int q = 0;
            while (i < n && (q || css[i] != ';')) {
                if (css[i] == '"' || css[i] == '\'') q = q == css[i] ? 0 : (q ? q : css[i]);
                i++;
            }
            i++;
            last = i;
            continue;
        }
        break;
    }
    return last < n ? last : n;
}

static int append_sheet(char *out, int cap, int n, const char *media, const char *css, int len) {
    int m = slen(media), start = sheet_body_start(css, len), save = n;
    if (m) {
        if ((n = put(out, cap, n, "@media ", 7)) < 0) return -1;
        if ((n = put(out, cap, n, media, m)) < 0) return -1;
        if ((n = put(out, cap, n, " {\n", 3)) < 0) return -1;
    }
    if ((n = put(out, cap, n, css + start, len - start)) < 0) { out[save] = 0; return -1; }
    if (m && (n = put(out, cap, n, "\n}", 2)) < 0) { out[save] = 0; return -1; }
    if ((n = put(out, cap, n, "\n", 1)) < 0) { out[save] = 0; return -1; }
    return n;
}

int wf_csslink_collect(const char *html, int n, const char *base_url, int vw, int vh,
                       wf_css_get_t get, void *ctx, char *out, int cap, wf_cssl_stats_t *st) {
    static wf_cssl_ref_t refs[WF_CSSL_FOUND], sub[WF_CSSL_FOUND];
    static char sheet[WF_CSSL_SHEET_MAX + 1], inner[WF_CSSL_SHEET_MAX + 1];
    wf_cssl_stats_t dummy;
    int nrefs, r, w = 0;
    if (!st) st = &dummy;
    zero(st, (int)sizeof(*st));
    if (!out || cap < 2 || !get) return -1;
    out[0] = 0;
    nrefs = wf_csslink_scan(html, n, base_url, refs, WF_CSSL_FOUND, st);
    for (r = 0; r < nrefs; r++) {
        int len, k, nsub;
        if (refs[r].media[0] && !wf_site_media_ok(refs[r].media, slen(refs[r].media), vw, vh)) {
            st->skipped_media++;
            continue;
        }
        if (st->fetched + st->failed >= WF_CSSL_SHEETS) { st->dropped++; continue; }
        len = get(ctx, refs[r].url, sheet, WF_CSSL_SHEET_MAX + 1);
        if (len < 0) { st->failed++; continue; }
        st->fetched++;
        if (len > WF_CSSL_SHEET_MAX) { len = WF_CSSL_SHEET_MAX; st->truncated++; }
        sheet[len] = 0;
        /* one level of @import inside the fetched sheet, placed before it */
        nsub = wf_csslink_imports(sheet, len, refs[r].url, sub, WF_CSSL_FOUND);
        st->found += nsub;
        for (k = 0; k < nsub; k++) {
            int il, nw;
            if (sub[k].media[0] && !wf_site_media_ok(sub[k].media, slen(sub[k].media), vw, vh)) { st->skipped_media++; continue; }
            if (st->fetched + st->failed >= WF_CSSL_SHEETS) { st->dropped++; continue; }
            il = get(ctx, sub[k].url, inner, WF_CSSL_SHEET_MAX + 1);
            if (il < 0) { st->failed++; continue; }
            st->fetched++;
            if (il > WF_CSSL_SHEET_MAX) { il = WF_CSSL_SHEET_MAX; st->truncated++; }
            inner[il] = 0;
            nw = append_sheet(out, cap, w, sub[k].media[0] ? sub[k].media : refs[r].media, inner, il);
            if (nw < 0) { st->dropped++; continue; }
            w = nw;
        }
        {
            int nw = append_sheet(out, cap, w, refs[r].media, sheet, len);
            if (nw < 0) { st->dropped++; continue; }
            w = nw;
        }
    }
    st->bytes = w;
    return w;
}

static int put_num(char *out, int cap, int n, const char *label, int v) {
    char t[12];
    int k = 0;
    n = put(out, cap, n, label, slen(label));
    if (n < 0) return -1;
    if (v < 0) v = 0;
    if (!v) t[k++] = '0';
    while (v && k < 11) { t[k++] = (char)('0' + v % 10); v /= 10; }
    while (k && n >= 0) { char c = t[--k]; n = put(out, cap, n, &c, 1); }
    return n;
}

int wf_csslink_stats_text(const wf_cssl_stats_t *st, char *out, int cap) {
    int n = 0;
    if (!out || cap < 8 || !st) return 0;
    out[0] = 0;
    n = put_num(out, cap, n, "css-links found ", st->found);
    if (n >= 0) n = put_num(out, cap, n, " fetched ", st->fetched);
    if (n >= 0) n = put_num(out, cap, n, " failed ", st->failed);
    if (n >= 0) n = put_num(out, cap, n, " media ", st->skipped_media);
    if (n >= 0) n = put_num(out, cap, n, " refused ", st->refused);
    if (n >= 0) n = put_num(out, cap, n, " dropped ", st->dropped);
    if (n >= 0) n = put_num(out, cap, n, " bytes ", st->bytes);
    if (n >= 0) n = put(out, cap, n, "\n", 1);
    return n < 0 ? slen(out) : n;
}
