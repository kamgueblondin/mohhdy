#!/usr/bin/env python3
"""Lot 102 model: the none keyword in colours, the anonymous scroll() and
view() timelines, animation-range and timeline-scope.

Written from CSS Color 4 (missing components), CSS Scroll-driven
Animations 1 and docs/lot102_plan.md, not from the C sources.
lot100_colormodel.rgba, lot100_colormodel.colour_match,
lot97_animmodel.interp_colour, lot98_animmodel.stack,
lot99_animmodel.timeline_one, lot99_animmodel.idle and
lot101_colormodel.clock are converted to consult this model.

Colours
* none is a component of rgb(), rgba(), hsl(), hsla() (the space forms
  only: with commas the colour is refused), hwb(), lab(), lch(), oklab(),
  oklch() and color(); it converts as 0. It is a missing component: the
  red, green and blue of rgb() and every component of color() (the rgb
  and xyz components are the analogues of red, green and blue), and the
  alpha of any of them. Other missing components (hue, saturation,
  lightness, whiteness, blackness, L, a, b, C, H) are not kept: the srgb
  space has no analogue for them.
* Mixing (color-mix(in srgb, ...)) and interpolating (animations,
  transitions) two colours: a component missing in one takes the other's
  value, missing in both it is 0 in both; a mix keeps the components
  missing in both as missing (for a mix inside a mix). Adding (animation
  composition add and accumulate) takes the values as they convert.
* A registered <color> takes rgb(), rgba(), hsl() and hsla() with a none
  by their conversion.

Timelines
* animation-timeline: scroll([nearest | root | self] || <axis>)
  (defaults nearest and block) and view(<axis> || [auto |
  <length-percentage>]{1,2}). scroll() runs on the reader viewport: root
  always; nearest for every element but the root (no other element is a
  scroll container); self only on the root html element; axis block or y.
  view() has no effect (no layout).
* animation-range-start / animation-range-end: [normal | <percentage> |
  0 | <range-name> <percentage>?]#, a range name alone 0% for a start and
  100% for an end; the shorthand animation-range takes [<start> <end>?]#,
  an end left out being the start's range name at 100% (normal after
  normal or a percentage). The animation shorthand resets both. On a
  scroll timeline a range name stands for normal (0% start, 100% end);
  the local time is (delay + duration * count) (100 pm - S) / (E - S),
  floored, within -2e8..2e8; E <= S gives no effect.
* timeline-scope: none | all | <dashed-ident># (at most 4). Looking for a
  named timeline from an element up: an element defining the name decides
  (Lot 101); else an element whose timeline-scope lists the name, or is
  all while a descendant (with no element between scoping the name)
  defines it, ends the search with no effect (that timeline belongs to a
  descendant, which is never the root).
"""
import os
import sys
from fractions import Fraction as F

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
import lot95_propmodel as P  # noqa: E402
import lot97_animmodel as A  # noqa: E402
import lot98_animmodel as M98  # noqa: E402

S = 10000
SP = " \t\n\r"
VAL = 128
NONE_FNS = ("rgba(", "rgb(", "hsla(", "hsl(", "hwb(", "lab(", "lch(", "oklab(", "oklch(", "color(")
CHANNEL_FNS = ("rgba(", "rgb(", "color(")


def rhalf(fr):
    return M98.rhalf(fr) if hasattr(M98, "rhalf") else A.rhalf(fr)


# ------------------------------------------------------------- none
def fn_of(lv):
    for f in NONE_FNS:
        if lv.startswith(f):
            return f
    return None


def split_fn(lv, f):
    """(head, argument text) of a colour function"""
    if f == "color(":
        body = lv[6:-1].lstrip(SP)
        i = 0
        while i < len(body) and body[i] not in SP:
            i += 1
        return "color(" + body[:i], body[i:]
    return f, lv[len(f):-1]


