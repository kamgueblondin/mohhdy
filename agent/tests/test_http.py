#!/usr/bin/env python3
"""Fumee HTTP ASSIST-010..022/030/031/040/041/051/053/060/061 + 013 (stdlib, sans Docker, hors make ci)."""

from __future__ import annotations

import json
import os
import shutil
import subprocess
import sys
import tempfile
import threading
import time
import unittest
import uuid
import urllib.error
import urllib.parse
import urllib.request
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
sys.path.insert(0, str(ROOT))

import server  # noqa: E402
import tools as agent_tools  # noqa: E402
import browser_engine as agent_browser  # noqa: E402

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


class OriginHelpers(unittest.TestCase):
    def test_extract_document_origin_prefers_header(self) -> None:
        got = agent_tools.extract_document_origin(
            "https://shop.example",
            "https://evil.example/page",
            "http://127.0.0.1:8080",
            "http://127.0.0.1:8080",
        )
        self.assertEqual(got, "https://shop.example")

    def test_extract_document_origin_referer_then_self(self) -> None:
        from_referer = agent_tools.extract_document_origin(
            "",
            "https://shop.example/panier?x=1",
            "",
            "http://127.0.0.1:8080",
        )
        self.assertEqual(from_referer, "https://shop.example")
        from_self = agent_tools.extract_document_origin(
            "", "", "", "http://127.0.0.1:8080"
        )
        self.assertEqual(from_self, "http://127.0.0.1:8080")

    def test_extract_null_origin_is_empty(self) -> None:
        self.assertEqual(
            agent_tools.extract_document_origin("null", "", "", "http://127.0.0.1:8"),
            "",
        )

    def test_self_origin_allowed_and_foreign_denied(self) -> None:
        self.assertTrue(
            agent_tools.origin_allowed(
                "http://127.0.0.1:8080", ["self"], "http://127.0.0.1:8080"
            )
        )
        self.assertFalse(
            agent_tools.origin_allowed(
                "https://evil.example", ["self"], "http://127.0.0.1:8080"
            )
        )
        self.assertTrue(
            agent_tools.origin_allowed(
                "https://shop.example",
                ["https://shop.example"],
                "http://127.0.0.1:8080",
            )
        )

    def test_origin_binding_token_self_vs_url(self) -> None:
        self.assertEqual(
            agent_tools.origin_binding_token(
                "http://127.0.0.1:8080",
                "http://127.0.0.1:8080",
                ["self"],
            ),
            "self",
        )
        self.assertEqual(
            agent_tools.origin_binding_token(
                "https://shop.example",
                "http://127.0.0.1:8080",
                ["https://shop.example"],
            ),
            "https://shop.example",
        )


