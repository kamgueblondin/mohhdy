/* wf_jsrt.c - Lot 113: values, the arena, strings, objects, properties,
 * conversions and errors of the JavaScript subset (see wf_js.h). */
#include "wf_jsi.h"

jv jv_undef(void) { jv v; v.t = JT_UNDEF; v.f = 0; v.pad = 0; v.a = 0; v.b = 0; return v; }
jv jv_null(void) { jv v = jv_undef(); v.t = JT_NULL; return v; }
jv jv_bool(int b) { jv v = jv_undef(); v.t = JT_BOOL; v.a = b ? 1u : 0u; return v; }
jv jv_int(int32_t i) { jv v = jv_undef(); v.t = JT_INT; v.a = (uint32_t)i; return v; }
jv jv_str(uint32_t r) { jv v = jv_undef(); v.t = JT_STR; v.a = r; return v; }
jv jv_obj(uint32_t r) { jv v = jv_undef(); v.t = JT_OBJ; v.a = r; return v; }
jv jv_empty(void) { jv v = jv_undef(); v.t = JT_EMPTY; return v; }

jv jv_num(jf64 n) {
    jv v = jv_undef();
    int32_t i;
    if (jf_is_i32(n, &i)) { v.t = JT_INT; v.a = (uint32_t)i; return v; }
    v.t = JT_NUM;
    v.a = (uint32_t)n;
    v.b = (uint32_t)(n >> 32);
    return v;
}

jf64 jv_f64(jv v) {
    if (v.t == JT_INT) return jf_from_i32((int32_t)v.a);
    return ((jf64)v.b << 32) | v.a;
}

int jv_is_num(jv v) { return v.t == JT_INT || v.t == JT_NUM; }
int jv_is_obj(jv v) { return v.t == JT_OBJ; }
int jv_is_nullish(jv v) { return v.t == JT_UNDEF || v.t == JT_NULL; }

static int cslen(const char *s) { int n = 0; while (s[n]) n++; return n; }

static void cscpy(char *d, int max, const char *s) {
    int i = 0;
    if (max <= 0) return;
    while (s[i] && i < max - 1) { d[i] = s[i]; i++; }
    d[i] = 0;
}

void js_abort(js_vm_t *vm, const char *msg) {
    if (vm->ctl != CTL_ABORT) cscpy(vm->err, (int)sizeof(vm->err), msg);
    vm->ctl = CTL_ABORT;
}

uint32_t js_alloc(js_vm_t *vm, uint32_t n) {
    uint32_t r, i;
    n = (n + 3u) & ~3u;
    if (n > vm->limit || vm->top > vm->limit - n) {
        js_abort(vm, "RangeError: out of memory (the realm's arena is full)");
        return 0;
    }
    r = vm->top;
    vm->top += n;
    for (i = 0; i < n; i++) vm->mem[r + i] = 0;
    return r;
}

/* ---- strings ---- */

static uint32_t hash_bytes(const char *s, int len) {
    uint32_t h = 2166136261u;
    int i;
    for (i = 0; i < len; i++) { h ^= (uint8_t)s[i]; h *= 16777619u; }
    return h ? h : 1u;
}

uint32_t js_str_new(js_vm_t *vm, const char *s, int len) {
    uint32_t r;
    js_str_t *p;
    int i;
    if (len < 0) len = 0;
    r = js_alloc(vm, (uint32_t)(12 + len + 1));
    if (!r) return vm->s_empty;
    p = JSTR(vm, r);
    p->len = (uint32_t)len;
    for (i = 0; i < len; i++) p->s[i] = s[i];
    p->s[len] = 0;
    return r;
}

uint32_t js_str_c(js_vm_t *vm, const char *s) { return js_str_new(vm, s, cslen(s)); }

uint32_t js_str_cat(js_vm_t *vm, uint32_t a, uint32_t b) {
    js_str_t *pa = JSTR(vm, a), *pb = JSTR(vm, b), *p;
    uint32_t r, i, la, lb;
    if (pa->len == 0) return b;
    if (pb->len == 0) return a;
    la = pa->len; lb = pb->len;
    r = js_alloc(vm, 12 + la + lb + 1);
    if (!r) return vm->s_empty;
    p = JSTR(vm, r);
    pa = JSTR(vm, a); pb = JSTR(vm, b);
    p->len = la + lb;
    for (i = 0; i < la; i++) p->s[i] = pa->s[i];
    for (i = 0; i < lb; i++) p->s[la + i] = pb->s[i];
    p->s[la + lb] = 0;
    return r;
}

static uint32_t str_hash(js_vm_t *vm, uint32_t r) {
    js_str_t *p = JSTR(vm, r);
    if (!p->hash) p->hash = hash_bytes(p->s, (int)p->len);
    return p->hash;
}

static int bytes_eq(const char *a, const char *b, int n) {
    int i;
    for (i = 0; i < n; i++) if (a[i] != b[i]) return 0;
    return 1;
}

static uint32_t intern_lookup(js_vm_t *vm, const char *s, int len, uint32_t h, int insert, uint32_t existing) {
    uint32_t *tab = (uint32_t *)JPTR(vm, vm->itab), i, mask = JS_ITAB - 1, n;
    for (n = 0, i = h & mask; n < JS_ITAB; n++, i = (i + 1) & mask) {
        uint32_t r = tab[i];
        if (!r) break;
        {
            js_str_t *p = JSTR(vm, r);
            if (p->hash == h && p->len == (uint32_t)len && bytes_eq(p->s, s, len)) return r;
        }
    }
    if (!insert) return 0;
    if (vm->icount >= JS_ITAB - JS_ITAB / 8) {
        js_abort(vm, "RangeError: too many distinct property names");
        return vm->s_empty ? vm->s_empty : 0;
    }
    if (!existing) {
        existing = js_str_new(vm, s, len);
        if (vm->ctl == CTL_ABORT) return vm->s_empty;
    }
    JSTR(vm, existing)->hash = h;
    JSTR(vm, existing)->interned = 1;
    tab = (uint32_t *)JPTR(vm, vm->itab);
    tab[i] = existing;
    vm->icount++;
    return existing;
}

