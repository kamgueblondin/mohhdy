#!/usr/bin/env python3
"""Lot 90 (US-031): generated suite for custom properties computed where
they are declared, CSS-wide keywords on them and @property, against an
independent model of CSS Variables 1 and the Properties and Values API
subset the reader keeps (see userspace/wf_cssprop.h and wf_sitecss.h).

The model is written from the specs, not from the C code: a tree of
elements, custom property declarations matched by tag, class and id
selectors (specificity then source order), :root declarations kept
sheet-wide (computed on the root), each element's own values computed on
that element and inherited as computed values, a var() that does not
resolve making the property guaranteed-invalid, and registered properties
with a type, an initial value and an inherits flag.

Writes tests/unit/userspace/test_lot90_vars.c. Deterministic (seed 9004).
"""
import os
import random

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
OUT = os.path.join(ROOT, "tests", "unit", "userspace", "test_lot90_vars.c")
DEPTH = 8          # WF_CSSVAR_DEPTH
VVAL = 64          # WF_SITE_VVAL (63 bytes + NUL)
EVARS = 64
NAMES = ["--a", "--b", "--c", "--d"]
PLAIN = ["1px", "2px", "0", "3", "1.5", "red", "4px", "x y"]
KEYWORDS = ["initial", "inherit", "unset"]

INVALID = object()   # guaranteed-invalid


def cstr(s):
    return '"' + s.replace("\\", "\\\\").replace('"', '\\"') + '"'


# ---- var() substitution (CSS Variables 1, section 3) -------------------

class Bad(Exception):
    pass


def subst(text, get, depth=0):
    """Returns the text with every var() substituted; raises Bad."""
    out = []
    i = 0
    n = len(text)
    while i < n:
        if text.startswith("var(", i) and (i == 0 or not (text[i - 1].isalnum() or text[i - 1] in "-_")):
            j = i + 4
            while j < n and text[j] == " ":
                j += 1
            k = j
            while k < n and (text[k].isalnum() or text[k] in "-_"):
                k += 1
            name = text[j:k]
            if not name.startswith("--") or len(name) < 3:
                raise Bad()
            while k < n and text[k] == " ":
                k += 1
            fb = None
            if k < n and text[k] == ",":
                k += 1
                a = k
                d = 0
                while k < n and (d or text[k] != ")"):
                    if text[k] == "(":
                        d += 1
                    elif text[k] == ")":
                        d -= 1
                    k += 1
                if k >= n:
                    raise Bad()
                fb = text[a:k].strip(" ")
            if k >= n or text[k] != ")":
                raise Bad()
            if depth >= DEPTH:
                raise Bad()
            v = get(name)
            if v is not None:
                out.append(subst(v, get, depth + 1))
            elif fb is not None:
                out.append(subst(fb, get, depth + 1))
            else:
                raise Bad()
            i = k + 1
            continue
        out.append(text[i])
        i += 1
    return "".join(out)


def substitute(text, get):
    """None when invalid, else the trimmed result (63 bytes at most)."""
    try:
        r = subst(text, get)
    except Bad:
        return None
    r = r.strip(" ")
    if len(r) > VVAL - 1:
        return None
    return r


def has_var(v):
    return "var(" in v


# ---- registered types ----------------------------------------------------

def is_num(v):
    s = v
    if s[:1] in "+-":
        s = s[1:]
    if not s:
        return False
    if s.count(".") > 1:
        return False
    a, _, b = s.partition(".")
    if "." in s and not b:
        return False
    return (a + b).isdigit() and len(a + b) > 0


def matches(syntax, v):
    if syntax == "*":
        return True
    if syntax == "<length>":
        if v.endswith("px") and is_num(v[:-2]):
            return True
        return is_num(v) and all(c in "0.+-" for c in v)
    if syntax == "<number>":
        return is_num(v)
    raise ValueError(syntax)


def canon(v):
    """a plain number written as computed (sign dropped when +, no trailing
    zeros, at most 4 decimals)"""
    from fractions import Fraction
    x = Fraction(v)
    sign = "-" if x < 0 else ""
    x = abs(x)
    whole = int(x)
    frac = x - whole
    out = sign + str(whole) if (whole or frac) else "0"
    if frac:
        out += "." + ("%04d" % round(frac * 10000)).rstrip("0")
    return out


def computed(syntax, v):
    """Lot 95 (converted): a plain value of a registered type is bound as its
    computed value (a unitless zero <length> is 0px); Lot 90 kept the text"""
    if syntax == "<length>":
        return canon(v[:-2] if v.endswith("px") else v) + "px"
    if syntax == "<number>":
        return canon(v)
    return v


# ---- one case -------------------------------------------------------------

