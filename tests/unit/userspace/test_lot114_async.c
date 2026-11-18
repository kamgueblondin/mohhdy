/* Lot 114 (US-031): generators, async functions, Promise and the
 * microtask queue of the JavaScript subset (wf_jsco.c) on the host.
 * Each case runs a script, then the microtask queue, and compares the
 * console lines (joined with '|') with what node v24 prints for the same
 * source; the expectations were checked against node. The last groups pin
 * the bounds (suspended coroutines, microtasks per drain, the step
 * budget inside a coroutine) and the refused forms. */
#include "../../framework/unity.h"
#include <string.h>
#include <stdio.h>
#include "wf_jsi.h"

#define ARENA (3u * 1024u * 1024u)
static unsigned char g_mem[ARENA];
static js_vm_t g_vm;
static char g_out[4096];

static void fresh(void) { TEST_ASSERT_EQUAL(0, js_init(&g_vm, g_mem, ARENA)); }

/* the console lines of src and its microtasks joined with '|'; a run
 * that did not end with JS_OK adds "!" and vm->err */
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

static int evs(const char *src, jv *out) { return js_eval(&g_vm, src, (int)strlen(src), out); }

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

/* generators: next/return/throw, yield*, for-of, spread, finally */
static const acase_t k_gen[] = {
    { "function* g(){ yield 1; yield 2; return 3 } const it = g();"
      "console.log(JSON.stringify([it.next(), it.next(), it.next(), it.next()]))",
      "[{\"value\":1,\"done\":false},{\"value\":2,\"done\":false},{\"value\":3,\"done\":true},{\"done\":true}]" },
    { "function* g(){ const a = yield 'first'; console.log('a=' + a); const b = yield a * 2; return a + b }"
      "const it = g(); console.log(it.next('ignored').value); console.log(it.next(5).value); console.log(it.next(7).value)",
      "first|a=5|10|12" },
    { "function* nat(){ let i = 0; for (;;) yield i++ } const out = [];"
      "for (const n of nat()) { if (n > 4) break; out.push(n) } console.log(out.join())", "0,1,2,3,4" },
    { "function* g(){ try { yield 1; yield 2 } finally { console.log('cleanup') } }"
      "for (const v of g()) { console.log(v); break } console.log('after')", "1|cleanup|after" },
    { "function* g(){ try { yield 1 } catch (e) { console.log('caught ' + e); yield 'recovered' } }"
      "const it = g(); it.next(); console.log(it.throw('bad').value); console.log(it.next().done)",
      "caught bad|recovered|true" },
    { "function* g(){ try { yield 1; yield 2 } finally { yield 'f' } } const it = g(); it.next();"
      "console.log(JSON.stringify(it.return(9))); console.log(JSON.stringify(it.next()))",
      "{\"value\":\"f\",\"done\":false}|{\"value\":9,\"done\":true}" },
    { "function* inner(){ const x = yield 'i1'; console.log('inner got ' + x); return 'done-inner' }"
      "function* outer(){ const r = yield* inner(); console.log('r=' + r); yield 'o1' }"
      "const it = outer(); console.log(it.next().value); console.log(it.next('X').value)",
      "i1|inner got X|r=done-inner|o1" },
    { "function* g(){ yield* [1, 2]; yield* 'ab'; yield* new Set([3]) } console.log([...g()].join())", "1,2,a,b,3" },
    { "function* fib(){ let [a, b] = [0, 1]; for (;;) { yield a; [a, b] = [b, a + b] } }"
      "const r = []; for (const f of fib()) { if (r.length === 10) break; r.push(f) } console.log(r.join(' '))",
      "0 1 1 2 3 5 8 13 21 34" },
    { "function* g(){ yield 1; yield 2; yield 3 } const [a, , c] = g(); console.log(a + c)", "4" },
    { "function* g(){ yield 'x'; yield 'y' } console.log(Array.from(g()).join('+'))", "x+y" },
    { "function* g(){ yield ['k', 1]; yield ['j', 2] } const m = new Map(g()); console.log(m.get('j'), m.size)", "2 2" },
    { "const o = { *items(){ yield this.a; yield this.b }, a: 1, b: 2 }; console.log([...o.items()].join())", "1,2" },
    { "class Tree { constructor(){ this.v = [3, 1, 2] } *[\"each\"](){ for (const x of this.v.sort()) yield x } }"
      "console.log([...new Tree().each()].join())", "1,2,3" },
    { "class C { static *range(n){ for (let i = 0; i < n; i++) yield i } } console.log([...C.range(4)].join())", "0,1,2,3" },
    { "function* g(){ yield 1 } const it = g(); console.log(typeof it.next, typeof g, it.toString())",
      "function function [object Generator]" },
    { "function* g(){ yield 1 } try { new g() } catch (e) { console.log(e instanceof TypeError) }", "true" },
    { "function* g(){ const it = yield; it.next() } const it = g(); it.next();"
      "try { it.next(it) } catch (e) { console.log(e.message) }", "Generator is already running" },
    { "function* g(){ throw new Error('in body') } const it = g();"
      "try { it.next() } catch (e) { console.log(e.message) } console.log(JSON.stringify(it.next()))",
      "in body|{\"done\":true}" },
    { "function* g(){ yield 1 } const it = g(); console.log(JSON.stringify(it.return(5)), JSON.stringify(it.next()))",
      "{\"value\":5,\"done\":true} {\"done\":true}" },
    { "function* g(){ let n = 0; while (true) { const cmd = yield n; if (cmd === 'reset') n = 0; else n++ } }"
      "const it = g(); it.next(); it.next(); it.next(); console.log(it.next().value, it.next('reset').value)", "3 0" },
    { "function* take(it, n){ for (const x of it) { if (n-- <= 0) return; yield x } }"
      "function* nat(){ let i = 1; while (true) yield i++ }"
      "function* map(it, f){ for (const x of it) yield f(x) }"
      "console.log([...take(map(nat(), x => x * x), 5)].join())", "1,4,9,16,25" },
    { "function* g(){ yield 1; yield 2 } outer: for (const a of g()) { for (const b of g()) { if (b === 2) continue outer;"
      "console.log(a, b) } }", "1 1|2 1" },
    { "function* inner(){ try { yield 1 } catch (e) { console.log('inner caught', e); yield 2 } }"
      "function* outer(){ yield* inner(); yield 3 } const it = outer(); it.next();"
      "console.log(it.throw('T').value, it.next().value)", "inner caught T|2 3" },
    { 0, 0 }
};

