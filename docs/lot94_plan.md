# Lot 94 plan - angled linear gradients, corners and conic gradients (engine and reader)

Base: 8bc8ef5 (Lot 93 head; its tree is the tree of 2455829, the head of
PR 251, which is merged in main as 2d27e88). Branch cursor/lot94. One
guest engine (userspace/webeng.c) and one shell path for the console and
the desktop. Kernel, rights and ABI unchanged. US-031 stays partial.
phase3_complete stays false. US-068/US-073, US-076, US-080 and US-081 stay
partial. US-078 and US-097 stay simule.

Scope taken from the Lot 93 first gaps, in order: gap 1 (reader
backgrounds: conic gradients and angles that are not quarter turns) is
taken with the engine drawing change it needed; background-clip: text, a
clip or origin per layer, background-attachment and fetched background
images stay out. Gap 2 (@property limits) and gap 3 (the rest of math)
are left for lot 95 because the size budget is reached with gap 1 (see
Not done).

A row is listed only when its code, tests and docs are in the branch.
Size: about 8350 added lines against 8bc8ef5 before these docs, of which
about 6000 are generated host test suites
(tests/unit/userspace/test_lot94_{cells,bgval,accept,sheet}.c, written by
scripts/lot94_gradgen.py from the model scripts/lot94_gradmodel.py, which
shares no code with the C under test, fixed seeds 9401 to 9403) and about
2350 are written by hand (about 380 lines of product code in webeng.c,
webeng.h, wf_cssbg.c and wf_cssbg.h, the model and generator, the hand
test, the QEMU test and page, the CI job, docs). The regenerated Lot 90
to 92 suites change 386 assertions (review point 7).

## Scope

| Id | Feature | Before | After |
|----|---------|--------|-------|
| A | Linear gradients at any angle (US-031, Lot 93 gap 1) | the engine drew top, right, bottom and left only; any other angle refused the gradient ("gradient refused", the colour stayed) | webeng.c: a new kind WE_GRAD_ANGLE keeps the angle in whole degrees 0..359 (deg, turn, grad, rad; negative and past 360 reduced; 44.5deg is 45, half away from zero); quarter turns keep the band kinds. The box is painted in at most 32 cells (WE_GRAD_CELLS): rows = isqrt(32 h / w) in [1, min(h, 32)], cols = min(32 / rows, w); each cell takes the colour at its centre. The position on the line is p = 1/2 + (dx sin a - dy cos a) / (w \|sin a\| + h \|cos a\|), sin from a 91-entry table (1/65536), computed in 64-bit integers with a shift-subtract division (no libgcc helper), rounded half up to 1/10000. Proof: test_lot94_cells.c (800 boxes), test_lot94_engine.c (4x4 by hand), `make qemu-lot94` |
| B | Linear gradients to a corner (US-031) | "to top right" and other corners refused the gradient | the corner is kept (grad_corner 1..4, bit 0 left, bit 1 bottom); sin and cos are taken as +-h and +-w of the box, so the line is perpendicular to the other diagonal as in CSS. Words in either order, any case; "to top bottom", "to left right" and three words refuse. Proof: test_lot94_cells.c, test_lot94_engine.c, `make qemu-lot94` (64x32 to top right) |
| C | Conic gradients (US-031, Lot 93 gap 1) | conic-gradient() was not a gradient (refused by the engine, dropped by the reader) | WE_GRAD_CONIC: "from ANGLE" (deg, turn, grad; not rad) and/or "at POSITION" (the radial position words, px or %), then stops "colour [N% \| Ndeg]" (Ndeg whole 0..360, a stop past 360deg or below 0 refuses). The turn of a cell centre is computed by CORDIC (31 steps, 1/2^32 turn), then p = (turn - from) mod 1. Same 32-cell grid. A refused conic gradient leaves the earlier gradient as it was. Proof: test_lot94_cells.c, test_lot94_sheet.c (240 sheets), test_lot94_engine.c, `make qemu-lot94` (from 90deg at 25% 75% with a 270deg stop) |
| D | Reader: any angle, radians, corners, conic-gradient (US-031) | the reader wrote only quarter-turn angles, radians within half a degree of a quarter turn, sides, no conic-gradient | wf_cssbg.c: deg (\|v\| <= 3600, 4 decimals), turn (<= 10) and grad (<= 4000, 2 decimals) are passed on; rad (2 integer digits, 4 decimals) is written as whole degrees rounded half away of x 180/pi; "to" a side or a corner; conic-gradient with from/at and degree stops; the shorthand class and the image layers take conic. Every gradient the reader writes is read by the engine as the model says (test_lot94_accept.c, 657 values). Proof: test_lot94_bgval.c (1164 values), test_lot94_accept.c, test_lot91_reader.c (converted), `make qemu-lot94` (/lot94/grad.html) |
| E | Painting rules kept (US-031) | bands, budget, transforms, translucent stops | the cells are display items like the bands: the gradient reserves 32 items and drops the unused ones, past the 96-item budget "display budget exceeded"; translucent stops are composed on the page; a transform takes the cells; "gradient painted in bands" is reported when the cells have two colours or more. A child of a vertical box is painted in the logical frame: its angle and corner are mapped there and back (vertical-lr transposes: 270 - a; vertical-rl rotates: a - 90, then a + 90), so the physical colours equal the horizontal ones; a conic child of a vertical box is refused ("conic refused in vertical flow", the colour stays). Proof: test_lot94_engine.c (budget, modes, vertical children, diag, transform) |

