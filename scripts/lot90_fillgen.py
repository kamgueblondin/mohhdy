#!/usr/bin/env python3
"""Lot 90: generated suite for autofill dates per value, accepted fills
(touch), per-value expiry, the automatic expiry and the version 3 session
file, against an independent model of the store (a key is a list of
(value, day) pairs, newest first, at most three; slots are taken lowest
first, as a fixed table).

Writes tests/unit/userspace/test_lot90_fillgen.c. Deterministic (seed 9005).
"""
import os
import random

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
OUT = os.path.join(ROOT, "tests", "unit", "userspace", "test_lot90_fillgen.c")
HIST = 3
SLOTS = 32
HOSTS = ["a.ex.am", "b.ex.am", "c.org"]
NAMES = ["q", "u"]
VALUES = ["v%d" % i for i in range(6)]


class Store:
    def __init__(self):
        self.slots = [None] * SLOTS   # (host, name, [[value, day], ...])
        self.today = 0
        self.expire = 0
        self.ran = 0
        self.tz = 0
        self.expired = 0
        self.touched = 0

    def find(self, host, name):
        for i, s in enumerate(self.slots):
            if s and s[0] == host and s[1] == name:
                return i
        return -1

    def push(self, vals, value, day):
        if vals[0][0] == value:
            if day:
                vals[0][1] = day
            return
        keep = 0
        olds = vals[1:]
        at = None
        for k, (v, d) in enumerate(olds):
            if v == value:
                at = k
                keep = d
        if at is not None:
            del olds[at]
        elif len(olds) == HIST - 1:
            olds.pop()
        new = [[value, day if day else keep]] + [vals[0]] + olds
        vals[:] = new

    def put(self, host, name, value):
        i = self.find(host, name)
        if i >= 0:
            self.push(self.slots[i][2], value, self.today)
            return 0
        for k in range(SLOTS):
            if self.slots[k] is None:
                self.slots[k] = (host, name, [[value, self.today]])
                return 0
        return -1

    def touch(self, host, name, value):
        i = self.find(host, name)
        if i < 0 or value not in [v for v, _ in self.slots[i][2]]:
            return -1
        self.push(self.slots[i][2], value, self.today)
        self.touched += 1
        return 0

    def drop(self, host, name, value):
        i = self.find(host, name)
        if i < 0:
            return -1
        vals = self.slots[i][2]
        idx = [v for v, _ in vals]
        if value not in idx:
            return -1
        if len(vals) == 1:
            self.slots[i] = None
            return 0
        del vals[idx.index(value)]
        return len(vals)

    def forget(self, days):
        if not self.today or days > 36500:
            return (-1, 0, 0)
        keys = und = gone = 0
        for i, s in enumerate(self.slots):
            if not s:
                continue
            vals = s[2]
            had = False
            for p in vals:
                if not p[1]:
                    p[1] = self.today
                    had = True
            if had:
                und += 1
            keep = [p for p in vals if not (p[1] < self.today and self.today - p[1] > days)]
            gone += len(vals) - len(keep)
            if not keep:
                self.slots[i] = None
                keys += 1
            else:
                vals[:] = keep
        return (keys, und, gone)

    def tick(self, day):
        self.today = day
        if not day:
            return -1
        if not self.expire or self.ran == day:
            return 0
        _, _, v = self.forget(self.expire)
        self.ran = day
        self.expired += v
        return v

    def age(self, day):
        if not day or not self.today:
            return -1
        return 0 if day >= self.today else self.today - day

    def save(self):
        h = "MOHHDY-AUTOFILL 3"
        if self.expire:
            h += " expire %d" % self.expire
        if self.tz:
            h += " tz %d" % self.tz
        out = [h + "\n"]
        for s in self.slots:
            if not s:
                continue
            for v, d in reversed(s[2]):
                out.append("%s\t%s\t%s%s\n" % (s[0], s[1], v, ("\t%d" % d) if d else ""))
        return "".join(out)


def cstr(s):
    return '"' + s.replace("\\", "\\\\").replace("\t", "\\t").replace("\n", "\\n").replace('"', '\\"') + '"'


def check_all(o, st, var="g_a"):
    for host in HOSTS:
        for name in NAMES:
            i = st.find(host, name)
            n = len(st.slots[i][2]) if i >= 0 else 0
            o.append("    TEST_ASSERT_EQUAL(%d, wf_autofill_count(&%s, %s, %s));" % (n, var, cstr(host), cstr(name)))
            if i < 0:
                continue
            for k, (v, d) in enumerate(st.slots[i][2]):
                o.append("    TEST_ASSERT_EQUAL_STRING(%s, at(&%s, %s, %s, %d));" % (cstr(v), var, cstr(host), cstr(name), k))
                o.append("    TEST_ASSERT_EQUAL(%d, wf_autofill_value_age(&%s, %s, %s, %d));" %
                         (st.age(d), var, cstr(host), cstr(name), k))


