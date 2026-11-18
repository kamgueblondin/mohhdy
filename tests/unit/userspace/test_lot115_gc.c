/* Lot 115 (US-031): arena reclaim for the JavaScript subset (wf_jsgc.c),
 * on the host. Until Lot 114 a realm only grew; now unreachable blocks
 * are reclaimed between runs by a mark and sweep collector that never
 * moves a block. Each scenario runs its stages in one realm with the
 * collector in stress mode (a collection after every stage) and freed
 * memory poisoned, so a block freed while still in use shows up as a
 * wrong value; the console lines of every stage are compared with what
 * node v24 prints when it runs the same stages in one context with the
 * microtasks drained between them (the expectations were produced by
 * node). The other tests check that the reclaim lets a long session
 * run in a small arena (and that the same session fails without it),
 * that freed blocks are reused, that suspended generators nobody can
 * reach give their stacks back, that a collection is refused while a
 * script runs, the host roots, the bounded mark stack and the heap
 * invariants. */
#include "../../framework/unity.h"
#include <string.h>
#include <stdio.h>
#include "wf_jsi.h"

#define ARENA (3u * 1024u * 1024u)
#define SMALL (1024u * 1024u)
static unsigned char g_mem[ARENA];
static js_vm_t g_vm;
static char g_out[4096];

static void fresh_size(uint32_t size) {
    TEST_ASSERT_EQUAL(0, js_init(&g_vm, g_mem, size));
}

static int run_rc(const char *src) {
    int rc, n, i, k = 0;
    const char *log;
    js_log_clear(&g_vm);
    js_set_budget(&g_vm, 30000000u);
    rc = js_eval(&g_vm, src, (int)strlen(src), 0);
    if (rc == JS_OK) {
        js_set_budget(&g_vm, 30000000u);
        rc = js_run_jobs(&g_vm);
    }
    log = js_log_text(&g_vm, &n);
    while (n > 0 && log[n - 1] == '\n') n--;
    for (i = 0; i < n && k < (int)sizeof(g_out) - 1; i++) g_out[k++] = log[i] == '\n' ? '|' : log[i];
    g_out[k] = 0;
    if (rc != JS_OK) snprintf(g_out + k, sizeof(g_out) - (size_t)k, "!%s", js_last_error(&g_vm));
    return rc;
}

static const char *run(const char *src) { (void)run_rc(src); return g_out; }

#define MAX_STAGES 8
typedef struct { const char *name; const char *stage[MAX_STAGES]; const char *want[MAX_STAGES]; } gscen_t;

