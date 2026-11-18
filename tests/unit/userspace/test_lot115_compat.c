/* Lot 115 (US-031): real-page compatibility of the page script pass.
 *
 * tests/fixtures/lot115 holds fifteen small pages written like the
 * scripts of real sites (a cookie banner, tabs, a form validator, lazy
 * images, a mini jQuery, templating, an event bus, a cart in
 * localStorage...). Each page logs its results with console.log;
 * expected.txt is what a reference DOM (jsdom 24 on node v24) logs for
 * the same pages (scripts/lot115_record.js wrote it). The first test runs
 * every page through wf_jspage_run and compares the console line by
 * line: all fifteen pages must match.
 *
 * The other tests pin the engine and DOM pieces the fixtures needed,
 * each against what the same reference prints for the same page:
 * $<name> in replacement strings, toSorted/toReversed/with,
 * escape/unescape, raw template text and string-literal substitutions
 * in tagged templates; eventPhase and the KeyboardEvent, MouseEvent,
 * InputEvent and FocusEvent constructors; el.attributes, form.elements
 * (radio groups included), tabIndex, delete on dataset, srcset/sizes
 * reflection, template.content, and the HTML 4 character references. */
#include "../../framework/unity.h"
#include <string.h>
#include <stdio.h>
#include "wf_jsdom.h"

#define ARENA (6u * 1024u * 1024u)
static unsigned char g_arena[ARENA] __attribute__((aligned(16)));
static char g_out[256 * 1024];
static char g_html[64 * 1024];
static char g_expected[64 * 1024];
static char g_log[64 * 1024];
static wf_jspage_stats_t g_st;

#define BASE "http://10.0.2.2:8067/lot115/compat/"
#define FIXTURES 15
#define CHECK_MSG(cond, msg) do { if (!(cond)) printf("  lot115 compat: %s\n", (msg)); TEST_ASSERT_TRUE(cond); } while (0)

static const char *const k_pages[FIXTURES] = {
    "analytics_queue", "cookie_banner", "dom_traverse", "event_bus", "form_validation", "json_config",
    "lazy_images", "mini_jquery", "modern_es", "nav_menu", "storage_cart", "tabs", "templating",
    "text_utils", "url_state"
};

static int load(const char *name, char *buf, int cap) {
    static const char *const pre[] = {
        "tests/fixtures/lot115/", "fixtures/lot115/", "../fixtures/lot115/", "../../fixtures/lot115/",
        "../../../tests/fixtures/lot115/", 0
    };
    char path[256];
    int i;
    for (i = 0; pre[i]; i++) {
        FILE *f;
        int n;
        snprintf(path, sizeof(path), "%s%s", pre[i], name);
        f = fopen(path, "rb");
        if (!f) continue;
        n = (int)fread(buf, 1, (size_t)cap - 1, f);
        fclose(f);
        buf[n] = 0;
        return n;
    }
    return -1;
}

/* the page's console log, without the trailing newline */
static const char *run_page(const char *html, int n, const char *url, int *rc) {
    int ln;
    const char *log;
    *rc = wf_jspage_run(g_arena, ARENA, html, n, url, g_out, (int)sizeof(g_out), &g_st);
    log = wf_jspage_log(&ln);
    if (ln > (int)sizeof(g_log) - 1) ln = (int)sizeof(g_log) - 1;
    memcpy(g_log, log, (size_t)ln);
    while (ln > 0 && g_log[ln - 1] == '\n') ln--;
    g_log[ln] = 0;
    return g_log;
}

/* the lines recorded for name in expected.txt (after "== name"), into out */
static int expected_of(const char *name, char *out, int cap) {
    const char *p = g_expected;
    int nl = (int)strlen(name), o = 0;
    while (*p) {
        const char *e = strchr(p, '\n');
        int len = e ? (int)(e - p) : (int)strlen(p);
        if (len == nl + 3 && p[0] == '=' && p[1] == '=' && p[2] == ' ' && !memcmp(p + 3, name, (size_t)nl)) {
            p += len + (e ? 1 : 0);
            while (*p && !(p[0] == '=' && p[1] == '=' && p[2] == ' ')) {
                e = strchr(p, '\n');
                len = e ? (int)(e - p) : (int)strlen(p);
                if (o + len + 2 >= cap) return -1;
                if (o) out[o++] = '\n';
                memcpy(out + o, p, (size_t)len);
                o += len;
                p += len + (e ? 1 : 0);
            }
            out[o] = 0;
            return o;
        }
        p += len + (e ? 1 : 0);
    }
    return -1;
}

/* the first line where a and b differ, for the failure message */
static void first_diff(const char *a, const char *b, char *msg, int cap) {
    int line = 1;
    const char *la = a, *lb = b;
    while (*a && *a == *b) {
        if (*a == '\n') { line++; la = a + 1; lb = b + 1; }
        a++;
        b++;
    }
    snprintf(msg, (size_t)cap, "line %d: got [%.100s] want [%.100s]", line, la, lb);
    {
        char *q;
        for (q = msg; *q; q++) if (*q == '\n') *q = '/';
    }
}