class AgentHttpSmoke(unittest.TestCase):
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

    def _get_status(self, path: str, headers: dict | None = None):
        req = urllib.request.Request(self.base + path, headers=headers or {})
        try:
            with urllib.request.urlopen(req, timeout=2) as resp:
                return json.loads(resp.read().decode("utf-8")), resp.status
        except urllib.error.HTTPError as err:
            return json.loads(err.read().decode("utf-8")), err.code

    def test_no_playwright_dependency(self) -> None:
        self.assertNotIn("playwright", sys.modules)
        for name in ("server.py", "tools.py", "browser_fs.py"):
            source = (ROOT / name).read_text(encoding="utf-8")
            self.assertNotIn("from playwright", source)
            self.assertNotIn("import playwright", source)
        engine_src = (ROOT / "browser_engine.py").read_text(encoding="utf-8")
        header, _sep, _tail = engine_src.partition("def _load_sync_playwright")
        self.assertTrue(_sep)
        self.assertNotIn("from playwright", header)
        self.assertNotIn("import playwright", header)
        self.assertEqual(agent_browser.engine_label(), "optional_not_installed")
        self.assertNotIn("playwright", sys.modules)

    def test_install_script_copies_browser_engine(self) -> None:
        script = (ROOT / "scripts" / "install.sh").read_text(encoding="utf-8")
        self.assertIn("browser_engine.py", script)
        self.assertIn('"${AGENT_DIR}"/*.py', script)
        with tempfile.TemporaryDirectory() as tmp:
            dest = Path(tmp)
            for py in ROOT.glob("*.py"):
                shutil.copy(py, dest / py.name)
            self.assertTrue((dest / "browser_engine.py").is_file())
            self.assertTrue((dest / "browser_fs.py").is_file())
            env = dict(os.environ)
            env.pop("MOHHDY_AGENT_BROWSER_ENGINE", None)
            code = (
                "import importlib.util, sys\n"
                "spec = importlib.util.spec_from_file_location('server', 'server.py')\n"
                "mod = importlib.util.module_from_spec(spec)\n"
                "spec.loader.exec_module(mod)\n"
                "assert 'playwright' not in sys.modules\n"
            )
            proc = subprocess.run(
                [sys.executable, "-c", code],
                cwd=str(dest),
                env=env,
                capture_output=True,
                text=True,
                timeout=8,
            )
            self.assertEqual(proc.returncode, 0, proc.stderr or proc.stdout)

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
        self.assertEqual(body["deployment_mode"], "self_host")
        self.assertEqual(body["runtime"], "docker")
        self.assertEqual(body["browser_engine"], "optional_not_installed")
        self.assertFalse(body["phase3_complete"])
        self.assertFalse(body["us031_complete"])
        self.assertTrue(body["browser_fs"])
        self.assertEqual(body["billing"], "none")
        self.assertEqual(body["default_site_id"], "unspecified")
        self.assertFalse(body["quota"]["billing"])
        self.assertFalse(body["quota"]["enforced"])
        self.assertEqual(body["quota"]["max_sessions"], 500)
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
        self.assertIn("window.location.origin", script)
        self.assertIn("origin_denied", script)
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
        self.assertIn("/browser", html)
        self.assertIn("/browser/fs", html)
        self.assertIn("/api/sessions", html)

    def test_browser_view_page(self) -> None:
        with self._get("/browser") as resp:
            self.assertEqual(resp.status, 200)
            html = resp.read().decode("utf-8")
        self.assertIn("ASSIST-060", html)
        self.assertIn("simulateur DOM", html)
        self.assertIn("US-031", html)
        self.assertIn("Playwright", html)
        self.assertIn("profil optionnel", html)
        self.assertIn("/api/browser/navigate", html)
        self.assertIn("/demo-app", html)
        self.assertIn("/browser/fs", html)
        lowered = html.lower()
        self.assertNotIn("api_key", lowered)

    def test_browser_fs_page(self) -> None:
        with self._get("/browser/fs") as resp:
            self.assertEqual(resp.status, 200)
            html = resp.read().decode("utf-8")
        self.assertIn("ASSIST-061", html)
        self.assertIn("/api/browser/fs", html)
        self.assertIn("MOHHDY_AGENT_RUNTIME", html)
        self.assertIn("US-031", html)
        self.assertIn("ADMIN_TOKEN", html)

    def test_api_browser_status_not_us031(self) -> None:
        body, status, _ = self._get_json("/api/browser")
        self.assertEqual(status, 200)
        self.assertEqual(body["runtime"], "docker")
        self.assertEqual(body["harness"], "dom_simulator")
        self.assertEqual(body["browser_engine"], "optional_not_installed")
        self.assertFalse(body["phase3_complete"])
        self.assertFalse(body["us031_complete"])
        self.assertTrue(body["browser_fs"])
        self.assertEqual(body["urls"]["view"], "/browser")
        self.assertEqual(body["urls"]["fs"], "/browser/fs")
        self.assertEqual(body["urls"]["navigate"], "/api/browser/navigate")
        self.assertEqual(body["urls"]["screenshot"], "/api/browser/screenshot")
        self.assertEqual(body["session_tools_harness"], "dom_simulator")
        self.assertEqual(body["page"]["url"], "")
        self.assertIn("dom", body)
        self.assertEqual(body["dom"]["harness"], "dom_simulator")
        names = [row["name"] for row in body["roots"]]
        self.assertIn("demo", names)
        self.assertNotIn("acl.", json.dumps(body))

    def test_browser_navigate_without_engine_is_501(self) -> None:
        body, status = self._post_status(
            "/api/browser/navigate", {"url": "/demo-app"}
        )
        self.assertEqual(status, 501)
        self.assertEqual(body["error"], "optional_not_installed")
        self.assertEqual(body["browser_engine"], "optional_not_installed")
        self.assertFalse(body["phase3_complete"])
        self.assertFalse(body["us031_complete"])
        self.assertNotIn("acl.", json.dumps(body))

    def test_browser_screenshot_without_engine_is_501(self) -> None:
        body, status = self._get_status("/api/browser/screenshot")
        self.assertEqual(status, 501)
        self.assertEqual(body["error"], "optional_not_installed")

    def test_browser_fs_list_and_read_demo(self) -> None:
        roots, status = self._get_status("/api/browser/fs")
        self.assertEqual(status, 200)
        names = [row["name"] for row in roots["entries"]]
        self.assertIn("demo", names)
        listing, status = self._get_status("/api/browser/fs?path=demo")
        self.assertEqual(status, 200)
        self.assertEqual(listing["type"], "dir")
        files = [row["name"] for row in listing["entries"]]
        self.assertIn("demo-app.html", files)
        self.assertIn("browser.html", files)
        self.assertNotIn("server.py", files)
        read, status = self._get_status("/api/browser/fs?path=demo/demo-app.html")
        self.assertEqual(status, 200)
        self.assertEqual(read["type"], "file")
        self.assertIn("simulateur DOM", read["content"])
        listed_read, status = self._get_status(
            "/api/browser/fs/read?path=demo/browser.html"
        )
        self.assertEqual(status, 200)
        self.assertIn("ASSIST-060", listed_read["content"])

    def test_browser_fs_traversal_refused(self) -> None:
        cases = (
            "../server.py",
            "demo/../server.py",
            "demo/../../server.py",
            "/etc/passwd",
            "demo/../../../etc/passwd",
            "..%2fserver.py",
            "demo/./../tools.py",
        )
        for raw in cases:
            body, status = self._get_status(
                "/api/browser/fs?path=" + urllib.parse.quote(raw, safe="")
            )
            self.assertEqual(status, 403, msg="attendu 403 pour %s (got %s %s)" % (raw, status, body))
            self.assertEqual(body["error"], "path_denied")
            dumped = json.dumps(body)
            self.assertNotIn("ADMIN_TOKEN", dumped)
            if "passwd" in raw:
                self.assertNotIn("root:", dumped)

    def test_browser_fs_python_source_not_in_sandbox(self) -> None:
        body, status = self._get_status("/api/browser/fs?path=demo/server.py")
        self.assertIn(status, (403, 404))
        self.assertIn(body["error"], ("path_denied", "not_found"))
        body, status = self._get_status("/api/browser/fs?path=server.py")
        self.assertEqual(status, 403)
        self.assertEqual(body["error"], "path_denied")

    def test_browser_fs_write_is_read_only(self) -> None:
        body, status = self._post_status(
            "/api/browser/fs?path=demo/new.txt",
            {"content": "should-not-write"},
        )
        self.assertEqual(status, 405)
        self.assertEqual(body["error"], "fs_read_only")

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
        self.assertEqual(body["document_origin"], self.base)
        self.assertEqual(body["origin_binding"], "self")
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

    def test_same_origin_header_creates_session(self) -> None:
        body, code = self._post_status(
            "/api/sessions",
            {"site_id": "origin_same"},
            headers={"Origin": self.base},
        )
        self.assertEqual(code, 201)
        uuid.UUID(body["session_id"])
        self.assertEqual(body["document_origin"], self.base)
        self.assertEqual(body["origin_binding"], "self")
        dumped = json.dumps(body).lower()
        self.assertNotIn("api_key", dumped)
        self.assertNotIn("admin.takeover", dumped)

    def test_foreign_origin_header_refuses_session_create(self) -> None:
        before, _, _ = self._get_json("/api/admin/sessions")
        before_ids = {row["session_id"] for row in before["sessions"]}
        denied, code = self._post_status(
            "/api/sessions",
            {"site_id": "origin_create_denied"},
            headers={"Origin": "https://evil.example"},
        )
        self.assertEqual(code, 403)
        self.assertEqual(denied["error"], "origin_denied")
        self.assertEqual(denied["origin"], "https://evil.example")
        self.assertTrue(denied["request_id"])
        uuid.UUID(denied["request_id"])
        self.assertNotIn("session_id", denied)
        dumped = json.dumps(denied).lower()
        self.assertNotIn("api_key", dumped)
        self.assertNotIn("admin_token", dumped)
        after, _, _ = self._get_json("/api/admin/sessions")
        after_ids = {row["session_id"] for row in after["sessions"]}
        self.assertEqual(after_ids, before_ids)
        journal, _, _ = self._get_json("/api/admin/journal")
        match = [
            row
            for row in journal["journal"]
            if row["request_id"] == denied["request_id"]
        ]
        self.assertEqual(len(match), 1)
        self.assertEqual(match[0]["outcome"], "origin_denied")
        self.assertEqual(match[0]["tool"], "session.create")

    def test_foreign_referer_refuses_session_create(self) -> None:
        denied, code = self._post_status(
            "/api/sessions",
            {"site_id": "origin_referer"},
            headers={"Referer": "https://evil.example/embed"},
        )
        self.assertEqual(code, 403)
        self.assertEqual(denied["error"], "origin_denied")
        self.assertEqual(denied["origin"], "https://evil.example")
        self.assertTrue(denied["request_id"])

    def test_opaque_origin_null_refused(self) -> None:
        denied, code = self._post_status(
            "/api/sessions",
            {"site_id": "origin_null"},
            headers={"Origin": "null"},
        )
        self.assertEqual(code, 403)
        self.assertEqual(denied["error"], "origin_denied")
        self.assertTrue(denied["request_id"])

    def test_foreign_origin_message_does_not_store_secret(self) -> None:
        created, _, _ = self._post_json("/api/sessions", {"site_id": "origin_msg"})
        sid = created["session_id"]
        secret = "secret-origin-gamma"
        denied, code = self._post_status(
            "/api/sessions/%s/messages" % sid,
            {"content": secret},
            headers={"Origin": "https://evil.example"},
        )
        self.assertEqual(code, 403)
        self.assertEqual(denied["error"], "origin_denied")
        self.assertTrue(denied["request_id"])
        dumped = json.dumps(denied)
        self.assertNotIn(secret, dumped)
        fetched, _, _ = self._get_json("/api/sessions/" + sid)
        self.assertEqual(fetched["status"], "open")
        joined = " ".join(item["content"] for item in fetched["messages"])
        self.assertNotIn(secret, joined)
        self.assertEqual(fetched["messages"], [])

    def test_foreign_origin_get_session_hides_messages(self) -> None:
        created, _, _ = self._post_json("/api/sessions", {"site_id": "origin_get"})
        sid = created["session_id"]
        self._post_json(
            "/api/sessions/%s/messages" % sid,
            {"content": "secret-origin-delta"},
        )
        denied, code = self._get_status(
            "/api/sessions/" + sid,
            headers={"Origin": "https://evil.example"},
        )
        self.assertEqual(code, 403)
        self.assertEqual(denied["error"], "origin_denied")
        dumped = json.dumps(denied)
        self.assertNotIn("secret-origin-delta", dumped)
        self.assertNotIn("messages", denied)
        fetched, _, _ = self._get_json("/api/sessions/" + sid)
        self.assertIn("secret-origin-delta", json.dumps(fetched))

    def test_foreign_origin_header_refuses_tool_without_act(self) -> None:
        created, _, _ = self._post_json("/api/sessions", {"site_id": "origin_hdr_tool"})
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
                "origin": self.base,
                "args": {"selector": "#menu-toggle"},
            },
            headers={"Origin": "https://evil.example"},
        )
        self.assertEqual(code, 403)
        self.assertEqual(denied["error"], "origin_denied")
        self.assertTrue(denied["request_id"])
        after, _, _ = self._get_json("/api/demo-app/state")
        self.assertEqual(after["menu_open"], before["menu_open"])
        fetched, _, _ = self._get_json("/api/sessions/" + sid)
        self.assertEqual(fetched["status"], "open")

    def test_admin_lists_despite_foreign_origin_header(self) -> None:
        created, _, _ = self._post_json("/api/sessions", {"site_id": "origin_admin"})
        sid = created["session_id"]
        listing, code = self._get_status(
            "/api/admin/sessions",
            headers={"Origin": "https://evil.example"},
        )
        self.assertEqual(code, 200)
        ids = [row["session_id"] for row in listing["sessions"]]
        self.assertIn(sid, ids)

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
        os.environ.pop("MOHHDY_AGENT_MODE", None)
        os.environ.pop("MOHHDY_AGENT_SITE_ID", None)
        os.environ.pop("MOHHDY_AGENT_RUNTIME", None)
        os.environ.pop("MOHHDY_AGENT_BROWSER_ENGINE", None)
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

    def test_admin_token_still_required_with_origin_header(self) -> None:
        req = urllib.request.Request(
            self.base + "/api/admin/sessions",
            headers={"Origin": self.base},
        )
        with self.assertRaises(urllib.error.HTTPError) as ctx:
            urllib.request.urlopen(req, timeout=2)
        self.assertEqual(ctx.exception.code, 401)
        created = self._post_json("/api/sessions", {"site_id": "token_origin"})
        req_ok = urllib.request.Request(
            self.base + "/api/admin/sessions",
            headers={
                "Authorization": "Bearer " + self.TOKEN,
                "Origin": "https://evil.example",
            },
        )
        with urllib.request.urlopen(req_ok, timeout=2) as resp:
            listing = json.loads(resp.read().decode("utf-8"))
        ids = [row["session_id"] for row in listing["sessions"]]
        self.assertIn(created["session_id"], ids)
        self.assertNotIn(self.TOKEN, json.dumps(listing))

    def test_admin_status_does_not_require_token(self) -> None:
        with urllib.request.urlopen(self.base + "/api/admin/status", timeout=2) as resp:
            body = json.loads(resp.read().decode("utf-8"))
        self.assertEqual(body["auth"], "token")
        self.assertNotIn(self.TOKEN, json.dumps(body))

    def test_token_not_leaked_in_public_pages(self) -> None:
        for path in ("/embed.js", "/admin", "/health", "/demo", "/", "/browser", "/browser/fs"):
            with urllib.request.urlopen(self.base + path, timeout=2) as resp:
                body = resp.read().decode("utf-8")
            self.assertNotIn(self.TOKEN, body)

    def test_browser_fs_requires_token(self) -> None:
        with self.assertRaises(urllib.error.HTTPError) as ctx:
            urllib.request.urlopen(self.base + "/api/browser/fs", timeout=2)
        self.assertEqual(ctx.exception.code, 401)
        payload = json.loads(ctx.exception.read().decode("utf-8"))
        self.assertEqual(payload["error"], "admin_token_required")
        self.assertNotIn(self.TOKEN, json.dumps(payload))

    def test_browser_fs_authorized_lists_demo(self) -> None:
        req = urllib.request.Request(
            self.base + "/api/browser/fs?path=demo",
            headers={"Authorization": "Bearer " + self.TOKEN},
        )
        with urllib.request.urlopen(req, timeout=2) as resp:
            listing = json.loads(resp.read().decode("utf-8"))
        names = [row["name"] for row in listing["entries"]]
        self.assertIn("demo-app.html", names)

    def test_browser_fs_wrong_token_rejected(self) -> None:
        req = urllib.request.Request(
            self.base + "/api/browser/fs?path=demo",
            headers={"Authorization": "Bearer wrong-token-not-for-image"},
        )
        with self.assertRaises(urllib.error.HTTPError) as ctx:
            urllib.request.urlopen(req, timeout=2)
        self.assertEqual(ctx.exception.code, 401)

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

    def test_browser_navigate_and_screenshot_require_token(self) -> None:
        data = json.dumps({"url": "/demo-app"}).encode("utf-8")
        req = urllib.request.Request(
            self.base + "/api/browser/navigate",
            data=data,
            method="POST",
            headers={"Content-Type": "application/json"},
        )
        with self.assertRaises(urllib.error.HTTPError) as ctx:
            urllib.request.urlopen(req, timeout=2)
        self.assertEqual(ctx.exception.code, 401)
        payload = json.loads(ctx.exception.read().decode("utf-8"))
        self.assertEqual(payload["error"], "admin_token_required")
        self.assertNotIn(self.TOKEN, json.dumps(payload))
        with self.assertRaises(urllib.error.HTTPError) as shot:
            urllib.request.urlopen(self.base + "/api/browser/screenshot", timeout=2)
        self.assertEqual(shot.exception.code, 401)
        req_ok = urllib.request.Request(
            self.base + "/api/browser/navigate",
            data=data,
            method="POST",
            headers={
                "Content-Type": "application/json",
                "Authorization": "Bearer " + self.TOKEN,
            },
        )
        with self.assertRaises(urllib.error.HTTPError) as ctx_ok:
            urllib.request.urlopen(req_ok, timeout=2)
        self.assertEqual(ctx_ok.exception.code, 501)
        body = json.loads(ctx_ok.exception.read().decode("utf-8"))
        self.assertEqual(body["error"], "optional_not_installed")
        self.assertNotIn(self.TOKEN, json.dumps(body))


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
        os.environ.pop("MOHHDY_AGENT_MODE", None)
        os.environ.pop("MOHHDY_AGENT_SITE_ID", None)
        os.environ.pop("MOHHDY_AGENT_RUNTIME", None)
        os.environ.pop("MOHHDY_AGENT_BROWSER_ENGINE", None)
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


