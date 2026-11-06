# Lot 78 plan

Base: local cursor/lot77 HEAD 7bbdcae (stacks on lot 77; main may merge
lot 77 separately). Branch: cursor/lot78.

## Chosen items (from lot 77 first gap + layout brief)

### A. place-* shorthands
Engine already has align/justify-items/content/self. Lot 78 adds CSS
place-items, place-content, place-self: one or two pack keywords set the
block and inline axes (second defaults to the first). Invalid values are
refused with a diag; distribution keywords only on place-content.

### B. Grid rows, line placement, item alignment (proved)
Lot 77 locked columns. Lot 78 systematically proves:

1. grid-template-rows: px, fr, repeat(N, ...), row-gap
2. grid-row: span N, N / M, explicit start lines
3. justify-items / align-items / justify-self / align-self in grid
   (start/end/center/stretch where the pack parser allows)
4. place-items / place-content / place-self as above
5. Light flex/grid edges: grid child with align-self; flex container
   beside a grid sibling on the same page

Out of scope: masonry, animation, JS VM, Chromium/WebKit, new font
engine, margin collapsing, changing dense packing algorithm.

## Constraints
Single engine webeng.c + .inc; freestanding i386 Ring 3; we_doc_t < 1 MiB;
7-bit ASCII; US-031 partial / phase3_complete false; mutations; no DNS
in host tests.

## Delivery
- Code: place-* in webeng.c style parse
- Tests: webeng_lot78_*.inc
- QEMU: /web/lot78.html, test_qemu_lot78.py, make qemu-lot78, CI
- /workspace/run/lot78/ deliverables; mbox is 7bbdcae..HEAD only
