# Lot 95 plan - @property limits, more syntax types and computed values (reader)

Base: 3173f47 (Lot 94 head; its tree dea273b is the tree of c7a6440, the
head of PR 252). Branch cursor/lot95. One guest engine (userspace/webeng.c,
unchanged in this lot) and one shell path for the console and the
desktop. Kernel, rights and ABI unchanged. US-031 stays partial.
phase3_complete stays false. US-068/US-073, US-076, US-080 and US-081 stay
partial. US-078 and US-097 stay simule.

Scope taken from the Lot 94 first gaps, in order: gap 2 (@property limits
and syntax types) is taken whole except transitions and animations; gap 3
(the rest of math) is taken for plain dimensions, lists,
length-percentage mixes and viewport units; font-relative lengths (em,
rem, ex, ch), the 1/10000 step and infinity/nan stay out. Fractional
degrees and repeating gradients (Lot 94 gap 3, "if budget allows") are
left for lot 96 because the size budget is reached (see Not done).

A row is listed only when its code, tests and docs are in the branch.
Size: about 11300 added lines (522 removed) against 3173f47 with these docs, of which
about 7760 are generated host test suites
(tests/unit/userspace/test_lot95_{syntax,match,compute,sheet}.c, written by
scripts/lot95_propgen.py from the model scripts/lot95_propmodel.py, which
shares no code with the C under test, fixed seeds 9501 to 9504) and about
3550 are written by hand or regenerated (about 500 lines of product code in
wf_cssprop.c, wf_cssprop.h, wf_sitecss.c, wf_sitecss.h and wf_cssvar.h, the
model and generator, the hand test, the QEMU test and page, the CI job,
docs) and about 720 lines of converted suites (review point 6). This is above the 8000 to 10000 target (review point 11).

## Scope

| Id | Feature | Before | After |
|----|---------|--------|-------|
| A | @property limits (US-031, Lot 94 gap 1) | 4 alternatives in a syntax, names of 23 bytes, values of 63 bytes, 8 registrations, 24 custom property names and 64 bound values per sheet | 8 alternatives (WF_CSSPROP_ALTS), names of 31 bytes (WF_CSSPROP_NAME, WF_CSSVAR_NAME, WF_SITE_VNAME 32), registered and custom values of 127 bytes (WF_CSSPROP_VAL, WF_SITE_VVAL 128; a 128-byte custom value is counted as dropped, a 128-byte ordinary value as long, as before at 64/128), 16 registrations (WF_SITE_PROPS), 32 names per element, 96 bound pairs. A ninth alternative, a 32-byte name or a 128-byte value is refused as before. Proof: test_lot95_props.c, test_lot95_syntax.c, test_lot90_props.c and test_lot89_elemvar.c (converted), `make qemu-lot95` (nine registrations, eight alternatives, a 31-byte name, nine alternatives refused) |
| B | Syntax types (US-031, Lot 94 gap 1) | <url>, <image>, <resolution>, <transform-function>, <transform-list> and <string> made the rule invalid | wf_cssprop.c: <url> (url(...) quoted or unquoted, escapes, no quote, parenthesis, space or control byte unquoted), <string> (same quote at both ends, no bare newline or quote of that kind), <image> (a url or one of the six gradient functions with balanced parentheses and a non-empty body; stops not checked, review point 4), <resolution> (dpi, dpcm, dppx, x), <transform-function> (20 functions with their argument counts and kinds; no matrix3d) and <transform-list> (one or more functions; '+' or '#' after it refuses the syntax). Proof: test_lot95_syntax.c (520 syntaxes), test_lot95_match.c (1100 values), test_lot95_props.c, test_lot91_propsyn.c (converted) |
| C | Computed values (US-031, Lot 94 gap 2) | only math functions of one type were computed; 1in, lists, mixes and viewport units were kept as text | a registered value is computed for every numeric item: plain dimensions in canonical units (px, deg, s, dppx; 0 of a length is 0px), each item of a '+' or '#' list (joined by one space or ", "), length-percentage mixes inside calc() as "calc(P% + Lpx)" (min(), max() and the other functions of a mix are kept), vw/vh/vmin/vmax with the reader viewport (640 x 480), each argument of a transform function (a zero angle is 0deg). Text that does not change is not counted; every output is written only when its whole text and NUL fit (exact lengths, checked by a byte sweep). Proof: test_lot95_compute.c (900 values), test_lot95_sheet.c (160 sheets), test_lot95_props.c, test_lot92_compute.c, test_lot93_compute.c and test_lot90_vars.c (converted), `make qemu-lot95` |
| D | Reader sheet (US-031) | the sheet computed registered values without a viewport | wf_sitecss.c passes the reader viewport to the computing of initial and declared values; css-props "simplified N" counts declared values whose text changed. Proof: test_lot95_sheet.c, test_lot95_props.c (31-byte var() name through wf_site_report), `make qemu-lot95` (simplified 5) |

