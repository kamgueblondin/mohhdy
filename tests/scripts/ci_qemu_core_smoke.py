#!/usr/bin/env python3
"""Short deterministic QEMU smoke test for boot, shell, GPT-2 runtime and RAM overlay."""
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
LOG_DIR = os.path.join(ROOT, "test_logs")
LOG = os.environ.get("LOG", os.path.join(LOG_DIR, "ci-qemu-serial.log"))
QEMU_ERR = os.environ.get("QEMU_ERR", os.path.join(LOG_DIR, "ci-qemu-stderr.log"))
MON_SOCK = os.environ.get("QEMU_MON_SOCK", os.path.join(LOG_DIR, "qemu-core-monitor.sock"))
TEST_DISK = os.environ.get("OVERLAY_DISK", os.path.join(LOG_DIR, "qemu-core-overlay.img"))
BOOT_TIMEOUT = float(os.environ.get("BOOT_TIMEOUT", "75"))
CMD_TIMEOUT = float(os.environ.get("CMD_TIMEOUT", "20"))
# Sous TCG, chaque scan-code est confirmé par le shell. La fenêtre de
# stabilité couvre tous les caractères afin de retirer un éventuel doublon
# avant `ret` plutôt que de rejouer une ligne après son exécution.
KEY_DELAY = float(os.environ.get("KEY_DELAY", "0.05"))
KEY_HOLD_MS = int(os.environ.get("KEY_HOLD_MS", "10"))
KEY_ECHO_TIMEOUT = float(os.environ.get("KEY_ECHO_TIMEOUT", "3"))
KEY_DUPLICATE_SETTLE_DELAY = float(os.environ.get("KEY_DUPLICATE_SETTLE_DELAY", "0.25"))
KEY_CHAR_RETRIES = int(os.environ.get("KEY_CHAR_RETRIES", "3"))


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
            # Let the shell reach the next SYS_GETS before the next command.
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


def prepare_test_disk():
    directory = os.path.dirname(TEST_DISK)
    if directory:
        os.makedirs(directory, exist_ok=True)
    subprocess.run([
        sys.executable,
        os.path.join(ROOT, "tests", "scripts", "make_fat16_image.py"),
        "--image", TEST_DISK,
    ], check=True)


def qemu_disk_args():
    return ["-drive", "file=%s,format=raw,if=ide,cache=writethrough" % TEST_DISK]


class CommandEchoMismatch(RuntimeError):
    """La ligne reçue par le shell diffère de celle injectée."""


def normalized_log(output):
    """Retire seulement les diagnostics asynchrones entre deux fragments."""
    output = re.sub(r"(?<=\w)TIMER_ALIVE: tick=\d+\+?\r?\n(?=\w)", "", output)
    output = re.sub(r"TIMER_ALIVE: tick=\d+\+?\r?\n(?=\w)", "", output)
    output = re.sub(r"TIMER_ALIVE: tick=\d+\+?\r?\n(?=\s+\w)", "", output)
    output = re.sub(r"TIMER_ALIVE: tick=\d+\+?", "", output)
    return output


def send_key(client, key):
    client.sendall(("sendkey %s %d\n" % (key, KEY_HOLD_MS)).encode("ascii"))


def key_echo_count(output, char):
    pattern = r"SYS_GETS: caractère ajouté:\s*'%s'" % re.escape(char)
    return len(re.findall(pattern, normalized_log(output)))


def send_command_once(client, command, proc):
    """Prépare exactement une ligne ; aucun caractère partiel n’est exécuté."""
    aliases = {" ": "spc", "-": "minus", ".": "dot", "/": "slash"}
    for char in command:
        count = 0
        for _ in range(KEY_CHAR_RETRIES):
            start = len(log_text())
            send_key(client, aliases.get(char, char.lower()))
            deadline = time.monotonic() + KEY_ECHO_TIMEOUT
            while time.monotonic() < deadline:
                if proc.poll() is not None:
                    raise RuntimeError("QEMU stopped unexpectedly; log tail:\n%s" % log_text()[-2000:])
                time.sleep(KEY_DELAY)
                count = key_echo_count(log_text()[start:], char.lower())
                if count:
                    # Ne jamais envoyer `ret` tant qu’un second scan-code peut
                    # encore arriver : un doublon est effacé dans le buffer local.
                    time.sleep(KEY_DUPLICATE_SETTLE_DELAY)
                    count = key_echo_count(log_text()[start:], char.lower())
                    break
            if count:
                break
        if count == 0:
            raise RuntimeError("character not received: %s" % char)
        for _ in range(count - 1):
            send_key(client, "backspace")
            time.sleep(KEY_DUPLICATE_SETTLE_DELAY)
    send_key(client, "ret")


