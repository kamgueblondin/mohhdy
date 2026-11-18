/* wf_jsproxy.c - Lot 115 (US-031): Proxy for the JavaScript subset.
 *
 * Lot 113 refused Proxy; reactive page code (a store that re-renders on
 * a property write, a validator, a default-value object, an observable
 * array) is written with it. A proxy is an object of class
 * JS_CLASS_PROXY: x0 the target, x1 the handler, x2 bit 0 callable, bit
 * 1 constructor (from the target). The engine's own paths call in here
 * where they meet a proxy: [[Get]] (also on the prototype chain), [[Set]],
 * [[HasProperty]], [[Delete]], the own key lists (Object.keys,
 * getOwnPropertyNames, for-in, JSON.stringify, spread, Reflect.ownKeys),
 * [[Call]], [[Construct]], getPrototypeOf/setPrototypeOf,
 * defineProperty and getOwnPropertyDescriptor. A missing trap forwards the
 * operation to the target. Proxy.revocable gives a revoke function; a
 * revoked proxy throws on every operation.
 *
 * Not checked (documented in docs/lot115_plan.md): the invariants a trap
 * result must keep against a non-configurable or non-extensible target;
 * isExtensible/preventExtensions traps (the operations go to the target);
 * the proxy's own [[Prototype]] seen by instanceof and the prototype walk
 * of a plain lookup is the target's at creation (getPrototypeOf trap
 * answers Object.getPrototypeOf and Reflect only). Internal fast paths
 * that read an object's property table directly see an empty object.
 *
 * Bounds: a chain of proxies without traps is followed through the
 * engine's call depth (JS_MAX_DEPTH). Freestanding C, no libc. */
#include "wf_jsi.h"

static int cslen(const char *s) { int n = 0; while (s[n]) n++; return n; }
#define KEY(vm, s) js_intern((vm), (s), cslen(s))

#define PX_CALL 1u
#define PX_CTOR 2u

static jv key_value(js_vm_t *vm, uint32_t key) { return JS_KEY_IS_SYM(vm, key) ? jv_sym(key) : jv_str(key); }

/* the trap of the handler (callable), or undefined to forward; a revoked
 * proxy or a trap that is not a function throws (vm->ctl set) */
static jv trap_of(js_vm_t *vm, uint32_t p, const char *name, jv *target) {
    js_obj_t *o = JOBJ(vm, p);
    jv h, t;
    if (!o->x1) {
        js_sb_t sb;
        js_sb_init(vm, &sb);
        js_sb_put(vm, &sb, "Cannot perform '", 16);
        js_sb_put(vm, &sb, name, cslen(name));
        js_sb_put(vm, &sb, "' on a proxy that has been revoked", 34);
        js_throw_err2(vm, JE_TYPE, "", js_sb_finish(vm, &sb));
        return jv_undef();
    }
    *target = jv_obj(o->x0);
    h = jv_obj(o->x1);
    if (vm->depth >= JS_MAX_DEPTH) { js_throw_err(vm, JE_RANGE, "Maximum call stack size exceeded (guest bound)"); return jv_undef(); }
    t = js_get(vm, h, KEY(vm, name));
    if (vm->ctl || jv_is_nullish(t)) return jv_undef();
    if (!js_is_callable(vm, t)) {
        js_sb_t sb;
        js_sb_init(vm, &sb);
        js_sb_put(vm, &sb, "proxy trap '", 12);
        js_sb_put(vm, &sb, name, cslen(name));
        js_sb_put(vm, &sb, "' is not a function", 19);
        js_throw_err2(vm, JE_TYPE, "", js_sb_finish(vm, &sb));
        return jv_undef();
    }
    return t;
}

/* a trap call with the depth counted (a trap that reads its own proxy
 * recurses through here) */
static jv trap_call(js_vm_t *vm, jv trap, uint32_t p, jv *argv, int argc) {
    jv r;
    vm->depth++;
    r = js_call(vm, trap, jv_obj(JOBJ(vm, p)->x1), argv, argc);
    vm->depth--;
    return r;
}

jv js_proxy_get(js_vm_t *vm, uint32_t p, uint32_t key, jv recv) {
    jv target = jv_undef(), trap = trap_of(vm, p, "get", &target), a[3], r;
    if (vm->ctl) return jv_undef();
    if (trap.t == JT_UNDEF) {
        vm->depth++;
        r = js_get_recv(vm, target, key, recv);
        vm->depth--;
        return r;
    }
    a[0] = target;
    a[1] = key_value(vm, key);
    a[2] = recv;
    return trap_call(vm, trap, p, a, 3);
}

