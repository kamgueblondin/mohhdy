/* Lot 114 (US-031): history and structuredClone of the page realm
 * (wf_jshist.c). The structuredClone cases are equal to node v24.22;
 * history follows the HTML standard for one document: pushState /
 * replaceState move location in place, other origins are refused,
 * traversals are tasks that fire popstate (and hashchange). */
#include "../../framework/unity.h"
#include <string.h>
#include <stdio.h>
#include "wf_jsdom.h"
#include "wf_jshist.h"

#define ARENA (6u * 1024u * 1024u)
static unsigned char g_arena[ARENA] __attribute__((aligned(16)));
static char g_out[256 * 1024];
static char g_html[8192];
static wf_jspage_stats_t g_st;
static int g_rc;
static char g_last_url[256];

static int hook(void *ctx, wf_jsreq_t *r) {
    int n;
    (void)ctx;
    snprintf(g_last_url, sizeof(g_last_url), "%s", r->url);
    r->status = 200;
    n = snprintf(r->out, (size_t)r->out_max, "ok");
    r->len = n;
    snprintf(r->ctype_out, sizeof(r->ctype_out), "text/plain");
    return 0;
}

static const char *run_at(const char *url, const char *html) {
    g_rc = wf_jspage_run(g_arena, ARENA, html, (int)strlen(html), url, g_out, (int)sizeof(g_out), &g_st);
    return g_rc > 0 ? g_out : "";
}

static const char *logtext(void) {
    int n;
    return wf_jspage_log(&n);
}

#define CHECK_STR(want, got) do { if (strcmp((want), (got)) != 0) { \
    printf("  want: [%s]\n  got:  [%s]\n", (want), (got)); TEST_ASSERT_TRUE(0); } } while (0)

/* each case: an expression and String() of it in node v24.22 */
typedef struct { const char *expr, *want; } clone_case_t;
static const clone_case_t g_cases[] = {
    { "JSON.stringify(structuredClone({a: 1, b: [1, 'x', null, true], c: {d: {e: 'f'}}}))",
      "{\"a\":1,\"b\":[1,\"x\",null,true],\"c\":{\"d\":{\"e\":\"f\"}}}" },
    { "(() => { const o = {n: 1}; o.self = o; const c = structuredClone(o); return (c.self === c) + ' ' + (c !== o) + ' ' + c.n; })()",
      "true true 1" },
    { "(() => { const s = {v: 2}; const c = structuredClone({x: s, y: s}); return (c.x === c.y) + ' ' + (c.x !== s) + ' ' + c.y.v; })()",
      "true true 2" },
    { "(() => { const a = [1, , 3]; const c = structuredClone(a); return c.length + ' ' + (1 in c) + ' ' + c[2]; })()",
      "3 false 3" },
    { "(() => { const m = new Map([[1, {k: 'v'}], ['s', [2]]]); const c = structuredClone(m); return (c instanceof Map) + ' ' + c.size + ' ' + c.get(1).k + ' ' + c.get('s')[0] + ' ' + (c.get(1) !== m.get(1)); })()",
      "true 2 v 2 true" },
    { "(() => { const s = new Set([1, 'a', 1]); const c = structuredClone(s); return (c instanceof Set) + ' ' + c.size + ' ' + c.has('a'); })()",
      "true 2 true" },
    { "(() => { const d = new Date(86400000); const c = structuredClone(d); return (c instanceof Date) + ' ' + c.getTime() + ' ' + (c !== d); })()",
      "true 86400000 true" },
    { "(() => { const r = /a+b/gi; r.lastIndex = 3; const c = structuredClone(r); return (c instanceof RegExp) + ' ' + c.source + ' ' + c.flags + ' ' + c.lastIndex; })()",
      "true a+b gi 0" },
    { "(() => { const e = structuredClone(new TypeError('bad')); return (e instanceof TypeError) + ' ' + e.name + ' ' + e.message; })()",
      "true TypeError bad" },
    { "(() => { try { structuredClone(() => 1); return 'no'; } catch (e) { return e.name + ' ' + e.message.endsWith(' could not be cloned.') + ' ' + (e instanceof DOMException) + ' ' + e.code; } })()",
      "DataCloneError true true 25" },
    { "(() => { try { structuredClone({f: function () {}}); return 'no'; } catch (e) { return e.name; } })()",
      "DataCloneError" },
    { "(() => { class P { constructor() { this.x = 1; } get y() { return 2; } } const c = structuredClone(new P()); return (c instanceof P) + ' ' + JSON.stringify(c) + ' ' + c.y; })()",
      "false {\"x\":1} undefined" },
    { "(() => { const o = {}; Object.defineProperty(o, 'h', {value: 1, enumerable: false}); o.v = 2; return JSON.stringify(structuredClone(o)); })()",
      "{\"v\":2}" },
    { "(() => { const n = structuredClone(new Number(5)); const s = structuredClone(new String('q')); return typeof n + ' ' + (n + 1) + ' ' + typeof s + ' ' + s; })()",
      "object 6 object q" },
    { "String(structuredClone(undefined)) + ' ' + structuredClone(null) + ' ' + structuredClone(-0) + ' ' + Object.is(structuredClone(-0), -0) + ' ' + structuredClone(NaN)",
      "undefined null 0 true NaN" },
    { "(() => { try { structuredClone(); } catch (e) { return e.name; } })()",
      "TypeError" },
    { "(() => { const o = {get g() { return 'got'; }}; const c = structuredClone(o); return c.g + ' ' + (Object.getOwnPropertyDescriptor(c, 'g').get === undefined); })()",
      "got true" },
    { "(() => { const a = []; a.push(a); const c = structuredClone(a); return (c[0] === c) + ' ' + c.length; })()",
      "true 1" },
    { "(() => { try { structuredClone(new URL('http://h/')); return 'no'; } catch (e) { return e.name; } })()",
      "DataCloneError" },
    { "(() => { try { structuredClone(Promise.resolve(1)); return 'no'; } catch (e) { return e.name; } })()",
      "DataCloneError" },
    { "(() => { const m = new Map(); m.set(m, m); const c = structuredClone(m); return c.get(c) === c; })()",
      "true" },
    { 0, 0 }
};

