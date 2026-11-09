/* Lot 85: news and docs pages, and HTML search forms (GET and POST).
 * The pages are fixtures of the shapes real sites use. Nothing here
 * fetches the network. Google and Wikipedia forms are routed to the
 * engines the guest can actually read; a docs site is submitted as-is. */
#include "../../framework/unity.h"
#include "../../../userspace/wf_html.h"
#include "../../../userspace/wf_read.h"
#include "../../../userspace/wf_score.h"
#include "../../../userspace/wf_form.h"
#include "../../../userspace/wf_searchform.h"
#include "../../../userspace/wf_article.h"
#include <stdio.h>
#include <string.h>

static char g_out[16384];
static wf_form_page_t g_forms;

static int reduce_forms(const char *page, const char *base_url) {
    wf_html_stats_t st;
    wf_url_t base;
    memset(&g_forms, 0, sizeof(g_forms));
    if (wf_url_parse(base_url, &base) != 0) return -1;
    return wf_html_reduce_ex(page, (int)strlen(page), &base, g_out, (int)sizeof(g_out), &st, 140, &g_forms);
}

static int load_fix(const char *name, char *buf, int cap) {
    static const char *const pre[] = {
        "fixtures/reader/", "tests/fixtures/reader/", "../fixtures/reader/", "../../fixtures/reader/", 0
    };
    int i;
    for (i = 0; pre[i]; i++) {
        char path[256];
        FILE *f;
        int n;
        snprintf(path, sizeof(path), "%s%s", pre[i], name);
        f = fopen(path, "rb");
        if (!f) continue;
        n = (int)fread(buf, 1, (size_t)(cap - 1), f);
        fclose(f);
        if (n < 0) n = 0;
        buf[n] = 0;
        return n;
    }
    return -1;
}

static void test_content_and_chrome_hints(void) {
    static const char *const content[] = {
        "article-body", "article__body", "story-body", "post-content", "entry-content",
        "markdown-body", "md-content", "rst-content", "devsite-article-body",
        "doc-content", "docs-content", "page-content", "news-body", "mw-parser-output",
        "vector-body", "article-text", "blog-post", "read-content", 0
    };
    static const char *const chrome[] = {
        "site-header", "masthead", "sidebar", "navbar", "breadcrumb", "breadcrumbs",
        "related-posts", "social-share", "newsletter", "pagination", "comments",
        "comment-list", "ad-slot", "advertisement", "cookie-banner", "site-footer",
        "global-nav", "main-nav", "pager", "sponsored", 0
    };
    int i;
    for (i = 0; content[i]; i++) {
        TEST_ASSERT_TRUE(wf_score_hint("", content[i]) >= 60);
        TEST_ASSERT_EQUAL(0, wf_score_is_chrome("", content[i]));
        TEST_ASSERT_EQUAL(0, wf_read_chrome("", content[i]));
    }
    for (i = 0; chrome[i]; i++) {
        TEST_ASSERT_TRUE(wf_score_hint("", chrome[i]) < 0);
        TEST_ASSERT_EQUAL(1, wf_score_is_chrome("", chrome[i]));
        TEST_ASSERT_EQUAL(1, wf_read_chrome("", chrome[i]));
    }
    TEST_ASSERT_EQUAL(0, wf_score_hint("", "reference-implementation"));
    TEST_ASSERT_EQUAL(0, wf_read_chrome("", "reference-implementation"));
    TEST_ASSERT_TRUE(wf_score_hint("", "post") > 0);
    TEST_ASSERT_TRUE(wf_score_hint("", "post") < 60);
}

static void test_news_div_beats_the_sidebar(void) {
    const char *page =
        "<html><body>"
        "<div class=\"site-header\"><a href=\"/home\">Home</a><a href=\"/world\">World</a></div>"
        "<div class=\"sidebar\"><a href=\"/a\">LinkA</a><a href=\"/b\">LinkB</a><a href=\"/c\">LinkC</a></div>"
        "<div class=\"article-body\"><h1>Harbour</h1>"
        "<p>The guest keeps the article and leaves the sidebar out of the node budget.</p>"
        "</div>"
        "<div class=\"comments\"><p>First comment stays out.</p></div>"
        "</body></html>";
    wf_read_info_t info;
    int lo = 0, hi = 0;
    TEST_ASSERT_EQUAL(1, wf_read_root(page, (int)strlen(page), &lo, &hi, &info));
    TEST_ASSERT_EQUAL(5, info.kind);
    TEST_ASSERT_NOT_NULL(strstr(page + lo, "Harbour"));
    TEST_ASSERT_TRUE(strstr(page + lo, "LinkA") == 0 || strstr(page + lo, "LinkA") >= page + hi);
    TEST_ASSERT_EQUAL(1, reduce_forms(page, "https://news.example/story") > 0);
    TEST_ASSERT_NOT_NULL(strstr(g_out, "Harbour"));
    TEST_ASSERT_NOT_NULL(strstr(g_out, "node budget"));
    TEST_ASSERT_NULL(strstr(g_out, "LinkA"));
    TEST_ASSERT_NULL(strstr(g_out, "First comment"));
}

