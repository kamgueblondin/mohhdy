/* wf_cssbg.c - Lot 90: see wf_cssbg.h. */
#include "wf_cssbg.h"

static char bl(char c) { return (c >= 'A' && c <= 'Z') ? (char)(c + 32) : c; }
static int bsp(char c) { return c == ' ' || c == '\t' || c == '\n' || c == '\r'; }
static int bslen(const char *s) { int n = 0; while (s[n]) n++; return n; }

/* n bytes at s equal the lower case word w */
static int bis(const char *s, int n, const char *w) {
    int i;
    for (i = 0; i < n; i++) if (!w[i] || bl(s[i]) != w[i]) return 0;
    return w[n] == 0;
}

static int bpre(const char *s, int n, const char *w) {
    int i;
    for (i = 0; w[i]; i++) if (i >= n || bl(s[i]) != w[i]) return 0;
    return 1;
}

static void btrim(const char *s, int *a, int *b) {
    while (*a < *b && bsp(s[*a])) (*a)++;
    while (*b > *a && bsp(s[*b - 1])) (*b)--;
}

typedef struct { char *o; int cap, n, bad; } bo_t;

static void bo_c(bo_t *b, char c) {
    if (b->n >= b->cap - 1) { b->bad = 1; return; }
    b->o[b->n++] = c;
    b->o[b->n] = 0;
}
static void bo_s(bo_t *b, const char *s, int n) { int i; for (i = 0; i < n; i++) bo_c(b, s[i]); }
static void bo_low(bo_t *b, const char *s, int n) { int i; for (i = 0; i < n; i++) bo_c(b, bl(s[i])); }
static int bo_end(bo_t *b) {
    if (b->bad || b->n == 0) { if (b->cap > 0) b->o[0] = 0; return 0; }
    return b->n;
}
static void bo_init(bo_t *b, char *o, int cap) { b->o = o; b->cap = cap; b->n = 0; b->bad = cap < 2; if (cap > 0) o[0] = 0; }

/* Splits s[a..b) at top-level separators: commas (sep ',') or spaces
 * (sep ' ', with '/' as a word of its own when slash is set). Parentheses
 * nest; an unbalanced value gives -1. Returns the item count (at most
 * max; more is -1), items as [ia[k], ib[k]) trimmed. */
static int bsplit(const char *s, int a, int b, char sep, int slash, int *ia, int *ib, int max) {
    int i = a, depth = 0, c = 0, start = a;
    if (sep == ',') {
        for (i = a; i <= b; i++) {
            if (i < b && s[i] == '(') depth++;
            else if (i < b && s[i] == ')') { if (--depth < 0) return -1; }
            else if (i == b || (s[i] == ',' && depth == 0)) {
                int x = start, y = i;
                btrim(s, &x, &y);
                if (x >= y || c >= max) return -1;
                ia[c] = x; ib[c] = y; c++;
                start = i + 1;
            }
        }
        return depth ? -1 : c;
    }
    while (i < b) {
        while (i < b && bsp(s[i])) i++;
        if (i >= b) break;
        if (slash && s[i] == '/') {
            if (c >= max) return -1;
            ia[c] = i; ib[c] = i + 1; c++;
            i++;
            continue;
        }
        start = i;
        depth = 0;
        while (i < b && (depth > 0 || (!bsp(s[i]) && !(slash && s[i] == '/')))) {
            if (s[i] == '(') depth++;
            else if (s[i] == ')') { if (--depth < 0) return -1; }
            i++;
        }
        if (depth) return -1;
        if (c >= max) return -1;
        ia[c] = start; ib[c] = i; c++;
    }
    return c;
}

/* ---- url() ---------------------------------------------------------- */

static int url_byte(char c) {
    if (c < 0x21 || c > 0x7e) return 0;
    return c != '"' && c != '\'' && c != '(' && c != ')' && c != '\\' && c != ';' && c != '<' && c != '>';
}

int wf_cssbg_url(const char *v, int n, char *out, int cap) {
    int a = 0, b = n, x, y, i;
    bo_t o;
    bo_init(&o, out, cap);
    if (!v) return 0;
    btrim(v, &a, &b);
    if (b - a < 6 || !bpre(v + a, b - a, "url(") || v[b - 1] != ')') return 0;
    x = a + 4; y = b - 1;
    btrim(v, &x, &y);
    if (y - x >= 2 && (v[x] == '"' || v[x] == '\'') && v[y - 1] == v[x]) { x++; y--; }
    if (y <= x || y - x > WF_CSSBG_URL) return 0;
    for (i = x; i < y; i++) if (!url_byte(v[i])) return 0;
    bo_s(&o, "url(", 4);
    bo_s(&o, v + x, y - x);
    bo_c(&o, ')');
    return bo_end(&o);
}

