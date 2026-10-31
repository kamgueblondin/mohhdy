/* Lot 71: forms of the guest browser (userspace/wf_form.c): editing,
 * focus, display, GET / POST submission and its encoding, bounds; the
 * request head of a POST (wf_http_build_request) and a POST through
 * wf_fetch_ex on an in-memory transport (303 then GET). */
#include "../../framework/unity.h"
#include "../../../userspace/wf_form.h"
#include "../../../userspace/wf_fetch.h"
#include <string.h>
#include <stdio.h>

static wf_form_page_t g_fp;
static wf_form_submit_t g_sub;

static void setup_google(void) {
    wf_url_t base;
    wf_form_reset(&g_fp);
    TEST_ASSERT_EQUAL(0, wf_url_parse("https://www.google.com/webhp?hl=fr", &base));
    TEST_ASSERT_EQUAL(0, wf_form_begin(&g_fp, &base, "/search", "get", 0));
    TEST_ASSERT_EQUAL(0, wf_form_add(&g_fp, &base, "hidden", "hl", "fr", 0, 0, 0));
    TEST_ASSERT_EQUAL(1, wf_form_add(&g_fp, &base, "text", "q", "", "Rechercher", 0, 0));
    TEST_ASSERT_EQUAL(2, wf_form_add(&g_fp, &base, "submit", "btnG", "Recherche Google", 0, 0, 0));
    TEST_ASSERT_EQUAL(3, wf_form_add(&g_fp, &base, "submit", "btnI", "J'ai de la chance", 0, 0, 0));
    TEST_ASSERT_EQUAL(-1, wf_form_add(&g_fp, &base, "file", "f", "", 0, 0, 0));
    TEST_ASSERT_EQUAL(4, wf_form_add(&g_fp, &base, "checkbox", "c", "", 0, 0, 0));   /* unchecked: kept, not sent */
    wf_form_end(&g_fp);
}

static void test_edit_and_display(void) {
    char d[80];
    setup_google();
    TEST_ASSERT_EQUAL(-1, wf_form_insert(&g_fp, 'x'));      /* nothing focused */
    TEST_ASSERT_EQUAL(-1, wf_form_focus(&g_fp, 0));         /* hidden is not editable */
    TEST_ASSERT_EQUAL(-1, wf_form_focus(&g_fp, 2));
    wf_form_display(&g_fp, 1, d, (int)sizeof(d));
    TEST_ASSERT_NOT_NULL(strstr(d, "Rechercher"));          /* placeholder when empty */
    TEST_ASSERT_EQUAL(0, wf_form_focus(&g_fp, 1));
    TEST_ASSERT_EQUAL(0, wf_form_insert(&g_fp, 'c'));
    TEST_ASSERT_EQUAL(0, wf_form_insert(&g_fp, 'a'));
    TEST_ASSERT_EQUAL(0, wf_form_insert(&g_fp, 'f'));
    TEST_ASSERT_EQUAL(0, wf_form_insert(&g_fp, (char)0xE9));
    TEST_ASSERT_EQUAL_STRING("caf\xe9", g_fp.field[1].value);
    TEST_ASSERT_EQUAL(0, wf_form_move(&g_fp, -2));
    TEST_ASSERT_EQUAL(0, wf_form_insert(&g_fp, 'X'));
    TEST_ASSERT_EQUAL_STRING("caXf\xe9", g_fp.field[1].value);
    TEST_ASSERT_EQUAL(0, wf_form_backspace(&g_fp));
    TEST_ASSERT_EQUAL(0, wf_form_delete(&g_fp));
    TEST_ASSERT_EQUAL_STRING("ca\xe9", g_fp.field[1].value);
    TEST_ASSERT_EQUAL(0, wf_form_move(&g_fp, 1000));
    TEST_ASSERT_EQUAL(-1, wf_form_delete(&g_fp));
    TEST_ASSERT_EQUAL(-1, wf_form_insert(&g_fp, '\n'));     /* not a textarea */
    TEST_ASSERT_EQUAL(-1, wf_form_insert(&g_fp, 0x07));
    wf_form_display(&g_fp, 1, d, (int)sizeof(d));
    TEST_ASSERT_EQUAL('[', d[0]);
    TEST_ASSERT_NOT_NULL(strstr(d, "ca\xe9|"));             /* cursor at the end */
    wf_form_display(&g_fp, 2, d, (int)sizeof(d));
    TEST_ASSERT_EQUAL_STRING("[ Recherche Google ]", d);
    TEST_ASSERT_EQUAL(0, wf_form_move(&g_fp, -1000));
    TEST_ASSERT_EQUAL(-1, wf_form_backspace(&g_fp));
}

