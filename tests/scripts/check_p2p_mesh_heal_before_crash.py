#!/usr/bin/env python3
"""Static contract: mesh must wait for b-d heal before crashing beta.

Lot 76 / CI 42205891754: after scenario 1 cuts b-d, delta logs "p2p peer beta
down". Healing b-d without waiting for "p2p peer beta up" on delta, then killing
beta, yields no second "down" (p2p only logs down when p->up). The harness then
times out while the log tail still shows the older down line.
"""
from __future__ import print_function

import os
import re
import sys

ROOT = os.path.dirname(os.path.dirname(os.path.dirname(os.path.abspath(__file__))))
MESH = os.path.join(ROOT, "tests", "scripts", "test_qemu_p2p_mesh.py")


def main():
    text = open(MESH, "r").read()
    m = re.search(
        r"# 2\. Diamond, then crash beta.*?"
        r"p3\.wait_log\(d, \"p2p peer beta down\"",
        text,
        re.S,
    )
    if not m:
        print("FAIL: scenario 2 crash block not found in %s" % MESH)
        return 1
    block = m.group(0)
    need = [
        'hub.heal_pair(b["mac"], d["mac"])',
        'heal_mark = ',
        'p3.wait_log(d, "p2p peer beta up id "',
        'p3.wait_log(b, "p2p peer delta up id "',
        'crash_mark = ',
        'base.terminate(b["proc"])',
        'p3.wait_log(d, "p2p peer beta down"',
    ]
    for item in need:
        if item not in block:
            print("FAIL: scenario 2 missing %r" % item)
            return 1
    pos = [block.find(item) for item in need]
    if pos != sorted(pos):
        print("FAIL: scenario 2 heal/up/crash ordering wrong: %s" % list(zip(need, pos)))
        return 1
    print("OK: p2p mesh waits for b-d heal (beta up on delta) before crash_mark")
    return 0


if __name__ == "__main__":
    sys.exit(main())