static void test_fixture_pages_match_reference(void) {
    static char want[16 * 1024];
    char url[160], file[96], msg[400];
    int i, matched = 0, rc;
    CHECK_MSG(load("expected.txt", g_expected, (int)sizeof(g_expected)) > 0,
                             "tests/fixtures/lot115/expected.txt not found");
    for (i = 0; i < FIXTURES; i++) {
        const char *got;
        int n;
        snprintf(file, sizeof(file), "%s.html", k_pages[i]);
        n = load(file, g_html, (int)sizeof(g_html));
        CHECK_MSG(n > 0, file);
        CHECK_MSG(expected_of(k_pages[i], want, (int)sizeof(want)) >= 0, k_pages[i]);
        snprintf(url, sizeof(url), BASE "%s", file);
        got = run_page(g_html, n, url, &rc);
        if (rc >= 0 && g_st.failed == 0 && !strcmp(got, want)) { matched++; continue; }
        first_diff(got, want, msg, (int)sizeof(msg));
        printf("  lot115 compat: %s rc %d failed %d %s\n", k_pages[i], rc, g_st.failed, msg);
    }
    TEST_ASSERT_EQUAL(FIXTURES, matched);
}

/* the pages stay small: hand-written fixtures, not captured sites */
static void test_fixtures_stay_small(void) {
    char file[96];
    int i, total = 0;
    for (i = 0; i < FIXTURES; i++) {
        int n;
        snprintf(file, sizeof(file), "%s.html", k_pages[i]);
        n = load(file, g_html, (int)sizeof(g_html));
        CHECK_MSG(n > 0 && n < 4096, file);
        total += n;
    }
    TEST_ASSERT_TRUE(total < 40000);
}

static const char k_js_page[] =
    "<script>\n"
    "var o=[];\n"
    "o.push('2026-11-18'.replace(/(?<y>\\d+)-(?<m>\\d+)-(?<d>\\d+)/,'$<d>.$<m>.$<y>'),'a-b'.replace(/(?<x>-)/g,'[$<x>]'),'ab'.replace(/(?<x>a)/,'$<nope>|'),'ab'.replace(/(a)/,'$<x>'),'ab'.replace(/(?<x>a)/,'$<x'));\n"
    "var a=[3,1,2];o.push(a.toSorted().join(),a.join(),a.toReversed().join(),a.with(-1,9).join(),a.join(),[5,10,1].toSorted((x,y)=>x-y).join(),[,1].toSorted().length);\n"
    "try{a.with(3,0)}catch(e){o.push(e.constructor.name)}\n"
    "try{a.toSorted(1)}catch(e){o.push(e.constructor.name)}\n"
    "o.push(escape('a b+c/\xC3\xA9\xE2\x82\xAC\xF0\x9F\x98\x80@*_-.'),unescape('%u20AC%E9%20x%zz%u12'),unescape(escape('mix\xC3\xA9\xE2\x82\xAC\xF0\x9F\x98\x80')) === 'mix\xC3\xA9\xE2\x82\xAC\xF0\x9F\x98\x80');\n"
    "function tag(s,...v){return JSON.stringify([s,s.raw,v])}\n"
    "o.push(tag`x${'y'}z${1}\\n`,String.raw`a\\tb${2}c\\u0041`,String.raw({raw:['p','q']},0),`l1\n"
    "l2`.length);\n"
    "console.log(o.join(' # '));\n"
    "</script>\n";
static const char k_js_want[] =
    "18.11.2026 # a[-]b # |b # $<x>b # $<xb # 1,2,3 # 3,1,2 # 2,1,3 # 3,1,9 # 3,1,2 # 1,5,10 # 2 # RangeError # TypeError # a%20b+c/%E9%u20AC%uD83D%uDE00@*_-. # \xE2\x82\xAC\xC3\xA9 x%zz%u12 # true # [[\"x\",\"z\",\"\\n\"],[\"x\",\"z\",\"\\\\n\"],[\"y\",1]] # a\\tb2c\\u0041 # p0q # 5";

static const char k_ev_page[] =
    "<div id=a><button id=b>x</button></div><script>\n"
    "var log=[],a=document.getElementById('a'),b=document.getElementById('b');\n"
    "a.addEventListener('k',function(e){log.push('a'+e.eventPhase+(e.currentTarget===a))});\n"
    "b.addEventListener('k',function(e){log.push('b'+e.eventPhase+(e.target===b))});\n"
    "var e=new CustomEvent('k',{bubbles:true,detail:7});log.push('p'+e.eventPhase);b.dispatchEvent(e);log.push('q'+e.eventPhase+e.detail);\n"
    "var k=new KeyboardEvent('keydown',{key:'Enter',ctrlKey:true,keyCode:13});\n"
    "log.push(k instanceof KeyboardEvent,k instanceof Event,k.key,k.code==='',k.ctrlKey,k.shiftKey,k.keyCode,k.bubbles,k.repeat);\n"
    "var m=new MouseEvent('click',{clientX:5,button:1,bubbles:true});log.push(m instanceof MouseEvent,m instanceof KeyboardEvent,m.clientX,m.clientY,m.button,m.relatedTarget,m.bubbles);\n"
    "var f=new FocusEvent('focus');log.push(f.relatedTarget,f.type,typeof InputEvent,new InputEvent('input',{data:'z'}).data,new InputEvent('input').inputType==='');\n"
    "console.log(log.join(' '));\n"
    "</script>\n";
