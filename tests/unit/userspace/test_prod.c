/* Phase 8 production toolkit (US-106..US-120). */
#include "../../framework/unity.h"
#include <string.h>
#include <stdio.h>
#include "../../../userspace/prod.h"

static prod_sample_t S(uint32_t t, uint32_t used, uint32_t busy) {
    prod_sample_t s; memset(&s, 0, sizeof(s)); s.tick = t; s.v[PM_MEM_USED] = used; s.v[PM_MEM_FREE] = 1000 - used; s.v[PM_BUSY] = busy; s.v[PM_PROCS] = 3; return s;
}

void test_metrics_ring_stats_and_prediction(void) {
    static prod_metrics_t m;
    prod_stat_t st;
    int32_t slope; uint32_t left;
    int i;
    prod_metrics_init(&m);
    TEST_ASSERT_EQUAL(-1, prod_metric_stat(&m, PM_BUSY, &st));
    for (i = 0; i < 70; i++) { prod_sample_t s = S((uint32_t)i * 100U, 200U + (uint32_t)i * 5U, (uint32_t)(i % 10)); prod_metrics_push(&m, &s); }
    TEST_ASSERT_EQUAL(PROD_SAMPLES, m.n);
    TEST_ASSERT_EQUAL(0, prod_metric_stat(&m, PM_MEM_USED, &st));
    TEST_ASSERT_EQUAL(230, (int)st.min); TEST_ASSERT_EQUAL(545, (int)st.max); TEST_ASSERT_EQUAL(545, (int)st.last);
    TEST_ASSERT_EQUAL(0, prod_predict(&m, PM_MEM_USED, 945, &slope, &left));
    TEST_ASSERT_EQUAL(50, slope);               /* 5 kib per 100 ticks */
    TEST_ASSERT_EQUAL(8000, (int)left);         /* 400 kib / 0.05 */
    TEST_ASSERT_EQUAL(0, prod_predict(&m, PM_MEM_FREE, 100, &slope, &left));
    TEST_ASSERT_EQUAL(-50, slope); TEST_ASSERT_EQUAL(7100, (int)left);
    TEST_ASSERT_EQUAL(0, prod_predict(&m, PM_MEM_USED, 100, &slope, &left));
    TEST_ASSERT_EQUAL(0, (int)left);            /* already past */
    TEST_ASSERT_EQUAL(PM_BUSY, prod_metric_id("busy"));
    TEST_ASSERT_EQUAL(-1, prod_metric_id("nope"));
}

void test_alerts_for_n_dedupe_and_hysteresis(void) {
    static prod_alerts_t a;
    prod_sample_t s;
    prod_alerts_init(&a);
    TEST_ASSERT_EQUAL(0, prod_alert_add(&a, "hot", PM_BUSY, 1, 80, 2));
    TEST_ASSERT_EQUAL(-2, prod_alert_add(&a, "hot", PM_BUSY, 1, 80, 2));
    TEST_ASSERT_EQUAL(-1, prod_alert_add(&a, "x", 99, 1, 1, 1));
    TEST_ASSERT_EQUAL(1, prod_alert_add(&a, "lowmem", PM_MEM_FREE, 0, 100, 1));
    s = S(1, 100, 90); TEST_ASSERT_EQUAL(0, prod_alerts_eval(&a, &s));  /* 1st breach */
    s = S(2, 100, 95); TEST_ASSERT_EQUAL(1, prod_alerts_eval(&a, &s));  /* fires */
    s = S(3, 100, 99); TEST_ASSERT_EQUAL(0, prod_alerts_eval(&a, &s));  /* deduped */
    s = S(4, 100, 10); TEST_ASSERT_EQUAL(0, prod_alerts_eval(&a, &s));  /* 1 clear */
    s = S(5, 100, 85); TEST_ASSERT_EQUAL(0, prod_alerts_eval(&a, &s));  /* flap: stays firing */
    s = S(6, 100, 10); prod_alerts_eval(&a, &s);
    s = S(7, 100, 10); TEST_ASSERT_EQUAL(1, prod_alerts_eval(&a, &s));  /* resolved */
    s = S(8, 950, 10); TEST_ASSERT_EQUAL(1, prod_alerts_eval(&a, &s));  /* lowmem free 50 < 100 */
    TEST_ASSERT_EQUAL(2, (int)a.fired_total); TEST_ASSERT_EQUAL(1, (int)a.resolved_total);
    TEST_ASSERT_EQUAL(2, (int)a.suppressed);
    TEST_ASSERT_EQUAL_STRING("FIRING hot v=95", a.ev[0].text);
    TEST_ASSERT_EQUAL_STRING("RESOLVED hot v=10", a.ev[1].text);
    TEST_ASSERT_EQUAL_STRING("FIRING lowmem v=50", a.ev[2].text);
    TEST_ASSERT_EQUAL(0, prod_alert_remove(&a, "hot"));
    TEST_ASSERT_EQUAL(-1, prod_alert_remove(&a, "hot"));
}

