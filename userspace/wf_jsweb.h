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
 * Lot 114: events after load reach the scripts of the open page through
 * one path, shell_page_js in shell_fetch.c (the console words below and
 * the GUI, whose clicks on "?__wfjs=N" links and keys become the same
 * lines):
 *
 *   web-js click TARGET        TARGET: @N, a CSS selector or a ?__wfjs=N URL
 *   web-js key KEY [TARGET]    KEY: a character, Enter, Escape, Tab,
 *                              Backspace, Delete, Arrow*, Home, End, Space
 *   web-js input TARGET TEXT   the value, then input and change
 *   web-js focus|blur|submit TARGET
 *   web-js event TYPE TARGET   a plain bubbling event
 *   web-js clicks              the clickable elements of the page
 *
 * The page pass is off by default. When on, shell_fetch.c gives every
 * fetched HTML body to wf_jsweb_page before the reducer, the sanitizer
 * and the renderer see it; those still drop every script, on* attribute
 * and javascript: URL (US-039 is unchanged). Arenas come from the user
 * heap (umalloc), allocated on first use and reused. */
#ifndef WF_JSWEB_H
#define WF_JSWEB_H

#define WF_JSWEB_REPL_ARENA (3u * 1024u * 1024u)    /* Lot 114: room for coroutine stacks */
#define WF_JSWEB_PAGE_ARENA (6u * 1024u * 1024u)
#define WF_JSWEB_REPL_BUDGET 2000000u

/* 1 when line is a js or web-js command (out has the answer) */
int wf_jsweb_command(const char *line, char *out, int out_max);
/* the page pass on body[0..n) (max: its buffer size); returns the new
 * length, or n when the pass is off, found no inline script or failed */
int wf_jsweb_page(char *body, int n, int max, const char *url);
/* append " js scripts=N ran=N failed=N" for the last page when it ran */
void wf_jsweb_note(char *out, int out_max);
int wf_jsweb_enabled(void);
/* Lot 114: the page pass after load. wf_jsweb_event writes the page
 * again into body (max bytes) and returns its length, or -1 no live
 * page, -2 no such target, -3 arena full. */
int wf_jsweb_event(const char *type, const char *target, const char *arg, char *body, int max);
int wf_jsweb_clickables(char *out, int out_max);
/* 1 when the live page listens to keys on window, document, the body or
 * the focused element (the GUI then gives it the keys the page view does
 * not use) */
int wf_jsweb_wants_keys(void);
/* the stats line of the last pass or event */
void wf_jsweb_event_note(char *out, int out_max);
void wf_jsweb_set(int on);

#endif
