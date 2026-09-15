#!/usr/bin/env python3
"""Measure host-observed latency from an MOHHDY prompt to the GPT-2 response."""
import os
import socket
import subprocess
import sys
import time

ROOT = os.path.abspath(os.path.join(os.path.dirname(__file__), "..", ".."))
LOG_DIR = os.path.join(ROOT, "test_logs")
LOG = os.path.join(LOG_DIR, "gpt2-kv-benchmark.log")
ERR = os.path.join(LOG_DIR, "gpt2-kv-benchmark.err")
MON = os.path.join(LOG_DIR, "gpt2-kv-benchmark-monitor.sock")


def read_log():
    try:
        with open(LOG, "r", errors="replace") as handle:
            return handle.read()
    except OSError:
        return ""


def wait_for(proc, marker, timeout, start=0):
    end = time.monotonic() + timeout
    while time.monotonic() < end:
        if proc.poll() is not None:
            raise RuntimeError("QEMU stopped unexpectedly")
        text = read_log()
        if marker in text[start:]:
            return text
        time.sleep(0.20)
    raise RuntimeError("timed out waiting for %r" % marker)


def connect_monitor():
    end = time.monotonic() + 10
    while time.monotonic() < end:
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
        time.sleep(0.06)
    client.sendall(b"sendkey ret\n")


def main():
    for asset in ("models/gpt2_124M.bin", "models/gpt2_tokenizer.bin"):
        if not os.path.isfile(os.path.join(ROOT, asset)):
            raise RuntimeError("missing model asset: %s" % asset)
    subprocess.run(["make", "all"], cwd=ROOT, check=True)
    os.makedirs(LOG_DIR, exist_ok=True)
    for path in (LOG, ERR, MON):
        try:
            os.remove(path)
        except OSError:
            pass
    proc = None
    client = None
    try:
        with open(ERR, "wb") as err:
            proc = subprocess.Popen([
                "qemu-system-i386", "-cpu", "pentium3", "-kernel", "build/mohhdy.bin", "-initrd", "my_initrd.tar",
                "-m", "1024M", "-display", "none", "-vga", "none", "-serial", "file:" + LOG,
                "-monitor", "unix:%s,server,nowait" % MON, "-no-reboot", "-no-shutdown",
            ], cwd=ROOT, stdout=err, stderr=err)
            wait_for(proc, "(-.-)", 75)
            client = connect_monitor()
            start_offset = len(read_log())
            start_time = time.monotonic()
            send_command(client, "ai hello")
            text = wait_for(proc, "[GPT-2 local]", 240, start=start_offset)
            cold_elapsed = time.monotonic() - start_time
            response = text[text.rfind("[GPT-2 local]"):].split("\n", 1)[0]
            warm_offset = len(text)
            warm_start = time.monotonic()
            send_command(client, "ai hello")
            warm_text = wait_for(proc, "[GPT-2 local]", 240, start=warm_offset)
            warm_elapsed = time.monotonic() - warm_start
            print("COLD_LATENCY_SECONDS=%.3f" % cold_elapsed)
            print("WARM_LATENCY_SECONDS=%.3f" % warm_elapsed)
            print("KV_REUSE=1")
            print("%s" % response)
        return 0
    finally:
        if client is not None:
            client.close()
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
        print("KV benchmark failed: %s" % error, file=sys.stderr)
        raise SystemExit(1)
