"""Lot 116 model: the engine's line-height (a number inherited as a
number, px with hundredths, em and percentages of the element's own font,
1..400 px used), the engine's math bounds raised (32 nodes, 8 functions
or parentheses deep, 8 arguments), the body's percentage width against
html's content width (the viewport less html's margins and padding) and
var() invalid at computed-value time unset for the box lengths, the
overflow, the timelines, scroll-padding and the animation lists.

Written from the specifications (CSS Inline 3 4.2, CSS Values 4 6.1.1
and 10, CSS 2 10.2, 10.3.3 and 10.5, CSS Variables 1 3.1, CSS Cascade 5
7.3), sharing no code with the C. The engine side follows the bounds
documented in webeng.h: integer arithmetic (a percentage is the basis
times it over 100, cut toward zero; a division is cut toward zero), whole
literals up to 2000, intermediate values up to 1000000, used sizes up to
16000; the glyph cell of a font of f px is scaled by q = (f + 2) / 4
(4..16) and the normal line is (10 q + 2) / 4 px.
"""
import contextlib

import lot111_model as L111
import lot112_model as L112

Unsure = L111.Unsure

# ------------------------------------------------------------ line-height
LH_MAX = 400              # WE_LINE_H_MAX: px
LH_NUM_MAX = 40           # WE_LH_NUMBER_MAX: a number, an em multiple
FONT_DEFAULT = 16


def milli(t):
    """a plain decimal as thousandths (digits, at most one point, the
    first three decimals kept, cut), None when it is not one"""
    if not t or t.count(".") > 1 or any(c not in "0123456789." for c in t) or t == ".":
        return None
    w, _, f = t.partition(".")
    if w and int(w) > 100000:
        return None
    return int(w or "0") * 1000 + int((f + "000")[:3])


