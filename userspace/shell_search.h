#ifndef MOHHDY_SHELL_SEARCH_H
#define MOHHDY_SHELL_SEARCH_H
/* Lot 69: `search`, `search-open`, `search-engine` (console and GUI share
 * the same results). The page comes from a no-JS search engine over the
 * Lot 67 fetch stack (wf_guest_fetch), parsed by userspace/wsearch.c. */
int shell_search_is(const char* line);
int shell_search_line(const char* line);
/* "search:QUERY" for the /browser loader: the results as a small HTML
 * page with links (same contract as shell_fetch_page) */
int shell_search_page(const char* query, char* src, int src_max, char* final_url, int url_max, char* status, int st_max);
/* URL of result n (1-based) of the last search; 0 or -1 */
int shell_search_url(int n, char* url, int max);
#endif
