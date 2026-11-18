/* Lot 114 (US-031): the web APIs of the page realm (wf_jsapi.c): URL and
 * URLSearchParams (each case equal to node v24.22), FormData from a form
 * and as a fetch body, localStorage / sessionStorage in the shell's
 * static store (kept across pages, per origin, the quota), AbortController
 * with fetch, DOMException. The network is a table here. */
#include "../../framework/unity.h"
#include <string.h>
#include <stdio.h>
#include "wf_jsdom.h"
#include "wf_jsapi.h"

#define ARENA (6u * 1024u * 1024u)
static unsigned char g_arena[ARENA] __attribute__((aligned(16)));
static char g_out[256 * 1024];
static char g_html[8192];
static wf_jspage_stats_t g_st;
static int g_rc;
static char g_last_body[1024], g_last_ctype[128], g_last_url[256];
static int g_calls;

static int hook(void *ctx, wf_jsreq_t *r) {
    int n;
    (void)ctx;
    g_calls++;
    snprintf(g_last_url, sizeof(g_last_url), "%s", r->url);
    snprintf(g_last_body, sizeof(g_last_body), "%.*s", r->body ? r->body_len : 0, r->body ? r->body : "");
    snprintf(g_last_ctype, sizeof(g_last_ctype), "%s", r->ctype ? r->ctype : "");
    r->status = 200;
    n = snprintf(r->out, (size_t)r->out_max, "ok %d", r->body_len);
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
#define CHECK_HAS(hay, needle) do { if (!strstr((hay), (needle))) { \
    printf("  missing: [%s]\n  in: [%.900s]\n", (needle), (hay)); TEST_ASSERT_TRUE(0); } } while (0)

