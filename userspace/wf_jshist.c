/* wf_jshist.c - Lot 114: history and structuredClone of the page realm
 * (see wf_jshist.h for what is kept and what is not). */
#include "wf_jshist.h"
#include "wf_jsapi.h"

#define HIST_URL 2048

static uint32_t g_entries;      /* array of [url string, state] */
static int g_idx, g_count;
static uint32_t g_p_hist, g_trav;
static char g_cur[HIST_URL], g_new[HIST_URL];

static int hlen(const char *s) { int n = 0; while (s[n]) n++; return n; }

static int hcstr(js_vm_t *vm, uint32_t s, char *out, int cap) {
    int n = (int)JSTR(vm, s)->len, i;
    if (n > cap - 1) n = cap - 1;
    for (i = 0; i < n; i++) out[i] = JSTR(vm, s)->s[i];
    out[n] = 0;
    return n;
}

static jv throw_dom(js_vm_t *vm, const char *msg, const char *name) {
    uint32_t e = js_api_domexc(vm, msg, name);
    if (!e) return jv_undef();
    return js_throw(vm, jv_obj(e));
}

/* ---- structuredClone ---- */

static uint32_t g_msrc[WF_JSHIST_CLONE_OBJS], g_mdst[WF_JSHIST_CLONE_OBJS];
static int g_mn;

static jv clone_v(js_vm_t *vm, jv v, int depth);

static jv construct_global(js_vm_t *vm, const char *name, jv *argv, int argc) {
    jv c = js_get_c(vm, jv_obj(vm->global), name);
    if (vm->ctl) return jv_undef();
    if (!js_is_callable(vm, c)) return js_throw_err(vm, JE_TYPE, "structuredClone: a built-in constructor is missing");
    return js_construct(vm, c, argv, argc, c);
}

static jv call_method(js_vm_t *vm, jv o, const char *name) {
    jv f = js_get_c(vm, o, name);
    if (vm->ctl) return jv_undef();
    if (!js_is_callable(vm, f)) return jv_undef();
    return js_call(vm, f, o, 0, 0);
}

static jv unsupported(js_vm_t *vm, jv v) {
    js_obj_t *o = JOBJ(vm, v.a);
    if (o->cls == JS_CLASS_FUNC || o->cls == JS_CLASS_NATIVE || o->cls == JS_CLASS_BOUND) {
        uint32_t s = js_to_string(vm, v), m;
        if (vm->ctl) return jv_undef();
        m = js_str_cat(vm, s, js_str_c(vm, " could not be cloned."));
        {
            uint32_t e = js_api_domexc(vm, "", "DataCloneError");
            if (!e) return jv_undef();
            js_define_c(vm, e, "message", jv_str(m), P_HIDDEN);
            return js_throw(vm, jv_obj(e));
        }
    }
    return throw_dom(vm, "Cannot clone object of unsupported type.", "DataCloneError");
}

static int memo_find(uint32_t src) {
    int i;
    for (i = 0; i < g_mn; i++) if (g_msrc[i] == src) return i;
    return -1;
}

static int memo_add(js_vm_t *vm, uint32_t src, uint32_t dst) {
    if (g_mn >= WF_JSHIST_CLONE_OBJS) {
        js_throw_err(vm, JE_RANGE, "structuredClone: more objects than the clone bound");
        return -1;
    }
    g_msrc[g_mn] = src;
    g_mdst[g_mn] = dst;
    g_mn++;
    return 0;
}

static int is_error_name(const char *n) {
    static const char *const names[] = { "Error", "TypeError", "RangeError", "SyntaxError", "ReferenceError",
                                         "URIError", "EvalError", 0 };
    int i, j;
    for (i = 0; names[i]; i++) {
        for (j = 0; names[i][j] && names[i][j] == n[j]; j++) {}
        if (!names[i][j] && !n[j]) return 1;
    }
    return 0;
}

