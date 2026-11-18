/* wf_jsrun.c - Lot 113: the tree-walking interpreter of the JavaScript
 * subset (see wf_js.h). Control flow (return, break, continue, throw,
 * abort) is vm->ctl, checked after every sub-evaluation. Environments
 * that no closure captured are recycled when their scope ends, so calls
 * in loops do not fill the arena while a script runs (wf_jsgc.c only
 * reclaims between runs, Lot 115). */
#include "wf_jsi.h"

static jv ev(js_vm_t *vm, uint32_t env, uint32_t n);
static void ex(js_vm_t *vm, uint32_t env, uint32_t n);
static void ex_list(js_vm_t *vm, uint32_t env, uint32_t list);
static jv call_func(js_vm_t *vm, uint32_t f, jv thisv, jv *argv, int argc, uint32_t newtarget);
static void bind_pattern(js_vm_t *vm, uint32_t env, uint32_t pat, jv v, int mode);
static jv eval_class(js_vm_t *vm, uint32_t env, uint32_t n);
static void init_fields(js_vm_t *vm, uint32_t fobj, jv thisv);

#define NODE(r) JNODE(vm, (r))
#define BIND_ASSIGN (-1)

void js_step(js_vm_t *vm) {
    if (++vm->steps > vm->budget) js_abort(vm, "RangeError: script step budget exhausted (guest bound)");
}

static int stack_low(js_vm_t *vm) {
    char probe;
    return vm->stack_base && (uint32_t)((uint8_t *)&probe - vm->stack_base) < JS_STACK_RESERVE;
}

/* Lot 114: a call is refused (a RangeError the script can catch) while
 * JS_CALL_MARGIN more bytes are left than the abort threshold of ev;
 * deep recursion on a coroutine stack then fails like it does on the
 * main stack instead of ending the run */
static int stack_low_call(js_vm_t *vm) {
    char probe;
    return vm->stack_base && (uint32_t)((uint8_t *)&probe - vm->stack_base) < JS_STACK_RESERVE + JS_CALL_MARGIN;
}

/* ---- environments ---- */

uint32_t js_env_new(js_vm_t *vm, uint32_t parent, int kind) {
    uint32_t r = vm->free_env;
    js_env_t *e;
    if (r) {
        int i;
        vm->free_env = JENV(vm, r)->parent;
        for (i = 0; i < (int)sizeof(js_env_t); i++) vm->mem[r + (uint32_t)i] = 0;
    } else {
        r = js_alloc_k(vm, sizeof(js_env_t), JK_ENV);
        if (!r) return 0;
    }
    e = JENV(vm, r);
    e->parent = parent;
    e->kind = (uint32_t)kind;
    return r;
}

static int vars_class(uint32_t cap) {
    int k = 0;
    while ((4u << k) < cap && k < 7) k++;
    return (4u << k) == cap ? k : -1;
}

static void env_free(js_vm_t *vm, uint32_t env) {
    js_env_t *e;
    if (!env) return;
    e = JENV(vm, env);
    if (e->kind & E_CAPTURED) return;
    if (e->vars) {
        int k = vars_class(e->cap);
        if (k >= 0) {
            *(uint32_t *)JPTR(vm, e->vars) = vm->free_vars[k];
            vm->free_vars[k] = e->vars;
        }
    }
    e->vars = 0;
    e->parent = vm->free_env;
    vm->free_env = env;
}

static uint32_t vars_alloc(js_vm_t *vm, uint32_t cap) {
    int k = vars_class(cap);
    uint32_t r;
    if (k >= 0 && vm->free_vars[k]) {
        uint32_t i;
        r = vm->free_vars[k];
        vm->free_vars[k] = *(uint32_t *)JPTR(vm, r);
        for (i = 0; i < cap * (uint32_t)sizeof(js_prop_t); i++) vm->mem[r + i] = 0;
        return r;
    }
    return js_alloc_k(vm, cap * (uint32_t)sizeof(js_prop_t), JK_PROPS);
}

static void env_capture(js_vm_t *vm, uint32_t env) {
    while (env) {
        js_env_t *e = JENV(vm, env);
        if (e->kind & E_CAPTURED) break;
        e->kind |= E_CAPTURED;
        env = e->parent;
    }
}

static js_prop_t *env_own(js_vm_t *vm, uint32_t env, uint32_t name) {
    js_env_t *e = JENV(vm, env);
    js_prop_t *v;
    uint32_t i;
    if (!e->vars) return 0;
    v = (js_prop_t *)JPTR(vm, e->vars);
    for (i = 0; i < e->n; i++) if (v[i].key == name) return &v[i];
    return 0;
}

int js_env_declare(js_vm_t *vm, uint32_t env, uint32_t name, jv v, uint32_t flags) {
    js_env_t *e;
    js_prop_t *p = env_own(vm, env, name);
    if (p) { p->v = v; p->flags = flags; return 1; }
    e = JENV(vm, env);
    if (e->n >= e->cap) {
        uint32_t cap = e->cap ? (uint32_t)e->cap * 2u : 4u, r, i;
        js_prop_t *src, *dst;
        if (cap > 16384u) { js_throw_err(vm, JE_RANGE, "too many variables in one scope"); return 0; }
        r = vars_alloc(vm, cap);
        if (!r) return 0;
        e = JENV(vm, env);
        src = (js_prop_t *)JPTR(vm, e->vars);
        dst = (js_prop_t *)JPTR(vm, r);
        for (i = 0; i < e->n; i++) dst[i] = src[i];
        if (e->vars) {
            int k = vars_class(e->cap);
            if (k >= 0) { *(uint32_t *)JPTR(vm, e->vars) = vm->free_vars[k]; vm->free_vars[k] = e->vars; }
        }
        e->vars = r;
        e->cap = (uint16_t)cap;
    }
    p = (js_prop_t *)JPTR(vm, e->vars) + e->n;
    p->key = name;
    p->flags = flags;
    p->v = v;
    e->n++;
    return 1;
}

/* the scope that holds `var`s: the nearest function or the global env */
static uint32_t var_scope(js_vm_t *vm, uint32_t env) {
    while (env) {
        int k = (int)E_KIND(JENV(vm, env)->kind);
        if (k == E_FUNC || k == E_ARROW || k == E_GLOBAL) return env;
        env = JENV(vm, env)->parent;
    }
    return vm->genv;
}

/* the nearest environment that binds this (not an arrow) */
static uint32_t this_scope(js_vm_t *vm, uint32_t env) {
    while (env) {
        int k = (int)E_KIND(JENV(vm, env)->kind);
        if (k == E_FUNC || k == E_GLOBAL) return env;
        env = JENV(vm, env)->parent;
    }
    return vm->genv;
}

static jv lookup(js_vm_t *vm, uint32_t env, uint32_t name, int typeof_mode) {
    uint32_t e = env;
    while (e) {
        js_prop_t *p = env_own(vm, e, name);
        if (p) {
            if (p->flags & V_TDZ) {
                return js_throw_err2(vm, JE_REFERENCE, "Cannot access variable before initialization: ", name);
            }
            return p->v;
        }
        e = JENV(vm, e)->parent;
    }
    if (js_has(vm, vm->global, name)) return js_get(vm, jv_obj(vm->global), name);
    if (typeof_mode) return jv_undef();
    return js_throw_err2(vm, JE_REFERENCE, "", js_str_cat(vm, name, js_str_c(vm, " is not defined")));
}

static void assign_var(js_vm_t *vm, uint32_t env, uint32_t name, jv v) {
    uint32_t e = env;
    while (e) {
        js_prop_t *p = env_own(vm, e, name);
        if (p) {
            if (p->flags & V_TDZ) { js_throw_err2(vm, JE_REFERENCE, "Cannot access variable before initialization: ", name); return; }
            if (p->flags & V_CONST) { js_throw_err(vm, JE_TYPE, "Assignment to constant variable."); return; }
            p->v = v;
            return;
        }
        e = JENV(vm, e)->parent;
    }
    js_put(vm, jv_obj(vm->global), name, v);   /* sloppy mode: an implicit global */
}

/* var x: declared once in the var scope (the global object at top level) */
static void declare_var(js_vm_t *vm, uint32_t env, uint32_t name) {
    uint32_t vs = var_scope(vm, env);
    if (E_KIND(JENV(vm, vs)->kind) == E_GLOBAL) {
        if (!js_has(vm, vm->global, name)) js_define(vm, vm->global, name, jv_undef(), P_ENUM | P_WRITE);
        return;
    }
    if (!env_own(vm, vs, name)) js_env_declare(vm, vs, name, jv_undef(), 0);
}

static void set_var_scope(js_vm_t *vm, uint32_t env, uint32_t name, jv v) {
    uint32_t vs = var_scope(vm, env);
    if (E_KIND(JENV(vm, vs)->kind) == E_GLOBAL) { js_put(vm, jv_obj(vm->global), name, v); return; }
    js_env_declare(vm, vs, name, v, 0);
}

/* ---- functions ---- */

uint32_t js_closure(js_vm_t *vm, uint32_t fnode, uint32_t env) {
    uint32_t f = js_obj_new(vm, JS_CLASS_FUNC, vm->p_function);
    if (!f) return 0;
    JOBJ(vm, f)->x0 = fnode;
    JOBJ(vm, f)->x1 = env;
    env_capture(vm, env);
    return f;
}

static void set_fn_name(js_vm_t *vm, jv v, uint32_t name) {
    js_obj_t *o;
    if (v.t != JT_OBJ || !name) return;
    o = JOBJ(vm, v.a);
    if (o->cls != JS_CLASS_FUNC || o->x2 || NODE(o->x0)->a) return;
    o->x2 = name;
}

/* the function declarations of a statement list, made in env */
static void hoist_funcs(js_vm_t *vm, uint32_t env, uint32_t list, int also_var_scope) {
    uint32_t s;
    for (s = list; s && !vm->ctl; s = NODE(s)->next) {
        uint32_t fn, f;
        if (NODE(s)->k != N_FUNCDECL) continue;
        fn = NODE(s)->a;
        f = js_closure(vm, fn, env);
        if (!f) return;
        if (E_KIND(JENV(vm, env)->kind) == E_GLOBAL) js_put(vm, jv_obj(vm->global), NODE(fn)->a, jv_obj(f));
        else {
            js_env_declare(vm, env, NODE(fn)->a, jv_obj(f), 0);
            if (also_var_scope) set_var_scope(vm, JENV(vm, env)->parent, NODE(fn)->a, jv_obj(f));
        }
    }
}

static uint32_t make_arguments(js_vm_t *vm, uint32_t f, jv *argv, int argc) {
    uint32_t a = js_obj_new(vm, JS_CLASS_ARGS, vm->p_object), arr;
    int i;
    if (!a) return 0;
    arr = js_alloc_k(vm, (uint32_t)(argc ? argc : 1) * (uint32_t)sizeof(jv), JK_JVS);
    if (!arr) return 0;
    for (i = 0; i < argc; i++) ((jv *)JPTR(vm, arr))[i] = argv[i];
    JOBJ(vm, a)->arr = arr;
    JOBJ(vm, a)->alen = (uint32_t)argc;
    JOBJ(vm, a)->acap = (uint32_t)(argc ? argc : 1);
    js_define(vm, a, vm->s_callee, jv_obj(f), P_HIDDEN);
    return a;
}

