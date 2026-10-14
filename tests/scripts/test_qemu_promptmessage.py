#!/usr/bin/env python3
"""Focused QEMU contract for Phase 4 PromptMessage (US-046..US-060).

Executes the PromptMessage integration contract in QEMU.
"""
import os
import subprocess
import sys

ROOT = os.path.abspath(os.path.join(os.path.dirname(__file__), "..", ".."))

def main():
    target = os.path.join(ROOT, "tests", "integration", "test_qemu_promptmessage.py")
    res = subprocess.call([sys.executable, target])
    return res

if __name__ == "__main__":
    sys.exit(main())
