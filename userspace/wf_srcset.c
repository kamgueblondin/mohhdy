/* wf_srcset.c - Lot 73: srcset, sizes, media and type selection (see
 * wf_srcset.h). */
#include "wf_srcset.h"

static int sp(char c) { return c == ' ' || c == '\t' || c == '\n' || c == '\r' || c == '\f'; }
static char low(char c) { return (c >= 'A' && c <= 'Z') ? (char)(c + 32) : c; }

/* s[0..n) equals the lower case word w */
static int word_is(const char *s, int n, const char *w) {
    int k;
    for (k = 0; k < n; k++) if (!w[k] || low(s[k]) != w[k]) return 0;
    return w[n] == 0;
}

/* a decimal number in s[0..n) times 100 (two decimals kept); -1 if bad */
static int num100(const char *s, int n) {
    int k = 0, v = 0, frac = 0, digits = 0;
    while (k < n && s[k] >= '0' && s[k] <= '9') { if (v < 100000) v = v * 10 + (s[k] - '0'); k++; digits++; }
    if (k < n && s[k] == '.') {
        int scale = 10;
        k++;
        while (k < n && s[k] >= '0' && s[k] <= '9') { frac += (s[k] - '0') * scale; scale /= 10; k++; digits++; }
    }
    if (!digits || k != n) return -1;
    return v * 100 + frac;
}

/* a CSS length in s[0..n) in px for a viewport of vw; -1 if bad */
static int length_px(const char *s, int n, int vw) {
    int u = n, v;
    while (u > 0 && ((s[u - 1] >= 'a' && s[u - 1] <= 'z') || (s[u - 1] >= 'A' && s[u - 1] <= 'Z'))) u--;
    v = num100(s, u);
    if (v < 0) return -1;
    if (word_is(s + u, n - u, "px")) return v / 100;
    if (word_is(s + u, n - u, "vw")) return (int)((long)v * vw / 10000);
    if (word_is(s + u, n - u, "em") || word_is(s + u, n - u, "rem")) return v * 16 / 100;
    if (u == n && v == 0) return 0;
    return -1;
}

/* one media feature "(min-width: Npx)" or a type word */
static int media_part(const char *s, int n, int vw) {
    int a = 0, k, colon = -1, px;
    while (n > 0 && sp(s[n - 1])) n--;
    while (a < n && sp(s[a])) a++;
    if (a >= n) return 1;
    if (word_is(s + a, n - a, "all") || word_is(s + a, n - a, "screen")) return 1;
    if (s[a] != '(' || s[n - 1] != ')') return 0;
    a++; n--;
    for (k = a; k < n; k++) if (s[k] == ':') { colon = k; break; }
    if (colon < 0) return 0;
    {
        int ne = colon, vs = colon + 1, ve = n;
        while (ne > a && sp(s[ne - 1])) ne--;
        while (a < ne && sp(s[a])) a++;
        while (vs < ve && sp(s[vs])) vs++;
        while (ve > vs && sp(s[ve - 1])) ve--;
        px = length_px(s + vs, ve - vs, vw);
        if (px < 0) return 0;
        if (word_is(s + a, ne - a, "min-width")) return vw >= px;
        if (word_is(s + a, ne - a, "max-width")) return vw <= px;
    }
    return 0;
}

/* a media query: parts joined by "and" */
static int media_query(const char *s, int n, int vw) {
    int k = 0, start = 0;
    while (k <= n) {
        if (k == n || (k + 5 <= n && sp(s[k]) && word_is(s + k + 1, 3, "and") && sp(s[k + 4]))) {
            if (!media_part(s + start, k - start, vw)) return 0;
            if (k == n) break;
            k += 5;
            start = k;
            continue;
        }
        k++;
    }
    return 1;
}

int wf_media_match(const char *media, int vw) {
    int k = 0, start = 0, any = 0;
    if (!media) return 1;
    for (;;) {
        if (media[k] == ',' || !media[k]) {
            int a = start, e = k;
            while (a < e && sp(media[a])) a++;
            if (a < e) { any = 1; if (media_query(media + a, e - a, vw)) return 1; }
            if (!media[k]) break;
            start = k + 1;
        }
        k++;
        if (k > 4096) return 0;
    }
    return !any;
}

