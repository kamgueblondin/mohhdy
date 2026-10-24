#!/usr/bin/env python3
"""The desktop file explorer and text editor (UX3) on the real OS.

On the VBE desktop (1024x768), keys through the QEMU monitor and clicks
through the USB tablet:
  * /files opens the explorer on /;
  * d creates a folder, Enter enters it, n creates a file, r renames it,
    a second file is deleted with Suppr + confirmation;
  * a toolbar click ([Haut]) goes back up, a click on the folder row and
    a double click enter it again (mouse path through the kernel events);
  * Enter on the file opens it in the editor, the typed text is saved
    with ESC w;
  * the terminal (cat) shows the same bytes: one file layer (fsops) for
    the shell commands and the desktop apps.
"""
from __future__ import print_function

import os
import subprocess
import sys
import time

HERE = os.path.dirname(os.path.abspath(__file__))
sys.path.insert(0, HERE)
LOGS = os.path.join(HERE, "..", "..", "test_logs")
os.environ.setdefault("OSUI_GUI_LOG", os.path.join(LOGS, "qemu-gui-apps-serial.log"))
os.environ.setdefault("OSUI_GUI_MON_SOCK", "/tmp/mohhdy-gui-apps-mon.sock")
os.environ.setdefault("OSUI_GUI_QMP_SOCK", "/tmp/mohhdy-gui-apps-qmp.sock")
os.environ.setdefault("OVERLAY_DISK", os.path.join(LOGS, "qemu-gui-apps-overlay.img"))
import test_qemu_osui_gui as g  # noqa: E402
import test_qemu_gui_term_window as tw  # noqa: E402

W, H = 1024, 768
BAR_H = 34
# explorer window (slot 0) default place: kernel/gfx_desktop.c get_layout
APP0_X, APP0_Y = 24, BAR_H + 20
TOOL_Y = APP0_Y + 36 - 3 + 6
GRID_Y = APP0_Y + 58
ROW_H = 14
ALIASES = {" ": "spc", "-": "minus", ".": "dot", "/": "slash", "=": "equal", "_": "shift-minus"}


def keys(monitor, text, delay=0.15):
    for ch in text:
        g.send_key(monitor, ALIASES.get(ch, ch.lower()))
        time.sleep(delay)


def key(monitor, name, delay=0.3):
    g.send_key(monitor, name)
    time.sleep(delay)


def expect(proc, marker, start, label):
    g.wait_for(proc, marker, g.CMD_TIMEOUT, start)
    g.say("%s -> %s" % (label, marker))


def step(monitor, proc, label, marker, seq):
    """seq: list of ('k', name) single keys or ('t', text)."""
    start = len(g.log_text())
    for kind, val in seq:
        if kind == "k":
            key(monitor, val)
        else:
            keys(monitor, val)
    expect(proc, marker, start, label)


def click(qmp, proc, x, y, marker, label):
    start = len(g.log_text())
    for _ in range(3):
        g.click_tablet(qmp, x, y, W, H)
        try:
            g.wait_for(proc, marker, 8.0, start)
            g.say("%s -> %s" % (label, marker))
            time.sleep(0.6)
            return
        except RuntimeError:
            continue
    raise RuntimeError("%s: no %r on serial" % (label, marker))


