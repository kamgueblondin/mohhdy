/* wf_jslib.c - Lot 113: realm setup, the private stack, js_eval, the
 * console display, and the Object, Function, Array, Error, console and
 * global function built-ins of the JavaScript subset (see wf_js.h).
 * String, Number, Math, JSON, Date, Map and Set are in wf_jslib2.c. */
#include "wf_jsi.h"

static int cslen(const char *s) { int n = 0; while (s[n]) n++; return n; }
#define KEY(vm, s) js_intern((vm), (s), cslen(s))

/* ---- the private stack ---- */

static void (*g_entry)(js_vm_t *);
static js_vm_t *g_entry_vm;

static void entry_thunk(void) { g_entry(g_entry_vm); }

static void on_stack(js_vm_t *vm, void (*fn)(js_vm_t *)) {
    void (*saved_fn)(js_vm_t *) = g_entry;
    js_vm_t *saved_vm = g_entry_vm;
    g_entry = fn;
    g_entry_vm = vm;
#if defined(__i386__)
    if (vm->stack_top && vm->stack_base) {
        /* esi keeps the caller's stack pointer across the call */
        __asm__ volatile(
            "movl %%esp, %%esi\n\t"
            "movl %0, %%esp\n\t"
            "andl $-16, %%esp\n\t"
            "call *%1\n\t"
            "movl %%esi, %%esp\n\t"
            :
            : "r"(vm->stack_top), "r"(entry_thunk)
            : "esi", "eax", "ecx", "edx", "memory", "cc");
    } else {
        entry_thunk();
    }
#else
    entry_thunk();
#endif
    g_entry = saved_fn;
    g_entry_vm = saved_vm;
}

/* ---- helpers ---- */

uint32_t js_ctor_new(js_vm_t *vm, const char *name, js_native_fn fn, int nargs, uint32_t proto) {
    uint32_t c = js_native_new(vm, name, fn, nargs, 0);
    if (!c) return 0;
    if (proto) {
        js_define(vm, c, vm->s_prototype, jv_obj(proto), 0);
        js_define(vm, proto, vm->s_constructor, jv_obj(c), P_HIDDEN);
    }
    js_define(vm, vm->global, JOBJ(vm, c)->x2, jv_obj(c), P_HIDDEN);
    return c;
}

static jv callback_arg(js_vm_t *vm, jv *argv, int argc, int i) {
    jv f = JARG(i);
    if (!js_is_callable(vm, f)) return js_throw_err(vm, JE_TYPE, "callback is not a function");
    return f;
}

static int idx_arg(js_vm_t *vm, jv v, int len, int dflt) {
    jf64 n;
    int64_t i;
    if (v.t == JT_UNDEF) return dflt;
    n = js_to_integer(vm, v);
    if (jf_isinf(n)) return jf_signbit(n) ? 0 : len;
    (void)jf_to_i64(n, &i);
    if (i < 0) { i += len; if (i < 0) i = 0; }
    if (i > len) i = len;
    return (int)i;
}

/* ---- js_iter_values: arrays, strings, arguments, Map, Set, array-likes of the host ---- */

static uint32_t keyed_values(js_vm_t *vm, js_obj_t *o, int pairs) {
    uint32_t ks = o->x0, vs = o->x1, n = JOBJ(vm, ks)->alen, i, arr;
    arr = js_array_new(vm, n);
    if (!arr) return 0;
    for (i = 0; i < n; i++) {
        jv k = ((jv *)JPTR(vm, JOBJ(vm, ks)->arr))[i];
        if (k.t == JT_EMPTY) continue;
        if (pairs) {
            uint32_t pair = js_array_new(vm, 2);
            if (!pair) return 0;
            js_array_push(vm, pair, k);
            js_array_push(vm, pair, js_array_get(vm, vs, i));
            js_array_push(vm, arr, jv_obj(pair));
        } else {
            js_array_push(vm, arr, k);
        }
    }
    return arr;
}

jv js_iter_values(js_vm_t *vm, jv v, uint32_t *arr_out) {
    uint32_t arr, i;
    *arr_out = 0;
    if (v.t == JT_STR) {
        uint32_t n = JSTR(vm, v.a)->len;
        arr = js_array_new(vm, n < 16 ? 16 : n);
        if (!arr) return jv_undef();
        i = 0;
        while (i < n) {      /* one element per UTF-8 sequence */
            uint8_t c = (uint8_t)JSTR(vm, v.a)->s[i];
            uint32_t l = c < 0x80 ? 1 : c < 0xE0 ? 2 : c < 0xF0 ? 3 : 4;
            if (i + l > n) l = n - i;
            js_array_push(vm, arr, jv_str(js_str_sub(vm, v.a, (int)i, (int)(i + l))));
            i += l;
        }
        *arr_out = arr;
        return jv_obj(arr);
    }
    if (v.t == JT_OBJ) {
        js_obj_t *o = JOBJ(vm, v.a);
        if (o->cls == JS_CLASS_ARRAY) { *arr_out = v.a; return v; }
        if (o->cls == JS_CLASS_ARGS) {
            arr = js_array_new(vm, o->alen);
            if (!arr) return jv_undef();
            for (i = 0; i < JOBJ(vm, v.a)->alen; i++) js_array_push(vm, arr, js_array_get(vm, v.a, i));
            *arr_out = arr;
            return jv_obj(arr);
        }
        if (o->cls == JS_CLASS_MAP || o->cls == JS_CLASS_SET) {
            arr = keyed_values(vm, o, o->cls == JS_CLASS_MAP);
            if (!arr) return jv_undef();
            *arr_out = arr;
            return jv_obj(arr);
        }
        if (o->cls == JS_CLASS_HOST) {      /* NodeList, HTMLCollection: length + index */
            uint32_t len;
            jv lv = js_get(vm, v, vm->s_length);
            if (vm->ctl) return jv_undef();
            if (jv_is_num(lv)) {
                js_length_of(vm, v, &len);
                arr = js_array_new(vm, len);
                if (!arr) return jv_undef();
                for (i = 0; i < len && !vm->ctl; i++) js_array_push(vm, arr, js_get_v(vm, v, jv_int((int32_t)i)));
                *arr_out = arr;
                return jv_obj(arr);
            }
        }
    }
    js_throw_err(vm, JE_TYPE, "value is not iterable (the guest iterates arrays, strings, Map, Set and arguments)");
    return jv_undef();
}

uint32_t js_join(js_vm_t *vm, uint32_t arr, uint32_t sep) {
    uint32_t i, len;
    js_sb_t b;
    jv a = jv_obj(arr);
    js_length_of(vm, a, &len);
    js_sb_init(vm, &b);
    for (i = 0; i < len && !vm->ctl; i++) {
        jv v = js_get_v(vm, a, jv_int((int32_t)i));
        if (vm->ctl) break;
        if (i) js_sb_str(vm, &b, sep);
        if (!jv_is_nullish(v)) {
            uint32_t s = js_to_string(vm, v);
            if (vm->ctl) break;
            js_sb_str(vm, &b, s);
        }
    }
    if (vm->ctl) return vm->s_empty;
    return js_sb_finish(vm, &b);
}

/* ---- Object ---- */

static jv obj_ctor(js_vm_t *vm, jv thisv, jv *argv, int argc, uint32_t m) {
    jv v = JARG(0);
    uint32_t o;
    (void)thisv; (void)m;
    if (jv_is_nullish(v)) o = js_obj_new(vm, JS_CLASS_OBJECT, vm->p_object);
    else o = js_to_object(vm, v);
    return o ? jv_obj(o) : jv_undef();
}

/* keys 0, values 1, entries 2, getOwnPropertyNames 3 */
static jv obj_keys(js_vm_t *vm, jv thisv, jv *argv, int argc, uint32_t mode) {
    uint32_t o, keys, out, i;
    (void)thisv;
    o = js_to_object(vm, JARG(0));
    if (!o) return jv_undef();
    keys = js_own_keys(vm, o, mode != 3);
    if (!keys || mode == 0 || mode == 3) return keys ? jv_obj(keys) : jv_undef();
    out = js_array_new(vm, JOBJ(vm, keys)->alen);
    if (!out) return jv_undef();
    for (i = 0; i < JOBJ(vm, keys)->alen && !vm->ctl; i++) {
        jv k = js_array_get(vm, keys, i), v = js_get(vm, jv_obj(o), k.a);
        if (vm->ctl) return jv_undef();
        if (mode == 1) {
            js_array_push(vm, out, v);
        } else {
            uint32_t pair = js_array_new(vm, 2);
            if (!pair) return jv_undef();
            js_array_push(vm, pair, k);
            js_array_push(vm, pair, v);
            js_array_push(vm, out, jv_obj(pair));
        }
    }
    return jv_obj(out);
}

static jv obj_assign(js_vm_t *vm, jv thisv, jv *argv, int argc, uint32_t m) {
    uint32_t t = js_to_object(vm, JARG(0));
    int a;
    (void)thisv; (void)m;
    if (!t) return jv_undef();
    for (a = 1; a < argc && !vm->ctl; a++) {
        uint32_t keys, i;
        if (argv[a].t != JT_OBJ) continue;
        keys = js_own_keys(vm, argv[a].a, 1);
        for (i = 0; keys && i < JOBJ(vm, keys)->alen && !vm->ctl; i++) {
            jv k = js_array_get(vm, keys, i), v = js_get(vm, argv[a], k.a);
            if (vm->ctl) break;
            js_put(vm, jv_obj(t), k.a, v);
        }
    }
    return jv_obj(t);
}

static int desc_flag(js_vm_t *vm, jv desc, uint32_t key, uint32_t bit, uint32_t *flags) {
    if (!js_has(vm, desc.a, key)) return 0;
    if (js_truthy(vm, js_get(vm, desc, key))) *flags |= bit;
    else *flags &= ~bit;
    return 1;
}

static void define_from_desc(js_vm_t *vm, uint32_t o, uint32_t key, jv desc) {
    jv g, s, v;
    uint32_t flags = 0;
    js_prop_t *old;
    if (desc.t != JT_OBJ) { js_throw_err(vm, JE_TYPE, "Property description must be an object"); return; }
    old = js_own_prop(vm, o, key);
    if (old) flags = old->flags & (P_ENUM | P_WRITE | P_CONF);
    desc_flag(vm, desc, vm->s_enumerable, P_ENUM, &flags);
    desc_flag(vm, desc, vm->s_configurable, P_CONF, &flags);
    g = js_get(vm, desc, vm->s_get);
    s = js_get(vm, desc, vm->s_set);
    if (vm->ctl) return;
    if (js_is_callable(vm, g) || js_is_callable(vm, s)) {
        int c = JOBJ(vm, o)->cls;
        if (c == JS_CLASS_ARRAY || c == JS_CLASS_ARGS) {
            uint32_t idx;
            if (js_str_index_key(vm, key, &idx)) { js_throw_err(vm, JE_TYPE, "accessors on array elements are not supported by the guest"); return; }
        }
        js_define_accessor(vm, o, key, js_is_callable(vm, g) ? g.a : 0, js_is_callable(vm, s) ? s.a : 0, flags & ~P_WRITE);
        return;
    }
    desc_flag(vm, desc, vm->s_writable, P_WRITE, &flags);
    old = js_own_prop(vm, o, key);
    if (js_has(vm, desc.a, vm->s_value)) v = js_get(vm, desc, vm->s_value);
    else v = old && !(old->flags & P_ACCESSOR) ? old->v : jv_undef();
    if (vm->ctl) return;
    {
        int c = JOBJ(vm, o)->cls;
        uint32_t idx;
        if (((c == JS_CLASS_ARRAY || c == JS_CLASS_ARGS) && (key == vm->s_length || js_str_index_key(vm, key, &idx))) ||
            c == JS_CLASS_HOST) {
            js_put(vm, jv_obj(o), key, v);
            return;
        }
    }
    js_define(vm, o, key, v, flags);
}