static void test_l114_clone_like_node(void) {
    int i, bad = 0;
    wf_jspage_set_fetch(0, 0);
    for (i = 0; g_cases[i].expr; i++) {
        static char want[1024];
        snprintf(g_html, sizeof(g_html), "<body><script>var v; try { v = String(%s); } catch (e) { v = 'THROW ' + e; }"
                 "console.log(v);</script></body>", g_cases[i].expr);
        run_at("http://10.0.2.2:8067/lot114/clone.html", g_html);
        snprintf(want, sizeof(want), "%s\n", g_cases[i].want);
        if (strcmp(want, logtext()) != 0) {
            printf("  case %d: %s\n  want: [%s]\n  got:  [%s]\n", i, g_cases[i].expr, g_cases[i].want, logtext());
            bad++;
        }
    }
    TEST_ASSERT_EQUAL(0, bad);
}

/* a DOM node cannot be cloned; the clone bounds throw a RangeError */
static void test_l114_clone_bounds(void) {
    run_at("http://10.0.2.2:8067/lot114/clone.html",
           "<body><p id=p>x</p><script>"
           "try { structuredClone(document.getElementById('p')); } catch (e) { console.log(e.name); }"
           "var deep = {}, cur = deep; for (var i = 0; i < 100; i++) { cur.n = {}; cur = cur.n; }"
           "try { structuredClone(deep); } catch (e) { console.log(e.name); }"
           "var ok = {}, c2 = ok; for (var j = 0; j < 60; j++) { c2.n = {}; c2 = c2.n; }"
           "console.log(typeof structuredClone(ok).n.n);"
           "var wide = []; for (var k = 0; k < 5000; k++) wide.push({k: k});"
           "try { structuredClone(wide); } catch (e) { console.log(e.name); }"
           "try { structuredClone(1, {transfer: [{}]}); } catch (e) { console.log(e.name); }"
           "</script></body>");
    CHECK_STR("DataCloneError\nRangeError\nobject\nRangeError\nDataCloneError\n", logtext());
}

