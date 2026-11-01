/* wf_cookie.c - Lot 71: see wf_cookie.h. */
#include "wf_cookie.h"

static int slen(const char* s) { int n = 0; while (s && s[n]) n++; return n; }
static char lc(char c) { return (c >= 'A' && c <= 'Z') ? (char)(c - 'A' + 'a') : c; }
static int ieq_n(const char* a, const char* b, int n) {
    int i;
    for (i = 0; i < n; i++) if (lc(a[i]) != lc(b[i]) || !b[i]) return 0;
    return 1;
}
static int seq(const char* a, const char* b) {
    int i;
    for (i = 0; a[i] || b[i]; i++) if (a[i] != b[i]) return 0;
    return 1;
}
static void ncpy(char* d, int cap, const char* s, int n, int lower) {
    int i;
    if (cap <= 0) return;
    for (i = 0; i < n && i < cap - 1 && s[i]; i++) d[i] = lower ? lc(s[i]) : s[i];
    d[i] = 0;
}
static int app(char* o, int cap, int p, const char* s) {
    while (p >= 0 && *s) { if (p >= cap - 1) return -1; o[p++] = *s++; }
    if (p >= 0 && p < cap) o[p] = 0;
    return p;
}
static int app_u(char* o, int cap, int p, uint32_t v) {
    char t[12];
    int n = 0;
    do { t[n++] = (char)('0' + v % 10U); v /= 10U; } while (v && n < 11);
    while (p >= 0 && n) { if (p >= cap - 1) return -1; o[p++] = t[--n]; }
    if (p >= 0 && p < cap) o[p] = 0;
    return p;
}
static int ws(char c) { return c == ' ' || c == '\t'; }

void wf_cookie_jar_init(wf_cookie_jar_t* j) {
    uint8_t* z = (uint8_t*)j;
    uint32_t i;
    if (!j) return;
    for (i = 0; i < (uint32_t)sizeof(*j); i++) z[i] = 0;
    j->enabled = 1;
}

/* RFC 6265 5.1.3: host equals domain, or ends with "." + domain and is a
 * name (not an IPv4 literal) */
static int is_ipv4(const char* h) {
    int i, dots = 0;
    for (i = 0; h[i]; i++) { if (h[i] == '.') dots++; else if (h[i] < '0' || h[i] > '9') return 0; }
    return dots == 3;
}
static int domain_match(const char* host, const char* dom) {
    int hl = slen(host), dl = slen(dom);
    if (!dl || dl > hl) return 0;
    if (!ieq_n(host + hl - dl, dom, dl)) return 0;
    if (hl == dl) return 1;
    return host[hl - dl - 1] == '.' && !is_ipv4(host);
}
/* RFC 6265 5.1.4: the request path, up to (not including) its last '/' */
static void default_path(const char* rp, char* out, int cap) {
    int n = 0, last = -1;
    if (!rp || rp[0] != '/') { ncpy(out, cap, "/", 1, 0); return; }
    while (rp[n] && rp[n] != '?' && rp[n] != '#') { if (rp[n] == '/') last = n; n++; }
    if (last <= 0) { ncpy(out, cap, "/", 1, 0); return; }
    ncpy(out, cap, rp, last, 0);
}
static int path_match(const char* rp, const char* cp) {
    int cl = slen(cp), i;
    for (i = 0; i < cl; i++) if (!rp[i] || rp[i] != cp[i]) return 0;
    if (!rp[cl] || rp[cl] == '?' || rp[cl] == '#') return 1;
    return cp[cl - 1] == '/' || rp[cl] == '/';
}

/* "Wed, 21 Oct 2015 07:28:00 GMT" (and the '-' variants) to
 * YYYYMMDDHHMMSS; 0 ok */
