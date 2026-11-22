"""Lot 118: random CSS math expressions (valid and invalid) for the
generated suites and the differential check of the model."""
FUNCS1 = ("abs", "sign", "sqrt", "exp", "sin", "cos", "tan", "asin", "acos", "atan")
NUMS = ("0", "1", "2", "3", "0.5", "1.5", "2.25", ".125", "10", "100", "1000", "2000", "e", "pi", "infinity",
        "-infinity", "NaN", "0.001", "7", "12.5", "1e3", "2.5e-1", "1E1", "1e+2", "12e-3", "3e-4", "0.5e1")


def leaf(rng, typ):
    k = rng.random()
    if typ == "number":
        if k < 0.7:
            return rng.choice(NUMS)
        return "%d.%03d" % (rng.randrange(0, 30), rng.randrange(0, 1000))
    if typ == "angle":
        u = rng.choice(["deg", "deg", "grad", "rad", "turn"])
        n = rng.choice(["0", "1", "30", "45", "60", "90", "180", "270", "360", "0.5", "0.25", "1.5", "720",
                        "%d.%d" % (rng.randrange(0, 400), rng.randrange(0, 10))])
        return n + u
    if k < 0.35:
        return "%dpx" % rng.randrange(0, 300)
    if k < 0.5:
        return "%d.%dpx" % (rng.randrange(0, 100), rng.randrange(0, 1000))
    if k < 0.65:
        return "%d%%" % rng.randrange(0, 101)
    if k < 0.8:
        return "%s%s" % (rng.choice(["1", "2", "0.5", "1.25", "10", "33.3", "50", "3"]),
                         rng.choice(["vw", "vh", "vi", "vb", "vmin", "vmax", "svw", "lvh", "dvmin", "em", "rem", "ch",
                                     "ex", "lh"]))
    if k < 0.9:
        return "%dpx" % rng.randrange(-50, 50)
    if k < 0.95:
        return rng.choice(["1.5e1px", "2e-1px", "5e1%", "1e2px", "25E-1px", "1e-4px"])
    return rng.choice(["0px", "1px", "2000px", "0.001px", "-0.5px"])


def expr(rng, typ, depth=0):
    k = rng.random()
    if depth >= 3 or k < 0.25:
        return leaf(rng, typ)
    if typ == "length":
        choices = ["calc", "minmax", "clamp", "round", "modrem", "abs", "hypot", "sum", "mul", "div", "divlen", "sign"]
    elif typ == "angle":
        choices = ["calc", "minmax", "clamp", "round", "modrem", "abs", "sum", "mul", "inv", "atan2"]
    else:
        choices = ["calc", "minmax", "clamp", "round", "modrem", "abs", "sign", "sum", "mul", "div", "pow", "sqrt",
                   "log", "exp", "trig", "hypot"]
    c = rng.choice(choices)
    sub = lambda t=typ: expr(rng, t, depth + 1)
    if c == "calc":
        return "calc(%s)" % sub()
    if c == "minmax":
        return "%s(%s)" % (rng.choice(["min", "max"]), ", ".join(sub() for _ in range(rng.randrange(1, 4))))
    if c == "clamp":
        return "clamp(%s, %s, %s)" % (sub(), sub(), sub())
    if c == "round":
        s = rng.choice(["", "nearest, ", "up, ", "down, ", "to-zero, "])
        if typ == "number" and rng.random() < 0.3:
            return "round(%s%s)" % (s, sub())
        return "round(%s%s, %s)" % (s, sub(), sub())
    if c == "modrem":
        return "%s(%s, %s)" % (rng.choice(["mod", "rem"]), sub(), sub())
    if c == "abs":
        return "abs(%s)" % sub()
    if c == "sign":
        return "calc(%s * sign(%s))" % (sub(), expr(rng, rng.choice(["length", "number", "angle"]), depth + 1)) \
            if typ != "number" else "sign(%s)" % expr(rng, rng.choice(["length", "number", "angle"]), depth + 1)
    if c == "hypot":
        return "hypot(%s)" % ", ".join(sub() for _ in range(rng.randrange(1, 4)))
    if c == "sum":
        n = rng.randrange(2, 4)
        parts = [sub() for _ in range(n)]
        out = parts[0]
        for p in parts[1:]:
            out += " %s %s" % (rng.choice("+-"), p)
        return "calc(%s)" % out
    if c == "mul":
        if rng.random() < 0.5:
            return "calc(%s * %s)" % (sub(), sub("number"))
        return "calc(%s * %s)" % (sub("number"), sub())
    if c == "div":
        return "calc(%s / %s)" % (sub(), sub("number"))
    if c == "divlen":
        return "calc(%s * (%s / %s))" % (sub(), sub("length"), sub("length"))
    if c == "pow":
        return "pow(%s, %s)" % (sub(), rng.choice(["2", "3", "0.5", "-1", "1.5", "0", "-2", sub()]))
    if c == "sqrt":
        return "sqrt(%s)" % sub()
    if c == "log":
        return "log(%s)" % sub() if rng.random() < 0.5 else "log(%s, %s)" % (sub(), rng.choice(["2", "10", "e", sub()]))
    if c == "exp":
        return "exp(%s)" % sub()
    if c == "trig":
        f = rng.choice(["sin", "cos", "tan"])
        return "%s(%s)" % (f, expr(rng, rng.choice(["angle", "number"]), depth + 1))
    if c == "inv":
        return "%s(%s)" % (rng.choice(["asin", "acos", "atan"]), expr(rng, "number", depth + 1))
    if c == "atan2":
        t = rng.choice(["length", "number", "angle"])
        return "atan2(%s, %s)" % (expr(rng, t, depth + 1), expr(rng, t, depth + 1))
    raise AssertionError(c)


def mangle(rng, s):
    """an invalid neighbour of an expression"""
    k = rng.randrange(9)
    if k == 8:
        return "calc(%s %s)" % (s, rng.choice(["+ 1.e2", "* 1e123", "+ 1e3em", "* 3e", "+ 2e-1vw", "* .e1", "* 9e3"]))
    if k == 0:
        return s.replace(" + ", "+", 1) if " + " in s else s + "px"
    if k == 1:
        return s[:-1]
    if k == 2:
        return s.replace("(", "( foo(", 1)
    if k == 3:
        return "calc(%s + 1)" % s
    if k == 4:
        return s.replace(",", ",,", 1) if "," in s else "calc(%s / 0)" % s
    if k == 5:
        return "calc(-pi * %s)" % s
    if k == 6:
        return "calc(%s * 1deg * 1px)" % s
    return "calc(%s %s)" % (s, rng.choice(["*", "/", "-"]))


def top(rng, typ):
    while True:
        s = expr(rng, typ)
        if "(" in s.split("px")[0][:6] or s[:1].isalpha():
            if s[:1].isalpha() and "(" in s:
                return s
        if not s[:1].isalpha():
            return "calc(%s)" % s
