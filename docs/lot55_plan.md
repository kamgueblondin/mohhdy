# Lot 55: bounded guest layout system (US-031 partial)

Base: fresh origin/main 8353ded (merged PR #211).
Branch: cursor/lot55-layout-system-d639. No merge or force-push.

This is one large implementation lot in userspace/webeng.c. The useful
engine/header/test addition target is 4000..6000 lines, excluding docs,
comments, blanks, copied functions and mechanical renames. No filler.

Scope: intrinsic text/block sizes and flex auto bases; flex alignment and
axis gaps; fixed/fractional grid tracks, bounded placement and spans;
definite percentage heights and border-box sizing; table colspan;
word wrapping; overflow clipping, visibility and consistent link hits.

One shell, one console/GUI engine, freestanding i386 Ring 3, fixed memory
ceilings. Kernel, rights and shared syscall ABI unchanged. US-031 remains
partial; phase3_complete=false. No Chromium, WebKit, network loader,
JavaScript VM or new font engine. US-059/060/063/074 unchanged;
US-G19..US-G24 remain partial. Roadmap statuses do not advance.

Every behavior requires host geometry or pixel assertions. The guest QEMU
page will combine the new behaviors and retain all old web assertions.
Measured limits, counts and results are filled after execution.

## Implemented behavior and explicit limits

- Text/inline/block/flex/grid/table min/max content widths are measured
  without painting. width:min-content/max-content/fit-content and explicit
  flex-basis:auto/content use those bounded sizes. Legacy equal slots and
  unspecified column defaults remain. Intrinsic image widths use alt or
  declared dimensions; full replaced-element intrinsic sizing is absent.
- Actual word formatting is shared by measurement and painting, including
  floats, nowrap, explicit br and break-word. Overlong words emit multiple
  bounded text items rather than losing their suffix. HTML whitespace
  between inline elements is retained without extra DOM nodes.
- A text source chunk stores at most 511 decoded characters, with a
  diagnostic on truncation. Inline word capacity is 64; nowrap overflow of
  this workspace is diagnosed. No pre/pre-wrap, shaping or new font.
- justify-content and align-content: start/end/center/space-between/around/
  evenly; align-content also stretches lines/auto grid rows. align-items,
  align-self, justify-items and justify-self: start/end/center/stretch;
  self:auto falls back to the container. Negative free space stays at the
  start. Baselines, reverse directions, ordering and auto margins absent.
- Row/column-gap and one/two-value gap. Fixed integer pixels 0..2000.
  Legacy one-value gap keeps prior defaults; explicit alignment/axis gaps
  opt into intrinsic flex defaults. Unsupported values are diagnosed.
- Grid lists/repeat of px, auto or integer 1fr..8fr, at most 8 declared
  tracks per axis. Fixed tracks retain their widths; auto columns shrink
  to measured word floors; fractions divide remaining space. Coordinates
  are capped at WE_LAYOUT_MAX. This is not full CSS minmax/subgrid sizing.
- Grid starts 1..8, spans 1..8 and start/end or start/span syntax. Explicit
  placements reserve cells before row-major auto placement. Collisions,
  invalid values and occupancy exhaustion are diagnosed. Implicit rows
  are bounded by WE_GRID_ROWS_MAX (160), columns by 8. No dense packing,
  named areas, negative indices or implicit column-axis auto flow.
- Definite percentage height/min/max-height values are integer 0..2000%.
  An indefinite containing height is diagnosed and treated as auto (or
  the limit ignored). Grid children resolve against their occupied area.
  Cyclic/fully general intrinsic percentage track resolution is absent.
- content-box/border-box apply to block and assigned layout boxes. Minima
  beat maxima; borders/padding cannot create negative content. Inline
  replaced-image box-model conformance is not claimed.
- Table colspan 1..8 shares the same column widths across rows/sections.
  Invalid spans and more than 8 total columns refuse the table. rowspan
  other than 1 is still refused. No border collapse or full auto-table CSS.
- overflow:hidden intersects ancestor padding boxes after geometry is
  laid out. Rectangles/images clip exactly; partial borders become real
  edge rectangles. Text keeps complete fixed-font glyphs only, omitting
  partial glyphs. No scrollbars or sub-glyph clipping. Hit tests use the
  published text items, clipped bounds and the deepest eligible node.
- visibility:hidden keeps space and inherits; visible descendants may
  override it. Hidden items do not consume the display list. The list is
  still 96 items, with diagnostic/counters on budget exhaustion.

## Fixed storage and single implementation

WE_MAX_NODES=160, WE_DEPTH_MAX=24, 8 columns, at most 160 implicit rows.
WE_LAYOUT_WORKSPACE_MAX=512 KiB bounds all static layout/measurement/clip
scratch with a compile-time assertion. The caller owns we_doc_t, including
per-document item owners; rendering another document does not break hits.
A per-node dimension cache and WE_MEASURE_BUDGET=25600 bound measurement.
Stylesheet declarations are parsed once per parsed document; repeated
style/layout does not grow the CSS pool. No heap allocation is added.

Images keep the one imgdec workspace, with the existing header ceiling,
64 KiB source/scratch and 64x64 decode/output limits. Only per-node sizes
and failure flags are cached; at most the last decoded image stays ready.
Interleaved images may be read again for painting, never kept in a second
pixel buffer. No new JPEG or network path.

## Tests and validation

Eight new host include files use the existing test_webeng binary and
webeng.c. They assert literal geometry, text positions, pixels, link hits,
limits, repeated layout/style and caller-document canaries. The former
refusals of 2fr, fixed columns, colspan=2 and percentage height limits use
values outside the new supported range; positive geometry assertions for
the newly supported cases are added separately. No assertion is weakened.

The QEMU contract retains all prior page assertions. /web/lot55.html adds
console inspector coordinates and a combined framebuffer comparison for
packing, mixed/spanned grid, percentage/border-box, colspan, clipping,
visibility and an initrd image. Exact output and gates follow execution.

## Measured local results

Executed on 28 October 2026 with the configured NASM/GCC multilib/QEMU
container. i386 host binaries use qemu-i386 on this execution kernel.

```text
Total Tests: 1596
Passed: 1596
Failed: 0
Skipped: 0
Execution Time: 154s
```

The engine has 455 passing cases, including 232 new cases. The same 455
pass ASan/UBSan. Ten separate mutation builds fail their geometry/pixel/
state assertions when intrinsic sizes, packing, grid spans, height
percentages, border-box, colspan, word wrapping, clipping, visibility or
CSS pool caching are removed. Mutations live outside the repository.

Actual QEMU console/framebuffer output:

```text
layout-system inspect fb box 100 15 20x10
layout-system inspect ga box 24 40 96x10
layout-system inspect span box 0 112 80x10
layout-system inspect intrinsic box 0 142 40x20
layout-system inspect image box 0 202 48x16
web-layout pixels image=48x32 compared=1536 wrap=2 table_rows=2
flex-sizing pixels compared=2000 shrink=50,50 frozen=70,30
column-sizing pixels compared=4000 shrink=50,50 frozen=70,30 wrap_x=14
osui web render file=/web/lot55.html nodes 35 rules 26 boxes 26 lines 6 items 69 dropped 0 page 640x228 diag 0 components 0 media_kept 0 media_dropped 0 vars 0 sanitized 0
layout-system pixels compared=7592 flex=packed grid=spanned table=colspan image=decoded clip=bounded
QEMU web layout contract passed.
```

From git diff origin/main --numstat: engine/header and behavior tests add
4597 raw lines: 4311 code (1273 engine/header, 3038 tests), 14 comment-only
and 272 blank lines. Documentation/roadmap, the HTML fixture and test build
rule do not count. No existing function is copied to increase the count.
The one formatter and table row collector are shared, not duplicated.
The useful count is inside the requested 4000..6000 band; no filler is
added to reach 5000. Full CSS and the explicit limits above remain missing.
CI run/job conclusions are reported in the PR only after reading them.

make all, make qemu-web-layout, make qemu-gui-parity,
make strict-image-check, make secrets-check, make roadmap-check and
python3 scripts/extract_guest_commands.py --check pass locally.
make tree-clean-check is run after committing the intended files.
