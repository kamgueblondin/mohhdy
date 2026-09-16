#!/usr/bin/env python3
"""Extrait le registre de commandes guest depuis userspace/shell.c.

Source de verite : le C Ring 3. Artefacts :
  shared/multiboot_shell_commands.json
  userspace/mohhdy_osui_bridge.h

Usage :
  python3 scripts/extract_guest_commands.py
  python3 scripts/extract_guest_commands.py --check
"""

from __future__ import annotations

import argparse
import json
import re
import sys
from pathlib import Path

REPO = Path(__file__).resolve().parents[1]
SHELL_C = REPO / "userspace" / "shell.c"
JSON_OUT = REPO / "shared" / "multiboot_shell_commands.json"
HEADER_OUT = REPO / "userspace" / "mohhdy_osui_bridge.h"

BUILTIN_RE = re.compile(
    r"static const char\* names\[\] = \{(.*?)\n\s*\};",
    re.DOTALL,
)
STRING_RE = re.compile(r'"([a-zA-Z0-9_./\[\]?-]+)"')
STRCMP_RE = re.compile(
    r'strcmp\(\s*command\s*,\s*"([a-zA-Z0-9_./\[\]?-]+)"\s*\)'
)
LINUX_TRAPS = [
    "bash",
    "sh",
    "zsh",
    "apt",
    "apt-get",
    "yum",
    "dnf",
    "dpkg",
    "sudo",
    "systemctl",
    "chmod",
    "chown",
    "uname",
    "docker",
    "systemd",
]
OSUI_EXTRA = [
    "session-new",
    "session-use",
    "session-status",
    "session-list",
    "chat",
    "prompt",
    "grant",
    "revoke",
    "escalate",
    "takeover",
    "admin-status",
    "origin-check",
    "browser-click",
    "browser-type",
    "browser-pointer",
    "browser-status",
    "mcp-invoice",
    "mcp-invoke",
    "fs-list",
    "fs-read",
    "fs-write",
    "stage",
    "stage-prompt",
    "os-help",
    "os-status",
    "os-browser",
    "os-shell",
    "os-admin",
    "os-support",
    "os-fs",
    "os-center",
    "os-close",
    "guest-status",
    "attach",
    "detach",
    "gui",
    "graphics",
    "desktop",
    "console",
    "gui-status",
    "gui-exit",
    "gui-move",
    "open",
]


def extract_commands(source: str) -> dict:
    builtin_block = ""
    match = BUILTIN_RE.search(source)
    if match:
        builtin_block = match.group(1)
    builtin = []
    seen = set()
    for token in STRING_RE.findall(builtin_block):
        if token in ("0",) or not token:
            continue
        if token not in seen:
            seen.add(token)
            builtin.append(token)
    dispatch = []
    for token in STRCMP_RE.findall(source):
        if token not in seen:
            seen.add(token)
            dispatch.append(token)
    commands = sorted(seen, key=lambda name: (name.lower(), name))
    return {
        "builtin": builtin,
        "dispatch_only": dispatch,
        "commands": commands,
    }


def manifest(parsed: dict) -> dict:
    return {
        "schema": "mohhdy.multiboot_shell_commands.v1",
        "source": "userspace/shell.c",
        "prompt": "MOHHDY>",
        "extracted_from": ["is_builtin", "dispatch_strcmp"],
        "guest_html_stage": False,
        "linux_bash": False,
        "commands": parsed["commands"],
        "builtin": parsed["builtin"],
        "dispatch_only": parsed["dispatch_only"],
        "osui_extra": list(OSUI_EXTRA),
        "linux_traps": list(LINUX_TRAPS),
        "note": (
            "Registre partage genere depuis userspace/shell.c. "
            "Les noms OS-UI (sessions, MCP, scene, gui) sont des builtins Ring 3. "
            "Commande canonique du bureau : gui (aliases graphics, desktop). "
            "Pas un bash Linux. Surface produit = bureau VBE QEMU, cerveau = C. "
            "guest_html_stage=false python_facade=false display_host=false."
        ),
    }


def header_text(data: dict) -> str:
    lines = [
        "/* mohhdy_osui_bridge.h - contrat OS-UI <-> guest Ring 3.",
        " * Genere par scripts/extract_guest_commands.py depuis userspace/shell.c.",
        " * Ne pas editer a la main. Surface produit = bureau VBE QEMU ; cerveau guest = C.",
        " */",
        "#ifndef MOHHDY_OSUI_BRIDGE_H",
        "#define MOHHDY_OSUI_BRIDGE_H",
        "",
        "#define MOHHDY_OSUI_GUEST_HTML_STAGE 0",
        "#define MOHHDY_OSUI_LIVE_ATTACH_HOST 0",
        "#define MOHHDY_OSUI_STAGE_VGA 1",
        "#define MOHHDY_OSUI_VGA_DESKTOP 1",
        "#define MOHHDY_OSUI_DISPLAY_HOST 0",
        '#define MOHHDY_OSUI_GUI_COMMAND "gui"',
        "#define MOHHDY_OSUI_PYTHON_FACADE 0",
        '#define MOHHDY_OSUI_LLM_KIND "stub_echo"',
        '#define MOHHDY_SHELL_PROMPT "MOHHDY>"',
        "#define MOHHDY_SHELL_COMMAND_COUNT %d" % len(data["commands"]),
        "",
        "static const char * const mohhdy_shell_commands[] = {",
    ]
    for name in data["commands"]:
        lines.append('    "%s",' % name)
    lines.extend(
        [
            "    0",
            "};",
            "",
            "#endif /* MOHHDY_OSUI_BRIDGE_H */",
            "",
        ]
    )
    return "\n".join(lines)


def write_artifacts(data: dict) -> None:
    JSON_OUT.parent.mkdir(parents=True, exist_ok=True)
    JSON_OUT.write_text(json.dumps(data, indent=2, ensure_ascii=True) + "\n", encoding="utf-8")
    HEADER_OUT.write_text(header_text(data), encoding="utf-8")


def load_committed() -> dict:
    return json.loads(JSON_OUT.read_text(encoding="utf-8"))


def main(argv: list[str] | None = None) -> int:
    parser = argparse.ArgumentParser(description="Extrait le registre shell guest")
    parser.add_argument("--check", action="store_true", help="echoue si JSON/header stales")
    args = parser.parse_args(argv)
    source = SHELL_C.read_text(encoding="utf-8", errors="replace")
    data = manifest(extract_commands(source))
    if args.check:
        committed = load_committed()
        if committed.get("commands") != data["commands"]:
            sys.stderr.write("FAIL: shared/multiboot_shell_commands.json stale vs userspace/shell.c\n")
            return 1
        expected_header = header_text(data)
        current_header = HEADER_OUT.read_text(encoding="utf-8")
        if current_header != expected_header:
            sys.stderr.write("FAIL: userspace/mohhdy_osui_bridge.h stale\n")
            return 1
        sys.stdout.write("OK guest command registry aligned (%d names)\n" % len(data["commands"]))
        return 0
    write_artifacts(data)
    sys.stdout.write(
        "wrote %s (%d commands) and %s\n"
        % (JSON_OUT.relative_to(REPO), len(data["commands"]), HEADER_OUT.relative_to(REPO))
    )
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
