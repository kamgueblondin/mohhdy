#!/usr/bin/env python3
"""Fumee HTTP ASSIST-010/011/040 (stdlib, sans Docker, hors make ci)."""

from __future__ import annotations

import json
import os
import sys
import tempfile
import threading
import time
import unittest
import uuid
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
    raise RuntimeError("serveur agent injoignable: %s" % last_error)


class AgentHttpSmoke(unittest.TestCase):
    @classmethod
    def setUpClass(cls) -> None:
        os.environ.pop("ADMIN_TOKEN", None)
        os.environ.pop("MOHHDY_AGENT_DATA", None)
        cls.httpd = server.make_server("127.0.0.1", 0)
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
        self.httpd.store.clear()

    def _get(self, path: str, headers: dict | None = None):
        req = urllib.request.Request(self.base + path, headers=headers or {})
        return urllib.request.urlopen(req, timeout=2)

    def _get_json(self, path: str, headers: dict | None = None):
        with self._get(path, headers=headers) as resp:
            body = json.loads(resp.read().decode("utf-8"))
            return body, resp.status, resp.headers

    def _post_json(self, path: str, payload: dict):
        data = json.dumps(payload).encode("utf-8")
        req = urllib.request.Request(
            self.base + path,
            data=data,
            method="POST",
            headers={"Content-Type": "application/json"},
        )
        with urllib.request.urlopen(req, timeout=2) as resp:
            body = json.loads(resp.read().decode("utf-8"))
            return body, resp.status, resp.headers

    def test_health_json(self) -> None:
        body, status, headers = self._get_json("/health")
        self.assertEqual(status, 200)
        self.assertIn("application/json", headers.get("Content-Type", ""))
        self.assertEqual(body["status"], "ok")
        self.assertEqual(body["service"], "mohhdy-agent")
        self.assertEqual(body["llm"], "stub_echo")
        self.assertEqual(body["admin_auth"], "open_stub")

    def test_admin_shell(self) -> None:
        with self._get("/admin") as resp:
            self.assertEqual(resp.status, 200)
            html = resp.read().decode("utf-8")
        self.assertIn("mohhdy-sessions", html)
        self.assertIn("ADMIN_TOKEN", html)
        self.assertIn("ASSIST-040", html)

    def test_embed_js_launcher_without_secrets(self) -> None:
        with self._get("/embed.js") as resp:
            self.assertEqual(resp.status, 200)
            self.assertIn("javascript", resp.headers.get("Content-Type", ""))
            script = resp.read().decode("utf-8")
        self.assertIn("mohhdy-launcher", script)
        self.assertIn("data-mohhdy-site", script)
        self.assertIn("/api/sessions", script)
        lowered = script.lower()
        for marker in SECRET_MARKERS:
            self.assertNotIn(marker.lower(), lowered)

    def test_embed_css_served(self) -> None:
        with self._get("/embed.css") as resp:
            self.assertEqual(resp.status, 200)
            self.assertIn("text/css", resp.headers.get("Content-Type", ""))
            css = resp.read().decode("utf-8")
        self.assertIn("#mohhdy-launcher", css)

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
        self.assertIn("/api/sessions", html)

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

    def test_create_session_returns_id(self) -> None:
        body, status, headers = self._post_json(
            "/api/sessions", {"site_id": "site_public_demo"}
        )
        self.assertEqual(status, 201)
        self.assertEqual(headers.get("Access-Control-Allow-Origin"), "*")
        uuid.UUID(body["session_id"])
        self.assertEqual(body["site_id"], "site_public_demo")
        self.assertEqual(body["status"], "open")
        self.assertEqual(body["messages"], [])
        self.assertEqual(body["llm"], "stub_echo")

    def test_visitor_message_echo_stub(self) -> None:
        created, _, _ = self._post_json("/api/sessions", {"site_id": "echo_site"})
        session_id = created["session_id"]
        posted, status, _ = self._post_json(
            "/api/sessions/%s/messages" % session_id,
            {"content": "bonjour boutique"},
        )
        self.assertEqual(status, 201)
        self.assertEqual(posted["llm"], "stub_echo")
        self.assertEqual(posted["visitor_message"]["content"], "bonjour boutique")
        self.assertIn("pas un LLM de production", posted["agent_message"]["content"])
        self.assertIn("bonjour boutique", posted["agent_message"]["content"])
        fetched, _, _ = self._get_json("/api/sessions/" + session_id)
        texts = [item["content"] for item in fetched["messages"]]
        self.assertEqual(texts[0], "bonjour boutique")
        self.assertIn("bonjour boutique", texts[1])

    def test_session_isolation(self) -> None:
        a, _, _ = self._post_json("/api/sessions", {"site_id": "site_a"})
        b, _, _ = self._post_json("/api/sessions", {"site_id": "site_b"})
        self.assertNotEqual(a["session_id"], b["session_id"])
        self._post_json(
            "/api/sessions/%s/messages" % a["session_id"],
            {"content": "secret-alpha"},
        )
        self._post_json(
            "/api/sessions/%s/messages" % b["session_id"],
            {"content": "secret-beta"},
        )
        ga, _, _ = self._get_json("/api/sessions/" + a["session_id"])
        gb, _, _ = self._get_json("/api/sessions/" + b["session_id"])
        texts_a = " ".join(item["content"] for item in ga["messages"])
        texts_b = " ".join(item["content"] for item in gb["messages"])
        self.assertIn("secret-alpha", texts_a)
        self.assertNotIn("secret-beta", texts_a)
        self.assertIn("secret-beta", texts_b)
        self.assertNotIn("secret-alpha", texts_b)

    def test_visitor_cannot_list_sessions(self) -> None:
        with self.assertRaises(urllib.error.HTTPError) as ctx:
            self._get("/api/sessions")
        self.assertEqual(ctx.exception.code, 405)

    def test_admin_lists_and_reads_sessions(self) -> None:
        created, _, _ = self._post_json("/api/sessions", {"site_id": "site_admin"})
        self._post_json(
            "/api/sessions/%s/messages" % created["session_id"],
            {"content": "vu par admin"},
        )
        listing, status, _ = self._get_json("/api/admin/sessions")
        self.assertEqual(status, 200)
        ids = [row["session_id"] for row in listing["sessions"]]
        self.assertIn(created["session_id"], ids)
        other, _, _ = self._post_json("/api/sessions", {"site_id": "other_site"})
        filtered, _, _ = self._get_json("/api/admin/sessions?site_id=site_admin")
        filtered_ids = [row["session_id"] for row in filtered["sessions"]]
        self.assertIn(created["session_id"], filtered_ids)
        self.assertNotIn(other["session_id"], filtered_ids)
        detail, _, _ = self._get_json(
            "/api/admin/sessions/" + created["session_id"]
        )
        self.assertEqual(detail["session_id"], created["session_id"])
        joined = " ".join(item["content"] for item in detail["messages"])
        self.assertIn("vu par admin", joined)

    def test_unknown_session_404(self) -> None:
        missing = str(uuid.uuid4())
        with self.assertRaises(urllib.error.HTTPError) as ctx:
            self._get("/api/sessions/" + missing)
        self.assertEqual(ctx.exception.code, 404)

    def test_invalid_site_id_rejected(self) -> None:
        data = json.dumps({"site_id": "bad site"}).encode("utf-8")
        req = urllib.request.Request(
            self.base + "/api/sessions",
            data=data,
            method="POST",
            headers={"Content-Type": "application/json"},
        )
        with self.assertRaises(urllib.error.HTTPError) as ctx:
            urllib.request.urlopen(req, timeout=2)
        self.assertEqual(ctx.exception.code, 400)

    def test_optional_file_backend(self) -> None:
        previous = os.environ.get("MOHHDY_AGENT_DATA")
        httpd = None
        thread = None
        httpd2 = None
        thread2 = None
        try:
            with tempfile.TemporaryDirectory() as tmp:
                os.environ["MOHHDY_AGENT_DATA"] = tmp
                httpd = server.make_server("127.0.0.1", 0)
                base = "http://127.0.0.1:%d" % httpd.server_address[1]
                thread = threading.Thread(target=httpd.serve_forever, daemon=True)
                thread.start()
                _wait_ready(base)
                req = urllib.request.Request(
                    base + "/api/sessions",
                    data=json.dumps({"site_id": "persist_site"}).encode("utf-8"),
                    method="POST",
                    headers={"Content-Type": "application/json"},
                )
                with urllib.request.urlopen(req, timeout=2) as resp:
                    created = json.loads(resp.read().decode("utf-8"))
                session_id = created["session_id"]
                httpd.shutdown()
                httpd.server_close()
                thread.join(timeout=2)
                httpd2 = server.make_server("127.0.0.1", 0)
                base2 = "http://127.0.0.1:%d" % httpd2.server_address[1]
                thread2 = threading.Thread(target=httpd2.serve_forever, daemon=True)
                thread2.start()
                _wait_ready(base2)
                with urllib.request.urlopen(
                    base2 + "/api/sessions/" + session_id, timeout=2
                ) as resp:
                    restored = json.loads(resp.read().decode("utf-8"))
                self.assertEqual(restored["session_id"], session_id)
                self.assertEqual(restored["site_id"], "persist_site")
        finally:
            if previous is None:
                os.environ.pop("MOHHDY_AGENT_DATA", None)
            else:
                os.environ["MOHHDY_AGENT_DATA"] = previous
            if httpd2 is not None:
                httpd2.shutdown()
                httpd2.server_close()
            if thread2 is not None:
                thread2.join(timeout=2)