static int parse_date(const char* s, int n, char out[15]) {
    static const char* const mon[12] = { "jan","feb","mar","apr","may","jun","jul","aug","sep","oct","nov","dec" };
    int i = 0, day = -1, month = -1, year = -1, hh = -1, mm = -1, ss = -1;
    while (i < n) {
        int st, l, v = 0, digits = 0, k;
        while (i < n && !((s[i] >= '0' && s[i] <= '9') || (lc(s[i]) >= 'a' && lc(s[i]) <= 'z'))) i++;
        st = i;
        while (i < n && ((s[i] >= '0' && s[i] <= '9') || (lc(s[i]) >= 'a' && lc(s[i]) <= 'z') || s[i] == ':')) i++;
        l = i - st;
        if (!l) break;
        if (hh < 0 && l == 8 && s[st + 2] == ':' && s[st + 5] == ':') {
            hh = (s[st] - '0') * 10 + s[st + 1] - '0';
            mm = (s[st + 3] - '0') * 10 + s[st + 4] - '0';
            ss = (s[st + 6] - '0') * 10 + s[st + 7] - '0';
            continue;
        }
        for (k = 0; k < l && s[st + k] >= '0' && s[st + k] <= '9'; k++) { v = v * 10 + s[st + k] - '0'; digits++; }
        if (digits == l) {
            if (day < 0 && digits <= 2) day = v;
            else if (year < 0 && (digits == 2 || digits == 4)) year = digits == 2 ? (v < 70 ? 2000 + v : 1900 + v) : v;
            continue;
        }
        if (month < 0 && l >= 3) for (k = 0; k < 12; k++) if (ieq_n(s + st, mon[k], 3)) month = k + 1;
    }
    if (day < 1 || day > 31 || month < 1 || year < 1601 || hh < 0 || hh > 23 || mm > 59 || ss > 59) return -1;
    {
        int v[6], w[6] = { 4, 2, 2, 2, 2, 2 }, p = 0, f, d;
        v[0] = year; v[1] = month; v[2] = day; v[3] = hh; v[4] = mm; v[5] = ss;
        for (f = 0; f < 6; f++) {
            int x = v[f];
            for (d = w[f] - 1; d >= 0; d--) { out[p + d] = (char)('0' + x % 10); x /= 10; }
            p += w[f];
        }
        out[14] = 0;
    }
    return 0;
}
static int ts_before(const char* a, const char* b) {
    int i;
    for (i = 0; i < 14; i++) if (a[i] != b[i]) return a[i] < b[i];
    return 0;
}

static int find(const wf_cookie_jar_t* j, const char* name, const char* dom, const char* path) {
    int i;
    for (i = 0; i < WF_COOKIE_MAX; i++)
        if (j->c[i].used && seq(j->c[i].name, name) && seq(j->c[i].domain, dom) && seq(j->c[i].path, path)) return i;
    return -1;
}

