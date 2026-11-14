#!/usr/bin/env python3
"""Lot 99 model of the reader's hsl() colours, hue units, accumulate on
transforms, the generated 0% and 100% keyframes and animation-timeline,
written from docs/lot99_plan.md and the specs it cites (CSS Color 4,
CSS Values 4 angle units, CSS Transforms 2 section 15, Web Animations 1
composite operations, CSS Animations 2), not from the C sources
(userspace/wf_cssprop.c, userspace/wf_sitecss.c, userspace/wf_cssbg.c).
It builds on scripts/lot98_animmodel.py.

Numbers are integers in 1/10000; rhalf rounds half away from zero.

* hue units: the hue of hsl() and hsla() may be written in deg, grad
  (9/10 deg), turn (360 deg) or rad (the reader's pi, 180 * 10^7 /
  31415927 deg per rad, the ratio <angle> values use since Lot 91); the
  value is rounded to 1/10000 deg like any angle.
* reader colour (wf_cssprop_rgb24): an opaque hsl() or hsla() (its alpha,
  clamped to 0..1, is 1) is a colour the reader paints: the three
  channels, each rounded half away from zero to 0..255. The text runs to
  the first ')'; after it only spaces, then the end, a ';' or a '!' may
  follow. Letters are taken in any case. A colour with an alpha under 1,
  or any other function or name, is not taken here (the reader takes its
  other colours itself).
* accumulate (wf_cssprop_accumulate): as add (Lot 98) for numbers,
  lengths, percentages, angles, times, resolutions and colours; for
  transforms (CSS Transforms 2 section 15):
  - add on a <transform-list> (and a <transform-function>+ list) is the
    list below followed by the value's list ("under v");
  - accumulate pairs the functions of the two lists in order: two
    functions with the same name and as many arguments give that
    function with the arguments summed (lengths, percentages and angles
    as add sums them; scale arguments, one-based, as a + b - 1 with a
    percentage taken as its number / 100; the rotate3d() axis must be the
    same text and is kept); the tail of the longer list is kept as it
    is (its pair is the identity); matrix(), perspective(), two other
    functions or another axis make the whole value replace the one below;
  - a <transform-function> alone (and each item of a <transform-function>#
    list) has no sum that is one function: add replaces, accumulate
    pairs the two functions as above;
  - the result is written into the room given (the reader gives 128
    bytes, the value limit): when it does not fit before the values are
    found not to combine, the result is empty (code 0) and the reader
    keeps the keyframe value uncomposed.
* generated keyframes (CSS Animations 2, generate initial and final
  frames): a 0% or 100% keyframe the rule leaves out takes the element's
  own value (base: its value before any animation) composed with the
  animation's composition (not a keyframe's own) on the value below;
  replace takes the base value itself. A composite that does not combine
  leaves the base value.
* animation-timeline: a comma list of auto, none or a dashed identifier
  (under 32 bytes, kept as written), one token per item; scroll() and
  view() are not taken (the declaration is dropped). The animation
  shorthand resets it to auto. An animation whose timeline is not auto
  has no effect here (the reader has no scroll or view timelines and no
  named timeline is ever defined): it is counted as idle when its name
  is a kept @keyframes, and it is skipped when the animations stack.
"""
import os
import re
import sys
from fractions import Fraction as F

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
import lot95_propmodel as P  # noqa: E402
import lot97_animmodel as A  # noqa: E402
import lot98_animmodel as M98  # noqa: E402

S = 10000
VAL = 128
rhalf = A.rhalf
SP = " \t\n\r"

ANIM_LONGS = M98.ANIM_LONGS + ["animation-timeline"]
ANIM_FIELD = M98.ANIM_FIELD + ["timeline"]
TIMELINE_CODE = {"auto": 0, "none": 1}


# ------------------------------------------------------------- colour
def rgb24(v):
    """(r, g, b) 0..255 of the reader colour at the start of v, or None"""
    s = v.lstrip(SP)
    b = s.find(")")
    if b < 0 or b + 1 >= VAL:
        return None
    rest = s[b + 1:].lstrip(SP)
    if rest and rest[0] not in ";!":
        return None
    t = s[:b + 1].lower()
    if not t.startswith(("hsl(", "hsla(")) or not A.colour_match(t):
        return None
    r = M98.hsl(t)
    if r is None or r[1] != S:
        return None
    return tuple(rhalf(F(c, S)) for c in r[0])


# ------------------------------------------------------------- transforms
def tf_parts(f):
    """(name, [arguments without spaces]) of one computed transform
    function, or None"""
    f = f.strip(SP)
    i = f.find("(")
    if i < 0 or not f.endswith(")") or f[:i] not in P.TFNS:
        return None
    args, depth, cur = [], 0, ""
    for c in f[i + 1:-1]:
        if c == "(":
            depth += 1
        elif c == ")":
            depth -= 1
        if c == "," and depth == 0:
            args.append(cur.strip(SP))
            cur = ""
        else:
            cur += c
    args.append(cur.strip(SP))
    if len(args) > 6:
        return None
    return f[:i], args


def scale_arg(a):
    r = M98.num_pct(a)
    if r is None:
        return None
    v, pct = r
    return rhalf(F(v, 100)) if pct else v


class Short(Exception):
    """the output does not fit the room given"""


class NoSum(Exception):
    """the values do not combine this way"""


