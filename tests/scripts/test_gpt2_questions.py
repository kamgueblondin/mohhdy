#!/usr/bin/env python3
"""Exercise multiple real GPT-2 prompts through the MOHHDY shell in QEMU."""
import os
import re
import socket
import subprocess
import sys
import time

ROOT = os.path.abspath(os.path.join(os.path.dirname(__file__), "..", ".."))
LOG_DIR = os.path.join(ROOT, "test_logs")
LOG = os.path.join(LOG_DIR, "gpt2-questions.log")
ERR = os.path.join(LOG_DIR, "gpt2-questions.err")
MON = os.path.join(LOG_DIR, "gpt2-questions-monitor.sock")
QUESTIONS = [
    "hello",
    "the capital of France is",
    "once upon a time",
]


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
        text = log_text()
        if needle in text[start:]:
            return text
        time.sleep(0.25)
    raise RuntimeError("missing output: %s" % needle)


def monitor_connect():
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
    raise RuntimeError("QEMU monitor unavailable")


def type_command(client, command):
    aliases = {" ": "spc", "-": "minus", ".": "dot"}
    for char in command:
        client.sendall(("sendkey %s\n" % aliases.get(char, char.lower())).encode("ascii"))
        time.sleep(0.06)
    client.sendall(b"sendkey ret\n")


def response_from(segment):
    clean = re.sub(r"\x1b\[[0-9;]*m", "", segment)
    matches = re.findall(r"\[GPT-2 local\]\s*([^\r\n]*)", clean)
    return matches[-1].strip() if matches else "(sortie introuvable)"


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

    proc = None
    client = None
    results = []
    try:
        with open(ERR, "wb") as error_handle:
            proc = subprocess.Popen([
                "qemu-system-i386", "-kernel", "build/mohhdy.bin", "-initrd", "my_initrd.tar",
                "-m", "1024M", "-display", "none", "-vga", "none", "-serial", "file:" + LOG,
                "-monitor", "unix:%s,server,nowait" % MON, "-no-reboot", "-no-shutdown",
            ], cwd=ROOT, stdout=error_handle, stderr=error_handle)
            wait_for(proc, "(-.-)", 70)
            client = monitor_connect()
            for question in QUESTIONS:
                offset = len(log_text())
                started = time.time()
                type_command(client, "ai " + question)
                updated = wait_for(proc, "[GPT-2 local]", 210, start=offset)
                elapsed = round(time.time() - started, 1)
                segment = updated[offset:]
                results.append((question, response_from(segment), elapsed))
                wait_for(proc, "(-.-)", 30, start=offset)
        for question, response, elapsed in results:
            print("QUESTION\t%s\nRESPONSE\t%s\nSECONDS\t%.1f\n" % (question, response, elapsed))
        return 0
    finally:
        if client is not None:
            client.close()
        if proc is not None and proc.poll() is None:
            proc.terminate()
            try:
                proc.wait(timeout=3)
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
        print("GPT-2 multi-question test failed: %s" % error, file=sys.stderr)
        raise SystemExit(1)
