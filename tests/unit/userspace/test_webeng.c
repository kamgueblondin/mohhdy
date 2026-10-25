/* Lot 6 (US-031, US-043, US-045): guest web engine, parse + style + layout
 * + display list, inspector and diagnostics. */
#include "../../framework/unity.h"
#include "../../../userspace/webeng.h"
#include <string.h>

static we_doc_t g_doc;

static const char* PAGE =
    "<!DOCTYPE html><html><head><title>Demo page</title>"
    "<style>body{margin:0} #hero{background:#ff0000;width:200px;height:40px;padding:4px}"
    " .note{color:#00ff00} div p{font-weight:bold} .hide{display:none} p.c{text-align:center}"
    " @media (max-width:10px){p{color:red}}</style></head>"
    "<body><div id=hero>Hi</div>"
    "<div><p class=note>green bold</p></div>"
    "<p class=hide>hidden</p>"
    "<p>one &amp; two<br>three</p>"
    "<ul><li>a<li>b</ul>"
    "<script>alert(1)</script>"
    "<p class=c style='color:#0000ff'>mid</p>"
    "</body></html>";

static const os_fb_web_item_t* find_text(const char* t) {
    uint32_t i;
    for (i = 0; i < g_doc.nitems; i++)
        if (g_doc.items[i].kind == OS_FB_WEB_TEXT && strcmp(g_doc.items[i].text, t) == 0) return &g_doc.items[i];
    return 0;
}

static void test_parse_tree_and_title(void) {
    int hero, li;
    TEST_ASSERT_EQUAL(0, we_parse(&g_doc, PAGE, (int)strlen(PAGE)));
    TEST_ASSERT_EQUAL(0, strcmp(g_doc.title, "Demo page"));
    TEST_ASSERT_EQUAL(1U, g_doc.perf.scripts_skipped);
    hero = we_find_id(&g_doc, "hero");
    TEST_ASSERT_TRUE(hero > 0);
    TEST_ASSERT_EQUAL(0, strcmp(g_doc.nodes[g_doc.nodes[hero].parent].tag, "body"));
    /* implied </li>: the two li are siblings */
    for (li = 0; li < (int)g_doc.nnodes; li++) if (strcmp(g_doc.nodes[li].tag, "li") == 0) break;
    TEST_ASSERT_TRUE(li < (int)g_doc.nnodes);
    TEST_ASSERT_TRUE(g_doc.nodes[li].next > 0);
    TEST_ASSERT_EQUAL(0, strcmp(g_doc.nodes[g_doc.nodes[li].next].tag, "li"));
    TEST_ASSERT_TRUE(g_doc.css_len > 20);
}

static void test_cascade(void) {
    int hero, i, note = -1, hide = -1, c = -1;
    TEST_ASSERT_EQUAL(0, we_parse(&g_doc, PAGE, (int)strlen(PAGE)));
    TEST_ASSERT_EQUAL(0, we_style(&g_doc));
    TEST_ASSERT_TRUE(g_doc.nrules >= 5);
    hero = we_find_id(&g_doc, "hero");
    TEST_ASSERT_EQUAL(1, g_doc.nodes[hero].has_bg);
    TEST_ASSERT_EQUAL(0xFF0000U, g_doc.nodes[hero].bg);
    TEST_ASSERT_EQUAL(200, g_doc.nodes[hero].width);
    for (i = 0; i < (int)g_doc.nnodes; i++) {
        if (strcmp(g_doc.nodes[i].cls, "note") == 0) note = i;
        if (strcmp(g_doc.nodes[i].cls, "hide") == 0) hide = i;
        if (strcmp(g_doc.nodes[i].cls, "c") == 0) c = i;
    }
    TEST_ASSERT_EQUAL(0x00FF00U, g_doc.nodes[note].color);
    TEST_ASSERT_EQUAL(1, g_doc.nodes[note].bold);           /* descendant div p */
    TEST_ASSERT_EQUAL(WE_DISP_NONE, g_doc.nodes[hide].display);
    TEST_ASSERT_EQUAL(0x0000FFU, g_doc.nodes[c].color);     /* inline style wins */
    TEST_ASSERT_EQUAL(WE_ALIGN_CENTER, g_doc.nodes[c].align);
    TEST_ASSERT_EQUAL(0, g_doc.nodes[c].bold);               /* not inside a div */
}

