# Lot 44 - GUI parity and system debts: review status

Jules delivered Lot 44 as one PR (#201, about 1000 changed lines, not the
30000 lines in 6 stacked PRs of the brief). The first version of this file
claimed every item done; that was not true. This is the reviewed state
after the fix-forward commits on the same branch.

## Per item

| Item | Status | What exists | Tests |
|---|---|---|---|
| PR1 US-G15 window-sized grid for all apps | missing | only the editor follows its window (UX5, unchanged) | - |
| PR1 editor lines over 72 columns | missing | lines still 72 columns | - |
| PR2 US-G16 large files | partial | overlay v3: chained 384-byte blocks, files up to 30 KiB, dense image in the 64-sector AIOV area, transactional write/append/copy, kernel relay in 4 KiB chunks through the 8 KiB atadriver buffer | test_overlay (30 KiB byte exact, read_at, snapshot/restore, refusal keeps old file, broken image refused), test_fsops, QEMU persistence (27993-byte file through the driver) |
| PR2 64 KiB Save As | missing | 64 KiB does not fit the 32 KiB AIOV area; needs a new disk layout (FAT16 base LBA 64) | - |
| PR3 US-G17 panels | partial | /vfs /sessions /mcp /media /grpc /pwa /devsync /sysadmin /api on the shell registry, double Enter for mutations (existing rule) | test_osui_parity (table vs registry); no QEMU per panel |
| PR3 /services driver restart | partial | `driver-restart` for the fixed Ring 3 drivers, kernel rights on SYS_KILL, kernel supervisor respawn | none in QEMU yet |
| PR4 generic form + ledger forms | partial | osui_form: text, list, submit, required fields; collab-offer, collab-vote, collab-escrow-create, release/refund/dispute built in the shell grammar, second Enter, output shown | test_osui_form |
| PR4 capture of commands that spawn a program | missing | output of a separate program is still not captured | - |
| PR4 bounded output buffer | partial | 64 KiB capture buffer with a truncation note; the window keeps the last 128 lines and says how many were dropped | unit (existing panel tests) |
| PR4 /search full file + pagination | partial | 32 KiB per file (was 8 KiB), pages of 64 hits shared by `grep -r` and /search; no chunked read past 32 KiB | test_fsops test_grep_pagination |
| PR5 BMP / PNG decoders, zoom, clipboard over 4 KiB | missing | not in the PR (the old plan text claimed them) | - |
| PR6 US-001 ATA IDENTIFY / FAT PIO out of Ring 0 | not changed | state as before the lot (see ETAT_REEL) | - |
| PR6 US-064 Raft partition / pre-vote / compaction | missing | - | - |
| PR6 HTTPS handshake speedup | missing | - | - |
| PR6 TCP ports honored | missing | - | - |

## Bugs found in the PR and fixed

- Kernel fallback wrote 512 sectors (LBA 0-511) over the FAT16 volume
  (LBA 64+); the atadriver path wrote only 64 sectors of a 241 KiB image, so
  the restore failed and the whole overlay was lost on reboot.
- overlay_write freed the old file before allocating the new one (data
  loss on NOSPACE); append could leave a half extended chain; copy copied
  only the first block of a chained file.
- Files over 8 KiB failed through the Ring 3 driver (single 8 KiB relay).
- Viewer read up to the file size into an 8 KiB static buffer (overflow).
- Forms sent commands the shell rejects (wrong grammar for collab-offer,
  collab-vote, collab-escrow-create); mutations ran on the first Enter.
- driver-restart killed any process whose name contained the argument.
- make test-all did not build (printf in test_overlay); test_osui_form
  asserted nothing useful; a viewer heap-path assertion had been removed.
