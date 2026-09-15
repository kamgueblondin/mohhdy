#!/usr/bin/env python3
"""Runtime HTTP agent MOHHDY (ASSIST-050 + 010/011/040).

Sert l'origine d'embed, des sessions visiteur isolees en memoire, un echo
stub local, et une console admin. Ce n'est pas un LLM de production, pas
d'appel OpenAI, pas d'actes navigateur, et ce n'est pas le noyau
Multiboot i386.
"""

from __future__ import annotations

import hmac
import json
import os
import posixpath
import re
import sys
import threading
import uuid
from datetime import datetime, timezone
from http.server import BaseHTTPRequestHandler, ThreadingHTTPServer
from pathlib import Path
from typing import Any, Optional
from urllib.parse import parse_qs, unquote, urlparse

SERVICE_NAME = "mohhdy-agent"
DEFAULT_HOST = "0.0.0.0"
DEFAULT_PORT = 8080
LLM_KIND = "stub_echo"

ROOT = Path(__file__).resolve().parent
STATIC_DIR = ROOT / "static"

MAX_BODY_BYTES = 16384
MAX_CONTENT_CHARS = 4000
MAX_MESSAGES = 100
MAX_SESSIONS = 500
SITE_ID_RE = re.compile(r"^[A-Za-z0-9._-]{1,64}$")
SESSION_ID_RE = re.compile(
    r"^[0-9a-f]{8}-[0-9a-f]{4}-[0-9a-f]{4}-[0-9a-f]{4}-[0-9a-f]{12}$"
)

TEXT_HEADERS = {
    "X-Content-Type-Options": "nosniff",
    "Referrer-Policy": "no-referrer",
    "Cache-Control": "no-store",
}

PUBLIC_JS_HEADERS = {
    "X-Content-Type-Options": "nosniff",
    "Referrer-Policy": "no-referrer",
    "Cache-Control": "public, max-age=60",
    "Access-Control-Allow-Origin": "*",
}

PUBLIC_CSS_HEADERS = {
    "X-Content-Type-Options": "nosniff",
    "Referrer-Policy": "no-referrer",
    "Cache-Control": "public, max-age=60",
    "Access-Control-Allow-Origin": "*",
}

VISITOR_API_HEADERS = {
    "X-Content-Type-Options": "nosniff",
    "Referrer-Policy": "no-referrer",
    "Cache-Control": "no-store",
    "Access-Control-Allow-Origin": "*",
    "Access-Control-Allow-Methods": "GET, POST, OPTIONS",
    "Access-Control-Allow-Headers": "Content-Type",
}


def utc_now() -> str:
    return datetime.now(timezone.utc).strftime("%Y-%m-%dT%H:%M:%SZ")


def env_host() -> str:
    return os.environ.get("MOHHDY_AGENT_HOST", DEFAULT_HOST)


def env_port() -> int:
    raw = os.environ.get("MOHHDY_AGENT_PORT", str(DEFAULT_PORT))
    try:
        port = int(raw)
    except ValueError as exc:
        raise SystemExit("MOHHDY_AGENT_PORT doit etre un entier") from exc
    if not (1 <= port <= 65535):
        raise SystemExit("MOHHDY_AGENT_PORT hors plage 1-65535")
    return port


def env_admin_token() -> str:
    return os.environ.get("ADMIN_TOKEN") or ""


def env_data_path() -> Optional[Path]:
    raw = (os.environ.get("MOHHDY_AGENT_DATA") or "").strip()
    if not raw:
        return None
    path = Path(raw)
    try:
        path.mkdir(parents=True, exist_ok=True)
    except OSError:
        return None
    return path / "sessions.json"


def admin_auth_mode() -> str:
    return "token" if env_admin_token() else "open_stub"


def normalize_site_id(raw: Any) -> str:
    if raw is None:
        return "unspecified"
    text = str(raw).strip()
    if not text:
        return "unspecified"
    if not SITE_ID_RE.match(text):
        raise ValueError("site_id invalide")
    return text


def stub_reply(text: str) -> str:
    excerpt = text.strip()
    if len(excerpt) > 240:
        excerpt = excerpt[:240] + "..."
    return (
        "Reponse stub (pas un LLM de production, aucun appel reseau). "
        "Vous avez dit : %s" % excerpt
    )


def safe_static_path(name: str) -> Optional[Path]:
    """Resout un fichier sous static/, sans traversal."""
    if not name or name.startswith(".") or "/" in name or "\\" in name:
        return None
    candidate = (STATIC_DIR / name).resolve()
    try:
        candidate.relative_to(STATIC_DIR.resolve())
    except ValueError:
        return None
    if not candidate.is_file():
        return None
    return candidate


