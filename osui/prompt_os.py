#!/usr/bin/env python3
"""Routes langage naturel du Prompt OS (chat central).

Le chat reste la surface de commande. Slash inchanges. Les prompts
peuvent ouvrir un programme, dessiner sur #ai-stage, lancer un mini-plan
autonome (stub), ou executer un builtin Multiboot sur (help / ai-help).
"""

from __future__ import annotations

import re
from typing import Optional

PANES = ("browser", "shell", "admin", "support", "status", "fs")

SLASH_COMMANDS = (
    {"slash": "/help", "summary": "Liste les raccourcis du SE"},
    {"slash": "/browser", "summary": "Ouvre Browser-OS (simulateur DOM)"},
    {"slash": "/shell", "summary": "Ouvre le shell Multiboot (vocabulaire guest Ring 3)"},
    {"slash": "/admin", "summary": "Ouvre Admin (grant/revoke, takeover)"},
    {"slash": "/support", "summary": "Ouvre Support (sessions, escalade)"},
    {"slash": "/status", "summary": "Ouvre Statut instance"},
    {"slash": "/fs", "summary": "Ouvre le FS sandbox lecture"},
    {"slash": "/center", "summary": "Ferme les programmes et ramene le chat au centre"},
    {"slash": "/close", "summary": "Ferme les programmes"},
    {"slash": "/plan", "summary": "Mini-plan autonome stub sur #ai-stage"},
    {"slash": "/draw", "summary": "Dessine sur la scene IA (stub HTML/SVG)"},
    {"slash": "/stage", "summary": "Met a jour la scene IA sans ouvrir de pane"},
    {"slash": "/guest", "summary": "Statut d'attache guest (bootstrap/live)"},
    {"slash": "/ai-help", "summary": "Aide IA Multiboot (stub llm=stub_echo)"},
)

_OPEN_PATTERNS = (
    (re.compile(r"\b(open|ouvre|lance|start|ouvrir)\b[\s\S]*\b(browser|navigateur|browser-os)\b", re.I), "browser"),
    (re.compile(r"\b(open|ouvre|lance|start|ouvrir)\b[\s\S]*\b(shell|terminal|multiboot)\b", re.I), "shell"),
    (re.compile(r"\b(open|ouvre|lance|start|ouvrir)\b[\s\S]*\b(admin|console)\b", re.I), "admin"),
    (re.compile(r"\b(open|ouvre|lance|start|ouvrir)\b[\s\S]*\b(support|sessions)\b", re.I), "support"),
    (re.compile(r"\b(open|ouvre|lance|start|ouvrir)\b[\s\S]*\b(status|statut|sante)\b", re.I), "status"),
    (re.compile(r"\b(open|ouvre|lance|start|ouvrir)\b[\s\S]*\b(fs|fichiers|files|sandbox)\b", re.I), "fs"),
    (re.compile(r"\b(montre|afficher|affiche)\b[\s\S]*\b(navigateur|browser)\b", re.I), "browser"),
    (re.compile(r"\b(montre|afficher|affiche)\b[\s\S]*\b(shell|terminal)\b", re.I), "shell"),
    (re.compile(r"\b(montre|afficher|affiche)\b[\s\S]*\b(admin)\b", re.I), "admin"),
)

_PLAN_RE = re.compile(
    r"\b(plan autonome|mini-plan|etape par etape|etapes|autonome|"
    r"agis puis (presente|dessine)|reflecting then acting|"
    r"reflechis puis|multi-step|sequenc)\b",
    re.I,
)
_DRAW_RE = re.compile(
    r"\b(dessine|dessiner|draw|trace|figure|svg|boites|cercle|graphe|arbre)\b",
    re.I,
)
_SIM_RE = re.compile(
    r"\b(simule|simulation|anime|animer|agir|agis)\b",
    re.I,
)
_SHELL_HELP_RE = re.compile(
    r"\b((run|execute|lance|tape|affiche|montre)\b[\s\S]*\bhelp\b|"
    r"liste(r)? les commandes( guest| multiboot)?|"
    r"aide (du )?shell)\b",
    re.I,
)
_AI_HELP_RE = re.compile(
    r"\b(ai-help|aihelp|aide ia|guide ia|aide de l[' ]ia)\b",
    re.I,
)
_GUEST_RE = re.compile(
    r"\b(guest-status|statut( du)? guest|live_guest|attache( live)?|"
    r"qemu serial|guest live)\b",
    re.I,
)
_VFS_RE = re.compile(
    r"\b(vfs-list|liste(r)? (l[' ]?)?(initrd|vfs))\b",
    re.I,
)
_AI_STUB_RE = re.compile(
    r"^\s*ai\s+\S+",
    re.I,
)
_SAFE_SHELL = frozenset(
    {
        "help",
        "ai-help",
        "aihelp",
        "ai-runtime",
        "ai-stats",
        "guest-status",
        "sysinfo",
        "whoami",
        "vfs-list",
        "vfs-stats",
        "ls",
        "pwd",
        "net-status",
        "which",
    }
)