static const gscen_t k_scen[] = {
    { "closures",
      {
        "var keep = []; function mk(i){ var s = 'v' + i; return function(){ return s + '!' }; }for (var i "
        "= 0; i < 3000; i++) { var junk = {a: i, b: 'x' + i, c: [i, i + 1]}; if (i % 500 == 0) "
        "keep.push(mk(i)); }console.log(keep.length)",
        "for (var j = 0; j < 3000; j++) { var t = 'garbage' + j + [j, j * 2].join(); "
        "}console.log(keep.map(function(f){ return f() }).join())",
        "keep.length = 2; console.log(keep[1](), typeof junk, junk.c.join())",
        0 },
      {
        "6",
        "v0!,v500!,v1000!,v1500!,v2000!,v2500!",
        "v500! object 2999,3000",
        0 } },
    { "objects",
      {
        "var o = {list: [], byKey: {}}; for (var i = 0; i < 200; i++) { var e = {id: i, name: 'n' + i, "
        "tags: ['t' + (i % 7)]}; o.list.push(e); o.byKey['k' + i] = e; } o.list.length = 50; "
        "console.log(Object.keys(o.byKey).length)",
        "for (var i = 0; i < 300; i++) { var tmp = JSON.parse(JSON.stringify({a: [1, 2, {b: 'c' + i}]})); "
        "}console.log(o.byKey.k199.name, o.list[49].tags[0], o.byKey.k10 === o.list[10])",
        "delete o.byKey.k5; var s = 0; for (var k in o.byKey) s += o.byKey[k].id; console.log(s)",
        0 },
      {
        "200",
        "n199 t0 true",
        "19895",
        0 } },
    { "getters",
      {
        "var acc = {_v: 'init', get v(){ return this._v + '*' }, set v(x){ this._v = 'set:' + x "
        "}};Object.defineProperty(acc, 'ro', {get: function(){ return 'ro' + this._v.length }, "
        "enumerable: false}); acc.v = 'first'",
        "for (var i = 0; i < 2000; i++) { var g = {get x(){ return i }}; g.x; }console.log(acc.v, acc.ro, "
        "Object.keys(acc).join())",
        "acc.v = 'again'; console.log(acc.v, acc.ro)",
        0 },
      {
        "",
        "set:first* ro9 _v,v",
        "set:again* ro9",
        0 } },
    { "classes",
      {
        "class Base { constructor(n){ this.n = n } who(){ return 'base ' + this.n } static make(n){ "
        "return new this(n) } }class Kid extends Base { #secret; constructor(n, s){ super(n); "
        "this.#secret = s } who(){ return super.who() + ' kid ' + this.#secret } static #count = 0; "
        "static bump(){ return ++Kid.#count } get s(){ return this.#secret } }var kids = []; for (var i = "
        "0; i < 300; i++) { var k = new Kid(i, 's' + i); if (i % 100 == 0) kids.push(k); }",
        "for (var i = 0; i < 1500; i++) { var tmp = new Kid(i, 'tmp'); tmp.who(); "
        "}console.log(kids.length, kids[2].who(), Kid.bump(), Kid.bump(), kids[1] instanceof Base)",
        "console.log(Kid.make(7).who(), kids[0].s, Object.getPrototypeOf(kids[0]) === Kid.prototype)",
        0 },
      {
        "",
        "3 base 200 kid s200 1 2 true",
        "base 7 kid undefined s0 true",
        0 } },
    { "generators",
      {
        "function* counter(){ var t = 0; for (;;) { var got = yield t; t += got === undefined ? 1 : got } "
        "}var live = counter(); live.next(); live.next(10);for (var i = 0; i < 6; i++) { var dropped = "
        "counter(); dropped.next() } dropped = null",
        "for (var i = 0; i < 6; i++) { var d2 = counter(); d2.next(); d2.next(2) } d2 = null; "
        "console.log(live.next(5).value)",
        "var held = []; for (var i = 0; i < 6; i++) { var h = counter(); h.next(); held.push(h) } h = "
        "null;console.log(held[5].next(3).value, live.next().value)",
        "held = null; console.log(live.next(0).value)",
        "var again = []; for (var i = 0; i < 7; i++) { var a = counter(); a.next(i); again.push(a) } a = "
        "null;console.log(again.map(function(g){ return g.next(1).value }).join())",
        0 },
      {
        "",
        "15",
        "3 16",
        "16",
        "1,1,1,1,1,1,1",
        0 } },
    { "iterators",
      {
        "function* inner(){ yield 'a'; yield 'b'; return 'r' }var it = inner(); it.next(); var arrIt = "
        "['x', 'y', 'z'][Symbol.iterator](); arrIt.next();var m = new Map([['k1', {v: 1}], ['k2', {v: "
        "2}]]); var mi = m.entries(); mi.next();",
        "for (var i = 0; i < 2000; i++) { var junk = [...'abc'].concat(Array.from({length: 3}, "
        "function(_, j){ return j })) }console.log(JSON.stringify(it.next()), "
        "JSON.stringify(arrIt.next()), JSON.stringify(mi.next().value))",
        "console.log(JSON.stringify([...it]), [...arrIt].join(), mi.next().done)",
        0 },
      {
        "",
        "{\"value\":\"b\",\"done\":false} {\"value\":\"y\",\"done\":false} [\"k2\",{\"v\":2}]",
        "[] z true",
        0 } },
    { "promises",
      {
        "var log = []; var resolveLater; var pending = new Promise(function(r){ resolveLater = r "
        "});pending.then(function(v){ log.push('then:' + v.x) }); var chain = pending.then(function(v){ "
        "return {y: v.x + '!'} });async function af(n){ var a = await pending; return a.x + n }",
        "for (var i = 0; i < 2000; i++) { var junk = {i: i, s: 'junk' + i} }var r1 = af('-a'); "
        "resolveLater({x: 'X'}); r1.then(function(v){ log.push(v) })",
        "chain.then(function(v){ log.push(v.y) }); Promise.all([pending, af('-b')]).then(function(a){ "
        "log.push(a[1]) })",
        "console.log(log.join(','))",
        0 },
      {
        "",
        "",
        "",
        "then:X,X-a,X!,X-b",
        0 } },
    { "async_suspended",
      {
        "var gate; var waiting = []; var p = new Promise(function(r){ gate = r });async function w(i){ "
        "var v = await p; return v + i }for (var i = 0; i < 5; i++) waiting.push(w(i))",
        "for (var i = 0; i < 3000; i++) { var junk = 'j' + i + 'k' }",
        "gate('v'); Promise.all(waiting).then(function(a){ console.log(a.join()) })",
        0 },
      {
        "",
        "",
        "v0,v1,v2,v3,v4",
        0 } },
    { "collections",
      {
        "var m = new Map(), s = new Set(); for (var i = 0; i < 300; i++) { m.set('k' + i, {i: i}); if (i "
        "% 3 == 0) s.add('s' + i) }for (var i = 0; i < 300; i += 2) m.delete('k' + i)",
        "for (var i = 0; i < 2000; i++) { var tmp = new Map([[i, String(i)]]); var t2 = new Set([i, i + "
        "1]) }console.log(m.size, s.size, m.get('k299').i, s.has('s99'), m.has('k10'))",
        "var tot = 0; m.forEach(function(v){ tot += v.i }); console.log(tot, [...s].slice(0, 3).join())",
        0 },
      {
        "",
        "150 100 299 true false",
        "22500 s0,s3,s6",
        0 } },
    { "symbols",
      {
        "var tag = Symbol('tag'), reg = Symbol.for('app.key'); var holder = {[tag]: {deep: 'value'}, "
        "[reg]: 'registered'};var custom = {[Symbol.toPrimitive](h){ return h == 'number' ? 42 : 'str' }, "
        "[Symbol.toStringTag]: 'Custom'}",
        "for (var i = 0; i < 2000; i++) { var junk = Symbol('j' + i); var o2 = {[junk]: i} "
        "}console.log(holder[tag].deep, holder[Symbol.for('app.key')], +custom, `${custom}`, "
        "String(custom), Object.prototype.toString.call(custom))",
        "console.log(Object.getOwnPropertySymbols(holder).length, Symbol.keyFor(reg), tag.description)",
        0 },
      {
        "",
        "value registered 42 str str [object Custom]",
        "2 app.key tag",
        0 } },
    { "strings",
      {
        "var parts = []; for (var i = 0; i < 400; i++) parts.push('p' + i + '\\u00e9'); var big = "
        "parts.join('-');var emoji = '\\u{1F600}x\\u{1F680}'; var re = /p(\\d+)/g; re.exec(big); "
        "re.exec(big);",
        "for (var i = 0; i < 2000; i++) { var junk = big.slice(i, i + 20).toUpperCase() "
        "}console.log(big.length, emoji.length, emoji.codePointAt(3).toString(16), re.lastIndex, "
        "re.exec(big)[1])",
        "console.log(big.split('-')[399], emoji.slice(0, 2) + emoji.slice(2), [...emoji].length, "
        "big.indexOf('p300'))",
        0 },
      {
        "",
        "2289 5 1f680 6 2",
        "p399\303\251 \360\237\230\200x\360\237\232\200 3 1690",
        0 } },
    { "bound_and_args",
      {
        "function sum(){ var t = this.base; for (var i = 0; i < arguments.length; i++) t += arguments[i]; "
        "return t }var b1 = sum.bind({base: 100}, 1, 2); var argsHeld; (function(){ argsHeld = arguments "
        "})('a', 'b', {c: 'd'})",
        "for (var i = 0; i < 2000; i++) { var junk = sum.bind({base: i}, i)(1) }console.log(b1(3, 4), "
        "argsHeld.length, argsHeld[2].c, Array.prototype.join.call(argsHeld, ':').length)",
        "var sorted = [5, 1, 4, 2, 3].map(function(n){ return {n: n} }).sort(function(a, b){ return a.n - "
        "b.n });console.log(sorted.map(function(o){ return o.n }).join(), b1.name, b1.length)",
        0 },
      {
        "",
        "110 3 d 19",
        "1,2,3,4,5 bound sum 0",
        0 } },
    { 0, { 0 }, { 0 } }
};

