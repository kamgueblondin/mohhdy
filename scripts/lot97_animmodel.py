#!/usr/bin/env python3
"""Lot 97 model of the reader's animations and transitions of registered
custom properties, written from docs/lot97_plan.md and the CSS specs it
cites (CSS Easing 1, CSS Animations 1, CSS Transitions 1 and 2, Web
Animations 1, CSS Properties and Values API 1, CSS Color 4), not from the
C sources (userspace/wf_cssprop.c, userspace/wf_sitecss.c).

Numbers are integers in 1/10000. rhalf rounds half away from zero.

* easing: linear; ease, ease-in, ease-out, ease-in-out as their cubic
  beziers; cubic-bezier(x1, y1, x2, y2) with x in 0..1 and y in
  -100..100; step-start, step-end, steps(n[, position]) with 1 <= n <=
  10000 (jump-none needs 2). A bezier is solved on its parameter u in
  1/2^30: the smallest u whose x(u) is at least x, then y(u) rounded half
  away from zero to 1/10000 (inputs at or past 0 and 10000 are returned as they are);
  bezier_true gives the exact curve, within 1 of it. Steps follow CSS
  Easing 1 with the before flag, the result step / jumps rounded half away
  from zero.
* animation shorthand: per item, the first time is the duration (not
  negative), the second the delay (times in s or ms to 1 ms, half away
  from zero, at most 1e8 ms); then the first keyword a property not yet
  given takes, in the order easing, iteration count (infinite or a number
  0..100000), direction, fill mode, play state, and the name last (an
  identifier, not a CSS-wide keyword, under 32 bytes; none is no name).
  At most four items; an empty item makes the list invalid.
* transition shorthand: duration, delay, easing, allow-discrete or normal,
  and the property ("all" by default; none only alone).
* progress: Web Animations 1 timing at local time ms (a paused animation
  stays at 0): before the delay with backwards fill, progress 0; in the
  active interval, iteration floor(e / d) and progress floor(10000 (e mod
  d) / d); after it with forwards fill, the end of the last iteration (a
  whole count gives 10000 of iteration count - 1, a fraction its
  fraction, a count of 0 gives 0). The direction reverses by iteration
  parity. A duration of 0 with an infinite count has no effect.
* interpolation: numbers, lengths, percentages, angles, times as a +
  rhalf((b - a) t / 10000) in their canonical units; length and
  percentage mix into calc(P% + Lpx); integers rounded half toward
  +infinity; '+' and '#' lists item by item (same length); colours in
  sRGB with premultiplied alpha, channels rhalf to 0..255, alpha rhalf to
  1/10000 (Lot 98, converted: hsl() colours too, by
  scripts/lot98_animmodel.py; they flipped at t = 5000); anything else flips at t = 5000. t = 0 and 10000 give the
  endpoints as written.
"""
import math
import os
import re
import sys
from fractions import Fraction as F

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
import lot95_propmodel as P  # noqa: E402

S = 10000
MS_MAX = 100000000
RESERVED = {"initial", "inherit", "unset", "revert", "revert-layer", "default"}


def rhalf(fr):
    fr = F(fr)
    n = abs(fr)
    q = math.floor(n)
    if n - q >= F(1, 2):
        q += 1
    return q if fr >= 0 else -q


def num4(text):
    """a plain number with up to four decimals (fifth rounds half away) in
    1/10000, or None"""
    if not re.fullmatch(r"[+-]?(\d+(\.\d*)?|\.\d+)", text):
        return None
    if text.endswith("."):
        return None
    v, big = P.raw_units(text)
    if big or abs(v) > P.LIM:
        return None
    return v


# ------------------------------------------------------------- easing
KW = {"ease": (2500, 1000, 2500, 10000), "ease-in": (4200, 0, 10000, 10000),
      "ease-out": (0, 0, 5800, 10000), "ease-in-out": (4200, 0, 5800, 10000)}


