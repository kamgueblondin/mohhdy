"""Lot 112 model: line-height passed to the engine, line-height with a
percentage or of numbers in a math function, rlh, all and revert on
line-height, comparable arguments of a mixed min()/max() (lot111_model
_Mix.op), var() and math functions in opacity keyframes.

Written from the specifications (CSS Inline 3 4.2, CSS Values 4 6.1.1
and 10, CSS Cascade 5 7.3 and 7.4, CSS Variables 1, CSS Animations 1 and
CSS Color 4 opacity), sharing no code with the C. The engine side follows
webeng.c's documented bounds: a line-height of whole px 10..40, or
normal.
"""
from fractions import Fraction as _F

import lot109_model as L9
import lot110_model as L10
import lot111_model as L111

Unsure = L111.Unsure
LH_MIN, LH_MAX = 10, 40      # WE_LINE_H, WE_LINE_H_MAX
ROOT_LH_INIT = 10000         # rlh in the root's line-height: normal at 16px


def lh_engine(lhs, parents):
    """what each element passes the engine for line-height: lhs are the
    line heights (px thousandths + 1, 0 normal) in document order and
    parents their parent indexes (-1 none); 0 nothing, 1 normal, else whole
    px within 10..40 (truncated). Normal is passed when an ancestor's line
    height is not normal (the engine inherits px), else nothing."""
    out, dirty = [], []
    for i, lh in enumerate(lhs):
        p = parents[i]
        d = bool(lh) or (p >= 0 and dirty[p])
        dirty.append(d)
        if not lh:
            out.append(1 if d else 0)
            continue
        px = (lh - 1) // 1000
        out.append(min(max(px, LH_MIN), LH_MAX))
    return out


def engine_lh(x, font_px):
    """the engine's line height (px) of an element given lh_engine's code
    x for it (0 or 1: normal, the engine's 10 px scaled by its font: q =
    (font + 2) / 4 within 4..16, (10 q + 2) / 4) at ui scale 1x"""
    if x > 1:
        return x
    q = min(max((font_px + 2) // 4, 4), 16)
    return (10 * q + 2) // 4


def lh_style(x):
    """the style attribute's line-height part of lh_engine's code"""
    if x == 0:
        return "-"
    return "line-height:normal" if x == 1 else "line-height:%dpx" % x


def lh_decl(t):
    """lot111_model.lh_decl with revert and revert-layer (("r", 4 or 5))"""
    if t == "revert":
        return ("r", 4)
    if t == "revert-layer":
        return ("r", 5)
    return L111.lh_decl(t)


def lh_var(text, env):
    """a line-height declaration with var() of the element's custom
    properties env: invalid after substitution (no value and no fallback,
    or no line-height) it is unset, inherited (CSS Variables 1 3.1)"""
    t = L111.subst(text, env)
    d = None if t is None else lh_decl(t.strip())
    return ("i", 0) if d is None else d


def all_lh(kw):
    """the line-height declaration of an all value (CSS Cascade 5 3.2):
    initial is normal; inherit and unset inherit (line-height is an
    inherited property); revert and revert-layer roll back"""
    return {"initial": ("n", 0), "inherit": ("i", 0), "unset": ("i", 0), "revert": ("r", 4),
            "revert-layer": ("r", 5)}.get(kw)


def lh_cascade(layered, plain):
    """the winning line-height declaration of an element from its
    declarations in a cascade layer (layered) and outside layers (plain),
    each a list in order of lh_decl / all_lh results (None: invalid,
    dropped): unlayered author rules beat layered ones; revert-layer in an
    unlayered rule takes the layered winner, revert (and revert-layer in a
    layer) goes to the user agent origin, which has no line-height: the
    value is inherited. None: nothing set (inherited)"""
    def win(ds):
        got = None
        for d in ds:
            if d is not None:
                got = d
        return got
    w = win(plain)
    if w is not None and w[0] == "r":
        if w[1] == 4:
            return None
        w = None
        lw = win(layered)
        return None if lw is None or lw[0] == "r" else lw
    if w is not None:
        return w
    lw = win(layered)
    return None if lw is None or lw[0] == "r" else lw


def with_rlh(rlh_milli, fn, *a):
    """fn(*a) with rlh as rlh_milli px thousandths (None: normal of rem)"""
    save = L9.RLH[0]
    try:
        L9.RLH[0] = None if rlh_milli is None else _F(rlh_milli, 1000)
        return fn(*a)
    finally:
        L9.RLH[0] = save


def op_frame(text, env):
    """an opacity keyframe value with var() of the animated element's
    custom properties env: 1/10000 or None when the frame is left out
    (invalid after substitution, or no opacity value)"""
    t = L111.subst(text, env) if "var(" in text else text
    if t is None:
        return None
    return L10.op_kf(t.strip())


def col_frame(text, env, colour_match):
    """a color or background-color keyframe value with var() of the
    animated element's custom properties env: the substituted text
    (lowercased, as the frames keep it) or None when the frame is left out
    (invalid after substitution, !important, not a colour: colour_match
    is the Lot 100 model's check)"""
    t = L111.subst(text, env) if "var(" in text else text
    if t is None:
        return None
    raw = t.strip().lower()
    if not raw or "!" in raw or len(raw) >= 128 or not colour_match(raw):
        return None
    return raw
