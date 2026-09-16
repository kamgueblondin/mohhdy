#!/usr/bin/env python3
"""Launch QEMU GTK so the Mohhdy VBE desktop fills the window.

zoom-to-fit keeps the whole desktop visible when the window is resized.
A second serial (COM2) tells the guest the inner size so VBE + compositor
reflow to that resolution (not an HTML host window).
"""
from __future__ import print_function

import os
import socket
import subprocess
import sys
import threading
import time

ROOT = os.path.abspath(os.path.join(os.path.dirname(__file__), ".."))
KERNEL = os.environ.get("MOHHDY_KERNEL", os.path.join(ROOT, "build", "mohhdy.bin"))
INITRD = os.environ.get("MOHHDY_INITRD", os.path.join(ROOT, "my_initrd.tar"))
DISK = os.environ.get("DISK_IMAGE", os.path.join(ROOT, "build", "overlay.img"))
RAM = os.environ.get("GPT2_RAM", "1024M")
SOCK = os.environ.get("MOHHDY_FIT_SOCK", "/tmp/mohhdy-qemu-fit.sock")
MIN_W, MIN_H = 640, 400
MAX_W, MAX_H = 1920, 1200


def clamp_size(width, height):
    if width < MIN_W:
        width = MIN_W
    if height < MIN_H:
        height = MIN_H
    if width > MAX_W:
        width = MAX_W
    if height > MAX_H:
        height = MAX_H
    return width & ~1, height & ~1


def qemu_window_size():
    try:
        out = subprocess.check_output(
            ["xdotool", "search", "--name", "QEMU"],
            stderr=subprocess.DEVNULL,
        ).decode("ascii", "replace").strip()
    except (OSError, subprocess.CalledProcessError):
        return None
    ids = [line for line in out.splitlines() if line.strip()]
    if not ids:
        return None
    wid = ids[-1]
    try:
        info = subprocess.check_output(
            ["xwininfo", "-id", wid],
            stderr=subprocess.DEVNULL,
        ).decode("ascii", "replace")
    except (OSError, subprocess.CalledProcessError):
        return None
    width = height = None
    for line in info.splitlines():
        line = line.strip()
        if line.startswith("Width:"):
            width = int(line.split(":", 1)[1])
        elif line.startswith("Height:"):
            height = int(line.split(":", 1)[1])
    if not width or not height:
        return None
    # GTK menu bar is hidden; keep a small hysteresis for WM chrome.
    if height > 40:
        height -= 28
    return clamp_size(width, height)


def fit_loop(stop):
    client = None
    last = None
    while not stop.is_set():
        if client is None:
            try:
                client = socket.socket(socket.AF_UNIX, socket.SOCK_STREAM)
                client.connect(SOCK)
                client.settimeout(0.2)
            except OSError:
                if client is not None:
                    try:
                        client.close()
                    except OSError:
                        pass
                client = None
                time.sleep(0.25)
                continue
        size = qemu_window_size()
        if size and last is not None:
            if abs(size[0] - last[0]) < 16 and abs(size[1] - last[1]) < 16:
                size = None
        if size and size != last:
            try:
                client.sendall(("%d %d\n" % size).encode("ascii"))
                last = size
            except OSError:
                try:
                    client.close()
                except OSError:
                    pass
                client = None
                last = None
        time.sleep(0.25)
    if client is not None:
        try:
            client.close()
        except OSError:
            pass


def main():
    os.environ.setdefault("DISPLAY", ":1")
    try:
        os.remove(SOCK)
    except OSError:
        pass

    cmd = [
        "qemu-system-i386",
        "-name", "Mohhdy OS",
        "-cpu", "pentium3",
        "-kernel", KERNEL,
        "-initrd", INITRD,
        "-m", RAM,
        "-vga", "std",
        "-display", "gtk,zoom-to-fit=on,show-menubar=off",
        "-serial", "mon:stdio",
        "-serial", "unix:%s,server,nowait" % SOCK,
        "-no-reboot", "-no-shutdown",
    ]
    if os.path.isfile(DISK):
        cmd.extend(["-drive", "file=%s,format=raw,if=ide,cache=writethrough" % DISK])

    sys.stderr.write("Mohhdy desktop fills the QEMU window (zoom-to-fit + VBE resize)\n")
    sys.stderr.write("chrome=qemu_fb  pas une fenetre HTML\n")
    qemu = subprocess.Popen(cmd, cwd=ROOT)
    stop = threading.Event()
    watcher = threading.Thread(target=fit_loop, args=(stop,))
    watcher.daemon = True
    watcher.start()
    try:
        return qemu.wait()
    finally:
        stop.set()
        if qemu.poll() is None:
            qemu.terminate()
            try:
                qemu.wait(timeout=3)
            except Exception:
                qemu.kill()
        try:
            os.remove(SOCK)
        except OSError:
            pass


if __name__ == "__main__":
    raise SystemExit(main())
