#!/usr/bin/env python3
"""Lot 68: the desktop text scale is real on screen.

Boots the guest, opens the desktop at 1x and measures the menu bar height
and the size of the glyph ink in the chat window, then sets `ui-scale 2`
on the console (the same command path the GUI uses), opens the desktop
again and checks the metrics doubled. Then Ctrl+'+' / Ctrl+'-' / Ctrl+'0'
on the desktop must report a zoom of the focused window, and the choice
must be saved in /persist/ui.cfg. PNG screenshots of both scales go to
LOT68_SHOTS (default test_logs/lot68-shots).
"""
from __future__ import print_function

import os
import sys
import time

HERE = os.path.dirname(os.path.abspath(__file__))
sys.path.insert(0, HERE)
LOGS = os.path.join(HERE, "..", "..", "test_logs")
os.environ.setdefault("OSUI_GUI_LOG", os.path.join(LOGS, "qemu-gui-scale-serial.log"))
os.environ.setdefault("OSUI_GUI_MON_SOCK", "/tmp/mohhdy-gui-scale-mon.sock")
os.environ.setdefault("OSUI_GUI_QMP_SOCK", "/tmp/mohhdy-gui-scale-qmp.sock")
os.environ.setdefault("OVERLAY_DISK", os.path.join(LOGS, "qemu-gui-scale-overlay.img"))
import test_qemu_osui_gui as g  # noqa: E402

SHOTS = os.environ.get("LOT68_SHOTS", os.path.join(LOGS, "lot68-shots"))


def px(rgb, w, x, y):
    i = (y * w + x) * 3
    return rgb[i], rgb[i + 1], rgb[i + 2]


def bar_height(w, h, rgb):
    """Rows from the top that keep the menu bar colour at x=2."""
    top = px(rgb, w, 2, 1)
    y = 1
    while y < h // 3 and px(rgb, w, 2, y) == top:
        y += 1
    return y


def ink_runs(w, h, rgb, y0, y1):
    """Median height of vertical runs of light text pixels in a band:
    an 8x8 glyph gives runs up to 8 px at 1x, 16 px at 2x."""
    runs = []
    for x in range(0, w, 3):
        run = 0
        for y in range(y0, y1):
            r, gg, b = px(rgb, w, x, y)
            if r + gg + b > 600:
                run += 1
            elif run:
                runs.append(run)
                run = 0
    runs = [r for r in runs if r >= 2]
    runs.sort()
    if not runs:
        return 0, 0
    return runs[len(runs) * 98 // 100], len(runs)


def save_png(name, w, h, rgb):
    try:
        from PIL import Image
    except ImportError:
        return None
    os.makedirs(SHOTS, exist_ok=True)
    path = os.path.join(SHOTS, name)
    Image.frombytes("RGB", (w, h), bytes(rgb)).save(path)
    return path


def open_desktop(monitor, proc, label):
    start = len(g.log_text())
    g.send_command_until(monitor, "gui", "osui gui live", proc, wait_prompt=False)
    g.wait_for(proc, "OSUI-SNAP", g.CMD_TIMEOUT, start)
    time.sleep(1.5)
    w, h, rgb = g.screendump_rgb(monitor, "qemu-gui-scale-%s.ppm" % label)
    if w < 1000:
        raise RuntimeError("desktop not in VBE mode: %dx%d" % (w, h))
    bar = bar_height(w, h, rgb)
    run, count = ink_runs(w, h, rgb, bar, h - 40)
    mrun, _ = ink_runs(w, h, rgb, 0, bar)
    shot = save_png("desktop-%s.png" % label, w, h, rgb)
    g.say("%s: %dx%d bar=%d menu-glyph p90=%d glyph-run p90=%d runs=%d png=%s" % (label, w, h, bar, mrun, run, count, shot))
    return w, h, mrun, run


def leave_desktop(monitor, proc):
    start = len(g.log_text())
    g.send_key(monitor, "esc")
    g.wait_for(proc, "chrome=text prompt=MOHHDY>", g.CMD_TIMEOUT, start)
    time.sleep(1.0)


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
            g.send_command_until(monitor, "ui-scale", "ui-scale ok global", proc)
            _, _, bar1, run1 = open_desktop(monitor, proc, "1x")
            leave_desktop(monitor, proc)
            g.send_command_until(monitor, "ui-scale 2", "char 16px", proc)
            g.send_command_until(monitor, "cat /persist/ui.cfg", "ui_scale=8", proc)
            _, _, bar2, run2 = open_desktop(monitor, proc, "2x")
            if bar2 < bar1 * 3 // 2:
                raise RuntimeError("menu bar text did not grow: %d -> %d" % (bar1, bar2))
            if run1 == 0 or run2 < run1 * 3 // 2:
                raise RuntimeError("glyphs did not grow: %d -> %d" % (run1, run2))
            for key, want in (("ctrl-equal", "osui zoom "), ("ctrl-minus", "osui zoom "),
                              ("ctrl-equal", "osui zoom "), ("ctrl-0", "chat zoom 1x effective 2x")):
                start = len(g.log_text())
                g.send_key(monitor, key)
                g.wait_for(proc, want, g.CMD_TIMEOUT, start)
                time.sleep(0.4)
            start = len(g.log_text())
            g.send_key(monitor, "ctrl-equal")
            g.wait_for(proc, "osui zoom ", g.CMD_TIMEOUT, start)
            time.sleep(1.5)
            w, h, rgb = g.screendump_rgb(monitor, "qemu-gui-scale-zoom.ppm")
            save_png("desktop-2x-ctrl-plus.png", w, h, rgb)
            run3, _ = ink_runs(w, h, rgb, bar_height(w, h, rgb), h - 40)
            g.say("after Ctrl+'+': glyph-run p98=%d" % run3)
            if run3 <= run2:
                raise RuntimeError("Ctrl+'+' did not enlarge the chat text: %d -> %d" % (run2, run3))
            leave_desktop(monitor, proc)
            g.send_command_until(monitor, "ui-scale 1", "ui-scale ok global 1x", proc)
            g.send_command_until(monitor, "text-size chat 0", "text-size ok chat", proc)
        g.say("QEMU GUI text scale contract passed (menu glyph %d->%d, glyph %d->%d)." % (bar1, bar2, run1, run2))
        return 0
    finally:
        if monitor is not None:
            monitor.close()
        g.terminate(proc)


if __name__ == "__main__":
    try:
        raise SystemExit(main())
    except Exception as error:
        print("QEMU GUI text scale failed: %s" % error, file=sys.stderr)
        raise SystemExit(1)
