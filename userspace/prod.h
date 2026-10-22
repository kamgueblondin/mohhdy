/* userspace/prod.h - Phase 8 production toolkit (US-106..US-120), pure C.
 * Metrics ring, threshold alerts with hysteresis, trend prediction, log
 * analysis, in-RAM backup archive with checksums, deploy health check,
 * autoscale decision, file integrity baseline, feedback and usage counters.
 * No allocation; host-testable. The shell front end is shell_prod.c. */
#ifndef MOHHDY_PROD_H
#define MOHHDY_PROD_H
#include <stdint.h>

enum { PM_MEM_USED = 0, PM_MEM_FREE, PM_PROCS, PM_BUSY, PM_CUSTOM, PM_COUNT };
#define PROD_SAMPLES 64
typedef struct { uint32_t tick; uint32_t v[PM_COUNT]; } prod_sample_t;
typedef struct { prod_sample_t s[PROD_SAMPLES]; int head, n; uint32_t total; } prod_metrics_t;
typedef struct { uint32_t min, max, avg, last; int n; } prod_stat_t;

void prod_metrics_init(prod_metrics_t* m);
void prod_metrics_push(prod_metrics_t* m, const prod_sample_t* s);
int prod_metric_id(const char* name); /* -1 unknown */
const char* prod_metric_name(int id);
int prod_metric_stat(const prod_metrics_t* m, int id, prod_stat_t* out);
/* Least squares slope in milli-units per tick over the window, and the
 * predicted ticks until `limit` is crossed (0xFFFFFFFF: not trending there). */
int prod_predict(const prod_metrics_t* m, int id, uint32_t limit, int32_t* slope_milli, uint32_t* ticks_left);

#define PROD_RULES 8
#define PROD_EVENTS 32
typedef struct { char name[16]; int metric; int above; uint32_t threshold; int for_n; int breach, clear, firing, used; } prod_rule_t;
typedef struct { uint32_t tick; char text[64]; } prod_event_t;
typedef struct {
    prod_rule_t r[PROD_RULES];
    prod_event_t ev[PROD_EVENTS]; int ev_head, ev_n; uint32_t fired_total, resolved_total, suppressed;
} prod_alerts_t;
void prod_alerts_init(prod_alerts_t* a);
int prod_alert_add(prod_alerts_t* a, const char* name, int metric, int above, uint32_t threshold, int for_n);
int prod_alert_remove(prod_alerts_t* a, const char* name);
/* Evaluates every rule on one sample; returns the number of transitions. */
int prod_alerts_eval(prod_alerts_t* a, const prod_sample_t* s);
void prod_event(prod_alerts_t* a, uint32_t tick, const char* text);

typedef struct { char pattern[48]; uint32_t count; } prod_pattern_t;
typedef struct {
    uint32_t lines, errors, warns, infos, other;
    uint32_t burst_max;   /* max errors in any window of 10 consecutive lines */
    uint32_t burst_line;  /* 1-based line where that window ends */
    char first_error[64];
    prod_pattern_t top[5]; int ntop;
} prod_log_report_t;
void prod_log_analyze(const char* text, prod_log_report_t* r);

#define PROD_ARCH_FILES 24
#define PROD_ARCH_BYTES 24576
typedef struct { char path[64]; uint32_t off, len, fnv; } prod_arch_entry_t;
typedef struct { char label[16]; uint32_t tick; prod_arch_entry_t e[PROD_ARCH_FILES]; int n; uint32_t used; uint8_t data[PROD_ARCH_BYTES]; } prod_archive_t;
uint32_t prod_fnv(const void* p, uint32_t n);
void prod_archive_init(prod_archive_t* a, const char* label, uint32_t tick);
int prod_archive_add(prod_archive_t* a, const char* path, const void* data, uint32_t len);
/* returns number of corrupted entries (0: intact) */
int prod_archive_verify(const prod_archive_t* a, int* first_bad);

/* Deploy manifest: lines "path fnvhex". read(path, buf, cap) -> length or <0. */
typedef int (*prod_read_fn)(void* ctx, const char* path, char* buf, int cap);
int prod_manifest_check(const char* manifest, prod_read_fn rd, void* ctx, char* why, int why_cap);

