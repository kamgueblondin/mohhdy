# Lot 46 - plan of the 4 PRs, status per item

Base: `origin/main` after the merge of PR #202 (Lot 45). One PR per theme,
stacked: B on A, C on B, D on C (branches `lot46-a-gui-parity`,
`lot46-b-large-files`, `lot46-c-us001`, `lot46-d-network`). Order of the
user's work order (Lot 46): B large files, C US-001, D network. An earlier
version of this file described C as the JPEG viewer and D as the kernel and
network leftovers: that was wrong, corrected here.

| PR | Theme | Status |
|---|---|---|
| A | End of the GUI parity (`lot46-a-gui-parity`) | done for items 1-3, item 4: 7 of 11 US-G entries done, 4 stay partial (reasons below) |
| B | Large files (US-G16, US-G02): overlay v4 disk layout over 30 KiB without touching FAT16, 64 KiB Save As persistent after reboot, refusal past it, editor 30 KiB limit gone | partial - code written, compiled, NON TESTE (no unit test, no QEMU run yet, see "PR B") |
| C | US-001: ATA IDENTIFY and the FAT PIO fallback out of Ring 0 into the Ring 3 atadriver, supervised restart of atadriver without data loss (QEMU crash + recovery tests) | see "PR C" |
| D | Network: US-064 Raft (real network partition between guests, pre-vote, log compaction, QEMU partition and re-election tests), TCP ports honored end to end (listen and connect on the asked port, QEMU tests) | see "PR D" |

Not in the Lot 46 order (stay partial, unchanged): US-G03 viewer (JPEG,
interlaced / 16-bit PNG) and US-G04 (no shell command configures the
interface).

## PR B - large files (US-G16, US-G02)

Status: code written and compiled (`make CC=i686-linux-gnu-gcc
LD=i686-linux-gnu-ld all`, unit test binaries cross-compiled), NON TESTE:
no unit test, no QEMU script was run in this PR (credit budget). Nothing
of this PR is "done" until `make test-all` and `make qemu-large-files`
pass.

Disk layout (LBA, 512-byte sectors), checked against the FAT16 fixture
(`tests/scripts/make_fat16_image.py`): AIOV primary area 0-63 (unchanged),
FAT16 volume 64-4221 (unchanged geometry, `FAT_DISK_END` = 4225 keeps the
same total sector count in the boot sector), mount journal 4222-4223,
event journal 4224 (unchanged), new v4 extension area 4225-4352
(`OS_ATA_OVERLAY_EXT_LBA`, 128 sectors). The disk image grows from 4225 to
4353 sectors (`DISK_SECTORS`); no FAT sector moves or is written.

| Item | Status | What the code does | Test written (not run) |
|---|---|---|---|
| v4 image format | partial - code written, NON TESTE | same dense v3 records; header version 4, node count 288 (RAM slots, was 128); image of 96 KiB (primary 32 KiB + extension 64 KiB); written only when the overlay does not fit v3 (more than 32 KiB or more than 128 records), so a small overlay keeps the exact v3 bytes and the slice 2 jobs (64 sectors) keep working; a v4 image cut to 32 KiB is refused, RAM unchanged | `test_overlay`: `test_v4_64k_file_roundtrip_and_bound`, `test_v4_cut_image_refused`, `test_v3_kept_when_it_fits_and_after_shrink`, `test_image_lba_layout` |
| Backward compatible load v1/v2/v3 | partial - code written, NON TESTE | v1 and v2 (fixed slots) unchanged; v3 header with 128 nodes accepted as before; v4 with 288 | existing `test_restore_v1_snapshot_compatibility`, `test_restore_v2_disk_image` (still asserts the next snapshot is v3), v3 roundtrips, plus `test_v3_kept_when_it_fits_and_after_shrink` |
| Disk budget | partial - code written, NON TESTE | `overlay_set_capacity`: 96 KiB when the atadriver reads the extension area, else 32 KiB (disk made before Lot 46): then the Lot 44 bound (30 KiB per file, 128 records) holds exactly, so what is in RAM still always persists | `test_large_file_write_read_and_bounds` (Lot 44 test, every assertion kept, now pinned to the 32 KiB capacity), `test_v3_capacity_bounds_record_count` |
| 64 KiB per file, refusal past it | partial - code written, NON TESTE | `OV_FILE_MAX_BYTES` 65536; 65537 bytes (write or append) refused with `OV_ERR_NOSPACE`, the old file untouched | `test_v4_64k_file_roundtrip_and_bound`, QEMU `qemu-large-files` |
| Persistence through the Ring 3 atadriver | partial - code written, NON TESTE | boot install reads the extension when the primary header is v4 (an unreadable extension installs nothing); handover reads it and sets the capacity (`atadriver overlay capacity=98304 v4`); persist writes the changed sectors of the primary area then of the extension; sector IPC writes into the extension are refused like LBA 0-63; Ring 0 fallback `overlay_save_disk` / `overlay_load_disk` write and read both areas; kernel mirror 96 KiB | `qemu-large-files` |
| Save As 64 KiB byte for byte, after reboot | partial - code written, NON TESTE | editor Save As uses the same path as before (heap buffer, `fsops_write`, chunked relay of 4 KiB) | `tests/scripts/test_qemu_large_files.py` (`make qemu-large-files`, step added to the CI job gui-parity, timeout 15 -> 20 min): boot 1 opens `/samples/big64.txt` (new sample, 1024 lines of 64 bytes = 65536), Save As `/big64.txt`, one more character then Save As `/over64.txt` refused; boot 2 on the same disk: overlay loaded, `stat /big64.txt` 65536, first and last lines found, `/over64.txt` absent |
| Editor 30 KiB limit (US-G02) | partial - code written, NON TESTE | the editor had no limit of its own: the 30 KiB came from the overlay; it is gone with the same path | same QEMU script |

Limits that stay (honest):
- The slice 2 snapshot jobs (kernel-held overlay before the store
  handover) still move 64 sectors: a kernel-held overlay over 32 KiB is not
  flushed by them (the snapshot does not fit their buffer, the job is
  refused); it is persisted by the driver after the handover or by the
  Ring 0 fallback. Not tested.
- `FSOPS_FILE_MAX` stays 32 KiB: `cp` and the explorer copy of a file over
  32 KiB answer "trop gros" (unchanged); the editor and Save As use the
  heap path. Not changed to avoid touching the shared fsops buffers.
- A disk image made before Lot 46 (4225 sectors) keeps the 32 KiB budget.

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
