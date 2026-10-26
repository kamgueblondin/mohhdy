#!/usr/bin/env python3
"""Lot 47 (US-120): /roadmap.txt stays a hand-kept file, but it can no
longer drift silently. Checks, without booting:
  1. every phase 9 line (US-Gxx) has the status of the US master table
     (US/mohhdy_user_stories_master.md, first word of the Statut column);
  2. the prod-roadmap counts and percentages expected by
     tests/integration/test_qemu_production.py are the ones computed from
     /roadmap.txt with the guest rule (done * 100 / total, integer);
  3. the entry count line matches.
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


def main():
    bad = []
    rows = roadmap()
    g = master_g()
    for us, phase, status in rows:
        if status not in STATUSES:
            bad.append("%s: unknown status %s" % (us, status))
        if phase == 9:
            if us not in g:
                bad.append("%s: missing from the US master table" % us)
            elif g[us] != status:
                bad.append("%s: roadmap.txt says %s, US master says %s" % (us, status, g[us]))
    for us in g:
        if us not in [r[0] for r in rows]:
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
        print("OK roadmap.txt (%d entries) matches the US master phase 9 table and the prod-roadmap test" % len(rows))
    return 1 if bad else 0


if __name__ == "__main__":
    sys.exit(main())
