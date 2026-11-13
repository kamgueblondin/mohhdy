#!/usr/bin/env python3
"""Lot 91 in QEMU, offline. One boot at ui-scale 1x with the guest clock
set to 2026-07-01 10:30 UTC (-rtc base=...,clock=vm); the Lot 67 test server runs
on the host and the guest reaches it as 10.0.2.2 (slirp).

  * reader backgrounds (/lot91/bg.html): radial gradients with explicit
    radii (circle 40px, ellipse 30px 20%), a 100grad linear gradient, the
    box keywords of the background shorthand (one keyword sets both
    boxes, two set origin then clip) and the background-clip and
    background-origin longhands are kept; a percentage circle and
    background-clip: text are refused (the colour stays).
  * @property (/lot91/props.html): 'auto | <length>' is registered (Lot 90
    refused the rule), so red falls back to the initial 11px; five
    alternatives are refused; calc(10px + 5%) is not a <length> (Lot 90
    took it) and falls back to 13px, calc(2px * 3 + 4px) is 10px.
  * autofill summer time: with the local time set near midnight by the
    tz offset, eu and us move the local date to 2 July, au (July is
    winter there) and off do not; the status line shows the rule.
  * autofill expiry before offers: a value dated the day before, expiry
    1 day; the local date crosses midnight while the guest runs and the
    next offer list forgets the value (Lot 90 ran the expiry only on
    stores and settings, so the value was still offered).
"""
from __future__ import print_function

import datetime
import os
import re
import subprocess
import sys
import time

HERE = os.path.dirname(os.path.abspath(__file__))
sys.path.insert(0, HERE)
LOGS = os.path.join(HERE, "..", "..", "test_logs")
os.environ.setdefault("OSUI_GUI_LOG", os.path.join(LOGS, "qemu-lot91-serial.log"))
os.environ.setdefault("OSUI_GUI_MON_SOCK", "/tmp/mohhdy-lot91-mon.sock")
os.environ.setdefault("OSUI_GUI_QMP_SOCK", "/tmp/mohhdy-lot91-qmp.sock")
os.environ.setdefault("OVERLAY_DISK", os.path.join(LOGS, "qemu-lot91-overlay.img"))
import test_qemu_osui_gui as g  # noqa: E402
import test_qemu_lot67_web as lw  # noqa: E402
import test_qemu_lot88 as l88  # noqa: E402  (boot, stop, need, refuse, keep_log)

ROOT = g.ROOT
SRV_LOG = os.path.join(g.LOG_DIR, "lot91-web-server.log")
need, refuse = l88.need, l88.refuse
RTC = "2026-07-01T10:30:00"
DAY0 = (datetime.date(2026, 7, 1) - datetime.date(1970, 1, 1)).days + 1
MARGIN_MIN = 4
DATE_RE = re.compile(r"date ok (\d{4})-(\d\d)-(\d\d) (\d\d):(\d\d):(\d\d) UTC")


def say(msg):
    print("[lot91] %s" % msg, flush=True)


def css_page(mon, proc, u, page, fin, keep, hide=()):
    lw.run(mon, proc, "web-css on", "web-css on", 20)
    lw.run(mon, proc, "web-open %s/lot91/%s" % (u, page), "osui web page sections", fail=["error="])
    css = lw.run(mon, proc, "web-css", "css-props ", 30)
    read = lw.run(mon, proc, "web-read", fin, 30)
    need(read, list(keep), "web-read " + page)
    refuse(read, list(hide), "web-read " + page)
    lw.run(mon, proc, "web-css off", "web-css off", 20)
    return css


