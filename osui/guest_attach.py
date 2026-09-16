#!/usr/bin/env python3
"""Attache live du shell Multiboot (QEMU serial / HMP), best-effort honnete.

Le guest Ring 3 lit le clavier PS/2. Le harness existant (make qemu-smoke)
injecte des scancodes via le moniteur HMP. Ce module expose le meme chemin
depuis /shell, plus un transport serie (unix/tcp/pty) pour un TTY deja ouvert.

Defaut : bootstrap. Live seulement si MOHHDY_SHELL_ATTACH=live ET qu'une
connexion reussit. Jamais de pretention VGA/#ai-stage guest.
"""

from __future__ import annotations

import os
import socket
import time
from dataclasses import dataclass
from typing import Optional

PROMPT = "MOHHDY>"
ATTACH_BOOTSTRAP = "bootstrap"
ATTACH_LIVE = "live_guest"
HMP_HOLD_MS = 10
HMP_KEY_ALIASES = {
    " ": "spc",
    "-": "minus",
    ".": "dot",
    "/": "slash",
    "_": "shift-minus",
    "=": "equal",
    ":": "shift-semicolon",
    ";": "semicolon",
    "'": "apostrophe",
    '"': "shift-apostrophe",
    ",": "comma",
    "<": "shift-comma",
    ">": "shift-dot",
    "?": "shift-slash",
    "!": "shift-1",
    "@": "shift-2",
    "#": "shift-3",
    "$": "shift-4",
    "%": "shift-5",
    "^": "shift-6",
    "&": "shift-7",
    "*": "shift-8",
    "(": "shift-9",
    ")": "shift-0",
}


def _env(name: str, default: str = "") -> str:
    return (os.environ.get(name) or default).strip()


@dataclass
class AttachConfig:
    mode: str = ATTACH_BOOTSTRAP
    serial: str = ""
    serial_kind: str = "auto"
    monitor: str = ""
    timeout: float = 2.0
    prompt: str = PROMPT
    transport: str = "auto"

    @classmethod
    def from_env(cls) -> "AttachConfig":
        mode = _env("MOHHDY_SHELL_ATTACH", ATTACH_BOOTSTRAP).lower()
        if mode in ("live", "live_guest", "qemu"):
            mode = ATTACH_LIVE
        else:
            mode = ATTACH_BOOTSTRAP
        timeout_raw = _env("MOHHDY_GUEST_TIMEOUT", "2.0")
        try:
            timeout = float(timeout_raw)
        except ValueError:
            timeout = 2.0
        return cls(
            mode=mode,
            serial=_env("MOHHDY_GUEST_SERIAL"),
            serial_kind=_env("MOHHDY_GUEST_SERIAL_KIND", "auto").lower(),
            monitor=_env("MOHHDY_GUEST_MONITOR"),
            timeout=max(0.2, min(timeout, 30.0)),
            prompt=_env("MOHHDY_GUEST_PROMPT", PROMPT) or PROMPT,
            transport=_env("MOHHDY_GUEST_TRANSPORT", "auto").lower(),
        )

    def public_dict(self) -> dict:
        return {
            "requested": self.mode,
            "serial": self.serial or None,
            "serial_kind": self.serial_kind,
            "monitor": self.monitor or None,
            "timeout_s": self.timeout,
            "prompt": self.prompt,
            "transport": self.transport,
            "env": {
                "MOHHDY_SHELL_ATTACH": "bootstrap|live",
                "MOHHDY_GUEST_SERIAL": "unix:/path.sock | tcp:127.0.0.1:PORT | /dev/pts/N",
                "MOHHDY_GUEST_MONITOR": "unix:/path-monitor.sock",
                "MOHHDY_GUEST_TRANSPORT": "auto|serial|hmp",
                "MOHHDY_GUEST_TIMEOUT": "secondes",
            },
        }