static void test_docs_markdown_and_rst(void) {
    const char *md =
        "<html><body><nav><a href=\"/idx\">Index</a></nav>"
        "<div class=\"markdown-body\"><h1>Install</h1>"
        "<p>Build the guest with make and boot it under QEMU.</p>"
        "<pre><code>make qemu-lot85</code></pre></div></body></html>";
    const char *rst =
        "<html><body><div class=\"rst-content\"><h1>API</h1>"
        "<p>The function returns zero when the block was parsed.</p>"
        "<table><tr><td>status</td><td>meaning</td></tr></table></div>"
        "<div class=\"related\">See also the other page.</div></body></html>";
    wf_read_info_t info;
    int lo, hi;
    TEST_ASSERT_EQUAL(1, wf_read_root(md, (int)strlen(md), &lo, &hi, &info));
    TEST_ASSERT_EQUAL(5, info.kind);
    TEST_ASSERT_NOT_NULL(strstr(md + lo, "Install"));
    TEST_ASSERT_EQUAL(1, reduce_forms(md, "https://docs.example/install") > 0);
    TEST_ASSERT_NOT_NULL(strstr(g_out, "make qemu-lot85"));
    TEST_ASSERT_NULL(strstr(g_out, "Index"));
    TEST_ASSERT_EQUAL(1, wf_read_root(rst, (int)strlen(rst), &lo, &hi, &info));
    TEST_ASSERT_NOT_NULL(strstr(rst + lo, "status"));
    TEST_ASSERT_EQUAL(1, reduce_forms(rst, "https://docs.example/api") > 0);
    TEST_ASSERT_NOT_NULL(strstr(g_out, "meaning"));
    TEST_ASSERT_NULL(strstr(g_out, "See also"));
}

static void test_short_page_is_not_rewritten(void) {
    const char *page = "<html><body><p>Hi.</p></body></html>";
    wf_read_info_t info;
    int lo = 9, hi = 9;
    TEST_ASSERT_EQUAL(0, wf_read_root(page, (int)strlen(page), &lo, &hi, &info));
    TEST_ASSERT_EQUAL(0, lo);
    TEST_ASSERT_EQUAL(0, info.kind);
}

static void test_header_search_form_get(void) {
    const char *page =
        "<html><body>"
        "<header><form action=\"/search\" method=\"get\">"
        "<input type=\"search\" name=\"q\" value=\"\">"
        "<input type=\"hidden\" name=\"src\" value=\"os\">"
        "<button type=\"submit\">Go</button>"
        "</form></header>"
        "<article><h1>Story</h1><p>The body of the story is what the reader keeps.</p></article>"
        "</body></html>";
    wf_form_submit_t sub;
    int text;
    TEST_ASSERT_TRUE(reduce_forms(page, "https://news.example/story") > 0);
    TEST_ASSERT_NOT_NULL(strstr(g_out, "Story"));
    TEST_ASSERT_EQUAL(1, g_forms.nforms);
    TEST_ASSERT_EQUAL(WF_METHOD_GET, g_forms.form[0].method);
    text = wf_form_first_text(&g_forms, 0);
    TEST_ASSERT_TRUE(text >= 0);
    TEST_ASSERT_EQUAL(0, wf_form_set(&g_forms, text, "harbour"));
    TEST_ASSERT_EQUAL(0, wf_form_submit(&g_forms, 0, -1, &sub));
    TEST_ASSERT_EQUAL(WF_METHOD_GET, sub.method);
    TEST_ASSERT_NOT_NULL(strstr(sub.url, "https://news.example/search?"));
    TEST_ASSERT_NOT_NULL(strstr(sub.url, "q=harbour"));
    TEST_ASSERT_NOT_NULL(strstr(sub.url, "src=os"));
    TEST_ASSERT_EQUAL(WF_ROUTE_FETCH, wf_search_route(sub.url, 0, g_out, (int)sizeof(g_out)));
    TEST_ASSERT_EQUAL_STRING("harbour", g_out);
}

