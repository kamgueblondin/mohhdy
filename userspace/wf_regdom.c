/* wf_regdom.c - Lot 88: see wf_regdom.h. */
#include "wf_regdom.h"
#include "wf_idna.h"

/* Multi-label public suffixes. Every entry has at least one dot; a
 * single label (com, fr, io...) is a TLD and handled by the rule. The
 * hosting suffixes matter most: a.github.io and b.github.io are two
 * owners and must not share form values. */
static const char *const k_suffix[] = {
    /* United Kingdom */
    "co.uk", "org.uk", "ac.uk", "gov.uk", "me.uk", "ltd.uk", "plc.uk", "net.uk", "sch.uk", "nhs.uk",
    /* Australia, New Zealand */
    "com.au", "net.au", "org.au", "edu.au", "gov.au", "asn.au", "id.au",
    "co.nz", "org.nz", "net.nz", "govt.nz", "ac.nz",
    /* Japan, Korea, China, Hong Kong, Taiwan, Singapore, India */
    "co.jp", "ne.jp", "or.jp", "ac.jp", "go.jp", "ed.jp",
    "co.kr", "or.kr", "ac.kr", "go.kr",
    "com.cn", "net.cn", "org.cn", "gov.cn", "edu.cn",
    "com.hk", "org.hk", "edu.hk", "gov.hk",
    "com.tw", "org.tw", "edu.tw", "gov.tw",
    "com.sg", "edu.sg", "gov.sg",
    "co.in", "net.in", "org.in", "gov.in", "ac.in",
    /* Americas */
    "com.br", "net.br", "org.br", "gov.br", "edu.br",
    "com.mx", "org.mx", "gob.mx", "edu.mx",
    "com.ar", "gob.ar", "com.co", "gov.co", "com.pe",
    /* Africa (the user's region first) */
    "co.za", "org.za", "gov.za", "ac.za",
    "co.mu", "com.mu", "org.mu", "gov.mu", "ac.mu",
    "com.ng", "gov.ng", "co.ke", "or.ke", "go.ke",
    "co.ma", "gov.ma", "com.eg", "gov.eg",
    "com.cm", "gov.cm", "co.cm", "co.tz", "co.ug",
    /* Europe */
    "com.tr", "gov.tr", "com.pl", "co.il", "org.il", "ac.il", "gov.il",
    "com.ru", "com.ua", "gv.at", "co.at", "or.at", "ac.be",
    /* hosting: one subdomain per owner */
    "github.io", "gitlab.io", "blogspot.com", "herokuapp.com", "appspot.com",
    "netlify.app", "vercel.app", "pages.dev", "workers.dev", "web.app",
    "firebaseapp.com", "azurewebsites.net", "cloudfront.net", "s3.amazonaws.com",
    "glitch.me", "neocities.org", "wordpress.com", "tumblr.com", "wixsite.com",
    "ngrok.io", "fly.dev", "onrender.com", "readthedocs.io",
    0
};

static char low(char c) { return (c >= 'A' && c <= 'Z') ? (char)(c + 32) : c; }

static int slen(const char *s) { int n = 0; while (s && s[n]) n++; return n; }

/* equal, ASCII case-insensitive, over n bytes of a and all of b */
static int eqn(const char *a, int n, const char *b) {
    int i;
    for (i = 0; i < n; i++) if (!b[i] || low(a[i]) != low(b[i])) return 0;
    return b[n] == 0;
}

int wf_regdom_is_ip(const char *h) {
    int i, dots = 0, digits = 0, colon = 0;
    if (!h || !h[0]) return 0;
    if (h[0] == '[') return 1;
    for (i = 0; h[i]; i++) {
        if (h[i] == ':') colon = 1;
        else if (h[i] == '.') { if (!digits) return 0; dots++; digits = 0; }
        else if (h[i] >= '0' && h[i] <= '9') { if (++digits > 3) return 0; }
        else if (!colon) return 0;
        else if (!((h[i] >= 'a' && h[i] <= 'f') || (h[i] >= 'A' && h[i] <= 'F'))) return 0;
    }
    if (colon) return 1;
    return dots == 3 && digits > 0;
}

