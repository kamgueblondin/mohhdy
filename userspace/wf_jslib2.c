/* wf_jslib2.c - Lot 113: the String, Number, Boolean, Math, JSON, Date,
 * Map and Set built-ins of the JavaScript subset, and the arena string
 * builder. Strings are byte strings: indexes and lengths count bytes,
 * case mapping is ASCII only. Date has a fixed clock and the UTC zone. */
#include "wf_jsi.h"

static int cslen(const char *s) { int n = 0; while (s[n]) n++; return n; }
#define KEY(vm, s) js_intern((vm), (s), cslen(s))

/* ---- the string builder: the block is a js_str_t being filled ---- */

void js_sb_init(js_vm_t *vm, js_sb_t *b) {
    (void)vm;
    b->ref = 0;
    b->len = 0;
    b->cap = 0;
}

static int sb_grow(js_vm_t *vm, js_sb_t *b, uint32_t need) {
    uint32_t cap = b->cap ? b->cap : 64, r, i;
    if (vm->ctl == CTL_ABORT) return 0;
    if (need > (1u << 24)) { js_abort(vm, "RangeError: string too long for the guest"); return 0; }
    while (cap < need) cap *= 2;
    r = js_alloc(vm, 12 + cap + 1);
    if (!r) return 0;
    for (i = 0; i < b->len; i++) JSTR(vm, r)->s[i] = JSTR(vm, b->ref)->s[i];
    b->ref = r;
    b->cap = cap;
    return 1;
}

void js_sb_put(js_vm_t *vm, js_sb_t *b, const char *s, int n) {
    int i;
    if (n <= 0) return;
    if (b->len + (uint32_t)n > b->cap && !sb_grow(vm, b, b->len + (uint32_t)n)) return;
    for (i = 0; i < n; i++) JSTR(vm, b->ref)->s[b->len + (uint32_t)i] = s[i];
    b->len += (uint32_t)n;
}

void js_sb_str(js_vm_t *vm, js_sb_t *b, uint32_t str) {
    uint32_t n = JSTR(vm, str)->len, i;
    if (!n) return;
    if (b->len + n > b->cap && !sb_grow(vm, b, b->len + n)) return;
    for (i = 0; i < n; i++) JSTR(vm, b->ref)->s[b->len + i] = JSTR(vm, str)->s[i];
    b->len += n;
}

void js_sb_ch(js_vm_t *vm, js_sb_t *b, char c) { js_sb_put(vm, b, &c, 1); }

uint32_t js_sb_finish(js_vm_t *vm, js_sb_t *b) {
    js_str_t *p;
    if (!b->ref || vm->ctl == CTL_ABORT) return vm->s_empty;
    p = JSTR(vm, b->ref);
    p->len = b->len;
    p->s[b->len] = 0;
    return b->ref;
}

static void sb_sub(js_vm_t *vm, js_sb_t *b, uint32_t str, int from, int to) {
    if (to > from) js_sb_put(vm, b, JSTR(vm, str)->s + from, to - from);
    /* the source string never moves: the arena only grows */
}

/* ---- this coercions ---- */

uint32_t js_this_string(js_vm_t *vm, jv thisv) {
    if (thisv.t == JT_STR) return thisv.a;
    if (jv_is_nullish(thisv)) {
        js_throw_err(vm, JE_TYPE, "String.prototype method called on null or undefined");
        return 0;
    }
    {
        uint32_t s = js_to_string(vm, thisv);
        return vm->ctl ? 0 : s;
    }
}

jv js_this_number(js_vm_t *vm, jv thisv, int *ok) {
    *ok = 1;
    if (jv_is_num(thisv)) return thisv;
    if (thisv.t == JT_OBJ && JOBJ(vm, thisv.a)->cls == JS_CLASS_NUMBER) {
        jv v;
        v.t = (uint8_t)JOBJ(vm, thisv.a)->x0;
        v.f = 0;
        v.pad = 0;
        v.a = JOBJ(vm, thisv.a)->x1;
        v.b = JOBJ(vm, thisv.a)->x2;
        return v;
    }
    *ok = 0;
    js_throw_err(vm, JE_TYPE, "Number.prototype method called on a non-number");
    return jv_undef();
}

#define THIS_STR(s) uint32_t s = js_this_string(vm, thisv); if (!s) return jv_undef()
#define SLEN(r_) ((int)JSTR(vm, r_)->len)
#define SCH(r_, i_) (JSTR(vm, r_)->s[(i_)])

static int str_idx(js_vm_t *vm, jv v, int len, int dflt) {
    jf64 n;
    int64_t i = 0;
    if (v.t == JT_UNDEF) return dflt;
    n = js_to_integer(vm, v);
    if (jf_isinf(n)) return jf_signbit(n) ? 0 : len;
    (void)jf_to_i64(n, &i);
    if (i < 0) i = 0;
    if (i > len) i = len;
    return (int)i;
}

static int rel_idx(js_vm_t *vm, jv v, int len, int dflt) {
    jf64 n;
    int64_t i = 0;
    if (v.t == JT_UNDEF) return dflt;
    n = js_to_integer(vm, v);
    if (jf_isinf(n)) return jf_signbit(n) ? 0 : len;
    (void)jf_to_i64(n, &i);
    if (i < 0) { i += len; if (i < 0) i = 0; }
    if (i > len) i = len;
    return (int)i;
}

static int find_sub(js_vm_t *vm, uint32_t s, uint32_t pat, int from) {
    int n = SLEN(s), m = SLEN(pat), i, j;
    if (from < 0) from = 0;
    for (i = from; i + m <= n; i++) {
        for (j = 0; j < m && SCH(s, i + j) == SCH(pat, j); j++) {}
        if (j == m) return i;
    }
    return -1;
}

/* ---- String ---- */

static jv str_ctor(js_vm_t *vm, jv thisv, jv *argv, int argc, uint32_t m) {
    uint32_t s = argc ? js_to_string(vm, argv[0]) : vm->s_empty;
    (void)thisv; (void)m;
    if (vm->ctl) return jv_undef();
    if (vm->in_new) return jv_obj(js_to_object(vm, jv_str(s)));
    return jv_str(s);
}

/* fromCharCode (UTF-8 encodes each code), fromCodePoint */
static jv str_from_code(js_vm_t *vm, jv thisv, jv *argv, int argc, uint32_t m) {
    js_sb_t b;
    int i;
    (void)thisv;
    js_sb_init(vm, &b);
    for (i = 0; i < argc && !vm->ctl; i++) {
        uint32_t c = js_to_uint32(vm, argv[i]);
        char u[4];
        if (m == 0) c &= 0xFFFFu;
        else if (c > 0x10FFFF) return js_throw_err(vm, JE_RANGE, "Invalid code point");
        if (c < 0x80) { u[0] = (char)c; js_sb_put(vm, &b, u, 1); }
        else if (c < 0x800) { u[0] = (char)(0xC0 | (c >> 6)); u[1] = (char)(0x80 | (c & 63)); js_sb_put(vm, &b, u, 2); }
        else if (c < 0x10000) {
            u[0] = (char)(0xE0 | (c >> 12)); u[1] = (char)(0x80 | ((c >> 6) & 63)); u[2] = (char)(0x80 | (c & 63));
            js_sb_put(vm, &b, u, 3);
        } else {
            u[0] = (char)(0xF0 | (c >> 18)); u[1] = (char)(0x80 | ((c >> 12) & 63));
            u[2] = (char)(0x80 | ((c >> 6) & 63)); u[3] = (char)(0x80 | (c & 63));
            js_sb_put(vm, &b, u, 4);
        }
    }
    return vm->ctl ? jv_undef() : jv_str(js_sb_finish(vm, &b));
}

static jv str_raw(js_vm_t *vm, jv thisv, jv *argv, int argc, uint32_t m) {
    jv strs = JARG(0), raw;
    uint32_t len, i;
    js_sb_t b;
    (void)thisv; (void)m;
    if (strs.t != JT_OBJ) return js_throw_err(vm, JE_TYPE, "String.raw needs a template object");
    raw = js_get(vm, strs, vm->s_raw);
    if (vm->ctl) return jv_undef();
    if (raw.t != JT_OBJ) return js_throw_err(vm, JE_TYPE, "String.raw needs a raw array");
    js_length_of(vm, raw, &len);
    js_sb_init(vm, &b);
    for (i = 0; i < len && !vm->ctl; i++) {
        uint32_t s = js_to_string(vm, js_get_v(vm, raw, jv_int((int32_t)i)));
        if (vm->ctl) break;
        js_sb_str(vm, &b, s);
        if (i + 1 < len && (int)(i + 1) < argc) {
            s = js_to_string(vm, argv[i + 1]);
            if (vm->ctl) break;
            js_sb_str(vm, &b, s);
        }
    }
    return vm->ctl ? jv_undef() : jv_str(js_sb_finish(vm, &b));
}

/* charAt 0, charCodeAt 1, codePointAt 2, at 3 */
static jv strp_char(js_vm_t *vm, jv thisv, jv *argv, int argc, uint32_t m) {
    int n, i;
    jf64 x;
    int64_t k = 0;
    THIS_STR(s);
    n = SLEN(s);
    x = js_to_integer(vm, JARG(0));
    if (vm->ctl) return jv_undef();
    if (jf_isinf(x)) k = -1 - n;
    else (void)jf_to_i64(x, &k);
    if (m == 3 && k < 0) k += n;
    if (k < 0 || k >= n) {
        if (m == 0) return jv_str(vm->s_empty);
        if (m == 1) return jv_num(JF_NAN);
        return jv_undef();
    }
    i = (int)k;
    if (m == 0 || m == 3) return jv_str(js_str_sub(vm, s, i, i + 1));
    if (m == 1) return jv_int((uint8_t)SCH(s, i));
    {   /* codePointAt decodes the UTF-8 sequence that starts at i */
        uint8_t c = (uint8_t)SCH(s, i);
        uint32_t cp;
        int l, j;
        if (c < 0x80) return jv_int(c);
        l = c >= 0xF0 ? 4 : c >= 0xE0 ? 3 : c >= 0xC0 ? 2 : 1;
        if (l == 1 || i + l > n) return jv_int(c);
        cp = c & (0x7Fu >> l);
        for (j = 1; j < l; j++) cp = (cp << 6) | ((uint8_t)SCH(s, i + j) & 63u);
        return jv_int((int32_t)cp);
    }
}

/* indexOf 0, lastIndexOf 1, includes 2, startsWith 3, endsWith 4 */
static jv strp_find(js_vm_t *vm, jv thisv, jv *argv, int argc, uint32_t m) {
    uint32_t pat;
    int n, pn, from, i, j;
    THIS_STR(s);
    if (m >= 2 && js_is_regexp(vm, JARG(0))) return js_throw_err(vm, JE_TYPE, "First argument must not be a regular expression");
    pat = js_to_string(vm, JARG(0));
    if (vm->ctl) return jv_undef();
    n = SLEN(s);
    pn = SLEN(pat);
    switch (m) {
    case 0: case 2:
        from = str_idx(vm, JARG(1), n, 0);
        i = find_sub(vm, s, pat, from);
        return m == 0 ? jv_int(i) : jv_bool(i >= 0);
    case 1: {
        jf64 x = js_to_number(vm, JARG(1));
        from = n;
        if (!jf_isnan(x)) from = str_idx(vm, JARG(1), n, n);
        for (i = from + pn > n ? n - pn : from; i >= 0; i--) {
            for (j = 0; j < pn && SCH(s, i + j) == SCH(pat, j); j++) {}
            if (j == pn) return jv_int(i);
        }
        return jv_int(-1);
    }
    case 3:
        from = str_idx(vm, JARG(1), n, 0);
        if (from + pn > n) return jv_bool(0);
        for (j = 0; j < pn; j++) if (SCH(s, from + j) != SCH(pat, j)) return jv_bool(0);
        return jv_bool(1);
    default:
        from = str_idx(vm, JARG(1), n, n);
        if (from - pn < 0) return jv_bool(0);
        for (j = 0; j < pn; j++) if (SCH(s, from - pn + j) != SCH(pat, j)) return jv_bool(0);
        return jv_bool(1);
    }
}

/* slice 0, substring 1, substr 2 */
static jv strp_slice(js_vm_t *vm, jv thisv, jv *argv, int argc, uint32_t m) {
    int n, a, b;
    THIS_STR(s);
    n = SLEN(s);
    if (m == 0) {
        a = rel_idx(vm, JARG(0), n, 0);
        b = rel_idx(vm, JARG(1), n, n);
    } else if (m == 1) {
        a = str_idx(vm, JARG(0), n, 0);
        b = str_idx(vm, JARG(1), n, n);
        if (a > b) { int t = a; a = b; b = t; }
    } else {
        a = rel_idx(vm, JARG(0), n, 0);
        b = JARG(1).t == JT_UNDEF ? n : a + str_idx(vm, JARG(1), n, n);
        if (b > n) b = n;
    }
    if (vm->ctl) return jv_undef();
    if (b <= a) return jv_str(vm->s_empty);
    return jv_str(js_str_sub(vm, s, a, b));
}