/* defineProperty 0, defineProperties 1 */
static jv obj_define(js_vm_t *vm, jv thisv, jv *argv, int argc, uint32_t m) {
    jv o = JARG(0);
    (void)thisv;
    if (o.t != JT_OBJ) return js_throw_err(vm, JE_TYPE, "Object.defineProperty called on non-object");
    if (m == 0) {
        uint32_t k = js_to_key(vm, JARG(1));
        if (vm->ctl) return jv_undef();
        define_from_desc(vm, o.a, k, JARG(2));
    } else {
        jv props = JARG(1);
        uint32_t keys, i;
        if (props.t != JT_OBJ) return o;
        keys = js_own_keys(vm, props.a, 1);
        for (i = 0; keys && i < JOBJ(vm, keys)->alen && !vm->ctl; i++) {
            jv k = js_array_get(vm, keys, i);
            jv d = js_get(vm, props, k.a);
            if (vm->ctl) break;
            define_from_desc(vm, o.a, k.a, d);
        }
    }
    return vm->ctl ? jv_undef() : o;
}

static jv obj_create(js_vm_t *vm, jv thisv, jv *argv, int argc, uint32_t m) {
    jv p = JARG(0);
    uint32_t o;
    (void)thisv; (void)m;
    if (p.t != JT_OBJ && p.t != JT_NULL) return js_throw_err(vm, JE_TYPE, "Object prototype may only be an Object or null");
    o = js_obj_new(vm, JS_CLASS_OBJECT, p.t == JT_OBJ ? p.a : 0);
    if (!o) return jv_undef();
    if (argc > 1 && argv[1].t == JT_OBJ) {
        jv a2[2];
        a2[0] = jv_obj(o);
        a2[1] = argv[1];
        obj_define(vm, jv_undef(), a2, 2, 1);
    }
    return vm->ctl ? jv_undef() : jv_obj(o);
}

/* getPrototypeOf 0, setPrototypeOf 1 */
static jv obj_proto_of(js_vm_t *vm, jv thisv, jv *argv, int argc, uint32_t m) {
    jv o = JARG(0);
    (void)thisv;
    if (m == 1) {
        jv p = JARG(1);
        if (p.t != JT_OBJ && p.t != JT_NULL) return js_throw_err(vm, JE_TYPE, "Object prototype may only be an Object or null");
        if (o.t == JT_OBJ) {
            uint32_t q;
            int d = 0;
            for (q = p.t == JT_OBJ ? p.a : 0; q && d++ < 256; q = JOBJ(vm, q)->proto)
                if (q == o.a) return js_throw_err(vm, JE_TYPE, "Cyclic __proto__ value");
            JOBJ(vm, o.a)->proto = p.t == JT_OBJ ? p.a : 0;
        }
        return o;
    }
    if (o.t != JT_OBJ) {
        uint32_t w = js_to_object(vm, o);
        if (!w) return jv_undef();
        o = jv_obj(w);
    }
    return JOBJ(vm, o.a)->proto ? jv_obj(JOBJ(vm, o.a)->proto) : jv_null();
}

/* freeze 0, seal/preventExtensions 1, isFrozen/isSealed 3, isExtensible 4 */
static jv obj_freeze(js_vm_t *vm, jv thisv, jv *argv, int argc, uint32_t m) {
    jv o = JARG(0);
    (void)thisv;
    if (o.t != JT_OBJ) return m == 3 ? jv_bool(1) : m == 4 ? jv_bool(0) : o;
    switch (m) {
    case 0: JOBJ(vm, o.a)->oflags = O_FROZEN; return o;
    case 1: JOBJ(vm, o.a)->oflags &= (uint8_t)~O_EXT; return o;
    case 3: return jv_bool((JOBJ(vm, o.a)->oflags & O_FROZEN) != 0);
    case 4: return jv_bool((JOBJ(vm, o.a)->oflags & O_EXT) != 0);
    default: return o;
    }
}

static void desc_put(js_vm_t *vm, uint32_t d, uint32_t key, jv v) { js_define(vm, d, key, v, P_DEFAULT); }

static jv desc_of(js_vm_t *vm, jv o, uint32_t key) {
    js_prop_t *p;
    uint32_t d, idx;
    js_obj_t *ob;
    if (o.t != JT_OBJ) return jv_undef();
    p = js_own_prop(vm, o.a, key);
    ob = JOBJ(vm, o.a);
    if (!p) {
        int own = 0;
        if ((ob->cls == JS_CLASS_ARRAY || ob->cls == JS_CLASS_ARGS) &&
            (key == vm->s_length || (js_str_index_key(vm, key, &idx) && idx < ob->alen &&
                                     ((jv *)JPTR(vm, ob->arr))[idx].t != JT_EMPTY)))
            own = 1;
        if ((ob->cls == JS_CLASS_FUNC || ob->cls == JS_CLASS_NATIVE) && (key == vm->s_length || key == vm->s_name)) own = 1;
        if (!own) return jv_undef();
        d = js_obj_new(vm, JS_CLASS_OBJECT, vm->p_object);
        if (!d) return jv_undef();
        desc_put(vm, d, vm->s_value, js_get(vm, o, key));
        desc_put(vm, d, vm->s_writable, jv_bool(key != vm->s_name && (key != vm->s_length || ob->cls == JS_CLASS_ARRAY)));
        desc_put(vm, d, vm->s_enumerable, jv_bool(key != vm->s_length && key != vm->s_name));
        desc_put(vm, d, vm->s_configurable, jv_bool(key != vm->s_length || ob->cls != JS_CLASS_ARRAY));
        return jv_obj(d);
    }
    d = js_obj_new(vm, JS_CLASS_OBJECT, vm->p_object);
    if (!d) return jv_undef();
    p = js_own_prop(vm, o.a, key);
    if (p->flags & P_ACCESSOR) {
        uint32_t g = p->v.a, s = p->v.b;
        desc_put(vm, d, vm->s_get, g ? jv_obj(g) : jv_undef());
        desc_put(vm, d, vm->s_set, s ? jv_obj(s) : jv_undef());
    } else {
        jv v = p->v;
        uint32_t f = p->flags;
        desc_put(vm, d, vm->s_value, v);
        desc_put(vm, d, vm->s_writable, jv_bool((f & P_WRITE) != 0 && !(JOBJ(vm, o.a)->oflags & O_FROZEN)));
    }
    p = js_own_prop(vm, o.a, key);
    {
        uint32_t f = p->flags;
        desc_put(vm, d, vm->s_enumerable, jv_bool((f & P_ENUM) != 0));
        desc_put(vm, d, vm->s_configurable, jv_bool((f & P_CONF) != 0 && !(JOBJ(vm, o.a)->oflags & O_FROZEN)));
    }
    return jv_obj(d);
}

static jv obj_get_desc(js_vm_t *vm, jv thisv, jv *argv, int argc, uint32_t m) {
    uint32_t k;
    (void)thisv; (void)m;
    k = js_to_key(vm, JARG(1));
    if (vm->ctl) return jv_undef();
    return desc_of(vm, JARG(0), k);
}

static jv obj_is(js_vm_t *vm, jv thisv, jv *argv, int argc, uint32_t m) {
    jv a = JARG(0), b = JARG(1);
    (void)thisv; (void)m;
    if (jv_is_num(a) && jv_is_num(b)) return jv_bool(jf_same_value(jv_f64(a), jv_f64(b)));
    return jv_bool(js_strict_eq(vm, a, b));
}

static jv obj_from_entries(js_vm_t *vm, jv thisv, jv *argv, int argc, uint32_t m) {
    uint32_t arr = 0, o, i;
    (void)thisv; (void)m;
    js_iter_values(vm, JARG(0), &arr);
    if (vm->ctl) return jv_undef();
    o = js_obj_new(vm, JS_CLASS_OBJECT, vm->p_object);
    if (!o) return jv_undef();
    for (i = 0; i < JOBJ(vm, arr)->alen && !vm->ctl; i++) {
        jv e = js_array_get(vm, arr, i), v;
        uint32_t k;
        if (e.t != JT_OBJ) return js_throw_err(vm, JE_TYPE, "Iterator value is not an entry object");
        k = js_to_key(vm, js_get_v(vm, e, jv_int(0)));
        v = js_get_v(vm, e, jv_int(1));
        if (vm->ctl) return jv_undef();
        js_define(vm, o, k, v, P_DEFAULT);
    }
    return jv_obj(o);
}

/* hasOwnProperty (this, key) and Object.hasOwn (m 1: obj, key) */
static jv objp_has_own(js_vm_t *vm, jv thisv, jv *argv, int argc, uint32_t m) {
    uint32_t k, o, idx;
    js_obj_t *ob;
    jv target = m ? JARG(0) : thisv, kv = m ? JARG(1) : JARG(0);
    k = js_to_key(vm, kv);
    if (vm->ctl) return jv_undef();
    o = js_to_object(vm, target);
    if (!o) return jv_undef();
    ob = JOBJ(vm, o);
    if (ob->cls == JS_CLASS_ARRAY || ob->cls == JS_CLASS_ARGS) {
        if (k == vm->s_length) return jv_bool(1);
        if (js_str_index_key(vm, k, &idx)) return jv_bool(idx < ob->alen && ((jv *)JPTR(vm, ob->arr))[idx].t != JT_EMPTY);
    }
    if (ob->cls == JS_CLASS_STRING || ob->cls == JS_CLASS_HOST) {
        uint32_t pr = ob->proto;
        int r;
        ob->proto = 0;
        r = js_has(vm, o, k);
        JOBJ(vm, o)->proto = pr;
        return jv_bool(r);
    }
    if ((ob->cls == JS_CLASS_FUNC || ob->cls == JS_CLASS_NATIVE) && (k == vm->s_length || k == vm->s_name)) return jv_bool(1);
    if (ob->cls == JS_CLASS_FUNC && k == vm->s_prototype) {
        jv pv = js_get(vm, jv_obj(o), k);     /* the lazy prototype exists once read */
        (void)pv;
    }
    return jv_bool(js_own_prop(vm, o, k) != 0);
}

static jv objp_is_proto(js_vm_t *vm, jv thisv, jv *argv, int argc, uint32_t m) {
    jv v = JARG(0);
    uint32_t o;
    int d = 0;
    (void)m;
    if (v.t != JT_OBJ || thisv.t != JT_OBJ) return jv_bool(0);
    for (o = JOBJ(vm, v.a)->proto; o && d++ < 256; o = JOBJ(vm, o)->proto) if (o == thisv.a) return jv_bool(1);
    return jv_bool(0);
}

