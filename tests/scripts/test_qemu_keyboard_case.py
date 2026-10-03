#!/usr/bin/env python3
"""Shift and Caps Lock must change letter case in both directions.

Left Shift break is scancode 0xAA. Dropping it latches Shift, so the next
letters stay uppercase. Caps Lock XOR Shift must turn letters back to
lowercase, and releasing Caps Lock must restore lowercase.
"""
from __future__ import print_function

import os
import socket
import subprocess
import sys
import time

ROOT = os.path.abspath(os.path.join(os.path.dirname(__file__), "..", ".."))
KERNEL = os.environ.get("KERNEL", os.path.join(ROOT, "build", "mohhdy.bin"))
INITRD = os.environ.get("INITRD", os.path.join(ROOT, "my_initrd.tar"))
LOG = os.environ.get("LOG", os.path.join(ROOT, "test_logs", "keyboard-case.log"))
ERR = os.environ.get("QEMU_ERR", os.path.join(ROOT, "test_logs", "keyboard-case.err"))
MON = os.environ.get("QEMU_MON_SOCK", os.path.join(ROOT, "test_logs", "keyboard-case-monitor.sock"))
BOOT_TIMEOUT = float(os.environ.get("BOOT_TIMEOUT", "180"))
KEY_DELAY = float(os.environ.get("KEY_DELAY", "0.15"))


def text():
    try:
        with open(LOG, "r", errors="replace") as handle:
            return handle.read()
    except OSError:
        return ""


def wait_for(proc, needle, timeout, start=0):
    end = time.monotonic() + timeout
    while time.monotonic() < end:
        if proc.poll() is not None:
            raise RuntimeError("QEMU stopped: %s" % text()[-2000:])
        if needle in text()[start:]:
            time.sleep(0.3)
            return text()
        time.sleep(0.2)
    raise RuntimeError("timeout for %r: %s" % (needle, text()[-2000:]))


def monitor():
    end = time.monotonic() + 15.0
    while time.monotonic() < end:
        if os.path.exists(MON):
            client = socket.socket(socket.AF_UNIX, socket.SOCK_STREAM)
            try:
                client.connect(MON)
                client.settimeout(0.1)
                try:
                    client.recv(4096)
                except socket.timeout:
                    pass
                return client
            except OSError:
                client.close()
        time.sleep(0.1)
    raise RuntimeError("QEMU monitor unavailable")


def key(client, name):
    client.sendall(("sendkey %s 80\n" % name).encode("ascii"))
    time.sleep(KEY_DELAY)


def line(client, names):
    for name in names:
        key(client, name)
    key(client, "ret")


def main():
    if not os.path.isfile(KERNEL) or not os.path.isfile(INITRD):
        raise RuntimeError("missing kernel or initrd")
    os.makedirs(os.path.dirname(LOG), exist_ok=True)
    for path in (LOG, ERR, MON):
        try:
            os.remove(path)
        except OSError:
            pass
    proc = None
    client = None
    try:
        with open(ERR, "wb") as err:
            proc = subprocess.Popen([
                "qemu-system-i386", "-cpu", "pentium3", "-kernel", KERNEL,
                "-initrd", INITRD, "-m", "1024M", "-display", "none", "-vga", "none",
                "-serial", "file:" + LOG, "-monitor", "unix:%s,server,nowait" % MON,
                "-machine", "type=pc,accel=tcg", "-no-reboot", "-no-shutdown",
            ], cwd=ROOT, stdout=err, stderr=err)
            wait_for(proc, "SYS_GETS: Debut", BOOT_TIMEOUT)
            client = monitor()
            start = len(text())
            # Shift only on T, then lowercase. A latched Shift would shout HE.
            line(client, ["e", "c", "h", "o", "spc", "shift-t", "h", "e"])
            wait_for(proc, "ligne lue: echo The", BOOT_TIMEOUT, start)
            start = len(text())
            # Caps Lock on, Shift+t must come back out lowercase, then e is E.
            line(client, ["e", "c", "h", "o", "spc", "caps_lock", "shift-t", "e", "caps_lock"])
            wait_for(proc, "ligne lue: echo tE", BOOT_TIMEOUT, start)
            start = len(text())
            # Caps Lock released: the next word is lowercase again.
            line(client, ["e", "c", "h", "o", "spc", "o", "k"])
            wait_for(proc, "ligne lue: echo ok", BOOT_TIMEOUT, start)
        print("keyboard case test passed")
        return 0
    finally:
        if client is not None:
            client.close()
        if proc is not None and proc.poll() is None:
            proc.terminate()
            try:
                proc.wait(timeout=4)
            except subprocess.TimeoutExpired:
                proc.kill()
        try:
            os.remove(MON)
        except OSError:
            pass


if __name__ == "__main__":
    try:
        raise SystemExit(main())
    except Exception as error:
        print("keyboard case test failed: %s" % error, file=sys.stderr)
        raise SystemExit(1)
