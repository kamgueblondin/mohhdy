/* wf_section.h - Lot 71: long pages in the guest browser.
 *
 * The web engine lays out at most WE_MAX_NODES (160) nodes from a 16 KiB
 * source. A real page reduced by the reader view (wf_html_reduce_ex with
 * a large node budget) is kept whole in a bounded buffer and cut here into
 * sections of at most WF_SECTION_NODES nodes and WF_SECTION_BYTES bytes,
 * preferably between blocks. Each section is a complete document: the
 * page title, the elements still open at the cut reopened (ul, ol,
 * blockquote...), the content, the elements closed again and a footer
 * "section k/n". Scrolling past the end of a section opens the next one
 * (userspace/osui_gui.c), so a page of several thousand nodes is read in
 * bounded memory. Form placeholders (\001N\002, see wf_form.h) are
 * replaced by the current value of field N while a section is written.
 * Host tested: tests/unit/userspace/test_wf_section.c. */
#ifndef WF_SECTION_H
#define WF_SECTION_H
#include "wf_form.h"

#define WF_SECTION_MAX 96
#define WF_SECTION_NODES 120
#define WF_SECTION_BYTES 12000
#define WF_SECTION_DEPTH 8
#define WF_SECTION_TAG 12

typedef struct {
    int n;                                   /* sections */
    int body_start, body_end;                /* body content in the source */
    int title_start, title_end;              /* title text in the source */
    int off[WF_SECTION_MAX + 1];             /* section k is [off[k], off[k+1]) */
    uint8_t depth[WF_SECTION_MAX + 1];       /* open elements at off[k] */
    char open[WF_SECTION_MAX + 1][WF_SECTION_DEPTH][WF_SECTION_TAG];
    int nodes;                               /* nodes counted in the whole page */
    int cut;                                 /* page longer than WF_SECTION_MAX sections */
} wf_sections_t;

/* Indexes a reduced document (the output of wf_html_reduce_ex). Returns
 * the section count (>= 1), or -1 when src is not a reduced document. */
int wf_section_index(const char* src, int n, wf_sections_t* s);
/* Writes section k as a complete document into out (NUL-terminated).
 * forms may be 0 (placeholders then show "[ ]"). Returns the length or -1. */
int wf_section_render(const char* src, int n, const wf_sections_t* s, int k, const wf_form_page_t* forms,
                      char* out, int cap);
/* Section that holds byte offset pos of the source, or 0. */
int wf_section_of(const wf_sections_t* s, int pos);
#endif