static jv objp_enum(js_vm_t *vm, jv thisv, jv *argv, int argc, uint32_t m) {
    uint32_t k, idx;
    js_prop_t *p;
    js_obj_t *o;
    (void)m;
    if (thisv.t != JT_OBJ) return jv_bool(0);
    k = js_to_key(vm, JARG(0));
    if (vm->ctl) return jv_undef();
    p = js_own_prop(vm, thisv.a, k);
    if (p) return jv_bool((p->flags & P_ENUM) != 0);
    o = JOBJ(vm, thisv.a);
    if ((o->cls == JS_CLASS_ARRAY || o->cls == JS_CLASS_ARGS) && js_str_index_key(vm, k, &idx))
        return jv_bool(idx < o->alen && ((jv *)JPTR(vm, o->arr))[idx].t != JT_EMPTY);
    return jv_bool(0);
}

static const char *class_tag(js_vm_t *vm, jv v) {
    if (v.t == JT_UNDEF) return "Undefined";
    if (v.t == JT_NULL) return "Null";
    if (v.t == JT_STR) return "String";
    if (jv_is_num(v)) return "Number";
    if (v.t == JT_BOOL) return "Boolean";
    switch (JOBJ(vm, v.a)->cls) {
    case JS_CLASS_ARRAY: return "Array";
    case JS_CLASS_FUNC: case JS_CLASS_NATIVE: case JS_CLASS_BOUND: return "Function";
    case JS_CLASS_ERROR: return "Error";
    case JS_CLASS_BOOLEAN: return "Boolean";
    case JS_CLASS_NUMBER: return "Number";
    case JS_CLASS_STRING: return "String";
    case JS_CLASS_REGEXP: return "RegExp";
    case JS_CLASS_ARGS: return "Arguments";
    case JS_CLASS_DATE: return "Date";
    case JS_CLASS_MAP: return "Map";
    case JS_CLASS_SET: return "Set";
    default: return "Object";
    }
}

static jv objp_to_string(js_vm_t *vm, jv thisv, jv *argv, int argc, uint32_t m) {
    uint32_t s = js_str_c(vm, "[object ");
    (void)argv; (void)argc; (void)m;
    s = js_str_cat(vm, s, js_str_c(vm, class_tag(vm, thisv)));
    return jv_str(js_str_cat(vm, s, js_str_c(vm, "]")));
}

static jv objp_value_of(js_vm_t *vm, jv thisv, jv *argv, int argc, uint32_t m) {
    uint32_t o;
    (void)argv; (void)argc; (void)m;
    o = js_to_object(vm, thisv);
    return o ? jv_obj(o) : jv_undef();
}

static jv objp_to_locale(js_vm_t *vm, jv thisv, jv *argv, int argc, uint32_t m) {
    jv f = js_get(vm, thisv, vm->s_toString);
    (void)argv; (void)argc; (void)m;
    if (vm->ctl) return jv_undef();
    return js_call(vm, f, thisv, 0, 0);
}

/* ---- Function ---- */

static jv fn_ctor(js_vm_t *vm, jv thisv, jv *argv, int argc, uint32_t m) {
    (void)thisv; (void)argv; (void)argc; (void)m;
    return js_throw_err(vm, JE_EVAL, "Function() and eval are not supported by the guest");
}

static jv fnp_call(js_vm_t *vm, jv thisv, jv *argv, int argc, uint32_t m) {
    (void)m;
    if (!js_is_callable(vm, thisv)) return js_throw_err(vm, JE_TYPE, "Function.prototype.call called on a non-function");
    return js_call(vm, thisv, JARG(0), argc > 1 ? argv + 1 : 0, argc > 1 ? argc - 1 : 0);
}

static jv fnp_apply(js_vm_t *vm, jv thisv, jv *argv, int argc, uint32_t m) {
    jv list = JARG(1);
    uint32_t len, i, arr;
    (void)m;
    if (!js_is_callable(vm, thisv)) return js_throw_err(vm, JE_TYPE, "Function.prototype.apply called on a non-function");
    if (jv_is_nullish(list)) return js_call(vm, thisv, JARG(0), 0, 0);
    if (list.t != JT_OBJ) return js_throw_err(vm, JE_TYPE, "CreateListFromArrayLike called on non-object");
    js_length_of(vm, list, &len);
    if (vm->ctl) return jv_undef();
    if (len > 65536) return js_throw_err(vm, JE_RANGE, "too many arguments for the guest");
    arr = js_array_new(vm, len);
    if (!arr) return jv_undef();
    for (i = 0; i < len && !vm->ctl; i++) {
        jv e = js_get_v(vm, list, jv_int((int32_t)i));
        js_array_push(vm, arr, e.t == JT_EMPTY ? jv_undef() : e);
    }
    if (vm->ctl) return jv_undef();
    return js_call(vm, thisv, JARG(0), len ? (jv *)JPTR(vm, JOBJ(vm, arr)->arr) : 0, (int)len);
}

static jv fnp_bind(js_vm_t *vm, jv thisv, jv *argv, int argc, uint32_t m) {
    uint32_t b, blk, s;
    int nb = argc > 1 ? argc - 1 : 0, i;
    jv nm;
    (void)m;
    if (!js_is_callable(vm, thisv)) return js_throw_err(vm, JE_TYPE, "Bind must be called on a function");
    blk = js_alloc(vm, (uint32_t)(1 + nb) * (uint32_t)sizeof(jv));
    b = blk ? js_obj_new(vm, JS_CLASS_BOUND, vm->p_function) : 0;
    if (!b) return jv_undef();
    ((jv *)JPTR(vm, blk))[0] = JARG(0);
    for (i = 0; i < nb; i++) ((jv *)JPTR(vm, blk))[1 + i] = argv[1 + i];
    JOBJ(vm, b)->x0 = thisv.a;
    JOBJ(vm, b)->x1 = blk;
    JOBJ(vm, b)->x2 = (uint32_t)nb;
    nm = js_get(vm, thisv, vm->s_name);
    if (vm->ctl) return jv_undef();
    s = js_str_c(vm, "bound ");
    if (nm.t == JT_STR) s = js_str_cat(vm, s, nm.a);
    js_define(vm, b, vm->s_name, jv_str(s), P_CONF);
    return jv_obj(b);
}

static jv fnp_to_string(js_vm_t *vm, jv thisv, jv *argv, int argc, uint32_t m) {
    jv nm;
    uint32_t s;
    (void)argv; (void)argc; (void)m;
    if (!js_is_callable(vm, thisv)) return js_throw_err(vm, JE_TYPE, "Function.prototype.toString requires a function");
    nm = js_get(vm, thisv, vm->s_name);
    if (vm->ctl) return jv_undef();
    s = js_str_c(vm, "function ");
    if (nm.t == JT_STR) s = js_str_cat(vm, s, nm.a);
    return jv_str(js_str_cat(vm, s, js_str_c(vm, JOBJ(vm, thisv.a)->cls == JS_CLASS_FUNC ? "() { [code] }" : "() { [native code] }")));
}

/* ---- Array ---- */

static jv arr_ctor(js_vm_t *vm, jv thisv, jv *argv, int argc, uint32_t m) {
    uint32_t a;
    int i;
    (void)thisv; (void)m;
    if (argc == 1 && jv_is_num(argv[0])) {
        jf64 n = jv_f64(argv[0]);
        uint32_t len = jf_to_u32(n);
        if (!jf_eq(jf_from_u32(len), n)) return js_throw_err(vm, JE_RANGE, "Invalid array length");
        a = js_array_new(vm, 0);
        if (!a || !js_array_set_len(vm, a, len)) return jv_undef();
        return jv_obj(a);
    }
    a = js_array_new(vm, (uint32_t)argc);
    if (!a) return jv_undef();
    for (i = 0; i < argc; i++) js_array_push(vm, a, argv[i]);
    return jv_obj(a);
}

static jv arr_is_array(js_vm_t *vm, jv thisv, jv *argv, int argc, uint32_t m) {
    (void)thisv; (void)m;
    return jv_bool(js_is_array(vm, JARG(0)));
}

static jv arr_from(js_vm_t *vm, jv thisv, jv *argv, int argc, uint32_t m) {
    jv src = JARG(0), fn = JARG(1);
    uint32_t out, i, len;
    (void)thisv; (void)m;
    if (jv_is_nullish(src)) return js_throw_err(vm, JE_TYPE, "Array.from requires an array-like or iterable object");
    if (fn.t != JT_UNDEF && !js_is_callable(vm, fn)) return js_throw_err(vm, JE_TYPE, "Array.from: the map function is not callable");
    out = js_array_new(vm, 4);
    if (!out) return jv_undef();
    if (src.t == JT_STR || (src.t == JT_OBJ && JOBJ(vm, src.a)->cls != JS_CLASS_OBJECT && JOBJ(vm, src.a)->cls != JS_CLASS_FUNC)) {
        uint32_t arr = 0;
        js_iter_values(vm, src, &arr);
        if (vm->ctl) return jv_undef();
        len = JOBJ(vm, arr)->alen;
        for (i = 0; i < len; i++) js_array_push(vm, out, js_array_get(vm, arr, i));
    } else if (src.t == JT_OBJ) {
        js_length_of(vm, src, &len);
        for (i = 0; i < len && !vm->ctl; i++) js_array_push(vm, out, js_get_v(vm, src, jv_int((int32_t)i)));
    }
    if (js_is_callable(vm, fn)) {
        for (i = 0; i < JOBJ(vm, out)->alen && !vm->ctl; i++) {
            jv a2[2], r;
            a2[0] = js_array_get(vm, out, i);
            a2[1] = jv_int((int32_t)i);
            r = js_call(vm, fn, JARG(2), a2, 2);
            if (vm->ctl) break;
            js_array_set(vm, out, i, r);
        }
    }
    return vm->ctl ? jv_undef() : jv_obj(out);
}

/* this as an object and its length (the generic methods) */
static int arr_this(js_vm_t *vm, jv thisv, jv *o, uint32_t *len) {
    uint32_t w;
    if (jv_is_nullish(thisv)) { js_throw_err(vm, JE_TYPE, "Array.prototype method called on null or undefined"); return 0; }
    w = js_to_object(vm, thisv);
    if (!w) return 0;
    *o = jv_obj(w);
    js_length_of(vm, *o, len);
    return !vm->ctl;
}

static int is_dense(js_vm_t *vm, jv o) {
    int c = JOBJ(vm, o.a)->cls;
    return c == JS_CLASS_ARRAY || c == JS_CLASS_ARGS;
}

static jv elem(js_vm_t *vm, jv o, uint32_t i) {
    js_obj_t *ob = JOBJ(vm, o.a);
    if (is_dense(vm, o) && i < ob->alen) {
        jv v = ((jv *)JPTR(vm, ob->arr))[i];
        if (v.t != JT_EMPTY) return v;
        if (!ob->proto || ob->proto == vm->p_array) return jv_undef();
    }
    return js_get_v(vm, o, jv_int((int32_t)i));
}

static int has_elem(js_vm_t *vm, jv o, uint32_t i) {
    js_obj_t *ob = JOBJ(vm, o.a);
    if (is_dense(vm, o)) return i < ob->alen && ((jv *)JPTR(vm, ob->arr))[i].t != JT_EMPTY;
    return js_has(vm, o.a, js_key_of_index(vm, i));
}

static void set_elem(js_vm_t *vm, jv o, uint32_t i, jv v) {
    if (JOBJ(vm, o.a)->cls == JS_CLASS_ARRAY && !(JOBJ(vm, o.a)->oflags & O_FROZEN)) js_array_set(vm, o.a, i, v);
    else js_put_v(vm, o, jv_int((int32_t)i), v);
}

