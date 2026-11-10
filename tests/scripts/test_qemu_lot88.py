#!/usr/bin/env python3
"""Lot 88 in QEMU, offline: the Lot 67 test server runs on the host and the
guest reaches it as 10.0.2.2 (slirp).

  * autofill per site: values stored by hand for login.example.com and
    www.example.com are offered to a page of shop.example.com (the site's
    hosts), never to example.org or to evil-example.com; an IP host shares
    with nobody. Forgetting from another host of the site works.
  * @scope: /lot88/scope.html hides the p of .card88 but not the one under
    its limit .foot88 nor the one outside; an @scope with no prelude in a
    <style> inside .wrap88 hides only the em there; :scope gets
    text-transform; all: revert undoes an author display: none.
  * TV hint (US-080): a second boot gets a 1080p HDMI TV EDID through
    fw_cfg and no scale pin; the safe TV scale (1.5x on 1024x768) is taken
    on that first boot and display-info says so.
  * window chrome (test_qemu_lot88_chrome.py): chat, Statut pane, Fichiers
    app and full screen Browser-OS, at 1x then on the TV boot at 1.5x:
    close button where the hit test puts it, button side from the scale,
    maximize and back, grip grow / shrink / back, minimize and dock
    restore (1x), close.
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
os.environ.setdefault("OSUI_GUI_LOG", os.path.join(LOGS, "qemu-lot88-serial.log"))
os.environ.setdefault("OSUI_GUI_MON_SOCK", "/tmp/mohhdy-lot88-mon.sock")
os.environ.setdefault("OSUI_GUI_QMP_SOCK", "/tmp/mohhdy-lot88-qmp.sock")
os.environ.setdefault("OVERLAY_DISK", os.path.join(LOGS, "qemu-lot88-overlay.img"))
import test_qemu_osui_gui as g  # noqa: E402
import test_qemu_lot67_web as lw  # noqa: E402
from test_qemu_gui_windows import W, H, BAR_H  # noqa: E402,F401
import test_qemu_lot88_chrome as chrome  # noqa: E402

ROOT = g.ROOT
SRV_LOG = os.path.join(g.LOG_DIR, "lot88-web-server.log")


def say(msg):
    print("[lot88] %s" % msg, flush=True)


def server_text():
    try:
        return open(SRV_LOG).read()
    except OSError:
        return ""


def wait_server(needle, timeout=60):
    deadline = time.monotonic() + timeout
    while time.monotonic() < deadline:
        if needle in server_text():
            return
        time.sleep(0.3)
    raise RuntimeError("server never saw %r; tail:\n%s" % (needle, server_text()[-1500:]))


def need(text, want, what):
    for w in want:
        if w not in text:
            raise RuntimeError("%s: missing %r in %r" % (what, w, text[-900:]))


def refuse(text, bad, what):
    for b in bad:
        if b in text:
            raise RuntimeError("%s: unexpected %r in %r" % (what, b, text[-900:]))


def console_autofill(mon, proc):
    """Autofill shared inside a site, through the same store and the same
    list the desktop uses (shell_desk)."""
    for host, val in (("login.example.com", "ada88"), ("www.example.com", "cy88"),
                      ("evil-example.com", "mallory88"), ("10.0.2.2", "lab88")):
        out = lw.run(mon, proc, "autofill add %s user %s" % (host, val), "autofill ", 20)
        need(out, ["autofill added"], "autofill add " + host)
    out = lw.run(mon, proc, "autofill offers https://shop.example.com/login user", "autofill offers ", 20)
    need(out, ["autofill offer 0 ada88 site", "autofill offer 1 cy88 site", "autofill offers 2"],
         "offers for shop.example.com")
    refuse(out, ["mallory88", "lab88"], "offers for shop.example.com")
    out = lw.run(mon, proc, "autofill offers https://www.example.com/ user", "autofill offers ", 20)
    need(out, ["autofill offer 0 cy88 host", "autofill offer 1 ada88 site", "autofill offers 2"],
         "offers for www.example.com")
    out = lw.run(mon, proc, "autofill offers https://example.org/ user", "autofill offers ", 20)
    need(out, ["autofill offers 0"], "offers for example.org")
    out = lw.run(mon, proc, "autofill offers http://10.0.2.3/ user", "autofill offers ", 20)
    need(out, ["autofill offers 0"], "offers for another IP")
    out = lw.run(mon, proc, "autofill offers http://10.0.2.2/ user", "autofill offers ", 20)
    need(out, ["autofill offer 0 lab88 host", "autofill offers 1"], "offers for the IP itself")
    out = lw.run(mon, proc, "autofill offers user", "autofill ", 20)
    need(out, ["autofill error usage"], "autofill offers usage")
    out = lw.run(mon, proc, "autofill forget login.example.com user ada88", "autofill ", 20)
    need(out, ["autofill forgot value left 0"], "autofill forget on login")
    out = lw.run(mon, proc, "autofill offers https://shop.example.com/login user", "autofill offers ", 20)
    need(out, ["autofill offer 0 cy88 site", "autofill offers 1"], "offers after forget")
    say("autofill per site: shop.example.com gets login's and www's values, not example.org's, "
        "evil-example.com's or the IP's")


def tv_edid():
    """A 1080p HDMI television, written field by field: base block with a
    1920x1080@60 detailed timing and no physical size, one CEA-861 rev 3
    extension with VIC 16 (native) and VIC 4, an HDMI vendor block at
    1.0.0.0, no underscan bit. 256 bytes, both checksums set."""
    base = bytearray(128)
    base[0:8] = b"\x00\xff\xff\xff\xff\xff\xff\x00"
    base[8:10] = b"\x35\x08"           # manufacturer "MOH"
    base[10:12] = b"\x88\x00"          # product 88
    base[16], base[17] = 1, 36          # week 1 of 2026
    base[18], base[19] = 1, 3           # EDID 1.3
    base[20] = 0x80                     # digital input
    base[21], base[22] = 0, 0           # no physical size: a TV hint
    base[23], base[24] = 0x78, 0x0A
    for k in range(38, 54):
        base[k] = 0x01                  # no standard timings
    # 1920x1080@60, 148.5 MHz: active/blank 1920/280, 1080/45
    base[54:72] = bytes([0x02, 0x3A, 0x80, 0x18, 0x71, 0x38, 0x2D, 0x40, 0x58, 0x2C,
                         0x45, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x1E])
    name = b"MOHHDY TV88\n"
    base[72:90] = bytes([0, 0, 0, 0xFC, 0]) + name + b" " * (13 - len(name))
    for off in (90, 108):
        base[off:off + 18] = bytes([0, 0, 0, 0x10, 0]) + bytes(13)
    base[126] = 1                       # one extension
    base[127] = (-sum(base[:127])) & 0xFF
    cea = bytearray(128)
    blocks = bytes([0x42, 0x90, 0x04,               # video: VIC 16 native, VIC 4
                    0x65, 0x03, 0x0C, 0x00, 0x10, 0x00])  # HDMI VSDB, 1.0.0.0
    cea[0], cea[1], cea[2], cea[3] = 0x02, 3, 4 + len(blocks), 0x40
    cea[4:4 + len(blocks)] = blocks
    cea[127] = (-sum(cea[:127])) & 0xFF
    return bytes(base + cea)


def boot(scale, srv_port, edid=None):
    """One QEMU at this ui-scale (quarters, 4 = 1x; None: no pin), with an
    EDID blob through fw_cfg when edid is a path; returns proc, mon, qmp."""
    for path in (g.LOG, g.MON_SOCK, g.QMP_SOCK):
        try:
            os.remove(path)
        except OSError:
            pass
    g.prepare_test_disk()
    err = open(os.devnull, "wb")
    pins = []
    if scale:
        pins += ["-fw_cfg", "name=opt/mohhdy/ui-scale,string=%d" % scale]
    if edid:
        pins += ["-fw_cfg", "name=opt/mohhdy/edid,file=%s" % edid]
    proc = subprocess.Popen([
        "qemu-system-i386"] + pins + [
        "-cpu", os.environ.get("LOT88_CPU", "max"),
        "-kernel", g.KERNEL, "-initrd", g.INITRD, "-m", "1024M",
        "-display", "none", "-vga", "std",
        "-usb", "-device", "usb-tablet",
        "-serial", "file:" + g.LOG,
        "-monitor", "unix:%s,server,nowait" % g.MON_SOCK,
        "-qmp", "unix:%s,server,nowait" % g.QMP_SOCK,
        "-machine", "type=pc,accel=tcg", "-no-reboot", "-no-shutdown",
        "-netdev", "user,id=n0", "-device", "ne2k_isa,netdev=n0",
    ] + g.qemu_disk_args(), cwd=ROOT, stdout=err, stderr=err)
    err.close()
    g.wait_for(proc, "[0m : ", g.BOOT_TIMEOUT)
    g.wait_for(proc, "usbdriver tablet enumerated port ", g.CMD_TIMEOUT)
    mon = g.monitor_connect()
    qmp = g.qmp_connect()
    return proc, mon, qmp


def stop(proc, mon, qmp):
    for s in (mon, qmp):
        if s:
            try:
                s.close()
            except Exception:
                pass
    if proc and proc.poll() is None:
        proc.terminate()
        try:
            proc.wait(timeout=5)
        except Exception:
            proc.kill()


def keep_log(name):
    """the serial log of a finished boot, kept for the CI artifact"""
    try:
        with open(g.LOG, "rb") as f:
            data = f.read()
        with open(os.path.join(g.LOG_DIR, name), "wb") as f:
            f.write(data)
    except OSError:
        pass


def main():
    part = os.environ.get("LOT88_PART", "all")
    if part not in ("all", "desk", "tv"):
        raise SystemExit("LOT88_PART must be all, desk or tv, not %r" % part)
    if not os.path.isfile(os.path.join(lw.PKI, "test_ca.pem")):
        subprocess.run([sys.executable, os.path.join(ROOT, "scripts", "lot67_gen_pki.py"), lw.PKI], check=True)
    os.makedirs(g.LOG_DIR, exist_ok=True)
    hp = 41000 + os.getpid() % 9000
    srv_log = open(SRV_LOG, "w")
    srv = subprocess.Popen([sys.executable, os.path.join(ROOT, "scripts", "lot67_web_server.py"), lw.PKI,
                            str(hp), str(hp + 1)], stdout=srv_log, stderr=subprocess.STDOUT)
    proc = mon = qmp = None
    t0 = time.monotonic()
    try:
        if part != "tv":
            proc, mon, qmp = boot_desk(hp)
        if part != "desk":
            proc, mon, qmp = boot_tv(hp)
        say("lot88 qemu (%s) ok in %.1fs" % (part, time.monotonic() - t0))
        return 0
    finally:
        stop(proc, mon, qmp)
        srv.terminate()
        try:
            srv.wait(timeout=5)
        except Exception:
            srv.kill()
        srv_log.close()


def boot_desk(hp):
    """Boot 1, 1x: the console steps, then the window chrome. Stops the guest."""
    proc, mon, qmp = boot(4, hp)
    try:
        g.wait_for(proc, "net-driver stack ring3 ready", g.CMD_TIMEOUT)
        time.sleep(0.5)
        u = "http://10.0.2.2:%d" % hp
        console_autofill(mon, proc)
        ctx = {"u": u, "mon": mon, "qmp": qmp, "proc": proc}
        for step in STEPS:
            step(ctx)
        docks = g.desktop_hit_centers(W, H)["docks"]
        q1 = chrome.run(qmp, mon, proc, docks=docks, want_q=lambda q: q == 4)
        say("window chrome at ui-scale 1x (q=%d): every window type, every button, the grip" % q1)
    finally:
        stop(proc, mon, qmp)
        keep_log("qemu-lot88-serial-1x.log")
    return None, None, None


def boot_tv(hp):
    """Boot 2: a TV EDID through fw_cfg and no scale pin. The safe TV
    scale (2x stepped down to 1.5x for 1024x768) is taken on this first
    boot, and the chrome follows it. Stops the guest."""
    edid = os.path.join(g.LOG_DIR, "lot88-tv-edid.bin")
    with open(edid, "wb") as f:
        f.write(tv_edid())
    proc, mon, qmp = boot(None, hp, edid=edid)
    try:
        g.wait_for(proc, "net-driver stack ring3 ready", g.CMD_TIMEOUT)
        boot_text = g.normalized_log(g.log_text())
        need(boot_text, ["[GFX] edid fw_cfg ", "[GFX] edid ext cea ok rev 3"], "fw_cfg TV EDID")
        time.sleep(1.0)
        out = lw.run(mon, proc, "ui-scale", "ui-scale ok", 20)
        need(out, ["ui-scale ok global 1.5x"], "first boot scale from the TV")
        out = lw.run(mon, proc, "display-info", "display tv-hint", 20)
        need(out, ["sink tv", "display tv-hint scale_q 8 safe_q 6 (stepped down for the framebuffer, applied 6)",
                   "display tv-hint overscan likely"], "display-info TV")
        say("TV EDID: 2x wanted, 1.5x safe on 1024x768 and taken on the first boot")
        step_energy(mon, proc)
        q2 = chrome.run(qmp, mon, proc, docks=None, want_q=lambda q: q == 6)
        say("window chrome at 1.5x from the TV hint (q=%d): buttons %d px, every window type" %
            (q2, chrome.py_btn(30, q2)))
    finally:
        stop(proc, mon, qmp)
        keep_log("qemu-lot88-serial-tv.log")
    return None, None, None


ENERGY_RE = re.compile(r"osui energy wake after (\d+) ms refresh (\d+) hz idle_fps10 (\d+) "
                       r"idle_drawn (\d+) idle_skipped (\d+)")


def step_energy(mon, proc):
    """US-081: saver with a 3 s dim timeout. The idle desktop dims at 3 s
    (7 Hz budget, mesh cap halved) and 6 s (5 Hz, quartered), idle wakes
    stop painting faster than that, and a key paints at once and wakes."""
    lw.run(mon, proc, "power-apply saver", "power-apply profile saver", 20)
    out = lw.run(mon, proc, "power-dim 3", "dim_after", 20)
    need(out, ["profile saver", "dim_after 3s"], "power-dim")
    out = lw.run(mon, proc, "power-dim 0x", "power-dim error", 20)
    start = len(g.log_text())
    g.send_command_until(mon, "gui", "osui gui live", proc, wait_prompt=False)
    g.wait_for(proc, "osui energy dim 1 after 3 s refresh 7 hz pace_cap_bps 32000", 60, start)
    g.wait_for(proc, "osui energy dim 2 after 6 s refresh 5 hz pace_cap_bps 16000", 60, start)
    time.sleep(4.0)
    mark = len(g.log_text())
    g.send_key(mon, "x")
    g.wait_for(proc, "osui energy wake after ", 30, mark)
    m = ENERGY_RE.search(g.normalized_log(g.log_text()[mark:]))
    if not m:
        raise RuntimeError("energy wake line unreadable")
    idle_ms, hz, fps10, drawn, skipped = (int(v) for v in m.groups())
    if hz != 15 or idle_ms < 9000 or drawn < 1:
        raise RuntimeError("energy wake: idle %d ms, %d hz, %d drawn, %d skipped" % (idle_ms, hz, drawn, skipped))
    # The contract: idle paints stay within the budget of each idle phase
    # (15 Hz until the 3 s dim, 7 Hz until 6 s, 5 Hz after), one frame of
    # slack per phase. How many wakes are skipped depends on how fast the
    # loop wakes: on the box it wakes about 10 times a second and skips
    # most; on a CI runner the TCG paint itself takes about 250 ms, the loop
    # wakes about 4 times a second, below every budget, and has nothing to
    # skip (run 42747597268: 43 drawn, 0 skipped in 10.6 s). So skips are
    # required only when the loop woke faster than the 7 Hz dim budget.
    budget = 15 * 3 + 7 * 3 + 5 * max(0, idle_ms - 6000) // 1000 + 3
    wakes = drawn + skipped
    if drawn > budget:
        raise RuntimeError("energy: %d idle frames in %d ms, budget %d" % (drawn, idle_ms, budget))
    if wakes * 1000 > 7 * idle_ms and skipped < 10:
        raise RuntimeError("energy: loop woke %d times in %d ms but skipped only %d" % (wakes, idle_ms, skipped))
    # the key painted: the next snapshot carries it
    g.wait_for(proc, "OSUI-SNAP", 20, mark)
    g.send_key(mon, "esc")
    g.wait_for(proc, "osui gui input cleared", 20, mark)
    g.send_command_until(mon, "/console", "osui gui exit", proc, mode="getc", wait_prompt=False)
    time.sleep(1.0)
    out = lw.run(mon, proc, "power-dim default", "dim_after", 20)
    need(out, ["dim_after 60s"], "power-dim default")
    lw.run(mon, proc, "power-apply balanced", "power-apply profile balanced", 20)
    say("energy: saver dims the idle desktop at 3 s and 6 s (7 then 5 Hz, mesh cap 32000 then 16000), "
        "%d idle wakes skipped and %d painted in %.1f s (%.1f fps, budget %d frames), a key woke it at 15 Hz" %
        (skipped, drawn, idle_ms / 1000.0, fps10 / 10.0, budget))


def step_scope(ctx):
    mon, proc, u = ctx["mon"], ctx["proc"], ctx["u"]
    lw.run(mon, proc, "web-css on", "web-css on", 20)
    lw.run(mon, proc, "web-open %s/lot88/scope.html" % u, "osui web page sections", fail=["error="])
    css = lw.run(mon, proc, "web-css", "css-scope ", 30)
    need(css, ["css-scope scopes 2 rules 3", "node div .card88 display auto uppercase"], "web-css @scope")
    read = lw.run(mon, proc, "web-read", "fin88", 30)
    # the card is uppercase (:scope): compare without case
    read = read.lower()
    need(read, ["scopekeep88", "scopeout88", "implkeep88", "allback88"], "web-read @scope")
    refuse(read, ["scopehide88", "implhide88"], "web-read @scope")
    lw.run(mon, proc, "web-css off", "web-css off", 20)
    say("@scope: the limit and the outside kept, the scoped p and the implicit-scope em hidden, "
        ":scope uppercase, all: revert undid display none")


def step_props(ctx):
    """US-031: background, list-style, font-size, text-indent and
    letter-spacing from the page's sheet, as the reader keeps them."""
    mon, proc, u = ctx["mon"], ctx["proc"], ctx["u"]
    lw.run(mon, proc, "web-css on", "web-css on", 20)
    lw.run(mon, proc, "web-open %s/lot88/props.html" % u, "osui web page sections", fail=["error="])
    css = lw.run(mon, proc, "web-css", "css-scope ", 30)
    need(css, ["node p #big88 display auto bg #ffeedd size 1.5em ls 2px",
               "node p #ind88 display auto indent 2em",
               "node ol #rom88 .roman88 display auto list upper-roman"], "web-css properties")
    for line in css.splitlines():
        if "#bad88" in line and " list " in line:
            raise RuntimeError("an invalid list-style was kept: %r" % line)
        if "#res88" in line and (" size " in line or " bg " in line):
            raise RuntimeError("all: initial did not reset: %r" % line)
    read = lw.run(mon, proc, "web-read", "fin88p", 30)
    need(read, ["propbig88", "propind88", "romi88", "badli88", "propreset88"], "web-read properties")
    lw.run(mon, proc, "web-css off", "web-css off", 20)
    say("properties: background, font-size 1.5em, letter-spacing, text-indent and list-style upper-roman kept; "
        "an invalid list-style dropped; all: initial reset font-size")


# later steps of the lot (each takes the context dict)
STEPS = [step_scope, step_props]


if __name__ == "__main__":
    try:
        sys.exit(main() or 0)
    except Exception as e:
        print("[lot88] FAIL: %s" % e, flush=True)
        sys.exit(1)
