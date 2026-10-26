# GUI parity: terminal command families in the desktop (UX5)

The shell registry (`shared/multiboot_shell_commands.json`, 443 commands)
is the reference. This file maps each terminal command family to its
desktop surface. Status: **done** (the family's everyday use works from a
window, tested in QEMU), **partial** (some commands have a window, the rest
only through `/cmd` or the terminal), **todo** (terminal only, or `/cmd`).

## Rights model of the desktop tools (since UX5)

Before UX5 the tool windows ran a fixed allowlist (ps, mem, kill, ...).
Now (`osui_tool_cmd_check`, `shell_cmd_known` in `userspace/shell.c`):

- a window may run any command of the shell registry (the shell builtins
  or the generated `mohhdy_shell_commands` list), with its arguments;
- refused: shell metacharacters (; | & > < backquote $), unknown names, and
  terminal-only commands that read the keyboard in a loop, own the screen,
  end the session or nest a desktop / AI runtime (gui, console, exit, quit,
  logout, clear, top, edit, pm-ide, reboot, shutdown, login, su, passwd,
  watch, sleep, osui-* except osui-audit, ...). The refusal names the reason;
- rights are the kernel's: the command runs in the shell task through the
  same `handle_line` as a typed command, so a protected PID, a read-only
  path or a missing capability is refused exactly as in the terminal. The
  window adds no privilege and removes none;
- panel entries that change state (marked `*`, flag MUT) need a second
  Enter (armed, then run). The generic `/cmd` window runs what the user
  typed, like the terminal, without a second confirmation.

Honest limits: the deny list is a list, not a property of each command; a
registry command that blocks (waits on the network for a long time) also
blocks the desktop until it returns. Since Lot 46 the output of a program
the command launches (spawn, exec) is captured by the kernel
(`SYS_OUT_CAPTURE`, 16 KiB and 16 tasks per run, only descendants of the
shell created during the run, rights unchanged) and shown in the window
after the shell's own text; past 16 KiB a note gives the bytes dropped, a
program still running after a bounded wait of 50 ticks is named and its
later output goes to the console. Window output is cut at 64 KiB per run
(note in the window) and the window keeps its last 128 lines.

## Panels (userspace/osui_parity.c, checked by scripts/check_gui_parity.py)

Each panel is a menu of real command lines; Enter runs the selected one,
the input line gives the argument (ARG), Esc goes back to the menu then
closes. Open them from the desktop input: `/net`, `/disk`, ...

| Panel | Family | Commands (MUT = confirm twice, ARG = input line) |
|---|---|---|
| /net Reseau | network | `net-status`, `net-wire-status`, `net-relay-status`, `p2p-health`, `p2p-peers`, `p2p-up` (ARG+MUT) |
| /disk Stockage | storage | `ata-status`, `mount-journal`, `persist-status`, `ls` (ARG), `stat` (ARG), `mkdir` (ARG+MUT), `persist-save` (MUT, Lot 46), `fat16-list` (Lot 46) |
| /services Services et pilotes | services | `devices`, `lspci`, `dev-list`, `jobs`, `task-summary`, `driver-restart` (ARG+MUT), `service-status` (ARG), `service-find` (ARG) |
| /logs Journaux | logs | `task-events`, `notify-list`, `osui-audit`, `history`, `prod-usage`, `prod-log-analyze` (ARG), `prod-log-append` (ARG+MUT), `notify-push` (ARG+MUT, Lot 46), `notify-ack` (ARG+MUT, Lot 46) |
| /ledger Registre collab | collab | `collab-balances`, `collab-tasks`, `collab-audit`, `p2p-up` (ARG+MUT), forms `collab-offer`, `collab-vote`, `collab-escrow-create`, `collab-escrow-release` (MUT) |
| /ci Production | prod | `prod-diag`, `prod-alerts`, `prod-backups`, `prod-scale-status`, `prod-metrics`, `prod-sample` (MUT), `deploy-verify` (ARG since Lot 46: the manifest), `prod-integrity` (ARG), `deploy-make` (ARG+MUT, Lot 46), `prod-backup` (ARG+MUT, Lot 46) |
| /security Securite et CA | security | `whoami`, `id-key`, `origin-check`, `pm-ca-verify` (ARG), `pm-ca-init` (MUT), `pm-ca-sign` (ARG+MUT), `pm-ca-revoke` (ARG+MUT, Lot 46) |
| /search Recherche | search | `grep -r` (ARG), `grep` (ARG), `ls` (ARG) |
| /devtools Outils web | web | `browser-status`, `browser-tabs`, `web-perf`, `web-cache`, `web-dom` (ARG), `web-console` (ARG), `web-inspect` (ARG, Lot 46), `web-a11y` (ARG, Lot 46) |
| /notes Notes et archives | notes | `ls /`, `cat` (ARG), `append` (ARG+MUT), `migrate-export` (ARG+MUT), `migrate-verify` (ARG), `migrate-import` (ARG+MUT, Lot 46) |
| /vfs Virtuel VFS (needs the vfs server: `spawn bin/vfsserver` in /cmd) | vfs | `vfs-stats`, `vfs-list` (ARG), `vfs-stat` (ARG), `vfs-read` (ARG), `vfs-mount-add` (ARG+MUT), `vfs-mount-remove` (ARG+MUT) |
| /sessions Sessions | sessions | `session-status`, `session-list`, `session-new` (ARG+MUT), `session-use` (ARG+MUT), `session-restore` (ARG+MUT), `session-cleanup` (MUT), `session-end` (MUT) |
| /mcp MCP | mcp | `mcp-status`, `mcp-list`, `mcp-credentials`, `mcp-invoke` (ARG+MUT), `mcp-invoice` (ARG+MUT), `mcp-invoice-void` (ARG+MUT) |
| /media Media | media | `browser-media`, `media-play` (ARG+MUT), `media-pause` (MUT), `media-stop` (MUT), `media-stream` (ARG+MUT) |
| /grpc gRPC Wire | grpc | `grpc-wire` (ARG) |
| /pwa PWA | pwa | `pwa-list`, `pwa-manifest` (ARG), `pwa-install` (ARG+MUT), `pwa-run` (ARG+MUT) |
| /devsync DevSync | devsync | `devsync-show`, `devsync-sync` (MUT), `devsync-set` (ARG+MUT), `devsync-del` (ARG+MUT) |
| /sysadmin Administration | admin | `os-admin`, `os-status`, `admin-status`, `admin-all`, `os-support` |
| /api API Tokens | api | `api`, `api-token` (ARG+MUT), `api-revoke` (ARG+MUT) |
| /cmd Commande | any | any registry command typed in the input line |