/* async functions and await */
static const acase_t k_async[] = {
    { "async function f(){ console.log('f1'); await null; console.log('f2') } f(); console.log('sync')", "f1|sync|f2" },
    { "async function f(x){ const y = await x; return y * 2 } f(21).then(v => console.log(v))", "42" },
    { "async function f(){ throw new Error('nope') } f().catch(e => console.log('caught', e.message))", "caught nope" },
    { "async function f(){ try { await Promise.reject(new TypeError('t')) } catch (e) { return e.name } }"
      "f().then(console.log)", "TypeError" },
    { "const f = async () => 1; const g = async x => x + 1; Promise.all([f(), g(2)]).then(a => console.log(a.join()))", "1,3" },
    { "const o = { v: 5, async get(){ await 0; return this.v } }; o.get().then(console.log)", "5" },
    { "class A { constructor(){ this.n = 2 } async twice(){ return (await this.n) * 2 } } new A().twice().then(console.log)", "4" },
    { "async function f(){ let s = 0; for (let i = 0; i < 5; i++) s += await i; return s } f().then(console.log)", "10" },
    { "async function a(){ console.log('a1'); await b(); console.log('a2') }"
      "async function b(){ console.log('b1'); await null; console.log('b2') }"
      "a(); console.log('main')", "a1|b1|main|b2|a2" },
    { "async function f(){ return 1 } console.log(f() instanceof Promise, typeof f)", "true function" },
    { "const th = { then(res){ console.log('then called'); res(7) } };"
      "async function f(){ const v = await th; console.log('v', v) } f(); console.log('sync')",
      "sync|then called|v 7" },
    { "async function f(a = (() => { throw new Error('param') })()){ return 1 }"
      "const p = f(); console.log(p instanceof Promise); p.catch(e => console.log(e.message))", "true|param" },
    { "async function f(){ await 1; console.log('f') } async function g(){ console.log('g') }"
      "f(); g(); Promise.resolve().then(() => console.log('p'))", "g|f|p" },
    { "async function f(){ const r = []; for (const x of [1, 2, 3]) r.push(await Promise.resolve(x * 10)); return r }"
      "f().then(r => console.log(r.join()))", "10,20,30" },
    { "async function f(){ try { return await Promise.reject('x') } finally { console.log('fin') } }"
      "f().catch(e => console.log('e', e))", "fin|e x" },
    { "async function f(){ await new Promise(r => r()); console.log(1) } f().then(() => console.log(2))", "1|2" },
    { "async function f(){ throw 1 } async function g(){ try { await f() } catch (e) { console.log('g caught', e) } }"
      "g()", "g caught 1" },
    { "let order = []; async function w(n){ order.push('s' + n); await null; order.push('e' + n) }"
      "w(1); w(2); Promise.resolve().then(() => console.log(order.join()))", "s1,s2,e1,e2" },
    { "async function f(){ return Promise.resolve(3) } f().then(console.log); Promise.resolve().then(() => console.log('a'))"
      ".then(() => console.log('b')).then(() => console.log('c'))", "a|b|3|c" },
    { "const f = async function named(){ return named.name }; f().then(console.log)", "named" },
    { 0, 0 }
};

