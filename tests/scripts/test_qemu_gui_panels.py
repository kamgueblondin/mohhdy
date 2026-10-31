#!/usr/bin/env python3
"""Lot 46 (PR A): every entry of the desktop panels that were "partial" in
docs/GUI_PARITY.md runs in QEMU through the menu of its window.

For /net /disk /services /logs /ci /security /devtools /notes the test
selects each entry with the arrow keys, types its argument when it has one,
confirms the state changes (first Enter arms, second Enter runs), and
asserts the run marker of the shell (`osui panel run cmd=... ok`) plus a
fragment of the real output of the command. The commands are the shell's
own (same task, same kernel rights): nothing is reimplemented in the GUI.
The order inside a panel follows what the commands need (p2p-up before
p2p-health, prod-sample before prod-metrics, sign before verify).
"""
import os
import re
import subprocess
import sys
import time

HERE = os.path.dirname(os.path.abspath(__file__))
sys.path.insert(0, HERE)
LOGS = os.path.join(HERE, "..", "..", "test_logs")
os.environ.setdefault("OSUI_GUI_LOG", os.path.join(LOGS, "qemu-gui-panels-serial.log"))
os.environ.setdefault("OSUI_GUI_MON_SOCK", "/tmp/mohhdy-gui-panels-mon.sock")
os.environ.setdefault("OSUI_GUI_QMP_SOCK", "/tmp/mohhdy-gui-panels-qmp.sock")
os.environ.setdefault("OVERLAY_DISK", os.path.join(LOGS, "qemu-gui-panels-overlay.img"))
import test_qemu_osui_gui as g  # noqa: E402
import test_qemu_gui_parity as par  # noqa: E402

# (entry index in userspace/osui_parity.c, argument or None, state change,
#  fragment of the real output; "" = any output line)
PANELS = [
    ("/net", "Reseau", [
        (0, None, False, "Reseau bare-metal"),
        (1, None, False, "net-wire ok worker"),
        (2, None, False, "net-relay ok worker"),
        (5, "alice 10.0.0.2", True, "p2p-up step 1/4"),
        (3, None, False, "p2p health ok"),
        (4, None, False, "p2p peers ok"),
        # Lot 47 (US-G04): the shell net-if command from the window (root
        # shell task, so the kernel accepts the change); no NIC here, the
        # static address is still recorded by the loopback networker.
        (6, None, False, "net-if "),
        (7, "static 10.32.0.77 255.255.255.0 10.32.0.2", True, "net-if ok mode static ip 10.32.0.77"),
    ]),
    ("/disk", "Stockage", [
        (0, None, False, "ata-status ok driver"),
        (1, None, False, "mount-journal ok initrd/"),
        (6, None, True, "persist-save node"),
        (2, None, False, "fnv ok"),
        (3, "/samples", False, "Initrd / VFS"),
        (4, "/samples/long.txt", False, "stat file /samples/long.txt"),
        (5, "/l46p", True, "mkdir ok"),
        (7, None, False, "vfs-list ok count"),
    ]),
    ("/services", "Services et pilotes", [
        (0, None, False, "devices ok"),
        (1, None, False, "pci 0:0.0"),
        (2, None, False, "dev cpu0"),
        (3, None, False, "Jobs"),
        (4, None, False, "task-summary ok"),
        (7, "ata-driver", False, "service-find ok ata-driver"),
        (6, "ata-driver", False, ""),
        (5, "pcidriver", True, "driver-restart ok pci-driver old"),
    ]),
    ("/logs", "Journaux", [
        (0, None, False, "task-events ok"),
        (1, None, False, "notify ok pending"),
        (2, None, False, "osui osui-audit count="),
        (3, None, False, "Historique"),
        (4, None, False, "prod-usage"),
        (6, "/l46.log hello", True, "prod-log-append ok bytes 6"),
        (5, "/l46.log", False, "prod-log lines 1"),
        (7, "1 hello", True, "notify-push ok id "),
        (8, "@id", True, "notify-ack ok"),
    ]),
    ("/ci", "Production", [
        (0, None, False, "prod-diag uptime_ticks"),
        (1, None, False, "prod-alerts ok"),
        (3, None, False, "prod-scale-status ok"),
        (5, None, True, "prod-sample ok n 1"),
        (4, None, False, "prod-metric mem_used"),
        (8, "/samples /samples /l46.man", True, "deploy-make ok files 8"),
        (6, "/l46.man", False, "deploy-verify ok files 8"),
        (7, "baseline /samples", False, "prod-integrity baseline files 8"),
        (7, "check /samples", False, "verdict clean"),
        (9, "l46b /samples", True, "prod-backup ok l46b files 8"),
        (2, None, False, "prod-backup l46b dir /samples files 8"),
    ]),
    ("/security", "Securite et CA", [
        (0, None, False, "whoami ok"),
        (1, None, False, "id-key ok"),
        (2, None, False, "origin-check ok"),
        (4, None, True, "pm-ca-init sealed key"),
        (5, "/pm/hello.pm", True, "pm-ca-sign ok /pm/hello.pm.ca"),
        (3, "/pm/hello.pm", False, "pm-ca-verify ok trust_chain verified"),
        (6, "/pm/hello.pm", True, "pm-ca-revoke ok /pm/hello.pm"),
        (3, "/pm/hello.pm", False, "pm-ca-verify error revoked"),
    ]),
    ("/devtools", "Outils web", [
        (0, None, False, "browser-status"),
        (1, None, False, "browser-tab-list count="),
        (2, None, False, "web-perf ok"),
        (3, None, False, "web-cache ok"),
        (4, "/web/demo.html", False, "dom 1 html"),
        (5, "/web/demo.html", False, "web-console ok"),
        (6, "/web/demo.html hero", False, ""),
        (7, "/web/demo.html", False, "web-a11y ok"),
    ]),
    ("/notes", "Notes et archives", [
        (0, None, False, "Initrd / VFS"),
        (2, "/l46n.txt hello", True, "append ok /l46n.txt"),
        (1, "/l46n.txt", False, "osui panel out=hello"),
        (3, "/samples /l46.arc", True, "migrate-export ok files 8"),
        (4, "/l46.arc /samples", False, "migrate-verify ok files 8"),
        (5, "/l46.arc /l46imp", True, "migrate-import ok files 8"),
    ]),
]

