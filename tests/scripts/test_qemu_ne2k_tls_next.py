#!/usr/bin/env python3
"""Contrat QEMU : second tour LLM sur la meme session TLS locale via ai-next."""
import os
import socket
import subprocess
import time
from qemu_ne2k_controlled_peer import ControlledEthernetPeer

ROOT = os.path.abspath(os.path.join(os.path.dirname(__file__), "..", ".."))
LOG_DIR = os.path.join(ROOT, "test_logs")
LOG = os.path.join(LOG_DIR, "ne2k-tls-next.log")
ERR = os.path.join(LOG_DIR, "ne2k-tls-next.err")
MON = os.path.join(LOG_DIR, "ne2k-tls-next-monitor.sock")


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
    aliases = {" ": "spc", ".": "dot", "-": "minus", "/": "slash"}
    for char in command:
        client.sendall(("sendkey %s\n" % aliases.get(char, char.lower())).encode("ascii"))
        time.sleep(0.30)
    client.sendall(b"sendkey ret\n")


def keys_retry_any(client, proc, command, needles, timeout=45):
    start = len(text())
    keys(client, command)
    deadline = time.monotonic() + timeout
    while time.monotonic() < deadline:
        if proc.poll() is not None:
            raise RuntimeError("QEMU stopped: %s" % text()[-2000:])
        chunk = text()[start:]
        for needle in needles:
            if needle in chunk:
                return needle
        if "Commande non trouvée" in chunk or "Commande non trouvee" in chunk:
            start = len(text())
            keys(client, command)
        time.sleep(0.15)
    raise RuntimeError("missing output %r: %s" % (needles, text()[-2000:]))


def keys_retry(client, proc, command, success, timeout=45):
    found = keys_retry_any(client, proc, command, (success,), timeout)
    if found != success:
        raise RuntimeError("missing output %r: %s" % (success, text()[-2000:]))


def handshake_tls(client, proc, peer):
    keys_retry(client, proc, "ai-runtime", "Entropie TLS RDRAND : disponible (materiel)", 20)
    try:
        keys_retry(client, proc, "ai-acquire example.com",
                   "ai-acquire: DHCP, DNS et SYN LLM demarres", 60)
    except RuntimeError as error:
        raise RuntimeError("%s; peer events=%r peer error=%r" %
                           (error, peer.events, peer.error))
    complete = False
    progressions = 0
    for _ in range(32):
        start = len(text())
        keys(client, "ai-tls-poll")
        deadline = time.monotonic() + 30
        while time.monotonic() < deadline:
            if proc.poll() is not None:
                raise RuntimeError("QEMU stopped: %s" % text()[-2000:])
            chunk = text()[start:]
            if "ai-tls-poll: echec TLS" in chunk:
                raise RuntimeError("TLS failed; peer events=%r sizes=%r peer error=%r log=%s" %
                                   (peer.events, peer.sent_sizes, peer.error, chunk[-1500:]))
            if "progression TLS publiee" in chunk:
                progressions += 1
                break
            if "attente de trame TLS" in chunk:
                break
            time.sleep(0.15)
        else:
            raise RuntimeError("tls-poll mute; peer events=%r sizes=%r peer error=%r" %
                               (peer.events, peer.sent_sizes, peer.error))
        if progressions >= 7:
            start = len(text())
            keys(client, "ai-runtime")
            wait_for(proc, "Session LLM noyau", 20, start)
            if "TLS_COMPLETE" in text()[start:]:
                complete = True
                break
    if not complete:
        start = len(text())
        keys(client, "ai-runtime")
        wait_for(proc, "Session LLM noyau", 20, start)
        if "TLS_COMPLETE" in text()[start:]:
            complete = True
    if not complete:
        raise RuntimeError("TLS_COMPLETE absent; peer events=%r sizes=%r peer error=%r log=%s" %
                           (peer.events, peer.sent_sizes, peer.error, text()[-2000:]))