class Out:
    """text written piece by piece into cap bytes (one kept for the end):
    a piece that does not fit raises Short, so what fails first, the room
    or the sum, is what decides (the order the pieces are written in)"""

    def __init__(self, cap):
        self.cap, self.t = cap, ""

    def put(self, w):
        if len(self.t) + len(w) >= self.cap:
            raise Short
        self.t += w


def tf_acc(x, y, w):
    """two transform functions accumulated into w (raises NoSum, Short)"""
    fx, fy = tf_parts(x), tf_parts(y)
    if fx is None or fy is None or fx[0] != fy[0] or len(fx[1]) != len(fy[1]):
        raise NoSum
    name = fx[0]
    if name in ("matrix", "perspective"):
        raise NoSum
    kinds = P.TFNS[name][2]
    w.put(name + "(")
    for k, (a, b) in enumerate(zip(fx[1], fy[1])):
        kind = kinds[k]
        if k:
            w.put(", ")
        if kind == "N":
            if a != b:
                raise NoSum
            w.put(a)
        elif kind == "S":
            va, vb = scale_arg(a), scale_arg(b)
            if va is None or vb is None or not P.fits(va + vb - S):
                raise NoSum
            w.put(P.fmt(va + vb - S, ""))
        else:
            ty = {"P": "<length-percentage>", "L": "<length>", "A": "<angle>"}[kind]
            r = M98.add_num(ty, a, b)
            if r is None:
                raise NoSum
            w.put(r)
    w.put(")")


def tl_funcs(s):
    """the function texts of a computed transform list"""
    out, i = [], 0
    while i < len(s):
        while i < len(s) and s[i] in SP:
            i += 1
        if i >= len(s):
            break
        j = s.index("(", i)
        e = P.group_end(s, j)
        out.append(s[i:e])
        i = e
    return out


def tl_combine(acc, a, b, w):
    if not acc:
        w.put(a.strip(SP))
        w.put(" ")
        w.put(b.strip(SP))
        return
    fa, fb = tl_funcs(a), tl_funcs(b)
    for k in range(max(len(fa), len(fb))):
        if k:
            w.put(" ")
        if k >= len(fa) or k >= len(fb):
            w.put(fa[k] if k < len(fa) else fb[k])
        else:
            tf_acc(fa[k], fb[k], w)


def combine(alts, comp, under, v, cap=VAL):
    """(text, 2 composed, 1 replaced by v, or 0 with "" when cap bytes
    are short): v put on under with comp ("add" or "accumulate"),
    computed values; the reader gives cap 128"""
    rep = (v, 1) if len(v) < cap else ("", 0)
    if alts is None:
        return rep
    al_a, al_b = A.first_alt(alts, under), A.first_alt(alts, v)
    if al_a is None or al_a is not al_b:
        return rep
    ty, mult, _ = al_a
    acc = comp == "accumulate"
    w = Out(cap)
    try:
        if (ty == "<transform-list>" and not mult) or (ty == "<transform-function>" and mult == "+"):
            tl_combine(acc, under, v, w)
        elif ty == "<transform-function>":
            if not acc:
                return rep
            if not mult:
                tf_acc(under, v, w)
            else:
                xa, xb = P.items(under, mult), P.items(v, mult)
                for k in range(max(len(xa), len(xb))):
                    if k >= len(xa) or k >= len(xb):
                        raise NoSum
                    if k:
                        w.put(", ")
                    tf_acc(xa[k], xb[k], w)
        else:
            r, code = M98.add(alts, under, v)
            return (r, code) if len(r) < cap else ("", 0)
    except Short:
        return "", 0
    except NoSum:
        return rep
    return w.t, 2


# ------------------------------------------------------------- timeline
def timeline_one(w):
    lw = w.lower()
    if lw in ("auto", "none"):
        return lw
    if len(w) < 3 or not w.startswith("--") or not A.is_ident(lw) or len(w) >= 32:
        return None
    return w


def anim_long_one(k, w):
    if k == 9:
        return timeline_one(w)
    return M98.anim_long_one(k, w)


def long_list(k, v):
    """the list of animation longhand k (0..9), or None"""
    if k < 9:
        return M98.long_list(True, k, v)
    items = A.list_items(v)
    if items is None:
        return None
    out = []
    for it in items:
        toks = A.tokens(it)
        if toks is None or len(toks) != 1:
            return None
        r = timeline_one(toks[0])
        if r is None:
            return None
        out.append(r)
    return out


def anim_init():
    d = M98.anim_init()
    d["timeline"] = "auto"
    return d


def coord(slots):
    """the coordinated animation list from the ten longhand slots (as
    lot98_animmodel.coord; a shorthand slot of animation-timeline is
    auto)"""
    lists = []
    for k, s in enumerate(slots):
        lst = None
        if s is not None:
            if s[0] == "sh":
                p = A.anim_parse(s[1])
                if p:
                    lst = ["auto"] * len(p) if k == 9 else [x[ANIM_FIELD[k]] for x in p]
            else:
                lst = long_list(k, s[1])
        if not lst:
            lst = [anim_init()[ANIM_FIELD[k]]]
        lists.append(lst)
    out = []
    for j in range(len(lists[0])):
        d = anim_init()
        for k, f in enumerate(ANIM_FIELD):
            d[f] = lists[k][j % len(lists[k])]
        out.append(d)
    return out


def idle(anims, kfs):
    return sum(1 for a in anims if a["timeline"] != "auto" and a["name"] and a["name"] in kfs)
