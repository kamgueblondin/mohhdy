#!/usr/bin/env python3
"""Lot 69: one window chrome for every window type, clicked through the
USB tablet on the VBE desktop (1024x768, ui-scale pinned to 1x so the
pixel layout is fixed):

  * Statut pane: minimize / dock restore / maximize on-off / corner resize
    (serial "gfx wm pane resized") / close;
  * chat window: maximize on-off and close / Chat menu restore;
  * full screen Browser-OS web page (/browse, local site): the shared
    title buttons replace the old traffic dots: minimize shows the desktop
    and the Browser-OS dock icon restores it, maximize is a no-op (already
    full screen), close closes the page ("osui web close").
  * explorer app window (/files): maximize on-off, minimize / Fichiers
    dock restore, right edge resize (the close button follows), close;
  * full screen editor (pm-ide): minimize / Editeur dock restore, close.
The host test test_gfx_desktop (test_lot69_every_window_shared_chrome)
checks the button geometry of every window type.
"""
from __future__ import print_function

import os
import subprocess
import sys
import time

HERE = os.path.dirname(os.path.abspath(__file__))
sys.path.insert(0, HERE)
LOGS = os.path.join(HERE, "..", "..", "test_logs")
os.environ.setdefault("OSUI_GUI_LOG", os.path.join(LOGS, "qemu-gui-lot69-win-serial.log"))
os.environ.setdefault("OSUI_GUI_MON_SOCK", "/tmp/mohhdy-gui-lot69-win-mon.sock")
os.environ.setdefault("OSUI_GUI_QMP_SOCK", "/tmp/mohhdy-gui-lot69-win-qmp.sock")
os.environ.setdefault("OVERLAY_DISK", os.path.join(LOGS, "qemu-gui-lot69-win-overlay.img"))
import test_qemu_osui_gui as g  # noqa: E402
import test_qemu_gui_term_window as tw  # noqa: E402
from test_qemu_gui_windows import btn, click, count, PANE, CHAT_CENTER, W, H, BAR_H  # noqa: E402

RED = (0xE0, 0x52, 0x4B)


