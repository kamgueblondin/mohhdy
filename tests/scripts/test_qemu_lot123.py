#!/usr/bin/env python3
"""QEMU contract of Lot 123: plain flex rows in the guest web engine.

/web/lot123.html holds plain rows (no flex property, no modern
alignment), now on the flex algorithm. #a123 ("aaaa") and #b123 ("b",
"b") take their content widths 32 and 8 (Lot 122: equal slots of 150) and
stretch to their line, 20 (align-items normal; Lot 122: #a123 10 high).
#c123 (width: 150px) shrinks to its 100 px row (Lot 122: stayed 150).
#d123 and #e123 ("aaaaaaaa", "bbbbbbbb") stop at their min-content widths
64: the row overflows and web-console reports "flex minimum prevents
fit", diag 1 (Lot 122: 50 and 50). In the 30 px high row #u123, #f123
stretches to 30 and #g123 (height: 50%) resolves to 15. #v123 says
align-items: normal and justify-content: normal (Lot 122: "alignment
refused" twice): both items 20 high. #x123, a plain row in the column
#w123, is measured on the same bases: #m123 92 wide on two lines, #n123
stretched to 20, #z123 at y 110 (Lot 122: 50 / 50 slots, y 130).
"""
from __future__ import print_function

import os
import subprocess
import sys
import time

HERE = os.path.dirname(os.path.abspath(__file__))
sys.path.insert(0, HERE)
LOGS = os.path.abspath(os.path.join(HERE, "..", "..", "test_logs"))
os.environ.setdefault("OSUI_GUI_LOG", os.path.join(LOGS, "qemu-lot123-serial.log"))
os.environ.setdefault("OSUI_GUI_ERR", os.path.join(LOGS, "qemu-lot123-stderr.log"))
os.environ.setdefault("OSUI_GUI_MON_SOCK", "/tmp/mohhdy-lot123-mon.sock")
os.environ.setdefault("OSUI_GUI_QMP_SOCK", "/tmp/mohhdy-lot123-qmp.sock")
os.environ.setdefault("OVERLAY_DISK", os.path.join(LOGS, "qemu-lot123-overlay.img"))
import test_qemu_osui_gui as g  # noqa: E402

PAGE = "/web/lot123.html"
# (tag, id): (box, color, bg or None, margin, padding, font100, lh100)
Z = ("#000000", None, "0 0 0 0", "0 0 0 0", 1600, 1000)
BOXES = [
    ("html", "p123", "0 0 640x120"), ("body", "q123", "0 0 640x120"),
    ("div", "r123", "0 0 300x20"), ("div", "a123", "0 0 32x20"), ("div", "b123", "32 0 8x20"),
    ("div", "s123", "0 20 100x10"), ("div", "c123", "0 20 100x10"),
    ("div", "t123", "0 30 100x10"), ("div", "d123", "0 30 64x10"), ("div", "e123", "64 30 64x10"),
    ("div", "u123", "0 40 300x30"), ("div", "f123", "0 40 8x30"), ("div", "g123", "8 40 8x15"),
    ("div", "v123", "0 70 300x20"), ("div", "h123", "0 70 8x20"), ("div", "k123", "8 70 8x20"),
    ("div", "w123", "0 90 100x30"), ("div", "x123", "0 90 100x20"), ("div", "m123", "0 90 92x20"),
    ("div", "n123", "92 90 8x20"), ("div", "z123", "0 110 100x10"),
]
WANT = [(tag, ident, box) + Z for tag, ident, box in BOXES]
# what Lot 122 gave (equal slots in plain rows, no shrink there, no default
# stretch of row items, normal refused)
NEVER = {"a123": " box 0 0 150x10 ", "c123": " box 0 20 150x10 ", "d123": " box 0 30 50x10 ",
         "f123": " box 0 40 8x10 ", "h123": " box 0 70 8x10 ", "m123": " box 0 90 50x40 ",
         "z123": " box 0 130 100x10 "}


def say(msg):
    print("[lot123] %s" % msg, flush=True)


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
        say("lot123 qemu ok in %.1fs" % (time.monotonic() - t0))
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