/* toUpperCase 0, toLowerCase 1 (ASCII) */
static jv strp_case(js_vm_t *vm, jv thisv, jv *argv, int argc, uint32_t m) {
    uint32_t r;
    int i, n;
    (void)argv; (void)argc;
    {
        THIS_STR(s);
        n = SLEN(s);
        r = js_str_new(vm, JSTR(vm, s)->s, n);
    }
    if (vm->ctl) return jv_undef();
    for (i = 0; i < n; i++) {
        char c = JSTR(vm, r)->s[i];
        if (m == 0 && c >= 'a' && c <= 'z') JSTR(vm, r)->s[i] = (char)(c - 32);
        if (m == 1 && c >= 'A' && c <= 'Z') JSTR(vm, r)->s[i] = (char)(c + 32);
    }
    return jv_str(r);
}

static int ws_at(js_vm_t *vm, uint32_t s, int i, int dir) {
    /* ASCII white space, NBSP (C2 A0) and the BOM (EF BB BF); returns its byte length */
    int n = SLEN(s);
    uint8_t c;
    if (dir > 0) {
        c = (uint8_t)SCH(s, i);
        if (c == ' ' || (c >= 9 && c <= 13)) return 1;
        if (c == 0xC2 && i + 1 < n && (uint8_t)SCH(s, i + 1) == 0xA0) return 2;
        if (c == 0xEF && i + 2 < n && (uint8_t)SCH(s, i + 1) == 0xBB && (uint8_t)SCH(s, i + 2) == 0xBF) return 3;
        return 0;
    }
    c = (uint8_t)SCH(s, i - 1);
    if (c == ' ' || (c >= 9 && c <= 13)) return 1;
    if (c == 0xA0 && i >= 2 && (uint8_t)SCH(s, i - 2) == 0xC2) return 2;
    return 0;
}

/* trim 0, trimStart 1, trimEnd 2 */
static jv strp_trim(js_vm_t *vm, jv thisv, jv *argv, int argc, uint32_t m) {
    int a = 0, b, w;
    (void)argv; (void)argc;
    {
        THIS_STR(s);
        b = SLEN(s);
        if (m != 2) while (a < b && (w = ws_at(vm, s, a, 1)) > 0) a += w;
        if (m != 1) while (b > a && (w = ws_at(vm, s, b, -1)) > 0) b -= w;
        if (a == 0 && b == SLEN(s)) return jv_str(s);
        return jv_str(js_str_sub(vm, s, a, b));
    }
}

static jv strp_repeat(js_vm_t *vm, jv thisv, jv *argv, int argc, uint32_t m) {
    jf64 c;
    int64_t k = 0;
    js_sb_t b;
    (void)m;
    {
        THIS_STR(s);
        c = js_to_integer(vm, JARG(0));
        if (vm->ctl) return jv_undef();
        if (jf_signbit(c) && !jf_iszero(c)) return js_throw_err(vm, JE_RANGE, "Invalid count value");
        if (jf_isinf(c)) return js_throw_err(vm, JE_RANGE, "Invalid count value: Infinity");
        (void)jf_to_i64(c, &k);
        if (k * (int64_t)SLEN(s) > (1 << 22)) return js_throw_err(vm, JE_RANGE, "Invalid string length");
        js_sb_init(vm, &b);
        while (k-- > 0 && !vm->ctl) js_sb_str(vm, &b, s);
        return jv_str(js_sb_finish(vm, &b));
    }
}

/* padStart 0, padEnd 1 */
static jv strp_pad(js_vm_t *vm, jv thisv, jv *argv, int argc, uint32_t m) {
    jf64 t;
    int64_t target = 0;
    uint32_t fill;
    js_sb_t b;
    int need, fl, i;
    THIS_STR(s);
    t = js_to_integer(vm, JARG(0));
    if (vm->ctl) return jv_undef();
    (void)jf_to_i64(t, &target);
    if (target > (1 << 22)) return js_throw_err(vm, JE_RANGE, "Invalid string length");
    fill = JARG(1).t == JT_UNDEF ? js_intern(vm, " ", 1) : js_to_string(vm, JARG(1));
    if (vm->ctl) return jv_undef();
    need = (int)target - SLEN(s);
    fl = SLEN(fill);
    if (need <= 0 || fl == 0) return jv_str(s);
    js_sb_init(vm, &b);
    if (m == 1) js_sb_str(vm, &b, s);
    for (i = 0; i < need; i++) js_sb_ch(vm, &b, SCH(fill, i % fl));
    if (m == 0) js_sb_str(vm, &b, s);
    return jv_str(js_sb_finish(vm, &b));
}

static jv strp_concat(js_vm_t *vm, jv thisv, jv *argv, int argc, uint32_t m) {
    js_sb_t b;
    int i;
    (void)m;
    {
        THIS_STR(s);
        js_sb_init(vm, &b);
        js_sb_str(vm, &b, s);
        for (i = 0; i < argc && !vm->ctl; i++) {
            uint32_t a = js_to_string(vm, argv[i]);
            if (vm->ctl) return jv_undef();
            js_sb_str(vm, &b, a);
        }
        return jv_str(js_sb_finish(vm, &b));
    }
}

static jv strp_locale_cmp(js_vm_t *vm, jv thisv, jv *argv, int argc, uint32_t m) {
    uint32_t o;
    int c;
    (void)m;
    {
        THIS_STR(s);
        o = js_to_string(vm, JARG(0));
        if (vm->ctl) return jv_undef();
        c = js_str_cmp(vm, s, o);
        return jv_int(c < 0 ? -1 : c > 0 ? 1 : 0);
    }
}

static jv strp_value(js_vm_t *vm, jv thisv, jv *argv, int argc, uint32_t m) {
    (void)argv; (void)argc; (void)m;
    if (thisv.t == JT_STR) return thisv;
    if (thisv.t == JT_OBJ && JOBJ(vm, thisv.a)->cls == JS_CLASS_STRING) return jv_str(JOBJ(vm, thisv.a)->x1);
    return js_throw_err(vm, JE_TYPE, "String.prototype.valueOf requires a string");
}

static jv strp_identity(js_vm_t *vm, jv thisv, jv *argv, int argc, uint32_t m) {
    (void)argv; (void)argc; (void)m;
    {
        THIS_STR(s);
        return jv_str(s);
    }
}

/* ---- split, replace, match, search ---- */

static jv strp_split(js_vm_t *vm, jv thisv, jv *argv, int argc, uint32_t m) {
    jv sep = JARG(0);
    uint32_t out, lim = 0xFFFFFFFFu;
    int n, p = 0, q;
    (void)m;
    {
        THIS_STR(s);
        n = SLEN(s);
        if (JARG(1).t != JT_UNDEF) lim = js_to_uint32(vm, argv[1]);
        if (vm->ctl) return jv_undef();
        out = js_array_new(vm, 8);
        if (!out) return jv_undef();
        if (lim == 0) return jv_obj(out);
        if (sep.t == JT_UNDEF) { js_array_push(vm, out, jv_str(s)); return jv_obj(out); }
        if (js_is_regexp(vm, sep)) {
            int caps[2 * JS_RE_MAXCAP], nc = js_regexp_ncap(vm, sep.a), k, r;
            if (n == 0) {
                r = js_regexp_search(vm, sep.a, s, 0, caps);
                if (r < 0) return jv_undef();
                if (!r || caps[1] != 0 || caps[0] != 0) js_array_push(vm, out, jv_str(s));
                return jv_obj(out);
            }
            q = 0;
            while (q < n) {      /* the spec's SplitMatcher loop, with search instead of sticky tries */
                r = js_regexp_search(vm, sep.a, s, q, caps);
                if (r < 0) return jv_undef();
                if (!r || caps[0] >= n) break;
                if (caps[1] == p) { q = caps[0] + 1; continue; }
                js_array_push(vm, out, jv_str(js_str_sub(vm, s, p, caps[0])));
                if (JOBJ(vm, out)->alen >= lim) return jv_obj(out);
                for (k = 1; k < nc; k++) {
                    jv g = caps[2 * k] < 0 ? jv_undef() : jv_str(js_str_sub(vm, s, caps[2 * k], caps[2 * k + 1]));
                    js_array_push(vm, out, g);
                    if (JOBJ(vm, out)->alen >= lim) return jv_obj(out);
                }
                p = caps[1];
                q = p;
            }
            js_array_push(vm, out, jv_str(js_str_sub(vm, s, p, n)));
            return jv_obj(out);
        }
        {
            uint32_t ps = js_to_string(vm, sep);
            int pn;
            if (vm->ctl) return jv_undef();
            pn = SLEN(ps);
            if (pn == 0) {    /* one element per UTF-8 sequence */
                uint32_t arr = 0, i;
                js_iter_values(vm, jv_str(s), &arr);
                if (vm->ctl) return jv_undef();
                for (i = 0; i < JOBJ(vm, arr)->alen && JOBJ(vm, out)->alen < lim; i++)
                    js_array_push(vm, out, js_array_get(vm, arr, i));
                return jv_obj(out);
            }
            while ((q = find_sub(vm, s, ps, p)) >= 0) {
                js_array_push(vm, out, jv_str(js_str_sub(vm, s, p, q)));
                if (JOBJ(vm, out)->alen >= lim) return jv_obj(out);
                p = q + pn;
            }
            js_array_push(vm, out, jv_str(js_str_sub(vm, s, p, n)));
            return jv_obj(out);
        }
    }
}

/* expand $$, $&, $`, $', $n and $nn of a replacement string */
static void expand_repl(js_vm_t *vm, js_sb_t *b, uint32_t rep, uint32_t s, const int *caps, int nc) {
    int rn = SLEN(rep), i;
    for (i = 0; i < rn; i++) {
        char c = SCH(rep, i), d;
        if (c != '$' || i + 1 >= rn) { js_sb_ch(vm, b, c); continue; }
        d = SCH(rep, i + 1);
        if (d == '$') { js_sb_ch(vm, b, '$'); i++; }
        else if (d == '&') { sb_sub(vm, b, s, caps[0], caps[1]); i++; }
        else if (d == '`') { sb_sub(vm, b, s, 0, caps[0]); i++; }
        else if (d == '\'') { sb_sub(vm, b, s, caps[1], SLEN(s)); i++; }
        else if (d >= '0' && d <= '9') {
            int g = d - '0', used = 1;
            if (i + 2 < rn && SCH(rep, i + 2) >= '0' && SCH(rep, i + 2) <= '9' && g * 10 + (SCH(rep, i + 2) - '0') < nc) {
                g = g * 10 + (SCH(rep, i + 2) - '0');
                used = 2;
            }
            if (g >= 1 && g < nc) {
                if (caps[2 * g] >= 0) sb_sub(vm, b, s, caps[2 * g], caps[2 * g + 1]);
                i += used;
            } else {
                js_sb_ch(vm, b, c);
            }
        } else {
            js_sb_ch(vm, b, c);
        }
    }
}

static int apply_repl(js_vm_t *vm, js_sb_t *b, jv rep, uint32_t reps, uint32_t s, const int *caps, int nc) {
    if (js_is_callable(vm, rep)) {
        jv a[JS_RE_MAXCAP + 2], r;
        int k;
        uint32_t rs;
        for (k = 0; k < nc; k++) a[k] = caps[2 * k] < 0 ? jv_undef() : jv_str(js_str_sub(vm, s, caps[2 * k], caps[2 * k + 1]));
        a[nc] = jv_int(caps[0]);
        a[nc + 1] = jv_str(s);
        r = js_call(vm, rep, jv_undef(), a, nc + 2);
        if (vm->ctl) return 0;
        rs = js_to_string(vm, r);
        if (vm->ctl) return 0;
        js_sb_str(vm, b, rs);
        return 1;
    }
    expand_repl(vm, b, reps, s, caps, nc);
    return 1;
}