static void set_length(js_vm_t *vm, jv o, uint32_t len) {
    if (JOBJ(vm, o.a)->cls == JS_CLASS_ARRAY && !(JOBJ(vm, o.a)->oflags & O_FROZEN)) js_array_set_len(vm, o.a, len);
    else js_put(vm, o, vm->s_length, jv_num(jf_from_u32(len)));
}

static jv arrp_push(js_vm_t *vm, jv thisv, jv *argv, int argc, uint32_t m) {
    jv o;
    uint32_t len;
    int i;
    (void)m;
    if (!arr_this(vm, thisv, &o, &len)) return jv_undef();
    if (JOBJ(vm, o.a)->oflags & O_FROZEN) return js_throw_err(vm, JE_TYPE, "Cannot add property, object is not extensible");
    for (i = 0; i < argc && !vm->ctl; i++) set_elem(vm, o, len + (uint32_t)i, argv[i]);
    if (JOBJ(vm, o.a)->cls != JS_CLASS_ARRAY) set_length(vm, o, len + (uint32_t)argc);
    return vm->ctl ? jv_undef() : jv_num(jf_from_u32(len + (uint32_t)argc));
}

static jv arrp_pop(js_vm_t *vm, jv thisv, jv *argv, int argc, uint32_t m) {
    jv o, v;
    uint32_t len;
    (void)argv; (void)argc; (void)m;
    if (!arr_this(vm, thisv, &o, &len)) return jv_undef();
    if (len == 0) { set_length(vm, o, 0); return jv_undef(); }
    v = elem(vm, o, len - 1);
    set_length(vm, o, len - 1);
    return v;
}

static jv arrp_shift(js_vm_t *vm, jv thisv, jv *argv, int argc, uint32_t m) {
    jv o, first;
    uint32_t len, i;
    (void)argv; (void)argc; (void)m;
    if (!arr_this(vm, thisv, &o, &len)) return jv_undef();
    if (len == 0) return jv_undef();
    first = elem(vm, o, 0);
    if (JOBJ(vm, o.a)->cls == JS_CLASS_ARRAY && !(JOBJ(vm, o.a)->oflags & O_FROZEN)) {
        jv *a = (jv *)JPTR(vm, JOBJ(vm, o.a)->arr);
        for (i = 1; i < len; i++) a[i - 1] = a[i];
    } else {
        for (i = 1; i < len && !vm->ctl; i++) set_elem(vm, o, i - 1, elem(vm, o, i));
    }
    set_length(vm, o, len - 1);
    return first;
}

static jv arrp_unshift(js_vm_t *vm, jv thisv, jv *argv, int argc, uint32_t m) {
    jv o;
    uint32_t len, i, n = (uint32_t)argc;
    (void)m;
    if (!arr_this(vm, thisv, &o, &len)) return jv_undef();
    if (n == 0) return jv_num(jf_from_u32(len));
    if (JOBJ(vm, o.a)->cls == JS_CLASS_ARRAY && !(JOBJ(vm, o.a)->oflags & O_FROZEN)) {
        jv *a;
        if (!js_array_set_len(vm, o.a, len + n)) return jv_undef();
        a = (jv *)JPTR(vm, JOBJ(vm, o.a)->arr);
        for (i = len; i > 0; i--) a[i - 1 + n] = a[i - 1];
        for (i = 0; i < n; i++) a[i] = argv[i];
    } else {
        for (i = len; i > 0 && !vm->ctl; i--) set_elem(vm, o, i - 1 + n, elem(vm, o, i - 1));
        for (i = 0; i < n; i++) set_elem(vm, o, i, argv[i]);
        set_length(vm, o, len + n);
    }
    return vm->ctl ? jv_undef() : jv_num(jf_from_u32(len + n));
}

static jv arrp_slice(js_vm_t *vm, jv thisv, jv *argv, int argc, uint32_t m) {
    jv o;
    uint32_t len, out, i;
    int from, to;
    (void)m;
    if (!arr_this(vm, thisv, &o, &len)) return jv_undef();
    from = idx_arg(vm, JARG(0), (int)len, 0);
    to = idx_arg(vm, JARG(1), (int)len, (int)len);
    out = js_array_new(vm, to > from ? (uint32_t)(to - from) : 0);
    if (!out) return jv_undef();
    for (i = (uint32_t)from; (int)i < to && !vm->ctl; i++) {
        if (has_elem(vm, o, i)) js_array_set(vm, out, i - (uint32_t)from, elem(vm, o, i));
        else js_array_set_len(vm, out, i - (uint32_t)from + 1);
    }
    return vm->ctl ? jv_undef() : jv_obj(out);
}

static jv arrp_splice(js_vm_t *vm, jv thisv, jv *argv, int argc, uint32_t m) {
    jv o;
    uint32_t len, removed, i, nlen, st, dl, in;
    int start, del, ins = argc > 2 ? argc - 2 : 0;
    (void)m;
    if (!arr_this(vm, thisv, &o, &len)) return jv_undef();
    start = idx_arg(vm, JARG(0), (int)len, 0);
    if (argc == 0) {
        del = 0;
    } else if (argc == 1) {
        del = (int)len - start;
    } else {
        jf64 d = js_to_integer(vm, argv[1]);
        int64_t di = 0;
        if (jf_isinf(d)) di = jf_signbit(d) ? 0 : (int64_t)len;
        else (void)jf_to_i64(d, &di);
        if (di < 0) di = 0;
        if (di > (int64_t)len - start) di = (int64_t)len - start;
        del = (int)di;
    }
    st = (uint32_t)start; dl = (uint32_t)del; in = (uint32_t)ins;
    removed = js_array_new(vm, dl);
    if (!removed) return jv_undef();
    for (i = 0; i < dl; i++) js_array_push(vm, removed, elem(vm, o, st + i));
    nlen = len - dl + in;
    if (in < dl) {
        for (i = st + dl; i < len && !vm->ctl; i++) set_elem(vm, o, i - dl + in, elem(vm, o, i));
        set_length(vm, o, nlen);
    } else if (in > dl) {
        if (JOBJ(vm, o.a)->cls == JS_CLASS_ARRAY && !js_array_set_len(vm, o.a, nlen)) return jv_undef();
        for (i = len; i > st + dl && !vm->ctl; i--) set_elem(vm, o, i - 1 - dl + in, elem(vm, o, i - 1));
    }
    for (i = 0; i < in; i++) set_elem(vm, o, st + i, argv[2 + i]);
    if (JOBJ(vm, o.a)->cls != JS_CLASS_ARRAY) set_length(vm, o, nlen);
    return vm->ctl ? jv_undef() : jv_obj(removed);
}

static jv arrp_concat(js_vm_t *vm, jv thisv, jv *argv, int argc, uint32_t m) {
    uint32_t out = js_array_new(vm, 8), i;
    int a;
    (void)m;
    if (!out) return jv_undef();
    for (a = -1; a < argc && !vm->ctl; a++) {
        jv v = a < 0 ? thisv : argv[a];
        if (js_is_array(vm, v)) {
            uint32_t len = JOBJ(vm, v.a)->alen, base = JOBJ(vm, out)->alen;
            for (i = 0; i < len; i++) {
                jv e = ((jv *)JPTR(vm, JOBJ(vm, v.a)->arr))[i];
                if (e.t == JT_EMPTY) js_array_set_len(vm, out, base + i + 1);
                else js_array_set(vm, out, base + i, e);
            }
        } else {
            js_array_push(vm, out, v);
        }
    }
    return vm->ctl ? jv_undef() : jv_obj(out);
}

static jv arrp_join(js_vm_t *vm, jv thisv, jv *argv, int argc, uint32_t m) {
    jv o;
    uint32_t len, sep, r;
    (void)m;
    if (!arr_this(vm, thisv, &o, &len)) return jv_undef();
    if (vm->depth > JS_MAX_DEPTH - 8) return jv_str(vm->s_empty);   /* an array that contains itself */
    sep = JARG(0).t == JT_UNDEF ? js_intern(vm, ",", 1) : js_to_string(vm, JARG(0));
    if (vm->ctl) return jv_undef();
    vm->depth++;
    r = js_join(vm, o.a, sep);
    vm->depth--;
    return vm->ctl ? jv_undef() : jv_str(r);
}

static jv arrp_reverse(js_vm_t *vm, jv thisv, jv *argv, int argc, uint32_t m) {
    jv o;
    uint32_t len, i;
    (void)argv; (void)argc; (void)m;
    if (!arr_this(vm, thisv, &o, &len)) return jv_undef();
    for (i = 0; i < len / 2 && !vm->ctl; i++) {
        jv a = elem(vm, o, i), b = elem(vm, o, len - 1 - i);
        set_elem(vm, o, i, b);
        set_elem(vm, o, len - 1 - i, a);
    }
    return o;
}

/* indexOf 0, lastIndexOf 1, includes 2 */
static jv arrp_index(js_vm_t *vm, jv thisv, jv *argv, int argc, uint32_t m) {
    jv o, x = JARG(0);
    uint32_t len;
    int i, from;
    if (!arr_this(vm, thisv, &o, &len)) return jv_undef();
    if (m == 1) {
        from = (int)len - 1;
        if (argc > 1) {
            jf64 n = js_to_integer(vm, argv[1]);
            int64_t k = 0;
            if (jf_isinf(n)) k = jf_signbit(n) ? -1 : (int64_t)len - 1;
            else { (void)jf_to_i64(n, &k); if (k < 0) k += len; }
            from = k >= (int64_t)len ? (int)len - 1 : (int)k;
        }
        for (i = from; i >= 0; i--)
            if (has_elem(vm, o, (uint32_t)i) && js_strict_eq(vm, elem(vm, o, (uint32_t)i), x)) return jv_int(i);
        return jv_int(-1);
    }
    from = idx_arg(vm, JARG(1), (int)len, 0);
    for (i = from; i < (int)len && !vm->ctl; i++) {
        jv e = elem(vm, o, (uint32_t)i);
        if (m == 2) { if (js_same_value_zero(vm, e, x)) return jv_bool(1); }
        else if (has_elem(vm, o, (uint32_t)i) && js_strict_eq(vm, e, x)) return jv_int(i);
    }
    return m == 2 ? jv_bool(0) : jv_int(-1);
}

/* forEach 0, map 1, filter 2, some 3, every 4, find 5, findIndex 6, findLast 7, findLastIndex 8 */
static jv arrp_iter(js_vm_t *vm, jv thisv, jv *argv, int argc, uint32_t m) {
    jv o, fn, t = JARG(1);
    uint32_t len, out = 0, i;
    if (!arr_this(vm, thisv, &o, &len)) return jv_undef();
    fn = callback_arg(vm, argv, argc, 0);
    if (vm->ctl) return jv_undef();
    if (m == 1) { out = js_array_new(vm, len); if (!out || !js_array_set_len(vm, out, len)) return jv_undef(); }
    if (m == 2) { out = js_array_new(vm, 4); if (!out) return jv_undef(); }
    for (i = 0; i < len && !vm->ctl; i++) {
        uint32_t k = (m == 7 || m == 8) ? len - 1 - i : i;
        jv a3[3], r, e;
        if (m <= 4 && !has_elem(vm, o, k)) continue;
        e = elem(vm, o, k);
        a3[0] = e;
        a3[1] = jv_int((int32_t)k);
        a3[2] = o;
        r = js_call(vm, fn, t, a3, 3);
        if (vm->ctl) return jv_undef();
        switch (m) {
        case 1: js_array_set(vm, out, k, r); break;
        case 2: if (js_truthy(vm, r)) js_array_push(vm, out, e); break;
        case 3: if (js_truthy(vm, r)) return jv_bool(1); break;
        case 4: if (!js_truthy(vm, r)) return jv_bool(0); break;
        case 5: case 7: if (js_truthy(vm, r)) return e; break;
        case 6: case 8: if (js_truthy(vm, r)) return jv_int((int32_t)k); break;
        default: break;
        }
    }
    if (vm->ctl) return jv_undef();
    switch (m) {
    case 1: case 2: return jv_obj(out);
    case 3: return jv_bool(0);
    case 4: return jv_bool(1);
    case 6: case 8: return jv_int(-1);
    default: return jv_undef();
    }
}