uint32_t js_intern(js_vm_t *vm, const char *s, int len) {
    return intern_lookup(vm, s, len, hash_bytes(s, len), 1, 0);
}

uint32_t js_intern_find(js_vm_t *vm, const char *s, int len) {
    return intern_lookup(vm, s, len, hash_bytes(s, len), 0, 0);
}

uint32_t js_intern_str(js_vm_t *vm, uint32_t r) {
    js_str_t *p = JSTR(vm, r);
    if (p->interned) return r;
    return intern_lookup(vm, p->s, (int)p->len, str_hash(vm, r), 1, r);
}

int js_str_eq(js_vm_t *vm, uint32_t a, uint32_t b) {
    js_str_t *pa, *pb;
    if (a == b) return 1;
    pa = JSTR(vm, a); pb = JSTR(vm, b);
    if (pa->interned && pb->interned) return 0;
    return pa->len == pb->len && bytes_eq(pa->s, pb->s, (int)pa->len);
}

int js_str_cmp(js_vm_t *vm, uint32_t a, uint32_t b) {
    js_str_t *pa = JSTR(vm, a), *pb = JSTR(vm, b);
    uint32_t i, n = pa->len < pb->len ? pa->len : pb->len;
    for (i = 0; i < n; i++) {
        uint8_t x = (uint8_t)pa->s[i], y = (uint8_t)pb->s[i];
        if (x != y) return x < y ? -1 : 1;
    }
    if (pa->len == pb->len) return 0;
    return pa->len < pb->len ? -1 : 1;
}

uint32_t js_str_sub(js_vm_t *vm, uint32_t r, int from, int to) {
    js_str_t *p = JSTR(vm, r);
    if (from < 0) from = 0;
    if (to > (int)p->len) to = (int)p->len;
    if (from == 0 && to == (int)p->len) return r;
    if (to <= from) return vm->s_empty;
    {
        uint32_t n = js_alloc(vm, (uint32_t)(12 + (to - from) + 1));
        js_str_t *q;
        int i;
        if (!n) return vm->s_empty;
        p = JSTR(vm, r);
        q = JSTR(vm, n);
        q->len = (uint32_t)(to - from);
        for (i = from; i < to; i++) q->s[i - from] = p->s[i];
        q->s[to - from] = 0;
        return n;
    }
}

int js_str_index_key(js_vm_t *vm, uint32_t key, uint32_t *idx) {
    js_str_t *p = JSTR(vm, key);
    uint32_t v = 0, i;
    if (p->len == 0 || p->len > 10) return 0;
    if (p->s[0] == '0' && p->len > 1) return 0;
    for (i = 0; i < p->len; i++) {
        uint32_t d;
        if (p->s[i] < '0' || p->s[i] > '9') return 0;
        d = (uint32_t)(p->s[i] - '0');
        if (v > (0xFFFFFFFEu - d) / 10u) return 0;
        v = v * 10u + d;
    }
    *idx = v;
    return 1;
}

static int u32_dec(uint32_t v, char *b) {
    char t[12];
    int n = 0, i;
    do { t[n++] = (char)('0' + v % 10u); v /= 10u; } while (v);
    for (i = 0; i < n; i++) b[i] = t[n - 1 - i];
    b[n] = 0;
    return n;
}

uint32_t js_key_of_index(js_vm_t *vm, uint32_t idx) {
    char b[12];
    int n = u32_dec(idx, b);
    return js_intern(vm, b, n);
}

/* ---- objects ---- */

uint32_t js_obj_new(js_vm_t *vm, int cls, uint32_t proto) {
    uint32_t r = js_alloc(vm, sizeof(js_obj_t));
    js_obj_t *o;
    if (!r) return 0;
    o = JOBJ(vm, r);
    o->cls = (uint8_t)cls;
    o->oflags = O_EXT;
    o->proto = proto;
    return r;
}

uint32_t js_array_new(js_vm_t *vm, uint32_t cap) {
    uint32_t r = js_obj_new(vm, JS_CLASS_ARRAY, vm->p_array);
    if (!r) return 0;
    if (cap) {
        uint32_t a = js_alloc(vm, cap * (uint32_t)sizeof(jv));
        if (!a) return r;
        JOBJ(vm, r)->arr = a;
        JOBJ(vm, r)->acap = cap;
    }
    return r;
}

#define JS_ARRAY_MAX (1u << 20)

static int array_reserve(js_vm_t *vm, uint32_t arr, uint32_t need) {
    js_obj_t *o = JOBJ(vm, arr);
    uint32_t cap, a, i;
    jv *src, *dst;
    if (need <= o->acap) return 1;
    if (need > JS_ARRAY_MAX) {
        js_throw_err(vm, JE_RANGE, "array too long for the guest (1048576 elements)");
        return 0;
    }
    cap = o->acap ? o->acap * 2 : 4;
    while (cap < need) cap *= 2;
    a = js_alloc(vm, cap * (uint32_t)sizeof(jv));
    if (!a) return 0;
    o = JOBJ(vm, arr);
    src = (jv *)JPTR(vm, o->arr);
    dst = (jv *)JPTR(vm, a);
    for (i = 0; i < o->alen; i++) dst[i] = src[i];
    for (; i < cap; i++) dst[i] = jv_empty();
    o->arr = a;
    o->acap = cap;
    return 1;
}

