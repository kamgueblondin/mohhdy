/* wf_read.h - Lot 85: choose the readable part of a fetched page.
 * The layout engine holds 160 nodes. A real article is surrounded by
 * navigation, language lists and footers. This picks one content root
 * (article, main, role=main, or a known content id) when that root holds
 * the text, and names the boilerplate classes the reader should skip.
 * It does not run JavaScript and it does not fetch anything. */
#ifndef WF_READ_H
#define WF_READ_H

typedef struct {
    int candidates;     /* elements that looked like a content root */
    int chosen;         /* 0 none (the whole page), 1 a root was selected */
    int kind;           /* 1 article, 2 main, 3 role=main, 4 content id,
                           5 class or text-density block (wf_score) */
    int text_chars;     /* letters inside the chosen root */
    int link_chars;     /* letters inside <a> in that root */
    int boilerplate;    /* times wf_read_chrome said yes during a reduce */
} wf_read_info_t;

/* Interior of the content root, in[lo, hi). The whole page when no root
 * is clearly the article (lo = 0, hi = n). Returns 1 when a root was
 * selected. info may be 0. */
int wf_read_root(const char *in, int n, int *lo, int *hi, wf_read_info_t *info);

/* 1 when this class or id is site chrome (table of contents, navbox,
 * language list, cookie banner, edit links) and the reader should skip
 * the element. A content id is never chrome. */
int wf_read_chrome(const char *id, const char *cls);

/* Up to max headings (h1..h3) inside in[lo, hi), each line "N text\n".
 * Returns the length written. */
int wf_read_outline(const char *in, int lo, int hi, char *out, int cap, int max_heads);
#endif