def _slash_name(token: str) -> Optional[str]:
    name = token.lower().lstrip("/")
    for row in SLASH_COMMANDS:
        if row["slash"][1:] == name:
            return name
    aliases = {
        "?": "help",
        "aide": "help",
        "nav": "browser",
        "browser-os": "browser",
        "sh": "shell",
        "terminal": "shell",
        "multiboot": "shell",
        "console": "admin",
        "sessions": "support",
        "sante": "status",
        "health": "status",
        "files": "fs",
        "fichiers": "fs",
        "centre": "center",
        "desktop": "center",
        "fermer": "close",
        "dessine": "draw",
        "scene": "stage",
    }
    return aliases.get(name)


def route_prompt(text: str) -> dict:
    """Classe un prompt chat : slash, ouverture, scene, shell sure, ou chat."""
    trimmed = (text or "").strip()
    if not trimmed:
        return {"kind": "empty", "text": ""}
    if trimmed.startswith("/"):
        token = trimmed.split()[0]
        name = _slash_name(token)
        rest = trimmed[len(token) :].strip()
        if name is None:
            return {"kind": "unknown_slash", "token": token, "text": trimmed}
        if name == "help":
            return {"kind": "slash", "name": "help", "text": trimmed}
        if name in ("center", "close"):
            return {"kind": "slash", "name": name, "text": trimmed}
        if name in PANES:
            return {"kind": "open_pane", "pane": name, "slash": True, "text": trimmed}
        if name == "plan":
            return {
                "kind": "stage_plan",
                "autonomous": True,
                "prompt": rest or "mini-plan autonome stub",
                "text": trimmed,
            }
        if name == "draw":
            return {
                "kind": "stage",
                "prompt": rest or "dessine trois boites",
                "text": trimmed,
            }
        if name == "stage":
            return {
                "kind": "stage",
                "prompt": rest or trimmed,
                "text": trimmed,
            }
        if name == "guest":
            return {"kind": "shell", "line": "guest-status", "open_shell": True, "text": trimmed}
        if name == "ai-help":
            return {"kind": "shell", "line": "ai-help", "open_shell": False, "text": trimmed}
        return {"kind": "slash", "name": name, "text": trimmed}

    for pattern, pane in _OPEN_PATTERNS:
        if pattern.search(trimmed):
            return {"kind": "open_pane", "pane": pane, "slash": False, "text": trimmed}

    if _PLAN_RE.search(trimmed):
        return {"kind": "stage_plan", "autonomous": True, "prompt": trimmed, "text": trimmed}

    if _AI_HELP_RE.search(trimmed):
        return {"kind": "shell", "line": "ai-help", "open_shell": False, "text": trimmed}

    if _GUEST_RE.search(trimmed):
        return {"kind": "shell", "line": "guest-status", "open_shell": True, "text": trimmed}

    if _SHELL_HELP_RE.search(trimmed):
        return {"kind": "shell", "line": "help", "open_shell": True, "text": trimmed}

    if _VFS_RE.search(trimmed):
        line = "vfs-list initrd/bin/" if "initrd" in trimmed.lower() else "vfs-list initrd/"
        if trimmed.lower().startswith("vfs-list"):
            line = trimmed
        return {"kind": "shell", "line": line, "open_shell": True, "text": trimmed}

    first = trimmed.split()[0].lower()
    if first in _SAFE_SHELL and (
        first in ("help", "ai-help", "aihelp", "guest-status", "sysinfo", "whoami")
        or trimmed.lower().startswith("vfs-list")
    ):
        return {"kind": "shell", "line": trimmed, "open_shell": first != "help", "text": trimmed}

    if _AI_STUB_RE.match(trimmed):
        return {"kind": "shell", "line": trimmed, "open_shell": False, "stage": True, "text": trimmed}

    if _DRAW_RE.search(trimmed) or _SIM_RE.search(trimmed):
        return {"kind": "stage", "prompt": trimmed, "text": trimmed}

    return {"kind": "chat", "prompt": trimmed, "stage": True, "text": trimmed}


def help_text() -> str:
    lines = [
        "Raccourcis du SE (chat central). llm=stub_echo.",
        "Un programme ouvert deplace le chat en panneau flottant (coin, draggable).",
        "Prompts : ouvrir shell/browser/admin, dessiner, mini-plan autonome, help guest.",
    ]
    for row in SLASH_COMMANDS:
        lines.append("%s  %s" % (row["slash"], row["summary"]))
    lines.append('Equivalent : "ouvre le navigateur", "open shell", "dessine un cercle".')
    lines.append("Un prompt hors slash met a jour #ai-stage (llm=stub_echo).")
    return "\n".join(lines)