static jv clone_error(js_vm_t *vm, jv v) {
    char name[24];
    jv nv = js_get_c(vm, v, "name"), mv, e;
    uint32_t key = js_intern(vm, "message", 7);
    js_prop_t *p;
    if (vm->ctl) return jv_undef();
    name[0] = 0;
    if (nv.t == JT_STR) hcstr(vm, nv.a, name, (int)sizeof(name));
    if (!is_error_name(name)) { name[0] = 'E'; name[1] = 'r'; name[2] = 'r'; name[3] = 'o'; name[4] = 'r'; name[5] = 0; }
    p = js_own_prop(vm, v.a, key);
    mv = p ? js_get(vm, v, key) : jv_undef();
    if (vm->ctl) return jv_undef();
    if (p) { uint32_t s = js_to_string(vm, mv); if (vm->ctl) return jv_undef(); mv = jv_str(s); }
    e = construct_global(vm, name, &mv, p ? 1 : 0);
    if (vm->ctl || !jv_is_obj(e)) return jv_undef();
    if (memo_add(vm, v.a, e.a) != 0) return jv_undef();
    return e;
}

static jv clone_keyed(js_vm_t *vm, jv v, int is_map, int depth) {
    uint32_t vals, out, i, n;
    jv arg, res;
    js_iter_values(vm, v, &vals);
    if (vm->ctl || !vals) return jv_undef();
    res = construct_global(vm, is_map ? "Map" : "Set", 0, 0);
    if (vm->ctl || !jv_is_obj(res)) return jv_undef();
    if (memo_add(vm, v.a, res.a) != 0) return jv_undef();
    n = JOBJ(vm, vals)->alen;
    out = js_array_new(vm, n ? n : 1);
    if (!out) return jv_undef();
    for (i = 0; i < n; i++) {
        jv e = js_array_get(vm, vals, i);
        if (is_map) {
            uint32_t pair = js_array_new(vm, 2);
            jv k, x;
            if (!pair) return jv_undef();
            k = clone_v(vm, js_array_get(vm, e.a, 0), depth + 1);
            if (vm->ctl) return jv_undef();
            x = clone_v(vm, js_array_get(vm, e.a, 1), depth + 1);
            if (vm->ctl) return jv_undef();
            js_array_push(vm, pair, k);
            js_array_push(vm, pair, x);
            js_array_push(vm, out, jv_obj(pair));
        } else {
            jv x = clone_v(vm, e, depth + 1);
            if (vm->ctl) return jv_undef();
            js_array_push(vm, out, x);
        }
    }
    /* fill the copy made first (so a cycle through it finds it) */
    {
        jv adder = js_get_c(vm, res, is_map ? "set" : "add");
        if (vm->ctl || !js_is_callable(vm, adder)) return jv_undef();
        for (i = 0; i < n; i++) {
            jv e = js_array_get(vm, out, i), a2[2];
            if (is_map) { a2[0] = js_array_get(vm, e.a, 0); a2[1] = js_array_get(vm, e.a, 1); js_call(vm, adder, res, a2, 2); }
            else { arg = e; js_call(vm, adder, res, &arg, 1); }
            if (vm->ctl) return jv_undef();
        }
    }
    return res;
}

static jv clone_array(js_vm_t *vm, jv v, int depth) {
    uint32_t len = JOBJ(vm, v.a)->alen, i, a;
    a = js_array_new(vm, len ? len : 1);
    if (!a) return jv_undef();
    if (memo_add(vm, v.a, a) != 0) return jv_undef();
    js_array_set_len(vm, a, len);
    for (i = 0; i < len; i++) {
        jv e, x;
        if (i >= JOBJ(vm, v.a)->alen) break;        /* a getter cannot run here, but stay safe */
        e = ((jv *)JPTR(vm, JOBJ(vm, v.a)->arr))[i];  /* js_array_get reads a hole as undefined */
        if (e.t == JT_EMPTY) continue;      /* a hole stays a hole */
        x = clone_v(vm, e, depth + 1);
        if (vm->ctl) return jv_undef();
        js_array_set(vm, a, i, x);
    }
    return jv_obj(a);
}

