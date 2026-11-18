/* wf_jsre.c - Lot 113: regular expressions for the JavaScript subset.
 *
 * A pattern compiles to a small bytecode with relative jumps (so a
 * quantified atom can be copied for {n,m}) and runs on a backtracking
 * machine with an explicit, bounded backtrack stack. Every 16
 * instructions count one interpreter step, so a catastrophic pattern
 * ends on the step budget instead of hanging the shell.
 *
 * Supported: alternation, greedy and lazy * + ? {n} {n,} {n,m},
 * capturing, non-capturing and named groups, back references (\1, \k<n>),
 * lookahead and lookbehind, classes with ranges and \d \w \s \D \W \S,
 * . ^ $ \b \B, \xHH \uHHHH \u{H..} \cX escapes, and the flags g i m s y
 * (u and d are accepted). Text is UTF-8: . and classes take one code
 * point, case folding is ASCII only. */
#include "wf_jsi.h"

#define KEY_GROUPS(vm) js_intern((vm), "groups", 6)

enum {
    RI_CHAR = 1, RI_ANY, RI_CLASS, RI_SPLIT, RI_SPLITL, RI_JMP, RI_SAVE, RI_MATCH, RI_BOL, RI_EOL, RI_WORDB,
    RI_NWORDB, RI_BACKREF, RI_LOOK, RI_MARK, RI_CHKPROG, RI_CLRCAP, RI_ATPOS
};
/* RI_LOOK kinds */
#define LK_AHEAD  0
#define LK_NAHEAD 1
#define LK_BEHIND 2
#define LK_NBEHIND 3

#define RF_G 1u
#define RF_I 2u
#define RF_M 4u
#define RF_S 8u
#define RF_Y 16u
#define RF_U 32u
#define RF_D 64u

#define RE_MAXINS 6000
#define RE_MAXREG 64
#define RE_SLOTS (2 * JS_RE_MAXCAP + RE_MAXREG)
#define RE_BTMAX 12000
#define RE_MAXNEST 40

typedef struct { int32_t op, x, y; } re_ins_t;

typedef struct {
    uint32_t ninst, ncap, nreg, flags;
    uint32_t names;        /* array: name string per group, or 0 */
    uint32_t code;         /* re_ins_t[ninst] */
} re_prog_t;

/* ---- the compiler ---- */

typedef struct {
    js_vm_t *vm;
    const char *s;
    int n, i;
    uint32_t flags;
    re_ins_t code[RE_MAXINS];
    int ninst;
    int ncap, nreg, nest;
    uint32_t names[JS_RE_MAXCAP];
    int has_names;
    int failed;
    const char *err;
} rc_t;

static rc_t g_rc;

static void rc_fail(rc_t *c, const char *msg) {
    if (!c->failed) { c->failed = 1; c->err = msg; }
}

static int emit(rc_t *c, int op, int x, int y) {
    if (c->ninst >= RE_MAXINS) { rc_fail(c, "pattern too large for the guest"); return -1; }
    c->code[c->ninst].op = op;
    c->code[c->ninst].x = x;
    c->code[c->ninst].y = y;
    return c->ninst++;
}

/* insert one instruction at position at, shifting [at, ninst) */
static int insert(rc_t *c, int at, int op, int x, int y) {
    int k;
    if (c->ninst >= RE_MAXINS) { rc_fail(c, "pattern too large for the guest"); return -1; }
    for (k = c->ninst; k > at; k--) c->code[k] = c->code[k - 1];
    c->code[at].op = op;
    c->code[at].x = x;
    c->code[at].y = y;
    c->ninst++;
    return at;
}

/* copy [from, to) to the end (relative jumps stay valid) */
static void copy_block(rc_t *c, int from, int to) {
    int k, len = to - from;
    if (c->ninst + len > RE_MAXINS) { rc_fail(c, "pattern too large for the guest"); return; }
    for (k = 0; k < len; k++) c->code[c->ninst + k] = c->code[from + k];
    c->ninst += len;
}

static int peek(rc_t *c) { return c->i < c->n ? (uint8_t)c->s[c->i] : -1; }

static int hexd(int ch) {
    if (ch >= '0' && ch <= '9') return ch - '0';
    if (ch >= 'a' && ch <= 'f') return ch - 'a' + 10;
    if (ch >= 'A' && ch <= 'F') return ch - 'A' + 10;
    return -1;
}

/* class data in the arena: n, neg, then n (lo, hi) code point pairs, sorted, merged */
typedef struct { int n; uint32_t lo[160], hi[160]; } rset_t;

static void rs_add(rc_t *c, rset_t *r, uint32_t lo, uint32_t hi) {
    if (r->n >= 160) { rc_fail(c, "character class too large for the guest"); return; }
    r->lo[r->n] = lo;
    r->hi[r->n] = hi;
    r->n++;
}

static void rs_norm(rset_t *r) {
    int i, j, k = 0;
    for (i = 1; i < r->n; i++) {      /* insertion sort by lo */
        uint32_t lo = r->lo[i], hi = r->hi[i];
        for (j = i - 1; j >= 0 && r->lo[j] > lo; j--) { r->lo[j + 1] = r->lo[j]; r->hi[j + 1] = r->hi[j]; }
        r->lo[j + 1] = lo;
        r->hi[j + 1] = hi;
    }
    for (i = 0; i < r->n; i++) {
        if (k && r->lo[i] <= r->hi[k - 1] + 1) {
            if (r->hi[i] > r->hi[k - 1]) r->hi[k - 1] = r->hi[i];
        } else {
            r->lo[k] = r->lo[i];
            r->hi[k] = r->hi[i];
            k++;
        }
    }
    r->n = k;
}

static void rs_add_set(rc_t *c, rset_t *r, const rset_t *s, int negate) {
    int i;
    uint32_t at = 0;
    if (!negate) {
        for (i = 0; i < s->n; i++) rs_add(c, r, s->lo[i], s->hi[i]);
        return;
    }
    for (i = 0; i < s->n; i++) {
        if (s->lo[i] > at) rs_add(c, r, at, s->lo[i] - 1);
        at = s->hi[i] + 1;
    }
    if (at <= 0x10FFFF) rs_add(c, r, at, 0x10FFFF);
}

