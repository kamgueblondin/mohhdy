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
assertion is kept; the fixtures whose refusal became valid behaviour are
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
- Column flex: first baseline falls back to start alignment and last
  baseline to end alignment (the item shrinks to max-content, CSS Box
  Alignment fallback); no diagnostic.
- vertical-align:baseline for table cells (shared row ascent/descent, cells
  shifted with their content) and for inline-block atoms in a line box
  (shared ascent A and descent D, step max(line_step, A+D)). A row-spanning
  baseline cell aligns in its first row, its spanned height includes the
  shift and it takes part in the row baseline the table exports.

## Subgrid

- grid-template-columns/rows: subgrid on a grid item of a grid parent. The
  subgrid inherits the parent's track sizes, gap, line names and area
  aliases on that axis; its own items size the parent tracks.
- Refused with diagnostics: no grid parent, nested subgrid, padding or
  border on the subgridded axis, and line-name lists after `subgrid`.
- Bounds: the subgrid spans at most 8 parent tracks; its items are clamped
  into that span. Numeric placements are restored from the authored value
  when the parent's auto-repeat count changes between binds.
- Own gap: a column-gap/row-gap declared on the subgrid overrides the
  parent gutter on that axis. The difference is split in half around each
  interior line (CSS Grid 9) and added to the items' track contributions.
- The subgrid's tracks follow the parent's real line positions, so the
  parent's justify/align-content distribution reaches the subgrid. One
  uniform gutter (the average effective parent gutter plus the own-gap
  difference) is used; rounding can leave 1 px on an interior right edge.
- Baselines: baseline-aligned items of a row subgrid join the parent's
  first/last baseline groups of the rows they sit in (CSS Grid 2, 9.6),
  own-gap offsets included; the shared ascent and last descent are handed
  back to the subgrid before it places its items.
- A subgrid ignores self-alignment on its subgridded axes (always
  stretched).
- Not handled: overflow diagnostics for implicit rows; column subgrids do
  not share baselines (rows are their own).

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
  band width B (widest resolved edge, capped at 8), found through
  thead/tbody/tfoot.
- Per-edge resolution (CSS 2.1 17.6.2.1) for every grid-edge segment:
  hidden suppresses, then the wider width, then style rank (double, solid,
  dashed, dotted, ridge, outset, groove, inset; none loses), then origin
  (cell above/left, cell below/right, row, row group, column, column
  group, table). border-top/right/bottom/left and 1-4 value
  border-style/width/color are parsed. The table's own border joins the
  outer segments. <col> (void, span) and <colgroup> (col children or span)
  add column and column-group borders: their outer top/bottom and their
  left/right lines.
- Half-border split: the winning line of width w sits at B/2 - w/2 in its
  band; each cell's content is inset past its half of the widest resolved
  line on that side. With an odd width the extra pixel goes to the cell
  after the line (right or below). Runs are merged, vertical lines paint
  after horizontal ones.
