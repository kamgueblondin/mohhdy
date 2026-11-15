#!/usr/bin/env python3
"""Lot 101 model of the reader's colours, written from docs/lot101_plan.md
and CSS Color 4 (sections 9 to 10: lab(), lch(), oklab(), oklch() and
color(), with the conversion matrices and transfer functions of its
sample code), not from the C sources (userspace/wf_cssprop.c,
userspace/wf_sitecss.c). It builds on scripts/lot100_colormodel.py, whose
rgba() and colour_match() are converted to consult this model.

The conversions use binary floating point (the C uses fixed point); a
value whose channel, in 1/10000 of 0..255, lies within MARGIN of a
rounding boundary is "unsure": rgba() raises Unsure and the generators
leave such cases out (they would test the last bit of either
arithmetic, not the rules).

* lab(L a b [/ A]) and lch(L C H [/ A]): spaces only (a comma makes the
  colour invalid). L a number or a percentage of 100, clamped to 0..100;
  a and b numbers or percentages of 125; C a number or a percentage of
  150, at least 0; H a number of degrees or an angle, modulo 360; A as in
  rgb(). a, b and C are kept within 10000. CIE Lab with the D50 white,
  Bradford to D65, then linear sRGB.
* oklab(L a b [/ A]) and oklch(L C H [/ A]): the same with L a number or
  a percentage of 1, clamped to 0..1; a, b and C percentages of 0.4, kept
  within 10.
* color(space c1 c2 c3 [/ A]) with srgb, srgb-linear, display-p3,
  a98-rgb, prophoto-rgb, rec2020, xyz, xyz-d65, xyz-d50: components
  numbers or percentages of 1, kept within 100; srgb gives its channels
  directly (clamped to 0..1, times 255); the others go to linear sRGB
  through their transfer function and matrix (prophoto-rgb and xyz-d50
  through Bradford).
* Linear sRGB values are clipped to 0..1 (no gamut mapping) and encoded
  with the sRGB curve; each channel is rounded half away from zero to
  1/10000 of 0..255, like every other colour.
* currentcolor (set by the caller in lot100_colormodel.CUR): in color
  it is the parent's final colour (color: currentcolor alone is inherit),
  in background-color and in the background shorthand the element's own
  final colour, black at the top; in keyframes and starting values the
  same as in the declaration of that property.

Timelines (CSS Scroll-driven Animations 1, the reader subset):

* scroll-timeline-name and view-timeline-name: [none | <dashed-ident>]#,
  scroll-timeline-axis and view-timeline-axis: [block | inline | x | y]#,
  the shorthands scroll-timeline and view-timeline: [<name> <axis>?]#
  (they set both lists, the axis block when left out); at most 4 items,
  names shorter than 32 bytes; initial and unset give no list, any other
  invalid value drops the declaration (inherit too). The names give the
  timeline count, the axes repeat.
* animation-timeline: <dashed-ident> names the timeline of the nearest
  element, the animated one or an ancestor, whose scroll-timeline-name,
  else view-timeline-name, has the name (the last one of its list). Only a
  scroll timeline on the root html element with axis block or y runs (the
  reader viewport); view timelines, inline and x axes, other elements and
  names nobody declares give no effect.
* On a running scroll timeline at position pm (0..10000) the local time
  is floor(pm * (delay * 10000 + duration * count) / 10^8) ms, the
  animation having no effect when the count is infinite or that sum is
  not in 1..10^8 * 10000 (1/10000 ms).
"""
import math
import os
import sys
from fractions import Fraction as F

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
import lot97_animmodel as A  # noqa: E402
import lot98_animmodel as M98  # noqa: E402

S = 10000
SP = " \t\n\r"
MARGIN = 0.02     # in 1/10000 of a channel unit


class Unsure(Exception):
    """a channel too close to a rounding boundary for a float model"""


def rh(x):
    """round half away from zero, Unsure near the boundary"""
    f = abs(x) - math.floor(abs(x))
    if abs(f - 0.5) < MARGIN:
        raise Unsure(x)
    r = math.floor(abs(x) + 0.5)
    return r if x >= 0 else -r


def mm(a, b):
    return [[sum(a[i][k] * b[k][j] for k in range(3)) for j in range(3)] for i in range(3)]


def mv(m, v):
    return [sum(m[i][k] * v[k] for k in range(3)) for i in range(3)]


def fl(m):
    return [[float(x) for x in r] for r in m]