def parse_endpoint(raw: str, kind: str = "auto") -> tuple[str, str]:
    """Retourne (kind, target) : unix path, tcp host:port, pty path."""
    text = (raw or "").strip()
    if not text:
        raise ValueError("endpoint vide")
    lowered = kind.lower()
    if text.startswith("unix:"):
        return "unix", text[5:]
    if text.startswith("tcp:"):
        return "tcp", text[4:]
    if text.startswith("pty:") or text.startswith("file:"):
        return "pty", text.split(":", 1)[1]
    if lowered in ("unix", "tcp", "pty"):
        return lowered, text
    if text.startswith("/") or text.endswith(".sock"):
        if "/pts/" in text or text.startswith("/dev/"):
            return "pty", text
        return "unix", text
    if ":" in text and not text.startswith("/"):
        return "tcp", text
    return "pty", text


class SerialTransport:
    """Client serie unix/tcp/pty. Best-effort, timeout court."""

    def __init__(self, endpoint: str, kind: str = "auto", timeout: float = 2.0) -> None:
        self.endpoint = endpoint
        self.kind, self.target = parse_endpoint(endpoint, kind)
        self.timeout = timeout
        self.sock: Optional[socket.socket] = None
        self.fd: Optional[int] = None
        self._buf = b""

    def connect(self) -> None:
        self.close()
        if self.kind == "unix":
            sock = socket.socket(socket.AF_UNIX, socket.SOCK_STREAM)
            sock.settimeout(self.timeout)
            sock.connect(self.target)
            self.sock = sock
        elif self.kind == "tcp":
            host, port_s = self.target.rsplit(":", 1)
            sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
            sock.settimeout(self.timeout)
            sock.connect((host, int(port_s)))
            self.sock = sock
        elif self.kind == "pty":
            self.fd = os.open(self.target, os.O_RDWR | os.O_NOCTTY)
            os.set_blocking(self.fd, False)
        else:
            raise ValueError("kind serie inconnu: %s" % self.kind)

    def send(self, data: str) -> None:
        blob = data.encode("utf-8", errors="replace")
        if self.sock is not None:
            self.sock.sendall(blob)
            return
        if self.fd is not None:
            os.write(self.fd, blob)
            return
        raise RuntimeError("serie non connectee")

    def recv_some(self) -> bytes:
        if self.sock is not None:
            try:
                chunk = self.sock.recv(4096)
            except socket.timeout:
                return b""
            return chunk or b""
        if self.fd is not None:
            try:
                return os.read(self.fd, 4096)
            except BlockingIOError:
                return b""
            except OSError:
                return b""
        return b""

    def read_until(self, needle: str, timeout: Optional[float] = None) -> str:
        deadline = time.monotonic() + (self.timeout if timeout is None else timeout)
        target = needle.encode("utf-8", errors="replace")
        while time.monotonic() < deadline:
            if target in self._buf:
                idx = self._buf.find(target) + len(target)
                out = self._buf[:idx]
                self._buf = self._buf[idx:]
                return out.decode("utf-8", errors="replace")
            chunk = self.recv_some()
            if chunk:
                self._buf += chunk
                continue
            time.sleep(0.02)
        text = self._buf.decode("utf-8", errors="replace")
        self._buf = b""
        return text

    def drain(self) -> str:
        bits = [self._buf]
        self._buf = b""
        while True:
            chunk = self.recv_some()
            if not chunk:
                break
            bits.append(chunk)
        return b"".join(bits).decode("utf-8", errors="replace")

    def close(self) -> None:
        if self.sock is not None:
            try:
                self.sock.close()
            except OSError:
                pass
            self.sock = None
        if self.fd is not None:
            try:
                os.close(self.fd)
            except OSError:
                pass
            self.fd = None
        self._buf = b""


class HmpTransport:
    """Moniteur QEMU (HMP) : sendkey, meme idee que tests/scripts/ci_qemu_*.py."""

    def __init__(self, endpoint: str, timeout: float = 2.0) -> None:
        self.serial_like = SerialTransport(endpoint, "auto", timeout)
        self.timeout = timeout

    def connect(self) -> None:
        self.serial_like.connect()
        self.serial_like.drain()

    def sendkey(self, key: str) -> None:
        self.serial_like.send("sendkey %s %d\n" % (key, HMP_HOLD_MS))
        time.sleep(0.01)
        self.serial_like.drain()

    def type_line(self, line: str) -> None:
        for char in line:
            key = HMP_KEY_ALIASES.get(char)
            if key is None:
                if char.isupper():
                    key = "shift-%s" % char.lower()
                else:
                    key = char.lower()
            self.sendkey(key)
        self.sendkey("ret")

    def close(self) -> None:
        self.serial_like.close()


