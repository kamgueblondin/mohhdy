#!/usr/bin/env python3
"""Lot 69 / Lot 71: generate the desktop bitmap fonts.

Native square-cell fonts used at 1.5x, 2x, 3x and 4x (cells 12, 16, 24
and 32 px) are rasterised from IBM Plex Mono Medium (SIL Open Font License
1.1, Copyright IBM Corp.). Each glyph is drawn at 4x resolution, stretched
to the square cell (the desktop keeps the 8x8 cell metrics, so a cell is as
wide as it is high), area-averaged and thresholded.

Lot 71: both fonts carry the whole display character set of
include/os_charset.h (OS_GLYPH_COUNT = 223 glyphs): ASCII 32..126, the
CP1252 extras 0x80..0x9F (undefined slots get '?') and Latin-1
0xA0..0xFF, so French accents are drawn instead of '?'. The 8x8 font keeps
its hand-made ASCII strokes; its accented letters are composed from the
base letter and an accent (Unicode NFD), the other new glyphs are
rasterised from the TTF at 8 px.

Lot 72: glyph pages. Four more pages of up to 123 characters each are
reached with a page selector byte (the five bytes CP1252 leaves undefined,
include/os_charset.h): page 1 Latin Extended-A, page 2 Greek (ISO-8859-7
layout in 0xA0..0xFF), page 3 Cyrillic (ISO-8859-5 layout, plus Ukrainian
and Kazakh letters in 0x80..0x9F), page 4 symbols (arrows, mathematics,
box drawing, blocks, shapes, Romanian comma letters, currencies). The page
tables go to include/os_charset_pages.h; the fonts grow by one glyph per new
code point (a code point already in the font reuses its glyph). Glyphs the
primary font lacks (Greek, some symbols) come from Noto Sans Mono Medium
(SIL OFL 1.1). In the 8x8 font, Greek and Cyrillic capitals and small
letters that look like Latin ones reuse the hand-made ASCII strokes, and
letters with a mark are composed from their base glyph.

  python3 scripts/gen_bitmap_font.py FONT.ttf kernel/gfx_font_hd.h kernel/gfx_font8.h \
      [--fallback NotoSansMono.ttf] [--pages include/os_charset_pages.h]
  python3 scripts/gen_bitmap_font.py --check kernel/gfx_font_hd.h kernel/gfx_font8.h include/os_charset_pages.h
"""
import re
import sys
import unicodedata

CELLS = [12, 16, 24, 32]
OVER = 4
CP1252_HIGH = [0x20AC, 0, 0x201A, 0x0192, 0x201E, 0x2026, 0x2020, 0x2021, 0x02C6, 0x2030, 0x0160, 0x2039,
               0x0152, 0, 0x017D, 0, 0, 0x2018, 0x2019, 0x201C, 0x201D, 0x2022, 0x2013, 0x2014, 0x02DC,
               0x2122, 0x0161, 0x203A, 0x0153, 0, 0x017E, 0x0178]
BASE_GLYPHS = 223
SELECTORS = [0x81, 0x8D, 0x8F, 0x90, 0x9D]      # page 0..4 selector bytes
PAGE_NAMES = ["CP1252", "Latin Extended-A", "Greek", "Cyrillic", "Symbols"]
CYR_EXTRA = "\u0490\u0491\u0492\u0493\u0496\u0497\u049a\u049b\u04a2\u04a3\u04ae\u04af\u04b0\u04b1\u04ba\u04bb\u04d8\u04d9\u04e8\u04e9"
SYMBOLS = ("\u2190\u2191\u2192\u2193\u2194\u2195\u21d0\u21d2\u21d4"           # arrows
           "\u2200\u2202\u2203\u2205\u2207\u2208\u2209\u220f\u2211\u2212\u221a\u221e"
           "\u2227\u2228\u2229\u222a\u222b\u2248\u2260\u2261\u2264\u2265\u2282\u2283"  # mathematics
           "\u2500\u2502\u250c\u2510\u2514\u2518\u251c\u2524\u252c\u2534\u253c"
           "\u2550\u2551\u2554\u2557\u255a\u255d\u2560\u2563\u2566\u2569\u256c"        # box drawing
           "\u2588\u2580\u2584\u2591\u2592\u2593\u25a0\u25a1\u25aa\u25ab"              # blocks
           "\u25b2\u25ba\u25bc\u25c4\u25cf\u25cb\u25c6\u25c7\u2713"                      # shapes, check mark
           "\u0218\u0219\u021a\u021b"                                                   # Romanian
           "\u20bd\u20b4\u20b9\u20ba\u20a9\u20aa\u20ab")                               # currencies


