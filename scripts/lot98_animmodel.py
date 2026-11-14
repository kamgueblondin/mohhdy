#!/usr/bin/env python3
"""Lot 98 model of the reader's animation and transition longhands,
animation-composition, @keyframes inside @scope and hsl() colours, written
from docs/lot98_plan.md and the specs it cites (CSS Animations 1 and 2,
CSS Transitions 1 and 2, CSS Values 4 coordinating list properties, Web
Animations 1 composite operations, CSS Properties and Values API 1, CSS
Color 4), not from the C sources (userspace/wf_cssprop.c,
userspace/wf_sitecss.c). It builds on scripts/lot97_animmodel.py.

Numbers are integers in 1/10000; rhalf rounds half away from zero.

* hsl(): hsl(H, S%, L%[, A]) and hsla() with commas (S and L must be
  percentages there), or hsl(H S L[ / A]) with spaces (S and L
  percentages or plain numbers, a number meaning that many percent). H is
  a number of degrees or an angle, taken modulo 360deg; S and L are
  clamped to 0..100%; A as for rgb() (a number 0..1 or a percentage). The
  sRGB channels are CSS Color 4's hslToRgb, f(n) = L - a max(-1, min(k -
  3, 9 - k, 1)) with k = (n + H / 30) mod 12 and a = S min(L, 1 - L),
  computed exactly and rounded half away from zero to 1/10000 of 0..255;
  the colour then interpolates like any rgb() colour (premultiplied sRGB).
* add (wf_cssprop_add): the sum of a value on the value below. Numbers,
  lengths, percentages, angles, times and resolutions add in their
  canonical units (a length and a percentage give their mix; an integer
  stays whole); '+' and '#' lists add item by item when they have the
  same length; colours add their premultiplied sRGB channels and alphas
  (the alpha clamped to 1, the channels to 0..255). Anything else, or a
  value that does not match the same alternative, is not additive: the
  value replaces the one below. accumulate is the same as add here (Web
  Animations 1 only tells them apart for values this reader keeps
  discrete; Lot 99: transforms, which scripts/lot99_animmodel.py adds and
  accumulates).
* longhands: animation-name (none or an identifier, not a CSS-wide
  keyword, under 32 bytes), -duration (auto = 0s, or a time not negative),
  -timing-function, -delay, -iteration-count (infinite or a number
  0..100000), -direction, -fill-mode, -play-state, -composition (replace,
  add, accumulate); transition-property (all, none alone, or an
  identifier), -duration, -timing-function, -delay, -behavior
  (allow-discrete, normal). Each is a comma list of one token per item,
  at most four items, no empty item. initial and unset reset one to its
  initial value; inherit is not taken. A declaration value of 128 bytes
  or more is dropped (the reader limit since Lot 90).
* cascade: the animation shorthand sets the first eight animation
  longhands (not animation-composition), the transition shorthand all
  five transition longhands, each to its own list; per longhand the
  declaration with the highest specificity wins, a later one on a tie.
* coordination: the number of animations is the length of
  animation-name (transitions: of transition-property); each other list
  is repeated cyclically (item j takes item j mod its length); a list
  that is not set is its initial value.
* composition: an animation whose composition (or a keyframe's own
  animation-composition) is add or accumulate puts each keyframe value
  given in the rule on the value below it. Lot 99, converted: the
  keyframes 0% and 100% the rule leaves out take the element's own value
  composed with the animation's composition (CSS Animations 2; Lot 98
  filled them in with the value below, uncomposed), and accumulate is
  told apart from add on transforms (scripts/lot99_animmodel.py).
  Animations stack in list order: each one's value below is the result
  of the ones before it.
* @keyframes inside @scope (up to four nested at-rules deep) count like
  top-level ones (keyframe names are global); @property inside @scope is
  not taken.
"""
import math
import os
import re
import sys
from fractions import Fraction as F

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
import lot95_propmodel as P  # noqa: E402
import lot97_animmodel as A  # noqa: E402

S = 10000
rhalf = A.rhalf


# ------------------------------------------------------------- hsl
def colour_args(body):
    """the 3 or 4 argument texts and whether they are comma separated, or
    None"""
    if "," in body:
        toks = [x.strip() for x in body.split(",")]
        if "/" in body:
            return None
        comma = True
    else:
        main, slash, al = body.partition("/")
        toks = main.split()
        if slash:
            if len(toks) != 3 or "/" in al:
                return None
            toks.append(al.strip())
        elif len(toks) != 3:
            return None     # a space separated alpha needs the slash
        comma = False
    if len(toks) not in (3, 4) or any(not t or " " in t for t in toks):
        return None
    return toks, comma


def num_pct(t):
    """(value in 1/10000, is percentage) of a number or percentage token"""
    q = P.leaf(t, None)
    if q is None or not q.ok or q.k not in ("N", "P"):
        return None
    if P.NUM.match(t).end() != len(t) - (1 if q.k == "P" else 0):
        return None
    return q.v, q.k == "P"


