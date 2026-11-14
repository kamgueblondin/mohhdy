#!/usr/bin/env python3
"""Lot 95 model of the reader's @property rules (CSS Properties and Values
API 1, restricted as docs/lot95_plan.md says): syntax strings with up to
eight alternatives and the types <url>, <image>, <resolution>,
<transform-function>, <transform-list> and <string>; matching a value;
and the computed value of a registered value.

This file shares no code with the C under test (userspace/wf_cssprop.c).
Lot 96 (converted): <image> gradients are checked as the background reader
checks them (lot96_gradmodel.gradient_ok).
Values are lower case and trimmed, as the reader keeps them.

Numbers are kept in 1/10000 of the canonical unit (px, %, deg, s, dppx or
a plain number). A leaf N<unit> is N (at most four decimals here) times
the unit's ratio, rounded half away from zero; a viewport unit is N times
the viewport side / 100, rounded the same way; em, rem, ex and ch are not
known (Lot 97, converted: they are N times the font size when the context
carries one, vp = (vw, vh, em, rem) in px thousandths, ex and ch being half
an em, rounded the same way). A product with a number and a quotient by a number are rounded
half away from zero at each step; sums are exact. Any value past 100000
units is not known. A length and a percentage add into a mix written
"calc(P% + Lpx)" (" - " and |L| when L < 0). An item whose value is not
known keeps its text; the whole value is "kept" (None) when its computed
text equals the text.
"""
from fractions import Fraction as F
import os
import re
import sys

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
import lot96_gradmodel as M96  # noqa: E402

S = 10000
LIM = 10 ** 9

TYPES = ["<length>", "<percentage>", "<length-percentage>", "<number>", "<integer>", "<color>", "<angle>",
         "<time>", "<custom-ident>", "<url>", "<image>", "<resolution>", "<transform-function>",
         "<transform-list>", "<string>"]
CODE = {"<length>": "WF_CSSPROP_LENGTH", "<percentage>": "WF_CSSPROP_PERCENT",
        "<length-percentage>": "WF_CSSPROP_LENPCT", "<number>": "WF_CSSPROP_NUMBER",
        "<integer>": "WF_CSSPROP_INTEGER", "<color>": "WF_CSSPROP_COLOR", "<angle>": "WF_CSSPROP_ANGLE",
        "<time>": "WF_CSSPROP_TIME", "<custom-ident>": "WF_CSSPROP_CIDENT", "<url>": "WF_CSSPROP_URL",
        "<image>": "WF_CSSPROP_IMAGE", "<resolution>": "WF_CSSPROP_RESOLUTION",
        "<transform-function>": "WF_CSSPROP_TFUNC", "<transform-list>": "WF_CSSPROP_TLIST",
        "<string>": "WF_CSSPROP_STRING", "kw": "WF_CSSPROP_KEYWORD"}
RESERVED = {"initial", "inherit", "unset", "revert", "revert-layer", "default"}
MAXALT = 8

# unit -> (kind, ratio); ratio None: viewport unit; 0: not known
UNITS = {"px": ("L", F(1)), "in": ("L", F(96)), "cm": ("L", F(4800, 127)), "mm": ("L", F(480, 127)),
         "q": ("L", F(120, 127)), "pt": ("L", F(4, 3)), "pc": ("L", F(16)),
         "em": ("L", 0), "rem": ("L", 0), "ex": ("L", 0), "ch": ("L", 0),
         "vw": ("L", None), "vh": ("L", None), "vmin": ("L", None), "vmax": ("L", None),
         "deg": ("A", F(1)), "grad": ("A", F(9, 10)), "turn": ("A", F(360)),
         "rad": ("A", F(1800000000, 31415927)), "s": ("T", F(1)), "ms": ("T", F(1, 1000)),
         "dppx": ("R", F(1)), "x": ("R", F(1)), "dpi": ("R", F(1, 96)), "dpcm": ("R", F(254, 9600))}