class AgentAdminToken(unittest.TestCase):
    TOKEN = "test-token-not-for-image"

    @classmethod
    def setUpClass(cls) -> None:
        cls._previous = os.environ.get("ADMIN_TOKEN")
        os.environ["ADMIN_TOKEN"] = cls.TOKEN
        os.environ.pop("MOHHDY_AGENT_DATA", None)
        cls.httpd = server.make_server("127.0.0.1", 0)
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
        if cls._previous is None:
            os.environ.pop("ADMIN_TOKEN", None)
        else:
            os.environ["ADMIN_TOKEN"] = cls._previous

    def _post_json(self, path: str, payload: dict):
        data = json.dumps(payload).encode("utf-8")
        req = urllib.request.Request(
            self.base + path,
            data=data,
            method="POST",
            headers={"Content-Type": "application/json"},
        )
        with urllib.request.urlopen(req, timeout=2) as resp:
            return json.loads(resp.read().decode("utf-8"))

    def test_health_reports_token_mode(self) -> None:
        with urllib.request.urlopen(self.base + "/health", timeout=2) as resp:
            body = json.loads(resp.read().decode("utf-8"))
        self.assertEqual(body["admin_auth"], "token")
        self.assertNotIn(self.TOKEN, json.dumps(body))

    def test_admin_unauthorized_without_token(self) -> None:
        with self.assertRaises(urllib.error.HTTPError) as ctx:
            urllib.request.urlopen(self.base + "/api/admin/sessions", timeout=2)
        self.assertEqual(ctx.exception.code, 401)

    def test_admin_rejects_wrong_token(self) -> None:
        req = urllib.request.Request(
            self.base + "/api/admin/sessions",
            headers={"Authorization": "Bearer wrong-token-not-for-image"},
        )
        with self.assertRaises(urllib.error.HTTPError) as ctx:
            urllib.request.urlopen(req, timeout=2)
        self.assertEqual(ctx.exception.code, 401)

    def test_admin_authorized_lists_session(self) -> None:
        created = self._post_json("/api/sessions", {"site_id": "token_site"})
        req = urllib.request.Request(
            self.base + "/api/admin/sessions",
            headers={"Authorization": "Bearer " + self.TOKEN},
        )
        with urllib.request.urlopen(req, timeout=2) as resp:
            listing = json.loads(resp.read().decode("utf-8"))
        ids = [row["session_id"] for row in listing["sessions"]]
        self.assertIn(created["session_id"], ids)

    def test_admin_status_does_not_require_token(self) -> None:
        with urllib.request.urlopen(self.base + "/api/admin/status", timeout=2) as resp:
            body = json.loads(resp.read().decode("utf-8"))
        self.assertEqual(body["auth"], "token")
        self.assertNotIn(self.TOKEN, json.dumps(body))

    def test_token_not_leaked_in_public_pages(self) -> None:
        for path in ("/embed.js", "/admin", "/health", "/demo", "/"):
            with urllib.request.urlopen(self.base + path, timeout=2) as resp:
                body = resp.read().decode("utf-8")
            self.assertNotIn(self.TOKEN, body)


if __name__ == "__main__":
    unittest.main(verbosity=2)
