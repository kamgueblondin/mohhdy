#!/usr/bin/env python3
"""Lot 69: real-site compatibility campaign (MANUAL, needs Internet).

Boots Mohhdy headless (-display none, own monitor socket) with
-netdev user + ne2k_isa + -cpu max and runs the shared guest `fetch`
command against a list of public sites. Prints one table row per site
(DNS / TCP / TLS / HTTP / title) and writes the table to
test_logs/lot69-web-campaign.md. Not run in CI: it depends on the
Internet. Usage: LOT69_WEB_REAL=1 python3 tests/scripts/lot69_web_campaign.py [URL...]
LOT69_WEB_SHOTS=1 also renders google.com and wikipedia in the GUI
/browser and saves screenshots (LOT69_SHOTS dir).
"""
from __future__ import print_function

import os
import re
import subprocess
import sys
import time

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
import test_qemu_osui_gui as g  # noqa: E402
import test_qemu_lot67_web as w  # noqa: E402

SITES = [
    "https://google.com/", "https://www.google.com/", "https://wikipedia.org/", "https://fr.wikipedia.org/",
    "https://en.wikipedia.org/wiki/Operating_system", "https://github.com/", "https://duckduckgo.com/",
    "https://html.duckduckgo.com/html/?q=mohhdy", "https://www.bing.com/", "https://www.cloudflare.com/",
    "https://www.mozilla.org/", "https://www.lemonde.fr/", "https://www.bbc.com/", "https://example.com/",
    "https://httpbin.org/get", "https://httpbin.org/gzip", "https://httpbin.org/redirect/3",
    "https://httpbin.org/stream/5", "http://httpbin.org/get", "http://example.com/", "http://neverssl.com/",
    "https://rsa2048.badssl.com/", "https://ecc256.badssl.com/", "https://ecc384.badssl.com/",
    "https://rsa4096.badssl.com/", "https://expired.badssl.com/", "https://wrong.host.badssl.com/",
    "https://self-signed.badssl.com/", "https://www.youtube.com/", "https://www.amazon.com/",
    "https://www.reddit.com/", "https://news.ycombinator.com/", "https://www.apple.com/",
    "https://www.microsoft.com/", "https://x.com/", "https://www.gov.uk/", "https://www.lexpress.mu/",
]


def fetch_row(mon, proc, url):
    start = len(g.log_text())
    w.type_line(mon, proc, "fetch -n 200 " + url)
    deadline = time.monotonic() + float(os.environ.get("LOT69_SITE_TIMEOUT", "240"))
    while time.monotonic() < deadline:
        out = g.normalized_log(g.log_text()[start:])
        if "fetch body-end" in out or "fetch error" in out:
            g.wait_for(proc, "[0m : ", g.CMD_TIMEOUT, start)
            break
        if proc.poll() is not None:
            raise RuntimeError("QEMU stopped")
        time.sleep(0.3)
    else:
        out = g.normalized_log(g.log_text()[start:])
        return {"url": url, "result": "TIMEOUT", "detail": out[-300:].replace("\n", " | ")}
    out = g.normalized_log(g.log_text()[start:])
    row = {"url": url, "dns": "?", "tcp": "?", "tls": "-", "http": "-", "title": "", "result": "", "detail": ""}
    row["dns"] = "fail" if "DNS: cannot resolve" in out else "ok"
    row["tcp"] = "ok" if "connect " in out and "TCP connect failed" not in out else ("fail" if "TCP connect failed" in out else "?")
    m = re.search(r"tls ok: (\S+)", out)
    if m:
        row["tls"] = m.group(1)
    m = re.search(r"fetch ok (.*)", out)
    if m:
        row["result"] = "OK"
        st = re.search(r"status (\d+)", m.group(1))
        row["http"] = st.group(1) if st else "?"
        row["detail"] = m.group(1)[:160]
        t = re.search(r"fetch title (.*)", out)
        row["title"] = t.group(1)[:50] if t else ""
    else:
        e = re.search(r"fetch error (.*)", out)
        row["result"] = "FAIL"
        row["detail"] = e.group(1)[:160] if e else out[-200:]
        if "TLS" in row["detail"]:
            row["tls"] = "fail"
    return row