static void test_submit_get(void) {
    int k;
    setup_google();
    wf_form_focus(&g_fp, 1);
    for (k = 0; "mohhdy os & caf"[k]; k++) wf_form_insert(&g_fp, "mohhdy os & caf"[k]);
    wf_form_insert(&g_fp, (char)0xE9);
    TEST_ASSERT_EQUAL(0, wf_form_submit(&g_fp, 0, 2, &g_sub));
    TEST_ASSERT_EQUAL(WF_METHOD_GET, g_sub.method);
    TEST_ASSERT_EQUAL_STRING("https://www.google.com/search?hl=fr&q=mohhdy+os+%26+caf%C3%A9&btnG=Recherche+Google", g_sub.url);
    /* Enter in the field: no submitter, no button name sent */
    TEST_ASSERT_EQUAL(0, wf_form_submit(&g_fp, 0, -1, &g_sub));
    TEST_ASSERT_EQUAL_STRING("https://www.google.com/search?hl=fr&q=mohhdy+os+%26+caf%C3%A9", g_sub.url);
    TEST_ASSERT_EQUAL(-1, wf_form_submit(&g_fp, 3, -1, &g_sub));
    /* a Latin-1 page (www.google.com sends ISO-8859-1): the form is sent
     * in the page charset, as browsers do; CP1252 0x80 stays one byte */
    g_fp.latin1 = 1;
    wf_form_insert(&g_fp, (char)0x80);
    TEST_ASSERT_EQUAL(0, wf_form_submit(&g_fp, 0, -1, &g_sub));
    TEST_ASSERT_EQUAL_STRING("https://www.google.com/search?hl=fr&q=mohhdy+os+%26+caf%E9%80", g_sub.url);
}

static void test_submit_post_and_query_replaced(void) {
    wf_url_t base;
    wf_form_reset(&g_fp);
    TEST_ASSERT_EQUAL(0, wf_url_parse("http://10.0.2.2:8080/form/page?old=1#frag", &base));
    TEST_ASSERT_EQUAL(0, wf_form_begin(&g_fp, &base, "?x=old#y", "GET", 0));
    wf_form_add(&g_fp, &base, "search", "q", "a b", 0, 0, 0);
    wf_form_end(&g_fp);
    TEST_ASSERT_EQUAL(0, wf_form_submit(&g_fp, 0, -1, &g_sub));
    TEST_ASSERT_EQUAL_STRING("http://10.0.2.2:8080/form/page?q=a+b", g_sub.url);
    TEST_ASSERT_EQUAL(1, wf_form_begin(&g_fp, &base, "/post", "PoSt", "application/x-www-form-urlencoded"));
    wf_form_add(&g_fp, &base, "textarea", "msg", "l1\nl2", 0, 0, 0);
    wf_form_add(&g_fp, &base, "password", "pw", "s3/=", 0, 0, 0);
    wf_form_add(&g_fp, &base, "checkbox", "ok", "", 0, 1, 0);
    wf_form_end(&g_fp);
    TEST_ASSERT_EQUAL(0, wf_form_submit(&g_fp, 1, -1, &g_sub));
    TEST_ASSERT_EQUAL(WF_METHOD_POST, g_sub.method);
    TEST_ASSERT_EQUAL_STRING("http://10.0.2.2:8080/post", g_sub.url);
    TEST_ASSERT_EQUAL_STRING("msg=l1%0D%0Al2&pw=s3%2F%3D&ok=on", g_sub.body);
    TEST_ASSERT_EQUAL((int)strlen(g_sub.body), g_sub.body_len);
    TEST_ASSERT_EQUAL_STRING("application/x-www-form-urlencoded", g_sub.content_type);
    /* multipart and javascript: actions are refused */
    TEST_ASSERT_EQUAL(2, wf_form_begin(&g_fp, &base, "/up", "post", "multipart/form-data"));
    wf_form_end(&g_fp);
    TEST_ASSERT_EQUAL(-1, wf_form_submit(&g_fp, 2, -1, &g_sub));
    TEST_ASSERT_EQUAL(-1, wf_form_begin(&g_fp, &base, "javascript:go()", "get", 0));
    TEST_ASSERT_EQUAL(1, g_fp.dropped);
}

