"""Lot 108 model (US-031), written from the specifications, sharing no
code with the C (userspace/wf_sitecss.c, wf_cssprop.c, webeng_scroll.inc).

- Box scrollers along x (CSS Overflow 3): overflow-x auto or scroll with a
  width or max-width (CSS Box Sizing 3, the values of height and
  max-height: absolute lengths, percentages of the containing block, font
  and viewport units, math functions without a percentage) is passed on
  to the layout as "overflow-x:auto;width:Npx;max-width:Mpx"; min-height
  is passed with a box passed along y.
- Scroll timelines on the inline or x axis run on a box scroller's own
  scroll range and offset along x (CSS Scroll-driven Animations 1; the
  inline axis is x, the reader has no writing mode); the root scrolls
  along y only.
- Relative colour syntax (CSS Color 5, 4): rgb(), rgba(), hsl(), hsla(),
  oklab() and oklch() from an origin colour, with the channel keywords
  and calc() (see relcolor()).
"""
from fractions import Fraction as F

import lot107_model as L7

SX = ("width", "max-width", "min-height")


def sc_cascade(decls):
    """[ox, oy, hpx, hslot, mpx, mslot, wslot, mwslot, mnslot] of one
    element: lot107_model.sc_cascade and the width, max-width and
    min-height slots (height_val; max-width takes none)"""
    import lot105_model as M5
    best = [None] * 7
    val = [0, 0, ("kept",), ("kept",), ("kept",), ("kept",), ("kept",)]
    for rank, prop, v in decls:
        if prop in ("overflow", "overflow-x", "overflow-y"):
            got = M5.ov_list(prop, v)
            if got is None:
                continue
            for k in (0, 1):
                if got[k] is not None and (best[k] is None or rank >= best[k]):
                    best[k], val[k] = rank, got[k]
        elif prop in ("height", "max-height") + SX:
            got = L7.height_val(v, prop in ("max-height", "max-width"))
            if got is None:
                continue
            k = {"height": 2, "max-height": 3, "width": 4, "max-width": 5, "min-height": 6}[prop]
            if best[k] is None or rank >= best[k]:
                best[k], val[k] = rank, got
    h, m = val[2], val[3]
    return [val[0], val[1], h[1] if h[0] == "px" else 0, h, m[1] if m[0] == "px" else 0, m,
            val[4], val[5], val[6]]


def passed(nodes, i, scr):
    """the style text the reader adds for node i, a box scroller a
    timeline reads: along y (auto or scroll, a height or max-height
    passed; min-height with them), then along x (a width or max-width)"""
    import lot105_model as M5
    sc = nodes[i]["sc"]
    if i not in scr or not M5.box_scroller(nodes, i):
        return ""
    em = nodes[i].get("fs", 16)
    rem = nodes[i].get("rem", 16)
    slot = lambda k: L7.box_len(sc[k], em, rem) if len(sc) > k else None  # noqa: E731
    if len(sc) > 3:
        hs = [slot(3), slot(5)]
    else:
        hs = [(sc[2] - 1, False) if sc[2] else None, None]
    ox, oy = sc[0], sc[1]
    parts = []
    y = M5.ov_axis(oy, ox)
    if (hs[0] is not None or hs[1] is not None) and y in (4, 5):
        t = "overflow-y:%s" % M5.OV_NAME[y]
        for name, h in (("height", hs[0]), ("max-height", hs[1]), ("min-height", slot(8))):
            if h is not None:
                t += ";%s:%s" % (name, L7.box_text(h))
        parts.append(t)
    x = M5.ov_axis(ox, oy)
    ws = [slot(6), slot(7)]
    if (ws[0] is not None or ws[1] is not None) and x in (4, 5):
        t = "overflow-x:%s" % M5.OV_NAME[x]
        for name, w in (("width", ws[0]), ("max-width", ws[1])):
            if w is not None:
                t += ";%s:%s" % (name, L7.box_text(w))
        parts.append(t)
    return ";".join(parts)


# ------------------------------------------------------------ x scrolling
BAR = 6             # the bar thickness (the engine's choice, Lot 74)
ROW = 12            # the height of one row of the test page


def xflow_page(c):
    """the HTML of an x scrolling case c: a box #o (overflow-x, width W,
    padding L/R, border B, overflow-y and height H when given) holding the
    rows c["kids"]: ("w", C) a C px wide row, ("nest", C, D) a C px wide
    row holding a D px wide one, ("clip", C, D) the same clipping along x,
    ("none", C) a hidden row"""
    st = "overflow-x:%s;width:%s;padding:0 %dpx 0 %dpx" % (c["ovx"], c["wcss"], c["R"], c["L"])
    if c["B"]:
        st += ";border:%dpx solid #000" % c["B"]
    if c["ovy"]:
        st += ";overflow-y:%s" % c["ovy"]
    if c["H"] is not None:
        st += ";height:%dpx" % c["H"]
    rows = []
    for k, kid in enumerate(c["kids"]):
        rid = "r%d" % k
        if kid[0] == "w":
            rows.append('<div id="%s" style="width:%dpx;height:%dpx"></div>' % (rid, kid[1], ROW))
        elif kid[0] == "none":
            rows.append('<div id="%s" style="display:none;width:%dpx;height:%dpx"></div>' % (rid, kid[1], ROW))
        else:
            rows.append('<div id="%s" style="%swidth:%dpx"><div style="width:%dpx;height:%dpx"></div></div>' % (
                rid, "overflow:hidden;" if kid[0] == "clip" else "", kid[1], kid[2], ROW))
    return '<html><body><div id="o" style="%s">%s</div><p id="a">after</p></body></html>' % (st, "".join(rows))


