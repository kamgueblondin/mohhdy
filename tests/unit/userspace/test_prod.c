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


void test_state_save_load(void) {
    static prod_metrics_t m, m2; static prod_alerts_t a, a2; static prod_archive_t ar[2], ar2[2]; static char dir[2][64], dir2[2][64];
    static prod_feedback_t fb, fb2; static uint8_t blob[65536];
    prod_state_ref_t r = {&m, &a, ar, dir, 2, &fb}, r2 = {&m2, &a2, ar2, dir2, 2, &fb2};
    prod_stat_t st; int i, n, bad;
    prod_metrics_init(&m); prod_alerts_init(&a); memset(&fb, 0, sizeof(fb));
    prod_archive_init(&ar[0], "daily", 7); prod_archive_init(&ar[1], "", 0); strcpy(dir[0], "/cfg"); dir[1][0] = 0;
    prod_archive_add(&ar[0], "/cfg/a", "mode=eco", 8);
    prod_alert_add(&a, "hot", PM_BUSY, 1, 80, 1);
    for (i = 0; i < 20; i++) { prod_sample_t s = S((uint32_t)i, 100U + (uint32_t)i, i == 19 ? 90U : 10U); prod_metrics_push(&m, &s); prod_alerts_eval(&a, &s); }
    prod_feedback_add(&fb, 4, "nice");
    ar[0].data[0] ^= 1; /* corruption must survive the round trip */
    n = prod_state_save(&r, blob, sizeof(blob));
    TEST_ASSERT_GREATER_THAN(0, n);
    TEST_ASSERT_EQUAL(-1, prod_state_save(&r, blob, 20));
    TEST_ASSERT_EQUAL(0, prod_state_load(&r2, blob, n));
    TEST_ASSERT_EQUAL(16, m2.n); TEST_ASSERT_EQUAL(20, (int)m2.total);
    prod_metric_stat(&m2, PM_MEM_USED, &st); TEST_ASSERT_EQUAL(119, (int)st.last); TEST_ASSERT_EQUAL(104, (int)st.min);
    TEST_ASSERT_EQUAL(1, a2.r[0].firing); TEST_ASSERT_EQUAL_STRING("hot", a2.r[0].name); TEST_ASSERT_EQUAL(1, (int)a2.fired_total);
    TEST_ASSERT_EQUAL_STRING("FIRING hot v=90", a2.ev[a2.ev_n - 1].text);
    TEST_ASSERT_EQUAL_STRING("daily", ar2[0].label); TEST_ASSERT_EQUAL_STRING("/cfg", dir2[0]); TEST_ASSERT_EQUAL(1, ar2[0].n);
    TEST_ASSERT_EQUAL(1, prod_archive_verify(&ar2[0], &bad));
    TEST_ASSERT_EQUAL(1, (int)fb2.total); TEST_ASSERT_EQUAL_STRING("nice", fb2.f[0].text);
    TEST_ASSERT_EQUAL(-1, prod_state_load(&r2, blob, n - 5)); /* truncated */
    TEST_ASSERT_EQUAL(0, m2.n);
    blob[0] ^= 1; TEST_ASSERT_EQUAL(-1, prod_state_load(&r2, blob, n));
}

