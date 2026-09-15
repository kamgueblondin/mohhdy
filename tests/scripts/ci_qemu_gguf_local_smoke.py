#!/usr/bin/env python3
"""Smoke QEMU optionnel : sélection shell puis première génération GPT-2 GGUF FAT16."""

from __future__ import print_function
import os
import socket
import subprocess
import sys
import time

ROOT = os.path.abspath(os.path.join(os.path.dirname(__file__), "..", ".."))
KERNEL = os.environ.get("KERNEL", os.path.join(ROOT, "build", "mohhdy.bin"))
INITRD = os.environ.get("INITRD", os.path.join(ROOT, "my_initrd.tar"))
DISK = os.environ.get("OVERLAY_DISK", os.path.join(ROOT, "build", "gpt2_gguf_fat16.img"))
LOG = os.environ.get("LOG", os.path.join(ROOT, "test_logs", "ci-qemu-gguf-local.log"))
ERR = os.environ.get("QEMU_ERR", os.path.join(ROOT, "test_logs", "ci-qemu-gguf-local.err"))
MON = os.environ.get("QEMU_MON_SOCK", os.path.join(ROOT, "test_logs", "qemu-gguf-monitor.sock"))
BOOT_TIMEOUT = float(os.environ.get("BOOT_TIMEOUT", "90"))
GENERATION_TIMEOUT = float(os.environ.get("GGUF_GENERATION_TIMEOUT", "600"))
KEY_DELAY = float(os.environ.get("KEY_DELAY", "0.65"))


def text():
    try:
        with open(LOG, "r", errors="replace") as handle:
            return handle.read()
    except OSError:
        return ""


def wait_for(proc, needle, timeout, start=0):
    end = time.monotonic() + timeout
    while time.monotonic() < end:
        if proc.poll() is not None:
            raise RuntimeError("QEMU stopped: %s" % text()[-2000:])
        if needle in text()[start:]:
            time.sleep(0.4)
            return
        time.sleep(0.15)
    raise RuntimeError("timeout for %r: %s" % (needle, text()[-2000:]))


def monitor():
    end = time.monotonic() + 10.0
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


def send(client, command):
    aliases = {" ": "spc", "-": "minus", ".": "dot"}
    for char in command:
        client.sendall(("sendkey %s\n" % aliases.get(char, char.lower())).encode("ascii"))
        time.sleep(KEY_DELAY)
    time.sleep(KEY_DELAY)
    client.sendall(b"sendkey ret\n")


def main():
    if not all(os.path.isfile(path) for path in (KERNEL, INITRD, DISK)):
        raise RuntimeError("missing kernel, initrd or GGUF FAT16 disk")
    os.makedirs(os.path.dirname(LOG), exist_ok=True)
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
                "qemu-system-i386", "-cpu", "pentium3", "-kernel", KERNEL,
                "-initrd", INITRD, "-m", "1024M", "-display", "none", "-vga", "none",
                "-serial", "file:" + LOG, "-monitor", "unix:%s,server,nowait" % MON,
                "-machine", "type=pc,accel=tcg", "-no-reboot", "-no-shutdown",
                "-drive", "file=%s,format=raw,if=ide,cache=writethrough" % DISK,
            ], cwd=ROOT, stdout=err, stderr=err)
            wait_for(proc, "GGUF: profil local FAT16 pret", BOOT_TIMEOUT)
            wait_for(proc, "SYS_GETS: Debut", BOOT_TIMEOUT)
            client = monitor()
            start = len(text())
            send(client, "ai-model use gpt2.gguf")
            wait_for(proc, "Profil GPT-2 GGUF selectionne", BOOT_TIMEOUT, start)
            start = len(text())
            started = time.monotonic()
            send(client, "ai bonjour")
            wait_for(proc, "[GPT-2 GGUF local]", GENERATION_TIMEOUT, start)
            if "[GPT-2 GGUF local] indisponible" in text()[start:]:
                raise RuntimeError("GGUF local rejected generation: %s" % text()[start:][-1000:])
            first_token_elapsed = time.monotonic() - started
            start = len(text())
            continued = time.monotonic()
            send(client, "ai-continue")
            wait_for(proc, "[GPT-2 GGUF local suite]", GENERATION_TIMEOUT, start)
            if "session indisponible" in text()[start:]:
                raise RuntimeError("GGUF continuation rejected: %s" % text()[start:][-1000:])
            continue_elapsed = time.monotonic() - continued
        print("QEMU GGUF local smoke passed: premier token %.2fs, continuation %.2fs." %
              (first_token_elapsed, continue_elapsed))
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
        print("QEMU GGUF local smoke failed: %s" % error, file=sys.stderr)
        raise SystemExit(1)
