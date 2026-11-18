/* wf_jsweak.c - Lot 115 (US-031): WeakMap, WeakSet, WeakRef,
 * FinalizationRegistry and Reflect for the JavaScript subset.
 *
 * Weak tables. Lot 113 refused WeakMap; real page scripts use it for
 * per-element state (a widget per node) and private data. With the
 * collector of wf_jsgc.c a weak table can be honest: its storage is one
 * JK_WEAK block (js_weak_t, wf_jsi.h) of entries { k, v, t }. The
 * collector does not trace a weak table; after marking it keeps v alive
 * while k is (an ephemeron, to a fixed point: a value may hold the key of
 * another entry) and empties the entries whose key died, before the
 * sweep. A WeakMap or WeakSet keeps its table in x0; a full table is
 * compacted (dead slots dropped) or doubled, up to WK_MAX entries.
 *
 * WeakRef keeps a one-entry table: deref() answers the target until a
 * collection finds nothing else holding it. Collections only run between
 * runs (wf_jsgc.c), so a target stays for the job that made or read the
 * WeakRef, as the specification asks.
 *
 * FinalizationRegistry keeps { target, held, token } per register(); an
 * entry goes when its target dies and unregister(token) drops the entries
 * of a token. The cleanup callback is never called: the specification
 * allows that, and calling into the page from the collector would run
 * scripts where the hosts do not expect them. Documented in
 * docs/lot115_plan.md.
 *
 * Reflect: the thirteen functions, over the same object internals as
 * Object (wf_jslib.c: js_define_desc, js_set_proto, js_desc_of), with
 * booleans where Object throws and TypeErrors for a non-object target.
 * There is no Proxy, so there is no trap to forward to.
 *
 * Bounds: WK_MAX entries per table (36 bytes each), JS_GC_WEAK tables
 * cleared per collection (more are kept strongly, wf_jsgc.c), Reflect
 * argument lists of 65536. Freestanding C, no libc. */
#include "wf_jsi.h"

#define WK_FIRST 4u
#define WK_MAX 16384u

static int cslen(const char *s) { int n = 0; while (s[n]) n++; return n; }
#define KEY(vm, s) js_intern((vm), (s), cslen(s))

enum { WM_GET = 0, WM_SET, WM_HAS, WM_DELETE };

/* ---- the table ---- */

static uint32_t wk_new(js_vm_t *vm, uint32_t cap) {
    uint32_t b = js_alloc_k(vm, (uint32_t)sizeof(js_weak_t) + cap * (uint32_t)sizeof(js_weak_ent_t), JK_WEAK);
    if (!b) return 0;
    ((js_weak_t *)JPTR(vm, b))->used = 0;
    ((js_weak_t *)JPTR(vm, b))->cap = cap;
    return b;
}

/* can v be a weak key? objects, and symbols that Symbol.for did not
 * register (anyone could make those again) */
static int holdable(js_vm_t *vm, jv v) {
    js_str_t *p;
    uint32_t k;
    js_prop_t *e;
    if (v.t == JT_OBJ) return 1;
    if (v.t != JT_SYM) return 0;
    p = JSTR(vm, v.a);
    if ((p->interned & JS_STR_KIND_MASK) != JS_SYM_PUBLIC) return 0;
    if (p->hash & JS_SYM_NODESC) return 1;
    k = js_intern_find(vm, p->s, (int)p->len);
    if (!k) return 1;
    e = js_own_prop(vm, vm->sym_registry, k);
    return !(e && e->v.t == JT_SYM && e->v.a == v.a);
}

static int same_ref(jv a, jv b) { return a.t == b.t && a.a == b.a; }

static int wk_find(js_vm_t *vm, uint32_t b, jv k) {
    uint32_t i, used = ((js_weak_t *)JPTR(vm, b))->used;
    js_weak_ent_t *e = JWEAK_ENT(vm, b);
    for (i = 0; i < used; i++)
        if (e[i].k.t != JT_EMPTY && same_ref(e[i].k, k)) return (int)i;
    return -1;
}