static void test_layout_and_display_list(void) {
    const os_fb_web_item_t *hi, *green, *one, *three, *mid;
    int hero;
    uint32_t i;
    int saw_red = 0;
    TEST_ASSERT_EQUAL(0, we_render(&g_doc, PAGE, (int)strlen(PAGE), 400));
    hero = we_find_id(&g_doc, "hero");
    /* body margin 0: hero at 0,0, 200 + 2*4 padding wide, 40 + 8 high */
    TEST_ASSERT_EQUAL(0, g_doc.nodes[hero].x);
    TEST_ASSERT_EQUAL(0, g_doc.nodes[hero].y);
    TEST_ASSERT_EQUAL(208, g_doc.nodes[hero].w);
    TEST_ASSERT_EQUAL(48, g_doc.nodes[hero].h);
    for (i = 0; i < g_doc.nitems; i++)
        if (g_doc.items[i].kind == OS_FB_WEB_RECT && g_doc.items[i].color == 0xFF0000U &&
            g_doc.items[i].w == 208 && g_doc.items[i].h == 48) saw_red = 1;
    TEST_ASSERT_EQUAL(1, saw_red);
    hi = find_text("Hi");
    TEST_ASSERT_NOT_NULL(hi);
    TEST_ASSERT_EQUAL(4, hi->x);
    TEST_ASSERT_EQUAL(5, hi->y);
    green = find_text("green bold");
    TEST_ASSERT_NOT_NULL(green);
    TEST_ASSERT_EQUAL(1, green->bold);
    TEST_ASSERT_EQUAL(0x00FF00U, green->color);
    TEST_ASSERT_TRUE(green->y > 48);
    TEST_ASSERT_NULL(find_text("hidden"));
    TEST_ASSERT_NULL(find_text("alert(1)"));
    one = find_text("one & two");
    three = find_text("three");
    TEST_ASSERT_NOT_NULL(one);
    TEST_ASSERT_NOT_NULL(three);
    TEST_ASSERT_EQUAL(one->x, three->x);
    TEST_ASSERT_EQUAL(one->y + WE_LINE_H, three->y);   /* <br> */
    mid = find_text("mid");
    TEST_ASSERT_NOT_NULL(mid);
    TEST_ASSERT_EQUAL((400 - 3 * 8) / 2, mid->x);      /* centered */
    TEST_ASSERT_EQUAL(0x0000FFU, mid->color);
    TEST_ASSERT_TRUE(g_doc.perf.page_h > (uint32_t)mid->y);
    TEST_ASSERT_TRUE(g_doc.perf.lines >= 6);
    TEST_ASSERT_EQUAL(g_doc.nitems, g_doc.perf.items);
}

static void test_wrapping_and_borders(void) {
    static const char* p = "<body style='margin:0'><div style='width:80px;border:2px solid #000;padding:2px'>aaa bbb ccc ddd</div></body>";
    const os_fb_web_item_t* a;
    uint32_t i, borders = 0;
    TEST_ASSERT_EQUAL(0, we_render(&g_doc, p, (int)strlen(p), 300));
    /* content width 80 px = 10 chars: "aaa bbb" then "ccc ddd" */
    a = find_text("aaa bbb");
    TEST_ASSERT_NOT_NULL(a);
    TEST_ASSERT_EQUAL(4, a->x);
    TEST_ASSERT_NOT_NULL(find_text("ccc ddd"));
    TEST_ASSERT_EQUAL(a->y + WE_LINE_H, find_text("ccc ddd")->y);
    for (i = 0; i < g_doc.nitems; i++) if (g_doc.items[i].kind == OS_FB_WEB_BORDER) { borders++; TEST_ASSERT_EQUAL(2, g_doc.items[i].bold); TEST_ASSERT_EQUAL(88, g_doc.items[i].w); }
    TEST_ASSERT_EQUAL(1U, borders);
}

static void test_percent_width_and_img_alt(void) {
    static const char* p = "<body style='margin:0'><div id=x style='width:50%;background:#123'>t <img alt='logo'> u</div></body>";
    int x;
    TEST_ASSERT_EQUAL(0, we_render(&g_doc, p, (int)strlen(p), 300));
    x = we_find_id(&g_doc, "x");
    TEST_ASSERT_EQUAL(150, g_doc.nodes[x].w);
    TEST_ASSERT_EQUAL(0x112233U, g_doc.nodes[x].bg);
    TEST_ASSERT_NOT_NULL(find_text("[logo]"));
    TEST_ASSERT_NOT_NULL(find_text("t"));
    TEST_ASSERT_NOT_NULL(find_text("u"));
}

static char g_out[4096];
static void cap(void* ctx, const char* s) { (void)ctx; strncat(g_out, s, sizeof(g_out) - strlen(g_out) - 1); }

