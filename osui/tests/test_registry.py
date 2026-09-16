#!/usr/bin/env python3
"""Registre partage aligne sur userspace/shell.c."""

from __future__ import annotations

import json
import sys
import unittest
from pathlib import Path

ROOT = Path(__file__).resolve().parents[2]
sys.path.insert(0, str(ROOT / "osui"))
sys.path.insert(0, str(ROOT / "osui" / "scripts"))

import extract_guest_commands as extract  # noqa: E402
from command_registry import load_registry  # noqa: E402


class GuestCommandRegistry(unittest.TestCase):
    def test_json_matches_shell_c(self) -> None:
        self.assertEqual(extract.main(["--check"]), 0)

    def test_registry_contains_core_guest_names(self) -> None:
        registry = load_registry()
        for name in ("help", "ai", "vfs-list", "vfs-read", "ai-acquire", "ls"):
            self.assertIn(name, registry.guest_commands)
        self.assertGreaterEqual(registry.guest_commands.__len__(), 100)
        self.assertFalse(registry.guest_html_stage)
        self.assertTrue(registry.is_linux_trap("apt"))
        public = registry.public_dict()
        self.assertEqual(public["artifact"], "shared/multiboot_shell_commands.json")
        self.assertEqual(public["header"], "userspace/mohhdy_osui_bridge.h")

    def test_header_mentions_no_guest_html_stage(self) -> None:
        header = (ROOT / "userspace" / "mohhdy_osui_bridge.h").read_text(encoding="utf-8")
        self.assertIn("#define MOHHDY_OSUI_GUEST_HTML_STAGE 0", header)
        self.assertIn("vfs-list", header)
        manifest = json.loads(
            (ROOT / "shared" / "multiboot_shell_commands.json").read_text(encoding="utf-8")
        )
        self.assertFalse(manifest["guest_html_stage"])
        self.assertIn("vfs-list", manifest["dispatch_only"])


if __name__ == "__main__":
    unittest.main(verbosity=2)