class AgentDeployScaffold(unittest.TestCase):
    """ASSIST-053 : mode hosted / quotas, sans facturation ni appel payant."""

    @classmethod
    def setUpClass(cls) -> None:
        cls._tmp = tempfile.TemporaryDirectory()
        cfg = Path(cls._tmp.name) / "hosted.json"
        cfg.write_text(
            json.dumps(
                {
                    "deployment": {"mode": "hosted", "billing": "stripe-ignored"},
                    "instance": {"site_id": "tenant_alpha"},
                    "quota": {
                        "billing": True,
                        "enforced": True,
                        "max_sessions": 12,
                        "max_sites": 3,
                        "max_messages_per_session": 9,
                    },
                    "sites": {
                        "tenant_alpha": {
                            "capabilities": [
                                "chat.reply",
                                "site.explain",
                                "session.escalate",
                            ]
                        },
                        "tenant_beta": {
                            "capabilities": ["chat.reply", "session.escalate"]
                        },
                    },
                }
            ),
            encoding="utf-8",
        )
        cls._prev = {
            "MOHHDY_AGENT_CONFIG": os.environ.get("MOHHDY_AGENT_CONFIG"),
            "MOHHDY_AGENT_KB": os.environ.get("MOHHDY_AGENT_KB"),
            "MOHHDY_AGENT_MODE": os.environ.get("MOHHDY_AGENT_MODE"),
            "MOHHDY_AGENT_SITE_ID": os.environ.get("MOHHDY_AGENT_SITE_ID"),
            "MOHHDY_AGENT_RUNTIME": os.environ.get("MOHHDY_AGENT_RUNTIME"),
            "ADMIN_TOKEN": os.environ.get("ADMIN_TOKEN"),
            "MOHHDY_AGENT_DATA": os.environ.get("MOHHDY_AGENT_DATA"),
        }
        os.environ["MOHHDY_AGENT_CONFIG"] = str(cfg)
        os.environ.pop("MOHHDY_AGENT_KB", None)
        os.environ.pop("MOHHDY_AGENT_MODE", None)
        os.environ.pop("MOHHDY_AGENT_SITE_ID", None)
        os.environ.pop("MOHHDY_AGENT_RUNTIME", None)
        os.environ.pop("MOHHDY_AGENT_BROWSER_ENGINE", None)
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
        for key, value in cls._prev.items():
            if value is None:
                os.environ.pop(key, None)
            else:
                os.environ[key] = value
        cls._tmp.cleanup()

    def test_hosted_mode_is_non_billing(self) -> None:
        with urllib.request.urlopen(self.base + "/health", timeout=2) as resp:
            body = json.loads(resp.read().decode("utf-8"))
        self.assertEqual(body["deployment_mode"], "hosted")
        self.assertEqual(body["billing"], "none")
        self.assertEqual(body["default_site_id"], "tenant_alpha")
        self.assertFalse(body["quota"]["billing"])
        self.assertFalse(body["quota"]["enforced"])
        self.assertEqual(body["quota"]["max_sessions"], 12)
        self.assertEqual(body["quota"]["max_sites"], 3)
        self.assertIn("pas une facturation", body["quota"]["note"])
        dumped = json.dumps(body).lower()
        self.assertNotIn("stripe", dumped)
        self.assertNotIn("sk_live", dumped)

    def test_admin_status_lists_tenants(self) -> None:
        with urllib.request.urlopen(self.base + "/api/admin/status", timeout=2) as resp:
            body = json.loads(resp.read().decode("utf-8"))
        ids = [row["site_id"] for row in body["tenants"]]
        self.assertIn("tenant_alpha", ids)
        self.assertIn("tenant_beta", ids)
        self.assertEqual(body["billing"], "none")
        self.assertEqual(body["deployment_mode"], "hosted")

    def test_env_overrides_mode_and_site(self) -> None:
        os.environ["MOHHDY_AGENT_MODE"] = "self_host"
        os.environ["MOHHDY_AGENT_SITE_ID"] = "from_env"
        os.environ["MOHHDY_AGENT_RUNTIME"] = "browser"
        self.httpd.reset_runtime()
        try:
            with urllib.request.urlopen(self.base + "/health", timeout=2) as resp:
                body = json.loads(resp.read().decode("utf-8"))
            self.assertEqual(body["deployment_mode"], "self_host")
            self.assertEqual(body["default_site_id"], "from_env")
            self.assertEqual(body["runtime"], "browser")
            self.assertFalse(body["phase3_complete"])
            self.assertFalse(body["us031_complete"])
        finally:
            os.environ.pop("MOHHDY_AGENT_MODE", None)
            os.environ.pop("MOHHDY_AGENT_SITE_ID", None)
            os.environ.pop("MOHHDY_AGENT_RUNTIME", None)
            os.environ.pop("MOHHDY_AGENT_BROWSER_ENGINE", None)
            self.httpd.reset_runtime()

    def test_no_paid_network_helpers(self) -> None:
        source = (ROOT / "server.py").read_text(encoding="utf-8")
        lowered = source.lower()
        self.assertNotIn("stripe", lowered)
        self.assertNotIn("paypal", lowered)
        self.assertNotIn("openai.com", lowered)
        packaging = ROOT / "packaging"
        for path in packaging.rglob("*"):
            if not path.is_file():
                continue
            text = path.read_text(encoding="utf-8")
            self.assertNotIn("sk_live", text)
            self.assertNotIn("BEGIN PRIVATE KEY", text)


