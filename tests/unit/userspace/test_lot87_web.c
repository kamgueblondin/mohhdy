/* Lot 87: honest reading of real pages - named script walls, the noscript
 * fallback, block pages, and the 2026-11-09 re-measure of the compat set. */
#include "../../framework/unity.h"
#include "wf_compat.h"
#include <stdio.h>
#include <string.h>

static char g_buf[8192];

static int load(const char *name) {
    static const char *const pre[] = {
        "fixtures/compat/", "tests/fixtures/compat/", "../fixtures/compat/", "../../fixtures/compat/", 0
    };
    char path[256];
    int i;
    for (i = 0; pre[i]; i++) {
        FILE *f;
        int n;
        snprintf(path, sizeof(path), "%s%s", pre[i], name);
        f = fopen(path, "rb");
        if (!f) continue;
        n = (int)fread(g_buf, 1, sizeof(g_buf) - 1, f);
        fclose(f);
        g_buf[n] = 0;
        return n;
    }
    return -1;
}

static void scan_str(const char *html, int status, wf_compat_t *c) {
    wf_compat_scan_ex(html, (int)strlen(html), status, c);
}

static void test_measured_set(void) {
    int i;
    TEST_ASSERT_EQUAL(6, wf_compat_expect87_count());
    TEST_ASSERT_NULL(wf_compat_expect87_at(-1));
    TEST_ASSERT_NULL(wf_compat_expect87_at(6));
    TEST_ASSERT_NULL(wf_compat_expect87_file(6));
    for (i = 0; i < wf_compat_expect87_count(); i++) {
        const wf_compat_expect_t *e = wf_compat_expect87_at(i);
        wf_compat_t c;
        char line[400];
        int n;
        TEST_ASSERT_NOT_NULL(e);
        n = load(wf_compat_expect87_file(i));
        TEST_ASSERT(n > 0);
        wf_compat_scan_ex(g_buf, n, wf_compat_expect87_status(i), &c);
        TEST_ASSERT(c.bytes >= e->min_bytes);
        TEST_ASSERT_EQUAL(e->js_wall, c.js_wall);
        TEST_ASSERT_EQUAL(wf_compat_expect87_wall(i), c.wall_kind);
        TEST_ASSERT_EQUAL_STRING(e->verdict, wf_compat_verdict(&c));
        TEST_ASSERT(wf_compat_line(e->host, &c, line, (int)sizeof(line)) > 0);
        TEST_ASSERT_NOT_NULL(strstr(line, e->host));
        if (c.wall_kind) TEST_ASSERT_NOT_NULL(strstr(line, wf_compat_wall_name(c.wall_kind)));
        else TEST_ASSERT_NULL(strstr(line, " wall "));
    }
}

static void test_google_fallback(void) {
    wf_compat_t c;
    char adv[300];
    int n = load("google_20261109.html");
    TEST_ASSERT(n > 0);
    wf_compat_scan_ex(g_buf, n, 200, &c);
    TEST_ASSERT_EQUAL(WF_WALL_ENABLEJS, c.wall_kind);
    /* the refresh inside noscript is the fallback; it is not a page refresh */
    TEST_ASSERT_EQUAL_STRING("/httpservice/retry/enablejs", c.fallback);
    TEST_ASSERT_EQUAL(0, c.meta_refresh);
    TEST_ASSERT(wf_compat_advice(&c, adv, (int)sizeof(adv)) > 0);
    TEST_ASSERT_NOT_NULL(strstr(adv, "web-compat js-wall: needs JavaScript"));
    TEST_ASSERT_NOT_NULL(strstr(adv, "fallback /httpservice/retry/enablejs"));
    /* never "works" for a wall */
    TEST_ASSERT_NULL(strstr(adv, "readable"));
}

static void test_ddg_lite_form(void) {
    wf_compat_t c;
    char adv[300];
    int n = load("ddglite_20261109.html");
    TEST_ASSERT(n > 0);
    wf_compat_scan_ex(g_buf, n, 200, &c);
    TEST_ASSERT_EQUAL(0, c.scripts);
    TEST_ASSERT_EQUAL(2, c.forms);
    TEST_ASSERT_EQUAL(1, c.tables);
    /* the query box and two submit buttons; the hidden fields do not count */
    TEST_ASSERT_EQUAL(3, c.inputs);
    TEST_ASSERT_EQUAL(1, c.search_form);
    TEST_ASSERT_EQUAL(WF_WALL_NONE, c.wall_kind);
    TEST_ASSERT(wf_compat_advice(&c, adv, (int)sizeof(adv)) > 0);
    TEST_ASSERT_EQUAL_STRING("web-compat table: readable without scripts, search form", adv);
}

