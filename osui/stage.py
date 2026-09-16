#!/usr/bin/env python3
"""Stub reasoner HTML pour la scene IA du bureau Mohhdy.

Le LLM "pense en HTML" : constructions, simulations et dessins se
rendent dans `#ai-stage`. Tant qu'aucun modele reel n'est attache,
`llm=stub_echo`. Le HTML produit passe par une allowlist : pas
d'execution de script. Le guest i386 n'heberge pas cette scene HTML
(voir docs/ETAT_REEL.md).
"""

from __future__ import annotations

import html
import re
from html.parser import HTMLParser

LLM_KIND = "stub_echo"
STAGE_MODES = ("reflecting", "acting", "presenting")
SANITIZER_KIND = "allowlist"

ALLOWED_TAGS = frozenset(
    {
        "div",
        "span",
        "p",
        "h1",
        "h2",
        "h3",
        "ul",
        "ol",
        "li",
        "strong",
        "em",
        "section",
        "article",
        "header",
        "svg",
        "rect",
        "circle",
        "line",
        "path",
        "text",
        "g",
        "polyline",
        "polygon",
        "ellipse",
        "small",
        "code",
        "pre",
        "br",
    }
)
VOIDISH_TAGS = frozenset(
    {
        "rect",
        "circle",
        "line",
        "path",
        "ellipse",
        "polyline",
        "polygon",
        "br",
    }
)
ALLOWED_ATTRS = frozenset(
    {
        "class",
        "id",
        "role",
        "aria-label",
        "data-mode",
        "data-kind",
        "viewbox",
        "width",
        "height",
        "x",
        "y",
        "cx",
        "cy",
        "r",
        "rx",
        "ry",
        "d",
        "fill",
        "stroke",
        "stroke-width",
        "transform",
        "points",
        "x1",
        "y1",
        "x2",
        "y2",
        "text-anchor",
        "opacity",
        "font-size",
    }
)
ATTR_RENAME = {"viewbox": "viewBox"}
DROP_TAGS = frozenset(
    {
        "script",
        "style",
        "iframe",
        "object",
        "embed",
        "link",
        "meta",
        "form",
        "input",
        "textarea",
        "button",
        "base",
        "svgscript",
    }
)
UNSAFE_ATTR_RE = re.compile(
    r"^\s*(javascript:|data:text/html|vbscript:)|expression\s*\(",
    re.IGNORECASE,
)

ACTING_RE = re.compile(
    r"\b(simule|simulation|anime|animer|agir|agis|action|acting|execute|executer)\b",
    re.IGNORECASE,
)
PRESENTING_RE = re.compile(
    r"\b(dessine|dessiner|draw|svg|montre|montrer|presente|presenter|"
    r"resultat|boite|box|scene|figure|cercle|graphe|arbre|horloge)\b",
    re.IGNORECASE,
)
PLAN_RE = re.compile(
    r"\b(plan autonome|mini-plan|etape par etape|autonome|"
    r"agis puis|reflechis puis|multi-step|sequenc)\b",
    re.IGNORECASE,
)
CIRCLE_RE = re.compile(r"\b(cercle|circle|disque)\b", re.IGNORECASE)
GRAPH_RE = re.compile(r"\b(graphe|graph|reseau|nodes?)\b", re.IGNORECASE)
TREE_RE = re.compile(r"\b(arbre|tree|hierarchie)\b", re.IGNORECASE)
CLOCK_RE = re.compile(r"\b(horloge|clock|pendule)\b", re.IGNORECASE)
VFS_RE = re.compile(r"\b(vfs|overlay|initrd|fat16)\b", re.IGNORECASE)


