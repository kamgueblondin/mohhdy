# Lot 61: last baselines, subgrid, viewport and glyph clipping (US-031 partial)

Base: origin/main ffcb267, merged PR #217 (lot 60).
Branch: feat/lot61-webeng-layout. No merge or force-push.

One console/GUI engine in userspace/webeng.c, C freestanding i386 Ring 3.
Kernel, rights and the shared ABI (os_fb_web_item_t) are unchanged. The
shell only switches web-perf to the new viewport entry point. US-031
partial; phase3_complete=false. No Chromium, WebKit, network loading,
JavaScript VM, new font engine or margin collapsing. US-059/060/063/074
unchanged; US-G19..US-G24 partial. Roadmap statuses and the complete
production non-done equality stay intact.

## Lot 60 gains checked first

make test-all on origin/main ffcb267: 2758/2758 in 156 s. Every lot 60
assertion is kept; five fixtures whose refusal became valid behaviour are
listed in "Adapted fixtures" with their new invalid value.

## Grid line names

- Occurrence-qualified names (`name 2`, `2 name`, negative counts from the
  end), named spans (`span name`, `span name 2`) and implicit area aliases
  (`area-start`, `area-end`, and a lone area ident naming both edges).
- A lone custom ident also names the end edge (CSS Grid 8.4); equal start
  and end lines resolve to span 1. Occurrence 0 stays refused.
- repeat(auto-fill, ...) and repeat(auto-fit, ...) with fixed track sizes
  expand to 1..8 tracks from the definite content width and gap. auto-fit
  collapses empty repeated tracks to 0 and their gaps. Names inside the
  repeated group shift with each repetition. Indefinite widths use one
  repetition.

## Baselines

- align-items/align-self accept `last baseline` for row flex lines and grid
  rows; first and last groups are separate. Row-spanning grid items join
  their start row (first) or end row (last).
