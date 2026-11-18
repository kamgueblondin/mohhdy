/* wf_jssym.c - Lot 115: Symbol and the iterator protocol for the guest
 * JavaScript subset (wf_js.h).
 *
 * A symbol is a value of tag JT_SYM whose a field is a record in the
 * arena laid out like a string (js_str_t): interned says it is a
 * symbol (JS_SYM_PUBLIC, or JS_SYM_PRIVATE for a class private name),
 * s holds the description and hash a serial number. Property keys are
 * plain refs compared by identity, so a symbol works as a key without a
 * second property table; key listings (Object.keys, for-in, JSON,
 * Object.assign) skip it, Object.getOwnPropertySymbols lists it.
 *
 * What is here: Symbol(), Symbol.for and Symbol.keyFor (a registry
 * object in the realm), the well-known symbols iterator, asyncIterator,
 * hasInstance, toPrimitive, toStringTag, species, unscopables,
 * isConcatSpreadable and match (the first five are honoured by the
 * runtime: for-of, spread and destructuring, instanceof, conversions,
 * Object.prototype.toString), Symbol.prototype (toString, valueOf,
 * description), iterator objects over a snapshot (the keys(), values()
 * and entries() of arrays, Map and Set, the string iterator) and the
 * protocol helpers the interpreter uses for user iterators.
 *
 * Bounds: a symbol costs one record (16 bytes plus its description);
 * the registry is an ordinary object; an iterator over a snapshot holds
 * its array. Nothing here allocates outside the realm's arena. */
#include "wf_jsi.h"

#define KEY(vm, s) js_intern((vm), (s), cslen(s))

static int cslen(const char *s) { int n = 0; while (s[n]) n++; return n; }

jv jv_sym(uint32_t r) { jv v = jv_undef(); v.t = JT_SYM; v.a = r; return v; }

uint32_t js_sym_new(js_vm_t *vm, uint32_t desc, uint32_t kind) {
    uint32_t len = desc ? JSTR(vm, desc)->len : 0, r, i;
    js_str_t *p;
    r = js_alloc_k(vm, 12u + len + 1u, JK_LEAF);
    if (!r) return 0;
    p = JSTR(vm, r);
    p->len = len;
    p->interned = kind;
    p->hash = (++vm->sym_serial & 0x7FFFFFFFu) | (desc ? 0u : JS_SYM_NODESC);
    if (desc) {
        js_str_t *d = JSTR(vm, desc);
        p = JSTR(vm, r);
        for (i = 0; i < len; i++) p->s[i] = d->s[i];
    }
    p->s[len] = 0;
    return r;
}

/* the description as a string ref (0 when Symbol() had none) */
static uint32_t sym_desc(js_vm_t *vm, uint32_t sym) {
    js_str_t *p = JSTR(vm, sym);
    if (p->hash & JS_SYM_NODESC) return 0;
    return js_str_new(vm, p->s, (int)p->len);
}

uint32_t js_sym_descr(js_vm_t *vm, uint32_t sym) {
    js_str_t *p = JSTR(vm, sym);
    uint32_t r, n = p->len, i;
    if (p->interned == JS_SYM_PRIVATE) return js_str_new(vm, p->s, (int)p->len);   /* "#x" */
    r = js_alloc_k(vm, 12u + n + 9u, JK_LEAF);
    if (!r) return vm->s_empty;
    {
        js_str_t *q = JSTR(vm, r);
        static const char pre[] = "Symbol(";
        p = JSTR(vm, sym);
        for (i = 0; i < 7; i++) q->s[i] = pre[i];
        for (i = 0; i < n; i++) q->s[7 + i] = p->s[i];
        q->s[7 + n] = ')';
        q->s[8 + n] = 0;
        q->len = n + 8u;
    }
    return r;
}

jv js_priv_missing(js_vm_t *vm, uint32_t key, int write) {
    uint32_t m = js_str_cat(vm, js_str_c(vm, write ? "Cannot write private member " : "Cannot read private member "),
                            js_str_new(vm, JSTR(vm, key)->s, (int)JSTR(vm, key)->len));
    m = js_str_cat(vm, m, js_str_c(vm, write ? " to an object whose class did not declare it"
                                              : " from an object whose class did not declare it"));
    return js_throw_err2(vm, JE_TYPE, "", m);
}

/* ---- Symbol and Symbol.prototype ---- */

