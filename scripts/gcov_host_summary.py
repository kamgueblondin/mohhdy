#!/usr/bin/env python3
"""Line coverage of the host unit tests, measured by gcov.

Each .gcda next to a test binary under build/ is one instrumented copy.
The same source compiled into several binaries is merged: a line counts as
executed when any copy executed it. Sources that no host test compiles are
not in the denominator. This is not guest-image coverage.
"""
from __future__ import print_function

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


def main():
    notes = gcda_files()
    if not notes:
        print("Coverage: not measured. No .gcda under build/. "
              "CFLAGS did not pass -fprofile-arcs -ftest-coverage, or the tests were not run.")
        return 1
    instrumented = {}
    covered = {}
    failed = 0
    for path in notes:
        directory = os.path.dirname(path)
        name = os.path.basename(path)
        try:
            proc = subprocess.run(
                ["gcov", "-n", "-o", directory, name],
                cwd=ROOT, capture_output=True, text=True, check=False)
        except OSError as error:
            print("Coverage: gcov failed to start: %s" % error)
            return 1
        if proc.returncode != 0 and "Lines executed" not in proc.stdout:
            failed += 1
            continue
        # gcov -n does not write a per-line file. Ask for the annotated
        # form in a private directory so lines can be merged across copies.
        with tempfile.TemporaryDirectory(prefix="gcov-") as tmp:
            proc = subprocess.run(
                ["gcov", "-o", directory, name],
                cwd=tmp, capture_output=True, text=True, check=False)
            if proc.returncode != 0 and not os.listdir(tmp):
                failed += 1
                continue
            for produced in os.listdir(tmp):
                if not produced.endswith(".gcov"):
                    continue
                src = None
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
                        instrumented.setdefault(src, set()).add(lineno)
                        if kind != "#####":
                            covered.setdefault(src, set()).add(lineno)
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
    return 0


if __name__ == "__main__":
    sys.exit(main())