# CSS Color 4 sample code (conversions.js)
XYZ_TO_SRGB = fl([[F(12831, 3959), F(-329, 214), F(-1974, 3959)],
                  [F(-851781, 878810), F(1648619, 878810), F(36519, 878810)],
                  [F(705, 12673), F(-2585, 12673), F(705, 667)]])
P3_TO_XYZ = fl([[F(608311, 1250200), F(189793, 714400), F(198249, 1000160)],
                [F(35783, 156275), F(247089, 357200), F(198249, 2500400)],
                [0, F(32229, 714400), F(5220557, 5000800)]])
A98_TO_XYZ = fl([[F(573536, 994567), F(263643, 1420810), F(187206, 994567)],
                 [F(591459, 1989134), F(6239551, 9945670), F(374412, 4972835)],
                 [F(53769, 1989134), F(351524, 4972835), F(4929758, 4972835)]])
R2020_TO_XYZ = fl([[F(63426534, 99577255), F(20160776, 139408157), F(47086771, 278816314)],
                   [F(26158966, 99577255), F(472592308, 697040785), F(8267143, 139408157)],
                   [0, F(19567812, 697040785), F(295819943, 278816314)]])
PRO_TO_XYZ = [[0.7977666449006423, 0.13518129740053308, 0.0313477341283922],
              [0.2880748288194013, 0.711835234241873, 0.00008993693872564],
              [0.0, 0.0, 0.8251046025104602]]
D50_TO_D65 = [[0.955473421488075, -0.02309845494876471, 0.06325924320057072],
              [-0.0283697093338637, 1.0099953980813041, 0.021041441191917323],
              [0.012314014864481998, -0.020507649298898964, 1.330365926242124]]
OKLAB_TO_LMS = [[1.0, 0.3963377773761749, 0.2158037573099136],
                [1.0, -0.1055613458156586, -0.0638541728258133],
                [1.0, -0.0894841775298119, -1.2914855480194092]]
LMS_TO_XYZ = [[1.2268798758459243, -0.5578149944602171, 0.2813910456659647],
              [-0.0405757452148008, 1.1122868032803170, -0.0717110580655164],
              [-0.0763729366746601, -0.4214933324022432, 1.5869240198367816]]
D50 = [0.3457 / 0.3585, 1.0, (1.0 - 0.3457 - 0.3585) / 0.3585]
KAPPA = 24389 / 27
EPS = 216 / 24389


def srgb_curve(v):
    a = abs(v)
    r = a / 12.92 if a <= 0.04045 else ((a + 0.055) / 1.055) ** 2.4
    return math.copysign(r, v)


def a98_curve(v):
    return math.copysign(abs(v) ** (563 / 256), v)


def pro_curve(v):
    a = abs(v)
    return math.copysign(a / 16 if a <= 16 / 512 else a ** 1.8, v)


def r2020_curve(v):
    al, be = 1.09929682680944, 0.018053968510807
    a = abs(v)
    return math.copysign(a / 4.5 if a < be * 4.5 else ((a + al - 1) / al) ** (1 / 0.45), v)


SPACES = {
    "srgb-linear": (None, None),
    "display-p3": (srgb_curve, P3_TO_XYZ),
    "a98-rgb": (a98_curve, A98_TO_XYZ),
    "prophoto-rgb": (pro_curve, "PRO"),
    "rec2020": (r2020_curve, R2020_TO_XYZ),
    "xyz": (None, "D65"),
    "xyz-d65": (None, "D65"),
    "xyz-d50": (None, "D50"),
}


def encode(lin):
    """linear sRGB to channels in 1/10000 of 0..255 (clipped)"""
    out = []
    for v in lin:
        v = min(max(v, 0.0), 1.0)
        e = 12.92 * v if v <= 0.0031308 else 1.055 * v ** (1 / 2.4) - 0.055
        out.append(rh(e * 255 * S))
    return out


def lab_lin(L, a, b):
    fy = (L + 16) / 116
    fx = a / 500 + fy
    fz = fy - b / 200
    x = fx ** 3 if fx ** 3 > EPS else (116 * fx - 16) / KAPPA
    y = ((L + 16) / 116) ** 3 if L > KAPPA * EPS else L / KAPPA
    z = fz ** 3 if fz ** 3 > EPS else (116 * fz - 16) / KAPPA
    xyz = [x * D50[0], y * D50[1], z * D50[2]]
    return mv(XYZ_TO_SRGB, mv(D50_TO_D65, xyz))


