#!/usr/bin/env python3
"""Guest↔guest : ACK final + ClientHello TLS 1.2 (pas de proxy hub).

Suite apres guest-tls-peer. Deux QEMU TCG partagent
SharedEthernetHub(proxy_peer_syn_ack=False). B ecoute, emet SYN-ACK guest ;
A complete le handshake TCP (ACK final) et emet un ClientHello TLS 1.2 reel
vers B via ai-tls-poll. B passe ESTABLISHED. Pas encore de role serveur TLS
sur B ni d echange applicatif chiffre bilaterale. Hors make ci. Sans TAP,
Internet, secret ni OpenAI.
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
        "log": os.path.join(LOG_DIR, "ne2k-guest-tls-chat-a.log"),
        "err": os.path.join(LOG_DIR, "ne2k-guest-tls-chat-a.err"),
        "mon": os.path.join(LOG_DIR, "ne2k-guest-tls-chat-a.monitor.sock"),
    },
    {
        "label": "b",
        "mac": "52:54:00:a0:20:0b",
        "log": os.path.join(LOG_DIR, "ne2k-guest-tls-chat-b.log"),
        "err": os.path.join(LOG_DIR, "ne2k-guest-tls-chat-b.err"),
        "mon": os.path.join(LOG_DIR, "ne2k-guest-tls-chat-b.monitor.sock"),
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
    raise RuntimeError("monitor unavailable: %s" % path)


def send_key(client, key):
    client.sendall(("sendkey %s %d\n" % (key, KEY_HOLD_MS)).encode("ascii"))


def key_echo_count(output, char):
    pattern = r"SYS_GETS: caractère ajouté:\s*'%s'" % re.escape(char)
    return len(re.findall(pattern, normalized_log(output)))


def command_echoed(output, command):
    expected = " ".join(command.split())
    for received in re.findall(r"SYS_GETS: ligne lue: ([^\r\n]+)", normalized_log(output)):
        if " ".join(received.split()) == expected:
            return True
    return False


def send_command_once(client, command, proc, path):
    aliases = {" ": "spc", ".": "dot", "-": "minus", "/": "slash"}
    for char in command:
        count = 0
        for _ in range(KEY_CHAR_RETRIES):
            start = len(log_text(path))
            send_key(client, aliases.get(char, char.lower()))
            deadline = time.monotonic() + KEY_ECHO_TIMEOUT
            while time.monotonic() < deadline:
                if proc.poll() is not None:
                    raise RuntimeError("QEMU stopped during keyboard input")
                time.sleep(KEY_DELAY)
                count = key_echo_count(log_text(path)[start:], char)
                if count:
                    if char in ".s":
                        time.sleep(KEY_DUPLICATE_SETTLE_DELAY)
                        count = key_echo_count(log_text(path)[start:], char)
                    break
            if count:
                break
        if count == 0:
            raise RuntimeError("missing keyboard echo for %r on %s" % (char, path))
        for _ in range(count - 1):
            send_key(client, "backspace")
            time.sleep(KEY_DELAY)
    send_key(client, "ret")


def run_command(inst, command, needle, peer_alive_check, timeout=60):
    peer_alive_check()
    with INJECT_LOCK:
        peer_alive_check()
        start = len(log_text(inst["log"]))
        send_command_once(inst["client"], command, inst["proc"], inst["log"])
        deadline = time.monotonic() + 20
        while time.monotonic() < deadline:
            if inst["proc"].poll() is not None:
                raise RuntimeError("guest %s died after send" % inst["label"])
            output = normalized_log(log_text(inst["log"])[start:])
            if command_echoed(output, command):
                break
            if "SYS_GETS: ligne lue:" in output:
                raise RuntimeError(
                    "altered command on %s: %s" % (inst["label"], output[-600:])
                )
            time.sleep(0.1)
        else:
            raise RuntimeError("command echo absent on %s: %s" % (inst["label"], command))
        wait_for(inst["proc"], inst["log"], needle, timeout, start)
        wait_for(inst["proc"], inst["log"], "(-.-)", 20, start)
        return start


def terminate(proc):
    if proc is None:
        return
    if proc.poll() is None:
        proc.terminate()
        try:
            proc.wait(timeout=3)
        except subprocess.TimeoutExpired:
            proc.kill()


def start_guest(inst, hub_port):
    for path in (inst["log"], inst["err"], inst["mon"]):
        try:
            os.remove(path)
        except OSError:
            pass
    cmd = [
        "qemu-system-i386", "-cpu", "max",
        "-kernel", KERNEL, "-initrd", INITRD,
        "-m", "1024M", "-display", "none", "-vga", "none",
        "-serial", "file:" + inst["log"],
        "-monitor", "unix:%s,server,nowait" % inst["mon"],
        "-machine", "type=pc,accel=tcg",
        "-rtc", "base=2026-08-18T00:00:00",
        "-netdev", "socket,id=n0,connect=127.0.0.1:%d" % hub_port,
        "-device", "ne2k_isa,netdev=n0,mac=%s" % inst["mac"],
        "-no-reboot", "-no-shutdown",
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

    hub = SharedEthernetHub(respond=True, full_tls=False, proxy_peer_syn_ack=False)
    hub.start()
    say("[guest-tls-chat] hub listening on 127.0.0.1:%d" % hub.port)

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

        for inst in INSTANCES:
            wait_for(inst["proc"], inst["log"], "(-.-)", BOOT_TIMEOUT)
            inst["client"] = monitor_connect(inst["mon"])
            say("[guest-tls-chat] guest %s booted" % inst["label"])

        def peer_alive_check():
            for peer in INSTANCES:
                if peer["proc"].poll() is not None:
                    raise RuntimeError(
                        "peer guest %s died; both must stay alive" % peer["label"]
                    )
            if hub.client_count < 2:
                raise RuntimeError("hub lost a client (count=%d)" % hub.client_count)
            if hub.error is not None:
                raise RuntimeError("hub error: %s" % hub.error)

        peer_alive_check()

        guest_a = INSTANCES[0]
        guest_b = INSTANCES[1]

        for inst in INSTANCES:
            run_command(
                inst, "net-status json", "detected", peer_alive_check, timeout=30
            )

        # B first: claim a lease so peer.local can resolve to B.
        run_command(
            guest_b, "ai-runtime", "Entropie TLS RDRAND : disponible",
            peer_alive_check, timeout=30,
        )
        try:
            run_command(
                guest_b, "ai-acquire example.com",
                "ai-acquire: DHCP, DNS et SYN LLM demarres",
                peer_alive_check, timeout=90,
            )
        except RuntimeError as error:
            raise RuntimeError(
                "%s; hub events=%r leases=%r" % (error, hub.events, hub.leases)
            )

        deadline = time.monotonic() + 10
        while time.monotonic() < deadline and len(hub.leases) < 1:
            time.sleep(0.1)
        if len(hub.leases) < 1:
            raise RuntimeError("expected lease for guest B; events=%r" % hub.events)
        say("[guest-tls-chat] guest B leased; leases=%r" % (
            {k: ".".join(str(b) for b in v) for k, v in hub.leases.items()},
        ))

        # B listens in guest before A sends the peer SYN.
        run_command(
            guest_b, "ai-peer-listen", "ai-peer-listen: LISTEN",
            peer_alive_check, timeout=30,
        )

        # A targets peer.local → DNS peer IP, ARP croise, SYN vers B.
        run_command(
            guest_a, "ai-runtime", "Entropie TLS RDRAND : disponible",
            peer_alive_check, timeout=30,
        )
        try:
            run_command(
                guest_a, "ai-acquire peer.local",
                "ai-acquire: DHCP, DNS et SYN LLM demarres",
                peer_alive_check, timeout=90,
            )
        except RuntimeError as error:
            raise RuntimeError(
                "%s; hub events=%r leases=%r" % (error, hub.events, hub.leases)
            )

        # B accepts SYN and emits guest SYN-ACK (hub does not proxy).
        try:
            run_command(
                guest_b, "ai-peer-accept 192",
                "ai-peer-accept: SYN-ACK guest emis",
                peer_alive_check, timeout=120,
            )
        except RuntimeError as error:
            raise RuntimeError(
                "%s; hub events=%r leases=%r" % (error, hub.events, hub.leases)
            )

        deadline = time.monotonic() + 10
        while time.monotonic() < deadline and hub.events["guest_syn_ack"] < 1:
            time.sleep(0.1)
        if hub.events["guest_syn_ack"] < 1:
            raise RuntimeError(
                "expected guest-originated SYN-ACK; events=%r" % hub.events
            )

        # A: ACK final + ClientHello TLS 1.2 reel (crypto invite, pas hub).
        started = False
        for _ in range(24):
            peer_alive_check()
            start = run_command(
                guest_a, "ai-tls-poll", "ai-tls-poll:",
                peer_alive_check, timeout=45,
            )
            chunk = log_text(guest_a["log"])[start:]
            if "ai-tls-poll: echec TLS" in chunk:
                raise RuntimeError(
                    "TLS start failed on A; events=%r log=%s"
                    % (hub.events, chunk[-1200:])
                )
            if hub.events["guest_client_hello"] >= 1:
                started = True
                break
            if hub.events["guest_final_ack"] >= 1 and "progression TLS publiee" in chunk:
                # ACK landed; wait a beat for CH counter if piggybacked.
                time.sleep(0.2)
                if hub.events["guest_client_hello"] >= 1:
                    started = True
                    break
        if not started:
            start = run_command(
                guest_a, "ai-runtime", "Session LLM noyau",
                peer_alive_check, timeout=30,
            )
            runtime = log_text(guest_a["log"])[start:]
            if "TLS_STARTED" not in runtime or hub.events["guest_client_hello"] < 1:
                raise RuntimeError(
                    "expected guest ClientHello / TLS_STARTED; events=%r runtime=%s"
                    % (hub.events, runtime[-800:])
                )

        # B: consomme ACK final → ESTABLISHED (pas de role TLS serveur ici).
        try:
            run_command(
                guest_b, "ai-peer-accept 256 established",
                "ai-peer-accept: ESTABLISHED",
                peer_alive_check, timeout=120,
            )
        except RuntimeError as error:
            raise RuntimeError(
                "%s; hub events=%r leases=%r" % (error, hub.events, hub.leases)
            )

        deadline = time.monotonic() + 10
        while time.monotonic() < deadline and (
            hub.events["cross_arp"] < 1
            or hub.events["cross_syn"] < 1
            or hub.events["peer_dns"] < 1
            or hub.events["guest_syn_ack"] < 1
            or hub.events["guest_final_ack"] < 1
            or hub.events["guest_client_hello"] < 1
        ):
            time.sleep(0.1)

        if hub.events["peer_dns"] < 1:
            raise RuntimeError(
                "expected peer.local DNS; events=%r" % hub.events
            )
        if hub.events["cross_arp"] < 1 or hub.events["cross_arp_reply"] < 1:
            raise RuntimeError(
                "expected cross-guest ARP; events=%r leases=%r"
                % (hub.events, hub.leases)
            )
        if hub.events["cross_syn"] < 1:
            raise RuntimeError(
                "expected cross-guest SYN; events=%r" % hub.events
            )
        if hub.events["guest_syn_ack"] < 1:
            raise RuntimeError(
                "expected guest-originated SYN-ACK; events=%r" % hub.events
            )
        if hub.events["guest_final_ack"] < 1:
            raise RuntimeError(
                "expected guest final ACK; events=%r" % hub.events
            )
        if hub.events["guest_client_hello"] < 1:
            raise RuntimeError(
                "expected guest TLS ClientHello; events=%r" % hub.events
            )
        if hub.events["cross_syn_ack"] != 0:
            raise RuntimeError(
                "hub must not proxy peer SYN-ACK; events=%r" % hub.events
            )
        if len(hub.leases) < 2:
            raise RuntimeError("expected 2 DHCP leases; got %r" % hub.leases)

        peer_alive_check()
        run_command(
            guest_a, "ai-runtime", "TLS_STARTED", peer_alive_check, timeout=30
        )
        run_command(
            guest_b, "net-status json", "detected", peer_alive_check, timeout=30
        )
        peer_alive_check()

        say(
            "QEMU NE2000 guest-guest TLS-chat step passed "
            "(ESTABLISHED mutual + guest ClientHello TLS 1.2, hub proxy off, "
            "no TLS server role on B yet; events=%s leases=%s)."
            % (
                hub.events,
                {k: ".".join(str(b) for b in v) for k, v in hub.leases.items()},
            )
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
        sys.stderr.write("QEMU NE2000 guest-guest TLS chat step failed: %s\n" % error)
        raise SystemExit(1)