## Review points

1. Branch base. cursor/lot94 starts at 8bc8ef5 (Lot 93 head). PR 251 is merged in main as 2d27e88 and its head 2455829 has the same tree as 8bc8ef5, so a PR against main shows only the Lot 94 commits. The mbox holds only the Lot 94 commits.
2. we_node_t grows by 4 bytes (int16_t grad_angle, uint8_t grad_corner and padding: 4664 to 4668 bytes) and we_doc_t from 870184 to 870824 bytes, still below 1 MiB. webeng.h enum values WE_GRAD_ANGLE = 6 and WE_GRAD_CONIC = 7 are new; no ABI structure (os_fb_web_item_t, syscalls) changes.
3. Approximation: a gradient that is not a quarter turn is painted in at most 32 flat cells (one colour each, from the cell centre), not per pixel; the bands of quarter turns are unchanged. "gradient painted in bands" says so. The 32-item cost per gradient counts against the 96-item display budget.
4. Angles are whole degrees in the engine (sin table, 1/65536). The engine reads rad with its old factor 57.3 (573/10, unchanged) while the reader writes radians as whole degrees with x 180/pi (1e-12 units); the reader output never carries rad, so the page path is exact; an inline style with rad in the engine can differ by one degree past about 50rad.
5. conic "from" does not take radians in the reader or the engine (the reader writes radians only as a linear angle); degree stops are whole numbers 0..360 in the reader (at most 3 digits), the engine takes the tf_angle forms. The reader is stricter than the engine (it refuses what it cannot write exactly); test_lot94_accept.c checks that what it writes is read as the model says.
6. Vertical flows: the root of a vertical flow keeps its background physical (unchanged); its children are now mapped (row E). The radial "at" swap of earlier lots for vertical-rl is unchanged (it mirrors x; not part of this lot). A conic child of a vertical box is refused, because the transpose of vertical-lr reverses the sweep and the rotated centre of vertical-rl needs the physical width at style time (lot 95).
7. Converted tests (assertions changed, none weakened; each now checks the new behaviour):
    - scripts/lot90_gen.py, scripts/lot91_bggen.py, scripts/lot92_bggen.py and their suites (regenerated, same seeds and the same random sequence, same counts): angles that are not quarter turns, corners and conic-gradient are now passed on where they were dropped. Assertions changed: test_lot90_bgval.c 3, test_lot91_bgradial.c 81, test_lot92_bgrad.c 255, test_lot92_bgsheet.c 47; the others are identical.
    - test_lot91_reader.c: conic-gradient, 50grad, 100.0grad and 45deg are accepted (were dropped); new refusals (a conic gradient with one stop, 100.125grad, a 45px angle).
    - tests/unit/userspace/webeng_lot62_gradient.inc: the refused declarations are now "to top bottom" and a 45px angle (45deg and "to top right" are drawn); new assertions for the kept corner 2 and angle 45.
    - tests/unit/userspace/webeng_lot63_transform.inc: #d (45deg) is WE_GRAD_ANGLE 45 with 0 refused (was refused).
    - tests/scripts/test_qemu_lot93.py: #init93 now reads "bg #123456 bgimg linear-gradient(45deg, #ff0000, #0000ff)" (the computed 45deg is passed on and drawn).