static jv clone_plain(js_vm_t *vm, jv v, int depth) {
    uint32_t o = js_obj_new(vm, JS_CLASS_OBJECT, vm->p_object), keys, i, n;
    if (!o) return jv_undef();
    if (memo_add(vm, v.a, o) != 0) return jv_undef();
    keys = js_own_keys(vm, v.a, 1);
    if (vm->ctl || !keys) return jv_undef();
    n = JOBJ(vm, keys)->alen;
    for (i = 0; i < n; i++) {
        uint32_t k = js_to_key(vm, js_array_get(vm, keys, i));
        jv x = js_get(vm, v, k);
        if (vm->ctl) return jv_undef();
        x = clone_v(vm, x, depth + 1);
        if (vm->ctl) return jv_undef();
        js_define(vm, o, k, x, P_DEFAULT);
    }
    return jv_obj(o);
}

static jv clone_v(js_vm_t *vm, jv v, int depth) {
    js_obj_t *o;
    int m;
    if (!jv_is_obj(v)) return v;               /* primitives are copied as they are */
    m = memo_find(v.a);
    if (m >= 0) return jv_obj(g_mdst[m]);
    if (depth > WF_JSHIST_CLONE_DEPTH) return js_throw_err(vm, JE_RANGE, "structuredClone: nesting deeper than the clone bound");
    if (js_api_platform(vm, v)) return unsupported(vm, v);
    o = JOBJ(vm, v.a);
    switch (o->cls) {
    case JS_CLASS_OBJECT: case JS_CLASS_ARGS: return clone_plain(vm, v, depth);
    case JS_CLASS_ARRAY: return clone_array(vm, v, depth);
    case JS_CLASS_MAP: return clone_keyed(vm, v, 1, depth);
    case JS_CLASS_SET: return clone_keyed(vm, v, 0, depth);
    case JS_CLASS_ERROR: return clone_error(vm, v);
    case JS_CLASS_DATE: case JS_CLASS_BOOLEAN: case JS_CLASS_NUMBER: case JS_CLASS_STRING: {
        static const char *const ctor[] = { "Date", "Boolean", "Number", "String" };
        int w = o->cls == JS_CLASS_DATE ? 0 : o->cls == JS_CLASS_BOOLEAN ? 1 : o->cls == JS_CLASS_NUMBER ? 2 : 3;
        jv prim = call_method(vm, v, w == 0 ? "getTime" : "valueOf"), r;
        if (vm->ctl) return jv_undef();
        r = construct_global(vm, ctor[w], &prim, 1);
        if (vm->ctl || !jv_is_obj(r)) return jv_undef();
        if (memo_add(vm, v.a, r.a) != 0) return jv_undef();
        return r;
    }
    case JS_CLASS_REGEXP: {
        jv a[2], r;
        a[0] = js_get_c(vm, v, "source");
        if (vm->ctl) return jv_undef();
        a[1] = js_get_c(vm, v, "flags");
        if (vm->ctl) return jv_undef();
        r = construct_global(vm, "RegExp", a, 2);
        if (vm->ctl || !jv_is_obj(r)) return jv_undef();
        if (memo_add(vm, v.a, r.a) != 0) return jv_undef();
        return r;
    }
    default: return unsupported(vm, v);    /* functions, DOM nodes, promises, generators, iterators */
    }
}

jv js_hist_clone(js_vm_t *vm, jv v) {
    jv r;
    g_mn = 0;
    r = clone_v(vm, v, 0);
    g_mn = 0;
    return r;
}

static jv clone_fn(js_vm_t *vm, jv thisv, jv *argv, int argc, uint32_t magic) {
    (void)thisv; (void)magic;
    if (argc < 1) return js_throw_err(vm, JE_TYPE, "structuredClone: 1 argument required, but only 0 present.");
    if (argc > 1 && jv_is_obj(argv[1])) {
        jv t = js_get_c(vm, argv[1], "transfer");
        JCK(vm);
        if (jv_is_obj(t) && JOBJ(vm, t.a)->cls == JS_CLASS_ARRAY && JOBJ(vm, t.a)->alen > 0)
            return throw_dom(vm, "Value not transferable", "DataCloneError");   /* nothing is transferable here */
    }
    return js_hist_clone(vm, argv[0]);
}