int wf_sizes_width(const char *sizes, int vw) {
    int k = 0, start = 0;
    if (!sizes) return vw;
    for (;;) {
        if (sizes[k] == ',' || !sizes[k]) {
            int a = start, e = k, m = -1, px;
            while (a < e && sp(sizes[a])) a++;
            while (e > a && sp(sizes[e - 1])) e--;
            if (a < e) {
                if (sizes[a] == '(') {          /* media condition up to its closing parenthesis */
                    int depth = 0, j;
                    for (j = a; j < e; j++) {
                        if (sizes[j] == '(') depth++;
                        else if (sizes[j] == ')' && --depth == 0) m = j;
                    }
                    /* "(a) and (b) len": the length is the last word */
                    {
                        int l = e;
                        while (l > a && !sp(sizes[l - 1])) l--;
                        if (l > a && l - 1 >= m) {
                            px = length_px(sizes + l, e - l, vw);
                            if (px >= 0 && media_query(sizes + a, l - 1 - a, vw)) return px;
                        }
                    }
                } else {
                    px = length_px(sizes + a, e - a, vw);
                    if (px >= 0) return px;
                }
            }
            if (!sizes[k]) break;
            start = k + 1;
        }
        k++;
        if (k > 4096) break;
    }
    return vw;
}

int wf_srcset_pick(const char *srcset, const char *sizes, int vw, int dpr100, char *out, int cap) {
    int k = 0, best_d = -1, best_s = 0, best_n = 0, top_d = -1, top_s = 0, top_n = 0, slot = -1, guard = 0;
    if (cap < 1) return 0;
    out[0] = 0;
    if (!srcset) return 0;
    if (dpr100 <= 0) dpr100 = 100;
    while (srcset[k] && guard++ < WF_SRCSET_MAX_CAND * 4) {
        int us, ue, d = -1, bad = 0, descs = 0;
        while (srcset[k] && (sp(srcset[k]) || srcset[k] == ',')) k++;
        if (!srcset[k]) break;
        us = k;
        while (srcset[k] && !sp(srcset[k])) k++;
        ue = k;
        /* a URL ending with commas: they separate candidates */
        if (ue > us && srcset[ue - 1] == ',') { while (ue > us && srcset[ue - 1] == ',') ue--; d = 100; descs = -1; }
        if (descs == 0) {
            /* descriptors up to the next comma */
            for (;;) {
                int ds;
                while (srcset[k] && sp(srcset[k])) k++;
                if (!srcset[k] || srcset[k] == ',') { if (srcset[k]) k++; break; }
                ds = k;
                while (srcset[k] && !sp(srcset[k]) && srcset[k] != ',') k++;
                descs++;
                {
                    int dl = k - ds;
                    char unit = dl > 0 ? low(srcset[k - 1]) : 0;
                    int v = dl > 1 ? num100(srcset + ds, dl - 1) : -1;
                    if (v <= 0 || (unit != 'x' && unit != 'w')) bad = 1;
                    else if (unit == 'x') d = v;
                    else {
                        if (slot < 0) slot = wf_sizes_width(sizes, vw);
                        if (v % 100) bad = 1;
                        else d = slot > 0 ? (int)((long)v / slot) : 1000000;
                    }
                }
            }
            if (descs == 0) d = 100;
            if (descs > 1) bad = 1;
        }
        if (bad || ue <= us || d <= 0) continue;
        if (d >= dpr100 && (best_d < 0 || d < best_d)) { best_d = d; best_s = us; best_n = ue - us; }
        if (d > top_d) { top_d = d; top_s = us; top_n = ue - us; }
    }
    if (best_d < 0) { best_s = top_s; best_n = top_n; }
    if (top_d < 0) return 0;
    {
        int n = best_n < cap - 1 ? best_n : cap - 1, j;
        for (j = 0; j < n; j++) out[j] = srcset[best_s + j];
        out[n] = 0;
        return n;
    }
}

int wf_image_type_ok(const char *type) {
    static const char *const ok[] = { "image/png", "image/jpeg", "image/jpg", "image/gif", "image/bmp",
                                      "image/x-icon", "image/vnd.microsoft.icon", 0 };
    int n = 0, a = 0, k;
    if (!type) return 1;
    while (sp(type[a])) a++;
    while (type[a + n] && type[a + n] != ';' && !sp(type[a + n]) && n < 64) n++;
    if (!n) return 1;
    for (k = 0; ok[k]; k++) if (word_is(type + a, n, ok[k])) return 1;
    return 0;
}
