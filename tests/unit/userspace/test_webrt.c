/* Lot 5 Phase 3: web runtime building blocks (userspace/webrt.c). */
#include "../../framework/unity.h"
#include "../../../userspace/webrt.h"
#include <string.h>

static char out[2048];

/* US-038 */
static void test_cache_freshness_revalidation_and_lru(void) {
    webrt_cache_t c;
    const char* etag = 0;
    int i;
    webrt_cache_init(&c);
    TEST_ASSERT_EQUAL(WEBRT_CACHE_MISS, webrt_cache_lookup(&c, "/app.css", 0, &etag));
    TEST_ASSERT_TRUE(webrt_cache_store(&c, "/app.css", "public, max-age=60", "\"v1\"", 120, 10) >= 0);
    TEST_ASSERT_EQUAL(WEBRT_CACHE_FRESH, webrt_cache_lookup(&c, "/app.css", 69, &etag));
    TEST_ASSERT_EQUAL(WEBRT_CACHE_STALE, webrt_cache_lookup(&c, "/app.css", 70, &etag));
    TEST_ASSERT_EQUAL_STRING("\"v1\"", etag);
    TEST_ASSERT_EQUAL(1, webrt_cache_revalidated(&c, "/app.css", 304, "max-age=30", 0, 0, 70));
    TEST_ASSERT_EQUAL(WEBRT_CACHE_FRESH, webrt_cache_lookup(&c, "/app.css", 99, &etag));
    TEST_ASSERT_EQUAL(WEBRT_CACHE_STALE, webrt_cache_lookup(&c, "/app.css", 100, &etag));
    /* no-store is refused and evicts; no-cache always revalidates */
    TEST_ASSERT_EQUAL(-1, webrt_cache_store(&c, "/app.css", "no-store", 0, 1, 101));
    TEST_ASSERT_EQUAL(WEBRT_CACHE_MISS, webrt_cache_lookup(&c, "/app.css", 101, &etag));
    TEST_ASSERT_TRUE(webrt_cache_store(&c, "/api", "no-cache, max-age=600", "\"a\"", 9, 0) >= 0);
    TEST_ASSERT_EQUAL(WEBRT_CACHE_STALE, webrt_cache_lookup(&c, "/api", 1, &etag));
    TEST_ASSERT_EQUAL(0, webrt_cache_revalidated(&c, "/api", 500, 0, 0, 0, 2));
    TEST_ASSERT_EQUAL(WEBRT_CACHE_MISS, webrt_cache_lookup(&c, "/api", 3, &etag));
    /* LRU: fill 8 slots, touch the first, the second is evicted */
    for (i = 0; i < WEBRT_CACHE_SLOTS; i++) {
        char url[8] = {'/', 'p', (char)('0' + i), 0};
        TEST_ASSERT_TRUE(webrt_cache_store(&c, url, "max-age=100", 0, 1, 200) >= 0);
    }
    TEST_ASSERT_EQUAL(WEBRT_CACHE_FRESH, webrt_cache_lookup(&c, "/p0", 201, &etag));
    TEST_ASSERT_TRUE(webrt_cache_store(&c, "/new", "max-age=100", 0, 1, 202) >= 0);
    TEST_ASSERT_EQUAL(1U, c.evictions);
    TEST_ASSERT_EQUAL(WEBRT_CACHE_FRESH, webrt_cache_lookup(&c, "/p0", 203, &etag));
    TEST_ASSERT_EQUAL(WEBRT_CACHE_MISS, webrt_cache_lookup(&c, "/p1", 203, &etag));
    TEST_ASSERT_EQUAL(1U, c.revalidated);
    TEST_ASSERT_EQUAL(1U, c.refused);
}

