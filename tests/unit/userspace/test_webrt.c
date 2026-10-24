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

static void test_lot26_webrt_timers_keyusage_ocsp(void) {
    char eval_buf[128];
    uint8_t dummy_cert[16] = {0x30, 0x0E, 0x02, 0x01, 0x01, 0x00, 0x00, 0x00};
    char status_buf[32];

    /* DOM Timers & Event Loop Tick */
    int t1 = webrt_timer_schedule(50, 0, "alert('timeout')");
    int t2 = webrt_timer_schedule(100, 1, "console.log('interval')");
    TEST_ASSERT_TRUE(t1 > 0);
    TEST_ASSERT_TRUE(t2 > 0);

    /* At time 10ms, no timer fired */
    TEST_ASSERT_EQUAL(0, webrt_event_loop_tick(10, eval_buf, sizeof(eval_buf)));

    /* At time 60ms, timeout t1 fired */
    TEST_ASSERT_EQUAL(1, webrt_event_loop_tick(60, eval_buf, sizeof(eval_buf)));
    TEST_ASSERT_EQUAL_STRING("alert('timeout')", eval_buf);

    /* At time 110ms, interval t2 fired */
    TEST_ASSERT_EQUAL(1, webrt_event_loop_tick(110, eval_buf, sizeof(eval_buf)));
    TEST_ASSERT_EQUAL_STRING("console.log('interval')", eval_buf);

    /* X.509 KeyUsage & OCSP status */
    TEST_ASSERT_EQUAL(0, webrt_x509_verify_key_usage(dummy_cert, 16, WEBRT_KU_DIGITAL_SIGNATURE, WEBRT_EKU_SERVER_AUTH));
    TEST_ASSERT_EQUAL(0, webrt_ocsp_check_status("SN-123456", status_buf, sizeof(status_buf)));
    TEST_ASSERT_EQUAL_STRING("GOOD", status_buf);

    TEST_ASSERT_EQUAL(-1, webrt_ocsp_check_status("SN-REVOKED-999", status_buf, sizeof(status_buf)));
    TEST_ASSERT_EQUAL_STRING("REVOKED", status_buf);
}

static void test_lot25_webrt_tls_and_dom_events(void) {
    webrt_tls_session_t tls;
    webrt_dom_event_mgr_t dom;
    uint8_t dummy_cert[32];
    uint8_t tls_buf[64];
    char script_buf[128];
    for (int i = 0; i < 32; i++) dummy_cert[i] = (uint8_t)i;

    /* Ring 3 TLS handshake and certificate verification */
    webrt_tls_init(&tls, "secure.mohhdy.local");
    TEST_ASSERT_EQUAL(WEBRT_TLS_STATE_IDLE, tls.state);
    TEST_ASSERT_EQUAL(0, webrt_tls_verify_cert_chain(&tls, dummy_cert, 32, 2));
    TEST_ASSERT_EQUAL(1, tls.cert_valid);

    TEST_ASSERT_EQUAL(32, webrt_tls_handshake_step(&tls, 0, 0, tls_buf, sizeof(tls_buf)));
    TEST_ASSERT_EQUAL(WEBRT_TLS_STATE_CLIENT_HELLO, tls.state);
    TEST_ASSERT_EQUAL(0, webrt_tls_handshake_step(&tls, 0, 0, 0, 0));
    TEST_ASSERT_EQUAL(WEBRT_TLS_STATE_SERVER_HELLO, tls.state);
    TEST_ASSERT_EQUAL(0, webrt_tls_handshake_step(&tls, 0, 0, 0, 0));
    TEST_ASSERT_EQUAL(WEBRT_TLS_STATE_CERT_VERIFY, tls.state);
    TEST_ASSERT_EQUAL(0, webrt_tls_handshake_step(&tls, 0, 0, 0, 0));
    TEST_ASSERT_EQUAL(WEBRT_TLS_STATE_KEY_EXCHANGE, tls.state);

    /* DOM Event Manager & Callbacks */
    webrt_dom_event_init(&dom);
    TEST_ASSERT_EQUAL(0, webrt_dom_add_event_listener(&dom, "btn1", "click", "console.log('clicked')"));
    TEST_ASSERT_EQUAL(1, dom.count);
    TEST_ASSERT_EQUAL(22, webrt_dom_dispatch_event(&dom, "btn1", "click", script_buf, sizeof(script_buf)));
    TEST_ASSERT_EQUAL_STRING("console.log('clicked')", script_buf);
    TEST_ASSERT_EQUAL(0, webrt_dom_dispatch_event(&dom, "btn1", "submit", script_buf, sizeof(script_buf)));
}

static void test_microtask_cb(void *arg) {
    int *val = (int*)arg;
    if (val) *val += 10;
}

static void test_macrotask_cb(void *arg) {
    int *val = (int*)arg;
    if (val) *val += 1;
}