def poll_sse(client, proc, peer):
    saw_delta = False
    complete = False
    needles = (
        "SSE : ok",
        "ai-sse-poll: flux SSE termine",
        "ai-sse-poll: attente de delta SSE",
        "ai-sse-poll: flux SSE non emis",
        "lecture refusee",
    )
    for _ in range(24):
        start = len(text())
        try:
            needle = keys_retry_any(client, proc, "ai-sse-poll", needles, 30)
        except RuntimeError as error:
            raise RuntimeError("sse-poll mute; peer events=%r sizes=%r peer error=%r (%s)" %
                               (peer.events, peer.sent_sizes, peer.error, error))
        chunk = text()[start:]
        if "ai-sse-poll: flux SSE non emis" in chunk or "lecture refusee" in chunk:
            raise RuntimeError("SSE refused; peer events=%r peer error=%r log=%s" %
                               (peer.events, peer.error, chunk[-1500:]))
        if "SSE : ok" in chunk:
            saw_delta = True
        if "ai-sse-poll: flux SSE termine" in chunk:
            complete = True
            wait_for(proc, "HTTP : 200", 10, start)
            break
        if needle == "ai-sse-poll: attente de delta SSE":
            continue
    if not saw_delta or not complete:
        raise RuntimeError("SSE incomplet delta=%s done=%s; peer events=%r sizes=%r peer error=%r log=%s" %
                           (saw_delta, complete, peer.events, peer.sent_sizes, peer.error, text()[-2000:]))


def main():
    os.makedirs(LOG_DIR, exist_ok=True)
    for path in (LOG, ERR, MON):
        try:
            os.remove(path)
        except OSError:
            pass
    peer = ControlledEthernetPeer(full_tls=True)
    peer.start()
    command = [
        "qemu-system-i386", "-kernel", os.path.join(ROOT, "build", "mohhdy.bin"),
        "-initrd", os.path.join(ROOT, "my_initrd.tar"), "-cpu", "max", "-m", "1024M",
        "-display", "none", "-vga", "none", "-serial", "file:" + LOG,
        "-monitor", "unix:%s,server,nowait" % MON, "-machine", "type=pc,accel=tcg",
        "-rtc", "base=2026-08-18T00:00:00",
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
            handshake_tls(client, proc, peer)
            try:
                keys_retry(client, proc, "ai-request ollama tiny /api/generate hi",
                           "ai-request: POST LLM chiffre emis", 60)
                start = len(text())
                keys_retry(client, proc, "ai-text-poll", "LLM : ok", 40)
                wait_for(proc, "HTTP : 200", 10, start)
                keys_retry(client, proc, "ai-next",
                           "ai-next: session LLM rearmement terminee", 30)
                start = len(text())
                keys(client, "ai-runtime")
                wait_for(proc, "Session LLM noyau", 20, start)
                if "TLS_COMPLETE" not in text()[start:]:
                    raise RuntimeError("TLS_COMPLETE absent apres ai-next; log=%s" %
                                       text()[start:][-800:])
                keys_retry(client, proc, "ai-stream-request ollama tiny /api/generate hi",
                           "ai-stream-request: POST SSE LLM chiffre emis", 60)
                poll_sse(client, proc, peer)
            except RuntimeError as error:
                raise RuntimeError("%s; peer events=%r peer error=%r" %
                                   (error, peer.events, peer.error))
            if peer.error is not None:
                raise RuntimeError("controlled Ethernet peer failed: %s" % peer.error)
            if peer.events.get("http_request", 0) < 2:
                raise RuntimeError("second POST absent; peer events=%r" % peer.events)
            required = ("client_hello", "certificate", "server_key_exchange",
                        "server_hello_done", "client_flight", "server_finished",
                        "http_response", "sse", "sse_done")
            missing = [name for name in required if peer.events.get(name, 0) == 0]
            if missing:
                raise RuntimeError("missing controlled TLS/next events: %s" % ", ".join(missing))
            print("QEMU NE2000 TLS/next local contract passed.")
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
        print("QEMU NE2000 TLS/next local contract failed: %s" % error)
        raise SystemExit(1)