int js_array_set_len(js_vm_t *vm, uint32_t arr, uint32_t len) {
    js_obj_t *o = JOBJ(vm, arr);
    uint32_t i;
    if (len > o->alen) {
        if (!array_reserve(vm, arr, len)) return 0;
        o = JOBJ(vm, arr);
        for (i = o->alen; i < len; i++) ((jv *)JPTR(vm, o->arr))[i] = jv_empty();
    }
    o->alen = len;
    return 1;
}

int js_array_set(js_vm_t *vm, uint32_t arr, uint32_t idx, jv v) {
    js_obj_t *o = JOBJ(vm, arr);
    if (idx >= o->alen) {
        if (idx - o->alen > 65536u && idx > 1024u) {
            js_throw_err(vm, JE_RANGE, "sparse array write too far past the end (guest bound)");
            return 0;
        }
        if (!js_array_set_len(vm, arr, idx + 1)) return 0;
        o = JOBJ(vm, arr);
    }
    ((jv *)JPTR(vm, o->arr))[idx] = v;
    return 1;
}

int js_array_push(js_vm_t *vm, uint32_t arr, jv v) {
    return js_array_set(vm, arr, JOBJ(vm, arr)->alen, v);
}

jv js_array_get(js_vm_t *vm, uint32_t arr, uint32_t idx) {
    js_obj_t *o = JOBJ(vm, arr);
    jv v;
    if (idx >= o->alen) return jv_undef();
    v = ((jv *)JPTR(vm, o->arr))[idx];
    return v.t == JT_EMPTY ? jv_undef() : v;
}

js_prop_t *js_own_prop(js_vm_t *vm, uint32_t obj, uint32_t key) {
    js_obj_t *o = JOBJ(vm, obj);
    js_prop_t *p;
    uint32_t i;
    if (!o->props) return 0;
    p = (js_prop_t *)JPTR(vm, o->props);
    for (i = 0; i < o->nprop; i++) if (p[i].key == key) return &p[i];
    return 0;
}

int js_define(js_vm_t *vm, uint32_t obj, uint32_t key, jv v, uint32_t flags) {
    js_obj_t *o;
    js_prop_t *p = js_own_prop(vm, obj, key);
    if (p) { p->v = v; p->flags = flags; return 1; }
    o = JOBJ(vm, obj);
    if (o->nprop >= o->pcap) {
        uint32_t cap = o->pcap ? (uint32_t)o->pcap * 2u : 4u, r, i;
        js_prop_t *src, *dst;
        if (cap > 60000u) { js_throw_err(vm, JE_RANGE, "too many properties on one object"); return 0; }
        r = js_alloc(vm, cap * (uint32_t)sizeof(js_prop_t));
        if (!r) return 0;
        o = JOBJ(vm, obj);
        src = (js_prop_t *)JPTR(vm, o->props);
        dst = (js_prop_t *)JPTR(vm, r);
        for (i = 0; i < o->nprop; i++) dst[i] = src[i];
        o->props = r;
        o->pcap = (uint16_t)cap;
    }
    p = (js_prop_t *)JPTR(vm, o->props) + o->nprop;
    p->key = key;
    p->flags = flags;
    p->v = v;
    o->nprop++;
    return 1;
}

int js_define_c(js_vm_t *vm, uint32_t obj, const char *name, jv v, uint32_t flags) {
    uint32_t k = js_intern(vm, name, cslen(name));
    if (vm->ctl == CTL_ABORT) return 0;
    return js_define(vm, obj, k, v, flags);
}

int js_define_accessor(js_vm_t *vm, uint32_t obj, uint32_t key, uint32_t getter, uint32_t setter, uint32_t flags) {
    js_prop_t *p = js_own_prop(vm, obj, key);
    jv v = jv_undef();
    if (p && (p->flags & P_ACCESSOR)) {
        if (getter) p->v.a = getter;
        if (setter) p->v.b = setter;
        return 1;
    }
    v.t = JT_OBJ;
    v.a = getter;
    v.b = setter;
    return js_define(vm, obj, key, v, flags | P_ACCESSOR);
}

int js_delete_own(js_vm_t *vm, uint32_t obj, uint32_t key) {
    js_obj_t *o = JOBJ(vm, obj);
    js_prop_t *p;
    uint32_t i, j;
    if (!o->props) return 1;
    p = (js_prop_t *)JPTR(vm, o->props);
    for (i = 0; i < o->nprop; i++) {
        if (p[i].key != key) continue;
        if (!(p[i].flags & P_CONF) || (o->oflags & O_FROZEN)) return 0;
        for (j = i + 1; j < o->nprop; j++) p[j - 1] = p[j];
        o->nprop--;
        return 1;
    }
    return 1;
}

uint32_t js_native_new(js_vm_t *vm, const char *name, js_native_fn fn, int nargs, uint32_t magic) {
    int i, idx = -1;
    uint32_t r;
    js_obj_t *o;
    for (i = 0; i < vm->nnat; i++) if (vm->nat[i] == fn) { idx = i; break; }
    if (idx < 0) {
        if (vm->nnat >= JS_MAX_NATIVES) { js_abort(vm, "internal: native table full"); return 0; }
        idx = vm->nnat++;
        vm->nat[idx] = fn;
    }
    r = js_obj_new(vm, JS_CLASS_NATIVE, vm->p_function);
    if (!r) return 0;
    o = JOBJ(vm, r);
    o->x0 = (uint32_t)idx;
    o->x1 = magic;
    o->pad = (uint16_t)nargs;
    o->x2 = js_intern(vm, name, cslen(name));
    return r;
}