## Review points

1. Branch base. cursor/lot95 starts at 3173f47 (Lot 94 head), whose tree equals the tree of c7a6440 (head of PR 252). PR 252 is merged in main as e6b1d9c (13 November 2026, 23:50 UTC+4), so a PR against main (containing Lot 94 and Lot 93) shows only the Lot 95 commits. The mbox holds only the Lot 95 commits.
2. Structure sizes: wf_site_t grows from 416524 to 447372 bytes and wf_cssprop_t from 164 to 308 bytes (limits of row A). we_doc_t is unchanged (870824 bytes, < 1 MiB). New enum values WF_CSSPROP_URL = 13 to WF_CSSPROP_STRING = 18 and the new type wf_cssprop_ctx_t with wf_cssprop_compute_ctx(); no ABI structure (os_fb_web_item_t, syscalls) changes.
3. wf_sitecss.c parse_decl reads one byte more (val[WF_SITE_VAL + 1]) so that a custom property value of exactly 128 bytes reaches the custom-property path and is counted as dropped (it was counted as a long value before because both limits were 128); an ordinary value of 128 bytes or more is still counted as long by a new check right after the custom-property block.
4. <image> checks the gradient name and its balanced parentheses only; the stops are checked by the background reader when the value is used.
5. Approximations kept from Lot 92: values are fixed point in 1/10000; text-indent resolves a percentage against 16px, as the reader did before (the QEMU page's mix gives 97.28px, review row C). em, rem, ex and ch are typed but kept. Transform arguments accept max() and min() in C; the model generates calc() only.
6. Converted tests (assertions changed, none weakened; each now checks the new behaviour):
    - scripts/lot92_mathgen.py and scripts/lot93_mathgen.py (same seeds and random sequence): mixes are known inside calc(), plain dimensions are computed, a value is counted as simplified only when its text changes. Assertions changed: test_lot92_compute.c 109, test_lot92_propsheet.c 3, test_lot93_compute.c 69.
    - scripts/lot91_propgen.py: up to 8 alternatives are valid and <resolution>, <transform-list> and <url> are valid types now. test_lot91_propsyn.c: 65 cases (57 with five alternatives, 8 with the new types) are now valid.
    - scripts/lot90_vargen.py: a unitless 0 of a registered length is computed as 0px: test_lot90_vars.c 23 assertions.
    - test_lot89_elemvar.c: names v26 to v33, a 128-byte value, "names 32".
    - test_lot90_props.c: <resolution> is a type now (<frequency> stays refused), 31/32-byte names, 127/128-byte initial values.
    - test_lot91_props.c: 8 alternatives valid, 9 refused; a list is computed ("6px, 0px"); the refused rule now has nine alternatives.
    - test_lot92_props.c: 3 assertions on computed plain values; the "too small an output" check used a 4-byte buffer for "6px", which now fits exactly, so the refusal is checked at 3 bytes and the 4-byte case is checked to succeed.
    - test_lot93_math.c: 1 assertion (a mix inside calc() is known).
    - tests/scripts/test_qemu_lot91.py and scripts/lot67_web_server.py: the /lot91/props.html rule that was refused for five alternatives now has nine (five are read now), so test_qemu_lot91.py keeps its expectations; only its docstrings change.
7. Removed while writing the mutations, since no result could change: the fits check of cp_num_item (cp_known already covers it), the trailing-text check after a plain leaf (the item already matches its type, f45e51f), the length check of cp_image, the space check of cp_tfunc_end and the early capacity checks of the mix writer; all appends of computed text go through one helper (cp_put). The byte sweep of the hand test found that the transform writer refused a value that fit its buffer exactly (one spare byte); fixed in ec63a3f.
8. Mutations: 130 applied / 130 killed / 0 survived (129 host mutations of rows A to D, and one QEMU mutation: WF_CSSVAR_NAME 32 to 24, qemu-lot95 failed on #long95, the 31-byte name). The first host run on ec63a3f (130 mutations) left 10 survivors; f45e51f adds witnesses for nine (vmax without a viewport width, a px overflow undone by a later subtraction, unclosed transform functions such as "rotate(0degX", a percentage z in translate3d, an all-space transform list, a url ending in a backslash, a control byte in a url, a 31-byte name set and read on one element, and a byte sweep of as-written list items), all killed by a second run (9/9); the tenth (M95-cv-trail) could not change a result and its check was removed (point 7), so it is not counted (mutations.txt).
9. CI: one new job, lot95-reader (make qemu-lot95, no guest clock option). The pull_request trigger keeps no branches filter.
10. ASan: base and head fail the same 5 suites, no new failure (see Gates).
12. qemu-web-layout failed once in the gate run (timeout after lot56.html rendered) and passed when rerun alone; both results are in Gates.
11. Size: about 11300 added lines, above the 8000 to 10000 target; the generated suites are 7759 of them. Fractional degrees and repeating gradients were therefore not started (Not done 1).

## Not done (first gaps for lot 96)

1. Gradients: fractional degrees in the engine and repeating-linear/radial/conic gradients (Lot 94 gap 3, deferred here for size), per-pixel painting (32 cells), conic "from" in radians, conic children of vertical boxes, the vertical-rl radial centre, background-clip: text, a clip or origin per layer, background-attachment, fetched background images.
2. @property: transitions and animations of registered properties; <image> stops are not checked at registration (review point 4); no <frequency>, <ratio> or <flex>; more than 8 alternatives, 16 registrations or 127-byte values are refused.
3. Math: em, rem, ex and ch are kept as text (no font context in the sheet); min(), max(), clamp() and the other functions of a length-percentage mix are kept; fixed point with 1/10000 steps; radians past 100 are kept; infinity and nan are never known.
4. Autofill: three fixed summer time rules (no tz database) (unchanged).
5. PSL: 360 hand-written entries (unchanged).
6. NE2000: the FIFO holds 16 frames, the drain is skipped during an LLM session (unchanged).
7. Shaping: single node settings, no WAN (US-068/US-073 partial, unchanged).
8. US-080: no DDC, fixed safe area margin (unchanged).
9. US-081: no backlight, no watts, battery simulated (unchanged).

## Tests

Host tests: tests/unit/userspace/test_lot95_syntax.c (104 tests, 520 syntaxes), test_lot95_match.c (110 tests, 1100 values), test_lot95_compute.c (90 tests, 900 values), test_lot95_sheet.c (160 sheets), test_lot95_props.c (15 hand tests, including byte sweeps of every capacity). Generator scripts/lot95_propgen.py (seeds 9501..9504) with the independent model scripts/lot95_propmodel.py (Fractions). Converted generators: scripts/lot90_vargen.py, scripts/lot91_propgen.py, scripts/lot92_mathgen.py, scripts/lot93_mathgen.py.
QEMU: tests/scripts/test_qemu_lot95.py, `make qemu-lot95` (one boot at 1x): /lot95/props.html from the Lot 67 server (web-css). CI job lot95-reader.

## Gates on the box

Run on 12bd3a5 (the code of this lot; the later commit changes docs only) on 14 November 2026 (UTC+4), gate starts 00:54:12 to 04:53:22 (DONE 04:53), one QEMU guest at a time (scripts in /workspace/run/lot95: gates.sh). Failed: qemu-web-layout (see below; rerun alone once, it passed); every other gate passed and none was rerun.

| Gate | Result | Time |
|------|--------|------|
| make-all | pass | 1 s |
| test-all | pass | 269 s |
| roadmap-check | pass | 0 s |
| font-check | pass | 0 s |
| secrets | pass | 7 s |
| strict-image | pass | 1 s |
| qemu-lot95 | pass | 32 s |
| qemu-lot94 | pass | 40 s |
| qemu-lot93 | pass | 31 s |
| qemu-lot92 | pass | 467 s |
| qemu-lot91 | pass | 869 s |
| qemu-lot90 | pass | 1308 s |
| qemu-gui-text-scale | pass | 166 s |
| qemu-gui-parity | pass | 831 s |
| qemu-lot89 | pass | 663 s |
| qemu-p2p | pass | 1496 s |
| qemu-p2p-mesh | pass | 107 s |
| qemu-p2p-phase5 | pass | 606 s |
| qemu-net-wire | pass | 84 s |
| qemu-lot67-web | pass | 684 s |
| qemu-lot88-desk | pass | 686 s |
| qemu-lot88-tv | pass | 867 s |
| qemu-lot87 | pass | 209 s |
| qemu-lot86 | pass | 585 s |
| qemu-lot85 | pass | 154 s |
| qemu-lot84 | pass | 562 s |
| qemu-lot83 | pass | 108 s |
| qemu-lot82 | pass | 97 s |
| qemu-lot81 | pass | 360 s |
| qemu-lot80 | pass | 97 s |
| qemu-lot79 | pass | 534 s |
| qemu-lot78 | pass | 39 s |
| qemu-lot77 | pass | 25 s |
| qemu-lot76 | pass | 21 s |
| qemu-lot75 | pass | 91 s |
| qemu-lot74 | pass | 96 s |
| qemu-lot73 | pass | 235 s |
| qemu-lot72 | pass | 265 s |
| qemu-lot71 | pass | 685 s |
| qemu-web-layout | FAIL rc=2 | 901 s |
| qemu-production | pass | 71 s |
| tree-clean | pass | 0 s |

make test-all: 21520 tests, 21520 passed, 0 failed (Lot 94 head: 21041).

ASan and UBSan (TEST_CC="gcc -fsanitize=address,undefined -fno-sanitize-recover=all", make test-all on git archive copies of each tree): base 3173f47 20956 tests, 5 failed (test_gpt2_infer, test_pmm, test_vfs_service, test_lot87_web, test_webrt), 1140 s; head 12bd3a5 21435 tests, 5 failed (the same five), 1612 s. All Lot 95 suites and the converted suites pass under ASan; no new failure. The ASan runs overlapped the mutation run and the first gates.

qemu-web-layout: the gate run failed it (rc=2, 901 s): the harness timed out waiting for 'osui web close file=/web/lot56.html' after the page had rendered and its 1952 pixels had been compared (logs/gate_qemu-web-layout.txt; the log tail ends in the middle of that line). The page has no style rules or custom properties (render line: rules 0, vars 0), and Lot 95 changes no engine, shell or kernel code (webeng.c is unchanged). A rerun of make qemu-web-layout alone on 12bd3a5 after the gate run (04:57 to 05:13, logs/gate_qemu-web-layout-rerun.txt) passed in 952 s ("QEMU web layout contract passed"). Read as a likely timing flake (the guests were slower than in Lot 94 on this box: qemu-lot90 1308 s vs 597 s, qemu-p2p 1496 s vs 1109 s); both results are reported and the table keeps the failure. No base comparison was run because the rerun passed.

Mutations: 130 applied / 130 killed / 0 survived (129 host, 1 QEMU), see review point 8.
