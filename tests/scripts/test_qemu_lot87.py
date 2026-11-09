#!/usr/bin/env python3
"""Lot 87 in QEMU, offline: the Lot 67 test server runs on the host and the
guest reaches it as 10.0.2.2 (slirp).

  * EDID: the boot log says what the first extension block is (a CEA-861
    summary, "tag 0x.. not parsed", or "none"); whatever QEMU gives.
    display-info (SYS_DISPLAY_INFO) reports the same revision, SVD count
    and best mode on its "display cea" line.
  * external stylesheets: web-css on, web-open /lot87/page.html fetches
    main.css and its @import base.css, never print.css; the missing
    @import fails; web-read no longer shows the banner and the side note.
  * cascade layers: /lot87/layers.html imports reset.css into a layer and
    skips gone.css (supports() false); the unlayered rule keeps the block
    the more specific layered rule hides; an unlayered revert-layer goes
    back to the reset layer; display: initial (inline) beats the layer's
    none.
  * script walls: /lot87/wall.html is named an enablejs wall with its
    noscript fallback; /lot87/blocked (403) is named a block page.
  * autofill forget HOST NAME VALUE forgets one value; the GUI's Delete
    on a highlighted row of the list forgets that one.
  * GUI form2 autofill: two values remembered from the console (web-fill
    remember, grace then ada) are offered when the tablet clicks the field
    of the rendered page; the list is drawn under the field (its highlight
    row is found in a screendump); Down fills ada, Down grace, Up ada
    again; a tablet click on the second row picks grace and closes the
    list; Enter posts it (the server log shows the body).
"""
from __future__ import print_function

import os
import subprocess
import sys
import time

HERE = os.path.dirname(os.path.abspath(__file__))
sys.path.insert(0, HERE)
LOGS = os.path.join(HERE, "..", "..", "test_logs")
os.environ.setdefault("OSUI_GUI_LOG", os.path.join(LOGS, "qemu-lot87-serial.log"))
os.environ.setdefault("OSUI_GUI_MON_SOCK", "/tmp/mohhdy-lot87-mon.sock")
os.environ.setdefault("OSUI_GUI_QMP_SOCK", "/tmp/mohhdy-lot87-qmp.sock")
os.environ.setdefault("OVERLAY_DISK", os.path.join(LOGS, "qemu-lot87-overlay.img"))
import test_qemu_osui_gui as g  # noqa: E402
import test_qemu_lot67_web as lw  # noqa: E402
from test_qemu_lot71 import form2_input_boxes  # noqa: E402
from test_qemu_gui_windows import W, H, BAR_H  # noqa: E402

ROOT = g.ROOT
SRV_LOG = os.path.join(g.LOG_DIR, "lot87-web-server.log")


def list_highlight(rgb, cx, cy):
    """Lot 87: the list highlight (0xCCE4FF) in the band under a field
    centred at cx, cy (screen pixels): (pixels, x0, y0, x1, y1)."""
    n = 0
    x0 = y0 = 1 << 30
    x1 = y1 = -1
    for y in range(max(0, cy - 60), min(H, cy + 90)):
        for x in range(max(0, cx - 260), min(W, cx + 320)):
            i = (y * W + x) * 3
            if abs(rgb[i] - 0xCC) <= 10 and abs(rgb[i + 1] - 0xE4) <= 10 and abs(rgb[i + 2] - 0xFF) <= 10:
                n += 1
                x0, y0, x1, y1 = min(x0, x), min(y0, y), max(x1, x), max(y1, y)
    return n, x0, y0, x1, y1


def say(msg):
    print("[lot87] %s" % msg, flush=True)


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


