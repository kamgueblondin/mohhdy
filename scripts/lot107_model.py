"""Lot 107 model (US-031), written from the specifications, sharing no
code with the C (userspace/wf_sitecss.c, wf_cssprop.c).

- CSS Color 4 12.3 for the legacy sRGB interpolation past its ends: the
  premultiplied channels are divided by the interpolated alpha as it is,
  the alpha is clamped to 0..1 afterwards (ORDER123; the Lot 97 model
  clamped the alpha first).
- Box scrollers (CSS Overflow 3, CSS Box Sizing 3): height and max-height
  with font and viewport units, percentages of the containing block and
  math functions, passed on to the layout.
- view-timeline-inset auto: the scroll container's scroll-padding (CSS
  Scroll-driven Animations 1).
- The body's own style written by the reader view.
"""
from fractions import Fraction as F

S = 10000
ORDER123 = True      # Lot 107: CSS Color 4 12.3 for the legacy path

import re

HMAX = 30000
VW, VH = 640, 480
SP = " \t\n\r"
ABS = {"px": F(1), "in": F(96), "cm": F(9600, 254), "mm": F(960, 254), "q": F(240, 254), "pt": F(4, 3),
       "pc": F(16)}
REL = ("em", "rem", "vw", "vh", "vmin", "vmax", "ex", "ch", "lh")   # Lot 109: ex, ch, lh
TOK = 12            # WF_SITE_TOK


import lot105_model as _M5  # noqa: E402


class Unsure(_M5.Unsure):
    """a value the exact model cannot place against the C's rounding (a
    lot105_model.Unsure, so the Lot 105 and 106 generators leave it out)"""


# ------------------------------------------------------------ lengths
def unit_px(u, em, rem):
    """px per unit u (em and rem in px), None for another unit"""
    if u in ABS:
        return ABS[u]
    return {"em": F(em), "rem": F(rem), "vw": F(VW, 100), "vh": F(VH, 100),
            "vmin": F(min(VW, VH), 100), "vmax": F(max(VW, VH), 100)}.get(u)


NUMRE = re.compile(r"[+-]?(\d+\.?\d*|\.\d+)")


class _P:
    def __init__(self, s, em, rem):
        self.s, self.i, self.em, self.rem = s, 0, em, rem

    def ws(self):
        while self.i < len(self.s) and self.s[self.i] == " ":
            self.i += 1

    def fail(self):
        raise ValueError()

    def func(self):
        for f in ("calc(", "min(", "max(", "clamp("):
            if self.s.startswith(f, self.i):
                self.i += len(f)
                args = [self.expr()]
                self.ws()
                while self.i < len(self.s) and self.s[self.i] == ",":
                    self.i += 1
                    args.append(self.expr())
                    self.ws()
                if self.i >= len(self.s) or self.s[self.i] != ")":
                    self.fail()
                self.i += 1
                if f == "calc(":
                    if len(args) != 1:
                        self.fail()
                    return args[0]
                if any(a[1] != args[0][1] for a in args):
                    self.fail()
                if f == "clamp(":
                    if len(args) != 3:
                        self.fail()
                    lo, v, hi = (a[0] for a in args)
                    return (max(lo, min(v, hi)), args[0][1])
                vals = [a[0] for a in args]
                return ((min if f == "min(" else max)(vals), args[0][1])
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
        m = NUMRE.match(self.s, self.i)
        if not m:
            self.fail()
        n = F(m.group(0))
        self.i = m.end()
        j = self.i
        while j < len(self.s) and self.s[j].isalpha():
            j += 1
        u = self.s[self.i:j]
        self.i = j
        if not u:
            return (n, False)
        k = unit_px(u, self.em, self.rem)
        if k is None:
            self.fail()
        return (n * k, True)

    def term(self):
        v = self.factor()
        while True:
            save = self.i
            self.ws()
            if self.i < len(self.s) and self.s[self.i] in "*/":
                op = self.s[self.i]
                self.i += 1
                w = self.factor()
                if op == "*":
                    if v[1] and w[1]:
                        self.fail()
                    v = (v[0] * w[0], v[1] or w[1])
                else:
                    if w[1] or w[0] == 0:
                        self.fail()
                    v = (v[0] / w[0], v[1])
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
                    if v[1] != w[1] and v[0] != 0 and w[0] != 0:
                        self.fail()
                    v = (v[0] + w[0] if op == "+" else v[0] - w[0], v[1] or w[1])
                    continue
            self.i = save
            return v


