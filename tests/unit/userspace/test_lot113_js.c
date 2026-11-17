/* Lot 113 (US-031): the JavaScript subset (wf_js*.c) on the host. Each
 * case is String(<program completion>) as node v24 prints it for the same
 * source (the expectations were checked against node); the last group
 * pins the documented limits of the subset and its guest bounds (step
 * budget, call depth, arena exhaustion). */
#include "../../framework/unity.h"
#include <string.h>
#include <stdio.h>
#include "wf_jsi.h"

#define ARENA (2u * 1024u * 1024u)
static unsigned char g_mem[ARENA];
static js_vm_t g_vm;
static char g_out[2048];

static void fresh(void) { TEST_ASSERT_EQUAL(0, js_init(&g_vm, g_mem, ARENA)); }

/* String(completion), or "!" + vm->err when the run did not return JS_OK */
static const char *ev(const char *src) {
    jv v;
    int rc;
    js_log_clear(&g_vm);
    js_set_budget(&g_vm, 2000000u);
    rc = js_eval(&g_vm, src, (int)strlen(src), &v);
    if (rc != JS_OK) { snprintf(g_out, sizeof(g_out), "!%s", js_last_error(&g_vm)); return g_out; }
    js_to_cstr(&g_vm, v, g_out, (int)sizeof(g_out));
    return g_out;
}

typedef struct { const char *src, *want; } jcase_t;

static void run_cases(const jcase_t *c) {
    int i;
    for (i = 0; c[i].src; i++) {
        const char *got;
        fresh();
        got = ev(c[i].src);
        if (strcmp(got, c[i].want) != 0) printf("  case %d: %s\n    got  %s\n    want %s\n", i, c[i].src, got, c[i].want);
        TEST_ASSERT_EQUAL_STRING(c[i].want, got);
    }
}

static const jcase_t k_lang[] = {
    { "1+2*3", "7" },
    { "0.1+0.2", "0.30000000000000004" },
    { "2**10", "1024" },
    { "7%3 + -7%3", "0" },
    { "[1/0, -1/0, 0/0].join()", "Infinity,-Infinity,NaN" },
    { "'5'*'2' + ',' + ('5'+2)", "10,52" },
    { "typeof null + typeof undefined + typeof 1 + typeof 'a' + typeof {} + typeof function(){}",
      "objectundefinednumberstringobjectfunction" },
    { "null == undefined && null !== undefined", "true" },
    { "NaN === NaN", "false" },
    { "let s=0; for (let i=0;i<10;i++) s+=i; s", "45" },
    { "var a=[]; for (var k in {x:1,y:2,z:3}) a.push(k); a.join('')", "xyz" },
    { "let t=0; for (const v of [1,2,3]) t+=v; t", "6" },
    { "function f(n){return n<2?n:f(n-1)+f(n-2)} f(15)", "610" },
    { "function mk(){let c=0;return ()=>++c} const g=mk(); g(); g(); g()", "3" },
    { "let {a, b: [c, d = 5], ...r} = {a:1, b:[2], e:3, f:4}; [a,c,d,Object.keys(r)].join('|')", "1|2|5|e,f" },
    { "const [x, , y = 9, ...z] = [1,2,undefined,4,5]; `${x}-${y}-${z}`", "1-9-4,5" },
    { "function h(a, b=a+1, ...c){return a+b+c.length} h(1) + h(1,2,3,4)", "8" },
    { "Math.max(...[1,5,3], 4)", "5" },
    { "class A{constructor(x){this.x=x} get dbl(){return this.x*2} static make(){return new A(3)}}"
      " class B extends A{constructor(){super(10)} get dbl(){return super.dbl+1}} A.make().dbl + new B().dbl", "27" },
    { "const o={a:{b:null}}; [o?.a?.b?.c, o.x?.y, o.a.b ?? 'd'].join(',')", ",,d" },
    { "let x=0; x ||= 5; x &&= x+1; x ?\?= 9; x", "6" },
    { "try { null.f } catch (e) { e instanceof TypeError }", "true" },
    { "let r; try { throw new RangeError('r') } catch ({name, message}) { r = name+':'+message } finally { r += '!' } r", "RangeError:r!" },
    { "function F(){ if (!(this instanceof F)) return new F(); this.v=1 } F().v", "1" },
    { "let r=''; switch (3) { case 1: r+='a'; break; case 3: r+='c'; default: r+='d' } r", "cd" },
    { "let n=0; outer: for (let i=0;i<5;i++){ for (let j=0;j<5;j++){ if (j==3) continue outer; if (i==3) break outer; n++ } } n", "9" },
    { "do { var q = 1 } while (false); q", "1" },
    { "(function(){ return arguments.length })(1,2,3)", "3" },
    { "var self = {n: 'o', f() { return [1].map(() => this.n)[0] }}; self.f()", "o" },
    { "function G(){} G.prototype.hi = function(){return 'hi'}; new G().hi()", "hi" },
    { "Object.getPrototypeOf([]) === Array.prototype", "true" },
    { "const o={_v:1, get v(){return this._v}, set v(x){this._v=x*10}}; o.v=2; o.v", "20" },
    { "Object.defineProperty({}, 'x', {get(){return 7}}).x", "7" },
    { "let i = 0; while (true) { if (++i > 4) break; } i", "5" },
    { "[0 || '' || null || 'x', 1 && 0 && 2].join()", "x,0" },
    { "(-7 >> 1) + ',' + (-7 >>> 28) + ',' + (5 & 3) + ',' + (5 | 3) + ',' + (5 ^ 3) + ',' + (~5)", "-4,15,1,7,6,-6" },
    { "var c = 0; function inc() { c++; return inc; } inc()()(); c", "3" },
    { "let fns = []; for (let i = 0; i < 3; i++) fns.push(() => i); fns.map(f => f()).join()", "0,1,2" },
    { "const fr = Object.freeze({a:1}); fr.a = 2; fr.a", "1" },
    { "class P { x = 1; static s = 'st'; m() { return this.x } } P.s + new P().m()", "st1" },
    { "var t = 'a'; { let t = 'b'; } t", "a" },
    { "[1,2,3].map((x, i, a) => a.length * x + i).join()", "3,7,11" },
    { "label: { break label; } 'after'", "after" },
    { 0, 0 }
};