/* Promise: ordering, chaining, combinators, errors */
static const acase_t k_promise[] = {
    { "Promise.resolve(1).then(v => console.log('then', v)); console.log('sync')", "sync|then 1" },
    { "new Promise(r => { console.log('executor'); r(2) }).then(v => console.log(v)); console.log('after')",
      "executor|after|2" },
    { "Promise.resolve(1).then(v => v + 1).then(v => { throw v * 10 }).catch(e => e + 1).then(console.log)", "21" },
    { "Promise.reject(new Error('r')).then(() => console.log('no')).catch(e => console.log('caught', e.message))",
      "caught r" },
    { "Promise.resolve('v').finally(() => console.log('fin')).then(v => console.log('kept', v))", "fin|kept v" },
    { "Promise.reject('e').finally(() => 'ignored').catch(e => console.log('still', e))", "still e" },
    { "Promise.resolve(1).finally(() => { throw 'replaced' }).catch(e => console.log(e))", "replaced" },
    { "Promise.all([1, Promise.resolve(2), new Promise(r => r(3))]).then(v => console.log(v.join()))", "1,2,3" },
    { "Promise.all([Promise.resolve(1), Promise.reject('no'), 3]).catch(e => console.log('all', e))", "all no" },
    { "Promise.all([]).then(v => console.log(Array.isArray(v), v.length))", "true 0" },
    { "Promise.allSettled([Promise.resolve(1), Promise.reject(2)]).then(r => console.log(JSON.stringify(r)))",
      "[{\"status\":\"fulfilled\",\"value\":1},{\"status\":\"rejected\",\"reason\":2}]" },
    { "Promise.race([new Promise(r => setTimeoutLike(r)), Promise.resolve('fast')]).then(console.log);"
      "function setTimeoutLike(){}", "fast" },
    { "Promise.any([Promise.reject(1), Promise.resolve(2), Promise.resolve(3)]).then(console.log)", "2" },
    { "Promise.any([Promise.reject(1), Promise.reject(2)]).catch(e => console.log(e.name, e.errors.join(), e.message))",
      "AggregateError 1,2 All promises were rejected" },
    { "const p = Promise.resolve(); p.then(() => console.log(1)); p.then(() => console.log(2)); p.then(() => console.log(3))",
      "1|2|3" },
    { "Promise.resolve().then(() => { console.log('a'); Promise.resolve().then(() => console.log('c')) })"
      ".then(() => console.log('b'))", "a|c|b" },
    { "queueMicrotask(() => console.log('q')); Promise.resolve().then(() => console.log('p')); console.log('s')", "s|q|p" },
    { "const p = new Promise((res, rej) => { res(1); rej(2); res(3) }); p.then(console.log, () => console.log('no'))", "1" },
    { "new Promise(() => { throw new RangeError('ex') }).catch(e => console.log(e.name))", "RangeError" },
    { "const p = Promise.resolve(); const q = p.then(() => q); q.catch(e => console.log(e.constructor.name, e.message))",
      "TypeError Chaining cycle detected for promise #<Promise>" },
    { "const p = Promise.resolve(5); const r = Promise.reject(1); r.catch(() => {});"
      "console.log(Promise.resolve(p) === p, r instanceof Promise)", "true true" },
    { "try { Promise(() => {}) } catch (e) { console.log(e instanceof TypeError) }", "true" },
    { "try { new Promise(1) } catch (e) { console.log(e.message) }", "Promise resolver 1 is not a function" },
    { "Promise.resolve({ then(r){ r('thenable') } }).then(console.log)", "thenable" },
    { "Promise.resolve(1).then(2).then(v => console.log('passed', v))", "passed 1" },
    { "Promise.reject(3).then(null, e => console.log('second arg', e))", "second arg 3" },
    { "console.log(Object.prototype.toString.call(Promise.resolve()))", "[object Promise]" },
    { "Promise.reject(new Error('lost'))", "Uncaught (in promise) Error: lost!Uncaught (in promise) Error: lost" },
    { "const p = Promise.reject(1); p.catch(() => console.log('handled late'))", "handled late" },
    { "queueMicrotask(() => { throw new Error('mt') }); queueMicrotask(() => console.log('next still runs'))",
      "Uncaught Error: mt|next still runs!Uncaught Error: mt" },
    { 0, 0 }
};


