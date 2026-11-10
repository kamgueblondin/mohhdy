#!/usr/bin/env python3
"""Lot 88: the window chrome of every desktop window type, through the USB
tablet, at any ui-scale. Imported by test_qemu_lot88.py (one boot at 1x and
one at 2x).

The clicks are not placed from a copy of the layout: the guest prints
"gfx wm hit WHO x= y= w= h= title= btn= q=" for the window a click lands on
(kernel/gfx_desktop.c), and the red close button is found on a screendump.
For the chat, the Statut pane, the Fichiers app and the full screen
Browser-OS window:
  * the close button on screen is where the guest's hit test puts it, and
    the button side follows the chrome scale (gfx_wm_btn);
  * maximize toggles and comes back to the same rect;
  * the grip grows the window past the screen (clamped), then a 100 x 60
    shrink is a 100 x 60 shrink on screen (Lot 88 fix in gfx_wm.c: the
    resize starts from the size shown), then back to its size;
  * minimize hides it and the dock restores it (1x boot, where the dock
    centres are known);
  * close closes it.
"""
from __future__ import print_function

import re
import time

import test_qemu_osui_gui as g
import test_qemu_gui_term_window as tw

W, H = 1024, 768
RED = (0xE0, 0x52, 0x4B)
PAD = 2
BTN = 14
HIT_RE = re.compile(r"gfx wm hit (\w+)(?: slot=(\d+))? x=(-?\d+) y=(-?\d+) w=(\d+) h=(\d+) "
                    r"title=(\d+) btn=(\d+) q=(\d+)")
CLICK_TIMEOUT = 20.0


def say(msg):
    print("[lot88-chrome] %s" % msg, flush=True)


def py_btn(title_h, scale):
    """gfx_wm_btn in Python, to check the guest's button side."""
    q = scale if scale >= 4 else max(1, scale) * 4
    n = (BTN * q + 2) // 4
    if title_h > 0 and n > title_h - 6:
        n = title_h - 6
    return max(n, BTN)


def btn_center(hit, which):
    x, y, w, title, n = hit["x"], hit["y"], hit["w"], hit["title"], hit["btn"]
    k = {"close": 1, "max": 2, "min": 3}[which]
    bx = x + w - PAD - k * (n + 4)
    by = y + PAD + (title - n) // 2
    return bx + n // 2, by + n // 2


def red_close(monitor, name, top=36, bottom=H - 64):
    """Centre of the topmost red close button on the screen, or None."""
    _, _, rgb = g.screendump_rgb(monitor, name)
    ymin = None
    pts = []
    for y in range(top, bottom):
        row = y * W * 3
        for x in range(W - 1, -1, -1):
            i = row + x * 3
            if abs(rgb[i] - RED[0]) <= 4 and abs(rgb[i + 1] - RED[1]) <= 4 and abs(rgb[i + 2] - RED[2]) <= 4:
                pts.append((x, y))
        if pts and ymin is None:
            ymin = y
        if ymin is not None and y > ymin + 40:
            break
    if not pts:
        return None
    xmax = max(p[0] for p in pts)
    blob = [p for p in pts if p[0] >= xmax - 40]
    x0, x1 = min(p[0] for p in blob), max(p[0] for p in blob)
    y0, y1 = min(p[1] for p in blob), max(p[1] for p in blob)
    return (x0 + x1) // 2, (y0 + y1) // 2, x1 - x0 + 1


def click_once(qmp, proc, x, y, marker, label):
    """One tablet click (never repeated: a second one would toggle back)."""
    start = len(g.log_text())
    g.click_tablet(qmp, x, y, W, H)
    try:
        g.wait_for(proc, marker, CLICK_TIMEOUT, start)
    except RuntimeError:
        raise RuntimeError("%s: no %r after the click at (%d, %d); serial tail:\n%s" %
                           (label, marker, x, y, g.normalized_log(g.log_text()[start:])[-1200:]))
    time.sleep(0.6)
    return start


def last_hit(start, who):
    text = g.normalized_log(g.log_text()[start:])
    hits = [m for m in HIT_RE.finditer(text) if m.group(1) == who]
    if not hits:
        raise RuntimeError("no 'gfx wm hit %s' line" % who)
    m = hits[-1]
    return {"who": m.group(1), "x": int(m.group(3)), "y": int(m.group(4)), "w": int(m.group(5)),
            "h": int(m.group(6)), "title": int(m.group(7)), "btn": int(m.group(8)), "q": int(m.group(9))}


