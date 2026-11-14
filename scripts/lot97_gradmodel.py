#!/usr/bin/env python3
"""Lot 97 model: the engine's gradient changes, written from the rules in
docs/lot97_plan.md, not from the C sources (userspace/webeng.c):

* a repeating gradient whose period is 0 (the last stop, after raising,
  not past the first) paints its average colour: the stops taken evenly
  spaced, the mean of the premultiplied linear interpolation, so weights 1
  at both ends and 2 between; each channel round half up of
  sum(w a v) / sum(w a), the opacity round half up of sum(w a) /
  (2 (n - 1)); fully transparent stops give transparent black. It is
  painted as a solid gradient along the y axis (180deg), so one rectangle
  over the origin box cut to the clip box;
* a repeating linear gradient along an axis (a side, or 0/90/180/270deg)
  is painted per pixel line across the axis: its colour at the pixel
  centre, equal neighbours merged; when that gives more rectangles than
  the 32 reserved cells, the Lot 96 cells are painted instead;
* in a vertical-rl box the cells are painted in the logical frame with
  their rows cut from the logical bottom: row j of R over a height H at
  the bottom y1 spans [y1 - (R - j) H / R, y1 - (R - j - 1) H / R)
  (integer division), and a repeating radial centre is mirrored in the
  logical frame (cy -> 2 oy + oh - cy).

Colours are 0xAARRGGBB with the alpha byte stored inverted (0 opaque).
"""
import os
import sys
from fractions import Fraction as F

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
import lot94_gradmodel as M  # noqa: E402
import lot96_gradmodel as M96  # noqa: E402

CELLS = 32


def half_up(fr):
    """round a non-negative Fraction half up"""
    return int(fr + F(1, 2))


def average(colours):
    """the average colour of stops 0xAARRGGBB (alpha inverted)"""
    n = len(colours)
    assert n >= 2
    tot, num = 0, [0, 0, 0]
    for k, c in enumerate(colours):
        w = 1 if k in (0, n - 1) else 2
        a = 255 - (c >> 24)
        tot += w * a
        for ch in range(3):
            num[ch] += w * a * ((c >> (8 * ch)) & 255)
    if tot == 0:
        return 255 << 24
    out = 0
    for ch in range(3):
        out |= half_up(F(num[ch], tot)) << (8 * ch)
    alpha = half_up(F(tot, 2 * (n - 1)))
    return out | ((255 - alpha) << 24)


def area(o, c):
    ax0, ay0 = max(o[0], c[0]), max(o[1], c[1])
    ax1, ay1 = min(o[0] + o[2], c[0] + c[2]), min(o[1] + o[3], c[1] + c[3])
    return ax0, ay0, ax1 - ax0, ay1 - ay0


def solid_items(o, c, colour):
    ax0, ay0, aw, ah = area(o, c)
    if aw <= 0 or ah <= 0:
        return []
    return [(ax0, ay0, aw, ah, colour)]


def axis_runs(o, c, colour_of, angle, cells=CELLS):
    """per-pixel runs of an axis gradient (angle 0, 90, 180 or 270): items,
    or None when they pass the reserved cells. colour_of(p) gives the
    colour of a position (repeat applied)."""
    ax0, ay0, aw, ah = area(o, c)
    if aw <= 0 or ah <= 0:
        return []
    vertical = angle % 180 == 0
    n = ah if vertical else aw
    runs = []
    for q in range(n):
        cell = (ax0, ay0 + q, aw, 1) if vertical else (ax0 + q, ay0, 1, ah)
        col = colour_of(M.linear_p(o, cell, angle=angle))
        if runs and runs[-1][1] == col:
            continue
        if len(runs) == cells:
            return None
        runs.append([q, col])
    out = []
    for k, (q, col) in enumerate(runs):
        e = runs[k + 1][0] if k + 1 < len(runs) else n
        if vertical:
            out.append((ax0, ay0 + q, aw, e - q, col))
        else:
            out.append((ax0 + q, ay0, e - q, ah, col))
    return out


def axis_items(o, c, stops, angle):
    """a repeating linear gradient along an axis: the runs, or the Lot 96
    cells when they do not fit"""
    pos = M.stop_positions(stops)

    def colour_of(p):
        return M.colour_at(stops, M96.repeat_p(pos, p))
    runs = axis_runs(o, c, colour_of, angle)
    if runs is not None:
        return runs
    out = []
    for cell in M96.grid_axis(o, c, angle % 180 == 0):
        out.append(cell + (colour_of(M.linear_p(o, cell, angle=angle)),))
    return out


def rl_rows(ay0, ay1, rows):
    """row bounds of the vertical-rl partition"""
    ah = ay1 - ay0
    return [(ay1 - (rows - j) * ah // rows, ay1 - (rows - j - 1) * ah // rows) for j in range(rows)]


def grid(o, c, rl=False, axis=None):
    """the cells (x, y, w, h) of origin box o cut to clip box c: the Lot 94
    grid, or along an axis (axis "v": min(h, 32) rows of one column, "h":
    min(w, 32) columns of one row); rl: rows cut from the bottom"""
    ax0, ay0, aw, ah = area(o, c)
    if aw <= 0 or ah <= 0:
        return []
    if axis == "v":
        rows, cols = min(ah, CELLS), 1
    elif axis == "h":
        rows, cols = 1, min(aw, CELLS)
    else:
        rows = max(1, min(M.isqrt(CELLS * ah // aw), ah, CELLS))
        cols = min(CELLS // rows, aw)
    ay1 = ay0 + ah
    out = []
    for j in range(rows):
        if rl:
            y0, y1 = rl_rows(ay0, ay1, rows)[j]
        else:
            y0, y1 = ay0 + j * ah // rows, ay0 + (j + 1) * ah // rows
        for i in range(cols):
            x0, x1 = ax0 + i * aw // cols, ax0 + (i + 1) * aw // cols
            out.append((x0, y0, x1 - x0, y1 - y0))
    return out


def rl_physical(items, w):
    """logical items of a box of physical width w in a vertical-rl box to
    physical ones (relative to the box): x = w - ly - lh, y = lx"""
    return [(w - y - h, x, h, ww, col) for (x, y, ww, h, col) in items]


def rl_logical_angle(centi):
    """a linear angle (1/100 degree) in the logical frame of vertical-rl"""
    return (centi + 27000) % 36000


def axis_items_rl(o, c, stops, angle, rl):
    """axis_items with the rl partition of the fallback cells"""
    pos = M.stop_positions(stops)

    def colour_of(p):
        return M.colour_at(stops, M96.repeat_p(pos, p))
    runs = axis_runs(o, c, colour_of, angle)
    if runs is not None:
        return runs
    return [cell + (colour_of(M.linear_p(o, cell, angle=angle)),)
            for cell in grid(o, c, rl, "v" if angle % 180 == 0 else "h")]