/* ---- colours ---------------------------------------------------------- */

static int hexd(char c) { c = bl(c); return (c >= '0' && c <= '9') || (c >= 'a' && c <= 'f'); }

/* digits [. digits] [%] */
static int cnum(const char *s, int n) {
    int i = 0, d = 0;
    while (i < n && s[i] >= '0' && s[i] <= '9') { i++; d++; }
    if (i < n && s[i] == '.') {
        int f = 0;
        i++;
        while (i < n && s[i] >= '0' && s[i] <= '9') { i++; f++; }
        if (!f) return 0;
        d += f;
    }
    if (i < n && s[i] == '%') i++;
    return d > 0 && i == n;
}

int wf_cssbg_color(const char *v, int n) {
    static const char *const names[] = { "black", "white", "red", "green", "blue", "yellow", "gray", "grey",
                                         "navy", "teal", "orange", "purple", "silver", "maroon", "lime",
                                         "transparent", 0 };
    int a = 0, b = n, i, k;
    if (!v) return 0;
    btrim(v, &a, &b);
    if (b <= a) return 0;
    if (v[a] == '#') {
        int h = b - a - 1;
        for (i = a + 1; i < b; i++) if (!hexd(v[i])) return 0;
        return h == 3 || h == 4 || h == 6 || h == 8;
    }
    for (k = 0; names[k]; k++) if (bis(v + a, b - a, names[k])) return 1;
    if ((bpre(v + a, b - a, "rgb(") || bpre(v + a, b - a, "rgba(")) && v[b - 1] == ')') {
        int s = a + (bl(v[a + 3]) == 'a' ? 5 : 4), e = b - 1, cnt = 0;
        i = s;
        while (i < e) {
            int t;
            while (i < e && (bsp(v[i]) || v[i] == ',' || v[i] == '/')) i++;
            if (i >= e) break;
            t = i;
            while (i < e && !bsp(v[i]) && v[i] != ',' && v[i] != '/') i++;
            if (!cnum(v + t, i - t)) return 0;
            cnt++;
        }
        return cnt == 3 || cnt == 4;
    }
    return 0;
}

/* ---- lengths, positions ----------------------------------------------- */

/* -?digits[.d{1,2}](px|%)? ; a unitless value only when it is zero.
 * Returns 1 valid (*neg, *zero set), 0 not a length. */
static int blen(const char *s, int n, int pct, int neg_ok, int *zero) {
    int i = 0, d = 0, v = 0, nz = 0, f = 0;
    *zero = 0;
    if (i < n && s[i] == '-') { if (!neg_ok) return 0; i++; }
    while (i < n && s[i] >= '0' && s[i] <= '9') {
        if (s[i] != '0') nz = 1;
        v = v * 10 + (s[i] - '0');
        if (v > 9999) return 0;
        i++; d++;
    }
    if (i < n && s[i] == '.') {
        i++;
        while (i < n && s[i] >= '0' && s[i] <= '9') { if (s[i] != '0') nz = 1; i++; f++; }
        if (f < 1 || f > 2) return 0;
        d += f;
    }
    if (!d) return 0;
    *zero = !nz;
    if (i == n) return !nz;
    if (bis(s + i, n - i, "px")) return 1;
    if (pct && s[i] == '%' && i + 1 == n) return 1;
    return 0;
}

/* position word: 1 left, 2 right, 3 top, 4 bottom, 5 center, 6 length */
static int pword(const char *s, int n) {
    int z;
    if (bis(s, n, "left")) return 1;
    if (bis(s, n, "right")) return 2;
    if (bis(s, n, "top")) return 3;
    if (bis(s, n, "bottom")) return 4;
    if (bis(s, n, "center")) return 5;
    if (blen(s, n, 1, 1, &z)) return 6;
    return 0;
}

