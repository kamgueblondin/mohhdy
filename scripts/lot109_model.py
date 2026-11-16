"""Lot 109 model (US-031), written from the specifications, sharing no
code with the C (userspace/wf_cssmath.c, wf_sitecss.c, wf_cssprop.c).

- Box scroller lengths (CSS Values 4, CSS Box Sizing 3): ex, ch and lh
  against the reader's font (the 8x8 font at 16 px: x height 5 px,
  advance 8 px, normal line height 10 px, so 5/16, 8/16 and 10/16 em); a
  percentage inside calc()/min()/max()/clamp() is of the containing
  block, so the value goes to the layout as the engine's math reads it
  (integer literals, at most 2000).
- Relative colours (CSS Color 5): lab(), lch(), hwb() and color() with
  "from", and the channels of an origin outside the sRGB gamut kept
  unclamped.
"""
from fractions import Fraction as F

import lot107_model as L7

LIT = 2000          # the engine's math literal bound (WE_MATH_LITERAL_MAX)
LIMIT = 10000       # px or % a value may reach (WF_CSSMATH_MAX)
FONT = {"ex": F(5, 16), "ch": F(8, 16), "lh": F(10, 16)}


class Unsure(L7.Unsure):
    """a value the exact model cannot place against the C's rounding"""


MATH_ABS = ("px", "pt", "pc")     # the absolute units wf_cssmath.h names


def unit_len(u, em, rem):
    """(px, pct) per unit u, None for another unit"""
    if u in L7.ABS and u not in MATH_ABS:
        return None
    if u == "%":
        return (F(0), F(1))
    if u in FONT:
        return (FONT[u] * em, F(0))
    k = L7.unit_px(u, em, rem)
    return None if k is None else (k, F(0))


class _Box:
    """a recursive descent over a length: values (px, pct, is_length,
    mixed) where mixed is the text of a min()/max()/clamp() comparing px
    with % (only as the whole value)"""

    def __init__(self, s, em, rem):
        self.s, self.i, self.em, self.rem, self.mix = s, 0, em, rem, None
        self.exact = True       # every step a whole number of 1/1000

    def chk(self, *xs):
        for x in xs:
            if (x * 1000).denominator != 1:
                self.exact = False

    def ws(self):
        while self.i < len(self.s) and self.s[self.i] == " ":
            self.i += 1

    def fail(self):
        raise ValueError()

    def args(self):
        out = [self.expr()]
        self.ws()
        while self.i < len(self.s) and self.s[self.i] == ",":
            self.i += 1
            out.append(self.expr())
            self.ws()
        if self.i >= len(self.s) or self.s[self.i] != ")":
            self.fail()
        self.i += 1
        return out

    def func(self):
        for f in ("calc(", "min(", "max(", "clamp("):
            if not self.s.startswith(f, self.i):
                continue
            self.i += len(f)
            a = self.args()
            if f == "calc(":
                if len(a) != 1:
                    self.fail()
                return a[0]
            if f == "clamp(" and len(a) != 3:
                self.fail()
            if any(x[3] for x in a) or any(x[2] != a[0][2] for x in a):
                self.fail()
            px = any(x[0] for x in a)
            pc = any(x[1] for x in a)
            if px and pc:
                # a percentage of an unknown (non-negative) basis against px
                if not a[0][2] or self.mix is not None:
                    self.fail()
                self.mix = f + ", ".join(lin_text(x[0], x[1]) for x in a) + ")"
                return (F(0), F(0), True, True)
            key = (lambda x: x[1]) if pc else (lambda x: x[0])
            if f == "clamp(":
                lo, v, hi = a
                v = hi if key(hi) < key(v) else v
                return lo if key(lo) > key(v) else v
            best = a[0]
            for x in a[1:]:
                if (key(x) < key(best)) if f == "min(" else (key(x) > key(best)):
                    best = x
            return best
        return None

    def factor(self):
        self.ws()
        r = self.func()
        if r is not None:
            return r
        if self.i < len(self.s) and self.s[self.i] == "(":
            self.i += 1
            r = self.expr()
            self.ws()
            if self.i >= len(self.s) or self.s[self.i] != ")":
                self.fail()
            self.i += 1
            return r
        m = L7.NUMRE.match(self.s, self.i)
        if not m:
            self.fail()
        n = F(m.group(0))
        self.i = m.end()
        j = self.i
        while j < len(self.s) and (self.s[j].isalpha() or self.s[j] == "%"):
            j += 1
            if self.s[j - 1] == "%":
                break
        u = self.s[self.i:j]
        self.i = j
        if not u:
            return (n, F(0), False, False)
        k = unit_len(u, self.em, self.rem)
        if k is None:
            self.fail()
        if u in FONT:
            self.chk(n * self.em)       # the C takes n em first
        self.chk(n * k[0], n * k[1])
        return (n * k[0], n * k[1], True, False)

    def term(self):
        v = self.factor()
        while True:
            save = self.i
            self.ws()
            if self.i < len(self.s) and self.s[self.i] in "*/":
                op = self.s[self.i]
                self.i += 1
                w = self.factor()
                if v[3] or w[3]:
                    self.fail()
                if op == "*":
                    if v[2] and w[2]:
                        self.fail()
                    k, x = (w[0], v) if not w[2] else (v[0], w)
                    v = (x[0] * k, x[1] * k, v[2] or w[2], False)
                else:
                    if w[2] or w[0] == 0:
                        self.fail()
                    v = (v[0] / w[0], v[1] / w[0], v[2], False)
                self.chk(v[0], v[1])
            else:
                self.i = save
                return v

    def expr(self):
        v = self.term()
        while True:
            save = self.i
            if self.i < len(self.s) and self.s[self.i] == " ":
                self.ws()
                if self.i < len(self.s) and self.s[self.i] in "+-" and self.s[self.i + 1:self.i + 2] == " ":
                    op = self.s[self.i]
                    self.i += 1
                    w = self.term()
                    if v[2] != w[2] or v[3] or w[3]:
                        self.fail()
                    sg = 1 if op == "+" else -1
                    v = (v[0] + sg * w[0], v[1] + sg * w[1], v[2], False)
                    continue
            self.i = save
            return v


