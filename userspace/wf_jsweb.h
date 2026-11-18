/* wf_jsweb.h - Lot 113 (US-031): the JavaScript subset in the shell and
 * the browser. One code path for the console and the GUI input line
 * (osui_gui_web_command calls wf_jsweb_command):
 *
 *   js CODE        run CODE in a persistent REPL realm (no DOM)
 *   web-js on|off  run the inline scripts of fetched HTML pages
 *   web-js status  the setting and the stats of the last page pass
 *   web-js log     the console log of the last page pass
 *   web-js reset   a fresh REPL realm
 *
 * The page pass is off by default. When on, shell_fetch.c gives every
 * fetched HTML body to wf_jsweb_page before the reducer, the sanitizer
 * and the renderer see it; those still drop every script, on* attribute
 * and javascript: URL (US-039 is unchanged). Arenas come from the user
 * heap (umalloc), allocated on first use and reused. */
#ifndef WF_JSWEB_H
#define WF_JSWEB_H

#define WF_JSWEB_REPL_ARENA (1536u * 1024u)
#define WF_JSWEB_PAGE_ARENA (4096u * 1024u)
#define WF_JSWEB_REPL_BUDGET 2000000u

/* 1 when line is a js or web-js command (out has the answer) */
int wf_jsweb_command(const char *line, char *out, int out_max);
/* the page pass on body[0..n) (max: its buffer size); returns the new
 * length, or n when the pass is off, found no inline script or failed */
int wf_jsweb_page(char *body, int n, int max, const char *url);
/* append " js scripts=N ran=N failed=N" for the last page when it ran */
void wf_jsweb_note(char *out, int out_max);
int wf_jsweb_enabled(void);
void wf_jsweb_set(int on);

#endif
