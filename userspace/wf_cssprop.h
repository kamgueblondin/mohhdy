/* wf_cssprop.h - Lot 90 (US-031): @property for the reader sheet.
 *
 * CSS Properties and Values API 1 lets a sheet register a custom property:
 *
 *   @property --gap { syntax: '<length>'; inherits: false; initial-value: 4px; }
 *
 * A registered property has a type, an initial value and says whether it
 * inherits. The reader keeps a subset:
 *   - syntax: '*', or up to WF_CSSPROP_ALTS alternatives joined by '|'
 *     (Lot 91; Lot 90 took one type). An alternative is a type name
 *     ('<length>', '<percentage>', '<length-percentage>', '<number>',
 *     '<integer>', '<color>', Lot 91: '<angle>', '<time>',
 *     '<custom-ident>') or a keyword (an identifier of 1..15 bytes that is
 *     not a CSS-wide keyword nor "default"), with an optional multiplier
 *     right after it: '+' a space separated list, '#' a comma separated
 *     list (one or more items each). Type names are ASCII case
 *     insensitive; keywords compare lower case like every value here;
 *   - inherits: true or false (required);
 *   - initial-value: required unless the syntax is '*'; it must match the
 *     syntax and be computationally independent (no em, rem, ex, ch, no
 *     var()), else the whole rule is invalid and ignored.
 * A value that does not match the syntax of its registration is invalid
 * at computed-value time: the property then acts as unset (the inherited
 * value when it inherits, else the initial value). The alternatives are
 * tried in order; the value matches when one of them does.
 * Lot 91: calc(), min(), max() and clamp() are type checked (CSS Values
 * 4): + and - need the same type (a length and a percentage give a
 * length-percentage), * needs a number on one side, / a number on the
 * right; min() and max() take one or more arguments of compatible types,
 * clamp() three; + and - need white space on both sides. The result
 * matches <length> when it is a length, <percentage> a percentage,
 * <length-percentage> any of the three, <number> and <integer> a number
 * (an integer context rounds), <angle> an angle, <time> a time. Lot 90
 * took any balanced math function as a match for the numeric types.
 * Lot 92: abs(), sign(), round() (with an optional nearest, up, down or
 * to-zero strategy; B may be left out only when A is a number), mod()
 * and rem() are typed and simplified; sin(), cos(), tan() (a number or an
 * angle in, a number out), asin(), acos(), atan() (a number in, an angle
 * out) and atan2() (two values of one type, an angle out) are typed but
 * not simplified. A registered value that is one math function and
 * matches one numeric alternative without a multiplier is computed
 * (wf_cssprop_compute): absolute lengths in px (in 96px, cm, mm, q, pt,
 * pc), angles in deg (grad, turn, rad with pi as 3.1415927), times in s,
 * numbers, and percentages alone, each operation in 1/10000 of the unit
 * rounded half away from zero; values past 100000 units, relative
 * lengths (em, rem, ex, ch, vw, vh, vmin, vmax), a length mixed with a
 * percentage, a division or a round() step of zero, and the trigonometric
 * functions are kept as text.
 * Lot 93: the trigonometric functions are simplified (an angle in deg is
 * reduced exactly; a number in radians is known up to 100; tan() at an
 * odd multiple of 90deg is infinite and kept; asin() and acos() past
 * [-1, 1] are NaN and kept), and pow(A, B), sqrt(A), log(A), log(A, B),
 * exp(A) (numbers in, a number out) and hypot(A, ...) (values of one type,
 * that type out) are typed and simplified. The constants e and pi are
 * 2.7183 and 3.1416 (1/10000, like every value here); infinity,
 * -infinity and nan are numbers that are never known (kept). sqrt(),
 * hypot() and pow() with an integer exponent (up to 127, down to -8, while
 * the power fits 127 bits) are exact; the other results are computed in
 * 62-bit fixed point with 128-bit intermediates (no libm, no 64-bit
 * division) and are within about 1e-12 (relative) of the exact value
 * before the final rounding half away from zero to 1/10000.
 * Lot 95: a syntax takes up to WF_CSSPROP_ALTS (8) alternatives (Lot 91:
 * 4) and the types '<url>' (url(...), quoted or not), '<string>' (a
 * quoted string), '<image>' (a url() or a linear, radial or conic
 * gradient, repeating or not, with balanced parentheses; the stops are
 * not checked here), '<resolution>' (dpi, dpcm, dppx, x; 96dpi is
 * 1dppx), '<transform-function>' (translate, translatex, translatey,
 * translatez, translate3d, scale, scalex, scaley, scalez, scale3d, rotate,
 * rotatex, rotatey, rotatez, skew, skewx, skewy, matrix, perspective,
 * with typed arguments) and '<transform-list>' (one or more transform
 * functions separated by white space; no multiplier). Names may have 31
 * bytes and values 127 (Lot 90: 23 and 63).
 * The computed value (wf_cssprop_compute_ctx) is the value with each
 * numeric part in its canonical unit: plain dimensions too (1in -> 96px,
 * 100grad -> 90deg, 500ms -> 0.5s, 96dpi -> 1dppx, 0 -> 0px, +1.50 ->
 * 1.5), each item of a '+' or '#' list (joined by one space or ", "),
 * the arguments of transform functions (translate(1in, 10%) ->
 * translate(96px, 10%), rotate(0) -> rotate(0deg)), a length mixed with
 * a percentage as calc(P% + Lpx) (calc(1in - 6px + 5%) -> calc(5% +
 * 90px); min(), max(), clamp() and the other functions of a mix are not
 * known), and vw, vh, vmin and vmax with the viewport of the context.
 * em, rem, ex and ch are still not known; an item that is not known
 * keeps its text. Colors, idents, keywords, urls, strings and images keep
 * their text.
 * Names and values are compared lower case. No heap. */
