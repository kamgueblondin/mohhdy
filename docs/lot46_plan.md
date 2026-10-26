# Lot 46 - plan of the 4 PRs, status per item

Base: `origin/main` after the merge of PR #202 (Lot 45). One PR per theme,
each one green on `make test-all` and its QEMU gates before the next.
PR A and, at the owner's request before the merge, the storage part of
PR B, the US-001 check and the Raft part of PR D are in this branch
(`lot46-a-gui-parity`). The viewer part (PR C) and the interface
configuration command are not started.

| PR | Theme | Status |
|---|---|---|
| A | End of the GUI parity (this branch) | done for items 1-3, item 4: 7 of 11 US-G entries done, 4 stay partial (reasons below) |
| B | Storage: overlay files over 30 KiB (new disk layout, FAT16 starts at LBA 64), editor saves of 64 KiB (US-G16, US-G02) | done (v4 image, see below) |
| C | Viewer: JPEG baseline and interlaced / 16-bit PNG in the shared `imgdec` (US-G03) | not started |
| D | Kernel and network leftovers of Lot 45: US-001 (ATA IDENTIFY and FAT PIO fallback out of Ring 0), TCP ports honored, interface configuration command (US-G04), Raft partition tests (US-064) | US-001: already done since Lot 2, re-proved; Raft: leader partition and re-election in QEMU (done); TCP ports: no ignored port found, no new test; interface configuration: not started |

## PR A - items

| Item | Status | Proof |
|---|---|---|
| 1. Output of programs launched from a window (spawn, exec) | done: kernel capture `SYS_OUT_CAPTURE` (163, `kernel/out_capture.c`): while a window runs a command, the tasks created by the shell (and their children) print into a static 16 KiB buffer (`OS_OUTCAP_MAX`, 16 tracked pids) instead of the VGA console (serial kept); the shell copies it into the same window buffer when the child returns (exec) or after a bounded wait of 50 ticks (spawn). Past 16 KiB the bytes are counted, not kept, and the window says `[sortie du programme tronquee: N octets perdus sur T (16 Kio max)]`; a program still running after the wait is named (`[N programme(s) encore actif(s): ...]`, its later output goes to the console). Only the task that opened the capture reads or closes it, and only its descendants created while it is open are captured: the kernel rights are unchanged. One path for panes and tool windows (`shell_cap_open` / `shell_cap_close`, the old duplicated 64 KiB cut code is shared, same message) | `test_out_capture` (3 tests), `test_osui_tools` (test_spawn_note), `qemu-gui-parity` (spawn_capture: `spawn ok` shows `exec ok`; `spawn outflood`, 19825 bytes: 16384 kept, the note gives the 3441 dropped, equal to the kernel count `[OUTCAP] end`) |
| 2. Explorer columns and rewrap at resize | done: the name column of the file explorer takes the width of the window (46 columns at the default 77-column grid, as before; a name wider than its column is cut with `~` and shown whole in a wider window); the output already shown in a tool window (command output, P2P, PromptMessage run) is wrapped again at the new width: every stored line knows if it is the soft-wrapped part of the line above (`cont[]`), soft parts are joined, hard line ends and empty lines are kept, the view stays at the end or on the same text line | `test_osui_files` (test_columns_follow_window_width), `test_osui_tools` (test_output_rewraps_at_resize: 200 characters = 76+76+48 at the default grid, 119+81 at 120 columns, back to 76+76+48), `qemu-gui-ux5` (rewrap_steps: `ata-status` lines are at most 76 characters, after maximize the first line is 124 characters at 125 columns; the maximized explorer gets 125 columns, screendump `qemu-gui-lot46-files-max.ppm`) |
| 3. driver-restart of atadriver and networker in QEMU | done: atadriver stop then supervised respawn at once (new pid, `[ATA] supervised respawn bin/atadriver pid N`), then `write` / `cat` of a file and `ata-status driver N` served by the new driver; networker restarted by the shell (new pid), then the `netrelay` proof client spawned from the same window does its TCP loopback through the relay (`mode relay`) and `net-relay-status` names the new worker | `qemu-gui-parity` (driver_respawn) |
| 4. The 11 partial US-G entries | 7 done, 4 partial, see below | `qemu-gui-panels` (new, CI job gui-panels): 64 entries of /net /disk /services /logs /ci /security /devtools /notes run through the window menu with their arguments and the second Enter of state changes, each with a fragment of its real output; `test_osui_parity` (test_lot46_entries) |

### Item 4, per US

| US | Lot 46 status | Why |
|---|---|---|
| US-G05 storage /disk | done | every entry runs in QEMU; `persist-save` (MUT) and `fat16-list` added |
| US-G06 services /services | done | driver-restart of pcidriver (Lot 45), atadriver and networker (item 3) asserted in QEMU; every entry runs in `qemu-gui-panels` |
| US-G07 logs /logs | done | every entry in QEMU; `notify-push` and `notify-ack` (MUT) added |
| US-G09 production /ci | done | every entry in QEMU; `deploy-verify` took no argument and always answered "manifest not found": it now takes its manifest (root cause, the command needs it); `deploy-make` and `prod-backup` (MUT) added |
| US-G10 security /security | done | every entry in QEMU (init, sign, verify, revoke then verify refused); `pm-ca-revoke` added. The CA itself is still a local Schnorr CA (no X.509, no public authority): that is the scope of the CA, not of the panel |
| US-G11 web devtools /devtools | done | every entry in QEMU; `web-inspect` and `web-a11y` added |
| US-G13 notes /notes | done | every entry in QEMU; `migrate-import` (MUT) added |
| US-G02 editor | partial | lines up to 256 columns and window grid are done; saves stop at 30 KiB because of the overlay layout (US-G16): PR B |
| US-G03 viewer | partial | no JPEG, no interlaced or 16-bit PNG (refused with the reason in the window): PR C |
| US-G04 network /net | partial | every entry of /net runs in QEMU, but the shell has no command that configures the interface (address, route); a window cannot do more than the shell: PR D |
| US-G16 large saves | partial | 30 KiB per file inside the 32 KiB AIOV area; 64 KiB needs a new disk layout: PR B |

