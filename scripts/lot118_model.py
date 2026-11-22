"""Lot 118 model: the engine's math (CSS Values 4 10) in thousandths, the
math functions and constants, angles and viewport-percentage units.

Written from the specifications (CSS Values 4 6.1.2 viewport-percentage
lengths: 1vw is 1% of the viewport width, vi and vb follow the root's
writing mode, vmin/vmax the smaller/larger axis, the sv*, lv* and dv*
sizes are the plain ones when the viewport has no retractable UI; 10 math
functions: calc, min, max, clamp, round(<strategy>), mod, rem, abs, sign,
pow, sqrt, hypot, log, exp, sin, cos, tan, asin, acos, atan, atan2; 10.7
the constants e, pi, infinity, -infinity, NaN; 10.9 the IEEE-754 results
of the operations at infinity, NaN and zero; 10.10 the types: a product
takes one number, a quotient a number or the same type (typed division
gives a number), a sum the same type) and the engine bounds written in
webeng_math.inc's header: values in thousandths, finite within +-1e9,
three decimals per literal, a font-relative or viewport leaf in
hundredths of a px, products and quotients truncated toward zero, the
used px truncated once.

Exact integer arithmetic for + - * / min max clamp round mod rem abs sign
sqrt hypot and integer powers. sin/cos/tan, the inverse functions, log,
exp and non-integer powers are given by the engine's fixed-point
algorithms ported below (CORDIC in micro-degrees, a base-2 logarithm by
squaring); check() compares that port with Python's floats, and the
generator only asserts those exactly where it agrees with the floats to
within 2 thousandths.
"""
from fractions import Fraction
import re
import math

import lot117_model as L117

MV_MAX = 10 ** 9
INF = 2 * 10 ** 9
NAN = -2 ** 31
LITERAL_MAX = 2000
NODES = 32
DEPTH = 8
ARGS = 8
VALUE_MAX = 1000000
NUMBER, LENGTH, ANGLE = "number", "length", "angle"
VP_UNITS = ("vw", "vh", "vi", "vb", "vmin", "vmax")
VP_ALL = VP_UNITS + tuple(p + u for p in "sld" for u in VP_UNITS)


class Refused(Exception):
    """the engine refuses the expression"""


def tdiv(a, b):
    q = abs(a) // abs(b)
    return q if (a < 0) == (b < 0) else -q


def tmod(a, b):
    return a - tdiv(a, b) * b


def finite(v):
    return v not in (NAN, INF, -INF)


def sat(v):
    return INF if v > MV_MAX else -INF if v < -MV_MAX else v


# ------------------------------------------------------------ viewport
class Ctx(L117.Ctx):
    """a unit context: Lot 117's font context and the viewport (px) and
    whether the root writing mode is vertical"""

    def __init__(self, font100=1600, root100=1600, lh100=None, rlh100=1000, vw=640, vh=480, vertical=False):
        L117.Ctx.__init__(self, font100, root100, lh100, rlh100)
        self.vw, self.vh, self.vertical = vw, vh, vertical

    def but(self, **kw):
        c = Ctx(self.font100, self.root100, self.lh100, self.rlh100, self.vw, self.vh, self.vertical)
        for k, v in kw.items():
            setattr(c, k, v)
        return c


def vp_base100(unit, c):
    u = unit[1:] if unit[:1] in "sld" else unit
    w, h = (c.vh, c.vw) if c.vertical else (c.vw, c.vh)
    return {"vw": c.vw, "vh": c.vh, "vi": w, "vb": h, "vmin": min(c.vw, c.vh), "vmax": max(c.vw, c.vh)}[u]


def split_unit(text):
    """(negative, whole, thousandths, unit) of a number and a font-relative
    or viewport unit, None otherwise"""
    t, neg = text, False
    if t[:1] in ("+", "-"):
        neg, t = t[0] == "-", t[1:]
    i = 0
    while i < len(t) and t[i].isdigit():
        i += 1
    whole_txt, frac_txt = t[:i], ""
    if i < len(t) and t[i] == ".":
        j = i + 1
        while j < len(t) and t[j].isdigit():
            j += 1
        frac_txt, i = t[i + 1:j], j
    if not whole_txt and not frac_txt:
        return None
    unit = t[i:].lower()
    if unit not in L117.UNITS and unit not in VP_ALL:
        return None
    return neg, min(int(whole_txt or "0"), 2000), int((frac_txt + "000")[:3]), unit