def gen_case(rng, idx, o):
    st = Store()
    o.append("static void test_l90_fill_%d(void) {" % idx)
    o.append("    int r, und = 0, vals = 0;")
    o.append("    (void)und;")
    o.append("    (void)vals;")
    o.append("    wf_autofill_init(&g_a);")
    day = rng.randint(1000, 30000)
    for step in range(rng.randint(8, 18)):
        op = rng.random()
        host, name, value = rng.choice(HOSTS), rng.choice(NAMES), rng.choice(VALUES[:4])
        if op < 0.12:
            d = 0 if rng.random() < 0.15 else day + rng.randint(0, 40)
            day = max(day, d)
            st.today = d
            o.append("    wf_autofill_set_today(&g_a, %dU);" % d)
        elif op < 0.45:
            r = st.put(host, name, value)
            o.append("    TEST_ASSERT_EQUAL(%d, wf_autofill_put(&g_a, %s, %s, %s, \"text\"));" %
                     (r, cstr(host), cstr(name), cstr(value)))
        elif op < 0.6:
            r = st.touch(host, name, value)
            o.append("    TEST_ASSERT_EQUAL(%d, wf_autofill_touch(&g_a, %s, %s, %s));" %
                     (r, cstr(host), cstr(name), cstr(value)))
        elif op < 0.7:
            r = st.drop(host, name, value)
            o.append("    TEST_ASSERT_EQUAL(%d, wf_autofill_drop(&g_a, %s, %s, %s));" %
                     (r, cstr(host), cstr(name), cstr(value)))
        elif op < 0.8:
            days = rng.choice([0, 1, 5, 10, 20, 30, 36501])
            keys, und, gone = st.forget(days)
            o.append("    r = wf_autofill_forget_older_values(&g_a, %dU, &und, &vals);" % days)
            o.append("    TEST_ASSERT_EQUAL(%d, r);" % keys)
            if keys >= 0:
                o.append("    TEST_ASSERT_EQUAL(%d, und);" % und)
                o.append("    TEST_ASSERT_EQUAL(%d, vals);" % gone)
        elif op < 0.87:
            days = rng.choice([0, 3, 10, 25])
            st.expire = days
            st.ran = 0
            o.append("    TEST_ASSERT_EQUAL(0, wf_autofill_set_expire(&g_a, %dU));" % days)
        elif op < 0.95:
            d = 0 if rng.random() < 0.1 else day + rng.randint(0, 15)
            day = max(day, d)
            r = st.tick(d)
            o.append("    TEST_ASSERT_EQUAL(%d, wf_autofill_tick(&g_a, %dU));" % (r, d))
        else:
            tz = rng.choice([0, 60, -300, 330, 840, -720])
            st.tz = tz
            o.append("    TEST_ASSERT_EQUAL(0, wf_autofill_set_tz(&g_a, %d));" % tz)
    check_all(o, st)
    o.append("    TEST_ASSERT_EQUAL(%dU, g_a.touched);" % st.touched)
    o.append("    TEST_ASSERT_EQUAL(%dU, g_a.expired);" % st.expired)
    text = st.save()
    o.append("    r = wf_autofill_save(&g_a, g_buf, (int)sizeof(g_buf));")
    o.append("    TEST_ASSERT_EQUAL(%d, r);" % len(text))
    o.append("    TEST_ASSERT_EQUAL_STRING(%s, g_buf);" % cstr(text))
    # load into a store with the same clock: same values and ages
    o.append("    wf_autofill_init(&g_b);")
    o.append("    wf_autofill_set_today(&g_b, %dU);" % st.today)
    o.append("    TEST_ASSERT_EQUAL(0, wf_autofill_load(&g_b, g_buf, r));")
    o.append("    TEST_ASSERT_EQUAL(%dU, g_b.expire_days);" % st.expire)
    o.append("    TEST_ASSERT_EQUAL(%d, g_b.tz_min);" % st.tz)
    check_all(o, st, "g_b")
    o.append("}")
    o.append("")


def main():
    rng = random.Random(9005)
    o = ["/* Generated by scripts/lot90_fillgen.py (seed 9005): autofill dates per",
         " * value, accepted fills, per-value and automatic expiry, version 3",
         " * files, against an independent model. Do not edit by hand. */",
         '#include "../../framework/unity.h"',
         "#include <string.h>",
         '#include "wf_autofill.h"',
         "",
         "static wf_autofill_t g_a, g_b;",
         "static char g_buf[4096];",
         "static char g_v[WF_AF_VALUE];",
         "",
         "static const char *at(wf_autofill_t *a, const char *h, const char *n, int k) {",
         "    if (wf_autofill_get_at(a, h, n, k, g_v, (int)sizeof(g_v)) != 0) return \"NONE\";",
         "    return g_v;",
         "}",
         ""]
    n = 250
    for i in range(n):
        gen_case(rng, i, o)
    o.append("int main(void) {")
    o.append("    unity_init();")
    for i in range(n):
        o.append("    RUN_TEST(test_l90_fill_%d);" % i)
    o += ["    unity_print_results();", "    unity_cleanup();",
          "    return unity_stats.tests_failed == 0 ? 0 : 1;", "}", ""]
    with open(OUT, "w") as f:
        f.write("\n".join(o))
    print("fill %d" % n)


if __name__ == "__main__":
    main()