static void rs_builtin(rc_t *c, rset_t *r, int which) {
    static const uint32_t sp[] = { 9, 13, 32, 32, 0xA0, 0xA0, 0x1680, 0x1680, 0x2000, 0x200A, 0x2028, 0x2029,
                                   0x202F, 0x202F, 0x205F, 0x205F, 0x3000, 0x3000, 0xFEFF, 0xFEFF };
    rset_t t;
    int i, lower = which | 32;
    t.n = 0;
    if (lower == 'd') rs_add(c, &t, '0', '9');
    else if (lower == 'w') { rs_add(c, &t, '0', '9'); rs_add(c, &t, 'A', 'Z'); rs_add(c, &t, '_', '_'); rs_add(c, &t, 'a', 'z'); }
    else for (i = 0; i < (int)(sizeof(sp) / sizeof(sp[0])); i += 2) rs_add(c, &t, sp[i], sp[i + 1]);
    rs_norm(&t);
    rs_add_set(c, r, &t, which != lower);
}

static uint32_t rs_store(rc_t *c, rset_t *r, int neg) {
    js_vm_t *vm = c->vm;
    uint32_t ref;
    int i;
    rs_norm(r);
    ref = js_alloc(vm, (uint32_t)(8 + 8 * r->n));
    if (!ref) { rc_fail(c, "out of memory"); return 0; }
    ((uint32_t *)JPTR(vm, ref))[0] = (uint32_t)r->n;
    ((uint32_t *)JPTR(vm, ref))[1] = (uint32_t)neg;
    for (i = 0; i < r->n; i++) {
        ((uint32_t *)JPTR(vm, ref))[2 + 2 * i] = r->lo[i];
        ((uint32_t *)JPTR(vm, ref))[3 + 2 * i] = r->hi[i];
    }
    return ref;
}

/* decode one UTF-8 code point of the pattern */
static uint32_t pat_cp(rc_t *c) {
    uint8_t b = (uint8_t)c->s[c->i];
    int l = b < 0x80 ? 1 : b >= 0xF0 ? 4 : b >= 0xE0 ? 3 : b >= 0xC0 ? 2 : 1, k;
    uint32_t cp;
    if (c->i + l > c->n) l = 1;
    cp = l == 1 ? b : (uint32_t)(b & (0x7F >> l));
    for (k = 1; k < l; k++) cp = (cp << 6) | ((uint8_t)c->s[c->i + k] & 63u);
    c->i += l;
    return cp;
}

/* an escape after the backslash (c->i is past it): a code point, or -1 when it was a
 * class escape added to r (r may be 0 outside classes: then *cls gets the letter) */
static int32_t parse_escape(rc_t *c, int in_class, int *cls) {
    int ch = peek(c), d;
    uint32_t v = 0;
    *cls = 0;
    if (ch < 0) { rc_fail(c, "\\ at end of pattern"); return 0; }
    c->i++;
    switch (ch) {
    case 'd': case 'D': case 'w': case 'W': case 's': case 'S': *cls = ch; return -1;
    case 'n': return '\n';
    case 'r': return '\r';
    case 't': return '\t';
    case 'v': return '\v';
    case 'f': return '\f';
    case 'b': if (in_class) return '\b'; break;
    case '0':
        if (peek(c) >= '0' && peek(c) <= '9' && !(c->flags & RF_U)) {     /* legacy octal */
            v = 0;
            while (peek(c) >= '0' && peek(c) <= '7' && v < 32) v = v * 8 + (uint32_t)(c->s[c->i++] - '0');
            return (int32_t)v;
        }
        return 0;
    case 'c':
        d = peek(c);
        if ((d >= 'a' && d <= 'z') || (d >= 'A' && d <= 'Z')) { c->i++; return d & 31; }
        c->i--;
        return '\\';
    case 'x':
        if (c->i + 2 <= c->n && hexd(c->s[c->i]) >= 0 && hexd(c->s[c->i + 1]) >= 0) {
            v = (uint32_t)(hexd(c->s[c->i]) * 16 + hexd(c->s[c->i + 1]));
            c->i += 2;
            return (int32_t)v;
        }
        return 'x';
    case 'u':
        if (peek(c) == '{') {
            int st = c->i;
            c->i++;
            while ((d = hexd(peek(c))) >= 0 && v <= 0x10FFFF) { v = v * 16 + (uint32_t)d; c->i++; }
            if (peek(c) == '}' && v <= 0x10FFFF && c->i > st + 1) { c->i++; return (int32_t)v; }
            c->i = st;
            return 'u';
        }
        if (c->i + 4 <= c->n && hexd(c->s[c->i]) >= 0 && hexd(c->s[c->i + 1]) >= 0 && hexd(c->s[c->i + 2]) >= 0 &&
            hexd(c->s[c->i + 3]) >= 0) {
            int k;
            for (k = 0; k < 4; k++) v = v * 16 + (uint32_t)hexd(c->s[c->i + k]);
            c->i += 4;
            if (v >= 0xD800 && v < 0xDC00 && c->i + 6 <= c->n && c->s[c->i] == '\\' && c->s[c->i + 1] == 'u') {
                uint32_t lo = 0;
                int ok = 1;
                for (k = 0; k < 4; k++) { d = hexd(c->s[c->i + 2 + k]); if (d < 0) ok = 0; else lo = lo * 16 + (uint32_t)d; }
                if (ok && lo >= 0xDC00 && lo < 0xE000) { v = 0x10000 + ((v - 0xD800) << 10) + (lo - 0xDC00); c->i += 6; }
            }
            return (int32_t)v;
        }
        return 'u';
    default: break;
    }
    if ((uint8_t)ch >= 0x80) { c->i--; return (int32_t)pat_cp(c); }
    return ch;      /* identity escape: \. \* \/ ... */
}

static void emit_cp(rc_t *c, uint32_t cp) {
    /* a code point as its UTF-8 bytes; ASCII letters fold under i */
    if (cp < 0x80) { emit(c, RI_CHAR, (int)cp, 0); return; }
    if (cp < 0x800) { emit(c, RI_CHAR, (int)(0xC0 | (cp >> 6)), 0); emit(c, RI_CHAR, (int)(0x80 | (cp & 63)), 0); return; }
    if (cp < 0x10000) {
        emit(c, RI_CHAR, (int)(0xE0 | (cp >> 12)), 0);
        emit(c, RI_CHAR, (int)(0x80 | ((cp >> 6) & 63)), 0);
        emit(c, RI_CHAR, (int)(0x80 | (cp & 63)), 0);
        return;
    }
    emit(c, RI_CHAR, (int)(0xF0 | (cp >> 18)), 0);
    emit(c, RI_CHAR, (int)(0x80 | ((cp >> 12) & 63)), 0);
    emit(c, RI_CHAR, (int)(0x80 | ((cp >> 6) & 63)), 0);
    emit(c, RI_CHAR, (int)(0x80 | (cp & 63)), 0);
}

