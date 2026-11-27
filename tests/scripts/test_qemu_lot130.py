#!/usr/bin/env python3
"""QEMU contract of Lot 130: a style pass restarts from the full initial
style, and a box narrower than its padding no longer stops the guest.

/web/lot130.html: #y130 is a container (inline-size) inside the container
#x130. The first rule (min-width 300px on #x130) makes #y130 100 px wide
once the page is laid out; the second (min-width 200px, read against
#y130) matched #y130 at its first width, 640, so the style pass that the
first one triggers gave #z1130 margin 30px, padding 5px and a red
background, #z2130 a 4 px border and max-width 50%, and #z3130 a column
with an 8 px gap. At 100 px the second rule no longer matches, and the
settled page has none of it: #z1130 0 0 100x10, #z2130 0 10 100x10,
#z3130 0 20 100x10 with #k1130 at 0 20 and #k2130 at 20 20 (a row).
Lot 129 kept every one of those values: #z1130 at 30 30 (40x20, red),
#z2130 58x18 at 0 80, the two items of #z3130 stacked at 98 and 107.
#i130, an img (a 40 px placeholder; 24x10 with its alt text) with
padding 0 25px, is laid out and painted: Lot 129 stopped the guest with
a divide error in the scrollbar code. web-console reports no diagnostic.
"""
from __future__ import print_function

import os
import subprocess
import sys
import time

HERE = os.path.dirname(os.path.abspath(__file__))
sys.path.insert(0, HERE)
LOGS = os.path.abspath(os.path.join(HERE, "..", "..", "test_logs"))
os.environ.setdefault("OSUI_GUI_LOG", os.path.join(LOGS, "qemu-lot130-serial.log"))
os.environ.setdefault("OSUI_GUI_ERR", os.path.join(LOGS, "qemu-lot130-stderr.log"))
os.environ.setdefault("OSUI_GUI_MON_SOCK", "/tmp/mohhdy-lot130-mon.sock")
os.environ.setdefault("OSUI_GUI_QMP_SOCK", "/tmp/mohhdy-lot130-qmp.sock")
os.environ.setdefault("OVERLAY_DISK", os.path.join(LOGS, "qemu-lot130-overlay.img"))
import test_qemu_osui_gui as g  # noqa: E402

PAGE = "/web/lot130.html"
# (tag, id): (box, color, bg or None, margin, padding, font100, lh100)
Z = ("#000000", None, "0 0 0 0", "0 0 0 0", 1600, 1000)
BOXES = [
    ("html", "p130", "0 0 640x40"), ("body", "q130", "0 0 640x40"),
    ("div", "x130", "0 0 640x30"), ("div", "y130", "0 0 100x30"),
    ("div", "z1130", "0 0 100x10"), ("div", "z2130", "0 10 100x10"), ("div", "z3130", "0 20 100x10"),
    ("div", "k1130", "0 20 20x10"), ("div", "k2130", "20 20 20x10"),
    ("img", "i130", "0 30 24x10"),
]
# the padding of the img is its own
PADDING = {"i130": "0 25 0 25"}
WANT = [(tag, ident, box) + Z[:2] + (Z[2], PADDING.get(ident, Z[3])) + Z[4:]
        for tag, ident, box in BOXES]
# what Lot 129 gave (the first pass kept)
NEVER = {"z1130": " box 30 30 40x20 ", "z2130": " box 0 80 58x18 ", "z3130": " box 0 98 100x10 ",
         "k1130": " box 0 98 20x1 ", "k2130": " box 0 107 20x1 "}


def say(msg):
    print("[lot130] %s" % msg, flush=True)


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
    require(text, "web-console ok diag 0 ", "web-console")
    for refused in ("rule refused", "container query unsettled", "gradient refused", "image"):
        refuse(text, refused, "web-console")


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
        say("lot130 qemu ok in %.1fs" % (time.monotonic() - t0))
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