class AgentBrowserFsIsolation(unittest.TestCase):
    """ASSIST-061 : isolation du sandbox data/ et auth, sans US-031."""

    TOKEN = "fs-token-not-for-image"

    @classmethod
    def setUpClass(cls) -> None:
        cls._tmp = tempfile.TemporaryDirectory()
        base = Path(cls._tmp.name)
        data = base / "data"
        data.mkdir()
        (data / "ok.txt").write_text("sandbox-visible\n", encoding="utf-8")
        (data / "secret.key").write_text("BEGIN PRIVATE KEY fake\n", encoding="utf-8")
        outside = base / "outside.txt"
        outside.write_text("leaked-outside\n", encoding="utf-8")
        leak = data / "leak.txt"
        try:
            leak.symlink_to(outside)
        except OSError:
            leak = None
        cls.outside = outside
        cls.leak = leak
        cls._prev = {
            "ADMIN_TOKEN": os.environ.get("ADMIN_TOKEN"),
            "MOHHDY_AGENT_DATA": os.environ.get("MOHHDY_AGENT_DATA"),
            "MOHHDY_AGENT_CONFIG": os.environ.get("MOHHDY_AGENT_CONFIG"),
            "MOHHDY_AGENT_KB": os.environ.get("MOHHDY_AGENT_KB"),
            "MOHHDY_AGENT_MODE": os.environ.get("MOHHDY_AGENT_MODE"),
            "MOHHDY_AGENT_SITE_ID": os.environ.get("MOHHDY_AGENT_SITE_ID"),
            "MOHHDY_AGENT_RUNTIME": os.environ.get("MOHHDY_AGENT_RUNTIME"),
        }
        os.environ["ADMIN_TOKEN"] = cls.TOKEN
        os.environ["MOHHDY_AGENT_DATA"] = str(data)
        os.environ["MOHHDY_AGENT_RUNTIME"] = "browser"
        os.environ.pop("MOHHDY_AGENT_CONFIG", None)
        os.environ.pop("MOHHDY_AGENT_KB", None)
        os.environ.pop("MOHHDY_AGENT_MODE", None)
        os.environ.pop("MOHHDY_AGENT_SITE_ID", None)
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
        for key, value in cls._prev.items():
            if value is None:
                os.environ.pop(key, None)
            else:
                os.environ[key] = value
        cls._tmp.cleanup()

    def _get(self, path: str, token: str | None = None):
        headers = {}
        if token:
            headers["Authorization"] = "Bearer " + token
        req = urllib.request.Request(self.base + path, headers=headers)
        try:
            with urllib.request.urlopen(req, timeout=2) as resp:
                return json.loads(resp.read().decode("utf-8")), resp.status
        except urllib.error.HTTPError as err:
            return json.loads(err.read().decode("utf-8")), err.code

    def test_runtime_browser_on_health(self) -> None:
        body, status = self._get("/health")
        self.assertEqual(status, 200)
        self.assertEqual(body["runtime"], "browser")
        self.assertFalse(body["phase3_complete"])
        self.assertFalse(body["us031_complete"])
        self.assertEqual(body["browser_engine"], "optional_not_installed")
        self.assertNotIn(self.TOKEN, json.dumps(body))

    def test_data_root_listed_and_readable_with_token(self) -> None:
        denied, status = self._get("/api/browser/fs?path=data")
        self.assertEqual(status, 401)
        body, status = self._get("/api/browser/fs?path=data", token=self.TOKEN)
        self.assertEqual(status, 200)
        names = [row["name"] for row in body["entries"]]
        self.assertIn("ok.txt", names)
        self.assertNotIn("secret.key", names)
        if self.leak is not None:
            self.assertNotIn("leak.txt", names)
        read, status = self._get(
            "/api/browser/fs?path=data/ok.txt", token=self.TOKEN
        )
        self.assertEqual(status, 200)
        self.assertIn("sandbox-visible", read["content"])
        self.assertNotIn("leaked-outside", read["content"])

    def test_symlink_and_outside_paths_denied(self) -> None:
        if self.leak is not None:
            body, status = self._get(
                "/api/browser/fs?path=data/leak.txt", token=self.TOKEN
            )
            self.assertEqual(status, 403)
            self.assertEqual(body["error"], "path_denied")
            self.assertNotIn("leaked-outside", json.dumps(body))
        body, status = self._get(
            "/api/browser/fs?path=" + urllib.parse.quote("../outside.txt", safe=""),
            token=self.TOKEN,
        )
        self.assertEqual(status, 403)
        self.assertEqual(body["error"], "path_denied")
        self.assertNotIn("leaked-outside", json.dumps(body))
        body, status = self._get(
            "/api/browser/fs?path=" + urllib.parse.quote("data/../outside.txt", safe=""),
            token=self.TOKEN,
        )
        self.assertEqual(status, 403)
        self.assertNotIn("leaked-outside", json.dumps(body))

    def test_key_file_not_served(self) -> None:
        body, status = self._get(
            "/api/browser/fs?path=data/secret.key", token=self.TOKEN
        )
        self.assertIn(status, (403, 415))
        dumped = json.dumps(body)
        self.assertNotIn("BEGIN PRIVATE KEY", dumped)
        self.assertNotIn(self.TOKEN, dumped)

    def test_write_still_read_only_with_token(self) -> None:
        data = json.dumps({"content": "nope"}).encode("utf-8")
        req = urllib.request.Request(
            self.base + "/api/browser/fs?path=data/ok.txt",
            data=data,
            method="POST",
            headers={
                "Content-Type": "application/json",
                "Authorization": "Bearer " + self.TOKEN,
            },
        )
        with self.assertRaises(urllib.error.HTTPError) as ctx:
            urllib.request.urlopen(req, timeout=2)
        self.assertEqual(ctx.exception.code, 405)
        payload = json.loads(ctx.exception.read().decode("utf-8"))
        self.assertEqual(payload["error"], "fs_read_only")


