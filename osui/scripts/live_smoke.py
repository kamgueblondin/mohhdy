#!/usr/bin/env python3
"""Fumee optionnelle d'attache live (hors make ci / hors integration-qemu).

Defaut : prouve le chemin serial contre un faux guest MOHHDY>, puis
sonde qemu-system-i386. Ne boot PAS QEMU (trop long pour osui-smoke).

MOHHDY_LIVE_QEMU=1 : tente un boot court seulement si kernel+initrd
existent. Echec honnete, pas de falsification.

Exit 0 si le chemin d'attache est prouve, meme si QEMU est absent (SKIP).
"""

from __future__ import annotations

import os
import socket
import sys
import tempfile
import threading
import time
from pathlib import Path

ROOT = Path(__file__).resolve().parents[2]
sys.path.insert(0, str(ROOT / "osui"))

from guest_attach import AttachConfig, GuestLink, qemu_available  # noqa: E402
from multiboot_shell import MultibootShell  # noqa: E402

PROMPT = "MOHHDY>"


def _fake_guest(path: str, stop: threading.Event) -> None:
    if os.path.exists(path):
        os.unlink(path)
    server = socket.socket(socket.AF_UNIX, socket.SOCK_STREAM)
    server.bind(path)
    server.listen(1)
    server.settimeout(0.3)
    try:
        while not stop.is_set():
            try:
                conn, _ = server.accept()
            except socket.timeout:
                continue
            with conn:
                conn.sendall(PROMPT.encode("ascii"))
                buf = b""
                conn.settimeout(0.4)
                while not stop.is_set():
                    try:
                        chunk = conn.recv(1024)
                    except socket.timeout:
                        continue
                    except OSError:
                        break
                    if not chunk:
                        break
                    buf += chunk
                    while b"\n" in buf:
                        line, buf = buf.split(b"\n", 1)
                        cmd = line.decode("utf-8", errors="replace").strip()
                        if cmd == "help":
                            body = "vfs-list\nai <question>\nhelp ok\n"
                        elif cmd == "apt":
                            body = "refuse\n"
                        else:
                            body = "ok %s\n" % (cmd or "empty")
                        conn.sendall((body + PROMPT).encode("utf-8"))
    finally:
        server.close()
        if os.path.exists(path):
            os.unlink(path)


def prove_fake_serial() -> None:
    tmp = tempfile.mkdtemp(prefix="mohhdy-live-")
    sock = os.path.join(tmp, "guest.sock")
    stop = threading.Event()
    thread = threading.Thread(target=_fake_guest, args=(sock, stop), daemon=True)
    thread.start()
    deadline = time.monotonic() + 2
    while time.monotonic() < deadline and not os.path.exists(sock):
        time.sleep(0.02)
    os.environ["MOHHDY_SHELL_ATTACH"] = "live"
    os.environ["MOHHDY_GUEST_SERIAL"] = "unix:" + sock
    os.environ["MOHHDY_GUEST_TRANSPORT"] = "serial"
    os.environ["MOHHDY_GUEST_TIMEOUT"] = "1.5"
    cfg = AttachConfig.from_env()
    shell = MultibootShell(cfg)
    out = shell.execute("attach")
    if not out.get("live_guest"):
        raise SystemExit("FAIL: attach fake serial live_guest=false (%s)" % out.get("output"))
    help_out = shell.execute("help")
    if "vfs-list" not in help_out.get("output", ""):
        raise SystemExit("FAIL: live help n'a pas renvoye vfs-list")
    trap = shell.execute("apt install nginx")
    if trap.get("rc") != 1 or "Pas un bash Linux" not in trap.get("output", ""):
        raise SystemExit("FAIL: apt doit rester refuse en live")
    shell.execute("detach")
    stop.set()
    print("OK live-attach fake serial (live_guest=true puis detach)")


def probe_qemu() -> None:
    kernel = os.environ.get("KERNEL", str(ROOT / "build" / "mohhdy.bin"))
    initrd = os.environ.get("INITRD", str(ROOT / "my_initrd.tar"))
    has_qemu = qemu_available()
    has_kernel = os.path.isfile(kernel) and os.path.isfile(initrd)
    if not has_qemu:
        print("SKIP qemu-system-i386 absent (pas de boot guest)")
        return
    print("INFO qemu-system-i386 present")
    if not has_kernel:
        print("SKIP kernel/initrd absents (pas de boot ; make all d'abord)")
        return
    if os.environ.get("MOHHDY_LIVE_QEMU") != "1":
        print(
            "SKIP boot QEMU (defaut). Pour tenter : "
            "MOHHDY_LIVE_QEMU=1 make osui-shell-live-smoke"
        )
        print("Exemple attache operateur :")
        print("  qemu-system-i386 -kernel build/mohhdy.bin -initrd my_initrd.tar \\")
        print("    -display none -serial unix:/tmp/mohhdy-serial.sock,server,nowait \\")
        print("    -monitor unix:/tmp/mohhdy-mon.sock,server,nowait -m 256M")
        print("  MOHHDY_SHELL_ATTACH=live MOHHDY_GUEST_SERIAL=unix:/tmp/mohhdy-serial.sock \\")
        print("    MOHHDY_GUEST_MONITOR=unix:/tmp/mohhdy-mon.sock python3 osui/server.py")
        print("Le guest lit PS/2 ; HMP sendkey est le harness connu. Serial = log/TTY.")
        print("Guest VGA n'heberge pas #ai-stage.")
        return
    print("INFO MOHHDY_LIVE_QEMU=1 : boot QEMU non implemente ici pour rester court.")
    print("Utiliser le harness make qemu-smoke / serial+monitor ci-dessus.")


def main() -> int:
    print("osui-shell-live-smoke (hors make ci, hors integration-qemu)")
    prove_fake_serial()
    probe_qemu()
    print("OK osui-shell-live-smoke")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
