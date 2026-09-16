#!/usr/bin/env python3
"""Routes Prompt OS (hors make ci)."""

from __future__ import annotations

import sys
import unittest
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parents[1]))

import prompt_os  # noqa: E402


class PromptOsRoutes(unittest.TestCase):
    def test_slash_and_open(self) -> None:
        self.assertEqual(prompt_os.route_prompt("/browser")["pane"], "browser")
        self.assertEqual(prompt_os.route_prompt("ouvre le shell")["pane"], "shell")
        self.assertEqual(prompt_os.route_prompt("open admin")["pane"], "admin")
        self.assertEqual(prompt_os.route_prompt("affiche le navigateur")["pane"], "browser")

    def test_stage_draw_and_plan(self) -> None:
        draw = prompt_os.route_prompt("dessine un cercle")
        self.assertEqual(draw["kind"], "stage")
        plan = prompt_os.route_prompt("mini-plan autonome puis presente")
        self.assertEqual(plan["kind"], "stage_plan")
        self.assertTrue(plan["autonomous"])
        slash_plan = prompt_os.route_prompt("/plan")
        self.assertEqual(slash_plan["kind"], "stage_plan")

    def test_safe_shell_from_chat(self) -> None:
        help_route = prompt_os.route_prompt("liste les commandes guest")
        self.assertEqual(help_route["kind"], "shell")
        self.assertEqual(help_route["line"], "help")
        ai_help = prompt_os.route_prompt("ai-help")
        self.assertEqual(ai_help["line"], "ai-help")
        guest = prompt_os.route_prompt("guest-status")
        self.assertEqual(guest["line"], "guest-status")
        vfs = prompt_os.route_prompt("vfs-list initrd/bin/")
        self.assertTrue(vfs["line"].startswith("vfs-list"))

    def test_chat_default(self) -> None:
        chat = prompt_os.route_prompt("explique le VFS")
        self.assertEqual(chat["kind"], "chat")
        self.assertTrue(chat["stage"])

    def test_unknown_slash(self) -> None:
        unknown = prompt_os.route_prompt("/nope")
        self.assertEqual(unknown["kind"], "unknown_slash")

    def test_linux_trap_from_chat(self) -> None:
        apt = prompt_os.route_prompt("apt install nginx")
        self.assertEqual(apt["kind"], "shell")
        self.assertTrue(apt.get("refused_linux"))
        sudo = prompt_os.route_prompt("sudo bash")
        self.assertEqual(sudo["kind"], "shell")
        self.assertTrue(sudo.get("refused_linux"))


if __name__ == "__main__":
    unittest.main(verbosity=2)
