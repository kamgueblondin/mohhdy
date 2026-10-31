#!/usr/bin/env python3
"""The terminal grid follows the window and the screen (vga_console_resize).

On the VBE desktop (1024x768, USB tablet, COM2 on a unix socket):
  * the 95-char output of an echo wraps at 80 columns in the default window;
  * maximize -> 'gfx term grid 127x46 reflow', term-dump is 127 wide and
    the long line is back on one row;
  * the QEMU window is resized (COM2 "1280x800") while the terminal is
    open -> 'gfx term fb resize 1280x800' and grid 159x48;
  * maximize off -> grid 80x25 again (window);
  * 'console' (text mode) -> term-dump backend=text grid=80x25.
"""
from __future__ import print_function

import os
import re
import socket
import subprocess
import sys
import time

HERE = os.path.dirname(os.path.abspath(__file__))
sys.path.insert(0, HERE)
LOGS = os.path.join(HERE, "..", "..", "test_logs")
os.environ.setdefault("OSUI_GUI_LOG", os.path.join(LOGS, "qemu-gui-term-resize-serial.log"))
os.environ.setdefault("OSUI_GUI_MON_SOCK", "/tmp/mohhdy-gui-resize-mon.sock")
os.environ.setdefault("OSUI_GUI_QMP_SOCK", "/tmp/mohhdy-gui-resize-qmp.sock")
os.environ.setdefault("OVERLAY_DISK", os.path.join(LOGS, "qemu-gui-term-resize-overlay.img"))
import test_qemu_osui_gui as g  # noqa: E402
import test_qemu_gui_term_window as tw  # noqa: E402

COM2_SOCK = "/tmp/mohhdy-gui-resize-com2.sock"
LONG = "echo " + "x" * 95       # echo prints 95 x: wraps at 80, fits in 127


def dump(monitor, proc):
    start = g.send_command_until(monitor, "term-dump", "TERM-DUMP END", proc)
    text = g.log_text()[start:]
    head = re.search(r"TERM-DUMP BEGIN backend=(\w+) grid=(\d+)x(\d+)", text)
    if not head:
        raise RuntimeError("no TERM-DUMP header with grid")
    body = text[text.find("TERM-DUMP BEGIN"):text.find("TERM-DUMP END")]
    rows = re.findall(r"^TERM\|(.*)$", body, re.M)
    return head.group(1), int(head.group(2)), int(head.group(3)), rows


def click(qmp, proc, xy, marker, label, wh=(1024, 768)):
    start = len(g.log_text())
    for _ in range(3):
        g.click_tablet(qmp, xy[0], xy[1], wh[0], wh[1])
        try:
            g.wait_for(proc, marker, 8.0, start)
            g.say("%s -> %s" % (label, marker))
            return start
        except RuntimeError:
            continue
    raise RuntimeError("%s: no %r on serial" % (label, marker))