#ifndef WF_CSSPROP_H
#define WF_CSSPROP_H

#define WF_CSSPROP_NAME 32        /* Lot 95: 31 bytes (Lot 90: 24) */
#define WF_CSSPROP_VAL 128        /* Lot 95: 127 bytes (Lot 90: 64) */
#define WF_CSSPROP_ALTS 8         /* alternatives in a syntax (Lot 91: 4, Lot 95: 8) */
#define WF_CSSPROP_IDENT 16       /* Lot 91: a keyword of a syntax, NUL included */

enum {
    WF_CSSPROP_BAD = 0,
    WF_CSSPROP_ANY = 1,           /* '*' */
    WF_CSSPROP_LENGTH = 2,
    WF_CSSPROP_PERCENT = 3,
    WF_CSSPROP_LENPCT = 4,
    WF_CSSPROP_NUMBER = 5,
    WF_CSSPROP_INTEGER = 6,
    WF_CSSPROP_COLOR = 7,
    /* Lot 91 */
    WF_CSSPROP_ANGLE = 8,
    WF_CSSPROP_TIME = 9,
    WF_CSSPROP_CIDENT = 10,       /* <custom-ident> */
    WF_CSSPROP_KEYWORD = 11,      /* a keyword of the syntax (alt_ident) */
    WF_CSSPROP_COMPLEX = 12,      /* several alternatives, a multiplier or a keyword */
    /* Lot 95 */
    WF_CSSPROP_URL = 13,
    WF_CSSPROP_IMAGE = 14,
    WF_CSSPROP_RESOLUTION = 15,
    WF_CSSPROP_TFUNC = 16,        /* <transform-function> */
    WF_CSSPROP_TLIST = 17,        /* <transform-list> */
    WF_CSSPROP_STRING = 18
};

/* Lot 95: what a computed value may use: the viewport in px (0: vw, vh,
 * vmin and vmax are not known) */
typedef struct { int vw, vh; } wf_cssprop_ctx_t;

/* Lot 91: the multiplier of an alternative */
enum { WF_CSSPROP_ONE = 0, WF_CSSPROP_PLUS = 1, WF_CSSPROP_HASH = 2 };