class AllowlistSanitizer(HTMLParser):
    """Reconstruit un fragment HTML en ne gardant que tags/attributs surs."""

    def __init__(self) -> None:
        super().__init__(convert_charrefs=True)
        self.parts: list[str] = []
        self._drop_depth = 0
        self.stripped_scripts = 0

    def handle_starttag(self, tag: str, attrs) -> None:
        self._open(tag, attrs, self_closing=False)

    def handle_startendtag(self, tag: str, attrs) -> None:
        self._open(tag, attrs, self_closing=True)

    def handle_endtag(self, tag: str) -> None:
        tag = tag.lower()
        if self._drop_depth:
            self._drop_depth -= 1
            return
        if tag not in ALLOWED_TAGS or tag in VOIDISH_TAGS:
            return
        self.parts.append("</%s>" % tag)

    def handle_data(self, data: str) -> None:
        if self._drop_depth:
            return
        self.parts.append(html.escape(data, quote=False))

    def handle_comment(self, data: str) -> None:
        return

    def handle_pi(self, data: str) -> None:
        return

    def handle_decl(self, decl: str) -> None:
        return

    def unknown_decl(self, data: str) -> None:
        return

    def _open(self, tag: str, attrs, self_closing: bool) -> None:
        tag = tag.lower()
        if self._drop_depth:
            self._drop_depth += 1
            return
        if tag in DROP_TAGS:
            self._drop_depth = 1
            if tag == "script":
                self.stripped_scripts += 1
            return
        if tag not in ALLOWED_TAGS:
            return
        chunks = ["<", tag]
        for raw_name, raw_value in attrs:
            name = (raw_name or "").lower()
            if name.startswith("on"):
                continue
            if name not in ALLOWED_ATTRS:
                continue
            value = "" if raw_value is None else str(raw_value)
            if UNSAFE_ATTR_RE.search(value):
                continue
            out_name = ATTR_RENAME.get(name, name)
            chunks.append(' %s="%s"' % (out_name, html.escape(value, quote=True)))
        if self_closing or tag in VOIDISH_TAGS:
            chunks.append("></")
            chunks.append(tag)
            chunks.append(">")
        else:
            chunks.append(">")
        self.parts.append("".join(chunks))

    def result(self) -> str:
        return "".join(self.parts)


def sanitize_html(fragment: str) -> tuple[str, int]:
    """Retourne (html_safe, scripts_stripped)."""
    parser = AllowlistSanitizer()
    parser.feed(fragment or "")
    parser.close()
    return parser.result(), parser.stripped_scripts


def classify_mode(prompt: str) -> str:
    text = prompt or ""
    if ACTING_RE.search(text) and not PRESENTING_RE.search(text):
        return "acting"
    if PRESENTING_RE.search(text):
        return "presenting"
    if ACTING_RE.search(text):
        return "acting"
    return "reflecting"


def wants_autonomous_plan(prompt: str, flag: bool | None = None) -> bool:
    if flag is True:
        return True
    if flag is False:
        return False
    return bool(PLAN_RE.search(prompt or ""))


def _caption(prompt: str) -> str:
    clipped = (prompt or "").strip()
    if len(clipped) > 180:
        clipped = clipped[:177] + "..."
    if not clipped:
        clipped = "(prompt vide)"
    return html.escape(clipped, quote=True)


def _scene_reflecting(prompt: str) -> str:
    cap = _caption(prompt)
    return (
        '<div class="ai-scene" data-mode="reflecting" data-kind="stub">'
        '<h2 class="ai-scene-title">Reflexion</h2>'
        '<p class="ai-scene-caption">llm=stub_echo · %s</p>'
        '<ol class="ai-plan">'
        "<li>lire le prompt</li>"
        "<li>contrat guest Ring 3</li>"
        "<li>plan d'acte (stub)</li>"
        "</ol>"
        '<div class="ai-row">'
        '<div class="ai-box">lire</div>'
        '<div class="ai-box">contraindre</div>'
        '<div class="ai-box">agir</div>'
        "</div>"
        "</div>"
    ) % cap


def _scene_acting(prompt: str) -> str:
    cap = _caption(prompt)
    kind = "vfs" if VFS_RE.search(prompt or "") else "generic"
    extra = (
        '<div class="ai-sim-box delay3">overlay</div>'
        if kind == "vfs"
        else ""
    )
    return (
        '<div class="ai-scene" data-mode="acting" data-kind="stub">'
        '<h2 class="ai-scene-title">Action / simulation</h2>'
        '<p class="ai-scene-caption">llm=stub_echo · %s</p>'
        '<div class="ai-sim" aria-label="simulation stub" data-kind="%s">'
        '<div class="ai-sim-box">acte 1</div>'
        '<div class="ai-sim-box delay">acte 2</div>'
        '<div class="ai-sim-box delay2">resultat</div>'
        "%s"
        "</div>"
        "</div>"
    ) % (cap, kind, extra)


def _scene_boxes(prompt: str) -> str:
    cap = _caption(prompt)
    return (
        '<div class="ai-scene" data-mode="presenting" data-kind="boxes">'
        '<h2 class="ai-scene-title">Resultat</h2>'
        '<p class="ai-scene-caption">llm=stub_echo · %s</p>'
        '<svg viewBox="0 0 360 140" width="360" height="140" role="img" '
        'aria-label="boites stub">'
        '<rect x="12" y="24" width="100" height="88" rx="10" fill="#2b7a6e">'
        "</rect>"
        '<rect x="130" y="24" width="100" height="88" rx="10" fill="#3d9a8a">'
        "</rect>"
        '<rect x="248" y="24" width="100" height="88" rx="10" fill="#7ad4c4">'
        "</rect>"
        '<text x="62" y="74" text-anchor="middle" fill="#e8eef4">A</text>'
        '<text x="180" y="74" text-anchor="middle" fill="#0f1419">B</text>'
        '<text x="298" y="74" text-anchor="middle" fill="#0f1419">C</text>'
        "</svg>"
        "</div>"
    ) % cap