- Nested export: a flex, grid or table box placed by its parent's baseline
  is laid out by a bounded probe at (0,0); state is restored afterwards.
  Probe depth is capped at 3 and the probe count at 64 per layout ("baseline
  probe budget"). Deeper boxes fall back to their synthesized bottom edge.
- Flex export: a baseline group wins, otherwise the main-start item (first)
  or main-end item (last). Grid export uses the first or last row group.
  Tables export the first row baseline.
- vertical-align:baseline for table cells (shared row ascent/descent, cells
  shifted with their content) and for inline-block atoms in a line box
  (shared ascent A and descent D, step max(line_step, A+D)). Table rowspan
  with baseline still diagnoses "spanning baseline unsupported".

## Subgrid

- grid-template-columns/rows: subgrid on a grid item of a grid parent. The
  subgrid inherits the parent's track sizes, gap, line names and area
  aliases on that axis; its own items size the parent tracks.
- Refused with diagnostics: no grid parent, nested subgrid, padding or
  border on the subgridded axis, and line-name lists after `subgrid`.
- Bounds: the subgrid spans at most 8 parent tracks; its items are clamped
  into that span. Numeric placements are restored from the authored value
  when the parent's auto-repeat count changes between binds.
- Not handled: the subgrid's own gap on that axis, the parent's
  justify/align-content distribution, subgrid items in parent baseline
  groups, overflow diagnostics for implicit rows.

## Viewport, fixed, static position, scroll

- we_layout_viewport(d, w, h) records the viewport; we_layout keeps its
  signature with a 480 px default. position:fixed boxes use the viewport as
  containing block, escape ancestor clips and paint after the page.
- Absolutes with auto insets use their static position: block flow, inline
  flow (the box leaves the line) and container content origin. The probe
  saves it and line/cell shifts move it.
- we_scroll(d, y) clamps to [0, page_h - viewport_h] and moves only fixed
  boxes and their items. The GUI pane (OSUI_WEB_VIEWPORT_H 480) scrolls
  through we_scroll; the mobile pane keeps its existing path.

## Partial glyph clipping

- A text item partly outside its clip is cut into RECT items from the
  kernel 8x8 font (kernel/gfx_font8.h, no copy), including bold. Runs are
  merged vertically. At most 64 fragments per item; beyond that the item is
  dropped with "clipping display budget". Fully visible text is unchanged.

## Collapsed table borders

- border-collapse:collapse shares cell edges: adjacent cells overlap by the
  widest cell border width B (capped at 8) of the table, found through
  thead/tbody/tfoot. The later cell paints the shared edge. Separate
  borders are unchanged.
- Not done: per-edge width resolution, the table/cell outer border merge
  and CSS conflict rules (style, width, then origin).

## Stacking contexts (bounded)

- A relative or absolute box with an integer z-index (-8..8) forms a
  stacking context that groups its subtree, nested up to 7 levels; deeper
  contexts are flattened with "stacking depth". Paint order per level:
  in-flow content, then contexts by z, tree order on ties. z-index on a
  static box still does not reorder. Fixed boxes stay last.
- A negative z context paints after the in-flow content of its level, the
  rule the engine already used for absolutes; CSS paints it below. The pass
  only runs when a relative box has a z-index, so absolute-only documents
  keep the existing per-containing-block order.

## Not done in this lot

- Negative z below in-flow content, opacity/transform contexts.
- Margin collapsing (excluded by the brief), percentage cycles, vertical
  writing modes, column flex baselines.

## Bounded storage

No allocator, no new decoder, no font engine. Node additions: subgrid
flags and gaps, fixed flag, static position, collapse width and two plain
placement bytes per axis. WE_DOCUMENT_MAX 1 MiB and
WE_LAYOUT_WORKSPACE_MAX 512 KiB static assertions still pass.

## Adapted fixtures

Every assertion is kept; only refusal inputs that became valid changed.

- lot60_refusal placement: `start 2 / end` (now a valid occurrence) became
  `start 0 / end`; positive occurrence tests are in lot61_names.
- lot56b vertical-align baseline refusal: now `vertical-align:diagonal`;
  positive tests in lot61_baseline.
- lot60 grid row-spanning baseline diagnostic: moved to a table cell with
  rowspan 2 (still unsupported), same four assertions; the grid fixture is a
  positive test in lot61_baseline.
- lot56 fixed refusal: now `position:floating`; positive tests in
  lot61_viewport.
- lot55 vertical partial clip: the 8 px clip only cut an empty glyph row,
  so the text is now kept (new positive test). The fixture uses a 5 px clip;
  `nitems == 0` became `nitems == 9` RECT fragments ending at y <= 5.

## QEMU

/web/lot61.html combines last baseline in a wrapping flex row, a subgrid
over auto-fill tracks, an inline-block on the text baseline, a collapsed
table, a partially clipped glyph, a fixed box and the shared image. The
console web-render and GUI paths both check it; all older scenarios stay.
The host test reads the same HTML file.

## Measured results

Actual make test-all on the final revision:

```text
Total Tests: 2871
Passed: 2871
Failed: 0
Skipped: 0
Execution Time: 76s
```

test_webeng: 1730 cases, also 1730/1730 under i386 ASan/UBSan
(-fsanitize=address,undefined, no recover). The i386 document is 678340
bytes (node 3840), under the unchanged 1 MiB ceiling; the 512 KiB layout
workspace assertion passes in make all.

Mutation builds (each a private copy of webeng.c linked into test_webeng):
subgrid placement restore, collapse overlap, probe depth, last-baseline
offset, inline-block baseline, glyph fragments, scroll state, auto-repeat
count, subgrid row tracks, stacking pass, stacking z order, stacking
in-flow level and stacking depth diagnostic each fail at least one lot 61
assertion. Removing the fixed-box early return in abs_containing is an
equivalent mutant: a fixed box placed in an ancestor's pass still uses the
viewport; test_l61_fixed_ignores_positioned_ancestors covers the result.

make all, tree-clean-check, strict-image-check, secrets-check,
roadmap-check, check-initrd-web-stamp and
scripts/extract_guest_commands.py --check pass. make qemu-gui-parity
passes. The production assertion is unchanged and complete:

```text
prod-roadmap next US-076 US-078 US-080 US-081 US-097 US-G19 US-G20 US-G21 US-G22 US-G23 US-G24
```

Actual make qemu-web-layout output for this lot (all earlier lots pass in
the same run):

```text
lot61 console flex-last-baseline box 0 10 50x10
lot61 console subgrid-span box 40 24 80x10
lot61 console inline-block-baseline box 16 34 20x16
lot61 console glyph-fragment box 9 69 3x2
lot61 console fixed box 610 470 30x10
osui web render file=/web/lot61.html nodes 34 rules 17 boxes 19 lines 6 items 29 dropped 0 page 640x95 diag 0 components 0 media_kept 0 media_dropped 0 vars 0 sanitized 0
lot61 pixels compared=4624 baseline=last,inline-block subgrid=auto-fill fixed=viewport clip=glyph table=collapse flex=wrap image=shared
QEMU web layout contract passed.
```

Line counts use git diff origin/main --numstat (30 files, +3016 -179).
Useful additions: 1042 engine/header code + 1397 behavioural unit test
code + 54 QEMU script = 2493. Excluded: 95 engine comment lines, 46 engine
blank lines, 44 test comment lines, 134 test blank lines, 203 doc/roadmap/
HTML lines. No copied function.

Under the 4000 target. The first gap left is per-edge collapsed border
resolution (CSS 17.6.2: width, then style, then cell/row/table origin, and
the table outer border merge); this lot uses one uniform shared width.
After it: negative z-index below in-flow content (it would change the
existing absolute rule asserted since lot 56), subgrid own gaps and
content distribution, and margin collapsing (excluded by this brief).
