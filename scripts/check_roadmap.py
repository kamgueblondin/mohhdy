#!/usr/bin/env python3
"""Lot 48 (US-120): /roadmap.txt stays a hand-kept file, but it can no
longer drift silently. Checks, without booting:
  1. every phase 9 line (US-Gxx) has the status of the US master table
     (US/mohhdy_user_stories_master.md, first word of the Statut column);
  2. US-076..US-120 are all in roadmap.txt, each in its phase (6, 7 or 8),
     and each has the status of the US master phase 6-8 table (lot 49);
  3. the prod-roadmap counts and percentages expected by
     tests/integration/test_qemu_production.py are the ones computed from
     /roadmap.txt with the guest rule (done * 100 / total, integer);
  4. the entry count line matches.
Exit 1 with the differences."""
import os
import re
import sys

ROOT = os.path.join(os.path.dirname(os.path.abspath(__file__)), "..")
STATUSES = ("done", "partial", "simule", "open")


def roadmap():
    rows = []
    for line in open(os.path.join(ROOT, "initrd_content", "roadmap.txt")):
        line = line.strip()
        if not line or line.startswith("#"):
            continue
        us, phase, status = line.split()[:3]
        rows.append((us, int(phase), status))
    return rows


def master_g():
    out = {}
    for line in open(os.path.join(ROOT, "US", "mohhdy_user_stories_master.md")):
        m = re.match(r"\| (US-G\d\d) \|[^|]*\| (\w+)", line)
        if m:
            out[m.group(1)] = m.group(2)
    return out


def master_678():
    out = {}
    for line in open(os.path.join(ROOT, "US", "mohhdy_user_stories_master.md")):
        m = re.match(r"\| (US-\d\d\d) \| ([678]) \|[^|]*\| (\w+) \|", line)
        if m:
            out[m.group(1)] = (int(m.group(2)), m.group(3))
    return out


def main():
    bad = []
    rows = roadmap()
    g = master_g()
    t678 = master_678()
    for us, phase, status in rows:
        if status not in STATUSES:
            bad.append("%s: unknown status %s" % (us, status))
        if phase == 9:
            if us not in g:
                bad.append("%s: missing from the US master table" % us)
            elif g[us] != status:
                bad.append("%s: roadmap.txt says %s, US master says %s" % (us, status, g[us]))
        elif phase in (6, 7, 8):
            if not us.startswith("US-"):
                bad.append("%s: invalid US tag format" % us)
            num = int(us.split("-")[1])
            expected_phase = 6 if 76 <= num <= 90 else 7 if 91 <= num <= 105 else 8 if 106 <= num <= 120 else 0
            if expected_phase != phase:
                bad.append("%s: phase %d does not match expected phase %d" % (us, phase, expected_phase))
            if us not in t678:
                bad.append("%s: missing from the US master phase 6-8 table" % us)
            elif t678[us] != (phase, status):
                bad.append("%s: roadmap.txt says phase %d %s, US master says phase %d %s"
                           % (us, phase, status, t678[us][0], t678[us][1]))

    # Verify presence of all US-076 through US-120 in roadmap.txt
    existing_us = {r[0] for r in rows}
    for i in range(76, 121):
        tag = "US-%03d" % i
        if tag not in existing_us:
            bad.append("%s: missing from roadmap.txt" % tag)

    for us in list(g) + list(t678):
        if us not in existing_us:
            bad.append("%s: in the US master table, not in roadmap.txt" % us)
    test = open(os.path.join(ROOT, "tests", "integration", "test_qemu_production.py")).read()
    for phase in sorted(set(r[1] for r in rows)):
        sel = [r for r in rows if r[1] == phase]
        c = {s: len([r for r in sel if r[2] == s]) for s in STATUSES}
        pct = c["done"] * 100 // len(sel)
        label = "phase 9 (gui-parity)" if phase == 9 else "phase %d" % phase
        want = "prod-roadmap %s done %d partial %d simule %d open %d progress %d%%" % (
            label, c["done"], c["partial"], c["simule"], c["open"], pct)
        if phase >= 6 and want not in test:
            bad.append("test_qemu_production.py does not expect: " + want)
    want = "prod-roadmap ok source /roadmap.txt entries %d bad 0" % len(rows)
    if want not in test:
        bad.append("test_qemu_production.py does not expect: " + want)
    for b in bad:
        print("FAIL roadmap: " + b)
    if not bad:
        print("OK roadmap.txt (%d entries): phases 6-9 statuses match the US master tables, US-076..US-120 present in their phase, prod-roadmap test counts match" % len(rows))
    return 1 if bad else 0


if __name__ == "__main__":
    sys.exit(main())
