#!/usr/bin/env python3
"""Lot 104 model: view progress timelines from the layout geometry.

Written from CSS Scroll-driven Animations 1 (sections 3 and 4) and
docs/lot104_plan.md, not from the C sources. lot102_model.range_edge,
range_ms and anon_tl are converted (lengths in animation-range, the
insets of view()).

Geometry
* The layout (webeng.c) hands back, for each element a view timeline
  needs (a subject), its border box along the block and inline axes from
  the content start of its nearest scroller, the scrollport size, scroll
  offset and scroll range of that scroller, and whether the scroller is
  the root (wf_geom_t). The reader uses the root scroller only: it
  scrolls along y to pm / 10000 of its vertical range (wf_site_set_scroll)
  and stays at 0 along x. A subject in a box scroller, or with no box,
  leaves its timeline inactive. Positions here are in 1/1000000 px.

Timelines
* view-timeline-inset: [[auto | <length-percentage>]{1,2}]# (one value is
  both), the insets of the view-timeline-name list item of the same index
  (repeated as the axes are); view-timeline: [<name> [<axis> ||
  <inset>{1,2}]?]# sets the name, axis and inset lists; view(<axis> ||
  <inset>{1,2}). An inset is auto (0: no scroll-padding), an absolute
  length, or a percentage of the scrollport (rounded down).
* With S the subject start, E its end, V the scrollport, is and ie the
  insets: cover [S - V + ie, E - is]; contain from E - V + ie and S - is
  (the smaller first); entry [cover start, contain start]; exit [contain
  end, cover end]; entry-crossing [S - V + ie, E - V + ie]; exit-crossing
  [S - is, E - is]; scroll [0, scroll range].
* animation-range edges: normal is cover 0% (start) or 100% (end); a
  percentage or a length alone is on cover; a percentage p of [lo, hi] is
  lo + floor(p (hi - lo)), a length l is lo + l. The local time is
  floor((delay + duration x count) (pos - St) / (En - St)) within
  -2e8..2e8 ms; no effect for an infinite count, a total outside 1..1e8
  ms, or En <= St.
* A scroll timeline with a length edge runs on the root scroll range
  (needs the geometry, range > 0): its cover range is [0, range], range
  names count as normal. Without a length edge the Lot 102 formula holds.
"""
import os
import sys

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
import lot95_propmodel as P  # noqa: E402
import lot97_animmodel as A  # noqa: E402
import lot101_colormodel as L101  # noqa: E402
import lot102_model as L102  # noqa: E402

S = 10000
U = 1000000          # positions: 1/1000000 px
LIM = 2 * 100000000
SP = " \t\n\r"
AXES = L102.AXES
RANGES = L102.RANGES
AUTO = ("auto", 0)
LONG = 128           # WF_SITE_LONG


# ------------------------------------------------------------- view math
def inset_px(view, ins):
    k, v = ins
    if k == "len":
        return v * 100
    if k == "pct":
        return view * v // U
    return 0


def view_range(v, rng):
    """(lo, hi) of range rng on the view timeline v (dict start, size,
    view, range, pos, ins)"""
    s, e, vw = v["start"], v["start"] + v["size"], v["view"]
    is_, ie = inset_px(vw, v["ins"][0]), inset_px(vw, v["ins"][1])
    a, b = e - vw + ie, s - is_
    if a > b:
        a, b = b, a
    if rng == "cover":
        return s - vw + ie, e - is_
    if rng == "contain":
        return a, b
    if rng == "entry":
        return s - vw + ie, a
    if rng == "exit":
        return b, e - is_
    if rng == "entry-crossing":
        return s - vw + ie, e - vw + ie
    if rng == "exit-crossing":
        return s - is_, e - is_
    return 0, v["range"]


def view_edge(v, e, end):
    lo, hi = view_range(v, e[0] if e[0] in RANGES else "cover")
    if e[0] == "normal":
        return hi if end else lo
    if len(e) > 2:
        return lo + e[1] * 100
    return lo + (hi - lo) * e[1] // U


def trunc_div(n, d):
    q = abs(n) // abs(d)
    return q if (n >= 0) == (d > 0) else -q