def none_fn(lv, f):
    """(channels, alpha, missing) of a colour function with a none token,
    None when invalid, NotImplemented without a none token"""
    head, rest = split_fn(lv, f)
    got = M98.colour_args(rest)
    if got is None:
        return NotImplemented
    toks, comma = got
    if "none" not in toks:
        return NotImplemented
    if comma:
        return None
    miss = 0
    for k, t in enumerate(toks):
        if t == "none" and (k == 3 or f in CHANNEL_FNS):
            miss |= 1 << k
    nt = ["0" if t == "none" else t for t in toks]
    text = head + ("" if head == f else " ") + " ".join(nt[:3]) + (" / " + nt[3] if len(nt) == 4 else "") + ")"
    import lot100_colormodel as L100
    r = L100.rgba_base(text)
    if r is None:
        return None
    return list(r[0]), r[1], miss


def carry(x, y):
    """CSS Color 4 missing components of two (channels, alpha, missing)"""
    vx, vy = list(x[0]) + [x[1]], list(y[0]) + [y[1]]
    for k in range(4):
        bx, by = (x[2] >> k) & 1, (y[2] >> k) & 1
        if bx and by:
            vx[k] = vy[k] = 0
        elif bx:
            vx[k] = vy[k]
        elif by:
            vy[k] = vx[k]
    return (vx[:3], vx[3], x[2]), (vy[:3], vy[3], y[2])


def mix_item(t, depth):
    import lot100_colormodel as L100
    toks = L100.split_top(t, False)
    if toks is None or len(toks) > 2:
        return None
    p = None
    c = toks[0]
    if len(toks) == 2:
        q = 0 if toks[0].endswith("%") else 1
        r = L100.num_pct(toks[q])
        if r is None or not r[1] or not 0 <= r[0] <= 100 * S:
            return None
        p, c = r[0], toks[1 - q]
    if len(c) >= VAL:
        return None
    col = rgbam(c, depth + 1)
    if col is None:
        return None
    return col, p


def mixm(v, depth):
    """Lot 100's color-mix() with missing components"""
    import lot100_colormodel as L100
    if depth > 3 or not (v.startswith("color-mix(") and v.endswith(")") and len(v) > 11):
        return None
    parts = L100.split_top(v[10:-1], True)
    if parts is None or len(parts) != 3:
        return None
    m = parts[0]
    if len(m) < 7 or m[:2] != "in" or m[2] not in SP or m[2:].lstrip(SP) != "srgb":
        return None
    items = []
    for t in parts[1:]:
        it = mix_item(t, depth)
        if it is None:
            return None
        items.append(it)
    (x, p0), (y, p1) = items
    x, y = carry(x, y)
    if p0 is None and p1 is None:
        p0 = p1 = 50 * S
    elif p0 is None:
        p0 = 100 * S - p1
    elif p1 is None:
        p1 = 100 * S - p0
    tot = p0 + p1
    if tot <= 0:
        return None
    an = x[1] * p0 + y[1] * p1
    ch = [rhalf(F(x[0][k] * x[1] * p0 + y[0][k] * y[1] * p1, an)) if an else 0 for k in range(3)]
    return ch, rhalf(F(an, max(tot, 100 * S))), x[2] & y[2]


def rgbam(v, depth=0):
    """(channels, alpha, missing bits) of any reader colour, or None"""
    import lot100_colormodel as L100
    lv = v.lower()
    if lv.startswith("color-mix("):
        return mixm(lv, depth)
    f = fn_of(lv)
    if f and lv.endswith(")") and len(lv) > len(f):
        r = none_fn(lv, f)
        if r is not NotImplemented:
            return r
    r = L100.rgba_base(v, depth)
    return None if r is None else (list(r[0]), r[1], 0)


def rgba(v, depth=0):
    r = rgbam(v, depth)
    return None if r is None else (r[0], r[1])


def carried(a, b):
    """the two colours of an interpolation after the missing components,
    as (channels, alpha), or None"""
    x, y = rgbam(a), rgbam(b)
    if x is None or y is None:
        return None
    x, y = carry(x, y)
    return (x[0], x[1]), (y[0], y[1])


