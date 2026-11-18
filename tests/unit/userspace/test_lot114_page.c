/* Lot 114 (US-031): the page pass after Lot 113 on the host: external
 * scripts through the fetch hook (order, defer/async, load/error, the
 * bounds), scripts inserted by scripts, Promise jobs in the page,
 * fetch()/Response/Headers/XMLHttpRequest (same origin, bounds), on*
 * attributes, the approximate layout values, and events after load
 * (click, key, input, focus, submit) with the clickable marks that the
 * GUI turns into "web-js click @N". The network is a table here. */
#include "../../framework/unity.h"
#include <string.h>
#include <stdio.h>
#include "wf_jsdom.h"

#define ARENA (6u * 1024u * 1024u)
static unsigned char g_arena[ARENA] __attribute__((aligned(16)));
static char g_out[256 * 1024];
static wf_jspage_stats_t g_st;
static int g_rc;

#define BASE "http://10.0.2.2:8067/lot114/"

typedef struct { const char *url; int status; const char *ctype; const char *body; } route_t;
static const route_t g_routes[] = {
    { BASE "a.js", 200, "text/javascript", "log.push('a:' + document.currentScript.getAttribute('src'));" },
    { BASE "b.js", 200, "text/javascript", "log.push('b'); document.write('<p id=w>written</p>');" },
    { BASE "d.js", 200, "text/javascript", "log.push('defer:' + document.readyState);" },
    { BASE "as.js", 200, "text/javascript", "log.push('async');" },
    { BASE "throw.js", 200, "text/javascript", "log.push('t'); null.x;" },
    { BASE "lib.js", 200, "text/javascript", "window.Lib = { twice: function (x) { return 2 * x; } };" },
    { BASE "dyn.js", 200, "text/javascript", "log.push('dyn:' + typeof Lib);" },
    { BASE "data.json", 200, "application/json", "{\"items\":[1,2,3],\"name\":\"mohhdy\"}" },
    { BASE "text.txt", 200, "text/plain", "hello text" },
    { BASE "missing.js", 404, "text/html", "<h1>not found</h1>" },
    { 0, 0, 0, 0 }
};
static int g_hook_calls;
static char g_last_method[8], g_last_body[256], g_last_ctype[96];

static int hook(void *ctx, wf_jsreq_t *r) {
    int i, n;
    (void)ctx;
    g_hook_calls++;
    snprintf(g_last_method, sizeof(g_last_method), "%s", r->method);
    snprintf(g_last_body, sizeof(g_last_body), "%.*s", r->body ? r->body_len : 0, r->body ? r->body : "");
    snprintf(g_last_ctype, sizeof(g_last_ctype), "%s", r->ctype ? r->ctype : "");
    if (strcmp(r->url, BASE "echo") == 0) {
        r->status = 201;
        n = snprintf(r->out, (size_t)r->out_max, "%s %s %s", r->method, r->ctype ? r->ctype : "-", g_last_body);
        r->len = n;
        snprintf(r->ctype_out, sizeof(r->ctype_out), "text/plain");
        return 0;
    }
    if (strcmp(r->url, BASE "big.js") == 0) {
        r->status = 200;
        r->len = r->out_max;
        memset(r->out, ' ', (size_t)r->out_max);
        r->truncated = 1;
        return 0;
    }
    if (strcmp(r->url, BASE "down.js") == 0) return -1;
    for (i = 0; g_routes[i].url; i++) {
        if (strcmp(g_routes[i].url, r->url) != 0) continue;
        r->status = g_routes[i].status;
        n = (int)strlen(g_routes[i].body);
        if (n > r->out_max) { n = r->out_max; r->truncated = 1; }
        memcpy(r->out, g_routes[i].body, (size_t)n);
        r->len = n;
        snprintf(r->ctype_out, sizeof(r->ctype_out), "%s", g_routes[i].ctype);
        return 0;
    }
    r->status = 404;
    r->len = 0;
    return 0;
}

static const char *run(const char *html) {
    g_hook_calls = 0;
    g_rc = wf_jspage_run(g_arena, ARENA, html, (int)strlen(html), BASE "index.html", g_out, (int)sizeof(g_out), &g_st);
    return g_rc > 0 ? g_out : "";
}

