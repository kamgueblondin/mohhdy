#!/usr/bin/env python3
"""Regression: ESC on the VBE desktop must give back a readable text console.

Boots the guest, opens the desktop with `gui`, presses ESC on an empty
line, then takes a QEMU screendump. The text console must be a real
80x25 text mode (720x400 screendump in QEMU std VGA) and must not be the
vertical stripe garbage seen when the VGA registers stay in graphics
mode. A command typed afterwards must still answer on the serial line.
"""
from __future__ import print_function

import os
import sys
import time

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
os.environ.setdefault("OSUI_GUI_LOG", os.path.join(
    os.path.dirname(os.path.abspath(__file__)), "..", "..", "test_logs", "qemu-gui-esc-serial.log"))
os.environ.setdefault("OSUI_GUI_MON_SOCK", "/tmp/mohhdy-gui-esc-mon.sock")
os.environ.setdefault("OSUI_GUI_QMP_SOCK", "/tmp/mohhdy-gui-esc-qmp.sock")
os.environ.setdefault("OVERLAY_DISK", os.path.join(
    os.path.dirname(os.path.abspath(__file__)), "..", "..", "test_logs", "qemu-gui-esc-overlay.img"))
import test_qemu_osui_gui as g  # noqa: E402


def text_screen_ok(width, height, rgb):
    """Text mode screendump: 720x400 (9x16 cells) or 640x400 (8x16).

    Stripe garbage is a graphics-mode screendump whose columns alternate
    saturated magenta/green. A text console is mostly black with light
    glyph pixels; count saturated magenta or green pixels."""
    if width > 800 or height > 600:
        return False, "still a graphics mode %dx%d" % (width, height)
    total = width * height
    bad = 0
    lit = 0
    for i in range(0, len(rgb) - 2, 3):
        r, gg, b = rgb[i], rgb[i + 1], rgb[i + 2]
        if max(r, gg, b) - min(r, gg, b) > 100:
            bad += 1
        if r + gg + b > 60:
            lit += 1
    # The console is cleared on exit: only the grey exit line and prompt
    # remain. Stripes (graphics mode) or stale desktop cells left in the
    # text memory show as saturated colour.
    if bad * 100 > total:
        return False, "stripe or stale cell garbage: %d/%d saturated pixels" % (bad, total)
    if lit == 0:
        return False, "blank screen"
    return True, "text %dx%d lit=%d saturated=%d" % (width, height, lit, bad)


def main():
    import subprocess
    os.makedirs(g.LOG_DIR, exist_ok=True)
    for path in (g.LOG, g.MON_SOCK, g.QMP_SOCK):
        try:
            os.remove(path)
        except OSError:
            pass
    g.prepare_test_disk()
    proc = None
    monitor = None
    try:
        with open(os.devnull, "wb") as err:
            proc = subprocess.Popen([
                "qemu-system-i386", "-cpu", "pentium3", "-kernel", g.KERNEL,
                "-initrd", g.INITRD, "-m", "1024M", "-display", "none", "-vga", "std",
                "-usb", "-device", "usb-tablet",
                "-serial", "file:" + g.LOG, "-monitor", "unix:%s,server,nowait" % g.MON_SOCK,
                "-machine", "type=pc,accel=tcg", "-no-reboot", "-no-shutdown",
            ] + g.qemu_disk_args(), cwd=g.ROOT, stdout=err, stderr=err)
            g.wait_for(proc, "[0m : ", g.BOOT_TIMEOUT)
            monitor = g.monitor_connect()
            time.sleep(0.6)
            for rnd in range(2):
                g.say("round %d: typing gui ..." % rnd)
                start = len(g.log_text())
                g.send_command_until(monitor, "gui", "osui gui live", proc, wait_prompt=False)
                g.wait_for(proc, "OSUI-SNAP", g.CMD_TIMEOUT, start)
                time.sleep(1.0)
                w, h, _ = g.screendump_rgb(monitor, "qemu-gui-esc-desktop.ppm")
                if w < 1000:
                    raise RuntimeError("desktop not in VBE mode: %dx%d" % (w, h))
                start = len(g.log_text())
                g.send_key(monitor, "esc")
                g.wait_for(proc, "chrome=text prompt=MOHHDY>", g.CMD_TIMEOUT, start)
                time.sleep(1.5)
                w, h, rgb = g.screendump_rgb(monitor, "qemu-gui-esc-text-%d.ppm" % rnd)
                ok, why = text_screen_ok(w, h, rgb)
                g.say("after ESC: " + why)
                if not ok:
                    raise RuntimeError("ESC left a broken screen: " + why)
                g.send_command_until(monitor, "gui-status", "gui_live=false", proc)
        g.say("QEMU GUI ESC text console contract passed.")
        return 0
    finally:
        if monitor is not None:
            monitor.close()
        g.terminate(proc)


if __name__ == "__main__":
    try:
        raise SystemExit(main())
    except Exception as error:
        print("QEMU GUI ESC failed: %s" % error, file=sys.stderr)
        raise SystemExit(1)