/* one position of nw words (1 or 2) as "X Y" */
static int pos_words(const char *s, const int *wa, const int *wb, int nw, bo_t *o) {
    int k0, k1, x, y;
    if (nw == 1) {
        k0 = pword(s + wa[0], wb[0] - wa[0]);
        if (!k0) return 0;
        if (k0 == 3 || k0 == 4) { bo_s(o, "center ", 7); bo_low(o, s + wa[0], wb[0] - wa[0]); return 1; }
        bo_low(o, s + wa[0], wb[0] - wa[0]);
        bo_s(o, " center", 7);
        return 1;
    }
    if (nw != 2) return 0;
    k0 = pword(s + wa[0], wb[0] - wa[0]);
    k1 = pword(s + wa[1], wb[1] - wa[1]);
    if (!k0 || !k1) return 0;
    if ((k0 == 1 || k0 == 2 || k0 == 5 || k0 == 6) && (k1 == 3 || k1 == 4 || k1 == 5 || k1 == 6)) { x = 0; y = 1; }
    else if ((k0 == 3 || k0 == 4 || k0 == 5) && (k1 == 1 || k1 == 2 || k1 == 5)) { x = 1; y = 0; }
    else return 0;
    bo_low(o, s + wa[x], wb[x] - wa[x]);
    bo_c(o, ' ');
    bo_low(o, s + wa[y], wb[y] - wa[y]);
    return 1;
}

static int size_words(const char *s, const int *wa, const int *wb, int nw, bo_t *o) {
    int k, z, autos = 0;
    if (nw == 1 && (bis(s + wa[0], wb[0] - wa[0], "cover") || bis(s + wa[0], wb[0] - wa[0], "contain"))) {
        bo_low(o, s + wa[0], wb[0] - wa[0]);
        return 1;
    }
    if (nw < 1 || nw > 2) return 0;
    for (k = 0; k < nw; k++) {
        if (bis(s + wa[k], wb[k] - wa[k], "auto")) { autos++; continue; }
        if (!blen(s + wa[k], wb[k] - wa[k], 1, 0, &z)) return 0;
    }
    if (autos == nw) { bo_s(o, "auto", 4); return 1; }
    for (k = 0; k < nw; k++) {
        if (k) bo_c(o, ' ');
        bo_low(o, s + wa[k], wb[k] - wa[k]);
    }
    return 1;
}

/* repeat word: 1 repeat, 2 no-repeat, 3 space, 4 round, 5 repeat-x, 6 repeat-y */
static int rword(const char *s, int n) {
    static const char *const w[] = { "repeat", "no-repeat", "space", "round", "repeat-x", "repeat-y", 0 };
    int k;
    for (k = 0; w[k]; k++) if (bis(s, n, w[k])) return k + 1;
    return 0;
}

static int repeat_words(const char *s, const int *wa, const int *wb, int nw, bo_t *o) {
    static const char *const w[] = { "", "repeat", "no-repeat", "space", "round", "repeat-x", "repeat-y" };
    int r0, r1;
    if (nw == 1) {
        r0 = rword(s + wa[0], wb[0] - wa[0]);
        if (!r0) return 0;
        bo_s(o, w[r0], bslen(w[r0]));
        return 1;
    }
    if (nw != 2) return 0;
    r0 = rword(s + wa[0], wb[0] - wa[0]);
    r1 = rword(s + wa[1], wb[1] - wa[1]);
    if (!r0 || !r1 || r0 > 4 || r1 > 4) return 0;
    if (r0 == r1) { bo_s(o, w[r0], bslen(w[r0])); return 1; }
    if (r0 == 1 && r1 == 2) { bo_s(o, "repeat-x", 8); return 1; }
    if (r0 == 2 && r1 == 1) { bo_s(o, "repeat-y", 8); return 1; }
    bo_s(o, w[r0], bslen(w[r0]));
    bo_c(o, ' ');
    bo_s(o, w[r1], bslen(w[r1]));
    return 1;
}

/* ---- gradients -------------------------------------------------------- */

/* writes s[a..b) lower case with each run of spaces as one space */
static void bo_collapse(bo_t *o, const char *s, int a, int b) {
    int i, sp = 0;
    for (i = a; i < b; i++) {
        if (bsp(s[i])) { sp = 1; continue; }
        if (sp && o->n > 0) bo_c(o, ' ');
        sp = 0;
        bo_c(o, bl(s[i]));
    }
}

/* n / d (d > 0) by shift and subtract: the freestanding guest has no
 * 64-bit division helper */
static uint64_t bdiv64(uint64_t n, uint64_t d) {
    uint64_t q = 0, r = 0;
    int b;
    for (b = 63; b >= 0; b--) {
        r = (r << 1) | ((n >> b) & 1u);
        if (r >= d) { r -= d; q |= (uint64_t)1 << b; }
    }
    return q;
}

