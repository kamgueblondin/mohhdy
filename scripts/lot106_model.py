#!/usr/bin/env python3
"""Lot 106 model: the default interpolation space of colours, body as a
view timeline subject, box scroller heights in font and viewport units.

Written from CSS Color 4 (12.1 interpolation space, 12.2 and 12.3
missing components, 4.4 none outside interpolation), CSS Scroll-driven
Animations 1 (view progress timelines) and docs/lot106_plan.md, not from
the C sources. lot97_animmodel.interp_colour is converted to consult
this model.

Colour interpolation (keyframes and transitions, wf_cssprop_interp)
* Two colours both in a legacy sRGB format (hex, a named colour,
  transparent, rgb(), rgba(), hsl(), hsla(), hwb(), in either syntax;
  currentcolor counts as one: the reader keeps it as an sRGB colour)
  interpolate in sRGB as before (Lot 97 to Lot 102).
* Otherwise (lab(), lch(), oklab(), oklch(), color(), color-mix() on
  either side) they interpolate in Oklab: the mix of color-mix(in oklab,
  A (100 - t)%, B t%) of the Lot 103 model (premultiplied, missing
  components carried by analogous set, gamut-mapped to sRGB), each
  channel rounded half away from zero to 0..255, the alpha written in
  1/10000 as the sRGB path writes it. Past the ends (an easing that
  overshoots) the weights are 100 - t and t as they are; the
  premultiplied components are divided by the weighted alpha sum as it
  is (CSS Color 4 12.3: un-premultiplied by the interpolated alpha), and
  the alpha is then clamped to 0..1 (an alpha sum of 0 gives 0
  components). Far past the ends the components can grow without bound
  as the alpha sum nears 0: such a case (linear sRGB past 8) is unsure.
  (The legacy sRGB path keeps its Lot 97 order: alpha clamped first.)
* add and accumulate are unchanged: premultiplied sRGB, a missing
  component counting as 0 (CSS Color 4 4.4: none is 0 outside
  interpolation).

View timelines (CSS Scroll-driven Animations 1, 3.1)
* The root element (html) has no ancestor scroll container: a view
  timeline whose subject it is (view() on it, or a view-timeline-name it
  defines) is inactive, and asks for no geometry.
* The body element (named body, a child of the root html element or at
  the top) is a subject like any other, whose nearest scroll container
  is the root (html and body are never box scrollers, Lot 105); its box
  comes from the geometry's own body record (geom["body"]), not from the
  boxes by element index. Everything else is the Lot 105 view clock.
"""
from fractions import Fraction as F
import os
import sys

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
import lot98_animmodel as M98  # noqa: E402
import lot100_colormodel as L100  # noqa: E402
import lot103_model as L103  # noqa: E402

S = 10000
Unsure = L103.Unsure

# False only inside keep(): the Lot 105 (sRGB) answer of an older case
OKLAB = True
LEGACY_FN = ("rgb(", "rgba(", "hsl(", "hsla(", "hwb(")


def legacy(v):
    """1 when colour text v is in a legacy sRGB format"""
    lv = v.lower()
    if lv.startswith("#") or L100.is_name(lv) or lv in ("transparent", "currentcolor"):
        return True
    return any(len(lv) > len(f) and lv.startswith(f) for f in LEGACY_FN)


def oklab_mix(x, y, p0, p1):
    """(channels 0..255, alpha 1/10000) of colours x, y (L103.full tuples)
    mixed in oklab with weights p0, p1 (1/10000 %, sum 100%)"""
    kind, sets, _ = L103.MIX["oklab"]
    tot = p0 + p1
    cv, mi = [], []
    for col in (x, y):
        c = L103.comps("oklab", col)
        m = 0
        for j in range(3):
            if (col[2] >> sets[j]) & 1:
                m |= 1 << j
        if (col[2] >> 3) & 1:
            m |= 8
        cv.append(list(c) + [col[1]])
        mi.append(m)
    for k in range(4):
        m0, m1 = (mi[0] >> k) & 1, (mi[1] >> k) & 1
        if m0 and m1:
            cv[0][k] = cv[1][k] = 0
        elif m0:
            cv[0][k] = cv[1][k]
        elif m1:
            cv[1][k] = cv[0][k]
    a0, a1 = cv[0][3], cv[1][3]
    an = a0 * p0 + a1 * p1
    out = [(cv[0][k] * a0 * p0 + cv[1][k] * a1 * p1) / an if an else 0.0 for k in range(3)]
    alpha = min(max(M98.rhalf(F(an, max(tot, 100 * S))), 0), S)
    lin = L103.to_lin(kind, out)
    if max(abs(v) for v in lin) > L103.BIG_MIX:
        raise Unsure("big")
    ch = L103.map_encode(lin, out, False)
    return [min(max(M98.rhalf(F(c, S)), 0), 255) for c in ch], alpha