def math_px(s, em=16, rem=16):
    """the px of a length or calc()/min()/max()/clamp() of lengths with
    absolute, font and viewport units, None when it does not resolve"""
    try:
        p = _P(s, em, rem)
        v = p.factor()
        p.ws()
        if p.i != len(s):
            return None
    except (ValueError, ZeroDivisionError, IndexError):
        return None
    if not v[1] and v[0] != 0:
        return None
    if abs(v[0]) > 10000:
        return None
    return v[0]


def floor_px(px):
    """floor of px, the C rounding the value to 1/1000 px first"""
    f = px - math_floor(px)
    if f != 0 and (f < F(1, 500) or f > 1 - F(1, 500)):
        raise Unsure()
    return math_floor(px)


def math_floor(x):
    return x.numerator // x.denominator


def num_unit(t):
    i = 1 if t[:1] in ("+", "-") else 0
    j = i
    while j < len(t) and (t[j].isdigit() or t[j] == "."):
        j += 1
    if j == i or not any(c.isdigit() for c in t[i:j]) or t[i:j].count(".") > 1:
        return None
    return t[:j], t[j:]


def height_val(v, mx=False):
    """("px", px + 1), ("tok", kind, text) with kind "%" or "len", ("kept",)
    or None (dropped) for a height (mx: max-height) value"""
    v = v.strip(SP).lower()
    if v in ("initial", "unset"):
        return ("kept",)
    if v in (("none",) if mx else ("auto",)) + ("min-content", "max-content", "fit-content"):
        return ("kept",)
    for f in ("calc(", "min(", "max(", "clamp(", "fit-content("):
        if v.startswith(f) and v.endswith(")") and len(v) > len(f):
            # Lot 109: lot109_model.math_box (ex, ch, lh; a percentage
            # goes to the layout, kind "expr")
            r = _box(v, 16, 16) if f != "fit-content(" else None
            if r is not None:
                return ("tok", "len" if r[0] == "px" else "expr", v)
            return ("kept",)
    nu = num_unit(v)
    if nu is None:
        return None
    n, u = nu
    if u in ABS or (u == "" and F(n) == 0):
        px = F(n) * ABS.get(u, F(1))
        if abs(px) > 100000:
            raise Unsure()
        if px < 0:
            return None
        f = px - math_floor(px)
        if f != 0 and (f < F(1, 1000) or f > 1 - F(1, 1000)):
            raise Unsure()
        return ("px", min(math_floor(px), HMAX) + 1)
    if n.startswith("-"):
        return None
    if u == "%":
        t = n.lstrip("+")
        return ("tok", "%", t) if len(t) + 1 < TOK else ("kept",)
    if u and all("a" <= c <= "z" for c in u):
        if u in REL:
            return ("tok", "len", n.lstrip("+") + u)
        return ("kept",)
    return None


def box_len(slot, em=16, rem=16):
    """(value, pct) a height or max-height slot passes on, or None; slot
    is ("px", n), ("tok", ...) or ("kept",)"""
    if slot[0] == "px":
        return (slot[1] - 1, False)
    if slot[0] != "tok":
        return None
    if slot[1] == "%":
        whole = slot[2].split(".")[0] or "0"
        return (min(int(whole), HMAX), True)
    r = _box(slot[2], em, rem)
    if r is None:
        return None
    if r[0] == "expr":
        return (r[1], "expr")       # Lot 109: the engine's expression
    return (min(floor_px(max(r[1], F(0))), HMAX), False)


def _box(v, em, rem):
    import lot109_model as M9
    return M9.math_box(v, em, rem)


def box_text(h):
    """Lot 109: a box_len value as the reader writes it"""
    return h[0] if h[1] == "expr" else "%d%s" % (h[0], "%" if h[1] else "px")