/* replace 0, replaceAll 1 */
static jv strp_replace(js_vm_t *vm, jv thisv, jv *argv, int argc, uint32_t m) {
    jv pat = JARG(0), rep = JARG(1);
    uint32_t reps = 0;
    js_sb_t b;
    int caps[2 * JS_RE_MAXCAP], n, p = 0;
    THIS_STR(s);
    n = SLEN(s);
    if (!js_is_callable(vm, rep)) {
        reps = js_to_string(vm, rep);
        if (vm->ctl) return jv_undef();
    }
    js_sb_init(vm, &b);
    if (js_is_regexp(vm, pat)) {
        int nc = js_regexp_ncap(vm, pat.a), global = js_regexp_global(vm, pat.a), q = 0, r;
        if (m == 1 && !global) return js_throw_err(vm, JE_TYPE, "replaceAll must be called with a global RegExp");
        while (q <= n) {
            r = js_regexp_search(vm, pat.a, s, q, caps);
            if (r < 0) return jv_undef();
            if (!r) break;
            sb_sub(vm, &b, s, p, caps[0]);
            if (!apply_repl(vm, &b, rep, reps, s, caps, nc)) return jv_undef();
            p = caps[1];
            if (!global) break;
            q = caps[1] == caps[0] ? caps[1] + 1 : caps[1];
            if (caps[1] == caps[0] && caps[0] < n) {   /* keep the byte an empty match skipped */
                sb_sub(vm, &b, s, caps[0], caps[0] + 1);
                p = caps[0] + 1;
            }
        }
        if (global) js_put(vm, pat, vm->s_lastIndex, jv_int(0));
        if (p < n) sb_sub(vm, &b, s, p, n);
        return vm->ctl ? jv_undef() : jv_str(js_sb_finish(vm, &b));
    }
    {
        uint32_t ps = js_to_string(vm, pat);
        int pn, q;
        if (vm->ctl) return jv_undef();
        pn = SLEN(ps);
        while ((q = find_sub(vm, s, ps, p)) >= 0 && q <= n) {
            caps[0] = q;
            caps[1] = q + pn;
            sb_sub(vm, &b, s, p, q);
            if (!apply_repl(vm, &b, rep, reps, s, caps, 1)) return jv_undef();
            p = q + pn;
            if (m == 0) break;
            if (pn == 0) {
                if (q >= n) { p = n + 1; break; }
                sb_sub(vm, &b, s, q, q + 1);
                p = q + 1;
            }
        }
        if (p < n) sb_sub(vm, &b, s, p, n);
        return vm->ctl ? jv_undef() : jv_str(js_sb_finish(vm, &b));
    }
}

static uint32_t to_regexp(js_vm_t *vm, jv v, const char *flags) {
    uint32_t src;
    if (js_is_regexp(vm, v)) return v.a;
    src = v.t == JT_UNDEF ? js_str_c(vm, "(?:)") : js_to_string(vm, v);
    if (vm->ctl) return 0;
    return js_regexp_new(vm, src, js_str_c(vm, flags));
}

/* match 0, matchAll 1, search 2 */
static jv strp_match(js_vm_t *vm, jv thisv, jv *argv, int argc, uint32_t m) {
    uint32_t re, out;
    int caps[2 * JS_RE_MAXCAP], n, q = 0, r, global;
    THIS_STR(s);
    n = SLEN(s);
    if (m == 1 && js_is_regexp(vm, JARG(0)) && !js_regexp_global(vm, JARG(0).a))
        return js_throw_err(vm, JE_TYPE, "matchAll must be called with a global RegExp");
    re = to_regexp(vm, JARG(0), m == 1 ? "g" : "");
    if (!re || vm->ctl) return jv_undef();
    global = js_regexp_global(vm, re);
    if (m == 2) {
        r = js_regexp_search(vm, re, s, 0, caps);
        if (r < 0) return jv_undef();
        return jv_int(r ? caps[0] : -1);
    }
    if (m == 0 && !global) return js_regexp_exec(vm, re, s);
    out = js_array_new(vm, 4);
    if (!out) return jv_undef();
    while (q <= n) {
        r = js_regexp_search(vm, re, s, q, caps);
        if (r < 0) return jv_undef();
        if (!r) break;
        if (m == 0) js_array_push(vm, out, jv_str(js_str_sub(vm, s, caps[0], caps[1])));
        else js_array_push(vm, out, js_regexp_result(vm, re, s, caps));
        q = caps[1] == caps[0] ? caps[1] + 1 : caps[1];
    }
    if (js_is_regexp(vm, JARG(0))) js_put(vm, JARG(0), vm->s_lastIndex, jv_int(0));
    if (m == 0 && JOBJ(vm, out)->alen == 0) return jv_null();
    return jv_obj(out);
}

/* ---- Number and Boolean ---- */

static jv num_ctor(js_vm_t *vm, jv thisv, jv *argv, int argc, uint32_t m) {
    jv v = argc ? jv_num(js_to_number(vm, argv[0])) : jv_int(0);
    (void)thisv; (void)m;
    if (vm->ctl) return jv_undef();
    if (vm->in_new) return jv_obj(js_to_object(vm, v));
    return v;
}

/* isInteger 0, isSafeInteger 1, isFinite 2, isNaN 3 */
static jv num_is(js_vm_t *vm, jv thisv, jv *argv, int argc, uint32_t m) {
    jv v = JARG(0);
    jf64 n;
    (void)thisv; (void)vm;
    if (!jv_is_num(v)) return jv_bool(0);
    if (v.t == JT_INT) return jv_bool(1);
    n = jv_f64(v);
    switch (m) {
    case 0: return jv_bool(jf_is_integer(n));
    case 1: return jv_bool(jf_is_integer(n) && jf_le(jf_abs(n), JF_MAXSAFE));
    case 2: return jv_bool(jf_isfinite(n));
    default: return jv_bool(jf_isnan(n));
    }
}

static int digits_arg(js_vm_t *vm, jv v, int lo, int hi, int dflt, int *out) {
    jf64 n;
    int32_t d;
    if (v.t == JT_UNDEF) { *out = dflt; return 1; }
    n = js_to_integer(vm, v);
    if (vm->ctl) return 0;
    if (!jf_is_i32(n, &d) && !jf_iszero(n)) d = jf_signbit(n) ? -1 : 1000;
    if (jf_iszero(n)) d = 0;
    if (d < lo || d > hi) { js_throw_err(vm, JE_RANGE, "digits argument must be between 0 and 100"); return 0; }
    *out = d;
    return 1;
}

/* toString 0, toFixed 1, toPrecision 2, toExponential 3, valueOf 4, toLocaleString 5 */
static jv nump_fmt(js_vm_t *vm, jv thisv, jv *argv, int argc, uint32_t m) {
    char b[1200];
    int ok, n = 0, d;
    jv v = js_this_number(vm, thisv, &ok);
    jf64 x;
    if (!ok) return jv_undef();
    x = jv_f64(v);
    switch (m) {
    case 0: {
        int radix = 10;
        if (JARG(0).t != JT_UNDEF) {
            radix = js_to_int32(vm, argv[0]);
            if (vm->ctl) return jv_undef();
            if (radix < 2 || radix > 36) return js_throw_err(vm, JE_RANGE, "toString() radix must be between 2 and 36");
        }
        if (radix == 10) return jv_str(js_num_to_str(vm, x));
        n = jf_format_radix(x, radix, b, (int)sizeof(b));
        break;
    }
    case 1:
        if (!digits_arg(vm, JARG(0), 0, 100, 0, &d)) return jv_undef();
        n = jf_format_fixed(x, d, b, (int)sizeof(b));
        break;
    case 2:
        if (JARG(0).t == JT_UNDEF) return jv_str(js_num_to_str(vm, x));
        if (!digits_arg(vm, JARG(0), 1, 100, 1, &d)) return jv_undef();
        n = jf_format_precision(x, d, b, (int)sizeof(b));
        break;
    case 3:
        if (!digits_arg(vm, JARG(0), 0, 100, -1, &d)) return jv_undef();
        n = jf_format_exp(x, d, b, (int)sizeof(b));
        break;
    case 4:
        return v;
    default: {
        /* en-US grouping, at most three fraction digits */
        char t[1200];
        int tn, i, ip = 0, start = 0, k = 0;
        if (!jf_isfinite(x)) return jv_str(js_num_to_str(vm, x));
        tn = jf_format_fixed(x, 3, t, (int)sizeof(t));
        while (tn > 0 && t[tn - 1] == '0') tn--;
        if (tn > 0 && t[tn - 1] == '.') tn--;
        if (t[0] == '-') { b[n++] = '-'; start = 1; }
        while (start + ip < tn && t[start + ip] != '.') ip++;
        for (i = 0; i < ip; i++) {
            b[n++] = t[start + i];
            k = ip - i - 1;
            if (k > 0 && k % 3 == 0) b[n++] = ',';
        }
        for (i = start + ip; i < tn; i++) b[n++] = t[i];
        if (n == 1 && b[0] == '-') b[n++] = '0';
        if (n == 2 && b[0] == '-' && b[1] == '0') { b[0] = '0'; n = 1; }   /* -0.0001 rounds to 0 */
        break;
    }
    }
    return jv_str(js_str_new(vm, b, n));
}

static jv bool_ctor(js_vm_t *vm, jv thisv, jv *argv, int argc, uint32_t m) {
    jv v = jv_bool(js_truthy(vm, JARG(0)));
    (void)thisv; (void)m;
    if (vm->in_new) return jv_obj(js_to_object(vm, v));
    return v;
}

static jv boolp(js_vm_t *vm, jv thisv, jv *argv, int argc, uint32_t m) {
    int b;
    (void)argv; (void)argc;
    if (thisv.t == JT_BOOL) b = (int)thisv.a;
    else if (thisv.t == JT_OBJ && JOBJ(vm, thisv.a)->cls == JS_CLASS_BOOLEAN) b = (int)JOBJ(vm, thisv.a)->x1;
    else return js_throw_err(vm, JE_TYPE, "Boolean.prototype method called on a non-boolean");
    if (m == 1) return jv_bool(b);
    return jv_str(js_str_c(vm, b ? "true" : "false"));
}

/* ---- Math ---- */

enum {
    M_ABS, M_FLOOR, M_CEIL, M_ROUND, M_TRUNC, M_SIGN, M_SQRT, M_CBRT, M_EXP, M_LOG, M_LOG2, M_LOG10, M_SIN, M_COS,
    M_TAN, M_ASIN, M_ACOS, M_ATAN, M_EXPM1, M_LOG1P, M_SINH, M_COSH, M_TANH, M_FROUND, M_CLZ32, M_COUNT1
};

static jf64 half(jf64 x) { return jf_ldexp(x, -1); }

static jv math1(js_vm_t *vm, jv thisv, jv *argv, int argc, uint32_t m) {
    jf64 x = js_to_number(vm, JARG(0)), r;
    (void)thisv;
    if (vm->ctl) return jv_undef();
    switch (m) {
    case M_ABS: r = jf_abs(x); break;
    case M_FLOOR: r = jf_floor(x); break;
    case M_CEIL: r = jf_ceil(x); break;
    case M_ROUND: r = jf_round(x); break;
    case M_TRUNC: r = jf_trunc(x); break;
    case M_SIGN: r = jf_isnan(x) || jf_iszero(x) ? x : jf_signbit(x) ? jf_from_i32(-1) : JF_ONE; break;
    case M_SQRT: r = jf_sqrt(x); break;
    case M_CBRT: r = jf_cbrt(x); break;
    case M_EXP: r = jf_exp(x); break;
    case M_LOG: r = jf_log(x); break;
    case M_LOG2: {
        int32_t k;
        r = jf_div(jf_log(x), JF_LN2);
        /* exact powers of two give exact results */
        if (jf_isfinite(r) && jf_is_i32(jf_round(r), &k) && jf_eq(jf_ldexp(JF_ONE, k), x)) r = jf_from_i32(k);
        break;
    }
    case M_LOG10: {
        jf64 k = jf_round(jf_div(jf_log(x), JF_LN10)), p = JF_ONE, t = jf_from_i32(10);
        int32_t ki;
        r = jf_div(jf_log(x), JF_LN10);
        if (jf_isfinite(r) && jf_is_i32(k, &ki) && ki >= 0 && ki <= 22) {
            while (ki-- > 0) p = jf_mul(p, t);
            if (jf_eq(p, x)) r = k;
        }
        break;
    }
    case M_SIN: r = jf_sin(x); break;
    case M_COS: r = jf_cos(x); break;
    case M_TAN: r = jf_tan(x); break;
    case M_ASIN: r = jf_asin(x); break;
    case M_ACOS: r = jf_acos(x); break;
    case M_ATAN: r = jf_atan(x); break;
    case M_EXPM1: r = jf_lt(jf_abs(x), 0x3F50624DD2F1A9FCULL) ? jf_add(x, half(jf_mul(x, x))) : jf_sub(jf_exp(x), JF_ONE); break;
    case M_LOG1P: r = jf_lt(jf_abs(x), 0x3F50624DD2F1A9FCULL) ? jf_sub(x, half(jf_mul(x, x))) : jf_log(jf_add(JF_ONE, x)); break;
    case M_SINH: r = jf_iszero(x) ? x : half(jf_sub(jf_exp(x), jf_exp(jf_neg(x)))); break;
    case M_COSH: r = half(jf_add(jf_exp(x), jf_exp(jf_neg(x)))); break;
    case M_TANH:
        if (jf_iszero(x) || jf_isnan(x)) r = x;
        else if (jf_lt(jf_from_i32(20), jf_abs(x))) r = jf_signbit(x) ? jf_from_i32(-1) : JF_ONE;
        else { jf64 e2 = jf_exp(jf_ldexp(x, 1)); r = jf_div(jf_sub(e2, JF_ONE), jf_add(e2, JF_ONE)); }
        break;
    case M_FROUND: {   /* round to binary32 precision: 24 significant bits */
        uint64_t bits = x, mant;
        int e = (int)((bits >> 52) & 0x7FF);
        if (e == 0x7FF || jf_iszero(x) || e < 1023 - 126) { r = x; break; }
        if (e > 1023 + 127) { r = jf_signbit(x) ? JF_NINF : JF_INF; break; }
        mant = bits & 0xFFFFFFFFFFFFFULL;
        {
            uint64_t rem = mant & 0x1FFFFFFFULL, halfv = 0x10000000ULL;
            mant &= ~0x1FFFFFFFULL;
            if (rem > halfv || (rem == halfv && (mant & 0x20000000ULL))) mant += 0x20000000ULL;
            r = (bits & 0xFFF0000000000000ULL) + mant;
        }
        break;
    }
    case M_CLZ32: {
        uint32_t u = js_to_uint32(vm, JARG(0));
        int c = 0;
        if (!u) return jv_int(32);
        while (!(u & 0x80000000u)) { u <<= 1; c++; }
        return jv_int(c);
    }
    default: r = JF_NAN; break;
    }
    return jv_num(r);
}