/* each case: an expression and String() of it in node v24.22 */
typedef struct { const char *expr, *want; } api_case_t;
static const api_case_t g_cases[] = {
    { "new URL('http://Example.COM:80/a/./b/../c?x=1#f').href",
      "http://example.com/a/c?x=1#f" },
    { "new URL('https://h:443/').host + '|' + new URL('https://h:8443/').host",
      "h|h:8443" },
    { "new URL('../d/e?q', 'http://h/a/b/c').href",
      "http://h/a/d/e?q" },
    { "new URL('//other/p', 'https://h/x').href",
      "https://other/p" },
    { "new URL('?z=2', 'http://h/a/b?y=1#k').href",
      "http://h/a/b?z=2" },
    { "new URL('#top', 'http://h/a?b').href",
      "http://h/a?b#top" },
    { "new URL('', 'http://h/a?b#c').href",
      "http://h/a?b" },
    { "new URL('/root', 'http://u:p@h:81/a').href",
      "http://u:p@h:81/root" },
    { "new URL('http://h/a b/\303\251?q=a b&r=\"x\"#f g').href",
      "http://h/a%20b/%C3%A9?q=a%20b&r=%22x%22#f%20g" },
    { "new URL('mailto:Me@Example.com').pathname + '|' + new URL('mailto:x').origin",
      "Me@Example.com|null" },
    { "new URL('foo://Host/P/../Q').href",
      "foo://Host/Q" },
    { "new URL('file:///etc/../tmp/x').href",
      "file:///tmp/x" },
    { "new URL('http://h/a/..').pathname + '|' + new URL('http://h/a/.').pathname + '|' + new URL('http://h/..').pathname",
      "/|/a/|/" },
    { "new URL('HTTP://h\\\\a\\\\b').href",
      "http://h/a/b" },
    { "(() => { const u = new URL('http://h:8080/p?a=1&b=2#x'); return [u.protocol, u.host, u.hostname, u.port, u.pathname, u.search, u.hash, u.origin].join(' '); })()",
      "http: h:8080 h 8080 /p ?a=1&b=2 #x http://h:8080" },
    { "(() => { const u = new URL('http://user:pa ss@h/'); return u.username + '|' + u.password + '|' + u.href; })()",
      "user|pa%20ss|http://user:pa%20ss@h/" },
    { "(() => { try { new URL('nope'); return 'no'; } catch (e) { return e.name + ': ' + (e instanceof TypeError); } })()",
      "TypeError: true" },
    { "(() => { try { new URL('http://h:99999/'); return 'no'; } catch (e) { return e.name; } })()",
      "TypeError" },
    { "URL.canParse('x', 'http://h/') + ' ' + URL.canParse('x') + ' ' + URL.canParse('http://[')",
      "true false false" },
    { "(() => { const u = new URL('http://h/p'); u.search = 'a=1 2'; u.hash = 'h'; u.pathname = 'q/r'; u.port = '8080'; u.hostname = 'Z.org'; return u.href; })()",
      "http://z.org:8080/q/r?a=1%202#h" },
    { "(() => { const u = new URL('http://h/p'); u.port = '80'; u.protocol = 'https'; u.username = 'me'; return u.href; })()",
      "https://me@h/p" },
    { "(() => { const u = new URL('http://h/p'); u.protocol = 'mailto'; u.href = 'https://x/y'; return u.href + ' ' + JSON.stringify({u}); })()",
      "https://x/y {\"u\":\"https://x/y\"}" },
    { "(() => { const u = new URL('http://h/p?a=1&b=2&a=3'); const p = u.searchParams; p.append('c', 'x y'); p.delete('b'); return u.search + ' ' + p.getAll('a').join() + ' ' + u.searchParams.size; })()",
      "?a=1&a=3&c=x+y 1,3 3" },
    { "(() => { const u = new URL('http://h/?x=1'); const p = u.searchParams; u.search = '?y=2'; return p.get('x') + ' ' + p.get('y'); })()",
      "null 2" },
    { "new URLSearchParams('?a=1&b=%20x+y&c&=e&a=2').toString()",
      "a=1&b=+x+y&c=&=e&a=2" },
    { "(() => { const p = new URLSearchParams({ q: 'caf\303\251 & co', n: 3 }); return p.toString() + ' ' + p.get('q'); })()",
      "q=caf%C3%A9+%26+co&n=3 caf\303\251 & co" },
    { "(() => { const p = new URLSearchParams([['b', '2'], ['a', '1'], ['b', '0']]); p.sort(); return p.toString() + ' ' + p.has('a') + p.has('z') + p.has('b', '0'); })()",
      "a=1&b=2&b=0 truefalsetrue" },
    { "(() => { const p = new URLSearchParams('a=1&a=2&b=3'); p.set('a', '9'); return p.toString() + ' ' + [...p.keys()].join() + ' ' + [...p.values()].join(); })()",
      "a=9&b=3 a,b 9,3" },
    { "(() => { const p = new URLSearchParams('x=1&y=2'); const out = []; p.forEach((v, k) => out.push(k + v)); for (const [k, v] of p.entries()) out.push(k + '=' + v); return out.join(' '); })()",
      "x1 y2 x=1 y=2" },
    { "(() => { const p = new URLSearchParams('a=1&a=2'); p.delete('a', '2'); return p.toString() + ' ' + p.get('zz'); })()",
      "a=1 null" },
    { "(() => { try { new URLSearchParams([['a']]); } catch (e) { return e.name; } })()",
      "TypeError" },
    { "new URLSearchParams(new URLSearchParams('k=v')).get('k')",
      "v" },
    { "(() => { const f = new FormData(); f.append('a', '1'); f.append('a', 2); f.set('b', 'x'); return f.getAll('a').join() + ' ' + f.get('b') + ' ' + f.has('c') + ' ' + [...f.keys()].join(); })()",
      "1,2 x false a,a,b" },
    { "(() => { const c = new AbortController(); const s = c.signal; const seen = []; s.addEventListener('abort', e => seen.push(e.type)); s.onabort = () => seen.push('on'); c.abort(); c.abort(); return s.aborted + ' ' + s.reason.name + ' ' + seen.join(); })()",
      "true AbortError abort,on" },
    { "(() => { const s = AbortSignal.abort('why'); try { s.throwIfAborted(); } catch (e) { return s.aborted + ' ' + e; } })()",
      "true why" },
    { "(() => { const e = new DOMException('m', 'NotFoundError'); return e.name + ' ' + e.message + ' ' + e.code + ' ' + (e instanceof Error) + ' ' + (e instanceof DOMException); })()",
      "NotFoundError m 8 true true" },
    { "(() => { try { new AbortSignal(); } catch (e) { return e.name; } })()",
      "TypeError" },
    { "AbortSignal.any([new AbortController().signal, AbortSignal.abort(1)]).reason",
      "1" },
    { 0, 0 }
};

