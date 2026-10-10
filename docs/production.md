# Phase 8 - Production (US-106 to US-120)

Status: delivered in the guest as C code (`userspace/prod.c`, pure and
host-tested; shell front end `userspace/shell_prod.c`). Proven by unit tests
(`tests/unit/userspace/test_prod.c`) and a single-guest QEMU contract
(`make qemu-production`). Independent of the P2P lots (#117, #119).
Everything runs inside one guest: there is no fleet and no remote collector.
Since the consolidation lot (#121) the state can be saved to disk and
reloaded after a cold reboot (`persist-*`, below).

## Persistence (consolidation lot)

`persist-save` writes four checksummed blobs to the overlay: `node` (the
P2P identity seed, so the node id and the collab signing key stay the same),
`prod` (newest 16 samples, alert rules and state, newest 8 events, the three
backup slots with their stored checksums, feedback) and `collab` (the whole
ledger with signatures, known public keys and the node's own key) and
`p2pkv` (the P2P key/value store, re-applied right after `p2p-up`). Overlay
files hold at most 384 bytes, so each blob is split into
`/persist/<name>.<k>` chunks; chunk 0 starts with magic, length and FNV-1a,
checked by `persist-load` / `persist-status` (a damaged blob is refused and
nothing is applied from it). With an IDE disk attached every overlay write is
flushed to LBA 0-63 by the Ring 3 `atadriver`, and the kernel reloads that
snapshot at boot. The overlay holds 64 nodes, which bounds the total size
(about 20 KiB).

Automatic saving: when the kernel started an `atadriver` (a disk is present)
the shell saves every 60 s by default (`persist-auto SECONDS|off`, 5 to
3600 s), and only the blobs whose content changed. The check runs after each
command and while the shell polls for keys (P2P or web active); a shell
blocked in a plain read does not save until the next command. `shutdown`,
`reboot` and `exit` save what changed first (`persist shutdown save ok`);
there is no ACPI power-off, so this covers the shell's own clean exits, not
a QEMU kill.

Signing key at rest: the secret key is never written in clear.
`persist-passphrase P` (8+ characters, kept in RAM only) makes the collab
blob carry the key encrypted with AES-128-GCM under PBKDF2-HMAC-SHA256
(2000 iterations, random salt and nonce, the tag also covers the key
length). Without a passphrase the key is not stored at all. After a reboot
the ledger loads with signing locked; `persist-unlock P` decrypts it (a wrong
passphrase fails the GCM tag) and saves skip the collab blob while it is
locked, so the encrypted key is never overwritten by a key-less copy. Salt
and nonce come from TSC + ticks (weak entropy in emulation).

Proof: `make qemu-persistence` (two boots of one guest on the same image:
autosave, shutdown save, locked key, wrong and right passphrase, P2P store,
signed PromptMessage certificate).

## Autoscaling with real tasks

`prod-scale-run Q1 Q2 ...` runs one scaler step per queue value (min 1, max
3 workers, scale up above 4 items per worker, down below 1, cooldown 2
steps) and starts or kills real tasks (the `idle` program, which only
yields) to match; `prod-scale-status` / `prod-scale-stop` report and stop the
pool, checking every pid against `ps`. Workers do no useful work; the shell's
four-children limit caps the pool.

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
| Scaling (real tasks) | `prod-scale-run Q1 Q2 ...`, `prod-scale-status`, `prod-scale-stop` |
| Persistence | `persist-save`, `persist-load`, `persist-status`, `persist-auto`, `persist-passphrase`, `persist-unlock` |
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
| US-110 autoscaling | partial | scaler starts/stops real worker tasks (`prod-scale-run`, #121); workers do no useful work, load is a given queue length |
| US-111 backup/recovery | done | checksummed backups, verified restore, corruption refusal; persisted to disk with `persist-save` (#121) |
| US-112 production security | partial | file integrity baseline and drift detection; no hardening review |
| US-113 log analysis | done | levels, patterns, bursts, incident verdict; QEMU |
| US-114 intelligent alerts | partial | thresholds with FOR, dedupe, hysteresis; no learning |
| US-115 predictive maintenance | partial | linear trend to a limit; no model |
| US-116 technical support | partial | diagnostic bundle (`prod-diag`) |
| US-117 user training | partial | six built-in lessons |
| US-118 continuous feedback | done | ratings and comments with summary; RAM only |
| US-119 business metrics | partial | command usage counters |
| US-120 evolving roadmap | partial | static roadmap summary command |