static void test_lot29_webrt_shadow_dom_event_loop(void) {
    webrt_shadow_init();
    webrt_shadow_root_t *root_open = 0;
    webrt_shadow_root_t *root_closed = 0;

    TEST_ASSERT_EQUAL(0, webrt_shadow_root_attach("x-card", WEBRT_SHADOW_MODE_OPEN, &root_open));
    TEST_ASSERT_NOT_NULL(root_open);
    TEST_ASSERT_EQUAL(0, webrt_shadow_root_add_node("x-card", "title", "h1", "Card Header"));

    TEST_ASSERT_EQUAL(0, webrt_shadow_root_attach("x-private", WEBRT_SHADOW_MODE_CLOSED, &root_closed));
    TEST_ASSERT_NOT_NULL(root_closed);

    webrt_shadow_root_t *get_root = 0;
    TEST_ASSERT_EQUAL(0, webrt_shadow_root_get("x-card", 0, &get_root));
    TEST_ASSERT_NOT_NULL(get_root);

    TEST_ASSERT_EQUAL(-1, webrt_shadow_root_get("x-private", 0, &get_root));
    TEST_ASSERT_EQUAL(0, webrt_shadow_root_get("x-private", 1, &get_root));

    webrt_dom_event_mgr_t mgr;
    webrt_dom_event_init(&mgr);
    webrt_dom_add_event_listener(&mgr, "btn-submit", "click", "submitForm()");

    char path_buf[256];
    TEST_ASSERT_EQUAL(0, webrt_dom_dispatch_event_path(&mgr, "btn-submit", "click", 1, path_buf, sizeof(path_buf)));
    TEST_ASSERT_EQUAL_STRING("CAP:document>CAP:body>TARGET:btn-submit>BUBBLE:body>BUBBLE:document", path_buf);

    int state_counter = 0;
    TEST_ASSERT_EQUAL(0, webrt_queue_macrotask(test_macrotask_cb, &state_counter, "macrotask_1"));
    TEST_ASSERT_EQUAL(0, webrt_queue_microtask(test_microtask_cb, &state_counter, "microtask_1"));

    char eval_out[128];
    int count = webrt_event_loop_tick(100, eval_out, sizeof(eval_out));
    TEST_ASSERT(count >= 2);
    TEST_ASSERT_EQUAL(11, state_counter);
}

static void test_lot28_webrt_vector_and_box_model(void) {
    webrt_box_model_t box;
    TEST_ASSERT_EQUAL(0, webrt_compute_box_model(100, 50, 10, 10, 10, 10, 2, 2, 2, 2, 5, 5, 5, 5, &box));
    TEST_ASSERT_EQUAL(134, box.total_width);
    TEST_ASSERT_EQUAL(84, box.total_height);

    char lines[256];
    int count = webrt_text_wrap("Hello Mohhdy OS Vector Engine", 80, 8, lines, 4, 64);
    TEST_ASSERT(count > 0);

    webrt_form_component_t comp;
    webrt_form_component_init(&comp, 1, WEBRT_FORM_INPUT, "username");
    webrt_form_component_set_focus(&comp, 1);
    TEST_ASSERT_EQUAL(1, comp.focused);

    webrt_vec_display_list_t dl;
    TEST_ASSERT(webrt_render_vector_dom(&box, 0x00FFFFFFU, 0x00000000U, &dl) > 0);
}

static void test_lot30_webrt_canvas2d_wasm_anim(void) {
    /* 1. Canvas 2D matrix transformation & draw path */
    webrt_canvas2d_t canvas;
    webrt_canvas2d_init(&canvas, 800, 600);
    webrt_canvas2d_transform(&canvas, 1 << 16, 0, 0, 1 << 16, 10, 20);
    TEST_ASSERT_EQUAL(0, webrt_canvas2d_draw_path(&canvas, WEBRT_CANVAS_PATH_LINE, 0, 0, 100, 100, 0xFF0000));
    TEST_ASSERT_EQUAL(1, canvas.command_count);
    TEST_ASSERT_EQUAL(10, canvas.commands[0].x1);
    TEST_ASSERT_EQUAL(20, canvas.commands[0].y1);

    /* 2. WebAssembly MVP bytecode VM execution */
    webrt_wasm_vm_t vm;
    webrt_wasm_vm_init(&vm);
    /* WASM Bytecode: i32.const 5, i32.const 10, i32.add, halt */
    uint8_t bc[] = { 0x41, 0x05, 0x41, 0x0A, 0x6A, 0x0F };
    TEST_ASSERT_EQUAL(0, webrt_wasm_exec_module(&vm, bc, sizeof(bc)));
    int32_t res = 0;
    TEST_ASSERT_EQUAL(0, webrt_wasm_stack_pop(&vm, &res));
    TEST_ASSERT_EQUAL(15, res);

    /* 3. Animation frame tick */
    int frame_id = webrt_request_animation_frame(42);
    TEST_ASSERT_TRUE(frame_id > 0);
    uint32_t cb = 0;
    TEST_ASSERT_EQUAL(1, webrt_render_frame_tick(1, &cb));
    TEST_ASSERT_EQUAL(42, cb);
}

