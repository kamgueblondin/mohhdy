#!/usr/bin/env python3
"""Gate Garde 2 : deux QEMU TCG simultanés + injection PS/2 fiable.

Préalable de la topologie locale partagée (tranche 2) : tant que deux invités
TCG n'acceptent pas une injection clavier confirmée par écho pendant qu'ils
tournent en parallèle, le multi-pairs reste séquentiel.

Politique d'injection : un verrou hôte sérialise les sendkey entre les deux
moniteurs. Les deux processus QEMU restent vivants pendant toute la preuve.
Cela démontre le prérequis « plusieurs QEMU simultanés + PS/2 fiable » sans
prétendre que des sendkey strictement chevauchés sont sûrs sous TCG.

Mode optionnel PS2_DUAL_PARALLEL=1 : tente des frappes chevauchées (diagnostic).
"""
from __future__ import print_function

import os
import re
import socket
import subprocess
import sys
import threading
import time


ROOT = os.path.abspath(os.path.join(os.path.dirname(__file__), "..", ".."))
KERNEL = os.environ.get("KERNEL", os.path.join(ROOT, "build", "mohhdy.bin"))
INITRD = os.environ.get("INITRD", os.path.join(ROOT, "my_initrd.tar"))
LOG_DIR = os.path.join(ROOT, "test_logs")
BOOT_TIMEOUT = float(os.environ.get("BOOT_TIMEOUT", "90"))
CMD_TIMEOUT = float(os.environ.get("CMD_TIMEOUT", "25"))
KEY_DELAY = float(os.environ.get("KEY_DELAY", "0.05"))
KEY_HOLD_MS = int(os.environ.get("KEY_HOLD_MS", "10"))
KEY_ECHO_TIMEOUT = float(os.environ.get("KEY_ECHO_TIMEOUT", "3"))
KEY_DUPLICATE_SETTLE_DELAY = float(os.environ.get("KEY_DUPLICATE_SETTLE_DELAY", "0.25"))
KEY_CHAR_RETRIES = int(os.environ.get("KEY_CHAR_RETRIES", "3"))
PARALLEL = os.environ.get("PS2_DUAL_PARALLEL", "0") == "1"

# Verrou hôte : une seule série sendkey à la fois (défaut Garde 2).
INJECT_LOCK = threading.Lock()


INSTANCES = (
    {
        "label": "a",
        "log": os.path.join(LOG_DIR, "ps2-dual-a.log"),
        "err": os.path.join(LOG_DIR, "ps2-dual-a.err"),
        "mon": os.path.join(LOG_DIR, "ps2-dual-a.monitor.sock"),
        "command": "whoami",
        "marker": "whoami ok",
    },
    {
        "label": "b",
        "log": os.path.join(LOG_DIR, "ps2-dual-b.log"),
        "err": os.path.join(LOG_DIR, "ps2-dual-b.err"),
        "mon": os.path.join(LOG_DIR, "ps2-dual-b.monitor.sock"),
        "command": "getpid",
        "marker": "getpid ok",
    },
)


def say(message):
    sys.stdout.write(message + "\n")
    sys.stdout.flush()


def log_text(path):
    try:
        with open(path, "r", errors="replace") as handle:
            return handle.read()
    except OSError:
        return ""


def normalized_log(output):
    output = re.sub(r"(?<=\w)TIMER_ALIVE: tick=\d+\+?\r?\n(?=\w)", "", output)
    output = re.sub(r"TIMER_ALIVE: tick=\d+\+?\r?\n(?=\w)", "", output)
    output = re.sub(r"TIMER_ALIVE: tick=\d+\+?\r?\n(?=\s+\w)", "", output)
    output = re.sub(r"TIMER_ALIVE: tick=\d+\+?", "", output)
    output = re.sub(r"\[SCHED\] switching to task \d+\r?\n?", "", output)
    return output


