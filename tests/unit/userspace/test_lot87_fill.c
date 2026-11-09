/* Lot 87: the autofill list. A key (host, field name) keeps its newest
 * value and two older ones; the editor and the desktop offer them newest
 * first; the session file keeps the order. */
#include "../../framework/unity.h"
#include "wf_autofill.h"
#include "wf_formfill.h"
#include "wf_http.h"
#include <string.h>

static wf_autofill_t g_a, g_b;

static void get_at(wf_autofill_t *a, const char *h, const char *n, int idx, const char *want) {
    char v[WF_AF_VALUE];
    if (!want) { TEST_ASSERT_EQUAL(-1, wf_autofill_get_at(a, h, n, idx, v, (int)sizeof(v))); return; }
    TEST_ASSERT_EQUAL(0, wf_autofill_get_at(a, h, n, idx, v, (int)sizeof(v)));
    TEST_ASSERT_EQUAL_STRING(want, v);
}

static void test_history_order(void) {
    wf_autofill_init(&g_a);
    TEST_ASSERT_EQUAL(0, wf_autofill_count(&g_a, "ex.am", "q"));
    TEST_ASSERT_EQUAL(0, wf_autofill_put(&g_a, "ex.am", "q", "one", "text"));
    TEST_ASSERT_EQUAL(1, wf_autofill_count(&g_a, "ex.am", "q"));
    TEST_ASSERT_EQUAL(0, wf_autofill_put(&g_a, "ex.am", "q", "two", "text"));
    TEST_ASSERT_EQUAL(0, wf_autofill_put(&g_a, "ex.am", "q", "three", "text"));
    /* still one key */
    TEST_ASSERT_EQUAL(1, g_a.n);
    TEST_ASSERT_EQUAL(3, wf_autofill_count(&g_a, "EX.am", "q"));
    get_at(&g_a, "ex.am", "q", 0, "three");
    get_at(&g_a, "ex.am", "q", 1, "two");
    get_at(&g_a, "ex.am", "q", 2, "one");
    get_at(&g_a, "ex.am", "q", 3, 0);
    get_at(&g_a, "ex.am", "q", -1, 0);
    get_at(&g_a, "ex.am", "nope", 0, 0);
    /* a fourth value pushes the oldest out */
    TEST_ASSERT_EQUAL(0, wf_autofill_put(&g_a, "ex.am", "q", "four", "text"));
    TEST_ASSERT_EQUAL(3, wf_autofill_count(&g_a, "ex.am", "q"));
    get_at(&g_a, "ex.am", "q", 0, "four");
    get_at(&g_a, "ex.am", "q", 1, "three");
    get_at(&g_a, "ex.am", "q", 2, "two");
}

static void test_history_moves_to_front(void) {
    wf_autofill_init(&g_a);
    (void)wf_autofill_put(&g_a, "ex.am", "q", "a", "text");
    (void)wf_autofill_put(&g_a, "ex.am", "q", "b", "text");
    (void)wf_autofill_put(&g_a, "ex.am", "q", "c", "text");
    /* the oldest typed again moves to the front, nothing is lost or doubled */
    TEST_ASSERT_EQUAL(0, wf_autofill_put(&g_a, "ex.am", "q", "a", "text"));
    TEST_ASSERT_EQUAL(3, wf_autofill_count(&g_a, "ex.am", "q"));
    get_at(&g_a, "ex.am", "q", 0, "a");
    get_at(&g_a, "ex.am", "q", 1, "c");
    get_at(&g_a, "ex.am", "q", 2, "b");
    /* the middle one */
    TEST_ASSERT_EQUAL(0, wf_autofill_put(&g_a, "ex.am", "q", "c", "text"));
    get_at(&g_a, "ex.am", "q", 0, "c");
    get_at(&g_a, "ex.am", "q", 1, "a");
    get_at(&g_a, "ex.am", "q", 2, "b");
    /* the current one again changes nothing */
    TEST_ASSERT_EQUAL(0, wf_autofill_put(&g_a, "ex.am", "q", "c", "text"));
    get_at(&g_a, "ex.am", "q", 0, "c");
    get_at(&g_a, "ex.am", "q", 1, "a");
    get_at(&g_a, "ex.am", "q", 2, "b");
    /* secrets still refused, and they do not touch the list */
    TEST_ASSERT_EQUAL(-1, wf_autofill_put(&g_a, "ex.am", "q", "pw", "password"));
    TEST_ASSERT_EQUAL(3, wf_autofill_count(&g_a, "ex.am", "q"));
    get_at(&g_a, "ex.am", "q", 0, "c");
}

