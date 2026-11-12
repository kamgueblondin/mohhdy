#!/usr/bin/env python3
"""Lot 89 in QEMU, offline: one boot at ui-scale 1x; the Lot 67 test server
runs on the host and the guest reaches it as 10.0.2.2 (slirp).

  * autofill dates: keys stored by hand are dated from the guest's CMOS
    clock; `autofill ages` lists them (never the values) and
    `autofill forget-older 30` keeps them, a bad DAYS is refused.
  * @scope lists (/lot89/scope.html): two starts and two limits, and
    "&:first-child" / ":scope:last-child" hide only the scoping root.
  * reader lengths (/lot89/props.html): calc(), min(), clamp(), max() and
    var() inside calc() and in a fallback resolve to one length; a var()
    cycle and an em + px sum drop the declaration, the earlier one stays.
  * custom properties per element (/lot89/vars.html): --x set by a class,
    a descendant selector, an @scope block and a style attribute, read by
    var() on each element; inherited through a div.
  * idle paint cost (US-081): saver with a 3 s dim; the wake line carries
    paint time and busy share per dim level and the paint time not spent,
    and they agree with the frame counts.
  * stray response (gap 6): /lot89/stray.html sends a second, unasked HTTP
    response on the kept-alive connection; after 12 s of quiet (the kept
    connection is dropped at 10 s) the next page must still load. Before
    Lot 89 the NE2000 ring could be overwritten while nobody read it and the
    driver then refused the same bad header forever: every later connect
    timed out. The worker's ring line must stay well formed.
    /lot89/stray1.html sends the page and the stray answer in one write:
    the guest reads them together, counts it (`http-keepalive ...
    unsolicited N`) and does not keep that connection, so the favicon
    request after it is not answered by the stray 404.
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
os.environ.setdefault("OSUI_GUI_LOG", os.path.join(LOGS, "qemu-lot89-serial.log"))
os.environ.setdefault("OSUI_GUI_MON_SOCK", "/tmp/mohhdy-lot89-mon.sock")
os.environ.setdefault("OSUI_GUI_QMP_SOCK", "/tmp/mohhdy-lot89-qmp.sock")
os.environ.setdefault("OVERLAY_DISK", os.path.join(LOGS, "qemu-lot89-overlay.img"))
import test_qemu_osui_gui as g  # noqa: E402
import test_qemu_lot67_web as lw  # noqa: E402
import test_qemu_lot88 as l88  # noqa: E402  (boot, stop, need, refuse, keep_log)

ROOT = g.ROOT
SRV_LOG = os.path.join(g.LOG_DIR, "lot89-web-server.log")
need, refuse = l88.need, l88.refuse

ENERGY_RE = re.compile(r"osui energy wake after (\d+) ms refresh (\d+) hz idle_fps10 (\d+) "
                       r"idle_drawn (\d+) idle_skipped (\d+)")
COST_RE = re.compile(r"paint_ms (\d+) (\d+) (\d+) wall_ms (\d+) (\d+) (\d+) "
                     r"busy_permille (\d+) (\d+) (\d+) cost_us (\d+) saved_ms (\d+)")


def say(msg):
    print("[lot89] %s" % msg, flush=True)


def step_autofill_dates(mon, proc):
    for host, val in (("www.example.com", "cy89"), ("shop.example.org", "ed89"), ("10.0.2.2", "lab89")):
        out = lw.run(mon, proc, "autofill add %s user %s" % (host, val), "autofill ", 20)
        need(out, ["autofill added"], "autofill add " + host)
    # dated from the CMOS clock (QEMU gives the host's UTC date): stored
    # seconds ago, so 0 days (1 if the run straddles a UTC midnight)
    out = lw.run(mon, proc, "autofill ages", "autofill ages ", 20)
    need(out, ["autofill ages 3 dated 3"], "autofill ages")
    for host in ("www.example.com", "shop.example.org", "10.0.2.2"):
        if not re.search(r"autofill age %s user [01]d" % re.escape(host), out):
            raise RuntimeError("autofill ages: no recent date for %s in %r" % (host, out[-600:]))
    refuse(out, ["cy89", "ed89", "lab89", "undated"], "autofill ages")
    out = lw.run(mon, proc, "autofill forget-older 30", "autofill ", 20)
    need(out, ["autofill forgot older than 30 days keys 0 undated 0 left 3"], "autofill forget-older 30")
    out = lw.run(mon, proc, "autofill forget-older 3x", "autofill ", 20)
    need(out, ["autofill error usage: autofill forget-older DAYS"], "autofill forget-older usage")
    out = lw.run(mon, proc, "autofill offers https://www.example.com/ user", "autofill offers ", 20)
    need(out, ["autofill offer 0 cy89 host", "autofill offers 1"], "offers kept after forget-older")
    say("autofill dates: three keys dated today from the CMOS clock, forget-older 30 kept them, 3x refused")


def step_scope_lists(mon, proc, u):
    lw.run(mon, proc, "web-css on", "web-css on", 20)
    lw.run(mon, proc, "web-open %s/lot89/scope.html" % u, "osui web page sections", fail=["error="])
    css = lw.run(mon, proc, "web-css", "css-scope ", 30)
    need(css, ["css-scope scopes 3 rules 3"], "web-css @scope lists")
    read = lw.run(mon, proc, "web-read", "fin89s", 30)
    need(read, ["listkeepa89", "listkeepb89", "listout89", "ampkeep89", "sckeep89"], "web-read @scope lists")
    refuse(read, ["listhidea89", "listhideb89", "amphide89", "schide89"], "web-read @scope lists")
    lw.run(mon, proc, "web-css off", "web-css off", 20)
    say("@scope lists: both starts scoped, both limits kept, &:first-child and :scope:last-child hid only the root")


def step_lengths(mon, proc, u):
    lw.run(mon, proc, "web-css on", "web-css on", 20)
    lw.run(mon, proc, "web-open %s/lot89/props.html" % u, "osui web page sections", fail=["error="])
    css = lw.run(mon, proc, "web-css", "css-scope ", 30)
    need(css, ["node p #calc89 display auto size 20px indent 1.5em",
               "node p #var89 display auto size 18px indent 3px",
               # Lot 90 (converted, review point): --a89 and --c89 form a
               # cycle, guaranteed-invalid where declared (the root), so
               # var(--a89, 30px) takes its fallback as CSS Variables 1 says
               # (was: dropped, 20px kept), and calc(1em + 2px) is resolved against the 16 px
               # font size (was: dropped, 1em kept)
               "node p #cyc89 display auto size 30px",
               "node p #mix89 display auto indent 18px"], "web-css lengths")
    # clamp() depends on the reader's viewport: between its two bounds
    m = re.search(r"node p #clamp89 display auto size (\d+)(?:\.(\d+))?px ls 2px", g.normalized_log(css))
    if not m or not 16 <= int(m.group(1)) <= 32:
        raise RuntimeError("web-css clamp(): %r" % css[-900:])
    read = lw.run(mon, proc, "web-read", "fin89p", 30)
    need(read, ["lencalc89", "lenclamp89", "lenvar89", "lencyc89", "lenmix89"], "web-read lengths")
    lw.run(mon, proc, "web-css off", "web-css off", 20)
    say("lengths: calc 20px, min 1.5em, clamp %spx, var() in calc 18px and in a fallback 3px; "
        "a var() cycle and 1em + 2px dropped" % m.group(1))


RING_RE = re.compile(r"net-driver nic ring dropped (\d+) resyncs (\d+)( last \d+ \d+ \d+ \d+ page \d+ curr \d+)? end")


def step_stray_recovery(mon, proc, u):
    lw.run(mon, proc, "web-open %s/lot89/stray.html" % u, "osui web page sections", fail=["error="])
    time.sleep(12)
    out = lw.run(mon, proc, "web-open %s/lot89/scope.html" % u, "osui web page sections", fail=["error="])
    need(out, ["osui web fetch ok status 200"], "page after a stray response")
    with open(SRV_LOG) as f:
        need(f.read(), ["STRAY sent"], "test server stray response")
    rings = RING_RE.findall(g.normalized_log(out))
    if not rings:
        raise RuntimeError("worker printed no NE2000 ring line")
    dropped, resyncs = int(rings[-1][0]), int(rings[-1][1])
    say("stray response: next page loaded after 12 s, ring dropped %d resyncs %d" % (dropped, resyncs))
    before = keepalive_unsolicited(mon, proc)
    out = lw.run(mon, proc, "web-open %s/lot89/stray1.html" % u, "osui web page sections", fail=["error="])
    need(out, ["osui web fetch ok status 200", "favicon ok"], "page followed by a stray answer in one read")
    after = keepalive_unsolicited(mon, proc)
    if after != before + 1:
        raise RuntimeError("unsolicited count %d -> %d, want +1" % (before, after))
    out = lw.run(mon, proc, "web-open %s/lot89/scope.html" % u, "osui web page sections", fail=["error="])
    need(out, ["osui web fetch ok status 200"], "page after a stray answer read together")
    say("stray answer read with the page: connection not kept (unsolicited %d -> %d), next page loaded" % (before, after))


def keepalive_unsolicited(mon, proc):
    out = lw.run(mon, proc, "http-keepalive", "http-keepalive on opened", 30)
    line = out[out.rfind("http-keepalive on opened"):].splitlines()[0]
    if " unsolicited " not in line:
        raise RuntimeError("no unsolicited count: %r" % line)
    return int(line.split(" unsolicited ")[1].split()[0])


def step_elem_vars(mon, proc, u):
    lw.run(mon, proc, "web-css on", "web-css on", 20)
    lw.run(mon, proc, "web-open %s/lot89/vars.html" % u, "osui web page sections", fail=["error="])
    css = lw.run(mon, proc, "web-css", "css-vars ", 30)
    need(css, ["node p #root89 display auto size 14px indent 1px",
               "node p #deep89 display auto size 20px indent 1px",
               "node p #big89 .big89 display auto size 24px indent 1px",
               "node p #pan89 display auto size 22px indent 1px",
               "node p #inl89 display auto size 14px indent 5px",
               "css-vars names 2 "], "web-css custom properties per element")
    read = lw.run(mon, proc, "web-read", "fin89v", 30)
    need(read, ["varroot89", "vardeep89", "varbig89", "varpan89", "varinl89"], "web-read custom properties")
    lw.run(mon, proc, "web-css off", "web-css off", 20)
    say("custom properties per element: root 14px, class 20px inherited, descendant 24px, @scope 22px, "
        "style attribute 5px")


def step_energy_cost(mon, proc):
    lw.run(mon, proc, "power-apply saver", "power-apply profile saver", 20)
    out = lw.run(mon, proc, "power-dim 3", "dim_after", 20)
    need(out, ["profile saver", "dim_after 3s"], "power-dim")
    start = len(g.log_text())
    g.send_command_until(mon, "gui", "osui gui live", proc, wait_prompt=False)
    g.wait_for(proc, "osui energy dim 1 after 3 s refresh 7 hz", 60, start)
    g.wait_for(proc, "osui energy dim 2 after 6 s refresh 5 hz", 60, start)
    time.sleep(4.0)
    mark = len(g.log_text())
    g.send_key(mon, "x")
    g.wait_for(proc, "osui energy wake after ", 30, mark)
    text = g.normalized_log(g.log_text()[mark:])
    m, c = ENERGY_RE.search(text), COST_RE.search(text)
    if not m or not c:
        raise RuntimeError("energy wake line unreadable: %r" % text[-600:])
    idle_ms, hz, fps10, drawn, skipped = (int(v) for v in m.groups())
    paint = [int(v) for v in c.groups()[0:3]]
    wall = [int(v) for v in c.groups()[3:6]]
    busy = [int(v) for v in c.groups()[6:9]]
    cost_us, saved_ms = int(c.group(10)), int(c.group(11))
    if hz != 15 or idle_ms < 9000 or drawn < 1:
        raise RuntimeError("energy wake: idle %d ms, %d hz, %d drawn" % (idle_ms, hz, drawn))
    # the levels split the idle time (10 ms ticks, the last interval is
    # counted after the line is written)
    if abs(sum(wall) - idle_ms) > 1000 or wall[1] < 2000 or wall[2] < 1000:
        raise RuntimeError("energy cost: wall %r against %d ms idle" % (wall, idle_ms))
    # painting fits in the idle time, one frame of slack
    if sum(paint) > idle_ms + cost_us // 1000 + 20 or max(busy) > 1000:
        raise RuntimeError("energy cost: paint %r busy %r in %d ms" % (paint, busy, idle_ms))
    # the estimate is what it says: skipped wakes times the frame cost
    if abs(saved_ms - skipped * cost_us // 1000) > 1:
        raise RuntimeError("energy cost: saved %d ms, %d skipped at %d us" % (saved_ms, skipped, cost_us))
    # the deep dim never paints a larger share than the awake desktop; on
    # a paint-bound CI runner both run at the loop rate (same share)
    if busy[2] > busy[0] + 150:
        raise RuntimeError("energy cost: deep dim busier (%d) than awake (%d)" % (busy[2], busy[0]))
    g.wait_for(proc, "OSUI-SNAP", 20, mark)
    g.send_key(mon, "esc")
    g.wait_for(proc, "osui gui input cleared", 20, mark)
    g.send_command_until(mon, "/console", "osui gui exit", proc, mode="getc", wait_prompt=False)
    time.sleep(1.0)
    out = lw.run(mon, proc, "power-dim default", "dim_after", 20)
    need(out, ["dim_after 60s"], "power-dim default")
    lw.run(mon, proc, "power-apply balanced", "power-apply profile balanced", 20)
    say("idle paint cost: %d painted, %d skipped in %.1f s; a frame costs %d us; painting took "
        "%d/%d/%d permille awake/dim/deep; about %d ms of painting not spent" %
        (drawn, skipped, idle_ms / 1000.0, cost_us, busy[0], busy[1], busy[2], saved_ms))


def main():
    if not os.path.isfile(os.path.join(lw.PKI, "test_ca.pem")):
        subprocess.run([sys.executable, os.path.join(ROOT, "scripts", "lot67_gen_pki.py"), lw.PKI], check=True)
    os.makedirs(g.LOG_DIR, exist_ok=True)
    hp = 31000 + os.getpid() % 9000
    srv_log = open(SRV_LOG, "w")
    srv = subprocess.Popen([sys.executable, os.path.join(ROOT, "scripts", "lot67_web_server.py"), lw.PKI,
                            str(hp), str(hp + 1)], stdout=srv_log, stderr=subprocess.STDOUT)
    proc = mon = qmp = None
    t0 = time.monotonic()
    try:
        proc, mon, qmp = l88.boot(4, hp)
        g.wait_for(proc, "net-driver stack ring3 ready", g.CMD_TIMEOUT)
        time.sleep(0.5)
        u = "http://10.0.2.2:%d" % hp
        step_autofill_dates(mon, proc)
        step_scope_lists(mon, proc, u)
        step_lengths(mon, proc, u)
        step_elem_vars(mon, proc, u)
        step_stray_recovery(mon, proc, u)
        step_energy_cost(mon, proc)
        say("lot89 qemu ok in %.1fs" % (time.monotonic() - t0))
        return 0
    finally:
        l88.stop(proc, mon, qmp)
        l88.keep_log("qemu-lot89-serial-1x.log")
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