def wait_for(proc, path, needle, timeout, start=0):
    deadline = time.monotonic() + timeout
    while time.monotonic() < deadline:
        if proc.poll() is not None:
            raise RuntimeError(
                "QEMU stopped early (%s); tail:\n%s" % (path, log_text(path)[-2000:])
            )
        if needle in normalized_log(log_text(path)[start:]):
            time.sleep(0.35)
            return
        time.sleep(0.1)
    raise RuntimeError(
        "timeout waiting for %r in %s; tail:\n%s" % (needle, path, log_text(path)[-2000:])
    )


def monitor_connect(path):
    deadline = time.monotonic() + 10
    while time.monotonic() < deadline:
        if os.path.exists(path):
            client = socket.socket(socket.AF_UNIX, socket.SOCK_STREAM)
            try:
                client.connect(path)
                client.settimeout(0.1)
                try:
                    client.recv(4096)
                except socket.timeout:
                    pass
                return client
            except OSError:
                client.close()
        time.sleep(0.1)
    raise RuntimeError("monitor unavailable: " + path)


def send_key(client, key):
    client.sendall(("sendkey %s %d\n" % (key, KEY_HOLD_MS)).encode("ascii"))


def key_echo_count(output, char):
    pattern = r"SYS_GETS: caractère ajouté:\s*'%s'" % re.escape(char)
    return len(re.findall(pattern, normalized_log(output)))


def command_echoed(output, command):
    expected = " ".join(command.lower().split())
    for received in re.findall(r"SYS_GETS: ligne lue: ([^\r\n]+)", normalized_log(output)):
        if " ".join(received.lower().split()) == expected:
            return True
    return False


def send_command_once(client, command, proc, path):
    aliases = {" ": "spc", "-": "minus", ".": "dot", "/": "slash"}
    for char in command:
        count = 0
        for _ in range(KEY_CHAR_RETRIES):
            start = len(log_text(path))
            send_key(client, aliases.get(char, char.lower()))
            deadline = time.monotonic() + KEY_ECHO_TIMEOUT
            while time.monotonic() < deadline:
                if proc.poll() is not None:
                    raise RuntimeError("QEMU stopped during inject: " + path)
                time.sleep(KEY_DELAY)
                count = key_echo_count(log_text(path)[start:], char.lower())
                if count:
                    time.sleep(KEY_DUPLICATE_SETTLE_DELAY)
                    count = key_echo_count(log_text(path)[start:], char.lower())
                    break
            if count:
                break
        if count == 0:
            raise RuntimeError("[%s] character missing: %s" % (path, char))
        for _ in range(count - 1):
            send_key(client, "backspace")
            time.sleep(KEY_DUPLICATE_SETTLE_DELAY)
    send_key(client, "ret")


def drive_instance(inst, peer_alive_check, errors):
    label = inst["label"]
    path = inst["log"]
    proc = inst["proc"]
    client = inst["client"]
    command = inst["command"]
    marker = inst["marker"]
    try:
        peer_alive_check()
        start = len(log_text(path))
        if PARALLEL:
            send_command_once(client, command, proc, path)
        else:
            with INJECT_LOCK:
                peer_alive_check()
                send_command_once(client, command, proc, path)
        deadline = time.monotonic() + CMD_TIMEOUT
        while time.monotonic() < deadline:
            peer_alive_check()
            if proc.poll() is not None:
                raise RuntimeError("QEMU %s exited during command" % label)
            output = normalized_log(log_text(path)[start:])
            if command_echoed(output, command):
                wait_for(proc, path, marker, CMD_TIMEOUT, start)
                wait_for(proc, path, "(-.-)", CMD_TIMEOUT, start)
                say("[ps2-dual] guest %s: %s -> %s" % (label, command, marker))
                return
            if "SYS_GETS: ligne lue: " in output:
                raise RuntimeError(
                    "guest %s: command echo altered for %r; got:\n%s"
                    % (label, command, output[-800:])
                )
            time.sleep(0.1)
        raise RuntimeError("guest %s: timeout waiting echo for %r" % (label, command))
    except Exception as exc:
        errors.append("%s: %s" % (label, exc))