static void test_password_display_and_implicit_form(void) {
    wf_url_t base;
    char d[64];
    int f;
    wf_form_reset(&g_fp);
    TEST_ASSERT_EQUAL(0, wf_url_parse("https://ex.org/p", &base));
    f = wf_form_add(&g_fp, &base, "password", "pw", "abc", 0, 0, 0);   /* no <form>: implicit */
    TEST_ASSERT_EQUAL(0, f);
    TEST_ASSERT_EQUAL(1, g_fp.nforms);
    TEST_ASSERT_EQUAL(1, g_fp.form[0].implicit);
    TEST_ASSERT_EQUAL(1, wf_form_add(&g_fp, &base, "", "q", "", 0, 0, 0));
    TEST_ASSERT_EQUAL(1, g_fp.nforms);
    wf_form_display(&g_fp, 0, d, (int)sizeof(d));
    TEST_ASSERT_NOT_NULL(strstr(d, "***"));
    TEST_ASSERT_NULL(strstr(d, "abc"));
    TEST_ASSERT_EQUAL(0, wf_form_focus_next(&g_fp, 1));
    TEST_ASSERT_EQUAL(0, g_fp.focus);
    TEST_ASSERT_EQUAL(0, wf_form_focus_next(&g_fp, 1));
    TEST_ASSERT_EQUAL(1, g_fp.focus);
    TEST_ASSERT_EQUAL(0, wf_form_focus_next(&g_fp, 1));
    TEST_ASSERT_EQUAL(0, g_fp.focus);
    TEST_ASSERT_EQUAL(0, wf_form_focus_next(&g_fp, -1));
    TEST_ASSERT_EQUAL(1, g_fp.focus);
    TEST_ASSERT_EQUAL(1, wf_form_first_text(&g_fp, -1) + 1);
}

static void test_bounds(void) {
    wf_url_t base;
    int i, k;
    char name[8];
    wf_form_reset(&g_fp);
    TEST_ASSERT_EQUAL(0, wf_url_parse("https://ex.org/", &base));
    for (i = 0; i < WF_FORM_MAX; i++) TEST_ASSERT_EQUAL(i, wf_form_begin(&g_fp, &base, "/a", "get", 0));
    TEST_ASSERT_EQUAL(-1, wf_form_begin(&g_fp, &base, "/a", "get", 0));
    g_fp.current = 0;
    for (i = 0; i < WF_FORM_FIELDS; i++) { sprintf(name, "f%d", i); TEST_ASSERT_EQUAL(i, wf_form_add(&g_fp, &base, "text", name, "", 0, 0, 3)); }
    TEST_ASSERT_EQUAL(-1, wf_form_add(&g_fp, &base, "text", "x", "", 0, 0, 0));
    TEST_ASSERT_TRUE(g_fp.dropped >= 2);
    wf_form_focus(&g_fp, 0);
    for (k = 0; k < 3; k++) TEST_ASSERT_EQUAL(0, wf_form_insert(&g_fp, 'z'));
    TEST_ASSERT_EQUAL(-1, wf_form_insert(&g_fp, 'z'));      /* maxlength 3 */
    /* a value of WF_FORM_VALUE - 1 bytes of non-ASCII still encodes or fails cleanly */
    {
        static char big[WF_FORM_VALUE];
        memset(big, 0xE9, sizeof(big) - 1);
        big[sizeof(big) - 1] = 0;
        g_fp.field[1].maxlen = WF_FORM_VALUE - 1;
        TEST_ASSERT_EQUAL(0, wf_form_set(&g_fp, 1, big));
        TEST_ASSERT_EQUAL(-1, wf_form_submit(&g_fp, 0, -1, &g_sub));   /* 255 * 6 bytes > query buffer */
    }
}