static jv call_func(js_vm_t *vm, uint32_t f, jv thisv, jv *argv, int argc, uint32_t newtarget) {
    js_obj_t *o = JOBJ(vm, f);
    uint32_t fn = o->x0, env, p;
    js_node_t *n = NODE(fn);
    int i, arrow = (n->fl & F_ARROW) != 0;
    jv r = jv_undef();
    if (vm->depth >= JS_MAX_DEPTH || stack_low_call(vm))
        return js_throw_err(vm, JE_RANGE, "Maximum call stack size exceeded (guest bound)");
    env = js_env_new(vm, o->x1, arrow ? E_ARROW : E_FUNC);
    if (!env) return jv_undef();
    vm->depth++;
    if (!arrow) {
        JENV(vm, env)->thisv = jv_is_nullish(thisv) ? jv_obj(vm->global) : thisv;
        JENV(vm, env)->fobj = f;
        JENV(vm, env)->newtarget = newtarget;
    }
    n = NODE(fn);
    if (n->a && !arrow && !(n->fl & F_METHOD)) js_env_declare(vm, env, n->a, jv_obj(f), 0);
    /* parameters */
    for (p = n->b, i = 0; p && !vm->ctl; p = NODE(p)->next, i++) {
        js_node_t *pn = NODE(p);
        jv v;
        if (pn->fl & F_REST) {
            uint32_t arr = js_array_new(vm, (uint32_t)(argc > i ? argc - i : 0));
            int k;
            if (!arr) break;
            for (k = i; k < argc; k++) js_array_push(vm, arr, argv[k]);
            v = jv_obj(arr);
        } else {
            v = i < argc ? argv[i] : jv_undef();
            if (v.t == JT_UNDEF && pn->b) {
                v = ev(vm, env, pn->b);
                if (vm->ctl) break;
                if (NODE(pn->a)->k == N_IDENT) set_fn_name(vm, v, NODE(pn->a)->a);
            }
        }
        bind_pattern(vm, env, pn->a, v, DK_LET);
    }
    n = NODE(fn);
    if (!vm->ctl && (n->fl & F_ARGS) && !arrow) {
        uint32_t a = make_arguments(vm, f, argv, argc);
        if (a && !env_own(vm, env, vm->s_arguments)) js_env_declare(vm, env, vm->s_arguments, jv_obj(a), 0);
    }
    for (p = NODE(fn)->d; p && !vm->ctl; p = NODE(p)->next)
        if (!env_own(vm, env, NODE(p)->a)) js_env_declare(vm, env, NODE(p)->a, jv_undef(), 0);
    if (!vm->ctl && !(NODE(fn)->fl & F_EXPRBODY)) hoist_funcs(vm, env, NODE(fn)->c, 0);
    /* Lot 114: a generator returns its object, an async function its
     * promise; the body runs on a coroutine stack (wf_jsco.c) and keeps
     * this environment, so it is never recycled */
    if (NODE(fn)->op & (FK_GEN | FK_ASYNC)) {
        int is_gen = (NODE(fn)->op & FK_GEN) != 0;
        vm->depth--;
        if (vm->ctl) return is_gen ? jv_undef() : js_async_rejected(vm);
        env_capture(vm, env);
        if (is_gen && (NODE(fn)->op & FK_ASYNC)) r = js_agen_new(vm, f, env);     /* Lot 115 */
        else r = is_gen ? js_gen_new(vm, f, env) : js_async_call(vm, f, env);
        return vm->ctl ? jv_undef() : r;
    }
    /* a base class constructor sets its fields before its body */
    if (!vm->ctl && (FN_FLAGS(JOBJ(vm, f)) & FN_CLASSCTOR) && !(FN_FLAGS(JOBJ(vm, f)) & FN_DERIVED))
        init_fields(vm, f, JENV(vm, env)->thisv);
    if (!vm->ctl && (FN_FLAGS(JOBJ(vm, f)) & FN_DEFAULT) && (FN_FLAGS(JOBJ(vm, f)) & FN_DERIVED)) {
        /* constructor(...args) { super(...args); } */
        uint32_t parent = JOBJ(vm, f)->proto;
        jv pv = jv_obj(parent);
        if (js_is_callable(vm, pv)) {
            if (JOBJ(vm, parent)->cls == JS_CLASS_FUNC) {
                jv pr = call_func(vm, parent, JENV(vm, env)->thisv, argv, argc, newtarget);
                if (!vm->ctl && pr.t == JT_OBJ) JENV(vm, env)->thisv = pr;     /* Lot 115 */
            } else {
                uint32_t sv = vm->super_this;
                vm->super_this = JENV(vm, env)->thisv.a;
                vm->in_new = 1;
                (void)js_call(vm, pv, JENV(vm, env)->thisv, argv, argc);
                vm->in_new = 0;
                vm->super_this = sv;
            }
            if (!vm->ctl) init_fields(vm, f, JENV(vm, env)->thisv);
        }
    } else if (!vm->ctl) {
        if (NODE(fn)->fl & F_EXPRBODY) {
            r = ev(vm, env, NODE(fn)->c);
        } else {
            ex_list(vm, env, NODE(fn)->c);
            if (vm->ctl == CTL_RETURN) { r = vm->ret; vm->ctl = CTL_NONE; }
            else if (vm->ctl == CTL_BREAK || vm->ctl == CTL_CONTINUE) vm->ctl = CTL_NONE;
        }
    }
    /* Lot 115: a derived constructor gives its this (super() may have replaced it) */
    if (!vm->ctl && r.t != JT_OBJ && (FN_FLAGS(JOBJ(vm, f)) & FN_CLASSCTOR) && (FN_FLAGS(JOBJ(vm, f)) & FN_DERIVED) &&
        JENV(vm, env)->thisv.t == JT_OBJ)
        r = JENV(vm, env)->thisv;
    vm->depth--;
    env_free(vm, env);
    return vm->ctl ? jv_undef() : r;
}

/* Lot 114: the body of f in its prepared environment (a coroutine runs
 * this on its own stack); return becomes the value */
jv js_fn_body(js_vm_t *vm, uint32_t f, uint32_t env) {
    uint32_t fn = JOBJ(vm, f)->x0;
    jv r = jv_undef();
    if (NODE(fn)->fl & F_EXPRBODY) {
        r = ev(vm, env, NODE(fn)->c);
    } else {
        ex_list(vm, env, NODE(fn)->c);
        if (vm->ctl == CTL_RETURN) { r = vm->ret; vm->ctl = CTL_NONE; }
        else if (vm->ctl == CTL_BREAK || vm->ctl == CTL_CONTINUE) vm->ctl = CTL_NONE;
    }
    return vm->ctl ? jv_undef() : r;
}

jv js_call(js_vm_t *vm, jv fn, jv thisv, jv *argv, int argc) {
    js_obj_t *o;
    if (vm->ctl) return jv_undef();
    if (!js_is_callable(vm, fn)) return js_throw_err(vm, JE_TYPE, "value is not a function");
    js_step(vm);
    o = JOBJ(vm, fn.a);
    if (o->cls == JS_CLASS_PROXY) return js_proxy_call(vm, fn.a, thisv, argv, argc);
    if (o->cls == JS_CLASS_NATIVE) {
        int saved_new = vm->in_new;
        jv r;
        if (vm->depth >= JS_MAX_DEPTH || stack_low_call(vm))
            return js_throw_err(vm, JE_RANGE, "Maximum call stack size exceeded (guest bound)");
        vm->depth++;
        r = vm->nat[o->x0](vm, thisv, argv, argc, o->x1);
        vm->depth--;
        vm->in_new = saved_new;
        return vm->ctl ? jv_undef() : r;
    }
    if (o->cls == JS_CLASS_BOUND) {
        uint32_t blk = o->x1, nb = JOBJ(vm, fn.a)->x2, k;
        jv *bound = (jv *)JPTR(vm, blk);
        jv local[16], *all = local;
        int total = (int)nb + argc, i;
        if (total > 16) {
            uint32_t r = js_alloc_k(vm, (uint32_t)total * (uint32_t)sizeof(jv), JK_JVS);
            if (!r) return jv_undef();
            all = (jv *)JPTR(vm, r);
            bound = (jv *)JPTR(vm, blk);
        }
        for (k = 0; k < nb; k++) all[k] = bound[1 + k];
        for (i = 0; i < argc; i++) all[nb + (uint32_t)i] = argv[i];
        return js_call(vm, jv_obj(o->x0), bound[0], all, total);
    }
    if (FN_FLAGS(o) & FN_CLASSCTOR) {
        return js_throw_err2(vm, JE_TYPE, "Class constructor cannot be invoked without 'new': ",
                             NODE(o->x0)->a ? NODE(o->x0)->a : vm->s_empty);
    }
    return call_func(vm, fn.a, thisv, argv, argc, 0);
}

jv js_construct(js_vm_t *vm, jv ctor, jv *argv, int argc, jv newtarget) {
    js_obj_t *o;
    jv protov, r;
    uint32_t obj;
    if (vm->ctl) return jv_undef();
    if (!js_is_callable(vm, ctor)) return js_throw_err(vm, JE_TYPE, "value is not a constructor");
    o = JOBJ(vm, ctor.a);
    if (o->cls == JS_CLASS_PROXY) return js_proxy_construct(vm, ctor.a, argv, argc, newtarget);
    if (o->cls == JS_CLASS_BOUND) {
        uint32_t blk = o->x1, nb = o->x2, k;
        jv local[16], *all = local, target = jv_obj(o->x0);
        int total = (int)nb + argc, i;
        jv *bound = (jv *)JPTR(vm, blk);
        if (total > 16) return js_throw_err(vm, JE_RANGE, "too many arguments for a bound constructor");
        for (k = 0; k < nb; k++) all[k] = bound[1 + k];
        for (i = 0; i < argc; i++) all[nb + (uint32_t)i] = argv[i];
        return js_construct(vm, target, all, total, newtarget.a == ctor.a ? target : newtarget);
    }
    if (o->cls == JS_CLASS_NATIVE) {
        int saved = vm->in_new;
        vm->in_new = 1;
        r = js_call(vm, ctor, jv_undef(), argv, argc);
        vm->in_new = saved;
        if (!vm->ctl && r.t == JT_OBJ && newtarget.a != ctor.a) {
            /* new on a subclass of a native: give the object the subclass prototype */
            jv pv = js_get(vm, newtarget, vm->s_prototype);
            if (!vm->ctl && pv.t == JT_OBJ) JOBJ(vm, r.a)->proto = pv.a;
        }
        return r;
    }
    if (NODE(o->x0)->op & (FK_GEN | FK_ASYNC))
        return js_throw_err2(vm, JE_TYPE, "is not a constructor: ", NODE(o->x0)->a ? NODE(o->x0)->a : vm->s_empty);
    if (NODE(o->x0)->fl & (F_ARROW | F_METHOD)) {
        if (!(NODE(o->x0)->fl & F_CTOR))
            return js_throw_err2(vm, JE_TYPE, "is not a constructor: ", NODE(o->x0)->a ? NODE(o->x0)->a : vm->s_empty);
    }
    protov = js_get(vm, newtarget, vm->s_prototype);
    if (vm->ctl) return jv_undef();
    obj = js_obj_new(vm, JS_CLASS_OBJECT, protov.t == JT_OBJ ? protov.a : vm->p_object);
    if (!obj) return jv_undef();
    r = call_func(vm, ctor.a, jv_obj(obj), argv, argc, newtarget.a);
    if (vm->ctl) return jv_undef();
    return r.t == JT_OBJ ? r : jv_obj(obj);
}

