/* wf_jsnet.c - Lot 114: fetch(), Headers, Response and XMLHttpRequest
 * of the page realm over the network hook of the page pass (see
 * wf_jsnet.h for the bounds and what is not there). */
#include "wf_jsnet.h"
#include "wf_jsapi.h"

static uint32_t g_p_headers, g_p_response, g_p_xhr, g_settle;

static int nlen(const char *s) { int n = 0; while (s[n]) n++; return n; }
static void nlog(js_vm_t *vm, const char *s) { js_log_put(vm, s, nlen(s)); }
static uint32_t key(js_vm_t *vm, const char *s) { return js_intern(vm, s, nlen(s)); }

static jv hidden_get(js_vm_t *vm, jv o, const char *name) {
    if (!jv_is_obj(o)) return jv_undef();
    return js_get(vm, o, key(vm, name));
}
static void hidden_set(js_vm_t *vm, uint32_t o, const char *name, jv v) {
    js_define(vm, o, key(vm, name), v, P_HIDDEN);
}

static jv type_error(js_vm_t *vm, const char *msg) {
    uint32_t e = js_error_new(vm, JE_TYPE, js_str_c(vm, msg));
    return e ? jv_obj(e) : jv_undef();
}

/* ---- Headers: names lowercased, values joined with ", " ---- */

static uint32_t lower_key(js_vm_t *vm, jv name) {
    uint32_t s = js_to_string(vm, name);
    char b[128];
    int i, n;
    if (vm->ctl) return 0;
    n = (int)JSTR(vm, s)->len;
    if (n > (int)sizeof(b)) n = (int)sizeof(b);
    for (i = 0; i < n; i++) {
        char c = JSTR(vm, s)->s[i];
        b[i] = (c >= 'A' && c <= 'Z') ? (char)(c + 32) : c;
    }
    return js_intern(vm, b, n);
}

static uint32_t headers_new(js_vm_t *vm) {
    uint32_t h = js_obj_new(vm, JS_CLASS_OBJECT, g_p_headers), m = js_obj_new(vm, JS_CLASS_OBJECT, 0);
    if (!h || !m) return 0;
    hidden_set(vm, h, "__h", jv_obj(m));
    return h;
}

static uint32_t headers_map(js_vm_t *vm, jv thisv) {
    jv m = hidden_get(vm, thisv, "__h");
    if (vm->ctl) return 0;
    if (!jv_is_obj(m)) { js_throw_err(vm, JE_TYPE, "Illegal invocation: not a Headers object"); return 0; }
    return m.a;
}

static void headers_put(js_vm_t *vm, uint32_t h, const char *name, const char *val, int vl) {
    jv m = hidden_get(vm, jv_obj(h), "__h");
    if (!jv_is_obj(m)) return;
    js_put(vm, m, key(vm, name), jv_str(js_str_new(vm, val, vl)));
}

/* magic: 0 get, 1 has, 2 set, 3 append, 4 delete, 5 forEach */
static jv headers_fn(js_vm_t *vm, jv thisv, jv *argv, int argc, uint32_t magic) {
    uint32_t m = headers_map(vm, thisv), k;
    jv cur;
    if (!m) return jv_undef();
    if (magic == 5) {
        uint32_t keys = js_own_keys(vm, m, 1), i, n;
        jv fn = JARG(0);
        if (!js_is_callable(vm, fn)) return js_throw_err(vm, JE_TYPE, "Headers.forEach: callback is not a function");
        n = keys ? JOBJ(vm, keys)->alen : 0;
        for (i = 0; i < n && !vm->ctl; i++) {
            jv a[3];
            a[1] = js_array_get(vm, keys, i);
            a[0] = js_get(vm, jv_obj(m), a[1].a);
            a[2] = thisv;
            js_call(vm, fn, JARG(1), a, 3);
        }
        return jv_undef();
    }
    k = lower_key(vm, JARG(0));
    JCK(vm);
    cur = js_get(vm, jv_obj(m), k);
    switch (magic) {
    case 0: return cur.t == JT_UNDEF ? jv_null() : cur;
    case 1: return jv_bool(cur.t != JT_UNDEF);
    case 2: case 3: {
        uint32_t v = js_to_string(vm, JARG(1));
        JCK(vm);
        if (magic == 3 && cur.t == JT_STR) {
            js_sb_t b;
            js_sb_init(vm, &b);
            js_sb_str(vm, &b, cur.a);
            js_sb_put(vm, &b, ", ", 2);
            js_sb_str(vm, &b, v);
            v = js_sb_finish(vm, &b);
        }
        js_put(vm, jv_obj(m), k, jv_str(v));
        return jv_undef();
    }
    case 4: (void)js_delete_own(vm, m, k); return jv_undef();
    default: return jv_undef();
    }
}