typedef struct {
    unsigned char type;           /* WF_CSSPROP_LENGTH .. WF_CSSPROP_KEYWORD */
    unsigned char mult;           /* WF_CSSPROP_ONE, _PLUS, _HASH */
    char ident[WF_CSSPROP_IDENT]; /* the keyword, lower case */
} wf_cssprop_alt_t;

typedef struct {
    char name[WF_CSSPROP_NAME];   /* "--x", lower case */
    unsigned char syntax;         /* WF_CSSPROP_* (one type alone, ANY, or COMPLEX) */
    unsigned char inherits;       /* 0 or 1 */
    unsigned char has_initial;
    char initial[WF_CSSPROP_VAL]; /* lower case, outer spaces trimmed */
    /* Lot 91: the alternatives (nalt 0 for '*') */
    unsigned char nalt;
    wf_cssprop_alt_t alt[WF_CSSPROP_ALTS];
} wf_cssprop_t;

/* Lot 91: the syntax descriptor value, quotes included, n bytes, parsed
 * into out (nalt and alt; syntax as wf_cssprop_syntax gives it). Returns
 * that code, WF_CSSPROP_BAD when it is not valid (out->nalt 0 then). */
int wf_cssprop_syntax_parse(const char *s, int n, wf_cssprop_t *out);

/* Lot 91: 1 when value (lower case, trimmed) matches the registration's
 * syntax (its alternatives). */
int wf_cssprop_match_prop(const wf_cssprop_t *p, const char *value);

/* Lot 91: the type of a math function (calc, min, max, clamp; Lot 92 also
 * abs, sign, round, mod, rem, sin, cos, tan, asin, acos, atan, atan2; Lot
 * 93 pow, sqrt, hypot, log, exp and the constants; the whole text): WF_CSSPROP_NUMBER, _LENGTH, _PERCENT, _LENPCT
 * (a length mixed with a percentage), _ANGLE, _TIME, or WF_CSSPROP_BAD
 * when it is not one or its types do not combine. */
int wf_cssprop_math_type(const char *value);

/* Lot 92: the value of a math function (the whole text, lower case) in
 * its canonical unit ("6px", "33.3333%", "90deg", "1.5s", "-2"): 1 and
 * out written, *type its type; 0 when it is a valid math function whose
 * value is not known here (out ""; *type still set); -1 when it is not
 * a valid math function. */
int wf_cssprop_math_eval(const char *value, int *type, char *out, int cap);

/* Lot 92: the computed value of a registered value: 1 and out written
 * when value is one math function, its first matching alternative is
 * one numeric type without a multiplier (<length>, <percentage>,
 * <length-percentage>, <number>, <integer>, <angle>, <time>) and its
 * value is known; <integer> rounds half toward +infinity. 0 otherwise
 * (the caller keeps the text). */
int wf_cssprop_compute(const wf_cssprop_t *p, const char *value, char *out, int cap);

/* Lot 95: the computed value of a registered value (see the header
 * comment): 1 and out written when it differs from value, 0 when value
 * is kept as it is (it does not match, nothing in it is computed, or out
 * is too small). c may be 0 (no viewport). wf_cssprop_compute is this
 * with c 0. */
int wf_cssprop_compute_ctx(const wf_cssprop_t *p, const char *value, const wf_cssprop_ctx_t *c, char *out, int cap);

/* The syntax descriptor value, quotes included ("'<length>'"), n bytes:
 * the type code when the syntax is one type without a multiplier,
 * WF_CSSPROP_ANY for '*', Lot 91: WF_CSSPROP_COMPLEX for alternatives, a
 * multiplier or a keyword; WF_CSSPROP_BAD when it is not valid (no
 * quotes, an unknown type, more than WF_CSSPROP_ALTS alternatives). */
int wf_cssprop_syntax(const char *s, int n);

/* 1 when value (lower case, trimmed) matches one type (a code from
 * WF_CSSPROP_ANY to WF_CSSPROP_CIDENT, Lot 95 also WF_CSSPROP_URL to
 * WF_CSSPROP_STRING, no multiplier). */
