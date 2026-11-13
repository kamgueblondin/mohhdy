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
 * Names and values are compared lower case. No heap. */
#ifndef WF_CSSPROP_H
#define WF_CSSPROP_H

#define WF_CSSPROP_NAME 24
#define WF_CSSPROP_VAL 64
#define WF_CSSPROP_ALTS 4         /* Lot 91: alternatives in a syntax */
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
    WF_CSSPROP_COMPLEX = 12       /* several alternatives, a multiplier or a keyword */
};

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

/* Lot 91: the type of a math function (calc, min, max, clamp; lower
 * case, the whole text): WF_CSSPROP_NUMBER, _LENGTH, _PERCENT, _LENPCT
 * (a length mixed with a percentage), _ANGLE, _TIME, or WF_CSSPROP_BAD
 * when it is not one or its types do not combine. */
int wf_cssprop_math_type(const char *value);

/* The syntax descriptor value, quotes included ("'<length>'"), n bytes:
 * the type code when the syntax is one type without a multiplier,
 * WF_CSSPROP_ANY for '*', Lot 91: WF_CSSPROP_COMPLEX for alternatives, a
 * multiplier or a keyword; WF_CSSPROP_BAD when it is not valid (no
 * quotes, an unknown type, more than WF_CSSPROP_ALTS alternatives). */
int wf_cssprop_syntax(const char *s, int n);

/* 1 when value (lower case, trimmed) matches one type (a code from
 * WF_CSSPROP_ANY to WF_CSSPROP_CIDENT, no multiplier). */
int wf_cssprop_match(int syntax, const char *value);

/* 1 when value is computationally independent (no em, rem, ex, ch unit
 * and no var()). */
int wf_cssprop_independent(const char *value);

/* One @property rule: the prelude after "@property" (the name, n bytes at
 * pre) and the block body (bn bytes at body, without braces). 1 and out
 * filled when the rule is valid, 0 when it is not (out cleared). */
int wf_cssprop_parse(const char *pre, int pn, const char *body, int bn, wf_cssprop_t *out);
#endif
