"""Lot 110 model (US-031), written from the specifications, sharing no
code with the C (userspace/wf_sitecss.c, webeng_geom.inc).

- The body element as a box scroller (CSS Overflow 3, 3.3, overflow
  viewport propagation): the root element's overflow applies to the
  viewport; the body's applies to the viewport instead only when the
  root's is visible along both axes, and the element whose value is
  propagated is not a scroll container. So html never is a box scroller,
  and the body (the body element under the root html element) is one
  exactly when html's computed overflow is not visible/visible and its
  own computed value is a box value along one axis. Its timelines are
  then its own (scroll(nearest) of its descendants, scroll(self) and
  scroll-timeline-name on it, view timelines of the elements it holds),
  run on the body's geometry record; a box laid out in the body box
  scroller says sm = SM_BODY (the body has no mark).
- A percentage height, max-height or min-height of the body box scroller
  is of the root element's height (CSS 2, 10.5): html's height in px, or
  a percentage of the viewport height (the initial containing block),
  truncated at each step, at most HMAX; otherwise it is passed as it is.
"""
import lot105_model as M5

SM_BODY = 193       # WF_GEOM_MAX + 1
VH = 480            # the test viewport height


class Unsure(M5.Unsure):
    """a value the exact model cannot place against the C's rounding"""


def is_body(nodes, q):
    nd = nodes[q]
    p = nd["parent"]
    return nd["tag"] == "body" and (p < 0 or (nodes[p]["parent"] < 0 and nodes[p]["tag"] == "html"))


def body_own(nodes, i):
    """1 when node i is the body under the root html element and html's
    computed overflow is not visible along both axes"""
    p = nodes[i]["parent"]
    if p < 0 or not is_body(nodes, i):
        return False
    ox, oy = nodes[p]["sc"][0], nodes[p]["sc"][1]
    return M5.ov_axis(ox, oy) != 1 or M5.ov_axis(oy, ox) != 1


def box_scroller(nodes, i):
    """the computed y code when node i is a scroll container, else 0"""
    nd = nodes[i]
    if nd["tag"] == "html" or (nd["tag"] == "body" and not body_own(nodes, i)):
        return 0
    ox, oy = nd["sc"][0], nd["sc"][1]
    x, y = M5.ov_axis(ox, oy), M5.ov_axis(oy, ox)
    if x not in (2, 4, 5) and y not in (2, 4, 5):
        return 0
    return y


def sm_of(nodes, s):
    """the sm a box laid out in box scroller s carries"""
    return SM_BODY if is_body(nodes, s) else s + 1


def box_of(nodes, q, g):
    """the geometry box of node q (the body record for the body), None"""
    if not g:
        return None
    return g.get("body") if is_body(nodes, q) else g["box"].get(q)