/* 1 the trap or the target took the write, 0 it refused (Reflect.set's
 * false; a plain assignment ignores it, as in sloppy code) */
int js_proxy_set(js_vm_t *vm, uint32_t p, uint32_t key, jv v, jv recv) {
    jv target = jv_undef(), trap = trap_of(vm, p, "set", &target), a[4], r;
    if (vm->ctl) return 0;
    if (trap.t == JT_UNDEF) {
        int ok;
        vm->depth++;
        if (recv.t == JT_OBJ && recv.a == p) recv = target;   /* the write lands on the target */
        ok = js_reflect_set(vm, target, key, v, recv);
        vm->depth--;
        return ok && !vm->ctl;
    }
    a[0] = target;
    a[1] = key_value(vm, key);
    a[2] = v;
    a[3] = recv;
    r = trap_call(vm, trap, p, a, 4);
    return !vm->ctl && js_truthy(vm, r);
}

int js_proxy_has(js_vm_t *vm, uint32_t p, uint32_t key) {
    jv target = jv_undef(), trap = trap_of(vm, p, "has", &target), a[2], r;
    int ok;
    if (vm->ctl) return 0;
    if (trap.t == JT_UNDEF) {
        vm->depth++;
        ok = js_has(vm, target.a, key);
        vm->depth--;
        return ok;
    }
    a[0] = target;
    a[1] = key_value(vm, key);
    r = trap_call(vm, trap, p, a, 2);
    return !vm->ctl && js_truthy(vm, r);
}

int js_proxy_delete(js_vm_t *vm, uint32_t p, uint32_t key) {
    jv target = jv_undef(), trap = trap_of(vm, p, "deleteProperty", &target), a[2], r;
    int ok;
    if (vm->ctl) return 0;
    if (trap.t == JT_UNDEF) {
        vm->depth++;
        ok = js_delete(vm, target, key_value(vm, key));
        vm->depth--;
        return ok;
    }
    a[0] = target;
    a[1] = key_value(vm, key);
    r = trap_call(vm, trap, p, a, 2);
    return !vm->ctl && js_truthy(vm, r);
}

/* getOwnPropertyDescriptor: a descriptor object or undefined */
jv js_proxy_desc(js_vm_t *vm, uint32_t p, uint32_t key) {
    jv target = jv_undef(), trap = trap_of(vm, p, "getOwnPropertyDescriptor", &target), a[2], r;
    if (vm->ctl) return jv_undef();
    if (trap.t == JT_UNDEF) {
        vm->depth++;
        r = js_desc_of(vm, target, key);
        vm->depth--;
        return r;
    }
    a[0] = target;
    a[1] = key_value(vm, key);
    r = trap_call(vm, trap, p, a, 2);
    if (vm->ctl) return jv_undef();
    if (r.t != JT_OBJ && r.t != JT_UNDEF)
        return js_throw_err(vm, JE_TYPE, "proxy getOwnPropertyDescriptor trap returned neither object nor undefined");
    return r;
}

int js_proxy_define(js_vm_t *vm, uint32_t p, uint32_t key, jv desc) {
    jv target = jv_undef(), trap = trap_of(vm, p, "defineProperty", &target), a[3], r;
    int ok;
    if (vm->ctl) return 0;
    if (trap.t == JT_UNDEF) {
        vm->depth++;
        ok = js_define_desc(vm, target.a, key, desc, 0);
        vm->depth--;
        return ok;
    }
    a[0] = target;
    a[1] = key_value(vm, key);
    a[2] = desc;
    r = trap_call(vm, trap, p, a, 3);
    return !vm->ctl && js_truthy(vm, r);
}

/* the own keys: names (strings) or symbols, only the enumerable ones when
 * asked (the descriptor decides, through the trap when there is one) */
