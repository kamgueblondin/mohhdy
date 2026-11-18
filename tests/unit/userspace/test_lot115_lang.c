/* Lot 115 (US-031): the language gaps Lot 114 left in the JavaScript
 * subset, on the host. Symbol and the well-known symbols the runtime
 * honours (wf_jssym.c), the iterator protocol (user [Symbol.iterator],
 * lazy for-of with return() on an early exit, destructuring that reads
 * only what it needs, keys()/values()/entries() iterators), class
 * private members (#x fields, methods, accessors, static ones, #x in o)
 * and static blocks, and UTF-16 string semantics over the UTF-8 storage
 * (wf_jsu16.c). Each case runs a script and its microtasks and compares
 * the console lines joined with '|' with what node v24 prints for the
 * same source (the expectations were produced by node). The last tests
 * pin what the guest refuses, the caches and the bounds. */
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

static const acase_t k_symbol[] = {
    { "const s = Symbol('foo'), t = Symbol('foo'); console.log(typeof s, s === t, s.toString(), "
      "String(s), s.description, Symbol().description)",
      "symbol false Symbol(foo) Symbol(foo) foo undefined" },
    { "const s = Symbol('k'); const o = { a: 1, [s]: 2 }; console.log(Object.keys(o).join(), o[s], "
      "JSON.stringify(o), Object.getOwnPropertySymbols(o).length, s in o)",
      "a 2 {\"a\":1} 1 true" },
    { "console.log(Symbol.for('k') === Symbol.for('k'), Symbol.keyFor(Symbol.for('k')), "
      "Symbol.keyFor(Symbol('k')), Symbol.for('k') === Symbol('k'))",
      "true k undefined false" },
    { "const s = Symbol('x'); try { '' + s } catch (e) { console.log(e.constructor.name, e.message) } "
      "try { +s } catch (e) { console.log(e.constructor.name) } try { new Symbol() } catch (e) { "
      "console.log(e.constructor.name) }",
      "TypeError Cannot convert a Symbol value to a string|TypeError|TypeError" },
    { "const s = Symbol('d'); const o = {}; o[s] = 1; for (const k in o) console.log('key', k); "
      "console.log(Object.assign({}, o)[s], Object.entries(o).length, "
      "Object.getOwnPropertyNames(o).length)",
      "1 0 0" },
    { "const s = Symbol('w'); const w = Object(s); console.log(typeof w, w.valueOf() === s, "
      "w.description, w == s, Object.prototype.toString.call(s))",
      "object true w true [object Symbol]" },
    { "console.log(typeof Symbol.iterator, Symbol.iterator.toString(), "
      "Symbol.asyncIterator.description, Symbol.hasInstance === Symbol.hasInstance)",
      "symbol Symbol(Symbol.iterator) Symbol.asyncIterator true" },
    { "const s = Symbol('1'); const a = []; a[s] = 5; console.log(a.length, a[s], "
      "Object.keys(a).length)",
      "0 5 0" },
    { "const m = new Map(); const s1 = Symbol('a'), s2 = Symbol('a'); m.set(s1, 1).set(s2, 2); "
      "console.log(m.size, m.get(s1), m.get(s2), new Set([s1, s1, s2]).size)",
      "2 1 2 2" },
    { "const s = Symbol('d'); console.log(s, [s], { k: s }, JSON.stringify({ k: s, n: 1 }), "
      "JSON.stringify([s]))",
      "Symbol(d) [ Symbol(d) ] { k: Symbol(d) } {\"n\":1} [null]" },
    { "const o = { [Symbol.toStringTag]: 'Custom' }; console.log(String(o), "
      "Object.prototype.toString.call(new Map()), Object.prototype.toString.call(function*(){}()))",
      "[object Custom] [object Map] [object Generator]" },
    { "const tp = { [Symbol.toPrimitive](h) { return h === 'number' ? 42 : 'str:' + h } }; "
      "console.log(+tp, `${tp}`, tp + '', tp * 2, tp > 41, [tp] + '')",
      "42 str:string str:default 84 true str:string" },
    { "class Even { static [Symbol.hasInstance](n) { return n % 2 === 0 } } console.log(2 instanceof "
      "Even, 3 instanceof Even)",
      "true false" },
    { "const d = new Date(0); console.log(typeof (d - 0), typeof (d + ''), d < 1)",
      "number string true" },
    { "const s = Symbol('sp'); const o = { [s]: 1, a: 2 }; const c = { ...o }; console.log(c[s], "
      "Object.keys(c).join(), JSON.stringify({ ...'\\u00e9\\u{1F600}' }).length)",
      "1 a 35" },
    { 0, 0 }
};

