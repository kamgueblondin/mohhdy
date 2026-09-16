#!/usr/bin/env python3
"""Host smoke: HTML desktop assets + thin display_host (no QEMU).

Hors make integration-qemu. Ne reimplemente pas le cerveau OS-UI.
"""
from __future__ import print_function

import json
import os
import sys
import threading
import time

try:
    from urllib.request import urlopen
except ImportError:
    from urllib2 import urlopen  # type: ignore

ROOT = os.path.abspath(os.path.join(os.path.dirname(__file__), "..", ".."))
sys.path.insert(0, os.path.join(ROOT, "osui"))
import display_host  # noqa: E402


def fail(msg):
    sys.stderr.write("FAIL: %s\n" % msg)
    sys.exit(1)


def read(rel):
    path = os.path.join(ROOT, rel)
    if not os.path.isfile(path):
        fail("missing %s" % rel)
    return open(path, "r", encoding="utf-8").read()


def test_assets():
    html = read("osui/static/index.html")
    css = read("osui/static/os.css")
    js = read("osui/static/os.js")
    for needle in (
        "Mohhdy OS",
        "Browser-OS",
        "Shell Multiboot",
        "Scene IA",
        "reflexion",
        "action",
        "resultats",
        "llm=stub_echo",
        "us031_complete=false",
        "data-testid=\"os-chat\"",
        "os-dock",
        "bootstrap graphique",
        "os-icon-glyph",
    ):
        if needle not in html:
            fail("index.html missing %r" % needle)
    if "backdrop-filter" not in css:
        fail("os.css missing glassmorphic backdrop-filter")
    if ".os-chat" not in css or ".os-icons" not in css:
        fail("os.css missing chat/dock chrome")
    if "inset: 0" not in css and "inset:0" not in css:
        fail("os.css ai-stage must cover the desktop (inset 0)")
    if "/api/sessions" in js:
        fail("os.js must not call retired agent session APIs")
    if "/api/line" not in js or "/api/state" not in js:
        fail("os.js must proxy to guest via /api/state and /api/line")
    snap = read("osui/static/fixture-snap.txt")
    parsed = display_host.parse_snap_stream(snap)
    if not parsed:
        fail("fixture snap did not parse")
    if parsed.get("chrome") != "html_host":
        fail("fixture chrome=%s" % parsed.get("chrome"))
    if parsed.get("chat_mode") != "center":
        fail("fixture chat_mode")
    if parsed.get("us031_complete") is not False:
        fail("honesty us031")


def test_http_fixture():
    port = display_host.free_port()
    bind = "127.0.0.1"
    state = display_host.DisplayState(os.path.join(ROOT, "osui", "static", "fixture-snap.txt"))
    httpd = display_host.ThreadingHTTPServer((bind, port), display_host.make_handler(state, display_host.STATIC))
    thread = threading.Thread(target=httpd.serve_forever, daemon=True)
    thread.start()
    time.sleep(0.15)
    try:
        html = urlopen("http://%s:%d/" % (bind, port), timeout=3).read().decode("utf-8", "replace")
        if "Mohhdy OS" not in html or "Scene IA" not in html:
            fail("GET / did not serve desktop HTML")
        css = urlopen("http://%s:%d/os/os.css" % (bind, port), timeout=3).read().decode("utf-8", "replace")
        if "os-chat" not in css:
            fail("GET /os/os.css missing")
        health = json.loads(urlopen("http://%s:%d/health" % (bind, port), timeout=3).read().decode("utf-8"))
        if health.get("python_facade") is not False:
            fail("health python_facade")
        if health.get("us031_complete") is not False:
            fail("health us031")
        if health.get("llm") != "stub_echo":
            fail("health llm")
        st = json.loads(urlopen("http://%s:%d/api/state" % (bind, port), timeout=3).read().decode("utf-8"))
        if st.get("chrome") != "html_host":
            fail("state chrome")
        if st.get("chat_mode") != "center":
            fail("state chat_mode")
    finally:
        httpd.shutdown()
        httpd.server_close()


def main():
    test_assets()
    test_http_fixture()
    print("OK osui display host + HTML desktop assets")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
