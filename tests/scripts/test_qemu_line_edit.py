#!/usr/bin/env python3
"""One console line editor in text mode and in the VBE terminal window.

In both modes: Up recalls the previous command, Tab completes a command
from the shell registry, Left/Delete/Home/End edit inside the line and a
triple Tab (completion then double Tab) lists the candidates on screen.
The line actually read by SYS_GETS is checked on serial.
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
os.environ.setdefault("OSUI_GUI_LOG", os.path.join(LOGS, "qemu-line-edit-serial.log"))
os.environ.setdefault("OSUI_GUI_MON_SOCK", "/tmp/mohhdy-line-edit-mon.sock")
os.environ.setdefault("OSUI_GUI_QMP_SOCK", "/tmp/mohhdy-line-edit-qmp.sock")
os.environ.setdefault("OVERLAY_DISK", os.path.join(LOGS, "qemu-line-edit-overlay.img"))
import test_qemu_osui_gui as g  # noqa: E402


def keys(monitor, *names):
    for name in names:
        g.send_key(monitor, name)
        time.sleep(0.45)


def expect_line(proc, line, start):
    pat = re.compile(r"SYS_GETS: ligne lue: ([^\r\n]*)")
    deadline = time.monotonic() + g.CMD_TIMEOUT
    while time.monotonic() < deadline:
        got = pat.findall(g.normalized_log(g.log_text()[start:]))
        if got:
            if got[0].strip() != line:
                raise RuntimeError("line read %r, expected %r" % (got[0], line))
            g.wait_for(proc, "[0m : ", g.CMD_TIMEOUT, start)
            time.sleep(0.4)
            return
        time.sleep(0.15)
    raise RuntimeError("no line read (expected %r)" % line)


def check(monitor, proc, label):
    g.send_command_until(monitor, "session-status", "session", proc)
    time.sleep(0.4)
    start = len(g.log_text())
    keys(monitor, "up", "ret")
    expect_line(proc, "session-status", start)
    g.say("%s: Up recalls the previous command" % label)
    start = len(g.log_text())
    g.send_command_once(monitor, "p2p-stat", proc, "gets", enter=False)
    keys(monitor, "tab", "ret")
    expect_line(proc, "p2p-stats", start)
    g.say("%s: Tab completes p2p-stat -> p2p-stats" % label)
    start = len(g.log_text())
    g.send_command_once(monitor, "session-statusx", proc, "gets", enter=False)
    keys(monitor, "left", "delete", "home", "delete")
    g.send_command_once(monitor, "s", proc, "gets", enter=False)
    keys(monitor, "end", "ret")
    expect_line(proc, "session-status", start)
    g.say("%s: Left/Delete/Home/End edit the line" % label)
    start = len(g.log_text())
    g.send_command_once(monitor, "p2p-ra", proc, "gets", enter=False)
    keys(monitor, "tab", "tab", "tab")
    time.sleep(0.8)
    keys(monitor, *(["backspace"] * 8))
    keys(monitor, "ret")
    expect_line(proc, "", start)
    start = g.send_command_until(monitor, "term-dump", "TERM-DUMP END", proc)
    dump = g.normalized_log(g.log_text()[start:])
    rows = [l for l in dump.splitlines() if l.startswith("TERM|")]
    if not any("p2p-raft-compact" in r and "p2p-raft-put" in r for r in rows):
        raise RuntimeError("double Tab did not list the candidates: %r" % rows[-12:])
    g.say("%s: double Tab lists p2p-raft candidates" % label)


def main():
    os.makedirs(g.LOG_DIR, exist_ok=True)
    for path in (g.LOG, g.MON_SOCK, g.QMP_SOCK):
        try:
            os.remove(path)
        except OSError:
            pass
    g.prepare_test_disk()
    proc = monitor = None
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
            check(monitor, proc, "text")
            g.send_command_until(monitor, "gui", "osui gui live", proc, wait_prompt=False)
            g.wait_for(proc, "OSUI-SNAP", g.CMD_TIMEOUT)
            start = len(g.log_text())
            g.send_command_until(monitor, "/shell", "chrome=vbe_term", proc, mode="getc", wait_prompt=False)
            g.wait_for(proc, "gfx term on fb ", g.CMD_TIMEOUT, start)
            g.wait_for(proc, "[0m : ", g.CMD_TIMEOUT, start)
            time.sleep(0.6)
            check(monitor, proc, "terminal window")
        g.say("QEMU line editor contract passed (text + terminal window).")
        return 0
    finally:
        if monitor is not None:
            monitor.close()
        g.terminate(proc)


if __name__ == "__main__":
    try:
        raise SystemExit(main())
    except Exception as error:
        print("QEMU line editor failed: %s" % error, file=sys.stderr)
        raise SystemExit(1)
