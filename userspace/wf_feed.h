/* wf_feed.h - Lot 86: RSS 2.0 and Atom, the subset a reader needs.
 * Channel or feed title, then items: title, link, a short plain
 * description. No network. No heap. At most WF_FEED_ITEMS items. */
#ifndef WF_FEED_H
#define WF_FEED_H

#define WF_FEED_ITEMS 12
#define WF_FEED_TITLE 96
#define WF_FEED_LINK 180
#define WF_FEED_BLURB 160

typedef struct {
    char title[WF_FEED_TITLE];
    char link[WF_FEED_LINK];
    char blurb[WF_FEED_BLURB];
} wf_feed_item_t;

typedef struct {
    int atom;                   /* 0 rss, 1 atom */
    char title[WF_FEED_TITLE];
    int n;
    int dropped;
    wf_feed_item_t item[WF_FEED_ITEMS];
} wf_feed_t;

/* 1 if this body looks like a feed, 0 otherwise. */
int wf_feed_looks(const char *s, int n);
/* 0 parsed (even with zero items), -1 not a feed or bad args. */
int wf_feed_parse(const char *s, int n, wf_feed_t *f);
/* Plain text, one item per paragraph. */
int wf_feed_text(const wf_feed_t *f, char *out, int cap);
#endif
