#!/usr/bin/env python3
"""Two QEMU boots: write overlay file, kill, reboot, cat the same file from disk."""
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
LOG = os.environ.get("PERSIST_LOG", os.path.join(LOG_DIR, "ci-qemu-persist-serial.log"))
QEMU_ERR = os.environ.get("PERSIST_ERR", os.path.join(LOG_DIR, "ci-qemu-persist-stderr.log"))
MON_SOCK = os.environ.get("PERSIST_MON_SOCK", os.path.join(LOG_DIR, "qemu-persist-monitor.sock"))
DISK = os.environ.get("PERSIST_DISK", os.path.join(ROOT, "build", "overlay-persist.img"))
BOOT_TIMEOUT = float(os.environ.get("BOOT_TIMEOUT", "40"))
CMD_TIMEOUT = float(os.environ.get("CMD_TIMEOUT", "20"))
KEY_DELAY = float(os.environ.get("KEY_DELAY", "0.65"))


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


def qemu_cmd():
    return [
        "qemu-system-i386", "-cpu", "pentium3", "-kernel", KERNEL,
        "-initrd", INITRD, "-m", "1024M", "-display", "none", "-vga", "none",
        "-serial", "file:" + LOG, "-monitor", "unix:%s,server,nowait" % MON_SOCK,
        "-machine", "type=pc,accel=tcg", "-no-reboot", "-no-shutdown",
        "-drive", "file=%s,format=raw,if=ide,cache=writethrough" % DISK,
    ]


def boot_and_type(command, marker, verify_command=None, verify_marker=None):
    proc = None
    monitor = None
    try:
        os.remove(MON_SOCK)
    except OSError:
        pass
    try:
        with open(QEMU_ERR, "ab") as err:
            proc = subprocess.Popen(qemu_cmd(), cwd=ROOT, stdout=err, stderr=err)
            wait_for(proc, "(-.-)", BOOT_TIMEOUT)
            wait_for(proc, "SYS_GETS: Debut", BOOT_TIMEOUT)
            monitor = monitor_connect()
            # Stabiliser GETS/PS2 avant la première touche HMP sur les runners lents.
            time.sleep(0.6)
            # QEMU TCG peut exceptionnellement dupliquer une touche PS/2 sur un
            # runner chargé. Chaque tentative conserve le même marqueur strict ;
            # elle ne masque donc aucune divergence fonctionnelle.
            last_error = None
            for attempt in range(1, 4):
                say("typing %s (attempt %d/3) ..." % (command, attempt))
                start = len(log_text())
                send_command(monitor, command)
                try:
                    wait_for(proc, marker, CMD_TIMEOUT, start)
                    if verify_command is not None:
                        say("verifying %s (attempt %d/3) ..." % (verify_command, attempt))
                        start = len(log_text())
                        send_command(monitor, verify_command)
                        wait_for(proc, verify_marker, CMD_TIMEOUT, start)
                    return
                except RuntimeError as error:
                    last_error = error
            raise last_error
    finally:
        if monitor is not None:
            monitor.close()
        terminate(proc)
        try:
            os.remove(MON_SOCK)
        except OSError:
            pass


def prepare_disk():
    os.makedirs(os.path.dirname(DISK), exist_ok=True)
    with open(DISK, "wb") as handle:
        handle.write(b"\x00" * (512 * 64))


def main():
    if not os.path.isfile(KERNEL) or not os.path.isfile(INITRD):
        raise RuntimeError("missing build artefacts; run make all first")
    os.makedirs(LOG_DIR, exist_ok=True)
    prepare_disk()
    for path in (LOG, QEMU_ERR, MON_SOCK):
        try:
            os.remove(path)
        except OSError:
            pass

    say("=== QEMU overlay persist boot 1 (write) ===")
    boot_and_type("write k.txt v7ok", "write ok k.txt", "cat k.txt", "v7ok")
    say("=== QEMU overlay persist boot 2 (cat after reboot) ===")
    boot_and_type("cat k.txt", "v7ok")
    say("QEMU overlay persist smoke passed.")
    return 0


if __name__ == "__main__":
    try:
        raise SystemExit(main())
    except Exception as error:
        print("QEMU persist smoke failed: %s" % error, file=sys.stderr)
        raise SystemExit(1)
