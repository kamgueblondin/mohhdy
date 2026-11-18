#!/usr/bin/env python3
"""Lot 115 (US-031): layout values of the page scripts from the renderer.

One boot (scale 4). The lot 67 web server serves /lot115/layout.html: a
box of width 200 px, height 50 px, padding 10 px and border 2 px, and a
flex row of two 100 px items. With "web-js on" the page script reads
them; the shell lays the page out with the GUI's engine (wf_jslayout.c
over webeng.c), so the reader shows "layout115 224x74 c220 r2 100 100"
(the CSS box model and the flex row), where the Lot 114 block model gave
1008x16 and stacked the items. "web-js status" reports the boxes the
engine gave (layout_engine, not 0). /lot115/lang.html runs a class with
private fields, a static private counter, a generator [Symbol.iterator]
and a #x in o brand check, a symbol key and a string with an astral
character: the reader shows "lang115 ab 1 true u16 7 d83d sym 0 symbol"
(node prints the same for the script; the Lot 114 subset refused the
page with a SyntaxError on #items). The js REPL then runs six lines
that each leave about 900 KiB of garbage in its 3 MiB arena: Lot 114 had
to drop the realm ("realm reset (arena full)"); now the arena is
reclaimed between lines (wf_jsgc.c), the array the lines fill keeps its
six entries and "js" reports the collections ("osui js gc runs N").
/lot115/compat/templating.html and tabs.html (fixtures of
tests/fixtures/lot115: template.content clones, a KeyboardEvent) log in the
guest exactly what jsdom logs for them (expected.txt). The Lot 114 page (/lot114/app.html) still runs on the same boot. No guest clock is involved (no -rtc option).
"""
from __future__ import print_function

import os
import re
import subprocess
import sys
import time

HERE = os.path.dirname(os.path.abspath(__file__))
sys.path.insert(0, HERE)
sys.path.insert(0, os.path.join(HERE, "..", "..", "scripts"))
LOGS = os.path.join(HERE, "..", "..", "test_logs")
os.environ.setdefault("OSUI_GUI_LOG", os.path.join(LOGS, "qemu-lot115-serial.log"))
os.environ.setdefault("OSUI_GUI_MON_SOCK", "/tmp/mohhdy-lot115-mon.sock")
os.environ.setdefault("OSUI_GUI_QMP_SOCK", "/tmp/mohhdy-lot115-qmp.sock")
os.environ.setdefault("OVERLAY_DISK", os.path.join(LOGS, "qemu-lot115-overlay.img"))
import test_qemu_osui_gui as g  # noqa: E402
import test_qemu_lot67_web as lw  # noqa: E402
import test_qemu_lot88 as l88  # noqa: E402  (boot, stop, need, refuse, keep_log)

ROOT = g.ROOT
SRV_LOG = os.path.join(g.LOG_DIR, "lot115-web-server.log")
need, refuse = l88.need, l88.refuse
lw.ALIASES.update({"*": "shift-8", "+": "shift-equal", "[": "bracket_left", "]": "bracket_right", ",": "comma",
                   "#": "shift-3", "(": "shift-9", ")": "shift-0", ";": "semicolon", "<": "shift-comma"})


def say(msg):
    print("[lot115] %s" % msg, flush=True)


def step_layout(mon, proc, u):
    lw.run(mon, proc, "web-js on", "osui web-js on", 20)
    lw.run(mon, proc, "web-open %s/lot115/layout.html" % u, "osui web page sections", fail=["error="])
    read = lw.run(mon, proc, "web-read", "fin115", 30)
    need(read, ["layout115 224x74 c220 r2 100 100", "box115"], "web-read with web-js on")
    refuse(read, ["layout115 wait", "layout115 1008x16"], "web-read with web-js on")
    out = lw.run(mon, proc, "web-js status", "osui web-js on", 20)
    m = re.search(r" layout_engine (\d+)", out)
    if not m or int(m.group(1)) < 4:
        raise RuntimeError("web-js status: layout_engine missing or too small: %r" % out[-400:])
    say("the page script read the renderer's boxes: 224x74 (padding and border), client 220, "
        "a flex row; layout_engine %s" % m.group(1))


