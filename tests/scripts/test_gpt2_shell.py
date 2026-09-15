#!/usr/bin/env python3
"""End-to-end shell test for the bare-metal GPT-2 syscall path."""
import os
import shutil
import socket
import struct
import subprocess
import sys
import time

ROOT = os.path.abspath(os.path.join(os.path.dirname(__file__), "..", ".."))
MODELS = os.path.join(ROOT, "models")
CHECKPOINT = os.path.join(MODELS, "gpt2_124M.bin")
TOKENIZER = os.path.join(MODELS, "gpt2_tokenizer.bin")
LOG_DIR = os.path.join(ROOT, "test_logs")
LOG = os.path.join(LOG_DIR, "gpt2-shell-smoke.log")
ERR = os.path.join(LOG_DIR, "gpt2-shell-smoke.err")
MON = os.path.join(LOG_DIR, "gpt2-shell-monitor.sock")


def parameter_count(max_t, vocab, layers, heads, channels, padded_vocab):
    del vocab, heads
    c = channels
    return (
        padded_vocab * c + max_t * c + layers * c + layers * c +
        layers * (3 * c) * c + layers * (3 * c) +
        layers * c * c + layers * c + layers * c + layers * c +
        layers * (4 * c) * c + layers * (4 * c) +
        layers * c * (4 * c) + layers * c + c + c
    )


def write_assets():
    os.makedirs(MODELS, exist_ok=True)
    config = (4, 8, 1, 1, 4, 8)
    header = [0] * 256
    header[0], header[1] = 20240326, 3
    for index, value in enumerate(config, start=2):
        header[index] = value
    with open(CHECKPOINT, "wb") as handle:
        handle.write(struct.pack("<256I", *header))
        handle.write(b"\x00" * (parameter_count(*config) * 4))
    tokenizer = [0] * 256
    tokenizer[0], tokenizer[1], tokenizer[2], tokenizer[3] = 20240328, 2, 8, 7
    with open(TOKENIZER, "wb") as handle:
        handle.write(struct.pack("<256I", *tokenizer))
        for piece in (b"a", b"b", b"c", b"d", b"e", b"f", b"g", b" "):
            handle.write(bytes([len(piece)]))
            handle.write(piece)


def read_log():
    try:
        with open(LOG, "r", errors="replace") as handle:
            return handle.read()
    except OSError:
        return ""


def wait_for(proc, needle, timeout=15, start=0):
    deadline = time.time() + timeout
    while time.time() < deadline:
        if proc.poll() is not None:
            raise RuntimeError("QEMU stopped early")
        if needle in read_log()[start:]:
            return
        time.sleep(0.1)
    raise RuntimeError("missing log output: %s" % needle)


def monitor_connect():
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
    aliases = {" ": "spc", "-": "minus", ".": "dot"}
    for char in command:
        client.sendall(("sendkey %s\n" % aliases.get(char, char.lower())).encode("ascii"))
        time.sleep(0.08)
    client.sendall(b"sendkey ret\n")


def main():
    backups = []
    for path in (CHECKPOINT, TOKENIZER):
        if os.path.exists(path):
            saved = path + ".saved-for-test"
            shutil.move(path, saved)
            backups.append((path, saved))
    proc = None
    monitor = None
    try:
        write_assets()
        subprocess.run(["make", "all"], cwd=ROOT, check=True)
        os.makedirs(LOG_DIR, exist_ok=True)
        for path in (LOG, ERR, MON):
            try:
                os.remove(path)
            except OSError:
                pass
        with open(ERR, "wb") as error_handle:
            proc = subprocess.Popen([
                "qemu-system-i386", "-kernel", "build/mohhdy.bin", "-initrd", "my_initrd.tar",
                "-m", "128M", "-display", "none", "-vga", "none", "-serial", "file:" + LOG,
                "-monitor", "unix:%s,server,nowait" % MON, "-no-reboot", "-no-shutdown",
            ], cwd=ROOT, stdout=error_handle, stderr=error_handle)
            wait_for(proc, "(-.-)")
            monitor = monitor_connect()
            start = len(read_log())
            send_command(monitor, "ai a")
            wait_for(proc, "[GPT-2 local]", start=start)
        print("GPT-2 shell smoke passed.")
        return 0
    finally:
        if monitor is not None:
            monitor.close()
        if proc is not None and proc.poll() is None:
            proc.terminate()
            try:
                proc.wait(timeout=2)
            except subprocess.TimeoutExpired:
                proc.kill()
        try:
            os.remove(MON)
        except OSError:
            pass
        for path in (CHECKPOINT, TOKENIZER):
            try:
                os.remove(path)
            except OSError:
                pass
        for original, saved in backups:
            shutil.move(saved, original)
        subprocess.run(["make", "all"], cwd=ROOT, check=True)


if __name__ == "__main__":
    try:
        raise SystemExit(main())
    except Exception as error:
        print("GPT-2 shell smoke failed: %s" % error, file=sys.stderr)
        raise SystemExit(1)