static void test_href(void) {
    int n = -1;
    TEST_ASSERT_EQUAL('f', wf_form_href("mohhdy-form:f:12", &n));
    TEST_ASSERT_EQUAL(12, n);
    TEST_ASSERT_EQUAL('s', wf_form_href("mohhdy-form:s:0", &n));
    TEST_ASSERT_EQUAL(0, n);
    TEST_ASSERT_EQUAL(0, wf_form_href("mohhdy-form:x:1", &n));
    TEST_ASSERT_EQUAL(0, wf_form_href("mohhdy-form:f:", &n));
    TEST_ASSERT_EQUAL(0, wf_form_href("mohhdy-form:f:1a", &n));
    TEST_ASSERT_EQUAL(0, wf_form_href("https://x/", &n));
}

static void test_request_head(void) {
    wf_url_t u;
    static char o[1024];
    TEST_ASSERT_EQUAL(0, wf_url_parse("https://api.example/v1?q=a", &u));
    TEST_ASSERT_TRUE(wf_http_build_request(&u, "POST", 0, 11, "X-Key: k\r\n", o, (int)sizeof(o)) > 0);
    TEST_ASSERT_NOT_NULL(strstr(o, "POST /v1?q=a HTTP/1.1\r\nHost: api.example\r\n"));
    TEST_ASSERT_NOT_NULL(strstr(o, "Content-Type: application/x-www-form-urlencoded\r\nContent-Length: 11\r\n"));
    TEST_ASSERT_NOT_NULL(strstr(o, "X-Key: k\r\n\r\n"));
    TEST_ASSERT_EQUAL(-1, wf_http_build_request(&u, "GET", 0, -1, "X: a\nY: b\r\n", o, (int)sizeof(o)));
    TEST_ASSERT_EQUAL(-1, wf_http_build_request(&u, "GET", 0, -1, "X: a", o, (int)sizeof(o)));
    TEST_ASSERT_EQUAL(-1, wf_http_build_request(&u, "POST", "a\r\nb", 1, 0, o, (int)sizeof(o)));
    TEST_ASSERT_TRUE(wf_http_build_get(&u, o, (int)sizeof(o)) > 0);
    TEST_ASSERT_NULL(strstr(o, "Content-Length"));
    TEST_ASSERT_EQUAL(0, wf_url_resolve(&u, "?q=b", &u));
    TEST_ASSERT_EQUAL_STRING("/v1?q=b", u.path);
}

/* in-memory transport: replies in order, records every request */
static const char *g_replies[4];
static int g_nrep, g_next, g_off, g_cur;
static char g_reqs[4][1024];
static int g_reqlen[4], g_nreq;