static void headers_fill(js_vm_t *vm, uint32_t h, jv init) {
    uint32_t keys, i, n;
    jv m = hidden_get(vm, jv_obj(h), "__h");
    if (!jv_is_obj(init) || !jv_is_obj(m)) return;
    if (JOBJ(vm, init.a)->cls == JS_CLASS_OBJECT && JOBJ(vm, init.a)->proto == g_p_headers) {
        init = hidden_get(vm, init, "__h");
        if (!jv_is_obj(init)) return;
    }
    keys = js_own_keys(vm, init.a, 1);
    n = keys ? JOBJ(vm, keys)->alen : 0;
    for (i = 0; i < n && !vm->ctl; i++) {
        jv kv = js_array_get(vm, keys, i), v = js_get(vm, init, kv.a);
        uint32_t lk = lower_key(vm, kv), s;
        if (vm->ctl) return;
        if (js_is_array(vm, init) && jv_is_obj(v)) {      /* [[name, value], ...] */
            lk = lower_key(vm, js_array_get(vm, v.a, 0));
            v = js_array_get(vm, v.a, 1);
        }
        s = js_to_string(vm, v);
        if (vm->ctl) return;
        js_put(vm, m, lk, jv_str(s));
    }
}

static jv headers_ctor(js_vm_t *vm, jv thisv, jv *argv, int argc, uint32_t magic) {
    uint32_t h;
    (void)thisv; (void)magic;
    if (!vm->in_new) return js_throw_err(vm, JE_TYPE, "Failed to construct 'Headers': Please use the 'new' operator");
    h = headers_new(vm);
    if (!h) return jv_undef();
    headers_fill(vm, h, JARG(0));
    return jv_obj(h);
}

/* ---- Response ---- */

static const char *status_text(int s) {
    switch (s) {
    case 200: return "OK";
    case 201: return "Created";
    case 204: return "No Content";
    case 301: return "Moved Permanently";
    case 302: return "Found";
    case 304: return "Not Modified";
    case 400: return "Bad Request";
    case 401: return "Unauthorized";
    case 403: return "Forbidden";
    case 404: return "Not Found";
    case 405: return "Method Not Allowed";
    case 500: return "Internal Server Error";
    case 502: return "Bad Gateway";
    case 503: return "Service Unavailable";
    default: return "";
    }
}

static uint32_t response_new(js_vm_t *vm, int status, uint32_t body, const char *url, const char *ctype, uint32_t headers) {
    uint32_t r = js_obj_new(vm, JS_CLASS_OBJECT, g_p_response);
    if (!r) return 0;
    if (!headers) {
        headers = headers_new(vm);
        if (!headers) return 0;
        if (ctype && ctype[0]) headers_put(vm, headers, "content-type", ctype, nlen(ctype));
    }
    js_define_c(vm, r, "status", jv_int(status), P_DEFAULT);
    js_define_c(vm, r, "ok", jv_bool(status >= 200 && status <= 299), P_DEFAULT);
    js_define_c(vm, r, "statusText", jv_str(js_str_c(vm, status_text(status))), P_DEFAULT);
    js_define_c(vm, r, "url", jv_str(js_str_c(vm, url ? url : "")), P_DEFAULT);
    js_define_c(vm, r, "redirected", jv_bool(0), P_DEFAULT);
    js_define_c(vm, r, "type", jv_str(js_str_c(vm, url && url[0] ? "basic" : "default")), P_DEFAULT);
    js_define_c(vm, r, "headers", jv_obj(headers), P_DEFAULT);
    js_define_c(vm, r, "bodyUsed", jv_bool(0), P_DEFAULT);
    hidden_set(vm, r, "__b", jv_str(body ? body : vm->s_empty));
    return r;
}

static jv json_parse(js_vm_t *vm, jv text) {
    jv json = js_get_c(vm, jv_obj(vm->global), "JSON"), parse;
    if (vm->ctl) return jv_undef();
    parse = js_get_c(vm, json, "parse");
    if (vm->ctl) return jv_undef();
    return js_call(vm, parse, json, &text, 1);
}

