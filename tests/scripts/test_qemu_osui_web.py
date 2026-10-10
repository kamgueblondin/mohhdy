#!/usr/bin/env python3
"""Contrat QEMU : TLS authentifie local puis HTTP 200 JSON sur pair controle."""
import os
import re
import socket
import subprocess
import threading
import time

from qemu_http_client_peer import HttpClientPeer, TlsConn

ROOT = os.path.abspath(os.path.join(os.path.dirname(__file__), "..", ".."))
LOG_DIR = os.path.join(ROOT, "test_logs")
RUN_LABEL = os.environ.get("MOHHDY_NE2K_RUN_LABEL", "")
if RUN_LABEL and not RUN_LABEL.replace("-", "").replace("_", "").isalnum():
    raise RuntimeError("MOHHDY_NE2K_RUN_LABEL invalide")
RUN_SUFFIX = ("-" + RUN_LABEL) if RUN_LABEL else ""
EXPECTED_GUEST_MAC = os.environ.get("MOHHDY_NE2K_GUEST_MAC", "").lower()
LOG = os.path.join(LOG_DIR, "osui-web%s.log" % RUN_SUFFIX)
ERR = os.path.join(LOG_DIR, "osui-web%s.err" % RUN_SUFFIX)
MON = os.path.join(LOG_DIR, "osui-web%s-monitor.sock" % RUN_SUFFIX)
KEY_HOLD_MS = 10
KEY_DELAY = 0.10
KEY_ECHO_TIMEOUT = 5.0
KEY_CHAR_RETRIES = 3
KEY_DUPLICATE_SETTLE_DELAY = 0.30
KEY_DUPLICATE_SETTLE_CHARS = ".s"


def text():
    try:
        with open(LOG, errors="replace") as handle:
            return handle.read()
    except OSError:
        return ""


def normalized_log(output):
    output = re.sub(r"(?<=\w)TIMER_ALIVE: tick=\d+\+?\r?\n(?=\w)", "", output)
    output = re.sub(r"TIMER_ALIVE: tick=\d+\+?\r?\n(?=\w)", "", output)
    output = re.sub(r"TIMER_ALIVE: tick=\d+\+?", "", output)
    output = re.sub(r"\[SCHED\] switching to task \d+\s*", " ", output)
    return output


def wait_for(proc, needle, timeout=45, start=0):
    deadline = time.monotonic() + timeout
    while time.monotonic() < deadline:
        if proc.poll() is not None:
            raise RuntimeError("QEMU stopped: %s" % text()[-2000:])
        if needle in text()[start:]:
            return
        time.sleep(0.10)
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


def send_command_once(client, command, proc):
    """Injecte une commande une seule fois après écho de chaque caractère.

    Un scan-code doublé est effacé avant l'entrée. Si une touche est absente,
    elle peut être répétée avant `ret` ; aucune ligne, requête TLS ou mutation
    ne sera réinjectée après l'exécution de la commande.
    """
    aliases = {" ": "spc", ".": "dot", "-": "minus", "/": "slash"}
    for char in command:
        count = 0
        for _ in range(KEY_CHAR_RETRIES):
            start = len(text())
            send_key(client, aliases.get(char, char.lower()))
            deadline = time.monotonic() + KEY_ECHO_TIMEOUT
            while time.monotonic() < deadline:
                if proc.poll() is not None:
                    raise RuntimeError("QEMU stopped during keyboard input")
                time.sleep(KEY_DELAY)
                count = key_echo_count(text()[start:], char)
                if count:
                    if char in KEY_DUPLICATE_SETTLE_CHARS:
                        time.sleep(KEY_DUPLICATE_SETTLE_DELAY)
                        count = key_echo_count(text()[start:], char)
                    break
            if count:
                break
        if count == 0:
            raise RuntimeError("missing keyboard echo for %r" % char)
        for _ in range(count - 1):
            send_key(client, "backspace")
            time.sleep(KEY_DELAY)
    send_key(client, "ret")


