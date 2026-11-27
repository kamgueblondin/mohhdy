# Lot 130: a style pass starts from the full initial style

Base: origin/main 209af85 (PR 288, Lot 129). One engine: userspace/webeng.c
and userspace/webeng_scroll.inc. Kernel, rights and ABI unchanged; webeng.h
unchanged: we_node_t (4692 bytes) and we_doc_t (875332) are unchanged,
under the 1 MiB bound. US-031 partial, phase3_complete=false.

## Scope and choice

The "Not covered" list of Lot 129 names a pre-existing quirk: style_node's
reset does not clear gap, flex_dir, flex_wrap, min_pct and max_pct. Probed
on 209af85 (640 x 480, 8 px glyphs, 10 px lines) it is a real, wide bug,
not a corner: a style pass after the first one keeps the value that an
earlier pass gave through a rule that no longer matches. The engine runs
such passes in three places: a container query settling on another size
(we_layout_viewport restyles while a container's size changes, at most
WE_CQ_PASSES times), a viewport-unit re-style (layout_once) and
wf_jslayout (we_style again on a parsed document).

A byte comparison of every node, a document laid out wide then narrow
(two style passes, an @container (min-width: 500px) block that stops
matching) against a fresh document laid out narrow (one pass), over 211
declarations (one element, then every element below the container, with
a table, lists, a form and generated content):

- 80 declarations left bytes of an earlier pass on 209af85 (single
  element; more on the rich document), 52 fields in all: margin,
  padding, border, gap, background, flex-direction, flex-wrap, float,
  min-width and max-width in percent, grid templates (columns, rows,
  areas, repeat(auto-fill), calc tracks), box-shadow, outline-color,
  transform, border edge colours, caret-color, accent-color,
  user-select, pointer-events, scroll-margin and -padding, text-
  decoration-color, gradients (count, stops, angle, corner, shape,
  centre, size, repeat), content and counters. 15 changed the layout.
- 0 on this lot's tree.

Examples (width 624, then 284):

1. #t { margin: 10px; padding: 7px; border: 2px solid red } only inside
   the container query: after the narrowing #t stayed at 10 10, 42 x 28
   and its row 48 high (a fresh page: 0 0 24 x 10, row 10 high).
2. #t { width: 200px } with max-width: 30% inside: 85 px wide after the
   narrowing (fresh: 200); min-width: 90% inside: 255 (fresh: 200).
3. #t { display: flex } with flex-direction: column, flex-wrap: wrap and
   gap: 20px inside: the two items stacked 30 px apart in a 40 px high box
   with the diagnostic "column wrap requires height" (fresh: a row, items
   at 0 and 16, 10 high, no diagnostic).
4. One layout call, no resize: a container .y inside a container .x; a
   rule on .x (min-width: 300px) makes .y 100 px wide, and a rule read
   against .y (min-width: 200px) matched .y at its first width. The
   settled page kept that rule's margin, padding, background, border,
   max-width and flex values: #z1 at 30 30, 40 x 20 and red (fresh and
   after this lot: 0 0, 100 x 10). This is what a page with nested
   container queries does when it first loads.

A second gap was found by the same sweep and fixed with it: scroll_paint_x
divides by box_cw, which is 0 for a box that does not scroll along x, when
its view width (the box width minus its padding) is negative. An img is a
fixed 40 x 10 placeholder (24 wide with only an alt text), so any img with
more than 40 px of horizontal padding (img { padding: 0 25px }, or padding:
5% in a 640 px page) stopped the guest with a divide error (SIGFPE on the
host build). No other box reaches it: a block box is at least as wide and as high as
its padding (probed with border-box sizes smaller than the padding, inputs
and flex containers); only the img placeholder is not.

## Fix

- style_reset_declared(n), called from style_node's reset block before
  ua_defaults, hints_apply and the cascade, returns each of the 52 fields
  to its parse-time value. Every value is 0 (the fields hold no other
  initial value; arrays go back to zero as a fresh document has them), so
  a restyled node is byte-identical to a first-pass node. Fields set by
  ua_defaults (the body margin, list and heading spacing, table borders)
  are set after it, as in the first pass.
- scroll_paint_x returns when box_cw is 0 or less: the box does not
  scroll along x.

Not changed: the parse-time and structural fields, the layout results,
@media (evaluated once, in parse_css: a repeated we_style after
we_set_media does not re-evaluate it, a separate gap), and the Lot 129
quirk about calc() followed by a keyword or a percentage, which probed
fine on 209af85 (height, min and max sizes, flex-basis, width and both
gaps: the later value wins; only a refused value keeps the calc, as CSS
requires).

## Tests

- Host: tests/unit/userspace/test_lot130_engine.c (10 tests): resize tests
  for the box model, the percent min and max width, flex direction, wrap
  and gap, paint (background, float, shadow, outline) and grid templates,
  each with explicit geometry and field checks and a byte comparison with
  the fresh document (nodes and painted items); one declaration per
  reset field (57, the last element of every array included) and 16 on
  the rich document (a table with a collapsed border, lists, form
  controls, generated content, an option of a select that kept
  own_edges); the nested container page of example 4 against
  the same page with a fixed 100 px width and no query; we_style again
  after a class is removed; img with padding wider than the box (four
  forms, plus the 40 px boundary); a scrolling box keeps its bar. 9 of
  the 10 fail on 209af85 (the img test stops the run with SIGFPE; the
  scrolling bar test is a positive control).
- Differential fuzz (outside the repository, tools in the run directory):
  random sets of 1 to 4 of 211 declarations in the container query,
  random widths, a plain and a rich document, reference = the first pass
  of a fresh document, byte comparison of every node and every painted
  item: 100000 cases, 0 mismatches on this tree; on 209af85 two of three
  small seeds stop with SIGFPE and the third gives 208 mismatches in 300
  cases.
- QEMU: /web/lot130.html, `make qemu-lot130`
  (tests/scripts/test_qemu_lot130.py), CI job
  lot130-engine-restyle-initial-style: the nested container page (#z1130,
  #z2130, #z3130 and its two items at their settled boxes, none of the
  209af85 values) and #i130, an img with padding 0 25px, laid out as 24 x
  10 with its padding; web-console reports no diagnostic.
- No existing page changes (every initrd web page renders byte-identical
  through the host pipeline, base vs lot 130).

## Not covered (honest)

- @media is evaluated once; a we_style after we_set_media keeps the first
  evaluation.
- An img keeps its fixed placeholder size: its padding does not enlarge
  the box (only the crash is fixed).
- The layout results of nodes that a later pass does not reach (an
  option inside a select) are not part of the guarantee beyond
  own_edges.
- revert-layer; the keywords on the other non-inherited properties.
- The earlier lots' list: vertical writing modes for left / right and
  self-*, legacy keywords, the overflow default of grid items, column
  baselines in wrap-reverse, the aspect-ratio transferred minimum.
