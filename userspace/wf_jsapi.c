/* wf_jsapi.c - Lot 114: URL, URLSearchParams, FormData, localStorage and
 * sessionStorage, AbortController / AbortSignal and DOMException in the
 * page realm (see wf_jsapi.h for the bounds and what is not there). */
#include "wf_jsapi.h"

static uint32_t g_p_usp, g_p_fd, g_p_url, g_p_storage, g_p_signal, g_p_ctrl, g_p_domexc;

static int alen(const char *s) { int n = 0; while (s[n]) n++; return n; }
static uint32_t akey(js_vm_t *vm, const char *s) { return js_intern(vm, s, alen(s)); }
static jv hget(js_vm_t *vm, jv o, const char *name) {
    if (!jv_is_obj(o)) return jv_undef();
    return js_get(vm, o, akey(vm, name));
}
static void hset(js_vm_t *vm, uint32_t o, const char *name, jv v) { js_define(vm, o, akey(vm, name), v, P_HIDDEN); }
static int is_of(js_vm_t *vm, jv v, uint32_t proto) { return jv_is_obj(v) && JOBJ(vm, v.a)->proto == proto; }

/* copy a JS string into a C buffer (cut at cap - 1); its length */
static int cstr(js_vm_t *vm, uint32_t s, char *out, int cap) {
    int n = (int)JSTR(vm, s)->len, i;
    if (n > cap - 1) n = cap - 1;
    for (i = 0; i < n; i++) out[i] = JSTR(vm, s)->s[i];
    out[n] = 0;
    return n;
}

/* ---- DOMException: an Error with a name and a legacy code ---- */

static int domexc_code(const char *name) {
    static const struct { const char *n; int c; } t[] = {
        { "IndexSizeError", 1 }, { "HierarchyRequestError", 3 }, { "WrongDocumentError", 4 },
        { "InvalidCharacterError", 5 }, { "NotFoundError", 8 }, { "NotSupportedError", 9 },
        { "InvalidStateError", 11 }, { "SyntaxError", 12 }, { "InvalidModificationError", 13 },
        { "NamespaceError", 14 }, { "InvalidAccessError", 15 }, { "SecurityError", 18 }, { "NetworkError", 19 },
        { "AbortError", 20 }, { "URLMismatchError", 21 }, { "QuotaExceededError", 22 }, { "TimeoutError", 23 },
        { "DataCloneError", 25 }, { 0, 0 } };
    int i, j;
    for (i = 0; t[i].n; i++) {
        for (j = 0; t[i].n[j] && t[i].n[j] == name[j]; j++) {}
        if (!t[i].n[j] && !name[j]) return t[i].c;
    }
    return 0;
}

static uint32_t domexc_new(js_vm_t *vm, uint32_t msg, uint32_t name) {
    uint32_t e = js_obj_new(vm, JS_CLASS_ERROR, g_p_domexc);
    char nb[40];
    if (!e) return 0;
    js_define(vm, e, vm->s_message, jv_str(msg ? msg : vm->s_empty), P_HIDDEN);
    js_define(vm, e, vm->s_name, jv_str(name), P_HIDDEN);
    cstr(vm, name, nb, (int)sizeof(nb));
    js_define_c(vm, e, "code", jv_int(domexc_code(nb)), P_HIDDEN);
    return e;
}

static uint32_t domexc_c(js_vm_t *vm, const char *msg, const char *name) {
    return domexc_new(vm, js_str_c(vm, msg), js_str_c(vm, name));
}

static jv domexc_ctor(js_vm_t *vm, jv thisv, jv *argv, int argc, uint32_t magic) {
    uint32_t msg = vm->s_empty, name, e;
    (void)thisv; (void)magic;
    if (!vm->in_new) return js_throw_err(vm, JE_TYPE, "Failed to construct 'DOMException': Please use the 'new' operator");
    if (argc > 0 && argv[0].t != JT_UNDEF) { msg = js_to_string(vm, argv[0]); JCK(vm); }
    if (argc > 1 && argv[1].t != JT_UNDEF) { name = js_to_string(vm, argv[1]); JCK(vm); }
    else name = js_str_c(vm, "Error");
    e = domexc_new(vm, msg, name);
    return e ? jv_obj(e) : jv_undef();
}

/* ---- ordered string pairs (URLSearchParams, FormData) ----
 * hidden "__l": an array of [name, value] arrays */

static uint32_t pair_new(js_vm_t *vm, uint32_t n, uint32_t v) {
    uint32_t p = js_array_new(vm, 2);
    if (!p) return 0;
    js_array_push(vm, p, jv_str(n));
    js_array_push(vm, p, jv_str(v));
    return p;
}

static void list_push(js_vm_t *vm, uint32_t list, uint32_t n, uint32_t v) {
    uint32_t p = pair_new(vm, n, v);
    if (p) js_array_push(vm, list, jv_obj(p));
}

static uint32_t pair_name(js_vm_t *vm, uint32_t list, uint32_t i) {
    return js_array_get(vm, js_array_get(vm, list, i).a, 0).a;
}
static uint32_t pair_value(js_vm_t *vm, uint32_t list, uint32_t i) {
    return js_array_get(vm, js_array_get(vm, list, i).a, 1).a;
}

/* drop pairs [i] (keeps the order) */
static void list_remove(js_vm_t *vm, uint32_t list, uint32_t i) {
    uint32_t n = JOBJ(vm, list)->alen, k;
    for (k = i; k + 1 < n; k++) js_array_set(vm, list, k, js_array_get(vm, list, k + 1));
    js_array_set_len(vm, list, n - 1);
}

/* application/x-www-form-urlencoded */
static int hexv(char c) {
    if (c >= '0' && c <= '9') return c - '0';
    if (c >= 'a' && c <= 'f') return c - 'a' + 10;
    if (c >= 'A' && c <= 'F') return c - 'A' + 10;
    return -1;
}

static uint32_t form_decode(js_vm_t *vm, const char *s, int n) {
    js_sb_t b;
    int i;
    js_sb_init(vm, &b);
    for (i = 0; i < n; i++) {
        if (s[i] == '+') js_sb_ch(vm, &b, ' ');
        else if (s[i] == '%' && i + 2 < n && hexv(s[i + 1]) >= 0 && hexv(s[i + 2]) >= 0) {
            js_sb_ch(vm, &b, (char)(hexv(s[i + 1]) * 16 + hexv(s[i + 2])));
            i += 2;
        } else js_sb_ch(vm, &b, s[i]);
    }
    return js_sb_finish(vm, &b);
}

