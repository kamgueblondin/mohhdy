#!/usr/bin/env python3
"""Record a short QEMU GTK demo video on DISPLAY=:1 (xfce desktop)."""
from __future__ import print_function

import os
import socket
import subprocess
import sys
import time

ROOT = os.path.abspath(os.path.join(os.path.dirname(__file__), "..", ".."))
KERNEL = os.path.join(ROOT, "build", "mohhdy.bin")
INITRD = os.path.join(ROOT, "my_initrd.tar")
DISK = os.path.join(ROOT, "test_logs", "gui-capture-overlay.img")
LOG_DIR = os.path.join(ROOT, "test_logs")
LOG = os.path.join(LOG_DIR, "gui-record-serial.log")
ERR = os.path.join(LOG_DIR, "gui-record-stderr.log")
MON_SOCK = os.path.join(LOG_DIR, "gui-record-monitor.sock")
VIDEO_DIR = "/opt/cursor/artifacts"
VIDEO = os.path.join(VIDEO_DIR, "mohhdy-qemu-demo.mp4")
KEY_DELAY = 0.55
BOOT_TIMEOUT = 90.0

os.environ.setdefault("DISPLAY", ":1")


def log_text():
    try:
        with open(LOG, "r", errors="replace") as handle:
            return handle.read()
    except OSError:
        return ""


def wait_for(proc, needle, timeout):
    deadline = time.monotonic() + timeout
    while time.monotonic() < deadline:
        if proc.poll() is not None:
            raise RuntimeError("QEMU stopped; tail:\n%s" % log_text()[-2000:])
        if needle in log_text():
            time.sleep(0.3)
            return
        time.sleep(0.15)
    raise RuntimeError("timeout %r; tail:\n%s" % (needle, log_text()[-2000:]))


def monitor_connect():
    deadline = time.monotonic() + 15
    while time.monotonic() < deadline:
        if os.path.exists(MON_SOCK):
            client = socket.socket(socket.AF_UNIX, socket.SOCK_STREAM)
            try:
                client.connect(MON_SOCK)
                client.settimeout(0.2)
                try:
                    client.recv(4096)
                except socket.timeout:
                    pass
                return client
            except OSError:
                client.close()
        time.sleep(0.1)
    raise RuntimeError("monitor unavailable")


def mon(client, line):
    client.sendall((line + "\n").encode("ascii"))
    time.sleep(0.12)
    try:
        client.recv(4096)
    except socket.timeout:
        pass


def send_command(client, command):
    aliases = {" ": "spc", "-": "minus", ".": "dot", "/": "slash"}
    for char in command:
        mon(client, "sendkey %s" % aliases.get(char, char.lower()))
        time.sleep(KEY_DELAY)
    time.sleep(0.4)
    mon(client, "sendkey ret")


def terminate(proc):
    if proc is None or proc.poll() is not None:
        return
    proc.terminate()
    try:
        proc.wait(timeout=4)
    except subprocess.TimeoutExpired:
        proc.kill()


def start_ffmpeg():
    os.makedirs(VIDEO_DIR, exist_ok=True)
    return subprocess.Popen([
        "ffmpeg", "-y",
        "-f", "x11grab",
        "-video_size", "1280x800",
        "-framerate", "10",
        "-i", ":1+0,0",
        "-an",
        "-c:v", "libx264", "-pix_fmt", "yuv420p", "-preset", "veryfast",
        VIDEO,
    ], stdout=subprocess.DEVNULL, stderr=subprocess.DEVNULL)


def main():
    os.makedirs(LOG_DIR, exist_ok=True)
    if not os.path.isfile(DISK):
        subprocess.run([
            sys.executable,
            os.path.join(ROOT, "tests", "scripts", "make_fat16_image.py"),
            "--image", DISK,
        ], check=True)
    for path in (LOG, ERR, MON_SOCK):
        try:
            os.remove(path)
        except OSError:
            pass

    qemu = None
    monitor = None
    rec = None
    try:
        rec = start_ffmpeg()
        time.sleep(0.8)
        with open(ERR, "wb") as err:
            qemu = subprocess.Popen([
                "qemu-system-i386", "-name", "MOHHDY",
                "-cpu", "pentium3",
                "-kernel", KERNEL, "-initrd", INITRD,
                "-m", "1024M", "-vga", "std", "-display", "gtk",
                "-serial", "file:" + LOG,
                "-monitor", "unix:%s,server,nowait" % MON_SOCK,
                "-drive", "file=%s,format=raw,if=ide,cache=writethrough" % DISK,
                "-netdev", "user,id=n0", "-device", "ne2k_isa,netdev=n0",
                "-machine", "type=pc,accel=tcg",
                "-no-reboot", "-no-shutdown",
            ], cwd=ROOT, stdout=err, stderr=err)
            wait_for(qemu, "(-.-)", BOOT_TIMEOUT)
            wait_for(qemu, "SYS_GETS: Debut", BOOT_TIMEOUT)
            monitor = monitor_connect()
            time.sleep(1.2)
            for command, needle in (
                ("help", "ligne lue: help"),
                ("ls", "Initrd / VFS"),
                ("fat16-list", "ligne lue: fat16-list"),
                ("fat16-cat fatok.txt", "ligne lue: fat16-cat"),
                ("date", "ligne lue: date"),
                ("whoami", "ligne lue: whoami"),
                ("net-status", "ligne lue: net-status"),
                ("net-status json", "ligne lue: net-status json"),
                ("ai-provider openai", "OpenAI selectionne"),
            ):
                send_command(monitor, command)
                wait_for(qemu, needle, 25)
                time.sleep(1.5)
            time.sleep(2.0)
            print("recorded", VIDEO)
            return 0
    finally:
        if monitor is not None:
            monitor.close()
        terminate(qemu)
        if rec is not None and rec.poll() is None:
            rec.send_signal(2)
            try:
                rec.wait(timeout=8)
            except subprocess.TimeoutExpired:
                rec.kill()
        if os.path.isfile(VIDEO):
            print("video bytes", os.path.getsize(VIDEO))


if __name__ == "__main__":
    sys.exit(main())
