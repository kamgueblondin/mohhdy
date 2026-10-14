#!/usr/bin/env python3
"""Fail when the worktree has changes outside build/.

git status skips ignored paths, so objects under build/, the initrd
tarball and test logs do not count. A tracked file rewritten in place,
or a new untracked file anywhere else, fails the check.
"""
import subprocess
import sys


def path_of(line):
    text = line[3:]
    if " -> " in text:
        text = text.split(" -> ", 1)[1]
    return text.strip().strip('"')


def main():
    run = subprocess.run(
        ["git", "status", "--porcelain", "-uall"],
        capture_output=True, text=True)
    if run.returncode != 0:
        sys.stderr.write(run.stderr)
        return run.returncode
    bad = []
    for line in run.stdout.splitlines():
        if len(line) < 4:
            continue
        path = path_of(line)
        if path == "build" or path.startswith("build/"):
            continue
        bad.append(line)
    if bad:
        sys.stderr.write("FAIL: tree dirty outside build/\n")
        for line in bad:
            sys.stderr.write(line + "\n")
        return 1
    print("OK tree clean outside build/")
    return 0


if __name__ == "__main__":
    sys.exit(main())
