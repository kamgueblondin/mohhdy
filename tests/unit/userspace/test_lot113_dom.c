/* Lot 113 (US-031): the page script pass (wf_jsdom.c) and the js/web-js
 * commands (wf_jsweb.c) on the host: the tolerant HTML parser and
 * serializer, the DOM API scripts use most, selectors, events, timers,
 * document.write, the bounds of the pass, and the shared command path. */
#include "../../framework/unity.h"
#include <string.h>
#include <stdio.h>
#include "wf_jsdom.h"
#include "wf_jsweb.h"
#include "umalloc.h"

#define ARENA (4u * 1024u * 1024u)
static unsigned char g_arena[ARENA] __attribute__((aligned(16)));
static char g_out[256 * 1024];
static wf_jspage_stats_t g_st;
static int g_rc;

static const char *run(const char *html) {
    g_rc = wf_jspage_run(g_arena, ARENA, html, (int)strlen(html), "http://10.0.2.2:8067/lot113/a.html?q=1#frag",
                         g_out, (int)sizeof(g_out), &g_st);
    return g_rc > 0 ? g_out : "";
}

static const char *logtext(void) {
    static char b[8192];
    int n;
    const char *l = wf_jspage_log(&n);
    if (n > (int)sizeof(b) - 1) n = (int)sizeof(b) - 1;
    memcpy(b, l, (size_t)n);
    b[n] = 0;
    return b;
}

/* a string check that shows both sides when they differ */
#define CHECK_STR(want, got) do { const char *w_ = (want), *g_ = (got); \
    if (strcmp(w_, g_) != 0) printf("\n  want: [%s]\n  got:  [%s]\n", w_, g_); \
    TEST_ASSERT_EQUAL_STRING(w_, g_); } while (0)

static const char *rt(const char *html) {
    int n = wf_jsdom_roundtrip(html, (int)strlen(html), g_out, (int)sizeof(g_out));
    return n >= 0 ? g_out : "(fail)";
}

static void test_l113_roundtrip(void) {
    TEST_ASSERT_EQUAL_STRING("<html><head></head><body><p>a</p></body></html>", rt("<p>a"));
    TEST_ASSERT_EQUAL_STRING("<html><head><title>T &amp; U</title></head><body><p>x</p><p>y</p></body></html>",
                             rt("<title>T &amp; U</title><p>x<p>y"));
    TEST_ASSERT_EQUAL_STRING("<!DOCTYPE html><html><head></head><body><ul><li>1</li><li>2</li></ul><br><img src=\"a.png\" alt=\"\"></body></html>",
                             rt("<!DOCTYPE html><ul><li>1<li>2</ul><br><img src=a.png alt>"));
    TEST_ASSERT_EQUAL_STRING("<html><head></head><body><div a=\"1 &quot;q&quot;\" b=\"x\"><!-- c --><script>if (a<b && c>d) {}</script></div></body></html>",
                             rt("<div a='1 \"q\"' B=x><!-- c --><script>if (a<b && c>d) {}</script></div>"));
    TEST_ASSERT_EQUAL_STRING("<html><head></head><body><table><tr><td>1</td><td>2</td></tr></table>end</body></html>",
                             rt("<table><tr><td>1<td>2</tr></table>end</b>"));
}

static void test_l113_no_script(void) {
    run("<html><body><p>static</p><noscript>no js</noscript></body></html>");
    TEST_ASSERT_EQUAL(0, g_rc);
    run("<html><body><script src=\"app.js\"></script><script type=\"application/ld+json\">{}</script></body></html>");
    TEST_ASSERT_EQUAL(0, g_rc);         /* nothing inline ran: the page is kept */
    TEST_ASSERT_EQUAL(1, g_st.external);
    TEST_ASSERT_EQUAL(1, g_st.other_type);
}