def sc_cascade(decls):
    """[ox, oy, hpx, hslot, mpx, mslot] of one element from its
    declarations [(rank, prop, value)] (rank: larger wins, a later equal
    one too); hpx/mpx as Lot 105 (absolute px + 1, else 0)"""
    import lot105_model as M5
    best = [None] * 4
    val = [0, 0, ("kept",), ("kept",)]
    for rank, prop, v in decls:
        if prop in ("overflow", "overflow-x", "overflow-y"):
            got = M5.ov_list(prop, v)
            if got is None:
                continue
            for k in (0, 1):
                if got[k] is not None and (best[k] is None or rank >= best[k]):
                    best[k], val[k] = rank, got[k]
        elif prop in ("height", "max-height"):
            got = height_val(v, prop == "max-height")
            if got is None:
                continue
            k = 2 if prop == "height" else 3
            if best[k] is None or rank >= best[k]:
                best[k], val[k] = rank, got
    h, m = val[2], val[3]
    return [val[0], val[1], h[1] if h[0] == "px" else 0, h, m[1] if m[0] == "px" else 0, m]


def passed(nodes, i, scr):
    """the style text the reader adds for node i (a box scroller a
    timeline reads, auto or scroll along y, with a height or max-height it
    passes on), or "" """
    import lot105_model as M5
    sc = nodes[i]["sc"]
    if i not in scr:
        return ""
    em = nodes[i].get("fs", 16)
    rem = nodes[i].get("rem", 16)
    hs = [box_len(sc[3], em, rem), box_len(sc[5], em, rem)] if len(sc) > 3 else \
        [(sc[2] - 1, False) if sc[2] else None, None]
    if hs[0] is None and hs[1] is None:
        return ""
    y = M5.box_scroller(nodes, i)
    if y not in (4, 5):
        return ""
    t = "overflow-y:%s" % M5.OV_NAME[y]
    for name, h in (("height", hs[0]), ("max-height", hs[1])):
        if h is not None:
            t += ";%s:%s" % (name, box_text(h))
    return t


# ------------------------------------------------------------ scroll-padding
SP_LONG = {"scroll-padding-block-start": 0, "scroll-padding-block-end": 1, "scroll-padding-inline-start": 2,
           "scroll-padding-inline-end": 3, "scroll-padding-top": 0, "scroll-padding-bottom": 1,
           "scroll-padding-left": 2, "scroll-padding-right": 3}


def sp_val(v):
    """a scroll-padding side: ("auto",), ("abs", px) for an absolute length,
    a height slot ("tok", ...) or ("kept",) (auto then), None when invalid"""
    if v == "auto":
        return ("auto",)
    nu = num_unit(v)
    if nu is not None and (nu[1] in ABS or (nu[1] == "" and F(nu[0]) == 0)):
        px = F(nu[0]) * ABS.get(nu[1], F(1))
        if px < 0:
            return None
        if px > 100000:
            raise Unsure()
        return ("abs", px)
    if v in ("min-content", "max-content", "fit-content", "none") or v.startswith("fit"):
        return None
    return height_val(v, False)


def split_vals(v):
    out, w, depth = [], "", 0
    for ch in v:
        if ch == " " and not depth:
            if w:
                out.append(w)
            w = ""
            continue
        if ch == "(":
            depth += 1
        elif ch == ")" and depth:
            depth -= 1
        w += ch
    if w:
        out.append(w)
    return out


def sp_decl(prop, v):
    """[(side, slot)] a scroll-padding declaration sets, None (dropped)"""
    n = {"scroll-padding": 4, "scroll-padding-block": 2, "scroll-padding-inline": 2}.get(prop)
    if n is None and prop not in SP_LONG:
        return None
    if v in ("initial", "unset"):
        vals = [("auto",)]
    else:
        ws = split_vals(v)
        if not ws or len(ws) > (n or 1):
            return None
        vals = [sp_val(w) for w in ws]
        if any(x is None for x in vals):
            return None
    if n is None:
        return [(SP_LONG[prop], vals[0])]
    if n == 2:
        base = 2 if prop == "scroll-padding-inline" else 0
        return [(base, vals[0]), (base + 1, vals[1] if len(vals) > 1 else vals[0])]
    t = vals[0]
    r = vals[1] if len(vals) > 1 else t
    b = vals[2] if len(vals) > 2 else t
    lf = vals[3] if len(vals) > 3 else r
    return [(0, t), (1, b), (2, lf), (3, r)]


