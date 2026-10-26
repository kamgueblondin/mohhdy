#!/usr/bin/env python3
"""The packed initrd must fit the kernel file table (fs/initrd.c
INITRD_FILE_MAX): past it, the last files (bin/airogue in UX4) silently
disappear and spawn fails. Counts files and directories, with margin."""
import os
import re
import sys

ROOT = os.path.join(os.path.dirname(os.path.abspath(__file__)), "..", "..")
src = open(os.path.join(ROOT, "fs", "initrd.c")).read()
cap = int(re.search(r"#define INITRD_FILE_MAX (\d+)U", src).group(1))
assert re.search(r"file_index < INITRD_FILE_MAX", src), "parser loop must use INITRD_FILE_MAX"
n = 0
for base, dirs, files in os.walk(os.path.join(ROOT, "initrd_content")):
    n += len(dirs) + len(files)
# Lot 47: the test image (QEMU tests) adds the proof programs to the same table
mk = open(os.path.join(ROOT, "Makefile")).read()
m = re.search(r"^TEST_ONLY_PROGRAMS = ((?:.*\\\n)*.*)$", mk, re.M)
assert m, "TEST_ONLY_PROGRAMS missing from the Makefile"
test_only = m.group(1).replace("\\\n", " ").split()
present = set(os.listdir(os.path.join(ROOT, "initrd_content", "bin"))) if os.path.isdir(os.path.join(ROOT, "initrd_content", "bin")) else set()
n += len([p for p in test_only if p not in present])
if n + 8 > cap:
    print("FAIL initrd has %d entries, table holds %d (keep 8 spare)" % (n, cap))
    sys.exit(1)
print("OK initrd entries %d of %d" % (n, cap))