void test_phase8_offline_extensions(void) {
    prod_perf_tune_t pt;
    prod_maint_t pm;
    prod_biz_metrics_t bm;
    prod_metrics_t m;
    prod_alerts_t a;
    prod_usage_t u;
    prod_scaler_t sc;
    prod_autoscale_workload_t wl;
    prod_sec_scan_t scan;
    char out_buf[512];

    prod_metrics_init(&m);
    prod_alerts_init(&a);
    memset(&u, 0, sizeof(u));

    /* US-106: measured rates are carried, the mode follows the busy average */
    TEST_ASSERT_EQUAL(0, prod_perf_tune_eval(&pt, &m, 111, 222, 333, 44, 1));
    TEST_ASSERT_EQUAL(1, (int)pt.tuning_mode);
    TEST_ASSERT_EQUAL(0, (int)pt.auto_chosen);
    TEST_ASSERT_EQUAL(111, (int)pt.cpu_perf_score);
    TEST_ASSERT_EQUAL(44, (int)pt.io_perf_score);
    TEST_ASSERT_EQUAL(0, prod_perf_tune_eval(&pt, &m, 1, 1, 1, 1, -1));
    TEST_ASSERT_EQUAL(0, (int)pt.tuning_mode);          /* no samples: balanced */
    TEST_ASSERT_EQUAL(1, (int)pt.auto_chosen);
    TEST_ASSERT_EQUAL(-1, prod_perf_tune_eval(&pt, &m, 1, 1, 1, 1, 3));
    {
        prod_metrics_t busy;
        prod_sample_t smp;
        int k;
        prod_metrics_init(&busy);
        memset(&smp, 0, sizeof(smp));
        for (k = 0; k < 6; k++) { smp.tick = (uint32_t)k * 100U; smp.v[PM_BUSY] = 90; smp.v[PM_MEM_FREE] = 1000U - (uint32_t)k * 100U; prod_metrics_push(&busy, &smp); }
        TEST_ASSERT_EQUAL(0, prod_perf_tune_eval(&pt, &busy, 1, 1, 1, 1, -1));
        TEST_ASSERT_EQUAL(90, (int)pt.busy_avg);
        TEST_ASSERT_EQUAL(6, (int)pt.samples);
        TEST_ASSERT_EQUAL(1, (int)pt.tuning_mode);
        TEST_ASSERT_EQUAL_STRING("high-throughput", prod_perf_tune_mode_name(pt.tuning_mode));
        /* US-115: free memory falls 1 per tick, 500 left: exhaustion soon */
        TEST_ASSERT_EQUAL(0, prod_maint_predict(&busy, &a, &pm));
        TEST_ASSERT_TRUE(pm.exhaust_ticks != 0xFFFFFFFFU && pm.exhaust_ticks < 6000U);
        TEST_ASSERT_EQUAL(50, (int)pm.failure_risk);
        TEST_ASSERT_EQUAL(50, (int)pm.health_score);
        for (k = 0; k < 6; k++) { smp.tick = 600U + (uint32_t)k * 100U; smp.v[PM_BUSY] = 10; prod_metrics_push(&busy, &smp); }
        TEST_ASSERT_EQUAL(0, prod_perf_tune_eval(&pt, &busy, 1, 1, 1, 1, -1));
        TEST_ASSERT_EQUAL(50, (int)pt.busy_avg);
        TEST_ASSERT_EQUAL(0, (int)pt.tuning_mode);
    }

    TEST_ASSERT_EQUAL(0, prod_maint_predict(&m, &a, &pm));
    TEST_ASSERT_EQUAL(100, (int)pm.health_score);
    TEST_ASSERT_EQUAL(0, (int)pm.failure_risk);
    TEST_ASSERT_TRUE(pm.exhaust_ticks == 0xFFFFFFFFU);

    TEST_ASSERT_EQUAL(0, prod_biz_metrics_get(&u, 10000, &bm));
    TEST_ASSERT_EQUAL(100, (int)bm.uptime_seconds);
    u.total = 50; u.n = 3;
    TEST_ASSERT_EQUAL(0, prod_biz_metrics_get(&u, 12000, &bm));
    TEST_ASSERT_EQUAL(25, (int)bm.business_kpi_score);   /* 50 commands in 2 minutes */
    TEST_ASSERT_EQUAL(3, (int)bm.distinct_commands);
    u.total = 0; u.n = 0;

    /* US-116: the bundle reflects live state */
    {
        static char bundle[2048];
        prod_feedback_t fb;
        int blen;
        memset(&fb, 0, sizeof(fb)); fb.total = 2;
        prod_alert_add(&a, "hot", PM_BUSY, 1, 80, 1);
        blen = prod_support_bundle_text(&m, &a, &u, &fb, 4242, bundle, (int)sizeof(bundle));
        TEST_ASSERT_TRUE(blen > 0);
        TEST_ASSERT_NOT_NULL(strstr(bundle, "mohhdy support bundle v1 tick 4242\n"));
        TEST_ASSERT_NOT_NULL(strstr(bundle, "alert hot busy > 80 ok\n"));
        TEST_ASSERT_NOT_NULL(strstr(bundle, "feedback total 2\nend\n"));
        TEST_ASSERT_EQUAL(-1, prod_support_bundle_text(&m, &a, &u, &fb, 1, bundle, 70));
        prod_alert_remove(&a, "hot");
    }

    /* US-108 CI/CD: real pipeline tested in test_cicd.c */

    /* US-110 Autoscaling Workload */
    prod_scaler_init(&sc, 1, 4, 4000U, 1000U, 2U);
    memset(&wl, 0, sizeof(wl));
    wl.queue_depth = 20;
    TEST_ASSERT_EQUAL(0, prod_autoscale_workload_eval(&sc, &wl, 100, out_buf, sizeof(out_buf)));
    TEST_ASSERT_GREATER_THAN(0, (int)wl.active_workers);
    TEST_ASSERT_NOT_NULL(strstr(out_buf, "autoscale workload workers "));

    /* US-112 security scanner: see test_sec_scan_real */
    (void)scan;
}

