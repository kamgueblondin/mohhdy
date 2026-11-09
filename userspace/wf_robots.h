/* wf_robots.h - Lot 86: a robots.txt allow/disallow check (RFC 9309
 * subset). One product token ("mohhdy", or "*"), path prefix rules,
 * '*' and '$' in the path, the longest match wins, Allow beats Disallow
 * on a tie. Crawl-delay is reported and not enforced. No heap. */
#ifndef WF_ROBOTS_H
#define WF_ROBOTS_H

#define WF_ROBOTS_RULES 48
#define WF_ROBOTS_PATH 96

typedef struct {
    int allow;                  /* 1 allow, 0 disallow */
    char path[WF_ROBOTS_PATH];
} wf_robots_rule_t;

typedef struct {
    int n;
    int crawl_delay;            /* seconds, -1 if absent */
    int ignored;                /* rules past the cap */
    wf_robots_rule_t rule[WF_ROBOTS_RULES];
} wf_robots_t;

void wf_robots_init(wf_robots_t *r);
/* agent is the product token, compared case-insensitively. A group for
 * that token is used; otherwise the '*' group; if neither exists every
 * path is allowed. */
int wf_robots_parse(wf_robots_t *r, const char *text, int n, const char *agent);
/* 1 allowed, 0 disallowed. Empty path is "/". */
int wf_robots_allowed(const wf_robots_t *r, const char *path);
#endif