static void parse_class(rc_t *c) {
    static rset_t r;
    int neg = 0;
    r.n = 0;
    c->i++;     /* [ */
    if (peek(c) == '^') { neg = 1; c->i++; }
    while (!c->failed) {
        int32_t lo, hi;
        int cls = 0;
        int ch = peek(c);
        if (ch < 0) { rc_fail(c, "unterminated character class"); return; }
        if (ch == ']') { c->i++; break; }
        if (ch == '\\') { c->i++; lo = parse_escape(c, 1, &cls); }
        else lo = (int32_t)pat_cp(c);
        if (cls) { rs_builtin(c, &r, cls); continue; }
        if (peek(c) == '-' && c->i + 1 < c->n && c->s[c->i + 1] != ']') {
            int cls2 = 0;
            c->i++;
            if (peek(c) == '\\') { c->i++; hi = parse_escape(c, 1, &cls2); }
            else hi = (int32_t)pat_cp(c);
            if (cls2) {    /* [a-\d]: the dash is literal */
                rs_add(c, &r, (uint32_t)lo, (uint32_t)lo);
                rs_add(c, &r, '-', '-');
                rs_builtin(c, &r, cls2);
                continue;
            }
            if (hi < lo) { rc_fail(c, "range out of order in character class"); return; }
            rs_add(c, &r, (uint32_t)lo, (uint32_t)hi);
        } else {
            rs_add(c, &r, (uint32_t)lo, (uint32_t)lo);
        }
    }
    if (c->failed) return;
    if (c->flags & RF_I) {    /* ASCII folding: add the other case of letter ranges */
        int k, n0 = r.n;
        for (k = 0; k < n0; k++) {
            uint32_t a = r.lo[k] < 'A' ? 'A' : r.lo[k], b = r.hi[k] > 'Z' ? 'Z' : r.hi[k];
            if (a <= b) rs_add(c, &r, a + 32, b + 32);
            a = r.lo[k] < 'a' ? 'a' : r.lo[k];
            b = r.hi[k] > 'z' ? 'z' : r.hi[k];
            if (a <= b) rs_add(c, &r, a - 32, b - 32);
        }
    }
    emit(c, RI_CLASS, (int)rs_store(c, &r, neg), 0);
}

/* ---- groups, quantifiers, alternatives ---- */

#define RI_STAR1 40     /* greedy loop over the one-character instruction that follows */

static void parse_disj(rc_t *c);

static int new_reg(rc_t *c) {
    if (c->nreg >= RE_MAXREG) { rc_fail(c, "too many loops in pattern for the guest"); return 0; }
    return c->nreg++;
}

static int single_char(rc_t *c, int s) {
    int op = c->code[s].op;
    return c->ninst == s + 1 && (op == RI_CHAR || op == RI_ANY || op == RI_CLASS);
}

/* wrap [s, ninst) in a * loop */
static void star_wrap(rc_t *c, int s, int lazy, int gfrom, int gto) {
    int r, end;
    if (c->failed) return;
    if (!lazy && single_char(c, s)) {
        insert(c, s, RI_STAR1, 0, 0);
        return;
    }
    r = new_reg(c);
    if (gto >= gfrom) insert(c, s, RI_CLRCAP, gfrom, gto);
    insert(c, s, RI_MARK, r, 0);
    insert(c, s, lazy ? RI_SPLITL : RI_SPLIT, 1, 0);
    emit(c, RI_CHKPROG, r, 0);
    emit(c, RI_JMP, s - c->ninst, 0);
    end = c->ninst;
    if (!c->failed) c->code[s].y = end - s;
}

static int parse_int(rc_t *c, int *out) {
    int v = 0, any = 0;
    while (peek(c) >= '0' && peek(c) <= '9') {
        if (v < 100000) v = v * 10 + (c->s[c->i] - '0');
        c->i++;
        any = 1;
    }
    *out = v;
    return any;
}

/* {n}, {n,}, {n,m}: 1 with *lo, *hi (-1 for none); 0 (nothing consumed) when it is a literal { */
static int parse_braces(rc_t *c, int *lo, int *hi) {
    int save = c->i;
    c->i++;
    if (!parse_int(c, lo)) { c->i = save; return 0; }
    *hi = *lo;
    if (peek(c) == ',') {
        c->i++;
        if (!parse_int(c, hi)) *hi = -1;
    }
    if (peek(c) != '}') { c->i = save; return 0; }
    c->i++;
    return 1;
}

static re_ins_t g_tmp[2048];

static void quantify(rc_t *c, int s, int gfrom, int gto) {
    int ch = peek(c), lo, hi, lazy = 0, len, k, splits[256], ns = 0;
    if (ch == '*') { lo = 0; hi = -1; c->i++; }
    else if (ch == '+') { lo = 1; hi = -1; c->i++; }
    else if (ch == '?') { lo = 0; hi = 1; c->i++; }
    else if (ch == '{') { if (!parse_braces(c, &lo, &hi)) return; }
    else return;
    if (peek(c) == '?') { lazy = 1; c->i++; }
    if (hi >= 0 && hi < lo) { rc_fail(c, "numbers out of order in {} quantifier"); return; }
    if (lo > 1000 || hi > 1000) { rc_fail(c, "repeat count too large for the guest"); return; }
    len = c->ninst - s;
    if (lo == 1 && hi == -1) {      /* + */
        copy_block(c, s, s + len);
        star_wrap(c, s + len, lazy, gfrom, gto);
        return;
    }
    if (lo == 0 && hi == -1) { star_wrap(c, s, lazy, gfrom, gto); return; }
    if (lo == 0 && hi == 1) {
        insert(c, s, lazy ? RI_SPLITL : RI_SPLIT, 1, len + 1);
        return;
    }
    /* general {lo,hi}: lo copies, then a loop or hi - lo optional copies */
    if (len > (int)(sizeof(g_tmp) / sizeof(g_tmp[0]))) { rc_fail(c, "repeated group too large for the guest"); return; }
    for (k = 0; k < len; k++) g_tmp[k] = c->code[s + k];
    c->ninst = s;
    for (k = 0; k < lo && !c->failed; k++) {
        int st = c->ninst, j;
        if (st + len > RE_MAXINS) { rc_fail(c, "pattern too large for the guest"); return; }
        for (j = 0; j < len; j++) c->code[st + j] = g_tmp[j];
        c->ninst += len;
    }
    if (hi < 0) {
        int st = c->ninst, j;
        if (st + len > RE_MAXINS) { rc_fail(c, "pattern too large for the guest"); return; }
        for (j = 0; j < len; j++) c->code[st + j] = g_tmp[j];
        c->ninst += len;
        star_wrap(c, st, lazy, gfrom, gto);
        return;
    }
    for (k = lo; k < hi && !c->failed; k++) {
        int j, st;
        if (ns >= 256) { rc_fail(c, "repeat count too large for the guest"); return; }
        splits[ns++] = emit(c, lazy ? RI_SPLITL : RI_SPLIT, 1, 0);
        st = c->ninst;
        if (st + len > RE_MAXINS) { rc_fail(c, "pattern too large for the guest"); return; }
        for (j = 0; j < len; j++) c->code[st + j] = g_tmp[j];
        c->ninst += len;
    }
    for (k = 0; k < ns && !c->failed; k++) c->code[splits[k]].y = c->ninst - splits[k];
}