static const char *logtext(void) {
    static char b[16384];
    int n;
    const char *l = wf_jspage_log(&n);
    if (n > (int)sizeof(b) - 1) n = (int)sizeof(b) - 1;
    memcpy(b, l, (size_t)n);
    b[n] = 0;
    return b;
}

#define CHECK_STR(want, got) do { const char *w_ = (want), *g_ = (got); \
    if (strcmp(w_, g_) != 0) printf("\n  want: [%s]\n  got:  [%s]\n", w_, g_); \
    TEST_ASSERT_EQUAL_STRING(w_, g_); } while (0)
#define CHECK_HAS(hay, needle) do { const char *h_ = (hay), *n_ = (needle); \
    if (!strstr(h_, n_)) printf("\n  missing: [%s]\n  in: [%s]\n", n_, h_); \
    TEST_ASSERT_NOT_NULL(strstr(h_, n_)); } while (0)

/* the page's log array, printed at the end of the pass */
#define DUMP_INLINE "window.addEventListener('load', function () { setTimeout(function () { console.log(log.join(' ')); }, 5000); });"
#define DUMP "<script>window.addEventListener('load', function () { setTimeout(function () { console.log(log.join(' ')); }, 5000); });</script>"

static void test_l114_external_order(void) {
    wf_jspage_set_fetch(hook, 0);
    run("<html><head><script>var log = [];</script>" DUMP
        "<script src=\"d.js\" defer></script><script async src=\"as.js\"></script>"
        "<script src=\"a.js\"></script></head><body><script src=\"b.js\"></script>"
        "<script>log.push('inline:' + document.getElementById('w').textContent);</script></body></html>");
    TEST_ASSERT_TRUE(g_rc > 0);
    CHECK_STR("a:a.js b inline:written defer:interactive async\n", logtext());
    TEST_ASSERT_EQUAL(4, g_st.external);
    TEST_ASSERT_EQUAL(4, g_st.ext_ran);
    TEST_ASSERT_EQUAL(0, g_st.ext_failed);
    TEST_ASSERT_EQUAL(4, g_hook_calls);
    CHECK_HAS(g_out, "<p id=\"w\">written</p>");
    TEST_ASSERT_NULL(strstr(g_out, "<script"));
}

static void test_l114_external_errors(void) {
    wf_jspage_set_fetch(hook, 0);
    run("<script>var log = [];</script>" DUMP
        "<script src=\"missing.js\" onerror=\"log.push('err404')\"></script>"
        "<script src=\"down.js\" onerror=\"log.push('errnet')\"></script>"
        "<script src=\"big.js\" onerror=\"log.push('errbig')\"></script>"
        "<script src=\"throw.js\" onload=\"log.push('loaded')\"></script>"
        "<script src=\"data:text/javascript,log.push(1)\" onerror=\"log.push('errdata')\"></script>"
        "<script type=\"module\" src=\"a.js\"></script>");
    CHECK_HAS(logtext(), "warn: script not loaded (HTTP status not 200): " BASE "missing.js\n");
    CHECK_HAS(logtext(), "warn: script not loaded (network error): " BASE "down.js\n");
    CHECK_HAS(logtext(), "warn: script not loaded (over the 256 KiB bound): " BASE "big.js\n");
    CHECK_HAS(logtext(), "error: Uncaught TypeError: Cannot read properties of null (reading 'x')\n");
    CHECK_HAS(logtext(), "warn: script src refused (not http or https): data:text/javascript,log.push(1)\n");
    CHECK_HAS(logtext(), "err404 errnet errbig t loaded errdata\n");
    TEST_ASSERT_EQUAL(6, g_st.external);
    TEST_ASSERT_EQUAL(1, g_st.other_type);
    TEST_ASSERT_EQUAL(1, g_st.ext_ran);
    TEST_ASSERT_EQUAL(4, g_st.ext_failed);
    TEST_ASSERT_EQUAL(5, g_st.handlers);     /* four onerror, one onload */
}