/* Lot 92, Lot 94: radians, at most two integer digits and four decimals,
 * as whole degrees (x * 180 / pi rounded half away from zero; the engine's
 * own factor 57.3 is not used). 1 and *deg set, -1 any other radian
 * value, 0 when s is not one. */
#define WF_DEG_PER_RAD 5729577951ULL  /* degrees per 1e-4 rad, in 1e-12 */
static int rad_degrees(const char *s, int n, int *deg) {
    int i = 0, neg = 0, v = 0, d = 0, fd = 0;
    /* "grad" also ends in "rad": the unit follows a digit or a point */
    if (n < 4 || !bis(s + n - 3, 3, "rad") || !((s[n - 4] >= '0' && s[n - 4] <= '9') || s[n - 4] == '.')) return 0;
    n -= 3;
    if (s[i] == '-') { neg = 1; i++; }
    while (i < n && s[i] >= '0' && s[i] <= '9') { v = v * 10 + (s[i] - '0'); if (++d > 2) return -1; i++; }
    v *= 10000;
    if (i < n && s[i] == '.') {
        int place = 1000;
        for (i++; i < n && s[i] >= '0' && s[i] <= '9'; i++) {
            if (++fd > 4) return -1;
            v += (s[i] - '0') * place;
            place /= 10;
        }
        if (!fd) return -1;
    }
    if (i != n || !(d + fd)) return -1;
    /* v is in 1e-4 rad; degrees = v * 0.005729577951 (1e-12 units) */
    *deg = (int)bdiv64((uint64_t)v * WF_DEG_PER_RAD + 500000000000ULL, 1000000000000ULL);
    if (neg) *deg = -*deg;
    return 1;
}

/* Lot 94: any angle the engine's tf_angle reads exactly: 1 an angle, 0
 * not an angle, -1 an angle out of range or with too many decimals.
 * deg: |v| <= 3600 with at most four decimals; turn: |v| <= 10 and grad:
 * |v| <= 4000, at most two decimals. (Lots 90-92 took only quarter turns.) */
static int angle(const char *s, int n) {
    int i = 0, v = 0, f = 0, fd = 0, d = 0;
    if (i < n && s[i] == '-') i++;
    while (i < n && s[i] >= '0' && s[i] <= '9') { v = v * 10 + (s[i] - '0'); if (v > 9999) return -1; i++; d++; }
    if (i < n && s[i] == '.') {
        i++;
        while (i < n && s[i] >= '0' && s[i] <= '9') { if (fd < 4) { f = f * 10 + (s[i] - '0'); fd++; } else return -1; i++; d++; }
        if (!fd) return -1;
    }
    if (!d) return 0;
    if (bis(s + i, n - i, "deg")) return (v < 3600 || (v == 3600 && f == 0)) ? 1 : -1;
    if (bis(s + i, n - i, "turn")) return fd <= 2 && (v < 10 || (v == 10 && f == 0)) ? 1 : -1;
    if (bis(s + i, n - i, "grad")) return fd <= 2 && (v < 4000 || (v == 4000 && f == 0)) ? 1 : -1;
    return -1;
}

/* Lot 94: "to" with one side, or one vertical and one horizontal side in
 * either order (a corner); s[a..b) follows "to" */
static int to_sides(const char *s, int a, int b) {
    int wa[3], wb[3], c = bsplit(s, a, b, ' ', 0, wa, wb, 3), k, vert = 0, horiz = 0;
    if (c < 1) return 0;                    /* three words repeat an axis below */
    for (k = 0; k < c; k++) {
        int l = wb[k] - wa[k];
        if (bis(s + wa[k], l, "top") || bis(s + wa[k], l, "bottom")) vert++;
        else if (bis(s + wa[k], l, "left") || bis(s + wa[k], l, "right")) horiz++;
        else return 0;
    }
    return vert <= 1 && horiz <= 1;
}

/* Lot 94: the conic first argument, "from ANGLE", "at POSITION" or both
 * in that order: 1 it is one, 0 it is not, -1 not valid. A radian "from"
 * is not taken (the reader writes radians only as a linear angle). */
