#!/usr/bin/env python3
"""Roadmap step 4 guardrail: no provider secret is ever built into the image.

Scans the kernel images and the initrd for credential shapes (API keys,
bearer tokens, private keys). Test-only TLS material used by the local QEMU
peer contracts is allowed only when it is explicitly marked as test data.
"""
import os
import re
import sys

ROOT = os.path.abspath(os.path.join(os.path.dirname(__file__), ".."))
TARGETS = ["build/mohhdy.bin", "build/mohhdy-netlegacy.bin", "my_initrd.tar"]
PATTERNS = [
    (b"openai_key", re.compile(rb"(?<![A-Za-z0-9-])sk-(?:proj-)?[A-Za-z0-9_]{20,}")),
    (b"bearer", re.compile(rb"Bearer [A-Za-z0-9._-]{16,}")),
    (b"env_key", re.compile(rb"(?:OPENAI|ANTHROPIC|API)_?KEY=[^\s\x00]{8,}")),
    (b"private_key", re.compile(rb"-----BEGIN (?:RSA |EC )?PRIVATE KEY-----")),
    (b"aws_key", re.compile(rb"AKIA[0-9A-Z]{16}")),
]


def main():
    found = 0
    scanned = 0
    for rel in TARGETS:
        path = os.path.join(ROOT, rel)
        if not os.path.exists(path):
            continue
        scanned += 1
        data = open(path, "rb").read()
        for name, rx in PATTERNS:
            for m in rx.finditer(data):
                found += 1
                print("FAIL: %s: %s at offset %d" % (rel, name.decode(), m.start()))
    if scanned == 0:
        print("FAIL: nothing to scan (build first)")
        return 1
    if found:
        return 1
    print("OK no provider secret in %d image(s)" % scanned)
    return 0


if __name__ == "__main__":
    sys.exit(main())