def unit100(text, c):
    s = split_unit(text)
    if s is None:
        return None
    neg, whole, frac, unit = s
    b = vp_base100(unit, c) if unit in VP_ALL else L117.base100(unit, c)
    v = whole * b + (frac * b + 500) // 1000
    return -v if neg else v


def unit_px(text, c):
    h = unit100(text, c)
    return None if h is None else L117.round_px(h)


# ------------------------------------------------------------ fixed point
ATAN = [45000000, 26565051, 14036243, 7125016, 3576334, 1789911, 895174, 447614, 223811, 111906, 55953, 27976,
        13988, 6994, 3497, 1749, 874, 437, 219, 109, 55, 27, 14, 7, 3, 2, 1, 0]
K_Q29 = 326016437
EXP2_Q30 = [round(2 ** (2.0 ** -j) * 2 ** 30) for j in range(1, 29)]
LN2_Q30 = 744261118
LOG2E_2X = 774541
DEG_PER_RAD_U = 57295780


def _rotate(z):
    x, y = K_Q29, 0
    for i in range(28):
        if z >= 0:
            x, y, z = x - (y >> i), y + (x >> i), z - ATAN[i]
        else:
            x, y, z = x + (y >> i), y - (x >> i), z + ATAN[i]
    return x, y


def atan2_udeg(y, x):
    z = 0
    if not x and not y:
        return 0
    if x < 0:
        if y >= 0:
            x, y, z = y, -x, 90000000
        else:
            x, y, z = -y, x, -90000000
    m = max(x, abs(y))
    shift = 0
    while (m >> shift) >= 1 << 28:
        shift += 1
    x, y, m = x >> shift, y >> shift, m >> shift
    while m and m < 1 << 27:
        m, x, y = m * 2, x * 2, y * 2
    for i in range(28):
        if y > 0:
            x, y, z = x + (y >> i), y - (x >> i), z + ATAN[i]
        elif y < 0:
            x, y, z = x - (y >> i), y + (x >> i), z - ATAN[i]
    return z


def q29_milli(v):
    return (v * 1000 + (1 << 28)) >> 29


