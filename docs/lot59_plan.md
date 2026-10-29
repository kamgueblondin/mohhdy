# Lot 59: intrinsic widths, word breaks and bounded insets (US-031 partial)

Base: fresh origin/main ca5890f, merged PR #215 (lot 58).
Branch: cursor/lot59-intrinsic-layout-d639. No merge or force-push.

One increment in userspace/webeng.c and the existing host/QEMU test binary.
One shell and one console/GUI path, C freestanding i386, Ring 3. Kernel,
rights and shared syscall ABI unchanged. US-031 stays partial;
phase3_complete=false. No Chromium, WebKit, network loading, JavaScript VM
or new font engine. US-059/060/063/074 unchanged; US-G19..US-G24 stay partial.
Roadmap statuses and the exact complete production non-done list do not move.

## Delivered subset

- min-width and max-width accept min-content, max-content and fit-content.
  Width and both constraints also accept fit-content(length-percentage).
  The function clamps its argument between the measured min/max content
  sizes; the keyword uses available space after the node's edges/margins.
  A function can exceed the available width. Intrinsic keywords describe
  content; a border-box functional argument first subtracts the edges.
  Minima win contradictory maxima. The same limit resolver bounds flex
  growth and shrink freezes, including the unadorned flex path.
- Functional arguments accept integer px, percent and the existing typed
  calc/min/max/clamp expressions. Existing 24-node, four-level expression
  limits, 2000 literal, 1000000 intermediate and 16000 used-value bounds
  apply. The fit-content wrapper does not consume an expression level.
  Invalid arguments keep the previous declaration atomically and diagnose.
  Dynamic overflow diagnoses and uses max-content as the function argument
  fallback, still bounded by the intrinsic interval. No heap is added.
- flex-basis accepts those intrinsic keywords/functions, independent of
  an explicit width. Rows use the existing grow/shrink/freeze solver.
  Columns use the shared content-height formatter at the available width:
  block-axis min/max content coincide in this subset, so the function also
  uses that natural block height. Declared height is not the content basis.
  This is not complete CSS intrinsic/replaced-element sizing.
- Bottom-up intrinsic contributions honor fixed intrinsic width requests
  through ordinary blocks, inline-blocks, flex, grids and table tracks.
  General percentage intrinsic cycles remain unsupported. Existing
  prepass percentage rules are retained rather than adding a cyclic solver.
- overflow-wrap:anywhere adds emergency word breaks and lowers min-content
  to a glyph. break-word keeps its previous larger intrinsic minimum.
  word-break:break-all additionally fills a partially occupied line before
  breaking a word. Both inherit, can reset to normal and respect nowrap/pre.
  The same formatter measures and paints. Float-shortened bands, pre-line,
  line-height and sub-glyph available widths are tested. ASCII fixed glyphs
  remain 8x8, 8px advance. No shaping, hyphenation, Unicode or new font.
  keep-all and other unimplemented word-break values diagnose/refuse.
- Relative right/bottom offsets are implemented with left/top precedence,
  signed px/percent/math, and the existing containing-box clamps. Auto clears
  the inset flag and expression. Vertical relative motion still requires a
  definite immediate-parent height; otherwise the existing zero shift is
  retained. Sibling flow stays unshifted; descendants, painted items and
  link hits move together. Oversized boxes do not acquire unbounded motion.
- inset (one to four values), inset-inline/block (one/two), and four logical
  edge aliases use one atomic parser shared with physical insets. Values
  are auto, bounded signed integer px/percent, or existing typed math.
  Absolute positioning reuses its existing padding-box path. This tightens
  physical inset parsing: malformed tails, decimals and unsupported units
  are refused rather than partially consumed. Only horizontal-tb/ltr is
  supported, as before; RTL and vertical modes remain diagnosed/refused.

## Storage and implementation