static const acase_t k_iter[] = {
    { "class R { constructor(n) { this.n = n } *[Symbol.iterator]() { for (let i = 0; i < this.n; i++) "
      "yield i } } console.log([...new R(3)].join(), Array.from(new R(2)).join(), Math.max(...new "
      "R(5)))",
      "0,1,2 0,1 4" },
    { "const it = { from: 1, to: 4, [Symbol.iterator]() { let c = this.from, t = this.to; return { "
      "next: () => c <= t ? { value: c++, done: false } : { value: undefined, done: true }, return() { "
      "console.log('closed'); return {} } } } }; for (const x of it) { if (x > 2) break; "
      "console.log('x', x) } const [p, q] = it; console.log(p, q); console.log([...it].join())",
      "x 1|x 2|closed|closed|1 2|1,2,3,4" },
    { "const inf = { [Symbol.iterator]() { let i = 0; return { next() { return { value: i++, done: "
      "false } }, return() { console.log('ret'); return {} } } } }; const [a, b, c] = inf; "
      "console.log(a, b, c)",
      "ret|0 1 2" },
    { "function* g() { let i = 0; try { for (;;) yield i++ } finally { console.log('fin') } } const [x, "
      "y] = g(); console.log(x, y)",
      "fin|0 1" },
    { "const o = { [Symbol.iterator]() { return { i: 0, next() { return this.i < 2 ? { value: this.i++, "
      "done: false } : { done: true } } } } }; for (const v of o) { try { throw new Error('in ' + v) } "
      "catch (e) { console.log(e.message) } } console.log(new Set(o).size, new Map([[1, 2]]).size)",
      "in 0|in 1|2 1" },
    { "const bad = { [Symbol.iterator]() { return 5 } }; try { for (const v of bad) {} } catch (e) { "
      "console.log(e.constructor.name) } try { [...{ [Symbol.iterator]: 1 }] } catch (e) { "
      "console.log(e.constructor.name) }",
      "TypeError|TypeError" },
    { "console.log([1, 2, 3][Symbol.iterator]().next().value, typeof [][Symbol.iterator], "
      "[][Symbol.iterator] === [].values, ''[Symbol.iterator]().next().done)",
      "1 function true true" },
    { "const e = [5, 6].entries(); console.log(JSON.stringify(e.next()), JSON.stringify([...e]), "
      "JSON.stringify(e.next()))",
      "{\"value\":[0,5],\"done\":false} [[1,6]] {\"done\":true}" },
    { "const k = ['a', 'b'].keys(); console.log(typeof k.next, k[Symbol.iterator]() === k, "
      "[...k].join(), Array.from([7, 8].values()).join())",
      "function true 0,1 7,8" },
    { "const m = new Map([[1, 'a'], [2, 'b']]); console.log(JSON.stringify([...m]), "
      "JSON.stringify([...m.keys()]), m[Symbol.iterator] === m.entries, "
      "JSON.stringify(m.values().next()))",
      "[[1,\"a\"],[2,\"b\"]] [1,2] true {\"value\":\"a\",\"done\":false}" },
    { "const st = new Set(['x', 'y']); const vi = st.values(); console.log(vi.next().value, "
      "[...vi].join(), st[Symbol.iterator] === st.values, JSON.stringify([...st.entries()]))",
      "x y true [[\"x\",\"x\"],[\"y\",\"y\"]]" },
    { "const ret = []; const it = { [Symbol.iterator]() { return { next() { return { value: 1, done: "
      "false } }, return() { ret.push('r'); return {} } } } }; function f() { for (const v of it) "
      "return v } console.log(f(), ret.join()); outer: for (const a of [1, 2]) { for (const v of it) "
      "continue outer } console.log(ret.length)",
      "1 r|3" },
    { "const arr = [1, 2]; arr[Symbol.iterator] = function* () { yield 'own' }; "
      "console.log([...arr].join(), Array.from(arr).join())",
      "own own" },
    { 0, 0 }
};

