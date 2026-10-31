# Lot 65 plan: GUI panels cover whole terminal command families

Branch cursor/lot65 from main 18f6188 (PR #221, lot 64, merged).

## Why this lot

Lots 57-64 were all CSS layout inside US-031 (still partial: no JavaScript
VM, no network loading, no margin collapsing). The user priority is GUI
usability, and the only partial phase 9 stories were the six panels of
lots 50-51 (US-G19..US-G24): each panel ran a handful of commands of its
family and the rest stayed on /cmd; the mesh and keys panels were only
ever seen with the node down. This lot closes five of them.

## Done (each with a test)

- One rule, checked by a unit test (test_osui_parity:
  test_lot65_families_complete): every shell registry command of the pm-,
  task-, child-, p2p- and collab- families, plus kill, kill-children and
  children, is an entry of a desktop panel, unless the GUI rights model
  makes it terminal-only or it is in a short excluded list with its
  reason. 100+ commands are checked; the test prints any that is missing.
- Excluded, with the reason: wait, wait-result, wait-any-result (they
  block until a child exits; still on /cmd); collab-forge (the forged
  transfer attack hook of the collab tests; still on /cmd).
- Root cause fix: pm-edit (the same line editor as pm-ide, keyboard loop)
  was allowed from a window; it is now terminal-only like pm-ide.
- US-G19 done: /pm has every pm-* command (version, run, debug, say,
  compile, exec, install, catalog-fetch, certify, verify, CA init, seal
  status, sign, verify, revoke). QEMU: pm-version and pm-run from the panel.
- US-G20 done: /proc has every task-* command plus kill and
  kill-children; state changes need a second Enter. QEMU: task-summary,
  task-events-notify on (armed, then confirmed).
- US-G21 done: /mesh has every p2p-* command except p2p-chat (keyboard
  loop). `make qemu-gui-mesh` (new, CI job "desktop panels"): one guest
  with an NE2000 taken by the Ring 3 networker; from the panel only:
  p2p-up (confirmed), p2p-stats, p2p-peers (0 up 0), p2p-health, p2p-qos
  (window 16), p2p-put / p2p-get / p2p-kv, p2p-down, then the down
  refusal.
- US-G22 done: /child has every child-* command (children is in /proc).
- US-G24 done: /keys has every collab-* command not in /ledger (except
  collab-forge). `make qemu-gui-mesh` on the live node: collab-join,
  collab-keys (signing on), collab-task 5 sum 10, collab-work (computed
  sum 10 = 55), collab-auction lamp 7, collab-auctions, collab-export.
- QEMU qemu-gui-parity: the new entries above run through the shell with
  their real output; the existing panel checks are unchanged.
- roadmap.txt, US master table, prod-roadmap expectations: phase 9 done 23
  partial 1 (95%); next list US-076 US-078 US-080 US-081 US-097 US-G23.

## Not done

- US-G23 stays partial: /release has verify/apply/rollback/restore, but
  no QEMU test applies and rolls back a real manifest from the panel.
- US-031 stays partial (no JavaScript VM, network loading, margin
  collapsing). US-001 (Ring 0 removal), US-064 and the phase 6-8 partial
  and simule stories are unchanged.
- The menu is one line per entry; long families (/mesh 34 entries) scroll.
