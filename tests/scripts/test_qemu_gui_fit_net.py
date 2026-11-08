#!/usr/bin/env python3
"""Host check: make run-gui gives the guest the ISA NE2000 it drives."""
import importlib.util
import os
import sys

ROOT = os.path.abspath(os.path.join(os.path.dirname(__file__), "..", ".."))
spec = importlib.util.spec_from_file_location("qemu_gui_fit", os.path.join(ROOT, "scripts", "qemu_gui_fit.py"))
mod = importlib.util.module_from_spec(spec)
spec.loader.exec_module(mod)
args = " ".join(mod.NET_ARGS)
assert "-netdev user,id=n0" in args, args
assert "-device ne2k_isa,netdev=n0" in args, args
src = open(os.path.join(ROOT, "scripts", "qemu_gui_fit.py")).read()
assert "cmd.extend(NET_ARGS" in src
print("qemu_gui_fit net ok")
sys.exit(0)
