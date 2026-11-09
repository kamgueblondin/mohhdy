/* wf_blocks.h - Lot 86: tables, definition lists, figures and details
 * from a page, as plain text. The layout engine already keeps a table
 * as paragraphs; this is the aligned reading of the same markup for
 * web-read and for a fixture check. No JavaScript, no site CSS. Bounded,
 * no heap. */
#ifndef WF_BLOCKS_H
#define WF_BLOCKS_H

typedef struct {
    int tables;
    int rows;
    int cells;
    int defs;
    int figures;
    int details;
    int truncated;
} wf_blocks_stats_t;

/* Bytes written (NUL-terminated). stats may be 0. */
int wf_blocks_text(const char *html, int n, char *out, int cap, wf_blocks_stats_t *st);
#endif