static void test_scenarios(void) {
    int s, i;
    for (s = 0; k_scen[s].name; s++) {
        fresh_size(ARENA);
        js_gc_config(&g_vm, 1, 1, 0);
        for (i = 0; k_scen[s].stage[i]; i++) {
            const char *got = run(k_scen[s].stage[i]);
            if (strcmp(got, k_scen[s].want[i]) != 0)
                printf("  %s stage %d\n    got  %s\n    want %s\n", k_scen[s].name, i, got, k_scen[s].want[i]);
            TEST_ASSERT_EQUAL_STRING(k_scen[s].want[i], got);
            TEST_ASSERT_TRUE(js_gc_idle(&g_vm));
            TEST_ASSERT_EQUAL(0, js_gc_check(&g_vm));
        }
    }
}

/* about 200 KiB of garbage per line, a small result kept */
static const char *k_churn =
    "var kept = typeof kept === 'undefined' ? [] : kept;"
    "for (var i = 0; i < 1500; i++) { var o = {i: i, s: 'item ' + i, a: [i, i + 1, i + 2]} }"
    "kept.push('k' + kept.length); kept.length";

static void test_long_session_small_arena(void) {
    int i, rc = JS_OK;
    js_gc_stats_t st;
    fresh_size(SMALL);
    for (i = 0; i < 60; i++) {
        rc = run_rc(k_churn);
        if (rc != JS_OK) break;
        js_gc_idle(&g_vm);
    }
    TEST_ASSERT_EQUAL(JS_OK, rc);
    TEST_ASSERT_EQUAL_STRING("", run("if (kept.length !== 60 || kept[59] !== 'k59') console.log('lost')"));
    js_gc_stats(&g_vm, &st);
    TEST_ASSERT_TRUE(st.runs >= 5);
    TEST_ASSERT_TRUE(st.freed_total > 5u * 1024u * 1024u);
    TEST_ASSERT_TRUE(st.live < 256u * 1024u);
    TEST_ASSERT_EQUAL(0, js_gc_check(&g_vm));
}