def none_match(v):
    """a registered <color> rgb(), rgba(), hsl() or hsla() with none: True
    or False, None when the value is not one of these"""
    lv = v.lower()
    for f in ("rgb(", "rgba(", "hsl(", "hsla("):
        if lv.startswith(f) and len(v) > len(f):
            if "none" in lv[len(f):]:
                return rgba(v) is not None
            return None
    return None


# ------------------------------------------------------------- timelines
AXES = ["block", "inline", "x", "y"]
SCROLLERS = ["nearest", "root", "self"]
RANGES = ["cover", "contain", "entry", "exit", "entry-crossing", "exit-crossing", "scroll"]
LIM = 2 * 100000000


def inset_tok(t):
    if t == "auto":
        return True
    if len(t) >= 32:
        return False
    q = P.leaf(t, None)
    if q is None or not q.ok:
        return False
    if q.k == "N":
        return P.NUM.match(t).end() == len(t) and q.v == 0
    return q.k in ("L", "P")


def anon_tl(w):
    """("scroll", scroller, axis) or ("view", axis) for scroll() or view(),
    None when invalid, NotImplemented for another token"""
    if len(w) >= 8 and w.startswith("scroll("):
        view, inner = False, w[7:-1]
    elif len(w) >= 6 and w.startswith("view("):
        view, inner = True, w[5:-1]
    else:
        return NotImplemented
    if not w.endswith(")"):
        return None
    toks = A.tokens(inner)
    if toks is None:
        return None
    src = axis = None
    inset, closed = 0, False
    for t in toks:
        if t in AXES and axis is None:
            axis, closed = t, inset > 0
            continue
        if not view:
            if t not in SCROLLERS or src is not None:
                return None
            src = t
            continue
        if closed or inset >= 2 or not inset_tok(t):
            return None
        inset += 1
    if view:
        return ("view", axis or "block")
    return ("scroll", src or "nearest", axis or "block")


def range_pct(t):
    r = M98.num_pct(t) if t and len(t) < 32 else None
    if r is None:
        return None
    return r[0] if r[1] or r[0] == 0 else None


def range_edge(toks, i, end):
    """(edge, next index) or None; edge: ("normal", 0), ("pct", p) or
    (range name, p)"""
    if i >= len(toks):
        return None
    t = toks[i]
    if t == "normal":
        return ("normal", 0), i + 1
    p = range_pct(t)
    if p is not None:
        return ("pct", p), i + 1
    if t not in RANGES:
        return None
    if i + 1 < len(toks) and range_pct(toks[i + 1]) is not None:
        return (t, range_pct(toks[i + 1])), i + 2
    return (t, 100 * S if end else 0), i + 1


def range_long(v, end):
    """the edges of animation-range-start (end False) or -end, or None"""
    items = A.list_items(v)
    if items is None or len(items) > 4:
        return None
    out = []
    for it in items:
        toks = A.tokens(it)
        if not toks:
            return None
        r = range_edge(toks, 0, end)
        if r is None or r[1] != len(toks):
            return None
        out.append(r[0])
    return out


def range_short(v):
    """the (start, end) edges of the animation-range shorthand, or None"""
    items = A.list_items(v)
    if items is None or len(items) > 4:
        return None
    out = []
    for it in items:
        toks = A.tokens(it)
        if not toks:
            return None
        r = range_edge(toks, 0, False)
        if r is None:
            return None
        st, i = r
        if i == len(toks):
            en = (st[0], 100 * S) if st[0] in RANGES else ("normal", 0)
        else:
            r = range_edge(toks, i, True)
            if r is None or r[1] != len(toks):
                return None
            en = r[0]
        out.append((st, en))
    return out


def edge_text(e):
    if e[0] == "normal":
        return "normal"
    t = P.fmt(e[1], "%")
    return t if e[0] == "pct" else e[0] + " " + t


def range_text(edges):
    return ", ".join(edge_text(e) for e in edges)


