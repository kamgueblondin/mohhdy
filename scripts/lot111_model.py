"""Lot 111 model (US-031), written from the specifications, sharing no
code with the C (userspace/wf_cssprop.c, wf_sitecss.c, wf_cssmath.c).

- Relative colours with missing origin components (CSS Color 5, 4.1 and
  4.2): the origin's missing components are carried forward to the
  analogous components of the relative function's space (CSS Color 4,
  12.2: reds r and x, greens g and y, blues b and z, lightness, colorfulness
  (chroma and saturation), hue, opponent a, opponent b); a channel keyword
  of such a component is none, so the result has it missing too; inside
  a math function none is 0; a left-out alpha is the origin's, missing
  included.
"""
import lot102_model as L102
import lot103_model as L103
import lot107_model as L7
import lot109_model as L9

CARRY = [True]      # False: the origin's missing components are 0 (before Lot 111; the generator's contrast)

_ORIG_FULL = L103.full
_ORIG_RGBAM = L102.rgbam

# analogous sets of each function's channels (CSS Color 4, 12.2), with
# the bits lot103_model.full uses for them: 0 reds, 1 greens, 2 blues,
# 4 lightness, 5 colorfulness, 6 hue, 7 opponent a, 8 opponent b
REL_SETS = {"rgb": (0, 1, 2), "hsl": (6, 5, 4), "hwb": (6, None, None), "lab": (4, 7, 8), "oklab": (4, 7, 8),
            "lch": (4, 5, 6), "oklch": (4, 5, 6), "color": (0, 1, 2)}


class Unsure(L9.Unsure):
    """a value the exact model cannot place against the C's rounding"""


def rel_miss(f, origin_miss):
    """the channels (bits 0..2) and alpha (bit 3) of function f missing
    for an origin with missing bits origin_miss"""
    out = 0
    for k, s in enumerate(REL_SETS[f]):
        if s is not None and (origin_miss >> s) & 1:
            out |= 1 << k
    if origin_miss & 8:
        out |= 8
    return out


def rel_text(v, depth=0):
    """the absolute colour (none where a component is missing) relative
    colour v is, None when invalid, NotImplemented when v is no relative
    colour; raises Unsure (an origin value near a rounding boundary too)"""
    import lot95_propmodel as P
    h = L9.rel_head(v)
    if h is None:
        return NotImplemented
    if depth > 3 or not v.endswith(")"):
        return None
    f, i = h
    toks = L9.rel_split(v, i, 5 if f == "color" else 4)
    if toks is None:
        return None
    sp = None
    if f == "color":
        sp = toks.pop(1).lower()
        if sp not in L9.COLOR_SP:
            return None
        kws = ("x", "y", "z") if sp.startswith("xyz") else ("r", "g", "b")
    else:
        kws = L9.REL_KW[f]
    org = toks[0]
    if L9.rel_head(org) is not None:
        org = rel_text(org, depth + 1)
        if org is None:
            return None
    col = full(org, depth + 1)
    if col is None:
        return None
    mk = rel_miss(f, col[2]) if CARRY[0] else 0
    try:
        kv = L9.rel_values(f, sp, org)
    except L9._Amb:
        raise Unsure("origin rounding")
    if kv is None:
        return None
    if any(isinstance(x, tuple) for x in kv):
        raise Unsure("origin rounding")
    for k in range(4):
        if (mk >> k) & 1:
            kv[k] = 0           # none in a math function
    args = []
    for t in toks[1:]:
        lt = t.lower()
        kw = kws.index(lt) if lt in kws else 3 if lt == "alpha" else -1
        if kw >= 0 and (mk >> kw) & 1:
            args.append("none")
            continue
        a = L9.rel_arg(kws, t, kv)
        if a is None:
            return None
        args.append(a)
    if len(args) == 3:
        args.append("none" if mk & 8 else P.fmt(kv[3], ""))
    return "%s(%s%s %s %s / %s)" % ((f, sp + " " if sp else "") + tuple(args))


def full(v, depth=0):
    """lot103_model.full with relative colours (rel_text)"""
    if L9.rel_head(v) is not None:
        t = rel_text(v, depth)
        if t is None or t is NotImplemented:
            return None
        return _ORIG_FULL(t, depth)
    return _ORIG_FULL(v, depth)


