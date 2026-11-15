#!/usr/bin/env python3
"""Lot 103 model: gamut mapping, color-mix() in every CSS Color 5 space
with the hue interpolation methods, and the analogous sets of missing
components.

Written from CSS Color 4 (sections 12 and 13, with the matrices and
transfer functions of its sample code), CSS Color 5 (color-mix()) and
docs/lot103_plan.md, not from the C sources. lot101_colormodel.encode
and lot102_model.mixm are converted to consult this model.

Gamut mapping (CSS Color 4 13.2.2, sRGB destination)
* A colour whose linear sRGB values are all within 0..1 is kept. Else its
  OKLab origin (the given values of oklab() and oklch(), else converted
  from linear sRGB) decides: L >= 1 is white, L <= 0 black. Else the
  clipped colour is the answer when its deltaE OK to the origin is under
  the JND 0.02; else a binary search on the OKLCh chroma (scaled by t in
  0..1, L and hue kept) runs while (tmax - tmin) C > 0.0001 (epsilon):
  a candidate in gamut, while no candidate failed it yet, raises tmin;
  else its clipped colour within the JND raises tmin (ending the search
  when JND - E < epsilon) and one beyond it lowers tmax. The answer is the
  last clipped colour (the sample code's).
* color(srgb ...) keeps its parse clamp (Lot 101).

Mixing (CSS Color 5 color-mix(), CSS Color 4 section 12)
* color-mix(in <space> [<hue-method> hue]?, C1 [P1], C2 [P2]) with
  <space> one of srgb (Lot 100), srgb-linear, display-p3, a98-rgb,
  prophoto-rgb, rec2020, lab, oklab, xyz, xyz-d50, xyz-d65, hsl, hwb,
  lch, oklch; <hue-method> shorter (default), longer, increasing or
  decreasing, for hsl, hwb, lch and oklch only.
* Each colour is converted from its linear sRGB values, not gamut-mapped
  (those of a lab(), lch(), oklab(), oklch(), color() other than srgb or
  color-mix() other than srgb and hsl and hwb), else decoded from its
  channels. hsl and hwb are taken from the channels, exactly: L = (M +
  m) / 2, S = (M - m) / min(M + m, 2 - M - m), W = m, B = 1 - M, each
  rounded to 1/10000 %, the hue to 1/10000 deg (half away from zero).
  lch and oklch: C = sqrt(a^2 + b^2), H = atan2(b, a), rounded to
  1/10000 deg into 0..360.
* Powerless hues are missing: hsl and hwb when M = m, lch when C <=
  0.0015, oklch when C <= 0.000004 (the CSS Color 4 sample code).
* Missing components (none) belong to analogous sets: red, green, blue
  (rgb, color() and xyz components), alpha, lightness (L, the hsl l),
  colourfulness (C, the hsl s), hue, opponent a, opponent b; hwb
  whiteness and blackness belong to none. A missing component carries
  into a mix when the space has a component of its set; missing in one
  colour it takes the other's value, missing in both it is 0 and stays
  missing in the result.
* The non-hue components are premultiplied by alpha (the Lot 100 rules
  for weights and alpha). The hue is fixed up by the method (CSS Color 4
  12.4; d = h2 - h1): shorter adds 360 to h1 when d > 180, to h2 when d
  < -180; longer adds 360 to h1 when 0 < d < 180, to h2 when -180 < d <=
  0; increasing adds 360 to h2 when h2 < h1; decreasing to h1 when h1 <
  h2. It is then the weighted mean (not premultiplied), rounded to
  1/10000 deg, modulo 360.
* hsl and hwb results convert back exactly (Lot 98 and Lot 100); the
  others to linear sRGB, then gamut-mapped (oklab, oklch from their OKLab
  values).

The conversions use binary floating point (the C uses fixed point); a
decision within a small tolerance of its threshold is taken both ways,
and the case is "unsure" (Unsure) when the answers differ; a rounding
within MARGIN of its boundary is unsure as in Lot 101.
"""
import math
import os
import sys
from fractions import Fraction as F

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
import lot98_animmodel as M98  # noqa: E402
import lot101_colormodel as L101  # noqa: E402