/* Lot 115: every case again in one realm, a collection with freed
 * memory poisoned after each (wf_jsgc.c): the results must not change */
static void run_cases_shared(const acase_t *c) {
    static char src[8192];
    int i;
    fresh();
    js_gc_config(&g_vm, 1, 1, 0);
    for (i = 0; c[i].src; i++) {
        const char *got;
        snprintf(src, sizeof(src), "{\n%s\n}", c[i].src);
        got = run(src);
        if (strcmp(got, c[i].want) != 0) printf("  shared case %d: %s\n    got  %s\n    want %s\n", i, c[i].src, got, c[i].want);
        TEST_ASSERT_EQUAL_STRING(c[i].want, got);
        TEST_ASSERT_TRUE(js_gc_idle(&g_vm));
        TEST_ASSERT_EQUAL(0, js_gc_check(&g_vm));
    }
}

static void test_shared_realm_reclaim(void) {
    run_cases_shared(k_gen);
    run_cases_shared(k_async);
    run_cases_shared(k_promise);
}

static void test_l114_generators(void) { run_cases(k_gen); }
static void test_l114_async(void) { run_cases(k_async); }
static void test_l114_promise(void) { run_cases(k_promise); }

/* the console and REPL display of the new kinds of values (as node prints them) */
static void test_l114_display(void) {
    char b[200];
    jv v;
    fresh();
    TEST_ASSERT_EQUAL(JS_OK, evs("Promise.resolve(1)", &v));
    js_display(&g_vm, v, b, (int)sizeof(b), 1);
    TEST_ASSERT_EQUAL_STRING("Promise { 1 }", b);
    TEST_ASSERT_EQUAL(JS_OK, evs("new Promise(() => {})", &v));
    js_display(&g_vm, v, b, (int)sizeof(b), 1);
    TEST_ASSERT_EQUAL_STRING("Promise { <pending> }", b);
    TEST_ASSERT_EQUAL(JS_OK, evs("var r = Promise.reject('x'); r.catch(() => 0); r", &v));
    js_display(&g_vm, v, b, (int)sizeof(b), 1);
    TEST_ASSERT_EQUAL_STRING("Promise { <rejected> 'x' }", b);
    TEST_ASSERT_EQUAL(JS_OK, evs("(function* gen(){})", &v));
    js_display(&g_vm, v, b, (int)sizeof(b), 1);
    TEST_ASSERT_EQUAL_STRING("[GeneratorFunction: gen]", b);
    TEST_ASSERT_EQUAL(JS_OK, evs("(async function af(){})", &v));
    js_display(&g_vm, v, b, (int)sizeof(b), 1);
    TEST_ASSERT_EQUAL_STRING("[AsyncFunction: af]", b);
    TEST_ASSERT_EQUAL(JS_OK, evs("(function* (){})()", &v));
    js_display(&g_vm, v, b, (int)sizeof(b), 1);
    TEST_ASSERT_EQUAL_STRING("Object [Generator] {}", b);
    TEST_ASSERT_EQUAL(JS_OK, js_run_jobs(&g_vm));
}

/* the realm keeps suspended coroutines across runs (a REPL line, then a
 * page callback): the queue is drained by the host, not by js_eval */
static void test_l114_across_runs(void) {
    int n;
    const char *log;
    fresh();
    js_set_budget(&g_vm, 1000000u);
    TEST_ASSERT_EQUAL(JS_OK, evs("var resume; var p = new Promise(r => resume = r);"
                                            "(async () => { console.log('got ' + await p) })(); 'queued'", 0));
    TEST_ASSERT_EQUAL(0, js_jobs_pending(&g_vm));
    TEST_ASSERT_EQUAL(JS_OK, evs("resume('later')", 0));
    TEST_ASSERT_EQUAL(1, js_jobs_pending(&g_vm));
    js_log_clear(&g_vm);
    TEST_ASSERT_EQUAL(JS_OK, js_run_jobs(&g_vm));
    log = js_log_text(&g_vm, &n);
    TEST_ASSERT_EQUAL_STRING("got later\n", log);
    TEST_ASSERT_EQUAL(0, js_jobs_pending(&g_vm));
    /* a generator started in one run, resumed in the next */
    TEST_ASSERT_EQUAL(JS_OK, evs("function* g(){ yield 1; yield 2 } var it = g(); it.next().value", 0));
    TEST_ASSERT_EQUAL_STRING("2", run("console.log(it.next().value)"));
}