def rgbam(v, depth=0):
    """lot102_model.rgbam (the srgb color-mix() items) with relative
    colours (rel_text)"""
    if L9.rel_head(v) is not None:
        r = full(v, depth)
        return None if r is None else (list(r[0]), r[1], r[2])
    return _ORIG_RGBAM(v, depth)


def install():
    """color-mix() items of lot102_model and lot103_model read relative
    colours here"""
    L103.full = full
    L102.rgbam = rgbam


def rgba(v):
    """(channels 1/10000 of 0..255, alpha 1/10000) or None"""
    install()
    r = full(v)
    return None if r is None else (r[0], r[1])


# ------------------------------------------------------------- opacity
# Opacity (CSS Color 4, 3.2) is <alpha-value>: a math function of a
# number or of a percentage is one too (CSS Values 4, 10: calc(),
# min(), max(), clamp(); a number and a percentage do not add, a
# percentage times a number is a percentage); var() is substituted
# first (CSS Variables 1, 3: the element's inherited custom property, or
# the fallback); a value invalid after substitution is dropped (the
# reader's Lot 89 rule for var(), see docs/lot111_plan.md); all sets
# opacity too (CSS Cascade 5, 3.2: every property but direction and
# unicode-bidi); revert rolls the author origin back (no user-agent
# opacity: 1), revert-layer the cascade layer (CSS Cascade 5, 7.3).
from fractions import Fraction as _F
import re as _re

_NUM = _re.compile(r"[+-]?(\d+(\.\d+)?|\.\d+)%?")


class _Bad(Exception):
    pass


class _Calc:
    """a math function of numbers and percentages: (value, is_pct)"""

    def __init__(self, s):
        self.s, self.i = s, 0

    def ws(self):
        while self.i < len(self.s) and self.s[self.i] == " ":
            self.i += 1

    def eat(self, t):
        self.ws()
        if self.s.startswith(t, self.i):
            self.i += len(t)
            return True
        return False

    def factor(self):
        self.ws()
        for fn in ("calc(", "min(", "max(", "clamp("):
            if self.s.startswith(fn, self.i):
                self.i += len(fn)
                args = [self.sum()]
                while self.eat(","):
                    args.append(self.sum())
                if not self.eat(")"):
                    raise _Bad()
                if fn == "calc(":
                    if len(args) != 1:
                        raise _Bad()
                    return args[0]
                if fn == "clamp(" and len(args) != 3:
                    raise _Bad()
                if len(set(a[1] for a in args)) != 1:
                    raise _Bad()
                vs = [a[0] for a in args]
                if fn == "min(":
                    return (min(vs), args[0][1])
                if fn == "max(":
                    return (max(vs), args[0][1])
                return (max(vs[0], min(vs[1], vs[2])), args[0][1])
        if self.eat("("):
            v = self.sum()
            if not self.eat(")"):
                raise _Bad()
            return v
        m = _NUM.match(self.s, self.i)
        if not m:
            raise _Bad()
        self.i = m.end()
        t = m.group(0)
        return (_F(t[:-1]), True) if t.endswith("%") else (_F(t), False)

    def prod(self):
        v = self.factor()
        while True:
            self.ws()
            if self.i < len(self.s) and self.s[self.i] in "*/":
                op = self.s[self.i]
                self.i += 1
                w = self.factor()
                if op == "*":
                    if v[1] and w[1]:
                        raise _Bad()
                    v = (v[0] * w[0], v[1] or w[1])
                else:
                    if w[1] or w[0] == 0:
                        raise _Bad()
                    v = (v[0] / w[0], v[1])
            else:
                return v

    def sum(self):
        v = self.prod()
        while True:
            self.ws()
            # + and - need spaces around them (CSS Values 4, 10.1)
            if self.s.startswith(" + ", self.i - 1) or self.s.startswith(" - ", self.i - 1):
                op = self.s[self.i]
                self.i += 1
                w = self.prod()
                if v[1] != w[1]:
                    raise _Bad()
                v = (v[0] + w[0] if op == "+" else v[0] - w[0], v[1])
            else:
                return v


