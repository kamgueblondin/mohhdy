/* wf_jsu16.c - Lot 115: UTF-16 string semantics over the UTF-8 storage
 * of the guest JavaScript subset.
 *
 * Strings stay UTF-8 bytes in the arena (the page text, the DOM and the
 * console all speak UTF-8), but what a script sees is counted in UTF-16
 * code units like in a browser: length, indexes (s[i], charAt,
 * charCodeAt, slice, substring, substr, indexOf, padStart...), regular
 * expression index and lastIndex. A code point above U+FFFF (a 4-byte
 * sequence) is two units; a lone surrogate (a slice through such a
 * pair, String.fromCharCode(0xD83D)) is kept as a 3-byte WTF-8
 * sequence (ED A0..BF xx) and js_str_cat joins a high and a low half
 * back into the 4-byte form. A byte that does not start a valid
 * sequence counts as one unit whose value is the byte.
 *
 * Cost: the unit count of a string is computed once and cached in the
 * upper bits of js_str_t.interned (JS_U16_*); an ASCII string (the usual
 * case) takes the identity fast path everywhere. Index conversion on
 * other strings scans from the start, or from the last position asked
 * for in the same string (one cached cursor per realm), so a loop over
 * s[i] stays linear. */
#include "wf_jsi.h"

/* the byte length of the sequence at i (1 for an invalid lead or a
 * truncated sequence), its UTF-16 unit count in *units and its first
 * unit (the code point, the high surrogate, or the byte) in *u0 */
static int seq_at(const js_str_t *p, uint32_t i, int *units, uint32_t *u0, uint32_t *u1) {
    uint8_t c = (uint8_t)p->s[i];
    uint32_t n = p->len, cp, k;
    int l;
    *units = 1;
    if (c < 0x80) { *u0 = c; return 1; }
    if (c >= 0xC2 && c <= 0xDF) l = 2;
    else if (c >= 0xE0 && c <= 0xEF) l = 3;
    else if (c >= 0xF0 && c <= 0xF4) l = 4;
    else { *u0 = c; return 1; }
    if (i + (uint32_t)l > n) { *u0 = c; return 1; }
    for (k = 1; k < (uint32_t)l; k++)
        if (((uint8_t)p->s[i + k] & 0xC0) != 0x80) { *u0 = c; return 1; }
    cp = (uint32_t)c & (0x7Fu >> l);
    for (k = 1; k < (uint32_t)l; k++) cp = (cp << 6) | ((uint8_t)p->s[i + k] & 63u);
    if ((l == 3 && cp < 0x800) || (l == 4 && (cp < 0x10000 || cp > 0x10FFFF))) { *u0 = c; return 1; }
    if (l == 4) {
        cp -= 0x10000;
        *units = 2;
        *u0 = 0xD800u + (cp >> 10);
        if (u1) *u1 = 0xDC00u + (cp & 0x3FFu);
        return 4;
    }
    *u0 = cp;
    return l;
}

static uint32_t scan_units(const js_str_t *p, int *ascii) {
    uint32_t i = 0, units = 0;
    *ascii = 1;
    while (i < p->len) {
        int u, l;
        uint32_t a;
        if ((uint8_t)p->s[i] < 0x80) { i++; units++; continue; }
        *ascii = 0;
        l = seq_at(p, i, &u, &a, 0);
        i += (uint32_t)l;
        units += (uint32_t)u;
    }
    return units;
}

static int str_kind(const js_str_t *p) { return (int)(p->interned & JS_STR_KIND_MASK); }

uint32_t js_str_units(js_vm_t *vm, uint32_t r) {
    js_str_t *p = JSTR(vm, r);
    uint32_t in = p->interned, u;
    int ascii;
    if (in & JS_U16_INFO) {
        if (in & JS_U16_ASCII) return p->len;
        u = in >> JS_U16_SHIFT;
        if (u != JS_U16_MAXC) return u;
        return scan_units(p, &ascii);
    }
    u = scan_units(p, &ascii);
    if (str_kind(p) < (int)JS_SYM_PUBLIC) {
        in = (in & JS_STR_KIND_MASK) | JS_U16_INFO;
        if (ascii) in |= JS_U16_ASCII;
        else in |= (u < JS_U16_MAXC ? u : JS_U16_MAXC) << JS_U16_SHIFT;
        JSTR(vm, r)->interned = in;
    }
    return u;
}

int js_str_ascii(js_vm_t *vm, uint32_t r) {
    js_str_t *p = JSTR(vm, r);
    if (!(p->interned & JS_U16_INFO)) (void)js_str_units(vm, r);
    p = JSTR(vm, r);
    if (p->interned & JS_U16_INFO) return (p->interned & JS_U16_ASCII) != 0;
    {
        int ascii;
        (void)scan_units(p, &ascii);
        return ascii;
    }
}

/* the byte offset where unit index `unit` starts; *mid is set when the
 * unit is the low half of a 4-byte sequence (the offset is then that
 * sequence's start). unit >= length gives the byte length. */
