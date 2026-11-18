/* wf_jslayout.h - Lot 115: the layout hook of the page script pass
 * (wf_jspage_set_layout in wf_jsdom.h) over the engine the GUI paints
 * with (webeng.c). The shell sets it once; host tests call it directly.
 *
 * The engine document (sizeof(we_doc_t), under 1 MiB) is taken from the
 * user heap the first time a page script reads a layout value and kept;
 * wf_jslayout_release gives it back. The engine lays out at most
 * WE_MAX_NODES nodes: wf_jsdom.c cuts the copy it hands over below that
 * (WF_JSLAY_NODES), the elements past it keep the block model. External
 * style sheets are not fetched for this copy (the <style> elements and
 * style= attributes of the page are read). */
#ifndef WF_JSLAYOUT_H
#define WF_JSLAYOUT_H
#include "wf_jsdom.h"

/* wf_jslayout_fn: lays html out in a vw x vh viewport and writes the
 * element boxes in document order; the count, or -1 (no memory, the
 * engine refused the document) */
int wf_jslayout_engine(void *ctx, const char *html, int n, int vw, int vh, wf_jslay_box_t *out, int max);
void wf_jslayout_release(void);
/* layouts made and refused since the shell started (web-js status) */
void wf_jslayout_counts(int *made, int *refused);
#endif