def oklab_lin(L, a, b):
    lms = [c ** 3 for c in mv(OKLAB_TO_LMS, [L, a, b])]
    return mv(XYZ_TO_SRGB, mv(LMS_TO_XYZ, lms))


def comp(t, ref):
    """a number (1/10000) or a percentage of ref, as a float, or None"""
    r = M98.num_pct(t) if t and len(t) < 32 else None
    if r is None:
        return None
    return r[0] / S * (ref / 100 if r[1] else 1)


def hue_deg(t):
    import lot100_colormodel as L100
    h = L100.hue(t)
    return None if h is None else F(h)


def alpha(toks):
    import lot100_colormodel as L100
    if len(toks) == 4:
        return L100.alpha_of(toks[3])
    return S


def clampf(x, lo, hi):
    return min(max(x, lo), hi)


def labish(v):
    lv = v.lower()
    ok = lv.startswith("ok")
    name = lv[2:] if ok else lv
    lch = name.startswith("lch(")
    got = M98.colour_args(v[len("oklab(") if ok else len("lab("):-1])
    if got is None or got[1]:
        return None
    toks = got[0]
    if len(toks) not in (3, 4):
        return None
    lim = 10.0 if ok else 10000.0
    L = comp(toks[0], 1 if ok else 100)
    if L is None:
        return None
    L = clampf(L, 0, 1 if ok else 100)
    if lch:
        c = comp(toks[1], 0.4 if ok else 150)
        h = hue_deg(toks[2])
        if c is None or h is None:
            return None
        c = clampf(c, 0, lim)
        r = math.radians(float(h))
        a, b = c * math.cos(r), c * math.sin(r)
    else:
        a = comp(toks[1], 0.4 if ok else 125)
        b = comp(toks[2], 0.4 if ok else 125)
        if a is None or b is None:
            return None
        a, b = clampf(a, -lim, lim), clampf(b, -lim, lim)
    al = alpha(toks)
    if al is None:
        return None
    lin = oklab_lin(L, a, b) if ok else lab_lin(L, a, b)
    return encode(lin), al


def color_fn(v):
    body = v[6:-1].lstrip(SP)
    i = 0
    while i < len(body) and body[i] not in SP:
        i += 1
    space = body[:i].lower()
    if space != "srgb" and space not in SPACES:
        return None
    got = M98.colour_args(body[i:])
    if got is None or got[1] or len(got[0]) not in (3, 4):
        return None
    toks = got[0]
    if space == "srgb":
        ch = []
        for t in toks[:3]:
            r = M98.num_pct(t) if len(t) < 32 else None
            if r is None:
                return None
            x = F(r[0] * 255, 100) if r[1] else F(r[0] * 255)
            ch.append(min(max(M98.rhalf(x), 0), 255 * S))
        al = alpha(toks)
        return None if al is None else (ch, al)
    xs = []
    for t in toks[:3]:
        c = comp(t, 1)
        if c is None:
            return None
        xs.append(clampf(c, -100, 100))
    al = alpha(toks)
    if al is None:
        return None
    curve, m = SPACES[space]
    if curve:
        xs = [curve(x) for x in xs]
    if m == "D65":
        lin = mv(XYZ_TO_SRGB, xs)
    elif m == "D50":
        lin = mv(XYZ_TO_SRGB, mv(D50_TO_D65, xs))
    elif m == "PRO":
        lin = mv(XYZ_TO_SRGB, mv(D50_TO_D65, mv(PRO_TO_XYZ, xs)))
    elif m is None:
        lin = xs
    else:
        lin = mv(XYZ_TO_SRGB, mv(m, xs))
    return encode(lin), al


def rhalf_f(fr):
    n, d = fr.numerator, fr.denominator
    q = (abs(n) * 2 + d) // (2 * d)
    return q if n >= 0 else -q


PREFIXES = ("lab(", "lch(", "oklab(", "oklch(", "color(")


def is_space_fn(v):
    lv = v.lower()
    return any(lv.startswith(p) and len(v) > len(p) + 1 for p in PREFIXES) and v.endswith(")")


def rgba(v):
    """channels in 1/10000 of 0..255 and alpha in 1/10000 for the Lot 101
    functions, None when invalid, NotImplemented for other values; raises
    Unsure"""
    if not is_space_fn(v):
        return NotImplemented
    lv = v.lower()
    if lv.startswith("color("):
        return color_fn(v)
    return labish(v)


# ------------------------------------------------------------- timelines
TL_AXES = ["block", "inline", "x", "y"]
TL_MAX = 4


