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
typedef struct {
    char name[16]; int metric; int above; uint32_t threshold; int for_n; int breach, clear, firing, used;
    /* US-114 adaptive rules: EWMA baseline (alpha 1/8, x16 fixed point) and
     * EWMA absolute deviation; fires above baseline + k * max(dev, 5%). */
    int adaptive, k, seen; uint32_t ewma16, dev16;
} prod_rule_t;
typedef struct { uint32_t tick; char text[64]; } prod_event_t;
typedef struct {
    prod_rule_t r[PROD_RULES];
    prod_event_t ev[PROD_EVENTS]; int ev_head, ev_n; uint32_t fired_total, resolved_total, suppressed;
} prod_alerts_t;
void prod_alerts_init(prod_alerts_t* a);
int prod_alert_add(prod_alerts_t* a, const char* name, int metric, int above, uint32_t threshold, int for_n);
#define PROD_LEARN 8   /* samples learned before an adaptive rule may fire */
int prod_alert_add_adaptive(prod_alerts_t* a, const char* name, int metric, int k, int for_n);
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

/* US-110 measured load: run queue = runnable (running/ready) user tasks
 * that actually consumed CPU in the measuring window (ran >= min_ran
 * ticks; a task that only yields is not load), minus the scaler's own
 * workers and the caller. */
typedef struct { int pid, state, user; uint32_t ran; } prod_task_t;
int prod_runq(const prod_task_t* t, int n, const int* exclude, int nexclude, uint32_t min_ran);

/* US-112 security scan over facts gathered by the host. */
#define PROD_SEC_TASKS 16
#define PROD_SEC_PORTS 4
typedef struct {
    char task[PROD_SEC_TASKS][32]; int ntask;
    struct { uint32_t port; int udp, encrypted; char what[16]; } port[PROD_SEC_PORTS]; int nport;
    int netkey_len, netkey_default;   /* P2P network key (0 len: P2P down) */
    int pass_len;                     /* persist passphrase length, 0: none */
    int signing_key, key_at_rest_encrypted;
} prod_sec_input_t;
/* Appends one "prod-sec ..." line per finding; returns the score 0..100
 * and sets *critical to the number of critical findings. */
int prod_sec_scan(const prod_sec_input_t* in, char* out, int cap, int* findings, int* critical);

#define PROD_BASE 32
typedef struct { char path[64]; uint32_t fnv; } prod_base_entry_t;
typedef struct { prod_base_entry_t e[PROD_BASE]; int n; } prod_baseline_t;
int prod_baseline_add(prod_baseline_t* b, const char* path, uint32_t fnv);
/* status of one current file: 0 same, 1 changed, 2 new (not in baseline) */
int prod_baseline_cmp(const prod_baseline_t* b, const char* path, uint32_t fnv);

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

/* Persistence (consolidation lot): compact serialization of the in-RAM
 * state; the shell stores it on the disk-backed overlay. Metrics keep the
 * newest 16 samples and alerts the newest 8 events. */
typedef struct {
    prod_metrics_t* m; prod_alerts_t* a; prod_archive_t* arch; char (*dir)[64]; int slots; prod_feedback_t* fb;
} prod_state_ref_t;
int prod_state_save(const prod_state_ref_t* r, uint8_t* out, int cap);
/* returns 0, or -1 on a malformed blob (state then reset) */
int prod_state_load(const prod_state_ref_t* r, const uint8_t* in, int len);
#endif
