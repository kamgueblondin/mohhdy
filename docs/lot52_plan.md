# Lot 52: bounded guest web layout (US-031, partial)

Base: fresh clone of origin/main, 303f23d (merge of PR #208).
Branch: cursor/lot52-webeng-d639. No merge and no force-push.

## Contract

One shell, one console/GUI path, one web engine: userspace/webeng.c.
Freestanding i386 Ring 3. Kernel, syscalls, ABI and rights unchanged.
US-031 remains partial. phase3_complete remains false. No Chromium,
no WebKit, no network loading, no JavaScript VM. The bitmap font is unchanged.
US-059, US-060, US-063 and US-074 are unchanged. US-G19..US-G24 stay partial.

## Implemented behavior

- Flex: legacy equal row slots and stacked columns retained; wrap on the
  sum of outer widths and gaps; integer grow 0..1000 distributes free
  space only to positive coefficients. Max-width caps redistribute unused
  shares. Auto grow bases are zero, not intrinsic measurements. Column
  grow requires a definite container height. No flex-shrink, order or
  fractional grow. Definite-height columns
  wrap to the next column; horizontal overflow is diagnosed.
- Grid: equal columns (1..8), pixel and 1fr row tracks (1..8), row-major
  placement, gaps and natural implicit rows. A definite height distributes
  free height across fraction rows; auto-height fraction rows use content.
  Fixed row tracks can have overflowing content; there is no overflow clip.
- Floats: left/right exclusion rectangles, word flow through the remaining
  interval and full-width flow after their bottom. Margins do not collapse.
  Up to 16 floats per formatting context; contexts own their floats and
  include their bottom in their height. A float after already queued words
  begins after that line. Inline wrappers share their parent's context.
- Tables: up to 8 shared columns, per-tr rows, natural or explicit row
  height, td/th, optional tbody/thead/tfoot and implied cell/row endings.
  Requested widths are bounded to the table. A nontrivial colspan or
  rowspan refuses the entire table with diagnostics; value 1 is supported.
- Images: img src reads absolute local paths through the existing shell FS
  adapter in both console and GUI. Shared imgdec handles decoding. One
  static workspace holds 64 KiB source, 64 KiB scratch, metadata and at most
  96 rectangles (compile-time workspace ceiling in webeng.h). Source and
  output dimensions are at most 64x64. Horizontal runs merge vertically;
  rectangles share the existing 96-item display list. Insufficient budget
  refuses the entire image, never a partial prefix. If a list slot remains,
  failures paint alt. Replaced images start a fresh line in the current
  inline implementation. No network resources.
- Extra holes, in the requested order: min/max-width (pixels or percent,
  minimum wins a conflict); position:relative (horizontal clamp, vertical
  clamp for a definite-height parent, otherwise vertical offset is zero;
  painted descendants move without changing normal-flow placement); disc,
  square, decimal and none list markers with independent nested counters.

These remain a bounded layout subset, not browser compatibility. Missing:
JavaScript execution, network loading, font loading/shaping, flex-shrink,
full intrinsic sizing, arbitrary grid columns, spans, margin collapse,
absolute/fixed positioning and general overflow clipping. Further work on
fonts, network or a JavaScript VM is outside this lot. No lines are added
merely to reach a target count.

## Tests and evidence

Host tests: tests/unit/userspace/webeng_lot52.inc, included in test_webeng.
They assert coordinates, outer widths/heights, row/column ordering, words
beside floats, source-order growth, bounds/refusals, repeated layout, link
hit testing after translation and every source pixel of an initrd image.
They do not search for property words in source code. ASan/UBSan supplement
the i386 runs. The existing Lot 49 assertions are unchanged.

QEMU: make qemu-web-layout boots /web/lot52.html, requires console rectangle
and text output for wrap/table/image, inspects computed boxes, opens the
same page in the GUI, and compares all 1536 image pixels in a screendump.
The existing qemu-gui-parity gate is also required.

## Measured local validation

All commands used the installed Debian development container. Host Linux
ELF i386 tests ran transparently through PRoot/qemu-i386 because the
instance kernel cannot execute them natively. The guest runs under QEMU
TCG. No assertions or test outcomes were overridden.

`make test-all`, final run, test_results_20261028_095642.log:

```text
Total Tests: 1344
Passed: 1344
Failed: 0
Skipped: 0
Execution Time: 149s
```

- test_webeng: 203 passed, 0 failed, 0 ignored (9 legacy cases retained).
- ASan/UBSan: the same 203 cases passed; no sanitizer error.
- The overlong-word/float regression first failed, then passed after
  waiting for the full text interval before allowing word overflow.
- Removal checks outside the checkout: wrap, grow, grid rows, floats,
  tables and image paint each make their corresponding test fail (one
  failed assertion in each selected case). Temporary source copies removed.
- `make all`, `make strict-image-check`, `make secrets-check`,
  `make roadmap-check`, `python3 scripts/extract_guest_commands.py --check`:
  passed. Registry: 447 names. Initrd: 98/128 entries. Roadmap: 69 entries.
- `make qemu-gui-parity`: passed on the final binary, including shared
  imgdec JPEG/PNG viewer cases and the unchanged panel/shell path.
- `make qemu-web-layout`: passed on the final binary, serial below.
- `make qemu-production`: passed in 71s; the exact complete next-id line
  is checked, not a prefix. Phase 9 stays done 18 partial 6, progress 75%.
- `make tree-clean-check` is run on the committed tree; the actual outcome
  is included in the handoff, since this guard requires a clean index.

Actual QEMU output (same page in console and the GUI framebuffer):

```text
osui web render file=/web/lot52.html nodes 30 rules 9 boxes 18 lines 8 items 60 dropped 0 page 640x154 diag 0 components 0 media_kept 0 media_dropped 0 vars 0 sanitized 0
web title Lot 52 bounded web layout
web-layout inspect a box 0 0 60x12
web-layout inspect b box 0 16 60x12
web-layout inspect cellc box 0 38 50x10
web-layout inspect picture box 0 48 48x32
web-layout inspect g3 box 0 124 48x10
web-layout pixels image=48x32 compared=1536 wrap=2 table_rows=2
QEMU web layout contract passed.
```

## Diff accounting

Source: `git diff origin/main --numstat`, base 303f23d. The following
counts exclude comment-only lines and empty lines; no copied function is
counted. The original flex/column implementation is retained as the
legacy path in the same engine, not copied into another engine.

| File | Added | Comments | Empty | Useful code/tests |
|---|---:|---:|---:|---:|

| tests/scripts/test_qemu_web_layout.py | 100 | 5 | 7 | 88 |
| tests/unit/userspace/test_webeng.c | 196 | 0 | 1 | 195 |
| tests/unit/userspace/webeng_lot52.inc | 3200 | 1 | 205 | 2994 |
| userspace/webeng.c | 785 | 15 | 27 | 743 |
| userspace/webeng.h | 36 | 10 | 2 | 24 |

Useful total: **4044** (engine/header 767, behavioral tests 3277).
Excluded within these files: 31 comment-only lines and 242 empty lines.
Copied functions: 0. Documentation, build/CI integration, FS wiring and
the HTML fixture are counted separately; none inflate the useful total.
The extra min/max widths, relative positioning and list markers are in
this total. Work stops inside the requested 4000..6000 band.

The complete PR adds 4556 and removes 105 lines.
Documentation adds 189 lines, plus 2 roadmap comment lines.

## External review and CI

PR: https://github.com/kamgueblondin/mohhdy/pull/209 (base main). No merge.
Opened through the Pull Requests REST API with injected auth; no gh or force-push.
The CI workflow runs on PRs to main/master, pushes to main/master, or a
manual dispatch; pushing this topic branch alone does not establish a CI
run. The handoff reports only API-observed run/job status, with its URL
when a run exists. No CI pass is claimed from local tests.