static void test_l114_api_like_node(void) {
    int i, bad = 0;
    wf_jspage_set_fetch(0, 0);
    for (i = 0; g_cases[i].expr; i++) {
        static char want[1024];
        snprintf(g_html, sizeof(g_html), "<body><script>var v; try { v = String(%s); } catch (e) { v = 'THROW ' + e; }"
                 "console.log(v);</script></body>", g_cases[i].expr);
        run_at("http://10.0.2.2:8067/lot114/api.html", g_html);
        snprintf(want, sizeof(want), "%s\n", g_cases[i].want);
        if (strcmp(want, logtext()) != 0) {
            printf("  case %d: %s\n  want: [%s]\n  got:  [%s]\n", i, g_cases[i].expr, g_cases[i].want, logtext());
            bad++;
        }
    }
    TEST_ASSERT_EQUAL(0, bad);
}

static void test_l114_formdata_from_form(void) {
    wf_jspage_set_fetch(0, 0);
    run_at("http://10.0.2.2:8067/lot114/f.html",
           "<body><form id=f><input name=a value=1><input name=b value=\"x y\" disabled><input type=checkbox name=c checked>"
           "<input type=checkbox name=d value=dv><input type=radio name=r value=r1><input type=radio name=r value=r2 checked>"
           "<select name=s><option value=o1>1<option value=o2 selected>2</select><textarea name=t>tx</textarea>"
           "<input type=submit name=go value=Go><input name=a value=3><input value=noname></form><p id=p></p><script>"
           "var f = new FormData(document.getElementById('f'));"
           "console.log(JSON.stringify([...f.entries()]));"
           "try { new FormData(document.getElementById('p')); } catch (e) { console.log(e.name); }"
           "</script></body>");
    CHECK_STR("[[\"a\",\"1\"],[\"c\",\"on\"],[\"r\",\"r2\"],[\"s\",\"o2\"],[\"t\",\"tx\"],[\"a\",\"3\"]]\nTypeError\n", logtext());
}

static void test_l114_bodies_and_abort(void) {
    wf_jspage_set_fetch(hook, 0);
    g_calls = 0;
    run_at("http://10.0.2.2:8067/lot114/b.html",
           "<body><p id=o></p><script>"
           "var out = [];"
           "fetch('echo', { method: 'POST', body: new URLSearchParams({ a: '1 2', b: '&' }) }).then(r => r.text()).then(t => out.push('usp ' + t));"
           "</script></body>");
    CHECK_STR("a=1+2&b=%26", g_last_body);
    CHECK_STR("application/x-www-form-urlencoded;charset=UTF-8", g_last_ctype);
    run_at("http://10.0.2.2:8067/lot114/b.html",
           "<body><script>"
           "var f = new FormData(); f.append('n', 'v1'); f.append('q\"x', 'line');"
           "var x = new XMLHttpRequest(); x.open('POST', 'echo', false); x.send(f); console.log(x.responseText);"
           "</script></body>");
    CHECK_STR("multipart/form-data; boundary=" WF_JSAPI_BOUNDARY, g_last_ctype);
    CHECK_STR("--" WF_JSAPI_BOUNDARY "\r\nContent-Disposition: form-data; name=\"n\"\r\n\r\nv1\r\n"
              "--" WF_JSAPI_BOUNDARY "\r\nContent-Disposition: form-data; name=\"q%22x\"\r\n\r\nline\r\n"
              "--" WF_JSAPI_BOUNDARY "--\r\n", g_last_body);
    /* an explicit Content-Type wins */
    run_at("http://10.0.2.2:8067/lot114/b.html",
           "<body><script>fetch('echo', { method: 'POST', headers: { 'Content-Type': 'text/x-mine' }, body: new URLSearchParams('k=v') });"
           "</script></body>");
    CHECK_STR("text/x-mine", g_last_ctype);
    CHECK_STR("k=v", g_last_body);
    /* abort: before the request (no call) and before the answer task */
    g_calls = 0;
    run_at("http://10.0.2.2:8067/lot114/b.html",
           "<body><script>"
           "var c1 = new AbortController(); c1.abort();"
           "fetch('a', { signal: c1.signal }).catch(e => console.log('early', e.name, e.message));"
           "var c2 = new AbortController();"
           "fetch('b', { signal: c2.signal }).then(() => console.log('late ok')).catch(e => console.log('late', e.name));"
           "c2.abort();"
           "var c3 = new AbortController();"
           "fetch('c', { signal: c3.signal }).then(r => r.text()).then(t => console.log('kept', t));"
           "fetch('d', { signal: AbortSignal.abort(new Error('mine')) }).catch(e => console.log('reason', e.message));"
           "</script></body>");
    CHECK_STR("early AbortError signal is aborted without reason\nreason mine\nlate AbortError\nkept ok 0\n", logtext());
    TEST_ASSERT_EQUAL(2, g_calls);
}