def main():
    os.makedirs(g.LOG_DIR, exist_ok=True)
    for path in (g.LOG, g.MON_SOCK, g.QMP_SOCK):
        try:
            os.remove(path)
        except OSError:
            pass
    g.prepare_test_disk()
    proc = monitor = qmp = None
    try:
        with open(os.devnull, "wb") as err:
            proc = subprocess.Popen([
                "qemu-system-i386", "-cpu", "pentium3", "-kernel", g.KERNEL,
                "-initrd", g.INITRD, "-m", "1024M", "-display", "none", "-vga", "std",
                "-usb", "-device", "usb-tablet",
                "-serial", "file:" + g.LOG, "-monitor", "unix:%s,server,nowait" % g.MON_SOCK,
                "-qmp", "unix:%s,server,nowait" % g.QMP_SOCK,
                "-machine", "type=pc,accel=tcg", "-no-reboot", "-no-shutdown",
            ] + g.qemu_disk_args(), cwd=g.ROOT, stdout=err, stderr=err)
            g.wait_for(proc, "[0m : ", g.BOOT_TIMEOUT)
            g.wait_for(proc, "usbdriver tablet enumerated port ", g.CMD_TIMEOUT)
            monitor = g.monitor_connect()
            qmp = g.qmp_connect()
            time.sleep(0.6)
            g.send_command_until(monitor, "gui", "osui gui live", proc, wait_prompt=False)
            g.wait_for(proc, "OSUI-SNAP", g.CMD_TIMEOUT)
            # explorer
            g.send_command_until(monitor, "/files", "osui files open dir=/", proc, mode="getc", wait_prompt=False)
            time.sleep(1.0)
            _, _, rgb = g.screendump_rgb(monitor, "qemu-gui-apps-files.ppm")
            # a directory row is drawn in the directory colour (122,196,236)
            n = tw_count(rgb, APP0_X + 12, GRID_Y + 2 * ROW_H, APP0_X + 400, GRID_Y + 22 * ROW_H, (122, 196, 236))
            if n < 30:
                raise RuntimeError("explorer window: no directory row drawn (%d px)" % n)
            step(monitor, proc, "new folder", "osui files mkdir ok /ux3", [("k", "d"), ("t", "ux3"), ("k", "ret")])
            step(monitor, proc, "enter folder", "osui files cd /ux3", [("k", "ret")])
            step(monitor, proc, "new file", "osui files touch ok /ux3/a.txt", [("k", "n"), ("t", "a.txt"), ("k", "ret")])
            step(monitor, proc, "rename", "osui files rename ok /ux3/b.txt",
                 [("k", "r")] + [("k", "backspace")] * 5 + [("t", "b.txt"), ("k", "ret")])
            step(monitor, proc, "second file", "osui files touch ok /ux3/c.txt", [("k", "n"), ("t", "c.txt"), ("k", "ret")])
            step(monitor, proc, "delete (confirm)", "osui files rm ok /ux3/c.txt", [("k", "delete"), ("k", "o")])
            # mouse: [Haut] in the toolbar, then click + double click on the ux3 row
            click(qmp, proc, APP0_X + 12 + 3 * 8, TOOL_Y, "osui files cd /", "toolbar [Haut]")
            # grid row of ux3 in / (entries start at grid row 2)
            row = ux3_row(monitor, proc)
            ry = GRID_Y + row * ROW_H + 6
            for attempt in range(3):
                start = len(g.log_text())
                tw.move(qmp, APP0_X + 12 + 4 * 8, ry)
                for down in (True, False, True, False):
                    g.qmp_cmd(qmp, {"execute": "input-send-event", "arguments": {"events": [
                        {"type": "btn", "data": {"down": down, "button": "left"}}]}})
                    time.sleep(0.09)
                try:
                    g.wait_for(proc, "osui files cd /ux3", 8.0, start)
                    break
                except RuntimeError:
                    if attempt == 2:
                        raise
            g.say("double click on the folder row -> osui files cd /ux3")
            # editor
            step(monitor, proc, "open in editor", "osui edit open file=/ux3/b.txt", [("k", "home"), ("k", "ret")])
            time.sleep(0.8)
            keys(monitor, "hello ux3")
            time.sleep(0.5)
            step(monitor, proc, "save", "osui edit saved file=/ux3/b.txt", [("k", "esc"), ("k", "w")])
            _, _, rgb = g.screendump_rgb(monitor, "qemu-gui-apps-editor.ppm")
            step(monitor, proc, "close editor", "osui edit close file=/ux3/b.txt", [("k", "esc"), ("k", "q")])
            step(monitor, proc, "close explorer", "osui files close", [("k", "esc")])
            time.sleep(0.8)
            # the terminal reads the same bytes
            tw.open_terminal(monitor, proc)
            g.send_command_until(monitor, "cat /ux3/b.txt", "hello ux3", proc)
            g.say("cat in the terminal shows the text saved by the desktop editor")
        g.say("QEMU GUI apps (explorer + editor) contract passed.")
        return 0
    finally:
        if qmp is not None:
            qmp.close()
        if monitor is not None:
            monitor.close()
        g.terminate(proc)


def tw_count(rgb, x0, y0, x1, y1, color, tol=10):
    n = 0
    for y in range(y0, y1):
        for x in range(x0, x1):
            i = (y * W + x) * 3
            p = rgb[i:i + 3]
            if all(abs(p[k] - color[k]) <= tol for k in range(3)):
                n += 1
    return n


def ux3_row(monitor, proc):
    """Grid row of the ux3 folder: the row whose leftmost cells are in the
    selection or directory colour and that the 'i' info names. Simpler and
    exact: Home then Down until 'osui files info /ux3' is logged."""
    for k in range(20):
        start = len(g.log_text())
        key(monitor, "home" if k == 0 else "down")
        key(monitor, "i")
        try:
            g.wait_for(proc, "osui files info ", 6.0, start)
        except RuntimeError:
            continue
        if "osui files info /ux3  dossier" in g.log_text()[start:]:
            return 2 + k
    raise RuntimeError("ux3 not found in the explorer listing")


if __name__ == "__main__":
    try:
        raise SystemExit(main())
    except Exception as error:
        print("QEMU GUI apps failed: %s" % error, file=sys.stderr)
        raise SystemExit(1)
