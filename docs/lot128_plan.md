# Lot 128: fractional flex-grow, flex-shrink and flex factors

Base: origin/main 7355829 (PR 286, Lot 127). One engine: userspace/webeng.c.
Rights and ABI unchanged; fs/initrd.c INITRD_FILE_MAX 152 -> 160 (the
initrd file table, review point). userspace/webeng.h gains two uint8_t
fields in we_node_t (grow_frac, shrink_frac, the hundredths of the
factors), placed in existing padding: we_node_t (4692 bytes) and
we_doc_t (875332) are unchanged, under the 1 MiB bound.
US-031 partial, phase3_complete=false.

## Scope and choice

The "Not done" lists of Lots 124 to 127 carry the same flex / grid gaps:
the overflow default of grid items, distributed fallbacks, column
baselines in wrap-reverse, fractional flex factors, flex inherit / unset
/ revert, the aspect-ratio transferred minimum. Probed on 7355829 (640 x
480, 8 px glyphs, 10 px lines):

1. Fractional flex factors were refused: flex-grow: 0.5 and 1.5 on two
   width: 0 items in a 100 px row gave 0 and 0 with "flex-grow refused";
   flex: 0.5 and flex: 1.5 gave 0 and 0 with "flex refused" (the whole
   shorthand dropped, so the basis 0 stayed 0); flex-shrink: 0.5 and 1.5
   on two 80 px items in 100 px gave 50 and 50 (factor 1) with "flex-shrink
   refused". Fractional factors are common in real style sheets
   (flex: 0.5, flex: 1.5 1 0, flex-shrink: 0.5), so these items rendered
   empty or wrongly sized.
2. Distributed fallbacks (space-between with one item, space-around in
   overflow) were checked: they already fall back as the specification
   says for the cases the engine supports.
3. The other gaps (grid overflow default, wrap-reverse column
   baselines, inherit / unset / revert on flex, the transferred minimum)
   change rarely seen rendering.

Item 1 renders wrong today on ordinary pages and stays inside the flex
algorithm of Lots 121 to 127; it is the lot.

## Parsing (CSS Flexbox 7.3: a non-negative <number>)

flex_factor(value) accepts digits, digits.digits and .digits; the whole
part is 0..1000 (as before), the fraction is kept to two decimals
(hundredths, further digits cut, 0.999 is 0.99). Refused: "1.", ".",
signs, exponents, a second dot, a unit, a whole part above 1000. A math
function (calc(1 / 2)) still gives an integer, rounded half up (Lot 118),
and clears the hundredths, like an integer value does.

- flex-grow / flex-shrink: flex_grow / flex_shrink keep the integer part
  (so every existing reader and assertion of these fields is unchanged),
  grow_frac / shrink_frac the hundredths; a refused value keeps both.
- flex: the two factors take fractions (flex: 0.5 is 0.5 1 0%,
  flex: 1.5 0.25, flex: 2.75 3.5 10px); none, initial and auto set
  integer factors and clear the hundredths; a refused shorthand keeps
  every earlier value.

## Layout (CSS Flexbox 9.7)

grow_factor(node) is flex_grow * 100 + grow_frac (0 when unset),
shrink_factor(node) flex_shrink * 100 + shrink_frac (100 when unset).

- grow_line: the weights are the factors in hundredths; a share is
  flex_share(free, factor, sum) (exact 64-bit product, no 64-bit
  division). Integer factors give exactly the base shares (both scaled by
  100).
- Factors summing below 1 (step 4b): the unfrozen growers get the
  initial free space times their sum in all, less what they already took
  in an earlier pass (an item clamped by max-width freezes; the others
  keep their own fraction of the initial free space, not of what is
  left). The rest of the line stays free. The pixel left by rounding goes
  left to right as before, never past that cap.
- shrink_line: the scaled shrink factor is basis * factor in hundredths
  (at most 16000 * 100099, inside 32 bits per item); the sum is a 64-bit
  value and flex_share takes a 64-bit denominator (the grid caller is
  unchanged). Factors summing below 1 shrink the line by that fraction
  of the initial overflow, less what they already gave; the rest
  overflows as intended, without "flex minimum prevents fit". A minimum
  that blocks the shrink inside that fraction still reports it.