typedef struct { int min, max, cur; uint32_t high_milli, low_milli, cooldown, last_change; int ups, downs; } prod_scaler_t;
void prod_scaler_init(prod_scaler_t* s, int min, int max, uint32_t high_milli, uint32_t low_milli, uint32_t cooldown);
/* returns the new worker count for a queue length at tick now */
int prod_scaler_step(prod_scaler_t* s, uint32_t queue, uint32_t now);

#define PROD_BASE 32
typedef struct { char path[64]; uint32_t fnv; } prod_base_entry_t;
typedef struct { prod_base_entry_t e[PROD_BASE]; int n; } prod_baseline_t;
int prod_baseline_add(prod_baseline_t* b, const char* path, uint32_t fnv);
/* status of one current file: 0 same, 1 changed, 2 new (not in baseline) */
int prod_baseline_cmp(const prod_baseline_t* b, const char* path, uint32_t fnv);

/* US-106 auto-tuning (Lot 7): the scores are rates measured in the guest by
 * the shell (ALU loop, memory copy, syscall round trip, 512-byte file rw),
 * the mode is chosen from the recorded busy average unless requested. */
typedef struct {
    uint32_t cpu_perf_score;   /* measured ALU ops per second */
    uint32_t mem_perf_score;   /* measured KiB copied per second */
    uint32_t sys_perf_score;   /* measured syscalls per second */
    uint32_t io_perf_score;    /* measured 512-byte rw ops per second */
    uint32_t busy_avg;         /* average of the recorded busy samples, 0 if none */
    uint32_t samples;          /* busy samples behind busy_avg */
    uint32_t tuning_mode;      /* 0 = balanced, 1 = high throughput, 2 = low latency */
    uint32_t auto_chosen;      /* 1 when the mode came from the busy average */
} prod_perf_tune_t;

/* US-115 (Lot 7): failure risk from firing alerts plus the free-memory trend
 * (prod_predict); exhaust_ticks is 0xFFFFFFFF when memory is not trending to
 * zero. A heuristic, not a learned model. */
typedef struct {
    uint32_t exhaust_ticks;    /* predicted ticks until free memory reaches 0 */
    uint32_t firing_alerts;
    uint32_t failure_risk;     /* 0-100 */
    uint32_t health_score;     /* 100 - failure_risk */
} prod_maint_t;

/* US-115 (Lot 9): learned failure predictor. A logistic regression in
 * Q12 fixed point (no floating point, no division), trained by stochastic
 * gradient descent on windows of the guest's own metrics ring. A sample
 * is a failure when the application metric (custom) reaches fail_at or
 * free memory falls to fail_mem_pct percent of the total; a window is
 * labelled positive when a failure follows within `horizon` samples.
 * Features of the window ending at sample i (PROD_MM_WIN samples):
 * custom level, custom trend, memory used share, busy share, bias. */
#define PROD_MM_FEAT 5
#define PROD_MM_WIN 4
typedef struct {
    int32_t w[PROD_MM_FEAT];         /* Q12 weights */
    uint32_t fail_at, fail_mem_pct, horizon;
    uint32_t trained, windows, positives, correct, epochs;
} prod_maint_model_t;
/* Q12 logistic of a Q12 argument (table over [-8, 8], linear interpolation) */
int32_t prod_sigmoid_q12(int32_t z);
/* features of the window ending at ring index `end` (0 = oldest); -1 when too short */
int prod_maint_features(const prod_metrics_t* m, const prod_maint_model_t* mm, int end, int32_t x[PROD_MM_FEAT]);
/* trains from scratch; returns the number of windows, -1 when < 8 or no positive or no negative */
int prod_maint_train(prod_maint_model_t* mm, const prod_metrics_t* m, uint32_t fail_at, uint32_t fail_mem_pct,
                     uint32_t horizon, uint32_t epochs);
/* failure probability 0..100 for the newest window, -1 when untrained or too few samples */
int prod_maint_model_risk(const prod_maint_model_t* mm, const prod_metrics_t* m);

/* US-119 (Lot 7): counters observed in this guest; one console session. */
typedef struct {
    uint32_t total_tasks_executed;   /* shell command words counted */
    uint32_t distinct_commands;
    uint32_t uptime_seconds;
    uint32_t business_kpi_score;     /* commands per minute of uptime */
    uint32_t avg_exec_time_ms;
    uint32_t global_efficiency_score;
} prod_biz_metrics_t;

