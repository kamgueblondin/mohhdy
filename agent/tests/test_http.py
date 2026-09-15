#!/usr/bin/env python3
"""Fumee HTTP ASSIST-010/011/012/020/021/022/030/031/040/041 (stdlib, sans Docker, hors make ci)."""

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
        os.environ.pop("MOHHDY_AGENT_CONFIG", None)
        os.environ.pop("MOHHDY_AGENT_KB", None)
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
        self.httpd.reset_runtime()

    def _get(self, path: str, headers: dict | None = None):
        req = urllib.request.Request(self.base + path, headers=headers or {})
        return urllib.request.urlopen(req, timeout=2)

    def _get_json(self, path: str, headers: dict | None = None):
        with self._get(path, headers=headers) as resp:
            body = json.loads(resp.read().decode("utf-8"))
            return body, resp.status, resp.headers

    def _post_json(self, path: str, payload: dict, headers: dict | None = None):
        data = json.dumps(payload).encode("utf-8")
        hdrs = {"Content-Type": "application/json"}
        if headers:
            hdrs.update(headers)
        req = urllib.request.Request(
            self.base + path,
            data=data,
            method="POST",
            headers=hdrs,
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
            self.base + path,
            data=data,
            method="POST",
            headers=hdrs,
        )
        try:
            with urllib.request.urlopen(req, timeout=2) as resp:
                return json.loads(resp.read().decode("utf-8")), resp.status
        except urllib.error.HTTPError as err:
            return json.loads(err.read().decode("utf-8")), err.code

    def test_no_playwright_dependency(self) -> None:
        self.assertNotIn("playwright", sys.modules)
        source = (ROOT / "server.py").read_text(encoding="utf-8")
        self.assertNotIn("playwright", source.lower())
        tools_src = (ROOT / "tools.py").read_text(encoding="utf-8")
        self.assertNotIn("from playwright", tools_src)
        self.assertNotIn("import playwright", tools_src)

    def test_health_json(self) -> None:
        body, status, headers = self._get_json("/health")
        self.assertEqual(status, 200)
        self.assertIn("application/json", headers.get("Content-Type", ""))
        self.assertEqual(body["status"], "ok")
        self.assertEqual(body["service"], "mohhdy-agent")
        self.assertEqual(body["llm"], "stub_echo")
        self.assertEqual(body["admin_auth"], "open_stub")
        self.assertFalse(body["kb_loaded"])
        self.assertEqual(body["harness"], "dom_simulator")
        self.assertNotIn("ADMIN_TOKEN", json.dumps(body))
        self.assertNotIn("acl.", json.dumps(body))

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
        self.assertIn("Parler a un humain", script)
        self.assertIn("/escalate", script)
        self.assertIn("human_active", script)
        self.assertNotIn("admin.takeover", script)
        self.assertNotIn("acl.", script)
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
        self.assertIn("/demo-app", html)
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
        self.assertIn("chat.reply", body["capabilities"])
        self.assertIn("site.explain", body["capabilities"])
        self.assertIn("session.escalate", body["capabilities"])
        dumped = json.dumps(body)
        self.assertNotIn("admin.observe", dumped)
        self.assertNotIn("admin.takeover", dumped)
        self.assertNotIn("acl.", dumped)
        self.assertNotIn("internal.", dumped)

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
        self.assertIn("admin.takeover", detail["capabilities"])

    def test_explain_without_kb_refuses_honestly(self) -> None:
        created, _, _ = self._post_json("/api/sessions", {"site_id": "empty_kb"})
        posted, status, _ = self._post_json(
            "/api/sessions/%s/messages" % created["session_id"],
            {"content": "comment ca marche ?"},
        )
        self.assertEqual(status, 201)
        self.assertEqual(posted["llm"], "stub_refusal")
        self.assertIn("Aucune base de connaissance autorisee", posted["agent_message"]["content"])
        self.assertNotIn("Vous avez dit", posted["agent_message"]["content"])

    def test_public_session_hides_admin_caps_and_acl(self) -> None:
        created, _, _ = self._post_json("/api/sessions", {"site_id": "caps_site"})
        fetched, _, _ = self._get_json("/api/sessions/" + created["session_id"])
        dumped = json.dumps(fetched)
        self.assertNotIn("admin.observe", dumped)
        self.assertNotIn("admin.takeover", dumped)
        self.assertNotIn("acl.", dumped)
        self.assertNotIn("/etc/", dumped)

    def test_revoke_chat_reply_refuses(self) -> None:
        created, _, _ = self._post_json("/api/sessions", {"site_id": "revoke_chat"})
        sid = created["session_id"]
        patched, status, _ = self._post_json(
            "/api/admin/sessions/%s/capabilities" % sid,
            {"revoke": ["chat.reply"]},
        )
        self.assertEqual(status, 200)
        self.assertNotIn("chat.reply", patched["capabilities"])
        posted, _, _ = self._post_json(
            "/api/sessions/%s/messages" % sid,
            {"content": "bonjour quand meme"},
        )
        self.assertEqual(posted["llm"], "stub_refusal")
        self.assertIn("chat.reply", posted["agent_message"]["content"])
        self.assertNotIn("bonjour quand meme", posted["agent_message"]["content"])

    def test_revoked_tool_refused_and_escalates(self) -> None:
        created, _, _ = self._post_json("/api/sessions", {"site_id": "tools_site"})
        sid = created["session_id"]
        denied, code = self._post_status(
            "/api/sessions/%s/tools" % sid, {"tool": "dom.click"}
        )
        self.assertEqual(code, 403)
        self.assertEqual(denied["error"], "capability_denied")
        self.assertEqual(denied["tool"], "dom.click")
        self.assertEqual(denied["session_status"], "waiting_human")
        fetched, _, _ = self._get_json("/api/sessions/" + sid)
        self.assertEqual(fetched["status"], "waiting_human")
        joined = " ".join(item["content"] for item in fetched["messages"])
        self.assertIn("absent de l'allowlist", joined)
        listing, _, _ = self._get_json("/api/admin/sessions?status=waiting_human")
        ids = [row["session_id"] for row in listing["sessions"]]
        self.assertIn(sid, ids)

    def test_grant_then_revoke_gesture_tool(self) -> None:
        created, _, _ = self._post_json("/api/sessions", {"site_id": "grant_revoke"})
        sid = created["session_id"]
        self._post_json(
            "/api/admin/sessions/%s/capabilities" % sid,
            {"grant": ["dom.click"]},
        )
        origin = self.base
        clicked, code = self._post_status(
            "/api/sessions/%s/tools" % sid,
            {"tool": "dom.click", "origin": origin, "args": {"selector": "#menu-toggle"}},
        )
        self.assertEqual(code, 200)
        self.assertTrue(clicked["ok"])
        self.assertEqual(clicked["harness"], "dom_simulator")
        self.assertTrue(clicked["result"]["dom"]["menu_open"])
        self._post_json(
            "/api/admin/sessions/%s/capabilities" % sid,
            {"revoke": ["dom.click"]},
        )
        denied, code = self._post_status(
            "/api/sessions/%s/tools" % sid,
            {"tool": "dom.click", "origin": origin, "args": {"selector": "#menu-toggle"}},
        )
        self.assertEqual(code, 403)
        self.assertEqual(denied["error"], "capability_denied")
        self.assertTrue(denied["escalate"])

    def test_demo_app_page_has_menu_and_form(self) -> None:
        with self._get("/demo-app") as resp:
            self.assertEqual(resp.status, 200)
            html = resp.read().decode("utf-8")
        self.assertIn("id=\"menu-toggle\"", html)
        self.assertIn("id=\"invoice-customer\"", html)
        self.assertIn("simulateur DOM", html)
        self.assertIn("Pas Chromium", html)
        self.assertIn("mcp.invoice.create", html)
        lowered = html.lower()
        self.assertNotIn("api_key", lowered)

    def test_gesture_updates_demo_state(self) -> None:
        created, _, _ = self._post_json("/api/sessions", {"site_id": "gesture_site"})
        sid = created["session_id"]
        self._post_json(
            "/api/admin/sessions/%s/capabilities" % sid,
            {"grant": ["dom.click", "dom.type", "pointer.move"]},
        )
        origin = self.base
        self._post_json(
            "/api/sessions/%s/tools" % sid,
            {"tool": "dom.click", "origin": origin, "args": {"selector": "#menu-toggle"}},
        )
        state, status, _ = self._get_json("/api/demo-app/state")
        self.assertEqual(status, 200)
        self.assertEqual(state["harness"], "dom_simulator")
        self.assertTrue(state["menu_open"])
        self._post_json(
            "/api/sessions/%s/tools" % sid,
            {
                "tool": "dom.type",
                "origin": origin,
                "args": {"selector": "#invoice-customer", "text": "Ada Lovelace"},
            },
        )
        self._post_json(
            "/api/sessions/%s/tools" % sid,
            {"tool": "pointer.move", "origin": origin, "args": {"x": 40, "y": 80}},
        )
        state, _, _ = self._get_json("/api/demo-app/state")
        self.assertEqual(state["form"]["customer"], "Ada Lovelace")
        self.assertEqual(state["pointer"]["x"], 40)
        self.assertEqual(state["pointer"]["y"], 80)
        self.assertTrue(state["last_request_id"])

    def test_foreign_origin_refused_with_request_id(self) -> None:
        created, _, _ = self._post_json("/api/sessions", {"site_id": "origin_site"})
        sid = created["session_id"]
        self._post_json(
            "/api/admin/sessions/%s/capabilities" % sid,
            {"grant": ["dom.click"]},
        )
        before, _, _ = self._get_json("/api/demo-app/state")
        denied, code = self._post_status(
            "/api/sessions/%s/tools" % sid,
            {
                "tool": "dom.click",
                "origin": "https://evil.example",
                "args": {"selector": "#menu-toggle"},
            },
        )
        self.assertEqual(code, 403)
        self.assertEqual(denied["error"], "origin_denied")
        self.assertEqual(denied["origin"], "https://evil.example")
        self.assertTrue(denied["request_id"])
        uuid.UUID(denied["request_id"])
        after, _, _ = self._get_json("/api/demo-app/state")
        self.assertEqual(after["menu_open"], before["menu_open"])
        journal, _, _ = self._get_json("/api/admin/journal")
        ids = [row["request_id"] for row in journal["journal"]]
        self.assertIn(denied["request_id"], ids)
        match = [row for row in journal["journal"] if row["request_id"] == denied["request_id"]]
        self.assertEqual(match[0]["outcome"], "origin_denied")
        fetched, _, _ = self._get_json("/api/sessions/" + sid)
        self.assertEqual(fetched["status"], "waiting_human")
        joined = " ".join(item["content"] for item in fetched["messages"])
        self.assertIn(denied["request_id"], joined)

    def test_invoice_create_same_session_then_revoke(self) -> None:
        created, _, _ = self._post_json("/api/sessions", {"site_id": "invoice_site"})
        sid = created["session_id"]
        denied, code = self._post_status(
            "/api/sessions/%s/tools" % sid,
            {
                "tool": "mcp.invoice.create",
                "origin": self.base,
                "args": {"customer": "Ada", "amount": "42.00"},
            },
        )
        self.assertEqual(code, 403)
        self.assertEqual(denied["error"], "capability_denied")
        self.assertTrue(denied["escalate"])
        self.assertEqual(denied["session_status"], "waiting_human")
        listing, _, _ = self._get_json("/api/demo-app/invoices")
        self.assertEqual(listing["invoices"], [])
        self._post_json(
            "/api/admin/sessions/%s/capabilities" % sid,
            {"grant": ["mcp.invoice.create"]},
        )
        created_inv, code = self._post_status(
            "/api/sessions/%s/tools" % sid,
            {
                "tool": "mcp.invoice.create",
                "origin": self.base,
                "args": {"customer": "Ada", "amount": "42.00"},
            },
        )
        self.assertEqual(code, 200)
        invoice = created_inv["result"]["invoice"]
        self.assertEqual(invoice["session_id"], sid)
        self.assertEqual(invoice["customer"], "Ada")
        self.assertEqual(invoice["amount"], "42.00")
        listed, _, _ = self._get_json("/api/demo-app/invoices?session_id=" + sid)
        ids = [row["invoice_id"] for row in listed["invoices"]]
        self.assertIn(invoice["invoice_id"], ids)
        self._post_json(
            "/api/admin/sessions/%s/capabilities" % sid,
            {"revoke": ["mcp.invoice.create"]},
        )
        again, code = self._post_status(
            "/api/sessions/%s/tools" % sid,
            {
                "tool": "mcp.invoice.create",
                "origin": self.base,
                "args": {"customer": "Ada", "amount": "99.00"},
            },
        )
        self.assertEqual(code, 403)
        self.assertEqual(again["error"], "capability_denied")
        listed2, _, _ = self._get_json("/api/demo-app/invoices?session_id=" + sid)
        self.assertEqual(len(listed2["invoices"]), 1)

    def test_undeclared_mcp_tool_refused(self) -> None:
        created, _, _ = self._post_json("/api/sessions", {"site_id": "mcp_absent"})
        sid = created["session_id"]
        self._post_json(
            "/api/admin/sessions/%s/capabilities" % sid,
            {"grant": ["mcp.not_registered"]},
        )
        denied, code = self._post_status(
            "/api/sessions/%s/tools" % sid,
            {"tool": "mcp.not_registered", "origin": self.base},
        )
        self.assertEqual(code, 403)
        self.assertEqual(denied["error"], "tool_undeclared")
        self.assertTrue(denied["request_id"])
        self.assertTrue(denied["escalate"])

    def test_invoice_from_form_state(self) -> None:
        created, _, _ = self._post_json("/api/sessions", {"site_id": "form_inv"})
        sid = created["session_id"]
        self._post_json(
            "/api/admin/sessions/%s/capabilities" % sid,
            {"grant": ["dom.type", "mcp.invoice.create"]},
        )
        origin = self.base
        self._post_json(
            "/api/sessions/%s/tools" % sid,
            {
                "tool": "dom.type",
                "origin": origin,
                "args": {"selector": "#invoice-customer", "text": "Grace Hopper"},
            },
        )
        self._post_json(
            "/api/sessions/%s/tools" % sid,
            {
                "tool": "dom.type",
                "origin": origin,
                "args": {"selector": "#invoice-amount", "text": "12.50"},
            },
        )
        created_inv, code = self._post_status(
            "/api/sessions/%s/tools" % sid,
            {"tool": "mcp.invoice.create", "origin": origin, "args": {}},
        )
        self.assertEqual(code, 200)
        invoice = created_inv["result"]["invoice"]
        self.assertEqual(invoice["session_id"], sid)
        self.assertEqual(invoice["customer"], "Grace Hopper")
        self.assertEqual(invoice["amount"], "12.50")

    def test_admin_catalog_lists_gestures_and_invoice(self) -> None:
        body, status, _ = self._get_json("/api/admin/capabilities")
        self.assertEqual(status, 200)
        names = [row["name"] for row in body["capabilities"]]
        self.assertIn("dom.click", names)
        self.assertIn("mcp.invoice.create", names)
        kinds = {row["name"]: row["kind"] for row in body["capabilities"]}
        self.assertEqual(kinds["dom.click"], "gesture")
        self.assertEqual(kinds["mcp.invoice.create"], "mcp")
        self.assertEqual(body["harness"], "dom_simulator")
        self.assertIn("mcp.invoice.create", body["declared_tools"])

    def test_admin_shell_has_queue_and_takeover(self) -> None:
        with self._get("/admin") as resp:
            html = resp.read().decode("utf-8")
        self.assertIn("File humain", html)
        self.assertIn("Prendre la main", html)
        self.assertIn("/takeover", html)
        self.assertIn("mohhdy-journal", html)

    def test_internal_acl_prefix_rejected(self) -> None:
        created, _, _ = self._post_json("/api/sessions", {"site_id": "acl_site"})
        body, code = self._post_status(
            "/api/admin/sessions/%s/capabilities" % created["session_id"],
            {"grant": ["acl.internal.read"]},
        )
        self.assertEqual(code, 400)
        self.assertEqual(body["error"], "bad_request")

    def test_visitor_escalate_waiting_human(self) -> None:
        created, _, _ = self._post_json("/api/sessions", {"site_id": "esc_site"})
        sid = created["session_id"]
        result, status, _ = self._post_json(
            "/api/sessions/%s/escalate" % sid, {"reason": "je veux un humain"}
        )
        self.assertEqual(status, 200)
        self.assertEqual(result["status"], "waiting_human")
        self.assertNotIn("admin.takeover", json.dumps(result))
        queue, _, _ = self._get_json("/api/admin/sessions?status=waiting_human")
        ids = [row["session_id"] for row in queue["sessions"]]
        self.assertIn(sid, ids)

    def test_escalate_revoked_is_denied(self) -> None:
        created, _, _ = self._post_json("/api/sessions", {"site_id": "no_esc"})
        sid = created["session_id"]
        self._post_json(
            "/api/admin/sessions/%s/capabilities" % sid,
            {"revoke": ["session.escalate"]},
        )
        body, code = self._post_status("/api/sessions/%s/escalate" % sid, {})
        self.assertEqual(code, 403)
        self.assertEqual(body["capability"], "session.escalate")
        fetched, _, _ = self._get_json("/api/sessions/" + sid)
        self.assertEqual(fetched["status"], "open")

    def test_handoff_same_session_stops_auto_reply(self) -> None:
        created, _, _ = self._post_json("/api/sessions", {"site_id": "handoff_site"})
        sid = created["session_id"]
        self._post_json(
            "/api/sessions/%s/messages" % sid,
            {"content": "bonjour avant humain"},
        )
        self._post_json("/api/sessions/%s/escalate" % sid, {})
        taken, status, _ = self._post_json(
            "/api/admin/sessions/%s/takeover" % sid, {}
        )
        self.assertEqual(status, 200)
        self.assertEqual(taken["session_id"], sid)
        self.assertEqual(taken["status"], "human_active")
        self.assertTrue(taken["handoff"])
        human, hstatus, _ = self._post_json(
            "/api/admin/sessions/%s/messages" % sid,
            {"content": "Bonjour, je suis un humain."},
        )
        self.assertEqual(hstatus, 201)
        self.assertEqual(human["human_message"]["role"], "human")
        self.assertEqual(human["human_message"]["speaker"], "human")
        visitor_view, _, _ = self._get_json("/api/sessions/" + sid)
        self.assertEqual(visitor_view["session_id"], sid)
        self.assertEqual(visitor_view["status"], "human_active")
        speakers = [item["speaker"] for item in visitor_view["messages"]]
        self.assertIn("visitor", speakers)
        self.assertIn("human", speakers)
        texts = [item["content"] for item in visitor_view["messages"]]
        self.assertIn("Bonjour, je suis un humain.", texts)
        dumped = json.dumps(visitor_view)
        self.assertNotIn("admin.takeover", dumped)
        after, status, _ = self._post_json(
            "/api/sessions/%s/messages" % sid,
            {"content": "merci humain"},
        )
        self.assertEqual(status, 201)
        self.assertFalse(after["auto_reply"])
        self.assertIsNone(after["agent_message"])
        self.assertEqual(after["visitor_message"]["content"], "merci humain")
        refetch, _, _ = self._get_json("/api/sessions/" + sid)
        agent_after = [
            item
            for item in refetch["messages"]
            if item["role"] == "agent"
            and item["request_id"] == after["request_id"]
        ]
        self.assertEqual(agent_after, [])

    def test_takeover_revoked_is_denied(self) -> None:
        created, _, _ = self._post_json("/api/sessions", {"site_id": "no_take"})
        sid = created["session_id"]
        self._post_json(
            "/api/admin/sessions/%s/capabilities" % sid,
            {"revoke": ["admin.takeover"]},
        )
        body, code = self._post_status(
            "/api/admin/sessions/%s/takeover" % sid, {}
        )
        self.assertEqual(code, 403)
        self.assertEqual(body["capability"], "admin.takeover")

    def test_site_capability_limit_applies_to_new_session(self) -> None:
        self._post_json(
            "/api/admin/sites/limited_site/capabilities",
            {"revoke": ["site.explain"]},
        )
        created, _, _ = self._post_json("/api/sessions", {"site_id": "limited_site"})
        self.assertNotIn("site.explain", created["capabilities"])

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
                grant = urllib.request.Request(
                    base + "/api/admin/sessions/%s/capabilities" % session_id,
                    data=json.dumps({"grant": ["mcp.invoice.create"]}).encode("utf-8"),
                    method="POST",
                    headers={"Content-Type": "application/json"},
                )
                with urllib.request.urlopen(grant, timeout=2):
                    pass
                inv_req = urllib.request.Request(
                    base + "/api/sessions/%s/tools" % session_id,
                    data=json.dumps(
                        {
                            "tool": "mcp.invoice.create",
                            "origin": base,
                            "args": {"customer": "Persist", "amount": "7.00"},
                        }
                    ).encode("utf-8"),
                    method="POST",
                    headers={"Content-Type": "application/json"},
                )
                with urllib.request.urlopen(inv_req, timeout=2) as resp:
                    created_inv = json.loads(resp.read().decode("utf-8"))
                invoice_id = created_inv["result"]["invoice"]["invoice_id"]
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
                with urllib.request.urlopen(
                    base2 + "/api/demo-app/invoices?session_id=" + session_id,
                    timeout=2,
                ) as resp:
                    invoices = json.loads(resp.read().decode("utf-8"))
                ids = [row["invoice_id"] for row in invoices["invoices"]]
                self.assertIn(invoice_id, ids)
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
        os.environ.pop("MOHHDY_AGENT_CONFIG", None)
        os.environ.pop("MOHHDY_AGENT_KB", None)
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

    def test_takeover_requires_token(self) -> None:
        created = self._post_json("/api/sessions", {"site_id": "token_take"})
        req = urllib.request.Request(
            self.base + "/api/admin/sessions/%s/takeover" % created["session_id"],
            data=b"{}",
            method="POST",
            headers={"Content-Type": "application/json"},
        )
        with self.assertRaises(urllib.error.HTTPError) as ctx:
            urllib.request.urlopen(req, timeout=2)
        self.assertEqual(ctx.exception.code, 401)