/* class instance fields of fobj's class on thisv */
static void init_fields(js_vm_t *vm, uint32_t fobj, jv thisv) {
    uint32_t cls = FN_CLASS(JOBJ(vm, fobj)), m;
    uint32_t env;
    if (!cls || thisv.t != JT_OBJ) return;
    {   /* Lot 115: the private methods and accessors first (kept on the class's holder) */
        js_prop_t *hp = env_own(vm, JOBJ(vm, fobj)->x1, vm->s_privm);
        if (hp && hp->v.t == JT_OBJ) {
            uint32_t h = hp->v.a, i;
            for (i = 0; i < JOBJ(vm, h)->nprop && !vm->ctl; i++) {
                js_prop_t q = ((js_prop_t *)JPTR(vm, JOBJ(vm, h)->props))[i];
                if (js_own_prop(vm, thisv.a, q.key)) {
                    js_throw_err(vm, JE_TYPE, "Cannot initialize private methods of a class twice on the same object");
                    return;
                }
                js_define(vm, thisv.a, q.key, q.v, q.flags);
            }
            if (vm->ctl) return;
        }
    }
    env = js_env_new(vm, JOBJ(vm, fobj)->x1, E_FUNC);
    if (!env) return;
    JENV(vm, env)->thisv = thisv;
    JENV(vm, env)->fobj = fobj;
    for (m = NODE(cls)->c; m && !vm->ctl; m = NODE(m)->next) {
        js_node_t *mn = NODE(m);
        uint32_t key;
        int priv = mn->op == MK_PRIVATE;
        jv v = jv_undef();
        if (mn->k != N_FIELD || (mn->fl & F_STATIC)) continue;
        if (priv) {
            jv pk = lookup(vm, env, mn->a, 0);
            if (vm->ctl) break;
            key = pk.a;
            if (js_own_prop(vm, thisv.a, key)) {
                js_throw_err2(vm, JE_TYPE, "Cannot initialize a private field twice on the same object: ", mn->a);
                break;
            }
        } else if (mn->fl & F_COMPUTED) {
            /* computed field keys were evaluated once with the class: kept in d */
            key = mn->d;
        } else key = mn->a;
        mn = NODE(m);
        if (mn->b) { v = ev(vm, env, mn->b); if (vm->ctl) break; set_fn_name(vm, v, priv ? NODE(m)->a : key); }
        js_define(vm, thisv.a, key, v, priv ? P_WRITE : P_DEFAULT);
    }
    env_free(vm, env);
}

/* ---- patterns ---- */

/* mode: BIND_ASSIGN (assignment), DK_VAR, DK_LET or DK_CONST */
static void bind_name(js_vm_t *vm, uint32_t env, uint32_t name, jv v, int mode) {
    if (mode == BIND_ASSIGN) assign_var(vm, env, name, v);
    else if (mode == DK_VAR) set_var_scope(vm, env, name, v);
    else js_env_declare(vm, env, name, v, mode == DK_CONST ? V_CONST : V_LET);
}

static void assign_target(js_vm_t *vm, uint32_t env, uint32_t t, jv v, int mode) {
    js_node_t *n = NODE(t);
    switch (n->k) {
    case N_IDENT: bind_name(vm, env, n->a, v, mode); return;
    case N_MEMBER: {
        jv o = ev(vm, env, n->a);
        if (vm->ctl) return;
        js_put(vm, o, NODE(t)->b, v);
        return;
    }
    case N_INDEX: {
        jv o = ev(vm, env, n->a), k;
        if (vm->ctl) return;
        k = ev(vm, env, NODE(t)->b);
        if (vm->ctl) return;
        js_put_v(vm, o, k, v);
        return;
    }
    case N_OPAT: case N_APAT: bind_pattern(vm, env, t, v, mode); return;
    case N_PELEM:
        if (v.t == JT_UNDEF && n->c) { v = ev(vm, env, n->c); if (vm->ctl) return; }
        assign_target(vm, env, NODE(t)->b, v, mode);
        return;
    default:
        js_throw_err(vm, JE_SYNTAX, "Invalid destructuring target");
    }
}

static void bind_pattern(js_vm_t *vm, uint32_t env, uint32_t pat, jv v, int mode) {
    js_node_t *n = NODE(pat);
    uint32_t e;
    if (n->k == N_IDENT) { bind_name(vm, env, n->a, v, mode); return; }
    if (n->k != N_OPAT && n->k != N_APAT) { assign_target(vm, env, pat, v, mode); return; }
    if (jv_is_nullish(v)) { js_throw_err(vm, JE_TYPE, "Cannot destructure undefined or null"); return; }
    if (n->k == N_OPAT) {
        uint32_t used = 0;
        for (e = n->a; e && !vm->ctl; e = NODE(e)->next) {
            js_node_t *pp = NODE(e);
            jv val;
            uint32_t key;
            if (pp->fl & F_REST) {
                uint32_t rest = js_obj_new(vm, JS_CLASS_OBJECT, vm->p_object), keys, i;
                if (!rest || v.t != JT_OBJ) { if (rest) assign_target(vm, env, pp->b, jv_obj(rest), mode); continue; }
                keys = js_own_keys(vm, v.a, 1);
                for (i = 0; keys && i < JOBJ(vm, keys)->alen && !vm->ctl; i++) {
                    jv k = js_array_get(vm, keys, i);
                    uint32_t u, skip = 0;
                    for (u = used; u; u = NODE(u)->next) if (NODE(u)->a == k.a) { skip = 1; break; }
                    if (!skip) js_define(vm, rest, k.a, js_get(vm, v, k.a), P_DEFAULT);
                }
                assign_target(vm, env, NODE(e)->b, jv_obj(rest), mode);
                continue;
            }
            if (pp->fl & F_COMPUTED) {
                jv kv = ev(vm, env, pp->a);
                if (vm->ctl) return;
                key = js_to_key(vm, kv);
            } else key = pp->a;
            if (vm->ctl) return;
            {   /* remember the key for a later ...rest */
                uint32_t u = js_node_new(vm, N_VARNAME, 0);
                if (!u) return;
                NODE(u)->a = key;
                NODE(u)->next = used;
                used = u;
            }
            val = js_get(vm, v, key);
            if (vm->ctl) return;
            pp = NODE(e);
            if (val.t == JT_UNDEF && pp->c) {
                val = ev(vm, env, pp->c);
                if (vm->ctl) return;
                if (NODE(NODE(e)->b)->k == N_IDENT) set_fn_name(vm, val, NODE(NODE(e)->b)->a);
            }
            assign_target(vm, env, NODE(e)->b, val, mode);
        }
        return;
    }
    {
        uint32_t arr = 0, i = 0;
        int need = 0;
        /* Lot 115: a user iterator or a generator gives only what the
         * pattern reads, then it is closed */
        for (e = n->a; e; e = NODE(e)->next) { if (NODE(e)->fl & F_REST) { need = -1; break; } need++; }
        arr = js_iter_take(vm, v, need);
        if (vm->ctl || !arr) return;
        for (e = n->a; e && !vm->ctl; e = NODE(e)->next, i++) {
            js_node_t *pe = NODE(e);
            jv val;
            if (pe->fl & F_REST) {
                uint32_t rest = js_array_new(vm, 0), k;
                if (!rest) return;
                for (k = i; k < JOBJ(vm, arr)->alen; k++) js_array_push(vm, rest, js_array_get(vm, arr, k));
                assign_target(vm, env, NODE(e)->b, jv_obj(rest), mode);
                break;
            }
            if (!pe->b) continue;      /* hole */
            val = js_array_get(vm, arr, i);
            if (val.t == JT_UNDEF && pe->c) {
                val = ev(vm, env, pe->c);
                if (vm->ctl) return;
            }
            assign_target(vm, env, NODE(e)->b, val, mode);
        }
    }
}

/* ---- operators ---- */

static jv num_result(jf64 r) { return jv_num(r); }

static jv arith(js_vm_t *vm, int op, jv a, jv b) {
    jf64 x, y;
    if (op == OP_ADD) {
        if (a.t == JT_INT && b.t == JT_INT) {
            int64_t s = (int64_t)(int32_t)a.a + (int64_t)(int32_t)b.a;
            if (s >= -2147483647LL - 1 && s <= 2147483647LL) return jv_int((int32_t)s);
            return jv_num(jf_from_i64(s));
        }
        if (a.t == JT_STR && b.t == JT_STR) return jv_str(js_str_cat(vm, a.a, b.a));
        a = js_to_primitive(vm, a, 0);
        if (vm->ctl) return jv_undef();
        b = js_to_primitive(vm, b, 0);
        if (vm->ctl) return jv_undef();
        if (a.t == JT_STR || b.t == JT_STR) {
            uint32_t sa = js_to_string(vm, a), sb;
            if (vm->ctl) return jv_undef();
            sb = js_to_string(vm, b);
            if (vm->ctl) return jv_undef();
            return jv_str(js_str_cat(vm, sa, sb));
        }
    }
    if (a.t == JT_INT && b.t == JT_INT) {
        int32_t i = (int32_t)a.a, j = (int32_t)b.a;
        switch (op) {
        case OP_SUB: {
            int64_t s = (int64_t)i - (int64_t)j;
            if (s >= -2147483647LL - 1 && s <= 2147483647LL) return jv_int((int32_t)s);
            return jv_num(jf_from_i64(s));
        }
        case OP_MUL: {
            int64_t s = (int64_t)i * (int64_t)j;
            if (s == 0 && (i < 0 || j < 0)) return jv_num(JF_NZERO);
            if (s >= -2147483647LL - 1 && s <= 2147483647LL) return jv_int((int32_t)s);
            return jv_num(jf_from_i64(s));
        }
        case OP_MOD:
            if (j != 0 && !(i == -2147483647 - 1 && j == -1)) {
                int32_t r = i % j;
                if (r == 0 && i < 0) return jv_num(JF_NZERO);
                return jv_int(r);
            }
            break;
        case OP_BAND: return jv_int(i & j);
        case OP_BOR: return jv_int(i | j);
        case OP_BXOR: return jv_int(i ^ j);
        case OP_SHL: return jv_int((int32_t)((uint32_t)i << (j & 31)));
        case OP_SAR: return jv_int(i >> (j & 31));
        case OP_SHR: return jv_num(jf_from_u32((uint32_t)i >> (j & 31)));
        default: break;
        }
    }
    x = js_to_number(vm, a);
    if (vm->ctl) return jv_undef();
    y = js_to_number(vm, b);
    if (vm->ctl) return jv_undef();
    switch (op) {
    case OP_ADD: return num_result(jf_add(x, y));
    case OP_SUB: return num_result(jf_sub(x, y));
    case OP_MUL: return num_result(jf_mul(x, y));
    case OP_DIV: return num_result(jf_div(x, y));
    case OP_MOD: return num_result(jf_rem(x, y));
    case OP_EXP: return num_result(jf_pow(x, y));
    case OP_BAND: return jv_int(jf_to_i32(x) & jf_to_i32(y));
    case OP_BOR: return jv_int(jf_to_i32(x) | jf_to_i32(y));
    case OP_BXOR: return jv_int(jf_to_i32(x) ^ jf_to_i32(y));
    case OP_SHL: return jv_int((int32_t)(jf_to_u32(x) << (jf_to_u32(y) & 31)));
    case OP_SAR: return jv_int(jf_to_i32(x) >> (jf_to_u32(y) & 31));
    case OP_SHR: return jv_num(jf_from_u32(jf_to_u32(x) >> (jf_to_u32(y) & 31)));
    default: return jv_undef();
    }
}

