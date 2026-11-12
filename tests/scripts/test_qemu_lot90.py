#!/usr/bin/env python3
"""Lot 90 in QEMU, offline. Two boots; the Lot 67 test server runs on the
host and the guest reaches it as 10.0.2.2 (slirp).

Boot 1, ui-scale 1x:
  * reader backgrounds and fonts (/lot90/bg.html): the background shorthand
    with a gradient is split into colour, image, position, size and repeat;
    list-style keeps its image and position; calc(1.5em + 2px) against a
    16 px parent is 26px, 2rem with html at 20px is 40px, a 75-byte
    clamp() is kept and resolves to 18.5px.
  * @property and declaration-time values (/lot90/props.html): --dbl90 is
    computed on the root (20px) and not re-expanded where --base90 changes;
    a registered --sz90 (inherits: false, initial 15px) is not inherited
    from a parent's style attribute and a wrong-type value falls back to
    the initial value; the 2em initial-value is refused.
  * @scope (/lot90/scope.html): a seven-selector start list keeps six (the
    sixth hides its p, the seventh does not); the nearer of two roots nine
    and eleven generations up wins (Lot 89 capped both at six: the later
    rule won).
  * autofill: status, a time zone, automatic expiry, per-value ages; the
    public suffix rules: x.city.kawasaki.jp shares its site with
    y.city.kawasaki.jp (exception rule), a.b.ck does not share one with
    c.b.ck (wildcard rule).
  * mesh shaping queue (single node): p2p-shape weight, age and cap
    settings show in the status line; a bad value is refused.
  * NE2000 idle drain: the worker's rxq line is well formed after pages
    (held = in - out, held <= peak, nothing oversize).
Boot 2, a TV EDID through fw_cfg, no scale pin (1.5x from the TV hint):
  * the kernel logs the TV safe area (2.5% per side, the EDID has no
    underscan bit) and display-info says the desktop keeps it clear;
  * the window chrome of every window type works at 1.5x and the guest's
    hit test reports title bars of 45 px (51 for the chat) instead of 30
    and 34: the title bars follow the chrome scale.
"""
from __future__ import print_function

import os
import re
import subprocess
import sys
import time

HERE = os.path.dirname(os.path.abspath(__file__))
sys.path.insert(0, HERE)
LOGS = os.path.join(HERE, "..", "..", "test_logs")
os.environ.setdefault("OSUI_GUI_LOG", os.path.join(LOGS, "qemu-lot90-serial.log"))
os.environ.setdefault("OSUI_GUI_MON_SOCK", "/tmp/mohhdy-lot90-mon.sock")
os.environ.setdefault("OSUI_GUI_QMP_SOCK", "/tmp/mohhdy-lot90-qmp.sock")
os.environ.setdefault("OVERLAY_DISK", os.path.join(LOGS, "qemu-lot90-overlay.img"))
import test_qemu_osui_gui as g  # noqa: E402
import test_qemu_lot67_web as lw  # noqa: E402
import test_qemu_lot88 as l88  # noqa: E402  (boot, stop, need, refuse, keep_log, tv_edid)
import test_qemu_lot88_chrome as chrome  # noqa: E402

ROOT = g.ROOT
SRV_LOG = os.path.join(g.LOG_DIR, "lot90-web-server.log")
need, refuse = l88.need, l88.refuse
RXQ_RE = re.compile(r"net-driver nic rxq held (\d+) peak (\d+) in (\d+) out (\d+) full (\d+) "
                    r"oversize (\d+) drains (\d+) idle (\d+) end")


def say(msg):
    print("[lot90] %s" % msg, flush=True)


def css_page(mon, proc, u, page, fin, keep, hide=()):
    lw.run(mon, proc, "web-css on", "web-css on", 20)
    lw.run(mon, proc, "web-open %s/lot90/%s" % (u, page), "osui web page sections", fail=["error="])
    css = lw.run(mon, proc, "web-css", "css-props ", 30)
    read = lw.run(mon, proc, "web-read", fin, 30)
    need(read, list(keep), "web-read " + page)
    refuse(read, list(hide), "web-read " + page)
    lw.run(mon, proc, "web-css off", "web-css off", 20)
    return css


def step_backgrounds(mon, proc, u):
    css = css_page(mon, proc, u, "bg.html", "fin90b", ["bgkeep90", "lskeep90", "emkeep90", "remkeep90", "longkeep90"])
    need(css, ["node html display auto size 20px",
               "node div #bg90 display auto bg #102030 bgimg linear-gradient(to right, #ff0000, #0000ff) "
               "bgpos center center bgsize cover bgrep no-repeat",
               "node ul #ls90 display auto list square limg url(dot90.png) lpos inside",
               "node p #em90 display auto size 26px",
               "node p #rem90 display auto size 40px",
               "node p #long90 display auto size 18.5px"], "web-css backgrounds and fonts")
    say("reader: background shorthand split (gradient, center / cover, no-repeat), list image inside, "
        "calc(1.5em + 2px) 26px, 2rem 40px, long clamp() 18.5px")