static uint32_t wk_live(js_vm_t *vm, uint32_t b) {
    uint32_t i, n = 0, used = ((js_weak_t *)JPTR(vm, b))->used;
    for (i = 0; i < used; i++)
        if (JWEAK_ENT(vm, b)[i].k.t != JT_EMPTY) n++;
    return n;
}

/* room for one more entry in the table of o (x0): drop the free slots,
 * double when more than half is live; 0 after a RangeError */
static uint32_t wk_room(js_vm_t *vm, uint32_t o) {
    uint32_t b = JOBJ(vm, o)->x0, live, cap, nb, i, j = 0;
    js_weak_t *h = (js_weak_t *)JPTR(vm, b);
    if (h->used < h->cap) return b;
    live = wk_live(vm, b);
    cap = h->cap;
    if (live * 2u >= cap) cap *= 2u;
    if (live >= WK_MAX || cap > WK_MAX) cap = WK_MAX;
    if (live >= cap) { js_throw_err(vm, JE_RANGE, "weak collection full (the guest keeps 16384 entries)"); return 0; }
    nb = wk_new(vm, cap);
    if (!nb) return 0;
    b = JOBJ(vm, o)->x0;
    for (i = 0; i < ((js_weak_t *)JPTR(vm, b))->used; i++) {
        js_weak_ent_t *e = JWEAK_ENT(vm, b) + i;
        if (e->k.t == JT_EMPTY) continue;
        JWEAK_ENT(vm, nb)[j++] = *e;
    }
    ((js_weak_t *)JPTR(vm, nb))->used = j;
    JOBJ(vm, o)->x0 = nb;
    return nb;
}

static int wk_put(js_vm_t *vm, uint32_t o, jv k, jv v, jv t, int replace) {
    uint32_t b = JOBJ(vm, o)->x0;
    int i = replace ? wk_find(vm, b, k) : -1;
    js_weak_ent_t *e;
    if (i >= 0) { JWEAK_ENT(vm, b)[i].v = v; return 1; }
    b = wk_room(vm, o);
    if (!b) return 0;
    e = JWEAK_ENT(vm, b) + ((js_weak_t *)JPTR(vm, b))->used++;
    e->k = k;
    e->v = v;
    e->t = t;
    return 1;
}

static void wk_drop(js_vm_t *vm, uint32_t b, int i) {
    js_weak_ent_t *e = JWEAK_ENT(vm, b) + i;
    e->k = jv_empty();
    e->v = jv_undef();
    e->t = jv_undef();
}

/* ---- construction ---- */

static const char *cls_name(int cls) {
    switch (cls) {
    case JS_CLASS_WEAKMAP: return "WeakMap";
    case JS_CLASS_WEAKSET: return "WeakSet";
    case JS_CLASS_WEAKREF: return "WeakRef";
    default: return "FinalizationRegistry";
    }
}

static uint32_t proto_of(js_vm_t *vm, int cls) {
    switch (cls) {
    case JS_CLASS_WEAKMAP: return vm->p_weakmap;
    case JS_CLASS_WEAKSET: return vm->p_weakset;
    case JS_CLASS_WEAKREF: return vm->p_weakref;
    default: return vm->p_finreg;
    }
}

/* the object a constructor fills: new, or the this of a subclass's super() */
static uint32_t weak_object(js_vm_t *vm, int cls, uint32_t cap) {
    uint32_t o, b;
    if (!vm->in_new && !vm->super_this) {
        js_sb_t sb;
        js_sb_init(vm, &sb);
        js_sb_put(vm, &sb, "Constructor ", 12);
        js_sb_put(vm, &sb, cls_name(cls), cslen(cls_name(cls)));
        js_sb_put(vm, &sb, " requires 'new'", 15);
        js_throw_err2(vm, JE_TYPE, "", js_sb_finish(vm, &sb));
        return 0;
    }
    if (vm->super_this && JOBJ(vm, vm->super_this)->cls == JS_CLASS_OBJECT) {
        o = vm->super_this;
        JOBJ(vm, o)->cls = (uint8_t)cls;
        vm->super_this = 0;
    } else {
        o = js_obj_new(vm, cls, proto_of(vm, cls));
    }
    if (!o) return 0;
    b = wk_new(vm, cap);
    if (!b) return 0;
    JOBJ(vm, o)->x0 = b;
    return o;
}

