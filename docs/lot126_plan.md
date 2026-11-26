# Lot 126: safe and unsafe alignment, true alignment of flex lines and grid tracks, grid items stretched

Base: origin/main 3179618 (PR 284, Lot 125). One engine: userspace/webeng.c.
Kernel, rights and ABI unchanged. userspace/webeng.h gains two enums
(WE_OVF_NONE / SAFE / UNSAFE and the property ids WE_OVF_JC ... JS) and
one field, we_node_t.align_overflow (uint16_t): we_node_t grows from 4688
to 4692 bytes and we_doc_t from 874692 to 875332 (under the 1 MiB bound,
checked at compile time). US-031 partial, phase3_complete=false.

## Scope and choice

Three out-of-theme findings of Lot 125, probed on 3179618 (640 x 480,
8 px glyphs, 10 px lines):

1. The safe and unsafe keywords were refused ("alignment refused") on
   every alignment property.
2. align-content of flex lines and the content alignment of grid tracks
   clamped an overflow to the start: two 20 px lines in a 20 px wrapping
   container under align-content: center stayed at 0 and 20; two 80 px
   columns in a 100 px grid under justify-content: center at 0 and 80.
3. Grid items were not stretched by default: a one-line item in a 20 px
   row stayed 10 high (align-self normal behaves as stretch for
   non-replaced grid items, CSS Box Alignment 6.1, Grid 11.3).

All three are done; nothing dropped.

## safe / unsafe (CSS Box Alignment 4.4)

Parsing: "safe <position>" and "unsafe <position>" on align-items,
align-self, align-content, justify-content, justify-items and
justify-self, where a position is start, end, flex-start, flex-end or
center; the keyword is refused with stretch, a distribution, a baseline,
normal, auto or without a position. place-items, place-content and
place-self take one keyword per value (up to four tokens); one value
copies its keyword to both axes. The keyword is stored in two bits per
property of we_node_t.align_overflow; a value without it clears them;
align-self / justify-self auto take the container's keyword.

Layout: aligned_distance(mode, free, index, count, overflow, default)
returns 0 for an overflowing subject (free < 0) under safe, the true
offset (true_distance, Lot 125) under unsafe, and the caller's default
without a keyword: true for flex items, flex lines and grid tracks, the
start for grid items (their default is unchanged; unsafe now aligns
them truly). The flex callers turn safe into start before their mirrors:
an overflowing item takes start (not mirrored in wrap-reverse), an
overflowing line of a safe justify-content takes start (swapped to end
before the row-reverse / column-reverse mirror, so it starts at the left
/ top), and safe overflowing lines take start before the wrap-reverse
swap. rtl mirrors after layout, so start is the right edge there.

## True alignment of flex lines and grid tracks

Flex: the lines of a multi-line container whose cross size is definite
align truly when they overflow it (cross_over = cross size - lines < 0):
center overflows both sides, end / flex-end the start side; normal
(stretch) and the distributed values fall back to the start. In
wrap-reverse the lines are mirrored around the container's cross size,
so flex-start packs from the bottom edge and overflows the top (it was
mirrored around the lines, kept inside). A single-line (nowrap)
container and an indefinite cross size are unchanged.

Grid: justify-content on the columns and align-content on the rows align
truly when the tracks overflow (rows only with a definite height),
unless safe; normal / stretch keep their start fallback.

## Grid items stretched by default

layout_grid: align-self normal (unset, from align-items normal) is
stretch for a grid item that is not replaced (img, input, textarea,
select, iframe, video, audio, canvas, object) and has no preferred
aspect ratio (Grid 6.2): its auto height takes the area (min / max
heights apply, a child percentage sees it), an explicit height, auto
margins, start or align-items: start keep the content height. The
horizontal axis already filled the area. Items of a row subgrid stretch
in its rows.

## Review points

- webeng.h (outside webeng.c): new field we_node_t.align_overflow
  (uint16_t) and the WE_OVF_* enums; we_node_t 4688 -> 4692 bytes,
  we_doc_t 874692 -> 875332 (bound 1 MiB, compile-time check). Not an
  ABI: we_node_t / we_doc_t are only used by userspace modules built
  from the same sources (webeng.c, shell.c, osui_gui.c, shell_fetch.c,
  wf_jslayout.c); no kernel structure, syscall or file format carries
  them.
- Rendering change: overflowing flex lines and grid tracks; wrap-reverse
  lines that overflow; grid items stretched. Three existing pages change
  under item 3 and are converted by markup only (align-items: start on
  their grid, identical to the base on the original page): lot52.html
  (#grid), lot62.html (#gp), lot82.html (#m). The other 45 pages do not
  change.
- Converted unit tests (markup only, align-items: start, no assertion
  changed): webeng_lot61_subgrid.inc test_l61_subgrid_rows_feed_parent_rows
  and test_l61_subgrid_both_axes_with_names.
- Changed assertion: test_lot124_engine.c test_l124_wrap_reverse, the
  item taller than its wrap-reverse line: the 40 px line now overflows
  the 30 px container from its bottom edge (a at y -10, b at 20; were 0
  and 30, the line kept inside).
- Files outside webeng.c: webeng.h, tests, Makefile (qemu-lot126), ci.yml
  (job lot126-engine-safe-alignment), run_all_tests.sh, initrd pages
  lot126.html (new), lot52.html, lot62.html, lot82.html (converted),
  tests/scripts/test_qemu_lot126.py, docs.

## Tests

- Host: tests/unit/userspace/test_lot126_engine.c: parsing (every
  longhand, auto, clearing, refused forms, place-* one and two values,
  refused place forms, extra tokens, a container query restyle clearing
  a keyword), flex (safe / unsafe / default items, inherited
  from align-items, a column, justify-content safe / unsafe, a fitting
  line, row-reverse, rtl), flex lines (center, flex-end, safe, unsafe,
  normal, space-around, wrap-reverse flex-start, center, safe end, a
  wrapping column, fitting lines, an indefinite height, a single-line
  container), grid alignment
  (columns center, end, safe, rtl; rows end, center, safe, indefinite
  height; items default, unsafe, safe, place-items unsafe), grid stretch
  (stretched items, child percentage, max-height, start, explicit height,
  auto margin, align-items start, img with a size, img with alt text,
  input, aspect-ratio, a flex-stretched
  grid, a row subgrid).
- QEMU: initrd_content/web/lot126.html, tests/scripts/test_qemu_lot126.py
  (make qemu-lot126, CI job lot126-engine-safe-alignment); converted
  pages through make qemu-web-layout (lot52, lot62) and make qemu-lot82.

## Not done

- Grid items keep the start as their overflow default (no keyword); CSS
  leaves the default to the user agent (a blend), browsers mostly align
  truly.
- left / right, self-start / self-end and the legacy keywords are not
  parsed; the distributed values' fallbacks stay at the start.
- A column flex item under start / flex-start takes the full width.
- Left from Lot 125: column baselines in wrap-reverse, the Lot 64
  vertical estimate, fractional flex factors, flex inherit / unset /
  revert, the aspect-ratio transferred minimum, osui_runtime web-theme on
  the US-041 resolver, calc-size, anchor, typed attr(), margin collapsing.