`prod-roadmap` phase 9 goes from done 7 partial 11 (38%) to done 14
partial 4 (77%) (`/roadmap.txt`, `qemu-production`).

In `qemu-gui-panels`, 62 of the 64 runs check a fragment of the real
output; `service-status ata-driver` and `web-inspect /web/demo.html hero`
only check that the window shows an output line of the shell (their
output is not asserted).

Build fix found by `qemu-production`: `pack-initrd` did not repack when a
hand-kept file of `initrd_content` changed (an edited `/roadmap.txt` left
the old initrd, phase 9 still read done 7); `roadmap.txt` and `samples/`
are now part of the stamp check.

## Bounds (new in PR A)

Capture of launched programs 16 KiB per window run and 16 tracked tasks,
bounded wait for spawned programs 50 ticks, window store 128 lines (the
rewrap at resize cannot bring back lines that scrolled out of it), window
output buffer 64 KiB (unchanged).

## Not done / still simule in PR A

- Output written by a launched program after the 50-tick wait goes to the
  console, not to the window (the note says so). Output through other
  paths than SYS_PUTC / SYS_PUTS (direct VGA syscalls) is not captured.
- `media-*` panel entries stay simule (unchanged).
- The /net panel runs on the loopback-only worker in QEMU (no NE2000 in
  the gui VMs); real NIC traffic is covered by the network suites, not by
  the GUI tests.

## Before the merge: review fixes, PR B, US-001, PR D

| Item | Status | Proof |
|---|---|---|
| Relaunched networker shown as "1 programme encore actif" | fixed: a task that registers a service leaves the capture (`out_cap_forget` in SYS_SERVICE_REGISTER), its output goes to the console | `test_out_capture` (test_daemon_leaves_and_dead_owner_releases), `qemu-gui-parity` (no "encore actif" after driver-restart networker) |
| Capture kept until the next call when the owner shell dies | fixed: released in `task_report_parent_exit` (exit, kill, fault) | same unit test |
| `outflood` in the production initrd | kept, documented: the build has one initrd that already carries the test probes (ok, devrogue, atarogue, pcirogue, ...); a test-only image is not done | - |
| US-G02, US-G03, US-G04 | G02 done (64 KiB saves); G03 partial: no JPEG, no interlaced or 16-bit PNG, not started; G04 partial: the shell has no command that sets the address or route, not started | `qemu-gui-bigsave` |
| PR B: US-G16 | done: overlay image v4 (header 32 bytes: magic, 4, 288 slots, records, ext_lba, 128); image sectors 0-63 at LBA 0-63, 64-191 at ext_lba..ext_lba+127; ext_lba = max(4225, 64 + FAT16 total sectors), enabled by atadriver only when IDENTIFY (words 60-61) reports ext_lba + 128 sectors and FAT16 is mounted; at boot the LBA of a v4 header is checked >= 4225 and >= the FAT16 end before it is read. 96 KiB store, 64 KiB per file; without the extension v3 exactly as before (32 KiB, 30 KiB). v1/v2/v3 load; v3 is rewritten as v4 on the next save. Kernel job queue (two areas, a chunk never crosses), mirror (96 KiB), boot install and Ring 0 fallback follow the header. Test disk 4353 sectors | `test_overlay` (test_v4_extension_64k_file, test_v3_image_loads_then_saves_v4; old v1/v2/v3 tests unchanged), `test_ata_job` (test_v4_image_two_areas), `make qemu-gui-bigsave` (new, CI job gui-panels): Save As of 65536 bytes, host parse of the disk byte exact, LBA 64-4221 byte identical to the fresh disk, cold reboot stat 65536, 65537 bytes refused and absent; `qemu-ata-driver` and `qemu-persistence` pass with the extension on |
| US-001 (PR C of the request) | nothing to move: IDENTIFY and the FAT PIO fallback are not linked in either image since Lot 2; supervised restart re-proved | `make strict-image-check`, `make qemu-ata-driver` (kill, mid-job #GP, respawn, reboot) |
| US-064 Raft | done: leader partition | `make qemu-p2p` scenario 2c: leader cut from both peers, its entry not committed, beta elected at a higher term after the 10 s lease, commits; after the heal alpha is follower at that term and applies the majority entry, the isolated entry is applied nowhere (pre-vote, compaction and InstallSnapshot: scenario 2b) |
| TCP ports | no ignored port found: `web-serve` listens on the requested port (8080, 8443 in `qemu-osui-web`), `ai-acquire` and `ai-peer-listen` take a port | no new test |
| Interface configuration in /net | not started | - |

Phase 9 of `prod-roadmap`: done 16 partial 2 (88%) (`qemu-production`).
