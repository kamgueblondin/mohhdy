/* shell_desk.h - Lot 86: settings, bookmarks, history. One path for the
 * console and the /settings window. */
#ifndef SHELL_DESK_H
#define SHELL_DESK_H

int shell_desk_is(const char *line);
int shell_desk_line(const char *line);
/* Record a completed navigation unless web-back / web-forward is loading. */
void shell_desk_note(const char *url, const char *title);
int shell_desk_nav_held(void);
#include "desk_cfg.h"
desk_cfg_t *shell_desk_cfg(void);
/* Apply scale, net and engine to the live shell. 0 ok. */
int shell_desk_apply(void);
#include "wf_formfill.h"
/* Remember non-secret fields of a form (form < 0: all). Returns how many. */
int shell_desk_remember_form(const wf_form_page_t *p, int form, const char *url);
int shell_desk_offers(const wf_form_page_t *p, int field, const char *url, wf_fill_offer_t *o, int cap);
int shell_desk_apply_offer(wf_form_page_t *p, int field, int offer, const char *url);
/* Form editor Down key: fill the focused text field. 0 filled, -1 none. */
int shell_desk_fill_key(wf_form_page_t *p, int field, const char *url);
#endif
