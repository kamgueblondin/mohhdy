# Lot 91 plan - reader radial radii, gradians and background boxes; @property syntaxes and calc() types; autofill summer time and expiry before offers

Base: 0576849 (Lot 90 head with the qemu-gui-text-scale fix, branch
lot90-reader-autofill-psl-rxq; main is bf79580, Lot 89). Branch
cursor/lot91. One guest engine (userspace/webeng.c, unchanged in this lot)
and one shell path for the console and the desktop. Kernel, rights and
ABI unchanged. US-031 stays partial. phase3_complete stays false.
US-068/US-073, US-076, US-080 and US-081 stay partial. US-078 and US-097
stay simule.

A row is listed only when its code, tests and docs are in the branch.
Size: about 24700 added lines, of which about 20700 are generated host
test suites (tests/unit/userspace/test_lot91_{bgradial,bgbox,propsyn,calc,
propmatch,proprule,dst,dststore}.c and the regenerated
test_lot90_bgsheet.c, written by scripts/lot91_*gen.py and lot90_gen.py
from Python models that do not share code with the C under test, fixed
seeds) and about 4000 are written by hand (code, generators, hand tests,
QEMU test, docs).

## Scope

| Id | Feature | Before | After |
|----|---------|--------|-------|
| A | Reader backgrounds: explicit radial radii, gradians, background-clip and background-origin (US-031, Lot 90 gap 1) | the reader refused a radial-gradient with a radius (only shape and extent keywords); angles only in deg and turn; background-clip and background-origin were dropped, and the box keywords of the background shorthand were accepted and thrown away, although the engine already draws explicit radii (radial_shape), 100grad steps (tf_angle) and one clip box and one origin box per element | wf_cssbg: radial-gradient sizes as the engine draws them: circle with one length (px or 0, not a percentage), ellipse with two lengths or percentages; with no shape word one length is a circle (a lone percentage is refused) and two are an ellipse; radii adjacent, never with an extent keyword, never negative. linear-gradient angles in gradians (an integer multiple of 100grad, at most 4000grad). wf_cssbg_box: background-clip and background-origin take one of border-box, padding-box, content-box, or a list of up to four that all name the same box (text is refused). The shorthand passes its boxes: one keyword sets both, two set the origin then the clip, none gives padding-box / border-box; layers that end in different boxes make the declaration invalid (the engine paints every layer in one box). wf_sitecss: pool kinds WF_SITE_K_BGCLIP 23 and WF_SITE_K_BGORIGIN 24 written into the style attribute as background-clip: and background-origin:. Proof: test_lot91_bgradial.c (840 generated cases), test_lot91_bgbox.c (700), test_lot91_reader.c (10), test_lot91_engine.c (6: the reader output fed to webeng draws the radius, the 100grad angle and the boxes), test_lot90_bgsheet.c (converted), `make qemu-lot91` (/lot91/bg.html) |
| B | @property syntaxes and calc() types (US-031, Lot 90 gap 2) | a syntax was one type name; '+', '#', '|' and keywords made the rule invalid, <angle>, <time> and <custom-ident> were refused; inside a registered property any balanced calc() matched a numeric type, so calc(10px + 5%) passed as a <length> | wf_cssprop_syntax_parse: up to four alternatives separated by '|', each a type name (<length>, <percentage>, <length-percentage>, <number>, <integer>, <color>, <angle>, <time>, <custom-ident>) or a keyword (an identifier, not a CSS-wide keyword or default), with an optional '+' (space list) or '#' (comma list). wf_cssprop_match_prop tries the alternatives in order and splits lists outside parentheses. wf_cssprop_math_type types calc(), min(), max() and clamp() by the CSS Values 4 rules (+ and - need the same type, length with percentage gives length-percentage, * needs a number on one side, / a number on the right, white space around + and -, clamp takes three, nesting at most 16); a numeric syntax matches on that type. wf_sitecss checks registered values with match_prop. Proof: test_lot91_propsyn.c (600 generated syntaxes), test_lot91_calc.c (916 expressions), test_lot91_propmatch.c (1560 values), test_lot91_proprule.c (400 rules through the sheet), test_lot91_props.c (6), test_lot90_props.c (converted), `make qemu-lot91` (/lot91/props.html) |
| C | Autofill summer time and expiry before offers (US-031, Lot 90 gap 4) | the local day used a fixed offset (no summer time); the automatic expiry ran only when a value was stored or a setting changed, so after midnight a value past its limit was still offered until the next store | wf_autofill: a summer time rule on top of the standard offset (`autofill dst eu|us|au|off`): eu from the last Sunday of March 01:00 UTC to the last Sunday of October 01:00 UTC; us from the second Sunday of March 02:00 local standard time to the first Sunday of November 01:00 standard (02:00 summer); au (south) from the first Sunday of October 02:00 standard to the first Sunday of April 02:00 standard (03:00 summer). wf_autofill_dst_offset, wf_autofill_day_rule (rule 0 is exactly wf_autofill_day_tz), wf_autofill_set_dst, _dst_parse, _dst_name. The session file header and the status line gain " dst R" only when a rule is set. shell_desk: the local day uses the rule; the expiry runs before every offer list (desktop list, key fill, apply, `autofill offers`). Proof: test_lot91_dst.c (2465 generated instants, the model checked against the tz database at generation time on 3252 instants for Europe/Paris, Helsinki, London, America/New_York, Denver, Los_Angeles, Australia/Sydney, Adelaide), test_lot91_dststore.c (160 sessions, 120 headers), test_lot91_fill.c (7), `make qemu-lot91` (guest clock at 2026-07-01 10:30 UTC: eu and us move the local date across midnight, au and off do not; a value from the day before is offered at 23:59 and forgotten by the first offer list after midnight) |

