#!/usr/bin/env python3
"""Lot 96 model: fractional degrees and repeating gradients, as the engine
paints them (in cells) and as the reader writes them.

Written from the rules in docs/lot96_plan.md, not from the C sources:

* angles are kept to 1/100 degree. The engine reads deg, turn, grad and
  rad text (at most three decimals are read; more are dropped) as the
  value in 1/1000 of its unit times the factor (turn 360, grad 9/10, rad
  573/10), truncated toward zero to 1/1000 degree, then rounded half away
  from zero to 1/100 degree (engine_centi);
* the reader writes a radian value (two integer digits, four decimals) as
  degrees to 1/100: x * 18000 / pi rounded half away from zero (exact pi
  here; a value within 1e-4 of a tie is ambiguous and the generators draw
  another), written "[-]D[.d[d]]deg" without trailing zeros (centi_text);
* sin in 1/65536 of an angle between whole degrees: lot94_gradmodel's
  table value of the lower degree plus (higher - lower) * hundredths / 100
  rounded half up, mirrored from the first quadrant;
* a conic stop "Ndeg" may carry two decimals; its position is round half up
  of N * 10000 / 360;
* a repeating gradient repeats its first to last stop: a position p maps to
  p0 + ((p - p0) mod (pN - p0)); a period of 0 is refused (Lot 97,
  converted: taken, see lot97_gradmodel.py);
* repeating linear gradients are painted in cells; one along an axis (a
  side, or 0/90/180/270deg) uses min(h, 32) rows of one column (top,
  bottom) or min(w, 32) columns of one row (left, right); any other one the
  Lot 94 grid;
* the reader writes a gradient with one space between words and ", "
  between arguments (radians as degrees); a written form past 127
  characters is refused (WF_CSSBG_VAL);
* repeating radial gradients: the Lot 94 grid; a cell centre's position is
  10000 * |(dx / rx, dy / ry)| (dx, dy from the centre): with doubled
  offsets X, Y, R = 2 rx ry, s = isqrt(64 ((X ry)^2 + (Y rx)^2)), then p =
  floor((10000 s + 4 R) / (8 R)); the radii as the Lot 91 rings (extent or
  explicit, at least 1).
"""
import math
import re
from decimal import Decimal, getcontext
from fractions import Fraction as F

getcontext().prec = 60
PI = Decimal("3.14159265358979323846264338327950288419716939937510582097494")


def round_half_away(fr):
    if fr >= 0:
        return math.floor(fr + F(1, 2))
    return -math.floor(-fr + F(1, 2))


def rad_centi(text):
    """1/100 degrees the reader writes for radian text (no unit), or None
    when within 1e-4 of a rounding tie"""
    x = Decimal(text) * 18000 / PI
    a = abs(x)
    fl = int(a)
    if abs(a - fl - Decimal("0.5")) < Decimal("0.0001"):
        return None
    w = int(a + Decimal("0.5"))
    return -w if x < 0 and w else w


