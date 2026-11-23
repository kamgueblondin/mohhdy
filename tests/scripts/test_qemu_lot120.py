#!/usr/bin/env python3
"""Lot 120 in QEMU, offline: the root box and the initial containing
block, container style queries and the hue-only angle units in the guest
engine.

One boot at ui-scale 1x (fw_cfg 4 quarters), no network and no guest clock:
"web-inspect /web/lot120.html ID" lays the local page out in the guest's
engine (640 x 480 viewport) and prints each box, its color, background,
margins, padding, font size and used line height, and " canvas #rrggbb"
on the element whose background color paints the canvas.

html#h120 is height: 50% of the initial containing block (480 px): a 240
px content box, 4 px padding and 2 px border, 640 x 252 (Lot 119: an
indefinite percentage, 640 x 42). body#y120 is height: 100% of it, 628 x
240 at 6 6, and as html has no background its #e0f0ff paints the canvas.
#a120 (absolute, bottom: 0, right: 0, 20 x 10) sits at 620 470 in the
viewport, not at the bottom of the document box (Lot 119: 620 32); #b120
is 50% x 25% of the viewport, 320 x 120. #d120 gets the rules of
@container theme style(--theme: dark): 100 px on #008000; #e120 those of
style((--theme: dark) and (--size: 2)), 50 px, and not the red of
style(--theme: light). #f120 keeps color #0000ff: rgb(10deg 20 30) is
refused (Lot 119 read it as #0a141e) while hsl(240deg 100% 50%) is
#0000ff. #c120 sets --theme and --size in its style attribute: the guest
pipeline resolves the custom properties of the style sheet before the
engine (webrt_css_resolve_vars, Lot 7). web-console reports "container query refused" (style(color: red))
and "color refused", diag 2.
"""
from __future__ import print_function

import os
import subprocess
import sys
import time

HERE = os.path.dirname(os.path.abspath(__file__))
sys.path.insert(0, HERE)
LOGS = os.path.abspath(os.path.join(HERE, "..", "..", "test_logs"))
os.environ.setdefault("OSUI_GUI_LOG", os.path.join(LOGS, "qemu-lot120-serial.log"))
os.environ.setdefault("OSUI_GUI_ERR", os.path.join(LOGS, "qemu-lot120-stderr.log"))
os.environ.setdefault("OSUI_GUI_MON_SOCK", "/tmp/mohhdy-lot120-mon.sock")
os.environ.setdefault("OSUI_GUI_QMP_SOCK", "/tmp/mohhdy-lot120-qmp.sock")
os.environ.setdefault("OVERLAY_DISK", os.path.join(LOGS, "qemu-lot120-overlay.img"))
import test_qemu_osui_gui as g  # noqa: E402

PAGE = "/web/lot120.html"
# (tag, id): (box, color, bg or None, margin, padding, font100, lh100)
WANT = [
    ("html", "h120", "0 0 640x252", "#000000", None, "0 0 0 0", "4 4 4 4", 1600, 1000),
    ("body", "y120", "6 6 628x240", "#000000", "#e0f0ff", "0 0 0 0", "0 0 0 0", 1600, 1000),
    ("div", "a120", "620 470 20x10", "#000000", "#800000", "0 0 0 0", "0 0 0 0", 1600, 1000),
    ("div", "b120", "0 0 320x120", "#000000", None, "0 0 0 0", "0 0 0 0", 1600, 1000),
    ("div", "c120", "6 6 628x20", "#000000", None, "0 0 0 0", "0 0 0 0", 1600, 1000),
    ("div", "d120", "6 6 100x10", "#000000", "#008000", "0 0 0 0", "0 0 0 0", 1600, 1000),
    ("div", "e120", "6 16 50x10", "#000000", None, "0 0 0 0", "0 0 0 0", 1600, 1000),
    ("div", "f120", "6 26 628x10", "#0000ff", "#0000ff", "0 0 0 0", "0 0 0 0", 1600, 1000),
]
# the element whose background paints the canvas, and its color
CANVAS = ("y120", "#e0f0ff")
# what Lot 119 gave (indefinite root percentage, the document box as the
# block of absolute boxes, style() refused, deg read as a number)
NEVER = {"h120": " box 0 0 640x42 ", "a120": " box 620 32 ", "b120": " 320x10 ",
         "d120": " box 6 6 628x10 ", "f120": " color #0a141e "}


def say(msg):
    print("[lot120] %s" % msg, flush=True)


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
        if ident == CANVAS[0]:
            require(line, " canvas %s" % CANVAS[1], what)
        else:
            refuse(line, " canvas ", what)
        say("#%s box %s color %s margin %s padding %s" % (ident, box, color, margin, padding))
    start = g.send_command_until(monitor, "web-console %s" % PAGE, "web-console ok", proc)
    text = g.normalized_log(g.log_text()[start:])
    require(text, "web-console container query refused", "web-console")
    require(text, "web-console color refused", "web-console")
    require(text, "web-console ok diag 2 ", "web-console")
    refuse(text, "rule refused", "web-console")


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
        say("lot120 qemu ok in %.1fs" % (time.monotonic() - t0))
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
