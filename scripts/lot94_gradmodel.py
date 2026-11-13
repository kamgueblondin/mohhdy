#!/usr/bin/env python3
"""Lot 94 model: angled linear and conic gradients, as the engine paints
them (in cells) and as the reader writes them.

Written from the rules in docs/lot94_plan.md, not from the C sources:

* the painted area is the origin box cut to the clip box; it is split into
  rows = isqrt(floor(32 h / w)) rows (1..h, at most 32) and
  cols = floor(32 / rows) columns (at most w); cell (i, j) spans
  x0 + floor(i w / cols) .. x0 + floor((i + 1) w / cols), rows alike, and
  the cells come row by row;
* a cell takes the colour of the gradient at its centre:
  - linear at a whole angle a: s = round(65536 sin a), c = round(65536
    cos a) (Python's math module), position 1/2 + ((x - cx) s - (y - cy) c)
    / (W |s| + H |c|) of the origin box (centre cx, cy, size W, H);
    to a corner, s = +-H and c = +-W (top right +H +W); in 1/10000,
    rounded half away from zero (exact fractions here);
  - conic: the direction of the cell centre from the "at" point,
    clockwise from up, as a fraction of a turn, minus from / 360, modulo
    1; in 1/10000 rounded half up (floating point here; a cell within
    1e-3 of a rounding boundary or 1e-6 of the wrap makes the case
    ambiguous, and the generators draw another);
* the colour at p (1/10000) between stops: before the first stop the
  first colour, past the last the last one, equal positions the later
  colour; else t = floor((p - p0) 10000 / (p1 - p0)) and each channel is
  round half up of (v0 (10000 - t) + v1 t) / 10000 (opaque colours only);
* stop positions: N% is N * 100 (one decimal), the first defaults to 0 and
  the last to 10000, a position below an earlier one is raised to it, the
  ones without a position are spread evenly (floor) between their
  neighbours; a conic stop "Ndeg" is round half up of N * 10000 / 360.
"""
import math
from fractions import Fraction as F


def sin65536(d):
    """round(65536 sin d) for whole degrees, exact at multiples of 30 and 90"""
    d %= 360
    exact = {0: 0, 30: 32768, 90: 65536, 150: 32768, 180: 0, 210: -32768, 270: -65536, 330: -32768}
    if d in exact:
        return exact[d]
    v = math.sin(math.radians(d)) * 65536
    r = math.floor(abs(v) + 0.5)
    assert abs(abs(v) - math.floor(abs(v)) - 0.5) > 1e-6
    return r if v >= 0 else -r


def isqrt(n):
    return math.isqrt(n)


def round_half_away(fr):
    if fr >= 0:
        return math.floor(fr + F(1, 2))
    return -math.floor(-fr + F(1, 2))