static const acase_t k_private[] = {
    { "class Counter { #count = 0; static #instances = 0; #secret; constructor(s) { this.#secret = s; "
      "Counter.#instances++ } inc() { this.#count++; return this } get value() { return this.#count } "
      "static count() { return Counter.#instances } peek(o) { return o.#secret } } const c = new "
      "Counter('s1'); c.inc().inc(); new Counter('x'); console.log(c.value, Counter.count(), c.peek(c), "
      "Object.keys(c).length, JSON.stringify(c), Object.getOwnPropertySymbols(c).length)",
      "2 2 s1 0 {} 0" },
    { "class A { #x = 1; peek(o) { return o.#x } } try { new A().peek({}) } catch (e) { "
      "console.log(e.constructor.name, e.message) }",
      "TypeError Cannot read private member #x from an object whose class did not declare it" },
    { "class A { #hidden() { return 'h' } call() { return this.#hidden() } tryWrite() { try { "
      "this.#hidden = 1 } catch (e) { return e.constructor.name } } } const a = new A(); "
      "console.log(a.call(), a.tryWrite(), typeof a.hidden)",
      "h TypeError undefined" },
    { "class A { #n = 3; get #dbl() { return this.#n * 2 } set #dbl(v) { this.#n = v / 2 } run() { "
      "this.#dbl = 10; return this.#dbl + ':' + this.#n } } console.log(new A().run())",
      "10:5" },
    { "class A { #x; static is(o) { return #x in o } } class B extends A {} console.log(A.is(new A()), "
      "A.is(new B()), A.is({}))",
      "true true false" },
    { "class A { #x = 1; getX(o) { return o?.#x } } console.log(new A().getX(new A()), new "
      "A().getX(null), new A().getX(undefined))",
      "1 undefined undefined" },
    { "class A { #x = 'a'; ax() { return this.#x } } class B extends A { #x = 'b'; bx() { return "
      "this.#x } } const b = new B(); console.log(b.ax(), b.bx())",
      "a b" },
    { "function make() { return class { #p = 1; static has(o) { return #p in o } } } const K1 = make(), "
      "K2 = make(); console.log(K1.has(new K1()), K1.has(new K2()))",
      "true false" },
    { "class C { #n = 0; inc() { return ++this.#n } add() { this.#n += 5; return this.#n-- } swap(o) { "
      "[this.#n, o.#n] = [o.#n, this.#n]; return this.#n } } const z = new C(); const w = new C(); "
      "w.inc(); console.log(z.inc(), z.add(), z.inc(), z.swap(w))",
      "1 6 6 1" },
    { "class O { #a = 'outer'; m() { class I { n(o) { return o.#a } } return new I().n(this) } } "
      "console.log(new O().m())",
      "outer" },
    { "class S { static #count = 0; static #bump() { return ++S.#count } static run() { S.#bump(); "
      "return S.#bump() } } console.log(S.run())",
      "2" },
    { "class F { #v = 1; set(x) { this.#v = x; return this.#v } } const o = Object.freeze(new F()); "
      "console.log(o.set(5), Object.isFrozen(o))",
      "5 true" },
    { "class Base { constructor(o) { return o } } class Stamp extends Base { #s = 'stamped'; static "
      "get(o) { return o.#s } } const t = {}; new Stamp(t); console.log(Stamp.get(t)); try { new "
      "Stamp(t) } catch (e) { console.log(e.constructor.name) }",
      "stamped|TypeError" },
    { "class M { #m() { return this.constructor.name } static t(o) { return o.#m() } } class N extends "
      "M {} console.log(M.t(new N()))",
      "N" },
    { "class A { constructor() { return { tag: 'plain' } } } class B extends A { #f = 1; constructor() "
      "{ super(); this.after = 2 } static has(o) { return #f in o } } const b = new B(); "
      "console.log(b.tag, b.after, B.has(b), b instanceof B)",
      "plain 2 true false" },
    { 0, 0 }
};

static const acase_t k_static[] = {
    { "let order = []; class S { static a = (order.push('a'), 1); static { order.push('block ' + "
      "this.a); var v = 3; S.b = v } static c = (order.push('c'), 3) } console.log(order.join(), S.b, "
      "typeof v)",
      "a,block 1,c 3 undefined" },
    { "class P { static #secret = 42; static { P.revealed = P.#secret } } console.log(P.revealed)",
      "42" },
    { "class Q { static { this.x = 1 } static { this.y = this.x + 1 } } console.log(Q.x, Q.y)",
      "1 2" },
    { 0, 0 }
};