/* magic: 0 text, 1 json, 2 clone, 3 arrayBuffer/blob/formData (refused) */
static jv response_fn(js_vm_t *vm, jv thisv, jv *argv, int argc, uint32_t magic) {
    jv body = hidden_get(vm, thisv, "__b");
    uint32_t p;
    (void)argv; (void)argc;
    JCK(vm);
    if (body.t != JT_STR) return js_throw_err(vm, JE_TYPE, "Illegal invocation: not a Response");
    if (magic == 2) {
        uint32_t r;
        if (js_truthy(vm, js_get_c(vm, thisv, "bodyUsed"))) return js_throw_err(vm, JE_TYPE, "Failed to execute 'clone' on 'Response': Response body is already used");
        r = js_obj_new(vm, JS_CLASS_OBJECT, g_p_response);
        if (!r) return jv_undef();
        {
            static const char *const f[] = { "status", "ok", "statusText", "url", "redirected", "type", "headers", "bodyUsed", 0 };
            int i;
            for (i = 0; f[i]; i++) js_define_c(vm, r, f[i], js_get_c(vm, thisv, f[i]), P_DEFAULT);
        }
        hidden_set(vm, r, "__b", body);
        return jv_obj(r);
    }
    p = js_promise_new(vm);
    if (!p) return jv_undef();
    if (magic == 3) {
        js_promise_reject(vm, p, type_error(vm, "Response body as bytes is not supported in the guest (use text() or json())"));
        return jv_obj(p);
    }
    if (js_truthy(vm, js_get_c(vm, thisv, "bodyUsed"))) {
        js_promise_reject(vm, p, type_error(vm, magic ? "Failed to execute 'json' on 'Response': body stream already read"
                                                      : "Failed to execute 'text' on 'Response': body stream already read"));
        return jv_obj(p);
    }
    js_put(vm, thisv, key(vm, "bodyUsed"), jv_bool(1));
    if (magic == 0) { js_promise_resolve(vm, p, body); return jv_obj(p); }
    {
        jv v = json_parse(vm, body);
        if (vm->ctl == CTL_THROW) {
            jv e = vm->exc;
            vm->ctl = CTL_NONE;
            vm->exc = jv_undef();
            js_promise_reject(vm, p, e);
        } else if (!vm->ctl) js_promise_resolve(vm, p, v);
    }
    return jv_obj(p);
}

static jv response_ctor(js_vm_t *vm, jv thisv, jv *argv, int argc, uint32_t magic) {
    uint32_t body = vm->s_empty, h;
    int status = 200;
    (void)thisv; (void)magic;
    if (!vm->in_new) return js_throw_err(vm, JE_TYPE, "Failed to construct 'Response': Please use the 'new' operator");
    if (argc > 0 && argv[0].t != JT_UNDEF && argv[0].t != JT_NULL) {
        body = js_to_string(vm, argv[0]);
        JCK(vm);
    }
    h = headers_new(vm);
    if (!h) return jv_undef();
    if (jv_is_obj(JARG(1))) {
        jv s = js_get_c(vm, argv[1], "status");
        JCK(vm);
        if (s.t != JT_UNDEF) status = js_to_int32(vm, s);
        if (status < 200 || status > 599) return js_throw_err(vm, JE_RANGE, "Failed to construct 'Response': The status provided is outside the range [200, 599].");
        headers_fill(vm, h, js_get_c(vm, argv[1], "headers"));
        JCK(vm);
    }
    if (body != vm->s_empty) {
        jv ct = js_get(vm, hidden_get(vm, jv_obj(h), "__h"), key(vm, "content-type"));
        if (ct.t == JT_UNDEF) headers_put(vm, h, "content-type", "text/plain;charset=UTF-8", 24);
    }
    return jv_obj(response_new(vm, status, body, "", 0, h));
}

/* ---- the request: checks, bounds, the hook ---- */

typedef struct {
    int ok;             /* 1 an answer, 0 a network error (msg says why) */
    int status;
    uint32_t body;
    char url[512];
    char ctype[96];
    const char *msg;
} net_ans_t;

