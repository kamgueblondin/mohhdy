#!/usr/bin/env python3
"""Phrase GPT-2 reelle : prompt lisible, continuation lisible.

Utilise gpt2-Q3_K_M.gguf (hors git) sur le disque FAT16 et le tokenizer de
l'initrd. `ai` doit rendre une suite de mots, pas un seul fragment.
Ce n'est pas le tour de latence KVM (43,916 s / 20,224 s restent historiques).
"""
from __future__ import print_function

import os
import re
import socket
import subprocess
import sys
import time

ROOT = os.path.abspath(os.path.join(os.path.dirname(__file__), "..", ".."))
KERNEL = os.environ.get("KERNEL", os.path.join(ROOT, "build", "mohhdy.bin"))
INITRD = os.environ.get("INITRD", os.path.join(ROOT, "my_initrd.tar"))
DISK = os.environ.get("OVERLAY_DISK", os.path.join(ROOT, "build", "gpt2_gguf_fat16.img"))
LOG = os.environ.get("LOG", os.path.join(ROOT, "test_logs", "gpt2-sentences.log"))
ERR = os.environ.get("QEMU_ERR", os.path.join(ROOT, "test_logs", "gpt2-sentences.err"))
MON = os.environ.get("QEMU_MON_SOCK", os.path.join(ROOT, "test_logs", "gpt2-sentences-monitor.sock"))
BOOT_TIMEOUT = float(os.environ.get("BOOT_TIMEOUT", "300"))
GENERATION_TIMEOUT = float(os.environ.get("GGUF_GENERATION_TIMEOUT", "1200"))
KEY_DELAY = float(os.environ.get("KEY_DELAY", "0.12"))
PROMPT = os.environ.get("GPT2_SENTENCE_PROMPT", "the capital of france is")


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
            return text()
        time.sleep(0.2)
    raise RuntimeError("timeout for %r: %s" % (needle, text()[-2000:]))


def monitor():
    end = time.monotonic() + 15.0
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
    aliases = {" ": "spc", "-": "minus", ".": "dot", "?": "shift-slash", "!": "shift-1"}
    for char in command:
        name = aliases.get(char, char.lower())
        client.sendall(("sendkey %s\n" % name).encode("ascii"))
        time.sleep(KEY_DELAY)
    time.sleep(KEY_DELAY)
    client.sendall(b"sendkey ret\n")


def response_from(segment):
    clean = re.sub(r"\x1b\[[0-9;]*m", "", segment)
    matches = re.findall(r"\[GPT-2 GGUF local\]\s*([^\r\n]*)", clean)
    return matches[-1].strip() if matches else ""


def assess(reply):
    words = re.findall(r"[A-Za-z']+", reply)
    long_words = [word for word in words if len(word) >= 2]
    letters = sum(1 for ch in reply if ch.isalpha())
    spaces = reply.count(" ")
    weird = sum(1 for ch in reply if ord(ch) < 32 or ord(ch) > 126)
    camel = sum(1 for word in long_words if re.search(r"[a-z][A-Z]", word))
    punct = any(ch in reply for ch in ".!?")
    reasons = []
    if len(reply) < 70:
        reasons.append("trop court (%d)" % len(reply))
    if len(long_words) < 10:
        reasons.append("pas assez de mots (%d)" % len(long_words))
    if spaces < 8:
        reasons.append("pas assez d'espaces (%d)" % spaces)
    if letters < 40:
        reasons.append("pas assez de lettres (%d)" % letters)
    if weird:
        reasons.append("octets non imprimables (%d)" % weird)
    if long_words and camel * 4 > len(long_words):
        reasons.append("mots colles (%d/%d)" % (camel, len(long_words)))
    if not punct and len(long_words) < 12:
        reasons.append("pas de fin de phrase")
    return reasons


def main():
    if not all(os.path.isfile(path) for path in (KERNEL, INITRD, DISK)):
        raise RuntimeError("missing kernel, initrd or GGUF FAT16 disk")
    os.makedirs(os.path.dirname(LOG), exist_ok=True)
    for path in (LOG, ERR, MON):
        try:
            os.remove(path)
        except OSError:
            pass
    accel = "kvm" if os.path.exists("/dev/kvm") else "tcg"
    proc = None
    client = None
    try:
        with open(ERR, "wb") as err:
            proc = subprocess.Popen([
                "qemu-system-i386", "-cpu", "pentium3", "-kernel", KERNEL,
                "-initrd", INITRD, "-m", "1024M", "-display", "none", "-vga", "none",
                "-serial", "file:" + LOG, "-monitor", "unix:%s,server,nowait" % MON,
                "-machine", "type=pc,accel=%s" % accel, "-no-reboot", "-no-shutdown",
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
            send(client, "ai " + PROMPT)
            updated = wait_for(proc, "[GPT-2 GGUF local]", GENERATION_TIMEOUT, start)
            elapsed = time.monotonic() - started
            segment = updated[start:]
            if "[GPT-2 GGUF local] indisponible" in segment:
                raise RuntimeError("generation refused: %s" % segment[-1500:])
            if "question: " + PROMPT not in re.sub(r"\x1b\[[0-9;]*m", "", segment):
                raise RuntimeError("prompt not echoed: %s" % segment[-1500:])
            reply = response_from(segment)
            reasons = assess(reply)
            print("PROMPT\t%s" % PROMPT)
            print("RESPONSE\t%s" % reply)
            print("SECONDS\t%.1f" % elapsed)
            print("ACCEL\t%s" % accel)
            if reasons:
                raise RuntimeError("phrase illisible (%s): %r" % (", ".join(reasons), reply))
        print("GPT-2 sentence test passed")
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
        print("GPT-2 sentence test failed: %s" % error, file=sys.stderr)
        raise SystemExit(1)