static const char k_ev_want[] =
    "p0 b2true a3true q07 true true Enter true true false 13 false false true false 5 0 1  true  focus function z true";

static const char k_dom_page[] =
    "<form id=f action=\"/s\"><input name=a id=x value=1><input type=image name=img><select name=s><option>o</option></select>\n"
    "<input type=radio name=r value=one><input type=radio name=r value=two checked><textarea name=t>tt</textarea><button>go</button></form>\n"
    "<p id=p data-big-name=\"v\" title=\"T&amp;C\" class=c>Caf&eacute; &hearts; &ucirc;&nbsp;&lt;x&gt;</p>\n"
    "<a id=l href=\"/x\">l</a><div id=d tabindex=\"3\">d</div><span id=s>s</span>\n"
    "<template id=t><li class=i>one</li><li class=i>two</li></template>\n"
    "<script>\n"
    "var f=document.getElementById('f'),p=document.getElementById('p'),out=[];\n"
    "out.push(f.elements.length,f.elements[0].name,f.elements.a.value,f.elements.r.length,f.elements.r.value,f.elements.img,f.elements[5].name,f.elements.x===f.elements.a);\n"
    "var at=p.attributes;out.push(at.length,at[0].name+'='+at[0].value,at[2].name+'='+at[2].value,at.title.value,at[9]);\n"
    "out.push(p.textContent.length,p.textContent);\n"
    "out.push(document.getElementById('l').tabIndex,document.getElementById('d').tabIndex,document.getElementById('s').tabIndex,f.elements.a.tabIndex);\n"
    "var s=document.getElementById('s');s.tabIndex=-1;out.push(s.getAttribute('tabindex'),s.tabIndex);\n"
    "out.push(p.dataset.bigName,delete p.dataset.bigName,p.hasAttribute('data-big-name'),p.dataset.bigName);\n"
    "var t=document.getElementById('t');out.push(document.querySelectorAll('li').length,t.content.childNodes.length,t.content.nodeType,t.childNodes.length);\n"
    "var c=t.content.cloneNode(true);document.body.appendChild(c);out.push(document.querySelectorAll('li').length,t.content.childNodes.length,t.innerHTML);\n"
    "var img=document.createElement('img');img.srcset='a.png 1x, b.png 2x';img.sizes='100vw';out.push(img.getAttribute('srcset'),img.sizes);\n"
    "console.log(out.join('|'));\n"
    "</script>\n"
    "<script>var t=document.getElementById('t');t.innerHTML='<b>n</b>';console.log(t.content.childNodes.length,t.innerHTML,t.childNodes.length,'&spades;'.length,document.createElement('div').innerHTML='x');\n"
    "var q=document.createElement('p');q.innerHTML='&spades;&rArr;&amp;&#x41;';console.log(q.textContent);</script>\n";
static const char k_dom_want[] =
    "6|a|1|2|two|||true|4|id=p|title=T&C|T&C||12|Caf\xC3\xA9 \xE2\x99\xA5 \xC3\xBB\xC2\xA0<x>|0|3|-1|0|-1|-1|v|true|false||0|2|11|0|2|2|<li class=\"i\">one</li><li class=\"i\">two</li>|a.png 1x, b.png 2x|100vw\n"
    "1 <b>n</b> 0 8 x\n"
    "\xE2\x99\xA0\xE2\x87\x92&A";

static void check_page(const char *page, const char *want) {
    int rc;
    const char *got = run_page(page, (int)strlen(page), BASE "t.html", &rc);
    TEST_ASSERT_TRUE(rc >= 0);
    TEST_ASSERT_EQUAL(0, g_st.failed);
    TEST_ASSERT_EQUAL_STRING(want, got);
}

static void test_engine_additions(void) { check_page(k_js_page, k_js_want); }
static void test_ui_events(void) { check_page(k_ev_page, k_ev_want); }
static void test_dom_details(void) { check_page(k_dom_page, k_dom_want); }

/* a page that is truncated or throws in a fixture would still be caught:
 * the comparison is on the whole log, not on a prefix */
static void test_diff_is_strict(void) {
    char msg[200];
    first_diff("a\nb\nc", "a\nb\nd", msg, (int)sizeof(msg));
    TEST_ASSERT_EQUAL_STRING("line 3: got [c] want [d]", msg);
    TEST_ASSERT_TRUE(strcmp("a\nb", "a\nb\nc") != 0);
}

int main(void) {
    unity_init();
    RUN_TEST(test_fixture_pages_match_reference);
    RUN_TEST(test_fixtures_stay_small);
    RUN_TEST(test_engine_additions);
    RUN_TEST(test_ui_events);
    RUN_TEST(test_dom_details);
    RUN_TEST(test_diff_is_strict);
    unity_print_results();
    unity_cleanup();
    return unity_stats.tests_failed == 0 ? 0 : 1;
}