/* the symbol a this value stands for (a symbol or Object(symbol)) */
static uint32_t this_sym(js_vm_t *vm, jv thisv) {
    if (thisv.t == JT_SYM) return thisv.a;
    if (thisv.t == JT_OBJ && JOBJ(vm, thisv.a)->cls == JS_CLASS_SYMBOL) return JOBJ(vm, thisv.a)->x1;
    js_throw_err(vm, JE_TYPE, "Symbol.prototype method called on a value that is not a symbol");
    return 0;
}

enum { SY_CALL = 0, SY_FOR, SY_KEYFOR, SY_TOSTRING, SY_VALUEOF, SY_DESC, SY_TOPRIM };

static jv sym_fn(js_vm_t *vm, jv thisv, jv *argv, int argc, uint32_t m) {
    switch (m) {
    case SY_CALL: {
        uint32_t d = 0, r;
        if (vm->in_new) return js_throw_err(vm, JE_TYPE, "Symbol is not a constructor");
        if (argc && argv[0].t != JT_UNDEF) {
            d = js_to_string(vm, argv[0]);
            if (vm->ctl) return jv_undef();
        }
        r = js_sym_new(vm, d, JS_SYM_PUBLIC);
        return r ? jv_sym(r) : jv_undef();
    }
    case SY_FOR: {
        uint32_t k = js_to_string(vm, JARG(0)), r;
        js_prop_t *p;
        if (vm->ctl) return jv_undef();
        k = js_intern_str(vm, k);
        if (vm->ctl) return jv_undef();
        p = js_own_prop(vm, vm->sym_registry, k);
        if (p) return p->v;
        r = js_sym_new(vm, k, JS_SYM_PUBLIC);
        if (!r) return jv_undef();
        js_define(vm, vm->sym_registry, k, jv_sym(r), P_HIDDEN);
        return jv_sym(r);
    }
    case SY_KEYFOR: {
        jv s = JARG(0);
        js_str_t *p;
        uint32_t k;
        js_prop_t *e;
        if (s.t != JT_SYM) return js_throw_err(vm, JE_TYPE, "Symbol.keyFor: the argument is not a symbol");
        p = JSTR(vm, s.a);
        if (p->hash & JS_SYM_NODESC) return jv_undef();
        k = js_intern_find(vm, p->s, (int)p->len);
        if (!k) return jv_undef();
        e = js_own_prop(vm, vm->sym_registry, k);
        if (e && e->v.t == JT_SYM && e->v.a == s.a) return jv_str(k);
        return jv_undef();
    }
    case SY_TOSTRING: {
        uint32_t s = this_sym(vm, thisv);
        if (!s) return jv_undef();
        return jv_str(js_sym_descr(vm, s));
    }
    case SY_VALUEOF: case SY_TOPRIM: {
        uint32_t s = this_sym(vm, thisv);
        return s ? jv_sym(s) : jv_undef();
    }
    case SY_DESC: {
        uint32_t s = this_sym(vm, thisv), d;
        if (!s) return jv_undef();
        d = sym_desc(vm, s);
        return d ? jv_str(d) : jv_undef();
    }
    default: return jv_undef();
    }
}

/* Object.getOwnPropertySymbols: the public symbol keys in creation order */
uint32_t js_own_symbols(js_vm_t *vm, uint32_t obj, int only_enum) {
    uint32_t out, i;
    if (JOBJ(vm, obj)->cls == JS_CLASS_PROXY) return js_proxy_keys(vm, obj, only_enum, 1);
    out = js_array_new(vm, 4);
    if (!out) return 0;
    for (i = 0; i < JOBJ(vm, obj)->nprop; i++) {
        js_prop_t *p = (js_prop_t *)JPTR(vm, JOBJ(vm, obj)->props) + i;
        if (JSTR(vm, p->key)->interned != JS_SYM_PUBLIC) continue;
        if (only_enum && !(p->flags & P_ENUM)) continue;
        if (!js_array_push(vm, out, jv_sym(p->key))) break;
    }
    return out;
}

static jv obj_syms(js_vm_t *vm, jv thisv, jv *argv, int argc, uint32_t m) {
    uint32_t o, r;
    (void)thisv; (void)m;
    o = js_to_object(vm, JARG(0));
    if (!o) return jv_undef();
    r = js_own_symbols(vm, o, 0);
    return r ? jv_obj(r) : jv_undef();
}

