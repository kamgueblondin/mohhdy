# Lot 64: vertical writing modes, direction rtl, translucent radial rings, arc patterns, background layers (US-031 partial)

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
