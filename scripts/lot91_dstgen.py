#!/usr/bin/env python3
"""Lot 91 test generator: autofill summer time rules (wf_autofill_dst_offset,
wf_autofill_day_rule) and the session file / status line with a rule.

Expectations come from this file, not from the C sources. The model uses
Python datetime arithmetic: a rule is a pair of transition instants per
year, found by walking the calendar (the n-th or last Sunday of a month).
When the tz database is installed, the model is checked against it at
generation time for the years the rules match the real zones
(Europe/Paris 1996.., America/New_York 2007.., Australia/Sydney 2008..);
a disagreement stops the generator.

Rules modelled:
* eu: summer time from the last Sunday of March 01:00 UTC to the last
  Sunday of October 01:00 UTC, whatever the offset;
* us: from the second Sunday of March 02:00 local standard time to the
  first Sunday of November 01:00 local standard time (02:00 summer time);
  the year is the year of the local standard time;
* au: (south) from the first Sunday of October 02:00 local standard time
  to the first Sunday of April 02:00 local standard time (03:00 summer);
  a year runs January to April in summer time, then October onwards;
* the local day is the day of UTC + offset + 60 in summer time; 0 when
  the local date is outside 1970-01-01 .. 2199-12-31 or the text, the
  offset (-720..840) or the rule is not valid; rule 0 is the fixed
  offset of Lot 90.

Writes tests/unit/userspace/test_lot91_dst.c (seed 9121) and
tests/unit/userspace/test_lot91_dststore.c (seed 9122).
"""
import datetime as dt
import os
import random

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
OUT = os.path.join(ROOT, "tests", "unit", "userspace", "test_lot91_dst.c")
OUT2 = os.path.join(ROOT, "tests", "unit", "userspace", "test_lot91_dststore.c")
EPOCH = dt.datetime(1970, 1, 1)
NAMES = {0: "off", 1: "eu", 2: "us", 3: "au"}


def nth_sunday(y, m, n):
    """date of the n-th Sunday of the month; n 9: the last one"""
    if n == 9:
        d = (dt.date(y + (m == 12), m % 12 + 1, 1) - dt.timedelta(days=1))
        while d.weekday() != 6:
            d -= dt.timedelta(days=1)
        return d
    d = dt.date(y, m, 1)
    while d.weekday() != 6:
        d += dt.timedelta(days=1)
    return d + dt.timedelta(days=7 * (n - 1))


def at(d, minutes):
    return dt.datetime(d.year, d.month, d.day) + dt.timedelta(minutes=minutes)


def transitions(rule, y):
    """start and end of summer time in year y: UTC for eu, local standard
    time for us and au"""
    if rule == 1:
        return at(nth_sunday(y, 3, 9), 60), at(nth_sunday(y, 10, 9), 60)
    if rule == 2:
        return at(nth_sunday(y, 3, 2), 120), at(nth_sunday(y, 11, 1), 60)
    return at(nth_sunday(y, 10, 1), 120), at(nth_sunday(y, 4, 1), 120)


def shift(utc, tz, rule):
    if rule == 1:
        a, b = transitions(1, utc.year)
        return 60 if a <= utc < b else 0
    if rule == 0:
        return 0
    loc = utc + dt.timedelta(minutes=tz)
    if loc < EPOCH:
        return 0
    a, b = transitions(rule, loc.year)
    if rule == 2:
        return 60 if a <= loc < b else 0
    return 60 if (loc >= a or loc < b) else 0


def day_of(utc, tz, rule):
    loc = utc + dt.timedelta(minutes=tz + shift(utc, tz, rule))
    if loc < EPOCH or loc.year > 2199:
        return 0
    return (loc.date() - EPOCH.date()).days + 1


def text(utc, sec):
    return "%04d%02d%02d%02d%02d%02d" % (utc.year, utc.month, utc.day, utc.hour, utc.minute, sec)


