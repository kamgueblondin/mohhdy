#!/usr/bin/env python3
"""Contrat MOHHDY Foundation : transport IPC entre deux tâches Ring 3."""
import os
import re
import socket
import subprocess
import sys
import time

ROOT = os.path.abspath(os.path.join(os.path.dirname(__file__), "..", ".."))
LOG_DIR = os.path.join(ROOT, "test_logs")
LOG = os.path.join(LOG_DIR, "ipc-foundation.log")
ERR = os.path.join(LOG_DIR, "ipc-foundation.err")
MON = os.path.join(LOG_DIR, "ipc-foundation-monitor.sock")
KERNEL = os.path.join(ROOT, "build", "mohhdy.bin")
INITRD = os.path.join(ROOT, "my_initrd.tar")
KEY_HOLD_MS = int(os.environ.get("KEY_HOLD_MS", "10"))


def log_text():
    try:
        with open(LOG, "r", errors="replace") as handle:
            return handle.read()
    except OSError:
        return ""


def wait_for(needle, proc, offset=0, timeout=15):
    deadline = time.time() + timeout
    while time.time() < deadline:
        if proc.poll() is not None:
            raise RuntimeError("QEMU s'est arrêté prématurément")
        if needle in log_text()[offset:]:
            return
        time.sleep(0.1)
    raise RuntimeError("sortie manquante : %s" % needle)


def connect_monitor():
    deadline = time.time() + 5
    while time.time() < deadline:
        if os.path.exists(MON):
            client = socket.socket(socket.AF_UNIX, socket.SOCK_STREAM)
            try:
                client.connect(MON)
                client.settimeout(0.1)
                try:
                    client.recv(4096)
                except socket.timeout:
                    pass
                return client
            except OSError:
                client.close()
        time.sleep(0.1)
    raise RuntimeError("moniteur QEMU indisponible")


def send_command(client, command):
    special = {" ": "spc", "-": "minus"}
    for char in command:
        client.sendall(("sendkey %s %d\n" %
                        (special.get(char, char.lower()), KEY_HOLD_MS)).encode("ascii"))
        time.sleep(0.35)
    client.sendall(("sendkey ret %d\n" % KEY_HOLD_MS).encode("ascii"))


def send_command_until(client, command, marker, proc, attempts=3):
    failure = None
    for _ in range(attempts):
        start = len(log_text())
        send_command(client, command)
        try:
            wait_for(marker, proc, start)
            return start
        except RuntimeError as error:
            failure = error
            time.sleep(0.4)
    raise failure


def main():
    os.makedirs(LOG_DIR, exist_ok=True)
    for path in (LOG, ERR, MON):
        try:
            os.remove(path)
        except OSError:
            pass
    command = [
        "qemu-system-i386", "-kernel", KERNEL, "-initrd", INITRD,
        "-m", "1024M", "-display", "none", "-vga", "none",
        "-serial", "file:" + LOG,
        "-monitor", "unix:%s,server,nowait" % MON,
        "-machine", "type=pc,accel=tcg", "-no-reboot", "-no-shutdown",
    ]
    with open(ERR, "wb") as err_handle:
        proc = subprocess.Popen(command, stdout=err_handle, stderr=err_handle)
        monitor = None
        try:
            wait_for("(-.-)", proc)
            monitor = connect_monitor()
            before_spawn = send_command_until(monitor, "spawn ipcserver", "spawn ok pid", proc)
            spawned = re.search(r"spawn ok pid (\d+) ipcserver", log_text()[before_spawn:])
            if not spawned:
                raise RuntimeError("PID du serveur IPC absent")
            server_pid = spawned.group(1)
            # spawn retourne le PID avant le premier tour coopératif de
            # l’enfant : céder le CPU rend le signal de disponibilité stable.
            send_command_until(monitor, "yield", "yield ok", proc)
            wait_for("ipc_server ready", proc, before_spawn, timeout=30)
            before_send = send_command_until(monitor, "ipc-send %s bonjour" % server_pid,
                                             "ipc-send ok %s 7" % server_pid, proc)
            wait_for("ipc recv from 1 type 0 data bonjour", proc, before_send)
            before_receive = send_command_until(monitor, "ipc-recv", "ipc-recv empty", proc)
            print("MOHHDY Foundation IPC contract passed")
            return 0
        finally:
            if monitor is not None:
                monitor.close()
            if proc.poll() is None:
                proc.terminate()
                try:
                    proc.wait(timeout=2)
                except subprocess.TimeoutExpired:
                    proc.kill()
            try:
                os.remove(MON)
            except OSError:
                pass


if __name__ == "__main__":
    try:
        raise SystemExit(main())
    except Exception as error:
        print("MOHHDY Foundation IPC contract failed: %s" % error, file=sys.stderr)
        raise SystemExit(1)