/* US-039 */
static void test_csp_parse_and_sources(void) {
    webrt_csp_t p;
    const char* page = "https://app.mohhdy.local";
    TEST_ASSERT_EQUAL(4, webrt_csp_parse(&p, "default-src 'self'; script-src 'self' https://cdn.example.com;"
                                             " img-src * data:; style-src 'self' 'unsafe-inline'; script-src *"));
    TEST_ASSERT_EQUAL(0, p.errors);
    TEST_ASSERT_TRUE(webrt_csp_allows(&p, "script-src", page, "/js/app.js"));
    TEST_ASSERT_TRUE(webrt_csp_allows(&p, "script-src", page, "https://app.mohhdy.local/x.js"));
    TEST_ASSERT_TRUE(webrt_csp_allows(&p, "script-src", page, "https://cdn.example.com/lib.js"));
    TEST_ASSERT_FALSE(webrt_csp_allows(&p, "script-src", page, "https://evil.example.org/x.js"));
    TEST_ASSERT_FALSE(webrt_csp_allows(&p, "script-src", page, "http://cdn.example.com/lib.js"));
    TEST_ASSERT_FALSE(webrt_csp_allows(&p, "script-src", page, "inline"));
    TEST_ASSERT_TRUE(webrt_csp_allows(&p, "style-src", page, "inline"));
    TEST_ASSERT_TRUE(webrt_csp_allows(&p, "img-src", page, "https://any.where/i.png"));
    TEST_ASSERT_TRUE(webrt_csp_allows(&p, "img-src", page, "data:image/png;base64,AA"));
    /* connect-src falls back to default-src 'self' */
    TEST_ASSERT_TRUE(webrt_csp_allows(&p, "connect-src", page, "/api"));
    TEST_ASSERT_FALSE(webrt_csp_allows(&p, "connect-src", page, "https://tracker.example/p"));
    TEST_ASSERT_EQUAL(1, webrt_csp_parse(&p, "default-src 'none'"));
    TEST_ASSERT_FALSE(webrt_csp_allows(&p, "img-src", page, "/a.png"));
    TEST_ASSERT_EQUAL(1, webrt_csp_parse(&p, "img-src *.example.com"));
    TEST_ASSERT_TRUE(webrt_csp_allows(&p, "img-src", page, "https://img.example.com/a.png"));
    TEST_ASSERT_FALSE(webrt_csp_allows(&p, "img-src", page, "https://example.com.evil/a.png"));
    TEST_ASSERT_TRUE(webrt_csp_allows(&p, "script-src", page, "https://x/y.js")); /* no policy */
    TEST_ASSERT_TRUE(webrt_url_origin("HTTPS://Host:8443/p?q", out, sizeof(out)));
    TEST_ASSERT_EQUAL_STRING("https://host:8443", out);
    TEST_ASSERT_FALSE(webrt_url_origin("/relative/a://b", out, sizeof(out)));
}

static void test_sanitizer(void) {
    int removed = -1;
    webrt_sanitize_html("<p onclick=\"steal()\" class=\"x\">hi<script>alert(1)</script>"
                        "<a href=\" javascript:go()\">l</a><a href=\"/ok\">ok</a>"
                        "<IFRAME src=\"https://x\"></iframe><img src=x onerror=boom alt=\"\"></p>",
                        out, sizeof(out), &removed);
    TEST_ASSERT_EQUAL(5, removed);
    TEST_ASSERT_EQUAL_STRING("<p class=\"x\">hi<a>l</a><a href=\"/ok\">ok</a><img src=x alt=\"\"></p>", out);
    webrt_sanitize_html("plain <b>text</b> 1 < 2", out, sizeof(out), &removed);
    TEST_ASSERT_EQUAL(0, removed);
    TEST_ASSERT_EQUAL_STRING("plain <b>text</b> 1 < 2", out);
    webrt_sanitize_html("a<script>never closed", out, sizeof(out), &removed);
    TEST_ASSERT_EQUAL_STRING("a", out);
}

/* Lot 7 US-039: page mode keeps <style> but filters what would fetch or run */
static void test_sanitize_page(void) {
    int removed = -1;
    webrt_sanitize_page("<style>@import 'x.css';\np { color: red; background: url(http://x/t.png) }\n"
                        ".a { width: expression(alert(1)); height: 4px }</style>"
                        "<p style=\"color:blue\">ok</p><p style=\"background:url(javascript:x)\">bad</p>"
                        "<script>x()</script><iframe srcdoc=\"<b>\"></iframe><div srcdoc=\"y\">d</div>",
                        out, sizeof(out), &removed);
    TEST_ASSERT_EQUAL_STRING("<style>\np { color: red; }\n.a {  height: 4px }</style>"
                             "<p style=\"color:blue\">ok</p><p>bad</p><div>d</div>", out);
    TEST_ASSERT_EQUAL(7, removed);
    /* an @import after leading whitespace is dropped too */
    webrt_sanitize_page("<style>\n  @import \"e.css\";\nb{color:red}</style>", out, sizeof(out), &removed);
    TEST_ASSERT_EQUAL_STRING("<style>\n  \nb{color:red}</style>", out);
    TEST_ASSERT_EQUAL(1, removed);
    /* the strict mode still drops <style> */
    webrt_sanitize_html("<style>p{}</style>t", out, sizeof(out), &removed);
    TEST_ASSERT_EQUAL_STRING("t", out);
    TEST_ASSERT_EQUAL(1, removed);
}