def red_button(rgb, y0, y1):
    """centre of the rightmost red close button between rows y0..y1"""
    best = None
    for y in range(y0, y1):
        for x in range(W - 1, 0, -1):
            i = (y * W + x) * 3
            p = rgb[i:i + 3]
            if abs(p[0] - RED[0]) <= 4 and abs(p[1] - RED[1]) <= 4 and abs(p[2] - RED[2]) <= 4:
                if best is None or x > best[0]:
                    best = (x, y)
                break
    if best is None:
        raise RuntimeError("no red close button in rows %d..%d" % (y0, y1))
    return best[0] - 7, best[1] + 4


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
                "qemu-system-i386", "-fw_cfg", "name=opt/mohhdy/ui-scale,string=4", "-cpu", "pentium3",
                "-kernel", g.KERNEL, "-initrd", g.INITRD, "-m", "1024M", "-display", "none", "-vga", "std",
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
            hits = g.desktop_hit_centers(W, H)
            docks, menu = hits["docks"], hits["menu"]
            # 1. pane: min / restore / max / resize / close
            click(qmp, proc, docks[5][0], docks[5][1], "line=/status", "dock Statut")
            time.sleep(2.0)
            x, y, w, h = PANE
            click(qmp, proc, *btn(PANE, "min", 30), marker="gfx wm pane minimize", label="pane minimize")
            click(qmp, proc, docks[5][0], docks[5][1], "gfx wm pane restore", "dock Statut restore")
            click(qmp, proc, *btn(PANE, "max", 30), marker="gfx wm pane maximize on", label="pane maximize")
            click(qmp, proc, *btn((0, BAR_H + 4, W, H - BAR_H - 4 - 52), "max", 30),
                  marker="gfx wm pane maximize off", label="pane maximize off")
            start = len(g.log_text())
            tw.move(qmp, x + w - 3, y + h - 3)
            tw.button(qmp, True)
            tw.move(qmp, x + w - 103, y + h - 83)
            tw.button(qmp, False)
            g.wait_for(proc, "gfx wm pane resized", g.CMD_TIMEOUT, start)
            time.sleep(1.0)
            _, _, rgb = g.screendump_rgb(monitor, "qemu-gui-lot69-resized.ppm")
            small = (x, y, w - 100, h - 80)
            if count(rgb, small[0] + small[2] - 60, y + 4, small[0] + small[2], y + 30, RED, 4) < 60:
                raise RuntimeError("resized pane: close button not at the new right edge")
            g.say("pane resized by its corner grip (buttons follow the edge)")
            click(qmp, proc, *btn(small, "close", 30), marker="gfx wm pane close", label="pane close")
            g.wait_for(proc, "line=/center", g.CMD_TIMEOUT)
            time.sleep(1.5)
            # 2. chat: max on / off, close, menu restore
            click(qmp, proc, *btn(CHAT_CENTER, "max", 34), marker="gfx wm chat maximize on", label="chat maximize")
            click(qmp, proc, *btn((0, BAR_H + 4, W, H - BAR_H - 4 - 52), "max", 34),
                  marker="gfx wm chat maximize off", label="chat maximize off")
            click(qmp, proc, *btn(CHAT_CENTER, "close", 34), marker="gfx wm chat close", label="chat close")
            click(qmp, proc, menu[0][0], menu[0][1], "gfx wm chat restore", "menu Chat")
            # 3. full screen web page
            start = len(g.log_text())
            g.send_command_until(monitor, "/browse", "osui web render file=/web/index.html", proc,
                                 mode="getc", wait_prompt=False)
            time.sleep(3.0)
            full = (8, BAR_H + 8, W - 16, H - BAR_H - 16)
            _, _, rgb = g.screendump_rgb(monitor, "qemu-gui-lot69-web.ppm")
            if count(rgb, W - 70, BAR_H + 8, W - 8, BAR_H + 40, RED, 4) < 60:
                raise RuntimeError("web window has no shared red close button at the right")
            click(qmp, proc, *btn(full, "min", 30), marker="gfx wm full minimize", label="web minimize")
            _, _, rgb = g.screendump_rgb(monitor, "qemu-gui-lot69-web-min.ppm")
            if count(rgb, W - 70, BAR_H + 8, W - 8, BAR_H + 40, RED, 4) >= 60:
                raise RuntimeError("minimized web window still drawn")
            click(qmp, proc, docks[1][0], docks[1][1], "gfx wm full restore", "dock Browser-OS restore")
            click(qmp, proc, *btn(full, "max", 30), marker="gfx wm full already maximized", label="web maximize")
            click(qmp, proc, *btn(full, "close", 30), marker="gfx wm full close", label="web close")
            g.wait_for(proc, "osui web close", g.CMD_TIMEOUT, start)
            # 4. explorer app window: locate its red close button on screen
            # (the chat is closed first: its close button is in the same rows)
            time.sleep(1.0)
            click(qmp, proc, *btn(CHAT_CENTER, "close", 34), marker="gfx wm chat close", label="chat close again")
            g.send_command_until(monitor, "/files", "osui files open dir=/", proc, mode="getc", wait_prompt=False)
            time.sleep(1.5)
            _, _, rgb = g.screendump_rgb(monitor, "qemu-gui-lot69-app.ppm")
            cx, cy = red_button(rgb, BAR_H + 8, BAR_H + 60)
            app = (0, cy - 9, cx + 9 + 2, 0)   # right edge = close right + pad
            click(qmp, proc, cx - 18, cy, "gfx wm app maximize on", "app maximize")
            _, _, rgb = g.screendump_rgb(monitor, "qemu-gui-lot69-app-max.ppm")
            mx, my = red_button(rgb, BAR_H, BAR_H + 40)
            click(qmp, proc, mx - 18, my, "gfx wm app maximize off", "app maximize off")
            click(qmp, proc, cx - 36, cy, "gfx wm app minimize", "app minimize")
            click(qmp, proc, docks[6][0], docks[6][1], "gfx wm app restore", "dock Fichiers restore")
            _, _, rgb = g.screendump_rgb(monitor, "qemu-gui-lot69-app-restored.ppm")
            cx, cy = red_button(rgb, BAR_H + 8, BAR_H + 60)
            right = cx + 7 + 4 + 2          # close right edge + gap + pad
            start = len(g.log_text())
            tw.move(qmp, right - 3, cy + 60)
            time.sleep(0.3)
            tw.button(qmp, True)
            time.sleep(0.3)
            tw.move(qmp, right - 83, cy + 60)
            time.sleep(0.3)
            tw.button(qmp, False)
            g.wait_for(proc, "gfx wm app resized", g.CMD_TIMEOUT, start)
            time.sleep(1.0)
            _, _, rgb = g.screendump_rgb(monitor, "qemu-gui-lot69-app-resized.ppm")
            rx, ry = red_button(rgb, BAR_H + 8, BAR_H + 60)
            if abs((cx - rx) - 80) > 3:
                raise RuntimeError("app resize: close button moved by %d, not 80" % (cx - rx))
            g.say("app window resized by its right edge (close button followed)")
            click(qmp, proc, rx, ry, "gfx wm app close", "app close")
            time.sleep(1.0)
            # 5. full screen editor
            g.send_command_until(monitor, "pm-ide w69.pm", "osui pm-ide open fullscreen", proc,
                                 mode="getc", wait_prompt=False)
            time.sleep(1.5)
            _, _, rgb = g.screendump_rgb(monitor, "qemu-gui-lot69-ide.ppm")
            if count(rgb, W - 70, BAR_H + 8, W - 8, BAR_H + 40, RED, 4) < 60:
                raise RuntimeError("editor window has no shared red close button")
            click(qmp, proc, *btn(full, "min", 30), marker="gfx wm full minimize", label="editor minimize")
            click(qmp, proc, docks[7][0], docks[7][1], "gfx wm full restore", "dock Editeur restore")
            start = len(g.log_text())
            click(qmp, proc, *btn(full, "close", 30), marker="gfx wm full close", label="editor close")
            g.wait_for(proc, "osui pm-ide quit", g.CMD_TIMEOUT, start)
        g.say("QEMU GUI lot69 windows contract passed.")
        return 0
    finally:
        if qmp is not None:
            qmp.close()
        if monitor is not None:
            monitor.close()
        g.terminate(proc)


if __name__ == "__main__":
    try:
        raise SystemExit(main())
    except Exception as error:
        print("QEMU GUI lot69 windows failed: %s" % error, file=sys.stderr)
        raise SystemExit(1)
