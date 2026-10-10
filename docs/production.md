# Phase 8 - Production (US-106 to US-120)

Status: delivered in the guest as C code (`userspace/prod.c`, pure and
host-tested; shell front end `userspace/shell_prod.c`). Proven by unit tests
(`tests/unit/userspace/test_prod.c`) and a single-guest QEMU contract
(`make qemu-production`). Independent of the P2P lots (#117, #119).
Everything runs inside one guest: there is no fleet, no remote collector,
no persistence across reboot (backups and metrics live in RAM).

## Commands

| Area | Commands |
|------|----------|
| Monitoring | `prod-sample [N] [GAP_TICKS]`, `prod-inject VALUE` (application metric), `prod-metrics` |
| Alerts | `prod-alert-add NAME METRIC above|below THRESHOLD [FOR]`, `prod-alert-del NAME`, `prod-alerts` |
| Predictive maintenance | `prod-predict METRIC LIMIT` |
| Log analysis | `prod-log-append PATH TEXT`, `prod-log-analyze PATH` |
| Backup | `prod-backup NAME DIR`, `prod-backups`, `prod-backup-verify NAME`, `prod-restore NAME`, `prod-backup-corrupt NAME` (test hook) |
| Deployment | `prod-manifest STAGING TARGET`, `prod-deploy STAGING TARGET`, `prod-rollback` |
| Scaling | `prod-scale-sim Q1 Q2 ...` |
| Security | `prod-integrity baseline|check DIR` |
| Performance | `prod-bench` |
| Support | `prod-diag`, `prod-tutorial [N]`, `prod-feedback RATING TEXT|summary`, `prod-usage`, `prod-roadmap` |

Metrics: `mem_used`, `mem_free` (KiB, from `SYS_MEMINFO`), `procs` (`SYS_PS`),
`busy` (share of scheduler run ticks of all tasks since the previous sample,
`SYS_TASK_METRICS`), `custom` (set by `prod-inject`). 64-sample ring.

Alerts fire after FOR consecutive breaching samples, are deduplicated while
firing, and resolve after two clean samples (hysteresis). Transitions are
printed (`prod-alert FIRING ...`) and kept in a 32-event journal.

Prediction: trend from the mean of the newer half of the window minus the
older half, over the mean tick gap (no 64-bit division in the freestanding
guest); prints ticks until LIMIT and `maintenance soon` under 60 s.

Log analysis: levels by keyword (error/fail/panic, warn, info), top five
patterns with numbers folded to `#`, worst error burst in 10 lines; a burst of
three or more is an incident (also written to the event journal).

Backup: every file of DIR (not recursive) copied into one of three RAM slots
(24 KiB, 24 files) with FNV-1a checksums; restore verifies first and writes
nothing if any entry is corrupt.

Deployment: back up TARGET as `pre-deploy`, copy STAGING files, check every
manifest entry checksum and `TARGET/health` == `ok`; on failure restore
`pre-deploy` automatically. `prod-rollback` restores it by hand.

## User stories

| US | Status | Notes |
|----|--------|-------|
| US-106 performance | partial | measurements (ALU, memory copy, syscall, small file I/O); no tuning done |
| US-107 monitoring | done | sampled metrics, ring, stats; QEMU |
| US-108 continuous deployment | partial | staged deploy with manifest and health check in one guest; no pipeline, no remote |
| US-109 automatic rollback | done | failed health check restores the previous files; QEMU |
| US-110 autoscaling | partial | decision engine (thresholds, bounds, cooldown) only; it starts no workers |
| US-111 backup/recovery | done | checksummed RAM backups, verified restore, corruption refusal; RAM only |
| US-112 production security | partial | file integrity baseline and drift detection; no hardening review |
| US-113 log analysis | done | levels, patterns, bursts, incident verdict; QEMU |
| US-114 intelligent alerts | partial | thresholds with FOR, dedupe, hysteresis; no learning |
| US-115 predictive maintenance | partial | linear trend to a limit; no model |
| US-116 technical support | partial | diagnostic bundle (`prod-diag`) |
| US-117 user training | partial | six built-in lessons |
| US-118 continuous feedback | done | ratings and comments with summary; RAM only |
| US-119 business metrics | partial | command usage counters |
| US-120 evolving roadmap | partial | static roadmap summary command |
