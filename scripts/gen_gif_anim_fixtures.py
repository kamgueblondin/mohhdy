#!/usr/bin/env python3
"""Lot 73: writes tests/unit/userspace/imgdec_anim_samples.h (animated GIF
fixtures and the expected composed canvases).

The GIFs are written by a small encoder of this script (no Pillow), so
every field is chosen: frame rectangles, local color tables, transparent
index, delays, disposal methods, interlace and the NETSCAPE2.0 loop count.
The expected canvases come from an independent model of the GIF89a rules
(white background, disposal 2 restores the rectangle to the background,
disposal 3 to the picture before the frame). --check compares the header
with a fresh run (make font-check)."""
import sys

WHITE = (255, 255, 255)


def lzw_raw(indices, min_code):
    """LZW stream with a clear code before every run of literal codes, so
    no string table entry is ever used and the code width stays
    min_code + 1 (a valid stream every decoder must read)."""
    clear, eoi = 1 << min_code, (1 << min_code) + 1
    width = min_code + 1
    run = (1 << width) - clear - 3          # literals before the table widens
    if run < 1:
        run = 1
    codes = []
    for i, v in enumerate(indices):
        if i % run == 0:
            codes.append(clear)
        codes.append(v)
    codes.append(eoi)
    out, acc, nbits = bytearray(), 0, 0
    for c in codes:
        acc |= c << nbits
        nbits += width
        while nbits >= 8:
            out.append(acc & 255)
            acc >>= 8
            nbits -= 8
    if nbits:
        out.append(acc & 255)
    blocks = bytearray()
    for i in range(0, len(out), 255):
        chunk = out[i:i + 255]
        blocks.append(len(chunk))
        blocks += chunk
    blocks.append(0)
    return bytes(blocks)


def table(colors, bits):
    n = 1 << bits
    t = bytearray()
    for k in range(n):
        t += bytes(colors[k] if k < len(colors) else (0, 0, 0))
    return bytes(t)


def interlace_order(h):
    return [y for start, step in ((0, 8), (4, 8), (2, 4), (1, 2)) for y in range(start, h, step)]


def gif(w, h, gct, frames, loop=None):
    """frames: dicts x y w h pix (row-major indices) delay disposal
    transparent (or None) lct (colors or None) interlace"""
    bits = 2 if len(gct) <= 4 else 3 if len(gct) <= 8 else 4
    d = bytearray(b"GIF89a")
    d += bytes([w & 255, w >> 8, h & 255, h >> 8, 0x80 | ((bits - 1) << 4) | (bits - 1), 0, 0])
    d += table(gct, bits)
    if loop is not None:
        d += bytes([0x21, 0xFF, 11]) + b"NETSCAPE2.0" + bytes([3, 1, loop & 255, loop >> 8, 0])
    for f in frames:
        tr = f.get("transparent")
        packed = (f.get("disposal", 0) << 2) | (1 if tr is not None else 0)
        dl = f.get("delay", 0)
        d += bytes([0x21, 0xF9, 4, packed, dl & 255, dl >> 8, tr or 0, 0])
        lct = f.get("lct")
        fl = 0x40 if f.get("interlace") else 0
        lbits = bits
        if lct:
            lbits = 2 if len(lct) <= 4 else 3 if len(lct) <= 8 else 4
            fl |= 0x80 | (lbits - 1)
        d += bytes([0x2C, f["x"] & 255, f["x"] >> 8, f["y"] & 255, f["y"] >> 8,
                    f["w"] & 255, f["w"] >> 8, f["h"] & 255, f["h"] >> 8, fl])
        if lct:
            d += table(lct, lbits)
        pix = f["pix"]
        if f.get("interlace"):
            pix = [pix[y * f["w"] + x] for y in interlace_order(f["h"]) for x in range(f["w"])]
        mc = max(2, lbits)
        d.append(mc)
        d += lzw_raw(pix, mc)
    d.append(0x3B)
    return bytes(d)