CANON = {"L": "px", "P": "%", "A": "deg", "T": "s", "R": "dppx", "N": ""}
TFNS = {"translate": (1, 2, "PP"), "translatex": (1, 1, "P"), "translatey": (1, 1, "P"),
        "translatez": (1, 1, "L"), "translate3d": (3, 3, "PPL"), "scale": (1, 2, "SS"), "scalex": (1, 1, "S"),
        "scaley": (1, 1, "S"), "scalez": (1, 1, "S"), "scale3d": (3, 3, "SSS"), "rotate": (1, 1, "A"),
        "rotatex": (1, 1, "A"), "rotatey": (1, 1, "A"), "rotatez": (1, 1, "A"), "rotate3d": (4, 4, "NNNA"),
        "skew": (1, 2, "AA"), "skewx": (1, 1, "A"), "skewy": (1, 1, "A"), "matrix": (6, 6, "NNNNNN"),
        "perspective": (1, 1, "L")}
COLORS = {"black", "white", "red", "green", "blue", "yellow", "gray", "grey", "silver", "maroon", "purple",
          "fuchsia", "lime", "olive", "navy", "teal", "aqua", "orange", "transparent", "currentcolor"}
GRADS = ["linear-gradient(", "radial-gradient(", "conic-gradient(", "repeating-linear-gradient(",
         "repeating-radial-gradient(", "repeating-conic-gradient("]
SP = " \t\n\r"


def rhalf(fr):
    """round a Fraction half away from zero to an integer"""
    n = abs(fr)
    q = int(n)
    if n - q >= F(1, 2):
        q += 1
    return q if fr >= 0 else -q


def is_ident(w):
    return bool(re.fullmatch(r"(-[a-z_-]|[a-z_])[a-z0-9_-]*", w.lower()))


# ------------------------------------------------------------- syntax
def parse_syntax(text):
    """('ANY', []) | (code, alts) | ('BAD', None); an alt is (type, mult, ident)"""
    t = text.strip(SP)
    if len(t) < 3 or t[0] not in "'\"" or t[-1] != t[0]:
        return "BAD", None
    body = t[1:-1].strip(SP)
    if body == "*":
        return "ANY", []
    parts = body.split("|")
    if len(parts) > MAXALT:
        return "BAD", None
    alts = []
    for p in parts:
        p = p.strip(SP)
        if not p:
            return "BAD", None
        mult = ""
        if p[-1] in "+#":
            mult = p[-1]
            p = p[:-1]
            if not p:
                return "BAD", None
        if p.startswith("<"):
            if p.lower() not in TYPES:
                return "BAD", None
            if p.lower() == "<transform-list>" and mult:
                return "BAD", None
            alts.append((p.lower(), mult, ""))
        else:
            if len(p) >= 16 or not is_ident(p) or p.lower() in RESERVED:
                return "BAD", None
            alts.append(("kw", mult, p.lower()))
    if len(alts) == 1 and alts[0][1] == "" and alts[0][0] != "kw":
        return CODE[alts[0][0]], alts
    return "WF_CSSPROP_COMPLEX", alts


# ------------------------------------------------------------- values
NUM = re.compile(r"[+-]?(\d+(\.\d+)?|\.\d+)")


class Q:
    """a typed value: kind L P LP N A T R; v (px or the value), p (% of a
    mix); ok: known"""
    def __init__(self, k, v=0, p=0, ok=True):
        self.k, self.v, self.p, self.ok = k, v, p, ok


def fits(x):
    return -LIM <= x <= LIM


def raw_units(numtext):
    """the number in 1/10000, rounded half away at the fifth decimal (the
    generators use four at most)"""
    neg = numtext.startswith("-")
    t = numtext.lstrip("+-")
    if "." in t:
        a, b = t.split(".")
    else:
        a, b = t, ""
    v = int(a or "0") * S + int((b + "0000")[:4])
    if len(b) > 4 and b[4] >= "5":
        v += 1
    return -v if neg else v, int(a or "0") > LIM


def leaf(text, vp):
    """a number with its unit: Q or None (not a leaf)"""
    m = NUM.match(text)
    if not m:
        return None
    raw, big = raw_units(m.group(0))
    u = text[m.end():]
    ok = not big and abs(raw) <= LIM
    if u == "":
        return Q("N", raw, 0, ok)
    if u == "%":
        return Q("P", raw, 0, ok)
    if u not in UNITS:
        return None
    kind, ratio = UNITS[u]
    if ratio == 0:
        if not vp or len(vp) < 4:
            return Q(kind, 0, 0, False)
        fs = vp[3] if u == "rem" else vp[2]
        if fs <= 0:
            return Q(kind, 0, 0, False)
        v = rhalf(F(raw * fs, 2000 if u in ("ex", "ch") else 1000))
        return Q(kind, v, 0, ok and fits(v))
    if ratio is None:
        if not vp or vp[0] <= 0 or vp[1] <= 0:
            return Q(kind, 0, 0, False)
        w, h = vp[0], vp[1]
        base = {"vw": w, "vh": h, "vmin": min(w, h), "vmax": max(w, h)}[u]
        v = rhalf(F(raw * base, 100))
    else:
        v = rhalf(raw * ratio)
    return Q(kind, v, 0, ok and fits(v))


