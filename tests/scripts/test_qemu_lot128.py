#!/usr/bin/env python3
"""QEMU contract of Lot 128: fractional flex-grow, flex-shrink and flex
factors in the guest web engine.

/web/lot128.html (rows 100 px wide, items 10 px high): in #g128
(y 20) #a128 and #b128 (width 0, flex-grow 0.5 and 1.5) share the 100 px
one to three: 0 20 25x10 and 25 20 75x10 (Lot 127 refused both factors,
0 wide). In #s128 (y 40) #c128 and #d128 say flex: 0.25 0 0 and
flex: .5 0 0, a sum below 1, so only 75 of the 100 px are used: 25 and
50 px (Lot 127: flex refused, 0 wide). In #k128 (y 60) #e128 (flex 0.5,
max-width 10px) freezes at 10 and #f128 (flex 0.25) keeps a quarter of
the initial free space, 25 px at x 10 (Lot 127: 0 wide). The 8 px row
#t128 (y 80) gives 8 * 0.9, 7 px, to three 0.3 items, 2 each and the
last pixel to the first: 3, 2, 2 (Lot 127: 0 wide). In #h128 (y 100)
two 80 px items with flex-shrink 0.5 and 1.5 lose the 60 px overflow
one to three: 65 and 35 px (Lot 127: 50 and 50). In #o128 (y 120)
#u128 (shrink 0.25, min-width 75px) stops at 75 and #v128 (shrink 0.25)
gives a quarter of the overflow, 15 px: 75 120 65x10 (Lot 127: 25 px).
In #x128 (y 140) #y128 (shrink 0.8, min-width 19px) freezes at 19 and
the 200 px #w128 (shrink 0.1), restarting from its basis, gives a tenth
of the 120 px overflow: 188 wide (Lot 127: 81; without the restart 140).
In the 50 px #z128 (y 160) the 0 basis #za128 (shrink 0.5) keeps its
factor in the sum, so #zb128 (100 px basis, shrink 0.5) takes the whole
overflow: 50 wide (Lot 127: flex refused, 0 wide).
web-console reports no diagnostic (an intended overflow is not one).
"""
from __future__ import print_function

import os
import subprocess
import sys
import time

HERE = os.path.dirname(os.path.abspath(__file__))
sys.path.insert(0, HERE)
LOGS = os.path.abspath(os.path.join(HERE, "..", "..", "test_logs"))
os.environ.setdefault("OSUI_GUI_LOG", os.path.join(LOGS, "qemu-lot128-serial.log"))
os.environ.setdefault("OSUI_GUI_ERR", os.path.join(LOGS, "qemu-lot128-stderr.log"))
os.environ.setdefault("OSUI_GUI_MON_SOCK", "/tmp/mohhdy-lot128-mon.sock")
os.environ.setdefault("OSUI_GUI_QMP_SOCK", "/tmp/mohhdy-lot128-qmp.sock")
os.environ.setdefault("OVERLAY_DISK", os.path.join(LOGS, "qemu-lot128-overlay.img"))
import test_qemu_osui_gui as g  # noqa: E402

PAGE = "/web/lot128.html"
# (tag, id): (box, color, bg or None, margin, padding, font100, lh100)
Z = ("#000000", None, "0 0 0 0", "0 0 0 0", 1600, 1000)
BOXES = [
    ("html", "p128", "0 0 640x170"), ("body", "q128", "0 0 640x170"),
    ("div", "g128", "0 20 100x10"), ("div", "a128", "0 20 25x10"), ("div", "b128", "25 20 75x10"),
    ("div", "s128", "0 40 100x10"), ("div", "c128", "0 40 25x10"), ("div", "d128", "25 40 50x10"),
    ("div", "k128", "0 60 100x10"), ("div", "e128", "0 60 10x10"), ("div", "f128", "10 60 25x10"),
    ("div", "t128", "0 80 8x10"), ("div", "l128", "0 80 3x10"), ("div", "m128", "3 80 2x10"),
    ("div", "n128", "5 80 2x10"),
    ("div", "h128", "0 100 100x10"), ("div", "i128", "0 100 65x10"), ("div", "j128", "65 100 35x10"),
    ("div", "o128", "0 120 100x10"), ("div", "u128", "0 120 75x10"), ("div", "v128", "75 120 65x10"),
    ("div", "x128", "0 140 100x10"), ("div", "w128", "0 140 188x10"), ("div", "y128", "188 140 19x10"),
    ("div", "z128", "0 160 50x10"), ("div", "za128", "0 160 0x10"), ("div", "zb128", "0 160 50x10"),
]
# the rows carry their top margins
MARGIN = {"g128": "20 0 0 0", "s128": "10 0 0 0", "k128": "10 0 0 0", "t128": "10 0 0 0",
          "h128": "10 0 0 0", "o128": "10 0 0 0", "x128": "10 0 0 0", "z128": "10 0 0 0"}
WANT = [(tag, ident, box) + Z[:2] + (MARGIN.get(ident, Z[2]),) + Z[3:] for tag, ident, box in BOXES]
# what Lot 127 gave (fractional factors refused: growers 0 wide, shrinkers
# at the default factor 1)
NEVER = {"a128": " box 0 20 0x10 ", "b128": " box 0 20 0x10 ", "c128": " box 0 40 0x10 ",
         "d128": " box 0 40 0x10 ", "e128": " box 0 60 0x10 ", "f128": " box 0 60 0x10 ",
         "l128": " box 0 80 0x10 ", "m128": " box 0 80 0x10 ", "n128": " box 0 80 0x10 ",
         "i128": " box 0 100 50x10 ", "j128": " box 50 100 50x10 ", "v128": " box 75 120 25x10 ",
         "w128": " box 0 140 81x10 ", "y128": " box 81 140 19x10 ", "zb128": " box 0 160 0x10 "}


def say(msg):
    print("[lot128] %s" % msg, flush=True)


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
    refuse(text, "flex refused", "web-console")
    refuse(text, "flex-grow refused", "web-console")
    refuse(text, "flex-shrink refused", "web-console")
    refuse(text, "flex minimum prevents fit", "web-console")


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
        say("lot128 qemu ok in %.1fs" % (time.monotonic() - t0))
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
