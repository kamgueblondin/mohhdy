# Lot 64: vertical writing modes, direction rtl, translucent radial rings, arc patterns, background layers, multicol, per-side borders (US-031 partial)

Base: origin/main 9156445 (merge of PR #220, lot 63).
Branch: feat/lot64-webeng-layout. No push, no merge, no force-push.

One console/GUI engine in userspace/webeng.c, C freestanding i386 Ring 3.
Kernel, rights and the shared ABI (os_fb_web_item_t) are unchanged; every
effect still resolves to opaque RECT, BORDER and TEXT items. US-031
partial; phase3_complete=false. No Chromium, WebKit, network loading,
JavaScript VM, new font engine or margin collapsing. US-059/060/063/074
unchanged; US-G19..US-G24 partial. Roadmap statuses and the complete
production non-done list stay intact (prod-roadmap next US-076 US-078
US-080 US-081 US-097 US-G19 US-G20 US-G21 US-G22 US-G23 US-G24).
we_doc_t stays under 1 MiB (see the PR body for the measured size).

## Scope (in order of value)

A. writing-mode vertical-rl / vertical-lr for block containers: the
   content is laid out in logical space (inline axis down) and mapped to
   physical space; glyphs are stacked upright (no font rotation).
B. direction: rtl subset: start/end alignment, inline-start logical
   properties, flex rows, grid and table columns from the right, list
   markers on the right (no bidi reordering of characters).
C. Translucent radial-gradient stops: disjoint rings within the 96 item
   list instead of overlapping ellipses.
D. Dotted and dashed patterns along rounded corner arcs.
E. Background layers (comma lists of url() images), background-repeat
   space and round, list-style-image url().
F. Multi-column blocks (column-count, column-width, column-gap,
   column-rule) as a bounded subset, if room remains.

Excluded or skipped (reasons in the sections below): margin collapsing
(brief), hyphens:auto (needs a dictionary), bidi reordering.

## Round 2 (same branch)

G. Per-side borders on boxes outside collapsed tables: border-<side>,
   border-<side>-width/-style/-color, two to four value lists of
   border-width/-style/-color. Each side keeps its width, style and
   colour; the box edges use them and the painter draws the top and the
   bottom across the box, the left and the right between them. With a
   border radius the sides are painted square ("edges radius painted
   square"). A side without a style has no width (CSS: none).
H. Logical borders: border-inline-start/-end, border-block-start/-end and
   their -width/-style/-color, the pairs border-inline / border-block
   (one or two values, atomic: a refused value changes neither side), and
   border-start-start/-start-end/-end-start/-end-end-radius, mapped under
   the element's writing-mode and direction.
I. The dir attribute (ltr, rtl; auto taken as ltr since there is no bidi,
   "dir auto taken as ltr"); a CSS direction wins over it.
J. background-position with edge offsets (three or four values: "right
   3px bottom 2px", "bottom 10% right", "center top 2px") and the
   background-position-x / -y longhands (comma lists per layer).
K. column-span: all (the spanner is laid out at the full width and cuts
   the content into segments, each balanced on its own) and forced column
   breaks (break-before / break-after: column; auto, avoid and
   avoid-column accepted; break-inside auto/avoid/avoid-column accepted,
   since boxes are never fragmented).
L. rtl lists and indent: the UA ul/ol padding is padding-inline-start
   (the right in rtl), the marker is painted right of the box with its
   full stop first (".1"), a list-style-image marker likewise, and
   text-indent indents the right end of the first line.

## Limits (documented, diagnosed where visible)

- Vertical modes: glyphs are upright only ("vertical text painted
  upright"); sideways needs rotated glyphs (refused). The intrinsic size
  of a vertical root is estimated as lines times the line height. Nested
  writing modes take the root's ("nested writing mode inherited").
  Percentage and keyword sizes inside stay physical ("vertical flow size
  kept physical"); background images and images inside are refused.
- rtl: no bidi reordering of characters; vertical rtl puts the lines at
  the end with the glyph order kept.
- Translucent radial stops use up to 64 slots; when the disjoint rings do
  not fit the nested rings are painted opaque ("radial alpha painted
  opaque").
- Arc patterns: at most 4 periods per arc; an arc shorter than one
  period stays solid.
- Multicol: boxes are never fragmented between columns; the top margin
  of a column is not truncated; a spanner percentage width resolves
  against the column ("column span width on the column"); fill and the
  definite height apply only without spanners; vertical multicol is
  ignored.
- Per-side borders with a radius are painted square.

## QEMU

- /web/lot64.html combines A-L (vertical-rl with border-block-start, rtl
  paragraph and list, two columns with a rule and a spanner, per-side
  styles, edge offset position, two layers, dotted rounded border);
  tests/scripts/test_qemu_web_layout.py checks its console items and its
  pixels in the GUI pane; the same page is asserted on the host
  (webeng_lot64_page.inc).

## Adapted fixtures

See /workspace/run/lot64/ADAPTED.md (copied in the PR body): values that
became valid are replaced by values still invalid, with the same counts;
arc expectations follow the new patterns.

## Remaining gaps (first one first)

1. Margin collapsing (excluded by the brief).
2. Within the brief: fragmentation of a box across columns (a block
   taller than the column moves whole; CSS splits its lines).
3. Rotated glyphs (sideways, mixed orientation), bidi reordering.
4. hyphens:auto (needs a dictionary), 3D transforms, a hit test.
5. Multi-column in vertical modes, per-side borders with rounded corners.

## Results

See /workspace/run/lot64 (gate logs, mutations.txt, PROGRESS.md) and the
PR body for the measured counts.

