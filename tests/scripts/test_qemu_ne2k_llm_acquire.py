#!/usr/bin/env python3
"""Contrat QEMU du bootstrap LLM DHCP/DNS/ARP/SYN sur NE2000 et réseau user."""
import os
import socket
import subprocess
import time
from qemu_ne2k_controlled_peer import ControlledEthernetPeer

ROOT = os.path.abspath(os.path.join(os.path.dirname(__file__), "..", ".."))
LOG_DIR = os.path.join(ROOT, "test_logs")
LOG = os.path.join(LOG_DIR, "ne2k-llm-acquire.log")
ERR = os.path.join(LOG_DIR, "ne2k-llm-acquire.err")
MON = os.path.join(LOG_DIR, "ne2k-llm-acquire-monitor.sock")


def text():
    try:
        with open(LOG, errors="replace") as handle:
            return handle.read()
    except OSError:
        return ""


def wait_for(proc, needle, timeout=45, start=0):
    deadline = time.monotonic() + timeout
    while time.monotonic() < deadline:
        if proc.poll() is not None:
            raise RuntimeError("QEMU stopped: %s" % text()[-2000:])
        if needle in text()[start:]:
            return
        time.sleep(0.15)
    raise RuntimeError("missing output %r: %s" % (needle, text()[-2000:]))


def monitor():
    deadline = time.monotonic() + 10.0
    while time.monotonic() < deadline:
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


def keys(client, command):
    aliases = {" ": "spc", ".": "dot", "-": "minus"}
    for char in command:
        client.sendall(("sendkey %s\n" % aliases.get(char, char.lower())).encode("ascii"))
        time.sleep(0.30)
    client.sendall(b"sendkey ret\n")


def main():
    os.makedirs(LOG_DIR, exist_ok=True)
    for path in (LOG, ERR, MON):
        try:
            os.remove(path)
        except OSError:
            pass
    peer = ControlledEthernetPeer()
    peer.start()
    command = [
        "qemu-system-i386", "-kernel", os.path.join(ROOT, "build", "mohhdy.bin"),
        "-initrd", os.path.join(ROOT, "my_initrd.tar"), "-cpu", "max", "-m", "1024M",
        "-display", "none", "-vga", "none", "-serial", "file:" + LOG,
        "-monitor", "unix:%s,server,nowait" % MON, "-machine", "type=pc,accel=tcg",
        "-netdev", "socket,id=n0,connect=127.0.0.1:%d" % peer.port,
        "-device", "ne2k_isa,netdev=n0", "-no-reboot", "-no-shutdown",
    ]
    proc = None
    client = None
    try:
        with open(ERR, "wb") as err:
            proc = subprocess.Popen(command, cwd=ROOT, stdout=err, stderr=err)
            wait_for(proc, "(-.-)")
            client = monitor()
            start = len(text())
            keys(client, "ai-runtime")
            wait_for(proc, "Entropie TLS RDRAND : disponible (materiel)", 20, start)
            start = len(text())
            keys(client, "ai-acquire example.com")
            try:
                wait_for(proc, "ai-acquire: DHCP, DNS et SYN LLM demarres", 60, start)
            except RuntimeError as error:
                raise RuntimeError("%s; peer events=%r peer error=%r" %
                                   (error, peer.events, peer.error))
            start = len(text())
            keys(client, "ai-tls-poll")
            try:
                wait_for(proc, "ai-tls-poll: progression TLS publiee", 30, start)
            except RuntimeError as error:
                raise RuntimeError("%s; peer events=%r peer error=%r" %
                                   (error, peer.events, peer.error))
            start = len(text())
            keys(client, "ai-tls-poll")
            try:
                wait_for(proc, "ai-tls-poll: progression TLS publiee", 30, start)
            except RuntimeError as error:
                raise RuntimeError("%s; peer events=%r peer error=%r" %
                                   (error, peer.events, peer.error))
            start = len(text())
            keys(client, "ai-runtime")
            wait_for(proc, "Session LLM noyau  : TLS_STARTED (NE2000 pret)", 20, start)
            wait_for(proc, "Bail DHCP noyau    : present (routes disponibles)", 20, start)
            deadline = time.monotonic() + 5.0
            while time.monotonic() < deadline and not all(peer.events[name] for name in peer.events):
                time.sleep(0.05)
            if peer.error is not None:
                raise RuntimeError("controlled Ethernet peer failed: %s" % peer.error)
            missing = [name for name, count in peer.events.items() if count == 0]
            if missing:
                raise RuntimeError("missing controlled network events: %s" % ", ".join(missing))
            print("QEMU NE2000 LLM acquire contract passed.")
            return 0
    finally:
        peer.close()
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
        print("QEMU NE2000 LLM acquire contract failed: %s" % error)
        raise SystemExit(1)