static void test_l113_dom_edit(void) {
    const char *o = run(
        "<!doctype html><html><head><title>Demo</title></head><body>"
        "<h1 id=t>Static</h1><ul id=list></ul><p class='a b'>x &amp; y</p><noscript>JS is off</noscript>"
        "<script>"
        "var ul = document.getElementById('list');"
        "['alpha','beta'].forEach(function (s, i) { var li = document.createElement('li');"
        "  li.textContent = (i + 1) + '. ' + s + ' <b>'; li.className = 'item'; ul.appendChild(li); });"
        "document.querySelector('#t').innerHTML = 'Dynamic <em>title</em>';"
        "var p = document.querySelector('p.a'); p.classList.add('c'); p.classList.remove('a');"
        "p.style.color = 'red'; p.style.fontSize = '12px'; p.dataset.userId = '7'; p.setAttribute('title', 'a\"b');"
        "document.title = 'New ' + document.title;"
        "console.log(document.querySelectorAll('li.item').length, p.textContent, p.style.color, p.className);"
        "</script></body></html>");
    TEST_ASSERT_TRUE(g_rc > 0);
    TEST_ASSERT_NOT_NULL(strstr(o, "<h1 id=\"t\">Dynamic <em>title</em></h1>"));
    TEST_ASSERT_NOT_NULL(strstr(o, "<ul id=\"list\"><li class=\"item\">1. alpha &lt;b&gt;</li><li class=\"item\">2. beta &lt;b&gt;</li></ul>"));
    TEST_ASSERT_NOT_NULL(strstr(o, "<p class=\"b c\" style=\"color: red; font-size: 12px;\" data-user-id=\"7\" title=\"a&quot;b\">x &amp; y</p>"));
    TEST_ASSERT_NOT_NULL(strstr(o, "<title>New Demo</title>"));
    TEST_ASSERT_NULL(strstr(o, "<script"));
    TEST_ASSERT_NULL(strstr(o, "JS is off"));          /* scripts ran: noscript is not shown */
    TEST_ASSERT_EQUAL_STRING("2 x & y red b c\n", logtext());
    TEST_ASSERT_EQUAL(1, g_st.scripts);
    TEST_ASSERT_EQUAL(1, g_st.ran);
    TEST_ASSERT_EQUAL(0, g_st.failed);
}

static void test_l113_selectors(void) {
    run("<body><div id=a class='x y'><p>1</p><span class=x>2</span><p title='t-1'>3</p></div>"
        "<div><p lang=en-US>4</p><input type=checkbox checked><a href='http://e/x.pdf'>5</a></div>"
        "<script>"
        "var q = s => document.querySelectorAll(s).length;"
        "console.log(q('p'), q('div > p'), q('#a p'), q('.x'), q('div.x.y'), q('span + p'), q('p ~ p'), q('[title]'),"
        " q('[title^=t-]'), q('[href$=\".pdf\"]'), q('[lang|=en]'), q('p:first-child'), q('p:last-child'), q('div:not(.x) p'),"
        " q('p, span'), q('*') > 10, q('input:checked'));"
        "var a = document.getElementById('a');"
        "console.log(a.querySelector('p:last-child').textContent, document.querySelector('span').closest('div').id,"
        " document.querySelector('span').matches('#a > .x'), document.getElementsByTagName('P').length,"
        " document.getElementsByClassName('x').length, a.children.length, a.childElementCount, a.firstElementChild.tagName);"
        "try { document.querySelector('') } catch (e) { console.log(e.name) }"
        "</script></body>");
    CHECK_STR("3 3 2 2 1 1 1 1 1 1 1 2 1 1 4 true 1\n3 a true 3 2 3 3 P\nSyntaxError\n", logtext());
}

static void test_l113_events(void) {
    run("<body><div id=o><button id=b>go</button></div><script>"
        "var log = [], b = document.getElementById('b'), o = document.getElementById('o');"
        "o.addEventListener('click', e => log.push('outer:' + e.target.id + ':' + e.currentTarget.id));"
        "b.addEventListener('click', function (e) { log.push('btn:' + (this === b)); });"
        "b.addEventListener('click', () => log.push('once'), {once: true});"
        "b.onclick = () => log.push('prop');"
        "b.click(); b.click();"
        "var h = () => log.push('removed'); b.addEventListener('x', h); b.removeEventListener('x', h);"
        "b.dispatchEvent(new Event('x'));"
        "o.addEventListener('stop', e => log.push('never'));"
        "b.addEventListener('stop', e => { e.stopPropagation(); log.push('stopped') });"
        "b.dispatchEvent(new Event('stop', {bubbles: true}));"
        "b.addEventListener('c', e => e.preventDefault());"
        "log.push(b.dispatchEvent(new CustomEvent('c', {cancelable: true, detail: 5})));"
        "document.addEventListener('ce', e => log.push('detail:' + e.detail));"
        "document.dispatchEvent(new CustomEvent('ce', {detail: {n: 1}}.detail ? {detail: 'n'} : {}));"
        "b.addEventListener('t', () => { throw new Error('in listener') }); b.addEventListener('t', () => log.push('after throw'));"
        "b.dispatchEvent(new Event('t'));"
        "console.log(log.join(' '));"
        "</script></body>");
    CHECK_STR("error: Uncaught Error: in listener\n"
              "btn:true once prop outer:b:o btn:true prop outer:b:o stopped false detail:n after throw\n", logtext());
}