class AgentConfiguredOrigins(unittest.TestCase):
    """ASSIST-013 : allowlist par site dans MOHHDY_AGENT_CONFIG."""

    @classmethod
    def setUpClass(cls) -> None:
        cls._tmp = tempfile.TemporaryDirectory()
        cfg = Path(cls._tmp.name) / "origins.json"
        cfg.write_text(
            json.dumps(
                {
                    "allowed_origins": ["self"],
                    "sites": {
                        "site_public_demo": {
                            "capabilities": [
                                "chat.reply",
                                "site.explain",
                                "session.escalate",
                            ],
                            "allowed_origins": ["self"],
                        },
                        "site_shop_example": {
                            "capabilities": [
                                "chat.reply",
                                "site.explain",
                                "session.escalate",
                            ],
                            "allowed_origins": ["https://shop.example"],
                        },
                    },
                }
            ),
            encoding="utf-8",
        )
        cls._prev = {
            "MOHHDY_AGENT_CONFIG": os.environ.get("MOHHDY_AGENT_CONFIG"),
            "ADMIN_TOKEN": os.environ.get("ADMIN_TOKEN"),
            "MOHHDY_AGENT_DATA": os.environ.get("MOHHDY_AGENT_DATA"),
            "MOHHDY_AGENT_KB": os.environ.get("MOHHDY_AGENT_KB"),
            "MOHHDY_AGENT_MODE": os.environ.get("MOHHDY_AGENT_MODE"),
            "MOHHDY_AGENT_SITE_ID": os.environ.get("MOHHDY_AGENT_SITE_ID"),
            "MOHHDY_AGENT_RUNTIME": os.environ.get("MOHHDY_AGENT_RUNTIME"),
        }
        os.environ["MOHHDY_AGENT_CONFIG"] = str(cfg)
        os.environ.pop("ADMIN_TOKEN", None)
        os.environ.pop("MOHHDY_AGENT_DATA", None)
        os.environ.pop("MOHHDY_AGENT_KB", None)
        os.environ.pop("MOHHDY_AGENT_MODE", None)
        os.environ.pop("MOHHDY_AGENT_SITE_ID", None)
        os.environ.pop("MOHHDY_AGENT_RUNTIME", None)
        os.environ.pop("MOHHDY_AGENT_BROWSER_ENGINE", None)
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
        for key, value in cls._prev.items():
            if value is None:
                os.environ.pop(key, None)
            else:
                os.environ[key] = value
        cls._tmp.cleanup()

    def setUp(self) -> None:
        self.httpd.reset_runtime()

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

    def test_declared_shop_origin_accepted(self) -> None:
        body, code = self._post_status(
            "/api/sessions",
            {"site_id": "site_shop_example"},
            headers={"Origin": "https://shop.example"},
        )
        self.assertEqual(code, 201)
        self.assertEqual(body["document_origin"], "https://shop.example")
        self.assertEqual(body["origin_binding"], "https://shop.example")
        self.assertEqual(body["site_id"], "site_shop_example")
        dumped = json.dumps(body).lower()
        self.assertNotIn("api_key", dumped)
        self.assertNotIn("admin.takeover", dumped)

    def test_shop_site_rejects_self_and_foreign(self) -> None:
        denied_self, code = self._post_status(
            "/api/sessions",
            {"site_id": "site_shop_example"},
            headers={"Origin": self.base},
        )
        self.assertEqual(code, 403)
        self.assertEqual(denied_self["error"], "origin_denied")
        denied_evil, code = self._post_status(
            "/api/sessions",
            {"site_id": "site_shop_example"},
            headers={"Origin": "https://evil.example"},
        )
        self.assertEqual(code, 403)
        self.assertEqual(denied_evil["error"], "origin_denied")
        self.assertTrue(denied_evil["request_id"])

    def test_demo_site_self_still_works_shop_does_not(self) -> None:
        ok, code = self._post_status(
            "/api/sessions",
            {"site_id": "site_public_demo"},
            headers={"Origin": self.base},
        )
        self.assertEqual(code, 201)
        self.assertEqual(ok["document_origin"], self.base)
        denied, code = self._post_status(
            "/api/sessions",
            {"site_id": "site_public_demo"},
            headers={"Origin": "https://shop.example"},
        )
        self.assertEqual(code, 403)
        self.assertEqual(denied["error"], "origin_denied")
        posted, code = self._post_status(
            "/api/sessions/%s/messages" % ok["session_id"],
            {"content": "secret-shop-cross"},
            headers={"Origin": "https://shop.example"},
        )
        self.assertEqual(code, 403)
        self.assertNotIn("secret-shop-cross", json.dumps(posted))

    def test_shop_session_rejects_self_messages(self) -> None:
        created, code = self._post_status(
            "/api/sessions",
            {"site_id": "site_shop_example"},
            headers={"Origin": "https://shop.example"},
        )
        self.assertEqual(code, 201)
        denied, code = self._post_status(
            "/api/sessions/%s/messages" % created["session_id"],
            {"content": "secret-self-cross"},
            headers={"Origin": self.base},
        )
        self.assertEqual(code, 403)
        self.assertEqual(denied["error"], "origin_denied")
        self.assertNotIn("secret-self-cross", json.dumps(denied))


