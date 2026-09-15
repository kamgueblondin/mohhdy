#!/usr/bin/env python3
"""QEMU GTK screendumps of MOHHDY: shell, FAT16, overlay, net-status, NE2000, IA stub."""
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
SHOT_DIR = os.environ.get("MOHHDY_GUI_SHOT_DIR", os.path.join(LOG_DIR, "gui-captures"))
KEY_DELAY = 0.65
BOOT_TIMEOUT = 90.0

os.environ.setdefault("DISPLAY", ":1")


def log_text(path):
    try:
        with open(path, "r", errors="replace") as handle:
            return handle.read()
    except OSError:
        return ""


def wait_for(proc, log_path, needle, timeout):
    deadline = time.monotonic() + timeout
    while time.monotonic() < deadline:
        if proc.poll() is not None:
            raise RuntimeError("QEMU stopped; tail:\n%s" % log_text(log_path)[-2000:])
        if needle in log_text(log_path):
            time.sleep(0.4)
            return
        time.sleep(0.15)
    raise RuntimeError("timeout %r; tail:\n%s" % (needle, log_text(log_path)[-2000:]))


def monitor_connect(sock_path):
    deadline = time.monotonic() + 15
    while time.monotonic() < deadline:
        if os.path.exists(sock_path):
            client = socket.socket(socket.AF_UNIX, socket.SOCK_STREAM)
            try:
                client.connect(sock_path)
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
    time.sleep(0.15)
    try:
        client.recv(4096)
    except socket.timeout:
        pass


def send_command(client, command):
    aliases = {
        " ": "spc",
        "-": "minus",
        ".": "dot",
        "/": "slash",
        ":": "shift-semicolon",
        "_": "shift-minus",
        "=": "equal",
    }
    for char in command:
        mon(client, "sendkey %s" % aliases.get(char, char.lower()))
        time.sleep(KEY_DELAY)
    time.sleep(KEY_DELAY)
    mon(client, "sendkey ret")


def ppm_to_png(path):
    data = open(path, "rb").read()
    if data[:2] != b"P6":
        return
    parts = data.split(b"\n", 3)
    wh = parts[1].split()
    w, h = int(wh[0]), int(wh[1])
    raw = parts[3]
    import struct
    import zlib

    def chunk(tag, payload):
        crc = zlib.crc32(tag + payload) & 0xFFFFFFFF
        return struct.pack(">I", len(payload)) + tag + payload + struct.pack(">I", crc)

    rows = b"".join(b"\x00" + raw[y * w * 3:(y + 1) * w * 3] for y in range(h))
    png = b"\x89PNG\r\n\x1a\n"
    png += chunk(b"IHDR", struct.pack(">IIBBBBB", w, h, 8, 2, 0, 0, 0))
    png += chunk(b"IDAT", zlib.compress(rows, 9))
    png += chunk(b"IEND", b"")
    with open(path, "wb") as handle:
        handle.write(png)


def send_command_checked(client, proc, log_path, command, needle, timeout, attempts=3):
    for attempt in range(attempts):
        start = len(log_text(log_path))
        send_command(client, command)
        deadline = time.monotonic() + timeout
        while time.monotonic() < deadline:
            if proc.poll() is not None:
                raise RuntimeError("QEMU stopped; tail:\n%s" % log_text(log_path)[-2000:])
            delta = log_text(log_path)[start:]
            if needle in delta:
                return
            marker = "SYS_GETS: ligne lue: "
            marker_at = delta.rfind(marker)
            if marker_at >= 0:
                actual = delta[marker_at + len(marker):].split("\n", 1)[0].strip()
                if actual and actual != command:
                    break
            time.sleep(0.15)
        if attempt + 1 < attempts:
            time.sleep(1.0)
    raise RuntimeError("command %r did not reach %r; tail:\n%s" %
                       (command, needle, log_text(log_path)[-2000:]))


def screendump(client, name):
    path = os.path.join(SHOT_DIR, name)
    mon(client, "screendump %s" % path)
    time.sleep(0.6)
    if not os.path.isfile(path) or os.path.getsize(path) < 100:
        raise RuntimeError("screendump failed: %s" % path)
    ppm_to_png(path)
    print("saved", path, os.path.getsize(path))


def prepare_disk():
    os.makedirs(LOG_DIR, exist_ok=True)
    subprocess.run([
        sys.executable,
        os.path.join(ROOT, "tests", "scripts", "make_fat16_image.py"),
        "--image", DISK,
    ], check=True)


def terminate(proc):
    if proc is None or proc.poll() is not None:
        return
    proc.terminate()
    try:
        proc.wait(timeout=4)
    except subprocess.TimeoutExpired:
        proc.kill()