/* ---- iterators over a snapshot (JS_CLASS_ITER: x0 the array, x1 the next index) ---- */

uint32_t js_iter_over(js_vm_t *vm, uint32_t arr) {
    uint32_t it = js_obj_new(vm, JS_CLASS_ITER, vm->p_iter);
    if (!it) return 0;
    JOBJ(vm, it)->x0 = arr;
    JOBJ(vm, it)->x1 = 0;
    return it;
}

static uint32_t iter_result(js_vm_t *vm, jv v, int done) {
    uint32_t r = js_obj_new(vm, JS_CLASS_OBJECT, vm->p_object);
    if (!r) return 0;
    js_define(vm, r, vm->s_value, v, P_DEFAULT);
    js_define(vm, r, vm->s_done, jv_bool(done), P_DEFAULT);
    return r;
}

enum { IT_NEXT = 0, IT_SELF, IT_RETURN, IT_STRING };

static jv iter_fn(js_vm_t *vm, jv thisv, jv *argv, int argc, uint32_t m) {
    (void)argv; (void)argc;
    switch (m) {
    case IT_SELF:
        return thisv;
    case IT_NEXT: case IT_RETURN: {
        js_obj_t *o;
        uint32_t r;
        jv v = jv_undef();
        int done = 1;
        if (thisv.t != JT_OBJ || JOBJ(vm, thisv.a)->cls != JS_CLASS_ITER)
            return js_throw_err(vm, JE_TYPE, "next method called on an object that is not an iterator");
        o = JOBJ(vm, thisv.a);
        if (m == IT_RETURN) {
            o->x1 = 0xFFFFFFFFu;            /* closed: next() is done from now on */
            v = JARG(0);
        } else if (o->x0 && o->x1 < JOBJ(vm, o->x0)->alen) {
            v = js_array_get(vm, o->x0, o->x1);
            JOBJ(vm, thisv.a)->x1++;
            done = 0;
        } else {
            JOBJ(vm, thisv.a)->x1 = 0xFFFFFFFFu;
        }
        r = iter_result(vm, v, done);
        return r ? jv_obj(r) : jv_undef();
    }
    case IT_STRING: {                       /* String.prototype[Symbol.iterator]: code points */
        uint32_t s, arr, it;
        if (jv_is_nullish(thisv)) return js_throw_err(vm, JE_TYPE, "String.prototype[Symbol.iterator] called on null or undefined");
        s = js_to_string(vm, thisv);
        if (vm->ctl) return jv_undef();
        js_iter_values(vm, jv_str(s), &arr);
        if (vm->ctl || !arr) return jv_undef();
        it = js_iter_over(vm, arr);
        return it ? jv_obj(it) : jv_undef();
    }
    default: return jv_undef();
    }
}

/* ---- the protocol for user iterators ---- */

static int is_builtin_iter(js_vm_t *vm, jv m) {
    uint32_t r;
    if (m.t != JT_OBJ) return 0;
    r = m.a;
    return r == vm->fn_iter_values || r == vm->fn_iter_self || r == vm->fn_str_iter ||
           r == vm->fn_map_iter || r == vm->fn_set_iter;
}

int js_iter_custom(js_vm_t *vm, jv v, jv *it, jv *next) {
    jv m, i;
    if (v.t != JT_OBJ || !vm->sym_iterator) return 0;
    m = js_get(vm, v, vm->sym_iterator);
    if (vm->ctl || jv_is_nullish(m) || is_builtin_iter(vm, m)) return 0;
    if (!js_is_callable(vm, m)) { js_throw_err(vm, JE_TYPE, "Symbol.iterator is not a function"); return 0; }
    i = js_call(vm, m, v, 0, 0);
    if (vm->ctl) return 0;
    if (i.t != JT_OBJ) { js_throw_err(vm, JE_TYPE, "Result of the Symbol.iterator method is not an object"); return 0; }
    *next = js_get(vm, i, vm->s_next);
    if (vm->ctl) return 0;
    if (!js_is_callable(vm, *next)) { js_throw_err(vm, JE_TYPE, "The iterator has no next method"); return 0; }
    *it = i;
    return 1;
}