def main():
    os.makedirs(g.LOG_DIR, exist_ok=True)
    for path in (g.LOG, g.MON_SOCK, g.QMP_SOCK, COM2_SOCK):
        try:
            os.remove(path)
        except OSError:
            pass
    g.prepare_test_disk()
    proc = monitor = qmp = com2 = None
    try:
        with open(os.devnull, "wb") as err:
            proc = subprocess.Popen([
                "qemu-system-i386", "-fw_cfg", "name=opt/mohhdy/ui-scale,string=4", "-cpu", "pentium3", "-kernel", g.KERNEL,
                "-initrd", g.INITRD, "-m", "1024M", "-display", "none", "-vga", "std",
                "-usb", "-device", "usb-tablet",
                "-serial", "file:" + g.LOG, "-serial", "unix:%s,server,nowait" % COM2_SOCK,
                "-monitor", "unix:%s,server,nowait" % g.MON_SOCK,
                "-qmp", "unix:%s,server,nowait" % g.QMP_SOCK,
                "-machine", "type=pc,accel=tcg", "-no-reboot", "-no-shutdown",
            ] + g.qemu_disk_args(), cwd=g.ROOT, stdout=err, stderr=err)
            g.wait_for(proc, "[0m : ", g.BOOT_TIMEOUT)
            g.wait_for(proc, "usbdriver tablet enumerated port ", g.CMD_TIMEOUT)
            monitor = g.monitor_connect()
            qmp = g.qmp_connect()
            com2 = socket.socket(socket.AF_UNIX, socket.SOCK_STREAM)
            com2.connect(COM2_SOCK)
            time.sleep(0.6)
            g.send_command_until(monitor, "gui", "osui gui live", proc, wait_prompt=False)
            g.wait_for(proc, "OSUI-SNAP", g.CMD_TIMEOUT)
            win = tw.open_terminal(monitor, proc)
            g.send_command_until(monitor, LONG, "ligne lue: echo", proc)
            backend, cols, rows, lines = dump(monitor, proc)
            if (backend, cols, rows) != ("vbe", 80, 25):
                raise RuntimeError("window grid %s %dx%d, want vbe 80x25" % (backend, cols, rows))
            if not any(l.rstrip() == "x" * 80 for l in lines):
                raise RuntimeError("100-char line does not wrap at 80 in the window")
            g.say("window: 80x25, long line wrapped")
            # maximize: the grid fills the screen and the line reflows
            start = click(qmp, proc, tw.button_center(win, "max"), "gfx term button maximize", "maximize")
            g.wait_for(proc, "gfx term grid 127x46 reflow", g.CMD_TIMEOUT, start)
            backend, cols, rows, lines = dump(monitor, proc)
            if (cols, rows) != (127, 46):
                raise RuntimeError("maximized term-dump %dx%d, want 127x46" % (cols, rows))
            if not any(l.rstrip() == "x" * 95 for l in lines):
                raise RuntimeError("long line did not reflow onto one row when maximized")
            g.say("maximized: 127x46, long line on one row")
            # QEMU window resize while the terminal is open
            start = len(g.log_text())
            com2.sendall(b"1280x800\n")
            g.wait_for(proc, "gfx term fb resize 1280x800", g.CMD_TIMEOUT, start)
            g.wait_for(proc, "gfx term grid 159x48 reflow", g.CMD_TIMEOUT, start)
            _b, cols, rows, lines = dump(monitor, proc)
            if (cols, rows) != (159, 48):
                raise RuntimeError("after screen resize term-dump %dx%d, want 159x48" % (cols, rows))
            g.say("screen resize 1280x800: grid 159x48")
            # maximize off: window grid again (button of the maximized layout)
            mx = (1, 0, 0, 1280, 800)
            start = click(qmp, proc, tw.button_center(mx, "max"), "gfx term button maximize", "maximize off",
                          (1280, 800))
            g.wait_for(proc, "gfx term grid 80x25 reflow", g.CMD_TIMEOUT, start)
            _b, cols, rows, lines = dump(monitor, proc)
            if (cols, rows) != (80, 25):
                raise RuntimeError("restored window %dx%d, want 80x25" % (cols, rows))
            g.say("maximize off: 80x25")
            # text mode stays 80x25
            g.send_command_until(monitor, "console", "ligne lue: console", proc, wait_prompt=False)
            time.sleep(2.0)
            backend, cols, rows, _l = dump(monitor, proc)
            if (backend, cols, rows) != ("text", 80, 25):
                raise RuntimeError("text mode %s %dx%d, want text 80x25" % (backend, cols, rows))
            g.say("text mode: 80x25")
        g.say("QEMU terminal resize/reflow contract passed.")
        return 0
    finally:
        for c in (com2, qmp, monitor):
            if c is not None:
                c.close()
        g.terminate(proc)


if __name__ == "__main__":
    try:
        raise SystemExit(main())
    except Exception as error:
        print("QEMU terminal resize failed: %s" % error, file=sys.stderr)
        raise SystemExit(1)