def gen_tree(rng):
    """nodes: (tag, id, cls, parent) with index 0 = html, 1 = body."""
    nodes = [("html", "", "", -1), ("body", "", "", 0)]
    count = rng.randint(3, 7)
    for i in range(count):
        parent = rng.choice([1] + list(range(2, len(nodes))))
        tag = rng.choice(["div", "p", "span"]) if nodes[parent][0] != "p" else "span"
        cls = rng.choice(["", "c1", "c2", "c3"])
        nodes.append((tag, "n%d" % i, cls, parent))
    return nodes


def html_of(nodes, css):
    kids = {}
    for i, n in enumerate(nodes):
        kids.setdefault(n[3], []).append(i)

    def emit(i):
        tag, id_, cls, _ = nodes[i]
        attrs = (" id=" + id_ if id_ else "") + (" class=" + cls if cls else "")
        inner = "".join(emit(k) for k in kids.get(i, []))
        if not inner:
            inner = "t"
        return "<%s%s>%s</%s>" % (tag, attrs, inner, tag)

    body = "".join(emit(k) for k in kids.get(1, []))
    return "<html><head><style>%s</style></head><body>%s</body></html>" % (css, body)


def gen_value(rng, names):
    r = rng.random()
    other = rng.choice(names + ["--z"])
    if r < 0.35:
        return rng.choice(PLAIN)
    if r < 0.5:
        return "var(%s)" % other
    if r < 0.7:
        return "var(%s, %s)" % (other, rng.choice(PLAIN[:5]))
    if r < 0.8:
        return "var(%s) %s" % (other, rng.choice(["1px", "2"]))
    if r < 0.88:
        return "var(%s, var(%s, %s))" % (other, rng.choice(names), rng.choice(PLAIN[:3]))
    return rng.choice(KEYWORDS)


def gen_case(rng):
    nodes = gen_tree(rng)
    names = NAMES[:rng.randint(2, 4)]
    regs = {}
    for nm in names:
        if rng.random() < 0.3:
            syn = rng.choice(["<length>", "<number>", "*"])
            inh = rng.random() < 0.5
            init = {"<length>": "4px", "<number>": "7", "*": rng.choice([None, "q"])}[syn]
            regs[nm] = (syn, inh, init)
    # rules: (selector, spec, name, value); :root ones too
    rules = []
    for _ in range(rng.randint(2, 8)):
        kind = rng.random()
        if kind < 0.2:
            sel, spec = ":root", 10
        elif kind < 0.45:
            sel, spec = rng.choice(["div", "p", "span"]), 1
        elif kind < 0.75:
            sel, spec = "." + rng.choice(["c1", "c2", "c3"]), 10
        else:
            sel, spec = "#n%d" % rng.randrange(len(nodes) - 2), 100
        rules.append((sel, spec, rng.choice(names), gen_value(rng, names)))
    return nodes, names, regs, rules


def match(sel, node):
    tag, id_, cls, parent = node
    if sel == ":root":
        return tag == "html" and parent < 0
    if sel.startswith("."):
        return cls == sel[1:]
    if sel.startswith("#"):
        return id_ == sel[1:]
    return tag == sel