8. Removed while writing the mutations, since no result could change: the axis special cases of grad_turn (CORDIC gives the same turns on the axes), the rounding branch for negative linear positions (the cell centre is inside the origin box, so the numerator is >= 0), the normalisation of negative angles in grad_sin_deg (callers pass 0..449), the three-word check of the reader to_sides (three words repeat an axis) and the -1 return of conic_shape for "from A x" (the stop check refuses it). Equivalent mutations kept out of the count: the 45deg sin entry (sin and cos scale together, the positions are ratios) and the reader 64-bit division remainder test (r >= d vs r > d: no value of the domain 0..999999 differs, checked exhaustively).
9. Mutations: see Gates (host mutations of rows A to E, and one QEMU mutation: the lower half of the CORDIC turn not turned, qemu-lot94 failed on the conic cells).
10. CI: one new job, lot94-reader (make qemu-lot94, no guest clock option). The pull_request trigger keeps no branches filter.
11. ASan (gcc -fsanitize=address,undefined): see Gates.

## Not done (first gaps for lot 95)

1. @property: at most four alternatives, no <url>, <image>, <transform-function> or <resolution> types, no transitions or animations of registered properties, 24 names and 64 values of 63 bytes (Lot 93 gap 2, unchanged).
2. Math: relative lengths and length-percentage mixes are kept as text; list values ('+', '#') and plain dimensions (1in) are not computed; fixed point with 1/10000 steps; radians past 100 are kept; infinity and nan are never known (Lot 93 gap 3, unchanged).
3. Gradients: per-pixel painting (32 cells now), fractional degrees in the engine, repeating-linear/radial/conic gradients, conic "from" in radians, conic children of vertical boxes, the vertical-rl radial centre, background-clip: text, a clip or origin per layer, background-attachment, fetched background images.
4. Autofill: three fixed summer time rules (no tz database) (unchanged).
5. PSL: 360 hand-written entries (unchanged).
6. NE2000: the FIFO holds 16 frames, the drain is skipped during an LLM session (unchanged).
7. Shaping: single node settings, no WAN (US-068/US-073 partial, unchanged).
8. US-080: no DDC, fixed safe area margin (unchanged).
9. US-081: no backlight, no watts, battery simulated (unchanged).

## Tests

Host tests: tests/unit/userspace/test_lot94_cells.c (200 tests, 800 boxes), test_lot94_bgval.c (146 tests, 1164 values), test_lot94_accept.c (83 tests, 657 values), test_lot94_sheet.c (60 tests, 240 sheets), test_lot94_engine.c (9 hand tests). Generator scripts/lot94_gradgen.py (seeds 9401..9403) with the independent model scripts/lot94_gradmodel.py (Fractions; a case within 1e-3 of a 1/10000 rounding boundary or 1e-6 of the conic wrap is not generated). Converted generators: scripts/lot90_gen.py, scripts/lot91_bggen.py, scripts/lot92_bggen.py.
QEMU: tests/scripts/test_qemu_lot94.py, `make qemu-lot94` (one boot at 1x): the local page /web/lot94.html (94 rectangles checked against the model) and /lot94/grad.html from the Lot 67 server (web-css). CI job lot94-reader.

## Gates on the box

Pending (filled by the gate run).