/* a < b: 1, 0, or -1 when undefined (NaN) */
static int less_than(js_vm_t *vm, jv a, jv b, int left_first) {
    jf64 x, y;
    if (a.t == JT_INT && b.t == JT_INT) return (int32_t)a.a < (int32_t)b.a;
    if (left_first) {
        a = js_to_primitive(vm, a, 2); if (vm->ctl) return 0;
        b = js_to_primitive(vm, b, 2); if (vm->ctl) return 0;
    } else {
        b = js_to_primitive(vm, b, 2); if (vm->ctl) return 0;
        a = js_to_primitive(vm, a, 2); if (vm->ctl) return 0;
    }
    if (a.t == JT_STR && b.t == JT_STR) return js_str_cmp(vm, a.a, b.a) < 0;
    x = js_to_number(vm, a);
    y = js_to_number(vm, b);
    if (jf_isnan(x) || jf_isnan(y)) return -1;
    return jf_lt(x, y);
}

static jv binary(js_vm_t *vm, int op, jv a, jv b) {
    int r;
    switch (op) {
    case OP_SEQ: return jv_bool(js_strict_eq(vm, a, b));
    case OP_SNE: return jv_bool(!js_strict_eq(vm, a, b));
    case OP_EQ: r = js_loose_eq(vm, a, b); return vm->ctl ? jv_undef() : jv_bool(r);
    case OP_NE: r = js_loose_eq(vm, a, b); return vm->ctl ? jv_undef() : jv_bool(!r);
    case OP_LT: r = less_than(vm, a, b, 1); return jv_bool(r == 1);
    case OP_GT: r = less_than(vm, b, a, 0); return jv_bool(r == 1);
    case OP_LE: r = less_than(vm, b, a, 0); return jv_bool(r == 0);
    case OP_GE: r = less_than(vm, a, b, 1); return jv_bool(r == 0);
    case OP_INSTANCEOF: r = js_instanceof(vm, a, b); return vm->ctl ? jv_undef() : jv_bool(r);
    case OP_IN: {
        uint32_t k;
        if (b.t != JT_OBJ) return js_throw_err(vm, JE_TYPE, "Cannot use 'in' operator to search for a key in a non-object");
        k = js_to_key(vm, a);
        if (vm->ctl) return jv_undef();
        return jv_bool(js_has(vm, b.a, k));
    }
    default: return arith(vm, op, a, b);
    }
}

static int compound_op(int op) {
    switch (op) {
    case OP_ADD_A: return OP_ADD; case OP_SUB_A: return OP_SUB; case OP_MUL_A: return OP_MUL;
    case OP_DIV_A: return OP_DIV; case OP_MOD_A: return OP_MOD; case OP_EXP_A: return OP_EXP;
    case OP_SHL_A: return OP_SHL; case OP_SAR_A: return OP_SAR; case OP_SHR_A: return OP_SHR;
    case OP_BAND_A: return OP_BAND; case OP_BOR_A: return OP_BOR; case OP_BXOR_A: return OP_BXOR;
    default: return OP_NONE;
    }
}

/* ---- calls and arguments ---- */

typedef struct { jv *argv; int argc; jv local[8]; } args_t;

static void eval_args(js_vm_t *vm, uint32_t env, uint32_t list, args_t *a) {
    uint32_t e;
    int n = 0, spread = 0;
    for (e = list; e; e = NODE(e)->next) { n++; if (NODE(e)->k == N_SPREAD) spread = 1; }
    a->argv = a->local;
    a->argc = 0;
    if (!spread && n <= 8) {
        for (e = list; e && !vm->ctl; e = NODE(e)->next) a->local[a->argc++] = ev(vm, env, e);
        return;
    }
    {
        uint32_t arr = js_array_new(vm, (uint32_t)n);
        if (!arr) return;
        for (e = list; e && !vm->ctl; e = NODE(e)->next) {
            if (NODE(e)->k == N_SPREAD) {
                uint32_t src = 0, i;
                jv v = ev(vm, env, NODE(e)->a);
                if (vm->ctl) return;
                js_iter_values(vm, v, &src);
                if (vm->ctl) return;
                for (i = 0; i < JOBJ(vm, src)->alen; i++) js_array_push(vm, arr, js_array_get(vm, src, i));
            } else {
                jv v = ev(vm, env, e);
                if (vm->ctl) return;
                js_array_push(vm, arr, v);
            }
        }
        a->argv = (jv *)JPTR(vm, JOBJ(vm, arr)->arr);
        a->argc = (int)JOBJ(vm, arr)->alen;
    }
}

static uint32_t callee_name(js_vm_t *vm, uint32_t c) {
    js_node_t *n = NODE(c);
    if (n->k == N_IDENT) return n->a;
    if (n->k == N_MEMBER) return n->b;
    return js_intern(vm, "expression", 10);
}

static jv super_base(js_vm_t *vm, uint32_t env, jv *thisv) {
    uint32_t ts = this_scope(vm, env), f, home;
    js_env_t *e = JENV(vm, ts);
    *thisv = e->thisv;
    f = e->fobj;
    if (!f) return js_throw_err(vm, JE_SYNTAX, "'super' keyword unexpected here");
    home = FN_HOME(JOBJ(vm, f));
    if (!home) return js_throw_err(vm, JE_SYNTAX, "'super' keyword unexpected here");
    return JOBJ(vm, home)->proto ? jv_obj(JOBJ(vm, home)->proto) : jv_null();
}

static jv eval_call(js_vm_t *vm, uint32_t env, uint32_t n) {
    js_node_t *c = NODE(n);
    uint32_t callee = c->a;
    js_node_t *cn = NODE(callee);
    jv fn, thisv = jv_undef(), r;
    args_t a;
    if (cn->k == N_SUPER) {
        uint32_t ts = this_scope(vm, env), f, parent;
        js_env_t *e = JENV(vm, ts);
        f = e->fobj;
        if (!f || !(FN_FLAGS(JOBJ(vm, f)) & FN_DERIVED)) return js_throw_err(vm, JE_SYNTAX, "'super' keyword unexpected here");
        thisv = e->thisv;
        parent = JOBJ(vm, f)->proto;
        eval_args(vm, env, c->b, &a);
        if (vm->ctl) return jv_undef();
        if (JOBJ(vm, parent)->cls == JS_CLASS_FUNC) {
            jv pr = call_func(vm, parent, thisv, a.argv, a.argc, JENV(vm, ts)->newtarget);
            /* Lot 115: a parent constructor that returns an object makes it this */
            if (!vm->ctl && pr.t == JT_OBJ && pr.a != thisv.a) { thisv = pr; JENV(vm, ts)->thisv = pr; }
        } else if (js_is_callable(vm, jv_obj(parent))) {
            uint32_t sv = vm->super_this;
            int sn = vm->in_new;
            vm->super_this = thisv.a;
            vm->in_new = 1;
            (void)js_call(vm, jv_obj(parent), thisv, a.argv, a.argc);
            vm->in_new = sn;
            vm->super_this = sv;
        }
        if (vm->ctl) return jv_undef();
        init_fields(vm, f, thisv);
        return jv_undef();
    }
    if (cn->k == N_MEMBER || cn->k == N_INDEX) {
        jv key = jv_undef();
        if (NODE(cn->a)->k == N_SUPER) {
            jv base = super_base(vm, env, &thisv);
            if (vm->ctl) return jv_undef();
            if (jv_is_nullish(base)) fn = js_get(vm, base, cn->k == N_MEMBER ? NODE(callee)->b : vm->s_empty);
            else if (cn->k == N_MEMBER) fn = js_get_recv(vm, base, NODE(callee)->b, thisv);
            else {
                uint32_t k;
                key = ev(vm, env, NODE(callee)->b);
                if (vm->ctl) return jv_undef();
                k = js_to_key(vm, key);
                if (vm->ctl) return jv_undef();
                fn = js_get_recv(vm, base, k, thisv);
            }
        } else {
            thisv = ev(vm, env, cn->a);
            if (vm->ctl) return jv_undef();
            if (vm->opt_short) return jv_undef();
            if ((NODE(callee)->fl & F_OPT) && jv_is_nullish(thisv)) { vm->opt_short = 1; return jv_undef(); }
            if (NODE(callee)->k == N_MEMBER) fn = js_get(vm, thisv, NODE(callee)->b);
            else {
                key = ev(vm, env, NODE(callee)->b);
                if (vm->ctl) return jv_undef();
                fn = js_get_v(vm, thisv, key);
            }
        }
    } else {
        fn = ev(vm, env, callee);
    }
    if (vm->ctl) return jv_undef();
    if (vm->opt_short) return jv_undef();
    if ((NODE(n)->fl & F_OPT) && jv_is_nullish(fn)) { vm->opt_short = 1; return jv_undef(); }
    if (!js_is_callable(vm, fn)) {
        return js_throw_err2(vm, JE_TYPE, "", js_str_cat(vm, callee_name(vm, callee), js_str_c(vm, " is not a function")));
    }
    eval_args(vm, env, NODE(n)->b, &a);
    if (vm->ctl) return jv_undef();
    r = js_call(vm, fn, thisv, a.argv, a.argc);
    return r;
}

/* ---- literals ---- */