def sp_cascade(decls):
    """the four scroll-padding sides of an element from [(rank, prop, v)]"""
    best, val = [None] * 4, [("auto",)] * 4
    for rank, prop, v in decls:
        got = sp_decl(prop, v)
        if got is None:
            continue
        for side, slot in got:
            if best[side] is None or rank >= best[side]:
                best[side], val[side] = rank, slot
    return val


def sp_inset(slot, em, rem, view=None):
    """the view timeline inset (lot104_model form) an auto inset takes from
    a scroll-padding side. Lot 111, converted: a math function with a
    percentage is of the scrollport along that axis (view px,
    lot111_model.pct_at), cut toward zero to 1/1000 px, below 0 is 0"""
    if slot[0] == "tok" and slot[1] == "expr" and view is not None:
        import lot111_model as L111
        px = L111.pct_at(slot[2], em, rem, view)
        if px is None:
            return ("len", 0)
        t = max(px, F(0)) * 1000
        if t != math_floor(t):
            raise Unsure()
        return ("len", math_floor(t) * 10)
    if slot[0] == "abs":
        q = slot[1] * 10000
        if q != math_floor(q):
            raise Unsure()
        return ("len", math_floor(q))
    if slot[0] != "tok" or slot[1] == "expr":
        return ("auto", 0)
    if slot[1] == "%":
        return ("pct", math_floor(F(slot[2] or "0") * 10000))
    r = _box(slot[2], em, rem)      # Lot 109: ex, ch and lh resolve
    if r is None:
        return ("auto", 0)
    px = r[1] if r[0] == "px" else F(0)
    if px < 0:
        return ("len", 0)
    t = px * 1000
    if t != math_floor(t):
        raise Unsure()
    return ("len", math_floor(t) * 10)


def html_of(nodes):
    for k, nd in enumerate(nodes):
        if nd["parent"] < 0 and nd["tag"] == "html":
            return k
    return -1


def view_clock(a, nodes, q, axis, ins, c):
    """lot106_model.view_clock with the auto insets taken from the
    scroll-padding of the scroller (the root element for the root)"""
    import lot105_model as M5
    import lot106_model as M6
    ins = list(ins)
    if ins[0][0] == "auto" or ins[1][0] == "auto":
        b = M6.box_of(nodes, q, c.geom)
        if b and not (nodes[q]["parent"] < 0 and nodes[q]["tag"] == "html"):
            s = M5.scroller_of(nodes, q)
            useb = axis == "block" or (axis == "y" and b["bvert"]) or (axis == "x" and not b["bvert"])
            sn = s if s >= 0 else html_of(nodes)
            if sn >= 0:
                sp = nodes[sn].get("sp", [("auto",)] * 4)
                em, rem = nodes[sn].get("fs", 16), nodes[sn].get("rem", 16)
                for e in (0, 1):
                    if ins[e][0] == "auto":
                        ins[e] = sp_inset(sp[(0 if useb else 2) + e], em, rem, b["vp_b"] if useb else b["vp_i"])
    return M6.view_clock(a, nodes, q, axis, tuple(ins), c)


def clock(a, nodes, i, ms, c):
    """lot106_model.clock with this view_clock"""
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
        # Lot 108, converted: the inline or x axis on a box scroller (the
        # root scrolls along y only)
        ax = axis in ("inline", "x")
        if src == "self":
            s = M5.scroll_src(nodes, i)
        elif src == "nearest":
            s = -2 if nd["parent"] < 0 else M5.scroller_of(nodes, i)
        else:
            s = -1
        return None if s < -1 or (ax and s < 0) else M5.scroll_clock(a, s, c, ax, nodes)
    if tl == "none":
        return None
    q = i
    while q >= 0:
        d = M4.defines(nodes, q, tl)
        if d:
            if d[0] == 2:
                return view_clock(a, nodes, q, d[1], d[2], c)
            s = M5.scroll_src(nodes, q)
            ax = d[1] in ("inline", "x")     # Lot 108, converted
            if s < -1 or (ax and s < 0):
                return None
            return M5.scroll_clock(a, s, c, ax, nodes)
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