## Review points

1. Branch base. cursor/lot91 starts at 0576849, so a PR against main (bf79580) also carries the Lot 90 commits and the qemu-gui-text-scale fix (Lot 90 review point 16: tests/scripts/test_qemu_gui_text_scale.py measures from the page top, a converted test whose assertions are unchanged). Those commits are not rewritten.
2. Background shorthand: Lot 90 accepted the box keywords and dropped them; they are now passed on (row A). A shorthand whose layers end in different boxes is now refused as a whole (Lot 90 accepted it), since the engine has one box per element.
3. wf_site_t grows from 412864 to 416520 bytes (two more pool kinds, seven rules per declaration). we_doc_t and webeng.c are unchanged. WF_SITE_K_ALL moves from 23 to 25 and WF_SITE_K_VAR from 24 to 26 (internal codes, not stored).
4. wf_cssprop: new syntax codes ANGLE 8, TIME 9, CIDENT 10, KEYWORD 11, COMPLEX 12; wf_cssprop_t gains nalt and alt[4] (each alternative a 16-byte keyword). calc() inside a registered numeric property is now type checked: calc(10px + 5%) no longer matches <length> (Lot 90 took it).
5. wf_autofill_t gains an int dst (16808 -> 16812 bytes). A session file saved with a rule has " dst R" in its header; a Lot 90 build refuses that header (its loader takes only expire and tz), so going back to a Lot 90 build after setting a rule loses the session file. Without a rule the file and the status line are byte for byte the Lot 90 ones.
6. shell_desk: fill_clock (an RTC read and, on a new day, the expiry) now runs before every offer list; the general autofill usage line gains "dst eu|us|au|off" (no test asserted the old line).
7. tests/scripts/test_qemu_lot88.py boot() takes an optional extra list of QEMU arguments (default none, so Lot 88 and Lot 90 boots are unchanged). test_qemu_lot91.py uses -rtc base=2026-07-01T10:30:00,clock=vm and polls the guest clock until local midnight (four to five minutes of the run; see point 12).
8. Converted tests (assertions changed, none weakened; each now checks the new behaviour):
    - scripts/lot90_gen.py and test_lot90_bgsheet.c (regenerated, same seed and the same random sequence): the model passes the box keywords of the shorthand (clip and origin checked) and refuses layers that end in different boxes (was: keywords accepted and not passed on).
    - test_lot90_props.c test_syntax: '<length>+', '<length>#' and '<length> | auto' are COMPLEX and '<angle>' is ANGLE (were BAD); new refusals '<length> +', '<length>+#', '<length> |' and '<resolution>'. test_parse: the unsupported-type case uses '<resolution>' (was '<angle>').
