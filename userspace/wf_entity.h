/* wf_entity.h - Lot 71: HTML character references and UTF-8 text for the
 * Ring 3 web stack. Text of fetched pages (UTF-8, or Latin-1 / CP1252 when
 * the page says so) becomes display bytes (include/os_charset.h, CP1252)
 * so French accents reach the desktop fonts instead of '?'.
 * Bounded, no allocation, host tested (tests/unit/userspace/test_wf_text.c). */
#ifndef WF_ENTITY_H
#define WF_ENTITY_H
#include <stdint.h>

/* Source charset of a page. */
enum { WF_CS_UTF8 = 0, WF_CS_LATIN1 = 1 };

/* Code point of the character reference at s[0] == '&' (n bytes left):
 * named (&eacute; &amp; ...; the ';' may be missing for the Latin-1 names
 * as browsers allow), decimal &#233; or hexadecimal &#xE9;. Returns the
 * code point and sets *used to the bytes consumed, or 0 (not a reference,
 * *used = 0). Numeric references above U+10FFFF, surrogates and NUL give
 * U+FFFD. */
uint32_t wf_entity_decode(const char *s, int n, int *used);
/* Looks a name up (without '&' and ';'); 0 when unknown. */
uint32_t wf_entity_lookup(const char *name, int len);

/* charset from a Content-Type value or a <meta charset> / http-equiv
 * value: WF_CS_LATIN1 for iso-8859-1, latin1, windows-1252, cp1252,
 * us-ascii; WF_CS_UTF8 otherwise. */
int wf_charset_from(const char *v, int n);
/* Lot 71: charset parameter of a Content-Type header, or WF_CS_NONE when
 * the header has none (then a <meta> of the page decides). */
#define WF_CS_NONE (-1)
int wf_charset_header(const char *v, int n);
/* Looks for <meta charset=...> or <meta http-equiv content="...charset=">
 * in the first 2 KiB; returns WF_CS_* or -1 when none. */
int wf_charset_sniff(const char *html, int n);

/* Next source character of text in charset cs at s[*i] (bytes up to n),
 * entities decoded when ent is set; returns the code point (0 at the end). */
uint32_t wf_text_next(const char *s, int n, int *i, int cs, int ent);

/* Text to display bytes: entities decoded, whitespace collapsed when
 * collapse is set, controls dropped. Returns the length (out is always
 * NUL-terminated, cap > 0). */
int wf_text_display(const char *s, int n, int cs, int collapse, char *out, int cap);
/* Display bytes (CP1252) to UTF-8, for form values sent to a server. */
int wf_display_to_utf8(const char *s, int n, char *out, int cap);
#endif
