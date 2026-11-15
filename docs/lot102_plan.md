# Lot 102 plan - reader none components, scroll() and view(), animation-range, timeline-scope

Base: 440a644 (origin/main). PR 259 (the Lot 101 branch) is merged into
origin/main, so the branch starts from origin/main. Branch cursor/lot102.
One guest engine (userspace/webeng.c, unchanged here) and one shell path.
Kernel, rights and ABI unchanged. US-031 stays partial. phase3_complete
stays false. US-068/US-073, US-076, US-080 and US-081 stay partial.
US-078 and US-097 stay simule.

Scope taken from the Lot 101 gaps (gap 1, colours: `none` components;
gap 2, timelines: scroll() and view(), timeline-scope, animation-range):
the none keyword in every colour function with missing components
carried in color-mix() and colour interpolation, the anonymous scroll()
and view() timelines, the animation-range longhands and shorthand, and
timeline-scope. Gamut mapping is not in this lot (see Not done).

A row is listed only when its code, tests and docs are in the branch.
Size: 10712 lines added and 744 removed outside docs (code 414 / 62 in wf_cssprop.c/.h and wf_sitecss.c/.h; model scripts/lot102_model.py 598 and generator scripts/lot102_gen.py 740, new; older models and generators 56 / 13 converted; hand suite 610; generated suites 7412; QEMU test, server page, Makefile and CI 195; converted tests 686 / 668, mostly the coordination arrays widened from 10 to 12 entries; run_all_tests.sh 1 / 1).

## Scope