static jv eval_object(js_vm_t *vm, uint32_t env, uint32_t n) {
    uint32_t o = js_obj_new(vm, JS_CLASS_OBJECT, vm->p_object), e;
    if (!o) return jv_undef();
    for (e = NODE(n)->a; e && !vm->ctl; e = NODE(e)->next) {
        js_node_t *p = NODE(e);
        uint32_t key;
        jv v;
        if (p->k == N_SPREAD) {
            jv src = ev(vm, env, p->a);
            uint32_t keys, i;
            if (vm->ctl) return jv_undef();
            if (src.t == JT_STR) {      /* Lot 115: one key per UTF-16 unit */
                uint32_t len = js_str_units(vm, src.a);
                for (i = 0; i < len; i++) js_define(vm, o, js_key_of_index(vm, i), jv_str(js_u16_sub(vm, src.a, i, i + 1)), P_DEFAULT);
                continue;
            }
            if (src.t != JT_OBJ) continue;
            keys = js_own_keys(vm, src.a, 1);
            for (i = 0; keys && i < JOBJ(vm, keys)->alen && !vm->ctl; i++) {
                jv k = js_array_get(vm, keys, i);
                jv val = js_get(vm, src, k.a);
                if (vm->ctl) return jv_undef();
                js_define(vm, o, k.a, val, P_DEFAULT);
            }
            keys = js_own_symbols(vm, src.a, 1);     /* Lot 115: {...o} copies symbol keys */
            for (i = 0; keys && i < JOBJ(vm, keys)->alen && !vm->ctl; i++) {
                jv k = js_array_get(vm, keys, i);
                jv val = js_get(vm, src, k.a);
                if (vm->ctl) return jv_undef();
                js_define(vm, o, k.a, val, P_DEFAULT);
            }
            continue;
        }
        if (p->fl & F_COMPUTED) {
            jv kv = ev(vm, env, p->a);
            if (vm->ctl) return jv_undef();
            key = js_to_key(vm, kv);
            if (vm->ctl) return jv_undef();
        } else key = p->a;
        p = NODE(e);
        if (p->fl & (F_GETTER | F_SETTER)) {
            uint32_t f = js_closure(vm, p->b, env);
            if (!f) return jv_undef();
            FN_HOME(JOBJ(vm, f)) = o;
            JOBJ(vm, f)->x2 = key;
            js_define_accessor(vm, o, key, (p->fl & F_GETTER) ? f : 0, (p->fl & F_SETTER) ? f : 0, P_ENUM | P_CONF);
            continue;
        }
        if (p->fl & F_METHOD) {
            uint32_t f = js_closure(vm, p->b, env);
            if (!f) return jv_undef();
            FN_HOME(JOBJ(vm, f)) = o;
            JOBJ(vm, f)->x2 = key;
            v = jv_obj(f);
        } else {
            v = ev(vm, env, p->b);
            if (vm->ctl) return jv_undef();
            set_fn_name(vm, v, key);
        }
        if (key == vm->s_prototype && 0) {}
        js_define(vm, o, key, v, P_DEFAULT);
    }
    return jv_obj(o);
}

static jv eval_array(js_vm_t *vm, uint32_t env, uint32_t n) {
    uint32_t arr = js_array_new(vm, 4), e;
    if (!arr) return jv_undef();
    for (e = NODE(n)->a; e && !vm->ctl; e = NODE(e)->next) {
        js_node_t *x = NODE(e);
        if (x->k == N_ELISION) { js_array_set_len(vm, arr, JOBJ(vm, arr)->alen + 1); continue; }
        if (x->k == N_SPREAD) {
            uint32_t src = 0, i;
            jv v = ev(vm, env, x->a);
            if (vm->ctl) return jv_undef();
            js_iter_values(vm, v, &src);
            if (vm->ctl) return jv_undef();
            for (i = 0; i < JOBJ(vm, src)->alen; i++) js_array_push(vm, arr, js_array_get(vm, src, i));
            continue;
        }
        {
            jv v = ev(vm, env, e);
            if (vm->ctl) return jv_undef();
            js_array_push(vm, arr, v);
        }
    }
    return jv_obj(arr);
}

static jv eval_template(js_vm_t *vm, uint32_t env, uint32_t n) {
    uint32_t acc = vm->s_empty, e;
    for (e = NODE(n)->a; e && !vm->ctl; e = NODE(e)->next) {
        uint32_t s;
        if (NODE(e)->k == N_STR) s = NODE(e)->a;
        else {
            jv v = ev(vm, env, e);
            if (vm->ctl) return jv_undef();
            s = js_to_string(vm, v);
            if (vm->ctl) return jv_undef();
        }
        acc = js_str_cat(vm, acc, s);
    }
    return jv_str(acc);
}

static jv eval_tagged(js_vm_t *vm, uint32_t env, uint32_t n) {
    uint32_t strs = js_array_new(vm, 4), raw = js_array_new(vm, 4), e, tn = NODE(n)->b;
    jv fn, thisv = jv_undef(), local[16];
    int argc = 1, i = 0;
    js_node_t *cn = NODE(NODE(n)->a);
    if (!strs || !raw) return jv_undef();
    if (cn->k == N_MEMBER) {
        thisv = ev(vm, env, cn->a);
        if (vm->ctl) return jv_undef();
        fn = js_get(vm, thisv, NODE(NODE(n)->a)->b);
    } else fn = ev(vm, env, NODE(n)->a);
    if (vm->ctl) return jv_undef();
    /* Lot 115: the parts alternate text, expression, text...; an expression
     * that is itself a string literal is a value, not a text part */
    for (e = NODE(tn)->a; e && !vm->ctl; e = NODE(e)->next, i++) {
        if (!(i & 1)) {
            js_array_push(vm, strs, jv_str(NODE(e)->a));
            js_array_push(vm, raw, jv_str(NODE(e)->b ? NODE(e)->b : NODE(e)->a));
        } else if (argc < 16) {
            local[argc++] = ev(vm, env, e);
        }
    }
    if (vm->ctl) return jv_undef();
    js_define(vm, strs, vm->s_raw, jv_obj(raw), P_HIDDEN);
    local[0] = jv_obj(strs);
    if (!js_is_callable(vm, fn)) return js_throw_err(vm, JE_TYPE, "tag is not a function");
    return js_call(vm, fn, thisv, local, argc);
}

/* ---- expressions ---- */

static jv eval_update(js_vm_t *vm, uint32_t env, uint32_t n) {
    js_node_t *u = NODE(n);
    uint32_t t = u->a;
    jv obj = jv_undef(), key = jv_undef(), old, nv;
    int k = NODE(t)->k, prefix = (u->fl & F_PREFIX) != 0, op = u->op;
    if (k == N_IDENT) old = lookup(vm, env, NODE(t)->a, 0);
    else {
        obj = ev(vm, env, NODE(t)->a);
        if (vm->ctl) return jv_undef();
        if (k == N_MEMBER) { key = jv_str(NODE(t)->b); old = js_get(vm, obj, NODE(t)->b); }
        else {
            key = ev(vm, env, NODE(t)->b);
            if (vm->ctl) return jv_undef();
            old = js_get_v(vm, obj, key);
        }
    }
    if (vm->ctl) return jv_undef();
    old = js_to_numeric(vm, old);
    if (vm->ctl) return jv_undef();
    nv = arith(vm, op == OP_INC ? OP_ADD : OP_SUB, old, jv_int(1));
    if (k == N_IDENT) assign_var(vm, env, NODE(t)->a, nv);
    else if (k == N_MEMBER) js_put(vm, obj, key.a, nv);
    else js_put_v(vm, obj, key, nv);
    return prefix ? nv : old;
}

static jv eval_assign(js_vm_t *vm, uint32_t env, uint32_t n) {
    js_node_t *as = NODE(n);
    uint32_t t = as->a;
    int op = as->op, k = NODE(t)->k;
    jv obj = jv_undef(), key = jv_undef(), v, old;
    if (op == OP_ASSIGN) {
        if (k == N_IDENT) {
            v = ev(vm, env, as->b);
            if (vm->ctl) return jv_undef();
            set_fn_name(vm, v, NODE(t)->a);
            assign_var(vm, env, NODE(t)->a, v);
            return v;
        }
        if (k == N_MEMBER || k == N_INDEX) {
            obj = ev(vm, env, NODE(t)->a);
            if (vm->ctl) return jv_undef();
            if (k == N_INDEX) { key = ev(vm, env, NODE(t)->b); if (vm->ctl) return jv_undef(); }
            v = ev(vm, env, NODE(n)->b);
            if (vm->ctl) return jv_undef();
            if (k == N_MEMBER) js_put(vm, obj, NODE(t)->b, v);
            else js_put_v(vm, obj, key, v);
            return v;
        }
        v = ev(vm, env, as->b);
        if (vm->ctl) return jv_undef();
        bind_pattern(vm, env, t, v, BIND_ASSIGN);
        return v;
    }
    /* compound and logical assignment: the reference is evaluated once */
    if (k == N_IDENT) old = lookup(vm, env, NODE(t)->a, 0);
    else {
        obj = ev(vm, env, NODE(t)->a);
        if (vm->ctl) return jv_undef();
        if (k == N_MEMBER) old = js_get(vm, obj, NODE(t)->b);
        else {
            key = ev(vm, env, NODE(t)->b);
            if (vm->ctl) return jv_undef();
            old = js_get_v(vm, obj, key);
        }
    }
    if (vm->ctl) return jv_undef();
    if (op == OP_AND_A || op == OP_OR_A || op == OP_NULLISH_A) {
        int keep = op == OP_AND_A ? !js_truthy(vm, old) : op == OP_OR_A ? js_truthy(vm, old) : !jv_is_nullish(old);
        if (keep) return old;
        v = ev(vm, env, NODE(n)->b);
        if (vm->ctl) return jv_undef();
        if (k == N_IDENT) set_fn_name(vm, v, NODE(t)->a);
    } else {
        jv rhs = ev(vm, env, NODE(n)->b);
        if (vm->ctl) return jv_undef();
        v = binary(vm, compound_op(op), old, rhs);
        if (vm->ctl) return jv_undef();
    }
    if (k == N_IDENT) assign_var(vm, env, NODE(t)->a, v);
    else if (k == N_MEMBER) js_put(vm, obj, NODE(t)->b, v);
    else js_put_v(vm, obj, key, v);
    return v;
}

static jv eval_unary(js_vm_t *vm, uint32_t env, uint32_t n) {
    js_node_t *u = NODE(n);
    int op = u->op;
    jv v;
    if (op == OP_TYPEOF && NODE(u->a)->k == N_IDENT) {
        v = lookup(vm, env, NODE(u->a)->a, 1);
        if (vm->ctl) return jv_undef();
        return jv_str(js_typeof(vm, v));
    }
    if (op == OP_DELETE) {
        uint32_t t = u->a;
        int k = NODE(t)->k;
        if (k == N_MEMBER || k == N_INDEX) {
            jv obj = ev(vm, env, NODE(t)->a), key;
            if (vm->ctl) return jv_undef();
            if (k == N_MEMBER) key = jv_str(NODE(t)->b);
            else { key = ev(vm, env, NODE(t)->b); if (vm->ctl) return jv_undef(); }
            return jv_bool(js_delete(vm, obj, key));
        }
        if (k == N_IDENT) return jv_bool(0);
        (void)ev(vm, env, t);
        return jv_bool(1);
    }
    v = ev(vm, env, u->a);
    if (vm->ctl) return jv_undef();
    switch (op) {
    case OP_NOT: return jv_bool(!js_truthy(vm, v));
    case OP_TYPEOF: return jv_str(js_typeof(vm, v));
    case OP_VOID: return jv_undef();
    case OP_BNOT: { int32_t i = js_to_int32(vm, v); return jv_int(~i); }
    case OP_PLUS: return jv_num(js_to_number(vm, v));
    case OP_NEG:
        if (v.t == JT_INT && (int32_t)v.a != 0 && (int32_t)v.a != -2147483647 - 1) return jv_int(-(int32_t)v.a);
        return jv_num(jf_neg(js_to_number(vm, v)));
    default: return jv_undef();
    }
}