/* US-040 */
static void test_media_queries(void) {
    webrt_viewport_t phone = {360, 640, 0}, desk = {1024, 768, 1};
    TEST_ASSERT_TRUE(webrt_mq_match("screen and (max-width: 600px)", &phone));
    TEST_ASSERT_FALSE(webrt_mq_match("screen and (max-width: 600px)", &desk));
    TEST_ASSERT_TRUE(webrt_mq_match("(min-width: 40em)", &desk));          /* 640px */
    TEST_ASSERT_TRUE(webrt_mq_match("(orientation: portrait)", &phone));
    TEST_ASSERT_TRUE(webrt_mq_match("(orientation: landscape)", &desk));
    TEST_ASSERT_TRUE(webrt_mq_match("print, (prefers-color-scheme: dark)", &desk));
    TEST_ASSERT_FALSE(webrt_mq_match("print", &desk));
    TEST_ASSERT_TRUE(webrt_mq_match("not print", &desk));
    TEST_ASSERT_FALSE(webrt_mq_match("(hover)", &desk));
    TEST_ASSERT_TRUE(webrt_mq_match("only screen and (min-width: 300px) and (max-height: 700px)", &phone));
}

static void test_flatten_media(void) {
    webrt_viewport_t phone = {360, 640, 0};
    int kept = 0, dropped = 0;
    webrt_css_flatten_media("p{color:red}@media (max-width:600px){h1{color:blue}}"
                            "@media (min-width:900px){h1{color:green}}b{x:y}",
                            &phone, out, sizeof(out), &kept, &dropped);
    TEST_ASSERT_EQUAL(1, kept);
    TEST_ASSERT_EQUAL(1, dropped);
    TEST_ASSERT_EQUAL_STRING("p{color:red}h1{color:blue}b{x:y}", out);
}

/* US-041 */
static void test_custom_properties(void) {
    int defined = 0, unresolved = 0;
    webrt_css_resolve_vars(":root{--fg: #222; --accent: var(--fg);}"
                           "h1{color: var(--accent); background: var(--bg, white)}"
                           "p{color: var(--nope)}",
                           out, sizeof(out), &defined, &unresolved);
    TEST_ASSERT_EQUAL(2, defined);
    TEST_ASSERT_EQUAL(1, unresolved);
    TEST_ASSERT_EQUAL_STRING(":root{ }h1{color: #222; background: white}p{color: }", out);
}

/* US-042 */
static void test_components(void) {
    int defined = 0, expanded = 0;
    webrt_components_expand("<template id=\"x-card\"><div class=\"card\"><slot></slot></div></template>"
                            "<template id=\"bad\"><p>no dash</p></template>"
                            "<x-card>Hello</x-card><p>between</p><x-card><b>Two</b></x-card><bad>x</bad>",
                            out, sizeof(out), &defined, &expanded);
    TEST_ASSERT_EQUAL(1, defined);
    TEST_ASSERT_EQUAL(2, expanded);
    TEST_ASSERT_EQUAL_STRING("<div class=\"card\">Hello</div><p>between</p><div class=\"card\"><b>Two</b></div><bad>x</bad>", out);
}

/* US-044 */
static void test_a11y_audit(void) {
    webrt_a11y_t r;
    int issues = webrt_a11y_audit("<html><h1>T</h1><h3>skip</h3><img src=a.png><img src=b alt=\"\">"
                                  "<label for=\"n\">Name</label><input id=\"n\"><input id=\"m\">"
                                  "<label>Mail <input type=email></label><input type=hidden>"
                                  "<button></button><button aria-label=\"close\"></button><button>OK</button>"
                                  "<a href=/x><img src=i></a><a href=/y>y</a></html>", &r);
    TEST_ASSERT_EQUAL(1, r.lang_missing);
    TEST_ASSERT_EQUAL(1, r.heading_skips);
    TEST_ASSERT_EQUAL(3, r.images);
    TEST_ASSERT_EQUAL(2, r.img_no_alt);
    TEST_ASSERT_EQUAL(3, r.inputs);
    TEST_ASSERT_EQUAL(1, r.input_no_label);
    TEST_ASSERT_EQUAL(3, r.buttons);
    TEST_ASSERT_EQUAL(1, r.button_no_text);
    TEST_ASSERT_EQUAL(2, r.links);
    TEST_ASSERT_EQUAL(1, r.link_no_text);
    TEST_ASSERT_EQUAL(7, issues);
    TEST_ASSERT_EQUAL(0, webrt_a11y_audit("<html lang=\"fr\"><h1>a</h1><h2>b</h2><img src=x alt=\"x\"></html>", &r));
}

