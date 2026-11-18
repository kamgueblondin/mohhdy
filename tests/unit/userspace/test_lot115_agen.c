/* Lot 115 (US-031): async generators and for await in the JavaScript
 * subset (wf_jsco.c, wf_jsparse.c, wf_jsrun.c), on the host. An async
 * generator runs on a coroutine and suspends for a yield or an await;
 * next/throw/return queue requests answered by promises. for await steps
 * an async iterator with an await on each result, awaits each value of a
 * plain iterable, and awaits return() on an early exit. The cases compare
 * the console lines with what node v24 prints for the same source
 * (expectations produced by node), including the microtask order of
 * queued requests and the await of a return value. The last tests pin the refused yield*, the request
 * bound, and that abandoned suspended generators give their coroutine
 * stacks back to the collector. */
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

static const acase_t k_agen[] = {
    { "const out = []; const done = () => console.log(out.join(' ')); async function* count(n) { for (let i = 1; i <= n; i++) { await null; yield i * 10; } return 'end'; } (async () => { for await (const v of count(3)) out.push(v); const g = count(2); const r = [await g.next(), await g.next(), await g.next(), await g.next()]; out.push(r.map(x => x.value + ':' + x.done).join(',')); done(); })()",
      "10 20 30 10:false,20:false,end:true,undefined:true" },
    { "const out = []; const done = () => console.log(out.join(' ')); async function* fail() { yield 1; throw new Error('boom'); } (async () => { try { for await (const v of fail()) out.push('f' + v); } catch (e) { out.push(e.message); } done(); })()",
      "f1 boom" },
    { "const out = []; const done = () => console.log(out.join(' ')); let fin = ''; async function* cleanup() { try { yield 1; yield 2; } finally { fin = 'cleaned'; } } (async () => { for await (const v of cleanup()) { out.push('c' + v); break; } out.push(fin); done(); })()",
      "c1 cleaned" },
    { "const out = []; const done = () => console.log(out.join(' ')); async function* count(n) { for (let i = 1; i <= n; i++) yield i * 10; } (async () => { const q = count(5); const ps = [q.next(), q.next(), q.next()]; out.push((await Promise.all(ps)).map(x => x.value).join('/')); const rr = await q.return('stop'); out.push(rr.value + ' ' + rr.done + ' ' + JSON.stringify(await q.next())); const t = count(1); try { await t.throw(new Error('early')); } catch (e) { out.push('thrown ' + e.message); } out.push(JSON.stringify(await t.next())); done(); })()",
      "10/20/30 stop true {\"done\":true} thrown early {\"done\":true}" },
    { "const out = []; const done = () => console.log(out.join(' ')); async function* echo() { let got = yield 'first'; while (got) got = yield 'got ' + got; } (async () => { const e = echo(); out.push((await e.next()).value, (await e.next('a')).value, (await e.next('b')).value, (await e.next()).done); done(); })()",
      "first got a got b true" },
    { "const out = []; const done = () => console.log(out.join(' ')); async function* g() { try { yield 1; } catch (e) { yield 'caught ' + e; } } (async () => { const it = g(); await it.next(); out.push((await it.throw('x')).value, (await it.next()).done); done(); })()",
      "caught x true" },
    { "async function* g() { yield 1; } console.log(Object.prototype.toString.call(g()), typeof g()[Symbol.asyncIterator], g()[Symbol.asyncIterator]() !== null, typeof g().next, g() instanceof Object)",
      "[object AsyncGenerator] function true function true" },
    { "const out = []; const done = () => console.log(out.join(' ')); async function* g() { yield Promise.resolve(1); return Promise.resolve(5); } (async () => { const it = g(); const a = await it.next(), b = await it.next(); out.push(a.value, b.value, b.done); const it2 = g(); await it2.next(); const c = await it2.return(Promise.resolve(7)); out.push(c.value, c.done); done(); })()",
      "1 5 true 7 true" },
    { "const log = []; async function* g() { yield 1; return 2; } const it = g(); it.next().then(r => log.push('a' + r.value)); it.next().then(r => log.push('b' + r.value)); Promise.resolve().then(() => log.push('m1')).then(() => log.push('m2')).then(() => log.push('m3')).then(() => log.push('m4')).then(() => log.push('m5')).then(() => console.log(log.join(' ')))",
      "m1 a1 m2 b2 m3 m4 m5" },
    { "const log = []; async function* g() { log.push('start'); yield 1; log.push('after1'); yield 2; } const it = g(); it.next().then(r => log.push('r' + r.value)); it.next().then(r => log.push('r' + r.value)); log.push('sync'); Promise.resolve().then(() => log.push('m1')).then(() => log.push('m2')).then(() => log.push('m3')).then(() => log.push('m4')).then(() => console.log(log.join(' ')))",
      "start sync after1 m1 r1 m2 r2 m3 m4" },
    { 0, 0 }
};

