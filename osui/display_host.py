#!/usr/bin/env python3
"""Thin host display helper for Mohhdy OS.

Serves osui/static (HTML/CSS/JS) and proxies lines to the guest via serial.
Does NOT implement chat, sessions, MCP, FS or stage logic. Those live in
userspace/osui_runtime.c. Honesty: python_facade=false, display_host=true,
llm=stub_echo, us031_complete=false, guest_html_stage=false.

Usage:
  python3 osui/display_host.py              # QEMU + http://127.0.0.1:18080
  python3 osui/display_host.py --open
  python3 osui/display_host.py --no-qemu    # static + fixture (UI smoke)
"""
from __future__ import print_function

import argparse
import json
import os
import socket
import subprocess
import sys
import threading
import time
import webbrowser

try:
    from http.server import BaseHTTPRequestHandler, ThreadingHTTPServer
except ImportError:
    from BaseHTTPServer import BaseHTTPRequestHandler  # type: ignore
    from SocketServer import ThreadingMixIn, TCPServer  # type: ignore

    class ThreadingHTTPServer(ThreadingMixIn, TCPServer):
        allow_reuse_address = True

ROOT = os.path.abspath(os.path.join(os.path.dirname(__file__), ".."))
STATIC = os.path.join(os.path.dirname(__file__), "static")
DEFAULT_FIXTURE = os.path.join(STATIC, "fixture-snap.txt")

HONEST = {
    "chrome": "html_host",
    "display_surface": "html_host",
    "llm": "stub_echo",
    "us031_complete": False,
    "python_facade": False,
    "phase3_complete": False,
    "guest_html_stage": False,
    "display_host": True,
    "live": False,
}


def parse_kv(blob):
    out = {}
    token = ""
    key = None
    i = 0
    while i <= len(blob):
        ch = blob[i] if i < len(blob) else " "
        if key is None:
            if ch == "=":
                key = token
                token = ""
            elif ch != " ":
                token += ch
            else:
                token = ""
        else:
            if ch == " " or i == len(blob):
                out[key] = token
                key = None
                token = ""
            else:
                token += ch
        i += 1
    return out


def parse_snap_stream(text):
    """Return the latest complete OSUI-SNAP block as a dict."""
    latest = None
    current = None
    messages = []
    for raw in text.splitlines():
        line = raw.strip("\r")
        if line.startswith("OSUI-SNAP "):
            header = line[len("OSUI-SNAP "):]
            input_val = ""
            marker = " input="
            idx = header.rfind(marker)
            if idx >= 0:
                input_val = header[idx + len(marker):]
                header = header[:idx]
            current = parse_kv(header)
            current["input"] = input_val
            messages = []
        elif line.startswith("OSUI-MSG ") and current is not None:
            parts = line.split(" ", 2)
            messages.append(parts[2] if len(parts) > 2 else "")
        elif line.startswith("OSUI-END") and current is not None:
            state = dict(HONEST)
            state.update({
                "chat_mode": current.get("chat_mode", "center"),
                "pane": current.get("pane", "none"),
                "stage_mode": current.get("stage", "reflecting"),
                "stage_kind": current.get("kind", "plan"),
                "session_id": current.get("session", "s0001"),
                "chat_x": int(current.get("chat_x") or 0),
                "chat_y": int(current.get("chat_y") or 0),
                "input": current.get("input", ""),
                "messages": list(messages),
                "llm": current.get("llm", "stub_echo"),
                "chrome": current.get("chrome", "html_host"),
                "display_surface": current.get("display_surface", "html_host"),
                "live": True,
            })
            latest = state
            current = None
    return latest


MIME = {
    ".html": "text/html; charset=utf-8",
    ".css": "text/css; charset=utf-8",
    ".js": "application/javascript; charset=utf-8",
    ".txt": "text/plain; charset=utf-8",
    ".svg": "image/svg+xml",
    ".png": "image/png",
}