/* the same session without the collector fills the arena: the test above means something */
static void test_session_fails_without_reclaim(void) {
    int i, rc = JS_OK;
    fresh_size(SMALL);
    js_gc_config(&g_vm, 0, 0, 1);
    for (i = 0; i < 60 && rc == JS_OK; i++) {
        rc = run_rc(k_churn);
        js_gc_idle(&g_vm);           /* off: does nothing */
    }
    TEST_ASSERT_EQUAL(JS_ABORTED, rc);
    TEST_ASSERT_NOT_NULL(strstr(js_last_error(&g_vm), "out of memory"));
    TEST_ASSERT_EQUAL(0, (int)g_vm.gc_runs);
}

static void test_free_blocks_reused(void) {
    uint32_t top0;
    int i;
    fresh_size(ARENA);
    TEST_ASSERT_EQUAL(JS_OK, run_rc(k_churn));
    js_gc(&g_vm);
    TEST_ASSERT_EQUAL(JS_OK, run_rc(k_churn));
    js_gc(&g_vm);
    top0 = g_vm.top;
    for (i = 0; i < 6; i++) {
        TEST_ASSERT_EQUAL(JS_OK, run_rc(k_churn));
        js_gc(&g_vm);
        TEST_ASSERT_EQUAL(0, js_gc_check(&g_vm));
    }
    /* the later lines ran in the blocks the earlier ones left */
    TEST_ASSERT_TRUE(g_vm.top <= top0 + 64u * 1024u);
    TEST_ASSERT_EQUAL_STRING("k7", run("console.log(kept[7])"));
}

