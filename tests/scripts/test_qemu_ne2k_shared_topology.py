#!/usr/bin/env python3
"""Tranche 2 : topologie Ethernet locale partagée (deux QEMU simultanés).

Prérequis Garde 2 (`make qemu-ps2-dual`) : injection PS/2 mutexée hôte.
Ce contrat branche deux invités NE2000 sur un hub socket 127.0.0.1 (pas de TAP,
pas d'Internet, pas d'OpenAI). Les deux restent vivants ; chacun prouve
`nic=detected` ; l'invité A émet un DHCP Discover observé par le hub pendant
que B reste sur le même segment.
"""
from __future__ import print_function

import os
import re
import socket
import subprocess
import sys
import threading
import time

from qemu_shared_ethernet_hub import SharedEthernetHub

ROOT = os.path.abspath(os.path.join(os.path.dirname(__file__), "..", ".."))
KERNEL = os.environ.get("KERNEL", os.path.join(ROOT, "build", "mohhdy.bin"))
INITRD = os.environ.get("INITRD", os.path.join(ROOT, "my_initrd.tar"))
LOG_DIR = os.path.join(ROOT, "test_logs")
BOOT_TIMEOUT = float(os.environ.get("BOOT_TIMEOUT", "90"))
CMD_TIMEOUT = float(os.environ.get("CMD_TIMEOUT", "45"))
KEY_DELAY = float(os.environ.get("KEY_DELAY", "0.05"))
KEY_HOLD_MS = int(os.environ.get("KEY_HOLD_MS", "10"))
KEY_ECHO_TIMEOUT = float(os.environ.get("KEY_ECHO_TIMEOUT", "3"))
KEY_DUPLICATE_SETTLE_DELAY = float(os.environ.get("KEY_DUPLICATE_SETTLE_DELAY", "0.25"))
KEY_CHAR_RETRIES = int(os.environ.get("KEY_CHAR_RETRIES", "3"))

INJECT_LOCK = threading.Lock()