def op_math(text):
    """the opacity (1/10000, clamped) of a math function text, None when
    it is none of a number or a percentage; raises Unsure when its value
    is not exact at four decimals (the reader's canonical text)"""
    if not any(text.startswith(f) for f in ("calc(", "min(", "max(", "clamp(")):
        return None
    c = _Calc(text)
    try:
        v, pct = c.factor()
    except (_Bad, ZeroDivisionError):
        return None
    c.ws()
    if c.i != len(text):
        return None
    if (v * 10000).denominator != 1 or abs(v) > 100000:
        raise Unsure("math rounding")
    if pct:
        v /= 100
    v = min(max(v, _F(0)), _F(1))
    return int(v * 10000)


def subst(text, env):
    """text with its var() replaced (CSS Variables 1), None when one is
    invalid (no value and no fallback)"""
    out, i = "", 0
    while True:
        j = text.find("var(", i)
        if j < 0:
            return out + text[i:]
        out += text[i:j]
        d, k = 0, j + 4
        while k < len(text) and (d or text[k] != ")"):
            d += text[k] == "("
            d -= text[k] == ")"
            k += 1
        inner = text[j + 4:k]
        name, _, fb = inner.partition(",")
        name, fb = name.strip(), fb.strip() if _ else None
        val = env.get(name)
        if val is None:
            if fb is None:
                return None
            val = subst(fb, env)
            if val is None:
                return None
        out += val
        i = k + 1


def op_value(text, env):
    """op_value_lot111 converted by Lot 112: a value with var() that is
    invalid after substitution (no value and no fallback, or no opacity)
    is unset (1), invalid at computed-value time (CSS Variables 1 3.1)"""
    got = op_value_lot111(text, env)
    if got is None and "var(" in text:
        return (1, 10000)
    return got


def op_value_lot111(text, env):
    """("val", code, value) of an opacity value after var(): code 1
    initial/unset, 2 a value, 3 inherit, 4 revert, 5 revert-layer; None
    when dropped"""
    import lot110_model as L10
    t = subst(text, env) if "var(" in text else text
    if t is None:
        return None
    t = t.strip()
    if t == "revert":
        return (4, 0)
    if t == "revert-layer":
        return (5, 0)
    got = L10.op_decl(t)
    if got is not None:
        return got
    n = op_math(t)
    return None if n is None else (2, n)


def all_value(text):
    """the opacity code of an all value, None when it is invalid"""
    return {"initial": (1, 10000), "unset": (1, 10000), "inherit": (3, 10000), "revert": (4, 0),
            "revert-layer": (5, 0)}.get(text)


def layer_step(imp, layer, nlayers):
    """CSS Cascade 5, 6.4: normal: later layers win, unlayered last;
    important: the reverse"""
    if not imp:
        return layer if layer else nlayers + 1
    return nlayers + 1 - layer if layer else 0


def op_cascade(decls, nlayers):
    """(code, value) of the winning opacity of one element from decls
    [(imp, layer, spec, order, code, value)] (codes as op_value, the
    dropped ones left out), revert and revert-layer rolled back;
    (0, 10000) when none"""
    keep = list(decls)
    for _round in range(nlayers + 4):
        best = None
        for d in keep:
            r = (d[0], layer_step(d[0], d[1], nlayers), d[2], d[3])
            if best is None or r >= best[0]:
                best = (r, d)
        if best is None:
            return (0, 10000)
        code = best[1][4]
        if code == 4:
            return (0, 10000)       # no user-agent opacity
        if code == 5:
            top = best[0][:2]
            keep = [d for d in keep if (d[0], layer_step(d[0], d[1], nlayers)) < top]
            continue
        return (code, best[1][5])
    raise Unsure("rounds")


# ------------------------------------------------------------- line-height and lh
# line-height (CSS Inline 3, 4.2; CSS 2 10.8.1): normal | <number [0,inf]>
# | <length-percentage [0,inf]>; computed: a number as specified (it
# inherits as a number, times each element's font size), a percentage
# times the element's font size, a length absolute, normal as is. lh
# (CSS Values 4, 6.1.1) is the element's computed line height (normal:
# the reader's font metric, 10/16 em); in line-height itself the
# parent's (the root's: the initial normal at 16 px). The reader keeps
# 1/1000 of a px, cut toward zero at each step (numbers and
# percentages to 1/1000 as written); at most 10000 px.
LH_MAX = 10 ** 7     # WF_CSSMATH_MAX in thousandths
HMAX = 30000         # WF_SITE_HMAX
VW = 640             # the test viewport width


