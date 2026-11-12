/* wf_autofill.h - Lot 86: remembered form values, per host.
 * Passwords are refused. The value is the text the operator typed, kept
 * so the next visit of the same host and field name can fill it. Session
 * file only when the caller saves it. No heap. */
#ifndef WF_AUTOFILL_H
#define WF_AUTOFILL_H
#include <stdint.h>

#define WF_AF_MAX 32
#define WF_AF_HOST 96
#define WF_AF_NAME 48
#define WF_AF_VALUE 120
/* Lot 87: values kept per host and field name, newest first (the current
 * value and two older ones). A key still counts once in n. */
#define WF_AF_HIST 3

typedef struct {
    int used;
    char host[WF_AF_HOST];
    char name[WF_AF_NAME];
    char value[WF_AF_VALUE];
    int nold;                                  /* Lot 87: 0..WF_AF_HIST-1 */
    char old[WF_AF_HIST - 1][WF_AF_VALUE];     /* older values, newest first */
    uint32_t day;                              /* Lot 89: last stored, wf_autofill_day; 0 undated */
} wf_af_ent_t;

typedef struct {
    wf_af_ent_t e[WF_AF_MAX];
    int n;
    uint32_t stored, refused, hits;
    uint32_t today;   /* Lot 89: wf_autofill_day of the clock, 0 unknown */
} wf_autofill_t;

void wf_autofill_init(wf_autofill_t *a);
/* 1 when this HTML type must not be remembered. */
int wf_autofill_secret(const char *type);
/* 0 stored or replaced, -1 refused. */
int wf_autofill_put(wf_autofill_t *a, const char *host, const char *name, const char *value, const char *type);
/* 0 and out receives the value, -1 none. */
int wf_autofill_get(wf_autofill_t *a, const char *host, const char *name, char *out, int cap);
/* Lot 87: value idx of a key, 0 the newest. 0, or -1 when there is none. */
int wf_autofill_get_at(wf_autofill_t *a, const char *host, const char *name, int idx, char *out, int cap);
/* Lot 87: how many values a key holds (0 when unknown). */
int wf_autofill_count(const wf_autofill_t *a, const char *host, const char *name);
int wf_autofill_forget(wf_autofill_t *a, const char *host);
/* Lot 87: forget one key (host, field name) with all its values: 1 when
 * it was there, 0 when not. */
int wf_autofill_forget_key(wf_autofill_t *a, const char *host, const char *name);
/* Lot 87: forget one value of a key (exact text). The older values move
 * up; the key goes when its last value goes. Returns how many values the
 * key still holds (0 when it went), -1 when that value was not there. */
int wf_autofill_drop(wf_autofill_t *a, const char *host, const char *name, const char *value);
/* Lot 88: the values of a field name for a host and then for the other
 * hosts of its site (same registrable domain, wf_regdom.h): the host's
 * own values first, newest first, then each other host's, newest first,
 * without repeats. Value idx of that list: 0, or -1. *other (may be 0)
 * gets 1 when the value came from another host of the site. */
int wf_autofill_get_site(wf_autofill_t *a, const char *host, const char *name, int idx, char *out, int cap,
                         int *other);
/* Lot 88: how many values get_site lists (at most WF_AF_SITE_MAX). */
int wf_autofill_count_site(const wf_autofill_t *a, const char *host, const char *name);
/* Lot 88: forget one value from the host, or else from the host of the
 * site that holds it. Values left on that key, or -1. */
int wf_autofill_drop_site(wf_autofill_t *a, const char *host, const char *name, const char *value);
#define WF_AF_SITE_MAX 8

/* ---- Lot 89: dates and expiry ---------------------------------
 * A key carries the day it was last stored (typed or added). Days count
 * from 1970-01-01 = 1 (0 means no date: a key loaded from a file saved
 * before this lot, or stored while the clock was unknown). The session
 * file adds the day as a fourth tab field on each line of a dated key
 * (header "MOHHDY-AUTOFILL 2"); a file without it still loads. */
/* Day number of "YYYYMMDD..." (the SYS_RTC_UTC text); 0 when the date is
 * not a valid Gregorian date between 1970 and 2199. */
uint32_t wf_autofill_day(const char *utc);
#define WF_AF_DAY_MAX 84006U   /* 2199-12-31 */
/* The clock's day for the next stores (0 = unknown: stores stay undated). */
void wf_autofill_set_today(wf_autofill_t *a, uint32_t day);
/* Days since the key was last stored, or -1 when it has no date, the
 * clock is unknown or the key is not there. A date after today reads 0. */
int wf_autofill_age(const wf_autofill_t *a, const char *host, const char *name);
/* Forget every key last stored more than days days ago (0..36500; 0
 * forgets what is older than today). An undated key is not forgotten: it
 * gets today's date, so it expires days days from now; *undated (may be
 * 0) counts them. Returns the keys forgotten, -1 when the clock is
 * unknown or days is out of range (nothing changes then). */
int wf_autofill_forget_older(wf_autofill_t *a, uint32_t days, int *undated);
/* One line per key: "autofill age HOST NAME Nd" or "... undated", then
 * "autofill ages N dated D". -1 when out is too small. */
int wf_autofill_ages(const wf_autofill_t *a, char *out, int cap);
int wf_autofill_save(const wf_autofill_t *a, char *out, int cap);
int wf_autofill_load(wf_autofill_t *a, const char *text, int len);
int wf_autofill_text(const wf_autofill_t *a, char *out, int cap);
#endif
