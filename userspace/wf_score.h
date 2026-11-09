/* wf_score.h - Lot 85: a content block when the page has no article,
 * main, role=main or content id. News and docs sites often wrap the
 * text in a div whose class is article-body, markdown-body, rst-content
 * and so on. This scores those blocks by class and by how much of the
 * text sits outside links. It does not run JavaScript and it does not
 * fetch anything. */
#ifndef WF_SCORE_H
#define WF_SCORE_H

/* Hint for a class or id. A content root is a large positive number.
 * Site chrome (sidebar, breadcrumb, comments, ads) is negative.
 * Anything else is 0. */
int wf_score_hint(const char *id, const char *cls);

/* 1 when this class or id is news/docs chrome the reader should skip. */
int wf_score_is_chrome(const char *id, const char *cls);

/* Interior of the best block, in[lo, hi). Returns 1 when a block was
 * selected. text and links may be 0. A page that is already one short
 * block, or a link list, is left alone (return 0). */
int wf_score_pick(const char *in, int n, int *lo, int *hi, int *text, int *links);
#endif
