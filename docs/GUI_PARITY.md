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
blocks the desktop until it returns. Output of a command that spawns a
separate ELF child is not captured (only the shell's own output is).
Captured output is cut at 4 KiB per run.

## Panels (userspace/osui_parity.c, checked by scripts/check_gui_parity.py)

Each panel is a menu of real command lines; Enter runs the selected one,
the input line gives the argument (ARG), Esc goes back to the menu then
closes. Open them from the desktop input: `/net`, `/disk`, ...

| Panel | Family | Commands (MUT = confirm twice, ARG = input line) |
|---|---|---|
| /net Reseau | network | `net-status`, `net-wire-status`, `net-relay-status`, `p2p-health`, `p2p-peers`, `p2p-up` (ARG+MUT) |
| /disk Stockage | storage | `ata-status`, `mount-journal`, `persist-status`, `ls` (ARG), `stat` (ARG), `mkdir` (ARG+MUT) |
| /services Services et pilotes | services | `devices`, `lspci`, `dev-list`, `jobs`, `task-summary`, `driver-restart` (ARG+MUT), `service-status` (ARG), `service-find` (ARG) |
| /logs Journaux | logs | `task-events`, `notify-list`, `osui-audit`, `history`, `prod-usage`, `prod-log-analyze` (ARG), `prod-log-append` (ARG+MUT) |
| /ledger Registre collab | collab | `collab-balances`, `collab-tasks`, `collab-audit`, `p2p-up` (ARG+MUT) |
| /ci Production | prod | `prod-diag`, `prod-alerts`, `prod-backups`, `prod-scale-status`, `prod-metrics`, `prod-sample` (MUT), `deploy-verify`, `prod-integrity` (ARG) |
| /security Securite et CA | security | `whoami`, `id-key`, `origin-check`, `pm-ca-verify` (ARG), `pm-ca-init` (MUT), `pm-ca-sign` (ARG+MUT) |
| /search Recherche | search | `grep -r` (ARG), `grep` (ARG), `ls` (ARG) |
| /devtools Outils web | web | `browser-status`, `browser-tabs`, `web-perf`, `web-cache`, `web-dom` (ARG), `web-console` (ARG) |
| /notes Notes et archives | notes | `ls /`, `cat` (ARG), `append` (ARG+MUT), `migrate-export` (ARG+MUT), `migrate-verify` (ARG) |
| /vfs Virtuel VFS | vfs | `vfs-stats`, `vfs-list` (ARG), `vfs-stat` (ARG), `vfs-read` (ARG), `vfs-mount-add` (ARG+MUT), `vfs-mount-remove` (ARG+MUT) |
| /sessions Sessions | sessions | `session-status`, `session-list`, `session-new` (ARG+MUT), `session-use` (ARG+MUT), `session-restore` (ARG+MUT), `session-cleanup` (MUT), `session-end` (MUT) |
| /mcp MCP | mcp | `mcp-status`, `mcp-list`, `mcp-credentials`, `mcp-invoke` (ARG+MUT), `mcp-invoice` (ARG+MUT), `mcp-invoice-void` (ARG+MUT) |
| /media Media | media | `media-status`, `media-play` (ARG+MUT), `media-pause` (MUT), `media-stop` (MUT), `media-stream` (ARG+MUT) |
| /grpc gRPC Wire | grpc | `grpc-wire` (ARG) |
| /pwa PWA | pwa | `pwa-list`, `pwa-manifest` (ARG), `pwa-install` (ARG+MUT), `pwa-run` (ARG+MUT) |
| /devsync DevSync | devsync | `devsync-show`, `devsync-sync` (MUT), `devsync-set` (ARG+MUT), `devsync-del` (ARG+MUT) |
| /sysadmin Administration | admin | `os-admin`, `os-status`, `admin-status`, `admin-all`, `os-support` |
| /api API Tokens | api | `api`, `api-token` (ARG+MUT), `api-revoke` (ARG+MUT) |
| /cmd Commande | any | any registry command typed in the input line |

## Inventory per family

| Family (commands) | GUI surface | Status |
|---|---|---|
| files: ls, cat, cp, mv, rm, mkdir, touch, stat (about 20) | Files explorer (UX3), editor, /disk, /notes | done |
| editor: edit, pm-ide, pm-edit | Editor window (heap, window-sized grid since UX5) | partial: still 72-column lines (not changed in Lot 44); saves up to 30 KiB (overlay chained blocks) |
| processes: ps, kill, mem, jobs, task-* (27), child-* (7) | Task manager (/tasks), /services (jobs, task-summary) | partial: task-events-*, task-priority, task-suspend only through /cmd |
| services and drivers: service-* (6), devices, lspci, dev-list, pci-rescan, driver-restart | /services, task manager services view | partial: status view and `driver-restart` (fixed Ring 3 driver names, respawn by the kernel supervisor; networker restarted by the shell); no QEMU proof yet |
| network: net-* (3), fetch-sim, grpc-* (6) | /net, /grpc | partial: status, wire, relay, p2p-up, grpc-wire; no interface configuration UI |
| p2p: p2p-* (27) | /peers (UX4), /net | partial: peers, stats, up, down, send; kv/raft/qos only through /cmd |
| storage: ata-*, fat16-*, mount-*, persist-* (6), vfs-* (26) | /disk, /vfs | partial: ata, mounts, persist, ls, stat, mkdir, vfs-stats/list/stat/read/mount-add/mount-remove; other vfs-* probes through /cmd |
| logs: task-events, notify-*, osui-audit, history, prod-log-*, prod-usage | /logs | partial |
| collab ledger: collab-* (37) | /ledger | partial: balances, tasks, audit, p2p-up; forms (offer, vote, escrow create, release/refund/dispute) build the shell grammar and need a second Enter; no QEMU proof yet |
| production / CI: prod-* (28), deploy-* (3), cicd | /ci | partial: diag, alerts, backups, scale, metrics, sample, integrity |
| security / CA: whoami, id-key, cap-token, pm-ca-*, origin-* | /security | partial: local Schnorr CA only (no public authority) |
| web devtools: web-* (13), browser-* (34) | /devtools, Browser window (UX4) | partial: status, tabs, perf, cache, DOM, console |
| search: grep (with UX5 grep -r) | /search | done: recursive, first 32 KiB of each file (cut files counted), pages of 64 hits (`grep -r MOTIF [REP] [PAGE]`) |
| notes / archives: append, cat, migrate-* (3) | /notes | partial: no rich text, archive of a directory |
| PromptMessage: pm-* (27) | /pmrun, editor ESC r | partial: run, check, edit; catalog/CA through /security or /cmd |
| AI: ai-* (22), aistats, chat | /chat (GPT-2 streaming) | partial |
| system: date, uptime, hal-*, rtc-check, screen-adapt, power-* | /settings | partial: read only |
| sessions, mcp, media, pwa, devsync, os-*, admin-*, api-* | /sessions, /mcp, /media, /pwa, /devsync, /sysadmin, /api | partial: panels on the registry (table checked in test-all), argument hints match the commands; media is simule; no QEMU test per panel |
| terminal-only: gui, console, exit, top, clear, reboot, shutdown, login | none by design | n/a |

## Tests

- Unit: `test_osui_parity` (rights model, table vs registry, menu, ARG,
  MUT confirmation, /search, /cmd refusals), `test_osui_gui`
  (`test_gui_parity_panels`), `test_fsops` (`test_grep_recursive`).
- QEMU: `make qemu-gui-parity` (CI job gui-parity): every family panel
  runs its first entry with real output, /search finds lines with grep -r,
  /disk mkdir needs the second Enter, /cmd runs uptime and refuses reboot.
- `scripts/check_gui_parity.py` (in `make test-all`): every panel command
  is in the registry.