def main():
    if not os.path.isfile(os.path.join(lw.PKI, "test_ca.pem")):
        subprocess.run([sys.executable, os.path.join(ROOT, "scripts", "lot67_gen_pki.py"), lw.PKI], check=True)
    os.makedirs(g.LOG_DIR, exist_ok=True)
    for path in (g.LOG, g.MON_SOCK, g.QMP_SOCK):
        try:
            os.remove(path)
        except OSError:
            pass
    hp = 41000 + os.getpid() % 9000
    srv_log = open(SRV_LOG, "w")
    srv = subprocess.Popen([sys.executable, os.path.join(ROOT, "scripts", "lot67_web_server.py"), lw.PKI,
                            str(hp), str(hp + 1)], stdout=srv_log, stderr=subprocess.STDOUT)
    proc = mon = qmp = None
    t0 = time.monotonic()
    try:
        g.prepare_test_disk()
        with open(os.devnull, "wb") as err:
            proc = subprocess.Popen([
                "qemu-system-i386", "-fw_cfg", "name=opt/mohhdy/ui-scale,string=4",
                "-cpu", os.environ.get("LOT87_CPU", "max"),
                "-kernel", g.KERNEL, "-initrd", g.INITRD, "-m", "1024M",
                "-display", "none", "-vga", "std",
                "-usb", "-device", "usb-tablet",
                "-serial", "file:" + g.LOG,
                "-monitor", "unix:%s,server,nowait" % g.MON_SOCK,
                "-qmp", "unix:%s,server,nowait" % g.QMP_SOCK,
                "-machine", "type=pc,accel=tcg", "-no-reboot", "-no-shutdown",
                "-netdev", "user,id=n0", "-device", "ne2k_isa,netdev=n0",
            ] + g.qemu_disk_args(), cwd=ROOT, stdout=err, stderr=err)
            g.wait_for(proc, "[0m : ", g.BOOT_TIMEOUT)
            # ---- EDID extension: reported as it is
            boot = g.normalized_log(g.log_text())
            ext = [ln for ln in boot.splitlines() if "[GFX] edid ext " in ln]
            if not ext:
                raise RuntimeError("no EDID extension line in the boot log: %r" % boot[-1500:])
            line = ext[0].split("[GFX] ", 1)[1]
            if not (line.startswith("edid ext cea ok rev ") or line == "edid ext none" or
                    (line.startswith("edid ext tag 0x") and line.endswith("not parsed"))):
                raise RuntimeError("EDID extension line: %r" % line)
            say("EDID: %s" % line)
            g.wait_for(proc, "usbdriver tablet enumerated port ", g.CMD_TIMEOUT)
            mon = g.monitor_connect()
            qmp = g.qmp_connect()
            g.wait_for(proc, "net-driver stack ring3 ready", g.CMD_TIMEOUT)
            time.sleep(0.5)
            u = "http://10.0.2.2:%d" % hp

            # ---- the same EDID extension through SYS_DISPLAY_INFO (display-info)
            di = lw.run(mon, proc, "display-info", "display cea", 20)
            cea = [ln for ln in g.normalized_log(di).splitlines() if "display cea" in ln]
            if not cea:
                raise RuntimeError("display-info without a cea line: %r" % di[-600:])
            if line.startswith("edid ext cea ok rev "):
                rev_svd = " ".join(line.split()[4:8])          # "rev R svd N"
                need(cea[-1], ["display cea " + rev_svd, " sink "], "display-info cea")
                best = line.split(" best ", 1)[1].split()[0]
                need(cea[-1], [" best " + best], "display-info best mode")
            elif line == "edid ext none":
                need(cea[-1], ["display cea none"], "display-info cea")
            say("display-info: %s" % cea[-1].strip())

            # ---- external stylesheets
            lw.run(mon, proc, "web-css on", "web-css on", 20)
            lw.run(mon, proc, "web-open %s/lot87/page.html" % u, "osui web page sections", fail=["error="])
            wait_server("CSS main.css")
            wait_server("CSS base.css")
            css = lw.run(mon, proc, "web-css", "css-links found", 30)
            need(css, ["css-links found 4 fetched 2 failed 1 media 1", "display none"], "web-css")
            refuse(server_text(), ["CSS print.css"], "print sheet")
            read = lw.run(mon, proc, "web-read", "Texte garde", 30)
            refuse(read, ["banner87", "sidenote87"], "web-read with sheets")
            say("external sheets: main.css + @import base.css applied, print.css not fetched, missing.css failed")
            # ---- cascade layers: @import layer(reset), supports() false
            lw.run(mon, proc, "web-open %s/lot87/layers.html" % u, "osui web page sections", fail=["error="])
            wait_server("CSS reset.css")
            css = lw.run(mon, proc, "web-css", "css-links found", 30)
            need(css, ["css-links found 2 fetched 2 failed 0 media 0", "layered 1 unsupported 1"], "web-css layers")
            refuse(server_text(), ["CSS gone.css"], "supports() false sheet")
            read = lw.run(mon, proc, "web-read", "fin87", 30)
            need(read, ["layerkeep87"], "web-read layers")
            # layerback87: the unlayered revert-layer goes back to the reset
            # layer, which hides it (without revert-layer the block shows)
            refuse(read, ["layergone87", "layerback87"], "web-read layers")
            need(css, ["node p .back87 display none", "node p .ini87 display inline"], "web-css revert-layer, initial")
            need(read, ["layerinitial87"], "web-read initial")
            lw.run(mon, proc, "web-css off", "web-css off", 20)
            say("layers: the unlayered rule beat the more specific layered one, revert-layer went back to "
                "the reset layer, gone.css not fetched")

            # ---- script wall and block page
            wall = lw.run(mon, proc, "web-open %s/lot87/wall.html" % u, "web-compat", 60)
            need(wall, ["wall enablejs", "needs JavaScript", "fallback /lot87/basic.html"], "wall page")
            blocked = lw.run(mon, proc, "web-open %s/lot87/blocked" % u, "web-compat", 60)
            need(blocked, ["wall blocked", "Not retried"], "block page")
            say("walls: enablejs with its fallback, 403 block page named")

            # ---- remember a value from the console
            lw.run(mon, proc, "web-open %s/form.html" % u, "osui web page sections 1", fail=["error"])
            # two values for the same field: grace first, then ada (newest)
            lw.run(mon, proc, "web-input 3 grace", "osui web input ok", 30)
            rem = lw.run(mon, proc, "web-fill remember", "web-fill remembered", 20)
            refuse(rem, ["remembered 0"], "web-fill remember")
            lw.run(mon, proc, "web-input 3 ada", "osui web input ok", 30)
            rem = lw.run(mon, proc, "web-fill remember", "web-fill remembered", 20)
            refuse(rem, ["remembered 0"], "web-fill remember")
            listed = lw.run(mon, proc, "autofill list", "autofill ", 20)
            need(listed, ["10.0.2.2 user values 2"], "autofill list")
            # the console editor lists both, newest first (same store)
            offers = lw.run(mon, proc, "web-fill 3", "web-fill field", 20)
            need(offers, ["offers 2", "offer 0 user ada", "offer 1 user grace"], "web-fill offers")
            # a third value, then forget just that one from the console
            lw.run(mon, proc, "web-input 3 temp87", "osui web input ok", 30)
            lw.run(mon, proc, "web-fill remember", "web-fill remembered", 20)
            listed = lw.run(mon, proc, "autofill list", "autofill ", 20)
            need(listed, ["10.0.2.2 user values 3"], "autofill list (3)")
            gone = lw.run(mon, proc, "autofill forget 10.0.2.2 user temp87", "autofill ", 20)
            need(gone, ["autofill forgot value left 2"], "autofill forget value")
            offers = lw.run(mon, proc, "web-fill 3", "web-fill field", 20)
            need(offers, ["offers 2", "offer 0 user ada", "offer 1 user grace"], "web-fill offers after forget")
            refuse(offers, ["temp87"], "web-fill offers after forget")
            say("console: temp87 remembered then forgotten alone; ada and grace kept")

            # ---- GUI: click the field, Down takes the value, Enter posts it
            g.send_command_until(mon, "gui", "osui gui live", proc, wait_prompt=False)
            g.wait_for(proc, "OSUI-SNAP", g.CMD_TIMEOUT)
            time.sleep(1.0)
            start = len(g.log_text())
            lw.type_line(mon, proc, "web %s/form.html" % u, mode="getc")
            g.wait_for(proc, "osui web fetch ok status 200", lw.FETCH_TIMEOUT, start)
            time.sleep(2.0)
            _, _, rgb = g.screendump_rgb(mon, "qemu-lot87-form.ppm")
            ox, oy = 16, BAR_H + 48
            boxes = form2_input_boxes(rgb, ox, oy + 10, ox + 600, oy + 320)
            if len(boxes) < 2:
                raise RuntimeError("form page: %d bordered inputs found" % len(boxes))
            offered = None
            for (fx, fy, bw, bh) in boxes[:4]:
                start = len(g.log_text())
                g.click_tablet(qmp, fx, fy, W, H)
                try:
                    g.wait_for(proc, "osui web form2 changed focus", 8.0, start)
                except RuntimeError:
                    continue
                time.sleep(0.6)
                if "osui web autofill offer name=user key=Down values=2" in g.normalized_log(g.log_text()[start:]):
                    offered = (fx, fy)
                    break
            if not offered:
                raise RuntimeError("no autofill offer for the user field; boxes %r tail %r" %
                                   (boxes, g.log_text()[-1500:]))
            say("GUI: click at %d,%d offered the two remembered user values" % offered)
            # the list is drawn under the field: its highlight row is there
            time.sleep(1.0)
            _, _, rgb = g.screendump_rgb(mon, "qemu-lot87-list.ppm")
            hl, hx0, hy0, hx1, hy1 = list_highlight(rgb, offered[0], offered[1])
            if hl < 200:
                raise RuntimeError("autofill list not drawn under the field (%d highlight px)" % hl)
            say("GUI: the list is drawn under the field (%d highlight px)" % hl)
            start = len(g.log_text())
            g.send_key(mon, "down")
            g.wait_for(proc, "osui web autofill applied name=user pick 1/2", g.CMD_TIMEOUT, start)
            time.sleep(0.6)
            start = len(g.log_text())
            g.send_key(mon, "down")
            g.wait_for(proc, "osui web autofill applied name=user pick 2/2", g.CMD_TIMEOUT, start)
            time.sleep(0.6)
            start = len(g.log_text())
            g.send_key(mon, "up")
            g.wait_for(proc, "osui web autofill applied name=user pick 1/2", g.CMD_TIMEOUT, start)
            time.sleep(0.8)
            # the tablet picks the second row: the highlight band is the
            # first row, the second is just under it
            row_h = hy1 - hy0 + 1
            start = len(g.log_text())
            g.click_tablet(qmp, (hx0 + hx1) // 2, hy1 + row_h // 2 + 1, W, H)
            g.wait_for(proc, "osui web autofill applied name=user pick 2/2 click", g.CMD_TIMEOUT, start)
            say("GUI: the tablet picked the second row (grace)")
            time.sleep(0.8)
            start = len(g.log_text())
            g.send_key(mon, "ret")
            g.wait_for(proc, "osui web form2 submit POST", lw.FETCH_TIMEOUT, start)
            wait_server("ECHO POST q='' body=b'user=grace&msg=bonjour'")
            say("GUI: Down ada, Down grace, Up ada, click grace; Enter posted user=grace")
            g.send_key(mon, "esc")
            time.sleep(0.5)

            # ---- GUI: Delete on a highlighted row forgets that value
            start = len(g.log_text())
            lw.type_line(mon, proc, "web %s/form.html" % u, mode="getc")
            g.wait_for(proc, "osui web fetch ok status 200", lw.FETCH_TIMEOUT, start)
            time.sleep(2.0)
            start = len(g.log_text())
            g.click_tablet(qmp, offered[0], offered[1], W, H)
            g.wait_for(proc, "osui web autofill offer name=user", g.CMD_TIMEOUT, start)
            time.sleep(0.6)
            offer_line = [ln for ln in g.normalized_log(g.log_text()[start:]).splitlines()
                          if "osui web autofill offer name=user" in ln][-1]
            nvals = int(offer_line.split("values=")[1].split()[0]) if "values=" in offer_line else 1
            start = len(g.log_text())
            g.send_key(mon, "down")
            g.wait_for(proc, "osui web autofill applied name=user", g.CMD_TIMEOUT, start)
            time.sleep(0.6)
            start = len(g.log_text())
            g.send_key(mon, "delete")
            want = "osui web autofill removed name=user row 1 left %d" % (nvals - 1)
            g.wait_for(proc, want, g.CMD_TIMEOUT, start)
            say("GUI: Delete forgot the highlighted value (%d -> %d)" % (nvals, nvals - 1))
            g.send_key(mon, "esc")
            time.sleep(0.4)
            g.send_key(mon, "esc")
            time.sleep(0.5)
            say("lot87 qemu ok in %.1fs" % (time.monotonic() - t0))
            return 0
    finally:
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
        srv.terminate()
        try:
            srv.wait(timeout=5)
        except Exception:
            srv.kill()
        srv_log.close()


if __name__ == "__main__":
    try:
        sys.exit(main() or 0)
    except Exception as e:
        print("[lot87] FAIL: %s" % e, flush=True)
        sys.exit(1)