9. Mutations: 44 host mutations (rows A, B, C) and one QEMU mutation (no expiry before `autofill offers`), all killed. M-prop-alts-5 (a fifth alternative) is killed by a crash: it writes past alt[4], which the bound protects. Two equivalent mutants found on the way were resolved in 76085e3: a space check in a syntax alternative that could not fail was removed, and a test now pins rule 0 of wf_autofill_day_rule to wf_autofill_day_tz.
10. CI: one new job, lot91-reader (make qemu-lot91). The pull_request trigger keeps no branches filter.
11. ASan (gcc -fsanitize=address,undefined) on base 0576849 fails 5 suites, the ones Lot 90 listed: test_gpt2_infer, test_pmm, test_vfs_service, test_webrt and test_lot87_web.

12. qemu-lot91 expiry wait. The first gate run of qemu-lot91, on e6c3e3f, failed at "the value one day old is offered": the tz step put local midnight 37 s ahead and typing the tz and offers lines took longer, so the first offer list already ran on 2 July and (rightly) forgot the value. 1dff139 sets local midnight four whole minutes ahead, checks with date that the first list ran before midnight (else it fails as a slow setup, not as a product fault), then waits until the guest clock has crossed midnight. A later run on 1dff139 failed at "guest clock too late for the tz -720 step: 755 min": QEMU's default -rtc clock=host follows the host wall clock, so a pause of the host jumped the guest from 10:30 to 12:35 UTC. The test now uses -rtc base=2026-07-01T10:30:00,clock=vm and polls the guest date until local midnight, failing only if that clock stops advancing. Assertions unchanged. That run passed in 457.7 s.

## Not done (first gaps for lot 92)

1. Reader backgrounds: conic gradients, angles that are not a multiple of 90deg (or 100grad, or a quarter turn), rad, background-clip: text, a clip or origin per layer, background-attachment; the engine does not fetch background images.
2. @property: at most four alternatives, no <url>, <image>, <transform-function> or <resolution> types, no transitions or animations of registered properties, no bounds on custom property counts beyond Lot 90 (24 names, 64 values of 63 bytes).
3. calc(): typed but not simplified for registered values (the text is kept); no sign(), abs(), round(), mod() or trigonometric functions.
4. Autofill: three fixed summer time rules (the eu, us and au rules of 2026 applied to every year 1970..2199, no tz database, no historical changes); expiry before offers needs a new local day, as before.
5. PSL: 360 hand-written entries, not the full list (unchanged).
6. NE2000: the FIFO holds 16 frames and the drain is skipped during an LLM session (unchanged).
7. Shaping: single node settings, no WAN (US-068/US-073 partial, unchanged).
8. US-080: no DDC, the mode set is unchanged, the safe area margin is fixed at 2.5% (unchanged).
9. US-081 unchanged: no backlight, no watts, battery simulated.

## Tests

