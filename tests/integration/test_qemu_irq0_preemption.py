#!/usr/bin/env python3
"""Contrat AOS-024 : IRQ0 préempte une tâche Ring 3 non coopérative."""
import os
import re
import socket
import subprocess
import sys
import time

ROOT = os.path.abspath(os.path.join(os.path.dirname(__file__), "..", ".."))
LOG_DIR = os.path.join(ROOT, "test_logs")
LOG = os.path.join(LOG_DIR, "irq0-preemption.log")
ERR = os.path.join(LOG_DIR, "irq0-preemption.err")
MON = os.path.join(LOG_DIR, "irq0-preemption-monitor.sock")
KERNEL = os.path.join(ROOT, "build", "mohhdy.bin")
INITRD = os.path.join(ROOT, "my_initrd.tar")
KEY_DELAY = float(os.environ.get("KEY_DELAY", "0.05"))
KEY_HOLD_MS = int(os.environ.get("KEY_HOLD_MS", "10"))
KEY_ECHO_TIMEOUT = float(os.environ.get("KEY_ECHO_TIMEOUT", "3"))
KEY_LINE_SETTLE_DELAY = float(os.environ.get("KEY_LINE_SETTLE_DELAY", "0.50"))
KEY_CHAR_RETRIES = int(os.environ.get("KEY_CHAR_RETRIES", "3"))
KEY_PRE_RET_RETRIES = int(os.environ.get("KEY_PRE_RET_RETRIES", "3"))


class CommandEchoMismatch(RuntimeError):
    """La ligne reçue par le shell diffère de celle préparée."""


def log_text():
    try:
        with open(LOG, "r", errors="replace") as handle:
            return handle.read()
    except OSError:
        return ""


def normalized_log(output):
    """Retire seulement les diagnostics noyau asynchrones complets."""
    output = re.sub(r"TIMER_ALIVE: tick=\d+\+?\r?\n?", "", output)
    return re.sub(r"\[SCHED\] switching to task \d+\r?\n?", "", output)


def wait_for(needle, proc, offset=0, timeout=12):
    deadline = time.time() + timeout
    while time.time() < deadline:
        if proc.poll() is not None:
            raise RuntimeError("QEMU s'est arrêté prématurément")
        if needle in log_text()[offset:]:
            return
        time.sleep(0.1)
    raise RuntimeError("sortie manquante : %s" % needle)


def connect_monitor():
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
    raise RuntimeError("moniteur QEMU indisponible")


def send_key(client, key):
    client.sendall(("sendkey %s %d\n" % (key, KEY_HOLD_MS)).encode("ascii"))


def key_echoes(output):
    """Retourne la séquence exacte reçue dans le buffer de la ligne courante."""
    return re.findall(r"SYS_GETS: caractère ajouté:\s*'(.)'", normalized_log(output))


def prepared_line_matches(output, command):
    return key_echoes(output) == [char.lower() for char in command]


def command_echoed(output, command):
    expected = " ".join(command.lower().split())
    for received in re.findall(r"SYS_GETS: ligne lue: ([^\r\n]+)", normalized_log(output)):
        if " ".join(received.lower().split()) == expected:
            return True
    return False


def verify_pre_ret_reconciliation_parser():
    """Refuse les lignes partielles ou doublées avant qu’elles ne soient soumises."""
    command = "spawn spin"
    exact = "\n".join(
        "SYS_GETS: caractère ajouté: '%s'" % char for char in command
    )
    exact = exact.replace(
        "SYS_GETS: caractère ajouté: 'p'\nSYS_GETS: caractère ajouté: 'a'",
        "SYS_GETS: caractère ajouté: 'p'\nTIMER_ALIVE: tick=99+\n"
        "[SCHED] switching to task 3\nSYS_GETS: caractère ajouté: 'a'",
        1,
    )
    if not prepared_line_matches(exact, command):
        raise RuntimeError("sonde pre-ret IRQ0 : echos exacts refuses")
    if prepared_line_matches(exact + "\nSYS_GETS: caractère ajouté: 'n'", command):
        raise RuntimeError("sonde pre-ret IRQ0 : doublon accepte")
    if prepared_line_matches(exact[:-32], command):
        raise RuntimeError("sonde pre-ret IRQ0 : ligne partielle acceptee")