static int32_t sample_host_fn(int32_t a, int32_t b) {
    return a * b;
}

static void test_lot31_webrt_webgl3d_as_jit_atomics(void) {
    /* 1. WebGL 3D Rasterizer */
    webrt_webgl3d_ctx_t gl_ctx;
    webrt_webgl3d_init(&gl_ctx);
    gl_ctx.vertex_count = 3;
    gl_ctx.face_count = 1;
    gl_ctx.faces[0].v1 = 0;
    gl_ctx.faces[0].v2 = 1;
    gl_ctx.faces[0].v3 = 2;
    gl_ctx.faces[0].color = 0x00FF00; /* Green */

    uint32_t fb[16 * 16];
    memset(fb, 0, sizeof(fb));
    TEST_ASSERT_EQUAL(1, webrt_webgl3d_render_mesh(&gl_ctx, fb, 16, 16));

    /* 2. AssemblyScript JIT VM */
    TEST_ASSERT_EQUAL(0, webrt_assemblyscript_register_host_fn(0x10, sample_host_fn));
    /* Bytecode: i32.const 6, i32.const 7, call_host 0x10 */
    uint8_t as_bc[] = { 0x01, 0x06, 0x00, 0x00, 0x00, 0x01, 0x07, 0x00, 0x00, 0x00, 0x03, 0x10 };
    int32_t as_res = 0;
    TEST_ASSERT_EQUAL(0, webrt_assemblyscript_exec(as_bc, sizeof(as_bc), &as_res));
    TEST_ASSERT_EQUAL(42, as_res);

    /* 3. Shared ArrayBuffer & Atomics */
    webrt_arraybuffer_t ab;
    memset(&ab, 0, sizeof(ab));
    TEST_ASSERT_EQUAL(0, webrt_arraybuffer_share(&ab, 256));
    TEST_ASSERT_EQUAL(1, ab.shared_ref_count);

    int32_t *atom_ptr = (int32_t *)(ab.data + 0);
    *atom_ptr = 100;

    TEST_ASSERT_EQUAL(0, webrt_atomics_wait_notify(&ab, 0, 100, 200));
    TEST_ASSERT_EQUAL(200, *atom_ptr);
    TEST_ASSERT_EQUAL(1, webrt_atomics_wait_notify(&ab, 0, 100, 300)); /* Mismatch */
}

static void test_lot32_webrt_gpu_simd_shared_memory(void) {
    /* Test GPU Rasterize Polygon */
    webrt_gpu_polygon_t poly;
    poly.count = 3;
    poly.color = 0xFF00FF00; /* Green */
    poly.pts[0].x = 10 << 16; poly.pts[0].y = 10 << 16; poly.pts[0].z = 0;
    poly.pts[1].x = 50 << 16; poly.pts[1].y = 10 << 16; poly.pts[1].z = 0;
    poly.pts[2].x = 10 << 16; poly.pts[2].y = 50 << 16; poly.pts[2].z = 0;

    uint32_t fb[100 * 100];
    for (int i = 0; i < 100 * 100; i++) fb[i] = 0;

    int drawn = webrt_gpu_rasterize_polygon(&poly, fb, 100, 100);
    TEST_ASSERT_TRUE(drawn > 0);

    /* Test GPU Frustum Clip */
    webrt_gpu_polygon_t clipped;
    int c_count = webrt_gpu_clip_frustum(&poly, &clipped);
    TEST_ASSERT_EQUAL(3, c_count);

    /* Test WASM SIMD execution */
    /* v128.const [1, 2, 3, 4] -> FD 0C ... */
    /* v128.const [10, 20, 30, 40] -> FD 0C ... */
    /* i32x4.add -> FD 6E */
    uint8_t simd_code[38];
    simd_code[0] = 0xFD; simd_code[1] = 0x0C;
    int32_t v1[4] = {1, 2, 3, 4};
    for (int i = 0; i < 16; i++) simd_code[2 + i] = ((uint8_t*)v1)[i];

    simd_code[18] = 0xFD; simd_code[19] = 0x0C;
    int32_t v2[4] = {10, 20, 30, 40};
    for (int i = 0; i < 16; i++) simd_code[20 + i] = ((uint8_t*)v2)[i];

    simd_code[36] = 0xFD; simd_code[37] = 0x6E;

    webrt_wasm_v128_t res;
    TEST_ASSERT_EQUAL(0, webrt_wasm_simd_exec(simd_code, sizeof(simd_code), &res));
    TEST_ASSERT_EQUAL(11, res.i32[0]);
    TEST_ASSERT_EQUAL(22, res.i32[1]);
    TEST_ASSERT_EQUAL(33, res.i32[2]);
    TEST_ASSERT_EQUAL(44, res.i32[3]);

    /* Test Shared Memory Futex Lock / Unlock */
    webrt_arraybuffer_t ab;
    ab.lock_state = 0;
    TEST_ASSERT_EQUAL(0, webrt_shared_memory_lock(&ab, 10));
    TEST_ASSERT_EQUAL(-1, webrt_shared_memory_lock(&ab, 10)); /* Already locked */
    TEST_ASSERT_EQUAL(0, webrt_shared_memory_unlock(&ab));
    TEST_ASSERT_EQUAL(0, webrt_shared_memory_lock(&ab, 10));
}

