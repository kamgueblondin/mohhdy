#!/usr/bin/env python3
"""QEMU contract of Lot 126: safe and unsafe alignment, true alignment of
flex lines and grid tracks, grid items stretched, in the guest web engine.

/web/lot126.html: #c126 (100 x 40 at y 20) says align-items: safe center:
the 60 px high #a126 stays at the start, box 0 20 (true center would put
it at 10); #b126 (align-self: unsafe end) overflows the start side: 20 0
(Lot 125 refused both values: "alignment refused", #b126 at 20 20). #j126
(100 px at x 40) says justify-content: safe center over two unshrinkable
80 px items: #h126 at 40 and #i126 at 120 (true center: 10 and 90). The
wrapping #w126 (50 x 20 at y 110) has two 20 px lines under
align-content: center: they overflow both sides, #d126 at y 100 and #e126
at 120 (Lot 125: 110 and 130, clamped to the start). The grid #g126 (100
px at x 40) centers two 80 px columns: #f126 at x 10 and #k126 at 90 (Lot
125: 40 and 120). In the grid #s126 the item #m126 stretches to its 20 px
row (Lot 125: 10 high). web-console reports the "flex minimum prevents
fit" of #j126 and the "grid width overflow" of #g126, and no refused
alignment.
"""
from __future__ import print_function

import os
import subprocess
import sys
import time

HERE = os.path.dirname(os.path.abspath(__file__))
sys.path.insert(0, HERE)
LOGS = os.path.abspath(os.path.join(HERE, "..", "..", "test_logs"))
os.environ.setdefault("OSUI_GUI_LOG", os.path.join(LOGS, "qemu-lot126-serial.log"))
os.environ.setdefault("OSUI_GUI_ERR", os.path.join(LOGS, "qemu-lot126-stderr.log"))
os.environ.setdefault("OSUI_GUI_MON_SOCK", "/tmp/mohhdy-lot126-mon.sock")
os.environ.setdefault("OSUI_GUI_QMP_SOCK", "/tmp/mohhdy-lot126-qmp.sock")
os.environ.setdefault("OVERLAY_DISK", os.path.join(LOGS, "qemu-lot126-overlay.img"))
import test_qemu_osui_gui as g  # noqa: E402

PAGE = "/web/lot126.html"
# (tag, id): (box, color, bg or None, margin, padding, font100, lh100)
Z = ("#000000", None, "0 0 0 0", "0 0 0 0", 1600, 1000)
BOXES = [
    ("html", "p126", "0 0 640x230"), ("body", "q126", "0 0 640x230"),
    ("div", "c126", "0 20 100x40"), ("div", "a126", "0 20 20x60"), ("div", "b126", "20 0 20x60"),
    ("div", "j126", "40 80 100x10"), ("div", "h126", "40 80 80x10"), ("div", "i126", "120 80 80x10"),
    ("div", "w126", "0 110 50x20"), ("div", "d126", "0 100 30x20"), ("div", "e126", "0 120 30x20"),
    ("div", "g126", "40 150 100x10"), ("div", "f126", "10 150 80x10"), ("div", "k126", "90 150 80x10"),
    ("div", "s126", "0 160 60x20"), ("div", "m126", "0 160 30x20"), ("div", "n126", "30 160 30x20"),
    ("div", "gs126", "40 200 100x10"), ("div", "u126", "40 200 80x10"), ("div", "v126", "120 200 80x10"),
    ("div", "gi126", "40 220 50x10"), ("div", "r126", "30 220 70x10"),
]
# #c126, #j126, #w126 and #g126 carry their margins (margin-top, margin-left)
MARGIN = {"c126": "20 0 0 0", "j126": "20 0 0 40", "w126": "20 0 0 0", "g126": "20 0 0 40",
          "gs126": "20 0 0 40", "gi126": "10 0 0 40"}
WANT = [(tag, ident, box) + Z[:2] + (MARGIN.get(ident, Z[2]),) + Z[3:] for tag, ident, box in BOXES]
# what Lot 125 gave (safe / unsafe refused, overflowing lines and grid
# tracks clamped to the start, grid items at their content height), and
# true alignment where safe asks for the start
NEVER = {"a126": " box 0 10 20x60 ", "b126": " box 20 20 20x60 ", "h126": " box 10 80 80x10 ",
         "i126": " box 90 80 80x10 ", "d126": " box 0 110 30x20 ", "e126": " box 0 130 30x20 ",
         "f126": " box 40 150 80x10 ", "k126": " box 120 150 80x10 ", "m126": " box 0 160 30x10 ",
         "u126": " box 10 200 80x10 ", "v126": " box 90 200 80x10 ", "r126": " box 40 220 70x10 "}


def say(msg):
    print("[lot126] %s" % msg, flush=True)


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
    require(text, "web-console grid width overflow", "web-console")
    require(text, "web-console ok diag 3 ", "web-console")
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
        say("lot126 qemu ok in %.1fs" % (time.monotonic() - t0))
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