jv js_iter_step(js_vm_t *vm, jv it, jv next, int *done) {
    jv r, d, v;
    *done = 1;
    r = js_call(vm, next, it, 0, 0);
    if (vm->ctl) return jv_undef();
    if (r.t != JT_OBJ) return js_throw_err(vm, JE_TYPE, "Iterator result is not an object");
    d = js_get(vm, r, vm->s_done);
    if (vm->ctl) return jv_undef();
    if (js_truthy(vm, d)) return jv_undef();
    v = js_get(vm, r, vm->s_value);
    if (vm->ctl) return jv_undef();
    *done = 0;
    return v;
}

/* an early exit (break, return, throw): the iterator's return() runs.
 * A throw that was already leaving stays (an error inside return() is
 * dropped); after break or return an error thrown by return() replaces
 * them; an abort (budget, memory) skips return() altogether. */
void js_iter_close(js_vm_t *vm, jv it) {
    int sctl = vm->ctl;
    jv sexc = vm->exc, sret = vm->ret, f;
    uint32_t slabel = vm->label;
    if (sctl == CTL_ABORT) return;
    vm->ctl = CTL_NONE;
    f = js_get(vm, it, KEY(vm, "return"));
    if (!vm->ctl && js_is_callable(vm, f)) {
        jv r = js_call(vm, f, it, 0, 0);
        if (!vm->ctl && r.t != JT_OBJ && sctl != CTL_THROW)
            js_throw_err(vm, JE_TYPE, "The iterator's return() result is not an object");
    }
    if (vm->ctl == CTL_ABORT) return;
    if (vm->ctl == CTL_THROW && sctl != CTL_THROW) return;
    vm->ctl = sctl;
    vm->exc = sexc;
    vm->ret = sret;
    vm->label = slabel;
}

/* up to max values (max < 0: all) of an iterable; a user iterator or a
 * generator that still has values is closed (destructuring reads only
 * what the pattern needs). Other sources go through js_iter_values. */
uint32_t js_iter_take(js_vm_t *vm, jv v, int max) {
    uint32_t arr;
    jv it, nx;
    int n = 0, done = 0;
    if (js_is_gen(vm, v)) {
        arr = js_array_new(vm, 8);
        if (!arr) return 0;
        while ((max < 0 || n < max) && !vm->ctl) {
            jv x = js_gen_step(vm, v.a, 0, jv_undef(), &done);
            if (vm->ctl || done) break;
            js_array_push(vm, arr, x);
            n++;
        }
        if (!done && !vm->ctl) js_gen_close(vm, v.a);
        return vm->ctl ? 0 : arr;
    }
    if (js_iter_custom(vm, v, &it, &nx)) {
        arr = js_array_new(vm, 8);
        if (!arr) return 0;
        while ((max < 0 || n < max) && !vm->ctl) {
            jv x = js_iter_step(vm, it, nx, &done);
            if (vm->ctl || done) break;
            js_array_push(vm, arr, x);
            n++;
        }
        if (!done && !vm->ctl) js_iter_close(vm, it);
        return vm->ctl ? 0 : arr;
    }
    if (vm->ctl) return 0;
    js_iter_values(vm, v, &arr);
    return vm->ctl ? 0 : arr;
}

/* ---- setup ---- */

static uint32_t well_known(js_vm_t *vm, uint32_t sym_ctor, const char *name) {
    uint32_t d = js_str_c(vm, "Symbol."), r;
    d = js_str_cat(vm, d, js_str_c(vm, name));
    r = js_sym_new(vm, d, JS_SYM_PUBLIC);
    if (r) js_define(vm, sym_ctor, KEY(vm, name), jv_sym(r), 0);
    return r;
}

static uint32_t def_sym_method(js_vm_t *vm, uint32_t obj, uint32_t sym, const char *name, js_native_fn fn, uint32_t magic) {
    uint32_t f = js_native_new(vm, name, fn, 0, magic);
    if (!f) return 0;
    js_define(vm, obj, sym, jv_obj(f), P_HIDDEN);
    return f;
}

