# Lot 54: column flex sizing (US-031 partial)

Base: fresh origin/main dc2f1d9, merged PR #210.
Branch: cursor/lot54-column-sizing-d639. No merge or force-push.

The next explicit gap after Lot 53 is column flex-basis/shrink. This is a
focused increment; actual useful line counts are reported in the PR.
No padding is added to reproduce the previous Lot 52 size target.

One shell, one webeng.c for console and GUI, freestanding i386 Ring 3.
Kernel, rights and ABI unchanged. US-031 remains partial and
phase3_complete=false. No Chromium, WebKit, network loading, JavaScript
VM or font loading. US-059, US-060, US-063, US-074 unchanged;
US-G19..US-G24 remain partial. No roadmap story status changes.

## Behavior and limits

- Reuse the existing weighted row grow/shrink solver for both axes;
  no second solver or copy of an existing function. Track sizes replace
  the former width-only workspace name, with the same fixed capacity.
- Columns accept the existing integer px/percent/auto flex-basis and
  0..1000 shrink factors. Percent requires a definite containing height.
  Without one it emits a diagnostic and uses declared child height or 0.
- Auto bases use declared height or 0. No intrinsic content measurement;
  text may overflow a smaller assigned box. No clipping is introduced.
- min-height/max-height accept integer content-box pixels 0..2000.
  Unsupported percent, negative, fractional or oversized limits emit
  diagnostics and preserve the preceding declaration. Min beats max.
  Block boxes also apply these limits after their content is laid out.
- Box extras do not shrink or contribute to scaled weights. Minima freeze,
  maxima cap growth, and the remaining space is redistributed. Rounding
  is deterministic; unavoidable negative free space is diagnosed.
- Column wrap partitions by basis before sizing and applies grow/shrink
  per column. It retains the existing cross-axis width bound/diagnostic.
  Percent cross widths are resolved once. Wrapping needs definite height.
- Row sizing and legacy column defaults remain. Explicit column sizing or
  height limits select the new column path. The temporary assigned height
  and limits are restored after layout, including nested axes and relayout.

The workspace stays static at WE_MAX_NODES * (WE_DEPTH_MAX + 1) tracks;
node storage adds two int16_t height limits. No heap, decoder duplication,
network loader, new slash command or shell initialization change.

## Validation

Host tests assert sizes and x/y positions for weighted/zero factors,
basis precedence, minimum freezes, capped grow, gaps/box extras, wrap,
indefinite percentages, invalid limits, nested axes, relayout, rounding,
painted text positions and unsigned maximum-node weight sums.
The former column-refusal test now asserts supported geometry and style
restoration; its obsolete diagnostic assertion is replaced, not skipped.

The QEMU web contract retains all prior pixel assertions and adds
/web/lot54.html: console inspectors and 4000 exact pixels for vertical
50/50 shrink, 70/30 minimum freezing, and wrap/grow at x=14.
Measured on 28 October 2026 with the configured multilib/NASM/QEMU
container (i386 host test binaries through qemu-i386):

```text
Total Tests: 1364
Passed: 1364
Failed: 0
Skipped: 0
Execution Time: 150s
```

The 223 web engine cases also pass ASan/UBSan. Separate mutations that
bypass column sizing or painted height limits fail the corresponding
geometry tests with exit 1. The repository sources are not mutated.

```text
column-sizing inspect a box 0 0 10x50
column-sizing inspect b box 0 50 10x50
column-sizing inspect locked box 0 100 10x70
column-sizing inspect tail box 0 170 10x30
column-sizing inspect wb box 14 200 10x100
column-sizing pixels compared=4000 shrink=50,50 frozen=70,30 wrap_x=14
QEMU web layout contract passed.
```

Serial for the new page: nodes 14, rules 10, boxes 12, items 6,
page 640x300, dropped 0, diag 0. All prior web assertions still pass.

From git diff origin/main --numstat, engine/header and behavior tests
add 356 raw lines: 339 code, 3 comment-only and 14 blank. Excluding 10
pure workspace rename lines gives 329 useful code lines (99 engine and
230 tests). No existing function is copied. The 15-line HTML fixture and
test build rule are separate; documentation/roadmap do not count.
CI run/job conclusions are reported separately in the PR after reading.

make all, make qemu-web-layout, make qemu-gui-parity,
make strict-image-check, make secrets-check, make roadmap-check and
python3 scripts/extract_guest_commands.py --check pass locally.
make tree-clean-check is run after committing the intended files.