static int name_lookup(rc_t *c, int from, int to) {
    js_vm_t *vm = c->vm;
    int g;
    for (g = 1; g <= JS_RE_MAXCAP - 1; g++) {
        uint32_t nm = c->names[g];
        int k, l = to - from;
        if (!nm || (int)JSTR(vm, nm)->len != l) continue;
        for (k = 0; k < l && JSTR(vm, nm)->s[k] == c->s[from + k]; k++) {}
        if (k == l) return g;
    }
    return 0;
}

static int total_groups;

static void parse_term(rc_t *c) {
    int ch = peek(c), s = c->ninst, g0 = c->ncap + 1;
    switch (ch) {
    case '^': c->i++; emit(c, RI_BOL, 0, 0); return;
    case '$': c->i++; emit(c, RI_EOL, 0, 0); return;
    case '.': c->i++; emit(c, RI_ANY, 0, 0); break;
    case '[': parse_class(c); break;
    case '*': case '+': case '?': rc_fail(c, "nothing to repeat"); return;
    case '{': {
        int lo, hi, save = c->i;
        if (parse_braces(c, &lo, &hi)) { c->i = save; rc_fail(c, "nothing to repeat"); return; }
        c->i++;
        emit(c, RI_CHAR, '{', 0);
        break;
    }
    case '(': {
        int kind = -1, idx = 0;
        c->i++;
        if (++c->nest > RE_MAXNEST) { rc_fail(c, "groups nested too deeply for the guest"); return; }
        if (peek(c) == '?') {
            c->i++;
            ch = peek(c);
            if (ch == ':') { c->i++; kind = -2; }
            else if (ch == '=') { c->i++; kind = LK_AHEAD; }
            else if (ch == '!') { c->i++; kind = LK_NAHEAD; }
            else if (ch == '<' && c->i + 1 < c->n && (c->s[c->i + 1] == '=' || c->s[c->i + 1] == '!')) {
                kind = c->s[c->i + 1] == '=' ? LK_BEHIND : LK_NBEHIND;
                c->i += 2;
            } else if (ch == '<') {
                c->i++;
                while (peek(c) >= 0 && peek(c) != '>') c->i++;
                if (peek(c) != '>') { rc_fail(c, "invalid group name"); return; }
                c->i++;
                kind = -3;
            } else {
                rc_fail(c, "invalid group");
                return;
            }
        }
        if (kind >= 0) {      /* lookaround: LOOK skip kind; body; [ATPOS]; MATCH */
            int at = emit(c, RI_LOOK, 0, kind), body_caps = c->ncap;
            (void)body_caps;
            parse_disj(c);
            if (kind >= LK_BEHIND) emit(c, RI_ATPOS, 0, 0);
            emit(c, RI_MATCH, 0, 0);
            if (!c->failed) c->code[at].x = c->ninst - at;
        } else if (kind == -2) {
            parse_disj(c);
        } else {
            if (c->ncap + 1 >= JS_RE_MAXCAP) { rc_fail(c, "too many capture groups for the guest"); return; }
            idx = ++c->ncap;
            emit(c, RI_SAVE, 2 * idx, 0);
            parse_disj(c);
            emit(c, RI_SAVE, 2 * idx + 1, 0);
        }
        c->nest--;
        if (peek(c) != ')') { rc_fail(c, "unterminated group"); return; }
        c->i++;
        if (kind >= LK_BEHIND) return;     /* lookbehind is not quantifiable */
        break;
    }
    case '\\': {
        int cls = 0;
        int32_t cp;
        c->i++;
        ch = peek(c);
        if (ch == 'b' || ch == 'B') { c->i++; emit(c, ch == 'b' ? RI_WORDB : RI_NWORDB, 0, 0); return; }
        if (ch >= '1' && ch <= '9') {
            int save = c->i, num;
            parse_int(c, &num);
            if (num <= total_groups) { emit(c, RI_BACKREF, num, 0); break; }
            c->i = save;
            if (ch >= '8') { c->i++; emit(c, RI_CHAR, ch, 0); break; }
            {
                uint32_t v = 0;      /* legacy octal escape */
                while (peek(c) >= '0' && peek(c) <= '7' && v < 32) v = v * 8 + (uint32_t)(c->s[c->i++] - '0');
                emit_cp(c, v);
                break;
            }
        }
        if (ch == 'k' && c->i + 1 < c->n && c->s[c->i + 1] == '<' && c->has_names) {
            int st = c->i + 2, e = st, g;
            while (e < c->n && c->s[e] != '>') e++;
            g = e < c->n ? name_lookup(c, st, e) : 0;
            if (!g) { rc_fail(c, "invalid named reference"); return; }
            c->i = e + 1;
            emit(c, RI_BACKREF, g, 0);
            break;
        }
        cp = parse_escape(c, 0, &cls);
        if (cls) {
            static rset_t r;
            r.n = 0;
            rs_builtin(c, &r, cls);
            emit(c, RI_CLASS, (int)rs_store(c, &r, 0), 0);
        } else {
            int fold = (c->flags & RF_I) && ((cp >= 'a' && cp <= 'z') || (cp >= 'A' && cp <= 'Z'));
            if (fold) emit(c, RI_CHAR, cp | 32, 1);
            else emit_cp(c, (uint32_t)cp);
            if (c->ninst - s > 1) {      /* a multi-byte literal quantifies as one unit */
                quantify(c, s, g0, c->ncap);
                return;
            }
        }
        break;
    }
    default: {
        uint32_t cp = pat_cp(c);
        if ((c->flags & RF_I) && ((cp >= 'a' && cp <= 'z') || (cp >= 'A' && cp <= 'Z'))) emit(c, RI_CHAR, (int)(cp | 32), 1);
        else emit_cp(c, cp);
        break;
    }
    }
    if (!c->failed) quantify(c, s, g0, c->ncap);
}

static void parse_alt(rc_t *c) {
    while (!c->failed && c->i < c->n && peek(c) != '|' && peek(c) != ')') parse_term(c);
}

