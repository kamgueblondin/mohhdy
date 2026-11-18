/* Lot 115 (US-031): weak collections and Reflect in the JavaScript
 * subset (wf_jsweak.c), on the host. WeakMap, WeakSet, WeakRef and
 * FinalizationRegistry keep their entries in a JK_WEAK table that the
 * collector (wf_jsgc.c) does not trace: a value lives while its key does
 * (to a fixed point) and an entry whose key died is emptied before the
 * sweep. Reflect works on the same object internals as Object, and
 * Object.defineProperty / setPrototypeOf now refuse what the language
 * refuses (a frozen or non-extensible object, a non-configurable
 * property). The cases compare the console lines joined with '|' with
 * what node v24 prints for the same source (expectations produced by
 * node). The collector tests check reclaim through WeakRef.deref, the
 * ephemeron chain, the table bound and the table limit per run. */
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

static const acase_t k_weakmap[] = {
    { "const a = {}, b = {}, s = Symbol('s'); const wm = new WeakMap([[a, 1]]); wm.set(b, 2).set(s, 3); console.log(wm.get(a), wm.get(b), wm.get(s), wm.has(a), wm.has({}), wm.get(1), wm.has(1))",
      "1 2 3 true false undefined false" },
    { "const a = {}; const wm = new WeakMap(); wm.set(a, 1); wm.set(a, 2); console.log(wm.get(a), wm.delete(a), wm.has(a), wm.delete(a), wm.get(a))",
      "2 true false false undefined" },
    { "const ws = new WeakSet(), a = {}, b = {}; ws.add(a); console.log(ws.has(a), ws.add(b) === ws, ws.has(b), ws.delete(b), ws.has(b), ws.has(1), ws.delete(1))",
      "true true true true false false false" },
    { "const m = new WeakMap(), keep = []; for (let i = 0; i < 300; i++) { const k = { i }; keep.push(k); m.set(k, i); } for (let i = 0; i < 300; i += 2) m.delete(keep[i]); let s = 0, n = 0; for (const k of keep) if (m.has(k)) { s += m.get(k); n++; } console.log(n, s)",
      "150 22500" },
    { "class Cache extends WeakMap { constructor(it) { super(it); this.hits = 0; } look(k) { this.hits++; return this.get(k); } } const a = {}; const c = new Cache([[a, 'x']]); console.log(c.look(a), c.hits, c instanceof WeakMap, c instanceof Cache, Object.getPrototypeOf(Cache.prototype) === WeakMap.prototype)",
      "x 1 true true true" },
    { "const priv = new WeakMap(); class Counter { constructor() { priv.set(this, { n: 0 }); } inc() { return ++priv.get(this).n; } } const c = new Counter(), d = new Counter(); c.inc(); c.inc(); d.inc(); console.log(c.inc(), d.inc(), priv.has(c), Object.keys(c).length)",
      "3 2 true 0" },
    { "console.log(String(new WeakMap()), Object.prototype.toString.call(new WeakSet()), Object.prototype.toString.call(new WeakRef({})), String(new FinalizationRegistry(() => {})), WeakMap.length, WeakSet.length, WeakRef.length, FinalizationRegistry.length, WeakMap.name)",
      "[object WeakMap] [object WeakSet] [object WeakRef] [object FinalizationRegistry] 0 0 1 1 WeakMap" },
    { "console.log(new WeakMap(), new WeakSet())",
      "WeakMap { <items unknown> } WeakSet { <items unknown> }" },
    { "const k = Symbol('local'), w = Symbol.iterator, m = new WeakMap([[k, 1], [w, 2]]); console.log(m.get(k), m.get(w)); try { m.set(Symbol.for('reg'), 1) } catch (e) { console.log(e.constructor.name, e.message) }",
      "1 2|TypeError Invalid value used as weak map key" },
    { 0, 0 }
};