def send_command_once(client, command, proc, key_delay=KEY_DELAY):
    """Prépare une ligne complète sans jamais l’exécuter."""
    special = {" ": "spc", "-": "minus", ".": "dot", "/": "slash"}
    for char in command:
        received = False
        for _ in range(KEY_CHAR_RETRIES):
            start = len(log_text())
            send_key(client, special.get(char, char.lower()))
            deadline = time.time() + KEY_ECHO_TIMEOUT
            while time.time() < deadline:
                if proc.poll() is not None:
                    raise RuntimeError("QEMU s'est arrêté prématurément")
                time.sleep(key_delay)
                if char.lower() in key_echoes(log_text()[start:]):
                    received = True
                    break
            if received:
                break
        if not received:
            raise RuntimeError("caractère non reçu : %s" % char)


def clear_prepared_line(client, output, key_delay=KEY_DELAY):
    """Efface uniquement une ligne non soumise dont les échos divergent."""
    for _ in key_echoes(output):
        send_key(client, "backspace")
        time.sleep(key_delay)
    time.sleep(KEY_LINE_SETTLE_DELAY)


def send_command(client, command, proc, key_delay=KEY_DELAY):
    """Réconcilie la ligne avant `ret`, puis interdit tout rejeu post-exécution."""
    for _ in range(KEY_PRE_RET_RETRIES):
        start = len(log_text())
        send_command_once(client, command, proc, key_delay)
        time.sleep(KEY_LINE_SETTLE_DELAY)
        prepared = normalized_log(log_text()[start:])
        if prepared_line_matches(prepared, command):
            send_key(client, "ret")
            deadline = time.time() + 15
            while time.time() < deadline:
                if proc.poll() is not None:
                    raise RuntimeError("QEMU s'est arrêté prématurément")
                output = normalized_log(log_text()[start:])
                if command_echoed(output, command):
                    return
                if "SYS_GETS: ligne lue: " in output:
                    raise CommandEchoMismatch("echo commande altéré : %s" % command)
                time.sleep(0.1)
            raise RuntimeError("echo commande absent : %s" % command)
        clear_prepared_line(client, prepared, key_delay)
    raise CommandEchoMismatch("echo pre-ret instable : %s" % command)


def main():
    verify_pre_ret_reconciliation_parser()
    if os.environ.get("IRQ0_INPUT_PROBE_ONLY") == "1":
        return 0
    os.makedirs(LOG_DIR, exist_ok=True)
    for path in (LOG, ERR, MON):
        try:
            os.remove(path)
        except OSError:
            pass
    command = [
        "qemu-system-i386", "-kernel", KERNEL, "-initrd", INITRD,
        "-m", "1024M", "-display", "none", "-vga", "none",
        "-serial", "file:" + LOG,
        "-monitor", "unix:%s,server,nowait" % MON,
        "-machine", "type=pc,accel=tcg", "-no-reboot", "-no-shutdown",
    ]
    with open(ERR, "wb") as err_handle:
        proc = subprocess.Popen(command, stdout=err_handle, stderr=err_handle)
        monitor = None
        try:
            wait_for("(-.-)", proc)
            monitor = connect_monitor()
            before_spawn = len(log_text())
            send_command(monitor, "spawn spin", proc)
            wait_for("spawn ok pid", proc, before_spawn)
            before_echo = len(log_text())
            send_command(monitor, "echo irq0-preempt-ok", proc)
            wait_for("irq0-preempt-ok", proc, before_echo)
            print("AOS-024 IRQ0 preemption contract passed")
            return 0
        finally:
            if monitor is not None:
                monitor.close()
            if proc.poll() is None:
                proc.terminate()
                try:
                    proc.wait(timeout=2)
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
        print("AOS-024 IRQ0 preemption contract failed: %s" % error, file=sys.stderr)
        raise SystemExit(1)
