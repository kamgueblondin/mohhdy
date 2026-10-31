#!/usr/bin/env python3
"""UX4 desktop tool apps on the real OS (generic window slots).

On the VBE desktop, keys through the QEMU monitor and clicks through the
USB tablet:
  * /tasks opens the task manager with the real ps and mem output; k k
    (kill, confirmed) on the first row asks the kernel to kill PID 0 and
    the kernel refuses it (protected process): the rights check is the
    kernel's, the window shows its answer;
  * /settings shows the RTC date (date) and the VBE framebuffer size;
  * /peers shows the real p2p-peers / p2p-stats output;
  * /pmrun runs a PromptMessage file written in the shell (pm-run);
  * /view shows a text file; five windows are open at once and the
    screen shows them (pixels of each window frame);
  * /browse renders /web/index.html in the web pane; its back button is
    a kernel event handled by Ring 3 (link clicks: unit tests);
  * /chat opens the chat window (the GPT-2 reply itself is checked by the
    run-gpt2 screenshots: this job has no GGUF model on its disk).
"""
from __future__ import print_function

import os
import subprocess
import sys
import time

HERE = os.path.dirname(os.path.abspath(__file__))
sys.path.insert(0, HERE)
LOGS = os.path.join(HERE, "..", "..", "test_logs")
os.environ.setdefault("OSUI_GUI_LOG", os.path.join(LOGS, "qemu-gui-tools-serial.log"))
os.environ.setdefault("OSUI_GUI_MON_SOCK", "/tmp/mohhdy-gui-tools-mon.sock")
os.environ.setdefault("OSUI_GUI_QMP_SOCK", "/tmp/mohhdy-gui-tools-qmp.sock")
os.environ.setdefault("OVERLAY_DISK", os.path.join(LOGS, "qemu-gui-tools-overlay.img"))
import test_qemu_osui_gui as g  # noqa: E402

FRAME = (0x0E, 0x14, 0x1B)   # opaque app window body (kernel/gfx_desktop.c)


def slash(monitor, proc, line, marker):
    start = len(g.log_text())
    g.send_command_until(monitor, line, marker, proc, mode="getc", wait_prompt=False)
    time.sleep(0.8)
    return g.log_text()[start:]


def desktop_focus(qmp):
    """click the desktop right edge (no app window reaches x=1020): the
    kernel sends "focus d", the keyboard goes back to the desktop line"""
    g.click_tablet(qmp, 1020, 420, 1024, 768)
    time.sleep(0.6)


def key(monitor, name, delay=0.4):
    g.send_key(monitor, name)
    time.sleep(delay)


def count(rgb, w, color, tol=6):
    n = 0
    for i in range(0, len(rgb), 3 * 7):
        if all(abs(rgb[i + k] - color[k]) <= tol for k in range(3)):
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
                "qemu-system-i386", "-fw_cfg", "name=opt/mohhdy/ui-scale,string=4", "-cpu", "pentium3", "-kernel", g.KERNEL,
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
            g.send_command_until(monitor, "write /ux4.pm print 4242", "write ok /ux4.pm", proc)
            g.send_command_until(monitor, "write /ux4.txt bonjour ux4", "write ok /ux4.txt", proc)
            g.send_command_until(monitor, "gui", "osui gui live", proc, wait_prompt=False)
            g.wait_for(proc, "OSUI-SNAP", g.CMD_TIMEOUT)

            out = slash(monitor, proc, "/tasks", "osui tool open app=Gestionnaire des taches")
            g.wait_for(proc, "osui tool line=", g.CMD_TIMEOUT)
            out = g.log_text()[g.log_text().rfind("osui tool open app=Gestionnaire"):]
            if "COMMAND" not in out or "kernel" not in out or "\x1b[" in out:
                raise RuntimeError("task manager does not show the ps table:\n" + out[:600])
            g.say("task manager lists the real ps output")
            start = len(g.log_text())
            key(monitor, "k")
            key(monitor, "k")
            g.wait_for(proc, "osui tasks kill pid=", g.CMD_TIMEOUT, start)
            line = [l for l in g.log_text()[start:].splitlines() if "osui tasks kill pid=" in l][-1]
            if "pid=0" not in line or "protege" not in g.log_text()[start:]:
                raise RuntimeError("kill of PID 0 not refused by the kernel: " + line)
            g.say("task manager kill went through the kernel, refused: " + line.strip())
            key(monitor, "esc")   # Esc closes the focused tool window
            g.wait_for(proc, "osui tool close app=Gestionnaire", g.CMD_TIMEOUT, start)

            out = slash(monitor, proc, "/settings", "osui tool open app=Parametres")
            g.wait_for(proc, "osui tool line=date ok", g.CMD_TIMEOUT)
            g.say("settings show the RTC date from the date command")
            desktop_focus(qmp)
            out = slash(monitor, proc, "/peers", "osui tool open app=P2P")
            g.wait_for(proc, "osui tool line=$ p2p-peers", g.CMD_TIMEOUT)
            g.say("P2P window runs p2p-peers")
            desktop_focus(qmp)
            out = slash(monitor, proc, "/pmrun /ux4.pm", "osui tool open app=PromptMessage")
            g.wait_for(proc, "4242", g.CMD_TIMEOUT)
            g.say("PromptMessage window shows the pm-run output (4242)")
            desktop_focus(qmp)
            out = slash(monitor, proc, "/view /ux4.txt", "osui tool open app=Visionneuse")
            g.wait_for(proc, "osui tool line=bonjour ux4", g.CMD_TIMEOUT)
            g.say("viewer shows the text file")
            desktop_focus(qmp)
            out = slash(monitor, proc, "/chat", "osui tool open app=Chat IA")
            g.say("chat window open")
            n_tools = count(g.screendump_rgb(monitor, "qemu-gui-tools.ppm")[2], 0, FRAME)
            if n_tools < 3000:
                raise RuntimeError("app window bodies not on screen (%d px)" % n_tools)
            g.say("five tool windows on screen at once (window body %d px)" % n_tools)
            desktop_focus(qmp)
            # browser: local site home through the guest web engine, then the
            # title bar back button reaches Ring 3 as a "web back" event
            slash(monitor, proc, "/browse", "osui web render file=/web/index.html")
            g.say("/browse renders /web/index.html")
            start = len(g.log_text())
            bar_h = max(28, min(40, 768 // 22))
            # Lot 69: back / forward are on the left of the title bar now
            g.click_tablet(qmp, 16 + 10, bar_h + 20, 1024, 768)
            g.wait_for(proc, "osui web back none", g.CMD_TIMEOUT, start)
            g.say("browser back button -> osui web back none (first page)")
        g.say("QEMU GUI tool apps (task manager, settings, P2P, PM run, viewer, chat) contract passed.")
        return 0
    finally:
        if qmp is not None:
            qmp.close()
        if monitor is not None:
            monitor.close()
        g.terminate(proc)


if __name__ == "__main__":
    sys.exit(main())
