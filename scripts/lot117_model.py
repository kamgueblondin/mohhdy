"""Lot 117 model: font-relative lengths in the engine.

The engine reads em of the element's own font (Lot 116 and before: em was
10 px wherever a length was read), rem of the root element's font, ex, ch,
cap and ic of the element's 8x8 glyph scale, lh of its line height and rlh
of the root's, in every length it reads, in its math (calc(), min(),
max(), clamp()), in font-size (against the parent) and in line-height
(lh against the parent's line height); the font size keeps hundredths of a
px; a number line height is the number times that font size; line-clamp
and the column steps count the carried fraction of a fractional line
height.

Written from the specifications (CSS Values 4 6.1.1 "font-relative
lengths": em is the computed font-size of the element, in font-size the
parent's; rem and rlh are of the root element, their initial values in
the root's own font-size and line-height; lh is the computed line-height,
in line-height the parent's; ex, ch, cap and ic are the font metrics, ic
falling back to 1em when the font has no ideograph; CSS Fonts 4 2.5
font-size, the keyword table and the smaller/larger ratio 1.2; CSS Inline
3 4.2 line-height; CSS Values 4 10 math functions; CSS Overflow 4 line-clamp),
sharing no code with the C. The engine side follows the bounds documented
in webeng.h: the glyph cell of a font of f px is scaled by q = (f + 2) / 4
(4..16) and a glyph row count r is (r q + 2) / 4 px (x height 5 rows,
cell 8 columns, capitals 7 rows, normal line 10 rows); the number before a
unit keeps three decimals (cut) and its whole part at most 2000; a length
is hundredths of a px rounded half away from zero to px within +-2000; a
font size is 4..64 px in hundredths; a line height 1..400 px.
"""
import lot116_model as L116

Refused = L116.Refused
UNITS = ("em", "rem", "ex", "ch", "cap", "ic", "lh", "rlh")
FONT_DEFAULT = 16
FONT_MIN, FONT_MAX = 4, 64
LH_MAX, LH_NUM_MAX = L116.LH_MAX, L116.LH_NUM_MAX
KEYWORDS = {"xx-small": 9, "x-small": 10, "small": 13, "medium": 16, "large": 18, "x-large": 24, "xx-large": 32,
            "xxx-large": 48}