def xflow(c):
    """what the layout gives for case c, from CSS Overflow 3 (the content
    width: the in-flow descendant boxes, not the insides of a box
    clipping along x nor hidden boxes; the scroll range: the content
    width less the scrollport, at least 0) and the engine's bars (Lot 74:
    BAR px inside the border edge, the horizontal one at the bottom, left
    of a painted vertical one; the thumb's length and offset in proportion
    to the scrollport and offset, rounded down; a click on a track scrolls
    to the offset in proportion to its position, rounded down)"""
    W, L, R, B, H = c["W"], c["L"], c["R"], c["B"], c["H"]
    content = 0
    rows = 0
    for kid in c["kids"]:
        if kid[0] == "none":
            continue
        rows += 1
        content = max(content, kid[1], kid[2] if kid[0] == "nest" else 0)
    ch = rows * ROW
    x, y = 8, 8
    w = W + L + R + 2 * B
    h = (H if H is not None else ch) + 2 * B
    view_h = H if H is not None else ch
    rg = max(0, content - W)
    track_h = h - 2 * B
    ybar = c["ovy"] in ("auto", "scroll") and track_h >= 4 and ch > view_h and x + w - B - BAR >= x
    tx, ty = x + B, y + h - B - BAR
    tw = w - 2 * B - (BAR if ybar else 0)
    xtrack = tw >= 4 and ty >= y
    out = {"x": x, "y": y, "w": w, "h": h, "rg": rg, "W": W, "tx": tx, "ty": ty, "tw": tw, "xtrack": xtrack,
           "ybar": ybar, "content": content, "first": None}
    for k, kid in enumerate(c["kids"]):
        if kid[0] != "none":
            out["first"] = (k, kid[1])
            break

    def bars(sx):
        b = []
        if content > W and xtrack:
            tm = max(4, W * tw // content)
            tm = min(tm, tw)
            b.append((tx, ty, tw, BAR, 0xDDDDDD))
            b.append((tx + (tw - tm) * sx // rg, ty, tm, BAR, 0x888888))
        if ybar:
            ty0 = y + B
            th = max(4, view_h * track_h // ch)
            th = min(th, track_h)
            b.append((x + w - B - BAR, ty0, BAR, track_h, 0xDDDDDD))
            b.append((x + w - B - BAR, ty0, BAR, th, 0x888888))
        return b

    sx = 0
    steps = []
    for px, py in c["clicks"]:
        on = xtrack and tx <= px < tx + tw and ty <= py < ty + BAR
        if not on:
            r = 0
        elif rg <= 0:
            r = 0
        else:
            want = min(rg * (px - tx) // tw, rg)
            if want == sx:
                r = 1
            else:
                r, sx = 2, want
        steps.append((px, py, r, sx, bars(sx)))
    out["bars0"] = bars(0)
    out["steps"] = steps
    return out


# ------------------------------------------------------------ relative colours
REL_FN = ["rgb", "hsl", "oklab", "oklch"]
REL_KW = [("r", "g", "b"), ("h", "s", "l"), ("l", "a", "b"), ("l", "c", "h")]
SPC = " \t\n\r"


def rel_head(v):
    """(function index, index after "from" and its space) of a relative
    colour, else None (CSS Color 5: the origin follows "from")"""
    lv = v.lower()
    for k, f in enumerate(REL_FN):
        if not lv.startswith(f):
            continue
        i = len(f) + (1 if k < 2 and lv[len(f):len(f) + 1] == "a" else 0)
        if lv[i:i + 1] != "(":
            continue
        i += 1
        while i < len(v) and v[i] in SPC:
            i += 1
        if lv[i:i + 4] == "from" and i + 5 < len(v) and v[i + 4] in SPC:
            return k, i + 5
        return None
    return None


def rel_split(v, i):
    """the origin and the arguments of a relative colour from v[i] (spaces
    and one "/" before the alpha at the top level, no comma there), or
    None"""
    toks, slash, n = [], -1, len(v)
    while True:
        while i < n - 1 and v[i] in SPC:
            i += 1
        if i >= n - 1:
            break
        if v[i] == "/":
            if slash >= 0:
                return None
            slash = len(toks)
            i += 1
            continue
        if len(toks) >= 5:
            return None
        a, d = i, 0
        while i < n - 1 and (d or (v[i] not in SPC and v[i] != "/")):
            if v[i] == "(":
                d += 1
            elif v[i] == ")":
                if not d:
                    return None
                d -= 1
            elif not d and v[i] == ",":
                return None
            i += 1
        if d:
            return None
        toks.append(v[a:i])
    if not ((len(toks) == 4 and slash < 0) or (len(toks) == 5 and slash == 4)):
        return None
    return toks


def ok_values(text):
    """the OKLab values of an absolute colour text when it is an oklab()
    or oklch() (as written, clamped), else None"""
    import lot101_colormodel as L101
    if text.lower().startswith(("oklab(", "oklch(")):
        r = L101.labish_lin(text)
        return None if r is None else r[1]
    return None


def rel_values(fn, text):
    """the keyword values (1/10000) of function fn for the absolute origin
    colour text: r g b its channels; h s l from them (exact, CSS Color 4
    sample code, the hue 0 for a grey); oklab l a b (the origin's own
    OKLab values for an oklab() or oklch() origin, else from its linear
    sRGB values) and oklch l c h (the hue 0 for a chroma at most
    0.000004); alpha; None when the origin is invalid. Raises Unsure"""
    import lot101_colormodel as L101
    import lot103_model as L103
    col = L103.full(text)
    if col is None:
        return None
    ch, al = col[0], col[1]
    if fn == 0:
        return [ch[0], ch[1], ch[2], al]
    if fn == 1:
        h, s, l = L103.hsl_comps(ch, False)
        return [h or 0, s, l, al]
    x = ok_values(text)
    if x is None:
        lin = col[3] if col[3] is not None else L103.decode_ch(ch)
        if max(abs(v) for v in lin) > L103.BIG_MIX:
            raise L101.Unsure("big")
        x = L103.lin_to("oklab", lin)
    if fn == 2:
        return [L101.rh(x[0] * S7), L101.rh(x[1] * S7), L101.rh(x[2] * S7), al]
    import math
    c = math.hypot(x[1], x[2])
    thr = 0.000004
    if abs(c - thr) <= thr * L103.TOL_REL + 1e-12:
        raise L101.Unsure("powerless")
    h = 0
    if c > thr:
        h = L101.rh(math.degrees(math.atan2(x[2], x[1])) * S7) % 3600000
    return [L101.rh(x[0] * S7), L101.rh(c * S7), h, al]


S7 = 10000


def rel_arg(fn, t, kv):
    """one argument as the absolute colour gets it, or None"""
    import lot95_propmodel as P
    kws = REL_KW[fn]
    lt = t.lower()
    if lt in kws:
        return P.fmt(kv[kws.index(lt)], "")
    if lt == "alpha":
        return P.fmt(kv[3], "")
    if "(" not in t:
        return t
    out, i = "", 0
    while i < len(t):
        c = t[i]
        if (c.isalpha() or c in "-_") and c.isascii() and (i == 0 or not (t[i - 1].isalnum() or t[i - 1] in "-_.")):
            j = i
            while j < len(t) and (t[j].isalnum() or t[j] in "-_") and t[j].isascii():
                j += 1
            w = t[i:j].lower()
            k = kws.index(w) if w in kws else 3 if w == "alpha" else -1
            if k >= 0:
                f = P.fmt(kv[k], "")
                out += "(%s)" % f if kv[k] < 0 else f
            else:
                out += t[i:j]
            i = j
            continue
        out += c
        i += 1
    q = P.calc(out.lower(), None)
    if q is None or not q.ok:
        return None
    return P.fmt_q(q)


def rel_text(v, depth=0):
    """the absolute colour a relative one is (CSS Color 5: the keywords
    and math functions resolved against the origin, the alpha the
    origin's when left out), None when it is invalid, NotImplemented when
    v is no relative colour. Raises Unsure"""
    h = rel_head(v)
    if h is None:
        return NotImplemented
    if depth > 3 or not v.endswith(")"):
        return None
    fn, i = h
    toks = rel_split(v, i)
    if toks is None:
        return None
    org = toks[0]
    if rel_head(org) is not None:
        org = rel_text(org, depth + 1)
        if org is None:
            return None
    kv = rel_values(fn, org)
    if kv is None:
        return None
    args = []
    for t in toks[1:]:
        a = rel_arg(fn, t, kv)
        if a is None:
            return None
        args.append(a)
    if len(args) == 3:
        import lot95_propmodel as P
        args.append(P.fmt(kv[3], ""))
    return "%s(%s %s %s / %s)" % ((REL_FN[fn],) + tuple(args))


def rel_rgba(v):
    """(channels in 1/10000 of 0..255, alpha in 1/10000) of a relative
    colour, None when invalid; raises Unsure"""
    import lot103_model as L103
    t = rel_text(v)
    if t is None or t is NotImplemented:
        return None
    r = L103.full(t)
    return None if r is None else (r[0], r[1])


# Lot 109: relative lab(), lch(), hwb() and color(), and the unclamped
# channels of an origin outside the sRGB gamut (lot109_model); the Lot 108
# functions above stay its rgb(), hsl(), oklab() and oklch() values
def rel_head(v):
    import lot109_model as M9
    return M9.rel_head(v)


def rel_rgba(v):
    import lot109_model as M9
    return M9.rel_rgba(v)
