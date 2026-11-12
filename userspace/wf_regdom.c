/* wf_regdom.c - Lot 88: see wf_regdom.h. */
#include "wf_regdom.h"
#include "wf_idna.h"

/* Multi-label public suffixes. Every entry has at least one dot; a
 * single label (com, fr, io...) is a TLD and handled by the rule. The
 * hosting suffixes matter most: a.github.io and b.github.io are two
 * owners and must not share form values.
 * Lot 90: the Public Suffix List rule forms: "*.x" (every label under x
 * is a public suffix) and "!y.x" (an exception: y.x is registrable even
 * though "*.x" covers it). Entries are copied from the PSL (ICANN and
 * private sections), well-known ones only; the list is not generated
 * from outside data at build time. */
static const char *const k_suffix[] = {
    /* United Kingdom (a school is one more level: *.sch.uk) */
    "co.uk", "org.uk", "ac.uk", "gov.uk", "me.uk", "ltd.uk", "plc.uk", "net.uk", "sch.uk", "*.sch.uk",
    "nhs.uk", "police.uk",
    /* Australia, New Zealand */
    "com.au", "net.au", "org.au", "edu.au", "gov.au", "asn.au", "id.au",
    "co.nz", "org.nz", "net.nz", "govt.nz", "ac.nz", "school.nz", "geek.nz", "gen.nz", "kiwi.nz",
    /* Japan (city wards: *.CITY.jp with the city hall as an exception),
     * Korea, China, Hong Kong, Taiwan, Singapore, India */
    "co.jp", "ne.jp", "or.jp", "ac.jp", "go.jp", "ed.jp", "ad.jp", "gr.jp", "lg.jp",
    "*.kawasaki.jp", "*.kitakyushu.jp", "*.kobe.jp", "*.nagoya.jp", "*.sapporo.jp", "*.sendai.jp",
    "*.yokohama.jp",
    "!city.kawasaki.jp", "!city.kitakyushu.jp", "!city.kobe.jp", "!city.nagoya.jp", "!city.sapporo.jp",
    "!city.sendai.jp", "!city.yokohama.jp",
    "co.kr", "or.kr", "ac.kr", "go.kr", "ne.kr", "re.kr", "pe.kr",
    "com.cn", "net.cn", "org.cn", "gov.cn", "edu.cn", "ac.cn", "mil.cn",
    "com.hk", "org.hk", "edu.hk", "gov.hk", "net.hk", "idv.hk",
    "com.tw", "org.tw", "edu.tw", "gov.tw", "net.tw", "idv.tw",
    "com.sg", "edu.sg", "gov.sg", "net.sg", "org.sg",
    "co.in", "net.in", "org.in", "gov.in", "ac.in", "firm.in", "gen.in", "ind.in", "edu.in", "res.in", "mil.in",
    "com.pk", "edu.pk", "gov.pk", "org.pk", "net.pk",
    "co.id", "or.id", "ac.id", "go.id", "web.id", "my.id",
    "com.my", "edu.my", "gov.my", "org.my", "net.my",
    "co.th", "ac.th", "go.th", "in.th", "or.th",
    "com.vn", "edu.vn", "gov.vn", "net.vn", "org.vn",
    "com.ph", "edu.ph", "gov.ph", "net.ph", "org.ph",
    /* wildcard country second levels */
    "*.ck", "!www.ck", "*.bd", "*.er", "*.fk", "*.mm", "*.np", "*.pg",
    /* Americas */
    "com.br", "net.br", "org.br", "gov.br", "edu.br", "mil.br", "art.br", "ind.br", "inf.br", "eng.br",
    "adv.br", "blog.br",
    "com.mx", "org.mx", "gob.mx", "edu.mx", "net.mx",
    "com.ar", "gob.ar", "org.ar", "net.ar", "edu.ar",
    "com.co", "gov.co", "org.co", "edu.co", "net.co",
    "com.pe", "gob.pe", "org.pe", "edu.pe",
    "com.ve", "gob.ve", "com.uy", "edu.uy", "com.ec", "gob.ec", "com.bo", "gob.bo", "com.py",
    "ab.ca", "bc.ca", "mb.ca", "nb.ca", "nf.ca", "nl.ca", "ns.ca", "nt.ca", "nu.ca", "on.ca", "pe.ca",
    "qc.ca", "sk.ca", "yk.ca", "gc.ca",
    "ca.us", "ny.us", "tx.us", "fl.us", "wa.us", "dc.us", "ma.us", "il.us",
    /* Africa (the user's region first) */
    "co.za", "org.za", "gov.za", "ac.za", "net.za", "web.za", "edu.za",
    "co.mu", "com.mu", "org.mu", "gov.mu", "ac.mu", "net.mu", "or.mu",
    "com.ng", "gov.ng", "edu.ng", "org.ng", "net.ng", "sch.ng", "mil.ng",
    "co.ke", "or.ke", "go.ke", "ac.ke", "ne.ke", "sc.ke", "me.ke",
    "co.ma", "gov.ma", "ac.ma", "net.ma", "org.ma", "com.eg", "gov.eg", "edu.eg", "org.eg",
    "com.cm", "gov.cm", "co.cm", "net.cm", "co.tz", "or.tz", "ac.tz", "go.tz", "co.ug", "or.ug", "ac.ug",
    "go.ug", "com.gh", "edu.gh", "gov.gh", "org.gh", "co.zw", "ac.zw", "co.zm", "co.bw", "com.et",
    "gov.et", "co.rw", "com.tn", "gov.tn", "com.dz", "gov.dz", "co.mz", "com.na", "co.na", "com.sn",
    "com.ci", "co.ci",
    /* Europe */
    "com.tr", "gov.tr", "edu.tr", "org.tr", "net.tr", "com.pl", "net.pl", "org.pl", "gov.pl", "edu.pl",
    "co.il", "org.il", "ac.il", "gov.il", "net.il", "muni.il",
    "com.ru", "com.ua", "gv.at", "co.at", "or.at", "ac.at", "ac.be",
    "gouv.fr", "asso.fr", "nom.fr", "com.fr", "tm.fr", "prd.fr",
    "com.es", "nom.es", "org.es", "gob.es", "edu.es",
    "gov.it", "edu.it", "com.gr", "edu.gr", "gov.gr", "org.gr", "com.pt", "gov.pt", "org.pt", "edu.pt",
    "co.hu", "co.rs", "edu.rs", "gov.rs", "com.cy", "gov.cy", "com.mt", "gov.mt",
    /* hosting: one subdomain per owner */
    "github.io", "gitlab.io", "blogspot.com", "herokuapp.com", "appspot.com",
    "netlify.app", "vercel.app", "pages.dev", "workers.dev", "web.app",
    "firebaseapp.com", "azurewebsites.net", "cloudfront.net", "s3.amazonaws.com",
    "glitch.me", "neocities.org", "wordpress.com", "tumblr.com", "wixsite.com",
    "ngrok.io", "fly.dev", "onrender.com", "readthedocs.io",
    "githubusercontent.com", "surge.sh", "now.sh", "myshopify.com", "dyndns.org", "duckdns.org",
    "ddns.net", "codeberg.page", "pythonanywhere.com", "r2.dev", "deno.dev", "translate.goog",
    "webflow.io", "carrd.co", "streamlit.app", "hf.space", "elasticbeanstalk.com", "bitbucket.io",
    "repl.co", "gitbook.io",
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

/* exact rule for s[0..n) */
static int listed(const char *s, int n) {
    int k;
    for (k = 0; k_suffix[k]; k++) if (k_suffix[k][0] != '*' && k_suffix[k][0] != '!' && eqn(s, n, k_suffix[k])) return 1;
    return 0;
}

/* Lot 90: "*.REST" where s[0..n) is one label then ".REST" */
static int wild(const char *s, int n) {
    int i, k;
    for (i = 0; i < n && s[i] != '.'; i++) {}
    if (i == 0 || i >= n - 1) return 0;
    for (k = 0; k_suffix[k]; k++)
        if (k_suffix[k][0] == '*' && k_suffix[k][1] == '.' && eqn(s + i + 1, n - i - 1, k_suffix[k] + 2)) return 1;
    return 0;
}

/* Lot 90: "!s" */
static int excepted(const char *s, int n) {
    int k;
    for (k = 0; k_suffix[k]; k++) if (k_suffix[k][0] == '!' && eqn(s, n, k_suffix[k] + 1)) return 1;
    return 0;
}

/* the public suffix of host (labels at[], dots of them): the label index
 * where it starts (PSL algorithm: an exception rule wins and its first
 * label is not part of the suffix; else the matching rule with the most
 * labels, exact or wildcard; else the TLD) */
static int suffix_start(const char *host, int n, const int *at, int dots) {
    int k;
    for (k = 0; k <= dots; k++) if (excepted(host + at[k], n - at[k])) return k + 1 <= dots ? k + 1 : dots;
    for (k = 0; k <= dots; k++)
        if (listed(host + at[k], n - at[k]) || wild(host + at[k], n - at[k])) return k;
    return dots;
}

int wf_regdom_is_suffix(const char *h) {
    int i, n, dots = 0;
    int at[WF_REGDOM_MAX];
    if (!h || !h[0]) return 0;
    n = slen(h);
    if (n >= WF_REGDOM_MAX) return 0;
    at[0] = 0;
    for (i = 0; i < n; i++) if (h[i] == '.') { if (dots + 1 >= WF_REGDOM_MAX) return 0; at[++dots] = i + 1; }
    if (!dots) return 1;                  /* one label: a TLD */
    return suffix_start(h, n, at, dots) == 0;
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
    /* the public suffix, then one more label before it */
    k = suffix_start(host, n, at, dots);
    if (k == 0) return -1;                /* the host is a public suffix */
    start = k - 1;
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