S = 10000
SP = " \t\n\r"
VAL = 128
Unsure = L101.Unsure
rh = L101.rh
mv = L101.mv

JND = 0.02
EPS = 0.0001
TOL_LIN = 1e-8      # linear sRGB and OKLab L against 0 and 1
TOL_REL = 1e-6      # relative, for squared distances and chroma
# linear sRGB magnitudes beyond which the float model does not follow the
# fixed point (the conversions amplify its rounding): the gamut mapping of
# a computed origin, the operands and result of a mix in a non-srgb space
BIG_MAP = 64.0
BIG_MIX = 8.0


def mm(a, b):
    return L101.mm(a, b)


# CSS Color 4 sample code: linear sRGB to XYZ D65, XYZ to each space
SRGB_TO_XYZ = L101.fl([[F(506752, 1228815), F(87881, 245763), F(12673, 70218)],
                       [F(87098, 409605), F(175762, 245763), F(12673, 175545)],
                       [F(7918, 409605), F(87881, 737289), F(1001167, 1053270)]])
D65_TO_D50 = [[1.0479298208405488, 0.022946793341019088, -0.05019222954313557],
              [0.029627815688159344, 0.990434484573249, -0.01707382502938514],
              [-0.009243058152591178, 0.015055144896577895, 0.7518742899580008]]
XYZ_TO_P3 = L101.fl([[F(446124, 178915), F(-333277, 357830), F(-72051, 178915)],
                     [F(-14852, 17905), F(63121, 35810), F(423, 17905)],
                     [F(11844, 330415), F(-50337, 660830), F(316169, 330415)]])
XYZ_TO_A98 = L101.fl([[F(1829569, 896150), F(-506331, 2785775), F(-308931, 896150)],
                      [F(-851781, 878810), F(1648619, 878810), F(36519, 878810)],
                      [F(16779, 1248040), F(-147721, 1248040), F(1266979, 1248040)]])
XYZ_TO_PRO = [[1.3457868816471583, -0.25557208737979464, -0.05110186497554526],
              [-0.5446307051249019, 1.5082477428451468, 0.02052744743642139],
              [0.0, 0.0, 1.2119675456389452]]
XYZ_TO_2020 = L101.fl([[F(30757411, 17917100), F(-6372589, 17917100), F(-4539589, 17917100)],
                       [F(-19765991, 29648200), F(47925759, 29648200), F(467509, 29648200)],
                       [F(792561, 44930125), F(-1921689, 44930125), F(42328811, 44930125)]])
XYZ_TO_LMS = [[0.8190224379967030, 0.3619062600528904, -0.1288737815209879],
              [0.0329836539323885, 0.9292868615863434, 0.0361446663506424],
              [0.0481771893596242, 0.2642395317527308, 0.6335478284694309]]
LMS_TO_OKLAB = [[0.2104542683093140, 0.7936177747023054, -0.0040720430116193],
                [1.9779985324311684, -2.4285922420485799, 0.4505937096174110],
                [0.0259040424655478, 0.7827717124575296, -0.8086757549230774]]
D50W = L101.D50
SRGB_TO_D50 = mm(D65_TO_D50, SRGB_TO_XYZ)