def tadd(a, b):
    if a == b:
        return a
    if a in ("L", "P", "LP") and b in ("L", "P", "LP"):
        return "LP"
    return None


def parts(q):
    if q.k == "P":
        return 0, q.v
    return q.v, (q.p if q.k == "LP" else 0)


def qadd(a, b, sub):
    t = tadd(a.k, b.k)
    if t is None:
        return None
    if t == "LP":
        ax, ap = parts(a)
        bx, bp = parts(b)
        x, p = (ax - bx, ap - bp) if sub else (ax + bx, ap + bp)
        return Q("LP", x, p, a.ok and b.ok and fits(x) and fits(p))
    v = a.v - b.v if sub else a.v + b.v
    return Q(t, v, 0, a.ok and b.ok and fits(v))


def qmul(a, b):
    if a.k == "N":
        f, x = a, b
    elif b.k == "N":
        f, x = b, a
    else:
        return None
    ok = a.ok and b.ok
    if not ok:
        return Q(x.k, 0, 0, False)
    v = rhalf(F(x.v * f.v, S))
    p = rhalf(F(x.p * f.v, S)) if x.k == "LP" else 0
    return Q(x.k, v, p, fits(v) and fits(p))


def qdiv(a, b):
    if b.k != "N":
        return None
    if not (a.ok and b.ok) or b.v == 0:
        return Q(a.k, 0, 0, False)
    v = rhalf(F(a.v * S, b.v))
    p = rhalf(F(a.p * S, b.v)) if a.k == "LP" else 0
    return Q(a.k, v, p, fits(v) and fits(p))


def calc(text, vp):
    """calc(A [+-*/] B ...) of leaves, with (...) groups: Q or None. The
    generators write only this subset of the math functions; + and - need
    white space around them."""
    if not text.startswith("calc(") or not text.endswith(")"):
        return None
    toks = tokenize(text[5:-1])
    if toks is None:
        return None
    pos = [0]

    def value():
        if pos[0] >= len(toks):
            return None
        t = toks[pos[0]]
        pos[0] += 1
        if t == "(":
            q = summ()
            if q is None or pos[0] >= len(toks) or toks[pos[0]] != ")":
                return None
            pos[0] += 1
            return q
        if t in ("+", "-", "*", "/", ")"):
            return None
        return leaf(t, vp)

    def product():
        q = value()
        while q is not None and pos[0] < len(toks) and toks[pos[0]] in ("*", "/"):
            op = toks[pos[0]]
            pos[0] += 1
            u = value()
            if u is None:
                return None
            q = qmul(q, u) if op == "*" else qdiv(q, u)
        return q

    def summ():
        q = product()
        while q is not None and pos[0] < len(toks) and toks[pos[0]] in ("+", "-"):
            op = toks[pos[0]]
            pos[0] += 1
            u = product()
            if u is None:
                return None
            q = qadd(q, u, op == "-")
        return q

    q = summ()
    if q is None or pos[0] != len(toks):
        return None
    return q


def tokenize(s):
    """leaves, '(' ')' '*' '/' and '+' '-' that have white space on both
    sides (else they are a sign of the next leaf)"""
    toks = []
    i = 0
    while i < len(s):
        c = s[i]
        if c in SP:
            i += 1
            continue
        if c in "()*/":
            toks.append(c)
            i += 1
            continue
        if c in "+-" and i + 1 < len(s) and s[i + 1] in SP and i > 0 and s[i - 1] in SP:
            toks.append(c)
            i += 1
            continue
        m = re.match(r"[+-]?(\d+(\.\d+)?|\.\d+)(%|[a-z]+)?", s[i:])
        if not m:
            return None
        toks.append(m.group(0))
        i += m.end()
    return toks


