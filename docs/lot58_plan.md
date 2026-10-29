# Lot 58: bounded dimensional math and box sizing (US-031 partial)

Base: fresh origin/main 25e5c62, merged PR #214 (lot 57).
Branch: cursor/lot58-css-sizing-d639. No merge or force-push.

One large increment in userspace/webeng.c and its existing host/QEMU tests.
One shell, one console/GUI path, freestanding i386 Ring 3. Kernel, rights
and shared syscall ABI unchanged. US-031 stays partial; phase3_complete=false.
No Chromium, WebKit, network loading, JavaScript VM or new font engine.
US-059/060/063/074 unchanged; US-G19..US-G24 remain partial. Roadmap statuses
and the complete production non-done list do not advance.

## Dimensional arithmetic subset

- calc, min, max and clamp for width/height, min/max width/height,
  flex-basis, absolute insets, relative left/top, row/column-gap and gap.
  Lengths use integer px and percent. Scalar numbers are accepted for
  multiplication and division; two lengths cannot be multiplied, and a
  length cannot be a divisor. Addition/subtraction and min/max/clamp need
  matching types. A result must be a length. Binary +/- need surrounding
  whitespace. Function/unit names are ASCII case insensitive.
- Parentheses, precedence and left associativity are bounded. min/max
  accept 1..4 arguments, clamp exactly 3. The lower clamp bound wins if it
  exceeds the upper bound. Integer division truncates at every operation;
  this is not floating-point or rational CSS arithmetic.
- An expression stores at most 24 postorder nodes, with at most 4 nested
  functions/parentheses. Literals are signed integers of magnitude <=2000.
  Intermediate magnitudes are <=1000000; overflow and zero divisors are
  refused rather than overflowing C integers. Used coordinates/sizes are
  capped at WE_LAYOUT_MAX (16000); negative dimensions clamp to zero.
  Signed positioned coordinates retain the existing -16000..16000 bound.
- Invalid syntax, type, constant overflow, division by zero or expression
  capacity/depth keeps the previous declaration and produces a diagnostic.
  Dynamic overflow or an indefinite percentage basis produces a layout
  diagnostic and the initial auto/ignored-limit fallback. Percentage flex
  bases in indefinite columns retain the existing diagnosed height fallback.
  Unused insets on static boxes are not evaluated.
- Width percentages use the containing width. Height/min/max-height use a
  definite containing height or a definite grid area, as before. Insets
  use the positioned padding box; relative vertical offsets still require
  a definite parent height and remain bounded by that parent. Relative
  right/bottom placement remains outside the subset.
- Gaps accept one/two plain or mathematical lengths, atomically. Percentage
  gaps, em/rem/vw/vh in expressions, decimals, variables/functions outside
  this subset and general CSS-wide keywords remain unsupported. Existing
  non-math value parsing keeps its earlier, limited behavior. Math is not
  added to padding/margin/border lengths or grid-template track descriptors.

## Box behavior

- aspect-ratio accepts auto or an integer numerator and optional denominator
  in 1..2000. It derives an automatic content height from the actual used
  width; explicit height wins. Content may establish a larger minimum,
  then min/max-height apply. border-box ratios include padding/borders;
  content-box ratios do not. Zero content width stays zero. General cyclic
  percentage sizing through an automatic ratio height is not implemented.
- Local images with explicit math or ratio use the existing constrained
  object sizing path. Auto dimensions use the shared decoder's natural
  ratio unless an explicit ratio overrides it. Explicit width and height
  still win. object-fit/position, alt fallback, one imgdec workspace and
  64x64 natural/output ceilings are retained. No enlarged pixel buffer,
  second decoder, new image format, EXIF or network resource is added.
- Ordinary block horizontal auto margins consume positive free space for
  fixed/constrained widths; both sides center, a single start auto pushes
  toward the end. Overflow uses the start. Auto-width blocks keep their
  fill behavior. Vertical block margin collapsing is still absent.
- Grid auto margins override self alignment/stretch independently on each
  axis. Their actual area is the containing box. Measurement and painting
  share grid_item_width so a fixed or mathematical child width, including
  its ratio-derived height, does not reserve a row measured at another
  width. Flex still uses the same grow/shrink/freeze solver.