static void test_save_load_keeps_order(void) {
    static char blob[4096], again[4096];
    int n, m;
    wf_autofill_init(&g_a);
    (void)wf_autofill_put(&g_a, "ex.am", "q", "old", "text");
    (void)wf_autofill_put(&g_a, "ex.am", "q", "mid", "text");
    (void)wf_autofill_put(&g_a, "ex.am", "q", "new", "text");
    (void)wf_autofill_put(&g_a, "ex.am", "user", "ada", "text");
    (void)wf_autofill_put(&g_a, "other.org", "q", "x", "search");
    n = wf_autofill_save(&g_a, blob, (int)sizeof(blob));
    TEST_ASSERT(n > 0);
    /* oldest first per key, so a load replays the same history */
    TEST_ASSERT(strstr(blob, "ex.am\tq\told\nex.am\tq\tmid\nex.am\tq\tnew\n") != 0);
    TEST_ASSERT_EQUAL(0, wf_autofill_load(&g_b, blob, n));
    TEST_ASSERT_EQUAL(3, g_b.n);
    TEST_ASSERT_EQUAL(3, wf_autofill_count(&g_b, "ex.am", "q"));
    get_at(&g_b, "ex.am", "q", 0, "new");
    get_at(&g_b, "ex.am", "q", 1, "mid");
    get_at(&g_b, "ex.am", "q", 2, "old");
    get_at(&g_b, "ex.am", "user", 0, "ada");
    TEST_ASSERT_EQUAL(1, wf_autofill_count(&g_b, "ex.am", "user"));
    m = wf_autofill_save(&g_b, again, (int)sizeof(again));
    TEST_ASSERT_EQUAL(n, m);
    TEST_ASSERT_EQUAL_STRING(blob, again);
    /* a short buffer fails cleanly */
    TEST_ASSERT_EQUAL(-1, wf_autofill_save(&g_a, blob, 40));
    /* the listing shows how many values a key holds, never the values */
    TEST_ASSERT(wf_autofill_text(&g_a, blob, (int)sizeof(blob)) > 0);
    TEST_ASSERT(strstr(blob, "autofill 3\n") == blob);
    TEST_ASSERT(strstr(blob, "ex.am q values 3\n") != 0);
    TEST_ASSERT(strstr(blob, "ex.am user\n") != 0);
    TEST_ASSERT(strstr(blob, "new") == 0);
    TEST_ASSERT(strstr(blob, "ada") == 0);
}

static int add_field(wf_form_page_t *p, const wf_url_t *base, const char *type, const char *name) {
    return wf_form_add(p, base, type, name, "", "", 0, 0);
}

static void test_formfill_offers_newest_first(void) {
    static wf_form_page_t p;
    wf_fill_offer_t o[WF_FILL_OFFERS];
    wf_url_t base;
    int q, pw, n;
    TEST_ASSERT_EQUAL(0, wf_url_parse("https://ex.am/find", &base));
    wf_form_reset(&p);
    TEST_ASSERT(wf_form_begin(&p, &base, "/find", "get", "") >= 0);
    q = add_field(&p, &base, "text", "q");
    pw = add_field(&p, &base, "password", "q");
    wf_form_end(&p);
    TEST_ASSERT(q >= 0);
    TEST_ASSERT(pw >= 0);
    wf_autofill_init(&g_a);
    /* a forgotten host leaves a hole before the key we want */
    (void)wf_autofill_put(&g_a, "gone.org", "q", "zzz", "text");
    (void)wf_autofill_put(&g_a, "ex.am", "q", "first", "text");
    (void)wf_autofill_put(&g_a, "ex.am", "q", "second", "text");
    TEST_ASSERT_EQUAL(1, wf_autofill_forget(&g_a, "gone.org"));
    TEST_ASSERT_EQUAL(1, g_a.n);
    n = wf_formfill_offers(&g_a, &p, q, "https://ex.am/find", o, WF_FILL_OFFERS);
    TEST_ASSERT_EQUAL(2, n);
    TEST_ASSERT_EQUAL_STRING("second", o[0].value);
    TEST_ASSERT_EQUAL_STRING("first", o[1].value);
    TEST_ASSERT_EQUAL(o[0].ent, o[1].ent);
    /* the cap is honoured */
    TEST_ASSERT_EQUAL(1, wf_formfill_offers(&g_a, &p, q, "https://ex.am/find", o, 1));
    /* a password field is never offered anything */
    TEST_ASSERT_EQUAL(0, wf_formfill_offers(&g_a, &p, pw, "https://ex.am/find", o, WF_FILL_OFFERS));
    /* the editor key takes the newest; apply takes any of them */
    TEST_ASSERT_EQUAL(0, wf_formfill_key(&g_a, &p, q, "https://ex.am/find"));
    TEST_ASSERT_EQUAL_STRING("second", p.field[q].value);
    n = wf_formfill_offers(&g_a, &p, q, "https://ex.am/find", o, WF_FILL_OFFERS);
    TEST_ASSERT_EQUAL(0, wf_formfill_apply(&p, q, &o[1]));
    TEST_ASSERT_EQUAL_STRING("first", p.field[q].value);
    /* another host: nothing */
    TEST_ASSERT_EQUAL(0, wf_formfill_offers(&g_a, &p, q, "https://ex.org/find", o, WF_FILL_OFFERS));
}

static void test_remember_builds_history(void) {
    static wf_form_page_t p;
    wf_url_t base;
    int q;
    TEST_ASSERT_EQUAL(0, wf_url_parse("https://ex.am/find", &base));
    wf_form_reset(&p);
    TEST_ASSERT(wf_form_begin(&p, &base, "/find", "get", "") >= 0);
    q = add_field(&p, &base, "search", "q");
    wf_form_end(&p);
    wf_autofill_init(&g_a);
    TEST_ASSERT_EQUAL(0, wf_form_set(&p, q, "kernel"));
    TEST_ASSERT_EQUAL(1, wf_formfill_remember(&g_a, &p, -1, "https://ex.am/find"));
    TEST_ASSERT_EQUAL(0, wf_form_set(&p, q, "qemu"));
    TEST_ASSERT_EQUAL(1, wf_formfill_remember(&g_a, &p, -1, "https://ex.am/find"));
    TEST_ASSERT_EQUAL(2, wf_autofill_count(&g_a, "ex.am", "q"));
    get_at(&g_a, "ex.am", "q", 0, "qemu");
    get_at(&g_a, "ex.am", "q", 1, "kernel");
}

int main(void) {
    unity_init();
    RUN_TEST(test_history_order);
    RUN_TEST(test_history_moves_to_front);
    RUN_TEST(test_save_load_keeps_order);
    RUN_TEST(test_formfill_offers_newest_first);
    RUN_TEST(test_remember_builds_history);
    unity_print_results();
    unity_cleanup();
    return unity_stats.tests_failed == 0 ? 0 : 1;
}