int wf_cssprop_match(int syntax, const char *value);

/* 1 when value is computationally independent (no em, rem, ex, ch unit
 * and no var()). */
int wf_cssprop_independent(const char *value);

/* One @property rule: the prelude after "@property" (the name, n bytes at
 * pre) and the block body (bn bytes at body, without braces). 1 and out
 * filled when the rule is valid, 0 when it is not (out cleared). */
int wf_cssprop_parse(const char *pre, int pn, const char *body, int bn, wf_cssprop_t *out);

/* ---- Lot 97: font-relative lengths, interpolation and timing ---------
 * wf_cssprop_compute_font is wf_cssprop_compute_ctx with a font context:
 * em is the element's font size and rem the root's, both in px
 * thousandths (0: not known, the unit is kept as text). Lot 98: ex and ch
 * come from the metrics of the font the web engine lays text out with
 * (the 8x8 glyphs of kernel/gfx_font8.h in the 8x10 cell of a 16 px font,
 * userspace/webeng.h): ex is the height of the inked rows of "x" (5 px of
 * 16, 5/16 em) and ch the advance of "0" (WE_CHAR_W 8 px of 16, 1/2 em);
 * Lot 97 used 1/2 em for both. Each item is rounded half away from zero
 * to 1/10000 px, from the exact product of its value and the font size.
 *
 * wf_cssprop_interp interpolates two computed values of a registration
 * at progress t (1/10000, may leave 0..10000 after an easing): both must
 * match the same alternative of the syntax, else (or for '*', keywords,
 * idents, urls, strings, images, transform functions and lists, a list of
 * another length, a value that is not known as a number) the values flip
 * at t = 5000 (discrete, CSS Properties and Values 1). Numbers, lengths,
 * percentages, angles, times and resolutions interpolate as
 * a + round((b - a) t / 10000) (half away from zero) in their canonical
 * unit; a length with a percentage gives calc(P% + Lpx); an integer is
 * rounded half toward +infinity; '+' and '#' lists item by item. Colours
 * (hex, rgb(), rgba(), the 19 names, transparent) interpolate in sRGB with
 * premultiplied alpha (CSS Color 4 for legacy colours), each channel
 * rounded half away from zero to 0..255, alpha to 1/10000 in 0..1, written
 * "rgb(R, G, B)" or "rgba(R, G, B, A)"; currentcolor is discrete. Lot 98:
 * hsl() and hsla() (hue as a number of degrees or with deg, grad, rad or
 * turn; saturation and lightness as percentages, or numbers in the space
 * form; commas, or spaces with "/ alpha") interpolate too: they are
 * legacy colours, so they interpolate in sRGB (CSS Color 4), after the
 * conversion of CSS Color 4 (hsl to rgb) with each channel rounded half
 * away from zero to 1/10000 of 0..255; saturation and lightness are
 * clamped to 0..100%, the hue taken modulo 360deg. t 0 gives a and t
 * 10000 gives b as written. Returns 2 (interpolated), 1 (discrete) or 0
 * (out too small, or no value).
 *
 * Lot 98: wf_cssprop_add composes value v on top of the underlying value
 * under (Web Animations 1 composite operations add and accumulate, which
 * this reader does not tell apart: they differ only for types it keeps
 * discrete, such as transforms): numbers, lengths, percentages,
 * length-percentage mixes, angles, times, resolutions and integers are
 * summed in their canonical unit; colours are summed in premultiplied
 * sRGB (the alpha sum clamped to 1, then each channel, the premultiplied
 * sum over that alpha rounded half away from zero, to 0..255); '+' and
 * '#' lists item by item when the lengths match (CSS Properties and
 * Values 1: simple lists). Returns 2 (composed into out), 1 (the values do
 * not add: out is v, the composite falls back to replace) or 0 (out too
 * small, or no value). */
