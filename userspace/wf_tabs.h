/* wf_tabs.h - Lot 86: a bounded tab strip for the guest browser.
 * Eight tabs. Each keeps an absolute URL, a title and a scroll row.
 * Opening a URL that is already open selects that tab. No heap. */
#ifndef WF_TABS_H
#define WF_TABS_H
#include "wf_nav.h"

#define WF_TABS_MAX 8

typedef struct {
    int used;
    int scroll;
    char url[WF_NAV_URL];
    char title[WF_NAV_TITLE];
} wf_tab_t;

typedef struct {
    wf_tab_t tab[WF_TABS_MAX];
    int n;
    int current;                /* -1 when empty */
    uint32_t opened, closed, focused;
} wf_tabs_t;

void wf_tabs_init(wf_tabs_t *t);
/* 0 and *index is the tab, -1 refused (bad URL or the strip is full
 * and the URL is new). */
int wf_tabs_open(wf_tabs_t *t, const char *url, const char *title, int *index);
int wf_tabs_close(wf_tabs_t *t, int index);
int wf_tabs_select(wf_tabs_t *t, int index);
int wf_tabs_next(wf_tabs_t *t, int dir);
int wf_tabs_set_title(wf_tabs_t *t, int index, const char *title);
int wf_tabs_set_scroll(wf_tabs_t *t, int index, int row);
const wf_tab_t *wf_tabs_at(const wf_tabs_t *t, int index);
int wf_tabs_find(const wf_tabs_t *t, const char *url);
int wf_tabs_save(const wf_tabs_t *t, char *out, int cap);
int wf_tabs_load(wf_tabs_t *t, const char *text, int len);
int wf_tabs_text(const wf_tabs_t *t, char *out, int cap);
#endif
