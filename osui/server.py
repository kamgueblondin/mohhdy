#!/usr/bin/env python3
"""Shell graphique du SE Mohhdy (OS-UI-0 / OS-UI-1 / OS-UI-2).

L'entree produit est le chrome OS (fenetre, barre, bureaux). Le backend
HTTP reste le scaffold temporaire agent/ (parite comportementale). Ce
n'est pas un LLM de production, pas US-031, pas Chromium de session, pas
le guest i386 Multiboot. Aucun secret n'est cuit dans l'image.
"""

from __future__ import annotations

import importlib.util
import os
import sys
from pathlib import Path
from typing import Optional
from urllib.parse import urlparse

OSUI_ROOT = Path(__file__).resolve().parent
REPO_ROOT = OSUI_ROOT.parent
STATIC_DIR = OSUI_ROOT / "static"
AGENT_ROOT = Path(
    os.environ.get("MOHHDY_AGENT_ROOT", str(REPO_ROOT / "agent"))
).resolve()

SERVICE_NAME = "mohhdy-os"
SHELL_KIND = "osui"
BACKEND_NAME = "mohhdy-agent"
OSUI_STATIC_RE = __import__("re").compile(r"^[A-Za-z0-9._-]{1,64}$")

MIME_BY_SUFFIX = {
    ".css": "text/css; charset=utf-8",
    ".js": "application/javascript; charset=utf-8",
    ".html": "text/html; charset=utf-8",
    ".svg": "image/svg+xml",
    ".txt": "text/plain; charset=utf-8",
    ".json": "application/json; charset=utf-8",
}


def _load_agent_server():
    """Charge agent/server.py sous un nom unique (evite le clash sys.modules)."""
    if not AGENT_ROOT.is_dir():
        raise SystemExit("backend agent/ introuvable: %s" % AGENT_ROOT)
    path = AGENT_ROOT / "server.py"
    if not path.is_file():
        raise SystemExit("agent/server.py introuvable: %s" % path)
    agent_path = str(AGENT_ROOT)
    if agent_path not in sys.path:
        sys.path.insert(0, agent_path)
    spec = importlib.util.spec_from_file_location("mohhdy_agent_server", path)
    if spec is None or spec.loader is None:
        raise SystemExit("impossible de charger agent/server.py")
    module = importlib.util.module_from_spec(spec)
    sys.modules["mohhdy_agent_server"] = module
    spec.loader.exec_module(module)
    return module


agent_server = _load_agent_server()


def safe_osui_static(name: str) -> Optional[Path]:
    """Resout un fichier sous osui/static/, sans traversal."""
    if not name or not OSUI_STATIC_RE.match(name):
        return None
    candidate = (STATIC_DIR / name).resolve()
    try:
        candidate.relative_to(STATIC_DIR.resolve())
    except ValueError:
        return None
    if not candidate.is_file():
        return None
    return candidate


def os_identity(httpd) -> dict:
    policy_status = httpd.policy.public_status()
    return {
        "status": "ok",
        "service": SERVICE_NAME,
        "shell": SHELL_KIND,
        "backend": BACKEND_NAME,
        "entry": "/",
        "llm": agent_server.LLM_KIND,
        "admin_auth": agent_server.admin_auth_mode(),
        "kb_loaded": policy_status["kb_loaded"],
        "harness": agent_server.HARNESS_KIND,
        "deployment_mode": policy_status["deployment_mode"],
        "runtime": policy_status["runtime"],
        "browser_engine": httpd.browser.label(),
        "phase3_complete": False,
        "us031_complete": False,
        "chromium_session_engine": False,
        "browser_fs": True,
        "billing": policy_status["billing"],
        "default_site_id": policy_status["default_site_id"],
        "quota": policy_status["quota"],
        "panes": ["browser-os", "support", "admin", "status"],
        "notes": {
            "llm": "stub_echo: echo / KB locale, pas un LLM de production",
            "browser": "simulateur DOM; Playwright optionnel, pas US-031",
            "guest": "le noyau i386 QEMU n'est pas boote dans ce conteneur",
        },
    }


