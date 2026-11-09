/* wf_robots.c - Lot 86: see wf_robots.h. */
#include "wf_robots.h"

static char rlow(char c) { return (c >= 'A' && c <= 'Z') ? (char)(c + 32) : c; }
static int rspace(char c) { return c == ' ' || c == '\t' || c == '\r'; }

static int ieq(const char *a, const char *b) {
    int i = 0;
    while (a[i] || b[i]) {
        if (rlow(a[i]) != rlow(b[i])) return 0;
        if (!a[i]) break;
        i++;
    }
    return 1;
}

static int prefix_ieq(const char *s, const char *p) {
    int i = 0;
    while (p[i]) { if (rlow(s[i]) != p[i]) return 0; i++; }
    return 1;
}

void wf_robots_init(wf_robots_t *r) {
    int i;
    if (!r) return;
    for (i = 0; i < (int)sizeof(*r); i++) ((unsigned char *)r)[i] = 0;
    r->crawl_delay = -1;
}

/* Pattern match: '*' any run, '$' end. */
static int match_pat(const char *pat, const char *path) {
    int i = 0, j = 0;
    while (pat[i]) {
        if (pat[i] == '*') {
            int k;
            i++;
            if (!pat[i]) return 1;
            for (k = j; path[k]; k++) if (match_pat(pat + i, path + k)) return 1;
            return match_pat(pat + i, path + j);
        }
        if (pat[i] == '$') return path[j] == 0 && pat[i + 1] == 0;
        if (!path[j] || path[j] != pat[i]) return 0;
        i++; j++;
    }
    return 1;
}

static int specificity(const char *pat) {
    int n = 0;
    while (pat[n] && pat[n] != '*' && pat[n] != '$') n++;
    return n;
}