# the mix spaces: (kind, analogous sets of the three components, hue index)
# kind: "rgb" spaces by their color() name, "lab", "oklab", "hsl", "hwb"
MIX = {
    "srgb-linear": ("srgb-linear", (0, 1, 2), None),
    "display-p3": ("display-p3", (0, 1, 2), None),
    "a98-rgb": ("a98-rgb", (0, 1, 2), None),
    "prophoto-rgb": ("prophoto-rgb", (0, 1, 2), None),
    "rec2020": ("rec2020", (0, 1, 2), None),
    "lab": ("lab", (4, 7, 8), None),
    "oklab": ("oklab", (4, 7, 8), None),
    "xyz": ("xyz", (0, 1, 2), None),
    "xyz-d50": ("xyz-d50", (0, 1, 2), None),
    "xyz-d65": ("xyz-d65", (0, 1, 2), None),
    "hsl": ("hsl", (6, 5, 4), 0),
    "hwb": ("hwb", (6, None, None), 0),
    "lch": ("lab", (4, 5, 6), 2),
    "oklch": ("oklab", (4, 5, 6), 2),
}
HUES = ["shorter", "longer", "increasing", "decreasing"]

# the analogous set of each component of a colour function with none
NONE_SETS = {"rgb(": (0, 1, 2), "rgba(": (0, 1, 2), "hsl(": (6, 5, 4), "hsla(": (6, 5, 4),
             "hwb(": (6, None, None), "lab(": (4, 7, 8), "oklab(": (4, 7, 8), "lch(": (4, 5, 6),
             "oklch(": (4, 5, 6), "color(": (0, 1, 2)}


# ------------------------------------------------------------- decisions
class _Fork(Exception):
    pass


class Oracle:
    """answers a < b, both ways (one per explored path) within tol"""

    def __init__(self, path):
        self.path, self.i = path, 0

    def lt(self, a, b, tol):
        if a < b - tol:
            return True
        if a > b + tol:
            return False
        if self.i < len(self.path):
            self.i += 1
            return self.path[self.i - 1]
        raise _Fork()


def explore(fn):
    """fn(oracle) over every path of near decisions: its answer when they
    agree, else Unsure"""
    out, stack, n = set(), [[]], 0
    while stack:
        path = stack.pop()
        n += 1
        if n > 256:
            raise Unsure("paths")
        try:
            r = fn(Oracle(path))
        except _Fork:
            stack.append(path + [True])
            stack.append(path + [False])
            continue
        out.add(tuple(r))
    if len(out) != 1:
        raise Unsure(out)
    return list(out.pop())


# ------------------------------------------------------------- conversions
def srgb_enc(v):
    a = abs(v)
    r = 12.92 * a if a <= 0.0031308 else 1.055 * a ** (1 / 2.4) - 0.055
    return math.copysign(r, v)


def a98_enc(v):
    return math.copysign(abs(v) ** (256 / 563), v)


def pro_enc(v):
    a = abs(v)
    return math.copysign(a ** (1 / 1.8) if a >= 1 / 512 else 16 * a, v)


def r2020_enc(v):
    al, be = 1.09929682680944, 0.018053968510807
    a = abs(v)
    return math.copysign(al * a ** 0.45 - (al - 1) if a > be else 4.5 * a, v)


def cbrt(v):
    return math.copysign(abs(v) ** (1 / 3), v)


def lin_to(kind, lin):
    """linear sRGB to the components of kind (rgb spaces gamma-encoded,
    lab L a b, oklab L a b)"""
    if kind == "srgb-linear":
        return list(lin)
    if kind in ("xyz", "xyz-d65"):
        return mv(SRGB_TO_XYZ, lin)
    if kind == "xyz-d50":
        return mv(SRGB_TO_D50, lin)
    if kind == "display-p3":
        return [srgb_enc(x) for x in mv(XYZ_TO_P3, mv(SRGB_TO_XYZ, lin))]
    if kind == "a98-rgb":
        return [a98_enc(x) for x in mv(XYZ_TO_A98, mv(SRGB_TO_XYZ, lin))]
    if kind == "prophoto-rgb":
        return [pro_enc(x) for x in mv(XYZ_TO_PRO, mv(SRGB_TO_D50, lin))]
    if kind == "rec2020":
        return [r2020_enc(x) for x in mv(XYZ_TO_2020, mv(SRGB_TO_XYZ, lin))]
    if kind == "oklab":
        return mv(LMS_TO_OKLAB, [cbrt(x) for x in mv(XYZ_TO_LMS, mv(SRGB_TO_XYZ, lin))])
    if kind == "lab":
        xyz = mv(SRGB_TO_D50, lin)
        f = []
        for k in range(3):
            t = xyz[k] / D50W[k]
            f.append(cbrt(t) if t > L101.EPS else (L101.KAPPA * t + 16) / 116)
        return [116 * f[1] - 16, 500 * (f[0] - f[1]), 200 * (f[1] - f[2])]
    raise ValueError(kind)