static const acase_t k_weak_errors[] = {
    { "for (const f of [() => new WeakMap().set(1, 1), () => new WeakSet().add('x'), () => new WeakRef(1), () => WeakMap(), () => WeakSet(), () => WeakRef({}), () => new FinalizationRegistry(1), () => new FinalizationRegistry(() => {}).register(1), () => { const o = {}; new FinalizationRegistry(() => {}).register(o, o) }, () => new FinalizationRegistry(() => {}).unregister(1), () => new WeakMap([1]), () => new WeakMap([[1, 2]]), () => new WeakSet([1])]) try { f(); console.log('no throw') } catch (e) { console.log(e.constructor.name) }",
      "TypeError|TypeError|TypeError|TypeError|TypeError|TypeError|TypeError|TypeError|TypeError|TypeError|TypeError|TypeError|TypeError" },
    { "for (const f of [() => new WeakMap().set(null, 1), () => new WeakSet().add(undefined), () => new WeakRef('s'), () => new FinalizationRegistry(() => {}).register({}, 1, 2)]) try { f(); console.log('no throw') } catch (e) { console.log(e.message) }",
      "Invalid value used as weak map key|Invalid value used in weak set|WeakRef: invalid target|Invalid unregisterToken ('2')" },
    { "for (const [f, t] of [[WeakMap.prototype.get, {}], [WeakSet.prototype.has, new WeakMap()], [WeakRef.prototype.deref, {}], [FinalizationRegistry.prototype.register, new WeakSet()]]) try { f.call(t, {}); console.log('no throw') } catch (e) { console.log(e.constructor.name) }",
      "TypeError|TypeError|TypeError|TypeError" },
    { 0, 0 }
};

static const acase_t k_weakref[] = {
    { "const a = { n: 1 }; const r = new WeakRef(a); console.log(r.deref() === a, r.deref().n, typeof WeakRef.prototype.deref, r instanceof WeakRef)",
      "true 1 function true" },
    { "const fr = new FinalizationRegistry(() => {}), tok = {}, a = {}, b = {}; console.log(fr.register(a, 'held', tok), fr.register(b, 'h2'), fr.register(b, 'h3', tok)); console.log(fr.unregister(tok), fr.unregister(tok), fr.unregister({}))",
      "undefined undefined undefined|true false false" },
    { 0, 0 }
};