def cp1252_cps():
    """Code points page 0 draws (ASCII, CP1252 extras, Latin-1)."""
    return set(range(32, 127)) | set(c for c in CP1252_HIGH if c) | set(range(0xA0, 0x100))


def page_tables():
    """pages[p][b - 0x80] = code point of byte b in page p (1..4), 0 = none."""
    sel = set(SELECTORS)
    free = [b for b in range(0x80, 0x100) if b not in sel]
    base = cp1252_cps()
    pages = {}
    t = [0] * 128
    for b, cp in zip(free, [c for c in range(0x100, 0x180) if c not in base]):
        t[b - 0x80] = cp
    pages[1] = t
    for p, codec, extra in ((2, "iso8859-7", ""), (3, "iso8859-5", CYR_EXTRA)):
        t = [0] * 128
        for b in range(0xA0, 0x100):
            try:
                cp = ord(bytes([b]).decode(codec))
            except UnicodeDecodeError:
                cp = 0
            if cp == 0xAD:
                cp = 0                                 # soft hyphen: not drawn
            t[b - 0x80] = cp
        for b, ch in zip([b for b in free if b < 0xA0], extra):
            t[b - 0x80] = ord(ch)
        pages[p] = t
    t = [0] * 128
    for b, ch in zip(free, SYMBOLS):
        t[b - 0x80] = ord(ch)
    pages[4] = t
    assert len(SYMBOLS) <= len(free) and len(CYR_EXTRA) <= 27
    return pages


def slot_codepoints():
    """Unicode code point of every glyph slot: the 223 of Lot 71 in
    os_glyph_index order, then each new code point of pages 1..4 once."""
    cps = list(range(32, 127))
    cps += [c if c else ord("?") for c in CP1252_HIGH]
    cps += list(range(0xA0, 0x100))
    assert len(cps) == BASE_GLYPHS
    seen = cp1252_cps()
    pages = page_tables()
    for p in (1, 2, 3, 4):
        for cp in pages[p]:
            if cp and cp not in seen:
                seen.add(cp)
                cps.append(cp)
    return cps


GLYPHS = len(slot_codepoints())


# Lot 72: box drawing and block elements are drawn by rule (a rasterised
# line does not reach the cell edges, so frames would not join).
# arms: up, down, left, right; 1 light, 2 double
BOX_ARMS = {
    0x2500: (0, 0, 1, 1), 0x2502: (1, 1, 0, 0), 0x250C: (0, 1, 0, 1), 0x2510: (0, 1, 1, 0), 0x2514: (1, 0, 0, 1),
    0x2518: (1, 0, 1, 0), 0x251C: (1, 1, 0, 1), 0x2524: (1, 1, 1, 0), 0x252C: (0, 1, 1, 1), 0x2534: (1, 0, 1, 1),
    0x253C: (1, 1, 1, 1), 0x2550: (0, 0, 2, 2), 0x2551: (2, 2, 0, 0), 0x2554: (0, 2, 0, 2), 0x2557: (0, 2, 2, 0),
    0x255A: (2, 0, 0, 2), 0x255D: (2, 0, 2, 0), 0x2560: (2, 2, 0, 2), 0x2563: (2, 2, 2, 0), 0x2566: (0, 2, 2, 2),
    0x2569: (2, 0, 2, 2), 0x256C: (2, 2, 2, 2),
}


