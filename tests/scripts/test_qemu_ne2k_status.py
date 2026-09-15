#!/usr/bin/env python3
"""Smoke QEMU du statut NIC dynamique avec un contrôleur NE2000 ISA."""
import os
import re
import socket
import subprocess
import time

ROOT = os.path.abspath(os.path.join(os.path.dirname(__file__), "..", ".."))
LOG_DIR = os.path.join(ROOT, "test_logs")
LOG = os.path.join(LOG_DIR, "ne2k-status.log")
ERR = os.path.join(LOG_DIR, "ne2k-status.err")
MON = os.path.join(LOG_DIR, "ne2k-status-monitor.sock")
KEY_DELAY = 0.15
KEY_HOLD_MS = 10
KEY_ECHO_TIMEOUT = 3
KEY_DUPLICATE_SETTLE_DELAY = 0.30
KEY_CHAR_RETRIES = 3
COMMAND_RETRIES = 3


def text():
    try:
        with open(LOG, errors="replace") as handle:
            return handle.read()
    except OSError:
        return ""


def wait(needle, proc, timeout=20):
    end = time.time() + timeout
    while time.time() < end:
        if proc.poll() is not None:
            raise RuntimeError("QEMU stopped early")
        if needle in text():
            return
        time.sleep(0.1)
    raise RuntimeError("missing output: " + needle)


def monitor():
    end = time.time() + 5
    while time.time() < end:
        if os.path.exists(MON):
            client = socket.socket(socket.AF_UNIX, socket.SOCK_STREAM)
            try:
                client.connect(MON)
                client.settimeout(0.1)
                return client
            except OSError:
                client.close()
        time.sleep(0.1)
    raise RuntimeError("monitor unavailable")


def send_key(client, key):
    client.sendall(("sendkey %s %d\n" % (key, KEY_HOLD_MS)).encode("ascii"))


def key_echo_count(output, char):
    pattern = r"SYS_GETS: caractère ajouté:\s*'%s'" % re.escape(char)
    return len(re.findall(pattern, output))


def command_echoed(output, command):
    expected = " ".join(command.split())
    for received in re.findall(r"SYS_GETS: ligne lue: ([^\r\n]+)", output):
        if " ".join(received.split()) == expected:
            return True
    return False


def send_command_once(client, command, proc):
    """Injecte une commande après confirmation de chaque écho du shell.

    Le contrat NE2000 n’envoie qu’une requête d’observation. Les doublons de
    scan-codes sont retirés avant ``ret`` : la ligne altérée n’est donc jamais
    exécutée comme une commande réseau différente.
    """
    special = {" ": "spc", ".": "dot", "-": "minus", "_": "shift-minus"}
    for char in command:
        count = 0
        for _ in range(KEY_CHAR_RETRIES):
            start = len(text())
            send_key(client, special.get(char, char.lower()))
            deadline = time.time() + KEY_ECHO_TIMEOUT
            while time.time() < deadline:
                if proc.poll() is not None:
                    raise RuntimeError("QEMU stopped early")
                time.sleep(KEY_DELAY)
                count = key_echo_count(text()[start:], char)
                if count:
                    # Ce petit contrat reste court : stabiliser chaque
                    # scan-code rend aussi les doublons non spécifiques sûrs.
                    time.sleep(KEY_DUPLICATE_SETTLE_DELAY)
                    count = key_echo_count(text()[start:], char)
                    break
            if count:
                break
        if count == 0:
            raise RuntimeError("keyboard character missing: " + char)
        for _ in range(count - 1):
            send_key(client, "backspace")
            time.sleep(KEY_DELAY)
    send_key(client, "ret")


def send_status_command(client, proc):
    """Exécute exactement ``net-status json`` après validation de son écho.

    Un second essai est permis exclusivement lorsque le journal prouve que la
    ligne altérée a été refusée comme commande inconnue ; aucune mutation n’est
    associée à cette commande d’observation.
    """
    command = "net-status json"
    for attempt in range(COMMAND_RETRIES):
        start = len(text())
        send_command_once(client, command, proc)
        deadline = time.time() + 15
        while time.time() < deadline:
            if proc.poll() is not None:
                raise RuntimeError("QEMU stopped while querying net-status")
            output = text()[start:]
            if command_echoed(output, command):
                return start
            if "SYS_GETS: ligne lue:" in output:
                rejected = "Commande non trouvée" in output or "Commande non trouvee" in output
                if rejected:
                    break
                raise RuntimeError("net-status command echo altered")
            time.sleep(0.1)
        if attempt + 1 < COMMAND_RETRIES:
            time.sleep(0.2)
    raise RuntimeError("net-status command echo unstable")


def main():
    os.makedirs(LOG_DIR, exist_ok=True)
    for path in (LOG, ERR, MON):
        try:
            os.remove(path)
        except OSError:
            pass
    cmd = [
        "qemu-system-i386", "-kernel", os.path.join(ROOT, "build", "mohhdy.bin"),
        "-initrd", os.path.join(ROOT, "my_initrd.tar"), "-m", "1024M", "-display", "none",
        "-vga", "none", "-serial", "file:" + LOG,
        "-monitor", "unix:%s,server,nowait" % MON, "-machine", "type=pc,accel=tcg",
        "-netdev", "user,id=n0", "-device", "ne2k_isa,netdev=n0", "-no-reboot", "-no-shutdown",
    ]
    with open(ERR, "wb") as err:
        proc = subprocess.Popen(cmd, stdout=err, stderr=err)
        client = None
        try:
            wait("(-.-)", proc)
            client = monitor()
            time.sleep(0.5)
            before = send_status_command(client, proc)
            end = time.time() + 15
            while time.time() < end:
                if '"nic":"detected"' in text()[before:]:
                    send_command_once(client, "ai-runtime", proc)
                    wait("Session LLM noyau  : IDLE (NE2000 pret)", proc)
                    wait("Bail DHCP noyau    : absent", proc)
                    print("QEMU NE2000 status smoke passed.")
                    return 0
                if proc.poll() is not None:
                    raise RuntimeError("QEMU stopped while querying net-status")
                time.sleep(0.1)
            raise RuntimeError("net-status did not report nic=detected; log tail: " + text()[-500:])
        finally:
            if client:
                client.close()
            if proc.poll() is None:
                proc.terminate()
                proc.wait(timeout=3)


if __name__ == "__main__":
    try:
        raise SystemExit(main())
    except Exception as error:
        print("NE2000 status smoke failed: " + str(error))
        raise SystemExit(1)
