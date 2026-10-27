# Lot 49 plan (branch cursor/lot49, from main ff9d13f)

Status: in progress. Each item says what is done on this branch and how
it is proven; nothing is marked done without a test.

## 1. Done on this branch

- PR C / US-120 `done`: the US master has a phase 6-8 status table
  (US-076..US-120, phase, note, status), copied from
  `initrd_content/roadmap.txt`. `make roadmap-check`
  (`scripts/check_roadmap.py`) now compares every phase 6-9 status and
  phase with the US master tables and fails on any difference (checked by
  hand: flipping US-077 in the master makes it fail). prod-roadmap phase 8
  is now done 15 partial 0 (100%); `make qemu-production` passes with it.
- Lot 48 docs: `docs/lot48_plan.md` section 4 (delivered items, measured TLS
  numbers 1.98 s -> 0.11 s guest time) and phase 5 statuses (US-069,
  US-070, US-075 done; US-068/073 partial).

## 2. Remaining (not started on this branch)

- Phase 5: US-068/073 under extreme load (partial).
- Phases 3-4 per docs/lot48_plan.md: US-031 JS VM / flex-grid, US-055,
  US-058, US-059, US-060, US-063, US-074.
- Phase 6-8 offline-feasible items from the new status table.
- Measured coverage in CI (a gcov floor job exists: "Host unit-test
  coverage (gcov floor)"; the docs must quote its measured figure).
- Progressive JPEG in imgdec.
- GUI: explorer/editor sharing the shell file code and rights, one shared
  shell, real window commands, QEMU usability pass.
