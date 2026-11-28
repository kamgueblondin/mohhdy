# Lot 132: display: contents on table parts

Base: origin/main 845be6c (PR 290, Lot 131). One engine: userspace/webeng.c
only (webeng.h unchanged). Kernel, rights and ABI unchanged. we_node_t is
4692 bytes and we_doc_t 876296 bytes (both measured, both unchanged), under
the 1 MiB bound. US-031 partial, phase3_complete=false.

## Scope and choice

Lot 131 left `display: contents` on a row or a cell unfinished: "Not covered:
a tr with it leaves its cells directly under the table". Candidates probed on
845be6c (640 x 480, 8 px glyphs, 10 px lines) before choosing:

- `tr` / `td` with `display: contents` in a table: wrong (below).
- sticky or fixed positioned element inside a contents element: correct on
  845be6c (the box parent is the containing block; the positions match the
  page without the wrapper). Dropped.
- `revert-layer` across two layers: correct on 845be6c. Dropped. In an
  unlayered rule it gives a wrong value (niche, not taken).
- `@media` evaluated once: nothing outside the tests re-styles after
  we_set_media, the flattening lives in the guest, outside the engine. Not
  taken.

Probed on 845be6c, table with `border=1`:

1. `<tr style="display:contents"><td id=a>a<td id=b>bb</tr><tr><td id=c>c
   <td id=d>d</tr>`: a and b have no box (0 0 0x0) and are not painted, two
   "table child refused" diagnostics, the table is 14 high, c at 1 1 319x12;
   expected a 1 1, b 320 1, c 1 13 and d 320 13 (26 high).
2. `<td id=a>a<td style="display:contents">xy<td id=b>b`: the text "xy" is not
   painted ("table cell refused"), a is 319 wide; expected three columns
   (a 213, the anonymous cell with "xy" at 214, b at 427).
3. `<td style="display:contents"><div id=k style="height:20px">k</div>n<td
   id=b>b`: k has no box and b is 638 wide; expected k 1 1 319x20, the table
   32 high, b at 320.

## How it works

CSS Tables 3, 9.1.2: what a contents row hands to its table, and what a
contents cell hands to its row, is wrapped in anonymous boxes.

- anon_table_parts runs at the end of splice_contents, after the children of
  every contents element were re-hung to their box ancestor. Two cases for a
  row (content that is not a cell is wrapped in one anonymous cell per run)
  and for a table or a section tbody, thead, tfoot (content that is not a row,
  a section, a caption or a column is wrapped in an anonymous cell, then each
  run of cells, real or anonymous, in an anonymous row).
- Only what came through a contents element is wrapped (the child is a new
  node, or its DOM parent is not its box parent). The stray children of a
  plain table keep being refused as before ("table child refused", "table cell
  refused").
- A run is consecutive children; display none and absolute children between
  two strays go with them. Consecutive contents rows therefore make one
  anonymous row, a plain row between two contents rows keeps them apart.
- An anonymous box is a node allocated after the generated ones, styled with
  pseudo 3 (no rule, no UA default, only inheritance from its box parent),
  display set to row or cell. No padding, no border, no size. The next we_style
  drops it with the generated nodes (unsplice restores every list first), so
  a restyle (container query, viewport units) starts clean.
- When the node table is full the anonymous box is not made: diagnostic
  "table anonymous box budget", the content stays refused as before the lot.
- The contents element keeps being a display none leaf with the flag, its
  style computed and inherited (a text-align on it reaches the content).

## Tests

- Host: tests/unit/userspace/test_lot132_engine.c (14 tests). Most compare
  the table with a contents row, cell or body (carrying margin, padding,
  border, background, size, position, transform and more) against the same
  table written with the anonymous boxes (plain rows, cells with no padding
  and no border): the box of each id and the painted display list must be the
  same; explicit values are checked too. They cover: a single row, a row in a
  section with colspan and rowspan, head, body and foot, a caption and a column
  group; consecutive rows merging, a plain row between, an empty contents row;
  a contents cell (text, blocks and inline, a table-cell div inside, an empty
  cell, th); a contents row holding a contents cell, content straight in the
  table, a contents body (also followed by a contents row, and a contents head
  and foot), a table inside a contents cell; table children that a contents
  element hands over (caption, colgroup, col, head, body, foot, rows) and are
  not wrapped, a table part handed to a row (wrapped); an absolute or hidden
  box between two cells; the style of an anonymous box (no universal rule, no
  rule of its row, inheritance from the table); a stale DOM link in the slot
  of an anonymous box after a pass that generated a box; strays of a plain
  table still refused with and without a contents element elsewhere; second
  and third style passes (node count and display list); the full node table
  (one diagnostic for a run of two cells); a container query that makes a row
  contents and plain again; table parts by display only; the box tree
  invariant on a mixed table. 13 of 14 fail on 845be6c (only the guard on
  plain tables passes there).
- Differential fuzz (outside the repository, tools in the run directory):
  random tables (contents rows, cells and bodies with 17 kinds of ignored
  properties, colspan and rowspan, blocks, inline and text in cells) against
  the same table with the anonymous boxes written out: box of every id, every
  painted item, page height: 12000 cases (6 seeds of 2000), 0 mismatches; on
  845be6c 289 of 300, 284 of 300 and 294 of 300 cases differ for three seeds.
- QEMU: /web/lot132.html, `make qemu-lot132` (tests/scripts/test_qemu_lot132.py),
  CI job lot132-engine-table-contents: two consecutive contents rows and a
  plain row, a contents cell with a block and an inline, a contents body with
  a contents row and a contents cell, a contents div handing a caption, a
  column group, a head, a body and a foot to a table (they are not wrapped:
  845be6c already laid them out, the rows are guards), an absolute box and a
  display none box between the two cells of a contents row; every contents
  element is `display contents`, box 0 0 0x0, style computed; the cells that
  845be6c refused have their new boxes (NEVER lists its values);
  web-console reports no diagnostic.
- No existing page changes (every initrd web page renders byte-identical
  through the host pipeline, base vs this lot).

## Not covered (honest)

- The stray children of a plain table (no contents element) are still refused:
  the spec wraps them too, the engine keeps its earlier behaviour on purpose.
- An anonymous box has no id or class and no rule matches it; it cannot be
  read with web-inspect (its cells and content can).
- The anonymous box budget is the node table (160): a page that fills it
  keeps the cells refused.
- A whitespace-only text node just before a closing tag is dropped by the
  parser whatever the element (found by the fuzz: `<span><u>x</u> </span>
  <u>y</u>` loses the space). It is not related to contents and is left.
- `revert-layer` in an unlayered rule, `@media` evaluated once per parse, and
  the earlier lots' list (writing modes for left / right and self-*, legacy
  keywords, grid overflow default, wrap-reverse column baselines,
  aspect-ratio transferred minimum).
- The Lot 131 list stays: the root element, writing-mode or direction set on
  the contents element itself, scroller, sticky or fixed on it.
