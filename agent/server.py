#!/usr/bin/env python3
"""Scaffold HTTP du runtime agent MOHHDY (ASSIST-050).

Sert l'origine d'embed, une coquille admin et un endpoint de sante.
Ce n'est pas un chat IA, pas des sessions persistantes, pas des actes
navigateur, et ce n'est pas le noyau Multiboot i386.
"""

from __future__ import annotations

import json
import os
import posixpath
import sys
from http.server import BaseHTTPRequestHandler, ThreadingHTTPServer
from pathlib import Path
from typing import Optional
from urllib.parse import unquote, urlparse

SERVICE_NAME = "mohhdy-agent"
DEFAULT_HOST = "0.0.0.0"
DEFAULT_PORT = 8080

ROOT = Path(__file__).resolve().parent
STATIC_DIR = ROOT / "static"

HEALTH_BODY = {
    "status": "ok",
    "service": SERVICE_NAME,
}

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


class AgentHandler(BaseHTTPRequestHandler):
    server_version = "MOHHDY-Agent-Scaffold/0.1"

    def log_message(self, fmt: str, *args) -> None:
        sys.stderr.write(
            "mohhdy-agent: %s - %s\n" % (self.address_string(), fmt % args)
        )

    def do_GET(self) -> None:
        self._dispatch(send_body=True)

    def do_HEAD(self) -> None:
        self._dispatch(send_body=False)

    def do_OPTIONS(self) -> None:
        parsed = urlparse(self.path)
        if self._normalize(parsed.path) == "/embed.js":
            self.send_response(204)
            self._write_headers(PUBLIC_JS_HEADERS)
            self.end_headers()
            return
        self._send_not_found(send_body=True)

    def _dispatch(self, send_body: bool) -> None:
        parsed = urlparse(self.path)
        path = self._normalize(parsed.path)
        if path == "/health":
            self._send_json(HEALTH_BODY, send_body=send_body)
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
        self._send_not_found(send_body=send_body)

    @staticmethod
    def _normalize(path: str) -> str:
        decoded = unquote(path)
        collapsed = posixpath.normpath(decoded)
        if not collapsed.startswith("/"):
            collapsed = "/" + collapsed
        if collapsed != "/" and collapsed.endswith("/"):
            return collapsed.rstrip("/") or "/"
        return collapsed

    def _send_json(self, payload: dict, send_body: bool) -> None:
        raw = json.dumps(payload, separators=(",", ":")).encode("utf-8")
        self.send_response(200)
        self._write_headers(TEXT_HEADERS)
        self.send_header("Content-Type", "application/json; charset=utf-8")
        self.send_header("Content-Length", str(len(raw)))
        self.end_headers()
        if send_body:
            self.wfile.write(raw)

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
        body = (
            b'{"status":"not_found","service":"mohhdy-agent"}'
        )
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
) -> ThreadingHTTPServer:
    bind_host = env_host() if host is None else host
    bind_port = env_port() if port is None else port
    httpd = ThreadingHTTPServer((bind_host, bind_port), AgentHandler)
    httpd.daemon_threads = True
    return httpd


def main() -> None:
    if not STATIC_DIR.is_dir():
        raise SystemExit("repertoire static/ introuvable: %s" % STATIC_DIR)

    httpd = make_server()
    host, port = httpd.server_address[:2]
    sys.stderr.write(
        "mohhdy-agent scaffold ASSIST-050 sur http://%s:%s "
        "(health=/health admin=/admin embed=/embed.js demo=/demo)\n"
        % (host, port)
    )
    try:
        httpd.serve_forever()
    except KeyboardInterrupt:
        sys.stderr.write("mohhdy-agent: arret\n")
    finally:
        httpd.server_close()


if __name__ == "__main__":
    main()