Host tests: tests/unit/userspace/test_lot91_bgradial.c (140 tests, 840 cases), test_lot91_bgbox.c (500 tests, 700 cases), test_lot91_reader.c (10), test_lot91_engine.c (6), test_lot91_propsyn.c (150 tests, 600 cases), test_lot91_calc.c (151 tests, 916 cases), test_lot91_propmatch.c (260 tests, 1560 cases), test_lot91_proprule.c (80 tests, 400 rules), test_lot91_props.c (6), test_lot91_dst.c (50 tests, 2465 cases), test_lot91_dststore.c (161 tests), test_lot91_fill.c (7). Generators: scripts/lot91_bggen.py (seeds 9101, 9102), lot91_propgen.py (9111..9114), lot91_dstgen.py (9121, 9122).
QEMU: tests/scripts/test_qemu_lot91.py, `make qemu-lot91` (one boot at 1x, guest clock 2026-07-01 10:30 UTC with -rtc clock=vm): /lot91/bg.html, /lot91/props.html, `autofill dst` near local midnight, expiry before offers across local midnight. CI job lot91-reader.

## Gates on the box

All on the box, 12 and 13 November 2026 (times Indian/Mauritius). There is no arm-none-eabi-gcc on the box, so the ARM image is built in CI only.

Run 1, gates.sh starting at e6c3e3f (one guest at a time, 16:40 on 12 November):
- make all; make test-all: 19056 tests, 0 failed, lot67-web-host 64/64 (244 s).
- roadmap-check, font-check, secrets-check, strict-image-check: all pass.
- qemu-lot91: FAILED in 223 s at "the value one day old is offered" (review point 12, then 1dff139).
- Passed: qemu-lot90 495 s, qemu-gui-text-scale 513 s (page top y 112, web glyph-run p98 10 -> 13), qemu-lot89 713 s, qemu-p2p 779 s, qemu-p2p-mesh 108 s, qemu-p2p-phase5 441 s, qemu-net-wire 83 s, qemu-lot67-web 279 s, qemu-lot88-desk 417 s, qemu-lot88-tv 222 s, qemu-lot87 209 s, qemu-lot86 175 s, qemu-lot85 152 s, qemu-lot84 146 s, qemu-lot83 107 s, qemu-lot82 98 s, qemu-lot81 117 s, qemu-lot80 97 s, qemu-lot79 60 s, qemu-lot78 40 s, qemu-lot77 25 s, qemu-lot76 22 s, qemu-lot75 92 s, qemu-lot74 97 s, qemu-lot73 171 s, qemu-lot72 266 s, qemu-lot71 285 s, qemu-web-layout (contract passed; 22413 s wall, the host was paused during the job; a Lot 90 run of the same job was 1343 s), qemu-production 71 s.
- qemu-gui-parity: FAILED in 139 s, panel key 'down' with no 'osui panel key sel=3 ilen=0' within 25 s (guest had sel=2 ilen=1). A flake: the same job then passed alone (review below).
- tree-clean-check: FAILED, uncommitted docs/lot91_plan.md (this file).

ASan/UBSan (TEST_CC with -fsanitize=address,undefined) on copies of the tree:
- Base 0576849: 17450 tests, 5 failed (test_gpt2_infer, test_pmm, test_vfs_service, test_webrt, test_lot87_web).
- Head e6c3e3f: 18971 tests, the same 5 failed. Nothing new (review point 11).

Run 2, after 1dff139, each job alone:
- qemu-lot91 on 1dff139 with the default clock=host: FAILED at "guest clock too late for the tz -720 step: 755 min" (guest date 12:35 UTC after a host pause; review point 12).
- qemu-gui-parity: PASS, "QEMU UX5 GUI parity panels contract passed", panel keys 136, slowest trace 0.2 s, 0 over 4 s. The 139 s failure was a flake, not a Lot 91 regression. Assertions unchanged.

Run 3, after the clock=vm change in test_qemu_lot91.py (review point 12), alone:
- qemu-lot91: PASS in 457.7 s. Reader backgrounds, @property, summer time (tz +788) and expiry before offers all reported as specified: a value from 30 June (expire 1) offered before midnight on 1 July, forgotten by the first offer list after midnight (expired 1). Guest date stayed on 10:3x UTC (clock=vm), not a host jump.

Mutations: 45 applied (44 host, 1 QEMU), 45 killed, 0 survived (/workspace/run/lot91/mutations.txt).

Final tree: tree-clean-check after this docs commit.
