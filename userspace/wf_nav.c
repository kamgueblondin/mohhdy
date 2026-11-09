/* wf_nav.c - Lot 86: see wf_nav.h. */
#include "wf_nav.h"

static int nlen(const char *s) { int n = 0; while (s && s[n]) n++; return n; }
static void nmove(void *d, const void *s, uint32_t n) {
    uint8_t *a = (uint8_t *)d;
    const uint8_t *b = (const uint8_t *)s;
    while (n--) *a++ = *b++;
}
static void ncpy(char *d, int cap, const char *s) {
    int i = 0;
    if (cap <= 0) return;
    while (s && s[i] && i < cap - 1) { d[i] = s[i]; i++; }
    d[i] = 0;
}
static int ncmp(const char *a, const char *b) {
    int i = 0;
    if (!a) a = "";
    if (!b) b = "";
    while (a[i] && a[i] == b[i]) i++;
    return a[i] == b[i];
}
static char nlow(char c) { return (c >= 'A' && c <= 'Z') ? (char)(c + 32) : c; }
static int ndigit(char c) { return c >= '0' && c <= '9'; }
static int nhex(char c) {
    if (ndigit(c)) return c - '0';
    if (c >= 'a' && c <= 'f') return c - 'a' + 10;
    if (c >= 'A' && c <= 'F') return c - 'A' + 10;
    return -1;
}

void wf_nav_init(wf_nav_t *n) {
    uint8_t *p;
    uint32_t i;
    if (!n) return;
    p = (uint8_t *)n;
    for (i = 0; i < (uint32_t)sizeof(*n); i++) p[i] = 0;
    n->cursor = -1;
}

/* Percent-decode into out. A bad % sequence is copied raw. */
static int decode(const char *s, int n, char *out, int cap) {
    int i = 0, o = 0;
    while (i < n) {
        unsigned char c = (unsigned char)s[i];
        if (c == '%' && i + 2 < n) {
            int h = nhex(s[i + 1]), l = nhex(s[i + 2]);
            if (h >= 0 && l >= 0) { c = (unsigned char)((h << 4) | l); i += 3; }
            else i++;
        } else i++;
        if (c == 0) c = ' ';
        if (o >= cap - 1) return -1;
        out[o++] = (char)c;
    }
    if (o < cap) out[o] = 0;
    return o;
}