def to_lin(kind, x):
    if kind == "lab":
        return L101.lab_lin(*x)
    if kind == "oklab":
        return L101.oklab_lin(*x)
    return L101.space_lin(kind, x)


# ------------------------------------------------------------- gamut
def gamut_path(o, lin, okv, exact):
    """the gamut-mapped channels (1/10000 of 0..255) of linear sRGB lin
    along one path of decisions"""
    def ingamut(l):
        return all(not o.lt(x, 0.0, TOL_LIN) and not o.lt(1.0, x, TOL_LIN) for x in l)

    def de2(p, q):
        return sum((p[k] - q[k]) ** 2 for k in range(3))

    def clip_ok(l):
        cl = [min(max(x, 0.0), 1.0) for x in l]
        return cl, lin_to("oklab", cl)

    if ingamut(lin):
        return L101.encode_clip(lin)
    if not exact and max(abs(x) for x in lin) > BIG_MAP:
        raise Unsure("big")
    org = list(okv) if okv else lin_to("oklab", lin)
    # given OKLab values are exact (L >= 1 and L <= 0 as written)
    if (org[0] >= 1.0) if exact else not o.lt(org[0], 1.0, TOL_LIN):
        return [255 * S] * 3
    if (org[0] <= 0.0) if exact else not o.lt(0.0, org[0], TOL_LIN):
        return [0] * 3
    cl, lc = clip_ok(lin)
    j2 = JND * JND
    e = de2(lc, org)
    if o.lt(e, j2, j2 * TOL_REL):
        return L101.encode_clip(cl)
    c2 = org[1] ** 2 + org[2] ** 2
    tmin, tmax, mig, it = 0.0, 1.0, True, 0
    while o.lt(4.0 ** it, c2 * 1e8, 4.0 ** it * TOL_REL):
        t = (tmin + tmax) / 2
        it += 1
        cur = [org[0], org[1] * t, org[2] * t]
        lcur = to_lin("oklab", cur)
        if mig and ingamut(lcur):
            tmin = t
            continue
        cl, lc = clip_ok(lcur)
        e = de2(lc, cur)
        if o.lt(e, j2, j2 * TOL_REL):
            lim = (JND - EPS) ** 2
            if o.lt(lim, e, lim * TOL_REL):
                break
            mig = False
            tmin = t
        else:
            tmax = t
    return L101.encode_clip(cl)


def map_encode(lin, okv=None, exact=True):
    """the channels of linear sRGB lin after gamut mapping (okv: the OKLab
    origin, exact when written as such), Unsure when the paths differ"""
    return explore(lambda o: gamut_path(o, lin, okv, exact and okv is not None))


# ------------------------------------------------------------- colours
def decode_ch(ch):
    return [L101.srgb_curve(c / (255 * S)) for c in ch]


def full(v, depth=0):
    """(channels, alpha, missing bits, linear sRGB or None) of a reader
    colour, or None; the missing bits use the analogous sets"""
    import lot100_colormodel as L100
    import lot102_model as L102
    lv = v.lower()
    if lv.startswith("color-mix("):
        return mix_full(lv, depth)
    miss, text = 0, lv
    f = L102.fn_of(lv)
    if f and lv.endswith(")") and len(lv) > len(f):
        head, rest = L102.split_fn(lv, f)
        got = M98.colour_args(rest)
        if got is not None and "none" in got[0]:
            toks = got[0]
            if got[1]:
                return None
            sets = NONE_SETS[f]
            for k, t in enumerate(toks):
                if t != "none":
                    continue
                if k == 3:
                    miss |= 8
                elif sets[k] is not None:
                    miss |= 1 << sets[k]
            nt = ["0" if t == "none" else t for t in toks]
            text = head + ("" if head == f else " ") + " ".join(nt[:3]) + (" / " + nt[3] if len(nt) == 4 else "") + ")"
    r = L100.rgba_base(text, depth)
    if r is None:
        return None
    lin = None
    if L101.is_space_fn(text):
        if text.startswith("color("):
            g = L101.color_lin(text)
            lin = g[1] if g and g[0] == "lin" else None
        else:
            lin = L101.labish_lin(text)[0]
    return list(r[0]), r[1], miss, lin


