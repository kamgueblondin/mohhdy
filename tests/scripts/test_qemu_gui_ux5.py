#!/usr/bin/env python3
"""UX5 desktop parity on the real OS (VBE desktop, keys through the QEMU
monitor, clicks through the USB tablet).

  * Editor (dynamic): /edit /samples/long.txt opens a 400-line, 28 KB file
    (over the old 96-line / 8 KB caps) in full through the user heap
    (SYS_HEAP_GROW + umalloc); maximize sends "app K size R C" and the
    editor follows the window; Save As of the 28 KB buffer reaches the
    writable overlay store, which refuses it (384-byte nodes): the error
    is shown, nothing is cut silently.
  * Viewer: /view /samples/mire.ppm (160x120 P6, 57 KB) is drawn as real
    pixels in the window (screen holds its red, green, blue quarters);
    closing the viewer clears the image.
"""
from __future__ import print_function

import os
import subprocess
import sys
import time

HERE = os.path.dirname(os.path.abspath(__file__))
sys.path.insert(0, HERE)
LOGS = os.path.join(HERE, "..", "..", "test_logs")
os.environ.setdefault("OSUI_GUI_LOG", os.path.join(LOGS, "qemu-gui-ux5-serial.log"))
os.environ.setdefault("OSUI_GUI_MON_SOCK", "/tmp/mohhdy-gui-ux5-mon.sock")
os.environ.setdefault("OSUI_GUI_QMP_SOCK", "/tmp/mohhdy-gui-ux5-qmp.sock")
os.environ.setdefault("OVERLAY_DISK", os.path.join(LOGS, "qemu-gui-ux5-overlay.img"))
import test_qemu_osui_gui as g  # noqa: E402

W, H = 1024, 768
FRAME = (0x0E, 0x14, 0x1B)   # opaque app window body (kernel/gfx_desktop.c)
ALIASES = {" ": "spc", "-": "minus", ".": "dot", "/": "slash", "=": "equal", "_": "shift-minus"}


def say(msg):
    g.say(msg)


def key(monitor, name, delay=0.15):
    g.send_key(monitor, name)
    time.sleep(delay)


def type_text(monitor, text):
    for ch in text:
        if ch in ALIASES:
            key(monitor, ALIASES[ch])
        elif ch.isupper():
            key(monitor, "shift-" + ch.lower())
        else:
            key(monitor, ch)


def slash(monitor, proc, line, marker):
    start = len(g.log_text())
    g.send_command_until(monitor, line, marker, proc, mode="getc", wait_prompt=False)
    time.sleep(0.8)
    return start


def desktop_focus(qmp):
    g.click_tablet(qmp, 1020, 420, W, H)
    time.sleep(0.6)


def pixel(rgb, x, y):
    i = (y * W + x) * 3
    return tuple(rgb[i:i + 3])


def near(p, c, tol=6):
    return all(abs(p[k] - c[k]) <= tol for k in range(3))


