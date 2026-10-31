#!/usr/bin/env python3
"""The VBE terminal window is a real window: hover, buttons, drag.

1. Hovering the pointer across the window must not erase it: after the
   pointer leaves, the window pixels are identical to before; while it is
   over the window only the 12x18 cursor box differs and the cursor is
   drawn over the terminal.
2. Maximize toggles, a title bar drag moves the window, close returns to
   the desktop with the console kept (reopening shows the earlier output).
"""
from __future__ import print_function

import os
import re
import subprocess
import sys
import time

HERE = os.path.dirname(os.path.abspath(__file__))
sys.path.insert(0, HERE)
LOGS = os.path.join(HERE, "..", "..", "test_logs")
os.environ.setdefault("OSUI_GUI_LOG", os.path.join(LOGS, "qemu-gui-term-window-serial.log"))
os.environ.setdefault("OSUI_GUI_MON_SOCK", "/tmp/mohhdy-gui-termwin-mon.sock")
os.environ.setdefault("OSUI_GUI_QMP_SOCK", "/tmp/mohhdy-gui-termwin-qmp.sock")
os.environ.setdefault("OVERLAY_DISK", os.path.join(LOGS, "qemu-gui-term-window-overlay.img"))
import test_qemu_osui_gui as g  # noqa: E402

W, H = 1024, 768


def move(qmp, x, y):
    ax, ay = g.tablet_abs_xy(x, y, W, H)
    g.qmp_cmd(qmp, {"execute": "input-send-event", "arguments": {"events": [
        {"type": "abs", "data": {"axis": "x", "value": ax}},
        {"type": "abs", "data": {"axis": "y", "value": ay}},
    ]}})
    time.sleep(0.35)


def button(qmp, down):
    g.qmp_cmd(qmp, {"execute": "input-send-event", "arguments": {"events": [
        {"type": "btn", "data": {"down": down, "button": "left"}},
    ]}})
    time.sleep(0.4)


def region(rgb, x, y, w, h):
    out = bytearray()
    for r in range(y, y + h):
        out += rgb[(r * W + x) * 3:(r * W + x + w) * 3]
    return bytes(out)


def diff_box(a, b, x, y, w, h):
    """Bounding box (x0, y0, x1, y1) of differing pixels in a window region."""
    box = None
    for r in range(h):
        ra = a[r * w * 3:(r + 1) * w * 3]
        rb = b[r * w * 3:(r + 1) * w * 3]
        if ra == rb:
            continue
        for c in range(w):
            if ra[c * 3:c * 3 + 3] != rb[c * 3:c * 3 + 3]:
                px, py = x + c, y + r
                if box is None:
                    box = [px, py, px, py]
                else:
                    box = [min(box[0], px), min(box[1], py), max(box[2], px), max(box[3], py)]
    return box


def window_from_log(start):
    m = None
    for m in re.finditer(r"gfx term on fb \d+x\d+ grid 80x25 scale (\d+) window (\d+),(\d+) (\d+)x(\d+)",
                         g.log_text()[start:]):
        pass
    if not m:
        raise RuntimeError("no terminal window line on serial")
    return tuple(int(v) for v in m.groups())


def button_center(win, which):
    scale, x, y, w, _h = win
    k = {"close": 1, "max": 2, "min": 3}[which]
    bx = x + w - 2 - k * (14 + 4) * scale
    by = y + 2 + (20 * scale - 14 * scale) // 2
    return bx + 7 * scale, by + 7 * scale