void test_log_analysis(void) {
    prod_log_report_t r;
    const char* log =
        "INFO boot ok\n"
        "WARN disk slow 120 ms\n"
        "ERROR timeout on req 17\n"
        "ERROR timeout on req 18\n"
        "info user login\n"
        "ERROR timeout on req 99\n"
        "something else\n"
        "WARN disk slow 300 ms\n"
        "panic: kernel oops 3\n"
        "\n";
    prod_log_analyze(log, &r);
    TEST_ASSERT_EQUAL(9, (int)r.lines);
    TEST_ASSERT_EQUAL(4, (int)r.errors); TEST_ASSERT_EQUAL(2, (int)r.warns); TEST_ASSERT_EQUAL(2, (int)r.infos); TEST_ASSERT_EQUAL(1, (int)r.other);
    TEST_ASSERT_EQUAL_STRING("ERROR timeout on req 17", r.first_error);
    TEST_ASSERT_EQUAL(4, (int)r.burst_max);
    TEST_ASSERT_EQUAL_STRING("ERROR timeout on req #", r.top[0].pattern); TEST_ASSERT_EQUAL(3, (int)r.top[0].count);
    TEST_ASSERT_EQUAL_STRING("WARN disk slow # ms", r.top[1].pattern); TEST_ASSERT_EQUAL(2, (int)r.top[1].count);
    TEST_ASSERT_EQUAL(3, r.ntop);
    prod_log_analyze("", &r); TEST_ASSERT_EQUAL(0, (int)r.lines);
}

void test_archive_verify_and_corruption(void) {
    static prod_archive_t a;
    int bad;
    static char big[PROD_ARCH_BYTES];
    prod_archive_init(&a, "b1", 5);
    TEST_ASSERT_EQUAL(0, prod_archive_add(&a, "/etc/app.conf", "mode=eco\n", 9));
    TEST_ASSERT_EQUAL(1, prod_archive_add(&a, "/etc/empty", "", 0));
    TEST_ASSERT_EQUAL(0, prod_archive_verify(&a, &bad)); TEST_ASSERT_EQUAL(-1, bad);
    TEST_ASSERT_EQUAL(-2, prod_archive_add(&a, "/big", big, sizeof(big)));
    a.data[3] ^= 1;
    TEST_ASSERT_EQUAL(1, prod_archive_verify(&a, &bad)); TEST_ASSERT_EQUAL(0, bad);
    TEST_ASSERT_EQUAL((int)0x811c9dc5, (int)prod_fnv("", 0));
}