#define WF_CSSPROP_EX_NUM 5      /* Lot 98: ex = 5/16 em (inked rows of "x" in the 8x8 font) */
#define WF_CSSPROP_EX_DEN 16
#define WF_CSSPROP_CH_NUM 8      /* Lot 98: ch = 8/16 em (WE_CHAR_W at WE_FONT_PX_DEFAULT) */
#define WF_CSSPROP_CH_DEN 16
typedef struct { int vw, vh, em, rem; } wf_cssprop_fctx_t;
int wf_cssprop_compute_font(const wf_cssprop_t *p, const char *value, const wf_cssprop_fctx_t *c, char *out, int cap);
int wf_cssprop_interp(const wf_cssprop_t *p, const char *a, const char *b, int t, char *out, int cap);
int wf_cssprop_add(const wf_cssprop_t *p, const char *under, const char *v, char *out, int cap);

/* An easing function (CSS Easing 1): linear, ease, ease-in, ease-out,
 * ease-in-out, cubic-bezier(x1, y1, x2, y2) (x in 0..1, y in -100..100,
 * four decimals), step-start, step-end and steps(n[, jump-start |
 * jump-end | jump-none | jump-both | start | end]) with 1 <= n <= 10000
 * (2 for jump-none). linear() is not taken. */
enum { WF_CSSEASE_LINEAR = 0, WF_CSSEASE_BEZIER = 1, WF_CSSEASE_STEPS = 2 };
enum { WF_CSSEASE_END = 0, WF_CSSEASE_START = 1, WF_CSSEASE_NONE = 2, WF_CSSEASE_BOTH = 3 };
typedef struct {
    unsigned char kind, pos;
    int n;                        /* steps */
    int x1, y1, x2, y2;           /* cubic-bezier, 1/10000 */
} wf_cssease_t;
/* 1 and *e when s[0..n) is one easing function (lower case), else 0 */
int wf_cssease_parse(const char *s, int n, wf_cssease_t *e);
/* the output progress at input progress x (1/10000, 0..10000); before:
 * the before flag of the steps (the animation is in its before phase).
 * A cubic-bezier is solved by bisection on its parameter in 1/2^30 with
 * 128-bit products (within 1/10000 of the exact curve); steps are exact
 * (step / jumps rounded half away from zero). */
int wf_cssease_at(const wf_cssease_t *e, int x, int before);

/* One animation of the animation shorthand (CSS Animations 1): the
 * keyframes name ("" for none), duration and delay in ms (seconds and
 * milliseconds rounded half away from zero to 1 ms, at most 1e8 ms; a
 * negative duration is invalid), the iteration count in 1/10000 (-1
 * infinite), direction, fill mode, play state and easing. A keyword that
 * a property not yet given accepts goes to that property before the name
 * (so "ease" alone is an easing, "ease ease" names ease). */
#define WF_CSSANIM_MAX 4          /* animations (or transitions) in one list */
#define WF_CSSANIM_MS_MAX 100000000
enum { WF_CSSANIM_NORMAL = 0, WF_CSSANIM_REVERSE = 1, WF_CSSANIM_ALTERNATE = 2, WF_CSSANIM_ALT_REVERSE = 3 };
enum { WF_CSSANIM_FILL_NONE = 0, WF_CSSANIM_FORWARDS = 1, WF_CSSANIM_BACKWARDS = 2, WF_CSSANIM_BOTH = 3 };
/* Lot 98: animation-composition (CSS Animations 2) */
enum { WF_CSSANIM_REPLACE = 0, WF_CSSANIM_ADD = 1, WF_CSSANIM_ACCUMULATE = 2 };
typedef struct {
    char name[WF_CSSPROP_NAME];
    int dur, delay, iters;
    unsigned char dir, fill, paused;
    unsigned char comp;           /* Lot 98: WF_CSSANIM_REPLACE, _ADD or _ACCUMULATE */
    wf_cssease_t ease;
} wf_cssanim_t;
/* the comma separated list v (lower case, trimmed): the number of
 * animations written to out (at most max), -1 when the list is not valid
 * or longer than max ("none" alone is one animation without a name) */