def probe(qmp, monitor, proc, who, tag):
    """Find the window by its close button, click its title bar left of
    the buttons, return the guest's rect for it."""
    c = red_close(monitor, "qemu-lot88-%s-%s.ppm" % (who, tag))
    if c is None:
        raise RuntimeError("%s %s: no red close button on screen" % (who, tag))
    cx, cy, _ = c
    px = cx - 5 * 32 - 20
    start = click_once(qmp, proc, px, cy, "gfx wm hit " + who, "%s %s probe" % (who, tag))
    # a title press starts a drag; its release logs "moved" (same place)
    time.sleep(0.4)
    hit = last_hit(start, who)
    ex, ey = btn_center(hit, "close")
    if abs(ex - cx) > 3 or abs(ey - cy) > 3:
        raise RuntimeError("%s %s: close button drawn at (%d, %d), hit test puts it at (%d, %d) (%r)" %
                           (who, tag, cx, cy, ex, ey, hit))
    if hit["btn"] != py_btn(hit["title"], hit["q"]):
        raise RuntimeError("%s %s: button side %d, gfx_wm_btn(%d, %d) is %d" %
                           (who, tag, hit["btn"], hit["title"], hit["q"], py_btn(hit["title"], hit["q"])))
    return hit


def grip_drag(qmp, proc, who, hit, dx, dy, label):
    gx, gy = hit["x"] + hit["w"] - 3, hit["y"] + hit["h"] - 3
    start = len(g.log_text())
    tw.move(qmp, gx, gy)
    tw.button(qmp, True)
    tw.move(qmp, max(0, min(W - 1, gx + dx)), max(0, min(H - 1, gy + dy)))
    tw.button(qmp, False)
    try:
        g.wait_for(proc, "gfx wm %s resized" % who, CLICK_TIMEOUT, start)
    except RuntimeError:
        raise RuntimeError("%s: no 'gfx wm %s resized'; serial tail:\n%s" %
                           (label, who, g.normalized_log(g.log_text()[start:])[-1200:]))
    time.sleep(0.8)


def toggle_max(qmp, proc, who, hit, label):
    x, y = btn_center(hit, "max")
    start = click_once(qmp, proc, x, y, "gfx wm %s maximize o" % who, label)
    text = g.normalized_log(g.log_text()[start:])
    return "on" if ("gfx wm %s maximize on" % who) in text else "off"


def same_rect(a, b):
    return (a["x"], a["y"], a["w"], a["h"]) == (b["x"], b["y"], b["w"], b["h"])


def resize_checks(qmp, monitor, proc, who, rest):
    """rest: the restored (not maximized) rect."""
    grip_drag(qmp, proc, who, rest, 3000, 3000, who + " grow past the screen")
    grown = probe(qmp, monitor, proc, who, "grown")
    if grown["x"] + grown["w"] > W or grown["y"] + grown["h"] > H:
        raise RuntimeError("%s grown off the screen: %r" % (who, grown))
    if grown["w"] < rest["w"] or grown["h"] < rest["h"]:
        raise RuntimeError("%s did not grow: %r -> %r" % (who, rest, grown))
    grip_drag(qmp, proc, who, grown, -100, -60, who + " shrink")
    shrunk = probe(qmp, monitor, proc, who, "shrunk")
    if (shrunk["w"], shrunk["h"]) != (grown["w"] - 100, grown["h"] - 60):
        raise RuntimeError("%s: a 100 x 60 shrink of %dx%d gave %dx%d" %
                           (who, grown["w"], grown["h"], shrunk["w"], shrunk["h"]))
    grip_drag(qmp, proc, who, shrunk, rest["w"] - shrunk["w"], rest["h"] - shrunk["h"], who + " back")
    back = probe(qmp, monitor, proc, who, "back")
    if (back["w"], back["h"]) != (rest["w"], rest["h"]):
        raise RuntimeError("%s: back to %dx%d gave %dx%d" % (who, rest["w"], rest["h"], back["w"], back["h"]))
    say("%s: grip grew to %dx%d (clamped), shrank by 100x60, came back to %dx%d" %
        (who, grown["w"], grown["h"], back["w"], back["h"]))
    return back