/* max 0, min 1, hypot 2 */
static jv math_n(js_vm_t *vm, jv thisv, jv *argv, int argc, uint32_t m) {
    jf64 r = m == 0 ? JF_NINF : m == 1 ? JF_INF : JF_ZERO;
    int i, nan = 0, inf = 0;
    (void)thisv;
    for (i = 0; i < argc; i++) {
        jf64 x = js_to_number(vm, argv[i]);
        if (vm->ctl) return jv_undef();
        if (jf_isnan(x)) { nan = 1; continue; }
        if (m == 2) {
            if (jf_isinf(x)) inf = 1;
            r = jf_hypot(r, x);
        } else if (m == 0) {
            if (jf_lt(r, x) || (jf_iszero(r) && jf_iszero(x) && !jf_signbit(x))) r = x;
        } else {
            if (jf_lt(x, r) || (jf_iszero(r) && jf_iszero(x) && jf_signbit(x))) r = x;
        }
    }
    if (inf) return jv_num(JF_INF);
    if (nan) return jv_num(JF_NAN);
    return jv_num(r);
}

/* pow 0, atan2 1, imul 2 */
static jv math2(js_vm_t *vm, jv thisv, jv *argv, int argc, uint32_t m) {
    jf64 a, b;
    (void)thisv;
    if (m == 2) {
        uint32_t x = js_to_uint32(vm, JARG(0)), y = js_to_uint32(vm, JARG(1));
        return jv_int((int32_t)(x * y));
    }
    a = js_to_number(vm, JARG(0));
    b = js_to_number(vm, JARG(1));
    if (vm->ctl) return jv_undef();
    return jv_num(m == 0 ? jf_pow(a, b) : jf_atan2(a, b));
}

/* a deterministic xorshift generator: Math.random() is reproducible on purpose */
static jv math_random(js_vm_t *vm, jv thisv, jv *argv, int argc, uint32_t m) {
    uint32_t x = vm->rng, hi, lo;
    (void)thisv; (void)argv; (void)argc; (void)m;
    x ^= x << 13; x ^= x >> 17; x ^= x << 5;
    hi = x;
    x ^= x << 13; x ^= x >> 17; x ^= x << 5;
    lo = x;
    vm->rng = x;
    /* 53 random bits scaled to [0, 1) */
    return jv_num(jf_ldexp(jf_from_u64(((uint64_t)(hi >> 11) << 32) | lo), -53));
}

/* ---- JSON ---- */

#define JSON_MAX_DEPTH 48

typedef struct {
    js_sb_t b;
    jv replacer;           /* a function, or undefined */
    uint32_t allow;        /* an array of allowed keys, or 0 */
    char gap[12];
    int gap_len;
    uint32_t stack[JSON_MAX_DEPTH];
    int depth;
} json_out_t;

static void json_quote(js_vm_t *vm, js_sb_t *b, uint32_t s) {
    static const char hx[] = "0123456789abcdef";
    int n = SLEN(s), i, run = 0;
    js_sb_ch(vm, b, '"');
    for (i = 0; i < n; i++) {
        uint8_t c = (uint8_t)SCH(s, i);
        const char *esc = 0;
        char u[6];
        switch (c) {
        case '"': esc = "\\\""; break;
        case '\\': esc = "\\\\"; break;
        case '\b': esc = "\\b"; break;
        case '\f': esc = "\\f"; break;
        case '\n': esc = "\\n"; break;
        case '\r': esc = "\\r"; break;
        case '\t': esc = "\\t"; break;
        default: break;
        }
        if (!esc && c >= 0x20) { run++; continue; }
        if (run) sb_sub(vm, b, s, i - run, i);
        run = 0;
        if (esc) {
            js_sb_put(vm, b, esc, 2);
        } else {
            u[0] = '\\'; u[1] = 'u'; u[2] = '0'; u[3] = '0'; u[4] = hx[c >> 4]; u[5] = hx[c & 15];
            js_sb_put(vm, b, u, 6);
        }
    }
    if (run) sb_sub(vm, b, s, n - run, n);
    js_sb_ch(vm, b, '"');
}

static void json_indent(js_vm_t *vm, json_out_t *j) {
    int i;
    if (!j->gap_len) return;
    js_sb_ch(vm, &j->b, '\n');
    for (i = 0; i < j->depth; i++) js_sb_put(vm, &j->b, j->gap, j->gap_len);
}

/* serialize holder[key] = v; returns 0 when the value is skipped (undefined, functions) */
static int json_value(js_vm_t *vm, json_out_t *j, jv holder, uint32_t key, jv v) {
    if (v.t == JT_OBJ) {
        jv tj = js_get(vm, v, vm->s_toJSON);
        if (vm->ctl) return 0;
        if (js_is_callable(vm, tj)) {
            jv k = jv_str(key);
            v = js_call(vm, tj, v, &k, 1);
            if (vm->ctl) return 0;
        }
    }
    if (j->replacer.t != JT_UNDEF) {
        jv a2[2];
        a2[0] = jv_str(key);
        a2[1] = v;
        v = js_call(vm, j->replacer, holder, a2, 2);
        if (vm->ctl) return 0;
    }
    if (v.t == JT_OBJ) {
        int c = JOBJ(vm, v.a)->cls;
        if (c == JS_CLASS_NUMBER || c == JS_CLASS_STRING || c == JS_CLASS_BOOLEAN) {
            v = js_to_primitive(vm, v, c == JS_CLASS_STRING);
            if (vm->ctl) return 0;
        }
    }
    switch (v.t) {
    case JT_NULL: js_sb_put(vm, &j->b, "null", 4); return 1;
    case JT_BOOL: if (v.a) js_sb_put(vm, &j->b, "true", 4); else js_sb_put(vm, &j->b, "false", 5); return 1;
    case JT_INT: case JT_NUM:
        if (!jf_isfinite(jv_f64(v))) js_sb_put(vm, &j->b, "null", 4);
        else js_sb_str(vm, &j->b, js_num_to_str(vm, jv_f64(v)));
        return 1;
    case JT_STR: json_quote(vm, &j->b, v.a); return 1;
    case JT_OBJ: break;
    default: return 0;
    }
    if (js_is_callable(vm, v)) return 0;
    {
        int i, first = 1;
        for (i = 0; i < j->depth; i++)
            if (j->stack[i] == v.a) { js_throw_err(vm, JE_TYPE, "Converting circular structure to JSON"); return 0; }
        if (j->depth >= JSON_MAX_DEPTH) { js_throw_err(vm, JE_RANGE, "JSON nesting too deep for the guest"); return 0; }
        j->stack[j->depth++] = v.a;
        if (js_is_array(vm, v)) {
            uint32_t len, k;
            js_length_of(vm, v, &len);
            js_sb_ch(vm, &j->b, '[');
            for (k = 0; k < len && !vm->ctl; k++) {
                jv e = js_get_v(vm, v, jv_int((int32_t)k));
                if (vm->ctl) break;
                if (k) js_sb_ch(vm, &j->b, ',');
                json_indent(vm, j);
                if (!json_value(vm, j, v, js_key_of_index(vm, k), e) && !vm->ctl) js_sb_put(vm, &j->b, "null", 4);
            }
            j->depth--;
            if (len && !vm->ctl) json_indent(vm, j);
            js_sb_ch(vm, &j->b, ']');
            return !vm->ctl;
        }
        {
            uint32_t keys = j->allow ? j->allow : js_own_keys(vm, v.a, 1), k;
            js_sb_ch(vm, &j->b, '{');
            for (k = 0; keys && k < JOBJ(vm, keys)->alen && !vm->ctl; k++) {
                jv kk = js_array_get(vm, keys, k), e;
                uint32_t mark;
                if (j->allow && !js_has(vm, v.a, kk.a)) continue;
                e = js_get(vm, v, kk.a);
                if (vm->ctl) break;
                mark = j->b.len;
                if (!first) js_sb_ch(vm, &j->b, ',');
                json_indent(vm, j);
                json_quote(vm, &j->b, kk.a);
                js_sb_ch(vm, &j->b, ':');
                if (j->gap_len) js_sb_ch(vm, &j->b, ' ');
                if (json_value(vm, j, v, kk.a, e)) first = 0;
                else j->b.len = mark;       /* drop "key": of a skipped value */
            }
            j->depth--;
            if (!first && !vm->ctl) json_indent(vm, j);
            js_sb_ch(vm, &j->b, '}');
            return !vm->ctl;
        }
    }
}

uint32_t js_json_stringify(js_vm_t *vm, jv v, jv replacer, jv space, int *undef) {
    static json_out_t jo;     /* one stringify at a time: the interpreter is not reentrant across realms */
    json_out_t saved = jo, *j = &jo;
    uint32_t r = 0, holder;
    int i;
    *undef = 0;
    js_sb_init(vm, &j->b);
    j->replacer = jv_undef();
    j->allow = 0;
    j->gap_len = 0;
    j->depth = 0;
    if (js_is_callable(vm, replacer)) {
        j->replacer = replacer;
    } else if (js_is_array(vm, replacer)) {
        uint32_t len, k;
        js_length_of(vm, replacer, &len);
        j->allow = js_array_new(vm, len);
        for (k = 0; j->allow && k < len && !vm->ctl; k++) {
            jv e = js_get_v(vm, replacer, jv_int((int32_t)k));
            if (e.t == JT_STR || jv_is_num(e)) js_array_push(vm, j->allow, jv_str(js_to_key(vm, e)));
        }
    }
    if (space.t == JT_OBJ) space = js_to_primitive(vm, space, 0);
    if (jv_is_num(space)) {
        int32_t g = 0;
        jf64 n = jf_trunc(jv_f64(space));
        if (!jf_is_i32(n, &g)) g = jf_signbit(n) ? 0 : 10;
        if (g > 10) g = 10;
        for (i = 0; i < g; i++) j->gap[i] = ' ';
        j->gap_len = g < 0 ? 0 : g;
    } else if (space.t == JT_STR) {
        for (i = 0; i < SLEN(space.a) && i < 10; i++) j->gap[i] = SCH(space.a, i);
        j->gap_len = i;
    }
    holder = js_obj_new(vm, JS_CLASS_OBJECT, vm->p_object);
    if (holder && !vm->ctl) {
        js_define(vm, holder, vm->s_empty, v, P_DEFAULT);
        if (json_value(vm, j, jv_obj(holder), vm->s_empty, v)) r = js_sb_finish(vm, &j->b);
        else if (!vm->ctl) *undef = 1;
    }
    jo = saved;
    return r;
}

static jv json_stringify(js_vm_t *vm, jv thisv, jv *argv, int argc, uint32_t m) {
    int undef;
    uint32_t r;
    (void)thisv; (void)m;
    r = js_json_stringify(vm, JARG(0), JARG(1), JARG(2), &undef);
    if (vm->ctl || undef || !r) return jv_undef();
    return jv_str(r);
}

