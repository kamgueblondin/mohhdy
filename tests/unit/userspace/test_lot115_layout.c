/* Lot 115 (US-031): layout values of the page scripts from the renderer
 * (wf_jslayout.c over webeng.c, the engine the GUI paints with) instead
 * of the Lot 114 block model: CSS sizes, padding, borders and flex rows
 * reach offsetWidth / clientWidth / getBoundingClientRect, a DOM change
 * lays the page out again, the elements past the node budget keep the
 * block model, and a refused layout falls back to it. The expected
 * numbers are the CSS box model (body margin 8 px, a 1024 px viewport). */
#include "../../framework/unity.h"
#include <string.h>
#include <stdio.h>
#include "wf_jsdom.h"
#include "wf_jslayout.h"
#include "umalloc.h"

#define ARENA (6u * 1024u * 1024u)
static unsigned char g_arena[ARENA] __attribute__((aligned(16)));
static unsigned char g_heap[3u * 1024u * 1024u] __attribute__((aligned(16)));
static char g_out[256 * 1024];
static wf_jspage_stats_t g_st;
static int g_rc, g_refuse;

static int grow(unsigned want) { return want <= sizeof(g_heap) ? (int)want : -1; }

static int refusing(void *ctx, const char *html, int n, int vw, int vh, wf_jslay_box_t *out, int max) {
    (void)ctx; (void)html; (void)n; (void)vw; (void)vh; (void)out; (void)max;
    g_refuse++;
    return -1;
}

static const char *run(const char *html) {
    g_rc = wf_jspage_run(g_arena, ARENA, html, (int)strlen(html), "http://10.0.2.2:8067/lot115/l.html", g_out,
                         (int)sizeof(g_out), &g_st);
    return g_rc > 0 ? g_out : "";
}

static const char *logtext(void) {
    int n;
    return wf_jspage_log(&n);
}

#define CHECK_STR(want, got) do { if (strcmp((want), (got)) != 0) { \
    printf("  want: [%s]\n  got:  [%s]\n", (want), (got)); TEST_ASSERT_TRUE(0); } } while (0)

#define REPORT "<script>var r = []; ['b', 'p', 'n'].forEach(function (i) { var e = document.getElementById(i);" \
    "r.push(i + ' ' + e.offsetLeft + ',' + e.offsetTop + ' ' + e.offsetWidth + 'x' + e.offsetHeight + ' c' +" \
    "e.clientWidth + 'x' + e.clientHeight); }); console.log(r.join(' | '));</script>"

/* width + padding + border, the next block below it, display none */
static void test_l115_box_model(void) {
    wf_jspage_set_layout(wf_jslayout_engine, 0);
    run("<html><head><style>#b { width: 200px; height: 50px; padding: 10px; border: 2px solid red } p { margin: 0 }"
        "</style></head><body><div id=b>box</div><p id=p>para</p><div id=n style='display:none'>x</div>" REPORT
        "</body></html>");
    CHECK_STR("b 8,8 224x74 c220x70 | p 8,82 1008x10 c1008x10 | n 0,0 0x0 c0x0\n", logtext());
    TEST_ASSERT_TRUE(g_st.layout_engine >= 4);
}

/* the same page without the hook: the Lot 114 block model (no CSS sizes) */
static void test_l115_model_without_hook(void) {
    wf_jspage_set_layout(0, 0);
    run("<html><head><style>#b { width: 200px; height: 50px; padding: 10px; border: 2px solid red } p { margin: 0 }"
        "</style></head><body><div id=b>box</div><p id=p>para</p><div id=n style='display:none'>x</div>" REPORT
        "</body></html>");
    CHECK_STR("b 8,8 1008x16 c1008x16 | p 8,24 1008x16 c1008x16 | n 0,0 0x0 c0x0\n", logtext());
    TEST_ASSERT_EQUAL(0, g_st.layout_engine);
}

/* a flex row (justify-content set: the engine's CSS flex path): items
 * keep their widths and sit side by side; a style change lays the page
 * out again; the rect follows */
static void test_l115_flex_and_relayout(void) {
    wf_jspage_set_layout(wf_jslayout_engine, 0);
    run("<body><div style='display:flex;justify-content:flex-start'><div id=a style='width:100px;height:30px'>A</div>"
        "<div id=b style='width:50px;height:20px'>B</div></div><script>"
        "var a = document.getElementById('a'), b = document.getElementById('b');"
        "console.log(a.offsetLeft, a.offsetTop, a.offsetWidth, a.offsetHeight, b.offsetLeft, b.offsetTop, b.offsetWidth, b.offsetHeight);"
        "a.style.width = '300px';"
        "console.log(a.offsetWidth, b.offsetLeft);"
        "var r = b.getBoundingClientRect(); console.log(r.left, r.top, r.right, r.bottom, r.width, r.height);"
        "</script></body>");
    CHECK_STR("8 8 100 30 108 8 50 20\n300 308\n308 8 358 28 50 20\n", logtext());
}