def fmt(v, unit):
    sign = "-" if v < 0 else ""
    v = abs(v)
    w, f = divmod(v, S)
    t = sign + str(w)
    if f:
        t += "." + ("%04d" % f).rstrip("0")
    return t + unit


def fmt_q(q):
    if q.k == "LP":
        return "calc(%s %s %s)" % (fmt(q.p, "%"), "-" if q.v < 0 else "+", fmt(abs(q.v), "px"))
    return fmt(q.v, CANON[q.k])


# ------------------------------------------------------------- matching
def num_type(text, vp=None):
    """the kind of a numeric value (a leaf or calc of leaves): (kind,
    zero) or None"""
    if text.startswith("calc("):
        q = calc(text, vp)
        return (q.k, False) if q else None
    q = leaf(text, vp)
    if q is None:
        return None
    if NUM.match(text).end() != len(text) and q.k == "N":
        return None
    zero = q.k == "N" and raw_units(NUM.match(text).group(0))[0] == 0
    return q.k, zero


def match_num(t, text):
    nt = num_type(text)
    if nt is None:
        return False
    k, zero = nt
    if t == "<length>":
        return k == "L" or (k == "N" and zero)
    if t == "<percentage>":
        return k == "P"
    if t == "<length-percentage>":
        return k in ("L", "P", "LP") or (k == "N" and zero)
    if t == "<number>":
        return k == "N"
    if t == "<integer>":
        if k != "N":
            return False
        return text.startswith("calc(") or "." not in text
    return {"<angle>": "A", "<time>": "T", "<resolution>": "R"}[t] == k


def is_string(s):
    if len(s) < 2 or s[0] not in "'\"" or s[-1] != s[0]:
        return False
    q = s[0]
    i = 1
    while i < len(s) - 1:
        c = s[i]
        if c == "\\":
            if i + 1 >= len(s) - 1 or s[i + 1] == "\n":
                return False
            i += 2
            continue
        if c == q or c in "\n\r":
            return False
        i += 1
    return True


def is_url(s):
    if len(s) < 5 or not s.startswith("url(") or not s.endswith(")"):
        return False
    inner = s[4:-1].strip(SP)
    if inner and inner[0] in "'\"":
        return is_string(inner)
    i = 0
    while i < len(inner):
        c = inner[i]
        if c == "\\":
            if i + 1 >= len(inner):
                return False
            i += 2
            continue
        if c in "\"'()" or c in SP or ord(c) < 32:
            return False
        i += 1
    return True


def group_end(s, i):
    depth = 0
    q = None
    j = i
    while j < len(s):
        c = s[j]
        if q:
            if c == "\\":
                j += 1
            elif c == q:
                q = None
        elif c in "'\"":
            q = c
        elif c == "(":
            depth += 1
        elif c == ")":
            depth -= 1
            if depth == 0:
                return j + 1
        j += 1
    return -1


def is_image(s):
    """Lot 96 (converted): a gradient is what the background reader takes
    (scripts/lot96_gradmodel.py, written from the reader's rules); Lot 95:
    any gradient name whose parentheses balance and close at the end"""
    if is_url(s):
        return True
    for g in GRADS:
        if s.startswith(g) and len(s) > len(g):
            if group_end(s, len(g) - 1) != len(s):
                return False
            return bool(M96.gradient_ok(s))
    return False


def split_args(inner):
    out, depth, cur = [], 0, ""
    for c in inner:
        if c == "(":
            depth += 1
        elif c == ")":
            depth -= 1
        if c == "," and depth == 0:
            out.append(cur)
            cur = ""
        else:
            cur += c
    out.append(cur)
    return [a.strip(SP) for a in out]


def targ_ok(kind, a):
    if not a:
        return False
    if kind == "P":
        return match_num("<length-percentage>", a)
    if kind == "L":
        return match_num("<length>", a)
    if kind == "N":
        return match_num("<number>", a)
    if kind == "S":
        return match_num("<number>", a) or match_num("<percentage>", a)
    nt = num_type(a)
    return match_num("<angle>", a) or (nt is not None and nt[0] == "N" and nt[1] and not a.startswith("calc("))


