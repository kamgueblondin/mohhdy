#!/usr/bin/env python3
"""Lot 52: the one guest web engine, console geometry and VBE pixels."""
import os
import subprocess
import sys
import time

HERE = os.path.dirname(os.path.abspath(__file__))
sys.path.insert(0, HERE)
LOGS = os.path.abspath(os.path.join(HERE, "..", "..", "test_logs"))
os.environ.setdefault("OSUI_GUI_LOG", os.path.join(LOGS, "qemu-web-layout-serial.log"))
os.environ.setdefault("OSUI_GUI_ERR", os.path.join(LOGS, "qemu-web-layout-stderr.log"))
os.environ.setdefault("OSUI_GUI_MON_SOCK", os.path.join(LOGS, "qemu-web-layout-mon.sock"))
os.environ.setdefault("OSUI_GUI_QMP_SOCK", os.path.join(LOGS, "qemu-web-layout-qmp.sock"))
os.environ.setdefault("OVERLAY_DISK", os.path.join(LOGS, "qemu-web-layout-overlay.img"))
import test_qemu_osui_gui as g


def require(text, fragment):
    if fragment not in text:
        raise RuntimeError("missing %r in serial output:\n%s" % (fragment, text[-2500:]))


def main():
    os.makedirs(LOGS, exist_ok=True)
    for path in (g.LOG, g.QEMU_ERR, g.MON_SOCK):
        try:
            os.remove(path)
        except FileNotFoundError:
            pass
    g.prepare_test_disk()
    proc = None
    monitor = None
    try:
        with open(g.QEMU_ERR, "w") as err:
            proc = subprocess.Popen([
                "qemu-system-i386", "-cpu", "pentium3", "-kernel", g.KERNEL,
                "-initrd", g.INITRD, "-m", "1024M", "-display", "none", "-vga", "std",
                "-usb", "-device", "usb-tablet", "-serial", "file:" + g.LOG,
                "-monitor", "unix:%s,server,nowait" % g.MON_SOCK,
                "-machine", "type=pc,accel=tcg", "-no-reboot", "-no-shutdown",
            ] + g.qemu_disk_args(), cwd=g.ROOT, stdout=err, stderr=err)
            g.wait_for(proc, "[0m : ", g.BOOT_TIMEOUT)
            monitor = g.monitor_connect()
            start = g.send_command_until(monitor, "web-render /web/lot52.html", "web-render ok", proc)
            text = g.normalized_log(g.log_text()[start:])
            require(text, "web title Lot 52 bounded web layout")
            require(text, "web item rect 0 0 60x12 #00ff0000")
            require(text, "web item rect 0 16 60x12 #000000ff")
            require(text, 'web item text 0 29 8x8 #00000000 "A"')
            require(text, 'web item text 0 39 8x8 #00000000 "C"')
            require(text, "web item rect 0 48 1x32 #00000000")
            require(text, "web item rect 1 48 1x32 #00ffffff")
            if "decode_failed" in text:
                raise RuntimeError("image fell back to alt")
            for line in text.splitlines():
                if line.startswith("osui web render") or line.startswith("web title"):
                    print(line, flush=True)
            for ident, box in (("a", "0 0 60x12"), ("b", "0 16 60x12"),
                               ("cellc", "0 38 50x10"), ("picture", "0 48 48x32"),
                               ("g3", "0 124 48x10")):
                start = g.send_command_until(monitor, "web-inspect /web/lot52.html " + ident,
                                             "inspect ", proc)
                # Wait for the complete inspector line, not just its prefix.
                g.wait_for(proc, " align ", g.CMD_TIMEOUT, start)
                out = g.normalized_log(g.log_text()[start:])
                require(out, " box " + box)
                print("web-layout inspect %s box %s" % (ident, box), flush=True)
            for ident, box in (("a", "0 0 10x50"), ("b", "0 50 10x50"),
                               ("locked", "0 100 10x70"), ("tail", "0 170 10x30"),
                               ("wb", "14 200 10x100")):
                start = g.send_command_until(monitor, "web-inspect /web/lot54.html " + ident,
                                             "inspect ", proc)
                g.wait_for(proc, " align ", g.CMD_TIMEOUT, start)
                require(g.normalized_log(g.log_text()[start:]), " box " + box)
                print("column-sizing inspect %s box %s" % (ident, box), flush=True)
            for ident, box in (("fb", "100 15 20x10"), ("ga", "24 40 96x10"),
                               ("span", "0 112 80x10"), ("intrinsic", "0 142 40x20"),
                               ("image", "0 202 48x16")):
                start = g.send_command_until(monitor, "web-inspect /web/lot55.html " + ident,
                                             "inspect ", proc)
                g.wait_for(proc, " align ", g.CMD_TIMEOUT, start)
                require(g.normalized_log(g.log_text()[start:]), " box " + box)
                print("layout-system inspect %s box %s" % (ident, box), flush=True)
            for ident, box in (("ra", "80 0 20x10"), ("rb", "100 0 20x10"),
                               ("host", "0 10 100x40"), ("abs", "10 18 30x12"),
                               ("c1", "0 50 36x10"), ("c2", "44 50 36x10"),
                               ("note", "0 60 640x20"), ("wa", "0 100 20x10"),
                               ("fl", "0 110 24x16"), ("cleared", "0 126 16x8"),
                               ("lines", "0 134 40x40")):
                start = g.send_command_until(monitor, "web-inspect /web/lot56.html " + ident,
                                             "inspect ", proc)
                g.wait_for(proc, " align ", g.CMD_TIMEOUT, start)
                require(g.normalized_log(g.log_text()[start:]), " box " + box)
                print("lot56 inspect %s box %s" % (ident, box), flush=True)
            for ident, box in (("wrapped", "0 14 56x10"), ("floor", "0 24 80x10"),
                               ("hole", "80 34 40x10"), ("side", "40 64 80x20"),
                               ("span", "0 84 80x10"), ("auto", "0 124 20x10"),
                               ("contain", "0 134 24x24"), ("cover", "0 158 24x24")):
                start = g.send_command_until(monitor, "web-inspect /web/lot57.html " + ident,
                                             "inspect ", proc)
                g.wait_for(proc, " align ", g.CMD_TIMEOUT, start)
                require(g.normalized_log(g.log_text()[start:]), " box " + box)
                print("lot57 inspect %s box %s" % (ident, box), flush=True)
            g.send_command_until(monitor, "gui", "osui gui live", proc, wait_prompt=False)
            g.wait_for(proc, "OSUI-SNAP", g.CMD_TIMEOUT)
            g.send_command_until(monitor, "web /web/lot52.html", "osui web render file=/web/lot52.html", proc,
                                 mode="getc", wait_prompt=False)
            time.sleep(0.5)
            width, height, pixels = g.screendump_rgb(monitor, "qemu-web-layout.ppm")
            # Same page origin as the existing guest page test at 1024x768.
            # The alternating columns are exact, decoded from mire-g1.png.
            ox, oy = 16, 82
            for y in range(32):
                for x in range(48):
                    expected = (255, 255, 255) if x & 1 else (0, 0, 0)
                    offset = ((oy + 48 + y) * width + ox + x) * 3
                    actual = tuple(pixels[offset:offset + 3])
                    if actual != expected:
                        raise RuntimeError("image pixel %d,%d: %r != %r" % (x, y, actual, expected))
            print("web-layout pixels image=48x32 compared=1536 wrap=2 table_rows=2", flush=True)
            step = len(g.log_text())
            g.send_key(monitor, "esc")
            g.wait_for(proc, "osui web close file=/web/lot52.html", g.CMD_TIMEOUT, step)
            start = g.send_command_until(monitor, "web /web/lot53.html",
                "osui web render file=/web/lot53.html", proc, mode="getc", wait_prompt=False)
            time.sleep(0.5)
            width, height, pixels = g.screendump_rgb(monitor, "qemu-flex-sizing.ppm")
            for row, spans in ((0, ((0, 50, (255, 0, 0)), (50, 100, (0, 0, 255)))),
                               (10, ((0, 70, (0, 128, 0)), (70, 100, (255, 255, 0))))):
                for left, right, color in spans:
                    for y in range(row, row + 10):
                        for x in range(left, right):
                            offset = ((oy + y) * width + ox + x) * 3
                            if tuple(pixels[offset:offset + 3]) != color:
                                raise RuntimeError("flex sizing pixel mismatch at %d,%d" % (x, y))
            print("flex-sizing pixels compared=2000 shrink=50,50 frozen=70,30", flush=True)
            step = len(g.log_text())
            g.send_key(monitor, "esc")
            g.wait_for(proc, "osui web close file=/web/lot53.html", g.CMD_TIMEOUT, step)
            start = g.send_command_until(monitor, "web /web/lot54.html",
                "osui web render file=/web/lot54.html", proc, mode="getc", wait_prompt=False)
            time.sleep(0.5)
            width, height, pixels = g.screendump_rgb(monitor, "qemu-column-sizing.ppm")
            compared = 0
            for left, top, right, bottom, color in (
                (0, 0, 10, 50, (255, 0, 0)), (0, 50, 10, 100, (0, 0, 255)),
                (0, 100, 10, 170, (0, 128, 0)), (0, 170, 10, 200, (255, 255, 0)),
                (0, 200, 10, 300, (255, 0, 0)), (14, 200, 24, 300, (0, 0, 255))):
                for y in range(top, bottom):
                    for x in range(left, right):
                        offset = ((oy + y) * width + ox + x) * 3
                        if tuple(pixels[offset:offset + 3]) != color:
                            raise RuntimeError("column sizing pixel mismatch at %d,%d" % (x, y))
                        compared += 1
            require(g.normalized_log(g.log_text()[start:]), "dropped 0 page 640x300 diag 0")
            print("column-sizing pixels compared=%d shrink=50,50 frozen=70,30 wrap_x=14" % compared,
                  flush=True)


            step = len(g.log_text())
            g.send_key(monitor, "esc")
            g.wait_for(proc, "osui web close file=/web/lot54.html", g.CMD_TIMEOUT, step)
            start = g.send_command_until(monitor, "web /web/lot55.html",
                "osui web render file=/web/lot55.html", proc, mode="getc", wait_prompt=False)
            time.sleep(0.5)
            width, height, pixels = g.screendump_rgb(monitor, "qemu-layout-system.ppm")
            compared = 0
            for left, top, right, bottom, color in (
                (0, 15, 20, 25, (255, 0, 0)), (100, 15, 120, 25, (0, 0, 255)),
                (24, 40, 120, 50, (0, 128, 0)), (0, 52, 20, 72, (255, 255, 0)),
                (24, 52, 120, 72, (0, 0, 128)), (3, 75, 23, 92, (0, 0, 255)),
                (0, 112, 80, 122, (0, 128, 0)), (80, 112, 120, 122, (255, 165, 0)),
                (0, 122, 40, 132, (255, 0, 0)), (40, 122, 80, 132, (0, 0, 255)),
                (80, 122, 120, 132, (255, 255, 0)), (0, 132, 20, 142, (0, 0, 128)),
                (20, 218, 40, 228, (0, 0, 255))):
                for y in range(top, bottom):
                    for x in range(left, right):
                        offset = ((oy + y) * width + ox + x) * 3
                        if tuple(pixels[offset:offset + 3]) != color:
                            raise RuntimeError("layout system pixel mismatch at %d,%d" % (x, y))
                        compared += 1
            for y in range(16):
                for x in range(48):
                    expected = (255, 255, 255) if x & 1 else (0, 0, 0)
                    offset = ((oy + 202 + y) * width + ox + x) * 3
                    if tuple(pixels[offset:offset + 3]) != expected:
                        raise RuntimeError("layout system image mismatch at %d,%d" % (x, y))
                    compared += 1
            for x, y in ((20, 132), (30, 142), (0, 218), (19, 227)):
                offset = ((oy + y) * width + ox + x) * 3
                if tuple(pixels[offset:offset + 3]) != (255, 255, 255):
                    raise RuntimeError("clipping/visibility leaked pixels at %d,%d" % (x, y))
                compared += 1
            serial = g.normalized_log(g.log_text()[start:])
            require(serial, "dropped 0 page 640x228 diag 0")
            for line in serial.splitlines():
                if line.startswith("osui web render file=/web/lot55.html"):
                    print(line, flush=True)
            print("layout-system pixels compared=%d flex=packed grid=spanned table=colspan image=decoded clip=bounded" % compared,
                  flush=True)
            step = len(g.log_text())
            g.send_key(monitor, "esc")
            g.wait_for(proc, "osui web close file=/web/lot55.html", g.CMD_TIMEOUT, step)
            start = g.send_command_until(monitor, "web /web/lot56.html",
                "osui web render file=/web/lot56.html", proc, mode="getc", wait_prompt=False)
            time.sleep(0.5)
            width, height, pixels = g.screendump_rgb(monitor, "qemu-lot56.ppm")
            compared = 0
            for left, top, right, bottom, color in (
                (80, 0, 100, 10, (255, 0, 0)), (100, 0, 120, 10, (0, 0, 255)),
                (10, 18, 40, 30, (0, 128, 0)),
                (16, 50, 36, 60, (0, 0, 128)), (52, 50, 80, 60, (255, 165, 0)),
                (0, 100, 20, 110, (255, 0, 255)),
                (0, 110, 24, 126, (0, 255, 255)),
                (0, 126, 16, 134, (255, 255, 0))):
                for y in range(top, bottom):
                    for x in range(left, right):
                        offset = ((oy + y) * width + ox + x) * 3
                        if tuple(pixels[offset:offset + 3]) != color:
                            raise RuntimeError("lot56 pixel mismatch at %d,%d" % (x, y))
                        compared += 1
            serial = g.normalized_log(g.log_text()[start:])
            require(serial, "dropped 0 page 640x174 diag 0")
            print("lot56 pixels compared=%d reverse=ordered absolute=inset table=spaced pre=two-lines wrap=reversed clear=below" % compared,
                  flush=True)

            step = len(g.log_text())
            g.send_key(monitor, "esc")
            g.wait_for(proc, "osui web close file=/web/lot56.html", g.CMD_TIMEOUT, step)
            start = g.send_command_until(monitor, "web /web/lot57.html",
                "osui web render file=/web/lot57.html", proc, mode="getc", wait_prompt=False)
            time.sleep(0.5)
            width, height, pixels = g.screendump_rgb(monitor, "qemu-lot57.ppm")
            compared = 0
            for left, top, right, bottom, color in (
                (0, 0, 56, 10, (255, 0, 0)), (60, 0, 116, 10, (255, 0, 0)),
                (0, 14, 56, 24, (255, 0, 0)), (0, 24, 80, 34, (255, 0, 0)),
                (80, 24, 120, 34, (0, 0, 255)), (0, 34, 80, 44, (0, 128, 0)),
                (0, 44, 80, 54, (0, 0, 128)), (80, 34, 120, 44, (255, 255, 0)),
                (0, 54, 120, 64, (255, 165, 0)), (0, 64, 40, 84, (255, 0, 0)),
                (40, 64, 120, 84, (0, 0, 255)), (0, 84, 120, 104, (0, 128, 0)),
                (0, 124, 20, 134, (0, 0, 255)), (20, 104, 40, 114, (0, 0, 255))):
                for y in range(top, bottom):
                    for x in range(left, right):
                        offset = ((oy + y) * width + ox + x) * 3
                        if tuple(pixels[offset:offset + 3]) != color:
                            raise RuntimeError("lot57 layout pixel mismatch at %d,%d" % (x, y))
                        compared += 1
            for top, rw, rh, dx, dy in ((134, 24, 16, 0, 4), (158, 36, 24, -6, 0)):
                for y in range(24):
                    for x in range(24):
                        expected = (255, 0, 0)
                        if dx <= x < dx + rw and dy <= y < dy + rh:
                            expected = (255, 255, 255) if ((x - dx) * 48 // rw) & 1 else (0, 0, 0)
                        offset = ((oy + top + y) * width + ox + x) * 3
                        if tuple(pixels[offset:offset + 3]) != expected:
                            raise RuntimeError("lot57 image pixel mismatch at %d,%d" % (x, top + y))
                        compared += 1
            serial = g.normalized_log(g.log_text()[start:])
            require(serial, "dropped 0 page 640x182 diag 0")
            for line in serial.splitlines():
                if line.startswith("osui web render file=/web/lot57.html"):
                    print(line, flush=True)
            print("lot57 pixels compared=%d grid=minmax,dense,areas flex=wrap,cross-auto table=colspan image=contain,cover cascade=important" % compared,
                  flush=True)

        print("QEMU web layout contract passed.", flush=True)
        return 0
    finally:
        if monitor is not None:
            monitor.close()
        g.terminate(proc)
        for path in (g.MON_SOCK,):
            try:
                os.remove(path)
            except FileNotFoundError:
                pass


if __name__ == "__main__":
    sys.exit(main())