static void net_do(js_vm_t *vm, const char *what, jv urlv, const char *method, jv bodyv, uint32_t ctype, net_ans_t *a) {
    static char href[1024], abs[1100], ct[96];
    static wf_jsreq_t r;
    wf_jspage_stats_t *st = wf_jsdom_stats();
    uint32_t us = js_to_string(vm, urlv), bs = 0;
    int i, n;
    a->ok = 0;
    a->status = 0;
    a->body = vm->s_empty;
    a->url[0] = 0;
    a->ctype[0] = 0;
    a->msg = "network error";
    if (vm->ctl) return;
    if (st) st->net_requests++;
    n = (int)JSTR(vm, us)->len;
    if (n >= (int)sizeof(href)) n = (int)sizeof(href) - 1;
    for (i = 0; i < n; i++) href[i] = JSTR(vm, us)->s[i];
    href[n] = 0;
    if (st && st->net_requests > WF_DOM_NET_REQS) {
        a->msg = "request bound reached";
        st->truncated = 1;
    } else if (wf_jsdom_resolve(href, abs, (int)sizeof(abs)) != 0) {
        a->msg = "URL refused (not http or https)";
    } else if (!wf_jsdom_same_origin(abs)) {
        a->msg = "cross-origin request refused (no CORS in the guest)";
    } else {
        for (i = 0; i < (int)sizeof(r); i++) ((char *)&r)[i] = 0;
        r.url = abs;
        r.method = method;
        if (bodyv.t != JT_UNDEF && bodyv.t != JT_NULL) {
            const char *api_ct = 0;
            if (!js_api_body(vm, bodyv, &bs, &api_ct)) bs = js_to_string(vm, bodyv);      /* URLSearchParams, FormData */
            if (vm->ctl) return;
            r.body = JSTR(vm, bs)->s;
            r.body_len = (int)JSTR(vm, bs)->len;
            ct[0] = 0;
            if (ctype) {
                n = (int)JSTR(vm, ctype)->len;
                if (n >= (int)sizeof(ct)) n = (int)sizeof(ct) - 1;
                for (i = 0; i < n; i++) ct[i] = JSTR(vm, ctype)->s[i];
                ct[n] = 0;
            } else {
                const char *d = api_ct ? api_ct : "text/plain;charset=UTF-8";
                for (i = 0; d[i] && i < (int)sizeof(ct) - 1; i++) ct[i] = d[i];
                ct[i] = 0;
            }
            r.ctype = ct;
        }
        r.out_max = WF_DOM_NET_BYTES;
        if (wf_jsdom_request(&r) != 0) a->msg = "network error";
        else if (r.truncated) a->msg = "answer over the 256 KiB bound";
        else {
            a->ok = 1;
            a->status = r.status;
            a->body = js_str_new(vm, r.out, r.len);
            for (i = 0; r.final_url[i] && i < (int)sizeof(a->url) - 1; i++) a->url[i] = r.final_url[i];
            a->url[i] = 0;
            for (i = 0; r.ctype_out[i] && i < (int)sizeof(a->ctype) - 1; i++) a->ctype[i] = r.ctype_out[i];
            a->ctype[i] = 0;
            return;
        }
    }
    if (st) st->net_failed++;
    nlog(vm, "warn: ");
    nlog(vm, what);
    nlog(vm, ": ");
    nlog(vm, a->msg);
    nlog(vm, ": ");
    nlog(vm, href);
    nlog(vm, "\n");
}

/* the task that settles a fetch() promise: arg [promise, value, reject,
 * signal]; a signal aborted meanwhile rejects with its reason */
static jv settle_fn(js_vm_t *vm, jv thisv, jv *argv, int argc, uint32_t magic) {
    jv a = JARG(0), reason;
    (void)thisv; (void)magic;
    if (!jv_is_obj(a)) return jv_undef();
    if (js_api_aborted(vm, js_array_get(vm, a.a, 3), &reason)) {
        js_promise_reject(vm, js_array_get(vm, a.a, 0).a, reason);
        return jv_undef();
    }
    if (js_truthy(vm, js_array_get(vm, a.a, 2))) js_promise_reject(vm, js_array_get(vm, a.a, 0).a, js_array_get(vm, a.a, 1));
    else js_promise_resolve(vm, js_array_get(vm, a.a, 0).a, js_array_get(vm, a.a, 1));
    return jv_undef();
}

static void settle_later(js_vm_t *vm, uint32_t p, jv v, int reject, jv signal) {
    uint32_t a = js_array_new(vm, 4);
    if (!a) return;
    js_array_push(vm, a, jv_obj(p));
    js_array_push(vm, a, v);
    js_array_push(vm, a, jv_bool(reject));
    js_array_push(vm, a, signal);
    if (wf_jsdom_task(vm, jv_obj(g_settle), jv_obj(a)) != 0) {
        nlog(vm, "warn: fetch: timer bound reached, the answer is dropped\n");
        js_promise_reject(vm, p, type_error(vm, "Failed to fetch"));
    }
}

