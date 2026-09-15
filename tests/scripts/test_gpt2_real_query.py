#!/usr/bin/env python3
"""Boot MOHHDY with real GPT-2 assets and issue one local shell query."""
import os
import socket
import subprocess
import sys
import time

ROOT = os.path.abspath(os.path.join(os.path.dirname(__file__), "..", ".."))
LOG_DIR = os.path.join(ROOT, "test_logs")
LOG = os.path.join(LOG_DIR, "gpt2-real-query.log")
ERR = os.path.join(LOG_DIR, "gpt2-real-query.err")
MON = os.path.join(LOG_DIR, "gpt2-real-query-monitor.sock")


def log_text():
    try:
        with open(LOG, "r", errors="replace") as handle:
            return handle.read()
    except OSError:
        return ""


def wait_for(proc, needle, timeout, start=0):
    deadline = time.time() + timeout
    while time.time() < deadline:
        if proc.poll() is not None:
            raise RuntimeError("QEMU stopped early")
        if needle in log_text()[start:]:
            return
        time.sleep(0.25)
    raise RuntimeError("missing output: %s" % needle)


def monitor():
    deadline = time.time() + 10
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
    raise RuntimeError("monitor unavailable")


def type_command(client, command):
    aliases = {" ": "spc", "-": "minus", ".": "dot"}
    for char in command:
        client.sendall(("sendkey %s\n" % aliases.get(char, char.lower())).encode("ascii"))
        time.sleep(0.08)
    client.sendall(b"sendkey ret\n")


def main():
    for asset in ("models/gpt2_124M.bin", "models/gpt2_tokenizer.bin"):
        if not os.path.isfile(os.path.join(ROOT, asset)):
            raise RuntimeError("asset missing: %s" % asset)
    subprocess.run(["make", "all"], cwd=ROOT, check=True)
    os.makedirs(LOG_DIR, exist_ok=True)
    for path in (LOG, ERR, MON):
        try:
            os.remove(path)
        except OSError:
            pass
    process = None
    client = None
    started = time.time()
    try:
        with open(ERR, "wb") as error_handle:
            process = subprocess.Popen([
                "qemu-system-i386", "-kernel", "build/mohhdy.bin", "-initrd", "my_initrd.tar",
                "-m", "1024M", "-display", "none", "-vga", "none", "-serial", "file:" + LOG,
                "-monitor", "unix:%s,server,nowait" % MON, "-no-reboot", "-no-shutdown",
            ], cwd=ROOT, stdout=error_handle, stderr=error_handle)
            wait_for(process, "(-.-)", 45)
            client = monitor()
            offset = len(log_text())
            type_command(client, "ai hello")
            wait_for(process, "[GPT-2 local]", 180, start=offset)
        elapsed = int(time.time() - started)
        print("Real GPT-2 local query passed in %ss." % elapsed)
        return 0
    finally:
        if client is not None:
            client.close()
        if process is not None and process.poll() is None:
            process.terminate()
            try:
                process.wait(timeout=3)
            except subprocess.TimeoutExpired:
                process.kill()
        try:
            os.remove(MON)
        except OSError:
            pass


if __name__ == "__main__":
    try:
        raise SystemExit(main())
    except Exception as error:
        print("Real GPT-2 local query failed: %s" % error, file=sys.stderr)
        raise SystemExit(1)
