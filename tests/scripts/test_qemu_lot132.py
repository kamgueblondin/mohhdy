#!/usr/bin/env python3
"""QEMU contract of Lot 132: display: contents on table parts.

/web/lot132.html: .w132 is display: contents with a red background, 20 px
padding, a 3 px border, a 9 px margin and a size that must not apply, on a
row (#r132 and #s132), a cell (#x132, #n132) and a body (#y132, #z132).
Lot 131 handed the cells of a contents row to the table ("table child
refused": #a132, #b132, #c132 and #o132 had no box and were not painted) and
the content of a contents cell to the row ("table cell refused": #k132 had
no box). Now the cells of the two consecutive contents rows #r132 and #s132
make one anonymous row (CSS Tables 3 9.1.2) with the plain row after it as
the second one: #a132 1 1 100x12, #b132 101 1 100x12, #c132 201 1 100x12,
#d132 1 13 100x12 and #e132 101 13 100x12 in a 302x26 table. The content of
the contents cell #x132 is an anonymous cell between #f132 and #g132: #k132
101 27 100x20. A contents cell in a contents row in a contents body gives
an anonymous cell next to #o132 (151 59 150x12). A contents div that hands a
caption, a column group, a head, a body and a foot to #v132 does not wrap
them (the column of 90 px, rows at 82, 92 and 102). An absolute box and a
display none box between the two cells of the contents row #aw132 do not cut
the row: #ab132 0 112 150x10 and #cd132 150 112 150x10 (Lot 131 gave them no
box). Each contents element keeps its computed style and has no box (0 0 0x0).
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
os.environ.setdefault("OSUI_GUI_LOG", os.path.join(LOGS, "qemu-lot132-serial.log"))
os.environ.setdefault("OSUI_GUI_ERR", os.path.join(LOGS, "qemu-lot132-stderr.log"))
os.environ.setdefault("OSUI_GUI_MON_SOCK", "/tmp/mohhdy-lot132-mon.sock")
os.environ.setdefault("OSUI_GUI_QMP_SOCK", "/tmp/mohhdy-lot132-qmp.sock")
os.environ.setdefault("OVERLAY_DISK", os.path.join(LOGS, "qemu-lot132-overlay.img"))
import test_qemu_osui_gui as g  # noqa: E402

PAGE = "/web/lot132.html"
# (tag, id, display, box, bg or None, margin, padding): a wrapper keeps its computed style
Z = ("#000000", "0 0 0 0", "0 0 0 0", 1600, 1000)
W = ("#ff0000", "9 9 9 9", "20 20 20 20")
ROWS = [
    ("html", "p132", "block", "0 0 640x122"), ("body", "q132", "block", "0 0 640x122"),
    ("table", "t132", "table", "0 0 302x26"), ("tr", "r132", "contents", "0 0 0x0", W),
    ("td", "a132", "cell", "1 1 100x12"), ("td", "b132", "cell", "101 1 100x12"),
    ("tr", "s132", "contents", "0 0 0x0", W), ("td", "c132", "cell", "201 1 100x12"),
    ("td", "d132", "cell", "1 13 100x12"), ("td", "e132", "cell", "101 13 100x12"),
    ("table", "u132", "table", "0 26 302x32"), ("td", "f132", "cell", "1 27 100x12"),
    ("td", "x132", "contents", "0 0 0x0", W), ("div", "k132", "block", "101 27 100x20"),
    ("td", "g132", "cell", "201 27 100x12"),
    ("table", "l132", "table", "0 58 302x14"), ("tbody", "y132", "contents", "0 0 0x0", W),
    ("tr", "z132", "contents", "0 0 0x0", W), ("td", "n132", "contents", "0 0 0x0", W),
    ("td", "o132", "cell", "151 59 150x12"),
    ("table", "v132", "table", "0 72 300x40"), ("caption", "cp132", "block", "0 72 300x10"),
    ("thead", "hd132", "inline", "0 0 0x0"), ("td", "h132", "cell", "0 82 90x10"), ("td", "i132", "cell", "90 82 210x10"),
    ("tbody", "bd132", "inline", "0 0 0x0"), ("td", "j132", "cell", "0 92 90x10"), ("td", "k2132", "cell", "90 92 210x10"),
    ("tfoot", "ft132", "inline", "0 0 0x0"), ("td", "f2132", "cell", "0 102 90x10"), ("td", "g2132", "cell", "90 102 210x10"),
    ("table", "w132t", "table", "0 112 300x10"), ("tr", "aw132", "contents", "0 0 0x0", W),
    ("td", "ab132", "cell", "0 112 150x10"), ("span", "sp132", "inline", "5 5 8x10"),
    ("span", "no132", "none", "0 0 0x0"), ("td", "cd132", "cell", "150 112 150x10"),
]
# what Lot 131 gave (the cells of a contents row had no box, the table was short)
NEVER = {"a132": " box 0 0 0x0 ", "b132": " box 0 0 0x0 ", "c132": " box 0 0 0x0 ", "d132": " box 1 1 150x12 ",
         "e132": " box 151 1 150x12 ", "t132": " box 0 0 302x14 ", "f132": " box 1 15 150x12 ", "k132": " box 0 0 0x0 ",
         "g132": " box 151 15 150x12 ", "u132": " box 0 14 302x14 ", "o132": " box 0 0 0x0 ", "l132": " box 0 28 302x2 ",
         "ab132": " box 0 0 0x0 ", "cd132": " box 0 0 0x0 ", "w132t": " box 0 70 300x0 ", "v132": " box 0 30 300x40 "}


def say(msg):
    print("[lot132] %s" % msg, flush=True)


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
    for refused in ("rule refused", "display refused", "table child refused", "table cell refused", "table anonymous box budget", "container query unsettled", "gradient refused", "image"):
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
        say("lot132 qemu ok in %.1fs" % (time.monotonic() - t0))
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
