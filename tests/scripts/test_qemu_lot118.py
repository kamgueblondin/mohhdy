#!/usr/bin/env python3
"""Lot 118 in QEMU, offline: CSS math, viewport units, colors and
@property in the guest engine.

One boot at ui-scale 1x (fw_cfg 4 quarters), no network and no guest clock:
"web-inspect /web/lot118.html ID" lays the local page out in the guest's
engine (640 x 480 viewport) and prints each box, its color, background,
margins, padding, font size and used line height.

#a118 50vw x 10vh is 320 x 48 on hwb(120 20% 30%) #33b333 (Lot 118: vw
refused, auto 640, no background). #b118 calc(100vw - 2e2px) x calc(1e1px)
is 440 x 10 in rgb(calc(255 / 2) 0 none) #800000. #c118 --w: 3em registered
as <length> is computed to 48px where it is declared: its margin-left 48
and the child #c118i at 32 px inherits 48px as its padding (Lot 118: the
text 3em, 96). #d118 max(10vmin, 5vh) x calc(sqrt(16) * 5px) is 48 x 20 on
hsl(0.5turn 100 50) #00ffff. #e118 font-size clamp(12px, 2vw, 20px) is
12.8 px with line-height calc(1.5) a number: 19.2 px. #f118 round(up, 13px,
5px) is a 15 px margin, pow(2, 3) a height of 8, color var(--k) the
registered initial hwb(240 0% 0%) #0000ff, background rgb(100% 0 50% / ...)
mixing percentages and numbers. #g118 margin-left atan2(1, 1) / 1deg is 45;
its --w: Auto does not match the syntax '<length> | auto' (identifiers are
case-sensitive): web-console reports "css property syntax refused". The
custom properties are set in style attributes: the page pipeline
(webrt_css_resolve_vars) substitutes var() in style sheets before the engine.
"""
from __future__ import print_function

import os
import subprocess
import sys
import time

HERE = os.path.dirname(os.path.abspath(__file__))
sys.path.insert(0, HERE)
LOGS = os.path.abspath(os.path.join(HERE, "..", "..", "test_logs"))
os.environ.setdefault("OSUI_GUI_LOG", os.path.join(LOGS, "qemu-lot118-serial.log"))
os.environ.setdefault("OSUI_GUI_ERR", os.path.join(LOGS, "qemu-lot118-stderr.log"))
os.environ.setdefault("OSUI_GUI_MON_SOCK", "/tmp/mohhdy-lot118-mon.sock")
os.environ.setdefault("OSUI_GUI_QMP_SOCK", "/tmp/mohhdy-lot118-qmp.sock")
os.environ.setdefault("OVERLAY_DISK", os.path.join(LOGS, "qemu-lot118-overlay.img"))
import test_qemu_osui_gui as g  # noqa: E402

PAGE = "/web/lot118.html"
# id: (box, color, bg or None, margin, padding, font100, lh100)
WANT = [
    ("a118", "0 0 320x48", "#000000", "#33b333", "0 0 0 0", "0 0 0 0", 1600, 1000),
    ("b118", "0 48 440x10", "#800000", None, "0 0 0 0", "0 0 0 0", 1600, 1000),
    ("c118", "48 58 592x4", "#000000", None, "0 0 0 48", "0 0 0 0", 1600, 1000),
    ("c118i", "48 58 592x2", "#000000", None, "0 0 0 0", "0 0 0 48", 3200, 2000),
    ("d118", "0 62 48x20", "#000000", "#00ffff", "0 0 0 0", "0 0 0 0", 1600, 1000),
    ("e118", "0 82 640x19", "#000000", None, "0 0 0 0", "0 0 0 0", 1280, 1920),
    ("f118", "0 116 640x8", "#0000ff", "#ff0080", "15 0 0 0", "0 0 0 0", 1600, 1000),
    ("g118", "45 124 595x1", "#ff0000", None, "0 0 0 45", "0 0 0 0", 1600, 1000),
]
# what Lot 117 gave (vw refused, @property ignored, 3em inherited as text)
NEVER = {"a118": " box 0 0 640x", "c118i": " padding 0 0 0 96 ", "f118": " color #000000 ", "d118": " box 0 62 640x"}


def say(msg):
    print("[lot118] %s" % msg, flush=True)


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
    require(text, "web-console css property syntax refused", "web-console")
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
        say("lot118 qemu ok in %.1fs" % (time.monotonic() - t0))
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
