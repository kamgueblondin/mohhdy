#!/usr/bin/env python3
"""Lot 85: masonry auto columns, packed-height insets, local search, text scale, shared chrome.

- web-open /web/lot85.html
- #a is the tall item in column 0 (10px). #b and the wide #c pack into column 1.
- #ia top:50% of the packed 80px grid is 40px below #ic, not the token 50.
- search-engine local / search fetch hits the guest catalogue (no network).
- text-size accepts every window slot.
- A USB tablet click minimizes the full-screen web window (shared chrome).
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
os.environ.setdefault("OSUI_GUI_LOG", os.path.join(LOGS, "qemu-lot85-serial.log"))
os.environ.setdefault("OSUI_GUI_MON_SOCK", "/tmp/mohhdy-lot85-mon.sock")
os.environ.setdefault("OSUI_GUI_QMP_SOCK", "/tmp/mohhdy-lot85-qmp.sock")
os.environ.setdefault("OVERLAY_DISK", os.path.join(LOGS, "qemu-lot85-overlay.img"))
import test_qemu_osui_gui as g  # noqa: E402
import test_qemu_lot67_web as lw  # noqa: E402
import test_qemu_gui_term_window as tw  # noqa: E402
from test_qemu_gui_windows import btn, click, BAR_H, W, H  # noqa: E402

ROOT = g.ROOT


def say(msg):
    print("[lot85] %s" % msg, flush=True)


def main():
    lw.ALIASES.setdefault("#", "shift-3")
    if not os.path.isfile(os.path.join(lw.PKI, "test_ca.pem")):
        subprocess.run([sys.executable, os.path.join(ROOT, "scripts", "lot67_gen_pki.py"), lw.PKI], check=True)
    os.makedirs(g.LOG_DIR, exist_ok=True)
    for path in (g.LOG, g.MON_SOCK, g.QMP_SOCK):
        try:
            os.remove(path)
        except OSError:
            pass
    proc = mon = qmp = None
    t0 = time.monotonic()
    try:
        g.prepare_test_disk()
        with open(os.devnull, "wb") as err:
            proc = subprocess.Popen([
                "qemu-system-i386", "-fw_cfg", "name=opt/mohhdy/ui-scale,string=4",
                "-cpu", os.environ.get("LOT85_CPU", "max"),
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
            g.wait_for(proc, "usbdriver tablet enumerated port ", g.CMD_TIMEOUT)
            mon = g.monitor_connect()
            qmp = g.qmp_connect()
            g.wait_for(proc, "net-driver stack ring3 ready", g.CMD_TIMEOUT)
            time.sleep(0.5)

            lw.run(mon, proc, "web-open /web/lot85.html", "osui web", 60, fail=["error="])

            def box(ident):
                dump = lw.run(mon, proc, "web-inspect /web/lot85.html %s" % ident, "web-inspect", 30)
                say("inspect %s: %r" % (ident, dump[-300:]))
                m = re.search(r"box (\d+) (\d+) (\d+)x(\d+)", dump)
                if not m:
                    raise RuntimeError("lot85 #%s missing box: %r" % (ident, dump[-300:]))
                return tuple(int(v) for v in m.groups())

            a = box("a")
            b = box("b")
            c = box("c")
            if a[2] != 10 or a[3] != 80:
                raise RuntimeError("lot85 #a size %r" % (a,))
            if b[0] != a[0] + 10 or b[1] != a[1]:
                raise RuntimeError("lot85 #b %r not beside #a %r" % (b, a))
            if c[0] != a[0] + 10 or c[1] != a[1] + 10 or c[2] != 70:
                raise RuntimeError("lot85 #c %r not packed under #b (a %r)" % (c, a))
            ic = box("ic")
            ia = box("ia")
            if ia[1] != ic[1] + 40:
                raise RuntimeError("lot85 #ia y %d, #ic y %d (50%% of 80 is 40)" % (ia[1], ic[1]))
            vn = box("vn")
            if vn[2] < 8:
                raise RuntimeError("lot85 #vn has no width: %r" % (vn,))

            eng = lw.run(mon, proc, "search-engine local", "search-engine ok", 20)
            if "local" not in eng:
                raise RuntimeError("lot85 search-engine: %r" % eng[-200:])
            found = lw.run(mon, proc, "search fetch", "search ok", 20)
            if "layout local" not in found or "results 1" not in found:
                raise RuntimeError("lot85 local search: %r" % found[-300:])

            for slot in ("web", "chat", "shell", "pane", "ide", "app0"):
                line = lw.run(mon, proc, "text-size %s 2" % slot, "text-size ok", 20)
                if slot not in line:
                    raise RuntimeError("lot85 text-size %s: %r" % (slot, line[-200:]))

            # Shared chrome: full-screen web and the editor, maximize and a corner drag.
            # Search forms on a local file are proved by the host tests: web-forms
            # only lists forms of an http(s) page (web_nsections).
            g.send_command_until(mon, "gui", "osui gui live", proc, wait_prompt=False)
            g.send_command_until(mon, "/browse", "osui web", proc, mode="getc", wait_prompt=False)
            time.sleep(1.5)
            full = (8, BAR_H + 8, W - 16, H - BAR_H - 16)
            docks = g.desktop_hit_centers(W, H)["docks"]
            click(qmp, proc, *btn(full, "min", 30), marker="gfx wm full minimize", label="web minimize")
            click(qmp, proc, docks[1][0], docks[1][1], "gfx wm full restore", "dock Browser restore")
            click(qmp, proc, *btn(full, "max", 30), marker="gfx wm full maximize off", label="web restore size")
            aw, ah = W - 16, H - BAR_H - 16
            rest = (8 + (aw - aw * 3 // 4) // 2, BAR_H + 8 + (ah - ah * 3 // 4) // 2, aw * 3 // 4, ah * 3 // 4)
            start = len(g.log_text())
            tw.move(qmp, rest[0] + rest[2] - 3, rest[1] + rest[3] - 3)
            tw.button(qmp, True)
            tw.move(qmp, rest[0] + rest[2] - 83, rest[1] + rest[3] - 63)
            tw.button(qmp, False)
            g.wait_for(proc, "gfx wm full resized", g.CMD_TIMEOUT, start)
            shrunk = (rest[0], rest[1], rest[2] - 80, rest[3] - 60)
            click(qmp, proc, *btn(shrunk, "max", 30), marker="gfx wm full maximize on", label="web maximize")
            click(qmp, proc, *btn(full, "close", 30), marker="gfx wm full close", label="web close")
            g.send_command_until(mon, "pm-ide w85.pm", "osui pm-ide open fullscreen", proc,
                                 mode="getc", wait_prompt=False)
            time.sleep(1.5)
            click(qmp, proc, *btn(full, "min", 30), marker="gfx wm full minimize", label="editor minimize")
            click(qmp, proc, docks[7][0], docks[7][1], "gfx wm full restore", "dock editor restore")
            click(qmp, proc, *btn(full, "max", 30), marker="gfx wm full maximize off", label="editor restore size")
            # The web drag already stored dw/dh on the shared full window,
            # so the editor restores at that smaller size, not at rest.
            ed = (rest[0], rest[1], rest[2] - 80, rest[3] - 60)
            start = len(g.log_text())
            tw.move(qmp, ed[0] + ed[2] - 3, ed[1] + ed[3] - 3)
            tw.button(qmp, True)
            tw.move(qmp, ed[0] + ed[2] - 83, ed[1] + ed[3] - 63)
            tw.button(qmp, False)
            g.wait_for(proc, "gfx wm full resized", g.CMD_TIMEOUT, start)
            ed2 = (ed[0], ed[1], ed[2] - 80, ed[3] - 60)
            click(qmp, proc, *btn(ed2, "close", 30), marker="gfx wm full close", label="editor close")
            say("lot85 qemu ok in %.1fs" % (time.monotonic() - t0))
            return 0
    finally:
        if mon:
            try:
                mon.close()
            except Exception:
                pass
        if qmp:
            try:
                qmp.close()
            except Exception:
                pass
        if proc and proc.poll() is None:
            proc.terminate()
            try:
                proc.wait(timeout=5)
            except Exception:
                proc.kill()


if __name__ == "__main__":
    try:
        sys.exit(main() or 0)
    except Exception as e:
        print("[lot85] FAIL: %s" % e, flush=True)
        sys.exit(1)