static int m_resolve(void *n, const char *h, uint8_t ip[4]) { (void)n; (void)h; ip[0] = 10; ip[1] = 0; ip[2] = 2; ip[3] = 2; return 0; }
static int m_connect(void *n, const uint8_t ip[4], int port) {
    (void)n; (void)ip; (void)port;
    if (g_next >= g_nrep) return -1;
    g_cur = g_next++; g_off = 0;
    g_reqlen[g_nreq] = 0;
    g_nreq++;
    return g_cur;
}
static int m_send(void *n, int h, const uint8_t *d, int len) {
    int r = g_nreq - 1, k;
    (void)n; (void)h;
    for (k = 0; k < len && g_reqlen[r] < 1023; k++) g_reqs[r][g_reqlen[r]++] = (char)d[k];
    g_reqs[r][g_reqlen[r]] = 0;
    return len;
}
static int m_recv(void *n, int h, uint8_t *d, int cap) {
    const char *s = g_replies[h];
    int l = (int)strlen(s) - g_off, k;
    (void)n;
    if (l <= 0) return 0;
    if (l > cap) l = cap;
    for (k = 0; k < l; k++) d[k] = (uint8_t)s[g_off + k];
    g_off += l;
    return l;
}
static void m_close(void *n, int h) { (void)n; (void)h; }
static int m_rand(void *n, uint8_t *o, int len) { int i; (void)n; for (i = 0; i < len; i++) o[i] = (uint8_t)i; return 0; }

static void test_fetch_post_303(void) {
    static wf_fetch_env_t e;
    static wf_fetch_result_t res;
    static uint8_t body[256];
    wf_fetch_req_t rq;
    memset(&e, 0, sizeof(e));
    e.resolve = m_resolve; e.connect = m_connect; e.send = m_send; e.recv = m_recv; e.close = m_close; e.rand = m_rand;
    e.max_redirects = 5;
    g_replies[0] = "HTTP/1.1 303 See Other\r\nLocation: /done?id=7\r\nContent-Length: 0\r\n\r\n";
    g_replies[1] = "HTTP/1.1 200 OK\r\nContent-Type: text/html\r\nContent-Length: 2\r\n\r\nok";
    g_nrep = 2; g_next = 0; g_nreq = 0;
    rq.method = "POST"; rq.body = "a=1&b=%C3%A9"; rq.body_len = 12; rq.ctype = 0; rq.extra = "X-Api: secret\r\n";
    TEST_ASSERT_EQUAL(0, wf_fetch_ex(&e, "http://10.0.2.2/form", &rq, body, (int)sizeof(body), &res));
    TEST_ASSERT_EQUAL(200, res.status);
    TEST_ASSERT_EQUAL(1, res.redirects);
    TEST_ASSERT_EQUAL(2, g_nreq);
    TEST_ASSERT_NOT_NULL(strstr(g_reqs[0], "POST /form HTTP/1.1"));
    TEST_ASSERT_NOT_NULL(strstr(g_reqs[0], "Content-Length: 12\r\n"));
    TEST_ASSERT_NOT_NULL(strstr(g_reqs[0], "\r\n\r\na=1&b=%C3%A9"));
    TEST_ASSERT_NOT_NULL(strstr(g_reqs[1], "GET /done?id=7 HTTP/1.1"));
    TEST_ASSERT_NULL(strstr(g_reqs[1], "Content-Length"));
    TEST_ASSERT_NOT_NULL(strstr(g_reqs[1], "X-Api: secret"));   /* same host: kept */
    /* 307 repeats the POST; another host drops the extra header */
    g_replies[0] = "HTTP/1.1 307 Temporary\r\nLocation: http://other.test/x\r\nContent-Length: 0\r\n\r\n";
    g_nrep = 2; g_next = 0; g_nreq = 0;
    TEST_ASSERT_EQUAL(0, wf_fetch_ex(&e, "http://10.0.2.2/form", &rq, body, (int)sizeof(body), &res));
    TEST_ASSERT_NOT_NULL(strstr(g_reqs[1], "POST /x HTTP/1.1"));
    TEST_ASSERT_NOT_NULL(strstr(g_reqs[1], "a=1&b=%C3%A9"));
    TEST_ASSERT_NULL(strstr(g_reqs[1], "X-Api"));
}