uint32_t js_def_native(js_vm_t *vm, uint32_t obj, const char *name, js_native_fn fn, int nargs, uint32_t magic) {
    uint32_t f = js_native_new(vm, name, fn, nargs, magic);
    if (!f) return 0;
    js_define(vm, obj, JOBJ(vm, f)->x2, jv_obj(f), P_HIDDEN);
    return f;
}

int js_is_callable(js_vm_t *vm, jv v) {
    int c;
    if (v.t != JT_OBJ) return 0;
    c = JOBJ(vm, v.a)->cls;
    return c == JS_CLASS_FUNC || c == JS_CLASS_NATIVE || c == JS_CLASS_BOUND;
}

int js_is_array(js_vm_t *vm, jv v) {
    return v.t == JT_OBJ && JOBJ(vm, v.a)->cls == JS_CLASS_ARRAY;
}

/* ---- errors ---- */

uint32_t js_error_new(js_vm_t *vm, int kind, uint32_t msg) {
    uint32_t e = js_obj_new(vm, JS_CLASS_ERROR, vm->p_err[kind]);
    if (!e) return 0;
    if (msg) js_define(vm, e, vm->s_message, jv_str(msg), P_HIDDEN);
    return e;
}

jv js_throw(js_vm_t *vm, jv v) {
    if (vm->ctl == CTL_ABORT) return jv_undef();
    vm->exc = v;
    vm->ctl = CTL_THROW;
    return jv_undef();
}

jv js_throw_err(js_vm_t *vm, int kind, const char *msg) {
    uint32_t m, e;
    if (vm->ctl == CTL_ABORT) return jv_undef();
    m = js_str_c(vm, msg);
    e = js_error_new(vm, kind, m);
    if (!e) return jv_undef();
    return js_throw(vm, jv_obj(e));
}

/* msg with the string s appended (a name in a TypeError) */
jv js_throw_err2(js_vm_t *vm, int kind, const char *msg, uint32_t s) {
    uint32_t m, e;
    if (vm->ctl == CTL_ABORT) return jv_undef();
    m = js_str_c(vm, msg);
    if (s) m = js_str_cat(vm, m, s);
    e = js_error_new(vm, kind, m);
    if (!e) return jv_undef();
    return js_throw(vm, jv_obj(e));
}

/* ---- conversions ---- */

int js_truthy(js_vm_t *vm, jv v) {
    switch (v.t) {
    case JT_BOOL: return v.a != 0;
    case JT_INT: return v.a != 0;
    case JT_NUM: { jf64 n = jv_f64(v); return !jf_isnan(n) && !jf_iszero(n); }
    case JT_STR: return JSTR(vm, v.a)->len != 0;
    case JT_OBJ: return 1;
    default: return 0;
    }
}

static int is_ws(uint8_t c) {
    return c == ' ' || c == '\t' || c == '\n' || c == '\r' || c == 0x0B || c == 0x0C || c == 0xA0;
}

jf64 js_str_to_num(js_vm_t *vm, uint32_t r) {
    js_str_t *p = JSTR(vm, r);
    int i = 0, n = (int)p->len, used;
    jf64 v;
    while (i < n && is_ws((uint8_t)p->s[i])) i++;
    while (n > i && is_ws((uint8_t)p->s[n - 1])) n--;
    if (i == n) return 0;
    if (n - i > 2 && p->s[i] == '0') {
        char x = p->s[i + 1];
        int base = (x == 'x' || x == 'X') ? 16 : (x == 'o' || x == 'O') ? 8 : (x == 'b' || x == 'B') ? 2 : 0;
        if (base) {
            jf64 acc = 0, b = jf_from_i32(base);
            int k;
            for (k = i + 2; k < n; k++) {
                char c = p->s[k];
                int d = (c >= '0' && c <= '9') ? c - '0' : (c >= 'a' && c <= 'z') ? c - 'a' + 10 :
                        (c >= 'A' && c <= 'Z') ? c - 'A' + 10 : 99;
                if (d >= base) return JF_NAN;
                acc = jf_add(jf_mul(acc, b), jf_from_i32(d));
            }
            return acc;
        }
    }
    v = jf_parse(p->s + i, n - i, &used);
    if (used != n - i) return JF_NAN;
    return v;
}

jf64 js_to_number(js_vm_t *vm, jv v) {
    switch (v.t) {
    case JT_UNDEF: return JF_NAN;
    case JT_NULL: return 0;
    case JT_BOOL: return v.a ? JF_ONE : 0;
    case JT_INT: case JT_NUM: return jv_f64(v);
    case JT_STR: return js_str_to_num(vm, v.a);
    case JT_OBJ: {
        jv p = js_to_primitive(vm, v, 0);
        if (vm->ctl) return JF_NAN;
        return js_to_number(vm, p);
    }
    default: return JF_NAN;
    }
}

jv js_to_numeric(js_vm_t *vm, jv v) {
    if (v.t == JT_INT || v.t == JT_NUM) return v;
    return jv_num(js_to_number(vm, v));
}

int32_t js_to_int32(js_vm_t *vm, jv v) {
    if (v.t == JT_INT) return (int32_t)v.a;
    return jf_to_i32(js_to_number(vm, v));
}

uint32_t js_to_uint32(js_vm_t *vm, jv v) {
    if (v.t == JT_INT) return v.a;
    return jf_to_u32(js_to_number(vm, v));
}

