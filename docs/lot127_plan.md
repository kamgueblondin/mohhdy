# Lot 127: self-start, self-end, left and right, fit-content width of column flex items and grid items

Base: origin/main fdde1fb (PR 285, Lot 126). One engine: userspace/webeng.c.
Kernel, rights and ABI unchanged. userspace/webeng.h gains four alignment
enum values (WE_PACK_SELF_START, WE_PACK_SELF_END, WE_PACK_LEFT,
WE_PACK_RIGHT, appended after WE_PACK_FLEX_END); we_node_t (4692 bytes)
and we_doc_t (875332) are unchanged, under the 1 MiB bound.
US-031 partial, phase3_complete=false.

## Scope and choice

The "Not done" list of Lot 126 names the next gaps of the flex / grid
alignment work of Lots 123 to 126. Probed on fdde1fb (640 x 480, 8 px
glyphs, 10 px lines):

1. A column flex item under align-self: start or flex-start took the
   full width of the container (a 16 px "ab" was 100 wide), unlike end
   and center.
2. The items that did not stretch (end, center, baseline, auto margins)
   took an uncapped max-content width: "aaaa bbbb cccc" under end in a
   40 px column was 112 wide at x -72 while its lines wrapped at 40 (30
   high). Grid items under justify-self: start / end / center did the
   same (112 x 10 in a 40 px area).
3. The items of a wrapping column were sized from one character
   (WE_CHAR_W): under align-content: start each line was 8 px wide and
   the text overflowed it.
4. self-start / self-end and left / right were refused ("alignment
   refused") on every property.
5. The height of a column flex item was measured at the container's
   width, not at the width it is laid out at: an item with width: 30px
   and two lines of text was 10 high (its text overflowed), likewise
   under max-width; with item 1 the same held for every item that no
   longer takes the full width.

These are the most visible remaining gaps of the alignment family on real
pages (a column of buttons or tags under align-items: flex-start is a
common pattern); they share one cause (the hypothetical cross size of a
column flex item, the width of a non-stretched grid item) and one set of
call sites (the positions resolved per axis). Item 5 follows from item
1 (the measurement must use the new width). All five are done.

## fit-content (CSS Flexbox 9.4 step 7, Grid 6.2)

A helper fit_content(node, available) returns min(max-content,
max(min-content, available)), available being a content width (the
container or area width less the item's borders, padding and margins).

- column_cross (prepare_flex, a column): the hypothetical outer cross
  size of an auto-width item is fit-content unless it stretches in a
  single line (align-self stretch or normal, no auto margin on the cross
  axis), where it keeps the whole width (Flexbox 9.8). So start, flex-start, end, flex-end, center, the
  baselines, self-start / self-end and auto margins take their content
  width capped by the container, and wrap inside it.
- A wrapping column sizes every item with fit-content (was one
  character): a line is as wide as its widest item, stretched items
  follow their line, align-content distributes what is left.
- grid_item_width: an item that does not fill its area (any
  justify-self value but normal and stretch: start, end, center,
  flex-start, flex-end, self-start, self-end, left, right, and the
  baseline that place-items: baseline copies to justify-items, which
  filled the area before; auto margins; not a subgrid) takes fit-content
  capped by the area (was max-content).
- The flex base size of a column item (auto basis, content keywords)
  and its automatic minimum (Lot 121) are measured at that outer cross
  size (Flexbox 9.2 step 3E), its own width or max-width included, not
  at the container's width.
- min-content wins over the space: an unbreakable word overflows as
  before (true alignment, Lot 125). Explicit widths, min / max-width and
  rows are unchanged.

## self-start, self-end, left, right (CSS Box Alignment 4.2)

Parsing: parse_position(value, property) accepts self-start / self-end
on align-items, align-self, justify-items and justify-self, and left /
right on justify-content, justify-items and justify-self, after
parse_pack; safe / unsafe go with them, and place-* takes them per value
(the first value is an align-* one, the second a justify-* one; one
value copies to both). Elsewhere they stay refused (left on align-items,
self-start on justify-content or align-content, place-content: left).

Layout: resolve_pack(mode, inline_axis, box, item) turns them into start
or end in the container's layout frame (laid out from the start,
mirrored after for rtl), before every use:

- left / right on the inline axis: left is start in ltr and end in rtl,
  right the opposite; off the inline axis (justify-content of a column)
  they behave as start.
- self-start / self-end on the inline axis: start / end, swapped when the
  item's direction differs from the container's; on the block axis
  plain start / end.
- Call sites: flex justify-content (before the row-reverse swap, so left
  stays on the left), flex items (align-self, the inline axis in a
  column), grid track justify-content, grid items (justify-self inline,
  align-self block), grid_item_width.

## Review points

- webeng.h (outside webeng.c): four enum values appended to the WE_PACK
  enum (12 to 15, stored in the existing uint8_t fields); no struct
  changes size (we_node_t 4692, we_doc_t 875332). Not an ABI: the
  structures are only used by userspace modules built from the same
  sources.
- Rendering change: column flex items that do not stretch, column items
  with their own width or max-width (height measured at it), the items of
  wrapping columns, non-stretched grid items wider than their area, grid
  items under place-items: baseline (justify-items baseline now keeps
  the item's width: baseline falls back to start, CSS Box Alignment 4.2).
  None of the 49 existing pages changes (host pipeline, base vs this
  lot); no page or unit test is converted, no assertion changed.
- Files outside webeng.c: webeng.h, tests/unit/userspace/test_lot127_engine.c
  (new), tests/scripts/run_all_tests.sh, Makefile (qemu-lot127), ci.yml
  (job lot127-engine-self-positions), initrd_content/web/lot127.html,
  tests/scripts/test_qemu_lot127.py, docs.

## Tests

- Host: tests/unit/userspace/test_lot127_engine.c: parsing (every
  longhand, safe / unsafe, refused forms on the wrong properties, place-*
  one and two values with keywords, refused place forms), self-start /
  self-end (flex row block axis with rtl items, flex column ltr and rtl
  containers and items, wrap-reverse, grid inline and block axes),
  left / right (flex rows ltr / rtl, row-reverse, columns and
  column-reverse as start, grid tracks and items ltr / rtl, safe /
  unsafe overflow, align-self: left refused), fit-content (column start,
  flex-start, baseline, last baseline, stretch, normal; end, center, auto
  margin, start wrapping within the container; min-content overflow,
  padding, margin, explicit width; heights measured at the laid out
  width (aspect-ratio children, a percentage width, padding and margin,
  max-width, min-width, a fixed width, flex-basis content and
  max-content, the automatic minimum); wrapping columns with align-content
  start and normal; rows unchanged; grid start, end with padding, auto
  margin, center overflow, self-end, flex-end, left, margin and border,
  stretch / normal / auto against every position, place-items:
  baseline).
- QEMU: initrd_content/web/lot127.html, tests/scripts/test_qemu_lot127.py
  (make qemu-lot127, CI job lot127-engine-self-positions), with NEVER
  values that the base engine gives.

## Not done

- Vertical writing modes: left / right and self-* resolve against the
  horizontal inline axis only; vertical writing modes are not handled or
  tested for them.
- The legacy keywords (legacy left, legacy right, legacy center) and
  anchor-center.
- Left from Lot 126: the overflow default of grid items stays at the
  start; distributed fallbacks; column baselines in wrap-reverse;
  fractional flex factors; flex inherit / unset / revert; the
  aspect-ratio transferred minimum; osui_runtime web-theme on the
  US-041 resolver; calc-size, anchor, typed attr(), margin collapsing.