def round_int(x):
    """x rounded half away from zero (the C rounds 1/1000 first)"""
    a = abs(x)
    f = a - L7.math_floor(a)
    if abs(f - F(1, 2)) < F(1, 100):
        raise Unsure()
    r = L7.math_floor(a + F(1, 2))
    if r > LIT:
        raise ValueError()
    return -r if x < 0 and r else r


def lin_text(px, pc):
    """"N%", "Npx" or "calc(N% + Mpx)" (the engine's form)"""
    if not pc:
        return "%dpx" % round_int(px)
    t = "%d%%" % round_int(pc)
    if not px:
        return t
    q = round_int(px)
    return "calc(%s %s %dpx)" % (t, "-" if px < 0 else "+", abs(q))


def math_box(s, em=16, rem=16, exact=False):
    """("px", px) for a box length without a percentage, ("expr", text)
    for one with a percentage, None when it does not resolve; exact: px
    must be what the C gets to 1/1000 (Unsure when a step is cut)"""
    try:
        p = _Box(s.lower(), em, rem)
        v = p.factor()
        p.ws()
        if p.i != len(s):
            return None
        if not v[2] and v[0] != 0:
            return None
        if p.mix is not None:
            return ("expr", p.mix)
        if abs(v[0]) > LIMIT or abs(v[1]) > LIMIT:
            return None
        if v[1]:
            return ("expr", lin_text(v[0], v[1]))
        if exact and not p.exact:
            raise Unsure("cut")
        return ("px", v[0])
    except (ValueError, ZeroDivisionError, IndexError):
        return None


