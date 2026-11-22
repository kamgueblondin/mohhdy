#!/usr/bin/env python3
"""Lot 117 in QEMU, offline: font-relative lengths in the guest engine.

One boot at ui-scale 1x (fw_cfg 4 quarters), no network and no guest clock: "web-inspect
/web/lot117.html ID" lays the local page out in the guest's engine (640 x
480) and prints each box, its margins, padding, border, its font size and
its used line height in hundredths of a px (font100, lh100).

html's font-size is 20px, so rem is 20 and rlh 13 (normal at 20 px: q 5,
10 rows of 5/4 px). #a117 margin-left: 2em before font-size: 15px is 30
px (its own font; Lot 116: 20, em was 10 px). #b117 at 13.5 px with
line-height 1.5 is 20.25 px a line: four lines 81 px (Lot 116: 14 px
fonts, 21 px lines, 84). #c117 width calc(2.5em + 1rem) is 70 (Lot 116:
refused, auto 640). #d117 padding-left 2ch is 20 and height 3ex 18 (ch 10,
ex 6 at q 5). #e117 line-clamp: 2 of 20.6 px lines is 41 (the carried
sum; Lot 116: 40). #f117 font-size calc(1em + 4px) is 24 px and its
height 1lh 15 (normal at 24 px). #g117 line-height 2rlh is 26. #h117
border 0.25em is 5, width 4cap 36 (capitals 7 rows at q 5: 9) and height
1ic 20: a 46 x 30 box. #i117 margin-top 1rlh is 13, font-size: smaller
16.67 px, line-height 2 33.34 px, padding-top 0.5lh 17: 50 px high.
"""
from __future__ import print_function

import os
import subprocess
import sys
import time

HERE = os.path.dirname(os.path.abspath(__file__))
sys.path.insert(0, HERE)
LOGS = os.path.abspath(os.path.join(HERE, "..", "..", "test_logs"))
os.environ.setdefault("OSUI_GUI_LOG", os.path.join(LOGS, "qemu-lot117-serial.log"))
os.environ.setdefault("OSUI_GUI_ERR", os.path.join(LOGS, "qemu-lot117-stderr.log"))
os.environ.setdefault("OSUI_GUI_MON_SOCK", "/tmp/mohhdy-lot117-mon.sock")
os.environ.setdefault("OSUI_GUI_QMP_SOCK", "/tmp/mohhdy-lot117-qmp.sock")
os.environ.setdefault("OVERLAY_DISK", os.path.join(LOGS, "qemu-lot117-overlay.img"))
import test_qemu_osui_gui as g  # noqa: E402

PAGE = "/web/lot117.html"
# id: (box, margin, padding, border, font100, lh100)
WANT = [
    ("a117", "30 0 610x10", "0 0 0 30", "0 0 0 0", 0, 1500, 1000),
    ("b117", "0 10 640x81", "0 0 0 0", "0 0 0 0", 0, 1350, 2025),
    ("c117", "0 91 70x8", "0 0 0 0", "0 0 0 0", 0, 2000, 1300),
    ("d117", "0 99 640x18", "0 0 0 0", "0 0 0 20", 0, 2000, 1300),
    ("e117", "0 117 640x41", "0 0 0 0", "0 0 0 0", 0, 2000, 2060),
    ("f117", "0 158 640x15", "0 0 0 0", "0 0 0 0", 0, 2400, 1500),
    ("g117", "0 173 640x26", "0 0 0 0", "0 0 0 0", 0, 2000, 2600),
    ("h117", "0 199 46x30", "0 0 0 0", "0 0 0 0", 5, 2000, 1300),
    ("i117", "0 242 640x50", "13 0 0 0", "17 0 0 0", 0, 1667, 3334),
]
# what Lot 116 gave (em 10 px, whole px fonts, the clamp cut)
NEVER = {"a117": " box 20 0 ", "b117": "x84 ", "c117": " box 0 91 640x", "e117": "x40 ", "i117": " lh100 3400"}


def say(msg):
    print("[lot117] %s" % msg, flush=True)


def require(text, fragment, what):
    if fragment not in text:
        raise RuntimeError("%s: missing %r in serial output:\n%s" % (what, fragment, text[-2500:]))


def refuse(text, fragment, what):
    if fragment in text:
        raise RuntimeError("%s: unexpected %r in serial output:\n%s" % (what, fragment, text[-2500:]))


def inspect_line(text, ident):
    for line in text.splitlines():
        if line.startswith("inspect div#" + ident + " "):
            return line
    raise RuntimeError("no inspect line for #%s:\n%s" % (ident, text[-2500:]))


def check(monitor, proc):
    for ident, box, margin, padding, border, f100, lh100 in WANT:
        start = g.send_command_until(monitor, "web-inspect %s %s" % (PAGE, ident), "inspect ", proc)
        g.wait_for(proc, " lh100 ", g.CMD_TIMEOUT, start)
        line = inspect_line(g.normalized_log(g.log_text()[start:]), ident)
        what = "#" + ident
        require(line, " box %s " % box, what)
        require(line, " margin %s " % margin, what)
        require(line, " padding %s " % padding, what)
        require(line, " border %d " % border, what)
        require(line, " font100 %d lh100 %d" % (f100, lh100), what)
        if ident in NEVER:
            refuse(line, NEVER[ident], what)
        say("#%s box %s font100 %d lh100 %d" % (ident, box, f100, lh100))
    start = g.send_command_until(monitor, "web-console %s" % PAGE, "web-console ok", proc)
    text = g.normalized_log(g.log_text()[start:])
    require(text, "web-console line clamp", "web-console")
    refuse(text, "refused", "web-console")


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
        say("lot117 qemu ok in %.1fs" % (time.monotonic() - t0))
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
