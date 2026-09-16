#!/usr/bin/env python3
"""Registre partage des commandes shell Multiboot (osui + guest).

Source de verite : userspace/shell.c, matérialisée dans
shared/multiboot_shell_commands.json (regenere par
osui/scripts/extract_guest_commands.py).
"""

from __future__ import annotations

import json
from pathlib import Path

OSUI_ROOT = Path(__file__).resolve().parent
REPO_ROOT = OSUI_ROOT.parent
DEFAULT_JSON = REPO_ROOT / "shared" / "multiboot_shell_commands.json"

FALLBACK_LINUX_TRAPS = (
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
)
FALLBACK_OSUI_EXTRA = ("attach", "detach", "guest-status", "open")


class CommandRegistry:
    """Liste unique des builtins guest + extras osui."""

    def __init__(self, payload: dict) -> None:
        self.source = str(payload.get("source") or "userspace/shell.c")
        self.prompt = str(payload.get("prompt") or "MOHHDY>")
        self.guest_commands = tuple(payload.get("commands") or ())
        self.osui_extra = tuple(payload.get("osui_extra") or FALLBACK_OSUI_EXTRA)
        self.linux_traps = frozenset(payload.get("linux_traps") or FALLBACK_LINUX_TRAPS)
        self.guest_html_stage = bool(payload.get("guest_html_stage", False))
        self.note = str(payload.get("note") or "")
        names = list(self.guest_commands)
        for extra in self.osui_extra:
            if extra not in names:
                names.append(extra)
        self.all_names = tuple(names)

    def is_guest_command(self, name: str) -> bool:
        return name in self.guest_commands

    def is_linux_trap(self, name: str) -> bool:
        return name in self.linux_traps

    def public_dict(self) -> dict:
        return {
            "source": self.source,
            "prompt": self.prompt,
            "count": len(self.guest_commands),
            "commands": list(self.guest_commands),
            "osui_extra": list(self.osui_extra),
            "linux_traps": sorted(self.linux_traps),
            "guest_html_stage": self.guest_html_stage,
            "artifact": "shared/multiboot_shell_commands.json",
            "header": "userspace/mohhdy_osui_bridge.h",
            "note": self.note,
        }


def load_registry(path: Path | None = None) -> CommandRegistry:
    candidate = path or DEFAULT_JSON
    if candidate.is_file():
        payload = json.loads(candidate.read_text(encoding="utf-8"))
        return CommandRegistry(payload)
    return CommandRegistry(
        {
            "source": "userspace/shell.c",
            "prompt": "MOHHDY>",
            "commands": [],
            "osui_extra": list(FALLBACK_OSUI_EXTRA),
            "linux_traps": list(FALLBACK_LINUX_TRAPS),
            "guest_html_stage": False,
            "note": "JSON registre absent ; extras osui seulement.",
        }
    )


REGISTRY = load_registry()
