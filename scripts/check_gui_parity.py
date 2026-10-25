#!/usr/bin/env python3
"""Checks the GUI parity table (userspace/osui_parity.c) against the shell
command registry (shared/multiboot_shell_commands.json): every command a
panel runs must be a real terminal command. With --table, prints the
markdown inventory of the panels (docs/GUI_PARITY.md is built from it)."""
import json, os, re, sys

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))

def registry():
    d = json.load(open(os.path.join(ROOT, "shared", "multiboot_shell_commands.json")))
    names = set()
    for v in d.values():
        if isinstance(v, list):
            for x in v:
                names.add(x if isinstance(x, str) else x.get("name", ""))
    return names

def table():
    src = open(os.path.join(ROOT, "userspace", "osui_parity.c")).read()
    arrays = {}
    for m in re.finditer(r"static const osui_par_entry_t (\w+)\[\] = \{(.*?)\n\};", src, re.S):
        arrays[m.group(1)] = re.findall(r'\{ "([^"]*)", "([^"]*)", ([^,]+), ', m.group(2))
    fams = re.findall(r'\{ OS_FB_APP_(\w+), "(/\w+)", "([^"]*)", "(\w+)", (\w+), ', src)
    return arrays, fams

def main():
    names = registry()
    arrays, fams = table()
    bad = []
    for _, slash, _, _, arr in fams:
        for label, cmd, flags in arrays.get(arr, []):
            if cmd.split()[0] not in names:
                bad.append("%s: %s" % (slash, cmd))
    if len(fams) < 11 or not arrays:
        print("check_gui_parity: table not parsed"); return 1
    if bad:
        print("check_gui_parity: commands missing from the registry:\n  " + "\n  ".join(bad)); return 1
    if "--table" in sys.argv:
        print("| Panel | Family | Commands (MUT = confirm twice, ARG = input line) |")
        print("|---|---|---|")
        for kind, slash, title, fam, arr in fams:
            ents = arrays.get(arr, [])
            cmds = ", ".join("`%s`%s" % (c, "" if f.strip() == "0" else " (" + f.replace("OSUI_PAR_", "").replace("A", "ARG").replace("M", "MUT").replace(" | ", "+") + ")") for _, c, f in ents)
            print("| %s %s | %s | %s |" % (slash, title, fam, cmds or "any registry command typed in the input line"))
    print("check_gui_parity ok families=%d entries=%d" % (len(fams), sum(len(arrays.get(a, [])) for *_, a in fams)))
    return 0

if __name__ == "__main__":
    sys.exit(main())
