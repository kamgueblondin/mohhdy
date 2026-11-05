#!/usr/bin/env python3
"""Lot 73: animated GIF in the browser and the new layout, in QEMU,
offline and deterministic.

The Lot 67 test server (scripts/lot67_web_server.py) serves
/lot73/anim.html: a 32x24 GIF of three frames (red, green, blue, 50 cs
each, looping forever) and a two-frame GIF played twice. The image bytes
are the unit test fixtures (tests/unit/userspace/imgdec_anim_samples.h).
Console part: web-images on, web-open of the page: web-images reports
2 animated images and 5 frames; web-anim lists both animations (frames,
loop forever / 1) and, a few seconds later, more frames shown and the
looping image on more plays (eight web-anim ticks 0.6 s apart: one
frame change at most per tick after a pause), the loop-1 image stopped.
GUI part: the same page in the web window; screenshots are taken over
about 3 s and the pure colours of the image area (red, green, blue)
is counted per shot: red, green and blue are all seen (green and blue
are frames 2 and 3 of the looping image), so the frames are drawn on the screen without a new page load.
Picture part: /lot73/pic.html, a picture whose gif source wins over an
avif one and an img whose 1x srcset candidate wins, plus a WebP lossless
image: the server sees anim3.gif, twice.gif and pal4.webp, never
never.gif, and web-images shows 3. HTTP cache part: fetch twice a
max-age=300 answer (the second is "fetch cache fresh", one GET on the
server) and an ETag + no-cache answer (the second is "fetch cache
revalidated" with the body, the server answers 200 then 304).
Layout part: web-render of the local page /web/lot73.html prints the
clamped paragraph (2 lines, the second ending with "...", the last word
gone), the inside list marker left of its text in the content box, the
small-caps text in capitals, EDGE 40 px after "pre " (inline border
and padding take room), the form controls (input value, the chosen
option only, a checked box, a closed details with only its summary)
and the paragraph cut between two columns
(the END word in the second column). Screenshots (PNG, when Pillow is
there) go to LOT73_SHOTS (default test_logs/lot73-shots).
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
os.environ.setdefault("OSUI_GUI_LOG", os.path.join(LOGS, "qemu-lot73-serial.log"))
os.environ.setdefault("OSUI_GUI_MON_SOCK", "/tmp/mohhdy-lot73-mon.sock")
os.environ.setdefault("OSUI_GUI_QMP_SOCK", "/tmp/mohhdy-lot73-qmp.sock")
os.environ.setdefault("OVERLAY_DISK", os.path.join(LOGS, "qemu-lot73-overlay.img"))
import test_qemu_osui_gui as g  # noqa: E402
import test_qemu_lot67_web as lw  # noqa: E402
from test_qemu_gui_windows import W, H, BAR_H  # noqa: E402

ROOT = g.ROOT
SHOTS = os.environ.get("LOT73_SHOTS", os.path.join(g.LOG_DIR, "lot73-shots"))


def say(msg):
    print("[lot73] %s" % msg, flush=True)


def shot(mon, name):
    w, h, rgb = g.screendump_rgb(mon, "qemu-lot73-%s.ppm" % name)
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
    return open(os.path.join(g.LOG_DIR, "lot73-web-server.log")).read()


def wait_server(needle, timeout=60):
    deadline = time.monotonic() + timeout
    while time.monotonic() < deadline:
        if needle in server_text():
            return
        time.sleep(0.3)
    raise RuntimeError("server never saw %r; tail:\n%s" % (needle, server_text()[-1500:]))


def colours(rgb, x0, y0, x1, y1):
    """pixels of pure red, green and blue (the frames), never UI chrome:
    one channel high, the two others low (the fixture colours are
    (220, 20, 30), (10, 200, 40) and (20, 40, 230))"""
    n = {"red": 0, "green": 0, "blue": 0}
    for y in range(y0, y1):
        for x in range(x0, x1):
            i = (y * W + x) * 3
            r, gg, b = rgb[i], rgb[i + 1], rgb[i + 2]
            if r > 180 and gg < 80 and b < 80:
                n["red"] += 1
            elif gg > 180 and r < 80 and b < 80:
                n["green"] += 1
            elif b > 180 and r < 80 and gg < 80:
                n["blue"] += 1
    return n


def anim_lines(out):
    return [l for l in out.splitlines() if l.startswith("anim slot ")]


def changed(out):
    m = re.search(r"web-anim changed (-?\d+) frames_shown (\d+)", out)
    if not m:
        raise RuntimeError("web-anim: no summary in %r" % out[-800:])
    return int(m.group(1)), int(m.group(2))


def items(out):
    found = []
    for m in re.finditer(r'web item text (-?\d+) (-?\d+) (\d+)x(\d+) #[0-9a-fA-F]+(?: bold)? "([^"]*)"', out):
        found.append((int(m.group(1)), int(m.group(2)), m.group(5)))
    return found


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
    srv_log = open(os.path.join(g.LOG_DIR, "lot73-web-server.log"), "w")
    srv = subprocess.Popen([sys.executable, os.path.join(ROOT, "scripts", "lot67_web_server.py"), lw.PKI, str(hp), str(hp + 1)],
                           stdout=srv_log, stderr=subprocess.STDOUT)
    proc = mon = qmp = None
    t0 = time.monotonic()
    try:
        g.prepare_test_disk()
        with open(os.devnull, "wb") as err:
            proc = subprocess.Popen([
                "qemu-system-i386", "-fw_cfg", "name=opt/mohhdy/ui-scale,string=4", "-cpu", os.environ.get("LOT73_CPU", "max"),
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
            # ---- layout: the local page, console
            out = lw.run(mon, proc, "web-render /web/lot73.html", "web-render ok", 60, fail=["error"])
            out = out[out.rfind("web-render /web/lot73.html"):] if "web-render /web/lot73.html" in out else out
            found = items(out)
            texts = [t for (_, _, t) in found]
            say("layout items: %r" % texts)
            clamp = [it for it in found if it[2].startswith("clamp one") or it[2].endswith("...")]
            if len(clamp) != 2 or not clamp[1][2].endswith("...") or clamp[1][1] <= clamp[0][1]:
                raise RuntimeError("line clamp: want 2 lines, the second with ..., got %r" % clamp)
            if any("LASTWORD" in t or "twelve" in t for t in texts):
                raise RuntimeError("line clamp: a clamped line is still painted: %r" % texts)
            marker = [it for it in found if it[2] == "#"]
            text = [it for it in found if it[2] == "inside marker"]
            if not marker or not text or marker[0][1] != text[0][1] or text[0][0] - marker[0][0] != 16:
                raise RuntimeError("inside marker: want '#' 16 px left of its text on its line: %r %r" % (marker, text))
            if "SMALL CAPS" not in texts:
                raise RuntimeError("small-caps: want SMALL CAPS in %r" % texts)
            first = [it for it in found if it[2].startswith("c1 ")]
            end = [it for it in found if it[2].endswith("END")]
            if not first or not end or end[0][0] <= first[0][0] + 100:
                raise RuntimeError("columns: want END in the second column: %r %r" % (first, end))
            pre = [it for it in found if it[2] == "pre"]
            edge = [it for it in found if it[2] == "EDGE"]
            if not pre or not edge or edge[0][0] - pre[0][0] != 24 + 8 + 2 + 6:
                raise RuntimeError("inline edges: want EDGE 40 px right of pre (space, border 2, padding 6): %r %r" % (pre, edge))
            for want in ("FORMVAL", "CHOSEN", "x", "SUMMARY"):
                if want not in texts:
                    raise RuntimeError("forms: want %r in %r" % (want, texts))
            for gone in ("NOTSHOWN", "HIDDENTXT"):
                if gone in texts:
                    raise RuntimeError("forms: %r must not be painted: %r" % (gone, texts))
            say("layout: 2 clamped lines with ..., inside marker, SMALL CAPS, inline edges, form controls, closed details, paragraph cut between columns")
            # ---- console: the animated page
            lw.run(mon, proc, "web-images on", "web-images on", 30)
            lw.run(mon, proc, "web-open %s/lot73/anim.html" % u, "osui web page sections", fail=["error"])
            for name in ("anim3.gif", "twice.gif"):
                wait_server("IMG %s" % name)
            out = lw.run(mon, proc, "web-images", "web-images on found", 30)
            line = out[out.rfind("web-images on found"):].splitlines()[0]
            say(line)
            for want in ("found 2 ", "shown 2 ", "animated 2 ", "frames 5 "):
                if want not in line:
                    raise RuntimeError("web-images: want %r in %r" % (want, line))
            out = lw.run(mon, proc, "web-anim", "web-anim changed", 30)
            time.sleep(0.5)
            out = g.log_text()
            out = out[out.rfind("web-anim changed"):]
            c0, shown0 = changed(out)
            lines = anim_lines(out)
            say("web-anim: %r" % lines)
            if len(lines) != 2 or "frames 3 " not in lines[0] or "loop forever" not in lines[0] \
                    or "frames 2 " not in lines[1] or "loop 1" not in lines[1]:
                raise RuntimeError("web-anim lines: %r" % lines)
            p0 = int(re.search(r" plays (\d+)", lines[0]).group(1))
            # one frame change at most per tick after a pause (no burst):
            # eight ticks 0.6 s apart pass more than one red-green-blue round
            for _ in range(8):
                time.sleep(0.6)
                lw.run(mon, proc, "web-anim", "web-anim changed", 30)
            time.sleep(0.5)
            out = g.log_text()
            out = out[out.rfind("web-anim changed"):]
            c1, shown1 = changed(out)
            lines = anim_lines(out)
            say("web-anim later: changed %d shown %d %r" % (c1, shown1, lines))
            p1 = int(re.search(r" plays (\d+)", lines[0]).group(1))
            if shown1 <= shown0 or p1 <= p0:
                raise RuntimeError("frames did not advance: shown %d -> %d plays %d -> %d" % (shown0, shown1, p0, p1))
            if " stopped 1" not in lines[1]:
                raise RuntimeError("the twice-played GIF is not stopped after 8 ticks: %r" % lines[1])
            say("console: 2 animations, frames advance, the looping one plays on, the loop-1 one stops")
            # ---- picture / srcset choice and a WebP lossless image
            lw.run(mon, proc, "web-open %s/lot73/pic.html" % u, "osui web page sections", fail=["error"])
            for name in ("anim3.gif", "twice.gif", "pal4.webp"):
                wait_server("IMG %s" % name)
            out = lw.run(mon, proc, "web-images", "web-images on found", 30)
            line = out[out.rfind("web-images on found"):].splitlines()[0]
            say(line)
            for want in ("found 3 ", "shown 3 "):
                if want not in line:
                    raise RuntimeError("picture page: want %r in %r" % (want, line))
            if "never.gif" in server_text():
                raise RuntimeError("picture / srcset: a candidate that must lose was fetched")
            say("picture: the gif source and the 1x candidate fetched, never.gif never; WebP shown")
            # ---- HTTP cache: max-age served without a request, ETag revalidated with 304
            lw.run(mon, proc, "http-cache clear", "http-cache on", 30)
            lw.run(mon, proc, "fetch %s/lot73/fresh.txt" % u, "fetch ok", 60, fail=["fetch error"])
            out = lw.run(mon, proc, "fetch %s/lot73/fresh.txt" % u, "fetch ok", 60, fail=["fetch error"])
            if "fetch cache fresh" not in out or server_text().count("FRESH GET") != 1:
                raise RuntimeError("http cache: the max-age answer was asked again: %r" % out[-600:])
            lw.run(mon, proc, "fetch %s/lot73/etag.txt" % u, "fetch ok", 60, fail=["fetch error"])
            out = lw.run(mon, proc, "fetch %s/lot73/etag.txt" % u, "fetch ok", 60, fail=["fetch error"])
            if "fetch cache revalidated" not in out or "etag body lot73" not in out:
                raise RuntimeError("http cache: no revalidation: %r" % out[-600:])
            if server_text().count("ETAG 200") != 1 or server_text().count("ETAG 304") != 1:
                raise RuntimeError("http cache: want one 200 then one 304 for the ETag")
            out = lw.run(mon, proc, "http-cache", "http-cache on", 30)
            say(out[out.rfind("http-cache on"):].splitlines()[0])
            say("http cache: fresh hit without a request, ETag 304 revalidation")
            # ---- GUI: frames drawn on the screen over time
            g.send_command_until(mon, "gui", "osui gui live", proc, wait_prompt=False)
            g.wait_for(proc, "OSUI-SNAP", g.CMD_TIMEOUT)
            time.sleep(1.0)
            start = len(g.log_text())
            lw.type_line(mon, proc, "web %s/lot73/anim.html" % u, mode="getc")
            g.wait_for(proc, "osui web fetch ok status 200", lw.FETCH_TIMEOUT, start)
            time.sleep(2.0)
            seen = set()
            for k in range(8):
                rgb = shot(mon, "anim-%d" % k)
                n = colours(rgb, 8, BAR_H + 40, W - 8, H - 40)
                say("shot %d colours %r" % (k, n))
                for name in n:
                    if n[name] >= 200:
                        seen.add(name)
                if len(seen) >= 3 and k >= 2:
                    break
                time.sleep(0.4)
            say("dominant frame colours seen: %r" % sorted(seen))
            if len(seen) < 3:
                raise RuntimeError("animation not drawn: colours seen %r" % sorted(seen))
            g.send_key(mon, "esc")
            time.sleep(1.0)
        say("QEMU Lot 73 contract passed in %.0f s." % (time.monotonic() - t0))
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
        print("QEMU Lot 73 failed: %s" % error, file=sys.stderr)
        raise SystemExit(1)