static void test_inspector_and_diagnostics(void) {
    static const char* p = "<body><div id=a class='k'>x</span></div><script>1</script><style>p>q{x:y} div{zz:1}</style></body>";
    int a;
    TEST_ASSERT_EQUAL(0, we_render(&g_doc, p, (int)strlen(p), 300));
    TEST_ASSERT_TRUE(g_doc.ndiag >= 3);
    TEST_ASSERT_NOT_NULL(strstr(g_doc.diag[0], "unmatched close tag"));
    TEST_ASSERT_NOT_NULL(strstr(g_doc.diag[1], "script ignored"));
    TEST_ASSERT_NOT_NULL(strstr(g_doc.diag[2], "css selector unsupported"));
    TEST_ASSERT_EQUAL(1U, g_doc.perf.unknown_props);
    a = we_find_id(&g_doc, "a");
    g_out[0] = 0;
    we_inspect(&g_doc, a, cap, 0);
    TEST_ASSERT_NOT_NULL(strstr(g_out, "inspect div#a display block box 8 8 284x10"));
    TEST_ASSERT_NOT_NULL(strstr(g_out, "color #000000"));
    g_out[0] = 0;
    we_dump_dom(&g_doc, cap, 0);
    TEST_ASSERT_NOT_NULL(strstr(g_out, "div#a class=k block"));
    TEST_ASSERT_NOT_NULL(strstr(g_out, "\"x\""));
    g_out[0] = 0;
    we_inspect(&g_doc, 999, cap, 0);
    TEST_ASSERT_NOT_NULL(strstr(g_out, "inspect none"));
}

static void test_limits(void) {
    static char big[8192];
    int i, n = 0;
    for (i = 0; i < 300 && n < (int)sizeof(big) - 16; i++) { memcpy(big + n, "<p>w</p>", 8); n += 8; }
    big[n] = 0;
    TEST_ASSERT_TRUE(we_parse(&g_doc, big, n) < 0);   /* node table full, reported */
    TEST_ASSERT_NOT_NULL(strstr(g_doc.diag[g_doc.ndiag - 1], "too many nodes"));
    n = 0;
    for (i = 0; i < 70; i++) { memcpy(big + n, "<b>q</b> ", 9); n += 9; }
    TEST_ASSERT_EQUAL(0, we_render(&g_doc, big, n, 100));
    TEST_ASSERT_TRUE(g_doc.nitems <= OS_FB_WEB_ITEMS);
    TEST_ASSERT_TRUE(g_doc.perf.items_dropped == 0 || g_doc.nitems == OS_FB_WEB_ITEMS);
    TEST_ASSERT_EQUAL(-1, we_layout(&g_doc, 10));
}

/* UX4: href kept by the parser, link hit test on laid-out text */
static void test_link_at(void) {
    static const char html[] = "<p>avant <a href=\"b.html\">le lien</a></p><div style=\"height:40px\">vide</div>";
    char href[32];
    uint32_t i;
    int x = -1, y = -1;
    TEST_ASSERT_EQUAL(0, we_render(&g_doc, html, (int)sizeof(html) - 1, 400));
    for (i = 0; i < g_doc.nitems; i++)
        if (g_doc.items[i].kind == OS_FB_WEB_TEXT && strstr(g_doc.items[i].text, "le lien")) { x = g_doc.items[i].x + 4; y = g_doc.items[i].y + 2; }
    TEST_ASSERT(x >= 0);
    TEST_ASSERT_EQUAL(6, we_link_at(&g_doc, x, y, href, (int)sizeof(href)));
    TEST_ASSERT_EQUAL_STRING("b.html", href);
    TEST_ASSERT_EQUAL(0, we_link_at(&g_doc, 2, y, href, (int)sizeof(href)));     /* "avant" */
    TEST_ASSERT_EQUAL(0, we_link_at(&g_doc, 2, y + 20, href, (int)sizeof(href)));
}

int main(void) {
    unity_init();
    RUN_TEST(test_parse_tree_and_title);
    RUN_TEST(test_cascade);
    RUN_TEST(test_layout_and_display_list);
    RUN_TEST(test_wrapping_and_borders);
    RUN_TEST(test_percent_width_and_img_alt);
    RUN_TEST(test_inspector_and_diagnostics);
    RUN_TEST(test_limits);
    RUN_TEST(test_link_at);
    unity_print_results();
    unity_cleanup();
    return unity_stats.tests_failed == 0 ? 0 : 1;
}