/* a plain flex row is laid out by the engine's legacy model (equal
 * shares, webeng_lot52 / lot64 tests): the scripts read what the GUI
 * paints, not what a browser would (said in docs/lot115_plan.md) */
static void test_l115_flex_legacy_is_what_the_gui_paints(void) {
    wf_jspage_set_layout(wf_jslayout_engine, 0);
    run("<body><div style='display:flex'><div id=a style='width:100px;height:30px'>A</div>"
        "<div id=b style='width:50px;height:20px'>B</div></div><script>"
        "var a = document.getElementById('a'), b = document.getElementById('b');"
        "console.log(a.offsetLeft, b.offsetLeft, b.offsetTop);</script></body>");
    CHECK_STR("8 512 8\n", logtext());
}

/* an inline element is the union of its painted text; clientWidth 0 */
static void test_l115_inline(void) {
    wf_jspage_set_layout(wf_jslayout_engine, 0);
    run("<body><p style='margin:0'>ab<span id=s>cdef</span></p><script>"
        "var s = document.getElementById('s');"
        "console.log(s.offsetWidth > 0, s.offsetHeight > 0, s.offsetLeft > 8, s.clientWidth, s.clientHeight);"
        "</script></body>");
    CHECK_STR("true true true 0 0\n", logtext());
}

/* past the node budget the block model answers; the document height
 * still covers both */
static void test_l115_budget(void) {
    wf_jspage_set_layout(wf_jslayout_engine, 0);
    run("<body><script>for (var i = 0; i < 200; i++) { var p = document.createElement('p'); p.textContent = 'p' + i;"
        "document.body.appendChild(p); }"
        "var ps = document.querySelectorAll('p');"
        "console.log(ps[0].offsetTop < ps[10].offsetTop, ps[10].offsetTop < ps[60].offsetTop,"
        "document.documentElement.scrollHeight >= ps[199].offsetTop);</script></body>");
    CHECK_STR("true true true\n", logtext());
    TEST_ASSERT_TRUE(g_st.layout_engine > 20);
    TEST_ASSERT_TRUE(g_st.layout_engine <= WF_JSLAY_NODES);
}

/* a hook that refuses: the block model, no error */
static void test_l115_refused(void) {
    g_refuse = 0;
    wf_jspage_set_layout(refusing, 0);
    run("<body><div id=d style='width:50px'>x</div><script>console.log(document.getElementById('d').offsetWidth);"
        "</script></body>");
    CHECK_STR("50\n", logtext());
    TEST_ASSERT_EQUAL(1, g_refuse);
    TEST_ASSERT_EQUAL(0, g_st.layout_engine);
    wf_jspage_set_layout(0, 0);
}

/* the hook alone: tags in document order, the engine's own counters */
static void test_l115_hook_direct(void) {
    static wf_jslay_box_t box[32];
    int made0, ref0, made1, ref1, n;
    const char *html = "<body><div style='width:40px;height:10px'></div><p>t</p></body>";
    wf_jslayout_counts(&made0, &ref0);
    n = wf_jslayout_engine(0, html, (int)strlen(html), 1024, 768, box, 32);
    wf_jslayout_counts(&made1, &ref1);
    TEST_ASSERT_TRUE(n >= 2);
    TEST_ASSERT_EQUAL(made0 + 1, made1);
    TEST_ASSERT_EQUAL(ref0, ref1);
    {
        int i, div = -1;
        for (i = 0; i < n; i++) if (!strcmp(box[i].tag, "div")) { div = i; break; }
        TEST_ASSERT_TRUE(div >= 0);
        TEST_ASSERT_EQUAL(40, box[div].w);
        TEST_ASSERT_EQUAL(10, box[div].h);
        TEST_ASSERT_EQUAL(1, box[div].ok);
    }
    TEST_ASSERT_EQUAL(-1, wf_jslayout_engine(0, html, -1, 1024, 768, box, 32));
    wf_jslayout_release();
}

int main(void) {
    unity_init();
    umalloc_init(g_heap, grow);
    RUN_TEST(test_l115_box_model);
    RUN_TEST(test_l115_model_without_hook);
    RUN_TEST(test_l115_flex_and_relayout);
    RUN_TEST(test_l115_flex_legacy_is_what_the_gui_paints);
    RUN_TEST(test_l115_inline);
    RUN_TEST(test_l115_budget);
    RUN_TEST(test_l115_refused);
    RUN_TEST(test_l115_hook_direct);
    wf_jslayout_release();
    unity_print_results();
    unity_cleanup();
    return unity_stats.tests_failed == 0 ? 0 : 1;
}
