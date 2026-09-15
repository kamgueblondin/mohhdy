#!/usr/bin/env python3
"""Confirm that MOHHDY accepts a regular shell command after real GPT-2 inference."""
import os
import socket
import subprocess
import sys
import time

ROOT = os.path.abspath(os.path.join(os.path.dirname(__file__), "..", ".."))
LOG_DIR = os.path.join(ROOT, "test_logs")
LOG = os.path.join(LOG_DIR, "gpt2-recovery.log")
ERR = os.path.join(LOG_DIR, "gpt2-recovery.err")
MON = os.path.join(LOG_DIR, "gpt2-recovery-monitor.sock")


def read_log():
    try:
        with open(LOG, "r", errors="replace") as handle:
            return handle.read()
    except OSError:
        return ""


def wait_for(proc, needle, timeout, start=0):
    end = time.time() + timeout
    while time.time() < end:
        if proc.poll() is not None:
            raise RuntimeError("QEMU stopped unexpectedly")
        if needle in read_log()[start:]:
            return
        time.sleep(0.25)
    raise RuntimeError("timed out waiting for %r" % needle)


def monitor_connect():
    end = time.time() + 10
    while time.time() < end:
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
    special = {" ": "spc", "-": "minus", ".": "dot"}
    for char in command:
        client.sendall(("sendkey %s\n" % special.get(char, char.lower())).encode("ascii"))
        time.sleep(0.08)
    client.sendall(b"sendkey ret\n")


def main():
    for name in ("models/gpt2_124M.bin", "models/gpt2_tokenizer.bin"):
        if not os.path.isfile(os.path.join(ROOT, name)):
            raise RuntimeError("missing real model asset: %s" % name)
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
                "qemu-system-i386", "-cpu", "pentium3", "-kernel", "build/mohhdy.bin", "-initrd", "my_initrd.tar",
                "-m", "1024M", "-display", "none", "-vga", "none", "-serial", "file:" + LOG,
                "-monitor", "unix:%s,server,nowait" % MON, "-no-reboot", "-no-shutdown",
            ], cwd=ROOT, stdout=err, stderr=err)
            wait_for(proc, "(-.-)", 75)
            monitor = monitor_connect()
            runtime_start = len(read_log())
            send_command(monitor, "ai-runtime")
            wait_for(proc, "cache KV actif", 30, start=runtime_start)
            initial = len(read_log())
            send_command(monitor, "ai hello")
            wait_for(proc, "[GPT-2 local]", 240, start=initial)
            # Le prompt de reprise peut etre imprime dans la meme lecture que la reponse GPT-2.
            # On envoie donc la commande suivante des que le marqueur de reponse est observe.
            command_start = len(read_log())
            send_command(monitor, "rc")
            wait_for(proc, "rc ok", 30, start=command_start)
        print("GPT-2 recovery test passed: runtime reported KV cache and shell accepted rc after local generation.")
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
        print("GPT-2 recovery test failed: %s" % error, file=sys.stderr)
        raise SystemExit(1)