uint32_t js_proxy_keys(js_vm_t *vm, uint32_t p, int only_enum, int symbols) {
    jv target = jv_undef(), trap = trap_of(vm, p, "ownKeys", &target), list;
    uint32_t out, len = 0, i;
    if (vm->ctl) return 0;
    out = js_array_new(vm, 8);
    if (!out) return 0;
    if (trap.t == JT_UNDEF) {
        uint32_t a, b;
        vm->depth++;
        a = js_own_keys(vm, target.a, 0);
        b = js_own_symbols(vm, target.a, 0);
        vm->depth--;
        if (!a || !b) return 0;
        for (i = 0; i < JOBJ(vm, b)->alen; i++) js_array_push(vm, a, js_array_get(vm, b, i));
        list = jv_obj(a);
    } else {
        list = trap_call(vm, trap, p, &target, 1);
        if (vm->ctl) return 0;
        if (list.t != JT_OBJ) { js_throw_err(vm, JE_TYPE, "CreateListFromArrayLike called on non-object"); return 0; }
    }
    js_length_of(vm, list, &len);
    for (i = 0; i < len && !vm->ctl; i++) {
        jv k = js_get_v(vm, list, jv_int((int32_t)i));
        if (vm->ctl) return 0;
        if (k.t != JT_STR && k.t != JT_SYM) { js_throw_err(vm, JE_TYPE, "proxy ownKeys trap result must hold only strings and symbols"); return 0; }
        if ((k.t == JT_SYM) != (symbols != 0)) continue;
        if (k.t == JT_STR) k = jv_str(js_intern_str(vm, k.a));
        if (only_enum) {
            jv d = js_proxy_desc(vm, p, k.a);
            if (vm->ctl) return 0;
            if (d.t != JT_OBJ || !js_truthy(vm, js_get(vm, d, vm->s_enumerable))) continue;
        }
        js_array_push(vm, out, k);
    }
    return vm->ctl ? 0 : out;
}

jv js_proxy_getproto(js_vm_t *vm, uint32_t p) {
    jv target = jv_undef(), trap = trap_of(vm, p, "getPrototypeOf", &target), r;
    if (vm->ctl) return jv_undef();
    if (trap.t == JT_UNDEF) {
        uint32_t q = JOBJ(vm, target.a)->proto;
        if (JOBJ(vm, target.a)->cls == JS_CLASS_PROXY) {
            vm->depth++;
            r = js_proxy_getproto(vm, target.a);
            vm->depth--;
            return r;
        }
        return q ? jv_obj(q) : jv_null();
    }
    r = trap_call(vm, trap, p, &target, 1);
    if (vm->ctl) return jv_undef();
    if (r.t != JT_OBJ && r.t != JT_NULL) return js_throw_err(vm, JE_TYPE, "proxy getPrototypeOf trap returned neither object nor null");
    return r;
}

int js_proxy_setproto(js_vm_t *vm, uint32_t p, uint32_t proto) {
    jv target = jv_undef(), trap = trap_of(vm, p, "setPrototypeOf", &target), a[2], r;
    int ok;
    if (vm->ctl) return 0;
    if (trap.t == JT_UNDEF) {
        vm->depth++;
        ok = js_set_proto(vm, target.a, proto);
        vm->depth--;
        return ok;
    }
    a[0] = target;
    a[1] = proto ? jv_obj(proto) : jv_null();
    r = trap_call(vm, trap, p, a, 2);
    return !vm->ctl && js_truthy(vm, r);
}

static uint32_t args_array(js_vm_t *vm, jv *argv, int argc) {
    uint32_t arr = js_array_new(vm, argc > 0 ? (uint32_t)argc : 1u);
    int i;
    for (i = 0; arr && i < argc; i++) js_array_push(vm, arr, argv[i]);
    return arr;
}

jv js_proxy_call(js_vm_t *vm, uint32_t p, jv thisv, jv *argv, int argc) {
    jv target = jv_undef(), trap = trap_of(vm, p, "apply", &target), a[3], r;
    uint32_t arr;
    if (vm->ctl) return jv_undef();
    if (trap.t == JT_UNDEF) {
        vm->depth++;
        r = js_call(vm, target, thisv, argv, argc);
        vm->depth--;
        return r;
    }
    arr = args_array(vm, argv, argc);
    if (!arr) return jv_undef();
    a[0] = target;
    a[1] = thisv;
    a[2] = jv_obj(arr);
    return trap_call(vm, trap, p, a, 3);
}

jv js_proxy_construct(js_vm_t *vm, uint32_t p, jv *argv, int argc, jv newtarget) {
    jv target = jv_undef(), trap, a[3], r;
    uint32_t arr;
    if (!(JOBJ(vm, p)->x2 & PX_CTOR)) return js_throw_err(vm, JE_TYPE, "proxy is not a constructor");
    trap = trap_of(vm, p, "construct", &target);
    if (vm->ctl) return jv_undef();
    if (newtarget.t == JT_OBJ && newtarget.a == p) newtarget = target;
    if (trap.t == JT_UNDEF) {
        vm->depth++;
        r = js_construct(vm, target, argv, argc, newtarget);
        vm->depth--;
        return r;
    }
    arr = args_array(vm, argv, argc);
    if (!arr) return jv_undef();
    a[0] = target;
    a[1] = jv_obj(arr);
    a[2] = newtarget;
    r = trap_call(vm, trap, p, a, 3);
    if (vm->ctl) return jv_undef();
    if (r.t != JT_OBJ) return js_throw_err(vm, JE_TYPE, "proxy [[Construct]] must return an object");
    return r;
}