/* bounds: suspended coroutines, coroutine stack depth, microtasks, budget */
static void test_l114_bounds(void) {
    const char *r;
    fresh();
    /* JS_CO_MAX started coroutines hold a stack at once; the next one is refused */
    r = run("var keep = []; function* g(){ yield 1 } try { for (let i = 0; i < 20; i++) { const it = g(); it.next(); keep.push(it) } }"
            "catch (e) { console.log(e.name, keep.length) }");
    TEST_ASSERT_EQUAL_STRING("RangeError 8", r);
    /* a finished coroutine gives its stack back */
    fresh();
    TEST_ASSERT_EQUAL_STRING("100", run("let s = 0; function* g(){ yield 1 } for (let i = 0; i < 100; i++) { for (const v of g()) s += v } console.log(s)"));
    fresh();
    TEST_ASSERT_EQUAL_STRING("300", run("async function f(i){ await null; return i } let n = 0;"
                                       "(async () => { for (let i = 0; i < 300; i++) n += (await f(1)) } )().then(() => console.log(n))"));
    /* deep recursion inside a generator stays a RangeError */
    fresh();
    r = run("function* g(){ function d(n){ return n ? d(n - 1) + 1 : 0 } yield d(1000) } try { g().next() } catch (e) { console.log(e.name) }");
    TEST_ASSERT_EQUAL_STRING("RangeError", r);
    /* a microtask loop that never ends is stopped by a bound (the arena
     * or the step budget, whichever comes first; nothing is collected) */
    fresh();
    r = run("function again(){ Promise.resolve().then(again) } again()");
    TEST_ASSERT_NOT_NULL(strstr(r, "!RangeError: "));
    TEST_ASSERT_EQUAL(JS_OK, js_init(&g_vm, g_mem, ARENA));
    /* an endless generator consumed by spread: the same, not the C stack */
    r = run("function* inf(){ let i = 0; while (true) yield i++ } [...inf()]");
    TEST_ASSERT_NOT_NULL(strstr(r, "!RangeError: "));
}

/* refused forms, with a SyntaxError that names them */
static void test_l114_refused(void) {
    fresh();
    /* Lot 115 runs async generators and for await; what stays refused */
    TEST_ASSERT_EQUAL_STRING("object", run("async function* g(){} console.log(typeof g())"));
    TEST_ASSERT_NOT_NULL(strstr(run("function f(){ for await (const x of y) {} }"), "for await is only valid in async functions"));
    TEST_ASSERT_NOT_NULL(strstr(run("function f(){ await 1 }"), "await is only valid in async functions"));
    TEST_ASSERT_EQUAL_STRING("function", run("class A { async *g(){} } console.log(typeof new A().g().next)"));
    TEST_ASSERT_NOT_NULL(strstr(run("async function f(){ for await (const x in y) {} }"), "for await needs of"));
    TEST_ASSERT_NOT_NULL(strstr(run("class A { async constructor(){} }"), "Class constructor may not be"));
    /* yield is an identifier outside generators (sloppy scripts) */
    TEST_ASSERT_EQUAL_STRING("3", run("var yield = 3; console.log(yield)"));
    TEST_ASSERT_EQUAL_STRING("4", run("var async = 4; console.log(async)"));
    TEST_ASSERT_EQUAL_STRING("5", run("function async(x){ return x } console.log(async(5))"));
}

int main(void) {
    unity_init();
    RUN_TEST(test_l114_generators);
    RUN_TEST(test_l114_async);
    RUN_TEST(test_l114_promise);
    RUN_TEST(test_l114_display);
    RUN_TEST(test_l114_across_runs);
    RUN_TEST(test_l114_bounds);
    RUN_TEST(test_l114_refused);
    RUN_TEST(test_shared_realm_reclaim);
    unity_print_results();
    unity_cleanup();
    return unity_stats.tests_failed == 0 ? 0 : 1;
}
