# Phase 6 - Multi-platform groundwork (US-076 to US-090)

Status: groundwork delivered in the i386 QEMU guest, in C
(`userspace/platform.c`, pure and host-tested; shell front end
`userspace/shell_platform.c`). There is no ARM port: no ARM kernel, boot
code, toolchain or image exists in this tree, and the HAL says so
(`hal-port arm` answers "not ported"). Nothing here runs on a phone.

## Commands

| Command | US | What it does |
|---|---|---|
| `hal-info` | 076 | CPUID vendor/family/features, page size, endianness, memory; ports `i386=built arm=not-ported` |
| `hal-port ARCH` | 076 | ok for i386, honest refusal otherwise |
| `screen-adapt [WxH]` | 080 | form factor (phone/tablet/desktop), orientation, text grid, font size, panel count, compact mode; default is the VGA text console 720x400 |
| `gesture x,y,ms,down ...` | 079 | single-pointer classifier: tap, double-tap, long-press, swipes, drag |
| `power-profile [performance\|balanced\|saver\|auto]`, `power-status` | 081 | policy table (yields, background budget, screen dim, service period); auto picks from the measured busy share of the other tasks (scheduler run ticks over 0.5 s) |
| `dev-list [NAME]` | 085 | registry probed from existing syscalls: cpu, memory, keyboard, VGA, RTC, PIT, NE2000 (net status), ATA (status), touch and sensor reported absent |
| `compat-check PATH`, `compat-scan [DIR]` | 087 | ELF header check: 32-bit little-endian i386 ET_EXEC with program headers; ARM and x86-64 binaries named as such |
| `notify-push PRIO TEXT`, `notify-list`, `notify-ack ID` | 082 | local queue of 8, priority order, duplicate coalescing, eviction of lower priorities, drop counter |
| `migrate-export DIR ARCHIVE`, `migrate-verify ARCHIVE`, `migrate-import ARCHIVE DIR` | 088 | MMIG1 text archive (per-file FNV-1a and whole-archive FNV-1a), stored as 1000-byte chunks `ARCHIVE.0..` because ramfs files hold 1 KiB |
| `deploy-make SRC DST MANIFEST`, `deploy-apply MANIFEST`, `deploy-verify MANIFEST` | 090 | DEPLOY1 manifest with checksums; apply verifies every source first (nothing changes on mismatch), copies, reads back, and rolls back what it wrote on failure |
| `admin-all` | 089 | one line: arch, cpu, free memory, devices present/total, power profile, pending notes, uptime |

## User stories

| US | Status | Notes |
|----|--------|-------|
| US-076 ARM kernel | partial (groundwork only) | HAL description of the i386 port; no ARM code. Porting is not started |
| US-077 native mobile UI | not delivered | nothing mobile exists |
| US-078 mobile sensors | not delivered | no sensor hardware is emulated; `dev-list` reports `sensor0 absent` |
| US-079 touch gestures | partial | classifier from pointer samples, fed from the shell; not wired to a touch device (none emulated) |
| US-080 screen adaptation | partial | layout computation for any size, shown for the real console; OS-UI is not yet re-laid out from it |
| US-081 energy manager | partial | profiles and auto choice from a measured busy share; the policy is reported, not yet applied to the schedulers; no battery |
| US-082 push notifications | partial | local queue only; not distributed (P2P transport is in #117) |
| US-083 multi-device sync | not delivered here | covered by the P2P replication of phase 5 (#117), not wired to devices |
| US-084 cross-platform continuity | not delivered | |
| US-085 universal device manager | partial | registry from existing probes; no hotplug, no PCI enumeration |
| US-086 legacy emulation | not delivered | |
| US-087 application compatibility | done (for this platform) | ELF compatibility check and scan |
| US-088 data migration | done (ramfs) | export, verify, import with checksums and tamper detection; ramfs files only, 16 files of 1 KiB |
| US-089 unified administration | partial | one aggregated view (`admin-all`); no remote administration |
| US-090 automated deployment | done (local) | manifest, verified apply with rollback, verify; local files only |

## Proofs

* `tests/unit/userspace/test_platform.c` (9 tests).
* `tests/integration/test_qemu_platform.py` (`make qemu-platform`): every
  command above in one i386 guest, including the refusal paths (ARM port,
  bad size, unknown device, changed deploy source, tampered archive).