static int scheme_ok(const char *s, int n) {
    int i;
    if (n < 2 || n > 8) return 0;
    if (!((s[0] >= 'a' && s[0] <= 'z') || (s[0] >= 'A' && s[0] <= 'Z'))) return 0;
    for (i = 1; i < n; i++) {
        char c = s[i];
        if (!((c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') || ndigit(c))) return 0;
    }
    return 1;
}

static int is_http(const char *sch) {
    return ncmp(sch, "http") || ncmp(sch, "https");
}

/* Remove "." and ".." segments. ".." above the root stays at the root. */
static int dot_segments(const char *in, char *out, int cap) {
    char seg[16][48];
    int ns = 0, i = 0, n = nlen(in), o = 0, abs = (n > 0 && in[0] == '/');
    if (n > 0 && in[0] == '/') i = 1;
    while (i <= n) {
        int st = i, sl;
        while (i < n && in[i] != '/') i++;
        sl = i - st;
        if (sl == 0 || (sl == 1 && in[st] == '.')) {
            /* empty or "." */
        } else if (sl == 2 && in[st] == '.' && in[st + 1] == '.') {
            if (ns > 0) ns--;
        } else {
            int k;
            if (ns >= 16 || sl >= 48) return -1;
            for (k = 0; k < sl; k++) seg[ns][k] = in[st + k];
            seg[ns][sl] = 0;
            ns++;
        }
        if (i >= n) break;
        i++;
    }
    if (abs) {
        if (o >= cap - 1) return -1;
        out[o++] = '/';
    }
    for (i = 0; i < ns; i++) {
        int k = 0;
        if (i && (o >= cap - 1)) return -1;
        if (i) out[o++] = '/';
        while (seg[i][k]) {
            if (o >= cap - 1) return -1;
            out[o++] = seg[i][k++];
        }
    }
    if (o == 0) {
        if (cap < 2) return -1;
        out[0] = '/';
        o = 1;
    }
    out[o] = 0;
    return o;
}

static int copy_span(char *d, int cap, const char *s, int n, int lower) {
    int i;
    if (n >= cap) return -1;
    for (i = 0; i < n; i++) d[i] = lower ? nlow(s[i]) : s[i];
    d[n] = 0;
    return n;
}

int wf_nav_resolve(const char *base, const char *ref, char *out, int cap) {
    char sch[12], host[96], path[WF_NAV_URL], query[WF_NAV_URL];
    char bhost[96], bpath[WF_NAV_URL];
    char raw[WF_NAV_URL], joined[WF_NAV_URL];
    int i, n, port = -1, bport = -1, o = 0, https = 0;
    const char *p;
    if (!out || cap < 12 || !ref || !ref[0]) return -1;
    n = nlen(ref);
    if (n >= (int)sizeof(raw)) return -1;
    /* Drop a fragment before anything else. */
    for (i = 0; i < n; i++) if (ref[i] == '#') { n = i; break; }
    if (decode(ref, n, raw, (int)sizeof(raw)) < 0) return -1;
    p = raw;
    /* scheme? */
    sch[0] = 0;
    for (i = 0; p[i] && p[i] != ':' && p[i] != '/' && p[i] != '?' && p[i] != ' '; i++) {}
    if (p[i] == ':' && scheme_ok(p, i) && p[i + 1] == '/' && p[i + 2] == '/') {
        if (copy_span(sch, (int)sizeof(sch), p, i, 1) < 0) return -1;
        if (!is_http(sch)) return -1;
        p += i + 3;
        https = ncmp(sch, "https");
    } else if (base && base[0]) {
        const char *b = base;
        int bi;
        for (bi = 0; b[bi] && b[bi] != ':' && bi < 8; bi++) {}
        if (b[bi] != ':' || b[bi + 1] != '/' || b[bi + 2] != '/') return -1;
        if (copy_span(sch, (int)sizeof(sch), b, bi, 1) < 0) return -1;
        if (!is_http(sch)) return -1;
        https = ncmp(sch, "https");
        b += bi + 3;
        for (bi = 0; b[bi] && b[bi] != '/' && b[bi] != '?' && b[bi] != ':'; bi++) {}
        if (copy_span(bhost, (int)sizeof(bhost), b, bi, 1) < 0) return -1;
        b += bi;
        if (*b == ':') {
            b++;
            bport = 0;
            while (ndigit(*b)) { bport = bport * 10 + (*b - '0'); b++; if (bport > 65535) return -1; }
        }
        if (*b == '/') {
            int pl = 0;
            while (b[pl] && b[pl] != '?') pl++;
            if (copy_span(bpath, (int)sizeof(bpath), b, pl, 0) < 0) return -1;
        } else ncpy(bpath, (int)sizeof(bpath), "/");
    } else return -1;

    host[0] = 0;
    path[0] = 0;
    query[0] = 0;
    if (raw[0] && sch[0] && p != raw) {
        /* absolute: host then path */
        for (i = 0; p[i] && p[i] != '/' && p[i] != '?' && p[i] != ':'; i++) {}
        if (i == 0 || copy_span(host, (int)sizeof(host), p, i, 1) < 0) return -1;
        p += i;
        if (*p == ':') {
            p++;
            port = 0;
            while (ndigit(*p)) { port = port * 10 + (*p - '0'); p++; if (port > 65535) return -1; }
        }
        if (*p == '?') {
            ncpy(path, (int)sizeof(path), "/");
            ncpy(query, (int)sizeof(query), p + 1);
        } else if (*p == '/') {
            int pl = 0;
            while (p[pl] && p[pl] != '?') pl++;
            if (copy_span(path, (int)sizeof(path), p, pl, 0) < 0) return -1;
            if (p[pl] == '?') ncpy(query, (int)sizeof(query), p + pl + 1);
        } else ncpy(path, (int)sizeof(path), "/");
    } else {
        /* relative to base */
        ncpy(host, (int)sizeof(host), bhost);
        port = bport;
        if (raw[0] == '?') {
            ncpy(path, (int)sizeof(path), bpath[0] ? bpath : "/");
            ncpy(query, (int)sizeof(query), raw + 1);
        } else if (raw[0] == '/') {
            int pl = 0;
            while (raw[pl] && raw[pl] != '?') pl++;
            if (copy_span(path, (int)sizeof(path), raw, pl, 0) < 0) return -1;
            if (raw[pl] == '?') ncpy(query, (int)sizeof(query), raw + pl + 1);
        } else if (!raw[0]) {
            ncpy(path, (int)sizeof(path), bpath[0] ? bpath : "/");
        } else {
            /* path relative to the base directory */
            int cut = nlen(bpath), k = 0;
            while (cut > 0 && bpath[cut - 1] != '/') cut--;
            if (cut + nlen(raw) >= (int)sizeof(joined)) return -1;
            for (k = 0; k < cut; k++) joined[k] = bpath[k];
            joined[cut] = 0;
            {
                int rl = 0;
                while (raw[rl] && raw[rl] != '?') rl++;
                if (cut + rl >= (int)sizeof(joined)) return -1;
                for (k = 0; k < rl; k++) joined[cut + k] = raw[k];
                joined[cut + rl] = 0;
                if (raw[rl] == '?') ncpy(query, (int)sizeof(query), raw + rl + 1);
            }
            ncpy(path, (int)sizeof(path), joined);
        }
    }
    if (dot_segments(path[0] ? path : "/", path, (int)sizeof(path)) < 0) return -1;
    if ((https && port == 443) || (!https && port == 80)) port = -1;
    /* assemble */
    for (i = 0; sch[i]; i++) { if (o >= cap - 1) return -1; out[o++] = sch[i]; }
    if (o + 3 >= cap) return -1;
    out[o++] = ':'; out[o++] = '/'; out[o++] = '/';
    for (i = 0; host[i]; i++) { if (o >= cap - 1) return -1; out[o++] = host[i]; }
    if (port >= 0) {
        char tmp[8];
        int t = 0, v = port;
        if (o >= cap - 1) return -1;
        out[o++] = ':';
        if (v == 0) tmp[t++] = '0';
        while (v) { tmp[t++] = (char)('0' + v % 10); v /= 10; }
        while (t) { if (o >= cap - 1) return -1; out[o++] = tmp[--t]; }
    }
    for (i = 0; path[i]; i++) { if (o >= cap - 1) return -1; out[o++] = path[i]; }
    if (query[0]) {
        if (o >= cap - 1) return -1;
        out[o++] = '?';
        for (i = 0; query[i]; i++) { if (o >= cap - 1) return -1; out[o++] = query[i]; }
    }
    out[o] = 0;
    return 0;
}

static int bad_url(const char *url) {
    int i;
    if (!url || !url[0]) return 1;
    if (nlen(url) >= WF_NAV_URL) return 1;
    for (i = 0; url[i]; i++) if ((unsigned char)url[i] < 0x20u || url[i] == ' ') return 1;
    if ((url[0] == 'j' || url[0] == 'J') && nlow(url[1]) == 'a') return 1; /* javascript: */
    if ((url[0] == 'd' || url[0] == 'D') && nlow(url[1]) == 'a' && nlow(url[2]) == 't' && nlow(url[3]) == 'a') return 1;
    return 0;
}

static void clean_title(char *d, int cap, const char *s) {
    int i = 0, o = 0, sp = 1;
    if (cap <= 0) return;
    if (!s) { d[0] = 0; return; }
    while (s[i] && o < cap - 1) {
        unsigned char c = (unsigned char)s[i++];
        if (c < 0x20u || c == 0x7Fu) c = ' ';
        if (c == ' ') { if (sp) continue; sp = 1; }
        else sp = 0;
        d[o++] = (char)c;
    }
    while (o > 0 && d[o - 1] == ' ') o--;
    d[o] = 0;
}

static void clean_stamp(char *d, const char *s) {
    int i;
    d[0] = 0;
    if (!s) return;
    for (i = 0; i < 14; i++) if (!ndigit(s[i])) return;
    if (s[14]) return;
    for (i = 0; i < 14; i++) d[i] = s[i];
    d[14] = 0;
}

static void hist_push(wf_nav_t *n, const wf_nav_ent_t *e) {
    n->hist[n->hist_next] = *e;
    n->hist_next = (n->hist_next + 1) % WF_NAV_HIST;
    if (n->hist_n < WF_NAV_HIST) n->hist_n++;
    else n->dropped++;
}

int wf_nav_visit(wf_nav_t *n, const char *url, const char *title, const char *stamp) {
    wf_nav_ent_t e;
    int i;
    if (!n || bad_url(url)) { if (n) n->refused++; return -1; }
    for (i = 0; i < (int)sizeof(e); i++) ((uint8_t *)&e)[i] = 0;
    ncpy(e.url, WF_NAV_URL, url);
    clean_title(e.title, WF_NAV_TITLE, title);
    clean_stamp(e.stamp, stamp);
    hist_push(n, &e);
    n->visits++;
    if (n->cursor >= 0 && ncmp(n->stack[n->cursor].url, e.url)) {
        ncpy(n->stack[n->cursor].title, WF_NAV_TITLE, e.title);
        if (e.stamp[0]) ncpy(n->stack[n->cursor].stamp, 15, e.stamp);
        return 0;
    }
    /* drop the forward list */
    if (n->cursor >= 0 && n->cursor + 1 < n->stack_n) n->stack_n = n->cursor + 1;
    if (n->stack_n >= WF_NAV_STACK) {
        int k;
        for (k = 1; k < n->stack_n; k++) n->stack[k - 1] = n->stack[k];
        n->stack_n--;
        if (n->cursor > 0) n->cursor--;
        n->dropped++;
    }
    n->stack[n->stack_n++] = e;
    n->cursor = n->stack_n - 1;
    return 0;
}

static int copy_out(const char *url, char *out, int cap) {
    if (!out || cap < 2 || nlen(url) >= cap) return -1;
    ncpy(out, cap, url);
    return 0;
}

int wf_nav_back(wf_nav_t *n, char *url, int cap) {
    if (!n || n->cursor <= 0) return -1;
    n->cursor--;
    n->backs++;
    return copy_out(n->stack[n->cursor].url, url, cap);
}

int wf_nav_forward(wf_nav_t *n, char *url, int cap) {
    if (!n || n->cursor < 0 || n->cursor + 1 >= n->stack_n) return -1;
    n->cursor++;
    n->forwards++;
    return copy_out(n->stack[n->cursor].url, url, cap);
}

int wf_nav_current(const wf_nav_t *n, char *url, int cap) {
    if (!n || n->cursor < 0) return -1;
    return copy_out(n->stack[n->cursor].url, url, cap);
}

int wf_nav_mark_add(wf_nav_t *n, const char *url, const char *title) {
    int i;
    wf_nav_ent_t e;
    if (!n || bad_url(url)) { if (n) n->refused++; return -1; }
    for (i = 0; i < n->nmark; i++) if (ncmp(n->mark[i].url, url)) {
        clean_title(n->mark[i].title, WF_NAV_TITLE, title);
        n->replaced++;
        return 0;
    }
    if (n->nmark >= WF_NAV_MARKS) { n->refused++; return -1; }
    for (i = 0; i < (int)sizeof(e); i++) ((uint8_t *)&e)[i] = 0;
    ncpy(e.url, WF_NAV_URL, url);
    clean_title(e.title, WF_NAV_TITLE, title);
    n->mark[n->nmark++] = e;
    n->marked++;
    return 0;
}

int wf_nav_mark_del(wf_nav_t *n, const char *url) {
    int i, k;
    if (!n || !url) return -1;
    for (i = 0; i < n->nmark; i++) if (ncmp(n->mark[i].url, url)) {
        for (k = i + 1; k < n->nmark; k++) n->mark[k - 1] = n->mark[k];
        n->nmark--;
        return 0;
    }
    return -1;
}

int wf_nav_mark_count(const wf_nav_t *n) { return n ? n->nmark : 0; }

const wf_nav_ent_t *wf_nav_mark_at(const wf_nav_t *n, int index) {
    if (!n || index < 0 || index >= n->nmark) return 0;
    return &n->mark[index];
}

static int contains(const char *hay, const char *needle) {
    int i, j, nl = nlen(needle);
    if (!nl) return 1;
    for (i = 0; hay[i]; i++) {
        for (j = 0; j < nl && hay[i + j] == needle[j]; j++) {}
        if (j == nl) return 1;
    }
    return 0;
}

int wf_nav_mark_find(const wf_nav_t *n, const char *needle) {
    int i;
    if (!n || !needle) return -1;
    for (i = 0; i < n->nmark; i++)
        if (contains(n->mark[i].url, needle) || contains(n->mark[i].title, needle)) return i;
    return -1;
}

int wf_nav_hist_count(const wf_nav_t *n) { return n ? n->hist_n : 0; }

const wf_nav_ent_t *wf_nav_hist_at(const wf_nav_t *n, int index) {
    int start;
    if (!n || index < 0 || index >= n->hist_n) return 0;
    start = (n->hist_n == WF_NAV_HIST) ? n->hist_next : 0;
    return &n->hist[(start + index) % WF_NAV_HIST];
}

static int app(char *o, int cap, int p, const char *s) {
    while (s && *s) { if (p >= cap - 1) return -1; o[p++] = *s++; }
    return p;
}
static int app_u(char *o, int cap, int p, int v) {
    char t[12];
    int n = 0;
    if (v < 0) v = 0;
    do { t[n++] = (char)('0' + v % 10); v /= 10; } while (v && n < 11);
    while (n) { if (p >= cap - 1) return -1; o[p++] = t[--n]; }
    return p;
}

static int emit_ent(char *o, int cap, int p, char kind, const wf_nav_ent_t *e) {
    if (p >= cap - 1) return -1;
    o[p++] = kind;
    if (p >= cap - 1) return -1;
    o[p++] = ' ';
    p = app(o, cap, p, e->url);
    if (p < 0 || p >= cap - 1) return -1;
    o[p++] = '\t';
    p = app(o, cap, p, e->title);
    if (p < 0 || p >= cap - 1) return -1;
    o[p++] = '\t';
    p = app(o, cap, p, e->stamp);
    if (p < 0 || p >= cap - 1) return -1;
    o[p++] = '\n';
    return p;
}

int wf_nav_save(const wf_nav_t *n, char *out, int cap) {
    int p = 0, i;
    if (!n || !out || cap < 8) return -1;
    p = app(out, cap, p, "C ");
    p = app_u(out, cap, p, n->cursor);
    if (p < 0 || p >= cap - 1) return -1;
    out[p++] = '\n';
    for (i = 0; i < n->stack_n; i++) {
        p = emit_ent(out, cap, p, 'S', &n->stack[i]);
        if (p < 0) return -1;
    }
    for (i = 0; i < n->nmark; i++) {
        p = emit_ent(out, cap, p, 'B', &n->mark[i]);
        if (p < 0) return -1;
    }
    for (i = 0; i < wf_nav_hist_count(n); i++) {
        p = emit_ent(out, cap, p, 'H', wf_nav_hist_at(n, i));
        if (p < 0) return -1;
    }
    out[p] = 0;
    return p;
}

static int split3(char *line, char **a, char **b, char **c) {
    char *t1 = 0, *t2 = 0;
    int i;
    for (i = 0; line[i]; i++) if (line[i] == '\t' && !t1) t1 = line + i;
    if (!t1) return -1;
    *t1 = 0;
    for (i = 1; t1[i]; i++) if (t1[i] == '\t') { t2 = t1 + i; break; }
    *a = line;
    if (!t2) { *b = t1 + 1; *c = t1 + i; return 0; }
    *t2 = 0;
    *b = t1 + 1;
    *c = t2 + 1;
    return 0;
}

int wf_nav_load(wf_nav_t *n, const char *text, int len) {
    static wf_nav_t tmp;
    int i = 0;
    if (!n || !text || len < 0) return -1;
    wf_nav_init(&tmp);
    while (i < len) {
        char line[WF_NAV_URL + WF_NAV_TITLE + 24];
        int k = 0, kind;
        while (i < len && text[i] != '\n' && k < (int)sizeof(line) - 1) line[k++] = text[i++];
        if (i < len && text[i] == '\n') i++;
        line[k] = 0;
        if (k == 0) continue;
        kind = line[0];
        if (kind == 'C' && line[1] == ' ') {
            int v = 0, p = 2;
            if (!ndigit(line[p]) && line[p] != '-') return -1;
            if (line[p] == '-') p++;
            while (ndigit(line[p])) { v = v * 10 + (line[p] - '0'); p++; }
            tmp.cursor = line[2] == '-' ? -1 : v;
        } else if (kind == 'S' || kind == 'B' || kind == 'H') {
            char *a, *b, *c;
            if (line[1] != ' ') return -1;
            if (split3(line + 2, &a, &b, &c) != 0) return -1;
            if (kind == 'S') {
                if (tmp.stack_n >= WF_NAV_STACK || bad_url(a)) return -1;
                ncpy(tmp.stack[tmp.stack_n].url, WF_NAV_URL, a);
                clean_title(tmp.stack[tmp.stack_n].title, WF_NAV_TITLE, b);
                clean_stamp(tmp.stack[tmp.stack_n].stamp, c);
                tmp.stack_n++;
            } else if (kind == 'B') {
                if (wf_nav_mark_add(&tmp, a, b) != 0) return -1;
            } else {
                wf_nav_ent_t e;
                int z;
                if (bad_url(a)) return -1;
                for (z = 0; z < (int)sizeof(e); z++) ((uint8_t *)&e)[z] = 0;
                ncpy(e.url, WF_NAV_URL, a);
                clean_title(e.title, WF_NAV_TITLE, b);
                clean_stamp(e.stamp, c);
                hist_push(&tmp, &e);
            }
        }
    }
    if (tmp.cursor >= tmp.stack_n) tmp.cursor = tmp.stack_n ? tmp.stack_n - 1 : -1;
    if (tmp.stack_n == 0) tmp.cursor = -1;
    nmove(n, &tmp, (uint32_t)sizeof(tmp));
    return 0;
}

static int cat_u(char *o, int cap, int p, uint32_t v) {
    char t[12];
    int n = 0;
    do { t[n++] = (char)('0' + (int)(v % 10U)); v /= 10U; } while (v && n < 11);
    while (n) { if (p >= cap - 1) return -1; o[p++] = t[--n]; }
    return p;
}

int wf_nav_text(const wf_nav_t *n, char *out, int cap) {
    int p = 0;
    if (!n || !out || cap < 8) return -1;
    p = app(out, cap, p, "nav visits ");
    p = cat_u(out, cap, p, n->visits);
    p = app(out, cap, p, " back ");
    p = cat_u(out, cap, p, n->backs);
    p = app(out, cap, p, " forward ");
    p = cat_u(out, cap, p, n->forwards);
    p = app(out, cap, p, " marks ");
    p = cat_u(out, cap, p, (uint32_t)n->nmark);
    p = app(out, cap, p, " hist ");
    p = cat_u(out, cap, p, (uint32_t)n->hist_n);
    p = app(out, cap, p, " cursor ");
    p = app_u(out, cap, p, n->cursor);
    if (p < 0) return -1;
    out[p] = 0;
    return p;
}
