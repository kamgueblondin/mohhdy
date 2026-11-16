#!/usr/bin/env python3
"""Lot 105 model: box scrollers as scroll and view timeline sources, and
the GUI scroll position as the root position.

Written from CSS Overflow 3 (scroll containers, the visible/clip rule),
CSS Scroll-driven Animations 1 (sections 2 and 3: scroll(), the nearest
scroll container, scroll-timeline-name on a scroll container, view
timelines of a subject in a scroll container) and docs/lot105_plan.md,
not from the C sources. lot104_model.scroll_clock, view_clock and clock
are converted (box scrollers, the GUI position); the rest is reused.

Scroll containers
* overflow: <kw> [<kw>]? (x then y; one value is both), overflow-x and
  overflow-y: one keyword of visible, hidden, clip, auto (overlay is an
  alias), scroll; initial and unset are visible. Anything else (inherit
  and the revert keywords included) is dropped: these properties are not
  in the reader's CSS-wide keyword handling.
* height: an absolute length (px, cm, mm, q, in, pt, pc, or a unitless
  0), at least 0 and within 100000px, is passed on rounded down to px
  (at most 30000); auto, min-content, max-content, fit-content, a
  percentage, a number with another unit and the math functions are kept
  (they win the cascade) but not passed on; anything else is dropped.
* The strongest declaration wins per longhand (importance, then
  specificity, then order); the overflow shorthand sets both.
* Computed: visible next to hidden, auto or scroll is auto; clip there is
  hidden. An element is a scroll container when one axis is hidden, auto
  or scroll; html and body never are (their overflow belongs to the
  viewport, the root scroller).

Timelines
* scroll(nearest): the nearest scroll container ancestor, else the root
  (the root element itself has none); scroll(self): the root element, or
  the element when it is a scroll container; scroll(root): the root. A
  scroll-timeline-name on the root element or a scroll container defines
  a scroll timeline there; on another element it has no effect. Axis block
  or y only (box scrollers scroll along y).
* A box scroller timeline needs the geometry: the box of the scroller
  with sv (it is a scroll box in the layout); it runs on its own scroll
  range s_rg and offset s_sc (scroll_px_ms; range 0: no effect).
* A view timeline subject's scroller is its nearest scroll container (or
  the root). For the root the box must say root; for a box scroller s the
  box must say not root and sm = s + 1. A box scroller position is its
  scroll offset sc (along the axis); the root position is pm / 10000 of
  the range along y (Lot 104), or with the GUI position the box's scroll
  offsets (sc_b, sc_i).
* The root scroll timeline with the GUI position runs on the geometry
  root range rg_y at sc_y (scroll_px_ms), and needs the geometry.
* Flags: a view subject needs its geometry; a box scroller read by a
  timeline needs its geometry and is passed on (overflow-y:auto or scroll
  and its height) when its computed y is auto or scroll and it has a
  passed height. live: a timeline read a box scroller or the GUI root.
"""
import os
import sys
from fractions import Fraction

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
import lot102_model as L102  # noqa: E402
import lot104_model as M4  # noqa: E402

U = M4.U
S = M4.S
HMAX = 30000
OV = {"visible": 1, "hidden": 2, "clip": 3, "auto": 4, "overlay": 4, "scroll": 5}
OV_NAME = {1: "visible", 2: "hidden", 3: "clip", 4: "auto", 5: "scroll"}
UNIT = {"px": Fraction(1), "in": Fraction(96), "cm": Fraction(9600, 254), "mm": Fraction(960, 254),
        "q": Fraction(240, 254), "pt": Fraction(4, 3), "pc": Fraction(16)}
SP = " \t\n\r"


class Unsure(Exception):
    """a height the float-free model cannot place (rounding at 1/10000 px)"""


# ------------------------------------------------------------- values
def ov_list(prop, v):
    """the (x, y) codes an overflow declaration sets (None: not set), or
    None when dropped"""
    v = v.strip(SP).lower()
    if v in ("initial", "unset"):
        return (1, 1) if prop == "overflow" else ((1, None) if prop == "overflow-x" else (None, 1))
    toks = v.split()
    if not toks or len(toks) > (2 if prop == "overflow" else 1) or any(t not in OV for t in toks):
        return None
    k = [OV[t] for t in toks]
    if prop == "overflow":
        return (k[0], k[-1])
    return (k[0], None) if prop == "overflow-x" else (None, k[0])


def num_unit(t):
    """(number text, unit) of a dimension token, or None"""
    i = 0
    if t[:1] in ("+", "-"):
        i = 1
    j = i
    while j < len(t) and (t[j].isdigit() or t[j] == "."):
        j += 1
    if j == i or not any(c.isdigit() for c in t[i:j]) or t[i:j].count(".") > 1:
        return None
    return t[:j], t[j:]


