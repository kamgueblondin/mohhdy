#!/usr/bin/env python3
"""Fumee OS-UI : chat central, slash, shell UI + parite agent (stdlib, hors make ci)."""

from __future__ import annotations

import json
import os
import sys
import threading
import time
import unittest
import uuid
import urllib.error
import urllib.request
from pathlib import Path

OSUI_ROOT = Path(__file__).resolve().parents[1]
sys.path.insert(0, str(OSUI_ROOT))

import server as osui_server  # noqa: E402

SECRET_MARKERS = (
    "api_key",
    "apikey",
    "secret_key",
    "BEGIN PRIVATE KEY",
    "sk-proj",
    "openai",
    "bearer ",
)


def _wait_ready(base: str) -> None:
    deadline = time.time() + 3
    last_error = None
    while time.time() < deadline:
        try:
            urllib.request.urlopen(base + "/health", timeout=0.3)
            return
        except OSError as exc:
            last_error = exc
            time.sleep(0.05)
    raise RuntimeError("serveur osui injoignable: %s" % last_error)


class OsuiHttpSmoke(unittest.TestCase):
    @classmethod
    def setUpClass(cls) -> None:
        os.environ.pop("ADMIN_TOKEN", None)
        os.environ.pop("MOHHDY_AGENT_DATA", None)
        os.environ.pop("MOHHDY_AGENT_CONFIG", None)
        os.environ.pop("MOHHDY_AGENT_KB", None)
        os.environ.pop("MOHHDY_AGENT_MODE", None)
        os.environ.pop("MOHHDY_AGENT_SITE_ID", None)
        os.environ.pop("MOHHDY_AGENT_RUNTIME", None)
        os.environ.pop("MOHHDY_AGENT_BROWSER_ENGINE", None)
        cls.httpd = osui_server.make_server("127.0.0.1", 0)
        cls.port = cls.httpd.server_address[1]
        cls.base = "http://127.0.0.1:%d" % cls.port
        cls.thread = threading.Thread(target=cls.httpd.serve_forever, daemon=True)
        cls.thread.start()
        _wait_ready(cls.base)

    @classmethod
    def tearDownClass(cls) -> None:
        cls.httpd.shutdown()
        cls.httpd.server_close()
        cls.thread.join(timeout=2)

    def setUp(self) -> None:
        self.httpd.reset_runtime()

    def _get(self, path: str, headers: dict | None = None):
        req = urllib.request.Request(self.base + path, headers=headers or {})
        return urllib.request.urlopen(req, timeout=2)

    def _get_json(self, path: str, headers: dict | None = None):
        with self._get(path, headers=headers) as resp:
            body = json.loads(resp.read().decode("utf-8"))
            return body, resp.status, resp.headers

    def _read(self, path: str) -> str:
        with self._get(path) as resp:
            return resp.read().decode("utf-8")

    def _post_json(self, path: str, payload: dict, headers: dict | None = None):
        data = json.dumps(payload).encode("utf-8")
        hdrs = {"Content-Type": "application/json"}
        if headers:
            hdrs.update(headers)
        req = urllib.request.Request(
            self.base + path, data=data, method="POST", headers=hdrs
        )
        with urllib.request.urlopen(req, timeout=2) as resp:
            body = json.loads(resp.read().decode("utf-8"))
            return body, resp.status, resp.headers

    def _post_status(self, path: str, payload: dict, headers: dict | None = None):
        data = json.dumps(payload).encode("utf-8")
        hdrs = {"Content-Type": "application/json"}
        if headers:
            hdrs.update(headers)
        req = urllib.request.Request(
            self.base + path, data=data, method="POST", headers=hdrs
        )
        try:
            with urllib.request.urlopen(req, timeout=2) as resp:
                return json.loads(resp.read().decode("utf-8")), resp.status
        except urllib.error.HTTPError as err:
            return json.loads(err.read().decode("utf-8")), err.code

    def test_shell_is_default_entry(self) -> None:
        html = self._read("/")
        self.assertIn("Mohhdy OS", html)
        self.assertIn('id="os-topbar"', html)
        self.assertIn('id="os-desktop"', html)
        self.assertIn('id="os-dock"', html)
        self.assertIn('id="os-chat"', html)
        self.assertIn('data-mode="center"', html)
        self.assertIn('id="os-slash-registry"', html)
        self.assertIn('data-slash="/help"', html)
        self.assertIn('data-slash="/browser"', html)
        self.assertIn('data-slash="/shell"', html)
        self.assertIn('data-slash="/admin"', html)
        self.assertIn('data-slash="/fs"', html)
        self.assertIn('data-drag="chat"', html)
        self.assertIn('data-pane="support"', html)
        self.assertIn('data-pane="admin"', html)
        self.assertIn('data-pane="browser"', html)
        self.assertIn('data-pane="status"', html)
        self.assertIn('data-pane="shell"', html)
        self.assertIn('data-pane="fs"', html)
        self.assertIn("llm=stub_echo", html)
        self.assertIn("phase3_complete=false", html)
        self.assertIn("us031_complete=false", html)
        self.assertIn("Simulateur DOM", html)
        self.assertIn("origin_denied", html)
        self.assertIn("Pas US-031", html)
        self.assertIn("Pas un LLM de production", html)
        self.assertIn("SE dirige par prompts", html)
        self.assertIn("Shell UI de l'instance Mohhdy", html)
        self.assertIn("Pas un root Linux", html)
        lowered = html.lower()
        for marker in SECRET_MARKERS:
            self.assertNotIn(marker.lower(), lowered)
        self.assertNotIn("admin.takeover", html)

    def test_os_assets_served(self) -> None:
        css = self._read("/os/os.css")
        self.assertIn(".os-topbar", css)
        self.assertIn(".os-window", css)
        self.assertIn('.os-chat[data-mode="center"]', css)
        self.assertIn('.os-chat[data-mode="float"]', css)
        self.assertIn("--os-chat-z: 1100", css)
        self.assertIn("cursor: grab", css)
        js = self._read("/os/os.js")
        self.assertIn("/api/sessions", js)
        self.assertIn("request_id", js)
        self.assertIn("mcp.invoice.create", js)
        self.assertIn("dom.click", js)
        self.assertIn('setChatMode("float")', js)
        self.assertIn("mohhdy.os.chat.pos", js)
        self.assertIn("window.MohhdyOS", js)
        self.assertIn("parseLine", js)
        self.assertIn('name: "help"', js)
        self.assertIn('name: "browser"', js)
        self.assertIn('name: "shell"', js)
        self.assertIn("Pas un root Linux", js)
        lowered = js.lower()
        self.assertNotIn("api_key", lowered)
        self.assertNotIn("sk-proj", lowered)

    def test_slash_registry_matches_api(self) -> None:
        html = self._read("/")
        os_json, _, _ = self._get_json("/api/os")
        slashes = [row["slash"] for row in os_json["commands"]]
        self.assertEqual(os_json["interaction"]["primary"], "center_chat")
        self.assertTrue(os_json["interaction"]["slash"])
        self.assertTrue(os_json["interaction"]["floating_chat"])
        self.assertTrue(os_json["interaction"]["chat_drag"])
        self.assertEqual(os_json["interaction"]["chat_pos_key"], "mohhdy.os.chat.pos")
        for slash in ("/help", "/browser", "/shell", "/admin", "/support", "/status", "/fs"):
            self.assertIn(slash, slashes)
            self.assertIn('data-slash="%s"' % slash, html)
        self.assertIn("shell", os_json["panes"])
        self.assertIn("chat", os_json["panes"])
        self.assertIn("fs", os_json["panes"])

    def test_os_static_traversal_rejected(self) -> None:
        with self.assertRaises(urllib.error.HTTPError) as ctx:
            self._get("/os/../server.py")
        self.assertEqual(ctx.exception.code, 404)

    def test_health_and_api_os_honest_flags(self) -> None:
        health, status, _ = self._get_json("/health")
        self.assertEqual(status, 200)
        self.assertEqual(health["status"], "ok")
        self.assertEqual(health["service"], "mohhdy-os")
        self.assertEqual(health["shell"], "osui")
        self.assertEqual(health["backend"], "mohhdy-agent")
        self.assertEqual(health["llm"], "stub_echo")
        self.assertFalse(health["phase3_complete"])
        self.assertFalse(health["us031_complete"])
        self.assertFalse(health["chromium_session_engine"])
        self.assertEqual(health["harness"], "dom_simulator")
        self.assertIn("browser-os", health["panes"])
        self.assertIn("chat", health["panes"])
        self.assertIn("shell", health["panes"])
        self.assertEqual(health["interaction"]["primary"], "center_chat")
        os_json, _, _ = self._get_json("/api/os")
        self.assertEqual(os_json["service"], "mohhdy-os")
        self.assertFalse(os_json["phase3_complete"])
        self.assertFalse(os_json["us031_complete"])
        self.assertEqual(os_json["commands"], osui_server.OS_COMMANDS)
        dumped = json.dumps(health).lower()
        self.assertNotIn("api_key", dumped)
        self.assertNotIn("sk-proj", dumped)

    def test_session_create_and_admin_list(self) -> None:
        created, status, _ = self._post_json(
            "/api/sessions", {"site_id": "osui_smoke"}
        )
        self.assertEqual(status, 201)
        uuid.UUID(created["session_id"])
        self.assertEqual(created["llm"], "stub_echo")
        listing, _, _ = self._get_json("/api/admin/sessions")
        ids = [row["session_id"] for row in listing["sessions"]]
        self.assertIn(created["session_id"], ids)

    def test_origin_denied_create_has_request_id(self) -> None:
        body, code = self._post_status(
            "/api/sessions",
            {"site_id": "osui_origin"},
            headers={"Origin": "https://evil.example"},
        )
        self.assertEqual(code, 403)
        self.assertEqual(body["error"], "origin_denied")
        self.assertTrue(body.get("request_id"))
        self.assertNotIn("session_id", body)

    def test_gesture_and_invoice_path(self) -> None:
        created, _, _ = self._post_json("/api/sessions", {"site_id": "osui_tools"})
        sid = created["session_id"]
        self._post_json(
            "/api/admin/sessions/%s/capabilities" % sid,
            {"grant": ["dom.click", "mcp.invoice.create"]},
        )
        clicked, code = self._post_status(
            "/api/sessions/%s/tools" % sid,
            {
                "tool": "dom.click",
                "origin": self.base,
                "args": {"selector": "#menu-toggle"},
            },
        )
        self.assertEqual(code, 200)
        self.assertEqual(clicked["harness"], "dom_simulator")
        invoice, code = self._post_status(
            "/api/sessions/%s/tools" % sid,
            {
                "tool": "mcp.invoice.create",
                "origin": self.base,
                "args": {"customer": "OSUI", "amount": "12.00"},
            },
        )
        self.assertEqual(code, 200)
        self.assertEqual(invoice["session_id"], sid)
        self.assertTrue(invoice.get("request_id"))

    def test_undeclared_tool_denied(self) -> None:
        created, _, _ = self._post_json("/api/sessions", {"site_id": "osui_deny"})
        sid = created["session_id"]
        denied, code = self._post_status(
            "/api/sessions/%s/tools" % sid,
            {"tool": "dom.click", "origin": self.base, "args": {"selector": "#menu-toggle"}},
        )
        self.assertEqual(code, 403)
        self.assertEqual(denied["error"], "capability_denied")
        self.assertTrue(denied.get("request_id"))

    def test_browser_flags_and_fs_and_playwright_501(self) -> None:
        browser, _, _ = self._get_json("/api/browser")
        self.assertFalse(browser["phase3_complete"])
        self.assertFalse(browser["us031_complete"])
        self.assertEqual(browser["harness"], "dom_simulator")
        fs, _, _ = self._get_json("/api/browser/fs?path=demo")
        self.assertIn("demo-app.html", json.dumps(fs))
        nav, code = self._post_status("/api/browser/navigate", {"url": "/demo-app"})
        self.assertEqual(code, 501)
        self.assertEqual(nav["error"], "optional_not_installed")
        self.assertFalse(nav["phase3_complete"])
        self.assertFalse(nav["us031_complete"])

    def test_escalate_and_takeover_same_session(self) -> None:
        created, _, _ = self._post_json("/api/sessions", {"site_id": "osui_handoff"})
        sid = created["session_id"]
        self._post_json(
            "/api/sessions/%s/escalate" % sid, {"reason": "osui smoke"}
        )
        listing, _, _ = self._get_json("/api/admin/sessions?status=waiting_human")
        ids = [row["session_id"] for row in listing["sessions"]]
        self.assertIn(sid, ids)
        taken, status, _ = self._post_json(
            "/api/admin/sessions/%s/takeover" % sid, {}
        )
        self.assertEqual(status, 200)
        self.assertEqual(taken["session_id"], sid)
        self.assertEqual(taken["status"], "human_active")

    def test_legacy_agent_pages_still_available(self) -> None:
        demo = self._read("/demo-app")
        self.assertIn("menu-toggle", demo)
        admin = self._read("/admin")
        self.assertIn("mohhdy-sessions", admin)


class OsuiStaticSafety(unittest.TestCase):
    def test_safe_osui_static_rejects_traversal(self) -> None:
        self.assertIsNone(osui_server.safe_osui_static("../server.py"))
        self.assertIsNone(osui_server.safe_osui_static("os/../os.css"))
        self.assertIsNotNone(osui_server.safe_osui_static("os.css"))
        self.assertIsNotNone(osui_server.safe_osui_static("index.html"))


if __name__ == "__main__":
    unittest.main(verbosity=2)
