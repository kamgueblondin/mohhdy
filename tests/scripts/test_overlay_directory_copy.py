#!/usr/bin/env python3
"""Exercise recursive directory copy in the MOHHDY overlay through the real shell."""
import os
import socket
import subprocess
import sys
import time

ROOT = os.path.abspath(os.path.join(os.path.dirname(__file__), "..", ".."))
LOG_DIR = os.path.join(ROOT, "test_logs")
LOG = os.path.join(LOG_DIR, "overlay-directory-copy.log")
ERR = os.path.join(LOG_DIR, "overlay-directory-copy.err")
MON = os.path.join(LOG_DIR, "overlay-directory-copy-monitor.sock")


def read_log():
    try:
        with open(LOG, "r", errors="replace") as handle:
            return handle.read()
    except OSError:
        return ""


def wait_for(proc, marker, timeout, start=0):
    deadline = time.monotonic() + timeout
    while time.monotonic() < deadline:
        if proc.poll() is not None:
            raise RuntimeError("QEMU stopped unexpectedly; log tail:\n%s" % read_log()[-2000:])
        if marker in read_log()[start:]:
            return
        time.sleep(0.15)
    raise RuntimeError("timed out waiting for %r; log tail:\n%s" % (marker, read_log()[-2000:]))


def connect_monitor():
    deadline = time.monotonic() + 10
    while time.monotonic() < deadline:
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
    aliases = {" ": "spc", "-": "minus", ".": "dot"}
    for char in command:
        client.sendall(("sendkey %s\n" % aliases.get(char, char.lower())).encode("ascii"))
        time.sleep(0.16)
    client.sendall(b"sendkey ret\n")


def main():
    subprocess.run(["make", "all"], cwd=ROOT, check=True)
    os.makedirs(LOG_DIR, exist_ok=True)
    for path in (LOG, ERR, MON):
        try:
            os.remove(path)
        except OSError:
            pass

    proc = None
    monitor = None
    try:
        with open(ERR, "wb") as err:
            proc = subprocess.Popen([
                "qemu-system-i386", "-cpu", "pentium3", "-kernel", "build/mohhdy.bin",
                "-initrd", "my_initrd.tar", "-m", "1024M", "-display", "none", "-vga", "none",
                "-serial", "file:" + LOG, "-monitor", "unix:%s,server,nowait" % MON,
                "-no-reboot", "-no-shutdown",
            ], cwd=ROOT, stdout=err, stderr=err)
            wait_for(proc, "(-.-)", 75)
            monitor = connect_monitor()

            for command, marker in (
                ("mkdir mydir", "mkdir ok mydir"),
                ("cp hello.txt mydir", "cp ok hello.txt"),
                ("cp mydir cpd", "cp ok cpd"),
                ("ls cpd", "hello.txt"),
            ):
                start = len(read_log())
                send_command(monitor, command)
                wait_for(proc, marker, 30, start=start)
        print("Overlay directory copy test passed: copied directory contains hello.txt.")
        return 0
    finally:
        if monitor is not None:
            monitor.close()
        if proc is not None and proc.poll() is None:
            proc.terminate()
            try:
                proc.wait(timeout=4)
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
        print("Overlay directory copy test failed: %s" % error, file=sys.stderr)
        raise SystemExit(1)
