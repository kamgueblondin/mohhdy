/* Lot 86: site CSS cascade, reader display:none, GET and POST preview,
 * autofill offered back into the form. */
#include "../../framework/unity.h"
#include "../../../userspace/wf_sitecss.h"
#include "../../../userspace/wf_formfill.h"
#include "../../../userspace/wf_html.h"
#include "../../../userspace/wf_http.h"
#include "../../../userspace/webeng.h"
#include "../../../userspace/wf_qstring.h"
#include <string.h>
#include <stdio.h>

static we_doc_t g_doc;

static const char *PAGE =
    "<!DOCTYPE html><html><head><title>T</title><style>"
    "p.ads { display: none }"
    "article p { font-weight: bold; color: #336699 }"
    "#note { text-align: center }"
    "div > span { color: red }"
    "p + p { text-align: right }"
    ".hide { display: none !important }"
    "#keep.hide { display: block }"
    "li:first-child { font-weight: bold }"
    "li:nth-child(2n) { text-align: center }"
    "a:link { color: blue }"
    "article { padding: 10px }"
    "@media (max-width: 10px) { p { display: none } }"
    "@media (min-width: 200px) { #wide { color: green } }"
    "@media (max-width: 100px) { #wide { display: none } }"
    "</style></head><body>"
    "<p id=wide>W</p>"
    "<p class=ads>banner</p>"
    "<article><p id=note>Keep</p><p>Second</p></article>"
    "<div><span>in</span></div>"
    "<p id=keep class=hide>nope</p>"
    "<ul><li>one</li><li>two</li></ul>"
    "<a href=\"https://example.com/a\">link</a>"
    "<form action=\"https://example.com/search\" method=\"get\">"
    "<input type=\"search\" name=\"q\" value=\"\">"
    "<input type=\"submit\" value=\"Go\">"
    "</form>"
    "<form action=\"https://example.com/login\" method=\"post\">"
    "<input type=\"text\" name=\"user\" value=\"\">"
    "<input type=\"password\" name=\"pw\" value=\"secret\">"
    "<input type=\"hidden\" name=\"tok\" value=\"abc\">"
    "<input type=\"submit\" value=\"Send\">"
    "</form>"
    "</body></html>";

static const wf_site_node_t *find_id(const wf_site_t *d, const char *id) {
    int i;
    for (i = 0; i < d->nnodes; i++) if (strcmp(d->node[i].id, id) == 0) return &d->node[i];
    return 0;
}

static const wf_site_node_t *find_tag_class(const wf_site_t *d, const char *tag, const char *cls) {
    int i, c;
    for (i = 0; i < d->nnodes; i++) {
        if (strcmp(d->node[i].tag, tag) != 0) continue;
        for (c = 0; c < d->node[i].ncls; c++) if (strcmp(d->node[i].cls[c], cls) == 0) return &d->node[i];
    }
    return 0;
}

static void test_cascade(void) {
    static wf_site_t doc;
    const wf_site_node_t *n;
    char rep[2048];
    TEST_ASSERT_EQUAL(0, wf_site_prepare(&doc, PAGE, (int)strlen(PAGE)));
    TEST_ASSERT(doc.nnodes > 4);
    TEST_ASSERT(doc.nrules > 4);
    n = find_tag_class(&doc, "p", "ads");
    TEST_ASSERT(n != 0);
    TEST_ASSERT_EQUAL(WF_SITE_DISP_NONE, n->disp);
    TEST_ASSERT_EQUAL(1, wf_site_hidden_at(&doc, n->start));
    n = find_id(&doc, "note");
    TEST_ASSERT(n != 0);
    TEST_ASSERT_EQUAL(2, n->weight);
    TEST_ASSERT_EQUAL(2, n->align);
    TEST_ASSERT_EQUAL(1, n->color_set);
    TEST_ASSERT_EQUAL(0x336699u, n->color);
    n = find_id(&doc, "keep");
    TEST_ASSERT(n != 0);
    TEST_ASSERT_EQUAL(WF_SITE_DISP_NONE, n->disp);   /* !important beats the id */
    n = find_tag_class(&doc, "span", "");
    /* span has no class; find by tag under div */
    {
        int i, saw = 0;
        for (i = 0; i < doc.nnodes; i++) if (strcmp(doc.node[i].tag, "span") == 0) {
            TEST_ASSERT_EQUAL(1, doc.node[i].color_set);
            TEST_ASSERT_EQUAL(0xff0000u, doc.node[i].color);
            saw = 1;
        }
        TEST_ASSERT_EQUAL(1, saw);
    }
    {
        int i, nli = 0;
        for (i = 0; i < doc.nnodes; i++) if (strcmp(doc.node[i].tag, "li") == 0) {
            if (doc.node[i].child_i == 1) TEST_ASSERT_EQUAL(2, doc.node[i].weight);
            if (doc.node[i].child_i == 2) TEST_ASSERT_EQUAL(2, doc.node[i].align);
            nli++;
        }
        TEST_ASSERT_EQUAL(2, nli);
    }
    {
        int i, saw = 0;
        for (i = 0; i < doc.nnodes; i++) if (strcmp(doc.node[i].tag, "a") == 0) {
            TEST_ASSERT_EQUAL(0x0000ffu, doc.node[i].color);
            saw = 1;
        }
        TEST_ASSERT_EQUAL(1, saw);
    }
    /* the @media rule must not hide every paragraph */
    n = find_id(&doc, "note");
    TEST_ASSERT(n->disp != WF_SITE_DISP_NONE);
    TEST_ASSERT(wf_site_report(&doc, rep, (int)sizeof(rep)) > 0);
    TEST_ASSERT(strstr(rep, "display none") != 0);
    TEST_ASSERT(strstr(rep, "#note") != 0);
    {
        int i, saw = 0;
        for (i = 0; i < doc.nnodes; i++) if (strcmp(doc.node[i].tag, "article") == 0) {
            TEST_ASSERT_EQUAL(10, doc.node[i].pad[0]);
            TEST_ASSERT_EQUAL(10, doc.node[i].pad[3]);
            saw = 1;
        }
        TEST_ASSERT_EQUAL(1, saw);
    }
    n = find_id(&doc, "wide");
    TEST_ASSERT(n != 0);
    TEST_ASSERT_EQUAL(0x008000u, n->color);
    TEST_ASSERT(n->disp != WF_SITE_DISP_NONE);
    TEST_ASSERT_EQUAL(0, wf_site_prepare_vw(&doc, PAGE, (int)strlen(PAGE), 50));
    n = find_id(&doc, "wide");
    TEST_ASSERT(n != 0);
    TEST_ASSERT_EQUAL(WF_SITE_DISP_NONE, n->disp);
    TEST_ASSERT_EQUAL(0, n->color_set);
}