/* ---- history ---- */

static jv entry(js_vm_t *vm, int i) { return js_array_get(vm, g_entries, (uint32_t)i); }

static int entry_url(js_vm_t *vm, int i, char *out, int cap) {
    jv e = entry(vm, i), u;
    out[0] = 0;
    if (!jv_is_obj(e)) return -1;
    u = js_array_get(vm, e.a, 0);
    if (u.t != JT_STR) return -1;
    hcstr(vm, u.a, out, cap);
    return 0;
}

static uint32_t entry_new(js_vm_t *vm, const char *url, jv state) {
    uint32_t e = js_array_new(vm, 2);
    if (!e) return 0;
    js_array_push(vm, e, jv_str(js_str_c(vm, url)));
    js_array_push(vm, e, state);
    return e;
}

/* location.href of the page into g_cur */
static int cur_url(js_vm_t *vm) {
    jv loc = js_get_c(vm, jv_obj(vm->global), "location"), h;
    g_cur[0] = 0;
    if (vm->ctl || !jv_is_obj(loc)) return -1;
    h = js_get_c(vm, loc, "href");
    if (vm->ctl || h.t != JT_STR) return -1;
    hcstr(vm, h.a, g_cur, (int)sizeof(g_cur));
    return 0;
}

static int only_fragment_differs(const char *a, const char *b) {
    int i = 0;
    while (a[i] && a[i] != '#' && a[i] == b[i]) i++;
    if ((a[i] && a[i] != '#') || (b[i] && b[i] != '#')) return 0;
    for (; a[i] || b[i]; i++) if (a[i] != b[i]) return 1;
    return 0;
}

static jv hist_push(js_vm_t *vm, jv *argv, int argc, int replace) {
    jv state;
    uint32_t e;
    const char *what = replace ? "replaceState" : "pushState";
    if (argc < 2) {
        static char msg[96];
        const char *a = "Failed to execute '", *b = "' on 'History': 2 arguments required, but only ", *c = " present.";
        int o = 0, i;
        for (i = 0; a[i]; i++) msg[o++] = a[i];
        for (i = 0; what[i]; i++) msg[o++] = what[i];
        for (i = 0; b[i]; i++) msg[o++] = b[i];
        msg[o++] = (char)('0' + argc);
        for (i = 0; c[i]; i++) msg[o++] = c[i];
        msg[o] = 0;
        return js_throw_err(vm, JE_TYPE, msg);
    }
    state = js_hist_clone(vm, argv[0]);
    JCK(vm);
    if (cur_url(vm) != 0) return jv_undef();
    if (argc > 2 && argv[2].t != JT_UNDEF && argv[2].t != JT_NULL) {
        char rel[HIST_URL];
        uint32_t s = js_to_string(vm, argv[2]);
        JCK(vm);
        hcstr(vm, s, rel, (int)sizeof(rel));
        if (js_api_resolve(rel, g_cur, g_new, (int)sizeof(g_new)) != 0 || !js_api_same_document(g_cur, g_new)) {
            static char msg[HIST_URL + 160];
            const char *parts[6];
            jv org = js_get_c(vm, js_get_c(vm, jv_obj(vm->global), "location"), "origin");
            char origin[300];
            int o = 0, i, p;
            origin[0] = 0;
            if (org.t == JT_STR) hcstr(vm, org.a, origin, (int)sizeof(origin));
            parts[0] = "Failed to execute '"; parts[1] = what;
            parts[2] = "' on 'History': A history state object with URL '"; parts[3] = rel;
            parts[4] = "' cannot be created in a document with origin '"; parts[5] = origin;
            for (p = 0; p < 6; p++) for (i = 0; parts[p][i] && o < (int)sizeof(msg) - 200; i++) msg[o++] = parts[p][i];
            {
                const char *t1 = "' and URL '", *t2 = "'.";
                for (i = 0; t1[i]; i++) msg[o++] = t1[i];
                for (i = 0; g_cur[i] && o < (int)sizeof(msg) - 4; i++) msg[o++] = g_cur[i];
                for (i = 0; t2[i]; i++) msg[o++] = t2[i];
            }
            msg[o] = 0;
            return throw_dom(vm, msg, "SecurityError");
        }
    } else {
        int i;
        for (i = 0; g_cur[i]; i++) g_new[i] = g_cur[i];
        g_new[i] = 0;
    }
    e = entry_new(vm, g_new, state);
    if (!e) return jv_undef();
    if (replace) js_array_set(vm, g_entries, (uint32_t)g_idx, jv_obj(e));
    else {
        js_array_set_len(vm, g_entries, (uint32_t)(g_idx + 1));     /* the forward entries go */
        js_array_push(vm, g_entries, jv_obj(e));
        g_idx++;
        if (g_idx + 1 > WF_JSHIST_ENTRIES) {               /* the oldest entry is dropped */
            uint32_t k, n = JOBJ(vm, g_entries)->alen, na = js_array_new(vm, n);
            if (!na) return jv_undef();
            for (k = 1; k < n; k++) js_array_push(vm, na, js_array_get(vm, g_entries, k));
            g_entries = na;
            js_define_c(vm, g_p_hist, "__e", jv_obj(na), P_HIDDEN);   /* keeps it reachable for inspection */
            g_idx--;
        }
        g_count = g_idx + 1;
    }
    if (hlen(g_new) > 0 && wf_jsdom_set_location(vm, g_new) != 0)
        return js_throw_err(vm, JE_RANGE, "history: the URL is longer than the page URL bound");
    return jv_undef();
}

