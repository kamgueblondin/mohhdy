#!/usr/bin/env python3
"""Allowlist HTML et stub reasoner de la scene IA (hors make ci)."""

from __future__ import annotations

import sys
import unittest
from pathlib import Path

OSUI_ROOT = Path(__file__).resolve().parents[1]
sys.path.insert(0, str(OSUI_ROOT))

import stage  # noqa: E402


class StageSanitizer(unittest.TestCase):
    def test_strips_script_and_handlers(self) -> None:
        raw = (
            '<div class="ai-scene" onclick="alert(1)">'
            "<script>alert(1)</script>"
            '<img src=x onerror="alert(1)">'
            "<p>ok</p>"
            "</div>"
        )
        html, stripped = stage.sanitize_html(raw)
        self.assertGreaterEqual(stripped, 1)
        self.assertNotIn("<script", html.lower())
        self.assertNotIn("onclick", html.lower())
        self.assertNotIn("onerror", html.lower())
        self.assertNotIn("<img", html.lower())
        self.assertIn("<p>ok</p>", html)

    def test_keeps_svg_rect(self) -> None:
        raw = '<svg viewBox="0 0 10 10"><rect x="1" y="2" width="3" height="4" fill="#2b7a6e"></rect></svg>'
        html, stripped = stage.sanitize_html(raw)
        self.assertEqual(stripped, 0)
        self.assertIn("<svg", html)
        self.assertIn("viewBox=", html)
        self.assertIn("<rect", html)

    def test_blocks_javascript_href(self) -> None:
        html, _ = stage.sanitize_html('<a href="javascript:alert(1)">x</a><p>y</p>')
        self.assertNotIn("javascript:", html.lower())
        self.assertIn("<p>y</p>", html)


class StageReasoner(unittest.TestCase):
    def test_modes_and_stub_html(self) -> None:
        reflecting = stage.reason_stage("explique le VFS")
        self.assertEqual(reflecting["llm"], "stub_echo")
        self.assertEqual(reflecting["mode"], "reflecting")
        self.assertIn("ai-scene", reflecting["html"])
        self.assertIn("explique le VFS", reflecting["html"])
        self.assertNotIn("<script", reflecting["html"].lower())

        acting = stage.reason_stage("simule un acte overlay")
        self.assertEqual(acting["mode"], "acting")
        self.assertIn("ai-sim", acting["html"])

        presenting = stage.reason_stage("dessine trois boites")
        self.assertEqual(presenting["mode"], "presenting")
        self.assertIn("<svg", presenting["html"])
        self.assertIn("<rect", presenting["html"])

    def test_prompt_script_is_escaped_caption(self) -> None:
        payload = stage.reason_stage('<script>alert(1)</script> dessine')
        self.assertEqual(payload["mode"], "presenting")
        self.assertNotIn("<script", payload["html"].lower())
        self.assertIn("&lt;script&gt;", payload["html"])


if __name__ == "__main__":
    unittest.main(verbosity=2)