int js_u16_off(js_vm_t *vm, uint32_t r, uint32_t unit, int *mid) {
    js_str_t *p;
    uint32_t i = 0, k = 0;
    if (mid) *mid = 0;
    if (js_str_ascii(vm, r)) return unit < JSTR(vm, r)->len ? (int)unit : (int)JSTR(vm, r)->len;
    p = JSTR(vm, r);
    if (vm->u16_str == r && vm->u16_unit <= unit) { i = vm->u16_byte; k = vm->u16_unit; }
    while (i < p->len) {
        int u, l;
        uint32_t a;
        l = seq_at(p, i, &u, &a, 0);
        if (k == unit) break;
        if (u == 2 && k + 1 == unit) {
            if (mid) *mid = 1;
            break;
        }
        k += (uint32_t)u;
        i += (uint32_t)l;
    }
    vm->u16_str = r;
    vm->u16_unit = k;
    vm->u16_byte = i;
    return (int)i;
}

/* the unit index at byte offset off (a byte inside a sequence counts as
 * the sequence's start) */
uint32_t js_u16_unit_of(js_vm_t *vm, uint32_t r, int off) {
    js_str_t *p;
    uint32_t i = 0, k = 0;
    if (off <= 0) return 0;
    if (js_str_ascii(vm, r)) return (uint32_t)off;
    p = JSTR(vm, r);
    if ((uint32_t)off >= p->len) return js_str_units(vm, r);
    if (vm->u16_str == r && vm->u16_byte <= (uint32_t)off) { i = vm->u16_byte; k = vm->u16_unit; }
    while (i < (uint32_t)off && i < p->len) {
        int u, l;
        uint32_t a;
        l = seq_at(p, i, &u, &a, 0);
        if (i + (uint32_t)l > (uint32_t)off) break;
        k += (uint32_t)u;
        i += (uint32_t)l;
    }
    vm->u16_str = r;
    vm->u16_unit = k;
    vm->u16_byte = i;
    return k;
}

/* the code unit at index unit, or -1 past the end */
int js_u16_at(js_vm_t *vm, uint32_t r, uint32_t unit) {
    int mid, off, u;
    uint32_t a, b = 0;
    js_str_t *p;
    if (unit >= js_str_units(vm, r)) return -1;
    off = js_u16_off(vm, r, unit, &mid);
    p = JSTR(vm, r);
    (void)seq_at(p, (uint32_t)off, &u, &a, &b);
    return (int)(mid ? b : a);
}

/* the code point at unit index (a pair gives the full code point, a
 * lone half its own value), or -1 past the end */
int js_u16_cp_at(js_vm_t *vm, uint32_t r, uint32_t unit) {
    int mid, off, u, l;
    uint32_t a, b = 0;
    js_str_t *p;
    if (unit >= js_str_units(vm, r)) return -1;
    off = js_u16_off(vm, r, unit, &mid);
    p = JSTR(vm, r);
    l = seq_at(p, (uint32_t)off, &u, &a, &b);
    if (mid) return (int)b;
    if (u == 2) return (int)(0x10000u + ((a - 0xD800u) << 10) + (b - 0xDC00u));
    (void)l;
    return (int)a;
}

/* a lone surrogate (or any BMP unit) as UTF-8/WTF-8 */
static int put_unit(char *o, uint32_t c) {
    if (c < 0x80) { o[0] = (char)c; return 1; }
    if (c < 0x800) { o[0] = (char)(0xC0 | (c >> 6)); o[1] = (char)(0x80 | (c & 63)); return 2; }
    o[0] = (char)(0xE0 | (c >> 12));
    o[1] = (char)(0x80 | ((c >> 6) & 63));
    o[2] = (char)(0x80 | (c & 63));
    return 3;
}

/* the units [from, to) as a new string; a cut through a pair keeps the
 * half on its side as a lone surrogate */
uint32_t js_u16_sub(js_vm_t *vm, uint32_t r, uint32_t from, uint32_t to) {
    uint32_t n = js_str_units(vm, r), res, len;
    int a, b, ma, mb, ua, ub;
    uint32_t a0, a1 = 0, b0, b1 = 0;
    char head[3], tail[3];
    int hn = 0, tn = 0, i;
    if (to > n) to = n;
    if (from >= to) return vm->s_empty;
    if (js_str_ascii(vm, r)) return js_str_sub(vm, r, (int)from, (int)to);
    b = js_u16_off(vm, r, to, &mb);
    if (mb) {                                   /* to cuts a pair: keep its high half */
        (void)seq_at(JSTR(vm, r), (uint32_t)b, &ub, &b0, &b1);
        tn = put_unit(tail, b0);
    }
    a = js_u16_off(vm, r, from, &ma);
    if (ma) {                                   /* from cuts a pair: start with its low half */
        (void)seq_at(JSTR(vm, r), (uint32_t)a, &ua, &a0, &a1);
        hn = put_unit(head, a1);
        a += 4;
    }
    if (!hn && !tn) return js_str_sub(vm, r, a, b);
    len = (uint32_t)(hn + (b - a) + tn);        /* from < to: a <= b even after a cut */
    res = js_alloc(vm, 12u + len + 1u);
    if (!res) return vm->s_empty;
    {
        js_str_t *q = JSTR(vm, res), *p = JSTR(vm, r);
        uint32_t k = 0;
        for (i = 0; i < hn; i++) q->s[k++] = head[i];
        for (i = a; i < b; i++) q->s[k++] = p->s[i];
        for (i = 0; i < tn; i++) q->s[k++] = tail[i];
        q->s[k] = 0;
        q->len = k;
    }
    return res;
}