static void test_blocked_needs_status(void) {
    wf_compat_t c;
    int n = load("reddit_20261109.html");
    TEST_ASSERT(n > 0);
    wf_compat_scan_ex(g_buf, n, 403, &c);
    TEST_ASSERT_EQUAL(WF_WALL_BLOCKED, c.wall_kind);
    TEST_ASSERT_EQUAL(0, c.js_wall);
    TEST_ASSERT_EQUAL_STRING("blocked", wf_compat_verdict(&c));
    /* the same words with a 200 are an ordinary page about blocking */
    wf_compat_scan_ex(g_buf, n, 200, &c);
    TEST_ASSERT_EQUAL(WF_WALL_NONE, c.wall_kind);
    TEST_ASSERT_EQUAL_STRING("reader", wf_compat_verdict(&c));
    /* a 403 without a notice is not named a block page */
    scan_str("<html><body><p>Members only area, please sign in with your account to continue reading.</p>"
             "</body></html>", 403, &c);
    TEST_ASSERT_EQUAL(WF_WALL_NONE, c.wall_kind);
    scan_str("<html><body><h1>429 Too Many Requests</h1></body></html>", 429, &c);
    TEST_ASSERT_EQUAL(WF_WALL_BLOCKED, c.wall_kind);
}

static void test_challenge_and_captcha(void) {
    wf_compat_t c;
    char adv[300];
    int n = load("challenge_20261109.html");
    TEST_ASSERT(n > 0);
    wf_compat_scan_ex(g_buf, n, 403, &c);
    /* the challenge wins over its own "Enable JavaScript" line and the 403 */
    TEST_ASSERT_EQUAL(WF_WALL_CHALLENGE, c.wall_kind);
    TEST_ASSERT_EQUAL(1, c.meta_refresh);
    TEST_ASSERT(wf_compat_advice(&c, adv, (int)sizeof(adv)) > 0);
    TEST_ASSERT_NOT_NULL(strstr(adv, "bot challenge"));
    TEST_ASSERT_NOT_NULL(strstr(adv, "Not retried"));
    TEST_ASSERT_NULL(strstr(adv, "fallback"));
    /* a login form with a captcha and real text is not a challenge page */
    {
        static char big[2048];
        int k;
        strcpy(big, "<html><body><h1>Sign in</h1><form action=/login method=post>"
                    "<input name=user><input type=password name=pw>"
                    "<div class=\"g-recaptcha\" data-sitekey=x></div></form><p>");
        for (k = 0; k < 20; k++) strcat(big, "Read our help pages for account recovery and privacy. ");
        strcat(big, "</p></body></html>");
        scan_str(big, 200, &c);
        TEST_ASSERT_EQUAL(WF_WALL_NONE, c.wall_kind);
        TEST_ASSERT_EQUAL(2, c.inputs);
    }
    scan_str("<html><body><div class=\"h-captcha\"></div></body></html>", 200, &c);
    TEST_ASSERT_EQUAL(WF_WALL_CHALLENGE, c.wall_kind);
}

static void test_spa_shell(void) {
    wf_compat_t c;
    int n = load("spa_20261109.html");
    TEST_ASSERT(n > 0);
    wf_compat_scan_ex(g_buf, n, 200, &c);
    TEST_ASSERT_EQUAL(1, c.empty_root);
    TEST_ASSERT_EQUAL(WF_WALL_SPA, c.wall_kind);
    TEST_ASSERT_EQUAL_STRING("spa-shell", wf_compat_wall_name(c.wall_kind));
    /* a server-rendered root is not a shell */
    scan_str("<html><head><script src=a.js></script></head><body><div id=\"__next\"><h1>Docs</h1>"
             "<p>Server rendered text that the reader can show as it is, with no script at all.</p></div>"
             "</body></html>", 200, &c);
    TEST_ASSERT_EQUAL(0, c.empty_root);
    TEST_ASSERT_EQUAL(WF_WALL_NONE, c.wall_kind);
    /* ids match whole: "rooted" is not "root" */
    scan_str("<html><script></script><body><div id=rooted></div><p>x</p></body></html>", 200, &c);
    TEST_ASSERT_EQUAL(0, c.empty_root);
    scan_str("<html><script></script><body><div class=a id='APP'>\n </div></body></html>", 200, &c);
    TEST_ASSERT_EQUAL(1, c.empty_root);
}