static jv weak_ctor(js_vm_t *vm, jv thisv, jv *argv, int argc, uint32_t cls) {
    uint32_t o, arr = 0, i;
    (void)thisv;
    if (cls == JS_CLASS_WEAKREF) {
        if (!vm->in_new && !vm->super_this) return js_throw_err(vm, JE_TYPE, "Constructor WeakRef requires 'new'");
        if (!holdable(vm, JARG(0))) return js_throw_err(vm, JE_TYPE, "WeakRef: invalid target");
        o = weak_object(vm, (int)cls, 1);
        if (!o || !wk_put(vm, o, argv[0], jv_undef(), jv_undef(), 0)) return jv_undef();
        return jv_obj(o);
    }
    if (cls == JS_CLASS_FINREG) {
        if (!vm->in_new && !vm->super_this)
            return js_throw_err(vm, JE_TYPE, "Constructor FinalizationRegistry requires 'new'");
        if (!js_is_callable(vm, JARG(0))) return js_throw_err(vm, JE_TYPE, "FinalizationRegistry: cleanup must be callable");
        o = weak_object(vm, (int)cls, WK_FIRST);
        if (!o) return jv_undef();
        JOBJ(vm, o)->x1 = argv[0].a;      /* kept for the record, never called */
        return jv_obj(o);
    }
    o = weak_object(vm, (int)cls, WK_FIRST);
    if (!o) return jv_undef();
    if (jv_is_nullish(JARG(0))) return jv_obj(o);
    js_iter_values(vm, argv[0], &arr);
    if (vm->ctl) return jv_undef();
    for (i = 0; i < JOBJ(vm, arr)->alen && !vm->ctl; i++) {
        jv e = js_array_get(vm, arr, i), k, v = jv_undef();
        if (cls == JS_CLASS_WEAKSET) {
            k = e;
            if (!holdable(vm, k)) return js_throw_err(vm, JE_TYPE, "Invalid value used in weak set");
        } else {
            if (e.t != JT_OBJ) return js_throw_err(vm, JE_TYPE, "Iterator value is not an entry object");
            k = js_get_v(vm, e, jv_int(0));
            v = js_get_v(vm, e, jv_int(1));
            if (vm->ctl) return jv_undef();
            if (!holdable(vm, k)) return js_throw_err(vm, JE_TYPE, "Invalid value used as weak map key");
        }
        if (!wk_put(vm, o, k, v, jv_undef(), 1)) return jv_undef();
    }
    return vm->ctl ? jv_undef() : jv_obj(o);
}

/* "Invalid unregisterToken ('2')", as V8 words it */
static jv bad_token(js_vm_t *vm, jv v) {
    uint32_t s = v.t == JT_SYM ? js_sym_descr(vm, v.a) : js_to_string(vm, v);
    if (vm->ctl) return jv_undef();
    return js_throw_err2(vm, JE_TYPE, "", js_str_cat(vm, js_str_cat(vm, js_str_c(vm, "Invalid unregisterToken ('"), s), js_str_c(vm, "')")));
}

/* ---- methods ---- */

static int weak_this(js_vm_t *vm, jv thisv, int cls, const char *method) {
    js_sb_t sb;
    if (thisv.t == JT_OBJ && JOBJ(vm, thisv.a)->cls == cls) return 1;
    js_sb_init(vm, &sb);
    js_sb_put(vm, &sb, "Method ", 7);
    js_sb_put(vm, &sb, cls_name(cls), cslen(cls_name(cls)));
    js_sb_put(vm, &sb, ".prototype.", 11);
    js_sb_put(vm, &sb, method, cslen(method));
    js_sb_put(vm, &sb, " called on incompatible receiver", 32);
    js_throw_err2(vm, JE_TYPE, "", js_sb_finish(vm, &sb));
    return 0;
}

static const char *const k_wm_names[] = { "get", "set", "has", "delete" };