class OsHandler(agent_server.AgentHandler):
    server_version = "MOHHDY-OS/0.8"

    def log_message(self, fmt: str, *args) -> None:
        sys.stderr.write(
            "mohhdy-os: %s - %s\n" % (self.address_string(), fmt % args)
        )

    def _dispatch(self, method: str, send_body: bool) -> None:
        parsed = urlparse(self.path)
        path = self._normalize(parsed.path)

        if method in ("GET", "HEAD") and path in ("/", "/index.html", "/os", "/os/"):
            self._send_osui_static(
                "index.html", "text/html; charset=utf-8", send_body
            )
            return

        if method in ("GET", "HEAD") and path.startswith("/os/"):
            rel = path[len("/os/") :]
            if rel.endswith("/"):
                rel = rel + "index.html"
            self._send_osui_named(rel, send_body)
            return

        if path == "/health":
            if method not in ("GET", "HEAD"):
                self._send_json_status(
                    405,
                    {"status": "error", "error": "method_not_allowed"},
                    extra_headers=agent_server.TEXT_HEADERS,
                )
                return
            self._send_json(os_identity(self.server), send_body=send_body)
            return

        if path == "/api/os":
            if method not in ("GET", "HEAD"):
                self._send_json_status(
                    405, {"status": "error", "error": "method_not_allowed"}
                )
                return
            self._send_json(os_identity(self.server), send_body=send_body)
            return

        return super()._dispatch(method, send_body)

    def _send_osui_named(self, name: str, send_body: bool) -> None:
        path = safe_osui_static(name)
        if path is None:
            self._send_not_found(send_body=send_body)
            return
        suffix = path.suffix.lower()
        content_type = MIME_BY_SUFFIX.get(suffix, "application/octet-stream")
        extra = agent_server.TEXT_HEADERS
        if suffix == ".js":
            extra = agent_server.PUBLIC_JS_HEADERS
        elif suffix == ".css":
            extra = agent_server.PUBLIC_CSS_HEADERS
        data = path.read_bytes()
        self._send_bytes(200, data, content_type, send_body, extra_headers=extra)

    def _send_osui_static(
        self, name: str, content_type: str, send_body: bool
    ) -> None:
        path = safe_osui_static(name)
        if path is None:
            self._send_not_found(send_body=send_body)
            return
        data = path.read_bytes()
        self._send_bytes(
            200,
            data,
            content_type,
            send_body,
            extra_headers=agent_server.TEXT_HEADERS,
        )


def make_server(
    host: Optional[str] = None, port: Optional[int] = None
) -> agent_server.AgentHTTPServer:
    bind_host = agent_server.env_host() if host is None else host
    bind_port = agent_server.env_port() if port is None else port
    httpd = agent_server.AgentHTTPServer((bind_host, bind_port), OsHandler)
    httpd.daemon_threads = True
    return httpd


def main() -> None:
    if not STATIC_DIR.is_dir():
        raise SystemExit("repertoire osui/static/ introuvable: %s" % STATIC_DIR)
    if not AGENT_ROOT.is_dir():
        raise SystemExit("backend agent/ introuvable: %s" % AGENT_ROOT)

    httpd = make_server()
    host, port = httpd.server_address[:2]
    identity = os_identity(httpd)
    sys.stderr.write(
        "mohhdy-os OS-UI-0/1/2 sur http://%s:%s "
        "(shell=/ health=/health api=/api/os backend=%s llm=%s "
        "harness=%s engine=%s admin_auth=%s phase3=non us031=non "
        "chromium_session=non)\n"
        % (
            host,
            port,
            BACKEND_NAME,
            identity["llm"],
            identity["harness"],
            identity["browser_engine"],
            identity["admin_auth"],
        )
    )
    try:
        httpd.serve_forever()
    except KeyboardInterrupt:
        sys.stderr.write("mohhdy-os: arret\n")
    finally:
        httpd.server_close()


if __name__ == "__main__":
    main()
