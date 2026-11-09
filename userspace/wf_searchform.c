/* wf_searchform.c - Lot 85. See wf_searchform.h. Freestanding, no heap. */
#include "wf_searchform.h"

static char lc(char c) { return (c >= 'A' && c <= 'Z') ? (char)(c + 32) : c; }

static int eq(const char *a, const char *b) {
    if (!a || !b) return 0;
    while (*a && *a == *b) { a++; b++; }
    return *a == *b;
}

static int eq_n(const char *a, int n, const char *b) {
    int i = 0;
    while (b[i] && i < n && lc(a[i]) == b[i]) i++;
    return b[i] == 0 && i == n;
}

static int starts(const char *a, int n, const char *b) {
    int i = 0;
    while (b[i] && i < n && lc(a[i]) == b[i]) i++;
    return b[i] == 0;
}

int wf_search_field_name(const char *name) {
    return eq(name, "q") || eq(name, "query") || eq(name, "search") || eq(name, "s");
}

/* Higher wins when a form has more than one of these names. "s" is the
 * weakest: some pages use it for a short search box, others for state. */
static int name_rank(const char *name) {
    if (eq(name, "q")) return 4;
    if (eq(name, "query") || eq(name, "search")) return 3;
    if (eq(name, "s")) return 1;
    return 0;
}

static int hex(char c) {
    if (c >= '0' && c <= '9') return c - '0';
    if (c >= 'a' && c <= 'f') return c - 'a' + 10;
    if (c >= 'A' && c <= 'F') return c - 'A' + 10;
    return -1;
}

static int dec_copy(const char *s, int n, char *out, int cap) {
    int i = 0, o = 0;
    if (!out || cap < 2) return 0;
    while (i < n && o < cap - 1) {
        if (s[i] == '+') { out[o++] = ' '; i++; }
        else if (s[i] == '%' && i + 2 < n) {
            int h = hex(s[i + 1]), l = hex(s[i + 2]);
            if (h >= 0 && l >= 0) {
                char c = (char)(h * 16 + l);
                if (c && c != '\r' && c != '\n') out[o++] = c;
                i += 3;
            } else out[o++] = s[i++];
        } else out[o++] = s[i++];
    }
    out[o] = 0;
    return o;
}

/* Scan a query string or a POST body. Keeps the highest-ranked search
 * name. Returns that rank, or 0. */
static int take_pairs(const char *s, char *query, int cap) {
    int i = 0, best = 0;
    if (query && cap > 0) query[0] = 0;
    if (!s) return 0;
    while (s[i] && s[i] != '#') {
        int ns, ne, vs, ve, r;
        char name[40];
        if (s[i] == '?' || s[i] == '&') i++;
        if (!s[i] || s[i] == '#') break;
        ns = i;
        while (s[i] && s[i] != '=' && s[i] != '&' && s[i] != '#') i++;
        ne = i;
        if (s[i] == '=') i++;
        vs = i;
        while (s[i] && s[i] != '&' && s[i] != '#') i++;
        ve = i;
        if (ne <= ns || ne - ns >= (int)sizeof(name)) continue;
        dec_copy(s + ns, ne - ns, name, (int)sizeof(name));
        r = name_rank(name);
        if (r > best) {
            char val[200];
            dec_copy(s + vs, ve - vs, val, (int)sizeof(val));
            if (val[0]) {
                int k = 0;
                best = r;
                if (query && cap > 1) {
                    while (val[k] && k < cap - 1) { query[k] = val[k]; k++; }
                    query[k] = 0;
                }
            }
        }
    }
    return best;
}

static int host_of(const char *url, const char **host, int *len) {
    int i = 0;
    if (!url) return 0;
    if (starts(url, 8, "https://")) i = 8;
    else if (starts(url, 7, "http://")) i = 7;
    else return 0;
    *host = url + i;
    while (url[i] && url[i] != '/' && url[i] != '?' && url[i] != ':' && url[i] != '#') i++;
    *len = (int)(url + i - *host);
    return *len > 0;
}

static int host_is(const char *host, int n, const char *root) {
    int rl = 0, i;
    while (root[rl]) rl++;
    if (n == rl) {
        for (i = 0; i < n; i++) if (lc(host[i]) != root[i]) return 0;
        return 1;
    }
    if (n > rl && host[n - rl - 1] == '.') {
        for (i = 0; i < rl; i++) if (lc(host[n - rl + i]) != root[i]) return 0;
        return 1;
    }
    return 0;
}

/* www.google.TLD, google.TLD, TLD com / two letters / co.XX / com.XX.
 * The same shapes wsearch_google_form accepts, so a POST to the same
 * host is the same engine. */
static int host_google(const char *host, int n) {
    int s = 0, r, len, k, ok = 0;
    if (n > 4 && eq_n(host, 4, "www.")) s = 4;
    if (n - s < 8 || !eq_n(host + s, 7, "google.")) return 0;
    r = s + 7;
    len = n - r;
    for (k = r; k < n; k++) {
        char c = lc(host[k]);
        if (!((c >= 'a' && c <= 'z') || c == '.')) return 0;
    }
    if (len == 3 && eq_n(host + r, 3, "com")) ok = 1;
    else if (len == 2 && host[r] != '.' && host[r + 1] != '.') ok = 1;
    else if (len == 5 && eq_n(host + r, 3, "co.") && host[r + 3] != '.' && host[r + 4] != '.') ok = 1;
    else if (len == 6 && eq_n(host + r, 4, "com.") && host[r + 4] != '.' && host[r + 5] != '.') ok = 1;
    return ok;
}

static const char *query_part(const char *url) {
    if (!url) return 0;
    while (*url && *url != '?' && *url != '#') url++;
    return *url == '?' ? url : 0;
}

int wf_search_route(const char *url, const char *body, char *query, int cap) {
    const char *host = 0, *qs;
    int hlen = 0, got = 0;
    char q[200];
    q[0] = 0;
    if (query && cap > 0) query[0] = 0;
    if (body && body[0]) got = take_pairs(body, q, (int)sizeof(q));
    if (!got) {
        qs = query_part(url);
        if (qs) got = take_pairs(qs, q, (int)sizeof(q));
    }
    if (!got || !q[0]) return WF_ROUTE_FETCH;
    if (!host_of(url, &host, &hlen)) return WF_ROUTE_FETCH;
    if (query && cap > 1) {
        int i = 0;
        while (q[i] && i < cap - 1) { query[i] = q[i]; i++; }
        query[i] = 0;
    }
    if (host_is(host, hlen, "wikipedia.org")) return WF_ROUTE_WIKI;
    if (host_google(host, hlen)) return WF_ROUTE_GOOGLE;
    return WF_ROUTE_FETCH;
}