static void form_encode(js_vm_t *vm, js_sb_t *b, uint32_t s) {
    static const char hex[] = "0123456789ABCDEF";
    const js_str_t *t = JSTR(vm, s);
    uint32_t i;
    for (i = 0; i < t->len; i++) {
        unsigned char c = (unsigned char)JSTR(vm, s)->s[i];
        if ((c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') || (c >= '0' && c <= '9') || c == '*' || c == '-' ||
            c == '.' || c == '_')
            js_sb_ch(vm, b, (char)c);
        else if (c == ' ') js_sb_ch(vm, b, '+');
        else {
            js_sb_ch(vm, b, '%');
            js_sb_ch(vm, b, hex[c >> 4]);
            js_sb_ch(vm, b, hex[c & 15]);
        }
    }
}

static void list_parse(js_vm_t *vm, uint32_t list, const char *s, int n) {
    int i = 0;
    if (n > 0 && s[0] == '?') i = 1;
    while (i < n && !vm->ctl) {
        int e = i, eq = -1;
        while (e < n && s[e] != '&') { if (s[e] == '=' && eq < 0) eq = e; e++; }
        if (e > i) {
            uint32_t nm, v;
            if (eq < 0) { nm = form_decode(vm, s + i, e - i); v = vm->s_empty; }
            else { nm = form_decode(vm, s + i, eq - i); v = form_decode(vm, s + eq + 1, e - eq - 1); }
            list_push(vm, list, nm, v);
        }
        i = e + 1;
    }
}

static uint32_t list_serialize(js_vm_t *vm, uint32_t list) {
    js_sb_t b;
    uint32_t i, n = JOBJ(vm, list)->alen;
    js_sb_init(vm, &b);
    for (i = 0; i < n; i++) {
        if (i) js_sb_ch(vm, &b, '&');
        form_encode(vm, &b, pair_name(vm, list, i));
        js_sb_ch(vm, &b, '=');
        form_encode(vm, &b, pair_value(vm, list, i));
    }
    return js_sb_finish(vm, &b);
}

/* fill list from a constructor argument: a string, a record, an
 * iterable of pairs, or another URLSearchParams / FormData */
static void list_fill(js_vm_t *vm, uint32_t list, jv init, const char *who) {
    if (init.t == JT_UNDEF || init.t == JT_NULL) return;
    if (is_of(vm, init, g_p_usp) || is_of(vm, init, g_p_fd)) {
        jv l = hget(vm, init, "__l");
        uint32_t i, n;
        if (!jv_is_obj(l)) return;
        n = JOBJ(vm, l.a)->alen;
        for (i = 0; i < n; i++) list_push(vm, list, pair_name(vm, l.a, i), pair_value(vm, l.a, i));
        return;
    }
    if (jv_is_obj(init) && js_is_array(vm, init)) {
        uint32_t i, n = JOBJ(vm, init.a)->alen;
        for (i = 0; i < n && !vm->ctl; i++) {
            jv p = js_array_get(vm, init.a, i);
            uint32_t a, b;
            if (!jv_is_obj(p) || !js_is_array(vm, p) || JOBJ(vm, p.a)->alen != 2) {
                js_sb_t m;
                js_sb_init(vm, &m);
                js_sb_put(vm, &m, "Failed to construct '", 21);
                js_sb_put(vm, &m, who, alen(who));
                js_sb_put(vm, &m, "': Invalid pair: each pair must have exactly two items", 54);
                js_throw(vm, jv_obj(js_error_new(vm, JE_TYPE, js_sb_finish(vm, &m))));
                return;
            }
            a = js_to_string(vm, js_array_get(vm, p.a, 0));
            if (vm->ctl) return;
            b = js_to_string(vm, js_array_get(vm, p.a, 1));
            if (vm->ctl) return;
            list_push(vm, list, a, b);
        }
        return;
    }
    if (jv_is_obj(init)) {
        uint32_t keys = js_own_keys(vm, init.a, 1), i, n = keys ? JOBJ(vm, keys)->alen : 0;
        for (i = 0; i < n && !vm->ctl; i++) {
            jv k = js_array_get(vm, keys, i);
            uint32_t v = js_to_string(vm, js_get(vm, init, k.a));
            if (vm->ctl) return;
            list_push(vm, list, k.a, v);
        }
        return;
    }
    {
        uint32_t s = js_to_string(vm, init);
        if (vm->ctl) return;
        list_parse(vm, list, JSTR(vm, s)->s, (int)JSTR(vm, s)->len);
    }
}

static uint32_t usp_new(js_vm_t *vm) {
    uint32_t o = js_obj_new(vm, JS_CLASS_OBJECT, g_p_usp), l = js_array_new(vm, 4);
    if (!o || !l) return 0;
    hset(vm, o, "__l", jv_obj(l));
    return o;
}

static void url_sync_query(js_vm_t *vm, uint32_t url, uint32_t list);
static void usp_sync_in(js_vm_t *vm, jv thisv, uint32_t list);

/* the list of a URLSearchParams (want 0) or FormData (want 1), synced
 * with its URL first; 0 after a TypeError */
static uint32_t list_of(js_vm_t *vm, jv thisv, int want) {
    jv l;
    if (!is_of(vm, thisv, want ? g_p_fd : g_p_usp)) {
        js_throw_err(vm, JE_TYPE, want ? "Illegal invocation: not a FormData" : "Illegal invocation: not a URLSearchParams");
        return 0;
    }
    l = hget(vm, thisv, "__l");
    if (!jv_is_obj(l)) { js_throw_err(vm, JE_TYPE, "Illegal invocation"); return 0; }
    if (!want) usp_sync_in(vm, thisv, l.a);
    return l.a;
}

/* magic & 0xff: 0 append, 1 delete, 2 get, 3 getAll, 4 has, 5 set,
 * 6 sort, 7 toString, 8 forEach, 9 keys, 10 values, 11 entries;
 * magic & 0x100: FormData */
static jv pairs_fn(js_vm_t *vm, jv thisv, jv *argv, int argc, uint32_t magic) {
    int fd = (magic & 0x100) != 0, op = (int)(magic & 0xff), changed = 0;
    uint32_t list = list_of(vm, thisv, fd), name = 0, i, n;
    if (!list) return jv_undef();
    n = JOBJ(vm, list)->alen;
    if (op <= 5) {
        if (argc < (op == 0 || op == 5 ? 2 : 1)) {
            static const char *const nm[] = { "append", "delete", "get", "getAll", "has", "set" };
            js_sb_t m;
            js_sb_init(vm, &m);
            js_sb_put(vm, &m, "Failed to execute '", 19);
            js_sb_put(vm, &m, nm[op], alen(nm[op]));
            js_sb_put(vm, &m, fd ? "' on 'FormData': " : "' on 'URLSearchParams': ", fd ? 17 : 24);
            js_sb_put(vm, &m, op == 0 || op == 5 ? "2 arguments required" : "1 argument required", op == 0 || op == 5 ? 20 : 19);
            js_sb_put(vm, &m, ", but only ", 11);
            js_sb_ch(vm, &m, (char)('0' + argc));
            js_sb_put(vm, &m, " present.", 9);
            return js_throw(vm, jv_obj(js_error_new(vm, JE_TYPE, js_sb_finish(vm, &m))));
        }
        name = js_to_string(vm, argv[0]);
        JCK(vm);
    }
    switch (op) {
    case 0: {
        uint32_t v = js_to_string(vm, argv[1]);
        JCK(vm);
        list_push(vm, list, name, v);
        changed = 1;
        break;
    }
    case 1: {
        int has_v = argc > 1 && argv[1].t != JT_UNDEF;
        uint32_t v = has_v ? js_to_string(vm, argv[1]) : 0;
        JCK(vm);
        for (i = n; i-- > 0;)
            if (js_str_eq(vm, pair_name(vm, list, i), name) && (!has_v || js_str_eq(vm, pair_value(vm, list, i), v))) {
                list_remove(vm, list, i);
                changed = 1;
            }
        break;
    }
    case 2:
        for (i = 0; i < n; i++) if (js_str_eq(vm, pair_name(vm, list, i), name)) return jv_str(pair_value(vm, list, i));
        return jv_null();
    case 3: {
        uint32_t a = js_array_new(vm, 2);
        if (!a) return jv_undef();
        for (i = 0; i < n; i++) if (js_str_eq(vm, pair_name(vm, list, i), name)) js_array_push(vm, a, jv_str(pair_value(vm, list, i)));
        return jv_obj(a);
    }
    case 4: {
        int has_v = argc > 1 && argv[1].t != JT_UNDEF;
        uint32_t v = has_v ? js_to_string(vm, argv[1]) : 0;
        JCK(vm);
        for (i = 0; i < n; i++)
            if (js_str_eq(vm, pair_name(vm, list, i), name) && (!has_v || js_str_eq(vm, pair_value(vm, list, i), v))) return jv_bool(1);
        return jv_bool(0);
    }
    case 5: {
        uint32_t v = js_to_string(vm, argv[1]);
        int found = 0;
        JCK(vm);
        for (i = 0; i < JOBJ(vm, list)->alen;) {
            if (js_str_eq(vm, pair_name(vm, list, i), name)) {
                if (!found) { js_array_set(vm, js_array_get(vm, list, i).a, 1, jv_str(v)); found = 1; i++; }
                else list_remove(vm, list, i);
            } else i++;
        }
        if (!found) list_push(vm, list, name, v);
        changed = 1;
        break;
    }
    case 6: {      /* a stable sort on the names (insertion sort keeps equal names in order) */
        uint32_t k;
        for (i = 1; i < n; i++) {
            jv cur = js_array_get(vm, list, i);
            uint32_t cn = js_array_get(vm, cur.a, 0).a;
            k = i;
            while (k > 0 && js_str_cmp(vm, pair_name(vm, list, k - 1), cn) > 0) {
                js_array_set(vm, list, k, js_array_get(vm, list, k - 1));
                k--;
            }
            js_array_set(vm, list, k, cur);
        }
        changed = 1;
        break;
    }
    case 7: return jv_str(list_serialize(vm, list));
    case 8: {
        jv fn = JARG(0);
        if (!js_is_callable(vm, fn)) return js_throw_err(vm, JE_TYPE, "forEach: parameter 1 is not of type 'Function'");
        for (i = 0; i < JOBJ(vm, list)->alen && !vm->ctl; i++) {
            jv a[3];
            a[0] = jv_str(pair_value(vm, list, i));
            a[1] = jv_str(pair_name(vm, list, i));
            a[2] = thisv;
            js_call(vm, fn, JARG(1), a, 3);
        }
        return jv_undef();
    }
    case 9: case 10: case 11: {
        uint32_t a = js_array_new(vm, n ? n : 1);
        if (!a) return jv_undef();
        for (i = 0; i < n; i++) {
            if (op == 9) js_array_push(vm, a, jv_str(pair_name(vm, list, i)));
            else if (op == 10) js_array_push(vm, a, jv_str(pair_value(vm, list, i)));
            else {
                uint32_t p = pair_new(vm, pair_name(vm, list, i), pair_value(vm, list, i));
                if (p) js_array_push(vm, a, jv_obj(p));
            }
        }
        return jv_obj(a);
    }
    default: return jv_undef();
    }
    if (changed && !fd) {
        jv u = hget(vm, thisv, "__u");
        if (jv_is_obj(u)) url_sync_query(vm, u.a, list);
    }
    return jv_undef();
}

static jv usp_size(js_vm_t *vm, jv thisv, jv *argv, int argc, uint32_t magic) {
    uint32_t list = list_of(vm, thisv, (int)magic);
    (void)argv; (void)argc;
    if (!list) return jv_undef();
    return jv_int((int32_t)JOBJ(vm, list)->alen);
}

static jv usp_ctor(js_vm_t *vm, jv thisv, jv *argv, int argc, uint32_t magic) {
    uint32_t o;
    (void)thisv; (void)magic;
    if (!vm->in_new) return js_throw_err(vm, JE_TYPE, "Failed to construct 'URLSearchParams': Please use the 'new' operator");
    o = usp_new(vm);
    if (!o) return jv_undef();
    list_fill(vm, hget(vm, jv_obj(o), "__l").a, JARG(0), "URLSearchParams");
    JCK(vm);
    return jv_obj(o);
}

static jv fd_ctor(js_vm_t *vm, jv thisv, jv *argv, int argc, uint32_t magic) {
    uint32_t o, l;
    (void)thisv; (void)magic;
    if (!vm->in_new) return js_throw_err(vm, JE_TYPE, "Failed to construct 'FormData': Please use the 'new' operator");
    o = js_obj_new(vm, JS_CLASS_OBJECT, g_p_fd);
    l = js_array_new(vm, 4);
    if (!o || !l) return jv_undef();
    hset(vm, o, "__l", jv_obj(l));
    if (argc > 0 && argv[0].t != JT_UNDEF) {
        if (wf_jsdom_form_pairs(vm, argv[0], l) != 0) {
            JCK(vm);
            return js_throw_err(vm, JE_TYPE, "Failed to construct 'FormData': parameter 1 is not of type 'HTMLFormElement'.");
        }
        JCK(vm);
    }
    return jv_obj(o);
}

/* multipart/form-data with the fixed boundary (names and values are
 * strings: no filename parts) */
static uint32_t fd_multipart(js_vm_t *vm, uint32_t list) {
    js_sb_t b;
    uint32_t i, n = JOBJ(vm, list)->alen, k;
    const char *bd = WF_JSAPI_BOUNDARY;
    js_sb_init(vm, &b);
    for (i = 0; i < n; i++) {
        uint32_t nm = pair_name(vm, list, i);
        js_sb_put(vm, &b, "--", 2);
        js_sb_put(vm, &b, bd, alen(bd));
        js_sb_put(vm, &b, "\r\nContent-Disposition: form-data; name=\"", 40);
        for (k = 0; k < JSTR(vm, nm)->len; k++) {     /* the name escaped as browsers do */
            char c = JSTR(vm, nm)->s[k];
            if (c == '"') js_sb_put(vm, &b, "%22", 3);
            else if (c == '\r') js_sb_put(vm, &b, "%0D", 3);
            else if (c == '\n') js_sb_put(vm, &b, "%0A", 3);
            else js_sb_ch(vm, &b, c);
        }
        js_sb_put(vm, &b, "\"\r\n\r\n", 5);
        js_sb_str(vm, &b, pair_value(vm, list, i));
        js_sb_put(vm, &b, "\r\n", 2);
    }
    js_sb_put(vm, &b, "--", 2);
    js_sb_put(vm, &b, bd, alen(bd));
    js_sb_put(vm, &b, "--\r\n", 4);
    return js_sb_finish(vm, &b);
}

int js_api_body(js_vm_t *vm, jv body, uint32_t *str, const char **ctype) {
    static char ct[96];
    jv l;
    if (is_of(vm, body, g_p_usp)) {
        l = hget(vm, body, "__l");
        if (!jv_is_obj(l)) return 0;
        usp_sync_in(vm, body, l.a);
        *str = list_serialize(vm, l.a);
        *ctype = "application/x-www-form-urlencoded;charset=UTF-8";
        return 1;
    }
    if (is_of(vm, body, g_p_fd)) {
        const char *p = "multipart/form-data; boundary=", *bd = WF_JSAPI_BOUNDARY;
        int i = 0, j;
        l = hget(vm, body, "__l");
        if (!jv_is_obj(l)) return 0;
        *str = fd_multipart(vm, l.a);
        for (j = 0; p[j]; j++) ct[i++] = p[j];
        for (j = 0; bd[j]; j++) ct[i++] = bd[j];
        ct[i] = 0;
        *ctype = ct;
        return 1;
    }
    return 0;
}

/* ---- URL: a WHATWG parser for the common cases ---- */

typedef struct {
    char scheme[24];            /* lowercased, without ':' */
    char user[128], pass[128];
    char host[256];
    char port[8];               /* "" for none or the default port */
    char path[WF_JSAPI_URL_MAX];
    char query[WF_JSAPI_URL_MAX];
    char frag[WF_JSAPI_URL_MAX];
    int has_host, has_query, has_frag, opaque, special;
} jurl_t;

static jurl_t g_u, g_base;

static int special_scheme(const char *s) {
    static const char *const sp[] = { "http", "https", "ws", "wss", "ftp", "file", 0 };
    int i, j;
    for (i = 0; sp[i]; i++) {
        for (j = 0; sp[i][j] && sp[i][j] == s[j]; j++) {}
        if (!sp[i][j] && !s[j]) return 1;
    }
    return 0;
}

static const char *default_port(const char *s) {
    if (s[0] == 'h' && s[1] == 't' && s[2] == 't' && s[3] == 'p' && !s[4]) return "80";
    if (s[0] == 'h' && s[1] == 't' && s[2] == 't' && s[3] == 'p' && s[4] == 's' && !s[5]) return "443";
    if (s[0] == 'w' && s[1] == 's' && !s[2]) return "80";
    if (s[0] == 'w' && s[1] == 's' && s[2] == 's' && !s[3]) return "443";
    if (s[0] == 'f' && s[1] == 't' && s[2] == 'p' && !s[3]) return "21";
    return 0;
}

static int s_same(const char *a, const char *b) {
    while (*a && *a == *b) { a++; b++; }
    return *a == *b;
}

static void s_put(char *d, int cap, const char *s, int n) {
    int i;
    if (n > cap - 1) n = cap - 1;
    for (i = 0; i < n; i++) d[i] = s[i];
    d[n] = 0;
}

/* append s[0..n) to d, percent-encoding the bytes of the given set:
 * 0 fragment, 1 query, 2 special query, 3 path, 4 userinfo, 5 opaque
 * path (C0 controls and bytes over 0x7e). -1 when d is full. */
static int enc_put(char *d, int cap, const char *s, int n, int set) {
    static const char hex[] = "0123456789ABCDEF";
    int o = alen(d), i;
    for (i = 0; i < n; i++) {
        unsigned char c = (unsigned char)s[i];
        int e = c < 0x20 || c > 0x7e;
        if (!e) {
            switch (set) {
            case 0: e = c == ' ' || c == '"' || c == '<' || c == '>' || c == '`'; break;
            case 1: e = c == ' ' || c == '"' || c == '#' || c == '<' || c == '>'; break;
            case 2: e = c == ' ' || c == '"' || c == '#' || c == '<' || c == '>' || c == '\''; break;
            case 3: e = c == ' ' || c == '"' || c == '#' || c == '<' || c == '>' || c == '?' || c == '`' || c == '{' || c == '}'; break;
            case 4: e = c == ' ' || c == '"' || c == '#' || c == '<' || c == '>' || c == '?' || c == '`' || c == '{' ||
                        c == '}' || c == '/' || c == ':' || c == ';' || c == '=' || c == '@' || c == '[' || c == '\\' ||
                        c == ']' || c == '^' || c == '|';
                break;
            default: e = 0; break;
            }
        }
        if (e) {
            if (o + 3 >= cap) return -1;
            d[o++] = '%'; d[o++] = hex[c >> 4]; d[o++] = hex[c & 15];
        } else {
            if (o + 1 >= cap) return -1;
            d[o++] = (char)c;
        }
    }
    d[o] = 0;
    return 0;
}

static int dot_seg(const char *s, int n, int two) {
    if (!two) return (n == 1 && s[0] == '.') || (n == 3 && s[0] == '%' && s[1] == '2' && (s[2] | 32) == 'e');
    if (n == 2) return s[0] == '.' && s[1] == '.';
    if (n == 4) return (s[0] == '.' && s[1] == '%' && s[2] == '2' && (s[3] | 32) == 'e') ||
                       (s[0] == '%' && s[1] == '2' && (s[2] | 32) == 'e' && s[3] == '.');
    return n == 6 && s[0] == '%' && s[1] == '2' && (s[2] | 32) == 'e' && s[3] == '%' && s[4] == '2' && (s[5] | 32) == 'e';
}

/* the path of u from p (already absolute or merged): segments with "."
 * and ".." resolved, each one encoded */
static int path_set(jurl_t *u, const char *p, int n) {
    static char out[WF_JSAPI_URL_MAX];
    int i = 0, o, last_dot = 0;
    out[0] = 0;
    if (n > 0 && (p[0] == '/' || (u->special && p[0] == '\\'))) i = 1;
    while (i <= n) {
        int e = i;
        while (e < n && p[e] != '/' && !(u->special && p[e] == '\\')) e++;
        last_dot = 0;
        if (dot_seg(p + i, e - i, 1)) {
            o = alen(out);
            while (o > 0 && out[o - 1] != '/') o--;
            if (o > 0) o--;
            out[o] = 0;
            last_dot = 1;
        } else if (dot_seg(p + i, e - i, 0)) {
            last_dot = 1;
        } else {
            o = alen(out);
            if (o + 1 >= (int)sizeof(out)) return -1;
            out[o] = '/'; out[o + 1] = 0;
            if (enc_put(out, (int)sizeof(out), p + i, e - i, 3) != 0) return -1;
        }
        if (e >= n) break;
        i = e + 1;
    }
    if (last_dot) {          /* "a/.." and "a/." end with a slash */
        o = alen(out);
        if (o + 1 < (int)sizeof(out)) { out[o] = '/'; out[o + 1] = 0; }
    }
    if (!out[0]) { out[0] = '/'; out[1] = 0; }
    if (!u->special && n == 0) out[0] = 0;     /* "foo://h" keeps an empty path */
    s_put(u->path, (int)sizeof(u->path), out, alen(out));
    return 0;
}

static void lower_into(char *d, int cap, const char *s, int n) {
    int i;
    if (n > cap - 1) n = cap - 1;
    for (i = 0; i < n; i++) d[i] = (s[i] >= 'A' && s[i] <= 'Z') ? (char)(s[i] + 32) : s[i];
    d[n] = 0;
}

/* host[:port] with userinfo before an '@'; 0 or -1 */
static int auth_set(jurl_t *u, const char *a, int n) {
    int at = -1, i, colon = -1, hs, bracket = 0;
    for (i = 0; i < n; i++) if (a[i] == '@') at = i;
    u->user[0] = u->pass[0] = 0;
    if (at >= 0) {
        int c = -1;
        for (i = 0; i < at; i++) if (a[i] == ':') { c = i; break; }
        if (enc_put(u->user, (int)sizeof(u->user), a, c < 0 ? at : c, 4) != 0) return -1;
        if (c >= 0 && enc_put(u->pass, (int)sizeof(u->pass), a + c + 1, at - c - 1, 4) != 0) return -1;
    }
    hs = at + 1;
    for (i = hs; i < n; i++) {
        if (a[i] == '[') bracket = 1;
        else if (a[i] == ']') bracket = 0;
        else if (a[i] == ':' && !bracket) colon = i;
    }
    {
        int he = colon >= 0 ? colon : n;
        for (i = hs; i < he; i++) {
            unsigned char c = (unsigned char)a[i];
            if (c <= ' ' || c == '#' || c == '%' || c == '/' || c == '<' || c == '>' || c == '?' || c == '@' ||
                c == '\\' || c == '^' || c == '|' || c == 0x7f || c == '[' || c == ']')   /* no IPv6 */
                if (!(c == '%' && !u->special)) return -1;      /* forbidden host code points */
        }
        if (he - hs >= (int)sizeof(u->host)) return -1;
        if (u->special) lower_into(u->host, (int)sizeof(u->host), a + hs, he - hs);
        else s_put(u->host, (int)sizeof(u->host), a + hs, he - hs);
        if (u->special && !u->host[0] && !s_same(u->scheme, "file")) return -1;
    }
    u->port[0] = 0;
    if (colon >= 0 && colon + 1 < n) {
        long v = 0;
        const char *dp;
        for (i = colon + 1; i < n; i++) {
            if (a[i] < '0' || a[i] > '9') return -1;
            v = v * 10 + (a[i] - '0');
            if (v > 65535) return -1;
        }
        {
            char b[8];
            int k = 0;
            char t[8];
            long w = v;
            do { t[k++] = (char)('0' + w % 10); w /= 10; } while (w && k < 7);
            for (i = 0; i < k; i++) b[i] = t[k - 1 - i];
            b[k] = 0;
            dp = default_port(u->scheme);
            if (!dp || !s_same(dp, b)) s_put(u->port, (int)sizeof(u->port), b, k);
        }
    }
    u->has_host = 1;
    return 0;
}

/* the query and fragment of s (from '?' or '#'); returns the end of the
 * path part */
static int tail_set(jurl_t *u, const char *s, int n) {
    int q = -1, h = -1, i;
    for (i = 0; i < n; i++) {
        if (s[i] == '#') { h = i; break; }
        if (s[i] == '?' && q < 0) q = i;
    }
    u->has_query = q >= 0;
    u->has_frag = h >= 0;
    u->query[0] = u->frag[0] = 0;
    if (q >= 0) (void)enc_put(u->query, (int)sizeof(u->query), s + q + 1, (h >= 0 ? h : n) - q - 1, u->special ? 2 : 1);
    if (h >= 0) (void)enc_put(u->frag, (int)sizeof(u->frag), s + h + 1, n - h - 1, 0);
    return q >= 0 ? q : h >= 0 ? h : n;
}

static void url_clear(jurl_t *u) {
    u->scheme[0] = u->user[0] = u->pass[0] = u->host[0] = u->port[0] = 0;
    u->path[0] = u->query[0] = u->frag[0] = 0;
    u->has_host = u->has_query = u->has_frag = u->opaque = u->special = 0;
}

static void url_copy(jurl_t *d, const jurl_t *s) {
    int i;
    for (i = 0; i < (int)sizeof(*d); i++) ((char *)d)[i] = ((const char *)s)[i];
}

/* parse in (n bytes) against base (0: none) into u; 0 or -1 */
static int url_parse(const char *in, int n, const jurl_t *base, jurl_t *u) {
    static char buf[WF_JSAPI_URL_MAX];
    int i, k = 0, sc = 0, pe;
    const char *s;
    /* strip leading and trailing C0 controls and spaces, drop tabs and newlines */
    while (n > 0 && (unsigned char)in[0] <= ' ') { in++; n--; }
    while (n > 0 && (unsigned char)in[n - 1] <= ' ') n--;
    for (i = 0; i < n; i++) {
        if (in[i] == '\t' || in[i] == '\n' || in[i] == '\r') continue;
        if (k >= (int)sizeof(buf) - 1) return -1;
        buf[k++] = in[i];
    }
    buf[k] = 0;
    s = buf;
    n = k;
    url_clear(u);
    if (n > 0 && ((s[0] | 32) >= 'a' && (s[0] | 32) <= 'z')) {
        for (i = 1; i < n; i++) {
            char c = s[i];
            if (c == ':') { sc = i; break; }
            if (!((c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') || (c >= '0' && c <= '9') || c == '+' || c == '-' || c == '.')) break;
        }
    }
    if (sc > 0 && sc < (int)sizeof(u->scheme)) {
        const char *r = s + sc + 1;
        int rn = n - sc - 1;
        lower_into(u->scheme, (int)sizeof(u->scheme), s, sc);
        u->special = special_scheme(u->scheme);
        if (u->special) {
            int a, e;
            if (base && s_same(base->scheme, u->scheme) && !(rn > 0 && (r[0] == '/' || r[0] == '\\'))) {
                /* "http:path" against an http base: a relative reference */
                s = r;
                n = rn;
                goto relative;
            }
            a = 0;
            while (a < rn && (r[a] == '/' || r[a] == '\\')) a++;
            if (s_same(u->scheme, "file")) {
                /* file: an empty host unless "//host/" */
                if (a == 2) {
                    e = a;
                    while (e < rn && r[e] != '/' && r[e] != '\\' && r[e] != '?' && r[e] != '#') e++;
                    if (auth_set(u, r + a, e - a) != 0) return -1;
                    if (u->host[0] && s_same(u->host, "localhost")) u->host[0] = 0;
                } else { e = a > 0 ? a - 1 : 0; u->has_host = 1; }
                pe = tail_set(u, r + e, rn - e);
                return path_set(u, r + e, pe);
            }
            e = a;
            while (e < rn && r[e] != '/' && r[e] != '\\' && r[e] != '?' && r[e] != '#') e++;
            if (auth_set(u, r + a, e - a) != 0) return -1;
            pe = tail_set(u, r + e, rn - e);
            return path_set(u, r + e, pe);
        }
        if (rn >= 2 && r[0] == '/' && r[1] == '/') {
            int e = 2;
            while (e < rn && r[e] != '/' && r[e] != '?' && r[e] != '#') e++;
            if (auth_set(u, r + 2, e - 2) != 0) return -1;
            pe = tail_set(u, r + e, rn - e);
            return path_set(u, r + e, pe);
        }
        if (rn > 0 && r[0] == '/') {
            pe = tail_set(u, r, rn);
            return path_set(u, r, pe);
        }
        /* an opaque path (mailto:, data:, javascript:...) */
        u->opaque = 1;
        pe = tail_set(u, r, rn);
        u->path[0] = 0;
        return enc_put(u->path, (int)sizeof(u->path), r, pe, 5);
    }
    if (!base) return -1;
    if (base->opaque) {
        if (n == 0 || s[0] != '#') return -1;
        url_copy(u, base);
        u->has_frag = 1;
        u->frag[0] = 0;
        return enc_put(u->frag, (int)sizeof(u->frag), s + 1, n - 1, 0);
    }
    s_put(u->scheme, (int)sizeof(u->scheme), base->scheme, alen(base->scheme));
    u->special = base->special;
relative:
    if (n >= 2 && (s[0] == '/' || (u->special && s[0] == '\\')) && (s[1] == '/' || (u->special && s[1] == '\\'))) {
        int e = 2;
        if (s_same(u->scheme, "file")) {
            while (e < n && s[e] != '/' && s[e] != '\\' && s[e] != '?' && s[e] != '#') e++;
            if (auth_set(u, s + 2, e - 2) != 0) return -1;
        } else {
            while (e < n && s[e] != '/' && !(u->special && s[e] == '\\') && s[e] != '?' && s[e] != '#') e++;
            if (auth_set(u, s + 2, e - 2) != 0) return -1;
        }
        pe = tail_set(u, s + e, n - e);
        return path_set(u, s + e, pe);
    }
    s_put(u->user, (int)sizeof(u->user), base->user, alen(base->user));
    s_put(u->pass, (int)sizeof(u->pass), base->pass, alen(base->pass));
    s_put(u->host, (int)sizeof(u->host), base->host, alen(base->host));
    s_put(u->port, (int)sizeof(u->port), base->port, alen(base->port));
    u->has_host = base->has_host;
    if (n > 0 && (s[0] == '/' || (u->special && s[0] == '\\'))) {
        pe = tail_set(u, s, n);
        return path_set(u, s, pe);
    }
    if (n == 0 || s[0] == '?' || s[0] == '#') {
        s_put(u->path, (int)sizeof(u->path), base->path, alen(base->path));
        if (n == 0) {
            u->has_query = base->has_query;
            s_put(u->query, (int)sizeof(u->query), base->query, alen(base->query));
            return 0;
        }
        if (s[0] == '#') {
            u->has_query = base->has_query;
            s_put(u->query, (int)sizeof(u->query), base->query, alen(base->query));
            u->has_frag = 1;
            u->frag[0] = 0;
            return enc_put(u->frag, (int)sizeof(u->frag), s + 1, n - 1, 0);
        }
        (void)tail_set(u, s, n);
        return 0;
    }
    {   /* merge: the base path up to its last slash, then the reference */
        static char m[WF_JSAPI_URL_MAX];
        int bl = alen(base->path), j;
        while (bl > 0 && base->path[bl - 1] != '/') bl--;
        pe = tail_set(u, s, n);
        if (bl + pe >= (int)sizeof(m)) return -1;
        for (j = 0; j < bl; j++) m[j] = base->path[j];
        if (bl == 0) m[bl++] = '/';
        for (j = 0; j < pe; j++) m[bl + j] = s[j];
        return path_set(u, m, bl + pe);
    }
}

/* the href of u in d */
static int url_format(const jurl_t *u, char *d, int cap) {
    int o = 0;
#define PUT(str) do { const char *p_ = (str); while (*p_) { if (o + 1 >= cap) return -1; d[o++] = *p_++; } } while (0)
    PUT(u->scheme);
    PUT(":");
    if (u->has_host) {
        PUT("//");
        if (u->user[0] || u->pass[0]) {
            PUT(u->user);
            if (u->pass[0]) { PUT(":"); PUT(u->pass); }
            PUT("@");
        }
        PUT(u->host);
        if (u->port[0]) { PUT(":"); PUT(u->port); }
    } else if (!u->opaque && u->path[0] == '/' && u->path[1] == '/') PUT("/.");
    PUT(u->path);
    if (u->has_query) { PUT("?"); PUT(u->query); }
    if (u->has_frag) { PUT("#"); PUT(u->frag); }
#undef PUT
    d[o] = 0;
    return o;
}

/* the parsed href of a URL object into g_u; 0, or -1 after a TypeError */
static int url_load(js_vm_t *vm, jv thisv) {
    static char b[WF_JSAPI_URL_MAX];
    jv h;
    if (!is_of(vm, thisv, g_p_url)) { js_throw_err(vm, JE_TYPE, "Illegal invocation: not a URL"); return -1; }
    h = hget(vm, thisv, "__href");
    if (h.t != JT_STR) { js_throw_err(vm, JE_TYPE, "Illegal invocation: not a URL"); return -1; }
    cstr(vm, h.a, b, (int)sizeof(b));
    if (url_parse(b, alen(b), 0, &g_u) != 0) { js_throw_err(vm, JE_TYPE, "Invalid URL"); return -1; }
    return 0;
}

static void url_store(js_vm_t *vm, uint32_t o, const jurl_t *u) {
    static char b[WF_JSAPI_URL_MAX * 2];
    if (url_format(u, b, (int)sizeof(b)) < 0) return;
    hset(vm, o, "__href", jv_str(js_str_c(vm, b)));
}

/* a URLSearchParams changed: the URL takes its serialization as query */
static void url_sync_query(js_vm_t *vm, uint32_t url, uint32_t list) {
    static char q[WF_JSAPI_URL_MAX];
    uint32_t s;
    if (url_load(vm, jv_obj(url)) != 0) { vm->ctl = CTL_NONE; return; }
    s = list_serialize(vm, list);
    cstr(vm, s, q, (int)sizeof(q));
    g_u.has_query = q[0] != 0;
    s_put(g_u.query, (int)sizeof(g_u.query), q, alen(q));
    url_store(vm, url, &g_u);
}

/* before a URLSearchParams of a URL is read: its list from the query */
static void usp_sync_in(js_vm_t *vm, jv thisv, uint32_t list) {
    jv u = hget(vm, thisv, "__u");
    if (!jv_is_obj(u) || url_load(vm, u) != 0) { if (vm->ctl == CTL_THROW) vm->ctl = CTL_NONE; return; }
    js_array_set_len(vm, list, 0);
    list_parse(vm, list, g_u.query, alen(g_u.query));
}

static uint32_t c_cat(js_vm_t *vm, const char *a, const char *b) {
    js_sb_t s;
    js_sb_init(vm, &s);
    js_sb_put(vm, &s, a, alen(a));
    js_sb_put(vm, &s, b, alen(b));
    return js_sb_finish(vm, &s);
}

/* getters: 0 href, 1 protocol, 2 username, 3 password, 4 host,
 * 5 hostname, 6 port, 7 pathname, 8 search, 9 hash, 10 origin,
 * 11 searchParams */
static jv url_get(js_vm_t *vm, jv thisv, jv *argv, int argc, uint32_t magic) {
    static char b[WF_JSAPI_URL_MAX * 2];
    (void)argv; (void)argc;
    if (magic == 11) {
        jv sp = hget(vm, thisv, "__sp");
        uint32_t o;
        if (url_load(vm, thisv) != 0) return jv_undef();
        if (jv_is_obj(sp)) return sp;
        o = usp_new(vm);
        if (!o) return jv_undef();
        hset(vm, o, "__u", thisv);
        hset(vm, thisv.a, "__sp", jv_obj(o));
        return jv_obj(o);
    }
    if (url_load(vm, thisv) != 0) return jv_undef();
    switch (magic) {
    case 0: if (url_format(&g_u, b, (int)sizeof(b)) < 0) return jv_str(vm->s_empty); return jv_str(js_str_c(vm, b));
    case 1: return jv_str(c_cat(vm, g_u.scheme, ":"));
    case 2: return jv_str(js_str_c(vm, g_u.user));
    case 3: return jv_str(js_str_c(vm, g_u.pass));
    case 4:
        if (!g_u.port[0]) return jv_str(js_str_c(vm, g_u.host));
        {
            js_sb_t s;
            js_sb_init(vm, &s);
            js_sb_put(vm, &s, g_u.host, alen(g_u.host));
            js_sb_ch(vm, &s, ':');
            js_sb_put(vm, &s, g_u.port, alen(g_u.port));
            return jv_str(js_sb_finish(vm, &s));
        }
    case 5: return jv_str(js_str_c(vm, g_u.host));
    case 6: return jv_str(js_str_c(vm, g_u.port));
    case 7: return jv_str(js_str_c(vm, g_u.path));
    case 8: return jv_str(g_u.has_query && g_u.query[0] ? c_cat(vm, "?", g_u.query) : vm->s_empty);
    case 9: return jv_str(g_u.has_frag && g_u.frag[0] ? c_cat(vm, "#", g_u.frag) : vm->s_empty);
    case 10:
        if (!g_u.special || s_same(g_u.scheme, "file")) return jv_str(js_str_c(vm, "null"));
        {
            js_sb_t s;
            js_sb_init(vm, &s);
            js_sb_put(vm, &s, g_u.scheme, alen(g_u.scheme));
            js_sb_put(vm, &s, "://", 3);
            js_sb_put(vm, &s, g_u.host, alen(g_u.host));
            if (g_u.port[0]) { js_sb_ch(vm, &s, ':'); js_sb_put(vm, &s, g_u.port, alen(g_u.port)); }
            return jv_str(js_sb_finish(vm, &s));
        }
    default: return jv_undef();
    }
}

/* setters, same numbers (origin and searchParams have none) */
static jv url_set(js_vm_t *vm, jv thisv, jv *argv, int argc, uint32_t magic) {
    static char v[WF_JSAPI_URL_MAX], t[WF_JSAPI_URL_MAX * 2];
    uint32_t vs = js_to_string(vm, JARG(0));
    int n, i;
    (void)argc;
    JCK(vm);
    n = cstr(vm, vs, v, (int)sizeof(v));
    if (magic == 0) {
        if (url_parse(v, n, 0, &g_u) != 0) {
            js_sb_t m;
            js_sb_init(vm, &m);
            js_sb_put(vm, &m, "Failed to set the 'href' property on 'URL': Invalid URL '", 57);
            js_sb_str(vm, &m, vs);
            js_sb_ch(vm, &m, '\'');
            return js_throw(vm, jv_obj(js_error_new(vm, JE_TYPE, js_sb_finish(vm, &m))));
        }
        url_store(vm, thisv.a, &g_u);
        return jv_undef();
    }
    if (url_load(vm, thisv) != 0) return jv_undef();
    switch (magic) {
    case 1: {           /* the scheme, when special stays special */
        char sc[24];
        int k = 0, sp;
        while (k < n && k < (int)sizeof(sc) - 1 && v[k] != ':') { sc[k] = (v[k] >= 'A' && v[k] <= 'Z') ? (char)(v[k] + 32) : v[k]; k++; }
        sc[k] = 0;
        if (!k || !((sc[0] >= 'a' && sc[0] <= 'z'))) return jv_undef();
        sp = special_scheme(sc);
        if (sp != g_u.special || (s_same(sc, "file") && (g_u.user[0] || g_u.port[0]))) return jv_undef();
        s_put(g_u.scheme, (int)sizeof(g_u.scheme), sc, k);
        {
            const char *dp = default_port(sc);
            if (dp && s_same(dp, g_u.port)) g_u.port[0] = 0;
        }
        break;
    }
    case 2: case 3:
        if (!g_u.has_host || !g_u.host[0] || s_same(g_u.scheme, "file")) return jv_undef();
        if (magic == 2) { g_u.user[0] = 0; (void)enc_put(g_u.user, (int)sizeof(g_u.user), v, n, 4); }
        else { g_u.pass[0] = 0; (void)enc_put(g_u.pass, (int)sizeof(g_u.pass), v, n, 4); }
        break;
    case 4: case 5: {
        static jurl_t h;
        int e = 0;
        if (g_u.opaque) return jv_undef();
        while (e < n && v[e] != '/' && v[e] != '?' && v[e] != '#' && !(g_u.special && v[e] == '\\')) e++;
        if (magic == 5) for (i = 0; i < e; i++) if (v[i] == ':') { e = i; break; }
        url_copy(&h, &g_u);
        if (auth_set(&h, v, e) != 0) return jv_undef();
        s_put(g_u.host, (int)sizeof(g_u.host), h.host, alen(h.host));
        if (magic == 4 && h.port[0]) s_put(g_u.port, (int)sizeof(g_u.port), h.port, alen(h.port));
        g_u.has_host = 1;
        break;
    }
    case 6: {
        long pv = 0;
        int k = 0;
        const char *dp;
        if (!g_u.has_host || !g_u.host[0] || s_same(g_u.scheme, "file")) return jv_undef();
        if (n == 0) { g_u.port[0] = 0; break; }
        while (k < n && v[k] >= '0' && v[k] <= '9') { pv = pv * 10 + (v[k] - '0'); if (pv > 65535) return jv_undef(); k++; }
        if (k == 0) return jv_undef();
        {
            char b[8], r[8];
            int q = 0, j;
            long w = pv;
            do { r[q++] = (char)('0' + w % 10); w /= 10; } while (w && q < 7);
            for (j = 0; j < q; j++) b[j] = r[q - 1 - j];
            b[q] = 0;
            dp = default_port(g_u.scheme);
            if (dp && s_same(dp, b)) g_u.port[0] = 0;
            else s_put(g_u.port, (int)sizeof(g_u.port), b, q);
        }
        break;
    }
    case 7:
        if (g_u.opaque) return jv_undef();
        {
            static char p2[WF_JSAPI_URL_MAX];
            int k = 0;
            if (n == 0 || (v[0] != '/' && !(g_u.special && v[0] == '\\'))) p2[k++] = '/';
            for (i = 0; i < n && k < (int)sizeof(p2) - 1; i++) p2[k++] = v[i];
            p2[k] = 0;
            if (path_set(&g_u, p2, k) != 0) return jv_undef();
        }
        break;
    case 8: {
        const char *q = v;
        int qn = n;
        if (qn > 0 && q[0] == '?') { q++; qn--; }
        g_u.query[0] = 0;
        g_u.has_query = qn > 0;
        if (qn > 0) (void)enc_put(g_u.query, (int)sizeof(g_u.query), q, qn, g_u.special ? 2 : 1);
        break;
    }
    case 9: {
        const char *q = v;
        int qn = n;
        if (qn > 0 && q[0] == '#') { q++; qn--; }
        g_u.frag[0] = 0;
        g_u.has_frag = qn > 0;
        if (qn > 0) (void)enc_put(g_u.frag, (int)sizeof(g_u.frag), q, qn, 0);
        break;
    }
    default: return jv_undef();
    }
    if (url_format(&g_u, t, (int)sizeof(t)) < 0) return jv_undef();
    hset(vm, thisv.a, "__href", jv_str(js_str_c(vm, t)));
    return jv_undef();
}

/* 0 toString / toJSON */
static jv url_fn(js_vm_t *vm, jv thisv, jv *argv, int argc, uint32_t magic) {
    (void)magic;
    return url_get(vm, thisv, argv, argc, 0);
}

/* new URL(url, base) and URL.canParse (magic 1): 0 and g_u, or -1 */
static int url_from_args(js_vm_t *vm, jv *argv, int argc) {
    static char a[WF_JSAPI_URL_MAX], b[WF_JSAPI_URL_MAX];
    uint32_t s = js_to_string(vm, JARG(0));
    int an;
    if (vm->ctl) return -1;
    an = cstr(vm, s, a, (int)sizeof(a));
    if (argc > 1 && argv[1].t != JT_UNDEF) {
        uint32_t bs;
        if (is_of(vm, argv[1], g_p_url)) bs = hget(vm, argv[1], "__href").a;
        else bs = js_to_string(vm, argv[1]);
        if (vm->ctl) return -1;
        cstr(vm, bs, b, (int)sizeof(b));
        if (url_parse(b, alen(b), 0, &g_base) != 0) return -1;
        return url_parse(a, an, &g_base, &g_u);
    }
    return url_parse(a, an, 0, &g_u);
}

static jv url_ctor(js_vm_t *vm, jv thisv, jv *argv, int argc, uint32_t magic) {
    uint32_t o;
    (void)thisv;
    if (magic == 1) {
        int r = url_from_args(vm, argv, argc);
        JCK(vm);
        return jv_bool(r == 0);
    }
    if (!vm->in_new) return js_throw_err(vm, JE_TYPE, "Failed to construct 'URL': Please use the 'new' operator");
    if (url_from_args(vm, argv, argc) != 0) {
        js_sb_t m;
        JCK(vm);
        js_sb_init(vm, &m);
        js_sb_put(vm, &m, "Failed to construct 'URL': Invalid URL", 38);
        return js_throw(vm, jv_obj(js_error_new(vm, JE_TYPE, js_sb_finish(vm, &m))));
    }
    o = js_obj_new(vm, JS_CLASS_OBJECT, g_p_url);
    if (!o) return jv_undef();
    url_store(vm, o, &g_u);
    return jv_obj(o);
}

/* ---- localStorage / sessionStorage: a static store in the shell ----
 * An item belongs to an area (origin slot * 2 + 1 for session); its key
 * and value bytes sit in g_sp (compacted when a write does not fit).
 * Items keep their insertion order (key(i)). */

typedef struct { uint16_t off, kl, vl; uint8_t area, pad; } st_item_t;
static st_item_t g_si[WF_JSAPI_STORE_ITEMS];
static int g_nsi, g_spu;
static char g_sp[WF_JSAPI_STORE_BYTES], g_sp_tmp[WF_JSAPI_STORE_BYTES];
static char g_sorg[WF_JSAPI_STORE_ORIGINS][96];
static uint32_t g_sage[WF_JSAPI_STORE_ORIGINS], g_sclock;
static int g_slot = -1;     /* the origin slot of the page */

void js_api_store_reset(void) {
    int i;
    g_nsi = g_spu = 0;
    for (i = 0; i < WF_JSAPI_STORE_ORIGINS; i++) { g_sorg[i][0] = 0; g_sage[i] = 0; }
    g_sclock = 0;
    g_slot = -1;
}

static void store_drop_area(int area) {
    int i, k = 0;
    for (i = 0; i < g_nsi; i++) if (g_si[i].area != area) g_si[k++] = g_si[i];
    g_nsi = k;
}

/* the slot of origin o (the oldest one is reused for a new origin) */
static int store_slot(const char *o) {
    int i, old = 0;
    for (i = 0; i < WF_JSAPI_STORE_ORIGINS; i++) {
        if (g_sorg[i][0] && s_same(g_sorg[i], o)) { g_sage[i] = ++g_sclock; return i; }
    }
    for (i = 0; i < WF_JSAPI_STORE_ORIGINS; i++) {
        if (!g_sorg[i][0]) { old = i; break; }
        if (g_sage[i] < g_sage[old]) old = i;
    }
    store_drop_area(old * 2);
    store_drop_area(old * 2 + 1);
    s_put(g_sorg[old], (int)sizeof(g_sorg[old]), o, alen(o));
    g_sage[old] = ++g_sclock;
    return old;
}

static void store_compact(void) {
    int i, o = 0, k;
    for (i = 0; i < g_nsi; i++) {
        int n = g_si[i].kl + g_si[i].vl;
        for (k = 0; k < n; k++) g_sp_tmp[o + k] = g_sp[g_si[i].off + k];
        g_si[i].off = (uint16_t)o;
        o += n;
    }
    for (k = 0; k < o; k++) g_sp[k] = g_sp_tmp[k];
    g_spu = o;
}

static int store_find(int area, const char *k, int kl) {
    int i, j;
    for (i = 0; i < g_nsi; i++) {
        if (g_si[i].area != area || g_si[i].kl != kl) continue;
        for (j = 0; j < kl && g_sp[g_si[i].off + j] == k[j]; j++) {}
        if (j == kl) return i;
    }
    return -1;
}

/* 0 stored, -1 over the quota */
static int store_set(int area, const char *k, int kl, const char *v, int vl) {
    int i = store_find(area, k, kl), j, at;
    if (kl + vl > WF_JSAPI_STORE_BYTES) return -1;
    if (i >= 0 && vl <= g_si[i].vl) {                  /* in place */
        for (j = 0; j < vl; j++) g_sp[g_si[i].off + kl + j] = v[j];
        g_si[i].vl = (uint16_t)vl;
        return 0;
    }
    if (i < 0 && g_nsi >= WF_JSAPI_STORE_ITEMS) return -1;
    if (g_spu + kl + vl > WF_JSAPI_STORE_BYTES) {
        int live = 0;
        if (i >= 0) g_si[i].kl = g_si[i].vl = 0;       /* its old bytes go too */
        store_compact();
        for (j = 0; j < g_nsi; j++) live += g_si[j].kl + g_si[j].vl;
        if (live + kl + vl > WF_JSAPI_STORE_BYTES) {
            if (i >= 0) { for (j = i; j + 1 < g_nsi; j++) g_si[j] = g_si[j + 1]; g_nsi--; }
            return -1;
        }
    }
    at = g_spu;
    for (j = 0; j < kl; j++) g_sp[at + j] = k[j];
    for (j = 0; j < vl; j++) g_sp[at + kl + j] = v[j];
    g_spu += kl + vl;
    if (i < 0) { i = g_nsi++; g_si[i].area = (uint8_t)area; }
    g_si[i].off = (uint16_t)at;
    g_si[i].kl = (uint16_t)kl;
    g_si[i].vl = (uint16_t)vl;
    return 0;
}

static void store_remove(int i) {
    int j;
    for (j = i; j + 1 < g_nsi; j++) g_si[j] = g_si[j + 1];
    g_nsi--;
}

static int storage_area(js_vm_t *vm, jv thisv) {
    jv a = hget(vm, thisv, "__a");
    if (!is_of(vm, thisv, g_p_storage) || a.t != JT_INT) { js_throw_err(vm, JE_TYPE, "Illegal invocation: not a Storage"); return -1; }
    if (g_slot < 0) { js_throw(vm, jv_obj(domexc_c(vm, "Storage is disabled inside this page (no origin).", "SecurityError"))); return -1; }
    return g_slot * 2 + (int)a.a;
}

/* 0 getItem, 1 setItem, 2 removeItem, 3 clear, 4 key, 5 length */
static jv storage_fn(js_vm_t *vm, jv thisv, jv *argv, int argc, uint32_t magic) {
    static char kb[WF_JSAPI_STORE_BYTES];
    int area = storage_area(vm, thisv), i, kl, n;
    uint32_t ks;
    if (area < 0) return jv_undef();
    if (magic == 3) { store_drop_area(area); return jv_undef(); }
    if (magic == 5) {
        for (i = 0, n = 0; i < g_nsi; i++) if (g_si[i].area == area) n++;
        return jv_int(n);
    }
    if (argc < (magic == 1 ? 2 : 1)) {
        static const char *const nm[] = { "getItem", "setItem", "removeItem", "", "key" };
        js_sb_t m;
        js_sb_init(vm, &m);
        js_sb_put(vm, &m, "Failed to execute '", 19);
        js_sb_put(vm, &m, nm[magic], alen(nm[magic]));
        js_sb_put(vm, &m, "' on 'Storage': ", 16);
        js_sb_put(vm, &m, magic == 1 ? "2 arguments required" : "1 argument required", magic == 1 ? 20 : 19);
        js_sb_put(vm, &m, ", but only ", 11);
        js_sb_ch(vm, &m, (char)('0' + argc));
        js_sb_put(vm, &m, " present.", 9);
        return js_throw(vm, jv_obj(js_error_new(vm, JE_TYPE, js_sb_finish(vm, &m))));
    }
    if (magic == 4) {
        int want = js_to_int32(vm, argv[0]);
        JCK(vm);
        for (i = 0, n = 0; i < g_nsi; i++) {
            if (g_si[i].area != area) continue;
            if (n++ == want) return jv_str(js_str_new(vm, g_sp + g_si[i].off, g_si[i].kl));
        }
        return jv_null();
    }
    ks = js_to_string(vm, argv[0]);
    JCK(vm);
    kl = cstr(vm, ks, kb, (int)sizeof(kb));
    i = store_find(area, kb, kl);
    if (magic == 0) return i < 0 ? jv_null() : jv_str(js_str_new(vm, g_sp + g_si[i].off + g_si[i].kl, g_si[i].vl));
    if (magic == 2) { if (i >= 0) store_remove(i); return jv_undef(); }
    {
        uint32_t vs = js_to_string(vm, argv[1]);
        JCK(vm);
        if ((int)JSTR(vm, vs)->len > WF_JSAPI_STORE_BYTES || store_set(area, kb, kl, JSTR(vm, vs)->s, (int)JSTR(vm, vs)->len) != 0) {
            js_sb_t m;
            js_sb_init(vm, &m);
            js_sb_put(vm, &m, "Failed to execute 'setItem' on 'Storage': Setting the value of '", 64);
            js_sb_str(vm, &m, ks);
            js_sb_put(vm, &m, "' exceeded the quota.", 21);
            return js_throw(vm, jv_obj(domexc_new(vm, js_sb_finish(vm, &m), js_str_c(vm, "QuotaExceededError"))));
        }
    }
    return jv_undef();
}

/* ---- AbortController / AbortSignal ---- */

static uint32_t signal_new(js_vm_t *vm) {
    uint32_t s = js_obj_new(vm, JS_CLASS_OBJECT, g_p_signal), l = js_array_new(vm, 2);
    if (!s || !l) return 0;
    js_define_c(vm, s, "aborted", jv_bool(0), P_DEFAULT);
    js_define_c(vm, s, "reason", jv_undef(), P_DEFAULT);
    js_define_c(vm, s, "onabort", jv_null(), P_DEFAULT);
    hset(vm, s, "__ls", jv_obj(l));
    return s;
}

static void signal_abort(js_vm_t *vm, uint32_t s, jv reason) {
    jv l, on;
    uint32_t ev, i, n;
    if (js_truthy(vm, js_get_c(vm, jv_obj(s), "aborted"))) return;
    if (reason.t == JT_UNDEF) {
        uint32_t e = domexc_c(vm, "signal is aborted without reason", "AbortError");
        reason = e ? jv_obj(e) : jv_undef();
    }
    js_put(vm, jv_obj(s), akey(vm, "aborted"), jv_bool(1));
    js_put(vm, jv_obj(s), akey(vm, "reason"), reason);
    ev = js_obj_new(vm, JS_CLASS_OBJECT, vm->p_object);
    if (!ev) return;
    js_define_c(vm, ev, "type", jv_str(js_str_c(vm, "abort")), P_DEFAULT);
    js_define_c(vm, ev, "target", jv_obj(s), P_DEFAULT);
    js_define_c(vm, ev, "currentTarget", jv_obj(s), P_DEFAULT);
    l = hget(vm, jv_obj(s), "__ls");
    n = jv_is_obj(l) ? JOBJ(vm, l.a)->alen : 0;
    for (i = 0; i < n && !vm->ctl; i++) {
        jv f = js_array_get(vm, l.a, i), a = jv_obj(ev);
        if (js_is_callable(vm, f)) js_call(vm, f, jv_obj(s), &a, 1);
        else if (jv_is_obj(f)) {
            jv h = js_get_c(vm, f, "handleEvent");
            if (!vm->ctl && js_is_callable(vm, h)) js_call(vm, h, f, &a, 1);
        }
    }
    on = js_get_c(vm, jv_obj(s), "onabort");      /* the handler after the listeners */
    if (!vm->ctl && js_is_callable(vm, on)) {
        jv a = jv_obj(ev);
        js_call(vm, on, jv_obj(s), &a, 1);
    }
}

int js_api_aborted(js_vm_t *vm, jv signal, jv *reason) {
    if (!is_of(vm, signal, g_p_signal)) return 0;
    if (!js_truthy(vm, js_get_c(vm, signal, "aborted"))) return 0;
    *reason = js_get_c(vm, signal, "reason");
    return 1;
}

/* 0 addEventListener, 1 removeEventListener, 2 throwIfAborted */
static jv signal_fn(js_vm_t *vm, jv thisv, jv *argv, int argc, uint32_t magic) {
    jv l;
    if (!is_of(vm, thisv, g_p_signal)) return js_throw_err(vm, JE_TYPE, "Illegal invocation: not an AbortSignal");
    if (magic == 2) {
        if (js_truthy(vm, js_get_c(vm, thisv, "aborted"))) return js_throw(vm, js_get_c(vm, thisv, "reason"));
        return jv_undef();
    }
    l = hget(vm, thisv, "__ls");
    if (!jv_is_obj(l)) return jv_undef();
    {
        uint32_t t = js_to_string(vm, JARG(0)), i, n = JOBJ(vm, l.a)->alen;
        jv f = JARG(1);
        JCK(vm);
        if (JSTR(vm, t)->len != 5 || JSTR(vm, t)->s[0] != 'a' || JSTR(vm, t)->s[1] != 'b') return jv_undef();   /* only "abort" */
        if (!jv_is_obj(f)) return jv_undef();
        for (i = 0; i < n; i++) {
            if (js_strict_eq(vm, js_array_get(vm, l.a, i), f)) {
                if (magic == 1) list_remove(vm, l.a, i);
                return jv_undef();
            }
        }
        if (magic == 0) js_array_push(vm, l.a, f);
    }
    (void)argc;
    return jv_undef();
}

/* AbortController (magic 0), its abort (1), AbortSignal() (2: refused),
 * AbortSignal.abort (3), AbortSignal.any (4) */
/* Storage and AbortSignal have no public constructor; js_ctor_new gives a
 * native magic 0, so the refusal needs its own function */
static jv illegal_ctor(js_vm_t *vm, jv thisv, jv *argv, int argc, uint32_t magic) {
    (void)thisv; (void)argv; (void)argc; (void)magic;
    return js_throw_err(vm, JE_TYPE, "Illegal constructor");
}

static jv abort_fn(js_vm_t *vm, jv thisv, jv *argv, int argc, uint32_t magic) {
    uint32_t s;
    switch (magic) {
    case 0: {
        uint32_t c;
        if (!vm->in_new) return js_throw_err(vm, JE_TYPE, "Failed to construct 'AbortController': Please use the 'new' operator");
        c = js_obj_new(vm, JS_CLASS_OBJECT, g_p_ctrl);
        s = signal_new(vm);
        if (!c || !s) return jv_undef();
        js_define_c(vm, c, "signal", jv_obj(s), P_DEFAULT);
        return jv_obj(c);
    }
    case 1: {
        jv sg;
        if (!is_of(vm, thisv, g_p_ctrl)) return js_throw_err(vm, JE_TYPE, "Illegal invocation: not an AbortController");
        sg = js_get_c(vm, thisv, "signal");
        JCK(vm);
        if (is_of(vm, sg, g_p_signal)) signal_abort(vm, sg.a, JARG(0));
        return jv_undef();
    }
    case 3:
        s = signal_new(vm);
        if (!s) return jv_undef();
        signal_abort(vm, s, JARG(0));
        return jv_obj(s);
    case 4: {       /* aborted when one of them is already aborted (later aborts are not followed) */
        uint32_t i, n;
        jv list = JARG(0), r;
        s = signal_new(vm);
        if (!s) return jv_undef();
        if (!jv_is_obj(list) || !js_is_array(vm, list)) return jv_obj(s);
        n = JOBJ(vm, list.a)->alen;
        for (i = 0; i < n; i++) {
            if (js_api_aborted(vm, js_array_get(vm, list.a, i), &r)) { signal_abort(vm, s, r); break; }
        }
        return jv_obj(s);
    }
    default: return jv_undef();
    }
}

/* ---- setup ---- */

static int accessor(js_vm_t *vm, uint32_t proto, const char *name, js_native_fn get, js_native_fn set, uint32_t magic) {
    uint32_t g = js_native_new(vm, name, get, 0, magic), s = set ? js_native_new(vm, name, set, 1, magic) : 0;
    if (!g || (set && !s)) return -1;
    return js_define_accessor(vm, proto, akey(vm, name), g, s, P_CONF) ? 0 : -1;   /* 1 on success */
}

int js_api_init(js_vm_t *vm, const char *page_url) {
    static const char *const pn[] = { "append", "delete", "get", "getAll", "has", "set", "sort", "toString", "forEach",
                                      "keys", "values", "entries", 0 };
    static const uint8_t pa[] = { 2, 1, 1, 1, 1, 2, 0, 0, 1, 0, 0, 0 };
    static const char *const ug[] = { "href", "protocol", "username", "password", "host", "hostname", "port", "pathname",
                                      "search", "hash", "origin", "searchParams", 0 };
    static const char *const sn[] = { "getItem", "setItem", "removeItem", "clear", "key", 0 };
    static const uint8_t sa[] = { 1, 2, 1, 0, 1 };
    uint32_t c, ls, ss;
    int i;
    g_p_usp = js_obj_new(vm, JS_CLASS_OBJECT, vm->p_object);
    g_p_fd = js_obj_new(vm, JS_CLASS_OBJECT, vm->p_object);
    g_p_url = js_obj_new(vm, JS_CLASS_OBJECT, vm->p_object);
    g_p_storage = js_obj_new(vm, JS_CLASS_OBJECT, vm->p_object);
    g_p_signal = js_obj_new(vm, JS_CLASS_OBJECT, vm->p_object);
    g_p_ctrl = js_obj_new(vm, JS_CLASS_OBJECT, vm->p_object);
    g_p_domexc = js_obj_new(vm, JS_CLASS_OBJECT, vm->p_err[JE_ERROR]);
    if (!g_p_usp || !g_p_fd || !g_p_url || !g_p_storage || !g_p_signal || !g_p_ctrl || !g_p_domexc) return -1;
    for (i = 0; pn[i]; i++) {
        js_def_native(vm, g_p_usp, pn[i], pairs_fn, pa[i], (uint32_t)i);
        if (i != 6 && i != 7) js_def_native(vm, g_p_fd, pn[i], pairs_fn, pa[i], 0x100u | (uint32_t)i);
    }
    if (accessor(vm, g_p_usp, "size", usp_size, 0, 0) != 0) return -1;
    if (!js_ctor_new(vm, "URLSearchParams", usp_ctor, 0, g_p_usp)) return -1;
    if (!js_ctor_new(vm, "FormData", fd_ctor, 0, g_p_fd)) return -1;
    for (i = 0; ug[i]; i++)
        if (accessor(vm, g_p_url, ug[i], url_get, i == 10 || i == 11 ? 0 : url_set, (uint32_t)i) != 0) return -1;
    js_def_native(vm, g_p_url, "toString", url_fn, 0, 0);
    js_def_native(vm, g_p_url, "toJSON", url_fn, 0, 0);
    c = js_ctor_new(vm, "URL", url_ctor, 1, g_p_url);
    if (!c) return -1;
    js_def_native(vm, c, "canParse", url_ctor, 1, 1);
    for (i = 0; sn[i]; i++) js_def_native(vm, g_p_storage, sn[i], storage_fn, sa[i], (uint32_t)i);
    if (accessor(vm, g_p_storage, "length", storage_fn, 0, 5) != 0) return -1;
    if (!js_ctor_new(vm, "Storage", illegal_ctor, 0, g_p_storage)) return -1;    /* new Storage() is refused */
    ls = js_obj_new(vm, JS_CLASS_OBJECT, g_p_storage);
    ss = js_obj_new(vm, JS_CLASS_OBJECT, g_p_storage);
    if (!ls || !ss) return -1;
    hset(vm, ls, "__a", jv_int(0));
    hset(vm, ss, "__a", jv_int(1));
    js_define_c(vm, vm->global, "localStorage", jv_obj(ls), P_HIDDEN);
    js_define_c(vm, vm->global, "sessionStorage", jv_obj(ss), P_HIDDEN);
    js_def_native(vm, g_p_signal, "addEventListener", signal_fn, 2, 0);
    js_def_native(vm, g_p_signal, "removeEventListener", signal_fn, 2, 1);
    js_def_native(vm, g_p_signal, "throwIfAborted", signal_fn, 0, 2);
    js_def_native(vm, g_p_ctrl, "abort", abort_fn, 0, 1);
    if (!js_ctor_new(vm, "AbortController", abort_fn, 0, g_p_ctrl)) return -1;
    c = js_ctor_new(vm, "AbortSignal", illegal_ctor, 0, g_p_signal);
    if (!c) return -1;
    js_def_native(vm, c, "abort", abort_fn, 0, 3);
    js_def_native(vm, c, "any", abort_fn, 1, 4);
    js_define_c(vm, g_p_domexc, "name", jv_str(js_str_c(vm, "Error")), P_HIDDEN);
    if (!js_ctor_new(vm, "DOMException", domexc_ctor, 0, g_p_domexc)) return -1;
    /* the storage origin: scheme, host and port of the page */
    g_slot = -1;
    if (page_url && url_parse(page_url, alen(page_url), 0, &g_u) == 0 && g_u.special && !s_same(g_u.scheme, "file")) {
        static char o[96];
        o[0] = 0;
        s_put(o, (int)sizeof(o), g_u.scheme, alen(g_u.scheme));
        if (alen(o) + 3 + alen(g_u.host) + 7 < (int)sizeof(o)) {
            int k = alen(o), j;
            o[k++] = ':'; o[k++] = '/'; o[k++] = '/';
            for (j = 0; g_u.host[j]; j++) o[k++] = g_u.host[j];
            if (g_u.port[0]) { o[k++] = ':'; for (j = 0; g_u.port[j]; j++) o[k++] = g_u.port[j]; }
            o[k] = 0;
            g_slot = store_slot(o);
        }
    }
    return vm->ctl ? -1 : 0;
}

/* ---- for wf_jshist.c (history, structuredClone) ---- */

uint32_t js_api_domexc(js_vm_t *vm, const char *msg, const char *name) {
    return domexc_c(vm, msg, name);
}

int js_api_resolve(const char *rel, const char *base, char *out, int cap) {
    if (!rel || !base || !out || cap < 2) return -1;
    if (url_parse(base, alen(base), 0, &g_base) != 0) return -1;
    if (url_parse(rel, alen(rel), &g_base, &g_u) != 0) return -1;
    return url_format(&g_u, out, cap) < 0 ? -1 : 0;
}

int js_api_same_document(const char *a, const char *b) {
    if (!a || !b) return 0;
    if (url_parse(a, alen(a), 0, &g_base) != 0 || url_parse(b, alen(b), 0, &g_u) != 0) return 0;
    if (g_base.opaque || g_u.opaque) return 0;          /* mailto:, data: ... have no paths to push */
    return s_same(g_base.scheme, g_u.scheme) && s_same(g_base.user, g_u.user) && s_same(g_base.pass, g_u.pass) &&
           s_same(g_base.host, g_u.host) && s_same(g_base.port, g_u.port) && g_base.has_host == g_u.has_host;
}

int js_api_platform(js_vm_t *vm, jv v) {
    uint32_t p;
    if (!jv_is_obj(v)) return 0;
    p = JOBJ(vm, v.a)->proto;
    return p && (p == g_p_usp || p == g_p_fd || p == g_p_url || p == g_p_storage || p == g_p_signal || p == g_p_ctrl);
}