- Per-edge declarations outside a collapsed table are refused ("border edge
  refused"); border-style none/hidden zeroes a separate border.
- Limits: the band width B is uniform per table (a borderless line still
  splits B between its cells), corners show the vertical line.

## Border styles

- dashed (dashes of max(2w, 3) px, equal gaps), dotted (w x w dots, w gaps)
  and double (w >= 3: two (w + 1) / 3 px lines) paint as RECT items for
  separate borders and collapsed lines, at most 8 pieces per line (the
  period grows past that). Solid separate borders keep the BORDER item.
- ridge, outset, groove and inset paint solid with one "border style
  painted solid" diagnostic.

## Stacking contexts (bounded)

- A relative or absolute box with an integer z-index (-8..8) forms a
  stacking context that groups its subtree, nested up to 7 levels; deeper
  contexts are flattened with "stacking depth". Paint order per level:
  in-flow content, then contexts by z, tree order on ties. z-index on a
  static box still does not reorder. Fixed boxes stay last.
- CSS 2.1 appendix E order per context: the context's own background,
  negative z children (most negative first), in-flow content, then z >= 0
  children. The root context background (document, root element, body
  under it) paints first. The pass runs when a relative box has a z-index
  or any positioned box has a negative z-index; otherwise absolute-only
  documents keep the per-containing-block order.
- opacity (number or percentage, clamped to 0..1; "opacity refused"
  otherwise) below 1 forms a z-index 0 stacking context, also on a static
  box (whose z-index stays ignored). The shared item has no alpha and the
  kernel painter is unchanged, so opacity 0 removes the subtree's items
  (the boxes keep their layout) and 0 < opacity < 1 paints opaque with one
  "opacity painted opaque" diagnostic.
- transform: the engine has no transform parsing; it stays an unknown
  property (not handled in this lot).

## Not done in this lot

- Alpha blending for 0 < opacity < 1 (needs an item alpha or painter
  change), transform, floats inside the negative layer, per-line collapsed
  band widths, 3D border styles.
- Margin collapsing (excluded by the brief), percentage cycles, vertical
  writing modes.

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
- lot60 column flex baseline refusal: now `align-items:baseline-start`
  ("alignment refused"); positive fallback tests in lot61_baseline.
- lot60 table rowspan baseline diagnostic: now `vertical-align:
  baseline-shift` ("vertical align refused"); positive rowspan tests in
  lot61_baseline.
- lot56 z-index on static box: b used `z-index:-1` with position:relative,
  now a valid negative context; it uses `z-index:-1.5` and also asserts
  "z-index refused". Positive tests in lot61_stack.
- lot61 internal: uniform collapse "later cell wins" now expects the CSS
  winner; negative z stacking tests expect appendix E order; collapsed
  geometry tests follow the half-border insets and B/2 - w/2 line offset
  (exact new boxes, text positions added); style tests sample dash/dot
  pieces and gaps.
- lot55 vertical partial clip: the 8 px clip only cut an empty glyph row,
  so the text is now kept (new positive test). The fixture uses a 5 px clip;
  `nitems == 0` became `nitems == 9` RECT fragments ending at y <= 5.

## QEMU

/web/lot61.html combines last baseline in a wrapping flex row, a subgrid
over auto-fill tracks, an inline-block on the text baseline, a collapsed
table, a partially clipped glyph, a fixed box, the shared image, a collapsed
table whose 4 px outer border wins over 2 px cells (content inset by half
of each line), a subgrid with its own 10 px column-gap, a negative z box
under an in-flow block, dashed and double borders and an opacity 0 box. The
console web-render and GUI paths both check it; all older scenarios stay.
The host test reads the same HTML file.

## Measured results

Actual make test-all on the final revision:

```text
Total Tests: 2920
Passed: 2920
Failed: 0
Skipped: 0
Execution Time: 80s
```

test_webeng: 1779 cases, also 1779/1779 under i386 ASan/UBSan
(-fsanitize=address,undefined, no recover). The i386 document is 684100
bytes (node 3876), under the unchanged 1 MiB ceiling; the 512 KiB layout
workspace assertion passes in make all.

Mutation builds (each a private copy of webeng.c linked into test_webeng)
each fail at least one lot 61 assertion: subgrid placement restore,
probe depth, last-baseline offset, inline-block baseline, glyph fragments,
scroll state, auto-repeat count, subgrid row tracks; collapse (17:
hidden, width, style rank, none, origin order cell/row/group/table, outer
border merge, run merge, band centring, edge parsing, refusal); subgrid
gap and distribution (9: own gap delta, half split, item extra on both
axes and in the intrinsic pass, line positions); baselines (8: column
fallback first/last, rowspan first row, span height, shift, row export);
stacking (pass, z order, negative below flow, own items first, root
background, negative trigger, depth diagnostic). Final round: collapsed
insets and columns (15: before/after inset, odd pixel side, widest line,
box_edge users, col and colgroup origin order, col span, colgroup
children, void col); border styles (11: dash period, dot size, piece cap
and clamp, double thickness and gap, double minimum width, 3D diagnostic,
separate-border split); opacity (11: parse range, percent, refusal,
stacking trigger, z of non-positioned context, opacity 0 drop, opaque
diagnostic); subgrid baselines (10: contribution offsets, share max,
row subgrid stretch, column subgrid not intrinsic). Removing the
fixed-box early return in abs_containing is an equivalent mutant;
test_l61_fixed_ignores_positioned_ancestors covers the result. Equivalent
mutants found in the final round were removed as dead code.

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
lot61 console collapse-outer-edge box 0 95 120x4
lot61 console collapse-inner-edge box 59 95 2x14
lot61 console subgrid-own-gap box 45 109 30x6
lot61 console negative-z box 0 115 120x6
lot61 console dashed-piece box 8 124 4x2
lot61 console double-line box 0 132 10x1
osui web render file=/web/lot61.html nodes 47 rules 28 boxes 31 lines 6 items 57 dropped 0 page 640x144 diag 0 components 0 media_kept 0 media_dropped 0 vars 0 sanitized 0
lot61 pixels compared=7648 baseline=last,inline-block subgrid=auto-fill fixed=viewport clip=glyph table=collapse,outer-edge,half-inset border=dashed,double opacity=0 subgrid-gap=own z=negative flex=wrap image=shared
QEMU web layout contract passed.
```

Line counts use git diff origin/main --numstat (32 files, +4612 -219
before this section). Useful additions: 1552 engine/header/shell code +
2077 behavioural unit test code + 66 QEMU script = 3695. Excluded: 187
engine comment lines, 87 engine blank lines, 103 test comment lines, 192
test/script blank lines, about 350 doc/roadmap/HTML lines. No copied
function.

Under the 4000 target; the brief's final round stopped after its four
items without padding. The first gap left is margin collapsing, excluded
by this brief. Within the brief, the first compatible gap left is alpha
blending for 0 < opacity < 1 (painted opaque with a diagnostic: the
kernel painter masks item alpha, so it needs an item alpha field or a
painter change outside this lot). After it: transform parsing and its
stacking context, 3D border styles (painted solid with a diagnostic),
per-line collapsed band widths (the band is uniform per table), column
subgrid baselines, floats in the negative stacking layer, percentage
cycles and vertical writing modes.
