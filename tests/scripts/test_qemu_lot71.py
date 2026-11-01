#!/usr/bin/env python3
"""Lot 71: forms, long pages, search config, full-screen window and chat
width in QEMU, offline and deterministic.

The Lot 67 test server (scripts/lot67_web_server.py) runs on the host and
the guest reaches it as 10.0.2.2 (slirp). Console part (same code as the
GUI input line, osui_gui_web_command):
  * web-open http://H/form.html, web-forms, web-input 0 TEXT, web-submit:
    the server receives GET /echo?q=TEXT&lang=fr; web-input 3 + web-submit 3:
    POST /echo body user=...&msg=bonjour;
  * web-open http://H/long.html: 26 sections, web-section 26 shows the end;
  * search-config searx http://H/searx + search-engine searx: 2 results.
GUI part (USB tablet clicks on the VBE desktop, 1024x768, ui-scale 1x):
  * the form page: click the search field, type, Enter submits (server
    log), screenshots before and after;
  * the Latin-1 page (accents drawn with the Latin-1 glyphs), screenshot;
  * full-screen web window: maximize restores a smaller window, the title
    drags it ("gfx wm full moved"), the grip resizes it ("gfx wm full
    resized"), the close button follows, maximize fills again;
  * Statut pane + floating chat: widening the pane by its grip keeps the
    chat width (measured on the screen).
Screenshots (PNG) go to LOT71_SHOTS (default test_logs/lot71-shots).
"""
from __future__ import print_function

import os
import subprocess
import sys
import time

HERE = os.path.dirname(os.path.abspath(__file__))
sys.path.insert(0, HERE)
LOGS = os.path.join(HERE, "..", "..", "test_logs")
os.environ.setdefault("OSUI_GUI_LOG", os.path.join(LOGS, "qemu-lot71-serial.log"))
os.environ.setdefault("OSUI_GUI_MON_SOCK", "/tmp/mohhdy-lot71-mon.sock")
os.environ.setdefault("OSUI_GUI_QMP_SOCK", "/tmp/mohhdy-lot71-qmp.sock")
os.environ.setdefault("OVERLAY_DISK", os.path.join(LOGS, "qemu-lot71-overlay.img"))
import test_qemu_osui_gui as g  # noqa: E402
import test_qemu_gui_term_window as tw  # noqa: E402
import test_qemu_lot67_web as lw  # noqa: E402
from test_qemu_gui_windows import btn, click, count, PANE, W, H, BAR_H  # noqa: E402

ROOT = g.ROOT
SHOTS = os.environ.get("LOT71_SHOTS", os.path.join(g.LOG_DIR, "lot71-shots"))
RED = (0xE0, 0x52, 0x4B)


def say(msg):
    print("[lot71] %s" % msg, flush=True)


def shot(mon, name):
    w, h, rgb = g.screendump_rgb(mon, "qemu-lot71-%s.ppm" % name)
    try:
        from PIL import Image
        os.makedirs(SHOTS, exist_ok=True)
        path = os.path.join(SHOTS, name + ".png")
        Image.frombytes("RGB", (w, h), bytes(rgb)).save(path)
        say("screenshot %s" % path)
    except ImportError:
        pass
    return rgb


def server_text():
    return open(os.path.join(g.LOG_DIR, "lot71-web-server.log")).read()


def wait_server(needle, timeout=60):
    deadline = time.monotonic() + timeout
    while time.monotonic() < deadline:
        if needle in server_text():
            return
        time.sleep(0.3)
    raise RuntimeError("server never saw %r; tail:\n%s" % (needle, server_text()[-1500:]))


def red_right(rgb, y0, y1, x0=0, x1=W):
    """rightmost red close button centre in a box"""
    best = None
    for y in range(y0, y1):
        for x in range(x1 - 1, x0, -1):
            i = (y * W + x) * 3
            p = rgb[i:i + 3]
            if abs(p[0] - RED[0]) <= 4 and abs(p[1] - RED[1]) <= 4 and abs(p[2] - RED[2]) <= 4:
                if best is None or x > best[0]:
                    best = (x, y)
                break
    if best is None:
        raise RuntimeError("no red close button in %d..%d" % (y0, y1))
    return best[0] - 7, best[1] + 4