/* Lot 7 (US-112): ELF32 header audit and baseline integrity diff. */
static void put32(uint8_t* p, uint32_t v) { p[0] = (uint8_t)v; p[1] = (uint8_t)(v >> 8); p[2] = (uint8_t)(v >> 16); p[3] = (uint8_t)(v >> 24); }
static int mk_elf(uint8_t* b, uint32_t entry, uint32_t text_flags, uint32_t data_flags, uint32_t stack_flags) {
    memset(b, 0, 52 + 3 * 32);
    b[0] = 0x7F; b[1] = 'E'; b[2] = 'L'; b[3] = 'F'; b[4] = 1; b[5] = 1; b[6] = 1;
    b[16] = 2; b[18] = 3; b[20] = 1;
    put32(b + 24, entry); put32(b + 28, 52);
    b[42] = 32; b[44] = 3;
    put32(b + 52, 1); put32(b + 52 + 8, 0x40000000U); put32(b + 52 + 20, 0x1000); put32(b + 52 + 24, text_flags);
    put32(b + 84, 1); put32(b + 84 + 8, 0x40001000U); put32(b + 84 + 20, 0x1000); put32(b + 84 + 24, data_flags);
    put32(b + 116, 0x6474E551U); put32(b + 116 + 24, stack_flags);
    return 52 + 3 * 32;
}
void test_sec_scan_real(void) {
    uint8_t b[256];
    prod_elf_audit_t a;
    prod_sec_scan_t sc;
    char base[512], out_buf[512];
    uint32_t sz, h;
    int n;
    /* clean W^X image: R+X text, R+W data, RW stack */
    n = mk_elf(b, 0x40000010U, 5, 6, 6);
    TEST_ASSERT_EQUAL(0, prod_elf_audit(b, n, &a));
    TEST_ASSERT_EQUAL(2, a.load_segments);
    TEST_ASSERT_EQUAL(1, a.exec_segments);
    TEST_ASSERT_EQUAL(0, a.wx_segments);
    TEST_ASSERT_EQUAL(0, a.stack_exec);
    TEST_ASSERT_EQUAL(1, a.entry_ok);
    /* RWX data, executable stack, entry in the data segment */
    n = mk_elf(b, 0x40001004U, 5, 7, 7);
    TEST_ASSERT_EQUAL(0, prod_elf_audit(b, n, &a));
    TEST_ASSERT_EQUAL(1, a.wx_segments);
    TEST_ASSERT_EQUAL(1, a.stack_exec);
    TEST_ASSERT_EQUAL(1, a.entry_ok);          /* RWX data is executable */
    n = mk_elf(b, 0x40001004U, 5, 6, 6);
    TEST_ASSERT_EQUAL(0, prod_elf_audit(b, n, &a));
    TEST_ASSERT_EQUAL(0, a.entry_ok);
    /* not ELF, truncated program headers, wrong machine */
    TEST_ASSERT_EQUAL(-1, prod_elf_audit((const uint8_t*)"hello world, not an elf file at all......................", 60, &a));
    n = mk_elf(b, 0x40000010U, 5, 6, 6);
    TEST_ASSERT_EQUAL(-1, prod_elf_audit(b, 100, &a));
    b[18] = 0x28;
    TEST_ASSERT_EQUAL(-1, prod_elf_audit(b, n, &a));
    /* FNV-1a reference values */
    TEST_ASSERT_TRUE(0x811C9DC5U == prod_fnv1a((const uint8_t*)"", 0));
    TEST_ASSERT_TRUE(0xE40C292CU == prod_fnv1a((const uint8_t*)"a", 1));
    /* baseline text round trip */
    base[0] = 0;
    TEST_ASSERT_EQUAL(0, prod_sec_baseline_line(base, sizeof(base), "shell", 750768U, 0xdeadbeefU));
    TEST_ASSERT_EQUAL(0, prod_sec_baseline_line(base, sizeof(base), "ok", 13012U, 0x0000abcdU));
    TEST_ASSERT_EQUAL_STRING("shell 750768 deadbeef\nok 13012 0000abcd\n", base);
    TEST_ASSERT_EQUAL(2, prod_sec_baseline_count(base));
    TEST_ASSERT_EQUAL(1, prod_sec_baseline_find(base, "ok", &sz, &h));
    TEST_ASSERT_EQUAL(13012, (int)sz);
    TEST_ASSERT_TRUE(0xabcdU == h);
    TEST_ASSERT_EQUAL(0, prod_sec_baseline_find(base, "o", &sz, &h));
    /* folding: 3 files against a 3-entry baseline: one same, one changed,
     * one added (so one baseline entry is missing), one W+X ELF */
    prod_sec_scan_init(&sc);
    sc.baseline = 1;
    n = mk_elf(b, 0x40000010U, 5, 6, 6);
    prod_elf_audit(b, n, &a);
    prod_sec_scan_file(&sc, 1, &a, 1, 1);
    n = mk_elf(b, 0x40000010U, 7, 6, 6);
    prod_elf_audit(b, n, &a);
    prod_sec_scan_file(&sc, 1, &a, 1, 0);
    prod_sec_scan_file(&sc, 0, &a, 0, 0);
    prod_sec_scan_finish(&sc, 3);
    TEST_ASSERT_EQUAL(3, (int)sc.files);
    TEST_ASSERT_EQUAL(2, (int)sc.elf);
    TEST_ASSERT_EQUAL(1, (int)sc.wx);
    TEST_ASSERT_EQUAL(1, (int)sc.changed);
    TEST_ASSERT_EQUAL(1, (int)sc.added);
    TEST_ASSERT_EQUAL(1, (int)sc.missing);
    TEST_ASSERT_EQUAL(30 + 25 + 10 + 10, (int)sc.risk_score);
    prod_sec_scan_report(&sc, out_buf, sizeof(out_buf));
    TEST_ASSERT_NOT_NULL(strstr(out_buf, "sec-scan files 3 elf 2 wx_segments 1 exec_stack 0 bad_entry 0 changed 1 added 1 missing 1 risk_score 75 status findings (no CVE database)"));
    /* clean scan without baseline */
    prod_sec_scan_init(&sc);
    n = mk_elf(b, 0x40000010U, 5, 6, 6);
    prod_elf_audit(b, n, &a);
    prod_sec_scan_file(&sc, 1, &a, 0, 0);
    prod_sec_scan_finish(&sc, 0);
    prod_sec_scan_report(&sc, out_buf, sizeof(out_buf));
    TEST_ASSERT_NOT_NULL(strstr(out_buf, "baseline none risk_score 0 status clean"));
}