def ease_parse(s):
    """("linear",) ("bezier", x1, y1, x2, y2) ("steps", n, pos) or None"""
    if s == "linear":
        return ("linear",)
    if s in KW:
        return ("bezier",) + KW[s]
    if s == "step-start":
        return ("steps", 1, "start")
    if s == "step-end":
        return ("steps", 1, "end")
    m = re.fullmatch(r"cubic-bezier\(([^()]*)\)", s)
    if m:
        args = [a.strip() for a in m.group(1).split(",")]
        if len(args) != 4 or any(not a for a in args):
            return None
        v = [num4(a) for a in args]
        if any(x is None for x in v):
            return None
        if not (0 <= v[0] <= S and 0 <= v[2] <= S and -100 * S <= v[1] <= 100 * S and -100 * S <= v[3] <= 100 * S):
            return None
        return ("bezier",) + tuple(v)
    m = re.fullmatch(r"steps\(([^()]*)\)", s)
    if m:
        args = [a.strip() for a in m.group(1).split(",")]
        if len(args) > 2 or any(not a for a in args):
            return None
        if not re.fullmatch(r"\d+", args[0]) or len(args[0]) > 6:
            return None
        n = int(args[0])
        if not 1 <= n <= 10000:
            return None
        pos = "end"
        if len(args) == 2:
            pos = {"jump-start": "start", "start": "start", "jump-end": "end", "end": "end",
                   "jump-none": "none", "jump-both": "both"}.get(args[1])
            if pos is None:
                return None
        if pos == "none" and n < 2:
            return None
        return ("steps", n, pos)
    return None


def _bez(c1, c2, u):
    q = (1 << 30) - u
    return 3 * c1 * q * q * u + 3 * c2 * q * u * u + S * u * u * u


def bezier_at(e, x):
    _, x1, y1, x2, y2 = e
    if x <= 0 or x >= S:
        return x
    want = x << 90
    lo, hi = 0, 1 << 30
    while hi - lo > 1:
        mid = (lo + hi) // 2
        if _bez(x1, x2, mid) < want:
            lo = mid
        else:
            hi = mid
    b = _bez(y1, y2, hi)
    m = (abs(b) + (1 << 89)) >> 90
    return m if b >= 0 else -m


COLOUR_NAMES = set(NAMED_LIST := ["black", "white", "red", "green", "blue", "yellow", "gray", "grey", "silver",
                                   "maroon", "purple", "fuchsia", "lime", "olive", "navy", "teal", "aqua",
                                   "orange", "transparent", "currentcolor"])


def colour_match(v):
    """a <color> value as the registration matches it: a hex colour (3, 4,
    6 or 8 digits), a name, or rgb(), rgba(), hsl(), hsla() with only
    digits, '.', '%', ',', '/', spaces, signs and the letters of deg
    inside one pair of parentheses"""
    if v.startswith("#"):
        return len(v) in (4, 5, 7, 9) and all(c in "0123456789abcdef" for c in v[1:])
    if v in COLOUR_NAMES:
        return True
    for f in ("rgb(", "rgba(", "hsl(", "hsla("):
        if v.startswith(f) and len(v) > len(f) + 1 and v.endswith(")"):
            return all(c in "0123456789.%,/ \t\n\r\fdeg-+" for c in v[len(f):-1])
    return False


def match_alt(alt, v):
    ty, mult, _ = alt
    if ty == "<color>":
        its = P.items(v, mult) if mult else [v]
        return len(its) > 0 and all(x and colour_match(x) for x in its)
    return P.match_alt(alt, v)


def first_alt(alts, v):
    for a in alts:
        if match_alt(a, v):
            return a
    return None


def bezier_true(e, x):
    _, x1, y1, x2, y2 = e
    x1, y1, x2, y2 = (F(v, S) for v in (x1, y1, x2, y2))
    xt = F(x, S)
    lo, hi = F(0), F(1)
    for _ in range(80):
        mid = (lo + hi) / 2
        u = mid
        xv = 3 * x1 * (1 - u) ** 2 * u + 3 * x2 * (1 - u) * u * u + u ** 3
        if xv < xt:
            lo = mid
        else:
            hi = mid
    u = hi
    return float((3 * y1 * (1 - u) ** 2 * u + 3 * y2 * (1 - u) * u * u + u ** 3) * S)


