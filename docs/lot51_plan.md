# Lot 51 plan (branch cursor/lot51-gui-terminal-d639, from main 839d47a)

Status words: done, partial, missing, simule, non prouvable.
Nothing here is marked done without a test. The desktop keeps one shell.

## 1. Why this shape

Lot 50 put `/pm`, `/proc` and `/mesh` on the existing shell. The inventory
still left three groups on `/cmd`: child results, deploy/rollback, and
collab keys. Lot 51 adds one menu for each.

The slashes are new on purpose. `/p2p` is the desktop pane. `/prod` is
the production pane. `/collab` is the ledger pane. `/ci` is the
production panel from Lot 46. Taking any of those failed CI on Lot 50.

Not in this lot:

- no new shell command;
- no JavaScript VM;
- no native compiler;
- no live mesh and no live ledger;
- US-059, US-060, US-063, US-074 are not advanced;
- pm-ide, top, gui, reboot stay terminal-only.

## 2. Done on this branch

- US-G22 `partial`. `/child` (kind 33) runs child-exit-count,
  child-results, child-result, child-result-any, child-results-observe.
  child-results-clear and child-results-forget need a second Enter.
  QEMU runs child-exit-count and expects `child-exit-count ok`.
- US-G23 `partial`. `/release` (kind 34) runs deploy-verify with no
  argument. The shell prints `deploy-verify error manifest not found`
  and does not write. deploy-apply and prod-restore take a path or a
  name and need a second Enter. prod-rollback needs a second Enter.
  QEMU does not apply a manifest and does not restore a backup.
- US-G24 `partial`. `/keys` (kind 35) runs collab-keys. While the node
  is down the shell prints `collab-keys error p2p down (p2p-up first)`
  before any key list. collab-contract-deploy needs arguments and a
  second Enter. Other collab-contract-* commands stay on `/cmd`.

## 3. Roadmap

`/roadmap.txt` gains US-G22, US-G23, US-G24, all `partial`. prod-roadmap
prints:

- phase 9 done 18 partial 6 simule 0 open 0 progress 75%
  (18 * 100 / 24 = 75);
- next US-076 US-078 US-080 US-081 US-097 US-G19 US-G20 US-G21
  US-G22 US-G23 US-G24;
- entries 69 bad 0.

## 4. Not done

- kill and the rest of task-* from a new window;
- a deploy that is proved to copy files from the window;
- a rollback that finds a pre-deploy backup;
- collab-join, a key fingerprint list with the node up;
- collab-contract-create, collab-contract-exec;
- `.jpeg` suffix;
- a second copy of the shell.
