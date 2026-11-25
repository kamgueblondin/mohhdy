# Lot 125: true alignment, start / end apart from flex-start / flex-end, stretched grid rows

Base: origin/main 62de290 (PR 283, Lot 124). One engine: userspace/webeng.c.
Kernel, rights and ABI unchanged; we_doc_t (874692 bytes) and we_node_t
(4688) unchanged. userspace/webeng.h gains two values at the end of the
alignment enum (WE_PACK_FLEX_START, WE_PACK_FLEX_END); no struct changes.
US-031 partial, phase3_complete=false.

## Scope and choice

Three out-of-theme findings of Lot 124, probed on 62de290 (640 x 480,
8 px glyphs, 10 px lines):

1. align-self / align-items center or end on a flex item larger than its
   line kept the item at the start (a 60 px item in a 40 px line was at
   y 0), and justify-content center or end on an overflowing line kept it
   at the start too (two unshrinkable 80 px items in 100 px at x 0 and 80).
   CSS Box Alignment 4.4: without a safe keyword the alignment is true
   (unsafe): center overflows both sides, end overflows the start side.
2. start and flex-start were one value (and end / flex-end): in
   row-reverse, column-reverse and wrap-reverse CSS separates them.
   start / end follow the writing mode (left / right, top / bottom), and
   flex-start / flex-end follow the flex axes (the reversed main axis,
   the wrap-reverse cross axis).
3. A grid container stretched as a flex item kept its auto rows at their
   content (a height: 50% child of a grid stretched to 40 was 10): grid
   align-content normal was not stretch in the engine.

## True alignment (CSS Box Alignment 4.4)

true_distance(mode, free, index, count) takes over from pack_distance
when the free space is negative: center gives free / 2 (C truncation:
the odd pixel goes past the end edge), end and flex-end give the whole
free space (the overflow goes past the start edge), start and the
distributed values stay at the start (their own fallbacks are out of
theme). It places a flex item in its line (align-self / align-items,
rows and columns) and the items of a line on the main axis
(justify-content uses the raw negative free space of a line whose
unshrinkable items overflow). Auto margins absorb no negative space and
keep the item at the start. rtl mirrors the result after layout
(rtl_mirror), so end overflows past the left edge there. align-content
and grid alignment keep their safe clamp (out of theme).

## start / end apart from flex-start / flex-end (5.1, 5.2)

parse_pack keeps flex-start and flex-end as their own values. In a flex
container:

- justify-content: the engine lays a reversed main axis out forward and
  mirrors it afterwards; start and end are swapped before the mirror, so
  start packs to the physical start (left / top), flex-start to the
  main-start (right / bottom). rtl row-reverse: start is the right edge.
- align-content: wrap-reverse lines are mirrored in the loop; start and
  end are swapped before, so start packs the lines at the top.
- align-self / align-items: the Lot 124 wrap-reverse mirror inside a line
  now applies to stretch, flex-start and flex-end only; start and end
  follow the writing mode.

In a grid, flex-start / flex-end behave as start / end (pack_distance,
grid_item_width). The column cross fallback (fit-content width) takes
flex-end as it took end.

## Grid align-content normal is stretch (5.1)

prepare_grid: align-content unset (normal) stretches the auto rows as an
explicit stretch did, whenever the grid height is definite: its own
height, or the one a flex line stretches it to (forced height, Lot 124).
So the auto row of a grid stretched to 40 by its flex line is 40, and a
height: 50% child sees 20. Two auto rows of a 100 px grid share the rest
(50 each). Fixed rows keep their size; an explicit align-content packs
as before; an indefinite height has nothing to share. Masonry grids take
their own pass and keep their packing (pinned for the grid rows of
masonry columns; the masonry draft is out of theme). A row subgrid
follows the stretched rows of its parent.

## Review points

- Rendering change: overflowing flex items and lines under center / end;
  start / end in reversed directions and wrap-reverse; auto rows of
  grids with a definite height. The 47 existing initrd pages, compared
  through the host copy of the guest pipeline (62de290 against this lot),
  do not change.
- webeng.h: two enum values appended (WE_PACK_FLEX_START,
  WE_PACK_FLEX_END); the stored values of start / end are unchanged.
- Changed assertions (the theme replaces what they pinned):
  webeng_lot55_flex.inc test_l55_flex_overflow_alignment_is_safe, renamed
  test_l55_flex_overflow_alignment_is_true (same markup, justify-content
  end overflow now at -60 / 20, the line ends on the end edge, the
  diagnostic is still required); webeng_lot56.inc
  test_l56_row_reverse_justify_matrix (start / end values swapped,
  flex-start / flex-end unchanged) and test_l56_column_reverse_justify_matrix
  (start / end values swapped, flex-start / flex-end cases added).
- Files outside webeng.c: webeng.h, tests, Makefile (qemu-lot125),
  ci.yml (job lot125-engine-true-alignment; ci.yml checked with
  yaml.safe_load, no plain name with ': '), run_all_tests.sh,
  initrd_content/web/lot125.html, tests/scripts/test_qemu_lot125.py, docs.

## Tests

- Host: tests/unit/userspace/test_lot125_engine.c: true cross alignment
  (center, an odd overflow, end, flex-end, start, flex-start, a column,
  the fit-content width of a column item under flex-end or end, auto
  margins, padding), true main alignment (center, flex-end,
  space-around fallback, a column, rtl, a line that fits), start against
  flex-start (row-reverse, rtl row-reverse, column-reverse, overflow),
  wrap-reverse (items start / end against flex-start / flex-end, lines
  start, flex-start, end, flex-end, plain wrap), grid rows (a flex
  stretched grid with a percentage child, two auto rows, a fixed row,
  align-content start, flex-end, justify-items flex-end, an indefinite
  height, masonry columns, a row subgrid).
- QEMU: initrd_content/web/lot125.html, tests/scripts/test_qemu_lot125.py
  (make qemu-lot125, CI job lot125-engine-true-alignment).

## Not done

- align-content true alignment when lines overflow the container, and
  grid true alignment (items and tracks) keep the safe clamp; the
  fallbacks of space-between / around / evenly; the safe and unsafe
  keywords are not parsed.
- Grid items are not stretched by default (align-items normal).
- A column flex item under start / flex-start takes the full width
  instead of fit-content.
- Left from Lot 124: column baselines in wrap-reverse keep the start/end
  fallback; the Lot 64 vertical estimate; fractional flex factors; flex:
  inherit / unset / revert; the transferred minimum of aspect-ratio
  items; osui_runtime web-theme keeps the US-041 resolver; calc-size,
  anchor, typed attr(), margin collapsing.