def blue_link(rgb, x0, y0, x1, y1):
    """first row of link-coloured pixels (blue) in a box: (x, y) centre"""
    for y in range(y0, y1):
        xs = [x for x in range(x0, x1)
              if rgb[(y * W + x) * 3 + 2] > 150 and rgb[(y * W + x) * 3] < 90 and rgb[(y * W + x) * 3 + 1] < 140]
        if len(xs) > 6:
            return (xs[0] + xs[-1]) // 2 if xs[-1] - xs[0] < 200 else xs[0] + 40, y + 4
    raise RuntimeError("no link on the page")


def run_width(rgb, y, x_from):
    """length of the run of the colour at (x_from, y) going left (gaps <= 3)"""
    i = (y * W + x_from) * 3
    ref = rgb[i:i + 3]
    x, gap, last = x_from, 0, x_from
    while x > 0 and gap <= 3:
        j = (y * W + x) * 3
        p = rgb[j:j + 3]
        if all(abs(p[k] - ref[k]) <= 10 for k in range(3)):
            gap = 0
            last = x
        else:
            gap += 1
        x -= 1
    return x_from - last


def main():
    if not os.path.isfile(os.path.join(lw.PKI, "test_ca.pem")):
        subprocess.run([sys.executable, os.path.join(ROOT, "scripts", "lot67_gen_pki.py"), lw.PKI], check=True)
    os.makedirs(g.LOG_DIR, exist_ok=True)
    for path in (g.LOG, g.MON_SOCK, g.QMP_SOCK):
        try:
            os.remove(path)
        except OSError:
            pass
    hp = 40000 + os.getpid() % 10000
    srv_log = open(os.path.join(g.LOG_DIR, "lot71-web-server.log"), "w")
    srv = subprocess.Popen([sys.executable, os.path.join(ROOT, "scripts", "lot67_web_server.py"), lw.PKI, str(hp), str(hp + 1)],
                           stdout=srv_log, stderr=subprocess.STDOUT)
    proc = mon = qmp = None
    t0 = time.monotonic()
    try:
        g.prepare_test_disk()
        with open(os.devnull, "wb") as err:
            proc = subprocess.Popen([
                "qemu-system-i386", "-fw_cfg", "name=opt/mohhdy/ui-scale,string=4", "-cpu", os.environ.get("LOT71_CPU", "max"),
                "-kernel", g.KERNEL, "-initrd", g.INITRD, "-m", "1024M", "-display", "none", "-vga", "std",
                "-usb", "-device", "usb-tablet",
                "-serial", "file:" + g.LOG, "-monitor", "unix:%s,server,nowait" % g.MON_SOCK,
                "-qmp", "unix:%s,server,nowait" % g.QMP_SOCK,
                "-machine", "type=pc,accel=tcg", "-no-reboot", "-no-shutdown",
                "-netdev", "user,id=n0", "-device", "ne2k_isa,netdev=n0",
            ] + g.qemu_disk_args(), cwd=ROOT, stdout=err, stderr=err)
            g.wait_for(proc, "[0m : ", g.BOOT_TIMEOUT)
            g.wait_for(proc, "usbdriver tablet enumerated port ", g.CMD_TIMEOUT)
            mon = g.monitor_connect()
            qmp = g.qmp_connect()
            g.wait_for(proc, "net-driver stack ring3 ready", g.CMD_TIMEOUT)
            time.sleep(0.5)
            u = "http://10.0.2.2:%d" % hp
            # ---- console: forms, sections, search config (same code as the GUI line)
            out = lw.run(mon, proc, "web-open %s/form.html" % u, "osui web page sections 1", fail=["error"])
            if "forms 3 fields 8" not in out:
                raise RuntimeError("form page: %r" % out[-600:])
            out = lw.run(mon, proc, "web-forms", "field 7", 30)
            for want in ("form 0 GET http://10.0.2.2:%d/echo" % hp, "form 1 POST", "field 1 form 0 hidden name=lang value=fr"):
                if want not in out:
                    raise RuntimeError("web-forms missing %r: %r" % (want, out[-800:]))
            lw.run(mon, proc, "web-input 0 mohhdy os", "osui web input ok", 30)
            lw.run(mon, proc, "web-submit", "osui web form submit", fail=["form error"])
            wait_server("ECHO GET q='q=mohhdy+os&lang=fr'")
            lw.run(mon, proc, "web-open %s/form.html" % u, "osui web page sections 1", fail=["error"])
            lw.run(mon, proc, "web-input 3 moi", "osui web input ok", 30)
            out = lw.run(mon, proc, "web-submit 3", "osui web form submit", fail=["form error"])
            if "method POST" not in out:
                raise RuntimeError("POST submit status: %r" % out[-600:])
            wait_server("ECHO POST q='' body=b'user=moi&msg=bonjour'")
            say("console forms: GET and POST received by the server")
            out = lw.run(mon, proc, "web-open %s/long.html" % u, "osui web page sections 26", fail=["error"])
            out = lw.run(mon, proc, "web-section 26", "osui web section 26/26", 30)
            lw.run(mon, proc, "web-section 99", "error=out_of_range", 30)
            lw.run(mon, proc, "web-section 1", "osui web section 1/26", 30)
            lw.run(mon, proc, "web-find fin-de-page", "osui web find ok section 26/26 matches 1", 30)
            lw.run(mon, proc, "web-find paragraphe", "osui web find ok section 26/26 matches 1500", 30)   # from the current section
            lw.run(mon, proc, "web-find", "osui web find ok section 1/26 matches 1500", 30)                # next match wraps
            lw.run(mon, proc, "web-find introuvable", "osui web find none", 30)
            say("long page: 26 sections, the last one reached, web-find jumps to the match")
            lw.run(mon, proc, "search-config searx %s/searx" % u, "search-config engine", 30)
            lw.run(mon, proc, "search-engine searx", "search-engine ok searx", 30)
            out = lw.run(mon, proc, "search mohhdy", "search end", fail=["search error"])
            if "search ok results 2 layout searx-html" not in out:
                raise RuntimeError("searx search: %r" % out[-800:])
            wait_server("SEARX q='mohhdy'")
            lw.run(mon, proc, "search-config clear", "search-config engine ddg", 30)
            say("search: configured SearXNG engine used, config cleared")
            # ---- checkbox, radio, select from the console
            lw.run(mon, proc, "web-open %s/controls.html" % u, "osui web page sections 1", fail=["error"])
            out = lw.run(mon, proc, "web-forms", "field 4", 30)
            for want in ("field 0 form 0 checkbox name=safe value=on unchecked", "field 2 form 0 radio name=t value=img unchecked",
                         'field 3 form 0 select name=lr value=lang_fr options "" *lang_fr lang_en'):
                if want not in out:
                    raise RuntimeError("controls web-forms missing %r: %r" % (want, out[-800:]))
            lw.run(mon, proc, "web-input 0 on", "osui web input ok", 30)
            lw.run(mon, proc, "web-input 2 on", "osui web input ok", 30)
            lw.run(mon, proc, "web-input 3 anglais", "osui web input ok", 30)
            lw.run(mon, proc, "web-submit 0", "osui web form submit", fail=["form error"])
            wait_server("ECHO GET q='safe=on&t=img&lr=lang_en'")
            say("console controls: checkbox, radio and select submitted")
            # ---- cookies: a login redirect keeps its session cookie
            lw.run(mon, proc, "web-open %s/cookie/login" % u, "osui web page sections 1", fail=["error"])
            wait_server("COOKIE home='sid=s3cr3t-session; theme=dark'")
            out = lw.run(mon, proc, "cookies", "values hidden", 30)
            if "cookies on count 2" not in out or "sid len=14" not in out or "s3cr3t" in out:
                raise RuntimeError("cookies list: %r" % out[-600:])
            lw.run(mon, proc, "cookies clear", "cookies cleared 2", 30)
            say("cookies: session cookie sent after the redirect, values hidden, cleared")
            # ---- GUI
            g.send_command_until(mon, "gui", "osui gui live", proc, wait_prompt=False)
            g.wait_for(proc, "OSUI-SNAP", g.CMD_TIMEOUT)
            time.sleep(1.0)
            start = len(g.log_text())
            lw.type_line(mon, proc, "web %s/form.html" % u, mode="getc")
            g.wait_for(proc, "osui web fetch ok status 200", lw.FETCH_TIMEOUT, start)
            time.sleep(2.0)
            rgb = shot(mon, "form-page")
            ox, oy = 16, BAR_H + 48
            fx, fy = blue_link(rgb, ox, oy + 40, ox + 700, oy + 300)
            start = len(g.log_text())
            for _ in range(3):
                g.click_tablet(qmp, fx, fy, W, H)
                try:
                    g.wait_for(proc, "osui web form focus f:0", 8.0, start)
                    break
                except RuntimeError:
                    continue
            else:
                raise RuntimeError("click on the field at %d,%d gave no focus" % (fx, fy))
            for ch in "gui":
                g.send_key(mon, ch)
                time.sleep(0.4)
            time.sleep(1.0)
            shot(mon, "form-typed")
            start = len(g.log_text())
            g.send_key(mon, "ret")
            g.wait_for(proc, "osui web form submit", lw.FETCH_TIMEOUT, start)
            wait_server("ECHO GET q='q=gui&lang=fr'")
            time.sleep(1.5)
            shot(mon, "form-answer")
            say("GUI form: click, typing and Enter submitted q=gui")
            g.send_key(mon, "esc")
            time.sleep(0.8)
            start = len(g.log_text())
            lw.type_line(mon, proc, "web %s/controls.html" % u, mode="getc")
            g.wait_for(proc, "osui web fetch ok status 200", lw.FETCH_TIMEOUT, start)
            time.sleep(2.0)
            rgb = shot(mon, "controls-before")
            fx, fy = blue_link(rgb, ox, oy, ox + 700, oy + 200)
            start = len(g.log_text())
            for _ in range(3):
                g.click_tablet(qmp, fx, fy, W, H)
                try:
                    g.wait_for(proc, "osui web form toggle f:0", 8.0, start)
                    break
                except RuntimeError:
                    continue
            else:
                raise RuntimeError("click on the checkbox at %d,%d did not toggle it" % (fx, fy))
            time.sleep(1.5)
            shot(mon, "controls-checked")
            say("GUI form: a click toggles the checkbox")
            # The click also focuses the control (keyboard goes on from it):
            # the first ESC leaves the field, the second closes the pane.
            start = len(g.log_text())
            g.send_key(mon, "esc")
            g.wait_for(proc, "osui web form blur", g.CMD_TIMEOUT, start)
            start = len(g.log_text())
            g.send_key(mon, "esc")
            g.wait_for(proc, "osui web close", g.CMD_TIMEOUT, start)
            time.sleep(0.8)
            start = len(g.log_text())
            lw.type_line(mon, proc, "web %s/latin1.html" % u, mode="getc")
            g.wait_for(proc, "osui web fetch ok status 200", lw.FETCH_TIMEOUT, start)
            time.sleep(2.0)
            shot(mon, "latin1-accents")
            # ---- full-screen window: restore, move, resize, maximize
            full = (8, BAR_H + 8, W - 16, H - BAR_H - 16)
            click(qmp, proc, *btn(full, "max", 30), marker="gfx wm full maximize off", label="web restore")
            aw, ah = W - 16, H - BAR_H - 16
            rest = [8 + (aw - aw * 3 // 4) // 2, BAR_H + 8 + (ah - ah * 3 // 4) // 2, aw * 3 // 4, ah * 3 // 4]
            time.sleep(1.0)
            rgb = shot(mon, "web-restored")
            cx, cy = red_right(rgb, rest[1], rest[1] + 30)
            if abs(cx - btn(rest, "close", 30)[0]) > 3:
                raise RuntimeError("restored window: close button at %d, expected %d" % (cx, btn(rest, "close", 30)[0]))
            start = len(g.log_text())
            tw.move(qmp, rest[0] + 300, rest[1] + 10)
            tw.button(qmp, True)
            time.sleep(0.3)
            tw.move(qmp, rest[0] + 240, rest[1] + 40)
            time.sleep(0.3)
            tw.button(qmp, False)
            g.wait_for(proc, "gfx wm full moved", g.CMD_TIMEOUT, start)
            rest[0] -= 60
            rest[1] += 30
            start = len(g.log_text())
            gx, gy = rest[0] + rest[2] - 3, rest[1] + rest[3] - 3
            tw.move(qmp, gx, gy)
            tw.button(qmp, True)
            time.sleep(0.3)
            tw.move(qmp, gx - 100, gy - 60)
            time.sleep(0.3)
            tw.button(qmp, False)
            g.wait_for(proc, "gfx wm full resized", g.CMD_TIMEOUT, start)
            rest[2] -= 100
            rest[3] -= 60
            time.sleep(1.5)
            rgb = shot(mon, "web-moved-resized")
            cx, cy = red_right(rgb, rest[1], rest[1] + 30, 0, rest[0] + rest[2] + 4)
            if abs(cx - btn(rest, "close", 30)[0]) > 3:
                raise RuntimeError("resized window: close button at %d, expected %d" % (cx, btn(rest, "close", 30)[0]))
            click(qmp, proc, *btn(rest, "max", 30), marker="gfx wm full maximize on", label="web maximize")
            time.sleep(1.0)
            shot(mon, "web-maximized")
            click(qmp, proc, *btn(full, "close", 30), marker="gfx wm full close", label="web close")
            say("full-screen web window: restore, move, resize, maximize, close")
            # ---- chat width kept when the pane is resized
            time.sleep(1.5)
            hits = g.desktop_hit_centers(W, H)
            docks = hits["docks"]
            click(qmp, proc, docks[5][0], docks[5][1], "line=/status", "dock Statut")
            time.sleep(2.5)
            rgb = shot(mon, "chat-before")
            x, y, w, h = PANE
            ccx, ccy = red_right(rgb, BAR_H + 8, H - 60, x + w + 4, W - 96)
            width0 = run_width(rgb, y + h + 12, ccx - 40)   # below the pane, never covered
            start = len(g.log_text())
            tw.move(qmp, x + w - 3, y + h - 3)
            tw.button(qmp, True)
            time.sleep(0.3)
            tw.move(qmp, x + w + 117, y + h - 3)
            time.sleep(0.3)
            tw.button(qmp, False)
            g.wait_for(proc, "gfx wm pane resized", g.CMD_TIMEOUT, start)
            time.sleep(1.5)
            rgb = shot(mon, "chat-after-pane-resize")
            ccx2, ccy2 = red_right(rgb, BAR_H + 8, H - 60, x + w + 4, W - 96)
            width1 = run_width(rgb, y + h + 12, ccx2 - 40)
            if ccx2 != ccx:
                raise RuntimeError("chat moved with the pane: close x %d -> %d" % (ccx, ccx2))
            say("chat body run %d px before, %d px after the pane grew by 120 px" % (width0, width1))
            if abs(width1 - width0) > 6:
                raise RuntimeError("chat width changed with the pane: %d -> %d" % (width0, width1))
        say("QEMU Lot 71 contract passed in %.0f s." % (time.monotonic() - t0))
        return 0
    finally:
        if qmp is not None:
            qmp.close()
        if mon is not None:
            mon.close()
        g.terminate(proc)
        srv.terminate()


if __name__ == "__main__":
    try:
        raise SystemExit(main())
    except Exception as error:
        print("QEMU Lot 71 failed: %s" % error, file=sys.stderr)
        raise SystemExit(1)
