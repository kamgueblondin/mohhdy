# Lot 122: row flex items (auto bases from the content width, default shrink, automatic minimum of row items)

Base: origin/main a1c5f9c (PR 280, Lot 121). One engine: userspace/webeng.c.
Kernel, rights and ABI unchanged; we_doc_t and we_node_t unchanged. US-031
partial, phase3_complete=false.

## Scope and choice

Lot 121 brought columns onto the flex algorithm and left rows as they
were. Its out-of-theme findings name the row gaps; probes on a1c5f9c (640 x
480, 8 px glyphs) showed they are one model, the Lot 52/59 row bases:

- on the flex path an item with an auto basis and no declared width got a
  basis of 0 unless the row was "modern" (justify-content, gaps, ...) or
  the item had flex-basis auto/content: next to a flex-basis: 50px item, or
  with flex-shrink: 0 only, it was laid out 0 wide; flex-grow: 1 items
  shared the whole line as if their basis were 0 (150 / 150 for "aaaa" and
  "b" in 300 px instead of 162 / 138); a wrapping row broke on 8 px bases;
- an overflowing line shrank only when an item had flex-basis or
  flex-shrink (or the row was modern); explicit growers never shrank;
- row items had no automatic minimum size (CSS Flexbox 4.5): flex: 1 1 0
  items and shrinking items went below their longest word.

Giving content bases alone would make long items overflow without
shrinking; shrinking alone would crush words. The lot does the three
together. Plain rows (no flex property on any item, no modern alignment,
no wrap) keep the equal slots of the early lots; on the 44 initrd pages,
compared through the host copy of the guest pipeline (a1c5f9c against this
lot), no box and no display item changes.

Not taken: fractional flex factors (we_node_t stores integer factors; a
fixed point would change the public fields), flex: inherit / unset /
revert (the engine has no CSS-wide keyword support for longhands either),
osui_runtime web-theme (older renderer), calc-size, anchor, typed attr(),
margin collapsing.

## Auto bases (CSS Flexbox 9.2.3)

On the flex path (any row item with a flex property, flex-wrap, or a
modern alignment) an auto basis without a declared width is the item's
max-content width (it was 0, or 8 px when wrapping). A declared width
(px, percentage, math, intrinsic keyword) stays the basis.

## Default shrink (7.2.2)

Every item of a row on the flex path shrinks by flex-shrink, 1 by
default, when its line overflows: growers included. The shrink pass is
the scaled one of Lot 59 (basis times factor, items frozen at their
minimum, last px to the first item with room).

## Automatic minimum of row items (4.5)

- A row item with min-width auto (unset or "auto") that is not a scroll
  container has as minimum its min-content width, at most its definite
  width (px, percentage of the container, math; an intrinsic width is never
  below min-content) and its max-width; the outer size includes padding,
  borders and margins.
- Same machinery as the column items of Lot 121 (flex_auto_floor, now
  for both axes): the hypothetical size is the base clamped to the
  minimum, growers below it are frozen and the line grows again, the
  shrink stops there ("flex minimum prevents fit" when the line still
  overflows).
- min-width 0, a length, a percentage, math, or an intrinsic keyword turns
  it off (an intrinsic min-width keeps its own value, honoured by the
  shrink).

## Measured rows

measure_flex and layout_flex use the same prepare_flex, so a row measured
inside a column item or a float takes the sizes above. Plain rows are
measured on their equal slots as before; tests now pin both.

## Review points

- Behavior change on the flex path of rows (auto bases, default shrink,
  automatic minimum). Pages: none of the 44 initrd pages changes (host
  comparison, logs/pages_cmp.txt of the lot run).
- Converted tests (markup only, no assertion changed):
  - webeng_lot52.inc test_l52_negative_free_space_does_not_shrink: the two
    70 px items get flex-shrink: 0 (Lot 52 asserted that items never
    shrank without flex-shrink; CSS shrinks them by default);
  - webeng_lot52.inc test_l52_wrap_oversized_first_item_terminates: the
    150 px item gets flex-shrink: 0 (it would shrink to its line);
  - webeng_lot55_intrinsic.inc test_l55_intrinsic_auto_basis_scaled_shrink:
    both items get min-width: 0 (single words, the automatic minimum would
    stop the scaled shrink the test is about);
  - test_lot121_engine.c, the "rows keep no automatic minimum" case: both
    items get min-width: 0 (the case documented the gap this lot fills).
- prepare_flex and layout_flex_tracks lose their sizing and modern
  parameters (no longer read).
- we_doc_t and we_node_t sizes unchanged (no new field).

## Tests

- tests/unit/userspace/test_lot122_engine.c: content bases (growers,
  flex-basis neighbours, flex-shrink only, declared width, wrapping),
  default shrink (growers, scaled shares, flex-shrink 0, wrapped line),
  automatic minimum (overflow and diagnostic, shrink of the others, frozen
  growers, definite px / percent / math width and max-width caps, min-width
  0 / length / percentage / math / auto, overflow hidden, intrinsic
  min-width, padding, borders, margins), measured rows (modern row, plain
  equal-slot rows, grower row inside a column).
- QEMU: initrd_content/web/lot122.html, tests/scripts/test_qemu_lot122.py
  (`make qemu-lot122`, CI job lot122-engine-rows).

## Not done (gaps for later lots)

- Plain rows keep equal slots and never shrink (a 150 px item in a 100 px
  plain row stays 150).
- Fractional flex factors; flex: inherit / unset / revert.
- The transferred minimum of aspect-ratio items; min-content of replaced
  items with percentage sizes.
- osui_runtime web-theme keeps the US-041 resolver.
- calc-size, anchor positioning, typed attr(), margin collapsing.