int wf_cookie_set(wf_cookie_jar_t* j, const char* host, const char* req_path, int https, const char* v,
                  const char* now) {
    wf_cookie_t c;
    int n, i, eq = -1, end, del = 0, k;
    if (!j || !host || !v || !j->enabled) return 0;
    for (i = 0; i < (int)sizeof(c); i++) ((uint8_t*)&c)[i] = 0;
    n = slen(v);
    for (end = 0; end < n && v[end] != ';'; end++) if (v[end] == '=' && eq < 0) eq = end;
    if (eq <= 0) { j->refused++; return -1; }                 /* no name */
    {
        int ns = 0, ne = eq, vs = eq + 1, ve = end;
        while (ns < ne && ws(v[ns])) ns++;
        while (ne > ns && ws(v[ne - 1])) ne--;
        while (vs < ve && ws(v[vs])) vs++;
        while (ve > vs && ws(v[ve - 1])) ve--;
        if (ne == ns || ne - ns >= WF_COOKIE_NAME || ve - vs >= WF_COOKIE_VALUE) { j->refused++; return -1; }
        for (k = ns; k < ne; k++) if ((unsigned char)v[k] < 0x21u || v[k] == 0x7f || v[k] == '=' || v[k] == ',') { j->refused++; return -1; }
        for (k = vs; k < ve; k++) if ((unsigned char)v[k] < 0x20u || v[k] == 0x7f) { j->refused++; return -1; }
        ncpy(c.name, (int)sizeof(c.name), v + ns, ne - ns, 0);
        ncpy(c.value, (int)sizeof(c.value), v + vs, ve - vs, 0);
    }
    c.host_only = 1;
    ncpy(c.domain, (int)sizeof(c.domain), host, slen(host), 1);
    default_path(req_path, c.path, (int)sizeof(c.path));
    i = end;
    while (i < n) {
        int as, ae, an_e, av_s;
        i++;                                                  /* ';' */
        while (i < n && ws(v[i])) i++;
        as = i;
        while (i < n && v[i] != ';') i++;
        ae = i;
        while (ae > as && ws(v[ae - 1])) ae--;
        for (an_e = as; an_e < ae && v[an_e] != '='; an_e++) {}
        av_s = an_e < ae ? an_e + 1 : ae;
        {
            int al = an_e, vl;
            while (al > as && ws(v[al - 1])) al--;
            while (av_s < ae && ws(v[av_s])) av_s++;
            al -= as;
            vl = ae - av_s;
            if (al == 6 && ieq_n(v + as, "domain", 6) && vl > 0) {
                const char* d = v + av_s;
                int dots = 0;
                if (*d == '.') { d++; vl--; }
                if (vl <= 0 || vl >= WF_COOKIE_DOMAIN) { j->refused++; return -1; }
                for (k = 0; k < vl; k++) if (d[k] == '.') dots++;
                if (!dots) { j->refused++; return -1; }       /* a top-level domain */
                ncpy(c.domain, (int)sizeof(c.domain), d, vl, 1);
                if (!domain_match(host, c.domain)) { j->refused++; return -1; }
                c.host_only = 0;                              /* Domain given: subdomains too */
            } else if (al == 4 && ieq_n(v + as, "path", 4)) {
                if (vl > 0 && v[av_s] == '/' && vl < WF_COOKIE_PATH) ncpy(c.path, (int)sizeof(c.path), v + av_s, vl, 0);
            } else if (al == 6 && ieq_n(v + as, "secure", 6)) {
                c.secure = 1;
            } else if (al == 8 && ieq_n(v + as, "httponly", 8)) {
                c.http_only = 1;
            } else if (al == 8 && ieq_n(v + as, "samesite", 8)) {
                c.same_site = (vl == 3 && ieq_n(v + av_s, "lax", 3)) ? 1 : (vl == 6 && ieq_n(v + av_s, "strict", 6)) ? 2
                            : (vl == 4 && ieq_n(v + av_s, "none", 4)) ? 3 : 0;
            } else if (al == 7 && ieq_n(v + as, "max-age", 7) && vl > 0) {
                if (v[av_s] == '-' || (vl == 1 && v[av_s] == '0')) del = 1;
                else {
                    int allz = 1;
                    for (k = 0; k < vl; k++) if (v[av_s + k] != '0') allz = 0;
                    if (allz) del = 1;
                }
            } else if (al == 7 && ieq_n(v + as, "expires", 7) && vl > 0 && now && slen(now) == 14) {
                char ts[15];
                if (parse_date(v + av_s, vl, ts) == 0 && ts_before(ts, now)) del = 1;
            }
        }
    }
    if (c.secure && !https) { j->refused++; return -1; }
    k = find(j, c.name, c.domain, c.path);
    if (del) {
        if (k >= 0) { j->c[k].used = 0; j->deleted++; return 2; }
        return 0;
    }
    if (k < 0) {
        uint32_t best = 0xffffffffU;
        for (i = 0; i < WF_COOKIE_MAX; i++) if (!j->c[i].used) { k = i; break; }
        if (k < 0) {
            for (i = 0; i < WF_COOKIE_MAX; i++) if (j->c[i].order < best) { best = j->c[i].order; k = i; }
            j->evicted++;
        }
    } else j->replaced++;
    c.used = 1;
    c.order = ++j->clock;
    j->c[k] = c;
    j->set++;
    return 1;
}