def _scene_circle(prompt: str) -> str:
    cap = _caption(prompt)
    return (
        '<div class="ai-scene" data-mode="presenting" data-kind="circle">'
        '<h2 class="ai-scene-title">Dessin</h2>'
        '<p class="ai-scene-caption">llm=stub_echo · %s</p>'
        '<svg viewBox="0 0 240 160" width="240" height="160" role="img" '
        'aria-label="cercle stub">'
        '<circle cx="120" cy="80" r="54" fill="#2b7a6e" stroke="#7ad4c4" '
        'stroke-width="4"></circle>'
        '<circle cx="120" cy="80" r="18" fill="#7ad4c4"></circle>'
        '<text x="120" y="84" text-anchor="middle" fill="#0f1419">IA</text>'
        "</svg>"
        "</div>"
    ) % cap


def _scene_graph(prompt: str) -> str:
    cap = _caption(prompt)
    return (
        '<div class="ai-scene" data-mode="presenting" data-kind="graph">'
        '<h2 class="ai-scene-title">Graphe</h2>'
        '<p class="ai-scene-caption">llm=stub_echo · %s</p>'
        '<svg viewBox="0 0 320 160" width="320" height="160" role="img" '
        'aria-label="graphe stub">'
        '<line x1="50" y1="80" x2="160" y2="36" stroke="#7ad4c4" stroke-width="3">'
        "</line>"
        '<line x1="50" y1="80" x2="160" y2="124" stroke="#7ad4c4" stroke-width="3">'
        "</line>"
        '<line x1="160" y1="36" x2="270" y2="80" stroke="#3d9a8a" stroke-width="3">'
        "</line>"
        '<line x1="160" y1="124" x2="270" y2="80" stroke="#3d9a8a" stroke-width="3">'
        "</line>"
        '<circle cx="50" cy="80" r="16" fill="#2b7a6e"></circle>'
        '<circle cx="160" cy="36" r="16" fill="#3d9a8a"></circle>'
        '<circle cx="160" cy="124" r="16" fill="#3d9a8a"></circle>'
        '<circle cx="270" cy="80" r="16" fill="#7ad4c4"></circle>'
        "</svg>"
        "</div>"
    ) % cap


def _scene_tree(prompt: str) -> str:
    cap = _caption(prompt)
    return (
        '<div class="ai-scene" data-mode="presenting" data-kind="tree">'
        '<h2 class="ai-scene-title">Construction</h2>'
        '<p class="ai-scene-caption">llm=stub_echo · %s</p>'
        '<ul class="ai-tree">'
        "<li>initrd/"
        "<ul><li>bin/shell</li><li>bin/vfsserver</li></ul>"
        "</li>"
        "<li>overlay/ (Ring 3)</li>"
        "<li>fat16/ README.TXT</li>"
        "</ul>"
        "</div>"
    ) % cap


def _scene_clock(prompt: str) -> str:
    cap = _caption(prompt)
    return (
        '<div class="ai-scene" data-mode="presenting" data-kind="clock">'
        '<h2 class="ai-scene-title">Horloge stub</h2>'
        '<p class="ai-scene-caption">llm=stub_echo · %s</p>'
        '<svg viewBox="0 0 160 160" width="160" height="160" role="img" '
        'aria-label="horloge stub">'
        '<circle cx="80" cy="80" r="70" fill="#15222c" stroke="#7ad4c4" '
        'stroke-width="4"></circle>'
        '<line class="ai-clock-hour" x1="80" y1="80" x2="80" y2="40" '
        'stroke="#e8eef4" stroke-width="4"></line>'
        '<line class="ai-clock-min" x1="80" y1="80" x2="120" y2="80" '
        'stroke="#c9a227" stroke-width="3"></line>'
        "</svg>"
        "</div>"
    ) % cap


def _scene_presenting(prompt: str) -> str:
    text = prompt or ""
    if CIRCLE_RE.search(text):
        return _scene_circle(text)
    if GRAPH_RE.search(text):
        return _scene_graph(text)
    if TREE_RE.search(text) or VFS_RE.search(text):
        return _scene_tree(text)
    if CLOCK_RE.search(text):
        return _scene_clock(text)
    return _scene_boxes(text)


def render_stub_html(prompt: str, mode: str) -> str:
    if mode == "acting":
        return _scene_acting(prompt)
    if mode == "presenting":
        return _scene_presenting(prompt)
    return _scene_reflecting(prompt)


