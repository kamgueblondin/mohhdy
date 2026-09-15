#!/usr/bin/env python3
"""Fumee HTTP du scaffold ASSIST-050 (stdlib, sans Docker, hors make ci)."""

from __future__ import annotations

import json
import sys
import threading
import time
import unittest
import urllib.error
import urllib.request
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
sys.path.insert(0, str(ROOT))

import server  # noqa: E402


SECRET_MARKERS = (
    "api_key",
    "apikey",
    "secret_key",
    "BEGIN PRIVATE KEY",
    "sk-proj",
    "openai",
    "bearer ",
)


class AgentHttpSmoke(unittest.TestCase):
    @classmethod
    def setUpClass(cls) -> None:
        cls.httpd = server.make_server("127.0.0.1", 0)
        cls.port = cls.httpd.server_address[1]
        cls.base = "http://127.0.0.1:%d" % cls.port
        cls.thread = threading.Thread(target=cls.httpd.serve_forever, daemon=True)
        cls.thread.start()
        cls._wait_ready()

    @classmethod
    def tearDownClass(cls) -> None:
        cls.httpd.shutdown()
        cls.httpd.server_close()
        cls.thread.join(timeout=2)

    @classmethod
    def _wait_ready(cls) -> None:
        deadline = time.time() + 3
        last_error = None
        while time.time() < deadline:
            try:
                urllib.request.urlopen(cls.base + "/health", timeout=0.3)
                return
            except OSError as exc:
                last_error = exc
                time.sleep(0.05)
        raise RuntimeError("serveur agent injoignable: %s" % last_error)

    def _get(self, path: str):
        return urllib.request.urlopen(self.base + path, timeout=2)

    def test_health_json(self) -> None:
        with self._get("/health") as resp:
            self.assertEqual(resp.status, 200)
            self.assertIn("application/json", resp.headers.get("Content-Type", ""))
            body = json.loads(resp.read().decode("utf-8"))
        self.assertEqual(body, {"status": "ok", "service": "mohhdy-agent"})

    def test_admin_shell(self) -> None:
        with self._get("/admin") as resp:
            self.assertEqual(resp.status, 200)
            html = resp.read().decode("utf-8")
        self.assertIn("mohhdy-sessions", html)
        self.assertIn("Aucune session", html)
        self.assertIn("Scaffold", html)

    def test_embed_js_launcher_without_secrets(self) -> None:
        with self._get("/embed.js") as resp:
            self.assertEqual(resp.status, 200)
            self.assertIn("javascript", resp.headers.get("Content-Type", ""))
            script = resp.read().decode("utf-8")
        self.assertIn("mohhdy-launcher", script)
        self.assertIn("data-mohhdy-site", script)
        lowered = script.lower()
        for marker in SECRET_MARKERS:
            self.assertNotIn(marker.lower(), lowered)

    def test_demo_loads_embed(self) -> None:
        with self._get("/demo") as resp:
            self.assertEqual(resp.status, 200)
            html = resp.read().decode("utf-8")
        self.assertIn("/embed.js", html)
        self.assertIn("data-mohhdy-site=", html)
        self.assertIn("stub", html.lower())

    def test_index_lists_routes(self) -> None:
        with self._get("/") as resp:
            self.assertEqual(resp.status, 200)
            html = resp.read().decode("utf-8")
        self.assertIn("/health", html)
        self.assertIn("/admin", html)
        self.assertIn("/embed.js", html)
        self.assertIn("/demo", html)

    def test_unknown_is_json_404(self) -> None:
        with self.assertRaises(urllib.error.HTTPError) as ctx:
            self._get("/no-such-route")
        err = ctx.exception
        self.assertEqual(err.code, 404)
        body = json.loads(err.read().decode("utf-8"))
        self.assertEqual(body["status"], "not_found")

    def test_dot_env_not_served(self) -> None:
        with self.assertRaises(urllib.error.HTTPError) as ctx:
            self._get("/.env")
        self.assertEqual(ctx.exception.code, 404)

    def test_path_traversal_rejected(self) -> None:
        with self.assertRaises(urllib.error.HTTPError) as ctx:
            self._get("/../../etc/passwd")
        self.assertEqual(ctx.exception.code, 404)


if __name__ == "__main__":
    unittest.main(verbosity=2)