Four style bytes per node increase the actual i386 document from 554176 to
554816 bytes, under WE_DOCUMENT_MAX=1 MiB. No new static workspace, heap,
image buffer, decoder or font is added. Each dimensional slot still holds
194 bytes; expression storage remains 403520 bytes. The static layout
workspace cap remains 512 KiB; depth 24, 160 nodes, 96 display items, fixed
string pools and one shared imgdec workspace remain unchanged.

Obsolete physical-inset and intrinsic-width parse branches are removed:
the common atomic handlers own those declarations. Width limits share one
resolver rather than copying the solver or introducing a second engine.

## Tests and validation contract

242 new cases in the existing test_webeng binary assert literal geometry,
painted text coordinates, refusal recovery, responsive/repeated layout,
style replacement, caller canaries, independent documents, flex freezes,
float exclusion, descendant/link movement and the actual combined initrd
fixture. No source-keyword tests. Every old assertion is retained.
Two old invalid fixtures named values now implemented (anywhere, left:auto).
They now use anywhere-extra and anchor(foo); all their previous geometry,
state-preservation and diagnostic assertions remain. New positive cases
separately prove the newly accepted values. Two new fixtures were corrected
to exercise an actual flex minimum freeze and explicit grid stretch;
no assertion was removed or reduced.

QEMU retains all lot 52..58 checks and adds /web/lot59.html through the same
web-render console and GUI web path. It requires exact rectangles for fit
width, wrapped intrinsic flex bases, logical relative offsets and a table,
then compares framebuffer regions and every pixel of the shared-decoder
48x32 initrd image. No alternate rendering implementation is tested.

US-031 remains partial. Still missing: complete CSS intrinsic cycles and
replaced-element conformance, baseline alignment, subgrid/named grid lines,
full stacking contexts, CSS static/fixed positions, collapsed table borders,
margin collapsing, scrolling, sub-glyph clipping, network/script loading,
a JavaScript VM and shaping/fonts. phase3_complete stays false.

Actual gate output and line accounting are appended after execution.

## Measured local results

Actual make test-all on the final engine and 242 new host cases:

```text
Total Tests: 2514
Passed: 2514
Failed: 0
Skipped: 0
Execution Time: 155s
```

1373/1373 engine cases also pass with native ASan/UBSan. Twelve private
mutation variants each exit 1 on the new matching behavior assertion:
intrinsic limits, fit floor/argument, intrinsic basis, break-all, anywhere
minimum/wrapping, relative right/bottom, auto inset clearing, logical
insets and nested intrinsic contributions. No source search is the oracle.

make all, strict-image-check, secrets-check, roadmap-check and the command
extractor pass. Registry 447 names, GUI map 27 families/161 entries,
production initrd 105/128 entries, roadmap 69 entries. qemu-gui-parity
passes, including driver restart and subsequent real storage/network use.

The first web pixel run caught an older embedded fixture without explicit
grid stretch. The stamp watched samples/images but omitted initrd_content/web.
That directory is now included. make check-initrd-web-stamp functionally
verifies added/edited/removed HTML bytes in both actual archives and reuse
when unchanged. The final web QEMU replay retains all pixel assertions.
The PR records its actual console/framebuffer output after completion.

The complete production non-done equality assertion remains unchanged:

```text
prod-roadmap next US-076 US-078 US-080 US-081 US-097 US-G19 US-G20 US-G21 US-G22 US-G23 US-G24
```

Line accounting uses git diff origin/main --numstat. Engine/header and
behavior tests add 5145 raw lines, 4898 nonblank/noncomment and 247 blank.
Conservatively exclude 24 lines of redistributed existing size logic and
both old refusal-fixture replacements (2). Useful added code is 4872:
191 engine/header plus 4681 behavior tests. No existing function is copied.
Documentation, the HTML fixture, build dependency rules and the independent
initrd-stamp regression script do not count toward the useful-code band.

The PR records the post-commit clean-tree check, web QEMU result and actual
CI run/job conclusions after reading their results. US-031 stays partial.