static void test_lot33_webrt_webgpu_compute_wasm_futex(void) {
    /* 1. WebGPU Compute Shader */
    webrt_webgpu_pipeline_t pipeline;
    webrt_webgpu_compute_init(&pipeline, 2, 5);
    TEST_ASSERT_EQUAL(1, pipeline.active);

    int32_t in_data[4] = { 10, 20, 30, 40 };
    int32_t out_data[4] = { 0 };

    TEST_ASSERT_EQUAL(0, webrt_webgpu_dispatch_compute(&pipeline, in_data, out_data, 4));
    TEST_ASSERT_EQUAL(1, pipeline.dispatch_count);
    TEST_ASSERT_EQUAL(25, out_data[0]); /* 10 * 2 + 5 */
    TEST_ASSERT_EQUAL(45, out_data[1]); /* 20 * 2 + 5 */

    /* 2. WASM Threads & Futex Engine */
    webrt_wasm_threads_ctx_t threads_ctx;
    webrt_wasm_threads_init(&threads_ctx, 4);
    TEST_ASSERT_EQUAL(4, threads_ctx.num_threads);
    TEST_ASSERT_EQUAL(1, threads_ctx.shared_memory_enabled);

    webrt_arraybuffer_t ab;
    ab.byte_length = 128;
    memset(ab.data, 0, sizeof(ab.data));

    int32_t *ptr = (int32_t *)(ab.data + 16);
    *ptr = 100;

    int32_t old_val = 0;
    TEST_ASSERT_EQUAL(0, webrt_wasm_atomic_op(&ab, 16, WEBRT_WASM_ATOMIC_ADD, 50, &old_val));
    TEST_ASSERT_EQUAL(100, old_val);
    TEST_ASSERT_EQUAL(150, *ptr);

    TEST_ASSERT_EQUAL(1, webrt_futex_wait(&ab, 16, 999, 100)); /* Mismatch */
    TEST_ASSERT_EQUAL(0, webrt_futex_wait(&ab, 16, 150, 100)); /* Match */
    TEST_ASSERT_EQUAL(2, webrt_futex_wake(&ab, 16, 2));
}

static void test_lot35_webrt_sw_offscreen_cache_simd_vec(void) {
    /* 1. Service Workers PWA Offscreen Cache Engine */
    webrt_sw_cache_mgr_t mgr;
    webrt_sw_cache_mgr_init(&mgr);

    TEST_ASSERT_EQUAL(0, webrt_service_worker_register(&mgr, "https://app.mohhdy.local/", "/sw.js"));
    TEST_ASSERT_EQUAL(1, mgr.worker_count);

    TEST_ASSERT_EQUAL(0, webrt_offscreen_cache_put(&mgr, "/api/data.json", "{\"status\":\"ok\"}", 15));
    TEST_ASSERT_EQUAL(1, mgr.cache_count);

    char body_buf[64];
    uint32_t body_len = 0;
    TEST_ASSERT_EQUAL(0, webrt_offscreen_cache_match(&mgr, "/api/data.json", body_buf, sizeof(body_buf), &body_len));
    TEST_ASSERT_EQUAL(15, body_len);
    TEST_ASSERT_EQUAL_STRING("{\"status\":\"ok\"}", body_buf);

    /* 2. WebAssembly SIMD Vector Acceleration Engine */
    float va[4] = { 1.0f, 2.0f, 3.0f, 4.0f };
    float vb[4] = { 10.0f, 20.0f, 30.0f, 40.0f };
    float vout[4] = { 0 };

    TEST_ASSERT_EQUAL(0, webrt_wasm_simd_vec_add_f32(va, vb, vout, 4));
    TEST_ASSERT_TRUE(vout[0] >= 10.9f && vout[0] <= 11.1f);
    TEST_ASSERT_TRUE(vout[1] >= 21.9f && vout[1] <= 22.1f);
    TEST_ASSERT_TRUE(vout[2] >= 32.9f && vout[2] <= 33.1f);
    TEST_ASSERT_TRUE(vout[3] >= 43.9f && vout[3] <= 44.1f);

    int32_t ia[4] = { 1, 2, 3, 4 };
    int32_t ib[4] = { 10, 20, 30, 40 };
    int32_t dot_out = 0;

    TEST_ASSERT_EQUAL(0, webrt_wasm_simd_vec_dot_i32(ia, ib, 4, &dot_out));
    TEST_ASSERT_EQUAL(300, dot_out); /* 10 + 40 + 90 + 160 = 300 */
}