static int field_named(const wf_form_page_t *p, const char *name) {
    int i;
    for (i = 0; i < p->nfields; i++) if (strcmp(p->field[i].name, name) == 0) return i;
    return -1;
}

static void test_reader_and_forms(void) {
    static char out[8192];
    static char prev[1024];
    static wf_form_page_t forms;
    static wf_autofill_t fill;
    wf_html_stats_t st;
    wf_url_t base;
    wf_fill_offer_t offer[4];
    int n, q, user, got;
    const char *styled;

    memset(&st, 0, sizeof(st));
    wf_html_set_sitecss(0);
    n = wf_html_reduce(PAGE, (int)strlen(PAGE), 0, out, (int)sizeof(out), &st);
    TEST_ASSERT(n > 0);
    TEST_ASSERT(strstr(out, "banner") != 0);

    wf_html_set_sitecss(1);
    TEST_ASSERT_EQUAL(0, wf_url_parse("https://example.com/search", &base));
    n = wf_html_reduce_ex(PAGE, (int)strlen(PAGE), &base, out, (int)sizeof(out), &st, 80, &forms);
    wf_html_set_sitecss(0);
    TEST_ASSERT(n > 0);
    TEST_ASSERT(strstr(out, "banner") == 0);
    TEST_ASSERT(strstr(out, "Keep") != 0);
    TEST_ASSERT(strstr(out, "font-weight:bold") != 0);
    TEST_ASSERT(strstr(out, "text-align:center") != 0);
    TEST_ASSERT(strstr(out, "#336699") != 0);
    TEST_ASSERT(strstr(out, "padding:10px 10px 10px 10px") != 0);

    memset(&g_doc, 0, sizeof(g_doc));
    styled = strstr(out, "font-weight:bold");
    TEST_ASSERT(styled != 0);
    TEST_ASSERT_EQUAL(0, we_parse(&g_doc, out, n));
    TEST_ASSERT_EQUAL(0, we_style(&g_doc));
    {
        int i, bold = 0;
        for (i = 0; i < (int)g_doc.nnodes; i++) if (g_doc.nodes[i].bold) bold++;
        TEST_ASSERT(bold > 0);
    }
    TEST_ASSERT_EQUAL(0, we_layout(&g_doc, 640));
    {
        int i, saw = 0;
        for (i = 0; i < (int)g_doc.nnodes; i++) if (g_doc.nodes[i].padding[0] == 10) saw++;
        TEST_ASSERT(saw > 0);
    }

    q = field_named(&forms, "q");
    user = field_named(&forms, "user");
    TEST_ASSERT(q >= 0);
    TEST_ASSERT(user >= 0);
    TEST_ASSERT_EQUAL(0, wf_form_set(&forms, q, "mohhdy"));
    TEST_ASSERT(wf_formfill_preview(&forms, q, prev, (int)sizeof(prev)) > 0);
    TEST_ASSERT(strstr(prev, "GET ") == prev);
    TEST_ASSERT(strstr(prev, "https://example.com/search?q=mohhdy") != 0);

    TEST_ASSERT_EQUAL(0, wf_form_set(&forms, user, "ada"));
    TEST_ASSERT(wf_formfill_preview(&forms, user, prev, (int)sizeof(prev)) > 0);
    TEST_ASSERT(strstr(prev, "POST ") == prev);
    TEST_ASSERT(strstr(prev, "https://example.com/login") != 0);
    TEST_ASSERT(strstr(prev, "user=ada") != 0);
    TEST_ASSERT(strstr(prev, "pw=secret") != 0);
    TEST_ASSERT(strstr(prev, "tok=abc") != 0);
    {
        wf_qs_t qs;
        const char *body = strchr(prev, '\n');
        TEST_ASSERT(body != 0);
        body++;
        TEST_ASSERT_EQUAL(0, wf_qs_parse(body, (int)strlen(body) - 1, &qs));
        {
            char v[64];
            TEST_ASSERT_EQUAL(0, wf_qs_get(&qs, "user", v, (int)sizeof(v)));
            TEST_ASSERT_EQUAL_STRING("ada", v);
            TEST_ASSERT_EQUAL(0, wf_qs_get(&qs, "tok", v, (int)sizeof(v)));
            TEST_ASSERT_EQUAL_STRING("abc", v);
            TEST_ASSERT(wf_qs_same("user=ada&pw=secret&tok=abc", 26, "tok=abc&user=ada&pw=secret", 26));
            TEST_ASSERT_EQUAL(0, wf_qs_parse("q=a%2Bb+c", 9, &qs));
            TEST_ASSERT_EQUAL(0, wf_qs_get(&qs, "q", v, (int)sizeof(v)));
            TEST_ASSERT_EQUAL_STRING("a+b c", v);
            TEST_ASSERT(wf_qs_parse("q=%ZZ", 6, &qs) != 0);
        }
    }

    wf_autofill_init(&fill);
    got = wf_formfill_remember(&fill, &forms, -1, "https://example.com/search");
    TEST_ASSERT(got >= 2);
    {
        char v[64];
        TEST_ASSERT_EQUAL(0, wf_autofill_get(&fill, "example.com", "q", v, (int)sizeof(v)));
        TEST_ASSERT_EQUAL_STRING("mohhdy", v);
        TEST_ASSERT_EQUAL(0, wf_autofill_get(&fill, "example.com", "user", v, (int)sizeof(v)));
        TEST_ASSERT_EQUAL_STRING("ada", v);
        TEST_ASSERT(wf_autofill_get(&fill, "example.com", "pw", v, (int)sizeof(v)) != 0);
        TEST_ASSERT(wf_autofill_get(&fill, "example.com", "tok", v, (int)sizeof(v)) != 0);
    }
    TEST_ASSERT_EQUAL(0, wf_form_set(&forms, q, ""));
    TEST_ASSERT_EQUAL(1, wf_formfill_offers(&fill, &forms, q, "https://example.com/search", offer, 4));
    TEST_ASSERT_EQUAL_STRING("mohhdy", offer[0].value);
    TEST_ASSERT_EQUAL(0, wf_formfill_apply(&forms, q, &offer[0]));
    TEST_ASSERT_EQUAL_STRING("mohhdy", forms.field[q].value);
    TEST_ASSERT(wf_formfill_preview(&forms, q, prev, (int)sizeof(prev)) > 0);
    TEST_ASSERT(strstr(prev, "q=mohhdy") != 0);
    /* Lot 86: the editor key (Down) fills an empty focused text field */
    TEST_ASSERT_EQUAL(0, wf_form_set(&forms, q, ""));
    forms.focus = -1;
    TEST_ASSERT_EQUAL(0, wf_formfill_key(&fill, &forms, q, "https://example.com/other"));
    TEST_ASSERT_EQUAL_STRING("mohhdy", forms.field[q].value);
    TEST_ASSERT_EQUAL(q, forms.focus);
    TEST_ASSERT_EQUAL(0, wf_form_insert(&forms, '!'));   /* cursor at the end */
    TEST_ASSERT_EQUAL_STRING("mohhdy!", forms.field[q].value);
    /* another host, a password or hidden field, a bad index: nothing */
    TEST_ASSERT_EQUAL(0, wf_form_set(&forms, q, ""));
    TEST_ASSERT_EQUAL(-1, wf_formfill_key(&fill, &forms, q, "https://other.example/"));
    TEST_ASSERT_EQUAL_STRING("", forms.field[q].value);
    TEST_ASSERT_EQUAL(-1, wf_formfill_key(&fill, &forms, field_named(&forms, "pw"), "https://example.com/"));
    TEST_ASSERT_EQUAL(-1, wf_formfill_key(&fill, &forms, field_named(&forms, "tok"), "https://example.com/"));
    TEST_ASSERT_EQUAL(-1, wf_formfill_key(&fill, &forms, forms.nfields, "https://example.com/"));
    TEST_ASSERT_EQUAL(-1, wf_formfill_key(0, &forms, q, "https://example.com/"));
    TEST_ASSERT_EQUAL(0, wf_html_sitecss_on());
}

int main(void) {
    unity_init();
    RUN_TEST(test_cascade);
    RUN_TEST(test_reader_and_forms);
    unity_print_results();
    unity_cleanup();
    return unity_stats.tests_failed == 0 ? 0 : 1;
}
