#!/usr/bin/env python3
"""QEMU contract of Lot 124: flex lines in the guest web engine.

/web/lot124.html: align-content normal now behaves as stretch in a flex
container. The wrapping row #r124 (100 x 100) holds #a124 and #b124 (60
px wide) on two lines that share the 80 px left: both 50 high (Lot 123:
10 high, packed at the top), and #c124 (height: 50%) inside the stretched
#a124 resolves to 25 (Lot 123: 5). The wrapping column #s124 (100 px
wide) stretches its two lines to 50: #d124, #e124, #f124 50 wide (Lot
123: 8). In the wrap-reverse row #t124 (40 x 30) the single line fills
the height: #h124 stretches to 30 and the 10 px high #g124 sits at the
bottom of the line, its cross-start (Lot 123: #h124 10 high at the
bottom). #u124 says place-content: normal and place-items: normal center
(Lot 123: "place alignment refused" twice, #k124 centered at 42,165):
#k124 and #m124 stretched to 40. web-console reports no diagnostic.
"""
from __future__ import print_function

import os
import subprocess
import sys
import time

HERE = os.path.dirname(os.path.abspath(__file__))
sys.path.insert(0, HERE)
LOGS = os.path.abspath(os.path.join(HERE, "..", "..", "test_logs"))
os.environ.setdefault("OSUI_GUI_LOG", os.path.join(LOGS, "qemu-lot124-serial.log"))
os.environ.setdefault("OSUI_GUI_ERR", os.path.join(LOGS, "qemu-lot124-stderr.log"))
os.environ.setdefault("OSUI_GUI_MON_SOCK", "/tmp/mohhdy-lot124-mon.sock")
os.environ.setdefault("OSUI_GUI_QMP_SOCK", "/tmp/mohhdy-lot124-qmp.sock")
os.environ.setdefault("OVERLAY_DISK", os.path.join(LOGS, "qemu-lot124-overlay.img"))
import test_qemu_osui_gui as g  # noqa: E402

PAGE = "/web/lot124.html"
# (tag, id): (box, color, bg or None, margin, padding, font100, lh100)
Z = ("#000000", None, "0 0 0 0", "0 0 0 0", 1600, 1000)
BOXES = [
    ("html", "p124", "0 0 640x190"), ("body", "q124", "0 0 640x190"),
    ("div", "r124", "0 0 100x100"), ("div", "a124", "0 0 60x50"), ("div", "c124", "0 0 60x25"),
    ("div", "b124", "0 50 60x50"),
    ("div", "s124", "0 100 100x20"), ("div", "d124", "0 100 50x10"), ("div", "e124", "0 110 50x10"),
    ("div", "f124", "50 100 50x10"),
    ("div", "t124", "0 120 40x30"), ("div", "g124", "0 140 20x10"), ("div", "h124", "20 120 20x30"),
    ("div", "u124", "0 150 100x40"), ("div", "k124", "0 150 8x40"), ("div", "m124", "8 150 8x40"),
]
WANT = [(tag, ident, box) + Z for tag, ident, box in BOXES]
# what Lot 123 gave (align-content normal packed the lines at the start,
# items at the top of a wrap-reverse line, place normal refused)
NEVER = {"a124": " box 0 0 60x10 ", "c124": " box 0 0 60x5 ", "b124": " box 0 10 60x10 ",
         "d124": " box 0 100 8x10 ", "f124": " box 8 100 8x10 ", "h124": " box 20 140 20x10 ",
         "k124": " box 42 165 8x10 "}


def say(msg):
    print("[lot124] %s" % msg, flush=True)


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
        say("lot124 qemu ok in %.1fs" % (time.monotonic() - t0))
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