/* Lot 71: checkbox, radio and select controls */
static void test_toggle_controls(void) {
    wf_url_t base;
    char d[80];
    int cb, r1, r2, r3, sel;
    wf_form_reset(&g_fp);
    TEST_ASSERT_EQUAL(0, wf_url_parse("http://10.0.2.2/f", &base));
    TEST_ASSERT_EQUAL(0, wf_form_begin(&g_fp, &base, "/echo", "get", 0));
    cb = wf_form_add(&g_fp, &base, "checkbox", "safe", "", "Filtre", 0, 0);
    r1 = wf_form_add(&g_fp, &base, "radio", "t", "web", 0, 1, 0);
    r2 = wf_form_add(&g_fp, &base, "radio", "t", "img", 0, 0, 0);
    r3 = wf_form_add(&g_fp, &base, "radio", "u", "x", 0, 0, 0);
    sel = wf_form_add(&g_fp, &base, "select", "lr", "", 0, 0, 0);
    TEST_ASSERT_EQUAL(0, wf_form_add_option(&g_fp, sel, "", "tout", 0));
    TEST_ASSERT_EQUAL(1, wf_form_add_option(&g_fp, sel, "lang_fr", "fran\xe7" "ais", 1));
    TEST_ASSERT_EQUAL(2, wf_form_add_option(&g_fp, sel, "lang_en", "anglais", 0));
    TEST_ASSERT_EQUAL(-1, wf_form_add_option(&g_fp, cb, "a", "b", 0));       /* not a select */
    wf_form_end(&g_fp);
    TEST_ASSERT_EQUAL(1, wf_form_is_toggle(&g_fp, cb));
    TEST_ASSERT_EQUAL(0, wf_form_is_toggle(&g_fp, -1));
    TEST_ASSERT_EQUAL(0, wf_form_submit(&g_fp, 0, -1, &g_sub));
    TEST_ASSERT_EQUAL_STRING("http://10.0.2.2/echo?t=web&lr=lang_fr", g_sub.url);
    wf_form_display(&g_fp, cb, d, (int)sizeof(d));
    TEST_ASSERT_EQUAL_STRING("[ ] Filtre", d);
    TEST_ASSERT_EQUAL(0, wf_form_activate(&g_fp, cb));
    wf_form_display(&g_fp, cb, d, (int)sizeof(d));
    TEST_ASSERT_EQUAL_STRING("[x] Filtre", d);
    TEST_ASSERT_EQUAL(0, wf_form_activate(&g_fp, r2));                        /* the group moves to img */
    TEST_ASSERT_EQUAL(0, g_fp.field[r1].checked);
    wf_form_display(&g_fp, r2, d, (int)sizeof(d));
    TEST_ASSERT_EQUAL_STRING("(*) img", d);
    TEST_ASSERT_EQUAL(0, g_fp.field[r3].checked);                             /* other group untouched */
    wf_form_display(&g_fp, sel, d, (int)sizeof(d));
    TEST_ASSERT_EQUAL_STRING("[ fran\xe7" "ais v ]", d);
    TEST_ASSERT_EQUAL(0, wf_form_activate(&g_fp, sel));
    TEST_ASSERT_EQUAL_STRING("lang_en", g_fp.field[sel].value);
    TEST_ASSERT_EQUAL(0, wf_form_activate(&g_fp, sel));                       /* wraps */
    TEST_ASSERT_EQUAL_STRING("", g_fp.field[sel].value);
    TEST_ASSERT_EQUAL(0, wf_form_submit(&g_fp, 0, -1, &g_sub));
    TEST_ASSERT_EQUAL_STRING("http://10.0.2.2/echo?safe=on&t=img&lr=", g_sub.url);
    /* console values */
    TEST_ASSERT_EQUAL(0, wf_form_set(&g_fp, cb, "off"));
    TEST_ASSERT_EQUAL(-1, wf_form_set(&g_fp, cb, "maybe"));
    TEST_ASSERT_EQUAL(0, wf_form_set(&g_fp, r1, "on"));
    TEST_ASSERT_EQUAL(-1, wf_form_set(&g_fp, r1, "off"));                      /* a radio is unchecked by another */
    TEST_ASSERT_EQUAL(0, wf_form_set(&g_fp, sel, "anglais"));                  /* by label */
    TEST_ASSERT_EQUAL_STRING("lang_en", g_fp.field[sel].value);
    TEST_ASSERT_EQUAL(0, wf_form_set(&g_fp, sel, "#2"));
    TEST_ASSERT_EQUAL_STRING("lang_fr", g_fp.field[sel].value);
    TEST_ASSERT_EQUAL(0, wf_form_set(&g_fp, sel, "lang_en"));                  /* by value */
    TEST_ASSERT_EQUAL(-1, wf_form_set(&g_fp, sel, "#9"));
    TEST_ASSERT_EQUAL(-1, wf_form_set(&g_fp, sel, "klingon"));
    TEST_ASSERT_EQUAL(0, wf_form_submit(&g_fp, 0, -1, &g_sub));
    TEST_ASSERT_EQUAL_STRING("http://10.0.2.2/echo?t=web&lr=lang_en", g_sub.url);
    TEST_ASSERT_EQUAL_STRING("lang_en", wf_form_option_value(&g_fp, sel, 2));
    TEST_ASSERT_EQUAL_STRING("anglais", wf_form_option_label(&g_fp, sel, 2));
    TEST_ASSERT_NULL(wf_form_option_value(&g_fp, sel, 3));
    TEST_ASSERT_EQUAL(-1, wf_form_activate(&g_fp, -1));
}