jf64 js_to_integer(js_vm_t *vm, jv v) {
    jf64 n;
    if (v.t == JT_INT) return jf_from_i32((int32_t)v.a);
    n = js_to_number(vm, v);
    if (jf_isnan(n)) return 0;
    if (jf_isinf(n)) return n;
    n = jf_trunc(n);
    return jf_iszero(n) ? 0 : n;
}

uint32_t js_num_to_str(js_vm_t *vm, jf64 n) {
    char b[40];
    int len = jf_format(n, b, (int)sizeof(b));
    return js_str_new(vm, b, len);
}

uint32_t js_to_string(js_vm_t *vm, jv v) {
    char b[16];
    switch (v.t) {
    case JT_UNDEF: return vm->s_undefined;
    case JT_NULL: return js_intern(vm, "null", 4);
    case JT_BOOL: return v.a ? js_intern(vm, "true", 4) : js_intern(vm, "false", 5);
    case JT_INT: {
        int32_t i = (int32_t)v.a;
        int n = 0;
        uint32_t u = i < 0 ? 0u - (uint32_t)i : (uint32_t)i;
        if (i < 0) b[n++] = '-';
        n += u32_dec(u, b + n);
        return js_str_new(vm, b, n);
    }
    case JT_NUM: return js_num_to_str(vm, jv_f64(v));
    case JT_STR: return v.a;
    case JT_OBJ: {
        jv p = js_to_primitive(vm, v, 1);
        if (vm->ctl) return vm->s_empty;
        return js_to_string(vm, p);
    }
    default: return vm->s_empty;
    }
}

uint32_t js_to_key(js_vm_t *vm, jv v) {
    uint32_t s;
    if (v.t == JT_INT && (int32_t)v.a >= 0) return js_key_of_index(vm, v.a);
    s = js_to_string(vm, v);
    if (vm->ctl) return vm->s_empty;
    return js_intern_str(vm, s);
}

jv js_to_primitive(js_vm_t *vm, jv v, int hint_string) {
    uint32_t names[2];
    int i;
    if (v.t != JT_OBJ) return v;
    if (JOBJ(vm, v.a)->cls == JS_CLASS_DATE && hint_string != 0) hint_string = 1;
    names[0] = hint_string ? vm->s_toString : vm->s_valueOf;
    names[1] = hint_string ? vm->s_valueOf : vm->s_toString;
    for (i = 0; i < 2; i++) {
        jv f = js_get(vm, v, names[i]);
        if (vm->ctl) return jv_undef();
        if (js_is_callable(vm, f)) {
            jv r = js_call(vm, f, v, 0, 0);
            if (vm->ctl) return jv_undef();
            if (r.t != JT_OBJ) return r;
        }
    }
    js_throw_err(vm, JE_TYPE, "Cannot convert object to primitive value");
    return jv_undef();
}

uint32_t js_to_object(js_vm_t *vm, jv v) {
    uint32_t o;
    switch (v.t) {
    case JT_OBJ: return v.a;
    case JT_UNDEF: case JT_NULL:
        js_throw_err(vm, JE_TYPE, "Cannot convert undefined or null to object");
        return 0;
    case JT_BOOL: o = js_obj_new(vm, JS_CLASS_BOOLEAN, vm->p_boolean); break;
    case JT_STR: o = js_obj_new(vm, JS_CLASS_STRING, vm->p_string); break;
    default: o = js_obj_new(vm, JS_CLASS_NUMBER, vm->p_number); break;
    }
    if (!o) return 0;
    JOBJ(vm, o)->x0 = v.t;
    JOBJ(vm, o)->x1 = v.a;
    JOBJ(vm, o)->x2 = v.b;
    return o;
}

int js_strict_eq(js_vm_t *vm, jv a, jv b) {
    if (jv_is_num(a) && jv_is_num(b)) {
        if (a.t == JT_INT && b.t == JT_INT) return a.a == b.a;
        return jf_eq(jv_f64(a), jv_f64(b));
    }
    if (a.t != b.t) return 0;
    switch (a.t) {
    case JT_UNDEF: case JT_NULL: return 1;
    case JT_BOOL: return a.a == b.a;
    case JT_STR: return js_str_eq(vm, a.a, b.a);
    case JT_OBJ: return a.a == b.a;
    default: return 0;
    }
}

int js_same_value_zero(js_vm_t *vm, jv a, jv b) {
    if (jv_is_num(a) && jv_is_num(b)) {
        jf64 x = jv_f64(a), y = jv_f64(b);
        if (jf_isnan(x) && jf_isnan(y)) return 1;
        return jf_eq(x, y);
    }
    return js_strict_eq(vm, a, b);
}

int js_loose_eq(js_vm_t *vm, jv a, jv b) {
    for (;;) {
        if (a.t == b.t || (jv_is_num(a) && jv_is_num(b))) return js_strict_eq(vm, a, b);
        if (jv_is_nullish(a) && jv_is_nullish(b)) return 1;
        if (jv_is_nullish(a) || jv_is_nullish(b)) return 0;
        if (jv_is_num(a) && b.t == JT_STR) { b = jv_num(js_str_to_num(vm, b.a)); continue; }
        if (a.t == JT_STR && jv_is_num(b)) { a = jv_num(js_str_to_num(vm, a.a)); continue; }
        if (a.t == JT_BOOL) { a = jv_int((int32_t)a.a); continue; }
        if (b.t == JT_BOOL) { b = jv_int((int32_t)b.a); continue; }
        if (a.t == JT_OBJ && b.t != JT_OBJ) { a = js_to_primitive(vm, a, 0); if (vm->ctl) return 0; continue; }
        if (b.t == JT_OBJ && a.t != JT_OBJ) { b = js_to_primitive(vm, b, 0); if (vm->ctl) return 0; continue; }
        return 0;
    }
}