static void test_lot37_service_worker_bg_sync(void) {
    webrt_bg_sync_queue_t q;
    webrt_bg_sync_init(&q);

    uint32_t task_id = 0;
    int res = webrt_bg_sync_register(&q, "sync-form", "data=123", 8, &task_id);
    TEST_ASSERT_EQUAL(0, res);
    TEST_ASSERT_EQUAL(1, task_id);
    TEST_ASSERT_EQUAL(1, q.count);
    TEST_ASSERT_EQUAL(0, q.tasks[0].status);

    int processed = 0;
    res = webrt_bg_sync_process(&q, &processed);
    TEST_ASSERT_EQUAL(0, res);
    TEST_ASSERT_EQUAL(1, processed);
    TEST_ASSERT_EQUAL(1, q.synced_count);
    TEST_ASSERT_EQUAL(1, q.tasks[0].status);
}

static void test_lot37_wasm_simd_tensor(void) {
    webrt_mat4x4_t a, b, out_mat;
    for (int i = 0; i < 4; i++) {
        for (int j = 0; j < 4; j++) {
            a.m[i][j] = (i == j) ? (1 << 16) : 0; /* Identity 16.16 */
            b.m[i][j] = (i + j + 1) * (1 << 16);  /* Scaled values */
        }
    }

    int res = webrt_wasm_simd_mat4_mul(&a, &b, &out_mat);
    TEST_ASSERT_EQUAL(0, res);
    TEST_ASSERT_EQUAL(1 * (1 << 16), out_mat.m[0][0]);
    TEST_ASSERT_EQUAL(2 * (1 << 16), out_mat.m[0][1]);

    webrt_tensor3d_t t1, t2;
    for (int i = 0; i < 2; i++) {
        for (int j = 0; j < 2; j++) {
            for (int k = 0; k < 2; k++) {
                t1.data[i][j][k] = 2 * (1 << 16);
                t2.data[i][j][k] = 3 * (1 << 16);
            }
        }
    }

    int32_t scalar = 0;
    res = webrt_wasm_simd_tensor3d_contract(&t1, &t2, &scalar);
    TEST_ASSERT_EQUAL(0, res);
    /* 8 elements * (2 * 3) = 48 (in 16.16 fp) */
    TEST_ASSERT_EQUAL(48 * (1 << 16), scalar);
}

static void test_lot38_sab_futex_and_wasm_simd_conv(void) {
    webrt_sab_futex_t futex;
    webrt_sab_futex_init(&futex, 1);
    TEST_ASSERT_EQUAL(1, futex.value);

    int res = webrt_sab_futex_wait(&futex, 1, 101);
    TEST_ASSERT_EQUAL(0, res);
    TEST_ASSERT_EQUAL(1, futex.waiter_count);

    int woken = 0;
    res = webrt_sab_futex_wake(&futex, 1, &woken);
    TEST_ASSERT_EQUAL(0, res);
    TEST_ASSERT_EQUAL(1, woken);
    TEST_ASSERT_EQUAL(0, futex.waiter_count);

    webrt_mat8x8_t m1, m2, m_out;
    for (int r = 0; r < 8; r++) {
        for (int c = 0; c < 8; c++) {
            m1.m[r][c] = (r == c) ? (1 << 16) : 0;
            m2.m[r][c] = (r + c + 1) * (1 << 16);
        }
    }
    res = webrt_wasm_simd_mat8_mul(&m1, &m2, &m_out);
    TEST_ASSERT_EQUAL(0, res);
    TEST_ASSERT_EQUAL(1 * (1 << 16), m_out.m[0][0]);
    TEST_ASSERT_EQUAL(2 * (1 << 16), m_out.m[0][1]);

    webrt_tensor_conv_t conv;
    for (int r = 0; r < 16; r++) {
        for (int c = 0; c < 16; c++) {
            conv.input[r][c] = (1 << 16);
        }
    }
    for (int kr = 0; kr < 3; kr++) {
        for (int kc = 0; kc < 3; kc++) {
            conv.kernel[kr][kc] = (1 << 16);
        }
    }
    res = webrt_wasm_simd_tensor_conv2d(&conv);
    TEST_ASSERT_EQUAL(0, res);
    TEST_ASSERT_EQUAL(9 * (1 << 16), conv.output[0][0]);
    TEST_ASSERT_EQUAL(14 * 14 * 9, conv.conv_ops_count);
}