void test_smart_alerts_and_perf_monitor(void) {
    prod_metrics_t m;
    prod_alerts_t a;
    prod_alert_learn_t al;
    prod_perf_monitor_t pm;
    prod_sample_t s1 = S(10, 200, 30);
    prod_sample_t s2 = S(20, 200, 50);
    char out_buf[512];

    prod_metrics_init(&m);
    prod_alerts_init(&a);
    prod_metrics_push(&m, &s1);
    prod_metrics_push(&m, &s2);

    /* US-114 Smart Alert Threshold Learning */
    TEST_ASSERT_EQUAL(0, prod_alert_learn_update(&al, &m, PM_BUSY));
    TEST_ASSERT_EQUAL(2, (int)al.sample_count);
    TEST_ASSERT_EQUAL(40, (int)al.mean);
    TEST_ASSERT_EQUAL(0, prod_alert_learn_threshold(&al, 20));
    TEST_ASSERT_GREATER_THAN(47, (int)al.learned_threshold);

    /* US-106 / US-107 Performance Monitor Consolidation */
    TEST_ASSERT_GREATER_THAN(0, prod_perf_monitor_eval(&m, &a, &pm, out_buf, sizeof(out_buf)));
    TEST_ASSERT_EQUAL(100, (int)pm.overall_health_score);
    TEST_ASSERT_EQUAL(0, (int)pm.active_alerts);
    TEST_ASSERT_EQUAL(1, pm.system_status_ok);
    TEST_ASSERT_NOT_NULL(strstr(out_buf, "perf-monitor health 100 active_alerts 0"));
    TEST_ASSERT_NOT_NULL(strstr(out_buf, "status ok_healthy"));
}

