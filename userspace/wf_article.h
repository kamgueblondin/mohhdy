/* wf_article.h - Lot 85: plain text of a page the reader kept.
 * Headings, paragraphs, lists, tables and preformatted blocks stay in
 * order. Tags, scripts and styles do not. The result is bounded by the
 * caller. No heap, no network. */
#ifndef WF_ARTICLE_H
#define WF_ARTICLE_H

/* Writes the article into out (NUL-terminated). Returns the length,
 * not counting the NUL. A short or empty page yields 0. */
int wf_article_text(const char *html, int n, char *out, int cap);
#endif
