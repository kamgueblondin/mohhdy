# Lot 129: the CSS-wide keywords on the layout properties

Base: origin/main d644827 (PR 287, Lot 128). One engine: userspace/webeng.c.
Kernel, rights and ABI unchanged; webeng.h unchanged: we_node_t (4692
bytes) and we_doc_t (875332) are unchanged, under the 1 MiB bound.
US-031 partial, phase3_complete=false.

## Scope and choice

The "Not covered" list of Lot 128 names flex: inherit, unset, revert.
Probed on d644827 (640 x 480, 8 px glyphs, 10 px lines), the gap is wider:
the four CSS-wide keywords (CSS Cascade 4, 7.3: inherit, initial, unset,
revert) are refused or ignored on every layout property of the flex,
alignment, gap and box sizing families:

1. Refused with a diagnostic, the earlier value kept: flex (inherit,
   unset, revert), flex-grow, flex-shrink, flex-basis, flex-direction,
   flex-wrap, flex-flow, order, justify-content, align-content,
   align-items, align-self, justify-items, justify-self, place-content,
   place-items, place-self, gap, row-gap, column-gap, height, min-width,
   max-width, min-height, max-height, box-sizing. Examples: flex: inherit
   under a flex: 2 3 4px parent gave an 8 px item next to a 292 px one
   (now 150 and 150); flex-flow: inherit under a column gave a row;
   max-width: 30px then max-width: revert stayed 30 px (now 100);
   box-sizing: inherit stayed content-box (60 instead of 50).
2. Ignored without a diagnostic: width (width: inherit kept auto, 8
   instead of the parent's 100; width: 50px then width: initial stayed
   50 instead of auto).
3. flex: initial was already supported (Lot 121, 0 1 auto).

Style sheets reset and inherit these properties routinely (all-reset
sheets, component libraries: flex: initial / unset, width: inherit,
box-sizing: inherit is the classic "inherit box-sizing" pattern), so the
gap renders real pages wrongly. It is the most useful layout gap left
after Lot 128 inside the flex / grid / box sizing scope.

## Rules (CSS Cascade 4, 7.3)

- inherit: the parent's computed value. The engine stores computed
  values (lengths in px, percentages as percentages, math expressions in
  px and percent only), so the parent's fields are copied as they are; a
  percentage then resolves against the child's own containing block
  (height: 50% inherited under a 100 px parent of a 200 px box gives 50).
  Every element has a parent node (top-level elements have the document
  node, which keeps the style reset); a node without a parent would take
  the initial values.
- initial: the initial value. flex-grow 0, flex-shrink 1 and flex-basis
  auto are stored as flex: initial stores them (Lot 121, layout-equal to
  the reset); every other field as the style reset stores it: no width,
  height, minimum or maximum (auto / none), no math expression, no
  intrinsic keyword, content-box, row nowrap, order 0, the alignment
  longhands normal / auto without safe / unsafe, no gap.
- unset: none of these properties is inherited, so unset is initial.
- revert: the user agent sheet of the engine sets none of these
  properties, so revert is initial (author and presentational values go
  back to the user agent origin).
- Keywords are case-insensitive and must be the whole value. revert-layer
  is not covered (the engine flattens @layer, Lot 87): the property
  refuses it as before.

## Implementation

apply_decl calls wide_keyword() right after the logical property mapping
(so inline-size: inherit is width: inherit) and before every property
handler. wide_groups() maps each of the 27 properties to the groups of
fields its value is stored in (a shorthand to several groups: flex to
grow, shrink and basis; flex-flow to direction and wrap; gap to the two
axes; place-* to their two longhands). wide_copy() copies the groups'
fields from a source node: the parent for inherit, else a scratch node
filled by wide_initial(). The groups:

- grow: flex_grow, grow_frac; shrink: flex_shrink, shrink_frac;
- basis: flex_basis, basis_pct, basis_set, basis_content, basis_kind,
  length[WE_LEN_BASIS];
- direction: flex_dir, flex_reverse; wrap: flex_wrap; order: item_order;
- each alignment longhand: its value and its two align_overflow bits;
- each gap axis: row_gap / col_gap, gap_pct, its gap_set bit and its
  length slot. The gap shorthand's single value (gap: 7px) lives in
  n->gap for both axes: gap copies it; a single axis left at -1 resolves
  to the source's value when the two nodes' shorthand values differ
  (column-gap: inherit under gap: 7px gives 7; gap: 9px then
  column-gap: initial gives 0 on that axis);
- width: width, width_pct, width_kind and its slot; height: height,
  height_pct, height_percent, height_intrinsic and its slot; min-width /
  max-width: value, min_pct / max_pct, kind and slot; min-height /
  max-height: value, pct, percent flag and slot; box-sizing: border_box.

No diagnostic is raised for an applied keyword. Other properties keep
their behaviour (grid-template-columns: inherit is still "grid tracks
refused"; color and the inherited text properties were never affected).

## Tests

- Host: tests/unit/userspace/test_lot129_engine.c (9 tests): each
  longhand and shorthand under inherit copies exactly its own fields and
  leaves the others (every basis form, every alignment longhand with its
  safe / unsafe bits, both gap forms, every sizing kind with math slots);
  initial, unset and revert give the initial values with the other
  groups kept; the top-level element, revert-layer, misspelt and
  case-mixed keywords; layout (flex: inherit sharing, flex-flow: inherit
  column, width / height / box-sizing / min-width inherit, initial,
  unset order, justify-content: inherit, an inherited percentage height);
  a second style pass from a container query restarts from the reset.
  All 9 fail on d644827.
- Converted (review point): test_lot121_engine.c's refused flex list held
  "inherit" and "unset"; they are now applied (tested in
  test_lot129_engine.c) and are replaced in that list by "revert-layer"
  and "inherits", still refused.
- QEMU: /web/lot129.html, `make qemu-lot129`
  (tests/scripts/test_qemu_lot129.py), CI job
  lot129-engine-css-wide-keywords: flex: inherit, flex-direction: inherit,
  column-gap: inherit, width and box-sizing inherit, flex: unset,
  justify-content: inherit, max-width: revert, order: inherit; every box
  refuses its d644827 value (NEVER).
- No existing page changes (every initrd web page renders byte-identical
  through the host pipeline, base vs lot 129).

## Not covered (honest)

- revert-layer; a real revert to user agent values for properties the
  user agent sheet sets (none of the 27 is set there).
- The keywords on the other non-inherited properties (grid templates and
  placement, margins, padding, borders, insets, display, position,
  overflow, aspect-ratio...) keep their current handling.
- all: initial / unset in the page engine.
- n->gap, flex_dir, flex_wrap, min_pct and max_pct are not cleared by the
  style reset itself (pre-existing; a second style pass keeps an earlier
  value of these fields unless a declaration sets them).
- Vertical writing modes for left / right and self-*, legacy keywords,
  grid overflow default, wrap-reverse column baselines, the aspect-ratio
  transferred minimum, calc-size, anchor, typed attr, margin collapsing.
