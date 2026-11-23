#!/usr/bin/env python3
"""Lot 119 in QEMU, offline: container queries, container units and
relative colors in the guest engine.

One boot at ui-scale 1x (fw_cfg 4 quarters), no network and no guest clock:
"web-inspect /web/lot119.html ID" lays the local page out in the guest's
engine (640 x 480 viewport) and prints each box, its color, background,
margins, padding, font size and used line height.

#a119 is a 400 px container named card (container: card / inline-size).
#b119 50cqw is 200 x 10 on rgb(from #102030 b g r) #302010 (Lot 118: cqw
refused, auto 400, background refused). #c119 gets the rules of @container
card (min-width: 300px): 25cqi is 100 px on hsl(from red calc(h + 120) s l)
#00ff00. #d119 is 400 px wide in rgb(from #ff8000 g r b) #80ff00 and the
rule of @container (max-width: 300px) does not apply (no red background).
#e119 is a 300 x 60 size container with 5 px padding: 310 x 70. #f119 gets
the rules of @container (orientation: landscape) and (height: 60px): 10cqw
x 10cqmin is 30 x 6 (the content box, padding out), on
rgb(from rgb(0 0 255 / 0.5) r g b / 1) #0000ff. #g119
floats with container-type: inline-size: its width is that of an empty box
(10 px, its padding) whatever its text. web-console reports the one refused
rule, "@container none (...)": "container query refused".
"""
from __future__ import print_function

import os
import subprocess
import sys
import time

HERE = os.path.dirname(os.path.abspath(__file__))
sys.path.insert(0, HERE)
LOGS = os.path.abspath(os.path.join(HERE, "..", "..", "test_logs"))
os.environ.setdefault("OSUI_GUI_LOG", os.path.join(LOGS, "qemu-lot119-serial.log"))
os.environ.setdefault("OSUI_GUI_ERR", os.path.join(LOGS, "qemu-lot119-stderr.log"))
os.environ.setdefault("OSUI_GUI_MON_SOCK", "/tmp/mohhdy-lot119-mon.sock")
os.environ.setdefault("OSUI_GUI_QMP_SOCK", "/tmp/mohhdy-lot119-qmp.sock")
os.environ.setdefault("OVERLAY_DISK", os.path.join(LOGS, "qemu-lot119-overlay.img"))
import test_qemu_osui_gui as g  # noqa: E402

PAGE = "/web/lot119.html"
# id: (box, color, bg or None, margin, padding, font100, lh100)
WANT = [
    ("a119", "0 0 400x16", "#000000", None, "0 0 0 0", "0 0 0 0", 1600, 1000),
    ("b119", "0 0 200x10", "#000000", "#302010", "0 0 0 0", "0 0 0 0", 1600, 1000),
    ("c119", "0 10 100x4", "#000000", "#00ff00", "0 0 0 0", "0 0 0 0", 1600, 1000),
    ("d119", "0 14 400x2", "#80ff00", None, "0 0 0 0", "0 0 0 0", 1600, 1000),
    ("e119", "0 16 310x70", "#000000", None, "0 0 0 0", "5 5 5 5", 1600, 1000),
    ("f119", "5 21 30x6", "#000000", "#0000ff", "0 0 0 0", "0 0 0 0", 1600, 1000),
    ("g119", "0 86 10x8", "#000000", None, "0 0 0 0", "0 5 0 5", 1600, 1000),
]
# what Lot 118 gave (cq units and @container refused, relative colors refused)
NEVER = {"b119": " box 0 0 400x", "c119": " box 0 10 400x", "f119": " box 5 21 300x", "e119": " padding 0 0 0 0 ", "d119": " color #000000 "}


def say(msg):
    print("[lot119] %s" % msg, flush=True)


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
    for ident, box, color, bg, margin, padding, f100, lh100 in WANT:
        start = g.send_command_until(monitor, "web-inspect %s %s" % (PAGE, ident), "inspect ", proc)
        g.wait_for(proc, " lh100 ", g.CMD_TIMEOUT, start)
        line = inspect_line(g.normalized_log(g.log_text()[start:]), ident)
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
        say("#%s box %s color %s margin %s padding %s" % (ident, box, color, margin, padding))
    start = g.send_command_until(monitor, "web-console %s" % PAGE, "web-console ok", proc)
    text = g.normalized_log(g.log_text()[start:])
    require(text, "web-console container query refused", "web-console")
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
        say("lot119 qemu ok in %.1fs" % (time.monotonic() - t0))
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
