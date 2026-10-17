#!/usr/bin/env python3
"""Builds DOS .COM images from commented hex sources (dosapps/*.com.hex).

Each line holds hex byte pairs; '#' starts a comment. The sources are text so
the tree stays ASCII; the binaries are produced at initrd pack time.
usage: com_from_hex.py SRC_DIR DEST_DIR
"""
import os
import sys


def convert(path):
    data = bytearray()
    with open(path, "r", encoding="ascii") as f:
        for line in f:
            line = line.split("#", 1)[0]
            for tok in line.split():
                data.append(int(tok, 16))
    return bytes(data)


def main():
    if len(sys.argv) != 3:
        print(__doc__)
        return 2
    src, dst = sys.argv[1], sys.argv[2]
    os.makedirs(dst, exist_ok=True)
    for name in sorted(os.listdir(src)):
        if not name.endswith(".com.hex"):
            continue
        out = os.path.join(dst, name[:-4])
        with open(out, "wb") as f:
            f.write(convert(os.path.join(src, name)))
    return 0


if __name__ == "__main__":
    sys.exit(main())
