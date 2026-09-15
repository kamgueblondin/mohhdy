#!/usr/bin/env python3
"""Smoke test for the AI provider/model control plane in MOHHDY."""
import os
import socket
import subprocess
import sys
import time

ROOT = os.path.abspath(os.path.join(os.path.dirname(__file__), "..", ".."))
LOG_DIR = os.path.join(ROOT, "test_logs")
LOG = os.path.join(LOG_DIR, "ai-provider-smoke.log")
ERR = os.path.join(LOG_DIR, "ai-provider-smoke.err")
MON = os.path.join(LOG_DIR, "ai-provider-monitor.sock")
KERNEL = os.path.join(ROOT, "build", "mohhdy.bin")
INITRD = os.path.join(ROOT, "my_initrd.tar")


def log_text():
    try:
        with open(LOG, "r", errors="replace") as handle:
            return handle.read()
    except OSError:
        return ""


def wait_for(needle, proc, timeout=30):
    deadline = time.time() + timeout
    while time.time() < deadline:
        if proc.poll() is not None:
            raise RuntimeError("QEMU stopped early")
        if needle in log_text():
            return
        time.sleep(0.1)
    raise RuntimeError("missing output: %s" % needle)


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
    raise RuntimeError("QEMU monitor unavailable")


def send_keys(client, keys):
    for key in keys:
        client.sendall(("sendkey %s\n" % key).encode("ascii"))
        time.sleep(0.30)


def key_sequence(command):
    special = {" ": "spc", ".": "dot", "-": "minus", "_": "shift-minus", "/": "slash"}
    return [special.get(char, char.lower()) for char in command] + ["ret"]


def run_command(client, proc, command, expected):
    for attempt in range(2):
        before = len(log_text())
        send_keys(client, key_sequence(command))
        deadline = time.time() + 10
        while time.time() < deadline:
            if proc.poll() is not None:
                raise RuntimeError("QEMU stopped while executing %s" % command)
            if expected in log_text()[before:]:
                return
            time.sleep(0.1)
        if attempt == 0:
            time.sleep(0.5)
    raise RuntimeError("command %s did not emit %s" % (command, expected))


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
            run_command(monitor, proc, "ai-provider", "Fournisseur IA : local")
            run_command(monitor, proc, "ai-model list", "gpt2.gguf")
            run_command(monitor, proc, "ai-model use control.bin", "Profil memorise; seuls les profils GPT-2 locaux sont executables")
            run_command(monitor, proc, "ai-model", "control.bin")
            run_command(monitor, proc, "ai-runtime", "Runtime IA bare-metal")
            run_command(monitor, proc, "ai-runtime", "Session LLM noyau  : IDLE (NE2000 absent)")
            run_command(monitor, proc, "ai-runtime", "Bail DHCP noyau    : absent")
            run_command(monitor, proc, "ai-runtime", "Entropie TLS RDRAND : indisponible")
            run_command(monitor, proc, "ai-runtime", "Ancre X.509 noyau  : ISRG Root X1 validee")
            run_command(monitor, proc, "ai-credential sk-test", "ai-credential: bearer OpenAI provisionne dans le noyau")
            run_command(monitor, proc, "history", "ai-credential [masque]")
            run_command(monitor, proc, "ai-acquire api.openai.com 0", "ai-acquire: port invalide")
            run_command(monitor, proc, "ai-acquire api.openai.com", "ai-acquire: NE2000 absent; aucun etat reseau publie")
            run_command(monitor, proc, "ai-tls-poll", "ai-tls-poll: NE2000 absent; aucun etat TLS publie")
            run_command(monitor, proc, "ai-request ollama llama3 /api/generate hello", "ai-request: TLS authentifie requis")
            run_command(monitor, proc, "ai-stream-request ollama llama3 /api/generate hello", "ai-request: TLS authentifie requis")
            run_command(monitor, proc, "ai-text-poll", "ai-text-poll: requete LLM non emise")
            run_command(monitor, proc, "ai-sse-poll", "ai-sse-poll: flux SSE non emis")
            run_command(monitor, proc, "ai-next", "ai-next: reponse LLM complete requise")
            run_command(monitor, proc, "ai-close", "ai-close: aucune session LLM active")
            run_command(monitor, proc, "ai-runtime", "Session LLM noyau  : IDLE (NE2000 absent)")
            run_command(monitor, proc, "ai-runtime", "Bail DHCP noyau    : absent")
            run_command(monitor, proc, "net-status", "net-status ok AOS-1521")
            run_command(monitor, proc, "net-status json", "\"nic\":\"absent\"")
            run_command(monitor, proc, "net-status json", "\"openai\":\"unavailable\"")
            run_command(monitor, proc, "ai-provider openai", "OpenAI selectionne")
            run_command(monitor, proc, "ai hello", "OpenAI selectionne : utilisez ai-acquire")
            run_command(monitor, proc, "ai-provider local", "Fournisseur local selectionne")
            print("AI provider control-plane smoke passed.")
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
        print("AI provider control-plane smoke failed: %s" % error, file=sys.stderr)
        raise SystemExit(1)