def check_tzdb():
    try:
        from zoneinfo import ZoneInfo
    except ImportError:
        return 0
    zones = [(1, "Europe/Paris", 60, 1996), (1, "Europe/Helsinki", 120, 1996), (1, "Europe/London", 0, 1996),
             (2, "America/New_York", -300, 2007), (2, "America/Denver", -420, 2007),
             (2, "America/Los_Angeles", -480, 2007), (3, "Australia/Sydney", 600, 2008),
             (3, "Australia/Adelaide", 570, 2008)]
    n = 0
    utc_tz = dt.timezone.utc
    for rule, name, tz, y0 in zones:
        try:
            z = ZoneInfo(name)
        except Exception:  # noqa: BLE001  (no tz database)
            return 0
        for y in range(y0, 2037):
            for a in transitions(rule, y):
                base = a if rule == 1 else a - dt.timedelta(minutes=tz)
                for dm in (-61, -1, 0, 1, 59, 61):
                    u = base + dt.timedelta(minutes=dm)
                    real = u.replace(tzinfo=utc_tz).astimezone(z).utcoffset()
                    got = tz + shift(u, tz, rule)
                    want = int(real.total_seconds() // 60)
                    if got != want:
                        raise SystemExit("model disagrees with %s at %s: %d vs %d" % (name, u, got, want))
                    n += 1
    return n


def rand_utc(rng, y0=1970, y1=2199):
    y = rng.randint(y0, y1)
    start = dt.datetime(y, 1, 1)
    span = (dt.datetime(y + 1, 1, 1) - start).days * 1440 if y < 2199 else 365 * 1440
    return start + dt.timedelta(minutes=rng.randrange(span))


TZ = {1: [-60, 0, 60, 120, 180, 330, -720, 840], 2: [-600, -540, -480, -420, -360, -300, -240, -210, 0],
      3: [480, 525, 570, 600, 630, 660, 0, -300]}


def cases():
    rng = random.Random(9121)
    out = []
    years = [1970, 1971, 2007, 2024, 2025, 2026, 2027, 2038, 2100, 2198, 2199] + \
        sorted(rng.sample(range(1972, 2198), 30))
    for rule in (1, 2, 3):
        for y in years:
            for which, a in enumerate(transitions(rule, y)):
                tz = rng.choice(TZ[rule]) if rng.random() < 0.8 else rng.randint(-720, 840)
                base = a if rule == 1 else a - dt.timedelta(minutes=tz)
                for dm in (-61, -60, -1, 0, 1, 59, 60):
                    u = base + dt.timedelta(minutes=dm)
                    if u < EPOCH or u.year > 2199:
                        continue
                    out.append((u, rng.randrange(60), tz, rule, "edge %s %d %s %+d" % (NAMES[rule], y, "ab"[which], dm)))
    for _ in range(700):
        rule = rng.randrange(4)
        tz = rng.randint(-720, 840) if rng.random() < 0.5 else rng.choice(TZ.get(rule, [0, 60, -300]))
        out.append((rand_utc(rng), rng.randrange(60), tz, rule, "random"))
    # the ends of the range: the local date leaves 1970..2199
    for u, tz in ((dt.datetime(1970, 1, 1, 0, 0), -1), (dt.datetime(1970, 1, 1, 11, 59), -720),
                  (dt.datetime(1970, 1, 1, 12, 0), -720), (dt.datetime(2199, 12, 31, 23, 59), 1),
                  (dt.datetime(2199, 12, 31, 9, 59), 840), (dt.datetime(2199, 12, 31, 10, 0), 840),
                  (dt.datetime(2199, 12, 31, 22, 59), 0)):
        for rule in range(4):
            out.append((u, 0, tz, rule, "range end"))
    return out


BAD = [("20261301000000", 0, 1), ("20260230120000", 0, 2), ("20261012240000", 0, 3), ("20261012236000", 60, 1),
       ("2026101223", 0, 1), ("", 0, 2), ("19691231235959", 0, 1), ("22000101000000", 0, 2),
       ("2026-10-12T1", 0, 3), ("20261012120000", 841, 1), ("20261012120000", -721, 2),
       ("20261012120000", 0, 4), ("20261012120000", 0, -1), ("2026101212a000", 0, 1), ("20261012 20000", 0, 3)]


def write_dst():
    rows = cases()
    tzdb = check_tzdb()
    lines = ["/* Generated by scripts/lot91_dstgen.py (seed 9121): summer time rules",
             " * against an independent datetime model%s. Do not edit by hand. */" %
             (" checked on %d tz database instants" % tzdb if tzdb else ""),
             '#include "../../framework/unity.h"', "#include <stdio.h>", "#include <string.h>",
             '#include "wf_autofill.h"', "",
             "/* the case in the message when a check fails */",
             "#define CHK(want, got, m) do { int w_ = (want), g_ = (got); \\",
             '        if (w_ != g_) printf("FAIL %s: want %d got %d\\n", (m), w_, g_); \\',
             "        TEST_ASSERT_EQUAL(w_, g_); } while (0)", "",
             "typedef struct { const char *utc; int tz, rule, off; unsigned day; const char *why; } dst_case_t;",
             "", "static const dst_case_t g_cases[] = {"]
    for u, sec, tz, rule, why in rows:
        lines.append('    {"%s", %d, %d, %d, %dU, "%s"},' % (text(u, sec), tz, rule, shift(u, tz, rule),
                                                          day_of(u, tz, rule), why))
    lines.append("};")
    lines.append("")
    lines.append("static const dst_case_t g_bad[] = {")
    for s, tz, rule in BAD:
        lines.append('    {"%s", %d, %d, -1, 0U, "bad"},' % (s, tz, rule))
    lines.append("};")
    lines += ["", "#define NCASES ((int)(sizeof(g_cases) / sizeof(g_cases[0])))",
              "#define NBAD ((int)(sizeof(g_bad) / sizeof(g_bad[0])))", ""]
    groups = [(i, min(i + 50, len(rows))) for i in range(0, len(rows), 50)]
    for k, (a, b) in enumerate(groups):
        lines += ["static void test_l91_dst_%d(void) {" % k, "    int i;", "    char m[96];",
                  "    for (i = %d; i < %d; i++) {" % (a, b), "        const dst_case_t *c = &g_cases[i];",
                  '        snprintf(m, sizeof(m), "case %d %s %s tz %d rule %d", i, c->why, c->utc, c->tz, c->rule);',
                  "        CHK(c->rule ? c->off : 0, wf_autofill_dst_offset(c->utc, c->tz, c->rule), m);",
                  "        CHK((int)c->day, (int)wf_autofill_day_rule(c->utc, c->tz, c->rule), m);",
                  "        if (!c->rule)",
                  "            CHK((int)wf_autofill_day_tz(c->utc, c->tz), (int)wf_autofill_day_rule(c->utc, c->tz, 0), m);",
                  "    }", "}", ""]
    lines += ["static void test_l91_dst_bad(void) {", "    int i;", "    char m[96];",
              "    for (i = 0; i < NBAD; i++) {",
              '        snprintf(m, sizeof(m), "bad %d %s tz %d rule %d", i, g_bad[i].utc, g_bad[i].tz, g_bad[i].rule);',
              "        CHK(-1, wf_autofill_dst_offset(g_bad[i].utc, g_bad[i].tz, g_bad[i].rule), m);",
              "        CHK(0, (int)wf_autofill_day_rule(g_bad[i].utc, g_bad[i].tz, g_bad[i].rule), m);",
              "    }", "    TEST_ASSERT_EQUAL(-1, wf_autofill_dst_offset(NULL, 0, 1));",
              "    TEST_ASSERT_EQUAL(0, (int)wf_autofill_day_rule(NULL, 0, 1));",
              "    TEST_ASSERT_EQUAL(0, (int)wf_autofill_day_rule(NULL, 0, 0));", "}", "",
              "int main(void) {", "    unity_init();"]
    for k in range(len(groups)):
        lines.append("    RUN_TEST(test_l91_dst_%d);" % k)
    lines += ["    RUN_TEST(test_l91_dst_bad);", "    unity_print_results();", "    unity_cleanup();",
              "    return unity_stats.tests_failed == 0 ? 0 : 1;", "}", ""]
    with open(OUT, "w") as f:
        f.write("\n".join(lines))
    return len(rows) + len(BAD), tzdb


def store_cases():
    """sessions: settings, a clock, values; the status suffix, the header
    and a load of the saved text, a load of hand-made headers"""
    rng = random.Random(9122)
    tests = []
    for k in range(160):
        rule = rng.randrange(4)
        tz = rng.choice([0, 60, -300, 600, 330, -720, 840, rng.randint(-720, 840)])
        expire = rng.choice([0, 0, 1, 30, 400])
        u = rand_utc(rng, 1990, 2150)
        day = day_of(u, tz, rule)
        body = []
        body.append("    wf_autofill_init(&g_a);")
        if expire:
            body.append("    TEST_ASSERT_EQUAL(0, wf_autofill_set_expire(&g_a, %dU));" % expire)
        body.append("    TEST_ASSERT_EQUAL(0, wf_autofill_set_tz(&g_a, %d));" % tz)
        name = NAMES[rule] if rng.random() < 0.8 or rule else "none"
        body.append('    TEST_ASSERT_EQUAL(%d, wf_autofill_dst_parse("%s"));' % (rule, name))
        body.append("    TEST_ASSERT_EQUAL(0, wf_autofill_set_dst(&g_a, %d));" % rule)
        body.append('    TEST_ASSERT_EQUAL_STRING("%s", wf_autofill_dst_name(%d));' % (NAMES[rule], rule))
        body.append('    TEST_ASSERT_EQUAL(%d, (int)wf_autofill_day_rule("%s", %d, g_a.dst));' % (day, text(u, 0), tz))
        body.append("    (void)wf_autofill_tick(&g_a, %dU);" % day)
        hdr = "MOHHDY-AUTOFILL 3"
        if expire:
            hdr += " expire %d" % expire
        if tz:
            hdr += " tz %d" % tz
        if rule:
            hdr += " dst %s" % NAMES[rule]
        status = "autofill status expire %d tz %d today %d touched 0 expired 0 keys 0 values 0%s\\n" % (
            expire, tz, day, " dst " + NAMES[rule] if rule else "")
        body.append("    TEST_ASSERT_TRUE(wf_autofill_status(&g_a, g_buf, (int)sizeof(g_buf)) > 0);")
        body.append('    TEST_ASSERT_EQUAL_STRING("%s", g_buf);' % status)
        body.append("    TEST_ASSERT_TRUE(wf_autofill_save(&g_a, g_buf, (int)sizeof(g_buf)) > 0);")
        body.append('    TEST_ASSERT_EQUAL(0, strncmp(g_buf, "%s\\n", %d));' % (hdr, len(hdr) + 1))
        body.append("    wf_autofill_init(&g_b);")
        body.append("    TEST_ASSERT_EQUAL(0, wf_autofill_load(&g_b, g_buf, (int)strlen(g_buf)));")
        body.append("    TEST_ASSERT_EQUAL(%d, g_b.dst);" % rule)
        body.append("    TEST_ASSERT_EQUAL(%d, g_b.tz_min);" % tz)
        tests.append(body)
    # hand-made headers: the order of settings, duplicates, unknown words
    hdrs = []
    for k in range(120):
        parts = []
        rule = rng.randrange(1, 4)
        if rng.random() < 0.6:
            parts.append(("expire", " expire %d" % rng.randint(1, 36500)))
        if rng.random() < 0.6:
            parts.append(("tz", " tz %d" % rng.randint(-720, 840)))
        parts.append(("dst", " dst " + NAMES[rule]))
        rng.shuffle(parts)
        fault = rng.choice([None, None, "dup", "word", "upper", "glued", "off", "long", "empty"])
        txt = "".join(p[1] for p in parts)
        ok = True
        if fault == "dup":
            txt += " dst " + NAMES[rng.randrange(1, 4)]
            ok = False
        elif fault == "word":
            txt = txt.replace(" dst " + NAMES[rule], " dst " + rng.choice(["xx", "e", "eux", "usa"]))
            ok = False
        elif fault == "upper":
            txt = txt.replace(" dst " + NAMES[rule], " dst " + NAMES[rule].upper())
            ok = False
        elif fault == "glued":
            txt = txt.replace(" dst " + NAMES[rule], " dst" + NAMES[rule])
            ok = False
        elif fault == "off":
            txt = txt.replace(" dst " + NAMES[rule], " dst off")
            ok = False
        elif fault == "long":
            txt = txt.replace(" dst " + NAMES[rule], " dst " + NAMES[rule] + "x")
            ok = False
        elif fault == "empty":
            txt = txt.replace(" dst " + NAMES[rule], " dst ")
            ok = False
        hdrs.append(("MOHHDY-AUTOFILL 3" + txt + "\\n", ok, rule if ok else 0))
    return tests, hdrs


def write_store():
    tests, hdrs = store_cases()
    lines = ["/* Generated by scripts/lot91_dstgen.py (seed 9122): the autofill store",
             " * with a summer time rule (status line, session file header, load).",
             " * Do not edit by hand. */",
             '#include "../../framework/unity.h"', "#include <stdio.h>", "#include <string.h>",
             '#include "wf_autofill.h"', "",
             "#define CHK(want, got, m) do { int w_ = (want), g_ = (got); \\",
             '        if (w_ != g_) printf("FAIL %s: want %d got %d\\n", (m), w_, g_); \\',
             "        TEST_ASSERT_EQUAL(w_, g_); } while (0)", "",
             "static wf_autofill_t g_a, g_b;", "static char g_buf[4096];", ""]
    for k, body in enumerate(tests):
        lines.append("static void test_l91_dststore_%d(void) {" % k)
        lines += body
        lines += ["}", ""]
    lines.append("typedef struct { const char *text; int ok, rule; } hdr_case_t;")
    lines.append("static const hdr_case_t g_hdrs[] = {")
    for t, ok, rule in hdrs:
        lines.append('    {"%s", %d, %d},' % (t, 1 if ok else 0, rule))
    lines.append("};")
    lines += ["", "static void test_l91_dststore_headers(void) {", "    int i;",
              "    for (i = 0; i < (int)(sizeof(g_hdrs) / sizeof(g_hdrs[0])); i++) {",
              "        wf_autofill_init(&g_b);",
              "        g_b.dst = 2;",
              "        CHK(g_hdrs[i].ok ? 0 : -1, wf_autofill_load(&g_b, g_hdrs[i].text, (int)strlen(g_hdrs[i].text)), g_hdrs[i].text);",
              "        CHK(g_hdrs[i].ok ? g_hdrs[i].rule : 2, g_b.dst, g_hdrs[i].text);",
              "    }", "}", "", "int main(void) {", "    unity_init();"]
    for k in range(len(tests)):
        lines.append("    RUN_TEST(test_l91_dststore_%d);" % k)
    lines += ["    RUN_TEST(test_l91_dststore_headers);", "    unity_print_results();", "    unity_cleanup();",
              "    return unity_stats.tests_failed == 0 ? 0 : 1;", "}", ""]
    with open(OUT2, "w") as f:
        f.write("\n".join(lines))
    return len(tests), len(hdrs)


if __name__ == "__main__":
    n, tzdb = write_dst()
    s, h = write_store()
    print("lot91_dstgen: %d rule cases (tz database %d), %d sessions, %d headers" % (n, tzdb, s, h))