def ease_at(e, x, before=False):
    if e[0] == "linear":
        return x
    if e[0] == "bezier":
        return bezier_at(e, x)
    _, n, pos = e
    cur = math.floor(F(x * n, S))
    if pos in ("start", "both"):
        cur += 1
    if before and (x * n) % S == 0:
        cur -= 1
    if x >= 0 and cur < 0:
        cur = 0
    jumps = n - 1 if pos == "none" else n + 1 if pos == "both" else n
    if x <= S and cur > jumps:
        cur = jumps
    return rhalf(F(cur * S, jumps))


# ------------------------------------------------------------- shorthands
def time_ms(t):
    m = re.fullmatch(r"([+-]?(?:\d+(?:\.\d+)?|\.\d+))(s|ms)", t)
    if not m:
        return None
    v, big = P.raw_units(m.group(1))
    if big:
        return None
    ms = rhalf(F(v * 1000, S)) if m.group(2) == "s" else rhalf(F(v, S))
    return ms if -MS_MAX <= ms <= MS_MAX else None


def is_ident(w):
    return bool(re.fullmatch(r"(-[a-z_-]|[a-z_])[a-z0-9_-]*", w)) and not w.startswith("--") or \
        bool(re.fullmatch(r"--[a-z0-9_-]+", w))


def tokens(item):
    out, depth, cur = [], 0, ""
    for ch in item:
        if ch == "(":
            depth += 1
        elif ch == ")":
            depth -= 1
            if depth < 0:
                return None
        if ch in " \t\n\r" and depth == 0:
            if cur:
                out.append(cur)
            cur = ""
        else:
            cur += ch
    if depth:
        return None
    if cur:
        out.append(cur)
    return out


def list_items(v):
    parts, depth, cur = [], 0, ""
    for ch in v:
        if ch == "(":
            depth += 1
        elif ch == ")":
            depth -= 1
        if ch == "," and depth == 0:
            parts.append(cur.strip())
            cur = ""
        else:
            cur += ch
    parts.append(cur.strip())
    if not v.strip() or any(not p for p in parts) or len(parts) > 4:
        return None
    return parts


DEFAULT_EASE = ("bezier",) + KW["ease"]


def anim_parse(v):
    items = list_items(v)
    if items is None:
        return None
    out = []
    for it in items:
        toks = tokens(it)
        if toks is None:
            return None
        a = {"name": "", "dur": 0, "delay": 0, "iters": S, "dir": "normal", "fill": "none", "paused": False,
             "ease": DEFAULT_EASE}
        have = set()
        times = 0
        for w in toks:
            ms = time_ms(w)
            if ms is not None:
                if times == 0:
                    if ms < 0:
                        return None
                    a["dur"] = ms
                elif times == 1:
                    a["delay"] = ms
                else:
                    return None
                times += 1
                continue
            e = None if "ease" in have else ease_parse(w)
            if e is not None:
                a["ease"] = e
                have.add("ease")
                continue
            if "count" not in have:
                if w == "infinite":
                    a["iters"] = -1
                    have.add("count")
                    continue
                c = num4(w)
                if c is not None:
                    if c < 0 or c > 100000 * S:
                        return None
                    a["iters"] = c
                    have.add("count")
                    continue
            if "dir" not in have and w in ("normal", "reverse", "alternate", "alternate-reverse"):
                a["dir"] = w
                have.add("dir")
                continue
            if "fill" not in have and w in ("none", "forwards", "backwards", "both"):
                a["fill"] = w
                have.add("fill")
                continue
            if "play" not in have and w in ("running", "paused"):
                a["paused"] = w == "paused"
                have.add("play")
                continue
            if "name" in have:
                return None
            have.add("name")
            if w == "none":
                continue
            if not is_ident(w) or w in RESERVED or len(w) >= 32:
                return None
            a["name"] = w
        out.append(a)
    return out


