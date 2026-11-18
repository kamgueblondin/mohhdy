/* Lot 115 (US-031): Proxy in the JavaScript subset (wf_jsproxy.c), on
 * the host. The engine's [[Get]] (also through a proxy on the prototype
 * chain), [[Set]], [[HasProperty]], [[Delete]], own key lists, [[Call]],
 * [[Construct]], prototype and descriptor paths call the handler's traps
 * or forward to the target; Proxy.revocable revokes. The cases compare
 * the console lines joined with '|' with what node v24 prints for the
 * same source (expectations produced by node), including the patterns
 * page code uses Proxy for (a reactive store, a validator, negative
 * array indexes, a method-call recorder). The last tests check the
 * collector keeps a proxy's target and handler, gives them back after a
 * revoke, and that a long chain of proxies ends in a RangeError. */
#include "../../framework/unity.h"
#include <string.h>
#include <stdio.h>
#include "wf_jsi.h"

#define ARENA (3u * 1024u * 1024u)
static unsigned char g_mem[ARENA];
static js_vm_t g_vm;
static char g_out[4096];

static void fresh(void) { TEST_ASSERT_EQUAL(0, js_init(&g_vm, g_mem, ARENA)); }

static const char *run(const char *src) {
    int rc, n, i, k = 0;
    const char *log;
    js_log_clear(&g_vm);
    js_set_budget(&g_vm, 3000000u);
    rc = js_eval(&g_vm, src, (int)strlen(src), 0);
    if (rc == JS_OK) {
        js_set_budget(&g_vm, 3000000u);
        rc = js_run_jobs(&g_vm);
    }
    log = js_log_text(&g_vm, &n);
    while (n > 0 && log[n - 1] == '\n') n--;
    for (i = 0; i < n && k < (int)sizeof(g_out) - 1; i++) g_out[k++] = log[i] == '\n' ? '|' : log[i];
    g_out[k] = 0;
    if (rc != JS_OK) snprintf(g_out + k, sizeof(g_out) - (size_t)k, "!%s", js_last_error(&g_vm));
    return g_out;
}

typedef struct { const char *src, *want; } acase_t;

static void run_cases(const acase_t *c) {
    int i;
    for (i = 0; c[i].src; i++) {
        const char *got;
        fresh();
        got = run(c[i].src);
        if (strcmp(got, c[i].want) != 0) printf("  case %d: %s\n    got  %s\n    want %s\n", i, c[i].src, got, c[i].want);
        TEST_ASSERT_EQUAL_STRING(c[i].want, got);
    }
}

static const acase_t k_traps[] = {
    { "const log = []; const store = new Proxy({ count: 0, name: 'a' }, { get(t, k, r) { log.push('get ' + String(k)); return Reflect.get(t, k, r); }, set(t, k, v) { log.push('set ' + k + '=' + v); t[k] = v; return true; }, has(t, k) { return k.startsWith('_') ? false : k in t; }, deleteProperty(t, k) { log.push('del ' + k); return delete t[k]; } }); store.count++; store.name = 'b'; console.log(store.count, store.name, 'count' in store, '_x' in store, delete store.name, store.name); console.log(log.join(','))",
      "1 b true false true undefined|get count,set count=1,set name=b,get count,get name,del name,get name" },
    { "const dflt = new Proxy({}, { get: (t, k) => k in t ? t[k] : 'none' }); dflt.a = 1; console.log(dflt.a, dflt.b, JSON.stringify(dflt))",
      "1 none {\"a\":1}" },
    { "const hidden = new Proxy({ a: 1, _p: 2, b: 3 }, { ownKeys: t => Reflect.ownKeys(t).filter(k => !k.startsWith('_')) }); const r = []; for (const k in hidden) r.push(k); console.log(Object.keys(hidden).join(), JSON.stringify(hidden), Object.getOwnPropertyNames(hidden).join(), Object.entries(hidden).length, r.join())",
      "a,b {\"a\":1,\"b\":3} a,b 2 a,b" },
    { "const fn = new Proxy(function (a, b) { return a + b; }, { apply(t, th, args) { return t(...args) * 10; } }); console.log(fn(1, 2), typeof fn, fn.call(null, 3, 4), fn.apply(null, [1, 1]))",
      "30 function 70 20" },
    { "class Pt { constructor(x) { this.x = x; } } const P2 = new Proxy(Pt, { construct(t, args, nt) { return new t(args[0] * 2); } }); const plain = new Proxy(Pt, {}); console.log(new P2(5).x, new P2(1) instanceof Pt, typeof P2, new plain(3).x, new plain(3) instanceof Pt, plain.name)",
      "10 true function 3 true Pt" },
    { "const proto = new Proxy({}, { get: (t, k, r) => k === 'hello' ? 'from proto ' + (r === child) : undefined }); const child = Object.create(proto); child.own = 1; console.log(child.hello, child.own)",
      "from proto true 1" },
    { "const desc = new Proxy({ a: 1 }, { getOwnPropertyDescriptor(t, k) { return { value: 42, writable: true, enumerable: true, configurable: true }; } }); console.log(Object.getOwnPropertyDescriptor(desc, 'a').value, Reflect.getOwnPropertyDescriptor(desc, 'zz').value, Object.prototype.hasOwnProperty.call(desc, 'q'))",
      "42 42 true" },
    { "const def = new Proxy({}, { defineProperty(t, k, d) { return k !== 'no' && Reflect.defineProperty(t, k, d); } }); console.log(Reflect.defineProperty(def, 'yes', { value: 1 }), Reflect.defineProperty(def, 'no', { value: 1 }), def.yes); try { Object.defineProperty(def, 'no', { value: 1 }) } catch (e) { console.log(e.constructor.name) }",
      "true false 1|TypeError" },
    { "const gp = new Proxy({}, { getPrototypeOf: () => Array.prototype, setPrototypeOf: () => false }); console.log(Object.getPrototypeOf(gp) === Array.prototype, Reflect.setPrototypeOf(gp, null), Reflect.getPrototypeOf(new Proxy([], {})) === Array.prototype)",
      "true false true" },
    { "const ro = new Proxy({ a: 1 }, { set: () => false }); ro.a = 2; console.log(ro.a, Reflect.set(ro, 'a', 3))",
      "1 false" },
    { "const counter = new Proxy(function () {}, { apply: (t, th, a) => a.length, construct: (t, a) => ({ made: a.join('-') }) }); console.log(counter(1, 2, 3), new counter('a', 'b').made)",
      "3 a-b" },
    { 0, 0 }
};