def open_terminal(monitor, proc):
    start = len(g.log_text())
    g.send_command_until(monitor, "/shell", "chrome=vbe_term", proc, mode="getc", wait_prompt=False)
    g.wait_for(proc, "gfx term on fb ", g.CMD_TIMEOUT, start)
    g.wait_for(proc, "[0m : ", g.CMD_TIMEOUT, start)
    time.sleep(0.8)
    return window_from_log(start)


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
                "qemu-system-i386", "-fw_cfg", "name=opt/mohhdy/ui-scale,string=4", "-cpu", "pentium3", "-kernel", g.KERNEL,
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
            win = open_terminal(monitor, proc)
            scale, wx, wy, ww, wh = win
            g.say("terminal window %r" % (win,))
            g.send_command_until(monitor, "p2p-status", "p2p", proc)
            time.sleep(0.5)
            # 1. hover
            move(qmp, 5, H - 5)
            _, _, a = g.screendump_rgb(monitor, "qemu-term-hover-a.ppm")
            ra = region(a, wx, wy, ww, wh)
            for (x, y) in ((wx + 40, wy + 60), (wx + ww // 2, wy + wh // 2), (wx + ww - 60, wy + wh - 40),
                           (wx + 100, wy + 8)):
                move(qmp, x, y)
            cx, cy = wx + ww // 2, wy + wh // 2
            move(qmp, cx, cy)
            time.sleep(0.5)
            _, _, c = g.screendump_rgb(monitor, "qemu-term-hover-c.ppm")
            box = diff_box(ra, region(c, wx, wy, ww, wh), wx, wy, ww, wh)
            if box is None:
                raise RuntimeError("pointer not drawn over the terminal window")
            if box[0] < cx - 1 or box[1] < cy - 1 or box[2] > cx + 12 or box[3] > cy + 18:
                raise RuntimeError("hover changed the window outside the cursor box: %r" % (box,))
            move(qmp, 5, H - 5)
            time.sleep(0.5)
            _, _, b = g.screendump_rgb(monitor, "qemu-term-hover-b.ppm")
            box = diff_box(ra, region(b, wx, wy, ww, wh), wx, wy, ww, wh)
            if box is not None:
                raise RuntimeError("hover erased terminal pixels in %r" % (box,))
            g.say("hover: cursor drawn over the window, window pixels restored")
            # 2. maximize on and off
            for state in ("on", "off"):
                start = len(g.log_text())
                move(qmp, *button_center(win if state == "on" else (scale, 0, 0, W, H), "max"))
                button(qmp, True)
                button(qmp, False)
                g.wait_for(proc, "gfx term button maximize " + state, g.CMD_TIMEOUT, start)
                time.sleep(1.0)
            g.say("maximize toggles")
            # 3. drag by the title bar
            start = len(g.log_text())
            move(qmp, wx + 60, wy + 10)
            button(qmp, True)
            move(qmp, wx + 60 - 150, wy + 10 - 100)
            button(qmp, False)
            g.wait_for(proc, "gfx term moved", g.CMD_TIMEOUT, start)
            time.sleep(1.0)
            _, _, d = g.screendump_rgb(monitor, "qemu-term-moved.ppm")
            if region(d, wx - 150, wy - 100, 40, 10) != region(a, wx, wy, 40, 10):
                raise RuntimeError("title bar not at the dragged position")
            g.say("drag moves the window")
            moved = (scale, wx - 150, wy - 100, ww, wh)
            # 4. close returns to the desktop, console kept
            start = len(g.log_text())
            move(qmp, *button_center(moved, "close"))
            button(qmp, True)
            button(qmp, False)
            g.wait_for(proc, "gfx term button close", g.CMD_TIMEOUT, start)
            g.wait_for(proc, "osui gui live", g.CMD_TIMEOUT, start)
            g.say("close returns to the desktop")
            open_terminal(monitor, proc)
            start = g.send_command_until(monitor, "term-dump", "TERM-DUMP END", proc)
            dump = g.log_text()[start:]
            if "p2p-status error" not in dump:
                raise RuntimeError("console output lost after close: %s" % dump[-1500:])
            g.say("reopened window keeps the console")
        g.say("QEMU terminal window contract passed.")
        return 0
    finally:
        if qmp is not None:
            qmp.close()
        if monitor is not None:
            monitor.close()
        g.terminate(proc)


if __name__ == "__main__":
    try:
        raise SystemExit(main())
    except Exception as error:
        print("QEMU terminal window failed: %s" % error, file=sys.stderr)
        raise SystemExit(1)
