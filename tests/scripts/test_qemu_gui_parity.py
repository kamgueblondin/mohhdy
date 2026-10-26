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
    one (reboot) with the reason, nothing is executed;
  * Lot 46: the output of a spawned program is shown in the window, bounded
    to 16 KiB with a note giving the dropped bytes; driver-restart of
    atadriver and networker gives new pids and working services.
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
    # Lot 44 (US-G17) panels
    # /vfs: the vfs server is not started at boot, the shell's own refusal
    # is the real output (start it with "spawn bin/vfsserver" in /cmd)
    ("/vfs", "Virtuel VFS", "vfs-stats", "service vfs indisponible"),
    ("/sessions", "Sessions utilisateur", "session-status", "osui session-status session_id="),
    ("/mcp", "Paiements MCP", "mcp-status", "osui mcp-status active="),
    ("/media", "Media et streaming", "browser-media", "osui browser-media status="),
    ("/pwa", "Applications PWA", "pwa-list", "osui pwa-list count="),
    ("/sysadmin", "Administration", "os-admin", "osui admin-status"),
    ("/api", "Tokens API", "api", "osui api status="),
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


def viewer_images(monitor, proc):
    """Lot 45 (+ Lot 47 JPEG, PNG 16-bit/Adam7): imgdec in the viewer (PNG, BMP) with zoom, and img-info in
    the shell through /cmd (same decoder)."""
    slash(monitor, proc, "/view /samples/mire.png", "fmt=png zoom=fit")
    g.wait_for(proc, "osui tool line=image 48x32 affichee en pixels 48x32 fmt=png", g.CMD_TIMEOUT)
    start = len(g.log_text())
    key(monitor, "shift-equal")
    g.wait_for(proc, "fmt=png zoom=200%", g.CMD_TIMEOUT, start)
    say("/view mire.png: PNG decoded in pixels, + zooms to 200%")
    g.screendump_rgb(monitor, "qemu-gui-lot45-viewer.ppm")
    start = len(g.log_text())
    key(monitor, "esc")
    g.wait_for(proc, "osui tool close app=Visionneuse", g.CMD_TIMEOUT, start)
    slash(monitor, proc, "/view /samples/mire24.bmp", "fmt=bmp zoom=fit")
    start = len(g.log_text())
    key(monitor, "esc")
    g.wait_for(proc, "osui tool close app=Visionneuse", g.CMD_TIMEOUT, start)
    say("/view mire24.bmp: BMP 24 bits decoded")
    slash(monitor, proc, "/cmd", "osui tool open app=Commande")
    start = len(g.log_text())
    type_text(monitor, "img-info /samples/mire32.bmp")
    key(monitor, "ret")
    run_marker(proc, start, "img-info /samples/mire32.bmp")
    g.wait_for(proc, "osui panel out=img-info ok fmt=bmp w=48 h=32 bpp=32", g.CMD_TIMEOUT, start)
    say("img-info (shell) decodes the same BMP 32 bits")
    # Lot 47 (US-G03): JPEG baseline and PNG 16-bit / Adam7 in the same decoder
    for path, want in (("/images/mire420.jpg", "img-info ok fmt=jpeg w=48 h=32 bpp=24"),
                       ("/images/mire-rgba16-i.png", "img-info ok fmt=png w=48 h=32 bpp=64 depth=16 interlace=1 px0=0,0,0"),
                       ("/images/mire-g2-i.png", "img-info ok fmt=png w=48 h=32 bpp=2 depth=2 interlace=1"),
                       ("/images/mire-prog.jpg", "img-info error fmt=jpeg w=48 h=32 bpp=24 reason=variante non supportee")):
        start = len(g.log_text())
        type_text(monitor, "img-info " + path)
        key(monitor, "ret")
        run_marker(proc, start, "img-info " + path)
        g.wait_for(proc, "osui panel out=" + want, g.CMD_TIMEOUT, start)
        say("img-info " + path + ": " + want)
    start = len(g.log_text())
    key(monitor, "esc")
    g.wait_for(proc, "osui tool close app=Commande", g.CMD_TIMEOUT, start)
    slash(monitor, proc, "/view /images/mire444.jpg", "fmt=jpeg zoom=fit")
    g.wait_for(proc, "osui tool line=image 48x32 affichee en pixels 48x32 fmt=jpeg", g.CMD_TIMEOUT)
    g.screendump_rgb(monitor, "qemu-gui-lot47-viewer-jpeg.ppm")
    start = len(g.log_text())
    key(monitor, "esc")
    g.wait_for(proc, "osui tool close app=Visionneuse", g.CMD_TIMEOUT, start)
    say("/view mire444.jpg: JPEG baseline drawn in pixels")
    slash(monitor, proc, "/view /images/mire-rgb-i.png", "fmt=png zoom=fit")
    start = len(g.log_text())
    key(monitor, "esc")
    g.wait_for(proc, "osui tool close app=Visionneuse", g.CMD_TIMEOUT, start)
    say("/view mire-rgb-i.png: interlaced PNG drawn")