static int conic_shape(const char *s, int a, int b) {
    int wa[8], wb[8], c = bsplit(s, a, b, ' ', 0, wa, wb, 8), k = 0;
    char t[40];
    bo_t o;
    if (c < 1) return -1;
    if (bis(s + wa[0], wb[0] - wa[0], "from")) {
        if (c < 2 || angle(s + wa[1], wb[1] - wa[1]) != 1) return -1;
        k = 2;
        if (c == 2) return 1;
    }
    if (!bis(s + wa[k], wb[k] - wa[k], "at")) return 0;     /* "from A x": the stop check refuses it */
    if (k + 1 >= c) return -1;
    bo_init(&o, t, (int)sizeof(t));
    return pos_words(s, wa + k + 1, wb + k + 1, c - k - 1, &o) ? 1 : -1;
}

/* "colour [N%]" with N 0..100 and at most one decimal; Lot 94: a conic
 * stop may also be "colour Ndeg" with N a whole number 0..360 */
static int stop_ok(const char *s, int a, int b, int conic) {
    int wa[3], wb[3], c = bsplit(s, a, b, ' ', 0, wa, wb, 3);
    if (c < 1 || c > 2 || !wf_cssbg_color(s + wa[0], wb[0] - wa[0])) return 0;
    if (c == 2 && conic && wb[1] - wa[1] > 3 && bis(s + wb[1] - 3, 3, "deg")) {
        int i = wa[1], v = 0;
        for (; i < wb[1] - 3; i++) {
            if (s[i] < '0' || s[i] > '9' || i - wa[1] >= 3) return 0;
            v = v * 10 + (s[i] - '0');
        }
        return v <= 360;
    }
    if (c == 2) {
        int i = wa[1], v = 0, d = 0;
        while (i < wb[1] && s[i] >= '0' && s[i] <= '9') { v = v * 10 + (s[i] - '0'); if (v > 100) return 0; i++; d++; }
        if (!d) return 0;
        if (i < wb[1] && s[i] == '.') {
            i++;
            if (i >= wb[1] || s[i] < '0' || s[i] > '9') return 0;
            if (v == 100 && s[i] != '0') return 0;
            i++;
        }
        if (i + 1 != wb[1] || s[i] != '%') return 0;
    }
    return 1;
}

/* radial shape argument: 1 it is one (valid), 0 it is not a shape
 * argument, -1 a shape argument that is not valid.
 * Lot 91: explicit radii as the engine draws them (webeng radial_shape):
 * circle with one length (px, not a percentage), ellipse with two
 * lengths or percentages; with no shape word one length is a circle and
 * two are an ellipse. The two radii are adjacent, sizes and an extent
 * keyword do not mix, and a radius is never negative. */
static int radial_shape(const char *s, int a, int b) {
    static const char *const ext[] = { "closest-side", "closest-corner", "farthest-side", "farthest-corner", 0 };
    int wa[8], wb[8], c = bsplit(s, a, b, ' ', 0, wa, wb, 8), k, form = -1, extent = 0, j;
    int nsz = 0, pct0 = 0, prev_size = 0, sizes_done = 0;
    if (c < 1) return -1;
    for (k = 0; k < c; k++) {
        int l = wb[k] - wa[k], z;
        if (bis(s + wa[k], l, "at")) {
            char t[40];
            bo_t o;
            if (k == c - 1) return -1;
            bo_init(&o, t, (int)sizeof(t));
            if (!pos_words(s, wa + k + 1, wb + k + 1, c - k - 1, &o)) return -1;
            break;
        }
        if (bis(s + wa[k], l, "circle") || bis(s + wa[k], l, "ellipse")) {
            if (form >= 0) return -1;
            form = bis(s + wa[k], l, "circle");
            if (nsz) sizes_done = 1;
            prev_size = 0;
            continue;
        }
        for (j = 0; ext[j] && !bis(s + wa[k], l, ext[j]); j++) {}
        if (ext[j]) {
            if (extent || nsz) return -1;
            extent = 1;
            prev_size = 0;
            continue;
        }
        if (blen(s + wa[k], l, 1, 0, &z)) {
            if (extent || sizes_done || nsz >= 2 || (nsz == 1 && !prev_size)) return -1;
            if (nsz == 0) pct0 = s[wb[k] - 1] == '%';
            nsz++;
            prev_size = 1;
            continue;
        }
        return k == 0 ? 0 : -1;
    }
    if (form == 1 && (nsz == 2 || (nsz == 1 && pct0))) return -1;
    if (form == 0 && nsz == 1) return -1;
    if (form < 0 && nsz == 1 && pct0) return -1;
    return 1;
}

