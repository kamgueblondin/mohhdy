# Lot 49 plan (branch cursor/lot49, from main ff9d13f)

Status: in progress on PR #206. Each item says what is done and how it
is proven. Nothing below is marked done without a test. Status words:
done, partial, missing, simule, non prouvable.

## 1. Done on this branch

- US-120 `done`. The US master has a phase 6-8 status table (US-076..US-120).
  `make roadmap-check` compares every phase 6-9 status and phase with that
  table and fails on any difference. prod-roadmap phase 8 is done 15
  partial 0. Proof: `make roadmap-check`, `make qemu-production`.
- Lot 48 documentation. `docs/lot48_plan.md` section 4 records the measured
  TLS handshake (198 ticks, 1.98 s, down to about 11 ticks, 0.11 s guest
  time) and the phase 5 statuses: US-069, US-070, US-075 done on the lab
  network; US-068 and US-073 stay partial.
- GUI, partial usability, not a new window system. One shell
  (`userspace/shell.c`), one file path (`fsops`). `/docs`, `/home` and
  `/home/user` exist in the shell ramfs from boot. They are copied into
  the kernel VFS when `gui` starts, together with `/docs/readme.txt`, so
  the explorer and `ls` list the same tree. They are not created at shell
  init: that filled the overlay and `vfs-list overlay/` was no longer
  `count 0` (CI job vfs-service failed on 65bb9fb). A maximize is painted
  once, when the window-event line is complete (guest lag from the kernel
  maximize to the editor resize was about 2.3 s, the guard is 1.2 s).
  Terminal blue (ANSI 34) is light blue (0x09). Proof: `make qemu-gui-lot49`.
- US-031 stays `partial`. `userspace/webeng.c` lays out `display:flex`
  (row: equal columns; `flex-direction:column`: stacked) and `display:grid`
  (`grid-template-columns: repeat(N, 1fr)`, N from 1 to 8, row-major).
  `gap` is one length on both axes. No flex-grow, no wrap, no align, no
  `grid-template-rows`, no named areas. No JavaScript VM. No network fetch
  inside the engine. `phase3_complete` stays false. Chromium and WebKit
  are not present. Proof: `test_flex_and_grid` in `test_webeng`.
- US-055 stays `partial`. `pm-cross FILE` is still portable C99.
  `pm-cross FILE py` emits Python 3 with the same output, the same 20000
  step budget and the same path rule (`/bin/` and `/models/` refused).
  The guest prints the text between markers. The host runs it. No native
  code generator, no third target. Proof: `test_cross_compile_py_matches_vm`.
- US-058 stays `partial`. `pm-assist` is still rules (`engine=rules`, no
  language model). A block that does not close now gets the hint
  `this block needs an 'end'`. Proof: `test_assist_hints_and_lint`.
- US-G03 stays `done`. Progressive Huffman JPEG (SOF2) decodes in the
  shared `imgdec` used by `img-info` and the desktop viewer: spectral
  selection and successive approximation, gray or YCbCr, sampling 1..2,
  restart markers. The caller owns the scratch: 8-bit pixels, then
  (SOF2 only) one int16 coefficient block per MCU block. Arithmetic
  (SOF9), lossless, 12-bit and CMYK stay `IMGDEC_E_UNSUP`. Proof:
  `test_jpeg_baseline` compares `img_mire_prog_jpg` to the reference mire;
  `test_jpeg_refusals_and_bounds` still refuses arithmetic JPEG and checks
  the scratch size.

## 2. Not done on this branch

- US-059 `partial`. Local catalogue only (`pm-catalog`, `pm-install`).
  No network marketplace. Not changed in this lot.
- US-060 `partial`, with a `simule` part. Local Schnorr CA, revocation
  list `/etc/pm_ca.crl`, private key sealed with AES-128-GCM
  (`SYS_KEY_SEAL`). The seal key is derived per boot (no TPM), so a sealed
  key does not survive a reboot. Not a public authority. Not changed in
  this lot.
- US-063 `partial`. Distance vector, at most 3 hops, on the lab hub.
  Not changed in this lot.
- US-068 / US-073 `partial`. Adaptive window under hub delay and loss
  (lot 48). Not under extreme load, not on a WAN. Not changed in this lot.
- US-074 `partial` for the codec, `simule` for the NAT. RFC 5389 Binding
  inside the sealed link; QEMU proves `direct`. The NAT is an address
  rewrite on the unit-test bus. `p2p-punch` stays simulated. No public
  STUN server. Not changed in this lot.
- Phase 6-8 offline items beyond the status table. Statuses were not
  changed. US-078 and US-097 stay `simule`.
- Host coverage is measured by `make coverage` (CI floor 78%). It is not
  guest-image coverage and it is not part of `make test-all`. This commit:
  81.91% lines executed (42464/51842), 211 files, 813 `.gcda`. Floor OK.

## 3. Gates

`make test-all` on this commit: 1148 passed, 0 failed, 0 skipped, 67 s
(journal `test_logs/test_results_20261027_235517.log`, 28 October 2026
03:55 Mauritius, UTC+4). `python3 scripts/extract_guest_commands.py --check`
is inside that run (447 names). `make roadmap-check`: OK, 63 entries.
`make strict-image-check` and `make secrets-check`: OK (production initrd
has no test program; no provider secret). `make coverage COVERAGE_MIN=78`:
81.91% (42464/51842). `make qemu-gui-lot49`: passed (explorer enters
`/docs`, maximize reaches the editor in 0.00 s, 48 px drawn at the bottom).
`make integration-qemu-vfs`: passed in 542.2 s, including `vfs-list ok
count 0` on the empty overlay. `make tree-clean-check` runs on the
committed tree.