static jv ev(js_vm_t *vm, uint32_t env, uint32_t n) {
    js_node_t *x;
    if (!n || vm->ctl) return jv_undef();
    js_step(vm);
    if (vm->ctl) return jv_undef();
    if (stack_low(vm)) { js_abort(vm, "RangeError: expression too deep for the guest stack"); return jv_undef(); }
    x = NODE(n);
    switch (x->k) {
    case N_NUM: return jv_num(((jf64)x->b << 32) | x->a);
    case N_STR: return jv_str(x->a);
    case N_TRUE: return jv_bool(1);
    case N_FALSE: return jv_bool(0);
    case N_NULL: return jv_null();
    case N_IDENT:
        if (x->a == vm->s_undefined) {
            uint32_t e;
            for (e = env; e; e = JENV(vm, e)->parent) if (env_own(vm, e, x->a)) return lookup(vm, env, x->a, 0);
            return jv_undef();
        }
        return lookup(vm, env, x->a, 0);
    case N_THIS: {
        uint32_t ts = this_scope(vm, env);
        return JENV(vm, ts)->thisv;
    }
    case N_NEWTARGET: {
        uint32_t ts = this_scope(vm, env);
        return JENV(vm, ts)->newtarget ? jv_obj(JENV(vm, ts)->newtarget) : jv_undef();
    }
    case N_TMPL: return eval_template(vm, env, n);
    case N_TTAG: return eval_tagged(vm, env, n);
    case N_ARRAY: return eval_array(vm, env, n);
    case N_OBJECT: return eval_object(vm, env, n);
    case N_FUNC: {
        uint32_t f = js_closure(vm, n, env);
        return f ? jv_obj(f) : jv_undef();
    }
    case N_CLASS: return eval_class(vm, env, n);
    case N_REGEX: {
        uint32_t r = js_regexp_lit(vm, x->a, x->b, &x->c);
        return r ? jv_obj(r) : jv_undef();
    }
    case N_UNARY: return eval_unary(vm, env, n);
    case N_UPDATE: return eval_update(vm, env, n);
    case N_BINARY: {
        jv a = ev(vm, env, x->a), b;
        if (vm->ctl) return jv_undef();
        b = ev(vm, env, NODE(n)->b);
        if (vm->ctl) return jv_undef();
        return binary(vm, NODE(n)->op, a, b);
    }
    case N_LOGICAL: {
        jv a = ev(vm, env, x->a);
        int op = NODE(n)->op;
        if (vm->ctl) return jv_undef();
        if (op == OP_AND) { if (!js_truthy(vm, a)) return a; }
        else if (op == OP_OR) { if (js_truthy(vm, a)) return a; }
        else if (!jv_is_nullish(a)) return a;
        return ev(vm, env, NODE(n)->b);
    }
    case N_ASSIGN: return eval_assign(vm, env, n);
    case N_COND: {
        jv c = ev(vm, env, x->a);
        if (vm->ctl) return jv_undef();
        return ev(vm, env, js_truthy(vm, c) ? NODE(n)->b : NODE(n)->c);
    }
    case N_SEQ: {
        (void)ev(vm, env, x->a);
        if (vm->ctl) return jv_undef();
        return ev(vm, env, NODE(n)->b);
    }
    case N_MEMBER: case N_INDEX: {
        jv o, r;
        int chain = (x->fl & F_CHAIN) != 0;
        if (NODE(x->a)->k == N_SUPER) {
            jv thisv, base = super_base(vm, env, &thisv);
            if (vm->ctl) return jv_undef();
            if (jv_is_nullish(base)) return js_get(vm, base, x->k == N_MEMBER ? x->b : vm->s_empty);
            if (x->k == N_MEMBER) return js_get_recv(vm, base, x->b, thisv);
            {
                jv key = ev(vm, env, NODE(n)->b);
                uint32_t k;
                if (vm->ctl) return jv_undef();
                k = js_to_key(vm, key);
                if (vm->ctl) return jv_undef();
                return js_get_recv(vm, base, k, thisv);
            }
        }
        o = ev(vm, env, x->a);
        if (vm->ctl) return jv_undef();
        if (vm->opt_short) { if (chain) vm->opt_short = 0; return jv_undef(); }
        if ((NODE(n)->fl & F_OPT) && jv_is_nullish(o)) { if (!chain) vm->opt_short = 1; return jv_undef(); }
        if (NODE(n)->k == N_MEMBER) r = js_get(vm, o, NODE(n)->b);
        else {
            jv key = ev(vm, env, NODE(n)->b);
            if (vm->ctl) return jv_undef();
            r = js_get_v(vm, o, key);
        }
        return r;
    }
    case N_CALL: {
        int chain = (x->fl & F_CHAIN) != 0;
        jv r = eval_call(vm, env, n);
        if (chain && vm->opt_short) vm->opt_short = 0;
        return r;
    }
    case N_NEW: {
        jv c = ev(vm, env, x->a);
        args_t a;
        if (vm->ctl) return jv_undef();
        if (!js_is_callable(vm, c))
            return js_throw_err2(vm, JE_TYPE, "", js_str_cat(vm, callee_name(vm, NODE(n)->a), js_str_c(vm, " is not a constructor")));
        eval_args(vm, env, NODE(n)->b, &a);
        if (vm->ctl) return jv_undef();
        return js_construct(vm, c, a.argv, a.argc, c);
    }
    case N_YIELD: {       /* Lot 114 */
        jv v = x->a ? ev(vm, env, x->a) : jv_undef();
        if (vm->ctl) return jv_undef();
        return js_co_yield(vm, v, NODE(n)->op == 1);
    }
    case N_AWAIT: {
        jv v = ev(vm, env, x->a);
        if (vm->ctl) return jv_undef();
        return js_co_await(vm, v);
    }
    case N_PRIVNAME:          /* Lot 115: the symbol the class body bound to #x */
        return lookup(vm, env, x->a, 0);
    case N_SPREAD:
        return js_throw_err(vm, JE_SYNTAX, "Unexpected spread");
    default:
        return js_throw_err(vm, JE_SYNTAX, "unsupported expression");
    }
}

/* ---- classes ---- */

static jv eval_class(js_vm_t *vm, uint32_t env, uint32_t n) {
    js_node_t *c = NODE(n);
    uint32_t proto_parent = vm->p_object, ctor_parent = vm->p_function, proto, f, cenv, ctor, m;
    jv parent = jv_undef();
    if (c->b) {
        parent = ev(vm, env, c->b);
        if (vm->ctl) return jv_undef();
        if (parent.t == JT_NULL) { proto_parent = 0; }
        else {
            jv pp;
            if (!js_is_callable(vm, parent)) return js_throw_err(vm, JE_TYPE, "Class extends value is not a constructor or null");
            pp = js_get(vm, parent, vm->s_prototype);
            if (vm->ctl) return jv_undef();
            if (pp.t != JT_OBJ && pp.t != JT_NULL) return js_throw_err(vm, JE_TYPE, "Class extends value does not have a valid prototype");
            proto_parent = pp.t == JT_OBJ ? pp.a : 0;
            ctor_parent = parent.a;
        }
    }
    cenv = js_env_new(vm, env, E_BLOCK);
    if (!cenv) return jv_undef();
    proto = js_obj_new(vm, JS_CLASS_OBJECT, proto_parent);
    if (!proto) return jv_undef();
    ctor = NODE(n)->d;
    if (!ctor) {
        ctor = js_node_new(vm, N_FUNC, NODE(n)->line);
        if (!ctor) return jv_undef();
        NODE(ctor)->fl = F_METHOD | F_CTOR | (NODE(n)->b ? F_DERIVED : 0);
    }
    f = js_closure(vm, ctor, cenv);
    if (!f) return jv_undef();
    JOBJ(vm, f)->proto = ctor_parent;
    JOBJ(vm, f)->x2 = NODE(n)->a;
    FN_HOME(JOBJ(vm, f)) = proto;
    FN_CLASS(JOBJ(vm, f)) = n;
    FN_FLAGS(JOBJ(vm, f)) = FN_CLASSCTOR | (NODE(n)->b ? FN_DERIVED : 0) | (NODE(n)->d ? 0 : FN_DEFAULT);
    js_define(vm, f, vm->s_prototype, jv_obj(proto), 0);
    js_define(vm, proto, vm->s_constructor, jv_obj(f), P_HIDDEN);
    if (NODE(n)->a) js_env_declare(vm, cenv, NODE(n)->a, jv_obj(f), V_CONST);
    /* Lot 115: every private name of this evaluation of the class gets a
     * new symbol in the class scope; instance private methods go on a
     * holder object init_fields copies to each instance */
    for (m = NODE(n)->c; m && !vm->ctl; m = NODE(m)->next) {
        js_node_t *mn = NODE(m);
        if (mn->op != MK_PRIVATE || (mn->k != N_FIELD && mn->k != N_METHOD)) continue;
        if (!env_own(vm, cenv, mn->a)) {
            uint32_t sym = js_sym_new(vm, NODE(m)->a, JS_SYM_PRIVATE);
            if (!sym) return jv_undef();
            js_env_declare(vm, cenv, NODE(m)->a, jv_sym(sym), V_CONST);
        }
        if (NODE(m)->k == N_METHOD && !(NODE(m)->fl & F_STATIC) && !env_own(vm, cenv, vm->s_privm)) {
            uint32_t h = js_obj_new(vm, JS_CLASS_OBJECT, 0);
            if (!h) return jv_undef();
            js_env_declare(vm, cenv, vm->s_privm, jv_obj(h), V_CONST);
        }
    }
    if (vm->ctl) return jv_undef();
    for (m = NODE(n)->c; m && !vm->ctl; m = NODE(m)->next) {
        js_node_t *mn = NODE(m);
        uint32_t key, target = (mn->fl & F_STATIC) ? f : proto;
        int priv = mn->op == MK_PRIVATE;
        if (mn->k == N_FIELD && mn->op == MK_SBLOCK) {      /* static { }: runs now with this = the class */
            uint32_t fb = js_closure(vm, mn->b, cenv);
            if (!fb) return jv_undef();
            FN_HOME(JOBJ(vm, fb)) = f;
            (void)call_func(vm, fb, jv_obj(f), 0, 0, 0);
            if (vm->ctl) return jv_undef();
            continue;
        }
        if (priv) {
            js_prop_t *pv = env_own(vm, cenv, mn->a);
            key = pv ? pv->v.a : 0;
            if (!key) return js_throw_err(vm, JE_SYNTAX, "private name without a binding");
            if (NODE(m)->k == N_METHOD && !(NODE(m)->fl & F_STATIC)) {
                js_prop_t *hv = env_own(vm, cenv, vm->s_privm);
                target = hv ? hv->v.a : proto;
            }
        } else if (mn->fl & F_COMPUTED) {
            jv kv = ev(vm, cenv, mn->a);
            if (vm->ctl) return jv_undef();
            key = js_to_key(vm, kv);
            if (vm->ctl) return jv_undef();
            NODE(m)->d = key;
        } else key = mn->a;
        mn = NODE(m);
        if (mn->k == N_METHOD) {
            uint32_t fm = js_closure(vm, mn->b, cenv);
            if (!fm) return jv_undef();
            FN_HOME(JOBJ(vm, fm)) = (priv && !(NODE(m)->fl & F_STATIC)) ? proto : target;
            JOBJ(vm, fm)->x2 = priv ? NODE(m)->a : key;
            if (NODE(m)->fl & F_GETTER) js_define_accessor(vm, target, key, fm, 0, priv ? 0 : P_CONF);
            else if (NODE(m)->fl & F_SETTER) js_define_accessor(vm, target, key, 0, fm, priv ? 0 : P_CONF);
            else js_define(vm, target, key, jv_obj(fm), priv ? 0 : P_HIDDEN);
        } else if (mn->fl & F_STATIC) {
            uint32_t fe = js_env_new(vm, cenv, E_FUNC);
            jv v = jv_undef();
            if (!fe) return jv_undef();
            JENV(vm, fe)->thisv = jv_obj(f);
            JENV(vm, fe)->fobj = f;
            if (NODE(m)->b) { v = ev(vm, fe, NODE(m)->b); set_fn_name(vm, v, priv ? NODE(m)->a : key); }
            env_free(vm, fe);
            if (vm->ctl) return jv_undef();
            if (priv && js_own_prop(vm, f, key))
                return js_throw_err2(vm, JE_TYPE, "Cannot initialize a private field twice on the same object: ", NODE(m)->a);
            js_define(vm, f, key, v, priv ? P_WRITE : P_DEFAULT);
        }
    }
    if (vm->ctl) return jv_undef();
    return jv_obj(f);
}

