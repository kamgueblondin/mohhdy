# Lot 53: row flex sizing (US-031 partial)

Base: fresh origin/main aa73933 (PR #209 merged).
Branch: cursor/lot53-flex-sizing-d639. No merge or force-push.

The next explicit layout gap is row flex-shrink and flex-basis. This is a
focused increment, not a repetition of the Lot 52 line-count target. No
code or test lines are added just to reach a size. Actual numstat counts
are reported in the PR.

One shell and one engine, userspace/webeng.c. Kernel, ABI and rights are
unchanged. US-031 stays partial, phase3_complete=false. No Chromium,
WebKit, network loading, JavaScript VM or new font engine. US-059,
US-060, US-063, US-074 and US-G19..US-G24 remain unchanged.

## Supported row behavior

- flex-basis: integer pixels (0..2000), percent (0..2000), or auto. A
  definite basis overrides width. Auto uses width, or the existing zero
  grow basis (one character in wrap mode); no intrinsic measurement.
- flex-shrink: integer 0..1000. Scale the factor by the original content
  basis, excluding borders, padding and margins.
- Once basis/shrink is explicit on a row item, unspecified shrink factors
  on that row default to 1. Rows without either property retain Lot 52
  defaults, including the equal-slot path and nonshrinking overflow.
- Freeze at min-width (including percentage), redistribute the remaining
  deficit and assign rounding pixels deterministically in source order.
  Unshrinkable overflow is diagnosed, not hidden or forced below minimum.
- Wrap partitions lines before sizing. Column basis/shrink is diagnosed
  and leaves the previous column layout intact.

Arithmetic and storage are bounded by WE_MAX_NODES and WE_DEPTH_MAX.
A fixed 64-step product/division avoids a freestanding libgcc dependency.
Weights and bases remain in depth-indexed static workspaces, without heap
allocation. No second flex engine or shell is introduced.

## Validation

New host tests cover proportional factors, zero, implicit factors, basis
precedence, percent/auto/zero bases, min/max widths, padding/margins,
minimum freezes, overflow, invalid values, high weights, nesting, wrap
and relayout. The previous 203 web engine cases are unchanged.

make qemu-web-layout retains the Lot 52 image/page checks and adds a
second local page, /web/lot53.html. Its 2000 framebuffer pixels require
50/50 shrink and 70/30 minimum freezing through the same GUI engine.

Measured on 28 October 2026 in the configured GCC multilib/NASM/QEMU
container (i386 host binaries through qemu-i386):

```text
Total Tests: 1353
Passed: 1353
Failed: 0
Skipped: 0
Execution Time: 152s
```

The web engine has 212 passing cases, including the 203 existing cases.
The same 212 cases pass ASan/UBSan. make all, make qemu-web-layout,
make qemu-gui-parity, make strict-image-check, make secrets-check,
make roadmap-check and extract_guest_commands.py --check all pass.
make tree-clean-check is run after committing the intended source files.
CI is reported separately in the PR from the actual run and job statuses.

```text
web-layout pixels image=48x32 compared=1536 wrap=2 table_rows=2
flex-sizing pixels compared=2000 shrink=50,50 frozen=70,30
QEMU web layout contract passed.
```

The new page's serial record is nodes 11, rules 6, boxes 9, items 4,
page 640x20, diag 0 and dropped 0. The original Lot 52 assertions are
retained; the harness closes its first web view before opening the next.

From git diff origin/main --numstat: engine/header and behavior tests
add 294 raw lines (275 code, 6 comment-only and 13 blank), remove 4.
No existing function is copied. The 11-line HTML fixture and one build
rule addition are counted separately; docs/roadmap are excluded.
US-031 remains partial: column basis/shrink, intrinsic sizing, full CSS,
JavaScript, network loading and font loading remain outside this lot.