def main():
    if os.environ.get("LOT69_WEB_REAL") != "1":
        print("[lot69-web-campaign] skipped: set LOT69_WEB_REAL=1 (needs Internet)")
        return 0
    sites = sys.argv[1:] or SITES
    os.makedirs(g.LOG_DIR, exist_ok=True)
    g.LOG = os.path.join(g.LOG_DIR, "qemu-lot69-web-campaign-serial.log")
    g.MON_SOCK = os.path.join(g.LOG_DIR, "qemu-lot69-web-campaign-monitor.sock")
    for p in (g.LOG, g.MON_SOCK):
        try:
            os.remove(p)
        except OSError:
            pass
    g.prepare_test_disk()
    proc = mon = None
    rows = []
    try:
        with open(os.devnull, "wb") as err:
            proc = subprocess.Popen([
                "qemu-system-i386", "-cpu", os.environ.get("LOT67_CPU", "max"), "-kernel", g.KERNEL,
                "-initrd", g.INITRD, "-m", "1024M", "-display", "none", "-vga", "std",
                "-serial", "file:" + g.LOG, "-monitor", "unix:%s,server,nowait" % g.MON_SOCK,
                "-machine", "type=pc,accel=tcg", "-no-reboot", "-no-shutdown",
                "-netdev", "user,id=n0", "-device", "ne2k_isa,netdev=n0"] + g.qemu_disk_args(),
                cwd=g.ROOT, stdout=err, stderr=err)
            g.wait_for(proc, "[0m : ", g.BOOT_TIMEOUT)
            mon = g.monitor_connect()
            g.wait_for(proc, "net-driver stack ring3 ready", g.CMD_TIMEOUT)
            time.sleep(0.5)
            for url in sites:
                t0 = time.monotonic()
                r = fetch_row(mon, proc, url)
                r["secs"] = "%.1f" % (time.monotonic() - t0)
                rows.append(r)
                print("[lot69-web-campaign] %-48s %-7s %s" % (url, r["result"], r.get("detail", "")[:120]), flush=True)
            if os.environ.get("LOT69_WEB_SHOTS") == "1":
                g.send_command_until(mon, "gui", "osui gui live", proc, wait_prompt=False)
                g.wait_for(proc, "OSUI-SNAP", g.CMD_TIMEOUT)
                for name, url in (("web-google", "https://www.google.com/"), ("web-wikipedia", "https://fr.wikipedia.org/")):
                    start = len(g.log_text())
                    w.type_line(mon, proc, "web " + url, mode="getc")
                    try:
                        g.wait_for(proc, "osui web fetch ok", 300, start)
                        g.wait_for(proc, "render ok", 60, start)
                    except Exception as e:  # noqa: BLE001
                        print("[lot69-web-campaign] GUI %s: %s" % (url, str(e)[:200]))
                    time.sleep(3)
                    wd, ht, rgb = g.screendump_rgb(mon, "qemu-lot69-%s.ppm" % name)
                    print("[lot69-web-campaign] screenshot %s" % w.save_png(name + ".png", wd, ht, rgb))
                    g.send_key(mon, "esc")
                    time.sleep(1)
    finally:
        if proc:
            g.terminate(proc)
        if mon:
            mon.close()
    md = ["| URL | DNS | TCP | TLS | HTTP | Title | Result | Time s | Detail |", "|---|---|---|---|---|---|---|---|---|"]
    for r in rows:
        md.append("| %s | %s | %s | %s | %s | %s | %s | %s | %s |" % (
            r["url"], r.get("dns", "?"), r.get("tcp", "?"), r.get("tls", "-"), r.get("http", "-"),
            r.get("title", "").replace("|", "/"), r["result"], r.get("secs", ""), r.get("detail", "").replace("|", "/")))
    path = os.path.join(g.LOG_DIR, "lot69-web-campaign.md")
    open(path, "w").write("\n".join(md) + "\n")
    print("[lot69-web-campaign] table: %s (%d sites, %d ok)" % (path, len(rows), sum(1 for r in rows if r["result"] == "OK")))
    return 0


if __name__ == "__main__":
    sys.exit(main())