def hsl_rgb(h, s, l):
    """h in degrees, s and l in 0..1 (Fractions): the channels in 1/10000
    of 0..255"""
    a = s * min(l, 1 - l)
    out = []
    for n in (0, 8, 4):
        k = (n + h / 30) % 12
        f = l - a * max(-1, min(k - 3, 9 - k, 1))
        out.append(rhalf(f * 255 * S))
    return out


def hsl(v):
    """(channels, alpha) of an hsl() or hsla() colour, or None"""
    m = re.fullmatch(r"hsla?\((.*)\)", v)
    if not m:
        return None
    got = colour_args(m.group(1))
    if got is None:
        return None
    toks, comma = got
    q = P.leaf(toks[0], None)
    if q is None or not q.ok or q.k not in ("N", "A"):
        return None
    if q.k == "N" and P.NUM.match(toks[0]).end() != len(toks[0]):
        return None
    h = F(q.v, S) % 360
    sl = []
    for t in toks[1:3]:
        r = num_pct(t)
        if r is None or (comma and not r[1]):
            return None
        sl.append(F(min(max(r[0], 0), 100 * S), 100 * S))
    a = S
    if len(toks) == 4:
        r = num_pct(toks[3])
        if r is None:
            return None
        a = min(max(rhalf(F(r[0], 100)) if r[1] else r[0], 0), S)
    return hsl_rgb(h, sl[0], sl[1]), a


# ------------------------------------------------------------- add
def add_colour(a, b):
    x, y = A.rgba(a), A.rgba(b)
    if x is None or y is None:
        return None
    (ca, aa), (cb, ab) = x, y
    an = min(max((aa + ab) * S, 0), S * S)
    out = []
    for k in range(3):
        pn = (ca[k] * aa + cb[k] * ab) * S
        out.append(min(max(rhalf(F(pn, an * S)), 0), 255) if an else 0)
    alpha = rhalf(F(an, S))
    if alpha == S:
        return "rgb(%d, %d, %d)" % tuple(out)
    return "rgba(%d, %d, %d, %s)" % (out[0], out[1], out[2], P.fmt(alpha, ""))


def add_num(t, a, b):
    qa, qb = A.item_q(t, a), A.item_q(t, b)
    if qa is None or qb is None:
        return None
    pa, ca = P.parts(qa)
    pb, cb = P.parts(qb)
    v, p = pa + pb, ca + cb
    if qa.k == qb.k and qa.k != "LP":
        k = qa.k
        if k == "P":
            v = p
    elif qa.k in ("L", "P", "LP") and qb.k in ("L", "P", "LP"):
        k = "LP"
    else:
        return None
    if t == "<integer>":
        v = math.floor(F(2 * v + S, 2 * S)) * S
    if not (P.fits(v) and P.fits(p)):
        return None
    return P.fmt_q(P.Q(k, v, p))


def add(alts, under, v):
    """(text, 2 composed or 1 replaced by v) for computed values"""
    rep = (v, 1)
    if alts is None:
        return rep
    al_a, al_b = A.first_alt(alts, under), A.first_alt(alts, v)
    if al_a is None or al_a is not al_b:
        return rep
    ty, mult, _ = al_a
    if ty == "kw":
        return rep

    def one(x, y):
        if ty in A.NUMERIC:
            return add_num(ty, x, y)
        if ty == "<color>":
            return add_colour(x, y)
        return None
    if not mult:
        r = one(under, v)
        return (r, 2) if r is not None else rep
    xa, xb = P.items(under, mult), P.items(v, mult)
    if len(xa) != len(xb):
        return rep
    out = []
    for x, y in zip(xa, xb):
        r = one(x, y)
        if r is None:
            return rep
        out.append(r)
    return (", " if mult == "#" else " ").join(out), 2


# ------------------------------------------------------------- longhands
ANIM_LONGS = ["animation-name", "animation-duration", "animation-timing-function", "animation-delay",
              "animation-iteration-count", "animation-direction", "animation-fill-mode", "animation-play-state",
              "animation-composition"]
TRANS_LONGS = ["transition-property", "transition-duration", "transition-timing-function", "transition-delay",
               "transition-behavior"]
ANIM_FIELD = ["name", "dur", "ease", "delay", "iters", "dir", "fill", "paused", "comp"]
TRANS_FIELD = ["prop", "dur", "ease", "delay", "discrete"]
DIRS = ["normal", "reverse", "alternate", "alternate-reverse"]
FILLS = ["none", "forwards", "backwards", "both"]
COMPS = ["replace", "add", "accumulate"]


def anim_init():
    return {"name": "", "dur": 0, "delay": 0, "iters": S, "dir": "normal", "fill": "none", "paused": False,
            "ease": A.DEFAULT_EASE, "comp": "replace"}


def trans_init():
    return {"prop": "all", "dur": 0, "delay": 0, "discrete": False, "ease": A.DEFAULT_EASE, "none": False}