static void test_csp_enforce(void) {
    webrt_csp_t p;
    webrt_csp_parse(&p, "default-src 'self'; script-src https://cdn.mohhdy.local");
    TEST_ASSERT_EQUAL(1, webrt_csp_enforce(&p, "https://app.mohhdy.local", "script", "https://cdn.mohhdy.local/app.js"));
    TEST_ASSERT_EQUAL(0, webrt_csp_enforce(&p, "https://app.mohhdy.local", "script", "https://evil.org/app.js"));
}

static char g_test_audit_action[32];
static char g_test_audit_detail[64];
static void test_audit_cb(const char* action, const char* detail) {
    strncpy(g_test_audit_action, action ? action : "", sizeof(g_test_audit_action));
    strncpy(g_test_audit_detail, detail ? detail : "", sizeof(g_test_audit_detail));
}

static void test_csp_validate_display_list(void) {
    webrt_csp_t p;
    webrt_set_audit_callback(test_audit_cb);
    g_test_audit_action[0] = 0;
    g_test_audit_detail[0] = 0;
    webrt_csp_parse(&p, "default-src 'self'; img-src https://img.mohhdy.local; style-src 'self' 'unsafe-inline'; script-src https://cdn.mohhdy.local");

    /* Allowed resources */
    TEST_ASSERT_EQUAL(1, webrt_csp_validate_display_list_resource(&p, "https://app.mohhdy.local", "img", "https://img.mohhdy.local/pic.png"));
    TEST_ASSERT_EQUAL(1, webrt_csp_validate_display_list_resource(&p, "https://app.mohhdy.local", "style", "inline"));
    TEST_ASSERT_EQUAL(1, webrt_csp_validate_display_list_resource(&p, "https://app.mohhdy.local", "script", "https://cdn.mohhdy.local/app.js"));

    /* Blocked image resource -> triggers audit callback */
    TEST_ASSERT_EQUAL(0, webrt_csp_validate_display_list_resource(&p, "https://app.mohhdy.local", "img", "https://evil.org/badge.png"));
    TEST_ASSERT_EQUAL_STRING("csp_blocked", g_test_audit_action);
    TEST_ASSERT_EQUAL_STRING("img:https://evil.org/badge.png", g_test_audit_detail);

    /* Blocked script resource -> triggers audit callback */
    g_test_audit_action[0] = 0;
    TEST_ASSERT_EQUAL(0, webrt_csp_validate_display_list_resource(&p, "https://app.mohhdy.local", "script", "https://unauthorized.org/ad.js"));
    TEST_ASSERT_EQUAL_STRING("csp_blocked", g_test_audit_action);
    TEST_ASSERT_EQUAL_STRING("script:https://unauthorized.org/ad.js", g_test_audit_detail);
}

static void test_lot20_responsive_aspect_ratio_and_nested_vars(void) {
    webrt_viewport_t vp = {1920, 1080, 0};
    int defined = 0, unresolved = 0;

    /* Aspect ratio media queries */
    TEST_ASSERT_TRUE(webrt_mq_match("(aspect-ratio: 16/9)", &vp));
    TEST_ASSERT_TRUE(webrt_mq_match("( min-aspect-ratio: 4/3 )", &vp));
    TEST_ASSERT_TRUE(webrt_mq_match("(max-aspect-ratio: 21/9)", &vp));
    TEST_ASSERT_FALSE(webrt_mq_match("(max-aspect-ratio: 4/3)", &vp));

    /* Nested CSS variables and 32 WEBRT_VARS limit support */
    webrt_css_resolve_vars(":root{--primary:#0080ff;--accent:var(--primary);--bg:var(--accent);}"
                           "body{color:var(--bg);fill:var(--primary);}",
                           out, sizeof(out), &defined, &unresolved);
    TEST_ASSERT_EQUAL(3, defined);
    TEST_ASSERT_EQUAL(0, unresolved);
    TEST_ASSERT_EQUAL_STRING(":root{}body{color:#0080ff;fill:#0080ff;}", out);
}