static void test_l114_history_push_replace(void) {
    run_at("http://10.0.2.2:8067/lot114/h.html",
           "<body><script>"
           "var log = [];"
           "log.push(history.length, String(history.state), history.scrollRestoration);"
           "history.pushState({p: 1}, '', 'a.html?x=1');"
           "log.push(history.length, location.href, location.pathname, location.search, JSON.stringify(history.state), document.URL);"
           "history.replaceState({p: 2}, '');"
           "log.push(history.length, history.state.p);"
           "history.pushState(null, '', '#frag');"
           "log.push(location.hash, history.length);"
           "try { history.pushState(null, '', 'http://other.org/'); } catch (e) { log.push(e.name, e.code, e.message); }"
           "try { history.pushState({f: function () {}}, ''); } catch (e) { log.push(e.name); }"
           "try { history.pushState({}); } catch (e) { log.push(e.message); }"
           "history.scrollRestoration = 'bogus'; log.push(history.scrollRestoration);"
           "history.scrollRestoration = 'manual'; log.push(history.scrollRestoration);"
           "try { new History(); } catch (e) { log.push(e.name); }"
           "log.push(history.length, location.href);"
           "log.forEach(function (x) { console.log(String(x)); });"
           "</script></body>");
    CHECK_STR("1\nnull\nauto\n2\nhttp://10.0.2.2:8067/lot114/a.html?x=1\n/lot114/a.html\n?x=1\n{\"p\":1}\n"
              "http://10.0.2.2:8067/lot114/a.html?x=1\n2\n2\n#frag\n3\nSecurityError\n18\n"
              "Failed to execute 'pushState' on 'History': A history state object with URL 'http://other.org/' "
              "cannot be created in a document with origin 'http://10.0.2.2:8067' and URL "
              "'http://10.0.2.2:8067/lot114/a.html?x=1#frag'.\nDataCloneError\n"
              "Failed to execute 'pushState' on 'History': 2 arguments required, but only 1 present.\n"
              "auto\nmanual\nTypeError\n3\nhttp://10.0.2.2:8067/lot114/a.html?x=1#frag\n", logtext());
}

static void test_l114_history_traverse(void) {
    run_at("http://10.0.2.2:8067/lot114/h.html",
           "<body><script>"
           "history.pushState({p: 1}, '', 'a.html?x=1');"
           "history.pushState({p: 2}, '', '#frag');"
           "window.addEventListener('popstate', function (e) { console.log('pop', JSON.stringify(e.state), location.href); });"
           "window.onhashchange = function (e) { console.log('hash', e.oldURL, e.newURL); };"
           "history.back();"
           "console.log('sync', location.hash);"
           "setTimeout(function () {"
           "  history.go(-1);"
           "  setTimeout(function () {"
           "    console.log('len', history.length, 'state', JSON.stringify(history.state));"
           "    history.go(5); history.go(0);"
           "    history.pushState(9, '', 'z');"
           "    console.log('trunc', history.length, location.pathname);"
           "    history.back(); history.forward();"
           "    setTimeout(function () { console.log('end', history.state, location.pathname); }, 5);"
           "  }, 0);"
           "}, 0);"
           "</script></body>");
    CHECK_STR("sync #frag\n"
              "pop {\"p\":1} http://10.0.2.2:8067/lot114/a.html?x=1\n"
              "hash http://10.0.2.2:8067/lot114/a.html?x=1#frag http://10.0.2.2:8067/lot114/a.html?x=1\n"
              "pop null http://10.0.2.2:8067/lot114/h.html\n"
              "len 3 state null\n"
              "trunc 2 /lot114/z\n"
              "pop null http://10.0.2.2:8067/lot114/h.html\n"
              "pop 9 http://10.0.2.2:8067/lot114/z\n"
              "end 9 /lot114/z\n", logtext());
}

/* the page's relative URLs follow the pushed location; the entry bound */
static void test_l114_history_base_and_bound(void) {
    wf_jspage_set_fetch(hook, 0);
    g_last_url[0] = 0;
    run_at("http://10.0.2.2:8067/lot114/h.html",
           "<body><script>"
           "history.pushState(null, '', 'sub/p.html');"
           "fetch('d.json').then(function (r) { return r.text(); }).then(function (t) { console.log('fetched', t); });"
           "for (var i = 0; i < 60; i++) history.pushState(i, '', '?i=' + i);"
           "console.log(history.length, history.state, location.search);"
           "</script></body>");
    CHECK_STR("http://10.0.2.2:8067/lot114/sub/d.json", g_last_url);
    CHECK_STR("50 59 ?i=59\nfetched ok\n", logtext());
    /* a new page starts with one entry */
    run_at("http://10.0.2.2:8067/lot114/h2.html", "<body><script>console.log(history.length, location.pathname);</script></body>");
    CHECK_STR("1 /lot114/h2.html\n", logtext());
    wf_jspage_set_fetch(0, 0);
}

int main(void) {
    unity_init();
    RUN_TEST(test_l114_clone_like_node);
    RUN_TEST(test_l114_clone_bounds);
    RUN_TEST(test_l114_history_push_replace);
    RUN_TEST(test_l114_history_traverse);
    RUN_TEST(test_l114_history_base_and_bound);
    unity_print_results();
    unity_cleanup();
    return unity_stats.tests_failed == 0 ? 0 : 1;
}