INSTANCES = (
    {
        "label": "a",
        "mac": "52:54:00:a0:20:0a",
        "log": os.path.join(LOG_DIR, "ne2k-shared-a.log"),
        "err": os.path.join(LOG_DIR, "ne2k-shared-a.err"),
        "mon": os.path.join(LOG_DIR, "ne2k-shared-a.monitor.sock"),
    },
    {
        "label": "b",
        "mac": "52:54:00:a0:20:0b",
        "log": os.path.join(LOG_DIR, "ne2k-shared-b.log"),
        "err": os.path.join(LOG_DIR, "ne2k-shared-b.err"),
        "mon": os.path.join(LOG_DIR, "ne2k-shared-b.monitor.sock"),
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


def run_command(inst, command, needle, peer_alive_check, timeout=None):
    path = inst["log"]
    proc = inst["proc"]
    client = inst["client"]
    if timeout is None:
        timeout = CMD_TIMEOUT
    with INJECT_LOCK:
        peer_alive_check()
        start = len(log_text(path))
        send_command_once(client, command, proc, path)
    deadline = time.monotonic() + timeout
    while time.monotonic() < deadline:
        peer_alive_check()
        if proc.poll() is not None:
            raise RuntimeError("QEMU %s exited during %r" % (inst["label"], command))
        output = normalized_log(log_text(path)[start:])
        if needle in output:
            say(
                "[shared-eth] guest %s: %s -> %s"
                % (inst["label"], command, needle[:48])
            )
            return start
        if command_echoed(output, command) and "Commande inconnue" in output:
            raise RuntimeError(
                "guest %s: unknown command after echo of %r" % (inst["label"], command)
            )
        time.sleep(0.1)
    raise RuntimeError(
        "guest %s: timeout %r / %r; tail:\n%s"
        % (inst["label"], command, needle, log_text(path)[-1500:])
    )


def terminate(proc):
    if proc is None or proc.poll() is not None:
        return
    proc.terminate()
    try:
        proc.wait(timeout=4)
    except subprocess.TimeoutExpired:
        proc.kill()
        proc.wait(timeout=4)


def start_guest(inst, hub_port):
    for path in (inst["log"], inst["err"], inst["mon"]):
        try:
            os.remove(path)
        except OSError:
            pass
    cmd = [
        "qemu-system-i386",
        "-cpu",
        "max",
        "-kernel",
        KERNEL,
        "-initrd",
        INITRD,
        "-m",
        "1024M",
        "-display",
        "none",
        "-vga",
        "none",
        "-serial",
        "file:" + inst["log"],
        "-monitor",
        "unix:%s,server,nowait" % inst["mon"],
        "-machine",
        "type=pc,accel=tcg",
        "-netdev",
        "socket,id=n0,connect=127.0.0.1:%d" % hub_port,
        "-device",
        "ne2k_isa,netdev=n0,mac=%s" % inst["mac"],
        "-no-reboot",
        "-no-shutdown",
    ]
    err = open(inst["err"], "wb")
    proc = subprocess.Popen(cmd, stdout=err, stderr=err)
    inst["err_handle"] = err
    inst["proc"] = proc
    return proc


def assert_nic_detected(inst, start):
    body = normalized_log(log_text(inst["log"])[start:])
    if '"nic":"detected"' not in body and "nic=detected" not in body:
        # net-status json form preferred; accept text form as fallback
        if '"nic": "detected"' not in body:
            raise RuntimeError(
                "guest %s missing nic detected after net-status; tail:\n%s"
                % (inst["label"], body[-1200:])
            )


def main():
    if not os.path.isfile(KERNEL) or not os.path.isfile(INITRD):
        raise RuntimeError("missing KERNEL or INITRD (build first)")
    os.makedirs(LOG_DIR, exist_ok=True)

    hub = SharedEthernetHub(respond=True)
    hub.start()
    say("[shared-eth] hub listening on 127.0.0.1:%d" % hub.port)

    try:
        for inst in INSTANCES:
            start_guest(inst, hub.port)

        deadline = time.monotonic() + 30
        while time.monotonic() < deadline and hub.client_count < 2:
            time.sleep(0.1)
        if hub.client_count < 2:
            raise RuntimeError(
                "hub expected 2 QEMU socket clients, got %d" % hub.client_count
            )
        say("[shared-eth] hub clients=%d" % hub.client_count)

        for inst in INSTANCES:
            wait_for(inst["proc"], inst["log"], "(-.-)", BOOT_TIMEOUT)
            inst["client"] = monitor_connect(inst["mon"])
            say("[shared-eth] guest %s booted (mac=%s)" % (inst["label"], inst["mac"]))

        def peer_alive_check():
            for peer in INSTANCES:
                if peer["proc"].poll() is not None:
                    raise RuntimeError(
                        "peer guest %s died; shared topology requires both alive"
                        % peer["label"]
                    )
            if hub.client_count < 2:
                raise RuntimeError(
                    "hub lost a client (count=%d)" % hub.client_count
                )

        peer_alive_check()

        # Both guests: net-status json while both alive on the shared segment.
        for inst in INSTANCES:
            start = run_command(
                inst, "net-status json", '"nic"', peer_alive_check, timeout=30
            )
            # Wait a moment for full JSON line
            wait_for(inst["proc"], inst["log"], "detected", 15, start)
            assert_nic_detected(inst, start)

        peer_alive_check()

        # Guest A: ensure RDRAND path then emit DHCP on the shared hub.
        guest_a = INSTANCES[0]
        run_command(
            guest_a,
            "ai-runtime",
            "Entropie TLS RDRAND : disponible",
            peer_alive_check,
            timeout=30,
        )
        run_command(
            guest_a,
            "ai-acquire example.com",
            "ai-acquire: DHCP, DNS et SYN LLM demarres",
            peer_alive_check,
            timeout=90,
        )

        deadline = time.monotonic() + 10
        while time.monotonic() < deadline and hub.events["discover"] == 0:
            time.sleep(0.1)
        if hub.events["discover"] == 0:
            raise RuntimeError(
                "hub saw no DHCP discover from shared segment; events=%r macs=%r"
                % (hub.events, sorted(hub.source_macs))
            )
        if hub.events["offer"] == 0:
            raise RuntimeError("hub did not emit DHCP offer; events=%r" % hub.events)

        # Guest A's MAC must appear; B may be silent until it TX's.
        mac_a = INSTANCES[0]["mac"].lower()
        if mac_a not in hub.source_macs:
            raise RuntimeError(
                "hub missing guest A MAC %s; seen=%r" % (mac_a, sorted(hub.source_macs))
            )

        peer_alive_check()
        # Guest B still answers net-status on the same segment after A's traffic.
        guest_b = INSTANCES[1]
        start_b = run_command(
            guest_b, "net-status json", '"nic"', peer_alive_check, timeout=30
        )
        wait_for(guest_b["proc"], guest_b["log"], "detected", 15, start_b)
        assert_nic_detected(guest_b, start_b)
        peer_alive_check()

        if hub.error is not None:
            raise RuntimeError("hub error: %s" % hub.error)

        say(
            "QEMU NE2000 shared Ethernet topology passed "
            "(2 simultaneous guests on 127.0.0.1 hub, mutexed PS/2, "
            "nic detected x2, DHCP discover from A, B still alive; "
            "events=%s macs=%s)."
            % (hub.events, sorted(hub.source_macs))
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
        hub.close()


if __name__ == "__main__":
    try:
        raise SystemExit(main())
    except Exception as error:
        sys.stderr.write("QEMU NE2000 shared Ethernet topology failed: %s\n" % error)
        raise SystemExit(1)