def lh_parse(v):
    """the engine's reading of one line-height value: ("n",) normal,
    ("num", h) a number in hundredths, ("em", h) an em multiple (a
    percentage over 100) in hundredths, ("px", px, frac); None refused
    (a negative value, a unit it does not read, past the bounds)"""
    if v == "normal":
        return ("n",)
    kind, t = "num", v
    if v.endswith("px"):
        kind, t = "px", v[:-2]
    elif v.endswith("em"):
        kind, t = "em", v[:-2]
    elif v.endswith("%"):
        kind, t = "pct", v[:-1]
    m = milli(t)
    if m is None:
        return None
    bound = {"px": LH_MAX * 1000, "pct": LH_NUM_MAX * 100000}.get(kind, LH_NUM_MAX * 1000)
    if m > bound:
        return None
    h = (m + 500) // 1000 if kind == "pct" else (m + 5) // 10
    if kind == "px":
        return ("px", h // 100, h % 100 if h else 1)
    return ("num" if kind == "num" else "em", h or 1)


def lh_computed(decls, parent, font_px):
    """an element's computed line height: decls its line-height values in
    order (a refused one is left out, the last that reads wins), parent
    the parent's computed value (None the root: normal); an em multiple is
    px of the element's own font (at most 400 px), a number stays a
    number (CSS Inline 3 4.2)"""
    c = parent if parent is not None else ("n",)
    for v in decls:
        r = lh_parse(v)
        if r is not None:
            c = r
    if c[0] == "em":
        h = min(c[1] * font_px, LH_MAX * 100)
        c = ("px", h // 100, h % 100)
    return c


def lh_fields(c):
    """(line_height, lh_unitless, lh_frac) of webeng.h for a computed value"""
    if c[0] == "n":
        return (0, 0, 0)
    if c[0] == "num":
        return (c[1], 1, 0)
    return (c[1], 0, c[2])


def normal_line(font_px):
    q = min(max((font_px + 2) // 4, 4), 16)
    return (10 * q + 2) // 4


def lh_used100(c, font_px):
    """the used line height in hundredths of a px (1..400 px)"""
    if c[0] == "n" or (c[0] == "px" and c[1] <= 0 and not c[2]):
        return normal_line(font_px) * 100
    v = c[1] * font_px if c[0] == "num" else c[1] * 100 + c[2]
    return min(max(v, 100), LH_MAX * 100)


def lines_h(used100, n):
    """the height of n lines of plain text: each line the whole px, the
    hundredths carried until they make a px"""
    return n * (used100 // 100) + n * (used100 % 100) // 100


def line_tops(used100, n):
    """the offsets of the n lines from the first"""
    return [lines_h(used100, k) for k in range(n)]


# ------------------------------------------------------------ engine math
NODES, DEPTH, ARGS = 32, 8, 8     # WE_MATH_NODES_MAX, WE_MATH_DEPTH_MAX, WE_MATH_ARGS_MAX
LITERAL_MAX, VALUE_MAX, LAYOUT_MAX = 2000, 1000000, 16000


class Refused(Exception):
    """the engine refuses the declaration"""


def _cdiv(a, b):
    q = abs(a) // abs(b)
    return q if (a < 0) == (b < 0) else -q


def _checked(a, b, op):
    if abs(a) > VALUE_MAX or abs(b) > VALUE_MAX:
        raise Refused("operand")
    if op == "*":
        if b and abs(a) > VALUE_MAX // abs(b):
            raise Refused("product")
        r = a * b
    elif op == "/":
        if not b:
            raise Refused("zero")
        r = _cdiv(a, b)
    else:
        r = a + b if op == "+" else a - b
    if abs(r) > VALUE_MAX:
        raise Refused("result")
    return r


class _Parse:
    """CSS Values 4 math (calc(), min(), max(), clamp(), parentheses) of
    whole numbers, px and percentages, read into the engine's node list:
    a literal, a sum, a difference, a product, a quotient and each
    min()/max() step (n - 1 of them) and a clamp() take a node; calc()
    and parentheses take none"""

    def __init__(self, s):
        self.s, self.i, self.depth, self.nodes = s, 0, 0, []

    def ws(self):
        while self.i < len(self.s) and self.s[self.i] in " \t\n\r\f":
            self.i += 1

    def node(self, n):
        if len(self.nodes) >= NODES:
            raise Refused("nodes")
        self.nodes.append(n)
        return n

    def atom(self):
        self.ws()
        s = self.s
        if self.i >= len(s):
            raise Refused("end")
        sign = 1
        if s[self.i] in "+-":
            sign = -1 if s[self.i] == "-" else 1
            self.i += 1
        j = self.i
        if j < len(s) and s[j].isdigit():
            while self.i < len(s) and s[self.i].isdigit():
                self.i += 1
            v = int(s[j:self.i])
            if v > LITERAL_MAX:
                raise Refused("literal")
            unit = ""
            if s.startswith("%", self.i):
                unit, self.i = "%", self.i + 1
            elif s[self.i:self.i + 2].lower() == "px":
                unit, self.i = "px", self.i + 2
            return self.node(("lit", sign * v, unit))
        if sign < 0 or j >= len(s) or self.depth >= DEPTH:
            raise Refused("depth or sign")
        self.depth += 1
        if s[self.i] == "(":
            self.i += 1
            r = self.sum()
            self.ws()
            if self.i >= len(s) or s[self.i] != ")":
                raise Refused("paren")
            self.i += 1
        else:
            k = self.i
            while self.i < len(s) and s[self.i].lower() in "abcdefghijklmnopqrstuvwxyz":
                self.i += 1
            name = s[k:self.i].lower()
            if name not in ("calc", "min", "max", "clamp") or self.i >= len(s) or s[self.i] != "(":
                raise Refused("function")
            self.i += 1
            args = []
            while True:
                if len(args) == ARGS:
                    raise Refused("args")
                a = self.sum()
                if args and dim(a) != dim(args[0]):
                    raise Refused("dimension")
                args.append(a)
                self.ws()
                if self.i >= len(s):
                    raise Refused("end")
                if s[self.i] != ",":
                    break
                self.i += 1
            if s[self.i] != ")" or (name == "calc" and len(args) != 1) or (name == "clamp" and len(args) != 3):
                raise Refused("close")
            self.i += 1
            if name == "calc":
                r = args[0]
            elif name == "clamp":
                r = self.node(("clamp", args[0], args[1], args[2]))
            else:
                r = args[0]
                for a in args[1:]:
                    r = self.node((name, r, a))
        self.depth -= 1
        return r

    def product(self):
        left = self.atom()
        while True:
            save = self.i
            self.ws()
            if self.i >= len(self.s) or self.s[self.i] not in "*/":
                self.i = save
                return left
            op = self.s[self.i]
            self.i += 1
            right = self.atom()
            if (op == "*" and dim(left) and dim(right)) or (op == "/" and dim(right)):
                raise Refused("product dimension")
            left = self.node((op, left, right))

    def sum(self):
        left = self.product()
        while True:
            save = self.i
            self.ws()
            if self.i >= len(self.s) or self.s[self.i] not in "+-":
                self.i = save
                return left
            # binary + and - need white space on both sides
            if self.i == save or self.i + 1 >= len(self.s) or self.s[self.i + 1] not in " \t\n\r\f":
                raise Refused("sign spacing")
            op = self.s[self.i]
            self.i += 1
            right = self.product()
            if dim(left) != dim(right):
                raise Refused("sum dimension")
            left = self.node((op, left, right))


def dim(n):
    """1 for a length (px or %), 0 for a number"""
    if n[0] == "lit":
        return 1 if n[2] else 0
    if n[0] in ("*", "/"):
        return dim(n[1]) or dim(n[2])
    return dim(n[1])


def ev(n, basis):
    if n[0] == "lit":
        return _cdiv(basis * n[1], 100) if n[2] == "%" else n[1]
    if n[0] == "clamp":
        a, b, c = ev(n[1], basis), ev(n[2], basis), ev(n[3], basis)
        return max(a, min(b, c))
    a, b = ev(n[1], basis), ev(n[2], basis)
    if n[0] == "min":
        return min(a, b)
    if n[0] == "max":
        return max(a, b)
    return _checked(a, b, n[0])


def math_parse(s):
    """(root, node count) of a length declaration value the engine takes;
    Refused when it does not (past a bound, a type error, a zero
    denominator or an overflow at a 0 basis)"""
    p = _Parse(s)
    root = p.sum()
    p.ws()
    if p.i != len(s) or not dim(root):
        raise Refused("trailing or number")
    ev(root, 0)
    return root, len(p.nodes)


def math_width(s, basis, fallback):
    """(used width, node count) of width: s in a containing block basis px
    wide: the value (at least 0, at most 16000); the fallback (auto, the
    containing block) when it overflows at this basis; count 0 and None
    when the declaration is refused"""
    try:
        root, count = math_parse(s)
    except Refused:
        return None, 0
    try:
        v = ev(root, basis)
    except Refused:
        return fallback, count
    return min(max(v, 0), LAYOUT_MAX), count


@contextlib.contextmanager
def bounds_lot112():
    """the reader's mirrors of the engine's bounds before Lot 116 (24
    nodes, 4 functions deep, 4 arguments, a 64-byte expression)"""
    old = (L111.NODES, L111.FN_DEPTH, L111.ARGS, L111.POOL, L111.BOXEX)
    L111.NODES, L111.FN_DEPTH, L111.ARGS, L111.POOL, L111.BOXEX = 24, 4, 4, 24, 64
    try:
        yield
    finally:
        L111.NODES, L111.FN_DEPTH, L111.ARGS, L111.POOL, L111.BOXEX = old


# ------------------------------------------------- the body's % width
VW, HMAX = 640, 10000      # the viewport's width, WF_SITE_HMAX


def box_px(v):
    """margin or padding as the reader keeps them: one to four whole px
    (px or no unit, at most 255 each, top right bottom left as CSS
    expands them); None when a value is anything else (the declaration
    is dropped)"""
    parts = v.split()
    if not 1 <= len(parts) <= 4:
        return None
    out = []
    for t in parts:
        d = t[:-2] if t.endswith("px") else t
        if not d.isdigit():
            return None
        out.append(min(int(d), 255))
    t, r, b, l = (out * 4)[:4] if len(out) == 1 else (
        [out[0], out[1], out[0], out[1]] if len(out) == 2 else
        [out[0], out[1], out[2], out[1]] if len(out) == 3 else out)
    return [t, r, b, l]


def body_pct_width(pct, margin, padding):
    """the body's percentage width (its whole part) against html's content
    width when html's width is auto (CSS 2 10.3.3: the viewport less
    html's left and right margins and padding), cut, at most 10000"""
    hv = max(VW - margin[1] - margin[3] - padding[1] - padding[3], 0)
    return min(hv * pct // 100, HMAX)


# ------------------------------------------- var() invalid at computed time
def unset_or_value(text, env, valid):
    """lot112_model.unset_or_value: the substituted text when the property
    takes it, else unset (CSS Variables 1 3.1); Lot 116 for the properties
    outside the reader's kinds too"""
    return L112.unset_or_value(text, env, valid)


# ------------------------------------------------ font-relative units
# CSS Values 4 6.1.1: ex is the x height, cap the cap height, ch the
# advance of "0", ic the advance of the ideograph U+6C34 (1em when the
# font has none, 6.1.1.1); rex, rcap, rch and ric are those of the root
# element's font. The engine draws one 8x8 font scaled by q (the font px
# rounded half up, q = (px + 2) / 4 within 4..16, each glyph pixel
# (p * q + 2) / 4 px): "x" has 5 inked rows, the capitals 7, the cell is
# 8 wide; it has no ideograph.
ROWS = {"ex": 5, "cap": 7, "ch": 8}


def font_q(f_milli):
    px = f_milli // 1000 + (1 if f_milli % 1000 >= 500 else 0)
    return min(max((px + 2) // 4, 4), 16)


def unit_milli(unit, em_milli, rem_milli):
    """the size of one unit in px thousandths against the element's font
    (em_milli) and the root's (rem_milli)"""
    root = unit.startswith("r") and unit != "rem"
    base = unit[1:] if root else unit
    f = rem_milli if root else em_milli
    if base == "ic":
        return f
    q = font_q(f)
    return (ROWS[base] * q + 2) // 4 * 1000


def len_milli(text, em_milli, rem_milli):
    """a length N<unit> (N up to three decimals) in px thousandths, cut
    toward zero as the reader's fixed point does"""
    for u in ("rcap", "rex", "rch", "ric", "cap", "ex", "ch", "ic"):
        if text.endswith(u):
            n = milli(text[:-len(u)])
            if n is None:
                raise Unsure(text)
            return n * unit_milli(u, em_milli, rem_milli) // 1000
    raise Unsure(text)


def px_text(m):
    """thousandths as a px length the reader reads back exactly"""
    s = "%d.%03d" % (m // 1000, m % 1000)
    s = s.rstrip("0").rstrip(".")
    return s + "px"