def height_val(v):
    """("px", px + 1), ("kept", 0) or None (dropped) for a height value"""
    v = v.strip(SP).lower()
    if v in ("initial", "unset"):
        return ("kept", 0)
    if v in ("auto", "min-content", "max-content", "fit-content"):
        return ("kept", 0)
    for f in ("calc(", "min(", "max(", "clamp(", "fit-content("):
        if v.startswith(f) and v.endswith(")") and len(v) > len(f):
            return ("kept", 0)
    nu = num_unit(v)
    if nu is None:
        return None
    n, u = nu
    if u in UNIT or (u == "" and Fraction(n) == 0):
        px = Fraction(n) * UNIT.get(u, Fraction(1))
        if abs(px) <= 100000:
            if px < 0:
                return None
            # the length is kept in 1/10000 px; stay clear of a px edge
            f = px - (px.numerator // px.denominator)
            if f != 0 and (f < Fraction(1, 1000) or f > 1 - Fraction(1, 1000)):
                raise Unsure()
            return ("px", min(int(px), HMAX) + 1)
    if n.startswith("-"):
        return None
    if u == "%":
        return ("kept", 0)
    if u and all("a" <= c <= "z" for c in u):
        return ("kept", 0)
    return None


def sc_cascade(decls):
    """the scroller slots [ox, oy, hpx] of one element from its
    declarations [(rank, prop, value)] (rank: a tuple, larger wins).
    Lot 107, converted: max-height and the heights that are not absolute
    lengths are kept too (scripts/lot107_model.py)"""
    import lot108_model as L108     # Lot 108, converted: width, max-width, min-height
    return L108.sc_cascade(decls)


def sc_cascade_lot105(decls):
    """the Lot 105 cascade (absolute heights only)"""
    best = [None, None, None]
    val = [0, 0, 0]
    for rank, prop, v in decls:
        if prop in ("overflow", "overflow-x", "overflow-y"):
            got = ov_list(prop, v)
            if got is None:
                continue
            for k in (0, 1):
                if got[k] is not None and (best[k] is None or rank >= best[k]):
                    best[k], val[k] = rank, got[k]
        elif prop == "height":
            got = height_val(v)
            if got is None:
                continue
            if best[2] is None or rank >= best[2]:
                best[2], val[2] = rank, got[1]
    return val


def ov_axis(self, other):
    box = other in (2, 4, 5)
    self = self or 1
    if box and self == 1:
        return 4
    if box and self == 3:
        return 2
    return self


def box_scroller(nodes, i):
    """the computed y code when node i is a scroll container, else 0.
    Lot 110, converted: the body is one when html's overflow is not
    visible (scripts/lot110_model.py)"""
    import lot110_model as M10
    return M10.box_scroller(nodes, i)


def box_scroller_lot105(nodes, i):
    """the Lot 105 rule: html and body never are"""
    nd = nodes[i]
    if nd["tag"] in ("html", "body"):
        return 0
    ox, oy = nd["sc"][0], nd["sc"][1]
    x, y = ov_axis(ox, oy), ov_axis(oy, ox)
    if x not in (2, 4, 5) and y not in (2, 4, 5):
        return 0
    return y


def scroller_of(nodes, i):
    p = nodes[i]["parent"]
    while p >= 0:
        if box_scroller(nodes, p):
            return p
        p = nodes[p]["parent"]
    return -1


def passed(nodes, i, scr):
    """the style text the reader adds for node i (a box scroller it
    passes on), or "". Lot 107, converted: relative heights and
    max-height (scripts/lot107_model.py)"""
    import lot108_model as L108     # Lot 108, converted: along x too, min-height
    return L108.passed(nodes, i, scr)


def passed_lot105(nodes, i, scr):
    """the Lot 105 style text (absolute heights only)"""
    if i not in scr or not nodes[i]["sc"][2]:
        return ""
    y = box_scroller(nodes, i)
    if y not in (4, 5):
        return ""
    return "overflow-y:%s;height:%dpx" % (OV_NAME[y], nodes[i]["sc"][2] - 1)


# ------------------------------------------------------------- clock
class Ctx(M4.Ctx):
    """M4.Ctx with the GUI position (gui), the root scroll offset of the
    geometry ("sc_y"), the box scrollers read (scr) and live"""

    def __init__(self, geom, pm, gui=False):
        M4.Ctx.__init__(self, geom, pm)
        self.gui = gui
        self.scr = set()
        self.live = False


def scroll_clock(a, src, c, ax=False, nodes=None):
    """Lot 108, converted: ax (the inline or x axis) runs a box scroller
    on its own range and offset along x (svx, x_rg, x_sc). Lot 110,
    converted: a body box scroller (nodes given) runs on the body record"""
    import lot110_model as M10
    g = c.geom
    if src >= 0:
        c.scr.add(src)
        c.live = True
        b = (M10.box_of(nodes, src, g) if nodes else g["box"].get(src)) if g else None
        if not b or not b.get("svx" if ax else "sv"):
            return None
        if ax:
            t = M4.scroll_px_ms(a, b["x_rg"] * U, b["x_sc"] * U)
        else:
            t = M4.scroll_px_ms(a, b["s_rg"] * U, b["s_sc"] * U)
        return None if t is None else ("box", t)
    if c.gui:
        c.live = True
    elif not M4.has_len(a):
        t = L102.range_ms(a, c.pm)
        return None if t is None else ("scroll", t)
    c.root = True
    if not g:
        return None
    pos = g.get("sc_y", 0) * U if c.gui else c.pm * g["rg_y"] * 100
    t = M4.scroll_px_ms(a, g["rg_y"] * U, pos)
    return None if t is None else ("scroll", t)


def view_clock(a, nodes, q, axis, ins, c):
    import lot110_model as M10
    # Lot 106, converted: the root element's view timeline is inactive
    # (no ancestor scroll container) and asks for no geometry
    if nodes[q]["parent"] < 0 and nodes[q]["tag"] == "html":
        return None
    s = scroller_of(nodes, q)
    c.need.add(q)
    if s >= 0:
        c.scr.add(s)
    if s >= 0 or c.gui:
        c.live = True
    g = c.geom
    b = g["box"].get(q) if g else None
    if not b:
        return None
    if s < 0 and not b["root"]:
        return None
    # Lot 110, converted: a box in the body box scroller says SM_BODY
    if s >= 0 and (b["root"] or b.get("sm", 0) != M10.sm_of(nodes, s)):
        return None
    useb = axis == "block" or (axis == "y" and b["bvert"]) or (axis == "x" and not b["bvert"])
    vert = b["bvert"] if useb else not b["bvert"]
    p = "b" if useb else "i"
    rg = b["rg_" + p]
    if s >= 0 or c.gui:
        pos = b.get("sc_" + p, 0) * U
    else:
        pos = c.pm * rg * 100 if vert else 0
    v = {"start": b[p + "_off"] * U, "size": b[p + "_size"] * U, "view": b["vp_" + p] * U, "range": rg * U,
         "pos": pos, "ins": ins}
    t = M4.view_ms(a, v)
    if t is None:
        return None
    return ("boxview" if s >= 0 else "view", t)


def scroll_src(nodes, j):
    if nodes[j]["parent"] < 0 and nodes[j]["tag"] == "html":
        return -1
    return j if box_scroller(nodes, j) else -2


def clock(a, nodes, i, ms, c):
    """("auto", ms), ("scroll", t), ("view", t), ("box", t), ("boxview",
    t) or None for animation a on node i"""
    tl = a.get("timeline", "auto")
    if tl == "auto":
        return ("auto", ms)
    nd = nodes[i]
    if isinstance(tl, tuple):
        if tl[0] == "view":
            return view_clock(a, nodes, i, tl[1], tl[2], c)
        _, src, axis = tl
        # Lot 108, converted: the inline or x axis on a box scroller (the
        # root scrolls along y only)
        ax = axis in ("inline", "x")
        if src == "self":
            s = scroll_src(nodes, i)
        elif src == "nearest":
            s = -2 if nd["parent"] < 0 else scroller_of(nodes, i)
        else:
            s = -1
        return None if s < -1 or (ax and s < 0) else scroll_clock(a, s, c, ax, nodes)
    if tl == "none":
        return None
    q = i
    while q >= 0:
        d = M4.defines(nodes, q, tl)
        if d:
            if d[0] == 2:
                return view_clock(a, nodes, q, d[1], d[2], c)
            s = scroll_src(nodes, q)
            ax = d[1] in ("inline", "x")     # Lot 108, converted
            if s < -1 or (ax and s < 0):
                return None
            return scroll_clock(a, s, c, ax, nodes)
        if M4.scoped_at(nodes, q, tl):
            return None
        q = nodes[q]["parent"]
    return None


def stack(alts, anims, kfs, under, ms, nodes, i, c):
    import lot98_animmodel as M98
    animated = composed = False
    base = under
    for a in anims:
        if not a["name"] or a["name"] not in kfs:
            continue
        r = clock(a, nodes, i, ms, c)
        if r is None:
            continue
        v, cp = M98.anim_value(alts, a, kfs[a["name"]], under, r[1], base)
        if v is not None:
            under, animated = v, True
            composed = composed or cp
    return under, animated, composed


def counts(anims, names, nodes, i, ms, c):
    """(scroll, view, box, idle) of the animations whose name has
    @keyframes"""
    sc = vw = bx = idle = 0
    for a in anims:
        if not a["name"] or a["name"] not in names:
            continue
        r = clock(a, nodes, i, ms, c)
        if r is None:
            idle += 1
            continue
        if r[0] in ("scroll", "box"):
            sc += 1
        if r[0] in ("view", "boxview"):
            vw += 1
        if r[0] in ("box", "boxview"):
            bx += 1
    return sc, vw, bx, idle