def step_props(mon, proc, u):
    css = css_page(mon, proc, u, "props.html", "fin90p", ["proot90", "pdecl90", "preg90", "pwrong90"])
    need(css, ["node p #root90 display auto size 20px",
               "node p #decl90 display auto size 20px",
               "node p #reg90 display auto size 15px",
               "node p #wrong90 display auto size 20px indent 15px",
               "css-props registered 1 rejected 1 computed 1 invalid 0 unset 1 initial 11"], "web-css @property")
    say("@property: --dbl90 computed where declared (20px, not 60px), --sz90 not inherited (15px), "
        "red for a <length> falls back to 15px, 2em initial-value refused")


def step_scope(mon, proc, u):
    css = css_page(mon, proc, u, "scope.html", "fin90s", ["sevenkeep90", "proxkeep90"], ["sixhide90"])
    need(css, ["node p #prox90 display auto color #00aa00", "css-scope scopes 3 rules 3"], "web-css @scope")
    say("@scope: six of seven start selectors kept, the root nine generations up beat the one eleven up")


def step_autofill(mon, proc):
    out = lw.run(mon, proc, "autofill status", "autofill status ", 20)
    m = re.search(r"autofill status expire 0 tz 0 today (\d+) touched \d+ expired 0 keys \d+ values \d+", out)
    if not m or int(m.group(1)) < 20000:
        raise RuntimeError("autofill status: %r" % out[-400:])
    for host, val in (("x.city.kawasaki.jp", "city90"), ("a.b.ck", "ck90"), ("www.example.net", "net90")):
        out = lw.run(mon, proc, "autofill add %s user %s" % (host, val), "autofill ", 20)
        need(out, ["autofill added"], "autofill add " + host)
    out = lw.run(mon, proc, "autofill offers https://y.city.kawasaki.jp/ user", "autofill offers ", 20)
    need(out, ["autofill offer 0 city90 site", "autofill offers 1"], "PSL exception rule")
    out = lw.run(mon, proc, "autofill offers https://c.b.ck/ user", "autofill offers ", 20)
    need(out, ["autofill offers 0"], "PSL wildcard rule")
    refuse(out, ["ck90"], "PSL wildcard rule")
    out = lw.run(mon, proc, "autofill tz 240", "autofill tz ", 20)
    need(out, ["autofill tz +240 today "], "autofill tz")
    out = lw.run(mon, proc, "autofill tz 900", "autofill ", 20)
    need(out, ["autofill error usage: autofill tz MINUTES (-720..840) | utc"], "autofill tz bound")
    out = lw.run(mon, proc, "autofill expire 30", "autofill expire ", 20)
    need(out, ["autofill expire 30 forgot 0"], "autofill expire 30")
    out = lw.run(mon, proc, "autofill ages values", "www.example.net user 0 ", 20)
    if not re.search(r"autofill value www\.example\.net user 0 [01]d", out):
        raise RuntimeError("autofill ages values: %r" % out[-600:])
    refuse(out, ["net90", "city90", "undated"], "autofill ages values")
    out = lw.run(mon, proc, "autofill status", "autofill status ", 20)
    need(out, ["autofill status expire 30 tz 240 "], "autofill status after settings")
    out = lw.run(mon, proc, "autofill expire off", "autofill expire ", 20)
    need(out, ["autofill expire off forgot 0"], "autofill expire off")
    lw.run(mon, proc, "autofill tz utc", "autofill tz ", 20)
    say("autofill: status, tz +240, expire 30 then off, per-value ages; PSL exception and wildcard rules")


def run_getc(mon, proc, line, marker, timeout=20):
    """while the mesh is up the shell reads keys with SYS_GETC (shell.c)"""
    start = len(g.log_text())
    lw.type_line(mon, proc, line, mode="getc")
    deadline = time.monotonic() + timeout
    while time.monotonic() < deadline:
        out = g.normalized_log(g.log_text()[start:])
        if marker in out:
            g.wait_for(proc, "[0m : ", g.CMD_TIMEOUT, start)
            return g.normalized_log(g.log_text()[start:])
        if proc.poll() is not None:
            raise RuntimeError("QEMU stopped")
        time.sleep(0.2)
    raise RuntimeError("timeout on %r waiting for %r; tail:\n%s" % (line, marker, g.log_text()[-2500:]))


def step_shape(mon, proc):
    lw.run(mon, proc, "p2p-up lot90 10.0.2.15 mohhdy-lab-key", "p2p-up ok name lot90", 120)
    out = run_getc(mon, proc, "p2p-shape", "p2p shape depth ", 20)
    need(out, [" weight 3/1 age_ms 0 cap 16 expired 0 capped 0"], "p2p-shape defaults")
    run_getc(mon, proc, "p2p-shape weight msg 5", "p2p shape depth ", 20)
    run_getc(mon, proc, "p2p-shape age 2000", "p2p shape depth ", 20)
    out = run_getc(mon, proc, "p2p-shape cap 4", "p2p shape depth ", 20)
    need(out, [" weight 5/1 age_ms 2000 cap 4 expired 0 capped 0"], "p2p-shape settings")
    out = run_getc(mon, proc, "p2p-shape weight msg 40", "p2p shape error", 20)
    need(out, ["p2p shape error weight 1..16"], "p2p-shape bound")
    run_getc(mon, proc, "p2p-down", "p2p-down ok", 20)
    say("mesh shaping: weight 5/1, age 2000 ms and cap 4 shown in p2p-shape; weight 40 refused")