static const acase_t k_forward[] = {
    { "const arr = new Proxy([1, 2, 3], {}); console.log(Array.isArray(arr), arr.length, arr[1], arr.map(x => x * 2).join(), [...arr].join(), JSON.stringify(arr))",
      "true 3 2 2,4,6 1,2,3 [1,2,3]" },
    { "const nested = new Proxy(new Proxy({ v: 1 }, {}), {}); nested.v = 5; console.log(nested.v, Object.keys(nested).join())",
      "5 v" },
    { "const sym = Symbol('s'); const sp = new Proxy({ [sym]: 1, x: 2 }, {}); console.log(sp[sym], Object.getOwnPropertySymbols(sp).length, Reflect.ownKeys(sp).length, ({ ...sp }).x, Object.assign({}, new Proxy({ q: 1 }, {})).q, Object.prototype.hasOwnProperty.call(new Proxy({ h: 1 }, {}), 'h'))",
      "1 1 2 2 1 true" },
    { "console.log(new Proxy({ a: 1, b: [2] }, {}), new Proxy([1, 2], {}))",
      "{ a: 1, b: [ 2 ] } [ 1, 2 ]" },
    { "const watched = []; const obs = (o) => new Proxy(o, { set(t, k, v) { watched.push(k); return Reflect.set(t, k, v); } }); const st = obs({ items: [] }); st.items = [1]; st.total = 3; st.total += 1; console.log(watched.join(), st.total, Reflect.set(st, 'z', 1), st.z)",
      "items,total,total 4 true 1" },
    { 0, 0 }
};

static const acase_t k_pages[] = {
    { "function reactive(obj, onChange) { return new Proxy(obj, { get(t, k, r) { const v = Reflect.get(t, k, r); return v && typeof v === 'object' ? reactive(v, onChange) : v; }, set(t, k, v, r) { const old = t[k]; const ok = Reflect.set(t, k, v, r); if (old !== v) onChange(k, v); return ok; }, deleteProperty(t, k) { onChange(k, undefined); return Reflect.deleteProperty(t, k); } }); } const changes = []; const state = reactive({ user: { name: 'ana' }, todos: [] }, (k, v) => changes.push(String(k) + ':' + JSON.stringify(v))); state.user.name = 'ben'; state.todos.push('x'); state.todos.push('y'); state.count = 1; state.count = 1; delete state.count; console.log(changes.join(' '), JSON.stringify(state))",
      "name:\"ben\" 0:\"x\" 1:\"y\" count:1 count:undefined {\"user\":{\"name\":\"ben\"},\"todos\":[\"x\",\"y\"]}" },
    { "const validator = new Proxy({}, { set(t, k, v) { if (k === 'age' && !Number.isInteger(v)) throw new TypeError('age must be an integer'); t[k] = v; return true; } }); validator.age = 30; try { validator.age = 'x'; } catch (e) { console.log(e.message, validator.age); }",
      "age must be an integer 30" },
    { "const neg = new Proxy([10, 20, 30], { get(t, k, r) { const i = Number(k); return Number.isInteger(i) && i < 0 ? t[t.length + i] : Reflect.get(t, k, r); } }); console.log(neg[-1], neg[0], neg.length, neg.slice(1).join())",
      "30 10 3 20,30" },
    { "const calls = []; const api = new Proxy({}, { get: (t, name) => (...args) => { calls.push(name + '(' + args.join() + ')'); return name.length; } }); console.log(api.fetchUser(1), api.save('a', 'b'), calls.join(' '))",
      "9 4 fetchUser(1) save(a,b)" },
    { 0, 0 }
};

