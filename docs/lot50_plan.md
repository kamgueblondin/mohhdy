# Lot 50 plan (branch cursor/lot50-gui-terminal-d639, from main 6936711)

Status words: done, partial, missing, simule, non prouvable.
Nothing here is marked done without a test. This lot does not copy the
shell into the GUI. A menu of existing commands is the whole feature.

## 1. Why this shape

The desktop and the terminal already share `handle_line` (`userspace/shell.c`),
the registry (`shared/multiboot_shell_commands.json`), `fsops` and `imgdec`.
A second implementation of PromptMessage, processes or the mesh would be
a regression of that rule, and it would not be a 30000-line feature: it
would be a second product. Lot 50 adds three panels and fixes the SOF2
contract the previous head left red.

Not in this lot, on purpose:

- no new shell command (the guest command check stays at the same count);
- no JavaScript VM (`phase3_complete` stays false);
- no native code generator (`pm-cross` stays C99 or Python 3);
- no language model behind `pm-assist` (still rules);
- US-059, US-060, US-063, US-074 are not advanced;
- pm-ide, top, gui, console, exit, reboot stay terminal-only;
- the mesh panel does not call `p2p-up`.

## 2. Done on this branch

- GUI parity regression. Lot 49 decodes Huffman SOF2, so `img-info` on
  `/images/mire-prog.jpg` prints `img-info ok fmt=jpeg w=48 h=32 bpp=24`.
  `tests/scripts/test_qemu_gui_parity.py` still waited for
  `reason=variante non supportee`. The check now expects success, and
  `/view /images/mire-prog.jpg` must draw pixels. Arithmetic SOF9 stays
  refused in `test_imgdec` (no arithmetic sample in the initrd). US-G03
  stays `done`.
- Explorer. `osui_path_is_image` treats `.jpg` like `.png` / `.bmp` /
  `.pgm`. A double-click in the file window opens the shared viewer.
  `.jpeg` (five letters) is not recognized. Proof: `test_lot50_entries`.
- US-G19 `partial`. `/pm` (kind `OS_FB_APP_PM`, 30, not `/pmrun` which
  stays kind 7 and is titled PromptMessage). Entries: pm-catalog,
  pm-catalog-search, pm-doc, pm-check, pm-disasm, pm-assist, pm-cross,
  pm-test, pm-versions. The window title is `Outils PM` so it does not
  collide with the launcher. QEMU runs `pm-catalog` and expects
  `pm-catalog ok`.
- US-G20 `partial`. `/proc` runs ps, mem, task-capacity, task-events,
  task-events-notify-status, task-priority-child-status,
  task-events-budget-status, task-events-watch-status, children.
  task-priority, task-suspend and task-resume append the input line and
  need a second Enter. QEMU runs `ps` and expects the banner
  `Processus`. kill and the rest of task-* / child-* stay on `/cmd`
  or `/tasks`.
- US-G21 `partial`. `/mesh` (kind `OS_FB_APP_P2PMESH`, 32, not
  `OS_FB_APP_P2P` 6 which is `/peers`) runs p2p-stats, p2p-analyze,
  p2p-route, p2p-qos, p2p-raft, p2p-stun, p2p-cache, p2p-kv. A second
  p2p-cache entry takes `put CLE VALEUR` and confirms. QEMU runs
  p2p-stats while the node is down and expects
  `p2p-stats error p2p down`. That is the shell's own line
  (`userspace/shell_p2p.c`). It is not a live mesh.
  The slash is `/mesh` because `/p2p` already opens the desktop pane
  (`pane=p2p`, `qemu-osui-gui` and `qemu-gui-apps`). Taking `/p2p`
  for the menu made those two jobs fail on CI run 40610642239.

## 3. Roadmap

`/roadmap.txt` gains three phase 9 lines. prod-roadmap therefore prints:

- phase 9 (gui-parity) done 18 partial 3 simule 0 open 0 progress 85%
  (18 * 100 / 21 = 85);
- next US-076 US-078 US-080 US-081 US-097 US-G19 US-G20 US-G21
  (file order of every status that is not done; US-081 is partial and
  US-097 is simule, already in the file before this lot);
- entries 66 bad 0.

`make roadmap-check` compares those statuses with the US master table.
`tests/integration/test_qemu_production.py` expects the same strings.
This plan does not claim `make qemu-production` passed unless the run
log says so.

## 4. Measured on 28 October 2026

- `make test-all`: 1149 passed, 0 failed, 0 skipped, 64 s. Journal
  `test_logs/test_results_20261028_003631.log` (04:36 Mauritius, UTC+4).
- `make tree-clean-check`, `make roadmap-check` (66 entries),
  `make secrets-check` (no provider secret in 4 images),
  `make strict-image-check`: passed. Guest command registry: 447 names.
  `scripts/check_gui_parity.py`: families=24 entries=148.
- `make qemu-gui-parity`: passed (about 248 s) while the mesh menu was
  still `/p2p`. That slash is now `/mesh`.
- CI run 40610642239 (`d54e2c0`): gui-parity passed. `qemu-production`
  printed the longer next line (US-081 and US-097 included). The test
  expected a string that skipped those two ids, so the job failed.
  `qemu-osui-gui` and `qemu-gui-apps` failed because `/p2p` no longer
  opened the pane.
- Re-run after the `/mesh` fix (journal
  `test_logs/test_results_20261028_025410.log`, 28 October 2026,
  06:54 Mauritius, UTC+4): `make test-all` 1149 passed, 0 failed,
  0 skipped, 114 s. `make qemu-gui-parity` passed (`/mesh` prints
  `p2p-stats error p2p down`). `make qemu-gui-apps` passed (the `/p2p`
  pane draws again). `make qemu-production` passed (the full next line).
  `make qemu-osui-gui` passed (`pane=p2p`, then ESC, shell, and the
  terminal window).

## 5. Not done

- pm-ide inside a window (it owns the keyboard);
- every task-* and child-* command;
- p2p-up, p2p-send, elections, NAT punch, a second guest;
- deploy-apply, prod-rollback, prod-restore, collab-keys (still `/cmd`);
- `.jpeg` suffix;
- flex-grow, grid areas, a JavaScript VM;
- measured coverage is not part of `make test-all` (host `make coverage`
  only, floor 78%).