def write(ch, alpha):
    import lot97_animmodel as M97
    if alpha == S:
        return "rgb(%d, %d, %d)" % tuple(ch)
    return "rgba(%d, %d, %d, %s)" % (ch[0], ch[1], ch[2], M97.P.fmt(alpha, ""))


def oklab_interp(a, b, t):
    """the interpolated text of colours a, b at t (1/10000) in oklab, or
    None when one is not a colour"""
    x, y = L103.full(a), L103.full(b)
    if x is None or y is None:
        return None
    ch, alpha = oklab_mix(x, y, (S - t) * 100, t * 100)
    return write(ch, alpha)


def keep(fn, alts, a, b, t):
    """the converted case fn(alts, a, b, t) of an older generator, as
    (m, a, b, t): m is ("ok", answer) or None for a case left out.

    A case the Lot 105 model already left out (unsure) stays out, so the
    case list of the older suite does not move.  A case the Oklab answer
    puts on a rounding boundary takes another t, then (if every t is on a
    boundary, as near black) another second colour rgb(R G B), from a side
    stream seeded by the case text: the main stream is not used."""
    import random
    import zlib

    def run(x, y, u):
        try:
            return ("ok", fn(alts, x, y, u))
        except Unsure:
            return None
    global OKLAB
    OKLAB = False
    try:
        old = run(a, b, t)
    finally:
        OKLAB = True
    if old is None:
        return None, a, b, t
    m = run(a, b, t)
    if m is not None:
        return m, a, b, t
    rs = random.Random(zlib.crc32(repr((a, b, t)).encode()))
    for i in range(128):
        u = rs.randrange(1, S)
        y = b if i < 64 else "rgb(%d %d %d)" % (rs.randrange(256), rs.randrange(256), rs.randrange(256))
        m = run(a, y, u)
        if m is not None:
            return m, a, y, u
    raise AssertionError((a, b, t))


# ------------------------------------------------------------- view timelines
def is_body(nodes, q):
    nd = nodes[q]
    p = nd["parent"]
    return nd["tag"] == "body" and (p < 0 or (nodes[p]["parent"] < 0 and nodes[p]["tag"] == "html"))


def box_of(nodes, q, g):
    """the geometry box of node q (the body record for the body), None"""
    if not g:
        return None
    return g.get("body") if is_body(nodes, q) else g["box"].get(q)


def view_clock(a, nodes, q, axis, ins, c):
    import lot104_model as M4
    import lot105_model as M5
    U = M4.U
    if nodes[q]["parent"] < 0 and nodes[q]["tag"] == "html":
        return None
    s = M5.scroller_of(nodes, q)
    c.need.add(q)
    if s >= 0:
        c.scr.add(s)
    if s >= 0 or c.gui:
        c.live = True
    b = box_of(nodes, q, c.geom)
    if not b:
        return None
    if s < 0 and not b["root"]:
        return None
    if s >= 0 and (b["root"] or b.get("sm", 0) != s + 1):
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


def clock(a, nodes, i, ms, c):
    """lot105_model.clock with this view_clock"""
    import lot104_model as M4
    import lot105_model as M5
    tl = a.get("timeline", "auto")
    if tl == "auto":
        return ("auto", ms)
    nd = nodes[i]
    if isinstance(tl, tuple):
        if tl[0] == "view":
            return view_clock(a, nodes, i, tl[1], tl[2], c)
        _, src, axis = tl
        if axis not in ("block", "y"):
            return None
        if src == "self":
            s = M5.scroll_src(nodes, i)
        elif src == "nearest":
            s = -2 if nd["parent"] < 0 else M5.scroller_of(nodes, i)
        else:
            s = -1
        return None if s < -1 else M5.scroll_clock(a, s, c)
    if tl == "none":
        return None
    q = i
    while q >= 0:
        d = M4.defines(nodes, q, tl)
        if d:
            if d[0] == 2:
                return view_clock(a, nodes, q, d[1], d[2], c)
            s = M5.scroll_src(nodes, q)
            if s < -1 or d[1] not in ("block", "y"):
                return None
            return M5.scroll_clock(a, s, c)
        if M4.scoped_at(nodes, q, tl):
            return None
        q = nodes[q]["parent"]
    return None


def stack(alts, anims, kfs, under, ms, nodes, i, c):
    import lot98_animmodel as M98b
    animated = composed = False
    base = under
    for a in anims:
        if not a["name"] or a["name"] not in kfs:
            continue
        r = clock(a, nodes, i, ms, c)
        if r is None:
            continue
        v, cp = M98b.anim_value(alts, a, kfs[a["name"]], under, r[1], base)
        if v is not None:
            under, animated = v, True
            composed = composed or cp
    return under, animated, composed


def counts(anims, names, nodes, i, ms, c):
    """(scroll, view, box, idle) as lot105_model.counts"""
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