/* the option pool and the per-select cap are bounded */
/* Lot 71: the keyboard reaches the controls */
static void test_keyboard_controls(void) {
    wf_url_t base;
    char d[80];
    int q, cb, sel;
    wf_form_reset(&g_fp);
    TEST_ASSERT_EQUAL(0, wf_url_parse("http://h/", &base));
    TEST_ASSERT_EQUAL(0, wf_form_begin(&g_fp, &base, "/s", "get", 0));
    q = wf_form_add(&g_fp, &base, "text", "q", "", 0, 0, 0);
    cb = wf_form_add(&g_fp, &base, "checkbox", "safe", "", "Filtre", 0, 0);
    (void)wf_form_add(&g_fp, &base, "submit", "go", "Go", 0, 0, 0);
    sel = wf_form_add(&g_fp, &base, "select", "lr", "", 0, 0, 0);
    wf_form_add_option(&g_fp, sel, "fr", "Fran\xe7" "ais", 1);
    wf_form_add_option(&g_fp, sel, "en", "anglais", 0);
    wf_form_add_option(&g_fp, sel, "de", "allemand", 0);
    wf_form_end(&g_fp);
    TEST_ASSERT_EQUAL(0, wf_form_focus(&g_fp, q));
    TEST_ASSERT_EQUAL(0, wf_form_focus_next(&g_fp, 1));            /* Tab: the checkbox */
    TEST_ASSERT_EQUAL(cb, g_fp.focus);
    wf_form_display(&g_fp, cb, d, (int)sizeof(d));
    TEST_ASSERT_EQUAL_STRING("> [ ] Filtre", d);
    TEST_ASSERT_EQUAL(0, wf_form_insert(&g_fp, ' '));               /* Space toggles */
    TEST_ASSERT_EQUAL(1, g_fp.field[cb].checked);
    TEST_ASSERT_EQUAL(-1, wf_form_insert(&g_fp, 'x'));              /* no text in a checkbox */
    TEST_ASSERT_EQUAL(-1, wf_form_backspace(&g_fp));
    TEST_ASSERT_EQUAL(-1, wf_form_move(&g_fp, 1));
    TEST_ASSERT_EQUAL(0, wf_form_focus_next(&g_fp, 1));            /* the submit button is skipped */
    TEST_ASSERT_EQUAL(sel, g_fp.focus);
    TEST_ASSERT_EQUAL(0, wf_form_move(&g_fp, 1));                   /* RIGHT */
    TEST_ASSERT_EQUAL_STRING("en", g_fp.field[sel].value);
    TEST_ASSERT_EQUAL(0, wf_form_move(&g_fp, 1000));                /* END */
    TEST_ASSERT_EQUAL_STRING("de", g_fp.field[sel].value);
    TEST_ASSERT_EQUAL(-1, wf_form_move(&g_fp, 1));                  /* already last */
    TEST_ASSERT_EQUAL(0, wf_form_insert(&g_fp, 'F'));               /* a letter */
    TEST_ASSERT_EQUAL_STRING("fr", g_fp.field[sel].value);
    TEST_ASSERT_EQUAL(0, wf_form_insert(&g_fp, 'a'));               /* anglais, then allemand */
    TEST_ASSERT_EQUAL_STRING("en", g_fp.field[sel].value);
    TEST_ASSERT_EQUAL(0, wf_form_insert(&g_fp, 'a'));
    TEST_ASSERT_EQUAL_STRING("de", g_fp.field[sel].value);
    TEST_ASSERT_EQUAL(-1, wf_form_insert(&g_fp, 'z'));
    wf_form_display(&g_fp, sel, d, (int)sizeof(d));
    TEST_ASSERT_EQUAL_STRING("> [ allemand v ]", d);
    TEST_ASSERT_EQUAL(0, wf_form_move(&g_fp, -1000));               /* HOME */
    TEST_ASSERT_EQUAL_STRING("fr", g_fp.field[sel].value);
    TEST_ASSERT_EQUAL(0, wf_form_focus_next(&g_fp, 1));            /* wraps to the text field */
    TEST_ASSERT_EQUAL(q, g_fp.focus);
    TEST_ASSERT_EQUAL(0, wf_form_submit(&g_fp, 0, -1, &g_sub));
    TEST_ASSERT_EQUAL_STRING("http://h/s?q=&safe=on&lr=fr", g_sub.url);
}