static const acase_t k_reflect[] = {
    { "const o = { x: 1, get g() { return this.x; } }; console.log(Reflect.get(o, 'x'), Reflect.get(o, 'g', { x: 9 }), Reflect.has(o, 'x'), Reflect.has(o, 'toString'), Reflect.has(o, 'nope'), Reflect.get([5, 6], 1), Reflect.get('ab'.split(''), 'length'))",
      "1 9 true true false 6 2" },
    { "const s = Symbol('s'); console.log(Reflect.ownKeys({ b: 1, a: 2, [s]: 3, 1: 0 }).map(String).join(','), Reflect.ownKeys([7, 8]).join(','), Reflect.ownKeys(Object.defineProperty({}, 'h', { value: 1 })).join())",
      "1,b,a,Symbol(s) 0,1,length h" },
    { "const o = {}; console.log(Reflect.defineProperty(o, 'z', { value: 5 }), o.z, Reflect.defineProperty(o, 'z', { value: 6 }), Reflect.defineProperty(o, 'z', { value: 5 }), o.z, Reflect.defineProperty(o, 'z', { enumerable: true }), Reflect.defineProperty(o, 'z', { get() {} }))",
      "true 5 false true 5 false false" },
    { "const o = { x: 1 }; Object.defineProperty(o, 'k', { value: 1 }); console.log(Reflect.deleteProperty(o, 'x'), 'x' in o, Reflect.deleteProperty(o, 'k'), 'k' in o, Reflect.deleteProperty(o, 'none'))",
      "true false false true true" },
    { "const fz = Object.freeze({ k: 1 }); console.log(Reflect.set(fz, 'k', 2), fz.k, Reflect.defineProperty(fz, 'n', { value: 1 }), Reflect.isExtensible(fz), Reflect.isExtensible({}), Reflect.set(fz, 'n', 1))",
      "false 1 false false true false" },
    { "const pe = {}; console.log(Reflect.preventExtensions(pe), Reflect.isExtensible(pe), Reflect.set(pe, 'q', 1), pe.q, Reflect.setPrototypeOf(pe, null), Reflect.setPrototypeOf(pe, Object.prototype), Reflect.defineProperty(pe, 'q', { value: 1 }))",
      "true false false undefined false true false" },
    { "const p1 = {}, p2 = Object.create(p1); console.log(Reflect.setPrototypeOf(p1, p2), Reflect.getPrototypeOf(p2) === p1, Reflect.setPrototypeOf(p2, null), Reflect.getPrototypeOf(p2), Reflect.getPrototypeOf([]) === Array.prototype)",
      "false true true null true" },
    { "console.log(Reflect.apply(Math.max, null, [1, 5, 3]), Reflect.apply(function (a, b) { return this.v + a + b; }, { v: 1 }, [2, 3]), Reflect.apply(String.prototype.slice, 'hello', [1, 3]), Reflect.apply(Math.min, null, { length: 2, 0: 4, 1: 2 }))",
      "5 6 el 2" },
    { "class P { constructor(a) { this.a = a; } } class Q extends P {} const q = Reflect.construct(P, [7], Q); console.log(q.a, q instanceof Q, q instanceof P, Reflect.construct(Date, [0]).getTime(), Reflect.construct(Array, [3]).length)",
      "7 true true 0 3" },
    { "const o = { get g() { return 1; }, v: 2 }; const d = Reflect.getOwnPropertyDescriptor(o, 'g'), e = Reflect.getOwnPropertyDescriptor(o, 'v'); console.log(typeof d.get, d.set, d.enumerable, d.configurable, e.value, e.writable, Reflect.getOwnPropertyDescriptor(o, 'nope'))",
      "function undefined true true 2 true undefined" },
    { "const tgt = {}, rcv = {}; console.log(Reflect.set(tgt, 'm', 4, rcv), tgt.m, rcv.m); const sp = { set v(x) { this._v = x * 2; } }, r2 = {}; Reflect.set(sp, 'v', 5, r2); console.log(r2._v, sp._v); const ro = Object.defineProperty({}, 'w', { value: 1 }); console.log(Reflect.set(ro, 'w', 2), Reflect.set({}, 'w', 2, ro), Reflect.set({ get a() { return 1; } }, 'a', 2))",
      "true undefined 4|10 undefined|false false false" },
    { "const a = [1, 2]; console.log(Reflect.set(a, 'length', 1), a.length, Reflect.set(a, 3, 'x'), a.length, a[3])",
      "true 1 true 4 x" },
    { "for (const f of [() => Reflect.get(1, 'a'), () => Reflect.ownKeys('s'), () => Reflect.apply(1), () => Reflect.apply(Math.max, null, 1), () => Reflect.defineProperty({}, 'a', 1), () => Reflect.setPrototypeOf({}, 1), () => Reflect.has(null, 'a'), () => Reflect.construct(Math.max, [], 1)]) try { f(); console.log('no throw') } catch (e) { console.log(e.constructor.name) }",
      "TypeError|TypeError|TypeError|TypeError|TypeError|TypeError|TypeError|TypeError" },
    { "for (const n of ['get', 'set', 'has', 'ownKeys', 'deleteProperty', 'getPrototypeOf', 'isExtensible']) try { Reflect[n](5) } catch (e) { console.log(e.message) }",
      "Reflect.get called on non-object|Reflect.set called on non-object|Reflect.has called on non-object|Reflect.ownKeys called on non-object|Reflect.deleteProperty called on non-object|Reflect.getPrototypeOf called on non-object|Reflect.isExtensible called on non-object" },
    { "console.log(String(Reflect), typeof Reflect, Reflect.apply.length, Reflect.construct.length, Reflect.set.length, Reflect.defineProperty.length, Object.keys(Reflect).length, Object.getOwnPropertyNames(Reflect).length)",
      "[object Reflect] object 3 2 3 3 0 13" },
    { 0, 0 }
};

