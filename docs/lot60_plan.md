# Lot 60: first baselines and named grid lines (US-031 partial)

Base: fresh origin/main 08d277c, merged PR #216 (lot 59).
Branch: cursor/lot60-baselines-grid-lines-d639. No merge or force-push.

One console/GUI engine in userspace/webeng.c, C freestanding i386 Ring 3.
Kernel, rights, shared syscall ABI and shell unchanged. US-031 partial;
phase3_complete=false. No Chromium, WebKit, network loading, JavaScript VM
or new font engine. US-059/060/063/074 unchanged; US-G19..US-G24 partial.
Roadmap statuses and its complete production non-done equality stay intact.

## Named grid lines

- Both template axes accept bracketed names between existing track tokens,
  including before/after repeat groups. At most eight explicit tracks and
  nine lines per axis, with two aliases per line and 15-byte ASCII names.
  Names start with a letter or underscore; subsequent digits/hyphens are
  accepted. Lookup is case sensitive; duplicate names on different lines
  resolve to the first match. Duplicate aliases on one line, empty groups,
  reserved CSS words, excess aliases and repeated brackets at one line are
  refused. Names inside repeat groups are outside this subset and refused.
- Existing fixed/fr/auto/minmax/fit-content and mixed repeat track parsing
  is reused. A group expands into local eight-element arrays before copying
  into the remaining track capacity. Bad tails never change old sizes,
  floors, caps or names. Replacing the template with unnamed tracks clears
  names. No string pool growth, heap or separate grid engine.
- grid-column and grid-row accept named starts/ends, named/numeric mixes,
  named-start / span N, span N / named-end, auto endpoints and negative
  explicit-line indices (-1 is the last explicit line). Existing positive
  numeric placements retain their legacy parser and bounds. Spans remain
  1..8. Endpoint names are fixed caller-owned strings, resolved each layout
  against the direct grid parent. Missing names, impossible resolved spans
  or out-of-range negative indices diagnose and use auto / span 1.
  Syntactically invalid declarations keep the previous request atomically.
- Occurrence-qualified names, named spans, implicit area-name line aliases,
  general grid-area shorthand, auto-fill/auto-fit and subgrid remain absent.
  Existing named areas and their precedence path are retained unchanged.

## First baseline subset

- align-items/align-self accept baseline and first baseline for row flex
  and single-row-span grid items. The current fixed glyph painter places
  an 8px glyph at line y+1; WE_TEXT_BASELINE=9 is its bounded baseline
  metric. No font loading, shaping or change to existing line-height paint.
- The existing formatter exports the first inline line during a bounded
  measurement probe, including nested ordinary blocks, explicit heights,
  padding, borders, top margins, wrapping and preceding empty blocks.
  The measurement cache includes the probe mode and captured baseline,
  preventing a fixed-height or earlier content-basis cache hit from hiding
  the first line. Existing depth and global measurement budgets still apply.
- A row flex line reserves max ascent plus max descent among participants,
  then offsets items to the shared baseline. Each wrapped line has its own
  group. Reverse rows preserve the baseline and existing order. Grid groups
  items per row and reserves an automatic row's shared ascent/descent before
  track sizing. Fixed row descriptors retain their existing fixed-size rule;
  an overflowing baseline group can therefore paint outside that fixed row.
- Cross-axis auto margins override baseline and exclude the item from its
  group. Empty boxes, overflow:hidden and nested flex/grid/table containers
  synthesize the bottom border edge, excluding the bottom margin. A replaced
  image likewise uses its existing measured bottom edge and one imgdec.
- Column flex baseline and row-spanning grid baseline diagnose and use start
  alignment. Last-baseline, vertical writing modes, table-cell baseline,
  inline-block line alignment and full nested flex/grid baseline export are
  still missing. The old inline-block top-alignment path is unchanged.

## Bounded storage

Each node owns two 9x2x16-byte name tables and two fixed endpoint requests.
WE_DOCUMENT_MAX remains 1 MiB and WE_LAYOUT_WORKSPACE_MAX remains 512 KiB;
both compile-time assertions pass. Eight tracks, depth 24, 160 nodes, 96
paint items, fixed CSS/text pools and existing math/image caps remain.
There is no new allocator, decoder, font engine or unbounded recursion.
Actual i386 document size and results are recorded below after measurement.

## Tests and acceptance

241 new cases in the existing test_webeng binary: every bounded named
start/end pair on both axes, negative indices over 2..8 explicit tracks,
span/auto derivation, aliases, atomic refusals, first-line ascent/descent
geometry, nested content, auto-margin precedence, wrap/reverse, caller
canaries and repeated style/resize. Every old assertion is retained intact.
The old refusal for 8 / span 2 initially caught a parser fallthrough; that
cause was fixed rather than changing the test.

QEMU retains all lot 52..59 assertions and adds /web/lot60.html through the
same console web-render and GUI web path. Literal rectangles prove flex
and grid baselines, aliases/spans, negative lines, a table and a shared
initrd image. Framebuffer comparisons include every output image pixel.
The host test reads that same actual HTML fixture.

US-031 remains partial, phase3_complete=false. Full CSS/intrinsic percentage
cycles, complete baseline/replaced-element conformance, subgrid, full
stacking contexts, static/fixed CSS positions, collapsed table borders,
margin collapsing, scroll and sub-glyph clipping remain incomplete. No
JavaScript VM, network/script loading, new fonts or Chromium/WebKit.

Measured results and line accounting follow actual execution.

## Measured local results

Actual make test-all:

```text
Total Tests: 2755
Passed: 2755
Failed: 0
Skipped: 0
Execution Time: 157s
```

1614/1614 engine cases also pass with native ASan/UBSan. Eleven private
mutation builds each exit 1 on the matching new geometry assertion:
name resolution, negative indices, spans, aliases, request application,
baseline parsing, flex/grid offsets, ascent/descent height, nested export
and first-line capture. Existing assertions and invalid fixtures unchanged.

The actual i386 document is 657856 bytes (node 3712, expression 194), under
its unchanged 1 MiB ceiling. Expression storage remains 403520 bytes.
The unchanged 512 KiB static layout assertion passes in the guest build.

make all, strict-image-check, secrets-check, roadmap-check,
check-initrd-web-stamp and the guest-command extractor pass. Registry 447,
GUI map 27 families/161 entries, production initrd 106/128 entries,
roadmap 69 entries. The complete exact production assertion remains:

```text
prod-roadmap next US-076 US-078 US-080 US-081 US-097 US-G19 US-G20 US-G21 US-G22 US-G23 US-G24
```

Line counts use git diff origin/main --numstat. Engine/header and behavior
tests add 5413 raw lines: 5168 nonblank/noncomment, 245 blank, 0 comment-only.
Useful additions are 5168 = 242 engine/header + 4926 behavior tests.
No existing function is copied and no filler rounds the total. Docs,
roadmap comments, HTML fixture and the Makefile dependency are excluded.

The PR records the post-commit clean-tree check, final GUI/web QEMU output
and actual CI run/job conclusions after reading them. US-031 stays partial.