def _pack(prompt: str, mode: str, raw: str, extra: dict | None = None) -> dict:
    safe, stripped = sanitize_html(raw)
    payload = {
        "status": "ok",
        "llm": LLM_KIND,
        "mode": mode,
        "modes": list(STAGE_MODES),
        "prompt": prompt,
        "html": safe,
        "sanitizer": SANITIZER_KIND,
        "scripts_stripped": stripped,
        "kind": "scene",
        "autonomous": False,
        "label": "stub",
        "step_index": 0,
        "step_count": 1,
        "done": True,
        "delay_ms": 0,
        "guest_html_stage": False,
        "note": (
            "stub local, pas un LLM de production. "
            "Scene HTML du bootstrap osui ; le guest i386 n'heberge pas #ai-stage."
        ),
    }
    if extra:
        payload.update(extra)
    return payload


def reason_stage(prompt: str, autonomous: bool | None = None) -> dict:
    """Produit une scene HTML stub (safe), ou un mini-plan autonome etiquete."""
    text = (prompt or "").strip()
    if wants_autonomous_plan(text, autonomous):
        return reason_plan(text)
    mode = classify_mode(text)
    return _pack(text, mode, render_stub_html(text, mode))


def reason_plan(prompt: str) -> dict:
    """Mini-plan stub : reflecting -> acting -> presenting."""
    text = (prompt or "").strip() or "mini-plan autonome stub"
    steps_src = (
        ("reflecting", "lire", _scene_reflecting(text)),
        ("acting", "agir", _scene_acting(text)),
        ("presenting", "presenter", _scene_presenting(text)),
    )
    steps = []
    for index, (mode, label, raw) in enumerate(steps_src):
        safe, stripped = sanitize_html(raw)
        steps.append(
            {
                "index": index,
                "mode": mode,
                "label": label,
                "html": safe,
                "scripts_stripped": stripped,
                "delay_ms": 450 if index < 2 else 0,
            }
        )
    first = steps[0]
    return _pack(
        text,
        first["mode"],
        first["html"],
        {
            "html": first["html"],
            "kind": "plan",
            "autonomous": True,
            "label": "stub",
            "plan_label": "stub",
            "steps": steps,
            "step_index": 0,
            "step_count": len(steps),
            "done": False,
            "delay_ms": first["delay_ms"],
            "note": (
                "mini-plan autonome stub (pas un LLM de production). "
                "Etapes via POST /api/os/stage/tick. Guest VGA sans #ai-stage."
            ),
        },
    )


class StageState:
    """Derniere scene servie par l'instance osui, y compris mini-plans."""

    def __init__(self) -> None:
        self.reset()

    def reset(self) -> None:
        idle = reason_stage("")
        idle["mode"] = "reflecting"
        idle["html"] = (
            '<div class="ai-scene" data-mode="reflecting" data-kind="idle">'
            '<h2 class="ai-scene-title">Scene IA</h2>'
            '<p class="ai-scene-caption">llm=stub_echo · en attente d\'un prompt</p>'
            '<div class="ai-row">'
            '<div class="ai-box muted">reflexion</div>'
            '<div class="ai-box muted">action</div>'
            '<div class="ai-box muted">resultats</div>'
            "</div>"
            "</div>"
        )
        idle["prompt"] = ""
        idle["kind"] = "idle"
        self.current = idle
        self.plan_steps: list[dict] = []

    def apply_prompt(self, prompt: str, autonomous: bool | None = None) -> dict:
        payload = reason_stage(prompt, autonomous=autonomous)
        self.current = dict(payload)
        self.plan_steps = list(payload.get("steps") or [])
        return dict(self.current)

    def tick(self) -> dict:
        steps = self.plan_steps
        if not steps:
            current = dict(self.current)
            current["done"] = True
            current["autonomous"] = False
            return current
        index = int(self.current.get("step_index") or 0) + 1
        if index >= len(steps):
            last = dict(self.current)
            last["done"] = True
            last["delay_ms"] = 0
            self.current = last
            return dict(last)
        step = steps[index]
        payload = dict(self.current)
        payload["mode"] = step["mode"]
        payload["html"] = step["html"]
        payload["step_index"] = index
        payload["done"] = index >= len(steps) - 1
        payload["delay_ms"] = 0 if payload["done"] else step.get("delay_ms", 450)
        payload["kind"] = "plan"
        payload["autonomous"] = True
        payload["label"] = "stub"
        self.current = payload
        return dict(payload)

    def snapshot(self) -> dict:
        return dict(self.current)


def public_stage_meta() -> dict:
    return {
        "id": "ai-stage",
        "modes": list(STAGE_MODES),
        "llm": LLM_KIND,
        "sanitizer": SANITIZER_KIND,
        "scripts": False,
        "guest_html_stage": False,
        "autonomous_plans": True,
        "plan_label": "stub",
        "tick": "/api/os/stage/tick",
    }