/* requested_mode < 0 lets the busy average choose. */
int prod_perf_tune_eval(prod_perf_tune_t* pt, const prod_metrics_t* m, uint32_t alu, uint32_t mem,
                        uint32_t sys, uint32_t io, int requested_mode);
const char* prod_perf_tune_mode_name(uint32_t mode);
int prod_maint_predict(const prod_metrics_t* m, const prod_alerts_t* a, prod_maint_t* pm);

/* US-108 CI/CD: real local pipeline in cicd.h (Lot 7; the former
 * prod_pipeline_run printed 5/5 and ran nothing). */

/* US-110 Autoscaling with workload generator */
typedef struct {
    uint32_t active_workers;
    uint32_t queue_depth;
    uint32_t cpu_load_pct;
    uint32_t scale_events;
} prod_autoscale_workload_t;
/* Lot 23 (US-110 / US-120): Dynamic workload autoscaling evaluation */
int prod_autoscale_workload_eval(prod_scaler_t* s, prod_autoscale_workload_t* wl, uint32_t now_tick, char* out, int cap);
int prod_autofix_stalled_workers(prod_autoscale_workload_t* wl, char* out, int cap);

/* US-112 security scanner (Lot 7, real checks; no CVE database):
 * ELF32 program headers of each file (W+X PT_LOAD segments, executable
 * PT_GNU_STACK, entry point outside an executable segment) and an
 * integrity diff against a baseline (size + FNV-1a of the first 64 KiB). */
#define PROD_SEC_HASH_MAX 65536
typedef struct {
    int is_elf;
    int load_segments, exec_segments, wx_segments;
    int stack_exec;
    int entry_ok;
    uint32_t entry;
} prod_elf_audit_t;
/* 0 audited, -1 not an ELF32 i386 image or truncated headers */
int prod_elf_audit(const uint8_t* b, int len, prod_elf_audit_t* r);
uint32_t prod_fnv1a(const uint8_t* b, int len);
/* baseline text: one "NAME SIZE HASHHEX" line per file */
int prod_sec_baseline_line(char* out, int cap, const char* name, uint32_t size, uint32_t hash);
int prod_sec_baseline_find(const char* text, const char* name, uint32_t* size, uint32_t* hash);
int prod_sec_baseline_count(const char* text);
typedef struct {
    uint32_t files, elf, not_elf;
    uint32_t wx, stack_exec, bad_entry;
    uint32_t baseline, changed, added, missing;
    uint32_t risk_score;   /* 0-100, from the findings above */
} prod_sec_scan_t;
void prod_sec_scan_init(prod_sec_scan_t* s);
/* folds one file: audit result (or not ELF) and baseline comparison
 * (in_baseline 0/1, same 0/1) */
void prod_sec_scan_file(prod_sec_scan_t* s, int audited, const prod_elf_audit_t* a, int in_baseline, int same);
void prod_sec_scan_finish(prod_sec_scan_t* s, int baseline_entries);
int prod_sec_scan_report(const prod_sec_scan_t* s, char* out, int cap);

/* US-112 (Lot 9): local signature database. One record per line:
 *   ID SEVERITY hex HEXBYTES     byte pattern searched in the file (hex so
 *                                the database never matches itself)
 *   ID SEVERITY fnv HASH8 SIZE   known-bad file (FNV-1a of the first
 *                                PROD_SEC_HASH_MAX bytes and exact size)
 * SEVERITY: low medium high critical. '#' lines are comments. Advisory ids
 * are local (MSA-...), not CVE numbers: there is no online CVE feed. */
#define PROD_SECDB_MAX 32
#define PROD_SECDB_PAT 32
enum { PROD_SEV_LOW = 1, PROD_SEV_MEDIUM, PROD_SEV_HIGH, PROD_SEV_CRITICAL };
enum { PROD_SIG_HEX = 1, PROD_SIG_FNV };
typedef struct {
    char id[20];
    uint8_t severity, kind;
    uint8_t pat[PROD_SECDB_PAT]; int plen;
    uint32_t hash, size;
} prod_secdb_rec_t;
typedef struct { prod_secdb_rec_t r[PROD_SECDB_MAX]; int n; int bad_lines; } prod_secdb_t;
extern const char prod_secdb_builtin[];
/* records parsed (malformed lines counted in bad_lines), -1 when the table overflows */
int prod_secdb_parse(const char* text, prod_secdb_t* db);
/* number of records matching this file; their indices in hits[0..max) */
int prod_secdb_match(const prod_secdb_t* db, const uint8_t* b, int len, uint32_t size, uint32_t hash,
                     int* hits, int max);