static const acase_t k_define_checks[] = {
    /* the built-in names getOwnPropertyNames and Reflect.ownKeys list (for-in still skips them) */
    { "const a=[1,2]; a.x=1; const r=[]; for (const k in a) r.push(k); console.log(Object.getOwnPropertyNames(a).join(), r.join(), Object.keys('ab').join(), Object.getOwnPropertyNames(new String('ab')).join(), Object.getOwnPropertyNames((() => 1)).join(), JSON.stringify(Object.getOwnPropertyDescriptor([], 'length')))",
      "0,1,length,x 0,1,x 0,1 0,1,length length,name {\"value\":0,\"writable\":true,\"enumerable\":false,\"configurable\":false}" },
    { "const fz = Object.freeze({ k: 1 }); for (const f of [() => Object.defineProperty(fz, 'k', { value: 3 }), () => Object.defineProperty(fz, 'nn', { value: 3 }), () => Object.defineProperty(fz, 'k', { value: 1 })]) try { f(); console.log('ok') } catch (e) { console.log(e.message) }",
      "Cannot redefine property: k|Cannot define property nn, object is not extensible|ok" },
    { "const nc = {}; Object.defineProperty(nc, 'c', { value: 1, writable: true }); Object.defineProperty(nc, 'c', { value: 2 }); console.log(nc.c); for (const d of [{ get() {} }, { configurable: true }, { enumerable: true }, { writable: false }, { value: 3 }]) try { Object.defineProperty(nc, 'c', d); console.log('ok', nc.c) } catch (e) { console.log(e.message) }",
      "2|Cannot redefine property: c|Cannot redefine property: c|Cannot redefine property: c|ok 2|Cannot redefine property: c" },
    { "const s = Symbol('t'), o = {}; Object.defineProperty(o, s, { value: 1 }); try { Object.defineProperty(o, s, { value: 2 }) } catch (e) { console.log(e.message) } const pe = Object.preventExtensions({}); try { Object.setPrototypeOf(pe, Array.prototype) } catch (e) { console.log(e.constructor.name, e.message) } console.log(Object.setPrototypeOf(pe, Object.prototype) === pe)",
      "Cannot redefine property: Symbol(t)|TypeError #<Object> is not extensible|true" },
    { "const g = () => 1, o = {}; Object.defineProperty(o, 'a', { get: g }); try { Object.defineProperty(o, 'a', { get: g }); console.log('same getter ok') } catch (e) { console.log(e.message) } try { Object.defineProperty(o, 'a', { get: () => 2 }) } catch (e) { console.log(e.message) } try { Object.defineProperties(Object.freeze({}), { x: { value: 1 } }) } catch (e) { console.log(e.message) }",
      "same getter ok|Cannot redefine property: a|Cannot define property x, object is not extensible" },
    { "const sealed = Object.seal({ a: 1 }); sealed.a = 2; console.log(sealed.a, Reflect.defineProperty(sealed, 'b', { value: 1 }), Object.isExtensible(sealed))",
      "2 false false" },
    { 0, 0 }
};

void test_weakmap_weakset(void) { run_cases(k_weakmap); }
void test_weak_errors(void) { run_cases(k_weak_errors); }
void test_weakref_registry(void) { run_cases(k_weakref); }
void test_reflect(void) { run_cases(k_reflect); }
void test_define_checks(void) { run_cases(k_define_checks); }

/* run, then collect (the collector only runs between runs); a broken
 * heap walk shows in the output */
static const char *run_gc(const char *src) {
    const char *r = run(src);
    int ck;
    js_gc(&g_vm);
    ck = js_gc_check(&g_vm);
    if (ck != 0) snprintf(g_out + strlen(g_out), sizeof(g_out) - strlen(g_out), "!heap %d", ck);
    return r;
}

/* a target nothing else holds is gone after a collection; a held one
 * stays, and so does a value whose key is alive */
void test_gc_clears_dead_keys(void) {
    fresh();
    js_gc_config(&g_vm, 0, 1, 0);
    TEST_ASSERT_EQUAL_STRING("", run_gc("var keep = { n: 'keep' }, wm = new WeakMap(); wm.set(keep, { big: 'x'.repeat(4000) });"
           "wm.set({ n: 'temp' }, 'y'.repeat(6000)); var r1 = new WeakRef({ n: 'r1' }), r2 = new WeakRef(keep); 0;"));
    TEST_ASSERT_EQUAL(2, (int)g_vm.gc_weak_cleared);      /* the temp key and r1's target */
    TEST_ASSERT_EQUAL_STRING("undefined true 4000",
                             run_gc("console.log(r1.deref(), r2.deref() === keep, wm.get(keep).big.length); 0;"));
    TEST_ASSERT_EQUAL(0, (int)g_vm.gc_weak_cleared);
    run_gc("keep = null; 0;");
    TEST_ASSERT_EQUAL_STRING("undefined", run_gc("console.log(r2.deref()); 0;"));
}