int wf_cssbg_gradient(const char *v, int n, char *out, int cap) {
    int a = 0, b = n, ia[6], ib[6], c, k, first = 0, radial, conic, rad = 0, degrees = 0, pl = 16;
    bo_t o;
    bo_init(&o, out, cap);
    if (!v) return 0;
    btrim(v, &a, &b);
    radial = bpre(v + a, b - a, "radial-gradient(");
    conic = bpre(v + a, b - a, "conic-gradient(");
    if (conic) pl = 15;
    if (!radial && !conic && !bpre(v + a, b - a, "linear-gradient(")) return 0;
    if (v[b - 1] != ')') return 0;
    c = bsplit(v, a + pl, b - 1, ',', 0, ia, ib, 6);
    if (c < 2) return 0;
    if (conic) {
        int r = conic_shape(v, ia[0], ib[0]);
        if (r < 0) return 0;
        first = r;
    } else if (radial) {
        int r = radial_shape(v, ia[0], ib[0]);
        if (r < 0) return 0;
        first = r;
    } else {
        int l = ib[0] - ia[0];
        if (bpre(v + ia[0], l, "to ") || bpre(v + ia[0], l, "to\t")) {
            if (!to_sides(v, ia[0] + 3, ib[0])) return 0;
            first = 1;
        } else {
            int g = rad_degrees(v + ia[0], l, &degrees);
            if (g == 0) g = angle(v + ia[0], l);
            else if (g > 0) rad = 1;
            if (g < 0) return 0;
            first = g;
        }
    }
    if (c - first < 2 || c - first > 4) return 0;
    for (k = first; k < c; k++) if (!stop_ok(v, ia[k], ib[k], conic)) return 0;
    bo_s(&o, conic ? "conic-gradient(" : radial ? "radial-gradient(" : "linear-gradient(", pl);
    for (k = 0; k < c; k++) {
        if (k) bo_s(&o, ", ", 2);
        if (k == 0 && rad) {
            /* Lot 92: a radian angle is written as its whole degrees */
            char deg[8];
            int dn = 0, dv = degrees < 0 ? -degrees : degrees, p = 1000;
            if (degrees < 0) deg[dn++] = '-';
            while (p > 1 && dv < p) p /= 10;
            for (; p; p /= 10) deg[dn++] = (char)('0' + dv / p % 10);
            bo_s(&o, deg, dn);
            bo_s(&o, "deg", 3);
        } else bo_collapse(&o, v, ia[k], ib[k]);
    }
    bo_c(&o, ')');
    return bo_end(&o);
}

/* ---- lists ------------------------------------------------------------- */

/* image item kind: 1 none, 2 url, 3 gradient; 0 invalid. Writes it. */
static int image_item(const char *s, int a, int b, bo_t *o) {
    char t[WF_CSSBG_VAL];
    int l;
    if (bis(s + a, b - a, "none")) { bo_s(o, "none", 4); return 1; }
    l = wf_cssbg_url(s + a, b - a, t, (int)sizeof(t));
    if (l) { bo_s(o, t, l); return 2; }
    l = wf_cssbg_gradient(s + a, b - a, t, (int)sizeof(t));
    if (l) { bo_s(o, t, l); return 3; }
    return 0;
}

int wf_cssbg_image(const char *v, char *out, int cap) {
    int ia[WF_CSSBG_LAYERS], ib[WF_CSSBG_LAYERS], c, k;
    bo_t o;
    bo_init(&o, out, cap);
    if (!v) return 0;
    c = bsplit(v, 0, bslen(v), ',', 0, ia, ib, WF_CSSBG_LAYERS);
    if (c < 1) return 0;
    for (k = 0; k < c; k++) {
        int kind;
        if (k) bo_s(&o, ", ", 2);
        kind = image_item(v, ia[k], ib[k], &o);
        if (!kind || (kind == 3 && k != c - 1)) { o.bad = 1; break; }
    }
    return bo_end(&o);
}

/* kind 0 position, 1 size, 2 repeat */
static int list_of(const char *v, char *out, int cap, int kind) {
    int ia[WF_CSSBG_LAYERS], ib[WF_CSSBG_LAYERS], c, k;
    bo_t o;
    bo_init(&o, out, cap);
    if (!v) return 0;
    c = bsplit(v, 0, bslen(v), ',', 0, ia, ib, WF_CSSBG_LAYERS);
    if (c < 1) return 0;
    for (k = 0; k < c; k++) {
        int wa[3], wb[3], nw = bsplit(v, ia[k], ib[k], ' ', 0, wa, wb, 3), ok;
        if (k) bo_s(&o, ", ", 2);
        if (nw < 1) { o.bad = 1; break; }
        ok = kind == 0 ? pos_words(v, wa, wb, nw, &o) : kind == 1 ? size_words(v, wa, wb, nw, &o) :
             repeat_words(v, wa, wb, nw, &o);
        if (!ok) { o.bad = 1; break; }
    }
    return bo_end(&o);
}

