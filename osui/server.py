#!/usr/bin/env python3
"""Shell graphique du SE Mohhdy (OS-UI-0 / OS-UI-1 / OS-UI-2).

L'entree produit est le chrome OS : chat central (prompts / slash),
scene IA plein ecran (#ai-stage), puis panes (browser, shell Multiboot,
admin, support, statut, fs). Le backend HTTP reste le scaffold temporaire
agent/ (parite comportementale). Ce n'est pas un LLM de production, pas
US-031, pas Chromium de session. osui est le bootstrap graphique du
meme SE Multiboot ; le guest i386 n'est pas dans ce processus. Aucun
secret n'est cuit dans l'image.
"""

from __future__ import annotations

import importlib.util
import os
import sys
from pathlib import Path
from typing import Optional
from urllib.parse import urlparse

import multiboot_shell
import prompt_os
import stage
from command_registry import REGISTRY
from guest_attach import AttachConfig

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

OS_COMMANDS = list(prompt_os.SLASH_COMMANDS)

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
        "panes": [
            "chat",
            "browser-os",
            "shell",
            "support",
            "admin",
            "status",
            "fs",
        ],
        "commands": list(OS_COMMANDS),
        "interaction": {
            "primary": "center_chat",
            "slash": True,
            "floating_chat": True,
            "chat_drag": True,
            "chat_pos_key": "mohhdy.os.chat.pos",
            "ai_stage": True,
            "multiboot_shell": True,
            "prompt_os": True,
            "autonomous_stage": True,
            "live_attach": True,
        },
        "stage": stage.public_stage_meta(),
        "multiboot_shell": httpd.os_shell.public_meta()
        if getattr(httpd, "os_shell", None)
        else multiboot_shell.MultibootShell().public_meta(),
        "registry": REGISTRY.public_dict(),
        "attach": (
            httpd.os_shell.attach_config.public_dict()
            if getattr(httpd, "os_shell", None)
            else AttachConfig.from_env().public_dict()
        ),
        "notes": {
            "llm": "stub_echo: echo / KB locale, pas un LLM de production",
            "browser": "simulateur DOM; Playwright optionnel, pas US-031",
            "guest": "le noyau i386 QEMU n'est pas boote dans ce conteneur",
            "shell": "shell Multiboot (userspace/shell.c). bootstrap par defaut, live si attache",
            "chat": "chat central = surface de commande; flottant si un programme est ouvert",
            "stage": "bureau = scene IA HTML (#ai-stage). Guest VGA n'a pas cette scene",
            "product": "un SE Multiboot Mohhdy ; osui = bootstrap graphique du meme OS",
            "convergence": "registre shared/multiboot_shell_commands.json ; features land in Multiboot SE",
        },
    }


class OsHTTPServer(agent_server.AgentHTTPServer):
    def __init__(self, server_address, RequestHandlerClass):
        super().__init__(server_address, RequestHandlerClass)
        self.os_stage = stage.StageState()
        self.os_shell = multiboot_shell.MultibootShell()

    def reset_runtime(self) -> None:
        super().reset_runtime()
        self.os_stage.reset()
        self.os_shell.reset()