def engine_px(text, basis):
    """the px the layout gives a box length the reader wrote (a px value
    or math_box's expression) against a containing block of basis px: a
    percentage is basis * N / 100 in whole px (toward zero, the engine's
    integer math), min()/max()/clamp() as CSS has them; a negative result
    is 0"""
    def val(t):
        t = t.strip()
        for f in ("min(", "max(", "clamp(", "calc("):
            if t.startswith(f):
                inner, args, d, cur = t[len(f):-1], [], 0, ""
                for ch in inner:
                    if ch == "," and not d:
                        args.append(cur)
                        cur = ""
                        continue
                    d += ch == "("
                    d -= ch == ")"
                    cur += ch
                args.append(cur)
                if f == "calc(":
                    a, op, b = args[0].split(" ")
                    return val(a) + (val(b) if op == "+" else -val(b))
                xs = [val(a) for a in args]
                if f == "clamp(":
                    return max(xs[0], min(xs[1], xs[2]))
                return min(xs) if f == "min(" else max(xs)
        if t.endswith("%"):
            n = int(t[:-1])
            q = abs(basis * n) // 100
            return q if basis * n >= 0 else -q
        return int(t[:-2])
    return max(0, val(text))


# ------------------------------------------------------------ relative colours
REL_FN = ("rgb", "hsl", "oklab", "oklch", "hwb", "lab", "lch", "color")
REL_KW = {"rgb": ("r", "g", "b"), "hsl": ("h", "s", "l"), "oklab": ("l", "a", "b"), "oklch": ("l", "c", "h"),
          "hwb": ("h", "w", "b"), "lab": ("l", "a", "b"), "lch": ("l", "c", "h")}
# the spaces of color() (CSS Color 4 10): rgb ones take r g b, xyz ones x y z
COLOR_SP = ("srgb", "srgb-linear", "display-p3", "a98-rgb", "prophoto-rgb", "rec2020", "xyz", "xyz-d50", "xyz-d65")
SPC = " \t\n\r"
S7 = 10000


def rel_head(v):
    """(function, index after "from" and its space), else None"""
    lv = v.lower()
    for k, f in enumerate(REL_FN):
        if not lv.startswith(f):
            continue
        i = len(f) + (1 if k < 2 and lv[len(f):len(f) + 1] == "a" else 0)
        if lv[i:i + 1] != "(":
            continue
        i += 1
        while i < len(v) and v[i] in SPC:
            i += 1
        if lv[i:i + 4] == "from" and i + 5 < len(v) and v[i + 4] in SPC:
            return f, i + 5
        return None
    return None


def rel_split(v, i, n_args):
    """the origin and the arguments from v[i] (n_args before the "/"), or
    None"""
    toks, slash, n = [], -1, len(v)
    while True:
        while i < n - 1 and v[i] in SPC:
            i += 1
        if i >= n - 1:
            break
        if v[i] == "/":
            if slash >= 0:
                return None
            slash = len(toks)
            i += 1
            continue
        if len(toks) >= n_args + 1:
            return None
        a, d = i, 0
        while i < n - 1 and (d or (v[i] not in SPC and v[i] != "/")):
            if v[i] == "(":
                d += 1
            elif v[i] == ")":
                if not d:
                    return None
                d -= 1
            elif not d and v[i] == ",":
                return None
            i += 1
        if d:
            return None
        toks.append(v[a:i])
    if not ((len(toks) == n_args and slash < 0) or (len(toks) == n_args + 1 and slash == n_args)):
        return None
    return toks