static void test_lot23_webrt_csp_and_origin_audit(void) {
    webrt_csp_t p;
    webrt_set_audit_callback(test_audit_cb);
    g_test_audit_action[0] = 0;
    g_test_audit_detail[0] = 0;

    webrt_csp_parse(&p, "default-src 'self'; script-src https://cdn.mohhdy.local; img-src https://img.mohhdy.local; media-src https://media.mohhdy.local");

    /* Allowed resources */
    TEST_ASSERT_EQUAL(1, webrt_csp_enforce(&p, "https://app.mohhdy.local", "script", "https://cdn.mohhdy.local/app.js"));
    TEST_ASSERT_EQUAL(1, webrt_csp_validate_display_list_resource(&p, "https://app.mohhdy.local", "media", "https://media.mohhdy.local/stream.mp4"));

    /* Blocked resources & audit callback verification */
    TEST_ASSERT_EQUAL(0, webrt_csp_enforce(&p, "https://app.mohhdy.local", "media", "https://unauthorized.org/video.mp4"));
    TEST_ASSERT_EQUAL_STRING("csp_blocked", g_test_audit_action);
    TEST_ASSERT_EQUAL_STRING("media:https://unauthorized.org/video.mp4", g_test_audit_detail);

    /* Origin validation and encrypted audit tracing */
    TEST_ASSERT_EQUAL(1, webrt_site_storage_validate_origin("https://app.mohhdy.local", "https://app.mohhdy.local"));
    TEST_ASSERT_EQUAL(0, webrt_site_storage_validate_origin("https://app.mohhdy.local", "https://evil.org"));

    g_test_audit_action[0] = 0;
    TEST_ASSERT_EQUAL(0, webrt_enc_v1_audit_log("storage_set", "/etc/web_storage.dat", 20));
    TEST_ASSERT_EQUAL_STRING("storage_set", g_test_audit_action);
    TEST_ASSERT_EQUAL_STRING("/etc/web_storage.dat:ENC_V1", g_test_audit_detail);
}

static void test_lot24_webrt_csp_and_origin_audit(void) {
    webrt_csp_t p;
    webrt_set_audit_callback(test_audit_cb);
    g_test_audit_action[0] = 0;
    g_test_audit_detail[0] = 0;

    webrt_csp_parse(&p, "default-src 'self'; connect-src https://api.mohhdy.local; script-src https://js.mohhdy.local");

    TEST_ASSERT_EQUAL(1, webrt_csp_enforce(&p, "https://app.mohhdy.local", "connect", "https://api.mohhdy.local/v1"));
    TEST_ASSERT_EQUAL(0, webrt_csp_enforce(&p, "https://app.mohhdy.local", "connect", "https://unauthorized-api.org"));
    TEST_ASSERT_EQUAL_STRING("csp_blocked", g_test_audit_action);
    TEST_ASSERT_EQUAL_STRING("connect:https://unauthorized-api.org", g_test_audit_detail);

    TEST_ASSERT_EQUAL(1, webrt_site_storage_validate_origin("https://app.mohhdy.local", "self"));
    TEST_ASSERT_EQUAL(0, webrt_site_storage_validate_origin("https://app.mohhdy.local", "https://untrusted.com"));
}

int main(void) {
    unity_init();
    RUN_TEST(test_cache_freshness_revalidation_and_lru);
    RUN_TEST(test_csp_parse_and_sources);
    RUN_TEST(test_csp_enforce);
    RUN_TEST(test_csp_validate_display_list);
    RUN_TEST(test_sanitizer);
    RUN_TEST(test_sanitize_page);
    RUN_TEST(test_media_queries);
    RUN_TEST(test_flatten_media);
    RUN_TEST(test_custom_properties);
    RUN_TEST(test_components);
    RUN_TEST(test_a11y_audit);
    RUN_TEST(test_lot20_responsive_aspect_ratio_and_nested_vars);
    RUN_TEST(test_lot23_webrt_csp_and_origin_audit);
    RUN_TEST(test_lot24_webrt_csp_and_origin_audit);
    unity_print_results();
    unity_cleanup();
    return unity_stats.tests_failed == 0 ? 0 : 1;
}