| Id | Feature | Before | After |
|----|---------|--------|-------|
| A | none components (CSS Color 4 section 4.4) | a colour with none was refused | none is taken in rgb()/rgba(), hsl()/hsla(), hwb(), lab(), lch(), oklab(), oklch() and color() in the space syntax only (the comma forms of rgb() and hsl() still refuse it), for any component and the alpha; it converts as 0. The channels of rgb() and of color() (every space: rgb and xyz components are analogous to red, green, blue) and the alpha of every function are kept as missing. A registered <color> takes these colours too. Proof: test_lot102_none.c (1000 colours, mixes and interpolations), test_lot102_reader.c (none_forms, none_carry, none_reader), `make qemu-lot102` (#n102 rgb(255 none 0) #ff0000) |
| B | missing components when mixing and interpolating | - | color-mix(in srgb, ...) and the colour interpolation of keyframes and transitions take, for a component missing in one colour, the other colour's value; missing in both, 0 in both and still missing in the color-mix() result (so a nested color-mix() carries it again). composition add takes the converted values (no carrying). Proof: test_lot102_none.c, test_lot102_reader.c (none_carry), `make qemu-lot102` (#m102 color-mix(in srgb, rgb(none 0 0), rgb(200 0 0)) bg #c80000) |
| C | scroll() and view() in animation-timeline (CSS Scroll-driven Animations 1) | refused (the declaration was dropped) | scroll([<scroller> \|\| <axis>]?) with nearest, root or self (nearest by default) and block, inline, x or y (block by default); view([<axis> \|\| [auto \| <length-percentage>]{1,2}]?), insets auto, a length, a percentage or 0. scroll() runs at the reader scroll position like a named root timeline (Lot 101) when its axis is block or y and its scroller is the viewport: root always, nearest for any element but the root html (the reader has no other scroll container), self only on the root html. view() is stored and never active (no layout), as view timelines. Proof: test_lot102_range.c, test_lot102_sheet.c (400 sheets), test_lot102_reader.c (anon_api, anon_reader), test_lot99_timeline.c and test_lot99_sheet.c (converted), `make qemu-lot102` (#s102 at 25%, 50%, 75%; #v102 idle) |
| D | animation-range-start, animation-range-end, animation-range | not taken | the longhands ([normal \| <percentage> \| <range-name> <percentage>?]#, a unitless 0 too) and the shorthand (start then end; without an end, the start's range name at 100%, else normal), kept as written per element and cascaded with the other animation longhands (two new longhands, WF_CSSANIM_LONGS 10 to 12). On a scroll timeline the range [S, E] of the scroll position gives the local time floor(T (100 pm - S) / (10000 (E - S))) (T = delay + duration x count, S and E in 1/10000 %, normal and a range name alone are 0% and 100%), clamped to +-2 x 10^8 ms; E <= S gives no effect. Named ranges (cover, contain, entry, exit, entry-crossing, exit-crossing, scroll) are view-timeline ranges: on a scroll timeline they are taken as normal (their percentages ignored). Proof: test_lot102_range.c (800 values, texts and times), test_lot102_sheet.c, test_lot102_reader.c (range_api, range_ms, range_reader), `make qemu-lot102` (#r102 animation-range: 20% 60%) |
| E | timeline-scope | not taken | none \| all \| <dashed-ident># (at most 4, none and all alone), cascaded as a fifth timeline longhand. The lookup of a named timeline walks up from the animated element: an element defining the name decides as in Lot 101; else an element whose timeline-scope lists the name, or is all with a descendant defining the name and no element in between scoping it, stops the search (the timeline is then a descendant's, never the root's: no effect). Proof: test_lot102_scope.c (320 trees), test_lot102_reader.c (scope_api, scope_reader), `make qemu-lot102` (#q102 idle under body { timeline-scope: --page102 }) |

## Review points

1. Structure sizes: we_doc_t keeps 870824 bytes (< 1 MiB, webeng.c untouched). wf_site_t grows from 505348 to 508420 bytes (two more animation longhand slots and one more timeline slot per element). wf_cssanim_t gains tlsrc, tlaxis, rs, re (bytes) and rsp, rep (ints).
2. Array contract change: WF_CSSANIM_LONGS goes from 10 to 12 (WF_CSSANIM_L_RANGE_START = 10, WF_CSSANIM_L_RANGE_END = 11), so wf_cssanim_coord now reads 12 value and flag entries; WF_SITE_ALONGS 15 to 17, WF_SITE_K_TL0 44 to 46, WF_SITE_TLS 4 to 5. Callers in the tree are converted (generators and tests); this is an internal userspace header, no kernel ABI.
3. New API: wf_cssanim_range, wf_cssanim_range_text, wf_cssanim_range_ms, wf_csstl_scoped; wf_csstl_parse takes WF_CSSTL_SCOPE (3), which used to be refused; cp_rgba_t carries a missing mask (internal).
4. Choices and approximations: nearest is the reader viewport for any non-root element; view() and view timelines stay inactive; named ranges are normal on a scroll timeline; lengths in animation-range are refused except a unitless 0 (no layout to resolve them); add composition does not carry missing components; hsl(), hwb(), lab(), lch(), oklab() and oklch() components converted from none are not kept as missing channels (only their alpha), since they are not sRGB channels (CSS Color 4 analogous components across spaces are not modelled); gamut mapping is not done (colours outside sRGB are still clipped per channel).
5. Simplifications from the mutation runs: the j == q test in the timeline-scope all lookup was removed (from j == q the walk never reaches q), and the text checks of the animation-range shorthand became (void) calls (each text is at most 118 bytes, under WF_SITE_LONG = 128).
6. Converted tests (assertions changed, none weakened; each now checks the new behaviour):
    - test_lot101_color.c and test_lot101_mix.c (regenerated by scripts/lot101_colorgen.py): 8 colours with none that were refused now give their colour (lab(none 0 0) #000000 ...), 4 registered <color> matches with none now match.
    - test_lot101_paint.c (regenerated): 3 styles whose none colour was dropped now paint.
    - test_lot101_reader.c: lab(none 0 0) gives #000000 255; wf_csstl_parse(3, "--a") is now the scope list (accepted), the refused index moves to 4.
    - test_lot100_anim.c and test_lot99_sheet.c (regenerated): sheets with animation-timeline: scroll() used to drop it and run on the reader clock; they now run on the scroll position (0 in these suites).
    - test_lot99_timeline.c (regenerated): scroll() and view() items now parse (kinds 3 and 4) instead of refusing the list; test_lot99_reader.c: the same by hand (scroll() and view(block) accepted; the reader runs scroll() at the position).
    - test_lot98_longs.c (regenerated) and test_lot98_reader.c: the coordination arrays have 12 entries (point 2); the last longhand index is now animation-range-end.
    - Models: lot97 interpolation, lot98 stack, lot99 timeline and idle, lot100 colour, lot101 clock take the Lot 102 functions from scripts/lot102_model.py; lot98 and lot99 generators write 12-entry arrays.
7. CI: one new job, lot102-reader-ranges (make qemu-lot102, timeout 15 minutes).
8. Mutations: host 199 mutants on wf_cssprop.c, wf_sitecss.c and the two headers, one checked with ASan (the list cap of wf_cssanim_range); the reader and engine suites of run_all_tests.sh plus test_webeng (90 suites, the Lot 102 suites first, baseline 0 failures): 197 killed, 2 survived, both guards shown unreachable and kept: the closing parenthesis test of scroll()/view() (the tokenizer never gives an unbalanced function token, and a token ending in ")x" is caught by the stray parenthesis check) and the empty-token test of a range edge (every caller has a token left). QEMU: 3 mutants on a git archive copy of 728bb23 (the carry removed in color-mix(), timeline-scope ignored, ranges ignored on scroll()), all killed by `make qemu-lot102`. Earlier runs left 13 then 3 survivors; each got a witness in test_lot102_reader.c, its code was simplified (point 5), or it was shown equivalent (a none token is always followed by ')', so the end bound of the conversion scan cannot matter).

## Not done (first gaps for lot 103)

1. Colours: gamut mapping (CSS Color 4 OKLCh chroma reduction; it would convert many lot101 suites); color-mix() in other colour spaces and with hue interpolation; missing components of hsl/hwb/lab/lch kept per space; relative colour syntax (from); system colours.
2. Timelines: view timelines and view() activity, named ranges on view timelines, length insets and range offsets, view-timeline-inset, scroll containers other than the viewport (all need layout); case-sensitive names; a running scroll position from real scrolling.
3. Animations: a running clock and repaint, other reader properties than color and background-color, transition events.
4. Fonts, gradients, @property, autofill, PSL, NE2000, shaping, US-080 and US-081: unchanged (see docs/lot99_plan.md).

## Tests

Host tests (generated by scripts/lot102_gen.py, seeds 10201..10204, from the model scripts/lot102_model.py with the Lot 97 to 101 models; the models share no code with the C): test_lot102_none.c (1000 colours, mixes and interpolations with none), test_lot102_range.c (800 animation-range values, texts and scroll times), test_lot102_scope.c (320 timeline-scope lists and trees), test_lot102_sheet.c (400 sheets with scroll(), view(), ranges and scopes). Hand suite: test_lot102_reader.c (10 tests).
QEMU: tests/scripts/test_qemu_lot102.py, `make qemu-lot102` (one boot at 1x): /lot102/range.html from the Lot 67 server (web-css scroll at 25%, 50% and 75% with the reader clock at 500 ms). CI job lot102-reader-ranges. `make qemu-lot101` is run again unchanged.

## Gates on the box

Code commit 728bb23 (the docs commit changes docs only), 15 November 2026, UTC+4:

- make all: pass; a fresh git archive make all: pass (29 s).
- make test-all: pass, 23836 tests, 0 failed (376 s).
- make qemu-lot102: pass (101 s); make qemu-lot101: pass (107 s).
- roadmap-check, secrets-check, font-check, strict-image-check: pass.
- ASan/UBSan test-all: base 440a644 23463 tests 5 failed, head 728bb23 23751 tests 5 failed (the same known five: test_gpt2_infer, test_pmm, test_vfs_service, test_lot87_web, test_webrt).
- Mutations: host 199 applied / 197 killed / 2 survived (the two unreachable guards above); QEMU 3 applied / 3 killed.
- Other QEMU gates: left to GitHub CI.