/* host characters: letters, digits, '-', '.', no empty label */
static int host_ok(const char *h) {
    int i, label = 0;
    if (!h || !h[0] || slen(h) >= WF_REGDOM_MAX) return 0;
    for (i = 0; h[i]; i++) {
        char c = low(h[i]);
        if (c == '.') { if (!label) return 0; label = 0; continue; }
        if (!((c >= 'a' && c <= 'z') || (c >= '0' && c <= '9') || c == '-')) return 0;
        label++;
    }
    return label > 0;
}

static int listed(const char *s, int n) {
    int k;
    for (k = 0; k_suffix[k]; k++) if (eqn(s, n, k_suffix[k])) return 1;
    return 0;
}

int wf_regdom_is_suffix(const char *h) {
    int i;
    if (!h || !h[0]) return 0;
    for (i = 0; h[i]; i++) if (h[i] == '.') break;
    if (!h[i]) return 1;                  /* one label: a TLD */
    return listed(h, slen(h));
}

int wf_regdom(const char *host, char *out, int cap) {
    int n, i, dots = 0, start, k;
    int at[WF_REGDOM_MAX];                /* label starts */
    char mapped[WF_REGDOM_MAX];
    if (!out || cap <= 0) return -1;
    out[0] = 0;
    /* Lot 89: a UTF-8 or upper-case host is compared in its ASCII form
     * (bucher.example with an umlaut is xn--bcher-kva.example) */
    if (host && wf_idna_needs_map(host)) {
        if (wf_idna_to_ascii(host, mapped, (int)sizeof(mapped)) != 0) return -1;
        host = mapped;
    }
    if (!host_ok(host) || wf_regdom_is_ip(host)) return -1;
    n = slen(host);
    if (host[n - 1] == '.') return -1;    /* a trailing dot: not handled */
    at[0] = 0;
    for (i = 0; i < n; i++) if (host[i] == '.') at[++dots] = i + 1;
    if (dots == 0) return -1;             /* localhost, intranet names */
    if (listed(host, n)) return -1;       /* the host is a public suffix */
    /* the longest listed suffix that leaves at least one label before it */
    start = -1;
    for (k = 1; k <= dots; k++) {
        if (listed(host + at[k], n - at[k])) { start = k - 1; break; }
    }
    if (start < 0) start = dots - 1;      /* NAME.TLD */
    if (n - at[start] >= cap) return -1;
    for (i = at[start], k = 0; i < n; i++) out[k++] = low(host[i]);
    out[k] = 0;
    return 0;
}

int wf_regdom_same_site(const char *a, const char *b) {
    char ra[WF_REGDOM_MAX], rb[WF_REGDOM_MAX];
    int i;
    char ma[WF_REGDOM_MAX], mb[WF_REGDOM_MAX];
    if (!a || !b || !a[0] || !b[0]) return 0;
    if (slen(a) == slen(b) && eqn(a, slen(a), b)) return 1;
    /* Lot 89: the same host written in UTF-8 and in its ASCII form */
    if ((wf_idna_needs_map(a) || wf_idna_needs_map(b)) && wf_idna_to_ascii(a, ma, (int)sizeof(ma)) == 0 &&
        wf_idna_to_ascii(b, mb, (int)sizeof(mb)) == 0 && slen(ma) == slen(mb) && eqn(ma, slen(ma), mb)) return 1;
    if (wf_regdom(a, ra, (int)sizeof(ra)) != 0 || wf_regdom(b, rb, (int)sizeof(rb)) != 0) return 0;
    for (i = 0; ra[i] || rb[i]; i++) if (ra[i] != rb[i]) return 0;
    return 1;
}