def step_backgrounds(mon, proc, u):
    css = css_page(mon, proc, u, "bg.html", "fin91b",
                   ["radkeep91", "ellkeep91", "gradkeep91", "boxkeep91", "onekeep91", "clipkeep91",
                    "badrkeep91", "badckeep91"])
    need(css, ["node div #rad91 display auto bgimg radial-gradient(circle 40px at center, #ff0000, #0000ff) "
               "bgrep no-repeat",
               "node div #ell91 display auto bgimg radial-gradient(ellipse 30px 20% at left top, #00ff00, #000000)",
               "node div #grad91 display auto bgimg linear-gradient(100grad, #ff0000, #0000ff)",
               "node div #box91 display auto bg #eeeeee bgclip padding-box bgorigin content-box",
               "node div #one91 display auto bg #dddddd bgclip border-box bgorigin border-box",
               "node div #clip91 display auto bg #ddeeff bgclip content-box bgorigin border-box",
               "node div #badc91 display auto bg #654321\n"], "web-css backgrounds")
    m = re.search(r"node div #badr91 display auto bg #123456([^\n]*)\n", css)
    if not m or "bgimg" in m.group(1):
        raise RuntimeError("percentage circle: %r" % (m.group(0) if m else css[-800:]))
    say("reader: circle 40px and ellipse 30px 20% kept, 100grad kept, shorthand boxes (two: origin then "
        "clip; one: both), clip/origin longhands; circle 10% and background-clip: text refused")


def step_props(mon, proc, u):
    css = css_page(mon, proc, u, "props.html", "fin91p", ["altkeep91", "altbadkeep91", "calcbadkeep91", "calcokkeep91"])
    need(css, ["node p #alt91 display auto size 24px",
               "node p #altbad91 display auto size 11px",
               "node p #calcbad91 display auto size 13px",
               "node p #calcok91 display auto size 10px",
               "css-props registered 3 rejected 1 "], "web-css @property")
    say("@property: 'auto | <length>' and '<length>#' registered, five alternatives refused; red -> 11px, "
        "calc(10px + 5%) -> 13px (not a <length>), calc(2px * 3 + 4px) -> 10px")


def guest_utc(mon, proc):
    out = lw.run(mon, proc, "date", "date ok ", 20)
    m = DATE_RE.search(out)
    if not m:
        raise RuntimeError("date: %r" % out[-300:])
    y, mo, d, hh, mm, ss = (int(x) for x in m.groups())
    if (y, mo, d) != (2026, 7, 1):
        raise RuntimeError("the guest clock is not on 2026-07-01 (-rtc base): %r" % m.group(0))
    return hh * 60 + mm, ss


def norm_tz(t):
    while t > 840:
        t -= 1440
    while t < -720:
        t += 1440
    return t


def step_dst(mon, proc):
    out = lw.run(mon, proc, "autofill status", "autofill status ", 20)
    need(out, ["autofill status expire 0 tz 0 today %d " % DAY0], "autofill status on 1 July")
    refuse(out, [" dst "], "autofill status with no rule")
    u, _ = guest_utc(mon, proc)
    tz = norm_tz(1439 - 20 - u)          # local standard time about 23:39
    out = lw.run(mon, proc, "autofill tz %d" % tz, "autofill tz ", 20)
    need(out, ["autofill tz %s%d today %d" % ("+" if tz > 0 else "", tz, DAY0)], "autofill tz near midnight")
    for rule, day in (("eu", DAY0 + 1), ("us", DAY0 + 1), ("au", DAY0), ("off", DAY0), ("eu", DAY0 + 1)):
        out = lw.run(mon, proc, "autofill dst " + rule, "autofill ", 20)
        need(out, ["autofill dst %s today %d" % (rule, day)], "autofill dst " + rule)
    out = lw.run(mon, proc, "autofill status", "autofill status ", 20)
    need(out, ["today %d " % (DAY0 + 1), " values 0 dst eu"], "autofill status with eu")
    out = lw.run(mon, proc, "autofill dst xx", "autofill ", 20)
    need(out, ["autofill error usage: autofill dst eu|us|au|off"], "autofill dst xx")
    lw.run(mon, proc, "autofill dst off", "autofill dst ", 20)
    lw.run(mon, proc, "autofill tz utc", "autofill tz ", 20)
    say("summer time: tz %+d (local 23:39 standard): eu and us move the date to 2 July, au and off keep "
        "1 July; status shows dst eu; a wrong rule is refused" % tz)