def editor_steps(monitor, qmp, proc):
    start = slash(monitor, proc, "/edit /samples/long.txt", "osui edit open file=/samples/long.txt")
    g.wait_for(proc, "osui edit lines=400 complete", g.CMD_TIMEOUT, start)
    say("editor opened the 400-line 28 KB file in full (user heap)")
    rgb = g.screendump_rgb(monitor, "qemu-gui-ux5-edit.ppm")[2]
    if near(pixel(rgb, 900, 300), FRAME):
        raise RuntimeError("editor window already wide before maximize")
    # slot 0 window: x=24, w=640, y=bar_h+20=54; maximize button center
    bar_h = max(28, min(40, H // 22))
    bx = 24 + 640 - 2 - 2 * (14 + 4) + 7
    by = bar_h + 20 + 2 + (30 - 14) // 2 + 7
    start = len(g.log_text())
    g.click_tablet(qmp, bx, by, W, H)
    g.wait_for(proc, "osui edit view resized", g.CMD_TIMEOUT, start)
    time.sleep(1.0)
    rgb = g.screendump_rgb(monitor, "qemu-gui-ux5-edit-max.ppm")[2]
    if not near(pixel(rgb, 900, 300), FRAME) and not near(pixel(rgb, 900, 300), (9, 13, 18)):
        raise RuntimeError("maximized editor does not cover x=900: %r" % (pixel(rgb, 900, 300),))
    say("maximize: kernel sent app size, editor grid follows the window")
    # Save As /long2.txt: the 28 KB buffer is built on the heap. Since Lot 44
    # the overlay stores it in chained blocks (bound 30 KiB) through the
    # chunked atadriver relay; fsops_write fails on any short write, so
    # "saved" means all 27993 bytes were written.
    start = len(g.log_text())
    key(monitor, "esc")
    key(monitor, "a")
    for _ in range(len("/samples/long.txt")):
        key(monitor, "backspace", 0.08)
    type_text(monitor, "/long2.txt")
    key(monitor, "ret")
    g.wait_for(proc, "osui edit saved file=/long2.txt", g.CMD_TIMEOUT, start)
    if "osui edit save error=" in g.log_text()[start:]:
        raise RuntimeError("Save As of 28 KB reported a store error")
    say("Save As of 28 KB: saved whole (overlay chained blocks, chunked relay)")
    # the maximized editor covers the desktop edge: close it (ESC Q)
    start = len(g.log_text())
    key(monitor, "esc")
    key(monitor, "shift-q")
    g.wait_for(proc, "osui edit close file=", g.CMD_TIMEOUT, start)
    desktop_focus(qmp)


def count_near(rgb, color, tol=10):
    n = 0
    for i in range(0, len(rgb), 3 * 5):
        if all(abs(rgb[i + k] - color[k]) <= tol for k in range(3)):
            n += 1
    return n


def viewer_steps(monitor, qmp, proc):
    # test card 160x120 P6 (57 KB, read through the heap): four colored
    # quarters drawn as real pixels by the compositor, scaled in the window
    before = g.screendump_rgb(monitor, "qemu-gui-ux5-before-view.ppm")[2]
    start = slash(monitor, proc, "/view /samples/mire.ppm", "osui tool open app=Visionneuse")
    g.wait_for(proc, "osui tool line=image 160x120 affichee en pixels 160x120", g.CMD_TIMEOUT, start)
    time.sleep(1.0)
    rgb = g.screendump_rgb(monitor, "qemu-gui-ux5-view.ppm")[2]
    for name, c in (("red", (230, 30, 30)), ("green", (30, 200, 60)), ("blue", (40, 80, 230))):
        n0, n1 = count_near(before, c), count_near(rgb, c)
        if n1 - n0 < 600:
            raise RuntimeError("viewer %s quarter not on screen in pixels (%d -> %d)" % (name, n0, n1))
    say("viewer draws the PPM test card as real pixels (red/green/blue quarters on screen)")
    key(monitor, "esc")
    g.wait_for(proc, "osui tool close app=Visionneuse", g.CMD_TIMEOUT, start)
    time.sleep(0.8)
    rgb = g.screendump_rgb(monitor, "qemu-gui-ux5-view-closed.ppm")[2]
    if count_near(rgb, (40, 80, 230)) - count_near(before, (40, 80, 230)) > 200:
        raise RuntimeError("image still drawn after the viewer closed")
    say("viewer closed: image cleared")


def tool_resize_steps(monitor, qmp, proc):
    """Lot 45 (US-G15): a tool window (/help) follows its grid: maximize
    sends "app K size R C", the Aide window shows more rows."""
    start = slash(monitor, proc, "/help", "osui tool open app=Aide")
    g.wait_for(proc, "osui panel run cmd=gui-help ok", g.CMD_TIMEOUT, start)
    bar_h = max(28, min(40, H // 22))
    bx = 24 + 640 - 2 - 2 * (14 + 4) + 7
    by = bar_h + 20 + 2 + (30 - 14) // 2 + 7
    start = len(g.log_text())
    g.click_tablet(qmp, bx, by, W, H)
    g.wait_for(proc, "osui tool view resized app=Aide rows=", g.CMD_TIMEOUT, start)
    import re
    m = re.search(r"osui tool view resized app=Aide rows=(\d+)", g.log_text()[start:])
    if not m or int(m.group(1)) <= 22:
        raise RuntimeError("maximized Aide window did not get more rows: %r" % (m.group(0) if m else None))
    time.sleep(1.0)
    g.screendump_rgb(monitor, "qemu-gui-lot45-help-max.ppm")
    say("maximize: the Aide tool window follows its grid (%s rows)" % m.group(1))
    start = len(g.log_text())
    key(monitor, "esc")
    key(monitor, "esc")
    g.wait_for(proc, "osui tool close app=Aide", g.CMD_TIMEOUT, start)
    desktop_focus(qmp)


def max_button(qmp):
    bar_h = max(28, min(40, H // 22))
    bx = 24 + 640 - 2 - 2 * (14 + 4) + 7
    by = bar_h + 20 + 2 + (30 - 14) // 2 + 7
    g.click_tablet(qmp, bx, by, W, H)


def rewrap_steps(monitor, qmp, proc):
    """Lot 46: the output already shown in a tool window is wrapped again
    when the window is maximized; the file explorer columns
    follow the window width (layout asserted in test_osui_files)."""
    import re
    slash(monitor, proc, "/cmd", "osui tool open app=Commande")
    start = len(g.log_text())
    type_text(monitor, "ata-status")
    key(monitor, "ret")
    g.wait_for(proc, "osui panel run cmd=ata-status ok", g.CMD_TIMEOUT, start)
    # before the resize the lines are cut at the default width (76 columns)
    tails = re.findall(r"osui panel tail=(.*)", g.log_text()[start:])
    if not tails or max(len(x) for x in tails) > 76:
        raise RuntimeError("ata-status not wrapped at the default width: %r" % tails)
    spec = r"osui tool view resized app=Commande rows=(\d+) cols=(\d+) lines=(\d+) w1=(\d+)"
    start = len(g.log_text())
    max_button(qmp)
    g.wait_for(proc, "osui tool view resized app=Commande rows=", g.CMD_TIMEOUT, start)
    big = re.search(spec, g.log_text()[start:])
    if not big:
        raise RuntimeError("no resize trace for the Commande window")
    b = [int(x) for x in big.groups()]
    # w1: the first output line, now longer than the default grid allows
    if not (b[1] > 77 and 76 < b[3] < b[1]):
        raise RuntimeError("ata-status output not rewrapped at the new width: %r" % (b,))
    time.sleep(1.0)
    g.screendump_rgb(monitor, "qemu-gui-lot46-cmd-max.ppm")
    say("maximize: ata-status output rewrapped at %d columns (first line %d chars, %d lines)" % (b[1], b[3], b[2]))
    start = len(g.log_text())
    key(monitor, "esc")
    g.wait_for(proc, "osui tool close app=Commande", g.CMD_TIMEOUT, start)
    desktop_focus(qmp)
    start = slash(monitor, proc, "/files /samples", "osui files open dir=/samples")
    start = len(g.log_text())
    max_button(qmp)
    g.wait_for(proc, "osui files view resized cols=", g.CMD_TIMEOUT, start)
    m = re.search(r"osui files view resized cols=(\d+)", g.log_text()[start:])
    if not m or int(m.group(1)) <= 77:
        raise RuntimeError("maximized explorer did not get more columns: %r" % (m.group(0) if m else None))
    time.sleep(1.0)
    g.screendump_rgb(monitor, "qemu-gui-lot46-files-max.ppm")
    say("maximize: the file explorer gets %s columns (name column widens)" % m.group(1))
    start = len(g.log_text())
    key(monitor, "esc")
    time.sleep(0.6)
    desktop_focus(qmp)


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
            editor_steps(monitor, qmp, proc)
            viewer_steps(monitor, qmp, proc)
            tool_resize_steps(monitor, qmp, proc)
            rewrap_steps(monitor, qmp, proc)
        say("QEMU UX5 GUI parity contract passed.")
        return 0
    finally:
        if qmp is not None:
            qmp.close()
        if monitor is not None:
            monitor.close()
        g.terminate(proc)


if __name__ == "__main__":
    sys.exit(main())