static void test_header_search_form_post(void) {
    const char *page =
        "<html><body>"
        "<form action=\"/find\" method=\"POST\">"
        "<input type=\"text\" name=\"query\" value=\"\">"
        "<input type=\"submit\" name=\"go\" value=\"Search\">"
        "</form>"
        "<main><h1>Manual</h1><p>POST sends the body and does not put it in the URL.</p></main>"
        "</body></html>";
    wf_form_submit_t sub;
    int text, submitter, i;
    TEST_ASSERT_TRUE(reduce_forms(page, "https://docs.example/manual") > 0);
    TEST_ASSERT_NOT_NULL(strstr(g_out, "Manual"));
    TEST_ASSERT_EQUAL(WF_METHOD_POST, g_forms.form[0].method);
    text = wf_form_first_text(&g_forms, 0);
    TEST_ASSERT_EQUAL(0, wf_form_set(&g_forms, text, "ring 3"));
    submitter = -1;
    for (i = 0; i < g_forms.nfields; i++)
        if (g_forms.field[i].type == WF_FIELD_SUBMIT) submitter = i;
    TEST_ASSERT_TRUE(submitter >= 0);
    TEST_ASSERT_EQUAL(0, wf_form_submit(&g_forms, 0, submitter, &sub));
    TEST_ASSERT_EQUAL(WF_METHOD_POST, sub.method);
    TEST_ASSERT_EQUAL_STRING("https://docs.example/find", sub.url);
    TEST_ASSERT_NOT_NULL(strstr(sub.body, "query=ring+3"));
    TEST_ASSERT_NOT_NULL(strstr(sub.body, "go=Search"));
    TEST_ASSERT_EQUAL(WF_ROUTE_FETCH, wf_search_route(sub.url, sub.body, g_out, (int)sizeof(g_out)));
    TEST_ASSERT_EQUAL_STRING("ring 3", g_out);
}

static void test_login_form_is_not_pulled_into_the_article(void) {
    const char *page =
        "<html><body>"
        "<form action=\"/login\" method=\"post\">"
        "<input type=\"text\" name=\"user\" value=\"\">"
        "<input type=\"password\" name=\"pw\" value=\"secret\">"
        "</form>"
        "<article><h1>Public</h1><p>The login form is chrome, not the story.</p></article>"
        "</body></html>";
    TEST_ASSERT_TRUE(reduce_forms(page, "https://news.example/") > 0);
    TEST_ASSERT_NOT_NULL(strstr(g_out, "Public"));
    TEST_ASSERT_EQUAL(0, g_forms.nforms);
    TEST_ASSERT_NULL(strstr(g_out, "secret"));
}

static void test_wikipedia_and_google_routes(void) {
    char q[80];
    TEST_ASSERT_EQUAL(WF_ROUTE_WIKI, wf_search_route(
        "https://en.wikipedia.org/w/index.php?title=Special:Search&search=Mohhdy", 0, q, (int)sizeof(q)));
    TEST_ASSERT_EQUAL_STRING("Mohhdy", q);
    TEST_ASSERT_EQUAL(WF_ROUTE_WIKI, wf_search_route(
        "https://fr.wikipedia.org/w/index.php", "search=port+libre&title=Special%3ASearch", q, (int)sizeof(q)));
    TEST_ASSERT_EQUAL_STRING("port libre", q);
    TEST_ASSERT_EQUAL(WF_ROUTE_GOOGLE, wf_search_route(
        "https://www.google.com/search", "q=guest+os&gbv=1", q, (int)sizeof(q)));
    TEST_ASSERT_EQUAL_STRING("guest os", q);
    TEST_ASSERT_EQUAL(WF_ROUTE_GOOGLE, wf_search_route(
        "https://www.google.fr/search?q=bonjour", 0, q, (int)sizeof(q)));
    TEST_ASSERT_EQUAL_STRING("bonjour", q);
    TEST_ASSERT_EQUAL(WF_ROUTE_GOOGLE, wf_search_route(
        "https://google.co.uk/search?q=harbour", 0, q, (int)sizeof(q)));
    /* An article URL is not a search, even on Wikipedia. */
    TEST_ASSERT_EQUAL(WF_ROUTE_FETCH, wf_search_route(
        "https://en.wikipedia.org/wiki/Harbour", 0, q, (int)sizeof(q)));
    TEST_ASSERT_EQUAL(0, q[0]);
    /* q beats a weaker s= on the same body. */
    TEST_ASSERT_EQUAL(WF_ROUTE_FETCH, wf_search_route(
        "https://docs.example/search?s=nope&q=kept", 0, q, (int)sizeof(q)));
    TEST_ASSERT_EQUAL_STRING("kept", q);
    TEST_ASSERT_EQUAL(1, wf_search_field_name("q"));
    TEST_ASSERT_EQUAL(1, wf_search_field_name("search"));
    TEST_ASSERT_EQUAL(0, wf_search_field_name("user"));
    TEST_ASSERT_EQUAL(WF_ROUTE_FETCH, wf_search_route("https://notwikipedia.org/w/index.php?search=x", 0, q, (int)sizeof(q)));
    TEST_ASSERT_EQUAL(WF_ROUTE_FETCH, wf_search_route("https://google.evil.com/search?q=x", 0, q, (int)sizeof(q)));
}