def range_ms(a, pm):
    """the local time of animation a on a scroll timeline at pm within its
    range, or None"""
    import lot101_colormodel as L101
    if L101.scroll_ms(a, pm) is None:
        return None
    rs, re_ = a.get("rs", ("normal", 0)), a.get("re", ("normal", 0))
    st = rs[1] if rs[0] == "pct" else 0
    en = re_[1] if re_[0] == "pct" else 100 * S
    if en <= st:
        return None
    s = a["delay"] * S + a["dur"] * a["iters"]
    t = (s * (pm * 100 - st)) // ((en - st) * S)
    return max(-LIM, min(LIM, t))


def scope_list(v):
    """timeline-scope: "all", a list of names ([] for none), or None"""
    items = A.list_items(v)
    if items is None or len(items) > 4:
        return None
    out = []
    for it in items:
        toks = A.tokens(it)
        if toks is None or len(toks) != 1:
            return None
        t = toks[0]
        if t in ("all", "none"):
            if len(items) > 1:
                return None
            return "all" if t == "all" else []
        if len(t) < 3 or not t.startswith("--") or not A.is_ident(t) or len(t) >= 32:
            return None
        out.append(t)
    return out


def scoped(v, name):
    """wf_csstl_scoped: 1 listed, 2 all, else 0"""
    if not name:
        return 0
    r = scope_list(v)
    if r == "all":
        return 2
    return 1 if r and name in r else 0


# A tree: a list of nodes {"parent": index or -1, "tag": name, "tl": [the
# five slots: scroll names, scroll axes, view names, view axes (Lot 101
# lists of (name, axis), or None), scope (scope_list, or None)]}
def defines(nodes, j, name):
    import lot101_colormodel as L101
    tl = nodes[j]["tl"]
    for s in (0, 1):
        if not tl[2 * s]:
            continue
        got = [x for x in L101.tl_coord(tl[2 * s], tl[2 * s + 1]) if x[0] == name]
        if got:
            return s + 1, got[-1][1]
    return None


def scope_of(nodes, j, name):
    sc = nodes[j]["tl"][4]
    if sc is None:
        return 0
    if sc == "all":
        return 2
    return 1 if name in sc else 0


def scoped_at(nodes, q, name):
    s = scope_of(nodes, q, name)
    if s != 2:
        return s
    for j in range(len(nodes)):
        if j == q or not defines(nodes, j, name):
            continue
        e = nodes[j]["parent"]
        while e >= 0 and e != q and not scope_of(nodes, e, name):
            e = nodes[e]["parent"]
        if e == q:
            return 1
    return 0


def clock(a, nodes, i, ms, pm):
    """("auto", ms), ("scroll", local ms) or None for animation a on node
    i"""
    tl = a.get("timeline", "auto")
    if tl == "auto":
        return ("auto", ms)
    nd = nodes[i]
    if isinstance(tl, tuple):
        if tl[0] == "view":
            return None
        _, src, axis = tl
        root = nd["parent"] < 0 and nd["tag"] == "html"
        if (src == "self" and not root) or (src == "nearest" and nd["parent"] < 0) or axis not in ("block", "y"):
            return None
        t = range_ms(a, pm)
        return None if t is None else ("scroll", t)
    if tl == "none":
        return None
    q = i
    while q >= 0:
        d = defines(nodes, q, tl)
        if d:
            n = nodes[q]
            if d[0] == 2 or n["parent"] >= 0 or n["tag"] != "html" or d[1] not in ("block", "y"):
                return None
            t = range_ms(a, pm)
            return None if t is None else ("scroll", t)
        if scoped_at(nodes, q, tl):
            return None
        q = nodes[q]["parent"]
    return None


# the element of the Lot 99 and Lot 100 sheets: #o, a child of body, no
# timeline names; the scroll position of wf_site_set_scroll (0 there)
SIMPLE = {"pm": 0}


def simple_clock(a, ms):
    nodes = [{"parent": -1, "tag": "html", "tl": [None] * 5}, {"parent": 0, "tag": "body", "tl": [None] * 5},
             {"parent": 1, "tag": "div", "tl": [None] * 5}]
    return clock(a, nodes, 2, ms, SIMPLE["pm"])