def trans_parse(v):
    items = list_items(v)
    if items is None:
        return None
    out = []
    for it in items:
        toks = tokens(it)
        if toks is None:
            return None
        t = {"prop": "all", "dur": 0, "delay": 0, "discrete": False, "ease": DEFAULT_EASE, "none": False}
        have = set()
        times = 0
        for w in toks:
            ms = time_ms(w)
            if ms is not None:
                if times == 0:
                    if ms < 0:
                        return None
                    t["dur"] = ms
                elif times == 1:
                    t["delay"] = ms
                else:
                    return None
                times += 1
                continue
            e = None if "ease" in have else ease_parse(w)
            if e is not None:
                t["ease"] = e
                have.add("ease")
                continue
            if "beh" not in have and w in ("allow-discrete", "normal"):
                t["discrete"] = w == "allow-discrete"
                have.add("beh")
                continue
            if "prop" in have:
                return None
            have.add("prop")
            if w == "none":
                t["prop"], t["none"] = "", True
                continue
            if not is_ident(w) or len(w) >= 32 or (w in RESERVED and w != "all"):
                return None
            t["prop"] = w
        if t["none"] and len(items) > 1:
            return None
        out.append(t)
    return out


def progress(a, ms):
    """(p, before) or None"""
    d, it_count = a["dur"], a["iters"]
    if d == 0 and it_count < 0:
        return None
    t = 0 if a["paused"] else ms
    e = t - a["delay"]
    before = False
    it = 0
    if e < 0:
        if a["fill"] not in ("backwards", "both"):
            return None
        before, simple = True, 0
    elif d > 0 and (it_count < 0 or e * S < d * it_count):
        it = e // d
        simple = (e - it * d) * S // d
    else:
        if a["fill"] not in ("forwards", "both"):
            return None
        if it_count == 0:
            it, simple = 0, 0
        elif it_count % S == 0:
            it, simple = it_count // S - 1, S
        else:
            it, simple = it_count // S, it_count % S
    dr = a["dir"]
    if dr == "reverse" or (dr == "alternate" and it % 2) or (dr == "alternate-reverse" and not it % 2):
        simple = S - simple
    return simple, before


# ------------------------------------------------------------- interpolation
NAMED = {"black": (0, 0, 0), "white": (255, 255, 255), "red": (255, 0, 0), "green": (0, 128, 0),
         "blue": (0, 0, 255), "yellow": (255, 255, 0), "gray": (128, 128, 128), "grey": (128, 128, 128),
         "silver": (192, 192, 192), "maroon": (128, 0, 0), "purple": (128, 0, 128), "fuchsia": (255, 0, 255),
         "lime": (0, 255, 0), "olive": (128, 128, 0), "navy": (0, 0, 128), "teal": (0, 128, 128),
         "aqua": (0, 255, 255), "orange": (255, 165, 0)}


