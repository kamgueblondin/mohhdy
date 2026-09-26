#!/usr/bin/env python3
"""Tranche 5 contract: net-driver worker gate on network syscalls.

Degraded (no net-driver): netclaim keeps the historical local path.
Worker live: networker still reaches socket/peer syscalls (positive proof);
netclaim is refused with OS_NET_WORKER_REQUIRED while net-status stays open
(negative proof). Worker killed: degraded path reopens.
The NE2000 driver itself stays in Ring 0.
"""
import os
import re
import socket
import subprocess
import sys
import time

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
from test_qemu_vfs_service import normalized_log  # noqa: E402

ROOT = os.path.abspath(os.path.join(os.path.dirname(__file__), "..", ".."))
LOG_DIR = os.path.join(ROOT, "test_logs")
LOG = os.path.join(LOG_DIR, "net-worker.log")
ERR = os.path.join(LOG_DIR, "net-worker.err")
MON = os.path.join(LOG_DIR, "net-worker-monitor.sock")
KERNEL = os.path.join(ROOT, "build", "mohhdy.bin")
INITRD = os.path.join(ROOT, "my_initrd.tar")
KEY_HOLD_MS = int(os.environ.get("KEY_HOLD_MS", "10"))


def log_text():
    try:
        with open(LOG, "r", errors="replace") as handle:
            return handle.read()
    except OSError:
        return ""


def wait_for(needle, proc, offset=0, timeout=25):
    deadline = time.time() + timeout
    while time.time() < deadline:
        if proc.poll() is not None:
            raise RuntimeError("QEMU stopped early")
        if needle in normalized_log(log_text()[offset:]):
            return
        time.sleep(0.1)
    raise RuntimeError("missing output: %s" % needle)


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
    raise RuntimeError("QEMU monitor unavailable")


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


def wait_child(client, proc, needle, start, rounds=6):
    """Give a freshly spawned child bounded cooperative turns."""
    for _ in range(rounds):
        try:
            wait_for(needle, proc, start, timeout=1)
            return
        except RuntimeError:
            pass
        send_command_until(client, "yield", "yield ok", proc)
    wait_for(needle, proc, start, timeout=3)


def spawn(client, proc, name):
    start = send_command_until(client, "spawn %s" % name, "spawn ok pid", proc)
    match = re.search(r"spawn ok pid[\s\S]*?(\d+) %s" % name,
                      normalized_log(log_text()[start:]))
    if not match:
        raise RuntimeError("%s not spawned" % name)
    return match.group(1), start


def kill(client, proc, pid):
    send_command_until(client, "kill %s" % pid, "Processus %s termine" % pid, proc)


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
        "-machine", "type=pc,accel=tcg",
        "-netdev", "user,id=n0", "-device", "ne2k_isa,netdev=n0",
        "-no-reboot", "-no-shutdown",
    ]
    with open(ERR, "wb") as err_handle:
        proc = subprocess.Popen(command, stdout=err_handle, stderr=err_handle)
        monitor = None
        try:
            wait_for("(-.-)", proc)
            monitor = connect_monitor()
            time.sleep(0.5)
            send_command_until(monitor, "net-status", "Carte Ethernet : detectee", proc)
            # Degraded: no net-driver, historical local path unchanged.
            claim_pid, start = spawn(monitor, proc, "netclaim")
            wait_child(monitor, proc, "netclaim local ok", start)
            kill(monitor, proc, claim_pid)
            # Worker live: positive proof from the worker PID itself.
            worker_pid, start = spawn(monitor, proc, "networker")
            wait_child(monitor, proc, "net-driver ready", start)
            wait_child(monitor, proc, "net-driver gated syscalls ok", start)
            send_command_until(monitor, "service-find net-driver",
                               "service-find ok net-driver %s" % worker_pid, proc)
            # Negative proof: non-worker task refused, status still readable.
            claim_pid, start = spawn(monitor, proc, "netclaim")
            wait_child(monitor, proc, "netclaim worker-required enforced", start)
            kill(monitor, proc, claim_pid)
            send_command_until(monitor, "net-status", "Carte Ethernet : detectee", proc)
            # Worker gone: degraded path reopens.
            kill(monitor, proc, worker_pid)
            send_command_until(monitor, "service-find net-driver",
                               "service-find: service indisponible", proc)
            claim_pid, start = spawn(monitor, proc, "netclaim")
            wait_child(monitor, proc, "netclaim local ok", start)
            kill(monitor, proc, claim_pid)
            print("MOHHDY Tranche 5 net-driver worker gate contract passed")
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
        print("MOHHDY Tranche 5 net-driver worker gate contract failed: %s" % error,
              file=sys.stderr)
        raise SystemExit(1)