static void test_l114_external_bound_and_no_hook(void) {
    static char page[8192];
    int i, n = 0;
    wf_jspage_set_fetch(hook, 0);
    n += snprintf(page + n, sizeof(page) - (size_t)n, "<script>var log = [];</script>");
    for (i = 0; i < WF_DOM_EXT_SCRIPTS + 3; i++) n += snprintf(page + n, sizeof(page) - (size_t)n, "<script src=\"lib.js\"></script>");
    run(page);
    TEST_ASSERT_EQUAL(WF_DOM_EXT_SCRIPTS, g_st.ext_ran);
    TEST_ASSERT_EQUAL(3, g_st.ext_skipped);
    TEST_ASSERT_EQUAL(1, g_st.truncated);
    CHECK_HAS(logtext(), "warn: external script bound reached, script skipped\n");
    wf_jspage_set_fetch(0, 0);
    run("<script>var log = [];</script><script src=\"a.js\"></script><script>console.log(log.length)</script>");
    TEST_ASSERT_EQUAL(1, g_st.ext_skipped);
    TEST_ASSERT_EQUAL(0, g_st.ext_ran);
    CHECK_STR("0\n", logtext());
}

static void test_l114_dynamic_scripts(void) {
    wf_jspage_set_fetch(hook, 0);
    run("<script>var log = [];</script>" DUMP "<body><div id=box></div><script>"
        "var s = document.createElement('script'); s.src = 'lib.js';"
        "s.onload = function () { log.push('lib:' + Lib.twice(21)); };"
        "document.head.appendChild(s);"
        "var t = document.createElement('script'); t.textContent = \"log.push('inline-dyn')\";"
        "document.body.appendChild(t); log.push('after-append');"
        "var u = document.createElement('script'); u.src = 'dyn.js'; log.push('not-inserted');"
        "document.getElementById('box').innerHTML = '<script>log.push(\"never\")<\\/script>';"
        "</script></body>");
    CHECK_STR("after-append not-inserted lib:42 inline-dyn\n", logtext());
    TEST_ASSERT_EQUAL(2, g_st.dynamic);
}

static void test_l114_microtasks_in_page(void) {
    wf_jspage_set_fetch(0, 0);
    run("<body><p id=o></p><script>"
        "var out = [];"
        "setTimeout(function () { out.push('timeout'); }, 0);"
        "Promise.resolve().then(function () { out.push('micro1'); });"
        "queueMicrotask(function () { out.push('micro2'); });"
        "(async function () { out.push('async-start'); await null; out.push('after-await');"
        "  await new Promise(function (r) { setTimeout(r, 10); }); out.push('after-timer');"
        "  document.getElementById('o').textContent = out.join(','); })();"
        "out.push('sync-end');"
        "Promise.reject(new Error('lost'));"
        "</script></body>");
    CHECK_HAS(g_out, "<p id=\"o\">async-start,sync-end,micro1,micro2,after-await,timeout,after-timer</p>");
    CHECK_HAS(logtext(), "Uncaught (in promise) Error: lost");
    TEST_ASSERT_EQUAL(1, g_st.rejections);
    TEST_ASSERT_TRUE(g_st.microtasks >= 4);
}