def run_session(name, extra_qemu, steps):
    log_path = os.path.join(LOG_DIR, "gui-capture-%s-serial.log" % name)
    err_path = os.path.join(LOG_DIR, "gui-capture-%s-stderr.log" % name)
    sock_path = os.path.join(LOG_DIR, "gui-capture-%s-monitor.sock" % name)
    for path in (log_path, err_path, sock_path):
        try:
            os.remove(path)
        except OSError:
            pass

    display_type = os.environ.get("QEMU_DISPLAY", "none")
    cmd = [
        "qemu-system-i386", "-cpu", "pentium3",
        "-kernel", KERNEL, "-initrd", INITRD,
        "-m", "1024M", "-vga", "std", "-display", display_type,
        "-serial", "file:" + log_path,
        "-monitor", "unix:%s,server,nowait" % sock_path,
        "-drive", "file=%s,format=raw,if=ide,cache=writethrough" % DISK,
        "-machine", "type=pc,accel=tcg",
        "-no-reboot", "-no-shutdown",
    ] + extra_qemu

    proc = None
    monitor = None
    try:
        with open(err_path, "wb") as err:
            proc = subprocess.Popen(cmd, cwd=ROOT, stdout=err, stderr=err)
            wait_for(proc, log_path, "(-.-)", BOOT_TIMEOUT)
            wait_for(proc, log_path, "SYS_GETS: Debut", BOOT_TIMEOUT)
            monitor = monitor_connect(sock_path)
            time.sleep(0.8)
            for step in steps:
                kind = step[0]
                if kind == "dump":
                    screendump(monitor, step[1])
                elif kind == "cmd":
                    send_command_checked(monitor, proc, log_path, step[1], step[2],
                                         step[3] if len(step) > 3 else 25)
                    time.sleep(0.7)
                elif kind == "key":
                    mon(monitor, "sendkey %s" % step[1])
                    time.sleep(step[2] if len(step) > 2 else 0.6)
            print("session %s done" % name)
            return 0
    finally:
        if monitor is not None:
            monitor.close()
        terminate(proc)


def main():
    os.makedirs(LOG_DIR, exist_ok=True)
    os.makedirs(SHOT_DIR, exist_ok=True)
    prepare_disk()

    core_steps = [
        ("dump", "01-shell.png"),
        ("cmd", "help", "ligne lue: help", 25),
        ("dump", "02-help-bottom.png"),
        ("key", "pgup", 0.5),
        ("key", "pgup", 0.8),
        ("dump", "03-help-pageup.png"),
        ("key", "pgdn", 0.5),
        ("key", "pgdn", 0.8),
        ("dump", "04-help-pagedown.png"),
        ("cmd", "ls", "Initrd / VFS", 20),
        ("dump", "05-ls.png"),
        ("cmd", "fat16-list", "ligne lue: fat16-list", 20),
        ("dump", "06-fat16-list.png"),
        ("cmd", "fat16-cat fatok.txt", "ligne lue: fat16-cat", 25),
        ("dump", "07-fat16-cat.png"),
        ("cmd", "write demo.txt hello", "ligne lue: write", 25),
        ("dump", "08-write.png"),
        ("cmd", "cat demo.txt", "ligne lue: cat", 20),
        ("dump", "09-cat-overlay.png"),
        ("cmd", "date", "ligne lue: date", 20),
        ("dump", "10-date.png"),
        ("cmd", "whoami", "ligne lue: whoami", 20),
        ("dump", "11-whoami.png"),
        ("cmd", "getpid", "ligne lue: getpid", 20),
        ("dump", "12-getpid.png"),
        ("cmd", "net-status", "ligne lue: net-status", 20),
        ("dump", "13-net-status.png"),
        ("cmd", "net-status json", "ligne lue: net-status json", 25),
        ("dump", "14-net-status-json.png"),
        ("cmd", "ps", "ligne lue: ps", 20),
        ("dump", "15-ps.png"),
        ("cmd", "mem", "ligne lue: mem", 20),
        ("dump", "16-mem.png"),
        ("cmd", "ai-runtime", "Runtime IA bare-metal", 25),
        ("dump", "17-ai-runtime.png"),
        ("cmd", "ai-provider openai", "OpenAI selectionne", 25),
        ("dump", "18-ai-provider-openai.png"),
        ("cmd", "ai hello", "OpenAI selectionne : utilisez ai-acquire", 30),
        ("dump", "19-ai-hello-openai.png"),
    ]
    run_session("core", [], core_steps)

    nic_steps = [
        ("dump", "20-ne2k-shell.png"),
        ("cmd", "net-status", "ligne lue: net-status", 20),
        ("dump", "21-ne2k-net-status.png"),
        ("cmd", "net-status json", '"nic":"detected"', 25),
        ("dump", "22-ne2k-net-status-json.png"),
    ]
    run_session("ne2k", ["-netdev", "user,id=n0", "-device", "ne2k_isa,netdev=n0"], nic_steps)
    print("GUI capture done")
    return 0


if __name__ == "__main__":
    sys.exit(main())
