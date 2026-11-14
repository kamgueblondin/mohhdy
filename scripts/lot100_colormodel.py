#!/usr/bin/env python3
"""Lot 100 model of the reader's colours, written from docs/lot100_plan.md
and the specs it cites (CSS Color 4 named colours, hwb() and the alpha of
rgb() and hsl(); CSS Color 5 color-mix() in srgb; CSS Transitions 2
@starting-style; CSS Animations 1 and Web Animations 1 for the colour
animations), not from the C sources (userspace/wf_cssprop.c,
userspace/wf_sitecss.c, userspace/wf_cssbg.c). It builds on
scripts/lot97_animmodel.py and scripts/lot98_animmodel.py; the Lot 97
colour functions (rgba, colour_match) are converted to consult this
model.

Numbers are integers in 1/10000; rhalf rounds half away from zero.
Channels are in 1/10000 of 0..255, alpha in 1/10000 of 0..1.

* names: the 148 named colours of CSS Color 4 (NAMED_TEXT below), any
  case, plus transparent (0 0 0 / 0); currentcolor matches a registered
  <color> but has no value here.
* hwb(H W B [/ A]): spaces only (a comma makes it invalid); H a number of
  degrees or an angle (deg, grad, rad, turn), taken modulo 360; W and B
  percentages or numbers on the same scale (50 is 50%), each clamped to
  0..100%; A as in rgb(). When W + B >= 100% the colour is the grey
  W / (W + B); else each channel is the channel of hsl(H 100% 50%) scaled
  by 1 - W - B, plus W. Each channel is rounded once.
* color-mix(in srgb, C1 [P1], C2 [P2]): exactly three top-level comma
  parts; the first is "in", spaces, "srgb". Each colour part is a colour
  and at most one percentage (0%..100%, before or after it; the token
  whose last byte is '%' is the percentage when there are two). Missing
  percentages are 50% each, or 100% minus the other. S = P1 + P2 must be
  over 0. With A = a1 P1 + a2 P2: each channel (c1 a1 P1 + c2 a2 P2) / A
  (0 when A is 0) and the alpha A / max(S, 100%), each rounded once. A
  colour part may itself be a color-mix(), to four levels in all; a
  colour part of 128 bytes or more, or a number token of 32 bytes or
  more, is invalid.
* reader colour (wf_cssprop_color): the value after leading spaces, to
  the ')' that closes its first '(' (else to a space, ';' or '!'); then
  only spaces and the end, a ';' or a '!'. Under 128 bytes, any case.
  The colour is any value above (names, hex, rgb(), rgba(), hsl(),
  hsla(), hwb(), color-mix()); its channels rounded to 0..255 and its
  alpha to 0..255 (alpha * 255 rounded).
* reader paint: color and background-color are written #rrggbb when the
  alpha is 255, else #rrggbbaa. background-color: transparent (the whole
  value) is no background, as before. The background shorthand keeps a
  colour word under 32 bytes with no '!' or ';' that is a reader colour;
  transparent there is no background.
* colour animations: keyframes that set color or background-color
  animate them as a registered <color> would (the Lot 98 and 99 rules:
  compositions, generated 0% and 100% keyframes). The value below is the
  element's own colour as written (#rrggbb or #rrggbbaa); for color
  without one the final colour of the nearest ancestor that has one,
  else #000000; for background-color without one, transparent. An
  !important own colour is not animated. Without an animation, a
  transition named color, background-color or all runs from the colour
  of an @starting-style rule (written the same way) to the value below.
  The result is painted when it is a reader colour.
"""
import os
import sys
from fractions import Fraction as F

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
import lot95_propmodel as P  # noqa: E402
import lot97_animmodel as A  # noqa: E402
import lot98_animmodel as M98  # noqa: E402

S = 10000
VAL = 128
TOK = 32
SP = " \t\n\r"
rhalf = A.rhalf