typedef struct { uint32_t s; int i, n; } json_in_t;

static void jp_ws(js_vm_t *vm, json_in_t *p) {
    while (p->i < p->n) {
        char c = SCH(p->s, p->i);
        if (c != ' ' && c != '\t' && c != '\n' && c != '\r') break;
        p->i++;
    }
}

static jv jp_fail(js_vm_t *vm, json_in_t *p) {
    char msg[80];
    const char *a = "Unexpected token in JSON at position ";
    int n = 0, k = p->i, d = 1;
    while (a[n]) { msg[n] = a[n]; n++; }
    while (d <= k / 10) d *= 10;
    while (d) { msg[n++] = (char)('0' + (k / d) % 10); d /= 10; }
    msg[n] = 0;
    return js_throw_err(vm, JE_SYNTAX, msg);
}

static void put_utf8(js_vm_t *vm, js_sb_t *b, uint32_t c) {
    char u[4];
    if (c < 0x80) { u[0] = (char)c; js_sb_put(vm, b, u, 1); }
    else if (c < 0x800) { u[0] = (char)(0xC0 | (c >> 6)); u[1] = (char)(0x80 | (c & 63)); js_sb_put(vm, b, u, 2); }
    else if (c < 0x10000) {
        u[0] = (char)(0xE0 | (c >> 12)); u[1] = (char)(0x80 | ((c >> 6) & 63)); u[2] = (char)(0x80 | (c & 63));
        js_sb_put(vm, b, u, 3);
    } else {
        u[0] = (char)(0xF0 | (c >> 18)); u[1] = (char)(0x80 | ((c >> 12) & 63));
        u[2] = (char)(0x80 | ((c >> 6) & 63)); u[3] = (char)(0x80 | (c & 63));
        js_sb_put(vm, b, u, 4);
    }
}

static int jp_hex4(js_vm_t *vm, json_in_t *p, uint32_t *out) {
    int k;
    uint32_t v = 0;
    if (p->i + 4 > p->n) return 0;
    for (k = 0; k < 4; k++) {
        char c = SCH(p->s, p->i + k);
        int d = (c >= '0' && c <= '9') ? c - '0' : (c >= 'a' && c <= 'f') ? c - 'a' + 10 : (c >= 'A' && c <= 'F') ? c - 'A' + 10 : -1;
        if (d < 0) return 0;
        v = v * 16 + (uint32_t)d;
    }
    p->i += 4;
    *out = v;
    return 1;
}

static jv jp_string(js_vm_t *vm, json_in_t *p) {
    js_sb_t b;
    int run;
    p->i++;      /* the opening quote */
    js_sb_init(vm, &b);
    run = p->i;
    while (p->i < p->n) {
        char c = SCH(p->s, p->i);
        if (c == '"') {
            sb_sub(vm, &b, p->s, run, p->i);
            p->i++;
            return jv_str(js_sb_finish(vm, &b));
        }
        if ((uint8_t)c < 0x20) return jp_fail(vm, p);
        if (c != '\\') { p->i++; continue; }
        sb_sub(vm, &b, p->s, run, p->i);
        p->i++;
        if (p->i >= p->n) return jp_fail(vm, p);
        c = SCH(p->s, p->i++);
        switch (c) {
        case '"': js_sb_ch(vm, &b, '"'); break;
        case '\\': js_sb_ch(vm, &b, '\\'); break;
        case '/': js_sb_ch(vm, &b, '/'); break;
        case 'b': js_sb_ch(vm, &b, '\b'); break;
        case 'f': js_sb_ch(vm, &b, '\f'); break;
        case 'n': js_sb_ch(vm, &b, '\n'); break;
        case 'r': js_sb_ch(vm, &b, '\r'); break;
        case 't': js_sb_ch(vm, &b, '\t'); break;
        case 'u': {
            uint32_t u, lo;
            if (!jp_hex4(vm, p, &u)) return jp_fail(vm, p);
            if (u >= 0xD800 && u < 0xDC00 && p->i + 1 < p->n && SCH(p->s, p->i) == '\\' && SCH(p->s, p->i + 1) == 'u') {
                int save = p->i;
                p->i += 2;
                if (jp_hex4(vm, p, &lo) && lo >= 0xDC00 && lo < 0xE000) u = 0x10000 + ((u - 0xD800) << 10) + (lo - 0xDC00);
                else p->i = save;
            }
            put_utf8(vm, &b, u);
            break;
        }
        default: p->i--; return jp_fail(vm, p);
        }
        run = p->i;
    }
    return jp_fail(vm, p);
}

static jv jp_value(js_vm_t *vm, json_in_t *p, int depth) {
    char c;
    jp_ws(vm, p);
    if (p->i >= p->n) return jp_fail(vm, p);
    if (depth > JSON_MAX_DEPTH * 4) return js_throw_err(vm, JE_RANGE, "JSON nesting too deep for the guest");
    c = SCH(p->s, p->i);
    if (c == '"') return jp_string(vm, p);
    if (c == '{') {
        uint32_t o = js_obj_new(vm, JS_CLASS_OBJECT, vm->p_object);
        if (!o) return jv_undef();
        p->i++;
        jp_ws(vm, p);
        if (p->i < p->n && SCH(p->s, p->i) == '}') { p->i++; return jv_obj(o); }
        for (;;) {
            jv k, v;
            jp_ws(vm, p);
            if (p->i >= p->n || SCH(p->s, p->i) != '"') return jp_fail(vm, p);
            k = jp_string(vm, p);
            if (vm->ctl) return jv_undef();
            jp_ws(vm, p);
            if (p->i >= p->n || SCH(p->s, p->i) != ':') return jp_fail(vm, p);
            p->i++;
            v = jp_value(vm, p, depth + 1);
            if (vm->ctl) return jv_undef();
            js_put(vm, jv_obj(o), js_intern_str(vm, k.a), v);
            jp_ws(vm, p);
            if (p->i < p->n && SCH(p->s, p->i) == ',') { p->i++; continue; }
            if (p->i < p->n && SCH(p->s, p->i) == '}') { p->i++; return jv_obj(o); }
            return jp_fail(vm, p);
        }
    }
    if (c == '[') {
        uint32_t a = js_array_new(vm, 4);
        if (!a) return jv_undef();
        p->i++;
        jp_ws(vm, p);
        if (p->i < p->n && SCH(p->s, p->i) == ']') { p->i++; return jv_obj(a); }
        for (;;) {
            jv v = jp_value(vm, p, depth + 1);
            if (vm->ctl) return jv_undef();
            js_array_push(vm, a, v);
            jp_ws(vm, p);
            if (p->i < p->n && SCH(p->s, p->i) == ',') { p->i++; continue; }
            if (p->i < p->n && SCH(p->s, p->i) == ']') { p->i++; return jv_obj(a); }
            return jp_fail(vm, p);
        }
    }
    if (c == '-' || (c >= '0' && c <= '9')) {
        int st = p->i, used;
        jf64 v;
        if (c == '-') p->i++;
        if (p->i >= p->n || SCH(p->s, p->i) < '0' || SCH(p->s, p->i) > '9') return jp_fail(vm, p);
        if (SCH(p->s, p->i) == '0' && p->i + 1 < p->n && SCH(p->s, p->i + 1) >= '0' && SCH(p->s, p->i + 1) <= '9') {
            p->i++;
            return jp_fail(vm, p);
        }
        while (p->i < p->n && SCH(p->s, p->i) >= '0' && SCH(p->s, p->i) <= '9') p->i++;
        if (p->i < p->n && SCH(p->s, p->i) == '.') {
            p->i++;
            if (p->i >= p->n || SCH(p->s, p->i) < '0' || SCH(p->s, p->i) > '9') return jp_fail(vm, p);
            while (p->i < p->n && SCH(p->s, p->i) >= '0' && SCH(p->s, p->i) <= '9') p->i++;
        }
        if (p->i < p->n && (SCH(p->s, p->i) == 'e' || SCH(p->s, p->i) == 'E')) {
            p->i++;
            if (p->i < p->n && (SCH(p->s, p->i) == '+' || SCH(p->s, p->i) == '-')) p->i++;
            if (p->i >= p->n || SCH(p->s, p->i) < '0' || SCH(p->s, p->i) > '9') return jp_fail(vm, p);
            while (p->i < p->n && SCH(p->s, p->i) >= '0' && SCH(p->s, p->i) <= '9') p->i++;
        }
        v = jf_parse(JSTR(vm, p->s)->s + st, p->i - st, &used);
        return jv_num(v);
    }
    {
        static const char *const words[] = { "true", "false", "null" };
        int w;
        for (w = 0; w < 3; w++) {
            int l = cslen(words[w]), k;
            if (p->i + l > p->n) continue;
            for (k = 0; k < l && SCH(p->s, p->i + k) == words[w][k]; k++) {}
            if (k == l) {
                p->i += l;
                return w == 0 ? jv_bool(1) : w == 1 ? jv_bool(0) : jv_null();
            }
        }
    }
    return jp_fail(vm, p);
}

static jv json_revive(js_vm_t *vm, jv rev, jv holder, uint32_t key, int depth) {
    jv v = js_get(vm, holder, key), a2[2];
    if (vm->ctl) return jv_undef();
    if (depth > JSON_MAX_DEPTH * 4) return js_throw_err(vm, JE_RANGE, "JSON nesting too deep for the guest");
    if (v.t == JT_OBJ) {
        uint32_t keys, k;
        if (js_is_array(vm, v)) {
            uint32_t len;
            js_length_of(vm, v, &len);
            for (k = 0; k < len && !vm->ctl; k++) {
                uint32_t kk = js_key_of_index(vm, k);
                jv nv = json_revive(vm, rev, v, kk, depth + 1);
                if (vm->ctl) return jv_undef();
                if (nv.t == JT_UNDEF) js_delete(vm, v, jv_str(kk));
                else js_put(vm, v, kk, nv);
            }
        } else {
            keys = js_own_keys(vm, v.a, 1);
            for (k = 0; keys && k < JOBJ(vm, keys)->alen && !vm->ctl; k++) {
                jv kk = js_array_get(vm, keys, k);
                jv nv = json_revive(vm, rev, v, kk.a, depth + 1);
                if (vm->ctl) return jv_undef();
                if (nv.t == JT_UNDEF) js_delete(vm, v, kk);
                else js_put(vm, v, kk.a, nv);
            }
        }
    }
    a2[0] = jv_str(key);
    a2[1] = v;
    return js_call(vm, rev, holder, a2, 2);
}

static jv json_parse(js_vm_t *vm, jv thisv, jv *argv, int argc, uint32_t m) {
    json_in_t p;
    jv v;
    (void)thisv; (void)m;
    p.s = js_to_string(vm, JARG(0));
    if (vm->ctl) return jv_undef();
    p.i = 0;
    p.n = SLEN(p.s);
    v = jp_value(vm, &p, 0);
    if (vm->ctl) return jv_undef();
    jp_ws(vm, &p);
    if (p.i < p.n) return jp_fail(vm, &p);
    if (js_is_callable(vm, JARG(1))) {
        uint32_t root = js_obj_new(vm, JS_CLASS_OBJECT, vm->p_object);
        if (!root) return jv_undef();
        js_define(vm, root, vm->s_empty, v, P_DEFAULT);
        return json_revive(vm, argv[1], jv_obj(root), vm->s_empty, 0);
    }
    return v;
}

/* ---- Date: a fixed clock and the UTC zone; times are jf64 ms in x0 (lo) and x1 (hi) ---- */

#define MS_DAY 86400000

static jf64 date_tv(js_vm_t *vm, jv d) { return ((jf64)JOBJ(vm, d.a)->x1 << 32) | JOBJ(vm, d.a)->x0; }

static void date_set_tv(js_vm_t *vm, jv d, jf64 t) {
    JOBJ(vm, d.a)->x0 = (uint32_t)t;
    JOBJ(vm, d.a)->x1 = (uint32_t)(t >> 32);
}

/* TimeClip: NaN outside +-8.64e15 ms */
static jf64 time_clip(jf64 t) {
    if (!jf_isfinite(t) || jf_lt(0x433EB208C2DC0000ULL, jf_abs(t))) return JF_NAN;
    return jf_add(jf_trunc(t), JF_ZERO);
}

/* days from 1970-01-01 of a proleptic Gregorian date (month 1..12): int32 only */
static int32_t days_from_civil(int32_t y, int32_t m, int32_t d) {
    int32_t era, yoe, doy, doe;
    y -= m <= 2;
    era = (y >= 0 ? y : y - 399) / 400;
    yoe = y - era * 400;
    doy = (153 * (m + (m > 2 ? -3 : 9)) + 2) / 5 + d - 1;
    doe = yoe * 365 + yoe / 4 - yoe / 100 + doy;
    return era * 146097 + doe - 719468;
}