def mix_item(t, depth):
    import lot100_colormodel as L100
    toks = L100.split_top(t, False)
    if toks is None or len(toks) > 2:
        return None
    p = None
    c = toks[0]
    if len(toks) == 2:
        q = 0 if toks[0].endswith("%") else 1
        r = L100.num_pct(toks[q])
        if r is None or not r[1] or not 0 <= r[0] <= 100 * S:
            return None
        p, c = r[0], toks[1 - q]
    if len(c) >= VAL:
        return None
    col = full(c, depth + 1)
    if col is None:
        return None
    return col, p


def head_of(m):
    """(space, hue method index) of the color-mix() head, or None"""
    toks = [t for t in m.replace("\t", " ").replace("\n", " ").replace("\r", " ").split(" ") if t]
    if len(toks) not in (2, 4) or toks[0] != "in":
        return None
    sp = toks[1]
    if sp != "srgb" and sp not in MIX:
        return None
    if len(toks) == 2:
        return sp, 0
    if sp == "srgb" or MIX[sp][2] is None or toks[2] not in HUES or toks[3] != "hue":
        return None
    return sp, HUES.index(toks[2])


def mix_full(lv, depth):
    import lot100_colormodel as L100
    import lot102_model as L102
    if depth > 3 or not (lv.startswith("color-mix(") and lv.endswith(")") and len(lv) > 11):
        return None
    parts = L100.split_top(lv[10:-1], True)
    if parts is None or len(parts) != 3:
        return None
    hd = head_of(parts[0])
    if hd is None:
        return None
    if hd[0] == "srgb":
        r = L102.mixm(lv, depth)
        return None if r is None else (list(r[0]), r[1], r[2], None)
    return mix_space(parts, hd, depth)


def hsl_comps(ch, hwb):
    """exact hue (1/10000 deg, None when powerless) and the two others
    (1/10000 %) of channels ch"""
    U = 255 * S
    r, g, b = ch
    mx, mn = max(ch), min(ch)
    d = mx - mn
    if d == 0:
        h = None
    else:
        if mx == r:
            hf = F(600000 * (g - b), d) + (3600000 if g < b else 0)
        elif mx == g:
            hf = F(600000 * (b - r), d) + 1200000
        else:
            hf = F(600000 * (r - g), d) + 2400000
        h = M98.rhalf(hf)
        if h >= 3600000:
            h -= 3600000
    if hwb:
        return h, M98.rhalf(F(mn * 1000000, U)), M98.rhalf(F((U - mx) * 1000000, U))
    den = min(mx + mn, 2 * U - mx - mn)
    s = M98.rhalf(F(d * 1000000, den)) if den > 0 else 0
    return h, s, M98.rhalf(F((mx + mn) * 1000000, 2 * U))


def comps(sp, col):
    """the three components of col in mix space sp (hue in 1/10000 deg as
    an int, None when powerless; hsl and hwb ints in 1/10000 %; others
    floats)"""
    kind, _, hi = MIX[sp]
    if kind in ("hsl", "hwb"):
        return list(hsl_comps(col[0], kind == "hwb"))
    lin = col[3] if col[3] is not None else decode_ch(col[0])
    if max(abs(v) for v in lin) > BIG_MIX:
        raise Unsure("big")
    x = lin_to(kind, lin)
    if hi is None:
        return x
    c = math.hypot(x[1], x[2])
    thr = 0.0015 if sp == "lch" else 0.000004
    if abs(c - thr) <= thr * TOL_REL + 1e-12:
        raise Unsure("powerless")
    h = None
    if c > thr:
        h = rh(math.degrees(math.atan2(x[2], x[1])) * S) % 3600000
    return [x[0], c, h]


