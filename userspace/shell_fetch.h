#ifndef MOHHDY_SHELL_FETCH_H
#define MOHHDY_SHELL_FETCH_H
/* Lot 67: `fetch` / `http-get` (real HTTP/HTTPS GET through the networker)
 * and the /browser network loader. One code path for the console and the
 * GUI: both call shell_fetch_page / wf_guest_fetch. */
int shell_fetch_is(const char* line);
int shell_fetch_line(const char* line);
/* GUI hook (osui_gui_set_web_fetch): fetches url, writes the reduced HTML
 * for the engine into src (NUL-terminated) and the final URL into final_url;
 * status gets one "osui web fetch ..." line. Returns the source length or -1. */
int shell_fetch_page(const char* url, char* src, int src_max, char* final_url, int url_max, char* status, int st_max);
#endif