static void civil_from_days(int32_t z, int32_t *y, int32_t *m, int32_t *d) {
    int32_t era, doe, yoe, doy, mp;
    z += 719468;
    era = (z >= 0 ? z : z - 146096) / 146097;
    doe = z - era * 146097;
    yoe = (doe - doe / 1460 + doe / 36524 - doe / 146096) / 365;
    *y = yoe + era * 400;
    doy = doe - (365 * yoe + yoe / 4 - yoe / 100);
    mp = (5 * doy + 2) / 153;
    *d = doy - (153 * mp + 2) / 5 + 1;
    *m = mp < 10 ? mp + 3 : mp - 9;
    *y += *m <= 2;
}

/* fields: year, month 0..11, day 1..31, hours, minutes, seconds, ms, weekday */
static int date_fields(jf64 t, int32_t f[8]) {
    jf64 day;
    int32_t dn, ms, mo;
    if (jf_isnan(t)) return 0;
    day = jf_floor(jf_div(t, jf_from_i32(MS_DAY)));
    dn = jf_to_i32(day);
    ms = jf_to_i32(jf_sub(t, jf_mul(day, jf_from_i32(MS_DAY))));
    civil_from_days(dn, &f[0], &mo, &f[2]);
    f[1] = mo - 1;
    f[3] = ms / 3600000;
    f[4] = ms / 60000 % 60;
    f[5] = ms / 1000 % 60;
    f[6] = ms % 1000;
    f[7] = ((dn % 7) + 11) % 7;     /* 1970-01-01 was a Thursday (4) */
    return 1;
}

/* MakeDate(MakeDay(y, m, d), MakeTime(h, mi, s, ms)) with jf64 fields (months may overflow) */
static jf64 make_date(const jf64 v[7]) {
    int i;
    jf64 y, m, ym, mn, days, t;
    int32_t yi, mi;
    for (i = 0; i < 7; i++) if (!jf_isfinite(v[i])) return JF_NAN;
    y = jf_trunc(v[0]);
    m = jf_trunc(v[1]);
    ym = jf_add(y, jf_floor(jf_div(m, jf_from_i32(12))));
    mn = jf_rem(m, jf_from_i32(12));
    if (jf_lt(mn, JF_ZERO)) mn = jf_add(mn, jf_from_i32(12));
    if (jf_lt(jf_from_i32(400000), jf_abs(ym))) return JF_NAN;
    yi = jf_to_i32(ym);
    mi = jf_to_i32(mn);
    days = jf_add(jf_from_i32(days_from_civil(yi, mi + 1, 1)), jf_sub(jf_trunc(v[2]), JF_ONE));
    t = jf_add(jf_add(jf_mul(jf_trunc(v[3]), jf_from_i32(3600000)), jf_mul(jf_trunc(v[4]), jf_from_i32(60000))),
               jf_add(jf_mul(jf_trunc(v[5]), jf_from_i32(1000)), jf_trunc(v[6])));
    return jf_add(jf_mul(days, jf_from_i32(MS_DAY)), t);
}

static int pdig(const char *s, int n, int *i, int count, int32_t *out) {
    int k;
    int32_t v = 0;
    if (*i + count > n) return 0;
    for (k = 0; k < count; k++) {
        char c = s[*i + k];
        if (c < '0' || c > '9') return 0;
        v = v * 10 + (c - '0');
    }
    *i += count;
    *out = v;
    return 1;
}

static int month_of(const char *s) {
    static const char *const mon = "JanFebMarAprMayJunJulAugSepOctNovDec";
    int k;
    for (k = 0; k < 12; k++)
        if ((s[0] | 32) == (mon[3 * k] | 32) && (s[1] | 32) == mon[3 * k + 1] && (s[2] | 32) == mon[3 * k + 2]) return k;
    return -1;
}

/* ISO 8601 (YYYY, YYYY-MM, YYYY-MM-DD, then THH:mm[:ss[.sss]] and Z or +-HH:mm)
 * and the "Mon DD YYYY [HH:mm[:ss]]" / "DD Mon YYYY" forms of toString */
static jf64 date_parse(js_vm_t *vm, uint32_t str) {
    const char *s = JSTR(vm, str)->s;
    int n = SLEN(str), i = 0, sign = 1;
    int32_t y = 0, mo = 1, d = 1, h = 0, mi = 0, se = 0, ms = 0, oh = 0, om = 0;
    jf64 v[7];
    while (i < n && s[i] == ' ') i++;
    if (i < n && (s[i] == '+' || s[i] == '-')) {
        sign = s[i] == '-' ? -1 : 1;
        i++;
        if (!pdig(s, n, &i, 6, &y)) return JF_NAN;
    } else if (!pdig(s, n, &i, 4, &y)) {
        /* "Jan 15 2026 10:00:00" or "Thu Jan 15 2026" or "15 Jan 2026" */
        int k = i, mon = -1;
        int32_t a = 0;
        int have_d = 0;
        while (k + 3 <= n && mon < 0) {
            if (s[k] >= 'A' && s[k] <= 'z') {
                mon = month_of(s + k);
                while (k < n && s[k] != ' ' && s[k] != ',') k++;
            } else if (s[k] >= '0' && s[k] <= '9') {
                a = 0;
                while (k < n && s[k] >= '0' && s[k] <= '9') a = a * 10 + (s[k++] - '0');
                d = a;
                have_d = 1;
            } else {
                k++;
            }
        }
        if (mon < 0) return JF_NAN;
        mo = mon + 1;
        while (k < n && (s[k] == ' ' || s[k] == ',')) k++;
        a = 0;
        if (!have_d) {
            while (k < n && s[k] >= '0' && s[k] <= '9') a = a * 10 + (s[k++] - '0');
            d = a;
            while (k < n && (s[k] == ' ' || s[k] == ',')) k++;
            a = 0;
        }
        if (k >= n || s[k] < '0' || s[k] > '9') return JF_NAN;
        while (k < n && s[k] >= '0' && s[k] <= '9') a = a * 10 + (s[k++] - '0');
        y = a;
        while (k < n && s[k] == ' ') k++;
        i = k;
        if (i < n && pdig(s, n, &i, 2, &h)) {
            if (i < n && s[i] == ':') { i++; if (!pdig(s, n, &i, 2, &mi)) return JF_NAN; }
            if (i < n && s[i] == ':') { i++; if (!pdig(s, n, &i, 2, &se)) return JF_NAN; }
        }
        goto build;
    }
    y *= sign;
    if (i < n && s[i] == '-') {
        i++;
        if (!pdig(s, n, &i, 2, &mo)) return JF_NAN;
        if (i < n && s[i] == '-') { i++; if (!pdig(s, n, &i, 2, &d)) return JF_NAN; }
    }
    if (i < n && (s[i] == 'T' || s[i] == 't' || s[i] == ' ')) {
        i++;
        if (!pdig(s, n, &i, 2, &h) || i >= n || s[i] != ':') return JF_NAN;
        i++;
        if (!pdig(s, n, &i, 2, &mi)) return JF_NAN;
        if (i < n && s[i] == ':') {
            i++;
            if (!pdig(s, n, &i, 2, &se)) return JF_NAN;
            if (i < n && s[i] == '.') {
                int k = 0;
                i++;
                ms = 0;
                while (i < n && s[i] >= '0' && s[i] <= '9') {
                    if (k < 3) ms = ms * 10 + (s[i] - '0');
                    k++;
                    i++;
                }
                if (!k) return JF_NAN;
                while (k++ < 3) ms *= 10;
            }
        }
        if (i < n && (s[i] == 'Z' || s[i] == 'z')) {
            i++;
        } else if (i < n && (s[i] == '+' || s[i] == '-')) {
            int os = s[i] == '-' ? -1 : 1;
            i++;
            if (!pdig(s, n, &i, 2, &oh)) return JF_NAN;
            if (i < n && s[i] == ':') i++;
            if (!pdig(s, n, &i, 2, &om)) return JF_NAN;
            oh *= os;
            om *= os;
        }
    }
    while (i < n && s[i] == ' ') i++;
    if (i != n) return JF_NAN;
build:
    if (mo < 1 || mo > 12 || d < 1 || d > 31 || h > 24 || mi > 59 || se > 59) return JF_NAN;
    v[0] = jf_from_i32(y);
    v[1] = jf_from_i32(mo - 1);
    v[2] = jf_from_i32(d);
    v[3] = jf_from_i32(h - oh);
    v[4] = jf_from_i32(mi - om);
    v[5] = jf_from_i32(se);
    v[6] = jf_from_i32(ms);
    return time_clip(make_date(v));
}

static jf64 now_ms(js_vm_t *vm) { return jf_from_u64(((uint64_t)vm->date_now_hi << 32) | vm->date_now_lo); }

static jv date_obj(js_vm_t *vm, jf64 t) {
    uint32_t o;
    if (vm->super_this && JOBJ(vm, vm->super_this)->cls == JS_CLASS_OBJECT) {
        o = vm->super_this;
        JOBJ(vm, o)->cls = JS_CLASS_DATE;
        vm->super_this = 0;
    } else {
        o = js_obj_new(vm, JS_CLASS_DATE, vm->p_date);
    }
    if (!o) return jv_undef();
    date_set_tv(vm, jv_obj(o), time_clip(t));
    return jv_obj(o);
}

static void put2(char *b, int *n, int32_t v) { b[(*n)++] = (char)('0' + v / 10 % 10); b[(*n)++] = (char)('0' + v % 10); }

static void put_year(char *b, int *n, int32_t y) {
    int32_t a = y < 0 ? -y : y, d = 1000;
    if (y < 0) b[(*n)++] = '-';
    while (d * 10 <= a) d *= 10;
    for (; d; d /= 10) b[(*n)++] = (char)('0' + a / d % 10);
}

static const char *const k_days = "SunMonTueWedThuFriSat";
static const char *const k_mons = "JanFebMarAprMayJunJulAugSepOctNovDec";

/* toISOString 0, toString 1, toDateString 2, toTimeString 3, toUTCString 4,
 * toLocaleDateString 5, toLocaleTimeString 6, toLocaleString 7, toJSON 8 */
static jv datep_str(js_vm_t *vm, jv thisv, jv *argv, int argc, uint32_t m) {
    char b[96];
    int n = 0, k;
    int32_t f[8];
    (void)argv; (void)argc;
    if (thisv.t != JT_OBJ || JOBJ(vm, thisv.a)->cls != JS_CLASS_DATE) {
        if (m == 8) {
            jv fn = js_get(vm, thisv, KEY(vm, "toISOString"));
            if (vm->ctl) return jv_undef();
            return js_call(vm, fn, thisv, 0, 0);
        }
        return js_throw_err(vm, JE_TYPE, "this is not a Date object.");
    }
    if (!date_fields(date_tv(vm, thisv), f)) {
        if (m == 0) return js_throw_err(vm, JE_RANGE, "Invalid time value");
        if (m == 8) return jv_null();
        return jv_str(js_str_c(vm, "Invalid Date"));
    }
    if (m == 0 || m == 8) {
        if (f[0] < 0 || f[0] > 9999) {
            b[n++] = f[0] < 0 ? '-' : '+';
            put2(b, &n, (f[0] < 0 ? -f[0] : f[0]) / 10000);
            k = (f[0] < 0 ? -f[0] : f[0]) % 10000;
            put2(b, &n, k / 100);
            put2(b, &n, k % 100);
        } else {
            put2(b, &n, f[0] / 100);
            put2(b, &n, f[0] % 100);
        }
        b[n++] = '-'; put2(b, &n, f[1] + 1); b[n++] = '-'; put2(b, &n, f[2]);
        b[n++] = 'T'; put2(b, &n, f[3]); b[n++] = ':'; put2(b, &n, f[4]); b[n++] = ':'; put2(b, &n, f[5]);
        b[n++] = '.'; b[n++] = (char)('0' + f[6] / 100); put2(b, &n, f[6] % 100); b[n++] = 'Z';
    } else if (m >= 5 && m <= 7) {     /* en-US: 1/15/2026, 10:05:00 AM */
        if (m != 6) {
            put_year(b, &n, f[1] + 1);
            b[n++] = '/';
            put_year(b, &n, f[2]);
            b[n++] = '/';
            put_year(b, &n, f[0]);
        }
        if (m == 7) { b[n++] = ','; b[n++] = ' '; }
        if (m != 5) {
            int32_t h12 = f[3] % 12 == 0 ? 12 : f[3] % 12;
            put_year(b, &n, h12);
            b[n++] = ':'; put2(b, &n, f[4]); b[n++] = ':'; put2(b, &n, f[5]);
            b[n++] = ' '; b[n++] = f[3] < 12 ? 'A' : 'P'; b[n++] = 'M';
        }
    } else {
        if (m == 4) {
            for (k = 0; k < 3; k++) b[n++] = k_days[3 * f[7] + k];
            b[n++] = ','; b[n++] = ' ';
            put2(b, &n, f[2]); b[n++] = ' ';
            for (k = 0; k < 3; k++) b[n++] = k_mons[3 * f[1] + k];
            b[n++] = ' ';
            if (f[0] >= 0 && f[0] < 1000) { b[n++] = '0'; if (f[0] < 100) b[n++] = '0'; if (f[0] < 10) b[n++] = '0'; }
            put_year(b, &n, f[0]);
            b[n++] = ' '; put2(b, &n, f[3]); b[n++] = ':'; put2(b, &n, f[4]); b[n++] = ':'; put2(b, &n, f[5]);
            b[n++] = ' '; b[n++] = 'G'; b[n++] = 'M'; b[n++] = 'T';
        } else {
            if (m != 3) {
                for (k = 0; k < 3; k++) b[n++] = k_days[3 * f[7] + k];
                b[n++] = ' ';
                for (k = 0; k < 3; k++) b[n++] = k_mons[3 * f[1] + k];
                b[n++] = ' ';
                put2(b, &n, f[2]);
                b[n++] = ' ';
                if (f[0] >= 0 && f[0] < 1000) { b[n++] = '0'; if (f[0] < 100) b[n++] = '0'; if (f[0] < 10) b[n++] = '0'; }
                put_year(b, &n, f[0]);
            }
            if (m == 1) b[n++] = ' ';
            if (m != 2) {
                const char *z = " GMT+0000 (Coordinated Universal Time)";
                put2(b, &n, f[3]); b[n++] = ':'; put2(b, &n, f[4]); b[n++] = ':'; put2(b, &n, f[5]);
                for (k = 0; z[k]; k++) b[n++] = z[k];
            }
        }
    }
    return jv_str(js_str_new(vm, b, n));
}