int wf_cssbg_position(const char *v, char *out, int cap) { return list_of(v, out, cap, 0); }
int wf_cssbg_size(const char *v, char *out, int cap) { return list_of(v, out, cap, 1); }
int wf_cssbg_repeat(const char *v, char *out, int cap) { return list_of(v, out, cap, 2); }

int wf_cssbg_list_image(const char *v, char *out, int cap) {
    int a = 0, b;
    bo_t o;
    bo_init(&o, out, cap);
    if (!v) return 0;
    b = bslen(v);
    btrim(v, &a, &b);
    if (bis(v + a, b - a, "none")) { bo_s(&o, "none", 4); return bo_end(&o); }
    return wf_cssbg_url(v + a, b - a, out, cap);
}

/* ---- the shorthand ------------------------------------------------------ */

static void bclear(char *s, int n) { int i; for (i = 0; i < n; i++) s[i] = 0; }

/* word classes inside one layer */
enum { W_IMG = 1, W_REP, W_POS, W_SLASH, W_ATT, W_BOX, W_COL };

static int wclass(const char *s, int a, int b) {
    int l = b - a;
    if (l == 1 && s[a] == '/') return W_SLASH;
    if (bis(s + a, l, "none") || bpre(s + a, l, "url(") || bpre(s + a, l, "linear-gradient(") ||
        bpre(s + a, l, "radial-gradient(") || bpre(s + a, l, "conic-gradient(")) return W_IMG;
    if (rword(s + a, l)) return W_REP;
    if (pword(s + a, l)) return W_POS;
    if (bis(s + a, l, "scroll") || bis(s + a, l, "fixed") || bis(s + a, l, "local")) return W_ATT;
    if (bis(s + a, l, "border-box") || bis(s + a, l, "padding-box") || bis(s + a, l, "content-box")) return W_BOX;
    return W_COL;
}

/* ---- Lot 91: background-clip and background-origin ------------------- */

enum { BOX_BORDER = 1, BOX_PADDING, BOX_CONTENT };

static int box_code(const char *s, int n) {
    if (bis(s, n, "border-box")) return BOX_BORDER;
    if (bis(s, n, "padding-box")) return BOX_PADDING;
    if (bis(s, n, "content-box")) return BOX_CONTENT;
    return 0;
}

static void box_text(int code, char *out) {
    const char *t = code == BOX_BORDER ? "border-box" : code == BOX_PADDING ? "padding-box" : "content-box";
    int i;
    for (i = 0; t[i]; i++) out[i] = t[i];
    out[i] = 0;
}

int wf_cssbg_box(const char *v, char *out, int cap) {
    int ia[WF_CSSBG_LAYERS], ib[WF_CSSBG_LAYERS], c, k, code = 0;
    bo_t o;
    bo_init(&o, out, cap);
    if (!v) return 0;
    c = bsplit(v, 0, bslen(v), ',', 0, ia, ib, WF_CSSBG_LAYERS);
    if (c < 1) return 0;
    for (k = 0; k < c; k++) {
        int b = box_code(v + ia[k], ib[k] - ia[k]);
        if (!b || (k && b != code)) return 0;
        code = b;
    }
    {
        char t[16];
        box_text(code, t);
        bo_s(&o, t, bslen(t));
    }
    return bo_end(&o);
}

