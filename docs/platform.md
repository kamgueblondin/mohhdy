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
| `screen-adapt [WxH]`, `screen-resize WxH` | 080 | form factor (phone/tablet/desktop), orientation, text grid, font size, panel count, compact mode, dynamic resizing; default is VGA text console 720x400 |
| `gesture x,y,ms,down ...` | 079 | single-pointer classifier: tap, double-tap, long-press, swipes, drag |
| `power-profile [performance\|balanced\|saver\|auto]`, `power-status` | 081 | policy table (yields, background budget, screen dim, service period); auto picks from the measured busy share of the other tasks (scheduler run ticks over 0.5 s) |
| `dev-list [NAME]`, `dev-rules`, `dev-hotplug NAME CLASS 0\|1 [DETAIL]` | 085 | device registry, hotplug rule engine, and simulated device attach/detach events |
| `compat-check PATH`, `compat-scan [DIR]` | 087 | ELF header check: 32-bit little-endian i386 ET_EXEC with program headers; ARM and x86-64 binaries named as such |
| `notify-push PRIO TEXT`, `notify-list`, `notify-ack ID` | 082 | local queue of 8, priority order, duplicate coalescing, eviction of lower priorities, drop counter |
| `migrate-export DIR ARCHIVE`, `migrate-verify ARCHIVE`, `migrate-import ARCHIVE DIR` | 088 | MMIG1 text archive (per-file FNV-1a and whole-archive FNV-1a), stored as 1000-byte chunks `ARCHIVE.0..` because ramfs files hold 1 KiB |
| `deploy-make SRC DST MANIFEST`, `deploy-apply MANIFEST`, `deploy-verify MANIFEST` | 090 | DEPLOY1 manifest with checksums; apply verifies every source first (nothing changes on mismatch), copies, reads back, and rolls back what it wrote on failure |
| `admin-all`, `admin-nodes`, `admin-node-add ID NAME ARCH FREE_KIB BUSY_PCT` | 089 | aggregated single-node view and multi-node administration management |

## User stories

| US | Status | Notes |
|----|--------|-------|
| US-076 ARM kernel | partial (lot 8) | ARMv7-A kernel core in `arch/arm` on QEMU `-M virt`: MMU with a PL1-only kernel section, GICv2, generic timer, USR tasks via SVC, preemption, portable `kernel/ipc.c` and `kernel/sha256.c`. Drivers, FS, AI, shell and OS-UI are not ported. Proof: `make qemu-arm-boot`, `test_arm_mmu`, CI job `arm-port` |
| US-077 native mobile UI | partial (lot 8) | `mobile_ui.c` + `mobile-ui`: phone/tablet layout, touch targets >= 48 px, nav bar, back stack, shade, gestures through US-079, hashed pixel model. No touch hardware (input simule), not painted on the VBE screen. Proof: `test_mobile_ui`, `make qemu-platform` |
| US-078 mobile sensors | partial (simule) | fixed values, the report says `status simule (no sensor hardware on i386, fixed values)` (`sensor-read`); no real sensor exists in QEMU i386 |
| US-079 touch gestures | done | single & multi-pointer gesture recognition (`gesture`, `gesture-multi`) |
| US-080 screen adaptation | done | layout computation, orientation, grid calculation and dynamic screen resizing (`screen-adapt`, `screen-resize`) |
| US-081 energy manager | done (no battery) | energy policy profiles and auto choice from the measured busy share (`power-profile`, `power-status` prints `battery none`) |
| US-082 push notifications | done | local push notification queue with TTL expiry (`notify-push`, `notify-list`, `notify-ack`) |
| US-083 multi-device sync | partial (lot 8) | `devsync.c` + `devsync-*`: LWW map (Lamport clock, device tie break, tombstones), version vectors, FNV-checked text deltas; three device replicas in one guest (simule), no inter-guest transport. Proof: `test_devsync`, `make qemu-platform` |
| US-084 cross-platform continuity | done | cross-device session handoff & clipboard sync protocol (`continuity-export`, `continuity-import`) |
| US-085 universal device manager | done | registry from existing probes, hotplug rule engine, attach/detach events (`dev-list`, `dev-rules`, `dev-hotplug`) |
| US-086 legacy emulation | done (8086 subset) | Lot 7: real interpreter of an 8086 subset with INT 21h basics running .COM files from `/dos` (`legacy-emul FILE [MAX_STEPS]`, `dosemu.c`); not a full DOS or BIOS |
| US-087 application compatibility | done (for this platform) | ELF compatibility check and scan |
| US-088 data migration | done (ramfs) | export, verify, import with checksums and tamper detection; ramfs files only, 16 files of 1 KiB |
| US-089 unified administration | done | aggregated single-node view and multi-node node management (`admin-all`, `admin-nodes`, `admin-node-add`) |
| US-090 automated deployment | done (local) | manifest, verified apply with rollback, verify; local files only |

## Proofs

* `tests/unit/userspace/test_platform.c` (9 tests).
* `tests/integration/test_qemu_platform.py` (`make qemu-platform`): every
  command above in one i386 guest, including the refusal paths (ARM port,
  bad size, unknown device, changed deploy source, tampered archive).

## Lot 9

- US-077 on screen: `mobile-ui ARGS` typed on the OS-UI VBE desktop (`gui`)
  shows the phone frame in the web pane. `mui_display_list` turns the mobile
  state into RECT and TEXT items; `test_mobile_ui` rasterizes them and compares
  every pixel with `mui_pixel`; `make qemu-osui-gui` checks the framebuffer
  (wallpaper 320x368 around 8 icons of 64x64, then the green app bar 320x48
  after `mobile-ui app chat`). Touch input stays simule (typed commands).
- US-083 between guests: see docs/p2p.md (`devsync-pull`).
