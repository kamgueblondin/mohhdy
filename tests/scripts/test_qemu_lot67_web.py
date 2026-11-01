#!/usr/bin/env python3
"""Lot 67: real HTTP/HTTPS from inside Mohhdy, offline and deterministic.

A host test server (scripts/lot67_web_server.py, throw-away test CA from
scripts/lot67_gen_pki.py) listens on 127.0.0.1; the guest reaches it as
10.0.2.2 through QEMU user networking (slirp) and the NE2000 owned by the
Ring 3 networker. The guest shell runs `fetch` (Ring 3 TLS 1.2 client,
chain checked against /etc/ca-roots.pem + /etc/ca-test.pem of the test
initrd) and the GUI /browser renders the same page through the same code.
No Internet access is used. LOT67_REAL=1 adds a manual check against
https://example.com (not run in CI).
"""
from __future__ import print_function

import os
import re
import subprocess
import sys
import time

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
import test_qemu_osui_gui as g  # noqa: E402

ROOT = g.ROOT
PKI = os.path.join(ROOT, "build", "lot67", "pki")
FETCH_TIMEOUT = float(os.environ.get("LOT67_FETCH_TIMEOUT", "120"))
ALIASES = {" ": "spc", "-": "minus", ".": "dot", "/": "slash", "=": "equal", ":": "shift-semicolon",
           "_": "shift-minus", "?": "shift-slash", "&": "shift-7"}
SHOTS = os.environ.get("LOT69_SHOTS", os.path.join(g.LOG_DIR, "lot69-shots"))


def save_png(name, w, h, rgb):
    try:
        from PIL import Image
    except ImportError:
        return None
    os.makedirs(SHOTS, exist_ok=True)
    path = os.path.join(SHOTS, name)
    Image.frombytes("RGB", (w, h), bytes(rgb)).save(path)
    return path


def say(msg):
    print("[lot67-web] %s" % msg, flush=True)


def type_line(mon, proc, text, mode="gets"):
    for ch in text:
        count = 0
        for _ in range(g.KEY_CHAR_RETRIES):
            start = len(g.log_text())
            g.send_key(mon, ALIASES.get(ch, ch.lower()))
            deadline = time.monotonic() + g.KEY_ECHO_TIMEOUT
            while time.monotonic() < deadline:
                if proc.poll() is not None:
                    raise RuntimeError("QEMU stopped")
                time.sleep(g.KEY_DELAY)
                count = g.key_echo_count(g.log_text()[start:], ch.lower(), mode)
                if count:
                    time.sleep(g.KEY_DUPLICATE_SETTLE_DELAY)
                    count = g.key_echo_count(g.log_text()[start:], ch.lower(), mode)
                    break
            if count:
                break
        if count == 0:
            raise RuntimeError("character not received: %r" % ch)
        for _ in range(count - 1):
            g.send_key(mon, "backspace")
            time.sleep(g.KEY_DUPLICATE_SETTLE_DELAY)
    g.send_key(mon, "ret")


def run(mon, proc, line, marker, timeout=None, fail=None):
    start = len(g.log_text())
    type_line(mon, proc, line)
    deadline = time.monotonic() + (timeout or FETCH_TIMEOUT)
    while time.monotonic() < deadline:
        out = g.normalized_log(g.log_text()[start:])
        if marker in out:
            g.wait_for(proc, "[0m : ", g.CMD_TIMEOUT, start)
            return g.normalized_log(g.log_text()[start:])
        for f in (fail or []):
            if f in out:
                raise RuntimeError("%s: got %r\n%s" % (line, f, out[-1500:]))
        if proc.poll() is not None:
            raise RuntimeError("QEMU stopped")
        time.sleep(0.2)
    raise RuntimeError("timeout on %r waiting for %r; tail:\n%s" % (line, marker, g.log_text()[-2500:]))