NAMED_TEXT = """aliceblue f0f8ff antiquewhite faebd7 aqua 00ffff aquamarine 7fffd4 azure f0ffff beige f5f5dc bisque ffe4c4
black 000000 blanchedalmond ffebcd blue 0000ff blueviolet 8a2be2 brown a52a2a burlywood deb887 cadetblue 5f9ea0
chartreuse 7fff00 chocolate d2691e coral ff7f50 cornflowerblue 6495ed cornsilk fff8dc crimson dc143c cyan 00ffff
darkblue 00008b darkcyan 008b8b darkgoldenrod b8860b darkgray a9a9a9 darkgreen 006400 darkgrey a9a9a9
darkkhaki bdb76b darkmagenta 8b008b darkolivegreen 556b2f darkorange ff8c00 darkorchid 9932cc darkred 8b0000
darksalmon e9967a darkseagreen 8fbc8f darkslateblue 483d8b darkslategray 2f4f4f darkslategrey 2f4f4f
darkturquoise 00ced1 darkviolet 9400d3 deeppink ff1493 deepskyblue 00bfff dimgray 696969 dimgrey 696969
dodgerblue 1e90ff firebrick b22222 floralwhite fffaf0 forestgreen 228b22 fuchsia ff00ff gainsboro dcdcdc
ghostwhite f8f8ff gold ffd700 goldenrod daa520 gray 808080 green 008000 greenyellow adff2f grey 808080
honeydew f0fff0 hotpink ff69b4 indianred cd5c5c indigo 4b0082 ivory fffff0 khaki f0e68c lavender e6e6fa
lavenderblush fff0f5 lawngreen 7cfc00 lemonchiffon fffacd lightblue add8e6 lightcoral f08080 lightcyan e0ffff
lightgoldenrodyellow fafad2 lightgray d3d3d3 lightgreen 90ee90 lightgrey d3d3d3 lightpink ffb6c1
lightsalmon ffa07a lightseagreen 20b2aa lightskyblue 87cefa lightslategray 778899 lightslategrey 778899
lightsteelblue b0c4de lightyellow ffffe0 lime 00ff00 limegreen 32cd32 linen faf0e6 magenta ff00ff maroon 800000
mediumaquamarine 66cdaa mediumblue 0000cd mediumorchid ba55d3 mediumpurple 9370db mediumseagreen 3cb371
mediumslateblue 7b68ee mediumspringgreen 00fa9a mediumturquoise 48d1cc mediumvioletred c71585
midnightblue 191970 mintcream f5fffa mistyrose ffe4e1 moccasin ffe4b5 navajowhite ffdead navy 000080
oldlace fdf5e6 olive 808000 olivedrab 6b8e23 orange ffa500 orangered ff4500 orchid da70d6 palegoldenrod eee8aa
palegreen 98fb98 paleturquoise afeeee palevioletred db7093 papayawhip ffefd5 peachpuff ffdab9 peru cd853f
pink ffc0cb plum dda0dd powderblue b0e0e6 purple 800080 rebeccapurple 663399 red ff0000 rosybrown bc8f8f
royalblue 4169e1 saddlebrown 8b4513 salmon fa8072 sandybrown f4a460 seagreen 2e8b57 seashell fff5ee
sienna a0522d silver c0c0c0 skyblue 87ceeb slateblue 6a5acd slategray 708090 slategrey 708090 snow fffafa
springgreen 00ff7f steelblue 4682b4 tan d2b48c teal 008080 thistle d8bfd8 tomato ff6347 turquoise 40e0d0
violet ee82ee wheat f5deb3 white ffffff whitesmoke f5f5f5 yellow ffff00 yellowgreen 9acd32"""
_W = NAMED_TEXT.split()
NAMED = {_W[i]: tuple(int(_W[i + 1][k:k + 2], 16) for k in (0, 2, 4)) for i in range(0, len(_W), 2)}
assert len(NAMED) == 148


def is_name(v):
    return v.lower() in NAMED


# ------------------------------------------------------------- tokens
def num_pct(t):
    """(value in 1/10000, is percentage) of a number or percentage token
    under 32 bytes, or None"""
    t = t.strip(SP)
    if not t or len(t) >= TOK:
        return None
    return M98.num_pct(t)


def hue(t):
    """the hue in degrees (a Fraction, 0..360) of a number or angle token,
    or None"""
    if not t or len(t) >= TOK:
        return None
    q = P.leaf(t, None)
    if q is None or not q.ok or q.k not in ("N", "A"):
        return None
    if q.k == "N" and P.NUM.match(t).end() != len(t):
        return None
    return F(q.v, S) % 360


def alpha_of(t):
    r = num_pct(t)
    if r is None:
        return None
    return min(max(rhalf(F(r[0], 100)) if r[1] else r[0], 0), S)


# ------------------------------------------------------------- hwb
def hwb_rgb(h, w, b):
    """h in degrees, w and b in 0..1 (Fractions): channels in 1/10000"""
    if w + b >= 1:
        return [rhalf(w / (w + b) * 255 * S)] * 3
    out = []
    for n in (0, 8, 4):
        k = (n + h / 30) % 12
        f = F(1, 2) - F(1, 2) * max(-1, min(k - 3, 9 - k, 1))
        out.append(rhalf((f * (1 - w - b) + w) * 255 * S))
    return out


def hwb(v):
    if not (v.startswith("hwb(") and v.endswith(")") and len(v) > 5):
        return None
    got = M98.colour_args(v[4:-1])
    if got is None or got[1]:
        return None
    toks = got[0]
    h = hue(toks[0])
    if h is None:
        return None
    wb = []
    for t in toks[1:3]:
        r = num_pct(t)
        if r is None:
            return None
        wb.append(F(min(max(r[0], 0), 100 * S), 100 * S))
    a = S
    if len(toks) == 4:
        a = alpha_of(toks[3])
        if a is None:
            return None
    return hwb_rgb(h, wb[0], wb[1]), a