static void test_l113_load_and_timers(void) {
    run("<body onload=\"console.log('body onload', document.readyState)\"><p id=p>0</p><script>"
        "var p = document.getElementById('p');"
        "document.addEventListener('DOMContentLoaded', () => console.log('dcl', document.readyState));"
        "window.addEventListener('load', () => console.log('load'));"
        "console.log('inline', document.readyState);"
        "setTimeout(() => console.log('t20'), 20);"
        "var id = setTimeout(() => console.log('cleared'), 5); clearTimeout(id);"
        "setTimeout(() => { console.log('t0'); setTimeout(() => console.log('nested'), 0) }, 0);"
        "var n = 0; setInterval(() => { p.textContent = ++n }, 10);"
        "requestAnimationFrame(t => console.log('raf', typeof t));"
        "</script></body>");
    CHECK_STR("inline loading\ndcl interactive\nbody onload complete\nload\nt0\nnested\nraf number\nt20\n", logtext());
    TEST_ASSERT_NOT_NULL(strstr(g_out, "<p id=\"p\">3</p>"));       /* the interval ran WF_DOM_INTERVAL_RUNS times */
    TEST_ASSERT_EQUAL(3, WF_DOM_INTERVAL_RUNS);
    TEST_ASSERT_EQUAL(7, g_st.timers);
}

static void test_l113_write(void) {
    const char *o = run("<body><p>a</p><script>document.write('<p id=w>b</p><script>document.write(\"<i>c</i>\")<\\/script>');</script>"
                        "<div id=d></div><script>document.getElementById('d').innerHTML = '<script>console.log(\"no\")<\\/script>x';"
                        "console.log(document.getElementById('w').textContent)</script></body>");
    TEST_ASSERT_NOT_NULL(strstr(o, "<p>a</p><p id=\"w\">b</p><i>c</i><div id=\"d\">x</div>"));
    TEST_ASSERT_EQUAL_STRING("b\n", logtext());
    TEST_ASSERT_EQUAL(3, g_st.scripts);          /* the written script ran, the innerHTML one did not */
    TEST_ASSERT_EQUAL(2, g_st.writes);
}

static void test_l113_errors_and_bounds(void) {
    const char *o = run("<body><p id=p>k</p><script>missing()</script><script>for(;;){}</script>"
                        "<script>document.getElementById('p').textContent = 'still runs'</script></body>");
    TEST_ASSERT_NOT_NULL(strstr(o, "<p id=\"p\">still runs</p>"));
    TEST_ASSERT_EQUAL(3, g_st.scripts);
    TEST_ASSERT_EQUAL(1, g_st.ran);
    TEST_ASSERT_EQUAL(2, g_st.failed);
    TEST_ASSERT_EQUAL_STRING("Uncaught ReferenceError: missing is not defined", g_st.first_err);
    TEST_ASSERT_NOT_NULL(strstr(logtext(), "step budget"));
    /* the node pool: a script that creates more than WF_DOM_NODES nodes */
    o = run("<body><div id=d></div><script>var d = document.getElementById('d'), n = 0;"
            "try { for (;;) { d.appendChild(document.createElement('i')); n++ } } catch (e) { console.log(e.name, n > 5000) }"
            "</script></body>");
    TEST_ASSERT_TRUE(g_rc > 0);
    TEST_ASSERT_EQUAL(1, g_st.truncated);
    CHECK_STR("RangeError true\n", logtext());
    /* a page larger than the output buffer is reported as cut */
    {
        static char big[64 * 1024];
        int i, n = 0;
        const char *h = "<body><script>document.body.innerHTML += 'z'.repeat(70000)</script>";
        for (i = 0; h[i]; i++) big[n++] = h[i];
        big[n] = 0;
        g_rc = wf_jspage_run(g_arena, ARENA, big, n, 0, g_out, 4096, &g_st);
        TEST_ASSERT_EQUAL(4095, g_rc);
        TEST_ASSERT_EQUAL(1, g_st.truncated);
    }
    TEST_ASSERT_EQUAL(-1, wf_jspage_run(g_arena, 100000u, "<script>1</script>", 18, 0, g_out, 100, &g_st));
}