static uint32_t method_of(js_vm_t *vm, jv m, const char **out) {
    uint32_t s;
    const js_str_t *t;
    *out = "GET";
    if (m.t == JT_UNDEF) return 1;
    s = js_to_string(vm, m);
    if (vm->ctl) return 0;
    t = JSTR(vm, s);
    if (t->len == 3 && (t->s[0] | 32) == 'g' && (t->s[1] | 32) == 'e' && (t->s[2] | 32) == 't') return 1;
    if (t->len == 4 && (t->s[0] | 32) == 'p' && (t->s[1] | 32) == 'o' && (t->s[2] | 32) == 's' && (t->s[3] | 32) == 't') {
        *out = "POST";
        return 1;
    }
    return 0;
}

static uint32_t content_type_of(js_vm_t *vm, jv headers) {
    jv v;
    if (!jv_is_obj(headers)) return 0;
    if (JOBJ(vm, headers.a)->proto == g_p_headers) v = js_get(vm, hidden_get(vm, headers, "__h"), key(vm, "content-type"));
    else {
        uint32_t keys = js_own_keys(vm, headers.a, 1), i, n = keys ? JOBJ(vm, keys)->alen : 0;
        v = jv_undef();
        for (i = 0; i < n; i++) {
            jv k = js_array_get(vm, keys, i);
            if (lower_key(vm, k) == key(vm, "content-type")) v = js_get(vm, headers, k.a);
        }
    }
    if (vm->ctl || v.t == JT_UNDEF) return 0;
    return js_to_string(vm, v);
}

static jv fetch_fn(js_vm_t *vm, jv thisv, jv *argv, int argc, uint32_t magic) {
    uint32_t p = js_promise_new(vm), ctype = 0;
    jv input = JARG(0), init = JARG(1), body = jv_undef(), signal = jv_undef(), reason;
    const char *method = "GET";
    net_ans_t a;
    (void)thisv; (void)magic;
    if (!p) return jv_undef();
    if (jv_is_obj(input) && !js_is_callable(vm, input)) {    /* a Request-like object: its url */
        input = js_get_c(vm, input, "url");
        JCK(vm);
    }
    if (jv_is_obj(init)) {
        if (!method_of(vm, js_get_c(vm, init, "method"), &method)) {
            JCK(vm);
            nlog(vm, "warn: fetch: only GET and POST are supported\n");
            js_promise_reject(vm, p, type_error(vm, "Failed to fetch"));
            return jv_obj(p);
        }
        body = js_get_c(vm, init, "body");
        JCK(vm);
        ctype = content_type_of(vm, js_get_c(vm, init, "headers"));
        JCK(vm);
        signal = js_get_c(vm, init, "signal");
        JCK(vm);
        if (js_api_aborted(vm, signal, &reason)) {      /* aborted before: no request */
            js_promise_reject(vm, p, reason);
            return jv_obj(p);
        }
    }
    net_do(vm, "fetch", input, method, method[0] == 'P' ? body : jv_undef(), ctype, &a);
    JCK(vm);
    if (!a.ok) settle_later(vm, p, type_error(vm, "Failed to fetch"), 1, signal);
    else {
        uint32_t r = response_new(vm, a.status, a.body, a.url, a.ctype, 0);
        if (!r) return jv_undef();
        settle_later(vm, p, jv_obj(r), 0, signal);
    }
    return jv_obj(p);
}

/* ---- XMLHttpRequest ---- */

static void xhr_fire(js_vm_t *vm, jv x, const char *type) {
    uint32_t ev = js_obj_new(vm, JS_CLASS_OBJECT, vm->p_object), lp;
    char on[32];
    int i, n = nlen(type);
    jv h, e;
    if (!ev || vm->ctl) return;
    js_define_c(vm, ev, "type", jv_str(key(vm, type)), P_DEFAULT);
    js_define_c(vm, ev, "target", x, P_DEFAULT);
    js_define_c(vm, ev, "currentTarget", x, P_DEFAULT);
    js_define_c(vm, ev, "lengthComputable", jv_bool(0), P_DEFAULT);
    e = jv_obj(ev);
    on[0] = 'o'; on[1] = 'n';
    for (i = 0; i < n && i < 28; i++) on[2 + i] = type[i];
    h = js_get(vm, x, js_intern(vm, on, 2 + i));
    if (vm->ctl) return;
    if (js_is_callable(vm, h)) { js_call(vm, h, x, &e, 1); if (vm->ctl) return; }
    {
        jv l = hidden_get(vm, x, "__l");
        uint32_t len, q, t = key(vm, type);
        if (!jv_is_obj(l)) return;
        lp = l.a;
        len = JOBJ(vm, lp)->alen;
        for (q = 0; q < len && !vm->ctl; q++) {
            jv it = js_array_get(vm, lp, q);
            if (it.t != JT_OBJ || js_array_get(vm, it.a, 0).a != t) continue;
            js_call(vm, js_array_get(vm, it.a, 1), x, &e, 1);
        }
    }
}