- Logical inline/block size and min/max size aliases, four logical margin
  edges, four logical padding edges, margin/padding-inline and -block
  pairs are supported in horizontal-tb / ltr mode only. Pairs accept one or
  two bounded pixel values (or auto for margins); they replace relevant
  auto flags atomically. Direction rtl and vertical writing modes are
  explicitly refused. Physical declarations retain source-order precedence.

## Fixed storage

Each of the 13 dimensional properties owns a fixed 194-byte expression in
its caller-owned node. All 160 nodes use 403520 bytes of expression storage.
The i386 document is 554176 bytes. WE_DOCUMENT_MAX=1 MiB is checked at
compile time. This is an explicit memory cost, not a sparse or heap-backed
representation. Re-style clears and reuses the slots; re-layout evaluates
against the new containing dimensions without growing the string pool.

WE_MAX_NODES=160, WE_DEPTH_MAX=24, 96 display items, the existing 512 KiB
static layout-workspace assertion and the existing image workspace ceiling
remain. The shared decoder still accepts at most 64 KiB source, 64 KiB
scratch and 64x64 dimensions. No allocation. Evaluation uses only the fixed typed length nodes above.

## Tests and proof

276 new cases in the existing test_webeng binary assert literal geometry,
text positions, image/box behavior, diagnostics, exact node/depth limits,
caller canaries, per-document ownership and repeated responsive layout.
Every previous assertion and refusal test is retained unchanged. Initial
new fixtures were corrected to specify an actual 22px border-box height
and to assert signed negative absolute coordinates; no assertion was removed.

The combined /web/lot58.html is itself read by a host geometry test. QEMU
keeps every lot 52..57 assertion, requires exact console box fragments for
calculated widths, percentage height and grid auto margins, then compares
the combined page's actual framebuffer pixels: ratio, calc flex-wrap/gaps,
clamp limits, grid auto margins, absolute insets, table and an initrd image.

Ten private mutation builds fail the new assertions when addition,
percentage resolution, minimum/clamp behavior, calculated flex basis,
aspect ratio, logical aliases, block/grid auto margins or math gaps are
removed. Mutations and their binaries live outside tracked sources.

US-031 remains partial. Full CSS, rational arithmetic, automatic ratio
height percentage cycles, intrinsic replaced-element conformance, subgrid,
named grid lines, baselines, full stacking contexts, static/fixed CSS
positions, merged table borders, margin collapsing, scrolling and sub-glyph
clipping remain incomplete. No JavaScript VM, network loading, shaping or
new font engine. Results and counts below are filled after actual execution.

## Measured local results

Actual make test-all summary, final implementation:

```text
Total Tests: 2269
Passed: 2269
Failed: 0
Skipped: 0
Execution Time: 159s
```

1128/1128 engine cases pass both i386 and native ASan/UBSan. Ten mutation
variants each exit 1 on the matching new behavior assertion. make all,
strict-image-check, secrets-check, roadmap-check and the guest-command
extractor check pass. Registry 447 names, GUI map 27 families/161 entries,
production initrd 104/128 entries, roadmap 69 entries. The complete exact
production assertion stays:

```text
prod-roadmap next US-076 US-078 US-080 US-081 US-097 US-G19 US-G20 US-G21 US-G22 US-G23 US-G24
```

Line counts come from git diff origin/main --numstat. Engine/header and
behavior tests add 5413 raw lines: 5114 nonblank/noncomment lines, 2
comment-only and 297 blank. Conservatively exclude all 6 lines of the
extracted grid item-width helper. Useful code: 5108 lines, comprising
461 engine/header and 4647 tests. No function is duplicated and no filler
rounds the count. Documentation/roadmap, the HTML fixture and the test
Makefile dependency do not count.

Actual QEMU console/framebuffer output, all old pages retained:

```text
lot58 console math box 35 0 50x25
lot58 console limits box 20 49 80x20
lot58 console grid-auto box 10 94 40x20
osui web render file=/web/lot58.html nodes 27 rules 16 boxes 22 lines 0 items 31 dropped 0 page 640x203 diag 0 components 0 media_kept 0 media_dropped 0 vars 0 sanitized 0
lot58 pixels compared=9066 math=typed ratio=bounded logical=ltr auto=block,grid flex=wrap table=colspan image=shared
QEMU web layout contract passed.
```

The PR records the post-commit clean-tree check, final GUI gate and actual
CI run/job conclusions after reading their results.