class GuestLink:
    """Session live : serial et/ou HMP. Handshake = prompt MOHHDY>."""

    def __init__(self, config: AttachConfig) -> None:
        self.config = config
        self.serial: Optional[SerialTransport] = None
        self.hmp: Optional[HmpTransport] = None
        self.live = False
        self.last_error = ""
        self.transport_used = ""

    def connect(self) -> bool:
        self.close()
        cfg = self.config
        want = cfg.transport
        errors = []
        use_hmp = want in ("auto", "hmp") and bool(cfg.monitor)
        use_serial = want in ("auto", "serial") and bool(cfg.serial)
        if not use_hmp and not use_serial:
            self.last_error = (
                "live demande mais MOHHDY_GUEST_SERIAL / MOHHDY_GUEST_MONITOR absents"
            )
            return False
        if use_serial:
            try:
                serial = SerialTransport(cfg.serial, cfg.serial_kind, cfg.timeout)
                serial.connect()
                serial.drain()
                serial.send("\n")
                seen = serial.read_until(cfg.prompt, cfg.timeout)
                if cfg.prompt not in seen:
                    serial.close()
                    errors.append("prompt %s absent sur la serie" % cfg.prompt)
                else:
                    self.serial = serial
                    self.transport_used = "serial"
            except OSError as exc:
                errors.append("serie: %s" % exc)
        if use_hmp and self.serial is None:
            try:
                hmp = HmpTransport(cfg.monitor, cfg.timeout)
                hmp.connect()
                self.hmp = hmp
                self.transport_used = "hmp"
            except OSError as exc:
                errors.append("hmp: %s" % exc)
        if self.serial is None and self.hmp is None:
            self.last_error = "; ".join(errors) or "connexion live impossible"
            return False
        if self.hmp is not None and self.serial is None and cfg.serial:
            try:
                serial = SerialTransport(cfg.serial, cfg.serial_kind, cfg.timeout)
                serial.connect()
                self.serial = serial
                self.transport_used = "hmp+serial"
            except OSError:
                pass
        self.live = True
        self.last_error = ""
        return True

    def execute(self, line: str) -> tuple[str, int]:
        if not self.live:
            return "live_guest=false : pas de session QEMU\n", 1
        text = (line or "").rstrip("\n")
        if self.serial is not None and self.hmp is None:
            self.serial.drain()
            self.serial.send(text + "\n")
            raw = self.serial.read_until(self.config.prompt, self.config.timeout)
            return _strip_echo(raw, text, self.config.prompt), 0
        if self.hmp is not None:
            if self.serial is not None:
                self.serial.drain()
            self.hmp.type_line(text)
            if self.serial is not None:
                raw = self.serial.read_until(self.config.prompt, self.config.timeout)
                return _strip_echo(raw, text, self.config.prompt), 0
            return (
                "(hmp sendkey envoye ; pas de socket serie pour lire la sortie)\n",
                0,
            )
        return "transport live incomplet\n", 1

    def close(self) -> None:
        if self.serial is not None:
            self.serial.close()
            self.serial = None
        if self.hmp is not None:
            self.hmp.close()
            self.hmp = None
        self.live = False
        self.transport_used = ""

    def status(self) -> dict:
        return {
            "live_guest": self.live,
            "transport": self.transport_used or None,
            "error": self.last_error or None,
            "qemu_serial": bool(self.serial),
            "qemu_monitor": bool(self.hmp),
        }


def _strip_echo(raw: str, command: str, prompt: str) -> str:
    text = raw.replace("\r\n", "\n").replace("\r", "\n")
    if text.startswith(command):
        text = text[len(command) :]
        if text.startswith("\n"):
            text = text[1:]
    if text.endswith(prompt):
        text = text[: -len(prompt)]
    if text.startswith(prompt):
        text = text[len(prompt) :]
        if text.startswith(" "):
            text = text[1:]
    return text if text.endswith("\n") or text == "" else text + "\n"


def qemu_available() -> bool:
    for directory in os.environ.get("PATH", "").split(os.pathsep):
        candidate = os.path.join(directory, "qemu-system-i386")
        if os.path.isfile(candidate) and os.access(candidate, os.X_OK):
            return True
    return False