## Lot 45

- `/help` in the chat opens the Aide window: the slash programs (from the
  dispatch table itself) and the terminal `help`.
- Every tool window and the file explorer follow their window grid
  (US-G15); the viewer decodes BMP and PNG (shared `imgdec`, also
  `img-info` in the shell) with zoom, fit and pan; clipboard 64 KiB.
- Not done in Lot 45: JPEG, output of spawned programs (done in Lot 46),
  US-001, Raft partition, TCP ports, overlay files over 30 KiB. Details:
  `docs/lot45_plan.md`.

## Lot 46 (PR A)

- Later in the branch: a refused Save As restores the previous file name
  (title); /net still has no interface configuration (US-G04 partial: no
  shell command exists to reuse).

- Output of launched programs in the window (kernel capture, above).
- The file explorer name column follows the window width; output already
  shown in a tool window is wrapped again when the window is resized.
- driver-restart of atadriver and networker asserted in QEMU (new pid and
  working service), after pcidriver in Lot 45.
- `make qemu-gui-panels`: every entry of /net /disk /services /logs /ci
  /security /devtools /notes (64 runs) through the window menu.
- Status per item and the 4 entries left partial: `docs/lot46_plan.md`.

## Lot 46 (before the merge: review fixes and PR B)

- A driver relaunched from a window leaves the capture (no "programme
  encore actif" for networker); the capture is freed when its shell dies.
- Editor saves up to 64 KiB (overlay image v4, extension area after the
  FAT16 volume), byte exact after a reboot: `make qemu-gui-bigsave`.
- Still partial: viewer (no JPEG, no interlaced PNG), /net (no interface
  configuration command in the shell).

## Inventory per family

| Family (commands) | GUI surface | Status |
|---|---|---|
| files: ls, cat, cp, mv, rm, mkdir, touch, stat (about 20) | Files explorer (UX3), editor, /disk, /notes | done |
| editor: edit, pm-ide, pm-edit | Editor window (heap, window-sized grid since UX5) | done: lines up to 256 columns with horizontal scroll (Lot 45); saves up to 64 KiB on a disk with the v4 extension area (Lot 46, `qemu-gui-bigsave`), 30 KiB otherwise |
| processes: ps, kill, mem, jobs, task-* (27), child-* (7) | Task manager (/tasks), /services (jobs, task-summary) | partial: task-events-*, task-priority, task-suspend only through /cmd |
| services and drivers: service-* (6), devices, lspci, dev-list, pci-rescan, driver-restart | /services, task manager services view | done (Lot 46): every /services entry runs in `qemu-gui-panels`; `driver-restart` (fixed Ring 3 driver names): supervised respawn of pcidriver (Lot 45) and atadriver with a new pid, networker restarted by the shell with a new pid, each service used after the restart (`qemu-gui-parity`) |
| network: net-* (3), fetch-sim, grpc-* (6) | /net, /grpc | partial: every /net entry runs in `qemu-gui-panels` (Lot 46), grpc-wire; the shell has no command that configures the interface (address, route), so no window can either (Lot 46 PR D) |
| p2p: p2p-* (27) | /peers (UX4), /net | partial: peers, stats, up, down, send; kv/raft/qos only through /cmd |
| storage: ata-*, fat16-*, mount-*, persist-* (6), vfs-* (26) | /disk, /vfs | done (Lot 46): every /disk entry runs in `qemu-gui-panels` (ata, mounts, persist status and save, ls, stat, mkdir, fat16-list); vfs-stats/list/stat/read/mount-add/mount-remove in /vfs; the vfs-backend-* probes are test tools, through /cmd |
| logs: task-events, notify-*, osui-audit, history, prod-log-*, prod-usage | /logs | done (Lot 46): every /logs entry runs in `qemu-gui-panels` (append then analyze a log, push then acknowledge a notification) |
| collab ledger: collab-* (37) | /ledger | done (US-G08 since Lot 45): balances, tasks, audit, p2p-up; forms (offer, vote, escrow create, release/refund/dispute) build the shell grammar and need a second Enter; QEMU proof of the offer form. Contracts and keys (collab-contract-*, collab-keys) through /cmd |
| production / CI: prod-* (28), deploy-* (3), cicd | /ci | done (Lot 46): every /ci entry runs in `qemu-gui-panels` (diag, alerts, scale, sample then metrics, deploy-make then deploy-verify, integrity baseline then check, backup then backups); restore, rollback, deploy-apply through /cmd |
| security / CA: whoami, id-key, cap-token, pm-ca-*, origin-* | /security | done (Lot 46): every /security entry runs in `qemu-gui-panels` (init, sign, verify, revoke, verify refused); the CA is a local Schnorr CA (no X.509, no public authority) |
| web devtools: web-* (13), browser-* (34) | /devtools, Browser window (UX4) | done (Lot 46): every /devtools entry runs in `qemu-gui-panels` (status, tabs, perf, cache, DOM, console, inspect, a11y); navigation in the Browser window |
| search: grep (with UX5 grep -r) | /search | done: recursive, whole files up to 1 MiB through the shell heap (Lot 45; cut files counted beyond), pages of 64 hits (`grep -r MOTIF [REP] [PAGE]`) |
| notes / archives: append, cat, migrate-* (3) | /notes | done (Lot 46): every /notes entry runs in `qemu-gui-panels` (append, cat, export, verify, import); plain text only, no rich text |
| PromptMessage: pm-* (27) | /pmrun, editor ESC r | partial: run, check, edit; catalog/CA through /security or /cmd |
| AI: ai-* (22), aistats, chat | /chat (GPT-2 streaming) | partial |
| system: date, uptime, hal-*, rtc-check, screen-adapt, power-* | /settings | partial: read only |
| sessions, mcp, media, pwa, devsync, os-*, admin-*, api-* | /sessions, /mcp, /media, /pwa, /devsync, /sysadmin, /api | partial: panels on the registry (table checked in test-all), argument hints match the commands; qemu-gui-parity runs the first entry of each, /grpc and /devsync with an argument (Lot 45); media is simule |
| terminal-only: gui, console, exit, top, clear, reboot, shutdown, login | none by design | n/a |

## Tests

- Unit: `test_osui_parity` (rights model, table vs registry, menu, ARG,
  MUT confirmation, /search, /cmd refusals), `test_osui_gui`
  (`test_gui_parity_panels`), `test_fsops` (`test_grep_recursive`).
- QEMU: `make qemu-gui-parity` (CI job gui-parity): every family panel
  runs its first entry with real output, /search finds lines with grep -r,
  /disk mkdir needs the second Enter, /cmd runs uptime and refuses reboot;
  Lot 46: output of spawned programs (16 KiB bound and note),
  driver-restart of atadriver and networker.
- QEMU: `make qemu-gui-panels` (CI job gui-panels, Lot 46): every entry of
  /net /disk /services /logs /ci /security /devtools /notes.
- QEMU: `make qemu-gui-ux5`: Lot 46 rewrap of shown output and explorer
  columns on maximize.
- Unit (Lot 46): `test_out_capture`, `test_osui_tools`
  (test_spawn_note, test_output_rewraps_at_resize), `test_osui_files`
  (test_columns_follow_window_width), `test_osui_parity`
  (test_lot46_entries).
- `scripts/check_gui_parity.py` (in `make test-all`): every panel command
  is in the registry.
