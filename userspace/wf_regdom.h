/* wf_regdom.h - Lot 88: the registrable domain of a host ("site"), for
 * autofill shared between hosts of one site (www.example.com and
 * login.example.com).
 *
 * Not the full Public Suffix List: a short hand-written list of the
 * multi-label public suffixes most often met (country second levels such
 * as co.uk or com.au, and hosting suffixes whose subdomains belong to
 * different owners such as github.io). Any other host is taken as
 * NAME.TLD. IP literals, single-label hosts (localhost) and a host that is
 * itself a public suffix have no registrable domain: they never share.
 * No heap, no table generated from outside data. */
#ifndef WF_REGDOM_H
#define WF_REGDOM_H

#define WF_REGDOM_MAX 96

/* 0 and out = registrable domain (lower case), or -1 when the host has
 * none (IP literal, one label, a public suffix, bad characters). */
int wf_regdom(const char *host, char *out, int cap);
/* 1 when host is a public suffix of the built-in list or a bare TLD. */
int wf_regdom_is_suffix(const char *host);
/* 1 when both hosts have the same registrable domain (exact hosts that
 * are equal are the same site too, even without one: 10.0.2.2). */
int wf_regdom_same_site(const char *a, const char *b);
/* 1 when host is an IPv4 literal or a bracketed / colon IPv6 literal. */
int wf_regdom_is_ip(const char *host);
#endif
