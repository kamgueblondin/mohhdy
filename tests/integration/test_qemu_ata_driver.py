#!/usr/bin/env python3
"""Tranche 4 contract: Ring 3 ATA PIO driver behind a TSS IOPB port capability.

Positive: atadriver executes IN/OUT on 0x1F0-0x1F7/0x3F6 at CPL 3 and serves a
sector window over IPC to the ata-client owner (write then read back LBA 2000).
Negative: a non-driver task cannot publish ata-driver (-58), its sector IPC is
refused by the driver, and a raw IN on 0x1F7 raises #GP which kills only that
task (kernel keeps running). The kernel overlay snapshot keeps its Ring 0 PIO.
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
LOG = os.path.join(LOG_DIR, "ata-driver.log")
ERR = os.path.join(LOG_DIR, "ata-driver.err")
MON = os.path.join(LOG_DIR, "ata-driver-monitor.sock")
DISK = os.path.join(LOG_DIR, "ata-driver.img")
KERNEL = os.path.join(ROOT, "build", "mohhdy.bin")
INITRD = os.path.join(ROOT, "my_initrd.tar")
KEY_HOLD_MS = int(os.environ.get("KEY_HOLD_MS", "10"))
GP_FAULT = "[FAULT] user task killed: int=0x0000000d"


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


def assert_no_port_leak(start):
    tail = log_text()[start:]
    if "atarogue port unexpectedly allowed" in tail:
        raise RuntimeError("ATA port reachable from a non-driver task")
    for needle in ("atarogue register unexpected", "atarogue client claim unexpected",
                   "atarogue sector ipc unexpected", "ataclient overlay region unexpected"):
        if needle in tail:
            raise RuntimeError("unexpected ATA capability outcome: %s" % needle)


def main():
    os.makedirs(LOG_DIR, exist_ok=True)
    for path in (LOG, ERR, MON, DISK):
        try:
            os.remove(path)
        except OSError:
            pass
    with open(DISK, "wb") as handle:
        handle.truncate(4 * 1024 * 1024)
    command = [
        "qemu-system-i386", "-kernel", KERNEL, "-initrd", INITRD,
        "-m", "1024M", "-display", "none", "-vga", "none",
        "-serial", "file:" + LOG,
        "-monitor", "unix:%s,server,nowait" % MON,
        "-machine", "type=pc,accel=tcg",
        "-drive", "file=%s,format=raw,if=ide,index=0,cache=writethrough" % DISK,
        "-no-reboot", "-no-shutdown",
    ]
    with open(ERR, "wb") as err_handle:
        proc = subprocess.Popen(command, stdout=err_handle, stderr=err_handle)
        monitor = None
        try:
            wait_for("(-.-)", proc)
            monitor = connect_monitor()
            time.sleep(0.5)
            # 1. No driver: every task has the ATA ports denied by the IOPB.
            _, start = spawn(monitor, proc, "atarogue")
            wait_child(monitor, proc, "atarogue register refused", start)
            wait_child(monitor, proc, GP_FAULT, start)
            assert_no_port_leak(start)
            send_command_until(monitor, "uptime", "uptime", proc)
            # 2. Driver live: Ring 3 PIO and IPC sector window (positive).
            driver_pid, start = spawn(monitor, proc, "atadriver")
            wait_child(monitor, proc, "atadriver ring3 pio ready", start)
            send_command_until(monitor, "service-find ata-driver",
                               "service-find ok ata-driver %s" % driver_pid, proc)
            client_pid, start = spawn(monitor, proc, "ataclient")
            wait_child(monitor, proc, "ataclient ring3 sector roundtrip ok", start, rounds=12)
            wait_child(monitor, proc, "ataclient overlay region refused", start, rounds=12)
            # 3. Driver live: non-driver refused at register, IPC and port.
            _, start = spawn(monitor, proc, "atarogue")
            wait_child(monitor, proc, "atarogue register refused", start)
            wait_child(monitor, proc, "atarogue client claim refused", start)
            wait_child(monitor, proc, "atarogue sector ipc refused", start, rounds=12)
            wait_child(monitor, proc, GP_FAULT, start)
            assert_no_port_leak(start)
            send_command_until(monitor, "service-find ata-driver",
                               "service-find ok ata-driver %s" % driver_pid, proc)
            # 4. Driver gone: kernel Ring 0 overlay path still persists files.
            kill(monitor, proc, client_pid)
            kill(monitor, proc, driver_pid)
            send_command_until(monitor, "service-find ata-driver",
                               "service-find: service indisponible", proc)
            send_command_until(monitor, "write t4note ok4", "write ok", proc)
            send_command_until(monitor, "cat t4note", "ok4", proc)
            print("MOHHDY Tranche 4 Ring 3 ATA driver contract passed")
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
        print("MOHHDY Tranche 4 Ring 3 ATA driver contract failed: %s" % error,
              file=sys.stderr)
        raise SystemExit(1)