static const jcase_t k_lib[] = {
    { "[3,1,2].sort().reverse().toString()", "3,2,1" },
    { "[10,9,1].sort((a,b)=>a-b).join()", "1,9,10" },
    { "'abc'.split('').reverse().join('')", "cba" },
    { "`a${1+1}b${'c'.toUpperCase()}`", "a2bC" },
    { "String([1,[2,[3]]].flat(Infinity))", "1,2,3" },
    { "[1,2,3,4].filter(x=>x%2).reduce((a,b)=>a+b, 10)", "14" },
    { "[1,2,3].includes(2) && ![1,2,3].includes('2')", "true" },
    { "Array.from({length:3}, (_, i) => i*i).join()", "0,1,4" },
    { "Object.entries({a:1,b:2}).map(([k,v])=>k+v).join()", "a1,b2" },
    { "JSON.stringify({a:[1,'x',null,true], b:{c:undefined}, d:1.5})", "{\"a\":[1,\"x\",null,true],\"b\":{},\"d\":1.5}" },
    { "JSON.parse('{\"x\":[1,2,{\"y\":\"z\"}]}').x[2].y", "z" },
    { "JSON.stringify([new Date(0)])", "[\"1970-01-01T00:00:00.000Z\"]" },
    { "JSON.stringify({b:[1,{c:2}]}, null, 2)", "{\n  \"b\": [\n    1,\n    {\n      \"c\": 2\n    }\n  ]\n}" },
    { "(255).toString(16) + (0.5).toFixed(2) + (1234.5678).toFixed(1)", "ff0.501234.6" },
    { "parseInt('0x1f') + parseInt('12px') + parseFloat('3.5e1abc')", "78" },
    { "Number('  42 ') + Number('') + (Number('x') ? 1 : 0)", "42" },
    { "'Hello World'.replace(/o/g, '0')", "Hell0 W0rld" },
    { "'2026-11-17'.replace(/(\\d+)-(\\d+)-(\\d+)/, '$3/$2/$1')", "17/11/2026" },
    { "/^(?<user>\\w+)@(?<host>[\\w.]+)$/.exec('ann@ex.org').groups.host", "ex.org" },
    { "'a1b22c333'.match(/\\d+/g).join('|')", "1|22|333" },
    { "'aBc'.toLowerCase() + 'x'.padStart(3, '-') + '  t '.trim()", "abc--xt" },
    { "'a,b,,c'.split(',').length", "4" },
    { "Math.round(2.5) + Math.round(-2.5) + Math.floor(-1.5) + Math.ceil(1.2) + Math.trunc(-4.7)", "-3" },
    { "Math.abs(-3) + Math.sqrt(16) + Math.hypot(3,4)", "12" },
    { "new Map([[1,'a'],[2,'b']]).get(2) + new Set([1,1,2]).size", "b2" },
    { "const m = new Map(); m.set('k', 1).set('j', 2); [...m.keys()].join() + m.has('k')", "k,jtrue" },
    { "Object.assign({}, {a:1}, {b:2}).b", "2" },
    { "'abc'.at(-1) + [1,2,3].at(-1)", "c3" },
    { "encodeURIComponent('a b&c/\xC3\xA9')", "a%20b%26c%2F%C3%A9" },
    { "decodeURIComponent('%E2%82%AC')", "\xE2\x82\xAC" },
    { "new Date(Date.UTC(2020, 1, 29, 12)).toISOString()", "2020-02-29T12:00:00.000Z" },
    { "new Date('2026-11-17T10:20:30Z').getTime()", "1794910830000" },
    { "(123.456).toPrecision(4) + ' ' + (1e21).toString() + ' ' + (1e-7).toString()", "123.5 1e+21 1e-7" },
    { "[0, -0, NaN].map(x => Object.is(x, -0)).join()", "false,true,false" },
    { "Number.MAX_SAFE_INTEGER + 2", "9007199254740992" },
    { "String(1/3) + ' ' + String(123e-20)", "0.3333333333333333 1.23e-18" },
    { "'x'.repeat(3) + 'abc'.slice(-2) + 'abcdef'.substring(4,1)", "xxxbcbcd" },
    { "[1,2,3,4,5].splice(1,2).join()", "2,3" },
    { "let arr=[1,2,3,4,5]; arr.splice(1,2,'a'); arr.join()", "1,a,4,5" },
    { "[5,1,4].indexOf(4) + [1,2,1].lastIndexOf(1) + [1,2].findIndex(x=>x==2)", "5" },
    { "[1,[2,3]].toString() + String(null) + String(undefined) + String({})", "1,2,3nullundefined[object Object]" },
    { "new Error('boom').message + (new TypeError('t') instanceof Error)", "boomtrue" },
    { "isNaN('abc') + ',' + Number.isNaN('abc') + ',' + Number.isInteger(5.0)", "true,false,true" },
    { "'%' + 'abc'.charCodeAt(1) + String.fromCharCode(72, 105)", "%98Hi" },
    { "[...'ab'].concat([1],[2,[3]]).length", "5" },
    { "Array.isArray([]) && !Array.isArray({length:0})", "true" },
    { "new Array(3).fill(0).join('-') + Array.of(7,8).join()", "0-0-07,8" },
    { "[1,2,3].some(x=>x>2) + '' + [1,2,3].every(x=>x>0)", "truetrue" },
    { "/a.c/s.test('a\\nc') + '' + /A/i.test('a') + /^b/m.test('a\\nb')", "truetruetrue" },
    { "'x-y_z'.replace(/[-_](\\w)/g, (m, c) => c.toUpperCase())", "xYZ" },
    { "'aaa'.lastIndexOf('a') + 'abc'.indexOf('c') + 'abc'.search(/c/)", "6" },
    { "const re=/o/g; let c=0; while (re.exec('foo boo')) c++; c", "4" },
    { "[...'a1b2'.matchAll(/[a-z](\\d)/g)].map(m=>m[1]).join()", "1,2" },
    { "'2,3;4'.split(/[,;]/).map(Number).reduce((a, b) => a * b)", "24" },
    { "/(a+)+b/.test('aaaaaaaaaaaaaaaaaaab')", "true" },
    { "/(?<=\\$)\\d+/.exec('cost $42')[0] + /\\d+(?!px)/.exec('12px 34em')[0]", "421" },
    { "/(\\w)\\1/.exec('abccd')[0] + /[^a-c]+/.exec('abcdef')[0]", "ccdef" },
    { "'Tab\\tEnd'.replace(/\\s/, '_') + 'A-B'.split(/(-)/).length", "Tab_End3" },
    { "(12345.678).toLocaleString()", "12,345.678" },
    { "(0.000001234).toExponential(2) + ' ' + (5e-324 > 0)", "1.23e-6 true" },
    { "Math.pow(2, 53) === 2**53 && Math.sign(-3) === -1", "true" },
    { "JSON.stringify(JSON.parse('[1e3, -0.5, \"\\\\u00e9\"]'))", "[1000,-0.5,\"\xC3\xA9\"]" },
    { "Object.keys('ab').join() + Object.values({x:1,y:[2]}).length", "0,12" },
    { "[3, 20, 100].sort().join()", "100,20,3" },
    { "new Date(2024, 0, 31).getMonth() + new Date(0).getUTCFullYear()", "1970" },
    { "Array.prototype.slice.call({0:'a', 1:'b', length:2}).join('')", "ab" },
    { "Function.prototype.call.call(function(){ return this.v }, {v: 'cv'})", "cv" },
    { "(function(a, b){ return this.p + a + b }).bind({p: 1}, 2)(3)", "6" },
    { "Math.max.apply(null, [4, 9, 2])", "9" },
    { 0, 0 }
};