/* the byte length of the sequence at byte i (an empty regular expression
 * match steps over a whole sequence, not one byte of it) */
int js_u8_step(js_vm_t *vm, uint32_t r, int i) {
    int u;
    uint32_t a;
    if (i < 0 || (uint32_t)i >= JSTR(vm, r)->len) return 1;
    return seq_at(JSTR(vm, r), (uint32_t)i, &u, &a, 0);
}

/* 1 when the bytes at s are a WTF-8 high (want 0) or low (want 1) surrogate */
static int is_half(const char *s, int want) {
    uint8_t c0 = (uint8_t)s[0], c1 = (uint8_t)s[1], c2 = (uint8_t)s[2];
    if (c0 != 0xED || (c2 & 0xC0) != 0x80) return 0;
    return want ? (c1 >= 0xB0 && c1 <= 0xBF) : (c1 >= 0xA0 && c1 <= 0xAF);
}

/* js_str_cat helper: when a ends with a high half and b starts with a
 * low half, the 4-byte form of the pair; 0 otherwise */
uint32_t js_u16_join(js_vm_t *vm, uint32_t a, uint32_t b) {
    js_str_t *pa = JSTR(vm, a), *pb = JSTR(vm, b);
    uint32_t hi, lo, cp, la = pa->len, lb = pb->len, r, i;
    if (la < 3 || lb < 3) return 0;
    if (!is_half(pa->s + la - 3, 0) || !is_half(pb->s, 1)) return 0;
    hi = (((uint32_t)(uint8_t)pa->s[la - 2] & 63u) << 6) | ((uint8_t)pa->s[la - 1] & 63u);
    hi |= 0xD000u;
    lo = (((uint32_t)(uint8_t)pb->s[1] & 63u) << 6) | ((uint8_t)pb->s[2] & 63u);
    lo |= 0xD000u;
    cp = 0x10000u + ((hi - 0xD800u) << 10) + (lo - 0xDC00u);
    r = js_alloc(vm, 12u + (la - 3) + 4 + (lb - 3) + 1);
    if (!r) return vm->s_empty;
    {
        js_str_t *q = JSTR(vm, r);
        uint32_t k = 0;
        pa = JSTR(vm, a); pb = JSTR(vm, b);
        for (i = 0; i < la - 3; i++) q->s[k++] = pa->s[i];
        q->s[k++] = (char)(0xF0 | (cp >> 18));
        q->s[k++] = (char)(0x80 | ((cp >> 12) & 63));
        q->s[k++] = (char)(0x80 | ((cp >> 6) & 63));
        q->s[k++] = (char)(0x80 | (cp & 63));
        for (i = 3; i < lb; i++) q->s[k++] = pb->s[i];
        q->s[k] = 0;
        q->len = k;
    }
    return r;
}

/* String.fromCharCode: units to UTF-8, a high then a low unit as one
 * 4-byte sequence, a lone half as WTF-8 */
void js_u16_put_units(js_vm_t *vm, js_sb_t *b, const uint32_t *u, int n) {
    int i;
    for (i = 0; i < n && vm->ctl != CTL_ABORT; i++) {
        uint32_t c = u[i] & 0xFFFFu;
        char o[4];
        if (c >= 0xD800 && c <= 0xDBFF && i + 1 < n && (u[i + 1] & 0xFFFFu) >= 0xDC00 && (u[i + 1] & 0xFFFFu) <= 0xDFFF) {
            uint32_t cp = 0x10000u + ((c - 0xD800u) << 10) + ((u[i + 1] & 0xFFFFu) - 0xDC00u);
            o[0] = (char)(0xF0 | (cp >> 18));
            o[1] = (char)(0x80 | ((cp >> 12) & 63));
            o[2] = (char)(0x80 | ((cp >> 6) & 63));
            o[3] = (char)(0x80 | (cp & 63));
            js_sb_put(vm, b, o, 4);
            i++;
            continue;
        }
        js_sb_put(vm, b, o, put_unit(o, c));
    }
}

/* JSON.stringify and the console: the unit value of a lone surrogate
 * at byte i (0 when the bytes there are not one) */
uint32_t js_u16_lone_at(const char *s, int left) {
    if (left < 3) return 0;
    if (!is_half(s, 0) && !is_half(s, 1)) return 0;
    return 0xD000u | ((((uint32_t)(uint8_t)s[1]) & 63u) << 6) | ((uint8_t)s[2] & 63u);
}