def model(nodes, names, regs, rules):
    # per-element names: declared by a selector other than :root, or registered
    vnames = set(regs)
    for sel, _, nm, _ in rules:
        if sel != ":root":
            vnames.add(nm)
    # sheet-wide: :root declarations of names that are not registered
    sheet_raw = {}
    for sel, _, nm, v in rules:
        if sel == ":root" and nm not in regs:
            sheet_raw[nm] = v
    sheet = {}
    for nm, v in sheet_raw.items():
        if v in KEYWORDS:
            sheet[nm] = INVALID
        elif has_var(v):
            r = substitute(v, lambda x: sheet_raw.get(x))
            sheet[nm] = INVALID if r is None else r
        else:
            sheet[nm] = v

    def sheet_get(x):
        v = sheet.get(x)
        return None if v is INVALID else v

    values = set()
    for nm, (syn, inh, init) in regs.items():
        if init is not None:
            values.add((nm, init))
    for _, _, nm, v in rules:
        values.add((nm, v))

    bind = []   # per node: name -> value str / INVALID / None (unbound)

    def initial(nm):
        reg = regs.get(nm)
        if not reg:
            return None
        return reg[2] if reg[2] is not None else INVALID

    def inherited(i, nm):
        reg = regs.get(nm)
        if reg and not reg[1]:
            return initial(nm)
        par = nodes[i][3]
        if par >= 0 and bind[par].get(nm) is not None:
            return bind[par][nm]
        return initial(nm) if reg else None

    for i, node in enumerate(nodes):
        b = {}
        bind.append(b)
        own = {}
        for nm in vnames:
            best = None
            for order, (sel, spec, rnm, v) in enumerate(rules):
                if rnm != nm or not match(sel, node):
                    continue
                if best is None or spec >= best[0]:
                    best = (spec, v)
            if best is not None:
                own[nm] = best[1]
        for nm in vnames:
            if nm not in own:
                b[nm] = inherited(i, nm)
            else:
                b[nm] = own[nm]
        for nm in list(own):
            v = own[nm]
            if v == "initial":
                b[nm] = initial(nm) if nm in regs else INVALID
            elif v == "inherit":
                par = node[3]
                pv = bind[par].get(nm) if par >= 0 else None
                b[nm] = pv if pv is not None else (initial(nm) if nm in regs else None)
            elif v == "unset":
                b[nm] = inherited(i, nm)
            else:
                continue
            del own[nm]

        def get(x):
            if x in vnames:
                v = b.get(x)
                if v is INVALID:
                    return None
                if v is not None:
                    return v
            return sheet_get(x)

        nb = dict(b)
        for nm, v in own.items():
            if not has_var(v):
                continue
            r = substitute(v, get)
            if r is None:
                nb[nm] = inherited(i, nm) if nm in regs else INVALID
            else:
                nb[nm] = r
                values.add((nm, r))
        for nm in vnames:
            b[nm] = nb[nm]
        for nm in own:
            reg = regs.get(nm)
            if not reg:
                continue
            v = b[nm]
            if v is not INVALID and v is not None and matches(reg[0], v):
                b[nm] = computed(reg[0], v)
                continue
            b[nm] = inherited(i, nm)
    # what wf_site_node_var returns
    res = {}
    for i, node in enumerate(nodes):
        if not node[1]:
            continue
        for nm in names + ["--z"]:
            v = bind[i].get(nm) if nm in vnames else None
            if v is INVALID:
                out = None
            elif v is not None:
                out = v
            else:
                out = sheet_get(nm)
            res[(node[1], nm)] = out
    return res, len(values)


def css_of(regs, rules):
    parts = []
    for nm, (syn, inh, init) in sorted(regs.items()):
        s = "@property %s { syntax: '%s'; inherits: %s" % (nm, syn, "true" if inh else "false")
        if init is not None:
            s += "; initial-value: %s" % init
        parts.append(s + " }")
    for sel, _, nm, v in rules:
        parts.append("%s { %s: %s }" % (sel, nm, v))
    return " ".join(parts)


def main():
    rng = random.Random(9004)
    o = ["/* Generated by scripts/lot90_vargen.py (seed 9004): custom properties",
         " * computed where they are declared, CSS-wide keywords and @property,",
         " * against an independent model. Do not edit by hand. */",
         '#include "../../framework/unity.h"',
         "#include <string.h>",
         '#include "wf_sitecss.h"',
         "",
         "static wf_site_t g_d;",
         "",
         "static const char *nv(const char *id, const char *name) {",
         "    int i;",
         "    const char *v;",
         "    for (i = 0; i < g_d.nnodes; i++) if (!strcmp(g_d.node[i].id, id)) break;",
         "    if (i == g_d.nnodes) return \"NO SUCH ID\";",
         "    v = wf_site_node_var(&g_d, i, name);",
         "    return v ? v : \"(none)\";",
         "}",
         ""]
    names_out = []
    made = 0
    while made < 300:
        nodes, names, regs, rules = gen_case(rng)
        res, nvals = model(nodes, names, regs, rules)
        if nvals > EVARS - 8:
            continue
        css = css_of(regs, rules)
        html = html_of(nodes, css)
        name = "test_l90_vars_%d" % made
        names_out.append(name)
        o.append("static void %s(void) {" % name)
        o.append("    static const char html[] = %s;" % cstr(html))
        o.append("    TEST_ASSERT_EQUAL(0, wf_site_prepare(&g_d, html, (int)strlen(html)));")
        o.append("    TEST_ASSERT_EQUAL(%d, g_d.nprops);" % len(regs))
        for (id_, nm), v in sorted(res.items(), key=lambda kv: (int(kv[0][0][1:]), kv[0][1])):
            o.append("    TEST_ASSERT_EQUAL_STRING(%s, nv(%s, %s));" % (cstr("(none)" if v is None else v), cstr(id_), cstr(nm)))
        o.append("}")
        o.append("")
        made += 1
    o.append("int main(void) {")
    o.append("    unity_init();")
    for n in names_out:
        o.append("    RUN_TEST(%s);" % n)
    o += ["    unity_print_results();", "    unity_cleanup();",
          "    return unity_stats.tests_failed == 0 ? 0 : 1;", "}", ""]
    with open(OUT, "w") as f:
        f.write("\n".join(o))
    print("vars %d" % made)


if __name__ == "__main__":
    main()