int wf_cssbg_shorthand(const char *v, wf_cssbg_t *out) {
    int la[WF_CSSBG_LAYERS], lb[WF_CSSBG_LAYERS], nl, L, org = BOX_PADDING, clp = BOX_BORDER;
    bo_t img, pos, siz, rep;
    if (!v || !out) return 0;
    bclear((char *)out, (int)sizeof(*out));
    nl = bsplit(v, 0, bslen(v), ',', 0, la, lb, WF_CSSBG_LAYERS);
    if (nl < 1) return 0;
    bo_init(&img, out->image, WF_CSSBG_VAL);
    bo_init(&pos, out->position, WF_CSSBG_VAL);
    bo_init(&siz, out->size, WF_CSSBG_VAL);
    bo_init(&rep, out->repeat, WF_CSSBG_VAL);
    for (L = 0; L < nl; L++) {
        int wa[12], wb[12], nw = bsplit(v, la[L], lb[L], ' ', 1, wa, wb, 12), k;
        int got_img = 0, att = 0, box = 0, col = 0, bx[2] = { 0, 0 }, lo, lc;
        int rep_a = -1, rep_n = 0, pos_a = -1, pos_n = 0, siz_a = -1, siz_n = 0, last = 0;
        int last_layer = L == nl - 1;
        if (nw < 1) return 0;
        if (L) { bo_s(&img, ", ", 2); bo_s(&pos, ", ", 2); bo_s(&siz, ", ", 2); bo_s(&rep, ", ", 2); }
        for (k = 0; k < nw; k++) {
            int c = wclass(v, wa[k], wb[k]);
            if (c == W_SLASH) {
                int j;
                if (last != W_POS || siz_a >= 0) return 0;
                j = k + 1;
                while (j < nw && j - k - 1 < 2) {
                    int l = wb[j] - wa[j], z;
                    if (bis(v + wa[j], l, "auto") || bis(v + wa[j], l, "cover") || bis(v + wa[j], l, "contain") ||
                        blen(v + wa[j], l, 1, 0, &z)) j++;
                    else break;
                }
                siz_a = k + 1;
                siz_n = j - k - 1;
                if (!siz_n) return 0;
                k = j - 1;
                last = W_SLASH;
                continue;
            }
            if (c == W_IMG) {
                char t[WF_CSSBG_VAL];
                bo_t one;
                int kind;
                if (got_img) return 0;
                bo_init(&one, t, (int)sizeof(t));
                kind = image_item(v, wa[k], wb[k], &one);
                if (!kind || (kind == 3 && !last_layer) || !bo_end(&one)) return 0;
                bo_s(&img, t, one.n);
                got_img = 1;
                out->has_image = 1;
            } else if (c == W_REP) {
                if (rep_a >= 0 && (last != W_REP || rep_n >= 2)) return 0;
                if (rep_a < 0) rep_a = k;
                rep_n++;
            } else if (c == W_POS) {
                if (pos_a >= 0 && (last != W_POS || pos_n >= 2)) return 0;
                if (pos_a < 0) pos_a = k;
                pos_n++;
            } else if (c == W_ATT) {
                if (++att > 1) return 0;
            } else if (c == W_BOX) {
                if (box >= 2) return 0;
                bx[box++] = box_code(v + wa[k], wb[k] - wa[k]);
            } else {
                int l = wb[k] - wa[k];
                if (!last_layer || col || !wf_cssbg_color(v + wa[k], l) || l >= (int)sizeof(out->color)) return 0;
                col = 1;
                {
                    int i;
                    for (i = 0; i < l; i++) out->color[i] = bl(v[wa[k] + i]);
                    out->color[l] = 0;
                }
            }
            last = c;
        }
        if (!got_img) bo_s(&img, "none", 4);
        if (rep_a >= 0) {
            if (!repeat_words(v, wa + rep_a, wb + rep_a, rep_n, &rep)) return 0;
            out->has_repeat = 1;
        } else bo_s(&rep, "repeat", 6);
        if (pos_a >= 0) {
            if (!pos_words(v, wa + pos_a, wb + pos_a, pos_n, &pos)) return 0;
            out->has_position = 1;
        } else bo_s(&pos, "0% 0%", 5);
        if (siz_a >= 0) {
            if (!size_words(v, wa + siz_a, wb + siz_a, siz_n, &siz)) return 0;
            out->has_size = 1;
        } else bo_s(&siz, "auto", 4);
        /* Lot 91: one box keyword sets the origin and the clip, two set
         * the origin then the clip; the engine paints every layer in one
         * origin box and one clip box, so the layers must agree */
        lo = box ? bx[0] : BOX_PADDING;
        lc = box ? bx[box - 1] : BOX_BORDER;
        if (box) { out->has_origin = 1; out->has_clip = 1; }
        if (L == 0) { org = lo; clp = lc; }
        else if (lo != org || lc != clp) { bclear((char *)out, (int)sizeof(*out)); return 0; }
    }
    box_text(org, out->origin);
    box_text(clp, out->clip);
    if (img.bad || pos.bad || siz.bad || rep.bad) { bclear((char *)out, (int)sizeof(*out)); return 0; }
    out->layers = nl;
    return 1;
}