static const acase_t k_errors[] = {
    { "const { proxy, revoke } = Proxy.revocable({ a: 1 }, {}); console.log(proxy.a); revoke(); revoke(); for (const f of [() => proxy.a, () => { proxy.a = 1 }, () => 'a' in proxy, () => delete proxy.a, () => Object.keys(proxy), () => Array.isArray(proxy)]) try { f(); console.log('no throw') } catch (e) { console.log(e.constructor.name, e.message) }",
      "1|TypeError Cannot perform 'get' on a proxy that has been revoked|TypeError Cannot perform 'set' on a proxy that has been revoked|TypeError Cannot perform 'has' on a proxy that has been revoked|TypeError Cannot perform 'deleteProperty' on a proxy that has been revoked|TypeError Cannot perform 'ownKeys' on a proxy that has been revoked|TypeError Cannot perform 'IsArray' on a proxy that has been revoked" },
    { "for (const f of [() => new Proxy(1, {}), () => new Proxy({}, null), () => Proxy({}, {}), () => new (new Proxy(() => 1, {}))(), () => new Proxy({}, { get: 1 }).a, () => new Proxy({}, {})(), () => new Proxy({}, { ownKeys: () => [1] }), () => Object.keys(new Proxy({}, { ownKeys: () => 5 }))]) try { f(); console.log('no throw') } catch (e) { console.log(e.constructor.name) }",
      "TypeError|TypeError|TypeError|TypeError|TypeError|TypeError|no throw|TypeError" },
    { "console.log(new Proxy({}, {}) instanceof Object, typeof Proxy, Proxy.length, typeof Proxy.revocable, Proxy.prototype, Object.keys(Proxy.revocable({}, {})).join())",
      "true function 2 function undefined proxy,revoke" },
    { "for (const m of [() => new Proxy(1, {}), () => Proxy({}, {})]) try { m() } catch (e) { console.log(e.message) }",
      "Cannot create proxy with a non-object as target or handler|Constructor Proxy requires 'new'" },
    { 0, 0 }
};

void test_traps(void) { run_cases(k_traps); }
void test_forwarding(void) { run_cases(k_forward); }
void test_page_patterns(void) { run_cases(k_pages); }
void test_errors(void) { run_cases(k_errors); }

/* run, then collect; a broken heap walk shows in the output */
static const char *run_gc(const char *src) {
    const char *r = run(src);
    int ck;
    js_gc(&g_vm);
    ck = js_gc_check(&g_vm);
    if (ck != 0) snprintf(g_out + strlen(g_out), sizeof(g_out) - strlen(g_out), "!heap %d", ck);
    return r;
}

/* a proxy keeps its target and handler alive; once revoked they go */
void test_gc_proxy(void) {
    fresh();
    js_gc_config(&g_vm, 0, 1, 0);
    TEST_ASSERT_EQUAL_STRING("", run_gc("var rv = Proxy.revocable({ big: 'x'.repeat(3000) }, { get: (t, k) => t[k] + '!' });"
                                        " var wt = new WeakRef(rv.proxy), keep = rv.proxy; 0;"));
    TEST_ASSERT_EQUAL_STRING("3001", run_gc("console.log(keep.big.length); 0;"));
    run_gc("rv.revoke(); 0;");
    TEST_ASSERT_EQUAL_STRING("TypeError", run_gc("try { keep.big } catch (e) { console.log(e.constructor.name) } 0;"));
    TEST_ASSERT_EQUAL_STRING("true", run_gc("console.log(wt.deref() === keep); keep = null; rv = null; 0;"));
    TEST_ASSERT_EQUAL_STRING("undefined", run_gc("console.log(wt.deref()); 0;"));
}

/* 4000 proxies without traps in a chain: the forwarding is bounded by
 * the call depth (a RangeError, no crash); 20 still forward */
void test_chain_bound(void) {
    fresh();
    TEST_ASSERT_EQUAL_STRING("RangeError|20",
        run("let p = { v: 20 }; for (let i = 0; i < 4000; i++) p = new Proxy(p, {}); try { p.v } catch (e) { console.log(e.constructor.name) }"
            " let q = { v: 20 }; for (let i = 0; i < 20; i++) q = new Proxy(q, {}); console.log(q.v)"));
}

int main(void) {
    unity_init();
    RUN_TEST(test_traps);
    RUN_TEST(test_forwarding);
    RUN_TEST(test_page_patterns);
    RUN_TEST(test_errors);
    RUN_TEST(test_gc_proxy);
    RUN_TEST(test_chain_bound);
    unity_print_results();
    unity_cleanup();
    return unity_stats.tests_failed == 0 ? 0 : 1;
}