static void parse_disj(rc_t *c) {
    int s = c->ninst;
    parse_alt(c);
    if (c->failed || peek(c) != '|') return;
    {
        int j, rest;
        c->i++;
        insert(c, s, RI_SPLIT, 1, 0);
        j = emit(c, RI_JMP, 0, 0);
        rest = c->ninst;
        if (c->failed) return;
        c->code[s].y = rest - s;
        parse_disj(c);
        if (!c->failed) c->code[j].x = c->ninst - j;
    }
}

/* count the groups and read their names before the parse (forward \k and \10 decisions) */
static void prescan(rc_t *c) {
    js_vm_t *vm = c->vm;
    int i, in_class = 0, g = 0;
    for (i = 0; i < c->n; i++) {
        char ch = c->s[i];
        if (ch == '\\') { i++; continue; }
        if (in_class) { if (ch == ']') in_class = 0; continue; }
        if (ch == '[') { in_class = 1; continue; }
        if (ch != '(') continue;
        if (i + 1 < c->n && c->s[i + 1] == '?') {
            if (i + 2 < c->n && c->s[i + 2] == '<' && i + 3 < c->n && c->s[i + 3] != '=' && c->s[i + 3] != '!') {
                int st = i + 3, e = st;
                while (e < c->n && c->s[e] != '>') e++;
                g++;
                if (g < JS_RE_MAXCAP) c->names[g] = js_intern(vm, c->s + st, e - st);
                c->has_names = 1;
            }
            continue;
        }
        g++;
    }
    total_groups = g;
}

static uint32_t compile(js_vm_t *vm, const char *src, int len, uint32_t flags, const char **err) {
    rc_t *c = &g_rc;
    uint32_t pref, code;
    re_prog_t *p;
    int k;
    c->vm = vm;
    c->s = src;
    c->n = len;
    c->i = 0;
    c->flags = flags;
    c->ninst = 0;
    c->ncap = 0;
    c->nreg = 0;
    c->nest = 0;
    c->failed = 0;
    c->err = 0;
    c->has_names = 0;
    for (k = 0; k < JS_RE_MAXCAP; k++) c->names[k] = 0;
    prescan(c);
    emit(c, RI_SAVE, 0, 0);
    parse_disj(c);
    if (!c->failed && c->i < c->n) rc_fail(c, peek(c) == ')' ? "unmatched ')'" : "unexpected character");
    emit(c, RI_SAVE, 1, 0);
    emit(c, RI_MATCH, 0, 0);
    if (c->failed) { *err = c->err; return 0; }
    pref = js_alloc(vm, (uint32_t)sizeof(re_prog_t));
    code = js_alloc(vm, (uint32_t)(c->ninst * (int)sizeof(re_ins_t)));
    if (!pref || !code) { *err = "out of memory"; return 0; }
    for (k = 0; k < c->ninst; k++) ((re_ins_t *)JPTR(vm, code))[k] = c->code[k];
    p = (re_prog_t *)JPTR(vm, pref);
    p->ninst = (uint32_t)c->ninst;
    p->ncap = (uint32_t)(c->ncap + 1);
    p->nreg = (uint32_t)c->nreg;
    p->flags = flags;
    p->code = code;
    p->names = 0;
    if (c->has_names) {
        uint32_t nm = js_array_new(vm, p->ncap);
        if (!nm) { *err = "out of memory"; return 0; }
        for (k = 0; k < (int)p->ncap; k++) js_array_push(vm, nm, c->names[k] ? jv_str(c->names[k]) : jv_undef());
        ((re_prog_t *)JPTR(vm, pref))->names = nm;
    }
    return pref;
}

/* ---- the backtracking machine ---- */

typedef struct { int32_t kind, a, b, c; } re_bt_t;     /* kind 0 branch (pc, pos), 1 restore (slot, old), 2 star */

#define RE_BT_CAP 16384
static re_bt_t g_bt[RE_BT_CAP];
static int g_bt_top;

typedef struct {
    js_vm_t *vm;
    const re_ins_t *code;     /* valid while the run allocates nothing */
    const uint8_t *s;
    int n;
    uint32_t flags;
    int slots[RE_SLOTS];
    uint32_t ticks;
    int status;               /* 0 ok, -1 aborted or thrown */
} rm_t;