int wf_cssanim_parse(const char *v, wf_cssanim_t *out, int max);

/* One transition of the transition shorthand (CSS Transitions 1 and 2):
 * the property ("all", a name, or "" for none), duration (>= 0) and delay
 * in ms, easing, and 1 for allow-discrete (transition-behavior). */
typedef struct {
    char prop[WF_CSSPROP_NAME];
    int dur, delay;
    unsigned char discrete;
    wf_cssease_t ease;
} wf_csstrans_t;
/* as wf_cssanim_parse; "none" is valid alone only */
int wf_csstrans_parse(const char *v, wf_csstrans_t *out, int max);

/* Lot 98: the longhands (CSS Animations 1 and 2, CSS Transitions 1 and
 * 2). Each is a comma separated list of one token per item:
 *   animation-name (none or an identifier that is not a CSS-wide
 *   keyword), animation-duration (a time >= 0, or auto: 0s),
 *   animation-timing-function (an easing function), animation-delay (a
 *   time), animation-iteration-count (infinite or a number 0..100000),
 *   animation-direction, animation-fill-mode, animation-play-state and
 *   animation-composition (replace, add, accumulate);
 *   transition-property (all, an identifier, or none alone),
 *   transition-duration (>= 0), transition-timing-function,
 *   transition-delay, transition-behavior (normal, allow-discrete).
 * wf_cssanim_long and wf_csstrans_long write the field of longhand which
 * in out[0..count) (the other fields are left as they are) and return the
 * count, -1 when the list is not valid or longer than max. */
enum {
    WF_CSSANIM_L_NAME = 0, WF_CSSANIM_L_DURATION, WF_CSSANIM_L_TIMING, WF_CSSANIM_L_DELAY, WF_CSSANIM_L_COUNT,
    WF_CSSANIM_L_DIRECTION, WF_CSSANIM_L_FILL, WF_CSSANIM_L_PLAY, WF_CSSANIM_L_COMPOSITION, WF_CSSANIM_LONGS
};
enum {
    WF_CSSTRANS_L_PROPERTY = 0, WF_CSSTRANS_L_DURATION, WF_CSSTRANS_L_TIMING, WF_CSSTRANS_L_DELAY,
    WF_CSSTRANS_L_BEHAVIOR, WF_CSSTRANS_LONGS
};
int wf_cssanim_long(int which, const char *v, wf_cssanim_t *out, int max);
int wf_csstrans_long(int which, const char *v, wf_csstrans_t *out, int max);
/* Lot 98: the coordinated lists (CSS Values 4, coordinating list-valued
 * properties). v[k] is the value of longhand k (0: its initial value),
 * sh[k] 1 when v[k] is a list of the shorthand (whose items give longhand
 * k). The number of animations is the length of the animation-name list
 * (of transitions, of transition-property); the other lists are repeated
 * or cut to that length. A list that does not parse counts as its
 * initial value. Returns the count written to out (at most max). */
int wf_cssanim_coord(const char *const *v, const unsigned char *sh, wf_cssanim_t *out, int max);
int wf_csstrans_coord(const char *const *v, const unsigned char *sh, wf_csstrans_t *out, int max);

/* Lot 97: the directed progress of one animation at local time ms (CSS
 * Animations 1 and Web Animations 1 timing: delay, duration, iteration
 * count, direction, fill mode; a paused animation stays at time 0, the
 * document start): 1 and *p (1/10000, 0..10000), *before (the before
 * phase, for steps) when the animation has an effect, 0 when it has none
 * (before or after its active interval without the fill mode, a zero
 * duration with an infinite count). Iteration and progress are taken
 * from the exact elapsed time: iteration floor(e / dur), progress
 * floor(10000 (e mod dur) / dur); at the end of a whole count the
 * progress is 10000 of the last iteration; a fractional count ends at
 * its fraction. */
int wf_cssanim_progress(const wf_cssanim_t *a, int ms, int *p, int *before);
#endif