def step_expiry(mon, proc):
    u, _ = guest_utc(mon, proc)
    if u >= 700:
        raise RuntimeError("guest clock too late for the tz -720 step: %d min" % u)
    out = lw.run(mon, proc, "autofill tz -720", "autofill tz ", 20)
    need(out, ["autofill tz -720 today %d" % (DAY0 - 1)], "autofill tz -720")
    out = lw.run(mon, proc, "autofill expire 1", "autofill expire ", 20)
    need(out, ["autofill expire 1 forgot 0"], "autofill expire 1")
    out = lw.run(mon, proc, "autofill add www.example.org user exp91", "autofill ", 20)
    need(out, ["autofill added"], "autofill add on 30 June")
    # Local midnight a few minutes ahead: typing a command line costs about
    # half a second per key, so the tz line and the first offers line take
    # up to a minute. With one minute of margin (the first version) the
    # guest crossed midnight before the first list in one run of two and
    # the value was (rightly) forgotten there already.
    u, s = guest_utc(mon, proc)
    tz = 1439 - u - MARGIN_MIN           # local 23:5x, MARGIN_MIN whole minutes to midnight
    out = lw.run(mon, proc, "autofill tz %d" % tz, "autofill tz ", 20)
    need(out, ["autofill tz +%d today %d" % (tz, DAY0)], "autofill tz before midnight")
    out = lw.run(mon, proc, "autofill offers https://www.example.org/ user", "autofill offers ", 20)
    u, s = guest_utc(mon, proc)
    if u + tz >= 1440:
        raise RuntimeError("setup too slow: local midnight passed before the first offer list was checked")
    need(out, ["autofill offer 0 exp91 host", "autofill offers 1"], "the value one day old is offered")
    # Wait on the guest clock until local midnight has passed. A single
    # host sleep is not enough: with QEMU's default clock=host, a pause
    # of the host jumps the guest RTC (one run reached 12:35 UTC, 755 min,
    # and failed the tz -720 window). clock=vm keeps the RTC on virtual
    # time. Poll the guest date until midnight, and fail only if that
    # clock stops advancing (not if the host wall clock jumps).
    last = u * 60 + s
    stagnant = 0
    while u + tz < 1440:
        time.sleep(5)
        u, s = guest_utc(mon, proc)
        now = u * 60 + s
        if now <= last:
            stagnant += 1
            if stagnant > 60:
                raise RuntimeError("the guest clock did not reach local midnight: utc minute %d tz %d" % (u, tz))
        else:
            stagnant = 0
            last = now
    out = lw.run(mon, proc, "autofill offers https://www.example.org/ user", "autofill offers ", 20)
    need(out, ["autofill offers 0"], "expiry before the offers after midnight")
    refuse(out, ["exp91"], "expiry before the offers after midnight")
    out = lw.run(mon, proc, "autofill status", "autofill status ", 20)
    need(out, ["autofill status expire 1 tz %d today %d touched 0 expired 1 " % (tz, DAY0 + 1)],
         "autofill status after midnight")
    lw.run(mon, proc, "autofill expire off", "autofill expire ", 20)
    lw.run(mon, proc, "autofill tz utc", "autofill tz ", 20)
    say("expiry before offers: a value from 30 June (expire 1) offered before midnight on 1 July, forgotten by "
        "the first offer list after midnight (expired 1)")


def boot_reader(hp):
    proc, mon, qmp = l88.boot(4, hp, extra=["-rtc", "base=%s,clock=vm" % RTC])
    try:
        g.wait_for(proc, "net-driver stack ring3 ready", g.CMD_TIMEOUT)
        time.sleep(0.5)
        u = "http://10.0.2.2:%d" % hp
        step_backgrounds(mon, proc, u)
        step_props(mon, proc, u)
        step_dst(mon, proc)
        step_expiry(mon, proc)
    finally:
        l88.stop(proc, mon, qmp)
        l88.keep_log("qemu-lot91-serial-1x.log")


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
        boot_reader(hp)
        say("lot91 qemu ok in %.1fs" % (time.monotonic() - t0))
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
