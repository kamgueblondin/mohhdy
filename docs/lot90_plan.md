# Lot 90 plan - reader backgrounds and @property, autofill expiry, PSL rules, NE2000 idle drain, shaping settings, scaled title bars, TV safe area

Base: Lot 89 head e051615 (PR #247). Branch cursor/lot90. One guest engine
and one shell path for the console and the desktop. Kernel rights and ABI
unchanged; the kernel drawing changes of rows J and K are review points.
US-031 stays partial. phase3_complete stays false. US-068/US-073, US-076,
US-080 and US-081 stay partial. US-078 and US-097 stay simule.

A row is listed only when its code, tests and docs are in the branch.
Size: about 43000 added lines, of which about 35800 are generated host
test suites (tests/unit/userspace/test_lot90_{fillgen,vars,fontpx,bgsheet,
psl,bgval,scopegen}.c, written by scripts/lot90_*gen.py from Python models
that do not share code with the C under test) and about 7200 are written
by hand (code, hand tests, QEMU test, docs).

## Scope

| Id | Feature | Before | After |
|----|---------|--------|-------|
| A | Background images in the reader sheet (US-031, Lot 89 gap 1) | the reader passed one background colour; a background with an image, a gradient or a position was dropped (an image layer and a colour gave nothing) | wf_cssbg: background-image (none, url(), linear-gradient with a side or a multiple of 90deg, radial-gradient with shape keywords, 2 to 4 stops, up to 4 layers, a gradient only in the last), background-position (one or two values, given back as "X Y"), -size (cover, contain, auto, lengths) and -repeat (one and two word forms), and the background shorthand split into them (attachment and box keywords accepted, not passed on). Values are given back in one normal form and written into the style attribute handed to the engine; a value the grammar refuses drops the declaration. Proof: test_lot90_bgval.c (860 generated cases), test_lot90_bgsheet.c (470), test_lot90_reader.c, QEMU /lot90/bg.html |
| B | List marker images (US-031, Lot 89 gap 1) | list-style-image was ignored; list-style with an image was dropped | list-style-image and list-style-position; list-style sets the three list properties (an image alone leaves the type at its initial value). Proof: test_lot90_reader.c, test_lot88_props.c (converted), QEMU /lot90/bg.html (#ls90: square, url(dot90.png), inside) |
| C | Long values and font lengths in px (US-031, Lot 89 gap 1) | a declaration value was cut at 47 bytes, so a long clamp() was dropped; calc() could not mix em with px; rem was 16 px whatever the root size | a value may hold 127 bytes (WF_SITE_VAL 128; a longer one is dropped and counted, never cut); long values (backgrounds, list images) sit in a pool of 32 strings. Font sizes that mix em and px and rem lengths when the root is not 16 px are resolved by the reader against the parent's (or the root's) font size and written in px; letter-spacing in em is resolved against the element's size. Proof: test_lot90_fontpx.c (430 generated cases), test_lot89_cssmath.c (converted), QEMU /lot90/bg.html (calc(1.5em + 2px) in a 16 px parent is 26px, 2rem with html at 20px is 40px, a 75-byte clamp() is 18.5px) |
| D | Custom properties computed where declared, @property (US-031, Lot 89 gap 2) | a property defined from another one (--a: var(--b)) was expanded where it was used; 12 names, 31-byte values; @property was not read | A custom property's var() are substituted on the element that declares it and the computed value is inherited (CSS Variables 1); sheet-wide properties are computed on the root. 24 names, 64 values of 63 bytes. wf_cssprop reads @property (syntax '*', length, percentage, length-percentage, number, integer, color; inherits; initial-value that must match and be computationally independent, else the rule is ignored and counted). A registered property that does not inherit is not inherited; a value that does not match its syntax acts as unset. `web-css` prints "css-props registered N rejected R computed C invalid I unset U initial J". Proof: test_lot90_props.c (17), test_lot90_vars.c (300 generated cases), test_lot89_cssvar.c and test_lot89_elemvar.c (converted), QEMU /lot90/props.html |
| E | Autofill dates per value, accepted fills, automatic expiry, time zone (US-031, Lot 89 gap 4) | one date per key (its last store); a value accepted from the offer list did not refresh it; nothing expired on its own; UTC only | Each value has its own date; a value typed again or accepted from the list (desktop list on Tab, Esc, submit or click; web-fill; the Down key with a single offer) becomes current and is dated today. forget-older drops values one by one. `autofill expire N|off` forgets values older than N days once per new day (wf_autofill_tick after load and before each store); `autofill tz MINUTES|utc` (-720..840) dates by the local day; `autofill status` and `autofill ages values` (never the values themselves). Session file "MOHHDY-AUTOFILL 3" with optional expire and tz on the header and a day per value line; versions 1 and 2 still load. Proof: test_lot90_fill.c (10), test_lot90_fillgen.c (250 generated cases), test_osui_gui.c test_lot90_offer_touch, test_lot89_fillage.c (converted), QEMU boot 1 |
| F | NE2000 idle drain (Lot 89 gap 6) | the Ring 3 worker read the card only during a relay op; a burst between ops could fill the 26 page ring and QEMU overwrote unread packets | net_rxq: a byte FIFO of 16 full frames. The worker drains the card into it when no relay request waits and no LLM session runs (8 frames per idle round); every reader of the card serves the FIFO first, then the card, so frame order is kept: ne2k_rx_poll asks an rx hook first (ne2k_set_rx_hook; the worker installs one that pops the FIFO, the kernel never sets one), so the P2P, LLM and DHCP clients compiled into the worker see the drained frames too, and the Ring 3 stack and the kernel wire pump use net_rxq_read; a full FIFO stops the drain (nothing is cut). The worker prints "net-driver nic rxq held H peak P in I out O full F oversize S drains D idle N end" on its own line (the counters line keeps its shape). Proof: test_lot90_rxq.c (7), qemu-p2p, qemu-p2p-mesh and qemu-p2p-phase5 (they failed before the hook, see review point 14), QEMU boot 1 (line consistent after pages: held = in - out, held <= peak, nothing oversize) |
| G | Public suffix rules with wildcards and exceptions (US-031, Lot 88 gap) | 126 hand-written suffixes, exact matches only | 360 hand-written entries and the PSL rule forms: "*.ck" (every label under ck is a suffix) and "!www.ck" (registrable although the wildcard covers it), applied as the PSL algorithm says (an exception wins, else the matching rule with most labels, else the TLD). Not the full list, no data generated from outside. Proof: test_lot90_psl.c (712 generated hosts), test_arm_portable (regdom PSL cases), QEMU boot 1 (x.city.kawasaki.jp shares a site with y.city.kawasaki.jp; a.b.ck does not with c.b.ck) |
| H | @scope lists of six, proximity to 14 generations (US-031, Lot 89 gap 3) | start and end lists of three selectors; proximity capped at 6 generations (two roots further up tied and the later rule won) | 6 selectors per list (a longer list keeps six and counts an error), proximity counted up to 14 generations; the cascade rank is proximity 0..15 x 128 + order, times WF_SITE_RANK_LOW 2048 (largest rank exactly 2^31 - 1). Proof: test_lot90_scopegen.c (300 generated cases), test_lot89_scope2.c (converted), QEMU /lot90/scope.html (sixth of seven selectors applies, the seventh does not; the root nine generations up beats the one eleven up) |
| I | Shaping queue settings (US-068/US-073, Lot 88 gap) | one eight-entry queue per node with fixed class weights | 16 slots; class weights set at run time (1..16, defaults 3 messages / 1 puts); optional age limit (an entry older than it is dropped at the next drain, counted expired); optional per-peer cap (a push beyond it is refused, counted capped). `p2p-shape [status] | weight msg|bulk N | age MS|off | cap N|off`; the status line gains "weight WM/WB age_ms A cap C expired E capped P". Single node, no WAN. Proof: test_lot90_shape.c (16), test_lot88_shape.c (converted), test_arm_portable, QEMU boot 1 |
| J | Title bars follow the display scale (US-080, Lot 88 gap 7) | title bars kept their 1x height (30, 34 for the chat) at every chrome scale | gfx_wm_title_h(base, q): base at q <= 4, else (base x q + 2) / 4 capped at 2 x base. Every body below a title bar (pane, shell, chat, IDE, app toolbar and grid, address bar, web origin, image rect, full-window title and favicon) moves down by the growth; nothing moves at 1x. Proof: test_lot90_title.c (formula, 1x unchanged, shifts, painted band rows at 1.5x and 2x), QEMU boot 2 (the guest hit test reports 45 px title bars, 51 for the chat, at q = 6) |
| K | TV overscan safe area (US-080, Lot 88 gap 7) | nothing kept the edges of a TV clear | edid_overscan_margin: a CEA TV sink without the EDID underscan bit gets 2.5% per side ((w x 25 + 500) / 1000). The desktop clips the window areas and the full-window geometry to the safe rect, raises the dock above the margin, and dims the band (a margin of a quarter or more of a side is ignored). The kernel logs "[GFX] tv safe area margin X Y (no underscan bit)" once per size; display-info says the desktop keeps it clear. No DDC, mode set unchanged. Proof: test_lot90_title.c (overscan rule, windows inside the safe rect including small screens, dock raised, band painted, bounds), test_arm_portable (arm_port_cea), QEMU boot 2 (26 x 19 px at 1024x768) |

## Review points

1. Kernel drawing changes (rows J and K): kernel/gfx_wm.c (gfx_wm_title_h), kernel/gfx_desktop.c (title heights, body shifts, safe area, dimmed band), kernel/gfx_fb.c (set_safe each frame, one log line per size), kernel/edid.c (edid_overscan_margin); kernel/ne2k_hw.c gains the rx hook of point 14. No syscall, right or ABI change. At the default auto scale of v8 scenes (1.5x at 1024x768) every title bar is taller than in Lot 89, so every body sits lower; the scaled QEMU jobs were run again (see Gates).
2. wf_site_t (one static in wf_html.c) grows from 331272 to 412864 bytes (127-byte values, a pool of 32 long values, 24 custom names and 64 values, 6-selector scope lists, @property registrations). we_doc_t is unchanged.
3. Custom properties are computed where they are declared; sheet-wide ones are computed on the root. A cycle is guaranteed-invalid on the root, so var(--a, 1px) now takes its fallback (test_lot89_cssvar.c converted: was dropped). `revert` on a custom property acts as unset. A registered property referenced from another custom property is read through its substituted text.
4. Autofill session file moves to version 3 (header settings, a day per value line). A Lot 89 build still reads a version 3 file (its loader checks only the first letter of the header): it ignores the expire and tz settings and dates each key by the day on its last value line. *.sch.uk and similar wildcard suffixes follow the PSL rule, so two schools under sch.uk no longer share a site. The automatic expiry runs after the file is loaded (fill_clock after load, so the saved time zone dates the first tick) and before each store.
5. The worker's new rxq line; the drain pauses while an LLM session is in progress (phase bits 8..15 of its status word) so a model download keeps the card to itself (the LLM client sees drained frames through the rx hook anyway, point 14). osui_web_page_ops_t gains a page_offer_touch member.
6. @scope: ALTS 3 -> 6, HOPS 6 -> 14, rank low 1024 -> 2048; the rank layout is in the header comment.
7. Shaping: P2P_SHAPE_Q 8 -> 16, the status line gains five fields at its end (tools parsing the old line by prefix keep working).
8. userspace/plat_form.c: the overscan note now reads "display tv-hint overscan likely (no underscan bit): the desktop keeps a 2.5% margin per side clear" (was a hint that nothing handled it).
9. CI: the ARM toolchain install step goes from 11 to 19 minutes and the arm-port job from 15 to 25 (run 42763680795 on e051615 hit the 11 minute step limit while apt stalled on the Azure mirror; two bounded attempts need up to 18 minutes). Two new jobs, lot90-reader and lot90-tv.
10. Converted tests (assertions changed, none weakened; each now checks the new behaviour):
    - test_lot88_props.c: background with url() and a colour (was: nothing passed on; now both longhands); list-style with an image only (was: dropped; now image and position, type initial).
    - test_lot89_cssmath.c #c: letter-spacing 0.1em against px (was: dropped; now 1px).
    - test_lot89_cssvar.c cycle with fallback (was: dropped; now the fallback, see point 3).
    - test_lot89_elemvar.c bounds: 12 -> 24 names, 31 -> 63 bytes.
    - test_lot89_fillage.c: header "MOHHDY-AUTOFILL 2" -> "MOHHDY-AUTOFILL 3".
    - test_lot89_scope2.c test_long_list_keeps_three: a seven-selector list (six kept, seventh dropped) instead of four.
    - test_lot88_shape.c: three tests count relative to P2P_SHAPE_Q; the status string gains the five new fields.
    - test_p2p.c test_rate_limit_and_stats (queue-full part) and test_lot88_shape_fair_and_ordered: counts follow P2P_SHAPE_Q instead of the literals 8, 9, 10 and 11; message labels have two digits.
    - tests/scripts/test_qemu_lot89.py step_lengths: #cyc89 is 30px (the fallback of var() on a cycle, was 20px kept) and #mix89 indent 18px (calc(1em + 2px) resolved, was 1em kept), the QEMU side of the cssvar and cssmath conversions.
    - arch/arm/arm_portable.c arm_port_shape: P2P_SHAPE_Q - 1 instead of 7, plus Lot 90 checks.
11. The QEMU step for the shaping settings types its keys in SYS_GETC mode: while the mesh is up the shell reads keys without blocking (shell.c), so the echo is "caractere retourne", not "caractere ajoute".
12. ASan (gcc -fsanitize=address,undefined) on base e051615 fails 5 suites, not the 4 usually listed: test_gpt2_infer, test_pmm, test_vfs_service, test_webrt and test_lot87_web (a stack underflow in wf_compat_advice, untouched by this lot). The head run found one more, test_lot90_rxq: the test pushed a 1536-byte frame from a 1514-byte buffer (a test bug, not net_rxq.c); the buffer is now NET_RXQ_FRAME_MAX and the rxq mutations are still killed.
13. The first full test-all after chunk 10 failed test_p2p (2 tests asserting the Lot 88 queue of 8); 3e185c5 converts them (point 10).
14. P2P regression found by the gates and fixed in be22267: with the idle drain, qemu-p2p (devsync reply lost), qemu-p2p-mesh (STUN answer lost) and qemu-p2p-phase5 (peers flapping) failed on 3e185c5, alone too, while qemu-p2p passed on base e051615 (583 s). Cause: kernel/net_llm_client.c (kernel_p2p_op, the LLM and DHCP paths), compiled into the worker, reads the card with ne2k_rx_poll directly, so frames the drain had moved into the FIFO never reached it. A run with the drain disabled passed (588 s), which isolated it. Fix: kernel/ne2k_hw.c ne2k_rx_poll asks an rx hook first (ne2k_set_rx_hook, the raw read is ne2k_rx_poll_card); the worker installs a FIFO hook; in the kernel no hook is set, so its path is unchanged. Kernel source change, no ABI or rights change.
15. CI trigger: removed the branches filter from pull_request so stacked PRs get CI (PR #248 base is lot89-css-autofill-energy, not main). The push trigger stays master/main only.

## Not done (first gaps for lot 91)

1. Reader backgrounds: angles that are not a multiple of 90deg, explicit radial radii, conic gradients and background-clip/origin are refused or dropped; the engine draws what the style attribute carries, it does not fetch background images.
2. @property: no '+' or '#' lists, no '|' alternatives, no custom idents; calc() is taken as a match without type checking inside it; no transitions on registered properties.
3. Custom properties: 24 names, 64 values of 63 bytes; over that they are counted and left out.
4. Autofill: the expiry runs once per new day (a session that never sees a new day does not expire); the time zone is a fixed offset (no daylight saving rules).
5. PSL: 360 hand-written entries, not the full list.
6. NE2000: the FIFO holds 16 frames and the drain is skipped during an LLM session; a longer burst still waits on the 26 page card ring.
7. Shaping: single node settings, no WAN, no per-class age limits.
8. US-080: no DDC, the mode set is unchanged, the safe area is drawn by the desktop only (a full-screen app may still draw into the margin); the margin is fixed at 2.5%.
9. US-081 unchanged: no backlight, no watts, battery simulated.

## Tests

Host tests: tests/unit/userspace/test_lot90_reader.c (16), test_lot90_bgval.c (144 tests, 860 cases), test_lot90_bgsheet.c (470), test_lot90_fontpx.c (430), test_lot90_props.c (17), test_lot90_vars.c (300), test_lot90_fill.c (10), test_lot90_fillgen.c (250), test_lot90_psl.c (37 tests, 712 hosts), test_lot90_scopegen.c (30 tests, 300 cases), test_lot90_shape.c (16); tests/unit/kernel/test_lot90_rxq.c (7), test_lot90_title.c (14); test_osui_gui.c test_lot90_offer_touch; test_arm_portable (PSL, shaping, CEA overscan). Generators: scripts/lot90_gen.py, lot90_vargen.py, lot90_fillgen.py, lot90_pslgen.py, lot90_scopegen.py (fixed seeds).
QEMU: tests/scripts/test_qemu_lot90.py, `make qemu-lot90` (boot 1 at 1x: /lot90/bg.html, /lot90/props.html, /lot90/scope.html, rxq line, autofill status/tz/expiry/ages and PSL rules, p2p-shape settings; boot 2 with a TV EDID: safe area log, display-info note, chrome of every window type at q = 6 with scaled title bars). LOT90_ONLY=reader|tv|shape runs one boot. CI jobs lot90-reader and lot90-tv.

## Gates on the box

All on the box, 12 November 2026 (times Indian/Mauritius). There is no arm-none-eabi-gcc on the box, so the ARM image is built in CI only; test_arm_portable runs the new self-test cases on the host.

Run 1, on 3e185c5 (two scripts in parallel, at most two guests at a time):
- make all; make test-all 17533 tests, 0 failed, lot67-web-host 64/64 (244 s). An earlier test-all on 387d41e had failed test_p2p, 2 tests (review point 13).
- roadmap-check, font-check, secrets-check, strict-image-check: all pass.
- qemu-lot90: 861 s, both boots.
- qemu-lot89: FAILED in 122 s, on the Lot 89 expectations for #cyc89 and #mix89 (converted, review point 10).
- Passed: qemu-lot88-desk 831 s, qemu-lot88-tv 676 s, qemu-lot87 209 s, qemu-lot86 552 s, qemu-lot85 152 s, qemu-lot84 146 s, qemu-lot83 535 s, qemu-lot82 98 s, qemu-lot81 117 s, qemu-lot80 97 s, qemu-lot79 59 s, qemu-lot78 66 s, qemu-lot77 25 s, qemu-lot76 21 s, qemu-lot75 433 s, qemu-lot74 97 s, qemu-lot73 170 s, qemu-lot72 681 s, qemu-lot71 740 s, qemu-lot67-web 277 s, qemu-web-layout 1343 s, qemu-gui-parity 837 s, qemu-production 72 s, qemu-net-wire 83 s.
- qemu-p2p-mesh, qemu-p2p-phase5 and qemu-p2p: FAILED. qemu-p2p failed again when run alone; it passed on base e051615 (583 s). See review point 14.

Run 2, after the fix be22267 (each job alone):
- qemu-p2p: 583 s, PASS.
- qemu-p2p-mesh: 107 s, PASS.
- qemu-p2p-phase5: 203 s, PASS.
- qemu-lot89 with the conversion: 276 s.
- qemu-net-wire 84 s, qemu-lot67-web 277 s.
- qemu-lot90: 427 s, both boots.
- make test-all: 17535 tests, 0 failed, lot67-web-host 64/64 (232 s).
- The GUI-only jobs of run 1 were not repeated: be22267 touches only the net worker's receive path and ne2k_rx_poll, which the kernel uses without a hook.

ASan/UBSan (TEST_CC with -fsanitize=address,undefined) on copies of the tree:
- Base e051615: 15708 tests, 5 failed (test_gpt2_infer, test_pmm, test_vfs_service, test_webrt, test_lot87_web).
- Head, after the test_lot90_rxq buffer fix: 17448 tests, the same 5 failed. Nothing new (review point 12).

Mutations: 33 applied, 33 killed (/workspace/run/lot90/mutations.txt).

Final tree: tree-clean-check after the docs commit.
