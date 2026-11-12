/* wf_cssprop.h - Lot 90 (US-031): @property for the reader sheet.
 *
 * CSS Properties and Values API 1 lets a sheet register a custom property:
 *
 *   @property --gap { syntax: '<length>'; inherits: false; initial-value: 4px; }
 *
 * A registered property has a type, an initial value and says whether it
 * inherits. The reader keeps a subset:
 *   - syntax: '*', '<length>', '<percentage>', '<length-percentage>',
 *     '<number>', '<integer>' and '<color>' (one type, no '+' or '#'
 *     lists, no '|' alternatives, no custom idents);
 *   - inherits: true or false (required);
 *   - initial-value: required unless the syntax is '*'; it must match the
 *     syntax and be computationally independent (no em, rem, ex, ch, no
 *     var()), else the whole rule is invalid and ignored.
 * A value that does not match the syntax of its registration is invalid
 * at computed-value time: the property then acts as unset (the inherited
 * value when it inherits, else the initial value). calc(), min(), max()
 * and clamp() are taken as a match for the numeric types when their
 * parentheses balance (the reader does not type-check inside them).
 * Names and values are compared lower case. No heap. */
#ifndef WF_CSSPROP_H
#define WF_CSSPROP_H

#define WF_CSSPROP_NAME 24
#define WF_CSSPROP_VAL 64

enum {
    WF_CSSPROP_BAD = 0,
    WF_CSSPROP_ANY = 1,           /* '*' */
    WF_CSSPROP_LENGTH = 2,
    WF_CSSPROP_PERCENT = 3,
    WF_CSSPROP_LENPCT = 4,
    WF_CSSPROP_NUMBER = 5,
    WF_CSSPROP_INTEGER = 6,
    WF_CSSPROP_COLOR = 7
};

typedef struct {
    char name[WF_CSSPROP_NAME];   /* "--x", lower case */
    unsigned char syntax;         /* WF_CSSPROP_* */
    unsigned char inherits;       /* 0 or 1 */
    unsigned char has_initial;
    char initial[WF_CSSPROP_VAL]; /* lower case, outer spaces trimmed */
} wf_cssprop_t;

/* The syntax descriptor value, quotes included ("'<length>'"), n bytes:
 * WF_CSSPROP_* or WF_CSSPROP_BAD (unknown, a list, alternatives, no
 * quotes). */
int wf_cssprop_syntax(const char *s, int n);

/* 1 when value (lower case, trimmed) matches the syntax. */
int wf_cssprop_match(int syntax, const char *value);

/* 1 when value is computationally independent (no em, rem, ex, ch unit
 * and no var()). */
int wf_cssprop_independent(const char *value);

/* One @property rule: the prelude after "@property" (the name, n bytes at
 * pre) and the block body (bn bytes at body, without braces). 1 and out
 * filled when the rule is valid, 0 when it is not (out cleared). */
int wf_cssprop_parse(const char *pre, int pn, const char *body, int bn, wf_cssprop_t *out);
#endif
