# Lot 82 plan - min()/max()/clamp() percentages, subgrid offsets, masonry columns

Base: main f7b81c7 (lot 81). Single engine userspace/webeng.c; kernel/ABI unchanged.
US-031 stays partial; phase3_complete stays false.

## Scope (the lot 81 first gap)

| Id | Feature | Before | After |
|----|---------|--------|-------|
| F1 | min()/max()/clamp() mixing % in grid tracks | refused (non-linear) | the whole expression is kept in a per-document pool (WE_TRACK_POOL_MAX 32) and evaluated against the track base at layout; templates, repeat(), minmax() floors (comma inside the floor now parsed), fit-content(), grid-auto-* |
| F1b | widths and gaps | already evaluated by the math engine | covered by new tests (width, max-width, grid/flex column-gap, row-gap definite and indefinite) |
| F2 | margin/border/padding on a subgrid | refused | edge offsets (CSS Grid 2, 9.3): the first inherited track loses the start offset and the last the end offset in content coordinates; edge items carry the offset in their track contributions (columns and rows, baseline edges) |
| F3 | grid-template-columns: masonry | refused | items take their shrink-to-fit width and the row window whose running edge is shortest (topmost on ties), explicit row start and row spans honored; auto rows grow to their tallest item; overflow reported |

## Not done (first gaps for lot 83)

1. Auto margins on a subgridded axis (still refused).
2. Masonry columns: items are not stretched to the row height; masonry-auto-flow and intrinsic (shrink-to-fit) sizing of a masonry-columns container are not implemented.
3. Pooled track expressions beyond 32 per document are refused.

## Tests

3 generated suites tests/unit/userspace/webeng_lot82_*.inc (903 tests).
QEMU: initrd_content/web/lot82.html, tests/scripts/test_qemu_lot82.py, `make qemu-lot82`, CI job lot82-layout.