/* reduce 0, reduceRight 1 */
static jv arrp_reduce(js_vm_t *vm, jv thisv, jv *argv, int argc, uint32_t m) {
    jv o, fn, acc = jv_undef();
    uint32_t len, i;
    int have = argc > 1;
    if (!arr_this(vm, thisv, &o, &len)) return jv_undef();
    fn = callback_arg(vm, argv, argc, 0);
    if (vm->ctl) return jv_undef();
    if (have) acc = argv[1];
    for (i = 0; i < len && !vm->ctl; i++) {
        uint32_t k = m ? len - 1 - i : i;
        jv a4[4];
        if (!has_elem(vm, o, k)) continue;
        if (!have) { acc = elem(vm, o, k); have = 1; continue; }
        a4[0] = acc;
        a4[1] = elem(vm, o, k);
        a4[2] = jv_int((int32_t)k);
        a4[3] = o;
        acc = js_call(vm, fn, jv_undef(), a4, 4);
    }
    if (!have && !vm->ctl) return js_throw_err(vm, JE_TYPE, "Reduce of empty array with no initial value");
    return vm->ctl ? jv_undef() : acc;
}

static int sort_cmp(js_vm_t *vm, jv fn, jv a, jv b) {
    uint32_t sa, sb;
    if (a.t == JT_UNDEF) return b.t == JT_UNDEF ? 0 : 1;
    if (b.t == JT_UNDEF) return -1;
    if (fn.t != JT_UNDEF) {
        jv a2[2], r;
        jf64 n;
        a2[0] = a;
        a2[1] = b;
        r = js_call(vm, fn, jv_undef(), a2, 2);
        if (vm->ctl) return 0;
        if (r.t == JT_INT) return (int32_t)r.a < 0 ? -1 : (int32_t)r.a > 0 ? 1 : 0;
        n = js_to_number(vm, r);
        if (jf_isnan(n) || jf_iszero(n)) return 0;
        return jf_signbit(n) ? -1 : 1;
    }
    sa = js_to_string(vm, a);     /* the default order compares strings: "10" < "9" */
    if (vm->ctl) return 0;
    sb = js_to_string(vm, b);
    if (vm->ctl) return 0;
    return js_str_cmp(vm, sa, sb);
}

/* a stable bottom-up merge sort of a[0..n) with tmp of the same size */
static void merge_sort(js_vm_t *vm, jv fn, uint32_t abuf, uint32_t tbuf, uint32_t n) {
    uint32_t width, i;
    for (width = 1; width < n && !vm->ctl; width *= 2) {
        for (i = 0; i < n && !vm->ctl; i += 2 * width) {
            uint32_t mid = i + width < n ? i + width : n, hi = i + 2 * width < n ? i + 2 * width : n;
            uint32_t l = i, r = mid, k = i;
            while (l < mid && r < hi) {
                jv *a = (jv *)JPTR(vm, abuf), *t = (jv *)JPTR(vm, tbuf);
                jv x = a[l], y = a[r];
                int c = sort_cmp(vm, fn, y, x);
                if (vm->ctl) return;
                t = (jv *)JPTR(vm, tbuf);
                if (c < 0) { t[k++] = y; r++; }
                else { t[k++] = x; l++; }
            }
            {
                jv *a = (jv *)JPTR(vm, abuf), *t = (jv *)JPTR(vm, tbuf);
                while (l < mid) t[k++] = a[l++];
                while (r < hi) t[k++] = a[r++];
            }
        }
        {
            jv *a = (jv *)JPTR(vm, abuf), *t = (jv *)JPTR(vm, tbuf);
            for (i = 0; i < n; i++) a[i] = t[i];
        }
    }
}

static jv arrp_sort(js_vm_t *vm, jv thisv, jv *argv, int argc, uint32_t m) {
    jv o, fn = JARG(0);
    uint32_t len, i, buf, tmp, n = 0;
    (void)m;
    if (fn.t != JT_UNDEF && !js_is_callable(vm, fn)) return js_throw_err(vm, JE_TYPE, "The comparison function must be either a function or undefined");
    if (!arr_this(vm, thisv, &o, &len)) return jv_undef();
    if (len < 2) return o;
    buf = js_alloc(vm, len * (uint32_t)sizeof(jv));
    tmp = buf ? js_alloc(vm, len * (uint32_t)sizeof(jv)) : 0;
    if (!tmp) return jv_undef();
    for (i = 0; i < len && !vm->ctl; i++) {
        if (has_elem(vm, o, i)) {
            jv e = elem(vm, o, i);
            ((jv *)JPTR(vm, buf))[n++] = e;
        }
    }
    merge_sort(vm, fn, buf, tmp, n);
    if (vm->ctl) return jv_undef();
    for (i = 0; i < n; i++) set_elem(vm, o, i, ((jv *)JPTR(vm, buf))[i]);
    for (; i < len; i++) {
        if (JOBJ(vm, o.a)->cls == JS_CLASS_ARRAY) ((jv *)JPTR(vm, JOBJ(vm, o.a)->arr))[i] = jv_empty();
        else js_delete(vm, o, jv_int((int32_t)i));
    }
    return o;
}

static jv arrp_fill(js_vm_t *vm, jv thisv, jv *argv, int argc, uint32_t m) {
    jv o;
    uint32_t len;
    int from, to, i;
    (void)m;
    if (!arr_this(vm, thisv, &o, &len)) return jv_undef();
    from = idx_arg(vm, JARG(1), (int)len, 0);
    to = idx_arg(vm, JARG(2), (int)len, (int)len);
    for (i = from; i < to && !vm->ctl; i++) set_elem(vm, o, (uint32_t)i, JARG(0));
    return o;
}

static void flat_into(js_vm_t *vm, uint32_t out, jv src, int depth) {
    uint32_t len, i;
    js_length_of(vm, src, &len);
    for (i = 0; i < len && !vm->ctl; i++) {
        jv e;
        if (!has_elem(vm, src, i)) continue;
        e = elem(vm, src, i);
        if (depth > 0 && js_is_array(vm, e)) flat_into(vm, out, e, depth - 1);
        else js_array_push(vm, out, e);
    }
}

/* flat 0, flatMap 1 */
static jv arrp_flat(js_vm_t *vm, jv thisv, jv *argv, int argc, uint32_t m) {
    jv o;
    uint32_t len, out, i;
    int depth = 1;
    if (!arr_this(vm, thisv, &o, &len)) return jv_undef();
    out = js_array_new(vm, len);
    if (!out) return jv_undef();
    if (m == 1) {
        jv fn = callback_arg(vm, argv, argc, 0);
        if (vm->ctl) return jv_undef();
        for (i = 0; i < len && !vm->ctl; i++) {
            jv a3[3], r;
            a3[0] = elem(vm, o, i);
            a3[1] = jv_int((int32_t)i);
            a3[2] = o;
            r = js_call(vm, fn, JARG(1), a3, 3);
            if (vm->ctl) return jv_undef();
            if (js_is_array(vm, r)) flat_into(vm, out, r, 0);
            else js_array_push(vm, out, r);
        }
        return jv_obj(out);
    }
    if (JARG(0).t != JT_UNDEF) {
        jf64 d = js_to_integer(vm, JARG(0));
        int64_t di = 0;
        if (jf_isinf(d)) di = jf_signbit(d) ? 0 : 32;
        else (void)jf_to_i64(d, &di);
        depth = di < 0 ? 0 : di > 32 ? 32 : (int)di;
    }
    flat_into(vm, out, o, depth);
    return vm->ctl ? jv_undef() : jv_obj(out);
}

/* keys 0, values 1, entries 2: arrays (the guest has no iterator objects) */
static jv arrp_keys(js_vm_t *vm, jv thisv, jv *argv, int argc, uint32_t m) {
    jv o;
    uint32_t len, out, i;
    (void)argv; (void)argc;
    if (!arr_this(vm, thisv, &o, &len)) return jv_undef();
    out = js_array_new(vm, len);
    if (!out) return jv_undef();
    for (i = 0; i < len && !vm->ctl; i++) {
        if (m == 0) {
            js_array_push(vm, out, jv_int((int32_t)i));
        } else if (m == 1) {
            js_array_push(vm, out, elem(vm, o, i));
        } else {
            uint32_t pair = js_array_new(vm, 2);
            if (!pair) return jv_undef();
            js_array_push(vm, pair, jv_int((int32_t)i));
            js_array_push(vm, pair, elem(vm, o, i));
            js_array_push(vm, out, jv_obj(pair));
        }
    }
    return jv_obj(out);
}

static jv arrp_at(js_vm_t *vm, jv thisv, jv *argv, int argc, uint32_t m) {
    jv o;
    uint32_t len;
    jf64 n;
    int64_t k = 0;
    (void)m;
    if (!arr_this(vm, thisv, &o, &len)) return jv_undef();
    n = js_to_integer(vm, JARG(0));
    if (jf_isinf(n)) return jv_undef();
    (void)jf_to_i64(n, &k);
    if (k < 0) k += len;
    if (k < 0 || k >= (int64_t)len) return jv_undef();
    return elem(vm, o, (uint32_t)k);
}

static jv arrp_to_string(js_vm_t *vm, jv thisv, jv *argv, int argc, uint32_t m) {
    (void)argv; (void)argc; (void)m;
    if (thisv.t != JT_OBJ) return objp_to_string(vm, thisv, 0, 0, 0);
    return arrp_join(vm, thisv, 0, 0, 0);
}

static jv arr_of(js_vm_t *vm, jv thisv, jv *argv, int argc, uint32_t m) {
    uint32_t a = js_array_new(vm, (uint32_t)argc);
    int i;
    (void)thisv; (void)m;
    if (!a) return jv_undef();
    for (i = 0; i < argc; i++) js_array_push(vm, a, argv[i]);
    return jv_obj(a);
}

/* ---- Error ---- */

static jv err_ctor(js_vm_t *vm, jv thisv, jv *argv, int argc, uint32_t kind) {
    uint32_t e;
    jv msg = JARG(0);
    (void)thisv;
    if (vm->super_this && JOBJ(vm, vm->super_this)->cls == JS_CLASS_OBJECT) {
        e = vm->super_this;       /* class MyError extends Error: initialize that this */
        JOBJ(vm, e)->cls = JS_CLASS_ERROR;
        vm->super_this = 0;
    } else {
        e = js_obj_new(vm, JS_CLASS_ERROR, vm->p_err[kind]);
    }
    if (!e) return jv_undef();
    if (msg.t != JT_UNDEF) {
        uint32_t s = js_to_string(vm, msg);
        if (vm->ctl) return jv_undef();
        js_define(vm, e, vm->s_message, jv_str(s), P_HIDDEN);
    }
    if (argc > 1 && argv[1].t == JT_OBJ && js_has(vm, argv[1].a, vm->s_cause)) {
        jv c = js_get(vm, argv[1], vm->s_cause);
        if (vm->ctl) return jv_undef();
        js_define(vm, e, vm->s_cause, c, P_HIDDEN);
    }
    return jv_obj(e);
}