static void test_l113_window(void) {
    run("<body><script>"
        "console.log(location.hostname, location.port, location.pathname, location.search, location.hash, location.origin);"
        "document.cookie = 'a=1; max-age=60'; document.cookie = 'b=2'; document.cookie = 'a=3';"
        "localStorage.setItem('k', 42); sessionStorage.setItem('s', 'v');"
        "console.log(document.cookie, localStorage.getItem('k') === '42', localStorage.length, localStorage.getItem('none'));"
        "console.log(btoa('Mohhdy!'), atob(btoa('ab')).length, typeof fetch, typeof XMLHttpRequest, navigator.userAgent.indexOf('Mohhdy') > 0);"
        "console.log(window === self, window.document === document, document.defaultView === window, innerWidth, matchMedia('(min-width: 1px)').matches);"
        "alert('hi'); console.log(confirm('?'), prompt('?'));"
        "console.log(document.body.tagName, document.head.nodeName, document.documentElement.nodeType, document.nodeType, document.body.parentNode.tagName);"
        "</script></body>");
    CHECK_STR("10.0.2.2 8067 /lot113/a.html ?q=1 #frag http://10.0.2.2:8067\n"
                             "a=3; b=2 true 1 null\n"
                             "TW9oaGR5IQ== 2 function function true\n"   /* Lot 114: fetch and XMLHttpRequest */
                             "true true true 1024 false\n"
                             "alert: hi\nconfirm: ?\nprompt: ?\nfalse null\n"
                             "BODY HEAD 1 9 HTML\n", logtext());
}

static void test_l113_tree_ops(void) {
    run("<body><ul id=u><li>1</li><li>2</li><li>3</li></ul><script>"
        "var u = document.getElementById('u'), li = u.children;"
        "u.insertBefore(li[2], li[0]);"
        "var c = li[0].cloneNode(true); c.textContent += 'c'; u.appendChild(c);"
        "u.replaceChild(document.createTextNode('T'), li[1]);"
        "li[1].remove(); u.prepend('P'); u.append(document.createElement('hr'), 'E');"
        "var f = document.createDocumentFragment(); f.appendChild(document.createElement('b')); u.appendChild(f);"
        "u.insertAdjacentHTML('beforeend', '<em>h</em>'); u.firstChild.after('A');"
        "try { u.appendChild(u) } catch (e) { console.log('cycle caught') }"
        "console.log(u.innerHTML, u.childNodes.length, u.contains(f), u.textContent);"
        "</script></body>");
    CHECK_STR("cycle caught\nPA<li>3</li>T<li>3c</li><hr>E<b></b><em>h</em> 9 false PA3T3cEh\n", logtext());
}

/* the commands, as the console and the GUI input line call them */
static unsigned char g_heap[12u * 1024u * 1024u] __attribute__((aligned(16)));
static int grow(unsigned want) {
    unsigned got = (want + 4095U) & ~4095U;
    return got > sizeof(g_heap) ? -1 : (int)got;
}

