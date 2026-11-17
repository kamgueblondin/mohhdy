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
/* Lot 109: s (a length or a math function) as a box scroller length
 * (height, width and their limits): as wf_css_math_px, and ex, ch and lh
 * are 5/16, 8/16 and 10/16 of em_px (the 8x8 font's x height, advance
 * and normal line height at 16 px: WF_CSSPROP_EX_NUM, WE_CHAR_W,
 * WE_LINE_H); % is of the containing block, unknown here. 1 and *px (px
 * thousandths) when there is no percentage; 2 and out (cap bytes) the
 * value as the engine's math reads it when there is one: "N%" or
 * "calc(N% + Mpx)" (integers, half away from zero, at most
 * WF_CSSMATH_LIT), or min()/max()/clamp() of those when it mixes them
 * (one such function, not nested and not in an arithmetic); 0 for an
 * empty string; -1 otherwise. */
#define WF_CSSMATH_LIT 2000       /* webeng.h WE_MATH_LITERAL_MAX */
/* Lot 111: a value mixing px and % may nest min()/max()/clamp() and sit
 * in a sum or a product with a number: the sums and factors go into the
 * arguments (min(a, b) + c is min(a + c, b + c), -clamp(a, b, c) is
 * min(-a, max(-b, -c))), written within the engine's bounds */
#define WF_CSSMATH_NODES 24       /* webeng.h WE_MATH_NODES_MAX */
#define WF_CSSMATH_FN_DEPTH 4     /* webeng.h WE_MATH_DEPTH_MAX */
#define WF_CSSMATH_ARGS 4         /* arguments of one function (webeng.c math_atom) */
int wf_css_math_box(const char *s, int vw, int vh, int em_px, int rem_px, int *px, char *out, int cap);
/* Lot 111: wf_css_math_box with lh against lh_px, the element's line
 * height in px thousandths (0..WF_CSSMATH_MAX), when it is not negative;
 * a negative lh_px is line-height normal (10/16 of em_px, as above) */
int wf_css_math_box_lh(const char *s, int vw, int vh, int em_px, int rem_px, int lh_px, int *px, char *out, int cap);
/* Lot 111: wf_css_math_box_lh with a percentage of basis (px thousandths,
 * 0..WF_CSSMATH_MAX: the containing block is known), so every value is
 * px: 1 and *px (px thousandths), -1 otherwise (0 for an empty string is
 * -1 here) */
int wf_css_math_box_at(const char *s, int vw, int vh, int em_px, int rem_px, int lh_px, int basis, int *px);
/* Lot 112: the inputs of a box length in one place: the viewport, em,
 * rem, the element's line height (lh) and the root element's (rlh) in px
 * thousandths, each negative for line-height normal (10/16 of em_px,
 * 10/16 of rem_px for rlh), and the basis of a percentage (negative: the
 * containing block is unknown) */
typedef struct {
    int vw, vh, em_px, rem_px, lh_px, rlh_px, basis;
    /* Lot 112: ex and ch of the element's actual font (px thousandths,
     * the engine's glyph metrics for its font size), each negative for the
     * 16 px proportions (5/16 and 8/16 of em_px) */
    int ex_px, ch_px;
} wf_css_box_ctx_t;
/* Lot 112: wf_css_math_box_lh or, with a basis, wf_css_math_box_at (out
 * may then be 0), with rlh against c->rlh_px */
int wf_css_math_box_ctx(const char *s, const wf_css_box_ctx_t *c, int *px, char *out, int cap);
#endif