def lh_normal(f):
    """normal's line height (px thousandths) for font size f (thousandths)"""
    return f * 10 // 16


def lh_px(comp, f):
    k, v = comp
    if k == 1:
        return min(f * v // 1000, LH_MAX)
    if k == 2:
        return v
    return lh_normal(f)


def _milli(t):
    """a non-negative decimal text in 1/1000 (cut), None when it is not one"""
    if not _re.fullmatch(r"\+?(\d+(\.\d+)?|\.\d+)", t):
        return None
    v = _F(t.lstrip("+"))
    return int(v * 1000)


def box_len_lh(s, f, root, lh):
    """math_box of s with em f, rem root (thousandths) and lh px
    thousandths (None: normal): px thousandths or None; Unsure when the
    C would cut a step"""
    save = L9.FONT["lh"]
    try:
        if lh is not None:
            L9.FONT["lh"] = _F(lh, 1000) / _F(f, 1000)
        r = L9.math_box(s, _F(f, 1000), _F(root, 1000), True)
    finally:
        L9.FONT["lh"] = save
    if r is None or r[0] != "px":
        return None
    v = r[1] * 1000
    if v.denominator != 1:
        raise Unsure("cut")
    return int(v)


def lh_num(t):
    """Lot 112: the value (1/1000, cut; below 0 is 0) of a math function
    of numbers, None when it is not one; Unsure past 1/1000 (the reader
    reads wf_cssprop_math_eval's canonical text) or when large"""
    if not any(t.startswith(f) for f in ("calc(", "min(", "max(", "clamp(")):
        return None
    c = _Calc(t)
    try:
        v, pct = c.factor()
    except (_Bad, ZeroDivisionError):
        return None
    c.ws()
    if c.i != len(t) or pct:
        return None
    if v < 0:
        return 0
    if (v * 1000).denominator != 1 or v > 100000:
        raise Unsure("number math")
    return int(v * 1000)


def lh_decl(t):
    """("n"|"i"|"#"|"%"|"p"|"m", value) of a line-height declaration
    value, None when it is invalid. Lot 112: a math function with a
    percentage ("m", resolved against the element's font size) and a math
    function of numbers ("#") are values too"""
    if t in ("normal", "initial"):
        return ("n", 0)
    if t in ("inherit", "unset"):
        return ("i", 0)
    if t.startswith("-"):
        return None
    m = _milli(t)
    if m is not None:
        return ("#", m)
    if t.endswith("%"):
        m = _milli(t[:-1])
        return None if m is None else ("%", m)
    if not t.endswith(")"):
        mm = _re.fullmatch(r"\+?(\d+(\.\d+)?|\.\d+)([a-z]+)", t)
        if mm is None:
            return None
        u = mm.group(3)
        if u in L7.ABS:
            return ("p", int(_F(mm.group(1)) * L7.ABS[u] * 1000))
        if u not in L7.REL and u != "rlh":
            return None
    if L9.math_box(t, 16, 16) is None:
        n = lh_num(t)
        return None if n is None else ("#", min(n, 10 ** 6))
    return ("m", t)


def box_len_lh_at(s, f, root, lh, basis):
    """Lot 112: box_len_lh with the percentages of basis (px thousandths)"""
    save = L9.FONT["lh"]
    try:
        if lh is not None:
            L9.FONT["lh"] = _F(lh, 1000) / _F(f, 1000)
        r = pct_at(s, _F(f, 1000), _F(root, 1000), _F(basis, 1000))
    finally:
        L9.FONT["lh"] = save
    if r is None:
        return None
    v = r * 1000
    if v.denominator != 1:
        raise Unsure("cut")
    return int(v)


def lh_comp(decl, f, root, parent_comp, parent_f):
    """the computed line-height (k, v) of an element of font size f from
    its winning declaration decl (None: inherited)"""
    plh = lh_px(parent_comp, parent_f) if parent_comp is not None else lh_normal(16000)
    pc = parent_comp if parent_comp is not None else (0, 0)
    if decl is None or decl[0] == "i":
        return pc
    k, v = decl
    if k == "n":
        return (0, 0)
    if k == "#":
        return (1, min(v, 10 ** 6))
    if k == "%":
        return (2, min(f * min(v, 10 ** 8) // 100000, LH_MAX))
    if k == "p":
        return (2, min(v, LH_MAX))
    r = box_len_lh_at(v, f, root, plh, f) if "%" in v else box_len_lh(v, f, root, plh)
    if r is None:
        return (0, 0)
    return (2, max(r, 0))


def lh_decl_lot111(t):
    """("n"|"i"|"#"|"%"|"p"|"m", value) of a line-height declaration
    value, None when it is invalid"""
    if t in ("normal", "initial"):
        return ("n", 0)
    if t in ("inherit", "unset"):
        return ("i", 0)
    if t.startswith("-"):
        return None
    m = _milli(t)
    if m is not None:
        return ("#", m)
    if t.endswith("%"):
        m = _milli(t[:-1])
        return None if m is None else ("%", m)
    if not t.endswith(")"):
        mm = _re.fullmatch(r"\+?(\d+(\.\d+)?|\.\d+)([a-z]+)", t)
        if mm is None:
            return None
        u = mm.group(3)
        if u in L7.ABS:
            return ("p", int(_F(mm.group(1)) * L7.ABS[u] * 1000))
        if u not in L7.REL:
            return None
    # a font or viewport length or a math function without a percentage
    if L9.math_box(t, 16, 16) is None or L9.math_box(t, 16, 16)[0] != "px":
        return None
    return ("m", t)


def lh_comp_lot111(decl, f, root, parent_comp, parent_f):
    """the computed line-height (k, v) of an element of font size f from
    its winning declaration decl (None: inherited)"""
    plh = lh_px(parent_comp, parent_f) if parent_comp is not None else lh_normal(16000)
    pc = parent_comp if parent_comp is not None else (0, 0)
    if decl is None or decl[0] == "i":
        return pc
    k, v = decl
    if k == "n":
        return (0, 0)
    if k == "#":
        return (1, min(v, 10 ** 6))
    if k == "%":
        return (2, min(f * min(v, 10 ** 8) // 100000, LH_MAX))
    if k == "p":
        return (2, min(v, LH_MAX))
    r = box_len_lh(v, f, root, plh)
    if r is None:
        return (0, 0)
    return (2, max(r, 0))


# ------------------------------------------------------------- the body's percentages
# CSS 2, 10.5 and 10.2: the body's percentage height (width) is of its
# containing block, html's content box; html's height (width) is
# definite when it is a length, a percentage of the initial containing
# block (the viewport) or an expression of those, and the body's
# percentage then has a value in px; html's auto height leaves it as it
# is (auto); html's auto width is the viewport's, which the layout gives
# anyway (left as it is). Each percentage literal of an expression is
# taken in px thousandths cut toward zero (the reader's math), the
# result in whole px cut toward zero, below 0 is 0.
_PCT = _re.compile(r"(?<![\w.])([+-]?)(\d+(?:\.\d+)?|\.\d+)%")


def pct_at(text, em, rem, basis):
    """math_box of text with its percentages of basis px (px Fraction),
    None when it does not resolve; Unsure when the C would cut"""
    def sub(m):
        n = int(_F(m.group(2)) * 1000)              # 1/1000 %, as parsed
        v = n * basis * 1000 // 100000              # px thousandths (n >= 0)
        if m.group(1) == "-":
            v = -v
        return "%s%d.%03dpx" % ("-" if v < 0 else "", abs(v) // 1000, abs(v) % 1000)
    t = _PCT.sub(sub, text.lower())
    r = L9.math_box(t, em, rem, True)
    if r is None or r[0] != "px":
        return None
    return r[1]


def html_len(slot, em, rem, basis):
    """html's height or width slot (lot107_model) in whole px against the
    viewport basis, None when it is auto or does not resolve"""
    hh = L7.box_len(slot, em, rem) if slot is not None else None
    if hh is None:
        return None
    if hh[1] == "expr":
        v = pct_at(slot[2], em, rem, basis)
        if v is None:
            return None
        v = L7.math_floor(max(v, _F(0)))
    elif hh[1]:
        v = basis * hh[0] // 100
    else:
        v = hh[0]
    return min(v, HMAX)


# ------------------------------------------------- mixed min()/max()/clamp()
# A box length mixing px with a percentage of the (unknown) containing
# block may nest min()/max()/clamp() and sit in sums and products with a
# number (CSS Values 4, 10): the value is written as the engine's math
# reads it (integer literals, at most WE_MATH_NODES_MAX nodes and
# WE_MATH_DEPTH_MAX functions deep, webeng.c parse_math). Sums and factors
# distribute over the arguments, which is exact arithmetic on the values:
# min(a, b) + c = min(a + c, b + c), k * min(a, b) = min(k * a, k * b) for
# k >= 0 and max(k * a, k * b) for k < 0, -clamp(a, b, c) =
# min(-a, max(-b, -c)) (clamp(a, b, c) = max(a, min(b, c))); a min() of a
# min() is one min() of all the arguments; arguments that are all plain
# sums of one part (px only or % only) are compared and the first least
# (greatest) one is the value.
NODES = 24          # WE_MATH_NODES_MAX
FN_DEPTH = 4        # WE_MATH_DEPTH_MAX
ARGS = 4            # arguments of one function the engine's math takes
KIDS = 8            # arguments of one function the reader keeps
POOL = 24           # tree nodes the reader builds for one value
BOXEX = 64          # WF_SITE_BOXEX: the expression and its terminator


class _Pool(Exception):
    """past the reader's bounds"""


def _lin(px, pc):
    return ("lin", px, pc)


def _one_part(xs):
    """1 px, 2 %, 0 both (zero counts as either)"""
    px = any(x[1] for x in xs)
    pc = any(x[2] for x in xs)
    return 0 if px and pc else (2 if pc else 1)


class _Mix(L9._Box):
    """lot109_model's descent with trees for mixed values: a value is
    (px, pct, is_length, tree) with tree None for a linear one"""

    def __init__(self, s, em, rem):
        super().__init__(s, em, rem)
        self.made = 0

    def new(self, t):
        self.made += 1
        if self.made > POOL:
            raise _Pool()
        if t[0] == "lin":
            self.chk(t[1], t[2])
            if abs(t[1]) > L9.LIMIT or abs(t[2]) > L9.LIMIT:
                raise ValueError()
        return t

    def tree(self, v):
        return v[3] if v[3] else self.new(_lin(v[0], v[1]))

    def val(self, t):
        if t[0] == "lin":
            return (t[1], t[2], True, None)
        return (F0, F0, True, t)

    def op(self, f, kids):
        """Lot 112: a plain-sum argument (a leaf) whose px and % parts are
        both at least (at most, for max) another leaf's goes, as a % is
        of a size that is never negative (min(10px, 20px, 5%) is min(10px,
        5%)); of equal leaves the first stays"""
        flat = []
        for k in kids:
            flat.extend(k[1] if k[0] == f else [k])
        if len(flat) > KIDS:
            raise _Pool()

        def cover(a, b):
            if f == "min":
                return a[1] <= b[1] and a[2] <= b[2]
            return a[1] >= b[1] and a[2] >= b[2]
        keep = []
        for k, b in enumerate(flat):
            gone = False
            if b[0] == "lin":
                for j, a in enumerate(flat):
                    if j == k or a[0] != "lin" or not cover(a, b):
                        continue
                    if j < k or not cover(b, a):
                        gone = True
                        break
            if not gone:
                keep.append(b)
        if len(keep) == 1:
            return keep[0]
        return self.new((f, keep))

    def op_lot111(self, f, kids):
        """Lot 111's rule: leaves compared only when all are of one part"""
        flat = []
        for k in kids:
            flat.extend(k[1] if k[0] == f else [k])
        if len(flat) > KIDS:
            raise _Pool()
        if all(k[0] == "lin" for k in flat):
            part = _one_part(flat)
            if part:
                best = flat[0]
                for k in flat[1:]:
                    a, b = k[part], best[part]
                    if (a < b) if f == "min" else (a > b):
                        best = k
                return best
        if len(flat) == 1:
            return flat[0]
        return self.new((f, flat))

    def clamp(self, kids):
        return self.new(("clamp", list(kids)))

    def scale(self, t, k):
        if t[0] == "lin":
            return self.new(_lin(t[1] * k, t[2] * k))
        kids = [self.scale(x, k) for x in t[1]]
        if t[0] == "clamp":
            if k >= 0:
                return self.clamp(kids)
            return self.op("min", [kids[0], self.op("max", kids[1:])])
        f = t[0] if k >= 0 else ("max" if t[0] == "min" else "min")
        return self.op(f, kids)

    def add(self, x, y, sg):
        if x[0] == "lin" and y[0] == "lin":
            return self.new(_lin(x[1] + sg * y[1], x[2] + sg * y[2]))
        if x[0] != "lin":
            kids = [self.add(k, y, sg) for k in x[1]]
            return self.clamp(kids) if x[0] == "clamp" else self.op(x[0], kids)
        if sg < 0:
            return self.add(x, self.scale(y, -1), 1)
        kids = [self.add(x, k, 1) for k in y[1]]
        return self.clamp(kids) if y[0] == "clamp" else self.op(y[0], kids)

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
            if len(a) > KIDS:
                self.fail()
            if any(x[2] != a[0][2] for x in a):
                self.fail()
            mixed = any(x[3] for x in a) or (any(x[0] for x in a) and any(x[1] for x in a))
            if mixed:
                if not a[0][2]:
                    self.fail()
                kids = [self.tree(x) for x in a]
                if f == "clamp(":
                    return self.val(self.clamp(kids))
                return self.val(self.op(f[:-1], kids))
            key = (lambda x: x[1]) if any(x[1] for x in a) else (lambda x: x[0])
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
                    if (v[3] and w[3]) or (op == "/" and not v[3]):
                        self.fail()
                    t, n = (v[3], w) if v[3] else (w[3], v)
                    if n[2]:
                        self.fail()
                    if op == "/" and n[0] == 0:
                        self.fail()
                    v = self.val(self.scale(t, n[0] if op == "*" else 1 / n[0]))
                    continue
                if op == "*":
                    if v[2] and w[2]:
                        self.fail()
                    k, x = (w[0], v) if not w[2] else (v[0], w)
                    v = (x[0] * k, x[1] * k, v[2] or w[2], None)
                else:
                    if w[2] or w[0] == 0:
                        self.fail()
                    v = (v[0] / w[0], v[1] / w[0], v[2], None)
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
                    if v[2] != w[2]:
                        self.fail()
                    sg = 1 if op == "+" else -1
                    if v[3] or w[3]:
                        v = self.val(self.add(self.tree(v), self.tree(w), sg))
                        continue
                    v = (v[0] + sg * w[0], v[1] + sg * w[1], v[2], None)
                    continue
            self.i = save
            return v


F0 = L9.F(0)


def _cost(t):
    """(engine nodes, function depth) of tree t as written"""
    if t[0] == "lin":
        both = bool(t[1]) and bool(t[2])
        return (3, 1) if both else (1, 0)
    if len(t[1]) > ARGS:
        return NODES + 1, 0
    n, d = len(t[1]) - (1 if t[0] != "clamp" else 2), 0
    for k in t[1]:
        a, b = _cost(k)
        n, d = n + a, max(d, b)
    return n, d + 1


def _text(t):
    if t[0] == "lin":
        return L9.lin_text(t[1], t[2])
    return "%s(%s)" % (t[0], ", ".join(_text(k) for k in t[1]))


def math_box(s, em=16, rem=16, exact=False):
    """lot109_model.math_box with the mixed values above (("expr", text))"""
    try:
        p = _Mix(s.lower(), em, rem)
        v = p.factor()
        p.ws()
        if p.i != len(s):
            return None
        if not v[2] and v[0] != 0:
            return None
        if v[3]:
            n, d = _cost(v[3])
            if n > NODES or d > FN_DEPTH:
                return None
            # as Lot 109: a cut 1/1000 cannot move a literal rounded to a
            # whole number (lin_text is Unsure within 1/100 of a half)
            t = _text(v[3])
            return ("expr", t) if len(t) < BOXEX else None
        if abs(v[0]) > L9.LIMIT or abs(v[1]) > L9.LIMIT:
            return None
        if v[1]:
            return ("expr", L9.lin_text(v[0], v[1]))
        if exact and not p.exact:
            raise Unsure("cut")
        return ("px", v[0])
    except (ValueError, ZeroDivisionError, IndexError, _Pool):
        return None