static void xhr_state(js_vm_t *vm, jv x, int s) {
    js_put(vm, x, key(vm, "readyState"), jv_int(s));
    xhr_fire(vm, x, "readystatechange");
}

/* the answer reaches the object (in send() when synchronous, else in a task) */
static void xhr_finish(js_vm_t *vm, jv x) {
    jv ok = hidden_get(vm, x, "__ok"), text = hidden_get(vm, x, "__rb");
    int status;
    if (vm->ctl || js_truthy(vm, hidden_get(vm, x, "__aborted"))) return;
    status = js_to_int32(vm, hidden_get(vm, x, "__rs"));
    if (js_truthy(vm, ok)) {
        uint32_t rt = js_to_string(vm, js_get_c(vm, x, "responseType"));
        jv resp = text;
        if (vm->ctl) return;
        js_put(vm, x, key(vm, "status"), jv_int(status));
        js_put(vm, x, key(vm, "statusText"), jv_str(js_str_c(vm, status_text(status))));
        js_put(vm, x, key(vm, "responseURL"), hidden_get(vm, x, "__ru"));
        js_put(vm, x, key(vm, "responseText"), text);
        if (JSTR(vm, rt)->len == 4 && JSTR(vm, rt)->s[0] == 'j') {
            resp = json_parse(vm, text);
            if (vm->ctl == CTL_THROW) { vm->ctl = CTL_NONE; vm->exc = jv_undef(); resp = jv_null(); }
        }
        js_put(vm, x, key(vm, "response"), resp);
        xhr_state(vm, x, 4);
        if (!vm->ctl) xhr_fire(vm, x, "load");
    } else {
        xhr_state(vm, x, 4);
        if (!vm->ctl) xhr_fire(vm, x, "error");
    }
    if (!vm->ctl) xhr_fire(vm, x, "loadend");
}

static jv xhr_task(js_vm_t *vm, jv thisv, jv *argv, int argc, uint32_t magic) {
    (void)thisv; (void)magic;
    xhr_finish(vm, JARG(0));
    return jv_undef();
}

static uint32_t g_xhr_task;

/* magic: 0 open, 1 send, 2 setRequestHeader, 3 abort, 4 getResponseHeader,
 * 5 getAllResponseHeaders, 6 addEventListener, 7 removeEventListener,
 * 8 overrideMimeType */
