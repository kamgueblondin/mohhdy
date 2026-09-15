#!/usr/bin/env python3
"""Contrat MOHHDY Foundation : transfert borné d'un nom de service."""
import os
import re
import socket
import subprocess
import sys
import time

ROOT = os.path.abspath(os.path.join(os.path.dirname(__file__), "..", ".."))
LOG_DIR = os.path.join(ROOT, "test_logs")
LOG = os.path.join(LOG_DIR, "service-grant.log")
ERR = os.path.join(LOG_DIR, "service-grant.err")
MON = os.path.join(LOG_DIR, "service-grant-monitor.sock")
KERNEL = os.path.join(ROOT, "build", "mohhdy.bin")
INITRD = os.path.join(ROOT, "my_initrd.tar")
KEY_DELAY = float(os.environ.get("KEY_DELAY", "0.24"))
KEY_RETRIES = int(os.environ.get("KEY_RETRIES", "3"))


def log_text():
    try:
        with open(LOG, "r", errors="replace") as handle:
            return handle.read()
    except OSError:
        return ""


def normalized_log(output):
    """Retire uniquement les diagnostics asynchrones hors contrat métier."""
    output = re.sub(r"(?<=\w)TIMER_ALIVE: tick=\d+\+?\r?\n(?=\w)", "", output)
    output = re.sub(r"TIMER_ALIVE: tick=\d+\+?\r?\n(?=\w)", "", output)
    output = re.sub(r"TIMER_ALIVE: tick=\d+\+?\r?\n(?=\s+\w)", "", output)
    output = re.sub(r"TIMER_ALIVE: tick=\d+\+?", "", output)
    scheduler = r"\[SCHED\] switching to task \d+\s*"
    output = re.sub(r"(?<=\w)" + scheduler + r"(?=\w)", " ", output)
    output = re.sub(scheduler, "", output)
    return re.sub(r"[ \t]{2,}", " ", output)


def wait_for(needle, proc, offset=0, timeout=15):
    deadline = time.time() + timeout
    while time.time() < deadline:
        if proc.poll() is not None:
            raise RuntimeError("QEMU s'est arrêté prématurément")
        if needle in normalized_log(log_text()[offset:]):
            return
        time.sleep(0.1)
    raise RuntimeError("sortie manquante : %s" % needle)


def wait_for_pattern(pattern, description, proc, offset=0, timeout=15):
    deadline = time.time() + timeout
    while time.time() < deadline:
        if proc.poll() is not None:
            raise RuntimeError("QEMU s'est arrêté prématurément")
        if re.search(pattern, normalized_log(log_text()[offset:])):
            return
        time.sleep(0.1)
    raise RuntimeError("sortie manquante : %s" % description)


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


def send_command_once(client, command):
    special = {" ": "spc", "-": "minus", ".": "dot"}
    for char in command:
        client.sendall(("sendkey %s\n" % special.get(char, char.lower())).encode("ascii"))
        time.sleep(KEY_DELAY)
    client.sendall(b"sendkey ret\n")


def command_echoed(output, command):
    """Retourne vrai si le shell a reçu la même commande après normalisation.

    Le parseur du shell accepte plusieurs espaces entre les arguments. Cette
    normalisation évite de réinjecter une commande déjà exécutée lorsque QEMU
    duplique uniquement un scancode d’espace.
    """
    expected = " ".join(command.split())
    for received in re.findall(r"SYS_GETS: ligne lue: ([^\r\n]+)", normalized_log(output)):
        if " ".join(received.split()) == expected:
            return True
    return False


def send_command(client, command, proc=None):
    """Injecte une commande et vérifie son écho sans rejouer un effet métier."""
    if proc is None:
        send_command_once(client, command)
        return
    for attempt in range(1, KEY_RETRIES + 1):
        start = len(log_text())
        send_command_once(client, command)
        deadline = time.time() + 15
        while time.time() < deadline:
            if proc.poll() is not None:
                raise RuntimeError("QEMU s'est arrêté prématurément")
            output = normalized_log(log_text()[start:])
            if command_echoed(output, command):
                return
            if "SYS_GETS: ligne lue: " in output:
                break
            time.sleep(0.1)
        if attempt < KEY_RETRIES:
            time.sleep(0.2)
    raise RuntimeError("echo commande instable : %s" % command)


def send_command_until(client, command, marker, proc, attempts=1):
    """Exécute une commande une seule fois, puis vérifie son résultat métier.

    Une répétition après un écho valide pourrait doubler un envoi IPC ou une
    mutation de registre ; les réessais sont donc réservés à la phase d’écho.
    """
    start = len(log_text())
    send_command(client, command, proc)
    wait_for(marker, proc, start)