static void test_wiki_form_inside_a_page(void) {
    const char *page =
        "<html><body>"
        "<form action=\"/w/index.php\" method=\"get\">"
        "<input type=\"hidden\" name=\"title\" value=\"Special:Search\">"
        "<input type=\"search\" name=\"search\" value=\"\">"
        "</form>"
        "<div id=\"mw-content-text\"><h1>Harbour</h1><p>A harbour is a sheltered body of water.</p></div>"
        "</body></html>";
    wf_form_submit_t sub;
    int text;
    TEST_ASSERT_TRUE(reduce_forms(page, "https://en.wikipedia.org/wiki/Harbour") > 0);
    TEST_ASSERT_NOT_NULL(strstr(g_out, "sheltered"));
    text = wf_form_first_text(&g_forms, 0);
    TEST_ASSERT_TRUE(text >= 0);
    TEST_ASSERT_EQUAL(0, wf_form_set(&g_forms, text, "quay"));
    TEST_ASSERT_EQUAL(0, wf_form_submit(&g_forms, 0, -1, &sub));
    TEST_ASSERT_EQUAL(WF_METHOD_GET, sub.method);
    TEST_ASSERT_EQUAL(WF_ROUTE_WIKI, wf_search_route(sub.url, 0, g_out, (int)sizeof(g_out)));
    TEST_ASSERT_EQUAL_STRING("quay", g_out);
    TEST_ASSERT_NOT_NULL(strstr(sub.url, "search=quay"));
}

static void test_percent_in_the_query(void) {
    char q[80];
    TEST_ASSERT_EQUAL(WF_ROUTE_WIKI, wf_search_route(
        "https://en.wikipedia.org/w/index.php?search=caf%C3%A9", 0, q, (int)sizeof(q)));
    TEST_ASSERT_EQUAL_STRING("caf\xC3\xA9", q);
}

static void test_fixture_pages(void) {
    static const char *const files[] = {
        "news_site.html", "docs_site.html", "blog_post.html", "readthedocs.html", 0
    };
    static const char *const keep[] = { "Harbour lights", "make qemu-lot85", "Ring 3", "sphinx" };
    static const char *const drop[] = { "COOKIEBANNER", "SIDEBARLINK", "RELATEDPOST", "PAGERNEXT" };
    char page[24000];
    int i;
    for (i = 0; files[i]; i++) {
        wf_read_info_t info;
        int n, lo = 0, hi = 0;
        n = load_fix(files[i], page, (int)sizeof(page));
        TEST_ASSERT_TRUE(n > 200);
        TEST_ASSERT_EQUAL(1, wf_read_root(page, n, &lo, &hi, &info));
        TEST_ASSERT_TRUE(info.kind == 1 || info.kind == 2 || info.kind == 5);
        TEST_ASSERT_NOT_NULL(strstr(page + lo, keep[i]));
        TEST_ASSERT_TRUE(reduce_forms(page, "https://example.test/page") > 0);
        TEST_ASSERT_NOT_NULL(strstr(g_out, keep[i]));
        TEST_ASSERT_NULL(strstr(g_out, drop[i]));
        {
            char plain[4096];
            TEST_ASSERT_TRUE(wf_article_text(g_out, (int)strlen(g_out), plain, (int)sizeof(plain)) > 0);
            TEST_ASSERT_NOT_NULL(strstr(plain, keep[i]));
            TEST_ASSERT_NULL(strstr(plain, drop[i]));
            TEST_ASSERT_NULL(strstr(plain, "<"));
        }
    }
}

int main(void) {
    unity_init();
    RUN_TEST(test_content_and_chrome_hints);
    RUN_TEST(test_news_div_beats_the_sidebar);
    RUN_TEST(test_docs_markdown_and_rst);
    RUN_TEST(test_short_page_is_not_rewritten);
    RUN_TEST(test_header_search_form_get);
    RUN_TEST(test_header_search_form_post);
    RUN_TEST(test_login_form_is_not_pulled_into_the_article);
    RUN_TEST(test_wikipedia_and_google_routes);
    RUN_TEST(test_wiki_form_inside_a_page);
    RUN_TEST(test_percent_in_the_query);
    RUN_TEST(test_fixture_pages);
    unity_print_results();
    unity_cleanup();
    return unity_stats.tests_failed == 0 ? 0 : 1;
}