def step_rxq(mon, proc, u):
    out = lw.run(mon, proc, "web-open %s/lot89/scope.html" % u, "osui web page sections", fail=["error="])
    lines = RXQ_RE.findall(g.normalized_log(out))
    if not lines:
        lines = RXQ_RE.findall(g.normalized_log(g.log_text()))
    if not lines:
        raise RuntimeError("worker printed no rxq line")
    held, peak, pin, pout, full, over, drains, idle = (int(v) for v in lines[-1])
    if held != pin - pout or held > peak or over != 0 or idle == 0:
        raise RuntimeError("rxq line inconsistent: %r" % (lines[-1],))
    say("NE2000 idle drain: rxq held %d peak %d in %d out %d full %d drains %d idle rounds %d" %
        (held, peak, pin, pout, full, drains, idle))


def boot_reader(hp):
    proc, mon, qmp = l88.boot(4, hp)
    try:
        g.wait_for(proc, "net-driver stack ring3 ready", g.CMD_TIMEOUT)
        time.sleep(0.5)
        u = "http://10.0.2.2:%d" % hp
        step_backgrounds(mon, proc, u)
        step_props(mon, proc, u)
        step_scope(mon, proc, u)
        step_rxq(mon, proc, u)
        step_autofill(mon, proc)
        step_shape(mon, proc)
    finally:
        l88.stop(proc, mon, qmp)
        l88.keep_log("qemu-lot90-serial-1x.log")


def boot_shape(hp):
    proc, mon, qmp = l88.boot(4, hp)
    try:
        g.wait_for(proc, "net-driver stack ring3 ready", g.CMD_TIMEOUT)
        time.sleep(0.5)
        step_shape(mon, proc)
    finally:
        l88.stop(proc, mon, qmp)


def boot_tv(hp):
    edid = os.path.join(g.LOG_DIR, "lot90-tv-edid.bin")
    with open(edid, "wb") as f:
        f.write(l88.tv_edid())
    proc, mon, qmp = l88.boot(None, hp, edid=edid)
    try:
        g.wait_for(proc, "net-driver stack ring3 ready", g.CMD_TIMEOUT)
        time.sleep(1.0)
        out = lw.run(mon, proc, "display-info", "display tv-hint", 20)
        need(out, ["display tv-hint overscan likely (no underscan bit): the desktop keeps a 2.5% margin per side clear"],
             "display-info overscan")
        mark = len(g.log_text())
        q = chrome.run(qmp, mon, proc, docks=None, want_q=lambda v: v == 6)
        text = g.normalized_log(g.log_text())
        need(text, ["[GFX] tv safe area margin 26 19 (no underscan bit)"], "kernel TV safe area")
        titles = {}
        for m in chrome.HIT_RE.finditer(g.normalized_log(g.log_text()[mark:])):
            titles.setdefault(m.group(1), set()).add(int(m.group(7)))
        want = {"pane": 45, "app": 45, "full": 45, "chat": 51}
        for who, t in want.items():
            if titles.get(who) != {t}:
                raise RuntimeError("title bar of %s: %r, want %d at q=6" % (who, titles.get(who), t))
        say("TV: safe area 26 x 19 px per side, chrome at q=%d with title bars %s" %
            (q, ", ".join("%s %d" % (k, v) for k, v in sorted(want.items()))))
    finally:
        l88.stop(proc, mon, qmp)
        l88.keep_log("qemu-lot90-serial-tv.log")


def main():
    if not os.path.isfile(os.path.join(lw.PKI, "test_ca.pem")):
        subprocess.run([sys.executable, os.path.join(ROOT, "scripts", "lot67_gen_pki.py"), lw.PKI], check=True)
    os.makedirs(g.LOG_DIR, exist_ok=True)
    hp = 31000 + os.getpid() % 9000
    srv_log = open(SRV_LOG, "w")
    srv = subprocess.Popen([sys.executable, os.path.join(ROOT, "scripts", "lot67_web_server.py"), lw.PKI,
                            str(hp), str(hp + 1)], stdout=srv_log, stderr=subprocess.STDOUT)
    t0 = time.monotonic()
    try:
        only = os.environ.get("LOT90_ONLY", "")
        if only == "shape":
            boot_shape(hp)
        if only in ("", "reader"):
            boot_reader(hp)
        if only in ("", "tv"):
            boot_tv(hp)
        say("lot90 qemu ok in %.1fs" % (time.monotonic() - t0))
        return 0
    finally:
        srv.terminate()
        try:
            srv.wait(timeout=5)
        except Exception:
            srv.kill()
        srv_log.close()


if __name__ == "__main__":
    try:
        sys.exit(main())
    except Exception as exc:  # noqa: BLE001
        say("FAIL: %s" % exc)
        sys.exit(1)
