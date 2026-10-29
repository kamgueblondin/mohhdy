# Lot 57: grid, local image boxes and CSS cascade (US-031 partial)

Base: fresh origin/main 7daf38b, merged PR #213 (lot 56).
Branch: cursor/lot57-grid-images-d639. No merge and no force-push.

One large increment in userspace/webeng.c and its existing host/QEMU tests.
One shell, one console/GUI implementation, freestanding i386 Ring 3, no
heap. Kernel, rights and syscall ABI stay unchanged. US-031 stays partial;
phase3_complete=false. No Chromium, WebKit, network loading, JavaScript VM
or new font engine. US-059/060/063/074 unchanged; US-G19..US-G24 partial.

## Grid subset and limits

- minmax(pixel-minimum, pixel-maximum/auto/integer-fr) on either axis.
  Minimum and maximum accept 0..2000 pixels. A maximum below the minimum
  is raised to the minimum. Fractions stay 1fr..8fr. The fractional solver
  freezes minima above the common fraction unit before redistributing the
  remaining space. Unsatisfiable column minima retain their sizes and
  produce the existing overflow diagnostic.
- fit-content(pixel-cap) is a hard cap on the intrinsic contribution in
  this subset, including long words. This is not the full CSS fit-content
  algorithm with an automatic min-content floor. Auto/capped tracks grow
  toward content demands, not toward arbitrary remaining free space.
- Lists can mix fixed/auto/fraction/functional tracks and repeat groups.
  repeat may contain multiple tracks and minmax/fit-content. Nested repeat,
  percentage tracks, intrinsic keyword minima and subgrid remain refused.
  Parsing is atomic: a bad tail keeps all previous track descriptors.
- grid-auto-rows/columns use one bounded track descriptor on each axis.
  Defaults preserve the previous auto rows and equal-fraction columns.
- grid-auto-flow row/column, optionally dense in either token order.
  Row flow supports at most 160 implicit rows and 8 columns. Column flow
  fills the declared/area row extent (or one row), capped at 8, and creates
  at most 8 columns. Dense restarts at the first cell for every auto item.
  Explicit placements reserve occupancy first; order ties remain stable.
  Collisions and capacity exhaustion are diagnosed, never overwritten.
- grid-template-areas has at most 8 rows, 8 columns and 8 distinct names.
  Names are at most 15 ASCII characters. Rectangles, equal row lengths and
  capacities are checked before replacing the previous map. Dot runs mark
  empty cells. Named grid-area resolves against the immediate grid parent;
  unknown names are diagnosed and use auto placement. Numeric grid-column
  or grid-row clears a named placement; grid-area:auto also clears it.
  No numeric grid-area shorthand, named grid lines or CSS-wide keywords.

## Image and flex behavior

- Explicit object-fit opts local images into natural width/ratio sizing
  for intrinsic layout and flex content bases. The legacy unspecified-fit
  path retains its earlier independent width/height behavior and alt-based
  intrinsic contribution. No claim of complete replaced-element CSS.
- fill, contain, cover, none and scale-down use the existing imgdec pixels
  and the one shared workspace. Virtual cover sizes are arithmetic only;
  no enlarged image or second pixel buffer is allocated. Crop/letterbox
  pixels stay inside the bounded output box. Inline fit images can paint
  their declared background behind the object. Rectangle publication is
  preflighted atomically, including that background slot.
- object-position accepts horizontal/vertical keywords or integer 0..100%
  on the two axes; a single value centers the other axis. No pixel offsets,
  four-value edge syntax, animation, EXIF orientation or network resources.
  Negative crop offsets round toward zero using bounded integer arithmetic.
- The existing header ceilings remain: 64 KiB source, 64 KiB decode scratch,
  64x64 natural/output dimensions, and one 96-item display list. Failures
  keep alt painting and diagnostics. Multiple local images may be reread;
  at most the last decoded image is retained. Hidden display subtrees do
  not read images. Reader/context reset and document isolation are tested.
- Flex cross-axis margin:auto overrides align-self/items and suppresses
  stretch. Positive free space goes to the auto side, or splits between
  both sides. Negative free space uses the start. Physical margins remain
  physical under main-axis reverse and wrap-reverse. Main-axis grow/shrink,
  packing and the legacy equal-row/column behavior retain their tests.

## Selectors and cascade

- At most 4 selector compounds, each with a tag, id, up to 3 classes,
  2 supported attribute tests and one structural pseudo class. Descendant,
  child, adjacent and general sibling relations share the same matcher.
  Backtracking is capped at 4096 attempts per selector match, with a diagnostic
  on exhaustion, recursion at
  the 4-part chain. Matching eligibility is computed once per cascade pass.