static const acase_t k_utf16[] = {
    { "const s = 'h\\u00e9llo \\u{1F600}!'; console.log(s.length, s[1], s.charCodeAt(1), s.charCodeAt(7), "
      "s.charCodeAt(8), s.codePointAt(7), s.codePointAt(8))",
      "9 \303\251 233 56832 33 56832 33" },
    { "const s = 'h\\u00e9llo \\u{1F600}!'; console.log(s.indexOf('l'), s.indexOf('!'), "
      "s.lastIndexOf('l'), s.slice(-1), s.substring(1, 3), s.substr(-3, 2).length, s.slice(7).length)",
      "2 8 3 ! \303\251l 2 2" },
    { "const s = '\\u{1F600}'; const hi = s[0], lo = s[1]; console.log(hi.length, (hi + lo) === s, "
      "`${hi}${lo}` === s, [hi, lo].join('') === s, (s.slice(0, 1) + s.slice(1)).length)",
      "1 true true true 2" },
    { "console.log(String.fromCharCode(0xD83D, 0xDE00) === '\\u{1F600}', String.fromCharCode(233, 72), "
      "JSON.stringify('\\u{1F600}'[0]), JSON.stringify('\\u{1F600}'), "
      "String.fromCodePoint(0x1F600).length)",
      "true \303\251H \"\\ud83d\" \"\360\237\230\200\" 2" },
    { "console.log('\\u00e9t\\u00e9'.split('').length, '\\u{1F600}'.split('').length, "
      "[...'\\u{1F600}a'].length, 'ab\\u00e9'.padStart(6, '\\u00e9').length, 'x'.padEnd(4, 'ab'), "
      "'x'.padStart(3, '\\u{1F600}').length)",
      "3 2 2 6 xaba 3" },
    { "console.log('\\u00e9a\\u00e9'.replace(/a/, (m, off) => off), '\\u00e9\\u00e9'.search(/\\u00e9$/), "
      "/b/g.exec('\\u00e9b').index, '\\u00e9-\\u00e9'.match(/-/).index)",
      "\303\2511\303\251 1 1 1" },
    { "const re = /o/g; re.exec('\\u00e9o\\u00e9o'); console.log(re.lastIndex, "
      "re.exec('\\u00e9o\\u00e9o').index, re.lastIndex, [...'\\u00e9o\\u00e9o'.matchAll(/o/g)].map(m => "
      "m.index).join())",
      "2 3 4 1,3" },
    { "console.log('\\u00e9'.replace(/(?:)/g, '-'), 'a\\u00e9'.at(-1), '\\u00e9t\\u00e9'.startsWith('t', "
      "1), '\\u00e9t\\u00e9'.endsWith('t', 2), '\\u00e9t\\u00e9'.includes('\\u00e9', 1), "
      "'\\u00e9\\u00e9'.split(/(?:)/).length)",
      "-\303\251- \303\251 true true true 2" },
    { "const s = 'h\\u00e9llo \\u{1F600}!'; let n = 0; for (let i = 0; i < s.length; i++) n += "
      "s.charCodeAt(i); console.log(n, Object.keys('\\u00e9\\u{1F600}').join(), new "
      "String('\\u00e9\\u00e9').length)",
      "112918 0,1,2 2" },
    { "console.log('\\u00e9t\\u00e9 \\u0101\\u013a\\u017e \\u03b1\\u03c9\\u03c2 \\u0436\\u0451 "
      "\\u00ff'.toUpperCase(), '\\u00c9T\\u00c9 \\u0100\\u0139\\u017d \\u0391\\u03a9 \\u0416\\u0401 "
      "\\u0178'.toLowerCase())",
      "\303\211T\303\211 \304\200\304\271\305\275 \316\221\316\251\316\243 \320\226\320\201 \305\270 \303\251t\303\251 \304\201\304\272\305\276 \316\261\317\211 \320\266\321\221 \303\277" },
    { "const big = '\\u00e9'.repeat(3000); let c = 0; for (let i = 0; i < big.length; i++) if (big[i] "
      "=== '\\u00e9') c++; console.log(big.length, c, big.lastIndexOf('\\u00e9'))",
      "3000 3000 2999" },
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
    run_cases_shared(k_symbol);
    run_cases_shared(k_iter);
    run_cases_shared(k_private);
    run_cases_shared(k_static);
    run_cases_shared(k_utf16);
}

static void test_symbol(void) { run_cases(k_symbol); }
static void test_iterator_protocol(void) { run_cases(k_iter); }
static void test_private_members(void) { run_cases(k_private); }
static void test_static_blocks(void) { run_cases(k_static); }
static void test_utf16_strings(void) { run_cases(k_utf16); }

/* early errors of private names: node refuses the same sources */
static void test_private_refused(void) {
    static const char *const src[] = {
        "class A { m() { return this.#y } }",
        "class A { #x; m() { delete this.#x } }",
        "class A { #constructor() {} }",
        "class A { #x; #x }",
        "this.#x",
        "class A { #x; m() { return #x } }",
        "class A { get #a() {} get #a() {} }",
        0
    };
    static const char *const want[] = {
        "!SyntaxError: Private field must be declared in an enclosing class (line 1)",
        "!SyntaxError: Private fields can not be deleted (line 1)",
        "!SyntaxError: Classes may not have a private field named '#constructor' (line 1)",
        "!SyntaxError: Identifier has already been declared (private name) (line 1)",
        "!SyntaxError: Private field must be declared in an enclosing class (line 1)",
        "!SyntaxError: Unexpected private name (only #x in obj stands alone) (line 1)",
        "!SyntaxError: Identifier has already been declared (private name) (line 1)",
    };
    int i;
    for (i = 0; src[i]; i++) {
        fresh();
        TEST_ASSERT_EQUAL_STRING(want[i], run(src[i]));
    }
}