static void test_noscript_link_and_attrs(void) {
    wf_compat_t c;
    char adv[300];
    scan_str("<html><body><noscript><p>Go to the <a href='/basic?x=1&amp;y=2'>basic page</a></p></noscript>"
             "</body></html>", 200, &c);
    TEST_ASSERT_EQUAL_STRING("/basic?x=1&y=2", c.fallback);
    TEST_ASSERT_EQUAL(WF_WALL_NOSCRIPT, c.wall_kind);
    TEST_ASSERT_EQUAL(1, c.js_wall);
    TEST_ASSERT(wf_compat_advice(&c, adv, (int)sizeof(adv)) > 0);
    TEST_ASSERT_NOT_NULL(strstr(adv, "fallback /basic?x=1&y=2"));
    /* a link outside noscript is not a fallback */
    scan_str("<html><body><a href=/x>x</a><noscript>no</noscript><p>plenty of other readable text here, "
             "enough for a reader page with words.</p></body></html>", 200, &c);
    TEST_ASSERT_EQUAL(0, c.fallback[0]);
    TEST_ASSERT_EQUAL(WF_WALL_NONE, c.wall_kind);
    /* attribute names must start at a boundary: data-href is not href */
    scan_str("<html><body><noscript><a data-href=/no href=\"/yes\">y</a></noscript></body></html>", 200, &c);
    TEST_ASSERT_EQUAL_STRING("/yes", c.fallback);
    /* an unquoted refresh URL, upper case */
    scan_str("<html><head><noscript><META HTTP-EQUIV=Refresh CONTENT=\"0; URL=/nojs\"></noscript></head>"
             "<script></script></html>", 200, &c);
    TEST_ASSERT_EQUAL_STRING("/nojs", c.fallback);
    TEST_ASSERT_EQUAL(WF_WALL_NOSCRIPT, c.wall_kind);
}

static void test_names_and_bounds(void) {
    wf_compat_t c;
    char small[20];
    static char huge[400];
    int k;
    TEST_ASSERT_EQUAL_STRING("none", wf_compat_wall_name(0));
    TEST_ASSERT_EQUAL_STRING("enablejs", wf_compat_wall_name(WF_WALL_ENABLEJS));
    TEST_ASSERT_EQUAL_STRING("noscript", wf_compat_wall_name(WF_WALL_NOSCRIPT));
    TEST_ASSERT_EQUAL_STRING("challenge", wf_compat_wall_name(WF_WALL_CHALLENGE));
    TEST_ASSERT_EQUAL_STRING("blocked", wf_compat_wall_name(WF_WALL_BLOCKED));
    TEST_ASSERT_EQUAL_STRING("old-browser", wf_compat_wall_name(WF_WALL_BROWSER));
    TEST_ASSERT_EQUAL_STRING("none", wf_compat_wall_name(99));
    scan_str("<html><body><p>Your browser is not supported.</p></body></html>", 200, &c);
    TEST_ASSERT_EQUAL(WF_WALL_BROWSER, c.wall_kind);
    TEST_ASSERT_EQUAL(-1, wf_compat_advice(0, small, (int)sizeof(small)));
    TEST_ASSERT_EQUAL(-1, wf_compat_advice(&c, small, 8));
    TEST_ASSERT_EQUAL(-1, wf_compat_advice(&c, small, (int)sizeof(small)));   /* too long: refused */
    /* a fallback longer than the field is cut, not overrun */
    strcpy(huge, "<noscript><a href=\"/");
    for (k = 0; k < 300; k++) strcat(huge, "a");
    strcat(huge, "\">x</a></noscript>");
    scan_str(huge, 200, &c);
    TEST_ASSERT_EQUAL((int)sizeof(c.fallback) - 1, (int)strlen(c.fallback));
    wf_compat_scan_ex(0, 10, 200, &c);
    TEST_ASSERT_EQUAL_STRING("empty", wf_compat_verdict(&c));
}

int main(void) {
    unity_init();
    RUN_TEST(test_measured_set);
    RUN_TEST(test_google_fallback);
    RUN_TEST(test_ddg_lite_form);
    RUN_TEST(test_blocked_needs_status);
    RUN_TEST(test_challenge_and_captcha);
    RUN_TEST(test_spa_shell);
    RUN_TEST(test_noscript_link_and_attrs);
    RUN_TEST(test_names_and_bounds);
    return unity_stats.tests_failed == 0 ? 0 : 1;
}
