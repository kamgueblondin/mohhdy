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


def lh_engine_lot112(lhs, parents):
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


def engine_lh_lot112(x, font_px):
    """the engine's line height (px) of an element given lh_engine_lot112's
    code x for it (0 or 1: normal, the engine's 10 px scaled by its font: q =
    (font + 2) / 4 within 4..16, (10 q + 2) / 4) at ui scale 1x"""
    if x > 1:
        return x
    q = min(max((font_px + 2) // 4, 4), 16)
    return (10 * q + 2) // 4


def lh_style_lot112(x):
    """the style attribute's line-height part of lh_engine_lot112's code"""
    if x == 0:
        return "-"
    return "line-height:normal" if x == 1 else "line-height:%dpx" % x


# Lot 116: the engine keeps px with hundredths up to 400 px and a unitless
# number (hundredths) up to 40, multiplied by its font size in whole px
# (CSS Inline 3 4.2); the used line height is 1..400 px
LH116_MAX, LH116_NUM_MAX = 400, 40


def lh_engine(lhs, comps, fss, parents):
    """Lot 116: what each element passes the engine for line-height: lhs as
    in lh_engine_lot112, comps the computed values (k, v) (lot111_model
    lh_comp: k 1 a number in 1/1000), fss the font sizes (px thousandths);
    0 nothing, 1 normal, -1 - hundredths for a number (at most 40, whole px
    font), else 2 + px hundredths (rounded half up, at most 400 px)"""
    out, dirty = [], []
    for i, lh in enumerate(lhs):
        p = parents[i]
        d = bool(lh) or (p >= 0 and dirty[p])
        dirty.append(d)
        if not lh:
            out.append(1 if d else 0)
            continue
        k, v = comps[i]
        if k == 1 and v <= LH116_NUM_MAX * 1000 and fss[i] % 1000 == 0:
            out.append(-1 - (v + 5) // 10)
            continue
        out.append(2 + min((lh - 1 + 5) // 10, LH116_MAX * 100))
    return out


def engine_lh100(x, font_px):
    """Lot 116: the engine's used line height in hundredths of a px of an
    element given lh_engine's code x (normal: (10 q + 2) / 4 px)"""
    if x in (0, 1):
        q = min(max((font_px + 2) // 4, 4), 16)
        return (10 * q + 2) // 4 * 100
    if x < 0:
        v = max(-1 - x, 1) * font_px
    else:
        v = x - 2 if x > 2 else 1
    return min(max(v, 100), LH116_MAX * 100)


def engine_lh(x, font_px):
    """Lot 116: the height of a one-line block: whole px of the used line
    height (a fraction is carried to the next line)"""
    return engine_lh100(x, font_px) // 100


def hund(v):
    """v hundredths as the reader writes them (trailing zeros left out)"""
    t = "%d" % (v // 100)
    if v % 100:
        t += (".%02d" % (v % 100)).rstrip("0")
    return t


def lh_style(x):
    """the style attribute's line-height part of lh_engine's code"""
    if x == 0:
        return "-"
    if x == 1:
        return "line-height:normal"
    return "line-height:" + (hund(-1 - x) if x < 0 else hund(x - 2) + "px")


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


# ------------------------------------------------- Lot 112 (second part)
# CSS Values 4, 6.1.1: font-relative lengths in font-size are of the
# parent element's font (em, ex, ch), lh of the parent's line height and
# rlh of the root's (the initial value, normal, in the root's own
# font-size); a percentage is of the parent's font size. ex and ch are
# the metrics of the font the engine draws (lot109_model.font_unit). A
# plain length must be above 0; a math function below the reader's 1/1000
# px or above 96 px keeps the parent's size, as the Lot 89 bounds do.
import re as _re  # noqa: E402

FS_LO, FS_HI = 1, 96000
_FS_PLAIN = _re.compile(r"([+-]?(\d+(\.\d+)?|\.\d+))(ex|ch|lh|rlh)")


def fs_metric(t, pf, rem, plh, rlh):
    """font-size value t (with ex, ch, lh or rlh) of an element whose
    parent has font size pf, line height plh (px thousandths, None for
    normal), the root's font size rem and line height rlh (None normal):
    the font size in px thousandths, None when it keeps the parent's"""
    m = _FS_PLAIN.fullmatch(t)
    if m and _F(m.group(1)) <= 0:
        return None
    # the declaration must resolve at 16px (as written: no var(), no
    # unknown unit, at most 10000 px)
    if with_rlh(None, L111.box_len_lh_at, t, 16000, 16000, None, 16000) is None:
        return None
    v = with_rlh(rlh, L111.box_len_lh_at, t, pf, rem, plh, pf)
    if v is None or v < FS_LO or v > FS_HI:
        return None
    return v


# CSS Variables 1, 3.1: a declaration whose var() substitution fails, or
# whose substituted value the property does not take, is invalid at
# computed-value time: the property is unset (inherited properties
# inherit, the others take their initial value), whatever the earlier
# declarations said.
def unset_or_value(text, env, valid):
    """the value a declaration text with var() stands for: the
    substituted text when it is one the property takes (valid(text)),
    else "unset" (invalid at computed-value time)"""
    t = L111.subst(text, env)
    if t is None or not valid(t.strip()):
        return "unset"
    return t.strip()


# CSS Animations 1 and CSS Variables 1: var() in a keyframe of a
# registered custom property is the animated element's value below its
# own animations; invalid after substitution (no value, or not of the
# property's syntax): the frame's declaration is left out.
def reg_frame(text, env, matches):
    """a registered property's keyframe value with var(): the substituted
    text, None when the declaration is left out"""
    t = L111.subst(text, env) if "var(" in text else text
    if t is None:
        return None
    t = t.strip()
    if not t or "!" in t or not matches(t):
        return None
    return t