static void test_lot39_wasm_simd256_and_webgpu_compute(void) {
    /* WASM SIMD 256-bit Vector Operations */
    int32_t raw_a[8] = {1, 2, 3, 4, 5, 6, 7, 8};
    int32_t raw_b[8] = {2, 2, 2, 2, 2, 2, 2, 2};
    webrt_v256_t va, vb, vc, vout;
    webrt_simd256_load(&va, raw_a);
    webrt_simd256_load(&vb, raw_b);

    int64_t dot = 0;
    int res = webrt_wasm_simd256_dot(&va, &vb, &dot);
    TEST_ASSERT_EQUAL(0, res);
    TEST_ASSERT_EQUAL(72, dot); /* (1+2+3+4+5+6+7+8)*2 = 36*2 = 72 */

    /* FMA */
    int32_t raw_c[8] = {10, 10, 10, 10, 10, 10, 10, 10};
    webrt_simd256_load(&vc, raw_c);
    res = webrt_wasm_simd256_fma(&va, &vb, &vc, &vout);
    TEST_ASSERT_EQUAL(0, res);

    /* ReLU and GELU */
    int32_t raw_mix[8] = {-131072, -65536, 0, 65536, 131072, 196608, -196608, -262144};
    webrt_v256_t vmix, vrelu, vgelu;
    webrt_simd256_load(&vmix, raw_mix);

    res = webrt_wasm_simd256_relu(&vmix, &vrelu);
    TEST_ASSERT_EQUAL(0, res);
    TEST_ASSERT_EQUAL(0, vrelu.val[0]);
    TEST_ASSERT_EQUAL(65536, vrelu.val[3]);

    res = webrt_wasm_simd256_gelu(&vmix, &vgelu);
    TEST_ASSERT_EQUAL(0, res);
    TEST_ASSERT_EQUAL(0, vgelu.val[6]); /* x <= -3 */
    TEST_ASSERT_EQUAL(196608, vgelu.val[5]); /* x >= +3 */

    /* WebGPU Compute Pipeline */
    webrt_webgpu_compute_pass_t pass;
    webrt_webgpu_compute_pass_init(&pass, 42);
    TEST_ASSERT_EQUAL(1, pass.state);

    webrt_webgpu_bind_group_t bg;
    bg.group_id = 0;
    bg.entries[0].binding_id = 1;
    bg.entries[0].buffer = raw_a;
    bg.entries[0].size = sizeof(raw_a);
    bg.entry_count = 1;

    res = webrt_webgpu_bind_group_set(&pass, 0, &bg);
    TEST_ASSERT_EQUAL(0, res);

    res = webrt_webgpu_dispatch_workgroups(&pass, 4, 4, 1);
    TEST_ASSERT_EQUAL(0, res);
    TEST_ASSERT_EQUAL(2, pass.state);

    int completed = 0;
    res = webrt_webgpu_compute_pass_finish(&pass, &completed);
    TEST_ASSERT_EQUAL(0, res);
    TEST_ASSERT_EQUAL(16, completed);
    TEST_ASSERT_EQUAL(3, pass.state);
}

static void test_lot40_simd512_and_webgpu_parallel_encoder(void) {
    int32_t data_a[16] = {1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16};
    int32_t data_b[16] = {2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2};
    webrt_v512_t va, vb, vc, vout;
    webrt_simd512_load(&va, data_a);
    webrt_simd512_load(&vb, data_b);

    int64_t dot = 0;
    int res = webrt_wasm_simd512_dot(&va, &vb, &dot);
    TEST_ASSERT_EQUAL(0, res);
    TEST_ASSERT_EQUAL(272, (int)dot);

    webrt_simd512_load(&vc, data_a);
    res = webrt_wasm_simd512_fma(&va, &vb, &vc, &vout);
    TEST_ASSERT_EQUAL(0, res);

    webrt_v512_t vsilu;
    res = webrt_wasm_simd512_silu(&va, &vsilu);
    TEST_ASSERT_EQUAL(0, res);

    webrt_v512_t vsoft;
    res = webrt_wasm_simd512_softmax(&va, &vsoft);
    TEST_ASSERT_EQUAL(0, res);

    webrt_webgpu_parallel_encoder_t enc;
    webrt_webgpu_parallel_encoder_init(&enc, 101);
    uint32_t buf1 = 0, buf2 = 0;
    res = webrt_webgpu_parallel_create_command_buffer(&enc, &buf1);
    TEST_ASSERT_EQUAL(0, res);
    res = webrt_webgpu_parallel_create_command_buffer(&enc, &buf2);
    TEST_ASSERT_EQUAL(0, res);

    res = webrt_webgpu_command_buffer_encode_dispatch(&enc, buf1, 1, 4, 1, 1);
    TEST_ASSERT_EQUAL(0, res);
    res = webrt_webgpu_command_buffer_encode_dispatch(&enc, buf2, 2, 8, 2, 1);
    TEST_ASSERT_EQUAL(0, res);

    int total_cmds = 0;
    res = webrt_webgpu_parallel_encoder_finish(&enc, &total_cmds);
    TEST_ASSERT_EQUAL(0, res);
    TEST_ASSERT_EQUAL(2, total_cmds);
}