static int is_nl(uint32_t c) { return c == '\n' || c == '\r' || c == 0x2028 || c == 0x2029; }
static int is_word(int c) { return (c >= '0' && c <= '9') || (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') || c == '_'; }

static int cp_at(const uint8_t *s, int n, int pos, uint32_t *cp) {
    uint8_t b = s[pos];
    int l = b < 0x80 ? 1 : b >= 0xF0 ? 4 : b >= 0xE0 ? 3 : b >= 0xC0 ? 2 : 1, k;
    if (pos + l > n) l = 1;
    if (l == 1) { *cp = b; return 1; }
    *cp = b & (0x7Fu >> l);
    for (k = 1; k < l; k++) {
        if ((s[pos + k] & 0xC0) != 0x80) { *cp = b; return 1; }
        *cp = (*cp << 6) | (s[pos + k] & 63u);
    }
    return l;
}

static int class_has(js_vm_t *vm, uint32_t ref, uint32_t cp) {
    const uint32_t *d = (const uint32_t *)JPTR(vm, ref);
    uint32_t n = d[0], lo = 0, hi = n;
    int in = 0;
    while (lo < hi) {      /* binary search over sorted ranges */
        uint32_t mid = (lo + hi) / 2;
        if (cp < d[2 + 2 * mid]) hi = mid;
        else if (cp > d[3 + 2 * mid]) lo = mid + 1;
        else { in = 1; break; }
    }
    return d[1] ? !in : in;
}

/* one-character instructions: the new position, or -1 */
static int one(rm_t *m, const re_ins_t *in, int pos) {
    uint32_t cp;
    int l;
    if (pos >= m->n) return -1;
    switch (in->op) {
    case RI_CHAR: {
        int b = m->s[pos];
        if (b == in->x || (in->y && (b | 32) == in->x)) return pos + 1;
        return -1;
    }
    case RI_ANY:
        l = cp_at(m->s, m->n, pos, &cp);
        if (!(m->flags & RF_S) && is_nl(cp)) return -1;
        return pos + l;
    case RI_CLASS:
        l = cp_at(m->s, m->n, pos, &cp);
        return class_has(m->vm, (uint32_t)in->x, cp) ? pos + l : -1;
    default:
        return -1;
    }
}

static int bt_push(rm_t *m, int kind, int a, int b, int c) {
    if (g_bt_top >= RE_BT_CAP) {
        js_throw_err(m->vm, JE_RANGE, "regular expression backtracking too deep for the guest");
        m->status = -1;
        return 0;
    }
    g_bt[g_bt_top].kind = kind;
    g_bt[g_bt_top].a = a;
    g_bt[g_bt_top].b = b;
    g_bt[g_bt_top].c = c;
    g_bt_top++;
    return 1;
}

static int set_slot(rm_t *m, int slot, int v) {
    if (m->slots[slot] == v) return 1;
    if (!bt_push(m, 1, slot, m->slots[slot], 0)) return 0;
    m->slots[slot] = v;
    return 1;
}

/* run from pc at pos; target >= 0 forces the end (lookbehind). Returns the end or -1. */
static int rm_run(rm_t *m, int pc, int pos, int target, int depth) {
    int base = g_bt_top;
    for (;;) {
        const re_ins_t *in = &m->code[pc];
        int np;
        if ((++m->ticks & 15u) == 0) {
            js_step(m->vm);
            if (m->vm->ctl) { m->status = -1; g_bt_top = base; return -1; }
        }
        switch (in->op) {
        case RI_CHAR: case RI_ANY: case RI_CLASS:
            np = one(m, in, pos);
            if (np < 0) goto fail;
            pos = np;
            pc++;
            continue;
        case RI_STAR1: {
            int st = pos;
            const re_ins_t *a = in + 1;
            while ((np = one(m, a, pos)) >= 0) {
                pos = np;
                if ((++m->ticks & 15u) == 0) {
                    js_step(m->vm);
                    if (m->vm->ctl) { m->status = -1; g_bt_top = base; return -1; }
                }
            }
            if (pos > st && !bt_push(m, 2, pc + 2, st, pos)) { g_bt_top = base; return -1; }
            pc += 2;
            continue;
        }
        case RI_SPLIT:
            if (!bt_push(m, 0, pc + in->y, pos, 0)) { g_bt_top = base; return -1; }
            pc += in->x;
            continue;
        case RI_SPLITL:
            if (!bt_push(m, 0, pc + in->x, pos, 0)) { g_bt_top = base; return -1; }
            pc += in->y;
            continue;
        case RI_JMP:
            pc += in->x;
            continue;
        case RI_SAVE:
            if (!set_slot(m, in->x, pos)) { g_bt_top = base; return -1; }
            pc++;
            continue;
        case RI_MARK:
            if (!set_slot(m, 2 * JS_RE_MAXCAP + in->x, pos)) { g_bt_top = base; return -1; }
            pc++;
            continue;
        case RI_CHKPROG:
            if (m->slots[2 * JS_RE_MAXCAP + in->x] == pos) goto fail;
            pc++;
            continue;
        case RI_CLRCAP: {
            int g;
            for (g = in->x; g <= in->y; g++)
                if (!set_slot(m, 2 * g, -1) || !set_slot(m, 2 * g + 1, -1)) { g_bt_top = base; return -1; }
            pc++;
            continue;
        }
        case RI_BOL:
            if (pos == 0 || ((m->flags & RF_M) && is_nl(m->s[pos - 1]))) { pc++; continue; }
            goto fail;
        case RI_EOL:
            if (pos == m->n || ((m->flags & RF_M) && is_nl(m->s[pos]))) { pc++; continue; }
            goto fail;
        case RI_WORDB: case RI_NWORDB: {
            int a = pos > 0 && is_word(m->s[pos - 1]), b = pos < m->n && is_word(m->s[pos]);
            if ((a != b) == (in->op == RI_WORDB)) { pc++; continue; }
            goto fail;
        }
        case RI_BACKREF: {
            int g = in->x, st = m->slots[2 * g], en = m->slots[2 * g + 1], l, k;
            if (st < 0 || en < 0) { pc++; continue; }
            l = en - st;
            if (pos + l > m->n) goto fail;
            for (k = 0; k < l; k++) {
                int x = m->s[st + k], y = m->s[pos + k];
                if (x != y && !((m->flags & RF_I) && is_word(x) && (x | 32) == (y | 32) && x >= 'A')) break;
            }
            if (k < l) goto fail;
            pos += l;
            pc++;
            continue;
        }
        case RI_LOOK: {
            int saved[RE_SLOTS], k, r = -1, kind = in->y;
            if (depth > RE_MAXNEST) goto fail;
            for (k = 0; k < RE_SLOTS; k++) saved[k] = m->slots[k];
            if (kind == LK_AHEAD || kind == LK_NAHEAD) {
                r = rm_run(m, pc + 1, pos, -1, depth + 1);
            } else {
                for (k = pos; k >= 0 && r < 0 && m->status == 0; k--) r = rm_run(m, pc + 1, k, pos, depth + 1);
            }
            if (m->status) { g_bt_top = base; return -1; }
            if ((r >= 0) == (kind == LK_AHEAD || kind == LK_BEHIND)) {
                if (r >= 0) {      /* keep the captures, restorable on backtrack */
                    for (k = 0; k < RE_SLOTS; k++) {
                        if (m->slots[k] != saved[k]) {
                            int now = m->slots[k];
                            m->slots[k] = saved[k];
                            if (!set_slot(m, k, now)) { g_bt_top = base; return -1; }
                        }
                    }
                } else {
                    for (k = 0; k < RE_SLOTS; k++) m->slots[k] = saved[k];
                }
                pc += in->x;
                continue;
            }
            for (k = 0; k < RE_SLOTS; k++) m->slots[k] = saved[k];
            goto fail;
        }
        case RI_ATPOS:
            if (target >= 0 && pos != target) goto fail;
            pc++;
            continue;
        case RI_MATCH:
            g_bt_top = base;
            return pos;
        default:
            goto fail;
        }
    fail:
        for (;;) {
            re_bt_t *e;
            if (g_bt_top == base) return -1;
            e = &g_bt[--g_bt_top];
            if (e->kind == 1) { m->slots[e->a] = e->b; continue; }
            if (e->kind == 0) { pc = e->a; pos = e->b; break; }
            {   /* star: give back one code point */
                int p2 = e->c - 1;
                while (p2 > e->b && (m->s[p2] & 0xC0) == 0x80) p2--;
                pc = e->a;
                pos = p2;
                if (p2 > e->b) { e->c = p2; g_bt_top++; }
                break;
            }
        }
    }
}

static re_prog_t *prog_of(js_vm_t *vm, uint32_t re) { return (re_prog_t *)JPTR(vm, JOBJ(vm, re)->x2); }

int js_regexp_search(js_vm_t *vm, uint32_t re, uint32_t str, int start, int *caps) {
    static rm_t rm;
    rm_t *m = &rm;
    re_prog_t *p = prog_of(vm, re);
    int nc = (int)p->ncap, pos, k, saved_top = g_bt_top;
    m->vm = vm;
    m->code = (const re_ins_t *)JPTR(vm, p->code);
    m->s = (const uint8_t *)JSTR(vm, str)->s;
    m->n = (int)JSTR(vm, str)->len;
    m->flags = p->flags;
    m->status = 0;
    for (pos = start < 0 ? 0 : start; pos <= m->n; pos++) {
        int r;
        for (k = 0; k < RE_SLOTS; k++) m->slots[k] = -1;
        r = rm_run(m, 0, pos, -1, 0);
        g_bt_top = saved_top;
        if (m->status) return -1;
        if (r >= 0) {
            for (k = 0; k < 2 * nc; k++) caps[k] = m->slots[k];
            return 1;
        }
        if (p->flags & RF_Y) break;
        if (pos < m->n && (m->s[pos] & 0xC0) == 0xC0) {      /* start positions step over whole code points */
            while (pos + 1 < m->n && (m->s[pos + 1] & 0xC0) == 0x80) pos++;
        }
    }
    return 0;
}

/* ---- RegExp objects: x0 source, x1 flag bits, x2 program ---- */

int js_regexp_ncap(js_vm_t *vm, uint32_t re) { return (int)prog_of(vm, re)->ncap; }
int js_regexp_global(js_vm_t *vm, uint32_t re) { return (JOBJ(vm, re)->x1 & RF_G) != 0; }

int js_is_regexp(js_vm_t *vm, jv v) { return v.t == JT_OBJ && JOBJ(vm, v.a)->cls == JS_CLASS_REGEXP; }

static int parse_flags(js_vm_t *vm, uint32_t fs, uint32_t *out) {
    static const char letters[] = "gimsyud";
    int i, k;
    *out = 0;
    if (!fs) return 1;
    for (i = 0; i < (int)JSTR(vm, fs)->len; i++) {
        char ch = JSTR(vm, fs)->s[i];
        for (k = 0; letters[k] && letters[k] != ch; k++) {}
        if (!letters[k] || (*out & (1u << k))) return 0;
        *out |= 1u << k;
    }
    return 1;
}

static uint32_t flags_str(js_vm_t *vm, uint32_t f) {
    char b[8];
    int n = 0;
    if (f & RF_D) b[n++] = 'd';
    if (f & RF_G) b[n++] = 'g';
    if (f & RF_I) b[n++] = 'i';
    if (f & RF_M) b[n++] = 'm';
    if (f & RF_S) b[n++] = 's';
    if (f & RF_U) b[n++] = 'u';
    if (f & RF_Y) b[n++] = 'y';
    return js_str_new(vm, b, n);
}

/* the source as it would appear between slashes */
static uint32_t escape_source(js_vm_t *vm, uint32_t src) {
    js_sb_t b;
    int i, n = (int)JSTR(vm, src)->len, in_class = 0, changed = 0;
    if (n == 0) return js_str_c(vm, "(?:)");
    for (i = 0; i < n; i++) {
        char ch = JSTR(vm, src)->s[i];
        if (ch == '\\') { i++; continue; }
        if (ch == '[') in_class = 1;
        else if (ch == ']') in_class = 0;
        else if ((ch == '/' && !in_class) || ch == '\n' || ch == '\r') changed = 1;
    }
    if (!changed) return src;
    js_sb_init(vm, &b);
    in_class = 0;
    for (i = 0; i < n; i++) {
        char ch = JSTR(vm, src)->s[i];
        if (ch == '\\' && i + 1 < n) {
            js_sb_ch(vm, &b, ch);
            js_sb_ch(vm, &b, JSTR(vm, src)->s[++i]);
            continue;
        }
        if (ch == '[') in_class = 1;
        else if (ch == ']') in_class = 0;
        if (ch == '/' && !in_class) js_sb_put(vm, &b, "\\/", 2);
        else if (ch == '\n') js_sb_put(vm, &b, "\\n", 2);
        else if (ch == '\r') js_sb_put(vm, &b, "\\r", 2);
        else js_sb_ch(vm, &b, ch);
    }
    return js_sb_finish(vm, &b);
}

static uint32_t make_obj(js_vm_t *vm, uint32_t src, uint32_t bits, uint32_t prog) {
    uint32_t o;
    if (vm->super_this && JOBJ(vm, vm->super_this)->cls == JS_CLASS_OBJECT) {
        o = vm->super_this;
        JOBJ(vm, o)->cls = JS_CLASS_REGEXP;
        vm->super_this = 0;
    } else {
        o = js_obj_new(vm, JS_CLASS_REGEXP, vm->p_regexp);
    }
    if (!o) return 0;
    JOBJ(vm, o)->x0 = src;
    JOBJ(vm, o)->x1 = bits;
    JOBJ(vm, o)->x2 = prog;
    js_define(vm, o, vm->s_lastIndex, jv_int(0), P_WRITE);
    return o;
}

static uint32_t regexp_build(js_vm_t *vm, uint32_t source, uint32_t flags, uint32_t *cache) {
    uint32_t bits, prog = cache ? *cache : 0;
    const char *err = 0;
    if (!parse_flags(vm, flags, &bits)) {
        js_throw_err2(vm, JE_SYNTAX, "Invalid regular expression flags: ", flags);
        return 0;
    }
    if (!prog) {
        prog = compile(vm, JSTR(vm, source)->s, (int)JSTR(vm, source)->len, bits, &err);
        if (!prog) {
            if (vm->ctl == CTL_ABORT) return 0;
            {
                uint32_t m = js_str_c(vm, "Invalid regular expression: /");
                m = js_str_cat(vm, m, source);
                m = js_str_cat(vm, m, js_str_c(vm, "/: "));
                m = js_str_cat(vm, m, js_str_c(vm, err ? err : "syntax error"));
                js_throw(vm, jv_obj(js_error_new(vm, JE_SYNTAX, m)));
            }
            return 0;
        }
        if (cache) *cache = prog;
    }
    return make_obj(vm, source, bits, prog);
}

uint32_t js_regexp_new(js_vm_t *vm, uint32_t source, uint32_t flags) { return regexp_build(vm, source, flags, 0); }

uint32_t js_regexp_lit(js_vm_t *vm, uint32_t source, uint32_t flags, uint32_t *cache) {
    return regexp_build(vm, source, flags, cache);
}

jv js_regexp_result(js_vm_t *vm, uint32_t re, uint32_t str, const int *caps) {
    re_prog_t *p = prog_of(vm, re);
    uint32_t a, names = p->names, groups = 0;
    int nc = (int)p->ncap, k;
    a = js_array_new(vm, (uint32_t)nc);
    if (!a) return jv_undef();
    for (k = 0; k < nc; k++)
        js_array_push(vm, a, caps[2 * k] < 0 || caps[2 * k + 1] < 0 ? jv_undef() : jv_str(js_str_sub(vm, str, caps[2 * k], caps[2 * k + 1])));
    js_define(vm, a, vm->s_index, jv_int(caps[0]), P_DEFAULT);
    js_define(vm, a, vm->s_input, jv_str(str), P_DEFAULT);
    if (names) {
        groups = js_obj_new(vm, JS_CLASS_OBJECT, 0);
        if (!groups) return jv_undef();
        for (k = 1; k < nc; k++) {
            jv nm = js_array_get(vm, names, (uint32_t)k);
            if (nm.t == JT_STR) js_define(vm, groups, nm.a, js_array_get(vm, a, (uint32_t)k), P_DEFAULT);
        }
    }
    js_define(vm, a, KEY_GROUPS(vm), groups ? jv_obj(groups) : jv_undef(), P_DEFAULT);
    return jv_obj(a);
}

jv js_regexp_exec(js_vm_t *vm, uint32_t re, uint32_t str) {
    int caps[2 * JS_RE_MAXCAP], r, start = 0;
    uint32_t bits = JOBJ(vm, re)->x1;
    int len = (int)JSTR(vm, str)->len;
    if (bits & (RF_G | RF_Y)) {
        jv li = js_get(vm, jv_obj(re), vm->s_lastIndex);
        jf64 n;
        int64_t v = 0;
        if (vm->ctl) return jv_undef();
        n = js_to_integer(vm, li);
        if (vm->ctl) return jv_undef();
        if (jf_isinf(n)) v = jf_signbit(n) ? 0 : (int64_t)len + 1;
        else (void)jf_to_i64(n, &v);
        if (v < 0) v = 0;
        if (v > len) {
            js_put(vm, jv_obj(re), vm->s_lastIndex, jv_int(0));
            return jv_null();
        }
        start = (int)v;
    }
    r = js_regexp_search(vm, re, str, start, caps);
    if (r < 0) return jv_undef();
    if (!r) {
        if (bits & (RF_G | RF_Y)) js_put(vm, jv_obj(re), vm->s_lastIndex, jv_int(0));
        return jv_null();
    }
    if (bits & (RF_G | RF_Y)) js_put(vm, jv_obj(re), vm->s_lastIndex, jv_int(caps[1]));
    if (vm->ctl) return jv_undef();
    return js_regexp_result(vm, re, str, caps);
}

static int this_re(js_vm_t *vm, jv thisv) {
    if (js_is_regexp(vm, thisv)) return 1;
    js_throw_err(vm, JE_TYPE, "RegExp method called on an incompatible receiver");
    return 0;
}

/* exec 0, test 1 */
static jv rep_exec(js_vm_t *vm, jv thisv, jv *argv, int argc, uint32_t m) {
    uint32_t s;
    jv r;
    if (!this_re(vm, thisv)) return jv_undef();
    s = js_to_string(vm, JARG(0));
    if (vm->ctl) return jv_undef();
    r = js_regexp_exec(vm, thisv.a, s);
    if (vm->ctl) return jv_undef();
    return m ? jv_bool(r.t == JT_OBJ) : r;
}

static jv rep_to_string(js_vm_t *vm, jv thisv, jv *argv, int argc, uint32_t m) {
    uint32_t s;
    (void)argv; (void)argc; (void)m;
    if (!this_re(vm, thisv)) return jv_undef();
    s = js_str_cat(vm, js_str_c(vm, "/"), escape_source(vm, JOBJ(vm, thisv.a)->x0));
    s = js_str_cat(vm, s, js_str_c(vm, "/"));
    return jv_str(js_str_cat(vm, s, flags_str(vm, JOBJ(vm, thisv.a)->x1)));
}

/* accessors: source 0, flags 1, then one flag bit each */
static jv rep_get(js_vm_t *vm, jv thisv, jv *argv, int argc, uint32_t m) {
    (void)argv; (void)argc;
    if (thisv.t == JT_OBJ && thisv.a == vm->p_regexp) return m == 0 ? jv_str(js_str_c(vm, "(?:)")) : m == 1 ? jv_str(vm->s_empty) : jv_undef();
    if (!this_re(vm, thisv)) return jv_undef();
    if (m == 0) return jv_str(escape_source(vm, JOBJ(vm, thisv.a)->x0));
    if (m == 1) return jv_str(flags_str(vm, JOBJ(vm, thisv.a)->x1));
    return jv_bool((JOBJ(vm, thisv.a)->x1 & (m >> 8)) != 0);
}

static jv re_ctor(js_vm_t *vm, jv thisv, jv *argv, int argc, uint32_t m) {
    jv pat = JARG(0), fl = JARG(1);
    uint32_t src, flags = 0, o;
    (void)thisv; (void)m;
    if (js_is_regexp(vm, pat)) {
        if (fl.t == JT_UNDEF && !vm->in_new && !vm->super_this) return pat;
        src = JOBJ(vm, pat.a)->x0;
        if (fl.t == JT_UNDEF) flags = flags_str(vm, JOBJ(vm, pat.a)->x1);
    } else {
        src = pat.t == JT_UNDEF ? vm->s_empty : js_to_string(vm, pat);
        if (vm->ctl) return jv_undef();
    }
    if (fl.t != JT_UNDEF) {
        flags = js_to_string(vm, fl);
        if (vm->ctl) return jv_undef();
    }
    o = js_regexp_new(vm, src, flags);
    return o ? jv_obj(o) : jv_undef();
}

static int name_len(const char *s) { int n = 0; while (s[n]) n++; return n; }

int js_regexp_init(js_vm_t *vm) {
    static const char *const names[] = { "source", "flags", "global", "ignoreCase", "multiline", "dotAll", "unicode",
                                         "sticky", "hasIndices", 0 };
    static const uint32_t bits[] = { 0, 0, RF_G, RF_I, RF_M, RF_S, RF_U, RF_Y, RF_D };
    uint32_t c = js_ctor_new(vm, "RegExp", re_ctor, 2, vm->p_regexp), p = vm->p_regexp;
    int i;
    if (!c) return -1;
    js_def_native(vm, p, "exec", rep_exec, 1, 0);
    js_def_native(vm, p, "test", rep_exec, 1, 1);
    js_def_native(vm, p, "toString", rep_to_string, 0, 0);
    for (i = 0; names[i]; i++) {
        uint32_t g = js_native_new(vm, names[i], rep_get, 0, i < 2 ? (uint32_t)i : (bits[i] << 8) | 2u);
        if (!g) return -1;
        js_define_accessor(vm, p, js_intern(vm, names[i], name_len(names[i])), g, 0, P_CONF);
    }
    return vm->ctl == CTL_ABORT ? -1 : 0;
}
