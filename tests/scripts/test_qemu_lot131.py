#!/usr/bin/env python3
"""QEMU contract of Lot 131: display: contents.

/web/lot131.html: .w131 is display: contents with a red background, 20 px
padding, a 3 px border, a 9 px margin, a size and (on #y131) a relative
position that must not apply. Lot 130 refused the value ("display
refused"): each wrapper stayed a box, #w131 one flex item holding #a131 and
#b131 (#c131 at 69 0, #a131 at 32 32), #v131 one grid cell, #x131 and #y131
boxes with edges. Now the wrapper has no box (display contents, box
0 0 0x0, its style still computed: red, margin 9, padding 20) and its
children belong to its parent: #a131 0 0 50x10, #b131 50 0 60x10 and #c131
110 0 30x10 in the flex row, #ga131 0 10 100x10, #gb131 100 10 100x10 and
#gc131 0 20 100x10 in the grid, #h131 (50% high) 0 30 200x20 against its
grandparent (40 px), #k131 absolute at 180 90 in #r131 (the relative
wrapper is not its containing block). web-console reports no diagnostic.
"""
from __future__ import print_function

import os
import subprocess
import sys
import time

HERE = os.path.dirname(os.path.abspath(__file__))
sys.path.insert(0, HERE)
LOGS = os.path.abspath(os.path.join(HERE, "..", "..", "test_logs"))
os.environ.setdefault("OSUI_GUI_LOG", os.path.join(LOGS, "qemu-lot131-serial.log"))
os.environ.setdefault("OSUI_GUI_ERR", os.path.join(LOGS, "qemu-lot131-stderr.log"))
os.environ.setdefault("OSUI_GUI_MON_SOCK", "/tmp/mohhdy-lot131-mon.sock")
os.environ.setdefault("OSUI_GUI_QMP_SOCK", "/tmp/mohhdy-lot131-qmp.sock")
os.environ.setdefault("OVERLAY_DISK", os.path.join(LOGS, "qemu-lot131-overlay.img"))
import test_qemu_osui_gui as g  # noqa: E402

PAGE = "/web/lot131.html"
# (tag, id, display, box, bg or None, margin, padding): a wrapper keeps its computed style
Z = ("#000000", "0 0 0 0", "0 0 0 0", 1600, 1000)
W = ("#ff0000", "9 9 9 9", "20 20 20 20")
ROWS = [
    ("html", "p131", "block", "0 0 640x100"), ("body", "q131", "block", "0 0 640x100"),
    ("div", "f131", "flex", "0 0 300x10"), ("div", "w131", "contents", "0 0 0x0", W),
    ("div", "a131", "block", "0 0 50x10"), ("div", "b131", "block", "50 0 60x10"), ("div", "c131", "block", "110 0 30x10"),
    ("div", "g131", "grid", "0 10 300x20"), ("div", "v131", "contents", "0 0 0x0", W),
    ("div", "ga131", "block", "0 10 100x10"), ("div", "gb131", "block", "100 10 100x10"), ("div", "gc131", "block", "0 20 100x10"),
    ("div", "t131", "block", "0 30 200x40"), ("div", "x131", "contents", "0 0 0x0", W),
    ("div", "h131", "block", "0 30 200x20"),
    ("div", "r131", "block", "0 70 200x30"), ("div", "y131", "contents", "0 0 0x0", W),
    ("div", "k131", "block", "180 90 20x10"),
]
# what Lot 130 gave (the wrappers were boxes)
NEVER = {"a131": " box 32 32 50x10 ", "b131": " box 32 42 60x10 ", "c131": " box 69 0 30x10 ",
         "ga131": " box 32 195 8x10 ", "gc131": " box 100 163 100x163 ", "h131": " box 32 358 8x49 ",
         "k131": " box 67 507 20x10 ", "f131": " box 0 0 300x163 "}


def say(msg):
    print("[lot131] %s" % msg, flush=True)


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
    for row in ROWS:
        tag, ident, display, box = row[:4]
        bg, margin, padding = row[4] if len(row) > 4 else (None, Z[1], Z[2])
        start = g.send_command_until(monitor, "web-inspect %s %s" % (PAGE, ident), "inspect ", proc)
        g.wait_for(proc, " lh100 ", g.CMD_TIMEOUT, start)
        line = inspect_line(g.normalized_log(g.log_text()[start:]), tag, ident)
        what = "#" + ident
        require(line, " display %s " % display, what)
        require(line, " box %s " % box, what)
        require(line, " color %s " % Z[0], what)
        if bg:
            require(line, " bg %s " % bg, what)
        else:
            refuse(line, " bg ", what)
        require(line, " margin %s " % margin, what)
        require(line, " padding %s " % padding, what)
        require(line, " font100 %d lh100 %d" % (Z[3], Z[4]), what)
        if ident in NEVER:
            refuse(line, NEVER[ident], what)
        refuse(line, " canvas ", what)
        say("#%s display %s box %s" % (ident, display, box))
    start = g.send_command_until(monitor, "web-console %s" % PAGE, "web-console ok", proc)
    text = g.normalized_log(g.log_text()[start:])
    require(text, "web-console ok diag 0 ", "web-console")
    for refused in ("rule refused", "display refused", "container query unsettled", "gradient refused", "image"):
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
        say("lot131 qemu ok in %.1fs" % (time.monotonic() - t0))
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