def anim_long_one(k, w):
    """the value of field k for one token w, or None"""
    if k == 0:
        if w == "none":
            return ""
        if not A.is_ident(w) or w in A.RESERVED or len(w) >= 32:
            return None
        return w
    if k in (1, 3):
        if k == 1 and w == "auto":
            return 0
        ms = A.time_ms(w)
        if ms is None or (k == 1 and ms < 0):
            return None
        return ms
    if k == 2:
        return A.ease_parse(w)
    if k == 4:
        if w == "infinite":
            return -1
        c = A.num4(w)
        if c is None or c < 0 or c > 100000 * S:
            return None
        return c
    if k == 7:
        return {"running": False, "paused": True}.get(w)
    lst = {5: DIRS, 6: FILLS, 8: COMPS}[k]
    return w if w in lst else None


def trans_long_one(k, w):
    if k == 0:
        if w == "none":
            return "NONE"
        if not A.is_ident(w) or len(w) >= 32 or (w in A.RESERVED and w != "all"):
            return None
        return w
    if k in (1, 3):
        ms = A.time_ms(w)
        if ms is None or (k == 1 and ms < 0):
            return None
        return ms
    if k == 2:
        return A.ease_parse(w)
    return {"allow-discrete": True, "normal": False}.get(w)


def long_list(anim, k, v):
    """the list of field values of longhand k, or None"""
    items = A.list_items(v)
    if items is None:
        return None
    out = []
    for it in items:
        toks = A.tokens(it)
        if toks is None or len(toks) != 1:
            return None
        r = (anim_long_one if anim else trans_long_one)(k, toks[0])
        if r is None:
            return None
        if not anim and r == "NONE" and len(items) > 1:
            return None
        out.append(r)
    return out


def coord(anim, slots):
    """slots: per longhand None, ("sh", text) or ("long", text): the
    coordinated list of dicts"""
    fields = ANIM_FIELD if anim else TRANS_FIELD
    init = anim_init if anim else trans_init
    lists = []
    for k, s in enumerate(slots):
        lst = None
        if s is not None:
            if s[0] == "sh":
                p = (A.anim_parse if anim else A.trans_parse)(s[1])
                if p:
                    lst = [x[fields[k]] for x in p]
                    if not anim and k == 0:
                        lst = ["NONE" if x["none"] else x["prop"] for x in p]
            else:
                lst = long_list(anim, k, s[1])
        if not lst:
            lst = [init()[fields[k]]]
        lists.append(lst)
    out = []
    for j in range(len(lists[0])):
        d = init()
        for k, f in enumerate(fields):
            d[f] = lists[k][j % len(lists[k])]
        if not anim:
            d["none"] = d["prop"] == "NONE"
            if d["none"]:
                d["prop"] = ""
        out.append(d)
    return out


def anim_value(alts, a, frames, under, ms, base=None):
    """frames: sorted [(offset, value, ease or None, comp or None)] of the
    rule; (value or None, composed). Lot 99, converted: base is the
    element's own value (under when not given); the generated 0% and 100%
    keyframes compose it with the animation's composition, and accumulate
    is lot99_animmodel.combine (distinct from add on transforms)"""
    import lot99_animmodel as M99
    if base is None:
        base = under
    pr = A.progress(a, ms)
    if pr is None or not frames:
        return None, False
    composed = False
    fr = []
    for o, v, e, c in frames:
        cc = c or a["comp"]
        if cc != "replace":
            r, code = M99.combine(alts, cc, under, v)
            if code == 2:
                v, composed = r, True
        fr.append((o, v, e))

    def generated():
        nonlocal composed
        if a["comp"] != "replace":
            r, code = M99.combine(alts, a["comp"], under, base)
            if code == 2:
                composed = True
                return r
        return base
    if fr[0][0] > 0:
        fr.insert(0, (0, generated(), None))
    if fr[-1][0] < S:
        fr.append((S, generated(), None))
    p, before = pr
    j = 0
    while j + 2 < len(fr) and fr[j + 1][0] <= p:
        j += 1
    x = (20000 * (p - fr[j][0]) // (fr[j + 1][0] - fr[j][0]) + 1) // 2
    e = fr[j][2] or a["ease"]
    return A.interp(alts, fr[j][1], fr[j + 1][1], A.ease_at(e, x, before))[0], composed


def stack(alts, anims, kfs, under, ms):
    """the value after the animations in list order (kfs: name -> frames),
    whether one animated and whether one composed"""
    animated = composed = False
    base = under
    for a in anims:
        # Lot 99: an animation whose timeline is not auto has no effect
        if not a["name"] or a["name"] not in kfs or a.get("timeline", "auto") != "auto":
            continue
        v, c = anim_value(alts, a, kfs[a["name"]], under, ms, base)
        if v is not None:
            under, animated = v, True
            composed = composed or c
    return under, animated, composed