def window_chrome(qmp, monitor, proc, who, dock=None, close_marker=None):
    """Every chrome action on the one open window of type who."""
    first = probe(qmp, monitor, proc, who, "open")
    say("%s at %d,%d %dx%d title %d btn %d q %d" % (who, first["x"], first["y"], first["w"], first["h"],
                                                  first["title"], first["btn"], first["q"]))
    state = toggle_max(qmp, proc, who, first, who + " maximize")
    other = probe(qmp, monitor, proc, who, "max1")
    if same_rect(first, other):
        raise RuntimeError("%s maximize %s did not change the rect %r" % (who, state, first))
    if state == "on" and (other["w"] < first["w"] or other["h"] < first["h"]):
        raise RuntimeError("%s maximized is smaller: %r -> %r" % (who, first, other))
    if state == "off":
        # the full screen window opens maximized: resize the restored one
        rest = resize_checks(qmp, monitor, proc, who, other)
        if toggle_max(qmp, proc, who, rest, who + " maximize again") != "on":
            raise RuntimeError("%s did not maximize again" % who)
        again = probe(qmp, monitor, proc, who, "max2")
        if not same_rect(again, first):
            raise RuntimeError("%s maximized again at %r, first %r" % (who, again, first))
    else:
        if toggle_max(qmp, proc, who, other, who + " restore") != "off":
            raise RuntimeError("%s did not restore" % who)
        again = probe(qmp, monitor, proc, who, "max2")
        if not same_rect(again, first):
            raise RuntimeError("%s restored at %r, first %r" % (who, again, first))
        again = resize_checks(qmp, monitor, proc, who, again)
    if dock is not None:
        x, y = btn_center(again, "min")
        click_once(qmp, proc, x, y, "gfx wm %s minimize" % who, who + " minimize")
        if red_close(monitor, "qemu-lot88-%s-min.ppm" % who) is not None:
            raise RuntimeError("%s minimized but a close button is still drawn" % who)
        click_once(qmp, proc, dock[0], dock[1], "gfx wm %s restore" % who, who + " dock restore")
        again = probe(qmp, monitor, proc, who, "restored")
    x, y = btn_center(again, "close")
    click_once(qmp, proc, x, y, close_marker or ("gfx wm %s close" % who), who + " close")
    say("%s: maximize, resize%s and close at q=%d" % (who, ", minimize/dock" if dock else "", first["q"]))
    return first


def run(qmp, monitor, proc, docks=None, want_q=None):
    """The four window types, one at a time. docks: dock centres when known
    (1x boot) for minimize / restore."""
    hits = {}
    g.send_command_until(monitor, "gui", "osui gui live", proc, wait_prompt=False)
    g.wait_for(proc, "OSUI-SNAP", g.CMD_TIMEOUT)
    time.sleep(1.5)
    hits["chat"] = window_chrome(qmp, monitor, proc, "chat", docks[0] if docks else None)
    if docks is not None:
        # minimize/restore left the chat open: close it for the next ones
        pass
    g.send_command_until(monitor, "/status", "line=/status", proc, mode="getc", wait_prompt=False)
    time.sleep(2.0)
    hits["pane"] = window_chrome(qmp, monitor, proc, "pane", docks[5] if docks else None)
    g.send_command_until(monitor, "/files", "osui files open dir=/", proc, mode="getc", wait_prompt=False)
    time.sleep(2.0)
    hits["app"] = window_chrome(qmp, monitor, proc, "app", docks[6] if docks else None)
    g.send_command_until(monitor, "/browse", "osui web render file=/web/index.html", proc,
                         mode="getc", wait_prompt=False)
    time.sleep(3.0)
    hits["full"] = window_chrome(qmp, monitor, proc, "full", docks[1] if docks else None)
    qs = set(h["q"] for h in hits.values())
    if len(qs) != 1:
        raise RuntimeError("window types disagree on the chrome scale: %r" % hits)
    q = qs.pop()
    if want_q is not None and not want_q(q):
        raise RuntimeError("chrome scale q=%d not the one asked for" % q)
    return q