/* the traversal task: delta applied to the entry current when it runs */
static jv trav_fn(js_vm_t *vm, jv thisv, jv *argv, int argc, uint32_t magic) {
    int delta = argc > 0 ? js_to_int32(vm, argv[0]) : 0, target;
    jv e;
    (void)thisv; (void)magic;
    JCK(vm);
    target = g_idx + delta;
    if (!delta || target < 0 || target >= g_count) return jv_undef();
    entry_url(vm, g_idx, g_cur, (int)sizeof(g_cur));
    g_idx = target;
    entry_url(vm, g_idx, g_new, (int)sizeof(g_new));
    if (wf_jsdom_set_location(vm, g_new) != 0) return jv_undef();
    e = entry(vm, g_idx);
    wf_jsdom_window_event(vm, "popstate", "state", js_array_get(vm, e.a, 1), "hasUAVisualTransition", jv_bool(0));
    if (!vm->ctl && only_fragment_differs(g_cur, g_new))
        wf_jsdom_window_event(vm, "hashchange", "oldURL", jv_str(js_str_c(vm, g_cur)), "newURL",
                              jv_str(js_str_c(vm, g_new)));
    return jv_undef();
}

static jv hist_fn(js_vm_t *vm, jv thisv, jv *argv, int argc, uint32_t magic) {
    int delta;
    if (!jv_is_obj(thisv) || JOBJ(vm, thisv.a)->proto != g_p_hist)
        return js_throw_err(vm, JE_TYPE, "Illegal invocation: not a History");
    switch (magic) {
    case 0: return hist_push(vm, argv, argc, 0);
    case 1: return hist_push(vm, argv, argc, 1);
    case 2: delta = -1; break;
    case 3: delta = 1; break;
    default:
        delta = argc > 0 ? js_to_int32(vm, argv[0]) : 0;
        JCK(vm);
        break;
    }
    if (!delta) return jv_undef();                    /* go(0) would reload: not done in the pass */
    if (wf_jsdom_task(vm, jv_obj(g_trav), jv_int(delta)) != 0)
        return js_throw_err(vm, JE_RANGE, "history: task bound reached");
    return jv_undef();
}

