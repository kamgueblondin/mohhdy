# Lot 84 plan - definite-first masonry, row height, border percentage math

Base: main a4b05b9 (lot 83, PR #241). Single engine userspace/webeng.c.
Kernel rights unchanged. US-031 stays partial. phase3_complete stays false.
US-068/US-073 stay partial. US-076 stays partial. US-078 and US-097 stay simule.

## Scope (the lot 83 first gap, then the two red QEMU jobs)

| Id | Feature | Before | After |
|----|---------|--------|-------|
| F1 | masonry-auto-flow: definite-first | keyword accepted, placement stayed in document order | two passes: items with an explicit grid-axis start (column start for masonry rows, row start for masonry columns) are placed first, then the others. pack and next both apply. The next cursor continues across the passes. ordered clears the flag |
| F2 | Masonry-rows content height with grid-auto-flow: column | column flow added one column per item, so the packed height collapsed to the tallest item | columns stay the declared tracks. The used height is the tallest packed column, including row-gap. height:min-content and height:max-content select that packed height at the matching intrinsic inline size |
| F3 | Percentage math in border widths | border widths were px only, capped at 8 on a side | longhands, the border shorthand and border-width accept min()/max()/clamp()/calc() and percentages. A fixed result is px (0..64, side cap 8). A percentage result is pooled (same pool as margin and padding) and resolved against the containing block width, then capped at 8. Negative px is refused and the previous width stays |
| F4 | Percentage math in absolute insets of a masonry item | math on insets already evaluated, not covered for a masonry containing block | left/right resolve against the masonry grid's padding width, top/bottom against its padding height. The absolute is not a masonry item |
| F5 | P2P bandwidth estimator, zero samples | a probe not acked within 2 s was dropped, so a pong that arrived after the peer guest was scheduled produced no delivery-rate sample. The QEMU check also stopped as soon as every probe was marked lost | the timed-out probe stays in a late table. Its ack still samples and is no longer counted as a final loss. The QEMU loop waits for 20 acks and a non-zero estimate |
| F6 | qemu-net-wire stack report | the worker printed wire_ops after relay_reply, so the test often read the log before close's report and saw wire_ops 3 | the report is printed before the client is unblocked. The assertion stays wire_ops == 4 |
| F7 | ARM self-test of the estimator | not compiled on ARM | userspace/p2p_bwe.c unchanged, late sample 128 B/s, boot line and host test |

## Review points

1. we_node_t gains masonry_definite, height_intrinsic, border_pct and border_spec[4]. The document stays under 1 MiB (asserted).
2. p2p_bwe_t gains a late table of P2P_BWE_INFLIGHT packets. A late ack does not put the bytes back into inflight. b->losses counts the timeout; the caller's probe_lost drops when the pong arrives.
3. Lot 83 definite-first next tests that name an explicit line were updated to the two-pass positions. Tests with no explicit start are unchanged.
4. The 0..8 border cap is the existing per-side paint cap, not a new limit. A percentage that resolves above 8 is 8 in layout and in paint.
5. height:min-content and height:max-content on a non-masonry box are accepted as the content height. They are not a full intrinsic block-size pass for every display type.

## Not done (first gaps for lot 85)

1. Masonry rows with auto column tracks still size those tracks from the ordinary grid placement, not from the masonry column each item actually occupies.
2. Percentage math in border widths is capped at 8 px per side. A wider painted border is not in this engine.
3. Absolute insets of a masonry item do not resolve a percentage of an indefinite grid height (the used value stays the fallback).
4. US-068/US-073 remain a laboratory estimator: no WAN, no cross traffic. US-078 and US-097 stay simule.

## Tests

Generated suites tests/unit/userspace/webeng_lot84_drow.inc, webeng_lot84_dcol.inc, webeng_lot84_border.inc, webeng_lot84_height.inc, webeng_lot84_inset.inc from scripts/lot84_gen.py (independent model), plus webeng_lot84_extra.inc.
QEMU: initrd_content/web/lot84.html, tests/scripts/test_qemu_lot84.py, `make qemu-lot84`, CI job lot84-layout.
The estimator's late ack is test_p2p_bwe and the ARM self-test. The wire-report order is the existing qemu-net-wire assertion.
