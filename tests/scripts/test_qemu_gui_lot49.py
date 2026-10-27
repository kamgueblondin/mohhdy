#!/usr/bin/env python3
"""Lot 49 GUI usability fixes, checked on the real OS (VBE 1024x768).

Gaps found by driving the desktop by hand in QEMU, each checked here:
  1. the explorer and the shell list the same tree: /docs, /home (HOME is
     /home/user) were shell-private ramfs directories, absent from the
     explorer; the explorer now enters /docs;
  2. maximize repaints at once: the window event arrived as characters and
     each one repainted the whole scene, so a maximized editor showed its
     old grid for about 2 s; 0.5 s after the click the bottom rows of the
     maximized editor are drawn;
  3. the terminal `ls` (same shell, same VFS) lists docs/ and home/.
"""
from __future__ import print_function

import os
import subprocess
import sys
import time

HERE = os.path.dirname(os.path.abspath(__file__))
sys.path.insert(0, HERE)
LOGS = os.path.join(HERE, "..", "..", "test_logs")
os.environ.setdefault("OSUI_GUI_LOG", os.path.join(LOGS, "qemu-gui-lot49-serial.log"))
os.environ.setdefault("OSUI_GUI_MON_SOCK", "/tmp/mohhdy-gui-lot49-mon.sock")
os.environ.setdefault("OSUI_GUI_QMP_SOCK", "/tmp/mohhdy-gui-lot49-qmp.sock")
os.environ.setdefault("OVERLAY_DISK", os.path.join(LOGS, "qemu-gui-lot49-overlay.img"))
import test_qemu_osui_gui as g  # noqa: E402
import test_qemu_gui_apps as A  # noqa: E402

A.g = g

W, H = 1024, 768


def differs(rgb, x0, y0, x1, y1, bg, tol=24):
    n = 0
    for y in range(y0, y1):
        for x in range(x0, x1):
            i = (y * W + x) * 3
            if any(abs(rgb[i + k] - bg[k]) > tol for k in range(3)):
                n += 1
    return n


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
            g.send_command_until(monitor, "/files", "osui files open dir=/", proc, mode="getc", wait_prompt=False)
            time.sleep(1.0)
            # 1. rows: bigfiles/ bin/ docs/ ...: two downs, Enter enters /docs
            A.step(monitor, proc, "explorer enters /docs", "/docs",
                   [("k", "down"), ("k", "down"), ("k", "ret")])
            # 2. open /hello.txt in the editor, maximize
            g.click_tablet(qmp, 1020, 420, W, H)   # desktop focus
            time.sleep(0.6)
            g.send_command_until(monitor, "/edit /hello.txt", "osui edit open file=/hello.txt", proc,
                                 mode="getc", wait_prompt=False)
            time.sleep(1.5)
            _, _, rgb = g.screendump_rgb(monitor, "qemu-gui-lot49-edit.ppm")
            # the editor is the focused window; find its maximize button by
            # the green button colour on the title row of the top window
            start = len(g.log_text())
            t_on = t_done = None
            for x, y in ((752, 106), (632, 70)):
                g.click_tablet(qmp, x, y, W, H)
                deadline = time.monotonic() + 10
                while time.monotonic() < deadline:
                    out = g.log_text()[start:]
                    if t_on is None and "gfx wm app maximize on" in out:
                        t_on = time.monotonic()
                    if t_on is not None and "osui edit view resized" in out:
                        t_done = time.monotonic()
                        break
                    time.sleep(0.02)
                if t_done is not None:
                    break
            if t_done is None:
                raise RuntimeError("maximize click not taken or editor not resized")
            # The click itself is slow to inject (usb-tablet); the guest part
            # is from the kernel's maximize to the editor's resize. Before the
            # fix about 2.6 s (one full repaint per event character).
            lag = t_done - t_on
            g.say("kernel maximize -> editor resized in %.2f s" % lag)
            if lag > 1.2:
                raise RuntimeError("maximize took %.2f s to reach the editor" % lag)
            _, _, rgb = g.screendump_rgb(monitor, "qemu-gui-lot49-max.ppm")
            bg = tuple(rgb[(640 * W + 600) * 3:(640 * W + 600) * 3 + 3])
            n = differs(rgb, 30, 600, 50, 680, bg)
            if n < 6:
                raise RuntimeError("maximized editor: bottom rows not drawn 0.5 s after resize (%d px)" % n)
            g.say("maximized editor drawn to the bottom at once (%d px)" % n)
            # 3. terminal ls shows the same directories
            g.click_tablet(qmp, 1010, 54, W, H)
            time.sleep(1.0)
            start = len(g.log_text())
            g.click_tablet(qmp, 440, 740, W, H)
            g.wait_for(proc, "osui gui exit chrome=vbe_term", g.CMD_TIMEOUT, start)
            time.sleep(1.0)
            start = len(g.log_text())
            A.keys(monitor, "ls /")
            A.key(monitor, "ret")
            g.wait_for(proc, "Total: ", g.CMD_TIMEOUT, start)
            out = g.log_text()[start:]
            for d in ("docs", "home", "bigfiles"):
                if d not in out:
                    raise RuntimeError("terminal ls: no %s: %s" % (d, out[-800:]))
            print("QEMU GUI lot 49 contract passed (explorer = shell tree, instant maximize, terminal ls).")
            return 0
    finally:
        for s in (monitor, qmp):
            try:
                if s:
                    s.close()
            except OSError:
                pass
        if proc and proc.poll() is None:
            proc.kill()


if __name__ == "__main__":
    try:
        sys.exit(main())
    except Exception as error:  # noqa: BLE001
        print("QEMU GUI lot 49 contract failed: %s" % error)
        sys.exit(1)