# ------------------------------------------------------------- color-mix
def split_top(s, comma):
    """the top-level parts of s split at commas (comma) or space runs,
    trimmed; None on an unbalanced parenthesis"""
    parts, depth, cur = [], 0, ""
    for c in s:
        if c == "(":
            depth += 1
        elif c == ")":
            depth -= 1
            if depth < 0:
                return None
        elif depth == 0 and (c == "," if comma else c in SP):
            parts.append(cur)
            cur = ""
            continue
        cur += c
    if depth:
        return None
    parts.append(cur)
    parts = [p.strip(SP) for p in parts]
    if not comma:
        parts = [p for p in parts if p] or [""]
    return parts


def mix_item(t, depth):
    """(colour, percentage in 1/10000 % or None) of a color-mix() part"""
    toks = split_top(t, False)
    if toks is None or len(toks) > 2:
        return None
    p = None
    c = toks[0]
    if len(toks) == 2:
        q = 0 if toks[0].endswith("%") else 1
        r = num_pct(toks[q])
        if r is None or not r[1] or not 0 <= r[0] <= 100 * S:
            return None
        p, c = r[0], toks[1 - q]
    if len(c) >= VAL:
        return None
    col = rgba(c, depth + 1)
    if col is None:
        return None
    return col, p


def mix(v, depth):
    if depth > 3 or not (v.startswith("color-mix(") and v.endswith(")") and len(v) > 11):
        return None
    parts = split_top(v[10:-1], True)
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
    return ch, rhalf(F(an, max(tot, 100 * S)))


# ------------------------------------------------------------- any colour
def rgba(v, depth=0):
    """channels in 1/10000 of 0..255 and alpha in 1/10000, or None (Lot
    97's rgba with every name, hwb() and color-mix())"""
    v = v.lower()
    if v == "transparent":
        return [0, 0, 0], 0
    if v in NAMED:
        return [c * S for c in NAMED[v]], S
    if v.startswith("#"):
        return A.rgba97(v)
    if not v.endswith(")"):
        return None
    if v.startswith("rgb"):
        return A.rgba97(v)
    if v.startswith("hsl"):
        return M98.hsl(v)
    if v.startswith("hwb("):
        return hwb(v)
    if v.startswith("color-mix("):
        return mix(v, depth)
    return None


def colour_match(v):
    """a registered <color> value (Lot 97's colour_match with every name,
    hwb() and color-mix())"""
    if v.lower() in NAMED or v.lower() in ("transparent", "currentcolor"):
        return True
    lv = v.lower()
    if (len(v) > 4 and lv.startswith("hwb(")) or (len(v) > 10 and lv.startswith("color-mix(")):
        return rgba(v) is not None
    return A.colour_match97(v)


# ------------------------------------------------------------- reader colour
def reader(v):
    """(0xrrggbb, alpha 0..255) of the reader colour at the start of v, or
    None"""
    i = 0
    while i < len(v) and v[i] in SP:
        i += 1
    depth, b = 0, i
    while b < len(v):
        c = v[b]
        if c == "(":
            depth += 1
        elif c == ")":
            depth -= 1
            if depth <= 0:
                b += 1
                break
        elif depth == 0 and (c in SP or c in ";!"):
            break
        b += 1
    if depth > 0 or b == i or b - i >= VAL:
        return None
    k = b
    while k < len(v) and v[k] in SP:
        k += 1
    if k < len(v) and v[k] not in ";!":
        return None
    r = rgba(v[i:b].lower())
    if r is None:
        return None
    ch = [rhalf(F(c, S)) for c in r[0]]
    return (ch[0] << 16) | (ch[1] << 8) | ch[2], rhalf(F(r[1] * 255, S))


def paint_text(v):
    """the text the reader writes for colour v, or None"""
    r = reader(v)
    if r is None:
        return None
    return "#%06x" % r[0] + ("%02x" % r[1] if r[1] != 255 else "")


def bg_word(v):
    """the background-color the background shorthand writes for its
    colour word v: a text, "" for no background, None when the shorthand
    is dropped"""
    if len(v) >= 32 or "!" in v or ";" in v:
        return None
    if v.lower() == "transparent":
        return ""
    return paint_text(v)


# ------------------------------------------------------------- animations
ALTS = [("<color>", "", None)]


def col_value(anims, kfs, base, ms, important=False):
    """(value text, animated, composed) of color or background-color after
    the animations (kfs: name -> frames of that property)"""
    if important:
        return base, False, False
    return M98.stack(ALTS, anims, kfs, base, ms)


def col_trans(tl, name, start, end, ms):
    """(value text or None) of the transition of property name from the
    starting colour text to end"""
    tr = None
    for x in tl:
        if x["prop"] in ("all", name):
            tr = x
    if tr is None or start == end:
        return None
    if A.interp(ALTS, start, end, 5000)[1] == 1 and not tr["discrete"]:
        return None
    e = ms - tr["delay"]
    if e >= tr["dur"]:
        return None
    if e < 0:
        return start
    return A.interp(ALTS, start, end, A.ease_at(tr["ease"], e * S // tr["dur"]))[0]