class DisplayState(object):
    def __init__(self, fixture_path):
        self.lock = threading.Lock()
        self.serial_buf = ""
        self.state = dict(HONEST)
        self.state.update({
            "chat_mode": "center",
            "pane": "none",
            "stage_mode": "reflecting",
            "stage_kind": "plan",
            "session_id": "s0001",
            "chat_x": 16,
            "chat_y": 5,
            "input": "",
            "messages": [],
        })
        self.serial_sock = None
        self.qemu = None
        self.ready = False
        if fixture_path and os.path.isfile(fixture_path):
            text = open(fixture_path, "r", encoding="utf-8").read()
            parsed = parse_snap_stream(text)
            if parsed:
                self.state = parsed
                self.state["live"] = False

    def ingest(self, chunk):
        if not chunk:
            return
        with self.lock:
            if isinstance(chunk, bytes):
                chunk = chunk.decode("utf-8", "replace")
            self.serial_buf += chunk
            if len(self.serial_buf) > 200000:
                self.serial_buf = self.serial_buf[-80000:]
            parsed = parse_snap_stream(self.serial_buf)
            if parsed:
                self.state = parsed
                self.ready = True

    def snapshot(self):
        with self.lock:
            return dict(self.state)

    def send_line(self, line):
        line = (line or "").replace("\r", " ").replace("\n", " ").strip()
        if not line:
            return False
        sock = self.serial_sock
        if sock is None:
            return False
        payload = line + "\n"
        sock.sendall(payload.encode("ascii", "replace"))
        return True


def make_handler(state, static_dir):
    class Handler(BaseHTTPRequestHandler):
        def log_message(self, fmt, *args):
            sys.stderr.write("[display-host] " + (fmt % args) + "\n")

        def _send(self, code, body, content_type="application/json; charset=utf-8"):
            data = body if isinstance(body, bytes) else body.encode("utf-8")
            self.send_response(code)
            self.send_header("Content-Type", content_type)
            self.send_header("Content-Length", str(len(data)))
            self.send_header("Cache-Control", "no-store")
            self.end_headers()
            self.wfile.write(data)

        def do_GET(self):
            path = self.path.split("?", 1)[0]
            if path == "/api/state" or path == "/api/os":
                self._send(200, json.dumps(state.snapshot()))
                return
            if path == "/health":
                body = dict(HONEST)
                body.update({"ok": True, "service": "mohhdy-os-display-host"})
                self._send(200, json.dumps(body))
                return
            rel = path
            if rel in ("/", "/os", "/os/", "/index.html"):
                rel = "/index.html"
            if rel.startswith("/os/"):
                rel = rel[3:]
            if rel == "/demo-app":
                rel = "/demo-app.html"
            rel = rel.lstrip("/")
            full = os.path.normpath(os.path.join(static_dir, rel))
            if not full.startswith(os.path.abspath(static_dir) + os.sep) and full != os.path.abspath(static_dir):
                self._send(403, json.dumps({"error": "forbidden"}))
                return
            if not os.path.isfile(full):
                self._send(404, json.dumps({"error": "not found", "path": rel}))
                return
            ext = os.path.splitext(full)[1]
            with open(full, "rb") as handle:
                self._send(200, handle.read(), MIME.get(ext, "application/octet-stream"))

        def do_POST(self):
            path = self.path.split("?", 1)[0]
            length = int(self.headers.get("Content-Length") or 0)
            raw = self.rfile.read(length) if length else b"{}"
            try:
                payload = json.loads(raw.decode("utf-8") or "{}")
            except ValueError:
                payload = {}
            line = payload.get("line") or payload.get("text") or payload.get("prompt") or ""
            if path in ("/api/line", "/api/os/prompt", "/api/os/shell"):
                ok = state.send_line(line)
                self._send(200, json.dumps({
                    "ok": ok,
                    "proxied": ok,
                    "llm": "stub_echo",
                    "python_facade": False,
                    "note": "line forwarded to guest C" if ok else "no guest serial (fixture mode)",
                }))
                return
            self._send(404, json.dumps({"error": "unknown route", "path": path}))

    return Handler


def free_port():
    sock = socket.socket()
    sock.bind(("127.0.0.1", 0))
    port = sock.getsockname()[1]
    sock.close()
    return port


def wait_connect(host, port, timeout=15):
    deadline = time.time() + timeout
    last = None
    while time.time() < deadline:
        sock = socket.socket()
        sock.settimeout(1)
        try:
            sock.connect((host, port))
            sock.settimeout(0.3)
            return sock
        except OSError as err:
            last = err
            try:
                sock.close()
            except OSError:
                pass
            time.sleep(0.15)
    raise RuntimeError("serial TCP connect failed: %s" % last)


def reader_loop(state, sock, stop):
    while not stop.is_set():
        try:
            chunk = sock.recv(4096)
        except socket.timeout:
            continue
        except OSError:
            break
        if not chunk:
            time.sleep(0.05)
            continue
        state.ingest(chunk)