static jv errp_to_string(js_vm_t *vm, jv thisv, jv *argv, int argc, uint32_t m) {
    jv n, msg;
    uint32_t ns, ms;
    (void)argv; (void)argc; (void)m;
    if (thisv.t != JT_OBJ) return js_throw_err(vm, JE_TYPE, "Error.prototype.toString called on non-object");
    n = js_get(vm, thisv, vm->s_name);
    if (vm->ctl) return jv_undef();
    msg = js_get(vm, thisv, vm->s_message);
    if (vm->ctl) return jv_undef();
    ns = n.t == JT_UNDEF ? js_intern(vm, "Error", 5) : js_to_string(vm, n);
    if (vm->ctl) return jv_undef();
    ms = msg.t == JT_UNDEF ? vm->s_empty : js_to_string(vm, msg);
    if (vm->ctl) return jv_undef();
    if (JSTR(vm, ns)->len == 0) return jv_str(ms);
    if (JSTR(vm, ms)->len == 0) return jv_str(ns);
    return jv_str(js_str_cat(vm, js_str_cat(vm, ns, js_str_c(vm, ": ")), ms));
}

static jv errp_stack(js_vm_t *vm, jv thisv, jv *argv, int argc, uint32_t m) {
    jv s = errp_to_string(vm, thisv, argv, argc, m);
    if (vm->ctl) return jv_undef();
    return jv_str(js_str_cat(vm, s.a, js_str_c(vm, "\n    at <guest script>")));
}

/* ---- console ---- */

static jv con_log(js_vm_t *vm, jv thisv, jv *argv, int argc, uint32_t level) {
    static const char *const pre[] = { "", "warn: ", "error: ", "info: ", "debug: " };
    char b[512];
    int i;
    (void)thisv;
    if (level < 5 && pre[level][0]) js_log_put(vm, pre[level], cslen(pre[level]));
    for (i = 0; i < argc; i++) {
        int n = js_display(vm, argv[i], b, (int)sizeof(b), 0);
        if (vm->ctl) return jv_undef();
        if (i) js_log_put(vm, " ", 1);
        js_log_put(vm, b, n);
    }
    js_log_put(vm, "\n", 1);
    return jv_undef();
}

/* ---- global functions ---- */

static int is_ws(char c) { return c == ' ' || c == '\t' || c == '\n' || c == '\r' || c == '\f' || c == '\v'; }

static jv g_parse_int(js_vm_t *vm, jv thisv, jv *argv, int argc, uint32_t m) {
    uint32_t s;
    js_str_t *p;
    int i = 0, n, neg = 0, radix = 0, any = 0;
    jf64 acc = 0;
    (void)thisv; (void)m;
    s = js_to_string(vm, JARG(0));
    if (vm->ctl) return jv_undef();
    if (argc > 1) radix = js_to_int32(vm, argv[1]);
    if (vm->ctl) return jv_undef();
    p = JSTR(vm, s);
    n = (int)p->len;
    while (i < n && is_ws(p->s[i])) i++;
    if (i < n && (p->s[i] == '-' || p->s[i] == '+')) { neg = p->s[i] == '-'; i++; }
    if ((radix == 0 || radix == 16) && i + 1 < n && p->s[i] == '0' && (p->s[i + 1] == 'x' || p->s[i + 1] == 'X')) {
        i += 2;
        radix = 16;
    }
    if (radix == 0) radix = 10;
    if (radix < 2 || radix > 36) return jv_num(JF_NAN);
    if (radix == 10) {    /* decimal: correctly rounded through jf_parse of the digit run */
        int st = i, used;
        while (i < n && p->s[i] >= '0' && p->s[i] <= '9') i++;
        if (i == st) return jv_num(JF_NAN);
        acc = jf_parse(p->s + st, i - st, &used);
        return jv_num(neg ? jf_neg(acc) : acc);
    }
    for (; i < n; i++) {
        char c = p->s[i];
        int d = (c >= '0' && c <= '9') ? c - '0' : (c >= 'a' && c <= 'z') ? c - 'a' + 10 : (c >= 'A' && c <= 'Z') ? c - 'A' + 10 : 99;
        if (d >= radix) break;
        acc = jf_add(jf_mul(acc, jf_from_i32(radix)), jf_from_i32(d));
        any = 1;
    }
    if (!any) return jv_num(JF_NAN);
    return jv_num(neg ? jf_neg(acc) : acc);
}

static jv g_parse_float(js_vm_t *vm, jv thisv, jv *argv, int argc, uint32_t m) {
    uint32_t s;
    js_str_t *p;
    int i = 0, used = 0, neg = 0, k;
    jf64 v;
    (void)thisv; (void)m;
    s = js_to_string(vm, JARG(0));
    if (vm->ctl) return jv_undef();
    p = JSTR(vm, s);
    while (i < (int)p->len && is_ws(p->s[i])) i++;
    k = i;
    if (k < (int)p->len && (p->s[k] == '-' || p->s[k] == '+')) { neg = p->s[k] == '-'; k++; }
    if ((int)p->len - k >= 8) {
        const char *inf = "Infinity";
        int j;
        for (j = 0; j < 8 && p->s[k + j] == inf[j]; j++) {}
        if (j == 8) return jv_num(neg ? jf_neg(JF_INF) : JF_INF);
    }
    v = jf_parse(p->s + i, (int)p->len - i, &used);
    if (!used) return jv_num(JF_NAN);
    return jv_num(v);
}

/* isNaN 0, isFinite 1 */
static jv g_is_nan(js_vm_t *vm, jv thisv, jv *argv, int argc, uint32_t m) {
    jf64 n = js_to_number(vm, JARG(0));
    (void)thisv;
    if (vm->ctl) return jv_undef();
    return jv_bool(m == 0 ? jf_isnan(n) : jf_isfinite(n));
}

static const char k_hex[] = "0123456789ABCDEF";

