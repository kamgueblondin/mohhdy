#!/usr/bin/env python3
"""QEMU contract of Lot 125: true alignment and writing-mode start / end in
the guest web engine.

/web/lot125.html: without a safe keyword the alignment is true. In #c125
(100 x 40 at y 20, align-items: center) the 60 px high #a125 overflows 10
px on both sides: box 0 10 (Lot 124: 0 20, kept at the start); #b125
(align-self: end) overflows the start side: 20 0. #j125 (100 px at x 40,
justify-content: center) holds two unshrinkable 80 px items: #h125 at
x 10 and #i125 at 90, 30 px past each side (Lot 124: 40 and 120).
start / end follow the writing mode, flex-start / flex-end the flex axes:
in the wrap-reverse #w125 (50 x 60 at y 90) align-content: start packs
the lines at the top, #e125 at y 90 and #d125 at 110 (Lot 124: 120 and
140, packed at the bottom as flex-start); the same container with
align-content: flex-start (#x125) keeps them at the bottom. In the
row-reverse #v125, justify-content: start packs to the left: #r125 at x
20, #s125 at 0 (Lot 124: 80 and 60). The grid #m125 stretched to 40 by
the flex line #k125 stretches its auto row, so #n125 (height: 50%) is 20
high (Lot 124: 10). web-console reports the single "flex minimum prevents
fit" of #j125.
"""
from __future__ import print_function

import os
import subprocess
import sys
import time

HERE = os.path.dirname(os.path.abspath(__file__))
sys.path.insert(0, HERE)
LOGS = os.path.abspath(os.path.join(HERE, "..", "..", "test_logs"))
os.environ.setdefault("OSUI_GUI_LOG", os.path.join(LOGS, "qemu-lot125-serial.log"))
os.environ.setdefault("OSUI_GUI_ERR", os.path.join(LOGS, "qemu-lot125-stderr.log"))
os.environ.setdefault("OSUI_GUI_MON_SOCK", "/tmp/mohhdy-lot125-mon.sock")
os.environ.setdefault("OSUI_GUI_QMP_SOCK", "/tmp/mohhdy-lot125-qmp.sock")
os.environ.setdefault("OVERLAY_DISK", os.path.join(LOGS, "qemu-lot125-overlay.img"))
import test_qemu_osui_gui as g  # noqa: E402

PAGE = "/web/lot125.html"
# (tag, id): (box, color, bg or None, margin, padding, font100, lh100)
Z = ("#000000", None, "0 0 0 0", "0 0 0 0", 1600, 1000)
BOXES = [
    ("html", "p125", "0 0 640x260"), ("body", "q125", "0 0 640x260"),
    ("div", "c125", "0 20 100x40"), ("div", "a125", "0 10 20x60"), ("div", "b125", "20 0 20x60"),
    ("div", "j125", "40 80 100x10"), ("div", "h125", "10 80 80x10"), ("div", "i125", "90 80 80x10"),
    ("div", "w125", "0 90 50x60"), ("div", "d125", "0 110 30x10"), ("div", "e125", "0 90 30x20"),
    ("div", "x125", "0 150 50x60"), ("div", "f125", "0 200 30x10"), ("div", "g125", "0 180 30x20"),
    ("div", "v125", "0 210 100x10"), ("div", "r125", "20 210 20x10"), ("div", "s125", "0 210 20x10"),
    ("div", "k125", "0 220 100x40"), ("div", "m125", "0 220 50x40"), ("div", "n125", "0 220 50x20"),
]
# #c125 and #j125 carry their margins (margin-top, margin-left)
MARGIN = {"c125": "20 0 0 0", "j125": "20 0 0 40"}
WANT = [(tag, ident, box) + Z[:2] + (MARGIN.get(ident, Z[2]),) + Z[3:] for tag, ident, box in BOXES]
# what Lot 124 gave (safe alignment of overflowing items and lines, start
# / end treated as flex-start / flex-end, grid rows packed at the start)
NEVER = {"a125": " box 0 20 20x60 ", "b125": " box 20 20 20x60 ", "h125": " box 40 80 80x10 ",
         "i125": " box 120 80 80x10 ", "d125": " box 0 140 30x10 ", "e125": " box 0 120 30x20 ",
         "r125": " box 80 210 20x10 ", "s125": " box 60 210 20x10 ", "n125": " box 0 220 50x10 "}