- first-child, last-child, only-child, empty, root, nth-child,
  nth-last-child, nth-of-type and nth-last-of-type. nth supports odd/even,
  integer and An+B with coefficients/constants -160..160. Text nodes do
  not count as element siblings; hidden elements do. Empty attributes have
  separate presence flags. Whitespace-only content prevents :empty.
- Attribute names: id/class/href/src/alt only. Presence, =, ~=, ^=, $=, *=,
  quoted values and optional ASCII-insensitive i flag. No arbitrary DOM
  attribute store, escapes, namespace, |=, :not/:is/:has, hover or pseudo
  elements. Legacy case-folded id/class selectors remain compatible;
  attribute value comparison is case sensitive unless i is specified.
- Specificity uses bounded separate weights so any id outranks the maximum
  supported class/attribute/pseudo count. Equal specificity keeps source
  order. !important declarations form a second cascade pass; inline
  important wins over stylesheet important, stylesheet important wins
  over inline normal. Inherited computed values do not carry priority.
  This remains the author/UA subset, not all CSS origins/layers.

Static layout scratch is still checked against 512 KiB. New selector and
area descriptors live in the caller-owned, fixed-size document. No dynamic
storage, new shell command, syscall, rights relaxation or second engine.

## Proof strategy and remaining gaps

New cases use the existing test_webeng executable and assert literal box
coordinates, text paint, resource bounds, diagnostics, link hits and image
pixels. Repeated style/layout checks preserve the pool. The image reference
uses the actual initrd 48x32 stripe fixture and explicit expected crop/fit
rectangles. One fixture uses the combined guest HTML file itself.

Two obsolete refusal tokens become unsupported variants: minmax(auto,1fr)
replaces the now supported numeric minimum; p||q replaces the now supported
child combinator. Every prior assertion is retained. New positive tests
verify the corresponding supported values. Initial new fixture expectations
were corrected against the PNG IHDR (48x32), the non-dense row cursor and
the root rule's actual containing width; assertions were not removed.

QEMU keeps every lot 52..56 page assertion and adds /web/lot57.html with
flex wrap/cross auto, minimum-constrained tracks, dense holes, named areas,
a table and two decoded contain/cover images. Both console coordinates and
framebuffer pixels are required.

US-031 remains partial. Still missing: full CSS/intrinsic sizing, general
cyclic percentage resolution, subgrid/named grid lines, baseline alignment,
full stacking contexts, CSS static/fixed positions, merged table borders,
margin collapsing, scrolling and partial-glyph clipping. No JavaScript VM,
network loading, shaping or new font engine. Results and useful line counts
are recorded below only after the commands complete.

## Measured host results

make test-all, on the final implementation and 263 new engine cases:

```text
Total Tests: 1993
Passed: 1993
Failed: 0
Skipped: 0
Execution Time: 148s
```

852/852 engine cases pass both i386 and native ASan/UBSan execution.
Nine external mutation builds each exit 1 on a corresponding new assertion
when track floors, dense placement, named area resolution, object fit,
image intrinsic contribution, cross-axis auto margins, child combinators,
important cascade or empty-attribute presence are removed. No repository
source is mutated by those checks.

make all, strict-image-check, secrets-check, roadmap-check and
python3 scripts/extract_guest_commands.py --check pass. The full production
next-id assertion is retained verbatim:

```text
prod-roadmap next US-076 US-078 US-080 US-081 US-097 US-G19 US-G20 US-G21 US-G22 US-G23 US-G24
```

Line accounting uses git diff origin/main --numstat. Engine/header and
behavior tests add 5057 raw lines: 4768 nonblank/noncomment lines, 6
comment-only and 283 blank lines. Conservatively exclude all 18 lines of
the extracted existing cascade helper and the two obsolete refusal-token
replacements as reused work. The useful count is 4748: 702 engine/header
and 4046 tests. No function is duplicated, no second engine is introduced,
and documentation/roadmap, the HTML fixture and the Makefile dependency
are outside that count. No lines are added to round up to 5000.

Actual combined QEMU console/framebuffer output (all old pages retained):

```text
lot57 inspect wrapped box 0 14 56x10
lot57 inspect floor box 0 24 80x10
lot57 inspect hole box 80 34 40x10
lot57 inspect side box 40 64 80x20
lot57 inspect span box 0 84 80x10
lot57 inspect auto box 0 124 20x10
lot57 inspect contain box 0 134 24x24
lot57 inspect cover box 0 158 24x24
osui web render file=/web/lot57.html nodes 33 rules 20 boxes 27 lines 0 items 38 dropped 0 page 640x182 diag 0 components 0 media_kept 0 media_dropped 0 vars 0 sanitized 0
lot57 pixels compared=12432 grid=minmax,dense,areas flex=wrap,cross-auto table=colspan image=contain,cover cascade=important
QEMU web layout contract passed.
```

The PR records the post-commit clean-tree check, final QEMU/GUI gates and
CI run/job conclusions after reading their actual results.