def stack(alts, anims, kfs, under, ms, nodes, i, pm):
    """lot98_animmodel.stack with the clock of each animation"""
    animated = composed = False
    base = under
    for a in anims:
        if not a["name"] or a["name"] not in kfs:
            continue
        c = clock(a, nodes, i, ms, pm)
        if c is None:
            continue
        v, cp = M98.anim_value(alts, a, kfs[a["name"]], under, c[1], base)
        if v is not None:
            under, animated = v, True
            composed = composed or cp
    return under, animated, composed


def counts(anims, names, nodes, i, ms, pm):
    """(running on a scroll timeline, idle) of the animations whose name has
    @keyframes"""
    sc = idle = 0
    for a in anims:
        if not a["name"] or a["name"] not in names:
            continue
        c = clock(a, nodes, i, ms, pm)
        if c is None:
            idle += 1
        elif c[0] == "scroll":
            sc += 1
    return sc, idle


# ------------------------------------------------------------- cascade
ANIM_LONGS = ["animation-name", "animation-duration", "animation-timing-function", "animation-delay",
              "animation-iteration-count", "animation-direction", "animation-fill-mode", "animation-play-state",
              "animation-composition", "animation-timeline", "animation-range-start", "animation-range-end"]


def anim_cascade(rules):
    """the twelve animation longhand slots after the rules [(spec,
    [(prop, value)])]: None, ("sh", text) or ("long", text)"""
    import lot99_animmodel as M99
    best = [None] * 12
    order = 0
    for spec, decls in rules:
        for prop, val in decls:
            val = val.strip().lower()
            order += 1
            if len(val) >= 128:
                continue
            setv = None
            if prop == "animation":
                ks = list(range(8)) + [9, 10, 11]
                if val in ("initial", "unset", "none"):
                    setv = [(k, None) for k in ks]
                elif A.anim_parse(val):
                    setv = [(k, ("sh", val)) for k in ks]
            elif prop == "animation-range":
                if val in ("initial", "unset"):
                    setv = [(10, None), (11, None)]
                else:
                    r = range_short(val)
                    if r:
                        t0, t1 = range_text([x[0] for x in r]), range_text([x[1] for x in r])
                        if len(t0) < 128 and len(t1) < 128:
                            setv = [(10, ("long", t0)), (11, ("long", t1))]
            elif prop in ANIM_LONGS:
                k = ANIM_LONGS.index(prop)
                if val in ("initial", "unset"):
                    setv = [(k, None)]
                elif (M99.long_list(k, val) if k < 10 else range_long(val, k == 11)) is not None:
                    setv = [(k, ("long", val))]
            for k, s in setv or []:
                if best[k] is None or (spec, order) >= best[k][0]:
                    best[k] = ((spec, order), s)
    return [b[1] if b else None for b in best]


def coord(slots):
    """the coordinated animations of the twelve slots"""
    import lot99_animmodel as M99
    anims = M99.coord(slots[:10])
    for k, f in ((10, "rs"), (11, "re")):
        s = slots[k]
        lst = None
        if s is not None and s[0] == "long":
            lst = range_long(s[1], k == 11)
        if not lst:
            lst = [("normal", 0)]
        for j, a in enumerate(anims):
            a[f] = lst[j % len(lst)]
    return anims


TL_PROPS = ["scroll-timeline-name", "scroll-timeline-axis", "view-timeline-name", "view-timeline-axis"]


def tl_cascade(decls):
    """the five timeline slots of one element after its declarations (Lot
    101's four and timeline-scope)"""
    import lot101_colormodel as L101
    slots = L101.tl_cascade([d for d in decls if d[0] != "timeline-scope"]) + [None]
    for prop, val in decls:
        if prop != "timeline-scope":
            continue
        v = val.strip(SP).lower()
        if v in ("initial", "unset"):
            slots[4] = None
            continue
        r = scope_list(v)
        if r is not None:
            slots[4] = r
    return slots
