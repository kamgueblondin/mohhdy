#!/usr/bin/env python3
"""UX5 GUI parity panels on the real OS (VBE desktop, keys through the
QEMU monitor).

Each terminal command family has a desktop panel (userspace/osui_parity.c)
whose entries are real shell command lines run through the same shell code
as the terminal (shell_app_run, registry rights model):

  * every panel opens from its slash command and Enter runs its first
    (read-only) entry: the window shows the real command output;
  * /search runs "grep -r line /samples" (recursive search, fsops_grep);
  * /disk mkdir (state changing) needs a second Enter: armed, then run;
  * /cmd runs a typed registry command (uptime) and refuses a terminal-only
    one (reboot) with the reason, nothing is executed.
"""
from __future__ import print_function

import os
import subprocess
import sys
import time

HERE = os.path.dirname(os.path.abspath(__file__))
sys.path.insert(0, HERE)
LOGS = os.path.join(HERE, "..", "..", "test_logs")
os.environ.setdefault("OSUI_GUI_LOG", os.path.join(LOGS, "qemu-gui-parity-serial.log"))
os.environ.setdefault("OSUI_GUI_MON_SOCK", "/tmp/mohhdy-gui-parity-mon.sock")
os.environ.setdefault("OSUI_GUI_QMP_SOCK", "/tmp/mohhdy-gui-parity-qmp.sock")
os.environ.setdefault("OVERLAY_DISK", os.path.join(LOGS, "qemu-gui-parity-overlay.img"))
import test_qemu_osui_gui as g  # noqa: E402

ALIASES = {" ": "spc", "-": "minus", ".": "dot", "/": "slash", "=": "equal", "_": "shift-minus"}

# slash, window title, first entry command, a fragment of its real output
PANELS = [
    ("/net", "Reseau", "net-status", "Reseau bare-metal"),
    ("/disk", "Stockage", "ata-status", "ata-status ok"),
    ("/services", "Services et pilotes", "devices", "devices ok"),
    ("/logs", "Journaux", "task-events", "task-events ok"),
    ("/ledger", "Registre collab", "collab-balances", "collab-balances"),
    ("/ci", "Production", "prod-diag", "prod-diag uptime_ticks"),
    ("/security", "Securite et CA", "whoami", ""),
    ("/devtools", "Outils web", "browser-status", "browser-status"),
    ("/notes", "Notes et archives", "ls /", "Initrd / VFS"),
    ("/devsync", "DevSync cluster", "devsync-show", "devsync"),
]


def help_window(monitor, proc):
    """Lot 45: /help in the chat opens the Aide window and shows the slash
    programs at once (gui-help, from the same table as the dispatch)."""
    start = slash(monitor, proc, "/help", "osui tool open app=Aide")
    run_marker(proc, start, "gui-help")
    g.wait_for(proc, "osui panel out=gui-help ok", g.CMD_TIMEOUT, start)
    say("/help: Aide window lists the desktop slash programs")
    start = len(g.log_text())
    key(monitor, "esc")
    key(monitor, "down")
    key(monitor, "ret")
    run_marker(proc, start, "help")
    say("/help: second entry runs the terminal help (shell registry)")
    close_panel(monitor, proc, "Aide")


def grpc(monitor, proc):
    slash(monitor, proc, "/grpc", "osui tool open app=Wire gRPC")
    start = len(g.log_text())
    type_text(monitor, "Echo Say hi")
    key(monitor, "ret")
    run_marker(proc, start, "grpc-wire Echo Say hi")
    say("/grpc: grpc-wire runs through the shell")
    close_panel(monitor, proc, "Wire gRPC")


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
    time.sleep(0.6)
    return start


def run_marker(proc, start, cmd, result="ok"):
    g.wait_for(proc, "osui panel run cmd=%s %s" % (cmd, result), g.CMD_TIMEOUT, start)


def close_panel(monitor, proc, title):
    start = len(g.log_text())
    key(monitor, "esc")      # output -> menu
    key(monitor, "esc")      # menu -> close
    g.wait_for(proc, "osui tool close app=" + title, g.CMD_TIMEOUT, start)