int wf_robots_parse(wf_robots_t *r, const char *text, int n, const char *agent) {
    int i = 0, care = 0, seen_star = 0, seen_agent = 0, in_group = 0, group_care = 0;
    if (!r) return -1;
    wf_robots_init(r);
    if (!text || n < 0) return -1;
    if (!agent || !agent[0]) agent = "mohhdy";
    /* Two passes: prefer the named agent, else '*'. */
    while (i < n) {
        char line[WF_ROBOTS_PATH + 32];
        int k = 0, hash;
        const char *val;
        while (i < n && text[i] != '\n' && k < (int)sizeof(line) - 1) line[k++] = text[i++];
        if (i < n && text[i] == '\n') i++;
        while (k > 0 && rspace(line[k - 1])) k--;
        line[k] = 0;
        hash = 0;
        while (line[hash] && rspace(line[hash])) hash++;
        if (!line[hash] || line[hash] == '#') {
            if (!line[hash]) { in_group = 0; }
            continue;
        }
        if (prefix_ieq(line + hash, "user-agent:")) {
            val = line + hash + 11;
            while (*val && rspace(*val)) val++;
            if (!in_group) group_care = 0;
            in_group = 1;
            if (ieq(val, "*")) { seen_star = 1; group_care = 1; }
            else if (ieq(val, agent)) { seen_agent = 1; group_care = 1; }
            else group_care = 0;
            care = group_care;
            continue;
        }
        if (!in_group) continue;
        if (prefix_ieq(line + hash, "disallow:") || prefix_ieq(line + hash, "allow:")) {
            int allow = prefix_ieq(line + hash, "allow:");
            int off = allow ? 6 : 9;
            val = line + hash + off;
            while (*val && rspace(*val)) val++;
            if (!care) continue;
            if (!val[0]) continue;          /* empty disallow means allow all */
            if (r->n >= WF_ROBOTS_RULES) { r->ignored++; continue; }
            r->rule[r->n].allow = allow;
            {
                int p = 0;
                while (val[p] && val[p] != '#' && p < WF_ROBOTS_PATH - 1) {
                    r->rule[r->n].path[p] = val[p];
                    p++;
                }
                while (p > 0 && rspace(r->rule[r->n].path[p - 1])) p--;
                r->rule[r->n].path[p] = 0;
            }
            if (r->rule[r->n].path[0]) r->n++;
        } else if (prefix_ieq(line + hash, "crawl-delay:") && care) {
            int v = 0, p = 0;
            val = line + hash + 12;
            while (*val && rspace(*val)) val++;
            while (val[p] >= '0' && val[p] <= '9' && v < 100000) { v = v * 10 + val[p] - '0'; p++; }
            if (p) r->crawl_delay = v;
        }
    }
    if (!seen_agent && seen_star) {
        /* The rules collected include both. Drop the named-agent miss:
         * we only set care for '*' or the agent, and if the agent was
         * never seen, care was only the star group. Nothing to drop. */
        (void)seen_agent;
    }
    if (seen_agent) {
        /* Re-parse keeping only the named agent, so a later '*' group
         * does not add rules. */
        wf_robots_t only;
        int j = 0;
        wf_robots_init(&only);
        i = 0; in_group = 0; care = 0;
        while (i < n) {
            char line[WF_ROBOTS_PATH + 32];
            int k = 0;
            const char *val;
            while (i < n && text[i] != '\n' && k < (int)sizeof(line) - 1) line[k++] = text[i++];
            if (i < n && text[i] == '\n') i++;
            while (k > 0 && rspace(line[k - 1])) k--;
            line[k] = 0;
            j = 0;
            while (line[j] && rspace(line[j])) j++;
            if (!line[j]) { in_group = 0; continue; }
            if (line[j] == '#') continue;
            if (prefix_ieq(line + j, "user-agent:")) {
                val = line + j + 11;
                while (*val && rspace(*val)) val++;
                if (!in_group) care = 0;
                in_group = 1;
                if (ieq(val, agent)) care = 1;
                else if (!care) care = 0;
                else if (!ieq(val, agent)) {
                    /* a new user-agent in the same group keeps care if we
                     * already matched; a blank line ended the group above */
                }
                if (!ieq(val, agent) && !ieq(val, "*")) care = 0;
                else if (ieq(val, agent)) care = 1;
                continue;
            }
            if (!care) continue;
            if (prefix_ieq(line + j, "allow:") || prefix_ieq(line + j, "disallow:")) {
                int allow = prefix_ieq(line + j, "allow:");
                val = line + j + (allow ? 6 : 9);
                while (*val && rspace(*val)) val++;
                if (!val[0] || only.n >= WF_ROBOTS_RULES) { if (val[0]) only.ignored++; continue; }
                only.rule[only.n].allow = allow;
                {
                    int p = 0;
                    while (val[p] && val[p] != '#' && p < WF_ROBOTS_PATH - 1) { only.rule[only.n].path[p] = val[p]; p++; }
                    while (p > 0 && rspace(only.rule[only.n].path[p - 1])) p--;
                    only.rule[only.n].path[p] = 0;
                }
                if (only.rule[only.n].path[0]) only.n++;
            } else if (prefix_ieq(line + j, "crawl-delay:")) {
                int v = 0, p = 0;
                val = line + j + 12;
                while (*val && rspace(*val)) val++;
                while (val[p] >= '0' && val[p] <= '9') { v = v * 10 + val[p] - '0'; p++; }
                if (p) only.crawl_delay = v;
            }
        }
        {
            unsigned char *d = (unsigned char *)r;
            const unsigned char *s = (const unsigned char *)&only;
            unsigned nbyte = (unsigned)sizeof(only);
            while (nbyte--) *d++ = *s++;
        }
    }
    return 0;
}

int wf_robots_allowed(const wf_robots_t *r, const char *path) {
    int i, best = -1, best_len = -1, allow = 1;
    if (!path || !path[0]) path = "/";
    if (!r || r->n == 0) return 1;
    for (i = 0; i < r->n; i++) {
        int len;
        if (!match_pat(r->rule[i].path, path)) continue;
        len = specificity(r->rule[i].path);
        if (len > best_len || (len == best_len && r->rule[i].allow && !allow)) {
            best_len = len;
            best = i;
            allow = r->rule[i].allow;
        }
    }
    if (best < 0) return 1;
    return allow;
}
