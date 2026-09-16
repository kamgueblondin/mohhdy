#!/usr/bin/env python3
"""Focused host check: product Python facade is gone (OS-UI-3).

Hors make integration-qemu. Python hote (tests/scripts, extracteur) reste autorise.
"""
from __future__ import print_function

import os
import sys

ROOT = os.path.abspath(os.path.join(os.path.dirname(__file__), "..", ".."))

FORBIDDEN = [
    "agent/server.py",
    "agent/tools.py",
    "agent/Dockerfile",
    "osui/server.py",
    "osui/stage.py",
    "osui/prompt_os.py",
    "osui/guest_attach.py",
    "osui/command_registry.py",
    "osui/static/index.html",
    "osui/static/os.js",
    "osui/Dockerfile",
    "osui/docker-compose.yml",
]

REQUIRED = [
    "userspace/osui_runtime.c",
    "userspace/osui_runtime.h",
    "userspace/osui_gui.c",
    "userspace/osui_gui.h",
    "userspace/shell.c",
    "userspace/mohhdy_osui_bridge.h",
    "shared/multiboot_shell_commands.json",
    "scripts/extract_guest_commands.py",
    "Dockerfile",
    "docker/qemu-nographic.sh",
    "docker/qemu-gui.sh",
]


def fail(message):
    sys.stderr.write("FAIL: %s\n" % message)
    sys.exit(1)


def main():
    for rel in FORBIDDEN:
        path = os.path.join(ROOT, rel)
        if os.path.exists(path):
            fail("python facade still present: %s" % rel)
    for rel in REQUIRED:
        path = os.path.join(ROOT, rel)
        if not os.path.isfile(path):
            fail("missing required OS-UI guest file: %s" % rel)
    header = open(os.path.join(ROOT, "userspace/mohhdy_osui_bridge.h"), "r").read()
    if "#define MOHHDY_OSUI_PYTHON_FACADE 0" not in header:
        fail("MOHHDY_OSUI_PYTHON_FACADE must be 0")
    if "#define MOHHDY_OSUI_GUEST_HTML_STAGE 0" not in header:
        fail("MOHHDY_OSUI_GUEST_HTML_STAGE must be 0")
    if "#define MOHHDY_OSUI_STAGE_VGA 1" not in header:
        fail("MOHHDY_OSUI_STAGE_VGA must be 1")
    if "#define MOHHDY_OSUI_VGA_DESKTOP 1" not in header:
        fail("MOHHDY_OSUI_VGA_DESKTOP must be 1")
    if '#define MOHHDY_OSUI_GUI_COMMAND "gui"' not in header:
        fail("MOHHDY_OSUI_GUI_COMMAND must be gui")
    dockerfile = open(os.path.join(ROOT, "Dockerfile"), "r").read()
    if "python3" in dockerfile.lower() and "CMD" in dockerfile:
        if "server.py" in dockerfile:
            fail("Dockerfile still launches a Python server")
    if "qemu-system-i386" not in open(os.path.join(ROOT, "docker/qemu-nographic.sh")).read():
        fail("docker entrypoint must boot qemu-system-i386")
    print("OK python facade removed; guest C + QEMU docker remain")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