TINY_PNG = (
    b"\x89PNG\r\n\x1a\n\x00\x00\x00\rIHDR\x00\x00\x00\x01\x00\x00\x00\x01"
    b"\x08\x02\x00\x00\x00\x90wS\xde\x00\x00\x00\x0cIDATx\x9cc\xf8\x0f\x00"
    b"\x00\x01\x01\x00\x05\x18\xd8N\x00\x00\x00\x00IEND\xaeB`\x82"
)


class FakePage:
    def __init__(self) -> None:
        self.url = "about:blank"
        self._title = ""

    def goto(self, url, wait_until=None, timeout=None):
        self.url = url
        parsed = urllib.parse.urlparse(url)
        self._title = parsed.path.strip("/") or parsed.netloc or "page"

    def title(self) -> str:
        return self._title

    def screenshot(self, type="png", timeout=None):
        return TINY_PNG


class FakeBrowser:
    def __init__(self) -> None:
        self.page = FakePage()
        self.closed = False

    def new_page(self) -> FakePage:
        return self.page

    def close(self) -> None:
        self.closed = True


class FakePlaywright:
    def __init__(self) -> None:
        self.chromium = self
        self.stopped = False
        self.browser = FakeBrowser()

    def launch(self, headless=True, args=None):
        return self.browser

    def stop(self) -> None:
        self.stopped = True