static void test_l113_language(void) { run_cases(k_lang); }
static void test_l113_library(void) { run_cases(k_lib); }

static void test_l113_console(void) {
    int n;
    const char *lg;
    fresh();
    TEST_ASSERT_EQUAL_STRING("undefined", ev("console.log('a', 1, [1, 'b'], {k: 'v'}, null); console.warn('w')"));
    lg = js_log_text(&g_vm, &n);
    TEST_ASSERT_EQUAL_STRING("a 1 [ 1, 'b' ] { k: 'v' } null\nwarn: w\n", lg);
    TEST_ASSERT_EQUAL((int)strlen(lg), n);
}

static void test_l113_display(void) {
    jv v;
    char b[256];
    const char *src = "({a: [1, 'x'], f() {}, n: null})";
    fresh();
    TEST_ASSERT_EQUAL(JS_OK, js_eval(&g_vm, src, (int)strlen(src), &v));
    js_display(&g_vm, v, b, (int)sizeof(b), 1);
    TEST_ASSERT_EQUAL_STRING("{ a: [ 1, 'x' ], f: [Function: f], n: null }", b);
    TEST_ASSERT_EQUAL(JS_OK, js_eval(&g_vm, "'q'", 3, &v));
    js_display(&g_vm, v, b, (int)sizeof(b), 1);
    TEST_ASSERT_EQUAL_STRING("'q'", b);
}