/* the target a proxy stands for (display, Array.isArray), 0 when revoked */
uint32_t js_proxy_target(js_vm_t *vm, uint32_t p) {
    int n = 0;
    while (p && JOBJ(vm, p)->cls == JS_CLASS_PROXY && n++ < 256) p = JOBJ(vm, p)->x1 ? JOBJ(vm, p)->x0 : 0;
    return p;
}

int js_proxy_is_array(js_vm_t *vm, jv v) {
    uint32_t t;
    if (v.t != JT_OBJ || JOBJ(vm, v.a)->cls != JS_CLASS_PROXY) return 0;
    t = js_proxy_target(vm, v.a);
    return t && JOBJ(vm, t)->cls == JS_CLASS_ARRAY;
}

/* ---- the constructor ---- */

static uint32_t proxy_new(js_vm_t *vm, jv target, jv handler) {
    uint32_t o;
    int c;
    if (target.t != JT_OBJ || handler.t != JT_OBJ) {
        js_throw_err(vm, JE_TYPE, "Cannot create proxy with a non-object as target or handler");
        return 0;
    }
    o = js_obj_new(vm, JS_CLASS_PROXY, JOBJ(vm, target.a)->proto);
    if (!o) return 0;
    JOBJ(vm, o)->x0 = target.a;
    JOBJ(vm, o)->x1 = handler.a;
    c = JOBJ(vm, target.a)->cls;
    if (js_is_callable(vm, target)) JOBJ(vm, o)->x2 = PX_CALL;
    if (c == JS_CLASS_FUNC) {
        js_node_t *n = JNODE(vm, JOBJ(vm, target.a)->x0);
        if (!(n->op & (FK_GEN | FK_ASYNC)) && (!(n->fl & (F_ARROW | F_METHOD)) || (n->fl & F_CTOR)))
            JOBJ(vm, o)->x2 |= PX_CTOR;
    } else if (c == JS_CLASS_NATIVE || c == JS_CLASS_BOUND || (c == JS_CLASS_PROXY && (JOBJ(vm, target.a)->x2 & PX_CTOR))) {
        JOBJ(vm, o)->x2 |= PX_CTOR;
    }
    return o;
}

static jv proxy_ctor(js_vm_t *vm, jv thisv, jv *argv, int argc, uint32_t m) {
    uint32_t o;
    (void)thisv; (void)m;
    if (!vm->in_new) return js_throw_err(vm, JE_TYPE, "Constructor Proxy requires 'new'");
    o = proxy_new(vm, JARG(0), JARG(1));
    return o ? jv_obj(o) : jv_undef();
}

static jv proxy_revoke(js_vm_t *vm, jv thisv, jv *argv, int argc, uint32_t p) {
    (void)thisv; (void)argv; (void)argc;
    if (p && JOBJ(vm, p)->cls == JS_CLASS_PROXY) {
        JOBJ(vm, p)->x0 = 0;
        JOBJ(vm, p)->x1 = 0;
    }
    return jv_undef();
}

static jv proxy_revocable(js_vm_t *vm, jv thisv, jv *argv, int argc, uint32_t m) {
    uint32_t p, r, f;
    (void)thisv; (void)m;
    p = proxy_new(vm, JARG(0), JARG(1));
    if (!p) return jv_undef();
    r = js_obj_new(vm, JS_CLASS_OBJECT, vm->p_object);
    f = r ? js_native_new(vm, "", proxy_revoke, 0, p) : 0;
    if (!f) return jv_undef();
    js_define(vm, r, KEY(vm, "proxy"), jv_obj(p), P_DEFAULT);
    js_define(vm, r, KEY(vm, "revoke"), jv_obj(f), P_DEFAULT);
    return jv_obj(r);
}

int js_proxy_init(js_vm_t *vm) {
    uint32_t c = js_native_new(vm, "Proxy", proxy_ctor, 2, 0);
    if (!c) return -1;
    js_def_native(vm, c, "revocable", proxy_revocable, 2, 0);
    js_define(vm, vm->global, KEY(vm, "Proxy"), jv_obj(c), P_HIDDEN);
    return vm->ctl == CTL_ABORT ? -1 : 0;
}