void test_prod_consolidation_and_fallback(void) {
    prod_metrics_t m;
    prod_alerts_t a;
    prod_alert_learn_t al;
    prod_perf_monitor_t pm;
    char out_buf[512];

    prod_metrics_init(&m);
    prod_alerts_init(&a);

    /* Alert learn fallback on empty metrics or invalid metric ID */
    TEST_ASSERT_EQUAL(-1, prod_alert_learn_update(&al, &m, 999));
    TEST_ASSERT_EQUAL(-1, prod_alert_learn_update(&al, &m, PM_BUSY)); /* empty ring */

    /* Perf monitor fallback on empty metrics */
    TEST_ASSERT_GREATER_THAN(0, prod_perf_monitor_eval(&m, &a, &pm, out_buf, sizeof(out_buf)));
    TEST_ASSERT_EQUAL(100, (int)pm.overall_health_score);
    TEST_ASSERT_NOT_NULL(strstr(out_buf, "status ok_healthy"));
}


/* Lot 8 US-120: roadmap counted from a status file. */
void test_roadmap_parse(void) {
    static prod_roadmap_t rm;
    const char* t = "# comment\nUS-076 6 partial arm\nUS-079 6 done g\nUS-097 7 simule pay\n\nbogus line\nUS-120 8 open x\nUS-121 9 done y\nUS-001 1 maybe z\n";
    TEST_ASSERT_EQUAL(4, prod_roadmap_parse(t, (int)strlen(t), &rm));
    TEST_ASSERT_TRUE(rm.partial[6] == 1U && rm.done[6] == 1U && rm.simule[7] == 1U && rm.open[8] == 1U);
    TEST_ASSERT_TRUE(rm.bad == 3U);
    TEST_ASSERT_EQUAL(0, strcmp(rm.next, "US-076 US-097 US-120"));
    TEST_ASSERT_EQUAL(-1, prod_roadmap_parse(0, 0, &rm));
}

int main(void) {
    unity_init();
    RUN_TEST(test_roadmap_parse);
    RUN_TEST(test_metrics_ring_stats_and_prediction);
    RUN_TEST(test_alerts_for_n_dedupe_and_hysteresis);
    RUN_TEST(test_log_analysis);
    RUN_TEST(test_archive_verify_and_corruption);
    RUN_TEST(test_manifest_check);
    RUN_TEST(test_scaler_cooldown_and_bounds);
    RUN_TEST(test_baseline_feedback_usage);
    RUN_TEST(test_state_save_load);
    RUN_TEST(test_phase8_offline_extensions);
    RUN_TEST(test_sec_scan_real);
    RUN_TEST(test_smart_alerts_and_perf_monitor);
    RUN_TEST(test_prod_consolidation_and_fallback);
    unity_print_results();
    unity_cleanup();
    return (unity_stats.tests_failed == 0) ? 0 : 1;
}
