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
            start = g.send_command_until(monitor, "web-render /web/lot58.html", "web-render ok", proc)
            serial = g.normalized_log(g.log_text()[start:])
            require(serial, "web title Lot 58 typed CSS sizing")
            for label, box in (("math", "35 0 50x25"), ("limits", "20 49 80x20"), ("grid-auto", "10 94 40x20")):
                require(serial, "web item rect " + box)
                print("lot58 console %s box %s" % (label, box), flush=True)
            require(serial, "dropped 0 page 640x203 diag 0")
            start = g.send_command_until(monitor, "web-render /web/lot59.html", "web-render ok", proc)
            serial = g.normalized_log(g.log_text()[start:])
            require(serial, "web title Lot 59 intrinsic layout")
            for label, box in (("fit", "0 0 40x20"), ("flex", "60 20 56x10"),
                               ("relative", "42 79 20x10"), ("table", "0 104 120x10")):
                require(serial, "web item rect " + box)
                print("lot59 console %s box %s" % (label, box), flush=True)
            require(serial, "dropped 0 page 640x146 diag 0")
            start = g.send_command_until(monitor, "web-render /web/lot60.html", "web-render ok", proc)
            serial = g.normalized_log(g.log_text()[start:])
            require(serial, "web title Lot 60 baseline and named grid lines")
            for label, box in (("flex-baseline", "50 6 50x10"), ("grid-baseline", "40 20 40x20"),
                               ("named-span", "0 40 120x10"), ("negative", "80 50 40x10")):
                require(serial, "web item rect " + box)
                print("lot60 console %s box %s" % (label, box), flush=True)
            require(serial, "dropped 0 page 640x91 diag 0")
            start = g.send_command_until(monitor, "web-render /web/lot61.html", "web-render ok", proc)
            serial = g.normalized_log(g.log_text()[start:])
            require(serial, "web title Lot 61 baselines subgrid viewport")
            for label, box in (("flex-last-baseline", "0 10 50x10"), ("subgrid-span", "40 24 80x10"),
                               ("inline-block-baseline", "16 34 20x16"), ("glyph-fragment", "9 69 3x2"),
                               ("fixed", "610 470 30x10"), ("collapse-outer-edge", "0 95 120x4"),
                               ("collapse-inner-edge", "59 95 2x14"), ("subgrid-own-gap", "45 109 30x6"),
                               ("negative-z", "0 115 120x6"), ("dashed-piece", "8 124 4x2"),
                               ("double-line", "0 132 10x1")):
                require(serial, "web item rect " + box)
                print("lot61 console %s box %s" % (label, box), flush=True)
            require(serial, "dropped 0 page 640x144 diag 0")
            start = g.send_command_until(monitor, "web-render /web/lot62.html", "web-render ok", proc)
            serial = g.normalized_log(g.log_text()[start:])
            require(serial, "web title Lot 62 alpha transform paint order")
            for label, box in (("alpha-composite", "0 0 60x10"), ("ridge-light", "0 10 23x1"),
                               ("ridge-dark", "1 11 21x1"), ("transform", "25 18 20x10"),
                               ("box-shadow", "4 44 20x2"), ("underline", "0 53 16x1"),
                               ("percent-margin", "30 54 90x4"), ("float", "0 58 20x10"),
                               ("gradient-band", "0 68 120x1"), ("radius-step", "3 76 14x2"),
                               ("round-clip-corner", "5 108 10x3"), ("round-clip-middle", "0 114 20x8"),
                               ("radial-ring", "10 171 10x4"), ("grid-area-percent", "48 188 72x4")):
                require(serial, "web item rect " + box)
                print("lot62 console %s box %s" % (label, box), flush=True)
            for label, item in (("justify", 'text 28 129 16x8 #00000000 "bb"'), ("letter-spacing", 'text 10 149 8x8 #00000000 "B"')):
                require(serial, "web item " + item)
                print("lot62 console %s %s" % (label, item), flush=True)
            require(serial, "dropped 0 page 640x192 diag 2")
            start = g.send_command_until(monitor, "web-render /web/lot63.html", "web-render ok", proc)
            serial = g.normalized_log(g.log_text()[start:])
            require(serial, "web title Lot 63 generated content transforms")
            for label, box in (("before-block", "0 0 40x4"), ("radial-at", "0 50 20x8"),
                               ("rotate-band", "22 70 14x3"), ("skew-row", "6 95 20x1"),
                               ("dashed-rounded", "6 98 4x2"), ("dashed-arc", "3 98 3x2"),
                               ("round-clip-transform", "50 116 20x8")):
                require(serial, "web item rect " + box)
                print("lot63 console %s box %s" % (label, box), flush=True)
            for label, item in (("counter", 'text 0 15 24x8 #00000000 "VI."'), ("counter-step", 'text 0 25 40x8 #00000000 "VIII."'),
                                ("quote", 'text 0 35 8x8 #00000000 "<"'), ("char-ref", 'text 0 5 16x8 #00000000 "-x"'),
                                ("overflow-x-clip", 'text 0 131 16x8 #00000000 "ab"')):
                require(serial, "web item " + item)
                print("lot63 console %s %s" % (label, item), flush=True)
            require(serial, "dropped 0 page 640x140 diag 2")
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

            step = len(g.log_text())
            g.send_key(monitor, "esc")
            g.wait_for(proc, "osui web close file=/web/lot57.html", g.CMD_TIMEOUT, step)
            start = g.send_command_until(monitor, "web /web/lot58.html",
                "osui web render file=/web/lot58.html", proc, mode="getc", wait_prompt=False)
            time.sleep(0.5)
            width, height, pixels = g.screendump_rgb(monitor, "qemu-lot58.ppm")
            compared = 0
            for left, top, right, bottom, color in (
                (35, 0, 85, 25, (255, 0, 0)), (0, 25, 58, 35, (0, 128, 0)),
                (62, 25, 120, 35, (0, 0, 255)), (0, 39, 58, 49, (0, 128, 0)),
                (20, 49, 100, 69, (0, 0, 255)), (10, 94, 50, 114, (0, 128, 0)),
                (80, 89, 100, 99, (255, 165, 0)), (60, 134, 110, 144, (128, 0, 128)),
                (0, 159, 120, 179, (0, 128, 0))):
                for y in range(top, bottom):
                    for x in range(left, right):
                        offset = ((oy + y) * width + ox + x) * 3
                        if tuple(pixels[offset:offset + 3]) != color:
                            raise RuntimeError("lot58 layout pixel mismatch at %d,%d" % (x, y))
                        compared += 1
            for y in range(24):
                for x in range(24):
                    expected = (255, 255, 255) if ((x + 6) * 48 // 36) & 1 else (0, 0, 0)
                    offset = ((oy + 179 + y) * width + ox + x) * 3
                    if tuple(pixels[offset:offset + 3]) != expected:
                        raise RuntimeError("lot58 decoded image mismatch at %d,%d" % (x, y))
                    compared += 1
            serial = g.normalized_log(g.log_text()[start:])
            require(serial, "dropped 0 page 640x203 diag 0")
            for line in serial.splitlines():
                if line.startswith("osui web render file=/web/lot58.html"):
                    print(line, flush=True)
            print("lot58 pixels compared=%d math=typed ratio=bounded logical=ltr auto=block,grid flex=wrap table=colspan image=shared" % compared,
                  flush=True)

            step = len(g.log_text())
            g.send_key(monitor, "esc")
            g.wait_for(proc, "osui web close file=/web/lot58.html", g.CMD_TIMEOUT, step)
            start = g.send_command_until(monitor, "web /web/lot59.html",
                "osui web render file=/web/lot59.html", proc, mode="getc", wait_prompt=False)
            time.sleep(0.5)
            width, height, pixels = g.screendump_rgb(monitor, "qemu-lot59.ppm")
            compared = 0
            for left, top, right, bottom, color in (
                (0, 0, 40, 20, (255, 0, 0)), (0, 20, 56, 30, (0, 128, 0)),
                (60, 20, 116, 30, (0, 0, 255)), (0, 34, 56, 44, (0, 128, 0)),
                (0, 44, 32, 64, (0, 128, 0)), (60, 44, 120, 64, (0, 0, 255)),
                (42, 79, 62, 89, (255, 165, 0)), (0, 104, 120, 114, (0, 128, 0))):
                for y in range(top, bottom):
                    for x in range(left, right):
                        offset = ((oy + y) * width + ox + x) * 3
                        if tuple(pixels[offset:offset + 3]) != color:
                            raise RuntimeError("lot59 layout pixel mismatch at %d,%d" % (x, y))
                        compared += 1
            for y in range(32):
                for x in range(48):
                    expected = (255, 255, 255) if x & 1 else (0, 0, 0)
                    offset = ((oy + 114 + y) * width + ox + x) * 3
                    if tuple(pixels[offset:offset + 3]) != expected:
                        raise RuntimeError("lot59 decoded image mismatch at %d,%d" % (x, y))
                    compared += 1
            serial = g.normalized_log(g.log_text()[start:])
            require(serial, "dropped 0 page 640x146 diag 0")
            for line in serial.splitlines():
                if line.startswith("osui web render file=/web/lot59.html"):
                    print(line, flush=True)
            print("lot59 pixels compared=%d intrinsic=limits,fit,basis flex=wrap relative=logical table=colspan image=shared" % compared,
                  flush=True)

            step = len(g.log_text())
            g.send_key(monitor, "esc")
            g.wait_for(proc, "osui web close file=/web/lot59.html", g.CMD_TIMEOUT, step)
            start = g.send_command_until(monitor, "web /web/lot60.html",
                "osui web render file=/web/lot60.html", proc, mode="getc", wait_prompt=False)
            time.sleep(0.5)
            width, height, pixels = g.screendump_rgb(monitor, "qemu-lot60.ppm")
            compared = 0
            for left, top, right, bottom, color in (
                (0, 0, 50, 16, (255, 0, 0)), (50, 6, 100, 16, (0, 0, 255)),
                (0, 16, 40, 30, (0, 128, 0)), (40, 20, 80, 40, (0, 0, 255)),
                (0, 40, 120, 50, (255, 165, 0)), (80, 50, 120, 60, (255, 0, 0)),
                (0, 60, 120, 70, (0, 128, 0))):
                for y in range(top, bottom):
                    for x in range(left, right):
                        offset = ((oy + y) * width + ox + x) * 3
                        if tuple(pixels[offset:offset + 3]) != color:
                            raise RuntimeError("lot60 layout pixel mismatch at %d,%d" % (x, y))
                        compared += 1
            for y in range(21):
                for x in range(32):
                    expected = (255, 255, 255) if (x * 48 // 32) & 1 else (0, 0, 0)
                    offset = ((oy + 70 + y) * width + ox + x) * 3
                    if tuple(pixels[offset:offset + 3]) != expected:
                        raise RuntimeError("lot60 decoded image mismatch at %d,%d" % (x, y))
                    compared += 1
            serial = g.normalized_log(g.log_text()[start:])
            require(serial, "dropped 0 page 640x91 diag 0")
            for line in serial.splitlines():
                if line.startswith("osui web render file=/web/lot60.html"):
                    print(line, flush=True)
            print("lot60 pixels compared=%d baseline=flex,grid names=aliases,span negative=explicit flex=wrap table=colspan image=shared" % compared,
                  flush=True)

            step = len(g.log_text())
            g.send_key(monitor, "esc")
            g.wait_for(proc, "osui web close file=/web/lot60.html", g.CMD_TIMEOUT, step)
            start = g.send_command_until(monitor, "web /web/lot61.html",
                "osui web render file=/web/lot61.html", proc, mode="getc", wait_prompt=False)
            time.sleep(0.5)
            width, height, pixels = g.screendump_rgb(monitor, "qemu-lot61.ppm")
            compared = 0
            for left, top, right, bottom, color in (
                (0, 10, 50, 20, (255, 0, 0)), (50, 0, 100, 24, (0, 0, 255)),
                (0, 24, 40, 34, (255, 165, 0)), (40, 24, 120, 34, (0, 128, 0)),
                (16, 34, 36, 50, (0, 0, 255)), (0, 54, 120, 56, (255, 0, 0)),
                (610, 470, 640, 480, (128, 0, 128)), (0, 95, 59, 99, (0, 0, 255)),
                (61, 95, 120, 99, (0, 0, 255)), (0, 105, 59, 109, (0, 0, 255)),
                (61, 105, 120, 109, (0, 0, 255)), (0, 99, 4, 105, (0, 0, 255)),
                (116, 99, 120, 105, (0, 0, 255)), (59, 95, 61, 109, (255, 0, 0)),
                (0, 109, 35, 115, (0, 128, 0)), (45, 109, 75, 115, (255, 165, 0)),
                (0, 115, 120, 118, (0, 255, 0)), (0, 118, 120, 124, (0, 0, 128)),
                (0, 124, 4, 126, (255, 0, 255)), (4, 124, 8, 126, (255, 255, 255)),
                (8, 124, 12, 126, (255, 0, 255)), (0, 130, 10, 131, (0, 128, 128)),
                (0, 131, 10, 132, (255, 255, 255)), (0, 132, 10, 133, (0, 128, 128)),
                (0, 140, 120, 144, (255, 255, 255))):
                for y in range(top, bottom):
                    for x in range(left, right):
                        offset = ((oy + y) * width + ox + x) * 3
                        if tuple(pixels[offset:offset + 3]) != color:
                            raise RuntimeError("lot61 layout pixel mismatch at %d,%d" % (x, y))
                        compared += 1
            inside = 0
            for y in range(65, 73):
                for x in range(0, 24):
                    offset = ((oy + y) * width + ox + x) * 3
                    black = tuple(pixels[offset:offset + 3]) == (0, 0, 0)
                    if x >= 12 and black:
                        raise RuntimeError("lot61 glyph painted past the clip at %d,%d" % (x, y))
                    if x >= 8 and x < 12 and black:
                        inside += 1
                    compared += 1
            if inside == 0:
                raise RuntimeError("lot61 partial glyph fragment missing")
            for y in range(21):
                for x in range(32):
                    expected = (255, 255, 255) if (x * 48 // 32) & 1 else (0, 0, 0)
                    offset = ((oy + 74 + y) * width + ox + x) * 3
                    if tuple(pixels[offset:offset + 3]) != expected:
                        raise RuntimeError("lot61 decoded image mismatch at %d,%d" % (x, y))
                    compared += 1
            serial = g.normalized_log(g.log_text()[start:])
            require(serial, "dropped 0 page 640x144 diag 0")
            for line in serial.splitlines():
                if line.startswith("osui web render file=/web/lot61.html"):
                    print(line, flush=True)
            print("lot61 pixels compared=%d baseline=last,inline-block subgrid=auto-fill fixed=viewport clip=glyph table=collapse,outer-edge,half-inset border=dashed,double opacity=0 subgrid-gap=own z=negative flex=wrap image=shared" % compared,
                  flush=True)

            step = len(g.log_text())
            g.send_key(monitor, "esc")
            g.wait_for(proc, "osui web close file=/web/lot61.html", g.CMD_TIMEOUT, step)
            start = g.send_command_until(monitor, "web /web/lot62.html",
                "osui web render file=/web/lot62.html", proc, mode="getc", wait_prompt=False)
            time.sleep(0.5)
            width, height, pixels = g.screendump_rgb(monitor, "qemu-lot62.ppm")
            compared = 0
            for left, top, right, bottom, color in (
                (0, 0, 60, 10, (128, 0, 127)), (60, 0, 120, 10, (0, 0, 255)),
                (0, 10, 23, 11, (0x75, 0xB5, 0xF5)), (1, 11, 21, 12, (0x20, 0x60, 0xA0)),
                (25, 18, 45, 28, (0, 128, 0)), (0, 28, 24, 30, (255, 0, 255)),
                (2, 30, 22, 36, (255, 255, 0)), (0, 38, 20, 44, (0, 0, 128)),
                (16, 44, 24, 46, (128, 128, 128)), (0, 53, 16, 54, (255, 0, 0)),
                (30, 54, 120, 58, (0, 255, 255)), (18, 58, 20, 68, (255, 165, 0)),
                (0, 68, 120, 69, (16, 16, 16)), (0, 75, 120, 76, (0xEF, 0xEF, 0xEF)),
                (3, 76, 17, 78, (128, 0, 0)), (0, 76, 3, 78, (255, 255, 255)),
                (0, 80, 20, 84, (128, 0, 0)), (1, 84, 19, 86, (128, 0, 0)),
                (5, 108, 15, 111, (255, 0, 0)), (0, 108, 5, 111, (255, 255, 255)),
                (0, 114, 20, 122, (255, 0, 0)), (16, 129, 28, 137, (255, 255, 255)),
                (8, 149, 10, 157, (255, 255, 255)), (0, 158, 2, 160, (0, 0, 255)),
                (10, 171, 20, 175, (0xD5, 0, 0x2A)), (5, 169, 10, 171, (128, 0, 128)),
                (48, 188, 120, 192, (0, 255, 0)), (40, 188, 48, 192, (255, 255, 255))):
                for y in range(top, bottom):
                    for x in range(left, right):
                        offset = ((oy + y) * width + ox + x) * 3
                        if tuple(pixels[offset:offset + 3]) != color:
                            raise RuntimeError("lot62 layout pixel mismatch at %d,%d" % (x, y))
                        compared += 1
            ink = 0
            for y in range(59, 68):
                for x in range(0, 16):
                    offset = ((oy + y) * width + ox + x) * 3
                    if tuple(pixels[offset:offset + 3]) == (0, 0, 0):
                        ink += 1
                    compared += 1
            if ink == 0:
                raise RuntimeError("lot62 text not painted over the later float")
            serial = g.normalized_log(g.log_text()[start:])
            require(serial, "dropped 0 page 640x192 diag 2")
            for line in serial.splitlines():
                if line.startswith("osui web render file=/web/lot62.html"):
                    print(line, flush=True)
            print("lot62 pixels compared=%d alpha=source-over border=ridge transform=translate,scale outline=after shadow=offset decoration=underline margin=percent order=text-over-float gradient=bands radius=steps clip=rounded justify=spaces letter-spacing=cells radial=rings grid-percent=area" % compared,
                  flush=True)

            step = len(g.log_text())
            g.send_key(monitor, "esc")
            g.wait_for(proc, "osui web close file=/web/lot62.html", g.CMD_TIMEOUT, step)
            start = g.send_command_until(monitor, "web /web/lot63.html",
                "osui web render file=/web/lot63.html", proc, mode="getc", wait_prompt=False)
            time.sleep(0.5)
            width, height, pixels = g.screendump_rgb(monitor, "qemu-lot63.ppm")
            compared = 0
            for left, top, right, bottom, color in (
                (0, 0, 40, 4, (255, 0, 0)), (40, 0, 120, 4, (255, 255, 255)),
                (0, 50, 4, 58, (0x2B, 0, 0xD4)), (22, 70, 36, 73, (0, 128, 0)),
                (6, 95, 26, 96, (128, 0, 128)), (6, 98, 10, 100, (0, 0, 128)),
                (10, 98, 14, 100, (255, 255, 255)), (0, 98, 2, 100, (255, 255, 255)),
                (3, 98, 6, 100, (0, 0, 128)), (50, 116, 70, 124, (255, 165, 0)),
                (50, 110, 55, 113, (255, 255, 255)), (0, 110, 20, 130, (255, 255, 255))):
                for y in range(top, bottom):
                    for x in range(left, right):
                        offset = ((oy + y) * width + ox + x) * 3
                        if tuple(pixels[offset:offset + 3]) != color:
                            raise RuntimeError("lot63 layout pixel mismatch at %d,%d" % (x, y))
                        compared += 1
            ink = 0
            for y in range(131, 139):
                for x in range(0, 24):
                    offset = ((oy + y) * width + ox + x) * 3
                    black = tuple(pixels[offset:offset + 3]) == (0, 0, 0)
                    if x >= 16 and black:
                        raise RuntimeError("lot63 text painted past overflow-x clip at %d,%d" % (x, y))
                    if black:
                        ink += 1
                    compared += 1
            if ink == 0:
                raise RuntimeError("lot63 clipped text missing")
            serial = g.normalized_log(g.log_text()[start:])
            require(serial, "dropped 0 page 640x140 diag 2")
            for line in serial.splitlines():
                if line.startswith("osui web render file=/web/lot63.html"):
                    print(line, flush=True)
            print("lot63 pixels compared=%d content=before-block counters=step quotes=list radial=at rotate=bands skew=bands border=dashed-rounded clip=rounded-transform overflow=x-clip" % compared,
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