/* encodeURIComponent 0, encodeURI 1 */
static jv g_encode(js_vm_t *vm, jv thisv, jv *argv, int argc, uint32_t m) {
    const char *keep = m ? "-_.!~*'();/?:@&=+$,#" : "-_.!~*'()";
    uint32_t s, out;
    char b[256];
    int n = 0, i, len;
    (void)thisv;
    s = js_to_string(vm, JARG(0));
    if (vm->ctl) return jv_undef();
    out = vm->s_empty;
    len = (int)JSTR(vm, s)->len;
    for (i = 0; i < len; i++) {
        uint8_t c = (uint8_t)JSTR(vm, s)->s[i];
        int k, ok = (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') || (c >= '0' && c <= '9');
        for (k = 0; !ok && keep[k]; k++) if (keep[k] == (char)c) ok = 1;
        if (n > (int)sizeof(b) - 4) { out = js_str_cat(vm, out, js_str_new(vm, b, n)); n = 0; }
        if (ok) {
            b[n++] = (char)c;
        } else {
            b[n++] = '%';
            b[n++] = k_hex[c >> 4];
            b[n++] = k_hex[c & 15];
        }
    }
    return jv_str(js_str_cat(vm, out, js_str_new(vm, b, n)));
}

static int hexv(int c) {
    if (c >= '0' && c <= '9') return c - '0';
    if (c >= 'a' && c <= 'f') return c - 'a' + 10;
    if (c >= 'A' && c <= 'F') return c - 'A' + 10;
    return -1;
}

/* decodeURIComponent 0, decodeURI 1 (keeps escapes of reserved characters) */
static jv g_decode(js_vm_t *vm, jv thisv, jv *argv, int argc, uint32_t m) {
    uint32_t s, out;
    char b[256];
    int n = 0, i, len;
    (void)thisv;
    s = js_to_string(vm, JARG(0));
    if (vm->ctl) return jv_undef();
    out = vm->s_empty;
    len = (int)JSTR(vm, s)->len;
    for (i = 0; i < len; i++) {
        const char *p = JSTR(vm, s)->s;
        char c = p[i];
        if (n > (int)sizeof(b) - 4) { out = js_str_cat(vm, out, js_str_new(vm, b, n)); n = 0; p = JSTR(vm, s)->s; }
        if (c == '%') {
            int h1 = i + 2 < len ? hexv(p[i + 1]) : -1, h2 = i + 2 < len ? hexv(p[i + 2]) : -1;
            char d;
            if (h1 < 0 || h2 < 0) return js_throw_err(vm, JE_URI, "URI malformed");
            d = (char)(h1 * 16 + h2);
            if (m == 1 && (d == ';' || d == '/' || d == '?' || d == ':' || d == '@' || d == '&' || d == '=' ||
                           d == '+' || d == '$' || d == ',' || d == '#')) {
                b[n++] = c;
                b[n++] = p[i + 1];
                b[n++] = p[i + 2];
            } else {
                b[n++] = d;
            }
            i += 2;
        } else {
            b[n++] = c;
        }
    }
    return jv_str(js_str_cat(vm, out, js_str_new(vm, b, n)));
}

/* ---- display (console.log and the js command) ---- */

typedef struct { char *out; int n, max; } dsp_t;

static void dput(dsp_t *d, const char *s, int n) {
    int i;
    for (i = 0; i < n; i++) {
        if (d->n < d->max - 1) d->out[d->n] = s[i];
        d->n++;
    }
}
static void dputs(dsp_t *d, const char *s) { dput(d, s, cslen(s)); }

static void dnum(dsp_t *d, uint32_t v) {
    char b[12];
    int n = 0;
    uint32_t dv = 1;
    while (dv <= v / 10) dv *= 10;
    while (dv) { b[n++] = (char)('0' + (v / dv) % 10); dv /= 10; }
    dput(d, b, n);
}

static void dstr(js_vm_t *vm, dsp_t *d, uint32_t s, int quote) {
    js_str_t *p = JSTR(vm, s);
    if (quote) dput(d, "'", 1);
    dput(d, p->s, (int)p->len);
    if (quote) dput(d, "'", 1);
}

static int ident_like(js_vm_t *vm, uint32_t s) {
    js_str_t *p = JSTR(vm, s);
    uint32_t i;
    if (!p->len) return 0;
    for (i = 0; i < p->len; i++) {
        char c = p->s[i];
        if (!((c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') || c == '_' || c == '$' || (i && c >= '0' && c <= '9'))) return 0;
    }
    return 1;
}

static void disp(js_vm_t *vm, dsp_t *d, jv v, int quote, int depth);

static void disp_array(js_vm_t *vm, dsp_t *d, jv v, int depth) {
    uint32_t len = JOBJ(vm, v.a)->alen, holes = 0, i;
    int first = 1;
    if (len == 0) { dputs(d, "[]"); return; }
    dputs(d, "[ ");
    for (i = 0; i < len && d->n < d->max; i++) {
        jv e = ((jv *)JPTR(vm, JOBJ(vm, v.a)->arr))[i];
        if (e.t == JT_EMPTY) { holes++; continue; }
        if (holes) {
            if (!first) dputs(d, ", ");
            dputs(d, "<");
            dnum(d, holes);
            dputs(d, holes == 1 ? " empty item>" : " empty items>");
            holes = 0;
            first = 0;
        }
        if (!first) dputs(d, ", ");
        first = 0;
        disp(vm, d, e, 1, depth + 1);
        if (vm->ctl) return;
    }
    if (holes) {
        if (!first) dputs(d, ", ");
        dputs(d, "<");
        dnum(d, holes);
        dputs(d, holes == 1 ? " empty item>" : " empty items>");
    }
    dputs(d, " ]");
}

static void disp_object(js_vm_t *vm, dsp_t *d, jv v, int depth) {
    uint32_t keys = js_own_keys(vm, v.a, 1), i;
    int first = 1;
    if (!keys) return;
    if (JOBJ(vm, keys)->alen == 0) { dputs(d, "{}"); return; }
    dputs(d, "{ ");
    for (i = 0; i < JOBJ(vm, keys)->alen && d->n < d->max; i++) {
        jv k = js_array_get(vm, keys, i);
        js_prop_t *p = js_own_prop(vm, v.a, k.a);
        if (!first) dputs(d, ", ");
        first = 0;
        dstr(vm, d, k.a, !ident_like(vm, k.a));
        dputs(d, ": ");
        if (p && (p->flags & P_ACCESSOR)) dputs(d, "[Getter/Setter]");
        else disp(vm, d, p ? p->v : js_get(vm, v, k.a), 1, depth + 1);
        if (vm->ctl) return;
    }
    dputs(d, " }");
}

static void disp(js_vm_t *vm, dsp_t *d, jv v, int quote, int depth) {
    js_obj_t *o;
    if (d->n >= d->max) return;
    switch (v.t) {
    case JT_UNDEF: case JT_EMPTY: dputs(d, "undefined"); return;
    case JT_NULL: dputs(d, "null"); return;
    case JT_BOOL: dputs(d, v.a ? "true" : "false"); return;
    case JT_INT: case JT_NUM: {
        char b[40];
        int n;
        if (v.t == JT_NUM && jf_iszero(jv_f64(v)) && jf_signbit(jv_f64(v))) { dputs(d, "-0"); return; }
        n = jf_format(jv_f64(v), b, (int)sizeof(b));
        dput(d, b, n);
        return;
    }
    case JT_STR: dstr(vm, d, v.a, quote); return;
    default: break;
    }
    o = JOBJ(vm, v.a);
    switch (o->cls) {
    case JS_CLASS_FUNC: case JS_CLASS_NATIVE: case JS_CLASS_BOUND: {
        int is_class = o->cls == JS_CLASS_FUNC && (FN_FLAGS(o) & FN_CLASSCTOR);
        jv nm = js_get(vm, v, vm->s_name);
        if (vm->ctl) return;
        dputs(d, is_class ? "[class " : "[Function");
        if (nm.t == JT_STR && JSTR(vm, nm.a)->len) {
            if (!is_class) dputs(d, ": ");
            dstr(vm, d, nm.a, 0);
        } else if (is_class) {
            dputs(d, "(anonymous)");
        } else {
            dputs(d, " (anonymous)");
        }
        dputs(d, "]");
        return;
    }
    case JS_CLASS_ERROR: {
        jv s = errp_to_string(vm, v, 0, 0, 0);
        if (vm->ctl) return;
        dstr(vm, d, s.a, 0);
        return;
    }
    case JS_CLASS_REGEXP: case JS_CLASS_DATE: {
        jv f = js_get(vm, v, o->cls == JS_CLASS_DATE ? KEY(vm, "toISOString") : vm->s_toString), s;
        if (vm->ctl || !js_is_callable(vm, f)) return;
        s = js_call(vm, f, v, 0, 0);
        if (vm->ctl == CTL_THROW) { vm->ctl = CTL_NONE; dputs(d, "Invalid Date"); return; }
        if (s.t == JT_STR) dstr(vm, d, s.a, 0);
        return;
    }
    case JS_CLASS_BOOLEAN: case JS_CLASS_NUMBER: case JS_CLASS_STRING: {
        jv p = jv_undef();
        p.t = (uint8_t)o->x0;
        p.a = o->x1;
        p.b = o->x2;
        dputs(d, o->cls == JS_CLASS_STRING ? "[String: " : o->cls == JS_CLASS_NUMBER ? "[Number: " : "[Boolean: ");
        disp(vm, d, p, 1, depth + 1);
        dputs(d, "]");
        return;
    }
    default: break;
    }
    if (depth > 2) {
        dputs(d, o->cls == JS_CLASS_ARRAY ? "[Array]" : "[Object]");
        return;
    }
    if (o->cls == JS_CLASS_ARRAY || o->cls == JS_CLASS_ARGS) { disp_array(vm, d, v, depth); return; }
    if (o->cls == JS_CLASS_MAP || o->cls == JS_CLASS_SET) {
        uint32_t arr = 0;
        js_iter_values(vm, v, &arr);
        if (vm->ctl) return;
        dputs(d, o->cls == JS_CLASS_MAP ? "Map(" : "Set(");
        dnum(d, JOBJ(vm, arr)->alen);
        dputs(d, ") ");
        disp_array(vm, d, jv_obj(arr), depth);
        return;
    }
    if (o->cls == JS_CLASS_HOST) {
        jv t = js_get(vm, v, KEY(vm, "tagName"));
        if (vm->ctl == CTL_THROW) { vm->ctl = CTL_NONE; t = jv_undef(); }
        if (vm->ctl) return;
        if (t.t == JT_STR) {
            dputs(d, "<");
            dstr(vm, d, t.a, 0);
            dputs(d, ">");
            return;
        }
    }
    disp_object(vm, d, v, depth);
}

int js_display(js_vm_t *vm, jv v, char *out, int max, int quote) {
    dsp_t d;
    if (max <= 0) return 0;
    d.out = out;
    d.n = 0;
    d.max = max;
    disp(vm, &d, v, quote, 0);
    if (d.n >= max) {
        d.n = max - 1;
        if (max > 4) {
            out[max - 4] = '.';
            out[max - 3] = '.';
            out[max - 2] = '.';
        }
    }
    out[d.n] = 0;
    return d.n;
}

int js_to_cstr(js_vm_t *vm, jv v, char *out, int max) {
    uint32_t s;
    js_str_t *p;
    int i;
    if (max <= 0) return 0;
    s = js_to_string(vm, v);
    if (vm->ctl) { out[0] = 0; return 0; }
    p = JSTR(vm, s);
    for (i = 0; i < (int)p->len && i < max - 1; i++) out[i] = p->s[i];
    out[i] = 0;
    return i;
}

/* ---- realm setup ---- */

static void def_list(js_vm_t *vm, uint32_t o, const char *const *names, js_native_fn fn, int nargs) {
    int i;
    for (i = 0; names[i]; i++) js_def_native(vm, o, names[i], fn, nargs, (uint32_t)i);
}

static int lib_object(js_vm_t *vm) {
    static const char *const keys[] = { "keys", "values", "entries", "getOwnPropertyNames", 0 };
    uint32_t c = js_ctor_new(vm, "Object", obj_ctor, 1, vm->p_object), o = vm->p_object;
    vm->c_object = c;
    def_list(vm, c, keys, obj_keys, 1);
    js_def_native(vm, c, "freeze", obj_freeze, 1, 0);
    js_def_native(vm, c, "seal", obj_freeze, 1, 1);
    js_def_native(vm, c, "preventExtensions", obj_freeze, 1, 1);
    js_def_native(vm, c, "isFrozen", obj_freeze, 1, 3);
    js_def_native(vm, c, "isSealed", obj_freeze, 1, 3);
    js_def_native(vm, c, "isExtensible", obj_freeze, 1, 4);
    js_def_native(vm, c, "assign", obj_assign, 2, 0);
    js_def_native(vm, c, "create", obj_create, 2, 0);
    js_def_native(vm, c, "defineProperty", obj_define, 3, 0);
    js_def_native(vm, c, "defineProperties", obj_define, 2, 1);
    js_def_native(vm, c, "getPrototypeOf", obj_proto_of, 1, 0);
    js_def_native(vm, c, "setPrototypeOf", obj_proto_of, 2, 1);
    js_def_native(vm, c, "getOwnPropertyDescriptor", obj_get_desc, 2, 0);
    js_def_native(vm, c, "is", obj_is, 2, 0);
    js_def_native(vm, c, "fromEntries", obj_from_entries, 1, 0);
    js_def_native(vm, c, "hasOwn", objp_has_own, 2, 1);
    js_def_native(vm, o, "hasOwnProperty", objp_has_own, 1, 0);
    js_def_native(vm, o, "isPrototypeOf", objp_is_proto, 1, 0);
    js_def_native(vm, o, "propertyIsEnumerable", objp_enum, 1, 0);
    js_def_native(vm, o, "toString", objp_to_string, 0, 0);
    js_def_native(vm, o, "toLocaleString", objp_to_locale, 0, 0);
    js_def_native(vm, o, "valueOf", objp_value_of, 0, 0);
    return c ? 0 : -1;
}

static int lib_function(js_vm_t *vm) {
    uint32_t c = js_ctor_new(vm, "Function", fn_ctor, 1, vm->p_function), o = vm->p_function;
    vm->c_function = c;
    js_def_native(vm, o, "call", fnp_call, 1, 0);
    js_def_native(vm, o, "apply", fnp_apply, 2, 0);
    js_def_native(vm, o, "bind", fnp_bind, 1, 0);
    js_def_native(vm, o, "toString", fnp_to_string, 0, 0);
    return c ? 0 : -1;
}

static int lib_array(js_vm_t *vm) {
    static const char *const idx[] = { "indexOf", "lastIndexOf", "includes", 0 };
    static const char *const it[] = { "forEach", "map", "filter", "some", "every", "find", "findIndex", "findLast",
                                      "findLastIndex", 0 };
    static const char *const kv[] = { "keys", "values", "entries", 0 };
    uint32_t c = js_ctor_new(vm, "Array", arr_ctor, 1, vm->p_array), o = vm->p_array;
    vm->c_array = c;
    js_def_native(vm, c, "isArray", arr_is_array, 1, 0);
    js_def_native(vm, c, "of", arr_of, 0, 0);
    js_def_native(vm, c, "from", arr_from, 1, 0);
    js_def_native(vm, o, "push", arrp_push, 1, 0);
    js_def_native(vm, o, "pop", arrp_pop, 0, 0);
    js_def_native(vm, o, "shift", arrp_shift, 0, 0);
    js_def_native(vm, o, "unshift", arrp_unshift, 1, 0);
    js_def_native(vm, o, "slice", arrp_slice, 2, 0);
    js_def_native(vm, o, "splice", arrp_splice, 2, 0);
    js_def_native(vm, o, "concat", arrp_concat, 1, 0);
    js_def_native(vm, o, "join", arrp_join, 1, 0);
    js_def_native(vm, o, "reverse", arrp_reverse, 0, 0);
    def_list(vm, o, idx, arrp_index, 1);
    def_list(vm, o, it, arrp_iter, 1);
    js_def_native(vm, o, "reduce", arrp_reduce, 1, 0);
    js_def_native(vm, o, "reduceRight", arrp_reduce, 1, 1);
    js_def_native(vm, o, "sort", arrp_sort, 1, 0);
    js_def_native(vm, o, "fill", arrp_fill, 1, 0);
    js_def_native(vm, o, "flat", arrp_flat, 0, 0);
    js_def_native(vm, o, "flatMap", arrp_flat, 1, 1);
    def_list(vm, o, kv, arrp_keys, 0);
    js_def_native(vm, o, "at", arrp_at, 1, 0);
    js_def_native(vm, o, "toString", arrp_to_string, 0, 0);
    return c ? 0 : -1;
}

static int lib_errors(js_vm_t *vm) {
    static const char *const names[JE_COUNT] = { "Error", "TypeError", "RangeError", "SyntaxError", "ReferenceError",
                                                 "URIError", "EvalError" };
    uint32_t base = 0, sg;
    int k;
    for (k = 0; k < JE_COUNT; k++) {
        uint32_t ec = js_native_new(vm, names[k], err_ctor, 1, (uint32_t)k);
        if (!ec) return -1;
        js_define(vm, ec, vm->s_prototype, jv_obj(vm->p_err[k]), 0);
        js_define(vm, vm->p_err[k], vm->s_constructor, jv_obj(ec), P_HIDDEN);
        js_define(vm, vm->p_err[k], vm->s_name, jv_str(JOBJ(vm, ec)->x2), P_HIDDEN);
        js_define(vm, vm->p_err[k], vm->s_message, jv_str(vm->s_empty), P_HIDDEN);
        js_define(vm, vm->global, JOBJ(vm, ec)->x2, jv_obj(ec), P_HIDDEN);
        if (k == 0) base = ec;
        else JOBJ(vm, ec)->proto = base;     /* TypeError.__proto__ === Error */
    }
    js_def_native(vm, vm->p_err[0], "toString", errp_to_string, 0, 0);
    sg = js_native_new(vm, "stack", errp_stack, 0, 0);
    if (!sg) return -1;
    js_define_accessor(vm, vm->p_err[0], vm->s_stack, sg, 0, P_CONF);
    return 0;
}

static int lib_globals(js_vm_t *vm) {
    static const char *const levels[] = { "log", "warn", "error", "info", "debug", 0 };
    uint32_t g = vm->global, con = js_obj_new(vm, JS_CLASS_OBJECT, vm->p_object);
    if (!con) return -1;
    js_define_c(vm, g, "console", jv_obj(con), P_HIDDEN);
    def_list(vm, con, levels, con_log, 0);
    js_def_native(vm, con, "trace", con_log, 0, 4);
    js_def_native(vm, g, "parseInt", g_parse_int, 2, 0);
    js_def_native(vm, g, "parseFloat", g_parse_float, 1, 0);
    js_def_native(vm, g, "isNaN", g_is_nan, 1, 0);
    js_def_native(vm, g, "isFinite", g_is_nan, 1, 1);
    js_def_native(vm, g, "encodeURIComponent", g_encode, 1, 0);
    js_def_native(vm, g, "encodeURI", g_encode, 1, 1);
    js_def_native(vm, g, "decodeURIComponent", g_decode, 1, 0);
    js_def_native(vm, g, "decodeURI", g_decode, 1, 1);
    js_define_c(vm, g, "NaN", jv_num(JF_NAN), 0);
    js_define_c(vm, g, "Infinity", jv_num(JF_INF), 0);
    js_define(vm, g, vm->s_undefined, jv_undef(), 0);
    js_define_c(vm, g, "globalThis", jv_obj(g), P_HIDDEN);
    js_define_c(vm, g, "window", jv_obj(g), P_HIDDEN);
    js_define_c(vm, g, "self", jv_obj(g), P_HIDDEN);
    return 0;
}

int js_lib_init(js_vm_t *vm) {
    if (lib_object(vm) || lib_function(vm) || lib_array(vm) || lib_errors(vm) || lib_globals(vm)) return -1;
    return vm->ctl == CTL_ABORT ? -1 : 0;
}

static void intern_names(js_vm_t *vm) {
    vm->s_empty = js_intern(vm, "", 0);
    vm->s_length = KEY(vm, "length");
    vm->s_prototype = KEY(vm, "prototype");
    vm->s_constructor = KEY(vm, "constructor");
    vm->s_message = KEY(vm, "message");
    vm->s_name = KEY(vm, "name");
    vm->s_toString = KEY(vm, "toString");
    vm->s_valueOf = KEY(vm, "valueOf");
    vm->s_callee = KEY(vm, "callee");
    vm->s_arguments = KEY(vm, "arguments");
    vm->s_get = KEY(vm, "get");
    vm->s_set = KEY(vm, "set");
    vm->s_value = KEY(vm, "value");
    vm->s_writable = KEY(vm, "writable");
    vm->s_enumerable = KEY(vm, "enumerable");
    vm->s_configurable = KEY(vm, "configurable");
    vm->s_index = KEY(vm, "index");
    vm->s_input = KEY(vm, "input");
    vm->s_lastIndex = KEY(vm, "lastIndex");
    vm->s_join = KEY(vm, "join");
    vm->s_stack = KEY(vm, "stack");
    vm->s_cause = KEY(vm, "cause");
    vm->s_default = KEY(vm, "default");
    vm->s_undefined = KEY(vm, "undefined");
    vm->s_toJSON = KEY(vm, "toJSON");
    vm->s_next = KEY(vm, "next");
    vm->s_done = KEY(vm, "done");
    vm->s_raw = KEY(vm, "raw");
    vm->s_source = KEY(vm, "source");
    vm->s_flags = KEY(vm, "flags");
    vm->s_global = KEY(vm, "global");
}

int js_init(js_vm_t *vm, void *mem, uint32_t size) {
    uint32_t i;
    int k;
    for (i = 0; i < (uint32_t)sizeof(*vm); i++) ((char *)vm)[i] = 0;
    if (!mem || size < JS_STACK_SIZE + 256u * 1024u) return -1;
    vm->mem = (uint8_t *)mem;
    vm->size = size;
    vm->limit = size - JS_STACK_SIZE;
    vm->stack_base = vm->mem + vm->limit;
    vm->stack_top = vm->mem + size - 16;
    vm->top = 16;
    vm->budget = JS_DEFAULT_BUDGET;
    vm->rng = 0x2545F491u;
    /* a fixed clock, 2026-01-01T00:00:00Z, until the host sets one */
    vm->date_now_lo = 0x76DAA800u;
    vm->date_now_hi = 0x19Bu;
    vm->itab = js_alloc(vm, JS_ITAB * 4u);
    if (!vm->itab) return -1;
    intern_names(vm);
    vm->p_object = js_obj_new(vm, JS_CLASS_OBJECT, 0);
    vm->p_function = js_obj_new(vm, JS_CLASS_OBJECT, vm->p_object);
    vm->p_array = js_obj_new(vm, JS_CLASS_ARRAY, vm->p_object);
    vm->p_string = js_obj_new(vm, JS_CLASS_STRING, vm->p_object);
    vm->p_number = js_obj_new(vm, JS_CLASS_NUMBER, vm->p_object);
    vm->p_boolean = js_obj_new(vm, JS_CLASS_BOOLEAN, vm->p_object);
    vm->p_regexp = js_obj_new(vm, JS_CLASS_OBJECT, vm->p_object);
    vm->p_date = js_obj_new(vm, JS_CLASS_OBJECT, vm->p_object);
    vm->p_iter = js_obj_new(vm, JS_CLASS_OBJECT, vm->p_object);
    vm->p_map = js_obj_new(vm, JS_CLASS_OBJECT, vm->p_object);
    vm->p_set = js_obj_new(vm, JS_CLASS_OBJECT, vm->p_object);
    vm->p_err[0] = js_obj_new(vm, JS_CLASS_OBJECT, vm->p_object);
    for (k = 1; k < JE_COUNT; k++) vm->p_err[k] = js_obj_new(vm, JS_CLASS_OBJECT, vm->p_err[0]);
    vm->global = js_obj_new(vm, JS_CLASS_OBJECT, vm->p_object);
    if (vm->ctl == CTL_ABORT || !vm->global) return -1;
    JOBJ(vm, vm->p_string)->x0 = JT_STR;
    JOBJ(vm, vm->p_string)->x1 = vm->s_empty;
    JOBJ(vm, vm->p_number)->x0 = JT_INT;
    JOBJ(vm, vm->p_boolean)->x0 = JT_BOOL;
    vm->genv = js_env_new(vm, 0, E_GLOBAL);
    if (!vm->genv) return -1;
    JENV(vm, vm->genv)->kind |= E_CAPTURED;
    JENV(vm, vm->genv)->thisv = jv_obj(vm->global);
    if (js_lib_init(vm) != 0 || js_lib2_init(vm) != 0 || js_regexp_init(vm) != 0) return -1;
    return vm->ctl == CTL_ABORT ? -1 : 0;
}

/* ---- eval and calls on the private stack ---- */

static void uncaught(js_vm_t *vm) {
    char b[sizeof(vm->err)];
    const char *pre = "Uncaught ";
    int n = 0, i;
    jv e = vm->exc;
    vm->ctl = CTL_NONE;
    for (i = 0; pre[i]; i++) b[n++] = pre[i];
    n += js_display(vm, e, b + n, (int)sizeof(b) - n, 1);
    if (vm->ctl) {
        vm->ctl = CTL_NONE;
        n = 8;
    }
    b[n] = 0;
    for (i = 0; i <= n; i++) vm->err[i] = b[i];
}

static void finish_run(js_vm_t *vm) {
    if (vm->ctl == CTL_THROW) {
        uncaught(vm);
        vm->run_rc = JS_THROWN;
    } else if (vm->ctl == CTL_ABORT) {
        vm->run_rc = JS_ABORTED;
    } else {
        vm->run_rc = JS_OK;
    }
    vm->ctl = CTL_NONE;
    vm->opt_short = 0;
    vm->depth = 0;
    vm->in_new = 0;
    vm->super_this = 0;
}

static void eval_entry(js_vm_t *vm) {
    uint32_t prog = js_parse(vm, vm->src, vm->src_len, 0);
    vm->run_out = jv_undef();
    if (!prog) {
        if (vm->ctl != CTL_ABORT) js_abort(vm, "SyntaxError: could not parse the script");
    } else {
        vm->run_prog = prog;
        vm->run_out = js_run_program(vm, prog);
    }
    finish_run(vm);
}

static void call_entry(js_vm_t *vm) {
    vm->run_out = js_call(vm, vm->run_fn, vm->run_this, vm->run_argv, vm->run_argc);
    finish_run(vm);
}

static void begin_run(js_vm_t *vm) {
    vm->ctl = CTL_NONE;
    vm->steps = 0;
    vm->depth = 0;
    vm->opt_short = 0;
    vm->err[0] = 0;
    vm->err_line = 0;
}

int js_eval(js_vm_t *vm, const char *src, int len, jv *out) {
    begin_run(vm);
    vm->src = src;
    vm->src_len = len;
    on_stack(vm, eval_entry);
    if (out) *out = vm->run_out;
    return vm->run_rc;
}

int js_call_value(js_vm_t *vm, jv fn, jv thisv, jv *argv, int argc, jv *out) {
    begin_run(vm);
    vm->run_fn = fn;
    vm->run_this = thisv;
    vm->run_argv = argv;
    vm->run_argc = argc;
    on_stack(vm, call_entry);
    if (out) *out = vm->run_out;
    return vm->run_rc;
}