class SessionStore:
    """Sessions en memoire, optionnellement relues/ecrites en JSON."""

    def __init__(self, persist_path: Optional[Path] = None) -> None:
        self._lock = threading.Lock()
        self._sessions: dict[str, dict[str, Any]] = {}
        self._persist_path = persist_path
        if persist_path is not None:
            self._load_unlocked()

    def clear(self) -> None:
        with self._lock:
            self._sessions = {}
            self._persist_unlocked()

    def create(self, site_id: str) -> dict[str, Any]:
        with self._lock:
            if len(self._sessions) >= MAX_SESSIONS:
                raise OverflowError("trop de sessions")
            session_id = str(uuid.uuid4())
            now = utc_now()
            record = {
                "session_id": session_id,
                "site_id": site_id,
                "status": "open",
                "created_at": now,
                "updated_at": now,
                "messages": [],
            }
            self._sessions[session_id] = record
            self._persist_unlocked()
            return self._public_session(record, include_messages=True)

    def get(self, session_id: str, include_messages: bool = True) -> Optional[dict[str, Any]]:
        with self._lock:
            record = self._sessions.get(session_id)
            if record is None:
                return None
            return self._public_session(record, include_messages=include_messages)

    def list_sessions(self, site_id: Optional[str] = None) -> list[dict[str, Any]]:
        with self._lock:
            rows = []
            for record in self._sessions.values():
                if site_id is not None and record["site_id"] != site_id:
                    continue
                rows.append(self._public_session(record, include_messages=False))
            rows.sort(key=lambda item: item["updated_at"], reverse=True)
            return rows

    def add_visitor_message(self, session_id: str, content: str) -> dict[str, Any]:
        with self._lock:
            record = self._sessions.get(session_id)
            if record is None:
                raise KeyError("session_id inconnu")
            if record["status"] != "open":
                raise PermissionError("session close")
            if len(record["messages"]) + 2 > MAX_MESSAGES:
                raise OverflowError("trop de messages")
            now = utc_now()
            request_id = str(uuid.uuid4())
            visitor_msg = {
                "id": str(uuid.uuid4()),
                "role": "visitor",
                "content": content,
                "created_at": now,
                "request_id": request_id,
                "kind": "user",
            }
            agent_msg = {
                "id": str(uuid.uuid4()),
                "role": "agent",
                "content": stub_reply(content),
                "created_at": now,
                "request_id": request_id,
                "kind": LLM_KIND,
            }
            record["messages"].append(visitor_msg)
            record["messages"].append(agent_msg)
            record["updated_at"] = now
            self._persist_unlocked()
            return {
                "session_id": session_id,
                "request_id": request_id,
                "llm": LLM_KIND,
                "visitor_message": dict(visitor_msg),
                "agent_message": dict(agent_msg),
            }

    @staticmethod
    def _public_session(record: dict[str, Any], include_messages: bool) -> dict[str, Any]:
        payload = {
            "session_id": record["session_id"],
            "site_id": record["site_id"],
            "status": record["status"],
            "created_at": record["created_at"],
            "updated_at": record["updated_at"],
            "message_count": len(record["messages"]),
        }
        if include_messages:
            payload["messages"] = [dict(item) for item in record["messages"]]
        return payload

    def _load_unlocked(self) -> None:
        path = self._persist_path
        if path is None or not path.is_file():
            return
        try:
            raw = json.loads(path.read_text(encoding="utf-8"))
        except (OSError, json.JSONDecodeError, UnicodeDecodeError):
            return
        sessions = raw.get("sessions") if isinstance(raw, dict) else None
        if not isinstance(sessions, dict):
            return
        restored: dict[str, dict[str, Any]] = {}
        for session_id, record in sessions.items():
            if not isinstance(session_id, str) or not SESSION_ID_RE.match(session_id):
                continue
            if not isinstance(record, dict):
                continue
            restored[session_id] = record
        self._sessions = restored

    def _persist_unlocked(self) -> None:
        path = self._persist_path
        if path is None:
            return
        payload = json.dumps({"sessions": self._sessions}, separators=(",", ":"))
        tmp = path.with_suffix(".json.tmp")
        try:
            tmp.write_text(payload, encoding="utf-8")
            tmp.replace(path)
        except OSError:
            try:
                tmp.unlink()
            except OSError:
                pass