def centi_text(c):
    s = "-" if c < 0 else ""
    v = abs(c)
    t = "%s%d" % (s, v // 100)
    f = v % 100
    if f:
        t += ".%02d" % f
        t = t.rstrip("0")
    return t + "deg"


def engine_centi(text, unit):
    """1/100 degrees the engine takes for text (no unit) in unit"""
    num, den = {"deg": (1, 1), "turn": (360, 1), "grad": (9, 10), "rad": (573, 10)}[unit]
    neg = text.startswith("-")
    body = text.lstrip("+-")
    if "." in body:
        w, f = body.split(".")
        f = (f + "000")[:3]
    else:
        w, f = body, "000"
    m = int(w or "0") * 1000 + int(f)
    m = m * num // den
    c = (m + 5) // 10
    return -c if neg else c


def conic_stop_pos_centi(c):
    return (c * 5 + 9) // 18


# ------------------------------------------------------------ the reader
# Which gradient values the background reader takes (CSS Images 3/4,
# restricted as docs/lot91_plan.md, docs/lot94_plan.md and
# docs/lot96_plan.md say), for the forms used by the generators; any
# other form raises Unknown.
class Unknown(Exception):
    pass


NAMED = {"black", "white", "red", "green", "blue", "yellow", "gray", "grey", "navy", "teal", "orange",
         "purple", "silver", "maroon", "lime", "transparent"}
SIDES = {"top", "right", "bottom", "left"}
EXTENTS = {"closest-side", "closest-corner", "farthest-side", "farthest-corner"}


def _num(t, places, lo, hi):
    """value of an unsigned-or-negative decimal with at most places decimals
    in [lo, hi], or None"""
    m = re.fullmatch(r"(-?)(\d+)(?:\.(\d+))?", t)
    if not m or (m.group(3) is not None and len(m.group(3)) > places):
        return None
    v = F(t)
    return v if lo <= v <= hi else None


def colour_ok(t):
    if t in NAMED:
        return True
    if re.fullmatch(r"#[0-9a-f]+", t):
        return len(t) - 1 in (3, 4, 6, 8)
    m = re.fullmatch(r"rgba?\((.*)\)", t)
    if m:
        parts = [p for p in re.split(r"[\s,/]+", m.group(1)) if p]
        if all(re.fullmatch(r"\d+(\.\d+)?%?", p) for p in parts):
            return len(parts) in (3, 4)
        raise Unknown(t)
    if re.fullmatch(r"[a-z-]+", t) or re.fullmatch(r"[-+\d.]+[a-z%]*", t) or t.startswith("("):
        return False
    raise Unknown(t)


def angle_centi(t, rad_ok=True):
    """1/100 degrees the reader writes for a linear angle (or conic from),
    None when it is not one it takes; False when t is no angle at all"""
    for unit, places, lim in (("turn", 2, 10), ("grad", 2, 4000), ("deg", 4, 3600)):
        if t.endswith(unit):
            body = t[:-len(unit)]
            if unit == "deg" and body.endswith("gra"):
                continue
            v = _num(body, places, -lim, lim)
            return None if v is None else True
    if t.endswith("rad"):
        body = t[:-3]
        m = re.fullmatch(r"-?(\d{0,2})(?:\.(\d{1,4}))?", body)
        if not rad_ok or not m or (not m.group(1) and not m.group(2)):
            return None
        return rad_centi(body)
    return False


def position_ok(words):
    """an "at" position of one or two words"""
    def length(w):
        return w == "0" or re.fullmatch(r"-?\d+(\.\d)?(px|%)", w) is not None
    if len(words) == 1:
        if words[0] in SIDES or words[0] == "center" or length(words[0]):
            return True
        raise Unknown(words)
    if len(words) == 2:
        x, y = words
        if (x in ("left", "right", "center") or length(x)) and (y in ("top", "bottom", "center") or length(y)):
            return True
        raise Unknown(words)
    raise Unknown(words)


def stop_pos(t, conic):
    """position in 1/10000 of a stop word, None when refused"""
    if conic and t.endswith("deg"):
        m = re.fullmatch(r"(\d{1,3})(?:\.(\d{1,2}))?deg", t)
        if not m:
            return None
        c = int(m.group(1)) * 100 + (int((m.group(2) + "0")[:2]) if m.group(2) else 0)
        return conic_stop_pos_centi(c) if c <= 36000 else None
    m = re.fullmatch(r"(\d+)(?:\.(\d))?%", t)
    if not m:
        if re.fullmatch(r"-?[\d.]+[a-z%]*", t):
            return None
        raise Unknown(t)
    tenths = int(m.group(1)) * 10 + (int(m.group(2)) if m.group(2) else 0)
    return tenths * 10 if tenths <= 1000 else None


def words(t):
    """t split at top-level white space"""
    out, depth, cur = [], 0, ""
    for c in t:
        depth += c == "("
        depth -= c == ")"
        if c in " \t" and depth == 0:
            if cur:
                out.append(cur)
            cur = ""
        else:
            cur += c
    if cur:
        out.append(cur)
    return out


def split_top(inner):
    out, depth, cur = [], 0, ""
    for c in inner:
        depth += c == "("
        depth -= c == ")"
        if c == "," and depth == 0:
            out.append(cur.strip())
            cur = ""
        else:
            cur += c
    out.append(cur.strip())
    return out


def gradient_ok(s):
    """1 when the reader takes gradient text s (lower case), else 0"""
    s = s.strip()
    repeat = s.startswith("repeating-")
    if repeat:
        s = s[10:]
    kind = None
    for k in ("linear", "radial", "conic"):
        if s.startswith(k + "-gradient("):
            kind = k
    if kind is None or not s.endswith(")"):
        return 0
    if '"' in s or "'" in s:
        raise Unknown(s)
    args = split_top(s[len(kind) + 10:-1])
    if len(args) > 6 or any(a == "" for a in args) or len(args) < 2:
        return 0
    first = 0
    w = words(args[0])
    if kind == "linear":
        if w[0] == "to":
            rest = w[1:]
            ok = (len(rest) == 1 and rest[0] in SIDES) or \
                 (len(rest) == 2 and {rest[0], rest[1]} in ({"top", "left"}, {"top", "right"},
                                                           {"bottom", "left"}, {"bottom", "right"}))
            if not ok:
                return 0
            first = 1
        else:
            a = angle_centi(args[0])
            if a is None:
                return 0
            if a is not False:
                first = 1
    elif kind == "conic":
        if w[0] in ("from", "at"):
            k = 0
            if w[0] == "from":
                if len(w) < 2 or angle_centi(w[1]) in (None, False):
                    return 0
                k = 2
            if k < len(w):
                if w[k] != "at" or k + 1 >= len(w):
                    return 0
                position_ok(w[k + 1:])
            first = 1
    else:
        def size(t):
            return t == "0" or re.fullmatch(r"\d+(\.\d)?(px|%)", t) is not None
        if w[0] in ("circle", "ellipse", "at") or w[0] in EXTENTS or size(w[0]):
            form, ext, sizes, k = None, False, [], 0
            prev = False
            done = False
            while k < len(w) and w[k] != "at":
                if w[k] in ("circle", "ellipse"):
                    if form:
                        return 0
                    form = w[k]
                    done = done or bool(sizes)
                    prev = False
                elif w[k] in EXTENTS:
                    if ext or sizes:
                        return 0
                    ext, prev = True, False
                elif size(w[k]):
                    if ext or done or len(sizes) >= 2 or (len(sizes) == 1 and not prev):
                        return 0
                    sizes.append(w[k])
                    prev = True
                elif re.fullmatch(r"-?[\d.]+[a-z%]*", w[k]):
                    return 0
                else:
                    raise Unknown(w)
                k += 1
            if k < len(w):
                if k + 1 >= len(w):
                    return 0
                position_ok(w[k + 1:])
            pct0 = bool(sizes) and sizes[0].endswith("%")
            if form == "circle" and (len(sizes) == 2 or (len(sizes) == 1 and pct0)):
                return 0
            if form == "ellipse" and len(sizes) == 1:
                return 0
            if form is None and len(sizes) == 1 and pct0:
                return 0
            first = 1
    stops = args[first:]
    if not 2 <= len(stops) <= 4:
        return 0
    pos = []
    for st in stops:
        sw = words(st)
        if len(sw) > 2 or not colour_ok(sw[0]):
            return 0
        if len(sw) == 2:
            p = stop_pos(sw[1], kind == "conic")
            if p is None:
                return 0
            pos.append(p)
        else:
            pos.append(None)
    # the written form: one space between words, ", " between arguments,
    # a radian angle as degrees; it must fit 127 characters
    norm = [" ".join(x.split()) for x in args]
    if first and kind == "linear" and angle_centi(args[0]) not in (None, False, True):
        norm[0] = centi_text(angle_centi(args[0]))
    if first and kind == "conic" and w[0] == "from" and angle_centi(w[1]) is not True:
        norm[0] = " ".join(["from", centi_text(angle_centi(w[1]))] + w[2:])
    if len(("repeating-" if repeat else "") + kind + "-gradient(" + ", ".join(norm) + ")") > 127:
        return 0
    # Lot 97 (converted): a repeating gradient with a period of 0 is taken
    # (the engine paints its average colour; Lot 96 refused it)
    return 1


# ------------------------------------------------------------ painting
def grid_axis(o, c, vertical):
    """the cells of a repeating gradient along an axis: min(h, 32) rows of
    one column (vertical) or min(w, 32) columns of one row"""
    ax0, ay0 = max(o[0], c[0]), max(o[1], c[1])
    ax1, ay1 = min(o[0] + o[2], c[0] + c[2]), min(o[1] + o[3], c[1] + c[3])
    aw, ah = ax1 - ax0, ay1 - ay0
    if aw <= 0 or ah <= 0:
        return []
    rows, cols = (min(ah, 32), 1) if vertical else (1, min(aw, 32))
    out = []
    for j in range(rows):
        y0, y1 = ay0 + j * ah // rows, ay0 + (j + 1) * ah // rows
        for i in range(cols):
            x0, x1 = ax0 + i * aw // cols, ax0 + (i + 1) * aw // cols
            out.append((x0, y0, x1 - x0, y1 - y0))
    return out


def repeat_p(positions, p):
    """p mapped into the first to last stop"""
    p0, period = positions[0], positions[-1] - positions[0]
    assert period > 0
    return p0 + (p - p0) % period


def radial_radii(o, shape, at):
    """centre and radii (cx, cy, rx, ry) of a radial gradient in origin
    box o. shape: (form, extent, sizes): form "circle"/"ellipse"/None,
    extent one of EXTENTS or None, sizes [] or [(v, pct)] or two of them;
    at: ((x, pct), (y, pct))"""
    form, extent, sizes = shape
    cx = o[0] + (o[2] * at[0][0] // 100 if at[0][1] else at[0][0])
    cy = o[1] + (o[3] * at[1][0] // 100 if at[1][1] else at[1][0])
    if sizes:
        if len(sizes) == 1:
            rx = ry = sizes[0][0]
        else:
            rx = o[2] * sizes[0][0] // 100 if sizes[0][1] else sizes[0][0]
            ry = o[3] * sizes[1][0] // 100 if sizes[1][1] else sizes[1][0]
    else:
        extent = extent or "farthest-corner"
        far = extent.startswith("farthest")
        dl, dr = cx - o[0], o[0] + o[2] - cx
        dt, db = cy - o[1], o[1] + o[3] - cy
        sx = abs(max(dl, dr) if far else min(dl, dr))
        sy = abs(max(dt, db) if far else min(dt, db))
        if form == "circle":
            if extent == "closest-side":
                rx = min(sx, sy)
            elif extent == "farthest-side":
                rx = max(sx, sy)
            else:
                rx = math.isqrt(sx * sx + sy * sy)
            ry = rx
        elif extent.endswith("side"):
            rx, ry = sx, sy
        else:
            rx, ry = sx * 1414 // 1000, sy * 1414 // 1000
    return cx, cy, max(rx, 1), max(ry, 1)


def radial_p(cx, cy, rx, ry, cell):
    """position of a cell centre in 1/10000 of the gradient ray"""
    X = 2 * cell[0] + cell[2] - 2 * cx
    Y = 2 * cell[1] + cell[3] - 2 * cy
    n = (X * ry) ** 2 + (Y * rx) ** 2
    s = math.isqrt(64 * n)
    R = 2 * rx * ry
    return (10000 * s + 4 * R) // (8 * R)


def radial_p_true(cx, cy, rx, ry, cell):
    x = cell[0] + cell[2] / 2 - cx
    y = cell[1] + cell[3] / 2 - cy
    return 10000 * math.hypot(x / rx, y / ry)
