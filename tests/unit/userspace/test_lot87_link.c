/* Lot 87: external stylesheets - scan, resolve, media filter, fetch
 * through a callback, one level of @import, joined text that the reader
 * stylesheet applies before the page <style>. */
#include "../../framework/unity.h"
#include "../../../userspace/wf_csslink.h"
#include "../../../userspace/wf_sitecss.h"
#include "../../../userspace/wf_http.h"
#include <string.h>
#include <stdio.h>

typedef struct { const char *url; const char *body; } fake_t;
static const fake_t g_site[] = {
    { "https://ex.test/css/main.css", "@charset \"utf-8\";\n@import url(\"base.css\");\n.ad { display: none }\n" },
    { "https://ex.test/css/base.css", "p { font-weight: bold }\n" },
    { "https://ex.test/blog/local.css", "h1 { text-align: center }\n" },
    { "https://cdn.test/wide.css", ".side { display: none }\n" },
    { "https://ex.test/print.css", ".nav { display: none }\n" },
    { 0, 0 }
};
static int g_gets;
static char g_last[256];

static int fake_get(void *ctx, const char *url, char *buf, int cap) {
    int i;
    (void)ctx;
    g_gets++;
    snprintf(g_last, sizeof(g_last), "%s", url);
    for (i = 0; g_site[i].url; i++) {
        if (strcmp(g_site[i].url, url) == 0) {
            int n = (int)strlen(g_site[i].body);
            if (n > cap) n = cap;
            memcpy(buf, g_site[i].body, (size_t)n);
            return n;
        }
    }
    return -1;
}

static const char *PAGE =
    "<html><head>"
    "<link rel=\"stylesheet\" href=\"/css/main.css\">"
    "<link rel=stylesheet href=\"local.css\" media=\"screen\">"
    "<link rel=\"stylesheet\" href=\"https://cdn.test/wide.css\" media=\"(max-width: 700px)\">"
    "<link rel=\"stylesheet\" href=\"/print.css\" media=\"print\">"
    "<link rel=\"alternate stylesheet\" href=\"/alt.css\">"
    "<link rel=\"stylesheet\" href=\"data:text/css,p{}\">"
    "<link rel=\"stylesheet\" href=\"/missing.css\">"
    "<!-- <link rel=stylesheet href=/commented.css> -->"
    "<script>var s='<link rel=stylesheet href=/in-script.css>';</script>"
    "<style>@import \"/print.css\" print; p { color: red }</style>"
    "</head><body><p class=ad>x</p><h1>t</h1><div class=side>s</div><div class=nav>n</div></body></html>";

static void test_scan(void) {
    wf_cssl_ref_t refs[WF_CSSL_FOUND];
    wf_cssl_stats_t st;
    int n;
    memset(&st, 0, sizeof(st));
    n = wf_csslink_scan(PAGE, (int)strlen(PAGE), "https://ex.test/blog/post.html", refs, WF_CSSL_FOUND, &st);
    TEST_ASSERT_EQUAL(5, n);
    TEST_ASSERT_EQUAL_STRING("https://ex.test/css/main.css", refs[0].url);
    TEST_ASSERT_EQUAL_STRING("https://ex.test/blog/local.css", refs[1].url);
    TEST_ASSERT_EQUAL_STRING("screen", refs[1].media);
    TEST_ASSERT_EQUAL_STRING("https://cdn.test/wide.css", refs[2].url);
    TEST_ASSERT_EQUAL_STRING("https://ex.test/print.css", refs[3].url);
    TEST_ASSERT_EQUAL_STRING("https://ex.test/missing.css", refs[4].url);
    TEST_ASSERT_EQUAL(1, st.refused);              /* data: */
    /* the @import of print.css is the same sheet: listed once */
}

static void test_imports(void) {
    wf_cssl_ref_t refs[4];
    const char *css = "/* c */ @import url(a.css) screen; @import 'b.css';\n@import url( \"../c.css\" );"
                      "p{} @import url(late.css);";
    int n = wf_csslink_imports(css, (int)strlen(css), "http://h.test/x/y/s.css", refs, 4);
    TEST_ASSERT_EQUAL(3, n);
    TEST_ASSERT_EQUAL_STRING("http://h.test/x/y/a.css", refs[0].url);
    TEST_ASSERT_EQUAL_STRING("screen", refs[0].media);
    TEST_ASSERT_EQUAL_STRING("http://h.test/x/y/b.css", refs[1].url);
    TEST_ASSERT_EQUAL_STRING("http://h.test/x/c.css", refs[2].url);
    TEST_ASSERT_EQUAL(1, refs[2].from_import);
}