/* WeakMap and WeakSet: magic = class << 8 | WM_* */
static jv weak_method(js_vm_t *vm, jv thisv, jv *argv, int argc, uint32_t m) {
    int cls = (int)(m >> 8), op = (int)(m & 0xFFu), i;
    uint32_t o, b;
    jv k = JARG(0);
    const char *name = cls == JS_CLASS_WEAKSET && op == WM_SET ? "add" : k_wm_names[op];
    if (!weak_this(vm, thisv, cls, name)) return jv_undef();
    o = thisv.a;
    b = JOBJ(vm, o)->x0;
    switch (op) {
    case WM_GET:
        i = holdable(vm, k) ? wk_find(vm, b, k) : -1;
        return i < 0 ? jv_undef() : JWEAK_ENT(vm, b)[i].v;
    case WM_SET:
        if (!holdable(vm, k))
            return js_throw_err(vm, JE_TYPE, cls == JS_CLASS_WEAKSET ? "Invalid value used in weak set" : "Invalid value used as weak map key");
        wk_put(vm, o, k, cls == JS_CLASS_WEAKSET ? jv_undef() : JARG(1), jv_undef(), 1);
        return vm->ctl ? jv_undef() : thisv;
    case WM_HAS:
        return jv_bool(holdable(vm, k) && wk_find(vm, b, k) >= 0);
    default:
        i = holdable(vm, k) ? wk_find(vm, b, k) : -1;
        if (i < 0) return jv_bool(0);
        wk_drop(vm, b, i);
        return jv_bool(1);
    }
}

static jv weakref_deref(js_vm_t *vm, jv thisv, jv *argv, int argc, uint32_t m) {
    uint32_t b;
    (void)argv; (void)argc; (void)m;
    if (!weak_this(vm, thisv, JS_CLASS_WEAKREF, "deref")) return jv_undef();
    b = JOBJ(vm, thisv.a)->x0;
    if (((js_weak_t *)JPTR(vm, b))->used == 0 || JWEAK_ENT(vm, b)[0].k.t == JT_EMPTY) return jv_undef();
    return JWEAK_ENT(vm, b)[0].k;
}

/* register 0, unregister 1 */
static jv finreg_method(js_vm_t *vm, jv thisv, jv *argv, int argc, uint32_t m) {
    uint32_t o, b, i, used;
    int n = 0;
    if (!weak_this(vm, thisv, JS_CLASS_FINREG, m ? "unregister" : "register")) return jv_undef();
    o = thisv.a;
    if (m == 0) {
        jv target = JARG(0), held = JARG(1), token = JARG(2);
        if (!holdable(vm, target)) return js_throw_err(vm, JE_TYPE, "FinalizationRegistry.prototype.register: invalid target");
        if (same_ref(target, held))
            return js_throw_err(vm, JE_TYPE, "FinalizationRegistry.prototype.register: target and holdings must not be same");
        if (token.t != JT_UNDEF && !holdable(vm, token)) return bad_token(vm, token);
        wk_put(vm, o, target, held, token, 0);
        return jv_undef();
    }
    if (!holdable(vm, JARG(0))) return bad_token(vm, JARG(0));
    b = JOBJ(vm, o)->x0;
    used = ((js_weak_t *)JPTR(vm, b))->used;
    for (i = 0; i < used; i++) {
        js_weak_ent_t *e = JWEAK_ENT(vm, b) + i;
        if (e->k.t != JT_EMPTY && same_ref(e->t, argv[0])) { wk_drop(vm, b, (int)i); n++; }
    }
    return jv_bool(n > 0);
}

/* ---- Reflect ---- */

enum {
    RF_APPLY = 0, RF_CONSTRUCT, RF_DEFINE, RF_DELETE, RF_GET, RF_DESC, RF_GETPROTO, RF_HAS,
    RF_ISEXT, RF_OWNKEYS, RF_PREVENT, RF_SET, RF_SETPROTO, RF_COUNT
};

static const char *const k_rf_names[RF_COUNT] = {
    "apply", "construct", "defineProperty", "deleteProperty", "get", "getOwnPropertyDescriptor",
    "getPrototypeOf", "has", "isExtensible", "ownKeys", "preventExtensions", "set", "setPrototypeOf"
};
static const int k_rf_len[RF_COUNT] = { 3, 2, 3, 2, 2, 2, 1, 2, 1, 1, 1, 3, 2 };