static void test_l114_storage(void) {
    js_api_store_reset();
    wf_jspage_set_fetch(0, 0);
    run_at("http://10.0.2.2:8067/lot114/s1.html",
           "<body><script>"
           "localStorage.setItem('k', 'v1'); localStorage.setItem('n', 42); sessionStorage.setItem('k', 's');"
           "console.log(localStorage.getItem('k'), localStorage.getItem('n'), localStorage.length, localStorage.key(1), localStorage.key(5));"
           "console.log(localStorage.getItem('none'), sessionStorage.getItem('k'), typeof localStorage.setItem);"
           "localStorage.setItem('k', 'a longer value than before');"
           "</script></body>");
    CHECK_STR("v1 42 2 n null\nnull s function\n", logtext());
    /* another page of the same origin sees them; another origin does not */
    run_at("http://10.0.2.2:8067/other/s2.html",
           "<body><script>console.log(localStorage.getItem('k'), localStorage.key(0), sessionStorage.length);"
           "localStorage.removeItem('n'); console.log(localStorage.length);</script></body>");
    CHECK_STR("a longer value than before k 1\n1\n", logtext());
    run_at("http://10.0.2.2:9999/s3.html",
           "<body><script>console.log(localStorage.length, localStorage.getItem('k'));"
           "localStorage.setItem('mine', 'x'); localStorage.clear(); console.log(localStorage.length);</script></body>");
    CHECK_STR("0 null\n0\n", logtext());
    run_at("http://10.0.2.2:8067/s4.html",
           "<body><script>"
           "var big = 'x'.repeat(20000);"
           "localStorage.setItem('b1', big);"
           "try { localStorage.setItem('b2', big); } catch (e) { console.log(e.name, e.code, e instanceof DOMException); }"
           "localStorage.removeItem('b1'); localStorage.setItem('b2', big); console.log(localStorage.getItem('b2').length);"
           "try { localStorage.setItem(); } catch (e) { console.log(e.message); }"
           "try { new Storage(); } catch (e) { console.log(e.name); }"
           "</script></body>");
    CHECK_STR("QuotaExceededError 22 true\n20000\nFailed to execute 'setItem' on 'Storage': 2 arguments required, but only 0 present.\n"
              "TypeError\n", logtext());
    /* a file: page has no origin for the store */
    run_at("file:///web/x.html", "<body><script>try { localStorage.getItem('k'); } catch (e) { console.log(e.name); }</script></body>");
    CHECK_STR("SecurityError\n", logtext());
    /* more origins than slots: the oldest goes */
    {
        int i;
        for (i = 0; i < WF_JSAPI_STORE_ORIGINS + 1; i++) {
            char url[64];
            snprintf(url, sizeof(url), "http://host%d.test/", i);
            run_at(url, "<body><script>localStorage.setItem('o', location.host);</script></body>");
        }
        run_at("http://host0.test/", "<body><script>console.log(localStorage.getItem('o'));</script></body>");
        CHECK_STR("null\n", logtext());
        run_at("http://host5.test/", "<body><script>console.log(localStorage.getItem('o'));</script></body>");
        CHECK_STR("host5.test\n", logtext());
    }
    js_api_store_reset();
}

int main(void) {
    unity_init();
    RUN_TEST(test_l114_api_like_node);
    RUN_TEST(test_l114_formdata_from_form);
    RUN_TEST(test_l114_bodies_and_abort);
    RUN_TEST(test_l114_storage);
    unity_print_results();
    unity_cleanup();
    return unity_stats.tests_failed == 0 ? 0 : 1;
}