static uint32_t g_inner_rc;
static jv gc_now(js_vm_t *vm, jv thisv, jv *argv, int argc, uint32_t m) {
    (void)thisv; (void)argv; (void)argc; (void)m;
    g_inner_rc = js_gc(vm);
    return jv_int((int32_t)js_gc_busy(vm));
}

static void test_refused_while_running(void) {
    js_gc_stats_t st;
    fresh_size(ARENA);
    js_def_native(&g_vm, g_vm.global, "gcNow", gc_now, 0, 0);
    g_inner_rc = 77;
    TEST_ASSERT_EQUAL_STRING("1", run("for (var i = 0; i < 500; i++) var g = {i: i}; console.log(gcNow())"));
    TEST_ASSERT_EQUAL(0, (int)g_inner_rc);
    g_inner_rc = 77;
    TEST_ASSERT_EQUAL_STRING("1|1", run("function* gen(){ console.log(gcNow()); yield 1 } var it = gen(); it.next();"
                                        "async function af(){ await null; console.log(gcNow()) } af()"));
    TEST_ASSERT_EQUAL(0, (int)g_inner_rc);
    js_gc_stats(&g_vm, &st);
    TEST_ASSERT_EQUAL(0, (int)st.runs);
    TEST_ASSERT_EQUAL(0, js_gc_busy(&g_vm));
    TEST_ASSERT_TRUE(js_gc(&g_vm) > 0);
}

static uint32_t g_keep;
static void keep_roots(js_vm_t *vm, void *ctx) {
    (void)ctx;
    js_gc_scan(vm, &g_keep, (uint32_t)sizeof(g_keep));
}

static void test_host_roots(void) {
    jv v;
    char b[64];
    uint32_t freed_without;
    fresh_size(ARENA);
    js_gc_config(&g_vm, 0, 1, 0);
    js_gc(&g_vm);
    {
        const char *src = "({k: 'kept ' + [1, 2].join('+'), big: new Array(500).fill(7)})";
        TEST_ASSERT_EQUAL(JS_OK, js_eval(&g_vm, src, (int)strlen(src), &v));
    }
    TEST_ASSERT_EQUAL(JT_OBJ, v.t);
    g_keep = v.a;
    TEST_ASSERT_EQUAL(JS_OK, js_eval(&g_vm, "0", 1, 0));      /* the completion no longer holds it */
    js_gc_set_roots(&g_vm, keep_roots, 0);
    (void)js_gc(&g_vm);
    TEST_ASSERT_EQUAL(0, js_gc_check(&g_vm));
    TEST_ASSERT_TRUE(js_to_cstr(&g_vm, js_get(&g_vm, jv_obj(g_keep), js_intern(&g_vm, "k", 1)), b, (int)sizeof(b)) > 0);
    TEST_ASSERT_EQUAL_STRING("kept 1+2", b);
    js_gc_set_roots(&g_vm, 0, 0);
    freed_without = js_gc(&g_vm);
    /* nothing else held it: the object, its 500 elements and its string went back */
    TEST_ASSERT_TRUE(freed_without >= 500u * (uint32_t)sizeof(jv));
    TEST_ASSERT_EQUAL(0, js_gc_check(&g_vm));
}