/* CreateListFromArrayLike into a fresh array; 0 after a throw */
static uint32_t arg_list(js_vm_t *vm, jv list) {
    uint32_t len, i, arr;
    if (list.t != JT_OBJ) { js_throw_err(vm, JE_TYPE, "CreateListFromArrayLike called on non-object"); return 0; }
    js_length_of(vm, list, &len);
    if (vm->ctl) return 0;
    if (len > 65536u) { js_throw_err(vm, JE_RANGE, "too many arguments for the guest"); return 0; }
    arr = js_array_new(vm, len ? len : 1u);
    for (i = 0; arr && i < len && !vm->ctl; i++) {
        jv e = js_get_v(vm, list, jv_int((int32_t)i));
        js_array_push(vm, arr, e.t == JT_EMPTY ? jv_undef() : e);
    }
    return vm->ctl ? 0 : arr;
}

static jv *list_ptr(js_vm_t *vm, uint32_t arr) {
    return JOBJ(vm, arr)->alen ? (jv *)JPTR(vm, JOBJ(vm, arr)->arr) : 0;
}

/* [[Set]] with a receiver (OrdinarySet): 1 done, 0 refused; a proxy on
 * the way answers through its set trap */
int js_reflect_set(js_vm_t *vm, jv target, uint32_t key, jv v, jv recv) {
    uint32_t o = target.a, idx;
    int depth = 0;
    js_prop_t *p;
    while (o && depth++ < 256) {
        js_obj_t *ob = JOBJ(vm, o);
        int exotic;
        if (ob->cls == JS_CLASS_PROXY) return js_proxy_set(vm, o, key, v, recv);
        exotic = ob->cls == JS_CLASS_HOST || ob->cls == JS_CLASS_STRING ||
                     ((ob->cls == JS_CLASS_ARRAY || ob->cls == JS_CLASS_ARGS) &&
                      (key == vm->s_length || js_str_index_key(vm, key, &idx)));
        if (exotic) {
            /* the engine's own path for host objects, strings and array slots */
            if (recv.t != JT_OBJ || (JOBJ(vm, recv.a)->oflags & O_FROZEN)) return 0;
            if (!same_ref(recv, target) && o != recv.a) break;
            return js_put(vm, recv, key, v) && !vm->ctl;
        }
        p = js_own_prop(vm, o, key);
        if (p) {
            if (p->flags & P_ACCESSOR) {
                uint32_t setter = p->v.b;
                if (!setter) return 0;
                js_call(vm, jv_obj(setter), recv, &v, 1);
                return !vm->ctl;
            }
            if (!(p->flags & P_WRITE) || (ob->oflags & O_FROZEN)) return 0;
            break;
        }
        o = ob->proto;
    }
    if (recv.t != JT_OBJ) return 0;
    if (JOBJ(vm, recv.a)->cls == JS_CLASS_PROXY) {
        /* a proxy receiver: its [[GetOwnProperty]] and [[DefineOwnProperty]]
         * (the traps, or its target) take the write */
        jv d = js_proxy_desc(vm, recv.a, key);
        uint32_t nd;
        if (vm->ctl) return 0;
        if (d.t == JT_OBJ) {
            jv w = js_get(vm, d, vm->s_writable);
            if (vm->ctl || js_has(vm, d.a, vm->s_get) || js_has(vm, d.a, vm->s_set) || !js_truthy(vm, w)) return 0;
        }
        nd = js_obj_new(vm, JS_CLASS_OBJECT, vm->p_object);
        if (!nd) return 0;
        js_define(vm, nd, vm->s_value, v, P_DEFAULT);
        if (d.t != JT_OBJ) {
            js_define(vm, nd, vm->s_writable, jv_bool(1), P_DEFAULT);
            js_define(vm, nd, vm->s_enumerable, jv_bool(1), P_DEFAULT);
            js_define(vm, nd, vm->s_configurable, jv_bool(1), P_DEFAULT);
        }
        return js_proxy_define(vm, recv.a, key, jv_obj(nd)) && !vm->ctl;
    }
    p = js_own_prop(vm, recv.a, key);
    if (p) {
        if ((p->flags & P_ACCESSOR) || !(p->flags & P_WRITE) || (JOBJ(vm, recv.a)->oflags & O_FROZEN)) return 0;
        p->v = v;
        return 1;
    }
    {
        js_obj_t *rb = JOBJ(vm, recv.a);
        if (!(rb->oflags & O_EXT) || (rb->oflags & O_FROZEN)) return 0;
        if (rb->cls != JS_CLASS_OBJECT && rb->cls != JS_CLASS_FUNC && rb->cls != JS_CLASS_ERROR)
            return js_put(vm, recv, key, v) && !vm->ctl;
    }
    return js_define(vm, recv.a, key, v, P_DEFAULT) && !vm->ctl;
}