static void test_l114_fetch(void) {
    wf_jspage_set_fetch(hook, 0);
    run("<body><ul id=l></ul><script>"
        "var log = [];" DUMP_INLINE
        "fetch('data.json').then(function (r) { log.push(r.status + ' ' + r.ok + ' ' + r.headers.get('Content-Type'));"
        "  return r.json(); }).then(function (d) { d.items.forEach(function (i) {"
        "  var li = document.createElement('li'); li.textContent = d.name + i; document.getElementById('l').appendChild(li); }); });"
        "fetch('text.txt').then(function (r) { return r.text().then(function (t) { log.push(t);"
        "  return r.text(); }); }).catch(function (e) { log.push(e.name + ': ' + e.message); });"
        "fetch('missing.js').then(function (r) { log.push('404 ok=' + r.ok + ' ' + r.statusText); });"
        "fetch('http://other.example/x').catch(function (e) { log.push('cors:' + e.message); });"
        "fetch('echo', { method: 'POST', body: 'a=1', headers: { 'Content-Type': 'application/x-www-form-urlencoded' } })"
        "  .then(function (r) { return r.text(); }).then(function (t) { log.push(t); });"
        "fetch('echo', { method: 'PUT' }).catch(function (e) { log.push('put:' + e.message); });"
        "log.push('sync');"
        "var h = new Headers({ 'X-A': '1' }); h.append('x-a', '2'); log.push(h.get('x-a'), h.has('X-B'));"
        "new Response('{\"v\":5}').json().then(function (j) { log.push('resp:' + j.v); });"
        "</script></body>");
    CHECK_HAS(g_out, "<ul id=\"l\"><li>mohhdy1</li><li>mohhdy2</li><li>mohhdy3</li></ul>");
    CHECK_HAS(logtext(), "warn: fetch: cross-origin request refused (no CORS in the guest): http://other.example/x\n");
    CHECK_HAS(logtext(), "warn: fetch: only GET and POST are supported\n");
    CHECK_HAS(logtext(), "sync 1, 2 false put:Failed to fetch resp:5 200 true application/json hello text "
                         "TypeError: Failed to execute 'text' on 'Response': body stream already read 404 ok=false Not Found "
                         "cors:Failed to fetch POST application/x-www-form-urlencoded a=1\n");
    TEST_ASSERT_EQUAL(5, g_st.net_requests);
    TEST_ASSERT_EQUAL(1, g_st.net_failed);
}

static void test_l114_xhr(void) {
    wf_jspage_set_fetch(hook, 0);
    run("<body><script>"
        "var log = [];" DUMP_INLINE
        "var x = new XMLHttpRequest(); x.open('GET', 'data.json');"
        "x.onreadystatechange = function () { log.push('rs' + x.readyState); };"
        "x.responseType = 'json';"
        "x.onload = function () { log.push('load ' + x.status + ' ' + x.response.items.length + ' ' + x.getResponseHeader('content-type')); };"
        "x.addEventListener('loadend', function (e) { log.push('end ' + e.type); });"
        "x.send(); log.push('sent');"
        "var s = new XMLHttpRequest(); s.open('POST', 'echo', false); s.setRequestHeader('Content-Type', 'text/plain');"
        "s.send('body!'); log.push('sync ' + s.status + ' ' + s.responseText);"
        "var e = new XMLHttpRequest(); e.open('GET', 'https://evil.example/');"
        "e.onerror = function () { log.push('xhr-error ' + e.status + ' ' + e.readyState); }; e.send();"
        "</script></body>");
    CHECK_HAS(logtext(), "sent sync 201 POST text/plain body! rs4 load 200 3 application/json end loadend xhr-error 0 4\n");
    CHECK_HAS(logtext(), "warn: XMLHttpRequest: cross-origin request refused (no CORS in the guest): https://evil.example/\n");
}

static void test_l114_net_bound(void) {
    wf_jspage_set_fetch(hook, 0);
    run("<body><script>var n = 0, f = 0; for (var i = 0; i < 20; i++) fetch('text.txt').then(function () { n++; }, function () { f++; });"
        "setTimeout(function () { console.log(n, f); }, 100);</script></body>");
    CHECK_STR("16 4\n", strstr(logtext(), "16 4") ? strstr(logtext(), "16 4") : logtext());
    TEST_ASSERT_EQUAL(16, g_hook_calls);
    TEST_ASSERT_EQUAL(1, g_st.truncated);
}

