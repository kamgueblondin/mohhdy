# Lot 77 plan

Base: origin/main 008ae17 (PR #233 lot 76). Branch: cursor/lot77.

## Chosen items (from lot 76 first gap)

### A. Richer type=file UX
Lot 75 gave WE_FORM_FILE, we_form_set_file (one absolute path), basename
paint from the bound state, and multipart. Still thin for real pages:

1. accept= on type=file: store the token list; we_form_set_file refuses a
   path that does not match (extension tokens like .png, or MIME groups
   image/* text/* audio/* video/* and a few exact types mapped to
   extensions). Empty accept accepts all. Host tests use local paths only
   (no DNS, no MIME sniff of file bytes).
2. multiple: with the multiple attribute, up to WE_FFILE_MAX_N (4) paths
   per control on we_forms_t; we_form_set_file appends (full => refused);
   without multiple, set replaces the single path. multipart emits one
   part per path (same name). Paint shows basename, or "name +N" when
   more than one.
3. clear / replace: we_form_clear_file; empty path clears; Escape on a
   focused file control clears; set_file again replaces (or appends when
   multiple). we_form_file_count / we_form_file_at for tests.
4. Name display: keep Browse... placeholder when empty; after bind show
   basename(s) as above.

### B. CSS grid subset (documented + proved)
Engine already has display:grid, tracks (px/fr/auto/minmax/repeat/
auto-fill/fit), gap, grid-column/row placement and span, auto placement.
Lot 77 locks the subset the reader may rely on with systematic host
tests and converts any old "refused" expectations that now pass:

- display:grid
- grid-template-columns: px, fr, repeat(N, ...), mixed lists
- gap / row-gap / column-gap
- grid-column: N / span N / N / M
- simple auto placement (row-major)

Out of this lot: dense packing, masonry, animation, subgrid changes
(already lot 61), Chromium/WebKit, JS VM, new font engine, margin
collapsing.

### C. More layout where it fits
While writing grid proofs, add covered tests for adjacent layout already
present (flex gap with grid siblings, inline-block in a grid cell) only
where a gap shows up. No new layout engines.

## Constraints (unchanged)
Single engine in userspace/webeng.c and .inc; freestanding i386 Ring 3;
we_doc_t < 1 MiB; 7-bit ASCII only; US-031 partial / phase3_complete
false; mutations for every new branch; never weaken assertions; host
tests no DNS.

## Delivery
- Code: webeng.h / webeng_forms.inc / webeng_finput.inc (+ tiny layout
  only if a real gap appears).
- Tests: webeng_lot77_*.inc wired in test_webeng.c.
- QEMU: initrd_content/web/lot77.html, test_qemu_lot77.py, make
  qemu-lot77, CI job lot77-forms.
- /workspace/run/lot77/: plan mirror, mutations.txt, mbox, PR_*, FINAL.
- Target +8000..10000 useful lines.