static void test_option_bounds(void) {
    wf_url_t base;
    char v[8];
    int sel, k, kept = 0;
    wf_form_reset(&g_fp);
    TEST_ASSERT_EQUAL(0, wf_url_parse("http://h/", &base));
    sel = wf_form_add(&g_fp, &base, "select", "s", "", 0, 0, 0);
    for (k = 0; k < WF_FORM_OPTIONS + 5; k++) {
        v[0] = 'o'; v[1] = (char)('a' + k % 26); v[2] = 0;
        if (wf_form_add_option(&g_fp, sel, v, v, 0) >= 0) kept++;
    }
    TEST_ASSERT_EQUAL(WF_FORM_OPTIONS, kept);
    TEST_ASSERT_EQUAL(5, g_fp.opt_dropped);
    TEST_ASSERT_TRUE(g_fp.opt_used <= WF_FORM_OPT_POOL);
    TEST_ASSERT_EQUAL(-1, wf_form_add_option(&g_fp, 99, "a", "a", 0));
}

int main(void) {
    unity_init();
    RUN_TEST(test_toggle_controls);
    RUN_TEST(test_option_bounds);
    RUN_TEST(test_keyboard_controls);
    RUN_TEST(test_edit_and_display);
    RUN_TEST(test_submit_get);
    RUN_TEST(test_submit_post_and_query_replaced);
    RUN_TEST(test_password_display_and_implicit_form);
    RUN_TEST(test_bounds);
    RUN_TEST(test_href);
    RUN_TEST(test_request_head);
    RUN_TEST(test_fetch_post_303);
    unity_print_results();
    unity_cleanup();
    return unity_stats.tests_failed == 0 ? 0 : 1;
}
