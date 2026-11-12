/* wf_cssmath.h - Lot 89 (US-031): calc(), min(), max() and clamp() in the
 * length values the reader sheet keeps (font-size, text-indent,
 * letter-spacing).
 *
 * A value is a sum of two parts, CSS px and em, each in thousandths. px,
 * pt (4/3 px), pc (16 px), rem (16 px: the reader's root size; a sheet
 * that changes the root size is not followed), vw, vh, vmin and vmax (the
 * viewport the sheet was prepared for) go to the px part; em and % (of the
 * parent font size, so 1/100 em) to the em part. + and - add parts; * and
 * / take a plain number on one side; min(), max() and clamp() compare
 * values that have only one part (a px against an em value cannot be
 * compared without the parent size: refused). The result is one token
 * with at most two decimals ("24.5px", "1.25em"), or refused when it has
 * both parts. Bounded: |value| <= 10000 px, numbers <= 10000, nesting 8,
 * 32-bit arithmetic only (no 64-bit division in the guest). Lot 90: the
 * reader keeps an expression that mixes parts (or uses rem) and resolves
 * it once the parent and root font sizes are known (wf_css_math_px). Host tested
 * (tests/unit/userspace/test_lot88_cssmath.c). */
#ifndef WF_CSSMATH_H
#define WF_CSSMATH_H

#define WF_CSSMATH_DEPTH 8
#define WF_CSSMATH_MAX 10000000   /* thousandths: 10000 px or 10000 em */

/* 1 and out = the resolved token when s is calc()/min()/max()/clamp();
 * 0 when s is not one of them (the caller checks it as before); -1 when
 * it is one but cannot be resolved (bad syntax, mixed parts, bounds,
 * division by zero, out too small). vw and vh: viewport in CSS px (0:
 * 1024 x 768). */
int wf_css_math(const char *s, int vw, int vh, char *out, int cap);
/* 1 when s starts with calc( min( max( or clamp( (ASCII case-insensitive) */
int wf_css_is_math(const char *s);
/* Lot 90: s (a length such as "1.5rem", or calc()/min()/max()/clamp()
 * mixing em, %, rem and px) in px thousandths, with em and % taken
 * against em_px and rem against rem_px (thousandths of a px, 1..10^7).
 * 1 and *px set; 0 for an empty string; -1 when it is not a length or
 * cannot be resolved. min() and clamp() compare any units then. */
int wf_css_math_px(const char *s, int vw, int vh, int em_px, int rem_px, int *px);
/* Lot 90: "N.NNunit" (at most two decimals, half away from zero) for a
 * value in thousandths; the length, -1 when cap is too small. */
int wf_css_token(int milli, const char *unit, char *out, int cap);
#endif
