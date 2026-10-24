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
n = 0
for base, dirs, files in os.walk(os.path.join(ROOT, "initrd_content")):
    n += len(dirs) + len(files)
if n + 8 > cap:
    print("FAIL initrd has %d entries, table holds %d (keep 8 spare)" % (n, cap))
    sys.exit(1)
print("OK initrd entries %d of %d" % (n, cap))