def send_command(client, proc, command):
    start = len(text())
    send_command_once(client, command, proc)
    deadline = time.monotonic() + 15
    while time.monotonic() < deadline:
        if proc.poll() is not None:
            raise RuntimeError("QEMU stopped after %s" % command)
        output = normalized_log(text()[start:])
        if command_echoed(output, command):
            return start
        if "SYS_GETS: ligne lue:" in output:
            raise RuntimeError("altered command %s: %s" % (command, output[-600:]))
        time.sleep(0.10)
    raise RuntimeError("command echo absent: %s" % command)


def type_line(client, proc, line):
    """Background web-serve: the shell reads keys with SYS_GETC (no
    SYS_GETS echo in the log), so keys are paced and the outcome checked."""
    aliases = {" ": "spc", ".": "dot", "-": "minus", "/": "slash"}
    for char in line:
        if proc.poll() is not None:
            raise RuntimeError("QEMU stopped during keyboard input")
        send_key(client, aliases.get(char, char.lower()))
        time.sleep(0.25)
    send_key(client, "ret")


def wait_for_prompt(proc, start):
    wait_for(proc, "(-.-)", 15, start)


def main():
    os.makedirs(LOG_DIR, exist_ok=True)
    for path in (LOG, ERR, MON):
        try:
            os.remove(path)
        except OSError:
            pass
    peer = HttpClientPeer()
    peer.start()
    command = [
        "qemu-system-i386", "-kernel", os.path.join(ROOT, "build", "mohhdy.bin"),
        "-initrd", os.path.join(ROOT, "my_initrd.tar"), "-cpu", "max", "-m", "1024M",
        "-display", "none", "-vga", "none", "-serial", "file:" + LOG,
        "-monitor", "unix:%s,server,nowait" % MON, "-machine", "type=pc,accel=tcg",
        "-rtc", "base=2026-08-18T00:00:00",
        "-netdev", "socket,id=n0,connect=127.0.0.1:%d" % peer.port,
        "-device", "ne2k_isa,netdev=n0" +
        ((",mac=" + EXPECTED_GUEST_MAC) if EXPECTED_GUEST_MAC else ""),
        "-no-reboot", "-no-shutdown",
    ]
    proc = None
    client = None
    try:
        with open(ERR, "wb") as err:
            proc = subprocess.Popen(command, cwd=ROOT, stdout=err, stderr=err)
        wait_for(proc, "(-.-)")
        client = monitor()
        # Roadmap step 5: the OS-UI local API and console served over HTTP
        # through the Ring 3 networker; the host peer is the HTTP client.
        start = send_command(client, proc, "ai-acquire example.com")
        wait_for(proc, "ai-acquire: DHCP, DNS et SYN LLM demarres", 60, start)
        wait_for_prompt(proc, start)
        start = send_command(client, proc, "grant web.api")
        wait_for(proc, "grant ok capability=web.api", 20, start)
        wait_for_prompt(proc, start)
        start = send_command(client, proc, "api-token")
        wait_for(proc, "api-token ok token=", 20, start)
        wait_for_prompt(proc, start)
        token = re.search(r"api-token ok token=(t\d{5})", text()[start:]).group(1).encode()
        # (1) HTTP, three connections open at the same time, then two more.
        start = send_command(client, proc, "web-serve 8080 5")
        c1, c2, c3 = peer.connect(8080), peer.connect(8080), peer.connect(8080)
        c3.send(b"GET /sessions HTTP/1.0\r\n\r\n")
        c1.send(b"GET /status HTTP/1.0\r\nHost: mohhdy\r\n\r\n")
        c2.send(b"GET / HTTP/1.0\r\n\r\n")
        r3, r1, r2 = c3.read_all(), c1.read_all(), c2.read_all()
        r4 = peer.request(b"GET /sessions HTTP/1.0\r\nAuthorization: Bearer " + token + b"\r\n\r\n")
        body = b"bonjour " * 40
        r5 = peer.request(b"POST /ai/chat HTTP/1.0\r\nAuthorization: Bearer " + token +
                          b"\r\nContent-Length: %d\r\n\r\n" % len(body) + body)
        wait_for(proc, "osui web-serve ok served=5 failed=0", 60, start)
        wait_for_prompt(proc, start)
        if not r1.startswith(b"HTTP/1.0 200 OK\r\n") or b'"phase3_complete":false' not in r1:
            raise RuntimeError("GET /status: %r" % r1[:300])
        if b"MOHHDY local console" not in r2:
            raise RuntimeError("GET /: %r" % r2[:300])
        if not r3.startswith(b"HTTP/1.0 401 "):
            raise RuntimeError("GET /sessions without token: %r" % r3[:300])
        if not r4.startswith(b"HTTP/1.0 200 OK") or b"own_session_only" not in r4:
            raise RuntimeError("GET /sessions with token: %r" % r4[:300])
        if not r5.startswith(b"HTTP/1.0 413 "):
            raise RuntimeError("POST /ai/chat 320 bytes: %r" % r5[:400])
        log = text()[start:]
        slots = set(re.findall(r"osui web-serve request slot=(\d) scheme=http method=GET path=/(?:status|sessions)? ", log)[:3])
        if len(slots) < 3:
            raise RuntimeError("three concurrent connections did not use three slots: %s" % slots)
        # (2) HTTPS, two concurrent TLS 1.2 connections.
        start = send_command(client, proc, "web-serve 8443 2 tls")
        results = {}

        def https(name, raw):
            try:
                t = TlsConn(peer.connect(8443)).handshake(240)
                t.send(raw)
                results[name] = t.read_all(120)
            except Exception as exc:  # reported below with the guest log
                results[name] = exc

        th = [threading.Thread(target=https, args=("s1", b"GET /status HTTP/1.0\r\n\r\n")),
              threading.Thread(target=https, args=("s2", b"POST /ai/chat HTTP/1.0\r\nAuthorization: Bearer " +
                                                  token + b"\r\nContent-Length: 2\r\n\r\nhi"))]
        t_https = time.monotonic()
        for t in th:
            t.start()
            time.sleep(0.5)
        for t in th:
            t.join(400)
        for name in ("s1", "s2"):
            if not isinstance(results.get(name), bytes):
                try:
                    wait_for(proc, "osui web-serve ok served=", 200, start)
                except RuntimeError:
                    pass
                raise RuntimeError("%s: %r; guest: %s" % (name, results.get(name), " | ".join(
                    l for l in text()[start:].splitlines() if "web-serve" in l)))
        s1, s2 = results["s1"], results["s2"]
        print("HTTPS: two concurrent TLS 1.2 exchanges in %.1f s, client retransmits %d" % (
            time.monotonic() - t_https, sum(c.retransmits for c in peer.conns.values())))
        wait_for(proc, "osui web-serve ok served=2 failed=0", 120, start)
        wait_for_prompt(proc, start)
        if not s1.startswith(b"HTTP/1.0 200 OK") or b'"phase3_complete":false' not in s1:
            raise RuntimeError("HTTPS GET /status: %r" % s1[:300])
        if not s2.startswith(b"HTTP/1.0 202 ") or b"osui chat" not in s2:
            raise RuntimeError("HTTPS POST /ai/chat: %r" % s2[:400])
        if "scheme=https method=GET path=/status status=200 sent=ok" not in text()[start:]:
            raise RuntimeError("guest log misses the https request")
        # (3) Background: the console stays usable while the server runs.
        start = send_command(client, proc, "web-serve start 8080")
        wait_for(proc, "mode=background", 20, start)
        b1 = peer.request(b"GET /status HTTP/1.0\r\n\r\n")
        if not b1.startswith(b"HTTP/1.0 200 OK"):
            raise RuntimeError("background GET /status: %r" % b1[:300])
        type_line(client, proc, "web-serve status")
        wait_for(proc, "osui web-serve status=running port=8080 scheme=http served=1", 30, start)
        b2 = peer.request(b"GET / HTTP/1.0\r\n\r\n")
        if b"MOHHDY local console" not in b2:
            raise RuntimeError("background GET /: %r" % b2[:300])
        type_line(client, proc, "web-serve stop")
        wait_for(proc, "osui web-serve ok served=2 failed=0", 30, start)
        wait_for_prompt(proc, len(text()) - 400)
        if peer.error is not None:
            raise RuntimeError("controlled Ethernet peer failed: %s" % peer.error)
        print("QEMU OS-UI web contract: http 3 concurrent + 2, https 2 concurrent, background 2")
        print("QEMU OS-UI web contract passed (HTTP/HTTPS to the guest API through networker).")
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
        print("QEMU OS-UI web contract failed: %s" % error)
        raise SystemExit(1)