def say(msg):
    print("[lot125] %s" % msg, flush=True)


def require(text, fragment, what):
    if fragment not in text:
        raise RuntimeError("%s: missing %r in serial output:\n%s" % (what, fragment, text[-2500:]))


def refuse(text, fragment, what):
    if fragment in text:
        raise RuntimeError("%s: unexpected %r in serial output:\n%s" % (what, fragment, text[-2500:]))


def inspect_line(text, tag, ident):
    for line in text.splitlines():
        if line.startswith("inspect %s#%s " % (tag, ident)):
            return line
    raise RuntimeError("no inspect line for #%s:\n%s" % (ident, text[-2500:]))


def check(monitor, proc):
    for tag, ident, box, color, bg, margin, padding, f100, lh100 in WANT:
        start = g.send_command_until(monitor, "web-inspect %s %s" % (PAGE, ident), "inspect ", proc)
        g.wait_for(proc, " lh100 ", g.CMD_TIMEOUT, start)
        line = inspect_line(g.normalized_log(g.log_text()[start:]), tag, ident)
        what = "#" + ident
        require(line, " box %s " % box, what)
        require(line, " color %s " % color, what)
        if bg:
            require(line, " bg %s " % bg, what)
        else:
            refuse(line, " bg ", what)
        require(line, " margin %s " % margin, what)
        require(line, " padding %s " % padding, what)
        require(line, " font100 %d lh100 %d" % (f100, lh100), what)
        if ident in NEVER:
            refuse(line, NEVER[ident], what)
        refuse(line, " canvas ", what)
        say("#%s box %s color %s margin %s padding %s" % (ident, box, color, margin, padding))
    start = g.send_command_until(monitor, "web-console %s" % PAGE, "web-console ok", proc)
    text = g.normalized_log(g.log_text()[start:])
    require(text, "web-console flex minimum prevents fit", "web-console")
    require(text, "web-console ok diag 1 ", "web-console")
    refuse(text, "rule refused", "web-console")
    refuse(text, "alignment refused", "web-console")


def main():
    os.makedirs(LOGS, exist_ok=True)
    for path in (g.LOG, g.QEMU_ERR, g.MON_SOCK):
        try:
            os.remove(path)
        except FileNotFoundError:
            pass
    g.prepare_test_disk()
    proc = None
    monitor = None
    t0 = time.monotonic()
    try:
        with open(g.QEMU_ERR, "w") as err:
            proc = subprocess.Popen([
                "qemu-system-i386", "-fw_cfg", "name=opt/mohhdy/ui-scale,string=4", "-cpu", "pentium3",
                "-kernel", g.KERNEL, "-initrd", g.INITRD, "-m", "1024M", "-display", "none", "-vga", "std",
                "-usb", "-device", "usb-tablet", "-serial", "file:" + g.LOG,
                "-monitor", "unix:%s,server,nowait" % g.MON_SOCK,
                "-machine", "type=pc,accel=tcg", "-no-reboot", "-no-shutdown",
            ] + g.qemu_disk_args(), cwd=g.ROOT, stdout=err, stderr=err)
            g.wait_for(proc, "[0m : ", g.BOOT_TIMEOUT)
            monitor = g.monitor_connect()
            check(monitor, proc)
        say("lot125 qemu ok in %.1fs" % (time.monotonic() - t0))
        return 0
    finally:
        if monitor is not None:
            monitor.close()
        g.terminate(proc)
        try:
            os.remove(g.MON_SOCK)
        except FileNotFoundError:
            pass


if __name__ == "__main__":
    try:
        sys.exit(main())
    except Exception as exc:  # noqa: BLE001
        say("FAIL: %s" % exc)
        sys.exit(1)
