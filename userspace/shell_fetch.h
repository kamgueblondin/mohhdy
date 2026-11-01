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
/* Lot 71: the last fetched page (GUI): sections of a long page, its forms
 * (focus, typing, submit with GET or POST urlencoded). */
int shell_page_sections(void);
int shell_page_find(const char* text, int from, int* count);
int shell_page_current(void);
int shell_page_section(int k, char* src, int src_max);
int shell_page_form_link(const char* href);
int shell_page_form_key(int key);
int shell_page_form_set(int n, const char* value);
int shell_page_forms_list(char* out, int cap);
int shell_page_submit(int n, char* src, int src_max, char* final_url, int url_max, char* status, int st_max);
#endif