static void test_l114_layout(void) {
    wf_jspage_set_fetch(0, 0);
    run("<html><body><div id=a>Hello world</div><p id=b style=\"width:100px\">aaaa bbbb cccc dddd</p>"
        "<span id=s>abc</span><img id=i width=40 height=30><input id=t><button id=btn>Go</button>"
        "<ul><li id=li>x</li></ul><div id=n style=\"display:none\"><span id=in>z</span></div>"
        "<h1 id=h>T</h1><div id=fixed style=\"height: 50px\"></div><script>"
        "function r(id) { var e = document.getElementById(id); return [e.offsetLeft, e.offsetTop, e.offsetWidth, e.offsetHeight].join(','); }"
        "console.log(r('a'), r('b'), r('s'), r('i'), r('t'), r('btn'));"
        "console.log(r('li'), r('n'), r('in'), r('h'), r('fixed'));"
        "var rc = document.getElementById('b').getBoundingClientRect();"
        "console.log(rc.x, rc.y, rc.width, rc.height, rc.right, rc.bottom);"
        "console.log(document.documentElement.clientWidth, document.documentElement.clientHeight, document.body.offsetWidth,"
        "  document.getElementById('s').clientWidth, document.getElementById('n').offsetParent, document.getElementById('a').offsetParent.tagName);"
        "var d = document.getElementById('a'); var w0 = d.offsetHeight; d.textContent = 'x '.repeat(200); console.log(w0, d.offsetHeight);"
        "</script></body></html>");
    /* 100px hold 12 characters: "aaaa bbbb" / "cccc dddd" */
    CHECK_STR("8,8,1008,16 8,24,100,32 8,56,24,16 32,56,40,30 72,56,160,22 232,56,32,22\n"
              "48,86,968,16 0,0,0,0 0,0,0,0 8,102,1008,32 8,134,1008,50\n"
              "8 24 100 32 108 56\n"
              "1024 768 1008 0 null BODY\n"
              "16 64\n", logtext());
    TEST_ASSERT_TRUE(g_st.layout_reads > 20);
}

static const char *ev(const char *type, const char *target, const char *arg) {
    g_rc = wf_jspage_event(type, target, arg, g_out, (int)sizeof(g_out), &g_st);
    return g_rc > 0 ? g_out : "";
}

static void test_l114_live_click(void) {
    char sel[16];
    int node;
    const char *o;
    wf_jspage_set_fetch(0, 0);
    o = run("<body><p id=c>0</p><button id=inc>plus</button><a id=nav href=\"/next\">next</a>"
            "<a id=js href=\"#\" onclick=\"document.getElementById('c').textContent = 'link'; return false;\">js link</a>"
            "<div id=d>plain</div><input type=button id=ib value=\"Tap\"><script>"
            "var n = 0; document.getElementById('inc').addEventListener('click', function (e) {"
            "  n++; document.getElementById('c').textContent = n + ' ' + e.type + ' ' + e.isTrusted + ' ' + e.button;"
            "  Promise.resolve().then(function () { document.getElementById('c').setAttribute('data-m', n); }); });"
            "document.getElementById('nav').addEventListener('click', function () {});"
            "document.getElementById('ib').onclick = function () { document.getElementById('c').textContent = 'ib'; };"
            "</script></body>");
    TEST_ASSERT_TRUE(g_rc > 0);
    TEST_ASSERT_EQUAL(1, wf_jspage_alive());
    TEST_ASSERT_EQUAL(3, g_st.marks);
    CHECK_HAS(o, "<a id=\"nav\" href=\"/next\">next</a>");          /* a real link is followed, not marked */
    TEST_ASSERT_NULL(strstr(o, "<div id=\"d\"><a"));
    {
        const char *m = strstr(o, "<button id=\"inc\"><a href=\"?__wfjs=");
        TEST_ASSERT_NOT_NULL(m);
        node = wf_jspage_mark_node(m);
        TEST_ASSERT_TRUE(node > 0);
        TEST_ASSERT_EQUAL(12, wf_jspage_mark_node("http://10.0.2.2:8067/lot114/index.html?__wfjs=12"));
        TEST_ASSERT_EQUAL(-1, wf_jspage_mark_node("http://10.0.2.2:8067/lot114/index.html?q=12"));
    }
    snprintf(sel, sizeof(sel), "@%d", node);
    o = ev("click", sel, 0);
    CHECK_HAS(o, "<p id=\"c\" data-m=\"1\">1 click true 0</p>");
    o = ev("click", "#inc", 0);
    CHECK_HAS(o, "<p id=\"c\" data-m=\"2\">2 click true 0</p>");
    o = ev("click", "#js", 0);
    CHECK_HAS(o, ">link</p>");
    CHECK_HAS(o, "<a href=\"?__wfjs=");
    CHECK_HAS(o, " id=\"js\" onclick=");
    o = ev("click", "#ib", 0);
    CHECK_HAS(o, ">ib</p>");
    CHECK_HAS(o, "<input type=\"button\" id=\"ib\" value=\"Tap\"><a href=\"?__wfjs=");
    TEST_ASSERT_EQUAL(-2, wf_jspage_event("click", "#nope", 0, g_out, (int)sizeof(g_out), &g_st));
    TEST_ASSERT_EQUAL(-2, wf_jspage_event("click", "@99999", 0, g_out, (int)sizeof(g_out), &g_st));
    TEST_ASSERT_EQUAL(4, g_st.live_events);
    {
        static char cl[2048];
        TEST_ASSERT_TRUE(wf_jspage_clickables(cl, (int)sizeof(cl)) > 0);
        CHECK_HAS(cl, " button #inc plus\n");
        CHECK_HAS(cl, " a #js js link\n");
        CHECK_HAS(cl, " input #ib\n");
    }
    wf_jspage_close();
    TEST_ASSERT_EQUAL(-1, wf_jspage_event("click", "#inc", 0, g_out, (int)sizeof(g_out), &g_st));
}

