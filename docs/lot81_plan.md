# Lot 81 plan - partial CSS layout pieces closed in webeng

Base: main 0e4a307 (lot 80). Single engine userspace/webeng.c; kernel/ABI unchanged.
US-031 stays partial; phase3_complete stays false.

## Scope (first gap from lot 80 first, then other partial pieces)

| Id | Feature | Before | After |
|----|---------|--------|-------|
| F1 | display: inline-flex / inline-grid | treated as block flex/grid | atomic inline box (inline_outer) placed on the line, flex/grid inside |
| F2 | auto-width floats | bounded half container | shrink-to-fit: clamp(max-content, min-content, available) in content space |
| F3 | grid-template-rows: masonry | refused | one row per item, item goes to the shortest column window (leftmost on ties) |
| F4 | calc() with % in gaps | refused | resolved against the container width / definite height |
| F5 | aspect-ratio decimals, `auto <ratio>` | refused | 3-decimal ratios reduced, auto uses natural ratio for replaced elements, fixed-height auto-width boxes size from the ratio |
| F6 | object-position offsets | 2 values only, 0..100% | signed lengths/percent, 3/4-value edge offsets |
| F7 | calc() tracks | refused | linear px+% calc() in templates, minmax floors, fit-content, grid-auto-* (32 slots) |
| F8 | nested subgrid | refused | column subgrid inside a subgrid recurses to the outer tracks |
| F9 | subgrid line-name lists | refused | `subgrid [a] [b]` names appended after inherited names |

## Not done (first gaps for lot 82)

1. min()/max()/clamp() mixing % in grid tracks (non-linear) - still refused with diag.
2. Padding/margin/border on a subgrid (box offsets) - still refused.
3. Masonry on columns (grid-template-columns: masonry) - refused.

## Tests

7 generated suites tests/unit/userspace/webeng_lot81_*.inc (1031 tests); converted
refusal tests in lot52/57/58/61/80 are listed as review points in the PR body.
QEMU: initrd_content/web/lot81.html, tests/scripts/test_qemu_lot81.py, `make qemu-lot81`, CI job lot81-layout.