uint32_t js_typeof(js_vm_t *vm, jv v) {
    const char *s;
    switch (v.t) {
    case JT_UNDEF: case JT_EMPTY: s = "undefined"; break;
    case JT_NULL: s = "object"; break;
    case JT_BOOL: s = "boolean"; break;
    case JT_INT: case JT_NUM: s = "number"; break;
    case JT_STR: s = "string"; break;
    default: s = js_is_callable(vm, v) ? "function" : "object"; break;
    }
    return js_intern(vm, s, cslen(s));
}

/* ---- property access ---- */

static uint32_t fn_name_of(js_vm_t *vm, uint32_t f) {
    js_obj_t *o = JOBJ(vm, f);
    if (o->cls == JS_CLASS_NATIVE) return o->x2 ? o->x2 : vm->s_empty;
    if (o->cls == JS_CLASS_FUNC) {
        js_node_t *n = JNODE(vm, o->x0);
        if (o->x2) return o->x2;
        return n->a ? n->a : vm->s_empty;
    }
    return vm->s_empty;
}

static int fn_length_of(js_vm_t *vm, uint32_t f) {
    js_obj_t *o = JOBJ(vm, f);
    if (o->cls == JS_CLASS_NATIVE) return o->pad;
    if (o->cls == JS_CLASS_FUNC) {
        js_node_t *n = JNODE(vm, o->x0);
        uint32_t p = n->b;
        int c = 0;
        while (p) {
            js_node_t *pn = JNODE(vm, p);
            if (pn->b || (pn->fl & F_REST)) break;
            c++;
            p = pn->next;
        }
        return c;
    }
    return 0;
}

/* a function's own prototype object, made on first use */
static jv fn_prototype(js_vm_t *vm, uint32_t f) {
    js_obj_t *o = JOBJ(vm, f);
    js_node_t *n;
    uint32_t p;
    if (o->cls != JS_CLASS_FUNC) return jv_undef();
    n = JNODE(vm, o->x0);
    if (n->fl & (F_ARROW | F_METHOD)) return jv_undef();
    p = js_obj_new(vm, JS_CLASS_OBJECT, vm->p_object);
    if (!p) return jv_undef();
    js_define(vm, p, vm->s_constructor, jv_obj(f), P_HIDDEN);
    js_define(vm, f, vm->s_prototype, jv_obj(p), P_WRITE);
    return jv_obj(p);
}

static jv string_own(js_vm_t *vm, uint32_t s, uint32_t key, int *found) {
    js_str_t *p = JSTR(vm, s);
    uint32_t idx;
    *found = 1;
    if (key == vm->s_length) return jv_int((int32_t)p->len);
    if (js_str_index_key(vm, key, &idx) && idx < p->len) return jv_str(js_str_sub(vm, s, (int)idx, (int)idx + 1));
    *found = 0;
    return jv_undef();
}

jv js_get(js_vm_t *vm, jv objv, uint32_t key) {
    uint32_t o;
    int depth = 0, found;
    switch (objv.t) {
    case JT_STR: {
        jv r = string_own(vm, objv.a, key, &found);
        if (found) return r;
        o = vm->p_string;
        break;
    }
    case JT_INT: case JT_NUM: o = vm->p_number; break;
    case JT_BOOL: o = vm->p_boolean; break;
    case JT_OBJ: o = objv.a; break;
    default:
        return js_throw_err2(vm, JE_TYPE, objv.t == JT_NULL ? "Cannot read properties of null (reading '" :
                                          "Cannot read properties of undefined (reading '", js_str_cat(vm, key, js_str_c(vm, "')")));
    }
    while (o && depth++ < 256) {
        js_obj_t *ob = JOBJ(vm, o);
        js_prop_t *p;
        uint32_t idx;
        switch (ob->cls) {
        case JS_CLASS_HOST:
            if (vm->host_get) {
                jv out;
                if (vm->host_get(vm, jv_obj(o), key, &out)) return out;
                if (vm->ctl) return jv_undef();
            }
            break;
        case JS_CLASS_ARRAY: case JS_CLASS_ARGS:
            if (key == vm->s_length) return jv_int((int32_t)ob->alen);
            if (js_str_index_key(vm, key, &idx)) {
                if (idx < ob->alen) {
                    jv v = ((jv *)JPTR(vm, ob->arr))[idx];
                    if (v.t != JT_EMPTY) return v;
                }
            }
            break;
        case JS_CLASS_STRING:
            if (ob->x0 == JT_STR) {
                jv r = string_own(vm, ob->x1, key, &found);
                if (found) return r;
            }
            break;
        default: break;
        }
        p = js_own_prop(vm, o, key);
        if (p) {
            if (p->flags & P_ACCESSOR) {
                if (!p->v.a) return jv_undef();
                return js_call(vm, jv_obj(p->v.a), objv, 0, 0);
            }
            return p->v;
        }
        if (ob->cls == JS_CLASS_FUNC || ob->cls == JS_CLASS_NATIVE) {
            if (key == vm->s_length) return jv_int(fn_length_of(vm, o));
            if (key == vm->s_name) return jv_str(fn_name_of(vm, o));
            if (key == vm->s_prototype && ob->cls == JS_CLASS_FUNC) return fn_prototype(vm, o);
        }
        o = ob->proto;
    }
    return jv_undef();
}

jv js_get_c(js_vm_t *vm, jv obj, const char *name) {
    uint32_t k = js_intern(vm, name, cslen(name));
    if (vm->ctl == CTL_ABORT) return jv_undef();
    return js_get(vm, obj, k);
}