/* ephemerons: a's value holds chain, the only path to chain; chain lives
 * while a does and goes with it */
void test_gc_ephemeron_chain(void) {
    fresh();
    js_gc_config(&g_vm, 0, 1, 0);
    run_gc("var wm = new WeakMap(), a = {}, chain = { n: 'chain' }; wm.set(chain, 'v2'); wm.set(a, { next: chain });"
           "var rc = new WeakRef(chain); chain = null; 0;");
    TEST_ASSERT_EQUAL_STRING("chain v2", run_gc("console.log(rc.deref().n, wm.get(wm.get(a).next)); 0;"));
    run_gc("a = null; 0;");
    TEST_ASSERT_EQUAL_STRING("undefined", run_gc("console.log(rc.deref()); 0;"));
    TEST_ASSERT_EQUAL(0, (int)g_vm.gc_weak_cleared);   /* both went in the run after a = null */
}

/* the table's memory comes back: a REPL-like session that keeps making
 * temporary keys does not grow the arena */
void test_gc_table_reclaim(void) {
    uint32_t top1 = 0, i;
    fresh();
    js_gc_config(&g_vm, 0, 1, 0);
    run_gc("var wm = new WeakMap(), ws = new WeakSet(), fr = new FinalizationRegistry(function () {}); 0;");
    for (i = 0; i < 6; i++) {
        TEST_ASSERT_EQUAL_STRING("", run_gc("for (let i = 0; i < 2000; i++) { const k = { i }; wm.set(k, [i, i]); ws.add(k);"
                                            " fr.register(k, i); } 0;"));
        if (i == 1) top1 = g_vm.top;
    }
    TEST_ASSERT_TRUE(g_vm.top <= top1 + 4096u);
    TEST_ASSERT_EQUAL_STRING("true", run_gc("const o = {}; wm.set(o, 1); console.log(wm.get(o) === 1); 0;"));
}

/* the table bound: 16384 live keys, then a RangeError (no abort); the
 * keys and the doubled tables need more than the 3 MiB REPL arena */
static unsigned char g_big[8u * 1024u * 1024u];

void test_weak_bound(void) {
    TEST_ASSERT_EQUAL(0, js_init(&g_vm, g_big, (uint32_t)sizeof(g_big)));
    TEST_ASSERT_EQUAL_STRING("RangeError 16384",
        run("const m = new WeakMap(), keep = []; try { for (let i = 0; i < 17000; i++) { const k = {}; keep.push(k); m.set(k, 1); } }"
            " catch (e) { console.log(e.constructor.name, keep.length - 1) }"));
}

/* more weak tables than the collector clears per run: the extra ones are
 * kept strongly (garbage kept, nothing live lost) */
void test_weak_tables_past_limit(void) {
    fresh();
    js_gc_config(&g_vm, 0, 1, 0);
    run_gc("var maps = [], keys = []; for (let i = 0; i < 80; i++) { const m = new WeakMap(), k = {}; m.set(k, i);"
           " m.set({}, 'dead'); maps.push(m); keys.push(k); } 0;");
    TEST_ASSERT_EQUAL(JS_GC_WEAK, g_vm.gc_nweak);
    TEST_ASSERT_TRUE(g_vm.gc_weak_strong >= 80 - JS_GC_WEAK);
    TEST_ASSERT_EQUAL_STRING("3160",
        run_gc("let s = 0; for (let i = 0; i < 80; i++) s += maps[i].get(keys[i]); console.log(s); 0;"));
}

int main(void) {
    unity_init();
    RUN_TEST(test_weakmap_weakset);
    RUN_TEST(test_weak_errors);
    RUN_TEST(test_weakref_registry);
    RUN_TEST(test_reflect);
    RUN_TEST(test_define_checks);
    RUN_TEST(test_gc_clears_dead_keys);
    RUN_TEST(test_gc_ephemeron_chain);
    RUN_TEST(test_gc_table_reclaim);
    RUN_TEST(test_weak_bound);
    RUN_TEST(test_weak_tables_past_limit);
    unity_print_results();
    unity_cleanup();
    return unity_stats.tests_failed == 0 ? 0 : 1;
}