def rule_glyph(cp, n):
    """Rows of a box drawing / block glyph in an n x n cell, or None."""
    px = [[0] * n for _ in range(n)]

    def hline(y, x0, x1):
        for y2 in range(y, y + t):
            for x in range(max(0, x0), min(n, x1 + 1)):
                px[y2][x] = 1

    def vline(x, y0, y1):
        for x2 in range(x, x + t):
            for y in range(max(0, y0), min(n, y1 + 1)):
                px[y][x2] = 1

    t = max(1, n // 8)
    c = (n - t) // 2
    g = max(1, n // 8) + (t - 1)
    a, b = c - g, c + g
    if cp in BOX_ARMS:
        up, down, left, right = BOX_ARMS[cp]
        if right == 1: hline(c, c, n - 1)
        if left == 1: hline(c, 0, c + t - 1)
        if down == 1: vline(c, c, n - 1)
        if up == 1: vline(c, 0, c + t - 1)
        if right == 2:
            hline(a, b if up else a, n - 1)
            hline(b, b if down else a, n - 1)
        if left == 2:
            hline(a, 0, (a if up else b) + t - 1)
            hline(b, 0, (a if down else b) + t - 1)
        if down == 2:
            vline(a, b if left else a, n - 1)
            vline(b, b if right else a, n - 1)
        if up == 2:
            vline(a, 0, (a if left else b) + t - 1)
            vline(b, 0, (a if right else b) + t - 1)
    elif cp == 0x2588:
        px = [[1] * n for _ in range(n)]
    elif cp in (0x2580, 0x2584):
        for y in range(n):
            if (y < n // 2) == (cp == 0x2580):
                px[y] = [1] * n
    elif cp in (0x2591, 0x2592, 0x2593):
        for y in range(n):
            for x in range(n):
                if cp == 0x2591:
                    px[y][x] = int(x % 2 == 0 and y % 2 == 0)
                elif cp == 0x2592:
                    px[y][x] = int((x + y) % 2 == 0)
                else:
                    px[y][x] = int(not (x % 2 == 1 and y % 2 == 1))
    elif cp in (0x25A0, 0x25A1, 0x25AA, 0x25AB):
        m = n // 8 if cp in (0x25A0, 0x25A1) else n // 4
        m = max(1, m)
        for y in range(m, n - m):
            for x in range(m, n - m):
                edge = y in (m, n - m - 1) or x in (m, n - m - 1)
                px[y][x] = int(cp in (0x25A0, 0x25AA) or edge)
    else:
        return None
    return [sum(1 << x for x in range(n) if px[y][x]) for y in range(n)]


def gen_pages(out):
    """include/os_charset_pages.h: page tables, glyph indexes, reverse map."""
    cps = slot_codepoints()
    gi = {}
    for i, cp in enumerate(cps):
        gi.setdefault(cp, i)
    gi[0x20] = 0
    pages = page_tables()
    qmark = ord("?") - 32
    lines = [
        "/* os_charset_pages.h - Lot 72: glyph pages 1..4 of the display",
        " * character set (include/os_charset.h). Generated by",
        " * scripts/gen_bitmap_font.py, do not edit. A page selector byte",
        " * (os_page_sel[]) makes the bytes 0x80..0xFF that follow it mean the",
        " * code points of that page; ASCII is the same in every page. */",
        "#ifndef OS_CHARSET_PAGES_H",
        "#define OS_CHARSET_PAGES_H",
        "#include <stdint.h>",
        "",
        "#define OS_GLYPH_TOTAL %d     /* glyphs in the kernel fonts */" % len(cps),
        "#define OS_PAGE_COUNT 5",
        "static const uint8_t os_page_sel[OS_PAGE_COUNT] = { %s };" % ", ".join("0x%02X" % b for b in SELECTORS),
        "",
        "/* code point of byte 0x80 + i in page p + 1 (0 = undefined) */",
        "static const uint16_t os_page_cp[4][128] = {",
    ]
    for p in (1, 2, 3, 4):
        lines.append("    { /* page %d: %s */" % (p, PAGE_NAMES[p]))
        t = pages[p]
        for r in range(0, 128, 16):
            lines.append("        " + ", ".join("0x%04X" % v for v in t[r:r + 16]) + ",")
        lines.append("    },")
    lines += ["};", "", "/* glyph slot of byte 0x80 + i in page p + 1 ('?' when undefined) */",
              "static const uint16_t os_page_glyph[4][128] = {"]
    for p in (1, 2, 3, 4):
        t = pages[p]
        lines.append("    { /* page %d */" % p)
        for r in range(0, 128, 16):
            lines.append("        " + ", ".join("%d" % (gi[v] if v else qmark) for v in t[r:r + 16]) + ",")
        lines.append("    },")
    rev = []
    base = cp1252_cps()
    for p in (1, 2, 3, 4):
        for i, cp in enumerate(pages[p]):
            if cp and cp not in base and not any(r[0] == cp for r in rev):
                rev.append((cp, p, 0x80 + i))
    rev.sort()
    lines += ["};", "",
              "/* code points outside CP1252, sorted: cp << 16 | page << 8 | byte */",
              "#define OS_PAGE_REV_COUNT %d" % len(rev),
              "static const uint32_t os_page_rev[OS_PAGE_REV_COUNT] = {"]
    for r in range(0, len(rev), 8):
        lines.append("    " + ", ".join("0x%08Xu" % ((c << 16) | (p << 8) | b) for c, p, b in rev[r:r + 8]) + ",")
    lines += ["};", "#endif", ""]
    with open(out, "w") as f:
        f.write("\n".join(lines))


def tag(cp):
    if 32 <= cp < 127:
        c = chr(cp)
        return c if c not in "\\*/" else "0x%02x" % cp
    return "U+%04X" % cp


# ------------------------------------------------------------------ HD fonts

def glyph(font, code, cell):
    from PIL import Image, ImageDraw
    big = cell * OVER
    img = Image.new("L", (big * 2, big * 2), 0)
    d = ImageDraw.Draw(img)
    d.text((big // 2, int(big * 1.62)), chr(code), fill=255, font=font, anchor="ls")
    top, bottom = int(big * 0.62), int(big * 1.62) + int(big * 0.22)
    adv = font.getlength("M")
    x0 = big // 2 - int(adv * 0.04)
    x1 = big // 2 + int(adv * 1.04)
    crop = img.crop((x0, top, x1, bottom)).resize((cell, cell), Image.BOX)
    rows = []
    for y in range(cell):
        bits = 0
        for x in range(cell):
            if crop.getpixel((x, y)) >= 100:
                bits |= 1 << x
        rows.append(bits)
    return rows


def load_font(path, size):
    """A TrueType font; a variable font is set to its Medium (500) weight."""
    from PIL import ImageFont
    font = ImageFont.truetype(path, size)
    try:
        axes = font.get_variation_axes()
    except (OSError, AttributeError):
        axes = None
    if axes:
        vals = []
        for a in axes:
            name = a.get("name", b"")
            name = name.decode() if isinstance(name, bytes) else str(name)
            vals.append(500 if name.lower().startswith("weight") else a.get("default", 100))
        font.set_variation_by_axes(vals)
    return font


def has_glyph(path, cp, _cache={}):
    if path not in _cache:
        from fontTools.ttLib import TTFont
        _cache[path] = set(TTFont(path).getBestCmap())
    return cp in _cache[path]


def pick(fonts, cp):
    """(font, path) of the first font that has cp (the primary one for the
    Lot 71 glyphs, so they are generated exactly as before)."""
    for path, font in fonts:
        if cp < 0x100 or has_glyph(path, cp):
            return font
    raise SystemExit("no font has U+%04X" % cp)


FALLBACK = []


def read_hd_rows(out):
    """Rows already generated per table (Lot 72: the 223 Lot 71 glyphs are
    kept as they are; another FreeType version moves a pixel here and
    there, and the fonts must not change under existing tests)."""
    keep = {}
    try:
        txt = open(out).read()
    except OSError:
        return keep
    for m in re.finditer(r"gfx_font_hd(\d+)\[\d+\]\[\d+\] = \{(.*?)\n\};", txt, re.S):
        keep[int(m.group(1))] = re.findall(r"    \{[^}]*\}, /\* .* \*/", m.group(2))
    return keep


def gen_hd(path, out):
    keep = read_hd_rows(out)
    lines = [
        "/* gfx_font_hd.h - Lot 69/71: native square-cell bitmap fonts for the",
        " * desktop at 1.5x/2x/3x/4x (12, 16, 24, 32 px), the %d glyphs of" % GLYPHS,
        " * include/os_charset.h (ASCII, CP1252 0x80..0x9F, Latin-1) and of",
        " * the Lot 72 glyph pages (include/os_charset_pages.h).",
        " * Generated by scripts/gen_bitmap_font.py from IBM Plex Mono Medium,",
        " * Copyright IBM Corp., and for the code points it lacks Noto Sans",
        " * Mono Medium, Copyright Google LLC, both SIL Open Font License 1.1",
        " * (rasterised derivative). Bit x of row y is pixel (x, y), bit 0 =",
        " * left. */",
        "#ifndef GFX_FONT_HD_H",
        "#define GFX_FONT_HD_H",
        "#include <stdint.h>",
    ]
    for cell in CELLS:
        fonts = [(p, load_font(p, cell * OVER)) for p in [path] + FALLBACK]
        ctype = "uint16_t" if cell <= 16 else "uint32_t"
        lines.append("static const %s gfx_font_hd%d[%d][%d] = {" % (ctype, cell, GLYPHS, cell))
        old = keep.get(cell, [])
        for i, cp in enumerate(slot_codepoints()):
            if i < BASE_GLYPHS and i < len(old):
                lines.append(old[i])
                continue
            g = rule_glyph(cp, cell) or glyph(pick(fonts, cp), cp, cell)
            lines.append("    {" + ",".join("0x%x" % v for v in g) + "}, /* %s */" % tag(cp))
        lines.append("};")
    lines += ["#endif", ""]
    with open(out, "w") as f:
        f.write("\n".join(lines))


# ------------------------------------------------------------------ 8x8 font

ACCENT_LOWER = {   # two rows above a lowercase body (rows 0, 1)
    "\u0301": (0x30, 0x18), "\u0300": (0x0c, 0x18), "\u0302": (0x18, 0x66), "\u0308": (0x00, 0x66),
    "\u0303": (0x4c, 0x32), "\u030a": (0x18, 0x18), "\u030c": (0x66, 0x18),
}
ACCENT_UPPER = {   # one row above a squashed capital (row 0)
    "\u0301": 0x30, "\u0300": 0x0c, "\u0302": 0x3c, "\u0308": 0x66, "\u0303": 0x5a, "\u030a": 0x18, "\u030c": 0x3c,
}
# Lot 72: marks of Latin Extended-A, Greek and Cyrillic letters (macron,
# breve, dot above, double acute; the ogonek goes under the body)
ACCENT_LOWER_72 = {"\u0304": (0x00, 0x3c), "\u0306": (0x24, 0x18), "\u0307": (0x00, 0x18), "\u030b": (0x6c, 0x36)}
ACCENT_UPPER_72 = {"\u0304": 0x3c, "\u0306": 0x24, "\u0307": 0x18, "\u030b": 0x6c}
# Lot 72: Greek and Cyrillic letters drawn like a Latin letter of the 8x8
# font (same strokes, so the hand-made ASCII glyph is reused)
HOMOGLYPH8 = {
    0x0391: "A", 0x0392: "B", 0x0395: "E", 0x0396: "Z", 0x0397: "H", 0x0399: "I", 0x039A: "K", 0x039C: "M",
    0x039D: "N", 0x039F: "O", 0x03A1: "P", 0x03A4: "T", 0x03A5: "Y", 0x03A7: "X", 0x03BF: "o",
    0x0405: "S", 0x0406: "I", 0x0408: "J", 0x0410: "A", 0x0412: "B", 0x0415: "E", 0x041A: "K", 0x041C: "M",
    0x041D: "H", 0x041E: "O", 0x0420: "P", 0x0421: "C", 0x0422: "T", 0x0425: "X", 0x0430: "a", 0x0435: "e",
    0x043E: "o", 0x0440: "p", 0x0441: "c", 0x0443: "y", 0x0445: "x", 0x0455: "s", 0x0456: "i", 0x0458: "j",
    0x04AE: "Y", 0x04BB: "h",
}


def read_font8_ascii(path):
    rows = []
    with open(path) as f:
        for line in f:
            m = re.match(r"\s*\{([^}]*)\},", line)
            if m:
                rows.append([int(v, 16) for v in m.group(1).split(",")])
    if len(rows) < 95:
        raise SystemExit("gfx_font8.h: expected at least 95 glyphs")
    return rows[:95]


def raster8(font, cp):
    from PIL import Image, ImageDraw
    big = 64
    img = Image.new("L", (big * 2, big * 2), 0)
    d = ImageDraw.Draw(img)
    d.text((big // 2, int(big * 1.62)), chr(cp), fill=255, font=font, anchor="ls")
    adv = font.getlength("M")
    crop = img.crop((big // 2 - int(adv * 0.04), int(big * 0.70), big // 2 + int(adv * 1.04), int(big * 1.70)))
    crop = crop.resize((8, 8), Image.BOX)
    out = []
    for y in range(8):
        bits = 0
        for x in range(8):
            if crop.getpixel((x, y)) >= 90:
                bits |= 1 << x
        out.append(bits)
    return out


def compose8(ascii_rows, cp, built=None):
    """Accented letter from its base letter, or None. Lot 72: the base may
    be any glyph already built (a Greek or Cyrillic letter), and macron,
    breve, dot above, double acute and ogonek are known."""
    d = unicodedata.normalize("NFD", chr(cp))
    if len(d) != 2:
        return None
    base, mark = d[0], d[1]
    if 32 <= ord(base) < 127:
        g = list(ascii_rows[ord(base) - 32])
    elif built is not None and ord(base) in built:
        g = list(built[ord(base)])
    else:
        return None
    lower_tab = dict(ACCENT_LOWER)
    upper_tab = dict(ACCENT_UPPER)
    if built is not None:
        lower_tab.update(ACCENT_LOWER_72)
        upper_tab.update(ACCENT_UPPER_72)
    if mark == "\u0327":                          # cedilla: under the body
        g[7] = 0x18
        return g
    if mark == "\u0328" and built is not None:  # ogonek: under the body, right
        g[7] = 0x30
        return g
    if base.islower() or base == "i":
        if mark not in lower_tab:
            return None
        if base in "ij":
            g[0] = 0
        a = lower_tab[mark]
        if base == "y" or base.isupper():
            pass
        g[0], g[1] = a[0], a[1]
        return g
    if mark not in upper_tab:
        return None
    body = [g[0], g[2], g[3], g[5], g[6]]          # 7 rows squashed to 5
    return [upper_tab[mark], 0x00] + body + [g[7]]


def gen_font8(path_ttf, font8_path):
    ascii_rows = read_font8_ascii(font8_path)
    fonts = [(p, load_font(p, 64)) for p in [path_ttf] + FALLBACK]
    font = fonts[0][1]
    built = {}
    lines = [
        "/* gfx_font8.h - 8x8 font. ASCII 32-126: stick font, public domain",
        " * strokes. Lot 71: the other glyphs of include/os_charset.h (CP1252",
        " * 0x80..0x9F, Latin-1) by scripts/gen_bitmap_font.py: accented",
        " * letters composed from their base letter, the rest rasterised from",
        " * IBM Plex Mono (SIL OFL 1.1). Lot 72: the glyph pages of",
        " * include/os_charset_pages.h; Greek / Cyrillic letters shaped like",
        " * Latin ones reuse the ASCII strokes, marked letters are composed,",
        " * the rest rasterised (Noto Sans Mono, SIL OFL 1.1, when Plex lacks",
        " * the code point). Bit x of row y is pixel (x, y). */",
        "#ifndef GFX_FONT8_H",
        "#define GFX_FONT8_H",
        "#include <stdint.h>",
        "static const uint8_t gfx_font8[%d][8] = {" % GLYPHS,
    ]
    for i, cp in enumerate(slot_codepoints()):
        if i < 95:
            g = ascii_rows[i]
        elif cp == ord("?"):
            g = ascii_rows[ord("?") - 32]
        elif cp == 0xA0:
            g = [0] * 8
        elif i < BASE_GLYPHS:
            g = compose8(ascii_rows, cp) or raster8(font, cp)
        elif rule_glyph(cp, 8):
            g = rule_glyph(cp, 8)
        elif cp in HOMOGLYPH8:
            g = list(ascii_rows[ord(HOMOGLYPH8[cp]) - 32])
        else:
            g = compose8(ascii_rows, cp, built) or raster8(pick(fonts, cp), cp)
        built[cp] = g
        lines.append("    { " + ", ".join("0x%02x" % v for v in g) + " }, /* %s */" % tag(cp))
    lines += ["};", "#endif", ""]
    with open(font8_path, "w") as f:
        f.write("\n".join(lines))


def check(paths):
    """Each table has GLYPHS glyphs, glyph 'e-acute' differs from 'e', and
    (Lot 72) the pages header matches page_tables(); Greek alpha and
    Cyrillic zhe are not drawn as '?'."""
    bad = 0
    for p in paths:
        txt = open(p).read()
        if "os_page_cp" in txt:
            m = re.search(r"#define OS_GLYPH_TOTAL (\d+)", txt)
            if not m or int(m.group(1)) != GLYPHS:
                print("%s: OS_GLYPH_TOTAL is not %d" % (p, GLYPHS))
                bad = 1
            tables = re.findall(r"0x([0-9A-F]{4}),", txt.split("os_page_glyph")[0])
            want = sum((page_tables()[k] for k in (1, 2, 3, 4)), [])
            if [int(v, 16) for v in tables] != want:
                print("%s: page tables differ from the generator" % p)
                bad = 1
            continue
        for m in re.finditer(r"(gfx_font\w+)\[(\d+)\]\[(\d+)\] = \{(.*?)\n\};", txt, re.S):
            n = int(m.group(2))
            rows = re.findall(r"\{([^}]*)\},", m.group(4))
            if n != GLYPHS or len(rows) != GLYPHS:
                print("%s %s: %d/%d glyphs" % (p, m.group(1), len(rows), n))
                bad = 1
                continue
            e = rows[ord("e") - 32]
            eacute = rows[127 + 0xE9 - 0xA0]
            if e == eacute:
                print("%s %s: e-acute equals e" % (p, m.group(1)))
                bad = 1
            cps = slot_codepoints()
            q = rows[ord("?") - 32]
            for cp in (0x03B1, 0x0416, 0x0107, 0x2192):
                if rows[cps.index(cp)] == q:
                    print("%s %s: U+%04X drawn as '?'" % (p, m.group(1), cp))
                    bad = 1
    print("font check %s" % ("FAIL" if bad else "ok"))
    return bad


def main():
    if sys.argv[1:2] == ["--check"]:
        sys.exit(check(sys.argv[2:]))
    args = list(sys.argv[1:])
    pages_out = None
    while "--fallback" in args:
        k = args.index("--fallback")
        FALLBACK.append(args[k + 1])
        del args[k:k + 2]
    if "--pages" in args:
        k = args.index("--pages")
        pages_out = args[k + 1]
        del args[k:k + 2]
    path = args[0]
    out = args[1] if len(args) > 1 else "kernel/gfx_font_hd.h"
    gen_hd(path, out)
    if len(args) > 2:
        gen_font8(path, args[2])
    if pages_out:
        gen_pages(pages_out)


if __name__ == "__main__":
    main()