const char* prod_sev_name(int sev);
uint32_t prod_sev_weight(int sev);

#define PROD_FEEDBACK 16
typedef struct { uint8_t rating; char text[48]; } prod_fb_t;
typedef struct { prod_fb_t f[PROD_FEEDBACK]; int n; uint32_t total, sum; uint32_t hist[6]; } prod_feedback_t;
int prod_feedback_add(prod_feedback_t* f, int rating, const char* text);

#define PROD_USAGE 24
typedef struct { char name[20]; uint32_t count; } prod_use_t;
typedef struct { prod_use_t u[PROD_USAGE]; int n; uint32_t total, other; } prod_usage_t;
void prod_usage_hit(prod_usage_t* u, const char* name);
/* sorts by count desc (stable for ties) */
void prod_usage_sort(prod_usage_t* u);

int prod_biz_metrics_get(const prod_usage_t* u, uint32_t uptime_ticks, prod_biz_metrics_t* bm);
int prod_biz_kpi_eval(const prod_usage_t* u, uint32_t uptime_ticks, uint32_t total_cmd_time_ms, prod_biz_metrics_t* bm);

/* US-114 Dynamic Threshold Learning */
typedef struct {
    uint32_t sample_count;
    uint32_t mean;
    uint32_t std_dev;
    uint32_t learned_threshold;
} prod_alert_learn_t;

int prod_alert_learn_update(prod_alert_learn_t* al, const prod_metrics_t* m, int metric_id);
int prod_alert_learn_threshold(prod_alert_learn_t* al, uint32_t margin_pct);

/* US-106 / US-107 Performance Monitor Consolidation */
typedef struct {
    uint32_t overall_health_score;
    uint32_t active_alerts;
    uint32_t sample_count;
    uint32_t avg_busy_pct;
    uint32_t free_mem_kib;
    int system_status_ok;
    uint32_t auto_restarts;
} prod_perf_monitor_t;

/* Lot 23 (US-110 / US-120): Performance monitor evaluation */
int prod_perf_monitor_eval(const prod_metrics_t* m, const prod_alerts_t* a, prod_perf_monitor_t* pm, char* out, int cap);

/* Persistence (consolidation lot): compact serialization of the in-RAM
 * state; the shell stores it on the disk-backed overlay. Metrics keep the
 * newest 16 samples and alerts the newest 8 events. */
typedef struct {
    prod_metrics_t* m; prod_alerts_t* a; prod_archive_t* arch; char (*dir)[64]; int slots; prod_feedback_t* fb;
} prod_state_ref_t;
int prod_state_save(const prod_state_ref_t* r, uint8_t* out, int cap);
/* returns 0, or -1 on a malformed blob (state then reset) */
int prod_state_load(const prod_state_ref_t* r, const uint8_t* in, int len);
/* US-116 support bundle (Lot 7): plain-text diagnostic report built from the
 * live metrics, alerts, events, usage and feedback; returns its length or -1
 * when cap is too small. The shell writes it to the overlay and verifies it. */
int prod_support_bundle_text(const prod_metrics_t* m, const prod_alerts_t* a, const prod_usage_t* u,
                             const prod_feedback_t* f, uint32_t tick, char* out, int cap);
/* US-120 (Lot 8): roadmap computed from a status file (/roadmap.txt in the
 * initrd): one "US-NNN PHASE STATUS NOTE" line per story. */
#define PROD_RM_PHASES 9
typedef struct {
    uint32_t done[PROD_RM_PHASES], partial[PROD_RM_PHASES], simule[PROD_RM_PHASES], open[PROD_RM_PHASES];
    uint32_t entries, bad;
    uint32_t checksum_fnv;       /* FNV-1a checksum over roadmap content */
    char next[256];              /* stories not done, in file order */
} prod_roadmap_t;
int prod_roadmap_parse(const char* text, int len, prod_roadmap_t* rm);
int prod_roadmap_eval(const prod_roadmap_t* rm, uint32_t* total_done, uint32_t* total_entries, uint32_t* pct);
#endif