/* ---- statements ---- */

static int loop_ctl(js_vm_t *vm, uint32_t label) {
    /* after a loop body: 1 to leave the loop, 0 to go on */
    if (vm->ctl == CTL_BREAK) {
        if (!vm->label || vm->label == label) { vm->ctl = CTL_NONE; vm->label = 0; }
        return 1;
    }
    if (vm->ctl == CTL_CONTINUE) {
        if (!vm->label || vm->label == label) { vm->ctl = CTL_NONE; vm->label = 0; return 0; }
        return 1;
    }
    return vm->ctl != CTL_NONE;
}

static uint32_t env_copy(js_vm_t *vm, uint32_t src) {
    uint32_t e = js_env_new(vm, JENV(vm, src)->parent, E_BLOCK), i;
    if (!e) return 0;
    for (i = 0; i < JENV(vm, src)->n; i++) {
        js_prop_t *v = (js_prop_t *)JPTR(vm, JENV(vm, src)->vars) + i;
        if (!js_env_declare(vm, e, v->key, v->v, v->flags)) return 0;
    }
    return e;
}

static void ex_for(js_vm_t *vm, uint32_t env, uint32_t n, uint32_t label) {
    js_node_t *f = NODE(n);
    uint32_t init = f->a, loop_env = env;
    int per_iter = init && NODE(init)->k == N_VAR && NODE(init)->op != DK_VAR;
    if (per_iter) {
        loop_env = js_env_new(vm, env, E_BLOCK);
        if (!loop_env) return;
    }
    if (init) {
        if (NODE(init)->k == N_VAR) ex(vm, loop_env, init);
        else (void)ev(vm, loop_env, init);
    }
    if (per_iter && !vm->ctl) {
        uint32_t c = env_copy(vm, loop_env);
        env_free(vm, loop_env);
        loop_env = c;
    }
    while (!vm->ctl) {
        if (NODE(n)->b) {
            jv t = ev(vm, loop_env, NODE(n)->b);
            if (vm->ctl || !js_truthy(vm, t)) break;
        }
        ex(vm, loop_env, NODE(n)->d);
        if (loop_ctl(vm, label)) break;
        if (per_iter) {
            uint32_t c = env_copy(vm, loop_env);
            if (!c) break;
            env_free(vm, loop_env);
            loop_env = c;
        }
        if (NODE(n)->c) (void)ev(vm, loop_env, NODE(n)->c);
    }
    if (per_iter) env_free(vm, loop_env);
}

/* the target of for-in/of: a declaration (fresh scope per iteration) or an lvalue */
static void forx_bind(js_vm_t *vm, uint32_t env, uint32_t target, jv v, uint32_t *iter_env) {
    js_node_t *t = NODE(target);
    *iter_env = env;
    if (t->k == N_VAR) {
        uint32_t d = t->a;
        int kind = t->op;
        if (kind != DK_VAR) {
            *iter_env = js_env_new(vm, env, E_BLOCK);
            if (!*iter_env) return;
        }
        bind_pattern(vm, *iter_env, NODE(d)->a, v, kind);
        return;
    }
    if (t->k == N_OPAT || t->k == N_APAT) bind_pattern(vm, env, target, v, BIND_ASSIGN);
    else assign_target(vm, env, target, v, BIND_ASSIGN);
}

static void ex_forin(js_vm_t *vm, uint32_t env, uint32_t n, uint32_t label) {
    jv o = ev(vm, env, NODE(n)->b);
    uint32_t keys, seen, proto, i;
    if (vm->ctl || jv_is_nullish(o)) return;
    if (o.t != JT_OBJ) {
        uint32_t w = js_to_object(vm, o);
        if (!w) return;
        o = jv_obj(w);
    }
    keys = js_array_new(vm, 8);
    seen = js_obj_new(vm, JS_CLASS_OBJECT, 0);
    if (!keys || !seen) return;
    /* own keys, then the prototype chain's enumerable keys not shadowed */
    for (proto = o.a; proto && !vm->ctl; proto = JOBJ(vm, proto)->proto) {
        uint32_t ks = js_own_keys(vm, proto, 0);
        for (i = 0; ks && i < JOBJ(vm, ks)->alen; i++) {
            jv k = js_array_get(vm, ks, i);
            js_prop_t *p;
            if (js_own_prop(vm, seen, k.a)) continue;
            js_define(vm, seen, k.a, jv_bool(1), P_DEFAULT);
            p = js_own_prop(vm, proto, k.a);
            if (p && !(p->flags & P_ENUM)) continue;
            if (!p && (k.a == vm->s_length || k.a == vm->s_name)) continue;   /* built-in, not enumerable */
            js_array_push(vm, keys, k);
        }
    }
    for (i = 0; i < JOBJ(vm, keys)->alen && !vm->ctl; i++) {
        jv k = js_array_get(vm, keys, i);
        uint32_t ie;
        if (!js_has(vm, o.a, k.a)) continue;   /* deleted during the loop */
        forx_bind(vm, env, NODE(n)->a, k, &ie);
        if (vm->ctl) break;
        ex(vm, ie, NODE(n)->c);
        if (ie != env) env_free(vm, ie);
        if (loop_ctl(vm, label)) break;
    }
}

/* Lot 115: for await (x of src) inside an async function or generator:
 * an async iterator (Symbol.asyncIterator) is stepped with an await on
 * each next() result; any other iterable is read as for-of reads it and
 * each value is awaited. An early exit awaits the iterator's return(). */
static void ex_forawait(js_vm_t *vm, uint32_t env, uint32_t n, uint32_t label, jv src) {
    jv m = jv_undef(), it, nx;
    uint32_t arr = 0, i;
    if (src.t == JT_OBJ || src.t == JT_STR) m = js_get(vm, src, vm->sym_async_iterator);
    if (vm->ctl) return;
    if (!jv_is_nullish(m)) {
        if (!js_is_callable(vm, m)) { js_throw_err(vm, JE_TYPE, "Symbol.asyncIterator is not a function"); return; }
        it = js_call(vm, m, src, 0, 0);
        if (vm->ctl) return;
        if (it.t != JT_OBJ) { js_throw_err(vm, JE_TYPE, "Result of the Symbol.asyncIterator method is not an object"); return; }
        nx = js_get(vm, it, vm->s_next);
        if (vm->ctl) return;
        for (;;) {
            uint32_t ie;
            jv r = js_call(vm, nx, it, 0, 0), v;
            if (vm->ctl) return;
            r = js_co_await(vm, r);
            if (vm->ctl) return;
            if (r.t != JT_OBJ) { js_throw_err(vm, JE_TYPE, "Iterator result is not an object"); return; }
            if (js_truthy(vm, js_get(vm, r, vm->s_done)) || vm->ctl) return;
            v = js_get(vm, r, vm->s_value);
            if (vm->ctl) return;
            forx_bind(vm, env, NODE(n)->a, v, &ie);
            if (!vm->ctl) ex(vm, ie, NODE(n)->c);
            if (ie != env) env_free(vm, ie);
            if (vm->ctl == CTL_CONTINUE && (!vm->label || vm->label == label)) { vm->ctl = CTL_NONE; vm->label = 0; continue; }
            if (vm->ctl) {
                int sctl = vm->ctl, brk = vm->ctl == CTL_BREAK && (!vm->label || vm->label == label);
                jv sret = vm->ret, sexc = vm->exc;
                uint32_t slabel = vm->label;
                if (sctl != CTL_ABORT && sctl != CTL_THROW) {
                    jv rf;
                    vm->ctl = CTL_NONE;
                    rf = js_get(vm, it, js_intern(vm, "return", 6));
                    if (!vm->ctl && js_is_callable(vm, rf)) {
                        jv rr = js_call(vm, rf, it, 0, 0);
                        if (!vm->ctl) (void)js_co_await(vm, rr);
                    }
                    if (vm->ctl) return;
                    vm->ctl = sctl; vm->ret = sret; vm->exc = sexc; vm->label = slabel;
                }
                if (brk && vm->ctl == CTL_BREAK) { vm->ctl = CTL_NONE; vm->label = 0; }
                return;
            }
        }
    }
    js_iter_values(vm, src, &arr);
    if (vm->ctl) return;
    for (i = 0; i < JOBJ(vm, arr)->alen && !vm->ctl; i++) {
        uint32_t ie;
        jv v = js_co_await(vm, js_array_get(vm, arr, i));
        if (vm->ctl) break;
        forx_bind(vm, env, NODE(n)->a, v, &ie);
        if (vm->ctl) break;
        ex(vm, ie, NODE(n)->c);
        if (ie != env) env_free(vm, ie);
        if (loop_ctl(vm, label)) break;
    }
}