static void test_collect_and_apply(void) {
    static char out[8192];
    static wf_site_t d;
    wf_cssl_stats_t st;
    char line[160];
    int n, i, ad = -1, h1 = -1, side = -1, p = -1, nav = -1;
    g_gets = 0;
    n = wf_csslink_collect(PAGE, (int)strlen(PAGE), "https://ex.test/blog/post.html", 1024, 768,
                           fake_get, 0, out, (int)sizeof(out), &st);
    TEST_ASSERT(n > 0);
    TEST_ASSERT_EQUAL(n, st.bytes);
    /* main.css + its base.css import + local.css; wide.css (max-width 700
     * at 1024) and print.css are not fetched; missing.css is the fourth
     * attempt and fails */
    TEST_ASSERT_EQUAL(3, st.fetched);
    TEST_ASSERT_EQUAL(4, g_gets);
    TEST_ASSERT_EQUAL(2, st.skipped_media);
    TEST_ASSERT_EQUAL(1, st.failed);
    TEST_ASSERT_EQUAL(0, st.dropped);
    TEST_ASSERT_NULL(strstr(out, "@import"));
    TEST_ASSERT_NULL(strstr(out, "@charset"));
    TEST_ASSERT(strstr(out, "font-weight") < strstr(out, ".ad"));   /* the import first */
    TEST_ASSERT_NOT_NULL(strstr(out, "@media screen {"));
    TEST_ASSERT(wf_csslink_stats_text(&st, line, (int)sizeof(line)) > 0);
    TEST_ASSERT_NOT_NULL(strstr(line, "css-links found"));
    TEST_ASSERT_NOT_NULL(strstr(line, " fetched 3"));

    TEST_ASSERT_EQUAL(0, wf_site_prepare_ext(&d, PAGE, (int)strlen(PAGE), 1024, 768, out, n));
    for (i = 0; i < d.nnodes; i++) {
        if (strcmp(d.node[i].tag, "p") == 0) { p = i; if (d.node[i].ncls) ad = i; }
        if (strcmp(d.node[i].tag, "h1") == 0) h1 = i;
        if (d.node[i].ncls && strcmp(d.node[i].cls[0], "side") == 0) side = i;
        if (d.node[i].ncls && strcmp(d.node[i].cls[0], "nav") == 0) nav = i;
    }
    TEST_ASSERT(ad >= 0 && h1 >= 0 && side >= 0 && p >= 0 && nav >= 0);
    TEST_ASSERT_EQUAL(WF_SITE_DISP_NONE, d.node[ad].disp);
    TEST_ASSERT_EQUAL(2, d.node[p].weight);
    TEST_ASSERT_EQUAL(2, d.node[h1].align);
    TEST_ASSERT(d.node[side].disp != WF_SITE_DISP_NONE);
    TEST_ASSERT(d.node[nav].disp != WF_SITE_DISP_NONE);

    /* a narrow viewport fetches wide.css too; the four-attempt budget
     * then leaves missing.css out */
    g_gets = 0;
    n = wf_csslink_collect(PAGE, (int)strlen(PAGE), "https://ex.test/blog/post.html", 600, 800,
                           fake_get, 0, out, (int)sizeof(out), &st);
    TEST_ASSERT(n > 0);
    TEST_ASSERT_NOT_NULL(strstr(out, ".side"));
    TEST_ASSERT_NOT_NULL(strstr(out, "@media (max-width: 700px) {"));
    TEST_ASSERT_EQUAL(4, st.fetched);
    TEST_ASSERT_EQUAL(1, st.dropped);
    TEST_ASSERT_EQUAL(0, st.failed);
}

static void test_failures_and_bounds(void) {
    static char out[32];
    wf_cssl_stats_t st;
    const char *one = "<link rel=stylesheet href=/nope.css><link rel=stylesheet href=/css/main.css>";
    int n;
    n = wf_csslink_collect(one, (int)strlen(one), "https://ex.test/", 800, 600, fake_get, 0, out, (int)sizeof(out), &st);
    TEST_ASSERT_EQUAL(1, st.failed);
    /* 32 bytes cannot hold main.css: dropped, the output stays a string */
    TEST_ASSERT(st.dropped >= 1);
    TEST_ASSERT_EQUAL((int)strlen(out), n);
    TEST_ASSERT_EQUAL(-1, wf_csslink_collect(one, (int)strlen(one), "https://ex.test/", 800, 600, 0, 0, out, 64, &st));
    /* no base URL: nothing is resolved, nothing is fetched */
    g_gets = 0;
    n = wf_csslink_collect(one, (int)strlen(one), 0, 800, 600, fake_get, 0, out, (int)sizeof(out), &st);
    TEST_ASSERT_EQUAL(0, n);
    TEST_ASSERT_EQUAL(0, g_gets);
    TEST_ASSERT_EQUAL(2, st.refused);
}

/* wf_url_resolve removes dot segments (RFC 3986 5.2.4); Lot 86 kept
 * "/x/y/../c.css" as is and the server saw the dots */
static void test_resolve_dots(void) {
    static const char *const cases[][3] = {
        { "http://h/x/y/s.css", "../c.css", "http://h/x/c.css" },
        { "http://h/x/y/s.css", "./d", "http://h/x/y/d" },
        { "http://h/x/y/s.css", "../../../e", "http://h/e" },
        { "http://h/x/y/s.css", "..", "http://h/x/" },
        { "http://h/x/y/s.css", "g/./h/../i", "http://h/x/y/g/i" },
        { "http://h/x/y/s.css", "/p/../q?x=../y", "http://h/q?x=../y" },
        { "http://h/x/y/s.css", ".", "http://h/x/y/" },
        { "https://h/a/", "b/c/", "https://h/a/b/c/" },
        { "https://h/a/b", "?q=1", "https://h/a/b?q=1" },
        { 0, 0, 0 }
    };
    int i;
    for (i = 0; cases[i][0]; i++) {
        wf_url_t b, o;
        char t[300];
        TEST_ASSERT_EQUAL(0, wf_url_parse(cases[i][0], &b));
        TEST_ASSERT_EQUAL(0, wf_url_resolve(&b, cases[i][1], &o));
        TEST_ASSERT(wf_url_format(&o, t, (int)sizeof(t)) > 0);
        TEST_ASSERT_EQUAL_STRING(cases[i][2], t);
    }
}

int main(void) {
    unity_init();
    RUN_TEST(test_resolve_dots);
    RUN_TEST(test_scan);
    RUN_TEST(test_imports);
    RUN_TEST(test_collect_and_apply);
    RUN_TEST(test_failures_and_bounds);
    unity_print_results();
    unity_cleanup();
    return unity_stats.tests_failed == 0 ? 0 : 1;
}
