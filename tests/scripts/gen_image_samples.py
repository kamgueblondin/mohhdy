#!/usr/bin/env python3
"""Lot 45 (Lot 47: PNG 1/2/4/16-bit and Adam7, JPEG fixtures): reference images for the imgdec decoders (BMP 24/32, PNG RGB
dynamic Huffman, PNG palette, PNG gray stored blocks). Writes them to
initrd_content/samples/ and the C byte arrays of the unit test
(tests/unit/userspace/imgdec_samples.h). Deterministic; --check compares."""
import os, struct, sys, zlib

ROOT = os.path.join(os.path.dirname(os.path.abspath(__file__)), "..", "..")
W, H = 48, 32


def px(x, y):
    return ((x * 5) & 255, (y * 8) & 255, ((x + y) * 3) & 255)


def bmp(bits, top_down=False):
    bpp = bits // 8
    stride = (W * bpp + 3) & ~3
    rows = []
    order = range(H) if top_down else range(H - 1, -1, -1)
    for y in order:
        r = bytearray()
        for x in range(W):
            c = px(x, y)
            r += bytes((c[2], c[1], c[0])) + (b"\xff" if bpp == 4 else b"")
        r += b"\0" * (stride - len(r))
        rows.append(bytes(r))
    data = b"".join(rows)
    hdr = struct.pack("<2sIHHI", b"BM", 54 + len(data), 0, 0, 54)
    dib = struct.pack("<IiiHHIIiiII", 40, W, -H if top_down else H, 1, bits, 0, len(data), 2835, 2835, 0, 0)
    return hdr + dib + data


def chunk(t, d):
    return struct.pack(">I", len(d)) + t + d + struct.pack(">I", zlib.crc32(t + d) & 0xffffffff)