def compose(w, h, gct, frames):
    """canvas after each frame (GIF89a disposal rules, white background)"""
    canvas = [WHITE] * (w * h)
    out, prev = [], None
    for f in frames:
        if prev is not None:
            pf, saved = prev
            for y in range(pf["y"], min(h, pf["y"] + pf["h"])):
                for x in range(pf["x"], min(w, pf["x"] + pf["w"])):
                    if pf.get("disposal") == 2:
                        canvas[y * w + x] = WHITE
                    elif pf.get("disposal") == 3:
                        canvas[y * w + x] = saved[y * w + x]
        saved = list(canvas)
        pal = f.get("lct") or gct
        for yy in range(f["h"]):
            for xx in range(f["w"]):
                v = f["pix"][yy * f["w"] + xx]
                x, y = f["x"] + xx, f["y"] + yy
                if v == f.get("transparent") or x >= w or y >= h:
                    continue
                canvas[y * w + x] = pal[v] if v < len(pal) else pal[0]
        out.append(list(canvas))
        prev = (f, saved)
    return out


RED, GREEN, BLUE, BLACK = (220, 20, 30), (10, 200, 40), (20, 40, 230), (0, 0, 0)
YELLOW, CYAN = (250, 230, 0), (0, 210, 220)


def fixtures():
    gct = [WHITE, RED, GREEN, BLUE]
    W, H = 8, 6
    fr = [
        dict(x=0, y=0, w=8, h=6, pix=[1] * 48, delay=10, disposal=1),
        # green rectangle, one transparent pixel (index 0 marked transparent)
        dict(x=2, y=1, w=3, h=2, pix=[2, 2, 2, 2, 0, 2], delay=20, disposal=2, transparent=0),
        # blue, restored to the previous picture afterwards
        dict(x=4, y=2, w=2, h=3, pix=[3] * 6, delay=0, disposal=3),
        # local table, interlaced 1 x 5 column with two colors
        dict(x=7, y=0, w=1, h=5, pix=[1, 2, 1, 2, 1], delay=5, disposal=0, interlace=True,
             lct=[BLACK, YELLOW, CYAN, WHITE]),
    ]
    a = gif(W, H, gct, fr, loop=3)
    ac = compose(W, H, gct, fr)
    # no loop extension, two frames, frame partly off the screen
    fr2 = [dict(x=0, y=0, w=4, h=4, pix=[2] * 16, delay=7),
           dict(x=2, y=2, w=4, h=4, pix=[3] * 16, delay=9)]
    b = gif(4, 4, gct, fr2)
    bc = compose(4, 4, gct, fr2)
    # browser page animation: three full frames, 32 x 24, forever, 0.5 s
    gb = [WHITE, RED, GREEN, BLUE]
    fr3 = [dict(x=0, y=0, w=32, h=24, pix=[k] * (32 * 24), delay=50, disposal=1) for k in (1, 2, 3)]
    c = gif(32, 24, gb, fr3, loop=0)
    # plays twice then stops on its last frame (loop 1), 0.3 s
    fr4 = [dict(x=0, y=0, w=32, h=24, pix=[k] * (32 * 24), delay=30, disposal=1) for k in (2, 1)]
    d = gif(32, 24, gb, fr4, loop=1)
    return [("anim_a", a, (W, H, ac)), ("anim_b", b, (4, 4, bc)), ("anim_page", c, None), ("anim_twice", d, None)]


def render():
    out = ["/* Generated by scripts/gen_gif_anim_fixtures.py (Lot 73). Do not edit. */\n"]
    for name, data, exp in fixtures():
        out.append("static const unsigned char gif_%s[%d] = {\n" % (name, len(data)))
        for i in range(0, len(data), 16):
            out.append("    " + ", ".join("0x%02x" % c for c in data[i:i + 16]) + ",\n")
        out.append("};\n")
        if exp:
            w, h, frames = exp
            out.append("#define GIF_%s_W %d\n#define GIF_%s_H %d\n#define GIF_%s_FRAMES %d\n"
                       % (name.upper(), w, name.upper(), h, name.upper(), len(frames)))
            out.append("static const unsigned char gif_%s_canvas[%d][%d] = {\n" % (name, len(frames), w * h * 3))
            for c in frames:
                flat = [v for px in c for v in px]
                out.append("    {" + ", ".join(str(v) for v in flat) + "},\n")
            out.append("};\n")
    return "".join(out)


def main(argv):
    path = "tests/unit/userspace/imgdec_anim_samples.h"
    text = render()
    if "--check" in argv:
        try:
            cur = open(path).read()
        except OSError:
            cur = ""
        if cur != text:
            print("FAIL: %s is not what scripts/gen_gif_anim_fixtures.py writes" % path)
            return 1
        print("OK gif anim fixtures")
        return 0
    open(path, "w").write(text)
    return 0


if __name__ == "__main__":
    sys.exit(main(sys.argv[1:]))