static void test_unreachable_generators_released(void) {
    int round;
    fresh_size(ARENA);
    TEST_ASSERT_EQUAL(JS_OK, run_rc("function* g(){ for (;;) yield 1 } var live = g(); live.next()"));
    for (round = 0; round < 5; round++) {
        /* seven suspended generators nobody keeps: JS_CO_MAX is 8 */
        TEST_ASSERT_EQUAL_STRING("", run("for (var i = 0; i < 7; i++) { var d = g(); d.next() } d = null"));
        js_gc(&g_vm);
        TEST_ASSERT_EQUAL(1, g_vm.co_live);
    }
    TEST_ASSERT_EQUAL_STRING("1", run("console.log(live.next().value)"));
    /* without the collection between lines the bound is reached, as before */
    TEST_ASSERT_EQUAL_STRING("!Uncaught RangeError: too many suspended generators or async functions (guest bound)",
                             run("for (var i = 0; i < 9; i++) { var d = g(); d.next() }"));
}

static void test_mark_stack_overflow(void) {
    js_gc_stats_t st;
    fresh_size(ARENA);
    js_gc_config(&g_vm, 0, 1, 0);
    /* 6000 objects reached from one array: more than JS_GC_MSTACK pending blocks */
    TEST_ASSERT_EQUAL(JS_OK, run_rc("var wide = []; for (var i = 0; i < 6000; i++) wide.push({v: i, s: 's' + i});"
                                    "var tree = {kids: []}; for (var i = 0; i < 600; i++) tree.kids.push({kids: [{leaf: i}]})"));
    js_gc(&g_vm);
    TEST_ASSERT_EQUAL(0, js_gc_check(&g_vm));
    TEST_ASSERT_EQUAL_STRING("17997000 s5999 179700",
        run("var t = 0, u = 0; for (var i = 0; i < 6000; i++) t += wide[i].v;"
            "for (var i = 0; i < 600; i++) u += tree.kids[i].kids[0].leaf;"
            "console.log(t, wide[5999].s, u)"));
    js_gc_stats(&g_vm, &st);
    TEST_ASSERT_TRUE(st.live > 6000u * (uint32_t)sizeof(js_obj_t));
}

static void test_bounds_and_stats(void) {
    js_gc_stats_t st;
    uint32_t heap;
    fresh_size(ARENA);
    js_gc_stats(&g_vm, &st);
    /* one bit per 4 bytes below the interpreter stack */
    TEST_ASSERT_EQUAL((int)((((ARENA - JS_STACK_SIZE) >> 2) + 31u) / 32u * 4u), (int)st.bitmap);
    TEST_ASSERT_EQUAL(0, (int)st.runs);
    heap = g_vm.limit - g_vm.gc_heap_lo;
    TEST_ASSERT_EQUAL((int)(g_vm.limit - g_vm.top), (int)js_mem_free(&g_vm));
    js_gc(&g_vm);
    js_gc_stats(&g_vm, &st);
    TEST_ASSERT_EQUAL(1, (int)st.runs);
    TEST_ASSERT_TRUE(st.live > 0 && st.live < heap);
    TEST_ASSERT_TRUE(st.free_bytes + st.live <= heap);
    /* the idle policy: nothing allocated since the last run, nothing to do */
    TEST_ASSERT_EQUAL(0, js_gc_idle(&g_vm));
    TEST_ASSERT_EQUAL(0, js_gc_check(&g_vm));
}

int main(void) {
    unity_init();
    RUN_TEST(test_scenarios);
    RUN_TEST(test_long_session_small_arena);
    RUN_TEST(test_session_fails_without_reclaim);
    RUN_TEST(test_free_blocks_reused);
    RUN_TEST(test_refused_while_running);
    RUN_TEST(test_host_roots);
    RUN_TEST(test_unreachable_generators_released);
    RUN_TEST(test_mark_stack_overflow);
    RUN_TEST(test_bounds_and_stats);
    unity_print_results();
    unity_cleanup();
    return unity_stats.tests_failed == 0 ? 0 : 1;
}