static jv date_ctor(js_vm_t *vm, jv thisv, jv *argv, int argc, uint32_t m) {
    jf64 t;
    (void)thisv; (void)m;
    if (!vm->in_new && !vm->super_this) {      /* Date() as a function: the current time as a string */
        jv d = date_obj(vm, now_ms(vm));
        if (vm->ctl) return jv_undef();
        return datep_str(vm, d, 0, 0, 1);
    }
    if (argc == 0) {
        t = now_ms(vm);
    } else if (argc == 1) {
        jv v = argv[0];
        if (v.t == JT_OBJ && JOBJ(vm, v.a)->cls == JS_CLASS_DATE) t = date_tv(vm, v);
        else {
            v = js_to_primitive(vm, v, 0);
            if (vm->ctl) return jv_undef();
            t = v.t == JT_STR ? date_parse(vm, v.a) : js_to_number(vm, v);
        }
    } else {
        jf64 f[7];
        int i;
        for (i = 0; i < 7; i++) {
            f[i] = i < argc ? js_to_number(vm, argv[i]) : (i == 2 ? JF_ONE : JF_ZERO);
            if (vm->ctl) return jv_undef();
        }
        if (jf_isfinite(f[0])) {
            jf64 yi = jf_trunc(f[0]);
            if (jf_le(JF_ZERO, yi) && jf_le(yi, jf_from_i32(99))) f[0] = jf_add(yi, jf_from_i32(1900));
        }
        t = make_date(f);
    }
    if (vm->ctl) return jv_undef();
    return date_obj(vm, t);
}

/* Date.now 0, Date.parse 1, Date.UTC 2 */
static jv date_static(js_vm_t *vm, jv thisv, jv *argv, int argc, uint32_t m) {
    (void)thisv;
    if (m == 0) return jv_num(now_ms(vm));
    if (m == 1) {
        uint32_t s = js_to_string(vm, JARG(0));
        if (vm->ctl) return jv_undef();
        return jv_num(date_parse(vm, s));
    }
    {
        jf64 f[7];
        int i;
        for (i = 0; i < 7; i++) {
            f[i] = i < argc ? js_to_number(vm, argv[i]) : (i == 2 ? JF_ONE : JF_ZERO);
            if (vm->ctl) return jv_undef();
        }
        if (jf_isfinite(f[0])) {
            jf64 yi = jf_trunc(f[0]);
            if (jf_le(JF_ZERO, yi) && jf_le(yi, jf_from_i32(99))) f[0] = jf_add(yi, jf_from_i32(1900));
        }
        return jv_num(time_clip(make_date(f)));
    }
}

static int this_date(js_vm_t *vm, jv thisv) {
    if (thisv.t == JT_OBJ && JOBJ(vm, thisv.a)->cls == JS_CLASS_DATE) return 1;
    js_throw_err(vm, JE_TYPE, "this is not a Date object.");
    return 0;
}

/* getTime 0, getFullYear 1, getMonth 2, getDate 3, getDay 4, getHours 5, getMinutes 6,
 * getSeconds 7, getMilliseconds 8, getTimezoneOffset 9, getYear 10 */
static jv datep_get(js_vm_t *vm, jv thisv, jv *argv, int argc, uint32_t m) {
    int32_t f[8];
    jf64 t;
    (void)argv; (void)argc;
    if (!this_date(vm, thisv)) return jv_undef();
    t = date_tv(vm, thisv);
    if (m == 0) return jv_num(t);
    if (!date_fields(t, f)) return jv_num(JF_NAN);
    switch (m) {
    case 1: return jv_int(f[0]);
    case 2: return jv_int(f[1]);
    case 3: return jv_int(f[2]);
    case 4: return jv_int(f[7]);
    case 5: return jv_int(f[3]);
    case 6: return jv_int(f[4]);
    case 7: return jv_int(f[5]);
    case 8: return jv_int(f[6]);
    case 9: return jv_int(0);
    default: return jv_int(f[0] - 1900);
    }
}

/* setTime 7, otherwise the first field index set: setFullYear 0, setMonth 1, setDate 2,
 * setHours 3, setMinutes 4, setSeconds 5, setMilliseconds 6 */
static jv datep_set(js_vm_t *vm, jv thisv, jv *argv, int argc, uint32_t m) {
    static const int maxargs[7] = { 3, 2, 1, 4, 3, 2, 1 };
    int32_t f[8];
    jf64 v[7], t;
    int i;
    if (!this_date(vm, thisv)) return jv_undef();
    if (m == 7) {
        t = time_clip(js_to_number(vm, JARG(0)));
        if (vm->ctl) return jv_undef();
        date_set_tv(vm, thisv, t);
        return jv_num(t);
    }
    t = date_tv(vm, thisv);
    if (!date_fields(t, f)) {
        if (m != 0) return jv_num(JF_NAN);
        for (i = 0; i < 7; i++) f[i] = i == 2 ? 1 : 0;
    }
    for (i = 0; i < 7; i++) v[i] = jf_from_i32(f[i]);
    for (i = 0; i < maxargs[m] && i < argc; i++) {
        v[m + i] = js_to_number(vm, argv[i]);
        if (vm->ctl) return jv_undef();
    }
    if (argc == 0) v[m] = JF_NAN;
    t = time_clip(make_date(v));
    date_set_tv(vm, thisv, t);
    return jv_num(t);
}

/* ---- Map and Set: x0 keys array, x1 values array (Map), x2 live count;
 * a deleted entry leaves an empty slot so forEach order holds */

static int coll_this(js_vm_t *vm, jv thisv, int cls) {
    if (thisv.t == JT_OBJ && JOBJ(vm, thisv.a)->cls == cls) return 1;
    js_throw_err(vm, JE_TYPE, cls == JS_CLASS_MAP ? "Method Map.prototype called on incompatible receiver" :
                                                    "Method Set.prototype called on incompatible receiver");
    return 0;
}

static int coll_find(js_vm_t *vm, uint32_t o, jv k) {
    uint32_t ks = JOBJ(vm, o)->x0, n = JOBJ(vm, ks)->alen, i;
    for (i = 0; i < n; i++) {
        jv e = ((jv *)JPTR(vm, JOBJ(vm, ks)->arr))[i];
        if (e.t != JT_EMPTY && js_same_value_zero(vm, e, k)) return (int)i;
    }
    return -1;
}

static void coll_add(js_vm_t *vm, uint32_t o, jv k, jv v) {
    int i;
    if (jv_is_num(k) && jf_iszero(jv_f64(k))) k = jv_int(0);     /* -0 is stored as +0 */
    i = coll_find(vm, o, k);
    if (i >= 0) {
        if (JOBJ(vm, o)->cls == JS_CLASS_MAP) js_array_set(vm, JOBJ(vm, o)->x1, (uint32_t)i, v);
        return;
    }
    js_array_push(vm, JOBJ(vm, o)->x0, k);
    if (JOBJ(vm, o)->cls == JS_CLASS_MAP) js_array_push(vm, JOBJ(vm, o)->x1, v);
    JOBJ(vm, o)->x2++;
}

static jv coll_ctor(js_vm_t *vm, jv thisv, jv *argv, int argc, uint32_t cls) {
    uint32_t o, ks, vs = 0, arr = 0, i;
    (void)thisv;
    if (!vm->in_new && !vm->super_this)
        return js_throw_err(vm, JE_TYPE, cls == JS_CLASS_MAP ? "Constructor Map requires 'new'" : "Constructor Set requires 'new'");
    if (vm->super_this && JOBJ(vm, vm->super_this)->cls == JS_CLASS_OBJECT) {
        o = vm->super_this;
        JOBJ(vm, o)->cls = (uint8_t)cls;
        vm->super_this = 0;
    } else {
        o = js_obj_new(vm, (int)cls, cls == JS_CLASS_MAP ? vm->p_map : vm->p_set);
    }
    ks = js_array_new(vm, 8);
    if (cls == JS_CLASS_MAP) vs = js_array_new(vm, 8);
    if (!o || !ks || (cls == JS_CLASS_MAP && !vs)) return jv_undef();
    JOBJ(vm, o)->x0 = ks;
    JOBJ(vm, o)->x1 = vs;
    JOBJ(vm, o)->x2 = 0;
    if (jv_is_nullish(JARG(0))) return jv_obj(o);
    js_iter_values(vm, argv[0], &arr);
    if (vm->ctl) return jv_undef();
    for (i = 0; i < JOBJ(vm, arr)->alen && !vm->ctl; i++) {
        jv e = js_array_get(vm, arr, i);
        if (cls == JS_CLASS_SET) { coll_add(vm, o, e, jv_undef()); continue; }
        if (e.t != JT_OBJ) return js_throw_err(vm, JE_TYPE, "Iterator value is not an entry object");
        {
            jv k = js_get_v(vm, e, jv_int(0)), v = js_get_v(vm, e, jv_int(1));
            if (vm->ctl) return jv_undef();
            coll_add(vm, o, k, v);
        }
    }
    return vm->ctl ? jv_undef() : jv_obj(o);
}

