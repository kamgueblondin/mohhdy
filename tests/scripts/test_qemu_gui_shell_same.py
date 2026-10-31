#!/usr/bin/env python3
"""The desktop terminal window is the console itself (one core, one shell).

Runs the same commands (clear, a console-only command, an OS-UI command,
the long help that scrolls) in text mode and then in the VBE terminal
window opened with /shell, dumps the visible 80x25 grid over serial with
term-dump each time and requires the two grids to be identical. Also
checks PgUp scrolls back in the window and console returns to text mode.
"""
from __future__ import print_function

import os
import re
import subprocess
import sys
import time

HERE = os.path.dirname(os.path.abspath(__file__))
sys.path.insert(0, HERE)
LOGS = os.path.join(HERE, "..", "..", "test_logs")
os.environ.setdefault("OSUI_GUI_LOG", os.path.join(LOGS, "qemu-gui-shell-same-serial.log"))
os.environ.setdefault("OSUI_GUI_MON_SOCK", "/tmp/mohhdy-gui-shell-mon.sock")
os.environ.setdefault("OSUI_GUI_QMP_SOCK", "/tmp/mohhdy-gui-shell-qmp.sock")
os.environ.setdefault("OVERLAY_DISK", os.path.join(LOGS, "qemu-gui-shell-same-overlay.img"))
import test_qemu_osui_gui as g  # noqa: E402

# clear first so both grids start empty; help is long and scrolls.
COMMANDS = [
    ("clear", None),
    ("p2p-status", "p2p"),
    ("session-status", "session"),
    ("help", "term-dump"),
]


def dump_grid(monitor, proc):
    start = g.send_command_until(monitor, "term-dump", "TERM-DUMP END", proc)
    text = g.normalized_log(g.log_text()[start:])
    head = re.search(r"TERM-DUMP BEGIN backend=(\w+)", text)
    body = text[text.find("TERM-DUMP BEGIN"):text.find("TERM-DUMP END")]
    rows = [line[5:] for line in body.splitlines() if line.startswith("TERM|")]
    if len(rows) != 25:
        raise RuntimeError("grid dump has %d rows: %r" % (len(rows), body[-800:]))
    return head.group(1) if head else "?", rows


def run_sequence(monitor, proc):
    for cmd, marker in COMMANDS:
        if marker:
            g.send_command_until(monitor, cmd, marker, proc)
        else:
            start = len(g.log_text())
            g.send_command_once(monitor, cmd, proc, "gets")
            g.wait_for(proc, "[0m : ", g.CMD_TIMEOUT, start)
            time.sleep(0.5)
    return dump_grid(monitor, proc)


def main():
    os.makedirs(g.LOG_DIR, exist_ok=True)
    for path in (g.LOG, g.MON_SOCK, g.QMP_SOCK):
        try:
            os.remove(path)
        except OSError:
            pass
    g.prepare_test_disk()
    proc = None
    monitor = None
    try:
        with open(os.devnull, "wb") as err:
            proc = subprocess.Popen([
                "qemu-system-i386", "-fw_cfg", "name=opt/mohhdy/ui-scale,string=4", "-cpu", "pentium3", "-kernel", g.KERNEL,
                "-initrd", g.INITRD, "-m", "1024M", "-display", "none", "-vga", "std",
                "-usb", "-device", "usb-tablet",
                "-serial", "file:" + g.LOG, "-monitor", "unix:%s,server,nowait" % g.MON_SOCK,
                "-machine", "type=pc,accel=tcg", "-no-reboot", "-no-shutdown",
            ] + g.qemu_disk_args(), cwd=g.ROOT, stdout=err, stderr=err)
            g.wait_for(proc, "[0m : ", g.BOOT_TIMEOUT)
            monitor = g.monitor_connect()
            time.sleep(0.6)
            back_a, grid_a = run_sequence(monitor, proc)
            if back_a != "text":
                raise RuntimeError("console dump backend %s" % back_a)
            g.say("console grid: %d non empty rows" % len([r for r in grid_a if r]))
            g.send_command_until(monitor, "gui", "osui gui live", proc, wait_prompt=False)
            g.wait_for(proc, "OSUI-SNAP", g.CMD_TIMEOUT)
            start = len(g.log_text())
            g.send_command_until(monitor, "/shell", "chrome=vbe_term", proc, mode="getc", wait_prompt=False)
            g.wait_for(proc, "gfx term on fb ", g.CMD_TIMEOUT, start)
            g.wait_for(proc, "[0m : ", g.CMD_TIMEOUT, start)
            back_b, grid_b = run_sequence(monitor, proc)
            if back_b != "vbe":
                raise RuntimeError("terminal dump backend %s" % back_b)
            diff = [(i, a, b) for i, (a, b) in enumerate(zip(grid_a, grid_b)) if a != b]
            if diff:
                raise RuntimeError("terminal grid differs from the console: %r" % diff[:4])
            g.say("terminal window grid == console grid (25 rows)")
            w, h, rgb = g.screendump_rgb(monitor, "qemu-gui-term.ppm")
            if w < 1000:
                raise RuntimeError("terminal not on the VBE screen: %dx%d" % (w, h))
            # PgUp scrolls the same console back inside the window.
            g.send_key(monitor, "pgup")
            time.sleep(1.5)
            _, _, rgb_up = g.screendump_rgb(monitor, "qemu-gui-term-pgup.ppm")
            if rgb_up == rgb:
                raise RuntimeError("PgUp did not scroll the terminal window")
            g.send_key(monitor, "pgdn")
            time.sleep(0.5)
            g.say("PgUp scrolls back in the terminal window")
            g.send_command_until(monitor, "console", "chrome=text", proc)
            time.sleep(1.0)
            w, h, _ = g.screendump_rgb(monitor, "qemu-gui-term-text.ppm")
            if w > 800:
                raise RuntimeError("console did not return to text mode: %dx%d" % (w, h))
            back_c, _ = dump_grid(monitor, proc)
            if back_c != "text":
                raise RuntimeError("after console the backend is %s" % back_c)
        g.say("QEMU GUI terminal = console contract passed.")
        return 0
    finally:
        if monitor is not None:
            monitor.close()
        g.terminate(proc)


if __name__ == "__main__":
    try:
        raise SystemExit(main())
    except Exception as error:
        print("QEMU GUI terminal failed: %s" % error, file=sys.stderr)
        raise SystemExit(1)