FAMILY_CMD = {}


def entry_cmds():
    """Commands of each entry, read from the C table (one source)."""
    src = open(os.path.join(HERE, "..", "..", "userspace", "osui_parity.c")).read()
    out = {}
    for name, body in re.findall(r"static const osui_par_entry_t (k_\w+)\[\] = \{(.*?)\n\};", src, re.S):
        out[name] = re.findall(r'\{ "[^"]*", "([^"]+)",', body)
    slash_of = dict(re.findall(r'\{ OS_FB_APP_\w+, "(/\w+)", "[^"]*", "[^"]*", (k_\w+),', src))
    return {sl: out[k] for sl, k in slash_of.items()}


def run_entry(monitor, proc, cur, idx, arg, mut, cmd):
    """Move from entry cur to idx, type the argument, Enter (twice for a
    state change); returns the log offset before the run."""
    k = "down" if idx >= cur else "up"
    for _ in range(abs(idx - cur)):
        par.key(monitor, k)
    if arg:
        par.type_text(monitor, arg)
    built = cmd + (" " + arg if arg else "")
    start = len(g.log_text())
    par.key(monitor, "ret")
    if mut:
        g.wait_for(proc, "osui panel armed cmd=" + built, g.CMD_TIMEOUT, start)
        if ("osui panel run cmd=" + built) in g.log_text()[start:]:
            raise RuntimeError("%s ran without confirmation" % built)
        par.key(monitor, "ret")
    par.run_marker(proc, start, built)
    return start, built


def panels(monitor, proc):
    cmds = entry_cmds()
    total = 0
    for sl, title, steps in PANELS:
        par.slash(monitor, proc, sl, "osui tool open app=" + title)
        cur = 0
        last_id = None
        for idx, arg, mut, frag in steps:
            if arg == "@id":
                arg = last_id
            start, built = run_entry(monitor, proc, cur, idx, arg, mut, cmds[sl][idx])
            g.wait_for(proc, "osui panel out=", g.CMD_TIMEOUT, start)
            if frag:
                deadline = time.time() + g.CMD_TIMEOUT
                while frag not in g.normalized_log(g.log_text()[start:]):
                    if time.time() > deadline:
                        raise RuntimeError("%s: %s: real output %r missing" % (sl, built, frag))
                    time.sleep(0.3)
            m = re.search(r"notify-push ok id (\d+)", g.normalized_log(g.log_text()[start:]))
            if m:
                last_id = m.group(1)
            total += 1
            par.key(monitor, "esc")          # output -> menu (same entry selected)
            time.sleep(0.3)
            cur = idx
        par.say("%s: %d entries run through the shell (%s)" % (sl, len(steps),
                ", ".join(sorted(set(cmds[sl][s[0]] for s in steps)))))
        start = len(g.log_text())
        par.key(monitor, "esc")              # menu -> close
        g.wait_for(proc, "osui tool close app=" + title, g.CMD_TIMEOUT, start)
        missing = [c for i, c in enumerate(cmds[sl]) if i not in set(s[0] for s in steps)]
        if missing:
            raise RuntimeError("%s: entries not exercised: %r" % (sl, missing))
    return total


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
                "-serial", "file:" + g.LOG, "-monitor", "unix:%s,server,nowait" % g.MON_SOCK,
                "-machine", "type=pc,accel=tcg", "-no-reboot", "-no-shutdown",
            ] + g.qemu_disk_args(), cwd=g.ROOT, stdout=err, stderr=err)
            g.wait_for(proc, "[0m : ", g.BOOT_TIMEOUT)
            monitor = g.monitor_connect()
            time.sleep(0.6)
            g.send_command_until(monitor, "gui", "osui gui live", proc, wait_prompt=False)
            g.wait_for(proc, "OSUI-SNAP", g.CMD_TIMEOUT)
            n = panels(monitor, proc)
            g.screendump_rgb(monitor, "qemu-gui-panels-end.ppm")
        par.say("QEMU GUI panels contract passed: %d entries of 8 panels run in QEMU." % n)
        return 0
    finally:
        if monitor is not None:
            monitor.close()
        g.terminate(proc)


if __name__ == "__main__":
    sys.exit(main())