def command_echoed(output, command):
    expected = " ".join(command.lower().split())
    for received in re.findall(r"SYS_GETS: ligne lue: ([^\r\n]+)", normalized_log(output)):
        if " ".join(received.lower().split()) == expected:
            return True
    return False


def send_command_until(client, command, marker, proc):
    """Exécute une commande une fois et échoue sur tout écho incertain."""
    start = len(log_text())
    send_command_once(client, command, proc)
    deadline = time.monotonic() + CMD_TIMEOUT
    while time.monotonic() < deadline:
        if proc.poll() is not None:
            raise RuntimeError("QEMU stopped unexpectedly; log tail:\n%s" % log_text()[-2000:])
        output = normalized_log(log_text()[start:])
        if command_echoed(output, command):
            wait_for(proc, marker, CMD_TIMEOUT, start)
            # Synchronise le prochain prompt sans rejouer une opération déjà
            # entrée, qu’elle soit une mutation ou une I/O.
            wait_for(proc, "(-.-)", CMD_TIMEOUT, start)
            return
        if "SYS_GETS: ligne lue: " in output:
            raise CommandEchoMismatch("command echo altered: %s" % command)
        time.sleep(0.1)
    raise RuntimeError("timeout waiting for command echo: %s" % command)


def terminate(proc):
    if proc is None or proc.poll() is not None:
        return
    proc.terminate()
    try:
        proc.wait(timeout=4)
    except subprocess.TimeoutExpired:
        proc.kill()


def main():
    if not os.path.isfile(KERNEL) or not os.path.isfile(INITRD):
        raise RuntimeError("missing build artefacts; run make all first")
    os.makedirs(LOG_DIR, exist_ok=True)
    for path in (LOG, QEMU_ERR, MON_SOCK):
        try:
            os.remove(path)
        except OSError:
            pass

    if os.environ.get("MOHHDY_PRESERVE_FAT16") != "1":
        prepare_test_disk()
    elif not os.path.isfile(TEST_DISK):
        raise RuntimeError("missing preserved FAT16 deployment disk")
    proc = None
    monitor = None
    try:
        with open(QEMU_ERR, "wb") as err:
            proc = subprocess.Popen([
                "qemu-system-i386", "-cpu", "pentium3", "-kernel", KERNEL,
                "-initrd", INITRD, "-m", "1024M", "-display", "none", "-vga", "none",
                "-serial", "file:" + LOG, "-monitor", "unix:%s,server,nowait" % MON_SOCK,
                "-machine", "type=pc,accel=tcg", "-no-reboot", "-no-shutdown",
            ] + qemu_disk_args(), cwd=ROOT, stdout=err, stderr=err)
            wait_for(proc, "(-.-)", BOOT_TIMEOUT)
            wait_for(proc, "SYS_GETS: Debut", BOOT_TIMEOUT)
            monitor = monitor_connect()
            # Laisser l'attente clavier atteindre un état stable sur les runners CI lents.
            time.sleep(0.6)

            commands = (
                ("ls", "Initrd / VFS"),
                ("fat16-list", "FATOK.TXT"),
                ("fat16-cat FATOK.TXT", "FAT16 fixture OK"),
                ("ai-runtime", "cache KV actif"),
                ("mkdir qd", "mkdir ok qd"),
                ("cp hello.txt qd", "cp ok hello.txt"),
                ("cp qd qc", "cp ok qc"),
                ("ls qc", "hello.txt"),
                ("append q.txt ok", "append ok q.txt"),
                ("cat q.txt", "ok"),
            )
            for command, marker in commands:
                say("typing %s ..." % command)
                send_command_until(monitor, command, marker, proc)
        say("QEMU core smoke passed.")
        return 0
    finally:
        if monitor is not None:
            monitor.close()
        terminate(proc)
        try:
            os.remove(MON_SOCK)
        except OSError:
            pass


if __name__ == "__main__":
    try:
        raise SystemExit(main())
    except Exception as error:
        print("QEMU core smoke failed: %s" % error, file=sys.stderr)
        raise SystemExit(1)
