# Lot 66 plan: /release in QEMU, Phase 5 hardening (cache, partition heal)

Branch cursor/lot66 from main 36db7b0 (PR #222, lot 65, merged).

## Scope asked, and what was already true before this lot

The request listed, in order: US-G23, US-001 (ATA IDENTIFY and fallback
FAT PIO out of Ring 0, supervised atadriver restart), US-064 Raft (real
partition, pre-vote, compaction), then Phase 5 (US-069, US-070) and known
debts (TCP ports, coverage). Checked against the repository first:

- US-001 (ATA part): already done since Lot 2 and re-proved in Lot 46.
  IDENTIFY and the FAT PIO fallback are not linked in either image
  (`kernel/ata_identify_absent.c`, `kernel/fat_pio_fallback_absent.c`,
  checked by `make strict-image-check`); `make qemu-ata-driver` kills the
  driver, faults it mid-job, respawns it (budget 4) and reboots. Nothing
  left to move for that item, so no new code here.
- US-064: done since Lot 46 (`make qemu-p2p` 2b and 2c: hub wire cut, not
  p2p-block; isolated pre-vote keeps its term; compaction and
  InstallSnapshot; leader partition and re-election). Not reopened.
- US-069 and US-070: closed in Lot 48 in the US master (`make
  qemu-p2p-phase5`); `docs/p2p.md` still said partial. This lot fixes
  real bugs found in them and adds proofs, and aligns `docs/p2p.md`.
- TCP ports ignored: Lot 46 found none (`web-serve` on any port, QEMU).
- Still partial in Phase 5: US-068 / US-073 (no extreme load, no WAN),
  US-063, US-067, US-074 (simule NAT). Not advanced in this lot.

## Done (each with a test)

### US-G23 /release: done

- `make qemu-gui-parity`, new step `release_panel`: files are prepared
  in /cmd, then from /release: deploy-apply (armed, second Enter) gives
  `deploy-apply ok files 1`, deploy-verify `deploy-verify ok files 1`;
  after the source changes, deploy-apply is refused with
  `deploy-apply error failed 1 nothing changed` and the target is
  unchanged; after a healthy prod-deploy (v1 -> v2), prod-rollback from
  the panel gives `prod-rollback ok files 2` and the file reads v1 again;
  after prod-backup and a change to v9, prod-restore NAME from the panel
  gives `prod-restore ok files 2` and the file reads v1.
- Bug fixed: the /release deploy-verify entry took no argument, so it
  could only ever print `manifest not found`. It now takes MANIFESTE
  (like the /ci entry since Lot 46). The unit assertion that pinned the
  old flag is replaced by a stronger one (flag plus built command line).
- New read-only first entry `prod-backups` (an empty argument is refused
  by the panel, so the families check runs that entry).
- Phase 9 of prod-roadmap: done 24 partial 0 (100%) (`qemu-production`).

### US-070 distributed cache: bugs fixed (status unchanged: done since Lot 48)

- A key could be cached twice: `p2p_cache_put` stopped at the first free
  slot before seeing the same key in a later slot; `get` then returned
  the older value. Now: same key, then free slot, then an expired line,
  then the least recently used live line (Raft lines last).
- Committed Raft lines were dropped on any peer down: the guard compared
  `expire_tick` with the TTL constant itself, never equal. Lines are now
  flagged `raft`, kept on peer down and never expire.
- TTL expiry was not wrap safe (`now > expire`); now a signed difference.
- New counters `evictions` and `expired` in `p2p-cache`.
- Proof: `test_p2p` test_lot66_cache_lru_no_duplicate_raft_kept (fails on
  the old code), `make qemu-gui-mesh`: nine lines into the 8-slot cache of
  a live node, the least recently used one (c0) is a miss, c8 is read
  back, `evictions 1 expired 0`, all from /mesh.

### US-069 fault tolerance: partition heal (status unchanged: done since Lot 48)

- A peer that was marked down (cut link) and came back got the cache
  fences, but the key/value writes made on each side during the cut were
  only exchanged after a manual `p2p-sync` (only a rejoin under a new id
  started anti-entropy). Now the down-to-up transition starts the
  two-way digest sync at once, re-sent until the answer arrives;
  counter `heal_syncs` in `p2p-stats`.
- Proof: `test_p2p` test_lot66_heal_starts_anti_entropy (both sides write
  during the cut, both converge with no p2p_sync; fails on the old code),
  `test_lot66_heal_with_raft_snapshot` (the 2b shape: Raft, compaction,
  InstallSnapshot during the heal), `make qemu-p2p` scenario 2c: `p2p-put
  hz side` on beta while the old leader alpha is cut by the hub reaches
  alpha after the heal as `p2p kv synced hz=side v1 from ...` (not a Raft
  entry), no p2p-sync typed on any guest, `heal_syncs` at least 1.
  Note found on the way: in 2b the InstallSnapshot also carries the whole
  key/value store, so 2b alone could not tell the two paths apart.

## Not done

- US-076, US-080, US-081 (phase 6 partial), US-078 and US-097 (simule:
  no sensor hardware, no real payment), US-031, US-068/US-073.
- Real coverage numbers beyond the existing gcov floor job.

## Gates

make test-all, tree-clean-check, strict-image-check, secrets-check,
roadmap-check, scripts/extract_guest_commands.py --check,
qemu-gui-parity, qemu-gui-mesh, qemu-p2p, qemu-production.