def udeg_milli(z):
    return -((-z + 500) // 1000) if z < 0 else (z + 500) // 1000


def sincos(deg, which):
    a = tmod(deg, 360000)
    if a > 180000:
        a -= 360000
    if a <= -180000:
        a += 360000
    flip = False
    if a > 90000:
        a, flip = a - 180000, True
    elif a < -90000:
        a, flip = a + 180000, True
    c, s = _rotate(a * 1000)
    if flip:
        c, s = -c, -s
    return q29_milli(c if which else s)


def log2_q28(n):
    k = 31
    while not n >> k:
        k -= 1
    y = n << (30 - k) if k <= 30 else n >> (k - 30)
    r = k << 28
    for j in range(1, 29):
        y = (y * y) >> 30
        if y >= 1 << 31:
            y >>= 1
            r += 1 << (28 - j)
    return r


LOG2_1000_Q28 = log2_q28(1000)


def log2_value(v):
    return log2_q28(v) - LOG2_1000_Q28


def exp2_milli(t):
    k = t >> 28
    f = t - (k << 28)
    if k >= 20:
        return INF
    if k <= -12:
        return 0
    v = 1 << 30
    for j in range(1, 29):
        if (f >> (28 - j)) & 1:
            v = (v * EXP2_Q30[j - 1]) >> 30
    m = v * 1000
    if k >= 0:
        m = ((m << k) + (1 << 29)) >> 30
    else:
        sh = 30 - k
        m = (m + (1 << (sh - 1))) >> sh
    return INF if m > MV_MAX else m


def ln_milli(v):
    q = (log2_value(v) * LN2_Q30) >> 30
    return (q * 1000 + (1 << 27)) >> 28


def m_round(strategy, a, b):
    if NAN in (a, b) or b == 0:
        return NAN
    if not finite(a):
        return a if finite(b) else NAN
    if not finite(b):
        if strategy == "up":
            return INF if a > 0 else 0
        if strategy == "down":
            return -INF if a < 0 else 0
        return 0
    step = abs(b)
    lower = tdiv(a, step) * step
    if lower > a:
        lower -= step
    upper = lower if lower == a else lower + step
    if strategy == "up":
        return sat(upper)
    if strategy == "down":
        return sat(lower)
    if strategy == "to-zero":
        return sat(lower if a >= 0 else upper)
    return sat(lower if a - lower < upper - a else upper)


def m_pow(a, b):
    if NAN in (a, b):
        return NAN
    if b == 0:
        return 1000
    if not finite(b):
        if a in (1000, -1000):
            return 1000
        big = not finite(a) or abs(a) > 1000
        return INF if (b > 0) == big else 0
    if not finite(a):
        if b < 0:
            return 0
        if a > 0:
            return INF
        return -INF if tmod(b, 2000) == 1000 else INF
    if b % 1000 == 0 and -64000 <= b <= 64000:
        r = 1000
        for _ in range(abs(b) // 1000):
            r = tdiv(r * a, 1000)
            if abs(r) > MV_MAX:
                r = INF if r > 0 else -INF
                break
        if b > 0:
            return r
        if not r:
            return INF
        if not finite(r):
            return 0
        return sat(tdiv(1000000, r))
    if a < 0:
        return NAN
    if a == 0:
        return 0 if b > 0 else INF
    if abs(b) > 1 << 29:
        return INF if (b > 0) == (a > 1000) else 0
    return exp2_milli(tdiv(log2_value(a) * b, 1000))


def m_log(a, base, has_base):
    if NAN in (a, base):
        return NAN
    if a < 0 or (has_base and base < 0):
        return NAN
    if not has_base:
        if a == 0:
            return -INF
        if a == INF:
            return INF
        return ln_milli(a)
    if not finite(base) or base == 0:
        return NAN
    if a == INF:
        return INF if base > 1000 else -INF
    if a == 0:
        return -INF if base > 1000 else INF
    la, lb = log2_value(a), log2_value(base)
    if not lb:
        return (INF if la > 0 else -INF) if la else NAN
    return sat(tdiv(la * 1000, lb))


def m_exp(a):
    if a == NAN:
        return NAN
    if a == INF:
        return INF
    if a == -INF:
        return 0
    return exp2_milli((a * LOG2E_2X) >> 1)


# ------------------------------------------------------------ nodes
class Node:
    def __init__(self, op, kids=(), value=0, typ=NUMBER, flag=0, pct=False):
        self.op, self.kids, self.value, self.type, self.flag, self.pct = op, list(kids), value, typ, flag, pct


def apply(n, vals, deps, checking):
    a = vals[0] if vals else 0
    b = vals[1] if len(vals) > 1 else a
    op = n.op
    if op in ("+", "-"):
        if NAN in (a, b):
            return NAN
        if op == "-":
            b = -b
        if not finite(a) or not finite(b):
            return NAN if not finite(a) and not finite(b) and a != b else (b if finite(a) else a)
        r = a + b
        if abs(r) > MV_MAX:
            raise Refused("overflow")
        return r
    if op == "*":
        if NAN in (a, b):
            return NAN
        if not finite(a) or not finite(b):
            if not a or not b:
                return NAN
            return -INF if (a < 0) != (b < 0) else INF
        p = tdiv(a * b, 1000)
        if abs(p) > MV_MAX:
            raise Refused("overflow")
        return p
    if op == "/":
        if NAN in (a, b):
            return NAN
        if b == 0:
            if checking and not deps[1]:
                raise Refused("zero")
            return NAN if a == 0 else INF if a > 0 else -INF
        if not finite(a) and not finite(b):
            return NAN
        if not finite(a):
            return -INF if (a < 0) != (b < 0) else INF
        if not finite(b):
            return 0
        q = tdiv(a * 1000, b)
        if abs(q) > MV_MAX:
            raise Refused("overflow")
        return q
    if op in ("min", "max"):
        if NAN in (a, b):
            return NAN
        return min(a, b) if op == "min" else max(a, b)
    if op == "clamp":
        c = vals[2]
        if NAN in (a, b, c):
            return NAN
        return max(a, min(b, c))
    if op == "round":
        return m_round(n.flag, a, b)
    if op in ("mod", "rem"):
        if NAN in (a, b) or not b or not finite(a):
            return NAN
        if not finite(b):
            return NAN if op == "mod" and a and (a < 0) != (b < 0) else a
        r = tmod(a, b)
        if op == "mod" and r and (r < 0) != (b < 0):
            r += b
        return r
    if op == "abs":
        return a if a == NAN else abs(a)
    if op == "sign":
        return a if a == NAN else 1000 if a > 0 else -1000 if a < 0 else 0
    if op == "pow":
        return m_pow(a, b)
    if op == "sqrt":
        if a == NAN or a < 0:
            return NAN
        return INF if a == INF else math.isqrt(a * 1000)
    if op == "hypot2":
        if NAN in (a, b):
            return NAN
        if not finite(a) or not finite(b):
            return INF
        return sat(math.isqrt(a * a + b * b))
    if op == "log":
        return m_log(a, b, n.flag)
    if op == "exp":
        return m_exp(a)
    if op in ("sin", "cos", "tan"):
        if not finite(a):
            return NAN
        d = tdiv(a * DEG_PER_RAD_U, 1000000) if n.flag else a
        d = tmod(d, 360000)
        if op != "tan":
            return sincos(d, op == "cos")
        s, c = sincos(d, 0), sincos(d, 1)
        if not c:
            return -INF if s < 0 else INF
        return sat(tdiv(s * 1000, c))
    if op in ("asin", "acos"):
        if a == NAN or abs(a) > 1000:
            return NAN
        rest = math.isqrt((1000000 - a * a) * 1000000)
        return udeg_milli(atan2_udeg(a * 1000, rest) if op == "asin" else atan2_udeg(rest, a * 1000))
    if op == "atan":
        if a == NAN:
            return NAN
        if not finite(a):
            return 90000 if a > 0 else -90000
        return udeg_milli(atan2_udeg(a, 1000))
    if op == "atan2":
        if NAN in (a, b):
            return NAN
        f = lambda v: v if finite(v) else (MV_MAX if v > 0 else -MV_MAX)
        return udeg_milli(atan2_udeg(f(a), f(b)))
    raise Refused("op")


def evaluate(n, basis, checking=False):
    """(value, depends on a percentage)"""
    if n.op == "num":
        return n.value, False
    if n.op == "%":
        p = tdiv(basis * n.value, 100)
        if abs(p) > MV_MAX:
            raise Refused("overflow")
        return p, True
    got = [evaluate(k, basis, checking) for k in n.kids]
    vals, deps = [g[0] for g in got], [g[1] for g in got]
    return apply(n, vals, deps, checking), any(deps)


# ------------------------------------------------------------ parser
FUNCS = ("calc", "min", "max", "clamp", "round", "mod", "rem", "abs", "sign", "pow", "sqrt", "hypot", "log",
         "exp", "sin", "cos", "tan", "asin", "acos", "atan", "atan2")
WS = " \t\n\r\f"


class Parser:
    def __init__(self, s, ctx):
        self.s, self.i, self.depth, self.count, self.ctx = s, 0, 0, 0, ctx

    def ws(self):
        while self.i < len(self.s) and self.s[self.i] in WS:
            self.i += 1

    def node(self, op, kids=(), value=0, typ=NUMBER, flag=0):
        if self.count >= NODES:
            raise Refused("nodes")
        self.count += 1
        return Node(op, kids, value, typ, flag)

    def digits(self):
        s, i = self.s, self.i
        whole, frac, scale, nd = 0, 0, 1000, 0
        while i < len(s) and s[i].isdigit():
            whole = whole * 10 + int(s[i])
            i += 1
            nd += 1
            if whole > LITERAL_MAX:
                raise Refused("literal")
        if i < len(s) and s[i] == ".":
            i += 1
            fd = 0
            while i < len(s) and s[i].isdigit():
                if scale > 1:
                    scale //= 10
                    frac += int(s[i]) * scale
                i += 1
                fd += 1
            if not fd:
                raise Refused("point")
            nd += fd
        if not nd:
            raise Refused("digits")
        return i, whole * 1000 + frac

    def scientific(self):
        """Lot 118: 1e3, 2.5E-2: the mantissa cut to seven significant
        digits, times 10^exp, in thousandths cut toward zero; None when
        there is no exponent."""
        s, i = self.s, self.i
        m = re.match(r"([0-9.]*)([eE])([+-]?)([0-9]+)", s[i:])
        if not m or not m.group(1):
            return None
        if len(m.group(4)) > 2:
            raise Refused("exponent")
        mant = m.group(1)
        if mant.count(".") > 1 or mant.endswith(".") or not any(c.isdigit() for c in mant):
            raise Refused("mantissa")
        whole, _, frac = mant.partition(".")
        digits = (whole + frac).lstrip("0")
        lead = len(whole + frac) - len(digits)
        kept = digits[:7]
        # the value of the kept digits at their place
        places_after = len(frac)
        total = whole + frac
        kept_full = total[:lead] + kept + "0" * (len(total) - lead - len(kept))
        value = Fraction(int(kept_full or "0"), 10 ** places_after)
        exp = int(m.group(3) + m.group(4))
        milli = value * Fraction(10) ** exp * 1000
        v = int(milli)          # positive: cut toward zero
        if v >= (LITERAL_MAX + 1) * 1000:
            raise Refused("literal")
        self.i = i + m.end()
        return v

    def literal(self, sign):
        s, first = self.s, self.i
        sci = self.scientific()
        if sci is not None:
            end = self.i
            if end < len(s) and s[end] == "%":
                self.i = end + 1
                return self.node("%", value=sci * sign, typ=LENGTH)
            u = end
            while u < len(s) and s[u].isascii() and s[u].isalpha():
                u += 1
            unit = s[end:u].lower()
            self.i = u
            if not unit:
                return self.node("num", value=sci * sign)
            if unit == "px":
                return self.node("num", value=sci * sign, typ=LENGTH)
            raise Refused("exponent unit")
        end, milli = self.digits()
        u = end
        while u < len(s) and s[u].isascii() and s[u].isalpha():
            u += 1
        unit = s[end:u].lower()
        if end < len(s) and s[end] == "%":
            self.i = end + 1
            return self.node("%", value=milli * sign, typ=LENGTH)
        self.i = u
        if not unit:
            return self.node("num", value=milli * sign)
        if unit == "px":
            return self.node("num", value=milli * sign, typ=LENGTH)
        if unit in L117.UNITS or unit in VP_ALL:
            h = unit100(s[first:u], self.ctx)
            if h > LITERAL_MAX * 100:
                raise Refused("unit literal")
            return self.node("num", value=h * 10 * sign, typ=LENGTH)
        if unit == "deg":
            v = milli
        elif unit == "grad":
            v = tdiv(milli * 9, 10)
        elif unit == "turn":
            v = milli * 360
        elif unit == "rad":
            v = tdiv(milli * DEG_PER_RAD_U, 1000000)
        else:
            raise Refused("unit")
        if v > MV_MAX:
            raise Refused("angle")
        return self.node("num", value=v * sign, typ=ANGLE)

    def constant(self, name, sign):
        name = name.lower()
        if name == "infinity" and sign <= 0:
            v = -INF if sign < 0 else INF
        elif sign:
            raise Refused("signed constant")
        elif name in ("e", "pi", "nan"):
            v = {"e": 2718, "pi": 3142, "nan": NAN}[name]
        else:
            raise Refused("name")
        return self.node("num", value=v)

    def strategy(self):
        s, save = self.s, self.i
        self.ws()
        start = end = self.i
        while end < len(s) and (s[end].isascii() and s[end].isalpha() or s[end] == "-"):
            end += 1
        if s[start:end].lower() in ("nearest", "up", "down", "to-zero"):
            self.i = end
            self.ws()
            if self.i < len(s) and s[self.i] == ",":
                self.i += 1
                return s[start:end].lower()
        self.i = save
        return None

    def atom(self):
        s = self.s
        self.ws()
        if self.i >= len(s):
            raise Refused("end")
        sign, had = 1, False
        if s[self.i] in "+-":
            sign, had = (-1 if s[self.i] == "-" else 1), True
            self.i += 1
        if self.i < len(s) and (s[self.i].isdigit() or s[self.i] == "."):
            return self.literal(sign)
        if self.i >= len(s) or self.depth >= DEPTH:
            raise Refused("depth")
        if s[self.i] == "(":
            if sign < 0:
                raise Refused("sign")
            self.depth += 1
            self.i += 1
            r = self.sum()
            self.ws()
            if self.i >= len(s) or s[self.i] != ")":
                raise Refused("paren")
            self.i += 1
            self.depth -= 1
            return r
        name = self.i
        while self.i < len(s) and ((s[self.i].isascii() and s[self.i].isalpha()) or (self.i > name and s[self.i].isdigit())):
            self.i += 1
        if self.i == name:
            raise Refused("atom")
        word = s[name:self.i]
        if self.i >= len(s) or s[self.i] != "(":
            return self.constant(word, sign if had else 0)
        if sign < 0 or word.lower() not in FUNCS:
            raise Refused("function")
        kind = word.lower()
        self.i += 1
        self.depth += 1
        strategy = self.strategy() if kind == "round" else None
        args = []
        while True:
            if len(args) == ARGS:
                raise Refused("args")
            args.append(self.sum())
            self.ws()
            if self.i >= len(s):
                raise Refused("end")
            if s[self.i] != ",":
                break
            self.i += 1
        if s[self.i] != ")":
            raise Refused("close")
        self.i += 1
        r = self.function(kind, args, strategy or "nearest")
        self.depth -= 1
        return r

    def function(self, kind, args, strategy):
        t0 = args[0].type
        for a in args[1:]:
            if (a.type != NUMBER) if kind in ("log", "pow") else (a.type != t0):
                raise Refused("types")
        n = len(args)
        if kind == "calc":
            if n != 1:
                raise Refused("calc")
            return args[0]
        if kind in ("min", "max", "hypot"):
            r = args[0]
            op = "hypot2" if kind == "hypot" else kind
            for a in args[1:]:
                r = self.node(op, (r, a), typ=t0)
            if kind == "hypot" and n == 1:
                r = self.node("abs", (args[0],), typ=t0)
            return r
        if kind == "clamp":
            if n != 3:
                raise Refused("clamp")
            return self.node("clamp", args, typ=t0)
        if kind == "round":
            if n == 1:
                if t0 != NUMBER:
                    raise Refused("round")
                step = self.node("num", value=1000)
            elif n == 2:
                step = args[1]
            else:
                raise Refused("round")
            return self.node("round", (args[0], step), typ=t0, flag=strategy)
        if kind in ("mod", "rem"):
            if n != 2:
                raise Refused(kind)
            return self.node(kind, args, typ=t0)
        if kind == "abs":
            if n != 1:
                raise Refused(kind)
            return self.node(kind, args, typ=t0)
        if kind == "sign":
            if n != 1:
                raise Refused(kind)
            return self.node(kind, args, typ=NUMBER)
        if kind == "pow":
            if n != 2 or t0 != NUMBER:
                raise Refused(kind)
            return self.node(kind, args, typ=NUMBER)
        if kind in ("sqrt", "exp"):
            if n != 1 or t0 != NUMBER:
                raise Refused(kind)
            return self.node(kind, args, typ=NUMBER)
        if kind == "log":
            if t0 != NUMBER or n > 2:
                raise Refused(kind)
            return self.node(kind, args if n == 2 else (args[0], args[0]), typ=NUMBER, flag=int(n == 2))
        if kind in ("sin", "cos", "tan"):
            if n != 1 or t0 == LENGTH:
                raise Refused(kind)
            return self.node(kind, args, typ=NUMBER, flag=int(t0 == NUMBER))
        if kind in ("asin", "acos", "atan"):
            if n != 1 or t0 != NUMBER:
                raise Refused(kind)
            return self.node(kind, args, typ=ANGLE)
        if n != 2:
            raise Refused(kind)
        return self.node("atan2", args, typ=ANGLE)

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
            lt, rt = left.type, right.type
            if op == "*":
                if lt != NUMBER and rt != NUMBER:
                    raise Refused("product type")
                typ = rt if lt == NUMBER else lt
            else:
                if rt != NUMBER and rt != lt:
                    raise Refused("quotient type")
                typ = lt if rt == NUMBER else NUMBER
            left = self.node(op, (left, right), typ=typ)

    def sum(self):
        left = self.product()
        while True:
            save = self.i
            self.ws()
            s = self.s
            if self.i >= len(s) or s[self.i] not in "+-":
                self.i = save
                return left
            if self.i == save or self.i + 1 == len(s) or s[self.i + 1] not in WS:
                raise Refused("operator spacing")
            op = s[self.i]
            self.i += 1
            right = self.product()
            if right.type != left.type:
                raise Refused("sum type")
            left = self.node(op, (left, right), typ=left.type)


def has_pct(n):
    return n.op == "%" or any(has_pct(k) for k in n.kids)


def parse(s, ctx=None, typ=LENGTH):
    """the expression tree of a whole value; Refused when the engine refuses it"""
    p = Parser(s, ctx or Ctx())
    root = p.sum()
    p.ws()
    if p.i != len(s) or root.type != typ:
        raise Refused("trailing or type")
    evaluate(root, 0, checking=True)
    return root


def is_math(v):
    lv = v.lower()
    return any(lv.startswith(f + "(") and len(v) > len(f) for f in FUNCS)


def used(v):
    """a value as a property uses it: NaN 0, the infinities the largest"""
    return 0 if v == NAN else MV_MAX if v == INF else -MV_MAX if v == -INF else v


def px_of(v):
    return 0 if v == NAN else VALUE_MAX if v == INF else -VALUE_MAX if v == -INF else tdiv(v, 1000)


def length_milli(s, ctx=None, basis=0):
    """the thousandths of a px of a length expression (used), None refused"""
    try:
        return used(evaluate(parse(s, ctx), basis)[0])
    except Refused:
        return None


def length_px(s, ctx=None, basis=0):
    """math_eval: whole px (truncated), None refused"""
    try:
        return px_of(evaluate(parse(s, ctx), basis)[0])
    except Refused:
        return None


def number_milli(s, ctx=None):
    try:
        r = parse(s, ctx, NUMBER)
        if has_pct(r):
            return None
        return used(evaluate(r, 0)[0])
    except Refused:
        return None


def angle_milli(s, ctx=None):
    try:
        r = parse(s, ctx, ANGLE)
        if has_pct(r):
            return None
        return used(evaluate(r, 0)[0])
    except Refused:
        return None


def integer(s, lo, hi):
    """math_integer: the nearest integer (halves up) clamped, None refused"""
    if not is_math(s):
        return None
    m = number_milli(s)
    if m is None:
        return None
    r = (m + 500) // 1000 if m >= 0 else -((-m - 500 + 999) // 1000)
    return max(lo, min(hi, r))


# ------------------------------------------------------------ float check
def check(n=4000, seed=118):
    """the fixed-point functions against floats (max error in thousandths)"""
    import random
    rng = random.Random(seed)
    worst = {}

    def note(k, got, want):
        worst[k] = max(worst.get(k, 0), abs(got - want))

    for _ in range(n):
        d = rng.randrange(-720000, 720001)
        note("sin", sincos(d, 0), math.sin(math.radians(d / 1000)) * 1000)
        note("cos", sincos(d, 1), math.cos(math.radians(d / 1000)) * 1000)
        a = rng.randrange(-1000, 1001)
        note("asin", apply(Node("asin"), [a], [False], False), math.degrees(math.asin(a / 1000)) * 1000)
        note("acos", apply(Node("acos"), [a], [False], False), math.degrees(math.acos(a / 1000)) * 1000)
        y, x = rng.randrange(-10 ** 6, 10 ** 6), rng.randrange(-10 ** 6, 10 ** 6)
        note("atan2", udeg_milli(atan2_udeg(y, x)), math.degrees(math.atan2(y, x)) * 1000)
        v = rng.randrange(1, 10 ** 7)
        note("ln", ln_milli(v), math.log(v / 1000) * 1000)
        e = rng.randrange(-8000, 8000)
        note("exp", m_exp(e), math.exp(e / 1000) * 1000)
        b = rng.randrange(-3000, 3000)
        base = rng.randrange(100, 5000)
        want = (base / 1000) ** (b / 1000) * 1000
        if want < 1e6:
            note("pow", m_pow(base, b), want)
    return worst


if __name__ == "__main__":
    print(check())