static void ex_forof(js_vm_t *vm, uint32_t env, uint32_t n, uint32_t label) {
    jv src = ev(vm, env, NODE(n)->b);
    uint32_t arr = 0, i;
    if (vm->ctl) return;
    if (NODE(n)->op & 1u) { ex_forawait(vm, env, n, label, src); return; }
    if (js_is_gen(vm, src)) {
        /* Lot 114: a generator is read one value at a time and closed
         * (its finally blocks run) when the loop leaves early */
        for (;;) {
            uint32_t ie;
            int done;
            jv v = js_gen_step(vm, src.a, 0, jv_undef(), &done);
            if (vm->ctl || done) return;
            forx_bind(vm, env, NODE(n)->a, v, &ie);
            if (!vm->ctl) ex(vm, ie, NODE(n)->c);
            if (ie != env) env_free(vm, ie);
            if (vm->ctl == CTL_CONTINUE && (!vm->label || vm->label == label)) { vm->ctl = CTL_NONE; vm->label = 0; continue; }
            if (vm->ctl) {
                int brk = vm->ctl == CTL_BREAK && (!vm->label || vm->label == label);
                js_gen_close(vm, src.a);
                if (brk && vm->ctl == CTL_BREAK) { vm->ctl = CTL_NONE; vm->label = 0; }
                return;
            }
        }
    }
    if (src.t == JT_OBJ) {
        /* Lot 115: a user [Symbol.iterator] is read one value at a time;
         * an early exit calls the iterator's return() */
        jv it, nx;
        if (js_iter_custom(vm, src, &it, &nx)) {
            for (;;) {
                uint32_t ie;
                int done;
                jv v = js_iter_step(vm, it, nx, &done);
                if (vm->ctl || done) return;
                forx_bind(vm, env, NODE(n)->a, v, &ie);
                if (!vm->ctl) ex(vm, ie, NODE(n)->c);
                if (ie != env) env_free(vm, ie);
                if (vm->ctl == CTL_CONTINUE && (!vm->label || vm->label == label)) { vm->ctl = CTL_NONE; vm->label = 0; continue; }
                if (vm->ctl) {
                    int brk = vm->ctl == CTL_BREAK && (!vm->label || vm->label == label);
                    js_iter_close(vm, it);
                    if (brk && vm->ctl == CTL_BREAK) { vm->ctl = CTL_NONE; vm->label = 0; }
                    return;
                }
            }
        }
        if (vm->ctl) return;
    }
    js_iter_values(vm, src, &arr);
    if (vm->ctl) return;
    for (i = 0; i < JOBJ(vm, arr)->alen && !vm->ctl; i++) {
        uint32_t ie;
        /* arrays are read live (pushes during the loop are visited) */
        jv v = js_array_get(vm, arr, i);
        forx_bind(vm, env, NODE(n)->a, v, &ie);
        if (vm->ctl) break;
        ex(vm, ie, NODE(n)->c);
        if (ie != env) env_free(vm, ie);
        if (loop_ctl(vm, label)) break;
    }
}

static void ex_try(js_vm_t *vm, uint32_t env, uint32_t n) {
    js_node_t *t = NODE(n);
    ex(vm, env, t->a);
    if (vm->ctl == CTL_THROW && NODE(n)->c) {
        jv exc = vm->exc;
        uint32_t ce = js_env_new(vm, env, E_CATCH);
        vm->ctl = CTL_NONE;
        vm->exc = jv_undef();
        if (!ce) return;
        if (NODE(n)->b) bind_pattern(vm, ce, NODE(n)->b, exc, DK_LET);
        if (!vm->ctl) ex(vm, ce, NODE(n)->c);
        env_free(vm, ce);
    }
    if (NODE(n)->d && vm->ctl != CTL_ABORT) {
        int sctl = vm->ctl;
        jv sret = vm->ret, sexc = vm->exc;
        uint32_t slabel = vm->label;
        vm->ctl = CTL_NONE;
        ex(vm, env, NODE(n)->d);
        if (vm->ctl == CTL_NONE) { vm->ctl = sctl; vm->ret = sret; vm->exc = sexc; vm->label = slabel; }
    }
}

static void ex_switch(js_vm_t *vm, uint32_t env, uint32_t n, uint32_t label) {
    jv d = ev(vm, env, NODE(n)->a);
    uint32_t c, start = 0, se = env, deflt = 0;
    if (vm->ctl) return;
    if (NODE(n)->fl & F_LEX) { se = js_env_new(vm, env, E_BLOCK); if (!se) return; }
    for (c = NODE(n)->b; c && !vm->ctl; c = NODE(c)->next) {
        jv t;
        if (NODE(c)->fl & F_STATIC) { deflt = c; continue; }
        t = ev(vm, se, NODE(c)->a);
        if (vm->ctl) break;
        if (js_strict_eq(vm, d, t)) { start = c; break; }
    }
    if (!start) start = deflt;
    if (start && se != env) {
        for (c = NODE(n)->b; c; c = NODE(c)->next) hoist_funcs(vm, se, NODE(c)->b, 1);
    }
    for (c = start; c && !vm->ctl; c = NODE(c)->next) ex_list(vm, se, NODE(c)->b);
    if (vm->ctl == CTL_BREAK && (!vm->label || vm->label == label)) { vm->ctl = CTL_NONE; vm->label = 0; }
    if (se != env) env_free(vm, se);
}

static void ex_labeled(js_vm_t *vm, uint32_t env, uint32_t body, uint32_t label) {
    js_node_t *b = NODE(body);
    switch (b->k) {
    case N_FOR: ex_for(vm, env, body, label); return;
    case N_FORIN: ex_forin(vm, env, body, label); return;
    case N_FOROF: ex_forof(vm, env, body, label); return;
    case N_SWITCH: ex_switch(vm, env, body, label); return;
    case N_WHILE:
        while (!vm->ctl) {
            jv t = ev(vm, env, NODE(body)->a);
            if (vm->ctl || !js_truthy(vm, t)) break;
            ex(vm, env, NODE(body)->b);
            if (loop_ctl(vm, label)) break;
        }
        return;
    case N_DOWHILE:
        while (!vm->ctl) {
            jv t;
            ex(vm, env, NODE(body)->b);
            if (loop_ctl(vm, label)) break;
            t = ev(vm, env, NODE(body)->a);
            if (vm->ctl || !js_truthy(vm, t)) break;
        }
        return;
    case N_LABEL:
        ex_labeled(vm, env, b->b, b->a);
        if (vm->ctl == CTL_BREAK && vm->label == label) { vm->ctl = CTL_NONE; vm->label = 0; }
        return;
    default:
        ex(vm, env, body);
        if (vm->ctl == CTL_BREAK && vm->label == label && label) { vm->ctl = CTL_NONE; vm->label = 0; }
        return;
    }
}

static void ex_block(js_vm_t *vm, uint32_t env, uint32_t n) {
    uint32_t be = env;
    if (NODE(n)->fl & F_LEX) {
        be = js_env_new(vm, env, E_BLOCK);
        if (!be) return;
        hoist_funcs(vm, be, NODE(n)->a, 1);
    }
    ex_list(vm, be, NODE(n)->a);
    if (be != env) env_free(vm, be);
}

static void ex(js_vm_t *vm, uint32_t env, uint32_t n) {
    js_node_t *s;
    if (!n || vm->ctl) return;
    js_step(vm);
    if (vm->ctl) return;
    s = NODE(n);
    switch (s->k) {
    case N_EXPR: {
        jv v = ev(vm, env, s->a);
        if (!vm->ctl) vm->completion = v;
        return;
    }
    case N_VAR: {
        uint32_t d;
        int kind = s->op;
        for (d = s->a; d && !vm->ctl; d = NODE(d)->next) {
            js_node_t *dn = NODE(d);
            jv v = jv_undef();
            if (dn->b) {
                v = ev(vm, env, dn->b);
                if (vm->ctl) return;
                if (NODE(NODE(d)->a)->k == N_IDENT) set_fn_name(vm, v, NODE(NODE(d)->a)->a);
            } else if (kind == DK_VAR) {
                continue;     /* var x; leaves x as it is */
            }
            bind_pattern(vm, env, NODE(d)->a, v, kind);
        }
        return;
    }
    case N_FUNCDECL: return;      /* hoisted */
    case N_CLASSDECL: {
        jv c = eval_class(vm, env, s->a);
        if (vm->ctl) return;
        if (E_KIND(JENV(vm, env)->kind) == E_GLOBAL) js_env_declare(vm, env, NODE(NODE(n)->a)->a, c, V_LET);
        else js_env_declare(vm, env, NODE(NODE(n)->a)->a, c, V_LET);
        return;
    }
    case N_BLOCK: ex_block(vm, env, n); return;
    case N_EMPTY: return;
    case N_IF: {
        jv c = ev(vm, env, s->a);
        if (vm->ctl) return;
        if (js_truthy(vm, c)) ex(vm, env, NODE(n)->b);
        else if (NODE(n)->c) ex(vm, env, NODE(n)->c);
        return;
    }
    case N_FOR: case N_FORIN: case N_FOROF: case N_WHILE: case N_DOWHILE: case N_SWITCH:
        ex_labeled(vm, env, n, 0);
        return;
    case N_LABEL: ex_labeled(vm, env, s->b, s->a); return;
    case N_RETURN:
        vm->ret = s->a ? ev(vm, env, s->a) : jv_undef();
        if (s->op && s->a && !vm->ctl) vm->ret = js_co_await(vm, vm->ret);   /* async generator: return await v */
        if (!vm->ctl) vm->ctl = CTL_RETURN;
        return;
    case N_BREAK: vm->ctl = CTL_BREAK; vm->label = s->a; return;
    case N_CONTINUE: vm->ctl = CTL_CONTINUE; vm->label = s->a; return;
    case N_THROW: {
        jv v = ev(vm, env, s->a);
        if (vm->ctl) return;
        js_throw(vm, v);
        return;
    }
    case N_TRY: ex_try(vm, env, n); return;
    default:
        js_throw_err(vm, JE_SYNTAX, "unsupported statement");
        return;
    }
}

static void ex_list(js_vm_t *vm, uint32_t env, uint32_t list) {
    uint32_t s;
    for (s = list; s && !vm->ctl; s = NODE(s)->next) ex(vm, env, s);
}

jv js_run_program(js_vm_t *vm, uint32_t prog) {
    uint32_t v;
    vm->completion = jv_undef();
    for (v = NODE(prog)->d; v && !vm->ctl; v = NODE(v)->next) declare_var(vm, vm->genv, NODE(v)->a);
    hoist_funcs(vm, vm->genv, NODE(prog)->a, 0);
    ex_list(vm, vm->genv, NODE(prog)->a);
    if (vm->ctl == CTL_BREAK || vm->ctl == CTL_CONTINUE || vm->ctl == CTL_RETURN) vm->ctl = CTL_NONE;
    return vm->completion;
}
