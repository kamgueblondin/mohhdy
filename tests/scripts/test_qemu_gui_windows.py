#!/usr/bin/env python3
"""Every OS-UI window has the shared window manager chrome (gfx_wm.c).

On the VBE desktop (1024x768) through the USB tablet:
  * Statut opens with live content (teal first line from osui_panes.c)
    and the menu bar highlights Statut;
  * minimize hides the pane and puts an amber mark under its dock icon,
    the dock icon restores it, maximize toggles, the title bar drags it,
    close closes it;
  * the chat window close button hides it, the Chat menu reopens it;
  * the terminal minimize button keeps the typed line: the Shell dock
    icon reopens the terminal with the line restored.
"""
from __future__ import print_function

import os
import subprocess
import sys
import time

HERE = os.path.dirname(os.path.abspath(__file__))
sys.path.insert(0, HERE)
LOGS = os.path.join(HERE, "..", "..", "test_logs")
os.environ.setdefault("OSUI_GUI_LOG", os.path.join(LOGS, "qemu-gui-windows-serial.log"))
os.environ.setdefault("OSUI_GUI_MON_SOCK", "/tmp/mohhdy-gui-windows-mon.sock")
os.environ.setdefault("OSUI_GUI_QMP_SOCK", "/tmp/mohhdy-gui-windows-qmp.sock")
os.environ.setdefault("OVERLAY_DISK", os.path.join(LOGS, "qemu-gui-windows-overlay.img"))
import test_qemu_osui_gui as g  # noqa: E402
import test_qemu_gui_term_window as tw  # noqa: E402

W, H = 1024, 768
BAR_H = 34
PANE = (36, BAR_H + 28, 1024 - 420, 548)      # default pane window
CHAT_CENTER = (256, BAR_H + 768 // 14, 512, 320)


def btn(win, which, title_h):
    x, y, w, _h = win
    k = {"close": 1, "max": 2, "min": 3}[which]
    return x + w - 2 - k * 18 + 7, y + 2 + (title_h - 14) // 2 + 7


def pixel(rgb, x, y):
    i = (y * W + x) * 3
    return tuple(rgb[i:i + 3])


def count(rgb, x0, y0, x1, y1, color, tol=12):
    n = 0
    for y in range(y0, y1):
        for x in range(x0, x1):
            p = pixel(rgb, x, y)
            if all(abs(p[k] - color[k]) <= tol for k in range(3)):
                n += 1
    return n


def click(qmp, proc, x, y, marker, label):
    start = len(g.log_text())
    for _ in range(3):
        g.click_tablet(qmp, x, y, W, H)
        try:
            g.wait_for(proc, marker, 8.0, start)
            g.say("%s -> %s" % (label, marker))
            time.sleep(0.8)
            return start
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
            hits = g.desktop_hit_centers(W, H)
            docks, menu = hits["docks"], hits["menu"]
            # Statut via the dock: live content + menu highlight
            click(qmp, proc, docks[5][0], docks[5][1], "line=/status", "dock Statut")
            time.sleep(2.5)
            _, _, rgb = g.screendump_rgb(monitor, "qemu-gui-win-status.ppm")
            x, y, w, h = PANE
            teal = count(rgb, x + 18, y + 94, x + w - 20, y + 108, (122, 212, 196))
            if teal < 40:
                raise RuntimeError("Statut window has no live first line (teal px %d)" % teal)
            hl = count(rgb, menu[5][0] - 20, 2, menu[5][0] + 20, BAR_H - 2, (61, 154, 138), 4)
            if hl < 100:
                raise RuntimeError("menu bar does not highlight Statut (%d px)" % hl)
            if count(rgb, x + w - 60, y + 4, x + w, y + 30, (0xE0, 0x52, 0x4B), 4) < 60:
                raise RuntimeError("no red close button on the pane title bar")
            g.say("Statut: live content, menu highlight, close/min/max buttons")
            # minimize, amber dock mark, restore from the dock
            click(qmp, proc, *btn(PANE, "min", 30), marker="gfx wm pane minimize", label="pane minimize")
            _, _, rgb = g.screendump_rgb(monitor, "qemu-gui-win-min.ppm")
            if count(rgb, docks[5][0] - 6, docks[5][1] + 14, docks[5][0] + 6, docks[5][1] + 22, (230, 180, 60), 4) < 10:
                raise RuntimeError("no minimized mark under the Statut dock icon")
            if count(rgb, x + 18, y + 94, x + w - 20, y + 108, (122, 212, 196)) > 10:
                raise RuntimeError("minimized pane still drawn")
            click(qmp, proc, docks[5][0], docks[5][1], "gfx wm pane restore", "dock Statut restore")
            # maximize on / off, drag
            click(qmp, proc, *btn(PANE, "max", 30), marker="gfx wm pane maximize on", label="pane maximize")
            click(qmp, proc, *btn((0, BAR_H + 4, W, H - BAR_H - 4 - 52), "max", 30),
                  marker="gfx wm pane maximize off", label="pane maximize off")
            start = len(g.log_text())
            tw.move(qmp, x + 80, y + 12)
            tw.button(qmp, True)
            tw.move(qmp, x + 180, y + 72)
            tw.button(qmp, False)
            g.wait_for(proc, "gfx wm pane moved", g.CMD_TIMEOUT, start)
            time.sleep(1.0)
            _, _, rgb = g.screendump_rgb(monitor, "qemu-gui-win-moved.ppm")
            moved = (x + 100, y + 60, w, h)
            if count(rgb, moved[0] + moved[2] - 60, moved[1] + 4, moved[0] + moved[2], moved[1] + 30,
                     (0xE0, 0x52, 0x4B), 4) < 60:
                raise RuntimeError("pane title bar not at the dragged place")
            g.say("pane dragged by its title bar")
            click(qmp, proc, *btn(moved, "close", 30), marker="gfx wm pane close", label="pane close")
            g.wait_for(proc, "line=/center", g.CMD_TIMEOUT)
            time.sleep(1.5)
            # chat close and reopen from the menu
            click(qmp, proc, *btn(CHAT_CENTER, "close", 34), marker="gfx wm chat close", label="chat close")
            click(qmp, proc, menu[0][0], menu[0][1], "gfx wm chat restore", "menu Chat")
            # terminal minimize keeps the typed line
            win = tw.open_terminal(monitor, proc)
            g.send_command_once(monitor, "uptime", proc, "gets", enter=False)
            time.sleep(0.5)
            click(qmp, proc, *tw.button_center(win, "min"), marker="gfx term button minimize", label="terminal minimize")
            g.wait_for(proc, "osui gui live", g.CMD_TIMEOUT)
            time.sleep(1.5)
            _, _, rgb = g.screendump_rgb(monitor, "qemu-gui-win-term-min.ppm")
            if count(rgb, docks[2][0] - 6, docks[2][1] + 14, docks[2][0] + 6, docks[2][1] + 22, (230, 180, 60), 4) < 10:
                raise RuntimeError("no minimized mark under the Shell dock icon")
            start = click(qmp, proc, docks[2][0], docks[2][1], "SYS_GETS: restored minimized line", "dock Shell")
            time.sleep(0.8)
            g.send_key(monitor, "ret")
            g.wait_for(proc, "SYS_GETS: ligne lue: uptime", g.CMD_TIMEOUT, start)
            g.wait_for(proc, "PIT ticks", g.CMD_TIMEOUT, start)
            g.say("terminal minimize kept the typed line 'uptime'")
        g.say("QEMU GUI windows contract passed.")
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
        print("QEMU GUI windows failed: %s" % error, file=sys.stderr)
        raise SystemExit(1)