def _srgb(lin, scale):
    """the sRGB-encoded values of lin (1/10000 of 0..scale); a value too
    near a rounding boundary is both candidates (lo, hi), which rel_rgba
    explores"""
    import lot101_colormodel as L101
    import lot103_model as L103
    out = []
    for x in lin:
        y = L103.srgb_enc(x) * scale * S7
        try:
            out.append(L101.rh(y))
        except L101.Unsure:
            lo = int(y // 1)
            out.append((lo, lo + 1))
    return out


def hsl_spec(ch, hwb):
    """hue (1/10000 deg, 0 when powerless) and saturation and lightness,
    or whiteness and blackness (1/10000 %), of channels ch (1/10000 of
    0..255, unclamped), CSS Color 4 sample code (rgbToHsl, rgbToHwb): a
    negative saturation turns the hue by 180 deg and is made positive"""
    from fractions import Fraction as F
    import lot98_animmodel as M98
    U = 255 * S7
    r, g, b = ch
    mx, mn = max(ch), min(ch)
    d = mx - mn
    h = 0
    sat = F(0)
    if d:
        if mx == r:
            hf = F(600000 * (g - b), d) + (3600000 if g < b else 0)
        elif mx == g:
            hf = F(600000 * (b - r), d) + 1200000
        else:
            hf = F(600000 * (r - g), d) + 2400000
        h = M98.rhalf(hf)
        if h >= 3600000:
            h -= 3600000
        den = min(mx + mn, 2 * U - mx - mn)       # 2 min(light, 1 - light)
        if den:
            sat = F(d * 1000000, den)
        if sat < 0:
            h = (h + 1800000) % 3600000
            sat = -sat
    if hwb:
        return h, M98.rhalf(F(mn * 1000000, U)), M98.rhalf(F((U - mx) * 1000000, U))
    return h, M98.rhalf(sat), M98.rhalf(F((mx + mn) * 1000000, 2 * U))


def rel_values(f, sp, text):
    """the keyword values (1/10000) of function f (sp: the color() space)
    for the absolute origin text, alpha last; None when it is invalid.
    rgb: the channels, or for an origin outside the sRGB gamut its linear
    values sRGB-encoded unclamped (0..255), hsl and hwb from those then
    (hsl_spec); color(): the origin's values
    in sp (srgb encoded from its linear values unclamped, else from the
    channels); the others as color-mix() takes them (lot103_model.comps:
    hsl and hwb from the channels, a powerless hue 0)"""
    import lot101_colormodel as L101
    import lot103_model as L103
    import lot108_model as L8
    col = L103.full(text)
    if col is None:
        return None
    ch, al, lin = col[0], col[1], col[3]
    if f in ("rgb", "hsl", "hwb"):
        if lin is not None and any(x < -1e-9 or x > 1 + 1e-9 for x in lin):
            ch = _srgb(lin, 255)
            if f == "rgb":
                return ch + [al]
            if any(isinstance(c, tuple) for c in ch):
                raise Unsure("unclamped channel rounding")
            h, a, b = hsl_spec(ch, f == "hwb")
            return [h, a, b, al]
        if lin is not None and any(x < 1e-9 or x > 1 - 1e-9 for x in lin):
            # at the gamut's edge: the same values either way, else unsure
            if _srgb(lin, 255) != list(ch):
                raise Unsure("gamut edge")
        if f == "rgb":
            return [ch[0], ch[1], ch[2], al]
    if f in ("hsl", "oklab", "oklch"):
        return L8.rel_values(REL_FN.index(f), text)
    if f == "color" and sp == "srgb":
        if lin is not None:
            return _srgb(lin, 1) + [al]
        return [L101.rh(c / 255) for c in ch] + [al]
    name = sp if f == "color" else f
    x = L103.comps(name, col)
    if f == "hwb":
        return [x[0] or 0, x[1], x[2], al]
    if f == "lch":
        return [L101.rh(x[0] * S7), L101.rh(x[1] * S7), x[2] or 0, al]
    return [L101.rh(v * S7) for v in x] + [al]


def rel_arg(kws, t, kv):
    """one argument as the absolute colour gets it, or None (lot108_model
    rel_arg with the keywords kws)"""
    import lot108_model as L8
    save = L8.REL_KW[0]
    L8.REL_KW[0] = kws
    try:
        return L8.rel_arg(0, t, kv)
    finally:
        L8.REL_KW[0] = save


def rel_text(v, depth=0, pick=None):
    """the absolute colour a relative one is, None when invalid,
    NotImplemented when v is no relative colour; raises Unsure. pick: the
    choices for the ambiguous values of the outermost origin (_srgb), in
    order; rel_rgba explores them"""
    import lot95_propmodel as P
    h = rel_head(v)
    if h is None:
        return NotImplemented
    if depth > 3 or not v.endswith(")"):
        return None
    f, i = h
    toks = rel_split(v, i, 5 if f == "color" else 4)
    if toks is None:
        return None
    sp = None
    if f == "color":
        sp = toks.pop(1).lower()
        if sp not in COLOR_SP:
            return None
        kws = ("x", "y", "z") if sp.startswith("xyz") else ("r", "g", "b")
    else:
        kws = REL_KW[f]
    org = toks[0]
    if rel_head(org) is not None:
        org = rel_text(org, depth + 1)
        if org is None:
            return None
    kv = rel_values(f, sp, org)
    if kv is None:
        return None
    amb = [k for k in range(4) if isinstance(kv[k], tuple)]
    if amb and (depth or pick is None or len(pick) != len(amb)):
        raise _Amb(len(amb))
    for k, c in zip(amb, pick or ()):
        kv[k] = kv[k][c]
    args = []
    for t in toks[1:]:
        a = rel_arg(kws, t, kv)
        if a is None:
            return None
        args.append(a)
    if len(args) == 3:
        args.append(P.fmt(kv[3], ""))
    return "%s(%s%s %s %s / %s)" % ((f, sp + " " if sp else "") + tuple(args))


class _Amb(Unsure):
    """values of the origin near a rounding boundary (their count)"""


def rel_rgba(v):
    """(channels in 1/10000 of 0..255, alpha in 1/10000) of a relative
    colour, None when invalid; raises Unsure. Values of the outermost
    origin near a rounding boundary are tried both ways: the same
    painted colour (8-bit channels and alpha) each way, else Unsure"""
    import itertools
    import lot103_model as L103

    def one(pick):
        t = rel_text(v, 0, pick)
        if t is None or t is NotImplemented:
            return None
        r = L103.full(t)
        return None if r is None else (r[0], r[1])
    try:
        return one(None)
    except _Amb as e:
        n = e.args[0]
    got = [one(p) for p in itertools.product((0, 1), repeat=n)]

    def painted(r):
        return None if r is None else (tuple((c + S7 // 2) // S7 for c in r[0]), (r[1] * 255 + S7 // 2) // S7)
    if len(set(painted(r) for r in got)) != 1:
        raise Unsure("rounding")
    return got[0]


# ---- engine heights past 2000 px (CSS 2.1 10.5, 10.7; webeng bounds) ----
LAYOUT_MAX = 16000      # webeng.h WE_LAYOUT_MAX: px heights and positions
PCT_MAX = 2000          # a percentage literal


def height_decl(v):
    """('px', N), ('pct', N), ('auto', 0) or None (refused) for one
    height, min-height or max-height value (integers only)"""
    import re
    if v == "auto":
        return ("auto", 0)       # height only (lot109_gen)
    m = re.fullmatch(r"(\d+)(px|%)", v)
    if not m:
        return None
    n = int(m.group(1))
    if m.group(2) == "%":
        return ("pct", n) if n <= PCT_MAX else None
    return ("px", n) if n <= LAYOUT_MAX else None


def tall_layout(divs):
    """(h, y) of empty block boxes stacked in an auto-height body: each
    a list of (property, value) in cascade order; a refused value keeps
    the earlier one; a percentage of the auto body is auto; h is
    max(min-height, min(height or 0, max-height)); y the sum of the
    heights before, at most LAYOUT_MAX"""
    out, y = [], 0
    for decls in divs:
        got = {}
        for p, v in decls:
            d = height_decl(v)
            if d is not None:
                got[p] = d
        val = got.get("height", ("auto", 0))
        h = val[1] if val[0] == "px" else 0
        mx = got.get("max-height")
        if mx and mx[0] == "px":
            h = min(h, mx[1])
        mn = got.get("min-height")
        if mn and mn[0] == "px":
            h = max(h, mn[1])
        out.append((h, min(y, LAYOUT_MAX)))
        y += h
    return out