static void test_l113_commands(void) {
    static char out[4096], body[8192];
    const char *page = "<html><body><p id=p>before</p><script>document.getElementById('p').textContent='after';console.log('page log')</script></body></html>";
    int n;
    umalloc_init(g_heap, grow);
    TEST_ASSERT_EQUAL(0, wf_jsweb_command("web-search x", out, (int)sizeof(out)));
    TEST_ASSERT_EQUAL(1, wf_jsweb_command("js", out, (int)sizeof(out)));
    TEST_ASSERT_NOT_NULL(strstr(out, "osui js usage"));
    out[0] = 0;
    TEST_ASSERT_EQUAL(1, wf_jsweb_command("js var q = [1, 2, 3].map(x => x * 2); console.log(\"hi\", q.length)", out, (int)sizeof(out)));
    TEST_ASSERT_NOT_NULL(strstr(out, "osui js log: hi 3\nosui js => undefined\n"));
    out[0] = 0;
    wf_jsweb_command("js q.join('+')", out, (int)sizeof(out));       /* the realm persists */
    TEST_ASSERT_NOT_NULL(strstr(out, "osui js => '2+4+6'\n"));
    out[0] = 0;
    wf_jsweb_command("js throw new TypeError('bad')", out, (int)sizeof(out));
    TEST_ASSERT_NOT_NULL(strstr(out, "osui js error=thrown Uncaught TypeError: bad\n"));
    out[0] = 0;
    wf_jsweb_command("js while (true) {}", out, (int)sizeof(out));
    TEST_ASSERT_NOT_NULL(strstr(out, "osui js error=aborted RangeError: script step budget exhausted (guest bound)"));
    out[0] = 0;
    wf_jsweb_command("web-js reset", out, (int)sizeof(out));
    out[0] = 0;
    wf_jsweb_command("js typeof q", out, (int)sizeof(out));
    TEST_ASSERT_NOT_NULL(strstr(out, "osui js => 'undefined'"));

    /* off by default: the page is untouched */
    out[0] = 0;
    wf_jsweb_command("web-js status", out, (int)sizeof(out));
    TEST_ASSERT_NOT_NULL(strstr(out, "osui web-js off pages 0"));
    strcpy(body, page);
    n = wf_jsweb_page(body, (int)strlen(body), (int)sizeof(body), "http://h/");
    TEST_ASSERT_EQUAL((int)strlen(page), n);
    TEST_ASSERT_NOT_NULL(strstr(body, "before"));
    out[0] = 0;
    wf_jsweb_command("web-js on", out, (int)sizeof(out));
    TEST_ASSERT_EQUAL(1, wf_jsweb_enabled());
    n = wf_jsweb_page(body, (int)strlen(body), (int)sizeof(body), "http://h/");
    TEST_ASSERT_EQUAL((int)strlen(body), n);
    TEST_ASSERT_NOT_NULL(strstr(body, "<p id=\"p\">after</p>"));
    TEST_ASSERT_NULL(strstr(body, "<script"));
    out[0] = 0;
    wf_jsweb_command("web-js status", out, (int)sizeof(out));
    TEST_ASSERT_NOT_NULL(strstr(out, "osui web-js on pages 1 last rc "));
    TEST_ASSERT_NOT_NULL(strstr(out, " scripts 1 ran 1 failed 0 "));
    out[0] = 0;
    wf_jsweb_command("web-js log", out, (int)sizeof(out));
    TEST_ASSERT_EQUAL_STRING("osui web-js log: page log\n", out);
    out[0] = 0;
    wf_jsweb_note(out, (int)sizeof(out));
    TEST_ASSERT_EQUAL_STRING(" js scripts=1 ran=1 failed=0", out);
    out[0] = 0;
    wf_jsweb_command("web-js off", out, (int)sizeof(out));
    TEST_ASSERT_EQUAL(0, wf_jsweb_enabled());
    out[0] = 0;
    wf_jsweb_command("web-js nonsense", out, (int)sizeof(out));
    TEST_ASSERT_NOT_NULL(strstr(out, "usage"));
}

int main(void) {
    unity_init();
    RUN_TEST(test_l113_roundtrip);
    RUN_TEST(test_l113_no_script);
    RUN_TEST(test_l113_dom_edit);
    RUN_TEST(test_l113_selectors);
    RUN_TEST(test_l113_events);
    RUN_TEST(test_l113_load_and_timers);
    RUN_TEST(test_l113_write);
    RUN_TEST(test_l113_errors_and_bounds);
    RUN_TEST(test_l113_window);
    RUN_TEST(test_l113_tree_ops);
    RUN_TEST(test_l113_commands);
    unity_print_results();
    unity_cleanup();
    return unity_stats.tests_failed == 0 ? 0 : 1;
}