int js_sym_init(js_vm_t *vm) {
    uint32_t c, p, d, f;
    jv m;
    vm->s_description = KEY(vm, "description");
    vm->s_privm = KEY(vm, " privm");      /* not an identifier: no script can name it */
    p = vm->p_symbol = js_obj_new(vm, JS_CLASS_OBJECT, vm->p_object);
    if (!p) return -1;
    c = vm->c_symbol = js_ctor_new(vm, "Symbol", sym_fn, 0, p);
    if (!c) return -1;
    JOBJ(vm, c)->x1 = SY_CALL;
    js_def_native(vm, c, "for", sym_fn, 1, SY_FOR);
    js_def_native(vm, c, "keyFor", sym_fn, 1, SY_KEYFOR);
    vm->sym_registry = js_obj_new(vm, JS_CLASS_OBJECT, 0);
    vm->sym_iterator = well_known(vm, c, "iterator");
    vm->sym_async_iterator = well_known(vm, c, "asyncIterator");
    vm->sym_has_instance = well_known(vm, c, "hasInstance");
    vm->sym_to_primitive = well_known(vm, c, "toPrimitive");
    vm->sym_to_string_tag = well_known(vm, c, "toStringTag");
    vm->sym_species = well_known(vm, c, "species");
    vm->sym_unscopables = well_known(vm, c, "unscopables");
    vm->sym_is_concat_spreadable = well_known(vm, c, "isConcatSpreadable");
    vm->sym_match = well_known(vm, c, "match");
    if (vm->ctl == CTL_ABORT) return -1;
    js_def_native(vm, p, "toString", sym_fn, 0, SY_TOSTRING);
    js_def_native(vm, p, "valueOf", sym_fn, 0, SY_VALUEOF);
    d = js_native_new(vm, "description", sym_fn, 0, SY_DESC);
    if (!d) return -1;
    js_define_accessor(vm, p, vm->s_description, d, 0, P_CONF);
    def_sym_method(vm, p, vm->sym_to_primitive, "[Symbol.toPrimitive]", sym_fn, SY_TOPRIM);
    js_define(vm, p, vm->sym_to_string_tag, jv_str(KEY(vm, "Symbol")), P_CONF);
    js_def_native(vm, vm->c_object, "getOwnPropertySymbols", obj_syms, 1, 0);

    /* iterators: %IteratorPrototype% is p_iter (next, return, [Symbol.iterator]) */
    js_def_native(vm, vm->p_iter, "next", iter_fn, 0, IT_NEXT);
    js_def_native(vm, vm->p_iter, "return", iter_fn, 1, IT_RETURN);
    vm->fn_iter_self = def_sym_method(vm, vm->p_iter, vm->sym_iterator, "[Symbol.iterator]", iter_fn, IT_SELF);
    js_define(vm, vm->p_iter, vm->sym_to_string_tag, jv_str(KEY(vm, "Array Iterator")), P_CONF);
    if (vm->p_gen) {
        js_define(vm, vm->p_gen, vm->sym_iterator, jv_obj(vm->fn_iter_self), P_HIDDEN);
        js_define(vm, vm->p_gen, vm->sym_to_string_tag, jv_str(KEY(vm, "Generator")), P_CONF);
    }
    if (vm->p_promise) js_define(vm, vm->p_promise, vm->sym_to_string_tag, jv_str(KEY(vm, "Promise")), P_CONF);
    /* Array.prototype[Symbol.iterator] is Array.prototype.values */
    m = js_get(vm, jv_obj(vm->p_array), KEY(vm, "values"));
    if (m.t == JT_OBJ) {
        vm->fn_iter_values = m.a;
        js_define(vm, vm->p_array, vm->sym_iterator, m, P_HIDDEN);
    }
    vm->fn_str_iter = def_sym_method(vm, vm->p_string, vm->sym_iterator, "[Symbol.iterator]", iter_fn, IT_STRING);
    m = js_get(vm, jv_obj(vm->p_map), KEY(vm, "entries"));
    if (m.t == JT_OBJ) { vm->fn_map_iter = m.a; js_define(vm, vm->p_map, vm->sym_iterator, m, P_HIDDEN); }
    js_define(vm, vm->p_map, vm->sym_to_string_tag, jv_str(KEY(vm, "Map")), P_CONF);
    m = js_get(vm, jv_obj(vm->p_set), KEY(vm, "values"));
    if (m.t == JT_OBJ) { vm->fn_set_iter = m.a; js_define(vm, vm->p_set, vm->sym_iterator, m, P_HIDDEN); }
    js_define(vm, vm->p_set, vm->sym_to_string_tag, jv_str(KEY(vm, "Set")), P_CONF);
    f = vm->fn_iter_values;
    (void)f;
    return vm->ctl == CTL_ABORT ? -1 : 0;
}
