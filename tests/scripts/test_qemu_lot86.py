#!/usr/bin/env python3
"""Lot 86: page-5 letter, settings, bookmark, shared chrome on the web window."""
from __future__ import print_function

import os
import subprocess
import sys
import time

HERE = os.path.dirname(os.path.abspath(__file__))
sys.path.insert(0, HERE)
LOGS = os.path.join(HERE, "..", "..", "test_logs")
os.environ.setdefault("OSUI_GUI_LOG", os.path.join(LOGS, "qemu-lot86-serial.log"))
os.environ.setdefault("OSUI_GUI_MON_SOCK", "/tmp/mohhdy-lot86-mon.sock")
os.environ.setdefault("OSUI_GUI_QMP_SOCK", "/tmp/mohhdy-lot86-qmp.sock")
os.environ.setdefault("OVERLAY_DISK", os.path.join(LOGS, "qemu-lot86-overlay.img"))
import test_qemu_osui_gui as g  # noqa: E402
import test_qemu_lot67_web as lw  # noqa: E402
import test_qemu_gui_term_window as tw  # noqa: E402
from test_qemu_gui_windows import btn, click, BAR_H, W, H  # noqa: E402

ROOT = g.ROOT


def say(msg):
    print("[lot86] %s" % msg, flush=True)


def main():
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
                "-cpu", os.environ.get("LOT86_CPU", "max"),
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
            time.sleep(0.4)

            lw.run(mon, proc, "web-open /web/lot86.html", "osui web", 60, fail=["error="])
            dom = lw.run(mon, proc, "web-dom /web/lot86.html", "web-dom ok", 30)
            if "nodes" not in dom:
                raise RuntimeError("lot86 web-dom: %r" % dom[-300:])
            box = lw.run(mon, proc, "web-inspect /web/lot86.html a", "box ", 30)
            if "box " not in box:
                raise RuntimeError("lot86 #a missing box: %r" % box[-300:])

            scale = lw.run(mon, proc, "settings scale 8", "echelle 8", 20)
            if "moteur" not in scale:
                raise RuntimeError("lot86 settings: %r" % scale[-300:])
            off = lw.run(mon, proc, "settings net off", "reseau off", 20)
            if "fetch refuse" not in off and "off" not in off:
                raise RuntimeError("lot86 net off: %r" % off[-300:])
            lw.run(mon, proc, "settings net on", "reseau on", 20)
            mark = lw.run(mon, proc, "bookmark add https://example.com/lot86 Lot86", "bookmark ok", 20)
            if "example.com/lot86" not in mark:
                raise RuntimeError("lot86 bookmark: %r" % mark[-200:])
            listed = lw.run(mon, proc, "bookmark list", "bookmark count", 20)
            if "example.com/lot86" not in listed:
                raise RuntimeError("lot86 bookmark list: %r" % listed[-200:])

            loaded = lw.run(mon, proc, "web-load /web/lot86form.html https://example.com/search", "forms ", 30)
            if "fields " not in loaded:
                raise RuntimeError("lot86 web-load: %r" % loaded[-300:])
            forms = lw.run(mon, proc, "web-forms", "name=q", 20)
            if "name=user" not in forms:
                raise RuntimeError("lot86 forms: %r" % forms[-400:])
            user = None
            for line in forms.splitlines():
                if "name=user" in line and line.startswith("field "):
                    user = int(line.split()[1])
            if user is None:
                raise RuntimeError("lot86 no user field: %r" % forms[-400:])
            lw.run(mon, proc, "web-input 0 mohhdy", "osui web input ok", 20)
            prev = lw.run(mon, proc, "web-preview 0", "web-preview GET ", 20)
            if "q=mohhdy" not in prev:
                raise RuntimeError("lot86 GET preview: %r" % prev[-300:])
            lw.run(mon, proc, "web-input %d ada" % user, "osui web input ok", 20)
            post = lw.run(mon, proc, "web-preview %d" % user, "web-preview POST ", 20)
            if "user=ada" not in post or "pw=secret" not in post:
                raise RuntimeError("lot86 POST preview: %r" % post[-300:])
            remembered = lw.run(mon, proc, "web-fill remember", "web-fill remembered", 20)
            if "remembered 0" in remembered:
                raise RuntimeError("lot86 remember: %r" % remembered[-200:])
            listed = lw.run(mon, proc, "autofill list", "autofill ", 20)
            if "example.com q" not in listed or "example.com user" not in listed or " pw" in listed:
                raise RuntimeError("lot86 autofill: %r" % listed[-300:])
            lw.run(mon, proc, "web-input 0", "osui web input ok", 20)
            lw.run(mon, proc, "web-fill apply 0", "web-fill applied", 20)
            again = lw.run(mon, proc, "web-preview 0", "q=mohhdy", 20)
            if "q=mohhdy" not in again:
                raise RuntimeError("lot86 reapplied: %r" % again[-300:])
            read = lw.run(mon, proc, "web-read", "Keep", 20)
            if "banner" in read:
                raise RuntimeError("lot86 css hid nothing: %r" % read[-300:])
            css = lw.run(mon, proc, "web-css", "display none", 20)
            if "css nodes" not in css:
                raise RuntimeError("lot86 css: %r" % css[-300:])

            g.send_command_until(mon, "gui", "osui gui live", proc, wait_prompt=False)
            g.send_command_until(mon, "/browse", "osui web", proc, mode="getc", wait_prompt=False)
            time.sleep(1.2)
            full = (8, BAR_H + 8, W - 16, H - BAR_H - 16)
            click(qmp, proc, *btn(full, "min", 30), marker="gfx wm full minimize", label="web minimize")
            docks = g.desktop_hit_centers(W, H)["docks"]
            click(qmp, proc, docks[1][0], docks[1][1], "gfx wm full restore", "dock restore")
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
            g.send_command_until(mon, "pm-ide w86.pm", "osui pm-ide open fullscreen", proc,
                                 mode="getc", wait_prompt=False)
            time.sleep(1.5)
            click(qmp, proc, *btn(full, "min", 30), marker="gfx wm full minimize", label="editor minimize")
            click(qmp, proc, docks[7][0], docks[7][1], "gfx wm full restore", "dock editor restore")
            click(qmp, proc, *btn(full, "max", 30), marker="gfx wm full maximize off", label="editor restore size")
            ed = shrunk
            start = len(g.log_text())
            tw.move(qmp, ed[0] + ed[2] - 3, ed[1] + ed[3] - 3)
            tw.button(qmp, True)
            tw.move(qmp, ed[0] + ed[2] - 83, ed[1] + ed[3] - 63)
            tw.button(qmp, False)
            g.wait_for(proc, "gfx wm full resized", g.CMD_TIMEOUT, start)
            ed2 = (ed[0], ed[1], ed[2] - 80, ed[3] - 60)
            click(qmp, proc, *btn(ed2, "close", 30), marker="gfx wm full close", label="editor close")
            say("lot86 qemu ok in %.1fs" % (time.monotonic() - t0))
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
        print("[lot86] FAIL: %s" % e, flush=True)
        sys.exit(1)
