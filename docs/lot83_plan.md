# Lot 83 plan - spacing math, masonry flow and intrinsic sizing, subgrid auto margins

Base: main dc4f885 (lot 82 + gui-net). Single engine userspace/webeng.c; kernel/ABI unchanged.
US-031 stays partial; phase3_complete stays false.

## Scope (the lot 82 first gap, then nearby features)

| Id | Feature | Before | After |
|----|---------|--------|-------|
| F1 | Auto margins on a subgridded axis | refused ("subgrid box offsets refused") | compute to zero: the subgrid fills its area (CSS Grid 2); the other axis keeps auto margins |
| F2 | Masonry columns: stretch to row height, align-self/align-items | worked but untested | covered (stretch, start, center, end, flex-start, flex-end; container and item; justify-self in masonry rows) |
| F3 | masonry-auto-flow | not parsed | pack or next, optional definite-first / ordered (document order is the placement order); next continues after the previous item's window and wraps; explicit starts win and move the cursor; refused values keep the old value with "masonry auto flow refused" |
| F4 | Masonry-columns container intrinsic widths | sized like an ordinary track list (a floated 30/20/50px two-row container got 50px and an overflow diagnostic) | max-content packs every item at its max-content width (longest row incl. gaps and item margins/border/padding); min-content is the widest item; used by floats, inline-grid, width:max-content/min-content/fit-content |
| F5 | Pooled expression limit (32) | refused beyond 32 per document | WE_TRACK_POOL_MAX 64 and identical expressions are shared (byte dedupe), so repeated rules use one slot |
| F6 | min()/max()/clamp()/calc() with % in margin and padding | margin math silently dropped, padding refused | longhands and shorthands accept math; a fixed result becomes px (padding clamped at 0, range +-2000); a % result is pooled (WE_SPACING_MATH marker) and evaluated against the containing block width at layout (grid area width for grid items) |

## Review points

1. lot82 tests converted: webeng_lot82_extra.inc pool counts (17/64/13; full-pool test redesigned for 64 slots) and webeng_lot82_suboff.inc auto margin refusal tests now assert the zero-margin layout.
2. WE_TRACK_POOL_MAX 32 -> 64 grows we_doc_t by 32 * sizeof(we_length_t); still far under 1 MiB (asserted).
3. we_node_t gains one byte (masonry_next).

## Not done (first gaps for lot 84)

1. masonry-auto-flow definite-first is accepted but has no effect beyond document order (no two-pass placement of definite items).
2. Masonry rows (lot 81) intrinsic height uses the old path; masonry with grid-auto-flow column is not combined.
3. Percentage math in border widths and in absolute insets of masonry items is not covered.

## Tests

7 generated suites tests/unit/userspace/webeng_lot83_*.inc plus webeng_lot83_extra.inc (hand-written).
QEMU: initrd_content/web/lot83.html, tests/scripts/test_qemu_lot83.py, `make qemu-lot83`, CI job lot83-layout.