static jv xhr_fn(js_vm_t *vm, jv thisv, jv *argv, int argc, uint32_t magic) {
    if (!jv_is_obj(thisv) || JOBJ(vm, thisv.a)->proto != g_p_xhr)
        return js_throw_err(vm, JE_TYPE, "Illegal invocation: not an XMLHttpRequest");
    switch (magic) {
    case 0: {
        const char *m;
        if (!method_of(vm, JARG(0), &m)) {
            JCK(vm);
            return js_throw_err(vm, JE_ERROR, "SyntaxError: only GET and POST are supported in the guest");
        }
        hidden_set(vm, thisv.a, "__m", jv_str(js_str_c(vm, m)));
        hidden_set(vm, thisv.a, "__u", jv_str(js_to_string(vm, JARG(1))));
        JCK(vm);
        hidden_set(vm, thisv.a, "__sync", jv_bool(argc > 2 && !js_truthy(vm, argv[2])));
        hidden_set(vm, thisv.a, "__aborted", jv_bool(0));
        hidden_set(vm, thisv.a, "__sent", jv_bool(0));
        xhr_state(vm, thisv, 1);
        return jv_undef();
    }
    case 1: {
        jv m = hidden_get(vm, thisv, "__m"), ct = hidden_get(vm, thisv, "__ct");
        const char *method;
        net_ans_t a;
        JCK(vm);
        if (m.t != JT_STR || js_to_int32(vm, js_get_c(vm, thisv, "readyState")) != 1 ||
            js_truthy(vm, hidden_get(vm, thisv, "__sent")))
            return js_throw_err(vm, JE_ERROR, "InvalidStateError: The object's state must be OPENED.");
        hidden_set(vm, thisv.a, "__sent", jv_bool(1));
        method = JSTR(vm, m.a)->s[0] == 'P' ? "POST" : "GET";
        net_do(vm, "XMLHttpRequest", hidden_get(vm, thisv, "__u"), method, method[0] == 'P' ? JARG(0) : jv_undef(),
               ct.t == JT_STR ? ct.a : 0, &a);
        JCK(vm);
        hidden_set(vm, thisv.a, "__ok", jv_bool(a.ok));
        hidden_set(vm, thisv.a, "__rs", jv_int(a.status));
        hidden_set(vm, thisv.a, "__rb", jv_str(a.body));
        hidden_set(vm, thisv.a, "__ru", jv_str(js_str_c(vm, a.url)));
        hidden_set(vm, thisv.a, "__rct", jv_str(js_str_c(vm, a.ctype)));
        if (js_truthy(vm, hidden_get(vm, thisv, "__sync"))) { xhr_finish(vm, thisv); return jv_undef(); }
        if (wf_jsdom_task(vm, jv_obj(g_xhr_task), thisv) != 0) nlog(vm, "warn: XMLHttpRequest: timer bound reached, the answer is dropped\n");
        return jv_undef();
    }
    case 2: {
        uint32_t k = lower_key(vm, JARG(0));
        JCK(vm);
        if (k == key(vm, "content-type")) hidden_set(vm, thisv.a, "__ct", jv_str(js_to_string(vm, JARG(1))));
        return jv_undef();
    }
    case 3:
        hidden_set(vm, thisv.a, "__aborted", jv_bool(1));
        js_put(vm, thisv, key(vm, "readyState"), jv_int(0));
        xhr_fire(vm, thisv, "abort");
        return jv_undef();
    case 4: {
        uint32_t k = lower_key(vm, JARG(0));
        jv ct = hidden_get(vm, thisv, "__rct");
        JCK(vm);
        if (js_to_int32(vm, js_get_c(vm, thisv, "readyState")) < 2) return jv_null();
        return k == key(vm, "content-type") && ct.t == JT_STR && JSTR(vm, ct.a)->len ? ct : jv_null();
    }
    case 5: {
        jv ct = hidden_get(vm, thisv, "__rct");
        js_sb_t b;
        if (ct.t != JT_STR || !JSTR(vm, ct.a)->len || js_to_int32(vm, js_get_c(vm, thisv, "readyState")) < 2)
            return jv_str(vm->s_empty);
        js_sb_init(vm, &b);
        js_sb_put(vm, &b, "content-type: ", 14);
        js_sb_str(vm, &b, ct.a);
        js_sb_put(vm, &b, "\r\n", 2);
        return jv_str(js_sb_finish(vm, &b));
    }
    case 6: case 7: {
        jv l = hidden_get(vm, thisv, "__l"), fn = JARG(1);
        uint32_t t = js_to_key(vm, JARG(0)), len, q;
        JCK(vm);
        if (!js_is_callable(vm, fn)) return jv_undef();
        if (!jv_is_obj(l)) {
            uint32_t a = js_array_new(vm, 2);
            if (!a) return jv_undef();
            hidden_set(vm, thisv.a, "__l", jv_obj(a));
            l = jv_obj(a);
        }
        len = JOBJ(vm, l.a)->alen;
        for (q = 0; q < len; q++) {
            jv it = js_array_get(vm, l.a, q);
            if (it.t == JT_OBJ && js_array_get(vm, it.a, 0).a == t && js_strict_eq(vm, js_array_get(vm, it.a, 1), fn)) {
                if (magic == 7) js_array_set(vm, l.a, q, jv_null());
                return jv_undef();
            }
        }
        if (magic == 6) {
            uint32_t it = js_array_new(vm, 2);
            if (!it) return jv_undef();
            js_array_push(vm, it, jv_str(t));
            js_array_push(vm, it, fn);
            js_array_push(vm, l.a, jv_obj(it));
        }
        return jv_undef();
    }
    default: return jv_undef();
    }
}