def hue_fix(h0, h1, hm):
    d = h1 - h0
    if hm == 0:
        if d > 1800000:
            h0 += 3600000
        elif d < -1800000:
            h1 += 3600000
    elif hm == 1:
        if 0 < d < 1800000:
            h0 += 3600000
        elif -1800000 < d <= 0:
            h1 += 3600000
    elif hm == 2:
        if h1 < h0:
            h1 += 3600000
    elif h0 < h1:
        h0 += 3600000
    return h0, h1


def mix_space(parts, hd, depth):
    sp, hm = hd
    kind, sets, hi = MIX[sp]
    items = []
    for t in parts[1:]:
        it = mix_item(t, depth)
        if it is None:
            return None
        items.append(it)
    (x, p0), (y, p1) = items
    if p0 is None and p1 is None:
        p0 = p1 = 50 * S
    elif p0 is None:
        p0 = 100 * S - p1
    elif p1 is None:
        p1 = 100 * S - p0
    tot = p0 + p1
    if tot <= 0:
        return None
    cv, mi, al = [], [], [x[1], y[1]]
    for col in (x, y):
        c = comps(sp, col)
        m = 0
        if hi is not None and c[hi] is None:
            m |= 1 << hi
            c[hi] = 0
        for j in range(3):
            if sets[j] is not None and (col[2] >> sets[j]) & 1:
                m |= 1 << j
        if (col[2] >> 3) & 1:
            m |= 8
        cv.append(c + [col[1]])
        mi.append(m)
    miss = 0
    for k in range(4):
        m0, m1 = (mi[0] >> k) & 1, (mi[1] >> k) & 1
        if m0 and m1:
            cv[0][k] = cv[1][k] = 0
            if k == 3:
                miss |= 8
            elif sets[k] is not None:
                miss |= 1 << sets[k]
        elif m0:
            cv[0][k] = cv[1][k]
        elif m1:
            cv[1][k] = cv[0][k]
    a0, a1 = cv[0][3], cv[1][3]
    an = a0 * p0 + a1 * p1
    out = [0, 0, 0]
    for k in range(3):
        if k == hi:
            h0, h1 = hue_fix(cv[0][k], cv[1][k], hm)
            out[k] = M98.rhalf(F(h0 * p0 + h1 * p1, tot)) % 3600000
        elif kind in ("hsl", "hwb"):
            out[k] = M98.rhalf(F(cv[0][k] * a0 * p0 + cv[1][k] * a1 * p1, an)) if an else 0
        else:
            out[k] = (cv[0][k] * a0 * p0 + cv[1][k] * a1 * p1) / an if an else 0.0
    alpha = M98.rhalf(F(an, max(tot, 100 * S)))
    if kind in ("hsl", "hwb"):
        h = F(out[0], S)
        u, w = (F(min(max(out[k], 0), 1000000), 1000000) for k in (1, 2))
        if kind == "hsl":
            ch = M98.hsl_rgb(h, u, w)
        else:
            import lot100_colormodel as L100
            ch = L100.hwb_rgb(h, u, w)
        return ch, alpha, miss, None
    if hi is not None:
        r = math.radians(out[2] / S)
        out = [out[0], out[1] * math.cos(r), out[1] * math.sin(r)]
    lin = to_lin(kind, out)
    if max(abs(v) for v in lin) > BIG_MIX:
        raise Unsure("big")
    return map_encode(lin, out if kind == "oklab" else None, False), alpha, miss, lin


def rgba(v):
    r = full(v)
    return None if r is None else (r[0], r[1])
