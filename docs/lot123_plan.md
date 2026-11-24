# Lot 123: plain flex rows on the flex algorithm, default stretch of row items

Base: origin/main 2f6c517 (PR 281, Lot 122). One engine: userspace/webeng.c.
Kernel, rights and ABI unchanged; we_doc_t (874692 bytes) and we_node_t
(4688) unchanged. US-031 partial, phase3_complete=false.

## Scope and choice

Lot 121 put columns on the flex algorithm, Lot 122 rows that had a flex
property, wrap or a modern alignment. Its first out-of-theme finding is
the rest: a plain row (no flex property on any item, no modern alignment,
no wrap) still took the equal slots of the early lots. Probes on 2f6c517
(640 x 480, 8 px glyphs):

- "aaaa" and "b" in a 300 px plain row were 150 / 150 (CSS: 32 / 8);
- a 150 px item in a 100 px plain row stayed 150 (no shrink);
- two empty items took 320 px each of a 640 px row;
- unbreakable words were cut to the slot (50 / 50 for two 64 px words);
- row items never stretched to their line: in a row of "a" and "b<br>b",
  "a" was 10 high where CSS gives 20 (align-items normal behaves as
  stretch, CSS Flexbox 8.3), on the plain path and on the flex path alike;
- align-items: normal and the other container alignment properties with
  their initial value normal were refused ("alignment refused").

Moving plain rows alone onto the flex algorithm would give items their
content widths but leave them at their content heights; default stretch
is the other half of what CSS does with a plain row, so the lot does both.
The 45 initrd pages, compared through the host copy of the guest
pipeline (2f6c517 against this lot), do not change; lot122.html is
converted (see Review points).

Not taken: fractional flex factors (we_node_t stores integer factors; a
fixed point would change the public fields), flex: inherit / unset /
revert (no CSS-wide keywords for longhands either), the transferred
minimum of aspect-ratio items, osui_runtime web-theme, calc-size, anchor,
typed attr(), margin collapsing.

## Plain rows (CSS Flexbox 9)

layout_flex and measure_flex always call the flex-track code
(prepare_flex, layout_flex_tracks). The equal-slot path of layout_flex
and measure_flex, flex_modes (which only chose between the two) and
export_items (the baselines of the equal-slot path) are removed. A plain
row now gets what Lot 122 gave rows on the flex path: auto bases from the
max-content width, shrink by the default factor 1, the automatic minimum
of row items.

## Default stretch (8.3, 9.4 step 11)

align-items normal (unset) stretches an item like align-items: stretch.
On a row, an item with an auto height takes its line's cross size, minus
its margins, within min-height and max-height. In a column, an item with
an auto width takes its line's width: a single line was already as wide
as the container, but the lines of a wrapping column left their items at
8 px (now as wide as the widest item of the line). A height or width,
align-self other than stretch, or auto margins on the cross axis keep the
content size.

## A single line is the container's definite cross size (9.4 step 15)

A single-line container with a definite cross size (row: a height;
column: its width) now makes its line exactly that size; the line only
grew to it before, so an item taller (wider) than the container made the
line, and with stretch every other item, as large as itself. The large
item still overflows; the others stretch to the container. Without this,
column items, which stretch by default now, would have followed a
min-width: 150px sibling past a 100 px column.

A percentage height whose container height is indefinite counts as auto
(CSS 2 10.5): such an item stretches now, with normal or an explicit
stretch; against a definite container height it resolves as before.

## normal

align-items, align-content, justify-content and justify-items accept
normal, their initial value (stored as unset). align-self and
justify-self keep refusing it: on them normal means stretch, not auto,
and the engine stores only auto. place-* still refuse normal.

## Review points

- Behavior change for every plain flex row (content widths, shrink,
  automatic minimum), for every row item with an auto height and no
  alignment (stretched to its line), for the items of wrapping columns
  (stretched to their line), and for single lines with an item larger
  than a definite container cross size. Pages: none of the 45 initrd pages
  changes through the host pipeline once lot122.html is converted.
- Converted tests (markup only, no assertion changed):
  - webeng_lot52.inc: test_l52_flex_fractional_grow_refused and
    test_l52_flex_grow_overflow_refused (width: 100% on the single empty
    item, which filled its equal slot), test_l52_legacy_auto_equal_columns
    and test_l52_nested_flex_keeps_outer_workspace (flex: 1 1 0 on the
    items: same slots);
  - webeng_lot63_gen.inc test_l63_gen_clearfix_and_flex: the ::before,
    span and ::after flex items say flex: 1 1 0 in a 318 px container
    (106 px thirds as before);
  - webeng_lot64_rtl.inc test_l64_rtl_flex_grid_table: column-gap: 30px
    keeps the item positions of the former 50 px slots (rtl and ltr);
  - webeng_lot64_cases.inc test_l64_column_cases: flex: 1 1 0 on the two
    items of the flex container with column-count;
  - webeng_lot64_vertical.inc test_l64_vertical_flex_and_nesting: the
    vertical root flex item gets width: 30px (its auto basis would be the
    Lot 64 max-content estimate, 40, see Not done);
  - test_lot122_engine.c: seven cases say align-items: flex-start (they
    pin content heights) and the two plain rows of measured_rows say
    flex: 1 1 0 on their items (equal slots);
  - test_lot115_layout.c test_l115_flex_legacy_is_what_the_gui_paints:
    flex: 1 1 0 on the two items (the equal shares the scripts read).
- Converted QEMU page: initrd_content/web/lot122.html, .n122 and #x122
  say align-items: flex-start; test_qemu_lot122.py unchanged apart from
  its docstring.
- The assigned height of a row item drops its percentage test: an item
  that is not stretched passes its own measured height, which already
  resolved its percentage (mutant run: no suite sees a difference).
- Files outside webeng.c: tests, Makefile (qemu-lot123), ci.yml (job
  lot123-engine-plain-rows; ci.yml checked with yaml.safe_load),
  run_all_tests.sh, initrd_content/web/lot123.html and lot122.html,
  tests/scripts/test_qemu_lot123.py and test_qemu_lot122.py (docstring),
  docs.

## Tests

- tests/unit/userspace/test_lot123_engine.c: plain bases (content
  widths, empty items, gaps, nested plain rows), plain shrink (a wide
  item, scaled shares, wrapping words, unbreakable words and the
  diagnostic), default stretch (auto heights, height, max-height,
  align-self, auto margins, min-height, definite container height, edges,
  wrapping lines, percentage heights, flex-start, columns, wrapping
  columns), single line (a taller row item, a wider column item, a
  definite height of 0), normal
  (accepted on the container properties, refused on align-self and
  justify-self; align-content normal packs the lines of a wrapping row
  as before), measured plain rows (in a column, in a float).
- QEMU: initrd_content/web/lot123.html, tests/scripts/test_qemu_lot123.py
  (`make qemu-lot123`, CI job lot123-engine-plain-rows).

## Not done (gaps for later lots)

- align-content normal should stretch the lines of a wrapping row; the
  engine's initial value packs them at the start.
- A stretched item's height is not definite for its children's
  percentages.
- place-items / place-content normal stay refused.
- The vertical root max-content estimate of Lot 64 (40 px for 30 laid
  out) is now the auto basis of such a flex item.
- Fractional flex factors; flex: inherit / unset / revert; the
  transferred minimum of aspect-ratio items; osui_runtime web-theme keeps
  the US-041 resolver; calc-size, anchor, typed attr(), margin collapsing.
