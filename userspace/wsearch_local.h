/* wsearch_local.h - Lot 85: search the guest's own catalogue.
 * No network. The catalogue is the shell commands and the concepts the
 * desktop actually implements (fetch, reader, masonry, form factor, the
 * simulated battery). `search-engine local` uses it; a network engine
 * that returns nothing does not call it by itself. */
#ifndef WSEARCH_LOCAL_H
#define WSEARCH_LOCAL_H
#include "wsearch.h"

#define WSEARCH_LAYOUT_LOCAL 14

/* Fills page with up to WSEARCH_MAX hits. Returns the count. */
int wsearch_local_query(const char *query, wsearch_page_t *page);
#endif