static jv xhr_ctor(js_vm_t *vm, jv thisv, jv *argv, int argc, uint32_t magic) {
    static const char *const nulls[] = { "onload", "onerror", "onreadystatechange", "onloadend", "onabort",
                                         "onprogress", "ontimeout", 0 };
    uint32_t x;
    int i;
    (void)thisv; (void)argv; (void)argc; (void)magic;
    if (!vm->in_new) return js_throw_err(vm, JE_TYPE, "Failed to construct 'XMLHttpRequest': Please use the 'new' operator");
    x = js_obj_new(vm, JS_CLASS_OBJECT, g_p_xhr);
    if (!x) return jv_undef();
    js_define_c(vm, x, "readyState", jv_int(0), P_DEFAULT);
    js_define_c(vm, x, "status", jv_int(0), P_DEFAULT);
    js_define_c(vm, x, "statusText", jv_str(vm->s_empty), P_DEFAULT);
    js_define_c(vm, x, "responseText", jv_str(vm->s_empty), P_DEFAULT);
    js_define_c(vm, x, "response", jv_str(vm->s_empty), P_DEFAULT);
    js_define_c(vm, x, "responseURL", jv_str(vm->s_empty), P_DEFAULT);
    js_define_c(vm, x, "responseType", jv_str(vm->s_empty), P_DEFAULT);
    js_define_c(vm, x, "timeout", jv_int(0), P_DEFAULT);
    js_define_c(vm, x, "withCredentials", jv_bool(0), P_DEFAULT);
    for (i = 0; nulls[i]; i++) js_define_c(vm, x, nulls[i], jv_null(), P_DEFAULT);
    return jv_obj(x);
}

int js_net_init(js_vm_t *vm) {
    static const char *const hn[] = { "get", "has", "set", "append", "delete", "forEach", 0 };
    static const char *const xn[] = { "open", "send", "setRequestHeader", "abort", "getResponseHeader",
                                      "getAllResponseHeaders", "addEventListener", "removeEventListener",
                                      "overrideMimeType", 0 };
    static const uint8_t xa[] = { 2, 0, 2, 0, 1, 0, 2, 2, 1 };
    uint32_t c;
    int i;
    g_p_headers = js_obj_new(vm, JS_CLASS_OBJECT, vm->p_object);
    g_p_response = js_obj_new(vm, JS_CLASS_OBJECT, vm->p_object);
    g_p_xhr = js_obj_new(vm, JS_CLASS_OBJECT, vm->p_object);
    if (!g_p_headers || !g_p_response || !g_p_xhr) return -1;
    for (i = 0; hn[i]; i++) js_def_native(vm, g_p_headers, hn[i], headers_fn, i == 5 ? 1 : i >= 2 && i <= 3 ? 2 : 1, (uint32_t)i);
    js_def_native(vm, g_p_response, "text", response_fn, 0, 0);
    js_def_native(vm, g_p_response, "json", response_fn, 0, 1);
    js_def_native(vm, g_p_response, "clone", response_fn, 0, 2);
    js_def_native(vm, g_p_response, "arrayBuffer", response_fn, 0, 3);
    js_def_native(vm, g_p_response, "blob", response_fn, 0, 3);
    js_def_native(vm, g_p_response, "formData", response_fn, 0, 3);
    for (i = 0; xn[i]; i++) js_def_native(vm, g_p_xhr, xn[i], xhr_fn, xa[i], (uint32_t)i);
    if (!js_ctor_new(vm, "Headers", headers_ctor, 0, g_p_headers)) return -1;
    if (!js_ctor_new(vm, "Response", response_ctor, 0, g_p_response)) return -1;
    c = js_ctor_new(vm, "XMLHttpRequest", xhr_ctor, 0, g_p_xhr);
    if (!c) return -1;
    {
        static const char *const sn[] = { "UNSENT", "OPENED", "HEADERS_RECEIVED", "LOADING", "DONE", 0 };
        for (i = 0; sn[i]; i++) {
            js_define_c(vm, c, sn[i], jv_int(i), P_DEFAULT);
            js_define_c(vm, g_p_xhr, sn[i], jv_int(i), P_DEFAULT);
        }
    }
    js_def_native(vm, vm->global, "fetch", fetch_fn, 1, 0);
    g_settle = js_native_new(vm, "settle", settle_fn, 1, 0);
    g_xhr_task = js_native_new(vm, "xhrDone", xhr_task, 1, 0);
    return g_settle && g_xhr_task && !vm->ctl ? 0 : -1;
}

/* Lot 115: the prototypes and the pending XHR task kept here (wf_jsgc.c) */
void js_net_gc_roots(js_vm_t *vm) {
    const uint32_t r[] = { g_p_headers, g_p_response, g_p_xhr, g_settle, g_xhr_task };
    js_gc_scan(vm, r, (uint32_t)sizeof(r));
}
