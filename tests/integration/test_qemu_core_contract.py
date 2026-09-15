#!/usr/bin/env python3
"""AOS-022 integration contract: a built MOHHDY image must pass the QEMU core shell scenario."""
import os
import subprocess
import sys

ROOT = os.path.abspath(os.path.join(os.path.dirname(__file__), "..", ".."))
SCENARIO = os.path.join(ROOT, "tests", "scripts", "ci_qemu_core_smoke.py")


def main():
    if not os.path.isfile(os.path.join(ROOT, "build", "mohhdy.bin")):
        raise RuntimeError("missing kernel artefact; run make all first")
    if not os.path.isfile(os.path.join(ROOT, "my_initrd.tar")):
        raise RuntimeError("missing initrd artefact; run make all first")
    result = subprocess.run([sys.executable, SCENARIO], cwd=ROOT)
    if result.returncode != 0:
        raise RuntimeError("QEMU core shell contract failed")
    print("AOS-022 integration contract passed: boot, ai-runtime, overlay copy, append and shell return.")


if __name__ == "__main__":
    try:
        main()
    except Exception as error:
        print("AOS-022 integration contract failed: %s" % error, file=sys.stderr)
        raise SystemExit(1)
