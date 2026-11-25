#!/usr/bin/env python3
"""QEMU contract of Lot 122: row flex items in the guest web engine.

/web/lot122.html holds rows on the flex algorithm. #a122 and #b122
(flex-grow: 1, "aaaa" and "b") start from their content (32 and 8 px)
and share the 260 px left: 162 and 138 (Lot 121: 150 and 150, auto bases
of 0). #d122 next to a flex-basis: 50px item keeps its content width, 8
(Lot 121: 0). In a 100 px row, #e122 ("aaaaaaaa") cannot shrink below its
min-content width 64 (automatic minimum, min-width auto) and #f122 ("bb bb
bb bb", flex-shrink: 1) shrinks to 36 on four lines (Lot 121: both 0
wide). #g122 and #h122 (flex: 1 1 0) are frozen at their min-content
widths 96 and 8: the row overflows and web-console reports "flex minimum
prevents fit", diag 1. The wrapping row #v122 breaks on the content
bases: #i122 alone on line 1 (100), #j122 and #k122 share line 2 (78, 22).
#x122, a row with justify-content inside the column #w122, is measured
with the same bases and minimums: #y122 92 wide on two lines, #z122 at
y 110.
Lot 123: row items stretch to their line by default, so .n122 and #x122
say align-items: flex-start to keep the content heights checked here.
"""
from __future__ import print_function

import os
import subprocess
import sys
import time

HERE = os.path.dirname(os.path.abspath(__file__))
sys.path.insert(0, HERE)
LOGS = os.path.abspath(os.path.join(HERE, "..", "..", "test_logs"))
os.environ.setdefault("OSUI_GUI_LOG", os.path.join(LOGS, "qemu-lot122-serial.log"))
os.environ.setdefault("OSUI_GUI_ERR", os.path.join(LOGS, "qemu-lot122-stderr.log"))
os.environ.setdefault("OSUI_GUI_MON_SOCK", "/tmp/mohhdy-lot122-mon.sock")
os.environ.setdefault("OSUI_GUI_QMP_SOCK", "/tmp/mohhdy-lot122-qmp.sock")
os.environ.setdefault("OVERLAY_DISK", os.path.join(LOGS, "qemu-lot122-overlay.img"))
import test_qemu_osui_gui as g  # noqa: E402

PAGE = "/web/lot122.html"
# (tag, id): (box, color, bg or None, margin, padding, font100, lh100)
Z = ("#000000", None, "0 0 0 0", "0 0 0 0", 1600, 1000)
BOXES = [
    ("html", "p122", "0 0 640x120"), ("body", "q122", "0 0 640x120"),
    ("div", "r122", "0 0 300x10"), ("div", "a122", "0 0 162x10"), ("div", "b122", "162 0 138x10"),
    ("div", "s122", "0 10 300x10"), ("div", "c122", "0 10 50x10"), ("div", "d122", "50 10 8x10"),
    ("div", "t122", "0 20 100x40"), ("div", "e122", "0 20 64x10"), ("div", "f122", "64 20 36x40"),
    ("div", "u122", "0 60 100x10"), ("div", "g122", "0 60 96x10"), ("div", "h122", "96 60 8x10"),
    ("div", "v122", "0 70 100x20"), ("div", "i122", "0 70 100x10"), ("div", "j122", "0 80 78x10"),
    ("div", "k122", "78 80 22x10"),
    ("div", "w122", "0 90 100x30"), ("div", "x122", "0 90 100x20"), ("div", "y122", "0 90 92x20"),
    ("div", "o122", "92 90 8x10"), ("div", "z122", "0 110 100x10"),
]
WANT = [(tag, ident, box) + Z for tag, ident, box in BOXES]
# what Lot 121 gave (row auto bases of 0 on the flex path, no shrink without
# flex-basis or flex-shrink, no automatic minimum in rows)
NEVER = {"a122": " box 0 0 150x10 ", "d122": " box 50 10 0x10 ", "e122": " box 0 20 0x10 ",
         "f122": " box 0 20 0x40 ", "g122": " box 0 60 50x10 ", "i122": " box 0 70 34x10 ",
         "y122": " box 0 80 95x20 "}


def say(msg):
    print("[lot122] %s" % msg, flush=True)


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
        say("lot122 qemu ok in %.1fs" % (time.monotonic() - t0))
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