def body_pct(nodes, i, h, vh=VH, ax=0):
    """box_len value h (value, pct) of node i with a body percentage made
    px against html's height. Lot 111, converted: html's height may be an
    expression with a percentage, and ax 1 takes a width against html's
    width (lot111_model.html_len)"""
    import lot107_model as L7
    import lot111_model as L111
    if h is None or h[1] is not True or not is_body(nodes, i) or nodes[i]["parent"] < 0:
        return h
    p = nodes[i]["parent"]
    sc = nodes[p]["sc"]
    em, rem = nodes[p].get("fs", 16), nodes[p].get("rem", 16)
    if ax:
        hv = L111.html_len(sc[6], em, rem, L111.VW) if len(sc) > 6 else None
    elif len(sc) > 3:
        hv = L111.html_len(sc[3], em, rem, vh)
    else:
        hv = sc[2] - 1 if sc[2] else None
    if hv is None:
        return h
    hv = min(hv, M5.HMAX)
    return (min(hv * h[0] // 100, M5.HMAX), False)


# ------------------------------------------------------------- opacity
# Opacity (CSS Color 4, 3.2): <alpha-value> = <number> | <percentage>;
# computed value: the number, clamped to [0, 1]; animation type: by
# computed value (as a number). The reader keeps 1/10000 (CSS Values 4:
# the value is truncated toward zero past the fourth decimal of the
# number); keyframe values are computed (clamped) when taken, the value
# after the animations and transitions is clamped again (add and
# accumulate may pass 1), and the reader writes whole percents, truncated
# (the engine keeps whole percents).
import re as _re
from fractions import Fraction as _F

OP_ALTS = [("<number>", "", None)]
_OP = _re.compile(r"^[+-]?(\d+(\.\d+)?|\.\d+)%?$")


def op_num(text):
    """an opacity text (lower case, trimmed) in 1/10000, None when it is
    not a number or a percentage"""
    if not _OP.match(text):
        return None
    pct = text.endswith("%")
    v = _F(text[:-1] if pct else text)
    if pct:
        v /= 100
    v = min(max(v, _F(0)), _F(1))
    return int(v * 10000)       # v >= 0: truncation is the floor


def op_text(v):
    if v >= 10000:
        return "1"
    if v <= 0:
        return "0"
    return ("0.%04d" % v).rstrip("0")


def op_decl(v):
    """op_decl_lot110 converted by Lot 111: a math function of a number
    or a percentage is a value too (lot111_model.op_math)"""
    got = op_decl_lot110(v)
    if got is None and v not in ("revert", "revert-layer"):
        import lot111_model as L111
        n = L111.op_math(v)
        if n is not None:
            return (2, n)
    return got


def op_decl_lot110(v):
    """("val", code, value) of one opacity declaration value or None:
    code 1 initial/unset, 2 a value, 3 inherit"""
    if v == "inherit":
        return (3, 10000)
    if v in ("initial", "unset"):
        return (1, 10000)
    n = op_num(v)
    return None if n is None else (2, n)


def op_cascade(decls):
    """(code, value, important) of the winning declaration of
    [(rank, value, important)], (0, 10000, False) when none"""
    best = None
    for rank, v, imp in decls:
        # Lot 111: revert and revert-layer win like a value; with author
        # rules only and no layer they leave nothing (no user-agent
        # opacity)
        got = (4, 0) if v in ("revert", "revert-layer") else op_decl(v)
        if got is None:
            continue
        if best is None or rank >= best[0]:
            best = (rank, got, imp)
    if best is None or best[1][0] == 4:
        return (0, 10000, False)
    return (best[1][0], best[1][1], best[2])


def op_trans(tl, start, end, ms):
    """the opacity text of the transition in tl from start to end at ms,
    None when none runs"""
    import lot97_animmodel as A
    S = 10000
    tr = None
    for x in tl:
        if x["prop"] in ("all", "opacity"):
            tr = x
    if tr is None or start == end:
        return None
    if A.interp(OP_ALTS, start, end, 5000)[1] == 1 and not tr["discrete"]:
        return None
    e = ms - tr["delay"]
    if e >= tr["dur"]:
        return None
    if e < 0:
        return start
    return A.interp(OP_ALTS, start, end, A.ease_at(tr["ease"], e * S // tr["dur"]))[0]


def op_final(own, parent_final, anims, kfs, ms, start=None, tl=()):
    """(final value or None when nothing is written, animated,
    transitioned, composed) of one element: own = op_cascade(...),
    parent_final the parent's final value (None: 1), kfs: name -> frames
    of opacity (values as op_text), start the starting value or None"""
    import lot98_animmodel as M98
    code, v, imp = own
    if code == 3:
        v = parent_final if parent_final is not None else 10000
    elif code != 2:
        v = 10000
    base = op_text(v)
    cur, animated, composed = (base, False, False) if imp else M98.stack(OP_ALTS, anims, kfs, base, ms)
    trans = False
    if not animated and start is not None:
        t = op_trans(list(tl), op_text(start), cur, ms)
        if t is not None:
            cur, trans = t, True
    if animated or trans:
        n = op_num(cur)
        if n is None:
            raise Unsure()
        return n, animated, trans, composed
    return (v if code >= 2 else None), False, False, composed