static const acase_t k_forawait[] = {
    { "const out = []; const done = () => console.log(out.join(' ')); (async () => { for await (const v of [Promise.resolve('a'), 'b', new Promise(r => queueMicrotask(() => r('c')))]) out.push(v); for await (const ch of 'hi') out.push(ch); done(); })()",
      "a b c h i" },
    { "const out = []; const done = () => console.log(out.join(' ')); const obj = { async *[Symbol.asyncIterator]() { yield 'x'; yield 'y'; } }; (async () => { for await (const v of obj) out.push(v); done(); })()",
      "x y" },
    { "const out = []; const done = () => console.log(out.join(' ')); class Feed { constructor(items) { this.items = items; } async *pages() { for (const it of this.items) yield await Promise.resolve(it.toUpperCase()); } } (async () => { for await (const p of new Feed(['p1', 'p2']).pages()) out.push(p); done(); })()",
      "P1 P2" },
    { "const out = []; const done = () => console.log(out.join(' ')); const manual = { i: 0, closed: false, [Symbol.asyncIterator]() { return { next: () => Promise.resolve(this.i < 5 ? { value: this.i++, done: false } : { done: true }), return: () => { this.closed = true; return Promise.resolve({ done: true }); } }; } }; (async () => { for await (const v of manual) { out.push('m' + v); if (v === 2) break; } out.push(manual.closed); done(); })()",
      "m0 m1 m2 true" },
    { "const out = []; const done = () => console.log(out.join(' ')); async function* src() { yield 1; yield 2; yield 3; } (async () => { outer: for await (const a of src()) { for await (const b of src()) { if (b === 2) continue outer; out.push(a + '' + b); } } done(); })()",
      "11 21 31" },
    { "const out = []; const done = () => console.log(out.join(' ')); (async () => { try { for await (const v of [Promise.reject(new Error('rej'))]) out.push(v); } catch (e) { out.push('caught ' + e.message); } try { for await (const v of { [Symbol.asyncIterator]: 1 }) out.push(v); } catch (e) { out.push(e.constructor.name); } done(); })()",
      "caught rej TypeError" },
    { "const out = []; const done = () => console.log(out.join(' ')); const { Readable } = { Readable: { from(arr) { return { async *[Symbol.asyncIterator]() { for (const x of arr) { await null; yield x; } } }; } } }; async function collect(it) { const r = []; for await (const x of it) r.push(x); return r; } collect(Readable.from([1, 2, 3])).then(r => { out.push(r.reduce((a, b) => a + b, 0)); done(); })",
      "6" },
    { 0, 0 }
};

void test_async_generators(void) { run_cases(k_agen); }
void test_for_await(void) { run_cases(k_forawait); }

void test_refused(void) {
    fresh();
    TEST_ASSERT_NOT_NULL(strstr(run("async function* g() { yield* [1, 2] }"), "yield* in async generators is not supported"));
    TEST_ASSERT_NOT_NULL(strstr(run("function f() { for await (const x of []) {} }"), "for await is only valid in async functions"));
    TEST_ASSERT_NOT_NULL(strstr(run("async function f() { for await (const x in {}) {} }"), "for await needs of"));
    TEST_ASSERT_NOT_NULL(strstr(run("async function f() { for await (;;) {} }"), "for await needs of"));
}

/* 1024 requests may wait on one generator; more is a RangeError */
void test_request_bound(void) {
    fresh();
    TEST_ASSERT_EQUAL_STRING("RangeError",
        run("async function* g() { await new Promise(() => {}); } const it = g(); "
            "try { for (let i = 0; i < 1100; i++) it.next(); } catch (e) { console.log(e.constructor.name) }"));
}

/* generators suspended at a yield and dropped: the collector (between
 * runs, like for Lot 114 generators) takes their coroutine stacks back, so
 * six rounds of six never hit the bound of JS_CO_MAX stacks at once */
void test_abandoned_reclaimed(void) {
    int i;
    char src[256];
    fresh();
    js_gc_config(&g_vm, 0, 1, 0);
    TEST_ASSERT_EQUAL_STRING("", run("var made = 0; async function* g() { made++; yield 1; yield 2; } 0;"));
    for (i = 0; i < 6; i++) {
        snprintf(src, sizeof(src), "for (let i = 0; i < 6; i++) g().next(); 0;");
        TEST_ASSERT_EQUAL_STRING("", run(src));
        js_gc(&g_vm);
        TEST_ASSERT_EQUAL(0, js_gc_check(&g_vm));
        TEST_ASSERT_EQUAL(0, g_vm.co_live);
    }
    TEST_ASSERT_EQUAL_STRING("36", run("console.log(made)"));
}

int main(void) {
    unity_init();
    RUN_TEST(test_async_generators);
    RUN_TEST(test_for_await);
    RUN_TEST(test_refused);
    RUN_TEST(test_request_bound);
    RUN_TEST(test_abandoned_reclaimed);
    unity_print_results();
    unity_cleanup();
    return unity_stats.tests_failed == 0 ? 0 : 1;
}