def span_ms(a, st, en, pos):
    if a["iters"] < 0:
        return None
    s = a["delay"] * S + a["dur"] * a["iters"]
    if s <= 0 or s > 100000000 * S or en <= st:
        return None
    num, den = s * (pos - st), (en - st) * S
    q = trunc_div(num, den)
    if q < -LIM:
        return -LIM
    if q > LIM:
        return LIM
    return max(-LIM, num // den)


def view_ms(a, v):
    rs, re_ = a.get("rs", ("normal", 0)), a.get("re", ("normal", 0))
    return span_ms(a, view_edge(v, rs, False), view_edge(v, re_, True), v["pos"])


def scroll_px_ms(a, rng, pos):
    if rng <= 0:
        return None
    v = {"start": 0, "size": rng, "view": 0, "range": rng, "pos": pos, "ins": (AUTO, AUTO)}
    rs, re_ = a.get("rs", ("normal", 0)), a.get("re", ("normal", 0))
    rs = ("normal", 0) if rs[0] in RANGES else rs
    re_ = ("normal", 0) if re_[0] in RANGES else re_
    return span_ms(a, view_edge(v, rs, False), view_edge(v, re_, True), pos)


def has_len(a):
    return len(a.get("rs", ("normal", 0))) > 2 or len(a.get("re", ("normal", 0))) > 2


# ------------------------------------------------------------- lists
def inset_list(v):
    """view-timeline-inset: the list of (start, end) insets, or None"""
    items = A.list_items(v)
    if items is None or len(items) > 4:
        return None
    out = []
    for it in items:
        toks = A.tokens(it)
        if not toks or len(toks) > 2:
            return None
        ins = [L102.inset_val(t) for t in toks]
        if None in ins:
            return None
        out.append((ins[0], ins[-1]))
    return out


def vshort_list(v):
    """view-timeline: the list of (name, axis, insets), or None"""
    items = A.list_items(v)
    if items is None or len(items) > 4:
        return None
    out = []
    for it in items:
        toks = A.tokens(it)
        if not toks:
            return None
        name = L101.tl_name(toks[0])
        if name is None:
            return None
        axis, ins, closed = None, [], False
        for t in toks[1:]:
            if t in AXES and axis is None:
                axis, closed = t, bool(ins)
                continue
            iv = L102.inset_val(t)
            if closed or len(ins) >= 2 or iv is None:
                return None
            ins.append(iv)
        ins = ins or [AUTO]
        out.append((name, axis or "block", (ins[0], ins[-1])))
    return out


def inset_text(ins):
    def one(x):
        if x[0] == "auto":
            return "auto"
        return P.fmt(x[1], "%" if x[0] == "pct" else "px")
    t = one(ins[0])
    return t if ins[0] == ins[1] else t + " " + one(ins[1])


def insets_text(lst):
    return ", ".join(inset_text(x) for x in lst)


def coord_ex(names, axes, insets):
    """(name, axis, insets) of the view-timeline-name list"""
    if not names:
        return []
    return [(n, axes[k % len(axes)][1] if axes else "block", insets[k % len(insets)] if insets else (AUTO, AUTO))
            for k, (n, _) in enumerate(names)]


# The six timeline slots of one element: scroll names, scroll axes, view
# names, view axes (Lot 101 lists of (name, axis)), scope (Lot 102), view
# insets (a list of (start, end)), or None
TL_PROPS = ["scroll-timeline-name", "scroll-timeline-axis", "view-timeline-name", "view-timeline-axis"]


def tl_cascade(decls):
    """the six timeline slots after the declarations of one rule, in order"""
    slots = [None] * 6
    for prop, val in decls:
        v = val.strip(SP).lower()
        ini = v in ("initial", "unset")
        if prop in TL_PROPS:
            k = TL_PROPS.index(prop)
            if ini:
                slots[k] = None
                continue
            r = L101.tl_list("axes" if k & 1 else "names", v)
            if r:
                slots[k] = r
        elif prop == "scroll-timeline":
            if ini:
                slots[0] = slots[1] = None
                continue
            r = L101.tl_list("short", v)
            if r:
                slots[0] = [(n, "block") for n, _ in r]
                slots[1] = [("", x) for _, x in r]
        elif prop == "view-timeline":
            if ini:
                slots[2] = slots[3] = slots[5] = None
                continue
            r = vshort_list(v)
            if r:
                ins = [x[2] for x in r]
                names = ", ".join(n or "none" for n, _, _ in r)
                if len(names) < LONG and len(insets_text(ins)) < LONG:
                    slots[2] = [(n, "block") for n, _, _ in r]
                    slots[3] = [("", x) for _, x, _ in r]
                    slots[5] = ins
        elif prop == "view-timeline-inset":
            if ini:
                slots[5] = None
                continue
            r = inset_list(v)
            if r:
                slots[5] = r
        elif prop == "timeline-scope":
            if ini:
                slots[4] = None
                continue
            r = L102.scope_list(v)
            if r is not None:
                slots[4] = r
    return slots


# ------------------------------------------------------------- clock
def defines(nodes, j, name):
    """(1 scroll or 2 view, axis, insets) when node j defines the name"""
    tl = nodes[j]["tl"]
    for s in (0, 1):
        if not tl[2 * s]:
            continue
        got = [x for x in coord_ex(tl[2 * s], tl[2 * s + 1], tl[5] if s else None) if x[0] == name]
        if got:
            return s + 1, got[-1][1], got[-1][2]
    return None


def scoped_at(nodes, q, name):
    s = L102.scope_of(nodes, q, name)
    if s != 2:
        return s
    for j in range(len(nodes)):
        if j == q or not defines(nodes, j, name):
            continue
        e = nodes[j]["parent"]
        while e >= 0 and e != q and not L102.scope_of(nodes, e, name):
            e = nodes[e]["parent"]
        if e == q:
            return 1
    return 0


class Ctx:
    """the geometry (None, or {"rg_y": .., "box": {index: box}}; a box
    {"root", "bvert", "b_off", "b_size", "i_off", "i_size", "vp_b",
    "vp_i", "rg_b", "rg_i"}), the scroll position, and what the clocks
    asked for"""

    def __init__(self, geom, pm):
        self.geom, self.pm = geom, pm
        self.need, self.root = set(), False


def scroll_clock(a, c):
    if not has_len(a):
        t = L102.range_ms(a, c.pm)
        return None if t is None else ("scroll", t)
    c.root = True
    g = c.geom
    if not g or g["rg_y"] <= 0:
        return None
    t = scroll_px_ms(a, g["rg_y"] * U, c.pm * g["rg_y"] * 100)
    return None if t is None else ("scroll", t)


def view_clock(a, q, axis, ins, c):
    c.need.add(q)
    g = c.geom
    b = g["box"].get(q) if g else None
    if not b or not b["root"]:
        return None
    useb = axis == "block" or (axis == "y" and b["bvert"]) or (axis == "x" and not b["bvert"])
    vert = b["bvert"] if useb else not b["bvert"]
    p = "b" if useb else "i"
    rg = b["rg_" + p]
    v = {"start": b[p + "_off"] * U, "size": b[p + "_size"] * U, "view": b["vp_" + p] * U, "range": rg * U,
         "pos": c.pm * rg * 100 if vert else 0, "ins": ins}
    t = view_ms(a, v)
    return None if t is None else ("view", t)


def clock(a, nodes, i, ms, c):
    """("auto", ms), ("scroll", t), ("view", t) or None for animation a on
    node i"""
    tl = a.get("timeline", "auto")
    if tl == "auto":
        return ("auto", ms)
    nd = nodes[i]
    if isinstance(tl, tuple):
        if tl[0] == "view":
            return view_clock(a, i, tl[1], tl[2], c)
        _, src, axis = tl
        root = nd["parent"] < 0 and nd["tag"] == "html"
        if (src == "self" and not root) or (src == "nearest" and nd["parent"] < 0) or axis not in ("block", "y"):
            return None
        return scroll_clock(a, c)
    if tl == "none":
        return None
    q = i
    while q >= 0:
        d = defines(nodes, q, tl)
        if d:
            n = nodes[q]
            if d[0] == 2:
                return view_clock(a, q, d[1], d[2], c)
            if n["parent"] >= 0 or n["tag"] != "html" or d[1] not in ("block", "y"):
                return None
            return scroll_clock(a, c)
        if scoped_at(nodes, q, tl):
            return None
        q = nodes[q]["parent"]
    return None


def stack(alts, anims, kfs, under, ms, nodes, i, c):
    import lot98_animmodel as M98
    animated = composed = False
    base = under
    for a in anims:
        if not a["name"] or a["name"] not in kfs:
            continue
        r = clock(a, nodes, i, ms, c)
        if r is None:
            continue
        v, cp = M98.anim_value(alts, a, kfs[a["name"]], under, r[1], base)
        if v is not None:
            under, animated = v, True
            composed = composed or cp
    return under, animated, composed


def counts(anims, names, nodes, i, ms, c):
    """(scroll, view, idle) of the animations whose name has @keyframes"""
    sc = vw = idle = 0
    for a in anims:
        if not a["name"] or a["name"] not in names:
            continue
        r = clock(a, nodes, i, ms, c)
        if r is None:
            idle += 1
        elif r[0] == "scroll":
            sc += 1
        elif r[0] == "view":
            vw += 1
    return sc, vw, idle