class OsHandler(agent_server.AgentHandler):
    server_version = "MOHHDY-OS/0.9"

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

        if path == "/api/os/stage":
            self._handle_stage(method, send_body)
            return

        if path == "/api/os/stage/tick":
            self._handle_stage_tick(method, send_body)
            return

        if path == "/api/os/prompt":
            self._handle_prompt(method, send_body)
            return

        if path == "/api/os/commands":
            if method not in ("GET", "HEAD"):
                self._send_json_status(
                    405, {"status": "error", "error": "method_not_allowed"}
                )
                return
            self._send_json(REGISTRY.public_dict(), send_body=send_body)
            return

        if path == "/api/os/attach":
            self._handle_attach(method, send_body)
            return

        if path == "/api/os/shell":
            self._handle_shell(method, send_body)
            return

        return super()._dispatch(method, send_body)

    def _handle_stage(self, method: str, send_body: bool) -> None:
        store = self.server.os_stage
        if method in ("GET", "HEAD"):
            self._send_json(store.snapshot(), send_body=send_body)
            return
        if method != "POST":
            self._send_json_status(
                405, {"status": "error", "error": "method_not_allowed"}
            )
            return
        payload, error = self._read_json_object()
        if error is not None:
            status, body = error
            self._send_json_status(status, body)
            return
        prompt = payload.get("prompt")
        if prompt is None:
            prompt = payload.get("content") or ""
        if not isinstance(prompt, str):
            self._send_json_status(
                400,
                {
                    "status": "error",
                    "error": "bad_request",
                    "message": "prompt texte requis",
                },
            )
            return
        autonomous = payload.get("autonomous")
        if autonomous is not None and not isinstance(autonomous, bool):
            autonomous = bool(autonomous)
        result = store.apply_prompt(prompt, autonomous=autonomous)
        self._send_json(result, send_body=True, status=200)

    def _handle_stage_tick(self, method: str, send_body: bool) -> None:
        if method != "POST":
            if method in ("GET", "HEAD"):
                self._send_json(self.server.os_stage.snapshot(), send_body=send_body)
                return
            self._send_json_status(
                405, {"status": "error", "error": "method_not_allowed"}
            )
            return
        self._send_json(self.server.os_stage.tick(), send_body=True, status=200)

    def _handle_prompt(self, method: str, send_body: bool) -> None:
        if method != "POST":
            self._send_json_status(
                405, {"status": "error", "error": "method_not_allowed"}
            )
            return
        payload, error = self._read_json_object()
        if error is not None:
            status, body = error
            self._send_json_status(status, body)
            return
        text = payload.get("text")
        if text is None:
            text = payload.get("prompt") or payload.get("content") or ""
        if not isinstance(text, str):
            self._send_json_status(
                400,
                {
                    "status": "error",
                    "error": "bad_request",
                    "message": "text requis",
                },
            )
            return
        routed = prompt_os.route_prompt(text)
        routed["status"] = "ok"
        routed["llm"] = agent_server.LLM_KIND
        if routed.get("kind") in ("stage", "stage_plan", "chat") and routed.get(
            "prompt"
        ) is not None:
            autonomous = routed.get("kind") == "stage_plan"
            routed["stage"] = self.server.os_stage.apply_prompt(
                routed["prompt"], autonomous=autonomous
            )
        if routed.get("kind") == "shell":
            line = routed.get("line") or ""
            routed["shell"] = self.server.os_shell.execute(line)
            stage_payload = routed["shell"].get("stage")
            if stage_payload:
                self.server.os_stage.current = dict(stage_payload)
                routed["stage"] = stage_payload
        self._send_json(routed, send_body=True, status=200)

    def _handle_attach(self, method: str, send_body: bool) -> None:
        shell = self.server.os_shell
        if method in ("GET", "HEAD"):
            meta = shell.public_meta()
            self._send_json(meta, send_body=send_body)
            return
        if method != "POST":
            self._send_json_status(
                405, {"status": "error", "error": "method_not_allowed"}
            )
            return
        payload, error = self._read_json_object()
        if error is not None:
            status, body = error
            self._send_json_status(status, body)
            return
        action = str(payload.get("action") or "status").lower()
        if action == "attach":
            output, rc = shell.try_attach()
            body = shell._result(output, rc)
            self._send_json(body, send_body=True, status=200)
            return
        if action == "detach":
            result = shell.execute("detach")
            self._send_json(result, send_body=True, status=200)
            return
        self._send_json(shell.public_meta(), send_body=True, status=200)

    def _handle_shell(self, method: str, send_body: bool) -> None:
        shell = self.server.os_shell
        if method in ("GET", "HEAD"):
            self._send_json(shell.public_meta(), send_body=send_body)
            return
        if method != "POST":
            self._send_json_status(
                405, {"status": "error", "error": "method_not_allowed"}
            )
            return
        payload, error = self._read_json_object()
        if error is not None:
            status, body = error
            self._send_json_status(status, body)
            return
        line = payload.get("line")
        if line is None:
            line = payload.get("command") or ""
        if not isinstance(line, str):
            self._send_json_status(
                400,
                {
                    "status": "error",
                    "error": "bad_request",
                    "message": "line texte requise",
                },
            )
            return
        result = shell.execute(line)
        stage_payload = result.get("stage")
        if stage_payload:
            self.server.os_stage.current = dict(stage_payload)
        self._send_json(result, send_body=True, status=200)

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
) -> OsHTTPServer:
    bind_host = agent_server.env_host() if host is None else host
    bind_port = agent_server.env_port() if port is None else port
    httpd = OsHTTPServer((bind_host, bind_port), OsHandler)
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
