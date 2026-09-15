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
    r"resultat|boite|box|scene|figure)\b",
    re.IGNORECASE,
)


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
    if ACTING_RE.search(text):
        return "acting"
    if PRESENTING_RE.search(text):
        return "presenting"
    return "reflecting"


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
        '<div class="ai-row">'
        '<div class="ai-box">lire le prompt</div>'
        '<div class="ai-box">contrat guest Ring 3</div>'
        '<div class="ai-box">plan d\'acte</div>'
        "</div>"
        "</div>"
    ) % cap


def _scene_acting(prompt: str) -> str:
    cap = _caption(prompt)
    return (
        '<div class="ai-scene" data-mode="acting" data-kind="stub">'
        '<h2 class="ai-scene-title">Action / simulation</h2>'
        '<p class="ai-scene-caption">llm=stub_echo · %s</p>'
        '<div class="ai-sim" aria-label="simulation stub">'
        '<div class="ai-sim-box">acte 1</div>'
        '<div class="ai-sim-box delay">acte 2</div>'
        '<div class="ai-sim-box delay2">resultat</div>'
        "</div>"
        "</div>"
    ) % cap


def _scene_presenting(prompt: str) -> str:
    cap = _caption(prompt)
    return (
        '<div class="ai-scene" data-mode="presenting" data-kind="stub">'
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


def render_stub_html(prompt: str, mode: str) -> str:
    if mode == "acting":
        return _scene_acting(prompt)
    if mode == "presenting":
        return _scene_presenting(prompt)
    return _scene_reflecting(prompt)


def reason_stage(prompt: str) -> dict:
    """Produit une scene HTML stub (safe) pour un prompt utilisateur."""
    text = (prompt or "").strip()
    mode = classify_mode(text)
    raw = render_stub_html(text, mode)
    safe, stripped = sanitize_html(raw)
    return {
        "status": "ok",
        "llm": LLM_KIND,
        "mode": mode,
        "modes": list(STAGE_MODES),
        "prompt": text,
        "html": safe,
        "sanitizer": SANITIZER_KIND,
        "scripts_stripped": stripped,
        "note": (
            "stub local, pas un LLM de production. "
            "Scene HTML du bootstrap osui ; le guest i386 n'heberge pas #ai-stage."
        ),
    }


class StageState:
    """Derniere scene servie par l'instance osui."""

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
        self.current = idle

    def apply_prompt(self, prompt: str) -> dict:
        payload = reason_stage(prompt)
        self.current = dict(payload)
        return payload

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
    }