- Shrink shares follow basis * factor while the fraction uses the factors
  only, so after other items freeze the remaining ones may already have
  given more than their fraction: they restart from their flex base
  sizes and give that fraction of the initial overflow (step 4 restart;
  200 px at 0.1 next to a 20 px item at 0.8 frozen at 19 px in 100 px:
  188, not 140).
- An item with a 0 flex basis and a non-zero shrink factor never violates
  its minimum and stays unfrozen in the specification: its factor counts
  in the sum (0 and 0.5 next to 100 px at 0.5 in 50 px: 50, not 75); held
  by a min-width it is frozen from the start.
- Checked against a direct implementation of 9.7 on random single-line
  rows (1 to 5 items, factors 0 to 3 with hundredths, bases 0 to the
  width): with integer factors the outputs are identical to the base
  engine; without min / max, 1 line in 4000 differs by more than one
  pixel per item (the Lot 121 water filling freezes a violating item one
  pass later, after which the cap applies).

## Review points

- fs/initrd.c (kernel, outside webeng.c): INITRD_FILE_MAX 152 -> 160.
  lot128.html is the 145th initrd entry and the capacity check of
  make test-all keeps 8 spare (as Lot 120 did, 144 -> 152). Static
  tables only (files_array, initrd_page_entries); no syscall, right or
  ABI change.
- webeng.h (outside webeng.c): two uint8_t fields in we_node_t, in
  existing padding (sizes unchanged). Not an ABI: the structures are only
  used by userspace modules built from the same sources.
- Converted tests (no assertion weakened):
  - webeng_lot52.inc test_l52_flex_fractional_grow_refused: the item
    now says flex-grow: 0.5; flex-grow: 1. ; 0.5 is accepted (flex_grow 0,
    grow_frac 50, was -1), "1." is refused and keeps it, so the
    "flex-grow refused" assertion and the 320 px width stay.
  - webeng_lot53.inc test_l53_invalid_values_do_not_replace_prior_declarations:
    flex-shrink: 0.5 became flex-shrink: -0.5 (still refused), the
    asserted value 2 is unchanged.
  - test_lot121_engine.c test_l121_flex_refused: the refused shorthand
    "1.5" became "1." (still refused, same five assertions).
- Rendering change: items with fractional factors, and lines whose
  factors sum below 1. None of the 50 existing pages changes (host
  pipeline, base vs this lot).
- Files outside webeng.c: webeng.h, fs/initrd.c, the three converted tests,
  tests/unit/userspace/test_lot128_engine.c (new),
  tests/scripts/run_all_tests.sh, Makefile (qemu-lot128), ci.yml (job
  lot128-engine-fractional-flex), initrd_content/web/lot128.html,
  tests/scripts/test_qemu_lot128.py, docs.

## Tests

- Host: tests/unit/userspace/test_lot128_engine.c: parsing (longhands
  with one, two and three decimals, leading dot, 1000.99, 0.0, an integer
  or a math function after a fraction, unset), refused forms (twelve,
  each keeping the earlier value and hundredths), the shorthand (one and
  two fractional factors, with a basis, auto, none / initial / auto
  clearing, refused shorthands), growing (weighted, rounding, sum 1,
  sums below 1 with and without a basis, max-width freezing below and
  above 1, an 8 px line with rounding, a column), shrinking (weighted,
  below 1 without diagnostic, a zero factor, under a pixel, a frozen
  minimum, a blocking minimum with diagnostic, integer factors, a weight
  sum above 32 bits).
- QEMU: initrd_content/web/lot128.html, tests/scripts/test_qemu_lot128.py
  (make qemu-lot128, CI job lot128-engine-fractional-flex), with NEVER
  values that the base engine gives.

## Not done

- Fractions beyond hundredths are cut; a math function in flex-grow /
  flex-shrink still gives an integer (calc(1 / 2) is 1).
- The incremental passes of grow_line / shrink_line are kept: a line
  whose factors sum to 1 or more keeps the Lot 121 water filling; an item
  that reaches its minimum or maximum exactly by rounding counts as
  frozen, and violating items freeze one pass at a time, so a cap below 1
  can apply a pass later than the specification's restart (rare, see
  above).
- Left from Lot 127: vertical writing modes for left / right and self-*;
  legacy keywords; the overflow default of grid items stays at the
  start; column baselines in wrap-reverse; flex inherit / unset / revert;
  the aspect-ratio transferred minimum; osui_runtime web-theme on the
  US-041 resolver; calc-size, anchor, typed attr(), margin collapsing.
