/* wf_idna.h - Lot 89: internationalised host names for the registrable
 * domain (wf_regdom) and anything else that compares hosts.
 *
 * A host may arrive as UTF-8 ("b\xc3\xbccher.example" in C notation, typed or from a link) or
 * in its ASCII form ("xn--bcher-kva.example", from a redirect or the
 * address bar). Both name one host, so hosts are compared in the ASCII
 * form:
 * - wf_idna_to_ascii: per label, UTF-8 is decoded, lower-cased with a
 *   small hand-written case map (ASCII, Latin-1, Latin Extended-A, modern
 *   Greek U+0386..U+03AB, Cyrillic U+0400..U+042F; not the full Unicode
 *   tables; capital I with dot, which lowers to two code points, is
 *   refused), and Punycode-encoded (RFC
 *   3492) behind "xn--". The ideographic and full-width full stops count
 *   as dots, as browsers do. Nontransitional: sharp s stays sharp s.
 * - wf_idna_to_unicode: "xn--" labels decoded back to UTF-8 for display.
 * Not UTS-46: no NFC normalisation, no bidi or joiner rules, no
 * disallowed code point table (controls, spaces and symbols below U+00A0
 * other than letters, digits and '-' are refused). Integer only, no
 * heap, bounded by WF_IDNA_MAX. Host tested (test_lot88_idna.c). */
#ifndef WF_IDNA_H
#define WF_IDNA_H
#include <stdint.h>

#define WF_IDNA_MAX 254          /* a host, dots included */
#define WF_IDNA_LABEL 63         /* one ASCII label */
#define WF_IDNA_CPS 64           /* code points of one label */

/* 0 and the ASCII form (lower case), or -1 (bad UTF-8, a refused code
 * point, an empty or too long label, out too small). */
int wf_idna_to_ascii(const char *host, char *out, int cap);
/* 0 and the UTF-8 form, or -1 when an "xn--" label does not decode or
 * does not encode back to itself. ASCII labels are lower-cased. */
int wf_idna_to_unicode(const char *host, char *out, int cap);
/* RFC 3492 on one label of code points: the encoded length, or -1 */
int wf_puny_encode(const uint32_t *cp, int n, char *out, int cap);
/* RFC 3492 decode of one label (without "xn--"): code points, or -1 */
int wf_puny_decode(const char *in, int n, uint32_t *cp, int cap);
/* 1 when the host has a byte above 0x7f or an upper-case letter */
int wf_idna_needs_map(const char *host);
#endif