def fake_playwright_launcher():
    session = FakePlaywright()

    class Handle:
        def start(self):
            return session

        def stop(self):
            session.stop()

    return Handle()


def _playwright_chromium_launchable() -> bool:
    if agent_browser.playwright_package_present() is False:
        return False
    try:
        from playwright.sync_api import sync_playwright
    except Exception:
        return False
    try:
        pw = sync_playwright().start()
    except Exception:
        return False
    try:
        browser = pw.chromium.launch(headless=True)
        browser.close()
        return True
    except Exception:
        return False
    finally:
        try:
            pw.stop()
        except Exception:
            pass


class BrowserEngineHelpers(unittest.TestCase):
    def test_resolve_local_and_deny_file(self) -> None:
        self.assertEqual(
            agent_browser.resolve_target_url("/demo-app", "http://127.0.0.1:8080"),
            "http://127.0.0.1:8080/demo-app",
        )
        with self.assertRaises(agent_browser.UrlDenied):
            agent_browser.resolve_target_url("file:///etc/passwd", "http://127.0.0.1:8080")
        with self.assertRaises(agent_browser.UrlDenied):
            agent_browser.resolve_target_url("javascript:alert(1)", "http://127.0.0.1:8080")
        with self.assertRaises(agent_browser.UrlError):
            agent_browser.resolve_target_url("", "http://127.0.0.1:8080")

    def test_url_allowed_self_and_foreign(self) -> None:
        self.assertTrue(
            agent_browser.url_allowed(
                "http://127.0.0.1:8080/demo-app",
                ["self"],
                "http://127.0.0.1:8080",
            )
        )
        self.assertFalse(
            agent_browser.url_allowed(
                "https://evil.example/",
                ["self"],
                "http://127.0.0.1:8080",
            )
        )

    def test_import_does_not_load_playwright(self) -> None:
        self.assertNotIn("playwright", sys.modules)
        self.assertEqual(agent_browser.engine_label(""), "optional_not_installed")
        self.assertEqual(agent_browser.engine_label("playwright"), "optional_not_installed")
        self.assertNotIn("playwright", sys.modules)