static void test_l114_live_forms(void) {
    const char *o;
    wf_jspage_set_fetch(0, 0);
    run("<body><form id=f action=\"/s\"><input id=q name=q value=\"ab\"><input type=checkbox id=cb>"
        "<input type=radio name=r id=r1 checked><input type=radio name=r id=r2><button id=go>Go</button></form>"
        "<p id=out></p><p id=keys></p><script>"
        "var keys = [];"
        "var q = document.getElementById('q');"
        "q.addEventListener('keydown', function (e) { keys.push('d:' + e.key + ':' + e.code + ':' + e.keyCode); if (e.key === 'x') e.preventDefault(); });"
        "q.addEventListener('input', function (e) { document.getElementById('out').textContent = 'input ' + q.value + ' ' + e.inputType; });"
        "q.addEventListener('keyup', function (e) { keys.push('u:' + e.key); document.getElementById('keys').textContent = keys.join(' '); });"
        "q.addEventListener('focus', function () { keys.push('focus'); });"
        "document.getElementById('cb').addEventListener('change', function (e) { document.getElementById('out').textContent = 'cb ' + e.target.checked; });"
        "document.getElementById('r2').addEventListener('change', function () { document.getElementById('out').textContent = 'r1 ' + document.getElementById('r1').checked; });"
        "document.getElementById('f').addEventListener('submit', function (e) { e.preventDefault(); document.getElementById('out').textContent = 'submit ' + q.value; });"
        "document.addEventListener('keydown', function (e) { if (e.target === document.body) document.getElementById('out').textContent = 'doc ' + e.key; });"
        "</script></body>");
    TEST_ASSERT_TRUE(g_rc > 0);
    ev("focus", "#q", 0);
    o = ev("key", "", "c");
    CHECK_HAS(o, "<input id=\"q\" name=\"q\" value=\"abc\">");
    CHECK_HAS(o, ">input abc insertText</p>");
    CHECK_HAS(o, "<p id=\"keys\">focus d:c:KeyC:67 u:c</p>");
    o = ev("key", "#q", "x");       /* keydown canceled: no character */
    CHECK_HAS(o, "value=\"abc\"");
    o = ev("key", "#q", "Backspace");
    CHECK_HAS(o, "value=\"ab\"");
    CHECK_HAS(o, ">input ab deleteContentBackward</p>");
    o = ev("input", "#q", "new <value>");
    CHECK_HAS(o, "value=\"new &lt;value>\"");
    o = ev("click", "#cb", 0);
    CHECK_HAS(o, "<input type=\"checkbox\" id=\"cb\" checked=\"\">");
    CHECK_HAS(o, ">cb true</p>");
    o = ev("click", "#r2", 0);
    CHECK_HAS(o, ">r1 false</p>");
    CHECK_HAS(o, "<input type=\"radio\" name=\"r\" id=\"r1\">");
    o = ev("click", "#go", 0);
    CHECK_HAS(o, ">submit new &lt;value&gt;</p>");
    o = ev("key", "#q", "Enter");
    CHECK_HAS(o, ">submit new &lt;value&gt;</p>");
    ev("blur", "#q", 0);
    o = ev("key", "", "Escape");
    CHECK_HAS(o, ">doc Escape</p>");
    TEST_ASSERT_EQUAL(-2, wf_jspage_event("key", "#q", "NoSuchKey", g_out, (int)sizeof(g_out), &g_st));
    TEST_ASSERT_EQUAL(-2, wf_jspage_event("input", "#out", "v", g_out, (int)sizeof(g_out), &g_st));
}