static void test_lot41_wasm_simd_mha_and_webgpu_multi_queue(void) {
    webrt_mha_kernel_t kernel;
    webrt_mha_init(&kernel, 4, 16);
    TEST_ASSERT_EQUAL(4, kernel.num_heads);
    TEST_ASSERT_EQUAL(16, kernel.head_dim);

    int32_t in_vec[64];
    int32_t q_vec[64], k_vec[64], v_vec[64], out_vec[64];
    for (int i = 0; i < 64; i++) in_vec[i] = i;

    /* scale = 1/sqrt(16) = 0.25 in 16.16 */
    TEST_ASSERT_EQUAL(16384, kernel.scale_q16);

    /* Identity projection (no learned weights): Q = K = V = input. */
    TEST_ASSERT_EQUAL(0, webrt_mha_qkv_proj(&kernel, in_vec, q_vec, k_vec, v_vec, 1));
    TEST_ASSERT_EQUAL(0, q_vec[0]);
    TEST_ASSERT_EQUAL(0, k_vec[0]);
    TEST_ASSERT_EQUAL(63, v_vec[63]);

    /* Softmax: equal logits split evenly; logits 0 and -ln(2) give 2/3, 1/3. */
    int32_t logits[4] = {10, 10, 10, 10};
    int32_t probs[4] = {0};
    TEST_ASSERT_EQUAL(0, webrt_mha_softmax_fast_q16(logits, probs, 4));
    for (int i = 0; i < 4; i++) TEST_ASSERT(probs[i] >= 16380 && probs[i] <= 16388);
    int32_t l2[2] = {0, -45426}; /* -ln(2) in 16.16 */
    int32_t p2[2] = {0, 0};
    TEST_ASSERT_EQUAL(0, webrt_mha_softmax_fast_q16(l2, p2, 2));
    TEST_ASSERT(p2[0] > 43600 && p2[0] < 43800);   /* 2/3 = 43690 */
    TEST_ASSERT(p2[1] > 21750 && p2[1] < 21950);   /* 1/3 = 21845 */
    int32_t l3[3] = {0, -2 * 65536, -100 * 65536};
    int32_t p3[3] = {0, 0, 0};
    TEST_ASSERT_EQUAL(0, webrt_mha_softmax_fast_q16(l3, p3, 3));
    TEST_ASSERT(p3[0] > 57600 && p3[0] < 57900);   /* 1/(1+e^-2) = 0.8808 */
    TEST_ASSERT_EQUAL(0, p3[2]);

    TEST_ASSERT_EQUAL(0, webrt_mha_compute_attention(&kernel, q_vec, k_vec, v_vec, out_vec, 1));
    TEST_ASSERT_EQUAL(1, kernel.total_attentions_computed);
    /* One position: the only weight is 1, so out = v. */
    for (int i = 0; i < 64; i++) TEST_ASSERT(out_vec[i] >= v_vec[i] - 1 && out_vec[i] <= v_vec[i]);

    /* Two positions, one head of dim 4. A query aligned with key 1 must
     * attend mostly to v1; a zero query must average v0 and v1. */
    webrt_mha_kernel_t k1;
    webrt_mha_init(&k1, 1, 4);
    TEST_ASSERT_EQUAL(32768, k1.scale_q16); /* 1/sqrt(4) */
    int32_t Q[8] = {0, 0, 0, 0,  0, 8 << 16, 0, 0};
    int32_t K[8] = {8 << 16, 0, 0, 0,  0, 8 << 16, 0, 0};
    int32_t V[8] = {100 << 16, 0, 0, 0,  0, 200 << 16, 0, 0};
    int32_t O[8];
    TEST_ASSERT_EQUAL(0, webrt_mha_compute_attention(&k1, Q, K, V, O, 2));
    /* Query 0 is zero: equal weights, out = (v0 + v1) / 2. */
    TEST_ASSERT(O[0] > (49 << 16) && O[0] < (51 << 16));
    TEST_ASSERT(O[1] > (99 << 16) && O[1] < (101 << 16));
    /* Query 1 . key 1 = 64, scaled 32: weight on v1 is ~1. */
    TEST_ASSERT(O[5] > (199 << 16));
    TEST_ASSERT(O[4] < (1 << 16));
    /* Bounds are enforced. */
    TEST_ASSERT_EQUAL(-1, webrt_mha_compute_attention(&k1, Q, K, V, O, WEBRT_MHA_SEQ_MAX + 1));

    webrt_webgpu_multi_queue_t mq;
    webrt_webgpu_multi_queue_init(&mq);
    TEST_ASSERT_EQUAL(0, webrt_webgpu_multi_queue_submit(&mq, 0, 101, 50));
    TEST_ASSERT_EQUAL(0, webrt_webgpu_multi_queue_submit(&mq, 1, 202, 100));
    TEST_ASSERT_EQUAL(2, mq.total_submissions);

    int executed = 0;
    TEST_ASSERT_EQUAL(0, webrt_webgpu_multi_queue_process(&mq, &executed));
    TEST_ASSERT_EQUAL(2, executed);
    TEST_ASSERT_EQUAL(2, mq.total_processed);
}

