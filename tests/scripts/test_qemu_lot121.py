#!/usr/bin/env python3
"""QEMU contract of Lot 121: flex items in the guest web engine.

/web/lot121.html is a column page: html { height: 100% }, body { height:
100%; display: flex; flex-flow: column }. #h121 (flex: none, 20 px) is at
the top, #z121 (flex: 0 0 30px) at the bottom (y 450) and #m121 (flex: 1)
takes the 430 px between them. In #m121, #c121 is a column of 100 px with
two flex: 1 items: #a121 holds a 60 px block, so its automatic minimum
(its content) freezes it at 60 and #b121 gets 40. #d121 is an auto-height
column of two flex: 1 1 0 items, #e121 and #f121, kept at their content
height (10 each) instead of 0. .q121 boxes take width: var(--w121) from
the style sheet: 120 px in .p121 (#q121), 60 px in .p121.n121 (#s121); the
guest pipeline hands the sheet's custom properties to the engine now (Lot
120 had one global value, the last: 60 for both). #x121 flex: 1 10px 2 is
refused: web-console reports "flex refused", diag 1.
"""
from __future__ import print_function

import os
import subprocess
import sys
import time

HERE = os.path.dirname(os.path.abspath(__file__))
sys.path.insert(0, HERE)
LOGS = os.path.abspath(os.path.join(HERE, "..", "..", "test_logs"))
os.environ.setdefault("OSUI_GUI_LOG", os.path.join(LOGS, "qemu-lot121-serial.log"))
os.environ.setdefault("OSUI_GUI_ERR", os.path.join(LOGS, "qemu-lot121-stderr.log"))
os.environ.setdefault("OSUI_GUI_MON_SOCK", "/tmp/mohhdy-lot121-mon.sock")
os.environ.setdefault("OSUI_GUI_QMP_SOCK", "/tmp/mohhdy-lot121-qmp.sock")
os.environ.setdefault("OVERLAY_DISK", os.path.join(LOGS, "qemu-lot121-overlay.img"))
import test_qemu_osui_gui as g  # noqa: E402

PAGE = "/web/lot121.html"
# (tag, id): (box, color, bg or None, margin, padding, font100, lh100)
WANT = [
    ("html", "r121", "0 0 640x480", "#000000", None, "0 0 0 0", "0 0 0 0", 1600, 1000),
    ("body", "y121", "0 0 640x480", "#000000", None, "0 0 0 0", "0 0 0 0", 1600, 1000),
    ("div", "h121", "0 0 640x20", "#000000", "#000080", "0 0 0 0", "0 0 0 0", 1600, 1000),
    ("div", "m121", "0 20 640x430", "#000000", None, "0 0 0 0", "0 0 0 0", 1600, 1000),
    ("div", "c121", "0 20 640x100", "#000000", None, "0 0 0 0", "0 0 0 0", 1600, 1000),
    ("div", "a121", "0 20 640x60", "#000000", "#008000", "0 0 0 0", "0 0 0 0", 1600, 1000),
    ("div", "b121", "0 80 640x40", "#000000", "#808000", "0 0 0 0", "0 0 0 0", 1600, 1000),
    ("div", "d121", "0 120 640x20", "#000000", None, "0 0 0 0", "0 0 0 0", 1600, 1000),
    ("div", "e121", "0 120 640x10", "#000000", None, "0 0 0 0", "0 0 0 0", 1600, 1000),
    ("div", "f121", "0 130 640x10", "#000000", None, "0 0 0 0", "0 0 0 0", 1600, 1000),
    ("div", "q121", "0 140 120x10", "#000000", "#000000", "0 0 0 0", "0 0 0 0", 1600, 1000),
    ("div", "s121", "0 150 60x10", "#000000", "#000000", "0 0 0 0", "0 0 0 0", 1600, 1000),
    ("div", "z121", "0 450 640x30", "#000000", "#800000", "0 0 0 0", "0 0 0 0", 1600, 1000),
]
# what Lot 120 gave (the flex shorthand and flex-flow unknown: a row of
# three equal slots; the sheet's --w121 resolved once, globally, to 60px)
NEVER = {"h121": " box 0 0 213x20 ", "m121": " box 213 0 213x140 ", "a121": " box 213 0 ",
         "b121": " box 213 60 213x10 ", "e121": " box 213 100 ", "q121": " 60x10 ", "z121": " box 426 0 213x0 "}


def say(msg):
    print("[lot121] %s" % msg, flush=True)


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
    require(text, "web-console flex refused", "web-console")
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
        say("lot121 qemu ok in %.1fs" % (time.monotonic() - t0))
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