static void test_l114_live_timers_and_custom(void) {
    const char *o;
    wf_jspage_set_fetch(hook, 0);
    run("<body><p id=o>idle</p><div id=t tabindex=0>t</div><script>"
        "var el = document.getElementById('t');"
        "el.addEventListener('click', function () { document.getElementById('o').textContent = 'wait';"
        "  setTimeout(function (a, b) { document.getElementById('o').textContent = 'done ' + a + b; }, 300, 'x', 'y');"
        "  fetch('text.txt').then(function (r) { return r.text(); }).then(function (t) { el.setAttribute('data-t', t); }); });"
        "el.addEventListener('ping', function (e) { document.getElementById('o').textContent = 'ping ' + e.bubbles + e.cancelable; });"
        "</script></body>");
    o = ev("click", "#t", 0);
    CHECK_HAS(o, ">done xy</p>");
    CHECK_HAS(o, "data-t=\"hello text\"");
    o = ev("ping", "#t", 0);
    CHECK_HAS(o, ">ping truetrue</p>");
}

static void test_l114_handler_attrs(void) {
    wf_jspage_set_fetch(0, 0);
    run("<body onload=\"log('body')\"><p id=o></p><button id=b onclick=\"log(this.id + ' ' + event.type)\">b</button>"
        "<button id=bad onclick=\"log(\">bad</button><script>"
        "function log(s) { var o = document.getElementById('o'); o.textContent += s + ';'; }"
        "document.getElementById('b').click(); document.getElementById('bad').click();"
        "var b = document.getElementById('b'); b.setAttribute('onclick', \"log('changed')\"); b.click();"
        "console.log(typeof b.onclick);"
        "</script></body>");
    CHECK_HAS(g_out, "<p id=\"o\">b click;changed;body;</p>");
    CHECK_HAS(logtext(), "error: SyntaxError in the onclick attribute: SyntaxError: ");
    CHECK_HAS(logtext(), "function\n");
    TEST_ASSERT_EQUAL(2, g_st.handlers);
}

static void test_l114_focus_api(void) {
    wf_jspage_set_fetch(0, 0);
    run("<body><input id=a><input id=b><script>"
        "var seq = [];"
        "['a','b'].forEach(function (id) { var e = document.getElementById(id);"
        "  e.addEventListener('focus', function () { seq.push('focus-' + id); });"
        "  e.addEventListener('blur', function () { seq.push('blur-' + id); }); });"
        "document.body.addEventListener('focusin', function (e) { seq.push('in-' + e.target.id); });"
        "console.log(document.activeElement.tagName);"
        "document.getElementById('a').focus(); document.getElementById('b').focus();"
        "console.log(document.activeElement.id, seq.join(' '));"
        "document.getElementById('b').blur(); console.log(document.activeElement.tagName);"
        "</script></body>");
    CHECK_STR("BODY\nb focus-a in-a blur-a focus-b in-b\nBODY\n", logtext());
}

int main(void) {
    unity_init();
    RUN_TEST(test_l114_external_order);
    RUN_TEST(test_l114_external_errors);
    RUN_TEST(test_l114_external_bound_and_no_hook);
    RUN_TEST(test_l114_dynamic_scripts);
    RUN_TEST(test_l114_microtasks_in_page);
    RUN_TEST(test_l114_fetch);
    RUN_TEST(test_l114_xhr);
    RUN_TEST(test_l114_net_bound);
    RUN_TEST(test_l114_layout);
    RUN_TEST(test_l114_live_click);
    RUN_TEST(test_l114_live_forms);
    RUN_TEST(test_l114_live_timers_and_custom);
    RUN_TEST(test_l114_handler_attrs);
    RUN_TEST(test_l114_focus_api);
    unity_print_results();
    unity_cleanup();
    return unity_stats.tests_failed == 0 ? 0 : 1;
}