def png(ctype, level, split=0):
    raw = bytearray()
    for y in range(H):
        f = y % 5
        row = bytearray()
        for x in range(W):
            c = px(x, y)
            if ctype == 2:
                row += bytes(c)
            elif ctype == 6:
                row += bytes(c) + bytes((200,))
            elif ctype == 0:
                row += bytes((c[0],))
            else:
                row += bytes(((x // 6 + y // 8) % 4,))
        bpp = {0: 1, 2: 3, 3: 1, 6: 4}[ctype]
        prev = raw_rows[-1] if raw_rows else bytes(len(row))
        out = bytearray()
        for i, v in enumerate(row):
            a = row[i - bpp] if i >= bpp else 0
            b = prev[i]
            c = prev[i - bpp] if i >= bpp else 0
            if f == 0: p = 0
            elif f == 1: p = a
            elif f == 2: p = b
            elif f == 3: p = (a + b) >> 1
            else:
                pp = a + b - c
                pa, pb, pc = abs(pp - a), abs(pp - b), abs(pp - c)
                p = a if pa <= pb and pa <= pc else (b if pb <= pc else c)
            out.append((v - p) & 255)
        raw_rows.append(bytes(row))
        raw += bytes((f,)) + out
    raw_rows.clear()
    z = zlib.compress(bytes(raw), level)
    ihdr = struct.pack(">IIBBBBB", W, H, 8, ctype, 0, 0, 0)
    body = chunk(b"IHDR", ihdr)
    if ctype == 3:
        body += chunk(b"PLTE", bytes((255, 0, 0, 0, 255, 0, 0, 0, 255, 255, 255, 255)))
    if split:
        for i in range(0, len(z), split):
            body += chunk(b"IDAT", z[i:i + split])
    else:
        body += chunk(b"IDAT", z)
    return b"\x89PNG\r\n\x1a\n" + body + chunk(b"IEND", b"")


ADAM7 = ((0, 0, 8, 8), (4, 0, 8, 8), (0, 4, 4, 8), (2, 0, 4, 4), (0, 2, 2, 4), (1, 0, 2, 2), (0, 1, 1, 2))


def gray_level(x, y, depth):
    """reference gray sample at bit depth (Lot 47)"""
    return ((x + 2 * y) * 7) % (1 << depth)


def png_ext(ctype, depth, interlace):
    """Lot 47: any bit depth, optional Adam7; filter type cycles per row."""
    ch = {0: 1, 2: 3, 3: 1, 4: 2, 6: 4}[ctype]

    def samples(x, y):
        if ctype in (0, 3):
            return [gray_level(x, y, depth) if ctype == 0 else (x // 6 + y // 8) % 4]
        c = px(x, y)
        base = list(c) if ctype in (2, 6) else [c[0]]
        if ctype in (4, 6):
            base.append(200)
        if depth == 16:
            return [v * 256 + ((x * 31 + y) & 255) for v in base]
        return base

    def row_bytes(xs, y):
        bits = []
        for x in xs:
            for v in samples(x, y):
                bits.append(v)
        if depth == 16:
            return b"".join(struct.pack(">H", v) for v in bits)
        if depth == 8:
            return bytes(bits)
        out = bytearray()
        per = 8 // depth
        for i in range(0, len(bits), per):
            b = 0
            for k in range(per):
                v = bits[i + k] if i + k < len(bits) else 0
                b |= v << (8 - depth * (k + 1))
            out.append(b)
        return bytes(out)

    fbpp = max(1, ch * depth // 8)
    passes = ADAM7 if interlace else ((0, 0, 1, 1),)
    raw = bytearray()
    for (x0, y0, dx, dy) in passes:
        xs = list(range(x0, W, dx))
        ys = list(range(y0, H, dy))
        if not xs or not ys:
            continue
        prev = None
        for j, y in enumerate(ys):
            row = row_bytes(xs, y)
            f = j % 5
            pr = prev if prev is not None else bytes(len(row))
            out = bytearray()
            for i, v in enumerate(row):
                a = row[i - fbpp] if i >= fbpp else 0
                b = pr[i]
                c = pr[i - fbpp] if i >= fbpp else 0
                if f == 0: p = 0
                elif f == 1: p = a
                elif f == 2: p = b
                elif f == 3: p = (a + b) >> 1
                else:
                    pp = a + b - c
                    pa, pb, pc = abs(pp - a), abs(pp - b), abs(pp - c)
                    p = a if pa <= pb and pa <= pc else (b if pb <= pc else c)
                out.append((v - p) & 255)
            raw += bytes((f,)) + out
            prev = row
    ihdr = struct.pack(">IIBBBBB", W, H, depth, ctype, 0, 0, 1 if interlace else 0)
    body = chunk(b"IHDR", ihdr)
    if ctype == 3:
        body += chunk(b"PLTE", bytes((255, 0, 0, 0, 255, 0, 0, 0, 255, 255, 255, 255)))
    body += chunk(b"IDAT", zlib.compress(bytes(raw), 9))
    return b"\x89PNG\r\n\x1a\n" + body + chunk(b"IEND", b"")


def jpeg_fixture(name, **opts):
    """JPEG fixtures are committed files: libjpeg output may differ between
    versions, so they are only (re)created with Pillow when missing."""
    path = os.path.join(ROOT, "initrd_content", "samples", name)
    if not os.path.exists(path):
        from PIL import Image
        im = Image.new("RGB", (W, H))
        im.putdata([px(x, y) for y in range(H) for x in range(W)])
        if opts.pop("gray", False):
            im = im.convert("L")
        im.save(path, "JPEG", **opts)
    return open(path, "rb").read()


raw_rows = []
SAMPLES = [
    ("mire24.bmp", bmp(24)),
    ("mire32.bmp", bmp(32, top_down=True)),
    ("mire.png", png(2, 9, split=100)),
    ("mire-rgba.png", png(6, 6)),
    ("mire-pal.png", png(3, 9)),
    ("mire-gray.png", png(0, 0)),
    ("mire-rgb16.png", png_ext(2, 16, False)),
    ("mire-rgba16-i.png", png_ext(6, 16, True)),
    ("mire-rgb-i.png", png_ext(2, 8, True)),
    ("mire-g1.png", png_ext(0, 1, False)),
    ("mire-g2-i.png", png_ext(0, 2, True)),
    ("mire-g4.png", png_ext(0, 4, False)),
    ("mire-g16.png", png_ext(0, 16, False)),
    ("mire-pal2-i.png", png_ext(3, 2, True)),
    ("mire-ga16.png", png_ext(4, 16, False)),
    ("mire444.jpg", jpeg_fixture("mire444.jpg", quality=95, subsampling=0)),
    ("mire420.jpg", jpeg_fixture("mire420.jpg", quality=90, subsampling=2)),
    ("mire-gray.jpg", jpeg_fixture("mire-gray.jpg", quality=92, gray=True)),
    ("mire-rst.jpg", jpeg_fixture("mire-rst.jpg", quality=90, subsampling=2, restart_marker_blocks=2)),
    ("mire-prog.jpg", jpeg_fixture("mire-prog.jpg", quality=90, progressive=True)),
]


def header():
    out = ["/* generated by tests/scripts/gen_image_samples.py - do not edit */",
           "#define IMGS_W %d" % W, "#define IMGS_H %d" % H]
    for name, data in SAMPLES:
        ident = "img_" + name.replace(".", "_").replace("-", "_")
        out.append("static const unsigned char %s[%d] = {" % (ident, len(data)))
        for i in range(0, len(data), 20):
            out.append("    " + ",".join(str(b) for b in data[i:i + 20]) + ",")
        out.append("};")
    text = b"Mohhdy imgdec fixed Huffman " * 20
    co = zlib.compressobj(9, zlib.DEFLATED, 15, 9, zlib.Z_FIXED)
    z = co.compress(text) + co.flush()
    out.append("#define ZFIXED_TEXT \"%s\"" % text[:28].decode())
    out.append("#define ZFIXED_LEN %d" % len(text))
    out.append("static const unsigned char zfixed[%d] = {%s};" % (len(z), ",".join(str(b) for b in z)))
    return "\n".join(out) + "\n"


def main():
    check = "--check" in sys.argv
    files = [(os.path.join(ROOT, "initrd_content", "samples", n), d) for n, d in SAMPLES]
    files.append((os.path.join(ROOT, "tests", "unit", "userspace", "imgdec_samples.h"), header().encode()))
    bad = 0
    for path, data in files:
        if check:
            try:
                same = open(path, "rb").read() == data
            except OSError:
                same = False
            if not same:
                print("stale:", path); bad = 1
        else:
            open(path, "wb").write(data)
    print("image samples", "stale" if bad else "ok")
    return bad


if __name__ == "__main__":
    sys.exit(main())
