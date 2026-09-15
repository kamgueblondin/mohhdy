#!/usr/bin/env python3
"""Contrat QEMU : TLS authentifie local puis HTTP 200 JSON sur pair controle."""
import os
import re
import socket
import subprocess
import time

from qemu_ne2k_controlled_peer import ControlledEthernetPeer

ROOT = os.path.abspath(os.path.join(os.path.dirname(__file__), "..", ".."))
LOG_DIR = os.path.join(ROOT, "test_logs")
RUN_LABEL = os.environ.get("MOHHDY_NE2K_RUN_LABEL", "")
if RUN_LABEL and not RUN_LABEL.replace("-", "").replace("_", "").isalnum():
    raise RuntimeError("MOHHDY_NE2K_RUN_LABEL invalide")
RUN_SUFFIX = ("-" + RUN_LABEL) if RUN_LABEL else ""
EXPECTED_GUEST_MAC = os.environ.get("MOHHDY_NE2K_GUEST_MAC", "").lower()
LOG = os.path.join(LOG_DIR, "ne2k-tls-http%s.log" % RUN_SUFFIX)
ERR = os.path.join(LOG_DIR, "ne2k-tls-http%s.err" % RUN_SUFFIX)
MON = os.path.join(LOG_DIR, "ne2k-tls-http%s-monitor.sock" % RUN_SUFFIX)
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
    peer = ControlledEthernetPeer(full_tls=True)
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
        start = send_command(client, proc, "ai-runtime")
        wait_for(proc, "Entropie TLS RDRAND : disponible (materiel)", 20, start)
        wait_for_prompt(proc, start)
        start = send_command(client, proc, "ai-acquire example.com")
        try:
            wait_for(proc, "ai-acquire: DHCP, DNS et SYN LLM demarres", 60, start)
            wait_for_prompt(proc, start)
        except RuntimeError as error:
            raise RuntimeError("%s; peer events=%r peer error=%r" %
                               (error, peer.events, peer.error))
        complete = False
        progressions = 0
        for _ in range(32):
            start = send_command(client, proc, "ai-tls-poll")
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
                    wait_for_prompt(proc, start)
                    break
                if "attente de trame TLS" in chunk:
                    wait_for_prompt(proc, start)
                    break
                time.sleep(0.10)
            else:
                raise RuntimeError("tls-poll mute; peer events=%r sizes=%r peer error=%r" %
                                   (peer.events, peer.sent_sizes, peer.error))
            if progressions >= 7:
                start = send_command(client, proc, "ai-runtime")
                wait_for(proc, "Session LLM noyau", 20, start)
                wait_for_prompt(proc, start)
                if "TLS_COMPLETE" in text()[start:]:
                    complete = True
                    break
        if not complete:
            start = send_command(client, proc, "ai-runtime")
            wait_for(proc, "Session LLM noyau", 20, start)
            wait_for_prompt(proc, start)
            if "TLS_COMPLETE" in text()[start:]:
                complete = True
        if not complete:
            raise RuntimeError("TLS_COMPLETE absent; peer events=%r sizes=%r peer error=%r log=%s" %
                               (peer.events, peer.sent_sizes, peer.error, text()[-2000:]))
        start = send_command(client, proc, "ai-request ollama tiny /api/generate hi")
        try:
            wait_for(proc, "ai-request: POST LLM chiffre emis", 40, start)
            wait_for_prompt(proc, start)
        except RuntimeError as error:
            raise RuntimeError("%s; peer events=%r peer error=%r" %
                               (error, peer.events, peer.error))
        start = send_command(client, proc, "ai-text-poll")
        try:
            wait_for(proc, "LLM : ok", 30, start)
            wait_for(proc, "HTTP : 200", 10, start)
            wait_for_prompt(proc, start)
        except RuntimeError as error:
            raise RuntimeError("%s; peer events=%r peer error=%r" %
                               (error, peer.events, peer.error))
        if peer.error is not None:
            raise RuntimeError("controlled Ethernet peer failed: %s" % peer.error)
        required = ("discover", "offer", "request", "ack", "arp", "dns", "syn", "syn_ack",
                    "client_hello", "certificate", "server_key_exchange", "server_hello_done",
                    "client_flight", "server_finished", "http_request", "http_response")
        missing = [name for name in required if peer.events.get(name, 0) == 0]
        if missing:
            raise RuntimeError("missing controlled TLS/HTTP events: %s" % ", ".join(missing))
        if EXPECTED_GUEST_MAC:
            try:
                expected_mac = bytes(int(part, 16) for part in EXPECTED_GUEST_MAC.split(":"))
            except ValueError:
                raise RuntimeError("MOHHDY_NE2K_GUEST_MAC invalide")
            if len(expected_mac) != 6 or peer.guest_mac != expected_mac:
                raise RuntimeError("guest MAC inattendue: %r" % peer.guest_mac)
        print("QEMU NE2000 TLS/HTTP local contract passed.")
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
        print("QEMU NE2000 TLS/HTTP local contract failed: %s" % error)
        raise SystemExit(1)