static int fake_read(void* ctx, const char* path, char* buf, int cap) {
    (void)ctx; (void)cap;
    if (strcmp(path, "/app/a") == 0) { memcpy(buf, "AAA", 3); return 3; }
    if (strcmp(path, "/app/health") == 0) { memcpy(buf, "ok", 2); return 2; }
    return -1;
}
void test_manifest_check(void) {
    char why[64], m[128];
    sprintf(m, "/app/a %x\n/app/health %x\n", prod_fnv("AAA", 3), prod_fnv("ok", 2));
    TEST_ASSERT_EQUAL(2, prod_manifest_check(m, fake_read, 0, why, sizeof(why)));
    sprintf(m, "/app/a %x\n", prod_fnv("AAB", 3));
    TEST_ASSERT_EQUAL(-3, prod_manifest_check(m, fake_read, 0, why, sizeof(why))); TEST_ASSERT_EQUAL_STRING("checksum /app/a", why);
    TEST_ASSERT_EQUAL(-2, prod_manifest_check("/app/zz 1\n", fake_read, 0, why, sizeof(why))); TEST_ASSERT_EQUAL_STRING("missing /app/zz", why);
    TEST_ASSERT_EQUAL(-1, prod_manifest_check("/app/a zz\n", fake_read, 0, why, sizeof(why)));
    TEST_ASSERT_EQUAL(-1, prod_manifest_check("\n", fake_read, 0, why, sizeof(why)));
}

void test_scaler_cooldown_and_bounds(void) {
    prod_scaler_t s;
    prod_scaler_init(&s, 1, 3, 4000, 1000, 50);
    TEST_ASSERT_EQUAL(2, prod_scaler_step(&s, 10, 0));    /* 10 per worker > 4 */
    TEST_ASSERT_EQUAL(2, prod_scaler_step(&s, 30, 10));   /* cooldown */
    TEST_ASSERT_EQUAL(3, prod_scaler_step(&s, 30, 60));
    TEST_ASSERT_EQUAL(3, prod_scaler_step(&s, 90, 200));  /* max */
    TEST_ASSERT_EQUAL(2, prod_scaler_step(&s, 0, 300));
    TEST_ASSERT_EQUAL(1, prod_scaler_step(&s, 0, 400));
    TEST_ASSERT_EQUAL(1, prod_scaler_step(&s, 0, 500));   /* min */
    TEST_ASSERT_EQUAL(2, s.ups); TEST_ASSERT_EQUAL(2, s.downs);
}

void test_baseline_feedback_usage(void) {
    static prod_baseline_t b;
    static prod_feedback_t f;
    static prod_usage_t u;
    TEST_ASSERT_EQUAL(0, prod_baseline_add(&b, "/bin/x", 7));
    TEST_ASSERT_EQUAL(0, prod_baseline_cmp(&b, "/bin/x", 7));
    TEST_ASSERT_EQUAL(1, prod_baseline_cmp(&b, "/bin/x", 8));
    TEST_ASSERT_EQUAL(2, prod_baseline_cmp(&b, "/bin/y", 7));
    TEST_ASSERT_EQUAL(-1, prod_feedback_add(&f, 6, "x"));
    prod_feedback_add(&f, 5, "great"); prod_feedback_add(&f, 3, "ok"); prod_feedback_add(&f, 4, "fine");
    TEST_ASSERT_EQUAL(3, (int)f.total); TEST_ASSERT_EQUAL(12, (int)f.sum); TEST_ASSERT_EQUAL(1, (int)f.hist[5]);
    prod_usage_hit(&u, "ls"); prod_usage_hit(&u, "ps"); prod_usage_hit(&u, "ps"); prod_usage_hit(&u, "cat"); prod_usage_hit(&u, "ps"); prod_usage_hit(&u, "cat");
    prod_usage_sort(&u);
    TEST_ASSERT_EQUAL_STRING("ps", u.u[0].name); TEST_ASSERT_EQUAL(3, (int)u.u[0].count);
    TEST_ASSERT_EQUAL_STRING("cat", u.u[1].name); TEST_ASSERT_EQUAL_STRING("ls", u.u[2].name);
    TEST_ASSERT_EQUAL(6, (int)u.total);
}

int main(void) {
    unity_init();
    RUN_TEST(test_metrics_ring_stats_and_prediction);
    RUN_TEST(test_alerts_for_n_dedupe_and_hysteresis);
    RUN_TEST(test_log_analysis);
    RUN_TEST(test_archive_verify_and_corruption);
    RUN_TEST(test_manifest_check);
    RUN_TEST(test_scaler_cooldown_and_bounds);
    RUN_TEST(test_baseline_feedback_usage);
    unity_print_results();
    unity_cleanup();
    return (unity_stats.tests_failed == 0) ? 0 : 1;
}