jv js_get_v(js_vm_t *vm, jv obj, jv key) {
    uint32_t k;
    if (key.t == JT_INT && (int32_t)key.a >= 0) {
        if (obj.t == JT_OBJ) {
            js_obj_t *o = JOBJ(vm, obj.a);
            if ((o->cls == JS_CLASS_ARRAY || o->cls == JS_CLASS_ARGS) && key.a < o->alen) {
                jv v = ((jv *)JPTR(vm, o->arr))[key.a];
                if (v.t != JT_EMPTY) return v;
            }
        } else if (obj.t == JT_STR) {
            js_str_t *p = JSTR(vm, obj.a);
            if (key.a < p->len) return jv_str(js_str_sub(vm, obj.a, (int)key.a, (int)key.a + 1));
        }
    }
    if (jv_is_nullish(obj)) {
        k = js_to_string(vm, key);
        if (vm->ctl) return jv_undef();
        return js_get(vm, obj, k);
    }
    k = js_to_key(vm, key);
    if (vm->ctl) return jv_undef();
    return js_get(vm, obj, k);
}

int js_put(js_vm_t *vm, jv objv, uint32_t key, jv v) {
    uint32_t o, start;
    js_obj_t *ob;
    js_prop_t *p;
    uint32_t idx;
    int depth = 0;
    if (objv.t != JT_OBJ) {
        if (jv_is_nullish(objv)) {
            js_throw_err2(vm, JE_TYPE, "Cannot set properties of undefined (setting '", js_str_cat(vm, key, js_str_c(vm, "')")));
            return 0;
        }
        return 1;      /* primitives: ignored (sloppy mode) */
    }
    start = objv.a;
    ob = JOBJ(vm, start);
    if (ob->oflags & O_FROZEN) return 1;
    if (ob->cls == JS_CLASS_HOST && vm->host_set) {
        if (vm->host_set(vm, objv, key, v)) return !vm->ctl;
        if (vm->ctl) return 0;
    }
    if (ob->cls == JS_CLASS_ARRAY || ob->cls == JS_CLASS_ARGS) {
        if (key == vm->s_length) {
            jf64 n = js_to_number(vm, v);
            uint32_t len = jf_to_u32(n);
            if (vm->ctl) return 0;
            if (!jf_eq(jf_from_u32(len), n)) { js_throw_err(vm, JE_RANGE, "Invalid array length"); return 0; }
            return js_array_set_len(vm, start, len);
        }
        if (js_str_index_key(vm, key, &idx)) return js_array_set(vm, start, idx, v);
    }
    p = js_own_prop(vm, start, key);
    if (p) {
        if (p->flags & P_ACCESSOR) {
            if (p->v.b) { js_call(vm, jv_obj(p->v.b), objv, &v, 1); return !vm->ctl; }
            return 1;
        }
        if (p->flags & P_WRITE) p->v = v;
        return 1;
    }
    o = JOBJ(vm, start)->proto;
    while (o && depth++ < 256) {
        js_prop_t *q = js_own_prop(vm, o, key);
        if (q) {
            if (q->flags & P_ACCESSOR) {
                if (q->v.b) { js_call(vm, jv_obj(q->v.b), objv, &v, 1); return !vm->ctl; }
                return 1;
            }
            if (!(q->flags & P_WRITE)) return 1;
            break;
        }
        o = JOBJ(vm, o)->proto;
    }
    if (!(JOBJ(vm, start)->oflags & O_EXT)) return 1;
    return js_define(vm, start, key, v, P_DEFAULT);
}

int js_put_v(js_vm_t *vm, jv obj, jv key, jv v) {
    uint32_t k;
    if (key.t == JT_INT && (int32_t)key.a >= 0 && obj.t == JT_OBJ) {
        js_obj_t *o = JOBJ(vm, obj.a);
        if (o->cls == JS_CLASS_ARRAY && !(o->oflags & O_FROZEN)) return js_array_set(vm, obj.a, key.a, v);
    }
    k = js_to_key(vm, key);
    if (vm->ctl) return 0;
    return js_put(vm, obj, k, v);
}

int js_has(js_vm_t *vm, uint32_t o, uint32_t key) {
    int depth = 0;
    while (o && depth++ < 256) {
        js_obj_t *ob = JOBJ(vm, o);
        uint32_t idx;
        if (ob->cls == JS_CLASS_HOST && vm->host_get) {
            jv out;
            if (vm->host_get(vm, jv_obj(o), key, &out)) return 1;
        }
        if (ob->cls == JS_CLASS_ARRAY || ob->cls == JS_CLASS_ARGS) {
            if (key == vm->s_length) return 1;
            if (js_str_index_key(vm, key, &idx) && idx < ob->alen &&
                ((jv *)JPTR(vm, ob->arr))[idx].t != JT_EMPTY) return 1;
        }
        if (ob->cls == JS_CLASS_STRING && ob->x0 == JT_STR) {
            int found;
            (void)string_own(vm, ob->x1, key, &found);
            if (found) return 1;
        }
        if (js_own_prop(vm, o, key)) return 1;
        if ((ob->cls == JS_CLASS_FUNC || ob->cls == JS_CLASS_NATIVE) &&
            (key == vm->s_length || key == vm->s_name || (key == vm->s_prototype && ob->cls == JS_CLASS_FUNC)))
            return 1;
        o = ob->proto;
    }
    return 0;
}