def terminate(proc):
    if proc is None or proc.poll() is not None:
        return
    proc.terminate()
    try:
        proc.wait(timeout=4)
    except subprocess.TimeoutExpired:
        proc.kill()
        proc.wait(timeout=4)


def start_guest(inst):
    for path in (inst["log"], inst["err"], inst["mon"]):
        try:
            os.remove(path)
        except OSError:
            pass
    cmd = [
        "qemu-system-i386",
        "-cpu", "pentium3",
        "-kernel", KERNEL,
        "-initrd", INITRD,
        "-m", "1024M",
        "-display", "none",
        "-vga", "none",
        "-serial", "file:" + inst["log"],
        "-monitor", "unix:%s,server,nowait" % inst["mon"],
        "-machine", "type=pc,accel=tcg",
        "-no-reboot",
        "-no-shutdown",
    ]
    err = open(inst["err"], "wb")
    proc = subprocess.Popen(cmd, stdout=err, stderr=err)
    inst["err_handle"] = err
    inst["proc"] = proc
    return proc


def main():
    if not os.path.isfile(KERNEL) or not os.path.isfile(INITRD):
        raise RuntimeError("missing KERNEL or INITRD (build first)")
    os.makedirs(LOG_DIR, exist_ok=True)

    mode = "parallel-sendkey" if PARALLEL else "mutexed-sendkey"
    say("[ps2-dual] starting two TCG guests (%s)" % mode)

    for inst in INSTANCES:
        start_guest(inst)

    try:
        for inst in INSTANCES:
            wait_for(inst["proc"], inst["log"], "(-.-)", BOOT_TIMEOUT)
            inst["client"] = monitor_connect(inst["mon"])
            say("[ps2-dual] guest %s booted" % inst["label"])

        def peer_alive_check():
            for peer in INSTANCES:
                if peer["proc"].poll() is not None:
                    raise RuntimeError(
                        "peer guest %s died; dual gate requires both alive"
                        % peer["label"]
                    )

        # Both processes must still be alive before any injection.
        peer_alive_check()

        errors = []
        threads = []
        for inst in INSTANCES:
            thread = threading.Thread(
                target=drive_instance, args=(inst, peer_alive_check, errors)
            )
            threads.append(thread)
        for thread in threads:
            thread.start()
        for thread in threads:
            thread.join()

        if errors:
            raise RuntimeError("dual PS/2 inject failed: " + " | ".join(errors))

        # Cross-talk: each log must not show the peer's confirmed command line.
        log_a = normalized_log(log_text(INSTANCES[0]["log"]))
        log_b = normalized_log(log_text(INSTANCES[1]["log"]))
        if "SYS_GETS: ligne lue: getpid" in log_a:
            raise RuntimeError("guest a unexpectedly executed getpid")
        if "SYS_GETS: ligne lue: whoami" in log_b:
            raise RuntimeError("guest b unexpectedly executed whoami")
        if "whoami ok" not in log_a:
            raise RuntimeError("guest a missing whoami ok")
        if "getpid ok" not in log_b:
            raise RuntimeError("guest b missing getpid ok")

        # Peers still alive after both commands.
        peer_alive_check()
        say(
            "QEMU PS/2 dual inject gate passed "
            "(two simultaneous TCG guests, %s, no cross-talk)." % mode
        )
        return 0
    finally:
        for inst in INSTANCES:
            client = inst.get("client")
            if client is not None:
                try:
                    client.close()
                except OSError:
                    pass
            terminate(inst.get("proc"))
            handle = inst.get("err_handle")
            if handle is not None:
                try:
                    handle.close()
                except OSError:
                    pass


if __name__ == "__main__":
    try:
        raise SystemExit(main())
    except Exception as error:
        sys.stderr.write("QEMU PS/2 dual inject gate failed: %s\n" % error)
        raise SystemExit(1)
