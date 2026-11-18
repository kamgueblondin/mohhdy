#!/usr/bin/env python3
"""Line coverage of the host unit tests, measured by gcov.

Each .gcda next to a test binary under build/ is one instrumented copy.
The same source compiled into several binaries is merged: a line counts as
executed when any copy executed it. Sources that no host test compiles are
not in the denominator. This is not guest-image coverage.
"""
from __future__ import print_function

import concurrent.futures
import os
import re
import subprocess
import sys
import tempfile

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
BUILD = os.path.join(ROOT, "build")
LINE_RE = re.compile(r"^\s*(-|#{5}|\d+):\s*(\d+):")


def gcda_files():
    found = []
    for dirpath, _dirs, files in os.walk(BUILD):
        for name in files:
            if name.endswith(".gcda"):
                found.append(os.path.join(dirpath, name))
    return found


def read_one(path):
    """gcov one .gcda in a private directory. Returns a list of
    (source, instrumented lines, executed lines), False when gcov could not
    read it, None when gcov did not start."""
    directory = os.path.dirname(path)
    name = os.path.basename(path)
    out = []
    with tempfile.TemporaryDirectory(prefix="gcov-") as tmp:
        try:
            proc = subprocess.run(
                ["gcov", "-o", directory, name],
                cwd=tmp, capture_output=True, text=True, check=False)
        except OSError:
            return None
        if proc.returncode != 0 and "Lines executed" not in proc.stdout:
            return False
        produced_any = False
        for produced in sorted(os.listdir(tmp)):
            if not produced.endswith(".gcov"):
                continue
            produced_any = True
            src = None
            lines = set()
            hits = set()
            with open(os.path.join(tmp, produced), "r", errors="replace") as handle:
                for raw in handle:
                    if raw.startswith("        -:    0:Source:"):
                        src = raw.split("Source:", 1)[1].strip()
                        continue
                    match = LINE_RE.match(raw)
                    if not match or src is None:
                        continue
                    kind, lineno_s = match.group(1), match.group(2)
                    if kind == "-":
                        continue
                    lineno = int(lineno_s)
                    lines.add(lineno)
                    if kind != "#####":
                        hits.add(lineno)
            if src is not None:
                out.append((src, lines, hits))
        if not produced_any:
            return False
    return out


def main():
    notes = gcda_files()
    if not notes:
        print("Coverage: not measured. No .gcda under build/. "
              "CFLAGS did not pass -fprofile-arcs -ftest-coverage, or the tests were not run.")
        return 1
    instrumented = {}
    covered = {}
    failed = 0
    jobs = int(os.environ.get("GCOV_JOBS", "0") or "0") or (os.cpu_count() or 1)
    # One gcov process per .gcda, run by a pool (gcov is the slow part).
    # The results are merged here in a fixed order, so the percentage does
    # not depend on the number of workers.
    with concurrent.futures.ThreadPoolExecutor(max_workers=jobs) as pool:
        results = list(pool.map(read_one, notes))
    for result in results:
        if result is None:
            print("Coverage: gcov failed to start.")
            return 1
        if result is False:
            failed += 1
            continue
        for src, lines, hits in result:
            instrumented.setdefault(src, set()).update(lines)
            covered.setdefault(src, set()).update(hits)
    if not instrumented:
        print("Coverage: not measured. gcov produced no executable lines "
              "(%d gcda, %d unreadable)." % (len(notes), failed))
        return 1
    total = sum(len(lines) for lines in instrumented.values())
    hit = 0
    for src, lines in instrumented.items():
        hit += len(lines & covered.get(src, set()))
    percent = (100.0 * hit / total) if total else 0.0
    print("Coverage: %.2f%% lines executed (%d/%d) across %d source files, %d .gcda."
          % (percent, hit, total, len(instrumented), len(notes)))
    print("Command: make coverage")
    print("Flags: --coverage (-fprofile-arcs -ftest-coverage) on host unit tests.")
    print("Scope: translation units those tests compile. Not the guest image.")
    if failed:
        print("Unreadable .gcda: %d" % failed)
    floor = os.environ.get("COVERAGE_MIN", "")
    if floor:
        if percent < float(floor):
            print("Coverage FAIL: %.2f%% is below the floor COVERAGE_MIN=%s%%." % (percent, floor))
            return 1
        print("Coverage floor OK: %.2f%% >= %s%%." % (percent, floor))
    return 0


if __name__ == "__main__":
    sys.exit(main())
