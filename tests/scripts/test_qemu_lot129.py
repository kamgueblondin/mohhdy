#!/usr/bin/env python3
"""QEMU contract of Lot 129: the CSS-wide keywords inherit, initial,
unset and revert on the layout properties of the guest web engine.

/web/lot129.html (rows 100 px wide, items 10 px high): in #g129 (y 20,
flex: 3 1 0) #a129 says flex: inherit, so 3 1 0 against #b129's 1 1 0:
0 20 75x10 and 75 20 25x10 (Lot 128 refused flex: inherit, #a129 0 wide).
In the column #s129 (y 40) #c129 says flex-direction: inherit, so its two
20x10 items stack: 0 40 and 0 50, 20 high (Lot 128: a row, the second at
x 20). In #k129 (y 70, column-gap 10px) #e129 says column-gap: inherit:
its second item is at x 30 (Lot 128: 20). In #h129 (y 90, 60 px,
border-box) #i129 says width: inherit and box-sizing: inherit with a
5 px padding and height 10px: 0 90 60x10 (Lot 128: width: inherit
ignored and box-sizing refused, 40x20). In #t129 (y 110) #l129 says
flex: 1 0 0 then flex: unset (0 1 auto) with width 20px, so #m129
(flex 1 0 0) takes the other 80 px: 0 110 20x10 and 20 110 80x10
(Lot 128: unset refused, 50 and 50). In #o129 (y 130, justify-content
center) #u129 says justify-content: inherit: its 20 px item is at x 40
(Lot 128: 0). In #x129 (y 150) #w129 says max-width 30px then
max-width: revert (none): 100 wide (Lot 128: 30). In #z129 (y 170,
order 1) #za129 says order: inherit, so it follows #zb129: x 20 and 0
(Lot 128: order refused, 0 and 20).
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
os.environ.setdefault("OSUI_GUI_LOG", os.path.join(LOGS, "qemu-lot129-serial.log"))
os.environ.setdefault("OSUI_GUI_ERR", os.path.join(LOGS, "qemu-lot129-stderr.log"))
os.environ.setdefault("OSUI_GUI_MON_SOCK", "/tmp/mohhdy-lot129-mon.sock")
os.environ.setdefault("OSUI_GUI_QMP_SOCK", "/tmp/mohhdy-lot129-qmp.sock")
os.environ.setdefault("OVERLAY_DISK", os.path.join(LOGS, "qemu-lot129-overlay.img"))
import test_qemu_osui_gui as g  # noqa: E402

PAGE = "/web/lot129.html"
# (tag, id): (box, color, bg or None, margin, padding, font100, lh100)
Z = ("#000000", None, "0 0 0 0", "0 0 0 0", 1600, 1000)
BOXES = [
    ("html", "p129", "0 0 640x180"), ("body", "q129", "0 0 640x180"),
    ("div", "g129", "0 20 100x10"), ("div", "a129", "0 20 75x10"), ("div", "b129", "75 20 25x10"),
    ("div", "s129", "0 40 100x20"), ("div", "c129", "0 40 100x20"), ("div", "c1129", "0 40 20x10"),
    ("div", "c2129", "0 50 20x10"),
    ("div", "k129", "0 70 100x10"), ("div", "e129", "0 70 100x10"), ("div", "e1129", "0 70 20x10"),
    ("div", "e2129", "30 70 20x10"),
    ("div", "h129", "0 90 60x10"), ("div", "i129", "0 90 60x10"),
    ("div", "t129", "0 110 100x10"), ("div", "l129", "0 110 20x10"), ("div", "m129", "20 110 80x10"),
    ("div", "o129", "0 130 100x10"), ("div", "u129", "0 130 100x10"), ("div", "v129", "40 130 20x10"),
    ("div", "x129", "0 150 100x10"), ("div", "w129", "0 150 100x10"),
    ("div", "z129", "0 170 100x10"), ("div", "za129", "20 170 20x10"), ("div", "zb129", "0 170 20x10"),
]
# the rows carry their top margins, #i129 its padding
MARGIN = {"g129": "20 0 0 0", "s129": "10 0 0 0", "k129": "10 0 0 0", "h129": "10 0 0 0",
          "t129": "10 0 0 0", "o129": "10 0 0 0", "x129": "10 0 0 0", "z129": "10 0 0 0"}
PADDING = {"i129": "5 5 5 5"}
WANT = [(tag, ident, box) + Z[:2] + (MARGIN.get(ident, Z[2]), PADDING.get(ident, Z[3])) + Z[4:]
        for tag, ident, box in BOXES]
# what Lot 128 gave (the keywords refused or ignored)
NEVER = {"a129": " box 0 20 0x10 ", "b129": " box 0 20 100x10 ", "s129": " box 0 40 100x10 ",
         "c2129": " box 20 40 20x10 ", "e2129": " box 20 60 20x10 ", "h129": " box 0 80 60x20 ",
         "i129": " box 0 80 40x20 ", "l129": " box 0 110 50x10 ", "m129": " box 50 110 50x10 ",
         "v129": " box 0 130 20x10 ", "w129": " box 0 150 30x10 ", "za129": " box 0 170 20x10 ",
         "zb129": " box 20 170 20x10 "}


def say(msg):
    print("[lot129] %s" % msg, flush=True)


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
    for refused in ("rule refused", "flex refused", "flex-direction refused", "axis gap refused",
                    "box sizing refused", "alignment refused", "width constraint refused", "order refused"):
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
        say("lot129 qemu ok in %.1fs" % (time.monotonic() - t0))
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