/* one realm keeps its globals across runs (the js REPL) and calls back */
static void test_l113_realm(void) {
    jv f, r, arg = jv_int(20);
    fresh();
    TEST_ASSERT_EQUAL_STRING("undefined", ev("var total = 1; function add(n) { total += n; return total; }"));
    TEST_ASSERT_EQUAL_STRING("6", ev("add(5)"));
    TEST_ASSERT_EQUAL(JS_OK, js_eval(&g_vm, "add", 3, &f));
    TEST_ASSERT_EQUAL(JS_OK, js_call_value(&g_vm, f, jv_undef(), &arg, 1, &r));
    TEST_ASSERT_EQUAL(26, r.a);
    TEST_ASSERT_EQUAL_STRING("26", ev("total"));
}

static void test_l113_errors(void) {
    fresh();
    TEST_ASSERT_EQUAL_STRING("!Uncaught ReferenceError: nope is not defined", ev("nope + 1"));
    TEST_ASSERT_EQUAL_STRING("!Uncaught TypeError: Cannot read properties of undefined (reading 'x')", ev("undefined.x"));
    TEST_ASSERT_EQUAL_STRING("!Uncaught { a: 1 }", ev("throw {a: 1}"));
    TEST_ASSERT_EQUAL_STRING("!Uncaught 'str'", ev("throw 'str'"));
    TEST_ASSERT_EQUAL_STRING("caught", ev("try { (void 0)(); } catch (e) { 'caught' }"));
    TEST_ASSERT_EQUAL('!', ev("let = ;")[0]);
    TEST_ASSERT_NOT_NULL(strstr(ev("let = ;"), "SyntaxError"));
    TEST_ASSERT_EQUAL_STRING("TypeError", ev("try { new (class X { constructor() { null.y } })() } catch (e) { e.name }"));
    TEST_ASSERT_EQUAL_STRING("!Uncaught TypeError: Assignment to constant variable.", ev("const k = 1; k = 2"));
    /* the realm still works after errors */
    TEST_ASSERT_EQUAL_STRING("3", ev("1 + 2"));
}