static jv reflect_fn(js_vm_t *vm, jv thisv, jv *argv, int argc, uint32_t m) {
    jv t = JARG(0);
    uint32_t key = 0, arr;
    (void)thisv;
    if (m != RF_APPLY && m != RF_CONSTRUCT && t.t != JT_OBJ) {
        js_sb_t sb;
        js_sb_init(vm, &sb);
        js_sb_put(vm, &sb, "Reflect.", 8);
        js_sb_put(vm, &sb, k_rf_names[m], cslen(k_rf_names[m]));
        js_sb_put(vm, &sb, " called on non-object", 21);
        return js_throw_err2(vm, JE_TYPE, "", js_sb_finish(vm, &sb));
    }
    if (m == RF_DEFINE || m == RF_DELETE || m == RF_GET || m == RF_DESC || m == RF_HAS || m == RF_SET) {
        key = js_to_key(vm, JARG(1));
        if (vm->ctl) return jv_undef();
    }
    switch (m) {
    case RF_APPLY:
        if (!js_is_callable(vm, t)) return js_throw_err(vm, JE_TYPE, "Reflect.apply: the target is not a function");
        arr = arg_list(vm, JARG(2));
        if (!arr) return jv_undef();
        return js_call(vm, t, JARG(1), list_ptr(vm, arr), (int)JOBJ(vm, arr)->alen);
    case RF_CONSTRUCT: {
        jv nt = argc > 2 ? argv[2] : t;
        if (!js_is_callable(vm, t)) return js_throw_err(vm, JE_TYPE, "Reflect.construct: the target is not a constructor");
        if (!js_is_callable(vm, nt)) return js_throw_err(vm, JE_TYPE, "Reflect.construct: newTarget is not a constructor");
        arr = arg_list(vm, JARG(1));
        if (!arr) return jv_undef();
        return js_construct(vm, t, list_ptr(vm, arr), (int)JOBJ(vm, arr)->alen, nt);
    }
    case RF_DEFINE:
        if (JARG(2).t != JT_OBJ) return js_throw_err(vm, JE_TYPE, "Property description must be an object");
        return jv_bool(js_define_desc(vm, t.a, key, argv[2], 0));
    case RF_DELETE:
        return jv_bool(js_delete(vm, t, jv_str(key)) && !vm->ctl);
    case RF_GET:
        return js_get_recv(vm, t, key, argc > 2 ? argv[2] : t);
    case RF_DESC:
        return js_desc_of(vm, t, key);
    case RF_GETPROTO:
        if (JOBJ(vm, t.a)->cls == JS_CLASS_PROXY) return js_proxy_getproto(vm, t.a);
        return JOBJ(vm, t.a)->proto ? jv_obj(JOBJ(vm, t.a)->proto) : jv_null();
    case RF_HAS:
        return jv_bool(js_has(vm, t.a, key));
    case RF_ISEXT:
        return jv_bool((JOBJ(vm, t.a)->oflags & O_EXT) && !(JOBJ(vm, t.a)->oflags & O_FROZEN));
    case RF_OWNKEYS: {
        uint32_t names, syms, i;
        if (JOBJ(vm, t.a)->cls == JS_CLASS_PROXY) {
            names = js_proxy_keys(vm, t.a, 0, 0);
            syms = names ? js_proxy_keys(vm, t.a, 0, 1) : 0;
            for (i = 0; syms && i < JOBJ(vm, syms)->alen && !vm->ctl; i++) js_array_push(vm, names, js_array_get(vm, syms, i));
            return names && !vm->ctl ? jv_obj(names) : jv_undef();
        }
        names = js_own_keys(vm, t.a, 0);
        if (!names) return jv_undef();
        syms = js_own_symbols(vm, t.a, 0);
        for (i = 0; syms && i < JOBJ(vm, syms)->alen && !vm->ctl; i++) js_array_push(vm, names, js_array_get(vm, syms, i));
        return vm->ctl ? jv_undef() : jv_obj(names);
    }
    case RF_PREVENT:
        JOBJ(vm, t.a)->oflags &= (uint8_t)~O_EXT;
        return jv_bool(1);
    case RF_SET:
        return jv_bool(js_reflect_set(vm, t, key, JARG(2), argc > 3 ? argv[3] : t));
    default: {      /* RF_SETPROTO */
        jv p = JARG(1);
        if (p.t != JT_OBJ && p.t != JT_NULL) return js_throw_err(vm, JE_TYPE, "Object prototype may only be an Object or null");
        return jv_bool(js_set_proto(vm, t.a, p.t == JT_OBJ ? p.a : 0));
    }
    }
}

