#!/usr/bin/env python3
"""The desktop Shell pane is the console shell (one shell.c, no copy).

Types the same commands in the text console and then in the VBE desktop
Shell pane (/shell), and checks both produce the same output lines on the
serial line while the desktop stays on the Shell pane.
Commands: a console-only shell.c command (p2p-status) and an OS-UI
command (session-status) that used to bypass shell.c in the pane.
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

COMMANDS = [
    ("p2p-status", "p2p"),
    ("session-status", "session"),
]
NOISE = ("SYS_GET", "OSUI-SNAP", "osui gui line=", "[0m : ", "SYS_PUT", "[SCHED", "[IRQ")


def output_lines(text):
    lines = []
    for raw in g.normalized_log(text).splitlines():
        line = re.sub(r"\x1b\[[0-9;]*[A-Za-z]", "", raw).strip()
        if not line or any(n in raw for n in NOISE) or line.endswith(":"):
            continue
        lines.append(line)
    return lines


def wait_quiet(proc, settle=2.0, timeout=30.0):
    deadline = time.monotonic() + timeout
    last = len(g.log_text())
    quiet_since = time.monotonic()
    while time.monotonic() < deadline:
        time.sleep(0.2)
        n = len(g.log_text())
        if n != last:
            last = n
            quiet_since = time.monotonic()
        elif time.monotonic() - quiet_since > settle:
            return
    if proc.poll() is not None:
        raise RuntimeError("QEMU stopped")


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
                "qemu-system-i386", "-cpu", "pentium3", "-kernel", g.KERNEL,
                "-initrd", g.INITRD, "-m", "1024M", "-display", "none", "-vga", "std",
                "-usb", "-device", "usb-tablet",
                "-serial", "file:" + g.LOG, "-monitor", "unix:%s,server,nowait" % g.MON_SOCK,
                "-machine", "type=pc,accel=tcg", "-no-reboot", "-no-shutdown",
            ] + g.qemu_disk_args(), cwd=g.ROOT, stdout=err, stderr=err)
            g.wait_for(proc, "[0m : ", g.BOOT_TIMEOUT)
            monitor = g.monitor_connect()
            time.sleep(0.6)
            console = {}
            for cmd, marker in COMMANDS:
                start = g.send_command_until(monitor, cmd, marker, proc)
                wait_quiet(proc)
                text = g.log_text()[start:]
                text = text[text.find("ligne lue: " + cmd):]
                console[cmd] = output_lines(text)[1:]
                g.say("console %s: %d lines" % (cmd, len(console[cmd])))
                if not console[cmd]:
                    raise RuntimeError("console %s printed nothing" % cmd)
            g.send_command_until(monitor, "gui", "osui gui live", proc, wait_prompt=False)
            g.wait_for(proc, "OSUI-SNAP", g.CMD_TIMEOUT)
            g.send_command_until(monitor, "/shell", "pane=shell", proc, mode="getc", wait_prompt=False)
            for cmd, marker in COMMANDS:
                start = g.send_command_until(monitor, cmd, marker, proc, mode="getc", wait_prompt=False)
                wait_quiet(proc)
                text = g.log_text()[start:]
                text = text[text.find("osui gui line=" + cmd):]
                gui = output_lines(text)
                missing = [l for l in console[cmd] if l not in gui]
                if missing:
                    raise RuntimeError("GUI %s differs from console; missing %r\nGUI: %r"
                                       % (cmd, missing[:5], gui[:20]))
                snaps = re.findall(r"OSUI-SNAP [^\r\n]*", g.normalized_log(text))
                if not snaps or "pane=shell" not in snaps[-1]:
                    raise RuntimeError("GUI left the shell pane: %r" % snaps[-1:])
                g.say("GUI %s: same %d lines as the console" % (cmd, len(console[cmd])))
            g.send_key(monitor, "esc")
            g.wait_for(proc, "chrome=text prompt=MOHHDY>", g.CMD_TIMEOUT)
        g.say("QEMU GUI shell = console shell contract passed.")
        return 0
    finally:
        if monitor is not None:
            monitor.close()
        g.terminate(proc)


if __name__ == "__main__":
    try:
        raise SystemExit(main())
    except Exception as error:
        print("QEMU GUI shell failed: %s" % error, file=sys.stderr)
        raise SystemExit(1)
