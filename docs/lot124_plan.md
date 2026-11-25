# Lot 124: flex lines stretched by align-content normal, wrap-reverse items, place-* normal

Base: origin/main af6e831 (PR 282, Lot 123). One engine: userspace/webeng.c.
Kernel, rights and ABI unchanged; we_doc_t (874692 bytes) and we_node_t
(4688) unchanged. US-031 partial, phase3_complete=false.

## Scope and choice

Three out-of-theme findings of Lot 123, probed on af6e831 (640 x 480,
8 px glyphs, 10 px lines):

1. align-content normal packed the lines of a wrapping flex container at
   the start: two 60 px items in a 100 x 100 wrapping row were 10 high at
   y 0 and 10, where CSS stretches the two lines to 50 (normal behaves as
   stretch in a flex container, CSS Box Alignment 5.1; Flexbox 9.4 step
   15). The items of a wrapping column stayed as wide as their line's
   content (8 px) in a 100 px column.
2. "A stretched item's height is not definite for its children's
   percentages": probes show it already is on af6e831. layout_area
   forces the assigned height (forced_height) and definite_height of the
   item returns it, so a height: 50% child of a row item stretched to 40
   is 20, single line or wrapping, definite container height or not; an
   item that is not stretched (flex-start) leaves the percentage auto.
   The Lot 123 finding was wrong. Nothing to change; the lot pins it
   (test_l124_stretched_percent), including lines stretched by item 1.
   Left: a grid container stretched as a flex item does not stretch its
   own auto row (grid align-content normal is not stretch in the engine),
   so its children's percentages see the content row (out of theme).
3. place-items and place-content refused normal ("place alignment
   refused") although Lot 123 accepted it on the longhands.

Item 1 exposed a gap that has to go with it: wrap-reverse swapped the
lines but not the items inside a line. With tight lines that did not
show; once normal stretches a line, a 10 px item in the 30 px single
line of a wrap-reverse row went to the top of the line (CSS: its
cross-start, the bottom). /web/lot56.html has exactly that item: without
the fix the page changed (y 100 -> 80); with it, no page changes.

## align-content normal (CSS Box Alignment 5.1, Flexbox 9.4 step 15)

layout_flex_tracks: the free cross space goes to the lines when
align-content is stretch or unset (normal), as an explicit stretch did:
each line gets free * (k + 1) / lines - free * k / lines more, stretched
items follow their line. Only flex containers: grid keeps its normal.
A row needs a definite height to have free space (an indefinite height
leaves nothing to share); a wrapping column's width is always definite
(the container's content width). A single line of a wrapping container
is stretched too (a wrapping container is multi-line in CSS); a nowrap
line already is the definite cross size (Lot 123). align-content:
flex-start (or any explicit value) packs as before. The returned content
height of a row keeps its stretch-only term: it only adds space when the
container height is definite, and then the container takes that height
anyway (resolved_height).

## wrap-reverse inside a line (5.2, 8.3)

wrap-reverse swaps cross-start and cross-end, for the lines (Lot 56) and
now for the items in their line: stretch (an item with a definite
height), flex-start and flex-end take the mirrored offset; center does
not move; auto margins are physical and are not mirrored. A row baseline
group sits against the bottom of its line (first baseline) or its top
(last baseline). Column baselines keep their start/end fallback.

## place-items and place-content normal

normal on either value of place-items and place-content is stored as
unset, like the longhands (Lot 123). place-self still refuses it (normal
would mean stretch there, not auto).

## Review points

- Rendering change: lines of wrapping flex containers with a definite
  cross size and no align-content now fill the container (items stretch
  with them); items in the lines of a wrap-reverse container are placed
  from the far edge. The 46 existing initrd pages, compared through the host copy
  of the guest pipeline (af6e831 against this lot), do not change.
- Converted unit tests (markup only, align-content: flex-start, no
  assertion changed): webeng_lot52.inc test_l52_column_wrap_moves_next_column,
  webeng_lot54.inc test_l54_column_wrap_basis_and_grow and
  test_l54_column_wrap_percent_cross_size_once, webeng_lot55_flex.inc
  test_l55_flex_axis_gaps_column_wrap, webeng_lot56b.inc
  test_l56_wrap_reverse_packs_lines_from_the_cross_end,
  test_l56_wrap_reverse_row_gap_mirrors_between_lines and
  test_l56_column_wrap_reverse_mirrors_x, test_lot123_engine.c
  (wrapping column case of test_l123_default_stretch).
- Changed assertion: test_lot123_engine.c test_l123_normal_keyword pinned
  "align-content normal packs a wrapping row" (16x10 at y 0 and 10), the
  gap this lot fills; same markup, now 16x20 at y 0 and 20.
- Files outside webeng.c: tests, Makefile (qemu-lot124), ci.yml (job
  lot124-engine-flex-lines; ci.yml checked with yaml.safe_load, no plain
  name with ': '), run_all_tests.sh, initrd_content/web/lot124.html,
  tests/scripts/test_qemu_lot124.py, docs.

## Tests

- Host: tests/unit/userspace/test_lot124_engine.c: stretched rows (two
  lines, the keyword, an odd remainder, a height, max-height and
  flex-end in the lines, flex-start items, row gap, a single wrapping
  line, an indefinite height, explicit flex-start), stretched columns
  (lines, a width, column gap and margins), wrap-reverse (single and two
  lines, flex-end, center, auto margins, overflow, baseline and last
  baseline groups, a column), percentages in stretched items, place
  normal (place-items one and two values, place-content one and two
  values, place-self refused).
- QEMU: initrd_content/web/lot124.html, tests/scripts/test_qemu_lot124.py
  (make qemu-lot124, CI job lot124-engine-flex-lines).

## Not done

- A grid container stretched as a flex item keeps its auto rows at their
  content (grid align-content normal); its children's percentages see
  that row.
- align-self: center on an item larger than its line is not centered in
  overflow; flex-start/start and flex-end/end are one value in the engine
  (start would not mirror in wrap-reverse in CSS).
- Column baselines in wrap-reverse keep the start/end fallback.
- The Lot 64 vertical estimate (40 px for 30 laid out) is the auto basis
  of such a flex item; fractional flex factors; flex: inherit / unset /
  revert; the transferred minimum of aspect-ratio items; osui_runtime
  web-theme keeps the US-041 resolver; calc-size, anchor, typed attr(),
  margin collapsing.