def ledger_form(monitor, proc):
    """Lot 45: the collab-offer form of /ledger builds the shell grammar,
    arms on the first Enter of submit and runs on the second."""
    slash(monitor, proc, "/ledger", "osui tool open app=Registre collab")
    for _ in range(4):
        key(monitor, "down")
    key(monitor, "ret")              # opens the form
    for _ in range(3):
        key(monitor, "ret")          # Nom -> Prix -> Capacite -> submit
    start = len(g.log_text())
    key(monitor, "ret")
    g.wait_for(proc, "osui form armed Entree encore pour confirmer: collab-offer gpu 10 1", g.CMD_TIMEOUT, start)
    if "osui form run" in g.log_text()[start:]:
        raise RuntimeError("ledger form ran without confirmation")
    start = len(g.log_text())
    key(monitor, "ret")
    g.wait_for(proc, "osui form run cmd=collab-offer gpu 10 1", g.CMD_TIMEOUT, start)
    g.wait_for(proc, "osui form status=", g.CMD_TIMEOUT, start)
    say("/ledger form: collab-offer armed, then run through the shell")
    start = len(g.log_text())
    key(monitor, "esc")              # form -> menu
    key(monitor, "esc")              # menu -> close
    g.wait_for(proc, "osui tool close app=Registre collab", g.CMD_TIMEOUT, start)


def devsync(monitor, proc):
    slash(monitor, proc, "/devsync", "osui tool open app=DevSync cluster")
    start = len(g.log_text())
    type_text(monitor, "A")
    key(monitor, "ret")
    run_marker(proc, start, "devsync-show A")
    g.wait_for(proc, "osui panel out=", g.CMD_TIMEOUT, start)
    say("/devsync: devsync-show A runs through the shell")
    close_panel(monitor, proc, "DevSync cluster")


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
    # Lot 44: restart of a Ring 3 driver from a window
    start = len(g.log_text())
    type_text(monitor, "driver-restart pcidriver")
    key(monitor, "ret")
    run_marker(proc, start, "driver-restart pcidriver")
    g.wait_for(proc, "osui panel out=driver-restart ok pci-driver old ", g.CMD_TIMEOUT, start)
    import re
    old = re.search(r"driver-restart ok pci-driver old (\d+)", g.normalized_log(g.log_text()[start:])).group(1)
    # the kernel supervisor respawns pcidriver at the next use of the PCI
    # service: lspci from the same window, then the new pid is asserted
    start = len(g.log_text())
    type_text(monitor, "lspci")
    key(monitor, "ret")
    run_marker(proc, start, "lspci")
    g.wait_for(proc, "[PCI] supervised respawn bin/pcidriver pid ", g.CMD_TIMEOUT, start)
    m = re.search(r"\[PCI\] supervised respawn bin/pcidriver pid (\d+)", g.log_text()[start:])
    if not m or m.group(1) == old:
        raise RuntimeError("driver-restart: no supervised respawn with a new pid")
    say("/cmd: driver-restart pcidriver: old pid %s, respawned as pid %s by the kernel supervisor" % (old, m.group(1)))
    start = len(g.log_text())
    key(monitor, "esc")
    g.wait_for(proc, "osui tool close app=Commande", g.CMD_TIMEOUT, start)


def spawn_capture(monitor, proc):
    """Lot 46: the output of a program launched from a window (spawn) is
    captured by the kernel (SYS_OUT_CAPTURE) and shown in the window, after
    the shell's own line; past 16 KiB the window says how many bytes were
    dropped and the end of the output never reaches it."""
    import re
    slash(monitor, proc, "/cmd", "osui tool open app=Commande")
    start = len(g.log_text())
    type_text(monitor, "spawn ok")
    key(monitor, "ret")
    run_marker(proc, start, "spawn ok")
    g.wait_for(proc, "osui panel out=spawn ok pid ", g.CMD_TIMEOUT, start)
    g.wait_for(proc, "osui panel tail=exec ok", g.CMD_TIMEOUT, start)
    g.wait_for(proc, "[OUTCAP] end bytes=8 dropped=0", g.CMD_TIMEOUT, start)
    say("/cmd spawn ok: the child's 'exec ok' is shown in the window")
    start = len(g.log_text())
    type_text(monitor, "spawn outflood")
    key(monitor, "ret")
    run_marker(proc, start, "spawn outflood")
    g.wait_for(proc, "osui panel tail=[sortie du programme tronquee: ", g.CMD_TIMEOUT, start)
    out = g.normalized_log(g.log_text()[start:])
    m = re.search(r"\[OUTCAP\] end bytes=(\d+) dropped=(\d+)", out)
    if not m or int(m.group(2)) <= 0 or int(m.group(1)) - int(m.group(2)) != 16384:
        raise RuntimeError("outflood: capture not bounded to 16 KiB: %r" % (m.groups() if m else None))
    note = re.search(r"tronquee: (\d+) octets perdus sur (\d+) \(16 Kio max\)", out)
    if not note or note.group(1) != m.group(2) or note.group(2) != m.group(1):
        raise RuntimeError("outflood: window note does not match the kernel count")
    if "outflood end" in out:
        raise RuntimeError("outflood: the dropped end of the output reached the window")
    say("/cmd spawn outflood: %s of %s bytes kept, the window notes %s dropped" %
        (int(m.group(1)) - int(m.group(2)), m.group(1), m.group(2)))
    start = len(g.log_text())
    key(monitor, "esc")
    g.wait_for(proc, "osui tool close app=Commande", g.CMD_TIMEOUT, start)