def main():
    if not os.path.isfile(os.path.join(PKI, "test_ca.pem")):
        subprocess.run([sys.executable, os.path.join(ROOT, "scripts", "lot67_gen_pki.py"), PKI], check=True)
    os.makedirs(g.LOG_DIR, exist_ok=True)
    g.LOG = os.path.join(g.LOG_DIR, "qemu-lot67-web-serial.log")
    g.MON_SOCK = os.path.join(g.LOG_DIR, "qemu-lot67-web-monitor.sock")
    for p in (g.LOG, g.MON_SOCK):
        try:
            os.remove(p)
        except OSError:
            pass
    hp = 30000 + os.getpid() % 10000
    sp = hp + 1
    srv_log = open(os.path.join(g.LOG_DIR, "lot67-web-server.log"), "w")
    srv = subprocess.Popen([sys.executable, os.path.join(ROOT, "scripts", "lot67_web_server.py"), PKI, str(hp), str(sp)],
                           stdout=srv_log, stderr=subprocess.STDOUT)
    proc = mon = None
    t_all = time.monotonic()
    results = []
    try:
        g.prepare_test_disk()
        cpu = os.environ.get("LOT67_CPU", "max")
        with open(os.devnull, "wb") as err:
            proc = subprocess.Popen([
                "qemu-system-i386", "-fw_cfg", "name=opt/mohhdy/ui-scale,string=4", "-cpu", cpu, "-kernel", g.KERNEL, "-initrd", g.INITRD, "-m", "1024M",
                "-display", "none", "-vga", "std", "-serial", "file:" + g.LOG,
                "-monitor", "unix:%s,server,nowait" % g.MON_SOCK, "-machine", "type=pc,accel=tcg",
                "-no-reboot", "-no-shutdown",
                "-netdev", "user,id=n0", "-device", "ne2k_isa,netdev=n0",
            ] + (["-object", "filter-dump,id=f0,netdev=n0,file=" + os.environ["LOT67_PCAP"]]
                 if os.environ.get("LOT67_PCAP") else []) + g.qemu_disk_args(), cwd=ROOT, stdout=err, stderr=err)
            g.wait_for(proc, "[0m : ", g.BOOT_TIMEOUT)
            mon = g.monitor_connect()
            g.wait_for(proc, "net-driver stack ring3 ready", g.CMD_TIMEOUT)
            time.sleep(0.5)
            base = "10.0.2.2"
            checks = [
                ("fetch http://%s:%d/hello" % (base, hp), "fetch ok status 200 bytes 17 ", ["fetch error"], "hello from lot67"),
                ("fetch http://%s:%d/chunked" % (base, hp), "chunk-one chunk-two chunk-three", ["fetch error"], None),
                ("fetch https://%s:%d/redirect" % (base, sp), "fetch body-end", ["fetch error"],
                 "redirects 1 url https://10.0.2.2:%d/page.html tls ECDHE-ECDSA-AES128-GCM-SHA256" % sp),
                ("fetch https://%s:%d/loop" % (base, sp), "fetch error too many redirects", ["fetch ok"], None),
                ("fetch -I https://%s:%d/big" % (base, sp), "fetch ok status 200 bytes 204800 ", ["fetch error"], None),
                ("fetch http://%s:%d/nope" % (base, hp), "fetch ok status 404", ["fetch error"], None),
                # Lot 69: TLS 1.3 (ECDSA and RSA-PSS), P-256 ECDHE, gzip, cross-scheme redirect, big page
                ("fetch https://%s:%d/hello" % (base, sp + 1), "fetch ok status 200 bytes 17 ", ["fetch error"],
                 "tls TLS13-AES256-GCM-SHA384"),
                ("fetch -I https://%s:%d/page.html" % (base, sp + 2), "fetch ok status 200", ["fetch error"],
                 "tls TLS13-AES256-GCM-SHA384"),
                ("fetch -I https://%s:%d/hello" % (base, sp + 3), "fetch ok status 200", ["fetch error"],
                 "tls ECDHE-RSA-AES128-GCM-SHA256"),
                ("fetch http://%s:%d/gzip" % (base, hp), "fetch body-end", ["fetch error"], "fetched over the network by Mohhdy"),
                ("fetch http://%s:%d/to/https/%s/%d/gzip" % (base, hp, base, sp + 1), "fetch body-end",
                 ["fetch error"], "redirects 1 url https://%s:%d/gzip tls TLS13" % (base, sp + 1)),
                ("fetch -I http://%s:%d/biggz" % (base, hp), "fetch ok status 200", ["fetch error"], "truncated 1"),
            ]
            only = os.environ.get("LOT67_ONLY")
            if only:
                checks = [c for c in checks if only in c[0]]
            for line, marker, fail, extra in checks:
                t0 = time.monotonic()
                out = run(mon, proc, line, marker, fail=fail)
                if extra and extra not in out:
                    raise RuntimeError("%s: missing %r in\n%s" % (line, extra, out[-1500:]))
                dt = time.monotonic() - t0
                results.append((line, dt))
                say("ok %-48s %.1fs" % (line, dt))
            out = run(mon, proc, "net-if", "net-if ok", 30)
            m = re.search(r"net-if ok mode \w+ ip [\d.]+ mask [\d.]+ gw ([\d.]+)[^\n]* dns ([\d.]+)", out)
            if not m:
                raise RuntimeError("net-if without gw/dns: %r" % out[-400:])
            say("interface: gw %s dns %s" % (m.group(1), m.group(2)))
            out = g.normalized_log(g.log_text())
            if any("https" in c[0] for c in checks) and ('root "MOHHDY Test Root"' not in out or "dates checked" not in out or "test-ca 1" not in out):
                raise RuntimeError("TLS report incomplete")
            if os.environ.get("LOT67_REAL") == "1":
                out = run(mon, proc, "fetch -v https://example.com/", "fetch body-end", 240, ["fetch error"])
                say("REAL: " + [l for l in out.splitlines() if l.startswith("fetch ok")][0][:300])
            # Lot 69: Internet search, offline: the test server plays a
            # DuckDuckGo-html-like engine; the query is URL-encoded by the guest
            eng = "http://%s:%d/html/?q=" % (base, hp)
            run(mon, proc, "search-engine " + eng, "search-engine ok " + eng, 30)
            out = run(mon, proc, "search mohhdy os & web", "search end", fail=["search error"])
            for want in ("search ok results 3 layout ddg-html status 200", "search 1 Result for mohhdy os & web",
                         "  https://example.org/mohhdy?a=1&b=2", "  Second result with an escaped & snippet."):
                if want not in out:
                    raise RuntimeError("search output missing %r:\n%s" % (want, out[-1500:]))
            out = run(mon, proc, "search-open 1", "render ok", fail=["search-open error"])
            if "search-open url http://%s:%d/page.html" % (base, hp) not in out:
                raise RuntimeError("search-open did not open result 1: %r" % out[-600:])
            run(mon, proc, "search-open 9", "search-open error no result 9", 30)
            run(mon, proc, "search-engine http://%s:%d/block/?q=" % (base, hp), "search-engine ok", 30)
            out = run(mon, proc, "search robots", "search end", fail=["search error"])
            if "blocked bot-check" not in out or "the engine sent a bot check page" not in out:
                raise RuntimeError("bot check page not reported: %r" % out[-600:])
            run(mon, proc, "search-engine " + eng, "search-engine ok", 30)
            srv_text = open(os.path.join(g.LOG_DIR, "lot67-web-server.log")).read()
            if "SEARCH q='mohhdy os & web'" not in srv_text:
                raise RuntimeError("server did not receive the encoded query")
            say("search: console search, search-open, bot check ok")
            if os.environ.get("LOT69_REAL") == "1":
                run(mon, proc, "search-engine ddg", "search-engine ok ddg", 30)
                out = run(mon, proc, "search freestanding c kernel", "search end", 240, ["search error"])
                say("REAL search: " + [l for l in out.splitlines() if l.startswith("search ok") or l.startswith("search note")][0][:300])
                run(mon, proc, "search-engine " + eng, "search-engine ok", 30)
            # the GUI /browser, same code path, page rendered by the engine
            g.send_command_until(mon, "gui", "osui gui live", proc, wait_prompt=False)
            g.wait_for(proc, "OSUI-SNAP", g.CMD_TIMEOUT)
            start = len(g.log_text())
            type_line(mon, proc, "web https://%s:%d/page.html" % (base, sp), mode="getc")
            g.wait_for(proc, "osui web fetch ok status 200", FETCH_TIMEOUT, start)
            g.wait_for(proc, "render ok", 10, start)
            time.sleep(1.5)
            shot = "qemu-lot67-browser.ppm"
            w, h, rgb = g.screendump_rgb(mon, shot)
            say("browser screenshot test_logs/%s %dx%d" % (shot, w, h))
            # Lot 69: search from the desktop, the address bar and the chat shortcut
            for line, label in (("search offline test", "search-gui"), ("web hello world", "search-address-bar"),
                                ("/search chat shortcut", "search-chat")):
                g.send_key(mon, "esc")
                time.sleep(0.8)
                start = len(g.log_text())
                type_line(mon, proc, line, mode="getc")
                g.wait_for(proc, "osui web search ok results 3", FETCH_TIMEOUT, start)
                g.wait_for(proc, "render ok", 10, start)
                time.sleep(1.5)
                w, h, rgb = g.screendump_rgb(mon, "qemu-lot69-%s.ppm" % label)
                say("%s: %s" % (label, save_png(label + ".png", w, h, rgb)))
            # Lot 69: a page that cannot open shows the reason in the window
            # (here: TLS to the plain HTTP port) instead of staying closed
            g.send_key(mon, "esc")
            time.sleep(0.8)
            start = len(g.log_text())
            type_line(mon, proc, "web https://%s:%d/page.html" % (base, hp), mode="getc")
            g.wait_for(proc, "osui web error-page shown", FETCH_TIMEOUT, start)
            if "osui web fetch error=TLS" not in g.normalized_log(g.log_text()[start:]):
                raise RuntimeError("error page without the TLS reason")
            time.sleep(1.5)
            w, h, rgb = g.screendump_rgb(mon, "qemu-lot69-web-error.ppm")
            say("error page: %s" % save_png("web-error-page.png", w, h, rgb))
            if os.environ.get("LOT69_REAL") == "1":
                try:
                    # manual only: the real engine from the desktop (not in CI)
                    g.send_key(mon, "esc")  # the page pane takes the keys; ESC closes it
                    g.wait_for(proc, "osui web close", 10, len(g.log_text()) - 4000)
                    time.sleep(0.5)
                    type_line(mon, proc, "search-engine ddg", mode="getc")
                    time.sleep(2)
                    start = len(g.log_text())
                    type_line(mon, proc, "search freestanding c kernel", mode="getc")
                    deadline = time.monotonic() + 240
                    while time.monotonic() < deadline:
                        m = re.search(r"osui web search (ok|error)[^\n]*", g.log_text()[start:])
                        if m:
                            break
                        time.sleep(1)
                    say("REAL GUI search: %s" % (m.group(0)[:300] if m else "no answer in 240 s"))
                    time.sleep(2)
                    w, h, rgb = g.screendump_rgb(mon, "qemu-lot69-search-real.ppm")
                    say("search-real: %s" % save_png("search-real.png", w, h, rgb))
                except RuntimeError as e:  # manual step, reported not fatal
                    say("REAL GUI search step failed: %s" % e)
            if os.environ.get("LOT67_REAL") == "1":
                g.send_key(mon, "esc")  # the page pane takes the keys; ESC closes it
                g.wait_for(proc, "osui web close", 10, len(g.log_text()) - 4000)
                time.sleep(0.5)
                start = len(g.log_text())
                type_line(mon, proc, "web https://example.com/", mode="getc")
                g.wait_for(proc, "osui web fetch ok status 200", 240, start)
                time.sleep(2)
                g.screendump_rgb(mon, "qemu-lot67-example-com.ppm")
                say("real page screenshot test_logs/qemu-lot67-example-com.ppm")
        srv_reqs = open(os.path.join(g.LOG_DIR, "lot67-web-server.log")).read().count("REQ ")
        say("server saw %d requests; total %.1fs" % (srv_reqs, time.monotonic() - t_all))
        print("[lot67-web] PASS %d fetch checks + GUI /browser" % len(results))
        return 0
    finally:
        g.terminate(proc)
        srv.terminate()
        if mon:
            mon.close()


if __name__ == "__main__":
    try:
        sys.exit(main())
    except Exception as e:  # noqa: BLE001
        print("[lot67-web] FAIL %s" % e)
        sys.exit(1)