def rgba(v):
    """channels in 1/10000 of 0..255 and alpha in 1/10000, or None"""
    if v == "transparent":
        return [0, 0, 0], 0
    if v in NAMED:
        return [c * S for c in NAMED[v]], S
    if v.startswith("#"):
        h = v[1:]
        if len(h) not in (3, 4, 6, 8) or not re.fullmatch(r"[0-9a-f]+", h):
            return None
        d = 1 if len(h) in (3, 4) else 2
        vals = [int(h[i * d:(i + 1) * d], 16) * (17 if d == 1 else 1) for i in range(len(h) // d)]
        a = rhalf(F(vals[3] * S, 255)) if len(vals) == 4 else S
        return [x * S for x in vals[:3]], a
    if v.startswith("hsl"):
        import lot98_animmodel  # Lot 98, converted: hsl() was not taken
        return lot98_animmodel.hsl(v)
    m = re.fullmatch(r"rgba?\((.*)\)", v)
    if not m:
        return None
    body = m.group(1).strip()
    if "," in body:
        toks = [x.strip() for x in body.split(",")]
    else:
        main, _, al = body.partition("/")
        toks = main.split() + ([al.strip()] if _ else [])
        if len(main.split()) != 3:
            return None     # Lot 98: four without the slash were taken
    if len(toks) not in (3, 4) or any(not t for t in toks):
        return None
    ch, a = [], S
    for k, t in enumerate(toks):
        pct = t.endswith("%")
        x = num4(t[:-1] if pct else t)
        if x is None:
            return None
        if k < 3:
            ch.append(min(max(rhalf(F(x * 255, 100)) if pct else x, 0), 255 * S))
        else:
            a = min(max(rhalf(F(x, 100)) if pct else x, 0), S)
    return ch, a


def interp_colour(a, b, t):
    x, y = rgba(a), rgba(b)
    if x is None or y is None:
        return None
    (ca, aa), (cb, ab) = x, y
    an = min(max(aa * S + (ab - aa) * t, 0), S * S)
    out = []
    for k in range(3):
        pa, pb = ca[k] * aa, cb[k] * ab
        pn = pa * S + (pb - pa) * t
        out.append(min(max(rhalf(F(pn, an * S)), 0), 255) if an else 0)
    alpha = rhalf(F(an, S))
    if alpha == S:
        return "rgb(%d, %d, %d)" % tuple(out)
    return "rgba(%d, %d, %d, %s)" % (out[0], out[1], out[2], P.fmt(alpha, ""))


def item_q(t, text):
    if text.startswith("calc("):
        q = P.calc(text, None)
    else:
        q = P.leaf(text, None)
        if q is not None and P.NUM.match(text).end() != len(text) and q.k == "N":
            return None
        if q is not None and q.k == "N" and t in ("<length>", "<length-percentage>"):
            q = P.Q("L", q.v)
    if q is None or not q.ok:
        return None
    return q


def interp_num(t, a, b, tt):
    qa, qb = item_q(t, a), item_q(t, b)
    if qa is None or qb is None:
        return None
    pa, ca = P.parts(qa)
    pb, cb = P.parts(qb)
    v = pa + rhalf(F((pb - pa) * tt, S))
    p = ca + rhalf(F((cb - ca) * tt, S))
    if qa.k == qb.k and qa.k != "LP":
        k = qa.k
        if k == "P":
            v = p
    elif qa.k in ("L", "P", "LP") and qb.k in ("L", "P", "LP"):
        k = "LP"
    else:
        return None
    if t == "<integer>":
        v = math.floor(F(2 * v + S, 2 * S)) * S
    if not (P.fits(v) and P.fits(p)):
        return None
    return P.fmt_q(P.Q(k, v, p))


NUMERIC = ("<length>", "<percentage>", "<length-percentage>", "<number>", "<integer>", "<angle>", "<time>",
           "<resolution>")


def interp(alts, a, b, t):
    """(result text, 2 interpolated or 1 discrete) for computed values a, b
    of a registration whose syntax alternatives are alts (None for '*')"""
    if t == 0:
        return a, 2
    if t == S:
        return b, 2
    disc = (a if t < 5000 else b, 1)
    if alts is None:
        return disc
    al_a, al_b = first_alt(alts, a), first_alt(alts, b)
    if al_a is None or al_a is not al_b:
        return disc
    ty, mult, _ = al_a
    if ty == "kw":
        return disc

    def one(x, y):
        if ty in NUMERIC:
            return interp_num(ty, x, y, t)
        if ty == "<color>":
            return interp_colour(x, y, t)
        return None
    if not mult:
        r = one(a, b)
        return (r, 2) if r is not None else disc
    xa, xb = P.items(a, mult), P.items(b, mult)
    if len(xa) != len(xb):
        return disc
    out = []
    for x, y in zip(xa, xb):
        r = one(x, y)
        if r is None:
            return disc
        out.append(r)
    return (", " if mult == "#" else " ").join(out), 2