def q_of(px):
    return min(max((px + 2) // 4, 4), 16)


def rows(r, q):
    return (r * q + 2) // 4


def font_px(f100):
    """the rounded font size (font_px of webeng.h)"""
    return (f100 + 50) // 100


def normal100(f100):
    return rows(10, q_of(font_px(f100))) * 100


class Ctx:
    """a unit context in hundredths of a px: the font (em, ic, and the
    glyph scale of ex, ch, cap), the root font (rem), the line height (lh)
    and the root line height (rlh)"""

    def __init__(self, font100=1600, root100=1600, lh100=None, rlh100=1000):
        self.font100, self.root100, self.rlh100 = font100, root100, rlh100
        self.lh100 = normal100(font100) if lh100 is None else lh100

    def but(self, **kw):
        c = Ctx(self.font100, self.root100, self.lh100, self.rlh100)
        for k, v in kw.items():
            setattr(c, k, v)
        return c


def base100(unit, c):
    q = q_of(font_px(c.font100))
    return {"em": c.font100, "ic": c.font100, "rem": c.root100, "ex": rows(5, q) * 100, "ch": rows(8, q) * 100,
            "cap": rows(7, q) * 100, "lh": c.lh100, "rlh": c.rlh100}[unit]


def split_unit(text):
    """(negative, whole, thousandths, unit) of a number followed by one of
    the units, None otherwise"""
    t, neg = text, False
    if t[:1] in ("+", "-"):
        neg, t = t[0] == "-", t[1:]
    i = 0
    while i < len(t) and t[i].isdigit():
        i += 1
    whole_txt = t[:i]
    frac_txt = ""
    if i < len(t) and t[i] == ".":
        j = i + 1
        while j < len(t) and t[j].isdigit():
            j += 1
        frac_txt, i = t[i + 1:j], j
    if not whole_txt and not frac_txt:
        return None
    unit = t[i:].lower()
    if unit not in UNITS:
        return None
    whole = min(int(whole_txt or "0"), 2000)
    return neg, whole, int((frac_txt + "000")[:3]), unit


def unit100(text, c):
    """hundredths of a px of a font-relative length, None when it is not one"""
    s = split_unit(text)
    if s is None:
        return None
    neg, whole, frac, unit = s
    b = base100(unit, c)
    v = whole * b + (frac * b + 500) // 1000
    return -v if neg else v


def round_px(h):
    r = -((-h + 50) // 100) if h < 0 else (h + 50) // 100
    return max(-2000, min(2000, r))


def unit_px(text, c):
    h = unit100(text, c)
    return None if h is None else round_px(h)


# ------------------------------------------------------------ math leaves
class _UParse(L116._Parse):
    """the Lot 116 engine math with font-relative leaves (decimals allowed,
    px rounded at the element); px and plain numbers stay whole"""

    def __init__(self, s, ctx):
        L116._Parse.__init__(self, s)
        self.ctx = ctx

    def atom(self):
        self.ws()
        s, i0 = self.s, self.i
        if i0 < len(s):
            j = i0 + (1 if s[i0] in "+-" else 0)
            sign = -1 if s[i0] == "-" else 1
            k = j
            while k < len(s) and (s[k].isdigit() or s[k] == "."):
                k += 1
            u = k
            while u < len(s) and s[u].isalpha():
                u += 1
            if k > j and s[k:u].lower() in UNITS:
                if s[j:k].count(".") > 1:
                    raise Refused("number")
                h = unit100(s[j:u], self.ctx)
                if h is None:
                    raise Refused("unit")
                px = (h + 50) // 100
                if px > L116.LITERAL_MAX:
                    raise Refused("literal")
                self.i = u
                return self.node(("lit", sign * px, "px"))
            if k > j and "." in s[j:k]:
                raise Refused("decimal")
        return L116._Parse.atom(self)


def math_parse(s, ctx):
    p = _UParse(s, ctx)
    root = p.sum()
    p.ws()
    if p.i != len(s) or not L116.dim(root):
        raise Refused("trailing or number")
    L116.ev(root, 0)
    return root


def math_px(s, ctx, basis):
    """the value at this percentage basis, None when refused"""
    try:
        return L116.ev(math_parse(s, ctx), basis)
    except Refused:
        return None


def is_math(v):
    lv = v.lower()
    return any(lv.startswith(f + "(") for f in ("calc", "min", "max", "clamp"))


# ------------------------------------------------------------ font-size
def _decimal_milli(t):
    return L116.milli(t)


def font_size100(v, parent):
    """an element's font-size value against its parent context: hundredths
    of a px within 4..64 px, None when refused (and the parent's kept)"""
    v = v.strip(" \t")
    lv = v.lower()
    p = parent.font100
    if not v:
        return None
    if lv in KEYWORDS:
        return KEYWORDS[lv] * 100
    if lv == "smaller":
        h = (p * 5 + 3) // 6
    elif lv == "larger":
        h = (p * 6 + 2) // 5
    elif is_math(v):
        px = math_px(v, parent, font_px(p))
        if px is None or px <= 0:
            return None
        h = min(px, FONT_MAX) * 100
    elif split_unit(v) is not None:
        h = unit100(v, parent)
    elif lv.endswith("px") or lv.endswith("pt"):
        m = _decimal_milli(v[:-2])
        if m is None:
            return None
        if m > 100000:
            h = 10000
        else:
            h = (m + 5) // 10 if lv.endswith("px") else (m * 4 // 3 + 5) // 10
    elif lv.endswith("%"):
        m = _decimal_milli(v[:-1])
        if m is None:
            return None
        m = min((m + 5) // 10, 160000)
        h = (p * m + 5000) // 10000
    else:
        return None
    if h < 50:
        return None
    return max(FONT_MIN * 100, min(FONT_MAX * 100, h))


def font_cascade(decls, parent):
    f = parent.font100
    for d in decls:
        r = font_size100(d, parent)
        if r is not None:
            f = r
    return f


# ------------------------------------------------------------ line-height
def _unit_tail(v):
    """the unit after the number at the end of v, None when none"""
    i = len(v)
    while i > 0 and v[i - 1].isalpha():
        i -= 1
    if 0 < i < len(v) and (v[i - 1].isdigit() or v[i - 1] == ".") and v[i:].lower() in UNITS:
        return v[i:].lower()
    return None


def lh_parse(v, ctx, parent_lh100):
    """("n",), ("num", h), ("em", h) or ("px", px, frac); None refused.
    ctx is the element's (its font known), lh the parent's line height"""
    if v.lower() == "normal":
        return ("n",)
    if is_math(v) or _unit_tail(v) not in (None, "em"):
        c = ctx.but(lh100=parent_lh100)
        if is_math(v):
            px = math_px(v, c, font_px(ctx.font100))
            h = None if px is None else px * 100
        else:
            h = unit100(v, c)
        if h is None or h < 0 or h > LH_MAX * 100:
            return None
        return ("px", h // 100, h % 100 if h else 1)
    return L116.lh_parse(v.lower())     # units and keywords are ASCII case-insensitive


def lh_computed(decls, parent_c, ctx, parent_lh100):
    c = parent_c if parent_c is not None else ("n",)
    for v in decls:
        r = lh_parse(v, ctx, parent_lh100)
        if r is not None:
            c = r
    if c[0] == "em":
        h = min((c[1] * ctx.font100 + 50) // 100, LH_MAX * 100)
        c = ("px", h // 100, h % 100)
    return c


def lh_used100(c, f100):
    if c[0] == "n" or (c[0] == "px" and c[1] <= 0 and not c[2]):
        return normal100(f100)
    v = (c[1] * f100 + 50) // 100 if c[0] in ("num", "em") else c[1] * 100 + c[2]
    return min(max(v, 100), LH_MAX * 100)


lh_fields = L116.lh_fields
lines_h = L116.lines_h
line_tops = L116.line_tops


# ------------------------------------------------------------ documents
class Elem:
    """one element of a chain html > body > ... : its font-size and
    line-height declarations in order; computed font100, line height and
    the context its other declarations use"""

    def __init__(self, fs=(), lh=()):
        self.fs, self.lh = list(fs), list(lh)


def chain(elems):
    """resolve html (elems[0]) and its descendants in order; each gets
    .font100, .lhc (computed line height), .lh100 (used) and .ctx"""
    root100, rlh100 = 1600, normal100(1600)
    parent_f, parent_c, parent_lh = 1600, None, normal100(1600)
    for k, e in enumerate(elems):
        pctx = Ctx(parent_f, root100, parent_lh, rlh100)
        e.font100 = font_cascade(e.fs, pctx)
        ectx = Ctx(e.font100, root100, parent_lh, rlh100)
        e.lhc = lh_computed(e.lh, parent_c, ectx, parent_lh)
        e.lh100 = lh_used100(e.lhc, e.font100)
        if k == 0:
            root100, rlh100 = e.font100, e.lh100
        e.ctx = Ctx(e.font100, root100, e.lh100, rlh100)
        parent_f, parent_c, parent_lh = e.font100, e.lhc, e.lh100
    return elems
