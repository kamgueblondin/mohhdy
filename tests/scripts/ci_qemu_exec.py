#!/usr/bin/env python3
"""QEMU smoke: blocking SYS_EXEC runs bin/ok then returns to the shell."""
from __future__ import print_function

import os
import socket
import subprocess
import sys
import time

ROOT = os.path.abspath(os.path.join(os.path.dirname(__file__), "..", ".."))
KERNEL = os.environ.get("KERNEL", os.path.join(ROOT, "build", "mohhdy.bin"))
INITRD = os.environ.get("INITRD", os.path.join(ROOT, "my_initrd.tar"))
LOG_DIR = os.path.join(ROOT, "test_logs")
LOG = os.environ.get("EXEC_LOG", os.path.join(LOG_DIR, "ci-qemu-exec-serial.log"))
QEMU_ERR = os.environ.get("EXEC_ERR", os.path.join(LOG_DIR, "ci-qemu-exec-stderr.log"))
MON_SOCK = os.environ.get("EXEC_MON_SOCK", os.path.join(LOG_DIR, "qemu-exec-monitor.sock"))
BOOT_TIMEOUT = float(os.environ.get("BOOT_TIMEOUT", "40"))
CMD_TIMEOUT = float(os.environ.get("CMD_TIMEOUT", "30"))
KEY_DELAY = float(os.environ.get("KEY_DELAY", "0.25"))


def say(message):
    sys.stdout.write(message + "\n")
    sys.stdout.flush()


def log_text():
    try:
        with open(LOG, "r", errors="replace") as handle:
            return handle.read()
    except OSError:
        return ""


def wait_for(proc, needle, timeout, start=0):
    deadline = time.monotonic() + timeout
    while time.monotonic() < deadline:
        if proc.poll() is not None:
            raise RuntimeError("QEMU stopped unexpectedly; log tail:\n%s" % log_text()[-2000:])
        if needle in log_text()[start:]:
            time.sleep(0.35)
            return
        time.sleep(0.15)
    raise RuntimeError("timeout waiting for %r; log tail:\n%s" % (needle, log_text()[-2000:]))


def qemu_disk_args():
    disk = os.environ.get("OVERLAY_DISK", os.path.join(ROOT, "build", "overlay.img"))
    if os.path.isfile(disk):
        return ["-drive", "file=%s,format=raw,if=ide,cache=writethrough" % disk]
    return []


def monitor_connect():
    deadline = time.monotonic() + 10
    while time.monotonic() < deadline:
        if os.path.exists(MON_SOCK):
            client = socket.socket(socket.AF_UNIX, socket.SOCK_STREAM)
            try:
                client.connect(MON_SOCK)
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


def drain_monitor(client):
    client.settimeout(0.05)
    while True:
        try:
            data = client.recv(8192)
            if not data:
                break
        except socket.timeout:
            break


def send_command(client, command):
    aliases = {" ": "spc", "-": "minus", ".": "dot"}
    for char in command:
        client.sendall(("sendkey %s\n" % aliases.get(char, char.lower())).encode("ascii"))
        drain_monitor(client)
        time.sleep(KEY_DELAY)
    client.sendall(b"sendkey ret\n")
    drain_monitor(client)


def terminate(proc):
    if proc is None or proc.poll() is not None:
        return
    proc.terminate()
    try:
        proc.wait(timeout=4)
    except subprocess.TimeoutExpired:
        proc.kill()


def main():
    if not os.path.isfile(KERNEL) or not os.path.isfile(INITRD):
        raise RuntimeError("missing build artefacts; run make all first")
    os.makedirs(LOG_DIR, exist_ok=True)
    for path in (LOG, QEMU_ERR, MON_SOCK):
        try:
            os.remove(path)
        except OSError:
            pass

    proc = None
    monitor = None
    try:
        with open(QEMU_ERR, "wb") as err:
            proc = subprocess.Popen([
                "qemu-system-i386", "-cpu", "pentium3", "-kernel", KERNEL,
                "-initrd", INITRD, "-m", "1024M", "-display", "none", "-vga", "none",
                "-serial", "file:" + LOG, "-monitor", "unix:%s,server,nowait" % MON_SOCK,
                "-machine", "type=pc,accel=tcg", "-no-reboot", "-no-shutdown",
            ] + qemu_disk_args(), cwd=ROOT, stdout=err, stderr=err)
            wait_for(proc, "(-.-)", BOOT_TIMEOUT)
            wait_for(proc, "SYS_GETS: Debut", BOOT_TIMEOUT)
            monitor = monitor_connect()

            say("typing ok ...")
            start = len(log_text())
            send_command(monitor, "ok")
            wait_for(proc, "exec ok", CMD_TIMEOUT, start)
            wait_for(proc, "SYS_GETS: Debut", CMD_TIMEOUT, start)

            say("typing rc ...")
            start = len(log_text())
            send_command(monitor, "rc")
            wait_for(proc, "rc ok 0", CMD_TIMEOUT, start)

        say("QEMU exec smoke passed.")
        return 0
    finally:
        if monitor is not None:
            monitor.close()
        terminate(proc)
        try:
            os.remove(MON_SOCK)
        except OSError:
            pass


if __name__ == "__main__":
    try:
        raise SystemExit(main())
    except Exception as error:
        print("QEMU exec smoke failed: %s" % error, file=sys.stderr)
        raise SystemExit(1)