/* guest bounds: none of these may hang or crash; all end the run */
static void test_l113_bounds(void) {
    const char *r;
    fresh();
    r = ev("for (;;) {}");
    TEST_ASSERT_NOT_NULL(strstr(r, "step budget"));
    r = ev("function rec(n) { return rec(n + 1) + 1 } rec(0)");
    TEST_ASSERT_EQUAL('!', r[0]);
    TEST_ASSERT_NOT_NULL(strstr(r, "call stack"));
    TEST_ASSERT_EQUAL_STRING("RangeError", ev("try { (function r(){ r() })() } catch (e) { e.name }"));
    /* the arena: a fresh 2 MiB realm fills and aborts, then a new realm is fine */
    js_set_budget(&g_vm, 50000000u);
    {
        jv v;
        const char *src = "let a = []; for (;;) a.push('x'.repeat(4096) + a.length)";
        int rc = js_eval(&g_vm, src, (int)strlen(src), &v);
        TEST_ASSERT_EQUAL(JS_ABORTED, rc);
        TEST_ASSERT_TRUE(js_mem_used(&g_vm) <= js_mem_size(&g_vm));
    }
    fresh();
    TEST_ASSERT_EQUAL_STRING("ok", ev("'o' + 'k'"));
    /* a catastrophic regex is bounded by the same budget */
    r = ev("/(a*)*b/.test('a'.repeat(40))");
    TEST_ASSERT_TRUE(strcmp(r, "false") == 0 || strstr(r, "budget") != 0);
    TEST_ASSERT_EQUAL(-1, js_init(&g_vm, g_mem, 64u * 1024u));      /* too small an arena is refused */
}

/* the documented limits of the subset (docs/lot113_plan.md): pinned so a
 * change of behaviour is a conscious one */
static void test_l113_limits(void) {
    fresh();
    TEST_ASSERT_EQUAL_STRING("undefined", ev("typeof Symbol"));
    TEST_ASSERT_EQUAL_STRING("undefined", ev("typeof Promise"));
    TEST_ASSERT_EQUAL_STRING("!Uncaught ReferenceError: eval is not defined", ev("eval('1')"));
    TEST_ASSERT_EQUAL_STRING("EvalError", ev("try { Function('return 1') } catch (e) { e.name }"));
    TEST_ASSERT_EQUAL_STRING("true", ev("Array.isArray(new Map([[1, 2]]).keys())"));
    TEST_ASSERT_EQUAL_STRING("2026", ev("new Date().getUTCFullYear()"));         /* fixed clock */
    TEST_ASSERT_EQUAL_STRING("true", ev("Math.random() === Math.random() ? false : true"));
    TEST_ASSERT_EQUAL('!', ev("function* g() { yield 1 }")[0]);
    TEST_ASSERT_EQUAL('!', ev("async function a() {}")[0]);
    TEST_ASSERT_NOT_NULL(strstr(ev("class P { #x = 1 }"), "private class members"));
}

int main(void) {
    unity_init();
    RUN_TEST(test_l113_language);
    RUN_TEST(test_l113_library);
    RUN_TEST(test_l113_console);
    RUN_TEST(test_l113_display);
    RUN_TEST(test_l113_realm);
    RUN_TEST(test_l113_errors);
    RUN_TEST(test_l113_bounds);
    RUN_TEST(test_l113_limits);
    unity_print_results();
    unity_cleanup();
    return unity_stats.tests_failed == 0 ? 0 : 1;
}
