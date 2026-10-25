#!/usr/bin/env python3
"""UX5 desktop parity on the real OS (VBE desktop, keys through the QEMU
monitor, clicks through the USB tablet).

  * Editor (dynamic): /edit /samples/long.txt opens a 400-line, 28 KB file
    (over the old 96-line / 8 KB caps) in full through the user heap
    (SYS_HEAP_GROW + umalloc); maximize sends "app K size R C" and the
    editor follows the window; Save As of the 28 KB buffer reaches the
    writable overlay store, which refuses it (384-byte nodes): the error
    is shown, nothing is cut silently.
"""
from __future__ import print_function

import os
import subprocess
import sys
import time

HERE = os.path.dirname(os.path.abspath(__file__))
sys.path.insert(0, HERE)
LOGS = os.path.join(HERE, "..", "..", "test_logs")
os.environ.setdefault("OSUI_GUI_LOG", os.path.join(LOGS, "qemu-gui-ux5-serial.log"))
os.environ.setdefault("OSUI_GUI_MON_SOCK", "/tmp/mohhdy-gui-ux5-mon.sock")
os.environ.setdefault("OSUI_GUI_QMP_SOCK", "/tmp/mohhdy-gui-ux5-qmp.sock")
os.environ.setdefault("OVERLAY_DISK", os.path.join(LOGS, "qemu-gui-ux5-overlay.img"))
import test_qemu_osui_gui as g  # noqa: E402

W, H = 1024, 768
FRAME = (0x0E, 0x14, 0x1B)   # opaque app window body (kernel/gfx_desktop.c)
ALIASES = {" ": "spc", "-": "minus", ".": "dot", "/": "slash", "=": "equal", "_": "shift-minus"}


def say(msg):
    g.say(msg)


def key(monitor, name, delay=0.15):
    g.send_key(monitor, name)
    time.sleep(delay)


def type_text(monitor, text):
    for ch in text:
        if ch in ALIASES:
            key(monitor, ALIASES[ch])
        elif ch.isupper():
            key(monitor, "shift-" + ch.lower())
        else:
            key(monitor, ch)


def slash(monitor, proc, line, marker):
    start = len(g.log_text())
    g.send_command_until(monitor, line, marker, proc, mode="getc", wait_prompt=False)
    time.sleep(0.8)
    return start


def desktop_focus(qmp):
    g.click_tablet(qmp, 1020, 420, W, H)
    time.sleep(0.6)


def pixel(rgb, x, y):
    i = (y * W + x) * 3
    return tuple(rgb[i:i + 3])


def near(p, c, tol=6):
    return all(abs(p[k] - c[k]) <= tol for k in range(3))


def editor_steps(monitor, qmp, proc):
    start = slash(monitor, proc, "/edit /samples/long.txt", "osui edit open file=/samples/long.txt")
    g.wait_for(proc, "osui edit lines=400 complete", g.CMD_TIMEOUT, start)
    say("editor opened the 400-line 28 KB file in full (user heap)")
    rgb = g.screendump_rgb(monitor, "qemu-gui-ux5-edit.ppm")[2]
    if near(pixel(rgb, 900, 300), FRAME):
        raise RuntimeError("editor window already wide before maximize")
    # slot 0 window: x=24, w=640, y=bar_h+20=54; maximize button center
    bar_h = max(28, min(40, H // 22))
    bx = 24 + 640 - 2 - 2 * (14 + 4) + 7
    by = bar_h + 20 + 2 + (30 - 14) // 2 + 7
    start = len(g.log_text())
    g.click_tablet(qmp, bx, by, W, H)
    g.wait_for(proc, "osui edit view resized", g.CMD_TIMEOUT, start)
    time.sleep(1.0)
    rgb = g.screendump_rgb(monitor, "qemu-gui-ux5-edit-max.ppm")[2]
    if not near(pixel(rgb, 900, 300), FRAME) and not near(pixel(rgb, 900, 300), (9, 13, 18)):
        raise RuntimeError("maximized editor does not cover x=900: %r" % (pixel(rgb, 900, 300),))
    say("maximize: kernel sent app size, editor grid follows the window")
    # Save As /long2.txt: the 28 KB buffer is built on the heap, the
    # writable overlay store refuses it (its node holds 384 bytes): the
    # editor reports the store error, the buffer stays modified-safe.
    start = len(g.log_text())
    key(monitor, "esc")
    key(monitor, "a")
    for _ in range(len("/samples/long.txt")):
        key(monitor, "backspace", 0.08)
    type_text(monitor, "/long2.txt")
    key(monitor, "ret")
    g.wait_for(proc, "osui edit save error=", g.CMD_TIMEOUT, start)
    say("Save As of 28 KB: store error reported (overlay node limit), no silent cut")
    desktop_focus(qmp)


def main():
    os.makedirs(g.LOG_DIR, exist_ok=True)
    for path in (g.LOG, g.MON_SOCK, g.QMP_SOCK):
        try:
            os.remove(path)
        except OSError:
            pass
    g.prepare_test_disk()
    proc = monitor = qmp = None
    try:
        with open(os.devnull, "wb") as err:
            proc = subprocess.Popen([
                "qemu-system-i386", "-cpu", "pentium3", "-kernel", g.KERNEL,
                "-initrd", g.INITRD, "-m", "1024M", "-display", "none", "-vga", "std",
                "-usb", "-device", "usb-tablet",
                "-serial", "file:" + g.LOG, "-monitor", "unix:%s,server,nowait" % g.MON_SOCK,
                "-qmp", "unix:%s,server,nowait" % g.QMP_SOCK,
                "-machine", "type=pc,accel=tcg", "-no-reboot", "-no-shutdown",
            ] + g.qemu_disk_args(), cwd=g.ROOT, stdout=err, stderr=err)
            g.wait_for(proc, "[0m : ", g.BOOT_TIMEOUT)
            g.wait_for(proc, "usbdriver tablet enumerated port ", g.CMD_TIMEOUT)
            monitor = g.monitor_connect()
            qmp = g.qmp_connect()
            time.sleep(0.6)
            g.send_command_until(monitor, "gui", "osui gui live", proc, wait_prompt=False)
            g.wait_for(proc, "OSUI-SNAP", g.CMD_TIMEOUT)
            editor_steps(monitor, qmp, proc)
        say("QEMU UX5 GUI parity contract passed.")
        return 0
    finally:
        if qmp is not None:
            qmp.close()
        if monitor is not None:
            monitor.close()
        g.terminate(proc)


if __name__ == "__main__":
    sys.exit(main())