/* a private name is not a property name: no listing, no reflection, and
 * the symbol behind it never reaches a script */
static void test_private_hidden(void) {
    fresh();
    TEST_ASSERT_EQUAL_STRING("0 0 0 {} undefined",
        run("class A { #p = 1; #m() {} } const a = new A();"
            "console.log(Object.keys(a).length, Object.getOwnPropertyNames(a).length,"
            " Object.getOwnPropertySymbols(a).length, JSON.stringify({ ...a }), Object.assign({}, a).p)"));
}

/* the UTF-16 count is cached in the string record and survives interning */
static void test_utf16_cache(void) {
    uint32_t s, k;
    fresh();
    s = js_str_new(&g_vm, "a\xc3\xa9\xf0\x9f\x98\x80", 7);
    TEST_ASSERT_EQUAL(4, js_str_units(&g_vm, s));
    TEST_ASSERT_TRUE((JSTR(&g_vm, s)->interned & JS_U16_INFO) != 0);
    TEST_ASSERT_EQUAL(0, JSTR(&g_vm, s)->interned & JS_STR_KIND_MASK);
    k = js_intern_str(&g_vm, s);
    TEST_ASSERT_EQUAL(1, JSTR(&g_vm, k)->interned & JS_STR_KIND_MASK);
    TEST_ASSERT_EQUAL(4, js_str_units(&g_vm, k));
    TEST_ASSERT_EQUAL(k, js_intern_str(&g_vm, k));
    TEST_ASSERT_EQUAL(0xD83D, js_u16_at(&g_vm, s, 2));
    TEST_ASSERT_EQUAL(0xDE00, js_u16_at(&g_vm, s, 3));
    TEST_ASSERT_EQUAL(-1, js_u16_at(&g_vm, s, 4));
    TEST_ASSERT_EQUAL(3, js_u16_off(&g_vm, s, 2, 0));
    TEST_ASSERT_EQUAL(2, js_u16_unit_of(&g_vm, s, 3));
    /* an invalid byte is one unit whose value is the byte */
    s = js_str_new(&g_vm, "x\xff" "y", 3);
    TEST_ASSERT_EQUAL(3, js_str_units(&g_vm, s));
    TEST_ASSERT_EQUAL(0xFF, js_u16_at(&g_vm, s, 1));
}

/* a lone surrogate reaches the console as U+FFFD (the log stays UTF-8) */
static void test_utf16_log_valid(void) {
    fresh();
    TEST_ASSERT_EQUAL_STRING("\357\277\275|1", run("const h = '\\u{1F600}'[0]; console.log(h); console.log(h.length)"));
}

/* still refused, honestly: async generators and for await */
static void test_still_refused(void) {
    fresh();
    TEST_ASSERT_EQUAL_STRING("!SyntaxError: async generators are not supported by the guest (line 1)",
                             run("async function* g() {}"));
}

/* Symbol() costs one record in the arena: a loop that makes them runs
 * into the arena bound (an abort, not a crash) */
static void test_symbol_bound(void) {
    const char *got;
    fresh();
    js_set_budget(&g_vm, 50000000u);
    got = run("const keep = []; for (;;) keep.push(Symbol('x'.repeat(200)))");
    TEST_ASSERT_TRUE(strstr(got, "out of memory") != 0 || strstr(got, "budget") != 0);
}

int main(void) {
    unity_init();
    RUN_TEST(test_symbol);
    RUN_TEST(test_iterator_protocol);
    RUN_TEST(test_private_members);
    RUN_TEST(test_static_blocks);
    RUN_TEST(test_utf16_strings);
    RUN_TEST(test_private_refused);
    RUN_TEST(test_private_hidden);
    RUN_TEST(test_utf16_cache);
    RUN_TEST(test_utf16_log_valid);
    RUN_TEST(test_still_refused);
    RUN_TEST(test_symbol_bound);
    RUN_TEST(test_shared_realm_reclaim);
    unity_print_results();
    unity_cleanup();
    return unity_stats.tests_failed == 0 ? 0 : 1;
}
