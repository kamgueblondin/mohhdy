# Lot 45 - GUI /help, Lot 44 gaps (status per item)

Branch `cursor/lot45`, one PR from `main` after #201. ASCII only. Every
"done" below has a test named next to it; everything else is listed as
partial or not done.

## 0. Lot 44 review fixes restored (critical)

The last Jules commit of #201 (`0f82add`, pushed after the review, merged)
reverted the reviewed fixes: the AIOV overlay image was 241 KB again while
its disk area is 64 sectors (a save could write over FAT16), `view_fill`
read `st.size` bytes into an 8 KB buffer, `driver-restart` killed any
process whose name matched, the ledger forms and the output cut note were
gone, and the matching tests were deleted. Lot 45 puts the reviewed code
and tests back (diff `0f82add..1caff0e`) before anything else.

## Items

| Item | Status | Proof |
|---|---|---|
| P0 `/help` in the GUI chat did nothing visible (os-help text went to the serial line only) | done: `/help` opens the "Aide" window, which lists the slash programs at once (`gui-help`, built from the same table as the chat dispatch, `osui_par_builtins` + parity families); second entry runs the terminal `help` | `qemu-gui-parity` (help_window), `qemu-gui-ux5` (maximize), `test_osui_parity` |
| US-G15 window-sized grid for every app | done: tool windows (viewer, tasks, settings, P2P, PM run, chat, all parity panels) and the file explorer follow `app K size R C`; text rewraps at the new width; menus, viewer, tasks and settings are rebuilt, command output keeps its lines | `test_osui_tools` (test_window_grid_follows_size), `qemu-gui-ux5` (maximized Aide window gets 42 rows) |
| Editor lines beyond 72 columns | done: line store 256 columns (`OSUI_IDE_COLS`), horizontal scroll to the caret, Home back to column 0; longer lines are cut and the file is marked truncated (never saved over) | `test_osui_editor` |
| BMP / PNG decoders shared by shell and viewer | done for BMP 24/32 (bottom-up and top-down, BI_RGB / BITFIELDS 32) and PNG bit depth 8, color types 0 2 3 4 6, no interlace, CRC checked, IDAT split over chunks, bounded inflate (stored, fixed, dynamic Huffman) into a caller-owned 1 MiB scratch; `img-info FILE` in the shell, the viewer, same module `userspace/imgdec.c` | `test_imgdec` (8 tests, samples from `tests/scripts/gen_image_samples.py`), `qemu-gui-parity` (viewer_images: PNG, BMP, img-info) |
| Viewer zoom / fit / pan | done: `+` `-` (25..800 %), `0` 100 %, `f` fit, arrows pan | `test_imgdec` (render), `qemu-gui-parity` (+ gives zoom=200%) |
| JPEG, interlaced PNG, 16-bit PNG | not done (refused with the reason in the window) | - |
| Clipboard beyond 4 KiB | done: explicit bound 64 KiB (`OS_CLIP_MAX`), multi-line, editor copy buffer the same bound | `test_gfx_desktop` (20000-byte multi-line round trip), `test_osui_editor` (test_clipboard_beyond_4k) |
| /search and `grep -r` beyond 32 KiB per file | done: files over `FSOPS_FILE_MAX` are read whole up to 1 MiB through the shell heap (`fsops_set_heap`), not in offset chunks (there is no offset read for initrd files) | `test_fsops` (test_grep_big_file_with_heap) |
| /services driver-restart respawn | partial: pcidriver stop then supervised respawn with a new pid asserted in QEMU (respawn at the next PCI use, `lspci`); atadriver / networker respawn not asserted in QEMU | `qemu-gui-parity` (generic) |
| Ledger forms QEMU test | done: collab-offer form, armed on the first Enter of submit, run on the second, serial trace `osui form armed` / `osui form run cmd=` | `qemu-gui-parity` (ledger_form) |
| /grpc and /devsync QEMU coverage | done | `qemu-gui-parity` (grpc, devsync) |
| Output of commands that spawn a separate program | not done: the child prints on the console, the window only gets the shell's own text | - |
| US-001 (ATA IDENTIFY and FAT PIO fallback out of Ring 0, supervised atadriver restart) | not done in Lot 45 | - |
| US-064 Raft (real partition, pre-vote, log compaction) | not done | - |
| TCP ports honored | not done | - |
| Overlay files over 30 KiB | not done: needs a new disk layout (FAT16 starts at LBA 64), not attempted | - |

## Bounds

PNG scanlines 1 MiB scratch (for example 512x512 RGBA), image sides 4096,
`img-info` files 2 MiB, viewer output 320x240 pixels, clipboard 64 KiB,
grep 1 MiB per file with a heap, editor lines 256 columns, tool windows 48
rows x 128 columns.