static jv hist_get(js_vm_t *vm, jv thisv, jv *argv, int argc, uint32_t magic) {
    (void)argv; (void)argc;
    if (!jv_is_obj(thisv) || JOBJ(vm, thisv.a)->proto != g_p_hist)
        return js_throw_err(vm, JE_TYPE, "Illegal invocation: not a History");
    switch (magic) {
    case 0: return jv_int(g_count);
    case 1: { jv e = entry(vm, g_idx); return jv_is_obj(e) ? js_array_get(vm, e.a, 1) : jv_null(); }
    default: return js_get_c(vm, jv_obj(g_p_hist), "__sr");
    }
}

static jv hist_set(js_vm_t *vm, jv thisv, jv *argv, int argc, uint32_t magic) {
    uint32_t s;
    char v[8];
    (void)magic;
    if (!jv_is_obj(thisv) || JOBJ(vm, thisv.a)->proto != g_p_hist)
        return js_throw_err(vm, JE_TYPE, "Illegal invocation: not a History");
    s = js_to_string(vm, JARG(0));
    JCK(vm);
    hcstr(vm, s, v, (int)sizeof(v));
    /* an enum attribute: another value is ignored */
    if ((v[0] == 'a' && v[1] == 'u' && v[2] == 't' && v[3] == 'o' && !v[4]) ||
        (v[0] == 'm' && v[1] == 'a' && v[2] == 'n' && v[3] == 'u' && v[4] == 'a' && v[5] == 'l' && !v[6]))
        js_define_c(vm, g_p_hist, "__sr", jv_str(s), P_HIDDEN);
    (void)argc;
    return jv_undef();
}

static jv hist_illegal(js_vm_t *vm, jv thisv, jv *argv, int argc, uint32_t magic) {
    (void)thisv; (void)argv; (void)argc; (void)magic;
    return js_throw_err(vm, JE_TYPE, "Illegal constructor");
}

int js_hist_init(js_vm_t *vm, const char *page_url) {
    static const char *const gn[] = { "length", "state", "scrollRestoration", 0 };
    static const char *const mn[] = { "pushState", "replaceState", "back", "forward", "go", 0 };
    static const int na[] = { 2, 2, 0, 0, 0 };
    uint32_t h, e;
    int i;
    g_mn = 0;
    g_idx = 0;
    g_count = 1;
    g_p_hist = js_obj_new(vm, JS_CLASS_OBJECT, vm->p_object);
    g_entries = js_array_new(vm, 8);
    g_trav = js_native_new(vm, "traverse", trav_fn, 1, 0);
    if (!g_p_hist || !g_entries || !g_trav) return -1;
    js_define_c(vm, g_p_hist, "__e", jv_obj(g_entries), P_HIDDEN);
    js_define_c(vm, g_p_hist, "__t", jv_obj(g_trav), P_HIDDEN);
    js_define_c(vm, g_p_hist, "__sr", jv_str(js_str_c(vm, "auto")), P_HIDDEN);
    for (i = 0; gn[i]; i++) {
        uint32_t g = js_native_new(vm, gn[i], hist_get, 0, (uint32_t)i), s = 0;
        if (i == 2) s = js_native_new(vm, gn[i], hist_set, 1, 0);
        if (!g || (i == 2 && !s)) return -1;
        if (!js_define_accessor(vm, g_p_hist, js_intern(vm, gn[i], hlen(gn[i])), g, s, P_CONF)) return -1;
    }
    for (i = 0; mn[i]; i++) js_def_native(vm, g_p_hist, mn[i], hist_fn, na[i], (uint32_t)i);
    if (!js_ctor_new(vm, "History", hist_illegal, 0, g_p_hist)) return -1;
    h = js_obj_new(vm, JS_CLASS_OBJECT, g_p_hist);
    e = entry_new(vm, page_url && page_url[0] ? page_url : "about:blank", jv_null());
    if (!h || !e) return -1;
    js_array_push(vm, g_entries, jv_obj(e));
    js_define_c(vm, vm->global, "history", jv_obj(h), P_HIDDEN);
    js_def_native(vm, vm->global, "structuredClone", clone_fn, 1, 0);
    return 0;
}