/* get 0, set 1, has 2, delete 3, clear 4, size 5, forEach 6, keys 7, values 8, entries 9 */
static jv coll_method(js_vm_t *vm, jv thisv, jv *argv, int argc, uint32_t m) {
    int cls = (int)(m >> 8), i;
    uint32_t o, op = m & 0xFFu;
    if (!coll_this(vm, thisv, cls)) return jv_undef();
    o = thisv.a;
    switch (op) {
    case 0:
        i = coll_find(vm, o, JARG(0));
        return i < 0 ? jv_undef() : js_array_get(vm, JOBJ(vm, o)->x1, (uint32_t)i);
    case 1:
        coll_add(vm, o, JARG(0), cls == JS_CLASS_MAP ? JARG(1) : jv_undef());
        return thisv;
    case 2:
        return jv_bool(coll_find(vm, o, JARG(0)) >= 0);
    case 3:
        i = coll_find(vm, o, JARG(0));
        if (i < 0) return jv_bool(0);
        ((jv *)JPTR(vm, JOBJ(vm, JOBJ(vm, o)->x0)->arr))[i] = jv_empty();
        if (cls == JS_CLASS_MAP) ((jv *)JPTR(vm, JOBJ(vm, JOBJ(vm, o)->x1)->arr))[i] = jv_undef();
        JOBJ(vm, o)->x2--;
        return jv_bool(1);
    case 4:
        js_array_set_len(vm, JOBJ(vm, o)->x0, 0);
        if (cls == JS_CLASS_MAP) js_array_set_len(vm, JOBJ(vm, o)->x1, 0);
        JOBJ(vm, o)->x2 = 0;
        return jv_undef();
    case 5:
        return jv_int((int32_t)JOBJ(vm, o)->x2);
    case 6: {
        jv fn = JARG(0);
        uint32_t k;
        if (!js_is_callable(vm, fn)) return js_throw_err(vm, JE_TYPE, "callback is not a function");
        for (k = 0; k < JOBJ(vm, JOBJ(vm, o)->x0)->alen && !vm->ctl; k++) {   /* entries added meanwhile are visited */
            jv key = ((jv *)JPTR(vm, JOBJ(vm, JOBJ(vm, o)->x0)->arr))[k], a3[3];
            if (key.t == JT_EMPTY) continue;
            a3[0] = cls == JS_CLASS_MAP ? js_array_get(vm, JOBJ(vm, o)->x1, k) : key;
            a3[1] = key;
            a3[2] = thisv;
            js_call(vm, fn, JARG(1), a3, 3);
        }
        return jv_undef();
    }
    default: {
        uint32_t arr = 0, out, k;
        if (op == 9 || (op == 8 && cls == JS_CLASS_SET) || (op == 7 && cls == JS_CLASS_SET)) {
            if (op == 9 && cls == JS_CLASS_SET) {
                js_iter_values(vm, thisv, &arr);
                out = js_array_new(vm, arr ? JOBJ(vm, arr)->alen : 0);
                for (k = 0; out && arr && k < JOBJ(vm, arr)->alen; k++) {
                    uint32_t pair = js_array_new(vm, 2);
                    if (!pair) return jv_undef();
                    js_array_push(vm, pair, js_array_get(vm, arr, k));
                    js_array_push(vm, pair, js_array_get(vm, arr, k));
                    js_array_push(vm, out, jv_obj(pair));
                }
                return out ? jv_obj(out) : jv_undef();
            }
            return js_iter_values(vm, thisv, &arr);
        }
        out = js_array_new(vm, JOBJ(vm, o)->x2);
        if (!out) return jv_undef();
        for (k = 0; k < JOBJ(vm, JOBJ(vm, o)->x0)->alen; k++) {
            jv key = ((jv *)JPTR(vm, JOBJ(vm, JOBJ(vm, o)->x0)->arr))[k];
            if (key.t == JT_EMPTY) continue;
            js_array_push(vm, out, op == 7 ? key : js_array_get(vm, JOBJ(vm, o)->x1, k));
        }
        return jv_obj(out);
    }
    }
}

/* ---- realm setup ---- */

static void defs(js_vm_t *vm, uint32_t o, const char *const *names, js_native_fn fn, int nargs, uint32_t base) {
    int i;
    for (i = 0; names[i]; i++) js_def_native(vm, o, names[i], fn, nargs, base + (uint32_t)i);
}

static int lib_string(js_vm_t *vm) {
    static const char *const chr[] = { "charAt", "charCodeAt", "codePointAt", "at", 0 };
    static const char *const fnd[] = { "indexOf", "lastIndexOf", "includes", "startsWith", "endsWith", 0 };
    static const char *const slc[] = { "slice", "substring", "substr", 0 };
    static const char *const cs[] = { "toUpperCase", "toLowerCase", 0 };
    static const char *const lcs[] = { "toLocaleUpperCase", "toLocaleLowerCase", 0 };
    static const char *const trm[] = { "trim", "trimStart", "trimEnd", 0 };
    static const char *const pad[] = { "padStart", "padEnd", 0 };
    static const char *const rep[] = { "replace", "replaceAll", 0 };
    static const char *const mat[] = { "match", "matchAll", "search", 0 };
    uint32_t c = js_ctor_new(vm, "String", str_ctor, 1, vm->p_string), o = vm->p_string;
    if (!c) return -1;
    js_def_native(vm, c, "fromCharCode", str_from_code, 1, 0);
    js_def_native(vm, c, "fromCodePoint", str_from_code, 1, 1);
    js_def_native(vm, c, "raw", str_raw, 1, 0);
    defs(vm, o, chr, strp_char, 1, 0);
    defs(vm, o, fnd, strp_find, 1, 0);
    defs(vm, o, slc, strp_slice, 2, 0);
    defs(vm, o, cs, strp_case, 0, 0);
    defs(vm, o, lcs, strp_case, 0, 0);
    defs(vm, o, trm, strp_trim, 0, 0);
    js_def_native(vm, o, "trimLeft", strp_trim, 0, 1);
    js_def_native(vm, o, "trimRight", strp_trim, 0, 2);
    defs(vm, o, pad, strp_pad, 2, 0);
    defs(vm, o, rep, strp_replace, 2, 0);
    defs(vm, o, mat, strp_match, 1, 0);
    js_def_native(vm, o, "split", strp_split, 2, 0);
    js_def_native(vm, o, "repeat", strp_repeat, 1, 0);
    js_def_native(vm, o, "concat", strp_concat, 1, 0);
    js_def_native(vm, o, "localeCompare", strp_locale_cmp, 1, 0);
    js_def_native(vm, o, "normalize", strp_identity, 0, 0);
    js_def_native(vm, o, "toString", strp_value, 0, 0);
    js_def_native(vm, o, "valueOf", strp_value, 0, 0);
    return 0;
}

static int lib_number(js_vm_t *vm) {
    static const char *const is[] = { "isInteger", "isSafeInteger", "isFinite", "isNaN", 0 };
    static const char *const fmt[] = { "toString", "toFixed", "toPrecision", "toExponential", "valueOf", "toLocaleString", 0 };
    uint32_t c = js_ctor_new(vm, "Number", num_ctor, 1, vm->p_number), b;
    jv pi, pf;
    if (!c) return -1;
    defs(vm, c, is, num_is, 1, 0);
    defs(vm, vm->p_number, fmt, nump_fmt, 1, 0);
    js_define_c(vm, c, "MAX_SAFE_INTEGER", jv_num(JF_MAXSAFE), 0);
    js_define_c(vm, c, "MIN_SAFE_INTEGER", jv_num(jf_neg(JF_MAXSAFE)), 0);
    js_define_c(vm, c, "EPSILON", jv_num(JF_EPS), 0);
    js_define_c(vm, c, "MAX_VALUE", jv_num(JF_MAXV), 0);
    js_define_c(vm, c, "MIN_VALUE", jv_num(JF_MINV), 0);
    js_define_c(vm, c, "POSITIVE_INFINITY", jv_num(JF_INF), 0);
    js_define_c(vm, c, "NEGATIVE_INFINITY", jv_num(JF_NINF), 0);
    js_define_c(vm, c, "NaN", jv_num(JF_NAN), 0);
    pi = js_get_c(vm, jv_obj(vm->global), "parseInt");
    pf = js_get_c(vm, jv_obj(vm->global), "parseFloat");
    js_define_c(vm, c, "parseInt", pi, P_HIDDEN);
    js_define_c(vm, c, "parseFloat", pf, P_HIDDEN);
    b = js_ctor_new(vm, "Boolean", bool_ctor, 1, vm->p_boolean);
    if (!b) return -1;
    js_def_native(vm, vm->p_boolean, "toString", boolp, 0, 0);
    js_def_native(vm, vm->p_boolean, "valueOf", boolp, 0, 1);
    return 0;
}

static int lib_math(js_vm_t *vm) {
    static const char *const one[] = { "abs", "floor", "ceil", "round", "trunc", "sign", "sqrt", "cbrt", "exp", "log",
                                       "log2", "log10", "sin", "cos", "tan", "asin", "acos", "atan", "expm1", "log1p",
                                       "sinh", "cosh", "tanh", "fround", "clz32", 0 };
    static const char *const many[] = { "max", "min", "hypot", 0 };
    static const char *const two[] = { "pow", "atan2", "imul", 0 };
    uint32_t mo = js_obj_new(vm, JS_CLASS_OBJECT, vm->p_object);
    if (!mo) return -1;
    js_define_c(vm, vm->global, "Math", jv_obj(mo), P_HIDDEN);
    defs(vm, mo, one, math1, 1, 0);
    defs(vm, mo, many, math_n, 2, 0);
    defs(vm, mo, two, math2, 2, 0);
    js_def_native(vm, mo, "random", math_random, 0, 0);
    js_define_c(vm, mo, "PI", jv_num(JF_PI), 0);
    js_define_c(vm, mo, "E", jv_num(JF_E), 0);
    js_define_c(vm, mo, "LN2", jv_num(JF_LN2), 0);
    js_define_c(vm, mo, "LN10", jv_num(JF_LN10), 0);
    js_define_c(vm, mo, "LOG2E", jv_num(JF_LOG2E), 0);
    js_define_c(vm, mo, "LOG10E", jv_num(JF_LOG10E), 0);
    js_define_c(vm, mo, "SQRT2", jv_num(JF_SQRT2), 0);
    js_define_c(vm, mo, "SQRT1_2", jv_num(JF_SQRT1_2), 0);
    return 0;
}

static int lib_json(js_vm_t *vm) {
    uint32_t j = js_obj_new(vm, JS_CLASS_OBJECT, vm->p_object);
    if (!j) return -1;
    js_define_c(vm, vm->global, "JSON", jv_obj(j), P_HIDDEN);
    js_def_native(vm, j, "stringify", json_stringify, 3, 0);
    js_def_native(vm, j, "parse", json_parse, 2, 0);
    return 0;
}

static int lib_date(js_vm_t *vm) {
    static const char *const st[] = { "toISOString", "toString", "toDateString", "toTimeString", "toUTCString",
                                      "toLocaleDateString", "toLocaleTimeString", "toLocaleString", "toJSON", 0 };
    static const char *const get[] = { "getTime", "getFullYear", "getMonth", "getDate", "getDay", "getHours",
                                       "getMinutes", "getSeconds", "getMilliseconds", "getTimezoneOffset", "getYear", 0 };
    static const char *const uget[] = { "valueOf", "getUTCFullYear", "getUTCMonth", "getUTCDate", "getUTCDay",
                                        "getUTCHours", "getUTCMinutes", "getUTCSeconds", "getUTCMilliseconds", 0 };
    static const char *const set[] = { "setFullYear", "setMonth", "setDate", "setHours", "setMinutes", "setSeconds",
                                       "setMilliseconds", "setTime", 0 };
    static const char *const uset[] = { "setUTCFullYear", "setUTCMonth", "setUTCDate", "setUTCHours", "setUTCMinutes",
                                        "setUTCSeconds", "setUTCMilliseconds", 0 };
    static const char *const sta[] = { "now", "parse", "UTC", 0 };
    uint32_t c = js_ctor_new(vm, "Date", date_ctor, 7, vm->p_date), p = vm->p_date;
    if (!c) return -1;
    defs(vm, c, sta, date_static, 1, 0);
    defs(vm, p, st, datep_str, 0, 0);
    js_def_native(vm, p, "toGMTString", datep_str, 0, 4);
    defs(vm, p, get, datep_get, 0, 0);
    defs(vm, p, uget, datep_get, 0, 0);
    defs(vm, p, set, datep_set, 1, 0);
    defs(vm, p, uset, datep_set, 1, 0);
    return 0;
}

static int lib_coll(js_vm_t *vm, const char *name, int cls, uint32_t proto) {
    static const char *const mm[] = { "get", "set", "has", "delete", "clear", "", "forEach", "keys", "values", "entries", 0 };
    uint32_t c = js_native_new(vm, name, coll_ctor, 0, (uint32_t)cls), sz;
    int i;
    if (!c) return -1;
    js_define(vm, c, vm->s_prototype, jv_obj(proto), 0);
    js_define(vm, proto, vm->s_constructor, jv_obj(c), P_HIDDEN);
    js_define(vm, vm->global, JOBJ(vm, c)->x2, jv_obj(c), P_HIDDEN);
    for (i = 0; mm[i]; i++) {
        if (!mm[i][0]) continue;
        if (cls == JS_CLASS_SET && (i == 0 || i == 1)) continue;
        js_def_native(vm, proto, mm[i], coll_method, i == 1 ? 2 : 1, ((uint32_t)cls << 8) | (uint32_t)i);
    }
    if (cls == JS_CLASS_SET) js_def_native(vm, proto, "add", coll_method, 1, ((uint32_t)cls << 8) | 1u);
    sz = js_native_new(vm, "size", coll_method, 0, ((uint32_t)cls << 8) | 5u);
    if (!sz) return -1;
    js_define_accessor(vm, proto, KEY(vm, "size"), sz, 0, P_CONF);
    return 0;
}

int js_lib2_init(js_vm_t *vm) {
    if (lib_string(vm) || lib_number(vm) || lib_math(vm) || lib_json(vm) || lib_date(vm) ||
        lib_coll(vm, "Map", JS_CLASS_MAP, vm->p_map) || lib_coll(vm, "Set", JS_CLASS_SET, vm->p_set))
        return -1;
    return vm->ctl == CTL_ABORT ? -1 : 0;
}
