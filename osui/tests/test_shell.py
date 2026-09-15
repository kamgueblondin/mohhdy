#!/usr/bin/env python3
"""Vocabulaire shell Multiboot bootstrap (hors make ci / hors QEMU)."""

from __future__ import annotations

import sys
import unittest
from pathlib import Path

OSUI_ROOT = Path(__file__).resolve().parents[1]
sys.path.insert(0, str(OSUI_ROOT))

from multiboot_shell import MultibootShell  # noqa: E402


class MultibootShellVocab(unittest.TestCase):
    def setUp(self) -> None:
        self.shell = MultibootShell()

    def test_help_lists_guest_commands(self) -> None:
        out = self.shell.execute("help")
        self.assertEqual(out["rc"], 0)
        self.assertEqual(out["prompt"], "MOHHDY>")
        self.assertEqual(out["attachment"], "bootstrap")
        self.assertFalse(out["live_guest"])
        text = out["output"]
        self.assertIn("ai <question>", text)
        self.assertIn("vfs-list", text)
        self.assertIn("userspace/shell.c", text)
        self.assertIn("Pas un bash Linux", text)
        self.assertIn("QEMU/serial", text)

    def test_vfs_list_mirror(self) -> None:
        out = self.shell.execute("vfs-list initrd/bin/")
        self.assertEqual(out["rc"], 0)
        self.assertIn("shell", out["output"])
        self.assertIn("vfsserver", out["output"])
        self.assertIn("vfsserver", out["output"])

    def test_ai_updates_stage_payload(self) -> None:
        out = self.shell.execute("ai dessine trois boites")
        self.assertEqual(out["rc"], 0)
        self.assertIn("stub", out["output"])
        self.assertIn("stage", out)
        self.assertEqual(out["stage"]["mode"], "presenting")
        self.assertIn("<svg", out["stage"]["html"])

    def test_linux_trap_and_attach_honesty(self) -> None:
        apt = self.shell.execute("apt install foo")
        self.assertEqual(apt["rc"], 1)
        self.assertIn("Pas un bash Linux", apt["output"])
        attach = self.shell.execute("attach")
        self.assertEqual(attach["rc"], 1)
        self.assertIn("n'est pas branche", attach["output"])
        self.assertIn("bootstrap", attach["output"])

    def test_openai_provider_refused(self) -> None:
        out = self.shell.execute("ai-provider openai")
        self.assertEqual(out["rc"], 1)
        self.assertIn("refuse", out["output"])


if __name__ == "__main__":
    unittest.main(verbosity=2)