def families(monitor, proc):
    for sl, title, cmd, frag in PANELS:
        start = slash(monitor, proc, sl, "osui tool open app=" + title)
        start = len(g.log_text())
        key(monitor, "ret")
        run_marker(proc, start, cmd)
        if frag:
            g.wait_for(proc, "osui panel out=", g.CMD_TIMEOUT, start)
            out = g.normalized_log(g.log_text()[start:])
            if frag not in out:
                raise RuntimeError("%s: real output of %s missing (%r)" % (sl, cmd, frag))
        say("%s panel runs %s through the shell (real output)" % (sl, cmd))
        close_panel(monitor, proc, title)


def search(monitor, proc):
    slash(monitor, proc, "/search", "osui tool open app=Recherche")
    start = len(g.log_text())
    type_text(monitor, "line /samples")
    key(monitor, "ret")
    run_marker(proc, start, "grep -r line /samples")
    g.wait_for(proc, "osui panel out=/samples/long.txt:2: line 002", g.CMD_TIMEOUT, start)
    say("/search: grep -r over the VFS finds /samples/long.txt lines")
    close_panel(monitor, proc, "Recherche")


def confirm_mutation(monitor, proc):
    slash(monitor, proc, "/disk", "osui tool open app=Stockage")
    for _ in range(5):
        key(monitor, "down")
    start = len(g.log_text())
    type_text(monitor, "/gp")
    key(monitor, "ret")
    g.wait_for(proc, "osui panel armed cmd=mkdir /gp", g.CMD_TIMEOUT, start)
    if "osui panel run cmd=mkdir" in g.log_text()[start:]:
        raise RuntimeError("mkdir ran without confirmation")
    say("/disk mkdir armed on the first Enter (state change needs confirmation)")
    start = len(g.log_text())
    key(monitor, "ret")
    run_marker(proc, start, "mkdir /gp")
    g.wait_for(proc, "osui panel out=mkdir ok", g.CMD_TIMEOUT, start)
    say("second Enter: mkdir ran in the shell task (kernel rights)")
    close_panel(monitor, proc, "Stockage")


def generic(monitor, proc):
    slash(monitor, proc, "/cmd", "osui tool open app=Commande")
    start = len(g.log_text())
    type_text(monitor, "uptime")
    key(monitor, "ret")
    run_marker(proc, start, "uptime")
    start = len(g.log_text())
    type_text(monitor, "reboot")
    key(monitor, "ret")
    run_marker(proc, start, "reboot", "refused")
    g.wait_for(proc, "reservee au terminal", g.CMD_TIMEOUT, start)
    say("/cmd: registry command runs, terminal-only reboot refused with its reason")
    start = len(g.log_text())
    key(monitor, "esc")
    g.wait_for(proc, "osui tool close app=Commande", g.CMD_TIMEOUT, start)


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
                "qemu-system-i386", "-cpu", "pentium3", "-kernel", g.KERNEL,
                "-initrd", g.INITRD, "-m", "1024M", "-display", "none", "-vga", "std",
                "-serial", "file:" + g.LOG, "-monitor", "unix:%s,server,nowait" % g.MON_SOCK,
                "-machine", "type=pc,accel=tcg", "-no-reboot", "-no-shutdown",
            ] + g.qemu_disk_args(), cwd=g.ROOT, stdout=err, stderr=err)
            g.wait_for(proc, "[0m : ", g.BOOT_TIMEOUT)
            monitor = g.monitor_connect()
            time.sleep(0.6)
            g.send_command_until(monitor, "gui", "osui gui live", proc, wait_prompt=False)
            g.wait_for(proc, "OSUI-SNAP", g.CMD_TIMEOUT)
            help_window(monitor, proc)
            families(monitor, proc)
            grpc(monitor, proc)
            search(monitor, proc)
            confirm_mutation(monitor, proc)
            generic(monitor, proc)
            g.screendump_rgb(monitor, "qemu-gui-parity-end.ppm")
        say("QEMU UX5 GUI parity panels contract passed.")
        return 0
    finally:
        if monitor is not None:
            monitor.close()
        g.terminate(proc)


if __name__ == "__main__":
    sys.exit(main())