def receive_service_event(client, proc, pattern, description):
    """Lit la FIFO jusqu’à l’événement de service attendu.

    La terminaison d’un enfant publie aussi un événement de tâche best-effort
    dans la même FIFO. Il est donc normal que cet événement précède la purge
    du service ; il ne doit pas être confondu avec une absence de purge.
    """
    failure = None
    for _ in range(KEY_RETRIES):
        start = len(log_text())
        try:
            send_command(client, "ipc-recv", proc)
            wait_for_pattern(pattern, description, proc, start)
            # L’événement est imprimé avant le retour au shell. Attendre le
            # prompt évite d’injecter la commande suivante pendant la fin de
            # traitement de l’IRQ série, sans réessayer d’effet métier.
            wait_for("(-.-)", proc, start)
            return
        except RuntimeError as error:
            failure = error
            time.sleep(0.2)
    raise failure


def main():
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
            time.sleep(0.5)
            before_publish = len(log_text())
            send_command(monitor, "service-publish demo", proc)
            wait_for("service-publish ok demo", proc, before_publish)
            before_status_empty = len(log_text())
            send_command(monitor, "service-status demo", proc)
            wait_for("service-status ok demo pid 1 queued 0 client-capacity 2 endpoint-capacity 4",
                     proc, before_status_empty)
            send_command_until(monitor, "ipc-send 1 one", "ipc-send ok 1 3", proc)
            send_command_until(monitor, "ipc-send 1 two", "ipc-send ok 1 3", proc)
            before_status_full = len(log_text())
            send_command(monitor, "service-status demo", proc)
            wait_for("service-status ok demo pid 1 queued 2 client-capacity 2 endpoint-capacity 4",
                     proc, before_status_full)
            send_command_until(monitor, "ipc-send 1 three",
                               "ipc-send: capacite du service atteinte", proc)
            before_capacity_drain_one = len(log_text())
            send_command(monitor, "ipc-recv", proc)
            wait_for("ipc-recv from 1 type 0 data one", proc, before_capacity_drain_one)
            before_capacity_drain_two = len(log_text())
            send_command(monitor, "ipc-recv", proc)
            wait_for("ipc-recv from 1 type 0 data two", proc, before_capacity_drain_two)
            before_status_drained = len(log_text())
            send_command(monitor, "service-status demo", proc)
            wait_for("service-status ok demo pid 1 queued 0 client-capacity 2 endpoint-capacity 4",
                     proc, before_status_drained)
            before_watch = len(log_text())
            send_command(monitor, "service-watch demo", proc)
            wait_for("service-watch ok demo", proc, before_watch)
            before_spawn = len(log_text())
            send_command(monitor, "spawn serviceclaim", proc)
            wait_for("spawn ok pid", proc, before_spawn)
            spawned = re.search(r"spawn ok pid (\d+) serviceclaim", normalized_log(log_text()[before_spawn:]))
            if not spawned:
                raise RuntimeError("client de revendication non lance")
            claimant_pid = spawned.group(1)
            send_command(monitor, "yield", proc)
            wait_for("serviceclaim waiting demo", proc, before_spawn)
            before_grant = len(log_text())
            send_command(monitor, "service-grant demo %s" % claimant_pid, proc)
            wait_for("service-grant ok demo %s" % claimant_pid, proc, before_grant)
            send_command(monitor, "yield", proc)
            wait_for("serviceclaim notified demo", proc, before_grant)
            wait_for("serviceclaim claimed demo", proc, before_grant)
            receive_service_event(
                monitor,
                proc,
                r"service-event demo old 1 new[\s\S]{0,160}?%s reason 2" % claimant_pid,
                "service-event demo old 1 new %s reason 2" % claimant_pid,
            )
            before_lookup = len(log_text())
            send_command(monitor, "service-find demo", proc)
            wait_for("service-find ok demo %s" % claimant_pid, proc, before_lookup)
            before_kill = len(log_text())
            send_command(monitor, "kill %s" % claimant_pid, proc)
            wait_for("Processus %s termine" % claimant_pid, proc, before_kill)
            receive_service_event(
                monitor,
                proc,
                r"service-event demo old %s new[\s\S]{0,160}?0 reason 4" % claimant_pid,
                "service-event demo old %s new 0 reason 4" % claimant_pid,
            )
            before_missing = len(log_text())
            send_command(monitor, "service-find demo", proc)
            wait_for("service-find: service indisponible", proc, before_missing)
            print("MOHHDY Foundation service grant contract passed")
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
        print("MOHHDY Foundation service grant contract failed: %s" % error, file=sys.stderr)
        raise SystemExit(1)