def grid(o, c):
    """the cells (x, y, w, h) of origin box o cut to clip box c"""
    ax0, ay0 = max(o[0], c[0]), max(o[1], c[1])
    ax1, ay1 = min(o[0] + o[2], c[0] + c[2]), min(o[1] + o[3], c[1] + c[3])
    aw, ah = ax1 - ax0, ay1 - ay0
    if aw <= 0 or ah <= 0:
        return []
    rows = isqrt(32 * ah // aw)
    rows = max(1, min(rows, ah, 32))
    cols = min(32 // rows, aw)
    out = []
    for j in range(rows):
        y0, y1 = ay0 + j * ah // rows, ay0 + (j + 1) * ah // rows
        for i in range(cols):
            x0, x1 = ax0 + i * aw // cols, ax0 + (i + 1) * aw // cols
            out.append((x0, y0, x1 - x0, y1 - y0))
    return out


def linear_p(o, cell, angle=None, corner=None):
    """p (1/10000) of a cell centre; corner 'top right' style tuple
    (vertical, horizontal) or a whole angle"""
    if corner is not None:
        v, h = corner
        s = -o[3] if h == "left" else o[3]
        c = -o[2] if v == "bottom" else o[2]
    else:
        s, c = sin65536(angle), sin65536(angle + 90)
    x = F(2 * cell[0] + cell[2], 2) - F(2 * o[0] + o[2], 2)
    y = F(2 * cell[1] + cell[3], 2) - F(2 * o[1] + o[3], 2)
    length = o[2] * abs(s) + o[3] * abs(c)
    return round_half_away(F(10000) * (F(1, 2) + (x * s - y * c) / length))


def linear_p_true(o, cell, angle):
    """the CSS position with exact trigonometry (a check of the cells)"""
    a = math.radians(angle)
    x = cell[0] + cell[2] / 2 - (o[0] + o[2] / 2)
    y = cell[1] + cell[3] / 2 - (o[1] + o[3] / 2)
    length = abs(o[2] * math.sin(a)) + abs(o[3] * math.cos(a))
    return 10000 * (0.5 + (x * math.sin(a) - y * math.cos(a)) / length)


class Ambiguous(Exception):
    pass


def conic_p(cx2, cy2, cell, frm):
    """p of a cell centre around the doubled centre (cx2, cy2)"""
    dx = 2 * cell[0] + cell[2] - cx2
    dy = 2 * cell[1] + cell[3] - cy2
    if dx == 0 and dy == 0:
        turn = F(0)
    elif dx == 0:
        turn = F(0) if dy < 0 else F(1, 2)
    elif dy == 0:
        turn = F(1, 4) if dx > 0 else F(3, 4)
    else:
        turn = None
    if turn is not None:
        t = (turn - F(frm, 360)) % 1
        return math.floor(t * 10000 + F(1, 2))
    phi = math.atan2(dx, -dy) / (2 * math.pi)
    t = (phi - frm / 360.0) % 1.0
    if min(t, 1 - t) < 1e-6:
        raise Ambiguous()
    x = t * 10000
    if abs(x - math.floor(x) - 0.5) < 1e-3:
        raise Ambiguous()
    return math.floor(x + 0.5)


def stop_positions(stops):
    """stops: list of (rgb, pos or None) with pos in 1/10000"""
    pos = [p for _, p in stops]
    if pos[0] is None:
        pos[0] = 0
    if pos[-1] is None:
        pos[-1] = 10000
    last = 0
    for k in range(1, len(pos)):
        if pos[k] is None:
            continue
        pos[k] = max(pos[k], pos[last])
        for j in range(last + 1, k):
            pos[j] = pos[last] + (pos[k] - pos[last]) * (j - last) // (k - last)
        last = k
    return pos


def colour_at(stops, p):
    pos = stop_positions(stops)
    cols = [c for c, _ in stops]
    if p <= pos[0]:
        return cols[0]
    k = 0
    while k + 1 < len(pos) and p > pos[k + 1]:
        k += 1
    if k + 1 >= len(pos):
        return cols[-1]
    if pos[k + 1] == pos[k]:
        return cols[k + 1]
    t = (p - pos[k]) * 10000 // (pos[k + 1] - pos[k])
    out = 0
    for sh in (0, 8, 16):
        v0, v1 = (cols[k] >> sh) & 255, (cols[k + 1] >> sh) & 255
        out |= math.floor(F(v0 * (10000 - t) + v1 * t, 10000) + F(1, 2)) << sh
    return out


def conic_stop_pos(deg):
    return math.floor(F(deg * 10000, 360) + F(1, 2))


def items_text(items):
    return ";".join("%d %d %d %d %08X" % it for it in items)


def rad_whole_degrees(text):
    """whole degrees of a radian value (text without the unit): x * 180 /
    pi rounded half away from zero; None when within 1e-6 of a tie"""
    r = float(text)
    a = abs(r) * 180.0 / math.pi
    if abs(a - math.floor(a) - 0.5) < 1e-6:
        return None
    w = int(math.floor(a + 0.5))
    return -w if r < 0 and w else w


def decimal_degrees(text, unit):
    """whole degrees the engine takes for deg, turn or grad text with at
    most three decimals: the exact value rounded half away from zero"""
    v = F(text)
    v = v * {"deg": 1, "turn": 360, "grad": F(9, 10)}[unit]
    return round_half_away(v)