class AgentHTTPServer(ThreadingHTTPServer):
    def __init__(self, server_address, RequestHandlerClass):
        super().__init__(server_address, RequestHandlerClass)
        self.store = SessionStore(env_data_path())


class AgentHandler(BaseHTTPRequestHandler):
    server_version = "MOHHDY-Agent/0.2"

    def log_message(self, fmt: str, *args) -> None:
        sys.stderr.write(
            "mohhdy-agent: %s - %s\n" % (self.address_string(), fmt % args)
        )

    def do_GET(self) -> None:
        self._dispatch(method="GET", send_body=True)

    def do_HEAD(self) -> None:
        self._dispatch(method="HEAD", send_body=False)

    def do_POST(self) -> None:
        self._dispatch(method="POST", send_body=True)

    def do_OPTIONS(self) -> None:
        parsed = urlparse(self.path)
        path = self._normalize(parsed.path)
        if path in ("/embed.js", "/embed.css") or path.startswith("/api/sessions"):
            self.send_response(204)
            if path.startswith("/api/sessions"):
                self._write_headers(VISITOR_API_HEADERS)
            elif path == "/embed.css":
                self._write_headers(PUBLIC_CSS_HEADERS)
            else:
                self._write_headers(PUBLIC_JS_HEADERS)
            self.end_headers()
            return
        self._send_json_status(404, {"status": "not_found", "service": SERVICE_NAME})

    def _dispatch(self, method: str, send_body: bool) -> None:
        parsed = urlparse(self.path)
        path = self._normalize(parsed.path)
        query = parse_qs(parsed.query)

        if path.startswith("/api/"):
            self._dispatch_api(method, path, query, send_body)
            return

        if method not in ("GET", "HEAD"):
            self._send_json_status(
                405,
                {"status": "error", "error": "method_not_allowed"},
                extra_headers=TEXT_HEADERS,
            )
            return

        if path == "/health":
            self._send_json(
                {
                    "status": "ok",
                    "service": SERVICE_NAME,
                    "llm": LLM_KIND,
                    "admin_auth": admin_auth_mode(),
                },
                send_body=send_body,
            )
            return
        if path in ("/", "/index.html"):
            self._send_static("index.html", "text/html; charset=utf-8", send_body)
            return
        if path in ("/admin", "/admin/"):
            self._send_static("admin.html", "text/html; charset=utf-8", send_body)
            return
        if path in ("/demo", "/demo/"):
            self._send_static("demo.html", "text/html; charset=utf-8", send_body)
            return
        if path == "/embed.js":
            self._send_static(
                "embed.js",
                "application/javascript; charset=utf-8",
                send_body,
                extra_headers=PUBLIC_JS_HEADERS,
            )
            return
        if path == "/embed.css":
            self._send_static(
                "embed.css",
                "text/css; charset=utf-8",
                send_body,
                extra_headers=PUBLIC_CSS_HEADERS,
            )
            return
        self._send_not_found(send_body=send_body)

    def _dispatch_api(
        self, method: str, path: str, query: dict, send_body: bool
    ) -> None:
        if path == "/api/admin/status":
            if method not in ("GET", "HEAD"):
                self._send_json_status(405, {"status": "error", "error": "method_not_allowed"})
                return
            self._send_json(
                {
                    "auth": admin_auth_mode(),
                    "service": SERVICE_NAME,
                },
                send_body=send_body,
            )
            return

        if path == "/api/admin/sessions" or path.startswith("/api/admin/sessions/"):
            self._dispatch_admin(method, path, query, send_body)
            return

        if path == "/api/sessions":
            if method == "POST":
                self._create_session()
                return
            self._send_json_status(405, {"status": "error", "error": "method_not_allowed"})
            return

        session_match = re.match(
            r"^/api/sessions/(" + SESSION_ID_RE.pattern[1:-1] + r")(/messages)?$",
            path,
        )
        if session_match:
            session_id = session_match.group(1)
            is_messages = session_match.group(2) == "/messages"
            if is_messages:
                if method == "POST":
                    self._post_message(session_id)
                    return
                self._send_json_status(405, {"status": "error", "error": "method_not_allowed"})
                return
            if method in ("GET", "HEAD"):
                self._get_session(session_id, send_body)
                return
            self._send_json_status(405, {"status": "error", "error": "method_not_allowed"})
            return

        self._send_json_status(404, {"status": "not_found", "service": SERVICE_NAME})

    def _dispatch_admin(
        self, method: str, path: str, query: dict, send_body: bool
    ) -> None:
        if method not in ("GET", "HEAD"):
            self._send_json_status(405, {"status": "error", "error": "method_not_allowed"})
            return
        if not self._admin_authorized():
            self._send_json_status(
                401,
                {
                    "status": "unauthorized",
                    "error": "admin_token_required",
                    "message": "Jeton admin manquant ou invalide.",
                },
            )
            return
        if path == "/api/admin/sessions":
            site_raw = query.get("site_id", [None])[0]
            site_id = None
            if site_raw:
                try:
                    site_id = normalize_site_id(site_raw)
                except ValueError:
                    self._send_json_status(
                        400,
                        {"status": "error", "error": "bad_request", "message": "site_id invalide"},
                    )
                    return
            rows = self.server.store.list_sessions(site_id=site_id)
            self._send_json({"sessions": rows, "auth": admin_auth_mode()}, send_body=send_body)
            return
        detail_match = re.match(
            r"^/api/admin/sessions/(" + SESSION_ID_RE.pattern[1:-1] + r")$",
            path,
        )
        if detail_match:
            session = self.server.store.get(detail_match.group(1), include_messages=True)
            if session is None:
                self._send_json_status(
                    404, {"status": "not_found", "error": "unknown_session"}
                )
                return
            self._send_json(session, send_body=send_body)
            return
        self._send_json_status(404, {"status": "not_found", "service": SERVICE_NAME})

    def _create_session(self) -> None:
        payload, error = self._read_json_object()
        if error is not None:
            self._send_json_status(error[0], error[1], extra_headers=VISITOR_API_HEADERS)
            return
        try:
            site_id = normalize_site_id(payload.get("site_id"))
        except ValueError:
            self._send_json_status(
                400,
                {"status": "error", "error": "bad_request", "message": "site_id invalide"},
                extra_headers=VISITOR_API_HEADERS,
            )
            return
        try:
            session = self.server.store.create(site_id)
        except OverflowError:
            self._send_json_status(
                503,
                {"status": "error", "error": "too_many_sessions"},
                extra_headers=VISITOR_API_HEADERS,
            )
            return
        session["llm"] = LLM_KIND
        self._send_json(session, send_body=True, status=201, extra_headers=VISITOR_API_HEADERS)

    def _get_session(self, session_id: str, send_body: bool) -> None:
        session = self.server.store.get(session_id, include_messages=True)
        if session is None:
            self._send_json_status(
                404,
                {"status": "not_found", "error": "unknown_session"},
                extra_headers=VISITOR_API_HEADERS,
            )
            return
        session["llm"] = LLM_KIND
        self._send_json(session, send_body=send_body, extra_headers=VISITOR_API_HEADERS)

    def _post_message(self, session_id: str) -> None:
        payload, error = self._read_json_object()
        if error is not None:
            self._send_json_status(error[0], error[1], extra_headers=VISITOR_API_HEADERS)
            return
        content = payload.get("content")
        if not isinstance(content, str):
            self._send_json_status(
                400,
                {"status": "error", "error": "bad_request", "message": "content texte requis"},
                extra_headers=VISITOR_API_HEADERS,
            )
            return
        content = content.strip()
        if not content:
            self._send_json_status(
                400,
                {"status": "error", "error": "bad_request", "message": "content vide"},
                extra_headers=VISITOR_API_HEADERS,
            )
            return
        if len(content) > MAX_CONTENT_CHARS:
            self._send_json_status(
                400,
                {"status": "error", "error": "bad_request", "message": "content trop long"},
                extra_headers=VISITOR_API_HEADERS,
            )
            return
        try:
            result = self.server.store.add_visitor_message(session_id, content)
        except KeyError:
            self._send_json_status(
                404,
                {"status": "not_found", "error": "unknown_session"},
                extra_headers=VISITOR_API_HEADERS,
            )
            return
        except OverflowError:
            self._send_json_status(
                409,
                {"status": "error", "error": "too_many_messages"},
                extra_headers=VISITOR_API_HEADERS,
            )
            return
        except PermissionError:
            self._send_json_status(
                409,
                {"status": "error", "error": "session_closed"},
                extra_headers=VISITOR_API_HEADERS,
            )
            return
        self._send_json(result, send_body=True, status=201, extra_headers=VISITOR_API_HEADERS)

    def _read_json_object(self) -> tuple[dict, Optional[tuple[int, dict]]]:
        length_raw = self.headers.get("Content-Length", "0") or "0"
        try:
            length = int(length_raw)
        except ValueError:
            return {}, (400, {"status": "error", "error": "bad_request", "message": "Content-Length invalide"})
        if length < 0 or length > MAX_BODY_BYTES:
            return {}, (413, {"status": "error", "error": "payload_too_large"})
        raw = self.rfile.read(length) if length else b""
        if not raw:
            return {}, None
        try:
            payload = json.loads(raw.decode("utf-8"))
        except (json.JSONDecodeError, UnicodeDecodeError):
            return {}, (400, {"status": "error", "error": "bad_request", "message": "JSON invalide"})
        if not isinstance(payload, dict):
            return {}, (400, {"status": "error", "error": "bad_request", "message": "objet JSON requis"})
        return payload, None

    def _extract_admin_token(self) -> str:
        header = self.headers.get("Authorization") or ""
        prefix = "Bearer "
        if header.startswith(prefix):
            return header[len(prefix) :].strip()
        return (self.headers.get("X-Admin-Token") or "").strip()

    def _admin_authorized(self) -> bool:
        expected = env_admin_token()
        if not expected:
            return True
        provided = self._extract_admin_token()
        if not provided or len(provided) != len(expected):
            return False
        return hmac.compare_digest(provided, expected)

    @staticmethod
    def _normalize(path: str) -> str:
        decoded = unquote(path)
        collapsed = posixpath.normpath(decoded)
        if not collapsed.startswith("/"):
            collapsed = "/" + collapsed
        if collapsed != "/" and collapsed.endswith("/"):
            return collapsed.rstrip("/") or "/"
        return collapsed

    def _send_json(
        self,
        payload: dict,
        send_body: bool,
        status: int = 200,
        extra_headers: Optional[dict] = None,
    ) -> None:
        raw = json.dumps(payload, separators=(",", ":")).encode("utf-8")
        self.send_response(status)
        headers = extra_headers or TEXT_HEADERS
        self._write_headers(headers)
        self.send_header("Content-Type", "application/json; charset=utf-8")
        self.send_header("Content-Length", str(len(raw)))
        self.end_headers()
        if send_body:
            self.wfile.write(raw)

    def _send_json_status(
        self, status: int, payload: dict, extra_headers: Optional[dict] = None
    ) -> None:
        self._send_json(payload, send_body=True, status=status, extra_headers=extra_headers)

    def _send_static(
        self,
        name: str,
        content_type: str,
        send_body: bool,
        extra_headers: Optional[dict] = None,
    ) -> None:
        path = safe_static_path(name)
        if path is None:
            self._send_not_found(send_body=send_body)
            return
        data = path.read_bytes()
        self.send_response(200)
        headers = extra_headers or TEXT_HEADERS
        self._write_headers(headers)
        self.send_header("Content-Type", content_type)
        self.send_header("Content-Length", str(len(data)))
        self.end_headers()
        if send_body:
            self.wfile.write(data)

    def _send_not_found(self, send_body: bool) -> None:
        body = b'{"status":"not_found","service":"mohhdy-agent"}'
        self.send_response(404)
        self._write_headers(TEXT_HEADERS)
        self.send_header("Content-Type", "application/json; charset=utf-8")
        self.send_header("Content-Length", str(len(body)))
        self.end_headers()
        if send_body:
            self.wfile.write(body)

    def _write_headers(self, headers: dict) -> None:
        for key, value in headers.items():
            self.send_header(key, value)


def make_server(
    host: Optional[str] = None, port: Optional[int] = None
) -> AgentHTTPServer:
    bind_host = env_host() if host is None else host
    bind_port = env_port() if port is None else port
    httpd = AgentHTTPServer((bind_host, bind_port), AgentHandler)
    httpd.daemon_threads = True
    return httpd


def main() -> None:
    if not STATIC_DIR.is_dir():
        raise SystemExit("repertoire static/ introuvable: %s" % STATIC_DIR)

    httpd = make_server()
    host, port = httpd.server_address[:2]
    sys.stderr.write(
        "mohhdy-agent ASSIST-010/011/040 sur http://%s:%s "
        "(health=/health admin=/admin embed=/embed.js demo=/demo "
        "llm=%s admin_auth=%s)\n"
        % (host, port, LLM_KIND, admin_auth_mode())
    )
    try:
        httpd.serve_forever()
    except KeyboardInterrupt:
        sys.stderr.write("mohhdy-agent: arret\n")
    finally:
        httpd.server_close()


if __name__ == "__main__":
    main()
