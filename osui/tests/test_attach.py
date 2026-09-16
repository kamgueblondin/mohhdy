#!/usr/bin/env python3
"""Attache live best-effort contre un faux guest serie (hors QEMU)."""

from __future__ import annotations

import os
import socket
import sys
import tempfile
import threading
import time
import unittest
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parents[1]))

from guest_attach import AttachConfig, GuestLink  # noqa: E402
from multiboot_shell import MultibootShell  # noqa: E402

PROMPT = "MOHHDY>"


class FakeGuest:
    def __init__(self) -> None:
        self.tmp = tempfile.mkdtemp(prefix="mohhdy-attach-")
        self.path = os.path.join(self.tmp, "serial.sock")
        self.stop = threading.Event()
        self.thread = threading.Thread(target=self._run, daemon=True)

    def start(self) -> str:
        self.thread.start()
        deadline = time.monotonic() + 2
        while time.monotonic() < deadline and not os.path.exists(self.path):
            time.sleep(0.02)
        return self.path

    def close(self) -> None:
        self.stop.set()
        self.thread.join(timeout=1)

    def _run(self) -> None:
        if os.path.exists(self.path):
            os.unlink(self.path)
        server = socket.socket(socket.AF_UNIX, socket.SOCK_STREAM)
        server.bind(self.path)
        server.listen(1)
        server.settimeout(0.25)
        try:
            while not self.stop.is_set():
                try:
                    conn, _ = server.accept()
                except socket.timeout:
                    continue
                with conn:
                    conn.sendall(PROMPT.encode("ascii"))
                    buf = b""
                    conn.settimeout(0.4)
                    while not self.stop.is_set():
                        try:
                            chunk = conn.recv(1024)
                        except socket.timeout:
                            continue
                        except OSError:
                            break
                        if not chunk:
                            break
                        buf += chunk
                        while b"\n" in buf:
                            line, buf = buf.split(b"\n", 1)
                            cmd = line.decode("utf-8", errors="replace").strip()
                            if cmd == "help":
                                body = "vfs-list\nai\nhelp ok\n"
                            else:
                                body = "guest %s\n" % (cmd or "empty")
                            conn.sendall((body + PROMPT).encode("utf-8"))
        finally:
            server.close()
            if os.path.exists(self.path):
                os.unlink(self.path)


class LiveAttachFakeSerial(unittest.TestCase):
    def setUp(self) -> None:
        self.fake = FakeGuest()
        self.path = self.fake.start()

    def tearDown(self) -> None:
        self.fake.close()

    def test_handshake_and_help(self) -> None:
        cfg = AttachConfig(
            mode="live_guest",
            serial="unix:" + self.path,
            transport="serial",
            timeout=1.5,
        )
        link = GuestLink(cfg)
        self.assertTrue(link.connect())
        self.assertTrue(link.live)
        output, rc = link.execute("help")
        self.assertEqual(rc, 0)
        self.assertIn("vfs-list", output)
        link.close()

    def test_shell_attach_sets_live_guest(self) -> None:
        cfg = AttachConfig(
            mode="bootstrap",
            serial="unix:" + self.path,
            transport="serial",
            timeout=1.5,
        )
        shell = MultibootShell(cfg)
        self.assertFalse(shell.execute("guest-status")["live_guest"])
        attached = shell.execute("attach")
        self.assertEqual(attached["rc"], 0)
        self.assertTrue(attached["live_guest"])
        help_out = shell.execute("help")
        self.assertIn("vfs-list", help_out["output"])
        trap = shell.execute("sudo apt")
        self.assertEqual(trap["rc"], 1)
        self.assertIn("Pas un bash Linux", trap["output"])
        detached = shell.execute("detach")
        self.assertFalse(detached["live_guest"])

    def test_missing_endpoint_stays_bootstrap(self) -> None:
        shell = MultibootShell(AttachConfig(mode="live_guest"))
        out = shell.execute("attach")
        self.assertEqual(out["rc"], 1)
        self.assertFalse(out["live_guest"])
        self.assertIn("bootstrap", out["output"])


if __name__ == "__main__":
    unittest.main(verbosity=2)
