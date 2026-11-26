#!/usr/bin/env python3
"""QEMU contract of Lot 127: self-start, self-end, left and right, and the
fit-content width of column flex items and grid items, in the guest web
engine.

/web/lot127.html: in the column #c127 (100 px at y 20) #a127 says
align-self: start and takes its content width, box 0 20 16x10 (Lot 126:
the full 100 px); the rtl #b127 says align-self: self-start, its own
start is the right: 84 30 16x10. In the 40 px column #w127 (x 40) #d127 (end) wraps
its 112 px text within 40 px: 40 50 40x30 (Lot 126: -32 50 112x30). The
rtl row #j127 says justify-content: left: #h127 at x 0 (Lot 126 refused
it: x 80). The grid #g127 puts its two 30 px columns on the right
(justify-content: right): #f127 (justify-self: left) at 40, the rtl
#k127 (self-end) at 70 (Lot 126: 0 and 30, full width). In #gi127 the
start item #r127 wraps within its 40 px area, 40x30 (Lot 126: 112x10).
The wrapping column #x127 sizes its lines from their widest item: #u127
and #v127 32 px wide, #y127 at x 32 (Lot 126: one 8 px character).
In the column #m127 (y 200) the 30 px wide #n127 (min-height: 0px, so
its flex base size alone sets its height) is measured at its own width:
two lines, 30x20 (Lot 126: 10 high, its text overflowed); #o127
(flex-basis: 0px) is held by its automatic minimum, measured at 30 px
too: 0 220 30x20 (Lot 126: 10 high).
web-console reports no diagnostic.
"""
from __future__ import print_function

import os
import subprocess
import sys
import time

HERE = os.path.dirname(os.path.abspath(__file__))
sys.path.insert(0, HERE)
LOGS = os.path.abspath(os.path.join(HERE, "..", "..", "test_logs"))
os.environ.setdefault("OSUI_GUI_LOG", os.path.join(LOGS, "qemu-lot127-serial.log"))
os.environ.setdefault("OSUI_GUI_ERR", os.path.join(LOGS, "qemu-lot127-stderr.log"))
os.environ.setdefault("OSUI_GUI_MON_SOCK", "/tmp/mohhdy-lot127-mon.sock")
os.environ.setdefault("OSUI_GUI_QMP_SOCK", "/tmp/mohhdy-lot127-qmp.sock")
os.environ.setdefault("OVERLAY_DISK", os.path.join(LOGS, "qemu-lot127-overlay.img"))
import test_qemu_osui_gui as g  # noqa: E402

PAGE = "/web/lot127.html"
# (tag, id): (box, color, bg or None, margin, padding, font100, lh100)
Z = ("#000000", None, "0 0 0 0", "0 0 0 0", 1600, 1000)
BOXES = [
    ("html", "p127", "0 0 640x240"), ("body", "q127", "0 0 640x240"),
    ("div", "c127", "0 20 100x20"), ("div", "a127", "0 20 16x10"), ("div", "b127", "84 30 16x10"),
    ("div", "w127", "40 50 40x30"), ("div", "d127", "40 50 40x30"),
    ("div", "j127", "0 90 100x10"), ("div", "h127", "0 90 20x10"),
    ("div", "g127", "0 110 100x10"), ("div", "f127", "40 110 8x10"), ("div", "k127", "70 110 8x10"),
    ("div", "gi127", "0 130 40x30"), ("div", "r127", "0 130 40x30"),
    ("div", "x127", "0 170 100x20"), ("div", "u127", "0 170 32x10"), ("div", "v127", "0 180 32x10"),
    ("div", "y127", "32 170 48x10"), ("div", "m127", "0 200 100x40"), ("div", "n127", "0 200 30x20"),
    ("div", "o127", "0 220 30x20"),
]
# the containers carry their margins (margin-top, margin-left)
MARGIN = {"c127": "20 0 0 0", "w127": "10 0 0 40", "j127": "10 0 0 0", "g127": "10 0 0 0",
          "gi127": "10 0 0 0", "x127": "10 0 0 0", "m127": "10 0 0 0"}
WANT = [(tag, ident, box) + Z[:2] + (MARGIN.get(ident, Z[2]),) + Z[3:] for tag, ident, box in BOXES]
# what Lot 126 gave (start items at the full width, end items at an
# uncapped max-content, self-* / left / right refused, wrapping columns one
# character wide, a fixed-width column item measured at the container)
NEVER = {"a127": " box 0 20 100x10 ", "b127": " box 0 30 100x10 ", "d127": " box -32 50 112x30 ",
         "h127": " box 80 90 20x10 ", "f127": " box 0 110 30x10 ", "k127": " box 30 110 30x10 ",
         "r127": " box 0 130 112x10 ", "u127": " box 0 150 8x10 ", "v127": " box 0 160 8x10 ",
         "y127": " box 8 150 8x10 ", "n127": " box 0 180 30x10 ",
         "o127": " box 0 190 30x10 "}


def say(msg):
    print("[lot127] %s" % msg, flush=True)


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
        say("lot127 qemu ok in %.1fs" % (time.monotonic() - t0))
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