int js_delete(js_vm_t *vm, jv obj, jv key) {
    uint32_t k, idx;
    js_obj_t *o;
    if (obj.t != JT_OBJ) {
        if (jv_is_nullish(obj)) { js_throw_err(vm, JE_TYPE, "Cannot convert undefined or null to object"); return 0; }
        return 1;
    }
    k = js_to_key(vm, key);
    if (vm->ctl) return 0;
    o = JOBJ(vm, obj.a);
    if ((o->cls == JS_CLASS_ARRAY || o->cls == JS_CLASS_ARGS) && js_str_index_key(vm, k, &idx)) {
        if (o->oflags & O_FROZEN) return 0;
        if (idx < o->alen) ((jv *)JPTR(vm, o->arr))[idx] = jv_empty();
        return 1;
    }
    return js_delete_own(vm, obj.a, k);
}

uint32_t js_own_keys(js_vm_t *vm, uint32_t obj, int only_enum) {
    uint32_t out = js_array_new(vm, 8), i, pass;
    js_obj_t *o;
    if (!out) return 0;
    o = JOBJ(vm, obj);
    if (o->cls == JS_CLASS_ARRAY || o->cls == JS_CLASS_ARGS) {
        for (i = 0; i < JOBJ(vm, obj)->alen; i++) {
            o = JOBJ(vm, obj);
            if (((jv *)JPTR(vm, o->arr))[i].t == JT_EMPTY) continue;
            if (!js_array_push(vm, out, jv_str(js_key_of_index(vm, i)))) return out;
        }
    }
    if (o->cls == JS_CLASS_STRING && o->x0 == JT_STR) {
        uint32_t n = JSTR(vm, o->x1)->len;
        for (i = 0; i < n; i++) if (!js_array_push(vm, out, jv_str(js_key_of_index(vm, i)))) return out;
    }
    /* integer-like keys ascending first, then the others in creation order */
    for (pass = 0; pass < 2; pass++) {
        uint32_t last = 0;
        int have_last = 0;
        for (;;) {
            uint32_t best = 0, bidx = 0;
            int found = 0;
            o = JOBJ(vm, obj);
            if (pass == 0) {
                for (i = 0; i < o->nprop; i++) {
                    js_prop_t *p = (js_prop_t *)JPTR(vm, o->props) + i;
                    uint32_t idx;
                    if (only_enum && !(p->flags & P_ENUM)) continue;
                    if (!js_str_index_key(vm, p->key, &idx)) continue;
                    if (have_last && idx <= last) continue;
                    if (!found || idx < bidx) { found = 1; bidx = idx; best = p->key; }
                }
                if (!found) break;
                last = bidx;
                have_last = 1;
                if (!js_array_push(vm, out, jv_str(best))) return out;
            } else {
                for (i = 0; i < o->nprop; i++) {
                    js_prop_t *p = (js_prop_t *)JPTR(vm, o->props) + i;
                    uint32_t idx;
                    if (only_enum && !(p->flags & P_ENUM)) continue;
                    if (js_str_index_key(vm, p->key, &idx)) continue;
                    if (!js_array_push(vm, out, jv_str(p->key))) return out;
                    o = JOBJ(vm, obj);
                }
                break;
            }
        }
    }
    return out;
}

int js_instanceof(js_vm_t *vm, jv v, jv ctor) {
    jv proto;
    uint32_t o;
    int depth = 0;
    if (!js_is_callable(vm, ctor)) { js_throw_err(vm, JE_TYPE, "Right-hand side of 'instanceof' is not callable"); return 0; }
    if (JOBJ(vm, ctor.a)->cls == JS_CLASS_BOUND) ctor = jv_obj(JOBJ(vm, ctor.a)->x0);
    if (v.t != JT_OBJ) return 0;
    proto = js_get(vm, ctor, vm->s_prototype);
    if (vm->ctl || proto.t != JT_OBJ) return 0;
    o = JOBJ(vm, v.a)->proto;
    while (o && depth++ < 256) {
        if (o == proto.a) return 1;
        o = JOBJ(vm, o)->proto;
    }
    return 0;
}

jv js_length_of(js_vm_t *vm, jv obj, uint32_t *len) {
    jv l;
    jf64 n;
    *len = 0;
    if (obj.t == JT_OBJ) {
        js_obj_t *o = JOBJ(vm, obj.a);
        if (o->cls == JS_CLASS_ARRAY || o->cls == JS_CLASS_ARGS) { *len = o->alen; return jv_undef(); }
    }
    l = js_get(vm, obj, vm->s_length);
    if (vm->ctl) return jv_undef();
    n = js_to_integer(vm, l);
    if (vm->ctl) return jv_undef();
    if (jf_lt(n, 0)) n = 0;
    if (jf_lt(jf_from_u32(JS_ARRAY_MAX), n)) n = jf_from_u32(JS_ARRAY_MAX);
    *len = jf_to_u32(n);
    return jv_undef();
}

void js_log_put(js_vm_t *vm, const char *s, int n) {
    int i;
    for (i = 0; i < n; i++) {
        if (vm->log_len >= JS_LOG_MAX - 4) {
            if (vm->log_len < JS_LOG_MAX - 1) {
                vm->log[vm->log_len++] = '~';
                vm->log[vm->log_len] = 0;
            }
            return;
        }
        vm->log[vm->log_len++] = s[i];
    }
    vm->log[vm->log_len] = 0;
}

const char *js_log_text(js_vm_t *vm, int *len) {
    if (len) *len = vm->log_len;
    return vm->log;
}

void js_log_clear(js_vm_t *vm) { vm->log_len = 0; vm->log[0] = 0; }

void js_set_budget(js_vm_t *vm, uint32_t steps) { vm->budget = steps; }
uint32_t js_mem_used(const js_vm_t *vm) { return vm->top; }
uint32_t js_mem_size(const js_vm_t *vm) { return vm->limit; }
uint32_t js_steps_used(const js_vm_t *vm) { return vm->steps; }