def tl_name(w):
    lw = w.lower()
    if lw == "none":
        return ""
    if len(w) < 3 or not w.startswith("--") or not A.is_ident(lw) or len(w) >= 32:
        return None
    return lw


def tl_list(which, v):
    """which: "names", "axes" or "short"; the list of (name, axis) of v,
    or None"""
    items = A.list_items(v)
    if items is None or len(items) > TL_MAX:
        return None
    out = []
    for it in items:
        toks = A.tokens(it)
        if not toks:
            return None
        if which == "short":
            if len(toks) > 2:
                return None
            n = tl_name(toks[0])
            ax = toks[1].lower() if len(toks) == 2 else "block"
            if n is None or ax not in TL_AXES:
                return None
            out.append((n, ax))
        elif len(toks) != 1:
            return None
        elif which == "names":
            n = tl_name(toks[0])
            if n is None:
                return None
            out.append((n, "block"))
        else:
            if toks[0].lower() not in TL_AXES:
                return None
            out.append(("", toks[0].lower()))
    return out


def tl_coord(names, axes):
    if not names:
        return []
    return [(n, axes[k % len(axes)][1] if axes else "block") for k, (n, _) in enumerate(names)]


TL_PROPS = ["scroll-timeline-name", "scroll-timeline-axis", "view-timeline-name", "view-timeline-axis"]


def tl_cascade(decls):
    """the four timeline lists (or None) after the declarations, in order
    (one rule: the later declaration wins)"""
    slots = [None] * 4
    for prop, val in decls:
        v = val.strip(SP).lower()
        ini = v in ("initial", "unset")
        if prop in TL_PROPS:
            k = TL_PROPS.index(prop)
            if ini:
                slots[k] = None
                continue
            r = tl_list("axes" if k & 1 else "names", v)
            if r:
                slots[k] = r
        elif prop in ("scroll-timeline", "view-timeline"):
            k = 0 if prop == "scroll-timeline" else 2
            if ini:
                slots[k] = slots[k + 1] = None
                continue
            r = tl_list("short", v)
            if r:
                slots[k] = [(n, "block") for n, _ in r]
                slots[k + 1] = [("", a) for _, a in r]
    return slots


def scroll_ms(a, pm):
    if a["iters"] < 0 or pm < 0 or pm > S:
        return None
    s = a["delay"] * S + a["dur"] * a["iters"]
    if s <= 0 or s > 100000000 * S:
        return None
    return s * pm // (S * S)


def clock(a, chain, ms, pm):
    """("auto", ms), ("scroll", local ms) or None for animation a; chain:
    (slots, is_root_html) of the element then its ancestors"""
    tl = a.get("timeline", "auto")
    if tl == "auto":
        return ("auto", ms)
    if isinstance(tl, tuple):
        # Lot 102, converted: scroll() and view() (scripts/lot102_model.py)
        import lot102_model as L102
        n = len(chain)
        nodes = [{"parent": k - 1, "tag": "html" if k == 0 and chain[-1][1] else "div", "tl": [None] * 5}
                 for k in range(n)]
        return L102.clock(a, nodes, n - 1, ms, pm)
    if tl == "none":
        return None
    name = tl.lower()
    for slots, root in chain:
        for s in (0, 1):
            if not slots[2 * s]:
                continue
            got = [x for x in tl_coord(slots[2 * s], slots[2 * s + 1]) if x[0] == name]
            if not got:
                continue
            if s or not root or got[-1][1] not in ("block", "y"):
                return None
            import lot102_model as L102     # Lot 102, converted: animation-range
            t = L102.range_ms(a, pm)
            return None if t is None else ("scroll", t)
    return None


def stack_tl(alts, anims, kfs, under, ms, pm, chain):
    """lot98_animmodel.stack with the clock of each animation; also the
    number of animations on a running scroll timeline (kfs: name ->
    frames, the names of the @keyframes rules)"""
    animated = composed = False
    base = under
    for a in anims:
        if not a["name"] or a["name"] not in kfs:
            continue
        c = clock(a, chain, ms, pm)
        if c is None:
            continue
        v, cp = M98.anim_value(alts, a, kfs[a["name"]], under, c[1], base)
        if v is not None:
            under, animated = v, True
            composed = composed or cp
    return under, animated, composed


def n_scroll(anims, names, chain, ms, pm):
    return sum(1 for a in anims if a["name"] and a["name"] in names and
               (clock(a, chain, ms, pm) or ("",))[0] == "scroll")