/* ---- realm setup ---- */

static uint32_t weak_class(js_vm_t *vm, int cls, int nargs) {
    const char *name = cls_name(cls);
    uint32_t proto = js_obj_new(vm, JS_CLASS_OBJECT, vm->p_object), c;
    if (!proto) return 0;
    c = js_native_new(vm, name, weak_ctor, nargs, (uint32_t)cls);
    if (!c) return 0;
    js_define(vm, c, vm->s_prototype, jv_obj(proto), 0);
    js_define(vm, proto, vm->s_constructor, jv_obj(c), P_HIDDEN);
    js_define(vm, vm->global, KEY(vm, name), jv_obj(c), P_HIDDEN);
    js_define(vm, proto, vm->sym_to_string_tag, jv_str(KEY(vm, name)), P_CONF);
    return proto;
}

int js_weak_init(js_vm_t *vm) {
    uint32_t r;
    int i;
    vm->p_weakmap = weak_class(vm, JS_CLASS_WEAKMAP, 0);
    vm->p_weakset = weak_class(vm, JS_CLASS_WEAKSET, 0);
    vm->p_weakref = weak_class(vm, JS_CLASS_WEAKREF, 1);
    vm->p_finreg = weak_class(vm, JS_CLASS_FINREG, 1);
    if (!vm->p_weakmap || !vm->p_weakset || !vm->p_weakref || !vm->p_finreg) return -1;
    for (i = 0; i < 4; i++) {
        js_def_native(vm, vm->p_weakmap, k_wm_names[i], weak_method, i == WM_SET ? 2 : 1,
                      ((uint32_t)JS_CLASS_WEAKMAP << 8) | (uint32_t)i);
        if (i == WM_GET) continue;
        js_def_native(vm, vm->p_weakset, i == WM_SET ? "add" : k_wm_names[i], weak_method, 1,
                      ((uint32_t)JS_CLASS_WEAKSET << 8) | (uint32_t)i);
    }
    js_def_native(vm, vm->p_weakref, "deref", weakref_deref, 0, 0);
    js_def_native(vm, vm->p_finreg, "register", finreg_method, 2, 0);
    js_def_native(vm, vm->p_finreg, "unregister", finreg_method, 1, 1);
    r = js_obj_new(vm, JS_CLASS_OBJECT, vm->p_object);
    if (!r) return -1;
    for (i = 0; i < RF_COUNT; i++) js_def_native(vm, r, k_rf_names[i], reflect_fn, k_rf_len[i], (uint32_t)i);
    js_define(vm, r, vm->sym_to_string_tag, jv_str(KEY(vm, "Reflect")), P_CONF);
    js_define(vm, vm->global, KEY(vm, "Reflect"), jv_obj(r), P_HIDDEN);
    return vm->ctl == CTL_ABORT ? -1 : 0;
}