def tfunc(s):
    """(name, [args]) when s is one transform function, else None"""
    m = re.match(r"[a-z0-9]*\(", s)
    if not m:
        return None
    name = m.group(0)[:-1]
    if name not in TFNS or group_end(s, len(name)) != len(s):
        return None
    args = split_args(s[len(name) + 1:-1])
    lo, hi, kinds = TFNS[name]
    if not (lo <= len(args) <= hi):
        return None
    for k, a in enumerate(args):
        if not targ_ok(kinds[k], a):
            return None
    return name, args


def tlist(s):
    out = []
    i = 0
    while i < len(s):
        while i < len(s) and s[i] in SP:
            i += 1
        if i >= len(s):
            break
        j = i
        while j < len(s) and s[j] != "(" and s[j] not in SP:
            j += 1
        if j >= len(s) or s[j] != "(":
            return None
        e = group_end(s, j)
        if e < 0:
            return None
        f = tfunc(s[i:e])
        if f is None:
            return None
        out.append(f)
        i = e
    return out or None


def is_color(v):
    if v.startswith("#"):
        return len(v) in (4, 5, 7, 9) and all(c in "0123456789abcdef" for c in v[1:])
    return v in COLORS


def match_one(t, ident, v):
    if not v:
        return False
    if t == "kw":
        return v == ident
    if t == "<color>":
        return is_color(v)
    if t == "<custom-ident>":
        return is_ident(v) and v not in RESERVED
    if t == "<url>":
        return is_url(v)
    if t == "<string>":
        return is_string(v)
    if t == "<image>":
        return is_image(v)
    if t == "<transform-function>":
        return tfunc(v) is not None
    if t == "<transform-list>":
        return tlist(v) is not None
    return match_num(t, v)


def items(v, mult):
    out, depth, cur = [], 0, ""
    for c in v:
        if c == "(":
            depth += 1
        elif c == ")":
            depth -= 1
        if depth == 0 and ((mult == "#" and c == ",") or (mult == "+" and c in SP)):
            out.append(cur)
            cur = ""
        else:
            cur += c
    out.append(cur)
    out = [x.strip(SP) for x in out]
    if mult == "+":
        out = [x for x in out if x]
    return out


def match_alt(alt, v):
    t, mult, ident = alt
    if not mult:
        return match_one(t, ident, v)
    its = items(v, mult)
    return len(its) > 0 and all(x and match_one(t, ident, x) for x in its)


def first_alt(alts, v):
    for a in alts:
        if match_alt(a, v):
            return a
    return None


# ------------------------------------------------------------- computed
def num_item(t, text, vp):
    if text.startswith("calc("):
        q = calc(text, vp)
        if q is None or not q.ok:
            return text
        if t == "<integer>":
            q = Q("N", (2 * q.v + S) // (2 * S) * S)   # half toward +infinity
        return fmt_q(q) if fits(q.v) else text
    q = leaf(text, vp)
    if q is None or not q.ok:
        return text
    if q.k == "N" and t in ("<length>", "<length-percentage>"):
        q = Q("L", q.v)
    return fmt_q(q)


def item(t, text, vp):
    if t in ("<length>", "<percentage>", "<length-percentage>", "<number>", "<integer>", "<angle>", "<time>",
             "<resolution>"):
        return num_item(t, text, vp)
    if t in ("<transform-function>", "<transform-list>"):
        fs = [tfunc(text)] if t == "<transform-function>" else tlist(text)
        out = []
        for name, args in fs:
            kinds = TFNS[name][2]
            cargs = []
            for k, a in zip(kinds, args):
                if k == "A" and not match_num("<angle>", a):
                    cargs.append("0deg")
                    continue
                ty = {"P": "<length-percentage>", "L": "<length>", "N": "<number>", "A": "<angle>"}.get(k)
                if k == "S":
                    ty = "<percentage>" if match_num("<percentage>", a) else "<number>"
                cargs.append(num_item(ty, a, vp))
            out.append("%s(%s)" % (name, ", ".join(cargs)))
        return " ".join(out)
    return text


def compute(alts, v, vp=None):
    """the computed text, or None when the value is kept as it is"""
    a = first_alt(alts, v)
    if a is None or a[0] == "kw":
        return None
    t, mult, _ = a
    if not mult:
        out = item(t, v, vp)
    else:
        out = (", " if mult == "#" else " ").join(item(t, x, vp) for x in items(v, mult))
    if len(out) >= 128:
        return None
    return None if out == v else out
