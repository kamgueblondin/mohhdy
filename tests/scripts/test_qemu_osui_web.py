#!/usr/bin/env python3
"""Contrat QEMU : TLS authentifie local puis HTTP 200 JSON sur pair controle."""
import os
import re
import socket
import subprocess
import time

from qemu_http_client_peer import HttpClientPeer

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


def wait_for_prompt(proc, start):
    wait_for(proc, "(-.-)", 15, start)


def main():
    os.makedirs(LOG_DIR, exist_ok=True)
    for path in (LOG, ERR, MON):
        try:
            os.remove(path)
        except OSError:
            pass
    peer = HttpClientPeer(8080)
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
        start = send_command(client, proc, "web-serve 8080 5")
        wait_for(proc, "SYS_GETS: ligne lue: web-serve 8080 5", 20, start)
        r1 = peer.request(b"GET /status HTTP/1.0\r\nHost: mohhdy\r\n\r\n")
        r2 = peer.request(b"GET / HTTP/1.0\r\n\r\n")
        r3 = peer.request(b"GET /sessions HTTP/1.0\r\n\r\n")
        r4 = peer.request(b"GET /sessions HTTP/1.0\r\nAuthorization: Bearer " + token + b"\r\n\r\n")
        r5 = peer.request(b"POST /ai/chat HTTP/1.0\r\nAuthorization: Bearer " + token +
                          b"\r\nContent-Length: 2\r\n\r\nhi")
        wait_for(proc, "osui web-serve ok served=5", 60, start)
        if not r4.startswith(b"HTTP/1.0 200 OK") or b"own_session_only" not in r4:
            raise RuntimeError("GET /sessions with token: %r" % r4[:300])
        if not r5.startswith(b"HTTP/1.0 202 ") or b"osui chat" not in r5:
            raise RuntimeError("POST /ai/chat: %r" % r5[:400])
        wait_for_prompt(proc, start)
        if not r1.startswith(b"HTTP/1.0 200 OK\r\n") or b'"phase3_complete":false' not in r1:
            raise RuntimeError("GET /status: %r" % r1[:300])
        if b"MOHHDY local console" not in r2:
            raise RuntimeError("GET /: %r" % r2[:300])
        if not r3.startswith(b"HTTP/1.0 401 "):
            raise RuntimeError("GET /sessions without token: %r" % r3[:300])
        for path in ("path=/status status=200 sent=ok", "path=/ status=200 sent=ok",
                     "path=/sessions status=401 sent=ok"):
            if path not in text()[start:]:
                raise RuntimeError("guest log misses %s" % path)
        if peer.error is not None:
            raise RuntimeError("controlled Ethernet peer failed: %s" % peer.error)
        print("QEMU OS-UI web contract: %d/%d/%d response bytes" % (len(r1), len(r2), len(r3)))
        print("QEMU OS-UI web contract passed (HTTP to the guest API through networker).")
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
