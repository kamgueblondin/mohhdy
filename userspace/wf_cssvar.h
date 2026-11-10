/* wf_cssvar.h - Lot 89 (US-031): var() substitution for the
 * reader sheet.
 *
 * The reader used to replace a value only when the whole value was one
 * var(--name) or var(--name, fallback), one level deep. Current sites
 * write var() inside other values: calc(var(--base) + 2px), a fallback
 * that is itself a var(), and custom properties defined from other ones
 * (--gap: var(--unit)). This substitutes every var() of a value, the
 * fallbacks and the values it brings in included, as CSS Variables 1
 * says, with bounds:
 *   - a reference chain deeper than WF_CSSVAR_DEPTH, a cycle included
 *     (--a: var(--b); --b: var(--a)), makes the value invalid;
 *   - an unknown property with no fallback makes it invalid (invalid at
 *     computed-value time: the declaration is dropped);
 *   - an empty fallback (var(--x,)) is an empty substitution;
 *   - a result that does not fit the caller's buffer makes it invalid,
 *     never a cut value.
 * Names are compared lower case, as the reader stores them. No heap; the
 * recursion is at most WF_CSSVAR_DEPTH + 1 frames of a few dozen bytes. */
#ifndef WF_CSSVAR_H
#define WF_CSSVAR_H

#define WF_CSSVAR_DEPTH 8
#define WF_CSSVAR_NAME 24

/* The value of the custom property name ("--x", lower case), 0 when it is
 * not defined. */
typedef const char *(*wf_cssvar_get_t)(const void *ctx, const char *name);

/* 0: no var() in the value (out gets a copy when it fits, else -1);
 * 1: every var() substituted into out; -1: invalid (see above) or a
 * malformed var() (no name, no closing parenthesis, a name that is not
 * --ident). out is an empty string after -1. */
int wf_cssvar_subst(const char *in, char *out, int cap, wf_cssvar_get_t get, const void *ctx);

/* How many var() references a value holds at its own level (0..), for
 * reports; -1 when one is malformed. */
int wf_cssvar_count(const char *in);
#endif