static void test_lot42_wasm_simd_kv_compressor_and_webgpu_async(void) {
    webrt_kv_compressor_t comp;
    webrt_kv_compressor_init(&comp, 0); /* Q4_K */
    int32_t in_q16[WEBRT_KV_BLOCK_SIZE];
    for (int i = 0; i < WEBRT_KV_BLOCK_SIZE; i++) {
        in_q16[i] = (i % 16) << 16;
    }
    uint8_t compressed[32];
    uint32_t comp_sz = 0;
    TEST_ASSERT_EQUAL(0, webrt_kv_compress_block(&comp, in_q16, compressed, &comp_sz));
    TEST_ASSERT_EQUAL(32, comp_sz);
    TEST_ASSERT_EQUAL(1, comp.total_blocks_compressed);

    int32_t out_q16[WEBRT_KV_BLOCK_SIZE];
    TEST_ASSERT_EQUAL(0, webrt_kv_decompress_block(&comp, compressed, out_q16, comp_sz));
    TEST_ASSERT_EQUAL(1, comp.total_blocks_decompressed);

    webrt_webgpu_async_engine_t async_eng;
    webrt_webgpu_async_engine_init(&async_eng);
    TEST_ASSERT_EQUAL(0, webrt_webgpu_async_compile_jit(&async_eng, 1001, 8, 8, 1));
    TEST_ASSERT_EQUAL(1, async_eng.shader_count);

    TEST_ASSERT_EQUAL(0, webrt_webgpu_async_dispatch_barrier(&async_eng, 1001, 16, 16, 1));
    TEST_ASSERT_EQUAL(1, async_eng.total_async_dispatches);
    TEST_ASSERT_EQUAL(1, async_eng.memory_barriers_passed);
}

static void test_lot43_wasm_simd_moe_router_and_webgpu_multi_pipeline(void) {
    webrt_moe_router_t router;
    webrt_moe_router_init(&router, 4, 2);
    TEST_ASSERT_EQUAL(4, router.num_experts);
    TEST_ASSERT_EQUAL(2, router.top_k);

    int32_t inputs_q16[4] = {1000 << 16, 2000 << 16, 3000 << 16, 4000 << 16};
    uint32_t exp_indices[8];
    int32_t weights[8];
    TEST_ASSERT_EQUAL(0, webrt_moe_route_tokens(&router, inputs_q16, 4, exp_indices, weights));
    TEST_ASSERT_EQUAL(4, router.total_tokens_routed);

    webrt_webgpu_multi_pipeline_t mp;
    webrt_webgpu_multi_pipeline_init(&mp);
    TEST_ASSERT_EQUAL(0, mp.pipeline_count);

    TEST_ASSERT_EQUAL(0, webrt_webgpu_multi_pipeline_create(&mp, 1, 10, 100));
    TEST_ASSERT_EQUAL(0, webrt_webgpu_multi_pipeline_create(&mp, 2, 11, 101));
    TEST_ASSERT_EQUAL(2, mp.pipeline_count);

    uint32_t pids[2] = {1, 2};
    int exec = 0;
    TEST_ASSERT_EQUAL(0, webrt_webgpu_multi_pipeline_dispatch_parallel(&mp, pids, 2, &exec));
    TEST_ASSERT_EQUAL(2, exec);
    TEST_ASSERT_EQUAL(1, mp.barrier_count);
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
    RUN_TEST(test_lot25_webrt_tls_and_dom_events);
    RUN_TEST(test_lot26_webrt_timers_keyusage_ocsp);
    RUN_TEST(test_lot29_webrt_shadow_dom_event_loop);
    RUN_TEST(test_lot28_webrt_vector_and_box_model);
    RUN_TEST(test_lot30_webrt_canvas2d_wasm_anim);
    RUN_TEST(test_lot31_webrt_webgl3d_as_jit_atomics);
    RUN_TEST(test_lot32_webrt_gpu_simd_shared_memory);
    RUN_TEST(test_lot33_webrt_webgpu_compute_wasm_futex);
    RUN_TEST(test_lot35_webrt_sw_offscreen_cache_simd_vec);
    RUN_TEST(test_lot37_service_worker_bg_sync);
    RUN_TEST(test_lot37_wasm_simd_tensor);
    RUN_TEST(test_lot38_sab_futex_and_wasm_simd_conv);
    RUN_TEST(test_lot39_wasm_simd256_and_webgpu_compute);
    RUN_TEST(test_lot40_simd512_and_webgpu_parallel_encoder);
    RUN_TEST(test_lot41_wasm_simd_mha_and_webgpu_multi_queue);
    RUN_TEST(test_lot42_wasm_simd_kv_compressor_and_webgpu_async);
    RUN_TEST(test_lot43_wasm_simd_moe_router_and_webgpu_multi_pipeline);

    unity_print_results();
    unity_cleanup();
    return unity_stats.tests_failed == 0 ? 0 : 1;
}