class AgentKbGrounding(unittest.TestCase):
    @classmethod
    def setUpClass(cls) -> None:
        cls._tmp = tempfile.TemporaryDirectory()
        cfg = Path(cls._tmp.name) / "config.json"
        cfg.write_text(
            json.dumps(
                {
                    "default_capabilities": [
                        "chat.reply",
                        "site.explain",
                        "session.escalate",
                        "admin.observe",
                        "admin.takeover",
                    ],
                    "sites": {
                        "kb_shop": {
                            "kb": [
                                {
                                    "id": "parcours",
                                    "title": "Parcours boutique",
                                    "text": (
                                        "Comment ca marche : ouvrez la bulle, "
                                        "l'offre est un support de demonstration "
                                        "sans paiement. Limites : pas d'actes navigateur."
                                    ),
                                }
                            ]
                        }
                    },
                }
            ),
            encoding="utf-8",
        )
        cls._prev_cfg = os.environ.get("MOHHDY_AGENT_CONFIG")
        cls._prev_kb = os.environ.get("MOHHDY_AGENT_KB")
        os.environ["MOHHDY_AGENT_CONFIG"] = str(cfg)
        os.environ.pop("MOHHDY_AGENT_KB", None)
        os.environ.pop("ADMIN_TOKEN", None)
        os.environ.pop("MOHHDY_AGENT_DATA", None)
        cls.httpd = server.make_server("127.0.0.1", 0)
        cls.base = "http://127.0.0.1:%d" % cls.httpd.server_address[1]
        cls.thread = threading.Thread(target=cls.httpd.serve_forever, daemon=True)
        cls.thread.start()
        _wait_ready(cls.base)

    @classmethod
    def tearDownClass(cls) -> None:
        cls.httpd.shutdown()
        cls.httpd.server_close()
        cls.thread.join(timeout=2)
        if cls._prev_cfg is None:
            os.environ.pop("MOHHDY_AGENT_CONFIG", None)
        else:
            os.environ["MOHHDY_AGENT_CONFIG"] = cls._prev_cfg
        if cls._prev_kb is None:
            os.environ.pop("MOHHDY_AGENT_KB", None)
        else:
            os.environ["MOHHDY_AGENT_KB"] = cls._prev_kb
        cls._tmp.cleanup()

    def setUp(self) -> None:
        self.httpd.store.clear()

    def _post_json(self, path: str, payload: dict):
        data = json.dumps(payload).encode("utf-8")
        req = urllib.request.Request(
            self.base + path,
            data=data,
            method="POST",
            headers={"Content-Type": "application/json"},
        )
        with urllib.request.urlopen(req, timeout=2) as resp:
            return json.loads(resp.read().decode("utf-8")), resp.status

    def test_health_reports_kb_loaded(self) -> None:
        with urllib.request.urlopen(self.base + "/health", timeout=2) as resp:
            body = json.loads(resp.read().decode("utf-8"))
        self.assertTrue(body["kb_loaded"])
        dumped = json.dumps(body)
        self.assertNotIn(self._tmp.name, dumped)
        self.assertNotIn("MOHHDY_AGENT_CONFIG", dumped)

    def test_explain_is_grounded_in_kb(self) -> None:
        created, _ = self._post_json("/api/sessions", {"site_id": "kb_shop"})
        posted, status = self._post_json(
            "/api/sessions/%s/messages" % created["session_id"],
            {"content": "comment ca marche ?"},
        )
        self.assertEqual(status, 201)
        self.assertEqual(posted["llm"], "stub_kb")
        text = posted["agent_message"]["content"]
        self.assertIn("base autorisee", text)
        self.assertIn("support de demonstration", text)
        self.assertIn("pas d'actes navigateur", text)
        self.assertNotIn("Vous avez dit : comment ca marche", text)

    def test_unrelated_question_still_uses_kb(self) -> None:
        created, _ = self._post_json("/api/sessions", {"site_id": "kb_shop"})
        posted, _ = self._post_json(
            "/api/sessions/%s/messages" % created["session_id"],
            {"content": "quel est le tarif secret invente ?"},
        )
        self.assertEqual(posted["llm"], "stub_kb")
        text = posted["agent_message"]["content"]
        self.assertIn("base autorisee", text)
        self.assertNotIn("tarif secret invente", text)
        self.assertNotIn("Vous avez dit", text)

    def test_revoke_explain_does_not_leak_kb(self) -> None:
        created, _ = self._post_json("/api/sessions", {"site_id": "kb_shop"})
        sid = created["session_id"]
        data = json.dumps({"revoke": ["site.explain"]}).encode("utf-8")
        req = urllib.request.Request(
            self.base + "/api/admin/sessions/%s/capabilities" % sid,
            data=data,
            method="POST",
            headers={"Content-Type": "application/json"},
        )
        with urllib.request.urlopen(req, timeout=2):
            pass
        posted, _ = self._post_json(
            "/api/sessions/%s/messages" % sid,
            {"content": "comment ca marche ?"},
        )
        self.assertEqual(posted["llm"], "stub_refusal")
        self.assertIn("site.explain", posted["agent_message"]["content"])
        self.assertNotIn("support de demonstration", posted["agent_message"]["content"])


if __name__ == "__main__":
    unittest.main(verbosity=2)