def start_qemu(args, serial_port):
    kernel = args.kernel
    initrd = args.initrd
    disk = args.disk
    cmd = [
        "qemu-system-i386",
        "-cpu", "pentium3",
        "-kernel", kernel,
        "-initrd", initrd,
        "-m", args.ram,
        "-display", "none",
        "-vga", "std",
        "-serial", "tcp:127.0.0.1:%d,server=on,wait=off" % serial_port,
        "-machine", "type=pc,accel=tcg",
        "-no-reboot",
        "-no-shutdown",
    ]
    if disk and os.path.isfile(disk):
        cmd += ["-drive", "file=%s,format=raw,if=ide,cache=writethrough" % disk]
    err_path = os.path.join(ROOT, "test_logs", "display-host-qemu.err")
    os.makedirs(os.path.dirname(err_path), exist_ok=True)
    err = open(err_path, "ab")
    proc = subprocess.Popen(cmd, cwd=ROOT, stdout=err, stderr=err)
    return proc, err


def auto_gui(state, sock, stop):
    deadline = time.time() + 90
    while time.time() < deadline and not stop.is_set():
        with state.lock:
            buf = state.serial_buf
        if "MOHHDY>" in buf or "(-.-)" in buf or "SYS_GETS: Debut" in buf:
            time.sleep(0.8)
            try:
                sock.sendall(b"gui\n")
            except OSError:
                return
            return
        time.sleep(0.2)


def main(argv=None):
    parser = argparse.ArgumentParser(description="Mohhdy HTML display host (thin proxy)")
    parser.add_argument("--bind", default="127.0.0.1")
    parser.add_argument("--port", type=int, default=int(os.environ.get("MOHHDY_DISPLAY_PORT", "18080")))
    parser.add_argument("--no-qemu", action="store_true", help="static assets + fixture, no guest")
    parser.add_argument("--open", action="store_true", help="open a browser")
    parser.add_argument("--kernel", default=os.path.join(ROOT, "build", "mohhdy.bin"))
    parser.add_argument("--initrd", default=os.path.join(ROOT, "my_initrd.tar"))
    parser.add_argument("--disk", default=os.path.join(ROOT, "build", "overlay.img"))
    parser.add_argument("--ram", default=os.environ.get("MOHHDY_RAM", "256M"))
    parser.add_argument("--fixture", default=DEFAULT_FIXTURE)
    parser.add_argument("--no-auto-gui", action="store_true")
    args = parser.parse_args(argv)

    state = DisplayState(args.fixture if args.no_qemu else None)
    stop = threading.Event()
    qemu = None
    errf = None
    serial_sock = None

    if not args.no_qemu:
        if not os.path.isfile(args.kernel) or not os.path.isfile(args.initrd):
            sys.stderr.write("missing kernel/initrd; run make all first\n")
            return 1
        serial_port = free_port()
        qemu, errf = start_qemu(args, serial_port)
        state.qemu = qemu
        serial_sock = wait_connect("127.0.0.1", serial_port)
        state.serial_sock = serial_sock
        threading.Thread(target=reader_loop, args=(state, serial_sock, stop), daemon=True).start()
        if not args.no_auto_gui:
            threading.Thread(target=auto_gui, args=(state, serial_sock, stop), daemon=True).start()

    httpd = ThreadingHTTPServer((args.bind, args.port), make_handler(state, STATIC))
    url = "http://%s:%d/" % (args.bind, args.port)
    sys.stdout.write("Mohhdy display host %s\n" % url)
    sys.stdout.write("honesty: llm=stub_echo us031_complete=false python_facade=false display_host=true\n")
    sys.stdout.write("brain: userspace/osui_runtime.c  surface: osui/static\n")
    sys.stdout.flush()
    if args.open:
        try:
            webbrowser.open(url)
        except Exception:
            pass

    try:
        httpd.serve_forever()
    except KeyboardInterrupt:
        pass
    finally:
        stop.set()
        httpd.server_close()
        if serial_sock is not None:
            try:
                serial_sock.close()
            except OSError:
                pass
        if qemu is not None and qemu.poll() is None:
            qemu.terminate()
            try:
                qemu.wait(timeout=4)
            except Exception:
                qemu.kill()
        if errf is not None:
            errf.close()
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