class AgentBrowserEngineMock(unittest.TestCase):
    TOKEN = "engine-token-not-for-image"

    @classmethod
    def setUpClass(cls) -> None:
        cls._prev_token = os.environ.get("ADMIN_TOKEN")
        cls._prev_engine = os.environ.get("MOHHDY_AGENT_BROWSER_ENGINE")
        os.environ.pop("ADMIN_TOKEN", None)
        os.environ.pop("MOHHDY_AGENT_BROWSER_ENGINE", None)
        os.environ.pop("MOHHDY_AGENT_CONFIG", None)
        os.environ.pop("MOHHDY_AGENT_DATA", None)
        cls.httpd = server.make_server("127.0.0.1", 0)
        cls.base = "http://127.0.0.1:%d" % cls.httpd.server_address[1]
        cls.httpd.browser = agent_browser.OptionalBrowser(
            launcher=fake_playwright_launcher,
            requested="playwright",
        )
        cls.thread = threading.Thread(target=cls.httpd.serve_forever, daemon=True)
        cls.thread.start()
        _wait_ready(cls.base)

    @classmethod
    def tearDownClass(cls) -> None:
        cls.httpd.shutdown()
        cls.httpd.server_close()
        cls.thread.join(timeout=2)
        if cls._prev_token is None:
            os.environ.pop("ADMIN_TOKEN", None)
        else:
            os.environ["ADMIN_TOKEN"] = cls._prev_token
        if cls._prev_engine is None:
            os.environ.pop("MOHHDY_AGENT_BROWSER_ENGINE", None)
        else:
            os.environ["MOHHDY_AGENT_BROWSER_ENGINE"] = cls._prev_engine

    def setUp(self) -> None:
        os.environ.pop("ADMIN_TOKEN", None)
        self.httpd.reset_runtime()
        self.httpd.browser = agent_browser.OptionalBrowser(
            launcher=fake_playwright_launcher,
            requested="playwright",
        )

    def _get(self, path: str, headers: dict | None = None):
        req = urllib.request.Request(self.base + path, headers=headers or {})
        try:
            with urllib.request.urlopen(req, timeout=2) as resp:
                return json.loads(resp.read().decode("utf-8")), resp.status, resp
        except urllib.error.HTTPError as err:
            return json.loads(err.read().decode("utf-8")), err.code, err

    def _post(self, path: str, payload: dict, headers: dict | None = None):
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

    def test_health_reports_playwright_when_mocked(self) -> None:
        body, status, _ = self._get("/health")
        self.assertEqual(status, 200)
        self.assertEqual(body["browser_engine"], "playwright")
        self.assertEqual(body["harness"], "dom_simulator")
        self.assertFalse(body["phase3_complete"])
        self.assertFalse(body["us031_complete"])

    def test_navigate_local_and_screenshot(self) -> None:
        opened, status = self._post("/api/browser/navigate", {"url": "/demo-app"})
        self.assertEqual(status, 200, msg=opened)
        self.assertTrue(opened["ok"])
        self.assertIn("/demo-app", opened["url"])
        self.assertEqual(opened["browser_engine"], "playwright")
        self.assertEqual(opened["harness"], "dom_simulator")
        self.assertTrue(opened["screenshot_available"])
        self.assertFalse(opened["phase3_complete"])
        req = urllib.request.Request(self.base + "/api/browser/screenshot")
        with urllib.request.urlopen(req, timeout=2) as resp:
            self.assertEqual(resp.status, 200)
            self.assertIn("image/png", resp.headers.get("Content-Type", ""))
            blob = resp.read()
        self.assertEqual(blob[:8], b"\x89PNG\r\n\x1a\n")
        info, status, _ = self._get("/api/browser")
        self.assertEqual(status, 200)
        self.assertIn("/demo-app", info["page"]["url"])
        self.assertTrue(info["page"]["screenshot_available"])

    def test_navigate_foreign_origin_denied(self) -> None:
        denied, status = self._post(
            "/api/browser/navigate", {"url": "https://evil.example/leak"}
        )
        self.assertEqual(status, 403)
        self.assertEqual(denied["error"], "origin_denied")
        self.assertIn("request_id", denied)
        self.assertFalse(denied["us031_complete"])
        info, _, _ = self._get("/api/browser")
        self.assertEqual(info["page"]["url"], "")

    def test_session_tools_stay_on_simulator(self) -> None:
        created, status = self._post("/api/sessions", {"site_id": "engine_site"})
        self.assertEqual(status, 201)
        sid = created["session_id"]
        req = urllib.request.Request(
            self.base + "/api/admin/sessions/%s/capabilities" % sid,
            data=json.dumps({"grant": ["dom.click"]}).encode("utf-8"),
            method="POST",
            headers={"Content-Type": "application/json"},
        )
        with urllib.request.urlopen(req, timeout=2):
            pass
        tool, status = self._post(
            "/api/sessions/%s/tools" % sid,
            {
                "tool": "dom.click",
                "origin": self.base,
                "args": {"selector": "#menu-toggle"},
            },
        )
        self.assertEqual(status, 200, msg=tool)
        self.assertEqual(tool["harness"], "dom_simulator")
        self.assertNotEqual(tool.get("browser_engine"), "playwright")

    def test_navigate_requires_admin_token_when_set(self) -> None:
        os.environ["ADMIN_TOKEN"] = self.TOKEN
        try:
            denied, status = self._post("/api/browser/navigate", {"url": "/demo-app"})
            self.assertEqual(status, 401)
            self.assertEqual(denied["error"], "admin_token_required")
            self.assertNotIn(self.TOKEN, json.dumps(denied))
            opened, status = self._post(
                "/api/browser/navigate",
                {"url": "/demo-app"},
                headers={"Authorization": "Bearer " + self.TOKEN},
            )
            self.assertEqual(status, 200, msg=opened)
            req = urllib.request.Request(self.base + "/api/browser/screenshot")
            with self.assertRaises(urllib.error.HTTPError) as ctx:
                urllib.request.urlopen(req, timeout=2)
            self.assertEqual(ctx.exception.code, 401)
        finally:
            os.environ.pop("ADMIN_TOKEN", None)


@unittest.skipUnless(
    _playwright_chromium_launchable(),
    "fumee Playwright optionnelle : paquet et Chromium absents",
)
class AgentPlaywrightOptional(unittest.TestCase):
    @classmethod
    def setUpClass(cls) -> None:
        os.environ.pop("ADMIN_TOKEN", None)
        os.environ["MOHHDY_AGENT_BROWSER_ENGINE"] = "playwright"
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
        os.environ.pop("MOHHDY_AGENT_BROWSER_ENGINE", None)

    def test_live_navigate_demo_app(self) -> None:
        with urllib.request.urlopen(self.base + "/health", timeout=2) as resp:
            health = json.loads(resp.read().decode("utf-8"))
        self.assertEqual(health["browser_engine"], "playwright")
        self.assertFalse(health["us031_complete"])
        data = json.dumps({"url": "/demo-app"}).encode("utf-8")
        req = urllib.request.Request(
            self.base + "/api/browser/navigate",
            data=data,
            method="POST",
            headers={"Content-Type": "application/json"},
        )
        with urllib.request.urlopen(req, timeout=15) as resp:
            body = json.loads(resp.read().decode("utf-8"))
            self.assertEqual(resp.status, 200)
        self.assertIn("/demo-app", body["url"])
        self.assertTrue(body["title"] or body["url"])
        shot = urllib.request.Request(self.base + "/api/browser/screenshot")
        with urllib.request.urlopen(shot, timeout=5) as resp:
            payload = resp.read()
        self.assertGreater(len(payload), 20)
        self.assertEqual(payload[:8], b"\x89PNG\r\n\x1a\n")


if __name__ == "__main__":
    unittest.main(verbosity=2)