def step_lang(mon, proc, u):
    lw.run(mon, proc, "web-open %s/lot115/lang.html" % u, "osui web page sections", fail=["error="])
    read = lw.run(mon, proc, "web-read", "end115", 30)
    need(read, ["lang115 ab 1 true u16 7 d83d sym 0 symbol weak w true 1 false px 4"], "web-read of the Lot 115 language page")
    refuse(read, ["lang115 wait"], "web-read of the Lot 115 language page")
    say("private members, Symbol.iterator, a symbol key, UTF-16 lengths, WeakMap, WeakRef, Reflect and Proxy ran in the page script")


def step_gc(mon, proc):
    out = lw.run(mon, proc, "web-js status", "osui web-js on", 20)
    need(out, [" gc ", " freed ", " free "], "web-js status (arena collections)")
    lw.run(mon, proc, "js var k=[]", "osui js steps", 20, fail=["error="])
    churn = "js for(var i=0;i<8e3;i++)var o=[i,i+1].join();k.push(1);k.length"
    for n in range(1, 7):
        out = lw.run(mon, proc, churn, "osui js steps", 30, fail=["error=", "realm reset"])
        need(out, ["osui js => %d" % n], "js churn line %d" % n)
    out = lw.run(mon, proc, "js", "osui js gc runs", 20)
    m = re.search(r"osui js gc runs (\d+) live (\d+) freed (\d+)", out)
    if not m or int(m.group(1)) < 3 or int(m.group(3)) < 2 * 1024 * 1024:
        raise RuntimeError("js: too few collections: %r" % out[-400:])
    say("the js REPL reclaimed its arena between lines: %s collections, %s bytes back, the realm kept"
        % (m.group(1), m.group(3)))


FIXTURES = os.path.join(ROOT, "tests", "fixtures", "lot115")


def expected_lines(name):
    """The console lines jsdom gave for a fixture (tests/fixtures/lot115/expected.txt)."""
    lines, on = [], False
    with open(os.path.join(FIXTURES, "expected.txt")) as fh:
        for line in fh.read().split("\n"):
            if line.startswith("== "):
                on = line[3:] == name
            elif on and line:
                lines.append(line)
    if not lines:
        raise RuntimeError("no expected lines for fixture %s" % name)
    return lines


def step_compat(mon, proc, u):
    # real-page fixtures: the guest's console for each page is what jsdom
    # logged for it, line for line (template.content, KeyboardEvent)
    for name in ("templating", "tabs"):
        lw.run(mon, proc, "web-open %s/lot115/compat/%s.html" % (u, name), "osui web page sections", fail=["error="])
        out = lw.run(mon, proc, "web-js status", "osui web-js on", 20)
        need(out, [" failed 0"], "web-js status after the %s fixture" % name)
        want = expected_lines(name)
        out = lw.run(mon, proc, "web-js log", "osui web-js log: " + want[-1], 30)
        got = [l.split("osui web-js log: ", 1)[1].rstrip("\r") for l in out.split("\n") if "osui web-js log: " in l]
        if got[-len(want):] != want:
            raise RuntimeError("%s fixture: guest console %r, jsdom %r" % (name, got[-len(want):], want))
        say("the %s fixture logged what jsdom logs (%d lines)" % (name, len(want)))


def step_lot114(mon, proc, u):
    lw.run(mon, proc, "web-open %s/lot114/app.html" % u, "osui web page sections", fail=["error="])
    read = lw.run(mon, proc, "web-read", "fin114", 30)
    need(read, ["ext114 42", "item114 3", "hist114 1 ?h=1 1"], "web-read of the Lot 114 page")
    say("the Lot 114 page still runs")


def boot(hp):
    proc, mon, qmp = l88.boot(4, hp)
    try:
        g.wait_for(proc, "net-driver stack ring3 ready", g.CMD_TIMEOUT)
        time.sleep(0.5)
        u = "http://10.0.2.2:%d" % hp
        step_layout(mon, proc, u)
        step_lang(mon, proc, u)
        step_gc(mon, proc)
        step_compat(mon, proc, u)
        step_lot114(mon, proc, u)
    finally:
        l88.stop(proc, mon, qmp)
        l88.keep_log("qemu-lot115-serial-1x.log")


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
        boot(hp)
        say("lot115 qemu ok in %.1fs" % (time.monotonic() - t0))
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
        print("[lot115] FAIL: %s" % exc, flush=True)
        sys.exit(1)