int wf_cookie_from_head(wf_cookie_jar_t* j, const char* host, const char* req_path, int https, const char* h, int n,
                        const char* now) {
    int p = 0, stored = 0;
    char line[WF_COOKIE_NAME + WF_COOKIE_VALUE + WF_COOKIE_DOMAIN + WF_COOKIE_PATH + 160];
    if (!j || !h) return 0;
    while (p < n) {
        int ls = p, le;
        for (le = p; le < n && h[le] != '\n'; le++) {}
        p = le + 1;
        if (le > ls && h[le - 1] == '\r') le--;
        if (le - ls > 11 && ieq_n(h + ls, "set-cookie:", 11)) {
            int vs = ls + 11, l;
            while (vs < le && ws(h[vs])) vs++;
            l = le - vs;
            if (l >= (int)sizeof(line)) { j->refused++; continue; }
            ncpy(line, (int)sizeof(line), h + vs, l, 0);
            if (wf_cookie_set(j, host, req_path, https, line, now) == 1) stored++;
        }
    }
    return stored;
}

int wf_cookie_header(wf_cookie_jar_t* j, const char* host, const char* req_path, int https, char* out, int cap) {
    int pick[WF_COOKIE_MAX], np = 0, i, k, p = 0;
    if (!out || cap <= 0) return -1;
    out[0] = 0;
    if (!j || !host || !j->enabled) return 0;
    if (!req_path || !req_path[0]) req_path = "/";
    for (i = 0; i < WF_COOKIE_MAX; i++) {
        const wf_cookie_t* c = &j->c[i];
        if (!c->used || (c->secure && !https)) continue;
        if (c->host_only ? !(slen(host) == slen(c->domain) && ieq_n(host, c->domain, slen(c->domain))) : !domain_match(host, c->domain)) continue;
        if (!path_match(req_path, c->path)) continue;
        pick[np++] = i;
    }
    if (!np) return 0;
    /* RFC 6265 5.4: longer paths first, then the earlier set */
    for (i = 1; i < np; i++) {
        int x = pick[i];
        k = i - 1;
        while (k >= 0 && (slen(j->c[pick[k]].path) < slen(j->c[x].path) ||
                          (slen(j->c[pick[k]].path) == slen(j->c[x].path) && j->c[pick[k]].order > j->c[x].order))) {
            pick[k + 1] = pick[k];
            k--;
        }
        pick[k + 1] = x;
    }
    p = app(out, cap, p, "Cookie: ");
    for (i = 0; i < np && p >= 0; i++) {
        if (i) p = app(out, cap, p, "; ");
        p = app(out, cap, p, j->c[pick[i]].name);
        p = app(out, cap, p, "=");
        p = app(out, cap, p, j->c[pick[i]].value);
    }
    p = app(out, cap, p, "\r\n");
    if (p < 0) { out[0] = 0; return -1; }
    j->sent += (uint32_t)np;
    return p;
}

int wf_cookie_count(const wf_cookie_jar_t* j) {
    int i, n = 0;
    if (!j) return 0;
    for (i = 0; i < WF_COOKIE_MAX; i++) n += j->c[i].used ? 1 : 0;
    return n;
}

int wf_cookie_clear(wf_cookie_jar_t* j, const char* dom) {
    int i, n = 0;
    if (!j) return 0;
    for (i = 0; i < WF_COOKIE_MAX; i++) {
        if (!j->c[i].used) continue;
        if (dom && dom[0] && !domain_match(j->c[i].domain, dom)) continue;
        j->c[i].used = 0;
        n++;
    }
    return n;
}

void wf_cookie_list(const wf_cookie_jar_t* j, char* out, int cap) {
    int i, p = 0;
    if (!out || cap <= 0) return;
    out[0] = 0;
    if (!j) return;
    for (i = 0; i < WF_COOKIE_MAX && p >= 0; i++) {
        const wf_cookie_t* c = &j->c[i];
        if (!c->used) continue;
        p = app(out, cap, p, c->domain);
        p = app(out, cap, p, " ");
        p = app(out, cap, p, c->path);
        p = app(out, cap, p, " ");
        p = app(out, cap, p, c->name);
        p = app(out, cap, p, " len=");
        p = app_u(out, cap, p, (uint32_t)slen(c->value));
        if (c->secure) p = app(out, cap, p, " secure");
        if (c->host_only) p = app(out, cap, p, " host-only");
        if (c->http_only) p = app(out, cap, p, " httponly");
        p = app(out, cap, p, "\n");
    }
    if (p < 0 && cap > 4) { out[cap - 4] = '.'; out[cap - 3] = '.'; out[cap - 2] = '\n'; out[cap - 1] = 0; }
}