def cmd_run(monitor, proc, line):
    start = len(g.log_text())
    type_text(monitor, line)
    key(monitor, "ret")
    run_marker(proc, start, line)
    return start


def driver_respawn(monitor, proc):
    """Lot 46: driver-restart of the storage and network drivers from a
    window. atadriver: the kernel supervisor respawns it at once (new pid),
    a file written then read back goes through the new driver, ata-status
    names it. networker: the shell starts it again (new pid), the netrelay
    proof client (spawned from the same window, output captured) does its
    TCP loopback through the relay of the new worker."""
    import re
    slash(monitor, proc, "/cmd", "osui tool open app=Commande")
    start = cmd_run(monitor, proc, "driver-restart atadriver")
    g.wait_for(proc, "osui panel out=driver-restart ok ata-driver old ", g.CMD_TIMEOUT, start)
    out = g.normalized_log(g.log_text()[start:])
    m = re.search(r"driver-restart ok ata-driver old (\d+) new (\d+)", out)
    r = re.search(r"\[ATA\] supervised respawn bin/atadriver pid (\d+)", out)
    if not m or not r or m.group(1) == m.group(2) or r.group(1) != m.group(2):
        raise RuntimeError("atadriver: no supervised respawn with a new pid: %r" % (m.groups() if m else None))
    new_ata = m.group(2)
    start = cmd_run(monitor, proc, "write l46ata respawnok")
    g.wait_for(proc, "osui panel out=write ok l46ata", g.CMD_TIMEOUT, start)
    start = cmd_run(monitor, proc, "cat l46ata")
    g.wait_for(proc, "osui panel out=respawnok", g.CMD_TIMEOUT, start)
    start = cmd_run(monitor, proc, "ata-status")
    g.wait_for(proc, "osui panel out=ata-status ok driver %s " % new_ata, g.CMD_TIMEOUT, start)
    say("/cmd: driver-restart atadriver: old pid %s, respawned as pid %s; write/cat and ata-status served by it" %
        (m.group(1), new_ata))
    start = cmd_run(monitor, proc, "driver-restart networker")
    g.wait_for(proc, "osui panel out=driver-restart ok net-driver old ", g.CMD_TIMEOUT, start)
    m = re.search(r"driver-restart ok net-driver old (\d+) new (\d+)", g.normalized_log(g.log_text()[start:]))
    if not m or m.group(1) == m.group(2):
        raise RuntimeError("networker: not restarted with a new pid: %r" % (m.groups() if m else None))
    new_net = m.group(2)
    restart_start = start
    start = cmd_run(monitor, proc, "spawn netrelay")
    # Lot 46 review: the relaunched networker registers net-driver, so it
    # leaves the capture: the window does not call it a program still running.
    if "encore actif" in g.normalized_log(g.log_text()[restart_start:start]):
        raise RuntimeError("networker: the relaunched driver is reported as a program still running")
    g.wait_for(proc, "netrelay tcp loopback ok ping pong mode relay", g.CMD_TIMEOUT, start)
    start = cmd_run(monitor, proc, "net-relay-status")
    g.wait_for(proc, "osui panel out=net-relay ok worker %s " % new_net, g.CMD_TIMEOUT, start)
    say("/cmd: driver-restart networker: old pid %s, new pid %s; netrelay TCP loopback through its relay" %
        (m.group(1), new_net))
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
            devsync(monitor, proc)
            ledger_form(monitor, proc)
            viewer_images(monitor, proc)
            search(monitor, proc)
            confirm_mutation(monitor, proc)
            generic(monitor, proc)
            spawn_capture(monitor, proc)
            driver_respawn(monitor, proc)
            g.screendump_rgb(monitor, "qemu-gui-parity-end.ppm")
        say("QEMU UX5 GUI parity panels contract passed.")
        return 0
    finally:
        if monitor is not None:
            monitor.close()
        g.terminate(proc)


if __name__ == "__main__":
    sys.exit(main())
