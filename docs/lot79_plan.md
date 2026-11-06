# Lot 79 plan

Base: local cursor/lot78 HEAD 3b453e1 (lot 78 PR in flight on main
b7f7c0e after lot 77 merge; not merged yet). Branch: cursor/lot79.

## Chosen items (from lot 78 first gap)

### A. grid-auto-flow dense packing
Engine already places with grid_dense (cursor resets to 0). Lot 79
systematically proves dense vs sparse row/column flow, span holes,
and refusal of bogus flow tokens (diag).

### B. grid-auto-rows / grid-auto-columns
Prove px, fr, minmax(floor, cap), auto for implicit tracks when items
overflow the explicit template; column-flow with auto-columns.

### C. Space distribution in grid
justify-content / align-content: space-between, space-around,
space-evenly (and start/end/center contrast) with free space in fixed
grid boxes. place-content two-keyword forms included.

### D. Richer flex/grid edges
min-width / max-width / min-height / max-height on grid and flex
children; minmax() in template tracks; flex item min/max beside a
grid sibling. No masonry.

Out of scope: masonry, animation, JS VM, Chromium/WebKit, new font
engine, margin collapsing, changing the dense algorithm itself.

## Constraints
Single engine webeng.c + .inc; freestanding i386 Ring 3; we_doc_t < 1 MiB;
7-bit ASCII; US-031 partial / phase3_complete false; mutations; no DNS
in host tests.

## Delivery
- Code: only if a proved gap needs a fix; else proof-only on existing
- Tests: webeng_lot79_*.inc
- QEMU: /web/lot79.html, test_qemu_lot79.py, make qemu-lot79, CI
- /workspace/run/lot79/ deliverables; mbox is 3b453e1..HEAD only
